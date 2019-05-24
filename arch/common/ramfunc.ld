/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

SECTION_DATA_PROLOGUE(.ramfunc,,)
{
	MPU_ALIGN(_ramfunc_ram_size);
	_ramfunc_ram_start = .;
	*(.ramfunc)
	*(".ramfunc.*")
	MPU_ALIGN(_ramfunc_ram_size);
	_ramfunc_ram_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
_ramfunc_ram_size = _ramfunc_ram_end - _ramfunc_ram_start;
_ramfunc_rom_start = LOADADDR(.ramfunc);
