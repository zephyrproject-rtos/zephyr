/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC extensions of cpu_idle.S for the Nordic Semiconductor nRF53 processors family.
 */


#if defined(_ASMLANGUAGE)

#define SOC_ON_EXIT_CPU_IDLE \
	nop; \
	nop; \
	nop; \
	nop;

#endif /* _ASMLANGUAGE */
