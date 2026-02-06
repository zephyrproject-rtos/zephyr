/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_slcd.h>

#include <string.h>

LOG_MODULE_REGISTER(nxp_slcd_sample, LOG_LEVEL_INF);

/* Slow the demo down so the SLCD contents are readable.
 * Adjust these as needed.
 */
#define SANITY_DELAY_MS  1500
#define COUNTER_DELAY_MS 1000

#if !DT_HAS_COMPAT_STATUS_OKAY(nxp_slcd)
#error "No nxp,slcd device found in devicetree"
#endif

/* Board DTS in this workspace labels the SLCD auxdisplay node as auxdisplay0 */
#define SLCD_NODE DT_NODELABEL(auxdisplay0)

static const struct device *get_slcd_dev(void)
{
	return DEVICE_DT_GET(SLCD_NODE);
}

static void write_str(const struct device *dev, const char *s)
{
	(void)auxdisplay_clear(dev);
	(void)auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	(void)auxdisplay_write(dev, (const uint8_t *)s, strlen(s));
}

int main(void)
{
	const struct device *slcd = get_slcd_dev();
	uint32_t n = 0;

	if (!device_is_ready(slcd)) {
		LOG_ERR("SLCD auxdisplay device not ready");
		return 0;
	}

	LOG_INF("NXP SLCD demo starting");

	(void)auxdisplay_display_on(slcd);
	(void)auxdisplay_clear(slcd);
	LOG_INF("NXP SLCD demo running 8888");
	write_str(slcd, "8888");
	LOG_INF("NXP SLCD demo running 12:12");
	write_str(slcd, "12:12");
	LOG_INF("NXP SLCD demo running 12:12:12");
	write_str(slcd, "12:12:12");

	/* Quick sanity pattern using auxdisplay */
	LOG_INF("Write 0123, 45.67, 78.90 using auxdisplay API");
	write_str(slcd, "0123");
	k_sleep(K_MSEC(SANITY_DELAY_MS));
	write_str(slcd, "45.67");
	k_sleep(K_MSEC(SANITY_DELAY_MS));
	write_str(slcd, "78.90");
	k_sleep(K_MSEC(SANITY_DELAY_MS));

	for (;;) {
		/* 0..9999 counter */
		uint32_t v = n % 10000U;
		char out[6];
		size_t j = 0;
		uint8_t dot_pos = (uint8_t)((n / 10U) % 4U);
		char d[4];

		d[0] = '0' + (v / 1000U);
		d[1] = '0' + ((v / 100U) % 10U);
		d[2] = '0' + ((v / 10U) % 10U);
		d[3] = '0' + (v % 10U);

		/* Insert '.' in the same write buffer so it attaches to the intended digit.
		 * '.' does not consume a digit position in the driver.
		 */
		for (uint8_t i = 0; i < 4; i++) {
			out[j++] = d[i];
			if (dot_pos < 3U && dot_pos == i) {
				out[j++] = '.';
			}
		}
		out[j] = '\0';
		LOG_INF("Counter: %s", out);
		write_str(slcd, out);

		n++;
		k_sleep(K_MSEC(COUNTER_DELAY_MS));
	}
}
