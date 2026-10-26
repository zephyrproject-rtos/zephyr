/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/ztest.h>

void __cyg_profile_func_enter(void *function, void *caller)
{
	ARG_UNUSED(function);
	ARG_UNUSED(caller);
}

void __cyg_profile_func_exit(void *function, void *caller)
{
	ARG_UNUSED(function);
	ARG_UNUSED(caller);
}

static void client_isr(const void *arg)
{
	uint32_t *calls = (uint32_t *)arg;

	(*calls)++;
}

ZTEST(shared_isr_literals, test_dispatch)
{
	uint32_t calls[2] = {0U, 0U};
	struct z_shared_isr_table_entry entry = {.client_num = 2U};

	entry.clients[0].isr = client_isr;
	entry.clients[0].arg = &calls[0];
	entry.clients[1].isr = client_isr;
	entry.clients[1].arg = &calls[1];

	z_shared_isr(&entry);
	zassert_equal(calls[0], 1U, "First client was not called once");
	zassert_equal(calls[1], 1U, "Second client was not called once");
}

ZTEST_SUITE(shared_isr_literals, NULL, NULL, NULL, NULL, NULL);
