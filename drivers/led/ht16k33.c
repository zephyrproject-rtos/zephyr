/*
 * Copyright (c) 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT holtek_ht16k33

/**
 * @file
 * @brief LED driver for the HT16K33 I2C LED driver with keyscan
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/ht16k33.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ht16k33, CONFIG_LED_LOG_LEVEL);

#include "led_context.h"

/* HT16K33 commands and options */
#define HT16K33_CMD_DISP_DATA_ADDR 0x00

#define HT16K33_CMD_SYSTEM_SETUP   0x20
#define HT16K33_OPT_S              BIT(0)

#define HT16K33_CMD_KEY_DATA_ADDR  0x40

#define HT16K33_CMD_INT_FLAG_ADDR  0x60

#define HT16K33_CMD_DISP_SETUP     0x80
#define HT16K33_OPT_D              BIT(0)
#define HT16K33_OPT_B0             BIT(1)
#define HT16K33_OPT_B1             BIT(2)
#define HT16K33_OPT_BLINK_OFF      0
#define HT16K33_OPT_BLINK_2HZ      HT16K33_OPT_B0
#define HT16K33_OPT_BLINK_1HZ      HT16K33_OPT_B1
#define HT16K33_OPT_BLINK_05HZ     (HT16K33_OPT_B1 | HT16K33_OPT_B0)

#define HT16K33_CMD_ROW_INT_SET    0xa0
#define HT16K33_OPT_ROW_INT        BIT(0)
#define HT16K33_OPT_ACT            BIT(1)
#define HT16K33_OPT_ROW            0
#define HT16K33_OPT_INT_LOW        HT16K33_OPT_ROW_INT
#define HT16K33_OPT_INT_HIGH       (HT16K33_OPT_ACT | HT16K33_OPT_ROW_INT)

#define HT16K33_CMD_DIMMING_SET    0xe0

/* HT16K33 size definitions */
#define HT16K33_DISP_ROWS          16
#define HT16K33_DISP_COLS          8
#define HT16K33_DISP_DATA_SIZE     HT16K33_DISP_ROWS
#define HT16K33_DISP_SEGMENTS      (HT16K33_DISP_ROWS * HT16K33_DISP_COLS)
#define HT16K33_DIMMING_LEVELS     16
#define HT16K33_KEYSCAN_ROWS       3
#define HT16K33_KEYSCAN_COLS       13
#define HT16K33_KEYSCAN_DATA_SIZE  6

struct ht16k33_cfg {
	const struct device *i2c_dev;
	uint16_t i2c_addr;
	bool irq_enabled;
#ifdef CONFIG_HT16K33_KEYSCAN
	struct gpio_dt_spec irq;
#endif /* CONFIG_HT16K33_KEYSCAN */
};

struct ht16k33_data {
	const struct device *dev;
	struct led_data dev_data;
	 /* Shadow buffer for the display data RAM */
	uint8_t buffer[HT16K33_DISP_DATA_SIZE];
#ifdef CONFIG_HT16K33_KEYSCAN
	struct k_mutex lock;
	const struct device *child;
	kscan_callback_t kscan_cb;
	struct gpio_callback irq_cb;
	struct k_thread irq_thread;
	struct k_sem irq_sem;
	struct k_timer timer;
	uint16_t key_state[HT16K33_KEYSCAN_ROWS];

	K_KERNEL_STACK_MEMBER(irq_thread_stack,
			      CONFIG_HT16K33_KEYSCAN_IRQ_THREAD_STACK_SIZE);
#endif /* CONFIG_HT16K33_KEYSCAN */
};

static int ht16k33_led_blink(const struct device *dev, uint32_t led,
			     uint32_t delay_on, uint32_t delay_off)
{
	/* The HT16K33 blinks all LEDs at the same frequency */
	ARG_UNUSED(led);

	const struct ht16k33_cfg *config = dev->config;
	struct ht16k33_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint32_t period;
	uint8_t cmd;

	period = delay_on + delay_off;
	if (period < dev_data->min_period || period > dev_data->max_period) {
		return -EINVAL;
	}

	cmd = HT16K33_CMD_DISP_SETUP | HT16K33_OPT_D;
	if (delay_off == 0) {
		cmd |= HT16K33_OPT_BLINK_OFF;
	} else if (period > 1500)  {
		cmd |= HT16K33_OPT_BLINK_05HZ;
	} else if (period > 750)  {
		cmd |= HT16K33_OPT_BLINK_1HZ;
	} else {
		cmd |= HT16K33_OPT_BLINK_2HZ;
	}

	if (i2c_write(config->i2c_dev, &cmd, sizeof(cmd), config->i2c_addr)) {
		LOG_ERR("Setting HT16K33 blink frequency failed");
		return -EIO;
	}

	return 0;
}

static int ht16k33_led_set_brightness(const struct device *dev, uint32_t led,
				      uint8_t value)
{
	ARG_UNUSED(led);

	const struct ht16k33_cfg *config = dev->config;
	struct ht16k33_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t dim;
	uint8_t cmd;

	if (value < dev_data->min_brightness ||
	    value > dev_data->max_brightness) {
		return -EINVAL;
	}

	dim = (value * (HT16K33_DIMMING_LEVELS - 1)) / dev_data->max_brightness;
	cmd = HT16K33_CMD_DIMMING_SET | dim;

	if (i2c_write(config->i2c_dev, &cmd, sizeof(cmd), config->i2c_addr)) {
		LOG_ERR("Setting HT16K33 brightness failed");
		return -EIO;
	}

	return 0;
}

static int ht16k33_led_set_state(const struct device *dev, uint32_t led,
				 bool on)
{
	const struct ht16k33_cfg *config = dev->config;
	struct ht16k33_data *data = dev->data;
	uint8_t cmd[2];
	uint8_t addr;
	uint8_t bit;

	if (led >= HT16K33_DISP_SEGMENTS) {
		return -EINVAL;
	}

	addr = led / HT16K33_DISP_COLS;
	bit = led % HT16K33_DISP_COLS;

	cmd[0] = HT16K33_CMD_DISP_DATA_ADDR | addr;
	if (on) {
		cmd[1] = data->buffer[addr] | BIT(bit);
	} else {
		cmd[1] = data->buffer[addr] & ~BIT(bit);
	}

	if (data->buffer[addr] == cmd[1]) {
		return 0;
	}

	if (i2c_write(config->i2c_dev, cmd, sizeof(cmd), config->i2c_addr)) {
		LOG_ERR("Setting HT16K33 LED %s failed", on ? "on" : "off");
		return -EIO;
	}

	data->buffer[addr] = cmd[1];

	return 0;
}

static int ht16k33_led_on(const struct device *dev, uint32_t led)
{
	return ht16k33_led_set_state(dev, led, true);
}

static int ht16k33_led_off(const struct device *dev, uint32_t led)
{
	return ht16k33_led_set_state(dev, led, false);
}

#ifdef CONFIG_HT16K33_KEYSCAN
static bool ht16k33_process_keyscan_data(const struct device *dev)
{
	const struct ht16k33_cfg *config = dev->config;
	struct ht16k33_data *data = dev->data;
	uint8_t keys[HT16K33_KEYSCAN_DATA_SIZE];
	bool pressed = false;
	uint16_t state;
	uint16_t changed;
	int row;
	int col;
	int err;

	err = i2c_burst_read(config->i2c_dev, config->i2c_addr,
			     HT16K33_CMD_KEY_DATA_ADDR, keys,
			     sizeof(keys));
	if (err) {
		LOG_WRN("Failed to to read HT16K33 key data (err %d)", err);
		/* Reprocess */
		return true;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (row = 0; row < HT16K33_KEYSCAN_ROWS; row++) {
		state = sys_get_le16(&keys[row * 2]);
		changed = data->key_state[row] ^ state;
		data->key_state[row] = state;

		if (state) {
			pressed = true;
		}

		if (data->kscan_cb == NULL) {
			continue;
		}

		for (col = 0; col < HT16K33_KEYSCAN_COLS; col++) {
			if (changed & BIT(col)) {
				data->kscan_cb(data->child, row, col,
					state & BIT(col));
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return pressed;
}

static void ht16k33_irq_thread(struct ht16k33_data *data)
{
	bool pressed;

	while (true) {
		k_sem_take(&data->irq_sem, K_FOREVER);

		do {
			k_sem_reset(&data->irq_sem);
			pressed = ht16k33_process_keyscan_data(data->dev);
			k_msleep(CONFIG_HT16K33_KEYSCAN_DEBOUNCE_MSEC);
		} while (pressed);
	}
}

static void ht16k33_irq_callback(const struct device *gpiob,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct ht16k33_data *data;

	ARG_UNUSED(gpiob);
	ARG_UNUSED(pins);

	data = CONTAINER_OF(cb, struct ht16k33_data, irq_cb);
	k_sem_give(&data->irq_sem);
}

static void ht16k33_timer_callback(struct k_timer *timer)
{
	struct ht16k33_data *data;

	data = CONTAINER_OF(timer, struct ht16k33_data, timer);
	k_sem_give(&data->irq_sem);
}

int ht16k33_register_keyscan_callback(const struct device *parent,
				      const struct device *child,
				      kscan_callback_t callback)
{
	struct ht16k33_data *data = parent->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->child = child;
	data->kscan_cb = callback;
	k_mutex_unlock(&data->lock);

	return 0;
}
#endif /* CONFIG_HT16K33_KEYSCAN */

static int ht16k33_init(const struct device *dev)
{
	const struct ht16k33_cfg *config = dev->config;
	struct ht16k33_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t cmd[1 + HT16K33_DISP_DATA_SIZE]; /* 1 byte command + data */
	int err;

	data->dev = dev;

	if (!device_is_ready(config->i2c_dev)) {
		LOG_ERR("I2C bus device not ready");
		return -EINVAL;
	}

	memset(&data->buffer, 0, sizeof(data->buffer));

	/* Hardware specific limits */
	dev_data->min_period = 0U;
	dev_data->max_period = 2000U;
	dev_data->min_brightness = 0U;
	dev_data->max_brightness = 100U;

	/* System oscillator on */
	cmd[0] = HT16K33_CMD_SYSTEM_SETUP | HT16K33_OPT_S;
	err = i2c_write(config->i2c_dev, cmd, 1, config->i2c_addr);
	if (err) {
		LOG_ERR("Enabling HT16K33 system oscillator failed (err %d)",
			err);
		return -EIO;
	}

	/* Clear display RAM */
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = HT16K33_CMD_DISP_DATA_ADDR;
	err = i2c_write(config->i2c_dev, cmd, sizeof(cmd), config->i2c_addr);
	if (err) {
		LOG_ERR("Clearing HT16K33 display RAM failed (err %d)", err);
		return -EIO;
	}

	/* Full brightness */
	cmd[0] = HT16K33_CMD_DIMMING_SET | 0x0f;
	err = i2c_write(config->i2c_dev, cmd, 1, config->i2c_addr);
	if (err) {
		LOG_ERR("Setting HT16K33 brightness failed (err %d)", err);
		return -EIO;
	}

	/* Display on, blinking off */
	cmd[0] = HT16K33_CMD_DISP_SETUP | HT16K33_OPT_D | HT16K33_OPT_BLINK_OFF;
	err = i2c_write(config->i2c_dev, cmd, 1, config->i2c_addr);
	if (err) {
		LOG_ERR("Enabling HT16K33 display failed (err %d)", err);
		return -EIO;
	}

#ifdef CONFIG_HT16K33_KEYSCAN
	k_mutex_init(&data->lock);
	k_sem_init(&data->irq_sem, 0, 1);

	/* Configure interrupt */
	if (config->irq_enabled) {
		uint8_t keys[HT16K33_KEYSCAN_DATA_SIZE];

		if (!device_is_ready(config->irq.port)) {
			LOG_ERR("IRQ device not ready");
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->irq, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure IRQ pin (err %d)", err);
			return -EINVAL;
		}

		gpio_init_callback(&data->irq_cb, &ht16k33_irq_callback,
				   BIT(config->irq.pin));

		err = gpio_add_callback(config->irq.port, &data->irq_cb);
		if (err) {
			LOG_ERR("Failed to add IRQ callback (err %d)", err);
			return -EINVAL;
		}

		/* Enable interrupt pin */
		cmd[0] = HT16K33_CMD_ROW_INT_SET | HT16K33_OPT_INT_LOW;
		if (i2c_write(config->i2c_dev, cmd, 1, config->i2c_addr)) {
			LOG_ERR("Enabling HT16K33 IRQ output failed");
			return -EIO;
		}

		/* Flush key data before enabling interrupt */
		err = i2c_burst_read(config->i2c_dev, config->i2c_addr,
				HT16K33_CMD_KEY_DATA_ADDR, keys, sizeof(keys));
		if (err) {
			LOG_ERR("Failed to to read HT16K33 key data");
			return -EIO;
		}

		err = gpio_pin_interrupt_configure_dt(&config->irq,
						      GPIO_INT_EDGE_FALLING);
		if (err) {
			LOG_ERR("Failed to configure IRQ pin flags (err %d)",
				err);
			return -EINVAL;
		}
	} else {
		/* No interrupt pin, enable ROW15 */
		cmd[0] = HT16K33_CMD_ROW_INT_SET | HT16K33_OPT_ROW;
		if (i2c_write(config->i2c_dev, cmd, 1, config->i2c_addr)) {
			LOG_ERR("Enabling HT16K33 ROW15 output failed");
			return -EIO;
		}

		/* Setup timer for polling key data */
		k_timer_init(&data->timer, ht16k33_timer_callback, NULL);
		k_timer_start(&data->timer, K_NO_WAIT,
			      K_MSEC(CONFIG_HT16K33_KEYSCAN_POLL_MSEC));
	}

	k_thread_create(&data->irq_thread, data->irq_thread_stack,
			CONFIG_HT16K33_KEYSCAN_IRQ_THREAD_STACK_SIZE,
			(k_thread_entry_t)ht16k33_irq_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_HT16K33_KEYSCAN_IRQ_THREAD_PRIO),
			0, K_NO_WAIT);
#endif /* CONFIG_HT16K33_KEYSCAN */

	return 0;
}

static const struct led_driver_api ht16k33_leds_api = {
	.blink = ht16k33_led_blink,
	.set_brightness = ht16k33_led_set_brightness,
	.on = ht16k33_led_on,
	.off = ht16k33_led_off,
};

#define HT16K33_DEVICE(id)						\
	static const struct ht16k33_cfg ht16k33_##id##_cfg = {		\
		.i2c_dev      = DEVICE_DT_GET(DT_INST_BUS(id)),		\
		.i2c_addr     = DT_INST_REG_ADDR(id),			\
		.irq_enabled  = false,					\
	};								\
									\
	static struct ht16k33_data ht16k33_##id##_data;			\
									\
	DEVICE_DT_INST_DEFINE(id, &ht16k33_init, NULL,			\
			    &ht16k33_##id##_data,			\
			    &ht16k33_##id##_cfg, POST_KERNEL,		\
			    CONFIG_LED_INIT_PRIORITY, &ht16k33_leds_api)

#ifdef CONFIG_HT16K33_KEYSCAN
#define HT16K33_DEVICE_WITH_IRQ(id)					\
	static const struct ht16k33_cfg ht16k33_##id##_cfg = {		\
		.i2c_dev      = DEVICE_DT_GET(DT_INST_BUS(id)),		\
		.i2c_addr     = DT_INST_REG_ADDR(id),			\
		.irq_enabled  = true,					\
		.irq          =	GPIO_DT_SPEC_INST_GET(id, irq_gpios),	\
	};								\
									\
	static struct ht16k33_data ht16k33_##id##_data;			\
									\
	DEVICE_DT_INST_DEFINE(id, &ht16k33_init, NULL,			\
			    &ht16k33_##id##_data,			\
			    &ht16k33_##id##_cfg, POST_KERNEL,		\
			    CONFIG_LED_INIT_PRIORITY, &ht16k33_leds_api)
#else /* ! CONFIG_HT16K33_KEYSCAN */
#define HT16K33_DEVICE_WITH_IRQ(id) HT16K33_DEVICE(id)
#endif /* ! CONFIG_HT16K33_KEYSCAN */

#define HT16K33_INSTANTIATE(id)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(id, irq_gpios),	\
		    (HT16K33_DEVICE_WITH_IRQ(id)),		\
		    (HT16K33_DEVICE(id)));

DT_INST_FOREACH_STATUS_OKAY(HT16K33_INSTANTIATE)
