/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <cmsis_core.h>	// <arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/sys_io.h>
#include <soc_common.h>

#define ROM_RESET_STATUS_ADDR	(0x100C7F00 + 0x84)
#define ROM_RESET_VCC_POWERUP	0x01
#define ROM_RESET_WDT_RST		0x02
#define ROM_RESET_DEBUGGER_RST	0x04
#define ROM_RESET_EXT_RST		0x08

enum npcm_reset_reason npcm_get_reset_reason(void)
{
	uint8_t reason_value;
	enum npcm_reset_reason reset_reason = NPCM_RESET_REASON_INVALID;

	reason_value = sys_read8((mem_addr_t) ROM_RESET_STATUS_ADDR);

	switch(reason_value) {
		case ROM_RESET_VCC_POWERUP:
			reset_reason = NPCM_RESET_REASON_VCC_POWERUP;
			break;
		case ROM_RESET_WDT_RST:
			reset_reason = NPCM_RESET_REASON_WDT_RST;
			break;
		case ROM_RESET_DEBUGGER_RST:
			reset_reason = NPCM_RESET_REASON_DEBUGGER_RST;
			break;
		default:
			break;
	}

	return reset_reason;
}

#ifdef CONFIG_XIP
void (*npcm_spi_nor_do_fw_update)(int type) = NULL;
uint8_t (*npcm_spi_nor_set_update_fw_address)(uint32_t fw_img_start,
		uint32_t fw_img_size, struct npcm_fw_write_bitmap *bitmap) = NULL;

__ramfunc uintptr_t npcm_vector_table_save(void)
{
	extern char _npcm_sram_vector_start[];
	uintptr_t vtor = 0;

	vtor = SCB->VTOR;
	SCB->VTOR = (uintptr_t)(_npcm_sram_vector_start) & SCB_VTOR_TBLOFF_Msk;
	__DSB();
	__ISB();
	return vtor;
}

__ramfunc void npcm_vector_table_restore(uintptr_t vtor)
{
	SCB->VTOR = vtor & SCB_VTOR_TBLOFF_Msk;
	__DSB();
	__ISB();
}

void npcm_sram_vector_table_copy(void)
{
        extern char _npcm_sram_vector_start[];
        extern char _npcm_rom_vector_table_start[];
        extern char _npcm_sram_vector_table_size[];

        /* copy sram vector table from rom to sram */
        (void *)memcpy(&_npcm_sram_vector_start, &_npcm_rom_vector_table_start,
                        (uint32_t)_npcm_sram_vector_table_size);
}

__ramfunc uint8_t npcm_set_update_fw_spi_nor_address(uint32_t fw_img_start,
                                                uint32_t fw_img_size,
                                                struct npcm_fw_write_bitmap *bitmap)
{
	if (npcm_spi_nor_set_update_fw_address)
		return npcm_spi_nor_set_update_fw_address(fw_img_start, fw_img_size, bitmap);

	return -1;
}

__ramfunc void sys_arch_reboot(int type)
{
	uintptr_t vtor = 0;

	vtor = npcm_vector_table_save();

	if (npcm_spi_nor_do_fw_update)
		npcm_spi_nor_do_fw_update(type);

	npcm_vector_table_restore(vtor);

	NVIC_SystemReset();

	/* pending here */
	for (;;);
}

#else /* !CONFIG_XIP */
uintptr_t npcm_vector_table_save(void)
{
	return 0;
}

void npcm_vector_table_restore(uintptr_t vtor)
{
	return;
}

void npcm_sram_vector_table_copy(void)
{
	return;
}

uint8_t npcm_set_update_fw_spi_nor_address(uint32_t fw_img_start,
						uint32_t fw_img_size,
						struct npcm_fw_write_bitmap *bitmap)
{
	return 0;
}
#endif
