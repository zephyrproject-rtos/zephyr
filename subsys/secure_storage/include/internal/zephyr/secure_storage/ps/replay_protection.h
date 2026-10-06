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
#include <zephyr/secure_storage/ps/common.h>

/** @brief Get the replay protection value for the given data.
 *
 * This is called every time an entry is written. The value is stored in ITS and passed to
 * the PS transform module, which binds it to the data stored in PS.
 *
 * @param[in]  data     The unencrypted data of the entry that's going to be stored in PS.
 *                      An implementation may use it to generate the replay protection value.
 *                      May be NULL when `data_len` is 0.
 * @param[in]  data_len The number of bytes in `data`.
 * @param[in]  curr_rp  The current replay protection value of the entry, of
 *                      @kconfig{CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE} bytes.
 *                      NULL if the entry doesn't exist or its current replay protection value
 *                      cannot be read.
 * @param[out] output   The buffer where the replay protection value will be written to.
 *                      Exactly @kconfig{CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE} bytes
 *                      must be written.
 *
 * @retval PSA_SUCCESS The replay protection value was written to `output`.
 * @return Anything else on failure. `psa_ps_set()` then returns `PSA_ERROR_GENERIC_ERROR`.
 */
psa_status_t secure_storage_ps_get_replay_protection(
		const uint8_t *data,
		size_t data_len,
		const uint8_t *curr_rp,
		uint8_t output[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE]);

#endif /* SECURE_STORAGE_PS_REPLAY_PROTECTION_H */
