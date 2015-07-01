/* system.c - system/hardware module for fsl_frdm_k64f BSP */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module provides routines to initialize and support board-level hardware
for the fsl_frdm_k64f BSP.
 */

#include <nanokernel.h>
#include <board.h>
#include <drivers/k20_mcg.h>
#include <drivers/uart.h>
#include <drivers/k20_pcr.h>
#include <drivers/k20_sim.h>
#include <drivers/k6x_mpu.h>
#include <drivers/k6x_pmc.h>
#include <sections.h>

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
#define DO_CONSOLE_INIT
#endif

/* board's setting for PLL multipler (PRDIV0) */
#define FRDM_K64F_PLL_DIV_20 (20 - 1)

/* board's setting for PLL multipler (VDIV0) */
#define FRDM_K64F_PLL_MULT_48 (48 - 24)

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif

/*
 * K64F Flash configuration fields
 * These 16 bytes, which must be loaded to address 0x400, include default
 * protection and security settings.
 * They are loaded at reset to various Flash Memory module (FTFE) registers.
 *
 * The structure is:
 * -Backdoor Comparison Key for unsecuring the MCU - 8 bytes
 * -Program flash protection bytes, 4 bytes, written to FPROT0-3
 * -Flash security byte, 1 byte, written to FSEC
 * -Flash nonvolatile option byte, 1 byte, written to FOPT
 * -Reserved, 1 byte, (Data flash protection byte for FlexNVM)
 * -Reserved, 1 byte, (EEPROM protection byte for FlexNVM)
 *
 */
uint8_t __security_frdm_k64f_section __security_frdm_k64f[] = {
	/* Backdoor Comparison Key (unused) */
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	/* Program flash protection; 1 bit/region - 0=protected, 1=unprotected
	   */
	0xFF, 0xFF, 0xFF, 0xFF,
	/*
	 * Flash security: Backdoor key disabled, Mass erase enabled,
	 *                 Factory access enabled, MCU is unsecure
	 */
	0xFE,
	/* Flash nonvolatile option: NMI enabled, EzPort enabled, Normal boot */
	0xFF,
	/* Reserved for FlexNVM feature (unsupported by this MCU) */
	0xFF, 0xFF};

/**
 *
 * clkInit - initialize the system clock
 *
 * This routine will configure the multipurpose clock generator (MCG) to
 * set up the system clock.
 * The MCG has nine possible modes, including Stop mode.  This routine assumes
 * that the current MCG mode is FLL Engaged Internal (FEI), as from reset.
 * It transitions through the FLL Bypassed External (FBE) and
 * PLL Bypassed External (PBE) modes to get to the desired
 * PLL Engaged External (PEE) mode and generate the maximum 120 MHz system clock.
 *
 * RETURNS: N/A
 *
 */

static void clkInit(void)
{
	uint8_t temp_reg;
	K20_MCG_t *mcg_p = (K20_MCG_t *)PERIPH_ADDR_BASE_MCG; /* clk gen. ctl */

	/*
	 * Select the 50 Mhz external clock as the MCG OSC clock.
	 * MCG Control 7 register:
	 * - Select OSCCLK0 / XTAL
	 */

	temp_reg = mcg_p->c7 & ~MCG_C7_OSCSEL_MASK;
	temp_reg |= MCG_C7_OSCSEL_OSC0;
	mcg_p->c7 = temp_reg;

	/*
	 * Transition MCG from FEI mode (at reset) to FBE mode.
	 */

	/*
	 * MCG Control 2 register:
	 * - Set oscillator frequency range = very high for 50 MHz external
	 * clock
	 * - Set oscillator mode = low power
	 * - Select external reference clock as the oscillator source
	 */

	temp_reg = mcg_p->c2 &
		   ~(MCG_C2_RANGE_MASK | MCG_C2_HGO_MASK | MCG_C2_EREFS_MASK);

	temp_reg |=
		(MCG_C2_RANGE_VHIGH | MCG_C2_HGO_LO_PWR | MCG_C2_EREFS_EXT_CLK);

	mcg_p->c2 = temp_reg;

	/*
	 * MCG Control 1 register:
	 * - Set system clock source (MCGOUTCLK) = external reference clock
	 * - Set FLL external reference divider = 1024 (MCG_C1_FRDIV_32_1024)
	 *   to get the FLL frequency of 50 MHz/1024 = 48.828KHz
	 *   (Note: If FLL frequency must be in the in 31.25KHz-39.0625KHz
	 *range,
	 *          the FLL external reference divider = 1280
	 *(MCG_C1_FRDIV_64_1280)
	 *          to get 50 MHz/1280 = 39.0625KHz)
	 * - Select the external reference clock as the FLL reference source
	 *
	 */

	temp_reg = mcg_p->c1 &
		   ~(MCG_C1_CLKS_MASK | MCG_C1_FRDIV_MASK | MCG_C1_IREFS_MASK);

	temp_reg |=
		(MCG_C1_CLKS_EXT_REF | MCG_C1_FRDIV_32_1024 | MCG_C1_IREFS_EXT);

	mcg_p->c1 = temp_reg;

	/* Confirm that the external reference clock is the FLL reference source
	 */

	while ((mcg_p->s & MCG_S_IREFST_MASK) != 0)
		;
	;

	/* Confirm the external ref. clock is the system clock source
	 * (MCGOUTCLK) */

	while ((mcg_p->s & MCG_S_CLKST_MASK) != MCG_S_CLKST_EXT_REF)
		;
	;

	/*
	 * Transition to PBE mode.
	 * Configure the PLL frequency in preparation for PEE mode.
	 * The goal is PEE mode with a 120 MHz system clock source (MCGOUTCLK),
	 * which is calculated as (oscillator clock / PLL divider) * PLL
	 * multiplier,
	 * where oscillator clock = 50MHz, PLL divider = 20 and PLL multiplier =
	 * 48.
	 */

	/*
	 * MCG Control 5 register:
	 * - Set the PLL divider
	 */

	temp_reg = mcg_p->c5 & ~MCG_C5_PRDIV0_MASK;

	temp_reg |= FRDM_K64F_PLL_DIV_20;

	mcg_p->c5 = temp_reg;

	/*
	 * MCG Control 6 register:
	 * - Select PLL as output for PEE mode
	 * - Set the PLL multiplier
	 */

	temp_reg = mcg_p->c6 & ~(MCG_C6_PLLS_MASK | MCG_C6_VDIV0_MASK);

	temp_reg |= (MCG_C6_PLLS_PLL | FRDM_K64F_PLL_MULT_48);

	mcg_p->c6 = temp_reg;

	/* Confirm that the PLL clock is selected as the PLL output */

	while ((mcg_p->s & MCG_S_PLLST_MASK) == 0)
		;
	;

	/* Confirm that the PLL has acquired lock */

	while ((mcg_p->s & MCG_S_LOCK0_MASK) == 0)
		;
	;

	/*
	 * Transition to PEE mode.
	 * MCG Control 1 register:
	 * - Select PLL as the system clock source (MCGOUTCLK)
	 */

	temp_reg = mcg_p->c1 & ~MCG_C1_CLKS_MASK;

	temp_reg |= MCG_C1_CLKS_FLL_PLL;

	mcg_p->c1 = temp_reg;

	/* Confirm that the PLL output is the system clock source (MCGOUTCLK) */

	while ((mcg_p->s & MCG_S_CLKST_MASK) != MCG_S_CLKST_PLL)
		;
	;
}

#if defined(DO_CONSOLE_INIT)

/**
 *
 * consoleInit - initialize target-only console
 *
 * Only used for debugging.
 *
 * RETURNS: N/A
 *
 */

#include <console/uart_console.h>

static void consoleInit(void)
{
	uint32_t port;
	uint32_t rxPin;
	uint32_t txPin;
	K20_PCR_t pcr = {0}; /* Pin Control Register */

	/* Port/pin ctrl module */
	K20_PORT_PCR_t *port_pcr_p = (K20_PORT_PCR_t *)PERIPH_ADDR_BASE_PCR;

	struct uart_init_info info = {
		.baud_rate = CONFIG_UART_CONSOLE_BAUDRATE,
		.sys_clk_freq = CONFIG_UART_CONSOLE_CLK_FREQ,
		/* Only supported in polling mode, but init all info fields */
		.int_pri = CONFIG_UART_CONSOLE_INT_PRI
	};

	/* UART0 Rx and Tx pin assignments */
	port = CONFIG_UART_CONSOLE_PORT;
	rxPin = CONFIG_UART_CONSOLE_PORT_RX_PIN;
	txPin = CONFIG_UART_CONSOLE_PORT_TX_PIN;

	/* Enable the UART Rx and Tx Pins */
	pcr.field.mux = CONFIG_UART_CONSOLE_PORT_MUX_FUNC;

	port_pcr_p->port[port].pcr[rxPin] = pcr;
	port_pcr_p->port[port].pcr[txPin] = pcr;

	uart_init(CONFIG_UART_CONSOLE_INDEX, &info);

	uart_console_init();
}

#else
#define consoleInit()     \
	do {/* nothing */ \
	} while ((0))
#endif /* DO_CONSOLE_INIT */

/**
 *
 * _InitHardware - perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers and the
 * Kinetis UART device driver.
 * Also initialize the timer device driver, if required.
 *
 * RETURNS: N/A
 */

void _InitHardware(void)
{
	/* System Integration module */
	K20_SIM_t *sim_p = (K20_SIM_t *)PERIPH_ADDR_BASE_SIM;

	/* Power Mgt Control module */
	K6x_PMC_t *pmc_p = (K6x_PMC_t *)PERIPH_ADDR_BASE_PMC;

	/* Power Mgt Control module */
	K6x_MPU_t *mpu_p = (K6x_MPU_t *)PERIPH_ADDR_BASE_MPU;

	int oldLevel; /* old interrupt lock level */
	uint32_t temp_reg;

	/* disable interrupts */
	oldLevel = irq_lock();

	/* enable the port clocks */
	sim_p->scgc5.value |= (SIM_SCGC5_PORTA_CLK_EN | SIM_SCGC5_PORTB_CLK_EN |
			       SIM_SCGC5_PORTC_CLK_EN | SIM_SCGC5_PORTD_CLK_EN |
			       SIM_SCGC5_PORTE_CLK_EN);

	/* release I/O power hold to allow normal run state */
	pmc_p->regsc.value |= PMC_REGSC_ACKISO_MASK;

	/*
	 * Disable memory protection and clear slave port errors.
	 * Note that the K64F does not implement the optional ARMv7-M memory
	 * protection unit (MPU), specified by the architecture (PMSAv7), in the
	 * Cortex-M4 core.  Instead, the processor includes its own MPU module.
	 */
	temp_reg = mpu_p->ctrlErrStatus.value;
	temp_reg &= ~MPU_VALID_MASK;
	temp_reg |= MPU_SLV_PORT_ERR_MASK;
	mpu_p->ctrlErrStatus.value = temp_reg;

	/* clear all faults */

	_ScbMemFaultAllFaultsReset();
	_ScbBusFaultAllFaultsReset();
	_ScbUsageFaultAllFaultsReset();

	_ScbHardFaultAllFaultsReset();

	/*
	 * Initialize the clock dividers for:
	 * core and system clocks = 120 MHz (PLL/OUTDIV1)
	 * bus clock = 60 MHz (PLL/OUTDIV2)
	 * FlexBus clock = 40 MHz (PLL/OUTDIV3)
	 * Flash clock = 24 MHz (PLL/OUTDIV4)
	 */
	sim_p->clkdiv1.value = ((SIM_CLKDIV(1) << SIM_CLKDIV1_OUTDIV1_SHIFT) |
				(SIM_CLKDIV(2) << SIM_CLKDIV1_OUTDIV2_SHIFT) |
				(SIM_CLKDIV(3) << SIM_CLKDIV1_OUTDIV3_SHIFT) |
				(SIM_CLKDIV(5) << SIM_CLKDIV1_OUTDIV4_SHIFT));

	clkInit(); /* Initialize PLL/system clock to 120 MHz */

	consoleInit(); /* NOP if not needed */

	NMI_INIT(); /* install default handler that simply resets the CPU
		     * if configured in the kernel, NOP otherwise */

	/* restore interrupt state */
	irq_unlock(oldLevel);
}
