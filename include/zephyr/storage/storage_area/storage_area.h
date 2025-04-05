/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief A storage area is a region on flash, eeprom, disk, ram, ... with
 *        fixed write-size and fixed erase-size.
 *
 * The storage area API is a subsystem that creates a unified method to work
 * with flash, eeprom, ram, disks, files, ... for storage.
 *
 * A storage area is an area that has a number of constant sized erase blocks,
 * and has constant write block size. The storage area does not necessarily
 * inherit the limitations of the underlying storage device but rather defines
 * a method of how the underlying storage device will be used (however it does
 * not remove any limitations of the underlying storage device).
 *
 * A storage area can be defined with wrong properties (e.g. an erase-size
 * smaller than the erase-block-size of the underlying storage device).
 * Optionally the definition is checked by defining the Kconfig option
 * `CONFIG_STORAGE_AREA_VERIFY` (default enabled for `DEBUG` builds).
 *
 * The subsystem is easy extendable to create custom (virtual) storage areas
 * that consist of e.g. a combination of flash and ram, an encrypted storage
 * area, ...
 *
 * There following methods are exposed:
 * - storage_area_read(): read data,
 * - storage_area_readv(): read data vector,
 * - storage_area_write(): write data,
 * - storage_area_writev(): write data vector,
 * - storage_area_erase(): erase (in erase block addressing),
 * - storage_area_ioctl(): used for e.g. getting xip addresses,
 *
 * A storage area is defined e.g. for a read-write area on flash:
 * @code{.c}
 * STORAGE_AREA_FLASH_RW_DEFINE(name, ...);
 * struct storage_area *area = GET_STORAGE_AREA(name);
 * @endcode
 *
 * For other storage devices (eeprom, ram, disk, ...) similar macros are
 * defines, but they might differ slightly.
 *
 * The write_size, erase_size, ... are declarations of how the storage_area
 * will be used The write_size is limited to a power of 2, erase_size should
 * be a multiple of write_size and size should be a multiple of erase_size.
 * The macro definitions STORAGE_AREA_XXX_DEFINE(...) checks these conditions.
 *
 * @defgroup storage_area Storage area
 * @ingroup storage_apis
 * @{
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_H_
#define ZEPHYR_INCLUDE_STORAGE_STORAGE_AREA_H_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allow the sa_off_t type to be redefined if needed */
#ifdef CONFIG_STORAGE_AREA_OFFSET_64BIT
typedef uint64_t sa_off_t;
#else
typedef uint32_t sa_off_t;
#endif

struct storage_area;

/** storage area input-output vector */
struct storage_area_iovec {
	void *data; /**< pointer to data */
	size_t len; /**< data length */
};

/** storage area properties masks */
enum storage_area_properties_mask {
	/** full overwrite (e.g. ram, rram, ...) */
	STORAGE_AREA_PROP_FOVRWRITE = BIT(0),
	/** limited overwrite (e.g. nor flash without crc */
	STORAGE_AREA_PROP_LOVRWRITE = BIT(1),
	/** erase value is 0x00 */
	STORAGE_AREA_PROP_ZEROERASE = BIT(2),
	/** erase while writing on flash devices */
	STORAGE_AREA_PROP_AUTOERASE = BIT(3),
};

/** storage area ioctl commands */
enum storage_area_ioctl_cmd {
	STORAGE_AREA_IOCTL_NONE,
	/** retrieve the storage area xip address */
	STORAGE_AREA_IOCTL_XIPADDRESS,
};

/** storage area api */
struct storage_area_api {
	int (*readv)(const struct storage_area *area, sa_off_t offset,
		     const struct storage_area_iovec *iovec, size_t iovcnt);
	int (*writev)(const struct storage_area *area, sa_off_t offset,
		      const struct storage_area_iovec *iovec, size_t iovcnt);
	int (*erase)(const struct storage_area *area, size_t sblk, size_t bcnt);
	int (*ioctl)(const struct storage_area *area,
		     enum storage_area_ioctl_cmd cmd, void *data);
};

/** storage area */
struct storage_area {
	const struct storage_area_api *api;
	size_t write_size;
	size_t erase_size;
	size_t erase_blocks;
	/** bitfield of properties */
	uint32_t props;
};

#define STORAGE_AREA_HAS_PROPERTY(area, prop) ((area->props & prop) == prop)
#define STORAGE_AREA_WRITESIZE(area)          area->write_size
#define STORAGE_AREA_ERASESIZE(area)          area->erase_size
#define STORAGE_AREA_SIZE(area)                                                 \
	(area->erase_size * area->erase_blocks)
#define STORAGE_AREA_FOVRWRITE(area)                                            \
	STORAGE_AREA_HAS_PROPERTY(area, STORAGE_AREA_PROP_FOVRWRITE)
#define STORAGE_AREA_LOVRWRITE(area)                                            \
	STORAGE_AREA_HAS_PROPERTY(area, STORAGE_AREA_PROP_LOVRWRITE)
#define STORAGE_AREA_ERASEVALUE(area)                                           \
	STORAGE_AREA_HAS_PROPERTY(area, STORAGE_AREA_PROP_ZEROERASE) ? 0x00     \
								     : 0xff
#define STORAGE_AREA_AUTOERASE(area)                                            \
	STORAGE_AREA_HAS_PROPERTY(area, STORAGE_AREA_PROP_AUTOERASE)

/**
 * @brief retrieve a pointer to a storage area
 *
 * @param _name the name of the storage area
 */
#define GET_STORAGE_AREA(_name)                                                 \
	(struct storage_area *)&_storage_area_##_name.area

/**
 * @brief	 Read iovec from storage area.
 *
 * @param area   storage area.
 * @param offset offset in storage area (byte).
 * @param iovec  io vector for read.
 * @param iovcnt iovec element count.
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_readv(const struct storage_area *area, sa_off_t offset,
		       const struct storage_area_iovec *iovec, size_t iovcnt);

/**
 * @brief	 Read from storage area.
 *
 * @param area	 storage area.
 * @param offset offset in storage area (byte).
 * @param data	 data.
 * @param len    read size.
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_read(const struct storage_area *area, sa_off_t offset,
		      void *data, size_t len);

/**
 * @brief	 Write iovec to storage area.
 *
 * @param area	 storage area.
 * @param offset offset in storage area (byte).
 * @param iovec	 io vector to write.
 * @param iovcnt iovec element count.
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_writev(const struct storage_area *area, sa_off_t offset,
			const struct storage_area_iovec *iovec, size_t iovcnt);

/**
 * @brief	 Write data to storage area.
 *
 * @param area	 storage area.
 * @param offset offset in storage area (byte).
 * @param data	 data to program.
 * @param len    program size.
 *
 * @retval	 0 on success else negative errno code.
 */
int storage_area_write(const struct storage_area *area, sa_off_t offset,
		       const void *data, size_t len);

/**
 * @brief      Erase storage area.
 *
 * @param area storage area.
 * @param sblk start block
 * @param bcnt number of blocks to erase.
 *
 * @retval     0 on success else negative errno code.
 */
int storage_area_erase(const struct storage_area *area, size_t sblk,
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
