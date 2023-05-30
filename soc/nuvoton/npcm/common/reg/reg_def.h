/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_REG_DEF_H
#define _NUVOTON_NPCM_REG_DEF_H

#define __I     volatile const  /*!< Defines 'read only' permissions                 */
#define __O     volatile        /*!< Defines 'write only' permissions                */
#define __IO    volatile        /*!< Defines 'read / write' permissions              */

/*
 * NPCM register structure size/offset checking macro function to mitigate
 * the risk of unexpected compiling results. All addresses of NPCM registers
 * must meet the alignment requirement of cortex-m4.
 * DO NOT use 'packed' attribute if module contains different length ie.
 * 8/16/32 bits registers.
 */
#define NPCM_REG_SIZE_CHECK(reg_def, size) \
	BUILD_ASSERT(sizeof(struct reg_def) == size, \
		"Failed in size check of register structure!")
#define NPCM_REG_OFFSET_CHECK(reg_def, member, offset) \
	BUILD_ASSERT(offsetof(struct reg_def, member) == offset, \
		"Failed in offset check of register structure member!")

/*
 * NPCM register access checking via structure macro function to mitigate the
 * risk of unexpected compiling results if module contains different length
 * registers. For example, a word register access might break into two byte
 * register accesses by adding 'packed' attribute.
 *
 * For example, add this macro for word register 'PRSC' of PWM module in its
 * device init function for checking violation. Once it occurred, core will be
 * stalled forever and easy to find out what happens.
 */
#define NPCM_REG_WORD_ACCESS_CHECK(reg, val) { \
		uint16_t placeholder = reg; \
		reg = val; \
		__ASSERT(reg == val, "16-bit reg access failed!"); \
		reg = placeholder; \
	}
#define NPCM_REG_DWORD_ACCESS_CHECK(reg, val) { \
		uint32_t placeholder = reg; \
		reg = val; \
		__ASSERT(reg == val, "32-bit reg access failed!"); \
		reg = placeholder; \
	}
/*
 * Core Domain Clock Generator (CDCG) device registers
 */
struct cdcg_reg {
	/* High Frequency Clock Generator (HFCG) registers */
	/* 0x000: HFCG Control */
	volatile uint8_t HFCGCTRL;
	volatile uint8_t reserved1;
	/* 0x002: HFCG M Low Byte Value */
	volatile uint8_t HFCGML;
	volatile uint8_t reserved2;
	/* 0x004: HFCG M High Byte Value */
	volatile uint8_t HFCGMH;
	volatile uint8_t reserved3;
	/* 0x006: HFCG N Value */
	volatile uint8_t HFCGN;
	volatile uint8_t reserved4;
	/* 0x008: HFCG Prescaler */
	volatile uint8_t HFCGP;
	volatile uint8_t reserved5[7];
	/* 0x010: HFCG Bus Clock Dividers */
	volatile uint8_t HFCBCD;
	volatile uint8_t reserved6;
	/* 0x012: HFCG Bus Clock Dividers */
	volatile uint8_t HFCBCD1;
	volatile uint8_t reserved7;
	/* 0x014: HFCG Bus Clock Dividers */
	volatile uint8_t HFCBCD2;
	volatile uint8_t reserved8[235];

	/* Low Frequency Clock Generator (LFCG) registers */
	/* 0x100: LFCG Control */
	volatile uint8_t  LFCGCTL;
	volatile uint8_t reserved9;
	/* 0x102: High-Frequency Reference Divisor I */
	volatile uint16_t HFRDI;
	/* 0x104: High-Frequency Reference Divisor F */
	volatile uint16_t HFRDF;
	/* 0x106: FRCLK Clock Divisor */
	volatile uint16_t FRCDIV;
	/* 0x108: Divisor Correction Value 1 */
	volatile uint16_t DIVCOR1;
	/* 0x10A: Divisor Correction Value 2 */
	volatile uint16_t DIVCOR2;
	volatile uint8_t reserved10[8];
	/* 0x114: LFCG Control 2 */
	volatile uint8_t  LFCGCTL2;
	volatile uint8_t  reserved11;
};


/* CDCG register fields */
#define NPCM_HFCGCTRL_LOAD                    0
#define NPCM_HFCGCTRL_LOCK                    2
#define NPCM_HFCGCTRL_CLK_CHNG                7

/*
 * Power Management Controller (PMC) device registers
 */
struct pmc_reg {
	/* 0x000: Power Management Controller */
	volatile uint8_t PMCSR;
	volatile uint8_t reserved1[2];
	/* 0x003: Enable in Sleep Control */
	volatile uint8_t ENIDL_CTL;
	/* 0x004: Disable in Idle Control */
	volatile uint8_t DISIDL_CTL;
	/* 0x005: Disable in Idle Control 1 */
	volatile uint8_t DISIDL_CTL1;
	volatile uint8_t reserved2[2];
	/* 0x008 - 0D: Power-Down Control 1 - 6 */
	volatile uint8_t PWDWN_CTL1[6];
	volatile uint8_t reserved3[18];
	/* 0x020 - 21: Power-Down Control 1 - 2 */
	volatile uint8_t RAM_PD[2];
	volatile uint8_t reserved4[2];
	/* 0x024: Power-Down Control 7 */
	volatile uint8_t PWDWN_CTL7[1];
};

/* PMC multi-registers */
#define NPCM_PWDWN_CTL_OFFSET(n) (((n) < 7) ? (0x007 + n) : (0x024 + (n - 7)))
#define NPCM_PWDWN_CTL(base, n) (*(volatile uint8_t *)(base + \
						NPCM_PWDWN_CTL_OFFSET(n)))

/* PMC register fields */
#define NPCM_PMCSR_DI_INSTW                   0
#define NPCM_PMCSR_DHF                        1
#define NPCM_PMCSR_IDLE                       2
#define NPCM_PMCSR_NWBI                       3
#define NPCM_PMCSR_OHFC                       6
#define NPCM_PMCSR_OLFC                       7
#define NPCM_DISIDL_CTL_RAM_DID               5
#define NPCM_ENIDL_CTL_LP_WK_CTL              6
#define NPCM_ENIDL_CTL_PECI_ENI               2

/*
 * System Configuration (SCFG) device registers
 */
struct scfg_reg {
	/* 0x000: Device Control */
	volatile uint8_t DEVCNT;
	/* 0x001: Straps Status */
	volatile uint8_t STRPST;
	/* 0x002: Reset Control and Status */
	volatile uint8_t RSTCTL;
	volatile uint8_t reserved1[3];
	/* 0x006: Device Control 4 */
	volatile uint8_t DEV_CTL4;
        volatile uint8_t reserved2[4];
	volatile uint8_t DEVALT10;
	volatile uint8_t DEVALT11;
	volatile uint8_t DEVALT12;
	volatile uint8_t reserved3[2];
	/* 0x010 - 1F: Device Alternate Function 0 - F */
	volatile uint8_t DEVALT0[16];
	volatile uint8_t reserved4[4];
	/* 0x024: DEVALTCX */
	volatile uint8_t DEVALTCX;
	volatile uint8_t reserved5[3];
	/* 0x028: Device Pull-Up Enable 0 */
	volatile uint8_t DEVPU0;
	/* 0x029: Device Pull-Down Enable 1 */
	volatile uint8_t DEVPD1;
	volatile uint8_t reserved6;
	/* 0x02B: Low-Voltage Pins Control 1 */
	volatile uint8_t LV_CTL1;
};

/* SCFG multi-registers */

/* Macro functions for SCFG multi-registers */
#define NPCM_DEV_CTL(base, n) \
        (*(volatile uint8_t *)(base + n))
#define NPCM_PUPD_EN(base, n) \
        (*(volatile uint8_t *)(base + NPCM_PUPD_EN_OFFSET(n)))

#define NPCM_DEVALT(base, n) (*(volatile uint8_t *)(base + n))
#define NPCM_DEVALT6A_OFFSET  0x5A

/* SCFG register fields */
#define NPCM_DEVCNT_F_SPI_TRIS                6
#define NPCM_DEVCNT_HIF_TYP_SEL_FIELD         FIELD(2, 2)
#define NPCM_DEVCNT_JEN1_HEN                  5
#define NPCM_DEVCNT_JEN0_HEN                  4
#define NPCM_STRPST_TRIST                     1
#define NPCM_STRPST_TEST                      2
#define NPCM_STRPST_JEN1                      4
#define NPCM_STRPST_JEN0                      5
#define NPCM_STRPST_SPI_COMP                  7
#define NPCM_RSTCTL_VCC1_RST_STS              0
#define NPCM_RSTCTL_DBGRST_STS                1
#define NPCM_RSTCTL_VCC1_RST_SCRATCH          3
#define NPCM_RSTCTL_LRESET_PLTRST_MODE        5
#define NPCM_RSTCTL_HIPRST_MODE               6
#define NPCM_DEV_CTL4_F_SPI_SLLK              2
#define NPCM_DEV_CTL4_SPI_SP_SEL              4
#define NPCM_DEV_CTL4_WP_IF                   5
#define NPCM_DEV_CTL4_VCC1_RST_LK             6
#define NPCM_DEVPU0_I2C0_0_PUE                0
#define NPCM_DEVPU0_I2C0_1_PUE                1
#define NPCM_DEVPU0_I2C1_0_PUE                2
#define NPCM_DEVPU0_I2C2_0_PUE                4
#define NPCM_DEVPU0_I2C3_0_PUE                6
#define NPCM_DEVPU1_F_SPI_PUD_EN              7
#define NPCM_DEVALT10_CRGPIO_SELECT_SL_CORE   0
#define NPCM_DEVALT10_CRGPIO_SELECT_SL_POWER  1
#define NPCM_DEVALTCX_GPIO_PULL_EN            7

#define NPCM_DEVALT6A_SIOX1_PU_EN             2
#define NPCM_DEVALT6A_SIOX2_PU_EN             3

/*
 * System Glue (GLUE) device registers
 */
struct glue_reg {
	volatile uint8_t reserved1[2];
	/* 0x002: SMBus Start Bit Detection */
	volatile uint8_t SMB_SBD;
	/* 0x003: SMBus Event Enable */
	volatile uint8_t SMB_EEN;
	volatile uint8_t reserved2[12];
	/* 0x010: Simple Debug Port Data 0 */
	volatile uint8_t SDPD0;
	volatile uint8_t reserved3;
	/* 0x012: Simple Debug Port Data 1 */
	volatile uint8_t SDPD1;
	volatile uint8_t reserved4;
	/* 0x014: Simple Debug Port Control and Status */
	volatile uint8_t SDP_CTS;
	volatile uint8_t reserved5[12];
	/* 0x021: SMBus Bus Select */
	volatile uint8_t SMB_SEL;
	volatile uint8_t reserved6[5];
	/* 0x027: PSL Control and Status */
	volatile uint8_t PSL_CTS;
};

/*
 * Universal Asynchronous Receiver-Transmitter (UART) device registers
 */
struct uart_reg {
	/* 0x000: Transmit Data Buffer */
	volatile uint8_t UTBUF;
	volatile uint8_t reserved1;
	/* 0x002: Receive Data Buffer */
	volatile uint8_t URBUF;
	volatile uint8_t reserved2;
	/* 0x004: Interrupt Control */
	volatile uint8_t UICTRL;
	volatile uint8_t reserved3;
	/* 0x006: Status */
	volatile uint8_t USTAT;
	volatile uint8_t reserved4;
	/* 0x008: Frame Select */
	volatile uint8_t UFRS;
	volatile uint8_t reserved5;
	/* 0x00A: Mode Select */
	volatile uint8_t UMDSL;
	volatile uint8_t reserved6;
	/* 0x00C: Baud Rate Divisor */
	volatile uint8_t UBAUD;
	volatile uint8_t reserved7;
	/* 0x00E: Baud Rate Prescaler */
	volatile uint8_t UPSR;
	volatile uint8_t reserved8[7];
	/* 0x016: FIFO Control */
	volatile uint8_t UFCTRL;
	volatile uint8_t reserved9;
	/* 0x018: TX FIFO Current Level */
	volatile uint8_t UTXFLV;
	volatile uint8_t reserved10;
	/* 0x01A: RX FIFO Current Level */
	volatile uint8_t URXFLV;
	volatile uint8_t reserved11;
};

/* UART register fields */
#define NPCM_UICTRL_TBE                       0
#define NPCM_UICTRL_RBF                       1
#define NPCM_UICTRL_ETI                       5
#define NPCM_UICTRL_ERI                       6
#define NPCM_UICTRL_EEI                       7
#define NPCM_USTAT_PE                         0
#define NPCM_USTAT_FE                         1
#define NPCM_USTAT_DOE                        2
#define NPCM_USTAT_ERR                        3
#define NPCM_USTAT_BKD                        4
#define NPCM_USTAT_RB9                        5
#define NPCM_USTAT_XMIP                       6
#define NPCM_UFRS_CHAR_FIELD                  FIELD(0, 2)
#define NPCM_UFRS_STP                         2
#define NPCM_UFRS_XB9                         3
#define NPCM_UFRS_PSEL_FIELD                  FIELD(4, 2)
#define NPCM_UFRS_PEN                         6
#define NPCM_UFCTRL_FIFOEN                    0
//#define NPCM_UMDSL_FIFO_MD                    0
#define NPCM_UTXFLV_TFL                       FIELD(0, 5)
#define NPCM_URXFLV_RFL                       FIELD(0, 5)
//#define NPCM_UFTSTS_TEMPTY_LVL_STS            5
//#define NPCM_UFTSTS_TFIFO_EMPTY_STS           6
//#define NPCM_UFTSTS_NXMIP                     7
//#define NPCM_UFRSTS_RFULL_LVL_STS             5
//#define NPCM_UFRSTS_RFIFO_NEMPTY_STS          6
//#define NPCM_UFRSTS_ERR                       7
//#define NPCM_UFTCTL_TEMPTY_LVL_SEL            FIELD(0, 5)
//#define NPCM_UFTCTL_TEMPTY_LVL_EN             5
//#define NPCM_UFTCTL_TEMPTY_EN                 6
//#define NPCM_UFTCTL_NXMIPEN                   7
//#define NPCM_UFRCTL_RFULL_LVL_SEL             FIELD(0, 5)
//#define NPCM_UFRCTL_RFULL_LVL_EN              5
//#define NPCM_UFRCTL_RNEMPTY_EN                6
//#define NPCM_UFRCTL_ERR_EN                    7

/*
 * Multi-Input Wake-Up Unit (MIWU) device registers
 */

/* MIWU multi-registers */
#define NPCM_WKEDG_OFFSET(n)    (0x000 + ((n) * 2L) + ((n) < 5 ? 0 : 0x1E))
#define NPCM_WKAEDG_OFFSET(n)   (0x001 + ((n) * 2L) + ((n) < 5 ? 0 : 0x1E))
#define NPCM_WKPND_OFFSET(n)    (0x00A + ((n) * 4L) + ((n) < 5 ? 0 : 0x10))
#define NPCM_WKPCL_OFFSET(n)    (0x00C + ((n) * 4L) + ((n) < 5 ? 0 : 0x10))
#define NPCM_WKEN_OFFSET(n)     (0x01E + ((n) * 2L) + ((n) < 5 ? 0 : 0x12))
#define NPCM_WKINEN_OFFSET(n)   (0x01F + ((n) * 2L) + ((n) < 5 ? 0 : 0x12))
#define NPCM_WKMOD_OFFSET(n)    (0x070 + (n))

#define NPCM_WKEDG(base, n) (*(volatile uint8_t *)(base + \
						NPCM_WKEDG_OFFSET(n)))
#define NPCM_WKAEDG(base, n) (*(volatile uint8_t *)(base + \
						NPCM_WKAEDG_OFFSET(n)))
#define NPCM_WKPND(base, n) (*(volatile uint8_t *)(base + \
						NPCM_WKPND_OFFSET(n)))
#define NPCM_WKPCL(base, n) (*(volatile uint8_t *)(base + \
						NPCM_WKPCL_OFFSET(n)))
#define NPCM_WKEN(base, n) (*(volatile uint8_t *)(base + \
						NPCM_WKEN_OFFSET(n)))
#define NPCM_WKINEN(base, n) (*(volatile uint8_t *)(base + \
						NPCM_WKINEN_OFFSET(n)))
#define NPCM_WKMOD(base, n) (*(volatile uint8_t *)(base + \
						NPCM_WKMOD_OFFSET(n)))

/*
 * General-Purpose I/O (GPIO) device registers
 */
struct gpio_reg {
	/* 0x000: Port GPIOx Data Out */
	volatile uint8_t PDOUT;
	/* 0x001: Port GPIOx Data In */
	volatile uint8_t PDIN;
	/* 0x002: Port GPIOx Direction */
	volatile uint8_t PDIR;
	/* 0x003: Port GPIOx Pull-Up or Pull-Down Enable */
	volatile uint8_t PPULL;
	/* 0x004: Port GPIOx Pull-Up/Down Selection */
	volatile uint8_t PPUD;
	/* 0x005: Port GPIOx Drive Enable by VDD Present */
	volatile uint8_t PENVDD;
	/* 0x006: Port GPIOx Output Type */
	volatile uint8_t PTYPE;
};

/*
 * Pulse Width Modulator (PWM) device registers
 */
struct pwm_reg {
	/* 0x000: Clock Prescaler */
	volatile uint16_t PRSC;
	/* 0x002: Cycle Time */
	volatile uint16_t CTR;
	/* 0x004: PWM Control */
	volatile uint8_t PWMCTL;
	volatile uint8_t reserved1;
	/* 0x006: Duty Cycle */
	volatile uint16_t DCR;
	volatile uint8_t reserved2[4];
	/* 0x00C: PWM Control Extended */
	volatile uint8_t PWMCTLEX;
	volatile uint8_t reserved3;
};

/* PWM register fields */
#define NPCM_PWMCTL_INVP                      0
#define NPCM_PWMCTL_CKSEL                     1
#define NPCM_PWMCTL_HB_DC_CTL_FIELD           FIELD(2, 2)
#define NPCM_PWMCTL_PWR                       7
#define NPCM_PWMCTLEX_FCK_SEL_FIELD           FIELD(4, 2)
#define NPCM_PWMCTLEX_OD_OUT                  7

/*
 * Analog-To-Digital Converter (ADC) device registers
 */
struct adc_reg {
	volatile uint16_t RESERVED0;
	/* 0x02: DSADC control register0 */
	volatile uint8_t  DSADCCTRL0;
	volatile uint8_t  RESERVED1[11];
	/* 0x0E: Operation Mode select */
	volatile uint16_t ADCTM;
	volatile uint16_t RESERVED2;
	/* 0x12: Offset setting for tdp */
	volatile uint16_t ADCTDPO[3];
	volatile uint16_t RESERVED3[4];
	/* 0x20: DSADC Analog Control register 1 */
	volatile uint8_t  ADCACTRL1;
	volatile uint8_t  RESERVED4;
	/* 0x22: DSADC Analog Power Down Control */
	volatile uint16_t ADCACTRL2;
	volatile uint8_t  RESERVED5[2];
	/* 0x26: Voltage / Thermister mode select */
	volatile uint8_t  DSADCCTRL6;
	volatile uint8_t  RESERVED6;
	/* 0x28: Voltage / Thermister mode select */
	volatile uint8_t  DSADCCTRL7;
	volatile uint8_t  RESERVED7[3];
	/* 0x2C: Voltage / Thermister mode select */
	volatile uint16_t DSADCCTRL8;
	volatile uint8_t  RESERVED8[74];
	/* 0x78: DSADC Configuration */
	volatile uint16_t DSADCCFG;
	/* 0x7A: DSADC Channel select */
	volatile uint8_t  DSADCCS;
	volatile uint8_t  RESERVED9;
	/* 0x7C: DSADC global status */
	volatile uint16_t DSADCSTS;
	volatile uint16_t RESERVED10;
	/* 0x80: Temperature Channel Data */
	volatile uint16_t TCHNDAT;
};

/* ADC register fields */
#define NPCM_CTRL0_VNT                         (5)
#define NPCM_CTRL0_CH_SEL                      (0)
#define NPCM_TM_T_MODE5                        (8)
#define NPCM_TM_T_MODE4                        (6)
#define NPCM_TM_T_MODE3                        (4)
#define NPCM_TM_T_MODE2                        (2)
#define NPCM_TM_T_MODE1                        (0)
#define NPCM_TD_POST_OFFSET                    (8)
#define NPCM_ACTRL1_PWCTRL                     (1)
#define NPCM_ACTRL2_PD_VPP_PG                  (10)
#define NPCM_ACTRL2_PD_PM                      (9)
#define NPCM_ACTRL2_PD_ATX_5VSB                (8)
#define NPCM_ACTRL2_PD_ANA                     (7)
#define NPCM_ACTRL2_PD_BG                      (6)
#define NPCM_ACTRL2_PD_DSM                     (5)
#define NPCM_ACTRL2_PD_DVBE                    (4)
#define NPCM_ACTRL2_PD_IREF                    (3)
#define NPCM_ACTRL2_PD_ISEN                    (2)
#define NPCM_ACTRL2_PD_RG                      (1)
#define NPCM_CFG_IOVFEN                        (6)
#define NPCM_CFG_ICEN                          (5)
#define NPCM_CFG_START                         (4)
#define NPCM_CS_CC5                            (5)
#define NPCM_CS_CC4                            (4)
#define NPCM_CS_CC3                            (3)
#define NPCM_CS_CC2                            (2)
#define NPCM_CS_CC1                            (1)
#define NPCM_CS_CC0                            (0)
#define NPCM_STS_OVFEV                         (1)
#define NPCM_STS_EOCEV                         (0)
#define NPCM_TCHNDATA_NEW                      (15)
#define NPCM_TCHNDATA_DAT                      (3)
#define NPCM_TCHNDATA_FRAC                     (0)
#define NPCM_THRCTL_EN                         (15)
#define NPCM_THRCTL_VAL                        (0)
#define NPCM_THRDCTL_EN                        (15)

/*
 * Timer Watchdog (TWD) device registers
 */
struct twd_reg {
	/* [0x00] Timer and Watchdog Configuration */
	volatile uint8_t TWCFG;
	volatile uint8_t reserved1[1];
	/* [0x02] Timer and Watchdog Clock Prescaler */
	volatile uint8_t TWCP;
	volatile uint8_t reserved2[1];
	/* [0x04] TWD Timer 0 Counter Preset */
	volatile uint16_t TWDT0;
	/* [0x06] TWDT0 Control and Status */
	volatile uint8_t T0CSR;
	volatile uint8_t reserved3[1];
	/* [0x08] Watchdog Count */
	volatile uint8_t WDCNT;
	volatile uint8_t reserved4[1];
	/* [0x0A] Watchdog Service Data Match */
	volatile uint8_t WDSDM;
	volatile uint8_t reserved5[1];
	/* [0x0C] TWD Timer 0 Counter */
	volatile uint16_t TWMT0;
	/* [0x0E] Watchdog Counter */
	volatile uint8_t TWMWD;
	volatile uint8_t reserved6[1];
	/* [0x10] Watchdog Clock Prescaler */
	volatile uint8_t WDCP;
};

/* TWD register fields */
#define NPCM_TWCFG_LTWD_CFG              (0)
#define NPCM_TWCFG_LTWCP                 (1)
#define NPCM_TWCFG_LTWDT0                (2)
#define NPCM_TWCFG_LWDCNT                (3)
#define NPCM_TWCFG_WDCT0I                (4)
#define NPCM_TWCFG_WDSDME                (5)
#define NPCM_TWCP_MDIV                   (0)
#define NPCM_T0CSR_RST                   (0)
#define NPCM_T0CSR_TC                    (1)
#define NPCM_T0CSR_WDLTD                 (3)
#define NPCM_T0CSR_WDRST_STS             (4)
#define NPCM_T0CSR_WD_RUN                (5)
#define NPCM_T0CSR_T0EN                  (6)
#define NPCM_T0CSR_TESDIS                (7)
#define NPCM_WDCP_WDIV                   (0)

/*
 * SPI PERIPHERAL INTERFACE (SPIP) device registers
 */
struct spip_reg {
	/* 0x00: SPI Control Register */
	volatile uint32_t CTL;
	/* 0x04: SPI Clock Divider Register */
	volatile uint32_t CLKDIV;
	/* 0x08: SPI Slave Select Control Register */
	volatile uint32_t SSCTL;
	/* 0x0C: SPI PDMA Control Register */
	volatile uint32_t PDMACTL;
	/* 0x10: SPI FIFO Control Register */
	volatile uint32_t FIFOCTL;
	/* 0x14: SPI Status Register */
	volatile uint32_t STATUS;
	volatile uint32_t RESERVE0[2];
	/* 0x20: SPI Data Transmit Register */
	volatile uint32_t TX;
	volatile uint32_t RESERVE1[3];
	/* 0x30: SPI Data Receive Register */
	volatile uint32_t RX;
};

/* SPIP register fields */
/* 0x00: SPI_CTL fields */
#define NPCM_CTL_QUADIOEN			(22)
#define NPCM_CTL_DUALIOEN			(21)
#define NPCM_CTL_QDIODIR			(20)
#define NPCM_CTL_REORDER			(19)
#define NPCM_CTL_SLAVE			(18)
#define NPCM_CTL_UNITIEN			(17)
#define NPCM_CTL_TWOBIT			(16)
#define NPCM_CTL_LSB				(13)
#define NPCM_CTL_DWIDTH			(8)
#define NPCM_CTL_SUSPITV			(4)
#define NPCM_CTL_CLKPOL			(3)
#define NPCM_CTL_TXNEG			(2)
#define NPCM_CTL_RXNEG			(1)
#define NPCM_CTL_SPIEN			(0)

/* 0x04: SPI_CLKDIV fields */
#define NPCM_CLKDIV_DIVIDER			(0)

/* 0x08: SPI_SSCTL fields */
#define NPCM_SSCTL_SLVTOCNT			(16)
#define NPCM_SSCTL_SSINAIEN			(13)
#define NPCM_SSCTL_SSACTIEN			(12)
#define NPCM_SSCTL_SLVURIEN			(9)
#define NPCM_SSCTL_SLVBEIEN			(8)
#define NPCM_SSCTL_SLVTORST			(6)
#define NPCM_SSCTL_SLVTOIEN			(5)
#define NPCM_SSCTL_SLV3WIRE			(4)
#define NPCM_SSCTL_AUTOSS			(3)
#define NPCM_SSCTL_SSACTPOL			(2)
#define NPCM_SSCTL_SS			(0)

/* 0x0C: SPI_PDMACTL fields */
#define NPCM_PDMACTL_PDMARST			(2)
#define NPCM_PDMACTL_RXPDMAEN		(1)
#define NPCM_PDMACTL_TXPDMAEN		(0)

/* 0x10: SPI_FIFOCTL fields */
#define NPCM_FIFOCTL_TXTH			(28)
#define NPCM_FIFOCTL_RXTH			(24)
#define NPCM_FIFOCTL_TXUFIEN			(7)
#define NPCM_FIFOCTL_TXUFPOL			(6)
#define NPCM_FIFOCTL_RXOVIEN			(5)
#define NPCM_FIFOCTL_RXTOIEN			(4)
#define NPCM_FIFOCTL_TXTHIEN			(3)
#define NPCM_FIFOCTL_RXTHIEN			(2)
#define NPCM_FIFOCTL_TXRST			(1)
#define NPCM_FIFOCTL_RXRST			(0)

/* 0x14: SPI_STATUS fields */
#define NPCM_STATUS_TXCNT			(28)
#define NPCM_STATUS_RXCNT			(24)
#define NPCM_STATUS_TXRXRST			(23)
#define NPCM_STATUS_TXUFIF			(19)
#define NPCM_STATUS_TXTHIF			(18)
#define NPCM_STATUS_TXFULL			(17)
#define NPCM_STATUS_TXEMPTY			(16)
#define NPCM_STATUS_SPIENSTS			(15)
#define NPCM_STATUS_RXTOIF			(12)
#define NPCM_STATUS_RXOVIF			(11)
#define NPCM_STATUS_RXTHIF			(10)
#define NPCM_STATUS_RXFULL			(9)
#define NPCM_STATUS_RXEMPTY			(8)
#define NPCM_STATUS_SLVUDRIF			(7)
#define NPCM_STATUS_SLVBEIF			(6)
#define NPCM_STATUS_SLVTOIF			(5)
#define NPCM_STATUS_SSLINE			(4)
#define NPCM_STATUS_SSINAIF			(3)
#define NPCM_STATUS_SSACTIF			(2)
#define NPCM_STATUS_UNITIF			(1)
#define NPCM_STATUS_BUSY			(0)

/* 0x20: SPI_TX fields */
#define NPCM_TX_TX				(0)

/* 0x30: SPI_RX fields */
#define NPCM_RX_RX				(0)

#define NPCM_SPIP_SINGLE_DUMMY_BYTE		0x8

#define NPCM_SPIP_SPI_NOR_READ_INIT		0
#define NPCM_SPIP_SPI_NOR_WRITE_INIT		1
#define NPCM_SPIP_SPI_NOR_READ_INIT_OK	BIT(NPCM_SPIP_SPI_NOR_READ_INIT)
#define NPCM_SPIP_SPI_NOR_WRITE_INIT_OK	BIT(NPCM_SPIP_SPI_NOR_WRITE_INIT)

/*
 * SPI Synchronous serial Interface Controller (SPIM) device registers
 */
struct spim_reg {
	/* 0x000: Control and Status Register 0 */
	volatile uint32_t SPIM_CTL0;
	/* 0x004: Control and Status Register 1 */
	volatile uint32_t SPIM_CTL1;
	volatile uint32_t reserved1;
	/* 0x00C: RX Clock Delay Control Register */
	volatile uint32_t SPIM_RXCLKDLY;
	/* 0x010: Data Receive Register 0 */
	volatile uint32_t SPIM_RX0;
	/* 0x014: Data Receive Register 1 */
	volatile uint32_t SPIM_RX1;
	/* 0x018: Data Receive Register 2 */
	volatile uint32_t SPIM_RX2;
	/* 0x01C: Data Receive Register 3 */
	volatile uint32_t SPIM_RX3;
	/* 0x020: Data Transmit Register 0 */
	volatile uint32_t SPIM_TX0;
	/* 0x024: Data Transmit Register 1 */
	volatile uint32_t SPIM_TX1;
	/* 0x028: Data Transmit Register 2 */
	volatile uint32_t SPIM_TX2;
	/* 0x02C: Data Transmit Register 3 */
	volatile uint32_t SPIM_TX3;
	/* 0x030: SRAM Memory Address Register */
	volatile uint32_t SPIM_SRAMADDR;
	/* 0x034: DMA Transfer Byte Count Register */
	volatile uint32_t SPIM_DMACNT;
	/* 0x038: SPI Flash Address Register */
	volatile uint32_t SPIM_FADDR;
	volatile uint32_t reserved2[2];
	/* 0x044: Direct Memory Mapping Mode Control Register */
	volatile uint32_t SPIM_DMMCTL;
	/* 0x048: Control Register 2 */
	volatile uint32_t SPIM_CTL2;
};

/* SPIM register fields */

/* 0x000: SPIM_CTL0 */
#define NPCM_SPIM_CTL0_CMDCODE		FIELD(24, 8)

#define NPCM_SPIM_CTL0_OPMODE		FIELD(22, 2)
#define NPCM_SPIM_CTL0_OPMODE_NORMAL_IO      0x0
#define NPCM_SPIM_CTL0_OPMODE_DMA_WRITE      0x1
#define NPCM_SPIM_CTL0_OPMODE_DMA_READ       0x2
#define NPCM_SPIM_CTL0_OPMODE_DMM            0x3

#define NPCM_SPIM_CTL0_BITMODE		FIELD(20, 2)
#define NPCM_SPIM_CTL0_BITMODE_STANDARD      0x0
#define NPCM_SPIM_CTL0_BITMODE_DUAL          0x1
#define NPCM_SPIM_CTL0_BITMODE_QUAD          0x2

#define NPCM_SPIM_CTL0_SUSPITV		FIELD(16, 4)
#define NPCM_SPIM_CTL0_QDIODIR		15

#define NPCM_SPIM_CTL0_BURSTNUM		FIELD(13, 2)
#define NPCM_SPIM_CTL0_BURSTNUM_1            0x0
#define NPCM_SPIM_CTL0_BURSTNUM_2            0x1
#define NPCM_SPIM_CTL0_BURSTNUM_3            0x2
#define NPCM_SPIM_CTL0_BURSTNUM_4            0x3

#define NPCM_SPIM_CTL0_DWIDTH		FIELD(8, 5)
#define NPCM_SPIM_CTL0_DWIDTH_8              0x7
#define NPCM_SPIM_CTL0_DWIDTH_16             0xF
#define NPCM_SPIM_CTL0_DWIDTH_24             0x17
#define NPCM_SPIM_CTL0_DWIDTH_32             0x1F

#define NPCM_SPIM_CTL0_IF			7
#define NPCM_SPIM_CTL0_IEN			6
#define NPCM_SPIM_CTL0_B4ADDREN		5
#define NPCM_SPIM_CTL0_CIPHOFF		0

/* 0x004: SPIM_CTL1 */
#define NPCM_SPIM_CTL1_DIVIDER		FIELD(16, 16)
#define NPCM_SPIM_CTL1_IDLE_TIME		FIELD(8, 4)
#define NPCM_SPIM_CTL1_SSACTPOL		5
#define NPCM_SPIM_CTL1_SS			4
#define NPCM_SPIM_CTL1_CDINVAL		3
#define NPCM_SPIM_CTL1_CACHEOFF		1
#define NPCM_SPIM_CTL1_SPIMEN		0

/* 0x00C: SPIM_RXCLKDLY */
#define NPCM_SPIM_RXCLKDLY_RDDLYSEL		FIELD(16, 3)
#define NPCM_SPIM_RXCLKDLY_PHDELSEL          FIELD(8, 8)
#define NPCM_SPIM_RXCLKDLY_DWDELSEL		FIELD(0, 8)

/* 0x044: SPIM_DMMCTL */
#define NPCM_SPIM_DMMCTL_ACTSCLKT		FIELD(28, 4)
#define NPCM_SPIM_DMMCTL_UACTSCLK		26
#define NPCM_SPIM_DMMCTL_CREN		25
#define NPCM_SPIM_DMMCTL_BWEN		24
#define NPCM_SPIM_DMMCTL_DESELTIM		FIELD(16, 5)
#define NPCM_SPIM_DMMCTL_CRMDAT		FIELD(8, 8)

/* 0x048: SPIM_CTL2 */
#define NPCM_SPIM_CTL2_DCNUM			FIELD(24, 5)
#define NPCM_SPIM_CTL2_USETEN		16

/* SPIM read write command init flags */
#define NPCM_SPIM_SPI_NOR_READ_INIT		0
#define NPCM_SPIM_SPI_NOR_WRITE_INIT		1
#define NPCM_SPIM_SPI_NOR_READ_INIT_OK	BIT(NPCM_SPIM_SPI_NOR_READ_INIT)
#define NPCM_SPIM_SPI_NOR_WRITE_INIT_OK	BIT(NPCM_SPIM_SPI_NOR_WRITE_INIT)

#define NPCM_SPIM_SINGLE_DUMMY_BYTE		0x8

/* Flash Interface Unit (FIU) device registers */
struct fiu_reg {
	volatile uint8_t reserved1;
	/* 0x001: Burst Configuration */
	volatile uint8_t BURST_CFG;
	/* 0x002: FIU Response Configuration */
	volatile uint8_t RESP_CFG;
	volatile uint8_t reserved2[17];
	/* 0x014: SPI Flash Configuration */
	volatile uint8_t SPI_FL_CFG;
	volatile uint8_t reserved3;
	/* 0x016: UMA Code Byte */
	volatile uint8_t UMA_CODE;
	/* 0x017: UMA Address Byte 0 */
	volatile uint8_t UMA_AB0;
	/* 0x018: UMA Address Byte 1 */
	volatile uint8_t UMA_AB1;
	/* 0x019: UMA Address Byte 2 */
	volatile uint8_t UMA_AB2;
	/* 0x01A: UMA Data Byte 0 */
	volatile uint8_t UMA_DB0;
	/* 0x01B: UMA Data Byte 1 */
	volatile uint8_t UMA_DB1;
	/* 0x01C: UMA Data Byte 2 */
	volatile uint8_t UMA_DB2;
	/* 0x01D: UMA Data Byte 3 */
	volatile uint8_t UMA_DB3;
	/* 0x01E: UMA Control and Status */
	volatile uint8_t UMA_CTS;
	/* 0x01F: UMA Extended Control and Status */
	volatile uint8_t UMA_ECTS;
	/* 0x020: UMA Data Bytes 0-3 */
	volatile uint32_t UMA_DB0_3;
	volatile uint8_t reserved4[2];
	/* 0x026: CRC Control Register */
	volatile uint8_t CRCCON;
	/* 0x027: CRC Entry Register */
	volatile uint8_t CRCENT;
	/* 0x028: CRC Initialization and Result Register */
	volatile uint32_t CRCRSLT;
	volatile uint8_t reserved5[2];
	/* 0x02E: FIU Read Command for Back-up flash */
	volatile uint8_t RD_CMD_BACK;
	volatile uint8_t reserved6;
	/* 0x030: FIU Read Command for private flash */
	volatile uint8_t RD_CMD_PVT;
	/* 0x031: FIU Read Command for shared flash */
	volatile uint8_t RD_CMD_SHD;
	volatile uint8_t reserved7;
	/* 0x033: FIU Extended Configuration */
	volatile uint8_t FIU_EXT_CFG;
	/* 0x034: UMA AB0~3 */
	volatile uint32_t UMA_AB0_3;
	volatile uint8_t reserved8[4];
	/* 0x03C: Set command enable in 4 Byte address mode */
	volatile uint8_t SET_CMD_EN;
	/* 0x03D: 4 Byte address mode Enable */
	volatile uint8_t ADDR_4B_EN;
	volatile uint8_t reserved9[3];
	/* 0x041: Master Inactive Counter Threshold */
	volatile uint8_t MI_CNT_THRSH;
	/* 0x042: FIU Matser Status */
	volatile uint8_t FIU_MSR_STS;
	/* 0x043: FIU Master Interrupt Enable and Configuration */
	volatile uint8_t FIU_MSR_IE_CFG;
	/* 0x044: Quad Program Enable */
	volatile uint8_t Q_P_EN;
	volatile uint8_t reserved10[3];
	/* 0x048: Extended Data Byte Configuration */
	volatile uint8_t EXT_DB_CFG;
	/* 0x049: Direct Write Configuration */
	volatile uint8_t DIRECT_WR_CFG;
	volatile uint8_t reserved11[6];
	/* 0x050 ~ 0x060: Extended Data Byte F to 0 */
	volatile uint8_t EXT_DB_F_0[16];
};

/* FIU register fields */
/* 0x001: BURST CFG */
#define NPCM_BURST_CFG_R_BURST               FIELD(0, 2)
#define NPCM_BURST_CFG_SLAVE                 2
#define NPCM_BURST_CFG_UNLIM_BURST           3
#define NPCM_BURST_CFG_SPI_WR_EN             7

/* 0x002: RESP CFG */
#define NCPM4XX_RESP_CFG_QUAD_EN                3

/* 0x014: SPI FL CFG */
#define NPCM_SPI_FL_CFG_RD_MODE              FIELD(6, 2)
#define NPCM_SPI_FL_CFG_RD_MODE_NORMAL       0
#define NPCM_SPI_FL_CFG_RD_MODE_FAST         1
#define NPCM_SPI_FL_CFG_RD_MODE_FAST_DUAL    3

/* 0x01E: UMA CTS */
#define NPCM_UMA_CTS_D_SIZE                  FIELD(0, 3)
#define NPCM_UMA_CTS_A_SIZE                  3
#define NPCM_UMA_CTS_C_SIZE                  4
#define NPCM_UMA_CTS_RD_WR                   5
#define NPCM_UMA_CTS_DEV_NUM                 6
#define NPCM_UMA_CTS_EXEC_DONE               7

/* 0x01F: UMA ECTS */
#define NPCM_UMA_ECTS_SW_CS0                 0
#define NPCM_UMA_ECTS_SW_CS1                 1
#define NPCM_UMA_ECTS_SW_CS2                 2
#define NPCM_UMA_ECTS_DEV_NUM_BACK           3
#define NPCM_UMA_ECTS_UMA_ADDR_SIZE          FIELD(4, 3)

/* 0x026: CRC Control Register */
#define NPCM_CRCCON_CALCEN                   0
#define NPCM_CRCCON_CKSMCRC                  1
#define NCPM4XX_CRCCON_UMAENT                   2

/* 0x033: FIU Extended Configuration Register */
#define NPCM_FIU_EXT_CFG_FOUR_BADDR          0

/* 0x03C: Set command enable 4 bytes address mode */
#define NPCM_SET_CMD_EN_PVT_CMD_EN           4
#define NCPM4XX_SET_CMD_EN_SHD_CMD_EN           5
#define NCPM4XX_SET_CMD_EN_BACK_CMD_EN          6

/* 0x03D: 4 bytes address mode enable */
#define NPCM_ADDR_4B_EN_PVT_4B               4
#define NCPM4XX_ADDR_4B_EN_SHD_4B               5
#define NCPM4XX_ADDR_4B_EN_BACK_4B              6

/* 0x043: FIU master interrupt enable and configuration register */
#define NPCM_FIU_MSR_IE_CFG_RD_PEND_UMA_IE   0
#define NPCM_FIU_MSR_IE_CFG_RD_PEND_FIU_IE   1
#define NPCM_FIU_MSR_IE_CFG_MSTR_INACT_IE    2
#define NPCM_FIU_MSR_IE_CFG_UMA_BLOCK        3

/* 0x044: Quad program enable register */
#define NPCM_Q_P_EN_QUAD_P_EN                0

/* 0x048: Extended data byte configurartion */
#define NPCM_EXT_DB_CFG_D_SIZE_DB            FIELD(0, 5)
#define NPCM_EXT_DB_CFG_EXT_DB_EN            5

/* 0x049: Direct write configuration */
#define NPCM_DIRECT_WR_CFG_DIRECT_WR_BLOCK   1
#define NPCM_DIRECT_WR_CFG_DW_CS2            5
#define NPCM_DIRECT_WR_CFG_DW_CS1            6
#define NPCM_DIRECT_WR_CFG_DW_CS0            7

/* BURST CFG R_BURST selections */
#define NPCM_BURST_CFG_R_BURST_1B            0
#define NPCM_BURST_CFG_R_BURST_16B           3

/* FIU read write command init flags */
#define NPCM_FIU_SPI_NOR_READ_INIT           0
#define NPCM_FIU_SPI_NOR_WRITE_INIT          1
#define NPCM_FIU_SPI_NOR_READ_INIT_OK        BIT(NPCM_FIU_SPI_NOR_READ_INIT)
#define NPCM_FIU_SPI_NOR_WRITE_INIT_OK       BIT(NPCM_FIU_SPI_NOR_WRITE_INIT)

#define NPCM_FIU_SINGLE_DUMMY_BYTE           0x8
#define NPCM_FIU_ADDR_3B_LENGTH              0x3
#define NPCM_FIU_ADDR_4B_LENGTH              0x4
#define NPCM_FIU_EXT_DB_SIZE                 0x10

/* UMA fields selections */
#define UMA_FLD_ADDR     BIT(NPCM_UMA_CTS_A_SIZE)  /* 3-bytes ADR field */
#define UMA_FLD_NO_CMD   BIT(NPCM_UMA_CTS_C_SIZE)  /* No 1-Byte CMD field */
#define UMA_FLD_WRITE    BIT(NPCM_UMA_CTS_RD_WR)   /* Write transaction */
#define UMA_FLD_SHD_SL   BIT(NPCM_UMA_CTS_DEV_NUM) /* Shared flash selected */
#define UMA_FLD_EXEC     BIT(NPCM_UMA_CTS_EXEC_DONE)

#define UMA_FIELD_ADDR_0 0x00
#define UMA_FIELD_ADDR_1 0x01
#define UMA_FIELD_ADDR_2 0x02
#define UMA_FIELD_ADDR_3 0x03
#define UMA_FIELD_ADDR_4 0x04

#define UMA_FIELD_DATA_0 0x00
#define UMA_FIELD_DATA_1 0x01
#define UMA_FIELD_DATA_2 0x02
#define UMA_FIELD_DATA_3 0x03
#define UMA_FIELD_DATA_4 0x04

/* UMA code for transaction */
#define UMA_CODE_ONLY_WRITE           (UMA_FLD_EXEC | UMA_FLD_WRITE)
#define UMA_CODE_ONLY_READ_BYTE(n)    (UMA_FLD_EXEC | UMA_FLD_NO_CMD | UMA_FIELD_DATA_##n)

/*
 * Enhanced Serial Peripheral Interface (eSPI) device registers
 */
struct espi_reg {
	volatile uint8_t reserved0[4];
	/* 0x004: eSPI Configuration */
	volatile uint32_t ESPICFG;
	/* 0x008: eSPI Status */
	volatile uint32_t ESPISTS;
	/* 0x00C: eSPI Interrupt Enable */
	volatile uint32_t ESPIIE;
	/* 0x010: eSPI Wake-Up Enable */
	volatile uint32_t ESPIWE;
	/* 0x014: Virtual Wire Register Index */
	volatile uint32_t VWREGIDX;
	/* 0x018: Virtual Wire Register Data */
	volatile uint32_t VWREGDATA;
	/* 0x01C: OOB Receive Buffer Read Head */
	volatile uint32_t OOBRXRDHEAD;
	/* 0x020: OOB Transmit Buffer Write Head */
	volatile uint32_t OOBTXWRHEAD;
	/* 0x024: OOB Channel Control */
	volatile uint32_t OOBCTL;
	/* 0x028: Flash Receive Buffer Read Head */
	volatile uint32_t FLASHRXRDHEAD;
	/* 0x02C: Flash Transmit Buffer Write Head */
	volatile uint32_t FLASHTXWRHEAD;
	volatile uint8_t reserved1[4];
	/* 0x034: Flash Channel Configuration */
	volatile uint32_t FLASHCFG;
	/* 0x038: Flash Channel Control */
	volatile uint32_t FLASHCTL;
	/* 0x03C: eSPI Error Status */
	volatile uint32_t ESPIERR;
	volatile uint8_t reserved2[16];
	/* 0x0050 Status Image Register(Host-side) */
	volatile uint16_t STATUS_IMG;
	volatile uint8_t  reserved3[174];
	/* 0x0100 Virtual Wire Event Slave-to-Master 0-9 */
	volatile uint32_t VWEVSM[10];
	volatile uint8_t reserved4[24];
	/* 0x0140 Virtual Wire Event Master-to-Slave 0-11 */
	volatile uint32_t VWEVMS[12];
	volatile uint8_t  reserved5[144];
	/* 0x0200 Virtual Wire Event Master-toSlave Status */
	volatile uint32_t VWEVMS_STS;
	volatile uint8_t  reserved6[4];
	/* 0x0208 Virtual Wire Event Slave-to-Master Type */
	volatile uint32_t VWEVSMTYPE;
	volatile uint8_t  reserved7[240];
	/* 0x02FC Virtual Wire Channel Control */
	volatile uint32_t VWCTL;
	/* 0x0300 OOB Receive Buffer */
	volatile uint32_t OOBRXBUF[20];
	volatile uint8_t  reserved8[48];
	/* 0x0380 OOB Transmit Buffer */
	volatile uint32_t OOBTXBUF[20];
	volatile uint8_t  reserved9[44];
	/* 0x03FC OOB Channel Control (Option) */
	volatile uint32_t OOBCTL_OPT;
	/* 0x0400 Flash Receive Buffer */
	volatile uint32_t FLASHRXBUF[18];
	volatile uint8_t  reserved10[56];
	/* 0x0480 Flash Transmit Buffer */
	volatile uint32_t FLASHTXBUF[18];
	volatile uint8_t  reserved11[24];
	/* 0x04E0 Flash Channel Configuration 2 */
	volatile uint32_t FLASHCFG2;
	/* 0x04E4 Flash Channel Configuration 3 */
	volatile uint32_t FLASHCFG3;
	/* 0x04E8 Flash Channel Configuration 4 */
	volatile uint32_t FLASHCFG4;
	volatile uint8_t  reserved12[4];
	/* 0x04F0 Flash Base */
	volatile uint32_t FLASHBASE;
	volatile uint8_t  reserved13[4];
	/* 0x04F8 Flash Channel Configuration (Option) */
	volatile uint32_t FLASHCFG_OPT;
	/* 0x04FC Flash Channel Control (Option) */
	volatile uint32_t FLASHCTL_OPT;
	volatile uint8_t  reserved14[256];
	/* 0x0600 Flash Protection Range Base Address Register */
	volatile uint32_t FLASH_PRTR_BADDR[16];
	/* 0x0640 Flash Protection Range High Address Register */
	volatile uint32_t FLASH_PRTR_HADDR[16];
	/* 0x0680 Flash Region TAG Override Register */
	volatile uint32_t FLASH_RGN_TAG_OVR[8];
};

/* eSPI register fields */
#define NPCM_ESPICFG_PCHANEN             0
#define NPCM_ESPICFG_VWCHANEN            1
#define NPCM_ESPICFG_OOBCHANEN           2
#define NPCM_ESPICFG_FLASHCHANEN         3
#define NPCM_ESPICFG_HPCHANEN            4
#define NPCM_ESPICFG_HVWCHANEN           5
#define NPCM_ESPICFG_HOOBCHANEN          6
#define NPCM_ESPICFG_HFLASHCHANEN        7
#define NPCM_ESPICFG_CHANS_FIELD         FIELD(0, 4)
#define NPCM_ESPICFG_HCHANS_FIELD        FIELD(4, 4)
#define NPCM_ESPICFG_IOMODE_FIELD        FIELD(8, 2)
#define NPCM_ESPICFG_MAXFREQ_FIELD       FIELD(10, 3)
#define NPCM_ESPICFG_VWMS_VALID_EN       13
#define NPCM_ESPICFG_VWSM_VALID_EN       14
#define NPCM_ESPICFG_PCCHN_SUPP          24
#define NPCM_ESPICFG_VWCHN_SUPP          25
#define NPCM_ESPICFG_OOBCHN_SUPP         26
#define NPCM_ESPICFG_FLASHCHN_SUPP       27
#define NPCM_ESPIIE_IBRSTIE              0
#define NPCM_ESPIIE_CFGUPDIE             1
#define NPCM_ESPIIE_BERRIE               2
#define NPCM_ESPIIE_OOBRXIE              3
#define NPCM_ESPIIE_FLASHRXIE            4
#define NPCM_ESPIIE_SFLASHRDIE           5
#define NPCM_ESPIIE_PERACCIE             6
#define NPCM_ESPIIE_DFRDIE               7
#define NPCM_ESPIIE_VWUPDIE              8
#define NPCM_ESPIIE_ESPIRSTIE            9
#define NPCM_ESPIIE_PLTRSTIE             10
#define NPCM_ESPIIE_AMERRIE              15
#define NPCM_ESPIIE_AMDONEIE             16
#define NPCM_ESPIIE_BMTXDONEIE           19
#define NPCM_ESPIIE_PBMRXIE              20
#define NPCM_ESPIIE_PMSGRXIE             21
#define NPCM_ESPIIE_BMBURSTERRIE         22
#define NPCM_ESPIIE_BMBURSTDONEIE        23
#define NPCM_ESPIWE_IBRSTWE              0
#define NPCM_ESPIWE_CFGUPDWE             1
#define NPCM_ESPIWE_BERRWE               2
#define NPCM_ESPIWE_OOBRXWE              3
#define NPCM_ESPIWE_FLASHRXWE            4
#define NPCM_ESPIWE_PERACCWE             6
#define NPCM_ESPIWE_DFRDWE               7
#define NPCM_ESPIWE_VWUPDWE              8
#define NPCM_ESPIWE_ESPIRSTWE            9
#define NPCM_ESPIWE_PBMRXWE              20
#define NPCM_ESPIWE_PMSGRXWE             21
#define NPCM_ESPISTS_IBRST               0
#define NPCM_ESPISTS_CFGUPD              1
#define NPCM_ESPISTS_BERR                2
#define NPCM_ESPISTS_OOBRX               3
#define NPCM_ESPISTS_FLASHRX             4
#define NPCM_ESPISTS_SFLASHRD            5
#define NPCM_ESPISTS_PERACC              6
#define NPCM_ESPISTS_DFRD                7
#define NPCM_ESPISTS_VWUPD               8
#define NPCM_ESPISTS_ESPIRST             9
#define NPCM_ESPISTS_PLTRST              10
#define NPCM_ESPISTS_ESPIRST_DEASSERT    17
#define NPCM_ESPISTS_PLTRST_DEASSERT     18
#define NPCM_ESPISTS_FLASHPRTERR         25
#define NPCM_ESPISTS_FLASHAUTORDREQ      26
#define NPCM_ESPISTS_VWUPDW              27
#define NPCM_ESPISTS_GB_RST              28
#define NPCM_ESPISTS_DNX_RST             29
#define NPCM_VWEVMS_WIRE                 FIELD(0, 4)
#define NPCM_VWEVMS_VALID                FIELD(4, 4)
#define NPCM_VWEVMS_IE                   18
#define NPCM_VWEVMS_WE                   20
#define NPCM_VWEVSM_WIRE                 FIELD(0, 4)
#define NPCM_VWEVSM_VALID                FIELD(4, 4)
#define NPCM_VWEVSM_BIT_VALID(n)         (4+n)
#define NPCM_VWEVSM_HW_WIRE              FIELD(24, 4)
#define NPCM_OOBCTL_OOB_FREE             0
#define NPCM_OOBCTL_OOB_AVAIL            1
#define NPCM_OOBCTL_RSTBUFHEADS          2
#define NPCM_OOBCTL_OOBPLSIZE            FIELD(10, 3)
#define NPCM_FLASHCFG_FLASHBLERSSIZE     FIELD(7, 3)
#define NPCM_FLASHCFG_FLASHPLSIZE        FIELD(10, 3)
#define NPCM_FLASHCFG_FLASHREQSIZE       FIELD(13, 3)
#define NPCM_FLASHCTL_FLASH_NP_FREE      0
#define NPCM_FLASHCTL_FLASH_TX_AVAIL     1
#define NPCM_FLASHCTL_STRPHDR            2
#define NPCM_FLASHCTL_DMATHRESH          FIELD(3, 2)
#define NPCM_FLASHCTL_AMTSIZE            FIELD(5, 8)
#define NPCM_FLASHCTL_RSTBUFHEADS        13
#define NPCM_FLASHCTL_CRCEN              14
#define NPCM_FLASHCTL_CHKSUMSEL          15
#define NPCM_FLASHCTL_AMTEN              16
#define NPCM_ESPIHINDP_AUTO_PCRDY        0
#define NPCM_ESPIHINDP_AUTO_VWCRDY       1
#define NPCM_ESPIHINDP_AUTO_OOBCRDY      2
#define NPCM_ESPIHINDP_AUTO_FCARDY       3

/*
 * Mobile System Wake-Up Control (MSWC) device registers
 */
struct mswc_reg {
	/* 0x000: MSWC Control Status 1 */
	volatile uint8_t MSWCTL1;
	volatile uint8_t reserved1;
	/* 0x002: MSWC Control Status 2 */
	volatile uint8_t MSWCTL2;
	volatile uint8_t reserved2[5];
	/* 0x008: Host Configuration Base Address Low */
	volatile uint8_t HCBAL;
	volatile uint8_t reserved3;
	/* 0x00A: Host Configuration Base Address High */
	volatile uint8_t HCBAH;
	volatile uint8_t reserved4;
	/* 0X00C: MSWC INTERRUPT ENABLE 2 */
	volatile uint8_t MSIEN2;
	volatile uint8_t reserved5;
	/* 0x00E: MSWC Host Event Status 0 */
	volatile uint8_t MSHES0;
	volatile uint8_t reserved6;
	/* 0x010: MSWC Host Event Interrupt Enable */
	volatile uint8_t MSHEIE0;
	volatile uint8_t reserved7;
	/* 0x012: Host Control */
	volatile uint8_t HOST_CTL;
	volatile uint8_t reserved8;
	/* 0x014: SMI Pulse Length */
	volatile uint8_t SMIP_LEN;
	volatile uint8_t reserved9;
	/* 0x016: SCI Pulse Length */
	volatile uint8_t SCIP_LEN;
	volatile uint8_t reserved10;
	/* 0x018: LPC event control register */
	volatile uint8_t LPC_EVENT;
	volatile uint8_t reserved11;
	/* 0x01A: LPC status register */
	volatile uint8_t LPC_STS;
	volatile uint8_t reserved12;
	/* 0x01C: SRID Core Access */
	volatile uint8_t SRID_CR;
	volatile uint8_t reserved13[3];
	/* 0x020: SID Core Access */
	volatile uint8_t SID_CR;
	volatile uint8_t reserved14;
	/* 0x022: DEVICE_ID Core Access */
	volatile uint8_t DEVICE_ID_CR;
	volatile uint8_t reserved15[11];
	/* 0x02E: Virtual Wire Sleep States */
	volatile uint8_t VW_SLPST;
};

/* MSWC register fields */
#define NPCM_MSWCTL1_HRSTOB              0
#define NPCM_MSWCTL1_HWPRON		    1
#define NPCM_MSWCTL1_PLTRST_ACT          2
#define NPCM_MSWCTL1_VHCFGA              3
#define NPCM_MSWCTL1_HCFGLK              4
#define NPCM_MSWCTL1_A20MB               7

/*
 * Shared Memory (SHM) device registers
 */
struct shm_reg {
	/* 0x000: Shared Memory Core Status */
	volatile uint8_t SMC_STS;
	/* 0x001: Shared Memory Core Control */
	volatile uint8_t SMC_CTL;
	/* 0x002: Shared Memory Host Control */
	volatile uint8_t SHM_CTL;
	volatile uint8_t reserved1[2];
	/* 0x005: Indirect Memory Access Window Size */
	volatile uint8_t IMA_WIN_SIZE;
	volatile uint8_t reserved2;
	/* 0x007: Shared Access Windows Size */
	volatile uint8_t WIN_SIZE;
	/* 0x008: Shared Access Window 1, Semaphore */
	volatile uint8_t SHAW1_SEM;
	/* 0x009: Shared Access Window 2, Semaphore */
	volatile uint8_t SHAW2_SEM;
	volatile uint8_t reserved3;
	/* 0x00B: Indirect Memory Access, Semaphore */
	volatile uint8_t IMA_SEM;
	volatile uint8_t reserved4[2];
	/* 0x00E: Shared Memory Configuration */
	volatile uint16_t SHCFG;
	/* 0x010: Shared Access Window 1 Write Protect */
	volatile uint8_t WIN1_WR_PROT;
	/* 0x011: Shared Access Window 1 Read Protect */
	volatile uint8_t WIN1_RD_PROT;
	/* 0x012: Shared Access Window 2 Write Protect */
	volatile uint8_t WIN2_WR_PROT;
	/* 0x013: Shared Access Window 2 Read Protect */
	volatile uint8_t WIN2_RD_PROT;
	volatile uint8_t reserved5[2];
	/* 0x016: Indirect Memory Access Write Protect */
	volatile uint8_t IMA_WR_PROT;
	/* 0x017: Indirect Memory Access Read Protect */
	volatile uint8_t IMA_RD_PROT;
	volatile uint8_t reserved6[8];
	/* 0x020: RAM Window 1 Base */
	volatile uint32_t WIN_BASE1;
	/* 0x024: RAM Window 2 Base */
	volatile uint32_t WIN_BASE2;
	volatile uint8_t reserved7[4];
	/* 0x02C: Immediate Memory Access Base */
	volatile uint32_t IMA_BASE;
	volatile uint8_t reserved8[10];
	/* 0x03A: Reset Configuration */
	volatile uint8_t RST_CFG;
	volatile uint8_t reserved9;
	/* 0x03C: Debug Port 80 Buffered Data 1 */
	volatile uint32_t DP80BUF1;
	/* 0x040: Debug Port 80 Buffered Data */
	volatile uint16_t DP80BUF;
	/* 0x042: Debug Port 80 Status */
	volatile uint8_t DP80STS;
	/* 0x043: Debug Port 80 Interrupt Enable */
	volatile uint8_t DP80IE;
	/* 0x044: Debug Port 80 Control */
	volatile uint8_t DP80CTL;
	volatile uint8_t reserved10[3];
	/* 0x048: Host_Offset in Windows 1,2,3,4 Status */
	volatile uint8_t HOFS_STS;
	/* 0x049: Host_Offset in Windows 1,2,3,4 Control */
	volatile uint8_t HOFS_CTL;
	/* 0x04A: Core_Offset in Window 2 Address */
	volatile uint16_t COFS2;
	/* 0x04C: Core_Offset in Window 1 Address */
	volatile uint16_t COFS1;
	/* 0x4E Shared Memory Core Control 2 */
	volatile uint8_t SMC_CTL2;
	volatile uint8_t reserved11;
	/* 0x50 Host Offset in windows 2 address */
	volatile uint16_t IHOFS2;
	/* 0x52 Host Offset in windows 1 address */
	volatile uint16_t IHOFS1;
	/* 0x54 Shared Access Window 3, Semaphore */
	volatile uint8_t SHAW3_SEM;
	/* 0x55 Shared Access Window 4, Semaphore */
	volatile uint8_t SHAW4_SEM;
	/* 0x56 Shared Memory Window 3 Write Protect */
	volatile uint8_t WIN3_WR_PROT;
	/* 0x57 Shared Memory Window 3 Read Protect */
	volatile uint8_t WIN3_RD_PROT;
	/* 0x58 Shared Memory Window 4 Write Protect */
	volatile uint8_t WIN4_WR_PROT;
	/* 0x59 Shared Memory Window 4 Read Protect */
	volatile uint8_t WIN4_RD_PROT;
	/* 0x5A RAM Windows Size 2 */
	volatile uint8_t WIN_SIZE2;
	volatile uint8_t reserved12;
	/* 0x5C RAM Window 3 Base */
	volatile uint32_t WIN_BASE3;
	/* 0x60 RAM Window 4 Base */
	volatile uint32_t WIN_BASE4;
	/* 0x64 Core_Offset in Window 3 Address */
	volatile uint16_t COFS3;
	/* 0x66 Core_Offset in Window 4 Address */
	volatile uint16_t COFS4;
	/* 0x68 Host Offset in windows 3 address */
	volatile uint16_t IHOFS3;
	/* 0x6A Host Offset in windows 4 address */
	volatile uint16_t IHOFS4;
	/* 0x6C Shared Memory Core Status 2 */
	volatile uint8_t SMC_STS2;
	/* 0x6D Indirect Memory Access 2, Semaphore */
	volatile uint8_t IMA2_SEM;
	/* 0x6E Indirect Memory Access 2 Write Protect */
	volatile uint8_t IMA2_WR_PROT;
	/* 0x6F Indirect Memory Access 2 Read Protect */
	volatile uint8_t IMA2_RD_PROT;
	/* 0x70 Immediate Memory Access Base 2 */
	volatile uint32_t IMA2_BASE;
	/* 0x74 Additional Indirect Memory Access, Semaphore */
	volatile uint8_t AIMA_SEM;
	/* 0x75 Host_Offset in Window 5 Status */
	volatile uint8_t HOFS_STS2;
	/* 0x76 Host_Offset in Window 5 Control */
	volatile uint8_t HOFS_CTL2;
	volatile uint8_t reserved13;
	/* 0x78 Additional Immediate Memory Access Base([31:8]) */
	volatile uint32_t AIMA_BASE;
	/* 0x7C Core_Offset in Window 5 Address */
	volatile uint16_t COFS5;
	/* 0x7E Host Offset in windows 5 address */
	volatile uint16_t IHOFS5;
	/* 0x80 Shared Memory Window 5 Write Protect */
	volatile uint8_t WIN5_WR_PROT;
	/* 0x81 Shared Memory Window 5 Read Protect */
	volatile uint8_t WIN5_RD_PROT;
	/* 0x82 Shared Access Window 5, Semaphore */
	volatile uint8_t SHAW5_SEM;
	/* 0x83 RAM Windows Size 3 */
	volatile uint8_t WIN_SIZE3;
	/* 0x84 RAM Window 5 Base */
	volatile uint32_t WIN_BASE5;

};

/* SHM register fields */
#define NPCM_SMC_STS_HRERR               0
#define NPCM_SMC_STS_HWERR               1
#define NPCM_SMC_STS_HSEM1W              4
#define NPCM_SMC_STS_HSEM2W              5
#define NPCM_SMC_STS_SHM_ACC             6
#define NPCM_SMC_CTL_HERR_IE             2
#define NPCM_SMC_CTL_HSEM1_IE            3
#define NPCM_SMC_CTL_HSEM2_IE            4
#define NPCM_SMC_CTL_ACC_IE              5
#define NPCM_SMC_CTL_HSEM_IMA_IE         6
#define NPCM_SMC_CTL_HOSTWAIT            7
#define NPCM_FLASH_SIZE_STALL_HOST       6
#define NPCM_FLASH_SIZE_RD_BURST         7
#define NPCM_WIN_SIZE_RWIN1_SIZE_FIELD   FIELD(0, 4)
#define NPCM_WIN_SIZE_RWIN2_SIZE_FIELD   FIELD(4, 4)
#define NPCM_WIN_PROT_RW1L_RP            0
#define NPCM_WIN_PROT_RW1L_WP            1
#define NPCM_WIN_PROT_RW1H_RP            2
#define NPCM_WIN_PROT_RW1H_WP            3
#define NPCM_WIN_PROT_RW2L_RP            4
#define NPCM_WIN_PROT_RW2L_WP            5
#define NPCM_WIN_PROT_RW2H_RP            6
#define NPCM_WIN_PROT_RW2H_WP            7
#define NPCM_PWIN_SIZEI_RPROT            13
#define NPCM_PWIN_SIZEI_WPROT            14
#define NPCM_CSEM2                       6
#define NPCM_CSEM3                       7
#define NPCM_DP80STS_FWR                 5
#define NPCM_DP80STS_FNE                 6
#define NPCM_DP80STS_FOR                 7
#define NPCM_DP80CTL_DP80EN              0
#define NPCM_DP80CTL_SYNCEN              1
#define NPCM_DP80CTL_ADV                 2
#define NPCM_DP80CTL_RAA                 3
#define NPCM_DP80CTL_RFIFO               4

/*
 * Keyboard and Mouse Controller (KBC) device registers
 */
struct kbc_reg {
	/* 0x000h: Host Interface Control */
	volatile uint8_t HICTRL;
	volatile uint8_t reserved1;
	/* 0x002h: Host Interface IRQ Control */
	volatile uint8_t HIIRQC;
	volatile uint8_t reserved2;
	/* 0x004h: Host Interface Keyboard/Mouse Status */
	volatile uint8_t HIKMST;
	volatile uint8_t reserved3;
	/* 0x006h: Host Interface Keyboard Data Out Buffer */
	volatile uint8_t HIKDO;
	volatile uint8_t reserved4;
	/* 0x008h: Host Interface Mouse Data Out Buffer */
	volatile uint8_t HIMDO;
	volatile uint8_t reserved5;
	/* 0x00Ah: Host Interface Keyboard/Mouse Data In Buffer */
	volatile uint8_t HIKMDI;
	/* 0x00Bh: Host Interface Keyboard/Mouse Shadow Data In Buffer */
	volatile uint8_t SHIKMDI;
};

/* KBC register field */
#define NPCM_HICTRL_OBFKIE               0
#define NPCM_HICTRL_OBFMIE               1
#define NPCM_HICTRL_OBECIE               2
#define NPCM_HICTRL_IBFCIE               3
#define NPCM_HICTRL_PMIHIE               4
#define NPCM_HICTRL_PMIOCIE              5
#define NPCM_HICTRL_PMICIE               6
#define NPCM_HICTRL_FW_OBF               7
#define NPCM_HIKMST_OBF                  0
#define NPCM_HIKMST_IBF                  1
#define NPCM_HIKMST_F0                   2
#define NPCM_HIKMST_A2                   3
#define NPCM_HIKMST_ST0                  4
#define NPCM_HIKMST_ST1                  5
#define NPCM_HIKMST_ST2                  6
#define NPCM_HIKMST_ST3                  7

/*
 * Power Management Channel (PMCH) device registers
 */

struct pmch_reg {
	/* 0x000: Host Interface PM Status */
	volatile uint8_t HIPMST;
	volatile uint8_t reserved1;
	/* 0x002: Host Interface PM Data Out Buffer */
	volatile uint8_t HIPMDO;
	volatile uint8_t reserved2;
	/* 0x004: Host Interface PM Data In Buffer */
	volatile uint8_t HIPMDI;
	/* 0x005: Host Interface PM Shadow Data In Buffer */
	volatile uint8_t SHIPMDI;
	/* 0x006: Host Interface PM Data Out Buffer with SCI */
	volatile uint8_t HIPMDOC;
	volatile uint8_t reserved3;
	/* 0x008: Host Interface PM Data Out Buffer with SMI */
	volatile uint8_t HIPMDOM;
	volatile uint8_t reserved4;
	/* 0x00A: Host Interface PM Data In Buffer with SCI */
	volatile uint8_t HIPMDIC;
	volatile uint8_t reserved5;
	/* 0x00C: Host Interface PM Control */
	volatile uint8_t HIPMCTL;
	/* 0x00D: Host Interface PM Control 2 */
	volatile uint8_t HIPMCTL2;
	/* 0x00E: Host Interface PM Interrupt Control */
	volatile uint8_t HIPMIC;
	volatile uint8_t reserved6;
	/* 0x010: Host Interface PM Interrupt Enable */
	volatile uint8_t HIPMIE;
};

/* PMCH register field */
#define NPCM_HIPMIE_SCIE                 1
#define NPCM_HIPMIE_SMIE                 2
#define NPCM_HIPMCTL_IBFIE               0
#define NPCM_HIPMCTL_OBEIE               1
#define NPCM_HIPMCTL_SCIPOL              6
#define NPCM_HIPMST_OBF                  0
#define NPCM_HIPMST_IBF                  1
#define NPCM_HIPMST_F0                   2
#define NPCM_HIPMST_CMD                  3
#define NPCM_HIPMST_ST0                  4
#define NPCM_HIPMST_ST1                  5
#define NPCM_HIPMST_ST2                  6
#define NPCM_HIPMST_ST3                  7
#define NPCM_HIPMIC_SMIB                 1
#define NPCM_HIPMIC_SCIB                 2
#define NPCM_HIPMIC_SMIPOL               6

/*
 * Core Access to Host (C2H) device registers
 */
struct c2h_reg {
	/* 0x000: Indirect Host I/O Address */
	volatile uint16_t IHIOA;
	/* 0x002: Indirect Host Data */
	volatile uint8_t IHD;
	volatile uint8_t reserved1;
	/* 0x004: Lock Host Access */
	volatile uint16_t LKSIOHA;
	/* 0x006: Access Lock Violation */
	volatile uint16_t SIOLV;
	/* 0x008: Core-to-Host Modules Access Enable */
	volatile uint16_t CRSMAE;
	/* 0x00A: Module Control */
	volatile uint8_t SIBCTRL;
	volatile uint8_t reserved2;
	/* 0x00C: Lock Host Access 2 */
	volatile uint16_t LKSIOHA2;
	/* 0x00E: Access Lock Violation 2 */
	volatile uint16_t SIOLV2;
	volatile uint8_t reserved3[14];
	/* 0x01E: Core to Host Access Version */
	volatile uint8_t C2H_VER;
};

/* C2H register fields */
#define NPCM_LKSIOHA_LKCFG               0
#define NPCM_LKSIOHA_LKSPHA              2
#define NPCM_LKSIOHA_LKHIKBD             11
#define NPCM_CRSMAE_CFGAE                0
#define NPCM_CRSMAE_HIKBDAE              11
#define NPCM_SIOLV_SPLV                  2
#define NPCM_SIBCTRL_CSAE                0
#define NPCM_SIBCTRL_CSRD                1
#define NPCM_SIBCTRL_CSWR                2

/*
 * i2c device registers
 */
struct i2c_reg {
	/* [0x00] I2C Serial Data */
	volatile uint8_t SMBnSDA;
	volatile uint8_t RESERVE0[1];
	/* [0x02] I2C Status */
	volatile uint8_t SMBnST;
	volatile uint8_t RESERVE1[1];
	/* [0x04] I2C Control Status */
	volatile uint8_t SMBnCST;
	volatile uint8_t RESERVE2[1];
    /* [0x06] I2C Control 1 */
	volatile uint8_t SMBnCTL1;
	volatile uint8_t RESERVE3[1];
    /* [0x08] I2C Own Address 1 */
	volatile uint8_t SMBnADDR1;
    /* [0x09] Timeout Status */
	volatile uint8_t TIMEOUT_ST;
    /* [0x0A] I2C Control 2 */
	volatile uint8_t SMBnCTL2;
    /* [0x0B] Timeout Enable */
	volatile uint8_t TIMEOUT_EN;
    /* [0x0C] I2C Own Address 2 */
	volatile uint8_t SMBnADDR2;
	volatile uint8_t RESERVE4[1];
    /* [0x0E] I2C Control 3 */
	volatile uint8_t SMBnCTL3;
    /* [0x0F] DMA Control */
	volatile uint8_t DMA_CTRL;
    /* [0x10] I2C Own Address 3 */
	volatile uint8_t SMBnADDR3;
    /* [0x11] I2C Own Address 7 */
	volatile uint8_t SMBnADDR7;
    /* [0x12] I2C Own Address 4 */
	volatile uint8_t SMBnADDR4;
    /* [0x13] I2C Own Address 8 */
	volatile uint8_t SMBnADDR8;
    /* [0x14] I2C Own Address 5 */
	volatile uint8_t SMBnADDR5;
    /* [0x15] I2C Own Address 9 */
	volatile uint8_t SMBnADDR9;
    /* [0x16] I2C Own Address 6 */
	volatile uint8_t SMBnADDR6;
    /* [0x17] I2C Own Address 10 */
	volatile uint8_t SMBnADDR10;
    /* [0x18] I2C Control Status 2 */
	volatile uint8_t SMBnCST2;
    /* [0x19] I2C Control Status 3 */
	volatile uint8_t SMBnCST3;
    /* [0x1A] I2C Control 4 */
	volatile uint8_t SMBnCTL4;
    /* [0x1B] I2C Control 5 */
	volatile uint8_t SMBnCTL5;
    /* [0x1C] I2C SCL Low Time (Fast Mode) */
	volatile uint8_t SMBnSCLLT;
    /* [0x1D] I2C Address Match Status */
	volatile uint8_t ADDMTCH_ST;
    /* [0x1E] I2C SCL High Time (Fast Mode) */
	volatile uint8_t SMBnSCLHT;
    /* [0x1F] I2C Version */
	volatile uint8_t SMBn_VER;
    /* [0x20] DMA Address Byte 1 */
	volatile uint8_t DMA_ADDR1;
    /* [0x21] DMA Address Byte 2 */
	volatile uint8_t DMA_ADDR2;
    /* [0x22] DMA Address Byte 3 */
	volatile uint8_t DMA_ADDR3;
    /* [0x23] DMA Address Byte 4 */
	volatile uint8_t DMA_ADDR4;
    /* [0x24] Data Length Byte 1 */
	volatile uint8_t DATA_LEN1;
    /* [0x25] Data Length Byte 2 */
	volatile uint8_t DATA_LEN2;
    /* [0x26] Data Counter Byte 1 */
	volatile uint8_t DATA_CNT1;
    /* [0x27] Data Counter Byte 2 */
	volatile uint8_t DATA_CNT2;
	volatile uint8_t RESERVE7[1];
    /* [0x29] Timeout Control 1 */
	volatile uint8_t TIMEOUT_CTL1;
    /* [0x2A] Timeout Control 2 */
	volatile uint8_t TIMEOUT_CTL2;
    /* [0x2B] I2C PEC Data */
	volatile uint8_t SMBnPEC;
};

/* I2C register fields */
/*--------------------------*/
/*     SMBnST fields        */
/*--------------------------*/
#define NPCM_SMBnST_XMIT                  0
#define NPCM_SMBnST_MASTER                1
#define NPCM_SMBnST_NMATCH                2
#define NPCM_SMBnST_STASTR                3
#define NPCM_SMBnST_NEGACK                4
#define NPCM_SMBnST_BER                   5
#define NPCM_SMBnST_SDAST                 6
#define NPCM_SMBnST_SLVSTP                7

/*--------------------------*/
/*     SMBnCST fields       */
/*--------------------------*/
#define NPCM_SMBnCST_BUSY                 0
#define NPCM_SMBnCST_BB                   1
#define NPCM_SMBnCST_MATCH                2
#define NPCM_SMBnCST_GCMATCH              3
#define NPCM_SMBnCST_TSDA                 4
#define NPCM_SMBnCST_TGSCL                5
#define NPCM_SMBnCST_MATCHAF              6
#define NPCM_SMBnCST_ARPMATCH             7

/*--------------------------*/
/*     SMBnCTL1 fields      */
/*--------------------------*/
#define NPCM_SMBnCTL1_START               0
#define NPCM_SMBnCTL1_STOP                1
#define NPCM_SMBnCTL1_INTEN               2
#define NPCM_SMBnCTL1_EOBINTE             3
#define NPCM_SMBnCTL1_ACK                 4
#define NPCM_SMBnCTL1_GCMEN               5
#define NPCM_SMBnCTL1_NMINTE              6
#define NPCM_SMBnCTL1_STASTRE             7

/*--------------------------*/
/*     SMBnADDR1-10 fields  */
/*--------------------------*/
#define NPCM_SMBnADDR_ADDR                0
#define NPCM_SMBnADDR_SAEN                7

/*--------------------------*/
/*    TIMEOUT_ST fields     */
/*--------------------------*/
#define NPCM_TIMEOUT_ST_T_OUTST1          0
#define NPCM_TIMEOUT_ST_T_OUTST2          1
#define NPCM_TIMEOUT_ST_T_OUTST1_EN       6
#define NPCM_TIMEOUT_ST_T_OUTST2_EN       7

/*--------------------------*/
/*     SMBnCTL2 fields      */
/*--------------------------*/
#define NPCM_SMBnCTL2_ENABLE              0
#define NPCM_SMBnCTL2_SCLFRQ60_FIELD      FIELD(1, 7)

/*--------------------------*/
/*     TIMEOUT_EN fields    */
/*--------------------------*/
#define NPCM_TIMEOUT_EN_TIMEOUT_EN        0
#define NPCM_TIMEOUT_EN_TO_CKDIV          2

/*--------------------------*/
/*     SMBnCTL3 fields      */
/*--------------------------*/
#define NPCM_SMBnCTL3_SCLFRQ87_FIELD      FIELD(0, 2)
#define NPCM_SMBnCTL3_ARPMEN              2
#define NPCM_SMBnCTL3_SLP_START           3
#define NPCM_SMBnCTL3_400K_MODE           4
#define NPCM_SMBnCTL3_SDA_LVL             6
#define NPCM_SMBnCTL3_SCL_LVL             7

/*--------------------------*/
/*      DMA_CTRL fields     */
/*--------------------------*/
#define NPCM_DMA_CTRL_DMA_INT_CLR         0
#define NPCM_DMA_CTRL_DMA_EN              1
#define NPCM_DMA_CTRL_LAST_PEC            2
#define NPCM_DMA_CTRL_DMA_STALL           3
#define NPCM_DMA_CTRL_DMA_IRQ             7

/*--------------------------*/
/*     SMBnCST2 fields      */
/*--------------------------*/
#define NPCM_SMBnCST2_MATCHA1F            0
#define NPCM_SMBnCST2_MATCHA2F            1
#define NPCM_SMBnCST2_MATCHA3F            2
#define NPCM_SMBnCST2_MATCHA4F            3
#define NPCM_SMBnCST2_MATCHA5F            4
#define NPCM_SMBnCST2_MATCHA6F            5
#define NPCM_SMBnCST2_MATCHA7F            6
#define NPCM_SMBnCST2_INTSTS              7

/*--------------------------*/
/*     SMBnCST3 fields      */
/*--------------------------*/
#define NPCM_SMBnCST3_MATCHA8F            0
#define NPCM_SMBnCST3_MATCHA9F            1
#define NPCM_SMBnCST3_MATCHA10F           2
#define NPCM_SMBnCST3_EO_BUSY             7

/*
 * INTERNAL 8-BIT/16-BIT/32-BIT TIMER (ITIM32) device registers
 */

struct itim32_reg {
	/* [0x00] Internal 8-Bit Timer Counter */
	volatile uint8_t CNT;
	/* [0x01] Internal Timer Prescaler */
	volatile uint8_t PRE;
	/* [0x02] Internal 16-Bit Timer Counter */
	volatile uint16_t CNT16;
	/* [0x04] Internal Timer Control and Status */
	volatile uint8_t CTS;
	volatile uint8_t RESERVED1[3];
	/* [0x08] Internal 32-Bit Timer Counter */
	volatile uint32_t CNT32;
};

/* ITIM32 register fields */
#define NPCM_CTS_ITEN                         (7)
#define NPCM_CTS_CKSEL                        (4)
#define NPCM_CTS_TO_WUE                       (3)
#define NPCM_CTS_TO_IE                        (2)
#define NPCM_CTS_TO_STS                       (0)

/*
 * Tachometer (TACH) Sensor device registers
 */
struct tach_reg {
	/* 0x000: Timer/Counter 1 */
	volatile uint16_t TCNT1;
	/* 0x002: Reload/Capture A */
	volatile uint16_t TCRA;
	/* 0x004: Reload/Capture B */
	volatile uint16_t TCRB;
	/* 0x006: Timer/Counter 2 */
	volatile uint16_t TCNT2;
	/* 0x008: Clock Prescaler */
	volatile uint8_t TPRSC;
	volatile uint8_t reserved1;
	/* 0x00A: Clock Unit Control */
	volatile uint8_t TCKC;
	volatile uint8_t reserved2;
	/* 0x00C: Timer Mode Control */
	volatile uint8_t TMCTRL;
	volatile uint8_t reserved3;
	/* 0x00E: Timer Event Control */
	volatile uint8_t TECTRL;
	volatile uint8_t reserved4;
	/* 0x010: Timer Event Clear */
	volatile uint8_t TECLR;
	volatile uint8_t reserved5;
	/* 0x012: Timer Interrupt Enable */
	volatile uint8_t TIEN;
	volatile uint8_t reserved6;
	/* 0x014: Compare A */
	volatile uint16_t TCPA;
	/* 0x016: Compare B */
	volatile uint16_t TCPB;
	/* 0x018: Compare Configuration */
	volatile uint8_t TCPCFG;
	volatile uint8_t reserved7;
	/* 0x01A: Timer Wake-Up Enablen */
	volatile uint8_t TWUEN;
	volatile uint8_t reserved8;
	/* 0x01C: Timer Configuration */
	volatile uint8_t TCFG;
	volatile uint8_t reserved9;
};

/* TACH register fields */
#define NPCM_TCKC_LOW_PWR                7
#define NPCM_TCKC_PLS_ACC_CLK            6
#define NPCM_TCKC_C1CSEL_FIELD           FIELD(0, 3)
#define NPCM_TCKC_C2CSEL_FIELD           FIELD(3, 3)
#define NPCM_TMCTRL_MDSEL_FIELD          FIELD(0, 3)
#define NPCM_TMCTRL_TAEN                 5
#define NPCM_TMCTRL_TBEN                 6
#define NPCM_TMCTRL_TAEDG                3
#define NPCM_TMCTRL_TBEDG                4
#define NPCM_TCFG_TADBEN                 6
#define NPCM_TCFG_TBDBEN                 7
#define NPCM_TECTRL_TAPND                0
#define NPCM_TECTRL_TBPND                1
#define NPCM_TECTRL_TCPND                2
#define NPCM_TECTRL_TDPND                3
#define NPCM_TECLR_TACLR                 0
#define NPCM_TECLR_TBCLR                 1
#define NPCM_TECLR_TCCLR                 2
#define NPCM_TECLR_TDCLR                 3
#define NPCM_TIEN_TAIEN                  0
#define NPCM_TIEN_TBIEN                  1
#define NPCM_TIEN_TCIEN                  2
#define NPCM_TIEN_TDIEN                  3
#define NPCM_TWUEN_TAWEN                 0
#define NPCM_TWUEN_TBWEN                 1
#define NPCM_TWUEN_TCWEN                 2
#define NPCM_TWUEN_TDWEN                 3

/* Debug Interface registers */
struct dbg_reg {
	/* 0x000: Debug Control */
	volatile uint8_t DBGCTRL;
	volatile uint8_t reserved1;
	/* 0x002: Debug Freeze Enable 1 */
	volatile uint8_t DBGFRZEN1;
	/* 0x003: Debug Freeze Enable 2 */
	volatile uint8_t DBGFRZEN2;
	/* 0x004: Debug Freeze Enable 3 */
	volatile uint8_t DBGFRZEN3;
	/* 0x005: Debug Freeze Enable 4 */
	volatile uint8_t DBGFRZEN4;
};
/* Debug Interface registers fields */
#define NPCM_DBGFRZEN3_GLBL_FRZ_DIS      7

/* Platform Environment Control Interface (PECI) device registers */
struct peci_reg {
	/* 0x000: PECI Control Status */
	volatile uint8_t PECI_CTL_STS;
	/* 0x001: PECI Read Length */
	volatile uint8_t PECI_RD_LENGTH;
	/* 0x002: PECI Address */
	volatile uint8_t PECI_ADDR;
	/* 0x003: PECI Command */
	volatile uint8_t PECI_CMD;
	/* 0x004: PECI Control 2 */
	volatile uint8_t PECI_CTL2;
	/* 0x005: PECI Index */
	volatile uint8_t PECI_INDEX;
	/* 0x006: PECI Index Data */
	volatile uint8_t PECI_IDATA;
	/* 0x007: PECI Write Length */
	volatile uint8_t PECI_WR_LENGTH;
	volatile uint8_t reserved1;
	/* 0x009: PECI Configuration */
	volatile uint8_t PECI_CFG;
	volatile uint8_t reserved2;
	/* 0x00B: PECI Write FCS */
	volatile uint8_t PECI_WR_FCS;
	/* 0x00C: PECI Read FCS */
	volatile uint8_t PECI_RD_FCS;
	/* 0x00D: PECI Assured Write FCS */
	volatile uint8_t PECI_AW_FCS;
	/* 0X00E: PECI Version */
	volatile uint8_t PECI_VER;
	/* 0x00F: PECI Transfer Rate */
	volatile uint8_t PECI_RATE;
	/* 0x010 - 0x02A: PECI Data In/Out */
	union {
		volatile uint8_t PECI_DATA_IN[27];
		volatile uint8_t PECI_DATA_OUT[27];
	};
};

/* PECI register fields */
#define NPCM_PECI_CTL_STS_START_BUSY     0
#define NPCM_PECI_CTL_STS_DONE           1
#define NPCM_PECI_CTL_STS_CRC_ERR        3
#define NPCM_PECI_CTL_STS_ABRT_ERR       4
#define NPCM_PECI_CTL_STS_AWFCS_EB       5
#define NPCM_PECI_CTL_STS_DONE_EN        6
#define NPCM_PECI_RATE_MAX_BIT_RATE      FIELD(0, 5)
#define NPCM_PECI_RATE_MAX_BIT_RATE_MASK 0x1F
/* The minimal valid value of NPCM_PECI_RATE_MAX_BIT_RATE field */
#define PECI_MAX_BIT_RATE_VALID_MIN      0x05
#define PECI_HIGH_SPEED_MIN_VAL          0x07

#define NPCM_PECI_RATE_EHSP              6
#define NPCM_PECI_RATE_HLOAD             7

struct i3c_reg {
	/* 0x000: Master Configuration */
	volatile uint32_t MCONFIG;
	/* 0x004: Configuration */
	volatile uint32_t CONFIG;
	/* 0x008: Status */
	volatile uint32_t STATUS;
	/* 0x00C: I3C Control */
	volatile uint32_t CTRL;
	/* 0x010: Interrupt Enable Set */
	volatile uint32_t INTSET;
	/* 0x014: Interrupt Enable Clear */
	volatile uint32_t INTCLR;
	/* 0x018: Interrupt Masked */
	volatile uint32_t INTMASKED;
	/* 0x01C: Error and Warning */
	volatile uint32_t ERRWARN;
	/* 0x020: DMA Control */
	volatile uint32_t DMACTRL;
	volatile uint8_t  RESERVE0[8];
	/* 0x02C: Data Control */
	volatile uint32_t DATACTRL;
	/* 0x030: Write Byte Data */
	volatile uint32_t WDATAB;
	/* 0x034: Write Byte Data as End */
	volatile uint32_t WDATABE;
	/* 0x038: Write Half-Word Data */
	volatile uint32_t WDATAH;
	/* 0x03C: Write Half-Word Data as End */
	volatile uint32_t WDATAHE;
	/* 0x040: Read Byte Data */
	volatile uint32_t RDATAB;
	volatile uint8_t  RESERVE1[4];
	/* 0x048: Read Half-Word Data */
	volatile uint32_t RDATAH;
	volatile uint8_t  RESERVE2[8];
	/* 0x054: Byte-Only Write Byte Data */
	volatile uint8_t  WDATAB1;
	volatile uint8_t  RESERVE3[11];
	/* 0x060: Capabilities */
	volatile uint32_t CAPABILITIES;
	/* 0x064: Dynamic Address */
	volatile uint32_t DYNADDR;
	/* 0x068: Maximum Limits */
	volatile uint32_t MAXLIMITS;
	/* 0x06C: Part Number */
	volatile uint32_t PARTNO;
	/* 0x070: ID Extension */
	volatile uint32_t IDEXT;
	/* 0x074: Vendor ID */
	volatile uint32_t VENDORID;
	/* 0x078: Timing Control Clock */
	volatile uint32_t TCCLOCK;
	volatile uint8_t  RESERVE4[8];
	/* 0x084: Master Control */
	volatile uint32_t MCTRL;
	/* 0x088: Master Status */
	volatile uint32_t MSTATUS;
	/* 0x08C: IBI Registry and Rules */
	volatile uint32_t IBIRULES;
	/* 0x090: Master Interrupt Enable Set */
	volatile uint32_t MINTSET;
	/* 0x094: Master Interrupt Enable Clear */
	volatile uint32_t MINTCLR;
	/* 0x098: Master Interrupt Masked */
	volatile uint32_t MINTMASKED;
	/* 0x09C: Master Error and Warning */
	volatile uint32_t MERRWARN;
	/* 0x0A0: Master DMA Control */
	volatile uint32_t MDMACTRL;
	volatile uint8_t  RESERVE5[8];
	/* 0x0AC: Master Data Control */
	volatile uint32_t MDATACTRL;
	/* 0x0B0: Master Write Byte Data */
	volatile uint32_t MWDATAB;
	/* 0x0B4: Master Write Byte Data as End */
	volatile uint32_t MWDATABE;
	/* 0x0B8: Master Write Half-Word Data */
	volatile uint32_t MWDATAH;
	/* 0x0BC: Master Write Half-Word Data as End */
	volatile uint32_t MWDATAHE;
	/* 0x0C0: Master Read Byte Data */
	volatile uint32_t MRDATAB;
	volatile uint8_t RESERVE6[4];
	/* 0x0C8: Master Read Half-Word Data */
	volatile uint32_t MRDATAH;
	/* 0x0CC: Master Byte-Only Write Byte Data */
	volatile uint8_t MWDATAB1;
	volatile uint8_t RESERVE7[3];
	/* 0x0D0: Start or Continue SDR Message */
	union {
		volatile uint32_t MWMSG_SDR;
		volatile uint32_t MWMSG_SDR_CONTROL;
		volatile uint32_t MWMSG_SDR_DATA;
	};
	/* 0x0D4: Read SDR Message Data */
	volatile uint32_t MRMSG_SDR;
	/* 0x0D8: Start or Continue DDR Message */
	union {
		volatile uint32_t MWMSG_DDR;
		volatile uint32_t MWMSG_DDR_CONTROL;
		volatile uint32_t MWMSG_DDR_ADDRCMD;
		volatile uint32_t MWMSG_DDR_DATA;
	};
	/* 0x0DC: Read DDR Message Data */
	volatile uint32_t MRMSG_DDR;
	volatile uint8_t  RESERVE8[4];
	/* 00xE4: Master Dynamic Address */
	volatile uint32_t MDYNADDR;
	volatile uint8_t RESERVE9[32];
	/* 0x108: HDR Command Register */
	volatile uint32_t HDRCMD;
	volatile uint8_t RESERVE10[52];
	/* 0x140: Extended IBI Data Register 1 */
	volatile uint32_t IBIEXT1;
	/* 0x144: Extended IBI Data Register 2 */
	volatile uint32_t IBIEXT2;
	volatile uint8_t  RESERVE11[180];
	/* 0x1FC: Block ID */
	volatile uint32_t ID;
};

/* I3C register fields */
#define NPCM_I3C_MCONFIG_I2CBAUD FIELD(28, 31)
#define NPCM_I3C_MCONFIG_ODHPP	24
#define NPCM_I3C_MCONFIG_ODBAUD	FIELD(16, 23)
#define NPCM_I3C_MCONFIG_PPLOW	FIELD(12, 15)
#define NPCM_I3C_MCONFIG_PPBAUD	FIELD(8, 11)
#define NPCM_I3C_MCONFIG_ODSTOP	6
#define NPCM_I3C_MCONFIG_DISTO	3
#define NPCM_I3C_MCONFIG_MSTENA	FIELD(0, 1)
#define NPCM_I3C_CONFIG_SADDR		FIELD(25, 31)
#define NPCM_I3C_CONFIG_BAMATCH		FIELD(16, 22)
#define NPCM_I3C_CONFIG_HDRCMD		10
#define NPCM_I3C_CONFIG_OFFLINE		9
#define NPCM_I3C_CONFIG_IDRAND		8
#define NPCM_I3C_CONFIG_DDROK		4
#define NPCM_I3C_CONFIG_S0IGNORE		3
#define NPCM_I3C_CONFIG_MATCHSS		2
#define NPCM_I3C_CONFIG_NACK			1
#define NPCM_I3C_CONFIG_SLVENA		0
#define NPCM_I3C_STATUS_TIMECTRL			FIELD(30, 31)
#define NPCM_I3C_STATUS_ACTSTATE			FIELD(28, 29)
#define NPCM_I3C_STATUS_HJDIS			27
#define NPCM_I3C_STATUS_MRDIS			25
#define NPCM_I3C_STATUS_IBIDIS			24
#define NPCM_I3C_STATUS_EVDET			FIELD(20, 21)
#define NPCM_I3C_STATUS_EVENT			18
#define NPCM_I3C_STATUS_CHANDLED			17
#define NPCM_I3C_STATUS_DDRMATCH			16
#define NPCM_I3C_STATUS_ERRWARN			15
#define NPCM_I3C_STATUS_CCC				14
#define NPCM_I3C_STATUS_DACHG			13
#define NPCM_I3C_STATUS_TXNOTFULL		12
#define NPCM_I3C_STATUS_RXPEND			11
#define NPCM_I3C_STATUS_STOP				10
#define NPCM_I3C_STATUS_MATCHED			9
#define NPCM_I3C_STATUS_START			8
#define NPCM_I3C_STATUS_STHDR			6
#define NPCM_I3C_STATUS_STDAA			5
#define NPCM_I3C_STATUS_STREQWR			4
#define NPCM_I3C_STATUS_STREQRD			3
#define NPCM_I3C_STATUS_STCCCH			2
#define NPCM_I3C_STATUS_STMSG			1
#define NPCM_I3C_STATUS_STNOTSTOP		0
#define NPCM_I3C_CTRL_VENDINFO		FIELD(24, 31)
#define NPCM_I3C_CTRL_ACTSTATE		FIELD(20, 21)
#define NPCM_I3C_CTRL_PENDINT		FIELD(16, 19)
#define NPCM_I3C_CTRL_IBIDATA		FIELD(8, 15)
#define NPCM_I3C_CTRL_EXTDATA		3
#define NPCM_I3C_CTRL_EVENT			FIELD(0, 1)
#define NPCM_I3C_INTSET_EVENT			18
#define NPCM_I3C_INTSET_CHANDLED			17
#define NPCM_I3C_INTSET_DDRMATCHED		16
#define NPCM_I3C_INTSET_ERRWARN			15
#define NPCM_I3C_INTSET_CCC				14
#define NPCM_I3C_INTSET_DACHG			13
#define NPCM_I3C_INTSET_TXNOTFULL		12
#define NPCM_I3C_INTSET_RXPEND			11
#define NPCM_I3C_INTSET_STOP				10
#define NPCM_I3C_INTSET_MATCHED			9
#define NPCM_I3C_INTSET_START			8
#define NPCM_I3C_INTCLR_EVENT		18
#define NPCM_I3C_INTCLR_CHANDLED		17
#define NPCM_I3C_INTCLR_DDRMATCHED	16
#define NPCM_I3C_INTCLR_ERRWARN		15
#define NPCM_I3C_INTCLR_CCC			14
#define NPCM_I3C_INTCLR_DACHG		13
#define NPCM_I3C_INTCLR_TXNOTFULL	12
#define NPCM_I3C_INTCLR_RXPEND		11
#define NPCM_I3C_INTCLR_STOP			10
#define NPCM_I3C_INTCLR_MATCHED		9
#define NPCM_I3C_INTCLR_START		8
#define NPCM_I3C_INTMASKED_EVENT			18
#define NPCM_I3C_INTMASKED_CHANDLED		17
#define NPCM_I3C_INTMASKED_DDRMATCHED	16
#define NPCM_I3C_INTMASKED_ERRWARN		15
#define NPCM_I3C_INTMASKED_CCC			14
#define NPCM_I3C_INTMASKED_DACHG			13
#define NPCM_I3C_INTMASKED_TXNOTFULL		12
#define NPCM_I3C_INTMASKED_RXPEND		11
#define NPCM_I3C_INTMASKED_STOP			10
#define NPCM_I3C_INTMASKED_MATCHED		9
#define NPCM_I3C_INTMASKED_START			8
#define NPCM_I3C_ERRWARN_OWRITE		17
#define NPCM_I3C_ERRWARN_OREAD		16
#define NPCM_I3C_ERRWARN_S0S1		11
#define NPCM_I3C_ERRWARN_HCRC		10
#define NPCM_I3C_ERRWARN_HPAR		9
#define NPCM_I3C_ERRWARN_SPAR		8
#define NPCM_I3C_ERRWARN_INVSTART	4
#define NPCM_I3C_ERRWARN_TERM		3
#define NPCM_I3C_ERRWARN_URUNNACK	2
#define NPCM_I3C_ERRWARN_URUN		1
#define NPCM_I3C_ERRWARN_ORUN		0
#define NPCM_I3C_DMACTRL_DMAWIDTH		FIELD(4, 5)
#define NPCM_I3C_DMACTRL_DMATB			FIELD(2, 3)
#define NPCM_I3C_DMACTRL_DMAFB			FIELD(0, 1)
#define NPCM_I3C_DATACTRL_RXEMPTY	31
#define NPCM_I3C_DATACTRL_TXFULL		30
#define NPCM_I3C_DATACTRL_RXCOUNT	FIELD(24, 28)
#define NPCM_I3C_DATACTRL_TXCOUNT	FIELD(16, 20)
#define NPCM_I3C_DATACTRL_RXTRIG		FIELD(6, 7)
#define NPCM_I3C_DATACTRL_TXTRIG		FIELD(4, 5)
#define NPCM_I3C_DATACTRL_UNLOCK		3
#define NPCM_I3C_DATACTRL_FLUSHFB	1
#define NPCM_I3C_DATACTRL_FLUSHTB	0
#define NPCM_I3C_WDATAB_END_A			16
#define NPCM_I3C_WDATAB_END_B			8
#define NPCM_I3C_WDATAB_DATA				FIELD(0, 7)
#define NPCM_I3C_WDATABE_DATA		FIELD(0, 7)
#define NPCM_I3C_WDATAH_END		16
#define NPCM_I3C_WDATAH_DATA1	FIELD(8, 15)
#define NPCM_I3C_WDATAH_DATA0	FIELD(0, 7)
#define NPCM_I3C_WDATAHE_DATA1		FIELD(8, 15)
#define NPCM_I3C_WDATAHE_DATA0		FIELD(0, 7)
#define NPCM_I3C_RDATAB_DATA0	FIELD(0, 7)
#define NPCM_I3C_RDATAH_DATA1		FIELD(8, 15)
#define NPCM_I3C_RDATAH_DATA0		FIELD(0, 7)
#define NPCM_I3C_WDATAB1_DATA	FIELD(0, 7)
#define NPCM_I3C_CAPABILITIES_DMA		31
#define NPCM_I3C_CAPABILITIES_INT		30
#define NPCM_I3C_CAPABILITIES_FIFORX		FIELD(28, 29)
#define NPCM_I3C_CAPABILITIES_FIFOTX		FIELD(26, 27)
#define NPCM_I3C_CAPABILITIES_TIMECTRL	21
#define NPCM_I3C_CAPABILITIES_IBI_MR_HJ	FIELD(16, 20)
#define NPCM_I3C_CAPABILITIES_CCCHANDLE	FIELD(12, 15)
#define NPCM_I3C_CAPABILITIES_SADDR		FIELD(10, 11)
#define NPCM_I3C_CAPABILITIES_HDRSUPP	6
#define NPCM_I3C_CAPABILITIES_IDREG		FIELD(2, 5)
#define NPCM_I3C_CAPABILITIES_IDENA		FIELD(0, 1)
#define NPCM_I3C_DYNADDR_DADDR		FIELD(1, 7)
#define NPCM_I3C_DYNADDR_DAVALID		0
#define NPCM_I3C_MAXLIMITS_MAXWR	FIELD(16, 27)
#define NPCM_I3C_MAXLIMITS_MAXRD	FIELD(0, 11)
#define NPCM_I3C_PARTNO_PARTNO		FIELD(0, 31)
#define NPCM_I3C_IDEXT_BCR		FIELD(16, 23)
#define NPCM_I3C_IDEXT_DCR		FIELD(8, 15)
#define NPCM_I3C_VENDORID_VID		FIELD(0, 14)
#define NPCM_I3C_TCCLOCK_FREQ			FIELD(8, 15)
#define NPCM_I3C_TCCLOCK_ACCURACY		FIELD(0, 7)
#define NPCM_I3C_MCTRL_RDTERM	FIELD(16, 23)
#define NPCM_I3C_MCTRL_ADDR		FIELD(9, 15)
#define NPCM_I3C_MCTRL_DIR		8
#define NPCM_I3C_MCTRL_IBIRESP	FIELD(6, 7)
#define NPCM_I3C_MCTRL_TYPE		FIELD(4, 5)
#define NPCM_I3C_MCTRL_REQUEST	FIELD(0, 2)
#define NPCM_I3C_MSTATUS_IBIADDR		FIELD(24, 30)
#define NPCM_I3C_MSTATUS_NOWMASTER	19
#define NPCM_I3C_MSTATUS_ERRWARN		15
#define NPCM_I3C_MSTATUS_IBIWON		13
#define NPCM_I3C_MSTATUS_TXNOTFULL	12
#define NPCM_I3C_MSTATUS_RXPEND		11
#define NPCM_I3C_MSTATUS_COMPLETE	10
#define NPCM_I3C_MSTATUS_MCTRLDONE	9
#define NPCM_I3C_MSTATUS_SLVSTART	8
#define NPCM_I3C_MSTATUS_IBITYPE		FIELD(6, 7)
#define NPCM_I3C_MSTATUS_NACKED		5
#define NPCM_I3C_MSTATUS_BETWEEN		4
#define NPCM_I3C_MSTATUS_STATE		FIELD(0, 2)
#define NPCM_I3C_IBIRULES_NOBYTE	31
#define NPCM_I3C_IBIRULES_MSB0	30
#define NPCM_I3C_IBIRULES_ADDR4	FIELD(24, 29)
#define NPCM_I3C_IBIRULES_ADDR3	FIELD(18, 23)
#define NPCM_I3C_IBIRULES_ADDR2	FIELD(12, 17)
#define NPCM_I3C_IBIRULES_ADDR1	FIELD(6, 11)
#define NPCM_I3C_IBIRULES_ADDR0	FIELD(0, 5)
#define NPCM_I3C_MINTSET_NOWMASTER	19
#define NPCM_I3C_MINTSET_ERRWARN		15
#define NPCM_I3C_MINTSET_IBIWON		13
#define NPCM_I3C_MINTSET_TXNOTFULL	12
#define NPCM_I3C_MINTSET_RXPEND		11
#define NPCM_I3C_MINTSET_COMPLETE	10
#define NPCM_I3C_MINTSET_MCTRLDONE	9
#define NPCM_I3C_MINTSET_SLVSTART	8
#define NPCM_I3C_MINTCLR_NOWMASTER		19
#define NPCM_I3C_MINTCLR_ERRWARN			15
#define NPCM_I3C_MINTCLR_IBIWON			13
#define NPCM_I3C_MINTCLR_TXNOTFULL		12
#define NPCM_I3C_MINTCLR_RXPEND			11
#define NPCM_I3C_MINTCLR_COMPLETE		10
#define NPCM_I3C_MINTCLR_MCTRLDONE		9
#define NPCM_I3C_MINTCLR_SLVSTART		8
#define NPCM_I3C_MINTMASKED_NOWMASTER		19
#define NPCM_I3C_MINTMASKED_ERRWARN			15
#define NPCM_I3C_MINTMASKED_IBIWON			13
#define NPCM_I3C_MINTMASKED_TXNOTFULL		12
#define NPCM_I3C_MINTMASKED_RXPEND			11
#define NPCM_I3C_MINTMASKED_COMPLETE			10
#define NPCM_I3C_MINTMASKED_MCTRLDONE		9
#define NPCM_I3C_MINTMASKED_SLVSTART			8
#define NPCM_I3C_MERRWARN_TIMEOUT		20
#define NPCM_I3C_MERRWARN_INVREQ			19
#define NPCM_I3C_MERRWARN_MSGERR			18
#define NPCM_I3C_MERRWARN_OWRITE			17
#define NPCM_I3C_MERRWARN_OREAD			16
#define NPCM_I3C_MERRWARN_HCRC			10
#define NPCM_I3C_MERRWARN_HPAR			9
#define NPCM_I3C_MERRWARN_TERM			4
#define NPCM_I3C_MERRWARN_WRABT			3
#define NPCM_I3C_MERRWARN_NACK			2
#define NPCM_I3C_MDMACTRL_DMAWIDTH	FIELD(4, 5)
#define NPCM_I3C_MDMACTRL_DMATB		FIELD(2, 3)
#define NPCM_I3C_MDMACTRL_DMAFB		FIELD(0, 1)
#define NPCM_I3C_MDATACTRL_RXEMPTY		31
#define NPCM_I3C_MDATACTRL_TXFULL		30
#define NPCM_I3C_MDATACTRL_RXCOUNT		FIELD(24, 28)
#define NPCM_I3C_MDATACTRL_TXCOUNT		FIELD(16, 20)
#define NPCM_I3C_MDATACTRL_RXTRIG		FIELD(6, 7)
#define NPCM_I3C_MDATACTRL_TXTRIG		FIELD(4, 5)
#define NPCM_I3C_MDATACTRL_UNLOCK		3
#define NPCM_I3C_MDATACTRL_FLUSHFB		1
#define NPCM_I3C_MDATACTRL_FLUSHTB		0
#define NPCM_I3C_MWDATAB_END_A		16
#define NPCM_I3C_MWDATAB_END_B		8
#define NPCM_I3C_MWDATAB_DATA		FIELD(0, 7)
#define NPCM_I3C_MWDATABE_DATA	FIELD(0, 7)
#define NPCM_I3C_MWDATAH_END			16
#define NPCM_I3C_MWDATAH_DATA1		FIELD(8, 15)
#define NPCM_I3C_MWDATAH_DATA0		FIELD(0, 7)
#define NPCM_I3C_MWDATAHE_DATA1			FIELD(8, 15)
#define NPCM_I3C_MWDATAHE_DATA0			FIELD(0, 7)
#define NPCM_I3C_MRDATAB_DATA		FIELD(0, 7)
#define NPCM_I3C_MRDATAH_DATA1	FIELD(8, 15)
#define NPCM_I3C_MRDATAH_DATA0	FIELD(0, 7)
#define NPCM_I3C_MWDATAB1_DATA		FIELD(0, 7)
#define NPCM_I3C_MWMSG_SDR_CONTROL_LEN	FIELD(11, 15)
#define NPCM_I3C_MWMSG_SDR_CONTROL_I2C	10
#define NPCM_I3C_MWMSG_SDR_CONTROL_END	8
#define NPCM_I3C_MWMSG_SDR_CONTROL_ADDR	FIELD(1, 7)
#define NPCM_I3C_MWMSG_SDR_CONTROL_DIR	0
#define NPCM_I3C_MWMSG_SDR_DATA_DATA16B		FIELD(0, 15)
#define NPCM_I3C_MRMSG_SDR_DATA			FIELD(0, 15)
#define NPCM_I3C_MWMSG_DDR_CONTROL_END		14
#define NPCM_I3C_MWMSG_DDR_CONTROL_LEN		FIELD(0, 9)
#define NPCM_I3C_MWMSG_DDR_CONTROL_ADDR	FIELD(9, 15)
#define NPCM_I3C_MWMSG_DDR_CONTROL_DIR	7
#define NPCM_I3C_MWMSG_DDR_CONTROL_CMD	FIELD(0, 6)
#define NPCM_I3C_MWMSG_DDR_DATA_DATA16B		FIELD(0, 15)
#define NPCM_I3C_MRMSG_DDR_DATA			FIELD(0, 15)
#define NPCM_I3C_MDYNADDR_DADDR		FIELD(1, 7)
#define NPCM_I3C_MDYNADDR_DAVALID	0
#define NPCM_I3C_HDRCMD_NEWCMD	31
#define NPCM_I3C_HDRCMD_OVFLW	30
#define NPCM_I3C_HDRCMD_CMD0		FIELD(0, 7)
#define NPCM_I3C_IBIEXT1_EXT3		FIELD(24, 31)
#define NPCM_I3C_IBIEXT1_EXT2		FIELD(16, 23)
#define NPCM_I3C_IBIEXT1_EXT1		FIELD(8, 15)
#define NPCM_I3C_IBIEXT1_MAX			FIELD(4, 6)
#define NPCM_I3C_IBIEXT1_CNT			FIELD(0, 2)
#define NPCM_I3C_IBIEXT2_EXT7			FIELD(24, 31)
#define NPCM_I3C_IBIEXT2_EXT6			FIELD(16, 23)
#define NPCM_I3C_IBIEXT2_EXT5			FIELD(8, 15)
#define NPCM_I3C_IBIEXT2_EXT4			FIELD(0, 7)
#define NPCM_I3C_SID_ID				FIELD(0, 31)

struct dsct_reg {
	__IO uint32_t CTL;
	__IO uint32_t ENDSA;
	__IO uint32_t ENDDA;
	__IO uint32_t NEXT;
};

struct pdma_reg {
	__IO struct dsct_reg DSCT[16];

	__I  uint32_t RESERVE0[192];

	__IO uint32_t CHCTL;

	__O  uint32_t STOP;

	__O  uint32_t SWREQ;

	__I  uint32_t TRGSTS;

	__IO uint32_t PRISET;

	__O  uint32_t PRICLR;

	__IO uint32_t INTEN;

	__IO uint32_t INTSTS;

	__IO uint32_t ABTSTS;

	__IO uint32_t TDSTS;

	__IO uint32_t SCATSTS;

	__I  uint32_t TACTSTS;

	__I  uint32_t RESERVE1[3];

	__IO uint32_t SCATBA;

	__IO uint32_t TOC0_1;

	__IO uint32_t TOC2_3;

	__IO uint32_t TOC4_5;

	__IO uint32_t TOC6_7;

	__IO uint32_t TOC8_9;

	__IO uint32_t TOC10_11;

	__IO uint32_t TOC12_13;

	__IO uint32_t TOC14_15;

	__I  uint32_t RESERVE2[8];

	__IO uint32_t REQSEL0_3;

	__IO uint32_t REQSEL4_7;

	__IO uint32_t REQSEL8_11;

	__IO uint32_t REQSEL12_15;
};

/*
 * USB device controller (USBD) device registers
 */
struct usbd_ep_reg {
	union {
	    /* 0x64+x*0x28:  Endpoint x Data Register */
		volatile uint32_t USBD_EPDAT;
	    /* 0x64+x*0x28:  Endpoint x Data Register for Byte Access */
		volatile uint8_t  USBD_EPDAT_BYTE;
	};
	/* 0x68+x*0x28:  Endpoint x Interrupt Status Register */
	volatile uint32_t USBD_EPINTSTS;
	/* 0x6C+x*0x28:  USBD_Endpoint x Interrupt Enable Register */
	volatile uint32_t USBD_EPINTEN;
	/* 0x70+x*0x28:  Endpoint x Data Available Count Register */
	volatile  uint32_t USBD_EPDATCNT;
	/* 0x74+x*0x28:  Endpoint x Response Control Register */
	volatile uint32_t USBD_EPRSPCTL;
	/* 0x78+x*0x28:  Endpoint x Maximum Packet Size Register */
	volatile uint32_t USBD_EPMPS;
	/* 0x7C+x*0x28:  Endpoint x Transfer Count Register */
	volatile uint32_t USBD_EPTXCNT;
	/* 0x80+x*0x28:  Endpoint x Configuration Register */
	volatile uint32_t USBD_EPCFG;
	/* 0x84+x*0x28:  Endpoint x RAM Start Address Register */
	volatile uint32_t USBD_EPBUFSTART;
	/* 0x88+x*0x28:  Endpoint x RAM End Address Register */
	volatile uint32_t USBD_EPBUFEND;
};

struct usbd_reg {
	/* 0x00:  Interrupt Status Low Register */
	volatile  uint32_t USBD_GINTSTS;
	uint32_t RESERVE0[1];
	/* 0x08:  Interrupt Enable Low Register */
	volatile uint32_t USBD_GINTEN;
	uint32_t RESERVE1[1];
	/* 0x10:  USB Bus Interrupt Status Register */
	volatile uint32_t USBD_BUSINTSTS;
	/* 0x14:  USB Bus Interrupt Enable Register */
	volatile uint32_t USBD_BUSINTEN;
	/* 0x18:  USB Operational Register */
	volatile uint32_t USBD_OPER;
	/* 0x1C:  USB Frame Count Register */
	volatile  uint32_t USBD_FRAMECNT;
	/* 0x20:  USB Function Address Register */
	volatile uint32_t USBD_FADDR;
	/* 0x24:  USB Test Mode Register */
	volatile uint32_t USBD_TEST;
	union {
		/* 0x28:  Control-Endpoint Data Buffer */
		volatile uint32_t USBD_CEPDAT;
		/* 0x28:  Control-Endpoint Data Buffer for Byte Access */
		volatile uint8_t  USBD_CEPDAT_BYTE;
	};
	/* 0x2C:  Control-Endpoint Control and Status */
	volatile uint32_t USBD_CEPCTL;
	/* 0x30:  Control-Endpoint Interrupt Enable */
	volatile uint32_t USBD_CEPINTEN;
	/* 0x34:  Control-Endpoint Interrupt Status */
	volatile uint32_t USBD_CEPINTSTS;
	/* 0x38:  Control-Endpoint In-transfer Data Count */
	volatile uint32_t USBD_CEPTXCNT;
	/* 0x3C:  Control-Endpoint Out-transfer Data Count */
	volatile  uint32_t USBD_CEPRXCNT;
	/* 0x40:  Control-Endpoint data count */
	volatile  uint32_t USBD_CEPDATCNT;
	/* 0x44:  Setup1 & Setup0 bytes */
	volatile  uint32_t USBD_SETUP1_0;
	/* 0x48:  Setup3 & Setup2 Bytes */
	volatile  uint32_t USBD_SETUP3_2;
	/* 0x4C:  Setup5 & Setup4 Bytes */
	volatile  uint32_t USBD_SETUP5_4;
	/* 0x50:  Setup7 & Setup6 Bytes */
	volatile  uint32_t USBD_SETUP7_6;
	/* 0x54:  Control Endpoint RAM Start Address Register */
	volatile uint32_t USBD_CEPBUFSTART;
	/* 0x58:  Control Endpoint RAM End Address Register */
	volatile uint32_t USBD_CEPBUFEND;
	/* 0x5C:  DMA Control Status Register */
	volatile uint32_t USBD_DMACTL;
	/* 0x60:  DMA Count Register */
	volatile uint32_t USBD_DMACNT;

	/* 0x64-0x240:  Endpoint Register */
	struct usbd_ep_reg EP[12];

	uint32_t RESERVE2[303];
	/* 0x700:  AHB DMA Address Register*/
	volatile uint32_t USBD_DMAADDR;
	/* 0x704:  USB PHY Control Register */
	volatile uint32_t USBD_PHYCTL;
};

/* USBD register fields */
#define NPCM_USBD_GINTSTS_USBIF           (0)
#define NPCM_USBD_GINTSTS_CEPIF           (1)
#define NPCM_USBD_GINTSTS_EPAIF           (2)
#define NPCM_USBD_GINTSTS_EPBIF           (3)
#define NPCM_USBD_GINTSTS_EPCIF           (4)
#define NPCM_USBD_GINTSTS_EPDIF           (5)
#define NPCM_USBD_GINTSTS_EPEIF           (6)
#define NPCM_USBD_GINTSTS_EPFIF           (7)
#define NPCM_USBD_GINTSTS_EPGIF           (8)
#define NPCM_USBD_GINTSTS_EPHIF           (9)
#define NPCM_USBD_GINTSTS_EPIIF           (10)
#define NPCM_USBD_GINTSTS_EPJIF           (11)
#define NPCM_USBD_GINTSTS_EPKIF           (12)
#define NPCM_USBD_GINTSTS_EPLIF           (13)

#define NPCM_USBD_GINTEN_USBIEN           (0)
#define NPCM_USBD_GINTEN_CEPIEN           (1)
#define NPCM_USBD_GINTEN_EPAIEN           (2)
#define NPCM_USBD_GINTEN_EPBIEN           (3)
#define NPCM_USBD_GINTEN_EPCIEN           (4)
#define NPCM_USBD_GINTEN_EPDIEN           (5)
#define NPCM_USBD_GINTEN_EPEIEN           (6)
#define NPCM_USBD_GINTEN_EPFIEN           (7)
#define NPCM_USBD_GINTEN_EPGIEN           (8)
#define NPCM_USBD_GINTEN_EPHIEN           (9)
#define NPCM_USBD_GINTEN_EPIIEN           (10)
#define NPCM_USBD_GINTEN_EPJIEN           (11)
#define NPCM_USBD_GINTEN_EPKIEN           (12)
#define NPCM_USBD_GINTEN_EPLIEN           (13)

#define NPCM_USBD_BUSINTSTS_SOFIF         (0)
#define NPCM_USBD_BUSINTSTS_RSTIF         (1)
#define NPCM_USBD_BUSINTSTS_RESUMEIF      (2)
#define NPCM_USBD_BUSINTSTS_SUSPENDIF     (3)
#define NPCM_USBD_BUSINTSTS_HISPDIF       (4)
#define NPCM_USBD_BUSINTSTS_DMADONEIF     (5)
#define NPCM_USBD_BUSINTSTS_PHYCLKVLDIF   (6)
#define NPCM_USBD_BUSINTSTS_VBUSDETIF     (8)

#define NPCM_USBD_BUSINTEN_SOFIEN         (0)
#define NPCM_USBD_BUSINTEN_RSTIEN         (1)
#define NPCM_USBD_BUSINTEN_RESUMEIEN      (2)
#define NPCM_USBD_BUSINTEN_SUSPENDIEN     (3)
#define NPCM_USBD_BUSINTEN_HISPDIEN       (4)
#define NPCM_USBD_BUSINTEN_DMADONEIEN     (5)
#define NPCM_USBD_BUSINTEN_PHYCLKVLDIEN   (6)
#define NPCM_USBD_BUSINTEN_VBUSDETIEN     (8)

#define NPCM_USBD_OPER_RESUMEEN           (0)
#define NPCM_USBD_OPER_HISPDEN            (1)
#define NPCM_USBD_OPER_CURSPD             (2)

#define NPCM_USBD_FRAMECNT_MFRAMECNT      FIELD(0, 3)
#define NPCM_USBD_FRAMECNT_FRAMECNT       FIELD(3, 11)

#define NPCM_USBD_FADDR_FADDR             FILED(0, 7)

#define NPCM_USBD_CEPDAT_DAT              FILED(0, 32)

#define NPCM_USBD_CEPCTL_NAKCLR           (0)
#define NPCM_USBD_CEPCTL_STALLEN          (1)
#define NPCM_USBD_CEPCTL_ZEROLEN          (2)
#define NPCM_USBD_CEPCTL_FLUSH            (3)

#define NPCM_USBD_CEPINTEN_SETUPTKIEN     (0)
#define NPCM_USBD_CEPINTEN_SETUPPKIEN     (1)
#define NPCM_USBD_CEPINTEN_OUTTKIEN       (2)
#define NPCM_USBD_CEPINTEN_INTKIEN        (3)
#define NPCM_USBD_CEPINTEN_PINGIEN        (4)
#define NPCM_USBD_CEPINTEN_TXPKIEN        (5)
#define NPCM_USBD_CEPINTEN_RXPKIEN        (6)
#define NPCM_USBD_CEPINTEN_NAKIEN         (7)
#define NPCM_USBD_CEPINTEN_STALLIEN       (8)
#define NPCM_USBD_CEPINTEN_ERRIEN         (9)
#define NPCM_USBD_CEPINTEN_STSDONEIEN     (10)
#define NPCM_USBD_CEPINTEN_BUFFULLIEN     (11)
#define NPCM_USBD_CEPINTEN_BUFEMPTYIEN    (12)

#define NPCM_USBD_CEPINTSTS_SETUPTKIF     (0)
#define NPCM_USBD_CEPINTSTS_SETUPPKIF     (1)
#define NPCM_USBD_CEPINTSTS_OUTTKIF       (2)
#define NPCM_USBD_CEPINTSTS_INTKIF        (3)
#define NPCM_USBD_CEPINTSTS_PINGIF        (4)
#define NPCM_USBD_CEPINTSTS_TXPKIF        (5)
#define NPCM_USBD_CEPINTSTS_RXPKIF        (6)
#define NPCM_USBD_CEPINTSTS_NAKIF         (7)
#define NPCM_USBD_CEPINTSTS_STALLIF       (8)
#define NPCM_USBD_CEPINTSTS_ERRIF         (9)
#define NPCM_USBD_CEPINTSTS_STSDONEIF     (10)
#define NPCM_USBD_CEPINTSTS_BUFFULLIF     (11)
#define NPCM_USBD_CEPINTSTS_BUFEMPTYIF    (12)

#define NPCM_USBD_CEPTXCNT_TXCNT          FIELD(0, 8)

#define NPCM_USBD_CEPRXCNT_RXCNT          FIELD(0, 8)

#define NPCM_USBD_CEPDATCNT_DATCNT        FIELD(0, 16)

#define NPCM_USBD_SETUP1_0_SETUP0         FIELD(0, 8)
#define NPCM_USBD_SETUP1_0_SETUP1         FIELD(8, 8)

#define NPCM_USBD_SETUP3_2_SETUP2         FIELD(0, 8)
#define NPCM_USBD_SETUP3_2_SETUP3         FIELD(8, 8)

#define NPCM_USBD_SETUP5_4_SETUP4         FIELD(0, 8)
#define NPCM_USBD_SETUP5_4_SETUP5         FIELD(8, 8)

#define NPCM_USBD_SETUP7_6_SETUP6         FIELD(0, 8)
#define NPCM_USBD_SETUP7_6_SETUP7         FIELD(8, 8)

#define NPCM_USBD_CEPBUFSTART_SADDR       FIELD(0, 12)

#define NPCM_USBD_CEPBUFEND_EADDR         FIELD(0, 12)

#define NPCM_USBD_DMACTL_EPNUM            FIELD(0, 4)
#define NPCM_USBD_DMACTL_DMARD            (4)
#define NPCM_USBD_DMACTL_DMAEN            (5)
#define NPCM_USBD_DMACTL_SGEN             (6)
#define NPCM_USBD_DMACTL_DMARST           (7)
#define NPCM_USBD_DMACTL_SVINEP           (8)

#define NPCM_USBD_DMACNT_DMACNT           FIELD(0, 20)

#define NPCM_USBD_EPDAT_EPDAT             FIELD(0, 32)

#define NPCM_USBD_EPINTSTS_BUFFULLIF      (0)
#define NPCM_USBD_EPINTSTS_BUFEMPTYIF     (1)
#define NPCM_USBD_EPINTSTS_SHORTTXIF      (2)
#define NPCM_USBD_EPINTSTS_TXPKIF         (3)
#define NPCM_USBD_EPINTSTS_RXPKIF         (4)
#define NPCM_USBD_EPINTSTS_OUTTKIF        (5)
#define NPCM_USBD_EPINTSTS_INTKIF         (6)
#define NPCM_USBD_EPINTSTS_PINGIF         (7)
#define NPCM_USBD_EPINTSTS_NAKIF          (8)
#define NPCM_USBD_EPINTSTS_STALLIF        (9)
#define NPCM_USBD_EPINTSTS_NYETIF         (10)
#define NPCM_USBD_EPINTSTS_ERRIF          (11)
#define NPCM_USBD_EPINTSTS_SHORTRXIF      (12)

#define NPCM_USBD_EPINTEN_BUFFULLIEN      (0)
#define NPCM_USBD_EPINTEN_BUFEMPTYIEN     (1)
#define NPCM_USBD_EPINTEN_SHORTTXIEN      (2)
#define NPCM_USBD_EPINTEN_TXPKIEN         (3)
#define NPCM_USBD_EPINTEN_RXPKIEN         (4)
#define NPCM_USBD_EPINTEN_OUTTKIEN        (5)
#define NPCM_USBD_EPINTEN_INTKIEN         (6)
#define NPCM_USBD_EPINTEN_PINGIEN         (7)
#define NPCM_USBD_EPINTEN_NAKIEN          (8)
#define NPCM_USBD_EPINTEN_STALLIEN        (9)
#define NPCM_USBD_EPINTEN_NYETIEN         (10)
#define NPCM_USBD_EPINTEN_ERRIEN          (11)
#define NPCM_USBD_EPINTEN_SHORTRXIEN      (12)

#define NPCM_USBD_EPDATCNT_DATCNT         FIELD(0, 16)
#define NPCM_USBD_EPDATCNT_DMALOOP        FIELD(16, 15)

#define NPCM_USBD_EPRSPCTL_FLUSH          (0)
#define NPCM_USBD_EPRSPCTL_MODE           FIELD(1, 2)
#define NPCM_USBD_EPRSPCTL_TOGGLE         (3)
#define NPCM_USBD_EPRSPCTL_HALT           (4)
#define NPCM_USBD_EPRSPCTL_ZEROLEN        (5)
#define NPCM_USBD_EPRSPCTL_SHORTTXEN      (6)
#define NPCM_USBD_EPRSPCTL_DISBUF         (7)

#define NPCM_USBD_EPMPS_EPMPS             FIELD(0, 11)

#define NPCM_USBD_EPTXCNT_TXCNT           FIELD(0, 11)

#define NPCM_USBD_EPCFG_EPEN              (0)
#define NPCM_USBD_EPCFG_EPTYPE            FIELD(1, 2)
#define NPCM_USBD_EPCFG_EPDIR             (3)
#define NPCM_USBD_EPCFG_EPNUM             FIELD(4, 4)

#define NPCM_USBD_EPBUFSTART_SADDR        FIELD(0, 12)

#define NPCM_USBD_EPBUFEND_EADDR          FIELD(0, 12)

#define NPCM_USBD_DMAADDR_DMAADDR         FIELD(0, 32)

#define NPCM_USBD_PHYCTL_DPPUEN           (8)
#define NPCM_USBD_PHYCTL_PHYEN            (9)
#define NPCM_USBD_PHYCTL_WKEN             (24)
#define NPCM_USBD_PHYCTL_VBUSDET          (31)

/*
 * SERIAL I/O EXPANSION INTERFACE (SIOX) device registers
 */
struct sgpio_reg {
	/* SIOn (n=1,2) */
	/* 0x00: I/O Expansion Data Out n Register 0 */
	volatile uint8_t XDOUT0;
	/* 0x01: I/O Expansion Data Out n Register 1 */
	volatile uint8_t XDOUT1;
	/* 0x02: I/O Expansion Data Out n Register 2 */
	volatile uint8_t XDOUT2;
	/* 0x03: I/O Expansion Data Out n Register 3 */
	volatile uint8_t XDOUT3;
	/* 0x04: I/O Expansion Data Out n Register 4 */
	volatile uint8_t XDOUT4;
	/* 0x05: I/O Expansion Data Out n Register 5 */
	volatile uint8_t XDOUT5;
	/* 0x06: I/O Expansion Data Out n Register 6 */
	volatile uint8_t XDOUT6;
	/* 0x07: I/O Expansion Data Out n Register 7 */
	volatile uint8_t XDOUT7;
	/* 0x08: I/O Expansion Data In n Register 0 */
	volatile uint8_t XDIN0;
	/* 0x09: I/O Expansion Data In n Register 1 */
	volatile uint8_t XDIN1;
	/* 0x0A: I/O Expansion Data In n Register 2 */
	volatile uint8_t XDIN2;
	/* 0x0B: I/O Expansion Data In n Register 3 */
	volatile uint8_t XDIN3;
	/* 0x0C: I/O Expansion Data In n Register 4 */
	volatile uint8_t XDIN4;
	/* 0x0D: I/O Expansion Data In n Register 5 */
	volatile uint8_t XDIN5;
	/* 0x0E: I/O Expansion Data In n Register 6 */
	volatile uint8_t XDIN6;
	/* 0x0F: I/O Expansion Data In n Register 7 */
	volatile uint8_t XDIN7;
	/* 0x10: I/O Expansion Event Configuration n Register 0 */
	volatile uint16_t XEVCFG0;
	/* 0x12: I/O Expansion Event Configuration n Register 1 */
	volatile uint16_t XEVCFG1;
	/* 0x14: I/O Expansion Event Configuration n Register 2 */
	volatile uint16_t XEVCFG2;
	/* 0x16: I/O Expansion Event Configuration n Register 3 */
	volatile uint16_t XEVCFG3;
	/* 0x18: I/O Expansion Event Configuration n Register 4 */
	volatile uint16_t XEVCFG4;
	/* 0x1A: I/O Expansion Event Configuration n Register 5 */
	volatile uint16_t XEVCFG5;
	/* 0x1C: I/O Expansion Event Configuration n Register 6 */
	volatile uint16_t XEVCFG6;
	/* 0x1E: I/O Expansion Event Configuration n Register 7 */
	volatile uint16_t XEVCFG7;
	/* 0x20: I/O Expansion Event Status n Register 0 */
	volatile uint8_t XEVSTS0;
	/* 0x21: I/O Expansion Event Status n Register 1 */
	volatile uint8_t XEVSTS1;
	/* 0x22: I/O Expansion Event Status n Register 2 */
	volatile uint8_t XEVSTS2;
	/* 0x23: I/O Expansion Event Status n Register 3 */
	volatile uint8_t XEVSTS3;
	/* 0x24: I/O Expansion Event Status n Register 4 */
	volatile uint8_t XEVSTS4;
	/* 0x25: I/O Expansion Event Status n Register 5 */
	volatile uint8_t XEVSTS5;
	/* 0x26: I/O Expansion Event Status n Register 6 */
	volatile uint8_t XEVSTS6;
	/* 0x27: I/O Expansion Event Status n Register 7 */
	volatile uint8_t XEVSTS7;
	/* 0x28: I/O Expansion Control and Status Register */
	volatile uint8_t IOXCTS;
	volatile uint8_t reserved1;
	/* 0x2A: I/O Expansion Configuration 1 Register */
	volatile uint8_t IOXCFG1;
	/* 0x2B: I/O Expansion Configuration 2 Register */
	volatile uint8_t IOXCFG2;
};

#define NPCM_SGPIO_PORT_PIN_NUM	  8U
#define MAX_NR_HW_SGPIO                   64
#define NPCM_IOXCTS_RD_MODE_MASK       0x6
#define NPCM_IOXCTS_RD_MODE_CONTINUOUS BIT(2)
#define NPCM_IOXCTS_IOXIF_EN           BIT(7)
#define NPCM_IOXCFG1_SFT_CLK_MASK      0xF

#define NPCM_XDOUT_OFFSET(n)           (0x00 + (n))
#define NPCM_XDIN_OFFSET(n)            (0x08 + (n))
#define NPCM_XEVCFG_OFFSET(n)          (0x10 + ((n) * 2L))
#define NPCM_XEVSTS_OFFSET(n)          (0x20 + (n))

#define NPCM_XDOUT(base, n)  (*(volatile uint8_t*)(base + \
						NPCM_XDOUT_OFFSET(n)))
#define NPCM_XDIN(base, n)   (*(volatile uint8_t*)(base + \
						NPCM_XDIN_OFFSET(n)))
#define NPCM_XEVCFG(base, n) (*(volatile uint16_t*)(base + \
						NPCM_XEVCFG_OFFSET(n)))
#define NPCM_XEVSTS(base, n) (*(volatile uint8_t*)(base + \
						NPCM_XEVSTS_OFFSET(n)))

#endif /* _NUVOTON_NPCM_REG_DEF_H */
