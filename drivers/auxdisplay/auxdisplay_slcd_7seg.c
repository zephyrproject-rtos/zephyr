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
 * an SLCD controller. It reads pin/com multiplexing configuration from the
 * device tree, and uses the parent SLCD controller to set the cooresponding
 * pins/coms to show configured characters
 */

#define DT_DRV_COMPAT slcd_7segment

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/slcd_controller.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(auxdisplay_segment_panel_7, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define BLANK (0x00)

/**
 * @brief 7-segment display patterns for digits 0-9
 *
 * Each digit is encoded as 7 bits where each bit represents a segment:
 * Bit 0 = Segment A, Bit 1 = B, Bit 2 = C, Bit 3 = D, Bit 4 = E, Bit 5 = F, Bit 6 = G
 *
 * Standard 7-segment encoding:
 *	-- A --
 *   |	   |
 *   F	   B
 *   |	   |
 *	-- G --
 *   |	   |
 *   E	   C
 *   |	   |
 *	-- D --
 */
static const uint8_t DIGITS[] = {
	[0] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),          /* 0: ABCDEF */
	[1] = BIT(1) | BIT(2),                                              /* 1: BC */
	[2] = BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(6),                   /* 2: ABDEG */
	[3] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(6),                   /* 3: ABCDG */
	[4] = BIT(1) | BIT(2) | BIT(5) | BIT(6),                            /* 4: Bconfig */
	[5] = BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(6),                   /* 5: ACDFG */
	[6] = BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),          /* 6: ACDEFG */
	[7] = BIT(0) | BIT(1) | BIT(2),                                     /* 7: ABC */
	[8] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6), /* 8: ABCDEFG */
	[9] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6),          /* 9: ABCDFG */
};

/**
 * @brief 7-segment display patterns for uppercase letters A-Z
 *
 * Note: Not all letters can be adequately represented with 7 segments.
 * Some letters are approximations.
 */
static const uint8_t LETTER_UPPER[26] = {
	BIT(0) | BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6),          /* A: ABCEFG */
	BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6), /* B: ABCDEFG */
	BIT(0) | BIT(3) | BIT(4) | BIT(5),                            /* C: ADEF */
	BIT(1) | BIT(2) | BIT(3) | BIT(4) |
		BIT(6), /* No good representation for D, use d instead: BCDEG */
	BIT(0) | BIT(3) | BIT(4) | BIT(5) | BIT(6),          /* E: ADEFG */
	BIT(0) | BIT(4) | BIT(5) | BIT(6),                   /* F: AEFG */
	BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5),          /* G: ACDEF */
	BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6),          /* H: BCEFG */
	BIT(4) | BIT(5),                                     /* I: EF (approximation) */
	BIT(1) | BIT(2) | BIT(3) | BIT(4),                   /* J: BCDE */
	BLANK,                                               /* K: (no good representation) */
	BIT(3) | BIT(4) | BIT(5),                            /* L: DEF */
	BLANK,                                               /* M: (no good representation) */
	BIT(0) | BIT(1) | BIT(2) | BIT(4) | BIT(5),          /* N: ABCEF */
	BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5), /* O: ABCDEF */
	BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(6),          /* P: ABEFG */
	BIT(0) | BIT(1) | BIT(2) | BIT(5) |
		BIT(6),  /* No good representation for Q, use q instead: ABCFG */
	BIT(4) | BIT(6), /* No good representation for R, use r instead: EG */
	BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(6), /* S: ACDFG */
	BIT(0) | BIT(4) | BIT(5) | BIT(6), /* No good representation for T, use t instead: AEFG */
	BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5), /* U: BCDEF */
	BLANK,                                      /* V: (no good representation) */
	BLANK,                                      /* W: (no good representation) */
	BLANK,                                      /* X: (no good representation) */
	BIT(1) | BIT(2) | BIT(3) | BIT(5) |
		BIT(6), /* No good representation for Y, use y instead: BCDFG */
	BLANK,          /* Z: (no good representation) */
};

/**
 * @brief 7-segment display patterns for lowercase letters a-z
 *
 * Lowercase letters use alternative segment combinations where applicable.
 */
static const uint8_t LETTER_LOWER[26] = {
	BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(6), /* a: ABCDEG */
	BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),          /* b: CDEFG */
	BIT(3) | BIT(4) | BIT(6),                            /* c: DEG */
	BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(6),          /* d: BCDEG */
	BIT(0) | BIT(3) | BIT(4) | BIT(5) |
		BIT(6),                    /* No good representation for e, use E instead: ADEFG */
	BIT(0) | BIT(4) | BIT(5) | BIT(6), /* No good representation for f, use F instead: AEFG */
	BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6), /* g: ABCDFG */
	BIT(2) | BIT(4) | BIT(5) | BIT(6),                   /* h: CEFG */
	BIT(0) | BIT(4),                                     /* i: AE (approximation) */
	BIT(0) | BIT(2) | BIT(3),                            /* j: ACD (approximation) */
	BLANK,                                               /* k: (no good representation) */
	BIT(3) | BIT(4) | BIT(5),                            /* l: DEF (approximation) */
	BLANK,                                               /* m: (no good representation) */
	BIT(2) | BIT(4) | BIT(6),                            /* n: CEG */
	BIT(2) | BIT(3) | BIT(4) | BIT(6),                   /* o: CDEG */
	BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(6),          /* p: ABEFG */
	BIT(0) | BIT(1) | BIT(2) | BIT(5) | BIT(6),          /* q: ABCFG */
	BIT(4) | BIT(6),                                     /* r: EG */
	BIT(0) | BIT(2) | BIT(3) | BIT(5) |
		BIT(6),                    /* No good representation for s, use S instead: ACDFG */
	BIT(0) | BIT(4) | BIT(5) | BIT(6), /* t: AEFG */
	BIT(2) | BIT(3) | BIT(4),          /* u: CDE */
	BLANK,                             /* v: (no good representation) */
	BLANK,                             /* w: (no good representation) */
	BLANK,                             /* x: (no good representation) */
	BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6), /* y: BCDFG */
	BLANK,                                      /* z: (no good representation) */
};

/**
 * @brief Configuration structure (compile-time)
 */
struct auxdisplay_7seg_config {
	const struct device *slcd_dev;
	struct auxdisplay_capabilities capabilities;
	/*
	 * Following are the pin/com configurations of the segments/icons which are
	 * read from the panel device tree configuration.
	 */
	const uint8_t *pin_list;
	const uint8_t *com_list;
	const uint8_t pin_list_len;
	const uint8_t com_list_len;
	const uint8_t *segment_pins;
	const uint8_t *segment_coms;
	const uint8_t *icon_pins;
	const uint8_t *icon_coms;
};

struct auxdisplay_7seg_data {
	int16_t cursor_x; /* Current cursor x position */
	int16_t cursor_y; /* Current cursor y position */
};

static int auxdisplay_7seg_display_on(const struct device *dev)
{
	const struct auxdisplay_7seg_config *config = dev->config;

	return slcd_start(config->slcd_dev);
}

static int auxdisplay_7seg_display_off(const struct device *dev)
{
	const struct auxdisplay_7seg_config *config = dev->config;

	return slcd_stop(config->slcd_dev);
}

static int auxdisplay_7seg_cursor_position_set(const struct device *dev,
					       enum auxdisplay_position type, int16_t x, int16_t y)
{
	const struct auxdisplay_7seg_config *config = dev->config;
	struct auxdisplay_7seg_data *data = dev->data;

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -EINVAL;
	}

	if (x < 0 || y < 0) {
		return -EINVAL;
	} else if (x >= config->capabilities.columns || y >= config->capabilities.rows) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;

	return 0;
}

static int auxdisplay_7seg_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct auxdisplay_7seg_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;

	return 0;
}

static int auxdisplay_7seg_capabilities_get(const struct device *dev,
					    struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_7seg_config *config = dev->config;

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_7seg_clear(const struct device *dev)
{
	const struct auxdisplay_7seg_config *config = dev->config;
	struct auxdisplay_7seg_data *data = dev->data;
	uint8_t com_mask = 0;

	data->cursor_x = 0;
	data->cursor_y = 0;

	/* Clear all the segments by setting pins to 0 for all COM lines. */
	for (uint8_t i = 0U; i < config->com_list_len; i++) {
		com_mask |= 1U << config->com_list[i];
	}

	for (uint8_t i = 0U; i < config->pin_list_len; i++) {
		slcd_set_pin(config->slcd_dev, config->pin_list[i], com_mask, false);
	}

	return 0;
}

struct pin_com_entry {
	uint8_t pin;      /* Actual pin number from pin_list */
	uint8_t com_mask; /* OR'ed COM mask for this pin */
};

static int auxdisplay_7seg_write_pattern_on_position(const struct device *dev, uint8_t pattern,
						     uint16_t position)
{
	const struct auxdisplay_7seg_config *config = dev->config;
	struct pin_com_entry entry[7]; /* In worst case, all 7 segments have different pins. */
	uint8_t merged_count = 0;
	int ret = 0;

	/* Iterate through segments A-G (bits 0-6) */
	for (uint8_t segment = 0; segment < 7; segment++) {
		if ((pattern & (1U << segment)) == 0) {
			/* This segment is not part of the pattern */
			continue;
		}

		/* Look up actual pin and COM from arrays */
		uint8_t pin = config->pin_list[config->segment_pins[position * 7 + segment]];
		uint8_t com_mask =
			1U << config->com_list[config->segment_coms[position * 7 + segment]];

		/* Search for existing pin/com entry */
		uint8_t found_idx = merged_count;

		for (uint8_t i = 0; i < merged_count; i++) {
			if (entry[i].pin == pin) {
				found_idx = i;
				break;
			}
		}

		if (found_idx < merged_count) {
			/* Merge COM mask with existing pin entry */
			entry[found_idx].com_mask |= com_mask;
		} else {
			/* Add new pin entry */
			entry[merged_count].pin = pin;
			entry[merged_count].com_mask = com_mask;
			merged_count++;
		}
	}

	/* Call slcd_set_pin once for each merged pin/COM mask pair */
	for (uint8_t i = 0; i < merged_count; i++) {
		ret = slcd_set_pin(config->slcd_dev, entry[i].pin, entry[i].com_mask, true);
		if (ret < 0) {
			LOG_WRN("Failed to set pin %u with COM mask 0x%02x at position %u",
				entry[i].pin, entry[i].com_mask, position);
			/* Continue with other pins */
		}
	}

	return ret;
}

static int auxdisplay_7seg_write(const struct device *dev, const uint8_t *ch, uint16_t len)
{
	const struct auxdisplay_7seg_config *config = dev->config;
	struct auxdisplay_7seg_data *data = dev->data;
	uint16_t i, position;
	uint8_t pattern;
	int ret = 0;

	for (i = 0; i < len; i++) {
		position = data->cursor_y * config->capabilities.columns + data->cursor_x;

		if (ch[i] >= '0' && ch[i] <= '9') {
			pattern = DIGITS[ch[i] - '0'];
		} else if (ch[i] >= 'A' && ch[i] <= 'Z') {
			pattern = LETTER_UPPER[ch[i] - 'A'];
		} else if (ch[i] >= 'a' && ch[i] <= 'z') {
			pattern = LETTER_LOWER[ch[i] - 'a'];
		} else {
			pattern = BLANK;
		}

		ret = auxdisplay_7seg_write_pattern_on_position(dev, pattern, position);
		if (ret != 0) {
			break;
		}

		/* Move the cursor */
		if (data->cursor_x < config->capabilities.columns - 1) {
			data->cursor_x++;
		} else if (data->cursor_y < config->capabilities.rows - 1) {
			data->cursor_x = 0;
			data->cursor_y++;
		} else {
			/* Reach the end of display, stop writing */
			break;
		}
	}

	return ret;
}

static int auxdisplay_7seg_write_icon(const struct device *dev,
				      struct auxdisplay_character *character)
{
	const struct auxdisplay_7seg_config *config = dev->config;
	uint8_t pin = config->pin_list[config->icon_pins[character->index]];
	uint8_t com_mask = 1U << config->com_list[config->icon_coms[character->index]];

	/*
	 * Each icon is controlled by only one pin and com line, and it is either on or off,
	 * so consider its width and height both being 1, and use the first data value.
	 */
	return slcd_set_pin(config->slcd_dev, pin, com_mask,
			    character->data[0] == 0xff ? true : false);
}

static int auxdisplay_7seg_blink(const struct device *dev, bool enabled)
{
	const struct auxdisplay_7seg_config *config = dev->config;

	return slcd_blink(config->slcd_dev, enabled);
}

static int auxdisplay_7seg_init(const struct device *dev)
{
	auxdisplay_7seg_display_on(dev);

	return 0;
}

static DEVICE_API(auxdisplay, auxdisplay_7seg_api) = {
	.display_on = auxdisplay_7seg_display_on,
	.display_off = auxdisplay_7seg_display_off,
	.cursor_position_set = auxdisplay_7seg_cursor_position_set,
	.cursor_position_get = auxdisplay_7seg_cursor_position_get,
	.capabilities_get = auxdisplay_7seg_capabilities_get,
	.clear = auxdisplay_7seg_clear,
	.write = auxdisplay_7seg_write,
	.custom_character_set = auxdisplay_7seg_write_icon,
	.position_blinking_set_enabled = auxdisplay_7seg_blink,
};

#define auxdisplay_7seg_INST(n)                                                                    \
	static struct auxdisplay_7seg_data auxdisplay_7seg_data_##n;                               \
	static const uint8_t auxdisplay_7seg_pin_list_##n[] = DT_PROP(n, pin_list);                \
	static const uint8_t auxdisplay_7seg_com_list_##n[] = DT_PROP(n, com_list);                \
	static const uint8_t                                                                       \
		auxdisplay_7seg_segment_pins_##n[DT_PROP(n, columns) * DT_PROP(n, rows) * 7] =     \
			DT_PROP(n, segment_pins);                                                  \
	static const uint8_t                                                                       \
		auxdisplay_7seg_segment_coms_##n[DT_PROP(n, columns) * DT_PROP(n, rows) * 7] =     \
			DT_PROP(n, segment_coms);                                                  \
	static const uint8_t auxdisplay_7seg_icon_pins_##n[DT_PROP(n, num_icons)] =                \
		DT_PROP(n, icon_pins);                                                             \
	static const uint8_t auxdisplay_7seg_icon_coms_##n[DT_PROP(n, num_icons)] =                \
		DT_PROP(n, icon_coms);                                                             \
	static const struct auxdisplay_7seg_config auxdisplay_7seg_config_##n = {                  \
		.slcd_dev = DEVICE_DT_GET(DT_PARENT(n)),                                           \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_PROP(n, columns),                                    \
				.rows = DT_PROP(n, rows),                                          \
				.custom_characters = DT_PROP(n, num_icons),                        \
			},                                                                         \
		.segment_pins = auxdisplay_7seg_segment_pins_##n,                                  \
		.segment_coms = auxdisplay_7seg_segment_coms_##n,                                  \
		.icon_pins = auxdisplay_7seg_icon_pins_##n,                                        \
		.icon_coms = auxdisplay_7seg_icon_coms_##n,                                        \
		.pin_list = auxdisplay_7seg_pin_list_##n,                                          \
		.pin_list_len = DT_PROP_LEN(n, pin_list),                                          \
		.com_list = auxdisplay_7seg_com_list_##n,                                          \
		.com_list_len = DT_PROP_LEN(n, com_list),                                          \
	};                                                                                         \
	DEVICE_DT_DEFINE(n, auxdisplay_7seg_init, NULL, &auxdisplay_7seg_data_##n,                 \
			 &auxdisplay_7seg_config_##n, POST_KERNEL,                                 \
			 CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_7seg_api);

DT_FOREACH_STATUS_OKAY(slcd_7segment, auxdisplay_7seg_INST)
