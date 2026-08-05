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

#define EXPECTED_LCD_SEGMENT_TYPE 14

/* To perform some tests, at least six columns are needed */
#define EXPECTED_LCD_COLUMNS_MIN 6

/* To perform some tests, at least 4 indicators are needed */
#define EXPECTED_LCD_INDICATORS_MIN 4

/* The text buffer can contain a digit/letter char in each position and a point char
 * for each position. The point char is optional.
 */
#define FIXED_TEXT_MESSAGE_LEN  (EXPECTED_LCD_COLUMNS_MIN * 2)
#define FIXED_TEXT_MESSAGE_SIZE (FIXED_TEXT_MESSAGE_LEN + 1)

#define SLCD_PANEL_SEGMENT_TYPE   DT_PROP(DT_NODELABEL(slcd_panel), segment_type)
#define SLCD_PANEL_NUM_INDICATORS DT_PROP(DT_NODELABEL(slcd_panel), num_indicators)

static int get_lcd_device(const struct device **dev)
{
	LOG_INF("Board target: %s", CONFIG_BOARD_TARGET);
	LOG_INF("STM32L476G Discovery 14Seg Glass LCD Sample Application");

	if (dev == NULL) {
		LOG_ERR("Invalid pointer to store the device");
		return -EINVAL;
	}

	/* Retrieve the LCD device structure using the chosen node from Devicetree */
	*dev = DEVICE_DT_GET(DT_NODELABEL(auxdisplay_0));

	/* Verify if the device structure is populated and properly instantiated */
	if (*dev == NULL) {
		LOG_ERR("Could not find the chosen auxdisplay device node");
		return -ENODEV;
	}

	/* Check if the LCD peripheral driver initialization sequence succeeded at boot */
	if (!device_is_ready(*dev)) {
		LOG_ERR("Segment LCD device is not ready for operation");
		return -EBUSY;
	}

	LOG_INF("LCD device ready");
	return 0;
}

static int print_title_and_clear(const struct device *dev)
{
	int ret;

	/*
	 * Send a test string to the 14-segment glass display using standard Zephyr API.
	 * The driver handles the character decoding matrix in background.
	 */
	const char *msg_zephyr = "ZEPHYR";

	ret = auxdisplay_write(dev, (const uint8_t *)msg_zephyr, strlen(msg_zephyr));
	if (ret) {
		LOG_ERR("Failed to write to the auxdisplay device (err: %d)", ret);
		return ret;
	}

	LOG_INF("String successfully written to the display!");
	k_msleep(500);

	auxdisplay_clear(dev);
	k_msleep(500);

	return 0;
}

static int check_panel_properties(const struct device *dev)
{
	int ret;
	struct auxdisplay_capabilities caps;

	if (SLCD_PANEL_SEGMENT_TYPE != EXPECTED_LCD_SEGMENT_TYPE) {
		LOG_ERR("This auxdisplay example is for 14 segment LCDs only, found %d",
			SLCD_PANEL_SEGMENT_TYPE);
		return -EINVAL;
	}

	ret = auxdisplay_capabilities_get(dev, &caps);
	if (ret) {
		LOG_ERR("Failed to read the auxdisplay capabilities (err: %d)", ret);
		return ret;
	}
	if (caps.columns < EXPECTED_LCD_COLUMNS_MIN) {
		LOG_WRN("This auxdisplay example needs at least 6 columns, found just "
			"%d, texts "
			"are going to be truncated",
			caps.columns);
		/* Don't return an error here, the driver must be tolerant even receiving texts
		 * longer than the column count.
		 */
	}

	if (SLCD_PANEL_NUM_INDICATORS < EXPECTED_LCD_INDICATORS_MIN) {
		LOG_WRN("This auxdisplay example needs at least 4 indicators, found "
			"just %d, bar "
			"tests could have an esthetically unexpected behaviour",
			SLCD_PANEL_NUM_INDICATORS);
		/* Don't return an error here, the driver must be tolerant even receiving an
		 * indicator index greater than the indicator count.
		 */
	}

	return 0;
}

enum custom_character {
	CUSTOM_CHARACTER_FULL,
	CUSTOM_CHARACTER_DEGREE,
	CUSTOM_CHARACTER_LOW_RING,
	CUSTOM_CHARACTER_PREFIX_D,
	CUSTOM_CHARACTER_PREFIX_C,
	CUSTOM_CHARACTER_PREFIX_M,
	CUSTOM_CHARACTER_PREFIX_U,
	CUSTOM_CHARACTER_PREFIX_N,
	CUSTOM_CHARACTER_COUNT
};

static const uint8_t custom_character_data[CUSTOM_CHARACTER_COUNT][EXPECTED_LCD_SEGMENT_TYPE] = {
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0},
	{0x0, 0xff, 0xff, 0xff, 0xff, 0x0, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0, 0xff, 0xff, 0x0, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0xff, 0x0},
	{0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{0x0, 0x0, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0x0, 0xff, 0x0},
};

static const char *custom_character_names[CUSTOM_CHARACTER_COUNT] = {
	"full", "degree", "low-ring", "prefix-d", "prefix-c", "prefix-m", "prefix-u", "prefix-n",
};

static uint8_t custom_character_codes[CUSTOM_CHARACTER_COUNT];

static int set_custom_characters(const struct device *dev)
{
	int ret;

	for (int i = 0; i < CUSTOM_CHARACTER_COUNT; i++) {
		/* Remove the const qualifier in a maintainable way.
		 * Any other way to cast it makes Sonarqube detect a "const drop" issue.
		 */
		void *character_data = (uint8_t *)(uintptr_t)custom_character_data[i];

		struct auxdisplay_character custom_character = {
			.index = i,
			.data = (uint8_t *)character_data,
			.character_code = 0,
		};

		ret = auxdisplay_custom_character_set(dev, &custom_character);
		if (ret < 0) {
			LOG_ERR("Failed to set the %s custom character (err: %d)",
				custom_character_names[i], ret);
			return ret;
		}

		custom_character_codes[i] = custom_character.character_code;
	}

	return 0;
}

static int write_all_character_segments(const struct device *dev)
{
	int ret;

	uint8_t msg[FIXED_TEXT_MESSAGE_SIZE];

	msg[0] = custom_character_codes[CUSTOM_CHARACTER_FULL];
	msg[1] = custom_character_codes[CUSTOM_CHARACTER_FULL];
	msg[2] = custom_character_codes[CUSTOM_CHARACTER_FULL];
	msg[3] = custom_character_codes[CUSTOM_CHARACTER_FULL];
	msg[4] = custom_character_codes[CUSTOM_CHARACTER_FULL];
	msg[5] = custom_character_codes[CUSTOM_CHARACTER_FULL];
	msg[6] = 0;
	ret = auxdisplay_write(dev, msg, 6);
	if (ret < 0) {
		LOG_ERR("%s failed (err: %d)", __func__, ret);
		return ret;
	}
	k_msleep(1000);
	auxdisplay_clear(dev);

	return 0;
}

static int write_all_indicators(const struct device *dev)
{
	int ret;

	for (int i = 0; i < SLCD_PANEL_NUM_INDICATORS; i++) {
		ret = auxdisplay_custom_indicator_set(dev, i, true);
		if (ret < 0) {
			LOG_ERR("%s failed (err: %d)", __func__, ret);
			return ret;
		}
		k_msleep(250);
	}
	k_msleep(500);
	auxdisplay_clear(dev);

	return 0;
}

/* writes one number at a time from left to right (0 1 2 3 4 5), then from right to left moving the
 * cursor to the left, ending with a space (  9 8 7 6 5).
 */
static int write_one_number_at_a_time_and_set_cursor(const struct device *dev)
{
	int ret;

	for (uint8_t i = 0x30; i < 0x36; i++) {
		auxdisplay_write(dev, &i, 1);
		k_msleep(250);
	}
	ret = auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_RELATIVE, -1, 0);
	if (ret < 0) {
		LOG_ERR("%s failed (err: %d)", __func__, ret);
		return ret;
	}
	auxdisplay_write(dev, "6", 1);
	for (uint8_t i = 0x37; i < 0x3A; i++) {
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_RELATIVE, -2, 0);
		auxdisplay_write(dev, &i, 1);
		k_msleep(250);
	}
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_RELATIVE, -2, 0);
	auxdisplay_write(dev, " ", 1);
	k_msleep(500);
	auxdisplay_clear(dev);

	return 0;
}

static int write_all_characters(const struct device *dev)
{
	const char *msg_alpha_num_1 = "ABCDEF";
	const char *msg_alpha_num_2 = "GHIJKL";
	const char *msg_alpha_num_3 = "MNOPQR";
	const char *msg_alpha_num_4 = "STUVWX";
	const char *msg_alpha_num_5 = "YZ0123";
	const char *msg_alpha_num_6 = "456789";
	const char msg_unit_prefixes[] = {/* d c m u n */
					  custom_character_codes[CUSTOM_CHARACTER_PREFIX_D],
					  custom_character_codes[CUSTOM_CHARACTER_PREFIX_C],
					  custom_character_codes[CUSTOM_CHARACTER_PREFIX_M],
					  custom_character_codes[CUSTOM_CHARACTER_PREFIX_U],
					  custom_character_codes[CUSTOM_CHARACTER_PREFIX_N],
					  ' ',
					  0};
	const char *msg_operators = "+-*/  ";
	const char msg_symbols[] = {'_', '(',
				    ')', custom_character_codes[CUSTOM_CHARACTER_DEGREE],
				    '/', custom_character_codes[CUSTOM_CHARACTER_LOW_RING],
				    0};

	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_1, strlen(msg_alpha_num_1));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_2, strlen(msg_alpha_num_2));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_3, strlen(msg_alpha_num_3));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_4, strlen(msg_alpha_num_4));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_5, strlen(msg_alpha_num_5));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_6, strlen(msg_alpha_num_6));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_unit_prefixes, strlen(msg_unit_prefixes));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_operators, strlen(msg_operators));
	k_msleep(1000);
	auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	auxdisplay_write(dev, (const uint8_t *)msg_symbols, strlen(msg_symbols));
	k_msleep(1000);
	auxdisplay_clear(dev);

	return 0;
}

static int write_indicators_as_characters(const struct device *dev)
{
	const char *msg_dot = " . . . . . .";
	const char *msg_double_dot = " : : : : : :";

	for (int i = 0; i < 3; i++) {
		auxdisplay_clear(dev);
		auxdisplay_write(dev, (const uint8_t *)msg_dot, strlen(msg_dot));
		k_msleep(500);

		auxdisplay_clear(dev);
		auxdisplay_write(dev, (const uint8_t *)msg_double_dot, strlen(msg_double_dot));
		k_msleep(500);

		auxdisplay_clear(dev);
		auxdisplay_write(dev, (const uint8_t *)msg_dot, strlen(msg_dot));
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_double_dot, strlen(msg_double_dot));
		k_msleep(500);
	}
	k_msleep(500);
	auxdisplay_clear(dev);

	return 0;
}

static int write_bars_as_binary_counter_from_0_to_15(const struct device *dev)
{
	uint8_t msg_bar[FIXED_TEXT_MESSAGE_SIZE];

	/* Bars: from 0 to 15 */
	msg_bar[0] = 'B';
	msg_bar[1] = 'A';
	msg_bar[2] = 'R';
	msg_bar[3] = 'S';
	msg_bar[6] = 0;
	for (int i = 0; i < 16; i++) {
		msg_bar[4] = i > 9 ? '1' : ' ';
		msg_bar[5] = i > 9 ? '0' + i - 10 : '0' + i;
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
		/* Set bar indicators in function of i */
		auxdisplay_custom_indicator_set(dev, SLCD_PANEL_NUM_INDICATORS - 4, i & 0x1);
		auxdisplay_custom_indicator_set(dev, SLCD_PANEL_NUM_INDICATORS - 3, i & 0x2);
		auxdisplay_custom_indicator_set(dev, SLCD_PANEL_NUM_INDICATORS - 2, i & 0x4);
		auxdisplay_custom_indicator_set(dev, SLCD_PANEL_NUM_INDICATORS - 1, i & 0x8);
		k_msleep(500);
	}
	k_msleep(500);
	auxdisplay_clear(dev);

	return 0;
}

static int write_bars_as_percentage_indicator(const struct device *dev)
{
	uint8_t msg_bar[FIXED_TEXT_MESSAGE_SIZE];

	/* Bars: from 0% to 100%, step 25% */
	for (int i = 0; i < 5; i++) {
		snprintf(msg_bar, FIXED_TEXT_MESSAGE_SIZE, "%3d", i * 25);
		msg_bar[3] = custom_character_codes[CUSTOM_CHARACTER_DEGREE];
		msg_bar[4] = '/';
		msg_bar[5] = custom_character_codes[CUSTOM_CHARACTER_LOW_RING];
		msg_bar[6] = 0;
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
		/* set bar indicators */
		if (i) {
			auxdisplay_custom_indicator_set(dev, SLCD_PANEL_NUM_INDICATORS - 5 + i,
							true);
		}
		k_msleep(500);
	}
	k_msleep(500);
	auxdisplay_clear(dev);

	return 0;
}

int main(void)
{
	const struct device *dev;
	int ret;

	ret = get_lcd_device(&dev);
	if (ret) {
		return ret;
	}

	ret = print_title_and_clear(dev);
	if (ret) {
		return ret;
	}

	ret = check_panel_properties(dev);
	if (ret) {
		return ret;
	}

	ret = set_custom_characters(dev);
	if (ret) {
		return ret;
	}

	while (1) {
		ret = write_all_character_segments(dev);
		if (ret) {
			return ret;
		}

		ret = write_all_indicators(dev);
		if (ret) {
			return ret;
		}

		ret = write_one_number_at_a_time_and_set_cursor(dev);
		if (ret) {
			return ret;
		}

		ret = write_all_characters(dev);
		if (ret) {
			return ret;
		}

		ret = write_indicators_as_characters(dev);
		if (ret) {
			return ret;
		}

		ret = write_bars_as_binary_counter_from_0_to_15(dev);
		if (ret) {
			return ret;
		}

		ret = write_bars_as_percentage_indicator(dev);
		if (ret) {
			return ret;
		}
	}

	return 0;
}
