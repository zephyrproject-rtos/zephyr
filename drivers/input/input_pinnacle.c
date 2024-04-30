/*
 * Copyright (c) 2024 Ilia Kharin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirque_pinnacle

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pinnacle, CONFIG_INPUT_LOG_LEVEL);

/*
 * Register Access Protocol Standard Registers.
 * Standard registers have 5-bit addresses, BIT[4:0], that range from
 * 0x00 to 0x1F. For reading, a register address has to be combined with
 * 0xA0 for reading and 0x80 for writing bits, BIT[7:5].
 */
#define PINNACLE_REG_FIRMWARE_ID      0x00 /* R */
#define PINNACLE_REG_FIRMWARE_VERSION 0x01 /* R */
#define PINNACLE_REG_STATUS1          0x02 /* R/W */
#define PINNACLE_REG_SYS_CONFIG1      0x03 /* R/W */
#define PINNACLE_REG_FEED_CONFIG1     0x04 /* R/W */
#define PINNACLE_REG_FEED_CONFIG2     0x05 /* R/W */
#define PINNACLE_REG_FEED_CONFIG3     0x06 /* R/W */
#define PINNACLE_REG_CAL_CONFIG1      0x07 /* R/W */
#define PINNACLE_REG_PS2_AUX_CONTROL  0x08 /* R/W */
#define PINNACLE_REG_SAMPLE_RATE      0x09 /* R/W */
#define PINNACLE_REG_Z_IDLE           0x0A /* R/W */
#define PINNACLE_REG_Z_SCALER         0x0B /* R/W */
#define PINNACLE_REG_SLEEP_INTERVAL   0x0C /* R/W */
#define PINNACLE_REG_SLEEP_TIMER      0x0D /* R/W */
#define PINNACLE_REG_EMI_THRESHOLD    0x0E /* R/W */
#define PINNACLE_REG_PACKET_BYTE0     0x12 /* R */
#define PINNACLE_REG_PACKET_BYTE1     0x13 /* R */
#define PINNACLE_REG_PACKET_BYTE2     0x14 /* R */
#define PINNACLE_REG_PACKET_BYTE3     0x15 /* R */
#define PINNACLE_REG_PACKET_BYTE4     0x16 /* R */
#define PINNACLE_REG_PACKET_BYTE5     0x17 /* R */
#define PINNACLE_REG_GPIO_A_CTRL      0x18 /* R/W */
#define PINNACLE_REG_GPIO_A_DATA      0x19 /* R/W */
#define PINNACLE_REG_GPIO_B_CTRL_DATA 0x1A /* R/W */
/* Value of the extended register */
#define PINNACLE_REG_ERA_VALUE        0x1B /* R/W */
/* High byte BIT[15:8] of the 16 bit extended register */
#define PINNACLE_REG_ERA_ADDR_HIGH    0x1C /* R/W */
/* Low byte BIT[7:0] of the 16 bit extended register */
#define PINNACLE_REG_ERA_ADDR_LOW     0x1D /* R/W */
#define PINNACLE_REG_ERA_CTRL         0x1E /* R/W */
#define PINNACLE_REG_PRODUCT_ID       0x1F /* R */

/* Extended Register Access */
#define PINNACLE_ERA_REG_CONFIG 0x0187 /* R/W */

/* Firmware ASIC ID value */
#define PINNACLE_FIRMWARE_ID 0x07

/* Status1 definition */
#define PINNACLE_STATUS1_SW_DR BIT(2)
#define PINNACLE_STATUS1_SW_CC BIT(3)

/* SysConfig1 definition */
#define PINNACLE_SYS_CONFIG1_RESET          BIT(0)
#define PINNACLE_SYS_CONFIG1_SHUTDOWN       BIT(1)
#define PINNACLE_SYS_CONFIG1_LOW_POWER_MODE BIT(2)

/* FeedConfig1 definition */
#define PINNACLE_FEED_CONFIG1_FEED_ENABLE        BIT(0)
#define PINNACLE_FEED_CONFIG1_DATA_MODE_ABSOLUTE BIT(1)
#define PINNACLE_FEED_CONFIG1_FILTER_DISABLE     BIT(2)
#define PINNACLE_FEED_CONFIG1_X_DISABLE          BIT(3)
#define PINNACLE_FEED_CONFIG1_Y_DISABLE          BIT(4)
#define PINNACLE_FEED_CONFIG1_X_INVERT           BIT(6)
#define PINNACLE_FEED_CONFIG1_Y_INVERT           BIT(7)
/* X max to 0 */
#define PINNACLE_FEED_CONFIG1_X_DATA_INVERT      BIT(6)
/* Y max to 0 */
#define PINNACLE_FEED_CONFIG1_Y_DATA_INVERT      BIT(7)

/* FeedConfig2 definition */
#define PINNACLE_FEED_CONFIG2_INTELLIMOUSE_ENABLE   BIT(0)
#define PINNACLE_FEED_CONFIG2_ALL_TAPS_DISABLE      BIT(1)
#define PINNACLE_FEED_CONFIG2_SECONDARY_TAP_DISABLE BIT(2)
#define PINNACLE_FEED_CONFIG2_SCROLL_DISABLE        BIT(3)
#define PINNACLE_FEED_CONFIG2_GLIDE_EXTEND_DISABLE  BIT(4)
/* 90 degrees rotation */
#define PINNACLE_FEED_CONFIG2_SWAP_X_AND_Y          BIT(7)

/* Relative position status in PacketByte0 */
#define PINNACLE_PACKET_BYTE0_BTN_PRIMARY  BIT(0)
#define PINNACLE_PACKET_BYTE0_BTN_SECONDRY BIT(1)

/* Extended Register Access Control */
#define PINNACLE_ERA_CTRL_READ           BIT(0)
#define PINNACLE_ERA_CTRL_WRITE          BIT(1)
#define PINNACLE_ERA_CTRL_READ_AUTO_INC  BIT(2)
#define PINNACLE_ERA_CTRL_WRITE_AUTO_INC BIT(3)
/* Asserting both BIT(1) and BIT(0) means WRITE/Verify */
#define PINNACLE_ERA_CTRL_WRITE_VERIFY   (BIT(1) | BIT(0))
#define PINNACLE_ERA_CTRL_COMPLETE       0x00

/* Extended Register Access Config */
#define PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X1 0x00
#define PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X2 0x40
#define PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X3 0x80
#define PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X4 0xC0

/*
 * Delay and retry count for waiting completion of calibration with 200 ms of
 * timeout.
 */
#define PINNACLE_CALIBRATION_AWAIT_DELAY_POLL_US 50000
#define PINNACLE_CALIBRATION_AWAIT_RETRY_COUNT   4

/*
 * Delay and retry count for waiting completion of ERA command with 50 ms of
 * timeout.
 */
#define PINNACLE_ERA_AWAIT_DELAY_POLL_US 10000
#define PINNACLE_ERA_AWAIT_RETRY_COUNT   5

/* Special definitions */
#define PINNACLE_SPI_FB 0xFB /* Filler byte */
#define PINNACLE_SPI_FC 0xFC /* Auto-increment byte */

/* Read and write masks */
#define PINNACLE_READ_MSK  0xA0
#define PINNACLE_WRITE_MSK 0x80

/* Read and write register addresses */
#define PINNACLE_READ_REG(addr)  (PINNACLE_READ_MSK | addr)
#define PINNACLE_WRITE_REG(addr) (PINNACLE_WRITE_MSK | addr)

struct pinnacle_bus {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		struct spi_dt_spec spi;
#endif
	};
	bool (*is_ready)(const struct pinnacle_bus *bus);
	int (*write)(const struct pinnacle_bus *bus, uint8_t address, uint8_t value);
	int (*seq_write)(const struct pinnacle_bus *bus, uint8_t *address, uint8_t *value,
			 uint8_t count);
	int (*read)(const struct pinnacle_bus *bus, uint8_t address, uint8_t *value);
	int (*seq_read)(const struct pinnacle_bus *bus, uint8_t address, uint8_t *data,
			uint8_t count);
};

enum pinnacle_sensitivity {
	PINNACLE_SENSITIVITY_X1,
	PINNACLE_SENSITIVITY_X2,
	PINNACLE_SENSITIVITY_X3,
	PINNACLE_SENSITIVITY_X4,
};

struct pinnacle_config {
	const struct pinnacle_bus bus;
	struct gpio_dt_spec dr_gpio;

	enum pinnacle_sensitivity sensitivity;
	bool relative_mode;
	uint8_t idle_packets_count;

	bool clipping_enabled;
	bool scaling_enabled;
	bool invert_x;
	bool invert_y;
	bool primary_tap_enabled;
	bool swap_xy;

	uint16_t active_range_x_min;
	uint16_t active_range_x_max;
	uint16_t active_range_y_min;
	uint16_t active_range_y_max;

	uint16_t resolution_x;
	uint16_t resolution_y;
};

union pinnacle_sample {
	struct {
		uint16_t abs_x;
		uint16_t abs_y;
		uint8_t abs_z;
	};
	struct {
		int16_t rel_x;
		int16_t rel_y;
		bool btn_primary;
	};
};

struct pinnacle_data {
	union pinnacle_sample sample;
	const struct device *dev;
	struct gpio_callback dr_cb_data;
	struct k_work work;
};

static inline bool pinnacle_bus_is_ready(const struct device *dev)
{
	const struct pinnacle_config *config = dev->config;

	return config->bus.is_ready(&config->bus);
}

static inline int pinnacle_write(const struct device *dev, uint8_t address, uint8_t value)
{
	const struct pinnacle_config *config = dev->config;

	return config->bus.write(&config->bus, address, value);
}
static inline int pinnacle_seq_write(const struct device *dev, uint8_t *address, uint8_t *value,
				     uint8_t count)
{
	const struct pinnacle_config *config = dev->config;

	return config->bus.seq_write(&config->bus, address, value, count);
}
static inline int pinnacle_read(const struct device *dev, uint8_t address, uint8_t *value)
{
	const struct pinnacle_config *config = dev->config;

	return config->bus.read(&config->bus, address, value);
}

static inline int pinnacle_seq_read(const struct device *dev, uint8_t address, uint8_t *data,
				    uint8_t count)
{
	const struct pinnacle_config *config = dev->config;

	return config->bus.seq_read(&config->bus, address, data, count);
}

static inline int pinnacle_clear_cmd_complete(const struct device *dev)
{
	const struct pinnacle_config *config = dev->config;

	return config->bus.write(&config->bus, PINNACLE_REG_STATUS1, 0x00);
}

static int pinnacle_era_wait_for_completion(const struct device *dev)
{
	int rc;
	uint8_t value;

	rc = WAIT_FOR(!pinnacle_read(dev, PINNACLE_REG_ERA_CTRL, &value) &&
			      value == PINNACLE_ERA_CTRL_COMPLETE,
		      PINNACLE_ERA_AWAIT_RETRY_COUNT * PINNACLE_ERA_AWAIT_DELAY_POLL_US,
		      k_sleep(K_USEC(PINNACLE_ERA_AWAIT_DELAY_POLL_US)));
	if (rc < 0) {
		return -EIO;
	}

	return 0;
}

static int pinnacle_era_write(const struct device *dev, uint16_t address, uint8_t value)
{
	uint8_t address_buf[] = {
		PINNACLE_REG_ERA_VALUE,
		PINNACLE_REG_ERA_ADDR_HIGH,
		PINNACLE_REG_ERA_ADDR_LOW,
		PINNACLE_REG_ERA_CTRL,
	};
	uint8_t value_buf[] = {
		value,
		address >> 8,
		address & 0xFF,
		PINNACLE_ERA_CTRL_WRITE,
	};
	int rc;

	rc = pinnacle_seq_write(dev, address_buf, value_buf, sizeof(address_buf));
	if (rc) {
		return rc;
	}

	return pinnacle_era_wait_for_completion(dev);
}

static int pinnacle_era_read(const struct device *dev, uint16_t address, uint8_t *value)
{
	uint8_t address_buf[] = {
		PINNACLE_REG_ERA_ADDR_HIGH,
		PINNACLE_REG_ERA_ADDR_LOW,
		PINNACLE_REG_ERA_CTRL,
	};
	uint8_t value_buf[] = {
		address >> 8,
		address & 0xFF,
		PINNACLE_ERA_CTRL_READ,
	};
	int rc;

	rc = pinnacle_seq_write(dev, address_buf, value_buf, sizeof(address_buf));
	if (rc) {
		return rc;
	}

	rc = pinnacle_era_wait_for_completion(dev);
	if (rc) {
		return rc;
	}

	return pinnacle_read(dev, PINNACLE_REG_ERA_VALUE, value);
}

static int pinnacle_set_sensitivity(const struct device *dev)
{
	const struct pinnacle_config *config = dev->config;

	uint8_t value;
	int rc;

	rc = pinnacle_era_read(dev, PINNACLE_ERA_REG_CONFIG, &value);
	if (rc) {
		return rc;
	}

	/* Clear BIT(7) and BIT(6) */
	value &= 0x3F;

	switch (config->sensitivity) {
	case PINNACLE_SENSITIVITY_X1:
		value |= PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X1;
		break;
	case PINNACLE_SENSITIVITY_X2:
		value |= PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X2;
		break;
	case PINNACLE_SENSITIVITY_X3:
		value |= PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X3;
		break;
	case PINNACLE_SENSITIVITY_X4:
		value |= PINNACLE_ERA_CONFIG_ADC_ATTENUATION_X4;
		break;
	}

	rc = pinnacle_era_write(dev, PINNACLE_ERA_REG_CONFIG, value);
	if (rc) {
		return rc;
	}

	/* Clear SW_CC after setting sensitivity */
	rc = pinnacle_clear_cmd_complete(dev);
	if (rc) {
		return rc;
	}

	return 0;
}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
static bool pinnacle_is_ready_i2c(const struct pinnacle_bus *bus)
{
	if (!i2c_is_ready_dt(&bus->i2c)) {
		LOG_ERR("I2C bus %s is not ready", bus->i2c.bus->name);
		return false;
	}

	return true;
}

static int pinnacle_write_i2c(const struct pinnacle_bus *bus, uint8_t address, uint8_t value)
{
	uint8_t buf[] = {PINNACLE_WRITE_REG(address), value};

	return i2c_write_dt(&bus->i2c, buf, 2);
}

static int pinnacle_seq_write_i2c(const struct pinnacle_bus *bus, uint8_t *address, uint8_t *value,
				  uint8_t count)
{
	uint8_t buf[count * 2];

	for (uint8_t i = 0; i < count; ++i) {
		buf[i * 2] = PINNACLE_WRITE_REG(address[i]);
		buf[i * 2 + 1] = value[i];
	}

	return i2c_write_dt(&bus->i2c, buf, count * 2);
}

static int pinnacle_read_i2c(const struct pinnacle_bus *bus, uint8_t address, uint8_t *value)
{
	uint8_t reg = PINNACLE_READ_REG(address);

	return i2c_write_read_dt(&bus->i2c, &reg, 1, value, 1);
}

static int pinnacle_seq_read_i2c(const struct pinnacle_bus *bus, uint8_t address, uint8_t *buf,
				 uint8_t count)
{
	uint8_t reg = PINNACLE_READ_REG(address);

	return i2c_burst_read_dt(&bus->i2c, reg, buf, count);
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
static bool pinnacle_is_ready_spi(const struct pinnacle_bus *bus)
{
	if (!spi_is_ready_dt(&bus->spi)) {
		LOG_ERR("SPI bus %s is not ready", bus->spi.bus->name);
		return false;
	}

	return true;
}

static int pinnacle_write_spi(const struct pinnacle_bus *bus, uint8_t address, uint8_t value)
{
	uint8_t tx_data[] = {
		PINNACLE_WRITE_REG(address),
		value,
	};
	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = sizeof(tx_data),
	}};
	const struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	return spi_write_dt(&bus->spi, &tx_set);
}

static int pinnacle_seq_write_spi(const struct pinnacle_bus *bus, uint8_t *address, uint8_t *value,
				  uint8_t count)
{
	uint8_t tx_data[count * 2];
	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = sizeof(tx_data),
	}};
	const struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	for (uint8_t i = 0; i < count; ++i) {
		tx_data[i * 2] = PINNACLE_WRITE_REG(address[i]);
		tx_data[i * 2 + 1] = value[i];
	}

	return spi_write_dt(&bus->spi, &tx_set);
}

static int pinnacle_read_spi(const struct pinnacle_bus *bus, uint8_t address, uint8_t *value)
{
	uint8_t tx_data[] = {
		PINNACLE_READ_REG(address),
		PINNACLE_SPI_FB,
		PINNACLE_SPI_FB,
		PINNACLE_SPI_FB,
	};
	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = sizeof(tx_data),
	}};
	const struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	const struct spi_buf rx_buf[] = {
		{
			.buf = NULL,
			.len = 3,
		},
		{
			.buf = value,
			.len = 1,
		},
	};
	const struct spi_buf_set rx_set = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	int rc;

	rc = spi_transceive_dt(&bus->spi, &tx_set, &rx_set);
	if (rc) {
		LOG_ERR("Failed to read from SPI %s", bus->spi.bus->name);
		return rc;
	}

	return 0;
}

static int pinnacle_seq_read_spi(const struct pinnacle_bus *bus, uint8_t address, uint8_t *buf,
				 uint8_t count)
{

	uint8_t size = count + 3;
	uint8_t tx_data[size];

	tx_data[0] = PINNACLE_READ_REG(address);
	tx_data[1] = PINNACLE_SPI_FC;
	tx_data[2] = PINNACLE_SPI_FC;

	uint8_t i = 3;

	for (; i < (count + 2); ++i) {
		tx_data[i] = PINNACLE_SPI_FC;
	}

	tx_data[i++] = PINNACLE_SPI_FB;

	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = size,
	}};
	const struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = 1,
	};

	const struct spi_buf rx_buf[] = {
		{
			.buf = NULL,
			.len = 3,
		},
		{
			.buf = buf,
			.len = count,
		},
	};
	const struct spi_buf_set rx_set = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	int rc;

	rc = spi_transceive_dt(&bus->spi, &tx_set, &rx_set);
	if (rc) {
		LOG_ERR("Failed to read from SPI %s", bus->spi.bus->name);
		return rc;
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

static void pinnacle_decode_sample(const struct device *dev, uint8_t *rx,
				   union pinnacle_sample *sample)
{
	const struct pinnacle_config *config = dev->config;

	if (config->relative_mode) {
		if (config->primary_tap_enabled) {
			sample->btn_primary = (rx[0] & BIT(0)) == BIT(0);
		}
		sample->rel_x = ((rx[0] & BIT(4)) == BIT(4)) ? -(256 - rx[1]) : rx[1];
		sample->rel_y = ((rx[0] & BIT(5)) == BIT(5)) ? -(256 - rx[2]) : rx[2];
	} else {
		sample->abs_x = ((rx[2] & 0x0F) << 8) | rx[0];
		sample->abs_y = ((rx[2] & 0xF0) << 4) | rx[1];
		sample->abs_z = rx[3] & 0x3F;
	}
}

static bool pinnacle_is_idle_sample(const union pinnacle_sample *sample)
{
	return (sample->abs_x == 0 && sample->abs_y == 0 && sample->abs_z == 0);
}

static void pinnacle_clip_sample(const struct device *dev, union pinnacle_sample *sample)
{
	const struct pinnacle_config *config = dev->config;

	if (sample->abs_x < config->active_range_x_min) {
		sample->abs_x = config->active_range_x_min;
	}
	if (sample->abs_x > config->active_range_x_max) {
		sample->abs_x = config->active_range_x_max;
	}
	if (sample->abs_y < config->active_range_y_min) {
		sample->abs_y = config->active_range_y_min;
	}
	if (sample->abs_y > config->active_range_y_max) {
		sample->abs_y = config->active_range_y_max;
	}
}

static void pinnacle_scale_sample(const struct device *dev, union pinnacle_sample *sample)
{
	const struct pinnacle_config *config = dev->config;

	uint16_t range_x = config->active_range_x_max - config->active_range_x_min;
	uint16_t range_y = config->active_range_y_max - config->active_range_y_min;

	sample->abs_x = (uint16_t)((uint32_t)(sample->abs_x - config->active_range_x_min) *
				   config->resolution_x / range_x);
	sample->abs_y = (uint16_t)((uint32_t)(sample->abs_y - config->active_range_y_min) *
				   config->resolution_y / range_y);
}

static int pinnacle_sample_fetch(const struct device *dev, union pinnacle_sample *sample)
{
	const struct pinnacle_config *config = dev->config;

	uint8_t rx[4];
	int rc;

	if (config->relative_mode) {
		rc = pinnacle_seq_read(dev, PINNACLE_REG_PACKET_BYTE0, rx, 3);
	} else {
		rc = pinnacle_seq_read(dev, PINNACLE_REG_PACKET_BYTE2, rx, 4);
	}

	if (rc) {
		LOG_ERR("Failed to read data from SPI device");
		return rc;
	}

	pinnacle_decode_sample(dev, rx, sample);

	rc = pinnacle_write(dev, PINNACLE_REG_STATUS1, 0x00);
	if (rc) {
		LOG_ERR("Failed to clear SW_CC and SW_DR");
		return rc;
	}

	return 0;
}

static int pinnacle_handle_interrupt(const struct device *dev)
{
	const struct pinnacle_config *config = dev->config;
	struct pinnacle_data *drv_data = dev->data;
	union pinnacle_sample *sample = &drv_data->sample;

	int rc;

	rc = pinnacle_sample_fetch(dev, sample);
	if (rc) {
		LOG_ERR("Failed to read data packets");
		return rc;
	}

	if (config->relative_mode) {
		input_report_rel(dev, INPUT_REL_X, sample->rel_x, false, K_FOREVER);
		input_report_rel(dev, INPUT_REL_Y, sample->rel_y, !config->primary_tap_enabled,
				 K_FOREVER);
		if (config->primary_tap_enabled) {
			input_report_key(dev, INPUT_BTN_TOUCH, sample->btn_primary, true,
					 K_FOREVER);
		}
	} else {
		if (config->clipping_enabled && !pinnacle_is_idle_sample(sample)) {
			pinnacle_clip_sample(dev, sample);
			if (config->scaling_enabled) {
				pinnacle_scale_sample(dev, sample);
			}
		}

		input_report_abs(dev, INPUT_ABS_X, sample->abs_x, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, sample->abs_y, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Z, sample->abs_z, true, K_FOREVER);
	}

	return 0;
}

static void pinnacle_data_ready_gpio_callback(const struct device *dev, struct gpio_callback *cb,
					      uint32_t pins)
{
	struct pinnacle_data *drv_data = CONTAINER_OF(cb, struct pinnacle_data, dr_cb_data);

	k_work_submit(&drv_data->work);
}

static void pinnacle_work_cb(struct k_work *work)
{
	struct pinnacle_data *drv_data = CONTAINER_OF(work, struct pinnacle_data, work);

	pinnacle_handle_interrupt(drv_data->dev);
}

int pinnacle_init_interrupt(const struct device *dev)
{
	struct pinnacle_data *drv_data = dev->data;
	const struct pinnacle_config *config = dev->config;
	const struct gpio_dt_spec *gpio = &config->dr_gpio;

	int rc;

	drv_data->dev = dev;
	drv_data->work.handler = pinnacle_work_cb;

	/* Configure GPIO pin for HW_DR signal */
	rc = gpio_is_ready_dt(gpio);
	if (!rc) {
		LOG_ERR("GPIO device %s/%d is not ready", gpio->port->name, gpio->pin);
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(gpio, GPIO_INPUT);
	if (rc) {
		LOG_ERR("Failed to configure %s/%d as input", gpio->port->name, gpio->pin);
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc) {
		LOG_ERR("Failed to configured interrupt for %s/%d", gpio->port->name, gpio->pin);
		return rc;
	}

	gpio_init_callback(&drv_data->dr_cb_data, pinnacle_data_ready_gpio_callback,
			   BIT(gpio->pin));

	rc = gpio_add_callback(gpio->port, &drv_data->dr_cb_data);
	if (rc) {
		LOG_ERR("Failed to configured interrupt for %s/%d", gpio->port->name, gpio->pin);
		return rc;
	}

	return 0;
}

static int pinnacle_init(const struct device *dev)
{
	const struct pinnacle_config *config = dev->config;

	int rc;
	uint8_t value;

	if (!pinnacle_bus_is_ready(dev)) {
		return -ENODEV;
	}

	rc = pinnacle_read(dev, PINNACLE_REG_FIRMWARE_ID, &value);
	if (rc) {
		LOG_ERR("Failed to read FirmwareId");
		return rc;
	}

	if (value != PINNACLE_FIRMWARE_ID) {
		LOG_ERR("Incorrect Firmware ASIC ID %x", value);
		return -ENODEV;
	}

	/* Wait until the calibration is completed (SW_CC is asserted) */
	rc = WAIT_FOR(!pinnacle_read(dev, PINNACLE_REG_STATUS1, &value) &&
			      (value & PINNACLE_STATUS1_SW_CC) == PINNACLE_STATUS1_SW_CC,
		      PINNACLE_CALIBRATION_AWAIT_RETRY_COUNT *
			      PINNACLE_CALIBRATION_AWAIT_DELAY_POLL_US,
		      k_sleep(K_USEC(PINNACLE_CALIBRATION_AWAIT_DELAY_POLL_US)));
	if (rc < 0) {
		LOG_ERR("Failed to wait for calibration complition");
		return -EIO;
	}

	/* Clear SW_CC after Power on Reset */
	rc = pinnacle_clear_cmd_complete(dev);
	if (rc) {
		LOG_ERR("Failed to clear SW_CC in Status1");
		return -EIO;
	}

	/* Set trackpad sensitivity */
	rc = pinnacle_set_sensitivity(dev);
	if (rc) {
		LOG_ERR("Failed to set sensitivity");
		return -EIO;
	}

	rc = pinnacle_write(dev, PINNACLE_REG_SYS_CONFIG1, 0x00);
	if (rc) {
		LOG_ERR("Failed to write SysConfig1");
		return rc;
	}

	/* Relative mode features */
	if (config->relative_mode) {
		value = (PINNACLE_FEED_CONFIG2_GLIDE_EXTEND_DISABLE |
			 PINNACLE_FEED_CONFIG2_SCROLL_DISABLE |
			 PINNACLE_FEED_CONFIG2_SECONDARY_TAP_DISABLE);
		if (config->swap_xy) {
			value |= PINNACLE_FEED_CONFIG2_SWAP_X_AND_Y;
		}
		if (!config->primary_tap_enabled) {
			value |= PINNACLE_FEED_CONFIG2_ALL_TAPS_DISABLE;
		}
	} else {
		value = (PINNACLE_FEED_CONFIG2_GLIDE_EXTEND_DISABLE |
			 PINNACLE_FEED_CONFIG2_SCROLL_DISABLE |
			 PINNACLE_FEED_CONFIG2_SECONDARY_TAP_DISABLE |
			 PINNACLE_FEED_CONFIG2_ALL_TAPS_DISABLE);
	}
	rc = pinnacle_write(dev, PINNACLE_REG_FEED_CONFIG2, value);
	if (rc) {
		LOG_ERR("Failed to write FeedConfig2");
		return rc;
	}

	/* Data output flags */
	value = PINNACLE_FEED_CONFIG1_FEED_ENABLE;
	if (!config->relative_mode) {
		value |= PINNACLE_FEED_CONFIG1_DATA_MODE_ABSOLUTE;
		if (config->invert_x) {
			value |= PINNACLE_FEED_CONFIG1_X_INVERT;
		}
		if (config->invert_y) {
			value |= PINNACLE_FEED_CONFIG1_Y_INVERT;
		}
	}
	rc = pinnacle_write(dev, PINNACLE_REG_FEED_CONFIG1, value);
	if (rc) {
		LOG_ERR("Failed to enable Feed in FeedConfig1");
		return rc;
	}

	/* Configure count of Z-Idle packets */
	rc = pinnacle_write(dev, PINNACLE_REG_Z_IDLE, config->idle_packets_count);
	if (rc) {
		LOG_ERR("Failed to set count of Z-idle packets");
		return rc;
	}

	rc = pinnacle_init_interrupt(dev);
	if (rc) {
		LOG_ERR("Failed to initialize interrupts");
		return rc;
	}

	return 0;
}

#define PINNACLE_CONFIG_BUS_I2C(inst)                                                              \
	.bus = {                                                                                   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.is_ready = pinnacle_is_ready_i2c,                                                 \
		.write = pinnacle_write_i2c,                                                       \
		.seq_write = pinnacle_seq_write_i2c,                                               \
		.read = pinnacle_read_i2c,                                                         \
		.seq_read = pinnacle_seq_read_i2c,                                                 \
	}

#define PINNACLE_SPI_OP (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_WORD_SET(8))
#define PINNACLE_CONFIG_BUS_SPI(inst)                                                              \
	.bus = {                                                                                   \
		.spi = SPI_DT_SPEC_INST_GET(inst, PINNACLE_SPI_OP, 0U),                            \
		.is_ready = pinnacle_is_ready_spi,                                                 \
		.write = pinnacle_write_spi,                                                       \
		.seq_write = pinnacle_seq_write_spi,                                               \
		.read = pinnacle_read_spi,                                                         \
		.seq_read = pinnacle_seq_read_spi,                                                 \
	}

#define PINNACLE_DEFINE(inst)                                                                      \
	static const struct pinnacle_config pinnacle_config_##inst = {                             \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c), (PINNACLE_CONFIG_BUS_I2C(inst),), ())       \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (PINNACLE_CONFIG_BUS_SPI(inst),), ())       \
		.dr_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, data_ready_gpios, {}),                   \
		.relative_mode = DT_INST_ENUM_IDX(inst, data_mode),                                \
		.sensitivity = DT_INST_ENUM_IDX(inst, sensitivity),                                \
		.idle_packets_count = DT_INST_PROP(inst, idle_packets_count),                      \
		.clipping_enabled = DT_INST_PROP(inst, clipping_enable),                           \
		.active_range_x_min = DT_INST_PROP(inst, active_range_x_min),                      \
		.active_range_x_max = DT_INST_PROP(inst, active_range_x_max),                      \
		.active_range_y_min = DT_INST_PROP(inst, active_range_y_min),                      \
		.active_range_y_max = DT_INST_PROP(inst, active_range_y_max),                      \
		.scaling_enabled = DT_INST_PROP(inst, scaling_enable),                             \
		.resolution_x = DT_INST_PROP(inst, scaling_x_resolution),                          \
		.resolution_y = DT_INST_PROP(inst, scaling_y_resolution),                          \
		.invert_x = DT_INST_PROP(inst, invert_x),                                          \
		.invert_y = DT_INST_PROP(inst, invert_y),                                          \
		.primary_tap_enabled = DT_INST_PROP(inst, primary_tap_enable),                     \
		.swap_xy = DT_INST_PROP(inst, swap_xy),                                            \
	};                                                                                         \
	static struct pinnacle_data pinnacle_data_##inst;                                          \
	DEVICE_DT_INST_DEFINE(inst, pinnacle_init, NULL, &pinnacle_data_##inst,                    \
			      &pinnacle_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);                                                               \
	BUILD_ASSERT(DT_INST_PROP(inst, active_range_x_min) <                                      \
			     DT_INST_PROP(inst, active_range_x_max),                               \
		     "active-range-x-min must be less than active-range-x-max");                   \
	BUILD_ASSERT(DT_INST_PROP(inst, active_range_y_min) <                                      \
			     DT_INST_PROP(inst, active_range_y_max),                               \
		     "active_range-y-min must be less than active_range-y-max");                   \
	BUILD_ASSERT(DT_INST_PROP(inst, scaling_x_resolution) > 0,                                 \
		     "scaling-x-resolution must be positive");                                     \
	BUILD_ASSERT(DT_INST_PROP(inst, scaling_y_resolution) > 0,                                 \
		     "scaling-y-resolution must be positive");                                     \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP(inst, idle_packets_count), 0, UINT8_MAX),               \
		     "idle-packets-count must be in range [0:255]");

DT_INST_FOREACH_STATUS_OKAY(PINNACLE_DEFINE)
