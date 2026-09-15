/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/canbus/j1939.h>

#include "J1939Ac.h"

/* Address claim PF (0xEE) as seen in a built 29-bit arbitration ID. */
#define J1939_AC_PDU_FORMAT ((j1939_pdu_format_t)0xEE)

/* Background j1939_task thread runs every CONFIG_J1939_TASK_INTERVAL_MS; give it several
 * ticks to drain the loopback CAN queue before we check results.
 */
#define RX_SETTLE_TIME K_MSEC(50)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

J1939_NODE_DEFINE(test_node,
	can_dev,
	0x20,  /* source address */
	100,   /* ID number */
	1,     /* manufacturer code */
	0,     /* ECU instance */
	0,     /* function instance */
	0,     /* function */
	0,     /* vehicle system */
	0,     /* vehicle system instance */
	0,     /* industry group */
	false  /* arbitrary address capable */
);

/* Captures the most recent address-claim frame transmitted by the node under test. */
static struct can_frame captured_claim_frame;
static volatile bool captured_claim_frame_valid;

static void capture_claim_frame(const struct device *dev, struct can_frame *frame, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if ((j1939_pdu_format_t)j1939_get_pdu_format(frame->id) == J1939_AC_PDU_FORMAT) {
		captured_claim_frame = *frame;
		captured_claim_frame_valid = true;
	}
}

/* Builds an 8-byte J1939 NAME array using the same bit layout as j1939_ac_name_config_to_byte_array,
 * so a synthetic "foreign" address-claim frame can be injected without a second J1939_NODE_DEFINE.
 */
static void encode_name_bytes(j1939_id_number_t id_number, j1939_manufacturer_code_t mfg_code,
			      j1939_ecu_instance_t ecu_instance,
			      j1939_function_instance_t function_instance,
			      j1939_function_t function, j1939_vehicle_system_t vehicle_system,
			      j1939_vehicle_system_instance_t vehicle_system_instance,
			      j1939_industry_group_t industry_group, bool arbitrary_capable,
			      uint8_t *array)
{
	array[0] = (uint8_t)LOBYTE(LOWORD(id_number));
	array[1] = (uint8_t)HIBYTE(LOWORD(id_number));
	array[2] = (uint8_t)(((uint8_t)((mfg_code & 0x07) << 5)) |
			     (LOBYTE(HIWORD(id_number)) & 0x1F));
	array[3] = (uint8_t)((mfg_code & 0x7FF) >> 3);
	array[4] = (uint8_t)(((function_instance & 0x1F) << 3) | (ecu_instance & 0x07));
	array[5] = (uint8_t)function;
	array[6] = (uint8_t)((vehicle_system & 0x7F) << 1);
	array[7] = (uint8_t)(((industry_group & 0x07) << 4) | (vehicle_system_instance & 0x0F));

	if (arbitrary_capable) {
		array[7] |= 0x80;
	}
}

/*
 * Address claim frames are broadcast (PDU-Specific = J1939_GLOBAL_ADDRESS). The default routing
 * path (j1939_route_without_loopback(), active since J1939_LOOPBACK_ENABLE is off) only forwards
 * frames whose PDU-Specific byte matches one of our own node source addresses, so a broadcast
 * frame sent via can_send()+j1939_process_rx_msgs() would never reach j1939_ac_is_received(). See
 * repo memory J1939_core_dispatch_notes.md. Call the routing handler directly instead.
 */
static void inject_address_claim(j1939_source_address_t source, const uint8_t *name_bytes)
{
	struct can_frame frame = {0};

	frame.flags = CAN_FRAME_IDE;
	frame.id = j1939_build_message_id(false, false, J1939_Priority_6,
					  j1939_build_pgn_from_pdu(J1939_AC_PDU_FORMAT,
								  J1939_GLOBAL_ADDRESS),
					  source);
	frame.dlc = 8;
	memcpy(frame.data, name_bytes, 8);

	(void)j1939_ac_is_received(&frame, &test_node);
}

/* j1939_task() unconditionally calls into these app-provided hooks (see J1939.c); the background
 * task thread started by j1939_init() reaches them regardless of whether a test exercises ECU/SW
 * ID reporting, so the test binary must supply them to link.
 */
void j1939_app_get_device_info(j1939_device_info_t *device_info)
{
	static const char serial[] = "ac-test";
	static const char revision[] = "1.0";
	static const char part_number[] = "AC-TEST";

	*device_info = (j1939_device_info_t){
		.hardware_serial_number = serial,
		.hardware_revision = revision,
		.hardware_part_number = part_number,
	};
}

void j1939_get_software_id(uint8_t **software_id_ptr)
{
	ARG_UNUSED(software_id_ptr);
}

static void *ac_suite_setup(void)
{
	zassert_true(device_is_ready(can_dev), "CAN device not ready");
	/* can_loopback.c only dispatches transmitted frames to installed RX filters when the
	 * device mode includes CAN_MODE_LOOPBACK.
	 */
	zassert_equal(can_set_mode(can_dev, CAN_MODE_LOOPBACK), 0, "Failed to set loopback mode");
	can_start(can_dev);
	j1939_init();
	k_sleep(RX_SETTLE_TIME);
	return NULL;
}

static void ac_before(void *fixture)
{
	ARG_UNUSED(fixture);
	captured_claim_frame_valid = false;
	j1939_ac_init();
}

ZTEST_SUITE(j1939_ac, NULL, ac_suite_setup, ac_before, NULL, NULL);

/* ── Stateless helper APIs ── */

ZTEST(j1939_ac, test_ac_name_config_to_byte_array_matches_node_fields)
{
	uint8_t expected[8];
	uint8_t actual[8];

	encode_name_bytes(100, 1, 0, 0, 0, 0, 0, 0, false, expected);
	j1939_ac_name_config_to_byte_array(&test_node, actual);

	zassert_mem_equal(actual, expected, sizeof(expected),
			   "Encoded NAME bytes do not match node's compile-time fields");
}

ZTEST(j1939_ac, test_ac_get_name_array_node_matches_name_config_to_byte_array)
{
	uint8_t via_array_api[8];
	uint8_t via_direct_call[8];

	j1939_ac_get_name_array_node(&test_node, via_array_api);
	j1939_ac_name_config_to_byte_array(&test_node, via_direct_call);

	zassert_mem_equal(via_array_api, via_direct_call, sizeof(via_array_api),
			   "get_name_array_node should match name_config_to_byte_array");
}

ZTEST(j1939_ac, test_ac_is_name_match_equal_names)
{
	j1939_name_t name1 = {.idNumber = 42, .mfgCode = 5};
	j1939_name_t name2 = {.idNumber = 42, .mfgCode = 5};

	zassert_true(j1939_ac_is_name_match(&name1, &name2), "Identical names should match");
}

ZTEST(j1939_ac, test_ac_is_name_match_differing_names)
{
	j1939_name_t name1 = {.idNumber = 42, .mfgCode = 5};
	j1939_name_t name2 = {.idNumber = 42, .mfgCode = 6};

	zassert_false(j1939_ac_is_name_match(&name1, &name2), "Differing names should not match");
}

ZTEST(j1939_ac, test_ac_set_get_name_config_round_trip)
{
	j1939_name_t set_name = {
		.idNumber = 12345,
		.mfgCode = 99,
		.ecuInstance = 2,
		.functionInstance = 3,
		.function = 4,
		.vehicleSystem = 5,
		.vehicleSystemInstance = 6,
		.industryGroup = 7,
		.isSelfConfig = false,
		.reservedBit = 0,
	};
	j1939_name_t read_name = {0};

	zassert_true(j1939_ac_set_name_config_node(&test_node, &set_name),
		     "Setting the node NAME config should succeed");
	zassert_true(j1939_ac_get_name_config_node(&test_node, &read_name),
		     "Getting the node NAME config should succeed");
	zassert_true(j1939_ac_is_name_match(&set_name, &read_name),
		     "Read back NAME should match what was set");
}

ZTEST(j1939_ac, test_ac_get_source_address)
{
	zassert_equal(j1939_ac_get_source_address(&test_node), 0x20,
		      "Source address should match the compile-time configured value");
}

ZTEST(j1939_ac, test_ac_initial_state_is_waiting_startup_init)
{
	zassert_equal(j1939_ac_get_state(&test_node), J1939_AC_STATE_WAITING_STARTUP_INIT,
		      "Freshly initialized node should start in WAITING_STARTUP_INIT");
}

ZTEST(j1939_ac, test_ac_initial_claim_count_is_zero)
{
	zassert_equal(j1939_ac_get_claimed_address_count(&test_node), 0,
		      "No addresses should be recorded immediately after init");
}

/* ── Loopback-driven recording of externally claimed addresses ── */

ZTEST(j1939_ac, test_ac_records_foreign_address_claim)
{
	const j1939_source_address_t foreign_source = 0x30;
	uint8_t name_bytes[8];

	encode_name_bytes(555, 3, 1, 2, 3, 4, 5, 6, false, name_bytes);
	inject_address_claim(foreign_source, name_bytes);

	zassert_true(j1939_ac_address_has_been_claimed(foreign_source, &test_node),
		     "Foreign source address should be recorded as claimed");
	zassert_equal(j1939_ac_get_claimed_address_count(&test_node), 1,
		      "Claimed address count should increment by one");
}

ZTEST(j1939_ac, test_ac_own_source_address_not_recorded_as_foreign)
{
	zassert_false(j1939_ac_address_has_been_claimed(0x20, &test_node),
		      "Our own source address should not appear as a foreign claim");
}

ZTEST(j1939_ac, test_ac_duplicate_claim_does_not_grow_count)
{
	const j1939_source_address_t foreign_source = 0x31;
	uint8_t name_bytes[8];

	encode_name_bytes(777, 4, 0, 0, 0, 0, 0, 0, false, name_bytes);
	inject_address_claim(foreign_source, name_bytes);
	inject_address_claim(foreign_source, name_bytes);

	zassert_equal(j1939_ac_get_claimed_address_count(&test_node), 1,
		      "Re-claiming the same NAME/source should update in place, not grow the table");
}

ZTEST(j1939_ac, test_ac_table_index_round_trip)
{
	const j1939_source_address_t foreign_source = 0x32;
	uint8_t name_bytes[8];
	uint8_t index;

	encode_name_bytes(888, 2, 0, 0, 0, 0, 0, 0, false, name_bytes);
	inject_address_claim(foreign_source, name_bytes);

	index = j1939_ac_get_claimed_table_index_from_source_address(foreign_source, &test_node);
	zassert_not_equal(index, 0xFF, "Foreign source address should be found in the table");
	zassert_equal(j1939_ac_get_source_addressFromTableIndex(&test_node, index), foreign_source,
		      "Table index should map back to the same source address");
}

ZTEST(j1939_ac, test_ac_table_index_out_of_range)
{
	uint8_t index = j1939_ac_get_claimed_table_index_from_source_address(0x99, &test_node);

	zassert_equal(index, 0xFF, "Unknown source address should return 0xFF");
	zassert_equal(j1939_ac_get_source_addressFromTableIndex(&test_node, 0), J1939_NULL_ADDRESS,
		      "Out-of-range table index should return J1939_NULL_ADDRESS");
}

/* ── Address claim request handling ── */

ZTEST(j1939_ac, test_ac_process_request_transmits_own_claim_when_claimed)
{
	int filter_id;
	struct can_filter filter = {.mask = 0, .flags = CAN_FILTER_IDE};
	uint8_t expected[8];

	filter_id = can_add_rx_filter(can_dev, capture_claim_frame, NULL, &filter);
	zassert_true(filter_id >= 0, "Failed to install test capture filter");

	/* Force the claimed state directly rather than waiting on the address-claim timers,
	 * since the timing-based state machine is out of scope for this suite.
	 */
	test_node.node_state = J1939_AC_STATE_CLAIMED;

	j1939_ac_process_request(&test_node);
	k_sleep(RX_SETTLE_TIME);

	zassert_true(captured_claim_frame_valid,
		     "Expected the node to transmit its address claim");

	j1939_ac_name_config_to_byte_array(&test_node, expected);
	zassert_mem_equal(captured_claim_frame.data, expected, sizeof(expected),
			   "Transmitted claim payload should match the node's NAME");

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(j1939_ac, test_ac_process_request_defers_when_not_yet_claimed)
{
	test_node.node_state = J1939_AC_STATE_WAITING;
	test_node.j1939_ac_is_requested = false;

	j1939_ac_process_request(&test_node);

	zassert_true(test_node.j1939_ac_is_requested,
		     "Request should be deferred (flagged) while address claim is pending");
}
