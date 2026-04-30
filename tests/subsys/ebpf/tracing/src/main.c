/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ebpf/ebpf.h>
#include <zephyr/ebpf/ebpf_tracepoint.h>

extern void ebpf_tracing_detach(struct ebpf_prog *prog);

static const struct ebpf_insn exit_prog[] = {
	EBPF_MOV64_IMM(EBPF_REG_R0, 0),
	EBPF_EXIT_INSN(),
};

static const struct ebpf_insn invalid_prog[] = {
	EBPF_INSN_OP(0xFE, 0, 0, 0, 0),
	EBPF_EXIT_INSN(),
};

static void init_prog(struct ebpf_prog *prog, const char *name)
{
	*prog = (struct ebpf_prog) {
		.name = name,
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = exit_prog,
		.insn_cnt = ARRAY_SIZE(exit_prog),
		.state = EBPF_PROG_STATE_DISABLED,
		.attach_point = -1,
	};
}

ZTEST(ebpf_tracing, test_attach_conflict_is_rejected)
{
	struct ebpf_prog prog_a;
	struct ebpf_prog prog_b;
	int ret;

	init_prog(&prog_a, "prog_a");
	init_prog(&prog_b, "prog_b");

	ret = ebpf_prog_attach(&prog_a, EBPF_TP_THREAD_SWITCHED_IN);
	zassert_ok(ret, "first attach failed: %d", ret);

	ret = ebpf_prog_attach(&prog_b, EBPF_TP_THREAD_SWITCHED_IN);
	zassert_equal(ret, -EBUSY, "expected -EBUSY, got %d", ret);

	ebpf_prog_detach(&prog_a);

	ret = ebpf_prog_attach(&prog_b, EBPF_TP_THREAD_SWITCHED_IN);
	zassert_ok(ret, "attach after detach failed: %d", ret);

	ebpf_prog_detach(&prog_b);
}

ZTEST(ebpf_tracing, test_disable_does_not_bypass_verifier)
{
	struct ebpf_prog prog = {
		.name = "invalid_prog",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = invalid_prog,
		.insn_cnt = ARRAY_SIZE(invalid_prog),
		.state = EBPF_PROG_STATE_DISABLED,
		.attach_point = -1,
	};
	int ret;

	ret = ebpf_prog_attach(&prog, EBPF_TP_THREAD_SWITCHED_IN);
	zassert_ok(ret, "attach failed: %d", ret);

	ret = ebpf_prog_disable(&prog);
	zassert_ok(ret, "disable failed: %d", ret);
	zassert_equal(prog.state, EBPF_PROG_STATE_DISABLED,
		      "disable should leave program disabled");

	ret = ebpf_prog_enable(&prog);
	zassert_equal(ret, -EINVAL, "invalid program should fail verification, got %d", ret);
	zassert_equal(prog.state, EBPF_PROG_STATE_DISABLED,
		      "failed enable should keep program disabled");

	ebpf_prog_detach(&prog);
}

ZTEST(ebpf_tracing, test_detach_only_clears_own_slot)
{
	struct ebpf_prog prog_a;
	struct ebpf_prog prog_b;
	int ret;

	init_prog(&prog_a, "prog_a_owner");
	init_prog(&prog_b, "prog_b_owner");

	ret = ebpf_prog_attach(&prog_a, EBPF_TP_THREAD_SWITCHED_IN);
	zassert_ok(ret, "attach failed: %d", ret);

	prog_b.attach_point = EBPF_TP_THREAD_SWITCHED_IN;
	ebpf_tracing_detach(&prog_b);
	prog_b.attach_point = -1;

	ret = ebpf_prog_attach(&prog_b, EBPF_TP_THREAD_SWITCHED_IN);
	zassert_equal(ret, -EBUSY, "foreign detach should not clear occupied slot, got %d", ret);

	ebpf_prog_detach(&prog_a);
}

ZTEST_SUITE(ebpf_tracing, NULL, NULL, NULL, NULL, NULL);
