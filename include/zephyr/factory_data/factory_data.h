/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FACTORY_DATA_FACTORY_DATA_H_
#define ZEPHYR_INCLUDE_FACTORY_DATA_FACTORY_DATA_H_

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FACTORY_DATA_TOTAL_LEN_MAX                                                                 \
	(CONFIG_FACTORY_DATA_NAME_LEN_MAX + 1 /* '\0' */ + CONFIG_FACTORY_DATA_VALUE_LEN_MAX)

/* Relevant only when backed by flash */
#define FACTORY_DATA_FLASH_PARTITION FLASH_AREA_ID(factory)

/**
 * @brief Public API of the factory data module.
 *
 * Accessing factory data.
 *
 * @defgroup factory_data Factory data
 * @{
 */

/**
 * Initialization of factory data subsystem
 *
 * Can be called at application startup.
 *
 * @return 0 on success, non-zero on failure.
 */
int factory_data_init(void);

#if CONFIG_FACTORY_DATA_WRITE
/**
 * Write a single serialized value to factory data storage.
 *
 * @note Overwriting an already existing value (same name) is not possible.
 *
 * @param name Name/key of the factory data item.
 * @param value Pointer to the value of the factory data item.
 * @param val_len Length of the value.
 *
 * @return 0 on success, non-zero on failure.
 */
int factory_data_save_one(const char *name, const void *value, size_t val_len);

/**
 * Clear all factory data
 *
 * @return 0 on success; non-zero on failure
 */
int factory_data_erase(void);
#endif /* CONFIG_FACTORY_DATA_WRITE */

/**
 * Callback function used for loading (all) factory data.
 *
 * Used by @ref factory_data_load function.
 *
 * @param[in]     name    Name of the factory data element
 * @param[in]     value   Factory data read from flash
 * @param[in]     len     Size of the factory data value in bytes
 * @param[in,out] param   parameter given to the @ref factory_data_load function.
 *
 * @return 0 continue loading, non-zero stop loading data.
 */
typedef int (*factory_data_load_direct_cb)(const char *name, const uint8_t *value, size_t len,
					   const void *param);

/**
 * Load factory data using given callback.
 *
 * @param[in]     cb      pointer to the callback function.
 * @param[in,out] param   parameter to be passed when callback
 *                        function is called.
 * @retval 0 on success, all elements walked
 * @retval negative on failure; entries up to the faulty one walked
 * @retval positive when aborted by callback; entries up to the aborting one walked
 */
int factory_data_load(factory_data_load_direct_cb cb, const void *param);

/**
 * Load single factory data value.
 *
 * @note Calling this function causes (worst case) all existing elements to be walked and read out.
 *       Whenever possible, use @ref factory_data_load instead.
 *
 * @param name Name/key of the factory data item.
 * @param value Pointer to the value of the factory data item.
 * @param val_len Length of the value.
 *
 * @return Number of read bytes, negative on failure.
 */
ssize_t factory_data_load_one(const char *name, uint8_t *value, size_t val_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FACTORY_DATA_FACTORY_DATA_H_ */
