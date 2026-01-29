/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_RISCV_CUSTOM_H
#define ZEPHYR_RISCV_CUSTOM_H

#include <zephyr/autoconf.h>
#include <soc.h>

/* clang-format off */

/*
 * xuantie core mstatus register definition
 * 1. C906FDV,C920,R920:
 *    It is an early implemented CPU and is not fully compatible with the RISC-V standard
 * 2. The implementation specific to the Xuantie extension
 **/

#if CONFIG_CPU_XUANTIE_C906 || CONFIG_CPU_XUANTIE_C906FD \
	|| CONFIG_CPU_XUANTIE_C906FDV || CONFIG_CPU_XUANTIE_C910 \
	|| CONFIG_CPU_XUANTIE_C920 || CONFIG_CPU_XUANTIE_R920

#define MSTATUS_VS_OFF   (0UL << 23)
#define MSTATUS_VS_INIT  (1UL << 23)
#define MSTATUS_VS_CLEAN (2UL << 23)
#define MSTATUS_VS_DIRTY (3UL << 23)
#else
#define MSTATUS_VS_OFF   (0UL << 9)
#define MSTATUS_VS_INIT  (1UL << 9)
#define MSTATUS_VS_CLEAN (2UL << 9)
#define MSTATUS_VS_DIRTY (3UL << 9)
#endif

#define MSTATUS_MS_OFF   (0UL << 25)
#define MSTATUS_MS_INIT  (1UL << 25)
#define MSTATUS_MS_CLEAN (2UL << 25)
#define MSTATUS_MS_DIRTY (3UL << 25)

/*
 * Support for soc extension extraction for core/thread.c
 * to prevent direct source code intrusion into public areas
 */
#define CUSTOM_INIT_THREAD_MSTATUS(stack_init)
#if CONFIG_CPU_XUANTIE_C906FDV || CONFIG_CPU_XUANTIE_C920 || CONFIG_CPU_XUANTIE_R920
	#undef CUSTOM_INIT_THREAD_MSTATUS
	/* vxsat、vxrm controlled by FS on 0.7.1 version */
	#define CUSTOM_INIT_THREAD_MSTATUS(stack_init) \
	do { stack_init->mstatus |= MSTATUS_VS_INIT | MSTATUS_FS_INIT; } while (false);
#else
	#undef CUSTOM_INIT_THREAD_MSTATUS
	#define CUSTOM_INIT_THREAD_MSTATUS(stack_init) \
	do { stack_init->mstatus |= MSTATUS_VS_INIT; } while (false);
#endif


/*
 *  The xuantie kernel entry is__xuantie_start, not__start.
 *  It has not been perfectly merged with the__start public by riscv for the time being.
 */

#define __start __xuantie_start

/*
 * Should riscv_soc_init_cpu be here?
 * And is it too intrusive and coupled to use it directly in smp.c?
 */

#ifndef __ASSEMBLER__
extern void riscv_soc_init_cpu(int cpu_num);
#endif

/*
 * Clock correction is applied to CYC_PER_TICK in riscv_machine_timer.c
 */

#undef  CYC_PER_TICK_CUSTOM_EXPANSION
#define CYC_PER_TICK_CUSTOM_EXPANSION / (1 + CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER)

/* clang-format on */

#endif /* ZEPHYR_RISCV_CUSTOM_H */
