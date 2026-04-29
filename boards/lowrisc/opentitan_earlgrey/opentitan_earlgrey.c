/*
 * Copyright (c) 2024 The Fifth Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/arch/cpu.h>

/*
 * Pinmux (PINMUX_AON) base address.
 * Source: hw/top_earlgrey/sw/autogen/top_earlgrey.h
 *         TOP_EARLGREY_PINMUX_AON_BASE_ADDR = 0x40460000
 */
#define PINMUX_BASE             0x40460000u

/*
 * MIO_PERIPH_INSEL_0 register array base offset.
 * Write insel value to PERIPH_INSEL_0 + (periph_idx * 4) to connect
 * an MIO pad to a peripheral input.
 * Source: hw/top_earlgrey/ip_autogen/pinmux/data/pinmux.hjson
 *         MIO_PERIPH_INSEL_0 offset = 0xe8
 */
#define PERIPH_INSEL_0_OFFSET   0x00e8u

/*
 * MIO_OUTSEL_0 register array base offset.
 * Write outsel value to MIO_OUTSEL_0 + (mio_pad_idx * 4) to connect
 * a peripheral output to an MIO pad.
 * Source: hw/top_earlgrey/ip_autogen/pinmux/data/pinmux.hjson
 *         MIO_OUTSEL_0 offset = 0x288
 */
#define MIO_OUTSEL_0_OFFSET     0x0288u

/*
 * UART0 pinmux configuration.
 *
 * RX: peripheral input index 42 (kTopEarlgreyPinmuxPeripheralInUart0Rx)
 *     connected to MIO pad IOC3, insel value 27 (kTopEarlgreyPinmuxInselIoc3)
 *
 * TX: MIO output pad index 26 (kTopEarlgreyPinmuxMioOutIoc4)
 *     driven by peripheral output 45 (kTopEarlgreyPinmuxOutselUart0Tx)
 *
 * Source: hw/top_earlgrey/sw/autogen/top_earlgrey.h
 *         sw/device/silicon_creator/lib/drivers/pinmux.c (kInputUart0, kOutputUart0)
 */
#define UART0_RX_PERIPH_IDX     42u
#define UART0_RX_INSEL          27u
#define UART0_TX_MIO_IDX        26u
#define UART0_TX_OUTSEL         45u

static int opentitan_pinmux_init(void)
{
	/* Route IOC3 → UART0 RX peripheral input */
	sys_write32(UART0_RX_INSEL,
		    PINMUX_BASE + PERIPH_INSEL_0_OFFSET + UART0_RX_PERIPH_IDX * 4u);

	/* Route UART0 TX peripheral output → IOC4 */
	sys_write32(UART0_TX_OUTSEL,
		    PINMUX_BASE + MIO_OUTSEL_0_OFFSET + UART0_TX_MIO_IDX * 4u);

	return 0;
}

SYS_INIT(opentitan_pinmux_init, PRE_KERNEL_1, 0);
