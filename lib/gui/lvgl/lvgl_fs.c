/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <zephyr.h>
#include <fs.h>

/* Stub for LVGL ufs (ram based FS) init function
 * as we use Zephyr FS API instead
 */
void lv_ufs_init(void)
{
}

bool lvgl_fs_ready(void)
{
	return true;
}

static lv_fs_res_t errno_to_lv_fs_res(int err)
{
	switch (err) {
	case 0:
		return LV_FS_RES_OK;
	case -EIO:
		/*Low level hardware error*/
		return LV_FS_RES_HW_ERR;
	case -EBADF:
		/*Error in the file system structure */
		return LV_FS_RES_FS_ERR;
	case -ENOENT:
		/*Driver, file or directory is not exists*/
		return LV_FS_RES_NOT_EX;
	case -EFBIG:
		/*Disk full*/
		return LV_FS_RES_FULL;
	case -EACCES:
		/*Access denied. Check 'fs_open' modes and write protect*/
		return LV_FS_RES_DENIED;
	case -EBUSY:
		/*The file system now can't handle it, try later*/
		return LV_FS_RES_BUSY;
	case -ENOMEM:
		/*Not enough memory for an internal operation*/
		return LV_FS_RES_OUT_OF_MEM;
	case -EINVAL:
		/*Invalid parameter among arguments*/
		return LV_FS_RES_INV_PARAM;
	default:
		return LV_FS_RES_UNKNOWN;
	}
}

static lv_fs_res_t lvgl_fs_open(void *file, const char *path,
		lv_fs_mode_t mode)
{
	int err;

	err = fs_open((struct fs_file_t *)file, path);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_close(void *file)
{
	int err;

	err = fs_close((struct fs_file_t *)file);
	return LV_FS_RES_NOT_IMP;
}

static lv_fs_res_t lvgl_fs_remove(const char *path)
{
	int err;

	err = fs_unlink(path);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_read(void *file, void *buf, u32_t btr,
		u32_t *br)
{
	int err;

	err = fs_read((struct fs_file_t *)file, buf, btr);
	if (err > 0) {
		if (br != NULL) {
			*br = err;
		}
		err = 0;
	} else if (br != NULL) {
		*br = 0;
	}
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_write(void *file, const void *buf, u32_t btw,
		u32_t *bw)
{
	int err;

	err = fs_write((struct fs_file_t *)file, buf, btw);
	if (err == btw) {
		if (bw != NULL) {
			*bw = btw;
		}
		err = 0;
	} else if (err < 0) {
		if (bw != NULL) {
			*bw = 0;
		}
	} else {
		if (bw != NULL) {
			*bw = err;
		}
		err = -EFBIG;
	}
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_seek(void *file, u32_t pos)
{
	int err;

	err = fs_seek((struct fs_file_t *)file, pos, FS_SEEK_SET);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_tell(void *file, u32_t *pos_p)
{
	*pos_p = fs_tell((struct fs_file_t *)file);
	return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_fs_trunc(void *file)
{
	int err;
	off_t length;

	length = fs_tell((struct fs_file_t *) file);
	++length;
	err = fs_truncate((struct fs_file_t *)file, length);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_size(void *file, u32_t *fsize)
{
	int err;
	off_t org_pos;

	/* LVGL does not provided path but pointer to file struct as such
	 * we can not use fs_stat, instead use a combination of fs_tell and
	 * fs_seek to get the files size.
	 */

	org_pos = fs_tell((struct fs_file_t *) file);

	err = fs_seek((struct fs_file_t *) file, 0, FS_SEEK_END);
	if (err != 0) {
		*fsize = 0;
		return errno_to_lv_fs_res(err);
	}

	*fsize = fs_tell((struct fs_file_t *) file) + 1;

	err = fs_seek((struct fs_file_t *) file, org_pos, FS_SEEK_SET);

	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_rename(const char *from, const char *to)
{
	int err;

	err = fs_rename(from, to);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_free(u32_t *total_p, u32_t *free_p)
{
	/* We have no easy way of telling the total file system size.
	 * Zephyr can only return this information per mount point.
	 */
	return LV_FS_RES_NOT_IMP;
}

static lv_fs_res_t lvgl_fs_dir_open(void *dir, const char *path)
{
	int err;

	err = fs_opendir((struct fs_dir_t *)dir, path);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_dir_read(void *dir, char *fn)
{
	/* LVGL expects a string as return parameter but the format of the
	 * string is not documented.
	 */
	return LV_FS_RES_NOT_IMP;
}

static lv_fs_res_t lvgl_fs_dir_close(void *dir)
{
	int err;

	err = fs_closedir((struct fs_dir_t *)dir);
	return errno_to_lv_fs_res(err);
}


void lvgl_fs_init(void)
{
	lv_fs_drv_t fs_drv;

	memset(&fs_drv, 0, sizeof(lv_fs_drv_t));

	fs_drv.file_size = sizeof(struct fs_file_t);
	fs_drv.rddir_size = sizeof(struct fs_dir_t);
	fs_drv.letter = '/';
	fs_drv.ready = lvgl_fs_ready;

	fs_drv.open = lvgl_fs_open;
	fs_drv.close = lvgl_fs_close;
	fs_drv.remove = lvgl_fs_remove;
	fs_drv.read = lvgl_fs_read;
	fs_drv.write = lvgl_fs_write;
	fs_drv.seek = lvgl_fs_seek;
	fs_drv.tell = lvgl_fs_tell;
	fs_drv.trunc = lvgl_fs_trunc;
	fs_drv.size = lvgl_fs_size;
	fs_drv.rename = lvgl_fs_rename;
	fs_drv.free = lvgl_fs_free;

	fs_drv.dir_open = lvgl_fs_dir_open;
	fs_drv.dir_read = lvgl_fs_dir_read;
	fs_drv.dir_close = lvgl_fs_dir_close;

	lv_fs_add_drv(&fs_drv);
}

