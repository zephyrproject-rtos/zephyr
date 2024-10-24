/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for storage area eeprom subsystem
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_EEPROM_H_
#define ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_EEPROM_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/storage_area/storage_area.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Storage_area_eeprom interface
 * @defgroup storage_area_eeprom_interface Storage_area_eeprom interface
 * @ingroup storage_apis
 * @{
 */

struct storage_area_eeprom {
	const struct storage_area area;
	const struct device *dev;
	const size_t start;
};

extern const struct storage_area_api storage_area_eeprom_api;

#define eeprom_storage_area(_dev, _start, _ws, _es, _size, _props)              \
	{                                                                       \
		.area =                                                         \
			{                                                       \
				.api = ((_ws == 0) ||                           \
					((_ws & (_ws - 1)) != 0) ||             \
					((_es % _ws) != 0) ||                   \
					((_size % _es) != 0))                   \
					       ? NULL                           \
					       : &storage_area_eeprom_api,      \
				.write_size = _ws,                              \
				.erase_size = _es,                              \
				.erase_blocks = _size / _es,                    \
				.props = _props | SA_PROP_FOVRWRITE,            \
			},                                                      \
		.dev = _dev, .start = _start,                                   \
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_EEPROM_H_ */
