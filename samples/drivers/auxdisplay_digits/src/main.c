/*
 * Copyright (c) 2023 Jamie McCrae
 * Copyright (c) 2024-2025 Chen Xingyu <hi@xingrz.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_sample, LOG_LEVEL_DBG);

#define AUXDISPLAY_NODE        DT_NODELABEL(auxdisplay_0)
#if DT_NODE_HAS_COMPAT(AUXDISPLAY_NODE, gpio_7_segment)
#define AUXDISPLAY_DIGIT_COUNT DT_PROP_LEN(AUXDISPLAY_NODE, digit_gpios)
#else
#define AUXDISPLAY_DIGIT_COUNT (DT_PROP(DT_NODELABEL(slcd_panel), columns) * \
	DT_PROP(DT_NODELABEL(slcd_panel), rows))
#endif

static const struct device *const dev = DEVICE_DT_GET(AUXDISPLAY_NODE);
static uint8_t data[AUXDISPLAY_DIGIT_COUNT * 2];

int main(void)
{
	int rc;
	int i, len;

	if (!device_is_ready(dev)) {
		LOG_ERR("Auxdisplay device is not ready.");
		return -ENODEV;
	}

	/* Light up all segments with '.'. */
	for (i = 0; i < AUXDISPLAY_DIGIT_COUNT; i++) {
		data[i * 2] = '8';
		data[i * 2 + 1] = '.';
	}
	rc = auxdisplay_write(dev, data, strlen(data));
	if (rc != 0) {
		LOG_ERR("Failed to write data: %d", rc);
		return rc;
	}

	k_msleep(500);

	/* Reset the cursor. */
	rc = auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	if (rc != 0) {
		LOG_ERR("Failed to set cursor: %d", rc);
		return rc;
	}

	k_msleep(500);

	/* Display '1' on all positions with ':'. */
	for (i = 0; i < AUXDISPLAY_DIGIT_COUNT; i++) {
		data[i * 2] = '1';
		data[i * 2 + 1] = ':';
	}
	rc = auxdisplay_write(dev, data, strlen(data));
	if (rc != 0) {
		LOG_ERR("Failed to write data: %d", rc);
		return rc;
	}

	k_msleep(500);

	/* Blinks it once */

	rc = auxdisplay_display_off(dev);
	if (rc != 0) {
		LOG_ERR("Failed to turn display off: %d", rc);
		return rc;
	}

	k_msleep(500);

	rc = auxdisplay_display_on(dev);
	if (rc != 0) {
		LOG_ERR("Failed to turn display on: %d", rc);
		return rc;
	}

	k_msleep(500);

	/* Clear the display */

	rc = auxdisplay_clear(dev);
	if (rc != 0) {
		LOG_ERR("Failed to clear display: %d", rc);
		return rc;
	}

	k_msleep(500);

	/* Test cursor movement by filling each digit with a number */

	for (i = 0; i < AUXDISPLAY_DIGIT_COUNT; i++) {
		snprintf(data, sizeof(data), "%d", i);
		rc = auxdisplay_write(dev, data, strlen(data));
		if (rc != 0) {
			LOG_ERR("Failed to write data: %d", rc);
			return rc;
		}

		k_msleep(500);
	}

	k_msleep(500);

	/* Count from 0 up to fill all digits */

	for (i = 0;; i = (i + 1) % (int)pow(10, AUXDISPLAY_DIGIT_COUNT)) {
		snprintk(data, sizeof(data), "%d", i);
		len = strlen(data);

		rc = auxdisplay_clear(dev);
		if (rc != 0) {
			LOG_ERR("Failed to clear display: %d", rc);
			return rc;
		}

		/* Right-align the number */
		rc = auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE,
						    AUXDISPLAY_DIGIT_COUNT - len, 0);
		if (rc != 0) {
			LOG_ERR("Failed to set cursor position: %d", rc);
			return rc;
		}

		rc = auxdisplay_write(dev, data, len);
		if (rc != 0) {
			LOG_ERR("Failed to write data: %d", rc);
			return rc;
		}

		k_msleep(100);
	}

	return 0;
}
