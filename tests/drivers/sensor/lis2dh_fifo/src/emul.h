/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIS2DH_TEST_EMUL_H_
#define LIS2DH_TEST_EMUL_H_

#include <zephyr/drivers/emul.h>
#include <zephyr/kernel.h>

struct lis2dh_test_bus {
	uint8_t regs[64];
	uint64_t fail_mask;
	bool fail_all;
	unsigned int operations;
	unsigned int reads;
	unsigned int diagnostic_reads;
	unsigned int writes;
	unsigned int bursts;
	unsigned int last_len;
	uint8_t last_cmd;
	bool block_burst;
	struct k_sem burst_entered;
	struct k_sem burst_release;
};

void lis2dh_test_reset(const struct emul *emul);
void lis2dh_test_fill(const struct emul *emul, unsigned int count);

#endif
