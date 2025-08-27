/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NCT_SOC_REG_DEF_H
#define _NUVOTON_NCT_SOC_REG_DEF_H

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
	volatile uint8_t reserved2;
	/* 0x007: Power-Down Control 0 */
	volatile uint8_t PWDWN_CTL0;
	/* 0x008: Power-Down Control 1 */
	volatile uint8_t PWDWN_CTL1;
	/* 0x009: Power-Down Control 2 */
	volatile uint8_t PWDWN_CTL2;
	/* 0x00A: Power-Down Control 3 */
	volatile uint8_t PWDWN_CTL3;
	/* 0x00B: Power-Down Control 4 */
	volatile uint8_t PWDWN_CTL4;
	/* 0x00C: Power-Down Control 5 */
	volatile uint8_t PWDWN_CTL5;
	/* 0x00D: Power-Down Control 6 */
	volatile uint8_t PWDWN_CTL6;
	volatile uint8_t reserved3[3];
	/* 0x011: RAM Power-Down Control 1 */
	volatile uint8_t RAM_PD1;
	/* 0x012: RAM Power-Down Control 2 */
	volatile uint8_t RAM_PD2;
	/* 0x013: Software Reset 1 */
	volatile uint8_t SW_RST1;
	/* 0x014: RAM Power-Down Control 3  */
	volatile uint8_t RAM_PD3;
	/* 0x015: Power-Down Control 7 */
	volatile uint8_t PWDWN_CTL7;
	/* 0x016: Power-Down Control 8 */
	volatile uint8_t PWDWN_CTL8;
};

/* PMC register fields */
#define NCT_PMCSR_DI_INSTW                   0
#define NCT_PMCSR_DHF                        1
#define NCT_PMCSR_IDLE                       2
#define NCT_PMCSR_NWBI                       3
#define NCT_PMCSR_OHFC                       6
#define NCT_PMCSR_OLFC                       7
#define NCT_DISIDL_CTL_RAM_DID               5
#define NCT_ENIDL_CTL_LP_WK_CTL              6
#define NCT_ENIDL_CTL_PECI_ENI               2

/* Macro functions for Development and Debugger Interface (DDI) registers */
#define NCT_DBGCTRL(base)   (*(volatile uint8_t *)(base + 0x022))
#define NCT_DBGFRZEN1(base) (*(volatile uint8_t *)(base + 0x076))
#define NCT_DBGFRZEN2(base) (*(volatile uint8_t *)(base + 0x077))
#define NCT_DBGFRZEN3(base) (*(volatile uint8_t *)(base + 0x078))
#define NCT_DBGFRZEN4(base) (*(volatile uint8_t *)(base + 0x079))
#define NCT_DBGFRZEN5(base) (*(volatile uint8_t *)(base + 0x07A))

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
	volatile uint8_t reserved1;
	/* 0x004: Device Control 3 */
	volatile uint8_t DEV_CTL3;
	volatile uint8_t reserved2;
	/* 0x006: Device Control 4 */
	volatile uint8_t DEV_CTL4;
	volatile uint8_t reserved3[4];
	volatile uint8_t DEVALT10;
	volatile uint8_t DEVALT11;
	volatile uint8_t DEVALT12;
	volatile uint8_t reserved4[2];
	/* 0x010 - 1F: Device Alternate Function 0 - F */
	volatile uint8_t DEVALT0[16];
	volatile uint8_t reserved5[4];
	/* 0x024: DEVALTCX */
	volatile uint8_t DEVALTCX;
	volatile uint8_t reserved6[3];
	/* 0x028: Device Pull-Up Enable 0 */
	volatile uint8_t DEVPU0;
	/* 0x029: Device Pull-Down Enable 1 */
	volatile uint8_t DEVPD1;
	/* 0x02A: Low-Voltage Pins Control 0 */
	volatile uint8_t LV_CTL0;
	/* 0x02B: Low-Voltage Pins Control 1 */
	volatile uint8_t LV_CTL1;
};

/* SCFG multi-registers */

/* Macro functions for SCFG multi-registers */
#define NCT_DEV_CTL(base, n) \
        (*(volatile uint8_t *)(base + n))
#define NCT_PUPD_EN(base, n) \
        (*(volatile uint8_t *)(base + NCT_PUPD_EN_OFFSET(n)))

#define NCT_DEVALT(base, n) (*(volatile uint8_t *)(base + n))
#define NCT_DEVALT6A_OFFSET  0x5A

/* SCFG register fields */
#define NCT_DEVCNT_F_SPI_TRIS                6
#define NCT_DEVCNT_HIF_TYP_SEL_FIELD         FIELD(2, 2)
#define NCT_DEVCNT_JEN1_HEN                  5
#define NCT_DEVCNT_JEN0_HEN                  4
#define NCT_STRPST_TRIST                     1
#define NCT_STRPST_TEST                      2
#define NCT_STRPST_JEN1                      4
#define NCT_STRPST_JEN0                      5
#define NCT_STRPST_SPI_COMP                  7
#define NCT_RSTCTL_VCC1_RST_STS              0
#define NCT_RSTCTL_DBGRST_STS                1
#define NCT_RSTCTL_VCC1_RST_SCRATCH          3
#define NCT_RSTCTL_LRESET_PLTRST_MODE        5
#define NCT_RSTCTL_HIPRST_MODE               6
#define NCT_DEV_CTL3_WP_INT_FL               0
#define NCT_DEV_CTL3_WP_GPIO55               1
#define NCT_DEV_CTL3_WP_GPIO76               2
#define NCT_DEV_CTL4_AMD_EN                  2
#define NCT_DEV_CTL4_PWROK_WD_EVENT          4
#define NCT_DEV_CTL4_ESPI_RSTO_EN            5
#define NCT_DEV_CTL4_USB_PWRDN               7
#define NCT_DEVPU0_I2C0_0_PUE                0
#define NCT_DEVPU0_I2C0_1_PUE                1
#define NCT_DEVPU0_I2C1_0_PUE                2
#define NCT_DEVPU0_I2C2_0_PUE                4
#define NCT_DEVPU0_I2C3_0_PUE                6
#define NCT_DEVPU1_F_SPI_PUD_EN              7
#define NCT_DEVALT10_CRGPIO_SELECT_SL_CORE   0
#define NCT_DEVALT10_CRGPIO_SELECT_SL_POWER  1
#define NCT_DEVALTCX_GPIO_PULL_EN            7

#define NCT_DEVALT6A_SIOX1_PU_EN             2
#define NCT_DEVALT6A_SIOX2_PU_EN             3

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
#define NCT_UICTRL_TBE                       0
#define NCT_UICTRL_RBF                       1
#define NCT_UICTRL_ETI                       5
#define NCT_UICTRL_ERI                       6
#define NCT_UICTRL_EEI                       7
#define NCT_USTAT_PE                         0
#define NCT_USTAT_FE                         1
#define NCT_USTAT_DOE                        2
#define NCT_USTAT_ERR                        3
#define NCT_USTAT_BKD                        4
#define NCT_USTAT_RB9                        5
#define NCT_USTAT_XMIP                       6
#define NCT_UFRS_CHAR_FIELD                  FIELD(0, 2)
#define NCT_UFRS_STP                         2
#define NCT_UFRS_XB9                         3
#define NCT_UFRS_PSEL_FIELD                  FIELD(4, 2)
#define NCT_UFRS_PEN                         6
#define NCT_UFCTRL_FIFOEN                    0
//#define NCT_UMDSL_FIFO_MD                    0
#define NCT_UTXFLV_TFL                       FIELD(0, 5)
#define NCT_URXFLV_RFL                       FIELD(0, 5)
//#define NCT_UFTSTS_TEMPTY_LVL_STS            5
//#define NCT_UFTSTS_TFIFO_EMPTY_STS           6
//#define NCT_UFTSTS_NXMIP                     7
//#define NCT_UFRSTS_RFULL_LVL_STS             5
//#define NCT_UFRSTS_RFIFO_NEMPTY_STS          6
//#define NCT_UFRSTS_ERR                       7
//#define NCT_UFTCTL_TEMPTY_LVL_SEL            FIELD(0, 5)
//#define NCT_UFTCTL_TEMPTY_LVL_EN             5
//#define NCT_UFTCTL_TEMPTY_EN                 6
//#define NCT_UFTCTL_NXMIPEN                   7
//#define NCT_UFRCTL_RFULL_LVL_SEL             FIELD(0, 5)
//#define NCT_UFRCTL_RFULL_LVL_EN              5
//#define NCT_UFRCTL_RNEMPTY_EN                6
//#define NCT_UFRCTL_ERR_EN                    7

/*
 * Multi-Input Wake-Up Unit (MIWU) device registers
 */

/* MIWU multi-registers */
#define NCT_WKEDG_OFFSET(n)    (0x000 + ((n) * 2L) + ((n) < 5 ? 0 : 0x1E))
#define NCT_WKAEDG_OFFSET(n)   (0x001 + ((n) * 2L) + ((n) < 5 ? 0 : 0x1E))
#define NCT_WKPND_OFFSET(n)    (0x00A + ((n) * 4L) + ((n) < 5 ? 0 : 0x10))
#define NCT_WKPCL_OFFSET(n)    (0x00C + ((n) * 4L) + ((n) < 5 ? 0 : 0x10))
#define NCT_WKEN_OFFSET(n)     (0x01E + ((n) * 2L) + ((n) < 5 ? 0 : 0x12))
#define NCT_WKINEN_OFFSET(n)   (0x01F + ((n) * 2L) + ((n) < 5 ? 0 : 0x12))
#define NCT_WKMOD_OFFSET(n)    (0x070 + (n))

#define NCT_WKEDG(base, n) (*(volatile uint8_t *)(base + \
						NCT_WKEDG_OFFSET(n)))
#define NCT_WKAEDG(base, n) (*(volatile uint8_t *)(base + \
						NCT_WKAEDG_OFFSET(n)))
#define NCT_WKPND(base, n) (*(volatile uint8_t *)(base + \
						NCT_WKPND_OFFSET(n)))
#define NCT_WKPCL(base, n) (*(volatile uint8_t *)(base + \
						NCT_WKPCL_OFFSET(n)))
#define NCT_WKEN(base, n) (*(volatile uint8_t *)(base + \
						NCT_WKEN_OFFSET(n)))
#define NCT_WKINEN(base, n) (*(volatile uint8_t *)(base + \
						NCT_WKINEN_OFFSET(n)))
#define NCT_WKMOD(base, n) (*(volatile uint8_t *)(base + \
						NCT_WKMOD_OFFSET(n)))

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
#define NCT_PWMCTL_INVP                      0
#define NCT_PWMCTL_CKSEL                     1
#define NCT_PWMCTL_HB_DC_CTL_FIELD           FIELD(2, 2)
#define NCT_PWMCTL_PWR                       7
#define NCT_PWMCTLEX_FCK_SEL_FIELD           FIELD(4, 2)
#define NCT_PWMCTLEX_OD_OUT                  7

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
#define NCT_CTRL0_VNT                         (5)
#define NCT_CTRL0_CH_SEL                      (0)
#define NCT_TM_T_MODE5                        (8)
#define NCT_TM_T_MODE4                        (6)
#define NCT_TM_T_MODE3                        (4)
#define NCT_TM_T_MODE2                        (2)
#define NCT_TM_T_MODE1                        (0)
#define NCT_TD_POST_OFFSET                    (8)
#define NCT_ACTRL1_PWCTRL                     (1)
#define NCT_ACTRL2_PD_VPP_PG                  (10)
#define NCT_ACTRL2_PD_PM                      (9)
#define NCT_ACTRL2_PD_ATX_5VSB                (8)
#define NCT_ACTRL2_PD_ANA                     (7)
#define NCT_ACTRL2_PD_BG                      (6)
#define NCT_ACTRL2_PD_DSM                     (5)
#define NCT_ACTRL2_PD_DVBE                    (4)
#define NCT_ACTRL2_PD_IREF                    (3)
#define NCT_ACTRL2_PD_ISEN                    (2)
#define NCT_ACTRL2_PD_RG                      (1)
#define NCT_CFG_IOVFEN                        (6)
#define NCT_CFG_ICEN                          (5)
#define NCT_CFG_START                         (4)
#define NCT_CS_CC5                            (5)
#define NCT_CS_CC4                            (4)
#define NCT_CS_CC3                            (3)
#define NCT_CS_CC2                            (2)
#define NCT_CS_CC1                            (1)
#define NCT_CS_CC0                            (0)
#define NCT_STS_OVFEV                         (1)
#define NCT_STS_EOCEV                         (0)
#define NCT_TCHNDATA_NEW                      (15)
#define NCT_TCHNDATA_DAT                      (3)
#define NCT_TCHNDATA_FRAC                     (0)
#define NCT_THRCTL_EN                         (15)
#define NCT_THRCTL_VAL                        (0)
#define NCT_THRDCTL_EN                        (15)

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
#define NCT_TWCFG_LTWD_CFG              (0)
#define NCT_TWCFG_LTWCP                 (1)
#define NCT_TWCFG_LTWDT0                (2)
#define NCT_TWCFG_LWDCNT                (3)
#define NCT_TWCFG_WDCT0I                (4)
#define NCT_TWCFG_WDSDME                (5)
#define NCT_TWCP_MDIV                   (0)
#define NCT_T0CSR_RST                   (0)
#define NCT_T0CSR_TC                    (1)
#define NCT_T0CSR_WDLTD                 (3)
#define NCT_T0CSR_WDRST_STS             (4)
#define NCT_T0CSR_WD_RUN                (5)
#define NCT_T0CSR_T0EN                  (6)
#define NCT_T0CSR_TESDIS                (7)
#define NCT_WDCP_WDIV                   (0)

/*
 * SPI PERIPHERAL INTERFACE (SPIP) device registers
 */
struct spip_reg {
	/* 0x00: SPI Control Register */
	volatile uint32_t SPIP_CTL;
	/* 0x04: SPI Clock Divider Register */
	volatile uint32_t SPIP_CLKDIV;
	/* 0x08: SPI Slave Select Control Register */
	volatile uint32_t SPIP_SSCTL;
	/* 0x0C: SPI PDMA Control Register */
	volatile uint32_t SPIP_PDMACTL;
	/* 0x10: SPI FIFO Control Register */
	volatile uint32_t SPIP_FIFOCTL;
	/* 0x14: SPI Status Register */
	volatile uint32_t SPIP_STATUS;
	volatile uint32_t RESERVE0[2];
	/* 0x20: SPI Data Transmit Register */
	volatile uint32_t SPIP_TX;
	volatile uint32_t RESERVE1[3];
	/* 0x30: SPI Data Receive Register */
	volatile uint32_t SPIP_RX;
};

/* SPIP register fields */
/* 0x00: SPI_CTL fields */
#define NCT_CTL_QUADIOEN		(22)
#define NCT_CTL_DUALIOEN		(21)
#define NCT_CTL_QDIODIR		(20)
#define NCT_CTL_REORDER		(19)
#define NCT_CTL_SLAVE			(18)
#define NCT_CTL_UNITIEN		(17)
#define NCT_CTL_TWOBIT			(16)
#define NCT_CTL_LSB			(13)
#define NCT_CTL_DWIDTH			FIELD(8, 5)
#define NCT_CTL_SUSPITV		FIELD(4, 4)
#define NCT_CTL_CLKPOL			(3)
#define NCT_CTL_TXNEG			(2)
#define NCT_CTL_RXNEG			(1)
#define NCT_CTL_SPIEN			(0)

/* 0x04: SPI_CLKDIV fields */
#define NCT_CLKDIV_DIVIDER		FIELD(0, 10)

/* 0x08: SPI_SSCTL fields */
#define NCT_SSCTL_SLVTOCNT		(16)
#define NCT_SSCTL_SSINAIEN		(13)
#define NCT_SSCTL_SSACTIEN		(12)
#define NCT_SSCTL_SLVURIEN		(9)
#define NCT_SSCTL_SLVBEIEN		(8)
#define NCT_SSCTL_SLVTORST		(6)
#define NCT_SSCTL_SLVTOIEN		(5)
#define NCT_SSCTL_SLV3WIRE		(4)
#define NCT_SSCTL_AUTOSS		(3)
#define NCT_SSCTL_SSACTPOL		(2)
#define NCT_SSCTL_SS			(0)

/* 0x0C: SPI_PDMACTL fields */
#define NCT_PDMACTL_PDMARST		(2)
#define NCT_PDMACTL_RXPDMAEN		(1)
#define NCT_PDMACTL_TXPDMAEN		(0)

/* 0x10: SPI_FIFOCTL fields */
#define NCT_FIFOCTL_TXTH		(28)
#define NCT_FIFOCTL_RXTH		(24)
#define NCT_FIFOCTL_TXUFIEN		(7)
#define NCT_FIFOCTL_TXUFPOL		(6)
#define NCT_FIFOCTL_RXOVIEN		(5)
#define NCT_FIFOCTL_RXTOIEN		(4)
#define NCT_FIFOCTL_TXTHIEN		(3)
#define NCT_FIFOCTL_RXTHIEN		(2)
#define NCT_FIFOCTL_TXRST		(1)
#define NCT_FIFOCTL_RXRST		(0)

/* 0x14: SPI_STATUS fields */
#define NCT_STATUS_TXCNT		(28)
#define NCT_STATUS_RXCNT		(24)
#define NCT_STATUS_TXRXRST		(23)
#define NCT_STATUS_TXUFIF		(19)
#define NCT_STATUS_TXTHIF		(18)
#define NCT_STATUS_TXFULL		(17)
#define NCT_STATUS_TXEMPTY		(16)
#define NCT_STATUS_SPIENSTS		(15)
#define NCT_STATUS_RXTOIF		(12)
#define NCT_STATUS_RXOVIF		(11)
#define NCT_STATUS_RXTHIF		(10)
#define NCT_STATUS_RXFULL		(9)
#define NCT_STATUS_RXEMPTY		(8)
#define NCT_STATUS_SLVUDRIF		(7)
#define NCT_STATUS_SLVBEIF		(6)
#define NCT_STATUS_SLVTOIF		(5)
#define NCT_STATUS_SSLINE		(4)
#define NCT_STATUS_SSINAIF		(3)
#define NCT_STATUS_SSACTIF		(2)
#define NCT_STATUS_UNITIF		(1)
#define NCT_STATUS_BUSY		(0)

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
	volatile uint32_t SPIM_RX[4];
	/* 0x020: Data Transmit Register 0 */
	volatile uint32_t SPIM_TX[4];
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
#define NCT_SPIM_CTL0_CMDCODE			FIELD(24, 8)
#define NCT_SPIM_CTL0_OPMODE			FIELD(22, 2)
#define NCT_SPIM_CTL0_OPMODE_NORMAL_IO		0x0
#define NCT_SPIM_CTL0_OPMODE_DMA_WRITE		0x1
#define NCT_SPIM_CTL0_OPMODE_DMA_READ		0x2
#define NCT_SPIM_CTL0_OPMODE_DMM		0x3
#define NCT_SPIM_CTL0_BITMODE			FIELD(20, 2)
#define NCT_SPIM_CTL0_BITMODE_STANDARD		0x0
#define NCT_SPIM_CTL0_BITMODE_DUAL		0x1
#define NCT_SPIM_CTL0_BITMODE_QUAD		0x2
#define NCT_SPIM_CTL0_SUSPITV			FIELD(16, 4)
#define NCT_SPIM_CTL0_QDIODIR			15
#define NCT_SPIM_CTL0_BURSTNUM			FIELD(13, 2)
#define NCT_SPIM_CTL0_BURSTNUM_1		0x0
#define NCT_SPIM_CTL0_BURSTNUM_2		0x1
#define NCT_SPIM_CTL0_BURSTNUM_3		0x2
#define NCT_SPIM_CTL0_BURSTNUM_4		0x3
#define NCT_SPIM_CTL0_DWIDTH			FIELD(8, 5)
#define NCT_SPIM_CTL0_DWIDTH_8			0x7
#define NCT_SPIM_CTL0_DWIDTH_16		0xF
#define NCT_SPIM_CTL0_DWIDTH_24		0x17
#define NCT_SPIM_CTL0_DWIDTH_32		0x1F
#define NCT_SPIM_CTL0_IF			7
#define NCT_SPIM_CTL0_IEN			6
#define NCT_SPIM_CTL0_B4ADDREN			5
#define NCT_SPIM_CTL0_CIPHOFF			0

/* 0x004: SPIM_CTL1 */
#define NCT_SPIM_CTL1_DIVIDER			FIELD(16, 16)
#define NCT_SPIM_CTL1_IDLE_TIME		FIELD(8, 4)
#define NCT_SPIM_CTL1_SSACTPOL			5
#define NCT_SPIM_CTL1_SS			4
#define NCT_SPIM_CTL1_CDINVAL			3
#define NCT_SPIM_CTL1_CACHEOFF			1
#define NCT_SPIM_CTL1_SPIMEN			0

/* 0x00C: SPIM_RXCLKDLY */
#define NCT_SPIM_RXCLKDLY_RDDLYSEL		FIELD(16, 3)
#define NCT_SPIM_RXCLKDLY_PHDELSEL		FIELD(8, 8)
#define NCT_SPIM_RXCLKDLY_DWDELSEL		FIELD(0, 8)

/* 0x044: SPIM_DMMCTL */
#define NCT_SPIM_DMMCTL_ACTSCLKT		FIELD(28, 4)
#define NCT_SPIM_DMMCTL_UACTSCLK		26
#define NCT_SPIM_DMMCTL_CREN			25
#define NCT_SPIM_DMMCTL_BWEN			24
#define NCT_SPIM_DMMCTL_DESELTIM		FIELD(16, 5)
#define NCT_SPIM_DMMCTL_CRMDAT			FIELD(8, 8)

/* 0x048: SPIM_CTL2 */
#define NCT_SPIM_CTL2_DCNUM			FIELD(24, 5)
#define NCT_SPIM_CTL2_USETEN			16

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
#define NCT_BURST_CFG_R_BURST			FIELD(0, 2)
#define NCT_BURST_CFG_SLAVE			2
#define NCT_BURST_CFG_UNLIM_BURST		3
#define NCT_BURST_CFG_SPI_WR_EN		7

/* 0x002: RESP CFG */
#define NCT_RESP_CFG_QUAD_EN			3

/* 0x014: SPI FL CFG */
#define NCT_SPI_FL_CFG_RD_MODE			FIELD(6, 2)
#define NCT_SPI_FL_CFG_RD_MODE_NORMAL		0
#define NCT_SPI_FL_CFG_RD_MODE_FAST		1
#define NCT_SPI_FL_CFG_RD_MODE_FAST_DUAL	3

/* 0x01E: UMA CTS */
#define NCT_UMA_CTS_D_SIZE			FIELD(0, 3)
#define NCT_UMA_CTS_A_SIZE			3
#define NCT_UMA_CTS_C_SIZE			4
#define NCT_UMA_CTS_RD_WR			5
#define NCT_UMA_CTS_DEV_NUM			6
#define NCT_UMA_CTS_EXEC_DONE			7

/* 0x01F: UMA ECTS */
#define NCT_UMA_ECTS_SW_CS0			0
#define NCT_UMA_ECTS_SW_CS1			1
#define NCT_UMA_ECTS_SW_CS2			2
#define NCT_UMA_ECTS_DEV_NUM_BACK		3
#define NCT_UMA_ECTS_UMA_ADDR_SIZE		FIELD(4, 3)

/* 0x026: CRC Control Register */
#define NCT_CRCCON_CALCEN			0
#define NCT_CRCCON_CKSMCRC			1
#define NCPM_CRCCON_UMAENT			2


/* 0x033: FIU Extended Configuration Register */
#define NCT_FIU_EXT_CFG_FOUR_BADDR		0

/* 0x03C: Set command enable 4 bytes address mode */
#define NCT_SET_CMD_EN_PVT_CMD_EN		4
#define NCPM_SET_CMD_EN_SHD_CMD_EN		5
#define NCPM_SET_CMD_EN_BACK_CMD_EN		6

/* 0x03D: 4 bytes address mode enable */
#define NCT_ADDR_4B_EN_PVT_4B			4
#define NCPM_ADDR_4B_EN_SHD_4B			5
#define NCPM_ADDR_4B_EN_BACK_4B			6

/* 0x042: Master FIU status */
#define NCT_FIU_MSR_STS_RD_PEND_UMA		0
#define NCT_FIU_MSR_STS_RD_PEND_FIU		1
#define NCT_FIU_MSR_STS_MSTR_INACT		2

/* 0x043: FIU master interrupt enable and configuration register */
#define NCT_FIU_MSR_IE_CFG_RD_PEND_UMA_IE	0
#define NCT_FIU_MSR_IE_CFG_RD_PEND_FIU_IE	1
#define NCT_FIU_MSR_IE_CFG_MSTR_INACT_IE	2
#define NCT_FIU_MSR_IE_CFG_UMA_BLOCK		3

/* 0x044: Quad program enable register */
#define NCT_Q_P_EN_QUAD_P_EN			0

/* 0x048: Extended data byte configurartion */
#define NCT_EXT_DB_CFG_D_SIZE_DB		FIELD(0, 5)
#define NCT_EXT_DB_CFG_EXT_DB_EN		5

/* 0x049: Direct write configuration */
#define NCT_DIRECT_WR_CFG_DIRECT_WR_BLOCK	1
#define NCT_DIRECT_WR_CFG_DW_CS2		5
#define NCT_DIRECT_WR_CFG_DW_CS1		6
#define NCT_DIRECT_WR_CFG_DW_CS0		7

/* BURST CFG R_BURST selections */
#define NCT_BURST_CFG_R_BURST_1B		0
#define NCT_BURST_CFG_R_BURST_16B		3

#define NCT_FIU_ADDR_3B_LENGTH			0x3
#define NCT_FIU_ADDR_4B_LENGTH			0x4
#define NCT_FIU_EXT_DB_SIZE			0x10

/* UMA fields selections */
#define UMA_FLD_ADDR     BIT(NCT_UMA_CTS_A_SIZE)  /* 3-bytes ADR field */
#define UMA_FLD_NO_CMD   BIT(NCT_UMA_CTS_C_SIZE)  /* No 1-Byte CMD field */
#define UMA_FLD_WRITE    BIT(NCT_UMA_CTS_RD_WR)   /* Write transaction */
#define UMA_FLD_SHD_SL   BIT(NCT_UMA_CTS_DEV_NUM) /* Shared flash selected */
#define UMA_FLD_EXEC     BIT(NCT_UMA_CTS_EXEC_DONE)

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
	volatile uint8_t  reserved5[16];
	/* 0x0180 Virtual Wire GPIO Slave-to-Master 0-15 */
	volatile uint32_t VWGPSM[16];
	/* 0x01C0 Virtual Wire GPIO Master-to-Slave 0-15 */
	volatile uint32_t VWGPMS[16];
	/* 0x0200 Virtual Wire Event Master-to-Slave Status */
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
#define NCT_ESPICFG_PCHANEN             0
#define NCT_ESPICFG_VWCHANEN            1
#define NCT_ESPICFG_OOBCHANEN           2
#define NCT_ESPICFG_FLASHCHANEN         3
#define NCT_ESPICFG_HPCHANEN            4
#define NCT_ESPICFG_HVWCHANEN           5
#define NCT_ESPICFG_HOOBCHANEN          6
#define NCT_ESPICFG_HFLASHCHANEN        7
#define NCT_ESPICFG_CHANS_FIELD         FIELD(0, 4)
#define NCT_ESPICFG_HCHANS_FIELD        FIELD(4, 4)
#define NCT_ESPICFG_IOMODE_FIELD        FIELD(8, 2)
#define NCT_ESPICFG_MAXFREQ_FIELD       FIELD(10, 3)
#define NCT_ESPICFG_VWMS_VALID_EN       13
#define NCT_ESPICFG_VWSM_VALID_EN       14
#define NCT_ESPICFG_FLASHCHANMODE       16
#define NCT_ESPICFG_PCCHN_SUPP          24
#define NCT_ESPICFG_VWCHN_SUPP          25
#define NCT_ESPICFG_OOBCHN_SUPP         26
#define NCT_ESPICFG_FLASHCHN_SUPP       27
#define NCT_ESPIIE_IBRSTIE              0
#define NCT_ESPIIE_CFGUPDIE             1
#define NCT_ESPIIE_BERRIE               2
#define NCT_ESPIIE_OOBRXIE              3
#define NCT_ESPIIE_FLASHRXIE            4
#define NCT_ESPIIE_SFLASHRDIE           5
#define NCT_ESPIIE_PERACCIE             6
#define NCT_ESPIIE_DFRDIE               7
#define NCT_ESPIIE_VWUPDIE              8
#define NCT_ESPIIE_ESPIRSTIE            9
#define NCT_ESPIIE_PLTRSTIE             10
#define NCT_ESPIIE_AMERRIE              15
#define NCT_ESPIIE_AMDONEIE             16
#define NCT_ESPIIE_BMTXDONEIE           19
#define NCT_ESPIIE_PBMRXIE              20
#define NCT_ESPIIE_PMSGRXIE             21
#define NCT_ESPIIE_BMBURSTERRIE         22
#define NCT_ESPIIE_BMBURSTDONEIE        23
#define NCT_ESPIWE_IBRSTWE              0
#define NCT_ESPIWE_CFGUPDWE             1
#define NCT_ESPIWE_BERRWE               2
#define NCT_ESPIWE_OOBRXWE              3
#define NCT_ESPIWE_FLASHRXWE            4
#define NCT_ESPIWE_SFLASHRDWE           5
#define NCT_ESPIWE_PERACCWE             6
#define NCT_ESPIWE_DFRDWE               7
#define NCT_ESPIWE_VWUPDWE              8
#define NCT_ESPIWE_ESPIRSTWE            9
#define NCT_ESPIWE_PBMRXWE              20
#define NCT_ESPIWE_PMSGRXWE             21
#define NCT_ESPISTS_IBRST               0
#define NCT_ESPISTS_CFGUPD              1
#define NCT_ESPISTS_BERR                2
#define NCT_ESPISTS_OOBRX               3
#define NCT_ESPISTS_FLASHRX             4
#define NCT_ESPISTS_SFLASHRD            5
#define NCT_ESPISTS_PERACC              6
#define NCT_ESPISTS_DFRD                7
#define NCT_ESPISTS_VWUPD               8
#define NCT_ESPISTS_ESPIRST             9
#define NCT_ESPISTS_PLTRST              10
#define NCT_ESPISTS_ESPIRST_DEASSERT    17
#define NCT_ESPISTS_PLTRST_DEASSERT     18
#define NCT_ESPISTS_FLASHPRTERR         25
#define NCT_ESPISTS_FLASHAUTORDREQ      26
#define NCT_ESPISTS_VWUPDW              27
#define NCT_ESPISTS_GB_RST              28
#define NCT_ESPISTS_DNX_RST             29
#define NCT_VWEVMS_WIRE                 FIELD(0, 4)
#define NCT_VWEVMS_VALID                FIELD(4, 4)
#define NCT_VWEVMS_MODIFIED             16
#define NCT_VWEV_S_TO_M                 0
#define NCT_VWEV_M_TO_S                 1
#define NCT_VWEVMS_DIRECTION_POS        7
#define NCT_VWEVMS_IE                   18
#define NCT_VWEVMS_WE                   24
#define NCT_VWEVSM_WIRE                 FIELD(0, 4)
#define NCT_VWEVSM_VALID                FIELD(4, 4)
#define NCT_VWEVSM_BIT_VALID(n)         (4+n)
#define NCT_VWEVSM_HW_WIRE              FIELD(24, 4)
#define NCT_VWGPSM_WIRE                 FIELD(0, 4)
#define NCT_VWGPSM_VALID                FIELD(4, 4)
#define NCT_VWGPSM_INDEX_EN             15
#define NCT_VWGPSM_IE                   18
#define NCT_VWGPMS_WIRE                 FIELD(0, 4)
#define NCT_VWGPMS_VALID                FIELD(4, 4)
#define NCT_VWGPMS_VALID_START_POS      4
#define NCT_VWGPMS_INDEX_EN             15
#define NCT_VWGPMS_MODIFIED             16
#define NCT_VWGPMS_IE                   18
#define NCT_VWGPMS_ENESPIRST            19
#define NCT_VWGP_S_TO_M                 0
#define NCT_VWGP_M_TO_S                 1
#define NCT_VWGPMS_DIRECTION_POS        7
#define NCT_VWEVSMTYPE_WAKETYPE		2
#define NCT_VWEVSMTYPE_PMETYPE		3
#define NCT_VWEVSMTYPE_SCITYPE		8
#define NCT_VWEVSMTYPE_SMITYPE		9
#define NCT_VWEVSMTYPE_RCINTYPE		10
#define NCT_OOBCTL_OOB_FREE             0
#define NCT_OOBCTL_OOB_AVAIL            1
#define NCT_OOBCTL_RSTBUFHEADS          2
#define NCT_OOBCTL_OOBPLSIZE            FIELD(10, 3)
#define NCT_FLASHCFG_FLASHBLERSSIZE     FIELD(7, 3)
#define NCT_FLASHCFG_FLASHPLSIZE        FIELD(10, 3)
#define NCT_FLASHCFG_FLASHREQSIZE       FIELD(13, 3)
#define NCT_FLASHCFG_FLASHCAPA          FIELD(16, 2)
#define NCT_FLASHCFG_SUPP_CAFS          1
#define NCT_FLASHCFG_SUPP_TAFS          2
#define NCT_FLASHCFG_SUPP_TAFS_CAFS     3
#define NCT_FLASHCFG_TRGFLASHEBLKSIZE   FIELD(18, 8)
#define NCT_FLASHCFG_FLASHREQSUP        FIELD(0, 3)
#define NCT_FLASHCTL_FLASH_NP_FREE      0
#define NCT_FLASHCTL_FLASH_TX_AVAIL     1
#define NCT_FLASHCTL_STRPHDR            2
#define NCT_FLASHCTL_DMATHRESH          FIELD(3, 2)
#define NCT_FLASHCTL_AMTSIZE            FIELD(5, 8)
#define NCT_FLASHCTL_RSTBUFHEADS        13
#define NCT_FLASHCTL_CRCEN              14
#define NCT_FLASHCTL_CHKSUMSEL          15
#define NCT_FLASHCTL_AMTEN              16
#define NCT_FLASHCTL_SAF_AUTO_READ      18
#define NCT_FLASHBASE_FLBASE_ADDR       FIELD(12, 17)
#define NCT_FLASH_PRTR_BADDR            FIELD(12, 17)
#define NCT_FRGN_RPR                    30
#define NCT_FRGN_WPR                    31
#define NCT_FLASH_PRTR_HADDR            FIELD(12, 17)
#define NCT_FLASH_TAG_OVR_RPR           FIELD(16, 16)
#define NCT_FLASH_TAG_OVR_WPR           FIELD(0, 16)
#define NCT_ESPIHINDP_AUTO_PCRDY        0
#define NCT_ESPIHINDP_AUTO_VWCRDY       1
#define NCT_ESPIHINDP_AUTO_OOBCRDY      2
#define NCT_ESPIHINDP_AUTO_FCARDY       3

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
#define NCT_MSWCTL1_HRSTOB		0
#define NCT_MSWCTL1_HWPRON		1
#define NCT_MSWCTL1_PLTRST_ACT		2
#define NCT_MSWCTL1_VHCFGA		3
#define NCT_MSWCTL1_HCFGLK		4
#define NCT_MSWCTL1_A20MB		7

#define NCT_BKUPSTS_VSB_STS	5
#define NCT_BKUPSTS_IBBR	7

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
#define NCT_SMC_STS_HRERR               0
#define NCT_SMC_STS_HWERR               1
#define NCT_SMC_STS_HSEM1W              4
#define NCT_SMC_STS_HSEM2W              5
#define NCT_SMC_STS_SHM_ACC             6
#define NCT_SMC_CTL_HERR_IE             2
#define NCT_SMC_CTL_HSEM1_IE            3
#define NCT_SMC_CTL_HSEM2_IE            4
#define NCT_SMC_CTL_ACC_IE              5
#define NCT_SMC_CTL_HSEM_IMA_IE         6
#define NCT_SMC_CTL_HOSTWAIT            7
#define NCT_SMC_CTL2_HSEM5_IE           4
#define NCT_FLASH_SIZE_STALL_HOST       6
#define NCT_FLASH_SIZE_RD_BURST         7
#define NCT_WIN_SIZE_RWIN1_SIZE_FIELD   FIELD(0, 4)
#define NCT_WIN_SIZE_RWIN2_SIZE_FIELD   FIELD(4, 4)
#define NCT_WIN_PROT_RW1L_RP            0
#define NCT_WIN_PROT_RW1L_WP            1
#define NCT_WIN_PROT_RW1H_RP            2
#define NCT_WIN_PROT_RW1H_WP            3
#define NCT_WIN_PROT_RW2L_RP            4
#define NCT_WIN_PROT_RW2L_WP            5
#define NCT_WIN_PROT_RW2H_RP            6
#define NCT_WIN_PROT_RW2H_WP            7
#define NCT_PWIN_SIZEI_RPROT            13
#define NCT_PWIN_SIZEI_WPROT            14
#define NCT_CSEM2                       6
#define NCT_CSEM3                       7
#define NCT_DP80STS_FWR                 5
#define NCT_DP80STS_FNE                 6
#define NCT_DP80STS_FOR                 7
#define NCT_DP80CTL_DP80EN              0
#define NCT_DP80CTL_SYNCEN              1
#define NCT_DP80CTL_ADV                 2
#define NCT_DP80CTL_RAA                 3
#define NCT_DP80CTL_RFIFO               4

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
#define NCT_HICTRL_OBFKIE               0
#define NCT_HICTRL_OBFMIE               1
#define NCT_HICTRL_OBECIE               2
#define NCT_HICTRL_IBFCIE               3
#define NCT_HICTRL_PMIHIE               4
#define NCT_HICTRL_PMIOCIE              5
#define NCT_HICTRL_PMICIE               6
#define NCT_HICTRL_FW_OBF               7
#define NCT_HIKMST_OBF                  0
#define NCT_HIKMST_IBF                  1
#define NCT_HIKMST_F0                   2
#define NCT_HIKMST_A2                   3
#define NCT_HIKMST_ST0                  4
#define NCT_HIKMST_ST1                  5
#define NCT_HIKMST_ST2                  6
#define NCT_HIKMST_ST3                  7

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
#define NCT_HIPMIE_SCIE                 1
#define NCT_HIPMIE_SMIE                 2
#define NCT_HIPMIE_HIRQE                3
#define NCT_HIPMIE_HSCIE                4
#define NCT_HIPMIE_HSMIE                5
#define NCT_HIPMIE_HSTA                 6
#define NCT_HIPMCTL_IBFIE               0
#define NCT_HIPMCTL_OBEIE               1
#define NCT_HIPMCTL_PLMS                3
#define NCT_HIPMCTL_SCIPOL              6
#define NCT_HIPMCTL_EME                 7
#define NCT_HIPMST_OBF                  0
#define NCT_HIPMST_IBF                  1
#define NCT_HIPMST_F0                   2
#define NCT_HIPMST_CMD                  3
#define NCT_HIPMST_ST0                  4
#define NCT_HIPMST_ST1                  5
#define NCT_HIPMST_ST2                  6
#define NCT_HIPMST_ST3                  7
#define NCT_HIPMIC_IRQB                 0
#define NCT_HIPMIC_SMIB                 1
#define NCT_HIPMIC_SCIB                 2
#define NCT_HIPMIC_SMIPOL               6
#define NCT_HIPMIC_SCIIS                7

#define NCT_HIPMCTL_PLMS_MSK            (0x7 << NCT_HIPMCTL_PLMS)

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
#define NCT_LKSIOHA_LKCFG               0
#define NCT_LKSIOHA_LKSPHA              2
#define NCT_LKSIOHA_LKHIKBD             11
#define NCT_CRSMAE_CFGAE                0
#define NCT_CRSMAE_HIKBDAE              11
#define NCT_SIOLV_SPLV                  2
#define NCT_SIBCTRL_CSAE                0
#define NCT_SIBCTRL_CSRD                1
#define NCT_SIBCTRL_CSWR                2

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
#define NCT_SMBnST_XMIT                  0
#define NCT_SMBnST_MASTER                1
#define NCT_SMBnST_NMATCH                2
#define NCT_SMBnST_STASTR                3
#define NCT_SMBnST_NEGACK                4
#define NCT_SMBnST_BER                   5
#define NCT_SMBnST_SDAST                 6
#define NCT_SMBnST_SLVSTP                7

/*--------------------------*/
/*     SMBnCST fields       */
/*--------------------------*/
#define NCT_SMBnCST_BUSY                 0
#define NCT_SMBnCST_BB                   1
#define NCT_SMBnCST_MATCH                2
#define NCT_SMBnCST_GCMATCH              3
#define NCT_SMBnCST_TSDA                 4
#define NCT_SMBnCST_TGSCL                5
#define NCT_SMBnCST_MATCHAF              6
#define NCT_SMBnCST_ARPMATCH             7

/*--------------------------*/
/*     SMBnCTL1 fields      */
/*--------------------------*/
#define NCT_SMBnCTL1_START               0
#define NCT_SMBnCTL1_STOP                1
#define NCT_SMBnCTL1_INTEN               2
#define NCT_SMBnCTL1_EOBINTE             3
#define NCT_SMBnCTL1_ACK                 4
#define NCT_SMBnCTL1_GCMEN               5
#define NCT_SMBnCTL1_NMINTE              6
#define NCT_SMBnCTL1_STASTRE             7

/*--------------------------*/
/*     SMBnADDR1-10 fields  */
/*--------------------------*/
#define NCT_SMBnADDR_ADDR                0
#define NCT_SMBnADDR_SAEN                7

/*--------------------------*/
/*    TIMEOUT_ST fields     */
/*--------------------------*/
#define NCT_TIMEOUT_ST_T_OUTST1          0
#define NCT_TIMEOUT_ST_T_OUTST2          1
#define NCT_TIMEOUT_ST_T_OUTST1_EN       6
#define NCT_TIMEOUT_ST_T_OUTST2_EN       7

/*--------------------------*/
/*     SMBnCTL2 fields      */
/*--------------------------*/
#define NCT_SMBnCTL2_ENABLE              0
#define NCT_SMBnCTL2_SCLFRQ60_FIELD      FIELD(1, 7)

/*--------------------------*/
/*     TIMEOUT_EN fields    */
/*--------------------------*/
#define NCT_TIMEOUT_EN_TIMEOUT_EN        0
#define NCT_TIMEOUT_EN_TO_CKDIV          2

/*--------------------------*/
/*     SMBnCTL3 fields      */
/*--------------------------*/
#define NCT_SMBnCTL3_SCLFRQ87_FIELD      FIELD(0, 2)
#define NCT_SMBnCTL3_ARPMEN              2
#define NCT_SMBnCTL3_SLP_START           3
#define NCT_SMBnCTL3_400K_MODE           4
#define NCT_SMBnCTL3_SDA_LVL             6
#define NCT_SMBnCTL3_SCL_LVL             7

/*--------------------------*/
/*      DMA_CTRL fields     */
/*--------------------------*/
#define NCT_DMA_CTRL_DMA_INT_CLR         0
#define NCT_DMA_CTRL_DMA_EN              1
#define NCT_DMA_CTRL_LAST_PEC            2
#define NCT_DMA_CTRL_DMA_STALL           3
#define NCT_DMA_CTRL_DMA_IRQ             7

/*--------------------------*/
/*     SMBnCST2 fields      */
/*--------------------------*/
#define NCT_SMBnCST2_MATCHA1F            0
#define NCT_SMBnCST2_MATCHA2F            1
#define NCT_SMBnCST2_MATCHA3F            2
#define NCT_SMBnCST2_MATCHA4F            3
#define NCT_SMBnCST2_MATCHA5F            4
#define NCT_SMBnCST2_MATCHA6F            5
#define NCT_SMBnCST2_MATCHA7F            6
#define NCT_SMBnCST2_INTSTS              7

/*--------------------------*/
/*     SMBnCST3 fields      */
/*--------------------------*/
#define NCT_SMBnCST3_MATCHA8F            0
#define NCT_SMBnCST3_MATCHA9F            1
#define NCT_SMBnCST3_MATCHA10F           2
#define NCT_SMBnCST3_EO_BUSY             7

/*
 * INTERNAL 8-BIT/16-BIT/32-BIT TIMER (ITIM32) device registers
 */

struct itim32_reg {
	/* [0x00] Internal 8-Bit Timer Counter */
	volatile uint8_t ITCNT;
	/* [0x01] Internal Timer Prescaler */
	volatile uint8_t ITPRE;
	/* [0x02] Internal 16-Bit Timer Counter */
	volatile uint16_t ITCNT16;
	/* [0x04] Internal Timer Control and Status */
	volatile uint8_t ITCTS;
	volatile uint8_t RESERVED1[3];
	/* [0x08] Internal 32-Bit Timer Counter */
	volatile uint32_t ITCNT32;
};

/* ITIM32 register fields */
#define NCT_ITCTS_ITEN				(7)
#define NCT_ITCTS_CKSEL			(4)
#define NCT_ITCTS_TO_WUE			(3)
#define NCT_ITCTS_TO_IE			(2)
#define NCT_ITCTS_TO_STS			(0)

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
#define NCT_TCKC_LOW_PWR                7
#define NCT_TCKC_C1CSEL_FIELD           FIELD(0, 3)
#define NCT_TCKC_C2CSEL_FIELD           FIELD(3, 3)
#define NCT_TMCTRL_MDSEL_FIELD          FIELD(0, 3)
#define NCT_TMCTRL_TAEN                 5
#define NCT_TMCTRL_TAEDG                3
#define NCT_TCFG_TADBEN                 6
#define NCT_TCFG_MFT_IN_SEL             FIELD(2, 4)
#define NCT_TECTRL_TAPND                0
#define NCT_TECTRL_TBPND                1
#define NCT_TECTRL_TCPND                2
#define NCT_TECTRL_TDPND                3
#define NCT_TECTRL_TEPND                4
#define NCT_TECTRL_TFPND                5
#define NCT_TECLR_TACLR                 0
#define NCT_TECLR_TBCLR                 1
#define NCT_TECLR_TCCLR                 2
#define NCT_TECLR_TDCLR                 3
#define NCT_TECLR_TECLR                 4
#define NCT_TECLR_TFCLR                 5
#define NCT_TIEN_TAIEN                  0
#define NCT_TIEN_TBIEN                  1
#define NCT_TIEN_TCIEN                  2
#define NCT_TIEN_TDIEN                  3
#define NCT_TIEN_TEIEN                  4
#define NCT_TIEN_TFIEN                  5
#define NCT_TWUEN_TAWEN                 0
#define NCT_TWUEN_TBWEN                 1
#define NCT_TWUEN_TCWEN                 2
#define NCT_TWUEN_TDWEN                 3
#define NCT_TWUEN_TEWEN                 4
#define NCT_TWUEN_TFWEN                 5

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
#define NCT_DBGFRZEN3_GLBL_FRZ_DIS      7

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
#define NCT_PECI_CTL_STS_START_BUSY     0
#define NCT_PECI_CTL_STS_DONE           1
#define NCT_PECI_CTL_STS_CRC_ERR        3
#define NCT_PECI_CTL_STS_ABRT_ERR       4
#define NCT_PECI_CTL_STS_AWFCS_EB       5
#define NCT_PECI_CTL_STS_DONE_EN        6
#define NCT_PECI_RATE_MAX_BIT_RATE      FIELD(0, 5)
#define NCT_PECI_RATE_MAX_BIT_RATE_MASK 0x1F
/* The minimal valid value of NCT_PECI_RATE_MAX_BIT_RATE field */
#define PECI_MAX_BIT_RATE_VALID_MIN      0x05
#define PECI_HIGH_SPEED_MIN_VAL          0x07

#define NCT_PECI_RATE_EHSP              6
#define NCT_PECI_RATE_HLOAD             7

struct pdma_dsct_reg {
	volatile uint32_t CTL;
	volatile uint32_t SA;
	volatile uint32_t DA;
	volatile uint32_t NEXT;
};

struct pdma_reg {
	/* 0x000 ~ 0x0DC: Descriptor Table Control Register 0 - 13 */
	struct pdma_dsct_reg PDMA_DSCT[14];
	volatile uint32_t reserved1[8];
	/* 0x100 ~ 0x134: Current Scatter-Gather Descriptor Table Address 0 - 13 */
	volatile uint32_t PDMA_CURSCAT[14];
	volatile uint32_t reserved2[178];
	/* 0x400: PDMA Channel Control Register */
	volatile uint32_t PDMA_CHCTL;
	/* 0x404: PDMA Stop Transfer Register */
	volatile uint32_t PDMA_STOP;
	/* 0x408: PDMA Software Request Register */
	volatile uint32_t PDMA_SWREQ;
	/* 0x40C: PDMA Request Active Flag Register */
	volatile uint32_t PDMA_TRGSTS;
	/* 0x410: PDMA Fixed Priority Setting Register */
	volatile uint32_t PDMA_PRISET;
	/* 0x414: PDMA Fixed Priority Clear Register */
	volatile uint32_t PDMA_PRICLR;
	/* 0x418: PDMA Interrupt Enable Control Register */
	volatile uint32_t PDMA_INTEN;
	/* 0x41C: PDMA PDMA Interrupt Status Register */
	volatile uint32_t PDMA_INTSTS;
	/* 0x420: PDMA Read/Write Target Abort Flag Register */
	volatile uint32_t PDMA_ABTSTS;
	/* 0x424: PDMA Transfer Done Flag Register */
	volatile uint32_t PDMA_TDSTS;
	/* 0x428: PDMA Scatter-Gather Transfer Done Flag Register */
	volatile uint32_t PDMA_SCATSTS;
	/* 0x42C: PDMA Transfer on Active Flag Register */
	volatile uint32_t PDMA_TACTSTS;
	volatile uint32_t reserved3[3];
	/* 0x43C: PDMA Scatter-Gather Descriptor Table Base Address Register */
	volatile uint32_t PDMA_SCATBA;
	volatile uint32_t reserved4[16];
	/* 0x480: PDMA Source Module Select Register 0 - 3 */
	volatile uint32_t PDMA_REQSEL[4];
};

#define NCT_PDMA_INTSTS_TEIF			2
#define NCT_PDMA_INTSTS_TDIF			1
#define NCT_PDMA_INTSTS_ABTIF			0
#define NCT_PDMA_SCATBA_16BITS			FIELD(16, 16)
#define NCT_PDMA_REQSEL_CHANNEL(ch)		FIELD((ch * 8), 7)
#define NCT_PDMA_DSCT_CTL_TXCNT		FIELD(16, 14)
#define NCT_PDMA_DSCT_CTL_TXWIDTH		FIELD(12, 2)
#define NCT_PDMA_DSCT_CTL_TX_WIDTH_8		0x0
#define NCT_PDMA_DSCT_CTL_TX_WIDTH_16		0x1
#define NCT_PDMA_DSCT_CTL_TX_WIDTH_32		0x2
#define NCT_PDMA_DSCT_CTL_DAINC		FIELD(10, 2)
#define NCT_PDMA_DSCT_CTL_DAINC_FIX		0x3
#define NCT_PDMA_DSCT_CTL_SAINC		FIELD(8, 2)
#define NCT_PDMA_DSCT_CTL_SAINC_FIX		0x3
#define NCT_PDMA_DSCT_CTL_TBINTDIS		7
#define NCT_PDMA_DSCT_CTL_BURSIZE		FIELD(4, 3)
#define NCT_PDMA_DSCT_CTL_TXTYPE_SINGLE	2
#define NCT_PDMA_DSCT_CTL_OPMODE		FIELD(0, 2)
#define NCT_PDMA_DSCT_CTL_OPMODE_STOP		0x0
#define NCT_PDMA_DSCT_CTL_OPMODE_BASIC		0x1
#define NCT_PDMA_DSCT_CTL_OPMODE_SGM		0x2
#define NCT_PDMA_DSCT_NEXT_DSCT_OFFSET		FIELD(2, 14)

/* n == pdma descriptor table address */
#define NCT_PDMA_BASE(n)			(n & 0xFFFFFF00)
#define NCT_PDMA_DSCT_IDX(n)			((n - (n & 0xFFFFFF00)) >> 4)
#define NCT_PDMA_CHANNEL_PER_REQ		0x4

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

#define NCT_SGPIO_PORT_PIN_NUM		8U
#define MAX_NR_HW_SGPIO			64
#define NCT_IOXCTS_RD_MODE_MASK	0x6
#define NCT_IOXCTS_RD_MODE_CONTINUOUS	BIT(2)
#define NCT_IOXCTS_IOXIF_EN		BIT(7)
#define NCT_IOXCFG1_SFT_CLK_MASK	0xF

#define NCT_XDOUT_OFFSET(n)		(0x00 + (n))
#define NCT_XDIN_OFFSET(n)		(0x08 + (n))
#define NCT_XEVCFG_OFFSET(n)		(0x10 + ((n) * 2L))
#define NCT_XEVSTS_OFFSET(n)		(0x20 + (n))

#define NCT_XDOUT(base, n)  (*(volatile uint8_t*)(base + \
						NCT_XDOUT_OFFSET(n)))
#define NCT_XDIN(base, n)   (*(volatile uint8_t*)(base + \
						NCT_XDIN_OFFSET(n)))
#define NCT_XEVCFG(base, n) (*(volatile uint16_t*)(base + \
						NCT_XEVCFG_OFFSET(n)))
#define NCT_XEVSTS(base, n) (*(volatile uint8_t*)(base + \
						NCT_XEVSTS_OFFSET(n)))

typedef enum {
	/* 1: Configuration */
	SIB_DEV_CFG = 1,
	/* 2: Print port */
	SIB_DEV_PRT,
	/* 3: UARTA */
	SIB_DEV_UARTA,
	/* 4: UARTB */
	SIB_DEV_UARTB,
	/* 5: Mailbox */
	SIB_DEV_MAILBOX,
	/* 6: CIR */
	SIB_DEV_CIR,
	/* 7: RTC */
	SIB_DEV_RTC,
	/* 8: Extend RAM */
	SIB_DEV_HRAM,
	/* 9: Mobile System Wake-Up Control */
	SIB_DEV_MSWC,
	/* 10: Shared Memory Core Access 2 */
	SIB_DEV_SHM2,
	/* 11: Shared Memory Core Access */
	SIB_DEV_SHM,
	/* 12: KBC */
	SIB_DEV_KBC,
	/* 13: Power Management Channel 1 */
	SIB_DEV_PMCHAN1,
	/* 14: Power Management Channel 2 */
	SIB_DEV_PMCHAN2,
	/* 15: Power Management Channel 3 */
	SIB_DEV_PMCHAN3,
	/* 16: Power Management Channel 4 */
	SIB_DEV_PMCHAN4,
	/* 17: UARTC */
	SIB_DEV_UARTC,
	/* 18: UARTD */
	SIB_DEV_UARTD,
	/* 19: UARTE */
	SIB_DEV_UARTE,
	/* 20: UARTF */
	SIB_DEV_UARTF,
} SIB_DEVICE_T;

typedef enum {
	/* means port 2E or 4E */
	CFG_INDEX   = 0x00,
	/* means port 2F or 4F */
	CFG_DATA    = 0x01,
} SIB_DEV_CFG_Enum;

typedef enum {
	RTC_INDEX   = 0x00,
	RTC_DATA    = 0x01,
} SIB_DEV_RTC_Enum;

typedef enum {
	RTC_SEC             = 0x00,
	RTC_SEC_ALARM       = 0x01,
	RTC_MIN             = 0x02,
	RTC_MIN_ALARM       = 0x03,
	RTC_HOUR            = 0x04,
	RTC_HOUR_ALARM      = 0x05,
	RTC_WEEKDAY         = 0x06,
	RTC_DAY             = 0x07,
	RTC_MONTH           = 0x08,
	RTC_YEAR            = 0x09,
	/* Timer Configuration */
	RTC_CFG             = 0x0A,
	/* Control */
	RTC_CTL             = 0x0B,
	/* Alarm Interrupt Flag */
	RTC_ALMFLG          = 0x0C,
	/* Control and Status */
	RTC_CTS             = 0x0D,
	RTC_WEEKDAY_ALARM   = 0x0E,
	RTC_DAY_ALARM       = 0x0F,
	RTC_MONTH_ALARM     = 0x10,
	RTC_YEAR_ALARM      = 0x11,
} SIB_RTC_OFFSET_Enum;

/*--------------------------*/
/* RTC_SECOND fields        */
/*--------------------------*/
#define RTC_SECOND_Pos                                     (0)
#define RTC_SECOND_Msk                                     (0x7F << RTC_SECOND_Pos)

/*--------------------------*/
/* RTC_SECONDALARM fields   */
/*--------------------------*/
#define RTC_SECONDALARM_Pos                                (0)
#define RTC_SECONDALARM_Msk                                (0x7F << RTC_SECONDALARM_Pos)

#define RTC_SECONDALARM_AENS_Pos                           (7)
#define RTC_SECONDALARM_AENS_Msk                           (0x01 << RTC_SECONDALARM_AENS_Pos)

/*--------------------------*/
/* RTC_MINUT fields         */
/*--------------------------*/
#define RTC_MINUT_Pos                                      (0)
#define RTC_MINUT_Msk                                      (0x7F << RTC_MINUT_Pos)

/*--------------------------*/
/* RTC_MINUTALARM fields    */
/*--------------------------*/
#define RTC_MINUTALARM_MINUTALARM_Pos                      (0)
#define RTC_MINUTALARM_MINUTALARM_Msk                      (0x7F << RTC_MINUTALARM_MINUTALARM_Pos)

#define RTC_MINUTALARM_AENM_Pos                            (7)
#define RTC_MINUTALARM_AENM_Msk                            (0x01 << RTC_MINUTALARM_AENM_Pos)

/*--------------------------*/
/* RTC_HOUR fields          */
/*--------------------------*/
#define RTC_HOUR_Pos                                       (0)
#define RTC_HOUR_Msk                                       (0x3F << RTC_HOUR_Pos)

/*--------------------------*/
/* RTC_HOURALARM fields     */
/*--------------------------*/
#define RTC_HOURALARM_HOURALARM_Pos                        (0)
#define RTC_HOURALARM_HOURALARM_Msk                        (0x3F << RTC_HOURALARM_HOURALARM_Pos)

#define RTC_HOURALARM_AMPM_Pos                             (7)
#define RTC_HOURALARM_AMPM_Msk                             (0x01 << RTC_HOURALARM_AMPM_Pos)

/*--------------------------*/
/* RTC_WEEKDAY fields       */
/*--------------------------*/
#define RTC_WEEKDAY_Pos                                    (0)
#define RTC_WEEKDAY_Msk                                    (0x07 << RTC_WEEKDAY_Pos)

/*--------------------------*/
/* RTC_DAY fields           */
/*--------------------------*/
#define RTC_DAY_Pos                                        (0)
#define RTC_DAY_Msk                                        (0x3F << RTC_DAY_Pos)

/*--------------------------*/
/* RTC_MONTH fields         */
/*--------------------------*/
#define RTC_MONTH_Pos                                      (0)
#define RTC_MONTH_Msk                                      (0x1F << RTC_MONTH_Pos)

/*--------------------------*/
/* RTC_YEAR fields          */
/*--------------------------*/
#define RTC_YEAR_Pos                                       (0)
#define RTC_YEAR_Msk                                       (0xFF << RTC_YEAR_Pos)

/*--------------------------*/
/* RTC_CFG fields           */
/*--------------------------*/
#define RTC_CFG_ENRTCTIME_Pos                              (0)
#define RTC_CFG_ENRTCTIME_Msk                              (0x01 << RTC_CFG_ENRTCTIME_Pos)

#define RTC_CFG_1224MODE_Pos                               (1)
#define RTC_CFG_1224MODE_Msk                               (0x01 << RTC_CFG_1224MODE_Pos)

#define RTC_CFG_dstMODE_Pos                                (2)
#define RTC_CFG_dstMODE_Msk                                (0x01 << RTC_CFG_dstMODE_Pos)

#define RTC_CFG_ENRTCPAD_Pos                               (3)
#define RTC_CFG_ENRTCPAD_Msk                               (0x01 << RTC_CFG_ENRTCPAD_Pos)

#define RTC_CFG_RTC2PME_Pos                                (5)
#define RTC_CFG_RTC2PME_Msk                                (0x01 << RTC_CFG_RTC2PME_Pos)

/*--------------------------*/
/* RTC_CTRL fields          */
/*--------------------------*/
#define RTC_CTRL_AIE_Pos                                   (0)
#define RTC_CTRL_AIE_Msk                                   (0x01 << RTC_CTRL_AIE_Pos)

/*--------------------------*/
/* RTC_ALMFLG fields        */
/*--------------------------*/
#define RTC_ALMFLG_AF_Pos                                  (0)
#define RTC_ALMFLG_AF_Msk                                  (0x01 << RTC_ALMFLG_AF_Pos)

/*--------------------------*/
/* RTC_CTS fields           */
/*--------------------------*/
#define RTC_CTS_AENY_Pos                                   (0)
#define RTC_CTS_AENY_Msk                                   (0x01 << RTC_CTS_AENY_Pos)

#define RTC_CTS_AENH_Pos                                   (1)
#define RTC_CTS_AENH_Msk                                   (0x01 << RTC_CTS_AENH_Pos)

#define RTC_CTS_ENRTCTIMESTS_Pos                           (5)
#define RTC_CTS_ENRTCTIMESTS_Msk                           (0x01 << RTC_CTS_ENRTCTIMESTS_Pos)

#define RTC_CTS_RTCPAD05STS_Pos                            (6)
#define RTC_CTS_RTCPAD05STS_Msk                            (0x01 << RTC_CTS_RTCPAD05STS_Pos)

#define RTC_CTS_PADSTS_Pos                                 (7)
#define RTC_CTS_PADSTS_Msk                                 (0x01 << RTC_CTS_PADSTS_Pos)

/*--------------------------*/
/* RTC_WEEKDAYALARM fields  */
/*--------------------------*/
#define RTC_WEEKDAYALARM_WEEKALARM_Pos                     (0)
#define RTC_WEEKDAYALARM_WEEKALARM_Msk                     (0x07 << RTC_WEEKDAYALARM_WEEKALARM_Pos)

#define RTC_WEEKDAYALARM_AENW_Pos                          (7)
#define RTC_WEEKDAYALARM_AENW_Msk                          (0x01 << RTC_WEEKDAYALARM_AENW_Pos)

/*--------------------------*/
/* RTC_DAYALARM fields      */
/*--------------------------*/
#define RTC_DAYALARM_Pos                                   (0)
#define RTC_DAYALARM_Msk                                   (0x3F << RTC_DAYALARM_Pos)

#define RTC_DAYALARM_AEND_Pos                              (7)
#define RTC_DAYALARM_AEND_Msk                              (0x01 << RTC_DAYALARM_AEND_Pos)

/*--------------------------*/
/* RTC_MONTHALARM fields    */
/*--------------------------*/
#define RTC_MONTHALARM_Pos                                 (0)
#define RTC_MONTHALARM_Msk                                 (0x1F << RTC_MONTHALARM_Pos)

#define RTC_MONTHALARM_AENMON_Pos                          (7)
#define RTC_MONTHALARM_AENMON_Msk                          (0x01 << RTC_MONTHALARM_AENMON_Pos)

/*--------------------------*/
/* RTC_YEARALARM fields     */
/*--------------------------*/
#define RTC_YEARALARM_Pos                                  (0)
#define RTC_YEARALARM_Msk                                  (0xFF << RTC_YEARALARM_Pos)


/*---------------------- USB Host Controller -------------------------*/
struct usbh_reg {
    /* 0x00: OHCI Revision Register */
    volatile uint32_t HcRevision;
    /* 0x04: OHCI Control Register */
    volatile uint32_t HcControl;
    /* 0x08: OHCI Command Status Register */
    volatile uint32_t HcCommandStatus;
    /* 0x0C: OHCI Interrupt Status Register */
    volatile uint32_t HcInterruptStatus;
    /* 0x10: OHCI Interrupt Enable Register */
    volatile uint32_t HcInterruptEnable;
    /* 0x14: OHCI Interrupt Disable Register */
    volatile uint32_t HcInterruptDisable;
    /* 0x18: OHCI HCCA Base Address Register */
    volatile uint32_t HcHCCA;
    /* 0x1C: OHCI Period Current ED Register */
    volatile uint32_t HcPeriodCurrentED;
    /* 0x20: OHCI Control Head ED Register */
    volatile uint32_t HcControlHeadED;
    /* 0x24: OHCI Control Current ED Register */
    volatile uint32_t HcControlCurrentED;
    /* 0x28: OHCI Bulk Head ED Register */
    volatile uint32_t HcBulkHeadED;
    /* 0x2C: OHCI Bulk Current ED Register */
    volatile uint32_t HcBulkCurrentED;
    /* 0x30: OHCI Done Head Register */
    volatile uint32_t HcDoneHead;
    /* 0x34: OHCI FM Interval Register */
    volatile uint32_t HcFmInterval;
    /* 0x38: OHCI FM Remaining Register */
    volatile uint32_t HcFmRemaining;
    /* 0x3C: OHCI FM Number Register */
    volatile uint32_t HcFmNumber;
    /* 0x40: OHCI Periodic Start Register */
    volatile uint32_t HcPeriodicStart;
    /* 0x44: OHCI LSThreshold Register */
    volatile uint32_t HcLSThreshold;
    /* 0x48: OHCI Root Hub Descriptor A Register */
    volatile uint32_t HcRhDescriptorA;
    /* 0x4C: OHCI Root Hub Descriptor B Register */
    volatile uint32_t HcRhDescriptorB;
    /* 0x50: OHCI Root Hub Status Register */
    volatile uint32_t HcRhStatus;
    /* 0x54, 0x58: OHCI Root Hub Port Status [1/2] Register */
    volatile uint32_t HcRhPortStatus[2];
    volatile uint32_t RESERVE0[108];
    /* 0x20C  USB Host Controller Config4 Register */
    volatile uint32_t HcConfig4;
};

/* HcRevision fields */
#define NCT_HcRevision_REV                 FIELD(0, 8)

/* HcControl field */
#define NCT_HcControl_CBSR_FIELD           FIELD(0, 1)
#define NCT_HcControl_PLE                  (2)
#define NCT_HcControl_IE                   (3)
#define NCT_HcControl_CLE                  (4)
#define NCT_HcControl_BLE                  (5)
#define NCT_HcControl_HCFS_FIELD           FIELD(6, 7)
#define NCT_HcControl_IR                   (8)
#define NCT_HcControl_RWC                  (9)
#define NCT_HcControl_RWCE                 (10)

/* HcCommandStatus fields */
#define NCT_HcCommandStatus_HCR            (0)
#define NCT_HcCommandStatus_CLF            (1)
#define NCT_HcCommandStatus_BLF            (2)
#define NCT_HcCommandStatus_OCR            (3)
#define NCT_HcCommandStatus_SOC_FIELD      FIELD(16, 17)

/* HcInterruptStatus fields */
#define NCT_HcInterruptStatus_SO           (0)
#define NCT_HcInterruptStatus_WDH          (1)
#define NCT_HcInterruptStatus_SF           (2)
#define NCT_HcInterruptStatus_RD           (3)
#define NCT_HcInterruptStatus_UE           (4)
#define NCT_HcInterruptStatus_FNO          (5)
#define NCT_HcInterruptStatus_RHSC         (6)
#define NCT_HcInterruptStatus_OC           (30)

/* HcInterruptEnable fields */
#define NCT_HcInterruptEnable_SO           (0)
#define NCT_HcInterruptEnable_WDH          (1)
#define NCT_HcInterruptEnable_SF           (2)
#define NCT_HcInterruptEnable_RD           (3)
#define NCT_HcInterruptEnable_UE           (4)
#define NCT_HcInterruptEnable_FNO          (5)
#define NCT_HcInterruptEnable_RHSC         (6)
#define NCT_HcInterruptEnable_OC           (30)
#define NCT_HcInterruptEnable_MIE          (31)

/* HcInterruptDisable fields */
#define NCT_HcInterruptDisable_SO          (0)
#define NCT_HcInterruptDisable_WDH         (1)
#define NCT_HcInterruptDisable_SF          (2)
#define NCT_HcInterruptDisable_RD          (3)
#define NCT_HcInterruptDisable_FNO         (5)
#define NCT_HcInterruptDisable_RHSC        (6)
#define NCT_HcInterruptDisable_MIE         (31)

/* HcHCCA fields */
#define NCT_HcHCCA_HCCA_FIELD              FIELD(8, 31)

/* HcPeriodCurrentED fields*/
#define NCT_HcPeriodCurrentED_PCED_FIELD   FIELD(4, 31)

/* HcControlHeadED fields*/
#define NCT_HcControlHeadED_CHED_FIELD     FIELD(4, 31)

/* HcControlCurrentED fields*/
#define NCT_HcControlCurrentED_CCED_FIELD  FIELD(4, 31)

/* HcBulkHeadED fields*/
#define NCT_HcBulkHeadED_BHED_FIELD        FIELD(4, 31)

/* HcBulkCurrentED fields*/
#define NCT_HcBulkCurrentED_BCED_FIELD     FIELD(4, 31)

/* HcDoneHead fields*/
#define NCT_HcDoneHead_DH_FIELD            FIELD(4, 31)

/* HcFmInterval fields*/
#define NCT_HcFmInterval_FI_FIELD          FIELD(0, 13)
#define NCT_HcFmInterval_FSMPS_FIELD       FIELD(16, 30)
#define NCT_HcFmInterval_FIT               (31)

/* HcFmRemaining fields*/
#define NCT_HcFmRemaining_FR_FIELD         FIELD(0, 13)
#define NCT_HcFmRemaining_FRT              (31)

/* HcFmNumber fields*/
#define NCT_HcFmNumber_FN_FIELD            FIELD(0, 15)

/* HcPeriodicStart fields*/
#define NCT_HcPeriodicStart_PS_FIELD       FIELD(0, 13)

/* HcLSThreshold fields*/
#define NCT_HcLSThreshold_LS_FIELDT        FIELD(0, 11)

/* HcRhDescriptorA fields*/
#define NCT_HcRhDescriptorA_NDP_FIELD      FIELD(0, 7)

/* HcRhDescriptorA fields*/
#define NCT_HcRhDescriptorA_PSM            (8)
#define NCT_HcRhDescriptorA_NPS            (9)
#define NCT_HcRhDescriptorA_DT             (10)
#define NCT_HcRhDescriptorA_OCPM           (11)
#define NCT_HcRhDescriptorA_NOCP           (12)
#define NCT_HcRhDescriptorA_POTPGT_FIELD   FIELD(24, 31)

/* HcRhDescriptorB fields*/
#define NCT_HcRhDescriptorB_DR_FIELD       FIELD(0, 15)
#define NCT_HcRhDescriptorB_PPCM_FIELD     FIELD(16, 31)

/* HcRhStatus fields*/
#define NCT_HcRhStatus_LPS                 (0)
#define NCT_HcRhStatus_OCI                 (1)
#define NCT_HcRhStatus_DRWE                (15)
#define NCT_HcRhStatus_LPSC                (16)
#define NCT_HcRhStatus_OCIC                (17)
#define NCT_HcRhStatus_CRWE                (31)

/* HcRhPortStatus fields*/
#define NCT_HcRhPortStatus_CCS             (0)
#define NCT_HcRhPortStatus_PES             (1)
#define NCT_HcRhPortStatus_PSS             (2)
#define NCT_HcRhPortStatus_POCI            (3)
#define NCT_HcRhPortStatus_PRS             (4)
#define NCT_HcRhPortStatus_PPS             (8)
#define NCT_HcRhPortStatus_LSDA            (9)
#define NCT_HcRhPortStatus_CSC             (16)
#define NCT_HcRhPortStatus_PESC            (17)
#define NCT_HcRhPortStatus_PSSC            (18)
#define NCT_HcRhPortStatus_OCIC            (19)
#define NCT_HcRhPortStatus_PRSC            (20)

/* HcConfig4 fields*/
#define NCT_HcConfig4_DMPULLDOWN           (3)
#define NCT_HcConfig4_DPPULLDOWN           (4)

/*---------------------- OTP -------------------------*/
struct otp_reg {
    __IO    uint8_t CTRL;                           /*!< [0x00]    OTP Control                                */
    __IO    uint8_t ADDR_H;                         /*!< [0x01]    OTP address H Configuration                */
    __IO    uint8_t ADDR_L;                         /*!< [0x02]    OTP address L Configuration                */
    __IO    uint8_t ECODE;                          /*!< [0x03]    OTP entry code Configuration               */
    __I     uint8_t STS;                            /*!< [0x04]    OTP status                                 */
    __IO    uint8_t CLKDIV;                         /*!< [0x05]    OTP clock divider                          */
            uint8_t Reserved1;
    __I     uint8_t DOUT;                           /*!< [0x07]    OTP data out                               */
    __IO    uint8_t LOCK;                           /*!< [0x08]    OTP region lock                            */
    __IO    uint8_t LOCK1;                          /*!< [0x09]    OTP region lock1                           */
            uint8_t Reserved2[20];
    __I     uint8_t KEYSTS;                         /*!< [0x1E]    OTP initial key status                     */
    __I     uint8_t VER;                            /*!< [0x1F]    OTP version                                */

};

/* OTP_CTRL fields          */
#define NCT_OTP_CTRL_RDEN                           (0)
#define NCT_OTP_CTRL_PGEN                           (1)
#define NCT_OTP_CTRL_ENOTP                          (7)

/* OTP_ADDR_H fields        */
#define NCT_OTP_ADDRH_ADDR5to8                      (0)

/* OTP_ADDR_L fields        */
#define NCT_OTP_ADDRL_PGBIT                         (0)
#define NCT_OTP_ADDRL_ADDR0to4                      (3)

/* OTP_STS fields          */
#define NCT_OTP_STS_PERROR                          (0)
#define NCT_OTP_STS_PGDSTS                          (1)
#define NCT_OTP_STS_RDDSTS                          (2)
#define NCT_OTP_STS_PWE                             (3)
#define NCT_OTP_STS_PRD                             (6)
#define NCT_OTP_STS_ENTRYOTP                        (7)

/* OTP_CLKDIV fields        */
#define NCT_OTP_CLKDIV                              (0)

/* OTP_DOUT fields          */
#define NCT_OTP_DOUT                                (0)

/* OTP_LOCK fields          */
#define NCT_OTP_LOCK_RGN0WRLOCK                     (0)
#define NCT_OTP_LOCK_RGN1WRLOCK                     (1)
#define NCT_OTP_LOCK_RGN2WRLOCK                     (2)
#define NCT_OTP_LOCK_RGN0RDWRLOCK                   (3)
#define NCT_OTP_LOCK_RGN1RDWRLOCK                   (4)
#define NCT_OTP_LOCK_RGN2RDWRLOCK                   (5)
#define NCT_OTP_LOCK_SESSKEYRDWRLOCK                (6)
#define NCT_OTP_LOCK_UNMAPROM                       (7)
#define NCT_OTP_LOCK1_RGN3WRLOCK                    (0)
#define NCT_OTP_LOCK1_RGN4WRLOCK                    (1)
#define NCT_OTP_LOCK1_RGN5WRLOCK                    (2)
#define NCT_OTP_LOCK1_RGN6WRLOCK                    (3)
#define NCT_OTP_LOCK1_RGN3RDWRLOCK                  (4)
#define NCT_OTP_LOCK1_RGN4RDWRLOCK                  (5)
#define NCT_OTP_LOCK1_RGN5RDWRLOCK                  (6)
#define NCT_OTP_LOCK1_RGN6RDWRLOCK                  (7)

#endif /* _NUVOTON_NCT_SOC_REG_DEF_H */
