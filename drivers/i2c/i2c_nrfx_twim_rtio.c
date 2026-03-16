/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 * Copyright (c) 2024, Croxel Inc
 * Copyright (c) 2024, Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_twim.h>
#include <zephyr/linker/devicetree_regions.h>

#include "i2c_nrfx_twim_common.h"

LOG_MODULE_REGISTER(i2c_nrfx_twim, CONFIG_I2C_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_twim

struct i2c_nrfx_twim_rtio_config {
	struct i2c_nrfx_twim_common_config common;
	struct i2c_rtio *ctx;
};

struct i2c_nrfx_twim_rtio_data {
	nrfx_twim_t twim;
	uint8_t *user_rx_buf;
	uint16_t user_rx_buf_size;
};

static bool i2c_nrfx_twim_rtio_msg_start(const struct device *dev, uint8_t flags, uint8_t *buf,
					 size_t buf_len, uint16_t i2c_addr)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_rtio *ctx = config->ctx;
	int ret = 0;

	ret = i2c_nrfx_twim_msg_transfer(dev, flags, buf, buf_len, i2c_addr);
	if (ret != 0) {
		return i2c_rtio_complete(ctx, ret);
	}

	return false;
}

static void i2c_nrfx_twim_rtio_complete(const struct device *dev, int status);

static void i2c_nrfx_twim_rtio_sqe_signaled(struct rtio_iodev_sqe *iodev_sqe, void *userdata)
{
	const struct device *dev = userdata;

	(void)i2c_nrfx_twim_rtio_complete(dev, 0);
}

static bool i2c_nrfx_twim_rtio_start(const struct device *dev)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_nrfx_twim_rtio_data *data = dev->data;
	struct i2c_rtio *ctx = config->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;
	struct rtio_iodev_sqe *iodev_sqe;

	switch (sqe->op) {
	case RTIO_OP_RX:
		if (!nrf_dma_accessible_check(&config->common.twim, sqe->rx.buf)) {
			if (sqe->rx.buf_len > config->common.msg_buf_size) {
				return i2c_rtio_complete(ctx, -ENOSPC);
			}

			data->user_rx_buf = sqe->rx.buf;
			data->user_rx_buf_size = sqe->rx.buf_len;
			return i2c_nrfx_twim_rtio_msg_start(dev,
							    I2C_MSG_READ | sqe->iodev_flags,
							    config->common.msg_buf,
							    data->user_rx_buf_size,
							    dt_spec->addr);
		}

		data->user_rx_buf = NULL;
		return i2c_nrfx_twim_rtio_msg_start(dev, I2C_MSG_READ | sqe->iodev_flags,
						    sqe->rx.buf, sqe->rx.buf_len, dt_spec->addr);
	case RTIO_OP_TINY_TX:
		return i2c_nrfx_twim_rtio_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
						    sqe->tiny_tx.buf, sqe->tiny_tx.buf_len,
						    dt_spec->addr);
	case RTIO_OP_TX:
		/* If buffer is not accessible by DMA, copy it into the internal driver buffer */
		if (!nrf_dma_accessible_check(&config->common.twim, sqe->tx.buf)) {
			/* Validate buffer will fit */
			if (sqe->tx.buf_len > config->common.msg_buf_size) {
				LOG_ERR("Need to use the internal driver "
					"buffer but its size is insufficient "
					"(%u > %u). "
					"Adjust the zephyr,concat-buf-size or "
					"zephyr,flash-buf-max-size property "
					"(the one with greater value) in the "
					"\"%s\"' node.",
					sqe->tx.buf_len, config->common.msg_buf_size, dev->name);
				return i2c_rtio_complete(ctx, -ENOSPC);
			}
			memcpy(config->common.msg_buf, sqe->tx.buf, sqe->tx.buf_len);
			sqe->tx.buf = config->common.msg_buf;
		}
		return i2c_nrfx_twim_rtio_msg_start(dev, I2C_MSG_WRITE | sqe->iodev_flags,
						    (uint8_t *)sqe->tx.buf, sqe->tx.buf_len,
						    dt_spec->addr);
	case RTIO_OP_I2C_CONFIGURE:
		(void)i2c_nrfx_twim_configure(dev, sqe->i2c_config);
		/** This request will not generate an event therefore, this
		 * code immediately submits a CQE in order to unblock
		 * i2c_rtio_configure.
		 */
		return i2c_rtio_complete(ctx, 0);
	case RTIO_OP_I2C_RECOVER:
		(void)i2c_nrfx_twim_recover_bus(dev);
		return false;
	case RTIO_OP_AWAIT:
		iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);
		rtio_iodev_sqe_await_signal(iodev_sqe, i2c_nrfx_twim_rtio_sqe_signaled,
					    (void *)dev);
		return false;
	default:
		LOG_ERR("Invalid op code %d for submission %p\n", sqe->op, (void *)sqe);
		return i2c_rtio_complete(ctx, -EINVAL);
	}
}

static void i2c_nrfx_twim_rtio_complete(const struct device *dev, int status)
{
	/** Finalize if there are no more pending xfers */
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_rtio *ctx = config->ctx;

	if (i2c_rtio_complete(ctx, status)) {
		(void)i2c_nrfx_twim_rtio_start(dev);
	} else {
		/* Release bus on completion */
		pm_device_runtime_put(dev);
	}
}

static int i2c_nrfx_twim_rtio_configure(const struct device *dev, uint32_t i2c_config)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_rtio *ctx = config->ctx;

	return i2c_rtio_configure(ctx, i2c_config);
}

static int i2c_nrfx_twim_rtio_transfer(const struct device *dev, struct i2c_msg *msgs,
				       uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_rtio *ctx = config->ctx;

	return i2c_rtio_transfer(ctx, msgs, num_msgs, addr);
}

static int i2c_nrfx_twim_rtio_recover_bus(const struct device *dev)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_rtio *ctx = config->ctx;

	return i2c_rtio_recover(ctx);
}

static void i2c_nrfx_twim_rtio_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_seq)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_rtio *ctx = config->ctx;

	if (i2c_rtio_submit(ctx, iodev_seq)) {
		if (pm_device_runtime_get(dev) < 0) {
			(void)i2c_rtio_complete(ctx, -EINVAL);
		} else {
			(void)i2c_nrfx_twim_rtio_start(dev);
		}
	}
}

static void event_handler(nrfx_twim_event_t const *p_event, void *p_context)
{
	const struct device *dev = p_context;
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_nrfx_twim_rtio_data *data = dev->data;
	int status = p_event->type == NRFX_TWIM_EVT_DONE ? 0 : -EIO;

	if (data->user_rx_buf) {
		memcpy(data->user_rx_buf, config->common.msg_buf, data->user_rx_buf_size);
	}

	i2c_nrfx_twim_rtio_complete(dev, status);
}

static DEVICE_API(i2c, i2c_nrfx_twim_driver_api) = {
	.configure = i2c_nrfx_twim_rtio_configure,
	.transfer = i2c_nrfx_twim_rtio_transfer,
	.recover_bus = i2c_nrfx_twim_rtio_recover_bus,
	.iodev_submit = i2c_nrfx_twim_rtio_submit,
};

static int i2c_nrfx_twim_rtio_init(const struct device *dev)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;

	i2c_rtio_init(config->ctx, dev);
	return i2c_nrfx_twim_common_init(dev);
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int i2c_nrfx_twim_rtio_deinit(const struct device *dev)
{
	return i2c_nrfx_twim_common_deinit(dev);
}
#endif

#define MSG_BUF_HAS_MEMORY_REGIONS(inst) DT_INST_NODE_HAS_PROP(inst, memory_regions)

#define MSG_BUF_LINKER_REGION_NAME(inst) \
	LINKER_DT_NODE_REGION_NAME(DT_INST_PHANDLE(inst, memory_regions))

#define MSG_BUF_ATTR_SECTION(inst) \
	__attribute__((__section__(MSG_BUF_LINKER_REGION_NAME(inst))))

#define MSG_BUF_ATTR(inst)			  \
	COND_CODE_1(				  \
		MSG_BUF_HAS_MEMORY_REGIONS(inst), \
		(MSG_BUF_ATTR_SECTION(inst)),	  \
		()				  \
	)

#define MSG_BUF_SYM(inst) \
	CONCAT(twim_, inst, _msg_buf)

#define MSG_BUF_DEFINE(inst) \
	static uint8_t MSG_BUF_SYM(inst)[MSG_BUF_SIZE(inst)] MSG_BUF_ATTR(inst)

#define I2C_NRFX_TWIM_RTIO_DEVICE(inst)								\
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(DT_DRV_INST(inst));					\
	NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(DT_DRV_INST(inst));			\
	BUILD_ASSERT(I2C_FREQUENCY(inst) != I2C_NRFX_TWIM_INVALID_FREQUENCY,			\
		     "Wrong I2C " #inst " frequency setting in dts");				\
	static struct i2c_nrfx_twim_rtio_data twim_##inst##z_data = {				\
		.twim =										\
			{									\
				.p_twim = (NRF_TWIM_Type *)DT_INST_REG_ADDR(inst),		\
			},									\
	};											\
	NRF_DT_INST_IRQ_DIRECT_DEFINE(								\
		inst,										\
		nrfx_twim_irq_handler,								\
		&CONCAT(twim_, inst, z_data.twim)						\
	)											\
	static void pre_init##inst(void)							\
	{											\
		twim_##inst##z_data.twim.p_twim = (NRF_TWIM_Type *)DT_INST_REG_ADDR(inst);	\
		NRF_DT_INST_IRQ_CONNECT(							\
			inst,									\
			nrfx_twim_irq_handler,							\
			&CONCAT(twim_, inst, z_data.twim)					\
		)										\
	}											\
	IF_ENABLED(USES_MSG_BUF(inst), (MSG_BUF_DEFINE(inst);))					\
	I2C_RTIO_DEFINE(_i2c##inst##_twim_rtio,							\
			DT_INST_PROP_OR(n, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),			\
			DT_INST_PROP_OR(n, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));			\
	PINCTRL_DT_INST_DEFINE(inst);								\
	static const struct i2c_nrfx_twim_rtio_config twim_##inst##z_config = {			\
		.common =									\
			{									\
				.twim_config =							\
					{							\
						.skip_gpio_cfg = true,				\
						.skip_psel_cfg = true,				\
						.frequency = I2C_FREQUENCY(inst),		\
					},							\
				.event_handler = event_handler,					\
				.msg_buf_size = MSG_BUF_SIZE(inst),				\
				.pre_init = pre_init##inst,					\
				.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
				IF_ENABLED(USES_MSG_BUF(inst), (.msg_buf = MSG_BUF_SYM(inst),))	\
				.max_transfer_size = MAX_TRANSFER_SIZE(inst),			\
				.twim = &twim_##inst##z_data.twim,				\
			},									\
		.ctx = &_i2c##inst##_twim_rtio,							\
	};											\
	PM_DEVICE_DT_INST_DEFINE(inst, twim_nrfx_pm_action, I2C_PM_ISR_SAFE(inst));		\
	I2C_DEVICE_DT_INST_DEINIT_DEFINE(inst, i2c_nrfx_twim_rtio_init,				\
					 i2c_nrfx_twim_rtio_deinit,				\
					 PM_DEVICE_DT_INST_GET(inst), &twim_##inst##z_data,	\
					 &twim_##inst##z_config, POST_KERNEL,			\
					 CONFIG_I2C_INIT_PRIORITY, &i2c_nrfx_twim_driver_api);	\

DT_INST_FOREACH_STATUS_OKAY(I2C_NRFX_TWIM_RTIO_DEVICE)
