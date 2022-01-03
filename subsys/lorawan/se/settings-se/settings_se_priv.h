/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SETTINGS_SE_PRIV_H
#define SETTINGS_SE_PRIV_H

#include <zephyr/types.h>
#include <LoRaMacTypes.h>

struct settings_se_key {
	/*!
	 * Key value
	 */
	uint8_t value[16];

	/*!
	 * Random used for derivation
	 */
	uint8_t random[32];
};

int settings_se_keys_load(KeyIdentifier_t id, struct settings_se_key *key);

int settings_se_keys_save(KeyIdentifier_t id, struct settings_se_key *key);

#endif /* SETTINGS_SE_PRIV_H */
