/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file pcr.h
 *MEC1501 Power Control Reset definitions
 */
/** @defgroup MEC1501 Peripherals PCR
 */

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

#ifndef _PCR_H
#define _PCR_H

#define MCHP_PCR_BASE_ADDR	0x40080100ul

#define MCHP_PCR_SYS_SLP_CTRL_OFS	0x00ul
#define MCHP_PCR_SYS_CLK_CTRL_OFS	0x04ul
#define MCHP_PCR_SLOW_CLK_CTRL_OFS	0x08ul
#define MCHP_PCR_OSC_ID_OFS		0x0Cul
#define MCHP_PCR_PRS_OFS		0x10ul
#define MCHP_PCR_PR_CTRL_OFS		0x14ul
#define MCHP_PCR_SYS_RESET_OFS		0x18ul
#define MCHP_PCR_PKE_CLK_CTRL_OFS	0x1Cul
#define MCHP_PCR_SLP_EN0_OFS		0x30ul
#define MCHP_PCR_SLP_EN1_OFS		0x34ul
#define MCHP_PCR_SLP_EN2_OFS		0x38ul
#define MCHP_PCR_SLP_EN3_OFS		0x3Cul
#define MCHP_PCR_SLP_EN4_OFS		0x40ul
#define MCHP_PCR_CLK_REQ0_OFS		0x50ul
#define MCHP_PCR_CLK_REQ1_OFS		0x54ul
#define MCHP_PCR_CLK_REQ2_OFS		0x58ul
#define MCHP_PCR_CLK_REQ3_OFS		0x5Cul
#define MCHP_PCR_CLK_REQ4_OFS		0x60ul
#define MCHP_PCR_PERIPH_RST0_OFS	0x70ul
#define MCHP_PCR_PERIPH_RST1_OFS	0x74ul
#define MCHP_PCR_PERIPH_RST2_OFS	0x78ul
#define MCHP_PCR_PERIPH_RST3_OFS	0x7Cul
#define MCHP_PCR_PERIPH_RST4_OFS	0x80ul
#define MCHP_PCR_PERIPH_RST_LCK_OFS	0x84ul

#define MCHP_PCR_SYS_SLP_CTRL_ADDR	(MCHP_PCR_BASE_ADDR)
#define MCHP_PCR_SYS_CLK_CTRL_ADDR	(MCHP_PCR_BASE_ADDR + 0x04ul)
#define MCHP_PCR_SLOW_CLK_CTRL_ADDR	(MCHP_PCR_BASE_ADDR + 0x08ul)
#define MCHP_PCR_OSC_ID_ADDR		(MCHP_PCR_BASE_ADDR + 0x0Cul)
#define MCHP_PCR_PRS_ADDR		(MCHP_PCR_BASE_ADDR + 0x10ul)
#define MCHP_PCR_PR_CTRL_ADDR		(MCHP_PCR_BASE_ADDR + 0x14ul)
#define MCHP_PCR_SYS_RESET_ADDR		(MCHP_PCR_BASE_ADDR + 0x18ul)
#define MCHP_PCR_PKE_CLK_CTRL_ADDR	(MCHP_PCR_BASE_ADDR + 0x1Cul)
#define MCHP_PCR_SLP_EN0_ADDR		(MCHP_PCR_BASE_ADDR + 0x30ul)
#define MCHP_PCR_SLP_EN1_ADDR		(MCHP_PCR_BASE_ADDR + 0x34ul)
#define MCHP_PCR_SLP_EN2_ADDR		(MCHP_PCR_BASE_ADDR + 0x38ul)
#define MCHP_PCR_SLP_EN3_ADDR		(MCHP_PCR_BASE_ADDR + 0x3Cul)
#define MCHP_PCR_SLP_EN4_ADDR		(MCHP_PCR_BASE_ADDR + 0x40ul)
#define MCHP_PCR_CLK_REQ0_ADDR		(MCHP_PCR_BASE_ADDR + 0x50ul)
#define MCHP_PCR_CLK_REQ1_ADDR		(MCHP_PCR_BASE_ADDR + 0x54ul)
#define MCHP_PCR_CLK_REQ2_ADDR		(MCHP_PCR_BASE_ADDR + 0x58ul)
#define MCHP_PCR_CLK_REQ3_ADDR		(MCHP_PCR_BASE_ADDR + 0x5Cul)
#define MCHP_PCR_CLK_REQ4_ADDR		(MCHP_PCR_BASE_ADDR + 0x60ul)
#define MCHP_PCR_PERIPH_RST0_ADDR	(MCHP_PCR_BASE_ADDR + 0x70ul)
#define MCHP_PCR_PERIPH_RST1_ADDR	(MCHP_PCR_BASE_ADDR + 0x74ul)
#define MCHP_PCR_PERIPH_RST2_ADDR	(MCHP_PCR_BASE_ADDR + 0x78ul)
#define MCHP_PCR_PERIPH_RST3_ADDR	(MCHP_PCR_BASE_ADDR + 0x7Cul)
#define MCHP_PCR_PERIPH_RST4_ADDR	(MCHP_PCR_BASE_ADDR + 0x80ul)
#define MCHP_PCR_PERIPH_RESET_LOCK_ADDR (MCHP_PCR_BASE_ADDR + 0x84ul)

#define MCHP_PCR_SLP_EN_ADDR(n) \
	(MCHP_PCR_BASE_ADDR + 0x30ul + ((uint32_t)(n) << 2))
#define MCHP_PCR_CLK_REQ_ADDR(n) \
	(MCHP_PCR_BASE_ADDR + 0x50ul + ((uint32_t)(n) << 2))
#define MCHP_PCR_PERIPH_RESET_ADDR(n) \
	(MCHP_PCR_BASE_ADDR + 0x70ul + ((uint32_t)(n) << 2))

#define MCHP_PCR_SLEEP_EN	(1u)
#define MCHP_PCR_SLEEP_DIS	(0u)

/*
 * MEC1501 PCR implements multiple SLP_EN, CLR_REQ, and RST_EN registers.
 * CLK_REQ bits are read-only. The peripheral sets its CLK_REQ if it requires
 * clocks. CLK_REQ bits must all be zero for the PCR block to put the MEC17xx
 * into light or heavy sleep.
 * SLP_EN bit = 1 instructs HW to gate off clock tree to peripheral only if
 * peripherals PCR CLK_REQ bit is 0.
 * RST_EN bit = 1 will reset the peripheral at any time. The RST_EN registers
 * must be unlocked by writing the unlock code to PCR Peripheral Reset Lock
 * register.
 * SLP_EN usage is:
 * Initialization set all PCR SLP_EN bits = 0 except for crypto blocks as
 * these IP do not implement internal clock gating.
 * When firmware wants to enter light or heavy sleep.
 * Configure wake up source(s)
 * Write MCHP_PCR_SYS_SLP_CTR register to value based on light/heavy with
 *	SLEEP_ALL bit = 1.
 * Execute Cortex-M4 WFI sequence. DSB(), ISB(), WFI(), NOP()
 * Cortex-M4 will assert sleep signal to PCR block.
 * PCR HW will spin until all CLK_REQ==0
 * PCR will then turn off clocks based on light/heavy sleep.
 *
 * RST_EN usage is:
 * Save and disable maskable interrupts
 * Write unlock code to PCR Peripheral Reset Lock
 * Write bit patterns to one or more of PCR RST_EN[0, 4] registers
 *  Selected peripherals will be reset.
 * Write lock code to PCR Peripheral Reset Lock.
 * Restore interrupts.
 */
#define MCHP_MAX_PCR_SCR_REGS	5ul

/*
 * VTR Powered PCR registers
 */

#define MCHP_PCR_SLP(bitpos)	(1ul << (bitpos))

/*
 * PCR System Sleep Control
 */

#define MCHP_PCR_SYS_SLP_CTRL_MASK		0x0109ul
#define MCHP_PCR_SYS_SLP_CTRL_SLP_LIGHT		(0ul << 0)
#define MCHP_PCR_SYS_SLP_CTRL_SLP_HEAVY		(1ul << 0)
#define MCHP_PCR_SYS_SLP_CTRL_SLP_ALL		(1ul << 3)
/*
 * bit[8] can be used to prevent entry to heavy sleep unless the
 * PLL is locked.
 * bit[8]==0 (POR default) system will allow entry to light or heavy
 * sleep if and only if PLL is locked.
 * bit[8]==1 system will allow entry to heavy sleep before PLL is locked.
 */
#define MCHP_PCR_SYS_SLP_CTRL_SLP_PLL_LOCK		(0ul << 8)
#define MCHP_PCR_SYS_SLP_CTRL_ALLOW_SLP_NO_PLL_LOCK	(0ul << 8)

#define MCHP_PCR_SYS_SLP_LIGHT	0x08ul
#define MCHP_PCR_SYS_SLP_HEAVY	0x09ul

/*
 * PCR Process Clock Control
 * Divides 48MHz clock to ARM Cortex-M4 core including
 * SysTick and NVIC.
 */
#define MCHP_PCR_PROC_CLK_CTRL_MASK	0xFFul
#define MCHP_PCR_PROC_CLK_CTRL_48MHZ	0x01ul
#define MCHP_PCR_PROC_CLK_CTRL_16MHZ	0x03ul
#define MCHP_PCR_PROC_CLK_CTRL_12MHZ	0x04ul
#define MCHP_PCR_PROC_CLK_CTRL_4MHZ	0x10ul
#define MCHP_PCR_PROC_CLK_CTRL_1MHZ	0x30ul

/*
 * PCR Slow Clock Control
 * Clock divicder for 100KHz clock domain
 */
#define MCHP_PCR_SLOW_CLK_CTRL_MASK	0x3FFul
#define MCHP_PCR_SLOW_CLK_CTRL_100KHZ	0x1E0ul

/*
 * PCR Oscillator ID register (Read-Only)
 */
#define MCHP_PCR_OSC_ID_MASK		0x1FFul
#define MCHP_PCR_OSC_ID_PLL_LOCK	(1ul << 8)

/*
 * PCR Power Reset Status Register
 */
#define MCHP_PCR_PRS_MASK			0xCECul
#define MCHP_PCR_PRS_VCC_PWRGD_STATE_RO		(1ul << 2)
#define MCHP_PCR_PRS_HOST_RESET_STATE_RO	(1ul << 3)
#define MCHP_PCR_PRS_VBAT_RST_RWC		(1ul << 5)
#define MCHP_PCR_PRS_VTR_RST_RWC		(1ul << 6)
#define MCHP_PCR_PRS_JTAG_RST_RWC		(1ul << 7)
#define MCHP_PCR_PRS_32K_ACTIVE_RO		(1ul << 10)
#define MCHP_PCR_PRS_LPC_ESPI_CLK_ACTIVE_RO	(1ul << 11)

/*
 * PCR Power Reset Control Register
 */
#define MCHP_PCR_PR_CTRL_MASK			0x101ul
#define MCHP_PCR_PR_CTRL_PWR_INV		(1ul << 0)
#define MCHP_PCR_PR_CTRL_USE_ESPI_PLTRST	(0ul << 8)
#define MCHP_PCR_PR_CTRL_USE_PCI_RST		(1ul << 8)

/*
 * PCR System Reset Register
 */
#define MCHP_PCR_SYS_RESET_MASK		0x100ul
#define MCHP_PCR_SYS_RESET_NOW		(1ul << 8)

/*
 * PCR PKE Clock Register
 */
#define MCHP_PCR_PKE_CLK_CTRL_MASK	0x03ul
#define MCHP_PCR_PKE_CLK_CTRL_96M	0x00ul
#define MCHP_PCR_PKE_CLK_CTRL_48M	0x03ul

/*
 * Sleep Enable Reg 0	    (Offset +30h)
 * Clock Required Reg 0	    (Offset +50h)
 * Reset Enable Reg 3	    (Offset +70h)
 */
#define MCHP_PCR0_JTAG_STAP_POS		0u

/*
 * Sleep Enable Reg 1	    (Offset +34h)
 * Clock Required Reg 1	    (Offset +54h)
 * Reset Enable Reg 1	    (Offset +74h)
 */
#define MCHP_PCR1_ECIA_POS	0u
#define MCHP_PCR1_PECI_POS	1u
#define MCHP_PCR1_TACH0_POS	2u
#define MCHP_PCR1_PWM0_POS	4u
#define MCHP_PCR1_PMC_POS	5u
#define MCHP_PCR1_DMA_POS	6u
#define MCHP_PCR1_TFDP_POS	7u
#define MCHP_PCR1_CPU_POS	8u
#define MCHP_PCR1_WDT_POS	9u
#define MCHP_PCR1_SMB0_POS	10u
#define MCHP_PCR1_TACH1_POS	11u
#define MCHP_PCR1_TACH2_POS	12u
#define MCHP_PCR1_TACH3_POS	13u
#define MCHP_PCR1_PWM1_POS	20u
#define MCHP_PCR1_PWM2_POS	21u
#define MCHP_PCR1_PWM3_POS	22u
#define MCHP_PCR1_PWM4_POS	23u
#define MCHP_PCR1_PWM5_POS	24u
#define MCHP_PCR1_PWM6_POS	25u
#define MCHP_PCR1_PWM7_POS	26u
#define MCHP_PCR1_PWM8_POS	27u
#define MCHP_PCR1_ECS_POS	29u
#define MCHP_PCR1_B16TMR0_POS	30u
#define MCHP_PCR1_B16TMR1_POS	31u

/*
 * Sleep Enable Reg 2	    (Offset +38h)
 * Clock Required Reg 2	    (Offset +58h)
 * Reset Enable Reg 2	    (Offset +78h)
 */
#define MCHP_PCR2_EMI0_POS	0u
#define MCHP_PCR2_UART0_POS	1u
#define MCHP_PCR2_UART1_POS	2u
#define MCHP_PCR2_GCFG_POS	12u
#define MCHP_PCR2_ACPI_EC0_POS	13u
#define MCHP_PCR2_ACPI_EC1_POS	14u
#define MCHP_PCR2_ACPI_PM1_POS	15u
#define MCHP_PCR2_KBC_POS	16u
#define MCHP_PCR2_MBOX_POS	17u
#define MCHP_PCR2_RTC_POS	18u
#define MCHP_PCR2_ESPI_POS	19u
#define MCHP_PCR2_SCR32_POS	20u
#define MCHP_PCR2_ACPI_EC2_POS	21u
#define MCHP_PCR2_ACPI_EC3_POS	22u
#define MCHP_PCR2_PORT80CAP0_POS	25u
#define MCHP_PCR2_PORT80CAP1_POS	26u
#define MCHP_PCR2_ESPI_SAF_POS	27u
#define MCHP_PCR2_UART2_POS	28u

/*
 * Sleep Enable Reg 3	    (Offset +3Ch)
 * Clock Required Reg 3	    (Offset +5Ch)
 * Reset Enable Reg 3	    (Offset +7Ch)
 */
#define MCHP_PCR3_HDMI_CEC_POS	1u
#define MCHP_PCR3_ADC_POS	3u
#define MCHP_PCR3_PS2_0_POS	5u
#define MCHP_PCR3_PS2_1_POS	6u
#define MCHP_PCR3_HTMR0_POS	10u
#define MCHP_PCR3_KEYSCAN_POS	11u
#define MCHP_PCR3_SMB1_POS	13u
#define MCHP_PCR3_SMB2_POS	14u
#define MCHP_PCR3_SMB3_POS	15u
#define MCHP_PCR3_LED0_POS	16u
#define MCHP_PCR3_LED1_POS	17u
#define MCHP_PCR3_LED2_POS	18u
#define MCHP_PCR3_SMB4_POS	20u
#define MCHP_PCR3_B32TMR0_POS	23u
#define MCHP_PCR3_B32TMR1_POS	24u
#define MCHP_PCR3_PKE_POS	26u
#define MCHP_PCR3_RNG_POS	27u
#define MCHP_PCR3_AESH_POS	28u
#define MCHP_PCR3_HTMR1_POS	29u
#define MCHP_PCR3_CCT_POS	30u

#define MCHP_PCR3_CRYPTO_MASK \
	((1ul << (MCHP_PCR3_PKE_POS)) +\
	(1ul << (MCHP_PCR3_RNG_POS)) + (1ul << (MCHP_PCR3_AESH_POS)))

/*
 * Sleep Enable Reg 4	    (Offset +40h)
 * Clock Required Reg 4	    (Offset +60h)
 * Reset Enable Reg 4	    (Offset +80h)
 */
#define MCHP_PCR4_RTMR_POS	6u
#define MCHP_PCR4_QMSPI_POS	8u
#define MCHP_PCR4_I2C0_POS	10u
#define MCHP_PCR4_I2C1_POS	11u
#define MCHP_PCR4_I2C3_POS	12u
#define MCHP_PCR4_SPISLV_POS	16u

/* Reset Enable Lock	    (Offset +84h) */
#define MCHP_PCR_RSTEN_UNLOCK	0xA6382D4Cul
#define MCHP_PCR_RSTEN_LOCK	0xA6382D4Dul

/*
 * PCR register access
 */
#define MCHP_PCR_SLP_CTRL()	REG32(MCHP_PCR_SYS_SLP_CTRL_ADDR)
#define MCHP_PCR_PROC_CLK_DIV()	REG8(MCHP_PCR_SYS_CLK_CTRL_ADDR)
#define MCHP_PCR_SLOW_CLK_CTRL()	REG32(MCHP_PCR_SLOW_CLK_CTRL_ADDR)
#define MCHP_PCR_OSC_ID()	REG32(MCHP_PCR_OSC_ID_ADDR)
#define MCHP_PCR_PRS()		REG32(MCHP_PCR_PRS_ADDR)
#define MCHP_PCR_PR_CTRL()	REG32(MCHP_PCR_PR_CTRL_ADDR)
#define MCHP_PCR_SYS_RESET()	REG32(MCHP_PCR_SYS_RESET_ADDR)
#define MCHP_PCR_PERIPH_RST_LOCK() \
	REG32(MCHP_PCR_PERIPH_RESET_LOCK_ADDR)
#define MCHP_PCR_SLP_EN(n)	REG32(MCHP_PCR_SLP_EN_ADDR(n))
#define MCHP_PCR_CLK_REQ_RO(n)	REG32(MCHP_PCR_CLK_REQ_ADDR(n))
#define MCHP_PCR_SLP_EN0()	REG32(MCHP_PCR_SLP_EN_ADDR(0))
#define MCHP_PCR_SLP_EN1()	REG32(MCHP_PCR_SLP_EN_ADDR(1))
#define MCHP_PCR_SLP_EN2()	REG32(MCHP_PCR_SLP_EN_ADDR(2))
#define MCHP_PCR_SLP_EN3()	REG32(MCHP_PCR_SLP_EN_ADDR(3))
#define MCHP_PCR_SLP_EN4()	REG32(MCHP_PCR_SLP_EN_ADDR(4))
#define MCHP_PCR_CLK_REQ0_RO()	REG32(MCHP_PCR_CLK_REQ_ADDR(0))
#define MCHP_PCR_CLK_REQ1_RO()	REG32(MCHP_PCR_CLK_REQ_ADDR(1))
#define MCHP_PCR_CLK_REQ2_RO()	REG32(MCHP_PCR_CLK_REQ_ADDR(2))
#define MCHP_PCR_CLK_REQ3_RO()	REG32(MCHP_PCR_CLK_REQ_ADDR(3))
#define MCHP_PCR_CLK_REQ4_RO()	REG32(MCHP_PCR_CLK_REQ_ADDR(4))
#define MCHP_PCR_PERIPH_RST(n)	REG32(MCHP_PCR_PERIPH_RESET_ADDR(n))

#define MCHP_PCR_DEV_SLP_EN_CLR(n, b) \
	REG32(MCHP_PCR_SLP_EN_ADDR(n)) &= ~(1ul << (uint32_t)(b))

#define MCHP_PCR_DEV_SLP_EN_SET(n, b) \
	 REG32(MCHP_PCR_SLP_EN_ADDR(n)) |= (1ul << (uint32_t)(b))

/*
 * PCR SleepEn/CLK_REQ/ResetOnSleep register offset from
 * SlpEn0/CLK_REQ0/ResetOnSleep0 registers in b[7:5]
 * Bit position in register = b[4:0]
 */
typedef enum pcr_id {
	PCR_STAP = 0u,
	PCR_OTP = 1u,
	PCR_ECIA = (128u + 0u),	/* 4 << 5 = 128 */
	PCR_PECI = (128u + 1u),
	PCR_TACH0 = (128u + 2u),
	PCR_PWM0 = (128u + 4u),
	PCR_PMC = (128u + 5u),
	PCR_DMA = (128u + 6u),
	PCR_TFDP = (128u + 7u),
	PCR_CPU = (128u + 8u),
	PCR_WDT = (128u + 9u),
	PCR_SMB0 = (128u + 10u),
	PCR_TACH1 = (128u + 11u),
	PCR_TACH2 = (128u + 12u),
	PCR_TACH3 = (128u + 13u),
	PCR_PWM1 = (128u + 20u),
	PCR_PWM2 = (128u + 21u),
	PCR_PWM3 = (128u + 22u),
	PCR_PWM4 = (128u + 23u),
	PCR_PWM5 = (128u + 24u),
	PCR_PWM6 = (128u + 25u),
	PCR_PWM7 = (128u + 26u),
	PCR_PWM8 = (128u + 27u),
	PCR_ECS = (128u + 29u),
	PCR_B16TMR0 = (128u + 30u),
	PCR_B16TMR1 = (128u + 31u),
	PCR_EMI0 = (256u + 0u),	/* 8 << 5 = 256 */
	PCR_UART0 = (256u + 1u),
	PCR_UART1 = (256u + 2u),
	PCR_GCFG = (256u + 12u),
	PCR_ACPI_EC0 = (256u + 13u),
	PCR_ACPI_EC1 = (256u + 14u),
	PCR_ACPI_PM1 = (256u + 15u),
	PCR_KBC = (256u + 16u),
	PCR_MBOX = (256u + 17u),
	PCR_RTC = (256u + 18u),
	PCR_ESPI = (256u + 19u),
	PCR_SCR32 = (256u + 20u),
	PCR_ACPI_EC2 = (256u + 21u),
	PCR_ACPI_EC3 = (256u + 22u),
	PCR_P80CAP0 = (256u + 25u),
	PCR_P80CAP1 = (256u + 26u),
	PCR_ESPI_SAF = (256u + 27u),
	PCR_UART2 = (256u + 28u),
	PCR_HDMI_CEC = (384u + 1u),	/* 12 << 5 = 384 */
	PCR_ADC = (384u + 3u),
	PCR_PS2_0 = (384u + 5u),
	PCR_PS2_1 = (384u + 6u),
	PCR_HTMR0 = (384u + 10u),
	PCR_KEYSCAN = (384u + 11u),
	PCR_SMB1 = (384u + 13u),
	PCR_SMB2 = (384u + 14u),
	PCR_SMB3 = (384u + 15u),
	PCR_LED0 = (384u + 16u),
	PCR_LED1 = (384u + 17u),
	PCR_LED2 = (384u + 18u),
	PCR_SMB4 = (384u + 20u),
	PCR_B32TMR0 = (384u + 23u),
	PCR_B32TMR1 = (384u + 24u),
	PCR_PKE = (384u + 26u),
	PCR_NDRNG = (384u + 27u),
	PCR_AESH = (384u + 28u),
	PCR_HTMR1 = (384u + 29u),
	PCR_CCT = (384u + 30u),
	PCR_RTMR = (512u + 6u),	/* 16 << 5 = 512 */
	PCR_QMSPI = (512u + 8u),
	PCR_I2C0 = (512u + 10u),
	PCR_I2C1 = (512u + 11u),
	PCR_I2C2 = (512u + 12u),
	PCR_SPISLV = (512u + 16u),
	PCR_MAX_ID,
} PCR_ID;

/* =====================================================================*/
/* ================	     PCR		       ================ */
/* =====================================================================*/

/**
  * @brief Power Control Reset (PCR)
  */

typedef struct pcr_regs
{		/*!< (@ 0x40080100) PCR Structure   */
	__IOM uint32_t SYS_SLP_CTRL;	/*!< (@ 0x0000) System Sleep Control */
	__IOM uint32_t PROC_CLK_CTRL;	/*!< (@ 0x0004) Processor Clock Control */
	__IOM uint32_t SLOW_CLK_CTRL;	/*!< (@ 0x0008) Slow Clock Control */
	__IOM uint32_t OSC_ID;	/*!< (@ 0x000C) Processor Clock Control */
	__IOM uint32_t PWR_RST_STS;	/*!< (@ 0x0010) Power Reset Status */
	__IOM uint32_t PWR_RST_CTRL;	/*!< (@ 0x0014) Power Reset Control */
	__IOM uint32_t SYS_RST;	/*!< (@ 0x0018) System Reset */
	__IOM uint32_t TEST1C;
	__IOM uint32_t TEST20;
	uint8_t RSVD1[12];
	__IOM uint32_t SLP_EN0;	/*!< (@ 0x0030) Sleep Enable 0 */
	__IOM uint32_t SLP_EN1;	/*!< (@ 0x0034) Sleep Enable 1 */
	__IOM uint32_t SLP_EN2;	/*!< (@ 0x0038) Sleep Enable 2 */
	__IOM uint32_t SLP_EN3;	/*!< (@ 0x003C) Sleep Enable 3 */
	__IOM uint32_t SLP_EN4;	/*!< (@ 0x0040) Sleep Enable 4 */
	uint8_t RSVD2[12];
	__IOM uint32_t CLK_REQ0;	/*!< (@ 0x0050) Clock Required 0 (RO) */
	__IOM uint32_t CLK_REQ1;	/*!< (@ 0x0054) Clock Required 1 (RO) */
	__IOM uint32_t CLK_REQ2;	/*!< (@ 0x0058) Clock Required 2 (RO) */
	__IOM uint32_t CLK_REQ3;	/*!< (@ 0x005C) Clock Required 3 (RO) */
	__IOM uint32_t CLK_REQ4;	/*!< (@ 0x0060) Clock Required 4 (RO) */
	uint8_t RSVD3[12];
	__IOM uint32_t RST_EN0;	/*!< (@ 0x0070) Peripheral Reset 0 */
	__IOM uint32_t RST_EN1;	/*!< (@ 0x0074) Peripheral Reset 1 */
	__IOM uint32_t RST_EN2;	/*!< (@ 0x0078) Peripheral Reset 2 */
	__IOM uint32_t RST_EN3;	/*!< (@ 0x007C) Peripheral Reset 3 */
	__IOM uint32_t RST_EN4;	/*!< (@ 0x0080) Peripheral Reset 4 */
	__IOM uint32_t RST_EN_LOCK;	/*!< (@ 0x0084) Peripheral Lock */
} PCR_Type;

static __attribute__ ((always_inline)) inline void
mchp_pcr_periph_slp_ctrl(PCR_ID pcr_id, uint8_t enable)
{
	uintptr_t raddr = (uintptr_t) (MCHP_PCR_SLP_EN0_ADDR);
	uint32_t bitpos = (uint32_t) pcr_id & 0x1F;

	raddr += ((uint32_t) pcr_id >> 5);
	if (enable) {
		REG32(raddr) |= (1ul << bitpos);
	} else {
		REG32(raddr) &= ~(1ul << bitpos);
	}
}

#endif				// #ifndef _DEFS_H
/* end pcr.h */
/**   @}
 */
