/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Raspberry Pi RP2040 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Raspberry Pi RP2040 family processor.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <hardware/regs/resets.h>
#include <hardware/clocks.h>
#include <hardware/resets.h>

#ifdef CONFIG_RUNTIME_NMI
extern void z_arm_nmi_init(void);
#define NMI_INIT() z_arm_nmi_init()
#else
#define NMI_INIT()
#endif

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

static int rp2040_init(const struct device *arg)
{
	uint32_t key;

	reset_block(~(RESETS_RESET_IO_QSPI_BITS | RESETS_RESET_PADS_QSPI_BITS |
		      RESETS_RESET_PLL_USB_BITS | RESETS_RESET_PLL_SYS_BITS));

	unreset_block_wait(RESETS_RESET_BITS &
			   ~(RESETS_RESET_ADC_BITS | RESETS_RESET_RTC_BITS |
			     RESETS_RESET_SPI0_BITS | RESETS_RESET_SPI1_BITS |
			     RESETS_RESET_UART0_BITS | RESETS_RESET_UART1_BITS |
			     RESETS_RESET_USBCTRL_BITS));

	clocks_init();

	unreset_block_wait(RESETS_RESET_BITS);

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(rp2040_init, PRE_KERNEL_1, 0);
