/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usbh_hid.h>

#define HID_TEST_MAX_VALUES 8u

struct hid_report_suite_fixture {
	struct usbh_hid_report report;
};

struct hid_test_values {
	size_t num_values;
	uint32_t values[HID_TEST_MAX_VALUES];
};

static void *suite_setup(void)
{
	static struct hid_report_suite_fixture fixture = {};

	return &fixture;
}

static void suite_before(void *fixture)
{
	memset(fixture, 0, sizeof(struct hid_report_suite_fixture));
}

ZTEST_SUITE(hid_report_suite, NULL, suite_setup, suite_before, NULL, NULL);

static int field_inspector(struct usbh_hid_report_field const *field, uint8_t report_id,
			   size_t data_length, uint8_t const *data, size_t bit_index,
			   void *user_data)
{
	size_t value_index = bit_index / 8;
	size_t value_bit_shift = bit_index % 8;
	int32_t i32_value = 0;
	uint32_t u32_value = 0;
	struct hid_test_values *test_values = user_data;

	/* Keyboard input */
	if (usbh_hid_report_match_usage_page(field, HID_USAGE_GEN_KEYBOARD)) {
		/* Keyboard array, each element is a button press */
		if (USBH_HID_REPORT_DATA_IS_ARRAY(field->flags) && field->size == 8u) {
			for (size_t key_index = value_index; key_index < field->count;
			     key_index++) {
				if (data[key_index] != 0u) {
					test_values->values[test_values->num_values] =
						data[key_index];
					test_values->num_values++;
				}
			}

		}
		/* Keyboard variable, mostly for modifiers */
		else if (USBH_HID_REPORT_DATA_IS_VARIABLE(field->flags) && field->size == 1u) {
			for (size_t key_position = 0u; key_position < field->count;
			     key_position++) {
				if (data[value_index] & (1u << key_position)) {
					test_values->values[test_values->num_values] =
						(field->usage_minimum & 0xFFFF) + key_position;
					test_values->num_values++;
				}
			}
		}
	}
	/* Mouse button input */
	if (usbh_hid_report_get_usage_id_u32(
		    field, data_length - value_index, &data[value_index], value_bit_shift,
		    HID_USAGE_ID(HID_USAGE_GEN_BUTTON, 1u), &u32_value) == 0) {
		test_values->values[test_values->num_values] = u32_value;
		test_values->num_values++;
	}
	/* Mouse X movement */
	if (usbh_hid_report_get_usage_id_i32(
		    field, data_length - value_index, &data[value_index], value_bit_shift,
		    HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_X),
		    &i32_value) == 0) {
		test_values->values[test_values->num_values] = i32_value;
		test_values->num_values++;
	}
	/* Mouse Y movement */
	if (usbh_hid_report_get_usage_id_i32(
		    field, data_length - value_index, &data[value_index], value_bit_shift,
		    HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_Y),
		    &i32_value) == 0) {
		test_values->values[test_values->num_values] = i32_value;
		test_values->num_values++;
	}
	/* Mouse wheel */
	if (usbh_hid_report_get_usage_id_i32(
		    field, data_length - value_index, &data[value_index], value_bit_shift,
		    HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_WHEEL),
		    &i32_value) == 0) {
		test_values->values[test_values->num_values] = i32_value;
		test_values->num_values++;
	}

	return 0;
}

static int capture_field_values(struct usbh_hid_report_field const *field, uint8_t report_id,
				size_t data_length, uint8_t const *data, size_t bit_index,
				void *user_data)
{
	struct hid_test_values *test_values = user_data;

	test_values->values[test_values->num_values] = data[bit_index / 8u];
	test_values->num_values++;

	return 0;
}

ZTEST_F(hid_report_suite, test_hid_mouse1_report)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	size_t collection_index = 0;
	size_t report_index = 0;
	size_t field_index = 0;
	uint8_t const data[] = {0x02u, 0x01u, 0x00u, 0xFEu, 0x1Fu, 0x00u, 0x10u, 0x00u};

	/* Complex mouse */
	uint8_t report_data[] = {
		0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x02, 0x09, 0x01, 0xa1, 0x00, 0x05, 0x09,
		0x19, 0x01, 0x29, 0x10, 0x15, 0x00, 0x25, 0x01, 0x95, 0x10, 0x75, 0x01, 0x81, 0x02,
		0x05, 0x01, 0x16, 0x01, 0xf8, 0x26, 0xff, 0x07, 0x75, 0x0c, 0x95, 0x02, 0x09, 0x30,
		0x09, 0x31, 0x81, 0x06, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x01, 0x09, 0x38,
		0x81, 0x06, 0x05, 0x0c, 0x0a, 0x38, 0x02, 0x95, 0x01, 0x81, 0x06, 0xc0, 0xc0, 0x05,
		0x0c, 0x09, 0x01, 0xa1, 0x01, 0x85, 0x03, 0x75, 0x10, 0x95, 0x02, 0x15, 0x01, 0x26,
		0x8c, 0x02, 0x19, 0x01, 0x2a, 0x8c, 0x02, 0x81, 0x00, 0xc0, 0x05, 0x01, 0x09, 0x80,
		0xa1, 0x01, 0x85, 0x04, 0x75, 0x02, 0x95, 0x01, 0x15, 0x01, 0x25, 0x03, 0x09, 0x82,
		0x09, 0x81, 0x09, 0x83, 0x81, 0x60, 0x75, 0x06, 0x81, 0x03, 0xc0, 0x06, 0x00, 0xff,
		0x09, 0x01, 0xa1, 0x01, 0x85, 0x10, 0x75, 0x08, 0x95, 0x06, 0x15, 0x00, 0x26, 0xff,
		0x00, 0x09, 0x01, 0x81, 0x00, 0x09, 0x01, 0x91, 0x00, 0xc0, 0x06, 0x00, 0xff, 0x09,
		0x02, 0xa1, 0x01, 0x85, 0x11, 0x75, 0x08, 0x95, 0x13, 0x15, 0x00, 0x26, 0xff, 0x00,
		0x09, 0x02, 0x81, 0x00, 0x09, 0x02, 0x91, 0x00, 0xc0,
	};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	/* Collections */
	zassert_equal(6u, report->num_collections,
		      "Wrong number of collections in report descriptor");
	collection_index = 0u;

	/* First collection, application */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_MOUSE),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(0u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(4u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Second collection, physical */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_POINTER),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_PHYSICAL, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(0u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(4u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Third collection, application */
	zassert_equal(HID_USAGE_ID(HID_USAGE_CONSUMER, HID_USAGE_CONSUMER_CONTROL),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(4u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(5u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Fourth collection, application */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_SYSTEM_CONTROL),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(5u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(7u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Fifth collection, application */
	zassert_equal(HID_USAGE_ID(0xFF00u, 1u), report->collections[collection_index].usage,
		      "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(7u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(9u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Sixth collection, application */
	zassert_equal(HID_USAGE_ID(0xFF00u, 2u), report->collections[collection_index].usage,
		      "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(9u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(11u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Reports */
	zassert_equal(5u, report->num_reports, "Wrong number of report variants");
	report_index = 0u;

	/* Fields and report parsing */
	zassert_equal(11u, report->num_fields, "Wrong number of fields");
	field_index = 0u;

	/* First field, buttons */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(16u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(1u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_BUTTON, 0x1),
		      report->fields[field_index].usage_minimum, "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_BUTTON, 0x10),
		      report->fields[field_index].usage_maximum, "Wrong field usage ID");
	zassert_equal(0u, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(1, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Second field, X/Y */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(2u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(12u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_X),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_Y),
		      report->fields[field_index].usages[1u], "Wrong field usage ID");
	zassert_equal(-2047, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(2047, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Third field, wheel */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(1u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_WHEEL),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(-127, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(127, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Fourth field, AC pan */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(1u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_CONSUMER, HID_USAGE_CONSUMER_AC_PAN),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(-127, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(127, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector, &test_values);

	zassert_equal(4u, test_values.num_values, "Wrong number of values");
	/* First button  */
	zassert_equal(1, test_values.values[0u], "Wrong value for first button");
	/* X relative movement */
	zassert_equal(-2, test_values.values[1u], "Wrong value for X movement");
	/* Y relative movement */
	zassert_equal(1, test_values.values[2u], "Wrong value for Y movement");
	/* Scroll */
	zassert_equal(16, test_values.values[3u], "Wrong scroll value");
}

ZTEST_F(hid_report_suite, test_hid_mouse2_report)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	size_t collection_index = 0u;
	size_t field_index = 0u;

	uint8_t const report_data[] = {
		0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01, 0xa1, 0x00, 0x05, 0x09, 0x15,
		0x00, 0x25, 0x01, 0x19, 0x01, 0x29, 0x05, 0x75, 0x01, 0x95, 0x05, 0x81, 0x02,
		0x95, 0x03, 0x81, 0x01, 0x05, 0x01, 0x16, 0x01, 0x80, 0x26, 0xff, 0x7f, 0x09,
		0x30, 0x09, 0x31, 0x75, 0x10, 0x95, 0x02, 0x81, 0x06, 0x15, 0x81, 0x25, 0x7f,
		0x09, 0x38, 0x75, 0x08, 0x95, 0x01, 0x81, 0x06, 0x05, 0x0c, 0x0a, 0x38, 0x02,
		0x95, 0x01, 0x81, 0x06, 0x06, 0x00, 0xff, 0x09, 0x20, 0x15, 0x00, 0x26, 0xff,
		0x00, 0x95, 0x40, 0xb1, 0x02, 0xc0, 0xc0,
	};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse the report descriptor");

	/* Collections */
	zassert_equal(2u, report->num_collections,
		      "Wrong number of collections in report descriptor");
	collection_index = 0u;

	/* First collection, application */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_MOUSE),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(0u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(6u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Second collection, physical */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_POINTER),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_PHYSICAL, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(0u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(6u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Reports */
	zassert_equal(0u, report->num_reports, "Wrong number of report variants");

	/* Fields and report parsing */
	zassert_equal(6u, report->num_fields, "Wrong number of fields");
	field_index = 0u;

	/* First field, buttons */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly an array");
	zassert_equal(5u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(1u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_BUTTON, 0x1),
		      report->fields[field_index].usage_minimum, "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_BUTTON, 0x5),
		      report->fields[field_index].usage_maximum, "Wrong field usage ID");
	zassert_equal(0u, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(1, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Second field, padding */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_equal(3u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(1u, report->fields[field_index].size, "Wrong field report size");
	zassert_true(USBH_HID_REPORT_DATA_IS_CONSTANT(report->fields[field_index].flags),
		     "Field is unexpectedly mutable");
	field_index++;

	/* Third field, X/Y */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(2u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(16u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_X),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_Y),
		      report->fields[field_index].usages[1u], "Wrong field usage ID");
	zassert_equal(-32767, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(32767, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Fourth field, wheel */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(1u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_WHEEL),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(-127, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(127, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Fifth field, AC pan */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(1u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_CONSUMER, HID_USAGE_CONSUMER_AC_PAN),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(-127, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(127, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Sixth field, vendor specific feature */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_FEATURE, report->fields[field_index].type,
		      "Wrong field type");
	zassert_equal(64u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(0xFF00, 0x20), report->fields[field_index].usages[0u],
		      "Wrong field usage ID");
	zassert_equal(0u, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(255, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	{
		/* Scroll forward by 1 */
		uint8_t const data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

		memset(&test_values, 0, sizeof(test_values));
		usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector,
					      &test_values);

		zassert_equal(4u, test_values.num_values, "Wrong number of values");
		zassert_equal(1u, test_values.values[3], "Wrong scroll value");
	}

	{
		/* Scroll backwards by 1 */
		uint8_t const data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00};

		memset(&test_values, 0, sizeof(test_values));
		usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector,
					      &test_values);

		zassert_equal(4u, test_values.num_values, "Wrong number of values");
		zassert_equal(-1u, test_values.values[3], "Wrong scroll value");
	}
}

ZTEST_F(hid_report_suite, test_hid_mouse3_report)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	size_t collection_index = 0;
	size_t field_index = 0;

	uint8_t const report_data[] = {
		0x06, 0xb5, 0xff, 0x09, 0x01, 0xa1, 0x01, 0x85, 0xb5, 0x09, 0x02, 0x15, 0x00, 0x26,
		0xff, 0x00, 0x75, 0x08, 0x95, 0x07, 0x81, 0x02, 0x09, 0x02, 0x15, 0x00, 0x26, 0xff,
		0x00, 0x75, 0x08, 0x95, 0x07, 0x91, 0x02, 0xc0, 0x05, 0x01, 0x09, 0x02, 0xa1, 0x01,
		0x85, 0x02, 0x09, 0x01, 0xa1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x08, 0x15, 0x00,
		0x25, 0x01, 0x95, 0x08, 0x75, 0x01, 0x81, 0x02, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
		0x16, 0x01, 0xf8, 0x26, 0xff, 0x07, 0x75, 0x0c, 0x95, 0x02, 0x81, 0x06, 0x09, 0x38,
		0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x01, 0x81, 0x06, 0x05, 0x0c, 0x0a, 0x38,
		0x02, 0x95, 0x01, 0x81, 0x06, 0xc0, 0xc0,
	};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse HID report");

	/* Collections */
	zassert_equal(3u, report->num_collections,
		      "Wrong number of collections in report descriptor");
	collection_index = 0u;

	/* First collection, application, vendor defined */
	zassert_equal(HID_USAGE_ID(0xFFB5, HID_USAGE_GEN_DESKTOP_POINTER),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(0u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(2u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Second collection, application */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_MOUSE),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(2u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(6u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Third collection, physical */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_POINTER),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_PHYSICAL, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(2u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(6u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Reports */
	zassert_equal(2u, report->num_reports, "Wrong number of report variants");

	/* Fields and report parsing */
	zassert_equal(6u, report->num_fields, "Wrong number of fields");
	field_index = 0u;

	/* First field, vendor specific */
	zassert_equal(0xB5u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(7u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(0xFFB5, 0x2), report->fields[field_index].usages[0],
		      "Wrong field usage ID");
	zassert_equal(0u, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(255, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Second field, vendor specific */
	zassert_equal(0xB5u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_OUTPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(7u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(0xFFB5, 0x02), report->fields[field_index].usages[0u],
		      "Wrong field usage ID");
	zassert_equal(0u, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(255, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Third field, buttons */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(8u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(1u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_BUTTON, 0x01),
		      report->fields[field_index].usage_minimum, "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_BUTTON, 0x08),
		      report->fields[field_index].usage_maximum, "Wrong field usage ID");
	zassert_equal(0u, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(1, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Fourth field, X/Y */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(2u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(12u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_X),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_Y),
		      report->fields[field_index].usages[1u], "Wrong field usage ID");
	zassert_equal(-2047, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(2047, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Fifth field, wheel */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(1u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_WHEEL),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(-127, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(127, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	/* Sixth field, AC pan */
	zassert_equal(2u, report->fields[field_index].report_id, "Wrong field report ID");
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly constant");
	zassert_equal(1u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_CONSUMER, HID_USAGE_CONSUMER_AC_PAN),
		      report->fields[field_index].usages[0u], "Wrong field usage ID");
	zassert_equal(-127, report->fields[field_index].logical_minimum,
		      "Wrong field logical minimum");
	zassert_equal(127, report->fields[field_index].logical_maximum,
		      "Wrong field logical maximum");
	field_index++;

	{
		uint8_t const data[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

		memset(&test_values, 0, sizeof(test_values));

		usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector,
					      &test_values);

		zassert_equal(4u, test_values.num_values);
		zassert_equal(1u, test_values.values[3]);
	}

	{
		uint8_t const data[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00};

		memset(&test_values, 0, sizeof(test_values));

		usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector,
					      &test_values);

		zassert_equal(4u, test_values.num_values);
		zassert_equal(-1u, test_values.values[3]);
	}
}

ZTEST_F(hid_report_suite, test_hid_gamepad_parsing)
{
	struct usbh_hid_report *report = &fixture->report;
	size_t collection_index = 0;

	uint8_t const report_data[] = {
		0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32,
		0x09, 0x35, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
		0x06, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x95, 0x01, 0x81, 0x02, 0x05, 0x01,
		0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3b, 0x01, 0x65, 0x14, 0x75,
		0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0f, 0x15,
		0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0f, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x21,
		0x95, 0x0d, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x22, 0x15, 0x00, 0x26, 0xff, 0x00,
		0x75, 0x08, 0x95, 0x34, 0x81, 0x02, 0x85, 0x02, 0x09, 0x23, 0x95, 0x2f, 0x91, 0x02,
		0x85, 0x05, 0x09, 0x33, 0x95, 0x28, 0xb1, 0x02, 0x85, 0x08, 0x09, 0x34, 0x95, 0x2f,
		0xb1, 0x02, 0x85, 0x09, 0x09, 0x24, 0x95, 0x13, 0xb1, 0x02, 0x85, 0x0a, 0x09, 0x25,
		0x95, 0x1a, 0xb1, 0x02, 0x85, 0x0b, 0x09, 0x41, 0x95, 0x29, 0xb1, 0x02, 0x85, 0x0c,
		0x09, 0x42, 0x95, 0x29, 0xb1, 0x02, 0x85, 0x20, 0x09, 0x26, 0x95, 0x3f, 0xb1, 0x02,
		0x85, 0x21, 0x09, 0x27, 0x95, 0x04, 0xb1, 0x02, 0x85, 0x22, 0x09, 0x40, 0x95, 0x3f,
		0xb1, 0x02, 0x85, 0x80, 0x09, 0x28, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0x81, 0x09, 0x29,
		0x95, 0x3f, 0xb1, 0x02, 0x85, 0x82, 0x09, 0x2a, 0x95, 0x09, 0xb1, 0x02, 0x85, 0x83,
		0x09, 0x2b, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0x84, 0x09, 0x2c, 0x95, 0x3f, 0xb1, 0x02,
		0x85, 0x85, 0x09, 0x2d, 0x95, 0x02, 0xb1, 0x02, 0x85, 0xa0, 0x09, 0x2e, 0x95, 0x01,
		0xb1, 0x02, 0x85, 0xe0, 0x09, 0x2f, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf0, 0x09, 0x30,
		0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf1, 0x09, 0x31, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2,
		0x09, 0x32, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0xf4, 0x09, 0x35, 0x95, 0x3f, 0xb1, 0x02,
		0x85, 0xf5, 0x09, 0x36, 0x95, 0x03, 0xb1, 0x02, 0xc0,
	};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	/* Collections */
	zassert_equal(1u, report->num_collections,
		      "Wrong number of collections in report descriptor");
	collection_index = 0u;

	/* First collection, application, vendor defined */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_GAMEPAD),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(0u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(29u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Reports */
	zassert_equal(24u, report->num_reports, "Wrong number of report variants");
}

ZTEST_F(hid_report_suite, test_hid_keyboard_report)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	size_t collection_index = 0;
	size_t field_index = 0;
	uint8_t const data[] = {0x01u, 0x00u, 0x1Bu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
	/* Keyboard */
	uint8_t report_data[] = {
		0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x95, 0x08, 0x75, 0x01, 0x15, 0x00,
		0x25, 0x01, 0x05, 0x07, 0x19, 0xe0, 0x29, 0xe7, 0x81, 0x02, 0x81, 0x03,
		0x95, 0x05, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02, 0x95, 0x01,
		0x75, 0x03, 0x91, 0x01, 0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x26, 0xff,
		0x00, 0x05, 0x07, 0x19, 0x00, 0x2a, 0xff, 0x00, 0x81, 0x00, 0xc0,
	};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	/* Collections */
	zassert_equal(1u, report->num_collections,
		      "Wrong number of collections in report descriptor");
	collection_index = 0u;

	/* First collection, application */
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_KEYBOARD),
		      report->collections[collection_index].usage, "Wrong collection usage ID");
	zassert_equal(HID_COLLECTION_APPLICATION, report->collections[collection_index].type,
		      "Wrong collection type");
	zassert_equal(0u, report->collections[collection_index].start,
		      "Wrong collection starting point");
	zassert_equal(5u, report->collections[collection_index].end, "Wrong collection end point");
	collection_index++;

	/* Reports */
	zassert_equal(0u, report->num_reports, "There should be no report variants");

	/* Fields and report parsing */
	zassert_equal(5u, report->num_fields, "Wrong number of fields");
	field_index = 0u;

	/* First field, modifiers */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly an array");
	zassert_equal(8u, report->fields[field_index].count, "Wrong field report size");
	zassert_equal(1u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP_KEYPAD,
				   HID_USAGE_GEN_DESKTOP_KEYBOARD_LEFT_CTRL),
		      report->fields[field_index].usage_minimum, "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP_KEYPAD,
				   HID_USAGE_GEN_DESKTOP_KEYBOARD_RIGHT_GUI),
		      report->fields[field_index].usage_maximum, "Wrong field usage ID");
	field_index++;

	/* Second field, padding */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_VARIABLE(report->fields[field_index].flags),
		     "Field is unexpectedly an array");
	zassert_equal(8u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(1u, report->fields[field_index].size, "Wrong field report size");
	zassert_true(USBH_HID_REPORT_DATA_IS_CONSTANT(report->fields[field_index].flags),
		     "Field is unexpectedly mutable");
	field_index++;

	/* Third field, LEDs */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_OUTPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_equal(5u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(1u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_LEDS, 1),
		      report->fields[field_index].usage_minimum, "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_LEDS, 5),
		      report->fields[field_index].usage_maximum, "Wrong field usage ID");
	field_index++;

	/* Fourth field, padding */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_OUTPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_equal(1u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(3u, report->fields[field_index].size, "Wrong field report size");
	zassert_true(USBH_HID_REPORT_DATA_IS_CONSTANT(report->fields[field_index].flags),
		     "Field is unexpectedly mutable");
	field_index++;

	/* Fifth field, keys */
	zassert_equal(USBH_HID_REPORT_FIELD_TYPE_INPUT, report->fields[field_index].type,
		      "Wrong field type");
	zassert_true(USBH_HID_REPORT_DATA_IS_ARRAY(report->fields[field_index].flags),
		     "Field is unexpectedly a variable");
	zassert_equal(6u, report->fields[field_index].count, "Wrong field report count");
	zassert_equal(8u, report->fields[field_index].size, "Wrong field report size");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP_KEYPAD, 0u),
		      report->fields[field_index].usage_minimum, "Wrong field usage ID");
	zassert_equal(HID_USAGE_ID(HID_USAGE_GEN_DESKTOP_KEYPAD, 255u),
		      report->fields[field_index].usage_maximum, "Wrong field usage ID");
	field_index++;

	usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector, &test_values);

	zassert_equal(2u, test_values.num_values);
	/* First button  */
	zassert_equal(HID_USAGE_GEN_DESKTOP_KEYBOARD_LEFT_CTRL, test_values.values[0u]);
	/* Second button */
	zassert_equal(0x1Bu, test_values.values[1u]);
}

ZTEST_F(hid_report_suite, test_hid_report_single_report_id)
{
	/* An explicitly stated, individual, non zero report ID is atypical but not incorrect */
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	uint8_t const report_data[] = {
		0x85u, 0x01u, /* Report ID 1 */
		0x75u, 0x08u, /* Report Size 8 */
		0x95u, 0x01u, /* Report Count 1 */
		0x81u, 0x02u, /* Input (Data,Var,Abs) */
	};
	/* Report ID byte followed by the field's data byte */
	uint8_t const data[] = {0x01u, 0x2Au};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");
	zassert_equal(1u, report->num_reports, "Wrong number of report variants");

	result = usbh_hid_report_get_input_size(report, 0x01u);
	zassert_equal(2, result, "Wrong input report size for a single report ID");

	result = usbh_hid_report_input_iterate(report, sizeof(data), data, capture_field_values,
					       &test_values);
	zassert_ok(result, "Could not iterate input report");
	zassert_equal(1u, test_values.num_values, "Wrong number of captured values");
	zassert_equal(0x2Au, test_values.values[0u],
		      "Report ID byte was not skipped with a single report ID");
}

ZTEST_F(hid_report_suite, test_hid_report_feature_larger_than_input)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	uint8_t const report_data[] = {
		0x75u, 0x08u, 0x95u, 0x01u, 0x81u, 0x02u, /* Input, 1 byte */
		0x95u, 0x14u, 0xB1u, 0x02u,               /* Feature, 20 bytes */
	};
	/* Only 1 byte, matching the Input field; the Feature field is not part of it */
	uint8_t const data[] = {0x7Fu};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	result = usbh_hid_report_input_iterate(report, sizeof(data), data, capture_field_values,
					       &test_values);
	zassert_ok(result, "A Feature field larger than the input report incorrectly aborted "
			   "iteration");
	zassert_equal(1u, test_values.num_values, "Wrong number of captured values");
	zassert_equal(0x7Fu, test_values.values[0u], "Wrong captured input value");
}

ZTEST_F(hid_report_suite, test_hid_report_duplicate_report_id)
{
	/* Fields described over multiple spans of the same report id is atypical but not incorrect
	 */
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	uint8_t const report_data[] = {
		0x85u, 0x01u,                             /* Report ID 1 */
		0x75u, 0x08u, 0x95u, 0x01u, 0x81u, 0x02u, /* Input field A */
		0x85u, 0x02u,                             /* Report ID 2 */
		0x81u, 0x02u,                             /* Input field B */
		0x85u, 0x01u,                             /* Report ID 1 again (duplicate) */
		0x81u, 0x02u,                             /* Input field C */
	};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");
	zassert_equal(2u, report->num_reports, "Duplicate report ID was counted twice");
	zassert_equal(3u, report->num_fields, "Wrong number of fields");

	result = usbh_hid_report_get_input_size(report, 0x01u);
	zassert_equal(3, result, "Fields after a duplicate report ID declaration are unreachable");

	result = usbh_hid_report_get_input_size(report, 0x02u);
	zassert_equal(2, result, "Wrong input report size for report ID 2");

	{
		uint8_t const data[] = {0x01u, 0xAAu, 0xBBu};

		result = usbh_hid_report_input_iterate(report, sizeof(data), data,
						       capture_field_values, &test_values);
		zassert_ok(result, "Could not iterate input report");
		zassert_equal(2u, test_values.num_values, "Wrong number of captured values");
		zassert_equal(0xAAu, test_values.values[0u],
			      "Wrong value for the first field of report ID 1");
		zassert_equal(0xBBu, test_values.values[1u],
			      "Field after a duplicate report ID declaration was unreachable");
	}
}

ZTEST_F(hid_report_suite, test_hid_report_size_32)
{
	struct usbh_hid_report *report = &fixture->report;
	uint8_t const report_data[] = {
		0x05u, 0x01u, 0x09u, 0x30u, /* Usage Page (Generic Desktop), Usage (X) */
		0x75u, 0x20u, 0x95u, 0x01u, /* Report Size 32, Report Count 1 */
		0x81u, 0x02u,               /* Input (Data,Var,Abs) */
	};
	int32_t value = 0;

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");
	zassert_equal(1u, report->num_fields, "Wrong number of fields");
	zassert_equal(32u, report->fields[0u].size, "Wrong field report size");

	{
		uint8_t const data[] = {0xFFu, 0xFFu, 0xFFu, 0xFFu};

		result = usbh_hid_report_get_usage_id_i32(
			&report->fields[0u], sizeof(data), data, 0u,
			HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_X), &value);
		zassert_ok(result, "Could not extract a 32-bit signed value");
		zassert_equal(-1, value, "Wrong value for an all-ones 32-bit field");
	}

	{
		uint8_t const data[] = {0x00u, 0x00u, 0x00u, 0x80u};

		result = usbh_hid_report_get_usage_id_i32(
			&report->fields[0u], sizeof(data), data, 0u,
			HID_USAGE_ID(HID_USAGE_GEN_DESKTOP, HID_USAGE_GEN_DESKTOP_X), &value);
		zassert_ok(result, "Could not extract a 32-bit signed value");
		zassert_equal(INT32_MIN, value,
			      "Wrong value for a 32-bit field with only the sign bit set");
	}
}

ZTEST_F(hid_report_suite, test_hid_report_usage_list_index)
{
	struct usbh_hid_report *report = &fixture->report;
	uint8_t const report_data[] = {
		0x05u, 0x01u, 0x09u, 0x30u, 0x09u, 0x31u, /* Usage Page, Usage X, Usage Y */
		0x75u, 0x08u, 0x95u, 0x02u,               /* Report Size 8, Report Count 2 */
		0x81u, 0x02u,                             /* Input (Data,Var,Abs) */
	};
	uint16_t usage_id = 0u;

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");
	zassert_equal(1u, report->num_fields, "Wrong number of fields");

	usage_id = usbh_hid_report_field_get_usage_id_by_index(&report->fields[0u], 0u);
	zassert_equal(HID_USAGE_GEN_DESKTOP_X, usage_id, "Wrong usage ID at index 0");

	usage_id = usbh_hid_report_field_get_usage_id_by_index(&report->fields[0u], 1u);
	zassert_equal(HID_USAGE_GEN_DESKTOP_Y, usage_id,
		      "Wrong usage ID at index 1 of a usage list field");

	usage_id = usbh_hid_report_field_get_usage_id_by_index(&report->fields[0u], 5u);
	zassert_equal(HID_USAGE_GEN_DESKTOP_Y, usage_id,
		      "Out-of-range index did not default to the last usage");
}

/*
 * The following tests feed `usbh_hid_report_parse` (and, where relevant, the report
 * inspection APIs) with malformed input and verify that an error is returned instead of the
 * data being silently accepted or misparsed.
 */

ZTEST_F(hid_report_suite, test_hid_report_error_reserved_item_type)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Short item with type field set to the reserved value 0b11 */
	uint8_t const report_data[] = {0x0Cu};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "Reserved item type was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_truncated_long_item)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Long item marker with no size/tag bytes following it */
	uint8_t const report_data[] = {0xFEu};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "Truncated long item header was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_unknown_global_tag)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Global item, tag 0xC: not one of the defined global item tags */
	uint8_t const report_data[] = {0xC4u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "Unknown global item tag was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_unknown_local_tag)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Local item, tag 0x6: not one of the defined local item tags */
	uint8_t const report_data[] = {0x68u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "Unknown local item tag was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_unknown_main_tag)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Main item, tag 0xD: not one of the defined main item tags */
	uint8_t const report_data[] = {0xD0u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "Unknown main item tag was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_report_id_zero)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Report ID 0 is reserved and must never be used */
	uint8_t const report_data[] = {0x85u, 0x00u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "A report ID of 0 was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_report_id_after_fields)
{
	struct usbh_hid_report *report = &fixture->report;
	/* An Input field is declared before any Report ID item, then a Report ID item
	 * appears; the field would belong to no report variant
	 */
	uint8_t const report_data[] = {
		0x75u, 0x08u, /* Report Size 8 */
		0x95u, 0x01u, /* Report Count 1 */
		0x81u, 0x02u, /* Input (Data,Var,Abs) */
		0x85u, 0x01u, /* Report ID 1 */
	};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "A report ID declared after unconditional fields was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_too_many_report_variants)
{
	struct usbh_hid_report *report = &fixture->report;
	/* One more Report ID item than CONFIG_USBH_HID_REPORT_MAX_VARIANTS supports */
	uint8_t report_data[2u * (CONFIG_USBH_HID_REPORT_MAX_VARIANTS + 1)];

	for (size_t i = 0; i < CONFIG_USBH_HID_REPORT_MAX_VARIANTS + 1; i++) {
		report_data[2u * i] = 0x85u;
		report_data[2u * i + 1u] = (uint8_t)(i + 1u);
	}

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_equal(-ENOMEM, result,
		      "Exceeding the maximum number of report variants did not fail");
}

ZTEST_F(hid_report_suite, test_hid_report_error_too_many_collections)
{
	struct usbh_hid_report *report = &fixture->report;
	/* One more unclosed Collection than CONFIG_USBH_HID_REPORT_MAX_COLLECTIONS supports */
	uint8_t report_data[2u * (CONFIG_USBH_HID_REPORT_MAX_COLLECTIONS + 1)];

	for (size_t i = 0; i < CONFIG_USBH_HID_REPORT_MAX_COLLECTIONS + 1; i++) {
		report_data[2u * i] = 0xA1u;
		report_data[2u * i + 1u] = 0x00u; /* Physical */
	}

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_equal(-ENOMEM, result, "Exceeding the maximum number of collections did not fail");
}

ZTEST_F(hid_report_suite, test_hid_report_error_collection_missing_type)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Collection item with no data, so no collection type can be read */
	uint8_t const report_data[] = {0xA0u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "A collection item with no type was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_mismatched_collection_end)
{
	struct usbh_hid_report *report = &fixture->report;
	/* End Collection with no matching Collection item */
	uint8_t const report_data[] = {0xC0u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result < 0, "A mismatched End Collection was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_too_many_usages)
{
	struct usbh_hid_report *report = &fixture->report;
	/* One more Usage item than CONFIG_USBH_HID_REPORT_MAX_USAGES supports */
	uint8_t report_data[2u * (CONFIG_USBH_HID_REPORT_MAX_USAGES + 1)];

	for (size_t i = 0; i < CONFIG_USBH_HID_REPORT_MAX_USAGES + 1; i++) {
		report_data[2u * i] = 0x09u;
		report_data[2u * i + 1u] = (uint8_t)(i + 1u);
	}

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_equal(-ENOMEM, result, "Exceeding the maximum number of usages did not fail");
}

ZTEST_F(hid_report_suite, test_hid_report_error_zero_size_usage_id)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Usage item with no data, so no usage ID can be read */
	uint8_t const report_data[] = {0x08u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "A zero sized usage ID was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_too_many_fields)
{
	struct usbh_hid_report *report = &fixture->report;
	/* One more Input field than CONFIG_USBH_HID_REPORT_MAX_FIELDS supports */
	uint8_t report_data[4u + (CONFIG_USBH_HID_REPORT_MAX_FIELDS + 1)];

	report_data[0] = 0x75u; /* Report Size 1 */
	report_data[1] = 0x01u;
	report_data[2] = 0x95u; /* Report Count 1 */
	report_data[3] = 0x01u;
	for (size_t i = 0; i < CONFIG_USBH_HID_REPORT_MAX_FIELDS + 1; i++) {
		report_data[4u + i] = 0x80u; /* Input */
	}

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_equal(-ENOMEM, result, "Exceeding the maximum number of fields should fail");
}

ZTEST_F(hid_report_suite, test_hid_report_error_too_many_pushed_states)
{
	struct usbh_hid_report *report = &fixture->report;
	/* One more Push item than CONFIG_USBH_HID_REPORT_MAX_STATE_STACK supports */
	uint8_t report_data[CONFIG_USBH_HID_REPORT_MAX_STATE_STACK + 1];

	for (size_t i = 0; i < CONFIG_USBH_HID_REPORT_MAX_STATE_STACK + 1; i++) {
		report_data[i] = 0xA4u;
	}

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_equal(-ENOMEM, result,
		      "Exceeding the maximum global state stack depth should fail");
}

ZTEST_F(hid_report_suite, test_hid_report_error_mismatched_pop)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Pop with no matching Push item */
	uint8_t const report_data[] = {0xB4u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_not_ok(result, "A mismatched Pop should was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_item_exceeds_descriptor_length)
{
	struct usbh_hid_report *report = &fixture->report;
	/* Report Size item declaring a 4-byte value, but only 1 byte of value is claimed to be
	 * part of the descriptor. The extra bytes are real, allocated memory (so a defective
	 * parser reading past the declared length doesn't fault), they are just not supposed to
	 * be part of the descriptor being parsed.
	 */
	uint8_t const report_data[] = {0x77u, 0xAAu, 0xBBu, 0xCCu, 0xDDu};
	size_t const declared_length = 2u;

	int result = usbh_hid_report_parse(report, declared_length, report_data);

	zassert_not_ok(
		result,
		"An item whose declared value size exceeds the descriptor length was be accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_input_iterate_null_args)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	uint8_t const report_data[] = {
		0x75u, 0x08u, /* Report Size 8 */
		0x95u, 0x01u, /* Report Count 1 */
		0x81u, 0x02u, /* Input (Data,Var,Abs) */
	};
	uint8_t const data[] = {0x00u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	result = usbh_hid_report_input_iterate(NULL, sizeof(data), data, field_inspector,
					       &test_values);
	zassert_equal(-EINVAL, result, "A NULL report was accepted");

	result = usbh_hid_report_input_iterate(report, sizeof(data), NULL, field_inspector,
					       &test_values);
	zassert_equal(-EINVAL, result, "A NULL data buffer was accepted");

	result = usbh_hid_report_input_iterate(report, sizeof(data), data, NULL, &test_values);
	zassert_equal(-EINVAL, result, "A NULL callback was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_input_iterate_insufficient_data)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	/* A single 16-bit input field, requiring 2 bytes of report data */
	uint8_t const report_data[] = {
		0x75u, 0x10u, /* Report Size 16 */
		0x95u, 0x01u, /* Report Count 1 */
		0x81u, 0x02u, /* Input (Data,Var,Abs) */
	};
	/* Only 1 byte provided, but the field needs 2 */
	uint8_t const data[] = {0x00u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	result = usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector,
					       &test_values);
	zassert_equal(-EINVAL, result,
		      "Data shorter than the report's declared fields was accepted");
}

ZTEST_F(hid_report_suite, test_hid_report_error_input_iterate_zero_length_report_id)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	uint8_t const report_data[] = {
		0x85u, 0x01u, /* Report ID 1 */
		0x75u, 0x08u, /* Report Size 8 */
		0x95u, 0x01u, /* Report Count 1 */
		0x81u, 0x02u, /* Input (Data,Var,Abs) */
	};
	/* Real, allocated memory so a defective parser reading past data_length doesn't fault */
	uint8_t const data[] = {0xAAu};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	/* A zero-length payload cannot contain the report ID byte */
	result = usbh_hid_report_input_iterate(report, 0u, data, field_inspector, &test_values);
	zassert_equal(-EINVAL, result,
		      "A zero-length payload was accepted despite a report ID being expected");
}

ZTEST_F(hid_report_suite, test_hid_report_error_unknown_report_id)
{
	struct usbh_hid_report *report = &fixture->report;
	struct hid_test_values test_values = {};
	/* Two report variants, IDs 1 and 2 */
	uint8_t const report_data[] = {
		0x85u, 0x01u, /* Report ID 1 */
		0x75u, 0x08u, /* Report Size 8 */
		0x95u, 0x01u, /* Report Count 1 */
		0x81u, 0x02u, /* Input (Data,Var,Abs) */
		0x85u, 0x02u, /* Report ID 2 */
		0x81u, 0x02u, /* Input (Data,Var,Abs) */
	};
	/* Report ID 0x99 does not exist in the descriptor above */
	uint8_t const data[] = {0x99u, 0x00u};

	int result = usbh_hid_report_parse(report, sizeof(report_data), report_data);

	zassert_ok(result, "Could not parse report descriptor");

	result = usbh_hid_report_get_input_size(report, 0x99u);
	zassert_equal(-EINVAL, result, "An unknown report ID was accepted");

	result = usbh_hid_report_input_iterate(report, sizeof(data), data, field_inspector,
					       &test_values);
	zassert_equal(-EINVAL, result, "An unknown report ID was accepted");
}
