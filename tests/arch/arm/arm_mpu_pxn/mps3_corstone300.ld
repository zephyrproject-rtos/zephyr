/*
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_DATA_PROLOGUE(.customramfunc, , )
{
	*(.customramfunc) * (".customramfunc.*")
}
GROUP_DATA_LINK_IN(SRAM, SRAM)
