/*
 * Copyright (c) 2018-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <nrfx_twim.h>
#include <zephyr/sys/util.h>
#include <zephyr/linker/devicetree_regions.h>
#include "i2c_context.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_nrfx_twim, CONFIG_I2C_LOG_LEVEL);

struct i2c_nrfx_twim_data {
	struct i2c_context ctx;

	/* The number of bytes to receive into primary_buf for RX transfer.
	 * The number of bytes to transmit from primary_buf for TX transfer.
	 */
	uint16_t primary_buf_length;

	/* Received data is copied from primary_buf to these messages post
	 * RX transfer.
	 */
	const struct i2c_msg *rx_msgs;
};

struct i2c_nrfx_twim_config {
	nrfx_twim_t twim;
	nrfx_twim_config_t twim_config;
	uint8_t *primary_buf;
	uint16_t primary_buf_size;
	void (*irq_connect)(void);
	const struct pinctrl_dev_config *pcfg;
	uint16_t max_transfer_size;
};

static void event_handler(nrfx_twim_evt_t const *p_event, void *p_context)
{
	struct i2c_nrfx_twim_data *data = p_context;
	struct i2c_context *ctx = &data->ctx;

	if (p_event->type == NRFX_TWIM_EVT_DONE) {
		i2c_context_continue_transfer(ctx);
	} else {
		i2c_context_cancel_transfer(ctx);
	}
}

static int i2c_nrfx_twim_configure(const struct device *dev,
				   uint32_t i2c_config)
{
	const struct i2c_nrfx_twim_config *dev_config = dev->config;

	if (I2C_ADDR_10_BITS & i2c_config) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(i2c_config)) {
	case I2C_SPEED_STANDARD:
		nrf_twim_frequency_set(dev_config->twim.p_twim,
				       NRF_TWIM_FREQ_100K);
		break;
	case I2C_SPEED_FAST:
		nrf_twim_frequency_set(dev_config->twim.p_twim,
				       NRF_TWIM_FREQ_400K);
		break;
#if NRF_TWIM_HAS_1000_KHZ_FREQ
	case I2C_SPEED_FAST_PLUS:
		nrf_twim_frequency_set(dev_config->twim.p_twim,
				       NRF_TWIM_FREQ_1000K);
		break;
#endif
	default:
		LOG_ERR("unsupported speed");
		return -EINVAL;
	}

	return 0;
}

static int i2c_nrfx_twim_recover_bus(const struct device *dev)
{
	const struct i2c_nrfx_twim_config *dev_config = dev->config;
	enum pm_device_state state;
	uint32_t scl_pin;
	uint32_t sda_pin;
	nrfx_err_t err;

	scl_pin = nrf_twim_scl_pin_get(dev_config->twim.p_twim);
	sda_pin = nrf_twim_sda_pin_get(dev_config->twim.p_twim);

	/* disable peripheral if active (required to release SCL/SDA lines) */
	(void)pm_device_state_get(dev, &state);
	if (state == PM_DEVICE_STATE_ACTIVE) {
		nrfx_twim_disable(&dev_config->twim);
	}

	err = nrfx_twim_bus_recover(scl_pin, sda_pin);

	/* restore peripheral if it was active before */
	if (state == PM_DEVICE_STATE_ACTIVE) {
		(void)pinctrl_apply_state(dev_config->pcfg,
					  PINCTRL_STATE_DEFAULT);
		nrfx_twim_enable(&dev_config->twim);
	}

	return (err == NRFX_SUCCESS ? 0 : -EBUSY);
}

static int i2c_nrfx_twim_transfer(const struct device *dev,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs,
				  uint16_t addr)
{
	struct i2c_nrfx_twim_data *data = dev->data;

	return i2c_context_start_transfer(&data->ctx,
					  msgs,
					  num_msgs,
					  addr);
}

#if defined(CONFIG_I2C_CALLBACK)
static int i2c_nrfx_twim_transfer_cb(const struct device *dev,
				     struct i2c_msg *msgs,
				     uint8_t num_msgs,
				     uint16_t addr,
				     i2c_callback_t cb,
				     void *userdata)
{
	struct i2c_nrfx_twim_data *data = dev->data;

	return i2c_context_start_transfer_cb(&data->ctx,
					     msgs,
					     num_msgs,
					     addr,
					     cb,
					     userdata);
}
#endif

static const struct i2c_driver_api i2c_nrfx_twim_driver_api = {
	.configure   = i2c_nrfx_twim_configure,
	.transfer    = i2c_nrfx_twim_transfer,
	.recover_bus = i2c_nrfx_twim_recover_bus,
#if defined(CONFIG_I2C_CALLBACK)
	.transfer_cb = i2c_nrfx_twim_transfer_cb,
#endif
};

#ifdef CONFIG_PM_DEVICE
static int twim_nrfx_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	const struct i2c_nrfx_twim_config *dev_config = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(dev_config->pcfg,
					  PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
		nrfx_twim_enable(&dev_config->twim);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		nrfx_twim_disable(&dev_config->twim);

		ret = pinctrl_apply_state(dev_config->pcfg,
					  PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int twim_init_transfer_handler(struct i2c_context *ctx)
{
	const struct device *dev = i2c_context_get_dev(ctx);
	struct i2c_nrfx_twim_data *data = dev->data;
	const struct i2c_msg *msgs = i2c_context_get_transfer_msgs(ctx);
	uint8_t num_msgs = i2c_context_get_transfer_num_msgs(ctx);

	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			LOG_ERR("10-bit address unsupported");
			return -EIO;
		}
	}

	data->primary_buf_length = 0;
	data->rx_msgs = NULL;

	(void)pm_device_runtime_get(dev);
	return 0;
}

/* Concurrent RX messages are concatenated and received into
 * the primary_buf. The received data is copied back into
 * the RX messages post transfer.
 */
static int twim_prepare_rx_xfer(const struct device *dev, nrfx_twim_xfer_desc_t *xfer)
{
	struct i2c_nrfx_twim_data *data = dev->data;
	const struct i2c_nrfx_twim_config *config = dev->config;
	struct i2c_context *ctx = &data->ctx;
	struct i2c_msg *msgs = i2c_context_get_transfer_msgs(ctx);
	uint8_t msg_idx = i2c_context_get_transfer_msg_idx(ctx);
	uint8_t num_msgs = i2c_context_get_transfer_num_msgs(ctx);
	struct i2c_msg msg;

	xfer->type = NRFX_TWIM_XFER_RX;

	/* Cache properties used to copy received data from the primary_buf to
	 * the rx_msgs post transfer.
	 */
	data->primary_buf_length = 0;
	data->rx_msgs = msgs + msg_idx;

	for (msg_idx = msg_idx; msg_idx < num_msgs; msg_idx++) {
		msg = msgs[msg_idx];

		if (msg.len > (config->primary_buf_size - data->primary_buf_length)) {
			LOG_ERR("concat buffer overrun");
			return -EIO;
		}

		data->primary_buf_length += msg.len;

		if (msg.flags & (I2C_MSG_STOP)) {
			/* Stop sent after this message, ending this transfer */
			break;
		}

		if (msg_idx == (num_msgs - 1)) {
			LOG_ERR("last i2c_msg malformed");
			return -EIO;
		}

		msg = msgs[msg_idx + 1];

		if (msg.flags & I2C_MSG_RESTART) {
			/* Next message starts with a restart, ending this transfer */
			break;
		}

		if (!(msg.flags & I2C_MSG_READ)) {
			/* Next message changes direction to TX, ending this transfer */
			break;
		}
	}

	xfer->primary_length = data->primary_buf_length;

	/* Update transfer iterator to account for concatenated messages */
	i2c_context_set_transfer_msg_idx(ctx, msg_idx);
	return 0;
}

/* Concurrent TX messages are concatenated and copied to the
 * primary_buf before the transfer is started.
 */
static int twim_prepare_tx_xfer(const struct device *dev, nrfx_twim_xfer_desc_t *xfer)
{
	struct i2c_nrfx_twim_data *data = dev->data;
	const struct i2c_nrfx_twim_config *config = dev->config;
	struct i2c_context *ctx = &data->ctx;
	struct i2c_msg *msgs = i2c_context_get_transfer_msgs(ctx);
	uint8_t msg_idx = i2c_context_get_transfer_msg_idx(ctx);
	uint8_t num_msgs = i2c_context_get_transfer_num_msgs(ctx);
	struct i2c_msg msg;

	xfer->type = NRFX_TWIM_XFER_TX;

	data->primary_buf_length = 0;

	for (msg_idx = msg_idx; msg_idx < num_msgs; msg_idx++) {
		msg = msgs[msg_idx];

		if (msg.len > (config->primary_buf_size - data->primary_buf_length)) {
			LOG_ERR("concat buffer overrun");
			return -EIO;
		}

		data->primary_buf_length += msg.len;

		if (msg.flags & (I2C_MSG_STOP)) {
			/* Stop sent after this message, ending this transfer */
			break;
		}

		if (msg_idx == (num_msgs - 1)) {
			LOG_ERR("last i2c_msg malformed");
			return -EIO;
		}

		msg = msgs[msg_idx + 1];

		if (msg.flags & I2C_MSG_RESTART) {
			/* Next message starts with a restart, ending this transfer */
			break;
		}

		if (msg.flags & I2C_MSG_READ) {
			/* Next message changes direction to RX, ending this transfer */
			break;
		}
	}

	xfer->primary_length = data->primary_buf_length;

	/* Update transfer iterator to account for concatenated messages */
	i2c_context_set_transfer_msg_idx(ctx, msg_idx);
	return 0;
}

static void twim_start_transfer_handler(struct i2c_context *ctx)
{
	const struct device *dev = i2c_context_get_dev(ctx);
	struct i2c_nrfx_twim_data *data = dev->data;
	const struct i2c_nrfx_twim_config *config = dev->config;
	struct i2c_msg *msgs = i2c_context_get_transfer_msgs(ctx);
	uint8_t msg_idx = i2c_context_get_transfer_msg_idx(ctx);
	struct i2c_msg msg = msgs[msg_idx];

	int ret;
	uint32_t xfer_flags;

	nrfx_twim_xfer_desc_t xfer = {
		.address = i2c_context_get_transfer_addr(ctx),
		.p_primary_buf = config->primary_buf,
	};

	if (msg.flags & I2C_MSG_READ) {
		ret = twim_prepare_rx_xfer(dev, &xfer);
	} else {
		ret = twim_prepare_tx_xfer(dev, &xfer);
	}

	if (ret) {
		LOG_ERR("failed to prepare transfer");
		i2c_context_cancel_transfer(ctx);
		return;
	}

	if (data->primary_buf_length > config->max_transfer_size) {
		LOG_ERR("concat buffer exceeds dma capacity");
		i2c_context_cancel_transfer(ctx);
		return;
	}

	msg_idx = i2c_context_get_transfer_msg_idx(ctx);
	msg = msgs[msg_idx];
	xfer_flags = (msg.flags & I2C_MSG_STOP) ? 0 : NRFX_TWIM_FLAG_TX_NO_STOP;

	if (nrfx_twim_xfer(&config->twim, &xfer, xfer_flags) == NRFX_SUCCESS) {
		return;
	}

	LOG_ERR("failed to start transfer");
	i2c_context_cancel_transfer(ctx);
}

static void twim_copy_primary_buf_to_rx_msgs(const struct device *dev)
{
	struct i2c_nrfx_twim_data *data = dev->data;
	const struct i2c_nrfx_twim_config *config = dev->config;
	const uint8_t *primary_buf_ptr = config->primary_buf;
	uint8_t *rx_msg_buf_ptr;
	uint16_t rx_msg_len;

	while (data->primary_buf_length) {
		rx_msg_buf_ptr = (*data->rx_msgs).buf;
		rx_msg_len = (*data->rx_msgs).len;
		memcpy(rx_msg_buf_ptr, primary_buf_ptr, rx_msg_len);
		primary_buf_ptr += rx_msg_len;
		data->primary_buf_length -= rx_msg_len;
		data->rx_msgs++;
	}

	data->rx_msgs = NULL;
}

static void twim_post_transfer_handler(struct i2c_context *ctx)
{
	const struct device *dev = i2c_context_get_dev(ctx);
	struct i2c_nrfx_twim_data *data = dev->data;

	if (data->rx_msgs) {
		twim_copy_primary_buf_to_rx_msgs(dev);
	}
}

static void twim_deinit_transfer_handler(struct i2c_context *ctx)
{
	const struct device *dev = i2c_context_get_dev(ctx);
	int result = i2c_context_get_transfer_result(ctx);

	if (result) {
		/* Try to recover bus in case of error */
		(void)i2c_nrfx_twim_recover_bus(dev);
	}

	(void)pm_device_runtime_put(dev);
}

static int i2c_nrfx_twim_init(const struct device *dev)
{
	const struct i2c_nrfx_twim_config *dev_config = dev->config;
	struct i2c_nrfx_twim_data *dev_data = dev->data;

	i2c_context_init(&dev_data->ctx,
			 dev,
			 twim_init_transfer_handler,
			 twim_start_transfer_handler,
			 twim_post_transfer_handler,
			 twim_deinit_transfer_handler);

	dev_config->irq_connect();

	int err = pinctrl_apply_state(dev_config->pcfg,
				      COND_CODE_1(CONFIG_PM_DEVICE_RUNTIME,
						  (PINCTRL_STATE_SLEEP),
						  (PINCTRL_STATE_DEFAULT)));
	if (err < 0) {
		return err;
	}

	if (nrfx_twim_init(&dev_config->twim, &dev_config->twim_config,
			   event_handler, dev_data) != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->name);
		return -EIO;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_init_suspended(dev);
	pm_device_runtime_enable(dev);
#else
	nrfx_twim_enable(&dev_config->twim);
#endif

	return 0;
}

#define I2C_NRFX_TWIM_INVALID_FREQUENCY  ((nrf_twim_frequency_t)-1)
#define I2C_NRFX_TWIM_FREQUENCY(bitrate)				       \
	(bitrate == I2C_BITRATE_STANDARD  ? NRF_TWIM_FREQ_100K :	       \
	 bitrate == 250000                ? NRF_TWIM_FREQ_250K :	       \
	 bitrate == I2C_BITRATE_FAST      ? NRF_TWIM_FREQ_400K :	       \
	IF_ENABLED(NRF_TWIM_HAS_1000_KHZ_FREQ,				       \
	(bitrate == I2C_BITRATE_FAST_PLUS ? NRF_TWIM_FREQ_1000K :))	       \
					    I2C_NRFX_TWIM_INVALID_FREQUENCY)

#define I2C(idx) DT_NODELABEL(i2c##idx)
#define I2C_HAS_PROP(idx, prop)	DT_NODE_HAS_PROP(I2C(idx), prop)
#define I2C_FREQUENCY(idx)						       \
	I2C_NRFX_TWIM_FREQUENCY(DT_PROP(I2C(idx), clock_frequency))

#define CONCAT_BUF_SIZE(idx)						       \
	COND_CODE_1(DT_NODE_HAS_PROP(I2C(idx), zephyr_concat_buf_size),	       \
		    (DT_PROP(I2C(idx), zephyr_concat_buf_size)), (0))
#define FLASH_BUF_MAX_SIZE(idx)						       \
	COND_CODE_1(DT_NODE_HAS_PROP(I2C(idx), zephyr_flash_buf_max_size),     \
		    (DT_PROP(I2C(idx), zephyr_flash_buf_max_size)), (0))

#define USES_MSG_BUF(idx)						       \
	COND_CODE_0(CONCAT_BUF_SIZE(idx),				       \
		(COND_CODE_0(FLASH_BUF_MAX_SIZE(idx), (0), (1))),	       \
		(1))
#define MSG_BUF_SIZE(idx)  MAX(CONCAT_BUF_SIZE(idx), FLASH_BUF_MAX_SIZE(idx))

#define I2C_NRFX_TWIM_DEVICE(idx)					       \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(I2C(idx));			       \
	BUILD_ASSERT(I2C_FREQUENCY(idx) !=				       \
		     I2C_NRFX_TWIM_INVALID_FREQUENCY,			       \
		     "Wrong I2C " #idx " frequency setting in dts");	       \
	static void irq_connect##idx(void)				       \
	{								       \
		IRQ_CONNECT(DT_IRQN(I2C(idx)), DT_IRQ(I2C(idx), priority),     \
			    nrfx_isr, nrfx_twim_##idx##_irq_handler, 0);       \
	}								       \
	IF_ENABLED(USES_MSG_BUF(idx),					       \
		(static uint8_t twim_##idx##_msg_buf[MSG_BUF_SIZE(idx)]	       \
		 I2C_MEMORY_SECTION(idx);))				       \
	static struct i2c_nrfx_twim_data twim_##idx##_data;		       \
	PINCTRL_DT_DEFINE(I2C(idx));					       \
	static const struct i2c_nrfx_twim_config twim_##idx##z_config = {      \
		.twim = NRFX_TWIM_INSTANCE(idx),			       \
		.twim_config = {					       \
			.skip_gpio_cfg = true,				       \
			.skip_psel_cfg = true,				       \
			.frequency = I2C_FREQUENCY(idx),		       \
		},							       \
		IF_ENABLED(USES_MSG_BUF(idx),				       \
			(.primary_buf = twim_##idx##_msg_buf,))		       \
		.primary_buf_size = MSG_BUF_SIZE(idx),			       \
		.irq_connect = irq_connect##idx,			       \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(I2C(idx)),		       \
		.max_transfer_size = BIT_MASK(				       \
				DT_PROP(I2C(idx), easydma_maxcnt_bits)),       \
	};								       \
	PM_DEVICE_DT_DEFINE(I2C(idx), twim_nrfx_pm_action);		       \
	I2C_DEVICE_DT_DEFINE(I2C(idx),					       \
		      i2c_nrfx_twim_init,				       \
		      PM_DEVICE_DT_GET(I2C(idx)),			       \
		      &twim_##idx##_data,				       \
		      &twim_##idx##z_config,				       \
		      POST_KERNEL,					       \
		      CONFIG_I2C_INIT_PRIORITY,				       \
		      &i2c_nrfx_twim_driver_api)

#define I2C_MEMORY_SECTION(idx)						       \
	COND_CODE_1(I2C_HAS_PROP(idx, memory_regions),			       \
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	       \
			DT_PHANDLE(I2C(idx), memory_regions)))))),	       \
		())

#ifdef CONFIG_HAS_HW_NRF_TWIM0
I2C_NRFX_TWIM_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM1
I2C_NRFX_TWIM_DEVICE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM2
I2C_NRFX_TWIM_DEVICE(2);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM3
I2C_NRFX_TWIM_DEVICE(3);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM20
I2C_NRFX_TWIM_DEVICE(20);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM21
I2C_NRFX_TWIM_DEVICE(21);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM22
I2C_NRFX_TWIM_DEVICE(22);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM30
I2C_NRFX_TWIM_DEVICE(30);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM120
I2C_NRFX_TWIM_DEVICE(120);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM130
I2C_NRFX_TWIM_DEVICE(130);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM131
I2C_NRFX_TWIM_DEVICE(131);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM132
I2C_NRFX_TWIM_DEVICE(132);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM133
I2C_NRFX_TWIM_DEVICE(133);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM134
I2C_NRFX_TWIM_DEVICE(134);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM135
I2C_NRFX_TWIM_DEVICE(135);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM136
I2C_NRFX_TWIM_DEVICE(136);
#endif

#ifdef CONFIG_HAS_HW_NRF_TWIM137
I2C_NRFX_TWIM_DEVICE(137);
#endif
