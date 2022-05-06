/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>

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
#endif /* CONFIG_TFM_PARTITION_PLATFORM */
