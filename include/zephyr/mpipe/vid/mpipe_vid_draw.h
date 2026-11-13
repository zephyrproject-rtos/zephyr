/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Simple drawing helpers for video frames.
 * @ingroup mpipe_vid_overlay
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_DRAW_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_DRAW_H_

/**
 * @addtogroup mpipe_vid_overlay
 * @{
 */

#include <stdint.h>

/**
 * @brief A rectangle in pixel coordinates
 */
struct mpipe_vid_rect {
	/** Left edge */
	int32_t x0;
	/** Top edge */
	int32_t y0;
	/** Right edge */
	int32_t x1;
	/** Bottom edge */
	int32_t y1;
};

/**
 * @brief Draw the outline of a rectangle into a video frame
 *
 * Supported pixel formats: RGB565, BGRX32. Coordinates are clamped to the
 * frame.
 *
 * @param frame Frame pixels
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param pixfmt Frame pixel format, a VIDEO_PIX_FMT_* value
 * @param rect The rectangle
 * @param argb Color as 0xAARRGGBB; alpha is ignored
 * @param thickness Edge thickness in pixels
 *
 * @retval 0 Success
 * @retval -EINVAL @p frame or @p rect is NULL
 * @retval -ENOTSUP Unsupported pixel format
 */
int mpipe_vid_draw_rect(uint8_t *frame, uint16_t width, uint16_t height, uint32_t pixfmt,
			const struct mpipe_vid_rect *rect, uint32_t argb, uint8_t thickness);

/**
 * @brief Fill a rectangle of a frame with one color
 *
 * The rectangle is clamped to the frame.
 *
 * @param frame  Frame pixels to draw into
 * @param width  Frame width in pixels
 * @param height Frame height in pixels
 * @param pixfmt Frame pixel format, a VIDEO_PIX_FMT_* value
 * @param rect   Rectangle to fill, inclusive edges
 * @param argb   Color as 0xAARRGGBB; the alpha is ignored
 *
 * @retval 0        Success
 * @retval -EINVAL  @p frame or @p rect is NULL
 * @retval -ENOTSUP @p pixfmt is not a format the helpers draw into
 */
int mpipe_vid_draw_fill(uint8_t *frame, uint16_t width, uint16_t height, uint32_t pixfmt,
			const struct mpipe_vid_rect *rect, uint32_t argb);

/**
 * @brief Draw a line of text into a frame
 *
 * The glyphs come from the largest of the character framebuffer's built-in
 * bitmap fonts, drawn @p scale times their size, the text starting at
 * (@p x, @p y) and clipped to the frame. Characters the font lacks are
 * skipped.
 *
 * @param frame  Frame pixels to draw into
 * @param width  Frame width in pixels
 * @param height Frame height in pixels
 * @param pixfmt Frame pixel format, a VIDEO_PIX_FMT_* value
 * @param x      Left edge of the first glyph
 * @param y      Top edge of the glyphs
 * @param scale  Integer magnification of the glyphs, at least 1
 * @param argb   Text color as 0xAARRGGBB; the alpha is ignored
 * @param text   NUL-terminated ASCII text
 *
 * @retval 0        Success
 * @retval -EINVAL  @p frame or @p text is NULL, or @p scale is 0
 * @retval -ENOTSUP @p pixfmt is not drawable, or no built-in font is compiled in
 */
int mpipe_vid_draw_text(uint8_t *frame, uint16_t width, uint16_t height, uint32_t pixfmt, int32_t x,
			int32_t y, uint8_t scale, uint32_t argb, const char *text);

/**
 * @brief Size of a text as @ref mpipe_vid_draw_text would draw it
 *
 * @param scale  Integer magnification of the glyphs, at least 1
 * @param text   NUL-terminated ASCII text
 * @param w      Where to write the width in pixels, may be NULL
 * @param h      Where to write the height in pixels, may be NULL
 *
 * @retval 0        Success
 * @retval -ENOTSUP No built-in font is compiled in
 */
int mpipe_vid_draw_text_size(uint8_t scale, const char *text, uint32_t *w, uint32_t *h);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_DRAW_H_ */
