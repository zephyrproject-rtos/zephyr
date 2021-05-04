/*
 * Copyright (c) 2019,2020 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#include <tfm_ns_interface.h>

/**
 * @file @brief Zephyr's TF-M NS interface implementation
 *
 */


/* Global mutex to be used by the TF-M NS dispatcher, preventing
 * the Non-Secure application from initiating multiple parallel
 * TF-M secure calls.
 */
K_MUTEX_DEFINE(tfm_mutex);

int32_t tfm_ns_interface_dispatch(veneer_fn fn,
				  uint32_t arg0, uint32_t arg1,
				  uint32_t arg2, uint32_t arg3)
{
	int32_t result;

	/* TF-M request protected by NS lock */
	if (k_mutex_lock(&tfm_mutex, K_FOREVER) != 0) {
		return (int32_t)TFM_ERROR_GENERIC;
	}

#if !defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS)
	/*
	 * Prevent the thread from being preempted, while executing a Secure
	 * function. This is required to prevent system crashes that could
	 * occur, if a thead context switch is triggered in the middle of a
	 * Secure call.
	 */
	k_sched_lock();
#endif

#if defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS) && defined(CONFIG_FPU_SHARING)
	uint32_t fp_ctx_caller_saved[16];
	uint32_t fp_ctx_callee_saved[16];
	uint32_t fp_ctx_FPSCR;
	bool context_saved = false;
	uint32_t CONTROL = __get_CONTROL();

	if (CONTROL & CONTROL_FPCA_Msk) {
		/* Store caller-saved and callee-saved FP registers. */
		__asm__ volatile(
			"vstmia %0, {s0-s15}\n"
			"vstmia %1, {s16-s31}\n"
			:: "r" (fp_ctx_caller_saved), "r" (fp_ctx_callee_saved) :
		);

		fp_ctx_FPSCR = __get_FPSCR();
		context_saved = true;

		/* Disable FPCA so no stacking of FP registers happens in TFM. */
		__set_CONTROL(CONTROL & ~CONTROL_FPCA_Msk);

		/* ISB is recommended after setting CONTROL. It's not needed
		 * here though, since FPCA should have no impact on instruction
		 * fetching.
		 */
	}
#endif /* defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS) && defined(CONFIG_FPU_SHARING) */

	result = fn(arg0, arg1, arg2, arg3);

#if defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS) && defined(CONFIG_FPU_SHARING)
	if (context_saved) {
		/* Set FPCA first so it is set even if an interrupt happens
		 * during restoration.
		 */
		__set_CONTROL(__get_CONTROL() | CONTROL_FPCA_Msk);

		/* Restore FP state. */
		__set_FPSCR(fp_ctx_FPSCR);

		__asm__ volatile(
			"vldmia %0, {s0-s15}\n"
			"vldmia %0, {s16-s31}\n"
			:: "r" (fp_ctx_caller_saved), "r" (fp_ctx_callee_saved) :
		);
	}
#endif /* defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS) && defined(CONFIG_FPU_SHARING) */

#if !defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS)
	/* Unlock the scheduler, to allow the thread to be preempted. */
	k_sched_unlock();
#endif

	k_mutex_unlock(&tfm_mutex);

	return result;
}

enum tfm_status_e tfm_ns_interface_init(void)
{
	/*
	 * The static K_MUTEX_DEFINE handles mutex initialization,
	 * so this function may be implemented as no-op.
	 */
	return TFM_SUCCESS;
}


#if defined(TFM_PSA_API)
#include "psa_manifest/sid.h"
#endif /* TFM_PSA_API */

static int ns_interface_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	__ASSERT(tfm_ns_interface_init() == TFM_SUCCESS,
		"TF-M NS interface init failed");

	return 0;
}

/* Initialize the TFM NS interface */
SYS_INIT(ns_interface_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
