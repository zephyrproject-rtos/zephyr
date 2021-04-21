/*
 * Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <zephyr.h>
#include <fs/fs.h>
#include "lvgl_fs.h"

static bool lvgl_fs_ready(struct _lv_fs_drv_t *drv)
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

static lv_fs_res_t lvgl_fs_open(struct _lv_fs_drv_t *drv, void *file,
		const char *path, lv_fs_mode_t mode)
{
	int err;
	int zmode = FS_O_CREATE;

	/* LVGL is passing absolute paths without the root slash add it back
	 * by decrementing the path pointer.
	 */
	path--;

	zmode |= (mode & LV_FS_MODE_WR) ? FS_O_WRITE : 0;
	zmode |= (mode & LV_FS_MODE_RD) ? FS_O_READ : 0;

	err = fs_open((struct fs_file_t *)file, path, zmode);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_close(struct _lv_fs_drv_t *drv, void *file)
{
	int err;

	err = fs_close((struct fs_file_t *)file);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_remove(struct _lv_fs_drv_t *drv, const char *path)
{
	int err;

	/* LVGL is passing absolute paths without the root slash add it back
	 * by decrementing the path pointer.
	 */
	path--;

	err = fs_unlink(path);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_read(struct _lv_fs_drv_t *drv, void *file,
		void *buf, uint32_t btr, uint32_t *br)
{
	int err;

	err = fs_read((struct fs_file_t *)file, buf, btr);
	if (err > 0) {
		if (br != NULL) {
			*br = err;
		}
		err = 0;
	} else if (br != NULL) {
		*br = 0U;
	}
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_write(struct _lv_fs_drv_t *drv, void *file,
		const void *buf, uint32_t btw, uint32_t *bw)
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
			*bw = 0U;
		}
	} else {
		if (bw != NULL) {
			*bw = err;
		}
		err = -EFBIG;
	}
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_seek(struct _lv_fs_drv_t *drv, void *file, uint32_t pos)
{
	int err;

	err = fs_seek((struct fs_file_t *)file, pos, FS_SEEK_SET);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_tell(struct _lv_fs_drv_t *drv, void *file,
		uint32_t *pos_p)
{
	*pos_p = fs_tell((struct fs_file_t *)file);
	return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_fs_trunc(struct _lv_fs_drv_t *drv, void *file)
{
	int err;
	off_t length;

	length = fs_tell((struct fs_file_t *) file);
	++length;
	err = fs_truncate((struct fs_file_t *)file, length);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_size(struct _lv_fs_drv_t *drv, void *file,
		uint32_t *fsize)
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
		*fsize = 0U;
		return errno_to_lv_fs_res(err);
	}

	*fsize = fs_tell((struct fs_file_t *) file) + 1;

	err = fs_seek((struct fs_file_t *) file, org_pos, FS_SEEK_SET);

	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_rename(struct _lv_fs_drv_t *drv, const char *from,
		const char *to)
{
	int err;

	/* LVGL is passing absolute paths without the root slash add it back
	 * by decrementing the path pointer.
	 */
	from--;
	to--;

	err = fs_rename(from, to);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_free(struct _lv_fs_drv_t *drv, uint32_t *total_p,
		uint32_t *free_p)
{
	/* We have no easy way of telling the total file system size.
	 * Zephyr can only return this information per mount point.
	 */
	return LV_FS_RES_NOT_IMP;
}

static lv_fs_res_t lvgl_fs_dir_open(struct _lv_fs_drv_t *drv, void *dir,
		const char *path)
{
	int err;

	/* LVGL is passing absolute paths without the root slash add it back
	 * by decrementing the path pointer.
	 */
	path--;

	fs_dir_t_init((struct fs_dir_t *)dir);
	err = fs_opendir((struct fs_dir_t *)dir, path);
	return errno_to_lv_fs_res(err);
}

static lv_fs_res_t lvgl_fs_dir_read(struct _lv_fs_drv_t *drv, void *dir,
		char *fn)
{
	/* LVGL expects a string as return parameter but the format of the
	 * string is not documented.
	 */
	return LV_FS_RES_NOT_IMP;
}

static lv_fs_res_t lvgl_fs_dir_close(struct _lv_fs_drv_t *drv, void *dir)
{
	int err;

	err = fs_closedir((struct fs_dir_t *)dir);
	return errno_to_lv_fs_res(err);
}

void lvgl_fs_init(void)
{
	lv_fs_drv_t fs_drv;
	lv_fs_drv_init(&fs_drv);

	fs_drv.file_size = sizeof(struct fs_file_t);
	fs_drv.rddir_size = sizeof(struct fs_dir_t);
	/* LVGL uses letter based mount points, just pass the root slash as a
	 * letter. Note that LVGL will remove the drive letter, or in this case
	 * the root slash, from the path passed via the FS callbacks.
	 * Zephyr FS API assumes this slash is present so we will need to add
	 * it back.
	 */
	fs_drv.letter = '/';
	fs_drv.ready_cb = lvgl_fs_ready;

	fs_drv.open_cb = lvgl_fs_open;
	fs_drv.close_cb = lvgl_fs_close;
	fs_drv.remove_cb = lvgl_fs_remove;
	fs_drv.read_cb = lvgl_fs_read;
	fs_drv.write_cb = lvgl_fs_write;
	fs_drv.seek_cb = lvgl_fs_seek;
	fs_drv.tell_cb = lvgl_fs_tell;
	fs_drv.trunc_cb = lvgl_fs_trunc;
	fs_drv.size_cb = lvgl_fs_size;
	fs_drv.rename_cb = lvgl_fs_rename;
	fs_drv.free_space_cb = lvgl_fs_free;

	fs_drv.dir_open_cb = lvgl_fs_dir_open;
	fs_drv.dir_read_cb = lvgl_fs_dir_read;
	fs_drv.dir_close_cb = lvgl_fs_dir_close;

	lv_fs_drv_register(&fs_drv);
}
