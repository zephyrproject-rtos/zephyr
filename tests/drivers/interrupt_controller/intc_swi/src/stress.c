/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <ztress.h>
#include <sys/atomic.h>
#include <drivers/interrupt_controller/intc_swi.h>

#define STRESS_SWI_COUNT 3

struct swi_stress_ctx {
	struct swi_channel swi;
	uint32_t cb_count;
	atomic_t trigger_count;
};

static struct swi_stress_ctx stress_swi[STRESS_SWI_COUNT];

static void swi_stress_cb(struct swi_channel *swi)
{
	struct swi_stress_ctx *ctx = CONTAINER_OF(swi, struct swi_stress_ctx, swi);

	ctx->cb_count++;
}

static bool stress_func(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	struct swi_stress_ctx *ctx = user_data;
	int result;

	result = swi_channel_trigger(&ctx->swi);
	if (result == 0) {
		atomic_inc(&ctx->trigger_count);
	} else  if (result != -EALREADY) {
		return false;
	}

	return true;
}

void test_intc_swi_stress(void)
{
	for (int i = 0; i < STRESS_SWI_COUNT; i++) {
		swi_channel_init(&stress_swi[i].swi, swi_stress_cb);
	}

	ztress_set_timeout(K_MSEC(10000));
	ZTRESS_EXECUTE(ZTRESS_TIMER(stress_func, &stress_swi[0], 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(stress_func, &stress_swi[0], 0, 2000, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(stress_func, &stress_swi[1], 0, 2000, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(stress_func, &stress_swi[2], 0, 2000, Z_TIMEOUT_TICKS(20)));

	for (int i = 0; i < STRESS_SWI_COUNT; i++) {
		swi_channel_deinit(&stress_swi[i].swi);
		zassert_equal(stress_swi[i].trigger_count, stress_swi[i].cb_count,
			      "Number of successful SWI triggers does not match the number "
			      "of executed callbacks: %u != %u",
			      stress_swi[i].trigger_count, stress_swi[i].cb_count);
	}
}
