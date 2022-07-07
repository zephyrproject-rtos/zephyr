/*
 * Copyright (c) 2019,2020 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/cortex_m/fpu.h>

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
	bool is_pre_kernel = k_is_pre_kernel();

	if (!is_pre_kernel) {
		/* TF-M request protected by NS lock */
		if (k_mutex_lock(&tfm_mutex, K_FOREVER) != 0) {
			return (int32_t)TFM_ERROR_GENERIC;
		}

#if !defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS)
		/* Prevent the thread from being preempted, while executing a
		 * Secure function. This is required to prevent system crashes
		 * that could occur, if a thead context switch is triggered in
		 * the middle of a Secure call.
		 */
		k_sched_lock();
#endif
	}

	struct fpu_ctx_full context_buffer;

	z_arm_save_fp_context(&context_buffer);

	result = fn(arg0, arg1, arg2, arg3);

	z_arm_restore_fp_context(&context_buffer);

	if (!is_pre_kernel) {
#if !defined(CONFIG_ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS)
		/* Unlock the scheduler, to allow the thread to be preempted. */
		k_sched_unlock();
#endif

		k_mutex_unlock(&tfm_mutex);
	}

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
