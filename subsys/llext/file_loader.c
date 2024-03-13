/*
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/file_loader.h>
#include <zephyr/sys/util.h>
#include <string.h>

int llext_file_read(struct llext_loader *l, void *buf, size_t len)
{
	struct llext_file_loader *file_l = CONTAINER_OF(l, struct llext_file_loader, loader);

	int nb_read = fs_read(file_l->fd, buf, len);
    int rc = 0;

    if(nb_read != len) {
        rc = -ENOMEM;
    }

	return rc;
}

int llext_file_seek(struct llext_loader *l, size_t pos)
{
	struct llext_file_loader *file_l = CONTAINER_OF(l, struct llext_file_loader, loader);

	return fs_seek(file_l->fd, pos, FS_SEEK_SET);
}
