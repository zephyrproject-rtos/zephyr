/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE vendor blob sections for HBN (Hibernation) RAM.
 * Injected via zephyr_linker_sources(SECTIONS).
 *
 * .hbn_code: RF functions from libbl702l_rf.a that must survive
 *            hibernation. Loaded from flash LMA to RETRAM VMA.
 * .retention_noinit: Data retained across PDS/HBN sleep, not zeroed.
 */

SECTION_DATA_PROLOGUE(.hbn_code,,)
{
	_hbn_load_start = LOADADDR(.hbn_code);
	_hbn_run_start = .;
	*(.hbn_code .hbn_code.*)
	_hbn_run_end = .;
} GROUP_DATA_LINK_IN(RETRAM, ROMABLE_REGION)
_hbn_run_size = SIZEOF(.hbn_code);

SECTION_DATA_PROLOGUE(.retention_noinit,(NOLOAD),)
{
	*(.retention_noinit .retention_noinit.*)
} GROUP_NOLOAD_LINK_IN(RETRAM, RETRAM)

/*
 * Symbols consumed by the BLE controller binary blob:
 * __LD_CONFIG_EM_SEL    — exchange memory size in bytes (blob uses this to
 *                         know how much EM the hardware was configured for)
 * __LD_BLE_CORE_REG_SIZE — BLE core register area size (fixed at 0x214)
 */
#if defined(CONFIG_BFLB_BL70XL_BLE_EM_16K)
PROVIDE(__LD_CONFIG_EM_SEL = 16384);
#else
PROVIDE(__LD_CONFIG_EM_SEL = 8192);
#endif
PROVIDE(__LD_BLE_CORE_REG_SIZE = 0x214);
