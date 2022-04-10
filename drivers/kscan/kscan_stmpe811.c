/*
 * Copyright (c) Konstantinos Papadopoulos <kostas.papadopulos@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stmpe811

#include <sys/byteorder.h>
#include <drivers/kscan.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stmpe811, CONFIG_KSCAN_LOG_LEVEL);

/* STMPE811 general registers */
#define STMPE811_REG_CHIP_ID			0x00
#define STMPE811_REG_ID_VER			0x02
#define STMPE811_REG_SYS_CTRL1			0x03
#define STMPE811_REG_SYS_CTRL2			0x04
#define STMPE811_REG_SPI_CFG			0x08
#define STMPE811_REG_INT_CTRL			0x09
#define STMPE811_REG_INT_EN			0x0A
#define STMPE811_REG_INT_STA			0x0B
#define STMPE811_REG_GPIO_ALT_FUNCT		0x17
#define STMPE811_REG_ADC_CTRL1			0x20
#define STMPE811_REG_ADC_CTRL2			0x21
#define STMPE811_REG_ADC_CAPT			0x22

/* STMPE811 touch screen registers */
#define STMPE811_REG_TSC_CTRL			0x40
#define STMPE811_REG_TSC_CFG			0x41
#define STMPE811_REG_WDW_TR_X			0x42
#define STMPE811_REG_WDW_TR_Y			0x44
#define STMPE811_REG_WDW_BL_X			0x46
#define STMPE811_REG_WDW_BL_Y			0x48
#define STMPE811_REG_FIFO_TH			0x4A
#define STMPE811_REG_FIFO_STA			0x4B
#define STMPE811_REG_FIFO_SIZE			0x4C
#define STMPE811_REG_TSC_DATA_X			0x4D
#define STMPE811_REG_TSC_DATA_Y			0x4F
#define STMPE811_REG_TSC_DATA_Z			0x51
#define STMPE811_REG_TSC_DATA_XYZ		0x52
#define STMPE811_REG_TSC_FRACTION_Z		0x56
#define STMPE811_REG_TSC_DATA			0x57
#define STMPE811_REG_TSC_I_DRIVE		0x58
#define STMPE811_REG_TSC_SHIELD			0x59
#define STMPE811_REG_TSC_DATA_NON_INC   0xD7

/* Chip IDs */
#define STMPE811_ID				0x811

/* SYS control definitions */
#define STMPE811_RESET				0x02
#define STMPE811_HIBERNATE			0x01

/* Global interrupt Enable bit */
#define STMPE811_INT_EN				0x01

/* STMPE811 functionalities */
#define STMPE811_ADC_FCT			0x01
#define STMPE811_TS_FCT				0x02
#define STMPE811_IO_FCT				0x04
#define STMPE811_TEMPSENS_FCT			0x08

#define STMPE811_INT_EN_FIFO_TH			0x10

/** STMPE811 configuration (DT). */
struct stmpe811_config {
	/** I2C bus. */
	struct i2c_dt_spec bus;
#ifdef CONFIG_KSCAN_STMPE811_INTERRUPT
	/** Interrupt GPIO information. */
	struct gpio_dt_spec int_gpio;
#endif
};

/** STMPE811 data. */
struct stmpe811_data {
	/** Device pointer. */
	const struct device *dev;
	/** KSCAN Callback. */
	kscan_callback_t callback;
	/** Work queue (for deferred read). */
	struct k_work work;
#ifdef CONFIG_KSCAN_STMPE811_INTERRUPT
	/** Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#else
	/** Timer (polling mode). */
	struct k_timer timer;
#endif
};

static int stmpe811_reset(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->bus, STMPE811_REG_SYS_CTRL1,
								STMPE811_RESET, STMPE811_RESET);

	k_msleep(10);

	ret = i2c_reg_update_byte_dt(&config->bus, STMPE811_REG_SYS_CTRL1,
								STMPE811_RESET, 0);
	k_msleep(10);
	return ret;
}

static int stmpe811_process(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	struct stmpe811_data *data = dev->data;

	int ret = 0;
	uint8_t reg = 0;
	uint8_t touch_data[3U];
	uint8_t pressed;
	uint32_t xy_raw;
	uint16_t row = 0;
	uint16_t col = 0;

	/* Read touchscreen control register */
	ret = i2c_reg_read_byte_dt(&config->bus, STMPE811_REG_TSC_CTRL, &pressed);
	pressed = (pressed & 0x80) >> 7;

	/* Read FIFO status register */
	ret = i2c_reg_read_byte_dt(&config->bus, STMPE811_REG_FIFO_STA, &reg);
	reg &= STMPE811_INT_EN_FIFO_TH;

	/* Check if touchscreen is pressed and the FIFO has reached the threshold */
	if (pressed && reg == STMPE811_INT_EN_FIFO_TH) {

		/* This driver is fixed to XY readings so we read only 3 bytes from TSC_DATA */
		ret = i2c_burst_read_dt(&config->bus, STMPE811_REG_TSC_DATA_NON_INC, touch_data, 3);
		xy_raw = (touch_data[0] << 16)|(touch_data[1] << 8)|(touch_data[2] << 0);
		row = xy_raw >> 12;
		col = xy_raw & 0xFFF;

		/* Clear FIFO status register to clear the buffer */
		ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_FIFO_STA, 0x1);
		ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_FIFO_STA, 0x0);

		col -= 350;

		if (row <= 3000) {
			row = 3900 - row;
		} else {
			row = 3800 - row;
		}

		/* Normalize */
		col = col/11;
		row = row/15;

		if (col <= 0) {
			col = 1;
		} else if (col >= 320) {
			col = 319;
		}

		if (row <= 0) {
			row = 1;
		} else if (row >= 240) {
			row = 239;
		}

		LOG_DBG("row: %d, col: %d", row, col);

		/* Clear interrupt flags */
		ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_INT_STA, 0xFF);
	}

	data->callback(dev, row, col, pressed);

	return ret;
}

static void stmpe811_work_handler(struct k_work *work)
{
	struct stmpe811_data *data = CONTAINER_OF(work, struct stmpe811_data, work);

	stmpe811_process(data->dev);
}

#ifdef CONFIG_KSCAN_STMPE811_INTERRUPT
static void stmpe811_isr_handler(const struct device *dev,
			       struct gpio_callback *cb, uint32_t pins)
{
	struct stmpe811_data *data = CONTAINER_OF(cb, struct stmpe811_data, int_gpio_cb);

	k_work_submit(&data->work);
}
#else
static void stmpe811_timer_handler(struct k_timer *timer)
{
	struct stmpe811_data *data = CONTAINER_OF(timer, struct stmpe811_data, timer);

	k_work_submit(&data->work);
}
#endif

static int stmpe811_enable_callback(const struct device *dev)
{
	struct stmpe811_data *data = dev->data;

#ifdef CONFIG_KSCAN_STMPE811_INTERRUPT
	const struct stmpe811_config *config = dev->config;

	gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
#else
	k_timer_start(&data->timer, K_MSEC(CONFIG_KSCAN_STMPE811_PERIOD),
		      K_MSEC(CONFIG_KSCAN_STMPE811_PERIOD));
#endif

	return 0;
}

static int stmpe811_disable_callback(const struct device *dev)
{
	struct stmpe811_data *data = dev->data;

#ifdef CONFIG_KSCAN_STMPE811_INTERRUPT
	const struct stmpe811_config *config = dev->config;

	gpio_remove_callback(config->int_gpio.port, &data->int_gpio_cb);
#else
	k_timer_stop(&data->timer);
#endif

	return 0;
}

static int stmpe811_configure(const struct device *dev,
			    kscan_callback_t callback)
{
	struct stmpe811_data *data = dev->data;

	if (!callback) {
		LOG_ERR("Invalid callback (NULL)");
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int stmpe811_init(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	struct stmpe811_data *data = dev->data;

	int ret = 0;
	uint16_t chip_id = 0;
	uint8_t cmd = 0;
	uint8_t reg = 0;

	data->dev = dev;
	k_work_init(&data->work, stmpe811_work_handler);

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	cmd = STMPE811_REG_CHIP_ID;
	ret = i2c_burst_read_dt(&config->bus, STMPE811_REG_CHIP_ID, (uint8_t *)&chip_id, 2);
	if (ret < 0) {
		LOG_ERR("failed to read Chip ID");
		return ret;
	}

	chip_id = sys_be16_to_cpu(chip_id);

	if (chip_id != STMPE811_ID) {
		LOG_ERR("wrong chip id, returned 0x%x", chip_id);
		return -ENODEV;
	}

	ret = stmpe811_reset(dev);
	if (ret < 0) {
		LOG_ERR("failed to reset");
		return ret;
	}

	/* Set the Functionalities to be Enabled */
	reg &= ~(STMPE811_TS_FCT | STMPE811_ADC_FCT);

	/* Write the new register value */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_SYS_CTRL2, reg);
	if (ret < 0) {
		LOG_ERR("failed to write SYS_CTRL2");
		return ret;
	}

	/* Select TSC pins in TSC alternate mode */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_GPIO_ALT_FUNCT, 0x0);
	if (ret < 0) {
		LOG_ERR("failed to write ALT_FUNCT");
		return ret;
	}

	/* Select Sample Time, bit number and ADC Reference */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_ADC_CTRL1, 0x49);
	if (ret < 0) {
		LOG_ERR("failed to write ADC_CTRL1");
		return ret;
	}

	k_msleep(2);

	/* Select the ADC clock speed: 3.25 MHz */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_ADC_CTRL2, 0x01);
	if (ret < 0) {
		LOG_ERR("failed to write ADC_CTRL2");
		return ret;
	}

	/* Configuration:
	 * Touch average control    : 4 samples
	 * Touch detect delay       : 1 mS
	 * Touch delay time         : 500 uS
	 * Panel driver setting time: 500 uS
	 */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_TSC_CFG, 0x9a);
	if (ret < 0) {
		LOG_ERR("failed to write TSC_CFG");
		return ret;
	}

	/* Configure the Touch FIFO threshold: 1 points reading */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_FIFO_TH, 0x01);
	if (ret < 0) {
		LOG_ERR("failed to write FIFO_TH");
		return ret;
	}

	/* Clear the FIFO memory content. */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_FIFO_STA, 0x01);
	if (ret < 0) {
		LOG_ERR("failed to write FIFO_STA");
		return ret;
	}

	/* Put the FIFO back into operation mode  */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_FIFO_STA, 0x00);
	if (ret < 0) {
		LOG_ERR("failed to write FIFO_STA");
		return ret;
	}

	/* Set the driving capability (limit) of the device for TSC pins: 50mA */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_TSC_I_DRIVE, 0x01);
	if (ret < 0) {
		LOG_ERR("failed to write TSC_I_DRIVE");
		return ret;
	}

	/* Touch screen control configuration (enable TSC):
	 * - No window tracking index
	 * - XY acquisition mode
	 */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_TSC_CTRL, 0x03);
	if (ret < 0) {
		LOG_ERR("failed to write TSC_CTRL");
		return ret;
	}

	/*  Clear all the status pending bits if any */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_INT_STA, 0xFF);
	if (ret < 0) {
		LOG_ERR("failed to write INT_STA");
		return ret;
	}

	k_work_init(&data->work, stmpe811_work_handler);

#ifdef CONFIG_KSCAN_STMPE811_INTERRUPT
	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio,
					    GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO.");
		return ret;
	}

	/* Enable global interrupt on STMPE811 */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_INT_CTRL, 0x1);
	if (ret < 0) {
		LOG_ERR("failed to write INT_CTRL");
		return ret;
	}

	/* Enable global FIFO_TH interrupt on STMPE811 */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_REG_INT_EN, 0x2);
	if (ret < 0) {
		LOG_ERR("failed to write INT_EN");
		return ret;
	}
	gpio_init_callback(&data->int_gpio_cb, stmpe811_isr_handler,
			   BIT(config->int_gpio.pin));
#else
	k_timer_init(&data->timer, stmpe811_timer_handler, NULL);
#endif

	return ret;
}

static const struct kscan_driver_api stmpe811_driver_api = {
	.config = stmpe811_configure,
	.enable_callback = stmpe811_enable_callback,
	.disable_callback = stmpe811_disable_callback,
};

#define STMPE811_INIT(index)									\
	static const struct stmpe811_config stmpe811_config_##index = {	\
		.bus = I2C_DT_SPEC_INST_GET(index),						\
		IF_ENABLED(CONFIG_KSCAN_STMPE811_INTERRUPT,					\
		(.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),))		\
	};											\
	static struct stmpe811_data stmpe811_data_##index;				\
	DEVICE_DT_INST_DEFINE(index, stmpe811_init, NULL,				\
			    &stmpe811_data_##index, &stmpe811_config_##index,	\
			    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,			\
			    &stmpe811_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STMPE811_INIT)
