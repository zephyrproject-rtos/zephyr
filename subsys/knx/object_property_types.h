/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OBJECT_PROPERTY_TYPES__
#define __OBJECT_PROPERTY_TYPES__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

/** The data type of a property. */
enum PropertyDataType_ {
	PDT_CONTROL = 0x00,            /**< length: 1 read, 10 write */
	PDT_CHAR = 0x01,               /**< length: 1 */
	PDT_UNSIGNED_CHAR = 0x02,      /**< length: 1 */
	PDT_INT = 0x03,                /**< length: 2 */
	PDT_UNSIGNED_INT = 0x04,       /**< length: 2 */
	PDT_KNX_FLOAT = 0x05,          /**< length: 2 */
	PDT_DATE = 0x06,               /**< length: 3 */
	PDT_TIME = 0x07,               /**< length: 3 */
	PDT_LONG = 0x08,               /**< length: 4 */
	PDT_UNSIGNED_LONG = 0x09,      /**< length: 4 */
	PDT_FLOAT = 0x0a,              /**< length: 4 */
	PDT_DOUBLE = 0x0b,             /**< length: 8 */
	PDT_CHAR_BLOCK = 0x0c,         /**< length: 10 */
	PDT_POLL_GROUP_SETTING = 0x0d, /**< length: 3 */
	PDT_SHORT_CHAR_BLOCK = 0x0e,   /**< length: 5 */
	PDT_DATE_TIME = 0x0f,          /**< length: 8 */
	PDT_VARIABLE_LENGTH = 0x10,
	PDT_GENERIC_01 = 0x11, /**< length: 1 */
	PDT_GENERIC_02 = 0x12, /**< length: 2 */
	PDT_GENERIC_03 = 0x13, /**< length: 3 */
	PDT_GENERIC_04 = 0x14, /**< length: 4 */
	PDT_GENERIC_05 = 0x15, /**< length: 5 */
	PDT_GENERIC_06 = 0x16, /**< length: 6 */
	PDT_GENERIC_07 = 0x17, /**< length: 7 */
	PDT_GENERIC_08 = 0x18, /**< length: 8 */
	PDT_GENERIC_09 = 0x19, /**< length: 9 */
	PDT_GENERIC_10 = 0x1a, /**< length: 10 */
	PDT_GENERIC_11 = 0x1b, /**< length: 11 */
	PDT_GENERIC_12 = 0x1c, /**< length: 12 */
	PDT_GENERIC_13 = 0x1d, /**< length: 13 */
	PDT_GENERIC_14 = 0x1e, /**< length: 14 */
	PDT_GENERIC_15 = 0x1f, /**< length: 15 */
	PDT_GENERIC_16 = 0x20, /**< length: 16 */
	PDT_GENERIC_17 = 0x21, /**< length: 17 */
	PDT_GENERIC_18 = 0x22, /**< length: 18 */
	PDT_GENERIC_19 = 0x23, /**< length: 19 */
	PDT_GENERIC_20 = 0x24, /**< length: 20 */
	/* Lengths below corrected against KNX spec 3/5/1 Resources; the previous
	 * "length: 3" on every entry was copy-paste and disagreed with the
	 * property_data_size() table that actually drives serialisation.
	 */
	PDT_UTF8 = 0x2f,               /**< length: variable */
	PDT_VERSION = 0x30,            /**< length: 2 */
	PDT_ALARM_INFO = 0x31,         /**< length: 6 */
	PDT_BINARY_INFORMATION = 0x32, /**< length: 1 */
	PDT_BITSET8 = 0x33,            /**< length: 1 */
	PDT_BITSET16 = 0x34,           /**< length: 2 */
	PDT_ENUM8 = 0x35,              /**< length: 1 */
	PDT_SCALING = 0x36,            /**< length: 1 */
	PDT_NE_VL = 0x3c,              /**< length: variable */
	PDT_NE_FL = 0x3d,              /**< length: variable */
	PDT_FUNCTION = 0x3e,           /**< length: variable (Function Property) */
	PDT_ESCAPE = 0x3f,             /**< length: n/a (escape code) */
};
typedef uint16_t PropertyDataType;

enum _PropertyID {
	/** Interface Object Type independent Properties */
	PID_OBJECT_TYPE = 1,
	PID_OBJECT_NAME = 2,
	PID_LOAD_STATE_CONTROL = 5,
	PID_RUN_STATE_CONTROL = 6,
	PID_TABLE_REFERENCE = 7,
	PID_SERVICE_CONTROL = 8,
	PID_FIRMWARE_REVISION = 9,
	PID_SERVICES_SUPPORTED = 10,
	PID_SERIAL_NUMBER = 11,
	PID_MANUFACTURER_ID = 12,
	PID_PROGRAM_VERSION = 13,
	PID_DEVICE_CONTROL = 14,
	PID_ORDER_INFO = 15,
	PID_PEI_TYPE = 16,
	PID_PORT_CONFIGURATION = 17,
	PID_POLL_GROUP_SETTINGS = 18,
	PID_MANUFACTURER_DATA = 19,
	PID_DESCRIPTION = 21,
	PID_TABLE = 23,
	PID_VERSION = 25,
	PID_MCB_TABLE = 27,
	PID_ERROR_CODE = 28,
	PID_OBJECT_INDEX = 29,
	PID_DOWNLOAD_COUNTER = 30,

	/** Properties in the Device Object */
	PID_ROUTING_COUNT = 51,
	PID_MAX_RETRY_COUNT = 52,
	PID_ERROR_FLAGS = 53,
	PID_PROGMODE = 54,
	PID_PRODUCT_ID = 55,
	PID_MAX_APDU_LENGTH = 56,
	PID_SUBNET_ADDR = 57,
	PID_DEVICE_ADDR = 58,
	PID_PB_CONFIG = 59,
	PID_IO_LIST = 71,
	PID_HARDWARE_TYPE = 78,
	PID_DEVICE_DESCRIPTOR = 83,

	PID_LAST = 0xFFFF,
};
typedef uint16_t PropertyID;

/* Standard Identifier Table 2.2 */
enum _ObjectType {
	/** Device object. */
	OT_DEVICE = 0,

	/** Address table object. */
	OT_ADDR_TABLE = 1,

	/** 4.17.5 Group Object Association Table */
	OT_ASSOC_TABLE = 2,

	/** Application program object. */
	OT_APPLICATION_PROG = 3,

	/** Interface program object. */
	OT_INTERFACE_PROG = 4,

	/** KNX - Object Associationtable. */
	OT_OJB_ASSOC_TABLE = 5,

	/** Router Object */
	OT_ROUTER = 6,

	/** LTE Address Routing Table Object */
	OT_LTE_ADDR_ROUTING_TABLE = 7,

	/** cEMI Server Object */
	OT_CEMI_SERVER = 8,

	/** Group Object Table Object */
	OT_GRP_OBJ_TABLE = 9,

	/** Polling Master */
	OT_POLLING_MASTER = 10,

	/** KNXnet/IP Parameter Object */
	OT_IP_PARAMETER = 11,

	/** Reserved. Shall not be used. */
	OT_RESERVED = 12,

	/** File Server Object */
	OT_FILE_SERVER = 13,

	/** RF Medium Object */
	OT_RF_MEDIUM = 19
};
typedef uint8_t ObjectType;

enum LoadState_ {
	LS_UNLOADED = 0,
	LS_LOADED = 1,
	LS_LOADING = 2,
	LS_ERROR = 3,
	LS_UNLOADING = 4,
	LS_LOADCOMPLETING = 5
};
typedef uint8_t LoadState;

enum LoadEvents {
	LE_NOOP = 0,
	LE_START_LOADING = 1,
	LE_LOAD_COMPLETED = 2,
	LE_ADDITIONAL_LOAD_CONTROLS = 3,
	LE_UNLOAD = 4
};

/* 20.011 DPT_ErrorClass_System */
enum ErrorCode_ {
	E_NO_FAULT = 0,
	E_GENERAL_DEVICE_FAULT = 1,
	E_COMMUNICATION_FAULT = 2,
	E_CONFIGURATION_FAULT = 3,
	E_HARDWARE_FAULT = 4,
	E_SOFTWARE_FAULT = 5,
	E_INSUFFICIENT_NON_VOLATILE_MEMORY = 6,
	E_INSUFFICIENT_VOLATILE_MEMORY = 7,
	E_GOT_MEM_ALLOC_ZERO = 8,
	E_CRC_ERROR = 9,
	E_WATCHDOG_RESET = 10,
	E_INVALID_OPCODE = 11,
	E_GENERAL_PROTECTION_FAULT = 12,
	E_MAX_TABLE_LENGTH_EXEEDED = 13,
	E_GOT_UNDEF_LOAD_CMD = 14,
	E_GAT_NOT_SORTED = 15,
	E_INVALID_CONNECTION_NUMBER = 16,
	E_INVALID_GO_NUMBER = 17,
	E_GO_TYPE_TOO_BIG = 18
};
typedef uint8_t ErrorCode;

/** The access level necessary to read a property of an interface object. */
enum AccessLevel {
	ReadLv0 = 0x00,
	ReadLv1 = 0x10,
	ReadLv2 = 0x20,
	ReadLv3 = 0x30,
	WriteLv0 = 0x00,
	WriteLv1 = 0x01,
	WriteLv2 = 0x02,
	WriteLv3 = 0x03,
};

/*
 * Property values wider than one octet MUST be stored in KNX wire order
 * (big-endian): interface_read_property() serialises `value.data` with a plain
 * byte copy, so a native uint16_t leaks the host's byte order onto the bus.
 * That is how PID_DEVICE_DESCRIPTOR used to answer 0xB007 instead of 0x07B0 and
 * the Address Table's PID_OBJECT_TYPE answered 256 instead of 1 — only
 * OT_DEVICE == 0 survived, by being palindromic.
 *
 * These types make the storage order explicit and un-assignable from an
 * integer, so the mistake cannot be repeated silently:
 *
 *     static knx_u16_be_t _object_type = KNX_U16_BE(OT_ADDR_TABLE);
 *
 * PID_MANUFACTURER_ID, PID_MAX_APDU_LENGTH and PID_VERSION were already
 * uint8_t[2] and therefore already correct; they are the pattern generalised
 * here.
 */
typedef uint8_t knx_u16_be_t[2];
typedef uint8_t knx_u32_be_t[4];

#define KNX_U16_BE(v) {(uint8_t)(((v) >> 8) & 0xFFu), (uint8_t)((v) & 0xFFu)}
#define KNX_U32_BE(v)                                                                              \
	{(uint8_t)(((v) >> 24) & 0xFFu), (uint8_t)(((v) >> 16) & 0xFFu),                           \
	 (uint8_t)(((v) >> 8) & 0xFFu), (uint8_t)((v) & 0xFFu)}

/** Read back a knx_u16_be_t as a host-order value. */
static inline uint16_t knx_u16_be_get(const knx_u16_be_t v)
{
	return (uint16_t)(((uint16_t)v[0] << 8) | v[1]);
}

struct property_value {
	uint16_t *nr_of_elem;
	void *data;
};

struct property_description {
	PropertyID property_id;
	bool write_enable;
	PropertyDataType property_datatype;
	uint16_t max_nr_of_elem;
	uint8_t access;
	/*
	 * True when *value.nr_of_elem is stored in KNX wire (big-endian) byte
	 * order — the GrAT/GrOAT/GrOT PID_TABLE current_length fields, which ETS
	 * writes directly into __memory alongside the entries. False for a
	 * property whose count is an ordinary host-order counter (e.g.
	 * PID_IO_LIST's _nb_interface).
	 *
	 * Replaces an earlier implicit rule that keyed this off
	 * property_datatype == PDT_GENERIC_04 — which broke the moment a
	 * PDT_GENERIC_04-declared property needed a host-order count, or a
	 * wire-order count needed a PDT other than PDT_GENERIC_04 (exactly the
	 * GrAT/GrOT case: their real per-entry PDT is
	 * PDT_UNSIGNED_INT/PDT_GENERIC_02, not PDT_GENERIC_04, but their count is
	 * still wire-order).
	 */
	bool count_is_be;
};

struct property {
	struct property_description description;
	struct property_value value;
};

/*
 * Declarative property-table macros.
 *
 * struct property_value.data is `void *`, so any storage pointer converts to
 * it silently — nothing ties the declared property_datatype to the C type
 * behind .data. A plain uint16_t where a knx_u16_be_t was required has
 * happened this way before, caught only by bench-testing against ETS.
 *
 * KNX_PDT_SIZE() is the one canonical "PDT -> storage octets" table, usable
 * both as an ordinary runtime expression (object_interface.c's
 * property_data_size()) and inside BUILD_ASSERT, because it is a plain
 * ternary chain of `==` comparisons on compile-time constants. It reports
 * the STORAGE size only: PDT_CONTROL's write-PDU length (10 octets, KNX spec
 * 3/5/1 §4.23.2) is a protocol detail of the write path, not a storage
 * width, and stays local to property_data_size().
 *
 * A "0 means variable-length/unsupported" entry mirrors
 * property_data_size()'s existing convention for those PDTs.
 */
#define KNX_PDT_SIZE(pdt)                                                                          \
	((pdt) == PDT_CHAR                 ? 1                                                     \
	 : (pdt) == PDT_UNSIGNED_CHAR      ? 1                                                     \
	 : (pdt) == PDT_GENERIC_01         ? 1                                                     \
	 : (pdt) == PDT_BITSET8            ? 1                                                     \
	 : (pdt) == PDT_ENUM8              ? 1                                                     \
	 : (pdt) == PDT_BINARY_INFORMATION ? 1                                                     \
	 : (pdt) == PDT_SCALING            ? 1                                                     \
	 : (pdt) == PDT_CONTROL            ? 1                                                     \
	 : (pdt) == PDT_INT                ? 2                                                     \
	 : (pdt) == PDT_UNSIGNED_INT       ? 2                                                     \
	 : (pdt) == PDT_KNX_FLOAT          ? 2                                                     \
	 : (pdt) == PDT_GENERIC_02         ? 2                                                     \
	 : (pdt) == PDT_BITSET16           ? 2                                                     \
	 : (pdt) == PDT_VERSION            ? 2                                                     \
	 : (pdt) == PDT_DATE               ? 3                                                     \
	 : (pdt) == PDT_TIME               ? 3                                                     \
	 : (pdt) == PDT_POLL_GROUP_SETTING ? 3                                                     \
	 : (pdt) == PDT_GENERIC_03         ? 3                                                     \
	 : (pdt) == PDT_LONG               ? 4                                                     \
	 : (pdt) == PDT_UNSIGNED_LONG      ? 4                                                     \
	 : (pdt) == PDT_FLOAT              ? 4                                                     \
	 : (pdt) == PDT_GENERIC_04         ? 4                                                     \
	 : (pdt) == PDT_SHORT_CHAR_BLOCK   ? 5                                                     \
	 : (pdt) == PDT_GENERIC_05         ? 5                                                     \
	 : (pdt) == PDT_ALARM_INFO         ? 6                                                     \
	 : (pdt) == PDT_GENERIC_06         ? 6                                                     \
	 : (pdt) == PDT_GENERIC_07         ? 7                                                     \
	 : (pdt) == PDT_DOUBLE             ? 8                                                     \
	 : (pdt) == PDT_DATE_TIME          ? 8                                                     \
	 : (pdt) == PDT_GENERIC_08         ? 8                                                     \
	 : (pdt) == PDT_GENERIC_09         ? 9                                                     \
	 : (pdt) == PDT_CHAR_BLOCK         ? 10                                                    \
	 : (pdt) == PDT_GENERIC_10         ? 10                                                    \
	 : (pdt) == PDT_GENERIC_11         ? 11                                                    \
	 : (pdt) == PDT_GENERIC_12         ? 12                                                    \
	 : (pdt) == PDT_GENERIC_13         ? 13                                                    \
	 : (pdt) == PDT_GENERIC_14         ? 14                                                    \
	 : (pdt) == PDT_GENERIC_15         ? 15                                                    \
	 : (pdt) == PDT_GENERIC_16         ? 16                                                    \
	 : (pdt) == PDT_GENERIC_17         ? 17                                                    \
	 : (pdt) == PDT_GENERIC_18         ? 18                                                    \
	 : (pdt) == PDT_GENERIC_19         ? 19                                                    \
	 : (pdt) == PDT_GENERIC_20         ? 20                                                    \
					   : 0)

/*
 * KNX_PROP_ASSERT_SIZE(pdt, storage) — fail the build if the storage backing
 * a scalar property no longer matches the octet width its declared PDT will
 * be serialised as. Place it once, at file scope, immediately under the
 * storage variable's own declaration — see KNX_PROP_SCALAR below for why it
 * cannot live inside the struct property[] initializer itself.
 */
#define KNX_PROP_ASSERT_SIZE(pdt, storage)                                                         \
	BUILD_ASSERT(sizeof(storage) == KNX_PDT_SIZE(pdt),                                         \
		     "sizeof(" #storage ") does not match the storage width "                      \
		     "KNX_PDT_SIZE(" #pdt ") declares - fix the storage type "                     \
		     "or the property_datatype, do not just widen this assert")

/*
 * KNX_PROP_SCALAR(pid, pdt, write_enable, access, ptr) — one property table
 * entry for the common case: a single element, no separate count
 * (max_nr_of_elem = 1, nr_of_elem = NULL). Covers plain scalars
 * (PID_OBJECT_TYPE, PID_DEVICE_CONTROL, PID_LOAD_STATE_CONTROL, ...) and
 * opaque multi-byte blocks read as one element (PID_SERIAL_NUMBER /
 * PDT_GENERIC_06, PID_PROGRAM_VERSION / PDT_GENERIC_05, ...).
 *
 * This does NOT check that `ptr`'s storage matches `pdt` — C has no way to
 * embed a _Static_assert inside a file-scope initializer list
 * (_Static_assert is a declaration, not an expression, and a GNU statement
 * expression is not a constant expression, so neither can appear in a
 * static array initializer). Pair every KNX_PROP_SCALAR with a
 * KNX_PROP_ASSERT_SIZE next to the storage declaration instead - that is
 * where the real check lives.
 */
#define KNX_PROP_SCALAR(pid_, pdt_, we_, acc_, ptr_)                                               \
	{                                                                                          \
		.description =                                                                     \
			{                                                                          \
				.property_id = (pid_),                                             \
				.write_enable = (we_),                                             \
				.property_datatype = (pdt_),                                       \
				.max_nr_of_elem = 1,                                               \
				.access = (acc_),                                                  \
			},                                                                         \
		.value = {                                                                         \
			.nr_of_elem = NULL,                                                        \
			.data = (ptr_)                                                             \
		}                                                                                  \
	}

/*
 * KNX_PROP_CHARS(pid, write_enable, access, array) — a fixed-size character
 * block property (PID_OBJECT_NAME and friends): property_datatype is always
 * PDT_UNSIGNED_CHAR, nr_of_elem = NULL, and max_nr_of_elem comes from
 * ARRAY_SIZE(array) rather than a hand-kept literal that can silently go
 * stale if the array is ever resized (object_device.c used to hardcode `13`
 * next to `char _name[13]`).
 */
#define KNX_PROP_CHARS(pid_, we_, acc_, array_)                                                    \
	{                                                                                          \
		.description =                                                                     \
			{                                                                          \
				.property_id = (pid_),                                             \
				.write_enable = (we_),                                             \
				.property_datatype = PDT_UNSIGNED_CHAR,                            \
				.max_nr_of_elem = ARRAY_SIZE(array_),                              \
				.access = (acc_),                                                  \
			},                                                                         \
		.value = {                                                                         \
			.nr_of_elem = NULL,                                                        \
			.data = (array_)                                                           \
		}                                                                                  \
	}

/*
 * KNX_PROP_ARRAY(pid, pdt, write_enable, access, max_elem, count_ptr,
 * data_ptr, count_be) — a count-bearing table property (PID_TABLE,
 * PID_IO_LIST). `max_elem` stays an explicit argument: it is not
 * ARRAY_SIZE(data_ptr) in general (the Association Table's PID_TABLE packs
 * 2 uint16 slots per logical entry, for instance). `count_be` sets
 * .description.count_is_be — true when *count_ptr is itself stored in KNX
 * wire order (the GrAT/GrOAT/GrOT table lengths ETS writes directly), false
 * for an ordinary host-order counter (PID_IO_LIST's _nb_interface).
 *
 * Deliberately no KNX_PROP_ASSERT_SIZE-style check here: `pdt` need not
 * match data_ptr's element size for a multi-slot-per-entry table like the
 * Association Table's — a per-element size assert would be
 * wrong for that case, not just unproven.
 */
#define KNX_PROP_ARRAY(pid_, pdt_, we_, acc_, max_elem_, count_ptr_, data_ptr_, count_be_)         \
	{                                                                                          \
		.description =                                                                     \
			{                                                                          \
				.property_id = (pid_),                                             \
				.write_enable = (we_),                                             \
				.property_datatype = (pdt_),                                       \
				.max_nr_of_elem = (max_elem_),                                     \
				.access = (acc_),                                                  \
				.count_is_be = (count_be_),                                        \
			},                                                                         \
		.value = {                                                                         \
			.nr_of_elem = (count_ptr_),                                                \
			.data = (data_ptr_)                                                        \
		}                                                                                  \
	}

struct data_block {
	void *data;
	size_t size;
};

struct object_memory {
	struct data_block parameters;
	struct data_block object_group;
	struct data_block address_table;
	struct data_block association_table;
};

struct association {
	uint16_t asap;
	uint16_t tsap;
};

#endif
