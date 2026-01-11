/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NXP_IMX95_SCMI_CPU_SOC_H_
#define ZEPHYR_NXP_IMX95_SCMI_CPU_SOC_H_

#define CPU_IDX_M33P        0U
#define CPU_IDX_M7P         1U
#define CPU_IDX_A55C0       2U
#define CPU_IDX_A55C1       3U
#define CPU_IDX_A55C2       4U
#define CPU_IDX_A55C3       5U
#define CPU_IDX_A55C4       6U
#define CPU_IDX_A55C5       7U
#define CPU_IDX_A55P        8U
#define CPU_NUM_IDX         9U

#define CPU_SLEEP_MODE_RUN      0U
#define CPU_SLEEP_MODE_WAIT     1U
#define CPU_SLEEP_MODE_STOP     2U
#define CPU_SLEEP_MODE_SUSPEND  3U

/*!
 * @name SCMI CPU LPM settings
 */
#define SCMI_CPU_LPM_SETTING_ON_NEVER          0U
#define SCMI_CPU_LPM_SETTING_ON_RUN            1U
#define SCMI_CPU_LPM_SETTING_ON_RUN_WAIT       2U
#define SCMI_CPU_LPM_SETTING_ON_RUN_WAIT_STOP  3U
#define SCMI_CPU_LPM_SETTING_ON_ALWAYS         4U

#define CPU_PER_LPI_IDX_GPIO1           0U
#define CPU_PER_LPI_IDX_GPIO2           1U
#define CPU_PER_LPI_IDX_GPIO3           2U
#define CPU_PER_LPI_IDX_GPIO4           3U
#define CPU_PER_LPI_IDX_GPIO5           4U
#define CPU_PER_LPI_IDX_CAN1            5U
#define CPU_PER_LPI_IDX_CAN2            6U
#define CPU_PER_LPI_IDX_CAN3            7U
#define CPU_PER_LPI_IDX_CAN4            8U
#define CPU_PER_LPI_IDX_CAN5            9U
#define CPU_PER_LPI_IDX_LPUART1         10U
#define CPU_PER_LPI_IDX_LPUART2         11U
#define CPU_PER_LPI_IDX_LPUART3         12U
#define CPU_PER_LPI_IDX_LPUART4         13U
#define CPU_PER_LPI_IDX_LPUART5         14U
#define CPU_PER_LPI_IDX_LPUART6         15U
#define CPU_PER_LPI_IDX_LPUART7         16U
#define CPU_PER_LPI_IDX_LPUART8         17U
#define CPU_PER_LPI_IDX_WDOG3           18U
#define CPU_PER_LPI_IDX_WDOG4           19U
#define CPU_PER_LPI_IDX_WDOG5           20U


/* MIX definitions */
#define PWR_NUM_MIX_SLICE               23U

#define PWR_MIX_SLICE_IDX_ANA           0U
#define PWR_MIX_SLICE_IDX_AON           1U
#define PWR_MIX_SLICE_IDX_BBSM          2U
#define PWR_MIX_SLICE_IDX_CAMERA        3U
#define PWR_MIX_SLICE_IDX_CCMSRCGPC     4U
#define PWR_MIX_SLICE_IDX_A55C0         5U
#define PWR_MIX_SLICE_IDX_A55C1         6U
#define PWR_MIX_SLICE_IDX_A55C2         7U
#define PWR_MIX_SLICE_IDX_A55C3         8U
#define PWR_MIX_SLICE_IDX_A55C4         9U
#define PWR_MIX_SLICE_IDX_A55C5         10U
#define PWR_MIX_SLICE_IDX_A55P          11U
#define PWR_MIX_SLICE_IDX_DDR           12U
#define PWR_MIX_SLICE_IDX_DISPLAY       13U
#define PWR_MIX_SLICE_IDX_GPU           14U
#define PWR_MIX_SLICE_IDX_HSIO_TOP      15U
#define PWR_MIX_SLICE_IDX_HSIO_WAON     16U
#define PWR_MIX_SLICE_IDX_M7            17U
#define PWR_MIX_SLICE_IDX_NETC          18U
#define PWR_MIX_SLICE_IDX_NOC           19U
#define PWR_MIX_SLICE_IDX_NPU           20U
#define PWR_MIX_SLICE_IDX_VPU           21U
#define PWR_MIX_SLICE_IDX_WAKEUP        22U

#define PWR_MEM_SLICE_IDX_AON           0U
#define PWR_MEM_SLICE_IDX_CAMERA        1U
#define PWR_MEM_SLICE_IDX_A55C0         2U
#define PWR_MEM_SLICE_IDX_A55C1         3U
#define PWR_MEM_SLICE_IDX_A55C2         4U
#define PWR_MEM_SLICE_IDX_A55C3         5U
#define PWR_MEM_SLICE_IDX_A55C4         6U
#define PWR_MEM_SLICE_IDX_A55C5         7U
#define PWR_MEM_SLICE_IDX_A55P          8U
#define PWR_MEM_SLICE_IDX_A55L3         9U
#define PWR_MEM_SLICE_IDX_DDR           10U
#define PWR_MEM_SLICE_IDX_DISPLAY       11U
#define PWR_MEM_SLICE_IDX_GPU           12U
#define PWR_MEM_SLICE_IDX_HSIO          13U
#define PWR_MEM_SLICE_IDX_M7            14U
#define PWR_MEM_SLICE_IDX_NETC          15U
#define PWR_MEM_SLICE_IDX_NOC1          16U
#define PWR_MEM_SLICE_IDX_NOC2          17U
#define PWR_MEM_SLICE_IDX_NPU           18U
#define PWR_MEM_SLICE_IDX_VPU           19U
#define PWR_MEM_SLICE_IDX_WAKEUP        20U

#endif /* ZEPHYR_NXP_IMX95_SCMI_CPU_SOC_H_ */
