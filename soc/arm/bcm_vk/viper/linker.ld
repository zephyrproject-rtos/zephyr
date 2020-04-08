/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>

#if defined(CONFIG_SOC_BCM58402_M7)
#include <linker_m7.ld>
#elif defined(CONFIG_SOC_BCM58402_A72)
#include <linker_a72.ld>
#endif
