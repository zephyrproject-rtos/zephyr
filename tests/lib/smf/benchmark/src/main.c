/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/smf.h>
#include <zephyr/timing/timing.h>
/* Simple HSM:
 *
 *         R
 *       /   \
 *      A     B
 *     / \   / \
 *    C   D E   F
 *   / \         \
 *  G   H         J
 *
 *
 * Entry/exit are NULL to measure raw transition overhead.
 */
#define S(PARENT) .parent = (PARENT), .entry = NULL, .run = NULL, .exit = NULL, .initial = NULL
#define SIBLING_TRANSITIONS 3

static const struct smf_state R, A, B, C, D, E, F, G, H, J;
static const struct smf_state R = { S(NULL) };
static const struct smf_state A = { S(&R) };
static const struct smf_state B = { S(&R) };
static const struct smf_state C = { S(&A) };
static const struct smf_state D = { S(&A) };
static const struct smf_state E = { S(&B) };
static const struct smf_state F = { S(&B) };
static const struct smf_state G = { S(&C) };
static const struct smf_state H = { S(&C) };
static const struct smf_state J = { S(&F) };

struct bench_case {
	const char *name;
	const struct smf_state *a;
	const struct smf_state *b;
};

static const struct bench_case cases[] = {
	{ "(A->B)",  &A, &B },
	{ "(C->D)",  &C, &D },
	{ "(G->H)",  &G, &H },
	{ "(G<->G)", &G, &G },
	{ "(C->G)",  &C, &G },
	{ "(A->H)",  &A, &H },
	{ "(G->D)",  &G, &D },
	{ "(D->E)",  &D, &E },
	{ "(J->D)",  &J, &D },
	{ "(J->G)",  &J, &G },
};

void print_transition_tree(void)
{
	printk("        R\n");
	printk("      /   \\\n");
	printk("     A     B\n");
	printk("    / \\   / \\\n");
	printk("   C   D E   F\n");
	printk("  / \\         \\\n");
	printk(" G   H         J\n\n");
}

static void run_lca_transition_speed_benchmark(const struct bench_case *bc, uint64_t reps,
					       uint64_t *avg_cycles)
{
	struct smf_ctx ctx = {0};
	timing_t t0, t1;

	if (reps == 0) {
		*avg_cycles = 0;
		printk("ERROR: reps invalid value\n");
		return;
	}

	smf_set_initial(&ctx, bc->a);

	timing_init();
	timing_start();
	t0 = timing_counter_get();
	for (uint32_t i = 0; i < reps; ++i) {
		smf_set_state(&ctx, bc->b);
		smf_set_state(&ctx, bc->a);
	}
	t1 = timing_counter_get();

	uint64_t total_cycles = timing_cycles_get(&t0, &t1);
	uint64_t transitions = (uint64_t)reps * 2;

	timing_stop();

	*avg_cycles = (total_cycles + transitions - 1) / transitions;
}

ZTEST(smf_bench_lca, test_transition_cycles)
{
	/* Number of transitions between two states = (reps * 2) */
	const uint64_t reps = 200000;
	uint8_t i;

	printk("\n=== SMF transition micro-benchmark ===\n");
	print_transition_tree();

	printk("Sibling Transitions\n");
	for (i = 0; i < SIBLING_TRANSITIONS; ++i) {
		uint64_t avg_cycles = 0;

		run_lca_transition_speed_benchmark(&cases[i], reps, &avg_cycles);

		/* Also print ns/transition for readability */
		uint32_t ns32 = (uint32_t)timing_cycles_to_ns(avg_cycles);

		printk("%s : %u cycles/transition (%u ns)\n",
		       cases[i].name, (uint32_t)avg_cycles, ns32);
	}

	printk("\nOther Transitions\n");
	for (; i < (ARRAY_SIZE(cases)); ++i) {
		uint64_t avg_cycles = 0;

		run_lca_transition_speed_benchmark(&cases[i], reps, &avg_cycles);

		/* Also print ns/transition for readability */
		uint32_t ns32 = (uint32_t)timing_cycles_to_ns(avg_cycles);

		printk("%s : %u cycles/transition (%u ns)\n",
		       cases[i].name, (uint32_t)avg_cycles, ns32);
	}
	printk("\n");
	ztest_test_pass();
}

ZTEST_SUITE(smf_bench_lca, NULL, NULL, NULL, NULL, NULL);
