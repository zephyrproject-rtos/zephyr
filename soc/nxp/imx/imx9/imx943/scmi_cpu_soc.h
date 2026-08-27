/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NXP_IMX943_SCMI_CPU_SOC_H_
#define ZEPHYR_NXP_IMX943_SCMI_CPU_SOC_H_

#define CPU_IDX_M33P            0U
#define CPU_IDX_M7P_0           1U
#define CPU_IDX_A55C0           2U
#define CPU_IDX_A55C1           3U
#define CPU_IDX_A55C2           4U
#define CPU_IDX_A55C3           5U
#define CPU_IDX_A55P            6U
#define CPU_IDX_M7P_1           7U
#define CPU_IDX_M33P_S          8U

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

#define PWR_NUM_MIX_SLICE               19U

#define PWR_MIX_SLICE_IDX_ANA           0U
#define PWR_MIX_SLICE_IDX_AON           1U
#define PWR_MIX_SLICE_IDX_BBSM          2U
#define PWR_MIX_SLICE_IDX_M7_1          3U
#define PWR_MIX_SLICE_IDX_CCMSRCGPC     4U
#define PWR_MIX_SLICE_IDX_A55C0         5U
#define PWR_MIX_SLICE_IDX_A55C1         6U
#define PWR_MIX_SLICE_IDX_A55C2         7U
#define PWR_MIX_SLICE_IDX_A55C3         8U
#define PWR_MIX_SLICE_IDX_A55P          9U
#define PWR_MIX_SLICE_IDX_DDR           10U
#define PWR_MIX_SLICE_IDX_DISPLAY       11U
#define PWR_MIX_SLICE_IDX_M7_0          12U
#define PWR_MIX_SLICE_IDX_HSIO_TOP      13U
#define PWR_MIX_SLICE_IDX_HSIO_WAON     14U
#define PWR_MIX_SLICE_IDX_NETC          15U
#define PWR_MIX_SLICE_IDX_NOC           16U
#define PWR_MIX_SLICE_IDX_NPU           17U
#define PWR_MIX_SLICE_IDX_WAKEUP        18U

#define PWR_NUM_MEM_SLICE               17U

#define PWR_MEM_SLICE_IDX_AON           0U
#define PWR_MEM_SLICE_IDX_M7_1          1U
#define PWR_MEM_SLICE_IDX_A55C0         2U
#define PWR_MEM_SLICE_IDX_A55C1         3U
#define PWR_MEM_SLICE_IDX_A55C2         4U
#define PWR_MEM_SLICE_IDX_A55C3         5U
#define PWR_MEM_SLICE_IDX_A55P          6U
#define PWR_MEM_SLICE_IDX_A55L3         7U
#define PWR_MEM_SLICE_IDX_DDR           8U
#define PWR_MEM_SLICE_IDX_DISPLAY       9U
#define PWR_MEM_SLICE_IDX_M7_0          10U
#define PWR_MEM_SLICE_IDX_HSIO          11U
#define PWR_MEM_SLICE_IDX_NETC          12U
#define PWR_MEM_SLICE_IDX_NOC1          13U
#define PWR_MEM_SLICE_IDX_NOC2          14U
#define PWR_MEM_SLICE_IDX_NPU           15U
#define PWR_MEM_SLICE_IDX_WAKEUP        16U

#endif /* ZEPHYR_NXP_IMX943_SCMI_CPU_SOC_H_ */
