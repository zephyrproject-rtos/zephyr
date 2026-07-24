/*
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(stm32wl3_RAM_VR, 0x20000000 (NOLOAD), )
{
	. += 4;
	KEEP(*(stm32wl3_RAM_VR));
} GROUP_LINK_IN(RAMABLE_REGION)
