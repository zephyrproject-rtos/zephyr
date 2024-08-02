/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE (.native_sim_if,,)
{
	KEEP(*(.native_sim_if));
	KEEP(*(.native_sim_if.*));
}
