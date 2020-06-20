/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(struct device *dev)
{
	struct device *muxa =
		device_get_binding(DT_LABEL(DT_NODELABEL(pinmux_a)));

	ARG_UNUSED(dev);

#if (ATMEL_SAM0_DT_SERCOM_CHECK(3, atmel_sam0_uart) && CONFIG_UART_SAM0)
	/* SERCOM3 on RX=PA16/pad 1, TX=PA17/pad 0 */
	pinmux_pin_set(muxa, 16, PINMUX_FUNC_D);
	pinmux_pin_set(muxa, 17, PINMUX_FUNC_D);
#endif

#if (ATMEL_SAM0_DT_SERCOM_CHECK(0, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(1, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(2, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif
#if (ATMEL_SAM0_DT_SERCOM_CHECK(5, atmel_sam0_uart) && CONFIG_UART_SAM0)
#warning Pin mapping may not be configured
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
