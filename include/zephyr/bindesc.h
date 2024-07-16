/*
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_BINDESC_H_
#define ZEPHYR_INCLUDE_ZEPHYR_BINDESC_H_

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Corresponds to the definitions in scripts/west_commands/bindesc.py.
 * Do not change without syncing the definitions in both files!
 */
#define BINDESC_MAGIC 0xb9863e5a7ea46046
#define BINDESC_ALIGNMENT 4
#define BINDESC_TYPE_UINT 0x0
#define BINDESC_TYPE_STR 0x1
#define BINDESC_TYPE_BYTES 0x2
#define BINDESC_TYPE_DESCRIPTORS_END 0xf

/**
 * @brief Binary Descriptor Definition
 * @defgroup bindesc_define Bindesc Define
 * @ingroup os_services
 * @{
 */

/*
 * Corresponds to the definitions in scripts/west_commands/bindesc.py.
 * Do not change without syncing the definitions in both files!
 */

/** The app version string such as "1.2.3" */
#define BINDESC_ID_APP_VERSION_STRING 0x800

/** The app version major such as 1 */
#define BINDESC_ID_APP_VERSION_MAJOR 0x801

/** The app version minor such as 2 */
#define BINDESC_ID_APP_VERSION_MINOR 0x802

/** The app version patchlevel such as 3 */
#define BINDESC_ID_APP_VERSION_PATCHLEVEL 0x803

/** The app version number such as 0x10203 */
#define BINDESC_ID_APP_VERSION_NUMBER 0x804

/** The kernel version string such as "3.4.0" */
#define BINDESC_ID_KERNEL_VERSION_STRING 0x900

/** The kernel version major such as 3 */
#define BINDESC_ID_KERNEL_VERSION_MAJOR 0x901

/** The kernel version minor such as 4 */
#define BINDESC_ID_KERNEL_VERSION_MINOR 0x902

/** The kernel version patchlevel such as 0 */
#define BINDESC_ID_KERNEL_VERSION_PATCHLEVEL 0x903

/** The kernel version number such as 0x30400 */
#define BINDESC_ID_KERNEL_VERSION_NUMBER 0x904

/** The year the image was compiled in */
#define BINDESC_ID_BUILD_TIME_YEAR 0xa00

/** The month of the year the image was compiled in */
#define BINDESC_ID_BUILD_TIME_MONTH 0xa01

/** The day of the month the image was compiled in */
#define BINDESC_ID_BUILD_TIME_DAY 0xa02

/** The hour of the day the image was compiled in */
#define BINDESC_ID_BUILD_TIME_HOUR 0xa03

/** The minute the image was compiled in */
#define BINDESC_ID_BUILD_TIME_MINUTE 0xa04

/** The second the image was compiled in */
#define BINDESC_ID_BUILD_TIME_SECOND 0xa05

/** The UNIX time (seconds since midnight of 1970/01/01) the image was compiled in */
#define BINDESC_ID_BUILD_TIME_UNIX 0xa06

/** The date and time of compilation such as "2023/02/05 00:07:04" */
#define BINDESC_ID_BUILD_DATE_TIME_STRING 0xa07

/** The date of compilation such as "2023/02/05" */
#define BINDESC_ID_BUILD_DATE_STRING 0xa08

/** The time of compilation such as "00:07:04" */
#define BINDESC_ID_BUILD_TIME_STRING 0xa09

/** The name of the host that compiled the image */
#define BINDESC_ID_HOST_NAME 0xb00

/** The C compiler name */
#define BINDESC_ID_C_COMPILER_NAME 0xb01

/** The C compiler version */
#define BINDESC_ID_C_COMPILER_VERSION 0xb02

/** The C++ compiler name */
#define BINDESC_ID_CXX_COMPILER_NAME 0xb03

/** The C++ compiler version */
#define BINDESC_ID_CXX_COMPILER_VERSION 0xb04

#define BINDESC_TAG_DESCRIPTORS_END BINDESC_TAG(DESCRIPTORS_END, 0x0fff)

/**
 * @cond INTERNAL_HIDDEN
 */

/*
 * Utility macro to generate a tag from a type and an ID
 *
 * type - Type of the descriptor, UINT, STR or BYTES
 * id - Unique ID for the descriptor, must fit in 12 bits
 */
#define BINDESC_TAG(type, id) ((BINDESC_TYPE_##type & 0xf) << 12 | (id & 0x0fff))

/**
 * @endcond
 */

#if !defined(_LINKER)

#include <zephyr/sys/byteorder.h>

/**
 * @cond INTERNAL_HIDDEN
 */

/*
 * Utility macro to get the name of a bindesc entry
 */
#define BINDESC_NAME(name) bindesc_entry_##name

/* Convenience helper for declaring a binary descriptor entry. */
#define __BINDESC_ENTRY_DEFINE(name)							\
	__aligned(BINDESC_ALIGNMENT) const struct bindesc_entry BINDESC_NAME(name)	\
	__in_section(_bindesc_entry, static, name) __used __noasan

/**
 * @endcond
 */

/**
 * @brief Define a binary descriptor of type string.
 *
 * @details
 * Define a string that is registered in the binary descriptor header.
 * The defined string can be accessed using @ref BINDESC_GET_STR
 *
 * @note The defined string is not static, so its name must not collide with
 * any other symbol in the executable.
 *
 * @param name Name of the descriptor
 * @param id Unique ID of the descriptor
 * @param value A string value for the descriptor
 */
#define BINDESC_STR_DEFINE(name, id, value)	\
	__BINDESC_ENTRY_DEFINE(name) = {	\
		.tag = BINDESC_TAG(STR, id),	\
		.len = (uint16_t)sizeof(value),	\
		.data = value,			\
	}

/**
 * @brief Define a binary descriptor of type uint.
 *
 * @details
 * Define an integer that is registered in the binary descriptor header.
 * The defined integer can be accessed using @ref BINDESC_GET_UINT
 *
 * @note The defined integer is not static, so its name must not collide with
 * any other symbol in the executable.
 *
 * @param name Name of the descriptor
 * @param id Unique ID of the descriptor
 * @param value An integer value for the descriptor
 */
#define BINDESC_UINT_DEFINE(name, id, value)		\
	__BINDESC_ENTRY_DEFINE(name) = {		\
		.tag = BINDESC_TAG(UINT, id),		\
		.len = (uint16_t)sizeof(uint32_t),	\
		.data = sys_uint32_to_array(value),	\
	}

/**
 * @brief Define a binary descriptor of type bytes.
 *
 * @details
 * Define a uint8_t array that is registered in the binary descriptor header.
 * The defined array can be accessed using @ref BINDESC_GET_BYTES.
 * The value should be given as an array literal, wrapped in parentheses, for
 * example:
 *
 *     BINDESC_BYTES_DEFINE(name, id, ({1, 2, 3, 4}));
 *
 * @note The defined array is not static, so its name must not collide with
 * any other symbol in the executable.
 *
 * @param name Name of the descriptor
 * @param id Unique ID of the descriptor
 * @param value A uint8_t array as data for the descriptor
 */
#define BINDESC_BYTES_DEFINE(name, id, value)				\
	__BINDESC_ENTRY_DEFINE(name) = {				\
		.tag = BINDESC_TAG(BYTES, id),				\
		.len = (uint16_t)sizeof((uint8_t [])__DEBRACKET value),	\
		.data = __DEBRACKET value,				\
	}

/**
 * @brief Get the value of a string binary descriptor
 *
 * @details
 * Get the value of a string binary descriptor, previously defined by
 * BINDESC_STR_DEFINE.
 *
 * @param name Name of the descriptor
 */
#define BINDESC_GET_STR(name) BINDESC_NAME(name).data

/**
 * @brief Get the value of a uint binary descriptor
 *
 * @details
 * Get the value of a uint binary descriptor, previously defined by
 * BINDESC_UINT_DEFINE.
 *
 * @param name Name of the descriptor
 */
#define BINDESC_GET_UINT(name) *(uint32_t *)&(BINDESC_NAME(name).data)

/**
 * @brief Get the value of a bytes binary descriptor
 *
 * @details
 * Get the value of a string binary descriptor, previously defined by
 * BINDESC_BYTES_DEFINE. The returned value can be accessed as an array:
 *
 *     for (size_t i = 0; i < BINDESC_GET_SIZE(name); i++)
 *         BINDESC_GET_BYTES(name)[i];
 *
 * @param name Name of the descriptor
 */
#define BINDESC_GET_BYTES(name) BINDESC_NAME(name).data

/**
 * @brief Get the size of a binary descriptor
 *
 * @details
 * Get the size of a binary descriptor. This is particularly useful for
 * bytes binary descriptors where there's no null terminator.
 *
 * @param name Name of the descriptor
 */
#define BINDESC_GET_SIZE(name) BINDESC_NAME(name).len

/*
 * An entry of the binary descriptor header. Each descriptor is
 * described by one of these entries.
 */
struct bindesc_entry {
	/** Tag of the entry */
	uint16_t tag;
	/** Length of the descriptor data */
	uint16_t len;
	/** Value of the entry. This is either an integer or a string */
	uint8_t data[];
} __packed;

/*
 * We're assuming that `struct bindesc_entry` has a specific layout in
 * memory, so it's worth making sure that the layout is really what we
 * think it is. If these assertions fail for your toolchain/platform,
 * please open a bug report.
 */
BUILD_ASSERT(offsetof(struct bindesc_entry, tag) == 0, "Incorrect memory layout");
BUILD_ASSERT(offsetof(struct bindesc_entry, len) == 2, "Incorrect memory layout");
BUILD_ASSERT(offsetof(struct bindesc_entry, data) == 4, "Incorrect memory layout");

#if defined(CONFIG_BINDESC_KERNEL_VERSION_STRING)
extern const struct bindesc_entry BINDESC_NAME(kernel_version_string);
#endif /* defined(CONFIG_BINDESC_KERNEL_VERSION_STRING) */

#if defined(CONFIG_BINDESC_KERNEL_VERSION_MAJOR)
extern const struct bindesc_entry BINDESC_NAME(kernel_version_major);
#endif /* defined(CONFIG_BINDESC_KERNEL_VERSION_MAJOR) */

#if defined(CONFIG_BINDESC_KERNEL_VERSION_MINOR)
extern const struct bindesc_entry BINDESC_NAME(kernel_version_minor);
#endif /* defined(CONFIG_BINDESC_KERNEL_VERSION_MINOR) */

#if defined(CONFIG_BINDESC_KERNEL_VERSION_PATCHLEVEL)
extern const struct bindesc_entry BINDESC_NAME(kernel_version_patchlevel);
#endif /* defined(CONFIG_BINDESC_KERNEL_VERSION_PATCHLEVEL) */

#if defined(CONFIG_BINDESC_KERNEL_VERSION_NUMBER)
extern const struct bindesc_entry BINDESC_NAME(kernel_version_number);
#endif /* defined(CONFIG_BINDESC_KERNEL_VERSION_NUMBER) */

#if defined(CONFIG_BINDESC_APP_VERSION_STRING)
extern const struct bindesc_entry BINDESC_NAME(app_version_string);
#endif /* defined(CONFIG_BINDESC_APP_VERSION_STRING) */

#if defined(CONFIG_BINDESC_APP_VERSION_MAJOR)
extern const struct bindesc_entry BINDESC_NAME(app_version_major);
#endif /* defined(CONFIG_BINDESC_APP_VERSION_MAJOR) */

#if defined(CONFIG_BINDESC_APP_VERSION_MINOR)
extern const struct bindesc_entry BINDESC_NAME(app_version_minor);
#endif /* defined(CONFIG_BINDESC_APP_VERSION_MINOR) */

#if defined(CONFIG_BINDESC_APP_VERSION_PATCHLEVEL)
extern const struct bindesc_entry BINDESC_NAME(app_version_patchlevel);
#endif /* defined(CONFIG_BINDESC_APP_VERSION_PATCHLEVEL) */

#if defined(CONFIG_BINDESC_APP_VERSION_NUMBER)
extern const struct bindesc_entry BINDESC_NAME(app_version_number);
#endif /* defined(CONFIG_BINDESC_APP_VERSION_NUMBER) */

#if defined(CONFIG_BINDESC_BUILD_TIME_YEAR)
extern const struct bindesc_entry BINDESC_NAME(build_time_year);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_YEAR) */

#if defined(CONFIG_BINDESC_BUILD_TIME_MONTH)
extern const struct bindesc_entry BINDESC_NAME(build_time_month);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_MONTH) */

#if defined(CONFIG_BINDESC_BUILD_TIME_DAY)
extern const struct bindesc_entry BINDESC_NAME(build_time_day);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_DAY) */

#if defined(CONFIG_BINDESC_BUILD_TIME_HOUR)
extern const struct bindesc_entry BINDESC_NAME(build_time_hour);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_HOUR) */

#if defined(CONFIG_BINDESC_BUILD_TIME_MINUTE)
extern const struct bindesc_entry BINDESC_NAME(build_time_minute);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_MINUTE) */

#if defined(CONFIG_BINDESC_BUILD_TIME_SECOND)
extern const struct bindesc_entry BINDESC_NAME(build_time_second);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_SECOND) */

#if defined(CONFIG_BINDESC_BUILD_TIME_UNIX)
extern const struct bindesc_entry BINDESC_NAME(build_time_unix);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_UNIX) */

#if defined(CONFIG_BINDESC_BUILD_DATE_TIME_STRING)
extern const struct bindesc_entry BINDESC_NAME(build_date_time_string);
#endif /* defined(CONFIG_BINDESC_BUILD_DATE_TIME_STRING) */

#if defined(CONFIG_BINDESC_BUILD_DATE_STRING)
extern const struct bindesc_entry BINDESC_NAME(build_date_string);
#endif /* defined(CONFIG_BINDESC_BUILD_DATE_STRING) */

#if defined(CONFIG_BINDESC_BUILD_TIME_STRING)
extern const struct bindesc_entry BINDESC_NAME(build_time_string);
#endif /* defined(CONFIG_BINDESC_BUILD_TIME_STRING) */

#if defined(CONFIG_BINDESC_HOST_NAME)
extern const struct bindesc_entry BINDESC_NAME(host_name);
#endif /* defined(CONFIG_BINDESC_HOST_NAME) */

#if defined(CONFIG_BINDESC_C_COMPILER_NAME)
extern const struct bindesc_entry BINDESC_NAME(c_compiler_name);
#endif /* defined(CONFIG_BINDESC_C_COMPILER_NAME) */

#if defined(CONFIG_BINDESC_C_COMPILER_VERSION)
extern const struct bindesc_entry BINDESC_NAME(c_compiler_version);
#endif /* defined(CONFIG_BINDESC_C_COMPILER_VERSION) */

#if defined(CONFIG_BINDESC_CXX_COMPILER_NAME)
extern const struct bindesc_entry BINDESC_NAME(cxx_compiler_name);
#endif /* defined(CONFIG_BINDESC_CXX_COMPILER_NAME) */

#if defined(CONFIG_BINDESC_CXX_COMPILER_VERSION)
extern const struct bindesc_entry BINDESC_NAME(cxx_compiler_version);
#endif /* defined(CONFIG_BINDESC_CXX_COMPILER_VERSION) */

#endif /* !defined(_LINKER) */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_BINDESC_H_ */
