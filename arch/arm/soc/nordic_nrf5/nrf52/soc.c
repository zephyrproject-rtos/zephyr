/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF52 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF52 family processor.
 */

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif

#include "nrf.h"

#define __SYSTEM_CLOCK_64M (64000000UL)

static bool ftpan_32(void);
static bool ftpan_37(void);
static bool ftpan_36(void);

uint32_t SystemCoreClock __used = __SYSTEM_CLOCK_64M;

static void clock_init(void)
{
	SystemCoreClock = __SYSTEM_CLOCK_64M;
}

static int nordicsemi_nrf52_init(struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	/* Note:
	* Magic numbers below are obtained by reading the registers
	* when the SoC was running the SAM-BA bootloader
	* (with reserved bits set to 0).
	*/

	key = irq_lock();

	/* Setup the vector table offset register (VTOR),
	* which is located at the beginning of flash area.
	*/
	_scs_relocate_vector_table((void *)CONFIG_FLASH_BASE_ADDRESS);

	/* Workaround for FTPAN-32 "DIF: Debug session automatically
	* enables TracePort pins" found at Product Anomaly document
	* for your device located at https://www.nordicsemi.com/
	*/
	if (ftpan_32()) {
		CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
	}

	/* Workaround for FTPAN-37 "AMLI: EasyDMA is slow with Radio,
	* ECB, AAR and CCM." found at Product Anomaly document
	* for your device located at https://www.nordicsemi.com/
	*/
	if (ftpan_37()) {
		*(volatile uint32_t *)0x400005A0 = 0x3;
	}

	/* Workaround for FTPAN-36 "CLOCK: Some registers are not
	* reset when expected." found at Product Anomaly document
	* for your device located at https://www.nordicsemi.com/
	*/
	if (ftpan_36()) {
		NRF_CLOCK->EVENTS_DONE = 0;
		NRF_CLOCK->EVENTS_CTTO = 0;
	}

	/* Enable the FPU if the compiler used floating point unit
	* instructions. __FPU_USED is a MACRO defined by the
	* compiler. Since the FPU consumes energy, remember to
	* disable FPU use in the compiler if floating point unit
	* operations are not used in your code.
	*/
	#if (__FPU_USED == 1)

	SCB->CPACR |= (3UL << 20) | (3UL << 22);
	__DSB();
	__ISB();

	#endif

	/* Configure NFCT pins as GPIOs if NFCT is not to be used in
	* your code. If CONFIG_NFCT_PINS_AS_GPIOS is not defined,
	* two GPIOs (see Product Specification to see which ones)
	* will be reserved for NFC and will not be available as
	* normal GPIOs.
	*/
	#if defined(CONFIG_NFCT_PINS_AS_GPIOS)

	if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) ==
	    (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)) {

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}
		NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}
		NVIC_SystemReset();
	}

	#endif

	/* Configure GPIO pads as pPin Reset pin if Pin Reset
	* capabilities desired. If CONFIG_GPIO_AS_PINRESET is not
	* defined, pin reset will not be available. One GPIO (see
	* Product Specification to see which one) will then be
	* reserved for PinReset and not available as normal GPIO.
	*/
	#if defined(CONFIG_GPIO_AS_PINRESET)
	if (((NRF_UICR->PSELRESET[0] & UICR_PSELRESET_CONNECT_Msk) !=
	     (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos)) ||
	    ((NRF_UICR->PSELRESET[0] & UICR_PSELRESET_CONNECT_Msk) !=
	     (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos))) {

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}
		NRF_UICR->PSELRESET[0] = 21;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}
		NRF_UICR->PSELRESET[1] = 21;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}
		NVIC_SystemReset();
	}
	#endif

	/* Enable SWO trace functionality. If ENABLE_SWO is not
	* defined, SWO pin will be used as GPIO (see Product
	* Specification to see which one).
	*/
	#if defined(ENABLE_SWO)

	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Serial <<
				  CLOCK_TRACECONFIG_TRACEMUX_Pos;
	#endif

	/* Enable Trace functionality. If ENABLE_TRACE is not
	* defined, TRACE pins will be used as GPIOs (see Product
	* Specification to see which ones).
	*/
	#if defined(ENABLE_TRACE)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Parallel <<
				  CLOCK_TRACECONFIG_TRACEMUX_Pos;
	#endif

	/* Clear all faults */
	_ScbMemFaultAllFaultsReset();
	_ScbBusFaultAllFaultsReset();
	_ScbUsageFaultAllFaultsReset();

	_ScbHardFaultAllFaultsReset();

	/* Setup master clock */
	clock_init();

	/* Install default handler that simply resets the CPU
	* if configured in the kernel, NOP otherwise
	*/
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

static bool ftpan_32(void)
{
	if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) &&
	    (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)) {
		if ((((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30) &&
		    (((*(uint32_t *)0xF0000FEC) & 0x000000F0) == 0x0)) {
			return true;
		}
	}

	return false;
}

static bool ftpan_37(void)
{
	if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) &&
	    (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)) {
		if ((((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30) &&
		    (((*(uint32_t *)0xF0000FEC) & 0x000000F0) == 0x0)) {
			return true;
		}
	}

	return false;
}

static bool ftpan_36(void)
{
	if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) &&
	    (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)) {
		if ((((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30) &&
		    (((*(uint32_t *)0xF0000FEC) & 0x000000F0) == 0x0)) {
			return true;
		}
	}

	return false;
}

SYS_INIT(nordicsemi_nrf52_init, PRE_KERNEL_1, 0);
