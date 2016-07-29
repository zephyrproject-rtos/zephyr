/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <init.h>
#include <ff.h>
#include <fs.h>
#include <misc/__assert.h>

static FATFS fat_fs;	/* FatFs work area */

static int translate_error(int error)
{
	switch (error) {
	case FR_OK:
		return 0;
	case FR_NO_FILE:
	case FR_NO_PATH:
	case FR_INVALID_NAME:
		return -ENOENT;
	case FR_DENIED:
		return -EACCES;
	case FR_EXIST:
		return -EEXIST;
	case FR_INVALID_OBJECT:
		return -EBADF;
	case FR_WRITE_PROTECTED:
		return -EROFS;
	case FR_INVALID_DRIVE:
	case FR_NOT_ENABLED:
	case FR_NO_FILESYSTEM:
		return -ENODEV;
	case FR_NOT_ENOUGH_CORE:
		return -ENOMEM;
	case FR_TOO_MANY_OPEN_FILES:
		return -EMFILE;
	case FR_INVALID_PARAMETER:
		return -EINVAL;
	case FR_LOCKED:
	case FR_TIMEOUT:
	case FR_MKFS_ABORTED:
	case FR_DISK_ERR:
	case FR_INT_ERR:
	case FR_NOT_READY:
		return -EIO;
	}

	return -EIO;
}

int fs_open(ZFILE *zfp, const char *file_name)
{
	FRESULT res;
	uint8_t fs_mode;

	fs_mode = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;

	res = f_open(&zfp->fp, file_name, fs_mode);

	return translate_error(res);
}

int fs_close(ZFILE *zfp)
{
	FRESULT res;

	res = f_close(&zfp->fp);

	return translate_error(res);
}

int fs_unlink(const char *path)
{
	FRESULT res;

	res = f_unlink(path);

	return translate_error(res);
}

ssize_t fs_read(ZFILE *zfp, void *ptr, size_t size)
{
	FRESULT res;
	unsigned int br;

	res = f_read(&zfp->fp, ptr, size, &br);
	if (res != FR_OK) {
		return translate_error(res);
	}

	return br;
}

ssize_t fs_write(ZFILE *zfp, const void *ptr, size_t size)
{
	FRESULT res;
	unsigned int bw;

	res = f_write(&zfp->fp, ptr, size, &bw);
	if (res != FR_OK) {
		return translate_error(res);
	}

	return bw;
}

int fs_seek(ZFILE *zfp, off_t offset, int whence)
{
	FRESULT res = FR_OK;
	off_t pos;

	switch (whence) {
	case SEEK_SET:
		pos = offset;
		break;
	case SEEK_CUR:
		pos = f_tell(&zfp->fp) + offset;
		break;
	case SEEK_END:
		pos = f_size(&zfp->fp) + offset;
		break;
	default:
		return -EINVAL;
	}

	if ((pos < 0) || (pos > f_size(&zfp->fp))) {
		return -EINVAL;
	}

	res = f_lseek(&zfp->fp, pos);

	return translate_error(res);
}

int fs_mkdir(const char *path)
{
	FRESULT res;

	res = f_mkdir(path);

	return translate_error(res);
}

int fs_opendir(ZDIR *zdp, const char *path)
{
	FRESULT res;

	res = f_opendir(&zdp->dp, path);

	return translate_error(res);
}

int fs_readdir(ZDIR *zdp, struct zfs_dirent *entry)
{
	FRESULT res;
	FILINFO fno;

	res = f_readdir(&zdp->dp, &fno);
	if (res == FR_OK) {
		entry->type = ((fno.fattrib & AM_DIR) ?
			       DIR_ENTRY_DIR : DIR_ENTRY_FILE);
		strcpy(entry->name, fno.fname);
		entry->size = fno.fsize;
	}

	return translate_error(res);
}

int fs_closedir(ZDIR *zdp)
{
	FRESULT res;

	res = f_closedir(&zdp->dp);

	return translate_error(res);
}

int fs_stat(const char *path, struct zfs_dirent *entry)
{
	FRESULT res;
	FILINFO fno;

	res = f_stat(path, &fno);
	if (res == FR_OK) {
		entry->type = ((fno.fattrib & AM_DIR) ?
			       DIR_ENTRY_DIR : DIR_ENTRY_FILE);
		strcpy(entry->name, fno.fname);
		entry->size = fno.fsize;
	}

	return translate_error(res);
}

static int fs_init(struct device *dev)
{
	FRESULT res;

	ARG_UNUSED(dev);

	res = f_mount(&fat_fs, "", 1);

	/* If no file system found then create one */
	if (res == FR_NO_FILESYSTEM) {
		uint8_t work[_MAX_SS];

		res = f_mkfs("", (FM_FAT | FM_SFD), _MAX_SS,
			     work, sizeof(work));

		if (res == FR_OK) {
			res = f_mount(&fat_fs, "", 1);
		}
	}

	__ASSERT((res == FR_OK), "FS init failed (%d)", translate_error(res));

	return translate_error(res);
}

SYS_INIT(fs_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
