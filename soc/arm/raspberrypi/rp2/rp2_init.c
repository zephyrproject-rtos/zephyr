/*
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pico.h"

#include "hardware/regs/m0plus.h"
#include "hardware/regs/resets.h"
#include "hardware/structs/mpu.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/padsbank0.h"

#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/resets.h"

void rp2_init(void)
{
	reset_block(~(RESETS_RESET_IO_QSPI_BITS |
				RESETS_RESET_PADS_QSPI_BITS |
				RESETS_RESET_PLL_USB_BITS |
				RESETS_RESET_PLL_SYS_BITS));

	unreset_block_wait(RESETS_RESET_BITS & ~(
					RESETS_RESET_ADC_BITS |
					RESETS_RESET_RTC_BITS |
					RESETS_RESET_SPI0_BITS |
					RESETS_RESET_SPI1_BITS |
					RESETS_RESET_UART0_BITS |
					RESETS_RESET_UART1_BITS |
					RESETS_RESET_USBCTRL_BITS));

	clocks_init();

	unreset_block_wait(RESETS_RESET_BITS);
}
