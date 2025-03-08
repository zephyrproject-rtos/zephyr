/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief A storage area on disk
 * @defgroup storage_area_disk Storage area on disk
 * @ingroup storage_area
 * @{
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_DISK_H_
#define ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_DISK_H_

#include <zephyr/storage/disk_access.h>
#include <zephyr/storage/storage_area/storage_area.h>

#ifdef __cplusplus
extern "C" {
#endif

struct storage_area_disk {
	const struct storage_area area;
	const uint32_t start;
	const size_t ssize;
	const char *name;
};

extern const struct storage_area_api storage_area_disk_rw_api;
extern const struct storage_area_api storage_area_disk_ro_api;

/**
 * @brief Helper macro to create a storage area on top of a disk
 */
#define STORAGE_AREA_DISK(_dname, _start, _ssize, _ws, _es, _size, _props,      \
			  _api)                                                 \
	{                                                                       \
		.area =                                                         \
			{                                                       \
				.api = ((_ws == 0) ||                           \
					((_ws & (_ws - 1)) != 0) ||             \
					((_es % _ws) != 0) ||                   \
					((_size % _es) != 0))                   \
					       ? NULL                           \
					       : _api,                          \
				.write_size = _ws,                              \
				.erase_size = _es,                              \
				.erase_blocks = _size / _es,                    \
				.props = _props | STORAGE_AREA_PROP_FOVRWRITE,  \
			},                                                      \
		.name = _dname, .start = _start, .ssize = _ssize,               \
	}

/**
 * @brief Define a read-write storage area on top of a disk
 *
 * @param _name	 storage area name: used by GET_STORAGE_AREA(_name)
 * @param _dname disk name
 * @param _start start sector on disk
 * @param _ws    write-size (equal or multiple of disk sector size)
 * @param _es	 erase-size (equal or multiple of disk sector size)
 * @param _size	 storage area size
 * @param _props storage area properties (see storage_area.h)
 */
#define STORAGE_AREA_DISK_RW_DEFINE(_name, _dname, _start, _ssize, _ws, _es,    \
				    _size, _props)                              \
	BUILD_ASSERT(_ws != 0, "Invalid write size");                           \
	BUILD_ASSERT((_ws & (_ws - 1)) == 0, "Invalid write size");             \
	BUILD_ASSERT((_es % _ws) == 0, "Invalid erase size");                   \
	BUILD_ASSERT((_size % _ws) == 0, "Invalid size");                       \
	BUILD_ASSERT((_ssize % _ws) == 0, "Invalid sector size");               \
	const struct storage_area_disk _storage_area_##_name =                  \
		STORAGE_AREA_DISK(_dname, _start, _ssize, _ws, _es, _size,      \
				  _props, &storage_area_disk_rw_api)

/**
 * @brief Define a read-only storage area on top of a disk
 *
 * see @ref STORAGE_AREA_DISK_RW_DEFINE for parameters
 *
 * remark: the write-size and erase-size are not used in read-only, however
 *         to have a correct interpretation of the data that is used in
 *         read-only mode it is advised to keep them equal to the sizes that
 *         were used when creating the area.
 */
#define STORAGE_AREA_DISK_RO_DEFINE(_name, _dname, _start, _ssize, _ws, _es,    \
				    _size, _props)                              \
	BUILD_ASSERT(_ws != 0, "Invalid write size");                           \
	BUILD_ASSERT((_ws & (_ws - 1)) == 0, "Invalid write size");             \
	BUILD_ASSERT((_es % _ws) == 0, "Invalid erase size");                   \
	BUILD_ASSERT((_size % _ws) == 0, "Invalid size");                       \
	BUILD_ASSERT((_ssize % _ws) == 0, "Invalid sector size");               \
	const struct storage_area_disk _storage_area_##_name =                  \
		STORAGE_AREA_DISK(_dname, _start, _ssize, _ws, _es, _size,      \
				  _props, &storage_area_disk_ro_api)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_DISK_H_ */
