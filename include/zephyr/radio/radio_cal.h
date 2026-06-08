/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic radio RF calibration data persistence.
 *
 * Vendor-neutral helper for storing and retrieving radio RF calibration data
 * in non-volatile storage. Some radios cache their RF calibration in host
 * flash and reuse it across reboots to avoid a slow full calibration on every
 * boot. The same calibration typically serves whichever protocol brings the
 * radio up first (Wi-Fi, Bluetooth or 802.15.4), so this facility is not tied
 * to any single protocol.
 */

#ifndef ZEPHYR_INCLUDE_RADIO_RADIO_CAL_H_
#define ZEPHYR_INCLUDE_RADIO_RADIO_CAL_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Store a radio calibration blob.
 *
 * @param key   Stable identifier for the blob.
 * @param data  Pointer to the data to store.
 * @param len   Length of the data in bytes.
 *
 * @return 0 on success, negative errno on failure.
 */
int radio_cal_store(const char *key, const void *data, size_t len);

/**
 * @brief Load a radio calibration blob.
 *
 * The stored entry must match @p len exactly; a missing entry or a size
 * mismatch is reported as an error so the caller falls back to a full
 * calibration.
 *
 * @param key   Stable identifier used when the blob was stored.
 * @param data  Buffer to receive the data.
 * @param len   Size of the buffer in bytes.
 *
 * @return 0 on success, negative errno on failure (including when no entry of
 *         the requested size is stored).
 */
int radio_cal_load(const char *key, void *data, size_t len);

/**
 * @brief Erase a stored radio calibration blob.
 *
 * @param key   Stable identifier used when the blob was stored.
 *
 * @return 0 on success, negative errno on failure.
 */
int radio_cal_erase(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RADIO_RADIO_CAL_H_ */
