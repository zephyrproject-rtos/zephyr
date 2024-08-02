/*
 * Copyright 2022 The ChromiumOS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

/* Places code in the section that gets mapped into ILM */
#define __soc_ram_code __attribute__((section(".__ram_code")))


#ifndef _ASMLANGUAGE
bool it8xxx2_is_ilm_configured(void);
#endif
