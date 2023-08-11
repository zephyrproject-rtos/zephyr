/**
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT st_stmpe811

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stmpe811, CONFIG_INPUT_LOG_LEVEL);

#define CHIP_ID 0x0811U

/* Touch Screen Pins definition */
#define STMPE811_GPIO_PIN_4 BIT(4)
#define STMPE811_GPIO_PIN_5 BIT(5)
#define STMPE811_GPIO_PIN_6 BIT(6)
#define STMPE811_GPIO_PIN_7 BIT(7)

#define STMPE811_TOUCH_YD STMPE811_GPIO_PIN_7
#define STMPE811_TOUCH_XD STMPE811_GPIO_PIN_6
#define STMPE811_TOUCH_YU STMPE811_GPIO_PIN_5
#define STMPE811_TOUCH_XU STMPE811_GPIO_PIN_4
#define STMPE811_TOUCH_IO_ALL                                                                      \
	(STMPE811_TOUCH_YD | STMPE811_TOUCH_XD | STMPE811_TOUCH_YU | STMPE811_TOUCH_XU)

/* Registers */
#define STMPE811_CHP_ID_LSB_REG       0x00U
#define STMPE811_ADC_CTRL1_REG        0x20U
#define STMPE811_ADC_CTRL2_REG        0x21U
#define STMPE811_SYS_CTRL1_REG        0x03U
#define STMPE811_SYS_CTRL2_REG        0x04U
#define STMPE811_TSC_CFG_REG          0x41U
#define STMPE811_IO_AF_REG            0x17U
#define STMPE811_FIFO_TH_REG          0x4AU
#define STMPE811_FIFO_STA_REG         0x4BU
#define STMPE811_FIFO_SIZE_REG        0x4CU
#define STMPE811_TSC_FRACT_XYZ_REG    0x56U
#define STMPE811_TSC_I_DRIVE_REG      0x58U
#define STMPE811_TSC_CTRL_REG         0x40U
#define STMPE811_INT_STA_REG          0x0BU
#define STMPE811_TSC_DATA_NON_INC_REG 0xD7U
#define STMPE811_INT_CTRL_REG         0x09U
#define STMPE811_INT_EN_REG           0x0AU

/* Touch detected bit */
#define STMPE811_TSC_CTRL_BIT_TOUCH_DET BIT(7)

/* Global interrupt enable bit */
#define STMPE811_INT_CTRL_BIT_GLOBAL_INT BIT(0)

/* IO expander functionalities */
#define STMPE811_SYS_CTRL2_BIT_ADC_FCT BIT(0)
#define STMPE811_SYS_CTRL2_BIT_TS_FCT  BIT(1)
#define STMPE811_SYS_CTRL2_BIT_IO_FCT  BIT(2)

/* Global Interrupts definitions */
#define STMPE811_INT_BIT_FIFO_THRESHOLD  BIT(1)       /* FIFO above threshold interrupt       */
#define STMPE811_INT_BIT_TOUCH           BIT(0)       /* Touch/release is detected interrupt  */
#define STMPE811_INT_ALL		 BIT_MASK(8)  /* All interrupts */

/* Reset control */
#define STMPE811_SYS_CTRL1_RESET_ON   0
#define STMPE811_SYS_CTRL1_RESET_SOFT BIT(1) /* Soft reset */

/* Delays to ensure registers erasing */
#define STMPE811_RESET_DELAY_MS 10
#define STMPE811_WAIT_DELAY_MS  2

/* Configuration */
#define STMPE811_FIFO_TH_SINGLE_POINT 1
#define STMPE811_FIFO_STA_CLEAR       1
#define STMPE811_FIFO_STA_OPERATIONAL 0
#define STMPE811_TSC_I_DRIVE_LIMIT    1

/**
 * Touch Screen Control
 *
 * - bits [1-3]   X, Y only acquisition mode
 */
#define STMPE811_TSC_CTRL_CONF 3U

/**
 * Analog-to-digital Converter
 *
 * - bit  [3]   selects 12 bit ADC
 * - bits [4-6] select ADC conversion time = 80
 */
#define STMPE811_ADC_CTRL1_CONF 0x48U

/**
 * ADC clock speed: 3.25 MHz
 *
 * - 00 : 1.625 MHz
 * - 01 : 3.25  MHz
 * - 10 : 6.5   MHz
 * - 11 : 6.5   MHz
 */
#define STMPE811_ADC_CLOCK_SPEED 1

/**
 * Range and accuracy of the pressure measurement (Z)
 *
 * - Fractional part : 7
 * - Whole part      : 1
 */
#define STMPE811_TSC_FRACT_XYZ_CONF 1

struct stmpe811_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec int_gpio;
	uint8_t panel_driver_settling_time_us;
	uint8_t touch_detect_delay_us;
	uint8_t touch_average_control;
	uint8_t tracking_index;
	uint16_t screen_width;
	uint16_t screen_height;
	int raw_x_min;
	int raw_y_min;
	uint16_t raw_x_max;
	uint16_t raw_y_max;
};

struct stmpe811_data {
	const struct device *dev;
	struct k_work processing_work;
	struct gpio_callback int_gpio_cb;
	uint32_t touch_x;
	uint32_t touch_y;
};

static int stmpe811_reset(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;

	/* Power down the stmpe811 */
	int ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_SYS_CTRL1_REG,
					STMPE811_SYS_CTRL1_RESET_SOFT);

	if (ret < 0) {
		return ret;
	}
	k_msleep(STMPE811_RESET_DELAY_MS);

	/* Power on the stmpe811 after the power off => all registers are reinitialized */
	ret = i2c_reg_write_byte_dt(&config->bus, STMPE811_SYS_CTRL1_REG,
				    STMPE811_SYS_CTRL1_RESET_ON);
	if (ret < 0) {
		return ret;
	}
	k_msleep(STMPE811_WAIT_DELAY_MS);

	return 0;
}

static int stmpe811_io_enable_af(const struct device *dev, uint32_t io_pin)
{
	const struct stmpe811_config *config = dev->config;

	/* Apply ~io_pin as a mask to the current register value */
	return i2c_reg_update_byte_dt(&config->bus, STMPE811_IO_AF_REG, io_pin, 0);
}

static uint8_t stmpe811_tsc_config_bits(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;

	/**
	 * Configuration:
	 * - bits [0-2] : panel driver settling time
	 * - bits [3-5] : touch detect delay
	 * - bits [6-7] : touch average control
	 */

	return config->panel_driver_settling_time_us | config->touch_detect_delay_us << 3 |
	       config->touch_average_control << 6;
}

static uint8_t stmpe811_tsc_control_bits(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;

	/**
	 * Touch Screen Control
	 *
	 * - bit  [0]     enables TSC
	 * - bits [1-3]   X, Y only acquisition mode
	 * - bits [4-6]   window tracking index (set from config)
	 * - bit  [7]     TSC status (writing has no effect)
	 */

	return STMPE811_TSC_CTRL_CONF | config->tracking_index << 4;
}

static int stmpe811_ts_init(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	int err;

	err = stmpe811_reset(dev);
	if (err < 0) {
		return err;
	}

	/* Select TSC pins in TSC alternate mode */
	err = stmpe811_io_enable_af(dev, STMPE811_TOUCH_IO_ALL);
	if (err < 0) {
		return err;
	}

	/**
	 * Set the functionalities to be enabled
	 * Bits [0-3] disable functionalities if set to 1 (reset value: 0x0f)
	 *
	 * Apply inverted sum of chosen FCT bits as a mask to the currect register value
	 */
	err = i2c_reg_update_byte_dt(&config->bus, STMPE811_SYS_CTRL2_REG,
				     STMPE811_SYS_CTRL2_BIT_IO_FCT | STMPE811_SYS_CTRL2_BIT_TS_FCT |
				     STMPE811_SYS_CTRL2_BIT_ADC_FCT, 0);
	if (err < 0) {
		return err;
	}

	/* Select sample time, bit number and ADC reference */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_ADC_CTRL1_REG, STMPE811_ADC_CTRL1_CONF);
	if (err < 0) {
		return err;
	}

	/* Select the ADC clock speed */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_ADC_CTRL2_REG, STMPE811_ADC_CLOCK_SPEED);
	if (err < 0) {
		return err;
	}

	/* Touch screen configuration */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_TSC_CFG_REG,
				    stmpe811_tsc_config_bits(dev));
	if (err < 0) {
		return err;
	}

	/* Configure the touch FIFO threshold */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_FIFO_TH_REG,
				    STMPE811_FIFO_TH_SINGLE_POINT);
	if (err < 0) {
		return err;
	}

	/* Clear the FIFO memory content */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_FIFO_STA_REG, STMPE811_FIFO_STA_CLEAR);
	if (err < 0) {
		return err;
	}

	/* Set the range and accuracy of the pressure measurement (Z) */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_TSC_FRACT_XYZ_REG,
				    STMPE811_TSC_FRACT_XYZ_CONF);
	if (err < 0) {
		return err;
	}

	/* Set the driving capability (limit) of the device for TSC pins */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_TSC_I_DRIVE_REG,
				    STMPE811_TSC_I_DRIVE_LIMIT);
	if (err < 0) {
		return err;
	}

	/* Touch screen control configuration */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_TSC_CTRL_REG,
				    stmpe811_tsc_control_bits(dev));
	if (err < 0) {
		return err;
	}

	/**
	 * Clear all the status pending bits if any.
	 * Writing '1' to this register clears the corresponding bits.
	 * This is an 8-bit register, so writing 0xFF clears all.
	 */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_INT_STA_REG, STMPE811_INT_ALL);
	if (err < 0) {
		return err;
	}

	/* Put the FIFO back into operation mode */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_FIFO_STA_REG,
				    STMPE811_FIFO_STA_OPERATIONAL);
	if (err < 0) {
		return err;
	}

	/* Enable FIFO and touch interrupts */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_INT_EN_REG,
				    STMPE811_INT_BIT_TOUCH | STMPE811_INT_BIT_FIFO_THRESHOLD);
	if (err < 0) {
		LOG_ERR("Could not enable interrupt types (%d)", err);
		return err;
	}

	return 0;
}

static int stmpe811_ts_get_data(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	struct stmpe811_data *data = dev->data;

	uint8_t data_xy[3];
	uint32_t uldata_xy;

	int ret = i2c_burst_read_dt(&config->bus, STMPE811_TSC_DATA_NON_INC_REG, data_xy,
				    sizeof(data_xy));
	if (ret < 0) {
		return ret;
	}

	/* Calculate positions values */
	uldata_xy = (data_xy[0] << 16) | (data_xy[1] << 8) | data_xy[2];
	data->touch_x = (uldata_xy >> 12U) & BIT_MASK(12);
	data->touch_y = uldata_xy & BIT_MASK(12);

	return 0;
}

static void stmpe811_report_touch(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	struct stmpe811_data *data = dev->data;
	int x = data->touch_x;
	int y = data->touch_y;

	if (config->screen_width > 0 && config->screen_height > 0) {
		x = (((int)data->touch_x - config->raw_x_min) * config->screen_width) /
			(config->raw_x_max - config->raw_x_min);
		y = (((int)data->touch_y - config->raw_y_min) * config->screen_height) /
			(config->raw_y_max - config->raw_y_min);

		x = CLAMP(x, 0, config->screen_width);
		y = CLAMP(y, 0, config->screen_height);
	}

	input_report_abs(dev, INPUT_ABS_X, x, false, K_FOREVER);
	input_report_abs(dev, INPUT_ABS_Y, y, false, K_FOREVER);
	input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
}

static int stmpe811_process(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;

	int err;
	uint8_t int_sta, fifo_size, tsc_ctrl;

	err = i2c_reg_read_byte_dt(&config->bus, STMPE811_INT_STA_REG, &int_sta);
	if (err < 0) {
		return err;
	}

	/* Clear processed interrupts */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_INT_STA_REG, int_sta);
	if (err < 0) {
		return err;
	}

	if (int_sta & STMPE811_INT_BIT_FIFO_THRESHOLD) {
		/**
		 * Report every element in FIFO
		 *
		 * This requires a while loop to avoid a race condition
		 * in which an element is added after reading FIFO_SIZE.
		 *
		 * Exiting ISR without handling every element in FIFO
		 * would prevent FIFO_THRESHOLD interrupt from being triggered again.
		 */
		while (true) {
			err = i2c_reg_read_byte_dt(&config->bus, STMPE811_FIFO_SIZE_REG,
						   &fifo_size);
			if (err < 0) {
				return err;
			}

			if (fifo_size == 0) {
				break;
			}

			for (int i = 0; i < fifo_size; i++) {
				err = stmpe811_ts_get_data(dev);
				if (err < 0) {
					return err;
				}

				stmpe811_report_touch(dev);
			}
		}
	}

	/* TOUCH interrupt also gets triggered at release */
	if (int_sta & STMPE811_INT_BIT_TOUCH) {
		err = i2c_reg_read_byte_dt(&config->bus, STMPE811_TSC_CTRL_REG, &tsc_ctrl);
		if (err < 0) {
			return err;
		}

		/* TOUCH interrupt + no touch detected in TSC_CTRL reg <=> release */
		if (!(tsc_ctrl & STMPE811_TSC_CTRL_BIT_TOUCH_DET)) {
			input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
		}
	}

	return 0;
}

static void stmpe811_work_handler(struct k_work *work)
{
	struct stmpe811_data *data = CONTAINER_OF(work, struct stmpe811_data, processing_work);
	const struct stmpe811_config *config = data->dev->config;

	stmpe811_process(data->dev);
	/**
	 * Reschedule ISR if there was an interrupt triggered during handling (race condition).
	 * IRQ is edge-triggered, so otherwise it would never be triggered again.
	 */
	if (gpio_pin_get_dt(&config->int_gpio)) {
		k_work_submit(&data->processing_work);
	}
}

static void stmpe811_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct stmpe811_data *data = CONTAINER_OF(cb, struct stmpe811_data, int_gpio_cb);

	k_work_submit(&data->processing_work);
}

static int stmpe811_verify_chip_id(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	uint8_t buf[2];

	i2c_burst_read_dt(&config->bus, STMPE811_CHP_ID_LSB_REG, buf, 2);

	if (sys_get_be16(buf) != CHIP_ID) {
		return -EINVAL;
	}

	return 0;
}

static int stmpe811_init(const struct device *dev)
{
	const struct stmpe811_config *config = dev->config;
	struct stmpe811_data *data = dev->data;
	int err;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->processing_work, stmpe811_work_handler);

	/* Verify CHIP_ID */
	err = stmpe811_verify_chip_id(dev);
	if (err) {
		LOG_ERR("CHIP ID verification failed (%d)", err);
		return err;
	}

	/* Initialize */
	err = stmpe811_ts_init(dev);
	if (err) {
		LOG_ERR("Touch screen controller initialization failed (%d)", err);
		return err;
	}

	/* Initialize GPIO interrupt */
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin (%d)", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0) {
		LOG_ERR("Could not configure GPIO interrupt (%d)", err);
		return err;
	}

	gpio_init_callback(&data->int_gpio_cb, stmpe811_interrupt_handler,
			   BIT(config->int_gpio.pin));
	err = gpio_add_callback_dt(&config->int_gpio, &data->int_gpio_cb);
	if (err < 0) {
		LOG_ERR("Could not set GPIO callback (%d)", err);
		return err;
	}

	/* Enable global interrupts */
	err = i2c_reg_write_byte_dt(&config->bus, STMPE811_INT_CTRL_REG,
				    STMPE811_INT_CTRL_BIT_GLOBAL_INT);
	if (err < 0) {
		LOG_ERR("Could not enable global interrupts (%d)", err);
		return err;
	}

	return 0;
}

#define STMPE811_DEFINE(index)                                                                     \
	BUILD_ASSERT(DT_INST_PROP_OR(index, raw_x_max, 4096) >                                     \
		     DT_INST_PROP_OR(index, raw_x_min, 0),                                         \
		     "raw-x-max should be larger than raw-x-min");                                 \
	BUILD_ASSERT(DT_INST_PROP_OR(index, raw_y_max, 4096) >                                     \
		     DT_INST_PROP_OR(index, raw_y_min, 0),                                         \
		     "raw-y-max should be larger than raw-y-min");                                 \
	static const struct stmpe811_config stmpe811_config_##index = {                            \
		.bus = I2C_DT_SPEC_INST_GET(index),                                                \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),                               \
		.panel_driver_settling_time_us =                                                   \
			DT_INST_ENUM_IDX(index, panel_driver_settling_time_us),                    \
		.screen_width = DT_INST_PROP(index, screen_width),                                 \
		.screen_height = DT_INST_PROP(index, screen_height),                               \
		.raw_x_min = DT_INST_PROP_OR(index, raw_x_min, 0),                                 \
		.raw_y_min = DT_INST_PROP_OR(index, raw_y_min, 0),                                 \
		.raw_x_max = DT_INST_PROP_OR(index, raw_x_max, 4096),                              \
		.raw_y_max = DT_INST_PROP_OR(index, raw_y_max, 4096),                              \
		.touch_detect_delay_us = DT_INST_ENUM_IDX(index, touch_detect_delay_us),           \
		.touch_average_control = DT_INST_ENUM_IDX(index, touch_average_control),           \
		.tracking_index = DT_INST_ENUM_IDX(index, tracking_index)};                        \
	static struct stmpe811_data stmpe811_data_##index;                                         \
	DEVICE_DT_INST_DEFINE(index, stmpe811_init, NULL, &stmpe811_data_##index,                  \
			      &stmpe811_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(STMPE811_DEFINE)
