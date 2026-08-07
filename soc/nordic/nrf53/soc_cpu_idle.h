/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC extensions of cpu_idle.S for the Nordic Semiconductor nRF53 processors family.
 */

#define SOC_ON_EXIT_CPU_IDLE_4 \
	__NOP(); \
	__NOP(); \
	__NOP(); \
	__NOP();
#define SOC_ON_EXIT_CPU_IDLE_8 \
	SOC_ON_EXIT_CPU_IDLE_4 \
	SOC_ON_EXIT_CPU_IDLE_4

#if defined(CONFIG_SOC_NRF53_ANOMALY_168_WORKAROUND_FOR_EXECUTION_FROM_RAM)
#define SOC_ON_EXIT_CPU_IDLE \
	SOC_ON_EXIT_CPU_IDLE_8; \
	SOC_ON_EXIT_CPU_IDLE_8; \
	SOC_ON_EXIT_CPU_IDLE_8; \
	__NOP(); \
	__NOP();
#elif defined(CONFIG_SOC_NRF53_ANOMALY_168_WORKAROUND)
#define SOC_ON_EXIT_CPU_IDLE SOC_ON_EXIT_CPU_IDLE_8
#endif
