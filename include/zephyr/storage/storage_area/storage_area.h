/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for storage area subsystem
 *
 * The storage area API is a subsystem that creates a unified method to work
 * with flash, eeprom, ram, disks, files, ... for storage. A storage area is
 * an area that has a number of constant sized erase blocks, and has constant
 * write block size. The storage area does not necessarily inherit the
 * limitations of the underlying storage device but rather defines a method of
 * how the underlying storage device will be used (however it does not remove
 * any limitations of the underlying storage device). A storage area can be
 * defined with wrong properties, optionally the definition will be checked
 * by defining the Kconfig option `CONFIG_STORAGE_AREA_VERIFY` that is by
 * default enabled for `DEBUG`builds.
 *
 *
 * There following methods area exposed:
 *   storage_area_read(),	** read chunks **
 *   storage_area_dread(),	** read data direct **
 *   storage_area_prog(),	** program chunks **
 *   storage_area_dprog(),	** program data direct **
 *   storage_area_erase(),	** erase (in erase block addressing) **
 *   storage_area_ioctl()	** used for e.g. getting xip addresses **
 *
 * The subsystem is easy extendable to create custom (virtual) storage areas
 * that consist of e.g. a combination of flash and ram, a encrypted storage
 * area, ...
 *
 * A storage area is defined e.g. for flash:
 * struct storage_area_flash fa =
 *	flash_storage_area(dev, start, xip_address, write_size, erase_size,
 *			   size, properties);
 * struct storage_area *area = &fa.area;
 *
 * For other storage devices (eeprom, ram, disk, ...) similar macros are
 * defines, but they might differ slightly.
 *
 * The write_size, erase_size, ... are declarations of how the storage_area
 * will be used The write_size is limited to a power of 2, erase_size should
 * be a multiple of write_size and size should be a multiple of erase_size.
 * The macro definitions xxxxx_storage_area(...) checks these conditions but
 * always succeeds. Trying to use a badly sized storage area will result in
 * failure of any of the exposed methods.
 *
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_H_
#define ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Storage_area interface
 * @defgroup storage_area_interface Storage_area interface
 * @ingroup storage_apis
 * @{
 */

struct storage_area;

struct storage_area_chunk { /* storage area data chunk */
	void *data;         /* pointer to data */
	size_t len;         /* data length */
};

enum storage_area_properties_mask {
	SA_PROP_READONLY = 0x0001,
	SA_PROP_FOVRWRITE = 0x0002, /* full overwrite (ram, rram, ...) */
	SA_PROP_LOVRWRITE = 0x0004, /* limited overwrite (nor flash) */
	SA_PROP_ZEROERASE = 0x0008, /* erased value is 0x00 */
};

enum storage_area_ioctl_cmd {
	SA_IOCTL_NONE,
	SA_IOCTL_XIPADDRESS, /* retrieve the storage area xip address */
};

/**
 * @brief storage_area API
 *
 * API to access storage area.
 */
struct storage_area_api {
	int (*read)(const struct storage_area *area, size_t start,
		    const struct storage_area_chunk *ch, size_t cnt);
	int (*prog)(const struct storage_area *area, size_t start,
		    const struct storage_area_chunk *ch, size_t cnt);
	int (*erase)(const struct storage_area *area, size_t start, size_t bcnt);
	int (*ioctl)(const struct storage_area *area,
		     enum storage_area_ioctl_cmd cmd, void *data);
};

/**
 * @brief storage_area struct
 */
struct storage_area {
	const struct storage_area_api *api;
	size_t write_size;
	size_t erase_size;
	size_t erase_blocks;
	uint32_t props; /* bitfield of storage area properties */
};

/**
 * @brief storage_area macros
 */
#define STORAGE_AREA_HAS_PROPERTY(area, prop) ((area->props & prop) == prop)
#define STORAGE_AREA_WRITESIZE(area)          area->write_size
#define STORAGE_AREA_ERASESIZE(area)          area->erase_size
#define STORAGE_AREA_SIZE(area)               area->erase_size * area->erase_blocks
#define STORAGE_AREA_FOVRWRITE(area)                                            \
	STORAGE_AREA_HAS_PROPERTY(area, SA_PROP_FOVRWRITE)
#define STORAGE_AREA_LOVRWRITE(area)                                            \
	STORAGE_AREA_HAS_PROPERTY(area, SA_PROP_LOVRWRITE)
#define STORAGE_AREA_ERASEVALUE(area)                                           \
	STORAGE_AREA_HAS_PROPERTY(area, SA_PROP_ZEROERASE) ? 0x00 : 0xff

/**
 * @brief	Read storage chunks.
 *
 * @param area	storage area.
 * @param start	start in storage area (byte).
 * @param ch	chunks for read.
 * @param cnt   chunk element count.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_read(const struct storage_area *area, size_t start,
		      const struct storage_area_chunk *ch, size_t cnt);

/**
 * @brief	Direct read from storage area.
 *
 * @param area	storage area.
 * @param start	start in storage area (byte).
 * @param data	data.
 * @param len   read size.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_dread(const struct storage_area *area, size_t start, void *data,
		       size_t len);

/**
 * @brief	Program storage chunks.
 *
 * @param area	storage area.
 * @param start	start in storage area (byte).
 * @param ch	chunks to program.
 * @param cnt   chunk element count.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_prog(const struct storage_area *area, size_t start,
		      const struct storage_area_chunk *ch, size_t cnt);

/**
 * @brief	Direct program to storage area.
 *
 * @param area	storage area.
 * @param start	start in storage area (byte).
 * @param data	data to program.
 * @param len   program size.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_dprog(const struct storage_area *area, size_t start,
		       const void *data, size_t len);

/**
 * @brief	Erase storage area.
 *
 * @param area  storage area.
 * @param start start block
 * @param bcnt	number of blocks to erase.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_erase(const struct storage_area *area, size_t start,
		       size_t bcnt);

/**
 * @brief	Storage area ioctl.
 *
 * @param area	storage area.
 * @param cmd	ioctl command.
 * @param data	in/out data pointer.
 *
 * @retval	0 on success else negative errno code.
 */
int storage_area_ioctl(const struct storage_area *area,
		       enum storage_area_ioctl_cmd cmd, void *data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_H_ */
