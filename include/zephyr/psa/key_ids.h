/* Copyright (c) 2025 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_PSA_KEY_IDS_H_
#define ZEPHYR_PSA_KEY_IDS_H_

/**
 * @file zephyr/psa/key_ids.h
 *
 * @brief This file defines the key ID ranges of the existing users of the PSA Crypto API.
 *
 * In addition to the application, different subsystems store and use persistent keys through the
 * PSA Crypto API. Because they are not aware of each other, collisions are avoided by having them
 * use different ID ranges.
 * This file acts as the registry of all the allocated PSA key ID ranges within Zephyr.
 *
 * The end-user application also has a dedicated range, `ZEPHYR_PSA_APPLICATION_KEY_ID_RANGE_BEGIN`.
 *
 * Some of the IDs below are based on previously existing and used values, while others
 * are chosen to be somewhere in the PSA user key ID range to try to avoid collisions
 * (avoiding, for example, the very beginning of the range).
 */

#include <stdint.h>
typedef uint32_t psa_key_id_t;

/** PSA key ID range to be used by OpenThread. The base ID is equal to the default value upstream:
 *  https://github.com/openthread/openthread/blob/thread-reference-20230706/src/core/config/platform.h#L138
 */
#define ZEPHYR_PSA_OPENTHREAD_KEY_ID_RANGE_BEGIN (psa_key_id_t)0x20000
#define ZEPHYR_PSA_OPENTHREAD_KEY_ID_RANGE_SIZE  0x10000 /* 64 Ki */

/** PSA key ID range to be used by Matter. The base ID is equal to the default value upstream:
 * https://github.com/project-chip/connectedhomeip/blob/v1.4.0.0/src/crypto/CHIPCryptoPALPSA.h#L55
 */
#define ZEPHYR_PSA_MATTER_KEY_ID_RANGE_BEGIN (psa_key_id_t)0x30000
#define ZEPHYR_PSA_MATTER_KEY_ID_RANGE_SIZE  0x10000 /* 64 Ki */

/** PSA key ID range to be used by Bluetooth Mesh. */
#define ZEPHYR_PSA_BT_MESH_KEY_ID_RANGE_BEGIN (psa_key_id_t)0x20000000
#define ZEPHYR_PSA_BT_MESH_KEY_ID_RANGE_SIZE  0xC000 /* 48 Ki */

/** PSA key ID range to be used by Wi-Fi credentials management. */
#define ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_BEGIN (psa_key_id_t)0x20010000
#define ZEPHYR_PSA_WIFI_CREDENTIALS_KEY_ID_RANGE_SIZE  0x100 /* 256 */

/** PSA key ID range to be used by the end-user application. */
#define ZEPHYR_PSA_APPLICATION_KEY_ID_RANGE_BEGIN (psa_key_id_t)0x30000000
#define ZEPHYR_PSA_APPLICATION_KEY_ID_RANGE_SIZE  0x100000 /* 1 Mi */

#endif /* ZEPHYR_PSA_KEY_IDS_H_ */
