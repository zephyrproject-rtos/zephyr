/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_MSS_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_MSS_CLOCK_H_

/**
 * @file
 * @brief Devicetree clock IDs for Microchip MPFS MSS clocks.
 */

/** @brief Peripheral gate ID for ENVM clock. */
#define MPFS_MSS_CLK_ENVM    0x00U
/** @brief Peripheral gate ID for MAC0 clock. */
#define MPFS_MSS_CLK_MAC0    0x01U
/** @brief Peripheral gate ID for MAC1 clock. */
#define MPFS_MSS_CLK_MAC1    0x02U
/** @brief Peripheral gate ID for MMC clock. */
#define MPFS_MSS_CLK_MMC     0x03U
/** @brief Peripheral gate ID for TIMER clock. */
#define MPFS_MSS_CLK_TIMER   0x04U
/** @brief Peripheral gate ID for MMUART0 clock. */
#define MPFS_MSS_CLK_MMUART0 0x05U
/** @brief Peripheral gate ID for MMUART1 clock. */
#define MPFS_MSS_CLK_MMUART1 0x06U
/** @brief Peripheral gate ID for MMUART2 clock. */
#define MPFS_MSS_CLK_MMUART2 0x07U
/** @brief Peripheral gate ID for MMUART3 clock. */
#define MPFS_MSS_CLK_MMUART3 0x08U
/** @brief Peripheral gate ID for MMUART4 clock. */
#define MPFS_MSS_CLK_MMUART4 0x09U
/** @brief Peripheral gate ID for SPI0 clock. */
#define MPFS_MSS_CLK_SPI0    0x0AU
/** @brief Peripheral gate ID for SPI1 clock. */
#define MPFS_MSS_CLK_SPI1    0x0BU
/** @brief Peripheral gate ID for I2C0 clock. */
#define MPFS_MSS_CLK_I2C0    0x0CU
/** @brief Peripheral gate ID for I2C1 clock. */
#define MPFS_MSS_CLK_I2C1    0x0DU
/** @brief Peripheral gate ID for CAN0 clock. */
#define MPFS_MSS_CLK_CAN0    0x0EU
/** @brief Peripheral gate ID for CAN1 clock. */
#define MPFS_MSS_CLK_CAN1    0x0FU
/** @brief Peripheral gate ID for USB clock. */
#define MPFS_MSS_CLK_USB     0x10U
/** @brief Peripheral gate ID for RTC clock. */
#define MPFS_MSS_CLK_RTC     0x12U
/** @brief Peripheral gate ID for QSPI clock. */
#define MPFS_MSS_CLK_QSPI    0x13U
/** @brief Peripheral gate ID for GPIO0 clock. */
#define MPFS_MSS_CLK_GPIO0   0x14U
/** @brief Peripheral gate ID for GPIO1 clock. */
#define MPFS_MSS_CLK_GPIO1   0x15U
/** @brief Peripheral gate ID for GPIO2 clock. */
#define MPFS_MSS_CLK_GPIO2   0x16U
/** @brief Peripheral gate ID for DDRC clock. */
#define MPFS_MSS_CLK_DDRC    0x17U
/** @brief Peripheral gate ID for FIC0 clock. */
#define MPFS_MSS_CLK_FIC0    0x18U
/** @brief Peripheral gate ID for FIC1 clock. */
#define MPFS_MSS_CLK_FIC1    0x19U
/** @brief Peripheral gate ID for FIC2 clock. */
#define MPFS_MSS_CLK_FIC2    0x1AU
/** @brief Peripheral gate ID for FIC3 clock. */
#define MPFS_MSS_CLK_FIC3    0x1BU
/** @brief Peripheral gate ID for ATHENA clock. */
#define MPFS_MSS_CLK_ATHENA  0x1CU
/** @brief Peripheral gate ID for CFM clock. */
#define MPFS_MSS_CLK_CFM     0x1DU

/** @brief Unknown clock class identifier. */
#define MPFS_CLK_UNKNOWN_CLK 0
/** @brief CPU derived clock class identifier. */
#define MPFS_CLK_CPU_CLK     1
/** @brief AXI derived clock class identifier. */
#define MPFS_CLK_AXI_CLK     2
/** @brief AHB/APB derived clock class identifier. */
#define MPFS_CLK_AHB_APB_CLK 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCHP_MSS_CLOCK_H_ */
