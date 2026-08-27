/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_

/*
 * Level-1 half of the espressif,esp32-intc driver (intc_esp32.c), called by the
 * arch glue in soc/espressif/common/irq.c. Level-2 and level-3 IRQs go through
 * the irq_next_level API instead.
 *
 * A level-1 IRQ number is the bare CPU line. A consumer that has the line to
 * itself also names the interrupt matrix source in the devicetree, so enabling
 * the line routes that source to it, and disabling the line unroutes it.
 */

/**
 * @brief Enable a CPU interrupt line, routing its dedicated source if it has one.
 *
 * @param line CPU interrupt line
 */
void esp_intc_line_enable(unsigned int line);

/**
 * @brief Disable a CPU interrupt line, unrouting its dedicated source if it has one.
 *
 * @param line CPU interrupt line
 */
void esp_intc_line_disable(unsigned int line);

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_INCLUDE_ESP_SOC_IRQ_H_ */
