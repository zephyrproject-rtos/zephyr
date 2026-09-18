/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for the KNX object interface (property read/write/description).
 *
 * Strategy: include object_interface.c + object_property_types.h directly and
 * provide a minimal set of test property tables.  We do NOT include the real
 * object_device.c / object_address_table.c / etc. — those have SYS_INIT and
 * GPIO dependencies.  Instead we define small test tables here.
 *
 * Tests cover:
 *   1. interface_read_property  — known PID, unknown PID, array indexing
 *   2. interface_write_property — scalar write, array element write
 *   3. interface_read_property_description — returned metadata
 *   4. load_state_machine_event — state transitions
 *   5. PID_OBJECT_NAME typo (ANALYZE.MD §6.1) — verifies current broken
 *      behaviour so that the fix can be validated by making these tests pass
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Logging stubs
 * ============================================================
 */

/*
 * object_interface.c includes <zephyr/logging/log.h> unconditionally.  Fake
 * its header guard so that include is a no-op and these stubs are what it
 * actually calls, instead of the real macros redefining them.
 */
#define ZEPHYR_INCLUDE_LOGGING_LOG_H_
#define LOG_MODULE_REGISTER(...)
#define LOG_DBG(...)
#define LOG_WRN(...)
#define LOG_ERR(...)
#define ARG_UNUSED(x)              (void)(x)
#define CONFIG_KNX_STACK_LOG_LEVEL 0

/* ============================================================
 * Include pure property machinery (no real objects)
 * ============================================================
 */

#include "../../../../subsys/knx/object_property_types.h"

/* ============================================================
 * Test property tables (stand-ins for the real object tables)
 * ============================================================
 */

#define TEST_NUMBER_OF_INTERFACE 2
#undef NUMBER_OF_INTERFACE
#define NUMBER_OF_INTERFACE TEST_NUMBER_OF_INTERFACE

static knx_u16_be_t t_object_type_0 = KNX_U16_BE(OT_DEVICE);
static char t_object_name_0[8] = "TestObj";
static uint8_t t_scalar_u8 = 0x42;
static uint16_t t_array_u16[4] = {3, 0x1001, 0x2002, 0x3003}; /* [0]=count */
static LoadState t_load_state = LS_UNLOADED;
/* Backs a read-only property whose write level is permissive — see the table. */
static knx_u16_be_t t_readonly_permissive = KNX_U16_BE(0x1234);
/* PID_IO_LIST element count, host order (see object_device.c: nr_of_elem is read
 * natively and re-emitted big-endian by interface_read_property()).
 */
static uint16_t t_nb_interface = TEST_NUMBER_OF_INTERFACE;
extern uint8_t object_interfaces_types[TEST_NUMBER_OF_INTERFACE + 1][2];
/*
 * [0] holds the count in KNX WIRE order, like the real GrAT/GrOAT/GrOT
 * PID_TABLE — 0x0200 is the little-endian in-memory value of
 * wire bytes 00 02, i.e. count = 2 once ntohs()'d.
 */
static uint16_t t_be_count_array[3] = {0x0200, 0x1001, 0x2002};
static ErrorCode t_error_code = E_NO_FAULT;

/* Object 0: a minimal device-like object */
/* clang-format off */
struct property test_properties_0[] = {
	{
		/* PID_OBJECT_TYPE */
		.description = {
			.property_id = PID_OBJECT_TYPE,
			.write_enable = false,
			.property_datatype = PDT_UNSIGNED_INT,
			.max_nr_of_elem = 1,
			.access = ReadLv3 | WriteLv0,
		},
		.value = {.nr_of_elem = NULL, .data = &t_object_type_0},
	},
	{
		/* PID_OBJECT_NAME — correctly using PID_OBJECT_NAME here (unlike the bug) */
		.description = {
			.property_id = PID_OBJECT_NAME,
			.write_enable = false,
			.property_datatype = PDT_UNSIGNED_CHAR,
			.max_nr_of_elem = sizeof(t_object_name_0),
			.access = ReadLv3 | WriteLv0,
		},
		.value = {.nr_of_elem = NULL, .data = t_object_name_0},
	},
	{
		/* PID_ROUTING_COUNT — a writable scalar */
		.description = {
			.property_id = PID_ROUTING_COUNT,
			.write_enable = true,
			.property_datatype = PDT_UNSIGNED_CHAR,
			.max_nr_of_elem = 1,
			.access = ReadLv3 | WriteLv3,
		},
		.value = {.nr_of_elem = NULL, .data = &t_scalar_u8},
	},
	{
		/* PID_TABLE — a writable array; [0] holds count */
		.description = {
			.property_id = PID_TABLE,
			.write_enable = true,
			.property_datatype = PDT_UNSIGNED_INT,
			.max_nr_of_elem = 4,
			.access = ReadLv3 | WriteLv3,
		},
		.value = {.nr_of_elem = &t_array_u16[0], .data = t_array_u16},
	},
	{
		/* PID_LOAD_STATE_CONTROL */
		.description = {
			.property_id = PID_LOAD_STATE_CONTROL,
			.write_enable = true,
			.property_datatype = PDT_CONTROL,
			.max_nr_of_elem = 1,
			.access = ReadLv3 | WriteLv3,
		},
		.value = {.nr_of_elem = NULL, .data = &t_load_state},
	},
	{
		/*
		 * Read-only but with a PERMISSIVE write level, deliberately.
		 *
		 * Every real read-only property pairs write_enable = false with WriteLv0,
		 * so either check alone would reject a write and a test using one cannot
		 * tell which mechanism fired. This entry isolates write_enable: the
		 * access level can never be the reason a write to it is refused.
		 */
		.description = {
			.property_id = PID_MANUFACTURER_ID,
			.write_enable = false,
			.property_datatype = PDT_UNSIGNED_INT,
			.max_nr_of_elem = 1,
			.access = ReadLv3 | WriteLv3,
		},
		.value = {.nr_of_elem = NULL, .data = &t_readonly_permissive},
	},
	{
		/*
		 * PID_IO_LIST — an array of wire-order uint16, like the real one. Element
		 * 0 mirrors the count; the types live at 1..N.
		 */
		.description = {
			.property_id = PID_IO_LIST,
			.write_enable = false,
			.property_datatype = PDT_UNSIGNED_INT,
			.max_nr_of_elem = TEST_NUMBER_OF_INTERFACE + 1,
			.access = ReadLv3 | WriteLv0,
		},
		.value = {.nr_of_elem = &t_nb_interface, .data = &object_interfaces_types},
	},
	{
		/*
		 * PID_MCB_TABLE — stands in for the real GrAT/GrOAT/GrOT PID_TABLE
		 * shape: count_is_be = true, unlike PID_TABLE and PID_IO_LIST above
		 * (this is what used to be keyed off
		 * property_datatype == PDT_GENERIC_04 instead of an explicit flag).
		 */
		.description = {
			.property_id = PID_MCB_TABLE,
			.write_enable = false,
			.property_datatype = PDT_UNSIGNED_INT,
			.max_nr_of_elem = 2,
			.access = ReadLv3 | WriteLv0,
			.count_is_be = true,
		},
		.value = {.nr_of_elem = &t_be_count_array[0], .data = t_be_count_array},
	},
	{
		/*
		 * PID_ERROR_CODE — mirrors object_address_table.c's entry, so
		 * set_error_code() (object_interface.c) has something to write
		 * to for object 0 in this test table.
		 */
		.description = {
			.property_id = PID_ERROR_CODE,
			.write_enable = false,
			.property_datatype = PDT_ENUM8,
			.max_nr_of_elem = 1,
			.access = ReadLv3 | WriteLv0,
		},
		.value = {.nr_of_elem = NULL, .data = &t_error_code},
	},
	{.description = {.property_id = PID_LAST}},
};
/* clang-format on */

/* Object 1: a second object with a duplicate PID_OBJECT_TYPE (simulates the
 * known bug in address_table_properties where PID_OBJECT_NAME entry has
 * property_id = PID_OBJECT_TYPE instead of PID_OBJECT_NAME — ANALYZE.MD §6.1)
 */
static knx_u16_be_t t_object_type_1 = KNX_U16_BE(OT_ADDR_TABLE);
static char t_object_name_1[6] = "Buggy";

/* clang-format off */
struct property test_properties_1_buggy[] = {
	{
		/* PID_OBJECT_TYPE — correct */
		.description = {
			.property_id = PID_OBJECT_TYPE,
			.write_enable = false,
			.property_datatype = PDT_UNSIGNED_INT,
			.max_nr_of_elem = 1,
			.access = ReadLv3,
		},
		.value = {.nr_of_elem = NULL, .data = &t_object_type_1},
	},
	{
		/* BUG: uses PID_OBJECT_TYPE instead of PID_OBJECT_NAME */
		.description = {
			.property_id = PID_OBJECT_TYPE, /* <-- bug from real code */
			.write_enable = false,
			.property_datatype = PDT_UNSIGNED_CHAR,
			.max_nr_of_elem = sizeof(t_object_name_1),
			.access = ReadLv3,
		},
		.value = {.nr_of_elem = NULL, .data = t_object_name_1},
	},
	{.description = {.property_id = PID_LAST}},
};
/* clang-format on */

/* Wire up the interface — mirrors object_interface.c layout */
#include "../../../../subsys/knx/object_interface.h"

/* Provide the externs that object_interface.c expects */
struct property *object_interfaces[TEST_NUMBER_OF_INTERFACE] = {
	test_properties_0,
	test_properties_1_buggy,
};

/* Wire order, like the real table. */
knx_u16_be_t object_interfaces_types[TEST_NUMBER_OF_INTERFACE + 1] = {
	KNX_U16_BE(TEST_NUMBER_OF_INTERFACE), KNX_U16_BE(OT_DEVICE), KNX_U16_BE(OT_ADDR_TABLE)};

void object_device_properties_written(PropertyID id)
{
	ARG_UNUSED(id);
}
void address_table_properties_written(PropertyID id)
{
	ARG_UNUSED(id);
}
void object_association_table_properties_written(PropertyID id)
{
	ARG_UNUSED(id);
}
void object_application_program_properties_written(PropertyID id)
{
	ARG_UNUSED(id);
}
void object_group_table_properties_written(PropertyID id)
{
	ARG_UNUSED(id);
}

/* properties_written array — must match NUMBER_OF_INTERFACE (= TEST_NUMBER_OF_INTERFACE = 2) */
void (*properties_written[TEST_NUMBER_OF_INTERFACE])(PropertyID id) = {
	object_device_properties_written,
	address_table_properties_written,
};

/* z_log_minimal_printk is referenced by LOG_ERR/LOG_DBG even when CONFIG_LOG
 * is forced on by the unittest framework.  Provide a silent stub.
 */
#include <stdarg.h>
void z_log_minimal_printk(const char *fmt, ...)
{
	(void)fmt;
}

/* ============================================================
 * object_memory stubs
 *
 * object_interface.c reaches into the persistent memory image for the load
 * state machine's Data Relative Allocation handling (segment base address,
 * segment capacity, and the fill operation).  Provide a real struct Memory so
 * the fill actually writes somewhere bounded, and map the two test objects
 * onto the address/association segments.
 * ============================================================
 */

#include "../../../../subsys/knx/object_memory.h"

struct Memory __memory;
static bool t_memory_modified;

uint32_t memory_table_offset(uint8_t object_index)
{
	switch (object_index) {
	case 0:
		return offsetof(struct Memory, addresses);
	case 1:
		return offsetof(struct Memory, associations);
	default:
		return 0;
	}
}

uint32_t memory_table_capacity(uint8_t object_index)
{
	switch (object_index) {
	case 0:
		return sizeof(__memory.addresses);
	case 1:
		return sizeof(__memory.associations);
	default:
		return 0;
	}
}

void memory_modified(void)
{
	t_memory_modified = true;
}

/* Include the implementation under test */
#include "../../../../subsys/knx/object_interface.c"

/* ============================================================
 * Test suite 1: interface_read_property
 * ============================================================
 */

ZTEST_SUITE(oi_read, NULL, NULL, NULL, NULL, NULL);

ZTEST(oi_read, test_read_scalar_pid_object_type)
{
	uint8_t data[4];
	uint32_t count = 1;

	int len = interface_read_property(0, PID_OBJECT_TYPE, 1, &count, data, sizeof(data));

	zassert_equal(len, 2, "PDT_UNSIGNED_INT is 2 bytes");
	uint16_t val = ((uint16_t)data[0] << 8) | data[1];

	zassert_equal(val, OT_DEVICE, "object type must be OT_DEVICE");
}

ZTEST(oi_read, test_read_scalar_routing_count)
{
	t_scalar_u8 = 0x42;
	uint8_t data[2];
	uint32_t count = 1;

	int len = interface_read_property(0, PID_ROUTING_COUNT, 1, &count, data, sizeof(data));

	zassert_equal(len, 1, "PDT_UNSIGNED_CHAR is 1 byte");
	zassert_equal(data[0], 0x42);
}

/*
 * 03_03_07 §3.4.4.1 (p.63): start_index = 0 on a SCALAR must
 * report its element count (always 1), not fall through to the value path.
 * The gate used to be `max_nr_of_elem > 1 && start_index == 0`, which is
 * false for a scalar (max_nr_of_elem == 1), so this request fell through and
 * returned the raw value 0x42 as if it were the count.
 */
ZTEST(oi_read, test_read_scalar_count_at_index_0)
{
	t_scalar_u8 = 0x42;
	uint8_t data[4];
	uint32_t count = 1;

	int len = interface_read_property(0, PID_ROUTING_COUNT, 0, &count, data, sizeof(data));

	zassert_equal(len, 2, "count response is always 2 bytes");
	uint16_t reported_count = ((uint16_t)data[0] << 8) | data[1];

	zassert_equal(reported_count, 1, "a scalar's element count is always 1, got %02x %02x",
		      data[0], data[1]);
}

/*
 * The object-0 test above compares against OT_DEVICE == 0, which
 * reads the same in either byte order — so it could never catch a byte-order
 * bug. Object 1's type is OT_ADDR_TABLE == 1, which serialises as 00 01 in wire
 * order and 01 00 natively, so this one does.
 */
ZTEST(oi_read, test_multi_octet_property_is_big_endian_on_the_wire)
{
	uint8_t data[4] = {0xAA, 0xAA, 0xAA, 0xAA};
	uint32_t count = 1;

	int len = interface_read_property(1, PID_OBJECT_TYPE, 1, &count, data, sizeof(data));

	zassert_equal(len, 2, "PDT_UNSIGNED_INT is 2 octets");
	zassert_equal(data[0], 0x00, "high octet first (KNX wire order)");
	zassert_equal(data[1], OT_ADDR_TABLE,
		      "low octet second — got %02x %02x, which is host order", data[0], data[1]);
}

ZTEST(oi_read, test_array_elements_are_big_endian_on_the_wire)
{
	uint8_t data[8];
	uint32_t count = 1;

	/* object_interfaces_types[2] is OT_ADDR_TABLE in the test table. */
	int len = interface_read_property(0, PID_IO_LIST, 2, &count, data, sizeof(data));

	zassert_equal(len, 2, "PDT_UNSIGNED_INT array element is 2 octets");
	zassert_equal(data[0], 0x00);
	zassert_equal(data[1], OT_ADDR_TABLE);
}

ZTEST(oi_read, test_read_array_count_at_index_0)
{
	/* start_index = 0 on an array property → returns current nr_of_elem as U16 */
	uint8_t data[4];
	uint32_t count = 1;

	int len = interface_read_property(0, PID_TABLE, 0, &count, data, sizeof(data));

	zassert_equal(len, 2, "array count response is 2 bytes");
	uint16_t reported_count = ((uint16_t)data[0] << 8) | data[1];

	zassert_equal(reported_count, t_array_u16[0], "reported count must match array[0]");
}

/*
 * The byte-swap used to be keyed off
 * property_datatype == PDT_GENERIC_04, which broke the moment a
 * PDT_GENERIC_04 property needed a host-order count (or vice versa) — the
 * exact GrAT/GrOT case, whose real per-entry PDT isn't PDT_GENERIC_04 but
 * whose count is still wire-order. count_is_be is the explicit replacement;
 * this table's PID_MCB_TABLE entry is PDT_UNSIGNED_INT with count_is_be =
 * true, the opposite pairing from PID_TABLE just above.
 */
ZTEST(oi_read, test_count_is_be_swaps_a_wire_order_count)
{
	uint8_t data[4];
	uint32_t count = 1;

	int len = interface_read_property(0, PID_MCB_TABLE, 0, &count, data, sizeof(data));

	zassert_equal(len, 2, "array count response is 2 bytes");
	uint16_t reported_count = ((uint16_t)data[0] << 8) | data[1];

	zassert_equal(reported_count, 2,
		      "0x0200 stored (wire bytes 00 02) must be reported as count 2, "
		      "not 0x0200 read natively");
}

ZTEST(oi_read, test_read_array_element)
{
	/* start_index = 1 → first real element, t_array_u16[1] = 0x1001
	 * The property is stored and returned in native byte order.
	 * Use memcpy to avoid endianness issues when comparing.
	 */
	uint8_t data[4];
	uint32_t count = 1;

	int len = interface_read_property(0, PID_TABLE, 1, &count, data, sizeof(data));

	zassert_equal(len, 2, "one PDT_UNSIGNED_INT element = 2 bytes");
	uint16_t val;

	memcpy(&val, data, sizeof(val));
	zassert_equal(val, t_array_u16[1], "first array element must match (native byte order)");
}

ZTEST(oi_read, test_read_unknown_pid_returns_zero)
{
	uint8_t data[4];
	uint32_t count = 1;

	int len = interface_read_property(0, 0xFF, 1, &count, data, sizeof(data));

	zassert_equal(len, 0, "unknown PID must return 0 bytes");
}

ZTEST(oi_read, test_read_invalid_object_index)
{
	uint8_t data[4];
	uint32_t count = 1;

	int len = interface_read_property(99, PID_OBJECT_TYPE, 1, &count, data, sizeof(data));

	zassert_equal(len, 0, "out-of-range object index must return 0 bytes");
}

/* ============================================================
 * Test suite 2: interface_write_property
 * ============================================================
 */

ZTEST_SUITE(oi_write, NULL, NULL, NULL, NULL, NULL);

ZTEST(oi_write, test_write_scalar)
{
	/* Phase 2.5 fix: scalar properties (nr_of_elem == NULL) now write correctly. */
	t_scalar_u8 = 0x42;
	uint8_t new_val = 0x07;
	bool ok = interface_write_property(0, PID_ROUTING_COUNT, 1, 1, &new_val, sizeof(new_val));

	zassert_true(ok, "scalar write must succeed after Phase 2.5 fix");
	zassert_equal(t_scalar_u8, 0x07, "scalar value must be updated");
}

ZTEST(oi_write, test_write_unknown_pid_fails)
{
	uint8_t val = 0x01;
	bool ok = interface_write_property(0, 0xFF, 1, 1, &val, sizeof(val));

	zassert_false(ok, "write to unknown PID must fail");
}

ZTEST(oi_write, test_write_invalid_object_fails)
{
	uint8_t val = 0x01;
	bool ok = interface_write_property(99, PID_ROUTING_COUNT, 1, 1, &val, sizeof(val));

	zassert_false(ok);
}

/* ============================================================
 * Test suite 2c: element count vs octet count
 *
 * interface_write_property() used to receive the PDU's octet count in its
 * element-count parameter and then write count * data_size octets.  It was
 * masked in production because every writeable property has either
 * max_nr_of_elem == 1 or a 1-octet datatype — PID_TABLE (PDT_UNSIGNED_INT[])
 * is exactly the shape that unmasks it, and the test table has one.
 * ============================================================
 */

ZTEST_SUITE(oi_elem_count, NULL, NULL, NULL, NULL, NULL);

static void array_reset(void)
{
	t_array_u16[0] = 3;
	t_array_u16[1] = 0x1001;
	t_array_u16[2] = 0x2002;
	t_array_u16[3] = 0x3003;
	interface_reset_access_level();
}

/*
 * The regression itself: one 2-octet element must write exactly one element.
 * With the octet count passed as the element count, this wrote TWO elements —
 * the second one from data[2..3], i.e. from whatever followed the received
 * octets in the packet buffer.
 */
ZTEST(oi_elem_count, test_one_element_writes_one_element)
{
	uint8_t data[2] = {0xAA, 0xBB};

	array_reset();
	zassert_true(interface_write_property(0, PID_TABLE, 1, 1, data, sizeof(data)),
		     "a well-formed single-element write must succeed");

	uint8_t *slot = (uint8_t *)&t_array_u16[1];

	zassert_equal(slot[0], 0xAA, "element 1 must take the received octets");
	zassert_equal(slot[1], 0xBB);
	zassert_equal(t_array_u16[2], 0x2002,
		      "element 2 must be untouched — writing it means the octet count "
		      "was used as an element count");
}

ZTEST(oi_elem_count, test_multiple_elements_write_exactly_that_many)
{
	uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};

	array_reset();
	zassert_true(interface_write_property(0, PID_TABLE, 1, 2, data, sizeof(data)));

	uint8_t *slots = (uint8_t *)&t_array_u16[1];

	zassert_mem_equal(slots, data, sizeof(data), "both elements must be written");
	zassert_equal(t_array_u16[3], 0x3003, "and nothing beyond them");
}

/*
 * A PDU that announces more elements than it carries octets for must be
 * refused, not truncated: the missing octets would otherwise be read from past
 * the received data.
 */
ZTEST(oi_elem_count, test_short_pdu_is_refused)
{
	uint8_t data[2] = {0xAA, 0xBB};

	array_reset();
	zassert_false(interface_write_property(0, PID_TABLE, 1, 2, data, sizeof(data)),
		      "2 elements of 2 octets need 4 octets; only 2 were received");
	zassert_equal(t_array_u16[1], 0x1001, "a refused write must change nothing");
	zassert_equal(t_array_u16[2], 0x2002);
}

/* Same check on a scalar whose datatype is wider than one octet. */
ZTEST(oi_elem_count, test_short_pdu_is_refused_on_wide_scalar)
{
	uint8_t data[1] = {0xAA};
	uint16_t before = knx_u16_be_get(t_readonly_permissive);

	interface_reset_access_level();
	/* Temporarily make the wide read-only property writeable so only the
	 * length check can refuse this.
	 */
	test_properties_0[5].description.write_enable = true;
	zassert_equal(test_properties_0[5].description.property_id, PID_MANUFACTURER_ID,
		      "test table layout changed");

	zassert_false(interface_write_property(0, PID_MANUFACTURER_ID, 1, 1, data, sizeof(data)),
		      "one PDT_UNSIGNED_INT element needs 2 octets, not 1");
	zassert_equal(knx_u16_be_get(t_readonly_permissive), before);

	test_properties_0[5].description.write_enable = false;
}

/*
 * start_index is a 12-bit PDU field indexing the backing store directly, so
 * clamping the element count alone does not bound the write.
 */
ZTEST(oi_elem_count, test_write_past_the_last_slot_is_refused)
{
	uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};

	array_reset();
	zassert_false(interface_write_property(0, PID_TABLE, 3, 2, data, sizeof(data)),
		      "slots 3 and 4 in a 4-slot store: element 4 does not exist");
	zassert_equal(t_array_u16[3], 0x3003, "a refused write must change nothing");
}

/* ============================================================
 * Test suite 2b: access control
 *
 * write_enable and the access levels used to be published in
 * A_PropertyDescription_Response and then ignored, so anything on the bus could
 * overwrite PID_SERIAL_NUMBER, PID_OBJECT_TYPE or PID_MANUFACTURER_ID — all
 * mandatory read-only (m/x) in Volume 6 §A.2.3.
 * ============================================================
 */

ZTEST_SUITE(oi_access, NULL, NULL, NULL, NULL, NULL);

static void access_reset(void)
{
	interface_reset_access_level();
}

/*
 * Isolates write_enable from the access level: PID_MANUFACTURER_ID is declared
 * read-only but WriteLv3 in the test table, so the current level (3) satisfies
 * the access check and only write_enable can refuse the write.
 */
ZTEST(oi_access, test_read_only_property_rejects_write)
{
	uint16_t before = knx_u16_be_get(t_readonly_permissive);
	uint16_t val = 0xBEEF;

	access_reset();
	zassert_false(interface_write_property(0, PID_MANUFACTURER_ID, 1, 1, (uint8_t *)&val,
					       sizeof(val)),
		      "a write_enable = false property must refuse the write even when "
		      "the access level would allow it");
	zassert_equal(knx_u16_be_get(t_readonly_permissive), before,
		      "a refused write must not modify the value");

	/* Level 0 is the most privileged and must still not override read-only. */
	interface_set_access_level(0);
	zassert_false(interface_write_property(0, PID_MANUFACTURER_ID, 1, 1, (uint8_t *)&val,
					       sizeof(val)),
		      "not even level 0 may write a read-only property");
	zassert_equal(knx_u16_be_get(t_readonly_permissive), before);
	access_reset();
}

/* And the pairing used by every real read-only property: write_enable = false
 * together with WriteLv0, where both checks agree.
 */
ZTEST(oi_access, test_read_only_with_restrictive_level_rejects_write)
{
	uint16_t before = knx_u16_be_get(t_object_type_0);
	uint16_t val = 0xBEEF;

	access_reset();
	zassert_false(
		interface_write_property(0, PID_OBJECT_TYPE, 1, 1, (uint8_t *)&val, sizeof(val)));
	zassert_equal(knx_u16_be_get(t_object_type_0), before);
}

ZTEST(oi_access, test_writable_property_accepts_write_at_default_level)
{
	uint8_t val = 0x5A;

	access_reset();
	zassert_equal(interface_access_level(), 3, "the default level is the least privileged");
	/* PID_ROUTING_COUNT is WriteLv3, so level 3 is enough — an ETS that has
	 * not authorised must still be able to write it.
	 */
	zassert_true(interface_write_property(0, PID_ROUTING_COUNT, 1, 1, &val, sizeof(val)));
	zassert_equal(t_scalar_u8, 0x5A);
}

ZTEST(oi_access, test_insufficient_access_level_rejects_write)
{
	uint8_t val = 0x11;

	access_reset();
	t_scalar_u8 = 0x42;

	/*
	 * Tighten the property to WriteLv0 and confirm the default level 3 is
	 * refused, then that level 0 is accepted. Restored afterwards so the
	 * other suites see the original table.
	 */
	uint8_t saved = test_properties_0[2].description.access;

	test_properties_0[2].description.access = ReadLv3 | WriteLv0;

	zassert_false(interface_write_property(0, PID_ROUTING_COUNT, 1, 1, &val, sizeof(val)),
		      "level 3 must not be allowed to write a WriteLv0 property");
	zassert_equal(t_scalar_u8, 0x42, "the value must be untouched");

	interface_set_access_level(0);
	zassert_true(interface_write_property(0, PID_ROUTING_COUNT, 1, 1, &val, sizeof(val)),
		     "level 0 is the most privileged and must be allowed");
	zassert_equal(t_scalar_u8, 0x11);

	test_properties_0[2].description.access = saved;
	access_reset();
}

ZTEST(oi_access, test_level_is_clamped_and_reset)
{
	access_reset();
	zassert_equal(interface_access_level(), 3);

	interface_set_access_level(0);
	zassert_equal(interface_access_level(), 0);

	/* Out-of-range levels must fall back to the least privileged, never wrap
	 * into something more privileged.
	 */
	interface_set_access_level(0xFF);
	zassert_equal(interface_access_level(), 3);

	interface_set_access_level(0);
	interface_reset_access_level();
	zassert_equal(interface_access_level(), 3,
		      "closing the connection must drop the granted level");
}

/*
 * A load-control write is subject to the same checks as any other write — it
 * must not bypass them via the PID_LOAD_STATE_CONTROL short-circuit.
 */
ZTEST(oi_access, test_load_state_control_respects_access)
{
	uint8_t unload[10] = {LE_UNLOAD};
	uint8_t saved = test_properties_0[4].description.access;

	access_reset();
	zassert_equal(test_properties_0[4].description.property_id, PID_LOAD_STATE_CONTROL,
		      "test table layout changed");

	test_properties_0[4].description.access = ReadLv3 | WriteLv0;
	zassert_false(
		interface_write_property(0, PID_LOAD_STATE_CONTROL, 1, 1, unload, sizeof(unload)),
		"a load control write must honour the access level too");

	test_properties_0[4].description.access = saved;
	zassert_true(
		interface_write_property(0, PID_LOAD_STATE_CONTROL, 1, 1, unload, sizeof(unload)),
		"and succeed once the level is sufficient");
	access_reset();
}

/* ============================================================
 * Test suite 3: interface_read_property_description
 * ============================================================
 */

ZTEST_SUITE(oi_desc, NULL, NULL, NULL, NULL, NULL);

ZTEST(oi_desc, test_description_object_type)
{
	bool write_enable = true;
	uint8_t pdt = 0;
	uint16_t max_elem = 0;
	uint8_t access = 0;
	uint8_t idx_found = 0xFF;

	interface_read_property_description(0, PID_OBJECT_TYPE, 0, &write_enable, &pdt, &max_elem,
					    &access, &idx_found);

	zassert_false(write_enable, "PID_OBJECT_TYPE is read-only");
	zassert_equal(pdt, PDT_UNSIGNED_INT);
	zassert_equal(max_elem, 1);
}

ZTEST(oi_desc, test_description_unknown_returns_defaults)
{
	bool write_enable = true;
	uint8_t pdt = 0xFF;
	uint16_t max_elem = 0xFFFF;
	uint8_t access = 0xFF;
	uint8_t idx_found = 0;

	/* Feed known-bad values in; they should NOT be overwritten on miss */
	interface_read_property_description(0, 0xAB, 0, &write_enable, &pdt, &max_elem, &access,
					    &idx_found);

	zassert_equal(pdt, 0xFF, "unknown PID: pdt must be unchanged");
	zassert_equal(max_elem, 0xFFFF, "unknown PID: max_elem must be unchanged");
}

/* ============================================================
 * Test suite 4: load_state_machine_event
 * ============================================================
 */

ZTEST_SUITE(oi_load_state, NULL, NULL, NULL, NULL, NULL);

ZTEST(oi_load_state, test_unloaded_to_loading)
{
	t_load_state = LS_UNLOADED;
	uint8_t cmd[] = {LE_START_LOADING};

	load_state_machine_event(0, cmd, sizeof(cmd));
	zassert_equal(t_load_state, LS_LOADING);
}

ZTEST(oi_load_state, test_loading_to_loaded)
{
	t_load_state = LS_LOADING;
	uint8_t cmd[] = {LE_LOAD_COMPLETED};

	load_state_machine_event(0, cmd, sizeof(cmd));
	zassert_equal(t_load_state, LS_LOADED);
}

ZTEST(oi_load_state, test_loaded_to_unloaded)
{
	t_load_state = LS_LOADED;
	uint8_t cmd[] = {LE_UNLOAD};

	load_state_machine_event(0, cmd, sizeof(cmd));
	zassert_equal(t_load_state, LS_UNLOADED);
}

ZTEST(oi_load_state, test_error_to_unloaded)
{
	t_load_state = LS_ERROR;
	uint8_t cmd[] = {LE_UNLOAD};

	load_state_machine_event(0, cmd, sizeof(cmd));
	zassert_equal(t_load_state, LS_UNLOADED);
}

/*
 * 03_05_01 §4.23.2 / Volume 6 §A.2.3.2 (p.142): an unexpected
 * event must record WHY the object went to LS_ERROR in PID_ERROR_CODE, not
 * just LOG_ERR it. set_error_code() is a no-op when the object declares no
 * PID_ERROR_CODE, so this is the first table in this suite to add one.
 */
ZTEST(oi_load_state, test_unexpected_event_sets_error_code)
{
	t_load_state = LS_UNLOADED;
	t_error_code = E_NO_FAULT;
	uint8_t cmd[] = {0xFFu}; /* not a defined LoadEvent */

	load_state_machine_event(0, cmd, sizeof(cmd));

	zassert_equal(t_load_state, LS_ERROR);
	zassert_equal(t_error_code, E_GOT_UNDEF_LOAD_CMD,
		      "an undefined load command must be recorded, got %u", t_error_code);
}

/*
 * Whatever fault put the object into LS_ERROR no longer applies once an
 * event takes it back out — the code must not stay stuck at a stale value.
 */
ZTEST(oi_load_state, test_recovering_from_error_clears_error_code)
{
	t_load_state = LS_ERROR;
	t_error_code = E_GOT_UNDEF_LOAD_CMD;
	uint8_t cmd[] = {LE_UNLOAD};

	load_state_machine_event(0, cmd, sizeof(cmd));

	zassert_equal(t_load_state, LS_UNLOADED);
	zassert_equal(t_error_code, E_NO_FAULT,
		      "leaving LS_ERROR must clear PID_ERROR_CODE, got %u", t_error_code);
}

ZTEST(oi_load_state, test_noop_in_loaded)
{
	t_load_state = LS_LOADED;
	uint8_t cmd[] = {LE_NOOP};

	load_state_machine_event(0, cmd, sizeof(cmd));
	zassert_equal(t_load_state, LS_LOADED, "NOOP must not change state");
}

/*
 * The event octets come from the bus, and
 * LE_ADDITIONAL_LOAD_CONTROLS reads up to data[7] (sub-command, allocation
 * size, fill mode, fill byte).  A truncated PDU used to make all of those come
 * from whatever followed in the packet buffer — including the memset size.
 */
ZTEST(oi_load_state, test_additional_load_controls_needs_its_octets)
{
	uint8_t cmd[1] = {LE_ADDITIONAL_LOAD_CONTROLS};

	t_load_state = LS_LOADING;
	zassert_false(load_state_machine_event(0, cmd, sizeof(cmd)),
		      "a 1-octet LE_ADDITIONAL_LOAD_CONTROLS must be refused");
	zassert_equal(t_load_state, LS_LOADING,
		      "a malformed PDU must leave the state untouched — not even LS_ERROR, "
		      "which ETS would read as a failed allocation");
}

ZTEST(oi_load_state, test_empty_pdu_is_refused)
{
	uint8_t cmd[1] = {LE_UNLOAD};

	t_load_state = LS_LOADED;
	zassert_false(load_state_machine_event(0, cmd, 0), "there is no event octet to act on");
	zassert_equal(t_load_state, LS_LOADED);
}

/* A well-formed load-control write still has to reach the state machine. */
ZTEST(oi_load_state, test_write_property_forwards_a_valid_event)
{
	uint8_t cmd[1] = {LE_START_LOADING};

	t_load_state = LS_UNLOADED;
	interface_reset_access_level();
	zassert_true(interface_write_property(0, PID_LOAD_STATE_CONTROL, 1, 1, cmd, sizeof(cmd)));
	zassert_equal(t_load_state, LS_LOADING);
}

/* ...and a malformed one must be reported as a failed write, so L7 answers
 * nr_of_elem = 0 rather than a bare ACK.
 */
ZTEST(oi_load_state, test_write_property_reports_a_refused_event)
{
	uint8_t cmd[1] = {LE_ADDITIONAL_LOAD_CONTROLS};

	t_load_state = LS_LOADING;
	interface_reset_access_level();
	zassert_false(interface_write_property(0, PID_LOAD_STATE_CONTROL, 1, 1, cmd, sizeof(cmd)));
	zassert_equal(t_load_state, LS_LOADING);
}

/*
 * interface_unload_object() is the wrapper A_Restart__ind() (Master Reset)
 * calls for every segment it erases.  Erasing __memory
 * without also driving the Load State back to LS_UNLOADED left the object
 * reporting LS_LOADED over zeroed tables, which is confirmed in the field to
 * make ETS abort a partial download.
 */
ZTEST(oi_load_state, test_unload_object_drives_loaded_to_unloaded)
{
	t_load_state = LS_LOADED;
	interface_unload_object(0);
	zassert_equal(t_load_state, LS_UNLOADED,
		      "Master Reset must leave the Load State machine unloaded, "
		      "not still reporting LS_LOADED over erased tables");
}

/* ============================================================
 * Test suite 5: PID_OBJECT_NAME typo (ANALYZE.MD §6.1)
 *
 * These tests document the KNOWN BUG in the real property tables.
 * In test_properties_1_buggy the second entry wrongly uses PID_OBJECT_TYPE.
 * The expected behaviour WITH the bug: PID_OBJECT_NAME lookup fails, but
 * PID_OBJECT_TYPE hits a second instance (the name string, wrong type).
 * After the fix (§6.1) these tests should be updated to expect success.
 * ============================================================
 */

/*
 * The oi_pid_name_bug suite tests a DELIBERATE copy of the buggy table layout
 * (test_properties_1_buggy has PID_OBJECT_TYPE in the PID_OBJECT_NAME slot).
 * It verifies that the interface correctly fails to find PID_OBJECT_NAME when
 * the table has the typo.
 *
 * The production tables (object_address_table.c etc.) were fixed in Phase 2.2.
 * These tests remain to guard against future regressions.
 */

ZTEST_SUITE(oi_pid_name_bug, NULL, NULL, NULL, NULL, NULL);

ZTEST(oi_pid_name_bug, test_object_name_lookup_fails_in_buggy_table)
{
	/* Object 1 in the test uses the buggy table (PID_OBJECT_TYPE in name slot).
	 * PID_OBJECT_NAME query must return 0 bytes (not found).
	 */
	uint8_t data[16];
	uint32_t count = 1;

	int len = interface_read_property(1, PID_OBJECT_NAME, 1, &count, data, sizeof(data));

	zassert_equal(len, 0, "PID_OBJECT_NAME must not be found in buggy table");
}

ZTEST(oi_pid_name_bug, test_object_name_lookup_succeeds_in_correct_table)
{
	/* Object 0 in the test has the CORRECT PID_OBJECT_NAME entry.
	 * Query must return the name string bytes.
	 */
	uint8_t data[16];
	uint32_t count = 1;

	int len = interface_read_property(0, PID_OBJECT_NAME, 1, &count, data, sizeof(data));

	zassert_true(len > 0, "PID_OBJECT_NAME must be found in correct table (obj 0)");
}
