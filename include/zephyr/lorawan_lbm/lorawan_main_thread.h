/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_LBM_LBM_MAIN_THREAD_H
#define SUBSYS_LORAWAN_LBM_LBM_MAIN_THREAD_H

#include <zephyr/lorawan_lbm/lorawan_hal_init.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the LoRa Basics Modem callbacks and starts its work thread.
 *
 * @param event_callback The callback that will be called each time an modem event
 * is raised internally
 */

void lora_basics_modem_start_work_thread(void (*event_callback)(void));

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_LBM_LBM_MAIN_THREAD_H */
