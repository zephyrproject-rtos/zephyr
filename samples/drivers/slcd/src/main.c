/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SLCD Panel Sample Application
 *
 * This sample demonstrates the use of SLCD (Segmented LCD) panel driver APIs.
 * It performs the following operations:
 *
 * 1. Obtains the SLCD panel device through the device tree chosen node
 * 2. Queries and logs the panel capabilities
 * 3. Displays numbers (0-9) sequentially on all positions (if supported)
 * 4. Displays uppercase letters (A-Z) sequentially on all positions (if supported)
 * 5. Displays lowercase letters (a-z) sequentially on all positions (if supported)
 * 6. Displays icons sequentially (if supported)
 * 7. Each display item is shown for 500ms
 * 8. Enables blink mode for 3 seconds
 *
 * The sample uses a chosen node (zephyr_slcd) to locate the SLCD panel device.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/slcd_panel.h>

/* Display timing constants (in milliseconds) */
#define DISPLAY_DURATION_MS 100
#define BLINK_DURATION_MS   3000

/* Character display ranges */
#define NUMBERS_COUNT   10
#define UPPERCASE_START 'A'
#define UPPERCASE_END   'Z'
#define LOWERCASE_START 'a'
#define LOWERCASE_END   'z'

/**
 * @brief Get the segment type name as a string
 *
 * @param segment_type The segment type enum value
 * @return Pointer to segment type string
 */
static const char *get_segment_type_name(enum slcd_panel_segment_type segment_type)
{
	switch (segment_type) {
	case SLCD_PANEL_SEGMENT_3:
		return "3-segment";
	case SLCD_PANEL_SEGMENT_7:
		return "7-segment";
	case SLCD_PANEL_SEGMENT_8:
		return "8-segment";
	case SLCD_PANEL_SEGMENT_9:
		return "9-segment";
	case SLCD_PANEL_SEGMENT_10:
		return "10-segment";
	case SLCD_PANEL_SEGMENT_11:
		return "11-segment";
	case SLCD_PANEL_SEGMENT_12:
		return "12-segment";
	case SLCD_PANEL_SEGMENT_14:
		return "14-segment";
	case SLCD_PANEL_SEGMENT_16:
		return "16-segment";
	default:
		return "unknown";
	}
}

/**
 * @brief Display numbers (0-9) on all positions
 *
 * Displays each digit 0-9 sequentially across all positions, with each
 * digit shown for DISPLAY_DURATION_MS milliseconds.
 *
 * @param dev SLCD panel device
 * @param num_positions Number of display positions
 * @return 0 on success, negative errno on failure
 */
static void display_numbers(const struct device *dev, uint8_t num_positions)
{
	LOG_INF("Displaying numbers (0-9) on all positions");

	for (uint8_t number = 0; number < NUMBERS_COUNT; number++) {
		/* Display the same number on all positions */
		for (uint8_t pos = 0; pos < num_positions; pos++) {
			slcd_panel_show_number(dev, pos, number, true);
			k_msleep(DISPLAY_DURATION_MS);
			slcd_panel_show_number(dev, pos, number, false);
			k_msleep(DISPLAY_DURATION_MS);
		}

		LOG_INF("  Displayed: %u", number);
	}
}

/**
 * @brief Display uppercase letters (A-Z) on all positions
 *
 * Displays each letter A-Z sequentially across all positions, with each
 * letter shown for DISPLAY_DURATION_MS milliseconds.
 *
 * @param dev SLCD panel device
 * @param num_positions Number of display positions
 */
static void display_uppercase_letters(const struct device *dev, uint8_t num_positions)
{
	LOG_INF("Displaying uppercase letters (A-Z) on all positions");

	for (uint8_t letter = UPPERCASE_START; letter <= UPPERCASE_END; letter++) {
		/* Display the same letter on all positions */
		for (uint8_t pos = 0; pos < num_positions; pos++) {
			slcd_panel_show_letter(dev, pos, letter, true);
			k_msleep(DISPLAY_DURATION_MS);
			slcd_panel_show_letter(dev, pos, letter, false);
			k_msleep(DISPLAY_DURATION_MS);
		}

		LOG_INF("  Displayed: %c", letter);
	}
}

/**
 * @brief Display lowercase letters (a-z) on all positions
 *
 * Displays each letter a-z sequentially across all positions, with each
 * letter shown for DISPLAY_DURATION_MS milliseconds.
 *
 * @param dev SLCD panel device
 * @param num_positions Number of display positions
 */
static void display_lowercase_letters(const struct device *dev, uint8_t num_positions)
{
	LOG_INF("Displaying lowercase letters (a-z) on all positions");

	for (uint8_t letter = LOWERCASE_START; letter <= LOWERCASE_END; letter++) {
		/* Display the same letter on all positions */
		for (uint8_t pos = 0; pos < num_positions; pos++) {
			slcd_panel_show_letter(dev, pos, letter, true);
			k_msleep(DISPLAY_DURATION_MS);
			slcd_panel_show_letter(dev, pos, letter, false);
			k_msleep(DISPLAY_DURATION_MS);
		}

		LOG_INF("  Displayed: %c", letter);
		k_msleep(DISPLAY_DURATION_MS);
	}
}

/**
 * @brief Display icons sequentially
 *
 * Displays each icon one by one, with each icon shown for DISPLAY_DURATION_MS
 * milliseconds. Only one icon is displayed at a time.
 *
 * @param dev SLCD panel device
 * @param num_icons Number of icons
 */
static void display_icons(const struct device *dev, uint8_t num_icons)
{
	LOG_INF("Displaying icons");

	for (uint8_t icon = 0; icon < num_icons; icon++) {
		/* Turn on the current icon */
		slcd_panel_show_icon(dev, icon, true);

		LOG_INF("  Displayed: icon %u", icon);
		k_msleep(DISPLAY_DURATION_MS);

		/* Turn off the current icon before moving to the next */
		slcd_panel_show_icon(dev, icon, false);
	}
}

/**
 * @brief Enable blink mode for a specified duration
 *
 * Enables the SLCD panel blinking mode and maintains it for BLINK_DURATION_MS
 * milliseconds, then disables it.
 *
 * @param dev SLCD panel device
 */
static void demo_blink_mode(const struct device *dev, uint8_t num_positions, uint8_t num_icons)
{
	LOG_INF("Enabling blink mode for %u ms", BLINK_DURATION_MS);

    /* Display all the segments then blink. */
    for (uint8_t pos = 0; pos < num_positions; pos++) {
        slcd_panel_show_number(dev, pos, 8, true);
    }

	for (uint8_t icon = 0; icon < num_icons; icon++) {
		/* Turn on the current icon */
		slcd_panel_show_icon(dev, icon, true);
	}
    
	slcd_panel_blink(dev, true);

	k_msleep(BLINK_DURATION_MS);

	slcd_panel_blink(dev, false);

	LOG_INF("Blink mode disabled");
}

/**
 * @brief Main application entry point
 *
 * Initializes the SLCD panel device, queries capabilities, and performs
 * a complete demonstration of the display capabilities.
 */
int main(void)
{
	const struct device *slcd_dev;
	struct slcd_panel_capabilities cap;
	int ret;

	/* Obtain the SLCD panel device from the device tree chosen node */
	slcd_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_slcd_panel));
	if (!device_is_ready(slcd_dev)) {
		LOG_ERR("SLCD device %s is not ready. Aborting sample.",
			slcd_dev->name);
		return 1;
	}

	LOG_INF("SLCD sample started on device %s", slcd_dev->name);

	/* Query and log the panel capabilities */
	ret = slcd_panel_get_capabilities(slcd_dev, &cap);
	if (ret < 0) {
		LOG_ERR("Failed to get panel capabilities (error %d)", ret);
		return 1;
	}

	LOG_INF("Panel Capabilities:");
	LOG_INF("  Segment Type: %s", get_segment_type_name(cap.segment_type));
	LOG_INF("  Number of Positions: %u", cap.num_positions);
	LOG_INF("  Number of Icons: %u", cap.num_icons);
	LOG_INF("  Support Numbers: %s", cap.support_number ? "yes" : "no");
	LOG_INF("  Support Letters: %s", cap.support_letter ? "yes" : "no");

	/* Display numbers if supported */
	if (cap.support_number) {
		display_numbers(slcd_dev, cap.num_positions);
	} else {
		LOG_INF("Number display not supported, skipping");
	}

	/* Display letters if supported */
	if (cap.support_letter) {
		display_uppercase_letters(slcd_dev, cap.num_positions);
        display_lowercase_letters(slcd_dev, cap.num_positions);
	} else {
		LOG_INF("Letter display not supported, skipping uppercase letters");
	}

	/* Display icons if any are available */
	if (cap.num_icons > 0) {
		display_icons(slcd_dev, cap.num_icons);
	} else {
		LOG_INF("No icons available, skipping icon display");
	}

	/* Demonstrate blink mode */
	demo_blink_mode(slcd_dev, cap.num_positions, cap.num_icons);

	LOG_INF("SLCD sample completed successfully");

	return 0;
}
