/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MEC172X_REGS_H
#define MEC172X_REGS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum IRQn {
	Reset_IRQn              = -15,
	NonMaskableInt_IRQn     = -14,
	HardFault_IRQn          = -13,
	MemoryManagement_IRQn   = -12,
	BusFault_IRQn           = -11,
	UsageFault_IRQn         = -10,
	SVCall_IRQn             = -5,
	DebugMonitor_IRQn       = -4,
	PendSV_IRQn             = -2,
	SysTick_IRQn            = -1,
	/* MEC172x Specific Interrupt Numbers */
	GIRQ08_IRQn             = 0,
	GIRQ09_IRQn             = 1,
	GIRQ10_IRQn             = 2,
	GIRQ11_IRQn             = 3,
	GIRQ12_IRQn             = 4,
	GIRQ13_IRQn             = 5,
	GIRQ14_IRQn             = 6,
	GIRQ15_IRQn             = 7,
	GIRQ16_IRQn             = 8,
	GIRQ17_IRQn             = 9,
	GIRQ18_IRQn             = 10,
	GIRQ19_IRQn             = 11,
	GIRQ20_IRQn             = 12,
	GIRQ21_IRQn             = 13,
	/* GIRQ22(peripheral clock wake) is not connected to NVIC */
	GIRQ23_IRQn             = 14,
	GIRQ24_IRQn             = 15,
	GIRQ25_IRQn             = 16,
	GIRQ26_IRQn             = 17,
	/* Reserved 18-19 */
	/* begin direct */
	I2C_SMB_0_IRQn          = 20,
	I2C_SMB_1_IRQn          = 21,
	I2C_SMB_2_IRQn          = 22,
	I2C_SMB_3_IRQn          = 23,
	DMA0_IRQn               = 24,
	DMA1_IRQn               = 25,
	DMA2_IRQn               = 26,
	DMA3_IRQn               = 27,
	DMA4_IRQn               = 28,
	DMA5_IRQn               = 29,
	DMA6_IRQn               = 30,
	DMA7_IRQn               = 31,
	DMA8_IRQn               = 32,
	DMA9_IRQn               = 33,
	DMA10_IRQn              = 34,
	DMA11_IRQn              = 35,
	DMA12_IRQn              = 36,
	DMA13_IRQn              = 37,
	DMA14_IRQn              = 38,
	DMA15_IRQn              = 39,
	UART0_IRQn              = 40,
	UART1_IRQn              = 41,
	EMI0_IRQn               = 42,
	EMI1_IRQn               = 43,
	EMI2_IRQn               = 44,
	ACPI_EC0_IBF_IRQn       = 45,
	ACPI_EC0_OBE_IRQn       = 46,
	ACPI_EC1_IBF_IRQn       = 47,
	ACPI_EC1_OBE_IRQn       = 48,
	ACPI_EC2_IBF_IRQn       = 49,
	ACPI_EC2_OBE_IRQn       = 50,
	ACPI_EC3_IBF_IRQn       = 51,
	ACPI_EC3_OBE_IRQn       = 52,
	ACPI_EC4_IBF_IRQn       = 53,
	ACPI_EC4_OBE_IRQn       = 54,
	ACPI_PM1_CTL_IRQn       = 55,
	ACPI_PM1_EN_IRQn        = 56,
	ACPI_PM1_STS_IRQn       = 57,
	KBC_OBE_IRQn            = 58,
	KBC_IBF_IRQn            = 59,
	MBOX_IRQn               = 60,
	/* reserved 61 */
	P80BD_0_IRQn            = 62,
	/* reserved 63-64 */
	PKE_IRQn                = 65,
	/* reserved 66 */
	RNG_IRQn                = 67,
	AESH_IRQn               = 68,
	/* reserved 69 */
	PECI_IRQn               = 70,
	TACH_0_IRQn             = 71,
	TACH_1_IRQn             = 72,
	TACH_2_IRQn             = 73,
	RPMFAN_0_FAIL_IRQn      = 74,
	RPMFAN_0_STALL_IRQn     = 75,
	RPMFAN_1_FAIL_IRQn      = 76,
	RPMFAN_1_STALL_IRQn     = 77,
	ADC_SNGL_IRQn           = 78,
	ADC_RPT_IRQn            = 79,
	RCID_0_IRQn             = 80,
	RCID_1_IRQn             = 81,
	RCID_2_IRQn             = 82,
	LED_0_IRQn              = 83,
	LED_1_IRQn              = 84,
	LED_2_IRQn              = 85,
	LED_3_IRQn              = 86,
	PHOT_IRQn               = 87,
	/* reserved 88-89 */
	SPIP_0_IRQn             = 90,
	QMSPI_0_IRQn            = 91,
	GPSPI_0_TXBE_IRQn       = 92,
	GPSPI_0_RXBF_IRQn       = 93,
	GPSPI_1_TXBE_IRQn       = 94,
	GPSPI_1_RXBF_IRQn       = 95,
	BCL_0_ERR_IRQn          = 96,
	BCL_0_BCLR_IRQn         = 97,
	/* reserved 98-99 */
	PS2_0_ACT_IRQn          = 100,
	/* reserved 101-102 */
	ESPI_PC_IRQn            = 103,
	ESPI_BM1_IRQn           = 104,
	ESPI_BM2_IRQn           = 105,
	ESPI_LTR_IRQn           = 106,
	ESPI_OOB_UP_IRQn        = 107,
	ESPI_OOB_DN_IRQn        = 108,
	ESPI_FLASH_IRQn         = 109,
	ESPI_RESET_IRQn         = 110,
	RTMR_IRQn               = 111,
	HTMR_0_IRQn             = 112,
	HTMR_1_IRQn             = 113,
	WK_IRQn                 = 114,
	WKSUB_IRQn              = 115,
	WKSEC_IRQn              = 116,
	WKSUBSEC_IRQn           = 117,
	WKSYSPWR_IRQn           = 118,
	RTC_IRQn                = 119,
	RTC_ALARM_IRQn          = 120,
	VCI_OVRD_IN_IRQn        = 121,
	VCI_IN0_IRQn            = 122,
	VCI_IN1_IRQn            = 123,
	VCI_IN2_IRQn            = 124,
	VCI_IN3_IRQn            = 125,
	VCI_IN4_IRQn            = 126,
	/* reserved 127-128 */
	PS2_0A_WAKE_IRQn        = 129,
	PS2_0B_WAKE_IRQn        = 130,
	/* reserved 131-134 */
	KEYSCAN_IRQn            = 135,
	B16TMR_0_IRQn           = 136,
	B16TMR_1_IRQn           = 137,
	B16TMR_2_IRQn           = 138,
	B16TMR_3_IRQn           = 139,
	B32TMR_0_IRQn           = 140,
	B32TMR_1_IRQn           = 141,
	CTMR_0_IRQn             = 142,
	CTMR_1_IRQn             = 143,
	CTMR_2_IRQn             = 144,
	CTMR_3_IRQn             = 145,
	CCT_IRQn                = 146,
	CCT_CAP0_IRQn           = 147,
	CCT_CAP1_IRQn           = 148,
	CCT_CAP2_IRQn           = 149,
	CCT_CAP3_IRQn           = 150,
	CCT_CAP4_IRQn           = 151,
	CCT_CAP5_IRQn           = 152,
	CCT_CMP0_IRQn           = 153,
	CCT_CMP1_IRQn           = 154,
	EEPROMC_IRQn            = 155,
	ESPI_VWIRE_IRQn         = 156,
	/* reserved 157 */
	I2C_SMB_4_IRQn          = 158,
	TACH_3_IRQn             = 159,
	/* reserved 160-165 */
	SAF_DONE_IRQn           = 166,
	SAF_ERR_IRQn            = 167,
	/* reserved 168 */
	SAF_CACHE_IRQn          = 169,
	/* reserved 170 */
	WDT_0_IRQn              = 171,
	GLUE_IRQn               = 172,
	OTP_RDY_IRQn            = 173,
	CLK32K_MON_IRQn         = 174,
	ACPI_EC0_IRQn           = 175,
	ACPI_EC1_IRQn           = 176,
	ACPI_EC2_IRQn           = 177,
	ACPI_EC3_IRQn           = 178,
	ACPI_EC4_IRQn           = 179,
	ACPI_PM1_IRQn           = 180,
	MAX_IRQn
};

/*
 * ARM CMSIS requires typedef IRQn_Type or it generates an unknown type name
 * compiler error. ;<(
 * We can't create new typedefs in zephyr code, so hack with a macro.
 * NOTE: C macros and typedef's are not equivalent! Some esoteric use cases
 * may not generate expected code!
 */
#define IRQn_Type enum IRQn

#define __CM4_REV 0x0201	/*!< Core Revision r2p1 */

#define __MPU_PRESENT		1
#define __VTOR_PRESENT		1
#define __NVIC_PRIO_BITS	3
#define __Vendor_SysTickConfig	0
#define __FPU_PRESENT		1
#define __FPU_DP		0
#define __ICACHE_PRESENT	0
#define __DCACHE_PRESENT	0
#define __DTCM_PRESENT		0

/* Requires: modules/hal/cmsis */
#include <core_cm4.h>

/** @brief GIRQ registers. Total size = 20(0x14) bytes */
struct girq_regs {
	volatile uint32_t SRC;
	volatile uint32_t EN_SET;
	volatile uint32_t RESULT;
	volatile uint32_t EN_CLR;
	uint32_t RSVD1[1];
};

/** @brief ECIA registers with each GIRQ elucidated */
struct ecia_regs {
	struct girq_regs GIRQ08;
	struct girq_regs GIRQ09;
	struct girq_regs GIRQ10;
	struct girq_regs GIRQ11;
	struct girq_regs GIRQ12;
	struct girq_regs GIRQ13;
	struct girq_regs GIRQ14;
	struct girq_regs GIRQ15;
	struct girq_regs GIRQ16;
	struct girq_regs GIRQ17;
	struct girq_regs GIRQ18;
	struct girq_regs GIRQ19;
	struct girq_regs GIRQ20;
	struct girq_regs GIRQ21;
	struct girq_regs GIRQ22;
	struct girq_regs GIRQ23;
	struct girq_regs GIRQ24;
	struct girq_regs GIRQ25;
	struct girq_regs GIRQ26;
	uint8_t RSVD2[(0x0200U - 0x017CU)];
	volatile uint32_t BLK_EN_SET;
	volatile uint32_t BLK_EN_CLR;
	volatile uint32_t BLK_ACTIVE;
};

/** @brief ECIA registers with array of GIRQ's */
struct ecia_ar_regs {
	struct girq_regs GIRQ[19];
	uint8_t RSVD2[(0x200u - 0x17Cu)];
	volatile uint32_t BLK_EN_SET;
	volatile uint32_t BLK_EN_CLR;
	volatile uint32_t BLK_ACTIVE;
};

/** @brief Watch Dog Timer (WDT) */
struct wdt_regs {
	volatile uint16_t LOAD;
	uint8_t RSVD1[2];
	volatile uint16_t CTRL;
	uint8_t RSVD2[2];
	volatile uint8_t KICK;
	uint8_t RSVD3[3];
	volatile uint16_t CNT;
	uint8_t RSVD4[2];
	volatile uint16_t STS;
	uint8_t RSVD5[2];
	volatile uint8_t IEN;
};

/** @brief 32 and 16 bit Basic Timers */
struct btmr_regs {
	volatile uint32_t CNT;
	volatile uint32_t PRLD;
	volatile uint8_t STS;
	uint8_t RSVDC[3];
	volatile uint8_t IEN;
	uint8_t RSVDD[3];
	volatile uint32_t CTRL;
};

/** @brief 16-bit Counter Timer */
struct ctmr_regs {
	volatile uint32_t CTRL;
	volatile uint32_t CEV_CTRL;
	volatile uint32_t RELOAD;
	volatile uint32_t COUNT;
};

/** @brief Capture/Compare Timer */
struct cct_regs {
	volatile uint32_t CTRL;
	volatile uint32_t CAP0_CTRL;
	volatile uint32_t CAP1_CTRL;
	volatile uint32_t FREE_RUN;
	volatile uint32_t CAP0;
	volatile uint32_t CAP1;
	volatile uint32_t CAP2;
	volatile uint32_t CAP3;
	volatile uint32_t CAP4;
	volatile uint32_t CAP5;
	volatile uint32_t COMP0;
	volatile uint32_t COMP1;
};

/** @brief Hibernation Timer (HTMR) */
struct htmr_regs {
	volatile uint16_t PRLD;
	uint16_t RSVD1[1];
	volatile uint16_t CTRL;
	uint16_t RSVD2[1];
	volatile uint16_t CNT;
	uint16_t RSVD3[1];
};

/** @brief RTOS Timer (RTMR) */
struct rtmr_regs {
	volatile uint32_t CNT;
	volatile uint32_t PRLD;
	volatile uint32_t CTRL;
	volatile uint32_t SOFTIRQ;
};

/** @brief Week Timer (WKTMR) */
struct wktmr_regs {
	volatile uint32_t CTRL;
	volatile uint32_t ALARM_CNT;
	volatile uint32_t TMR_COMP;
	volatile uint32_t CLKDIV;
	volatile uint32_t SS_INTR_SEL;
	volatile uint32_t SWK_CTRL;
	volatile uint32_t SWK_ALARM;
	volatile uint32_t BGPO_DATA;
	volatile uint32_t BGPO_PWR;
	volatile uint32_t BGPO_RST;
};

/** @brief Real Time Clock (RTC) */
struct rtc_regs {
	volatile uint8_t SECONDS;
	volatile uint8_t SEC_ALARM;
	volatile uint8_t MINUTES;
	volatile uint8_t MIN_ALARM;
	volatile uint8_t HOURS;
	volatile uint8_t HOURS_ALARM;
	volatile uint8_t DAY_OF_WEEK;
	volatile uint8_t DAY_OF_MONTH;
	volatile uint8_t MONTH;
	volatile uint8_t YEAR;
	volatile uint8_t REGA;
	volatile uint8_t REGB;
	volatile uint8_t REGC;
	volatile uint8_t REGD;
	uint8_t RSVD1[2];
	volatile uint8_t CONTROL;
	uint8_t RSVD2[3];
	volatile uint8_t WEEK_ALARM;
	uint8_t RSVD3[3];
	volatile uint32_t DLSF;
	volatile uint32_t DLSB;
};

/** @brief DMA Main (DMAM) */
struct dma_main_regs {
	volatile uint8_t ACTRST;
	uint8_t RSVDA[3];
	volatile uint32_t DATA_PKT;
	volatile uint32_t ARB_FSM;
};

/** @brief DMA Channels 0 and 1 with ALU */
struct dma_chan_alu_regs {
	volatile uint32_t ACTV;
	volatile uint32_t MSTART;
	volatile uint32_t MEND;
	volatile uint32_t DSTART;
	volatile uint32_t CTRL;
	volatile uint32_t ISTS;
	volatile uint32_t IEN;
	volatile uint32_t FSM;
	volatile uint32_t ALU_EN;
	volatile uint32_t ALU_DATA;
	volatile uint32_t ALU_STS;
	volatile uint32_t ALU_FSM;
	uint32_t RSVD6[4];
};

/** @brief DMA Channels 2 through 11 no ALU */
struct dma_chan_regs {
	volatile uint32_t ACTV;
	volatile uint32_t MSTART;
	volatile uint32_t MEND;
	volatile uint32_t DSTART;
	volatile uint32_t CTRL;
	volatile uint32_t ISTS;
	volatile uint32_t IEN;
	volatile uint32_t FSM;
	uint32_t RSVD4[8];
};

/** @brief DMA block: main and channels */
struct dma_regs {
	volatile uint32_t ACTRST;
	volatile uint32_t DATA_PKT;
	volatile uint32_t ARB_FSM;
	uint32_t RSVD2[13];
	struct dma_chan_alu_regs CHAN[16];
};

/** @brief I2C with SMBus Network Layer (SNL) */
struct i2c_smb_regs {
	volatile uint8_t CTRLSTS;
	uint8_t RSVD1[3];
	volatile uint32_t OWN_ADDR;
	volatile uint8_t I2CDATA;
	uint8_t RSVD2[3];
	volatile uint32_t MCMD;
	volatile uint32_t SCMD;
	volatile uint8_t PEC;
	uint8_t RSVD3[3];
	volatile uint32_t RSHTM;
	volatile uint32_t EXTLEN;
	volatile uint32_t COMPL;
	volatile uint32_t IDLSC;
	volatile uint32_t CFG;
	volatile uint32_t BUSCLK;
	volatile uint32_t BLKID;
	volatile uint32_t BLKREV;
	volatile uint8_t BBCTRL;
	uint8_t RSVD7[3];
	volatile uint32_t CLKSYNC;
	volatile uint32_t DATATM;
	volatile uint32_t TMOUTSC;
	volatile uint8_t SLV_TXB;
	uint8_t RSVD8[3];
	volatile uint8_t SLV_RXB;
	uint8_t RSVD9[3];
	volatile uint8_t MTR_TXB;
	uint8_t RSVD10[3];
	volatile uint8_t MTR_RXB;
	uint8_t RSVD11[3];
	volatile uint32_t FSM;
	volatile uint32_t FSM_SMB;
	volatile uint8_t WAKE_STS;
	uint8_t RSVD12[3];
	volatile uint8_t WAKE_EN;
	uint32_t RSVD13[2];
	volatile uint32_t PROM_ISTS;
	volatile uint32_t PROM_IEN;
	volatile uint32_t PROM_CTRL;
	volatile uint32_t SHADOW_DATA;
};

/** @brief EEPROM Controller (EEPROMC) */
struct eepromc_regs {
	volatile uint32_t MODE;
	volatile uint32_t EXE;
	volatile uint32_t STATUS;
	volatile uint32_t IEN;
	volatile uint32_t PSWD;
	volatile uint32_t UNLOCK;
	volatile uint32_t LOCK;
	uint32_t RSVD1[1];
	union {
		volatile uint32_t DATA32[8];
		volatile uint16_t DATA16[16];
		volatile uint8_t DATA8[32];
	};
};

/** @brief SPI Peripheral registers (SPIP) */
struct spip_regs {
	volatile uint32_t CONFIG;
	volatile uint32_t SLV_STATUS;
	volatile uint32_t EC_STATUS;
	volatile uint32_t SPI_INT_EN;
	volatile uint32_t EC_INT_EN;
	volatile uint32_t MCONFIG;
	volatile uint32_t MBA0;
	volatile uint32_t MBA0_WLIM;
	volatile uint32_t MBA0_RLIM;
	volatile uint32_t MBA1;
	volatile uint32_t MBA1_WLIM;
	volatile uint32_t MBA1_RLIM;
	volatile uint32_t RXF_HBAR;
	volatile uint32_t RXF_BCNT;
	volatile uint32_t TXF_HBAR;
	volatile uint32_t TXF_BCNT;
	volatile uint32_t SYS_CONFIG;
	volatile uint32_t MBOX_S2EC;
	volatile uint32_t MBOX_EC2S;
};

/** @brief General Purpose SPI Controller (GPSPI) */
struct gpspi_regs {
	volatile uint32_t BLOCK_EN;
	volatile uint32_t CONTROL;
	volatile uint32_t STATUS;
	volatile uint8_t TX_DATA;
	uint8_t RSVD1[3];
	volatile uint8_t RX_DATA;
	uint8_t RSVD2[3];
	volatile uint32_t CLK_CTRL;
	volatile uint32_t CLK_GEN;
};

/** @brief QMSPI Local DMA channel */
struct qldma_chan {
	volatile uint32_t CTRL;
	volatile uint32_t MSTART;
	volatile uint32_t LEN;
	uint32_t RSVD1[1];
};

/** @brief Quad Controller SPI with local DMA added */
struct qmspi_regs {
	volatile uint32_t MODE;
	volatile uint32_t CTRL;
	volatile uint32_t EXE;
	volatile uint32_t IFCTRL;
	volatile uint32_t STS;
	volatile uint32_t BCNT_STS;
	volatile uint32_t IEN;
	volatile uint32_t BCNT_TRIG;
	volatile uint32_t TX_FIFO;
	volatile uint32_t RX_FIFO;
	volatile uint32_t CSTM;
	uint32_t RSVD1[1];
	volatile uint32_t DESCR[16];
	uint32_t RSVD2[16];
	volatile uint32_t ALIAS_CTRL;
	uint32_t RSVD3[3];
	volatile uint32_t MODE_ALT1;
	uint32_t RSVD4[3];
	volatile uint32_t TM_TAPS;
	volatile uint32_t TM_TAPS_ADJ;
	volatile uint32_t TM_TAPS_CTRL;
	uint32_t RSVD5[9];
	volatile uint32_t LDMA_RX_DESCR_BM;
	volatile uint32_t LDMA_TX_DESCR_BM;
	uint32_t RSVD6[2];
	struct qldma_chan LDRX[3];
	struct qldma_chan LDTX[3];
};

/** @brief Processor Hot Interface (PROCHOT) */
struct prochot_regs {
	volatile uint32_t CUMUL_COUNT;
	volatile uint32_t DUTY_COUNT;
	volatile uint32_t DUTY_PERIOD;
	volatile uint32_t CTRL_STS;
	volatile uint32_t ASSERT_COUNT;
	volatile uint32_t ASSERT_LIMIT;
};

/** @brief Platform Environment Control Interface Registers (PECI) */
struct peci_regs {
	volatile uint8_t WR_DATA;
	uint8_t RSVD1[3];
	volatile uint8_t RD_DATA;
	uint8_t RSVD2[3];
	volatile uint8_t CONTROL;
	uint8_t RSVD3[3];
	volatile uint8_t STATUS1;
	uint8_t RSVD4[3];
	volatile uint8_t STATUS2;
	uint8_t RSVD5[3];
	volatile uint8_t ERROR;
	uint8_t RSVD6[3];
	volatile uint8_t INT_EN1;
	uint8_t RSVD7[3];
	volatile uint8_t INT_EN2;
	uint8_t RSVD8[3];
	volatile uint8_t OPT_BIT_TIME_LSB;
	uint8_t RSVD9[3];
	volatile uint8_t OPT_BIT_TIME_MSB;
	uint8_t RSVD10[87]; /* 0x25 - 0x7C */
};

/** @brief Pulse Width Modulator Registers (PWM) */
struct pwm_regs {
	volatile uint32_t COUNT_ON;
	volatile uint32_t COUNT_OFF;
	volatile uint32_t CONFIG;
};

/** @brief Tachometer Registers (TACH) */
struct tach_regs {
	volatile uint32_t CONTROL;
	volatile uint32_t STATUS;
	volatile uint32_t LIMIT_HI;
	volatile uint32_t LIMIT_LO;
};

/** @brief RPM to PWM Fan Registers (RPMFAN) */
struct rpmfan_regs {
	volatile uint16_t FAN_SETTING;
	volatile uint16_t FAN_CONFIG;
	volatile uint8_t PWM_DIVIDE;
	volatile uint8_t GAIN;
	volatile uint8_t FSPU_CFG;
	volatile uint8_t FAN_STEP;
	volatile uint8_t FAN_MIN_DRV;
	volatile uint8_t VALID_TACH_CNT;
	volatile uint16_t FAN_DFB;
	volatile uint16_t TACH_TARGET;
	volatile uint16_t TACH_READING;
	volatile uint8_t PWM_DBF;
	volatile uint8_t FAN_STATUS;
	uint8_t RSVD1[6];
};

/** @brief Keyboard Scan Matrix Registers (KSCAN) */
struct kscan_regs {
	uint32_t RSVD[1];
	volatile uint32_t KSO_SEL;
	volatile uint32_t KSI_IN;
	volatile uint32_t KSI_STS;
	volatile uint32_t KSI_IEN;
	volatile uint32_t EXT_CTRL;
};

/** @brief Emulated 8042 Keyboard Controller Registers (KBC) */
struct kbc_regs {
	volatile uint32_t HOST_AUX_DATA;
	volatile uint32_t KBC_STS_RD;
	uint8_t RSVD1[0x100u - 0x08u];
	volatile uint32_t EC_DATA;
	volatile uint32_t EC_KBC_STS;
	volatile uint32_t KBC_CTRL;
	volatile uint32_t EC_AUX_DATA;
	uint32_t RSVD2[1];
	volatile uint32_t PCOBF;
	uint8_t RSVD3[0x0330ul - 0x0118ul];
	volatile uint32_t KBC_PORT92_EN;
};

/** @brief Fast Port92h Registers (PORT92) */
struct port92_regs {
	volatile uint32_t HOST_P92;
	uint8_t RSVD1[0x100u - 0x04u];
	volatile uint32_t GATEA20_CTRL;
	uint32_t RSVD2[1];
	volatile uint32_t SETGA20L;
	volatile uint32_t RSTGA20L;
	uint8_t RSVD3[0x0330ul - 0x0110ul];
	volatile uint32_t ACTV;
};

/** @brief PS/2 Controller Registers (PS2) */
struct ps2_regs {
	volatile uint32_t TRX_BUFF;
	volatile uint32_t CTRL;
	volatile uint32_t STATUS;
};

/** @brief VBAT Register Bank (VBATR) */
struct vbatr_regs {
	volatile uint32_t PFRS;
	uint32_t RSVD1[1];
	volatile uint32_t CLK32_SRC;
	uint32_t RSVD2[5];
	volatile uint32_t MCNT_LO;
	volatile uint32_t MCNT_HI;
	uint32_t RSVD3[3];
	volatile uint32_t EMBRD_EN;
};

/** @brief VBAT Memory (VBATM) */
struct vbatm_regs {
	union vbmem_u {
		uint32_t u32[128U / 4U];
		uint16_t u16[128U / 2U];
		uint8_t u8[128U];
	} MEM;
};

/** @brief VBAT powered control interface (VCI) */
struct vci_regs {
	volatile uint32_t CONFIG;
	volatile uint32_t LATCH_EN;
	volatile uint32_t LATCH_RST;
	volatile uint32_t INPUT_EN;
	volatile uint32_t HOLD_OFF;
	volatile uint32_t POLARITY;
	volatile uint32_t PEDGE_DET;
	volatile uint32_t NEDGE_DET;
	volatile uint32_t BUFFER_EN;
};

/** @brief Breathing-Blinking LED (LED) */
struct led_regs {
	volatile uint32_t CFG;
	volatile uint32_t LIMIT;
	volatile uint32_t DLY;
	volatile uint32_t STEP;
	volatile uint32_t INTRVL;
	volatile uint32_t OUTDLY;
};

/** @brief BC-Link Controller (BCL) */
struct bcl_regs {
	volatile uint32_t STATUS;
	volatile uint32_t TADDR;
	volatile uint32_t DATA;
	volatile uint32_t CLKSEL;
};

/** @brief Glue Logic registers (GLUE) */
struct glue_regs {
	uint32_t RSVD1[1];
	volatile uint32_t S0IX_DET_EN;
	uint32_t RSVD2[(0x10Cu - 0x008u) / 4];
	volatile uint32_t PWRGD_SRC_CFG;
	volatile uint32_t S0IX_DET_CFG;
	uint32_t RSVD3[5];
	volatile uint32_t MON_STATE;
	volatile uint32_t MON_IPEND;
	volatile uint32_t MON_IEN;
};

/** @brief Analog to Digital Converter Registers (ADC) */
struct adc_regs {
	volatile uint32_t CONTROL;
	volatile uint32_t DELAY;
	volatile uint32_t STATUS;
	volatile uint32_t SINGLE;
	volatile uint32_t REPEAT;
	volatile uint32_t RD[8];
	uint8_t RSVD1[0x7C - 0x34];
	volatile uint32_t CONFIG;
	volatile uint32_t VREF_CHAN_SEL;
	volatile uint32_t VREF_CTRL;
	volatile uint32_t SARADC_CTRL;
};

/** @brief RC-ID */
struct rcid_regs {
	volatile uint32_t CTRL;
	volatile uint32_t DATA;
};

/** @brief Glocal Configuration Registers */
struct global_cfg_regs {
	volatile uint8_t RSVD0[2];
	volatile uint8_t TEST02;
	volatile uint8_t RSVD1[4];
	volatile uint8_t LOG_DEV_NUM;
	volatile uint8_t RSVD2[20];
	volatile uint32_t DEV_REV_ID;
	volatile uint8_t LEGACY_DEV_ID;
	volatile uint8_t RSVD3[14];
};

/**  @brief EC Subsystem (ECS) */
struct ecs_regs {
	uint32_t RSVD1[1];
	volatile uint32_t AHB_ERR_ADDR;
	uint32_t RSVD2[2];
	volatile uint32_t OSC_ID;
	volatile uint32_t AHB_ERR_CTRL;
	volatile uint32_t INTR_CTRL;
	volatile uint32_t ETM_CTRL;
	volatile uint32_t DEBUG_CTRL;
	volatile uint32_t OTP_LOCK;
	volatile uint32_t WDT_CNT;
	uint32_t RSVD3[5];
	volatile uint32_t PECI_DIS;
	uint32_t RSVD4[3];
	volatile uint32_t VCI_FW_OVR;
	uint32_t RSVD5[1];
	volatile uint32_t CRYPTO_CFG;
	uint32_t RSVD6[5];
	volatile uint32_t JTAG_MCFG;
	volatile uint32_t JTAG_MSTS;
	volatile uint32_t JTAG_MTDO;
	volatile uint32_t JTAG_MTDI;
	volatile uint32_t JTAG_MTMS;
	volatile uint32_t JTAG_MCMD;
	uint32_t RSVD7[2];
	volatile uint32_t VW_FW_OVR;
	volatile uint32_t CMP_CTRL;
	volatile uint32_t CMP_SLP_CTRL;
	uint32_t RSVD8[(0x144 - 0x9C) / 4];
	volatile uint32_t SLP_STS_MIRROR;
};

/** @brief Power Control Reset (PCR) */
struct pcr_regs {
	volatile uint32_t SYS_SLP_CTRL;
	volatile uint32_t PROC_CLK_CTRL;
	volatile uint32_t SLOW_CLK_CTRL;
	volatile uint32_t OSC_ID;
	volatile uint32_t PWR_RST_STS;
	volatile uint32_t PWR_RST_CTRL;
	volatile uint32_t SYS_RST;
	volatile uint32_t TURBO_CLK;
	volatile uint32_t TEST20;
	uint32_t RSVD1[3];
	volatile uint32_t SLP_EN[5];
	uint32_t RSVD2[3];
	volatile uint32_t CLK_REQ[5];
	uint32_t RSVD3[3];
	volatile uint32_t RST_EN[5];
	volatile uint32_t RST_EN_LOCK;
	volatile uint32_t VBAT_SRST;
	volatile uint32_t CLK32K_SRC_VTR;
	volatile uint32_t TEST90;
	uint32_t RSVD4[(0x00C0 - 0x0094) / 4];
	volatile uint32_t CNT32K_PER;
	volatile uint32_t CNT32K_PULSE_HI;
	volatile uint32_t CNT32K_PER_MIN;
	volatile uint32_t CNT32K_PER_MAX;
	volatile uint32_t CNT32K_DV;
	volatile uint32_t CNT32K_DV_MAX;
	volatile uint32_t CNT32K_VALID;
	volatile uint32_t CNT32K_VALID_MIN;
	volatile uint32_t CNT32K_CTRL;
	volatile uint32_t CLK32K_MON_ISTS;
	volatile uint32_t CLK32K_MON_IEN;
};

/** @brief All GPIO register as arrays of registers */
struct gpio_regs {
	volatile uint32_t CTRL[192];
	volatile uint32_t PARIN[6];
	uint32_t RSVD1[(0x380u - 0x318u) / 4];
	volatile uint32_t PAROUT[6];
	uint32_t RSVD2[(0x3ECu - 0x398u) / 4u];
	volatile uint32_t LOCK[6];
	uint32_t RSVD3[(0x500u - 0x400u) / 4u];
	volatile uint32_t CTRL2[192];
};

/** @brief GPIO control registers by pin name */
struct gpio_ctrl_regs {
	volatile uint32_t CTRL_0000;
	uint32_t RSVD1[1];
	volatile uint32_t CTRL_0002;
	volatile uint32_t CTRL_0003;
	volatile uint32_t CTRL_0004;
	uint32_t RSVD2[2];
	volatile uint32_t CTRL_0007;
	volatile uint32_t CTRL_0010;
	volatile uint32_t CTRL_0011;
	volatile uint32_t CTRL_0012;
	volatile uint32_t CTRL_0013;
	volatile uint32_t CTRL_0014;
	volatile uint32_t CTRL_0015;
	volatile uint32_t CTRL_0016;
	volatile uint32_t CTRL_0017;
	volatile uint32_t CTRL_0020;
	volatile uint32_t CTRL_0021;
	volatile uint32_t CTRL_0022;
	volatile uint32_t CTRL_0023;
	volatile uint32_t CTRL_0024;
	volatile uint32_t CTRL_0025;
	volatile uint32_t CTRL_0026;
	volatile uint32_t CTRL_0027;
	volatile uint32_t CTRL_0030;
	volatile uint32_t CTRL_0031;
	volatile uint32_t CTRL_0032;
	volatile uint32_t CTRL_0033;
	volatile uint32_t CTRL_0034;
	volatile uint32_t CTRL_0035;
	volatile uint32_t CTRL_0036;
	uint32_t RSVD3[1];
	volatile uint32_t CTRL_0040;
	uint32_t RSVD4[1];
	volatile uint32_t CTRL_0042;
	volatile uint32_t CTRL_0043;
	volatile uint32_t CTRL_0044;
	volatile uint32_t CTRL_0045;
	volatile uint32_t CTRL_0046;
	volatile uint32_t CTRL_0047;
	volatile uint32_t CTRL_0050;
	volatile uint32_t CTRL_0051;
	volatile uint32_t CTRL_0052;
	volatile uint32_t CTRL_0053;
	volatile uint32_t CTRL_0054;
	volatile uint32_t CTRL_0055;
	volatile uint32_t CTRL_0056;
	volatile uint32_t CTRL_0057;
	volatile uint32_t CTRL_0060;
	volatile uint32_t CTRL_0061;
	volatile uint32_t CTRL_0062;
	volatile uint32_t CTRL_0063;
	volatile uint32_t CTRL_0064;
	volatile uint32_t CTRL_0065;
	volatile uint32_t CTRL_0066;
	volatile uint32_t CTRL_0067;
	volatile uint32_t CTRL_0070;
	volatile uint32_t CTRL_0071;
	volatile uint32_t CTRL_0072;
	volatile uint32_t CTRL_0073;
	uint32_t RSVD5[4];
	volatile uint32_t CTRL_0100;
	volatile uint32_t CTRL_0101;
	volatile uint32_t CTRL_0102;
	uint32_t RSVD6[1];
	volatile uint32_t CTRL_0104;
	volatile uint32_t CTRL_0105;
	volatile uint32_t CTRL_0106;
	volatile uint32_t CTRL_0107;
	uint32_t RSVD7[2];
	volatile uint32_t CTRL_0112;
	volatile uint32_t CTRL_0113;
	volatile uint32_t CTRL_0114;
	volatile uint32_t CTRL_0115;
	uint32_t RSVD8[2];
	volatile uint32_t CTRL_0120;
	volatile uint32_t CTRL_0121;
	volatile uint32_t CTRL_0122;
	volatile uint32_t CTRL_0123;
	volatile uint32_t CTRL_0124;
	volatile uint32_t CTRL_0125;
	volatile uint32_t CTRL_0126;
	volatile uint32_t CTRL_0127;
	volatile uint32_t CTRL_0130;
	volatile uint32_t CTRL_0131;
	volatile uint32_t CTRL_0132;
	uint32_t RSVD9[5];
	volatile uint32_t CTRL_0140;
	volatile uint32_t CTRL_0141;
	volatile uint32_t CTRL_0142;
	volatile uint32_t CTRL_0143;
	volatile uint32_t CTRL_0144;
	volatile uint32_t CTRL_0145;
	volatile uint32_t CTRL_0146;
	volatile uint32_t CTRL_0147;
	volatile uint32_t CTRL_0150;
	volatile uint32_t CTRL_0151;
	volatile uint32_t CTRL_0152;
	volatile uint32_t CTRL_0153;
	volatile uint32_t CTRL_0154;
	volatile uint32_t CTRL_0155;
	volatile uint32_t CTRL_0156;
	volatile uint32_t CTRL_0157;
	uint32_t RSVD10[1];
	volatile uint32_t CTRL_0161;
	volatile uint32_t CTRL_0162;
	uint32_t RSVD11[2];
	volatile uint32_t CTRL_0165;
	uint32_t RSVD12[2];
	volatile uint32_t CTRL_0170;
	volatile uint32_t CTRL_0171;
	uint32_t RSVD13[3];
	volatile uint32_t CTRL_0175;
	uint32_t RSVD14[2];
	volatile uint32_t CTRL_0200;
	volatile uint32_t CTRL_0201;
	volatile uint32_t CTRL_0202;
	volatile uint32_t CTRL_0203;
	volatile uint32_t CTRL_0204;
	volatile uint32_t CTRL_0205;
	volatile uint32_t CTRL_0206;
	volatile uint32_t CTRL_0207;
	uint32_t RSVD15[9];
	volatile uint32_t CTRL_0221;
	volatile uint32_t CTRL_0222;
	volatile uint32_t CTRL_0223;
	volatile uint32_t CTRL_0224;
	uint32_t RSVD16[1];
	volatile uint32_t CTRL_0226;
	volatile uint32_t CTRL_0227;
	uint32_t RSVD17[8];
	volatile uint32_t CTRL_0240;
	volatile uint32_t CTRL_0241;
	volatile uint32_t CTRL_0242;
	volatile uint32_t CTRL_0243;
	volatile uint32_t CTRL_0244;
	volatile uint32_t CTRL_0245;
	volatile uint32_t CTRL_0246;
	uint32_t RSVD18[5];
	volatile uint32_t CTRL_0254;
	volatile uint32_t CTRL_0255;
};

/** @brief GPIO Control 2 registers by pin name */
struct gpio_ctrl2_regs {
	volatile uint32_t CTRL2_0000;
	uint32_t RSVD1[1];
	volatile uint32_t CTRL2_0002;
	volatile uint32_t CTRL2_0003;
	volatile uint32_t CTRL2_0004;
	uint32_t RSVD2[2];
	volatile uint32_t CTRL2_0007;
	volatile uint32_t CTRL2_0010;
	volatile uint32_t CTRL2_0011;
	volatile uint32_t CTRL2_0012;
	volatile uint32_t CTRL2_0013;
	volatile uint32_t CTRL2_0014;
	volatile uint32_t CTRL2_0015;
	volatile uint32_t CTRL2_0016;
	volatile uint32_t CTRL2_0017;
	volatile uint32_t CTRL2_0020;
	volatile uint32_t CTRL2_0021;
	volatile uint32_t CTRL2_0022;
	volatile uint32_t CTRL2_0023;
	volatile uint32_t CTRL2_0024;
	volatile uint32_t CTRL2_0025;
	volatile uint32_t CTRL2_0026;
	volatile uint32_t CTRL2_0027;
	volatile uint32_t CTRL2_0030;
	volatile uint32_t CTRL2_0031;
	volatile uint32_t CTRL2_0032;
	volatile uint32_t CTRL2_0033;
	volatile uint32_t CTRL2_0034;
	volatile uint32_t CTRL2_0035;
	volatile uint32_t CTRL2_0036;
	uint32_t RSVD3[1];
	volatile uint32_t CTRL2_0040;
	uint32_t RSVD4[1];
	volatile uint32_t CTRL2_0042;
	volatile uint32_t CTRL2_0043;
	volatile uint32_t CTRL2_0044;
	volatile uint32_t CTRL2_0045;
	volatile uint32_t CTRL2_0046;
	volatile uint32_t CTRL2_0047;
	volatile uint32_t CTRL2_0050;
	volatile uint32_t CTRL2_0051;
	volatile uint32_t CTRL2_0052;
	volatile uint32_t CTRL2_0053;
	volatile uint32_t CTRL2_0054;
	volatile uint32_t CTRL2_0055;
	volatile uint32_t CTRL2_0056;
	volatile uint32_t CTRL2_0057;
	volatile uint32_t CTRL2_0060;
	volatile uint32_t CTRL2_0061;
	volatile uint32_t CTRL2_0062;
	volatile uint32_t CTRL2_0063;
	volatile uint32_t CTRL2_0064;
	volatile uint32_t CTRL2_0065;
	volatile uint32_t CTRL2_0066;
	volatile uint32_t CTRL2_0067;
	volatile uint32_t CTRL2_0070;
	volatile uint32_t CTRL2_0071;
	volatile uint32_t CTRL2_0072;
	volatile uint32_t CTRL2_0073;
	uint32_t RSVD5[4];
	volatile uint32_t CTRL2_0100;
	volatile uint32_t CTRL2_0101;
	volatile uint32_t CTRL2_0102;
	uint32_t RSVD6[1];
	volatile uint32_t CTRL2_0104;
	volatile uint32_t CTRL2_0105;
	volatile uint32_t CTRL2_0106;
	volatile uint32_t CTRL2_0107;
	uint32_t RSVD7[2];
	volatile uint32_t CTRL2_0112;
	volatile uint32_t CTRL2_0113;
	volatile uint32_t CTRL2_0114;
	volatile uint32_t CTRL2_0115;
	uint32_t RSVD8[2];
	volatile uint32_t CTRL2_0120;
	volatile uint32_t CTRL2_0121;
	volatile uint32_t CTRL2_0122;
	volatile uint32_t CTRL2_0123;
	volatile uint32_t CTRL2_0124;
	volatile uint32_t CTRL2_0125;
	volatile uint32_t CTRL2_0126;
	volatile uint32_t CTRL2_0127;
	volatile uint32_t CTRL2_0130;
	volatile uint32_t CTRL2_0131;
	volatile uint32_t CTRL2_0132;
	uint32_t RSVD9[5];
	volatile uint32_t CTRL2_0140;
	volatile uint32_t CTRL2_0141;
	volatile uint32_t CTRL2_0142;
	volatile uint32_t CTRL2_0143;
	volatile uint32_t CTRL2_0144;
	volatile uint32_t CTRL2_0145;
	volatile uint32_t CTRL2_0146;
	volatile uint32_t CTRL2_0147;
	volatile uint32_t CTRL2_0150;
	volatile uint32_t CTRL2_0151;
	volatile uint32_t CTRL2_0152;
	volatile uint32_t CTRL2_0153;
	volatile uint32_t CTRL2_0154;
	volatile uint32_t CTRL2_0155;
	volatile uint32_t CTRL2_0156;
	volatile uint32_t CTRL2_0157;
	uint32_t RSVD10[1];
	volatile uint32_t CTRL2_0161;
	volatile uint32_t CTRL2_0162;
	uint32_t RSVD11[2];
	volatile uint32_t CTRL2_0165;
	uint32_t RSVD12[2];
	volatile uint32_t CTRL2_0170;
	volatile uint32_t CTRL2_0171;
	uint32_t RSVD13[3];
	volatile uint32_t CTRL2_0175;
	uint32_t RSVD14[2];
	volatile uint32_t CTRL2_0200;
	volatile uint32_t CTRL2_0201;
	volatile uint32_t CTRL2_0202;
	volatile uint32_t CTRL2_0203;
	volatile uint32_t CTRL2_0204;
	volatile uint32_t CTRL2_0205;
	volatile uint32_t CTRL2_0206;
	volatile uint32_t CTRL2_0207;
	uint32_t RSVD15[9];
	volatile uint32_t CTRL2_0221;
	volatile uint32_t CTRL2_0222;
	volatile uint32_t CTRL2_0223;
	volatile uint32_t CTRL2_0224;
	uint32_t RSVD16[1];
	volatile uint32_t CTRL2_0226;
	volatile uint32_t CTRL2_0227;
	uint32_t RSVD17[8];
	volatile uint32_t CTRL2_0240;
	volatile uint32_t CTRL2_0241;
	volatile uint32_t CTRL2_0242;
	volatile uint32_t CTRL2_0243;
	volatile uint32_t CTRL2_0244;
	volatile uint32_t CTRL2_0245;
	volatile uint32_t CTRL2_0246;
	uint32_t RSVD18[5];
	volatile uint32_t CTRL2_0254;
	volatile uint32_t CTRL2_0255;
};

/** @brief GPIO Parallel Input register. 32 GPIO pins per bank */
struct gpio_parin_regs {
	volatile uint32_t PARIN0;
	volatile uint32_t PARIN1;
	volatile uint32_t PARIN2;
	volatile uint32_t PARIN3;
	volatile uint32_t PARIN4;
	volatile uint32_t PARIN5;
};

/** @brief GPIO Parallel Output register. 32 GPIO pins per bank */
struct gpio_parout_regs {
	volatile uint32_t PAROUT0;
	volatile uint32_t PAROUT1;
	volatile uint32_t PAROUT2;
	volatile uint32_t PAROUT3;
	volatile uint32_t PAROUT4;
	volatile uint32_t PAROUT5;
};

/** @brief GPIO Lock registers. 32 GPIO pins per bank. Write-once bits */
struct gpio_lock_regs {
	volatile uint32_t LOCK5;
	volatile uint32_t LOCK4;
	volatile uint32_t LOCK3;
	volatile uint32_t LOCK2;
	volatile uint32_t LOCK1;
	volatile uint32_t LOCK0;
};

/** @brief Trace FIFO Debug Port Registers (TFDP) */
struct tfdp_regs {
	volatile uint8_t DATA_OUT;
	uint8_t RSVD1[3];
	volatile uint32_t CTRL;
};

/** @brief Mailbox Registers (MBOX) */
struct mbox_regs {
	volatile uint8_t OS_IDX;
	volatile uint8_t OS_DATA;
	uint8_t RSVD1[0x100u - 0x02u];
	volatile uint32_t HOST_TO_EC;
	volatile uint32_t EC_TO_HOST;
	volatile uint32_t SMI_SRC;
	volatile uint32_t SMI_MASK;
	union {
		volatile uint32_t MBX32[8];
		volatile uint8_t MBX8[32];
	};
};

/** @brief ACPI EC Registers (ACPI_EC) */
struct acpi_ec_regs {
	volatile uint32_t OS_DATA;
	volatile uint8_t OS_CMD_STS;
	volatile uint8_t OS_BYTE_CTRL;
	uint8_t RSVD1[0x100u - 0x06u];
	volatile uint32_t EC2OS_DATA;
	volatile uint8_t EC_STS;
	volatile uint8_t EC_BYTE_CTRL;
	uint8_t RSVD2[2];
	volatile uint32_t OS2EC_DATA;
};

/** @brief ACPI PM1 Registers (ACPI_PM1) */
struct acpi_pm1_regs {
	volatile uint8_t RT_STS1;
	volatile uint8_t RT_STS2;
	volatile uint8_t RT_EN1;
	volatile uint8_t RT_EN2;
	volatile uint8_t RT_CTRL1;
	volatile uint8_t RT_CTRL2;
	volatile uint8_t RT_CTRL21;
	volatile uint8_t RT_CTRL22;
	uint8_t RSVD1[(0x100u - 0x008u)];
	volatile uint8_t EC_STS1;
	volatile uint8_t EC_STS2;
	volatile uint8_t EC_EN1;
	volatile uint8_t EC_EN2;
	volatile uint8_t EC_CTRL1;
	volatile uint8_t EC_CTRL2;
	volatile uint8_t EC_CTRL21;
	volatile uint8_t EC_CTRL22;
	uint8_t RSVD2[(0x0110u - 0x0108u)];
	volatile uint8_t EC_PM_STS;
	uint8_t RSVD3[3];
};

/** @brief UART interface (UART) */
struct uart_regs {
	volatile uint8_t RTXB;
	volatile uint8_t IER;
	volatile uint8_t IIR_FCR;
	volatile uint8_t LCR;
	volatile uint8_t MCR;
	volatile uint8_t LSR;
	volatile uint8_t MSR;
	volatile uint8_t SCR;
	uint8_t RSVDA[0x330u - 0x08u];
	volatile uint8_t ACTV;
	uint8_t RSVDB[0x3F0u - 0x331u];
	volatile uint8_t CFG_SEL;
};

/** @brief EMI Registers (EMI) */
struct emi_regs {
	volatile uint8_t OS_H2E_MBOX;
	volatile uint8_t OS_E2H_MBOX;
	volatile uint8_t OS_EC_ADDR_LSB;
	volatile uint8_t OS_EC_ADDR_MSB;
	volatile uint32_t OS_EC_DATA;
	volatile uint8_t OS_INT_SRC_LSB;
	volatile uint8_t OS_INT_SRC_MSB;
	volatile uint8_t OS_INT_MASK_LSB;
	volatile uint8_t OS_INT_MASK_MSB;
	volatile uint32_t OS_APP_ID;
	uint8_t RSVD1[0x100u - 0x10u];
	volatile uint8_t H2E_MBOX;
	volatile uint8_t E2H_MBOX;
	uint8_t RSVD2[2];
	volatile uint32_t MEM_BASE_0;
	volatile uint32_t MEM_LIMIT_0;
	volatile uint32_t MEM_BASE_1;
	volatile uint32_t MEM_LIMIT_1;
	volatile uint16_t EC_OS_INT_SET;
	volatile uint16_t EC_OS_INT_CLR_EN;
};

/** @brief BIOS Debug Port 80h and Alias port capture registers. */
struct p80bd_regs {
	volatile uint32_t HDATA;
	uint8_t RSVD1[0x100u - 0x04u];
	volatile uint32_t EC_DA;
	volatile uint32_t CONFIG;
	volatile uint32_t STS_IEN;
	volatile uint32_t SNAPSHOT;
	volatile uint32_t CAPTURE;
	uint8_t RSVD2[0x330ul - 0x114ul];
	volatile uint32_t ACTV;
	uint8_t RSVD3[0x400ul - 0x334ul];
	volatile uint8_t ALIAS_HDATA;
	uint8_t RSVD4[0x730ul - 0x401ul];
	volatile uint32_t ALIAS_ACTV;
	uint8_t RSVD5[0x7F0ul - 0x734ul];
	volatile uint32_t ALIAS_BLS;
};

struct espi_io_mbar { /* 80-bit register */
	volatile uint16_t  LDN_MASK;
	volatile uint16_t  RESERVED[4];
}; /* Size = 10 (0xa) */

struct espi_mbar_host {
	volatile uint16_t  VALID;
	volatile uint16_t  HADDR_LSH;
	volatile uint16_t  HADDR_MSH;
	volatile uint16_t  RESERVED[2];
}; /* Size = 10 (0xa) */

struct espi_sram_bar {
	volatile uint16_t  VACCSZ;
	volatile uint16_t  EC_SRAM_BASE_LSH;
	volatile uint16_t  EC_SRAM_BASE_MSH;
	volatile uint16_t  RESERVED[2];
}; /* Size = 10 (0xa) */

struct espi_sram_host_bar {
	volatile uint16_t  ACCSZ;
	volatile uint16_t  HBASE_LSH;
	volatile uint16_t  HBASE_MSH;
	volatile uint16_t  RESERVED[2];
}; /* Size = 10 (0xa) */

struct espi_io_regs { /* @ 0x400F3400 */
	volatile uint8_t   RTIDX;		/* @ 0x0000 */
	volatile uint8_t   RTDAT;		/* @ 0x0001 */
	volatile uint16_t  RESERVED;
	volatile uint32_t  RESERVED1[63];
	volatile uint32_t  PCLC[3];		/* @ 0x0100 */
	volatile uint32_t  PCERR[2];		/* @ 0x010C */
	volatile uint32_t  PCSTS;		/* @ 0x0114 */
	volatile uint32_t  PCIEN;		/* @ 0x0118 */
	volatile uint32_t  RESERVED2;
	volatile uint32_t  PCBINH[2];		/* @ 0x0120 */
	volatile uint32_t  PCBINIT;		/* @ 0x0128 */
	volatile uint32_t  PCECIRQ;		/* @ 0x012C */
	volatile uint32_t  PCCKNP;		/* @ 0x0130 */
	volatile uint32_t  PCBARI[29];		/* @ 0x0134 */
	volatile uint32_t  RESERVED3[30];
	volatile uint32_t  PCLTRSTS;		/* @ 0x0220 */
	volatile uint32_t  PCLTREN;		/* @ 0x0224 */
	volatile uint32_t  PCLTRCTL;		/* @ 0x0228 */
	volatile uint32_t  PCLTRM;		/* @ 0x022C */
	volatile uint32_t  RESERVED4[4];
	volatile uint32_t  OOBRXA[2];		/* @ 0x0240 */
	volatile uint32_t  OOBTXA[2];		/* @ 0x0248 */
	volatile uint32_t  OOBRXL;		/* @ 0x0250 */
	volatile uint32_t  OOBTXL;		/* @ 0x0254 */
	volatile uint32_t  OOBRXC;		/* @ 0x0258 */
	volatile uint32_t  OOBRXIEN;		/* @ 0x025C */
	volatile uint32_t  OOBRXSTS;		/* @ 0x0260 */
	volatile uint32_t  OOBTXC;		/* @ 0x0264 */
	volatile uint32_t  OOBTXIEN;		/* @ 0x0268 */
	volatile uint32_t  OOBTXSTS;		/* @ 0x026C */
	volatile uint32_t  RESERVED5[4];
	volatile uint32_t  FCFA[2];		/* @ 0x0280 */
	volatile uint32_t  FCBA[2];		/* @ 0x0288 */
	volatile uint32_t  FCLEN;		/* @ 0x0290 */
	volatile uint32_t  FCCTL;		/* @ 0x0294 */
	volatile uint32_t  FCIEN;		/* @ 0x0298 */
	volatile uint32_t  FCCFG;		/* @ 0x029C */
	volatile uint32_t  FCSTS;		/* @ 0x02A0 */
	volatile uint32_t  RESERVED6[3];
	volatile uint32_t  VWSTS;		/* @ 0x02B0 */
	volatile uint32_t  RESERVED7[11];
	volatile uint8_t   CAPID;		/* @ 0x02E0 */
	volatile uint8_t   CAP0;		/* @ 0x02E1 */
	volatile uint8_t   CAP1;		/* @ 0x02E2 */
	volatile uint8_t   CAPPC;		/* @ 0x02E3 */
	volatile uint8_t   CAPVW;		/* @ 0x02E4 */
	volatile uint8_t   CAPOOB;		/* @ 0x02E5 */
	volatile uint8_t   CAPFC;		/* @ 0x02E6 */
	volatile uint8_t   PCRDY;		/* @ 0x02E7 */
	volatile uint8_t   OOBRDY;		/* @ 0x02E8 */
	volatile uint8_t   FCRDY;		/* @ 0x02E9 */
	volatile uint8_t   ERIS;		/* @ 0x02EA */
	volatile uint8_t   ERIE;		/* @ 0x02EB */
	volatile uint8_t   PLTSRC;		/* @ 0x02EC */
	volatile uint8_t   VWRDY;		/* @ 0x02ED */
	volatile uint8_t   SAFEBS;		/* @ 0x02EE */
	volatile uint8_t   RESERVED8;
	volatile uint32_t  RESERVED9[16];
	volatile uint32_t  ACTV;		/* @ 0x0330 */
	volatile uint32_t  IOHBAR[29];		/* @ 0x0334 */
	volatile uint32_t  RESERVED10[18];
	volatile uint32_t  VWERREN;		/* @ 0x03F0 */
	volatile uint32_t  RESERVED11[79];
	struct espi_io_mbar MBAR[10];		/* @ 0x0530 */
	volatile uint32_t  RESERVED12[6];
	struct espi_sram_bar SRAMBAR[2];	/* @ 0x05AC */
	volatile uint32_t  RESERVED13[16];
	volatile uint32_t  BM_STATUS;		/* @ 0x0600 */
	volatile uint32_t  BM_IEN;		/* @ 0x0604 */
	volatile uint32_t  BM_CONFIG;		/* @ 0x0608 */
	volatile uint32_t  RESERVED14;
	volatile uint32_t  BM_CTRL1;		/* @ 0x0610 */
	volatile uint32_t  BM_HADDR1_LSW;	/* @ 0x0614 */
	volatile uint32_t  BM_HADDR1_MSW;	/* @ 0x0618 */
	volatile uint32_t  BM_EC_ADDR1_LSW;	/* @ 0x061C */
	volatile uint32_t  BM_EC_ADDR1_MSW;	/* @ 0x0620 */
	volatile uint32_t  BM_CTRL2;		/* @ 0x0624 */
	volatile uint32_t  BM_HADDR2_LSW;	/* @ 0x0628 */
	volatile uint32_t  BM_HADDR2_MSW;	/* @ 0x062C */
	volatile uint32_t  BM_EC_ADDR2_LSW;	/* @ 0x0630 */
	volatile uint32_t  BM_EC_ADDR2_MSW;	/* @ 0x0634 */
	volatile uint32_t  RESERVED15[62];
	struct espi_mbar_host HMBAR[10];	/* @ 0x0730 */
	volatile uint32_t  RESERVED16[6];
	struct espi_sram_host_bar HSRAMBAR[2];	/* @ 0x07AC */
}; /* Size = 1984 (0x7c0) */

/** @brief ESPI Host interface IO Component (MCHP_ESPI_IO) @ 0x400f36b0 */
struct espi_io_cap_regs {
	volatile uint32_t VW_EN_STS;
	uint8_t RSVD1[0x36E0 - 0x36B4];
	volatile uint8_t CAP_ID;
	volatile uint8_t GLB_CAP0;
	volatile uint8_t GLB_CAP1;
	volatile uint8_t PC_CAP;
	volatile uint8_t VW_CAP;
	volatile uint8_t OOB_CAP;
	volatile uint8_t FC_CAP;
	volatile uint8_t PC_RDY;
	volatile uint8_t OOB_RDY;
	volatile uint8_t FC_RDY;
	volatile uint8_t ERST_STS;
	volatile uint8_t ERST_IEN;
	volatile uint8_t PLTRST_SRC;
	volatile uint8_t VW_RDY;
	volatile uint8_t FC_SERBZ;
	uint8_t RSVD2[0x37F0u - 0x36EE];
	volatile uint32_t VW_ERR_STS;
};

/** @brief eSPI IO Component Peripheral Channel (PC) register structure */
struct espi_io_pc_regs {
	volatile uint32_t PC_LC_ADDR_LSW;
	volatile uint32_t PC_LC_ADDR_MSW;
	volatile uint32_t PC_LC_LEN_TYPE_TAG;
	volatile uint32_t PC_ERR_ADDR_LSW;
	volatile uint32_t PC_ERR_ADDR_MSW;
	volatile uint32_t PC_STATUS;
	volatile uint32_t PC_IEN;
};

/** @brief eSPIO IO Component LTR registers @ 0x400f3620 */
struct espi_io_ltr_regs {
	volatile uint32_t LTR_STS;
	volatile uint32_t LTR_IEN;
	volatile uint32_t LTR_CTRL;
	volatile uint32_t LTR_MSG;
};

/** @brief eSPI IO Component OOB registers @ 0x400F3640 */
struct espi_io_oob_regs {
	volatile uint32_t RX_ADDR_LSW;
	volatile uint32_t RX_ADDR_MSW;
	volatile uint32_t TX_ADDR_LSW;
	volatile uint32_t TX_ADDR_MSW;
	volatile uint32_t RX_LEN;
	volatile uint32_t TX_LEN;
	volatile uint32_t RX_CTRL;
	volatile uint32_t RX_IEN;
	volatile uint32_t RX_STS;
	volatile uint32_t TX_CTRL;
	volatile uint32_t TX_IEN;
	volatile uint32_t TX_STS;
};

/** @brief eSPI IO Flash Channel registers @ 0x40003680 */
struct espi_io_fc_regs {
	volatile uint32_t FL_ADDR_LSW;
	volatile uint32_t FL_ADDR_MSW;
	volatile uint32_t MEM_ADDR_LSW;
	volatile uint32_t MEM_ADDR_MSW;
	volatile uint32_t XFR_LEN;
	volatile uint32_t CTRL;
	volatile uint32_t IEN;
	volatile uint32_t CFG;
	volatile uint32_t STS;
};

/** @brief eSPI IO component registers related to VW channel @ 0x400F36B0 */
struct espi_io_vw_en {
	volatile uint32_t VW_EN_STS;
	uint32_t RSVD1[12];
	volatile uint8_t VW_CAP;
	uint8_t RSVD2[8];
	volatile uint8_t VW_RDY;
	uint8_t RSVD3[0x102];
	volatile uint32_t VW_ERR_STS;
};

/** @brief eSPI IO BAR Host registers at 0x400f3520 */
struct espi_io_bar_host_regs {
	volatile uint32_t IOBAR_INH_LSW;
	volatile uint32_t IOBAR_INH_MSW;
	volatile uint32_t IOBAR_INIT;
	volatile uint32_t EC_IRQ;
	uint32_t RSVD1[1];
	volatile uint32_t IOBAR[23];
};

/** @brief ESPI_IO_BAR_EC - eSPI IO EC-only component of IO BAR @ 0x400F3730 */
struct espi_io_bar_ec_regs {
	volatile uint32_t IO_ACTV;
	volatile uint32_t IOBAR[23];
};

/** @brief ESPI_IO_BAR_EC - eSPI IO EC-only component of IO BAR @ 0x400F3730
 * use named registers
 */
struct espi_io_bar_ec_named_regs {
	volatile uint32_t IO_ACTV;
	volatile uint32_t EC_BAR_IOC;
	volatile uint32_t EC_BAR_MEM;
	volatile uint32_t EC_BAR_MBOX;
	volatile uint32_t EC_BAR_KBC;
	volatile uint32_t EC_BAR_ACPI_EC_0;
	volatile uint32_t EC_BAR_ACPI_EC_1;
	volatile uint32_t EC_BAR_ACPI_EC_2;
	volatile uint32_t EC_BAR_ACPI_EC_3;
	volatile uint32_t EC_BAR_ACPI_EC_4;
	volatile uint32_t EC_BAR_ACPI_PM1;
	volatile uint32_t EC_BAR_PORT92;
	volatile uint32_t EC_BAR_UART_0;
	volatile uint32_t EC_BAR_UART_1;
	volatile uint32_t EC_BAR_EMI_0;
	volatile uint32_t EC_BAR_EMI_1;
	volatile uint32_t EC_BAR_EMI_2;
	volatile uint32_t EC_BAR_P80CAP_0;
	volatile uint32_t EC_BAR_P80CAP_1;
	volatile uint32_t EC_BAR_RTC;
	uint32_t RSVD1[1];
	volatile uint32_t EC_BAR_T32B;
	uint32_t RSVD2[1];
	volatile uint32_t EC_BAR_GLUE_LOG;
};

/** @brief eSPI IO Component Logical Device Serial IRQ config @ 0x400F37A0
 * SIRQ index with enum espi_io_sirq_idx
 */
struct espi_io_sirq_regs {
	uint8_t RSVD1[12];
	uint8_t SIRQ[20];
};

struct espi_io_sirq_named_regs {
	uint8_t RSVD1[12];
	volatile uint8_t MBOX_SIRQ_0;
	volatile uint8_t MBOX_SIRQ_1;
	volatile uint8_t KBC_SIRQ_0;
	volatile uint8_t KBC_SIRQ_1;
	volatile uint8_t ACPI_EC_0_SIRQ;
	volatile uint8_t ACPI_EC_1_SIRQ;
	volatile uint8_t ACPI_EC_2_SIRQ;
	volatile uint8_t ACPI_EC_3_SIRQ;
	volatile uint8_t ACPI_EC_4_SIRQ;
	volatile uint8_t UART_0_SIRQ;
	volatile uint8_t UART_1_SIRQ;
	volatile uint8_t EMI_0_SIRQ_0;
	volatile uint8_t EMI_0_SIRQ_1;
	volatile uint8_t EMI_1_SIRQ_0;
	volatile uint8_t EMI_1_SIRQ_1;
	volatile uint8_t EMI_2_SIRQ_0;
	volatile uint8_t EMI_2_SIRQ_1;
	volatile uint8_t RTC_SIRQ;
	volatile uint8_t EC_SIRQ;
	uint8_t RSVD2[1];
};

/** @brief eSPI Memory Component Bus Master registers @ 0x400F3A00 */
struct espi_mem_bm_regs {
	volatile uint32_t BM_STS;
	volatile uint32_t BM_IEN;
	volatile uint32_t BM_CFG;
	uint8_t RSVD1[4];
	volatile uint32_t BM1_CTRL;
	volatile uint32_t BM1_HOST_ADDR_LSW;
	volatile uint32_t BM1_HOST_ADDR_MSW;
	volatile uint32_t BM1_EC_ADDR_LSW;
	volatile uint32_t BM1_EC_ADDR_MSW;
	volatile uint32_t BM2_CTRL;
	volatile uint32_t BM2_HOST_ADDR_LSW;
	volatile uint32_t BM2_HOST_ADDR_MSW;
	volatile uint32_t BM2_EC_ADDR_LSW;
	volatile uint32_t BM2_EC_ADDR_MSW;
};

/** @brief eSPI Memory Component Memory BAR EC-only registers @ 0x400F3930 */
struct espi_mem_bar_ec_regs {
	volatile uint16_t EMBAR[9 * 5];
};

/** @brief eSPI Memory Component Memory BAR Host registers @ 0x400F3B30 */
struct espi_mem_bar_host_regs {
	volatile uint16_t HMBAR[9 * 5];
};

/** @brief eSPI Memory Component SRAM 0 and 1 EC BAR's @ 0x400F39A0 */
struct espi_mem_sram_bar_ec_regs {
	uint32_t RSVD1[3];
	volatile uint16_t ESMB[2 * 5];
};

/** @brief eSPI Memory Component SRAM 0 and 1 Host BAR's @ 0x400F3BA0 */
struct espi_mem_sram_bar_host_regs {
	uint32_t RSVD1[3];
	volatile uint16_t HSMB[2 * 5];
};

/** @brief eSPI 96-bit Master-to-Slave Virtual Wire register */
struct espi_msvw_reg {
	volatile uint8_t INDEX;
	volatile uint8_t MTOS;
	uint8_t RSVD1[2];
	volatile uint32_t SRC_IRQ_SEL;
	volatile uint32_t SRC;
};

/** @brief HW implements 11 Master-to-Slave VW registers as an array */
struct espi_msvw_ar_regs {
	struct espi_msvw_reg MSVW[11];
};

/** @brief HW implements 11 Master-to-Slave VW registers as named registers */
struct espi_msvw_named_regs {
	struct espi_msvw_reg MSVW00;
	struct espi_msvw_reg MSVW01;
	struct espi_msvw_reg MSVW02;
	struct espi_msvw_reg MSVW03;
	struct espi_msvw_reg MSVW04;
	struct espi_msvw_reg MSVW05;
	struct espi_msvw_reg MSVW06;
	struct espi_msvw_reg MSVW07;
	struct espi_msvw_reg MSVW08;
	struct espi_msvw_reg MSVW09;
	struct espi_msvw_reg MSVW10;
};

/** @brief eSPI M2S VW registers as an array of words at 0x400F9C00 */
struct espi_msvw32_regs {
	volatile uint32_t MSVW32[11 * 3];
};

/** @brief eSPI 64-bit Slave-to-Master Virtual Wire register */
struct espi_smvw_reg {
	volatile uint8_t INDEX;
	volatile uint8_t STOM;
	volatile uint8_t SRC_CHG;
	uint8_t RSVD1[1];
	volatile uint32_t SRC;
};

/** @brief HW implements 11 Slave-to-Master VW registers as an array */
struct esp_smvws_reg {
	struct espi_smvw_reg SMVW[11];
};

/** @brief HW implements 11 Slave-to-Master VW registers as named registers */
struct espi_smvw_named_regs {
	struct espi_smvw_reg SMVW00;
	struct espi_smvw_reg SMVW01;
	struct espi_smvw_reg SMVW02;
	struct espi_smvw_reg SMVW03;
	struct espi_smvw_reg SMVW04;
	struct espi_smvw_reg SMVW05;
	struct espi_smvw_reg SMVW06;
	struct espi_smvw_reg SMVW07;
	struct espi_smvw_reg SMVW08;
	struct espi_smvw_reg SMVW09;
	struct espi_smvw_reg SMVW10;
};

/** @brief eSPI S2M VW registers as an array of words at 0x400F9E00 */
struct espi_smvw32_regs {
	volatile uint32_t SMVW[11 * 2];
};

/** @brief eSPI Host to Device 96-bit register: controls 4 Virtual Wires */
struct espi_vw_h2d_reg {
	volatile uint32_t  IRSS;
	volatile uint32_t  IRQ_SELECT;
	volatile uint32_t  SRC;
}; /* Size = 12 (0xc) */

/** @brief Device to eSPI Host 64-bit register: controls 4 Virtual Wires */
struct espi_vw_d2h_reg {
	volatile uint32_t  IRSCH;
	volatile uint32_t  SRC;
}; /* Size = 8 (0x8) */

struct espi_vw_regs { /* @ 0x400F9C00 */
	struct espi_vw_h2d_reg HDVW[11]; /* @ 0x0000 Host-to-Device VW */
	uint32_t  RESERVED[95];
	struct espi_vw_d2h_reg DHVW[11]; /* @ 0x0200 Device-to-Host VW */
}; /* Size = 600 (0x258) */

/** @brief SAF SPI Opcodes and descriptor indices */
struct mchp_espi_saf_op {
	volatile uint32_t OPA;
	volatile uint32_t OPB;
	volatile uint32_t OPC;
	volatile uint32_t OP_DESCR;
};

/** @brief SAF protection regions contain 4 32-bit registers. */
struct mchp_espi_saf_pr {
	volatile uint32_t START;
	volatile uint32_t LIMIT;
	volatile uint32_t WEBM;
	volatile uint32_t RDBM;
};

/** @brief eSPI SAF configuration and control registers at 0x40008000 */
struct mchp_espi_saf {
	uint32_t RSVD1[6];
	volatile uint32_t SAF_ECP_CMD;
	volatile uint32_t SAF_ECP_FLAR;
	volatile uint32_t SAF_ECP_START;
	volatile uint32_t SAF_ECP_BFAR;
	volatile uint32_t SAF_ECP_STATUS;
	volatile uint32_t SAF_ECP_INTEN;
	volatile uint32_t SAF_FL_CFG_SIZE_LIM;
	volatile uint32_t SAF_FL_CFG_THRH;
	volatile uint32_t SAF_FL_CFG_MISC;
	volatile uint32_t SAF_ESPI_MON_STATUS;
	volatile uint32_t SAF_ESPI_MON_INTEN;
	volatile uint32_t SAF_ECP_BUSY;
	uint32_t RSVD2[1];
	struct mchp_espi_saf_op SAF_CS_OP[2];
	volatile uint32_t SAF_FL_CFG_GEN_DESCR;
	volatile uint32_t SAF_PROT_LOCK;
	volatile uint32_t SAF_PROT_DIRTY;
	volatile uint32_t SAF_TAG_MAP[3];
	struct mchp_espi_saf_pr SAF_PROT_RG[17];
	volatile uint32_t SAF_POLL_TMOUT;
	volatile uint32_t SAF_POLL_INTRVL;
	volatile uint32_t SAF_SUS_RSM_INTRVL;
	volatile uint32_t SAF_CONSEC_RD_TMOUT;
	volatile uint16_t SAF_CS0_CFG_P2M;
	volatile uint16_t SAF_CS1_CFG_P2M;
	volatile uint32_t SAF_FL_CFG_SPM;
	volatile uint32_t SAF_SUS_CHK_DLY;
	volatile uint16_t SAF_CS0_CM_PRF;
	volatile uint16_t SAF_CS1_CM_PRF;
	volatile uint32_t SAF_DNX_PROT_BYP;
};

/* Peripheral and SRAM base address */
#define CODE_SRAM_BASE		0x000C0000UL
#define DATA_SRAM_BASE		0x00118000UL
#define DATA_SRAM_SIZE		0x10000UL
#define PERIPH_BASE		0x40000000UL

/* Peripheral memory map */
#define WDT_BASE		(PERIPH_BASE + 0x0400ul)
#define BTMR16_0_BASE		(PERIPH_BASE + 0x0C00ul)
#define BTMR16_1_BASE		(PERIPH_BASE + 0x0C20ul)
#define BTMR16_2_BASE		(PERIPH_BASE + 0x0C40ul)
#define BTMR16_3_BASE		(PERIPH_BASE + 0x0C60ul)
#define BTMR32_0_BASE		(PERIPH_BASE + 0x0C80ul)
#define BTMR32_1_BASE		(PERIPH_BASE + 0x0CA0ul)
#define BTMR16_BASE(n)		(BTMR16_0_BASE + ((uint32_t)(n) * 0x20ul))
#define BTMR32_BASE(n)		(BTMR32_0_BASE + ((uint32_t)(n) * 0x20ul))
#define CTMR_BASE(n)		(PERIPH_BASE + 0x0D00ul + ((n) * 0x20u))
#define CCT_0_BASE		(PERIPH_BASE + 0x1000ul)
#define RCID_BASE(n)		(PERIPH_BASE + 0x1400ul + \
				((uint32_t)(n) * 0x80))
#define DMA_BASE		(PERIPH_BASE + 0x2400ul)
#define DMA_CHAN_BASE(n)	(DMA_BASE + (((n) + 1) * 0x40U))
#define EEPROMC_BASE		(PERIPH_BASE + 0x2C00ul)
#define PROCHOT_BASE		(PERIPH_BASE + 0x3400ul)
#define I2C_SMB_BASE(n)		(PERIPH_BASE + 0x4000ul + ((n) * 0x400UL))
#define PWM_BASE(n)		(PERIPH_BASE + 0x5800ul + ((n) * 0x10UL))
#define TACH_BASE(n)		(PERIPH_BASE + 0x6000ul + ((n) * 0x10UL))
#define PECI_0_BASE		(PERIPH_BASE + 0x6400ul)
#define SPIP_0_BASE		(PERIPH_BASE + 0x7000ul)
#define RTMR_0_BASE		(PERIPH_BASE + 0x7400ul)
#define ADC_0_BASE		(PERIPH_BASE + 0x7C00ul)
#define ESPI_SAF_BASE		(PERIPH_BASE + 0x8000ul)
#define TFDP_0_BASE		(PERIPH_BASE + 0x8C00ul)
#define PS2_0_BASE		(PERIPH_BASE + 0x9000ul)
#define GPSPI_BASE(n)		(PERIPH_BASE + 0x9400ul + ((n) * 0x80U))
#define HTMR_BASE(n)		(PERIPH_BASE + 0x9800ul + ((n) * 0x20U))
#define KEYSCAN_BASE		(PERIPH_BASE + 0x9C00ul)
#define RPMFAN_BASE(n)		(PERIPH_BASE + 0xA000ul + ((n) * 0x80U))
#define VBATR_BASE		(PERIPH_BASE + 0xA400ul)
#define VBATM_BASE		(PERIPH_BASE + 0xA800ul)
#define WKTMR_BASE		(PERIPH_BASE + 0xAC80ul)
#define VCI_BASE		(PERIPH_BASE + 0xAE00ul)
#define LED_BASE(n)		(PERIPH_BASE + 0xB800ul + ((n) * 0x100U))
#define BCL_0_BASE		(PERIPH_BASE + 0xCD00ul)
#define ECIA_BASE		(PERIPH_BASE + 0xE000ul)
/*!< (GIRQ 8<=n<=26) address */
#define GIRQ_BASE(n)		(ECIA_BASE + (((n) - 8U) * 0x14ul))
#define ECS_BASE		(PERIPH_BASE + 0xFC00ul)
#define CACHE_CTRL_BASE		(PERIPH_BASE + 0x10000ul)

/* AHB Segment 7 */
#define QMSPI_0_BASE		(PERIPH_BASE + 0x70000ul)

/* AHB Segment 8 */
#define PCR_BASE		(PERIPH_BASE + 0x80100ul)
#define GPIO_BASE		(PERIPH_BASE + 0x81000ul)
#define GPIO_CTRL_BASE		(GPIO_BASE)
#define GPIO_PARIN_BASE		(GPIO_BASE + 0x0300ul)
#define GPIO_PAROUT_BASE	(GPIO_BASE + 0x0380ul)
#define GPIO_LOCK_BASE		(GPIO_BASE + 0x03E8ul)
#define GPIO_CTRL2_BASE		(GPIO_BASE + 0x0500ul)
#define OTP_BASE		(PERIPH_BASE + 0x82000ul)

/* AHB Segment 0xF Host Peripherals */
#define MBOX_0_BASE		(PERIPH_BASE + 0xF0000ul)
#define KBC_0_BASE		(PERIPH_BASE + 0xF0400ul)
/*!< (ACPI EC n ) Base Address 0 <= n <= 4 */
#define ACPI_EC_BASE(n)		(PERIPH_BASE + 0xF0800ul + ((n) * 0x400UL))
#define ACPI_PM1_BASE		(PERIPH_BASE + 0xF1C00ul)
#define PORT92_BASE		(PERIPH_BASE + 0xF2000ul)
#define UART_BASE(n)		(PERIPH_BASE + 0xF2400ul + ((n) * 0x400UL))
#define GLUE_BASE		(PERIPH_BASE + 0xF3C00ul)
#define EMI_BASE(n)		(PERIPH_BASE + 0xF4000ul + ((n) * 0x400U))
#define RTC_BASE		(PERIPH_BASE + 0xF5000ul)
#define P80BD_0_BASE		(PERIPH_BASE + 0xF8000ul)
#define GCFG_BASE		(PERIPH_BASE + 0xFFF00ul)

/* -------- eSPI Base Addresses -------- */
#define ESPI_IO_BASE		(PERIPH_BASE + 0xF3400ul)
#define ESPI_IO_PC_BASE		((ESPI_IO_BASE) + 0x100ul)
#define ESPI_IO_HOST_BAR_BASE	((ESPI_IO_BASE) + 0x120ul)
#define ESPI_IO_LTR_BASE	((ESPI_IO_BASE) + 0x220ul)
#define ESPI_IO_OOB_BASE	((ESPI_IO_BASE) + 0x240ul)
#define ESPI_IO_FC_BASE		((ESPI_IO_BASE) + 0x280ul)
#define ESPI_IO_CAP_BASE	((ESPI_IO_BASE) + 0x2B0ul)
#define ESPI_IO_EC_BAR_BASE	((ESPI_IO_BASE) + 0x330ul)
#define ESPI_IO_VW_BASE		((ESPI_IO_BASE) + 0x2B0ul)
#define ESPI_IO_SIRQ_BASE	((ESPI_IO_BASE) + 0x3A0ul)

#define ESPI_MEM_BASE			(PERIPH_BASE + 0xF3800ul)
#define ESPI_MEM_EC_BAR_BASE		((ESPI_MEM_BASE) + 0x0130ul)
#define ESPI_MEM_HOST_BAR_BASE		((ESPI_MEM_BASE) + 0x0330ul)
#define ESPI_MEM_SRAM_EC_BAR_BASE	((ESPI_MEM_BASE) + 0x01A0ul)
#define ESPI_MEM_SRAM_HOST_BAR_BASE	((ESPI_MEM_BASE) + 0x03A0ul)
#define ESPI_MEM_BM_BASE		((ESPI_MEM_BASE) + 0x0200ul)

#define ESPI_VW_BASE		(PERIPH_BASE + 0xF9C00ul)
#define ESPI_SMVW_BASE		(ESPI_VW_BASE + 0x200ul)

/* 1 us Delay register address */
#define DELAY_US_BASE (0x08000000ul)

/* ARM Cortex-M4 input clock from PLL */
#define MCHP_EC_CLOCK_INPUT_HZ 96000000U

#define MCHP_ACMP_INSTANCES 1
#define MCHP_ACPI_EC_INSTANCES 5
#define MCHP_ACPI_PM1_INSTANCES 1
#define MCHP_ADC_INSTANCES 1
#define MCHP_BCL_INSTANCES 1
#define MCHP_BTMR16_INSTANCES 4
#define MCHP_BTMR32_INSTANCES 2
#define MCHP_CCT_INSTANCES 1
#define MCHP_CTMR_INSTANCES 4
#define MCHP_DMA_INSTANCES 1
#define MCHP_ECIA_INSTANCES 1
#define MCHP_EMI_INSTANCES 3
#define MCHP_HTMR_INSTANCES 2
#define MCHP_I2C_INSTANCES 0
#define MCHP_I2C_SMB_INSTANCES 5
#define MCHP_LED_INSTANCES 4
#define MCHP_MBOX_INSTANCES 1
#define MCHP_OTP_INSTANCES 1
#define MCHP_P80BD_INSTANCES 1
#define MCHP_PECI_INSTANCES 1
#define MCHP_PROCHOT_INSTANCES 1
#define MCHP_PS2_INSTANCES 1
#define MCHP_PWM_INSTANCES 9
#define MCHP_QMSPI_INSTANCES 1
#define MCHP_RCID_INSTANCES 3
#define MCHP_RPMFAN_INSTANCES 2
#define MCHP_RTC_INSTANCES 1
#define MCHP_RTMR_INSTANCES 1
#define MCHP_SPIP_INSTANCES 1
#define MCHP_TACH_INSTANCES 4
#define MCHP_TFDP_INSTANCES 1
#define MCHP_UART_INSTANCES 2
#define MCHP_WDT_INSTANCES 1
#define MCHP_WKTMR_INSTANCES 1

#define MCHP_ACMP_CHANNELS 2
#define MCHP_ADC_CHANNELS 8
#define MCHP_BGPO_GPIO_PINS 2
#define MCHP_DMA_CHANNELS 16
#define MCHP_GIRQS 19
#define MCHP_GPIO_PINS 123
#define MCHP_GPIO_PORTS 6
#define MCHP_GPTP_PORTS 6
#define MCHP_I2C_SMB_PORTS 15
#define MCHP_I2C_PORTMAP 0xF7FFul;
#define MCHP_QMSPI_PORTS 3
#define MCHP_PS2_PORTS 2
#define MCHP_VCI_IN_PINS 4
#define MCHP_VCI_OUT_PINS 1
#define MCHP_VCI_OVRD_IN_PINS 1

/* Peripheral instantiations */
#define WDT_REGS	((struct wdt_regs *)WDT_BASE)

#define BTMR16_0_REGS	((struct btmr_regs *)BTMR16_BASE(0))
#define BTMR16_1_REGS	((struct btmr_regs *)BTMR16_BASE(1))
#define BTMR16_2_REGS	((struct btmr_regs *)BTMR16_BASE(2))
#define BTMR16_3_REGS	((struct btmr_regs *)BTMR16_BASE(3))
#define BTMR32_0_REGS	((struct btmr_regs *)BTMR32_BASE(0))
#define BTMR32_1_REGS	((struct btmr_regs *)BTMR32_BASE(1))

#define CTMR_0_REGS	((struct ctmr_regs *)CTMR_BASE(0))
#define CTMR_1_REGS	((struct ctmr_regs *)CTMR_BASE(1))
#define CTMR_2_REGS	((struct ctmr_regs *)CTMR_BASE(2))
#define CTMR_3_REGS	((struct ctmr_regs *)CTMR_BASE(3))

#define CCT_0_REGS	((struct cct_regs *)(CCT_0_BASE))

#define RCID_0_REGS	((struct rcid_regs *)RCID_BASE(0))
#define RCID_1_REGS	((struct rcid_regs *)RCID_BASE(1))
#define RCID_2_REGS	((struct rcid_regs *)RCID_BASE(2))

/* Complete DMA block */
#define DMA_REGS	((struct dma_regs *)(DMA_BASE))
/* DMA Main only */
#define DMAM_REGS	((struct dma_main_regs *)(DMA_BASE))
/* Individual DMA channels */
#define DMA0_REGS	((struct dma_chan_alu_regs *)(DMA_CHAN_BASE(0)))
#define DMA1_REGS	((struct dma_chan_alu_regs *)(DMA_CHAN_BASE(1)))
#define DMA2_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(2)))
#define DMA3_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(3)))
#define DMA4_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(4)))
#define DMA5_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(5)))
#define DMA6_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(6)))
#define DMA7_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(7)))
#define DMA8_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(8)))
#define DMA9_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(9)))
#define DMA10_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(10)))
#define DMA11_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(11)))
#define DMA12_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(12)))
#define DMA13_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(13)))
#define DMA14_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(14)))
#define DMA15_REGS	((struct dma_chan_regs *)(DMA_CHAN_BASE(15)))

#define EEPROMC_REGS	((struct eepromc_regs *)EEPROMC_BASE)

#define PROCHOT_REGS	((struct prochot_regs *)PROCHOT_BASE)

#define I2C_SMB_0_REGS	((struct i2c_smb_regs *)I2C_SMB_BASE(0))
#define I2C_SMB_1_REGS	((struct i2c_smb_regs *)I2C_SMB_BASE(1))
#define I2C_SMB_2_REGS	((struct i2c_smb_regs *)I2C_SMB_BASE(2))
#define I2C_SMB_3_REGS	((struct i2c_smb_regs *)I2C_SMB_BASE(3))
#define I2C_SMB_4_REGS	((struct i2c_smb_regs *)I2C_SMB_BASE(4))

#define PWM_0_REGS	((struct pwm_regs *)PWM_BASE(0))
#define PWM_1_REGS	((struct pwm_regs *)PWM_BASE(1))
#define PWM_2_REGS	((struct pwm_regs *)PWM_BASE(2))
#define PWM_3_REGS	((struct pwm_regs *)PWM_BASE(3))
#define PWM_4_REGS	((struct pwm_regs *)PWM_BASE(4))
#define PWM_5_REGS	((struct pwm_regs *)PWM_BASE(5))
#define PWM_6_REGS	((struct pwm_regs *)PWM_BASE(6))
#define PWM_7_REGS	((struct pwm_regs *)PWM_BASE(7))
#define PWM_8_REGS	((struct pwm_regs *)PWM_BASE(8))

#define TACH_0_REGS	((struct tach_regs *)TACH_BASE(0))
#define TACH_1_REGS	((struct tach_regs *)TACH_BASE(1))
#define TACH_2_REGS	((struct tach_regs *)TACH_BASE(2))
#define TACH_3_REGS	((struct tach_regs *)TACH_BASE(3))

#define PECI_REGS	((struct peci_regs *)PECI_0_BASE)

#define SPIP_0_REGS	((struct spip_regs *)SPIP_0_BASE)

#define RTMR_0_REGS	((struct rtmr_regs *)RTMR_0_BASE)

#define ADC_0_REGS	((struct adc_regs *)ADC_0_BASE)

#define TFDP_0_REGS	((struct tfdp_regs *)TFDP_0_BASE)

#define PS2_0_REGS	((struct ps2_regs *)PS2_0_BASE)

#define GPSPI_0_REGS	((struct gpspi_regs *)GPSPI_BASE(0))
#define GPSPI_1_REGS	((struct gpspi_regs *)GPSPI_BASE(1))

#define HTMR_0_REGS	((struct htmr_regs *)HTMR_BASE(0))
#define HTMR_1_REGS	((struct htmr_regs *)HTMR_BASE(1))

#define KSCAN_REGS	((struct kscan_regs *)(KEYSCAN_BASE))

#define RPMFAN_0_REGS	((struct rpmfan_regs *)RPMFAN_BASE(0))
#define RPMFAN_1_REGS	((struct rpmfan_regs *)RPMFAN_BASE(1))

#define VBATR_REGS	((struct vbatr_regs *)VBATR_BASE)
#define VBATM_REGS	((struct vbatm_regs *)VBATM_BASE)

#define WKTMR_REGS	((struct wktmr_regs *)WKTMR_BASE)

#define VCI_REGS	((struct vci_regs *)VCI_BASE)

#define LED_0_REGS	((struct led_regs *)LED_BASE(0))
#define LED_1_REGS	((struct led_regs *)LED_BASE(1))
#define LED_2_REGS	((struct led_regs *)LED_BASE(2))
#define LED_3_REGS	((struct led_regs *)LED_BASE(3))

#define BCL_0_REGS	((struct bcl_regs *)BCL_0_BASE)

#define ECIA_REGS	((struct ecia_regs *)ECIA_BASE)
#define ECIA_AR_REGS	((struct ecia_ar_regs *)ECIA_BASE)
#define GIRQ08_REGS	((struct girq_regs *)GIRQ_BASE(8))
#define GIRQ09_REGS	((struct girq_regs *)GIRQ_BASE(9))
#define GIRQ10_REGS	((struct girq_regs *)GIRQ_BASE(10))
#define GIRQ11_REGS	((struct girq_regs *)GIRQ_BASE(11))
#define GIRQ12_REGS	((struct girq_regs *)GIRQ_BASE(12))
#define GIRQ13_REGS	((struct girq_regs *)GIRQ_BASE(13))
#define GIRQ14_REGS	((struct girq_regs *)GIRQ_BASE(14))
#define GIRQ15_REGS	((struct girq_regs *)GIRQ_BASE(15))
#define GIRQ16_REGS	((struct girq_regs *)GIRQ_BASE(16))
#define GIRQ17_REGS	((struct girq_regs *)GIRQ_BASE(17))
#define GIRQ18_REGS	((struct girq_regs *)GIRQ_BASE(18))
#define GIRQ19_REGS	((struct girq_regs *)GIRQ_BASE(19))
#define GIRQ20_REGS	((struct girq_regs *)GIRQ_BASE(20))
#define GIRQ21_REGS	((struct girq_regs *)GIRQ_BASE(21))
#define GIRQ22_REGS	((struct girq_regs *)GIRQ_BASE(22))
#define GIRQ23_REGS	((struct girq_regs *)GIRQ_BASE(23))
#define GIRQ24_REGS	((struct girq_regs *)GIRQ_BASE(24))
#define GIRQ25_REGS	((struct girq_regs *)GIRQ_BASE(25))
#define GIRQ26_REGS	((struct girq_regs *)GIRQ_BASE(26))

#define ECS_REGS	((struct ecs_regs *)ECS_BASE)

#define QMSPI_REGS	((struct qmspi_regs *)QMSPI_0_BASE)

#define PCR_REGS	((struct pcr_regs *)PCR_BASE)

#define GPIO_REGS		((struct gpio_regs *)(GPIO_CTRL_BASE))
#define GPIO_CTRL_REGS		((struct gpio_ctrl_regs *)(GPIO_CTRL_BASE))
#define GPIO_CTRL2_REGS		((struct gpio_ctrl2_regs *)(GPIO_CTRL2_BASE))
#define GPIO_PARIN_REGS		((struct gpio_parin_regs *)(GPIO_PARIN_BASE))
#define GPIO_PAROUT_REGS	((struct gpio_parout_regs *)(GPIO_PAROUT_BASE))
#define GPIO_LOCK_REGS		((struct gpio_lock_regs *)(GPIO_LOCK_BASE))

#define MBOX_0_REGS	((struct mbox_regs *)(MBOX_0_BASE))

#define KBC_0_REGS	((struct kbc_regs *)(KBC_0_BASE))
#define KBC_REGS	((struct kbc_regs *)(KBC_0_BASE)) /* compatibility */

#define ACPI_EC_0_REGS	((struct acpi_ec_regs *)(ACPI_EC_BASE(0)))
#define ACPI_EC_1_REGS	((struct acpi_ec_regs *)(ACPI_EC_BASE(1)))
#define ACPI_EC_2_REGS	((struct acpi_ec_regs *)(ACPI_EC_BASE(2)))
#define ACPI_EC_3_REGS	((struct acpi_ec_regs *)(ACPI_EC_BASE(3)))
#define ACPI_EC_4_REGS	((struct acpi_ec_regs *)(ACPI_EC_BASE(4)))

#define ACPI_PM1_REGS	((struct acpi_pm1_regs *)ACPI_PM1_BASE)

#define PORT92_REGS	((struct port92_regs *)(PORT92_BASE))

#define UART_0_REGS	((struct uart_regs *)UART_BASE(0))
#define UART_1_REGS	((struct uart_regs *)UART_BASE(1))
/* compatibility */
#define UART0_REGS	((struct uart_regs *)UART_BASE(0))
#define UART1_REGS	((struct uart_regs *)UART_BASE(1))

#define GLUE_REGS	((struct glue_regs *)(GLUE_BASE))

#define EMI_0_REGS	((struct emi_regs *)(EMI_BASE(0)))
#define EMI_1_REGS	((struct emi_regs *)(EMI_BASE(1)))
#define EMI_2_REGS	((struct emi_regs *)(EMI_BASE(2)))

#define RTC_REGS	((struct rtc_regs *)RTC_BASE)

#define P80BD_0_REGS	((struct p80bd_regs *)(P80BD_0_BASE))

#define GLOBAL_CFG_REGS	((struct global_cfg_regs *)GCFG_BASE)

/* -------- eSPI Register instaniations -------- */
#define ESPI_PC_REGS		((struct espi_io_pc_regs *)(ESPI_IO_PC_BASE))

#define ESPI_HIO_BAR_REGS	\
	((struct espi_io_bar_host_regs *)(ESPI_IO_HOST_BAR_BASE))

#define ESPI_LTR_REGS		((struct espi_io_ltr_regs *)(ESPI_IO_LTR_BASE))
#define ESPI_OOB_REGS		((struct espi_io_oob_regs *)(ESPI_IO_OOB_BASE))
#define ESPI_FC_REGS		((struct espi_io_fc_regs *)(ESPI_IO_FC_BASE))
#define ESPI_CAP_REGS		((struct espi_io_cap_regs *)(ESPI_IO_CAP_BASE))
#define ESPI_EIO_BAR_REGS	\
	((struct espi_io_bar_ec_named_regs *)(ESPI_IO_EC_BAR_BASE))
#define ESPI_SIRQ_REGS		\
	((struct espi_io_sirq_named_regs *)(ESPI_IO_SIRQ_BASE))

#define ESPI_MEM_EBAR_REGS	\
	((struct espi_mem_bar_ec_regs *)(ESPI_MEM_EC_BAR_BASE))
#define ESPI_MEM_HBAR_REGS	\
	((struct espi_mem_bar_host_regs *)(ESPI_MEM_HOST_BAR_BASE))

#define ESPI_MEM_SRAM_EBAR_REGS	\
	((struct espi_mem_sram_bar_ec_regs)(ESPI_MEM_SRAM_EC_BAR_BASE))
#define ESPI_MEM_SRAM_HBAR_REGS	\
	((struct espi_mem_sram_bar_host_regs *)(ESPI_MEM_SRAM_HOST_BAR_BASE))

#define ESPI_MEM_BM_REGS ((struct espi_mem_bm_regs *)(ESPI_MEM_BM_BASE))

/* eSPI Virtual Wire registers in IO component */
#define ESPI_IO_VW_REGS		((struct espi_io_vw_en *)(ESPI_IO_VW_BASE))

/* eSPI Virtual Wire registers for each group of 4 VWires */
#define ESPI_M2S_VW_REGS	((struct espi_msvw_named_regs *)(ESPI_VW_BASE))
#define ESPI_S2M_VW_REGS	\
	((struct espi_smvw_named_regs *)(ESPI_SMVW_BASE))

#ifdef __cplusplus
}
#endif
#endif /* MEC172X_REGS_H */
