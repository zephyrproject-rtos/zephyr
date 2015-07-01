/* system.c - system/hardware module for ti_lm3s6965 BSP */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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
for the ti_lm3s6965 BSP.
 */

#include <nanokernel.h>
#include <board.h>
#include <drivers/uart.h>

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
#define DO_CONSOLE_INIT
#endif

#define RCGC1 *((volatile uint32_t *)0x400FE104)

#define RCGC1_UART0_EN 0x00000001
#define RCGC1_UART1_EN 0x00000002
#define RCGC1_UART2_EN 0x00000004

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif

#if defined(DO_CONSOLE_INIT)

/**
 *
 * uart_generic_info_init - initialize generic information for one UART
 *
 * @return N/A
 *
 */

inline void uart_generic_info_init(struct uart_init_info *pInfo)
{
	pInfo->sys_clk_freq = SYSCLK_DEFAULT_IOSC_HZ;
	pInfo->baud_rate = CONFIG_UART_CONSOLE_BAUDRATE;
	/* Only supported in polling mode, but init all info fields */
	pInfo->int_pri = CONFIG_UART_CONSOLE_INT_PRI;
}

#endif /* DO_CONSOLE_INIT */

#if defined(DO_CONSOLE_INIT)

/**
 *
 * consoleInit - initialize target-only console
 *
 * Only used for debugging.
 *
 * @return N/A
 *
 */

#include <console/uart_console.h>

static void consoleInit(void)
{
	struct uart_init_info info;

	/* enable clock to UART0 */
	RCGC1 |= RCGC1_UART0_EN;

	uart_generic_info_init(&info);

	uart_init(CONFIG_UART_CONSOLE_INDEX, &info);

	uart_console_init();
}

#else
#define consoleInit()     \
	do {/* nothing */ \
	} while ((0))
#endif /* DO_CONSOLE_INIT */

#if defined(CONFIG_BLUETOOTH)
#if defined(CONFIG_BLUETOOTH_UART)
#include <bluetooth/uart.h>
#endif /* CONFIG_BLUETOOTH_UART */

static void bluetooth_init(void)
{
#if defined(CONFIG_BLUETOOTH_UART)
	/* Enable clock to UART1 */
	RCGC1 |= RCGC1_UART1_EN;

	/* General UART init */
	bt_uart_init();
#endif /* CONFIG_BLUETOOTH_UART */
}
#else /* CONFIG_BLUETOOTH */
#define bluetooth_init()	\
	do {/* nothing */	\
	} while ((0))
#endif /* CONFIG_BLUETOOTH */

/**
 *
 * _InitHardware - perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers and the
 * integrated 16550-compatible UART device driver.
 * Also initialize the timer device driver, if required.
 *
 * @return N/A
 */

void _InitHardware(void)
{
	consoleInit(); /* NOP if not needed */
	bluetooth_init(); /* NOP if not needed */

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();
}
