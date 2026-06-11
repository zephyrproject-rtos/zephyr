/*
 * SPDX-FileCopyrightText: Copyright Bavariamatic GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file microchip-smartfusion2-irq.h
 * @brief SmartFusion2 interrupt binding identifiers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_MICROCHIP_SMARTFUSION2_IRQ_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_MICROCHIP_SMARTFUSION2_IRQ_H_

/**
 * @defgroup smartfusion2_irq SmartFusion2 Interrupt Bindings
 * @brief SmartFusion2 SoC interrupt assignments.
 * @{
 */

/** @brief Default, unspecified interrupt type. */
#define IRQ_TYPE_NONE                      0
/** @brief Rising-edge triggered interrupt. */
#define IRQ_TYPE_EDGE_RISING               1
/** @brief Falling-edge triggered interrupt. */
#define IRQ_TYPE_EDGE_FALLING              2
/** @brief Both-edge triggered interrupt. */
#define IRQ_TYPE_EDGE_BOTH                 (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
/** @brief Active-high level triggered interrupt. */
#define IRQ_TYPE_LEVEL_HIGH                4
/** @brief Active-low level triggered interrupt. */
#define IRQ_TYPE_LEVEL_LOW                 8

/** @brief Watchdog wakeup source; not connected to the NVIC. */
#define SMARTFUSION2_IRQ_WDOG_WAKEUP      0
/** @brief RTC wakeup source; not connected to the NVIC. */
#define SMARTFUSION2_IRQ_RTC_WAKEUP       1
/** @brief SPI0 interrupt. */
#define SMARTFUSION2_IRQ_SPI0             2
/** @brief SPI1 interrupt. */
#define SMARTFUSION2_IRQ_SPI1             3
/** @brief I2C0 interrupt. */
#define SMARTFUSION2_IRQ_I2C0             4
/** @brief I2C1 interrupt. */
#define SMARTFUSION2_IRQ_I2C1             7
/** @brief UART0 interrupt. */
#define SMARTFUSION2_IRQ_UART0            10
/** @brief UART1 interrupt. */
#define SMARTFUSION2_IRQ_UART1            11
/** @brief Ethernet MAC interrupt. */
#define SMARTFUSION2_IRQ_ETHERNET_MAC     12
/** @brief Peripheral DMA interrupt. */
#define SMARTFUSION2_IRQ_PDMA             13
/** @brief Timer 1 interrupt. */
#define SMARTFUSION2_IRQ_TIMER1           14
/** @brief Timer 2 interrupt. */
#define SMARTFUSION2_IRQ_TIMER2           15
/** @brief CAN controller interrupt. */
#define SMARTFUSION2_IRQ_CAN              16
/** @brief Communication block interrupt. */
#define SMARTFUSION2_IRQ_COMBLK           19
/** @brief USB controller interrupt. */
#define SMARTFUSION2_IRQ_USB              20
/** @brief USB DMA interrupt. */
#define SMARTFUSION2_IRQ_USB_DMA          21
/** @brief Main PLL lock interrupt. */
#define SMARTFUSION2_IRQ_PLL_LOCK         22
/** @brief Main PLL lock-lost interrupt. */
#define SMARTFUSION2_IRQ_PLL_LOCK_LOST    23
/** @brief Communication switch error interrupt. */
#define SMARTFUSION2_IRQ_COMM_SWITCH_ERR  24
/** @brief Cache error interrupt. */
#define SMARTFUSION2_IRQ_CACHE_ERROR      25
/** @brief DDR subsystem interrupt. */
#define SMARTFUSION2_IRQ_DDR              26
/** @brief High-performance DMA completion interrupt. */
#define SMARTFUSION2_IRQ_HPDMA_COMPLETE   27
/** @brief High-performance DMA error interrupt. */
#define SMARTFUSION2_IRQ_HPDMA_ERROR      28
/** @brief ECC error interrupt. */
#define SMARTFUSION2_IRQ_ECC_ERROR        29
/** @brief MDDR I/O calibration interrupt. */
#define SMARTFUSION2_IRQ_MDDR_IO_CALIB    30
/** @brief Fabric PLL lock interrupt. */
#define SMARTFUSION2_IRQ_FAB_PLL_LOCK     31
/** @brief Fabric PLL lock-lost interrupt. */
#define SMARTFUSION2_IRQ_FAB_PLL_LOCK_LOST 32
/** @brief FIC64 interrupt. */
#define SMARTFUSION2_IRQ_FIC64            33
/** @brief GPIO0 interrupt. */
#define SMARTFUSION2_IRQ_GPIO0            50

/** @brief Legacy alias for watchdog wakeup interrupt. */
#define M2S_IRQ_WDOG                      SMARTFUSION2_IRQ_WDOG_WAKEUP
/** @brief Legacy alias for RTC wakeup interrupt. */
#define M2S_IRQ_RTC_WAKEUP                SMARTFUSION2_IRQ_RTC_WAKEUP
/** @brief Legacy alias for RTC match interrupt. */
#define M2S_IRQ_RTC_MATCH                 SMARTFUSION2_IRQ_RTC_WAKEUP
/** @brief Legacy alias for SPI0 interrupt. */
#define M2S_IRQ_SPI0                      SMARTFUSION2_IRQ_SPI0
/** @brief Legacy alias for SPI1 interrupt. */
#define M2S_IRQ_SPI1                      SMARTFUSION2_IRQ_SPI1
/** @brief Legacy alias for I2C0 main interrupt. */
#define M2S_IRQ_I2C0_MAIN                 SMARTFUSION2_IRQ_I2C0
/** @brief Legacy alias for I2C0 alert interrupt. */
#define M2S_IRQ_I2C0_ALERT                SMARTFUSION2_IRQ_I2C0
/** @brief Legacy alias for I2C0 suspend interrupt. */
#define M2S_IRQ_I2C0_SUS                  SMARTFUSION2_IRQ_I2C0
/** @brief Legacy alias for I2C1 main interrupt. */
#define M2S_IRQ_I2C1_MAIN                 SMARTFUSION2_IRQ_I2C1
/** @brief Legacy alias for I2C1 alert interrupt. */
#define M2S_IRQ_I2C1_ALERT                SMARTFUSION2_IRQ_I2C1
/** @brief Legacy alias for I2C1 suspend interrupt. */
#define M2S_IRQ_I2C1_SUS                  SMARTFUSION2_IRQ_I2C1
/** @brief Legacy alias for UART0 interrupt. */
#define M2S_IRQ_UART0                     SMARTFUSION2_IRQ_UART0
/** @brief Legacy alias for UART1 interrupt. */
#define M2S_IRQ_UART1                     SMARTFUSION2_IRQ_UART1
/** @brief Legacy alias for Ethernet MAC interrupt. */
#define M2S_IRQ_MAC0                      SMARTFUSION2_IRQ_ETHERNET_MAC
/** @brief Legacy alias for peripheral DMA interrupt. */
#define M2S_IRQ_DMA                       SMARTFUSION2_IRQ_PDMA
/** @brief Legacy alias for Timer 1 interrupt. */
#define M2S_IRQ_TIMER1                    SMARTFUSION2_IRQ_TIMER1
/** @brief Legacy alias for Timer 2 interrupt. */
#define M2S_IRQ_TIMER2                    SMARTFUSION2_IRQ_TIMER2
/** @brief Legacy alias for CAN0 interrupt. */
#define M2S_IRQ_CAN0                      SMARTFUSION2_IRQ_CAN
/** @brief Legacy alias for CAN1 interrupt. */
#define M2S_IRQ_CAN1                      SMARTFUSION2_IRQ_CAN
/** @brief Legacy alias for USB controller interrupt. */
#define M2S_IRQ_USB_MC                    SMARTFUSION2_IRQ_USB
/** @brief Legacy alias for USB DMA interrupt. */
#define M2S_IRQ_USB_DMA                   SMARTFUSION2_IRQ_USB_DMA
/** @brief Legacy alias for GPIO0 interrupt. */
#define M2S_IRQ_GPIO0                     SMARTFUSION2_IRQ_GPIO0

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_MICROCHIP_SMARTFUSION2_IRQ_H_ */
