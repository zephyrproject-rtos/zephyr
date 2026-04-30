/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ebpf/ebpf.h>

extern int64_t ebpf_vm_exec(const struct ebpf_prog *prog, void *ctx_data,
			    uint32_t ctx_size);

/* Test: MOV + ADD + EXIT return value */
ZTEST(ebpf_vm, test_mov_add_exit)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R0, 10),
		EBPF_ADD64_IMM(EBPF_REG_R0, 32),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_prog = {
		.name = "test_prog",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_prog, NULL, 0);

	zassert_equal(ret, 42, "Expected 42, got %lld", ret);
}

/* Test: SUB instruction */
ZTEST(ebpf_vm, test_sub)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R0, 100),
		EBPF_SUB64_IMM(EBPF_REG_R0, 58),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_sub = {
		.name = "test_sub",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_sub, NULL, 0);

	zassert_equal(ret, 42, "Expected 42, got %lld", ret);
}

/* Test: MOV register to register */
ZTEST(ebpf_vm, test_mov_reg)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R1, 99),
		EBPF_MOV64_REG(EBPF_REG_R0, EBPF_REG_R1),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_movreg = {
		.name = "test_movreg",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_movreg, NULL, 0);

	zassert_equal(ret, 99, "Expected 99, got %lld", ret);
}

/* Test: MUL instruction */
ZTEST(ebpf_vm, test_mul)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R0, 6),
		EBPF_INSN_OP(EBPF_OP_MUL64_IMM, EBPF_REG_R0, 0, 0, 7),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_mul = {
		.name = "test_mul",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_mul, NULL, 0);

	zassert_equal(ret, 42, "Expected 42, got %lld", ret);
}

/* Test: Division by zero returns 0 */
ZTEST(ebpf_vm, test_div_by_zero)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R0, 100),
		EBPF_INSN_OP(EBPF_OP_DIV64_IMM, EBPF_REG_R0, 0, 0, 0),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_div0 = {
		.name = "test_div0",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_div0, NULL, 0);

	zassert_equal(ret, 0, "Div by zero should return 0, got %lld", ret);
}

/* Test: AND / OR / XOR */
ZTEST(ebpf_vm, test_bitwise)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R0, 0xFF),
		EBPF_AND64_IMM(EBPF_REG_R0, 0x0F),
		/* R0 should be 0x0F = 15 */
		EBPF_OR64_IMM(EBPF_REG_R0, 0x30),
		/* R0 should be 0x3F = 63 */
		EBPF_XOR64_IMM(EBPF_REG_R0, 0x15),
		/* R0 should be 0x2A = 42 */
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_bits = {
		.name = "test_bits",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_bits, NULL, 0);

	zassert_equal(ret, 42, "Expected 42, got %lld", ret);
}

/* Test: Conditional jump (JEQ) */
ZTEST(ebpf_vm, test_jeq)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R0, 0),
		EBPF_MOV64_IMM(EBPF_REG_R1, 5),
		/* if R1 == 5, skip next insn */
		EBPF_JEQ_IMM(EBPF_REG_R1, 5, 1),
		EBPF_MOV64_IMM(EBPF_REG_R0, 99),  /* should be skipped */
		EBPF_MOV64_IMM(EBPF_REG_R0, 42),  /* should execute */
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_jeq = {
		.name = "test_jeq",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_jeq, NULL, 0);

	zassert_equal(ret, 42, "Expected 42, got %lld", ret);
}

/* Test: JNE (not equal) */
ZTEST(ebpf_vm, test_jne)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_MOV64_IMM(EBPF_REG_R0, 0),
		EBPF_MOV64_IMM(EBPF_REG_R1, 5),
		/* if R1 != 10, skip next insn */
		EBPF_JNE_IMM(EBPF_REG_R1, 10, 1),
		EBPF_MOV64_IMM(EBPF_REG_R0, 99),  /* should be skipped */
		EBPF_MOV64_IMM(EBPF_REG_R0, 1),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_jne = {
		.name = "test_jne",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_jne, NULL, 0);

	zassert_equal(ret, 1, "Expected 1, got %lld", ret);
}

/* Test: Memory store/load via stack (R10) */
ZTEST(ebpf_vm, test_stack_memory)
{
	static const struct ebpf_insn prog_insns[] = {
		/* Store 42 to stack at [R10-4] */
		EBPF_ST_MEM_W(EBPF_REG_R10, -4, 42),
		/* Load from stack [R10-4] into R0 */
		EBPF_LDX_MEM_W(EBPF_REG_R0, EBPF_REG_R10, -4),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_stack = {
		.name = "test_stack",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_stack, NULL, 0);

	zassert_equal(ret, 42, "Expected 42, got %lld", ret);
}

/* Test: Context read (R1 points to context data) */
ZTEST(ebpf_vm, test_context_read)
{
	struct test_ctx {
		uint32_t value;
	} ctx = { .value = 123 };

	static const struct ebpf_insn prog_insns[] = {
		/* R1 = ctx pointer, read ctx->value (offset 0) */
		EBPF_LDX_MEM_W(EBPF_REG_R0, EBPF_REG_R1, 0),
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_ctx_prog = {
		.name = "test_ctx",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_ctx_prog, &ctx, sizeof(ctx));

	zassert_equal(ret, 123, "Expected 123, got %lld", ret);
}

/* Test: CALL helper (ktime_get_ns returns non-zero) */
ZTEST(ebpf_vm, test_call_helper)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_CALL_HELPER(EBPF_HELPER_KTIME_GET_NS),
		/* R0 now has timestamp, should be > 0 */
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_call = {
		.name = "test_call",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_call, NULL, 0);

	zassert_true(ret >= 0, "ktime_get_ns should return >= 0, got %lld", ret);
}

/* Test: Unknown opcode returns error */
ZTEST(ebpf_vm, test_unknown_opcode)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_INSN_OP(0xFE, 0, 0, 0, 0), /* Invalid opcode */
		EBPF_EXIT_INSN(),
	};

	struct ebpf_prog test_bad_op = {
		.name = "test_bad_op",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};

	int64_t ret = ebpf_vm_exec(&test_bad_op, NULL, 0);

	zassert_equal(ret, -EINVAL, "Expected -EINVAL, got %lld", ret);
}

ZTEST(ebpf_vm, test_runtime_bounds_check_rejects_wrapped_region)
{
	static const struct ebpf_insn prog_insns[] = {
		EBPF_LDX_MEM_W(EBPF_REG_R0, EBPF_REG_R1, 0),
		EBPF_EXIT_INSN(),
	};
	struct ebpf_prog prog = {
		.name = "test_wrapped_region",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
		.state = EBPF_PROG_STATE_ENABLED,
		.attach_point = -1,
	};
	void *ctx = (void *)(uintptr_t)(UINTPTR_MAX - 1U);
	int64_t ret = ebpf_vm_exec(&prog, ctx, sizeof(uint32_t));

	zassert_equal(ret, -EFAULT, "wrapped region should be rejected, got %lld", ret);
}

ZTEST_SUITE(ebpf_vm, NULL, NULL, NULL, NULL, NULL);
