/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ebpf/ebpf.h>

extern int ebpf_verify(const struct ebpf_prog *prog);

ZTEST(ebpf_verifier, test_positive_r10_offset_is_rejected)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_ST_MEM_W(EBPF_REG_R10, 4, 1),
		EBPF_EXIT_INSN(),
	};
	struct ebpf_prog prog = {
		.name = "bad_r10_offset",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
	};

	zassert_equal(ebpf_verify(&prog), -EINVAL,
		      "positive R10 offset should be rejected");
}

ZTEST(ebpf_verifier, test_negative_r10_offset_is_accepted)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_ST_MEM_W(EBPF_REG_R10, -4, 1),
		EBPF_EXIT_INSN(),
	};
	struct ebpf_prog prog = {
		.name = "good_r10_offset",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
	};

	zassert_ok(ebpf_verify(&prog), "negative R10 offset should pass");
}

ZTEST_SUITE(ebpf_verifier, NULL, NULL, NULL, NULL, NULL);
