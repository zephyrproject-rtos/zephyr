/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 * Copyright (c) 2024, Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_twi.h>
#include "i2c_nrfx_twi_common.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_nrfx_twi, CONFIG_I2C_LOG_LEVEL);

struct i2c_nrfx_twi_rtio_data {
	uint32_t dev_config;
	bool twi_enabled;
	struct i2c_rtio *ctx;
};

/* Enforce dev_config matches the same offset as the common structure,
 * otherwise common API won't be compatible with i2c_nrfx_twi_rtio.
 */
BUILD_ASSERT(
	offsetof(struct i2c_nrfx_twi_rtio_data, dev_config) ==
	offsetof(struct i2c_nrfx_twi_common_data, dev_config)
);

static void i2c_nrfx_twi_rtio_complete(const struct device *dev, int status);

static bool i2c_nrfx_twi_rtio_msg_start(const struct device *dev, uint8_t flags,
					uint8_t *buf, size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_rtio_data *const dev_data = dev->data;
	struct i2c_rtio *ctx = dev_data->ctx;
	int ret = 0;

	/** Enabling while already enabled ends up in a failed assertion: skip it. */
	if (!dev_data->twi_enabled) {
		nrfx_twi_enable(&config->twi);
		dev_data->twi_enabled = true;
	}

	ret = i2c_nrfx_twi_msg_transfer(dev, flags, buf, buf_len, i2c_addr, false);
	if (ret != 0) {
		nrfx_twi_disable(&config->twi);
		dev_data->twi_enabled = false;

		return i2c_rtio_complete(ctx, ret);
	}

	return false;
}

static bool i2c_nrfx_twi_rtio_start(const struct device *dev)
{
	struct i2c_nrfx_twi_rtio_data *const dev_data = dev->data;
	struct i2c_rtio *ctx = dev_data->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;

	switch (sqe->op) {
	case RTIO_OP_RX:
		return i2c_nrfx_twi_rtio_msg_start(dev, I2C_MSG_READ | sqe->iodev_flags,
						   sqe->rx.buf, sqe->rx.buf_len, dt_spec->addr);
	case RTIO_OP_TINY_TX:
		return i2c_nrfx_twi_rtio_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
						   (uint8_t *)sqe->tiny_tx.buf,
						   sqe->tiny_tx.buf_len, dt_spec->addr);
	case RTIO_OP_TX:
		return i2c_nrfx_twi_rtio_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
						   (uint8_t *)sqe->tx.buf,
						   sqe->tx.buf_len, dt_spec->addr);
	case RTIO_OP_I2C_CONFIGURE:
		(void)i2c_nrfx_twi_configure(dev, sqe->i2c_config);
		return false;
	case RTIO_OP_I2C_RECOVER:
		(void)i2c_nrfx_twi_recover_bus(dev);
		return false;
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(ctx, -EINVAL);
	}
}

static void i2c_nrfx_twi_rtio_complete(const struct device *dev, int status)
{
	/** Finalize if there are no more pending xfers */
	const struct i2c_nrfx_twi_config *config = dev->config;
	struct i2c_nrfx_twi_rtio_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_complete(ctx, status)) {
		(void)i2c_nrfx_twi_rtio_start(dev);
	} else {
		nrfx_twi_disable(&config->twi);
		data->twi_enabled = false;
	}
}

static int i2c_nrfx_twi_rtio_configure(const struct device *dev, uint32_t i2c_config)
{
	struct i2c_rtio *const ctx = ((struct i2c_nrfx_twi_rtio_data *)
		dev->data)->ctx;

	return i2c_rtio_configure(ctx, i2c_config);
}

static int i2c_nrfx_twi_rtio_transfer(const struct device *dev, struct i2c_msg *msgs,
				      uint8_t num_msgs, uint16_t addr)
{
	struct i2c_rtio *const ctx = ((struct i2c_nrfx_twi_rtio_data *)
		dev->data)->ctx;

	return i2c_rtio_transfer(ctx, msgs, num_msgs, addr);
}

static int i2c_nrfx_twi_rtio_recover_bus(const struct device *dev)
{
	struct i2c_rtio *const ctx = ((struct i2c_nrfx_twi_rtio_data *)
		dev->data)->ctx;

	return i2c_rtio_recover(ctx);
}

static void event_handler(nrfx_twi_evt_t const *p_event, void *p_context)
{
	const struct device *dev = p_context;
	int status = 0;

	if (i2c_nrfx_twi_get_evt_result(p_event) != NRFX_SUCCESS) {
		status = -EIO;
	}

	i2c_nrfx_twi_rtio_complete(dev, status);
}

static void i2c_nrfx_twi_rtio_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_seq)
{
	struct i2c_nrfx_twi_rtio_data *data = dev->data;
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_submit(ctx, iodev_seq)) {
		(void)i2c_nrfx_twi_rtio_start(dev);
	}
}

static const struct i2c_driver_api i2c_nrfx_twi_rtio_driver_api = {
	.configure   = i2c_nrfx_twi_rtio_configure,
	.transfer    = i2c_nrfx_twi_rtio_transfer,
	.recover_bus = i2c_nrfx_twi_rtio_recover_bus,
	.iodev_submit = i2c_nrfx_twi_rtio_submit,
};

#define I2C_NRFX_TWI_RTIO_DEVICE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(I2C(idx));			       \
	BUILD_ASSERT(I2C_FREQUENCY(idx)	!=				       \
		     I2C_NRFX_TWI_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static int twi_##idx##_init(const struct device *dev)		       \
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twi_##idx##_irq_handler, 0);	       \
		const struct i2c_nrfx_twi_config *config = dev->config;	       \
		const struct i2c_nrfx_twi_rtio_data *dev_data = dev->data;     \
		int err = pinctrl_apply_state(config->pcfg,		       \
					      PINCTRL_STATE_DEFAULT);	       \
		if (err < 0) {						       \
			return err;					       \
		}							       \
		i2c_rtio_init(dev_data->ctx, dev);			       \
		return i2c_nrfx_twi_init(dev);				       \
	}								       \
	I2C_RTIO_DEFINE(_i2c##idx##_twi_rtio,				       \
			DT_INST_PROP_OR(n, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),  \
			DT_INST_PROP_OR(n, cq_size, CONFIG_I2C_RTIO_CQ_SIZE)); \
	static struct i2c_nrfx_twi_rtio_data twi_##idx##_data = {	       \
		.ctx = &_i2c##idx##_twi_rtio,				       \
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
		      &i2c_nrfx_twi_rtio_driver_api)

#ifdef CONFIG_HAS_HW_NRF_TWI0
I2C_NRFX_TWI_RTIO_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWI1
I2C_NRFX_TWI_RTIO_DEVICE(1);
#endif
