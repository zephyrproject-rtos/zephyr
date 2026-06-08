/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/arm/cortex_a_r/sys_io.h>

#define BCM2835_GPIO_BASE	0x20200000U
#define BCM2835_GPFSEL1		(BCM2835_GPIO_BASE + 0x04U)
#define BCM2835_GPPUD		(BCM2835_GPIO_BASE + 0x94U)
#define BCM2835_GPPUDCLK0	(BCM2835_GPIO_BASE + 0x98U)

#define BCM2835_GPIO14_SHIFT	12U
#define BCM2835_GPIO15_SHIFT	15U
#define BCM2835_GPIO_FSEL_MASK	0x7U
#define BCM2835_GPIO_ALT5	0x2U

#define BCM2835_MU_IO		0x00U
#define BCM2835_MU_IER		0x04U
#define BCM2835_MU_IIR		0x08U
#define BCM2835_MU_LCR		0x0CU
#define BCM2835_MU_MCR		0x10U
#define BCM2835_MU_LSR		0x14U
#define BCM2835_MU_CNTL		0x20U
#define BCM2835_MU_BAUD		0x28U

#define BCM2835_AUX_ENABLES_OFFSET	0x3CU
#define BCM2835_AUX_ENABLES_MU		BIT(0)
#define BCM2835_MU_LSR_TX_EMPTY	BIT(5)

#if DT_HAS_CHOSEN(zephyr_console) && \
	(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), brcm_bcm2835_aux_uart) || \
	 DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), brcm_bcm2711_aux_uart))

#define BCM2835_EARLY_UART_BASE	DT_REG_ADDR(DT_CHOSEN(zephyr_console))
#define BCM2835_EARLY_UART_CLOCK_HZ DT_PROP(DT_CHOSEN(zephyr_console), clock_frequency)
#define BCM2835_EARLY_UART_BAUDRATE DT_PROP(DT_CHOSEN(zephyr_console), current_speed)

static inline void bcm2835_early_delay(void)
{
	for (volatile int i = 0; i < 150; i++) {
		;
	}
}

static inline uint32_t bcm2835_early_uart_divider(void)
{
	return (BCM2835_EARLY_UART_CLOCK_HZ / (BCM2835_EARLY_UART_BAUDRATE * 8U)) - 1U;
}

static void bcm2835_early_uart_putc(uint8_t c)
{
	if (c == '\n') {
		while ((sys_read32(BCM2835_EARLY_UART_BASE + BCM2835_MU_LSR) &
			BCM2835_MU_LSR_TX_EMPTY) == 0U) {
			;
		}
		sys_write32('\r', BCM2835_EARLY_UART_BASE + BCM2835_MU_IO);
	}

	while ((sys_read32(BCM2835_EARLY_UART_BASE + BCM2835_MU_LSR) &
		BCM2835_MU_LSR_TX_EMPTY) == 0U) {
		;
	}
	sys_write32(c, BCM2835_EARLY_UART_BASE + BCM2835_MU_IO);
}

static void bcm2835_early_uart_init(void)
{
	uint32_t ra;

	sys_write32(sys_read32(BCM2835_EARLY_UART_BASE - BCM2835_AUX_ENABLES_OFFSET) |
		    BCM2835_AUX_ENABLES_MU,
		    BCM2835_EARLY_UART_BASE - BCM2835_AUX_ENABLES_OFFSET);
	sys_write32(0U, BCM2835_EARLY_UART_BASE + BCM2835_MU_IER);
	sys_write32(0U, BCM2835_EARLY_UART_BASE + BCM2835_MU_CNTL);
	sys_write32(3U, BCM2835_EARLY_UART_BASE + BCM2835_MU_LCR);
	sys_write32(0U, BCM2835_EARLY_UART_BASE + BCM2835_MU_MCR);
	sys_write32(0xC6U, BCM2835_EARLY_UART_BASE + BCM2835_MU_IIR);
	sys_write32(bcm2835_early_uart_divider(), BCM2835_EARLY_UART_BASE + BCM2835_MU_BAUD);

	ra = sys_read32(BCM2835_GPFSEL1);
	ra &= ~(BCM2835_GPIO_FSEL_MASK << BCM2835_GPIO14_SHIFT);
	ra |= BCM2835_GPIO_ALT5 << BCM2835_GPIO14_SHIFT;
	ra &= ~(BCM2835_GPIO_FSEL_MASK << BCM2835_GPIO15_SHIFT);
	ra |= BCM2835_GPIO_ALT5 << BCM2835_GPIO15_SHIFT;
	sys_write32(ra, BCM2835_GPFSEL1);

	sys_write32(0U, BCM2835_GPPUD);
	bcm2835_early_delay();
	sys_write32(BIT(14) | BIT(15), BCM2835_GPPUDCLK0);
	bcm2835_early_delay();
	sys_write32(0U, BCM2835_GPPUD);
	sys_write32(0U, BCM2835_GPPUDCLK0);

	sys_write32(3U, BCM2835_EARLY_UART_BASE + BCM2835_MU_CNTL);
}

int arch_printk_char_out(int c)
{
	static bool initialized;

	if (!initialized) {
		bcm2835_early_uart_init();
		initialized = true;
	}

	bcm2835_early_uart_putc((uint8_t)c);

	return c;
}

#endif
