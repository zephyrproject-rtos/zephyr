/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Structure: one media capability, held by value.
 * @ingroup mpipe_structure
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_STRUCTURE_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_STRUCTURE_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include <zephyr/mpipe/mpipe_value.h>

/**
 * @defgroup mpipe_structure Capability Structure
 * @ingroup mpipe_framework
 * @brief Fixed-size container describing one media capability.
 *
 * An mpipe_structure holds a set of fields, each an @ref mpipe_caps_field identifier
 * paired with an @ref mpipe_value. It is a fixed-size type held by value, so it
 * needs no allocation: the fields occupy the first slots of two parallel
 * arrays, one naming the field and one holding its value.
 *
 * A video capability such as
 *
 *     video, format=VIDEO_PIX_FMT_RGB565, width=[16, 1280, 2], height=[16, 720, 2]
 *
 * is built directly in caller storage:
 *
 * @code{.c}
 * struct mpipe_structure s;
 *
 * mpipe_structure_init_fields(&s, MPIPE_MEDIA_VIDEO,
 *     MPIPE_CAPS_PIXEL_FORMAT, MPIPE_TYPE_UINT, VIDEO_PIX_FMT_RGB565,
 *     MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT_RANGE, 16, 1280, 2,
 *     MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT_RANGE, 16, 720, 2,
 *     MPIPE_CAPS_END);
 * @endcode
 *
 * A range is written `[min, max, step]`, so the width above admits every
 * multiple of 2 from 16 to 1280.
 *
 * Two structures intersect when they have the same media type, carry at least
 * one field identifier in common, and every field they do share has
 * intersecting values. The result is the union of both: a shared field holds
 * the intersected value, and a field only one side carries passes through
 * unchanged. That last rule is what lets a constraint travel down a chain of
 * elements that do not themselves care about it.
 *
 * Pointers passed to this API must not be NULL unless the parameter is
 * documented otherwise.
 *
 * @{
 */

/**
 * @brief Media type a capability describes
 */
enum mpipe_media_type {
	/** Unknown media type, what a structure constraining nothing carries */
	MPIPE_MEDIA_UNKNOWN = 0,
	/** Audio in PCM format */
	MPIPE_MEDIA_AUDIO_PCM,
	/** Video, including raw, bayer and compressed formats */
	MPIPE_MEDIA_VIDEO,
	/** One past the last media type; not itself a usable media type */
	MPIPE_MEDIA_END,
};

/**
 * @brief Caps field identifiers
 *
 * A field of an @ref mpipe_structure is one of these identifiers paired with an
 * @ref mpipe_value. Each entry below names the value type that carries it. A
 * capability offering a span rather than one setting uses the matching range
 * type instead, MPIPE_TYPE_UINT_RANGE where the entry says MPIPE_TYPE_UINT, which is
 * how a device advertises every width from 16 to 1280 rather than a single one.
 * Negotiation then narrows the span, and fixation picks one value out of it.
 * MPIPE_CAPS_PIXEL_FORMAT and MPIPE_CAPS_INTERLEAVED, for example, are the exceptions:
 * neither has a meaningful range form, so both are always carried fixed.
 *
 * A caps field is not a property. A property configures one element, and
 * nothing else has to agree on it: a camera's gain or contrast, or the device
 * an element binds to. It is set on that element and used by that element.
 *
 * A caps field describes the data crossing a link, so both ends have to agree
 * on it before anything can flow, and that agreement is what negotiation
 * settles. The test when adding one is whether two different elements would
 * have to arrive at the same value for the stream to be correct. If only the
 * element that owns it cares, it is a property.
 *
 * MPIPE_CAPS_IMAGE_WIDTH passes that test: a camera, a converter and a display all
 * understand it and all have to agree on one number. Camera gain does not, as
 * the display neither knows nor cares what it is.
 *
 * These identifiers are shared by every domain, and one structure holds only
 * CONFIG_MPIPE_STRUCTURE_MAX_FIELDS of them at a time, so a new field earns its
 * place by meaning something to more than the element that introduced it.
 * Prefer one a whole domain agrees on, or one that spans domains the way
 * MPIPE_CAPS_FRAME_INTERVAL does for audio and video.
 *
 * Add it before MPIPE_CAPS_END and document its unit and value type the way the
 * entries below do, and give it a name in dump_field_names in mpipe_dump.c so a
 * dump can print it.
 */
enum mpipe_caps_field {
	/** Pixel format, as a VIDEO_PIX_FMT_* fourcc, MPIPE_TYPE_UINT */
	MPIPE_CAPS_PIXEL_FORMAT = 0,
	/** Image width in pixels, MPIPE_TYPE_UINT */
	MPIPE_CAPS_IMAGE_WIDTH,
	/** Image height in pixels, MPIPE_TYPE_UINT */
	MPIPE_CAPS_IMAGE_HEIGHT,
	/** Sampling frequency in Hz (audio), MPIPE_TYPE_UINT */
	MPIPE_CAPS_SAMPLE_RATE,
	/** Sample size in bits (audio), MPIPE_TYPE_UINT */
	MPIPE_CAPS_BITWIDTH,
	/** Number of channels (audio), MPIPE_TYPE_UINT */
	MPIPE_CAPS_NUM_OF_CHANNEL,
	/**
	 * Layout of the channels within a buffer (audio), MPIPE_TYPE_BOOLEAN: true for
	 * interleaved (LRLRLRLR), false for non-interleaved (LLLLRRRR)
	 */
	MPIPE_CAPS_INTERLEAVED,
	/**
	 * Time covered by one frame, in microseconds, MPIPE_TYPE_UINT. Used by both the
	 * audio and the video domains: for video it is the frame interval, the
	 * reciprocal of the frame rate.
	 */
	MPIPE_CAPS_FRAME_INTERVAL,
	/**
	 * One past the last field identifier. Bounds the range of valid
	 * identifiers, and terminates the field list of
	 * @ref mpipe_structure_init_fields.
	 */
	MPIPE_CAPS_END,
};

/**
 * @brief The structure constrains nothing and intersects with anything.
 *
 * Distinct from a structure with no fields set, which constrains nothing
 * because it is empty and therefore intersects with nothing.
 */
#define MPIPE_STRUCTURE_FLAG_ANY BIT(0)

/**
 * @struct mpipe_structure
 * @brief Fixed-size container holding the fields of one capability.
 *
 * Fields occupy the first `num_fields` slots of `ids` and `values`,
 * which are parallel: `ids[i]` names the field that `values[i]` holds. A slot
 * index carries no meaning of its own, so the number of identifiers that exist
 * costs nothing here; only how many fields one structure holds at once does,
 * bounded by CONFIG_MPIPE_STRUCTURE_MAX_FIELDS.
 */
struct mpipe_structure {
	/** Media type of the structure, see @ref mpipe_media_type */
	uint8_t media_type_id;
	/** Structure flags, see MPIPE_STRUCTURE_FLAG_ANY */
	uint8_t flags;
	/** Number of slots in use at the front of @ref ids and @ref values */
	uint8_t num_fields;
	/** Field identifier held by each slot, see @ref mpipe_caps_field */
	uint8_t ids[CONFIG_MPIPE_STRUCTURE_MAX_FIELDS];
	/** Value held by each slot */
	struct mpipe_value values[CONFIG_MPIPE_STRUCTURE_MAX_FIELDS];
};

/** @cond INTERNAL_HIDDEN */
#define MPIPE_STRUCTURE_FIELD_COUNT(field_id, value) 1 +
#define MPIPE_STRUCTURE_FIELD_ID(field_id, value)    (field_id),
#define MPIPE_STRUCTURE_FIELD_VALUE(field_id, value) value,
/** @endcond */

/**
 * @brief Define a read-only @ref mpipe_structure in .rodata.
 *
 * An element whose capabilities are known at build time can carry them here
 * instead of building them at runtime, and copy one out by plain struct
 * assignment when it is enumerated.
 *
 * @p fields is a macro taking one argument, which it applies to each field as
 * `arg(field_id, value_initializer)`. Listing the fields once is what keeps the
 * identifiers, the values and the count from disagreeing, which would silently
 * drop a field. Defining more fields than CONFIG_MPIPE_STRUCTURE_MAX_FIELDS fails
 * the build.
 *
 * @code{.c}
 * #define JPEG_SRC_FIELDS(X) X(MPIPE_CAPS_PIXEL_FORMAT, MPIPE_VALUE_UINT(VIDEO_PIX_FMT_JPEG))
 * MPIPE_STRUCTURE_DEFINE(jpeg_src, MPIPE_MEDIA_VIDEO, JPEG_SRC_FIELDS);
 * @endcode
 *
 * @param name Name of the structure to define.
 * @param media Media type of the structure, see @ref mpipe_media_type.
 * @param fields Macro listing the fields, see above.
 */
#define MPIPE_STRUCTURE_DEFINE(name, media, fields)                                                \
	BUILD_ASSERT((fields(MPIPE_STRUCTURE_FIELD_COUNT) 0) <= CONFIG_MPIPE_STRUCTURE_MAX_FIELDS, \
		     #name " has more fields than CONFIG_MPIPE_STRUCTURE_MAX_FIELDS");             \
	static const struct mpipe_structure name = {                                               \
		.media_type_id = (media),                                                          \
		.flags = 0,                                                                        \
		.num_fields = fields(MPIPE_STRUCTURE_FIELD_COUNT) 0,                               \
		.ids = {fields(MPIPE_STRUCTURE_FIELD_ID)},                                         \
		.values = {fields(MPIPE_STRUCTURE_FIELD_VALUE)},                                   \
	}

/**
 * @brief Initialize an @ref mpipe_structure.
 *
 * Leaves the structure with no field set.
 *
 * @param structure Pointer to the structure to initialize.
 * @param media_type_id Media type of the structure, see @ref mpipe_media_type.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p media_type_id is not a media type
 */
int mpipe_structure_init(struct mpipe_structure *structure, uint8_t media_type_id);

/**
 * @brief Initialize an @ref mpipe_structure with a media type and a list of fields.
 *
 * Builds a capability directly into caller storage: repeated
 * `field_id, type, value-args...` triples terminated by MPIPE_CAPS_END, with the
 * same per-type argument rules as @ref mpipe_value_set. Allocates nothing, so a
 * capability can be described on the stack or in a caller's own slot.
 *
 * @param structure Pointer to the structure to initialize.
 * @param media_type_id Media type of the structure, see @ref mpipe_media_type.
 * @param ... Field triples, terminated by MPIPE_CAPS_END.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p media_type_id is not a media
 *         type, or a field identifier or value type is invalid
 * @retval -EEXIST The list names the same field twice
 * @retval -ENOSPC The list holds more fields than
 *         CONFIG_MPIPE_STRUCTURE_MAX_FIELDS
 */
int mpipe_structure_init_fields(struct mpipe_structure *structure, uint8_t media_type_id, ...);

/**
 * @brief Initialize a structure that constrains nothing.
 *
 * Intersecting it with another structure yields a copy of that structure, and
 * fixating it reports -ENOENT because there is nothing to choose.
 *
 * @param structure Pointer to the structure to initialize.
 *
 * @retval 0 Success.
 */
int mpipe_structure_init_any(struct mpipe_structure *structure);

/**
 * @brief Check whether a structure constrains nothing.
 *
 * @param structure Pointer to the structure to check, may be NULL.
 *
 * @return true if the structure constrains nothing, false otherwise or if @p
 *         structure is NULL
 */
bool mpipe_structure_is_any(const struct mpipe_structure *structure);

/**
 * @brief Check whether an @ref mpipe_structure matches nothing.
 *
 * The counterpart of @ref mpipe_structure_is_any - a structure that constrains
 * nothing because it holds no field intersects with nothing, where an ANY one
 * intersects with everything.
 *
 * @param structure Pointer to the structure to check, may be NULL.
 *
 * @return true if the structure is empty, false otherwise or if @p structure
 *         is NULL
 */
bool mpipe_structure_is_empty(const struct mpipe_structure *structure);

/**
 * @brief Append a field to an @ref mpipe_structure from caller-owned storage.
 *
 * Copies @p value into the next free slot; the caller keeps its own copy.
 *
 * @param structure Pointer to the structure to append the field to.
 * @param field_id Field identifier, see @ref mpipe_caps_field. Must not already be set.
 * @param value Pointer to the field value to copy in.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p field_id is not a field identifier
 * @retval -EEXIST The structure already carries @p field_id
 * @retval -ENOSPC The structure already holds
 *         CONFIG_MPIPE_STRUCTURE_MAX_FIELDS fields
 */
int mpipe_structure_append_value(struct mpipe_structure *structure, uint8_t field_id,
				 const struct mpipe_value *value);

/**
 * @brief Copy a field from one @ref mpipe_structure to another.
 *
 * Leaves @p dst untouched and reports -ENOENT when @p src does not carry the
 * field, so a caller building a capability out of another one can offer every
 * field it wants to pass through and ignore that code, rather than testing for
 * each of them first.
 *
 * @param src Pointer to the structure to read the field from.
 * @param dst Pointer to the structure to append the field to.
 * @param field_id Field identifier, see @ref mpipe_caps_field.
 *
 * @retval 0 Success.
 * @retval -ENOENT @p src does not carry the field
 * @retval -EINVAL @p field_id is not a field identifier
 * @retval -EEXIST @p dst already carries the field
 * @retval -ENOSPC @p dst is full
 */
int mpipe_structure_copy_field(const struct mpipe_structure *src, struct mpipe_structure *dst,
			       uint8_t field_id);

/**
 * @brief Empty an @ref mpipe_structure, dropping every field it carries.
 *
 * The structure keeps its media type and its flags, so it can be filled in
 * again without being initialized first.
 *
 * This leaves it constraining nothing because it holds no field, which
 * intersects with nothing. It does not make it an ANY structure, which
 * constrains nothing but intersects with everything: use
 * @ref mpipe_structure_init_any for that.
 *
 * @param structure Pointer to the structure to clear.
 *
 * @retval 0 Success.
 */
int mpipe_structure_clear(struct mpipe_structure *structure);

/**
 * @brief Check if an @ref mpipe_structure is fixed.
 *
 * A structure is fixed when it carries at least one field and every field it
 * carries holds a single value rather than a range. A structure that
 * constrains nothing is never fixed, whether it is flagged ANY or simply
 * holds no field: there is nothing to have settled.
 *
 * @param structure Pointer to the structure to check.
 *
 * @return true if the structure is fixed, false otherwise or if @p structure
 *         is NULL
 */
bool mpipe_structure_is_fixed(const struct mpipe_structure *structure);

/**
 * @brief Get the value of a field in an @ref mpipe_structure.
 *
 * Retrieves the value associated with the specified field ID.
 *
 * @param structure Pointer to the structure containing the field, may be NULL.
 * @param field_id Field identifier, see @ref mpipe_caps_field.
 *
 * @return Pointer to the value of the field, NULL if the structure does not
 *         carry it or @p structure is NULL.
 */
const struct mpipe_value *mpipe_structure_get_value(const struct mpipe_structure *structure,
						    uint8_t field_id);

/**
 * @brief Remove a field from an @ref mpipe_structure.
 *
 * Deletes the field with the specified ID from the structure.
 *
 * @param structure Pointer to the structure containing the field to remove.
 * @param field_id Field identifier, see @ref mpipe_caps_field.
 *
 * @retval 0 Success.
 * @retval -ENOENT The structure does not carry the field
 */
int mpipe_structure_remove_field(struct mpipe_structure *structure, uint8_t field_id);

/**
 * @brief Intersect two structures into caller-provided storage.
 *
 * The structures must share the same media type, carry at least one field
 * identifier in common, and every field they do share must have intersecting
 * values. The result is the union of both inputs: a field both sides carry
 * holds the intersected value, and a field only one side carries is copied
 * through unchanged, which can leave the result holding more fields than
 * either input. Intersecting an ANY structure yields a copy of the other one.
 *
 * @p out is written field by field, so it must not be one of the inputs.
 *
 * @param struct1 Pointer to the first structure.
 * @param struct2 Pointer to the second structure.
 * @param[out] out Pointer to storage for the result, left untouched on -EINVAL
 *             and cleared on any other failure.
 *
 * @retval 0 Success.
 * @retval -ENOENT The structures share no field, or a shared field has no
 *         common value
 * @retval -EINVAL @p out aliases an input, or the media types differ
 * @retval -ENOSPC The union does not fit CONFIG_MPIPE_STRUCTURE_MAX_FIELDS
 */
int mpipe_structure_intersect(const struct mpipe_structure *struct1,
			      const struct mpipe_structure *struct2, struct mpipe_structure *out);

/**
 * @brief Fixate a structure into caller-provided storage.
 *
 * Each field is reduced to a single value: a range to its minimum, a value
 * already fixed to itself.
 *
 * @param src Pointer to the structure to fixate.
 * @param[out] out Pointer to storage for the result, left untouched on failure.
 *
 * @retval 0 Success.
 * @retval -ENOENT @p src constrains nothing and so has nothing to fixate,
 *         whether it is flagged ANY or simply holds no field
 * @retval -EINVAL @p out is @p src
 */
int mpipe_structure_fixate(const struct mpipe_structure *src, struct mpipe_structure *out);

/**
 * @brief Print an @ref mpipe_structure.
 *
 * Outputs the contents of the structure for debugging or inspection.
 *
 * @param structure Pointer to the structure to print.
 */
void mpipe_structure_print(const struct mpipe_structure *structure);

/** @} */

#endif /*ZEPHYR_INCLUDE_MPIPE_MPIPE_STRUCTURE_H_*/
