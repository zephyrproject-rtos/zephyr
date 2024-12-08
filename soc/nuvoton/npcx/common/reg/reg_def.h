/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_REG_DEF_H
#define _NUVOTON_NPCX_REG_DEF_H

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

/*
 * NPCX register structure size/offset checking macro function to mitigate
 * the risk of unexpected compiling results. All addresses of NPCX registers
 * must meet the alignment requirement of cortex-m4.
 * DO NOT use 'packed' attribute if module contains different length ie.
 * 8/16/32 bits registers.
 */
#define NPCX_REG_SIZE_CHECK(reg_def, size) \
	BUILD_ASSERT(sizeof(struct reg_def) == size, \
		"Failed in size check of register structure!")
#define NPCX_REG_OFFSET_CHECK(reg_def, member, offset) \
	BUILD_ASSERT(offsetof(struct reg_def, member) == offset, \
		"Failed in offset check of register structure member!")

/*
 * NPCX register access checking via structure macro function to mitigate the
 * risk of unexpected compiling results if module contains different length
 * registers. For example, a word register access might break into two byte
 * register accesses by adding 'packed' attribute.
 *
 * For example, add this macro for word register 'PRSC' of PWM module in its
 * device init function for checking violation. Once it occurred, core will be
 * stalled forever and easy to find out what happens.
 */
#define NPCX_REG_WORD_ACCESS_CHECK(reg, val) { \
		uint16_t placeholder = reg; \
		reg = val; \
		__ASSERT(reg == val, "16-bit reg access failed!"); \
		reg = placeholder; \
	}
#define NPCX_REG_DWORD_ACCESS_CHECK(reg, val) { \
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
	volatile uint8_t reserved12[8];
	/* 0x01d: HFCG Bus Clock Dividers */
	volatile uint8_t HFCBCD3;
	volatile uint8_t reserved8[226];

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
#define NPCX_HFCGCTRL_LOAD                    0
#define NPCX_HFCGCTRL_LOCK                    2
#define NPCX_HFCGCTRL_CLK_CHNG                7

#define NPCX_LFCGCTL2_XT_OSC_SL_EN            6

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

/* PMC internal inline functions for multi-registers */
static inline uint32_t npcx_pwdwn_ctl_offset(uint32_t ctl_no)
{
	if (ctl_no < 6) {
		return 0x008 + ctl_no;
	} else {
		return 0x024 + ctl_no - 6;
	}
}

/* Macro functions for PMC multi-registers */
#define NPCX_PWDWN_CTL(base, n) (*(volatile uint8_t *)(base + \
						npcx_pwdwn_ctl_offset(n)))

/* PMC register fields */
#define NPCX_PMCSR_DI_INSTW                   0
#define NPCX_PMCSR_DHF                        1
#define NPCX_PMCSR_IDLE                       2
#define NPCX_PMCSR_NWBI                       3
#define NPCX_PMCSR_OHFC                       6
#define NPCX_PMCSR_OLFC                       7
#define NPCX_DISIDL_CTL_RAM_DID               5
#define NPCX_ENIDL_CTL_ADC_LFSL               7
#define NPCX_ENIDL_CTL_LP_WK_CTL              6
#define NPCX_ENIDL_CTL_PECI_ENI               2
#define NPCX_ENIDL_CTL_ADC_ACC_DIS            1

/* Macro functions for Development and Debugger Interface (DDI) registers */
#define NPCX_DBGCTRL(base)   (*(volatile uint8_t *)(base + 0x004))
#define NPCX_DBGFRZEN1(base) (*(volatile uint8_t *)(base + 0x006))
#define NPCX_DBGFRZEN2(base) (*(volatile uint8_t *)(base + 0x007))
#define NPCX_DBGFRZEN3(base) (*(volatile uint8_t *)(base + 0x008))
#define NPCX_DBGFRZEN4(base) (*(volatile uint8_t *)(base + 0x009))

/* DDI register fields */
#define NPCX_DBGCTRL_CCDEV_SEL		FIELD(6, 2)
#define NPCX_DBGCTRL_CCDEV_DIR		5
#define NPCX_DBGCTRL_SEQ_WK_EN		4
#define NPCX_DBGCTRL_FRCLK_SEL_DIS	3
#define NPCX_DBGFRZEN1_SPIFEN		7
#define NPCX_DBGFRZEN1_HIFEN		6
#define NPCX_DBGFRZEN1_ESPISEN		5
#define NPCX_DBGFRZEN1_UART1FEN		4
#define NPCX_DBGFRZEN1_SMB3FEN		3
#define NPCX_DBGFRZEN1_SMB2FEN		2
#define NPCX_DBGFRZEN1_MFT2FEN		1
#define NPCX_DBGFRZEN1_MFT1FEN		0
#define NPCX_DBGFRZEN2_ITIM6FEN		7
#define NPCX_DBGFRZEN2_ITIM5FEN		6
#define NPCX_DBGFRZEN2_ITIM4FEN		5
#define NPCX_DBGFRZEN2_ITIM64FEN	3
#define NPCX_DBGFRZEN2_SMB1FEN		2
#define NPCX_DBGFRZEN2_SMB0FEN		1
#define NPCX_DBGFRZEN2_MFT3FEN		0
#define NPCX_DBGFRZEN3_GLBL_FRZ_DIS	7
#define NPCX_DBGFRZEN3_ITIM3FEN		6
#define NPCX_DBGFRZEN3_ITIM2FEN		5
#define NPCX_DBGFRZEN3_ITIM1FEN		4
#define NPCX_DBGFRZEN3_I3CFEN		2
#define NPCX_DBGFRZEN3_SMB4FEN		1
#define NPCX_DBGFRZEN3_SHMFEN		0
#define NPCX_DBGFRZEN4_UART2FEN		6
#define NPCX_DBGFRZEN4_UART3FEN		5
#define NPCX_DBGFRZEN4_UART4FEN		4
#define NPCX_DBGFRZEN4_LCTFEN		3
#define NPCX_DBGFRZEN4_SMB7FEN		2
#define NPCX_DBGFRZEN4_SMB6FEN		1
#define NPCX_DBGFRZEN4_SMB5FEN		0

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
	/* 0x004: Device Control 3*/
	volatile uint8_t DEV_CTL3;
	volatile uint8_t reserved2;
	/* 0x006: Device Control 4 */
	volatile uint8_t DEV_CTL4;
	volatile uint8_t reserved3[9];
	/* 0x010 - 1F: Device Alternate Function 0 - F */
	volatile uint8_t DEVALT0[16];
	volatile uint8_t reserved4[6];
	/* 0x026: Low-Voltage GPIO Pins Control 5 */
	volatile uint8_t LV_GPIO_CTL5[1];
	volatile uint8_t reserved5;
	/* 0x028: Pull-Up/Pull-Down Enable 0 */
	volatile uint8_t PUPD_EN0;
	/* 0x029: Pull-Up/Pull-Down Enable 1 */
	volatile uint8_t PUPD_EN1;
	/* 0x02A - 2E: Low-Voltage GPIO Pins Control 0 - 4 */
	volatile uint8_t LV_GPIO_CTL0[5];
};

/* Macro functions for SCFG multi-registers */
#define NPCX_DEV_CTL(base, n) \
	(*(volatile uint8_t *)(base + n))
#define NPCX_DEVALT(base, n) \
	(*(volatile uint8_t *)(base + NPCX_DEVALT_OFFSET(n)))
#define NPCX_DEVALT_LK(base, n) \
	(*(volatile uint8_t *)(base + NPCX_DEVALT_LK_OFFSET(n)))
#define NPCX_PUPD_EN(base, n) \
	(*(volatile uint8_t *)(base + NPCX_PUPD_EN_OFFSET(n)))
#define NPCX_LV_GPIO_CTL(base, n) \
	(*(volatile uint8_t *)(base + NPCX_LV_GPIO_CTL_OFFSET(n)))

#define NPCX_JEN_CTL1_OFFSET 0x120
#define NPCX_JEN_CTL1(base) (*(volatile uint8_t *)(base + (NPCX_JEN_CTL1_OFFSET)))

#define NPCX_JEN_CTL1_JEN_EN       FIELD(0, 4)
#define NPCX_JEN_CTL1_JEN_HEN      FIELD(4, 4)
#define NPCX_JEN_CTL1_JEN_ENABLE   0x9
#define NPCX_JEN_CTL1_JEN_DISABLE  0x6

/* SCFG register fields */
#define NPCX_DEVCNT_F_SPI_TRIS                6
#define NPCX_DEVCNT_HIF_TYP_SEL_FIELD         FIELD(2, 2)
#define NPCX_DEVCNT_JEN1_HEN                  5
#define NPCX_DEVCNT_JEN0_HEN                  4
#define NPCX_STRPST_TRIST                     1
#define NPCX_STRPST_TEST                      2
#define NPCX_STRPST_JEN1                      4
#define NPCX_STRPST_JEN0                      5
#define NPCX_STRPST_SPI_COMP                  7
#define NPCX_RSTCTL_VCC1_RST_STS              0
#define NPCX_RSTCTL_DBGRST_STS                1
#define NPCX_RSTCTL_VCC1_RST_SCRATCH          3
#define NPCX_RSTCTL_LRESET_PLTRST_MODE        5
#define NPCX_RSTCTL_HIPRST_MODE               6
#define NPCX_DEV_CTL3_RNGINT_MD               1
#define NPCX_DEV_CTL3_FVCC1_PURST_EN          2
#define NPCX_DEV_CTL3_I3C1_MS                 3
#define NPCX_DEV_CTL3_I3C2_MS                 4
#define NPCX_DEV_CTL3_I3C3_MS                 5
#define NPCX_DEV_CTL3_SIO_CLK_SEL             FIELD(6, 2)
#define NPCX_DEV_CTL4_F_SPI_SLLK              2
#define NPCX_DEV_CTL4_SPI_SP_SEL              4
#define NPCX_DEV_CTL4_WP_IF                   5
#define NPCX_DEV_CTL4_VCC1_RST_LK             6
#define NPCX_DEVPU0_I2C0_0_PUE                0
#define NPCX_DEVPU0_I2C0_1_PUE                1
#define NPCX_DEVPU0_I2C1_0_PUE                2
#define NPCX_DEVPU0_I2C2_0_PUE                4
#define NPCX_DEVPU0_I2C3_0_PUE                6
#define NPCX_DEVPU1_F_SPI_PUD_EN              7

#if defined(CONFIG_SOC_SERIES_NPCX4)
/* I3C module controller, target mode for the MDMA module operation */
#define NPCX_DEV_CTL3_I3C_MODE_BIT(inst_id) BIT(inst_id + 3)
#endif /* End of CONFIG_SOC_SERIES_NPCX4 */

/* Supported host interface type for HIF_TYP_SEL FILED in DEVCNT register. */
enum npcx_hif_type {
	NPCX_HIF_TYPE_NONE,
	NPCX_HIF_TYPE_LPC,
	NPCX_HIF_TYPE_ESPI_SHI,
};

/* Supported VOSCCLK frequency for SIO_CLK_SEL FILED in DEV_CTL3 register. */
enum npcx_voscclk_type {
	NPCX_VOSCCLK_96MHz,
	NPCX_VOSCCLK_100MHz,
	NPCX_VOSCCLK_120MHz,
	NPCX_VOSCCLK_90MHz,
};

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

/* GLUE register fields */
/* PSL input detection mode is configured by bits 7:4 of PSL_CTS */
#define NPCX_PSL_CTS_MODE_BIT(bit) BIT(bit + 4)
/* PSL input assertion events are reported by bits 3:0 of PSL_CTS */
#define NPCX_PSL_CTS_EVENT_BIT(bit) BIT(bit)

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
	volatile uint8_t reserved8[17];
	/* 0x020: FIFO Mode Transmit Status */
	volatile uint8_t UFTSTS;
	volatile uint8_t reserved9;
	/* 0x022: FIFO Mode Receive Status */
	volatile uint8_t UFRSTS;
	volatile uint8_t reserved10;
	/* 0x024: FIFO Mode Transmit Control */
	volatile uint8_t UFTCTL;
	volatile uint8_t reserved11;
	/* 0x026: FIFO Mode Receive Control */
	volatile uint8_t UFRCTL;
};

/* UART register fields */
#define NPCX_UICTRL_TBE                       0
#define NPCX_UICTRL_RBF                       1
#define NPCX_UICTRL_ETI                       5
#define NPCX_UICTRL_ERI                       6
#define NPCX_UICTRL_EEI                       7
#define NPCX_USTAT_PE                         0
#define NPCX_USTAT_FE                         1
#define NPCX_USTAT_DOE                        2
#define NPCX_USTAT_ERR                        3
#define NPCX_USTAT_BKD                        4
#define NPCX_USTAT_RB9                        5
#define NPCX_USTAT_XMIP                       6
#define NPCX_UFRS_CHAR_FIELD                  FIELD(0, 2)
#define NPCX_UFRS_STP                         2
#define NPCX_UFRS_XB9                         3
#define NPCX_UFRS_PSEL_FIELD                  FIELD(4, 2)
#define NPCX_UFRS_PEN                         6
#define NPCX_UMDSL_FIFO_MD                    0
#define NPCX_UMDSL_ETD                        4
#define NPCX_UMDSL_ERD                        5

#define NPCX_UFTSTS_TEMPTY_LVL                FIELD(0, 5)
#define NPCX_UFTSTS_TEMPTY_LVL_STS            5
#define NPCX_UFTSTS_TFIFO_EMPTY_STS           6
#define NPCX_UFTSTS_NXMIP                     7
#define NPCX_UFRSTS_RFULL_LVL_STS             5
#define NPCX_UFRSTS_RFIFO_NEMPTY_STS          6
#define NPCX_UFRSTS_ERR                       7
#define NPCX_UFTCTL_TEMPTY_LVL_SEL            FIELD(0, 5)
#define NPCX_UFTCTL_TEMPTY_LVL_EN             5
#define NPCX_UFTCTL_TEMPTY_EN                 6
#define NPCX_UFTCTL_NXMIP_EN                  7
#define NPCX_UFRCTL_RFULL_LVL_SEL             FIELD(0, 5)
#define NPCX_UFRCTL_RFULL_LVL_EN              5
#define NPCX_UFRCTL_RNEMPTY_EN                6
#define NPCX_UFRCTL_ERR_EN                    7

/* Macro functions for MIWU multi-registers */
#define NPCX_WKEDG(base, group) \
	(*(volatile uint8_t *)(base +  NPCX_WKEDG_OFFSET(group)))
#define NPCX_WKAEDG(base, group) \
	(*(volatile uint8_t *)(base + NPCX_WKAEDG_OFFSET(group)))
#define NPCX_WKPND(base, group) \
	(*(volatile uint8_t *)(base + NPCX_WKPND_OFFSET(group)))
#define NPCX_WKPCL(base, group) \
	(*(volatile uint8_t *)(base + NPCX_WKPCL_OFFSET(group)))
#define NPCX_WKEN(base, group) \
	(*(volatile uint8_t *)(base + NPCX_WKEN_OFFSET(group)))
#define NPCX_WKINEN(base, group) \
	(*(volatile uint8_t *)(base + NPCX_WKINEN_OFFSET(group)))
#define NPCX_WKMOD(base, group) \
	(*(volatile uint8_t *)(base + NPCX_WKMOD_OFFSET(group)))
#define NPCX_WKST(base, group) \
	(*(volatile uint8_t *)(base + NPCX_WKST_OFFSET(group)))

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
	/* 0x007: Port GPIOx Lock Control */
	volatile uint8_t PLOCK_CTL;
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
#define NPCX_PWMCTL_INVP                      0
#define NPCX_PWMCTL_CKSEL                     1
#define NPCX_PWMCTL_HB_DC_CTL_FIELD           FIELD(2, 2)
#define NPCX_PWMCTL_PWR                       7
#define NPCX_PWMCTLEX_FCK_SEL_FIELD           FIELD(4, 2)
#define NPCX_PWMCTLEX_OD_OUT                  7

/*
 * Analog-To-Digital Converter (ADC) device registers
 */
struct adc_reg {
	/* 0x000: ADC Status */
	volatile uint16_t ADCSTS;
	/* 0x002: ADC Configuration */
	volatile uint16_t ADCCNF;
	/* 0x004: ADC Timing Control */
	volatile uint16_t ATCTL;
	/* 0x006: ADC Single Channel Address */
	volatile uint16_t ASCADD;
	/* 0x008: ADC Scan Channels Select */
	volatile uint16_t ADCCS;
	/* 0x00A: ADC Scan Channels Select 2 */
	volatile uint16_t ADCCS2;
	volatile uint8_t reserved1[14];
	/* 0x01A:  Threshold Status */
	volatile uint16_t THRCTS;
	volatile uint8_t reserved2[4];
	/* 0x020: Internal register 1 for ADC Speed */
	volatile uint16_t ADCCNF2;
	/* 0x022: Internal register 2 for ADC Speed */
	volatile uint16_t GENDLY;
	volatile uint8_t reserved3[2];
	/* 0x026: Internal register 3 for ADC Speed */
	volatile uint16_t MEAST;
};

/* ADC internal inline functions for multi-registers */
#define CHNDAT(base, ch) \
	(*(volatile uint16_t *)((base) + NPCX_CHNDAT_OFFSET(ch)))
#define THRCTL(base, ctrl) \
	(*(volatile uint16_t *)(base + NPCX_THRCTL_OFFSET(ctrl)))

/* ADC register fields */
#define NPCX_ATCTL_SCLKDIV_FIELD              FIELD(0, 6)
#define NPCX_ATCTL_DLY_FIELD                  FIELD(8, 3)
#define NPCX_ASCADD_SADDR_FIELD               FIELD(0, 5)
#define NPCX_ADCSTS_EOCEV                     0
#define NPCX_ADCSTS_EOCCEV                    1
#define NPCX_ADCCNF_ADCEN                     0
#define NPCX_ADCCNF_ADCMD_FIELD               FIELD(1, 2)
#define NPCX_ADCCNF_ADCRPTC                   3
#define NPCX_ADCCNF_START                     4
#define NPCX_ADCCNF_ADCTTE                    5
#define NPCX_ADCCNF_INTECEN                   6
#define NPCX_ADCCNF_INTECCEN                  7
#define NPCX_ADCCNF_INTETCEN                  8
#define NPCX_ADCCNF_INTOVFEN                  9
#define NPCX_ADCCNF_STOP                      11
#define NPCX_CHNDAT_CHDAT_FIELD               FIELD(0, 10)
#define NPCX_CHNDAT_NEW                       15
#define NPCX_THRCTS_ADC_WKEN                  15
#define NPCX_THRCTS_THR3_IEN                  10
#define NPCX_THRCTS_THR2_IEN                  9
#define NPCX_THRCTS_THR1_IEN                  8
#define NPCX_THRCTS_ADC_EVENT                 7
#define NPCX_THRCTS_THR3_STS                  2
#define NPCX_THRCTS_THR2_STS                  1
#define NPCX_THRCTS_THR1_STS                  0
#define NPCX_THR_DCTL_THRD_EN                 15
#define NPCX_THR_DCTL_THR_DVAL                FIELD(0, 10)

/*
 * Timer Watchdog (TWD) device registers
 */
struct twd_reg {
	/* 0x000: Timer and Watchdog Configuration */
	volatile uint8_t TWCFG;
	volatile uint8_t reserved1;
	/* 0x002: Timer and Watchdog Clock Prescaler */
	volatile uint8_t TWCP;
	volatile uint8_t reserved2;
	/* 0x004: TWD Timer 0 */
	volatile uint16_t TWDT0;
	/* 0x006: TWDT0 Control and Status */
	volatile uint8_t T0CSR;
	volatile uint8_t reserved3;
	/* 0x008: Watchdog Count */
	volatile uint8_t WDCNT;
	volatile uint8_t reserved4;
	/* 0x00A: Watchdog Service Data Match */
	volatile uint8_t WDSDM;
	volatile uint8_t reserved5;
	/* 0x00C: TWD Timer 0 Counter */
	volatile uint16_t TWMT0;
	/* 0x00E: Watchdog Counter */
	volatile uint8_t TWMWD;
	volatile uint8_t reserved6;
	/* 0x010: Watchdog Clock Prescaler */
	volatile uint8_t WDCP;
	volatile uint8_t reserved7;
};

/* TWD register fields */
#define NPCX_TWCFG_LTWCFG                      0
#define NPCX_TWCFG_LTWCP                       1
#define NPCX_TWCFG_LTWDT0                      2
#define NPCX_TWCFG_LWDCNT                      3
#define NPCX_TWCFG_WDCT0I                      4
#define NPCX_TWCFG_WDSDME                      5
#define NPCX_T0CSR_RST                         0
#define NPCX_T0CSR_TC                          1
#define NPCX_T0CSR_WDLTD                       3
#define NPCX_T0CSR_WDRST_STS                   4
#define NPCX_T0CSR_WD_RUN                      5
#define NPCX_T0CSR_TESDIS                      7

/*
 * Enhanced Serial Peripheral Interface (eSPI) device registers
 */
struct espi_reg {
	/* 0x000: eSPI Identification */
	volatile uint32_t ESPIID;
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
	volatile uint32_t reserved1;
	/* 0x034: Flash Channel Configuration */
	volatile uint32_t FLASHCFG;
	/* 0x038: Flash Channel Control */
	volatile uint32_t FLASHCTL;
	/* 0x03C: eSPI Error Status */
	volatile uint32_t ESPIERR;
	/* 0x040: Peripheral Bus Master Receive Buffer Read Head */
	volatile uint32_t PBMRXRDHEAD;
	/* 0x044: Peripheral Bus Master Transmit Buffer Write Head */
	volatile uint32_t PBMTXWRHEAD;
	/* 0x048: Peripheral Channel Configuration */
	volatile uint32_t PERCFG;
	/* 0x04C: Peripheral Channel Control */
	volatile uint32_t PERCTL;
	/* 0x050: Status Image Register */
	volatile uint16_t STATUS_IMG;
	volatile uint16_t reserved2[79];
	/* 0x0F0: NPCX specific eSPI Register1 */
	volatile uint8_t NPCX_ONLY_ESPI_REG1;
	/* 0x0F1: NPCX specific eSPI Register2 */
	volatile uint8_t NPCX_ONLY_ESPI_REG2;
	volatile uint16_t reserved3[7];
	/* 0x100 - 127: Virtual Wire Event Slave-to-Master 0 - 9 */
	volatile uint32_t VWEVSM[10];
	volatile uint32_t reserved4[6];
	/* 0x140 - 16F: Virtual Wire Event Master-to-Slave 0 - 11 */
	volatile uint32_t VWEVMS[12];
	volatile uint32_t reserved5[4];
	/* 0x180 - 1BF: Virtual Wire GPIO Event Master-to-Slave 0 - 15 */
	volatile uint32_t VWGPSM[16];
	volatile uint32_t reserved6[79];
	/* 0x2FC: Virtual Wire Channel Control */
	volatile uint32_t VWCTL;
	/* 0x300 - 34F: OOB Receive Buffer 0 - 19 */
	volatile uint32_t OOBRXBUF[20];
	volatile uint32_t reserved7[12];
	/* 0x380 - 3CF: OOB Transmit Buffer 0-19 */
	volatile uint32_t OOBTXBUF[20];
	volatile uint32_t reserved8[11];
	/* 0x3FC: OOB Channel Control used in 'direct' mode */
	volatile uint32_t OOBCTL_DIRECT;
	/* 0x400 - 443: Flash Receive Buffer 0-17 */
	volatile uint32_t FLASHRXBUF[18];
	volatile uint32_t reserved9[14];
	/* 0x480 - 497: Flash Transmit Buffer 0-16 */
	volatile uint32_t FLASHTXBUF[17];
	volatile uint32_t reserved10[14];
	/* 0x4FC: Flash Channel Control used in 'direct' mode */
	volatile uint32_t FLASHCTL_DIRECT;
	volatile uint32_t reserved12[64];
	/* 0x600 - 63F */
	volatile uint32_t FLASH_PRTR_BADDR[16];
	/* 0x640 - 67F */
	volatile uint32_t FLASH_PRTR_HADDR[16];
	/* 0x680 - 6BF */
	volatile uint32_t FLASH_RGN_TAG_OVR[16];
	volatile uint32_t reserved13[80];
	/* 0x800 */
	volatile uint32_t FLASH_RPMC_CFG_1;
	/* 0x804 */
	volatile uint32_t FLASH_RPMC_CFG_2;
	/* 0x808 */
	volatile uint32_t RMAP_FLASH_OFFS;
	/* 0x80C */
	volatile uint32_t RMAP_DST_BASE;
	/* 0x810 */
	volatile uint32_t RMAP_WIN_SIZE;
	/* 0x814 */
	volatile uint32_t FLASHBASE;
	volatile uint32_t reserved14[58];
};

/* eSPI register fields */
#define NPCX_ESPICFG_PCHANEN             0
#define NPCX_ESPICFG_VWCHANEN            1
#define NPCX_ESPICFG_OOBCHANEN           2
#define NPCX_ESPICFG_FLASHCHANEN         3
#define NPCX_ESPICFG_HPCHANEN            4
#define NPCX_ESPICFG_HVWCHANEN           5
#define NPCX_ESPICFG_HOOBCHANEN          6
#define NPCX_ESPICFG_HFLASHCHANEN        7
#define NPCX_ESPICFG_CHANS_FIELD         FIELD(0, 4)
#define NPCX_ESPICFG_HCHANS_FIELD        FIELD(4, 4)
#define NPCX_ESPICFG_IOMODE_FIELD        FIELD(8, 2)
#define NPCX_ESPICFG_MAXFREQ_FIELD       FIELD(10, 3)
#define NPCX_ESPICFG_FLCHANMODE          16
#define NPCX_ESPICFG_PCCHN_SUPP          24
#define NPCX_ESPICFG_VWCHN_SUPP          25
#define NPCX_ESPICFG_OOBCHN_SUPP         26
#define NPCX_ESPICFG_FLASHCHN_SUPP       27
#define NPCX_ESPIIE_IBRSTIE              0
#define NPCX_ESPIIE_CFGUPDIE             1
#define NPCX_ESPIIE_BERRIE               2
#define NPCX_ESPIIE_OOBRXIE              3
#define NPCX_ESPIIE_FLASHRXIE            4
#define NPCX_ESPIIE_FLNACSIE             5
#define NPCX_ESPIIE_PERACCIE             6
#define NPCX_ESPIIE_DFRDIE               7
#define NPCX_ESPIIE_VWUPDIE              8
#define NPCX_ESPIIE_ESPIRSTIE            9
#define NPCX_ESPIIE_PLTRSTIE             10
#define NPCX_ESPIIE_AMERRIE              15
#define NPCX_ESPIIE_AMDONEIE             16
#define NPCX_ESPIIE_BMTXDONEIE           19
#define NPCX_ESPIIE_PBMRXIE              20
#define NPCX_ESPIIE_PMSGRXIE             21
#define NPCX_ESPIIE_BMBURSTERRIE         22
#define NPCX_ESPIIE_BMBURSTDONEIE        23
#define NPCX_ESPIWE_IBRSTWE              0
#define NPCX_ESPIWE_CFGUPDWE             1
#define NPCX_ESPIWE_BERRWE               2
#define NPCX_ESPIWE_OOBRXWE              3
#define NPCX_ESPIWE_FLASHRXWE            4
#define NPCX_ESPIWE_FLNACSWE             5
#define NPCX_ESPIWE_PERACCWE             6
#define NPCX_ESPIWE_DFRDWE               7
#define NPCX_ESPIWE_VWUPDWE              8
#define NPCX_ESPIWE_ESPIRSTWE            9
#define NPCX_ESPIWE_PBMRXWE              20
#define NPCX_ESPIWE_PMSGRXWE             21
#define NPCX_ESPISTS_IBRST               0
#define NPCX_ESPISTS_CFGUPD              1
#define NPCX_ESPISTS_BERR                2
#define NPCX_ESPISTS_OOBRX               3
#define NPCX_ESPISTS_FLASHRX             4
#define NPCX_ESPISTS_FLNACS              5
#define NPCX_ESPISTS_PERACC              6
#define NPCX_ESPISTS_DFRD                7
#define NPCX_ESPISTS_VWUPD               8
#define NPCX_ESPISTS_ESPIRST             9
#define NPCX_ESPISTS_PLTRST              10
#define NPCX_ESPISTS_AMERR               15
#define NPCX_ESPISTS_AMDONE              16
#define NPCX_ESPISTS_VWUPDW              17
#define NPCX_ESPISTS_BMTXDONE            19
#define NPCX_ESPISTS_PBMRX               20
#define NPCX_ESPISTS_PMSGRX              21
#define NPCX_ESPISTS_BMBURSTERR          22
#define NPCX_ESPISTS_BMBURSTDONE         23
#define NPCX_ESPISTS_ESPIRST_LVL         24
#define NPCX_VWSWIRQ_IRQ_NUM             FIELD(0, 7)
#define NPCX_VWSWIRQ_IRQ_LVL             7
#define NPCX_VWSWIRQ_INDEX               FIELD(8, 7)
#define NPCX_VWSWIRQ_INDEX_EN            15
#define NPCX_VWSWIRQ_DIRTY               16
#define NPCX_VWSWIRQ_ENPLTRST            17
#define NPCX_VWSWIRQ_ENCDRST             19
#define NPCX_VWSWIRQ_EDGE_IRQ            28
#define NPCX_VWEVMS_WIRE                 FIELD(0, 4)
#define NPCX_VWEVMS_VALID                FIELD(4, 4)
#define NPCX_VWEVMS_INDEX                FIELD(8, 7)
#define NPCX_VWEVMS_INDEX_EN             15
#define NPCX_VWEVMS_IE                   18
#define NPCX_VWEVMS_WE                   20
#define NPCX_VWEVSM_WIRE                 FIELD(0, 4)
#define NPCX_VWEVSM_VALID                FIELD(4, 4)
#define NPCX_VWEVSM_INDEX                FIELD(8, 7)
#define NPCX_VWEVSM_INDEX_EN             15
#define NPCX_VWEVSM_BIT_VALID(n)         (4+n)
#define NPCX_VWEVSM_HW_WIRE              FIELD(24, 4)
#define NPCX_VWGPSM_INDEX_EN             15
#define NPCX_OOBCTL_OOB_FREE             0
#define NPCX_OOBCTL_OOB_AVAIL            1
#define NPCX_OOBCTL_RSTBUFHEADS          2
#define NPCX_OOBCTL_OOBPLSIZE            FIELD(10, 3)
#define NPCX_FLASHCFG_FLASHBLERSSIZE     FIELD(7, 3)
#define NPCX_FLASHCFG_FLASHPLSIZE        FIELD(10, 3)
#define NPCX_FLASHCFG_FLASHREQSIZE       FIELD(13, 3)
#define NPCX_FLASHCFG_FLCAPA             FIELD(24, 2)
#define NPCX_FLASHCFG_TRGFLEBLKSIZE      FIELD(16, 8)
#define NPCX_FLASHCFG_FLREQSUP           FIELD(0, 3)
#define NPCX_FLASHCTL_FLASH_NP_FREE      0
#define NPCX_FLASHCTL_FLASH_TX_AVAIL     1
#define NPCX_FLASHCTL_STRPHDR            2
#define NPCX_FLASHCTL_DMATHRESH          FIELD(3, 2)
#define NPCX_FLASHCTL_AMTSIZE            FIELD(5, 8)
#define NPCX_FLASHCTL_RSTBUFHEADS        13
#define NPCX_FLASHCTL_CRCEN              14
#define NPCX_FLASHCTL_CHKSUMSEL          15
#define NPCX_FLASHCTL_AMTEN              16
#define NPCX_FLASHCTL_SAF_AUTO_READ      18
#define NPCX_FLASHCTL_AUTO_RD_DIS_CTL    19
#define NPCX_FLASHCTL_BLK_FLASH_NP_FREE  20
#define NPCX_FLASHBASE_FLBASE_ADDR       FIELD(12, 15)
#define NPCX_FLASH_PRTR_BADDR            FIELD(12, 15)
#define NPCX_FRGN_WPR                    29
#define SAF_PROT_LCK                     31
#define NPCX_FRGN_RPR                    30
#define NPCX_FLASH_PRTR_HADDR            FIELD(12, 15)
#define NPCX_FLASH_TAG_OVR_RPR           FIELD(16, 16)
#define NPCX_FLASH_TAG_OVR_WPR           FIELD(0, 16)
#define NPCX_FLASH_RPMC_CFG1_CNTR        FIELD(0, 4)
#define NPCX_FLASH_RPMC_CFG1_OP1         FIELD(4, 8)
#define NPCX_FLASH_RPMC_CFG1_TRGRPMCSUP  FIELD(26, 6)
#define NPCX_ONLY_ESPI_REG1_UNLOCK_REG2         0x55
#define NPCX_ONLY_ESPI_REG1_LOCK_REG2           0
#define NPCX_ONLY_ESPI_REG2_TRANS_END_CONFIG    4

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
	volatile uint8_t reserved10[5];
	/* 0x01C: SRID Core Access */
	volatile uint8_t SRID_CR;
	volatile uint8_t reserved11[3];
	/* 0x020: SID Core Access */
	volatile uint8_t SID_CR;
	volatile uint8_t reserved12;
	/* 0x022: DEVICE_ID Core Access */
	volatile uint8_t DEVICE_ID_CR;
	volatile uint8_t reserved13[5];
	/* 0x028: Chip Revision Core Access */
	volatile uint8_t CHPREV_CR;
	volatile uint8_t reserved14[5];
	/* 0x02E: Virtual Wire Sleep States */
	volatile uint8_t VW_SLPST1;
	volatile uint8_t reserved15;
};

/* MSWC register fields */
#define NPCX_MSWCTL1_HRSTOB              0
#define NPCS_MSWCTL1_HWPRON              1
#define NPCX_MSWCTL1_PLTRST_ACT          2
#define NPCX_MSWCTL1_VHCFGA              3
#define NPCX_MSWCTL1_HCFGLK              4
#define NPCX_MSWCTL1_PWROFFB             6
#define NPCX_MSWCTL1_A20MB               7

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
	/* 0x020: Shared Access Window 1 Base */
	volatile uint32_t WIN_BASE1;
	/* 0x024: Shared Access Window 2 Base */
	volatile uint32_t WIN_BASE2;
	volatile uint32_t reserved7;
	/* 0x02C: Indirect Memory Access Base */
	volatile uint32_t IMA_BASE;
	volatile uint8_t reserved8[10];
	/* 0x03A: Reset Configuration */
	volatile uint8_t RST_CFG;
	volatile uint8_t reserved9[5];
	/* 0x040: Debug Port 80 Buffered Data */
	volatile uint16_t DP80BUF;
	/* 0x042: Debug Port 80 Status */
	volatile uint8_t DP80STS;
	volatile uint8_t reserved10;
	/* 0x044: Debug Port 80 Control */
	volatile uint8_t DP80CTL;
	volatile uint8_t reserved11[3];
	/* 0x048: Host_Offset in Windows 1, 2 Status */
	volatile uint8_t HOFS_STS;
	/* 0x049: Host_Offset in Windows 1, 2 Control */
	volatile uint8_t HOFS_CTL;
	/* 0x04A: Core_Offset in Window 2 Address */
	volatile uint16_t COFS2;
	/* 0x04C: Core_Offset in Window 1 Address */
	volatile uint16_t COFS1;
	volatile uint16_t reserved12;
};

/* SHM register fields */
#define NPCX_SMC_STS_HRERR               0
#define NPCX_SMC_STS_HWERR               1
#define NPCX_SMC_STS_HSEM1W              4
#define NPCX_SMC_STS_HSEM2W              5
#define NPCX_SMC_STS_SHM_ACC             6
#define NPCX_SMC_CTL_HERR_IE             2
#define NPCX_SMC_CTL_HSEM1_IE            3
#define NPCX_SMC_CTL_HSEM2_IE            4
#define NPCX_SMC_CTL_ACC_IE              5
#define NPCX_SMC_CTL_PREF_EN             6
#define NPCX_SMC_CTL_HOSTWAIT            7
#define NPCX_FLASH_SIZE_STALL_HOST       6
#define NPCX_FLASH_SIZE_RD_BURST         7
#define NPCX_WIN_SIZE_RWIN1_SIZE_FIELD   FIELD(0, 4)
#define NPCX_WIN_SIZE_RWIN2_SIZE_FIELD   FIELD(4, 4)
#define NPCX_WIN_PROT_RW1L_RP            0
#define NPCX_WIN_PROT_RW1L_WP            1
#define NPCX_WIN_PROT_RW1H_RP            2
#define NPCX_WIN_PROT_RW1H_WP            3
#define NPCX_WIN_PROT_RW2L_RP            4
#define NPCX_WIN_PROT_RW2L_WP            5
#define NPCX_WIN_PROT_RW2H_RP            6
#define NPCX_WIN_PROT_RW2H_WP            7
#define NPCX_PWIN_SIZEI_RPROT            13
#define NPCX_PWIN_SIZEI_WPROT            14
#define NPCX_CSEM2                       6
#define NPCX_CSEM3                       7
#define NPCX_DP80STS_FWR                 5
#define NPCX_DP80STS_FNE                 6
#define NPCX_DP80STS_FOR                 7
#define NPCX_DP80CTL_DP80EN              0
#define NPCX_DP80CTL_SYNCEN              1
#define NPCX_DP80CTL_ADV                 2
#define NPCX_DP80CTL_RAA                 3
#define NPCX_DP80CTL_RFIFO               4
#define NPCX_DP80CTL_CIEN                5
#define NPCX_DP80CTL_DP80_HF_CFG         7
#define NPCX_DP80BUF_OFFS_FIELD          FIELD(8, 3)

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
#define NPCX_HICTRL_OBFKIE               0
#define NPCX_HICTRL_OBFMIE               1
#define NPCX_HICTRL_OBECIE               2
#define NPCX_HICTRL_IBFCIE               3
#define NPCX_HICTRL_PMIHIE               4
#define NPCX_HICTRL_PMIOCIE              5
#define NPCX_HICTRL_PMICIE               6
#define NPCX_HICTRL_FW_OBF               7
#define NPCX_HIKMST_OBF                  0
#define NPCX_HIKMST_IBF                  1
#define NPCX_HIKMST_F0                   2
#define NPCX_HIKMST_A2                   3
#define NPCX_HIKMST_ST0                  4
#define NPCX_HIKMST_ST1                  5
#define NPCX_HIKMST_ST2                  6
#define NPCX_HIKMST_ST3                  7

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
	volatile uint8_t reserved7;
};

/* PMCH register field */
#define NPCX_HIPMIE_SCIE                 1
#define NPCX_HIPMIE_SMIE                 2
#define NPCX_HIPMCTL_IBFIE               0
#define NPCX_HIPMCTL_OBEIE               1
#define NPCX_HIPMCTL_SCIPOL              6
#define NPCX_HIPMST_OBF                  0
#define NPCX_HIPMST_IBF                  1
#define NPCX_HIPMST_F0                   2
#define NPCX_HIPMST_CMD                  3
#define NPCX_HIPMST_ST0                  4
#define NPCX_HIPMST_ST1                  5
#define NPCX_HIPMST_ST2                  6
#define NPCX_HIPMIC_SMIB                 1
#define NPCX_HIPMIC_SCIB                 2
#define NPCX_HIPMIC_SMIPOL               6

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
	volatile uint8_t reserved3;
};

/* C2H register fields */
#define NPCX_LKSIOHA_LKCFG               0
#define NPCX_LKSIOHA_LKSPHA              2
#define NPCX_LKSIOHA_LKHIKBD             11
#define NPCX_CRSMAE_CFGAE                0
#define NPCX_CRSMAE_HIKBDAE              11
#define NPCX_SIOLV_SPLV                  2
#define NPCX_SIBCTRL_CSAE                0
#define NPCX_SIBCTRL_CSRD                1
#define NPCX_SIBCTRL_CSWR                2

/*
 * SMBUS (SMB) device registers
 */
struct smb_reg {
	/* 0x000: SMB Serial Data */
	volatile uint8_t SMBSDA;
	volatile uint8_t reserved1;
	/* 0x002: SMB Status */
	volatile uint8_t SMBST;
	volatile uint8_t reserved2;
	/* 0x004: SMB Control Status */
	volatile uint8_t SMBCST;
	volatile uint8_t reserved3;
	/* 0x006: SMB Control 1 */
	volatile uint8_t SMBCTL1;
	volatile uint8_t reserved4;
	/* 0x008: SMB Own Address */
	volatile uint8_t SMBADDR1;
	volatile uint8_t reserved5;
	/* 0x00A: SMB Control 2 */
	volatile uint8_t SMBCTL2;
	volatile uint8_t reserved6;
	/* 0x00C: SMB Own Address */
	volatile uint8_t SMBADDR2;
	volatile uint8_t reserved7;
	/* 0x00E: SMB Control 3 */
	volatile uint8_t SMBCTL3;
	/* 0x00F: SMB Bus Timeout */
	volatile uint8_t SMBT_OUT;
	union {
		/* Bank 0 */
		struct {
			/* 0x010: SMB Own Address 3 */
			volatile uint8_t SMBADDR3;
			/* 0x011: SMB Own Address 7 */
			volatile uint8_t SMBADDR7;
			/* 0x012: SMB Own Address 4 */
			volatile uint8_t SMBADDR4;
			/* 0x013: SMB Own Address 8 */
			volatile uint8_t SMBADDR8;
			/* 0x014: SMB Own Address 5 */
			volatile uint8_t SMBADDR5;
			volatile uint8_t reserved8;
			/* 0x016: SMB Own Address 6 */
			volatile uint8_t SMBADDR6;
			volatile uint8_t reserved9;
			/* 0x018: SMB Control Status 2 */
			volatile uint8_t SMBCST2;
			/* 0x019: SMB Control Status 3 */
			volatile uint8_t SMBCST3;
			/* 0x01A: SMB Control 4 */
			volatile uint8_t SMBCTL4;
			volatile uint8_t reserved10;
			/* 0x01C: SMB SCL Low Time */
			volatile uint8_t SMBSCLLT;
			/* 0x01D: SMB FIFO Control */
			volatile uint8_t SMBFIF_CTL;
			/* 0x01E: SMB SCL High Time */
			volatile uint8_t SMBSCLHT;
			volatile uint8_t reserved11;
		};
		/* Bank 1 */
		struct {
			/* 0x010: SMB FIFO Control */
			volatile uint8_t SMBFIF_CTS;
			volatile uint8_t reserved12;
			/* 0x012: SMB Tx-FIFO Control */
			volatile uint8_t SMBTXF_CTL;
			volatile uint8_t reserved13;
			/* 0x014: SMB Bus Timeout */
			volatile uint8_t SMB_T_OUT;
			volatile uint8_t reserved14[3];
			/* 0x018: SMB Control Status 2 (FIFO) */
			volatile uint8_t SMBCST2_FIFO;
			/* 0x019: SMB Control Status 3 (FIFO) */
			volatile uint8_t SMBCST3_FIFO;
			/* 0x01A: SMB Tx-FIFO Status */
			volatile uint8_t SMBTXF_STS;
			volatile uint8_t reserved15;
			/* 0x01C: SMB Rx-FIFO Status */
			volatile uint8_t SMBRXF_STS;
			volatile uint8_t reserved16;
			/* 0x01E: SMB Rx-FIFO Control */
			volatile uint8_t SMBRXF_CTL;
			volatile uint8_t reserved17[1];
		};
	};
};

/* SMB register fields */
#define NPCX_SMBST_XMIT                  0
#define NPCX_SMBST_MASTER                1
#define NPCX_SMBST_NMATCH                2
#define NPCX_SMBST_STASTR                3
#define NPCX_SMBST_NEGACK                4
#define NPCX_SMBST_BER                   5
#define NPCX_SMBST_SDAST                 6
#define NPCX_SMBST_SLVSTP                7
#define NPCX_SMBCST_BUSY                 0
#define NPCX_SMBCST_BB                   1
#define NPCX_SMBCST_MATCH                2
#define NPCX_SMBCST_GCMATCH              3
#define NPCX_SMBCST_TSDA                 4
#define NPCX_SMBCST_TGSCL                5
#define NPCX_SMBCST_MATCHAF              6
#define NPCX_SMBCST_ARPMATCH             7
#define NPCX_SMBCST2_MATCHA1F            0
#define NPCX_SMBCST2_MATCHA2F            1
#define NPCX_SMBCST2_MATCHA3F            2
#define NPCX_SMBCST2_MATCHA4F            3
#define NPCX_SMBCST2_MATCHA5F            4
#define NPCX_SMBCST2_MATCHA6F            5
#define NPCX_SMBCST2_MATCHA7F            6
#define NPCX_SMBCST2_INTSTS              7
#define NPCX_SMBCST3_MATCHA8F            0
#define NPCX_SMBCST3_MATCHA9F            1
#define NPCX_SMBCST3_MATCHA10F           2
#define NPCX_SMBCTL1_START               0
#define NPCX_SMBCTL1_STOP                1
#define NPCX_SMBCTL1_INTEN               2
#define NPCX_SMBCTL1_ACK                 4
#define NPCX_SMBCTL1_GCMEN               5
#define NPCX_SMBCTL1_NMINTE              6
#define NPCX_SMBCTL1_STASTRE             7
#define NPCX_SMBCTL2_ENABLE              0
#define NPCX_SMBCTL2_SCLFRQ0_6_FIELD     FIELD(1, 7)
#define NPCX_SMBCTL3_ARPMEN              2
#define NPCX_SMBCTL3_SCLFRQ7_8_FIELD     FIELD(0, 2)
#define NPCX_SMBCTL3_IDL_START           3
#define NPCX_SMBCTL3_400K                4
#define NPCX_SMBCTL3_BNK_SEL             5
#define NPCX_SMBCTL3_SDA_LVL             6
#define NPCX_SMBCTL3_SCL_LVL             7
#define NPCX_SMBCTL4_HLDT_FIELD          FIELD(0, 6)
#define NPCX_SMBCTL4_LVL_WE              7
#define NPCX_SMBADDR1_SAEN               7
#define NPCX_SMBADDR2_SAEN               7
#define NPCX_SMBADDR3_SAEN               7
#define NPCX_SMBADDR4_SAEN               7
#define NPCX_SMBADDR5_SAEN               7
#define NPCX_SMBADDR6_SAEN               7
#define NPCX_SMBADDR7_SAEN               7
#define NPCX_SMBADDR8_SAEN               7
#define NPCX_SMBSEL_SMB4SEL              4
#define NPCX_SMBSEL_SMB5SEL              5
#define NPCX_SMBSEL_SMB6SEL              6
#define NPCX_SMBFIF_CTS_RXF_TXE          1
#define NPCX_SMBFIF_CTS_CLR_FIFO         6
#define NPCX_SMBFIF_CTL_FIFO_EN          4
#define NPCX_SMBRXF_STS_RX_THST          6

/* RX FIFO threshold */
#define NPCX_SMBRXF_CTL_RX_THR           FIELD(0, 6)
#define NPCX_SMBRXF_CTL_LAST             7

/*
 * Internal 32-bit Timer (ITIM32) device registers
 */
struct itim32_reg {
	volatile uint8_t reserved1;
	/* 0x001: Internal 32-bit Timer Prescaler */
	volatile uint8_t ITPRE32;
	volatile uint8_t reserved2[2];
	/* 0x004: Internal 32-bit Timer Control and Status */
	volatile uint8_t ITCTS32;
	volatile uint8_t reserved3[3];
	/* 0x008: Internal 32-Bit Timer Counter */
	volatile uint32_t ITCNT32;
};

/*
 * Internal 64-bit Timer (ITIM54) device registers
 */
struct itim64_reg {
	volatile uint8_t reserved1;
	/* 0x001: Internal 64-bit Timer Prescaler */
	volatile uint8_t ITPRE64;
	volatile uint8_t reserved2[2];
	/* 0x004: Internal 64-bit Timer Control and Status */
	volatile uint8_t ITCTS64;
	volatile uint8_t reserved3[3];
	/* 0x008: Internal 32-Bit Timer Counter */
	volatile uint32_t ITCNT64L;
	/* 0x00C: Internal 32-Bit Timer Counter */
	volatile uint32_t ITCNT64H;
};

/* ITIM register fields */
#define NPCX_ITCTSXX_TO_STS              0
#define NPCX_ITCTSXX_TO_IE               2
#define NPCX_ITCTSXX_TO_WUE              3
#define NPCX_ITCTSXX_CKSEL               4
#define NPCX_ITCTSXX_ITEN                7

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
	/* 0x01A: Timer Wake-Up Enable */
	volatile uint8_t TWUEN;
	volatile uint8_t reserved8;
	/* 0x01C: Timer Configuration */
	volatile uint8_t TCFG;
	volatile uint8_t reserved9;
};

/* TACH register fields */
#define NPCX_TCKC_LOW_PWR                7
#define NPCX_TCKC_PLS_ACC_CLK            6
#define NPCX_TCKC_C1CSEL_FIELD           FIELD(0, 3)
#define NPCX_TCKC_C2CSEL_FIELD           FIELD(3, 3)
#define NPCX_TMCTRL_MDSEL_FIELD          FIELD(0, 3)
#define NPCX_TMCTRL_TAEN                 5
#define NPCX_TMCTRL_TBEN                 6
#define NPCX_TMCTRL_TAEDG                3
#define NPCX_TMCTRL_TBEDG                4
#define NPCX_TCFG_TADBEN                 6
#define NPCX_TCFG_TBDBEN                 7
#define NPCX_TECTRL_TAPND                0
#define NPCX_TECTRL_TBPND                1
#define NPCX_TECTRL_TCPND                2
#define NPCX_TECTRL_TDPND                3
#define NPCX_TECLR_TACLR                 0
#define NPCX_TECLR_TBCLR                 1
#define NPCX_TECLR_TCCLR                 2
#define NPCX_TECLR_TDCLR                 3
#define NPCX_TIEN_TAIEN                  0
#define NPCX_TIEN_TBIEN                  1
#define NPCX_TIEN_TCIEN                  2
#define NPCX_TIEN_TDIEN                  3
#define NPCX_TWUEN_TAWEN                 0
#define NPCX_TWUEN_TBWEN                 1
#define NPCX_TWUEN_TCWEN                 2
#define NPCX_TWUEN_TDWEN                 3

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
#define NPCX_DBGFRZEN3_GLBL_FRZ_DIS      7

/* PS/2 Interface registers */
struct ps2_reg {
	/* 0x000: PS/2 Data */
	volatile uint8_t PSDAT;
	volatile uint8_t reserved1;
	/* 0x002: PS/2 Status */
	volatile uint8_t PSTAT;
	volatile uint8_t reserved2;
	/* 0x004: PS/2 Control */
	volatile uint8_t PSCON;
	volatile uint8_t reserved3;
	/* 0x006: PS/2 Output Signal */
	volatile uint8_t PSOSIG;
	volatile uint8_t reserved4;
	/* 0x008: PS/2 Input Signal */
	volatile uint8_t PSISIG;
	volatile uint8_t reserved5;
	/* 0x00A: PS/2 Interrupt Enable */
	volatile uint8_t PSIEN;
	volatile uint8_t reserved6;
};

/* PS/2 Interface registers fields */
#define NPCX_PSTAT_SOT                   0
#define NPCX_PSTAT_EOT                   1
#define NPCX_PSTAT_PERR                  2
#define NPCX_PSTAT_ACH                   FIELD(3, 3)
#define NPCX_PSTAT_RFERR                 6

#define NPCX_PSCON_EN                    0
#define NPCX_PSCON_XMT                   1
#define NPCX_PSCON_HDRV                  FIELD(2, 2)
#define NPCX_PSCON_IDB                   FIELD(4, 3)
#define NPCX_PSCON_WPUED                 7

#define NPCX_PSOSIG_WDAT0                0
#define NPCX_PSOSIG_WDAT1                1
#define NPCX_PSOSIG_WDAT2                2
#define NPCX_PSOSIG_CLK0                 3
#define NPCX_PSOSIG_CLK1                 4
#define NPCX_PSOSIG_CLK2                 5
#define NPCX_PSOSIG_WDAT3                6
#define NPCX_PSOSIG_CLK3                 7
#define NPCX_PSOSIG_CLK(n)               (((n) < 3) ? ((n) + 3) : 7)
#define NPCX_PSOSIG_WDAT(n)              (((n) < 3) ? ((n) + 0) : 6)
#define NPCX_PSOSIG_CLK_MASK_ALL \
					 (BIT(NPCX_PSOSIG_CLK0) | \
					  BIT(NPCX_PSOSIG_CLK1) | \
					  BIT(NPCX_PSOSIG_CLK2) | \
					  BIT(NPCX_PSOSIG_CLK3))

#define NPCX_PSIEN_SOTIE                 0
#define NPCX_PSIEN_EOTIE                 1
#define NPCX_PSIEN_PS2_WUE               4
#define NPCX_PSIEN_PS2_CLK_SEL           7

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
	volatile uint8_t reserved5[4];
	/* 0x030: FIU Read Command */
	volatile uint8_t FIU_RD_CMD;
	volatile uint8_t reserved6;
	/* 0x032: FIU Dummy Cycles */
	volatile uint8_t FIU_DMM_CYC;
	/* 0x033: FIU Extended Configuration */
	volatile uint8_t FIU_EXT_CFG;
#if defined(CONFIG_SOC_SERIES_NPCX9)
	/* 0x034: UMA address byte 0-3 */
	volatile uint32_t UMA_AB0_3;
	/* 0x038-0x3C */
	volatile uint8_t reserved8[5];
	/* 0x03D: SPI Device */
	volatile uint8_t SPI1_DEV;
	/* 0x03E-0x3F */
	volatile uint8_t reserved9[2];
#elif defined(CONFIG_SOC_SERIES_NPCX4)
	/* 0x034: UMA address byte 0-3 */
	volatile uint32_t UMA_AB0_3;
	/* 0x038-0x3B */
	volatile uint8_t reserved8[4];
	/* 0x03C: SPI Device */
	volatile uint8_t SPI_DEV;
	/* 0x03D */
	volatile uint8_t reserved9;
	/* 0x03E */
	volatile uint8_t SPI_DEV_SIZE;
	/* 0x03F */
	volatile uint8_t reserved10;
#endif
};

/* FIU register fields */
#define NPCX_BURST_CFG_SPI_DEV_SEL       FIELD(4, 2)
#define NPCX_RESP_CFG_IAD_EN             0
#define NPCX_RESP_CFG_DEV_SIZE_EX        2
#define NPCX_RESP_CFG_QUAD_EN            3
#define NPCX_SPI_FL_CFG_RD_MODE          FIELD(6, 2)
#define NPCX_UMA_CTS_A_SIZE              3
#define NPCX_UMA_CTS_C_SIZE              4
#define NPCX_UMA_CTS_RD_WR               5
#define NPCX_UMA_CTS_DEV_NUM             6
#define NPCX_UMA_CTS_EXEC_DONE           7
#define NPCX_UMA_ECTS_SW_CS0             0
#define NPCX_UMA_ECTS_SW_CS1             1
#define NPCX_UMA_ECTS_SEC_CS             2
#define NPCX_UMA_ECTS_UMA_LOCK           3
#define NPCX_UMA_ECTS_UMA_ADDR_SIZE      FIELD(4, 3)
#define NPCX_SPI1_DEV_FOUR_BADDR_CS10    6
#define NPCX_SPI1_DEV_FOUR_BADDR_CS11    7
#define NPCX_SPI1_DEV_SPI1_LO_DEV_SIZE   FIELD(0, 4)
#define NPCX_FIU_EXT_CFG_SET_DMM_EN      2
#define NPCX_FIU_EXT_CFG_SET_CMD_EN      1
#define NPCX_SPI_DEV_NADDRB              FIELD(5, 3)

#define NPCX_MSR_IE_CFG_UMA_BLOCK        3

/* UMA fields selections */
#define UMA_FLD_ADDR     BIT(NPCX_UMA_CTS_A_SIZE)  /* 3-bytes ADR field */
#define UMA_FLD_NO_CMD   BIT(NPCX_UMA_CTS_C_SIZE)  /* No 1-Byte CMD field */
#define UMA_FLD_WRITE    BIT(NPCX_UMA_CTS_RD_WR)   /* Write transaction */
#define UMA_FLD_SHD_SL   BIT(NPCX_UMA_CTS_DEV_NUM) /* Shared flash selected */
#define UMA_FLD_EXEC     BIT(NPCX_UMA_CTS_EXEC_DONE)

#define UMA_FIELD_DATA_1 0x01
#define UMA_FIELD_DATA_2 0x02
#define UMA_FIELD_DATA_3 0x03
#define UMA_FIELD_DATA_4 0x04

/* UMA code for transaction */
#define UMA_CODE_CMD_ONLY       (UMA_FLD_EXEC | UMA_FLD_SHD_SL)
#define UMA_CODE_CMD_ADR        (UMA_FLD_EXEC | UMA_FLD_ADDR | \
					UMA_FLD_SHD_SL)
#define UMA_CODE_CMD_RD_BYTE(n) (UMA_FLD_EXEC | UMA_FIELD_DATA_##n | \
					UMA_FLD_SHD_SL)
#define UMA_CODE_RD_BYTE(n)     (UMA_FLD_EXEC | UMA_FLD_NO_CMD | \
					UMA_FIELD_DATA_##n | UMA_FLD_SHD_SL)
#define UMA_CODE_CMD_WR_ONLY    (UMA_FLD_EXEC | UMA_FLD_WRITE | \
					UMA_FLD_SHD_SL)
#define UMA_CODE_CMD_WR_BYTE(n) (UMA_FLD_EXEC | UMA_FLD_WRITE | \
					UMA_FIELD_DATA_##n | UMA_FLD_SHD_SL)
#define UMA_CODE_CMD_WR_ADR     (UMA_FLD_EXEC | UMA_FLD_WRITE | UMA_FLD_ADDR | \
				UMA_FLD_SHD_SL)

#define UMA_CODE_CMD_ADR_WR_BYTE(n) (UMA_FLD_EXEC | UMA_FLD_WRITE | \
					UMA_FLD_ADDR | UMA_FIELD_DATA_##n | \
					UMA_FLD_SHD_SL)

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
	volatile uint8_t reserved1[3];
	/* 0x00B: PECI Write FCS */
	volatile uint8_t PECI_WR_FCS;
	/* 0x00C: PECI Read FCS */
	volatile uint8_t PECI_RD_FCS;
	/* 0x00D: PECI Assured Write FCS */
	volatile uint8_t PECI_AW_FCS;
	volatile uint8_t reserved2;
	/* 0x00F: PECI Transfer Rate */
	volatile uint8_t PECI_RATE;
	/* 0x010 - 0x04F: PECI Data In/Out */
	union {
		volatile uint8_t PECI_DATA_IN[64];
		volatile uint8_t PECI_DATA_OUT[64];
	};
};

/* PECI register fields */
#define NPCX_PECI_CTL_STS_START_BUSY     0
#define NPCX_PECI_CTL_STS_DONE           1
#define NPCX_PECI_CTL_STS_CRC_ERR        3
#define NPCX_PECI_CTL_STS_ABRT_ERR       4
#define NPCX_PECI_CTL_STS_AWFCS_EB       5
#define NPCX_PECI_CTL_STS_DONE_EN        6
#define NPCX_PECI_RATE_MAX_BIT_RATE      FIELD(0, 5)
#define NPCX_PECI_RATE_MAX_BIT_RATE_MASK 0x1F
/* The minimal valid value of NPCX_PECI_RATE_MAX_BIT_RATE field */
#define PECI_MAX_BIT_RATE_VALID_MIN      0x05
#define PECI_HIGH_SPEED_MIN_VAL          0x07

#define NPCX_PECI_RATE_EHSP              6

/* KBS (Keyboard Scan) device registers */
struct kbs_reg {
	volatile uint8_t reserved1[4];
	/* 0x004: Keyboard Scan In */
	volatile uint8_t KBSIN;
	/* 0x005: Keyboard Scan In Pull-Up Enable */
	volatile uint8_t KBSINPU;
	/* 0x006: Keyboard Scan Out 0 */
	volatile uint16_t KBSOUT0;
	/* 0x008: Keyboard Scan Out 1 */
	volatile uint16_t KBSOUT1;
	/* 0x00A: Keyboard Scan Buffer Index */
	volatile uint8_t KBS_BUF_INDX;
	/* 0x00B: Keyboard Scan Buffer Data */
	volatile uint8_t KBS_BUF_DATA;
	/* 0x00C: Keyboard Scan Event */
	volatile uint8_t KBSEVT;
	/* 0x00D: Keyboard Scan Control */
	volatile uint8_t KBSCTL;
	/* 0x00E: Keyboard Scan Configuration Index */
	volatile uint8_t KBS_CFG_INDX;
	/* 0x00F: Keyboard Scan Configuration Data */
	volatile uint8_t KBS_CFG_DATA;
};

/* KBS register fields */
#define NPCX_KBSBUFINDX                  0
#define NPCX_KBSEVT_KBSDONE              0
#define NPCX_KBSEVT_KBSERR               1
#define NPCX_KBSCTL_START                0
#define NPCX_KBSCTL_KBSMODE              1
#define NPCX_KBSCTL_KBSIEN               2
#define NPCX_KBSCTL_KBSINC               3
#define NPCX_KBSCTL_KBHDRV_FIELD         FIELD(6, 2)
#define NPCX_KBSCFGINDX                  0
/* Index of 'Automatic Scan' configuration register */
#define KBS_CFG_INDX_DLY1                0 /* Keyboard Scan Delay T1 Byte */
#define KBS_CFG_INDX_DLY2                1 /* Keyboard Scan Delay T2 Byte */
#define KBS_CFG_INDX_RTYTO               2 /* Keyboard Scan Retry Timeout */
#define KBS_CFG_INDX_CNUM                3 /* Keyboard Scan Columns Number */
#define KBS_CFG_INDX_CDIV                4 /* Keyboard Scan Clock Divisor */

/* SHI (Serial Host Interface) registers */
struct shi_reg {
	volatile uint8_t reserved1;
	/* 0x001: SHI Configuration 1 */
	volatile uint8_t SHICFG1;
	/* 0x002: SHI Configuration 2 */
	volatile uint8_t SHICFG2;
	volatile uint8_t reserved2[2];
	/* 0x005: Event Enable */
	volatile uint8_t EVENABLE;
	/* 0x006: Event Status */
	volatile uint8_t EVSTAT;
	/* 0x007: SHI Capabilities */
	volatile uint8_t CAPABILITY;
	/* 0x008: Status */
	volatile uint8_t STATUS;
	volatile uint8_t reserved3;
	/* 0x00A: Input Buffer Status */
	volatile uint8_t IBUFSTAT;
	/* 0x00B: Output Buffer Status */
	volatile uint8_t OBUFSTAT;
	/* 0x00C: SHI Configuration 3 */
	volatile uint8_t SHICFG3;
	/* 0x00D: SHI Configuration 4 */
	volatile uint8_t SHICFG4;
	/* 0x00E: SHI Configuration 5 */
	volatile uint8_t SHICFG5;
	/* 0x00F: Event Status 2 */
	volatile uint8_t EVSTAT2;
	/* 0x010: Event Enable 2 */
	volatile uint8_t EVENABLE2;
	/* 0x011: SHI Configuration 6 - only in chips which support enhanced buffer mode */
	volatile uint8_t SHICFG6;
	/* 0x012: Single Byte Output Buffer - only in chips which support enhanced buffer mode */
	volatile uint8_t SBOBUF;
	volatile uint8_t reserved4[13];
	/* 0x20~0x9F: Output Buffer */
	volatile uint8_t OBUF[128];
	/* 0xA0~0x11F: Input Buffer */
	volatile uint8_t IBUF[128];
};

/* SHI register fields */
#define NPCX_SHICFG1_EN                  0
#define NPCX_SHICFG1_MODE                1
#define NPCX_SHICFG1_WEN                 2
#define NPCX_SHICFG1_AUTIBF              3
#define NPCX_SHICFG1_AUTOBE              4
#define NPCX_SHICFG1_DAS                 5
#define NPCX_SHICFG1_CPOL                6
#define NPCX_SHICFG1_IWRAP               7
#define NPCX_SHICFG2_SIMUL               0
#define NPCX_SHICFG2_BUSY                1
#define NPCX_SHICFG2_ONESHOT             2
#define NPCX_SHICFG2_SLWU                3
#define NPCX_SHICFG2_REEN                4
#define NPCX_SHICFG2_RESTART             5
#define NPCX_SHICFG2_REEVEN              6
#define NPCX_EVENABLE_OBEEN              0
#define NPCX_EVENABLE_OBHEEN             1
#define NPCX_EVENABLE_IBFEN              2
#define NPCX_EVENABLE_IBHFEN             3
#define NPCX_EVENABLE_EOREN              4
#define NPCX_EVENABLE_EOWEN              5
#define NPCX_EVENABLE_STSREN             6
#define NPCX_EVENABLE_IBOREN             7
#define NPCX_EVSTAT_OBE                  0
#define NPCX_EVSTAT_OBHE                 1
#define NPCX_EVSTAT_IBF                  2
#define NPCX_EVSTAT_IBHF                 3
#define NPCX_EVSTAT_EOR                  4
#define NPCX_EVSTAT_EOW                  5
#define NPCX_EVSTAT_STSR                 6
#define NPCX_EVSTAT_IBOR                 7
#define NPCX_STATUS_OBES                 6
#define NPCX_STATUS_IBFS                 7
#define NPCX_SHICFG3_OBUFLVLDIS          7
#define NPCX_SHICFG4_IBUFLVLDIS          7
#define NPCX_SHICFG5_IBUFLVL2            FIELD(0, 6)
#define NPCX_SHICFG5_IBUFLVL2DIS         7
#define NPCX_EVSTAT2_IBHF2               0
#define NPCX_EVSTAT2_CSNRE               1
#define NPCX_EVSTAT2_CSNFE               2
#define NPCX_EVENABLE2_IBHF2EN           0
#define NPCX_EVENABLE2_CSNREEN           1
#define NPCX_EVENABLE2_CSNFEEN           2
#define NPCX_SHICFG6_EBUFMD              0
#define NPCX_SHICFG6_OBUF_SL             1

#define IBF_IBHF_EN_MASK                 (BIT(NPCX_EVENABLE_IBFEN) | BIT(NPCX_EVENABLE_IBHFEN))

/* SPIP (SPI Peripheral Interface) registers */
struct spip_reg {
	/* 0x000: SPIP Data In/Out */
	volatile uint16_t SPIP_DATA;
	/* 0x002: SPIP Control 1 */
	volatile uint16_t SPIP_CTL1;
	/* 0x004: SPIP Status */
	volatile uint8_t SPIP_STAT;
	volatile uint8_t reserved1;
};

#define NPCX_SPIP_CTL1_SPIEN            0
#define NPCX_SPIP_CTL1_MOD              2
#define NPCX_SPIP_CTL1_EIR              5
#define NPCX_SPIP_CTL1_EIW              6
#define NPCX_SPIP_CTL1_SCM              7
#define NPCX_SPIP_CTL1_SCIDL            8
#define NPCX_SPIP_CTL1_SCDV             FIELD(9, 7)
#define NPCX_SPIP_STAT_BSY              0
#define NPCX_SPIP_STAT_RBF              1

/* Software-triggered Pheripheral Reset Controller Register */
struct swrst_reg {
	/* 0x000: Software Reset Trigger */
	volatile uint16_t SWRST_TRG;
	volatile uint8_t reserved1[2];
	volatile uint32_t SWRST_CTL[4];
};

/* Improved Inter Integrated Circuit  (I3C) device registers */
struct i3c_reg {
	/* 0x000: Controller Configuration */
	volatile uint32_t MCONFIG;
	/* 0x004: Target Configuration */
	volatile uint32_t CONFIG;
	/* 0x008: Target Status */
	volatile uint32_t STATUS;
	/* 0x00C: Target Control  */
	volatile uint32_t CTRL;
	/* 0x010: Target Interrupt Enable Set  */
	volatile uint32_t INTSET;
	/* 0x014: Target Interrupt Enable Clear */
	volatile uint32_t INTCLR;
	/* 0x018: Target Interrupt Masked */
	volatile uint32_t INTMASKED;
	/* 0x01C: Target Error and Warning */
	volatile uint32_t ERRWARN;
	/* 0x020: Target DMA Control */
	volatile uint32_t DMACTRL;
	/* 0x024-0x02B: reserved */
	volatile uint32_t reserved0[2];
	/* 0x02C: Target Data Control */
	volatile uint32_t DATACTRL;
	/* 0x030: Target Write Byte Data */
	volatile uint32_t WDATAB;
	/* 0x034: Target Write Byte Data as End */
	volatile uint32_t WDATABE;
	/* 0x038: Target Write Half-Word Data */
	volatile uint32_t WDATAH;
	/* 0x03C: Target Write Half-Word Data as End */
	volatile uint32_t WDATAHE;
	/* 0x040: Target Read Byte Data */
	volatile uint32_t RDATAB;
	/* 0x044: reserved */
	volatile uint32_t reserved1;
	/* 0x048: Target Read Half-Word Data */
	volatile uint32_t RDATAH;
	/* 0x04C-0x05B: reserved */
	volatile uint32_t reserved2[4];
	/* 0x05C: Target Capabilities 2 */
	volatile uint32_t CCAPABILITIES2;
	/* 0x060: Target Capabilities */
	volatile uint32_t CCAPABILITIES;
	/* 0x064: Target Dynamic Address */
	volatile uint32_t DYNADDR;
	/* 0x068: Target Maximum Limits */
	volatile uint32_t MAXLIMITS;
	/* 0x06C: Target Part Number */
	volatile uint32_t PARTNO;
	/* 0x070: Target ID Extension */
	volatile uint32_t IDEXT;
	/* 0x074: Target Vendor ID */
	volatile uint32_t VENDORID;
	/* 0x078: Target Timing Control Clock */
	volatile uint32_t TCCLOCK;
	/* 0x07C: Target Matching Address Index */
	volatile uint32_t MSGLAST;
	/* 0x080: reserved */
	volatile uint32_t reserved3;
	/* 0x084: Controller Control  */
	volatile uint32_t MCTRL;
	/* 0x088: Controller Status */
	volatile uint32_t MSTATUS;
	/* 0x08C: IBI Registry and Rules */
	volatile uint32_t IBIRULES;
	/* 0x090: Controller Interrupt Enable Set  */
	volatile uint32_t MINTSET;
	/* 0x094: Controller Interrupt Enable Clear */
	volatile uint32_t MINTCLR;
	/* 0x098: Controller Interrupt Masked */
	volatile uint32_t MINTMASKED;
	/* 0x09C: Controller Error and Warning */
	volatile uint32_t MERRWARN;
	/* 0x0A0: Controller DMA Control */
	volatile uint32_t MDMACTRL;
	/* 0x0A0-0x0AB: reserved */
	volatile uint32_t reserved4[2];
	/* 0x0AC: Controller Data Control */
	volatile uint32_t MDATACTRL;
	/* 0x0B0: Controller Write Byte Data */
	volatile uint32_t MWDATAB;
	/* 0x0B4: Controller Write Byte Data as End */
	volatile uint32_t MWDATABE;
	/* 0x0B8: Controller Write Half-Word Data */
	volatile uint32_t MWDATAH;
	/* 0x0BC: Controller Write Half-Word Data as End */
	volatile uint32_t MWDATAHE;
	/* 0x0C0: Controller Read Byte Data */
	volatile uint32_t MRDATAB;
	/* 0x0C4: reserved */
	volatile uint32_t reserved5;
	/* 0x0C8: Controller Read Half-Word Data */
	volatile uint32_t MRDATAH;
	/* 0x0CC-0x0D7: reserved */
	volatile uint32_t reserved6[3];
	/* 0x0D8: Start or Continue DDR Message */
	volatile uint32_t MWMSG_DDR;
	/* 0x0DC: Read DDR Message Data */
	volatile uint32_t MRMSG_DDR;
	/* 0x0E0: reserved */
	volatile uint32_t reserved7;
	/* 0x0E4: Controller Dynamic Address */
	volatile uint32_t MDYNADDR;
	/* 0x0E8-0x0FF reserved */
	volatile uint32_t reserved8[6];
	/* 0x100: Target Reset Recovery Time */
	volatile uint32_t RSTACTTIME;
	/* 0x104: reserved */
	volatile uint32_t reserved9;
	/* 0x108: Target HDR Command Byte */
	volatile uint32_t HDRCMD;
	/* 0x10C-0x113: reserved */
	volatile uint32_t reserved10[2];
	/* 0x114: Target Group Definition */
	volatile uint32_t GROUPDEF;
	/* 0x118: reserved */
	volatile uint32_t reserved11;
	/* 0x11C: Target Map Control 0 */
	volatile uint32_t MAPCTRL0;
	/* 0x120: Target Map Control 1 */
	volatile uint32_t MAPCTRL1;
	/* 0x124: Target Map Control 2 */
	volatile uint32_t MAPCTRL2;
	/* 0x128: Target Map Control 3 */
	volatile uint32_t MAPCTRL3;
	/* 0x12C: Target Map Control 4 */
	volatile uint32_t MAPCTRL4;
	/* 0x130: Target Map Control 5 */
	volatile uint32_t MAPCTRL5;
	/* 0x134: Target Map Control 6 */
	volatile uint32_t MAPCTRL6;
	/* 0x138: Target Map Control 7 */
	volatile uint32_t MAPCTRL7;
	/* 0x13C: Target Map Control 8 */
	volatile uint32_t MAPCTRL8;
	/* 0x140: Target Extended IBI Data 1 */
	volatile uint32_t IBIEXT1;
	/* 0x144: Target Extended IBI Data 2 */
	volatile uint32_t IBIEXT2;
};

/* I3C controller register fields */
#define NPCX_I3C_MCONFIG_CTRENA         FIELD(0, 2)
#define NPCX_I3C_MCONFIG_DISTO          3
#define NPCX_I3C_MCONFIG_HKEEP          FIELD(4, 2) /* Must be '11' */
#define NPCX_I3C_MCONFIG_ODSTOP         6
#define NPCX_I3C_MCONFIG_PPBAUD         FIELD(8, 4)
#define NPCX_I3C_MCONFIG_PPLOW          FIELD(12, 4)
#define NPCX_I3C_MCONFIG_ODBAUD         FIELD(16, 8)
#define NPCX_I3C_MCONFIG_ODHPP          24
#define NPCX_I3C_MCONFIG_SKEW           FIELD(25, 3)
#define NPCX_I3C_MCONFIG_I2CBAUD        FIELD(28, 4)
#define NPCX_I3C_MCTRL_REQUEST          FIELD(0, 3)
#define NPCX_I3C_MCTRL_TYPE             FIELD(4, 2)
#define NPCX_I3C_MCTRL_IBIRESP          FIELD(6, 2)
#define NPCX_I3C_MCTRL_DIR              8
#define NPCX_I3C_MCTRL_ADDR             FIELD(9, 7)
#define NPCX_I3C_MCTRL_RDTERM           FIELD(16, 8)
#define NPCX_I3C_MSTATUS_STATE          FIELD(0, 3)
#define NPCX_I3C_MSTATUS_BETWEEN        4
#define NPCX_I3C_MSTATUS_NACKED         5
#define NPCX_I3C_MSTATUS_IBITYPE        FIELD(6, 2)
#define NPCX_I3C_MSTATUS_TGTSTART       8
#define NPCX_I3C_MSTATUS_MCTRLDONE      9
#define NPCX_I3C_MSTATUS_COMPLETE       10
#define NPCX_I3C_MSTATUS_RXPEND         11
#define NPCX_I3C_MSTATUS_TXNOTFULL      12
#define NPCX_I3C_MSTATUS_IBIWON         13
#define NPCX_I3C_MSTATUS_ERRWARN        15
#define NPCX_I3C_MSTATUS_NOWCNTLR       19
#define NPCX_I3C_MSTATUS_IBIADDR        FIELD(24, 7)
#define NPCX_I3C_IBIRULES_MSB0          30
#define NPCX_I3C_IBIRULES_NOBYTE        31
#define NPCX_I3C_MINTSET_TGTSTART       8
#define NPCX_I3C_MINTSET_MCTRLDONE      9
#define NPCX_I3C_MINTSET_COMPLETE       10
#define NPCX_I3C_MINTSET_RXPEND         11
#define NPCX_I3C_MINTSET_TXNOTFULL      12
#define NPCX_I3C_MINTSET_IBIWON         13
#define NPCX_I3C_MINTSET_ERRWARN        15
#define NPCX_I3C_MINTSET_NOWCNTLR       19
#define NPCX_I3C_MINTCLR_TGTSTART       8
#define NPCX_I3C_MINTCLR_MCTRLDONE      9
#define NPCX_I3C_MINTCLR_COMPLETE       10
#define NPCX_I3C_MINTCLR_RXPEND         11
#define NPCX_I3C_MINTCLR_TXNOTFULL      12
#define NPCX_I3C_MINTCLR_IBIWON         13
#define NPCX_I3C_MINTCLR_ERRWARN        15
#define NPCX_I3C_MINTCLR_NOWCNTLR       19
#define NPCX_I3C_MINTMASKED_NOWCNTLR    19
#define NPCX_I3C_MDATACTRL_FLUSHTB      0
#define NPCX_I3C_MDATACTRL_FLUSHFB      1
#define NPCX_I3C_MDATACTRL_UNLOCK       3
#define NPCX_I3C_MDATACTRL_TXTRIG       FIELD(4, 2)
#define NPCX_I3C_MDATACTRL_RXTRIG       FIELD(6, 2)
#define NPCX_I3C_MDATACTRL_TXCOUNT      FIELD(16, 5)
#define NPCX_I3C_MDATACTRL_RXCOUNT      FIELD(24, 5)
#define NPCX_I3C_MDATACTRL_TXFULL       30
#define NPCX_I3C_MDATACTRL_RXEMPTY      31
#define NPCX_I3C_MERRWARN_NACK          2
#define NPCX_I3C_MERRWARN_WRABT         3
#define NPCX_I3C_MERRWARN_TERM          4
#define NPCX_I3C_MERRWARN_HPAR          9
#define NPCX_I3C_MERRWARN_HCRC          10
#define NPCX_I3C_MERRWARN_OREAD         16
#define NPCX_I3C_MERRWARN_OWRITE        17
#define NPCX_I3C_MERRWARN_MSGERR        18
#define NPCX_I3C_MERRWARN_INVERQ        19
#define NPCX_I3C_MERRWARN_TIMEOUT       20
#define NPCX_I3C_MDMACTRL_DMAFB         FIELD(0, 2)
#define NPCX_I3C_MDMACTRL_DMATB         FIELD(2, 2)

/* I3C target register fields */
#define NPCX_I3C_CONFIG_TGTENA          0
#define NPCX_I3C_CONFIG_MATCHSS         2
#define NPCX_I3C_CONFIG_S0IGNORE        3
#define NPCX_I3C_CONFIG_IDRAND          8
#define NPCX_I3C_CONFIG_OFFLINE         9
#define NPCX_I3C_CONFIG_HDRCMD          FIELD(10, 2)
#define NPCX_I3C_CONFIG_BAMATCH         FIELD(16, 7)
#define NPCX_I3C_CONFIG_SADDR           FIELD(25, 7)
#define NPCX_I3C_STATUS_STNOTSTOP       0
#define NPCX_I3C_STATUS_STMSG           1
#define NPCX_I3C_STATUS_STCCCH          2
#define NPCX_I3C_STATUS_STREQRD         3
#define NPCX_I3C_STATUS_STREQWR         4
#define NPCX_I3C_STATUS_STDAA           5
#define NPCX_I3C_STATUS_STHDR           6
#define NPCX_I3C_STATUS_START           8
#define NPCX_I3C_STATUS_MATCHED         9
#define NPCX_I3C_STATUS_STOP            10
#define NPCX_I3C_STATUS_RXPEND          11
#define NPCX_I3C_STATUS_TXNOTFULL       12
#define NPCX_I3C_STATUS_DACHG           13
#define NPCX_I3C_STATUS_CCC             14
#define NPCX_I3C_STATUS_ERRWARN         15
#define NPCX_I3C_STATUS_HDRMATCH        16
#define NPCX_I3C_STATUS_CHANDLED        17
#define NPCX_I3C_STATUS_EVENT           18
#define NPCX_I3C_STATUS_TGTRST          19
#define NPCX_I3C_STATUS_EVDET           FIELD(20, 2)
#define NPCX_I3C_STATUS_IBIDIS          24
#define NPCX_I3C_STATUS_MRDIS           25
#define NPCX_I3C_STATUS_HJDIS           27
#define NPCX_I3C_STATUS_ACTSTATE        FIELD(28, 2)
#define NPCX_I3C_STATUS_TIMECTRL        FIELD(30, 2)
#define NPCX_I3C_CTRL_EVENT             FIELD(0, 2)
#define NPCX_I3C_CTRL_EXTDATA           3
#define NPCX_I3C_CTRL_MAPIDX            FIELD(4, 4)
#define NPCX_I3C_CTRL_IBIDATA           FIELD(8, 8)
#define NPCX_I3C_CTRL_PENDINT           FIELD(16, 4)
#define NPCX_I3C_CTRL_ACTSTATE          FIELD(20, 2)
#define NPCX_I3C_CTRL_VENDINFO          FIELD(24, 8)
#define NPCX_I3C_INTSET_START           8
#define NPCX_I3C_INTSET_MATCHED         9
#define NPCX_I3C_INTSET_STOP            10
#define NPCX_I3C_INTSET_RXPEND          11
#define NPCX_I3C_INTSET_TXNOTFULL       12
#define NPCX_I3C_INTSET_DACHG           13
#define NPCX_I3C_INTSET_CCC             14
#define NPCX_I3C_INTSET_ERRWARN         15
#define NPCX_I3C_INTSET_HDRMATCH        16
#define NPCX_I3C_INTSET_CHANDLED        17
#define NPCX_I3C_INTSET_EVENT           18
#define NPCX_I3C_INTSET_TGTRST          19
#define NPCX_I3C_INTCLR_START           8
#define NPCX_I3C_INTCLR_MATCHED         9
#define NPCX_I3C_INTCLR_STOP            10
#define NPCX_I3C_INTCLR_RXPEND          11
#define NPCX_I3C_INTCLR_TXNOTFULL       12
#define NPCX_I3C_INTCLR_DACHG           13
#define NPCX_I3C_INTCLR_CCC             14
#define NPCX_I3C_INTCLR_ERRWARN         15
#define NPCX_I3C_INTCLR_HDRMATCH        16
#define NPCX_I3C_INTCLR_CHANDLED        17
#define NPCX_I3C_INTCLR_EVENT           18
#define NPCX_I3C_INTCLR_TGTRST          19
#define NPCX_I3C_INTMASKED_START        8
#define NPCX_I3C_INTMASKED_MATCHED      9
#define NPCX_I3C_INTMASKED_STOP         10
#define NPCX_I3C_INTMASKED_RXPEND       11
#define NPCX_I3C_INTMASKED_TXNOTFULL    12
#define NPCX_I3C_INTMASKED_DACHG        13
#define NPCX_I3C_INTMASKED_CCC          14
#define NPCX_I3C_INTMASKED_ERRWARN      15
#define NPCX_I3C_INTMASKED_HDRMATCH     16
#define NPCX_I3C_INTMASKED_CHANDLED     17
#define NPCX_I3C_INTMASKED_EVENT        18
#define NPCX_I3C_INTMASKED_TGTRST       19
#define NPCX_I3C_ERRWARN_ORUN           0
#define NPCX_I3C_ERRWARN_URUN           1
#define NPCX_I3C_ERRWARN_URUNNACK       2
#define NPCX_I3C_ERRWARN_TERM           3
#define NPCX_I3C_ERRWARN_INVSTART       4
#define NPCX_I3C_ERRWARN_SPAR           8
#define NPCX_I3C_ERRWARN_HPAR           9
#define NPCX_I3C_ERRWARN_HCRC           10
#define NPCX_I3C_ERRWARN_S0S1           11
#define NPCX_I3C_ERRWARN_OREAD          16
#define NPCX_I3C_ERRWARN_OWRITE         17
#define NPCX_I3C_DMACTRL_DMAFB          FIELD(0, 2)
#define NPCX_I3C_DMACTRL_DMATB          FIELD(2, 2)
#define NPCX_I3C_DATACTRL_FLUSHTB       0
#define NPCX_I3C_DATACTRL_FLUSHFB       1
#define NPCX_I3C_DATACTRL_UNLOCK        3
#define NPCX_I3C_DATACTRL_TXTRIG        FIELD(4, 2)
#define NPCX_I3C_DATACTRL_RXTRIG        FIELD(6, 2)
#define NPCX_I3C_DATACTRL_TXCOUNT       FIELD(16, 6)
#define NPCX_I3C_DATACTRL_RXCOUNT       FIELD(24, 6)
#define NPCX_I3C_DATACTRL_TXFULL        30
#define NPCX_I3C_DATACTRL_RXEMPTY       31
#define NPCX_I3C_WDATAB_DATA            FIELD(0, 8)
#define NPCX_I3C_WDATAB_END_B           8
#define NPCX_I3C_WDATAB_END_A           16
#define NPCX_I3C_WDATABE_DATA           FIELD(0, 8)
#define NPCX_I3C_WDATAH_DATA0           FIELD(0, 8)
#define NPCX_I3C_WDATAH_DATA1           FIELD(8, 8)
#define NPCX_I3C_WDATAH_END             16
#define NPCX_I3C_WDATAHE_DATA0          FIELD(0, 8)
#define NPCX_I3C_WDATAHE_DATA1          FIELD(8, 8)
#define NPCX_I3C_RDATAB_DATA            FIELD(0, 8)
#define NPCX_I3C_RDATAH_DATA0           FIELD(0, 8)
#define NPCX_I3C_RDATAH_DATA1           FIELD(8, 8)
#define NPCX_I3C_CAPABILITIES2_MAPCNT   FIELD(0, 4)
#define NPCX_I3C_CAPABILITIES2_I2C10B   4
#define NPCX_I3C_CAPABILITIES2_I2CRST   5
#define NPCX_I3C_CAPABILITIES2_I2CDEVID 6
#define NPCX_I3C_CAPABILITIES2_DATA32   7
#define NPCX_I3C_CAPABILITIES2_IBIEXT   8
#define NPCX_I3C_CAPABILITIES2_IBIXREG  9
#define NPCX_I3C_CAPABILITIES2_SMLANE   FIELD(12, 2)
#define NPCX_I3C_CAPABILITIES2_V1_1     16
#define NPCX_I3C_CAPABILITIES2_TGTRST   17
#define NPCX_I3C_CAPABILITIES2_GROUP    FIELD(18, 2)
#define NPCX_I3C_CAPABILITIES2_AASA     21
#define NPCX_I3C_CAPABILITIES2_SSTSUB   22
#define NPCX_I3C_CAPABILITIES2_SSTWR    23
#define NPCX_I3C_CAPABILITIES_IDENA     FIELD(0, 2)
#define NPCX_I3C_CAPABILITIES_IDREG     FIELD(2, 4)
#define NPCX_I3C_CAPABILITIES_HDRSUPP   FIELD(6, 3)
#define NPCX_I3C_CAPABILITIES_CNTLR     9
#define NPCX_I3C_CAPABILITIES_SADDR     FIELD(10, 2)
#define NPCX_I3C_CAPABILITIES_CCCHANDLE FIELD(12, 4)
#define NPCX_I3C_CAPABILITIES_IBI_MR_HJ FIELD(16, 5)
#define NPCX_I3C_CAPABILITIES_TIMECTRL  21
#define NPCX_I3C_CAPABILITIES_EXTFIFO   FIELD(23, 3)
#define NPCX_I3C_CAPABILITIES_FIFOTX    FIELD(26, 2)
#define NPCX_I3C_CAPABILITIES_FIFORX    FIELD(28, 2)
#define NPCX_I3C_CAPABILITIES_INT       30
#define NPCX_I3C_CAPABILITIES_DMA       31
#define NPCX_I3C_DYNADDR_DAVALID        0
#define NPCX_I3C_DYNADDR_DADDR          FIELD(1, 7)
#define NPCX_I3C_DYNADDR_CAUSE          FIELD(8, 3)
#define NPCX_I3C_MAXLIMITS_MAXRD        FIELD(0, 12)
#define NPCX_I3C_MAXLIMITS_MAXWR        FIELD(16, 12)
#define NPCX_I3C_IDEXT_DCR              FIELD(8, 8)
#define NPCX_I3C_IDEXT_BCR              FIELD(16, 8)
#define NPCX_I3C_VENDORID_VID           FIELD(0, 15)
#define NPCX_I3C_TCCLOCK_ACCURACY       FIELD(0, 8)
#define NPCX_I3C_TCCLOCK_FREQ           FIELD(8, 8)
#define NPCX_I3C_MSGLAST_MAPLAST        FIELD(0, 4)
#define NPCX_I3C_MSGLAST_LASTSTATIC     4
#define NPCX_I3C_MSGLAST_LASTGROUP      5
#define NPCX_I3C_MSGLAST_LASTMODE       FIELD(6, 2)
#define NPCX_I3C_MSGLAST_MAPLASTM1      FIELD(8, 4)
#define NPCX_I3C_MSGLAST_LASTGROUPM1    13
#define NPCX_I3C_MSGLAST_LASTMODE1      FIELD(14, 2)
#define NPCX_I3C_MSGLAST_MAPLASTM2      FIELD(16, 4)
#define NPCX_I3C_MSGLAST_LASTGROUPM2    21
#define NPCX_I3C_MSGLAST_LASTMODE2      FIELD(22, 2)
#define NPCX_I3C_RSTACTTIME_PERRSTTIM   FIELD(0, 8)
#define NPCX_I3C_RSTACTTIME_SYSRSTTIM   FIELD(8, 8)
#define NPCX_I3C_HDRCMD_CMD0            FIELD(0, 8)
#define NPCX_I3C_HDRCMD_OVFL            30
#define NPCX_I3C_HDRCMD_NEWCMD          31
#define NPCX_I3C_GROUPDEF_GRP0ENA       0
#define NPCX_I3C_GROUPDEF_GRP0DA        FIELD(1, 7)
#define NPCX_I3C_GROUPDEF_GRP1ENA       8
#define NPCX_I3C_GROUPDEF_GRP1DA        FIELD(9, 7)
#define NPCX_I3C_GROUPDEF_GRP2ENA       16
#define NPCX_I3C_GROUPDEF_GRP2DA        FIELD(17, 7)
#define NPCX_I3C_MAPCTRL0_ENA           0
#define NPCX_I3C_MAPCTRL0_DA            FIELD(1, 7)
#define NPCX_I3C_MAPCTRL0_CAUSE         FIELD(8, 3)
#define NPCX_I3C_MAPCTRLn_ENA           0
#define NPCX_I3C_MAPCTRLn_ADDR          FIELD(1, 7)
#define NPCX_I3C_MAPCTRLn_MAPSA         8
#define NPCX_I3C_MAPCTRLn_NACK          12
#define NPCX_I3C_MAPCTRLn_AUTO          13
#define NPCX_I3C_MAPCTRLn_SLOT_PID      FIELD(14, 5)
#define NPCX_I3C_IBIEXT1_CNT            FIELD(0, 3)
#define NPCX_I3C_IBIEXT1_EXT1           FIELD(8, 8)
#define NPCX_I3C_IBIEXT1_EXT2           FIELD(16, 8)
#define NPCX_I3C_IBIEXT1_EXT3           FIELD(24, 8)
#define NPCX_I3C_IBIEXT2_EXT4           FIELD(0, 8)
#define NPCX_I3C_IBIEXT2_EXT5           FIELD(8, 8)
#define NPCX_I3C_IBIEXT2_EXT6           FIELD(16, 8)
#define NPCX_I3C_IBIEXT2_EXT7           FIELD(24, 8)

/* MDMA Controller registers */
struct mdma_reg {
	/* Channel 0 */
	/* 0x000: Channel 0 Control */
	volatile uint32_t MDMA_CTL0;
	/* 0x004: Channel 0 Source Base Address */
	volatile uint32_t MDMA_SRCB0;
	/* 0x008: Channel 0 Destination Base Address */
	volatile uint32_t MDMA_DSTB0;
	/* 0x00C: Channel 0 Transfer Count */
	volatile uint32_t MDMA_TCNT0;
	/* 0x010: reserved1 */
	volatile uint32_t reserved1;
	/* 0x014: Channel 0 Current Destination */
	volatile uint32_t MDMA_CDST0;
	/* 0x018: Channel 0 Current Transfer Count */
	volatile uint32_t MDMA_CTCNT0;
	/* 0x01C: reserved2 */
	volatile uint32_t reserved2;

	/* Channel 1 */
	/* 0x020: Channel 1 Control */
	volatile uint32_t MDMA_CTL1;
	/* 0x024: Channel 1 Source Base Address */
	volatile uint32_t MDMA_SRCB1;
	/* 0x028: Channel 1 Destination Base Address */
	volatile uint32_t MDMA_DSTB1;
	/* 0x02C: Channel 1 Transfer Count */
	volatile uint32_t MDMA_TCNT1;
	/* 0x030: Channel 1 Current Source */
	volatile uint32_t MDMA_CSRC1;
	/* 0x034: reserved3 */
	volatile uint32_t reserved3;
	/* 0x038: Channel 1 Current Transfer Count */
	volatile uint32_t MDMA_CTCNT1;
};

/* MDMA register fields */
#define NPCX_MDMA_CTL_MDMAEN 0
#define NPCX_MDMA_CTL_MPD    1
#define NPCX_MDMA_CTL_SIEN   8
#define NPCX_MDMA_CTL_MPS    14
#define NPCX_MDMA_CTL_TC     18

/* Channel 0/1 Transfer Count Register (MDMA_TCNT0/MDMA_TCNT1) */
#define NPCX_MDMA_TCNT_TFR_CNT FIELD(0, 13)

/* Channel 0/1 Current Transfer Count Register (MDMA_CTCNT0/MDMA_CTCNT1) */
#define NPCX_MDMA_CTCNT_CURRENT_TFR_CNT FIELD(0, 13)

#endif /* _NUVOTON_NPCX_REG_DEF_H */
