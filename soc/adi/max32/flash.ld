/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_DATA_PROLOGUE(.flashprog,, SUBALIGN(4))
{
	KEEP(*(.flashprog*)) /* Flash program */
}
