/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <cmsis_core.h>	// <arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/devicetree.h>
#include <soc_common.h>
#include <reg/reg_def.h>

#define ROM_RESET_STATUS_ADDR	(0x100C7F00 + 0x84)
#define ROM_RESET_VCC_POWERUP	0x01
#define ROM_RESET_WDT_RST		0x02
#define ROM_RESET_DEBUGGER_RST	0x04
#define ROM_RESET_EXT_RST		0x08

#define OTP_NODE DT_NODELABEL(otp)
#define OTP_BASE_ADDR DT_REG_ADDR(OTP_NODE)

#define OTP  ((struct otp_reg *)  OTP_BASE_ADDR)

#define OTP_WRITE_TIMEOUT   0x10000
#define OTP_ENTRY_CODE      0x66
#define OTP_EXIT_CODE       0x55

#define OTP_WAIT_WR_READY(otp_cnter, result) { \
    while((OTP->STS & (0x01 << NCT_OTP_STS_PGDSTS)) == 0) { \
        if(OTP->STS & (0x01 << NCT_OTP_STS_PERROR)) {\
            result = -1; \
            break; \
        } \
        if((otp_cnter--) == 0) { \
            result = -2; \
            break; \
    }}}

enum nct_reset_reason nct_get_reset_reason(void)
{
	uint8_t reason_value;
	enum nct_reset_reason reset_reason = NCT_RESET_REASON_INVALID;

	reason_value = sys_read8((mem_addr_t) ROM_RESET_STATUS_ADDR);

	switch(reason_value) {
		case ROM_RESET_VCC_POWERUP:
			reset_reason = NCT_RESET_REASON_VCC_POWERUP;
			break;
		case ROM_RESET_WDT_RST:
			reset_reason = NCT_RESET_REASON_WDT_RST;
			break;
		case ROM_RESET_DEBUGGER_RST:
			reset_reason = NCT_RESET_REASON_DEBUGGER_RST;
			break;
		default:
			break;
	}

	return reset_reason;
}

#ifdef CONFIG_XIP
void (*nct_spi_nor_do_fw_update)(int type) = NULL;
uint8_t (*nct_spi_nor_set_update_fw_address)(uint32_t fw_img_start,
		uint32_t fw_img_size, struct nct_fw_write_bitmap *bitmap) = NULL;

__ramfunc uintptr_t nct_vector_table_save(void)
{
	extern char _nct_sram_vector_start[];
	uintptr_t vtor = 0;

	vtor = SCB->VTOR;
	SCB->VTOR = (uintptr_t)(_nct_sram_vector_start) & SCB_VTOR_TBLOFF_Msk;
	__DSB();
	__ISB();
	return vtor;
}

__ramfunc void nct_vector_table_restore(uintptr_t vtor)
{
	SCB->VTOR = vtor & SCB_VTOR_TBLOFF_Msk;
	__DSB();
	__ISB();
}

void nct_sram_vector_table_copy(void)
{
        extern char _nct_sram_vector_start[];
        extern char _nct_rom_vector_table_start[];
        extern char _nct_sram_vector_table_size[];

        /* copy sram vector table from rom to sram */
        (void *)memcpy(&_nct_sram_vector_start, &_nct_rom_vector_table_start,
                        (uint32_t)_nct_sram_vector_table_size);
}

__ramfunc uint8_t nct_set_update_fw_spi_nor_address(uint32_t fw_img_start,
                                                uint32_t fw_img_size,
                                                struct nct_fw_write_bitmap *bitmap)
{
	if (nct_spi_nor_set_update_fw_address)
		return nct_spi_nor_set_update_fw_address(fw_img_start, fw_img_size, bitmap);

	return -1;
}

__ramfunc void sys_arch_reboot(int type)
{
	uintptr_t vtor = 0;

	vtor = nct_vector_table_save();

	if (nct_spi_nor_do_fw_update)
		nct_spi_nor_do_fw_update(type);

	nct_vector_table_restore(vtor);

	NVIC_SystemReset();

	/* pending here */
	for (;;);
}

#else /* !CONFIG_XIP */
uintptr_t nct_vector_table_save(void)
{
	return 0;
}

void nct_vector_table_restore(uintptr_t vtor)
{
	return;
}

void nct_sram_vector_table_copy(void)
{
	return;
}

uint8_t nct_set_update_fw_spi_nor_address(uint32_t fw_img_start,
						uint32_t fw_img_size,
						struct nct_fw_write_bitmap *bitmap)
{
	return 0;
}
#endif

int32_t nct_erpmc_WriteBit(uint16_t adr, uint8_t bit)
{
	uint32_t otp_cnter = OTP_WRITE_TIMEOUT;
	int32_t result = 0;
	uint8_t addrH, addrL;

	if((adr > 586) || (adr < 576)) {return 1;}
	if(bit > 7) {return 1;}

	addrH = (uint8_t)((adr & 0x01E0) >> 5);
	addrL = ((uint8_t)(adr & 0x001F)) << NCT_OTP_ADDRL_ADDR0to4;
	addrL |= bit;

	OTP->CTRL |= (0x01 << NCT_OTP_CTRL_ENOTP);
	OTP->LOCK = 0;
	OTP->ECODE = OTP_ENTRY_CODE;
	OTP->ADDR_H = addrH;
	OTP->ADDR_L = addrL;
	OTP->CTRL |= (0x01 << NCT_OTP_CTRL_PGEN);

	OTP_WAIT_WR_READY(otp_cnter, result);

	OTP->ECODE = OTP_EXIT_CODE; //leave OTP operation
	OTP->CTRL = 0;

	return result;
}

uint8_t nct_otp_Read(uint16_t adr)
{
	uint8_t temp;
	uint8_t addrH,addrL;
	unsigned long cnt = 10000;
	OTP->CTRL |= (0x01 << NCT_OTP_CTRL_ENOTP);
	OTP->LOCK = 0;
	OTP->ECODE = 0x66; //entry OTP operation
	addrH = (uint8_t)((adr & 0x03E0) >> 5);
	addrL = ((uint8_t)(adr & 0x001F)) << NCT_OTP_ADDRL_ADDR0to4;
	OTP->ADDR_H = addrH;
	OTP->ADDR_L = addrL;
	OTP->CTRL |= (0x01 << NCT_OTP_CTRL_RDEN);
	while(cnt--) {}
	//if((OTP->CTRL & OTP_CTRL_RDEN_Msk)) {OTP->CTRL &= ~OTP_CTRL_RDEN_Msk; printf("\r\n %d is RD locked!!", adr);}
	while(!(OTP->STS & (0x01 << NCT_OTP_STS_RDDSTS))) {}
	temp = OTP->DOUT;
	OTP->ECODE = OTP_EXIT_CODE; //leave OTP operation
	OTP->CTRL &= ~(0x01 << NCT_OTP_CTRL_ENOTP);

	// wait OTP state machine ready
	while(OTP->CTRL & 0x40);

	return temp;
}


uint8_t nct_otp_BlockRead(uint16_t start_adr, uint16_t len, uint8_t* Data)
{
	uint16_t i;
	if(start_adr + len > 1024){ return 0;}
	for(i=0;i<len;i++) { Data[i] = nct_otp_Read(start_adr + i);}
	return 1;
}


int32_t nct_erpmc_WriteByte(uint16_t adr, uint8_t dat)
{
	uint32_t otp_cnter = OTP_WRITE_TIMEOUT;
	uint16_t i;
	int32_t result = 0;

	/* 0 is not need update */
	if(dat == 0) { return -1; }

	/* erpmc range is 576 ~ 586 */
	if((adr > 586) || (adr < 576)) { return -1; }

	for(i = adr<<3; i < ((adr+1)<<3); i++)
	{
		if(dat & 0x1)
		{
			OTP->CTRL = (0x01 << NCT_OTP_CTRL_ENOTP);
			OTP->ADDR_H = (i >> 8);
			OTP->ADDR_L = (i & 0xFF);
			OTP->ECODE = OTP_ENTRY_CODE;
			OTP->CTRL = (0x01 << NCT_OTP_CTRL_ENOTP) | (0x01 << NCT_OTP_CTRL_PGEN);
			otp_cnter = OTP_WRITE_TIMEOUT;

			OTP_WAIT_WR_READY(otp_cnter, result);

			OTP->ECODE = OTP_EXIT_CODE;
			OTP->CTRL = 0;
		}
		dat >>= 1;
	}

	return result;
}
