/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/ztest.h>
#include <zephyr/ztress.h>

/* Uart device used to unlock shell if deadlock symptom occurrec. */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* Collision test variables */
#define MAX_SHELL_PRINT_CNT 1000
#define MAX_PRINTK_CNT 3000
#define MAX_TEST_TIMEOUT 60000 /* Unit: ms */

int shell_print_iter;

static bool func_shell_fprintf(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	const struct shell *sh = shell_backend_uart_get_ptr();

	/* Yield the contronl first and has chance to preempt the thread used printk() */
	k_usleep(15000);
	shell_fprintf(sh, SHELL_NORMAL,
		">Iter%d shell_fprintf 11111111111111111111111111111111111111<\n", iter_cnt);
	shell_print_iter++;

	return 1;
}

static bool func_printk(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	printk(">Iter%d printk 22222222222222222222----deadlock---------<\n", iter_cnt);

	return 1;
}

/* Custom PM policy handler to forbid ec enter deep sleep */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);

	return NULL;
}

static void shell_timeout_abort(struct k_timer *timer)
{
	if (shell_print_iter != MAX_SHELL_PRINT_CNT) {
		/* Enable TX transmit interrupt explicitly to bypass the symptom */
		uart_irq_tx_enable(uart_dev);
		printk("A deadlock on shell occurred\n");
	}
	ztress_abort();
}

ZTEST(shell_collision, test_shell_fprintf_printk_concurrency)
{
	struct k_timer timer;

	/* A timer to jump out of the deadlock symptom */
	k_timer_init(&timer, shell_timeout_abort, NULL);
	k_timer_start(&timer, K_MSEC(MAX_TEST_TIMEOUT), K_NO_WAIT);

	ZTRESS_EXECUTE(ZTRESS_THREAD(func_shell_fprintf, (void *)0,
				     MAX_SHELL_PRINT_CNT, 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(func_printk, (void *)0,
				     MAX_PRINTK_CNT, 0, Z_TIMEOUT_TICKS(20)));
	k_timer_stop(&timer);
	zassert_true(shell_print_iter == MAX_SHELL_PRINT_CNT);
}

ZTEST_SUITE(shell_collision, NULL, NULL, NULL, NULL, NULL);
