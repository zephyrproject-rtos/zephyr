/*
 * Copyright (c) 2026 Fabien Proriol
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief KNX subsystem core definitions and logging helpers.
 *
 * This file contains KNX type definitions and formatting macros for
 * logging KNX addresses and telegrams.
 */

#ifndef ZEPHYR_INCLUDE_KNX_CORE_H_
#define ZEPHYR_INCLUDE_KNX_CORE_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/**
 * @defgroup knx_core KNX Core Types and Formatting
 * @ingroup connectivity
 * @{
 */

/**
 * @brief KNX address type enumeration
 *
 * Defines whether an address is an individual (physical) address
 * or a group address. This value is used in the address type field
 * of KNX telegrams.
 */
typedef enum knx_addr_type {
	/** Individual address (physical address) */
	KNX_ADDR_TYPE_INDIVIDUAL = 0,

	/** Group address */
	KNX_ADDR_TYPE_GROUP = 1,
} knx_addr_type_t;

/**
 * @brief KNX frame format enumeration
 *
 * Defines the frame format used in KNX telegrams.
 * The frame format determines the structure and length of the telegram.
 */
typedef enum knx_frame_format {
	/** Extended frame format (up to 254 bytes of data) */
	KNX_FRAME_FORMAT_EXTENDED = 0x0,

	/** Standard frame format (up to 15 bytes of data) */
	KNX_FRAME_FORMAT_STANDARD = 0x1,
} knx_frame_format_t;

/**
 * @brief KNX priority enumeration
 *
 * Defines the priority level of KNX telegrams.
 * Priority is encoded in bits 3-2 of the control field.
 */
typedef enum knx_priority {
	/** System priority - mainly used by ETS for device programming */
	KNX_PRIORITY_SYSTEM = 0x0,

	/** Normal priority - more important telegrams like central functions */
	KNX_PRIORITY_NORMAL = 0x1,

	/** Urgent priority - used for alarms */
	KNX_PRIORITY_URGENT = 0x2,

	/** Low priority - normal priority of group communication */
	KNX_PRIORITY_LOW = 0x3,

	/** Invalid priority - used for transmission error indication */
	KNX_PRIORITY_INVALID = 0x4,

	/** Repeated telegram - used for transmission state indication */
	KNX_PRIORITY_REPEATED = 0x5,
} knx_priority_t;

/**
 * @brief KNX address type (16-bit)
 *
 * Can represent both physical addresses and group addresses.
 * Use @ref knx_addr_type to distinguish between the two types.
 */
typedef uint16_t knx_addr_t;

/**
 * @name KNX Physical Address Formatting
 * @{
 */

/**
 * @brief Format string for KNX physical address (area.line.device)
 *
 * Physical address format:
 * - Bits 15-12: Area (0-15)
 * - Bits 11-8:  Line (0-15)
 * - Bits 7-0:   Device (0-255)
 *
 * Example: 0x1234 displays as "1.2.52"
 *
 * Usage:
 * @code
 * knx_addr_t addr = 0x1234;
 * LOG_INF("Physical address: " KNX_ADDR_FMT, KNX_ADDR_VAL(addr));
 * @endcode
 */
#define KNX_ADDR_FMT "%u.%u.%u"

/**
 * @brief Extract components of KNX physical address for formatting
 *
 * @param addr KNX physical address (knx_addr_t)
 * @return Three comma-separated values: area, line, device
 */
#define KNX_ADDR_VAL(addr)                                                                         \
	(unsigned int)(((addr) >> 12) & 0xF), (unsigned int)(((addr) >> 8) & 0xF),                 \
		(unsigned int)((addr) & 0xFF)

/**
 * @}
 */

/**
 * @name KNX Address Type Formatting
 * @{
 */

/**
 * @brief Format string for KNX address type
 *
 * Displays the address type as a human-readable string.
 *
 * Usage:
 * @code
 * knx_addr_type_t type = KNX_ADDR_TYPE_GROUP;
 * LOG_INF("Address type: " KNX_ADDR_TYPE_FMT, KNX_ADDR_TYPE_VAL(type));
 * @endcode
 */
#define KNX_ADDR_TYPE_FMT "%s"

/**
 * @brief Convert KNX address type to string
 *
 * @param type Address type (knx_addr_type_t)
 * @return String representation ("Individual" or "Group")
 */
#define KNX_ADDR_TYPE_VAL(type) ((type) == KNX_ADDR_TYPE_INDIVIDUAL ? "Individual" : "Group")

/**
 * @}
 */

/**
 * @name KNX Frame Format Formatting
 * @{
 */

/**
 * @brief Format string for KNX frame format
 *
 * Displays the frame format as a human-readable string.
 *
 * Usage:
 * @code
 * knx_frame_format_t format = KNX_FRAME_FORMAT_STANDARD;
 * LOG_INF("Frame format: " KNX_FRAME_FMT, KNX_FRAME_VAL(format));
 * @endcode
 */
#define KNX_FRAME_FMT "%s"

/**
 * @brief Convert KNX frame format to string
 *
 * @param format Frame format (knx_frame_format_t)
 * @return String representation ("Extended" or "Standard")
 */
#define KNX_FRAME_VAL(format) ((format) == KNX_FRAME_FORMAT_EXTENDED ? "Extended" : "Standard")

/**
 * @}
 */

/**
 * @name KNX Priority Formatting
 * @{
 */

/**
 * @brief Format string for KNX priority
 *
 * Displays the priority as a human-readable string.
 *
 * Usage:
 * @code
 * knx_priority_t priority = KNX_PRIORITY_LOW;
 * LOG_INF("Priority: " KNX_PRIORITY_FMT, KNX_PRIORITY_VAL(priority));
 * @endcode
 */
#define KNX_PRIORITY_FMT "%s"

/**
 * @brief Convert KNX priority to string
 *
 * @param prio Priority (knx_priority_t)
 * @return String representation ("System", "Normal", "Urgent", "Low", "Invalid", or "Repeated")
 */
#define KNX_PRIORITY_VAL(prio)                                                                     \
	((prio) == KNX_PRIORITY_SYSTEM     ? "System"                                              \
	 : (prio) == KNX_PRIORITY_NORMAL   ? "Normal"                                              \
	 : (prio) == KNX_PRIORITY_URGENT   ? "Urgent"                                              \
	 : (prio) == KNX_PRIORITY_LOW      ? "Low"                                                 \
	 : (prio) == KNX_PRIORITY_INVALID  ? "Invalid"                                             \
	 : (prio) == KNX_PRIORITY_REPEATED ? "Repeated"                                            \
					   : "Unknown")

/**
 * @}
 */

/**
 * @name KNX Group Address Formatting (3-level)
 * @{
 */

/**
 * @brief Format string for KNX group address 3-level (main/middle/sub)
 *
 * Group address 3-level format:
 * - Bits 15-11: Main group (0-31)
 * - Bits 10-8:  Middle group (0-7)
 * - Bits 7-0:   Sub group (0-255)
 *
 * Example: 0x0801 displays as "1/0/1"
 *
 * Usage:
 * @code
 * knx_addr_t group = 0x0801;
 * LOG_INF("Group address: " KNX_GROUP_FMT, KNX_GROUP_VAL(group));
 * @endcode
 */
#define KNX_GROUP_FMT "%u/%u/%u"

/**
 * @brief Extract components of KNX group address (3-level) for formatting
 *
 * @param addr KNX group address (knx_addr_t)
 * @return Three comma-separated values: main, middle, sub
 */
#define KNX_GROUP_VAL(addr)                                                                        \
	(unsigned int)(((addr) >> 11) & 0x1F), (unsigned int)(((addr) >> 8) & 0x7),                \
		(unsigned int)((addr) & 0xFF)

/**
 * @}
 */

/**
 * @name KNX Group Address Formatting (2-level)
 * @{
 */

/**
 * @brief Format string for KNX group address 2-level (main/sub)
 *
 * Group address 2-level format:
 * - Bits 15-11: Main group (0-31)
 * - Bits 10-0:  Sub group (0-2047)
 *
 * Example: 0x0801 displays as "1/1"
 *
 * Usage:
 * @code
 * knx_addr_t group = 0x0801;
 * LOG_INF("Group address: " KNX_GROUP_2LEVEL_FMT, KNX_GROUP_2LEVEL_VAL(group));
 * @endcode
 */
#define KNX_GROUP_2LEVEL_FMT "%u/%u"

/**
 * @brief Extract components of KNX group address (2-level) for formatting
 *
 * @param addr KNX group address (knx_addr_t)
 * @return Two comma-separated values: main, sub
 */
#define KNX_GROUP_2LEVEL_VAL(addr)                                                                 \
	(unsigned int)(((addr) >> 11) & 0x1F), (unsigned int)((addr) & 0x7FF)

/**
 * @}
 */

/**
 * @name KNX Dynamic Address Formatting
 * @{
 */

/**
 * @brief Get format string based on address type
 *
 * Returns the appropriate format string for either individual (physical)
 * or group addresses.
 *
 * @param type Address type (knx_addr_type_t)
 * @return KNX_ADDR_FMT for individual, KNX_GROUP_FMT for group
 *
 * Usage:
 * @code
 * knx_addr_type_t type = KNX_ADDR_TYPE_GROUP;
 * knx_addr_t addr = 0x0801;
 * LOG_INF("Address: " KNX_ADDR_AUTO_FMT(type), KNX_ADDR_AUTO_VAL(type, addr));
 * @endcode
 */
#define KNX_ADDR_AUTO_FMT(type) ((type) == KNX_ADDR_TYPE_INDIVIDUAL ? KNX_ADDR_FMT : KNX_GROUP_FMT)

/**
 * @brief Extract address components based on address type
 *
 * Returns the appropriate value extraction for either individual (physical)
 * or group addresses. Must be used with @ref KNX_ADDR_AUTO_FMT.
 *
 * @param type Address type (knx_addr_type_t)
 * @param addr KNX address (knx_addr_t)
 * @return Extracted components matching the format from KNX_ADDR_AUTO_FMT
 *
 * Usage:
 * @code
 * knx_addr_type_t type = KNX_ADDR_TYPE_GROUP;
 * knx_addr_t addr = 0x0801;
 * LOG_INF("Address: " KNX_ADDR_AUTO_FMT(type), KNX_ADDR_AUTO_VAL(type, addr));
 * // Output: "Address: 1/0/1"
 * @endcode
 */
#define KNX_ADDR_AUTO_VAL(type, addr)                                                              \
	((type) == KNX_ADDR_TYPE_INDIVIDUAL ? KNX_ADDR_VAL(addr) : KNX_GROUP_VAL(addr))

/**
 * @}
 */

/**
 * @name KNX Address Formatters
 * @{
 */

/**
 * @brief Number of rotating buffers for thread-local formatting
 */
#define KNX_FORMAT_BUFFERS 4

/**
 * @brief Size of each formatting buffer
 */
#define KNX_FORMAT_BUF_SIZE 32

/**
 * @}
 */

/**
 * @}
 */

#endif
