/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/llext/fs_loader.h>
#include <zephyr/sys/util.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(llext_fs_loader, CONFIG_LLEXT_LOG_LEVEL);

int llext_fs_prepare(struct llext_loader *l)
{
	int ret = 0;
	struct llext_fs_loader *fs_l = CONTAINER_OF(l, struct llext_fs_loader, loader);

	fs_file_t_init(&fs_l->file);

	ret = fs_open(&fs_l->file, fs_l->name, FS_O_READ);
	if (ret != 0) {
		LOG_DBG("Failed opening a file: %d", ret);
		return ret;
	}

	fs_l->is_open = true;
	return 0;
}

int llext_fs_read(struct llext_loader *l, void *buf, size_t len)
{
	int ret = 0;
	struct llext_fs_loader *fs_l = CONTAINER_OF(l, struct llext_fs_loader, loader);

	if (fs_l->is_open) {
		ret = fs_read(&fs_l->file, buf, len);
	} else {
		ret = -EINVAL;
	}

	return ret == len ? 0 : -EINVAL;
}

int llext_fs_seek(struct llext_loader *l, size_t pos)
{
	struct llext_fs_loader *fs_l = CONTAINER_OF(l, struct llext_fs_loader, loader);

	if (fs_l->is_open) {
		return fs_seek(&fs_l->file, pos, FS_SEEK_SET);
	} else {
		return -EINVAL;
	}
}

void llext_fs_finalize(struct llext_loader *l)
{
	struct llext_fs_loader *fs_l = CONTAINER_OF(l, struct llext_fs_loader, loader);

	if (fs_l->is_open) {
		fs_close(&fs_l->file);
		fs_l->is_open = false;
	}
}
