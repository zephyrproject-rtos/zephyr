/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * ITCM section inputs and BLE controller GP-relative data layout.
 * Injected via zephyr_linker_sources(ITCM_SECTION).
 */

*(.tcm_code .tcm_code.*)
*(.tcm_data .tcm_data.*)
*(.pds_code .pds_code.*)

/*
 * BLE controller binary blob uses GP-relative addressing for its state data.
 * The blob is compiled with GP = _rom_data_run + 0x800 and a revision-specific
 * data area size. These values must match the vendor SDK linker script exactly.
 *
 * Direct assignment overrides any prior PROVIDE(__global_pointer$).
 * CONFIG_RISCV_GP must be disabled when BLE is active so Zephyr
 * code does not rely on GP-relative addressing.
 */

#if defined(CONFIG_BFLB_BL70XL_REV_A1)
#define ROM_DATA_LOAD 0x2103b5c0
#define ROM_DATA_SIZE 0xd4c
#else /* A0 */
#define ROM_DATA_LOAD 0x21037d50
#define ROM_DATA_SIZE 0xc98
#endif

. = ALIGN(16);
_rom_data_run = .;
. = . + ROM_DATA_SIZE;
__global_pointer$ = _rom_data_run + 0x800;

PROVIDE(_rom_data_load = ROM_DATA_LOAD);
PROVIDE(_rom_data_size = ROM_DATA_SIZE);

/*
 * GP-relative symbols from rom_map.ld needed by ROM extension objects.
 * These map to fixed offsets in the ROM data area relative to __global_pointer$.
 * A0 and A1 have different ROM data layouts, so offsets differ.
 */
#if defined(CONFIG_BFLB_BL70XL_REV_A1)
PROVIDE(bl_irq_ctx_list      = __global_pointer$ - 564);
PROVIDE(bl_irq_handler_list  = __global_pointer$ - 560);
PROVIDE(bl_rtc_frequency     = __global_pointer$ - 578);
PROVIDE(rf_get_rf_state_ptr  = __global_pointer$ - 328);
PROVIDE(rf_set_tx_bypass_ptr = __global_pointer$ - 324);
#else /* A0 */
PROVIDE(bl_irq_handler_list  = __global_pointer$ - 576);
PROVIDE(bl_rtc_frequency     = __global_pointer$ - 580);
PROVIDE(rf_get_rf_state_ptr  = __global_pointer$ - 320);
PROVIDE(rf_set_tx_bypass_ptr = __global_pointer$ - 316);
#endif
