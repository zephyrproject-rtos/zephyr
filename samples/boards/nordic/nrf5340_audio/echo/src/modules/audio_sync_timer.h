/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AUDIO_SYNC_TIMER_H_
#define _AUDIO_SYNC_TIMER_H_

#include <zephyr/kernel.h>
#include <stdint.h>

/**
 * @brief Capture a timestamp on the sync timer.
 *
 * @retval The current timestamp of the audio sync timer.
 */
uint32_t audio_sync_timer_capture(void);

/**
 * @brief Returns the last captured value of the sync timer.
 *
 * The captured time is corresponding to the I2S frame start.
 *
 * See @ref audio_sync_timer_capture().
 *
 * @retval The last captured timestamp of the audio sync timer.
 */
uint32_t audio_sync_timer_capture_get(void);

#endif /* _AUDIO_SYNC_TIMER_H_ */
