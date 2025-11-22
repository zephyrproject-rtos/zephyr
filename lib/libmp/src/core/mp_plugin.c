/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mp_plugin.h"

void initialize_plugins(void)
{
	STRUCT_SECTION_FOREACH(mp_plugin, p) {
		if (p->init) {
			p->init();
		}
	}
}
