/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC extensions of cpu_idle.S for the Nordic Semiconductor nRF53 processors family.
 */


#if defined(_ASMLANGUAGE)

#if defined(CONFIG_SOC_NRF53_ANOMALY_168_WORKAROUND_FOR_EXECUTION_FROM_RAM)
#define SOC_ON_EXIT_CPU_IDLE \
	.rept 26; \
	nop; \
	.endr
#elif defined(CONFIG_SOC_NRF53_ANOMALY_168_WORKAROUND)
#define SOC_ON_EXIT_CPU_IDLE \
	.rept 8; \
	nop; \
	.endr
#endif

#endif /* _ASMLANGUAGE */
