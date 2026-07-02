/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_linux_leds

#include <nsi_errno.h>
#include <nsi_host_trampolines.h>
#include <posix_native_task.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(linux_leds, CONFIG_LED_LOG_LEVEL);

struct linux_led_config {
	const char *path;
	int fd;
	unsigned int max;
};

struct linux_leds_config {
	struct linux_led_config *led;
	int count;
};

static int linux_leds_set_brightness(const struct device *dev,
				     uint32_t led_num, uint8_t value)
{
	const struct linux_leds_config *cfg = dev->config;
	int val;
	char buf[32];
	int len;
	int ret;

	if (led_num >= cfg->count) {
		return -EINVAL;
	}

	struct linux_led_config *led = &cfg->led[led_num];

	/* round up so a binary led turns on any value > 0 */
	val = DIV_ROUND_UP(value * led->max, LED_BRIGHTNESS_MAX);

	len = snprintf(buf, sizeof(buf), "%d", val);
	if (len < 0) {
		return len;
	}

	ret = nsi_host_write(led->fd, buf, len);
	if (ret != len) {
		LOG_ERR("Write error: %s", strerror(nsi_host_get_errno()));
		return -EIO;
	}

	return 0;
}

static DEVICE_API(led, linux_leds_api) = {
	.set_brightness = linux_leds_set_brightness,
};

static int linux_led_open(const char *path, int *max)
{
	/* 34: Length of /sys/class/leds/%s/max_brightness + \0 */
	char full_path[strlen(path) + 34];
	char buf[32];
	int fd;
	int ret;

	ret = snprintf(full_path, sizeof(full_path), "/sys/class/leds/%s/max_brightness", path);
	if (ret < 0) {
		return ret;
	} else if (ret >= sizeof(full_path)) {
		return -ENOMEM;
	}

	fd = nsi_host_open(full_path, ZVFS_O_RDONLY);
	if (fd < 0) {
		LOG_ERR("Failed to open the led max brightness device %s: %s",
			full_path, strerror(nsi_host_get_errno()));
		return -EIO;
	}

	ret = nsi_host_read(fd, buf, sizeof(buf) - 1);
	if (ret < 0) {
		LOG_WRN("Read error: %s", strerror(nsi_host_get_errno()));
		nsi_host_close(fd);
		return -EIO;
	}

	buf[ret] = '\0';
	*max = strtol(buf, NULL, 10);

	nsi_host_close(fd);

	ret = snprintf(full_path, sizeof(full_path), "/sys/class/leds/%s/brightness", path);
	if (ret < 0) {
		return ret;
	} else if (ret >= sizeof(full_path)) {
		return -ENOMEM;
	}

	fd = nsi_host_open(full_path, ZVFS_O_WRONLY);
	if (fd < 0) {
		LOG_ERR("Failed to open the led device %s: %s",
			full_path, strerror(nsi_host_get_errno()));
		return -EIO;
	}

	return fd;
}

static int linux_leds_init(const struct device *dev)
{
	const struct linux_leds_config *cfg = dev->config;
	int ret;

	for (int i = 0; i < cfg->count; i++) {
		struct linux_led_config *led = &cfg->led[i];

		ret = linux_led_open(led->path, &led->max);
		if (ret < 0) {
			goto cleanup;
		}

		led->fd = ret;
	}

	return 0;

cleanup:
	/* Close all fds we opened before failing */
	for (int i = 0; i < cfg->count; i++) {
		if (cfg->led[i].fd < 0) {
			continue;
		}

		nsi_host_close(cfg->led[i].fd);
		cfg->led[i].fd = -1;
	}
	return ret;
}

#define DEVICE_DT_GET_COMMA(node_id) DEVICE_DT_GET(node_id),

static void linux_leds_cleanup(void)
{
	const struct device *devs[] = {
		DT_FOREACH_STATUS_OKAY(DT_DRV_COMPAT, DEVICE_DT_GET_COMMA)
	};
	const int count = ARRAY_SIZE(devs);

	for (int i = 0; i < count; i++) {
		const struct linux_leds_config *cfg = devs[i]->config;

		for (int j = 0; j < cfg->count; j++) {
			if (cfg->led[j].fd < 0) {
				continue;
			}

			nsi_host_close(cfg->led[j].fd);
			cfg->led[j].fd = -1;
		}
	}
}

NATIVE_TASK(linux_leds_cleanup, ON_EXIT, 10);

#define LED_CFG(n) {			\
	.path = DT_PROP(n, path),	\
	.fd = -1,			\
}

#define LINUX_LEDS_INIT(inst)							\
	static const struct linux_leds_config linux_leds_cfg_##inst = {		\
		.led = (struct linux_led_config []){				\
			DT_INST_FOREACH_CHILD_SEP(inst, LED_CFG, (,))		\
		},								\
		.count = DT_INST_CHILD_NUM(inst),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, linux_leds_init, NULL,			\
			      NULL, &linux_leds_cfg_##inst,			\
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,		\
			      &linux_leds_api);

DT_INST_FOREACH_STATUS_OKAY(LINUX_LEDS_INIT)
