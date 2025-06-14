/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/storage/ffatdisk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ffat, CONFIG_FFATDISK_LOG_LEVEL);

#define FAT16_CLUSTERS_MIN		0x0FF5U
#define FAT16_CLUSTERS_MAX		0xFFF4U
#define FAT16_END_OF_CHAIN		0xFFFFU
#define FAT16_FIRST_ENTRY		0xFFF8U

#define FAT32_CLUSTERS_MIN		0x0000FFF5UL
#define FAT32_CLUSTERS_MAX		0x0FFFFFF4UL
#define FAT32_END_OF_CHAIN		0x0FFFFFFFUL
#define FAT32_FIRST_ENTRY		0x0FFFFFF8UL

#define FAT_BS_SECTOR			0UL
#define FAT_BS_BACKUP_SECTOR		6UL
#define FAT_BS_SIG_WORD			0xAA55U
#define FAT_BS_SIG_WORD_OFFSET		510U

#define FAT_FSI_SECTOR			1UL
#define FAT_FSI_BACKUP_SECTOR		7UL
#define FAT_FSI_LEAD_SIG		0x41615252UL
#define FAT_FSI_STRUC_SIG		0x61417272UL
#define FAT_FSI_TRAIL_SIG		0xAA550000UL

#define FAT_DIR_ATTR_READ_ONLY		BIT(0)
#define FAT_DIR_ATTR_HIDDEN		BIT(1)
#define FAT_DIR_ATTR_SYSTEM		BIT(2)
#define FAT_DIR_ATTR_VOLUME_ID		BIT(3)
#define FAT_DIR_ATTR_DIRECTORY		BIT(4)
#define FAT_DIR_ATTR_ARCHIVE		BIT(5)

/*
 * Number of sectors/clusters for the root directory, informative only,
 * should not be changed. So the number of files/directories is limited
 * by this, but should be enough for all use cases.
 */
#define FFAT16_RD_SECTORS		1U
#define FFAT32_RD_CLUSTERS		1U

/*
 * For the FAT structures from the spec, we use the same identifier scheme,
 * just dropped the prefix and converted from silly camel case.
 */

struct fat16_ebpb {
	uint8_t drv_num;		/* Drive number */
	uint8_t reserved1;		/* Reserved */
	uint8_t boot_sig;		/* Extended boot signature */
	uint32_t vol_id;		/* Volume serial number */
	uint8_t vol_lab[11];		/* Volume label */
	uint8_t fil_sys_type[8];	/* Filesystem type */
} __packed;

struct fat32_ebpb {
	uint32_t fat_sz32;		/* Number of sectors occupied by one FAT */
	uint16_t ext_flags;		/* Flags */
	uint16_t fs_ver;		/* Always 0 */
	uint32_t root_clus;		/* First cluster of the root directory */
	uint16_t fs_info;		/* Sector number of fsinfo structure */
	uint16_t bk_boot_sec;		/* Sector number of fsinfo structure */
	uint8_t reserved[12];		/* Reserved */
	uint8_t drv_num;		/* Drive number */
	uint8_t reserved1;		/* Reserved */
	uint8_t boot_sig;		/* Extended boot signature */
	uint32_t vol_id;		/* Volume serial number */
	uint8_t vol_lab[11];		/* Volume label */
	uint8_t fil_sys_type[8];	/* File-system type */
} __packed;

struct fat_boot_sector {
	uint8_t jump_boot[3];		/* Jump instruction */
	uint8_t oem_name[8];		/* OEM name or ID */
	/* BIOS parameter block */
	uint16_t byts_per_sec;		/* Sector size in bytes
					 * 512, 1024, 2048 or 4096
					 */
	uint8_t sec_per_clus;		/* Number of sectors per cluster,
					 * 1, 2, 4, 8, 16, 32, 64
					 */
	uint16_t rsvd_sec_cnt;		/* Number of reserved sectors */
	uint8_t num_fats;		/* Number of FATs */
	uint16_t root_ent_cnt;		/* Number of root directory entries */
	uint16_t tot_sec16;		/* Number of sectors */
	uint8_t media;			/* Media code */
	uint16_t fat_sz16;		/* FAT length in sectors */
	uint16_t sec_per_trk;		/* Number of sectors per track */
	uint16_t num_heads;		/* Number of heads */
	uint32_t hidd_sec;		/* Hidden sectors */
	uint32_t tot_sec32;		/* Total number of sectors */
	/* Extended BIOS parameter block */
	union {
		struct fat16_ebpb ebpb16;
		struct fat32_ebpb ebpb32;
	};
} __packed;

struct fat_fsi_sector {
	uint32_t lead_sig;
	uint8_t reserved1[480];
	uint32_t struc_sig;
	uint32_t free_count;
	uint32_t nxt_free;
	uint8_t reserved2[12];
	uint32_t trail_sig;
} __packed;

BUILD_ASSERT(sizeof(struct fat_fsi_sector) == 512);

struct fat_dir_entry {
	char name[FAT_FILE_NAME_LEN];	/* File name */
	uint8_t attr;			/* File attribute */
	uint8_t ntres;			/* Reserved */
	uint8_t crt_time_tenth;		/* Creation time 10ms */
	uint16_t crt_time;		/* Creation time */
	uint16_t crt_date;		/* Creation date */
	uint16_t lst_acc_date;		/* Last access date */
	uint16_t fst_clus_hi;		/* Zero on FAT16 */
	uint16_t wrt_time;		/* Last modification time */
	uint16_t wrt_date;		/* Last modification date */
	uint16_t fst_clus_lo;		/* File name */
	uint32_t file_size;		/* File size in bytes */
} __packed;

BUILD_ASSERT(sizeof(struct fat_dir_entry) == 32);

struct ffatdisk_data {
	sys_slist_t f_list;
	struct disk_info info;
	struct ffat_file vol_id;
	uint32_t clusters_free;
};

struct ffatdisk_config {
	const struct fat_boot_sector *fat_bs;
	uint32_t fat_entries;
	uint32_t root_ent_cnt;
	uint32_t fdc;
	uint32_t fat1_start;
	uint32_t fat2_start;
	uint32_t rd_start;
	uint32_t data_start;
	uint32_t sector_count;
	uint32_t clusters;
	uint32_t cluster_size;
	void (*ffat_read)(const struct device *dev, uint8_t *const buf,
			  const uint32_t fat_sector);
	bool fat32;
};

static ALWAYS_INLINE size_t bs_get_byts_per_sec(const struct device *dev)
{
	const struct ffatdisk_config *config = dev->config;

	return config->fat_bs->byts_per_sec;
}

static ALWAYS_INLINE size_t bs_get_sec_per_clus(const struct device *dev)
{
	const struct ffatdisk_config *config = dev->config;

	return config->fat_bs->sec_per_clus;
}

static ALWAYS_INLINE size_t file_size_in_clusters(const struct device *dev,
						  const struct ffat_file *const f,
						  uint32_t c_free)
{
	const struct ffatdisk_config *config = dev->config;
	size_t q  = f->size / config->cluster_size;

	if (f->size % config->cluster_size) {
		q += 1;
	}

	return MIN(q, c_free);
}

static int ffat_init_files(struct disk_info *const disk)
{
	const struct device *dev = disk->dev;
	const struct ffatdisk_config *config = dev->config;
	struct ffatdisk_data *const data = dev->data;
	struct ffat_file *const vol_id = &data->vol_id;
	unsigned int rd_entries = 0;
	uint32_t c_free = config->clusters;
	uint32_t c_num = config->fdc;

	/* In the case of FAT32, one cluster is reserved for the root directory. */
	if (config->fat32) {
		c_free -= FFAT32_RD_CLUSTERS;
	}

	/* Add Volume ID entry */
	sys_slist_append(&data->f_list, &vol_id->node);
	rd_entries++;

	LOG_INF("%u cluster free, cluster size %u", c_free, config->cluster_size);

	STRUCT_SECTION_FOREACH(ffat_file, f) {
		uint32_t f_clusters;

		if (strcmp(disk->name, f->disk_name) != 0) {
			/* Not our file */
			continue;
		}

		f_clusters = file_size_in_clusters(dev, f, c_free);

		if (f_clusters) {
			f->start = c_num;
			f->end  = c_num + f_clusters - 1;
			c_num += f_clusters;
		}

		LOG_INF("Add file to disk %s, start %zu, end %zu, size %zu (%zu)",
			f->disk_name, f->start, f->end, f->size, f_clusters);

		/* Fix file name if necessary */
		for (size_t n = 0; n < sizeof(f->name); n++) {
			f->name[n] = f->name[n] ? f->name[n] : ' ';
		}

		if (f->wr_cb == NULL) {
			f->attr |= FAT_DIR_ATTR_READ_ONLY;
		}

		if (f->rd_cb == NULL) {
			f->attr |= FAT_DIR_ATTR_HIDDEN;
		}

		sys_slist_append(&data->f_list, &f->node);
		c_free -= f_clusters;

		if (++rd_entries >= config->root_ent_cnt || !c_free) {
			LOG_INF("Disk is full");
			break;
		}
	}

	data->clusters_free = c_free;

	return 0;
}

static struct ffat_file *ffat_get_file(const struct device *dev,
				       const uint32_t cluster)
{
	struct ffatdisk_data *const data = dev->data;
	struct ffat_file *f;

	SYS_SLIST_FOR_EACH_CONTAINER(&data->f_list, f, node) {
		if (IN_RANGE(cluster, f->start, f->end)) {
			return f;
		}
	}

	return NULL;
}

static void ffat_read_bs(const struct device *dev, uint8_t *const buf)
{
	const struct ffatdisk_config *config = dev->config;

	memcpy(buf, config->fat_bs, sizeof(struct fat_boot_sector));

	sys_put_le16(FAT_BS_SIG_WORD, &buf[FAT_BS_SIG_WORD_OFFSET]);
}

static void ffat_read_fsi(const struct device *dev, uint8_t *const buf)
{
	struct ffatdisk_data *const data = dev->data;
	struct fat_fsi_sector *fsi = (void *)buf;

	sys_put_le16(FAT_BS_SIG_WORD, &buf[FAT_BS_SIG_WORD_OFFSET]);
	fsi->lead_sig = sys_cpu_to_le32(FAT_FSI_LEAD_SIG);
	fsi->struc_sig = sys_cpu_to_le32(FAT_FSI_STRUC_SIG);
	fsi->free_count = data->clusters_free;
	fsi->nxt_free = 0xFFFFFFFFUL;
	fsi->trail_sig = sys_cpu_to_le32(FAT_FSI_TRAIL_SIG);
}

/* Read FAT entry, that is either pointer to the next entry or EOC. */
static ALWAYS_INLINE void ffat_read_fat32_entry(struct ffat_file *const f,
						uint32_t *const ptr,
						const uint32_t e)
{
	if (e < f->end) {
		*ptr = sys_cpu_to_le32(e + 1U);
	} else {
		*ptr = sys_cpu_to_le32(FAT32_END_OF_CHAIN);
	}
}

static inline void ffat_read_fat32(const struct device *dev, uint8_t *const buf,
				   const uint32_t fat_sector)
{
	const struct ffatdisk_config *config = dev->config;
	struct ffat_file *f = NULL;
	uint32_t *ptr = (void *)buf;
	uint32_t fa_idx;

	fa_idx = fat_sector * config->fat_entries;

	for (uint32_t i = 0; i < config->fat_entries; i++) {

		/* Do not fetch file if it is in range. */
		if (f != NULL && IN_RANGE(fa_idx + i, f->start, f->end)) {
			ffat_read_fat32_entry(f, &ptr[i], fa_idx + i);
			continue;
		} else {
			/* Get file that is in range. */
			f = ffat_get_file(dev, fa_idx + i);

			if (f != NULL) {
				ffat_read_fat32_entry(f, &ptr[i], fa_idx + i);
				continue;
			}
		}

		if (fa_idx + i == 0U) {
			ptr[i] = sys_cpu_to_le32(FAT32_FIRST_ENTRY);
			continue;
		}

		if (fa_idx + i == 1U) {
			ptr[i] = sys_cpu_to_le32(FAT32_END_OF_CHAIN);
			continue;
		}

		if (fa_idx + i == 2U) {
			ptr[i] = sys_cpu_to_le32(FAT32_END_OF_CHAIN);
			continue;
		}
	}

	LOG_DBG("Read FAT sector %u", fat_sector);
}

/* Read FAT entry, that is either pointer to the next entry or EOC. */
static ALWAYS_INLINE void ffat_read_fat16_entry(struct ffat_file *const f,
						uint16_t *const ptr,
						const uint16_t e)
{
	if (e < f->end) {
		*ptr = sys_cpu_to_le16(e + 1);
	} else {
		*ptr = sys_cpu_to_le16(FAT16_END_OF_CHAIN);
	}
}

static inline void ffat_read_fat16(const struct device *dev, uint8_t *const buf,
				   const uint32_t fat_sector)
{
	const struct ffatdisk_config *config = dev->config;
	struct ffat_file *f = NULL;
	uint16_t *ptr = (void *)buf;
	uint32_t fa_idx;

	fa_idx = fat_sector * config->fat_entries;

	for (uint32_t i = 0; i < config->fat_entries; i++) {

		/* Do not fetch file if it is in range. */
		if (f != NULL  && IN_RANGE(fa_idx + i, f->start, f->end)) {
			ffat_read_fat16_entry(f, &ptr[i], fa_idx + i);
			continue;
		} else {
			/* Get file that is in range. */
			f = ffat_get_file(dev, fa_idx + i);

			if (f != NULL) {
				ffat_read_fat16_entry(f, &ptr[i], fa_idx + i);
				continue;
			}
		}

		if (fa_idx + i == 0U) {
			ptr[i] = sys_cpu_to_le16(FAT16_FIRST_ENTRY);
			continue;
		}

		if (fa_idx + i == 1U) {
			ptr[i] = sys_cpu_to_le16(FAT16_END_OF_CHAIN);
			continue;
		}
	}

	LOG_DBG("Read FAT sector %u", fat_sector);
}

static void ffat_read_rd(const struct device *dev, uint8_t *const buf,
			 const uint32_t sector)
{
	const struct ffatdisk_config *config = dev->config;
	struct ffatdisk_data *const data = dev->data;
	struct fat_dir_entry *de = (void *)buf;
	uint32_t rd_sector = sector - config->rd_start;
	struct ffat_file *f;

	LOG_DBG("Read %u RD entries, sector %u (%u)",
		config->root_ent_cnt, rd_sector, sector);

	if (rd_sector != 0) {
		/* Ignore the higher sectors of a FAT32 cluster */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&data->f_list, f, node) {
		memcpy(de->name, f->name, sizeof(de->name));
		/* TODO Set wrt_time and wrt_data */
		de->wrt_time = 0;
		de->wrt_date = 0;

		de->fst_clus_lo = f->start;
		de->fst_clus_hi = config->fat32 ? f->start >> 16 : 0;

		de->file_size = f->size;
		de->attr = f->attr;

		de++;
	}
}

/* Get any file with a valid write callback */
static struct ffat_file *ffat_get_file_any_wr_cb(const struct device *dev)
{
	struct ffatdisk_data *const data = dev->data;
	struct ffat_file *f;

	SYS_SLIST_FOR_EACH_CONTAINER(&data->f_list, f, node) {
		if (f->wr_cb != NULL) {
			return f;
		}
	}

	return NULL;
}

/* Get file and file block index based on sector */
static struct ffat_file *ffat_get_file_and_block(const struct device *dev,
						 const uint32_t sector,
						 uint32_t *const f_block)
{
	const struct ffatdisk_config *config = dev->config;
	uint32_t d_sector = sector - config->data_start;
	struct ffat_file *f;
	uint32_t start_sector;
	uint32_t c_num;

	/* Get cluster number from relative data region sector number */
	c_num = d_sector / bs_get_sec_per_clus(dev) + config->fdc;

	f = ffat_get_file(dev, c_num);
	if (f == NULL) {
		*f_block = 0UL;
		return NULL;
	}

	/* Get relative file start sector based on first cluster
	 * that is start of data region.
	 */
	start_sector = (f->start - config->fdc) * bs_get_sec_per_clus(dev);

	/* For the file write/read callback we need a block index
	 * relative to the file start sector.
	 */
	*f_block = d_sector - start_sector;
	LOG_DBG("File block %u (s %u, d_s %u, c %u, start_s %u))",
		*f_block, sector, d_sector, c_num, start_sector);

	return f;
}

static void ffat_read_file(const struct device *dev, uint8_t *const buf,
			   const uint32_t sector)
{
	uint32_t byts_per_sec = bs_get_byts_per_sec(dev);
	struct ffat_file *f;
	uint32_t f_block;

	f = ffat_get_file_and_block(dev, sector, &f_block);

	if (f != NULL && f->rd_cb != NULL) {
		f->rd_cb(f, f_block, buf, byts_per_sec);
		LOG_DBG("Read file block %u (%u)", f_block, sector);
	}
}

static void ffat_read_sector(struct disk_info *const disk, uint8_t *const buf,
			     const uint32_t sector)
{
	const struct device *dev = disk->dev;
	const struct ffatdisk_config *config = dev->config;
	uint32_t fat_sector;

	if (IN_RANGE(sector, 0, config->sector_count - 1)) {
		memset(buf, 0, bs_get_byts_per_sec(dev));
	}

	if (sector == FAT_BS_SECTOR ||
	    (config->fat32 && sector == FAT_BS_BACKUP_SECTOR)) {
		ffat_read_bs(dev, buf);
		LOG_DBG("Read boot sector (%u)", sector);

		return;
	}

	if (config->fat32 &&
	    (sector == FAT_FSI_SECTOR || sector == FAT_FSI_BACKUP_SECTOR)) {
		ffat_read_fsi(dev, buf);
		LOG_DBG("Read FSI (%u)", sector);

		return;
	}

	if (IN_RANGE(sector, config->fat1_start, config->fat2_start - 1)) {
		fat_sector = sector - config->fat1_start;
		LOG_DBG("Read FAT1 sector %u", fat_sector);
		config->ffat_read(dev, buf, fat_sector);

		return;
	}

	if (IN_RANGE(sector, config->fat2_start, config->rd_start - 1)) {
		fat_sector = sector - config->fat2_start;
		LOG_DBG("Read FAT2 sector %u", fat_sector);
		config->ffat_read(dev, buf, fat_sector);

		return;
	}

	if (IN_RANGE(sector, config->rd_start, config->data_start - 1)) {
		ffat_read_rd(dev, buf, sector);

		return;
	}

	if (IN_RANGE(sector, config->data_start, config->sector_count - 1)) {
		ffat_read_file(dev, buf, sector);

		return;
	}
}

static int ffatdisk_access_read(struct disk_info *const disk, uint8_t *const buf,
				const uint32_t sector, const uint32_t count)
{
	const struct device *dev = disk->dev;
	const struct ffatdisk_config *config = dev->config;
	uint32_t sector_max = sector + count;

	if (sector_max < sector || sector_max > config->sector_count) {
		LOG_ERR("Sector %u is outside the range %u",
			sector_max, config->sector_count);
		return -EIO;
	}

	for (uint32_t i = sector; i < sector_max; i++) {
		ffat_read_sector(disk, buf, i);
	}

	return 0;
}

static int ffat_write_file(const struct device *dev,
			   const uint8_t *const buf, const uint32_t sector)
{
	struct ffatdisk_data *const data = dev->data;
	uint32_t byts_per_sec = bs_get_byts_per_sec(dev);
	struct ffat_file *f;
	uint32_t f_block = 0UL;

	if (data->clusters_free) {
		/*
		 * If there are free clusters on the volume,
		 * the filesystem driver can write to any of the free
		 * clusters and we cannot determine the sector number
		 * of the file. Therefore, there should be only one file
		 * with write callback. To get the exact sector (or block)
		 * number and other metadata, backends need to encapsulate
		 * the payload, which we do not care about at all.
		 */
		f = ffat_get_file_any_wr_cb(dev);
	} else {
		/*
		 * If there are no free clusters on the volume,
		 * we can determine the sector index of the file.
		 * This is nice, but less practical because some OS
		 * do not want to overwrite the file if there is no space.
		 */
		f = ffat_get_file_and_block(dev, sector, &f_block);
	}

	if (f != NULL && f->wr_cb != NULL) {
		f->wr_cb(f, f_block, buf, byts_per_sec);
		LOG_DBG("Write file block %u (%u)", f_block, sector);
	}

	return 0;
}

static int ffat_write_sector(struct disk_info *const disk,
			     const uint8_t *const buf, const uint32_t sector)
{
	const struct device *dev = disk->dev;
	const struct ffatdisk_config *config = dev->config;
	uint32_t fat_sector;

	/*
	 * For now, we ignore write accesses from the (host) filesystem driver
	 * to all sectors except the data area. Perhaps we can use some of these
	 * to implement a mounted/unmounted state indication callback.
	 */

	if (sector == FAT_BS_SECTOR ||
	    (config->fat32 && sector == FAT_BS_BACKUP_SECTOR)) {
		LOG_DBG("Write boot sector");

		return 0;
	}

	if (config->fat32 &&
	    (sector == FAT_FSI_SECTOR || sector == FAT_FSI_BACKUP_SECTOR)) {
		LOG_DBG("Write FSI %u", sector);

		return 0;
	}

	if (IN_RANGE(sector, config->fat1_start, config->fat2_start - 1)) {
		fat_sector = sector - config->fat1_start;
		LOG_DBG("Write FAT1, sector %u (%u)", fat_sector, sector);

		return 0;
	}

	if (IN_RANGE(sector, config->fat2_start, config->rd_start - 1)) {
		fat_sector = sector - config->fat2_start;
		LOG_DBG("Write FAT2, sector %u (%u)", fat_sector, sector);

		return 0;
	}

	if (IN_RANGE(sector, config->rd_start, config->data_start - 1)) {
		LOG_DBG("Write root directory (%u)", sector);

		return 0;
	}

	if (IN_RANGE(sector, config->data_start, config->sector_count - 1)) {
		return ffat_write_file(dev, buf, sector);
	}

	return 0;
}

static int ffatdisk_access_write(struct disk_info *disk, const uint8_t *buf,
				 uint32_t sector, uint32_t count)
{
	const struct device *dev = disk->dev;
	const struct ffatdisk_config *config = dev->config;
	uint32_t sector_max = sector + count;

	if (sector_max < sector || sector_max > config->sector_count) {
		LOG_ERR("Sector %zu is outside the range %zu",
			sector_max, config->sector_count);
		return -EIO;
	}

	for (uint32_t i = sector; i < sector_max; i++) {
		ffat_write_sector(disk, buf, sector);
	}

	return 0;
}

static int ffatdisk_init(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	const struct ffatdisk_config *config = dev->config;

	LOG_INF("FAT1 start %u, FAT2 start %u, RD start %u, data start %u",
		config->fat1_start, config->fat2_start,
		config->rd_start, config->data_start);

	LOG_INF("tot_sec16 %u, tot_sec32 %u fat_sz16 %u clusters %u",
		config->fat_bs->tot_sec16,
		config->fat_bs->tot_sec32,
		config->fat_bs->fat_sz16,
		config->clusters);

	return 0;
}

static int ffatdisk_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buf)
{
	const struct ffatdisk_config *config = disk->dev->config;

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buf = config->sector_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buf = bs_get_byts_per_sec(disk->dev);
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buf  = 1U;
		break;
	case DISK_IOCTL_CTRL_INIT:
		return ffatdisk_init(disk);
	case DISK_IOCTL_CTRL_DEINIT:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ffatdisk_access_status(struct disk_info *disk)
{
	return DISK_STATUS_OK;
}

static int ffatdisk_preinit(const struct device *dev)
{
	struct ffatdisk_data *const data = dev->data;

	data->info.dev = dev;
	ffat_init_files(&data->info);

	return disk_access_register(&data->info);
}

static const struct disk_operations ffatdisk_ops = {
	.init = ffatdisk_init,
	.status = ffatdisk_access_status,
	.read = ffatdisk_access_read,
	.write = ffatdisk_access_write,
	.ioctl = ffatdisk_access_ioctl,
};

#define DT_DRV_COMPAT			zephyr_ffat_disk

#define FFAT_DEFAULT_NUM_FAT		2U
#define FFAT_DEFAULT_MEDIA		0xF8U
#define FFAT_DEFAULT_DRV_NUM		0x80U
#define FFAT_EXTENDED_BOOT_SIG		0x29U

#define FFAT_BS_TOT_SEC16(n)								\
	(DT_INST_PROP(n, sector_count) > UINT16_MAX ? 0 : (uint16_t)DT_INST_PROP(n, sector_count))

#define FFAT_BS_TOT_SEC32(n)								\
	(DT_INST_PROP(n, sector_count) > UINT16_MAX ? DT_INST_PROP(n, sector_count) : 0)

/* Get the number of clusters on the volume rounded up */
#define FFAT_CLUSTER_ROUND_UP(n)							\
	DIV_ROUND_UP(DT_INST_PROP(n, sector_count),					\
		     DT_INST_PROP(n, sector_per_cluster))

/* Cluster size in bytes */
#define FFAT_CLUSTER_SIZE(n)								\
	(DT_INST_PROP(n, sector_size) * DT_INST_PROP(n, sector_per_cluster))

/*
 * Get the number of FAT directory entries in a sector.
 * We limit the number of files to what will fit into a sector.
 * This means that the maximum for a 512 byte sector is 16 entries,
 * which should be enough for the use cases of this design.
 * We use the same macro for FAT32 to avoid any special handling.
 */
#define FFAT16_ROOT_ENT_CNT(n)								\
	(DT_INST_PROP(n, sector_size) / sizeof(struct fat_dir_entry))

/* Get the number of FAT16 entries in a single sector */
#define FFAT16_ENT_IN_SECTOR(n)								\
	(DT_INST_PROP(n, sector_size) / sizeof(uint16_t))

/* Get the size of a FAT16 in sectors */
#define FFAT_SZ16(n)									\
	DIV_ROUND_UP(FFAT_CLUSTER_ROUND_UP(n), FFAT16_ENT_IN_SECTOR(n))

#define FFAT16_RSVD_SEC_CNT		1U

/* Entries FAT[0] and FAT[1] are reserved */
#define FAT16_FIRST_DATA_CLUSTER	2UL

#define FFAT16_FAT1_START	FFAT16_RSVD_SEC_CNT
#define FFAT16_FAT2_START(n)	(FFAT16_RSVD_SEC_CNT + FFAT_SZ16(n))
#define FFAT16_RD_START(n)	(FFAT16_FAT2_START(n) + FFAT_SZ16(n))
#define FFAT16_DATA_START(n)	(FFAT16_RD_START(n) + FFAT16_RD_SECTORS)

/* Number of sectors in the data region */
#define FFAT16_DATA_SECTORS(n)								\
	(DT_INST_PROP(n, sector_count) - FFAT16_DATA_START(n))

/* Actually number of clusters */
#define FFAT16_CLUSTERS(n)								\
	(FFAT16_DATA_SECTORS(n) / DT_INST_PROP(n, sector_per_cluster))

#define FFATDISK_CONFIG_FAT16_DEFINE(n)							\
	BUILD_ASSERT(FFAT16_CLUSTERS(n) >= FAT16_CLUSTERS_MIN,				\
		     "FAT16 cluster count too low");					\
	BUILD_ASSERT(FFAT16_CLUSTERS(n) <= FAT16_CLUSTERS_MAX,				\
		     "FAT16 cluster count too high");					\
											\
	static const struct fat_boot_sector fat_bs_##n = {				\
		.jump_boot = {0xEB, 0xFF, 0x90},					\
		.oem_name = "Zephyr  ",							\
		.byts_per_sec = DT_INST_PROP(n, sector_size),				\
		.sec_per_clus = DT_INST_PROP(n, sector_per_cluster),			\
		.rsvd_sec_cnt = FFAT16_RSVD_SEC_CNT,					\
		.num_fats = FFAT_DEFAULT_NUM_FAT,					\
		.root_ent_cnt = FFAT16_ROOT_ENT_CNT(n),					\
		.tot_sec16 = FFAT_BS_TOT_SEC16(n),					\
		.media = FFAT_DEFAULT_MEDIA,						\
		.fat_sz16 = FFAT_SZ16(n),						\
		.sec_per_trk = 1U,							\
		.num_heads = 1U,							\
		.hidd_sec = 0U,								\
		.tot_sec32 = FFAT_BS_TOT_SEC32(n),					\
		.ebpb16 = {								\
			.drv_num = FFAT_DEFAULT_DRV_NUM,				\
			.reserved1 = 0U,						\
			.boot_sig = FFAT_EXTENDED_BOOT_SIG,				\
			.vol_id = 0x00420042UL,						\
			.vol_lab = "NO NAME    ",					\
			.fil_sys_type = "FAT16   ",					\
		},									\
	};										\
											\
	static const struct ffatdisk_config ffatdisk_config_##n = {			\
		.fat_bs = &fat_bs_##n,							\
		.sector_count = DT_INST_PROP(n, sector_count),				\
		.fat_entries = FFAT16_ENT_IN_SECTOR(n),					\
		.root_ent_cnt = FFAT16_ROOT_ENT_CNT(n),					\
		.fdc = FAT16_FIRST_DATA_CLUSTER,					\
		.fat1_start = FFAT16_FAT1_START,					\
		.fat2_start = FFAT16_FAT2_START(n),					\
		.rd_start = FFAT16_RD_START(n),						\
		.data_start = FFAT16_DATA_START(n),					\
		.clusters = FFAT16_CLUSTERS(n),						\
		.cluster_size = FFAT_CLUSTER_SIZE(n),					\
		.ffat_read = ffat_read_fat16,						\
		.fat32 = false,								\
	}

/* Get the number of FAT32 entries in a single sector */
#define FFAT32_ENT_IN_SECTOR(n)	(DT_INST_PROP(n, sector_size) / sizeof(uint32_t))

/* Get the size of a FAT32 in sectors */
#define FFAT_SZ32(n)									\
	DIV_ROUND_UP(FFAT_CLUSTER_ROUND_UP(n), FFAT32_ENT_IN_SECTOR(n))

#define FFAT32_RSVD_SEC_CNT		16U

/* Entries FAT[0] and FAT[1] are reserved, FAT[3] is root directory */
#define FAT32_FIRST_DATA_CLUSTER	3UL

/* number of clusters */
#define FFAT32_RD_SECTORS(n)								\
	(FFAT32_RD_CLUSTERS * DT_INST_PROP(n, sector_per_cluster))

#define FFAT32_FAT1_START	FFAT32_RSVD_SEC_CNT
#define FFAT32_FAT2_START(n)	(FFAT32_RSVD_SEC_CNT + FFAT_SZ32(n))
#define FFAT32_RD_START(n)	(FFAT32_FAT2_START(n) + FFAT_SZ32(n))
#define FFAT32_DATA_START(n)	(FFAT32_RD_START(n) + FFAT32_RD_SECTORS(n))

/* Number of sectors in the data region */
#define FFAT32_DATA_SECTORS(n)								\
	(DT_INST_PROP(n, sector_count) - FFAT32_RD_START(n))

/* Actually number of clusters */
#define FFAT32_CLUSTERS(n)								\
	(FFAT32_DATA_SECTORS(n) / DT_INST_PROP(n, sector_per_cluster))

#define FFATDISK_CONFIG_FAT32_DEFINE(n)							\
	BUILD_ASSERT(FFAT32_CLUSTERS(n) >= FAT32_CLUSTERS_MIN,				\
		     "FAT32 cluster count too low");					\
	BUILD_ASSERT(FFAT32_CLUSTERS(n) <= FAT32_CLUSTERS_MAX,				\
		     "FAT32 cluster count too high");					\
											\
	static const struct fat_boot_sector fat_bs_##n = {				\
		.jump_boot = {0xEB, 0xFF, 0x90},					\
		.oem_name = "Zephyr  ",							\
		.byts_per_sec = DT_INST_PROP(n, sector_size),				\
		.sec_per_clus = DT_INST_PROP(n, sector_per_cluster),			\
		.rsvd_sec_cnt = FFAT32_RSVD_SEC_CNT,					\
		.num_fats = FFAT_DEFAULT_NUM_FAT,					\
		.root_ent_cnt = 0U,							\
		.tot_sec16 = 0U,							\
		.media = FFAT_DEFAULT_MEDIA,						\
		.fat_sz16 = 0U,								\
		.sec_per_trk = 1U,							\
		.num_heads = 1U,							\
		.hidd_sec = 0U,								\
		.tot_sec32 = FFAT_BS_TOT_SEC32(n),					\
		.ebpb32 = {								\
			.fat_sz32 = FFAT_SZ32(n),					\
			.ext_flags = 0U,						\
			.fs_ver = 0U,							\
			.root_clus = FAT32_FIRST_DATA_CLUSTER - 1UL,			\
			.fs_info = 1U,							\
			.bk_boot_sec = 6U,						\
			.drv_num = FFAT_DEFAULT_DRV_NUM,				\
			.reserved1 = 0U,						\
			.boot_sig = FFAT_EXTENDED_BOOT_SIG,				\
			.vol_id = 0x00420042UL,						\
			.vol_lab = "NO NAME    ",					\
			.fil_sys_type = "FAT32   ",					\
		},									\
	};										\
											\
	static const struct ffatdisk_config ffatdisk_config_##n = {			\
		.fat_bs = &fat_bs_##n,							\
		.sector_count = DT_INST_PROP(n, sector_count),				\
		.fat_entries = FFAT32_ENT_IN_SECTOR(n),					\
		.root_ent_cnt = FFAT16_ROOT_ENT_CNT(n),					\
		.fdc = FAT32_FIRST_DATA_CLUSTER,					\
		.fat1_start = FFAT32_FAT1_START,					\
		.fat2_start = FFAT32_FAT2_START(n),					\
		.rd_start = FFAT32_RD_START(n),						\
		.data_start = FFAT32_DATA_START(n),					\
		.clusters = FFAT32_CLUSTERS(n),						\
		.cluster_size = FFAT_CLUSTER_SIZE(n),					\
		.ffat_read = ffat_read_fat32,						\
		.fat32 = true,								\
	}

#define FFATDISK_CONFIG_DEFINE(n)							\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), zephyr_ffat32_disk),		\
		    (FFATDISK_CONFIG_FAT32_DEFINE(n)),					\
		    (FFATDISK_CONFIG_FAT16_DEFINE(n)))

#define FFATDISK_DEVICE_DEFINE(n)							\
	BUILD_ASSERT(DT_INST_PROP(n, sector_count) <= UINT32_MAX,			\
		     "Sector count is greater than UINT32_MAX");			\
											\
	FFATDISK_CONFIG_DEFINE(n);							\
											\
	static struct ffatdisk_data ffatdisk_data_##n = {				\
		.info  = {								\
			.name = DT_INST_PROP(n, disk_name),				\
			.ops = &ffatdisk_ops,						\
		},									\
		.vol_id = {								\
			.name = "NO NAME    ",						\
			.attr = FAT_DIR_ATTR_VOLUME_ID,					\
		},									\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, ffatdisk_preinit, NULL,				\
			      &ffatdisk_data_##n, &ffatdisk_config_##n,			\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &ffatdisk_ops);

DT_INST_FOREACH_STATUS_OKAY(FFATDISK_DEVICE_DEFINE)
