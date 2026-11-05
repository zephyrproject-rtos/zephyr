/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CANopen object dictionary definitions
 */

#ifndef ZEPHYR_INCLUDE_CANBUS_CANOPEN_OD_H_
#define ZEPHYR_INCLUDE_CANBUS_CANOPEN_OD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup canopen
 * @{
 */

/** @brief Object dictionary data types (CiA 301, Table 44). */
enum canopen_od_type {
	/** BOOLEAN */
	CANOPEN_OD_TYPE_BOOLEAN = 0x01,
	/** INTEGER8 */
	CANOPEN_OD_TYPE_INTEGER8 = 0x02,
	/** INTEGER16 */
	CANOPEN_OD_TYPE_INTEGER16 = 0x03,
	/** INTEGER32 */
	CANOPEN_OD_TYPE_INTEGER32 = 0x04,
	/** UNSIGNED8 */
	CANOPEN_OD_TYPE_UNSIGNED8 = 0x05,
	/** UNSIGNED16 */
	CANOPEN_OD_TYPE_UNSIGNED16 = 0x06,
	/** UNSIGNED32 */
	CANOPEN_OD_TYPE_UNSIGNED32 = 0x07,
	/** REAL32 */
	CANOPEN_OD_TYPE_REAL32 = 0x08,
	/** VISIBLE_STRING */
	CANOPEN_OD_TYPE_VISIBLE_STRING = 0x09,
	/** OCTET_STRING */
	CANOPEN_OD_TYPE_OCTET_STRING = 0x0A,
	/** UNICODE_STRING */
	CANOPEN_OD_TYPE_UNICODE_STRING = 0x0B,
	/** TIME_OF_DAY */
	CANOPEN_OD_TYPE_TIME_OF_DAY = 0x0C,
	/** TIME_DIFFERENCE */
	CANOPEN_OD_TYPE_TIME_DIFFERENCE = 0x0D,
	/** DOMAIN, data of variable size handled by @ref canopen_od_domain callbacks */
	CANOPEN_OD_TYPE_DOMAIN = 0x0F,
	/** INTEGER24 */
	CANOPEN_OD_TYPE_INTEGER24 = 0x10,
	/** REAL64 */
	CANOPEN_OD_TYPE_REAL64 = 0x11,
	/** INTEGER40 */
	CANOPEN_OD_TYPE_INTEGER40 = 0x12,
	/** INTEGER48 */
	CANOPEN_OD_TYPE_INTEGER48 = 0x13,
	/** INTEGER56 */
	CANOPEN_OD_TYPE_INTEGER56 = 0x14,
	/** INTEGER64 */
	CANOPEN_OD_TYPE_INTEGER64 = 0x15,
	/** UNSIGNED24 */
	CANOPEN_OD_TYPE_UNSIGNED24 = 0x16,
	/** UNSIGNED40 */
	CANOPEN_OD_TYPE_UNSIGNED40 = 0x18,
	/** UNSIGNED48 */
	CANOPEN_OD_TYPE_UNSIGNED48 = 0x19,
	/** UNSIGNED56 */
	CANOPEN_OD_TYPE_UNSIGNED56 = 0x1A,
	/** UNSIGNED64 */
	CANOPEN_OD_TYPE_UNSIGNED64 = 0x1B,
};

/** @brief Object codes (CiA 301, Table 42). */
enum canopen_od_code {
	/** Single value */
	CANOPEN_OD_CODE_VAR = 0x07,
	/** Values of one type, sub-index 0 holds the number of elements */
	CANOPEN_OD_CODE_ARRAY = 0x08,
	/** Values of mixed types, sub-index 0 holds the highest sub-index */
	CANOPEN_OD_CODE_RECORD = 0x09,
};

/** @brief SDO abort codes (CiA 301, Table 22). */
enum canopen_sdo_abort {
	/** No abort */
	CANOPEN_SDO_ABORT_NONE = 0x00000000,
	/** Toggle bit not alternated */
	CANOPEN_SDO_ABORT_TOGGLE = 0x05030000,
	/** SDO protocol timed out */
	CANOPEN_SDO_ABORT_TIMEOUT = 0x05040000,
	/** Command specifier not valid or unknown */
	CANOPEN_SDO_ABORT_COMMAND = 0x05040001,
	/** Invalid block size, block transfer only */
	CANOPEN_SDO_ABORT_BLOCK_SIZE = 0x05040002,
	/** Invalid sequence number, block transfer only */
	CANOPEN_SDO_ABORT_SEQUENCE = 0x05040003,
	/** CRC error, block transfer only */
	CANOPEN_SDO_ABORT_CRC = 0x05040004,
	/** Out of memory */
	CANOPEN_SDO_ABORT_OUT_OF_MEMORY = 0x05040005,
	/** Unsupported access to an object */
	CANOPEN_SDO_ABORT_UNSUPPORTED_ACCESS = 0x06010000,
	/** Attempt to read a write only object */
	CANOPEN_SDO_ABORT_WRITE_ONLY = 0x06010001,
	/** Attempt to write a read only object */
	CANOPEN_SDO_ABORT_READ_ONLY = 0x06010002,
	/** Object does not exist */
	CANOPEN_SDO_ABORT_NO_OBJECT = 0x06020000,
	/** Object cannot be mapped to the PDO */
	CANOPEN_SDO_ABORT_NOT_MAPPABLE = 0x06040041,
	/** Number and length of the mapped objects exceed the PDO length */
	CANOPEN_SDO_ABORT_MAP_LENGTH = 0x06040042,
	/** General parameter incompatibility */
	CANOPEN_SDO_ABORT_PARAM_INCOMPATIBLE = 0x06040043,
	/** General internal incompatibility */
	CANOPEN_SDO_ABORT_INTERNAL_INCOMPATIBLE = 0x06040047,
	/** Access failed due to a hardware error */
	CANOPEN_SDO_ABORT_HARDWARE = 0x06060000,
	/** Length of service parameter does not match */
	CANOPEN_SDO_ABORT_LENGTH_MISMATCH = 0x06070010,
	/** Length of service parameter too high */
	CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH = 0x06070012,
	/** Length of service parameter too low */
	CANOPEN_SDO_ABORT_LENGTH_TOO_LOW = 0x06070013,
	/** Sub-index does not exist */
	CANOPEN_SDO_ABORT_NO_SUB_INDEX = 0x06090011,
	/** Invalid value for parameter */
	CANOPEN_SDO_ABORT_INVALID_VALUE = 0x06090030,
	/** Value of parameter written too high */
	CANOPEN_SDO_ABORT_VALUE_TOO_HIGH = 0x06090031,
	/** Value of parameter written too low */
	CANOPEN_SDO_ABORT_VALUE_TOO_LOW = 0x06090032,
	/** General error */
	CANOPEN_SDO_ABORT_GENERAL = 0x08000000,
	/** Data cannot be transferred or stored to the application */
	CANOPEN_SDO_ABORT_DATA_TRANSFER = 0x08000020,
	/** Data cannot be transferred because of local control */
	CANOPEN_SDO_ABORT_LOCAL_CONTROL = 0x08000021,
	/** Data cannot be transferred because of the present device state */
	CANOPEN_SDO_ABORT_DEVICE_STATE = 0x08000022,
	/** No data available */
	CANOPEN_SDO_ABORT_NO_DATA = 0x08000024,
};

/** Entry is readable from the network */
#define CANOPEN_OD_ATTR_READ  BIT(0)
/** Entry is writable from the network */
#define CANOPEN_OD_ATTR_WRITE BIT(1)
/** Entry value does not change outside NMT state Initialisation */
#define CANOPEN_OD_ATTR_CONST BIT(2)
/** Entry may be mapped into a TPDO */
#define CANOPEN_OD_ATTR_TPDO  BIT(3)
/** Entry may be mapped into an RPDO */
#define CANOPEN_OD_ATTR_RPDO  BIT(4)
/**
 * Entry is stored with object 1010h, sub-index 3 for entries 6000h to 9FFFh and sub-index 4
 * for entries 2000h to 5FFFh, and restored on NMT reset node
 */
#define CANOPEN_OD_ATTR_PERSIST BIT(5)

/** Read only access */
#define CANOPEN_OD_RO    CANOPEN_OD_ATTR_READ
/** Write only access */
#define CANOPEN_OD_WO    CANOPEN_OD_ATTR_WRITE
/** Read and write access */
#define CANOPEN_OD_RW    (CANOPEN_OD_ATTR_READ | CANOPEN_OD_ATTR_WRITE)
/** Read only access, constant value */
#define CANOPEN_OD_CONST (CANOPEN_OD_ATTR_READ | CANOPEN_OD_ATTR_CONST)

/** @brief Object dictionary sub-entry. */
struct canopen_od_sub {
	/** Value in native representation */
	void *data;
	/** Size of the value in bytes */
	uint16_t size;
	/** Sub-index */
	uint8_t sub;
	/** Data type, see @ref canopen_od_type */
	uint8_t type;
	/** Access attributes, CANOPEN_OD_ATTR_* */
	uint8_t attr;
};

struct canopen_od_entry;

/**
 * @brief Hook called before a value written from the network is stored.
 *
 * @param entry     Object dictionary entry.
 * @param sub       Sub-index.
 * @param value     New value in native representation.
 * @param len       Length of the new value in bytes.
 * @param user_data User data of the entry.
 *
 * @retval CANOPEN_SDO_ABORT_NONE The value is stored.
 * @retval other @ref canopen_sdo_abort code sent to the SDO client, the value is not stored.
 */
typedef uint32_t (*canopen_od_write_hook_t)(const struct canopen_od_entry *entry, uint8_t sub,
					    const void *value, size_t len, void *user_data);

/** @brief Object dictionary entry. */
struct canopen_od_entry {
	/** Sub-entries */
	const struct canopen_od_sub *subs;
	/** Optional write hook */
	canopen_od_write_hook_t write_hook;
	/** User data passed to the write hook */
	void *user_data;
	/** Index */
	uint16_t index;
	/** Object code, see @ref canopen_od_code */
	uint8_t code;
	/** Number of sub-entries, or number of elements for an array */
	uint8_t count;
};

/**
 * @brief Callbacks of a DOMAIN entry.
 *
 * The callbacks are called from the CANopen thread with the node locked.
 */
struct canopen_od_domain {
	/**
	 * @brief Read data for an SDO upload.
	 *
	 * @param entry     Object dictionary entry.
	 * @param offset    Offset of the requested data.
	 * @param buf       Buffer for the data.
	 * A block upload can read the same offset again when the client reports a lost
	 * segment, so the data must not change during a transfer.
	 *
	 * @param len       In: size of @p buf. Out: number of bytes copied, fewer than the size
	 *                  of @p buf only at the end of the data.
	 * @param user_data User data of the entry.
	 *
	 * @retval CANOPEN_SDO_ABORT_NONE Data copied.
	 * @retval other @ref canopen_sdo_abort code sent to the SDO client.
	 */
	uint32_t (*read)(const struct canopen_od_entry *entry, size_t offset, uint8_t *buf,
			 size_t *len, void *user_data);
	/**
	 * @brief Store data of an SDO download.
	 *
	 * An expedited download is one call with @p last set. A segmented download starts with
	 * a call with @p len 0 before any data is sent, so the transfer can be refused in the
	 * initiate response, for example when @p size is too large.
	 *
	 * @param entry     Object dictionary entry.
	 * @param offset    Offset of the data.
	 * @param buf       Data.
	 * @param len       Number of bytes in @p buf, 0 for the first call of a segmented
	 *                  download and possible for the last call.
	 * @param size      Total size indicated by the SDO client, 0 if not indicated.
	 * @param last      True for the last data of the transfer.
	 * @param user_data User data of the entry.
	 *
	 * @retval CANOPEN_SDO_ABORT_NONE Data stored.
	 * @retval other @ref canopen_sdo_abort code sent to the SDO client.
	 */
	uint32_t (*write)(const struct canopen_od_entry *entry, size_t offset, const uint8_t *buf,
			  size_t len, size_t size, bool last, void *user_data);
};

/** @brief Application object dictionary. */
struct canopen_od {
	/** Entries sorted by ascending index */
	const struct canopen_od_entry *entries;
	/** Number of entries */
	size_t count;
};

/**
 * @brief Initializer for an object dictionary sub-entry.
 *
 * @param _sub  Sub-index.
 * @param _type Data type, see @ref canopen_od_type.
 * @param _attr Access attributes.
 * @param _var  Variable holding the value.
 */
#define CANOPEN_OD_SUB(_sub, _type, _attr, _var)                                                   \
	{                                                                                          \
		.data = &(_var),                                                                   \
		.size = sizeof(_var),                                                              \
		.sub = (_sub),                                                                     \
		.type = (_type),                                                                   \
		.attr = (_attr),                                                                   \
	}

/**
 * @brief Initializer for a VAR entry with a write hook.
 *
 * @param _index Index.
 * @param _type  Data type, see @ref canopen_od_type.
 * @param _attr  Access attributes.
 * @param _var   Variable holding the value.
 * @param _hook  Write hook, see @ref canopen_od_write_hook_t.
 * @param _user  User data passed to the write hook.
 */
#define CANOPEN_OD_VAR_HOOK(_index, _type, _attr, _var, _hook, _user)                              \
	{                                                                                          \
		.subs = (const struct canopen_od_sub[]){                                           \
			CANOPEN_OD_SUB(0U, _type, _attr, _var),                                    \
		},                                                                                 \
		.write_hook = (_hook),                                                             \
		.user_data = (_user),                                                              \
		.index = (_index),                                                                 \
		.code = CANOPEN_OD_CODE_VAR,                                                       \
		.count = 1U,                                                                       \
	}

/**
 * @brief Initializer for a VAR entry.
 *
 * @param _index Index.
 * @param _type  Data type, see @ref canopen_od_type.
 * @param _attr  Access attributes.
 * @param _var   Variable holding the value.
 */
#define CANOPEN_OD_VAR(_index, _type, _attr, _var)                                                 \
	CANOPEN_OD_VAR_HOOK(_index, _type, _attr, _var, NULL, NULL)

/**
 * @brief Initializer for a DOMAIN entry.
 *
 * Requires @kconfig{CONFIG_CANOPEN_DOMAIN}. A DOMAIN cannot be mapped into a PDO or stored with
 * 1010h, and is only accessible over SDO.
 *
 * @param _index  Index.
 * @param _attr   Access attributes, @ref CANOPEN_OD_RO, @ref CANOPEN_OD_WO or
 *                @ref CANOPEN_OD_RW.
 * @param _domain Callbacks, a @ref canopen_od_domain.
 * @param _user   User data passed to the callbacks.
 */
#define CANOPEN_OD_DOMAIN(_index, _attr, _domain, _user)                                           \
	{                                                                                          \
		.subs = (const struct canopen_od_sub[]){                                           \
			{                                                                          \
				.data = (void *)&(_domain),                                        \
				.sub = 0U,                                                         \
				.type = CANOPEN_OD_TYPE_DOMAIN,                                    \
				.attr = (_attr),                                                   \
			},                                                                         \
		},                                                                                 \
		.user_data = (_user),                                                              \
		.index = (_index),                                                                 \
		.code = CANOPEN_OD_CODE_VAR,                                                       \
		.count = 1U,                                                                       \
	}

/**
 * @brief Initializer for an ARRAY entry.
 *
 * Element n of @p _arr is sub-index n + 1. Sub-index 0 is read only and returns the number of
 * elements.
 *
 * @param _index Index.
 * @param _type  Data type of the elements, see @ref canopen_od_type.
 * @param _attr  Access attributes of the elements.
 * @param _arr   Array holding the values.
 */
#define CANOPEN_OD_ARRAY(_index, _type, _attr, _arr)                                               \
	{                                                                                          \
		.subs = (const struct canopen_od_sub[]){                                           \
			{                                                                          \
				.data = (_arr),                                                    \
				.size = sizeof((_arr)[0]),                                         \
				.sub = 1U,                                                         \
				.type = (_type),                                                   \
				.attr = (_attr),                                                   \
			},                                                                         \
		},                                                                                 \
		.index = (_index),                                                                 \
		.code = CANOPEN_OD_CODE_ARRAY,                                                     \
		.count = ARRAY_SIZE(_arr),                                                         \
	}

/**
 * @brief Initializer for a RECORD entry.
 *
 * The sub-entries are given as @ref CANOPEN_OD_SUB initializers in ascending sub-index order.
 * If no sub-entry for sub-index 0 is given, sub-index 0 is read only and returns the highest
 * sub-index.
 *
 * @param _index Index.
 * @param ...    Sub-entry initializers.
 */
#define CANOPEN_OD_RECORD(_index, ...)                                                             \
	{                                                                                          \
		.subs = (const struct canopen_od_sub[]){__VA_ARGS__},                              \
		.index = (_index),                                                                 \
		.code = CANOPEN_OD_CODE_RECORD,                                                    \
		.count = sizeof((const struct canopen_od_sub[]){__VA_ARGS__}) /                    \
			 sizeof(struct canopen_od_sub),                                            \
	}

/**
 * @brief Statically define an application object dictionary.
 *
 * @param _name Name of the @ref canopen_od.
 * @param ...   Entry initializers in ascending index order.
 */
#define CANOPEN_OD_DEFINE(_name, ...)                                                              \
	static const struct canopen_od_entry _CONCAT(_name, _entries)[] = {__VA_ARGS__};           \
	static const struct canopen_od _name = {                                                   \
		.entries = _CONCAT(_name, _entries),                                               \
		.count = ARRAY_SIZE(_CONCAT(_name, _entries)),                                     \
	}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CANBUS_CANOPEN_OD_H_ */
