/*
 * Copyright (c) 2020 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/toolchain.h>

#include <cmsis_core.h>

extern uint32_t _vector_table;
extern uint32_t __vector_relay_handler;
extern uint32_t _vector_table_pointer;

ZTEST(arm_sw_vector_relay, test_arm_sw_vector_relay)
{
	uint32_t vector_relay_table_addr = (uint32_t)&__vector_relay_table;
	uint32_t vector_relay_handler_func = (uint32_t)&__vector_relay_handler;

	/* Verify that the vector relay table entries (except first two
	 * entries for MSP and ResetHandler) point to the relay handling
	 * function.
	 */
	const uint32_t *vector_relay_table_addr_val =
		(uint32_t *)(vector_relay_table_addr);

	for (int i = 2; i < 16 + CONFIG_NUM_IRQS; i++) {
		zassert_true(vector_relay_table_addr_val[i] ==
			vector_relay_handler_func,
			"vector relay table not pointing to the relay handler: 0x%x, 0x%x\n",
			vector_relay_table_addr_val[i],
			vector_relay_handler_func);
	}

#if defined(CONFIG_CPU_CORTEX_M_HAS_VTOR)

	uint32_t vector_table_addr = (uint32_t)&_vector_table;

	/* Verify that the forwarding vector table and the real
	 * interrupt vector table respect the VTOR.TBLOFF alignment
	 * requirements.
	 */
	uint32_t mask = MAX(128, Z_POW2_CEIL(4 * (16 + CONFIG_NUM_IRQS))) - 1;

	zassert_true(((vector_table_addr) & mask) == 0,
		"vector table not properly aligned: 0x%x\n",
		vector_table_addr);
	zassert_true(((vector_relay_table_addr) & mask) == 0,
		"vector relay table not properly aligned: 0x%x\n",
		vector_relay_table_addr);

	/* Verify that the VTOR points to the real vector table,
	 * NOT the table that contains the forwarding function.
	 */
	zassert_true(SCB->VTOR == (uint32_t)_vector_start,
		"VTOR not pointing to the real vector table\n");
#else
	/* If VTOR is not present then we already need to forward interrupts
	 * before loading any child chain-loadable image.
	 */
	zassert_true(_vector_table_pointer == (uint32_t)_vector_start,
		"vector table pointer not pointing to vector start, 0x%x, 0x%x\n",
		_vector_table_pointer, _vector_start);
#endif
}
/**
 * @}
 */
