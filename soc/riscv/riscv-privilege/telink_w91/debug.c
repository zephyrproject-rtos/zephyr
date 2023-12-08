/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

#if (CONFIG_TELINK_W91_DEBUG_PORT == 0)
#define UART_BASE_ADDR                     0xf0200000
#elif (CONFIG_TELINK_W91_DEBUG_PORT == 1)
#define UART_BASE_ADDR                     0xf0300000
#else
#error unsupported debug backend
#endif /* CONFIG_TELINK_W91_DEBUG_PORT */

#define GPIO_BASE_ADDR                     0xf0700000

#define __read_reg32(addr)                 (*(volatile uint32_t *)(addr))
#define __write_reg32(addr, value)         (*(volatile uint32_t *)(addr) = (uint32_t)(value))

#define F_REQ                              40000000

#define OFT_ATCUART_HWC                    0x10
#define OFT_ATCUART_OSC                    0x14
#define OFT_ATCUART_RBR                    0x20
#define OFT_ATCUART_THR                    0x20
#define OFT_ATCUART_DLL                    0x20
#define OFT_ATCUART_IER                    0x24
#define OFT_ATCUART_DLM                    0x24
#define OFT_ATCUART_IIR                    0x28
#define OFT_ATCUART_FCR                    0x28
#define OFT_ATCUART_LCR                    0x2c
#define OFT_ATCUART_MCR                    0x30
#define OFT_ATCUART_LSR                    0x34
#define OFT_ATCUART_MSR                    0x38
#define OFT_ATCUART_SCR                    0x3c

static void debug_init(void)
{
	static volatile bool debug_inited;

	if (!debug_inited) {
		unsigned int keys = arch_irq_lock();

		if (!debug_inited) {
			debug_inited = true;

			__write_reg32(GPIO_BASE_ADDR + 0x28,
				__read_reg32(GPIO_BASE_ADDR + 0x28) & ~0x0ff00000);

			const uint32_t os = __read_reg32(UART_BASE_ADDR + OFT_ATCUART_OSC) & 0x1f;
			const uint32_t div = F_REQ / (CONFIG_TELINK_W91_DEBUG_BAUDRATE * os);
			uint32_t lc = __read_reg32(UART_BASE_ADDR + OFT_ATCUART_LCR);

			lc |= 0x80;
			__write_reg32(UART_BASE_ADDR + OFT_ATCUART_LCR, lc);

			__write_reg32(UART_BASE_ADDR + OFT_ATCUART_DLM, (uint8_t)(div >> 8));
			__write_reg32(UART_BASE_ADDR + OFT_ATCUART_DLL, (uint8_t)div);

			lc &= ~(0x80);
			__write_reg32(UART_BASE_ADDR + OFT_ATCUART_LCR, lc);

			__write_reg32(UART_BASE_ADDR + OFT_ATCUART_LCR, 3);
			__write_reg32(UART_BASE_ADDR + OFT_ATCUART_FCR, 7);
		}
		arch_irq_unlock(keys);
	}
}

int arch_printk_char_out(int c)
{
	debug_init();
	while (!(__read_reg32(UART_BASE_ADDR + OFT_ATCUART_LSR) & (1 << 5))) {
	}
	__write_reg32(UART_BASE_ADDR + OFT_ATCUART_THR, c);

	return 0;
}
