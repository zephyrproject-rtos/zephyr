/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_se_priv.h"
#include <zephyr.h>
#include <settings/settings.h>
#include <stdlib.h>
#include <stdio.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(lorawan_nrfx_se_sng, CONFIG_LORAWAN_LOG_LEVEL);

#define LORAWAN_KEYS_SETTINGS_BASE                "lorawan/keys"

static int load_setting(void *tgt, size_t tgt_size,
			const char *key, size_t len,
			settings_read_cb read_cb, void *cb_arg)
{
	if (len != tgt_size) {
		return -EINVAL;
	}

	if (!tgt) {
		return -EINVAL;
	}

	if (read_cb(cb_arg, tgt, len) != len) {
		return -EINVAL;
	}

	return 0;
}

K_SEM_DEFINE(nrfx_keys_sem, 1, 1)

static KeyIdentifier_t current_id;

static int on_setting_loaded(const char *key, size_t len,
			     settings_read_cb read_cb,
			     void *cb_arg, void *param)
{
	int err;
	struct settings_se_key *se_key = param;

	if (atoi(key) == current_id) {
		err = load_setting(se_key,
				   sizeof(*se_key),
				   key, len, read_cb, cb_arg);
		if (err) {
			return err;
		}

		return 0;
	}

	return 0;
}

int settings_se_keys_load(KeyIdentifier_t id, struct settings_se_key *key)
{
	int err;

	k_sem_take(&nrfx_keys_sem, K_FOREVER);

	current_id = id;
	err = settings_load_subtree_direct(LORAWAN_KEYS_SETTINGS_BASE,
					   on_setting_loaded,
					   key);

	k_sem_give(&nrfx_keys_sem);

	return err;
}

int settings_se_keys_save(KeyIdentifier_t id, struct settings_se_key *key)
{
	int err;
	char path[sizeof(LORAWAN_KEYS_SETTINGS_BASE) + 1 + 4] = {0};

	k_sem_take(&nrfx_keys_sem, K_FOREVER);

	sprintf(path, LORAWAN_KEYS_SETTINGS_BASE"/%d", id);
	err = settings_save_one(path, key, sizeof(*key));

	k_sem_give(&nrfx_keys_sem);

	return err;
}
