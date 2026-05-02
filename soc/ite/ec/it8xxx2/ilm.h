/*
 * Copyright 2022 The ChromiumOS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

/* Places code in the section that gets mapped into ILM */
#ifdef CONFIG_SOC_IT8XXX2_RAM_CODE_NOINLINE
#define __soc_ram_code __attribute__((noinline)) __attribute__((section(".__ram_code")))
#else
#define __soc_ram_code __attribute__((section(".__ram_code")))
#endif

#ifndef _ASMLANGUAGE
bool it8xxx2_is_ilm_configured(void);
#endif
