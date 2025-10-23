/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHIP_CHIPREGS_H
#define CHIP_CHIPREGS_H

#include <zephyr/sys/util.h>

#define IT51XXX_EC_FREQ KHZ(9200)

#ifdef _ASMLANGUAGE
#define ECREG(x) x
#else

/*
 * Macros for hardware registers access.
 */
#define ECREG(x)     (*((volatile unsigned char *)(x)))
#define ECREG_u16(x) (*((volatile unsigned short *)(x)))
#define ECREG_u32(x) (*((volatile unsigned long *)(x)))

#endif /* _ASMLANGUAGE */

/**
 *
 * (10xxh) Shared Memory Flash Interface Bridge (SMFI) registers
 *
 */
#ifndef __ASSEMBLER__
struct smfi_it51xxx_regs {
	volatile uint8_t reserved1[59];
	/* 0x3B: EC-Indirect memory address 0 */
	volatile uint8_t SMFI_ECINDAR0;
	/* 0x3C: EC-Indirect memory address 1 */
	volatile uint8_t SMFI_ECINDAR1;
	/* 0x3D: EC-Indirect memory address 2 */
	volatile uint8_t SMFI_ECINDAR2;
	/* 0x3E: EC-Indirect memory address 3 */
	volatile uint8_t SMFI_ECINDAR3;
	/* 0x3F: EC-Indirect memory data */
	volatile uint8_t SMFI_ECINDDR;
	/* 0x40: Scratch SRAM 0 address low byte */
	volatile uint8_t SMFI_SCAR0L;
	/* 0x41: Scratch SRAM 0 address middle byte */
	volatile uint8_t SMFI_SCAR0M;
	/* 0x42: Scratch SRAM 0 address high byte */
	volatile uint8_t SMFI_SCAR0H;
	volatile uint8_t reserved1_1[23];
	/* 0x5A: Host RAM Window Control */
	volatile uint8_t SMFI_HRAMWC;
	/* 0x5B: Host RAM Window 0 Base Address [11:4] */
	volatile uint8_t SMFI_HRAMW0BA;
	/* 0x5C: Host RAM Window 1 Base Address [11:4] */
	volatile uint8_t SMFI_HRAMW1BA;
	/* 0x5D: Host RAM Window 0 Access Allow Size */
	volatile uint8_t SMFI_HRAMW0AAS;
	/* 0x5E: Host RAM Window 1 Access Allow Size */
	volatile uint8_t SMFI_HRAMW1AAS;
	volatile uint8_t reserved_5f_80[34];
	/* 0x81: Flash control 6 */
	volatile uint8_t SMFI_FLHCTRL6R;
};
#endif /* !__ASSEMBLER__ */

/* SMFI register fields */
/* EC-Indirect read internal flash */
#define EC_INDIRECT_READ_INTERNAL_FLASH BIT(6)
/* Enable EC-indirect page program command */
#define IT51XXX_SMFI_MASK_ECINDPP       BIT(3)
#define ITE_EC_SMFI_MASK_ECINDPP        IT51XXX_SMFI_MASK_ECINDPP
/* 0x42: Scratch SRAM 0 address high byte */
#define SCARH_ENABLE                    BIT(7)
#define SCARH_ADDR_BIT19                BIT(3)

#define IT51XXX_SMFI_BASE      0xf01000
/* 0x63: Flash Control Register 3 */
#define IT51XXX_SMFI_FLHCTRL3R (IT51XXX_SMFI_BASE + 0x63)
#define IT51XXX_SMFI_FFSPITRI  BIT(0)

/**
 *
 * (16xxh) General Purpose I/O Port (GPIO) registers
 *
 */
#define GPIO_IT51XXX_REGS_BASE ((struct gpio_it51xxx_regs *)DT_REG_ADDR(DT_NODELABEL(gpiogcr)))

#ifndef __ASSEMBLER__
struct gpio_it51xxx_regs {
	/* 0x00: General Control */
	volatile uint8_t GPIO_GCR;
	/* 0x01-C1: Reserved_01_c1 */
	volatile uint8_t reserved_01_c1[193];
	/* 0xC2: General Control 35 */
	volatile uint8_t GPIO_GCR35;
	/* 0xC3-CF: Reserved_c3_cf */
	volatile uint8_t reserved_c3_cf[13];
	/* 0xD0: General Control 31 */
	volatile uint8_t GPIO_GCR31;
	/* 0xD1: General Control 32 */
	volatile uint8_t GPIO_GCR32;
	/* 0xD2: Reserved_d2 */
	volatile uint8_t reserved_d2[1];
	/* 0xD3: General Control 34 */
	volatile uint8_t GPIO_GCR34;
	/* 0xD4: GPA Voltage Selection */
	volatile uint8_t GPIO_GPAVSR;
	/* 0xD5: GPB Voltage Selection */
	volatile uint8_t GPIO_GPBVSR;
	/* 0xD6: GPC Voltage Selection */
	volatile uint8_t GPIO_GPCVSR;
	/* 0xD7: GPD Voltage Selection */
	volatile uint8_t GPIO_GPDVSR;
	/* 0xD8: GPE Voltage Selection */
	volatile uint8_t GPIO_GPEVSR;
	/* 0xD9: GPF Voltage Selection */
	volatile uint8_t GPIO_GPFVSR;
	/* 0xDA: GPG Voltage Selection */
	volatile uint8_t GPIO_GPGVSR;
	/* 0xDB: GPH Voltage Selection */
	volatile uint8_t GPIO_GPHVSR;
	/* 0xDC: GPI Voltage Selection */
	volatile uint8_t GPIO_GPIVSR;
	/* 0xDD: GPJ Voltage Selection */
	volatile uint8_t GPIO_GPJVSR;
	/* 0xDE: GP I3C Control */
	volatile uint8_t GPIO_GPI3CCR;
	/* 0xDF: Reserved_df */
	volatile uint8_t reserved_df[1];
	/* 0xE0: General Control 16 */
	volatile uint8_t GPIO_GCR16;
	/* 0xE1: General Control 17 */
	volatile uint8_t GPIO_GCR17;
	/* 0xE2: General Control 18 */
	volatile uint8_t GPIO_GCR18;
	/* 0xE3: Reserved_e3 */
	volatile uint8_t reserved_e3;
	/* 0xE4: General Control 19 */
	volatile uint8_t GPIO_GCR19;
	/* 0xE5: General Control 20 */
	volatile uint8_t GPIO_GCR20;
	/* 0xE6: General Control 21 */
	volatile uint8_t GPIO_GCR21;
	/* 0xE7: General Control 22 */
	volatile uint8_t GPIO_GCR22;
	/* 0xE8: Reserved_e8 */
	volatile uint8_t reserved_e8[1];
	/* 0xE9: General Control 24 */
	volatile uint8_t GPIO_GCR24;
	/* 0xEA: General Control 25 */
	volatile uint8_t GPIO_GCR25;
	/* 0xEB: General Control 26 */
	volatile uint8_t GPIO_GCR26;
	/* 0xEC: Reserved_ec */
	volatile uint8_t reserved_ec[1];
	/* 0xED: General Control 28 */
	volatile uint8_t GPIO_GCR28;
	/* 0xEE-0xEF: Reserved_ee_ef */
	volatile uint8_t reserved_ee_ef[2];
	/* 0xF0: General Control 1 */
	volatile uint8_t GPIO_GCR1;
	/* 0xF1: General Control 2 */
	volatile uint8_t GPIO_GCR2;
	/* 0xF2: General Control 3 */
	volatile uint8_t GPIO_GCR3;
	/* 0xF3: General Control 4 */
	volatile uint8_t GPIO_GCR4;
	/* 0xF4: General Control 5 */
	volatile uint8_t GPIO_GCR5;
	/* 0xF5: General Control 6 */
	volatile uint8_t GPIO_GCR6;
	/* 0xF6: General Control 7 */
	volatile uint8_t GPIO_GCR7;
	/* 0xF7: General Control 8 */
	volatile uint8_t GPIO_GCR8;
	/* 0xF8: General Control 9 */
	volatile uint8_t GPIO_GCR9;
	/* 0xF9: General Control 10 */
	volatile uint8_t GPIO_GCR10;
	/* 0xFA: General Control 11 */
	volatile uint8_t GPIO_GCR11;
	/* 0xFB: General Control 12 */
	volatile uint8_t GPIO_GCR12;
	/* 0xFC: General Control 13 */
	volatile uint8_t GPIO_GCR13;
	/* 0xFD: General Control 14 */
	volatile uint8_t GPIO_GCR14;
	/* 0xFE: General Control 15 */
	volatile uint8_t GPIO_GCR15;
	/* 0xFF: Power Good Watch Control */
	volatile uint8_t GPIO_PGWCR;
};
#endif /* !__ASSEMBLER__ */

/* GPIO register fields */
/* 0x00: General Control */
#define IT51XXX_GPIO_LPCRSTEN             (BIT(2) | BIT(1))
#define ITE_EC_GPIO_LPCRSTEN              IT51XXX_GPIO_LPCRSTEN
/* 0xC2: General Control 35 */
#define IT51XXX_GPIO_USBPDEN              BIT(5)
/* 0xF0: General Control 1 */
#define IT51XXX_GPIO_U2CTRL_SIN1_SOUT1_EN BIT(2)
#define IT51XXX_GPIO_U1CTRL_SIN0_SOUT0_EN BIT(0)
/* 0xE6: General Control 21 */
#define IT51XXX_GPIO_GPH1VS               BIT(1)
#define IT51XXX_GPIO_GPH2VS               BIT(0)

#define KSIX_KSOX_KBS_GPIO_MODE BIT(7)
#define KSIX_KSOX_GPIO_OUTPUT   BIT(6)
#define KSIX_KSOX_GPIO_PULLUP   BIT(2)
#define KSIX_KSOX_GPIO_PULLDOWN BIT(1)

#define GPCR_PORT_PIN_MODE_INPUT    BIT(7)
#define GPCR_PORT_PIN_MODE_OUTPUT   BIT(6)
#define GPCR_PORT_PIN_MODE_PULLUP   BIT(2)
#define GPCR_PORT_PIN_MODE_PULLDOWN BIT(1)

/*
 * If both PULLUP and PULLDOWN are set to 1b, the corresponding port would be
 * configured as tri-state.
 */
#define GPCR_PORT_PIN_MODE_TRISTATE                                                                \
	(GPCR_PORT_PIN_MODE_INPUT | GPCR_PORT_PIN_MODE_PULLUP | GPCR_PORT_PIN_MODE_PULLDOWN)

/**
 *
 * (1Exxh) PLL Control Mode
 *
 */
#ifndef __ASSEMBLER__
enum chip_pll_mode {
	CHIP_PLL_DOZE = 0,
	CHIP_PLL_SLEEP = 1,
	CHIP_PLL_DEEP_DOZE = 3,
};
#endif

/**
 *
 * (20xxh) General Control (GCTRL) registers
 *
 */
#define GCTRL_IT51XXX_REGS_BASE ((struct gctrl_it51xxx_regs *)DT_REG_ADDR(DT_NODELABEL(gctrl)))

#ifndef __ASSEMBLER__
struct gctrl_it51xxx_regs {
	/* 0x00-0x01: Reserved_00_01 */
	volatile uint8_t reserved_00_01[2];
	/* 0x02: Chip Version */
	volatile uint8_t GCTRL_ECHIPVER;
	/* 0x03-0x05: Reserved_03_05 */
	volatile uint8_t reserved_03_05[3];
	/* 0x06: Reset Status */
	volatile uint8_t GCTRL_RSTS;
	/* 0x07-0x09: Reserved_07_09 */
	volatile uint8_t reserved_07_09[3];
	/* 0x0A: Base Address Select */
	volatile uint8_t GCTRL_BADRSEL;
	/* 0x0B: Wait Next Clock Rising */
	volatile uint8_t GCTRL_WNCKR;
	/* 0x0C: Special Control 5 */
	volatile uint8_t GCTRL_SPCTRL5;
	/* 0x0D: Special Control 1 */
	volatile uint8_t GCTRL_SPCTRL1;
	/* 0x0E-0x0F: reserved_0e_0f */
	volatile uint8_t reserved_0e_0f[2];
	/* 0x10: Reset Control DMM */
	volatile uint8_t GCTRL_RSTDMMC;
	/* 0x11: Reset Control 4 */
	volatile uint8_t GCTRL_RSTC4;
	/* 0x12-0x1B: reserved_12_1b */
	volatile uint8_t reserved_12_1b[10];
	/* 0x1C: Special Control 4 */
	volatile uint8_t GCTRL_SPCTRL4;
	/* 0x1D-0x1F: reserved_1d_1f */
	volatile uint8_t reserved_1d_1f[3];
	/* 0x20: Reset Control 5 */
	volatile uint8_t GCTRL_RSTC5;
	/* 0x21-0x2f: reserved_21_2f */
	volatile uint8_t reserved_21_2f[15];
	/* 0x30: Port 80h/81h Status Register */
	volatile uint8_t GCTRL_P80H81HSR;
	/* 0x31: Port 80h Data Register */
	volatile uint8_t GCTRL_P80HDR;
	/* 0x32: Port 81h Data Register */
	volatile uint8_t GCTRL_P81HDR;
	/* 0x33-0x37: reserved_33_37 */
	volatile uint8_t reserved_33_37[5];
	/* 0x38: Special Control 9 */
	volatile uint8_t GCTRL_SPCTRL9;
	/* 0x39-0x46: reserved_39_46 */
	volatile uint8_t reserved_39_46[14];
	/* 0x47: Scratch SRAM0 Base Address */
	volatile uint8_t GCTRL_SCR0BAR;
	/* 0x48: Scratch ROM 0 Size */
	volatile uint8_t GCTRL_SCR0SZR;
	/* 0x49-0x84: reserved_49_84 */
	volatile uint8_t reserved_49_84[60];
	/* 0x85: Chip ID Byte 1 */
	volatile uint8_t GCTRL_ECHIPID1;
	/* 0x86: Chip ID Byte 2 */
	volatile uint8_t GCTRL_ECHIPID2;
	/* 0x87: Chip ID Byte 3 */
	volatile uint8_t GCTRL_ECHIPID3;
};
#endif /* !__ASSEMBLER__ */

/* GCTRL register fields */
/* 0x06: Reset Status */
#define IT51XXX_GCTRL_LRS         (BIT(1) | BIT(0))
#define IT51XXX_GCTRL_IWDTR       BIT(1)
/* 0x0B: Wait Next 65K Rising */
#define IT51XXX_GCTRL_WN65K       0x00
/* 0x10: Reset Control DMM */
#define IT51XXX_GCTRL_UART1SD     BIT(3)
#define IT51XXX_GCTRL_UART2SD     BIT(2)
/* 0x11: Reset Control 4 */
#define IT51XXX_GCTRL_RPECI       BIT(4)
#define IT51XXX_GCTRL_RUART       BIT(2)
/* 0x1C: Special Control 4 */
#define IT51XXX_GCTRL_LRSIWR      BIT(2)
#define IT51XXX_GCTRL_LRSIPWRSWTR BIT(1)
#define IT51XXX_GCTRL_LRSIPGWR    BIT(0)
/* 0x38: Special Control 9 */
#define IT51XXX_GCTRL_ALTIE       BIT(4)
/* 0x47: Scratch SRAM0 Base Address */
#define IT51XXX_SEL_SRAM0_BASE_4K 0x04
/* 0x48: Scratch ROM 0 Size */
#define IT51XXX_GCTRL_SCRSIZE_4K  0x03

/* Alias gpio_ite_ec_regs to gpio_it51xxx_regs for compatibility */
#define gpio_ite_ec_regs       gpio_it51xxx_regs
#define GPIO_ITE_EC_REGS_BASE  GPIO_IT51XXX_REGS_BASE
/* Alias smfi_ite_ec_regs to smfi_it51xxx_regs for compatibility */
#define smfi_ite_ec_regs       smfi_it51xxx_regs
/* Alias gctrl_ite_ec_regs to gctrl_it51xxx_regs for compatibility */
#define gctrl_ite_ec_regs      gctrl_it51xxx_regs
#define GCTRL_ITE_EC_REGS_BASE GCTRL_IT51XXX_REGS_BASE

/**
 *
 * (22xxh) Battery-backed SRAM (BRAM) registers
 *
 */
#ifndef __ASSEMBLER__
/* Battery backed RAM indices. */
#define BRAM_MAGIC_FIELD_OFFSET 0x7c

enum bram_indices {
	/* This field is used to indicate BRAM is valid or not. */
	BRAM_IDX_VALID_FLAGS0 = BRAM_MAGIC_FIELD_OFFSET,
	BRAM_IDX_VALID_FLAGS1,
	BRAM_IDX_VALID_FLAGS2,
	BRAM_IDX_VALID_FLAGS3
};
#endif /* !__ASSEMBLER__ */

/**
 *
 * (42xxh) SMBus Interface for target (SMB) registers
 *
 */
#define IT51XXX_SMB_BASE 0xf04200
/* 0x0a, 0x2a, 0x4a: Slave n Dedicated FIFO Pre-defined Control Register */
#define SMB_SADFPCTL     (IT51XXX_SMB_BASE + 0x0a)
#define SMB_SBDFPCTL     (IT51XXX_SMB_BASE + 0x2a)
#define SMB_SCDFPCTL     (IT51XXX_SMB_BASE + 0x4a)
#define SMB_HSAPE        BIT(1)

#endif /* CHIP_CHIPREGS_H */
