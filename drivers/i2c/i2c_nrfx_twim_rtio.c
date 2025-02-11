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

struct i2c_nrfx_twim_rtio_config {
	struct i2c_nrfx_twim_common_config common;
	struct i2c_rtio *ctx;
};

struct i2c_nrfx_twim_rtio_data {
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

static bool i2c_nrfx_twim_rtio_start(const struct device *dev)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;
	struct i2c_nrfx_twim_rtio_data *data = dev->data;
	struct i2c_rtio *ctx = config->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;

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
		return false;
	case RTIO_OP_I2C_RECOVER:
		(void)i2c_nrfx_twim_recover_bus(dev);
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

static void event_handler(nrfx_twim_evt_t const *p_event, void *p_context)
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

int i2c_nrfx_twim_rtio_init(const struct device *dev)
{
	const struct i2c_nrfx_twim_rtio_config *config = dev->config;

	i2c_rtio_init(config->ctx, dev);
	return i2c_nrfx_twim_common_init(dev);
}

#define CONCAT_BUF_SIZE(idx)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(I2C(idx), zephyr_concat_buf_size),                            \
		    (DT_PROP(I2C(idx), zephyr_concat_buf_size)), (0))
#define FLASH_BUF_MAX_SIZE(idx)                                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(I2C(idx), zephyr_flash_buf_max_size),                         \
		    (DT_PROP(I2C(idx), zephyr_flash_buf_max_size)), (0))

#define USES_MSG_BUF(idx)                                                                          \
	COND_CODE_0(CONCAT_BUF_SIZE(idx), (COND_CODE_0(FLASH_BUF_MAX_SIZE(idx), (0), (1))), (1))
#define MSG_BUF_SIZE(idx) MAX(CONCAT_BUF_SIZE(idx), FLASH_BUF_MAX_SIZE(idx))

#define MSG_BUF_HAS_MEMORY_REGIONS(idx) \
	DT_NODE_HAS_PROP(I2C(idx), memory_regions)

#define MSG_BUF_LINKER_REGION_NAME(idx) \
	LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(I2C(idx), memory_regions))

#define MSG_BUF_ATTR_SECTION(idx) \
	__attribute__((__section__(MSG_BUF_LINKER_REGION_NAME(idx))))

#define MSG_BUF_ATTR(idx)                                                                          \
	COND_CODE_1(                                                                               \
		MSG_BUF_HAS_MEMORY_REGIONS(idx),                                                   \
		(MSG_BUF_ATTR_SECTION(idx)),                                                       \
		()                                                                                 \
	)

#define MSG_BUF_SYM(idx) \
	_CONCAT_3(twim_, idx, _msg_buf)

#define MSG_BUF_DEFINE(idx) \
	static uint8_t MSG_BUF_SYM(idx)[MSG_BUF_SIZE(idx)] MSG_BUF_ATTR(idx)

#define MAX_TRANSFER_SIZE(idx) \
	BIT_MASK(DT_PROP(I2C(idx), easydma_maxcnt_bits))

#define I2C_NRFX_TWIM_RTIO_DEVICE(idx)                                                             \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(I2C(idx));                                             \
	BUILD_ASSERT(I2C_FREQUENCY(idx) != I2C_NRFX_TWIM_INVALID_FREQUENCY,                        \
		     "Wrong I2C " #idx " frequency setting in dts");                               \
	static void irq_connect##idx(void)                                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority), nrfx_isr,               \
			    nrfx_twim_##idx##_irq_handler, 0);                                     \
	}                                                                                          \
	IF_ENABLED(USES_MSG_BUF(idx), (MSG_BUF_DEFINE(idx);))                                      \
	I2C_RTIO_DEFINE(_i2c##idx##_twim_rtio,                                                     \
			DT_INST_PROP_OR(n, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),                      \
			DT_INST_PROP_OR(n, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));                     \
	PINCTRL_DT_DEFINE(I2C(idx));                                                               \
	static struct i2c_nrfx_twim_rtio_data twim_##idx##z_data;                                  \
	static const struct i2c_nrfx_twim_rtio_config twim_##idx##z_config = {                     \
		.common =                                                                          \
			{                                                                          \
				.twim = NRFX_TWIM_INSTANCE(idx),                                   \
				.twim_config =                                                     \
					{                                                          \
						.skip_gpio_cfg = true,                             \
						.skip_psel_cfg = true,                             \
						.frequency = I2C_FREQUENCY(idx),                   \
					},                                                         \
				.event_handler = event_handler,                                    \
				.msg_buf_size = MSG_BUF_SIZE(idx),                                 \
				.irq_connect = irq_connect##idx,                                   \
				.pcfg = PINCTRL_DT_DEV_CONFIG_GET(I2C(idx)),                       \
				IF_ENABLED(USES_MSG_BUF(idx), (.msg_buf = MSG_BUF_SYM(idx),))      \
				.max_transfer_size = MAX_TRANSFER_SIZE(idx),                       \
			},                                                                         \
		.ctx = &_i2c##idx##_twim_rtio,                                                     \
	};                                                                                         \
	PM_DEVICE_DT_DEFINE(I2C(idx), twim_nrfx_pm_action, PM_DEVICE_ISR_SAFE);                    \
	I2C_DEVICE_DT_DEFINE(I2C(idx), i2c_nrfx_twim_rtio_init, PM_DEVICE_DT_GET(I2C(idx)),        \
			     &twim_##idx##z_data, &twim_##idx##z_config, POST_KERNEL,              \
			     CONFIG_I2C_INIT_PRIORITY, &i2c_nrfx_twim_driver_api);

#ifdef CONFIG_HAS_HW_NRF_TWIM0
I2C_NRFX_TWIM_RTIO_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM1
I2C_NRFX_TWIM_RTIO_DEVICE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM2
I2C_NRFX_TWIM_RTIO_DEVICE(2);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM3
I2C_NRFX_TWIM_RTIO_DEVICE(3);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM20
I2C_NRFX_TWIM_RTIO_DEVICE(20);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM21
I2C_NRFX_TWIM_RTIO_DEVICE(21);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM22
I2C_NRFX_TWIM_RTIO_DEVICE(22);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM30
I2C_NRFX_TWIM_RTIO_DEVICE(30);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM120
I2C_NRFX_TWIM_RTIO_DEVICE(120);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM130
I2C_NRFX_TWIM_RTIO_DEVICE(130);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM131
I2C_NRFX_TWIM_RTIO_DEVICE(131);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM132
I2C_NRFX_TWIM_RTIO_DEVICE(132);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM133
I2C_NRFX_TWIM_RTIO_DEVICE(133);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM134
I2C_NRFX_TWIM_RTIO_DEVICE(134);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM135
I2C_NRFX_TWIM_RTIO_DEVICE(135);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM136
I2C_NRFX_TWIM_RTIO_DEVICE(136);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM137
I2C_NRFX_TWIM_RTIO_DEVICE(137);
#endif
