/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/authentication/fido2/fido2_types.h>
#include <zephyr/authentication/fido2/fido2_storage.h>

static int none_backend_init(void)
{
	return 0;
}

static int none_backend_store(const struct fido2_credential *cred)
{
	ARG_UNUSED(cred);

	return -ENOTSUP;
}

static int none_backend_load(const uint8_t *cred_id, size_t cred_id_len,
			     struct fido2_credential *cred)
{
	ARG_UNUSED(cred_id);
	ARG_UNUSED(cred_id_len);
	ARG_UNUSED(cred);

	return -ENOENT;
}

static int none_backend_remove(const uint8_t *cred_id, size_t cred_id_len,
			       struct fido2_credential *cred)
{
	ARG_UNUSED(cred_id);
	ARG_UNUSED(cred_id_len);
	ARG_UNUSED(cred);

	return -ENOENT;
}

static int none_backend_find_by_rp(const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				   struct fido2_credential *creds, size_t max_creds, size_t *count)
{
	ARG_UNUSED(rp_id_hash);
	ARG_UNUSED(creds);
	ARG_UNUSED(max_creds);

	*count = 0;

	return 0;
}

static int none_backend_sign_count_increment(const uint8_t *cred_id, size_t cred_id_len,
					     uint32_t *new_count)
{
	ARG_UNUSED(cred_id);
	ARG_UNUSED(cred_id_len);
	ARG_UNUSED(new_count);

	return -ENOTSUP;
}

const struct fido2_storage_api fido2_storage_backend = {
	.init = none_backend_init,
	.store = none_backend_store,
	.load = none_backend_load,
	.remove = none_backend_remove,
	.find_by_rp = none_backend_find_by_rp,
	.sign_count_increment = none_backend_sign_count_increment,
};
