/*
 * Copyright (c) 2019,2020 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>

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
	bool isr_mode = k_is_in_isr() || k_is_pre_kernel();
	int tfm_ns_saved_prio;
	int32_t result;

	if (!isr_mode) {
		/* TF-M request protected by NS lock */
		if (k_mutex_lock(&tfm_mutex, K_FOREVER) != 0) {
			return (int32_t)PSA_ERROR_GENERIC_ERROR;
		}

#if !defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS)
		/* Prevent the thread from being preempted, while executing a
		 * Secure function. This is required to prevent system crashes
		 * that could occur, if a thread context switch is triggered in
		 * the middle of a Secure call. Note that the code below takes
		 * into account MetaIRQ, which can preempt cooperative threads
		 * of any priority.
		 */
		tfm_ns_saved_prio = k_thread_priority_get(k_current_get());
		k_thread_priority_set(k_current_get(), K_HIGHEST_THREAD_PRIO);
#else
		ARG_UNUSED(tfm_ns_saved_prio);
#endif
	}

	struct fpu_ctx_full context_buffer;

	z_arm_save_fp_context(&context_buffer);

	result = fn(arg0, arg1, arg2, arg3);

	z_arm_restore_fp_context(&context_buffer);

	if (!isr_mode) {
#if !defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS)
		/* Restore thread priority, to allow the thread to be preempted. */
		k_thread_priority_set(k_current_get(), tfm_ns_saved_prio);
#endif

		k_mutex_unlock(&tfm_mutex);
	}

	return result;
}

uint32_t tfm_ns_interface_init(void)
{
	/*
	 * The static K_MUTEX_DEFINE handles mutex initialization,
	 * so this function may be implemented as no-op.
	 */
	return PSA_SUCCESS;
}


#if defined(TFM_PSA_API)
#include "psa_manifest/sid.h"
#endif /* TFM_PSA_API */

static int ns_interface_init(void)
{

	__ASSERT(tfm_ns_interface_init() == PSA_SUCCESS,
		"TF-M NS interface init failed");

	return 0;
}

/* Initialize the TFM NS interface */
SYS_INIT(ns_interface_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
