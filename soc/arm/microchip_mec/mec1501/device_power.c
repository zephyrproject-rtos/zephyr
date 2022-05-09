/*
 * Copyright (c) 2019 Microchip Technology Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/pm/pm.h>
#include <soc.h>

/*
 * CPU will spin up to DEEP_SLEEP_WAIT_SPIN_CLK_REQ times
 * waiting for PCR CLK_REQ bits to clear except for the
 * CPU bit itself. This is not necessary as the sleep hardware
 * will wait for all CLK_REQ to clear once WFI has executed.
 * Once all CLK_REQ signals are clear the hardware will transition
 * to the low power state.
 */
/* #define DEEP_SLEEP_WAIT_ON_CLK_REQ_ENABLE */
#define DEEP_SLEEP_WAIT_SPIN_CLK_REQ		1000


/*
 * Some peripherals if enabled always assert their CLK_REQ bits.
 * For example, any peripheral with a clock generator such as
 * timers, counters, UART, etc. We save the enables for these
 * peripherals, disable them, and restore the enabled state upon
 * wake.
 */
#define DEEP_SLEEP_PERIPH_SAVE_RESTORE


/*
 * Light sleep: PLL remains on. Fastest wake latency.
 */
void soc_lite_sleep_enable(void)
{
	SCB->SCR &= ~(1ul << 2);
	PCR_REGS->SYS_SLP_CTRL = MCHP_PCR_SYS_SLP_LIGHT;
}

/*
 * Deep sleep: PLL is turned off. Wake is fast. PLL requires
 * a minimum of 3ms to lock. During this time the main clock
 * will be ramping up from ~16 to 24 MHz.
 */
void soc_deep_sleep_enable(void)
{
	SCB->SCR = (1ul << 2); /* Cortex-M4 SLEEPDEEP */
	PCR_REGS->SYS_SLP_CTRL = MCHP_PCR_SYS_SLP_HEAVY;
}

/*
 * Clear PCR Sleep control sleep all causing HW to de-assert all peripheral
 * SLP_EN signals. HW will does this automatically only if it vectors to an
 * ISR after wake. We are masking ISR's from running until we restore
 * peripheral state therefore we force HW to de-assert the SLP_EN signals.
 */
void soc_deep_sleep_disable(void)
{
	PCR_REGS->SYS_SLP_CTRL = 0U;
	SCB->SCR &= ~(1ul << 2); /* disable Cortex-M4 SLEEPDEEP */
}


void soc_deep_sleep_wait_clk_idle(void)
{
#ifdef DEEP_SLEEP_WAIT_ON_CLK_REQ_ENABLE
	uint32_t clkreq, cnt;

	cnt = DEEP_SLEEP_WAIT_CLK_REQ;
	do {
		clkreq = PCR_REGS->CLK_REQ0 | PCR_REGS->CLK_REQ1
			 | PCR_REGS->CLK_REQ2 | PCR_REGS->CLK_REQ3
			 | PCR_REGS->CLK_REQ4;
	} while ((clkreq != (1ul << MCHP_PCR1_CPU_POS)) && (cnt-- != 0));
#endif
}


/*
 * Allow peripherals connected to external masters to wake the PLL but not
 * the EC. Once the peripheral has serviced the external master the PLL
 * will be turned back off. For example, if the eSPI master requests eSPI
 * configuration information or state of virtual wires the EC doesn't need
 * to be involved. The hardware can power on the PLL long enough to service
 * the request and then turn the PLL back off.  The SMBus and I2C peripherals
 * in slave mode can also make use of this feature.
 */
void soc_deep_sleep_non_wake_en(void)
{
#ifdef CONFIG_ESPI_XEC
	GIRQ22_REGS->SRC = 0xfffffffful;
	GIRQ22_REGS->EN_SET = (1ul << 9);
#endif
}

void soc_deep_sleep_non_wake_dis(void)
{
#ifdef CONFIG_ESPI_XEC
	GIRQ22_REGS->EN_CLR = 0xfffffffful;
	GIRQ22_REGS->SRC = 0xfffffffful;
#endif
}

/* Variables used to save various HW state */
#ifdef DEEP_SLEEP_PERIPH_SAVE_RESTORE

static uint32_t ecs[1];

static void deep_sleep_save_ecs(void)
{
	ecs[0] = ECS_REGS->ETM_CTRL;
	ECS_REGS->ETM_CTRL = 0;
}

struct ds_timer_info {
	uintptr_t addr;
	uint32_t restore_mask;
};

const struct ds_timer_info ds_timer_tbl[] = {
	{
		(uintptr_t)&B16TMR0_REGS->CTRL, 0
	},
	{
		(uintptr_t)&B16TMR1_REGS->CTRL, 0
	},
	{
		(uintptr_t)&B32TMR0_REGS->CTRL, 0
	},
	{
		(uintptr_t)&B32TMR1_REGS->CTRL, 0
	},
	{
		(uintptr_t)&CCT_REGS->CTRL,
		(MCHP_CCT_CTRL_COMP1_SET | MCHP_CCT_CTRL_COMP0_SET),
	},
};
#define NUM_DS_TIMER_ENTRIES \
	(sizeof(ds_timer_tbl) / sizeof(struct ds_timer_info))


static uint32_t timers[NUM_DS_TIMER_ENTRIES];
static uint8_t uart_activate[3];

static void deep_sleep_save_uarts(void)
{
	uart_activate[0] = UART0_REGS->ACTV;
	if (uart_activate[0]) {
		while ((UART0_REGS->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}
	UART0_REGS->ACTV = 0;
	uart_activate[1] = UART1_REGS->ACTV;
	if (uart_activate[1]) {
		while ((UART1_REGS->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}
	UART1_REGS->ACTV = 0;
	uart_activate[2] = UART2_REGS->ACTV;
	if (uart_activate[2]) {
		while ((UART2_REGS->LSR & MCHP_UART_LSR_TEMT) == 0) {
		}
	}
	UART2_REGS->ACTV = 0;
}

static void deep_sleep_save_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		timers[i] = REG32(p->addr);
		REG32(p->addr) = 0;
		p++;
	}
}

static void deep_sleep_restore_ecs(void)
{
	ECS_REGS->ETM_CTRL = ecs[0];
}

static void deep_sleep_restore_uarts(void)
{
	UART0_REGS->ACTV = uart_activate[0];
	UART1_REGS->ACTV = uart_activate[1];
	UART2_REGS->ACTV = uart_activate[2];
}

static void deep_sleep_restore_timers(void)
{
	const struct ds_timer_info *p;
	uint32_t i;

	p = &ds_timer_tbl[0];
	for (i = 0; i < NUM_DS_TIMER_ENTRIES; i++) {
		REG32(p->addr) = timers[i] & ~p->restore_mask;
		p++;
	}
}

void soc_deep_sleep_periph_save(void)
{
	deep_sleep_save_uarts();
	deep_sleep_save_ecs();
	deep_sleep_save_timers();
}

void soc_deep_sleep_periph_restore(void)
{
	deep_sleep_restore_ecs();
	deep_sleep_restore_uarts();
	deep_sleep_restore_timers();
}

#else

void soc_deep_sleep_periph_save(void)
{
}

void soc_deep_sleep_periph_restore(void)
{
}

#endif /* DEEP_SLEEP_PERIPH_SAVE_RESTORE */
