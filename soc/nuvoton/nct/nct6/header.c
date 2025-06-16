/*----------------------------------------------------------------------------*/
/* Copyright (c) 2025 by Nuvoton Electronics Corporation                      */
/* All rights reserved.							      */
/*----------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>
#include <header.h>

extern unsigned long _vector_table;
extern unsigned long __fast_hook_seg_start__;
extern unsigned long __fast_hook_seg_end__;
extern unsigned long __main_fw_seg_start__;
extern unsigned long __main_fw_seg_end__;
extern unsigned long __ram_code_flash_start__;
extern unsigned long __ram_code_flash_end__;
extern unsigned long __ram_code_ram_start__;
//extern unsigned long __ram_code_ram_end__;


__attribute__((section(".header"))) struct FIRMWARE_HEDAER_TYPE fw_header = {
	.hUserFWEntryPoint = (uint32_t)(&_vector_table),
	.hUserFWRamCodeFlashStart = (uint32_t)(&__ram_code_flash_start__),
	.hUserFWRamCodeFlashEnd = (uint32_t)(&__ram_code_flash_end__),
	.hUserFWRamCodeRamStart = (uint32_t)(&__ram_code_ram_start__),

	.hRomHook1Ptr = 0, /* Hook 1 muse be flash code */
	.hRomHook2Ptr = 0,
	.hRomHook3Ptr = 0,
	.hRomHook4Ptr = 0,

	/* seg 0 - fw image information */
	.hFwSeg[0].hOffset = 0x210, /* fixed, can't change */
	.hFwSeg[0].hSize = 0x500, /* fixed, can't change */

	/* seg 1 - RomHook (no use) */
	.hFwSeg[1].hOffset = (uint32_t)(&__fast_hook_seg_start__) - CONFIG_FLASH_BASE_ADDRESS,
	.hFwSeg[1].hSize = (uint32_t)(&__fast_hook_seg_end__) - CONFIG_FLASH_BASE_ADDRESS,

	/* seg 2 - fw body start and end offset (flash view) */
	.hFwSeg[2].hOffset = (uint32_t)(&__main_fw_seg_start__) - CONFIG_FLASH_BASE_ADDRESS,
	.hFwSeg[2].hSize = (uint32_t)(&__main_fw_seg_end__) - CONFIG_FLASH_BASE_ADDRESS,

	/* seg 3 - reserved */
	.hFwSeg[3].hOffset = 0, /* Reserved for fan table and fill in BinGentool */
	.hFwSeg[3].hSize = 0, /* Reserved for fan table and fill in BinGentool */
};
