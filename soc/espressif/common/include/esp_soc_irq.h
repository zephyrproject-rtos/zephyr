/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_

#include <stdint.h>

/**
 * @brief Register IRAM flags for a CPU interrupt line connect path client.
 *
 * All clients on the same CPU IRQ line must agree on IRAM capability.
 * Returns -EINVAL if registration would mix IRAM and non-IRAM clients.
 * Updates non_iram_int_mask when any non-IRAM client is present.
 *
 * Interim implementation lives in intc_esp32.c until Phase 2 migration.
 */
int z_soc_irq_flags_apply(unsigned int irq, uint32_t flags);

/**
 * @brief Undo a prior z_soc_irq_flags_apply() for the same irq/flags pair.
 */
int z_soc_irq_flags_clear(unsigned int irq, uint32_t flags);

/**
 * @brief Validate ISR routine and flags.
 */
int z_soc_irq_validate(void (*isr)(const void *parameter), uint32_t flags);

#if defined(CONFIG_ZTEST)
uint8_t z_soc_irq_line_total_clients_get(unsigned int irq);
uint8_t z_soc_irq_line_non_iram_clients_get(unsigned int irq);
uint32_t z_soc_irq_non_iram_int_mask_get(unsigned int irq);
#endif

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_ */
