/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_

/*
 * Entry points of the espressif,esp32-intc driver (intc_esp32.c) for the arch
 * glue in soc/espressif/common/irq.c. They take a multi-level encoded IRQ of
 * any level and are in IRAM, so irq_enable() and irq_disable() work while the
 * flash cache is off.
 *
 * A level-1 IRQ is a bare CPU line; when a consumer has the line to itself the
 * devicetree also names its source, which is routed to the line on enable and
 * away from it on disable. A level-2 IRQ names a matrix source, routed to the
 * CPU line its devicetree consumer uses. A level-3 IRQ acts on the source of
 * its aggregator on enable; on disable the peripheral masks its own bit.
 */

/**
 * @brief Enable an interrupt on the calling core.
 *
 * @param irq Multi-level encoded IRQ number
 */
void esp_intc_irq_enable(unsigned int irq);

/**
 * @brief Disable an interrupt on every core.
 *
 * @param irq Multi-level encoded IRQ number
 */
void esp_intc_irq_disable(unsigned int irq);

/**
 * @brief Check whether an interrupt is enabled.
 *
 * @param irq Multi-level encoded IRQ number
 *
 * @retval 1 Enabled
 * @retval 0 Disabled
 */
int esp_intc_irq_is_enabled(unsigned int irq);

/**
 * @brief Pause the interrupts that cannot run with the flash cache off.
 *
 * Acts on the calling core: masks every enabled CPU line not marked
 * ESP_INTR_FLAG_IRAM in the devicetree, and every reserved line. Interrupts
 * raised meanwhile stay pending on their line. Calls must pair with
 * esp_intc_noniram_enable().
 */
void esp_intc_noniram_disable(void);

/**
 * @brief Resume the interrupts paused by esp_intc_noniram_disable().
 */
void esp_intc_noniram_enable(void);

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_ */
