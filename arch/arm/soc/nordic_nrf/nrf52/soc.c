/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF52 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF52 family processor.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <cortex_m/exc.h>

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif

#include "nrf.h"

#define __SYSTEM_CLOCK_64M (64000000UL)

#ifdef CONFIG_SOC_NRF52832
static bool ftpan_32(void)
{
	if ((((*(u32_t *)0xF0000FE0) & 0x000000FF) == 0x6) &&
		(((*(u32_t *)0xF0000FE4) & 0x0000000F) == 0x0)) {
		if ((((*(u32_t *)0xF0000FE8) & 0x000000F0) == 0x30) &&
			(((*(u32_t *)0xF0000FEC) & 0x000000F0) == 0x0)) {
			return true;
		}
	}

	return false;
}

static bool ftpan_37(void)
{
	if ((((*(u32_t *)0xF0000FE0) & 0x000000FF) == 0x6) &&
		(((*(u32_t *)0xF0000FE4) & 0x0000000F) == 0x0)) {
		if ((((*(u32_t *)0xF0000FE8) & 0x000000F0) == 0x30) &&
			(((*(u32_t *)0xF0000FEC) & 0x000000F0) == 0x0)) {
			return true;
		}
	}

	return false;
}

static bool ftpan_36(void)
{
	if ((((*(u32_t *)0xF0000FE0) & 0x000000FF) == 0x6) &&
		(((*(u32_t *)0xF0000FE4) & 0x0000000F) == 0x0)) {
		if ((((*(u32_t *)0xF0000FE8) & 0x000000F0) == 0x30) &&
			(((*(u32_t *)0xF0000FEC) & 0x000000F0) == 0x0)) {
			return true;
		}
	}

	return false;
}

static bool errata_136_nrf52832(void)
{
	if ((((*(u32_t *)0xF0000FE0) & 0x000000FF) == 0x6) &&
	    (((*(u32_t *)0xF0000FE4) & 0x0000000F) == 0x0)) {
		if (((*(u32_t *)0xF0000FE8) & 0x000000F0) == 0x30) {
			return true;
		}
		if (((*(u32_t *)0xF0000FE8) & 0x000000F0) == 0x40) {
			return true;
		}
		if (((*(u32_t *)0xF0000FE8) & 0x000000F0) == 0x50) {
			return true;
		}
	}

	return false;
}

static void nordicsemi_nrf52832_init(void)
{
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
		*(volatile u32_t *)0x400005A0 = 0x3;
	}

	/* Workaround for FTPAN-36 "CLOCK: Some registers are not
	* reset when expected." found at Product Anomaly document
	* for your device located at https://www.nordicsemi.com/
	*/
	if (ftpan_36()) {
		NRF_CLOCK->EVENTS_DONE = 0;
		NRF_CLOCK->EVENTS_CTTO = 0;
	}

	/* Workaround for Errata 136 "System: Bits in RESETREAS are set when
	 * they should not be" found at the Errata document for your device
	 * located at https://infocenter.nordicsemi.com/
	 */
	if (errata_136_nrf52832()) {
		if (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk) {
			NRF_POWER->RESETREAS = ~POWER_RESETREAS_RESETPIN_Msk;
		}
	}

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
}
#endif /* CONFIG_SOC_NRF52832 */

#ifdef CONFIG_SOC_NRF52840
static bool errata_36(void)
{
	if ((*(u32_t *)0x10000130ul == 0x8ul) &&
	    (*(u32_t *)0x10000134ul == 0x0ul)) {
		return true;
	}

	return false;
}

static bool errata_98(void)
{
	if ((*(u32_t *)0x10000130ul == 0x8ul) &&
	    (*(u32_t *)0x10000134ul == 0x0ul)) {
		return true;
	}

	return false;
}

static bool errata_103(void)
{
	if ((*(u32_t *)0x10000130ul == 0x8ul) &&
	    (*(u32_t *)0x10000134ul == 0x0ul)) {
		return true;
	}

	return false;
}

static bool errata_115(void)
{
	if ((*(u32_t *)0x10000130ul == 0x8ul) &&
	    (*(u32_t *)0x10000134ul == 0x0ul)) {
		return true;
	}

	return false;
}

static bool errata_120(void)
{
	if ((*(u32_t *)0x10000130ul == 0x8ul) &&
	    (*(u32_t *)0x10000134ul == 0x0ul)) {
		return true;
	}

	return false;
}

static bool errata_136_nrf52840(void)
{
	if ((*(u32_t *)0x10000130ul == 0x8ul) &&
	    (*(u32_t *)0x10000134ul == 0x0ul)) {
		return true;
	}

	return false;
}

static void nordicsemi_nrf52840_init(void)
{
	/* Workaround for Errata 36 "CLOCK: Some registers are not reset when
	 * expected" found at the Errata document for your device located at
	 * https://infocenter.nordicsemi.com/
	 */
	if (errata_36()) {
		NRF_CLOCK->EVENTS_DONE = 0;
		NRF_CLOCK->EVENTS_CTTO = 0;
		NRF_CLOCK->CTIV = 0;
	}

	/* Workaround for Errata 98 "NFCT: Not able to communicate with the
	 * peer" found at the Errata document for your device located at
	 * https://infocenter.nordicsemi.com/
	 */
	if (errata_98()) {
		*(volatile u32_t *)0x4000568Cul = 0x00038148ul;
	}

	/* Workaround for Errata 103 "CCM: Wrong reset value of CCM
	 * MAXPACKETSIZE" found at the Errata document for your device
	 * located at https://infocenter.nordicsemi.com/
	 */
	if (errata_103()) {
		NRF_CCM->MAXPACKETSIZE = 0xFBul;
	}

	/* Workaround for Errata 115 "RAM: RAM content cannot be trusted upon
	 * waking up from System ON Idle or System OFF mode" found at the
	 * Errata document for your device located at
	 * https://infocenter.nordicsemi.com/
	 */
	if (errata_115()) {
		*(volatile u32_t *)0x40000EE4 =
			(*(volatile u32_t *) 0x40000EE4 & 0xFFFFFFF0) |
			(*(u32_t *)0x10000258 & 0x0000000F);
	}

	/* Workaround for Errata 120 "QSPI: Data read or written is corrupted"
	 * found at the Errata document for your device located at
	 * https://infocenter.nordicsemi.com/
	 */
	if (errata_120()) {
		*(volatile u32_t *)0x40029640ul = 0x200ul;
	}

	/* Workaround for Errata 136 "System: Bits in RESETREAS are set when
	 * they should not be" found at the Errata document for your device
	 * located at https://infocenter.nordicsemi.com/
	 */
	if (errata_136_nrf52840()) {
		if (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk) {
			NRF_POWER->RESETREAS = ~POWER_RESETREAS_RESETPIN_Msk;
		}
	}

	/* Configure GPIO pads as pPin Reset pin if Pin Reset capabilities
	 * desired.
	 * If CONFIG_GPIO_AS_PINRESET is not defined, pin reset will not be
	 * available. One GPIO (see Product Specification to see which one) will
	 * then be reserved for PinReset and not available as normal GPIO.
	 */
#if defined(CONFIG_GPIO_AS_PINRESET)
	if (((NRF_UICR->PSELRESET[0] & UICR_PSELRESET_CONNECT_Msk) !=
	     (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos)) ||
	    ((NRF_UICR->PSELRESET[1] & UICR_PSELRESET_CONNECT_Msk) !=
	     (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos))) {
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		}
		NRF_UICR->PSELRESET[0] = 18;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		}
		NRF_UICR->PSELRESET[1] = 18;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		}
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		}
		NVIC_SystemReset();
	}
#endif
	/* Enable SWO trace functionality. If ENABLE_SWO is not defined, SWO pin
	 * will be used as GPIO (see Product Specification to see which one).
	 */
#if defined(ENABLE_SWO)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Serial <<
		CLOCK_TRACECONFIG_TRACEMUX_Pos;
	NRF_P1->PIN_CNF[0] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos)
		| (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
#endif

	/* Enable Trace functionality. If ENABLE_TRACE is not defined, TRACE
	 * pins will be used as GPIOs (see Product Specification to see which
	 * ones).
	 */
#if defined(ENABLE_TRACE)
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Parallel <<
		CLOCK_TRACECONFIG_TRACEMUX_Pos;
	NRF_P0->PIN_CNF[7]	= (GPIO_PIN_CNF_DRIVE_H0H1 <<
			GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect <<
				GPIO_PIN_CNF_INPUT_Pos) |
			(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	NRF_P1->PIN_CNF[0]	= (GPIO_PIN_CNF_DRIVE_H0H1 <<
			GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect <<
				GPIO_PIN_CNF_INPUT_Pos) |
			(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	NRF_P0->PIN_CNF[12] = (GPIO_PIN_CNF_DRIVE_H0H1 <<
			GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect <<
				GPIO_PIN_CNF_INPUT_Pos) |
			(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	NRF_P0->PIN_CNF[11] = (GPIO_PIN_CNF_DRIVE_H0H1 <<
			GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect <<
				GPIO_PIN_CNF_INPUT_Pos) |
			(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	NRF_P1->PIN_CNF[9]	= (GPIO_PIN_CNF_DRIVE_H0H1 <<
			GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect <<
				GPIO_PIN_CNF_INPUT_Pos) |
			(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
#endif
}
#endif /* CONFIG_SOC_NRF52840 */

uint32_t SystemCoreClock __used = __SYSTEM_CLOCK_64M;

static void clock_init(void)
{
	SystemCoreClock = __SYSTEM_CLOCK_64M;
}

static int nordicsemi_nrf52_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

#ifdef CONFIG_SOC_NRF52832
	nordicsemi_nrf52832_init();
#endif
#ifdef CONFIG_SOC_NRF52840
	nordicsemi_nrf52840_init();
#endif
	/* Enable the FPU if the compiler used floating point unit
	 * instructions. Since the FPU consumes energy, remember to
	 * disable FPU use in the compiler if floating point unit
	 * operations are not used in your code.
	 */
#if defined(CONFIG_FLOAT)
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

	_ClearFaults();

	/* Setup master clock */
	clock_init();

	/* Install default handler that simply resets the CPU
	* if configured in the kernel, NOP otherwise
	*/
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(nordicsemi_nrf52_init, PRE_KERNEL_1, 0);
