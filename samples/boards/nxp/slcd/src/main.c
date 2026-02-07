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

/* Segment bit layout */
#define SEG_A  BIT(0)
#define SEG_B  BIT(1)
#define SEG_C  BIT(2)
#define SEG_D  BIT(3)
#define SEG_E  BIT(4)
#define SEG_F  BIT(5)
#define SEG_G  BIT(6)
#define SEG_DP BIT(7)

static const uint8_t DIGITS_7SEG[] = {
	[0] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
	[1] = SEG_B | SEG_C,
	[2] = SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
	[3] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
	[4] = SEG_B | SEG_C | SEG_F | SEG_G,
	[5] = SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
	[6] = SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
	[7] = SEG_A | SEG_B | SEG_C,
	[8] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
	[9] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
};

/* Keep these as real arrays so sizeof/ARRAY_SIZE works. */
static const uint8_t com_pins[] = DT_PROP(SLCD_NODE, com_pins);
static const uint8_t d_pins[] = DT_PROP(SLCD_NODE, d_pins);

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

static void slcd_compute_masks(uint32_t *low_en, uint32_t *high_en, uint32_t *low_bp,
			       uint32_t *high_bp)
{
	*low_en = 0U;
	*high_en = 0U;
	*low_bp = 0U;
	*high_bp = 0U;

	/* Enable pins and set which ones are backplanes */
	for (size_t i = 0; i < ARRAY_SIZE(com_pins); i++) {
		uint8_t pin = com_pins[i];

		if (pin < 32U) {
			*low_en |= BIT(pin);
			*low_bp |= BIT(pin);
		} else {
			*high_en |= BIT(pin - 32U);
			*high_bp |= BIT(pin - 32U);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(d_pins); i++) {
		uint8_t pin = d_pins[i];

		if (pin < 32U) {
			*low_en |= BIT(pin);
		} else {
			*high_en |= BIT(pin - 32U);
		}
	}
}

static void slcd_log_dt_mapping(void)
{
	uint32_t low_en, high_en, low_bp, high_bp;

	slcd_compute_masks(&low_en, &high_en, &low_bp, &high_bp);

	LOG_INF("SLCD DT com-pins (COM0..COM3): %u %u %u %u", com_pins[0], com_pins[1], com_pins[2],
		com_pins[3]);
	LOG_INF("SLCD DT d-pins   (D0..D7):     %u %u %u %u %u %u %u %u", d_pins[0], d_pins[1],
		d_pins[2], d_pins[3], d_pins[4], d_pins[5], d_pins[6], d_pins[7]);

	LOG_INF("Computed masks: low_en=0x%08x high_en=0x%08x low_bp=0x%08x high_bp=0x%08x", low_en,
		high_en, low_bp, high_bp);
}

/* customer override the default internal one */
uint8_t mcux_slcd_lcd_encode_char(uint8_t ch, bool allow_dot)
{
	uint8_t seg_mask = 0U;

	if (ch >= '0' && ch <= '9') {
		seg_mask = DIGITS_7SEG[ch - '0'];
	}
	if (ch == '-') {
		seg_mask = SEG_G;
	}
	if (ch == ' ') {
		seg_mask = 0U;
	}
	if (allow_dot) {
		seg_mask |= SEG_DP;
	}
	return seg_mask;
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
	slcd_log_dt_mapping();

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
