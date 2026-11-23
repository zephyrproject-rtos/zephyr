/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/rh850/sysreg.h>
#include <zephyr/sys/util.h>
#include <kernel_arch_func.h>

void z_rh850_fxu_save(uint8_t *wr, uint32_t *sr);
void z_rh850_fxu_load(const uint8_t *wr, const uint32_t *sr);

static uint8_t *fxu_wr(struct k_thread *thread)
{
	return (uint8_t *)ROUND_UP((uintptr_t)thread->arch.fxu_wr, 16U);
}

void z_rh850_fxu_switch(struct k_thread *old, struct k_thread *new)
{
	if (old == new) {
		return;
	}

	if (old->arch.fxu_user != 0U) {
		z_rh850_fxu_save(fxu_wr(old), old->arch.fxu_sr);
	}

	if (new->arch.fxu_user != 0U) {
		z_rh850_fxu_load(fxu_wr(new), new->arch.fxu_sr);
	}
}

void z_rh850_fxu_arch_switch(void **switched_from)
{
	z_rh850_fxu_switch(CONTAINER_OF(switched_from, struct k_thread, switch_handle), _current);
}

void z_rh850_ucpop(struct arch_esf *esf, uint32_t cause, uint32_t nested)
{
	struct k_thread *thread = _current;

	if ((cause & 0xffffU) != RH850_FEIC_UCPOP_FXU || nested != 0U || thread == NULL ||
	    thread->arch.fxu_user != 0U) {
		z_rh850_fault(esf, cause, RH850_EXC_LEVEL_FE);
		return;
	}

	(void)memset(thread->arch.fxu_wr, 0, sizeof(thread->arch.fxu_wr));
	thread->arch.fxu_sr[0] = RH850_FXSR_INIT;
	thread->arch.fxu_sr[1] = 0U;
	thread->arch.fxu_user = 1U;
	esf->eipsw |= RH850_PSW_CU1;
	z_rh850_fxu_load(fxu_wr(thread), thread->arch.fxu_sr);
}

/* Thread abort and exit */
void z_rh850_fxu_disable(struct k_thread *thread)
{
	unsigned int key = arch_irq_lock();

	thread->arch.fxu_user = 0U;

	if (thread == _current && !arch_is_in_isr()) {
		rh850_sysreg_write(RH850_SR_PSW, rh850_sysreg_read(RH850_SR_PSW) & ~RH850_PSW_CU1);
		__asm__ volatile("synci" ::: "memory");
	}

	arch_irq_unlock(key);
}
