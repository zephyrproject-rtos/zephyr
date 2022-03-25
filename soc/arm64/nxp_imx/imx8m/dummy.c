/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONFIG_UART_MCUX_IUART

#define DT_DRV_COMPAT nxp_imx_iuart

#include <device.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
#include <errno.h>
#include <soc.h>

static int dummy_init(const struct device *dev)
{
	return 0;
}

#define IUART_MCUX_INIT(n)						\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &dummy_init,				\
			    NULL,					\
			    NULL,					\
			    NULL,					\
			    PRE_KERNEL_1,				\
			    0,						\
			    NULL);

DT_INST_FOREACH_STATUS_OKAY(IUART_MCUX_INIT)

#endif
