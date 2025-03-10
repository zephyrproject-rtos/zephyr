/*
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MICROCHIP_PIC32CMMC00_H_
#define _SOC_MICROCHIP_PIC32CMMC00_H_

/* Structures and defines required by the sam0 drivers */

#include <stdint.h>

/* devicetree helpers */

#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_NAME(inst, name, cell)                          \
	DT_PHA_BY_NAME(DT_DRV_INST(inst), atmel_assigned_clocks, name, cell)

#define ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n)                                              \
	(volatile uint32_t *)(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, mclk)) +              \
			      DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, offset))

#define ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, cell)                                            \
	BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, cell))

/* Register access  helpers */

typedef union {
	uint8_t reg;
} _reg8_t;

typedef union {
	uint16_t reg;
} _reg16_t;

typedef union {
	uint32_t reg;
} _reg32_t;

/* External Interrupt Controller registers and macros */

typedef enum IRQn {
	PendSV_IRQn = -2,
	SysTick_IRQn = -1,
} IRQn_Type;

typedef union {
	struct {
		uint8_t: 1;
		uint8_t ENABLE: 1;
		uint8_t: 6;
	} bit;
	uint8_t reg;
} eic_ctrla_t;

typedef volatile struct {
	eic_ctrla_t CTRLA;
	uint8_t res0[3];
	const _reg32_t SYNCBUSY;
	uint32_t res1;
	_reg32_t INTENCLR;
	_reg32_t INTENSET;
	_reg32_t INTFLAG;
	uint32_t res2;
	_reg32_t CONFIG[2];
} eic_t;

#define EIC          ((eic_t *)DT_REG_ADDR(DT_INST(0, atmel_sam0_eic)))
#define EIC_CTRLA    ((uintptr_t)EIC + 0x00)
#define EIC_SYNCBUSY ((uintptr_t)EIC + 0x04)

#define EIC_CONFIG_FILTEN0     8
#define EIC_CONFIG_SENSE0_BOTH 3
#define EIC_CONFIG_SENSE0_FALL 2
#define EIC_CONFIG_SENSE0_HIGH 4
#define EIC_CONFIG_SENSE0_LOW  5
#define EIC_CONFIG_SENSE0_RISE 1
#define EIC_EXTINT_NUM         16
#define EIC_GCLK_ID            2

#define REG_EIC_CTRLA    EIC_CTRLA
#define REG_EIC_SYNCBUSY EIC_SYNCBUSY

/* Generic Clock Controller registers and macros */

typedef volatile struct {
	uint32_t res0[32];
	_reg32_t PCHCTRL[34];
} gclk_t;

#define GCLK          ((gclk_t *)DT_REG_ADDR(DT_INST(0, atmel_sam0_gclk)))
#define GCLK_GENCTRL0 ((uintptr_t)GCLK + 0x20)
#define GCLK_SYNCBUSY ((uintptr_t)GCLK + 0x04)

#define GCLK_GENCTRL_DIV_MASK GENMASK(31, 16)
#define GCLK_GENCTRL_GENEN    BIT(8)
#define GCLK_GENCTRL_SRC_MASK GENMASK(4, 0)
#define GCLK_PCHCTRL_CHEN     BIT(6)
#define GCLK_PCHCTRL_GEN(n)   FIELD_PREP(GENMASK(3, 0), n)

#define GCLK_GENCTRL_SRC_OSC48M_VAL 6
#define GCLK_PCHCTRL_GEN_GCLK0      0
#define GCLK_SYNCBUSY_GENCTRL0_BIT  2

#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

/* Main Clock registers and macros */

typedef volatile struct {
	uint32_t res0[5];
	_reg32_t APBAMASK;
	_reg32_t APBBMASK;
} mclk_t;

#define MCLK        ((mclk_t *)DT_REG_ADDR(DT_INST(0, atmel_sam0_mclk)))
#define MCLK_CPUDIV ((uintptr_t)MCLK + 0x04)

#define MCLK_APBAMASK_EIC     BIT(10)
#define MCLK_APBBMASK_NVMCTRL BIT(2)

#define MCLK_CPUDIV_CPUDIV_DIV1_VAL 1

/* Nonvolatile Memory Controller registers and macros */

typedef union {
	struct {
		uint32_t: 7;
		uint32_t MANW: 1;
		uint32_t: 24;
	} bit;
	uint32_t reg;
} nvmctrl_ctrlb_t;

typedef const union {
	struct {
		uint32_t READY: 1;
		uint32_t: 31;
	} bit;
	uint32_t reg;
} nvmctrl_intflag_t;

typedef union {
	struct {
		uint32_t: 2;
		uint32_t PROGE: 1;
		uint32_t LOCKE: 1;
		uint32_t NVME: 1;
		uint32_t: 27;
	} bit;
	uint32_t reg;
} NVMCTRL_STATUS_Type;

typedef volatile struct {
	_reg32_t CTRLA;
	nvmctrl_ctrlb_t CTRLB;
	uint32_t res0[3];
	nvmctrl_intflag_t INTFLAG;
	NVMCTRL_STATUS_Type STATUS;
	_reg32_t ADDR;
} nvmctrl_t;

#define NVMCTRL       ((nvmctrl_t *)DT_REG_ADDR(DT_INST(0, atmel_sam0_nvmctrl)))
#define NVMCTRL_CTRLB ((uintptr_t)NVMCTRL + 0x04)

#define NVMCTRL_CTRLA_CMDEX_KEY FIELD_PREP(GENMASK(15, 8), 0xA5)
#define NVMCTRL_CTRLB_MANW      BIT(7)
#define NVMCTRL_CTRLB_RWS_MASK  GENMASK(4, 1)
#define NVMCTRL_CTRLA_CMD_MASK  GENMASK(6, 0)

#define NVMCTRL_CTRLA_CMD_ER  FIELD_PREP(NVMCTRL_CTRLA_CMD_MASK, 0x02)
#define NVMCTRL_CTRLA_CMD_LR  FIELD_PREP(NVMCTRL_CTRLA_CMD_MASK, 0x40)
#define NVMCTRL_CTRLA_CMD_PBC FIELD_PREP(NVMCTRL_CTRLA_CMD_MASK, 0x44)
#define NVMCTRL_CTRLA_CMD_UR  FIELD_PREP(NVMCTRL_CTRLA_CMD_MASK, 0x41)
#define NVMCTRL_CTRLA_CMD_WP  FIELD_PREP(NVMCTRL_CTRLA_CMD_MASK, 0x04)

#define FLASH_PAGE_SIZE  64
#define FLASH_SIZE       CONFIG_FLASH_SIZE
#define NVMCTRL_ROW_SIZE 256

#define NVM_SW_CAL_ADDR        0x00806020
#define NVM_SW_CAL_CAL48M_MASK GENMASK64(40, 19)

/* Oscillators Controller macros */

#define OSCCTRL                DT_REG_ADDR(DT_PATH(soc, oscctrl_40001000))
#define OSCCTRL_CAL48M         (OSCCTRL + 0x38)
#define OSCCTRL_OSC48MDIV      (OSCCTRL + 0x15)
#define OSCCTRL_OSC48MSYNCBUSY (OSCCTRL + 0x18)
#define OSCCTRL_STATUS         (OSCCTRL + 0x0C)

#define OSCCTRL_CAL48M_MASK                  GENMASK(21, 0)
#define OSCCTRL_OSC48MSYNCBUSY_OSC48MDIV_BIT 2
#define OSCCTRL_STATUS_OSC48MRDY_BIT         4

/* I/O Pin Controller registers and macros */

typedef union {
	struct {
		uint8_t PMUXEN: 1;
		uint8_t INEN: 1;
		uint8_t PULLEN: 1;
		uint8_t: 3;
		uint8_t DRVSTR: 1;
		uint8_t: 1;
	} bit;
	uint8_t reg;
} PORT_PINCFG_Type;

typedef union {
	struct {
		uint8_t PMUXE: 4;
		uint8_t PMUXO: 4;
	} bit;
	uint8_t reg;
} PORT_PMUX_Type;

typedef volatile struct {
	_reg32_t DIR;
	_reg32_t DIRCLR;
	_reg32_t DIRSET;
	uint32_t res0;
	_reg32_t OUT;
	_reg32_t OUTCLR;
	_reg32_t OUTSET;
	_reg32_t OUTTGL;
	const _reg32_t IN;
	uint32_t res1[3];
	PORT_PMUX_Type PMUX[16];
	PORT_PINCFG_Type PINCFG[32];
} PortGroup;

#define PORT_GROUPS DT_NUM_INST_STATUS_OKAY(atmel_sam0_gpio)

/* Serial Communication Interface USART registers and macros */

typedef union {
	struct {
		uint32_t: 1;
		uint32_t ENABLE: 1;
		uint32_t: 22;
		uint32_t FORM: 4;
		uint32_t: 4;
	} bit;
	uint32_t reg;
} SERCOM_USART_CTRLA_Type;

typedef union {
	struct {
		uint32_t CHSIZE: 3;
		uint32_t: 3;
		uint32_t SBMODE: 1;
		uint32_t: 6;
		uint32_t PMODE: 1;
		uint32_t: 18;
	} bit;
	uint32_t reg;
} SERCOM_USART_CTRLB_Type;

typedef union {
	struct {
		uint8_t DRE: 1;
		uint8_t: 1;
		uint8_t RXC: 1;
		uint8_t: 5;
	} bit;
	uint8_t reg;
} sercom_usart_intflag_t;

typedef union {
	struct {
		uint8_t DRE: 1;
		uint8_t TXC: 1;
		uint8_t: 6;
	} bit;
	uint8_t reg;
} sercom_usart_intenset_t;

typedef volatile struct {
	SERCOM_USART_CTRLA_Type CTRLA;
	SERCOM_USART_CTRLB_Type CTRLB;
	uint32_t res0;
	_reg16_t BAUD;
	uint8_t res1[6];
	_reg8_t INTENCLR;
	uint8_t res2;
	sercom_usart_intenset_t INTENSET;
	uint8_t res3;
	sercom_usart_intflag_t INTFLAG;
	uint8_t res4;
	_reg16_t STATUS;
	const _reg32_t SYNCBUSY;
	uint32_t res5[2];
	_reg16_t DATA;
} SercomUsart;

#define SERCOM_USART_CTRLA_CPOL      BIT(29)
#define SERCOM_USART_CTRLA_DORD      BIT(30)
#define SERCOM_USART_CTRLA_FORM(n)   FIELD_PREP(GENMASK(27, 24), n)
#define SERCOM_USART_CTRLA_MODE(n)   FIELD_PREP(GENMASK(4, 2), n)
#define SERCOM_USART_CTRLA_RXPO_Pos  20
#define SERCOM_USART_CTRLA_TXPO_Pos  16
#define SERCOM_USART_CTRLB_CHSIZE(n) FIELD_PREP(GENMASK(2, 0), n)
#define SERCOM_USART_CTRLB_RXEN      BIT(17)
#define SERCOM_USART_CTRLB_TXEN      BIT(16)
#define SERCOM_USART_INTENCLR_DRE    BIT(0)
#define SERCOM_USART_INTENCLR_MASK   0xBF
#define SERCOM_USART_INTENCLR_RXC    BIT(2)
#define SERCOM_USART_INTENCLR_RXS    BIT(3)
#define SERCOM_USART_INTENCLR_TXC    BIT(1)
#define SERCOM_USART_INTENSET_DRE    BIT(0)
#define SERCOM_USART_INTENSET_RXC    BIT(2)
#define SERCOM_USART_INTENSET_TXC    BIT(1)
#define SERCOM_USART_STATUS_BUFOVF   BIT(2)
#define SERCOM_USART_STATUS_FERR     BIT(1)
#define SERCOM_USART_STATUS_PERR     BIT(0)
#define SERCOM_USART_SYNCBUSY_MASK   GENMASK(2, 0)

#endif /* _SOC_MICROCHIP_PIC32CMMC00_H_ */
