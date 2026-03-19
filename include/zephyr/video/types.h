/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_types
 * @brief Types used by both video drivers and subsystem
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_TYPES_H
#define ZEPHYR_INCLUDE_VIDEO_TYPES_H

/**
 * @brief Video type definitions.
 * @defgroup video_types Video Types
 * @since 4.4
 * @version 0.0.1
 * @ingroup video
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief video_buf_type enum
 *
 * Supported video buffer types of a video device.
 * The direction (input or output) is defined from the device's point of view.
 * Devices like cameras support only output type, encoders support only input
 * types while m2m devices like ISP, PxP support both input and output types.
 */
enum video_buf_type {
	/** input buffer type */
	VIDEO_BUF_TYPE_INPUT = 1,
	/** output buffer type */
	VIDEO_BUF_TYPE_OUTPUT = 2,
};

/**
 * @brief Video buffer structure
 *
 * Represents a video frame.
 */
struct video_buffer {
	/** Pointer to driver specific data. */
	/* It must be kept as first field of the struct if used for @ref k_fifo APIs. */
	void *driver_data;
	/** type of the buffer */
	enum video_buf_type type;
	/** type of the buffer memory, see @ref video_buf_memory */
	uint8_t memory;
	/** pointer to the start of the buffer. */
	uint8_t *buffer;
	/** index of the buffer in the video buffer pool */
	uint16_t index;
	/** size of the buffer in bytes. */
	uint32_t size;
	/** number of bytes occupied by the valid data in the buffer. */
	uint32_t bytesused;
	/** time reference in milliseconds at which the last data byte was
	 * actually received for input endpoints or to be consumed for output
	 * endpoints.
	 */
	uint32_t timestamp;
	/** Line offset within frame this buffer represents, from the
	 * beginning of the frame. This offset is given in pixels,
	 * so `line_offset` * `pitch` provides offset from the start of
	 * the frame in bytes.
	 */
	uint16_t line_offset;
};

/**
 * @brief Video format structure
 *
 * Used to configure frame format.
 */
struct video_format {
	/** type of the buffer */
	enum video_buf_type type;
	/** FourCC pixel format value (\ref video_pixel_formats) */
	uint32_t pixelformat;
	/** frame width in pixels. */
	uint32_t width;
	/** frame height in pixels. */
	uint32_t height;
	/**
	 * @brief line stride.
	 *
	 * This is the number of bytes that needs to be added to the address in the
	 * first pixel of a row in order to go to the address of the first pixel of
	 * the next row (>=width).
	 */
	uint32_t pitch;
	/**
	 * @brief size of the buffer in bytes, need to be set by the drivers
	 *
	 * For uncompressed formats, this is the size of the raw data buffer in bytes,
	 * which could be the whole raw image or a portion of the raw image in cases
	 * the receiver / dma supports it.
	 *
	 * For compressed formats, this is the maximum number of bytes required to
	 * hold a complete compressed frame, estimated for the worst case.
	 */
	uint32_t size;
};

/**
 * @brief Video format capability
 *
 * Used to describe a video endpoint format capability.
 */
struct video_format_cap {
	/** FourCC pixel format value (\ref video_pixel_formats). */
	uint32_t pixelformat;
	/** minimum supported frame width in pixels. */
	uint32_t width_min;
	/** maximum supported frame width in pixels. */
	uint32_t width_max;
	/** minimum supported frame height in pixels. */
	uint32_t height_min;
	/** maximum supported frame height in pixels. */
	uint32_t height_max;
	/** width step size in pixels. */
	uint16_t width_step;
	/** height step size in pixels. */
	uint16_t height_step;
};

/**
 * @brief Video format capabilities
 *
 * Used to describe video endpoint capabilities.
 */
struct video_caps {
	/** type of the buffer */
	enum video_buf_type type;
	/** list of video format capabilities (zero terminated). */
	const struct video_format_cap *format_caps;
	/** minimal count of video buffers to enqueue before being able to start
	 * the stream.
	 */
	uint8_t min_vbuf_count;
	/** requirement on the buffer alignment, in bytes */
	size_t buf_align;
};

/**
 * @struct video_control_range
 * @brief Video control range structure
 *
 * Describe range of a control including min, max, step and default values
 */
struct video_ctrl_range {
	/** control minimum value, inclusive */
	union {
		int32_t min;
		int64_t min64;
	};
	/** control maximum value, inclusive */
	union {
		int32_t max;
		int64_t max64;
	};
	/** control value step */
	union {
		int32_t step;
		int64_t step64;
	};
	/** control default value for VIDEO_CTRL_TYPE_INTEGER, _BOOLEAN, _MENU or
	 * _INTEGER_MENU, not valid for other types
	 */
	union {
		int32_t def;
		int64_t def64;
	};
};

/**
 * @brief Video signal result
 *
 * Identify video event.
 */
enum video_signal_result {
	VIDEO_BUF_DONE,    /**< Buffer is done */
	VIDEO_BUF_ABORTED, /**< Buffer is aborted */
	VIDEO_BUF_ERROR,   /**< Buffer is in error */
};

/**
 * @brief Video selection target
 *
 * Used to indicate which selection to query or set on a video device
 */
enum video_selection_target {
	/** Current crop setting */
	VIDEO_SEL_TGT_CROP,
	/** Crop bound (aka the maximum crop achievable) */
	VIDEO_SEL_TGT_CROP_BOUND,
	/** Native size of the input frame */
	VIDEO_SEL_TGT_NATIVE_SIZE,
	/** Current compose setting */
	VIDEO_SEL_TGT_COMPOSE,
	/** Compose bound (aka the maximum compose achievable) */
	VIDEO_SEL_TGT_COMPOSE_BOUND,
};

/**
 * @brief Description of a rectangle area.
 *
 * Used for crop/compose and possibly within drivers as well
 */
struct video_rect {
	/** left offset of selection rectangle */
	uint32_t left;
	/** top offset of selection rectangle */
	uint32_t top;
	/** width of selection rectangle */
	uint32_t width;
	/** height of selection rectangle */
	uint32_t height;
};

/**
 * @brief Video selection (crop / compose) structure
 *
 * Used to describe the query and set selection target on a video device
 */
struct video_selection {
	/** buffer type, allow to select for device having both input and output */
	enum video_buf_type type;
	/** selection target enum */
	enum video_selection_target target;
	/** selection target rectangle */
	struct video_rect rect;
};


/**
 * @brief Supported frame interval type of a video device.
 */
enum video_frmival_type {
	/** discrete frame interval type */
	VIDEO_FRMIVAL_TYPE_DISCRETE = 1,
	/** stepwise frame interval type */
	VIDEO_FRMIVAL_TYPE_STEPWISE = 2,
};

/**
 * @brief Video frame interval structure
 *
 * Used to describe a video frame interval.
 */
struct video_frmival {
	/** numerator of the frame interval */
	uint32_t numerator;
	/** denominator of the frame interval */
	uint32_t denominator;
};

/**
 * @brief Video frame interval stepwise structure
 *
 * Used to describe the video frame interval stepwise type.
 */
struct video_frmival_stepwise {
	/** minimum frame interval in seconds */
	struct video_frmival min;
	/** maximum frame interval in seconds */
	struct video_frmival max;
	/** frame interval step size in seconds */
	struct video_frmival step;
};

/**
 * @brief Video frame interval enumeration structure
 *
 * Used to describe the supported video frame intervals of a given video format.
 */
struct video_frmival_enum {
	/** frame interval index during enumeration */
	uint32_t index;
	/** video format for which the query is made */
	const struct video_format *format;
	/** frame interval type the device supports */
	enum video_frmival_type type;
	/** the actual frame interval */
	union {
		struct video_frmival discrete;
		struct video_frmival_stepwise stepwise;
	};
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_TYPES_H */
