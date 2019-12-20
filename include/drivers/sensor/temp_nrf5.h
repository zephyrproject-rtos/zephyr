/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef TEMP_NRF5_H__
#define TEMP_NRF5_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Callback.
 *
 * @param sample	Raw sample value.
 * @param user_data	User data.
 */
typedef void (*temp_nrf5_callback_t)(int32_t sample, void *user_data);

/** @brief Asynchronously sample temperature.
 *
 * @param callback	Callback called when sampling is completed.
 * @param user_data	User data passed to the callback.
 *
 * @retval 0 on success,
 * @retval -EAGAIN if module is not initialized.
 * @retval -EBUSY if sampling is in progress.
 */
int temp_nrf5_sample(temp_nrf5_callback_t callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* TEMP5_NRF_H__ */
