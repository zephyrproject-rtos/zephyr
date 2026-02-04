/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SLCD Panel 7-Segment Display Driver
 *
 * This driver implements support for 7-segment LCD displays connected to
 * an SLCD controller. It provides APIs to display numbers (0-9), letters (A-Z, a-z),
 * and special icons on the segmented display.
 *
 * The driver reads segment and icon multiplexing configuration from the device tree
 * and uses the parent SLCD controller to drive the individual segments.
 */

#define DT_DRV_COMPAT zephyr_segment7

#include <zephyr/device.h>
#include <zephyr/drivers/slcd_controller.h>
#include <zephyr/drivers/slcd_panel.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(segment7, CONFIG_SLCD_LOG_LEVEL);

/**
 * @brief Configuration structure (compile-time)
 */
struct segment7_config {
	const struct device *slcd_dev;
	uint32_t num_positions;
	uint32_t num_icons;
	uint32_t num_segments;
	const uint32_t *segment_mux;
	uint32_t segment_mux_len;
	const uint32_t *icon_mux;
	uint32_t icon_mux_len;
	const uint32_t *pin_list;
	uint32_t pin_list_len;
	const uint32_t *com_list;
	uint32_t com_list_len;
};

/**
 * @brief 7-segment display patterns for digits 0-9
 *
 * Each digit is encoded as 7 bits where each bit represents a segment:
 * Bit 0 = Segment A, Bit 1 = B, Bit 2 = C, Bit 3 = D, Bit 4 = E, Bit 5 = F, Bit 6 = G
 *
 * Standard 7-segment encoding:
 *     A
 *    ---
 *   |   |
 *   F   B
 *    ---
 *   |   | G
 *   E   C
 *    ---
 *     D
 */
static const uint8_t digit_patterns[10] = {
	0x3F,  /* 0: ABCDEF (0111111) */
	0x06,  /* 1: BC (0000110) */
	0x5B,  /* 2: ABDEG (1011011) */
	0x4F,  /* 3: ABCDG (1001111) */
	0x66,  /* 4: BCFG (1100110) */
	0x6D,  /* 5: ACDFG (1101101) */
	0x7D,  /* 6: ACDEFG (1111101) */
	0x07,  /* 7: ABC (0000111) */
	0x7F,  /* 8: ABCDEFG (1111111) */
	0x6F,  /* 9: ABCDFG (1101111) */
};

/**
 * @brief 7-segment display patterns for uppercase letters A-Z
 *
 * Note: Not all letters can be adequately represented with 7 segments.
 * Some letters are approximations.
 */
static const uint8_t letter_patterns_upper[26] = {
	0x77,  /* A: ABCEFG */
	0x7F,  /* B: ABCDEFG */
	0x39,  /* C: ADEF */
	0x00,  /* D: (no good 7-segment representation) */
	0x79,  /* E: ADEFG */
	0x71,  /* F: AEFG */
	0x3D,  /* G: ACDEF */
	0x76,  /* H: BCEFG */
	0x30,  /* I: BC (approximation) */
	0x1E,  /* J: BCDE */
	0x00,  /* K: (no good 7-segment representation) */
	0x38,  /* L: DEF */
	0x00,  /* M: (no good 7-segment representation) */
	0x37,  /* N: ABCEF */
	0x3F,  /* O: ABCDEF */
	0x73,  /* P: ABEFG */
	0x00,  /* Q: (no good 7-segment representation) */
	0x00,  /* R: (no good 7-segment representation) */
	0x6D,  /* S: ACDFG */
	0x00,  /* T: (no good 7-segment representation) */
	0x3E,  /* U: BCDEF */
	0x00,  /* V: (no good 7-segment representation) */
	0x00,  /* W: (no good 7-segment representation) */
	0x00,  /* X: (no good 7-segment representation) */
	0x00,  /* Y: (no good 7-segment representation) */
	0x00,  /* Z: (no good 7-segment representation) */
};

/**
 * @brief 7-segment display patterns for lowercase letters a-z
 *
 * Lowercase letters use alternative segment combinations where applicable.
 */
static const uint8_t letter_patterns_lower[26] = {
	0x5F,  /* a: ABCDEG */
	0x7C,  /* b: CDEFG */
	0x58,  /* c: DEG */
	0x5E,  /* d: BCDEG */
	0x00,  /* e: (no good 7-segment representation) */
	0x00,  /* f: (no good representation) */
	0x6F,  /* g: ABCDFG */
	0x74,  /* h: CEFG */
	0x11,  /* i: AE (approximation) */
	0x0D,  /* j: ACD (approximation) */
	0x00,  /* k: (no good representation) */
	0x38,  /* l: EF (approximation) */
	0x00,  /* m: (no good representation) */
	0x54,  /* n: CEG */
	0x5C,  /* o: CDEG */
	0x73,  /* p: ABEFG */
	0x67,  /* q: ABCFG */
	0x50,  /* r: EG */
	0x00,  /* s: (no good 7-segment representation) */
	0x78,  /* t: AEFG */
	0x1C,  /* u: CDEG */
	0x00,  /* v: (no good representation) */
	0x00,  /* w: (no good representation) */
	0x00,  /* x: (no good representation) */
	0x6E,  /* y: BCDFG */
	0x00,  /* z: (no good 7-segment representation) */
};

/**
 * @brief Merged pin/COM mask entry
 *
 * Represents a unique pin with combined COM masks from multiple segments
 */
struct merged_pin_entry {
	uint8_t pin;            /* Actual pin number from pin_list */
	uint8_t com_mask;       /* Combined COM mask for this pin */
};

/**
 * @brief Decode mux value to extract pin and COM bit position
 *
 * The mux encoding format is 0xAB where:
 * - A (upper 8 bits): Pin number (0-based)
 * - B (lower 8 bits): COM bit mask (single bit set, representing which COM line)
 *
 * @param mux_value Encoded mux value
 * @param pin Pointer to store decoded pin number
 * @param com_bit Pointer to store decoded COM bit position (0-7)
 */
static inline void decode_mux_value(uint16_t mux_value, uint8_t *pin,
				     uint8_t *com_bit)
{
 *pin = (mux_value >> 8) & 0xFF;
	/* Extract which bit is set in the low byte (0-7) */
 *com_bit = __builtin_ctz(mux_value & 0xFF);
}

/**
 * @brief Collect and merge mux values by pin for a given pattern
 *
 * Processes all segments in a pattern, collects their mux values,
 * and merges those with the same pin (combining COM masks).
 *
 * @param dev SLCD panel device instance
 * @param position Display position (0-based)
 * @param pattern 7-bit pattern indicating which segments to enable
 * @param merged_pins Array to store merged pin/COM mask entries
 * @param merged_count Pointer to store count of merged entries
 *
 * @return 0 on success, negative error code on failure
 */
static int collect_and_merge_mux_values(const struct device *dev,
					uint8_t position,
					uint8_t pattern,
					struct merged_pin_entry *merged_pins,
					uint8_t *merged_count)
{
	const struct segment7_config *config = dev->config;
	uint8_t count = 0;

	/* Iterate through segments A-G (bits 0-6) */
	for (uint8_t segment = 0; segment < 7; segment++) {
		if ((pattern & (1U << segment)) == 0) {
			/* This segment is not part of the pattern */
			continue;
		}

		/* Get mux index for this segment at this position */
		uint16_t mux_value = config->segment_mux[position * 7 + segment];
		uint8_t pin_index;
		uint8_t com_index;

		decode_mux_value(mux_value, &pin_index, &com_index);

		/* Validate pin index is within bounds */
		if (pin_index > config->pin_list_len) {
			LOG_ERR("Pin index %u out of range (1-%u)", pin_index,
				config->pin_list_len);
			return -ENOTSUP;
		}

		/* Validate com index is within bounds */
		if (com_index >= config->com_list_len) {
			LOG_ERR("COM index %u out of range (0-%u)", com_index,
				config->com_list_len - 1);
			return -ENOTSUP;
		}

		/* Look up actual pin and COM mask from arrays */
		uint32_t pin = config->pin_list[pin_index];
		uint8_t com_mask = 1U << config->com_list[com_index];

		/* Search for existing pin entry in merged list */
		uint32_t found_idx = count;
		for (uint32_t i = 0; i < count; i++) {
			if (merged_pins[i].pin == pin) {
				found_idx = i;
				break;
			}
		}

		if (found_idx < count) {
			/* Merge COM mask with existing pin entry */
			merged_pins[found_idx].com_mask |= com_mask;
		} else {
			/* Add new pin entry */
			merged_pins[count].pin = pin;
			merged_pins[count].com_mask = com_mask;
			count++;

			if (count >= 7) {
				/* Safety check: should not exceed number of segments */
				LOG_WRN("Merged pin count exceeded 7 for position %u",
					position);
				break;
			}
		}
	}

 *merged_count = count;
	return 0;
}

/**
 * @brief Display a number at a specific position
 *
 * @param dev SLCD panel device instance
 * @param position Display position (0-based)
 * @param number Digit 0-9 to display
 * @param on true to turn on, false to turn off
 *
 * @return 0 on success, negative error code on failure
 */
static int segment7_show_number(const struct device *dev, uint32_t position,
			 uint8_t number, bool on)
{
	const struct segment7_config *config = dev->config;
	int ret;

	if (position >= config->num_positions) {
		LOG_ERR("Position %u out of range (max %u)", position,
			config->num_positions - 1);
		return -EINVAL;
	}

	if (number >= 10) {
		LOG_ERR("Number %u out of range (0-9)", number);
		return -EINVAL;
	}

	uint8_t pattern = digit_patterns[number];

	/* Collect and merge mux values by pin for all segments in pattern */
	struct merged_pin_entry merged_pins[7];
	uint8_t merged_count = 0;

	ret = collect_and_merge_mux_values(dev, position, pattern,
					   merged_pins, &merged_count);
	if (ret < 0) {
		LOG_ERR("Failed to collect and merge mux values for position %u",
			position);
		return ret;
	}

	/* Call slcd_set_pin once for each merged pin/COM mask pair */
	for (uint8_t i = 0; i < merged_count; i++) {
		ret = slcd_set_pin(config->slcd_dev, merged_pins[i].pin,
				   merged_pins[i].com_mask, on);
		if (ret < 0) {
			LOG_WRN("Failed to set pin %u with COM mask 0x%02x at position %u",
				merged_pins[i].pin, merged_pins[i].com_mask, position);
			/* Continue with other pins */
		}
	}

	return 0;
}

/**
 * @brief Display a letter at a specific position
 *
 * @param dev SLCD panel device instance
 * @param position Display position (0-based)
 * @param letter ASCII character (A-Z or a-z) to display
 * @param on true to turn on, false to turn off
 *
 * @return 0 on success, negative error code on failure
 */
static int segment7_show_letter(const struct device *dev, uint32_t position,
			 uint8_t letter, bool on)
{
	const struct segment7_config *config = dev->config;
	int ret;

	if (position >= config->num_positions) {
		LOG_ERR("Position %u out of range (max %u)", position,
			config->num_positions - 1);
		return -EINVAL;
	}

	uint8_t pattern;

	/* Convert ASCII to pattern lookup */
	if (letter >= 'A' && letter <= 'Z') {
		/* Uppercase letter */
		pattern = letter_patterns_upper[letter - 'A'];
	} else if (letter >= 'a' && letter <= 'z') {
		/* Lowercase letter */
		pattern = letter_patterns_lower[letter - 'a'];
	} else {
		LOG_ERR("Not an alphabetic letter: %c", letter);
		return -EINVAL;
	}

	if (pattern == 0) {
		LOG_WRN("Letter %c has no 7-segment representation",
			letter);
		return -ENOTSUP;
	}

	/* Collect and merge mux values by pin for all segments in pattern */
	struct merged_pin_entry merged_pins[7];
	uint8_t merged_count = 0;

	ret = collect_and_merge_mux_values(dev, position, pattern,
					   merged_pins, &merged_count);
	if (ret < 0) {
		LOG_ERR("Failed to collect and merge mux values for letter at position %u",
			position);
		return ret;
	}

	/* Call slcd_set_pin once for each merged pin/COM mask pair */
	for (uint8_t i = 0; i < merged_count; i++) {
		ret = slcd_set_pin(config->slcd_dev, merged_pins[i].pin,
				   merged_pins[i].com_mask, on);
		if (ret < 0) {
			LOG_WRN("Failed to set pin %u with COM mask 0x%02x for letter at position %u",
				merged_pins[i].pin, merged_pins[i].com_mask, position);
			/* Continue with other pins */
		}
	}

	return 0;
}

/**
 * @brief Display a special icon
 *
 * @param dev SLCD panel device instance
 * @param icon_index Index of the icon to control (0-based)
 * @param on true to turn on, false to turn off
 *
 * @return 0 on success, negative error code on failure
 */
static int segment7_show_icon(const struct device *dev, uint32_t icon_index,
		       bool on)
{
	const struct segment7_config *config = dev->config;

	if (icon_index >= config->icon_mux_len) {
		LOG_ERR("Icon index %u out of range (max %u)", icon_index,
			config->icon_mux_len - 1);
		return -EINVAL;
	}

	uint16_t mux_value = config->icon_mux[icon_index];
	uint8_t pin_index;
	uint8_t com_index;

	decode_mux_value(mux_value, &pin_index, &com_index);

	/* Validate pin index is within bounds */
	if (pin_index > config->pin_list_len) {
		LOG_ERR("Icon %u: Pin index %u out of range (1-%u)", icon_index,
			pin_index, config->pin_list_len);
		return -ENOTSUP;
	}

	/* Validate com index is within bounds */
	if (com_index >= config->com_list_len) {
		LOG_ERR("Icon %u: COM index %u out of range (0-%u)", icon_index,
			com_index, config->com_list_len - 1);
		return -ENOTSUP;
	}

	/* Look up actual pin and COM values from arrays */
	uint8_t pin = config->pin_list[pin_index];
	uint8_t com_mask = 1U << config->com_list[com_index];

	return slcd_set_pin(config->slcd_dev, pin, com_mask, on);
}

/**
 * @brief Enable or disable panel blinking
 *
 * @param dev SLCD panel device instance
 * @param on true to enable blinking, false to disable blinking
 *
 * @return 0 on success, negative error code on failure
 */
static int segment7_blink(const struct device *dev, bool on)
{
	const struct segment7_config *config = dev->config;

	return slcd_blink(config->slcd_dev, on);
}

/**
 * @brief Get panel capabilities
 *
 * @param dev SLCD panel device instance
 * @param cap Pointer to capabilities structure to fill
 *
 * @return 0 on success, negative error code on failure
 */
static int segment7_get_capabilities(const struct device *dev,
				    struct slcd_panel_capabilities *cap)
{
	const struct segment7_config *config = dev->config;

	if (cap == NULL) {
		return -EINVAL;
	}

	cap->segment_type = SLCD_PANEL_SEGMENT_7;
	cap->num_positions = config->num_positions;
	cap->num_icons = config->num_icons;
	cap->support_number = true;
	cap->support_letter = true;

	return 0;
}

/**
 * @brief Driver API structure
 */
static const struct slcd_panel_driver_api segment7_driver_api = {
	.show_number = segment7_show_number,
	.show_letter = segment7_show_letter,
	.show_icon = segment7_show_icon,
	.blink = segment7_blink,
	.get_capabilities = segment7_get_capabilities,
};


/* Define driver instances for each segment7 device in device tree */
#define SEGMENT7_INIT(node_id)                                                 \
	static const uint32_t segment7_segment_mux_##node_id[] =                   \
		DT_PROP(node_id, segment_mux);                                     \
	static const uint32_t segment7_icon_mux_##node_id[] =                      \
		DT_PROP(node_id, icon_mux);                                        \
	static const uint32_t segment7_pin_list_##node_id[] =                      \
		DT_PROP(node_id, pin_list);                                        \
	static const uint32_t segment7_com_list_##node_id[] =                      \
		DT_PROP(node_id, com_list);                                        \
                                                                                 \
	static const struct segment7_config segment7_config_##node_id = {          \
		.slcd_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                    \
		.num_positions = DT_PROP(node_id, num_positions),                  \
		.num_icons = DT_PROP(node_id, num_icons),                          \
		.num_segments = 7,                                                 \
		.segment_mux = segment7_segment_mux_##node_id,                     \
		.segment_mux_len = DT_PROP_LEN(node_id, segment_mux),             \
		.icon_mux = segment7_icon_mux_##node_id,                           \
		.icon_mux_len = DT_PROP_LEN(node_id, icon_mux),                   \
		.pin_list = segment7_pin_list_##node_id,                           \
		.pin_list_len = DT_PROP_LEN(node_id, pin_list),                   \
		.com_list = segment7_com_list_##node_id,                           \
		.com_list_len = DT_PROP_LEN(node_id, com_list),                   \
	};                                                                         \
                                                                                 \
	DEVICE_DT_DEFINE(node_id, NULL, NULL,                                      \
			 NULL,                                                         \
			 &segment7_config_##node_id, POST_KERNEL,                 \
			 CONFIG_SLCD_PANEL_INIT_PRIORITY,                         \
			 &segment7_driver_api)

DT_FOREACH_STATUS_OKAY(zephyr_segment7, SEGMENT7_INIT);
