/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the shell terminal types and colors.
 * @ingroup shell_api
 */

#ifndef ZEPHYR_INCLUDE_SHELL_TYPES_H_
#define ZEPHYR_INCLUDE_SHELL_TYPES_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup shell_api
 * @{
 */

/** @brief Text colors available for shell output. */
enum shell_vt100_color {
	SHELL_VT100_COLOR_BLACK,   /**< Black. */
	SHELL_VT100_COLOR_RED,     /**< Red. */
	SHELL_VT100_COLOR_GREEN,   /**< Green. */
	SHELL_VT100_COLOR_YELLOW,  /**< Yellow. */
	SHELL_VT100_COLOR_BLUE,    /**< Blue. */
	SHELL_VT100_COLOR_MAGENTA, /**< Magenta. */
	SHELL_VT100_COLOR_CYAN,    /**< Cyan. */
	SHELL_VT100_COLOR_WHITE,   /**< White. */

	SHELL_VT100_COLOR_DEFAULT, /**< Terminal default foreground color. */

	/** @cond INTERNAL_HIDDEN */
	VT100_COLOR_END
	/** @endcond */
};

/** @} */

/** @cond INTERNAL_HIDDEN */

struct shell_vt100_colors {
	enum shell_vt100_color col; /*!< Text color. */
	enum shell_vt100_color bgcol; /*!< Background color. */
};

struct shell_multiline_cons {
	uint16_t cur_x;     /*!< horizontal cursor position in edited command line.*/
	uint16_t cur_x_end; /*!< horizontal cursor position at the end of command.*/
	uint16_t cur_y;     /*!< vertical cursor position in edited command.*/
	uint16_t cur_y_end; /*!< vertical cursor position at the end of command.*/
	uint16_t terminal_hei; /*!< terminal screen height.*/
	uint16_t terminal_wid; /*!< terminal screen width.*/
	uint8_t name_len;   /*!<console name length.*/
};

struct shell_vt100_ctx {
	struct shell_multiline_cons cons;
	struct shell_vt100_colors col;
	uint16_t printed_cmd;  /*!< printed commands counter */
};

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_TYPES_H_ */
