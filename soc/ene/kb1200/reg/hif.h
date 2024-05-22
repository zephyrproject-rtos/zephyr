/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_HIF_H
#define ENE_KB1200_HIF_H

/**
 *  Structure type to Host Interface (HIF).
 */
struct hif_regs {
	volatile uint32_t IDXCFG;      /*Index IO Configuration Register */
	volatile uint32_t IDX32CFG;    /*Index32 IO Configuration Register */
	volatile uint32_t MEMFCFG;     /*Memory to Flash Register */
	volatile uint8_t USRIRQ;       /*User Define IRQ Register */
	volatile uint8_t Reserved0[3]; /*Reserved */
	volatile uint32_t IOSCFG;      /*IO to SRAM Register */
	volatile uint32_t IOSIE;       /*IO to SRAM Interrupt Enable Register */
	volatile uint8_t IOSPF;        /*IO to SRAM Event Pending Flag Register */
	volatile uint8_t Reserved1[3]; /*Reserved */
	volatile uint32_t IOSRWP;      /*IO to SRAM Read/Write Protection Register */
	volatile uint32_t MEMSCFG;     /*Memory to SRAM Register */
	volatile uint32_t MEMSIE;      /*Memory to SRAM Interrupt Enable Register */
	volatile uint8_t MEMSPF;       /*Memory to SRAM Event Pending Flag Register */
	volatile uint8_t Reserved2[3]; /*Reserved */
	volatile uint16_t MEMSAINF;    /*Memory to SRAM Address Information Register */
	volatile uint16_t Reserved3;   /*Reserved */
	volatile uint32_t MEMSBRP;     /*Memory to SRAM Block Read Protection Register */
	volatile uint32_t MEMSBWP;     /*Memory to SRAM Block Write Protection Register */
	volatile uint8_t MEMSSS0;      /*Memory to SRAM Sector Selection0 Register */
	volatile uint8_t Reserved4[3]; /*Reserved */
	volatile uint32_t MEMSSP0;     /*Memory to SRAM Sector Protection0 Register */
	volatile uint8_t MEMSSS1;      /*Memory to SRAM Sector Selection1 Register */
	volatile uint8_t Reserved5[3]; /*Reserved */
	volatile uint32_t MEMSSP1;     /*Memory to SRAM Sector Protection1 Register */
};

#define INDEX32_0_FUNCTION_ENABLE 0x00000001
#define INDEX32_0_MASK            0x0000FFFF
#define INDEX32_0_POS             0
#define INDEX32_1_FUNCTION_ENABLE 0x00010000
#define INDEX32_1_MASK            0xFFFF0000
#define INDEX32_1_POS             16

#define IO2SRAM_FUNCTION_ENABLE 0x01

#define IO2SRAM_WRITE_EVENT 0x01

#define IO2SRAM_SRAM_BASE_POS  0
#define IO2SRAM_SRAM_BASE_MASK 0xFFFFFF00
#define IO2SRAM_IO_BASE_POS    16
#define IO2SRAM_IO_BASE_MASK   0xFFFF

#endif /* ENE_KB1200_HIF_H */
