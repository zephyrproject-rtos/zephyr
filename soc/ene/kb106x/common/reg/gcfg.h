/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_GCFG_H
#define ENE_KB106X_GCFG_H

/**
 *  Structure type to access General Configuration (GCFG).
 */
struct gcfg_regs {
	volatile uint8_t IDV;             /*Version ID Register */
	volatile uint8_t Reserved0;       /*Reserved */
	volatile uint16_t IDC;            /*Chip ID Register */
	volatile uint32_t FWID;           /*Firmware ID Register */
	volatile uint32_t MCURST;         /*MCU Reset Control Register */
	volatile uint32_t RSTFLAG;        /*Reset Pending Flag Register */
	volatile uint32_t GPIOALT;        /*GPIO Alternate Register */
	volatile uint8_t GPIOBPC1513;     /*GPIO Bypass Control Register */
	volatile uint8_t GPIOBPC1718;     /*GPIO Bypass Control Register */
	volatile uint8_t GPIOBPC7E6B;     /*GPIO Bypass Control Register */
	volatile uint8_t Reserved1;       /*Reserved */
	volatile uint16_t GPIOMUX;        /*GPIO MUX Control Register */
	volatile uint16_t Reserved2;      /*Reserved */
	volatile uint8_t I2CSPMS;         /*I2CS Pin Map Selection Register */
	volatile uint8_t Reserved3[3];    /*Reserved */
	volatile uint8_t CLKCFG;          /*Clock Configuration Register */
	volatile uint8_t Reserved4[3];    /*Reserved */
	volatile uint32_t DPLLFREQ;       /*DPLL Frequency Register */
	volatile uint8_t GPIORPSC_C;      /*Auto-Reset at Power Sequence Fail Register */
	volatile uint8_t GPIORPSC_5X;     /*Auto-Reset at Power Sequence Fail Register */
	volatile uint8_t GPIORPSC_6X;     /*Auto-Reset at Power Sequence Fail Register */
	volatile uint8_t Reserved5;       /*Reserved */
	volatile uint32_t GCFGMISC;       /*Misc. Register */
	volatile uint8_t EXTIE;           /*Extended Command Interrupt Enable Register */
	volatile uint8_t Reserved6[3];    /*Reserved */
	volatile uint8_t EXTPF;           /*Extended Command Pending Flag Register */
	volatile uint8_t Reserved7[3];    /*Reserved */
	volatile uint32_t EXTARG;         /*Extended Command Argument0/1/2 Register */
	volatile uint8_t EXTCMD;          /*Extended Command Port Register */
	volatile uint8_t Reserved8[3];    /*Reserved */
	volatile uint32_t ADCOTR;         /*ADCO Register */
	volatile uint32_t Reserved9;      /*Reserved */
	volatile uint32_t IDSR;           /*IDSR Register */
	volatile uint32_t Reserved10[13]; /*Reserved */
	volatile uint32_t TRAPMODE;
	volatile uint32_t CLK1UCFG;
	volatile uint32_t LDO15TRIM;
};

#define GCFG_CLKCFG_48M 0x00000014
#define GCFG_CLKCFG_24M 0x00000004

/*-- Constant Define -------------------------------------------*/
#define LDO15_OFFSET            0x22
#define ECMISC2_OFFSET          0x25
#define CLOCK_SOURCE_OSC32K     1
#define CLOCK_SOURCE_EXTERNAL   0
#define EXTERNAL_CLOCK_GPIO_NUM 0x5D

#define FREQ_96M48M 0
#define FREQ_48M24M 1
#define FREQ_24M12M 2
#define FREQ_12M06M 3

#define DPLL_32K_TARGET_VALUE         0x05B9UL
#define DPLL_SOF_TARGET_VALUE         0xBB80UL
#define DPLL_DONT_CARE_VALUE          3
#define DPLL_DISABLE                  0
#define DPLL_SOURCE_SOF               1
#define DPLL_SOURCE_OSC32K            2
#define DPLL_SOURCE_EXTCLK            3
/* gcfg->FWID status flag */
#define ECFV_NORMAL_RUN               0x00
#define ECFV_PREPARE_UPDATE_ESCD      0x01
#define ECFV_UPDATE_ESCD              0x81
#define ECFV_PREPARE_UPDATE_BIOS      0x02
#define ECFV_UPDATE_BIOS              0x82
#define ECFV_PREPARE_ENTER_DEEP_SLEEP 0x03
#define ECFV_ENTER_DEEP_SLEEP         0x83
#define S0I3_ENTER_STOP               0x04
#define NON_DEEP_SLEEP_STATE          0x05
#define MAIN_LOOP_ENTER_IDLE          0x06

/* VCC Status */
#define VCC_0V   0x00
#define VCC_3P3V 0x01
#define VCC_1P8V 0x02

#endif /* ENE_KB106X_GCFG_H */
