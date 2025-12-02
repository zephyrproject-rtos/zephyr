/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NXP_IMX943_PM_MCORE_H_
#define ZEPHYR_NXP_IMX943_PM_MCORE_H_

#ifdef CONFIG_SOC_MIMX94398_M33
#define cpu_idx	CPU_IDX_M33P_S
#define pwr_mix_idx	PWR_MIX_SLICE_IDX_NETC;
#define pwr_mem_idx	PWR_MEM_SLICE_IDX_NETC
#define nvic_iser_nb	16
#elif	CONFIG_SOC_MIMX94398_M7_0
#define cpu_idx	CPU_IDX_M7P_0
#define pwr_mix_idx	PWR_MIX_SLICE_IDX_M7_0;
#define pwr_mem_idx	PWR_MEM_SLICE_IDX_M7_0
#define nvic_iser_nb	8
#else
#define cpu_idx	CPU_IDX_M7P_1
#define pwr_mix_idx	PWR_MIX_SLICE_IDX_M7_1;
#define pwr_mem_idx	PWR_MEM_SLICE_IDX_M7_1
#define nvic_iser_nb	8
#endif

#endif /* ZEPHYR_NXP_IMX943_PM_MCORE_H_ */
