/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Compact FrameBuffer (CFB) API
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
 * @brief The Compact Framebuffer (CFB) subsystem provides fundamental graphics
 * functionality with low memory consumption and footprint size.
 * @defgroup compact_framebuffer_subsystem Compact FrameBuffer subsystem
 * @ingroup utilities
 * @{
 */

/**
 * @brief Macro for creating a font entry.
 *
 * @param _name   Name of the font entry.
 * @param _width  Width of the font in pixels.
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
 * Initializer macro for draw-point operation.
 *
 * @param _x x position of the point.
 * @param _y y position of the point.
 */
#define CFB_OP_INIT_DRAW_POINT(_x, _y)                                                             \
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
 * Initializer macro for draw-line operation.
 *
 * @param _s_x Start x position of the line.
 * @param _s_y Start y position of the line.
 * @param _e_x End x position of the line.
 * @param _e_y End y position of the line.
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
 * Initializer macro for draw-rect operation.
 *
 * @param _s_x Top-Left X position of the rectangle.
 * @param _s_y Top-Left X position of the rectangle.
 * @param _e_x Bottom-Right Y position of the rectangle.
 * @param _e_y Top-Left Y position of the rectangle.
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
 * Initializer macro for draw-text operation.
 *
 * @param _str Strings to display.
 * @param _x X position of the beginning of the strings.
 * @param _y Y position of the beginning of the strings.
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
 * Initializer macro for print operation.
 *
 * @param _str Strings to display.
 * @param _x X position of the beginning of the strings.
 * @param _y Y position of the beginning of the strings.
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
 * Initializer macro for draw-text-ref operation.
 *
 * @param _str Strings to display.
 * @param _x X position of the beginning of the strings.
 * @param _y Y position of the beginning of the strings.
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
 * Initializer macro for print-ref operation.
 *
 * @param _str Strings to display.
 * @param _x X position of the beginning of the strings.
 * @param _y Y position of the beginning of the strings.
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
 * Initializer macro for invert-area operation.
 *
 * @param _x X Position of the beginning of the area.
 * @param _y Y Position of the beginning of the area.
 * @param _w Width of area in pixels.
 * @param _h Height of area in pixels.
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
 * Initializer macro for set-font operation.
 *
 * @param _font_idx Font index.
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
 * Initializer macro for set-kerning operation.
 *
 * @param _kerning Spacing between each character in pixels.
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
 * Initializer macro for set-fgcolor operation.
 *
 * @param r The red component of the foreground color in 32-bit color representation.
 * @param g The green component of the foreground color in 32-bit color representation.
 * @param b The blue component of the foreground color in 32-bit color representation.
 * @param a The alpha channel of the foreground color in 32-bit color representation.
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
 * Initializer macro for set-bgcolor operation.
 *
 * @param r The red component of the foreground color in 32-bit color representation.
 * @param g The green component of the foreground color in 32-bit color representation.
 * @param b The blue component of the foreground color in 32-bit color representation.
 * @param a The alpha channel of the foreground color in 32-bit color representation.
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
 * Initializer macro for swap fg/bg color operation.
 */
#define CFB_OP_INIT_SWAP_FG_BG_COLOR()                                                             \
	{                                                                                          \
		.param = {                                                                         \
			.op = CFB_OP_SWAP_FG_BG_COLOR,                                             \
		},                                                                                 \
	}

/**
 * Definitions of rendering operation types.
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
	/** Set a font for text rendering */
	CFB_OP_SET_FONT,
	/** Set a kerning for text rendering */
	CFB_OP_SET_KERNING,
	/** Set a foreground color */
	CFB_OP_SET_FG_COLOR,
	/** Set a background color */
	CFB_OP_SET_BG_COLOR,
	/** Swap foreground and background colors */
	CFB_OP_SWAP_FG_BG_COLOR,
	/** Fills the drawing surface with the background color */
	CFB_OP_FILL = 0xFE,
	CFB_OP_TERMINATE = UINT8_MAX,
};

enum cfb_display_param {
	CFB_DISPLAY_HEIGH		= 0,
	CFB_DISPLAY_WIDTH,
	CFB_DISPLAY_PPT,
	CFB_DISPLAY_ROWS,
	CFB_DISPLAY_COLS,
};

/**
 * 2-dimensional position
 */
struct cfb_position {
	/** X coordinate */
	int16_t x;
	/** Y coordinate */
	int16_t y;
};

/**
 * Rendering operation parameter.
 */
struct cfb_command_param {
	union {
		/**
		 * Parameter for CFB_OP_INVERT_AREA operation
		 */
		struct {
			int16_t x;
			int16_t y;
			uint16_t w;
			uint16_t h;
		} invert_area;

		/**
		 * Parameter for CFB_OP_DRAW_POINT, CFB_OP_DRAW_LINE
		 *        and CFB_DRAW_RECT operation
		 */
		struct {
			struct cfb_position start;
			struct cfb_position end;
		} draw_figure;

		/**
		 * Parameter for CFB_OP_PRINT and CFB_OP_DRAW_TEXT
		 *        operation
		 */
		struct {
			const char *str;
			struct cfb_position pos;
		} draw_text;

		/**
		 * Parameter for CFB_OP_SET_FONT operation
		 */
		struct {
			uint8_t font_idx;
		} set_font;

		/**
		 * Parameter for CFB_OP_SET_KERNING operation
		 */
		struct {
			int8_t kerning;
		} set_kerning;

		/**
		 * Parameter for CFB_OP_SET_FG_COLOR and
		 * CFB_OP_SET_BG_COLOR operation
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
	};

	/** Type of operation */
	enum cfb_operation op;
} __packed;

struct cfb_command {
	/** Linked-list node */
	sys_snode_t node;
	struct cfb_command_param param;
};

/** @cond INTERNAL_HIDDEN */

enum cfb_font_caps {
	CFB_FONT_MONO_VPACKED		= BIT(0),
	CFB_FONT_MONO_HPACKED		= BIT(1),
	CFB_FONT_MSB_FIRST		= BIT(2),
};

struct cfb_font {
	const void *data;
	enum cfb_font_caps caps;
	uint8_t width;
	uint8_t height;
	uint8_t first_char;
	uint8_t last_char;
};

enum init_commands {
	CFB_INIT_CMD_SET_FONT,
	CFB_INIT_CMD_SET_KERNING,
	CFB_INIT_CMD_SET_FG_COLOR,
	CFB_INIT_CMD_SET_BG_COLOR,
	CFB_INIT_CMD_FILL,
	NUM_OF_INIT_CMDS,
};

struct cfb_command_iterator {
	sys_snode_t *node;
	struct cfb_command_param *param;
	struct cfb_display *disp;
	struct cfb_command_iterator *(*next)(struct cfb_command_iterator *ite);
	bool (*is_last)(struct cfb_command_iterator *ite);
};

struct cfb_commandbuffer {
	uint8_t *buf;
	uint32_t size;
	size_t pos;
};

struct cfb_draw_settings {
	uint8_t font_idx;
	int8_t kerning;
	uint32_t fg_color;
	uint32_t bg_color;
};

/** @endcond */

/**
 * A parameter structure for ::cfb_display_init.
 */
struct cfb_display_init_param {
	/** Display device */
	const struct device *dev;

	/** Pointer to buffer that is used as a transfer buffer */
	uint8_t *transfer_buf;

	/** Size of the transfer_buf */
	uint32_t transfer_buf_size;

	/** Pointer to buffer that is used as a command buffer */
	uint8_t *command_buf;

	/** Size of the command_buf */
	uint32_t command_buf_size;
};

/**
 * A representation of framebuffer.
 *
 * This struct is a private definition.
 * Do not have direct access to members of this structure.
 */
struct cfb_framebuffer {
	/**
	 * @private
	 * Pointer to a buffer in RAM
	 */
	uint8_t *buf;

	/**
	 * @private
	 * Size of the buf
	 */
	uint32_t size;

	/**
	 * @private
	 * Display pixel format
	 */
	enum display_pixel_format pixel_format;

	/**
	 * @private
	 * Display screen info
	 */
	enum display_screen_info screen_info;

	/**
	 * @private
	 * Framebuffer width in pixels.
	 */
	uint16_t width;

	/**
	 * @private
	 * Framebuffer height in pixels.
	 */
	uint16_t height;

	/**
	 * @private
	 * The top-left of partial-framebuffer.
	 */
	struct cfb_position pos;

	/**
	 * @private
	 * Resolution of a framebuffer in pixels in X direction.
	 */
	struct cfb_position res;

	/**
	 * @private
	 *
	 * @param disp A display instance.
	 * @return 0 on succeeded, negative value otherwise
	 */
	int (*finalize)(struct cfb_framebuffer *disp, int16_t x, int16_t y, uint16_t width,
			uint16_t height);

	/**
	 * @private
	 *
	 * @param disp A display instance.
	 * @param clear_display Clear the display as well.
	 * @return 0 on succeeded, negative value otherwise.
	 */
	int (*clear)(struct cfb_framebuffer *disp, bool clear_display);

	/**
	 * @private
	 *
	 * @param disp A display instance.
	 * @param cmd A command for append to the list.
	 * @return 0 on succeeded, negative value otherwise.
	 */
	int (*append_command)(struct cfb_framebuffer *disp, struct cfb_command *cmd);

	/**
	 * @private
	 *
	 * @param disp A display instance.
	 * @return Pointer to the iterator, Null on unsucceeded.
	 */
	struct cfb_command_iterator *(*init_iterator)(struct cfb_framebuffer *disp);
};

/**
 * A representation of display.
 *
 * This struct is a private definition.
 * Do not have direct access to members of this structure.
 */
struct cfb_display {
	/**
	 * @private
	 * Framebuffer
	 */
	struct cfb_framebuffer fb;

	/**
	 * @private
	 * Command buffer
	 */
	struct cfb_commandbuffer cmd;

	/**
	 * @private
	 * Pointer to device
	 */
	const struct device *dev;

	/**
	 * @private
	 * Current draw settings
	 */
	struct cfb_draw_settings settings;

	/**
	 * @private
	 * Linked list for queueing commands
	 */
	sys_slist_t cmd_list;

	/**
	 * @private
	 * Variable for storing commands that run on every time rendering
	 */
	struct cfb_command init_cmds[NUM_OF_INIT_CMDS];

	/**
	 * @private
	 * iterator for process commands
	 */
	struct cfb_command_iterator iterator;
};

/**
 * Append a command to a list.
 *
 * Append a command to an internal list structure. Execute commands in the list at finalizing.
 *
 * @param fb A framebuffer to rendering.
 * @param cmd A command for append to the list.
 *
 * @retval 0 on succeeded
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_append_command(struct cfb_framebuffer *fb, struct cfb_command *cmd)
{
	return fb->append_command(fb, cmd);
}

/**
 * Print strings.
 *
 * This function copies the argument strings into a buffer.
 * The argument strings can be discarded immediately, but it uses buffer memory.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param str Strings to display.
 * @param x X Position of the beginning of the strings.
 * @param y Y Position of the beginning of the strings.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_print(struct cfb_framebuffer *fb, const char *const str, int16_t x, int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_PRINT(str, x, y);

	return fb->append_command(fb, &cmd);
}

/**
 * Print strings by reference.
 *
 * This function does not copy the argument strings into a buffer.
 * The strings referenced by the argument must exist until ::cfb_finalize is finished.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param str Strings to display.
 * @param x X Position of the beginning of the strings.
 * @param y Y Position of the beginning of the strings.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_print_ref(struct cfb_framebuffer *fb, const char *const str, int16_t x,
				int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_PRINT_REF(str, x, y);

	return fb->append_command(fb, &cmd);
}

/**
 * Draw strings.
 *
 * For comparison to ::cfb_print, ::cfb_draw_text accepts non-tile-aligned coords
 * and not line wrapping.
 *
 * This function copies the argument strings into a buffer.
 * The argument strings can be discarded immediately, but it uses buffer memory.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param str Strings to display.
 * @param x X Position of the beginning of the strings.
 * @param y Y Position of the beginning of the strings.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space.
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_text(struct cfb_framebuffer *fb, const char *const str, int16_t x,
				int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_TEXT(str, x, y);

	return fb->append_command(fb, &cmd);
}

/**
 * Draw strings with strings reference.
 *
 * For comparison to cfb_print, ::cfb_draw_text accepts non-tile-aligned coords
 * and not line wrapping.
 *
 * This function does not copy the argument strings into a buffer.
 * The strings referenced by the argument need to exist until ::cfb_finalize is finished.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param str Strings to display.
 * @param x X Position of the beginning of the strings.
 * @param y Y Position of the beginning of the strings.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_text_ref(struct cfb_framebuffer *fb, const char *const str, int16_t x,
				    int16_t y)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_TEXT_REF(str, x, y);

	return fb->append_command(fb, &cmd);
}

/**
 * Draw a point.
 *
 * Draw a point to specified coordination.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param pos The position of the point.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_point(struct cfb_framebuffer *fb, const struct cfb_position *pos)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_POINT(pos->x, pos->y);

	return fb->append_command(fb, &cmd);
}

/**
 * Draw a line.
 *
 * Draw a line between the specified two points.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param start The starting point of the line.
 * @param end The ending point of the line.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_draw_line(struct cfb_framebuffer *fb, const struct cfb_position *start,
				const struct cfb_position *end)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_LINE(start->x, start->y, end->x, end->y);

	return fb->append_command(fb, &cmd);
}

/**
 * Draw a rectangle.
 *
 * Draw a rectangle that is formed by specified start and end points.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param start The Top-Left position of the rectangle.
 * @param end The Bottom-Right position of the rectangle.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_draw_rect(struct cfb_framebuffer *fb, const struct cfb_position *start,
				const struct cfb_position *end)
{
	struct cfb_command cmd = CFB_OP_INIT_DRAW_RECT(start->x, start->y, end->x, end->y);

	return fb->append_command(fb, &cmd);
}

/**
 * Clear command buffer and framebuffer.
 *
 * @param fb A framebuffer to rendering.
 * @param clear_display Clear the display as well.
 *
 * @retval 0 on succeeded
 * @retval -errno Negative errno for other failures.
 */
static inline int cfb_clear(struct cfb_framebuffer *fb, bool clear_display)
{
	return fb->clear(fb, clear_display);
}

/**
 * Inverts foreground and background colors.
 *
 * @param fb A framebuffer to rendering.
 *
 * @retval 0 on succeeded
 * @retval -errno Negative errno for other failures
 */
static inline int cfb_invert(struct cfb_framebuffer *fb)
{
	struct cfb_command swap_fg_bg_cmd = CFB_OP_INIT_SWAP_FG_BG_COLOR();
	struct cfb_command invert_area_cmd = CFB_OP_INIT_INVERT_AREA(0, 0, UINT16_MAX, UINT16_MAX);
	int err;

	err = fb->append_command(fb, &swap_fg_bg_cmd);
	if (err) {
		return err;
	}

	return fb->append_command(fb, &invert_area_cmd);
}

/**
 * Invert the color in the selected area.
 *
 * Invert bits of pixels in the selected area.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to rendering.
 * @param x X Position of the beginning of the area.
 * @param y Y Position of the beginning of the area.
 * @param width Width of area in pixels.
 * @param height Height of area in pixels.
 *
 * @return 0 on succeeded, negative value otherwise
 */
static inline int cfb_invert_area(struct cfb_framebuffer *fb, int16_t x, int16_t y, uint16_t width,
				  uint16_t height)
{
	struct cfb_command cmd = CFB_OP_INIT_INVERT_AREA(x, y, width, height);

	return fb->append_command(fb, &cmd);
}

/**
 * Finalize framebuffer.
 *
 * If the buffer is smaller than the screen size, this function executes the command
 * in the command buffer to draw and transfer the image.
 * If the buffer is larger than the screen size, the drawing has already been completed
 * and only the data is transferred.
 *
 * @param fb A framebuffer to rendering.
 *
 * @return 0 on succeeded, negative value otherwise
 */
static inline int cfb_finalize(struct cfb_framebuffer *fb)
{
	return fb->finalize(fb, 0, 0, UINT16_MAX, UINT16_MAX);
}

/**
 * Finalize partial framebuffer.
 *
 * If the buffer is smaller than the screen size, this function executes the command
 * in the command buffer to draw and transfer the image.
 * If the buffer is larger than the screen size, the drawing has already been completed
 * and only the data is transferred.
 *
 * @param fb A framebuffer to rendering.
 * @param x The X position of the starting point of the area for drawing and data transfer.
 * @param y The Y position of the starting point of the area for drawing and data transfer.
 * @param width The width of the drawing and data transfer area.
 * @param height The height of the drawing and data transfer area.
 *
 * @return 0 on succeeded, negative value otherwise
 */
static inline int cfb_finalize_area(struct cfb_framebuffer *fb, int16_t x, int16_t y,
				    uint16_t width, uint16_t height)
{
	return fb->finalize(fb, x, y, width, height);
}

/**
 * @brief Get the display parameter.
 *
 * @param disp A display instance.
 * @param cfb_display_param One of the display parameters.
 *
 * @return Display parameter value
 */
int cfb_get_display_parameter(const struct cfb_display *disp, enum cfb_display_param);

/**
 * @brief Select a font.
 *
 * Select a font that is used by cfb_draw_text and cfb_print for drawing.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to set.
 * @param idx Font index.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_font(struct cfb_framebuffer *fb, uint8_t idx)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_FONT(idx);

	return fb->append_command(fb, &cmd);
}

/**
 * Set font kerning space.
 *
 * Set spacing between individual letters.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to set.
 * @param kerning Spacing between each character in pixels.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_kerning(struct cfb_framebuffer *fb, int8_t kerning)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_KERNING(kerning);

	return fb->append_command(fb, &cmd);
}

/**
 * Set foreground color.
 *
 * Set foreground color with RGBA values in 32-bit color representation.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to set.
 * @param r The red component of the foreground color in 32-bit color representation.
 * @param g The green component of the foreground color in 32-bit color representation.
 * @param b The blue component of the foreground color in 32-bit color representation.
 * @param a The alpha channel of the foreground color in 32-bit color representation.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_fg_color(struct cfb_framebuffer *fb, uint8_t r, uint8_t g, uint8_t b,
				   uint8_t a)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_FG_COLOR(r, g, b, a);

	return fb->append_command(fb, &cmd);
}

/**
 * Set background color.
 *
 * Set background color with RGBA values in 32-bit color representation.
 *
 * This function immediately draws to the buffer if the buffer is large enough
 * to store the entire screen. Otherwise, store the command in the command buffer.
 *
 * @param fb A framebuffer to set.
 * @param r The red component of the foreground color in 32-bit color representation.
 * @param g The green component of the foreground color in 32-bit color representation.
 * @param b The blue component of the foreground color in 32-bit color representation.
 * @param a The alpha channel of the foreground color in 32-bit color representation.
 *
 * @retval 0 on succeeded
 * @retval -ENOBUFS The command buffer does not have enough space
 * @retval -errno  Negative errno for other failures.
 */
static inline int cfb_set_bg_color(struct cfb_framebuffer *fb, uint8_t r, uint8_t g, uint8_t b,
				   uint8_t a)
{
	struct cfb_command cmd = CFB_OP_INIT_SET_BG_COLOR(r, g, b, a);

	return fb->append_command(fb, &cmd);
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
 * Initialize display
 *
 * Specify the buffer via param.
 * If transfer_buf specifies a buffer large enough to store the entire screen's data,
 * the drawing operation is performed immediately without using the command buffer.
 * If the buffer specified by transfer_buf is not large enough to store the entire screen's data,
 * the drawing operations are stored in the command buffer and executed by calling cfb_finalize.
 * In this case, the screen is displayed by repeatedly drawing and transferring the area
 * that can be contained in the buffer size multiple times.
 * Therefore, command_buf specifies a buffer large enough to hold the drawing operation
 * you want to perform.
 *
 * @param disp A display instance to initialize.
 * @param param Pointer to display initialize parameter.
 *
 * @return 0 on succeeded
 */
int cfb_display_init(struct cfb_display *disp, const struct cfb_display_init_param *param);

/**
 * Allocate a full-screen buffer and initialize a display instance.
 *
 * @param dev A display device which is used to displaying.
 *
 * @retval NULL If memory allocation fails.
 * @retval Non-NULL on succeeded
 */
struct cfb_display *cfb_display_alloc(const struct device *dev);

/**
 * Deinitialize display instance.
 *
 * @param disp A display instance to deinitialize.
 */
void cfb_display_deinit(struct cfb_display *disp);

/**
 * Release an allocated display instance.
 *
 * @param disp A display instance that was allocated by ::cfb_display_alloc.
 */
void cfb_display_free(struct cfb_display *disp);

/**
 * Get a framebuffer subordinate to the given display.
 *
 * @param disp A display instance to retrieve framebuffer.
 * @return A framebuffer is subordinate to the given display.
 */
struct cfb_framebuffer *cfb_display_get_framebuffer(struct cfb_display *disp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __CFB_H__ */
