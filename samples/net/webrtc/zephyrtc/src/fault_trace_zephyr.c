/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "fault_trace_zephyr.h"

#include <stdint.h>
#if defined(CONFIG_ARM)
#include <zephyr/arch/cpu.h>
#endif
#include <zephyr/fatal.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/printk.h>

struct fault_snapshot_zephyr {
	uint32_t magic;
	uint32_t reason;
	uintptr_t esf;
	uintptr_t thread;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
	uint32_t msp;
	uint32_t psp;
	uint32_t control;
	uint32_t primask;
	uint32_t basepri;
	uint32_t faultmask;
	uint32_t cfsr;
	uint32_t hfsr;
	uint32_t mmfar;
	uint32_t bfar;
	uint32_t shcsr;
	uint32_t icsr;
};

/* DTCM keeps the snapshot outside the caches; native_sim has no DTCM. */
#if defined(CONFIG_ARM)
#define FAULT_SNAPSHOT_SECTION __dtcm_data_section
#else
#define FAULT_SNAPSHOT_SECTION
#endif

volatile struct fault_snapshot_zephyr fault_snapshot_zephyr FAULT_SNAPSHOT_SECTION;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
#if defined(CONFIG_ARM)
	const uintptr_t msp = __get_MSP();
	const uintptr_t psp = __get_PSP();

	fault_snapshot_zephyr.magic = UINT32_C(0x4641554c);
	fault_snapshot_zephyr.reason = reason;
	fault_snapshot_zephyr.esf = (uintptr_t)esf;
	fault_snapshot_zephyr.thread = (uintptr_t)k_current_get();
	if (esf != NULL) {
		fault_snapshot_zephyr.r0 = esf->basic.r0;
		fault_snapshot_zephyr.r1 = esf->basic.r1;
		fault_snapshot_zephyr.r2 = esf->basic.r2;
		fault_snapshot_zephyr.r3 = esf->basic.r3;
		fault_snapshot_zephyr.r12 = esf->basic.r12;
		fault_snapshot_zephyr.lr = esf->basic.lr;
		fault_snapshot_zephyr.pc = esf->basic.pc;
		fault_snapshot_zephyr.xpsr = esf->basic.xpsr;
	}
	fault_snapshot_zephyr.msp = msp;
	fault_snapshot_zephyr.psp = psp;
	fault_snapshot_zephyr.control = __get_CONTROL();
	fault_snapshot_zephyr.primask = __get_PRIMASK();
	fault_snapshot_zephyr.basepri = __get_BASEPRI();
	fault_snapshot_zephyr.faultmask = __get_FAULTMASK();
	fault_snapshot_zephyr.cfsr = SCB->CFSR;
	fault_snapshot_zephyr.hfsr = SCB->HFSR;
	fault_snapshot_zephyr.mmfar = SCB->MMFAR;
	fault_snapshot_zephyr.bfar = SCB->BFAR;
	fault_snapshot_zephyr.shcsr = SCB->SHCSR;
	fault_snapshot_zephyr.icsr = SCB->ICSR;
	__DSB();

	printk("event=fault reason=%u esf=%p thread=%p\n", reason, (const void *)esf,
	       (const void *)k_current_get());
	if (esf != NULL) {
		printk("event=fault_regs r0=%08x r1=%08x r2=%08x r3=%08x\n",
		       (unsigned int)esf->basic.r0, (unsigned int)esf->basic.r1,
		       (unsigned int)esf->basic.r2, (unsigned int)esf->basic.r3);
		printk("event=fault_regs r12=%08x lr=%08x pc=%08x xpsr=%08x\n",
		       (unsigned int)esf->basic.r12, (unsigned int)esf->basic.lr,
		       (unsigned int)esf->basic.pc, (unsigned int)esf->basic.xpsr);
	}
	printk("event=fault_state msp=%08x psp=%08x control=%08x primask=%08x "
	       "basepri=%08x faultmask=%08x\n",
	       (unsigned int)msp, (unsigned int)psp, (unsigned int)__get_CONTROL(),
	       (unsigned int)__get_PRIMASK(), (unsigned int)__get_BASEPRI(),
	       (unsigned int)__get_FAULTMASK());
	printk("event=fault_scb cfsr=%08x hfsr=%08x mmfar=%08x bfar=%08x "
	       "shcsr=%08x icsr=%08x\n",
	       (unsigned int)SCB->CFSR, (unsigned int)SCB->HFSR, (unsigned int)SCB->MMFAR,
	       (unsigned int)SCB->BFAR, (unsigned int)SCB->SHCSR, (unsigned int)SCB->ICSR);
#else
	printk("event=fault reason=%u esf=%p thread=%p\n", reason, (const void *)esf,
	       (const void *)k_current_get());
#endif
	k_fatal_halt(reason);
	CODE_UNREACHABLE;
}
