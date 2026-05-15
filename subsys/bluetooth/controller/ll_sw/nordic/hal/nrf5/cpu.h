/*
 * Copyright (c) 2016-2021 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_RISCV)
#include <hal/nrf_vpr_clic.h>
#endif

static inline void cpu_sleep(void)
{
	k_cpu_atomic_idle(irq_lock());
}

static inline void cpu_dmb(void)
{
#if defined(CONFIG_CPU_CORTEX_M)
	/* NOTE: Refer to ARM Cortex-M Programming Guide to Memory Barrier
	 *       Instructions, Section 4.1 Normal access in memories
	 *
	 *       Implementation: In the Cortex-M processors data transfers are
	 *       carried out in the programmed order.
	 *
	 * Hence, there is no need to use a memory barrier instruction between
	 * each access. Only a compiler memory clobber is sufficient.
	 */
	__asm__ volatile ("" : : : "memory");
#elif defined(CONFIG_RISCV)
	__asm__ volatile ("" : : : "memory");
#elif defined(CONFIG_ARCH_POSIX)
	/* FIXME: Add necessary host machine required Data Memory Barrier
	 *        instruction alongwith the below defined compiler memory
	 *        clobber.
	 */
	__asm__ volatile ("" : : : "memory");
#else
#error "Unsupported CPU."
#endif
}

static inline void cpu_irq_pending_set(uint32_t irq)
{
#if defined(CONFIG_RISCV)
	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, irq);
#else
	NVIC_SetPendingIRQ(irq);
#endif
}

static inline void cpu_irq_pending_clear(uint32_t irq)
{
#if defined(CONFIG_RISCV)
	nrf_vpr_clic_int_pending_clear(NRF_VPRCLIC, irq);
#else
	NVIC_ClearPendingIRQ(irq);
#endif
}
