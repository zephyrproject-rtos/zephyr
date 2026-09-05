/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/virtual_fat.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(virtual_fat, CONFIG_VIRTUAL_FAT_LOG_LEVEL);

#define FAT12_FILENAME_LEN      8
#define FAT12_FILEEXTENSION_LEN 3

struct fat1x_dir {
	char DIR_Name[FAT12_FILENAME_LEN + FAT12_FILEEXTENSION_LEN];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	/* Component of the file creation time. Valid range: 0 - 199 */
	uint8_t DIR_CrtTimeTenth;
	/* Creation time. Granularity is 2 seconds. */
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	/* High word of first data cluster number for file/directory described by this entry */
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate_day: 5;
	uint16_t DIR_WrtDate_month: 4;
	uint16_t DIR_WrtDate_year: 7;
	/* Low word of first data cluster number for file/directory described by this entry.*/
	uint16_t DIR_FstClusLO;
	/* 32-bit quantity containing size in bytes of file/directory described by this entry. */
	uint32_t DIR_FileSize;
} __packed;

#define BOOT_SECTOR_SIGNATURE        0xaa55
#define BOOT_SECTOR_SIGNATURE_OFFSET 510

/* Defined by the standard. First entry in the fat is media type,
 * the second entry is EOC (end of cluster) 0xfff
 */
#define FAT_RESERVED_CLUSTERS 2

#define FAT_START_LBA FAT_BS_RSVD_SECTORS

static uint32_t get_fat_end_lba(const struct virtual_fat_ctx *ctx)
{
	return FAT_START_LBA +
	       (FAT_NUM_SECTORS_PER_FAT(ctx->sector_size, ctx->data_partition_size) *
		FAT_BS_NUM_FATS);
}

static uint32_t get_root_dir_start_lba(const struct virtual_fat_ctx *ctx)
{
	return get_fat_end_lba(ctx);
}

static uint32_t get_root_dir_end_lba(const struct virtual_fat_ctx *ctx)
{
	return get_root_dir_start_lba(ctx) + ROOT_DIR_SECTORS(ctx->sector_size);
}

static uint32_t get_data_start_lba(const struct virtual_fat_ctx *ctx)
{
	return get_root_dir_end_lba(ctx);
}

static uint32_t get_data_end_lba(const struct virtual_fat_ctx *ctx)
{
	return get_data_start_lba(ctx) + (ctx->data_partition_size / ctx->sector_size);
}

void register_new_file_write_cb(struct virtual_fat_ctx *ctx, put_file_content_chunk_t callback,
				void *user_data)
{
	ctx->new_file_write_cb = callback;
	ctx->new_file_write_user_data = user_data;
}

int register_virtual_files(struct virtual_fat_ctx *ctx, const struct virtual_file *files,
			   size_t files_len)
{
	if (files_len > FAT_BS_ROOT_ENT_CNT(ctx->sector_size)) {
		LOG_ERR("Too many files for root dir: %zu > %d", files_len,
			FAT_BS_ROOT_ENT_CNT(ctx->sector_size));
		return -EINVAL;
	}

	ctx->virtual_files = files;
	ctx->files_len = files_len;

	uint32_t last_file_offs = get_root_dir_end_lba(ctx);

	for (size_t i = 0; i < files_len; ++i) {
		last_file_offs +=
			DIV_ROUND_UP(files[i].file_size, FAT_BYTES_PER_CLUSTER(ctx->sector_size)) *
			CONFIG_FAT_SECTORS_PER_CLUSTER;
	}

	if (last_file_offs > get_data_end_lba(ctx)) {
		LOG_ERR("Not enough space for all files");
		return -ENOMEM;
	}

	ctx->last_file_offs = last_file_offs;

	return 0;
}

void set_volume_label(struct virtual_fat_ctx *ctx, const char *label)
{
	strncpy(ctx->volume_label, label, sizeof(ctx->volume_label));
	ctx->volume_label[sizeof(ctx->volume_label) - 1] = '\0';
}

static uint32_t get_fat_offset(const struct virtual_fat_ctx *ctx, uint32_t lba, uint32_t offset)
{
	return (lba - FAT_START_LBA) * ctx->sector_size + offset;
}

static uint32_t get_root_dir_offset(const struct virtual_fat_ctx *ctx, uint32_t lba,
				    uint32_t offset)
{
	return (lba - get_root_dir_start_lba(ctx)) * ctx->sector_size + offset;
}

static uint16_t get_cluster_nr_from_lba(const struct virtual_fat_ctx *ctx, uint32_t lba)
{
	return (lba - get_data_start_lba(ctx)) / CONFIG_FAT_SECTORS_PER_CLUSTER +
	       FAT_RESERVED_CLUSTERS;
}

static void transfer_boot_sector(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len,
				 uint32_t offset)
{
	uint8_t *rem_buf = buf;
	size_t rem_len = len;

	if (offset < sizeof(struct fat1x_BS)) {
		size_t data_residual = sizeof(struct fat1x_BS) - offset;
		size_t cpy_len = MIN(data_residual, len);

		memcpy(buf, ((uint8_t *)ctx->boot_sector) + offset, cpy_len);
		rem_len -= cpy_len;
		rem_buf += cpy_len;
	}

	if (rem_len <= 0) {
		return;
	}

	memset(rem_buf, 0, rem_len);

	const uint16_t signature = sys_cpu_to_le16(BOOT_SECTOR_SIGNATURE);

	if ((offset <= BOOT_SECTOR_SIGNATURE_OFFSET) &&
	    (offset + len >= BOOT_SECTOR_SIGNATURE_OFFSET + sizeof(signature))) {
		memcpy(buf + BOOT_SECTOR_SIGNATURE_OFFSET - offset, &signature, sizeof(signature));
	}
}

static const struct virtual_file *get_file_from_cluster(struct virtual_fat_ctx *ctx,
							const struct virtual_file *files,
							size_t virtual_files_len, uint16_t cluster,
							uint16_t *start_cluster)
{
	uint16_t cluster_start = FAT_RESERVED_CLUSTERS;

	for (size_t i = 0; i < virtual_files_len; ++i) {
		uint16_t cluster_end =
			cluster_start +
			DIV_ROUND_UP(files[i].file_size, FAT_BYTES_PER_CLUSTER(ctx->sector_size));
		if (cluster >= cluster_start && cluster < cluster_end) {
			*start_cluster = cluster_start;
			return &files[i];
		}

		cluster_start = cluster_end;
	}

	return NULL;
}

static inline size_t fat12_index_to_cluster(uint16_t index)
{
	return DIV_ROUND_UP(index * 2, 3);
}

static void fat12_write_entry(uint8_t *buf, uint16_t cluster, uint16_t entry)
{
	if (cluster & 0x1) {
		/*On odd clusters, use high nibble + next byte*/
		buf[0] = (buf[0] & 0x0f) | ((entry & 0x00f) << 4);
		buf[1] = (entry >> 4) & 0xff;
	} else {
		/*On even clusters, use the byte and the next low nibble*/
		buf[0] = entry & 0xff;
		buf[1] = (buf[1] & 0xf0) | ((entry & 0xf00) >> 8);
	}
}

static void write_fat_entries_fat12(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len,
				    uint32_t offset, const struct virtual_file *files,
				    size_t files_len)
{
	uint16_t start_cluster = fat12_index_to_cluster(offset);
	const uint16_t end_cluster = fat12_index_to_cluster(offset + len);
	size_t index = 0;
	const struct virtual_file *file = files;
	uint16_t cluster_file_end = FAT_RESERVED_CLUSTERS - 1;

	memset(buf, 0, len);

	if (offset == 0) {
		/* RESERVED fat entries */
		buf[0] = ctx->boot_sector->BPB_Media;
		buf[1] = 0xff;
		buf[2] = 0xff;

		index += 3;
	}

	for (uint16_t cluster = start_cluster < FAT_RESERVED_CLUSTERS ? FAT_RESERVED_CLUSTERS
								      : start_cluster;
	     cluster < end_cluster; ++cluster) {
		while (cluster_file_end < cluster) {
			if (files_len-- == 0) {
				return;
			}
			cluster_file_end += DIV_ROUND_UP(file->file_size,
							 FAT_BYTES_PER_CLUSTER(ctx->sector_size));
			++file;
		}

		const uint16_t entry = (cluster == cluster_file_end) ? 0xfff : (cluster + 1);

		fat12_write_entry(buf + index, cluster, entry);
		index += cluster & 0x1 ? 2 : 1;
	}
}

static inline size_t fat16_index_to_cluster(uint16_t index)
{
	return index / 2;
}

static void fat16_write_entry(uint8_t *buf, uint16_t entry)
{
	*((uint16_t *)buf) = sys_cpu_to_le16(entry);
}

static void write_fat_entries_fat16(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len,
				    uint32_t offset, const struct virtual_file *files,
				    size_t files_len)
{
	uint16_t start_cluster = fat16_index_to_cluster(offset);
	const uint16_t end_cluster = fat16_index_to_cluster(offset + len);
	size_t index = 0;
	const struct virtual_file *file = files;
	uint16_t cluster_file_end = FAT_RESERVED_CLUSTERS - 1;

	memset(buf, 0, len);

	if (offset == 0) {
		/* RESERVED fat entries */
		buf[0] = ctx->boot_sector->BPB_Media;
		buf[1] = 0xff;
		buf[2] = 0xff;
		buf[3] = 0xff;

		index += 4;
	}

	for (uint16_t cluster = start_cluster < FAT_RESERVED_CLUSTERS ? FAT_RESERVED_CLUSTERS
								      : start_cluster;
	     cluster < end_cluster; ++cluster) {
		while (cluster_file_end < cluster) {
			if (files_len-- == 0) {
				return;
			}
			cluster_file_end += DIV_ROUND_UP(file->file_size,
							 FAT_BYTES_PER_CLUSTER(ctx->sector_size));
			++file;
		}

		const uint16_t entry = (cluster == cluster_file_end) ? 0xffff : (cluster + 1);

		fat16_write_entry(buf + index, entry);
		index += 2;
	}
}

static void transfer_fat(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len, uint32_t offset)
{
	const struct virtual_file *files = ctx->virtual_files;
	const size_t files_len = ctx->files_len;

	ctx->fat_type == FAT12 ? write_fat_entries_fat12(ctx, buf, len, offset, files, files_len)
			       : write_fat_entries_fat16(ctx, buf, len, offset, files, files_len);
}

static void insert_virtual_file(const struct virtual_file *file, uint8_t *buf, uint16_t cluster)
{
	struct fat1x_dir *dir = (struct fat1x_dir *)buf;
	const size_t name_len = strlen(file->name);

	memset(dir->DIR_Name, ' ', sizeof(dir->DIR_Name));
	memcpy(dir->DIR_Name, file->name, MIN(name_len, sizeof(dir->DIR_Name) - sizeof(file->ext)));

	if (file->ext[0] != 0) {
		memcpy(dir->DIR_Name + sizeof(dir->DIR_Name) - sizeof(file->ext), file->ext,
		       sizeof(file->ext));
	}

	dir->DIR_Attr = file->attributes;
	dir->DIR_NTRes = 0x00;
	dir->DIR_CrtTimeTenth = 0x00;
	dir->DIR_CrtTime = 0x00;
	dir->DIR_CrtDate = 0x00;
	dir->DIR_LstAccDate = 0x00;
	dir->DIR_FstClusHI = 0x0000;
	dir->DIR_WrtTime = 0x00;
	dir->DIR_WrtDate_day = CONFIG_VIRTUAL_FAT_DATE_DAY;
	dir->DIR_WrtDate_month = CONFIG_VIRTUAL_FAT_DATE_MONTH;
	dir->DIR_WrtDate_year = CONFIG_VIRTUAL_FAT_DATE_YEAR - 1980;
	dir->DIR_FstClusLO = sys_cpu_to_le16(cluster);
	dir->DIR_FileSize = sys_cpu_to_le32(file->file_size);
}

static void insert_vol_label(struct virtual_fat_ctx *ctx, uint8_t *buf)
{
	struct virtual_file file = {.name = ctx->volume_label,
				    .ext = {0},
				    .file_size = 0,
				    .attributes = FAT_ATTR_VOLUME_ID};

	if (strlen(ctx->volume_label) > FAT12_FILENAME_LEN) {
		strncpy(file.ext, &ctx->volume_label[8], sizeof(file.ext));
	}

	insert_virtual_file(&file, buf, 0);
}

static uint16_t cluster_from_file(struct virtual_fat_ctx *ctx, const struct virtual_file *files,
				  size_t index)
{
	uint16_t cluster = FAT_RESERVED_CLUSTERS;

	for (int i = 0; i < index; ++i) {
		cluster +=
			DIV_ROUND_UP(files[i].file_size, FAT_BYTES_PER_CLUSTER(ctx->sector_size));
	}

	return cluster;
}

static void transfer_files(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len, uint32_t offset)
{
	/* reads are always at a boundary of a directory/file - 1 for the roo directory*/
	const size_t start_index = (offset / sizeof(struct fat1x_dir)) - 1;
	const struct virtual_file *files = ctx->virtual_files;
	const size_t files_len = ctx->files_len;
	size_t remaining = len;

	for (size_t i = start_index; remaining >= sizeof(struct fat1x_dir) && i < files_len;
	     remaining -= sizeof(struct fat1x_dir), ++i) {
		insert_virtual_file(&files[i], buf, cluster_from_file(ctx, files, i));
		buf += sizeof(struct fat1x_dir);
	}

	memset(buf, 0, remaining);
}

static void transfer_root_dir(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len,
			      uint32_t offset)
{
	const size_t root_dir_size = (ctx->files_len + 1) * sizeof(struct fat1x_dir);

	__ASSERT_NO_MSG(offset % sizeof(struct fat1x_dir) == 0);

	if (offset >= root_dir_size) {
		memset(buf, 0, len);
		return;
	}

	if (offset == 0) {
		insert_vol_label(ctx, buf);
		buf += sizeof(struct fat1x_dir);
		offset += sizeof(struct fat1x_dir);
		len -= sizeof(struct fat1x_dir);
	}

	transfer_files(ctx, buf, len, offset);
}

static void transfer_data(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len, uint32_t lba,
			  uint32_t offset)
{
	const uint16_t cluster = get_cluster_nr_from_lba(ctx, lba);
	const struct virtual_file *files = ctx->virtual_files;
	const size_t files_len = ctx->files_len;
	uint16_t file_start_cluster;
	size_t data_written;
	size_t file_offset;

	const struct virtual_file *file =
		get_file_from_cluster(ctx, files, files_len, cluster, &file_start_cluster);

	if (file == NULL || file->get_chunk == NULL) {
		memset(buf, 0, len);
		return;
	}

	file_offset = (cluster - file_start_cluster) * FAT_BYTES_PER_CLUSTER(ctx->sector_size) +
		      ((lba - get_data_start_lba(ctx)) % CONFIG_FAT_SECTORS_PER_CLUSTER) *
			      ctx->sector_size +
		      offset;

	data_written = file->get_chunk(file_offset, buf, len, file->user_data);

	if (data_written < len) {
		memset(buf + data_written, 0, len - data_written);
	}
}

int virtual_fat_read(struct virtual_fat_ctx *ctx, uint8_t *buf, size_t len, uint32_t lba,
		     uint32_t offset)
{
	/* Host reads Boot Sector */
	if (lba < FAT_START_LBA) {
		transfer_boot_sector(ctx, buf, len, offset);
		return 0;
	} else if (lba < get_fat_end_lba(ctx)) {
		transfer_fat(ctx, buf, len,
			     get_fat_offset(ctx, lba, offset) %
				     (ctx->sector_size *
				      FAT_NUM_SECTORS_PER_FAT(ctx->sector_size,
							      ctx->data_partition_size)));
		return 0;
	} else if (lba < get_root_dir_end_lba(ctx)) {
		transfer_root_dir(ctx, buf, len, get_root_dir_offset(ctx, lba, offset));
		return 0;
	} else if (lba < get_data_end_lba(ctx)) {
		transfer_data(ctx, buf, len, lba, offset);
		return 0;
	}

	LOG_ERR("Host tries to read beyond the last sector");
	return -EINVAL;
}

static inline void rootdir_writes(const uint8_t *buf, size_t len, uint32_t offset)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	LOG_DBG("Host writes root directory offset %d", offset);
}

static inline void write_boot(const uint8_t *buf, size_t len, uint32_t offset)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	LOG_DBG("Host writes bootsector offset %d", offset);
}

static inline void write_fat(const uint8_t *buf, size_t len, uint32_t offset)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	LOG_DBG("Host writes fat %d", offset);
}

int virtual_fat_write(struct virtual_fat_ctx *ctx, const uint8_t *buf, size_t len, uint32_t lba,
		      uint32_t offset)
{
	if (lba < FAT_START_LBA) {
		write_boot(buf, len, offset);
		return 0;
	}

	if (lba < get_fat_end_lba(ctx)) {
		write_fat(buf, len, get_fat_offset(ctx, lba, offset));
		return 0;
	}

	if (lba < get_root_dir_end_lba(ctx)) {
		rootdir_writes(buf, len, get_root_dir_offset(ctx, lba, offset));
		return 0;
	}

	if (lba < ctx->last_file_offs) {
		return 0;
	}

	if (lba < get_data_end_lba(ctx)) {
		if (ctx->new_file_write_cb) {
			ctx->new_file_write_cb((lba - ctx->last_file_offs) * ctx->sector_size +
						       offset,
					       buf, len, ctx->new_file_write_user_data);
		}

		return 0;
	}

	LOG_ERR("Host tries to write over the last sector");
	return -EINVAL;
}
