/*
 * Copyright (c) 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_sample, LOG_LEVEL_DBG);

/* To perform some tests, at least six columns are needed */
#define EXPECTED_LCD_COLUMNS_MIN 6
/* The text buffer can contain a digit/letter char in each position and a point char
 * for each position. The point char is optional.
 */
#define FIXED_TEXT_MESSAGE_LEN   EXPECTED_LCD_COLUMNS_MIN * 2
#define FIXED_TEXT_MESSAGE_SIZE  FIXED_TEXT_MESSAGE_LEN + 1

int main(void)
{
	struct auxdisplay_capabilities caps;
	int ret;

	LOG_INF("Board target: %s", CONFIG_BOARD_TARGET);
	LOG_INF("STM32L476G Discovery 14Seg Glass LCD Sample Application");

	/* Retrieve the LCD device structure using the chosen node from Devicetree */
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(glass_lcd));

	/* Verify if the device structure is populated and properly instantiated */
	if (dev == NULL) {
		LOG_ERR("Could not find the chosen auxdisplay device node");
		return -ENODEV;
	}

	/* Check if the LCD peripheral driver initialization sequence succeeded at boot */
	if (!device_is_ready(dev)) {
		LOG_ERR("Segment LCD device is not ready for operation");
		return -EBUSY;
	}

	LOG_INF("LCD device ready");

	/*
	 * Send a test string to the 14-segment glass display using standard Zephyr API.
	 * The driver handles the character decoding matrix in background.
	 */
	const char *msg_zephyr = "ZEPHYR";

	ret = auxdisplay_write(dev, (const uint8_t *)msg_zephyr, strlen(msg_zephyr));
	if (ret < 0) {
		LOG_ERR("Failed to write to the auxdisplay device (err: %d)", ret);
		return ret;
	}

	LOG_INF("String successfully written to the display!");
	k_msleep(500);

	auxdisplay_clear(dev);
	k_msleep(500);

	ret = auxdisplay_capabilities_get(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to read the auxdisplay capabilities (err: %d)", ret);
	}
	if (caps.columns < EXPECTED_LCD_COLUMNS_MIN) {
		LOG_WRN("This auxdisplay example needs at least 6 columns, found just %d, texts "
			"are going to be truncated",
			caps.columns);
		/* Don't return here, the driver must be tolerant if receiving texts longer than the
		 * column count.
		 */
	}

	/* Strings "#;#;#;#;#;#\xF" and "#;#;#;#;##\xF" cause the same output, because position 5
	 * does not have any dot.
	 */
	const char *msg_all = "#;#;#;#;#;#\xF";
	const char *msg_alpha_num_1 = "ABCDEF";
	const char *msg_alpha_num_2 = "GHIJKL";
	const char *msg_alpha_num_3 = "MNOPQR";
	const char *msg_alpha_num_4 = "STUVWX";
	const char *msg_alpha_num_5 = "YZ0123";
	const char *msg_alpha_num_6 = "456789";
	const char *msg_unit_prefixes = "dcmun ";
	const char *msg_operators = "+-*/  ";
	/* The percentage symbol is three positions wide and includes the degree symbol */
	const char *msg_symbols = "%_()";
	const char *msg_dot_bar = " . . . . . \x1";
	const char *msg_double_dot_bar = " : : : : : \x6";
	const char *msg_triple_dot_bar = " ; ; ; ; ; \x7";

	while (1) {
		auxdisplay_write(dev, (const uint8_t *)msg_all, strlen(msg_all));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_1, strlen(msg_alpha_num_1));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_2, strlen(msg_alpha_num_2));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_3, strlen(msg_alpha_num_3));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_4, strlen(msg_alpha_num_4));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_5, strlen(msg_alpha_num_5));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_6, strlen(msg_alpha_num_6));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_unit_prefixes,
				 strlen(msg_unit_prefixes));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_operators, strlen(msg_operators));
		k_msleep(1000);
		auxdisplay_write(dev, (const uint8_t *)msg_symbols, strlen(msg_symbols));
		k_msleep(1000);

		for (int i = 0; i < 3; i++) {
			auxdisplay_write(dev, (const uint8_t *)msg_dot_bar, strlen(msg_dot_bar));
			k_msleep(500);

			auxdisplay_write(dev, (const uint8_t *)msg_double_dot_bar,
					 strlen(msg_double_dot_bar));
			k_msleep(500);

			auxdisplay_write(dev, (const uint8_t *)msg_triple_dot_bar,
					 strlen(msg_triple_dot_bar));
			k_msleep(500);
		}
		k_msleep(500);

		/* Bars: from 0 to 15 */
		uint8_t msg_bar[FIXED_TEXT_MESSAGE_SIZE];

		msg_bar[0] = 'B';
		msg_bar[1] = 'A';
		msg_bar[2] = 'R';
		msg_bar[3] = 'S';
		msg_bar[7] = 0;
		for (int i = 0; i < 16; i++) {
			msg_bar[4] = i > 9 ? '1' : ' ';
			msg_bar[5] = i > 9 ? '0' + i - 10 : '0' + i;
			msg_bar[6] = i;
			auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
			k_msleep(500);
		}
		k_msleep(500);

		/* Bars: from 0% to 100%, step 25% */
		msg_bar[5] = 0;
		for (int i = 0; i < 5; i++) {
			snprintf(msg_bar, FIXED_TEXT_MESSAGE_SIZE, "%3d%%", i * 25);
			msg_bar[4] = (1 << i) - 1;
			auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
			k_msleep(500);
		}
	}

	return 0;
}
