/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Capabilities header file.
 */

#ifndef __MP_CAPS_H__
#define __MP_CAPS_H__

#include <stddef.h>

#include "mp_object.h"
#include "mp_structure.h"

/**
 * @defgroup mp_caps
 * @brief Media Capabilities
 *
 * CAPS is an object represents the supported media formats (e.g., audio, video) and
 * data formats (e.g., codec, resolution, framerate) of an element. Each caps
 * object consists of one or more @ref mp_cap_structure, where each of them describes
 * a specific capability.
 *
 * @{
 */

/**
 * @brief Media type identifiers
 */
enum {
	/** Unknown media type */
	MP_MEDIA_UNKNOWN = 0,

	/** Audio in PCM format */
	MP_MEDIA_AUDIO_PCM,
	/** Video type including raw, bayer, compressed formats */
	MP_MEDIA_VIDEO,

	/** Maximum media type identifier */
	MP_MEDIA_END = UINT8_MAX,
};

/**
 * @brief Caps identifiers for media structures
 */
enum {
	/** Unknown caps field identifier */
	MP_CAPS_UNKNOWN = 0,

	/* Common fields used in display and video domains */
	/** Pixel format */
	MP_CAPS_PIXEL_FORMAT,
	/** Image width */
	MP_CAPS_IMAGE_WIDTH,
	/** Image height */
	MP_CAPS_IMAGE_HEIGHT,
	/** Frame rate */
	MP_CAPS_FRAME_RATE,

	/* Common fields used in Audio domain */
	/** Sample rate */
	MP_CAPS_SAMPLE_RATE,
	/** Bit width */
	MP_CAPS_BITWIDTH,
	/** Number of channels */
	MP_CAPS_NUM_OF_CHANNEL,
	/** Frame interval */
	MP_CAPS_FRAME_INTERVAL,
	/** Data buffer count */
	MP_CAPS_BUFFER_COUNT,
	/**
	 * The layout of channels within a buffer: interleaved (LRLRLRLR) and non-interleaved
	 * (LLLLRRRR)
	 */
	MP_CAPS_INTERLEAVED,

	/** End of the caps field and maximum caps field identifier */
	MP_CAPS_END = UINT8_MAX,
};

/**
 * @struct mp_caps
 *
 * @brief Represents a list of media capabilities.
 *
 */
struct mp_caps {
	struct mp_object object;     /**< Base object */
	sys_slist_t caps_structures; /**< List of capability structures */
};

/**
 * @struct mp_cap_structure
 * @brief structure used to hold a single capability structure.
 *
 * Each caps structure has:
 * - A media type ID: Specifies the nature (e.g., video, audio) and format type (e.g., raw,
 * compressed) of the media stream.
 * - A set of field–value pairs: Each pair represents a specific capability of the element.
 */
struct mp_cap_structure {
	sys_snode_t node;               /**< Linked list node */
	struct mp_structure *structure; /**< Pointer to the capability structure */
};

/** @brief Flag indicating ANY caps type */
#define MP_CAPS_FLAG_ANY 0x1

/** @brief Cast a generic object to mp_caps */
#define MP_CAPS(obj) ((struct mp_caps *)(obj))

/**
 * @brief Create a new @ref struct mp_caps of one caps structure with a media type ID and fields in
 * the same way as the @ref mp_structure_new
 *
 * @param media_type_id Media type ID
 * @param ... Variadic list of field definitions:
 *            (uint8_t field_id, int field_type, void *field_value),
 *            terminated by 0
 *
 * @return Pointer to newly created @ref struct mp_caps, or NULL on failure
 */
struct mp_caps *mp_caps_new(uint8_t media_type_id, ...);
/**
 * @brief Create new @ref struct mp_caps of type ANY.
 *
 * @return Pointer to newly created @ref struct mp_caps
 */
struct mp_caps *mp_caps_new_any(void);

/**
 * @brief Initialize a @ref struct mp_caps.
 *
 * @param caps Pointer to @ref struct mp_caps to initialize
 * @param flag Initialization flags (i.e., 0 if no flag or MP_CAPS_FLAG_ANY)
 */
void mp_caps_init(struct mp_caps *caps, uint8_t flag);

/**
 * @brief Append a structure to the @ref struct mp_caps.
 *
 * @param caps Pointer to @ref struct mp_caps
 * @param structure Pointer to @ref struct mp_structure to append
 *
 * @return true if the structure was successfully appended, false otherwise
 */
bool mp_caps_append(struct mp_caps *caps, struct mp_structure *structure);

/**
 * @brief Retrieve a @ref struct mp_structure from @ref struct mp_caps by index.
 *
 * @param caps Pointer to @ref struct mp_caps
 * @param index Index of the structure to retrieve
 *
 * @return Pointer to @ref struct mp_structure if found, NULL otherwise
 */
struct mp_structure *mp_caps_get_structure(struct mp_caps *caps, int index);

/**
 * @brief Check if the @ref struct mp_caps is ANY.
 *
 * @param caps Pointer to @ref struct mp_caps
 *
 * @return true if @ref struct mp_caps type is ANY, false otherwise
 */
bool mp_caps_is_any(struct mp_caps *caps);

/**
 * @brief Check if the @ref struct mp_caps are empty
 *
 * @param caps Pointer to @ref struct mp_caps
 *
 * @return true if caps is empty, false otherwise
 */
bool mp_caps_is_empty(struct mp_caps *caps);

/**
 * @brief Check if the @ref struct mp_caps are fixed
 *
 * @param caps Pointer to @ref struct mp_caps
 *
 * @return true if caps is fixed, false otherwise
 */
bool mp_caps_is_fixed(struct mp_caps *caps);

/**
 * @brief Check if two @ref struct mp_caps can intersect.
 *
 * Two caps can intersect if there exists at least one capability structure
 * in the first caps that can be intersected with a structure in the second caps.
 *
 * @param caps1 Pointer to first @ref struct mp_caps
 * @param caps2 Pointer to second @ref struct mp_caps
 *
 * @return true if the two caps can intersect, false otherwise
 */
bool mp_caps_can_intersect(struct mp_caps *caps1, struct mp_caps *caps2);

/**
 * @brief Perform intersection of two @ref struct mp_caps
 *
 * The intersection is performed by comparing each caps structure in
 * @p caps1 with those in @p caps2. Only structures with compatible fields
 * will be included in the resulting caps.
 *
 * @param caps1 Pointer to first @ref struct mp_caps
 * @param caps2 Pointer to second @ref struct mp_caps
 *
 * @return Pointer to intersected @ref struct mp_caps, or NULL if no intersection
 */
struct mp_caps *mp_caps_intersect(struct mp_caps *caps1, struct mp_caps *caps2);

/**
 * @brief Fixate @ref struct mp_caps.
 *
 * @param caps Pointer to @ref struct mp_caps to fixate
 *
 * @return Pointer to new fixated @ref struct mp_caps, or NULL if caps is ANY or EMPTY
 */
struct mp_caps *mp_caps_fixate(struct mp_caps *caps);

/**
 * @brief Duplicate a @ref struct mp_caps object.
 *
 * @param caps Pointer to @ref struct mp_caps to duplicate
 *
 * @return Pointer to new duplicated @ref struct mp_caps
 */
struct mp_caps *mp_caps_duplicate(struct mp_caps *caps);

/**
 * @brief Print @ref struct mp_caps to the console.
 *
 * @param caps Pointer to @ref struct mp_caps to print
 */
void mp_caps_print(struct mp_caps *caps);

/**
 * @brief Create a new reference to a @ref struct mp_caps object.
 *
 * @param caps Pointer to @ref struct mp_caps
 *
 * @return Pointer to the same @ref struct mp_caps with increased reference count
 */
static inline struct mp_caps *mp_caps_ref(struct mp_caps *caps)
{
	return (struct mp_caps *)mp_object_ref(&caps->object);
}

/**
 * @brief Release a reference to a @ref struct mp_caps object.
 *
 * @param caps Pointer to @ref struct mp_caps
 */
static inline void mp_caps_unref(struct mp_caps *caps)
{
	mp_object_unref(&caps->object);
}

/**
 * @brief Replace a @ref struct mp_caps pointer with a new one.
 *
 * @param target_caps Pointer to the target @ref struct mp_caps pointer
 * @param new_caps Pointer to the new @ref struct mp_caps object
 */
void mp_caps_replace(struct mp_caps **target_caps, struct mp_caps *new_caps);

/** @} */

#endif /* __MP_CAPS_H__ */
