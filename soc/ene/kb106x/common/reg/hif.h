/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_HIF_H
#define ENE_KB106X_HIF_H

/**
 *  Structure type to Host Interface (HIF).
 */
struct hif_regs {
	volatile uint32_t idx_cfg;      /*Index IO Configuration Register */
	volatile uint32_t idx32_cfg;    /*Index32 IO Configuration Register */
	volatile uint32_t memf_cfg;     /*Memory to Flash Register */
	volatile uint8_t usr_irq;       /*User Define IRQ Register */
	volatile uint8_t reserved_0[3]; /*Reserved */
	volatile uint32_t ios_cfg;      /*IO to SRAM Register */
	volatile uint32_t ios_ie;       /*IO to SRAM Interrupt Enable Register */
	volatile uint8_t ios_pf;        /*IO to SRAM Event Pending Flag Register */
	volatile uint8_t reserved_1[3]; /*Reserved */
	volatile uint32_t ios_rwp;      /*IO to SRAM Read/Write Protection Register */
	volatile uint32_t mems_cfg;     /*Memory to SRAM Register */
	volatile uint32_t mems_ie;      /*Memory to SRAM Interrupt Enable Register */
	volatile uint8_t mems_pf;       /*Memory to SRAM Event Pending Flag Register */
	volatile uint8_t reserved_2[3]; /*Reserved */
	volatile uint16_t mems_ainf;    /*Memory to SRAM Address Information Register */
	volatile uint16_t reserved_3;   /*Reserved */
	volatile uint32_t mems_brp;     /*Memory to SRAM Block Read Protection Register */
	volatile uint32_t mems_bwp;     /*Memory to SRAM Block Write Protection Register */
	volatile uint8_t mems_ss0;      /*Memory to SRAM Sector Selection0 Register */
	volatile uint8_t reserved_4[3]; /*Reserved */
	volatile uint32_t mems_sp0;     /*Memory to SRAM Sector Protection0 Register */
	volatile uint8_t mems_ss1;      /*Memory to SRAM Sector Selection1 Register */
	volatile uint8_t reserved_5[3]; /*Reserved */
	volatile uint32_t mems_sp1;     /*Memory to SRAM Sector Protection1 Register */
};

#define INDEX32_0_FUNCTION_ENABLE 0x00000001
#define INDEX32_0_MASK            0x0000FFFF
#define INDEX32_0_POS             0
#define INDEX32_1_FUNCTION_ENABLE 0x00010000
#define INDEX32_1_MASK            0xFFFF0000
#define INDEX32_1_POS             16

#define IO2SRAM_FUNCTION_ENABLE 0x01

#define IO2SRAM_WRITE_EVENT 0x01

#define IO2RAM_RAM_BASE_POS       0
#define IO2RAM_RAM_BASE_MASK      0xFFFFFF00
#define IO2RAM_IO_BASE_POS        16
#define IO2RAM_IO_BASE_MASK       0xFFFF
#define IO2RAM_NOTIFY_OFFSET_POS  8
#define IO2RAM_NOTIFY_OFFSET_MASK 0xFF

#define MEN2RAM_RAM_BASE_POS  0
#define MEN2RAM_RAM_BASE_MASK 0xFFFFE000
#define MEN2RAM_MEM_BASE_POS  0
#define MEN2RAM_MEM_BASE_MASK 0xFFFFE000

#define H2RAM_FUNCTION_ENABLE BIT(0)

#define H2RAM_NOTIFY_EVENT BIT(0)

#endif /*ENE_KB106X_HIF_H*/
