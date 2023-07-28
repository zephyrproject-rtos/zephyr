/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

#include "tfm_platform_api.h"

#if defined(TFM_PSA_API)
#include "psa_manifest/sid.h"
#endif /* TFM_PSA_API */

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 * The function requests Trusted-Firmware-M to reset the processor,
 * on behalf of the Non-Secure application. The function overrides
 * the weak implementation of z_sys_reboot() in scb.c.
 *
 * \pre The implementation requires the TFM_PARTITION_PLATFORM be defined.
 */

void z_sys_reboot(enum sys_reboot_mode mode)
{
	ARG_UNUSED(mode);

	(void)tfm_platform_system_reset();

	CODE_UNREACHABLE;
}
