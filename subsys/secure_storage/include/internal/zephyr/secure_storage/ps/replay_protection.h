/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_PS_REPLAY_PROTECTION_H
#define SECURE_STORAGE_PS_REPLAY_PROTECTION_H

/** @file zephyr/secure_storage/ps/replay_protection.h
 * The secure storage PS replay protection module.
 *
 * The function declared in this header implements the PS replay protection module.
 * It is meant to be called only by the PS implementation.
 * This header may be included when providing a custom implementation of the
 * PS replay protection module (@kconfig{CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_CUSTOM}).
 */
#include <zephyr/secure_storage/uid.h>
#include <zephyr/secure_storage/ps/common.h>

/** @brief Get the replay protection value for the given stored data.
 *
 * @param[in] stored_data     The buffer containing the data coming from/going to the PS store.
 * @param[in] stored_data_len The number of bytes in `stored_data`.
 * @param[out] output         The buffer where the replay protection value will be written to.
 * @param[out] output_len     On success, the number of bytes written to `output`.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_ps_get_replay_protection(
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t stored_data_len,
		uint8_t output[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		size_t *output_len);

#endif /* SECURE_STORAGE_PS_REPLAY_PROTECTION_H */
