/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline void cpu_sleep(void)
{
#if defined(CONFIG_CPU_CORTEX_M0) || \
	defined(CONFIG_CPU_CORTEX_M4) || \
	defined(CONFIG_CPU_CORTEX_M33)
	__WFE();
	__SEV();
	__WFE();
#endif
}

static inline void cpu_dsb(void)
{
#if defined(CONFIG_CPU_CORTEX_M0) || defined(CONFIG_ARCH_POSIX)
	/* No need of data synchronization barrier */
#elif defined(CONFIG_CPU_CORTEX_M4) || defined(CONFIG_CPU_CORTEX_M33)
	__DSB();
#else
#error "Unsupported CPU."
#endif
}
