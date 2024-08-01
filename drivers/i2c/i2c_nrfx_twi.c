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
#include "i2c_context.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_nrfx_twi, CONFIG_I2C_LOG_LEVEL);

struct i2c_nrfx_twi_data {
	uint32_t dev_config;
	struct i2c_context ctx;
};

/* Enforce dev_config matches the same offset as the common structure,
 * otherwise common API won't be compatible with i2c_nrfx_twi.
 */
BUILD_ASSERT(
	offsetof(struct i2c_nrfx_twi_data, dev_config) ==
	offsetof(struct i2c_nrfx_twi_common_data, dev_config)
);

static int twi_init_transfer_handler(struct i2c_context *ctx)
{
	const struct device *dev = ctx->dev;
	const struct i2c_nrfx_twi_config *config = dev->config;

	nrfx_twi_enable(&config->twi);
	return 0;
}

static void twi_start_transfer_handler(struct i2c_context *ctx)
{
	const struct device *dev = i2c_context_get_dev(ctx);
	uint8_t msg_idx = i2c_context_get_transfer_msg_idx(ctx);
	uint8_t num_msgs = i2c_context_get_transfer_num_msgs(ctx);
	struct i2c_msg *msgs = i2c_context_get_transfer_msgs(ctx);
	uint16_t addr = i2c_context_get_transfer_addr(ctx);
	int ret;

	bool more_msgs = msg_idx < (num_msgs - 1) &&
			 !(msgs[msg_idx + 1].flags & I2C_MSG_RESTART);

	ret = i2c_nrfx_twi_msg_transfer(dev,
					msgs[msg_idx].flags,
					msgs[msg_idx].buf,
					msgs[msg_idx].len,
					addr,
					more_msgs);

	if (ret) {
		i2c_context_cancel_transfer(ctx);
	}
}

static void twi_post_transfer_handler(struct i2c_context *ctx)
{
	ARG_UNUSED(ctx);
}

static void twi_deinit_transfer_handler(struct i2c_context *ctx)
{
	const struct device *dev = i2c_context_get_dev(ctx);
	const struct i2c_nrfx_twi_config *config = dev->config;
	int result = i2c_context_get_transfer_result(ctx);

	nrfx_twi_disable(&config->twi);

	if (result) {
		/* Try to recover bus in case of error */
		(void)i2c_nrfx_twi_recover_bus(dev);
	}
}

static int i2c_nrfx_twi_transfer(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	struct i2c_nrfx_twi_data *data = dev->data;

	return i2c_context_start_transfer(&data->ctx,
					  msgs,
					  num_msgs,
					  addr);
}

#if defined(CONFIG_I2C_CALLBACK)
static int i2c_nrfx_twi_transfer_cb(const struct device *dev,
				    struct i2c_msg *msgs,
				    uint8_t num_msgs,
				    uint16_t addr,
				    i2c_callback_t cb,
				    void *userdata)
{
	struct i2c_nrfx_twi_data *data = dev->data;

	return i2c_context_start_transfer_cb(&data->ctx,
					     msgs,
					     num_msgs,
					     addr,
					     cb,
					     userdata);
}
#endif

static void event_handler(nrfx_twi_evt_t const *p_event, void *p_context)
{
	const struct device *dev = p_context;
	struct i2c_nrfx_twi_data *data = (struct i2c_nrfx_twi_data *)dev->data;
	struct i2c_context *ctx = &data->ctx;

	if (p_event->type == NRFX_TWI_EVT_DONE) {
		i2c_context_continue_transfer(ctx);
	} else {
		i2c_context_cancel_transfer(ctx);
	}
}

static const struct i2c_driver_api i2c_nrfx_twi_driver_api = {
	.configure   = i2c_nrfx_twi_configure,
	.transfer    = i2c_nrfx_twi_transfer,
	.recover_bus = i2c_nrfx_twi_recover_bus,
#if defined(CONFIG_I2C_CALLBACK)
	.transfer_cb = i2c_nrfx_twi_transfer_cb,
#endif
};

#define I2C_NRFX_TWI_DEVICE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(I2C(idx));			       \
	BUILD_ASSERT(I2C_FREQUENCY(idx)	!=				       \
		     I2C_NRFX_TWI_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twi_##idx##_init(const struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twi_##idx##_irq_handler, 0);	       \
		const struct i2c_nrfx_twi_config *config = dev->config;	       \
		int err = pinctrl_apply_state(config->pcfg,		       \
					      PINCTRL_STATE_DEFAULT);	       \
		if (err < 0) {						       \
			return err;					       \
		}							       \
		struct i2c_nrfx_twi_data *data = dev->data;		       \
		i2c_context_init(&data->ctx,				       \
				 dev,					       \
				 twi_init_transfer_handler,		       \
				 twi_start_transfer_handler,		       \
				 twi_post_transfer_handler,		       \
				 twi_deinit_transfer_handler);		       \
		return i2c_nrfx_twi_init(dev);				       \
	}								       \
	static struct i2c_nrfx_twi_data twi_##idx##_data;		       \
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
