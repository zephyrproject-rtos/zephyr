/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/common/semihost.h>
#include <zephyr/llext/semihost_loader.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(llext_semihost_loader, CONFIG_LLEXT_LOG_LEVEL);

int llext_semihost_prepare(struct llext_loader *ldr)
{
	struct llext_semihost_loader *sh_l =
		CONTAINER_OF(ldr, struct llext_semihost_loader, loader);

	sh_l->fd = semihost_open(sh_l->filename, SEMIHOST_OPEN_RB);
	if (sh_l->fd < 0) {
		LOG_ERR("Failed to open file: %s", sh_l->filename);
		return -ENOENT;
	}

	return 0;
}

int llext_semihost_read(struct llext_loader *ldr, void *buf, size_t len)
{
	struct llext_semihost_loader *sh_l =
		CONTAINER_OF(ldr, struct llext_semihost_loader, loader);

	long ret = semihost_read(sh_l->fd, buf, len);

	if (ret < 0) {
		LOG_ERR("Failed to read from file: %s: %ld", sh_l->filename, ret);
		return -EIO;
	}

	sh_l->total_bytes += ret;

	return 0;
}

int llext_semihost_seek(struct llext_loader *ldr, size_t pos)
{
	struct llext_semihost_loader *sh_l =
		CONTAINER_OF(ldr, struct llext_semihost_loader, loader);

	long ret = semihost_seek(sh_l->fd, pos);

	if (ret < 0) {
		LOG_ERR("Failed to seek in file: %s: %ld", sh_l->filename, ret);
		return -EIO;
	}

	return 0;
}

void llext_semihost_finalize(struct llext_loader *ldr)
{
	struct llext_semihost_loader *sh_l =
		CONTAINER_OF(ldr, struct llext_semihost_loader, loader);

	if (sh_l->fd >= 0) {
		long ret = semihost_close(sh_l->fd);

		if (ret < 0) {
			LOG_ERR("Failed to close file: %s: %ld", sh_l->filename, ret);
		}
		sh_l->fd = -1;
	}
}
