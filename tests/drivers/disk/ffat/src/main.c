/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ff.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/storage/ffatdisk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ffat_test, LOG_LEVEL_INF);

const char txt_test_file[] =
	"Zephyr RTOS\n"
	"Board " CONFIG_BOARD "\n"
	"Arch "CONFIG_ARCH "\n";

struct binblock {
	uint32_t s_tag;
	uint32_t b_num;
	uint8_t reserved[500];
	uint32_t e_tag;
} __packed;

BUILD_ASSERT(sizeof(struct binblock) == 512U);

static struct binblock last_bb;
static uint32_t wr_cb_anchors;
static uint32_t wr_cb_flushes;

/* Boot sector fields, offsets fixed by the FAT specification */
#define BS_RSVD_SEC_CNT		14U
#define BS_FAT_SZ16		22U
#define BS_NTRES		37U
#define BS_NTRES32		65U

#define FAT_STATE_DIRTY		BIT(0)
#define FAT16_CLN_SHUT		BIT(15)
#define FAT32_CLN_SHUT		BIT(27)

#define BF0_SIZE	(512UL * 32408UL)
#define BF1_SIZE	(2048UL * 65406UL)
#define BF2_SIZE	(8192UL * 65502UL)
#define BF3_SIZE	(131072UL * 32765UL)

#define BF4_SIZE	(512UL * 129004UL)
#define BF5_SIZE	(2048UL * 130553UL)
#define BF6_SIZE	(8192UL * 130939UL)
#define BF7_SIZE	UINT32_MAX

static int textfile_rd_cb(struct ffat_file *const f, const uint32_t block,
			  uint8_t *const buf, const uint32_t size)
{
	const uint32_t f_off = size * block;

	if (f->size > f_off) {
		uint32_t len = MIN(f->size - f_off, size);

		memcpy(buf, (uint8_t *)f->priv + f_off, len);
		LOG_DBG("Read %u bytes, block %u file offset %u, f->size %u",
			len, block, f_off, f->size);
	} else {
		LOG_INF("Offset (%u) is outside of file range (%u)", f_off, f->size);
	}

	return 0;
}

static int binfile_rd_cb(struct ffat_file *const f, const uint32_t block,
			 uint8_t *const buf, const uint32_t size)
{
	const uint32_t f_off = size * block;

	if (f->size > f_off) {
		uint32_t len = MIN(f->size - f_off, size);
		struct binblock *bb = (void *)buf;

		bb->s_tag = sys_cpu_to_le32(0xDECAFBAD);
		bb->b_num = sys_cpu_to_le32(block);
		bb->e_tag = sys_cpu_to_le32(0xDEADDA7A);

		LOG_DBG("Read %u bytes, block %u file offset %u, f->size %u",
			len, block, f_off, f->size);
	} else {
		LOG_INF("Offset (%u) is outside of file range (%u)", f_off, f->size);
	}

	return 0;
}

static int binfile_wr_cb(struct ffat_file *const f, const uint32_t block,
			 const uint8_t *const buf, const uint32_t size)
{
	const struct binblock *bb = (const void *)buf;
	const uint32_t f_off = size * block;

	if (buf == NULL) {
		LOG_INF("Host flushed the volume");
		wr_cb_flushes++;

		return 0;
	}

	if (sys_le32_to_cpu(bb->s_tag) != 0xDEADDA7A ||
	    sys_le32_to_cpu(bb->e_tag) != 0xDECAFBAD) {
		/* Not our payload */
		return -ENODATA;
	}

	if (sys_le32_to_cpu(bb->b_num) == 0U) {
		/* Start of a payload */
		memcpy(&last_bb, buf, MIN(size, sizeof(last_bb)));
		wr_cb_anchors++;

		return 1;
	}

	if (sys_le32_to_cpu(bb->b_num) != block) {
		LOG_ERR("Block index mismatch: block %u, payload block %u",
			block, sys_le32_to_cpu(bb->b_num));

		return -EINVAL;
	}

	if (f->size > f_off) {
		uint32_t len = MIN(f->size - f_off, size);

		memcpy(&last_bb, buf, MIN(len, sizeof(last_bb)));

		LOG_DBG("Write %u bytes, block %u file offset %u, f->size %u",
			len, block, f_off, f->size);
	} else {
		LOG_ERR("Offset (%u) is outside of file range (%u)", f_off, f->size);
	}

	return 0;
}

FFAT_FILE_DEFINE(test1, "RAM", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test2, "RAM", "TEST_002TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test3, "RAM", "TEST_003TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test4, "RAM", "TEST_004TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test5, "RAM", "TEST_005TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test6, "RAM", "TEST_006TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test7, "RAM", "TEST_007TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test8, "RAM", "TEST_008TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test9, "RAM", "TEST_009TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test10, "RAM", "TEST_010TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test11, "RAM", "TEST_011TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test12, "RAM", "TEST_012TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test13, "RAM", "TEST_013TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test14, "RAM", "TEST_014TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);
FFAT_FILE_DEFINE(test15, "RAM", "TEST_000BIN", BF0_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);

FFAT_FILE_DEFINE(test16, "NAND", "TEST_001BIN", BF1_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);
FFAT_FILE_DEFINE(test17, "NAND", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);

FFAT_FILE_DEFINE(test18, "CF", "TEST_002BIN", BF2_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);
FFAT_FILE_DEFINE(test19, "CF", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);

FFAT_FILE_DEFINE(test20, "SD", "TEST_003BIN", BF3_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);
FFAT_FILE_DEFINE(test21, "SD", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);

FFAT_FILE_DEFINE(test22, "SD2", "TEST_004BIN", BF4_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);
FFAT_FILE_DEFINE(test23, "SD2", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);

FFAT_FILE_DEFINE(test24, "USB", "TEST_005BIN", BF5_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);
FFAT_FILE_DEFINE(test25, "USB", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);

FFAT_FILE_DEFINE(test26, "USB2", "TEST_006BIN", BF6_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);
FFAT_FILE_DEFINE(test27, "USB2", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);

FFAT_FILE_DEFINE(test28, "USB3", "TEST_007BIN", BF7_SIZE,
		 binfile_rd_cb, binfile_wr_cb, NULL);
FFAT_FILE_DEFINE(test29, "USB3", "TEST_001TXT", sizeof(txt_test_file),
		 textfile_rd_cb, NULL, txt_test_file);

static union {
	struct binblock bb;
	uint8_t a[4096];
} ffat_test_buf;

static FATFS fatfs[8];

static struct fs_mount_t mnt0 = {
	.type = FS_FATFS,
	.mnt_point = "/RAM:",
	.fs_data = &fatfs[0],
};

static struct fs_mount_t mnt1 = {
	.type = FS_FATFS,
	.mnt_point = "/NAND:",
	.fs_data = &fatfs[1],
};

static struct fs_mount_t mnt2 = {
	.type = FS_FATFS,
	.mnt_point = "/CF:",
	.fs_data = &fatfs[2],
};

static struct fs_mount_t mnt3 = {
	.type = FS_FATFS,
	.mnt_point = "/SD:",
	.fs_data = &fatfs[3],
};

static struct fs_mount_t mnt4 = {
	.type = FS_FATFS,
	.mnt_point = "/SD2:",
	.fs_data = &fatfs[4],
};

static struct fs_mount_t mnt5 = {
	.type = FS_FATFS,
	.mnt_point = "/USB:",
	.fs_data = &fatfs[5],
};

static struct fs_mount_t mnt6 = {
	.type = FS_FATFS,
	.mnt_point = "/USB2:",
	.fs_data = &fatfs[6],
};

static struct fs_mount_t mnt7 = {
	.type = FS_FATFS,
	.mnt_point = "/USB3:",
	.fs_data = &fatfs[7],
};

struct ffat_file_info {
	const char *path;
	fs_mode_t flags;
	size_t size;
};

const struct ffat_file_info file_path0[] = {
	{.path = "/RAM:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_002.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_003.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_004.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_005.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_006.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_007.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_008.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_009.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_010.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_011.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_012.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_013.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_014.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/RAM:/TEST_000.BIN", .flags = FS_O_RDWR, .size = BF0_SIZE,},
};

const struct ffat_file_info file_path1[] = {
	{.path = "/NAND:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/NAND:/TEST_001.BIN", .flags = FS_O_RDWR, .size = BF1_SIZE},
};

const struct ffat_file_info file_path2[] = {
	{.path = "/CF:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/CF:/TEST_002.BIN", .flags = FS_O_RDWR, .size = BF2_SIZE},
};

const struct ffat_file_info file_path3[] = {
	{.path = "/SD:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/SD:/TEST_003.BIN", .flags = FS_O_RDWR, .size = BF3_SIZE},
};

const struct ffat_file_info file_path4[] = {
	{.path = "/SD2:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/SD2:/TEST_004.BIN", .flags = FS_O_RDWR, .size = BF4_SIZE},
};

const struct ffat_file_info file_path5[] = {
	{.path = "/USB:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/USB:/TEST_005.BIN", .flags = FS_O_RDWR, .size = BF5_SIZE},
};

const struct ffat_file_info file_path6[] = {
	{.path = "/USB2:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/USB2:/TEST_006.BIN", .flags = FS_O_RDWR, .size = BF6_SIZE},
};

const struct ffat_file_info file_path7[] = {
	{.path = "/USB3:/TEST_001.TXT", .flags = FS_O_READ, .size = 0,},
	{.path = "/USB3:/TEST_007.BIN", .flags = FS_O_RDWR, .size = BF7_SIZE},
};

static int test_mount_ffat_disk(struct fs_mount_t *const mnt)
{
	int err = 0;

	mnt->flags = FS_MOUNT_FLAG_NO_FORMAT;
	err = fs_mount(mnt);

	if (err < 0) {
		TC_PRINT("Failed to open ffat disk %s (%d)\n",
			 mnt->mnt_point, err);
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_umount_ffat_disk(struct fs_mount_t *const mnt)
{
	int err = 0;

	err = fs_unmount(mnt);

	if (err < 0) {
		TC_PRINT("Failed to unmount ffat disk %s (%d)\n",
			 mnt->mnt_point, err);
		return TC_FAIL;
	}

	return TC_PASS;
}

static int ffat_test_open(struct fs_file_t *const f,
			  const struct ffat_file_info *const finfo)
{
	struct fs_dirent entry;
	int err;

	err = fs_stat(finfo->path, &entry);
	if (err) {
		return TC_FAIL;
	}

	err = fs_open(f, finfo->path, finfo->flags);
	if (err) {
		TC_PRINT("Failed to open file %s (%d)\n", finfo->path, err);
		return err;
	}

	return err;
}

static int test_file_rw_bin(struct fs_file_t *const f,
			    const size_t fsize, const size_t bsize)
{
	const uint32_t blocks  = fsize / bsize;
	ssize_t len;
	int err;

	err = fs_seek(f, 0, FS_SEEK_SET);
	if (err) {
		TC_PRINT("Failed to reset file position (%d)\n", err);
		return err;
	}

	TC_PRINT("File size: %zu, block size: %zu, blocks: %u\n", fsize, bsize, blocks);

	for (uint32_t n = 0; n < blocks; n++) {
		len = fs_read(f, ffat_test_buf.a, MIN(bsize, sizeof(ffat_test_buf)));
		if (len != bsize) {
			TC_PRINT("Failed to read file block %u (%zd)\n", n, len);
			return len;
		}

		/* Verify that the file block was read successfully */
		if (sys_le32_to_cpu(ffat_test_buf.bb.s_tag) != 0xDECAFBAD ||
		    sys_le32_to_cpu(ffat_test_buf.bb.b_num) != n ||
		    sys_le32_to_cpu(ffat_test_buf.bb.e_tag) != 0xDEADDA7A) {
			TC_PRINT("r %u: s_tag %u b_num %u e_tag %u\n",
				 n,
				 sys_le32_to_cpu(ffat_test_buf.bb.s_tag),
				 sys_le32_to_cpu(ffat_test_buf.bb.b_num),
				 sys_le32_to_cpu(ffat_test_buf.bb.e_tag));
			return TC_FAIL;
		}
	}

	err = fs_seek(f, 0, FS_SEEK_SET);
	if (err) {
		TC_PRINT("Reset file position failed (%d)\n", err);
		return err;
	}

	for (uint32_t n = 0; n < blocks; n++) {
		ffat_test_buf.bb.s_tag = sys_cpu_to_le32(0xDEADDA7A);
		ffat_test_buf.bb.b_num = sys_cpu_to_le32(n);
		ffat_test_buf.bb.e_tag = sys_cpu_to_le32(0xDECAFBAD);

		len = fs_write(f, ffat_test_buf.a, MIN(bsize, sizeof(ffat_test_buf)));
		if (len != bsize) {
			TC_PRINT("Failed to write file block %u (%zd)\n", n, len);
			return len;
		}

		/* Verify that writing to the file block was successful */
		if (sys_le32_to_cpu(last_bb.s_tag) != 0xDEADDA7A ||
		    sys_le32_to_cpu(last_bb.b_num) != n ||
		    sys_le32_to_cpu(last_bb.e_tag) != 0xDECAFBAD) {
			TC_PRINT("w %u: s_tag %u b_num %u e_tag %u\n",
				 n,
				 sys_le32_to_cpu(last_bb.s_tag),
				 sys_le32_to_cpu(last_bb.b_num),
				 sys_le32_to_cpu(last_bb.e_tag));
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

static int test_file_r_txt(struct fs_file_t *const f)
{
	ssize_t len;
	int err;

	err = fs_seek(f, 0, FS_SEEK_SET);
	if (err) {
		TC_PRINT("Reset file position failed (%d)\n", err);
		return err;
	}

	memset(ffat_test_buf.a, 0, sizeof(ffat_test_buf));

	len = fs_read(f, ffat_test_buf.a, sizeof(txt_test_file));
	if (len < 0) {
		TC_PRINT("Failed to read txt file (%zd)\n", len);
		return len;
	}

	if (memcmp(txt_test_file, ffat_test_buf.a, sizeof(txt_test_file))) {
		TC_PRINT("The read file differs from the original %s\n", ffat_test_buf.a);
		return TC_FAIL;
	}

	return TC_PASS;
}

/*
 * A file created by the host is placed in clusters that no FFAT file owns.
 * It requires a free root directory entry and free clusters on the disk.
 * The test is for the path where the backend anchors the payload itself.
 */
static int test_file_w_stream(struct fs_mount_t *const mnt, const char *const path)
{
	struct fs_file_t filep;
	struct fs_statvfs stat;
	size_t bsize;
	ssize_t len;
	int err;

	err = test_mount_ffat_disk(mnt);
	if (err) {
		return err;
	}

	err = fs_statvfs(mnt->mnt_point, &stat);
	if (err) {
		TC_PRINT("Failed to retrieve vfs statistics (%d)\n", err);
		return err;
	}

	bsize = stat.f_bsize;
	wr_cb_anchors = 0;
	memset(&filep, 0, sizeof(filep));

	err = fs_open(&filep, path, FS_O_CREATE | FS_O_WRITE);
	if (err) {
		TC_PRINT("Failed to create file %s (%d)\n", path, err);
		return err;
	}

	for (uint32_t n = 0; n < 4U; n++) {
		memset(ffat_test_buf.a, 0, sizeof(ffat_test_buf));
		ffat_test_buf.bb.s_tag = sys_cpu_to_le32(0xDEADDA7A);
		ffat_test_buf.bb.b_num = sys_cpu_to_le32(n);
		ffat_test_buf.bb.e_tag = sys_cpu_to_le32(0xDECAFBAD);

		len = fs_write(&filep, ffat_test_buf.a, MIN(bsize, sizeof(ffat_test_buf)));
		if (len != bsize) {
			TC_PRINT("Failed to write block %u (%zd)\n", n, len);
			fs_close(&filep);
			return TC_FAIL;
		}
	}

	fs_close(&filep);

	/* Only the payload start is anchored */
	if (wr_cb_anchors != 1U) {
		TC_PRINT("Payload anchored %u times\n", wr_cb_anchors);
		return TC_FAIL;
	}

	err = test_umount_ffat_disk(mnt);
	if (err) {
		return err;
	}

	return TC_PASS;
}

/* A block without the payload framing must not fail the host write */
static int test_file_w_unframed(struct fs_mount_t *const mnt, const char *const path)
{
	struct fs_file_t filep;
	struct fs_statvfs stat;
	size_t bsize;
	ssize_t len;
	int err;

	err = test_mount_ffat_disk(mnt);
	if (err) {
		return err;
	}

	err = fs_statvfs(mnt->mnt_point, &stat);
	if (err) {
		TC_PRINT("Failed to retrieve vfs statistics (%d)\n", err);
		return err;
	}

	bsize = stat.f_bsize;
	wr_cb_anchors = 0;
	memset(&last_bb, 0, sizeof(last_bb));
	memset(ffat_test_buf.a, 0xA5, sizeof(ffat_test_buf));
	memset(&filep, 0, sizeof(filep));

	err = fs_open(&filep, path, FS_O_CREATE | FS_O_WRITE);
	if (err) {
		TC_PRINT("Failed to open file %s (%d)\n", path, err);
		return err;
	}

	len = fs_write(&filep, ffat_test_buf.a, MIN(bsize, sizeof(ffat_test_buf)));
	fs_close(&filep);

	if (len != bsize) {
		TC_PRINT("Failed to write unframed block (%zd)\n", len);
		return TC_FAIL;
	}

	if (wr_cb_anchors != 0U) {
		TC_PRINT("Unframed block anchored a payload\n");
		return TC_FAIL;
	}

	if (last_bb.s_tag != 0U) {
		TC_PRINT("Unframed block was consumed\n");
		return TC_FAIL;
	}

	err = test_umount_ffat_disk(mnt);
	if (err) {
		return err;
	}

	return TC_PASS;
}

static bool ffat_bs_is_fat32(const uint8_t *const bs)
{
	return sys_get_le16(&bs[BS_FAT_SZ16]) == 0U;
}

/*
 * ELF FAT does not allow the dirty flag to be set. Test it directly using
 * disk_access_write().
 */
static int test_dirty_flag_bs(const char *const disk)
{
	const uint32_t flushes = wr_cb_flushes;
	uint8_t *const bs = ffat_test_buf.a;
	uint32_t off;
	int err;

	err = disk_access_init(disk);
	if (err) {
		TC_PRINT("Failed to init disk %s (%d)\n", disk, err);
		return TC_FAIL;
	}

	err = disk_access_read(disk, bs, 0U, 1U);
	if (err) {
		TC_PRINT("Failed to read boot sector (%d)\n", err);
		return TC_FAIL;
	}

	off = ffat_bs_is_fat32(bs) ? BS_NTRES32 : BS_NTRES;

	/* Mark dirty first */
	bs[off] |= FAT_STATE_DIRTY;
	err = disk_access_write(disk, bs, 0U, 1U);
	if (err) {
		TC_PRINT("Failed to write boot sector (%d)\n", err);
		return TC_FAIL;
	}

	if (wr_cb_flushes != flushes) {
		TC_PRINT("Setting the dirty flag must not notify the backend\n");
		return TC_FAIL;
	}

	bs[off] &= ~FAT_STATE_DIRTY;
	err = disk_access_write(disk, bs, 0U, 1U);
	if (err) {
		TC_PRINT("Failed to write boot sector (%d)\n", err);
		return TC_FAIL;
	}

	if (wr_cb_flushes != flushes + 1U) {
		TC_PRINT("Cleaning the dirty flag must notify the backend\n");
		return TC_FAIL;
	}

	return disk_access_ioctl(disk, DISK_IOCTL_CTRL_DEINIT, NULL);
}

/*
 * ELF FAT does not allow the clean bit to be set. Test it directly using
 * disk_access_write().
 */
static int test_dirty_flag_fat(const char *const disk)
{
	const uint32_t flushes = wr_cb_flushes;
	uint8_t *const buf = ffat_test_buf.a;
	uint32_t fat1_start;
	bool fat32;
	int err;

	err = disk_access_init(disk);
	if (err) {
		TC_PRINT("Failed to init disk %s (%d)\n", disk, err);
		return TC_FAIL;
	}

	err = disk_access_read(disk, buf, 0U, 1U);
	if (err) {
		TC_PRINT("Failed to read boot sector (%d)\n", err);
		return TC_FAIL;
	}

	fat32 = ffat_bs_is_fat32(buf);
	fat1_start = sys_get_le16(&buf[BS_RSVD_SEC_CNT]);

	err = disk_access_read(disk, buf, fat1_start, 1U);
	if (err) {
		TC_PRINT("Failed to read FAT sector 0 (%d)\n", err);
		return TC_FAIL;
	}

	/* FAT[1] follows FAT[0], clearing the clean bit marks the volume dirty */
	if (fat32) {
		sys_put_le32(sys_get_le32(&buf[4]) & ~FAT32_CLN_SHUT, &buf[4]);
	} else {
		sys_put_le16(sys_get_le16(&buf[2]) & ~FAT16_CLN_SHUT, &buf[2]);
	}

	err = disk_access_write(disk, buf, fat1_start, 1U);
	if (err) {
		TC_PRINT("Failed to write FAT sector 0 (%d)\n", err);
		return TC_FAIL;
	}

	if (wr_cb_flushes != flushes) {
		TC_PRINT("Clearing the clean bit must not notify the backend\n");
		return TC_FAIL;
	}

	if (fat32) {
		sys_put_le32(sys_get_le32(&buf[4]) | FAT32_CLN_SHUT, &buf[4]);
	} else {
		sys_put_le16(sys_get_le16(&buf[2]) | FAT16_CLN_SHUT, &buf[2]);
	}

	err = disk_access_write(disk, buf, fat1_start, 1U);
	if (err) {
		TC_PRINT("Failed to write FAT sector 0 (%d)\n", err);
		return TC_FAIL;
	}

	if (wr_cb_flushes != flushes + 1U) {
		TC_PRINT("Setting the clean bit must notify the backend\n");
		return TC_FAIL;
	}

	return disk_access_ioctl(disk, DISK_IOCTL_CTRL_DEINIT, NULL);
}

static int test_files_rw(struct fs_mount_t *const mnt,
			 const struct ffat_file_info *const finfo, const size_t size)
{
	struct fs_file_t filep;
	struct fs_statvfs stat;
	int err;

	err = test_mount_ffat_disk(mnt);
	if (err) {
		return err;
	}

	err = fs_statvfs(mnt->mnt_point, &stat);
	if (err) {
		TC_PRINT("Failed to retrieve vfs statistics (%d)\n", err);
		return err;
	}

	TC_PRINT("FS block size: %u (cluster size: %u), fs blocks: %u\n",
		 stat.f_bsize, stat.f_frsize, stat.f_blocks);

	for (int i = 0; i < size; i++) {
		memset(&filep, 0, sizeof(filep));

		err = ffat_test_open(&filep, &finfo[i]);
		if (err) {
			return err;
		}

		if (finfo[i].flags == FS_O_RDWR) {
			err = test_file_rw_bin(&filep, finfo[i].size, stat.f_bsize);
		} else {
			err = test_file_r_txt(&filep);
		}

		fs_close(&filep);
		if (err) {
			TC_PRINT("Failed on file %s\n", finfo[i].path);
			return err;
		}

		TC_PRINT("Test on %s passed\n", finfo[i].path);
	}

	err = test_umount_ffat_disk(mnt);
	if (err) {
		return err;
	}

	return TC_PASS;
}

ZTEST(ffat_test, test_fat16_a)
{
	zassert_true(test_files_rw(&mnt0, file_path0, ARRAY_SIZE(file_path0)) == TC_PASS);
}

ZTEST(ffat_test, test_fat16_b)
{
	zassert_true(test_files_rw(&mnt1, file_path1, ARRAY_SIZE(file_path1)) == TC_PASS);
}

ZTEST(ffat_test, test_fat16_c)
{
	zassert_true(test_files_rw(&mnt2, file_path2, ARRAY_SIZE(file_path2)) == TC_PASS);
}

ZTEST(ffat_test, test_fat16_d)
{
	zassert_true(test_files_rw(&mnt3, file_path3, ARRAY_SIZE(file_path3)) == TC_PASS);
}

ZTEST(ffat_test, test_fat32_a)
{
	zassert_true(test_files_rw(&mnt4, file_path4, ARRAY_SIZE(file_path4)) == TC_PASS);
}

ZTEST(ffat_test, test_fat32_b)
{
	zassert_true(test_files_rw(&mnt5, file_path5, ARRAY_SIZE(file_path5)) == TC_PASS);
}

ZTEST(ffat_test, test_fat32_c)
{
	zassert_true(test_files_rw(&mnt6, file_path6, ARRAY_SIZE(file_path6)) == TC_PASS);
}

ZTEST(ffat_test, test_fat32_d)
{
	zassert_true(test_files_rw(&mnt7, file_path7, ARRAY_SIZE(file_path7)) == TC_PASS);
}

ZTEST(ffat_test, test_stream_write)
{
	zassert_true(test_file_w_stream(&mnt7, "/USB3:/STREAM.BIN") == TC_PASS);
}

/* FFAT file owns the clusters, the block index is exact */
ZTEST(ffat_test, test_stream_write_unframed)
{
	zassert_true(test_file_w_unframed(&mnt7, "/USB3:/TEST_007.BIN") == TC_PASS);
}

/* No FFAT file owns the clusters (new file), the payload is not anchored */
ZTEST(ffat_test, test_stream_write_unframed_new)
{
	zassert_true(test_file_w_unframed(&mnt7, "/USB3:/UNFRAMED.BIN") == TC_PASS);
}

ZTEST(ffat_test, test_dirty_flag_bs_fat16)
{
	zassert_true(test_dirty_flag_bs("RAM") == TC_PASS);
}

ZTEST(ffat_test, test_dirty_flag_bs_fat32)
{
	zassert_true(test_dirty_flag_bs("USB3") == TC_PASS);
}

ZTEST(ffat_test, test_dirty_flag_fat_fat16)
{
	zassert_true(test_dirty_flag_fat("RAM") == TC_PASS);
}

ZTEST(ffat_test, test_dirty_flag_fat_fat32)
{
	zassert_true(test_dirty_flag_fat("USB3") == TC_PASS);
}

ZTEST_SUITE(ffat_test, NULL, NULL, NULL, NULL, NULL);
