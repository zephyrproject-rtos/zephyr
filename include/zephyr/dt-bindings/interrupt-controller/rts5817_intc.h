/*
 * Copyright (c) 2025 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Realtek RTS5817 interrupt source numbers
 * @ingroup dt_realtek_rts5817_intc
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RTS5817_INTC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RTS5817_INTC_H_

/**
 * @defgroup dt_realtek_rts5817_intc Realtek RTS5817 interrupt sources
 * @brief Devicetree interrupt source numbers for the Realtek RTS5817.
 * @ingroup devicetree-interrupt_controller
 *
 * Peripheral interrupt line numbers for the RTS5817 SoC. These @c IRQ_NUM_* values are used as the
 * interrupt number in a peripheral's @c interrupts property, with the Arm NVIC as the parent
 * interrupt controller.
 *
 * @code{.dts}
 * &uart0 {
 *         interrupt-parent = <&nvic>;
 *         interrupts = <IRQ_NUM_UART0 0>;
 * };
 * @endcode
 * @{
 */

#define IRQ_NUM_SIE        0  /**< Sensor interface engine interrupt */
#define IRQ_NUM_MC         1  /**< Memory controller interrupt */
#define IRQ_NUM_SENSOR_SPI 4  /**< Sensor SPI interrupt */
#define IRQ_NUM_SPI_MASTER 5  /**< SPI master interrupt */
#define IRQ_NUM_AES        6  /**< AES accelerator interrupt */
#define IRQ_NUM_SHA        7  /**< SHA accelerator interrupt */
#define IRQ_NUM_GPIO       11 /**< GPIO interrupt */
#define IRQ_NUM_SP_GPIO    12 /**< Special-purpose GPIO interrupt */
#define IRQ_NUM_UART0      15 /**< UART0 interrupt */
#define IRQ_NUM_UART1      16 /**< UART1 interrupt */
#define IRQ_NUM_SPI_SLAVE  23 /**< SPI slave interrupt */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RTS5817_INTC_H_ */
