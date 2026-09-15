/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief virtual FAT12/FAT16 filesystem API.
 * @ingroup virtual_fat_interface
 */

#ifndef LIBMCU_COMPONENTS_VIRTUAL_FAT_INCLUDE_VIRTUAL_FAT_H
#define LIBMCU_COMPONENTS_VIRTUAL_FAT_INCLUDE_VIRTUAL_FAT_H

#include <zephyr/sys/byteorder.h>

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup virtual_fat_interface Virtual FAT filesystem
 * @since 4.5
 * @version 0.1.0
 * @ingroup file_system_api
 * @{
 */

/** FAT file attribute: read-only. */
#define FAT_ATTR_READ_ONLY 0x01
/** FAT file attribute: hidden. */
#define FAT_ATTR_HIDDEN    0x02
/** FAT file attribute: system. */
#define FAT_ATTR_SYSTEM    0x04
/** FAT directory attribute: volume label. */
#define FAT_ATTR_VOLUME_ID 0x08
/** FAT file attribute: directory. */
#define FAT_ATTR_DIRECTORY 0x10
/** FAT file attribute: archive. */
#define FAT_ATTR_ARCHIVE   0x20
/** FAT long name attribute combination. */
#define FAT_ATTR_LONG_NAME 0x0f

/** FAT12/FAT16 Boot Sector structure.
 *
 *  All fields in this structure are internal and should not be accessed by user code.
 */
struct fat1x_BS {
	/** @cond INTERNAL_HIDDEN */
	uint8_t BS_jmpBoot[3];
	char BS_OEMName[8];
	uint16_t BPB_BytsPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;
	/* Extended BPB structure for FAT12 and FAT16 volumes */
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	char BS_VolLab[11];
	char BS_FilSysType[8];
	/** @endcond */
} __packed;

/** Number of reserved sectors before the first FAT. */
#define FAT_BS_RSVD_SECTORS 1

/** Number of FAT copies in the volume. Two is the default. */
#define FAT_BS_NUM_FATS 2

/**
 * @brief Define the max number of entries (files or folders) in the root dir.
 *
 * BPB_RootEntCnt * 32 % BPB_BytsPerSec has to be 0.
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 */
#define FAT_BS_ROOT_ENT_CNT(sector_size) ((sector_size) / 32)

/**
 * @brief Determine the number of data clusters.
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 * @param data_partition_size  data partition size in bytes
 */
#define FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size)                                      \
	(DIV_ROUND_UP(DIV_ROUND_UP((data_partition_size), (sector_size)),                          \
		      CONFIG_FAT_SECTORS_PER_CLUSTER))
/**
 * @brief Determine the amount of sectors for one FAT in a FAT12 partition
 *
 * Fat 12 uses 12 bits for each entry. This is 3/2 bytes
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 * @param data_partition_size  data partition size in bytes
 */
#define FAT_SIZE_FAT12(sector_size, data_partition_size)                                           \
	(DIV_ROUND_UP(DIV_ROUND_UP(FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size) * 3, 2),  \
		      (sector_size)))

/**
 * @brief Determine the amount of sectors for one FAT in a FAT16 partition
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 * @param data_partition_size  data partition size in bytes
 */
#define FAT_SIZE_FAT16(sector_size, data_partition_size)                                           \
	(DIV_ROUND_UP((FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size) * 2), (sector_size)))

/**
 * @brief Calculate the number of sectors occupied by the root directory.
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 */
#define ROOT_DIR_SECTORS(sector_size)                                                              \
	DIV_ROUND_UP(FAT_BS_ROOT_ENT_CNT(sector_size) * 32, (sector_size))

/**
 * @brief Calculate the number of bytes in one cluster.
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 */
#define FAT_BYTES_PER_CLUSTER(sector_size) ((sector_size) * CONFIG_FAT_SECTORS_PER_CLUSTER)

/** FAT type used by the filesystem. */
enum fat_type {
	/** FAT12 filesystem (12-bit FAT entries). */
	FAT12,
	/** FAT16 filesystem (16-bit FAT entries). */
	FAT16
};

/**
 * @brief Determine the FAT type based on the number of data clusters.
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 * @param data_partition_size  data partition size in bytes
 */
#define FAT_TYPE(sector_size, data_partition_size)                                                 \
	(FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size) < 4085 ? FAT12 : FAT16)

/**
 * @brief Calculate the number of sectors occupied by one FAT.
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 * @param data_partition_size  data partition size in bytes
 */
#define FAT_NUM_SECTORS_PER_FAT(sector_size, data_partition_size)                                  \
	(FAT_TYPE(sector_size, data_partition_size) == FAT12                                       \
		 ? FAT_SIZE_FAT12(sector_size, data_partition_size)                                \
		 : FAT_SIZE_FAT16(sector_size, data_partition_size))

/**
 * @brief Calculate the total number of sectors in a virtual FAT volume.
 *
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 * @param data_partition_size  data partition size in bytes
 */
#define FAT_BS_TOT_SEC(sector_size, data_partition_size)                                           \
	(FAT_BS_RSVD_SECTORS +                                                                     \
	 FAT_BS_NUM_FATS * FAT_NUM_SECTORS_PER_FAT(sector_size, data_partition_size) +             \
	 ROOT_DIR_SECTORS(sector_size) + data_partition_size / sector_size)

/**
 * @brief Define a FAT12/FAT16 boot sector.
 *
 * The generated object is intended to be used by a
 * @ref virtual_fat_ctx.
 *
 * @param name                 name of the generated boot-sector object
 * @param volume_label         11-character, space-padded volume label
 * @param sector_size          sector size in bytes. Must be 512, 1024, 2048, or 4096.
 * @param data_partition_size  data partition size in bytes
 */
#define VIRTUAL_FAT_DEFINE_BOOT_SECTOR(name, volume_label, sector_size, data_partition_size)       \
	BUILD_ASSERT((sector_size) == 512 || (sector_size) == 1024 || (sector_size) == 2048 ||     \
			     (sector_size) == 4096,                                                \
		     "Sector size must be one of 512, 1024, 2048, or 4096 bytes");                 \
	BUILD_ASSERT((CONFIG_FAT_SECTORS_PER_CLUSTER & (CONFIG_FAT_SECTORS_PER_CLUSTER - 1)) == 0, \
		     "CONFIG_FAT_SECTORS_PER_CLUSTER must be a power of two");                     \
	BUILD_ASSERT(FAT_BYTES_PER_CLUSTER(sector_size) <= 32768,                                  \
		     "Cluster size must not exceed 32 KiB");                                       \
	BUILD_ASSERT(FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size) > 0);                   \
	BUILD_ASSERT(FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size) < 65525,                \
		     "Only FAT12 and FAT16 are supported");                                        \
	BUILD_ASSERT(FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size) < 4085 - 16 ||          \
			     FAT_NUM_OF_CLUSTERS(sector_size, data_partition_size) > 4086 + 16,    \
		     "Amount of clusters should not be close to the boundary between FAT12 and "   \
		     "FAT16, because filesystem "                                                  \
		     "type detection might not work reliably");                                    \
                                                                                                   \
	static const struct fat1x_BS name = {                                                      \
		.BS_jmpBoot = {0xeb, 0x3c, 0x90}, /* Jump instruction on x86 */                    \
		.BS_OEMName =                                                                      \
			{'Z', 'e', 'p', 'h', 'y', 'r', 'M',                                        \
			 'S'}, /* This is some indication of what system formatted the volume.*/   \
		.BPB_BytsPerSec = sys_cpu_to_le16(sector_size),                                    \
		.BPB_SecPerClus = CONFIG_FAT_SECTORS_PER_CLUSTER,                                  \
		.BPB_RsvdSecCnt = sys_cpu_to_le16(FAT_BS_RSVD_SECTORS),                            \
		.BPB_NumFATs = FAT_BS_NUM_FATS,                                                    \
		.BPB_RootEntCnt = sys_cpu_to_le16(FAT_BS_ROOT_ENT_CNT(sector_size)),               \
		.BPB_TotSec16 = sys_cpu_to_le16(                                                   \
			(FAT_BS_TOT_SEC(sector_size, data_partition_size) > 0xffff                 \
				 ? 0                                                               \
				 : (uint16_t)FAT_BS_TOT_SEC(sector_size, data_partition_size))),   \
		.BPB_Media = 0xF0, /* F0 designates non-partitioned devices */                     \
		.BPB_FATSz16 = sys_cpu_to_le16(                                                    \
			FAT_NUM_SECTORS_PER_FAT(sector_size, data_partition_size)),                \
		.BPB_SecPerTrk = 0,                                                                \
		.BPB_NumHeads = 0,                                                                 \
		.BPB_HiddSec = 0,                                                                  \
		.BPB_TotSec32 = sys_cpu_to_le32(                                                   \
			FAT_BS_TOT_SEC(sector_size, data_partition_size) > 0xffff                  \
				? FAT_BS_TOT_SEC(sector_size, data_partition_size)                 \
				: 0), /* If more than 0xffff sectors exist, this field is used */  \
		.BS_DrvNum = 0x00,    /* Spec says set 0x80 or 0x00*/                              \
		.BS_Reserved1 = 0x00,                                                              \
		.BS_BootSig = 0x29, /* 0x29 means that BSVolID and label is set*/                  \
		.BS_VolID = sys_cpu_to_le32(0x5faaa57a), /* 15.11.2020 15:36*/                     \
		.BS_VolLab = volume_label,                                                         \
		.BS_FilSysType = {'F', 'A', 'T', '1',                                              \
				  FAT_TYPE(sector_size, data_partition_size) == FAT12 ? '2' : '6', \
				  ' ', ' ', ' '},                                                  \
	}

/**
 * @brief handler that is called whenever the file is read by the host
 *
 * This handler is called by the virtual file stack whenever the host reads
 * the file. This handler gets calles multiple times with an offset within the
 * file and expects that the buffer gets filled by the function.
 *
 * @param offset    offset within the file
 * @param buf       transmit buffer to be filled by this function
 * @param len       buffer length
 * @param user_data user_data field from the virtual_file struct.
 *
 * @return number of bytes written to @p buf
 */
typedef size_t (*get_file_content_chunk_t)(off_t offset, uint8_t *buf, size_t len, void *user_data);

/**
 * @brief handler that is called whenever the file is written by the host
 *
 * This handler is called by the virtual file stack whenever the host writes
 * the file.
 *
 * @param offset    offset within the file
 * @param buf       data the host wants to write
 * @param len       buffer length
 * @param user_data user_data field from the virtual_file struct.
 *
 */
typedef void (*put_file_content_chunk_t)(off_t offset, const uint8_t *buf, size_t len,
					 void *user_data);

/** Virtual file to be added to a virtual_fat filesystem */
struct virtual_file {
	/**< File name, max 8 char all uppercase */
	const char *name;
	/** < File ending, i.e. TXT */
	char ext[3];
	/** only FAT_ATTR_READ_ONLY, FAT_ATTR_HIDDEN and FAT_ATTR_SYSTEM are allowed */
	uint8_t attributes;
	/** file size in bytes */
	uint32_t file_size;
	/** this handler gets called whenever the host reads a chunk of data from the file*/
	get_file_content_chunk_t get_chunk;
	/** void* parameter passed to the get handler and set by the user */
	void *user_data;
};

/** Virtual FAT context structure.
 *
 *  All fields in this structure are internal and should not be accessed by user code.
 */
struct virtual_fat_ctx {
	/** @cond INTERNAL_HIDDEN */
	const uint32_t sector_size;
	const uint32_t data_partition_size;
	const enum fat_type fat_type;
	const struct fat1x_BS *boot_sector;
	const struct virtual_file *virtual_files;
	put_file_content_chunk_t new_file_write_cb;
	void *new_file_write_user_data;
	size_t files_len;
	uint32_t file_size;
	uint32_t last_file_offs;
	char volume_label[11 + 1];
	char filename[8 + 1];
	/** @endcond */
};

/**
 * @brief Define and initialize a virtual FAT partition.
 *
 * @param name             name of the @ref virtual_fat_ctx object
 * @param vol_label        initial volume label
 * @param sect_size        sector size in bytes
 * @param data_part_size   data partition size in bytes
 */
#define VIRTUAL_FAT_DEFINE_PARTITION(name, vol_label, sect_size, data_part_size)                   \
                                                                                                   \
	VIRTUAL_FAT_DEFINE_BOOT_SECTOR(virtual_fat_boot_sector_##name, (vol_label), (sect_size),   \
				       (data_part_size));                                          \
                                                                                                   \
	struct virtual_fat_ctx name = {.sector_size = (sect_size),                                 \
				       .data_partition_size = (data_part_size),                    \
				       .fat_type = FAT_TYPE((sect_size), (data_part_size)),        \
				       .boot_sector = &virtual_fat_boot_sector_##name,             \
				       .virtual_files = NULL,                                      \
				       .files_len = 0,                                             \
				       .last_file_offs = 0,                                        \
				       .volume_label = (vol_label)}

/**
 * @brief register files that are shown in the mass storage device
 *
 * This function accepts an array with virtual files that should be displayed
 * then the mass storage device is mounted by the host.
 * When the device is mounted, this array must not change!
 *
 * @param ctx       virtual FAT volume context
 * @param files     array of virtual files
 * @param files_len length of the array
 * @retval 0 on success, negative errno code otherwise
 */
int register_virtual_files(struct virtual_fat_ctx *ctx, const struct virtual_file *files,
			   size_t files_len);

/**
 * @brief register a callback that is called when data for a new file is written
 *
 * @param ctx       virtual FAT volume context
 * @param callback  function called when data is written to a new file
 * @param user_data data passed to @p callback
 */
void register_new_file_write_cb(struct virtual_fat_ctx *ctx, put_file_content_chunk_t callback,
				void *user_data);

/**
 * @brief set the volume label
 *
 * This function sets the volume label (first file in the root directory) to
 * a given string. The maximum length is 11. The string is capped if it is too long
 *
 * @param ctx    virtual FAT volume context
 * @param label  new label
 */
void set_volume_label(struct virtual_fat_ctx *ctx, const char *label);

/**
 * @brief Read from a virtual FAT volume.
 *
 * @param ctx     virtual FAT volume context
 * @param buf     destination buffer
 * @param len     number of bytes to read
 * @param lba     logical block address
 * @param offset  byte offset within the logical block
 * @retval 0 on success, negative errno code otherwise
 */
int virtual_fat_read(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len, uint32_t lba,
		     uint32_t offset);

/**
 * @brief Write to a virtual FAT volume.
 *
 * @param ctx     virtual FAT volume context
 * @param buf     source buffer
 * @param len     number of bytes to write
 * @param lba     logical block address
 * @param offset  byte offset within the logical block
 * @retval 0 on success, negative errno code otherwise
 */
int virtual_fat_write(struct virtual_fat_ctx *ctx, const uint8_t *buf, size_t len, uint32_t lba,
		      uint32_t offset);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* LIBMCU_COMPONENTS_VIRTUAL_FAT_INCLUDE_VIRTUAL_FAT_H */
