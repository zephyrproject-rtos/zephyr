/*
 * Copyright (c) 2020 NXP
 * Copyright (c) 2020 Mark Olsson <mark@markolsson.se>
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT goodix_gt911

#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gt911, CONFIG_KSCAN_LOG_LEVEL);

/* GT911 used registers */
#define DEVICE_ID           __bswap_16(0x8140U)
#define REG_STATUS		    __bswap_16(0x814EU)
#define REG_FIRST_POINT		__bswap_16(0x814FU)

/* REG_TD_STATUS: Touch points. */
#define TOUCH_POINTS_MSK	0x0FU

/* REG_TD_STATUS: Pressed. */
#define TOUCH_STATUS_MSK    (1 << 7U)

/* The GT911's config */
#define GT911_CONFIG_REG         __bswap_16(0x8047U)
#define REG_CONFIG_VERSION GT911_CONFIG_REG
#define REG_CONFIG_SIZE (186U)

/** GT911 configuration (DT). */
struct gt911_config {
	/** I2C bus. */
	struct i2c_dt_spec bus;
	struct gpio_dt_spec rst_gpio;
	/** Interrupt GPIO information. */
	struct gpio_dt_spec int_gpio;
};

/** GT911 data. */
struct gt911_data {
	/** Device pointer. */
	const struct device *dev;
	/** KSCAN Callback. */
	kscan_callback_t callback;
	/** Work queue (for deferred read). */
	struct k_work work;
#ifdef CONFIG_KSCAN_GT911_INTERRUPT
	/** Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#else
	/** Timer (polling mode). */
	struct k_timer timer;
#endif
};

/** gt911 point reg */
struct  gt911_point_reg_t {
	uint8_t id;       /*!< Track ID. */
	uint8_t lowX;     /*!< Low byte of x coordinate. */
	uint8_t highX;    /*!< High byte of x coordinate. */
	uint8_t lowY;     /*!< Low byte of y coordinate. */
	uint8_t highY;    /*!< High byte of x coordinate. */
	uint8_t lowSize;  /*!< Low byte of point size. */
	uint8_t highSize; /*!< High byte of point size. */
	uint8_t reserved; /*!< Reserved. */
};

static int gt911_process(const struct device *dev)
{
	const struct gt911_config *config = dev->config;
	struct gt911_data *data = dev->data;

	int r;
	uint16_t reg_addr;
	uint8_t status;
	uint8_t points;
	struct gt911_point_reg_t pointRegs;
	uint16_t row, col;
	bool pressed;

	/* obtain number of touch points (NOTE: multi-touch ignored) */
	reg_addr = REG_STATUS;
	r = i2c_write_read_dt(&config->bus, &reg_addr, sizeof(reg_addr),
							&status, sizeof(status));
	if (r < 0) {
		return r;
	}

	points = status & TOUCH_POINTS_MSK;
	if (points != 0U && points != 1U && (0 != (status & TOUCH_STATUS_MSK))) {
		return 0;
	}

	/* need to clear the status */
	uint8_t clear_buffer[3] = {(uint8_t)REG_STATUS, (uint8_t)(REG_STATUS >> 8), 0};

	r = i2c_write_dt(&config->bus, clear_buffer, sizeof(clear_buffer));
	if (r < 0) {
		return r;
	}

	/* obtain first point X, Y coordinates and event from:
	 * REG_P1_XH, REG_P1_XL, REG_P1_YH, REG_P1_YL.
	 */
	reg_addr = REG_FIRST_POINT;
	r = i2c_write_read_dt(&config->bus, &reg_addr, sizeof(reg_addr),
							&pointRegs, sizeof(pointRegs));
	if (r < 0) {
		return r;
	}

	pressed = (points == 1);
	row = ((pointRegs.highX) << 8U) | pointRegs.lowX;
	col = ((pointRegs.highY) << 8U) | pointRegs.lowY;

	LOG_DBG("pressed: %d, row: %d, col: %d", pressed, row, col);

	data->callback(dev, row, col, pressed);

	return 0;
}

static void gt911_work_handler(struct k_work *work)
{
	struct gt911_data *data = CONTAINER_OF(work, struct gt911_data, work);

	gt911_process(data->dev);
}

#ifdef CONFIG_KSCAN_GT911_INTERRUPT
static void gt911_isr_handler(const struct device *dev,
			       struct gpio_callback *cb, uint32_t pins)
{
	struct gt911_data *data = CONTAINER_OF(cb, struct gt911_data, int_gpio_cb);

	k_work_submit(&data->work);
}
#else
static void gt911_timer_handler(struct k_timer *timer)
{
	struct gt911_data *data = CONTAINER_OF(timer, struct gt911_data, timer);

	k_work_submit(&data->work);
}
#endif

static int gt911_configure(const struct device *dev,
			    kscan_callback_t callback)
{
	struct gt911_data *data = dev->data;

	if (!callback) {
		LOG_ERR("Invalid callback (NULL)");
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int gt911_enable_callback(const struct device *dev)
{
	struct gt911_data *data = dev->data;

#ifdef CONFIG_KSCAN_GT911_INTERRUPT
	const struct gt911_config *config = dev->config;

	gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
#else
	k_timer_start(&data->timer, K_MSEC(CONFIG_KSCAN_GT911_PERIOD),
		      K_MSEC(CONFIG_KSCAN_GT911_PERIOD));
#endif

	return 0;
}

static int gt911_disable_callback(const struct device *dev)
{
	struct gt911_data *data = dev->data;

#ifdef CONFIG_KSCAN_GT911_INTERRUPT
	const struct gt911_config *config = dev->config;

	gpio_remove_callback(config->int_gpio.port, &data->int_gpio_cb);
#else
	k_timer_stop(&data->timer);
#endif

	return 0;
}

static uint8_t gt911_get_firmware_checksum(const uint8_t *firmware)
{
	uint8_t sum = 0;
	uint16_t i  = 0;

	for (i = 0; i < REG_CONFIG_SIZE - 2U; i++) {
		sum += (*firmware);
		firmware++;
	}

	return (~sum + 1U);
}

static bool gt911_verify_firmware(const uint8_t *firmware)
{
	return ((firmware[REG_CONFIG_VERSION - GT911_CONFIG_REG] != 0U) &&
		(gt911_get_firmware_checksum(firmware) == firmware[REG_CONFIG_SIZE - 2U]));
}

static int gt911_init(const struct device *dev)
{
	const struct gt911_config *config = dev->config;
	struct gt911_data *data = dev->data;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->work, gt911_work_handler);

	int r;

	r = gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_INACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure reset GPIO pin");
		return r;
	}

	/* we need to configure the int-pin to 0, in order toenter the AddressModel0 */
	r = gpio_pin_configure_dt(&config->int_gpio, GPIO_OUTPUT_INACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure int GPIO pin");
		return r;
	}

	/* reset the device and confgiure the addr mode0 */
	gpio_pin_set_dt(&config->rst_gpio, 0);
	/* hold down at least 1us, 1ms here */
	k_sleep(K_MSEC(1));
	gpio_pin_set_dt(&config->rst_gpio, 1);
	/* hold down at least 5ms, before set the int pin low */
	k_sleep(K_MSEC(5));
	gpio_pin_set_dt(&config->int_gpio, 0);
	/* hold down 50ms to make sure the address available */
	k_sleep(K_MSEC(50));

#ifdef CONFIG_KSCAN_GT911_INTERRUPT
	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	r = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return r;
	}

	r = gpio_pin_interrupt_configure_dt(&config->int_gpio,
					    GPIO_INT_EDGE_TO_ACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt.");
		return r;
	}

	gpio_init_callback(&data->int_gpio_cb, gt911_isr_handler,
			   BIT(config->int_gpio.pin));
#else
	k_timer_init(&data->timer, gt911_timer_handler, NULL);
#endif

	/* check the Device ID first: '911' */
	uint32_t reg_id = 0;
	uint16_t reg_addr = DEVICE_ID;

	r = i2c_write_read_dt(&config->bus, &reg_addr, sizeof(reg_addr),
							&reg_id, sizeof(reg_id));
	if (r < 0) {
		return r;
	}
	if (reg_id != 0x00313139U) {
		LOG_ERR("The Devide ID is not correct");
		return -ENODEV;
	}

	/* need to setup the firmware first: read and write */
	uint8_t gt911Config[REG_CONFIG_SIZE + 2] = {
		(uint8_t)GT911_CONFIG_REG, (uint8_t)(GT911_CONFIG_REG >> 8)
	};

	reg_addr = GT911_CONFIG_REG;
	r = i2c_write_read_dt(&config->bus, &reg_addr, sizeof(reg_addr),
							gt911Config + 2, REG_CONFIG_SIZE);
	if (r < 0) {
		return r;
	}
	if (!gt911_verify_firmware(gt911Config + 2)) {
		return -ENODEV;
	}

	gt911Config[REG_CONFIG_SIZE] = gt911_get_firmware_checksum(gt911Config + 2);
	gt911Config[REG_CONFIG_SIZE + 1] = 1;

	r = i2c_write_dt(&config->bus, gt911Config, sizeof(gt911Config));
	if (r < 0) {
		return r;
	}

	return 0;
}

static const struct kscan_driver_api gt911_driver_api = {
	.config = gt911_configure,
	.enable_callback = gt911_enable_callback,
	.disable_callback = gt911_disable_callback,
};

#define GT911_INIT(index)                                                     \
	static const struct gt911_config gt911_config_##index = {	       \
		.bus = I2C_DT_SPEC_INST_GET(index),			       \
		.rst_gpio = GPIO_DT_SPEC_INST_GET(index, reset_gpios), \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios)	       \
	};								       \
	static struct gt911_data gt911_data_##index;			       \
	DEVICE_DT_INST_DEFINE(index, gt911_init, NULL,			       \
			    &gt911_data_##index, &gt911_config_##index,      \
			    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,	       \
			    &gt911_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GT911_INIT)
