/*----------------------------------------------------------------------------*/
/* Copyright (c) 2019 by Nuvoton Electronics Corporation                      */
/* All rights reserved.							      */
/*----------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>
#include <header.h>

extern unsigned long _vector_table;
#ifndef CONFIG_XIP
extern unsigned long __main_fw_seg_size__;
#endif
extern unsigned long __main_fw_seg_end__;

__attribute__((section(".header"))) struct FIRMWARE_HEDAER_TYPE fw_header = {
	.hUserFWEntryPoint = (uint32_t)(&_vector_table),
	.hUserFWRamCodeFlashStart = CONFIG_FLASH_BASE_ADDRESS +
		NPCM4XX_FW_HEADER_SIZE,
#ifndef CONFIG_XIP
	.hUserFWRamCodeFlashEnd = CONFIG_FLASH_BASE_ADDRESS +
		NPCM4XX_FW_HEADER_SIZE +
		(uint32_t)(&__main_fw_seg_size__),
	.hUserFWRamCodeRamStart = (uint32_t)(&_vector_table),
#else
	.hUserFWRamCodeFlashEnd = CONFIG_FLASH_BASE_ADDRESS +
		NPCM4XX_FW_HEADER_SIZE,
	.hUserFWRamCodeRamStart = 0,
#endif

	.hRomHook1Ptr = 0, /* Hook 1 muse be flash code */
	.hRomHook2Ptr = 0,
	.hRomHook3Ptr = 0,
	.hRomHook4Ptr = 0,

	/* seg 0 - fw image information */
	.hFwSeg[0].hOffset = 0x210, /* fixed, can't change */
	.hFwSeg[0].hSize = 0x500, /* fixed, can't change */

	/* seg 1 - RomHook (no use) */
	.hFwSeg[1].hOffset = 0x0,
	.hFwSeg[1].hSize = 0,

	/* seg 2 - fw body start and end offset (flash view) */
	.hFwSeg[2].hOffset = 0x600,
#ifndef CONFIG_XIP
	.hFwSeg[2].hSize = (uint32_t)(&__main_fw_seg_end__) - CONFIG_SRAM_BASE_ADDRESS,
#else
	.hFwSeg[2].hSize = (uint32_t)(&__main_fw_seg_end__) - CONFIG_FLASH_BASE_ADDRESS,
#endif

	/* seg 3 - reserved */
	.hFwSeg[3].hOffset = 0, /* Reserved for fan table and fill in BinGentool */
	.hFwSeg[3].hSize = 0, /* Reserved for fan table and fill in BinGentool */
};
