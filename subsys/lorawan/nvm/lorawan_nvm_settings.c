/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include "lorawan_nvm.h"

static const char *const record_names[] = {
	[LORAWAN_NVM_CRYPTO] = "lorawan/nvm/Crypto",
	[LORAWAN_NVM_MAC_GROUP1] = "lorawan/nvm/MacGroup1",
	[LORAWAN_NVM_MAC_GROUP2] = "lorawan/nvm/MacGroup2",
	[LORAWAN_NVM_SECURE_ELEMENT] = "lorawan/nvm/SecureElement",
	[LORAWAN_NVM_REGION_GROUP1] = "lorawan/nvm/RegionGroup1",
	[LORAWAN_NVM_REGION_GROUP2] = "lorawan/nvm/RegionGroup2",
	[LORAWAN_NVM_CLASS_B] = "lorawan/nvm/ClassB",
};

struct nvm_read_ctx {
	void *data;
	size_t size;
	bool found;
	int error;
};

static int read_record(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
		       void *param)
{
	struct nvm_read_ctx *ctx = param;
	ssize_t ret;

	/* A subtree lookup also visits child keys; only accept an exact match. */
	if ((key != NULL && key[0] != '\0') || ctx->error != 0) {
		return ctx->error;
	}
	ctx->found = true;
	if (len != ctx->size) {
		ctx->error = -EINVAL;
		return ctx->error;
	}
	ret = read_cb(cb_arg, ctx->data, ctx->size);
	if (ret < 0) {
		ctx->error = (int)ret;
	} else if (ret != ctx->size) {
		ctx->error = -EIO;
	}
	return ctx->error;
}

int lorawan_nvm_init(void)
{
	return settings_subsys_init();
}

int lorawan_nvm_read(enum lorawan_nvm_id id, void *data, size_t size)
{
	struct nvm_read_ctx ctx = {
		.data = data,
		.size = size,
	};
	int ret;

	if ((unsigned int)id >= ARRAY_SIZE(record_names) || data == NULL || size == 0) {
		return -EINVAL;
	}
	ret = settings_load_subtree_direct(record_names[id], read_record, &ctx);
	if (ret != 0) {
		return ret;
	}
	/* Settings can discard callback errors; preserve them in the load context. */
	if (ctx.error != 0) {
		return ctx.error;
	}
	return ctx.found ? 0 : -ENOENT;
}

int lorawan_nvm_write(enum lorawan_nvm_id id, const void *data, size_t size)
{
	if ((unsigned int)id >= ARRAY_SIZE(record_names) || data == NULL || size == 0) {
		return -EINVAL;
	}
	return settings_save_one(record_names[id], data, size);
}
