/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE vendor blob initialized data sections for BL702.
 * Injected via zephyr_linker_sources(RWDATA).
 *
 * The BL702 BLE blob uses FreeRTOS ROM at 0x21020000 which stores mutable
 * state via GP-relative addressing. We reserve space for:
 *   .bleromrw  = 0x84 bytes (BLE ROM writable data)
 *   .rtosromrw = 0x4A0 bytes (FreeRTOS ROM writable data)
 *
 * __global_pointer$ is placed so that GP-relative accesses from ROM code
 * land in the .rtosromrw region. Layout matches vendor flash_rom.ld exactly.
 */

*(.tcm_data .tcm_data.*)
*(.retention .retention.*)
*(.bleromrw .bleromrw.*)

. = ALIGN(16);

/* BLE ROM writable data (0x84 bytes) */
_bleromrw_start = .;
. = . + 0x84;

/* FreeRTOS ROM writable data region */
. = ALIGN(16);
_rtosromrw_start = .;
. = . + 0x20;
__global_pointer_head$ = .;
__global_pointer$ = . + 0x7E0;
. = _rtosromrw_start + 0x20 + 0x4A0;

/* GP-relative symbols needed by FreeRTOS ROM and BLE blob */
PROVIDE(pTrapNetCounter = __global_pointer_head$);
PROVIDE(TrapNetCounter = __global_pointer_head$ + 0x5C);
