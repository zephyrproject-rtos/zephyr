/**
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif interrupt definitions shared by every SoC
 * @ingroup dt_espressif_intmux_common
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESPRESSIF_INTMUX_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESPRESSIF_INTMUX_COMMON_H_

#include <zephyr/dt-bindings/interrupt-controller/espressif-esp32-l3-intc.h>

/**
 * @defgroup dt_espressif_intmux_common Espressif common interrupt definitions
 * @brief Devicetree interrupt definitions that do not depend on the SoC.
 * @ingroup devicetree-interrupt_controller
 *
 * Included by every per-SoC interrupt header, so a devicetree file gets these by
 * including only the header for its own SoC.
 *
 * Note that IRQ_DEFAULT_PRIORITY is deliberately *not* here: it is 0 on Xtensa
 * and 1 on RISC-V, and a devicetree header is preprocessed with no way to tell
 * the two apart. It stays in the per-SoC headers.
 *
 * @{
 */

/**
 * @name Interrupt allocation flags
 *
 * The flags cell of an "espressif,esp32-intc" interrupt. Values mirror
 * ESP_INTR_FLAG_* in hal_espressif
 * (components/esp_hw_support/include/esp_intr_alloc.h) and must stay in step
 * with it.
 *
 * The LEVELn values are deliberately (1 << level), so a level can be turned
 * into its flag by shifting.
 *
 * @{
 */
#define ESP_INTR_FLAG_LEVEL1 (1 << 1)  /**< Accept a level 1 vector, lowest priority */
#define ESP_INTR_FLAG_LEVEL2 (1 << 2)  /**< Accept a level 2 vector */
#define ESP_INTR_FLAG_LEVEL3 (1 << 3)  /**< Accept a level 3 vector */
#define ESP_INTR_FLAG_LEVEL4 (1 << 4)  /**< Accept a level 4 vector */
#define ESP_INTR_FLAG_LEVEL5 (1 << 5)  /**< Accept a level 5 vector */
#define ESP_INTR_FLAG_LEVEL6 (1 << 6)  /**< Accept a level 6 vector */
#define ESP_INTR_FLAG_NMI    (1 << 7)  /**< Accept a level 7 vector, highest priority */
#define ESP_INTR_FLAG_SHARED (1 << 8)  /**< Can be shared between ISRs */
#define ESP_INTR_FLAG_EDGE   (1 << 9)  /**< Edge-triggered interrupt */
#define ESP_INTR_FLAG_IRAM   (1 << 10) /**< ISR can be called if cache is disabled */
#define ESP_INTR_FLAG_INTRDISABLED (1 << 11) /**< Return with this interrupt disabled */
#define ESP_INTR_FLAG_SHARED_PRIVATE (1 << 12) /**< Shared with `*_bind` functions only */

/** Low and medium priority interrupts. These can be handled in C. */
#define ESP_INTR_FLAG_LOWMED (ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_LEVEL3)

/** High level interrupts. Need to be handled in assembly. */
#define ESP_INTR_FLAG_HIGH                                                                         \
	(ESP_INTR_FLAG_LEVEL4 | ESP_INTR_FLAG_LEVEL5 | ESP_INTR_FLAG_LEVEL6 | ESP_INTR_FLAG_NMI)

/** Mask for all level flags */
#define ESP_INTR_FLAG_LEVELMASK                                                                    \
	(ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_LEVEL3 |                       \
	 ESP_INTR_FLAG_LEVEL4 | ESP_INTR_FLAG_LEVEL5 | ESP_INTR_FLAG_LEVEL6 | ESP_INTR_FLAG_NMI)

/** @} */

/**
 * @brief Define the "espressif,esp32-cpu-interrupt" node of CPU line @p n.
 *
 * Use inside the "espressif,esp32-intc" node, next to the &intmux node. The
 * result is node and label cpu_interrupt_<n>, an interrupt nexus whose single
 * map row sends every consumer to intmux as <n source>: the mask reduces any
 * consumer cell to the row key 0, and pass-thru copies the consumer cell (the
 * matrix source) into the rightmost parent cell. The line is written once, for
 * both "cpu-line" and the map.
 *
 * Devicetree only: the bare '#' of the cell-count properties is accepted by the
 * devicetree preprocessor (assembler-with-cpp) but is an error in C.
 *
 * @param n CPU interrupt line, a plain decimal number
 */
#ifdef __DTS__
#define ESP_CPU_INTERRUPT(n)                                                                       \
	cpu_interrupt_##n: cpu_interrupt_##n {                                                     \
		compatible = "espressif,esp32-cpu-interrupt";                                      \
		#interrupt-cells = <1>;                                                            \
		#address-cells = <0>;                                                              \
		cpu-line = <n>;                                                                    \
		interrupt-map-mask = <0>;                                                          \
		interrupt-map-pass-thru = <0xffffffff>;                                            \
		interrupt-map = <0 &intmux n 0>;                                                   \
	};
#endif /* __DTS__ */


/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESPRESSIF_INTMUX_COMMON_H_ */
