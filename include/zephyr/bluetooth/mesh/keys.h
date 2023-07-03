/** @file
 *  @brief Keys APIs.
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_KEYS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_KEYS_H_

#include <stdint.h>
#if defined CONFIG_BT_MESH_USES_MBEDTLS_PSA || defined CONFIG_BT_MESH_USES_TFM_PSA
#include <psa/crypto.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined CONFIG_BT_MESH_USES_MBEDTLS_PSA || defined CONFIG_BT_MESH_USES_TFM_PSA

/** The structure that keeps representation of key. */
struct bt_mesh_key {
	/** PSA key representation is the PSA key identifier. */
	psa_key_id_t key;
};

#elif defined CONFIG_BT_MESH_USES_TINYCRYPT

/** The structure that keeps representation of key. */
struct bt_mesh_key {
	/** tinycrypt key representation is the pure key value. */
	uint8_t key[16];
};

#else
#error "Crypto library has not been chosen"
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_KEYS_H_ */
