/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_espi

#include <assert.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it51xxx.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it8xxx2.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <soc_dt.h>
#include "soc_espi.h"
#include "espi_utils.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#define ESPI_ITE_GET_GCTRL_BASE ((struct gctrl_ite_ec_regs *)DT_REG_ADDR(DT_NODELABEL(gctrl)))

#define IT8XXX2_ESPI_IRQ     DT_INST_IRQ_BY_IDX(0, 0, irq)
#define IT8XXX2_ESPI_VW_IRQ  DT_INST_IRQ_BY_IDX(0, 1, irq)
#define IT8XXX2_KBC_IBF_IRQ  DT_INST_IRQ_BY_IDX(0, 2, irq)
#define IT8XXX2_KBC_OBE_IRQ  DT_INST_IRQ_BY_IDX(0, 3, irq)
#define IT8XXX2_PMC1_IBF_IRQ DT_INST_IRQ_BY_IDX(0, 4, irq)
#define IT8XXX2_PORT_80_IRQ  DT_INST_IRQ_BY_IDX(0, 5, irq)
#define IT8XXX2_PMC2_IBF_IRQ DT_INST_IRQ_BY_IDX(0, 6, irq)
#define IT8XXX2_TRANS_IRQ    DT_INST_IRQ_BY_IDX(0, 7, irq)
#define IT8XXX2_PMC3_IBF_IRQ DT_INST_IRQ_BY_IDX(0, 8, irq)
#define IT8XXX2_PMC4_IBF_IRQ DT_INST_IRQ_BY_IDX(0, 9, irq)
#define IT8XXX2_PMC5_IBF_IRQ DT_INST_IRQ_BY_IDX(0, 10, irq)

/* General Capabilities and Configuration 1 */
#define IT8XXX2_ESPI_MAX_FREQ_MASK GENMASK(2, 0)
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_20 0
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_25 1
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_33 2
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_50 3
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_66 4

#define IT8XXX2_ESPI_PC_READY_MASK  BIT(1)
#define IT8XXX2_ESPI_VW_READY_MASK  BIT(1)
#define IT8XXX2_ESPI_OOB_READY_MASK BIT(1)
#define IT8XXX2_ESPI_FC_READY_MASK  BIT(1)

#define IT8XXX2_ESPI_INTERRUPT_ENABLE          BIT(7)
#define IT8XXX2_ESPI_TO_WUC_ENABLE             BIT(4)
#define IT8XXX2_ESPI_VW_INTERRUPT_ENABLE       BIT(7)
#define IT8XXX2_ESPI_INTERRUPT_PUT_PC          BIT(7)

/*
 * VWCTRL2 register:
 * bit4 = 1b: Refers to ESPI_RESET# for PLTRST#.
 */
#define IT8XXX2_ESPI_VW_RESET_PLTRST           BIT(4)

#define IT8XXX2_ESPI_UPSTREAM_ENABLE           BIT(7)
#define IT8XXX2_ESPI_UPSTREAM_GO               BIT(6)
#define IT8XXX2_ESPI_UPSTREAM_INTERRUPT_ENABLE BIT(5)
#define IT8XXX2_ESPI_UPSTREAM_CHANNEL_DISABLE  BIT(2)
#define IT8XXX2_ESPI_UPSTREAM_DONE             BIT(1)
#define IT8XXX2_ESPI_UPSTREAM_BUSY             BIT(0)

#define IT8XXX2_ESPI_CYCLE_TYPE_OOB            0x07

#define IT8XXX2_ESPI_PUT_OOB_STATUS            BIT(7)
#define IT8XXX2_ESPI_PUT_OOB_INTERRUPT_ENABLE  BIT(7)
#define IT8XXX2_ESPI_PUT_OOB_LEN_MASK          GENMASK(6, 0)

#define IT8XXX2_ESPI_INPUT_PAD_GATING          BIT(6)

#define IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE    64
#define IT8XXX2_ESPI_PUT_FLASH_TAG_MASK        GENMASK(7, 4)
#define IT8XXX2_ESPI_PUT_FLASH_LEN_MASK        GENMASK(6, 0)

/*
 * EC2I bridge registers
 */
struct ec2i_regs {
	/* 0x00: Indirect Host I/O Address Register */
	volatile uint8_t IHIOA;
	/* 0x01: Indirect Host Data Register */
	volatile uint8_t IHD;
	/* 0x02: Lock Super I/O Host Access Register */
	volatile uint8_t LSIOHA;
	/* 0x03: Super I/O Access Lock Violation Register */
	volatile uint8_t SIOLV;
	/* 0x04: EC to I-Bus Modules Access Enable Register */
	volatile uint8_t IBMAE;
	/* 0x05: I-Bus Control Register */
	volatile uint8_t IBCTL;
};

/* Index list of the host interface registers of PNPCFG */
enum host_pnpcfg_index {
	/* Logical Device Number */
	HOST_INDEX_LDN = 0x07,
	/* Chip ID Byte 1 */
	HOST_INDEX_CHIPID1 = 0x20,
	/* Chip ID Byte 2 */
	HOST_INDEX_CHIPID2 = 0x21,
	/* Chip Version */
	HOST_INDEX_CHIPVER = 0x22,
	/* Super I/O Control */
	HOST_INDEX_SIOCTRL = 0x23,
	/* Super I/O IRQ Configuration */
	HOST_INDEX_SIOIRQ = 0x25,
	/* Super I/O General Purpose */
	HOST_INDEX_SIOGP = 0x26,
	/* Super I/O Power Mode */
	HOST_INDEX_SIOPWR = 0x2D,
	/* Depth 2 I/O Address */
	HOST_INDEX_D2ADR = 0x2E,
	/* Depth 2 I/O Data */
	HOST_INDEX_D2DAT = 0x2F,
	/* Logical Device Activate Register */
	HOST_INDEX_LDA = 0x30,
	/* I/O Port Base Address Bits [15:8] for Descriptor 0 */
	HOST_INDEX_IOBAD0_MSB = 0x60,
	/* I/O Port Base Address Bits [7:0] for Descriptor 0 */
	HOST_INDEX_IOBAD0_LSB = 0x61,
	/* I/O Port Base Address Bits [15:8] for Descriptor 1 */
	HOST_INDEX_IOBAD1_MSB = 0x62,
	/* I/O Port Base Address Bits [7:0] for Descriptor 1 */
	HOST_INDEX_IOBAD1_LSB = 0x63,
	/* Interrupt Request Number and Wake-Up on IRQ Enabled */
	HOST_INDEX_IRQNUMX = 0x70,
	/* Interrupt Request Type Select */
	HOST_INDEX_IRQTP = 0x71,
	/* DMA Channel Select 0 */
	HOST_INDEX_DMAS0 = 0x74,
	/* DMA Channel Select 1 */
	HOST_INDEX_DMAS1 = 0x75,
	/* Device Specific Logical Device Configuration 1 to 10 */
	HOST_INDEX_DSLDC1 = 0xF0,
	HOST_INDEX_DSLDC2 = 0xF1,
	HOST_INDEX_DSLDC3 = 0xF2,
	HOST_INDEX_DSLDC4 = 0xF3,
	HOST_INDEX_DSLDC5 = 0xF4,
	HOST_INDEX_DSLDC6 = 0xF5,
	HOST_INDEX_DSLDC7 = 0xF6,
	HOST_INDEX_DSLDC8 = 0xF7,
	HOST_INDEX_DSLDC9 = 0xF8,
	HOST_INDEX_DSLDC10 = 0xF9,
	HOST_INDEX_DSLDC11 = 0xFA,
	HOST_INDEX_DSLDC12 = 0xFB,
	HOST_INDEX_DSLDC13 = 0xFD,
};

/* List of logical device number (LDN) assignments */
enum logical_device_number {
	/* Serial Port 1 */
	LDN_UART1 = 0x01,
	/* Serial Port 2 */
	LDN_UART2 = 0x02,
	/* System Wake-Up Control */
	LDN_SWUC = 0x04,
	/* KBC/Mouse Interface */
	LDN_KBC_MOUSE = 0x05,
	/* KBC/Keyboard Interface */
	LDN_KBC_KEYBOARD = 0x06,
	/* Consumer IR */
	LDN_CIR = 0x0A,
	/* Shared Memory/Flash Interface */
	LDN_SMFI = 0x0F,
	/* RTC-like Timer */
	LDN_RTCT = 0x10,
	/* Power Management I/F Channel 1 */
	LDN_PMC1 = 0x11,
	/* Power Management I/F Channel 2 */
	LDN_PMC2 = 0x12,
	/* Serial Peripheral Interface */
	LDN_SSPI = 0x13,
	/* Platform Environment Control Interface */
	LDN_PECI = 0x14,
	/* Power Management I/F Channel 3 */
	LDN_PMC3 = 0x17,
	/* Power Management I/F Channel 4 */
	LDN_PMC4 = 0x18,
	/* Power Management I/F Channel 5 */
	LDN_PMC5 = 0x19,
};

/* Structure for initializing PNPCFG via ec2i. */
struct ec2i_t {
	/* index port */
	enum host_pnpcfg_index index_port;
	/* data port */
	uint8_t data_port;
};

/* EC2I access index/data port */
enum ec2i_access {
	/* index port */
	EC2I_ACCESS_INDEX = 0,
	/* data port */
	EC2I_ACCESS_DATA = 1,
};

/* EC to I-Bus Access Enabled */
#define EC2I_IBCTL_CSAE  BIT(0)
/* EC Read from I-Bus */
#define EC2I_IBCTL_CRIB  BIT(1)
/* EC Write to I-Bus */
#define EC2I_IBCTL_CWIB  BIT(2)
#define EC2I_IBCTL_CRWIB (EC2I_IBCTL_CRIB | EC2I_IBCTL_CWIB)

/* PNPCFG Register EC Access Enable */
#define EC2I_IBMAE_CFGAE BIT(0)

/*
 * KBC registers
 */
struct kbc_regs {
	/* 0x00: KBC Host Interface Control Register */
	volatile uint8_t KBHICR;
	/* 0x01: Reserved1 */
	volatile uint8_t reserved1;
	/* 0x02: KBC Interrupt Control Register */
	volatile uint8_t KBIRQR;
	/* 0x03: Reserved2 */
	volatile uint8_t reserved2;
	/* 0x04: KBC Host Interface Keyboard/Mouse Status Register */
	volatile uint8_t KBHISR;
	/* 0x05: Reserved3 */
	volatile uint8_t reserved3;
	/* 0x06: KBC Host Interface Keyboard Data Output Register */
	volatile uint8_t KBHIKDOR;
	/* 0x07: Reserved4 */
	volatile uint8_t reserved4;
	/* 0x08: KBC Host Interface Mouse Data Output Register */
	volatile uint8_t KBHIMDOR;
	/* 0x09: Reserved5 */
	volatile uint8_t reserved5;
	/* 0x0a: KBC Host Interface Keyboard/Mouse Data Input Register */
	volatile uint8_t KBHIDIR;
};

/* Output Buffer Full */
#define KBC_KBHISR_OBF      BIT(0)
/* Input Buffer Full */
#define KBC_KBHISR_IBF      BIT(1)
/* A2 Address (A2) */
#define KBC_KBHISR_A2_ADDR  BIT(3)
#define KBC_KBHISR_STS_MASK (KBC_KBHISR_OBF | KBC_KBHISR_IBF | KBC_KBHISR_A2_ADDR)

/* Clear Output Buffer Full */
#define KBC_KBHICR_COBF      BIT(6)
/* IBF/OBF Clear Mode Enable */
#define KBC_KBHICR_IBFOBFCME BIT(5)
/* Input Buffer Full CPU Interrupt Enable */
#define KBC_KBHICR_IBFCIE    BIT(3)
/* Output Buffer Empty CPU Interrupt Enable */
#define KBC_KBHICR_OBECIE    BIT(2)
/* Output Buffer Full Mouse Interrupt Enable */
#define KBC_KBHICR_OBFMIE    BIT(1)
/* Output Buffer Full Keyboard Interrupt Enable */
#define KBC_KBHICR_OBFKIE    BIT(0)

/*
 * PMC registers
 */
struct pmc_regs {
	/* 0x00: Host Interface PM Channel 1 Status */
	volatile uint8_t PM1STS;
	/* 0x01: Host Interface PM Channel 1 Data Out Port */
	volatile uint8_t PM1DO;
	/* 0x02: Host Interface PM Channel 1 Data Out Port with SCI# */
	volatile uint8_t PM1DOSCI;
	/* 0x03: Host Interface PM Channel 1 Data Out Port with SMI# */
	volatile uint8_t PM1DOSMI;
	/* 0x04: Host Interface PM Channel 1 Data In Port */
	volatile uint8_t PM1DI;
	/* 0x05: Host Interface PM Channel 1 Data In Port with SCI# */
	volatile uint8_t PM1DISCI;
	/* 0x06: Host Interface PM Channel 1 Control */
	volatile uint8_t PM1CTL;
	/* 0x07: Host Interface PM Channel 1 Interrupt Control */
	volatile uint8_t PM1IC;
	/* 0x08: Host Interface PM Channel 1 Interrupt Enable */
	volatile uint8_t PM1IE;
	/* 0x09-0x0f: Reserved1 */
	volatile uint8_t reserved1[7];
	/* 0x10: Host Interface PM Channel 2 Status */
	volatile uint8_t PM2STS;
	/* 0x11: Host Interface PM Channel 2 Data Out Port */
	volatile uint8_t PM2DO;
	/* 0x12: Host Interface PM Channel 2 Data Out Port with SCI# */
	volatile uint8_t PM2DOSCI;
	/* 0x13: Host Interface PM Channel 2 Data Out Port with SMI# */
	volatile uint8_t PM2DOSMI;
	/* 0x14: Host Interface PM Channel 2 Data In Port */
	volatile uint8_t PM2DI;
	/* 0x15: Host Interface PM Channel 2 Data In Port with SCI# */
	volatile uint8_t PM2DISCI;
	/* 0x16: Host Interface PM Channel 2 Control */
	volatile uint8_t PM2CTL;
	/* 0x17: Host Interface PM Channel 2 Interrupt Control */
	volatile uint8_t PM2IC;
	/* 0x18: Host Interface PM Channel 2 Interrupt Enable */
	volatile uint8_t PM2IE;
	/* 0x19: Mailbox Control */
	volatile uint8_t MBXCTRL;
	/* 0x1a-0x1f: Reserved2 */
	volatile uint8_t reserved2[6];
	/* 0x20: Host Interface PM Channel 3 Status */
	volatile uint8_t PM3STS;
	/* 0x21: Host Interface PM Channel 3 Data Out Port */
	volatile uint8_t PM3DO;
	/* 0x22: Host Interface PM Channel 3 Data In Port */
	volatile uint8_t PM3DI;
	/* 0x23: Host Interface PM Channel 3 Control */
	volatile uint8_t PM3CTL;
	/* 0x24: Host Interface PM Channel 3 Interrupt Control */
	volatile uint8_t PM3IC;
	/* 0x25: Host Interface PM Channel 3 Interrupt Enable */
	volatile uint8_t PM3IE;
	/* 0x26-0x2f: reserved_26_2f */
	volatile uint8_t reserved_26_2f[10];
	/* 0x30: PMC4 Status Register */
	volatile uint8_t PM4STS;
	/* 0x31: PMC4 Data Out Port */
	volatile uint8_t PM4DO;
	/* 0x32: PMC4 Data In Port */
	volatile uint8_t PM4DI;
	/* 0x33: PMC4 Control */
	volatile uint8_t PM4CTL;
	/* 0x34: PMC4 Interrupt Control */
	volatile uint8_t PM4IC;
	/* 0x35: PMC4 Interrupt Enable*/
	volatile uint8_t PM4IE;
	/* 0x36-0x3f: reserved_36_3f */
	volatile uint8_t reserved_36_3f[10];
	/* 0x40: PMC5 Status Register */
	volatile uint8_t PM5STS;
	/* 0x41: PMC5 Data Out Port */
	volatile uint8_t PM5DO;
	/* 0x42: PMC5 Data In Port */
	volatile uint8_t PM5DI;
	/* 0x43: PMC5 Control */
	volatile uint8_t PM5CTL;
	/* 0x44: PMC5 Interrupt Control */
	volatile uint8_t PM5IC;
	/* 0x45: PMC5 Interrupt Enable*/
	volatile uint8_t PM5IE;
	/* 0x46-0xff: reserved_46_ff */
	volatile uint8_t reserved_46_ff[0xba];
};

/* Input Buffer Full Interrupt Enable */
#define PMC_PM1CTL_IBFIE   BIT(0)
/* Output Buffer Full */
#define PMC_PM1STS_OBF     BIT(0)
/* Input Buffer Full */
#define PMC_PM1STS_IBF     BIT(1)
/* General Purpose Flag */
#define PMC_PM1STS_GPF     BIT(2)
/* A2 Address (A2) */
#define PMC_PM1STS_A2_ADDR BIT(3)

/* PMC2 Input Buffer Full Interrupt Enable */
#define PMC_PM2CTL_IBFIE BIT(0)
/* General Purpose Flag */
#define PMC_PM2STS_GPF   BIT(2)

/* PMC3 Input Buffer Full Interrupt Enable */
#define PMC_PM3CTL_IBFIE   BIT(0)
/* A2 Address (A2) */
#define PMC_PM3STS_A2_ADDR BIT(3)
/* Input Buffer Full Interrupt Enable */
#define PMC_PM4CTL_IBFIE   BIT(0)
/* A2 Address (A2) */
#define PMC_PM4STS_A2_ADDR BIT(3)
/* Input Buffer Full Interrupt Enable */
#define PMC_PM5CTL_IBFIE   BIT(0)
/* A2 Address (A2) */
#define PMC_PM5STS_A2_ADDR BIT(3)

/*
 * Dedicated Interrupt
 * 0b:
 * INT3: PMC Output Buffer Empty Int
 * INT25: PMC Input Buffer Full Int
 * 1b:
 * INT3: PMC1 Output Buffer Empty Int
 * INT25: PMC1 Input Buffer Full Int
 * INT26: PMC2 Output Buffer Empty Int
 * INT27: PMC2 Input Buffer Full Int
 */
#define PMC_MBXCTRL_DINT BIT(5)

/*
 * eSPI slave registers
 */
struct espi_slave_regs {
	/* 0x00-0x03: Reserved1 */
	volatile uint8_t reserved1[4];

	/* 0x04: General Capabilities and Configuration 0 */
	volatile uint8_t GCAPCFG0;
	/* 0x05: General Capabilities and Configuration 1 */
	volatile uint8_t GCAPCFG1;
	/* 0x06: General Capabilities and Configuration 2 */
	volatile uint8_t GCAPCFG2;
	/* 0x07: General Capabilities and Configuration 3 */
	volatile uint8_t GCAPCFG3;

	/* Channel 0 (Peripheral Channel) Capabilities and Configurations */
	/* 0x08: Channel 0 Capabilities and Configuration 0 */
	volatile uint8_t CH_PC_CAPCFG0;
	/* 0x09: Channel 0 Capabilities and Configuration 1 */
	volatile uint8_t CH_PC_CAPCFG1;
	/* 0x0A: Channel 0 Capabilities and Configuration 2 */
	volatile uint8_t CH_PC_CAPCFG2;
	/* 0x0B: Channel 0 Capabilities and Configuration 3 */
	volatile uint8_t CH_PC_CAPCFG3;

	/* Channel 1 (Virtual Wire Channel) Capabilities and Configurations */
	/* 0x0C: Channel 1 Capabilities and Configuration 0 */
	volatile uint8_t CH_VW_CAPCFG0;
	/* 0x0D: Channel 1 Capabilities and Configuration 1 */
	volatile uint8_t CH_VW_CAPCFG1;
	/* 0x0E: Channel 1 Capabilities and Configuration 2 */
	volatile uint8_t CH_VW_CAPCFG2;
	/* 0x0F: Channel 1 Capabilities and Configuration 3 */
	volatile uint8_t CH_VW_CAPCFG3;

	/* Channel 2 (OOB Message Channel) Capabilities and Configurations */
	/* 0x10: Channel 2 Capabilities and Configuration 0 */
	volatile uint8_t CH_OOB_CAPCFG0;
	/* 0x11: Channel 2 Capabilities and Configuration 1 */
	volatile uint8_t CH_OOB_CAPCFG1;
	/* 0x12: Channel 2 Capabilities and Configuration 2 */
	volatile uint8_t CH_OOB_CAPCFG2;
	/* 0x13: Channel 2 Capabilities and Configuration 3 */
	volatile uint8_t CH_OOB_CAPCFG3;

	/* Channel 3 (Flash Access Channel) Capabilities and Configurations */
	/* 0x14: Channel 3 Capabilities and Configuration 0 */
	volatile uint8_t CH_FLASH_CAPCFG0;
	/* 0x15: Channel 3 Capabilities and Configuration 1 */
	volatile uint8_t CH_FLASH_CAPCFG1;
	/* 0x16: Channel 3 Capabilities and Configuration 2 */
	volatile uint8_t CH_FLASH_CAPCFG2;
	/* 0x17: Channel 3 Capabilities and Configuration 3 */
	volatile uint8_t CH_FLASH_CAPCFG3;
	/* Channel 3 Capabilities and Configurations 2 */
	/* 0x18: Channel 3 Capabilities and Configuration 2-0 */
	volatile uint8_t CH_FLASH_CAPCFG2_0;
	/* 0x19: Channel 3 Capabilities and Configuration 2-1 */
	volatile uint8_t CH_FLASH_CAPCFG2_1;
	/* 0x1A: Channel 3 Capabilities and Configuration 2-2 */
	volatile uint8_t CH_FLASH_CAPCFG2_2;
	/* 0x1B: Channel 3 Capabilities and Configuration 2-3 */
	volatile uint8_t CH_FLASH_CAPCFG2_3;

	/* 0x1c-0x1f: Reserved2 */
	volatile uint8_t reserved2[4];
	/* 0x20-0x8f: Reserved3 */
	volatile uint8_t reserved3[0x70];

	/* 0x90: eSPI PC Control 0 */
	volatile uint8_t ESPCTRL0;
	/* 0x91: eSPI PC Control 1 */
	volatile uint8_t ESPCTRL1;
	/* 0x92: eSPI PC Control 2 */
	volatile uint8_t ESPCTRL2;
	/* 0x93: eSPI PC Control 3 */
	volatile uint8_t ESPCTRL3;
	/* 0x94: eSPI PC Control 4 */
	volatile uint8_t ESPCTRL4;
	/* 0x95: eSPI PC Control 5 */
	volatile uint8_t ESPCTRL5;
	/* 0x96: eSPI PC Control 6 */
	volatile uint8_t ESPCTRL6;
	/* 0x97: eSPI PC Control 7 */
	volatile uint8_t ESPCTRL7;
	/* 0x98-0x9f: Reserved4 */
	volatile uint8_t reserved4[8];

	/* 0xa0: eSPI General Control 0 */
	volatile uint8_t ESGCTRL0;
	/* 0xa1: eSPI General Control 1 */
	volatile uint8_t ESGCTRL1;
	/* 0xa2: eSPI General Control 2 */
	volatile uint8_t ESGCTRL2;
	/* 0xa3: eSPI General Control 3 */
	volatile uint8_t ESGCTRL3;
	/* 0xa4-0xaf: Reserved5 */
	volatile uint8_t reserved5[12];

	/* 0xb0: eSPI Upstream Control 0 */
	volatile uint8_t ESUCTRL0;
	/* 0xb1: eSPI Upstream Control 1 */
	volatile uint8_t ESUCTRL1;
	/* 0xb2: eSPI Upstream Control 2 */
	volatile uint8_t ESUCTRL2;
	/* 0xb3: eSPI Upstream Control 3 */
	volatile uint8_t ESUCTRL3;
	/* 0xb4-0xb5: Reserved6 */
	volatile uint8_t reserved6[2];
	/* 0xb6: eSPI Upstream Control 6 */
	volatile uint8_t ESUCTRL6;
	/* 0xb7: eSPI Upstream Control 7 */
	volatile uint8_t ESUCTRL7;
	/* 0xb8: eSPI Upstream Control 8 */
	volatile uint8_t ESUCTRL8;
	/* 0xb9-0xbf: Reserved7 */
	volatile uint8_t reserved7[7];

	/* 0xc0: eSPI OOB Control 0 */
	volatile uint8_t ESOCTRL0;
	/* 0xc1: eSPI OOB Control 1 */
	volatile uint8_t ESOCTRL1;
	/* 0xc2-0xc3: Reserved8 */
	volatile uint8_t reserved8[2];
	/* 0xc4: eSPI OOB Control 4 */
	volatile uint8_t ESOCTRL4;
	/* 0xc5-0xcf: Reserved9 */
	volatile uint8_t reserved9[11];

	/* 0xd0: eSPI SAFS Control 0 */
	volatile uint8_t ESPISAFSC0;
	/* 0xd1: eSPI SAFS Control 1 */
	volatile uint8_t ESPISAFSC1;
	/* 0xd2: eSPI SAFS Control 2 */
	volatile uint8_t ESPISAFSC2;
	/* 0xd3: eSPI SAFS Control 3 */
	volatile uint8_t ESPISAFSC3;
	/* 0xd4: eSPI SAFS Control 4 */
	volatile uint8_t ESPISAFSC4;
	/* 0xd5: eSPI SAFS Control 5 */
	volatile uint8_t ESPISAFSC5;
	/* 0xd6: eSPI SAFS Control 6 */
	volatile uint8_t ESPISAFSC6;
	/* 0xd7: eSPI SAFS Control 7 */
	volatile uint8_t ESPISAFSC7;
};

/*
 * eSPI VW registers
 */
struct espi_vw_regs {
	/* 0x00-0x7f: VW index */
	volatile uint8_t VW_INDEX[0x80];
	/* 0x80-0x8f: Reserved1 */
	volatile uint8_t reserved1[0x10];
	/* 0x90: VW Control 0 */
	volatile uint8_t VWCTRL0;
	/* 0x91: VW Control 1 */
	volatile uint8_t VWCTRL1;
	/* 0x92: VW Control 2 */
	volatile uint8_t VWCTRL2;
	/* 0x93: VW Control 3 */
	volatile uint8_t VWCTRL3;
	/* 0x94: Reserved2 */
	volatile uint8_t reserved2;
	/* 0x95: VW Control 5 */
	volatile uint8_t VWCTRL5;
	/* 0x96: VW Control 6 */
	volatile uint8_t VWCTRL6;
	/* 0x97: VW Control 7 */
	volatile uint8_t VWCTRL7;
	/* 0x98-0x99: Reserved3 */
	volatile uint8_t reserved3[2];
};

#define ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE 80
/*
 * eSPI Queue 0 registers
 */
struct espi_queue0_regs {
	/* 0x00-0x3f: PUT_PC Data Byte 0-63 */
	volatile uint8_t PUT_PC_DATA[0x40];
	/* 0x40-0x7f: Reserved1 */
	volatile uint8_t reserved1[0x40];
	/* 0x80-0xcf: PUT_OOB Data Byte 0-79 */
	volatile uint8_t PUT_OOB_DATA[ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE];
};

/*
 * eSPI Queue 1 registers
 */
struct espi_queue1_regs {
	/* 0x00-0x4f: Upstream Data Byte 0-79 */
	volatile uint8_t UPSTREAM_DATA[ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE];
	/* 0x50-0x7f: Reserved1 */
	volatile uint8_t reserved1[0x30];
	/* 0x80-0xbf: PUT_FLASH_NP Data Byte 0-63 */
	volatile uint8_t PUT_FLASH_NP_DATA[0x40];
};

/* H2RAM Path Select. 1b: H2RAM through LPC IO cycle. */
#define SMFI_H2RAMPS       BIT(4)
/* H2RAM Window 1 Enable */
#define SMFI_H2RAMW1E      BIT(1)
/* H2RAM Window 0 Enable */
#define SMFI_H2RAMW0E      BIT(0)
/* Host RAM Window x Write Protect Enable (All protected) */
#define SMFI_HRAMWXWPE_ALL (BIT(5) | BIT(4))

/* Accept Port 80h Cycle */
#define IT8XXX2_GCTRL_ACP80 BIT(6)
/* Accept Port 81h Cycle */
#define IT8XXX2_GCTRL_ACP81 BIT(3)

#define IT8XXX2_GPIO_GCR_ESPI_RST_D2      0x2
#define IT8XXX2_GPIO_GCR_ESPI_RST_POS     1
#define IT8XXX2_GPIO_GCR_ESPI_RST_EN_MASK (0x3 << IT8XXX2_GPIO_GCR_ESPI_RST_POS)

/*
 * VCC Detector Option.
 * bit[7-6] = 1: The VCC power status is treated as power-on.
 * The VCC supply of eSPI and related functions (EC2I, KBC, PMC and
 * PECI). It means VCC should be logic high before using these
 * functions, or firmware treats VCC logic high.
 */
#define IT8XXX2_GCTRL_VCCDO_MASK   (BIT(6) | BIT(7))
#define IT8XXX2_GCTRL_VCCDO_VCC_ON BIT(6)
/*
 * bit[3] = 0: The reset source of PNPCFG is RSTPNP bit in RSTCH
 * register and WRST#.
 */
#define IT8XXX2_GCTRL_HGRST        BIT(3)
/* bit[2] = 1: Enable global reset. */
#define IT8XXX2_GCTRL_GRST         BIT(2)

/*
 * IT8XXX2 register structure size/offset checking macro function to mitigate
 * the risk of unexpected compiling results.
 */
#define IT8XXX2_ESPI_REG_SIZE_CHECK(reg_def, size)                                                 \
	BUILD_ASSERT(sizeof(struct reg_def) == size, "Failed in size check of register "           \
						     "structure!")
#define IT8XXX2_ESPI_REG_OFFSET_CHECK(reg_def, member, offset)                                     \
	BUILD_ASSERT(offsetof(struct reg_def, member) == offset,                                   \
		     "Failed in offset check of register structure member!")

/* EC2I register structure check */
IT8XXX2_ESPI_REG_SIZE_CHECK(ec2i_regs, 0x06);
IT8XXX2_ESPI_REG_OFFSET_CHECK(ec2i_regs, IHIOA, 0x00);
IT8XXX2_ESPI_REG_OFFSET_CHECK(ec2i_regs, IHD, 0x01);
IT8XXX2_ESPI_REG_OFFSET_CHECK(ec2i_regs, LSIOHA, 0x02);
IT8XXX2_ESPI_REG_OFFSET_CHECK(ec2i_regs, IBMAE, 0x04);
IT8XXX2_ESPI_REG_OFFSET_CHECK(ec2i_regs, IBCTL, 0x05);

/* KBC register structure check */
IT8XXX2_ESPI_REG_SIZE_CHECK(kbc_regs, 0x0b);
IT8XXX2_ESPI_REG_OFFSET_CHECK(kbc_regs, KBHICR, 0x00);
IT8XXX2_ESPI_REG_OFFSET_CHECK(kbc_regs, KBIRQR, 0x02);
IT8XXX2_ESPI_REG_OFFSET_CHECK(kbc_regs, KBHISR, 0x04);
IT8XXX2_ESPI_REG_OFFSET_CHECK(kbc_regs, KBHIKDOR, 0x06);
IT8XXX2_ESPI_REG_OFFSET_CHECK(kbc_regs, KBHIMDOR, 0x08);
IT8XXX2_ESPI_REG_OFFSET_CHECK(kbc_regs, KBHIDIR, 0x0a);

/* PMC register structure check */
IT8XXX2_ESPI_REG_SIZE_CHECK(pmc_regs, 0x100);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM1STS, 0x00);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM1DO, 0x01);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM1DI, 0x04);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM1CTL, 0x06);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM2STS, 0x10);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM2DO, 0x11);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM2DI, 0x14);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM2CTL, 0x16);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, MBXCTRL, 0x19);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM3STS, 0x20);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM3DO, 0x21);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM3DI, 0x22);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM3CTL, 0x23);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM3IC, 0x24);
IT8XXX2_ESPI_REG_OFFSET_CHECK(pmc_regs, PM3IE, 0x25);

/* eSPI slave register structure check */
IT8XXX2_ESPI_REG_SIZE_CHECK(espi_slave_regs, 0xd8);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, GCAPCFG1, 0x05);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, CH_PC_CAPCFG3, 0x0b);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, CH_VW_CAPCFG3, 0x0f);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, CH_OOB_CAPCFG3, 0x13);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, CH_FLASH_CAPCFG3, 0x17);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, CH_FLASH_CAPCFG2_3, 0x1b);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESPCTRL0, 0x90);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL0, 0xa0);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL1, 0xa1);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL2, 0xa2);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESUCTRL0, 0xb0);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESOCTRL0, 0xc0);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESOCTRL1, 0xc1);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESPISAFSC0, 0xd0);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_slave_regs, ESPISAFSC7, 0xd7);

/* eSPI vw register structure check */
IT8XXX2_ESPI_REG_SIZE_CHECK(espi_vw_regs, 0x9a);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_vw_regs, VW_INDEX, 0x00);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_vw_regs, VWCTRL0, 0x90);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_vw_regs, VWCTRL1, 0x91);

/* eSPI Queue 0 registers structure check */
IT8XXX2_ESPI_REG_SIZE_CHECK(espi_queue0_regs, 0xd0);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_queue0_regs, PUT_OOB_DATA, 0x80);

/* eSPI Queue 1 registers structure check */
IT8XXX2_ESPI_REG_SIZE_CHECK(espi_queue1_regs, 0xc0);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_queue1_regs, UPSTREAM_DATA, 0x00);
IT8XXX2_ESPI_REG_OFFSET_CHECK(espi_queue1_regs, PUT_FLASH_NP_DATA, 0x80);

/* Register used to record VWx data transmitted to the eSPI host. */
#define IT8XXX2_ESPI_VW_REC_VW4 0xe1
#define IT8XXX2_ESPI_VW_REC_VW5 0xe2
#define IT8XXX2_ESPI_VW_REC_VW6 0xe3
#define IT8XXX2_ESPI_VW_REC_VW40 0xe4

struct espi_it8xxx2_wuc {
	/* WUC control device structure */
	const struct device *wucs;
	/* WUC pin mask */
	uint8_t mask;
};

struct espi_it8xxx2_config {
	uintptr_t base_espi_slave;
	uintptr_t base_espi_vw;
	uintptr_t base_espi_queue1;
	uintptr_t base_espi_queue0;
	uintptr_t base_ec2i;
	uintptr_t base_kbc;
	uintptr_t base_pmc;
	uintptr_t base_smfi;
	const struct espi_it8xxx2_wuc wuc;
};

struct espi_it8xxx2_data {
	sys_slist_t callbacks;
#ifdef CONFIG_ESPI_OOB_CHANNEL
	struct k_sem oob_upstream_go;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	struct k_sem flash_upstream_go;
	uint8_t put_flash_cycle_type;
	uint8_t put_flash_tag;
	uint8_t put_flash_len;
	uint8_t flash_buf[IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE];
#endif
};

struct vw_channel_t {
	uint8_t vw_index;    /* VW index of signal */
	uint8_t level_mask;  /* level bit of signal */
	uint8_t valid_mask;  /* valid bit of signal */
	uint8_t vw_sent_reg; /* vw signal sent to host */
};

struct vwidx_isr_t {
	void (*vwidx_isr)(const struct device *dev, uint8_t update_flag);
	uint8_t vw_index;
};

enum espi_ch_enable_isr_type {
	DEASSERTED_FLAG = 0,
	ASSERTED_FLAG = 1,
};

struct espi_isr_t {
	void (*espi_isr)(const struct device *dev, bool enable);
	enum espi_ch_enable_isr_type isr_type;
};

struct espi_vw_signal_t {
	enum espi_vwire_signal signal;
	void (*vw_signal_isr)(const struct device *dev);
};

/* EC2I bridge and PNPCFG devices */
static const struct ec2i_t kbc_settings[] = {
	/* Select logical device 06h(keyboard) */
	{HOST_INDEX_LDN, LDN_KBC_KEYBOARD},
	/* Set IRQ=01h for logical device */
	{HOST_INDEX_IRQNUMX, 0x01},
	/* Configure IRQTP for KBC. */
	/*
	 * Interrupt request type select (IRQTP) for KBC.
	 * bit 1, 0: IRQ request is buffered and applied to SERIRQ
	 *        1: IRQ request is inverted before being applied to SERIRQ
	 * bit 0, 0: Edge triggered mode
	 *        1: Level triggered mode
	 *
	 * This interrupt configuration should the same on both host and EC side
	 */
	{HOST_INDEX_IRQTP, 0x02},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},

#ifdef CONFIG_ESPI_IT8XXX2_PNPCFG_DEVICE_KBC_MOUSE
	/* Select logical device 05h(mouse) */
	{HOST_INDEX_LDN, LDN_KBC_MOUSE},
	/* Set IRQ=0Ch for logical device */
	{HOST_INDEX_IRQNUMX, 0x0C},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
#endif
};

static const struct ec2i_t pmc1_settings[] = {
	/* Select logical device 11h(PM1 ACPI) */
	{HOST_INDEX_LDN, LDN_PMC1},
	/* Set IRQ=00h for logical device */
	{HOST_INDEX_IRQNUMX, 0x00},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
#define IT8XXX2_ESPI_HOST_IO_PVT_DATA_PORT_MSB                                                     \
	((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM >> 8) & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT_DATA_PORT_LSB (CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT_CMD_PORT_MSB                                                      \
	(((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM + 4) >> 8) & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT_CMD_PORT_LSB                                                      \
	((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM + 4) & 0xff)
static const struct ec2i_t pmc3_settings[] = {
	/* Select logical device 17h(PMC3) */
	{HOST_INDEX_LDN, LDN_PMC3},
	/* I/O Port Base Address (data/command ports) */
	{HOST_INDEX_IOBAD0_MSB, IT8XXX2_ESPI_HOST_IO_PVT_DATA_PORT_MSB},
	{HOST_INDEX_IOBAD0_LSB, IT8XXX2_ESPI_HOST_IO_PVT_DATA_PORT_LSB},
	{HOST_INDEX_IOBAD1_MSB, IT8XXX2_ESPI_HOST_IO_PVT_CMD_PORT_MSB},
	{HOST_INDEX_IOBAD1_LSB, IT8XXX2_ESPI_HOST_IO_PVT_CMD_PORT_LSB},
	/* Set IRQ=00h for logical device */
	{HOST_INDEX_IRQNUMX, 0x00},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2
#define IT8XXX2_ESPI_HOST_IO_PVT2_DATA_PORT_MSB                                                    \
	((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2_PORT_NUM >> 8) & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT2_DATA_PORT_LSB                                                    \
	(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2_PORT_NUM & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT2_CMD_PORT_MSB                                                     \
	(((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2_PORT_NUM + 4) >> 8) & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT2_CMD_PORT_LSB                                                     \
	((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2_PORT_NUM + 4) & 0xff)
static const struct ec2i_t pmc4_settings[] = {
	/* Select logical device 18h(PMC4) */
	{HOST_INDEX_LDN, LDN_PMC4},
	/* I/O Port Base Address (data/command ports) */
	{HOST_INDEX_IOBAD0_MSB, IT8XXX2_ESPI_HOST_IO_PVT2_DATA_PORT_MSB},
	{HOST_INDEX_IOBAD0_LSB, IT8XXX2_ESPI_HOST_IO_PVT2_DATA_PORT_LSB},
	{HOST_INDEX_IOBAD1_MSB, IT8XXX2_ESPI_HOST_IO_PVT2_CMD_PORT_MSB},
	{HOST_INDEX_IOBAD1_LSB, IT8XXX2_ESPI_HOST_IO_PVT2_CMD_PORT_LSB},
	/* Set IRQ=00h for logical device */
	{HOST_INDEX_IRQNUMX, 0x00},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2 */

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3
#define IT8XXX2_ESPI_HOST_IO_PVT3_DATA_PORT_MSB                                                    \
	((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3_PORT_NUM >> 8) & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT3_DATA_PORT_LSB                                                    \
	(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3_PORT_NUM & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT3_CMD_PORT_MSB                                                     \
	(((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3_PORT_NUM + 4) >> 8) & 0xff)
#define IT8XXX2_ESPI_HOST_IO_PVT3_CMD_PORT_LSB                                                     \
	((CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3_PORT_NUM + 4) & 0xff)
static const struct ec2i_t pmc5_settings[] = {
	/* Select logical device 19h(PMC5) */
	{HOST_INDEX_LDN, LDN_PMC5},
	/* I/O Port Base Address (data/command ports) */
	{HOST_INDEX_IOBAD0_MSB, IT8XXX2_ESPI_HOST_IO_PVT3_DATA_PORT_MSB},
	{HOST_INDEX_IOBAD0_LSB, IT8XXX2_ESPI_HOST_IO_PVT3_DATA_PORT_LSB},
	{HOST_INDEX_IOBAD1_MSB, IT8XXX2_ESPI_HOST_IO_PVT3_CMD_PORT_MSB},
	{HOST_INDEX_IOBAD1_LSB, IT8XXX2_ESPI_HOST_IO_PVT3_CMD_PORT_LSB},
	/* Set IRQ=00h for logical device */
	{HOST_INDEX_IRQNUMX, 0x00},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3 */

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
#define IT8XXX2_ESPI_HC_DATA_PORT_MSB \
	((CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM >> 8) & 0xff)
#define IT8XXX2_ESPI_HC_DATA_PORT_LSB \
	(CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM & 0xff)
#define IT8XXX2_ESPI_HC_CMD_PORT_MSB \
	(((CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM + 4) >> 8) & 0xff)
#define IT8XXX2_ESPI_HC_CMD_PORT_LSB \
	((CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM + 4) & 0xff)
static const struct ec2i_t pmc2_settings[] = {
	/* Select logical device 12h(PM2 host command) */
	{HOST_INDEX_LDN, LDN_PMC2},
	/* I/O Port Base Address (data/command ports) */
	{HOST_INDEX_IOBAD0_MSB, IT8XXX2_ESPI_HC_DATA_PORT_MSB},
	{HOST_INDEX_IOBAD0_LSB, IT8XXX2_ESPI_HC_DATA_PORT_LSB},
	{HOST_INDEX_IOBAD1_MSB, IT8XXX2_ESPI_HC_CMD_PORT_MSB},
	{HOST_INDEX_IOBAD1_LSB, IT8XXX2_ESPI_HC_CMD_PORT_LSB},
	/* Set IRQ=00h for logical device */
	{HOST_INDEX_IRQNUMX, 0x00},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
/*
 * Host to RAM (H2RAM) memory mapping.
 * This feature allows host access EC's memory directly by eSPI I/O cycles.
 * Mapping range is 4K bytes and base address is adjustable.
 * Eg. the I/O cycle 800h~8ffh from host can be mapped to x800h~x8ffh.
 * Linker script will make the pool 4K aligned.
 */
#define IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX 0x1000
#define IT8XXX2_ESPI_H2RAM_OFFSET_MASK   GENMASK(5, 0)
#define IT8XXX2_ESPI_H2RAM_BASEADDR_MASK GENMASK(19, 0)

#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
#define H2RAM_ACPI_SHM_MAX ((CONFIG_ESPI_IT8XXX2_ACPI_SHM_H2RAM_SIZE) + \
			(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM))
#if (H2RAM_ACPI_SHM_MAX > IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX)
#error "ACPI shared memory region out of h2ram"
#endif
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
#define H2RAM_EC_HOST_CMD_MAX ((CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE) + \
			(CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM))
#if (H2RAM_EC_HOST_CMD_MAX > IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX)
#error "EC host command parameters out of h2ram"
#endif
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
#if (MIN(H2RAM_ACPI_SHM_MAX, H2RAM_EC_HOST_CMD_MAX) > \
	MAX(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM, \
		CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM))
#error "ACPI and HC sections of h2ram overlap"
#endif
#endif

static uint8_t h2ram_pool[MAX(H2RAM_ACPI_SHM_MAX, H2RAM_EC_HOST_CMD_MAX)]
					__attribute__((section(".h2ram_pool")));

#define H2RAM_WINDOW_SIZE(ram_size) ((find_msb_set((ram_size) / 16) - 1) & 0x7)

static const struct ec2i_t smfi_settings[] = {
	/* Select logical device 0Fh(SMFI) */
	{HOST_INDEX_LDN, LDN_SMFI},
	/* Internal RAM base address on eSPI I/O space */
	{HOST_INDEX_DSLDC6, 0x00},
	/* Enable H2RAM eSPI I/O cycle */
	{HOST_INDEX_DSLDC7, 0x01},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};

static void smfi_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct smfi_ite_ec_regs *const smfi_reg = (struct smfi_ite_ec_regs *)config->base_smfi;

#ifdef CONFIG_SOC_SERIES_IT8XXX2
	struct gctrl_ite_ec_regs *const gctrl = ESPI_ITE_GET_GCTRL_BASE;
	uint8_t h2ram_offset;

	/* Set the host to RAM cycle address offset */
	h2ram_offset = ((uint32_t)h2ram_pool & IT8XXX2_ESPI_H2RAM_BASEADDR_MASK) /
		       IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX;
	gctrl->GCTRL_H2ROFSR =
		(gctrl->GCTRL_H2ROFSR & ~IT8XXX2_ESPI_H2RAM_OFFSET_MASK) |
		h2ram_offset;
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	memset(&h2ram_pool[CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM], 0,
			CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE);
	/* Set host RAM window 0 base address */
	smfi_reg->SMFI_HRAMW0BA =
		(CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM >> 4) & 0xff;
	/* Set host RAM window 0 size. (allow R/W) */
	smfi_reg->SMFI_HRAMW0AAS =
		H2RAM_WINDOW_SIZE(CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE);
	/* Enable window 0, H2RAM through IO cycle */
	smfi_reg->SMFI_HRAMWC |= (SMFI_H2RAMPS | SMFI_H2RAMW0E);
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	memset(&h2ram_pool[CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM], 0,
			CONFIG_ESPI_IT8XXX2_ACPI_SHM_H2RAM_SIZE);
	/* Set host RAM window 1 base address */
	smfi_reg->SMFI_HRAMW1BA =
		(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM >> 4) & 0xff;
	/* Set host RAM window 1 size. (read-only) */
	smfi_reg->SMFI_HRAMW1AAS =
		H2RAM_WINDOW_SIZE(CONFIG_ESPI_IT8XXX2_ACPI_SHM_H2RAM_SIZE) |
		SMFI_HRAMWXWPE_ALL;
	/* Enable window 1, H2RAM through IO cycle */
	smfi_reg->SMFI_HRAMWC |= (SMFI_H2RAMPS | SMFI_H2RAMW1E);
#endif
}
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD ||
	* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	*/

static void ec2i_it8xxx2_wait_status_cleared(const struct device *dev,
						uint8_t mask)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct ec2i_regs *const ec2i = (struct ec2i_regs *)config->base_ec2i;

	while (ec2i->IBCTL & mask) {
		;
	}
}

static void ec2i_it8xxx2_write_pnpcfg(const struct device *dev,
					enum ec2i_access sel, uint8_t data)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct ec2i_regs *const ec2i = (struct ec2i_regs *)config->base_ec2i;

	/* bit0: EC to I-Bus access enabled. */
	ec2i->IBCTL |= EC2I_IBCTL_CSAE;
	/*
	 * Wait that both CRIB and CWIB bits in IBCTL register
	 * are cleared.
	 */
	ec2i_it8xxx2_wait_status_cleared(dev, EC2I_IBCTL_CRWIB);
	/* Enable EC access to the PNPCFG registers */
	ec2i->IBMAE |= EC2I_IBMAE_CFGAE;
	/* Set indirect host I/O offset. */
	ec2i->IHIOA = sel;
	/* Write the data to IHD register */
	ec2i->IHD = data;
	/* Wait the CWIB bit in IBCTL cleared. */
	ec2i_it8xxx2_wait_status_cleared(dev, EC2I_IBCTL_CWIB);
	/* Disable EC access to the PNPCFG registers. */
	ec2i->IBMAE &= ~EC2I_IBMAE_CFGAE;
	/* Disable EC to I-Bus access. */
	ec2i->IBCTL &= ~EC2I_IBCTL_CSAE;
}

static void ec2i_it8xxx2_write(const struct device *dev,
				enum host_pnpcfg_index index, uint8_t data)
{
	/* Set index */
	ec2i_it8xxx2_write_pnpcfg(dev, EC2I_ACCESS_INDEX, index);
	/* Set data */
	ec2i_it8xxx2_write_pnpcfg(dev, EC2I_ACCESS_DATA, data);
}

static void pnpcfg_it8xxx2_configure(const struct device *dev,
					const struct ec2i_t *settings,
					size_t entries)
{
	for (size_t i = 0; i < entries; i++) {
		ec2i_it8xxx2_write(dev, settings[i].index_port,
					settings[i].data_port);
	}
}

#define PNPCFG(_s)						\
	pnpcfg_it8xxx2_configure(dev, _s##_settings, ARRAY_SIZE(_s##_settings))
extern uint8_t _h2ram_pool_start[];

static void pnpcfg_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct ec2i_regs *const ec2i = (struct ec2i_regs *)config->base_ec2i;
	struct gctrl_ite_ec_regs *const gctrl = ESPI_ITE_GET_GCTRL_BASE;

	/* The register pair to access PNPCFG is 004Eh and 004Fh */
	gctrl->GCTRL_BADRSEL = 0x1;
	/* Host access is disabled */
	ec2i->LSIOHA |= 0x3;
	/* configure pnpcfg devices */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_8042_KBC)) {
		PNPCFG(kbc);
	}
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO)) {
		PNPCFG(pmc1);
	}
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	PNPCFG(pmc2);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
	PNPCFG(pmc3);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2
	PNPCFG(pmc4);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3
	PNPCFG(pmc5);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	PNPCFG(smfi);

#ifdef CONFIG_SOC_SERIES_IT51XXX
	uint8_t h2ram_pool_idx;

	h2ram_pool_idx = ((uint32_t)_h2ram_pool_start & IT8XXX2_ESPI_H2RAM_BASEADDR_MASK) /
			 IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX;
	/* H2RAM 4K page select */
	ec2i_it8xxx2_write(dev, HOST_INDEX_DSLDC13, h2ram_pool_idx);
#endif
#endif
}

/* KBC (port 60h/64h) */
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
static void kbc_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct kbc_regs *const kbc_reg = (struct kbc_regs *)config->base_kbc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA
	};
	struct espi_evt_data_kbc *kbc_evt =
			(struct espi_evt_data_kbc *)&evt.evt_data;

	/* KBC Input Buffer Full event */
	kbc_evt->evt = HOST_KBC_EVT_IBF;
	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	kbc_evt->type = !!(kbc_reg->KBHISR & KBC_KBHISR_A2_ADDR);
	/* The data in KBC Input Buffer */
	kbc_evt->data = kbc_reg->KBHIDIR;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void kbc_it8xxx2_obe_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct kbc_regs *const kbc_reg = (struct kbc_regs *)config->base_kbc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA
	};
	struct espi_evt_data_kbc *kbc_evt =
				(struct espi_evt_data_kbc *)&evt.evt_data;

	/* Disable KBC OBE interrupt first */
	kbc_reg->KBHICR &= ~KBC_KBHICR_OBECIE;

	/* Notify application that host already read out data. */
	kbc_evt->evt = HOST_KBC_EVT_OBE;
	kbc_evt->data = 0;
	kbc_evt->type = 0;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void kbc_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct kbc_regs *const kbc_reg = (struct kbc_regs *)config->base_kbc;

	/* Disable KBC serirq IRQ */
	kbc_reg->KBIRQR = 0;

	/*
	 * bit3: Input Buffer Full CPU Interrupt Enable.
	 * bit1: Enable the interrupt to mouse driver in the host processor via
	 *       SERIRQ when the output buffer is full.
	 * bit0: Enable the interrupt to keyboard driver in the host processor
	 *       via SERIRQ when the output buffer is full
	 */
	kbc_reg->KBHICR |=
		(KBC_KBHICR_IBFCIE | KBC_KBHICR_OBFKIE | KBC_KBHICR_OBFMIE);

	/* Input Buffer Full CPU Interrupt Enable. */
	IRQ_CONNECT(IT8XXX2_KBC_IBF_IRQ, 0, kbc_it8xxx2_ibf_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_KBC_IBF_IRQ);

	/* Output Buffer Empty CPU Interrupt Enable */
	IRQ_CONNECT(IT8XXX2_KBC_OBE_IRQ, 0, kbc_it8xxx2_obe_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_KBC_OBE_IRQ);
}
#endif

/* PMC 1 (APCI port 62h/66h) */
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
static void pmc1_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO,
		ESPI_PERIPHERAL_NODATA
	};
	struct espi_evt_data_acpi *acpi_evt =
				(struct espi_evt_data_acpi *)&evt.evt_data;

	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	acpi_evt->type = !!(pmc_reg->PM1STS & PMC_PM1STS_A2_ADDR);
	/* Set processing flag before reading command byte */
	pmc_reg->PM1STS |= PMC_PM1STS_GPF;
	acpi_evt->data = pmc_reg->PM1DI;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void pmc1_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;

	/* Enable pmc1 input buffer full interrupt */
	pmc_reg->PM1CTL |= PMC_PM1CTL_IBFIE;
	IRQ_CONNECT(IT8XXX2_PMC1_IBF_IRQ, 0, pmc1_it8xxx2_ibf_isr,
			DEVICE_DT_INST_GET(0), 0);
	if (!IS_ENABLED(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)) {
		irq_enable(IT8XXX2_PMC1_IBF_IRQ);
	}
}
#endif

/* Port 80 */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
static void port80_it8xxx2_isr(const struct device *dev)
{
	struct espi_it8xxx2_data *const data = dev->data;
	struct gctrl_ite_ec_regs *const gctrl = ESPI_ITE_GET_GCTRL_BASE;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		(ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
		ESPI_PERIPHERAL_NODATA
	};

	if (IS_ENABLED(CONFIG_ESPI_IT8XXX2_PORT_81_CYCLE)) {
		evt.evt_data = gctrl->GCTRL_P80HDR | (gctrl->GCTRL_P81HDR << 8);
	} else {
		evt.evt_data = gctrl->GCTRL_P80HDR;
	}
	/* Write 1 to clear this bit */
	gctrl->GCTRL_P80H81HSR |= BIT(0);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void port80_it8xxx2_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct gctrl_ite_ec_regs *const gctrl = ESPI_ITE_GET_GCTRL_BASE;

	/* Accept Port 80h (and 81h) Cycle */
	if (IS_ENABLED(CONFIG_ESPI_IT8XXX2_PORT_81_CYCLE)) {
		gctrl->GCTRL_SPCTRL1 |=
			(IT8XXX2_GCTRL_ACP80 | IT8XXX2_GCTRL_ACP81);
	} else {
		gctrl->GCTRL_SPCTRL1 |= IT8XXX2_GCTRL_ACP80;
	}
	IRQ_CONNECT(IT8XXX2_PORT_80_IRQ, 0, port80_it8xxx2_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_PORT_80_IRQ);
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
/* PMC 2 (Host command port CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM) */
static void pmc2_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_EC_HOST_CMD,
		ESPI_PERIPHERAL_NODATA
	};

	/* Set processing flag before reading command byte */
	pmc_reg->PM2STS |= PMC_PM2STS_GPF;
	evt.evt_data = pmc_reg->PM2DI;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void pmc2_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;

	/* Dedicated interrupt for PMC2 */
	pmc_reg->MBXCTRL |= PMC_MBXCTRL_DINT;
	/* Enable pmc2 input buffer full interrupt */
	pmc_reg->PM2CTL |= PMC_PM2CTL_IBFIE;
	IRQ_CONNECT(IT8XXX2_PMC2_IBF_IRQ, 0, pmc2_it8xxx2_ibf_isr,
			DEVICE_DT_INST_GET(0), 0);
	if (!IS_ENABLED(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)) {
		irq_enable(IT8XXX2_PMC2_IBF_IRQ);
	}
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
/* PMC3 (Host private port) */
static void pmc3_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = ESPI_PERIPHERAL_HOST_IO_PVT,
				 .evt_data = ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_pvt *pvt_evt = (struct espi_evt_data_pvt *)&evt.evt_data;

	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	pvt_evt->type = !!(pmc_reg->PM3STS & PMC_PM3STS_A2_ADDR);
	pvt_evt->data = pmc_reg->PM3DI;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void pmc3_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;

	/* Enable pmc3 input buffer full interrupt */
	pmc_reg->PM3CTL |= PMC_PM3CTL_IBFIE;
	IRQ_CONNECT(IT8XXX2_PMC3_IBF_IRQ, 0, pmc3_it8xxx2_ibf_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_PMC3_IBF_IRQ);
}
#endif

/* PMC4 (Host private port 2) */
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2
static void pmc4_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = ESPI_PERIPHERAL_HOST_IO_PVT2,
				 .evt_data = ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_pvt *pvt_evt = (struct espi_evt_data_pvt *)&evt.evt_data;

	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	pvt_evt->type = !!(pmc_reg->PM4STS & PMC_PM4STS_A2_ADDR);
	pvt_evt->data = pmc_reg->PM4DI;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void pmc4_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;

	/* Enable pmc4 input buffer full interrupt */
	pmc_reg->PM4CTL |= PMC_PM4CTL_IBFIE;
	IRQ_CONNECT(IT8XXX2_PMC4_IBF_IRQ, 0, pmc4_it8xxx2_ibf_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_PMC4_IBF_IRQ);
}
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2 */

/* PMC5 (Host private port 3) */
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3
static void pmc5_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = ESPI_PERIPHERAL_HOST_IO_PVT3,
				 .evt_data = ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_pvt *pvt_evt = (struct espi_evt_data_pvt *)&evt.evt_data;

	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	pvt_evt->type = !!(pmc_reg->PM5STS & PMC_PM5STS_A2_ADDR);
	pvt_evt->data = pmc_reg->PM5DI;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void pmc5_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;

	/* Enable pmc5 input buffer full interrupt */
	pmc_reg->PM5CTL |= PMC_PM5CTL_IBFIE;
	IRQ_CONNECT(IT8XXX2_PMC5_IBF_IRQ, 0, pmc5_it8xxx2_ibf_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_PMC5_IBF_IRQ);
}
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3 */

#define IT8XXX2_ESPI_VW_SEND_TIMEOUT_US (USEC_PER_MSEC * 10)

/* eSPI api functions */
#define VW_CHAN(signal, index, level, valid, reg) \
	[signal] = {.vw_index = index, .level_mask = level, \
			.valid_mask = valid, .vw_sent_reg = reg}

/* VW signals used in eSPI */
static const struct vw_channel_t vw_channel_list[] = {
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_S3,        0x02, BIT(0), BIT(4), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_S4,        0x02, BIT(1), BIT(5), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_S5,        0x02, BIT(2), BIT(6), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_OOB_RST_WARN,  0x03, BIT(2), BIT(6), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_PLTRST,        0x03, BIT(1), BIT(5), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_STAT,      0x03, BIT(0), BIT(4), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_NMIOUT,        0x07, BIT(2), BIT(6), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SMIOUT,        0x07, BIT(1), BIT(5), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_HOST_RST_WARN, 0x07, BIT(0), BIT(4), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_A,         0x41, BIT(3), BIT(7), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, 0x41, BIT(1), BIT(5), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_WARN,      0x41, BIT(0), BIT(4), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_WLAN,      0x42, BIT(1), BIT(5), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_LAN,       0x42, BIT(0), BIT(4), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_HOST_C10,      0x47, BIT(0), BIT(4), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_DNX_WARN,      0x4a, BIT(1), BIT(5), 0),
	VW_CHAN(ESPI_VWIRE_SIGNAL_PME,           0x04, BIT(3), BIT(7), IT8XXX2_ESPI_VW_REC_VW4),
	VW_CHAN(ESPI_VWIRE_SIGNAL_WAKE,          0x04, BIT(2), BIT(6), IT8XXX2_ESPI_VW_REC_VW4),
	VW_CHAN(ESPI_VWIRE_SIGNAL_OOB_RST_ACK,   0x04, BIT(0), BIT(4), IT8XXX2_ESPI_VW_REC_VW4),
	VW_CHAN(ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS,  0x05, BIT(3), BIT(7), IT8XXX2_ESPI_VW_REC_VW5),
	VW_CHAN(ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, 0x05, BIT(2), BIT(6), IT8XXX2_ESPI_VW_REC_VW5),
	VW_CHAN(ESPI_VWIRE_SIGNAL_ERR_FATAL,     0x05, BIT(1), BIT(5), IT8XXX2_ESPI_VW_REC_VW5),
	VW_CHAN(ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 0x05, BIT(0), BIT(4), IT8XXX2_ESPI_VW_REC_VW5),
	VW_CHAN(ESPI_VWIRE_SIGNAL_HOST_RST_ACK,  0x06, BIT(3), BIT(7), IT8XXX2_ESPI_VW_REC_VW6),
	VW_CHAN(ESPI_VWIRE_SIGNAL_RST_CPU_INIT,  0x06, BIT(2), BIT(6), IT8XXX2_ESPI_VW_REC_VW6),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SMI,           0x06, BIT(1), BIT(5), IT8XXX2_ESPI_VW_REC_VW6),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SCI,           0x06, BIT(0), BIT(4), IT8XXX2_ESPI_VW_REC_VW6),
	VW_CHAN(ESPI_VWIRE_SIGNAL_DNX_ACK,       0x40, BIT(1), BIT(5), IT8XXX2_ESPI_VW_REC_VW40),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_ACK,       0x40, BIT(0), BIT(4), IT8XXX2_ESPI_VW_REC_VW40),
};

static int espi_it8xxx2_configure(const struct device *dev,
					struct espi_cfg *cfg)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	uint8_t capcfg1 = 0;

	/* Set frequency */
	switch (cfg->max_freq) {
	case 20:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_20;
		break;
	case 25:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_25;
		break;
	case 33:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_33;
		break;
	case 50:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_50;
		break;
	case 66:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_66;
		break;
	default:
		return -EINVAL;
	}
	slave_reg->GCAPCFG1 =
		(slave_reg->GCAPCFG1 & ~IT8XXX2_ESPI_MAX_FREQ_MASK) | capcfg1;

	/*
	 * Configure eSPI I/O mode. (Register read only)
	 * Supported I/O mode : single, dual and quad.
	 */

	/* Configure eSPI supported channels. (Register read only)
	 * Supported channels: peripheral, virtual wire, OOB, and flash access.
	 */

	return 0;
}

static bool espi_it8xxx2_channel_ready(const struct device *dev,
					enum espi_channel ch)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	bool sts = false;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = slave_reg->CH_PC_CAPCFG3 & IT8XXX2_ESPI_PC_READY_MASK;
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = slave_reg->CH_VW_CAPCFG3 & IT8XXX2_ESPI_VW_READY_MASK;
		break;
	case ESPI_CHANNEL_OOB:
		sts = slave_reg->CH_OOB_CAPCFG3 & IT8XXX2_ESPI_OOB_READY_MASK;
		break;
	case ESPI_CHANNEL_FLASH:
		sts = slave_reg->CH_FLASH_CAPCFG3 & IT8XXX2_ESPI_FC_READY_MASK;
		break;
	default:
		break;
	}

	return sts;
}

static int espi_it8xxx2_send_vwire(const struct device *dev,
			enum espi_vwire_signal signal, uint8_t level)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	uint8_t vw_index = vw_channel_list[signal].vw_index;
	uint8_t level_mask = vw_channel_list[signal].level_mask;
	uint8_t valid_mask = vw_channel_list[signal].valid_mask;
	uint8_t vw_sent = vw_channel_list[signal].vw_sent_reg;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	if (level) {
		vw_reg->VW_INDEX[vw_index] |= level_mask;
	} else {
		vw_reg->VW_INDEX[vw_index] &= ~level_mask;
	}

	vw_reg->VW_INDEX[vw_index] |= valid_mask;

	if (espi_it8xxx2_channel_ready(dev, ESPI_CHANNEL_VWIRE) && vw_sent) {
		if (!WAIT_FOR(vw_reg->VW_INDEX[vw_index] ==
			sys_read8(config->base_espi_vw + vw_sent),
			IT8XXX2_ESPI_VW_SEND_TIMEOUT_US, k_busy_wait(10))) {
			LOG_WRN("VW send to host has timed out vw[0x%x] = 0x%x",
			vw_index, vw_reg->VW_INDEX[vw_index]);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int espi_it8xxx2_receive_vwire(const struct device *dev,
				  enum espi_vwire_signal signal, uint8_t *level)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	uint8_t vw_index = vw_channel_list[signal].vw_index;
	uint8_t level_mask = vw_channel_list[signal].level_mask;
	uint8_t valid_mask = vw_channel_list[signal].valid_mask;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_ESPI_VWIRE_VALID_BIT_CHECK)) {
		if (vw_reg->VW_INDEX[vw_index] & valid_mask) {
			*level = !!(vw_reg->VW_INDEX[vw_index] & level_mask);
		} else {
			/* Not valid */
			*level = 0;
		}
	} else {
		*level = !!(vw_reg->VW_INDEX[vw_index] & level_mask);
	}

	return 0;
}

#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
static void host_custom_opcode_enable_interrupts(void)
{
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO)) {
		irq_enable(IT8XXX2_PMC1_IBF_IRQ);
	}
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)) {
		irq_enable(IT8XXX2_PMC2_IBF_IRQ);
	}
}

static void host_custom_opcode_disable_interrupts(void)
{
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO)) {
		irq_disable(IT8XXX2_PMC1_IBF_IRQ);
	}
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)) {
		irq_disable(IT8XXX2_PMC2_IBF_IRQ);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */

static int espi_it8xxx2_manage_callback(const struct device *dev,
				    struct espi_callback *callback, bool set)
{
	struct espi_it8xxx2_data *const data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_it8xxx2_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t *data)
{
	const struct espi_it8xxx2_config *const config = dev->config;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_regs *const kbc_reg =
			(struct kbc_regs *)config->base_kbc;

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(kbc_reg->KBHISR & KBC_KBHISR_OBF);
			break;
		case E8042_IBF_HAS_CHAR:
			*data = !!(kbc_reg->KBHISR & KBC_KBHISR_IBF);
			break;
		case E8042_READ_KB_STS:
			*data = kbc_reg->KBHISR;
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct pmc_regs *const pmc_reg =
			(struct pmc_regs *)config->base_pmc;

		switch (op) {
		case EACPI_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(pmc_reg->PM1STS & PMC_PM1STS_OBF);
			break;
		case EACPI_IBF_HAS_CHAR:
			*data = !!(pmc_reg->PM1STS & PMC_PM1STS_IBF);
			break;
		case EACPI_READ_STS:
			*data = pmc_reg->PM1STS;
			break;
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
		case EACPI_GET_SHARED_MEMORY:
			*data = (uint32_t)&h2ram_pool[
			CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM];
			break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
		default:
			return -EINVAL;
		}
	}
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {

		switch (op) {
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
			*data = (uint32_t)&h2ram_pool[
				CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM];
			break;
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE:
			*data = CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE;
			break;
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	else {
		return -ENOTSUP;
	}

	return 0;
}

static int espi_it8xxx2_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	const struct espi_it8xxx2_config *const config = dev->config;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_regs *const kbc_reg =
			(struct kbc_regs *)config->base_kbc;

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			kbc_reg->KBHIKDOR = (*data & 0xff);
			/*
			 * Enable OBE interrupt after putting data in
			 * data register.
			 */
			kbc_reg->KBHICR |= KBC_KBHICR_OBECIE;
			break;
		case E8042_WRITE_MB_CHAR:
			kbc_reg->KBHIMDOR = (*data & 0xff);
			/*
			 * Enable OBE interrupt after putting data in
			 * data register.
			 */
			kbc_reg->KBHICR |= KBC_KBHICR_OBECIE;
			break;
		case E8042_RESUME_IRQ:
			/* Enable KBC IBF interrupt */
			irq_enable(IT8XXX2_KBC_IBF_IRQ);
			break;
		case E8042_PAUSE_IRQ:
			/* Disable KBC IBF interrupt */
			irq_disable(IT8XXX2_KBC_IBF_IRQ);
			break;
		case E8042_CLEAR_OBF:
			volatile uint8_t _kbhicr __unused;
			/*
			 * After enabling IBF/OBF clear mode, we have to make
			 * sure that IBF interrupt is not triggered before
			 * disabling the clear mode. Or the interrupt will keep
			 * triggering until the watchdog is reset.
			 */
			unsigned int key = irq_lock();
			/*
			 * When IBFOBFCME is enabled, write 1 to COBF bit to
			 * clear KBC OBF.
			 */
			kbc_reg->KBHICR |= KBC_KBHICR_IBFOBFCME;
			kbc_reg->KBHICR |= KBC_KBHICR_COBF;
			kbc_reg->KBHICR &= ~KBC_KBHICR_COBF;
			/* Disable clear mode */
			kbc_reg->KBHICR &= ~KBC_KBHICR_IBFOBFCME;
			/*
			 * I/O access synchronization, this load operation will
			 * guarantee the above modification of SOC's register
			 * can be seen by any following instructions.
			 */
			_kbhicr = kbc_reg->KBHICR;
			irq_unlock(key);
			break;
		case E8042_SET_FLAG:
			kbc_reg->KBHISR |= (*data & 0xff);
			break;
		case E8042_CLEAR_FLAG:
			kbc_reg->KBHISR &= ~(*data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct pmc_regs *const pmc_reg =
			(struct pmc_regs *)config->base_pmc;

		switch (op) {
		case EACPI_WRITE_CHAR:
			pmc_reg->PM1DO = (*data & 0xff);
			break;
		case EACPI_WRITE_STS:
			pmc_reg->PM1STS = (*data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	}
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		struct pmc_regs *const pmc_reg =
			(struct pmc_regs *)config->base_pmc;

		switch (op) {
		/* Enable/Disable PMCx interrupt */
		case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
			if (*data) {
				host_custom_opcode_enable_interrupts();
			} else {
				host_custom_opcode_disable_interrupts();
			}
			break;
		case ECUSTOM_HOST_CMD_SEND_RESULT:
			/* Write result to data output port (set OBF status) */
			pmc_reg->PM2DO = (*data & 0xff);
			/* Clear processing flag */
			pmc_reg->PM2STS &= ~PMC_PM2STS_GPF;
			break;
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	else {
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_ESPI_OOB_CHANNEL
/* eSPI cycle type field */
#define ESPI_OOB_CYCLE_TYPE          0x21
#define ESPI_OOB_TAG                 0x00
#define ESPI_OOB_TIMEOUT_MS          200

/* eSPI tag + len[11:8] field */
#define ESPI_TAG_LEN_FIELD(tag, len) \
		   ((((tag) & 0xF) << 4) | (((len) >> 8) & 0xF))

struct espi_oob_msg_packet {
	FLEXIBLE_ARRAY_DECLARE(uint8_t, data_byte);
};

static int espi_it8xxx2_send_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;
	struct espi_oob_msg_packet *oob_pckt =
		(struct espi_oob_msg_packet *)pckt->buf;

	if (!(slave_reg->CH_OOB_CAPCFG3 & IT8XXX2_ESPI_OOB_READY_MASK)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

	if (slave_reg->ESUCTRL0 & IT8XXX2_ESPI_UPSTREAM_BUSY) {
		LOG_ERR("%s: OOB upstream busy", __func__);
		return -EIO;
	}

	if (pckt->len > ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE) {
		LOG_ERR("%s: Out of OOB queue space", __func__);
		return -EINVAL;
	}

	/* Set cycle type */
	slave_reg->ESUCTRL1 = IT8XXX2_ESPI_CYCLE_TYPE_OOB;
	/* Set tag and length[11:8] */
	slave_reg->ESUCTRL2 = ESPI_TAG_LEN_FIELD(0, pckt->len);
	/* Set length [7:0] */
	slave_reg->ESUCTRL3 = pckt->len & 0xff;

	/* Set data byte */
	for (int i = 0; i < pckt->len; i++) {
		queue1_reg->UPSTREAM_DATA[i] = oob_pckt->data_byte[i];
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	return 0;
}

static int espi_it8xxx2_receive_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue0_regs *const queue0_reg =
		(struct espi_queue0_regs *)config->base_espi_queue0;
	struct espi_oob_msg_packet *oob_pckt =
		(struct espi_oob_msg_packet *)pckt->buf;
	uint8_t oob_len;

	if (!(slave_reg->CH_OOB_CAPCFG3 & IT8XXX2_ESPI_OOB_READY_MASK)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_it8xxx2_data *const data = dev->data;
	int ret;

	/* Wait until receive OOB message or timeout */
	ret = k_sem_take(&data->oob_upstream_go, K_MSEC(ESPI_OOB_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}
#endif

	/* Get length */
	oob_len = (slave_reg->ESOCTRL4 & IT8XXX2_ESPI_PUT_OOB_LEN_MASK);
	/*
	 * Buffer passed to driver isn't enough.
	 * The first three bytes of buffer are cycle type, tag, and length.
	 */
	if (oob_len > pckt->len) {
		LOG_ERR("%s: Out of rx buf %d vs %d", __func__,
			oob_len, pckt->len);
		return -EINVAL;
	}

	pckt->len = oob_len;
	/* Get data byte */
	for (int i = 0; i < oob_len; i++) {
		oob_pckt->data_byte[i] = queue0_reg->PUT_OOB_DATA[i];
	}

	return 0;
}

static void espi_it8xxx2_oob_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_it8xxx2_data *const data = dev->data;

	k_sem_init(&data->oob_upstream_go, 0, 1);
#endif

	/* Upstream interrupt enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_INTERRUPT_ENABLE;

	/* PUT_OOB interrupt enable */
	slave_reg->ESOCTRL1 |= IT8XXX2_ESPI_PUT_OOB_INTERRUPT_ENABLE;
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#define ESPI_FLASH_TAG                      0x01
#define ESPI_FLASH_READ_TIMEOUT_MS          200
#define ESPI_FLASH_WRITE_TIMEOUT_MS         500
#define ESPI_FLASH_ERASE_TIMEOUT_MS         1000

/* Successful completion without data */
#define ESPI_IT8XXX2_PUT_FLASH_C_SCWOD      0
/* Successful completion with data */
#define ESPI_IT8XXX2_PUT_FLASH_C_SCWD       4

enum espi_flash_cycle_type {
	IT8XXX2_ESPI_CYCLE_TYPE_FLASH_READ = 0x08,
	IT8XXX2_ESPI_CYCLE_TYPE_FLASH_WRITE = 0x09,
	IT8XXX2_ESPI_CYCLE_TYPE_FLASH_ERASE = 0x0A,
};

static int espi_it8xxx2_flash_trans(const struct device *dev,
				struct espi_flash_packet *pckt,
				enum espi_flash_cycle_type tran)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;

	if (!(slave_reg->CH_FLASH_CAPCFG3 & IT8XXX2_ESPI_FC_READY_MASK)) {
		LOG_ERR("%s: Flash channel isn't ready (tran:%d)",
			__func__, tran);
		return -EIO;
	}

	if (slave_reg->ESUCTRL0 & IT8XXX2_ESPI_UPSTREAM_BUSY) {
		LOG_ERR("%s: Upstream busy (tran:%d)", __func__, tran);
		return -EIO;
	}

	if (pckt->len > IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE) {
		LOG_ERR("%s: Invalid size request (tran:%d)", __func__, tran);
		return -EINVAL;
	}

	/* Set cycle type */
	slave_reg->ESUCTRL1 = tran;
	/* Set tag and length[11:8] */
	slave_reg->ESUCTRL2 = (ESPI_FLASH_TAG << 4);
	/*
	 * Set length [7:0]
	 * Note: for erasing, the least significant 3 bit of the length field
	 * specifies the size of the block to be erased:
	 * 001b: 4 Kbytes
	 * 010b: 64Kbytes
	 * 100b: 128 Kbytes
	 * 101b: 256 Kbytes
	 */
	slave_reg->ESUCTRL3 = pckt->len;
	/* Set flash address */
	queue1_reg->UPSTREAM_DATA[0] = (pckt->flash_addr >> 24) & 0xff;
	queue1_reg->UPSTREAM_DATA[1] = (pckt->flash_addr >> 16) & 0xff;
	queue1_reg->UPSTREAM_DATA[2] = (pckt->flash_addr >> 8) & 0xff;
	queue1_reg->UPSTREAM_DATA[3] = pckt->flash_addr & 0xff;

	return 0;
}

static int espi_it8xxx2_flash_read(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	int ret;

	ret = espi_it8xxx2_flash_trans(dev, pckt,
					IT8XXX2_ESPI_CYCLE_TYPE_FLASH_READ);
	if (ret) {
		return ret;
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	/* Wait until upstream done or timeout */
	ret = k_sem_take(&data->flash_upstream_go,
			K_MSEC(ESPI_FLASH_READ_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (data->put_flash_cycle_type != ESPI_IT8XXX2_PUT_FLASH_C_SCWD) {
		LOG_ERR("%s: Unsuccessful completion", __func__);
		return -EIO;
	}

	memcpy(pckt->buf, data->flash_buf, pckt->len);

	LOG_INF("%s: read (%d) bytes from flash over espi", __func__,
		data->put_flash_len);

	return 0;
}

static int espi_it8xxx2_flash_write(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;
	int ret;

	ret = espi_it8xxx2_flash_trans(dev, pckt,
					IT8XXX2_ESPI_CYCLE_TYPE_FLASH_WRITE);
	if (ret) {
		return ret;
	}

	/* Set data byte */
	for (int i = 0; i < pckt->len; i++) {
		queue1_reg->UPSTREAM_DATA[4 + i] = pckt->buf[i];
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	/* Wait until upstream done or timeout */
	ret = k_sem_take(&data->flash_upstream_go,
			K_MSEC(ESPI_FLASH_WRITE_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (data->put_flash_cycle_type != ESPI_IT8XXX2_PUT_FLASH_C_SCWOD) {
		LOG_ERR("%s: Unsuccessful completion", __func__);
		return -EIO;
	}

	return 0;
}

static int espi_it8xxx2_flash_erase(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	int ret;

	ret = espi_it8xxx2_flash_trans(dev, pckt,
					IT8XXX2_ESPI_CYCLE_TYPE_FLASH_ERASE);
	if (ret) {
		return ret;
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	/* Wait until upstream done or timeout */
	ret = k_sem_take(&data->flash_upstream_go,
			K_MSEC(ESPI_FLASH_ERASE_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (data->put_flash_cycle_type != ESPI_IT8XXX2_PUT_FLASH_C_SCWOD) {
		LOG_ERR("%s: Unsuccessful completion", __func__);
		return -EIO;
	}

	return 0;
}

static void espi_it8xxx2_flash_upstream_done_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;

	data->put_flash_cycle_type = slave_reg->ESUCTRL6;
	data->put_flash_tag = slave_reg->ESUCTRL7 &
				IT8XXX2_ESPI_PUT_FLASH_TAG_MASK;
	data->put_flash_len = slave_reg->ESUCTRL8 &
				IT8XXX2_ESPI_PUT_FLASH_LEN_MASK;

	if (slave_reg->ESUCTRL1 == IT8XXX2_ESPI_CYCLE_TYPE_FLASH_READ) {
		if (data->put_flash_len > IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE) {
			LOG_ERR("%s: Invalid size (%d)", __func__,
							data->put_flash_len);
		} else {
			for (int i = 0; i < data->put_flash_len; i++) {
				data->flash_buf[i] =
					queue1_reg->UPSTREAM_DATA[i];
			}
		}
	}

	k_sem_give(&data->flash_upstream_go);
}

static void espi_it8xxx2_flash_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	k_sem_init(&data->flash_upstream_go, 0, 1);

	/* Upstream interrupt enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_INTERRUPT_ENABLE;
}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

/* eSPI driver registration */
static int espi_it8xxx2_init(const struct device *dev);

static DEVICE_API(espi, espi_it8xxx2_driver_api) = {
	.config = espi_it8xxx2_configure,
	.get_channel_status = espi_it8xxx2_channel_ready,
	.send_vwire = espi_it8xxx2_send_vwire,
	.receive_vwire = espi_it8xxx2_receive_vwire,
	.manage_callback = espi_it8xxx2_manage_callback,
	.read_lpc_request = espi_it8xxx2_read_lpc_request,
	.write_lpc_request = espi_it8xxx2_write_lpc_request,
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_it8xxx2_send_oob,
	.receive_oob = espi_it8xxx2_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_it8xxx2_flash_read,
	.flash_write = espi_it8xxx2_flash_write,
	.flash_erase = espi_it8xxx2_flash_erase,
#endif
};

static void espi_it8xxx2_vw_notify_system_state(const struct device *dev,
				enum espi_vwire_signal signal)
{
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_event evt = {ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0};
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev, signal, &level);

	evt.evt_details = signal;
	evt.evt_data = level;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void espi_vw_signal_no_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static const struct espi_vw_signal_t vwidx2_signals[] = {
	{ESPI_VWIRE_SIGNAL_SLP_S3, NULL},
	{ESPI_VWIRE_SIGNAL_SLP_S4, NULL},
	{ESPI_VWIRE_SIGNAL_SLP_S5, NULL},
};

static void espi_it8xxx2_vwidx2_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx2_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx2_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_vw_oob_rst_warn_isr(const struct device *dev)
{
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_WARN, &level);
	espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK, level);
}

static void espi_vw_pltrst_isr(const struct device *dev)
{
	uint8_t pltrst = 0;

	espi_it8xxx2_receive_vwire(dev, ESPI_VWIRE_SIGNAL_PLTRST, &pltrst);

	if (pltrst) {
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_SMI, 1);
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_SCI, 1);
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 1);
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 1);
	}

	LOG_INF("VW PLTRST_L %sasserted", pltrst ? "de" : "");
}

static const struct espi_vw_signal_t vwidx3_signals[] = {
	{ESPI_VWIRE_SIGNAL_OOB_RST_WARN, espi_vw_oob_rst_warn_isr},
	{ESPI_VWIRE_SIGNAL_PLTRST,       espi_vw_pltrst_isr},
};

static void espi_it8xxx2_vwidx3_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx3_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx3_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			vwidx3_signals[i].vw_signal_isr(dev);
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_vw_host_rst_warn_isr(const struct device *dev)
{
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev,
		ESPI_VWIRE_SIGNAL_HOST_RST_WARN, &level);
	espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, level);
}

static const struct espi_vw_signal_t vwidx7_signals[] = {
	{ESPI_VWIRE_SIGNAL_HOST_RST_WARN, espi_vw_host_rst_warn_isr},
};

static void espi_it8xxx2_vwidx7_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx7_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx7_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			vwidx7_signals[i].vw_signal_isr(dev);
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_vw_sus_warn_isr(const struct device *dev)
{
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_WARN, &level);
	espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK, level);
}

static const struct espi_vw_signal_t vwidx41_signals[] = {
	{ESPI_VWIRE_SIGNAL_SUS_WARN,      espi_vw_sus_warn_isr},
	{ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, espi_vw_signal_no_isr},
	{ESPI_VWIRE_SIGNAL_SLP_A,         espi_vw_signal_no_isr},
};

static void espi_it8xxx2_vwidx41_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx41_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx41_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			vwidx41_signals[i].vw_signal_isr(dev);
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static const struct espi_vw_signal_t vwidx42_signals[] = {
	{ESPI_VWIRE_SIGNAL_SLP_LAN, NULL},
	{ESPI_VWIRE_SIGNAL_SLP_WLAN, NULL},
};

static void espi_it8xxx2_vwidx42_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx42_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx42_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_it8xxx2_vwidx43_isr(const struct device *dev,
					uint8_t updated_flag)
{
	ARG_UNUSED(dev);
	/*
	 * We haven't send callback to system because there is no index 43
	 * virtual wire signal is listed in enum espi_vwire_signal.
	 */
	LOG_INF("vw isr %s is ignored!", __func__);
}

static void espi_it8xxx2_vwidx44_isr(const struct device *dev,
					uint8_t updated_flag)
{
	ARG_UNUSED(dev);
	/*
	 * We haven't send callback to system because there is no index 44
	 * virtual wire signal is listed in enum espi_vwire_signal.
	 */
	LOG_INF("vw isr %s is ignored!", __func__);
}

static const struct espi_vw_signal_t vwidx47_signals[] = {
	{ESPI_VWIRE_SIGNAL_HOST_C10, NULL},
};
static void espi_it8xxx2_vwidx47_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx47_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx47_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

/*
 * The ISR of espi VW interrupt in array needs to match bit order in
 * ESPI VW VWCTRL1 register.
 */
static const struct vwidx_isr_t vwidx_isr_list[] = {
	[0] = {espi_it8xxx2_vwidx2_isr,  0x02},
	[1] = {espi_it8xxx2_vwidx3_isr,  0x03},
	[2] = {espi_it8xxx2_vwidx7_isr,  0x07},
	[3] = {espi_it8xxx2_vwidx41_isr, 0x41},
	[4] = {espi_it8xxx2_vwidx42_isr, 0x42},
	[5] = {espi_it8xxx2_vwidx43_isr, 0x43},
	[6] = {espi_it8xxx2_vwidx44_isr, 0x44},
	[7] = {espi_it8xxx2_vwidx47_isr, 0x47},
};

/*
 * This is used to record the previous VW valid/level field state to discover
 * changes. Then do following sequence only when state is changed.
 */
static uint8_t vwidx_cached_flag[ARRAY_SIZE(vwidx_isr_list)];

static void espi_it8xxx2_reset_vwidx_cache(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;

	/* reset vwidx_cached_flag */
	for (int i = 0; i < ARRAY_SIZE(vwidx_isr_list); i++) {
		vwidx_cached_flag[i] =
			vw_reg->VW_INDEX[vwidx_isr_list[i].vw_index];
	}
}

static void espi_it8xxx2_vw_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	uint8_t vwidx_updated = vw_reg->VWCTRL1;

	/* write-1 to clear */
	vw_reg->VWCTRL1 = vwidx_updated;

	for (int i = 0; i < ARRAY_SIZE(vwidx_isr_list); i++) {
		if (vwidx_updated & BIT(i)) {
			uint8_t vw_flag;

			vw_flag = vw_reg->VW_INDEX[vwidx_isr_list[i].vw_index];
			vwidx_isr_list[i].vwidx_isr(dev,
					vwidx_cached_flag[i] ^ vw_flag);
			vwidx_cached_flag[i] = vw_flag;
		}
	}
}

static void espi_it8xxx2_ch_notify_system_state(const struct device *dev,
						enum espi_channel ch, bool en)
{
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
		.evt_details = ch,
		.evt_data = en,
	};

	espi_send_callbacks(&data->callbacks, dev, evt);
}

/*
 * Peripheral channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "Peripheral Channel Enable" bit.
 */
static void espi_it8xxx2_peripheral_ch_en_isr(const struct device *dev,
						bool enable)
{
	espi_it8xxx2_ch_notify_system_state(dev,
					ESPI_CHANNEL_PERIPHERAL, enable);
}

/*
 * VW channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "Virtual Wire Channel Enable" bit.
 */
static void espi_it8xxx2_vw_ch_en_isr(const struct device *dev, bool enable)
{
	espi_it8xxx2_ch_notify_system_state(dev, ESPI_CHANNEL_VWIRE, enable);
}

/*
 * OOB message channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "OOB Message Channel Enable" bit.
 */
static void espi_it8xxx2_oob_ch_en_isr(const struct device *dev, bool enable)
{
	espi_it8xxx2_ch_notify_system_state(dev, ESPI_CHANNEL_OOB, enable);
}

/*
 * Flash channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "Flash Access Channel Enable" bit.
 */
static void espi_it8xxx2_flash_ch_en_isr(const struct device *dev, bool enable)
{
	if (enable) {
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		espi_it8xxx2_send_vwire(dev,
					ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);
	}

	espi_it8xxx2_ch_notify_system_state(dev, ESPI_CHANNEL_FLASH, enable);
}

static void espi_it8xxx2_put_pc_status_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	/*
	 * TODO: To check cycle type (bit[3-0] at ESPCTRL0) and make
	 * corresponding modification if needed.
	 */
	LOG_INF("isr %s is ignored!", __func__);

	/* write-1-clear to release PC_FREE */
	slave_reg->ESPCTRL0 = IT8XXX2_ESPI_INTERRUPT_PUT_PC;
}

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_it8xxx2_upstream_channel_disable_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	LOG_INF("isr %s is ignored!", __func__);

	/* write-1 to clear this bit */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_CHANNEL_DISABLE;
}

static void espi_it8xxx2_put_oob_status_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_OOB_RECEIVED,
				  .evt_details = 0,
				  .evt_data = 0 };
#endif

	/* Write-1 to clear this bit for the next coming posted transaction. */
	slave_reg->ESOCTRL0 |= IT8XXX2_ESPI_PUT_OOB_STATUS;

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_give(&data->oob_upstream_go);
#else
	/* Additional detail is length field of PUT_OOB message packet. */
	evt.evt_details = (slave_reg->ESOCTRL4 & IT8XXX2_ESPI_PUT_OOB_LEN_MASK);
	espi_send_callbacks(&data->callbacks, dev, evt);
#endif
}
#endif

#if defined(CONFIG_ESPI_OOB_CHANNEL) || defined(CONFIG_ESPI_FLASH_CHANNEL)
static void espi_it8xxx2_upstream_done_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* cycle type is flash read, write, or erase */
	if (slave_reg->ESUCTRL1 != IT8XXX2_ESPI_CYCLE_TYPE_OOB) {
		espi_it8xxx2_flash_upstream_done_isr(dev);
	}
#endif

	/* write-1 to clear this bit */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_DONE;
	/* upstream disable */
	slave_reg->ESUCTRL0 &= ~IT8XXX2_ESPI_UPSTREAM_ENABLE;
}
#endif

/*
 * The ISR of espi interrupt event in array need to be matched bit order in
 * IT8XXX2 ESPI ESGCTRL0 register.
 */
static const struct espi_isr_t espi_isr_list[] = {
	[0] = {espi_it8xxx2_peripheral_ch_en_isr, ASSERTED_FLAG},
	[1] = {espi_it8xxx2_vw_ch_en_isr,         ASSERTED_FLAG},
	[2] = {espi_it8xxx2_oob_ch_en_isr,        ASSERTED_FLAG},
	[3] = {espi_it8xxx2_flash_ch_en_isr,      ASSERTED_FLAG},
	[4] = {espi_it8xxx2_peripheral_ch_en_isr, DEASSERTED_FLAG},
	[5] = {espi_it8xxx2_vw_ch_en_isr,         DEASSERTED_FLAG},
	[6] = {espi_it8xxx2_oob_ch_en_isr,        DEASSERTED_FLAG},
	[7] = {espi_it8xxx2_flash_ch_en_isr,      DEASSERTED_FLAG},
};

static void espi_it8xxx2_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	/* get espi interrupt events */
	uint8_t espi_event = slave_reg->ESGCTRL0;
#if defined(CONFIG_ESPI_OOB_CHANNEL) || defined(CONFIG_ESPI_FLASH_CHANNEL)
	uint8_t espi_upstream = slave_reg->ESUCTRL0;
#endif

	/* write-1 to clear */
	slave_reg->ESGCTRL0 = espi_event;

	/* process espi interrupt events */
	for (int i = 0; i < ARRAY_SIZE(espi_isr_list); i++) {
		if (espi_event & BIT(i)) {
			espi_isr_list[i].espi_isr(dev,
				espi_isr_list[i].isr_type);
		}
	}

	/*
	 * bit7: the peripheral has received a peripheral posted/completion.
	 * This bit indicates the peripheral has received a packet from eSPI
	 * peripheral channel.
	 */
	if (slave_reg->ESPCTRL0 & IT8XXX2_ESPI_INTERRUPT_PUT_PC) {
		espi_it8xxx2_put_pc_status_isr(dev);
	}

#ifdef CONFIG_ESPI_OOB_CHANNEL
	/*
	 * The corresponding channel of the eSPI upstream transaction is
	 * disabled.
	 */
	if (espi_upstream & IT8XXX2_ESPI_UPSTREAM_CHANNEL_DISABLE) {
		espi_it8xxx2_upstream_channel_disable_isr(dev);
	}

	/* The eSPI slave has received a PUT_OOB message. */
	if (slave_reg->ESOCTRL0 & IT8XXX2_ESPI_PUT_OOB_STATUS) {
		espi_it8xxx2_put_oob_status_isr(dev);
	}
#endif

	/* eSPI oob and flash channels use the same interrupt of upstream. */
#if defined(CONFIG_ESPI_OOB_CHANNEL) || defined(CONFIG_ESPI_FLASH_CHANNEL)
	/* The eSPI upstream transaction is done. */
	if (espi_upstream & IT8XXX2_ESPI_UPSTREAM_DONE) {
		espi_it8xxx2_upstream_done_isr(dev);
	}
#endif
}

void espi_it8xxx2_enable_pad_ctrl(const struct device *dev, bool enable)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	if (enable) {
		/* Enable eSPI pad. */
		slave_reg->ESGCTRL2 &= ~IT8XXX2_ESPI_INPUT_PAD_GATING;
	} else {
		/* Disable eSPI pad. */
		slave_reg->ESGCTRL2 |= IT8XXX2_ESPI_INPUT_PAD_GATING;
	}
}

void espi_it8xxx2_enable_trans_irq(const struct device *dev, bool enable)
{
	const struct espi_it8xxx2_config *const config = dev->config;

	if (enable) {
		irq_enable(IT8XXX2_TRANS_IRQ);
	} else {
		irq_disable(IT8XXX2_TRANS_IRQ);
		/* Clear pending interrupt */
#ifdef CONFIG_SOC_SERIES_IT51XXX
		it51xxx_wuc_clear_status(config->wuc.wucs, config->wuc.mask);
#else
		it8xxx2_wuc_clear_status(config->wuc.wucs, config->wuc.mask);
#endif
	}
}

static void espi_it8xxx2_trans_isr(const struct device *dev)
{
	/*
	 * This interrupt is only used to wake up CPU, there is no need to do
	 * anything in the isr in addition to disable interrupt.
	 */
	espi_it8xxx2_enable_trans_irq(dev, false);
}

void espi_it8xxx2_espi_reset_isr(const struct device *port,
				struct gpio_callback *cb, uint32_t pins)
{
	struct espi_it8xxx2_data *const data = ESPI_IT8XXX2_SOC_DEV->data;
	struct espi_event evt = {ESPI_BUS_RESET, 0, 0};
	bool espi_reset = gpio_pin_get(port, (find_msb_set(pins) - 1));

	if (!(espi_reset)) {
		/* Reset vwidx_cached_flag[] when espi_reset# asserted. */
		espi_it8xxx2_reset_vwidx_cache(ESPI_IT8XXX2_SOC_DEV);
	}

	evt.evt_data = espi_reset;
	espi_send_callbacks(&data->callbacks, ESPI_IT8XXX2_SOC_DEV, evt);

	LOG_INF("eSPI reset %sasserted", espi_reset ? "de" : "");
}

/* eSPI reset# is enabled on GPD2 */
#define ESPI_IT8XXX2_ESPI_RESET_PORT DEVICE_DT_GET(DT_NODELABEL(gpiod))
#define ESPI_IT8XXX2_ESPI_RESET_PIN  2
static void espi_it8xxx2_enable_reset(void)
{
	struct gpio_ite_ec_regs *const gpio_regs = GPIO_ITE_EC_REGS_BASE;
	static struct gpio_callback espi_reset_cb;

	/* eSPI reset is enabled on GPD2 */
	gpio_regs->GPIO_GCR =
		(gpio_regs->GPIO_GCR & ~IT8XXX2_GPIO_GCR_ESPI_RST_EN_MASK) |
		(IT8XXX2_GPIO_GCR_ESPI_RST_D2 << IT8XXX2_GPIO_GCR_ESPI_RST_POS);
	/* enable eSPI reset isr */
	gpio_init_callback(&espi_reset_cb, espi_it8xxx2_espi_reset_isr,
				BIT(ESPI_IT8XXX2_ESPI_RESET_PIN));
	gpio_add_callback(ESPI_IT8XXX2_ESPI_RESET_PORT, &espi_reset_cb);
	gpio_pin_interrupt_configure(ESPI_IT8XXX2_ESPI_RESET_PORT,
					ESPI_IT8XXX2_ESPI_RESET_PIN,
					GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_BOTH);
}

static struct espi_it8xxx2_data espi_it8xxx2_data_0;
static const struct espi_it8xxx2_config espi_it8xxx2_config_0 = {
	.base_espi_slave = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.base_espi_vw = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.base_espi_queue0 = DT_INST_REG_ADDR_BY_IDX(0, 2),
	.base_espi_queue1 = DT_INST_REG_ADDR_BY_IDX(0, 3),
	.base_ec2i = DT_INST_REG_ADDR_BY_IDX(0, 4),
	.base_kbc = DT_INST_REG_ADDR_BY_IDX(0, 5),
	.base_pmc = DT_INST_REG_ADDR_BY_IDX(0, 6),
	.base_smfi = DT_INST_REG_ADDR_BY_IDX(0, 7),
	.wuc = IT8XXX2_DT_WUC_ITEMS_FUNC(0, 0),
};

DEVICE_DT_INST_DEFINE(0, &espi_it8xxx2_init, NULL,
		    &espi_it8xxx2_data_0, &espi_it8xxx2_config_0,
		    PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
		    &espi_it8xxx2_driver_api);

static int espi_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct gctrl_ite_ec_regs *const gctrl = ESPI_ITE_GET_GCTRL_BASE;

	/* configure VCC detector */
	gctrl->GCTRL_RSTS = (gctrl->GCTRL_RSTS &
			~(IT8XXX2_GCTRL_VCCDO_MASK | IT8XXX2_GCTRL_HGRST)) |
			(IT8XXX2_GCTRL_VCCDO_VCC_ON | IT8XXX2_GCTRL_GRST);

	/* enable PNPCFG devices */
	pnpcfg_it8xxx2_init(dev);

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	/* enable kbc port (60h/64h) */
	kbc_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	/* enable pmc1 for ACPI port (62h/66h) */
	pmc1_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	/* Accept Port 80h Cycle */
	port80_it8xxx2_init(dev);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	smfi_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	/* enable pmc2 for host command port */
	pmc2_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
	/* enable pmc3 for host private port */
	pmc3_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2
	/* enable pmc4 for host private port */
	pmc4_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3
	/* enable pmc5 for host private port */
	pmc5_it8xxx2_init(dev);
#endif

	/* Reset vwidx_cached_flag[] at initialization */
	espi_it8xxx2_reset_vwidx_cache(dev);

	/* Enable espi vw interrupt */
	vw_reg->VWCTRL0 |= IT8XXX2_ESPI_VW_INTERRUPT_ENABLE;
	IRQ_CONNECT(IT8XXX2_ESPI_VW_IRQ, 0, espi_it8xxx2_vw_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_ESPI_VW_IRQ);

	/* Reset PLTRST# virtual wire signal during eSPI reset */
	vw_reg->VWCTRL2 |= IT8XXX2_ESPI_VW_RESET_PLTRST;

#ifdef CONFIG_ESPI_OOB_CHANNEL
	espi_it8xxx2_oob_init(dev);
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	espi_it8xxx2_flash_init(dev);
#endif

	/* Enable espi interrupt */
	slave_reg->ESGCTRL1 |= IT8XXX2_ESPI_INTERRUPT_ENABLE;
	IRQ_CONNECT(IT8XXX2_ESPI_IRQ, 0, espi_it8xxx2_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_ESPI_IRQ);

	/* enable interrupt and reset from eSPI_reset# */
	espi_it8xxx2_enable_reset();

	/*
	 * Enable eSPI to WUC.
	 * If an eSPI transaction is accepted, WU42 interrupt will be asserted.
	 */
	slave_reg->ESGCTRL2 |= IT8XXX2_ESPI_TO_WUC_ENABLE;

	/* Enable WU42 of WUI */
#ifdef CONFIG_SOC_SERIES_IT51XXX
	it51xxx_wuc_clear_status(config->wuc.wucs, config->wuc.mask);
	it51xxx_wuc_enable(config->wuc.wucs, config->wuc.mask);
#else
	it8xxx2_wuc_clear_status(config->wuc.wucs, config->wuc.mask);
	it8xxx2_wuc_enable(config->wuc.wucs, config->wuc.mask);
#endif
	/*
	 * Only register isr here, the interrupt only need to be enabled
	 * before CPU and RAM clocks gated in the idle function.
	 */
	IRQ_CONNECT(IT8XXX2_TRANS_IRQ, 0, espi_it8xxx2_trans_isr,
			DEVICE_DT_INST_GET(0), 0);

	return 0;
}
