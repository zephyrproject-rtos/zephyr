/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_MP_STALL_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_MP_STALL_H_

#include <zephyr/kernel.h>
#include <esp_attr.h>

/* Cross-core stall used while the shared cache is suspended: the peer core is
 * parked in an IRAM spin inside its stall ISR until the requester releases it.
 */

/**
 * @brief Pause the other CPU and enter a section safe for suspending the cache
 *
 * Re-entrant on the owning CPU.
 */
void soc_mp_pause_others(void);

/**
 * @brief Release the peer and leave the section opened by soc_mp_pause_others()
 */
void soc_mp_resume_others(void);

/**
 * @brief Stall ISR body, provided by the common layer
 *
 * @param arg Unused
 */
void esp_mp_stall_isr(const void *arg);

/**
 * @brief Install the stall ISR for the calling core on its FROM_CPU stall line
 *
 * @param core_id Core the ISR is installed on, which is the calling core
 *
 * @retval 0 Success
 * @retval -errno The interrupt allocation failed
 */
int esp_mp_stall_isr_register(int core_id);

/**
 * @brief Publish that a core is online
 *
 * Set only after that core's stall ISR is installed.
 *
 * @param cpu Core number
 * @param online true once the core has its stall ISR installed
 */
void esp_mp_set_cpu_online(int cpu, bool online);

/**
 * @brief Return whether a core has been published online
 *
 * @param cpu Core number
 *
 * @return true if esp_mp_set_cpu_online() marked the core online
 */
bool esp_mp_cpu_online(int cpu);

/**
 * @brief Arm the protocol
 *
 */
void esp_mp_stall_enable(void);

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_MP_STALL_H_ */
