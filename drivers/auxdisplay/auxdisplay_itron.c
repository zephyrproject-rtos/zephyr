/*
 * Copyright (c) 2022-2023 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT noritake_itron

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "auxdisplay_itron.h"

LOG_MODULE_REGISTER(auxdisplay_itron, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* Display commands */
#define AUXDISPLAY_ITRON_CMD_USER_SETTING 0x1f
#define AUXDISPLAY_ITRON_CMD_ESCAPE 0x1b
#define AUXDISPLAY_ITRON_CMD_BRIGHTNESS 0x58
#define AUXDISPLAY_ITRON_CMD_DISPLAY_CLEAR 0x0c
#define AUXDISPLAY_ITRON_CMD_CURSOR 0x43
#define AUXDISPLAY_ITRON_CMD_CURSOR_SET 0x24
#define AUXDISPLAY_ITRON_CMD_ACTION 0x28
#define AUXDISPLAY_ITRON_CMD_N 0x61
#define AUXDISPLAY_ITRON_CMD_SCREEN_SAVER 0x40

/* Time values when multithreading is disabled */
#define AUXDISPLAY_ITRON_RESET_TIME K_MSEC(2)
#define AUXDISPLAY_ITRON_RESET_WAIT_TIME K_MSEC(101)
#define AUXDISPLAY_ITRON_BUSY_DELAY_TIME_CHECK K_MSEC(4)
#define AUXDISPLAY_ITRON_BUSY_WAIT_LOOPS 125

/* Time values when multithreading is enabled */
#define AUXDISPLAY_ITRON_BUSY_MAX_TIME K_MSEC(500)

struct auxdisplay_itron_data {
	uint16_t character_x;
	uint16_t character_y;
	uint8_t brightness;
	bool powered;
#ifdef CONFIG_MULTITHREADING
	struct k_sem lock_sem;
	struct k_sem busy_wait_sem;
	struct gpio_callback busy_wait_callback;
#endif
};

struct auxdisplay_itron_config {
	const struct device *uart;
	struct auxdisplay_capabilities capabilities;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec busy_gpio;
};

static int send_cmd(const struct device *dev, const uint8_t *command, uint8_t length, bool pm,
		    bool lock);
static int auxdisplay_itron_is_busy(const struct device *dev);
static int auxdisplay_itron_clear(const struct device *dev);
static int auxdisplay_itron_set_powered(const struct device *dev, bool enabled);

#ifdef CONFIG_MULTITHREADING
void auxdisplay_itron_busy_gpio_change_callback(const struct device *port,
						struct gpio_callback *cb,
						gpio_port_pins_t pins)
{
	struct auxdisplay_itron_data *data = CONTAINER_OF(cb,
			struct auxdisplay_itron_data, busy_wait_callback);
	k_sem_give(&data->busy_wait_sem);
}
#endif

static int auxdisplay_itron_init(const struct device *dev)
{
	const struct auxdisplay_itron_config *config = dev->config;
	struct auxdisplay_itron_data *data = dev->data;
	int rc;

	if (!device_is_ready(config->uart)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	/* Configure and set busy GPIO */
	if (config->busy_gpio.port) {
		rc = gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);

		if (rc < 0) {
			LOG_ERR("Configuration of text display busy GPIO failed: %d", rc);
			return rc;
		}

#ifdef CONFIG_MULTITHREADING
		k_sem_init(&data->lock_sem, 1, 1);
		k_sem_init(&data->busy_wait_sem, 0, 1);

		gpio_init_callback(&data->busy_wait_callback,
				   auxdisplay_itron_busy_gpio_change_callback,
				   BIT(config->busy_gpio.pin));
		rc = gpio_add_callback(config->busy_gpio.port, &data->busy_wait_callback);

		if (rc != 0) {
			LOG_ERR("Configuration of busy interrupt failed: %d", rc);
			return rc;
		}
#endif
	}

	/* Configure and set reset GPIO */
	if (config->reset_gpio.port) {
		rc = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("Configuration of text display reset GPIO failed");
			return rc;
		}
	}

	data->character_x = 0;
	data->character_y = 0;
	data->brightness = 0;

	/* Reset display to known configuration */
	if (config->reset_gpio.port) {
		uint8_t wait_loops = 0;

		gpio_pin_set_dt(&config->reset_gpio, 1);
		k_sleep(AUXDISPLAY_ITRON_RESET_TIME);
		gpio_pin_set_dt(&config->reset_gpio, 0);
		k_sleep(AUXDISPLAY_ITRON_RESET_WAIT_TIME);

		while (auxdisplay_itron_is_busy(dev) == 1) {
			/* Display is busy, wait */
			k_sleep(AUXDISPLAY_ITRON_BUSY_DELAY_TIME_CHECK);
			++wait_loops;

			if (wait_loops >= AUXDISPLAY_ITRON_BUSY_WAIT_LOOPS) {
				/* Waited long enough for display not to be busy, bailing */
				return -EIO;
			}
		}
	} else {
		/* Ensure display is powered on so that it can be initialised */
		(void)auxdisplay_itron_set_powered(dev, true);
		auxdisplay_itron_clear(dev);
	}

	return 0;
}

static int auxdisplay_itron_set_powered(const struct device *dev, bool enabled)
{
	uint8_t cmd[] = {AUXDISPLAY_ITRON_CMD_USER_SETTING, AUXDISPLAY_ITRON_CMD_ACTION,
			 AUXDISPLAY_ITRON_CMD_N, AUXDISPLAY_ITRON_CMD_SCREEN_SAVER, 0};

	if (enabled) {
		cmd[4] = 1;
	}

	return send_cmd(dev, cmd, sizeof(cmd), true, true);
}

static bool auxdisplay_itron_is_powered(const struct device *dev)
{
	struct auxdisplay_itron_data *data = dev->data;
	bool is_powered;

#ifdef CONFIG_MULTITHREADING
	k_sem_take(&data->lock_sem, K_FOREVER);
#endif

	is_powered = data->powered;

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&data->lock_sem);
#endif

	return is_powered;
}

static int auxdisplay_itron_display_on(const struct device *dev)
{
	return auxdisplay_itron_set_powered(dev, true);
}

static int auxdisplay_itron_display_off(const struct device *dev)
{
	return auxdisplay_itron_set_powered(dev, false);
}

static int auxdisplay_itron_cursor_set_enabled(const struct device *dev, bool enabled)
{
	uint8_t cmd[] = {AUXDISPLAY_ITRON_CMD_USER_SETTING, AUXDISPLAY_ITRON_CMD_CURSOR,
			 (uint8_t)enabled};

	return send_cmd(dev, cmd, sizeof(cmd), false, true);
}

static int auxdisplay_itron_cursor_position_set(const struct device *dev,
					      enum auxdisplay_position type,
					      int16_t x, int16_t y)
{
	uint8_t cmd[] = {AUXDISPLAY_ITRON_CMD_USER_SETTING, AUXDISPLAY_ITRON_CMD_CURSOR_SET,
			 0, 0, 0, 0};

	if (type != AUXDISPLAY_POSITION_ABSOLUTE) {
		return -EINVAL;
	}

	sys_put_le16(x, &cmd[2]);
	sys_put_le16(y, &cmd[4]);

	return send_cmd(dev, cmd, sizeof(cmd), false, true);
}

static int auxdisplay_itron_capabilities_get(const struct device *dev,
					     struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_itron_config *config = dev->config;

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_itron_clear(const struct device *dev)
{
	uint8_t cmd[] = {AUXDISPLAY_ITRON_CMD_DISPLAY_CLEAR};

	return send_cmd(dev, cmd, sizeof(cmd), false, true);
}

static int auxdisplay_itron_brightness_get(const struct device *dev, uint8_t *brightness)
{
	struct auxdisplay_itron_data *data = dev->data;

#ifdef CONFIG_MULTITHREADING
	k_sem_take(&data->lock_sem, K_FOREVER);
#endif

	*brightness = data->brightness;

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&data->lock_sem);
#endif

	return 0;
}

static int auxdisplay_itron_brightness_set(const struct device *dev, uint8_t brightness)
{
	struct auxdisplay_itron_data *data = dev->data;
	uint8_t cmd[] = {AUXDISPLAY_ITRON_CMD_USER_SETTING, AUXDISPLAY_ITRON_CMD_BRIGHTNESS,
			 brightness};
	int rc;

	if (brightness < AUXDISPLAY_ITRON_BRIGHTNESS_MIN ||
	    brightness > AUXDISPLAY_ITRON_BRIGHTNESS_MAX) {
		return -EINVAL;
	}

#ifdef CONFIG_MULTITHREADING
	k_sem_take(&data->lock_sem, K_FOREVER);
#endif

	rc = send_cmd(dev, cmd, sizeof(cmd), false, false);

	if (rc == 0) {
		data->brightness = brightness;
	}

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&data->lock_sem);
#endif

	return rc;
}

static int auxdisplay_itron_is_busy(const struct device *dev)
{
	const struct auxdisplay_itron_config *config = dev->config;
	int rc;

	if (config->busy_gpio.port == NULL) {
		return -ENOTSUP;
	}

	rc = gpio_pin_get_dt(&config->busy_gpio);

	return rc;
}

static int auxdisplay_itron_is_busy_check(const struct device *dev)
{
	struct auxdisplay_itron_data *data = dev->data;
	int rc;

#ifdef CONFIG_MULTITHREADING
	k_sem_take(&data->lock_sem, K_FOREVER);
#endif

	rc = auxdisplay_itron_is_busy(dev);

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&data->lock_sem);
#endif

	return rc;
}

static int send_cmd(const struct device *dev, const uint8_t *command, uint8_t length, bool pm,
		    bool lock)
{
	uint8_t i = 0;
	const struct auxdisplay_itron_config *config = dev->config;
	const struct device *uart = config->uart;
	int rc = 0;
#ifdef CONFIG_MULTITHREADING
	struct auxdisplay_itron_data *data = dev->data;
#endif

	if (pm == false && auxdisplay_itron_is_powered(dev) == false) {
		/* Display is not powered, only PM commands can be used */
		return -ESHUTDOWN;
	}

#ifdef CONFIG_MULTITHREADING
	if (lock) {
		k_sem_take(&data->lock_sem, K_FOREVER);
	}
#endif

#ifdef CONFIG_MULTITHREADING
	/* Enable interrupt triggering */
	rc = gpio_pin_interrupt_configure_dt(&config->busy_gpio, GPIO_INT_EDGE_TO_INACTIVE);

	if (rc != 0) {
		LOG_ERR("Failed to enable busy interrupt: %d", rc);
		goto end;
	}
#endif

	while (i < length) {
#ifdef CONFIG_MULTITHREADING
		if (auxdisplay_itron_is_busy(dev) == 1) {
			if (k_sem_take(&data->busy_wait_sem,
				       AUXDISPLAY_ITRON_BUSY_MAX_TIME) != 0) {
				rc = -EIO;
				goto cleanup;
			}
		}
#else
		uint8_t wait_loops = 0;

		while (auxdisplay_itron_is_busy(dev) == 1) {
			/* Display is busy, wait */
			k_sleep(AUXDISPLAY_ITRON_BUSY_DELAY_TIME_CHECK);
			++wait_loops;

			if (wait_loops >= AUXDISPLAY_ITRON_BUSY_WAIT_LOOPS) {
				/* Waited long enough for display not to be busy, bailing */
				return -EIO;
			}
		}
#endif

		uart_poll_out(uart, command[i]);
		++i;
	}

#ifdef CONFIG_MULTITHREADING
cleanup:
	(void)gpio_pin_interrupt_configure_dt(&config->busy_gpio, GPIO_INT_DISABLE);
#endif

end:
#ifdef CONFIG_MULTITHREADING
	if (lock) {
		k_sem_give(&data->lock_sem);
	}
#endif

	return rc;
}

static int auxdisplay_itron_write(const struct device *dev, const uint8_t *data, uint16_t len)
{
	uint16_t i = 0;

	/* Check all characters are valid */
	while (i < len) {
		if (data[i] < AUXDISPLAY_ITRON_CHARACTER_MIN &&
		    data[i] != AUXDISPLAY_ITRON_CHARACTER_BACK_SPACE &&
		    data[i] != AUXDISPLAY_ITRON_CHARACTER_TAB &&
		    data[i] != AUXDISPLAY_ITRON_CHARACTER_LINE_FEED &&
		    data[i] != AUXDISPLAY_ITRON_CHARACTER_CARRIAGE_RETURN) {
			return -EINVAL;
		}

		++i;
	}

	return send_cmd(dev, data, len, false, true);
}

static DEVICE_API(auxdisplay, auxdisplay_itron_auxdisplay_api) = {
	.display_on = auxdisplay_itron_display_on,
	.display_off = auxdisplay_itron_display_off,
	.cursor_set_enabled = auxdisplay_itron_cursor_set_enabled,
	.cursor_position_set = auxdisplay_itron_cursor_position_set,
	.capabilities_get = auxdisplay_itron_capabilities_get,
	.clear = auxdisplay_itron_clear,
	.brightness_get = auxdisplay_itron_brightness_get,
	.brightness_set = auxdisplay_itron_brightness_set,
	.is_busy = auxdisplay_itron_is_busy_check,
	.write = auxdisplay_itron_write,
};

#define AUXDISPLAY_ITRON_DEVICE(inst)								\
	static struct auxdisplay_itron_data auxdisplay_itron_data_##inst;			\
	static const struct auxdisplay_itron_config auxdisplay_itron_config_##inst = {		\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.capabilities = {								\
			.columns = DT_INST_PROP(inst, columns),					\
			.rows = DT_INST_PROP(inst, rows),					\
			.mode = AUXDISPLAY_ITRON_MODE_UART,					\
			.brightness.minimum = AUXDISPLAY_ITRON_BRIGHTNESS_MIN,			\
			.brightness.maximum = AUXDISPLAY_ITRON_BRIGHTNESS_MAX,			\
			.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,			\
			.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,			\
		},										\
		.busy_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, busy_gpios, {0}),			\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),			\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			&auxdisplay_itron_init,							\
			NULL,									\
			&auxdisplay_itron_data_##inst,						\
			&auxdisplay_itron_config_##inst,					\
			POST_KERNEL,								\
			CONFIG_AUXDISPLAY_INIT_PRIORITY,					\
			&auxdisplay_itron_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_ITRON_DEVICE)
