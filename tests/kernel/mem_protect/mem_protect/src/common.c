/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_BMEM volatile bool valid_fault;

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d %d\n", reason, valid_fault);
	if (valid_fault) {
		printk("fatal error expected as part of test case\n");
		valid_fault = false; /* reset back to normal */
	} else {
		printk("fatal error was unexpected, aborting\n");
		printk("PROJECT EXECUTION FAILED\n");
		k_fatal_halt(reason);
	}
}
