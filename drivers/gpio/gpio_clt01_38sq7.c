/*
 * Copyright (c) 2026 Sebastian Balling
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * GPIO driver for the ST CLT01-38SQ7 octal digital input current limiter in
 * 16-bit SPI mode (SPM = GND), datasheet DS14432.
 *
 * Frame (chip -> MCU, read only), MSB first:
 *   [15:8] IN8..IN1
 *   [7]    /UVA  undervoltage alarm, active low
 *   [6]    /OTA  over-temperature alarm, active low
 *   [5:2]  PC1..PC4 check bits (XNOR of input groups)
 *   [1]    always low
 *   [0]    stop bit, always high (low when the chip has lost its supply)
 *
 * Frames with wrong fixed bits or check bits are rejected with -EIO.
 * The chip has no interrupt output: every port read is one SPI transfer.
 */

#define DT_DRV_COMPAT st_clt01_38sq7

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio/gpio_clt01_38sq7.h>

LOG_MODULE_REGISTER(gpio_clt01_38sq7, CONFIG_GPIO_LOG_LEVEL);

#if CONFIG_SPI_INIT_PRIORITY >= CONFIG_GPIO_CLT01_38SQ7_INIT_PRIORITY
#error "SPI_INIT_PRIORITY must be lower than GPIO_CLT01_38SQ7_INIT_PRIORITY"
#endif

#define CLT01_38SQ7_SPI_OPERATION (SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB | SPI_WORD_SET(16))

enum clt01_38sq7_state {
	CLT01_38SQ7_STATE_UNKNOWN,
	CLT01_38SQ7_STATE_OK,
	CLT01_38SQ7_STATE_ALARM,
	CLT01_38SQ7_STATE_NO_RESPONSE,
	CLT01_38SQ7_STATE_CHECK_ERROR,
};

struct clt01_38sq7_config {
	struct gpio_driver_config common;
	struct spi_dt_spec bus;
};

struct clt01_38sq7_data {
	struct gpio_driver_data common;
	struct k_mutex lock;
	uint8_t status;
	bool status_valid;
	enum clt01_38sq7_state state;
};

/* XNOR over all bits of v (as defined in the datasheet): 1 for an even number of ones. */
static inline uint8_t xnor_parity(uint8_t v)
{
	return (uint8_t)(__builtin_parity(v) ^ 1);
}

/* Expected control bits PC1..PC4 (bits 5..2) for the given inputs (bit n = IN(n+1)). */
static uint8_t clt01_38sq7_check_bits(uint8_t in)
{
	const uint8_t pc1 = xnor_parity(in);         /* IN1..IN8 */
	const uint8_t pc2 = xnor_parity(in & 0xF0U); /* IN5..IN8 */
	const uint8_t pc3 = xnor_parity(in & 0x0FU); /* IN1..IN4 */
	const uint8_t pc4 = xnor_parity(in & 0x3CU); /* IN3..IN6 */

	return (uint8_t)((pc1 << 5) | (pc2 << 4) | (pc3 << 3) | (pc4 << 2));
}

static void clt01_38sq7_set_state(const struct device *dev, enum clt01_38sq7_state state,
				  uint8_t status)
{
	struct clt01_38sq7_data *data = dev->data;

	if (state == data->state) {
		return;
	}

	switch (state) {
	case CLT01_38SQ7_STATE_OK:
		LOG_INF("%s: ok", dev->name);
		break;
	case CLT01_38SQ7_STATE_ALARM:
		LOG_WRN("%s: alarm%s%s", dev->name,
			(status & CLT01_38SQ7_STATUS_NUVA) ? "" : " undervoltage",
			(status & CLT01_38SQ7_STATUS_NOTA) ? "" : " over-temperature");
		break;
	case CLT01_38SQ7_STATE_NO_RESPONSE:
		LOG_ERR("%s: invalid frame, status 0x%02x (no supply?)", dev->name, status);
		break;
	case CLT01_38SQ7_STATE_CHECK_ERROR:
		LOG_ERR("%s: check bit error, status 0x%02x", dev->name, status);
		break;
	default:
		break;
	}

	data->state = state;
}

static int clt01_38sq7_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);

	return ((flags & GPIO_OUTPUT) != 0U) ? -ENOTSUP : 0;
}

static int clt01_38sq7_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct clt01_38sq7_config *cfg = dev->config;
	struct clt01_38sq7_data *data = dev->data;
	uint16_t rx = 0;

	const struct spi_buf rx_buf = {.buf = &rx, .len = sizeof(rx)};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret = spi_read_dt(&cfg->bus, &rx_set);

	if (ret < 0) {
		LOG_ERR("%s: SPI read failed: %d", dev->name, ret);
		goto out;
	}

	const uint8_t inputs = (uint8_t)(rx >> 8);
	const uint8_t status = (uint8_t)(rx & 0xFFU);
	const uint8_t fixed_mask = CLT01_38SQ7_STATUS_STOP | CLT01_38SQ7_STATUS_LOW;
	const uint8_t check_mask = CLT01_38SQ7_STATUS_PC1 | CLT01_38SQ7_STATUS_PC2 |
				   CLT01_38SQ7_STATUS_PC3 | CLT01_38SQ7_STATUS_PC4;

	if ((status & fixed_mask) != CLT01_38SQ7_STATUS_STOP) {
		clt01_38sq7_set_state(dev, CLT01_38SQ7_STATE_NO_RESPONSE, status);
		ret = -EIO;
		goto out;
	}

	if ((status & check_mask) != clt01_38sq7_check_bits(inputs)) {
		clt01_38sq7_set_state(dev, CLT01_38SQ7_STATE_CHECK_ERROR, status);
		ret = -EIO;
		goto out;
	}

	data->status = status;
	data->status_valid = true;
	clt01_38sq7_set_state(
		dev, clt01_38sq7_status_ok(status) ? CLT01_38SQ7_STATE_OK : CLT01_38SQ7_STATE_ALARM,
		status);

	*value = inputs;

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int clt01_38sq7_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mask);
	ARG_UNUSED(value);

	return -ENOTSUP;
}

static int clt01_38sq7_port_pins_unsupported(const struct device *dev, gpio_port_pins_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	return -ENOTSUP;
}

static int clt01_38sq7_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(trig);

	return (mode == GPIO_INT_MODE_DISABLED) ? 0 : -ENOTSUP;
}

int clt01_38sq7_get_status(const struct device *dev, uint8_t *status)
{
	struct clt01_38sq7_data *data = dev->data;
	int ret = 0;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->status_valid) {
		*status = data->status;
	} else {
		ret = -ENODATA;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(gpio, clt01_38sq7_api) = {
	.pin_configure = clt01_38sq7_pin_configure,
	.port_get_raw = clt01_38sq7_port_get_raw,
	.port_set_masked_raw = clt01_38sq7_port_set_masked_raw,
	.port_set_bits_raw = clt01_38sq7_port_pins_unsupported,
	.port_clear_bits_raw = clt01_38sq7_port_pins_unsupported,
	.port_toggle_bits = clt01_38sq7_port_pins_unsupported,
	.pin_interrupt_configure = clt01_38sq7_pin_interrupt_configure,
};

static int clt01_38sq7_init(const struct device *dev)
{
	const struct clt01_38sq7_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

#define CLT01_38SQ7_DEFINE(inst)                                                                   \
	static struct clt01_38sq7_data clt01_38sq7_data_##inst = {                                 \
		.lock = Z_MUTEX_INITIALIZER(clt01_38sq7_data_##inst.lock),                         \
		.state = CLT01_38SQ7_STATE_UNKNOWN,                                                \
	};                                                                                         \
                                                                                                   \
	static const struct clt01_38sq7_config clt01_38sq7_config_##inst = {                       \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.bus = SPI_DT_SPEC_INST_GET(inst, CLT01_38SQ7_SPI_OPERATION),                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, clt01_38sq7_init, NULL, &clt01_38sq7_data_##inst,              \
			      &clt01_38sq7_config_##inst, POST_KERNEL,                             \
			      CONFIG_GPIO_CLT01_38SQ7_INIT_PRIORITY, &clt01_38sq7_api);

DT_INST_FOREACH_STATUS_OKAY(CLT01_38SQ7_DEFINE)
