/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>
#include <zsr.h>

/**
 * @brief Read a special register.
 *
 * @param sr Name of special register.
 *
 * @return Value of special register.
 */
#define XTENSA_RSR(sr) \
	({uint32_t v; \
	 __asm__ volatile ("rsr." sr " %0" : "=a"(v)); \
	 v; })

/**
 * @brief Write to a special register.
 *
 * @param sr Name of special register.
 * @param v Value to be written to special register.
 */
#define XTENSA_WSR(sr, v) \
	do { \
		__asm__ volatile ("wsr." sr " %0" : : "r"(v)); \
	} while (false)

/**
 * @brief Read a user register.
 *
 * @param ur Name of user register.
 *
 * @return Value of user register.
 */
#define XTENSA_RUR(ur) \
	({uint32_t v; \
	 __asm__ volatile ("rur." ur " %0" : "=a"(v)); \
	 v; })

/**
 * @brief Write to a user register.
 *
 * @param ur Name of user register.
 * @param v Value to be written to user register.
 */
#define XTENSA_WUR(ur, v) \
	do { \
		__asm__ volatile ("wur." ur " %0" : : "r"(v)); \
	} while (false)

/** Implementation of @ref arch_curr_cpu. */
static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	_cpu_t *cpu;

	cpu = (_cpu_t *)XTENSA_RSR(ZSR_CPU_STR);

	return cpu;
}

/** Implementation of @ref arch_proc_id. */
static ALWAYS_INLINE uint32_t arch_proc_id(void)
{
	uint32_t prid;

	__asm__ volatile("rsr %0, PRID" : "=r"(prid));
	return prid;
}

#ifdef CONFIG_SOC_HAS_RUNTIME_NUM_CPUS
extern unsigned int soc_num_cpus;
#endif

/** Implementation of @ref arch_num_cpus. */
static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
#ifdef CONFIG_SOC_HAS_RUNTIME_NUM_CPUS
	return soc_num_cpus;
#else
	return CONFIG_MP_MAX_NUM_CPUS;
#endif
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_INLINES_H_ */
