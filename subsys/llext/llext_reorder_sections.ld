/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTIONS {
	.pinned_text : {
		*(.pinned_text .pinned_text.*)
	}

	.text : {
		*(.text .text.*)
	}

	.bss : {
		*(.bss .bss.*)
	}

	.data : {
		*(.data .data.*)
	}

	.rodata_in_data : {
		*(.rodata_in_data .rodata_in_data.*)
	}

	.rodata : {
		*(.rodata .rodata.*)
	}
}
