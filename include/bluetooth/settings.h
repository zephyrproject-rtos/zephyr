/** @file
 *  @brief Generic Bluetooth settings key handling.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SETTINGS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SETTINGS_H_

#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bluetooth Settings encode key.
 *
 *  This function encodes the Bluetooth settings key from the
 *  device Bluetooth LE address.
 *
 *  @param path Pointer to Bluetooth settings path buffer.
 *  @param path_size Maximum size of the Bluetooth settings path buffer.
 *  @param subsys Pointer to subsystem path string.
 *  @param addr Pointer to device Bluetooth LE address.
 *  @param key Pointer to the ID string.
 */
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
			    bt_addr_le_t *addr, const char *key);

/** @brief Bluetooth Settings decode key.
 *
 *  This function decodes the Bluetooth settings key to the
 *  device Bluetooth LE address
 *
 *  @param key Pointer to Bluetooth settings key.
 *  @param addr Pointer to device Bluetooth LE address.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_settings_decode_key(const char *key, bt_addr_le_t *addr);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SETTINGS_H_ */
