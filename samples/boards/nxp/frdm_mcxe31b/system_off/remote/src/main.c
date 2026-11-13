/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include "../../src/standby.h"

int main(void)
{
	struct standby_data *const shared =
		(struct standby_data *)DT_REG_ADDR(DT_NODELABEL(stdby_ram));

	sys_cache_instr_enable();
	sys_cache_data_enable();
	shared->state = CPU1_READY;
	__DSB();

	while (shared->request != CPU1_STOP) {
		k_sleep(K_MSEC(1));
	}

	(void)irq_lock();
#if defined(CONFIG_ZERO_LATENCY_IRQS)
	(void)arch_zli_lock();
#endif
	SysTick->CTRL = 0U;
	for (size_t i = 0; i < ARRAY_SIZE(NVIC->ICER); i++) {
		NVIC->ICER[i] = UINT32_MAX;
		NVIC->ICPR[i] = UINT32_MAX;
	}
	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
	if (sys_cache_data_flush_all() < 0) {
		shared->state = CPU1_ERROR;
	} else {
		sys_cache_data_disable();
		shared->state = CPU1_PARKED;
	}
	__DSB();
	__ISB();

	for (;;) {
		__WFI();
	}
}
