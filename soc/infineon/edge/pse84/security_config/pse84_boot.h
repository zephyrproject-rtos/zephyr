/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <infineon_kconfig.h>
#include <cy_sysint.h>
#include <system_edge.h>
#include "cy_pdl.h"

#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
#include "partition_ARMCM33.h"
#include <zephyr/drivers/timer/system_timer.h>

#include "pse84_s_system.h"
#include "pse84_s_sau.h"
#include "pse84_s_protection.h"
#include "pse84_s_mpc.h"

#define CM55_BOOT_WAIT_TIME_USEC (10U)

void ifx_pse84_cm55_startup(void);
#endif
