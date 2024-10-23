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
/* sizeof ignores the data as it's a flexible array */
#define BINDESC_ENTRY_HEADER_SIZE (sizeof(struct bindesc_entry))

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

/** The app git reference such as "v3.3.0-18-g2c85d9224fca" */
#define BINDESC_ID_APP_BUILD_VERSION 0x805

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

/** The kernel git reference such as "v3.3.0-18-g2c85d9224fca" */
#define BINDESC_ID_KERNEL_BUILD_VERSION 0x905

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
 * @brief Utility macro to get the type of a bindesc tag
 *
 * @param tag Tag to get the type of
 */
#define BINDESC_GET_TAG_TYPE(tag) ((tag >> 12) & 0xf)

/**
 * @endcond
 */

#if !defined(_LINKER) || defined(__DOXYGEN__)

#include <zephyr/sys/byteorder.h>
#include <zephyr/device.h>

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
#define BINDESC_STR_DEFINE(name, id, value)							\
	__BINDESC_ENTRY_DEFINE(name) = {							\
		.tag = BINDESC_TAG(STR, id),							\
		.len = (uint16_t)sizeof(value),							\
		.data = value,									\
	};											\
	BUILD_ASSERT(sizeof(value) <= CONFIG_BINDESC_DEFINE_MAX_DATA_SIZE,			\
		     "Bindesc " STRINGIFY(name) " exceeded maximum size, consider reducing the"	\
		     " size or changing CONFIG_BINDESC_DEFINE_MAX_DATA_SIZE. ")

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
#define BINDESC_BYTES_DEFINE(name, id, value)							\
	__BINDESC_ENTRY_DEFINE(name) = {							\
		.tag = BINDESC_TAG(BYTES, id),							\
		.len = (uint16_t)sizeof((uint8_t [])__DEBRACKET value),				\
		.data = __DEBRACKET value,							\
	};											\
	BUILD_ASSERT(sizeof((uint8_t [])__DEBRACKET value) <=					\
		     CONFIG_BINDESC_DEFINE_MAX_DATA_SIZE,					\
		     "Bindesc " STRINGIFY(name) " exceeded maximum size, consider reducing the"	\
		     " size or changing CONFIG_BINDESC_DEFINE_MAX_DATA_SIZE. ")

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

/**
 * @}
 */

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

struct bindesc_handle {
	const uint8_t *address;
	enum {
		BINDESC_HANDLE_TYPE_RAM,
		BINDESC_HANDLE_TYPE_MEMORY_MAPPED_FLASH,
		BINDESC_HANDLE_TYPE_FLASH,
	} type;
	size_t size_limit;
#if IS_ENABLED(CONFIG_BINDESC_READ_FLASH)
	const struct device *flash_device;
	uint8_t buffer[sizeof(struct bindesc_entry) +
			CONFIG_BINDESC_READ_FLASH_MAX_DATA_SIZE] __aligned(BINDESC_ALIGNMENT);
#endif /* IS_ENABLED(CONFIG_BINDESC_READ_FLASH) */
};

/**
 * @brief Reading Binary Descriptors of other images.
 * @defgroup bindesc_read Bindesc Read
 * @ingroup os_services
 * @{
 */

/**
 * @brief Callback type to be called on descriptors found during a walk
 *
 * @param entry Current descriptor
 * @param user_data The user_data given to @ref bindesc_foreach
 *
 * @return Any non zero value will halt the walk
 */
typedef int (*bindesc_callback_t)(const struct bindesc_entry *entry, void *user_data);

/**
 * @brief Open an image's binary descriptors for reading, from a memory mapped flash
 *
 * @details
 * Initializes a bindesc handle for subsequent calls to bindesc API.
 * Memory mapped flash is any flash that can be directly accessed by the CPU,
 * without needing to use the flash API for copying the data to RAM.
 *
 * @param handle Bindesc handle to be given to subsequent calls
 * @param offset The offset from the beginning of the flash that the bindesc magic can be found at
 *
 * @retval 0 On success
 * @retval -ENOENT If no bindesc magic was found at the given offset
 */
int bindesc_open_memory_mapped_flash(struct bindesc_handle *handle, size_t offset);

/**
 * @brief Open an image's binary descriptors for reading, from RAM
 *
 * @details
 * Initializes a bindesc handle for subsequent calls to bindesc API.
 * It's assumed that the whole bindesc context was copied to RAM prior to calling
 * this function, either by the user or by a bootloader.
 *
 * @note The given address must be aligned to BINDESC_ALIGNMENT
 *
 * @param handle Bindesc handle to be given to subsequent calls
 * @param address The address that the bindesc magic can be found at
 * @param max_size Maximum size of the given buffer
 *
 * @retval 0 On success
 * @retval -ENOENT If no bindesc magic was found at the given address
 * @retval -EINVAL If the given address is not aligned
 */
int bindesc_open_ram(struct bindesc_handle *handle, const uint8_t *address, size_t max_size);

/**
 * @brief Open an image's binary descriptors for reading, from flash
 *
 * @details
 * Initializes a bindesc handle for subsequent calls to bindesc API.
 * As opposed to reading bindesc from RAM or memory mapped flash, this
 * backend requires reading the data from flash to an internal buffer
 * using the flash API
 *
 * @param handle Bindesc handle to be given to subsequent calls
 * @param offset The offset from the beginning of the flash that the bindesc magic can be found at
 * @param flash_device Flash device to read descriptors from
 *
 * @retval 0 On success
 * @retval -ENOENT If no bindesc magic was found at the given offset
 */
int bindesc_open_flash(struct bindesc_handle *handle, size_t offset,
		       const struct device *flash_device);

/**
 * @brief Walk the binary descriptors and run a user defined callback on each of them
 *
 * @note
 * If the callback returns a non zero value, the walk stops.
 *
 * @param handle An initialized bindesc handle
 * @param callback A user defined callback to be called on each descriptor
 * @param user_data User defined data to be given to the callback
 *
 * @return If the walk was finished prematurely by the callback,
 *         return the callback's retval, zero otherwise
 */
int bindesc_foreach(struct bindesc_handle *handle, bindesc_callback_t callback, void *user_data);

/**
 * @brief Find a specific descriptor of type string
 *
 * @warning
 * When using the flash backend, result will be invalidated by the next call to any bindesc API.
 * Use the value immediately or copy it elsewhere.
 *
 * @param handle An initialized bindesc handle
 * @param id ID to search for
 * @param result Pointer to the found string
 *
 * @retval 0 If the descriptor was found
 * @retval -ENOENT If the descriptor was not found
 */
int bindesc_find_str(struct bindesc_handle *handle, uint16_t id, const char **result);

/**
 * @brief Find a specific descriptor of type uint
 *
 * @warning
 * When using the flash backend, result will be invalidated by the next call to any bindesc API.
 * Use the value immediately or copy it elsewhere.
 *
 * @param handle An initialized bindesc handle
 * @param id ID to search for
 * @param result Pointer to the found uint
 *
 * @retval 0 If the descriptor was found
 * @retval -ENOENT If the descriptor was not found
 */
int bindesc_find_uint(struct bindesc_handle *handle, uint16_t id, const uint32_t **result);

/**
 * @brief Find a specific descriptor of type bytes
 *
 * @warning
 * When using the flash backend, result will be invalidated by the next call to any bindesc API.
 * Use the value immediately or copy it elsewhere.
 *
 * @param handle An initialized bindesc handle
 * @param id ID to search for
 * @param result Pointer to the found bytes
 * @param result_size Size of the found bytes
 *
 * @retval 0 If the descriptor was found
 * @retval -ENOENT If the descriptor was not found
 */
int bindesc_find_bytes(struct bindesc_handle *handle, uint16_t id, const uint8_t **result,
		       size_t *result_size);

/**
 * @brief Get the size of an image's binary descriptors
 *
 * @details
 * Walks the binary descriptor structure to caluculate the total size of the structure
 * in bytes. This is useful, for instance, if the whole structure is to be copied to RAM.
 *
 * @param handle An initialized bindesc handle
 * @param result Pointer to write result to
 *
 * @return 0 On success, negative errno otherwise
 */
int bindesc_get_size(struct bindesc_handle *handle, size_t *result);

/**
 * @}
 */

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

#if defined(CONFIG_BINDESC_KERNEL_BUILD_VERSION)
extern const struct bindesc_entry BINDESC_NAME(kernel_build_version);
#endif /* defined(CONFIG_BINDESC_KERNEL_BUILD_VERSION) */

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

#if defined(CONFIG_BINDESC_APP_BUILD_VERSION)
extern const struct bindesc_entry BINDESC_NAME(app_build_version);
#endif /* defined(CONFIG_BINDESC_APP_BUILD_VERSION) */

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_BINDESC_H_ */
