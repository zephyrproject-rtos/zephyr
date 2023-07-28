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

void z_sys_reboot(enum sys_reboot_mode mode)
{
	ARG_UNUSED(mode);

	(void)tfm_platform_system_reset();

	CODE_UNREACHABLE;
}
