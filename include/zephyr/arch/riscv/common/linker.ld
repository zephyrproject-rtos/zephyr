/*
 * Copyright (c) 2016-2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "linker-head.ld"

	GROUP_START(ROMABLE_REGION)
#include "linker-rom.ld"
	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)
#include "linker-ram.ld"
	GROUP_END(RAMABLE_REGION)

#include "linker-tail.ld"
