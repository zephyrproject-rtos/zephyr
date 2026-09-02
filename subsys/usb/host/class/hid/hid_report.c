/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usbh_hid.h>

/* Whether the item is long or short, given its first byte */
#define ITEM_IS_LONG(First)  ((First) == 0xFE)
#define ITEM_IS_SHORT(First) !ITEM_IS_LONG(First)
/* Get the tag field of the item's first byte */
#define ITEM_TAG(Tag)        (((Tag) & 0xF0) >> 4)
/* Get the type field of the item's first byte */
#define ITEM_TYPE(Tag)       (((Tag) & 0x0C) >> 2)
/* Get the size field of the item's first byte */
#define ITEM_SIZE(Tag)       ((Tag) & 0x03)

enum item_type {
	ITEM_TYPE_MAIN = 0,
	ITEM_TYPE_GLOBAL = 1,
	ITEM_TYPE_LOCAL = 2,
};

/* The size of short items can be of either 0, 1, 2 or 4 bytes, skipping 3. This enumeration
 * reflects that
 */
enum item_data_size {
	ITEM_DATA_SIZE_0 = 0,
	ITEM_DATA_SIZE_1 = 1,
	ITEM_DATA_SIZE_2 = 2,
	ITEM_DATA_SIZE_4 = 3,
};

/* Possible main items and associated tag value */
enum main_item_tag {
	MAIN_ITEM_TAG_INPUT = 0x8,
	MAIN_ITEM_TAG_OUTPUT = 0x9,
	MAIN_ITEM_TAG_COLLECTION = 0xA,
	MAIN_ITEM_TAG_FEATURE = 0xB,
	MAIN_ITEM_TAG_COLLECTION_END = 0xC,
};

/* Possible global items and associated tag value */
enum global_item_tag {
	GLOBAL_ITEM_TAG_USAGE_PAGE = 0x0,
	GLOBAL_ITEM_TAG_LOGICAL_MINIMUM = 0x1,
	GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM = 0x2,
	GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM = 0x3,
	GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM = 0x4,
	GLOBAL_ITEM_TAG_UNIT_EXPONENT = 0x5,
	GLOBAL_ITEM_TAG_UNIT = 0x6,
	GLOBAL_ITEM_TAG_REPORT_SIZE = 0x7,
	GLOBAL_ITEM_TAG_REPORT_ID = 0x8,
	GLOBAL_ITEM_TAG_REPORT_COUNT = 0x9,
	GLOBAL_ITEM_TAG_PUSH = 0xA,
	GLOBAL_ITEM_TAG_POP = 0xB,
};

/* Possible local items and associated tag value */
enum local_item_tag {
	LOCAL_ITEM_TAG_USAGE = 0x0,
	LOCAL_ITEM_TAG_USAGE_MINIMUM = 0x1,
	LOCAL_ITEM_TAG_USAGE_MAXIMUM = 0x2,
	LOCAL_ITEM_TAG_DESIGNATOR_INDEX = 0x3,
	LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM = 0x4,
	LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM = 0x5,
	LOCAL_ITEM_TAG_STRING_INDEX = 0x7,
	LOCAL_ITEM_TAG_STRING_MINIMUM = 0x8,
	LOCAL_ITEM_TAG_STRING_MAXIMUM = 0x9,
	LOCAL_ITEM_TAG_DELIMITER = 0xA,
};

/* Global state during report descriptor parsing */
struct global_state {
	uint32_t usage_page;

	/* Minimum reportable value */
	int32_t logical_minimum;
	/* Maximum reportable value */
	int32_t logical_maximum;

	/* Minimum value in units */
	int32_t physical_minimum;
	/* Maximum value in units */
	int32_t physical_maximum;
	/* Value of this unit exponent in base 10 */
	uint32_t unit_exponent;
	/* Unit values */
	uint32_t unit;

	/* Current report ID, in case of multiple variants */
	uint8_t report_id;
	/* Size of this field */
	uint32_t report_size;
	/* Number of instances of this field */
	uint32_t report_count;
};

struct hid_report_parse_context {
	/* A stack of indexes pointing to the report's collection list */
	size_t collection_stack_index;
	size_t collection_stack[CONFIG_USBH_HID_REPORT_MAX_COLLECTIONS];

	/* Local state, reset at each main item, to be used during parsing */
	struct {
		/* Usage range */
		uint32_t usage_minimum;
		uint32_t usage_maximum;

		/* Index of the physical body part related to this field */
		uint32_t designator_index;
		/* Starting designator  */
		uint32_t designator_minimum;
		/* End designator  */
		uint32_t designator_maximum;

		/* Index of a string descriptor describing this field */
		uint32_t string_index;
		/* Starting string index */
		uint32_t string_minimum;
		/* End string index */
		uint32_t string_maximum;

		/* Pseudo-stack of usage IDs */
		size_t num_usages;
		uint32_t usages[CONFIG_USBH_HID_REPORT_MAX_USAGES];
	} local_state;

	/* Global state stack used during parsing. The stack can stores copies of the global state,
	 * to be pushed and popped from and to `global_state`
	 */
	size_t state_stack_index;
	struct global_state state_stack[CONFIG_USBH_HID_REPORT_MAX_STATE_STACK];
	struct global_state global_state;

	/* Pointer to the output report structure */
	struct usbh_hid_report *report;

	/* Prasing cursor (i.e. byte index for the data array) */
	size_t cursor;
	/* Length of the report descriptor data */
	size_t data_length;
	/* Raw report descriptor */
	const uint8_t *data;
};

/*
 * Checks that the item currently under cursor can fit in the remaining data.
 */
static int check_size(struct hid_report_parse_context *context);

/*
 * Reads a usage page under the cursor position into the context.
 * Returns -EINVAL if the usage page is invalid.
 */
static int read_usage_page(struct hid_report_parse_context *context);

/*
 * Reads a usage ID under the cursor.
 * Returns -EINVAL if the usage ID is invalid.
 */
static int read_usage_id(struct hid_report_parse_context *context, uint32_t *usage);

/*
 * Reads up to a 32-bit unsigned integer from under the cursor.
 * Returns -EINVAL if the value is invalid.
 */
static int read_u32(struct hid_report_parse_context *context, uint32_t *value);

/*
 * Reads up to a 32-bit signed integer from under the cursor.
 * Returns -EINVAL if the value is invalid.
 */
static int read_i32(struct hid_report_parse_context *context, int32_t *value);

/*
 * Pops a usage ID from the list in the context. The last ID in the list is always kept on, only
 * cleared when moving to a new main item.
 * Returns -EINVAL if the value is invalid
 */
static int pop_usage(struct hid_report_parse_context *context, uint32_t *usage);

/*
 * Parse a global item. Does not advance the cursor.
 * Returns -EINVAL if the value is invalid, -ENOMEM if the statically allocated resources are not
 * sufficient.
 */
static int parse_global_item(struct hid_report_parse_context *context);

/*
 * Parse a local item. Does not advance the cursor.
 * Returns -EINVAL if the value is invalid, -ENOMEM if the statically allocated resources are not
 * sufficient.
 */
static int parse_local_item(struct hid_report_parse_context *context);

/*
 * Parse a main item. Does not advance the cursor.
 * Returns -EINVAL if the value is invalid, -ENOMEM if the statically allocated resources are not
 * sufficient.
 */
static int parse_main_item(struct hid_report_parse_context *context);

/*
 * Parse a field. Does not advance the cursor. Most of the data for the field is actually taken from
 * local and global states. Returns -EINVAL if the value is invalid, -ENOMEM if the statically
 * allocated resources are not sufficient.
 */
static int parse_field(struct hid_report_parse_context *context,
			enum usbh_hid_report_field_type type);

int usbh_hid_report_parse(struct usbh_hid_report *report, size_t data_length, const uint8_t *data)
{
	int ret = 0;
	/* Initialize the context */
	struct hid_report_parse_context context = {
		.report = report,
		.data_length = data_length,
		.data = data,
	};

	if (report == NULL || data == NULL) {
		return -EINVAL;
	}

	memset(report, 0, sizeof(*report));

	while (context.cursor < data_length) {
		/* Long item: reserved for future use, skip */
		if (ITEM_IS_LONG(data[context.cursor])) {
			uint8_t data_size = 0;
			/* Wouldn't fit in the remaining data */
			if (context.cursor + 3 > data_length) {
				return -EINVAL;
			}

			data_size = data[context.cursor + 1];

			/* Too big */
			if (context.cursor + 3 + data_size > data_length) {
				return -EINVAL;
			}

			context.cursor += 3 + data_size;
		} else {
			/* Short item, part of the specification */
			int item_length = check_size(&context);

			if (item_length < 0) {
				/* Doesn't fit in the remaining data */
				return item_length;
			}

			/* Switch over the type of the item currently under examination */
			switch (ITEM_TYPE(data[context.cursor])) {
			case ITEM_TYPE_GLOBAL: {
				ret = parse_global_item(&context);
				break;
			}
			case ITEM_TYPE_LOCAL: {
				ret = parse_local_item(&context);
				break;
			}
			case ITEM_TYPE_MAIN: {
				ret = parse_main_item(&context);
				break;
			}
				/* Unknown item type, probably corrupted data */
			default: {
				return -EINVAL;
			}
			}

			if (ret < 0) {
				return ret;
			}

			/* Advance the cursor */
			context.cursor += item_length;
		}
	}

	return 0;
}

uint16_t usbh_hid_report_field_get_usage_id_by_index(const struct usbh_hid_report_field *field,
						     size_t field_index)
{
	uint16_t ret = 0;

	if (
		/* Usage ID is specified as range */
		field->usage_minimum != 0 && field->usage_maximum != 0 &&
		/* The range is valid */
		field->usage_maximum > field->usage_minimum) {

		/* if the index fits in it use it */
		if (field_index < field->usage_maximum - field->usage_minimum) {
			/* Remove the usage page */
			ret = (uint16_t)(field->usage_minimum & 0xFFFF) + field_index;
		}
		/* Otherwise it defaults to the last */
		else {
			/* Remove the usage page */
			ret = (uint16_t)(field->usage_maximum & 0xFFFF);
		}
	}
	/* Othersise look for it in the usages list */
	else {
		ret = field->usages[0];

		/* If there are not enough usage IDs it defaults to the last */
		for (size_t usage_index = 0;
		     usage_index < CONFIG_USBH_HID_REPORT_MAX_USAGES &&
		     usage_index <= field_index && field->usages[usage_index] != 0;
		     usage_index++) {
			/* Remove the usage page */
			ret = (uint16_t)(field->usages[usage_index] & 0xFFFF);
		}
	}

	return ret;
}

bool usbh_hid_report_field_contains_usage_id(const struct usbh_hid_report_field *field,
					     uint32_t usage_id, size_t *field_index)
{
	bool ret = false;

	/* Usage range */
	if (field->usage_minimum != 0 && field->usage_maximum != 0) {
		ret = (usage_id >= field->usage_minimum) && (usage_id <= field->usage_maximum);

		if (ret && field_index != NULL) {
			*field_index = usage_id - field->usage_minimum;
		}
	}
	/* Search the usage list */
	else {
		for (size_t usage_index = 0; usage_index < CONFIG_USBH_HID_REPORT_MAX_USAGES &&
					     field->usages[usage_index] != 0;
		     usage_index++) {
			if (field->usages[usage_index] == usage_id) {
				ret = true;

				if (field_index != NULL) {
					*field_index = usage_index;
				}
				break;
			}
		}
	}

	return ret;
}

int usbh_hid_report_get_usage_id_u32(const struct usbh_hid_report_field *field, size_t data_length,
				     const uint8_t *data, size_t bit_start, uint32_t usage_id,
				     uint32_t *value)
{
	size_t index = 0;

	if (field == NULL || data == NULL || value == NULL) {
		return -EINVAL;
	}

	if (field->size == 0 || field->size > 32) {
		/* Invalid field size */
		return -ENOTSUP;
	}

	if (USBH_HID_REPORT_DATA_IS_ARRAY(field->flags)) {
		/* Search is supported only for data items */
		return -ENOTSUP;
	}

	if (!usbh_hid_report_field_contains_usage_id(field, usage_id, &index)) {
		/* No such usage ID */
		return -ENOENT;
	}

	if (index >= field->count) {
		/* Invalid usage ID */
		return -EINVAL;
	}

	*value = 0;
	bit_start += index * field->size;
	for (size_t bit_index = 0; bit_index < field->size; bit_index++) {
		size_t bit_field_position = bit_start + bit_index;
		uint32_t byte_index = bit_field_position / 8;
		uint32_t bit_shift = bit_field_position % 8;

		if (byte_index >= data_length) {
			/* Not enough data */
			return -ENODATA;
		}

		if (data[byte_index] & (1u << bit_shift)) {
			*value |= 1u << bit_index;
		}
	}

	return 0;
}

int usbh_hid_report_get_usage_id_i32(const struct usbh_hid_report_field *field, size_t data_length,
				     const uint8_t *data, size_t bit_start, uint32_t usage_id,
				     int32_t *value)
{
	/* First, fetch the u32 value */
	uint32_t unsigned_value = 0;
	uint32_t max_field_value = 0;

	/* Null size, the following code would not make sense */
	if (field->size == 0) {
		*value = 0;
		return 0;
	}

	int ret = usbh_hid_report_get_usage_id_u32(field, data_length, data, bit_start, usage_id,
						   &unsigned_value);
	if (ret != 0) {
		return ret;
	}

	/* If the value would be negative (most significant bit set) convert between the arbitrarily
	 * sized two's complement to the i32
	 */
	if ((unsigned_value & (1u << (field->size - 1u))) != 0u) {
		max_field_value = (uint32_t)(((uint64_t)1u << field->size) - 1u);
		*value = (int32_t)(((int64_t)unsigned_value - (int64_t)max_field_value) - 1ll);
	}
	/* The value was positive */
	else {
		*value = (int32_t)unsigned_value;
	}

	return 0;
}

bool usbh_hid_report_match_usage_page(const struct usbh_hid_report_field *field,
				      uint16_t usage_page)
{
	bool ret = false;

	/* Generic usage page, always true */
	if (usage_page == 0) {
		ret = true;
	}
	/* Usage range */
	else if (field->usage_minimum != 0 && field->usage_maximum != 0) {
		ret = ((field->usage_minimum >> 16) == usage_page) &&
		      (field->usage_maximum >> 16 == usage_page);
	}
	/* Search usage list */
	else {
		for (size_t usage_index = 0;
		     usage_index < ARRAY_SIZE(field->usages) && field->usages[usage_index] != 0;
		     usage_index++) {
			if ((field->usages[usage_index] >> 16) == usage_page) {
				ret = true;
				break;
			}
		}
	}

	return ret;
}

int usbh_hid_report_get_input_size(const struct usbh_hid_report *report, uint8_t report_id)
{
	/* The report descriptors have bit precision */
	size_t bitsize = 0;
	bool report_id_present = false;

	if (report == NULL) {
		return -EINVAL;
	}

	/* Empty report */
	if (report->num_fields == 0) {
		return 0;
	}

	for (size_t field_index = 0; field_index < report->num_fields; field_index++) {
		const struct usbh_hid_report_field *field = &report->fields[field_index];

		/* Skip fields not belonging to this report ID */
		if (report->num_reports > 0 && field->report_id != report_id) {
			continue;
		} else {
			report_id_present = true;
		}

		/* We are only interested in input reports */
		if (field->type != USBH_HID_REPORT_FIELD_TYPE_INPUT) {
			continue;
		}

		/* Invalid size */
		if (field->size == 0u || field->size > 32u ||
		    (uint64_t)field->size * (uint64_t)field->count > (uint64_t)0xFFFFFFFFu) {
			return -EINVAL;
		}

		bitsize += (field->size * field->count);
	}

	if (!report_id_present) {
		return -EINVAL;
	}

	/* Pad to byte size */
	if ((bitsize % 8) != 0) {
		bitsize += 8 - (bitsize % 8);
	}

	if (report->num_reports > 0) {
		/* If multiple variants are present the report ID is prepended to the report, adding
		 * one byte to the total length
		 */
		bitsize += 8;
	}

	/* The length in bytes */
	return bitsize / 8;
}

int usbh_hid_report_input_iterate(const struct usbh_hid_report *report, size_t data_length,
				  const uint8_t *data, usbh_hid_report_cb_t callback,
				  void *user_data)
{
	size_t data_bit_position = 0;
	int ret = 0;
	uint8_t report_id = 0;
	bool report_id_present = false;

	if (report == NULL || data == NULL || callback == NULL) {
		return -EINVAL;
	}

	/* Empty report */
	if (report->num_fields == 0) {
		return 0;
	}

	/* If multiple variants are specified expect the report ID in the first byte */
	if (report->num_reports > 0) {
		if (data_length < 1) {
			return -EINVAL;
		}

		report_id = data[0];
		data_bit_position = 8;
	}

	for (size_t field_index = 0; field_index < report->num_fields; field_index++) {
		const struct usbh_hid_report_field *field = &report->fields[field_index];
		size_t field_bit_length = field->size * field->count;

		/* Skip fields not belonging to this report ID */
		if (report->num_reports > 0 && field->report_id != report_id) {
			continue;
		} else {
			report_id_present = true;
		}

		/* Stop only on input fields */
		if (field->type != USBH_HID_REPORT_FIELD_TYPE_INPUT) {
			continue;
		}

		/* Invalid size */
		if (field->size == 0u || field->size > 32u ||
		    (uint64_t)field->size * (uint64_t)field->count > (uint64_t)0xFFFFFFFFu) {
			return -EINVAL;
		}

		if (data_bit_position + field_bit_length > data_length * 8) {
			return -EINVAL;
		}

		/* Invoke the user provided callback on each non-constant field */
		if (USBH_HID_REPORT_DATA_IS_DATA(field->flags)) {
			ret = callback(field, report_id, data_length, data, data_bit_position,
				       user_data);
			if (ret != 0) {
				return ret;
			}
		}

		data_bit_position += field->size * field->count;
	}

	if (!report_id_present) {
		return -EINVAL;
	}

	return 0;
}

static int parse_main_item(struct hid_report_parse_context *context)
{
	enum item_data_size item_size = ITEM_SIZE(context->data[context->cursor]);
	struct usbh_hid_report *report = context->report;
	int ret = 0;

	switch (ITEM_TAG(context->data[context->cursor])) {
	case MAIN_ITEM_TAG_INPUT: {
		ret = parse_field(context, USBH_HID_REPORT_FIELD_TYPE_INPUT);
		if (ret < 0) {
			return ret;
		}
		break;
	}
	case MAIN_ITEM_TAG_OUTPUT: {
		ret = parse_field(context, USBH_HID_REPORT_FIELD_TYPE_OUTPUT);
		if (ret < 0) {
			return ret;
		}
		break;
	}
	case MAIN_ITEM_TAG_FEATURE: {
		ret = parse_field(context, USBH_HID_REPORT_FIELD_TYPE_FEATURE);
		if (ret < 0) {
			return ret;
		}
		break;
	}
	case MAIN_ITEM_TAG_COLLECTION: {
		uint32_t usage = 0;
		size_t collection_index = report->num_collections;
		struct usbh_hid_report_collection *collection = NULL;

		if (report->num_collections >= ARRAY_SIZE(report->collections)) {
			/* Too many collections */
			return -ENOMEM;
		}

		if (context->collection_stack_index >= ARRAY_SIZE(context->collection_stack)) {
			/* Too many collections in the stack */
			return -ENOMEM;
		}

		/* Invalid collection with no type */
		if (item_size == 0) {
			return -EINVAL;
		}

		/* Get the usage from the local state */
		ret = pop_usage(context, &usage);
		if (ret < 0) {
			return ret;
		}

		/* Initialize the collection */
		collection = &report->collections[collection_index];
		report->num_collections++;

		context->collection_stack[context->collection_stack_index] = collection_index;

		collection->type = context->data[context->cursor + 1];
		/* The collection is delimited by the current data item */
		collection->start = report->num_fields;
		collection->usage = usage;
		context->collection_stack_index++;
		break;
	}
	case MAIN_ITEM_TAG_COLLECTION_END: {
		if (context->collection_stack_index == 0) {
			/* Mismatched collection end */
			return -EINVAL;
		}

		/* Pop the collection from the stack */
		context->collection_stack_index--;
		/* Add the end to its fields */
		report->collections[context->collection_stack[context->collection_stack_index]]
			.end = report->num_fields;
		break;
	}
	default: {
		ret = -EINVAL;
	}
	}

	/* Each time a new main item is found clear the local state */
	context->local_state.usage_minimum = 0;
	context->local_state.usage_maximum = 0;
	context->local_state.num_usages = 0;
	context->local_state.designator_index = 0;
	context->local_state.designator_minimum = 0;
	context->local_state.designator_maximum = 0;
	context->local_state.string_index = 0;
	context->local_state.string_minimum = 0;
	context->local_state.string_maximum = 0;

	return ret;
}

static int parse_global_item(struct hid_report_parse_context *context)
{
	int ret = 0;

	switch (ITEM_TAG(context->data[context->cursor])) {
	case GLOBAL_ITEM_TAG_USAGE_PAGE: {
		ret = read_usage_page(context);
		break;
	}
	case GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM: {
		ret = read_i32(context, &context->global_state.physical_minimum);
		break;
	}
	case GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM: {
		ret = read_i32(context, &context->global_state.physical_maximum);
		break;
	}
	case GLOBAL_ITEM_TAG_UNIT_EXPONENT: {
		ret = read_u32(context, &context->global_state.unit_exponent);
		break;
	}
	case GLOBAL_ITEM_TAG_UNIT: {
		ret = read_u32(context, &context->global_state.unit);
		break;
	}
	case GLOBAL_ITEM_TAG_LOGICAL_MINIMUM: {
		ret = read_i32(context, &context->global_state.logical_minimum);
		break;
	}
	case GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM: {
		ret = read_i32(context, &context->global_state.logical_maximum);
		break;
	}
	case GLOBAL_ITEM_TAG_REPORT_COUNT: {
		ret = read_u32(context, &context->global_state.report_count);
		break;
	}
	case GLOBAL_ITEM_TAG_REPORT_SIZE: {
		ret = read_u32(context, &context->global_state.report_size);
		break;
	}
	case GLOBAL_ITEM_TAG_REPORT_ID: {
		struct usbh_hid_report *report = context->report;
		uint32_t report_id = 0;
		bool report_id_present = false;

		/* Read the report ID */
		ret = read_u32(context, &report_id);
		if (ret != 0) {
			break;
		}

		/* A report id of 0 is reserved */
		if (report_id == 0) {
			ret = -EINVAL;
			break;
		}

		/* The first report ID must appear before any field */
		if (report->num_fields > 0 && report->num_reports == 0) {
			ret = -EINVAL;
			break;
		}

		/* Check if the same report ID was already encountered */
		for (size_t report_index = 0; report_index < report->num_reports; report_index++) {
			if (report->reports[report_index] == report_id) {
				report_id_present = true;
				break;
			}
		}

		if (report_id_present) {
			/* If so, do nothing */
		} else {
			if (report->num_reports >= ARRAY_SIZE(report->reports)) {
				/* Too many report variants */
				ret = -ENOMEM;
				break;
			}

			/* Otherwise push it */
			report->reports[report->num_reports] = (uint8_t)report_id;
			report->num_reports++;
		}

		context->global_state.report_id = report_id;
		break;
	}
	case GLOBAL_ITEM_TAG_PUSH: {
		if (context->state_stack_index >= ARRAY_SIZE(context->state_stack)) {
			/* Too many states */
			ret = -ENOMEM;
			break;
		}

		/* Push a copy of the global item table on the stack */
		context->state_stack[context->state_stack_index] = context->global_state;
		context->state_stack_index++;
		break;
	}
	case GLOBAL_ITEM_TAG_POP: {
		if (context->state_stack_index == 0) {
			/* Mismatched pop operation */
			ret = -EINVAL;
			break;
		}

		/* Pop the global state from the stack */
		context->state_stack_index--;
		context->global_state = context->state_stack[context->state_stack_index];
		break;
	}
	default: {
		ret = -EINVAL;
		break;
	}
	}

	return ret;
}

static int parse_local_item(struct hid_report_parse_context *context)
{
	int ret = 0;

	switch (ITEM_TAG(context->data[context->cursor])) {
	case LOCAL_ITEM_TAG_USAGE: {
		/* Populate next usage in the local list*/
		uint32_t *usage = NULL;

		if (context->local_state.num_usages >= ARRAY_SIZE(context->local_state.usages)) {
			/* Too many usages */
			return -ENOMEM;
		}

		usage = &context->local_state.usages[context->local_state.num_usages];
		ret = read_usage_id(context, usage);
		if (ret != 0) {
			break;
		}

		context->local_state.num_usages++;
		break;
	}
	case LOCAL_ITEM_TAG_USAGE_MINIMUM: {
		ret = read_usage_id(context, &context->local_state.usage_minimum);
		break;
	}
	case LOCAL_ITEM_TAG_USAGE_MAXIMUM: {
		ret = read_usage_id(context, &context->local_state.usage_maximum);
		break;
	}
	case LOCAL_ITEM_TAG_DESIGNATOR_INDEX: {
		ret = read_u32(context, &context->local_state.designator_index);
		break;
	}
	case LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM: {
		ret = read_u32(context, &context->local_state.designator_minimum);
		break;
	}
	case LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM: {
		ret = read_u32(context, &context->local_state.designator_maximum);
		break;
	}
	case LOCAL_ITEM_TAG_STRING_INDEX: {
		ret = read_u32(context, &context->local_state.string_index);
		break;
	}
	case LOCAL_ITEM_TAG_STRING_MINIMUM: {
		ret = read_u32(context, &context->local_state.string_minimum);
		break;
	}
	case LOCAL_ITEM_TAG_STRING_MAXIMUM: {
		ret = read_u32(context, &context->local_state.string_maximum);
		break;
	}
	case LOCAL_ITEM_TAG_DELIMITER: {
		/* Ignored as non mandatory */
		break;
	}
	default: {
		ret = -EINVAL;
	}
	}

	return ret;
}

static int read_usage_page(struct hid_report_parse_context *context)
{
	struct global_state *global_state = &context->global_state;
	enum item_data_size item_size = ITEM_SIZE(context->data[context->cursor]);

	switch (item_size) {
	case ITEM_DATA_SIZE_0: {
		/* We assume a zero sized usage page represents a value of 0 */
		global_state->usage_page = 0;
		break;
	}

	case ITEM_DATA_SIZE_1: {
		/* If just one byte it's considered part of the upper halfword */
		global_state->usage_page = context->data[context->cursor + 1] << 16;
		break;
	}

	case ITEM_DATA_SIZE_2: {
		/* Size 2 means the upper two bytes */
		global_state->usage_page =
			((uint32_t)sys_get_le16(&context->data[context->cursor + 1])) << 16;
		break;
	}

	case ITEM_DATA_SIZE_4: {
		global_state->usage_page = sys_get_le32(&context->data[context->cursor + 1]);
		break;
	}

	default: {
		return -EINVAL;
	}
	}

	return 0;
}

static int read_usage_id(struct hid_report_parse_context *context, uint32_t *usage)
{
	struct global_state *global_state = &context->global_state;
	enum item_data_size item_size = ITEM_SIZE(context->data[context->cursor]);

	switch (item_size) {
	case ITEM_DATA_SIZE_0: {
		/* A zero sized usage id is invalid */
		return -EINVAL;
	}

	case ITEM_DATA_SIZE_1: {
		/* If just one byte it's considered an offset ot the current usage page */
		*usage = global_state->usage_page | context->data[context->cursor + 1];
		break;
	}

	case ITEM_DATA_SIZE_2: {
		/* If two bytes long it's considered an offset ot the current usage page */

		*usage = global_state->usage_page |
			 sys_get_le16(&context->data[context->cursor + 1]);
		break;
	}

	case ITEM_DATA_SIZE_4: {
		/* Full usage, no need to rely on the usage page */
		*usage = sys_get_le32(&context->data[context->cursor + 1]);
		break;
	}

	default: {
		return -EINVAL;
	}
	}

	return 0;
}

static int read_i32(struct hid_report_parse_context *context, int32_t *value)
{
	enum item_data_size item_size = ITEM_SIZE(context->data[context->cursor]);

	switch (item_size) {
	case ITEM_DATA_SIZE_0: {
		*value = 0;
		break;
	}

	case ITEM_DATA_SIZE_1: {
		*value = (int8_t)context->data[context->cursor + 1];
		break;
	}

	case ITEM_DATA_SIZE_2: {
		*value = (int16_t)sys_get_le16(&context->data[context->cursor + 1]);
		break;
	}

	case ITEM_DATA_SIZE_4: {
		*value = (int32_t)sys_get_le32(&context->data[context->cursor + 1]);
		break;
	}

	default: {
		return -EINVAL;
	}
	}

	return 0;
}

static int read_u32(struct hid_report_parse_context *context, uint32_t *value)
{
	enum item_data_size item_size = ITEM_SIZE(context->data[context->cursor]);

	switch (item_size) {
	case ITEM_DATA_SIZE_0: {
		*value = 0;
		break;
	}

	case ITEM_DATA_SIZE_1: {
		*value = context->data[context->cursor + 1];
		break;
	}

	case ITEM_DATA_SIZE_2: {
		*value = sys_get_le16(&context->data[context->cursor + 1]);
		break;
	}

	case ITEM_DATA_SIZE_4: {
		*value = sys_get_le32(&context->data[context->cursor + 1]);
		break;
	}

	default: {
		return -EINVAL;
	}
	}

	return 0;
}

static int check_size(struct hid_report_parse_context *context)
{
	/* `item_size` is guaranteed to be 0-3 */
	enum item_data_size item_size = ITEM_SIZE(context->data[context->cursor]);
	/* Value to actual size conversion */
	size_t sizes[] = {1, 2, 3, 5};

	if (context->cursor + sizes[item_size] > context->data_length) {
		return -EINVAL;
	}

	return sizes[item_size];
}

static int pop_usage(struct hid_report_parse_context *context, uint32_t *usage)
{
	if (context->local_state.num_usages == 0) {
		/* No usage to pop; it's not correct but we allow it */
		*usage = 0;
		return 0;
	}

	*usage = context->local_state.usages[context->local_state.num_usages - 1];

	if (context->local_state.num_usages > 1) {
		context->local_state.num_usages--;
	}

	return 0;
}

static int parse_field(struct hid_report_parse_context *context,
			enum usbh_hid_report_field_type type)
{
	struct usbh_hid_report *report = context->report;
	struct global_state *global_state = &context->global_state;
	struct usbh_hid_report_field *field = NULL;

	if (report->num_fields >= ARRAY_SIZE(report->fields)) {
		/* Too many fields */
		return -ENOMEM;
	}

	field = &report->fields[report->num_fields];
	report->num_fields++;

	field->type = type;

	/* Copy usages to the data item */
	for (size_t usage_index = 0; usage_index < context->local_state.num_usages; usage_index++) {
		field->usages[usage_index] = context->local_state.usages[usage_index];
	}
	field->usage_minimum = context->local_state.usage_minimum;
	field->usage_maximum = context->local_state.usage_maximum;

	/* Store the remaining global state */
	field->logical_minimum = global_state->logical_minimum;
	field->logical_maximum = global_state->logical_maximum;

	/* Physical limits were not specified, they default to logical ones */
	if (global_state->physical_minimum == 0 && global_state->physical_maximum == 0) {
		field->physical_minimum = global_state->logical_minimum;
		field->physical_maximum = global_state->logical_maximum;
	}
	/* They were specified, use them */
	else {
		field->physical_minimum = global_state->physical_minimum;
		field->physical_maximum = global_state->physical_maximum;
	}

	field->report_id = global_state->report_id;
	field->count = global_state->report_count;
	field->size = global_state->report_size;
	field->unit = global_state->unit;
	field->unit_exponent = global_state->unit_exponent;
	field->designator_index = context->local_state.designator_index;
	field->designator_minimum = context->local_state.designator_minimum;
	field->designator_maximum = context->local_state.designator_maximum;
	field->string_index = context->local_state.string_index;
	field->string_minimum = context->local_state.string_minimum;
	field->string_maximum = context->local_state.string_maximum;

	return read_u32(context, &field->flags);
}
