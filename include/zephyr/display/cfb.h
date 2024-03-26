/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Compact Framebuffer subsystem API
 */

#ifndef __CFB_H__
#define __CFB_H__

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Public Compact Framebuffer subsystem API
 * @defgroup compact_framebuffer Compact Framebuffer
 * @ingroup utilities
 * @{
 */

/**
 * @brief Macro for creating a font entry.
 *
 * @param _name   Name of the font entry.
 * @param _width  Width of the font in pixels
 * @param _height Height of the font in pixels.
 * @param _caps   Font capabilities.
 * @param _data   Raw data of the font.
 * @param _fc     Character mapped to first font element.
 * @param _lc     Character mapped to last font element.
 */
#define FONT_ENTRY_DEFINE(_name, _width, _height, _caps, _data, _fc, _lc)                          \
	static const STRUCT_SECTION_ITERABLE(cfb_font, _name) = {                                  \
		.data = _data,                                                                     \
		.caps = _caps,                                                                     \
		.width = _width,                                                                   \
		.height = _height,                                                                 \
		.first_char = _fc,                                                                 \
		.last_char = _lc,                                                                  \
	}

/**
 * @brief Initializer macro for draw-point operation
 *
 * @param _x x position of the point
 * @param _y y position of the point
 */
#define CFB_OP_INIT_DRAW_POINT(str, _x, _y)                                                        \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_DRAW_POINT,                                                   \
			.draw_figure =                                                             \
				{                                                                  \
					.start = {_x, _y},                                         \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for draw-line operation
 *
 * @param _s_x Start x position of the line
 * @param _s_y Start y position of the line
 * @param _e_x End x position of the line
 * @param _e_y End y position of the line
 */
#define CFB_OP_INIT_DRAW_LINE(_s_x, _s_y, _e_x, _e_y)                                              \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_DRAW_LINE,                                                    \
			.draw_figure =                                                             \
				{                                                                  \
					.start = {_s_x, _s_y},                                     \
					.end = {_e_x, _e_y},                                       \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for draw-rect operation
 *
 * @param _s_x Top-Left x position of the rectangle
 * @param _s_y Top-Left x position of the rectangle
 * @param _e_x Bottom-Right y position of the rectangle
 * @param _e_y Top-Left y position of the rectangle
 */
#define CFB_OP_INIT_DRAW_RECT(_s_x, _s_y, _e_x, _e_y)                                              \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_DRAW_RECT,                                                    \
			.draw_figure =                                                             \
				{                                                                  \
					.start = {_s_x, _s_y},                                     \
					.end = {_e_x, _e_y},                                       \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for draw-text operation
 *
 * @param _str String to print
 * @param _x Position in X direction of the beginning of the strings
 * @param _y Position in Y direction of the beginning of the strings
 */
#define CFB_OP_INIT_DRAW_TEXT(_str, _x, _y)                                                        \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_DRAW_TEXT,                                                    \
			.draw_text =                                                               \
				{                                                                  \
					.pos = {_x, _y},                                           \
					.str = _str,                                               \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for print operation
 *
 * @param _str String to print
 * @param _x Position in X direction of the beginning of the strings
 * @param _y Position in Y direction of the beginning of the strings
 */
#define CFB_OP_INIT_PRINT(_str, _x, _y)                                                            \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_PRINT,                                                        \
			.draw_text =                                                               \
				{                                                                  \
					.pos = {_x, _y},                                           \
					.str = _str,                                               \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for draw text-reference operation
 *
 * @param _str String to print
 * @param _x Position in X direction of the beginning of the strings
 * @param _y Position in Y direction of the beginning of the strings
 */
#define CFB_OP_INIT_DRAW_TEXT_REF(_str, _x, _y)                                                    \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_DRAW_TEXT_REF,                                                \
			.draw_text =                                                               \
				{                                                                  \
					.pos = {_x, _y},                                           \
					.str = _str,                                               \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for print-ref operation
 *
 * @param _str String to print
 * @param _x Position in X direction of the beginning of the strings
 * @param _y Position in Y direction of the beginning of the strings
 */
#define CFB_OP_INIT_PRINT_REF(_str, _x, _y)                                                        \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_PRINT_REF,                                                    \
			.draw_text =                                                               \
				{                                                                  \
					.pos = {_x, _y},                                           \
					.str = _str,                                               \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for invert-area operation
 *
 * @param _x Position in X direction of the beginning of area
 * @param _y Position in Y direction of the beginning of area
 * @param _w Width of area in pixels
 * @param _h Height of area in pixels
 *
 */
#define CFB_OP_INIT_INVERT_AREA(_x, _y, _w, _h)                                                    \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_INVERT_AREA,                                                  \
			.invert_area =                                                             \
				{                                                                  \
					.x = _x,                                                   \
					.y = _y,                                                   \
					.w = _w,                                                   \
					.h = _h,                                                   \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for set-font operation
 *
 * @param _font_idx Font index
 */
#define CFB_OP_INIT_SET_FONT(_font_idx)                                                            \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_SET_FONT,                                                     \
			.set_font =                                                                \
				{                                                                  \
					.font_idx = _font_idx,                                     \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for set-kerning operation
 *
 * @param _kerning Spacing between each characters in pixels
 */
#define CFB_OP_INIT_SET_KERNING(_kerning)                                                          \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_SET_KERNING,                                                  \
			.set_kerning =                                                             \
				{                                                                  \
					.kerning = _kerning,                                       \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for set-fgcolor operation
 * @param r red in 24-bit color notation
 * @param g green in 24-bit color notation
 * @param b blue in 24-bit color notation
 * @param a alpha channel
 */
#define CFB_OP_INIT_SET_FG_COLOR(r, g, b, a)                                                       \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_SET_FG_COLOR,                                                 \
			.set_color =                                                               \
				{                                                                  \
					.red = r,                                                  \
					.green = g,                                                \
					.blue = b,                                                 \
					.alpha = a,                                                \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for set-bgcolor operation
 * @param r red in 24-bit color notation
 * @param g green in 24-bit color notation
 * @param b blue in 24-bit color notation
 * @param a alpha channel
 */
#define CFB_OP_INIT_SET_BG_COLOR(r, g, b, a)                                                       \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_SET_BG_COLOR,                                                 \
			.set_color =                                                               \
				{                                                                  \
					.red = r,                                                  \
					.green = g,                                                \
					.blue = b,                                                 \
					.alpha = a,                                                \
				},                                                                 \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for swap fg/bg color operation
 */
#define CFB_OP_INIT_SWAP_FG_BG_COLOR()                                                             \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_SWAP_FG_BG_COLOR,                                             \
		},                                                                                 \
	}

/**
 * @brief Initializer macro for set command buffer operation
 * @param _cmd_buf Command buffer
 */
#define CFB_OP_INIT_COMMAND_BUFFER(_cmd_buf)                                                       \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_SET_COMMAND_BUFFER,                                           \
			.cmd_buffer =                                                              \
				{                                                                  \
					.cmd_buf = _cmd_buf,                                       \
				},                                                                 \
		},                                                                                 \
	}

enum cfb_display_param {
	CFB_DISPLAY_HEIGH		= 0,
	CFB_DISPLAY_WIDTH,
	CFB_DISPLAY_PPT,
	CFB_DISPLAY_ROWS,
	CFB_DISPLAY_COLS,
};

enum cfb_font_caps {
	CFB_FONT_MONO_VPACKED		= BIT(0),
	CFB_FONT_MONO_HPACKED		= BIT(1),
	CFB_FONT_MSB_FIRST		= BIT(2),
};

/**
 * @brief CFB operation types.
 */
enum cfb_operation {
	CFB_OP_NOP = 0x0,
	/** Inverts the color of the specified area */
	CFB_OP_INVERT_AREA,
	/** Draw a point */
	CFB_OP_DRAW_POINT,
	/** Draw a line */
	CFB_OP_DRAW_LINE,
	/** Draw a rectangle */
	CFB_OP_DRAW_RECT,
	/** Draw a text */
	CFB_OP_DRAW_TEXT,
	/** Print a text */
	CFB_OP_PRINT,
	/** Draw a text specified by reference */
	CFB_OP_DRAW_TEXT_REF,
	/** Print a text specified by reference */
	CFB_OP_PRINT_REF,
	/** Set font for text rendering */
	CFB_OP_SET_FONT,
	/** Set kerning for text rendering */
	CFB_OP_SET_KERNING,
	/** Set foreground color */
	CFB_OP_SET_FG_COLOR,
	/** Set background color */
	CFB_OP_SET_BG_COLOR,
	/** Swap foreground and background colors */
	CFB_OP_SWAP_FG_BG_COLOR,
	/** Add command buffer */
	CFB_OP_SET_COMMAND_BUFFER,
	/**
	 * Fills the drawing surface with the background color.
	 * This is internal operation. Don't use it.
	 */
	CFB_OP_FILL = 0xFE,
	CFB_OP_TERMINATE = 0xFF,
};

enum init_commands {
	CFB_INIT_CMD_SET_FONT,
	CFB_INIT_CMD_SET_KERNING,
	CFB_INIT_CMD_SET_FG_COLOR,
	CFB_INIT_CMD_SET_BG_COLOR,
	CFB_INIT_CMD_FILL,
	CFB_INIT_CMD_SET_COMMAND_BUFFER,
	NUM_OF_INIT_CMDS,
};

struct cfb_font {
	const void *data;
	enum cfb_font_caps caps;
	uint8_t width;
	uint8_t height;
	uint8_t first_char;
	uint8_t last_char;
};

struct cfb_position {
	int16_t x;
	int16_t y;
};

/**
 * @brief CFB operation structure
 */
struct cfb_command_param {
	union {
		/**
		 * @brief Parameter for CFB_OP_INVERT_AREA operation
		 */
		struct {
			int16_t x;
			int16_t y;
			uint16_t w;
			uint16_t h;
		} invert_area;

		/**
		 * @brief Parameter for CFB_OP_DRAW_POINT, CFB_OP_DRAW_LINE
		 *        and CFB_DRAW_RECT operation
		 */
		struct {
			struct cfb_position start;
			struct cfb_position end;
		} draw_figure;

		/**
		 * @brief Parameter for CFB_OP_PRINT and CFB_OP_DRAW_TEXT
		 *        operation
		 */
		struct {
			const char *str;
			struct cfb_position pos;
		} draw_text;

		/**
		 * @brief Parameter for CFB_OP_SET_FONT operation
		 */
		struct {
			uint8_t font_idx;
		} set_font;

		/**
		 * @brief Parameter for CFB_OP_SET_KERNING operation
		 */
		struct {
			int8_t kerning;
		} set_kerning;

		/**
		 * @brief Parameter for CFB_OP_SET_FG_COLOR and
		 *        CFB_OP_SET_BG_COLOR operation
		 */
		union {
			struct {
				uint8_t blue;
				uint8_t green;
				uint8_t red;
				uint8_t alpha;
			};
			uint32_t color;
		} set_color;

		/**
		 * @brief Parameter for CFB_OP_SET_COMMAND_BUFFER
		 */
		struct {
			struct cfb_commandbuffer *cmdbuf;
		} cmd_buffer;
	};

	/** Type of operation */
	uint8_t op;
} __packed;

struct cfb_command {
	/** Linked-list node */
	sys_snode_t node;
	struct cfb_command_param param;
};

/**
 * A framebuffer definition in CFB.
 *
 * This is a private definition.
 * Don't access directly to members of this structure.
 */
struct cfb_framebuffer {
	/** Pointer to a buffer in RAM */
	uint8_t *buf;

	/** Size of the framebuffer */
	uint32_t size;

	/** Display pixel format */
	enum display_pixel_format pixel_format;

	/** Display screen info */
	enum display_screen_info screen_info;

	/** Framebuffer width in pixels */
	uint16_t width;

	/** Framebuffer height in pixels */
	uint16_t height;

	/**
	 * Bytes per pixel(bpp) or number of pixels per tile(ppt).
	 * This measn bpp if this value is positive, if not so,
	 * this means ppt.
	 */
	int8_t bpp_ppt;

	/**
	 * @param disp Pointer to display instance
	 * @return 0 on success, negative value otherwise
	 */
	int (*finalize)(const struct cfb_framebuffer *disp, int16_t x, int16_t y, uint16_t width,
			uint16_t height);

	/**
	 * @param disp Pointer to display instance
	 * @param clear_display Clear the display as well
	 * @return 0 on success, negative value otherwise
	 */
	int (*clear)(const struct cfb_framebuffer *disp, bool clear_display);

	/**
	 * @param disp Pointer to display instance
	 * @param cmd Pointer to command for append to the list
	 * @param append_buffer Store in buffer if true
	 * @return 0 on success, negative value otherwise
	 */
	int (*append_command)(const struct cfb_framebuffer *disp, struct cfb_command *cmd,
			      bool append_buffer);

	/**
	 * @param disp Pointer to display instance
	 * @param x x-position of transfer region
	 * @param y y-position of transfer region
	 * @param w width of transfer region
	 * @param h height of transfer region
	 */
	int (*transfer_buffer)(const struct cfb_framebuffer *disp, int16_t x, int16_t y, uint16_t w,
			       uint16_t h);
};

/**
 * CFB command buffer .
 *
 * This is a private definition.
 * Don't access directly to members of this structure.
 */
struct cfb_commandbuffer {
	/** Pointer to a buffer */
	uint8_t *buf;

	/** Size of the commandbuffer*/
	uint32_t size;

	/** Current position of buffer */
	size_t pos;
};

/**
 * A display definition in CFB.
 *
 * This is a private definition.
 * Don't access directly to members of this structure.
 */
struct cfb_display {
	/** Framebuffer */
	struct cfb_framebuffer fb;

	/** Command buffer */
	struct cfb_commandbuffer cmdbuf;

	/** Pointer to device */
	const struct device *dev;

	/** Resolution of a framebuffer in pixels in X direction */
	uint16_t x_res;

	/** Resolution of a framebuffer in pixels in Y direction */
	uint16_t y_res;

	/** Current font index */
	uint8_t font_idx;

	/** Current font kerning */
	int8_t kerning;

	/** Current foreground color */
	uint32_t fg_color;

	/** Current background color */
	uint32_t bg_color;

	/** Linked list for queueing commands */
	sys_slist_t cmd_list;

	/**  Variable for storing commands that are used every time rendering initializing */
	struct cfb_command init_cmds[NUM_OF_INIT_CMDS];
};

/**
 * @brief Append command to list
 *
 * Append command to list. Execute commands in the list at finalizing.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param cmd Pointer to command for append to the list
 *
 * @retval 0 on success
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_append_command(const struct cfb_framebuffer *fb, struct cfb_command *cmd)
{
	return fb->append_command(fb, cmd, false);
}

/**
 * @brief Print a string into the framebuffer.
 *
 * This function copies the argument string into a buffer.
 * The argument string can be discarded immediately, but it uses buffer memory.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_print(const struct cfb_framebuffer *fb, const char *const str, int16_t x,
			    int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_PRINT(str, x, y);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Print a string reference into the framebuffer.
 *
 * This function copies the argument string into a buffer.
 * The argument string can be discarded immediately, but it uses buffer memory.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_print_ref(const struct cfb_framebuffer *fb, const char *const str, int16_t x,
				int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_PRINT_REF(str, x, y);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Print a string into the framebuffer.
 * For compare to cfb_print, cfb_draw_text accept non tile-aligned coords
 * and not line wrapping.
 *
 * This function copies the argument string into a buffer.
 * The argument string can be discarded immediately, but it uses buffer memory.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_text(const struct cfb_framebuffer *fb, const char *const str, int16_t x,
				int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_TEXT(str, x, y);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Print a string reference into the framebuffer.
 * For compare to cfb_print, cfb_draw_text accept non tile-aligned coords
 * and not line wrapping.
 *
 * This function copies the argument string into a buffer.
 * The argument string can be discarded immediately, but it uses buffer memory.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_text_ref(const struct cfb_framebuffer *fb, const char *const str,
				    int16_t x, int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_TEXT_REF(str, x, y);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Draw a point.
 *
 * Draw a point to specified coordination.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param pos Position of the point
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_point(const struct cfb_framebuffer *fb, const struct cfb_position *pos)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_POINT(str, pos->x, pos->y);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Draw a line.
 *
 * Draw a line between specified two points.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param start Start position of the line
 * @param end End position of the line
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_line(const struct cfb_framebuffer *fb, const struct cfb_position *start,
				const struct cfb_position *end)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_LINE(start->x, start->y, end->x, end->y);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Draw a rectangle.
 *
 * Draw a rectangle that formed by specified start and end points.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param start Top-Left position of the rectangle
 * @param end Bottom-Right position of the rectangle
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_draw_rect(const struct cfb_framebuffer *fb, const struct cfb_position *start,
				const struct cfb_position *end)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_RECT(start->x, start->y, end->x, end->y);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Clear command buffer and framebuffer.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param clear_display Clear the display as well
 *
 * @retval 0 on success
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_clear(const struct cfb_framebuffer *fb, bool clear_display)
{
	return fb->clear(fb, clear_display);
}

/**
 * @brief Inverts foreground and background colors
 *
 * @param fb Pointer to framebuffer to rendering
 *
 * @return 0 on success, negative value otherwise
 */
static inline int cfb_invert(const struct cfb_framebuffer *fb)
{
	struct cfb_command swap_fg_bg_cmd = CFB_OP_INIT_SWAP_FG_BG_COLOR();
	struct cfb_command invert_area_cmd = CFB_OP_INIT_INVERT_AREA(0, 0, UINT16_MAX, UINT16_MAX);
	int err;

	err = fb->append_command(fb, &swap_fg_bg_cmd, true);
	if (err) {
		return err;
	}

	return fb->append_command(fb, &invert_area_cmd, true);
}

/**
 * @brief Invert Pixels in selected area.
 *
 * Invert bits of pixels in selected area.
 *
 * This function returns the error -ENOBUFS if the command buffer does not have
 * enough space.
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param x Position in X direction of the beginning of area
 * @param y Position in Y direction of the beginning of area
 * @param width Width of area in pixels
 * @param height Height of area in pixels
 *
 * @return 0 on success, negative value otherwise
 */
static inline int cfb_invert_area(const struct cfb_framebuffer *fb, int16_t x, int16_t y,
				  uint16_t width, uint16_t height)
{
	struct cfb_command cmd = CFB_OP_INIT_INVERT_AREA(x, y, width, height);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Finalize framebuffer.
 *
 * Execute commands and write image to display.
 * If the buffer is small, it will draw and transfer the screen multiple
 * times with the size of the buffer.
 * It will work with less buffer, but drawing will be slower.
 *
 * @param fb Pointer to framebuffer to rendering
 *
 * @return 0 on success, negative value otherwise
 */
static inline int cfb_finalize(const struct cfb_framebuffer *fb)
{
	return fb->finalize(fb, 0, 0, UINT16_MAX, UINT16_MAX);
}

static inline int cfb_finalize_area(const struct cfb_framebuffer *fb, int16_t x, int16_t y,
				    uint16_t width, uint16_t height)
{
	return fb->finalize(fb, x, y, width, height);
}

/**
 * @brief Get display parameter.
 *
 * @param disp Pointer to display instance
 * @param cfb_display_param One of the display parameters
 *
 * @return Display parameter value
 */
int cfb_get_display_parameter(const struct cfb_display *disp, enum cfb_display_param);

/**
 * @brief Set font.
 *
 * Select font that is used by cfb_draw_text and cfb_print for drawing.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 * The setting keeps even after finalize is executed.
 *
 * @param fb Pointer to framebuffer instance
 * @param idx Font index
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_font(const struct cfb_framebuffer *fb, uint8_t idx)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_FONT(idx);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Set font kerning.
 *
 * Set spacing between individual letters.
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 * The setting keeps even after finalize is executed.
 *
 * @param fb Pointer to framebuffer instance
 * @param kerning Spacing between each characters in pixels
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_kerning(const struct cfb_framebuffer *fb, int8_t kerning)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_KERNING(kerning);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Set foreground color.
 *
 * Set foreground color with RGBA values in 32-bit color representation
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 * The setting keeps even after finalize is executed.
 *
 * @param fb Pointer to framebuffer instance
 * @param r The red component of the foreground color in 32-bit color
 * @param g The green component of the foreground color in 32-bit color
 * @param b The blue component of the foreground color in 32-bit color
 * @param a The alpha channel of the foreground color in 32-bit color
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_fg_color(const struct cfb_framebuffer *fb, uint8_t r, uint8_t g,
				   uint8_t b, uint8_t a)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_FG_COLOR(r, g, b, a);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Set background color.
 *
 * Set background color with RGBA values in 32-bit color representation
 *
 * This function simply stores the drawing command in the buffer,
 * and the actual drawing will be done when cfb_finalize is executed.
 * The setting keeps even after finalize is executed.
 *
 * @param fb Pointer to framebuffer instance
 * @param r The red component of the foreground color in 32-bit color
 * @param g The green component of the foreground color in 32-bit color
 * @param b The blue component of the foreground color in 32-bit color
 * @param a The alpha channel of the foreground color in 32-bit color
 *
 * @retval 0 on success
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_bg_color(const struct cfb_framebuffer *fb, uint8_t r, uint8_t g,
				   uint8_t b, uint8_t a)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_BG_COLOR(r, g, b, a);

	return fb->append_command(fb, &cmd, true);
}

/**
 * @brief Get font size.
 *
 * Get width and height of font that is specified by idx.
 *
 * @param idx Font index
 * @param width Pointers to the variable where the font width will be stored.
 * @param height Pointers to the variable where the font height will be stored.
 *
 * @retval 0 on success
 * @retval -EINVAL If you specify idx for a font that does not exist
 */
int cfb_get_font_size(uint8_t idx, uint8_t *width, uint8_t *height);

/**
 * @brief Get number of fonts.
 *
 * @return number of fonts
 */
int cfb_get_numof_fonts(void);

/**
 * @brief Initialize display.
 *
 *
 * @param disp Pointer to cfb_display structure
 * @param dev Pointer to device that use to displaying
 * @param xferbuf Pointer to image transfer buffer
 * @param xferbuf_size Image transfer buffer size.
 *                     You must allocate at least 1 pixel of memory for the transfer buffer.
 *                     Buffers are used line by line. Allocating memory in multiples of.
 *                     the X-resolution eliminates waste.
 * @param cmdbuf Pointer to command buffer
 * @param cmdbuf_size Command buffer size.
 *                    If the command buffer is not allocated, the drawing API will
 *                    result in an error -ENOBUFS. Even in this case, you can use
 *                    cfb_command_append to draw with command variables managed
 *                    by the user.
 * @return 0 on success
 */
int cfb_display_init(struct cfb_display *disp, const struct device *dev, void *xferbuf,
		     size_t xferbuf_size, void *cmdbuf, size_t cmdbuf_size);

/**
 * @brief Allocate and initialize display object.
 *
 * @param dev Pointer to device that use to displaying
 * @param xferbuf_size Size of image transfer buffer to allocate
 * @param cmdbuf_size Size of command buffer to allocate
 *
 * @retval NULL If memory allocation failed.
 * @retval Non-NULL on success
 */
struct cfb_display *cfb_display_alloc(const struct device *dev, size_t xferbuf_size,
				      size_t cmdbuf_size);

/**
 * @brief Deinitialize Character Framebuffer.
 *
 * @param disp Pointer to display instance to deinitialize
 */
void cfb_display_deinit(struct cfb_display *disp);

/**
 * @brief Release allocated cfb_display object
 *
 * Release cfb_display object that allocated by
 * cfb_display_alloc_init.
 *
 * @param disp Pointer to display object that allocated by
 *             cfb_display_alloc_init.
 */
void cfb_display_free(struct cfb_display *disp);

/**
 * @brief Get framebuffer subordinate to cfb_display.
 *
 * @param disp Pointer to display instance to retrieve framebuffer.
 */
struct cfb_framebuffer *cfb_display_get_framebuffer(struct cfb_display *disp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __CFB_H__ */
