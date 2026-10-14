/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/kernel.h>
#include <cmsis_core.h>
#include <ironside_client/integration.h>

static K_MUTEX_DEFINE(client_mutex);

#if defined(CONFIG_FPU_SHARING)
/*
 * The hooks share no stack frame, so the FP context saved around a call lives
 * here. The mutex keeps the thread copy to one caller at a time. An ISR-mode
 * caller keeps its copy to itself by masking interrupts for the duration of its
 * call. It needs one only when the FPU is active, which for an interrupt handler
 * means it has used the FPU itself, as exception entry clears FPCA.
 */
static struct fpu_ctx_full thread_fpu_ctx;
static struct fpu_ctx_full isr_fpu_ctx;
static unsigned int isr_irq_key;
#endif

/* Interrupt and pre-kernel callers alike cannot block on the mutex. */
static bool isr_mode(void)
{
	return k_is_in_isr() || k_is_pre_kernel();
}

void ironside_client_lock_acquire(void)
{
	if (!isr_mode()) {
		(void)k_mutex_lock(&client_mutex, K_FOREVER);
#if defined(CONFIG_FPU_SHARING)
		z_arm_save_fp_context(&thread_fpu_ctx);
#endif
		return;
	}

#if defined(CONFIG_FPU_SHARING)
	if (__get_CONTROL() & CONTROL_FPCA_Msk) {
		isr_irq_key = irq_lock();
		z_arm_save_fp_context(&isr_fpu_ctx);
	}
#endif
}

void ironside_client_lock_release(void)
{
	if (!isr_mode()) {
#if defined(CONFIG_FPU_SHARING)
		z_arm_restore_fp_context(&thread_fpu_ctx);
		/*
		 * A save with the FPU inactive leaves ctx_saved as is, so clear it
		 * or the next caller without FP state restores this one's.
		 */
		thread_fpu_ctx.ctx_saved = false;
#endif
		(void)k_mutex_unlock(&client_mutex);
		return;
	}

#if defined(CONFIG_FPU_SHARING)
	/* Only this caller can have set it: it did so with interrupts masked. */
	if (isr_fpu_ctx.ctx_saved) {
		z_arm_restore_fp_context(&isr_fpu_ctx);
		isr_fpu_ctx.ctx_saved = false;
		irq_unlock(isr_irq_key);
	}
#endif
}
