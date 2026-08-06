/*
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT issi_is31fl3743b

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/led/is31fl3743b.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(is31fl3743b, CONFIG_LED_LOG_LEVEL);

/**
 * @file
 * @brief IS31FL3743B LED Matrix driver
 *
 * General note: This driver is based on the following datasheet:
 * - Title: IS31FL3743B, Rev. C, 10/25/2024
 * - Link: https://www.lumissil.com/assets/pdf/core/IS31FL3743B_DS.pdf
 *
 * If any comments in this driver refer to a "datasheet" for reference, the document linked above
 * is what they're talking about.
 */

/* is31fl3743b SPI command byte layout (MSB first):
 *
 *  RW[7:7]   (1 bit)  (note: this bit is always 0 since this driver only sends writes)
 *  ID[6:4]   (3 bits) (note: this is always 0b101)
 *	PAGE[3:0] (4 bits)
 *
 * See Table 1 on page 8 of the datasheet.
 */
#define CMD_ID               0x50
#define CMD_BYTE_WRITE(page) (CMD_ID | (page))

/* IS31FL3743B register pages (see "D3:D0" in Table 1 on page 8 of the datasheet) */
#define PAGE_PWM     0x0 /* PWM duty cycle registers */
#define PAGE_SCALING 0x1 /* Peak current scaling registers */
#define PAGE_FUNC    0x2 /* Function/configuration registers */

/*	Per-LED register base address. Used for both PG0 and PG1.
 *	Ranges from "01h~C6h". See Table 2 on page 11 of the datasheet.
 */
#define LED_REG_BASE 0x01

/* Function Registers (for PG2). See Table 5 on page 14 of the datasheet. */
#define CONF_REG                0x00 /* Configuration Register */
#define GLOBAL_CURRENT_CTRL_REG 0x01 /* Global Current Control Register */
#define SPREAD_SPECTRUM_REG     0x25 /* Spread Spectrum and Sync Register */
#define RESET_REG               0x2F /* Reset Register */

/* Configuration Register fields. See Table 6 on page 14 of the datasheet. */
#define CONF_REG_SSD_MASK 0x01 /* Software shutdown (1 = normal operation) */
#define CONF_REG_FIXED    0x08 /* Reserved D3 bit. must be set to 1 */

/* Spread Spectrum Register fields. See Table 11 on page 16 of the datasheet. */
#define SS_REG_SYNC_SHIFT 6 /* Sync configuration field shift */

/*	Value written to the reset register to restore power-on defaults.
 *	See the bit of text under "2Fh Reset Register" on page 16 of the datasheet.
 */
#define RESET_COMMAND 0xAE

/*	Sync mode raw values for the spread spectrum register sync field.
 *	See the "SYNC" header on page 16 of the datasheet.
 */
#define SYNC_DISABLED 0x0 /* Disable SYNC function, internal 30k pull-low */
#define SYNC_RECEIVER 0x2 /* Receiver, clock input */
#define SYNC_SOURCE   0x3 /* Source, clock input */

/* Sync mode enumeration indices. these are meant to match the devicetree sync-mode property */
#define SYNC_MODE_NONE     0
#define SYNC_MODE_SOURCE   1
#define SYNC_MODE_RECEIVER 2

/*	Matrix is laid out with 11 columns
 *	(the switch columns/SW1-SW11) and 18 rows (the current sinks/CS1-CS18).
 *	See Figure 12 on page 15 of the datasheet.
 */
#define IS31FL3743B_SW_COUNT 11
#define IS31FL3743B_CS_COUNT 18
#define IS31FL3743B_MAX_LED  (IS31FL3743B_SW_COUNT * IS31FL3743B_CS_COUNT)

/* Every SPI write is prefixed by a command byte and a register address */
#define IS31FL3743B_HEADER_LEN 2

#define IS31FL3743B_SPI_OPERATION (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

struct is31fl3743b_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec sdb;
	uint8_t current_limit;
	uint8_t sync_mode;
};

struct is31fl3743b_data {
	/* Cached configuration register state. the register is write only */
	uint8_t conf_reg;
	/* Scratch buffer used for bulk controller writes */
	uint8_t scratch_buf[IS31FL3743B_MAX_LED + IS31FL3743B_HEADER_LEN];
};

/* Writes a single register on the given page. */
static int is31fl3743b_write_reg(const struct device *dev, uint8_t page, uint8_t addr,
				 uint8_t value)
{
	const struct is31fl3743b_config *config = dev->config;
	uint8_t buf[3] = {CMD_BYTE_WRITE(page), addr, value};
	const struct spi_buf tx_buf = {.buf = buf, .len = sizeof(buf)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&config->bus, &tx);
}

/* Sends the first len bytes of the scratch buffer as a single SPI transaction. */
static int is31fl3743b_bulk_write(const struct device *dev, size_t len)
{
	const struct is31fl3743b_config *config = dev->config;
	struct is31fl3743b_data *data = dev->data;
	const struct spi_buf tx_buf = {.buf = data->scratch_buf, .len = len};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&config->bus, &tx);
}

static int is31fl3743b_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	uint8_t pwm = (uint8_t)(((uint32_t)value * 255) / LED_BRIGHTNESS_MAX);

	if (led >= IS31FL3743B_MAX_LED) {
		return -EINVAL;
	}

	return is31fl3743b_write_reg(dev, PAGE_PWM, LED_REG_BASE + led, pwm);
}

static int is31fl3743b_led_write_channels(const struct device *dev, uint32_t start_channel,
					  uint32_t num_channels, const uint8_t *buf)
{
	struct is31fl3743b_data *data = dev->data;

	if ((uint64_t)start_channel + num_channels > IS31FL3743B_MAX_LED) {
		return -EINVAL;
	}

	/* Command byte and start address followed by the PWM values. The device
	 * auto-increments the register address for each subsequent byte.
	 */
	data->scratch_buf[0] = CMD_BYTE_WRITE(PAGE_PWM);
	data->scratch_buf[1] = LED_REG_BASE + start_channel;
	memcpy(&data->scratch_buf[IS31FL3743B_HEADER_LEN], buf, num_channels);

	LOG_HEXDUMP_DBG(&data->scratch_buf[IS31FL3743B_HEADER_LEN], num_channels, "PWM states");

	return is31fl3743b_bulk_write(dev, num_channels + IS31FL3743B_HEADER_LEN);
}

static int is31fl3743b_init(const struct device *dev)
{
	const struct is31fl3743b_config *config = dev->config;
	struct is31fl3743b_data *data = dev->data;
	static const uint8_t sync_values[] = {SYNC_DISABLED, SYNC_SOURCE, SYNC_RECEIVER};
	int ret;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	if (config->sdb.port != NULL) {
		if (!gpio_is_ready_dt(&config->sdb)) {
			LOG_ERR("GPIO SDB pin not ready");
			return -ENODEV;
		}
		/* Drive SDB high to exit hardware shutdown */
		ret = gpio_pin_configure_dt(&config->sdb, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	/* Reset all registers to their power-on state, in case we are booting
	 * from a warm reset.
	 */
	ret = is31fl3743b_write_reg(dev, PAGE_FUNC, RESET_REG, RESET_COMMAND);
	if (ret < 0) {
		return ret;
	}

	/* Configure the SYNC clock role when the device is part of a synchronized group */
	if (config->sync_mode != SYNC_MODE_NONE) {
		ret = is31fl3743b_write_reg(dev, PAGE_FUNC, SPREAD_SPECTRUM_REG,
					    sync_values[config->sync_mode] << SS_REG_SYNC_SHIFT);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set global current control register based off devicetree value */
	ret = is31fl3743b_write_reg(dev, PAGE_FUNC, GLOBAL_CURRENT_CTRL_REG, config->current_limit);
	if (ret < 0) {
		return ret;
	}

	/* Scale every LED to full peak current so that only the PWM duty cycle
	 * controls per-LED brightness in this driver.
	 */
	data->scratch_buf[0] = CMD_BYTE_WRITE(PAGE_SCALING);
	data->scratch_buf[1] = LED_REG_BASE;
	memset(&data->scratch_buf[IS31FL3743B_HEADER_LEN], 0xFF, IS31FL3743B_MAX_LED);
	ret = is31fl3743b_bulk_write(dev, IS31FL3743B_MAX_LED + IS31FL3743B_HEADER_LEN);
	if (ret < 0) {
		return ret;
	}

	/* As a final step, exit software shutdown, enabling display output */
	data->conf_reg = CONF_REG_FIXED | CONF_REG_SSD_MASK;
	return is31fl3743b_write_reg(dev, PAGE_FUNC, CONF_REG, data->conf_reg);
}

/* Custom IS31FL3743B specific APIs */

/**
 * @brief Blanks IS31FL3743B LED display.
 *
 * When blank_en is set, the LED display will be disabled. This can be used for
 * flicker-free display updates or power saving.
 *
 * @param dev LED device structure
 * @param blank_en should blanking be enabled
 * @return 0 on success or negative value on error.
 */
int is31fl3743b_blank(const struct device *dev, bool blank_en)
{
	struct is31fl3743b_data *data = dev->data;
	uint8_t conf_reg = data->conf_reg;
	int ret = 0;

	if (blank_en) {
		conf_reg &= ~CONF_REG_SSD_MASK;
	} else {
		conf_reg |= CONF_REG_SSD_MASK;
	}

	ret = is31fl3743b_write_reg(dev, PAGE_FUNC, CONF_REG, conf_reg);
	if (ret < 0) {
		return ret;
	}

	/* if we're here the write succeeded */
	data->conf_reg = conf_reg;
	return ret;
}

/**
 * @brief Sets LED current limit.
 *
 * Sets the global current limit for the LED driver. This is a separate value
 * from per-LED brightness and applies to all LEDs. See the Global Current Control
 * Register docs in and below Table 7 on page 14 of the datasheet.
 *
 * This value sets the output current limit according to
 * the following formula: (343/R_ISET) * (limit/256).
 * This formula corresponds to Formula (3) on page 14 of the datasheet.
 *
 * @param dev LED device structure
 * @param limit current limit to apply
 * @return 0 on success, or negative value on error.
 */
int is31fl3743b_current_limit(const struct device *dev, uint8_t limit)
{
	return is31fl3743b_write_reg(dev, PAGE_FUNC, GLOBAL_CURRENT_CTRL_REG, limit);
}

static DEVICE_API(led, is31fl3743b_api) = {
	.set_brightness = is31fl3743b_led_set_brightness,
	.write_channels = is31fl3743b_led_write_channels,
};

#define IS31FL3743B_DEVICE(n)                                                                      \
	static const struct is31fl3743b_config is31fl3743b_config_##n = {                          \
		.bus = SPI_DT_SPEC_INST_GET(n, IS31FL3743B_SPI_OPERATION),                         \
		.sdb = GPIO_DT_SPEC_INST_GET_OR(n, sdb_gpios, {}),                                 \
		.current_limit = DT_INST_PROP(n, current_limit),                                   \
		.sync_mode = DT_INST_ENUM_IDX(n, sync_mode),                                       \
	};                                                                                         \
                                                                                                   \
	static struct is31fl3743b_data is31fl3743b_data_##n;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &is31fl3743b_init, NULL, &is31fl3743b_data_##n,                   \
			      &is31fl3743b_config_##n, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,      \
			      &is31fl3743b_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3743B_DEVICE)
