/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

/* Places code in the section that gets mapped into ILM */
#define __soc_ram_code __attribute__((section(".__ram_code")))

#ifndef _ASMLANGUAGE
void custom_reset_instr_cache(void);
bool it8xxx2_is_ilm_configured(void);

#endif
