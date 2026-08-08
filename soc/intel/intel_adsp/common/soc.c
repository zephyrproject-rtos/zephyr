/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <adsp_memory.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/section_tags.h>

#include <zephyr/arch/xtensa/xtensa_ptr.h>

extern void power_init(void);
extern void adsp_clock_init(void);

#if CONFIG_MP_MAX_NUM_CPUS > 1
extern void soc_mp_init(void);
#endif

extern char _imr_start[];
extern char _imr_end[];
extern char _end[];
extern char _heap_sentry[];
extern char _cached_start[];
extern char _cached_end[];

bool xtensa_soc_ptr_executable(const void *p)
{
	return (p >= (void *)__text_region_start && p <= (void *)__text_region_end) ||
		(p >= (void *)_imr_start && p <= (void *)_imr_end);
}

bool xtensa_soc_stack_ptr_is_sane(uint32_t sp)
{
	return ((char *)sp >= _end && (char *)sp <= _heap_sentry) ||
		((char *)sp >= _cached_start && (char *)sp <= _cached_end) ||
		(sp >= (IMR_BOOT_LDR_MANIFEST_BASE - CONFIG_ISR_STACK_SIZE)
		 && sp <= IMR_BOOT_LDR_MANIFEST_BASE);
}

static __imr int soc_init(void)
{
	power_init();

#ifdef CONFIG_ADSP_CLOCK
	adsp_clock_init();
#endif

#if CONFIG_MP_MAX_NUM_CPUS > 1
	soc_mp_init();
#endif

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
