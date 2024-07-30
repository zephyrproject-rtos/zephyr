/*
 * Copyright (c) 2018-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_twi.h>
#include "i2c_nrfx_twi_common.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_nrfx_twi, CONFIG_I2C_LOG_LEVEL);

#if CONFIG_I2C_NRFX_TRANSFER_TIMEOUT
#define TWI_TRANSFER_TIMEOUT(_num_msgs) K_MSEC(CONFIG_I2C_NRFX_TRANSFER_TIMEOUT * _num_msgs)
#else
#define TWI_TRANSFER_TIMEOUT(_num_msgs) K_FOREVER
#endif

struct i2c_nrfx_twi_data {
	uint32_t dev_config;
	const struct device *dev;
	struct k_sem transfer_sync;
	struct k_sem completion_sync;
	struct k_work transfer_work;
	struct k_work_delayable timeout_dwork;
	struct i2c_msg *transfer_msgs;
	uint8_t transfer_num_msgs;
	uint16_t transfer_addr;
	uint16_t transfer_msg_idx;
	bool transfer_ok;
#if CONFIG_I2C_CALLBACK
	i2c_callback_t transfer_callback;
	void *transfer_userdata;
#endif
};

/* Enforce dev_config matches the same offset as the common structure,
 * otherwise common API won't be compatible with i2c_nrfx_twi.
 */
BUILD_ASSERT(
	offsetof(struct i2c_nrfx_twi_data, dev_config) ==
	offsetof(struct i2c_nrfx_twi_common_data, dev_config)
);

static void twi_transfer_lock(const struct device *dev)
{
	struct i2c_nrfx_twi_data *data = dev->data;

	(void)k_sem_take(&data->transfer_sync, K_FOREVER);
}

#if CONFIG_I2C_CALLBACK
static bool twi_transfer_try_lock(const struct device *dev)
{
	struct i2c_nrfx_twi_data *data = dev->data;

	return k_sem_take(&data->transfer_sync, K_NO_WAIT) == 0;
}

static void twi_transfer_set_callback(const struct device *dev,
				      i2c_callback_t cb,
				      void *userdata)
{
	struct i2c_nrfx_twi_data *data = dev->data;

	data->transfer_callback = cb;
	data->transfer_userdata = userdata;
}
#endif

static void twi_transfer_start(const struct device *dev,
			       struct i2c_msg *msgs,
			       uint8_t num_msgs,
			       uint16_t addr)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_data *data = dev->data;

	data->transfer_msgs = msgs;
	data->transfer_num_msgs = num_msgs;
	data->transfer_addr = addr;
	data->transfer_msg_idx = 0;
	data->transfer_ok = true;

	nrfx_twi_enable(&config->twi);
	k_sem_reset(&data->completion_sync);
	k_work_schedule(&data->timeout_dwork, TWI_TRANSFER_TIMEOUT(num_msgs));
	k_work_submit(&data->transfer_work);
}

static int twi_transfer_await_done(const struct device *dev)
{
	struct i2c_nrfx_twi_data *data = dev->data;

	return k_sem_take(&data->completion_sync, K_FOREVER);
}

static void twi_transfer_stop(const struct device *dev)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_data *data = dev->data;
	int ret;

#if CONFIG_I2C_CALLBACK
	i2c_callback_t callback;
	void *userdata;
#endif

	nrfx_twi_disable(&config->twi);

	ret = data->transfer_ok ? 0 : -EIO;

	if (ret < 0) {
		(void)i2c_nrfx_twi_recover_bus(dev);
		k_sem_reset(&data->completion_sync);
	} else {
		k_sem_give(&data->completion_sync);
	}

#if CONFIG_I2C_CALLBACK
	callback = data->transfer_callback;
	userdata = data->transfer_userdata;
	data->transfer_callback = NULL;
	data->transfer_userdata = NULL;
#endif

	k_work_cancel(&data->transfer_work);
	k_work_cancel_delayable(&data->timeout_dwork);
	k_sem_give(&data->transfer_sync);

#if CONFIG_I2C_CALLBACK
	if (callback != NULL) {
		callback(dev, ret, userdata);
	}
#endif
}

static int twi_transfer_msg(const struct device *dev)
{
	struct i2c_nrfx_twi_data *data = dev->data;
	bool more_msgs;

	more_msgs = data->transfer_msg_idx < (data->transfer_num_msgs - 1) &&
		    !(data->transfer_msgs[data->transfer_msg_idx + 1].flags & I2C_MSG_RESTART);

	return i2c_nrfx_twi_msg_transfer(dev,
					 data->transfer_msgs[data->transfer_msg_idx].flags,
					 data->transfer_msgs[data->transfer_msg_idx].buf,
					 data->transfer_msgs[data->transfer_msg_idx].len,
					 data->transfer_addr,
					 more_msgs);
}

static void twi_transfer_handler(struct k_work *work)
{
	struct i2c_nrfx_twi_data *data =
		CONTAINER_OF(work, struct i2c_nrfx_twi_data, transfer_work);
	const struct device *dev = data->dev;

	if (!data->transfer_ok ||
	    data->transfer_msg_idx == data->transfer_num_msgs ||
	    twi_transfer_msg(dev) < 0) {
		twi_transfer_stop(dev);
		return;
	}

	data->transfer_msg_idx++;
}

static void twi_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct i2c_nrfx_twi_data *data =
		CONTAINER_OF(dwork, struct i2c_nrfx_twi_data, timeout_dwork);
	const struct device *dev = data->dev;

	data->transfer_ok = false;
	twi_transfer_stop(dev);
}

static int i2c_nrfx_twi_transfer(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	if (num_msgs == 0) {
		return 0;
	}

	twi_transfer_lock(dev);
	twi_transfer_start(dev, msgs, num_msgs, addr);
	return twi_transfer_await_done(dev);
}

#if CONFIG_I2C_CALLBACK
static int i2c_nrfx_twi_transfer_cb(const struct device *dev,
				    struct i2c_msg *msgs,
				    uint8_t num_msgs,
				    uint16_t addr,
				    i2c_callback_t cb,
				    void *userdata)
{
	if (num_msgs == 0) {
		if (cb != NULL) {
			cb(dev, 0, userdata);
		}
		return 0;
	}

	if (!twi_transfer_try_lock(dev)) {
		return -EWOULDBLOCK;
	}

	twi_transfer_set_callback(dev, cb, userdata);
	twi_transfer_start(dev, msgs, num_msgs, addr);
	return 0;
}
#endif

static void event_handler(nrfx_twi_evt_t const *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct i2c_nrfx_twi_data *data = dev->data;

	data->transfer_ok = p_event->type == NRFX_TWI_EVT_DONE;
	k_work_submit(&data->transfer_work);
}

static const struct i2c_driver_api i2c_nrfx_twi_driver_api = {
	.configure   = i2c_nrfx_twi_configure,
	.transfer    = i2c_nrfx_twi_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i2c_nrfx_twi_transfer_cb,
#endif
	.recover_bus = i2c_nrfx_twi_recover_bus,
};

#define I2C_NRFX_TWI_DEVICE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(I2C(idx));			       \
	BUILD_ASSERT(I2C_FREQUENCY(idx)	!=				       \
		     I2C_NRFX_TWI_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twi_##idx##_init(const struct device *dev)		       \
	{								       \
		struct i2c_nrfx_twi_data *data = dev->data;		       \
		data->dev = dev;					       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twi_##idx##_irq_handler, 0);	       \
		const struct i2c_nrfx_twi_config *config = dev->config;	       \
		int err = pinctrl_apply_state(config->pcfg,		       \
					      PINCTRL_STATE_DEFAULT);	       \
		if (err < 0) {						       \
			return err;					       \
		}							       \
		return i2c_nrfx_twi_init(dev);				       \
	}								       \
	static struct i2c_nrfx_twi_data twi_##idx##_data = {		       \
		.transfer_sync = Z_SEM_INITIALIZER(                            \
			twi_##idx##_data.transfer_sync, 1, 1),                 \
		.completion_sync = Z_SEM_INITIALIZER(                          \
			twi_##idx##_data.completion_sync, 1, 1),               \
		.transfer_work = Z_WORK_INITIALIZER(twi_transfer_handler),     \
		.timeout_dwork = Z_WORK_DELAYABLE_INITIALIZER(		       \
			twi_timeout_handler),				       \
	};								       \
	PINCTRL_DT_DEFINE(I2C(idx));					       \
	static const struct i2c_nrfx_twi_config twi_##idx##z_config = {	       \
		.twi = NRFX_TWI_INSTANCE(idx),				       \
		.config = {						       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.frequency = I2C_FREQUENCY(idx),		       \
		},							       \
		.event_handler = event_handler,				       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(I2C(idx)),		       \
	};								       \
	PM_DEVICE_DT_DEFINE(I2C(idx), twi_nrfx_pm_action);		       \
	I2C_DEVICE_DT_DEFINE(I2C(idx),					       \
		      twi_##idx##_init,					       \
		      PM_DEVICE_DT_GET(I2C(idx)),			       \
		      &twi_##idx##_data,				       \
		      &twi_##idx##z_config,				       \
		      POST_KERNEL,					       \
		      CONFIG_I2C_INIT_PRIORITY,				       \
		      &i2c_nrfx_twi_driver_api)

#ifdef CONFIG_HAS_HW_NRF_TWI0
I2C_NRFX_TWI_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWI1
I2C_NRFX_TWI_DEVICE(1);
#endif
