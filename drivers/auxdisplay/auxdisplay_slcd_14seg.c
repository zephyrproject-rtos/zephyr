/*
 * Copyright 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "auxdisplay_slcd_config.h"

LOG_MODULE_REGISTER(auxdisplay_st_14seg, CONFIG_AUXDISPLAY_LOG_LEVEL);

uint16_t slcd_14seg_convert_ascii_char_to_14seg_pattern(const struct auxdisplay_panel_config *panel,
							uint8_t ascii_char)
{
	switch (ascii_char) {
	case ' ':
		return SLCD_C_SPACE_MAP;
	case '_':
		return SLCD_C_UNDERSCORE_MAP;
	case '-':
		return SLCD_C_MINUS_MAP;
	case '+':
		return SLCD_C_PLUS_MAP;
	case '/':
		return SLCD_C_SLASH_MAP;
	case '*':
		return SLCD_C_STAR_MAP;
	case '(':
		return SLCD_C_OPEN_PAR_MAP;
	case ')':
		return SLCD_C_CLOSE_PAR_MAP;
	default:
		if (IN_RANGE(ascii_char, '0', '9')) {
			return panel->digits[ascii_char - '0'];
		}
		if (IN_RANGE(ascii_char, 'A', 'Z')) {
			return panel->letters_upper[ascii_char - 'A'];
		}
	}

	LOG_WRN("ASCII character not supported (0x%X)", ascii_char);
	return SLCD_BLANK;
}
