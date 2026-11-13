/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <tfm_platform_api.h>
#include <tfm_ns_interface_zephyr.h>

#if defined(TFM_PSA_API)
#include <psa_manifest/sid.h>
#endif /* TFM_PSA_API */

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 * The function requests Trusted-Firmware-M to reset the processor,
 * on behalf of the Non-Secure application. The function overrides
 * the weak implementation of sys_arch_reboot() in scb.c.
 *
 * \pre The implementation requires the TFM_PARTITION_PLATFORM be defined.
 */

#if defined(CONFIG_TFM_PARTITION_PLATFORM)
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	(void)tfm_platform_system_reset();
}

/**
 *
 * @brief Prepare for the reset request
 *
 * sys_arch_reboot() above is a Secure call: tfm_platform_system_reset() goes
 * through tfm_ns_interface_dispatch(), which takes the NS interface lock.
 * sys_reboot() calls sys_arch_reboot() with interrupts locked, where blocking
 * on that lock is not allowed (and asserts with CONFIG_SPIN_VALIDATE) should
 * another thread be in the middle of a Secure call at that time.
 *
 * Take the lock here instead, while the scheduler is still running, and keep
 * it: the system is about to reset, and the nested lock taken later by the
 * dispatcher on behalf of this thread succeeds without blocking. The wait has
 * no timeout on purpose. If the holder never returns from the Secure world
 * there is no safe way to request the reset from TF-M anyway, as entering the
 * Secure world again while another call is suspended in it is undefined.
 *
 * In ISR and pre-kernel context Secure calls bypass the lock, so the reset
 * request goes straight through as before, on a best effort basis.
 */
void sys_arch_reboot_prepare(int type)
{
	ARG_UNUSED(type);

	tfm_ns_interface_lock_forever();
}
#endif /* CONFIG_TFM_PARTITION_PLATFORM */
