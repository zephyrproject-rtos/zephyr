/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief A storage area on ram
 * @defgroup storage_area_ram Storage area on ram
 * @ingroup storage_area
 * @{
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_RAM_H_
#define ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_RAM_H_

#include <zephyr/kernel.h>
#include <zephyr/storage/storage_area/storage_area.h>

#ifdef __cplusplus
extern "C" {
#endif

struct storage_area_ram {
	const struct storage_area area;
	const uintptr_t start;
};

extern const struct storage_area_api storage_area_ram_rw_api;
extern const struct storage_area_api storage_area_ram_ro_api;

/**
 * @brief Helper macro to create a storage area on top of a memory region
 */
#define STORAGE_AREA_RAM(_start, _ws, _es, _size, _props, _api)                 \
	{                                                                       \
		.area =                                                         \
			{                                                       \
				.api = ((_ws == 0) ||                           \
					((_ws & (_ws - 1)) != 0) ||             \
					((_es % _ws) != 0) ||                   \
					((_size % _es) != 0)) ? NULL : _api,    \
				.write_size = _ws,                              \
				.erase_size = _es,                              \
				.erase_blocks = _size / _es,                    \
				.props = _props | STORAGE_AREA_PROP_FOVRWRITE |	\
					 STORAGE_AREA_PROP_ZEROERASE,           \
			},                                                      \
		.start = _start,                                                \
	}

/**
 * @brief Define a read-write storage area on top of a memory region
 *
 * @param _name	   storage area name: used by GET_STORAGE_AREA(_name)
 * @param _start   absolute address of the memory region
 * @param _ws      write-size
 * @param _es	   erase-size
 * @param _size	   storage area size
 * @param _props   storage area properties (see storage_area.h)
 */
#define STORAGE_AREA_RAM_RW_DEFINE(_name, _start, _ws, _es, _size, _props)	\
	BUILD_ASSERT(_ws != 0, "Invalid write size");				\
	BUILD_ASSERT((_ws & (_ws - 1)) == 0, "Invalid write size");		\
	BUILD_ASSERT((_es % _ws) == 0, "Invalid erase size");			\
	BUILD_ASSERT((_size % _ws) == 0, "Invalid size");			\
	const struct storage_area_ram _storage_area_##_name =			\
		STORAGE_AREA_RAM(_start, _ws, _es, _size, _props,		\
				 &storage_area_ram_rw_api)

/**
 * @brief Define a read-only storage area on top of a memory region
 *
 * see @ref STORAGE_AREA_RAM_RW_DEFINE for parameters
 *
 * remark: the write-size and erase-size are not used in read-only, however
 *         to have a correct interpretation of the data that is used in
 *         read-only mode it is advised to keep them equal to the sizes that
 *         were used when creating the area.
 */
#define STORAGE_AREA_RAM_RO_DEFINE(_name, _start, _ws, _es, _size, _props)	\
	BUILD_ASSERT(_ws != 0, "Invalid write size");				\
	BUILD_ASSERT((_ws & (_ws - 1)) == 0, "Invalid write size");		\
	BUILD_ASSERT((_es % _ws) == 0, "Invalid erase size");			\
	BUILD_ASSERT((_size % _ws) == 0, "Invalid size");			\
	const struct storage_area_ram _storage_area_##_name =			\
		STORAGE_AREA_RAM(_start, _ws, _es, _size, _props,		\
				 &storage_area_ram_ro_api)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_RAM_H_ */
