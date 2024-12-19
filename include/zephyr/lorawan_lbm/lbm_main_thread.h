/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_LBM_LBM_MAIN_THREAD_H
#define SUBSYS_LORAWAN_LBM_LBM_MAIN_THREAD_H

#include <smtc_modem_hal_init.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the LoRa Basics Modem callbacks and starts its work thread.
 *
 * @param event_callback Function to be called when a modem event is raised internally
 * @param hal_cb User provided callbacks for Device Management
 */

void lora_basics_modem_start_work_thread(void (*event_callback)(void),
					 struct smtc_modem_hal_cb *hal_cb);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_LBM_LBM_MAIN_THREAD_H */
