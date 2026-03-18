/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ebpf/ebpf.h>

extern int64_t ebpf_vm_exec(const struct ebpf_prog *prog, void *ctx_data,
			    uint32_t ctx_size);
extern int ebpf_maps_init(void);

EBPF_MAP_DEFINE(test_array_map, EBPF_MAP_TYPE_ARRAY,
		sizeof(uint32_t), sizeof(uint32_t), 1);

#ifdef CONFIG_EBPF_MAP_PER_CPU_ARRAY
EBPF_MAP_DEFINE(test_percpu_map, EBPF_MAP_TYPE_PER_CPU_ARRAY,
		sizeof(uint32_t), sizeof(uint32_t), 1);
#endif

#ifdef CONFIG_EBPF_MAP_RINGBUF
struct ring_record {
	uint32_t a;
	uint16_t b;
};

EBPF_MAP_DEFINE(test_ringbuf_map, EBPF_MAP_TYPE_RINGBUF,
		0, sizeof(struct ring_record), 64);
#endif

static void *ebpf_maps_setup(void)
{
	ebpf_maps_init();
	return NULL;
}

ZTEST(ebpf_maps, test_map_handle_lookup_and_bounds)
{
	static struct ebpf_insn prog_insns[] = {
		EBPF_ST_MEM_W(EBPF_REG_R10, -4, 0),
		EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
		EBPF_ADD64_IMM(EBPF_REG_R2, -4),
		EBPF_MOV64_IMM(EBPF_REG_R1, 0),
		EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),
		EBPF_JEQ_IMM(EBPF_REG_R0, 0, 3),
		EBPF_LDX_MEM_W(EBPF_REG_R1, EBPF_REG_R0, 0),
		EBPF_ADD64_IMM(EBPF_REG_R1, 1),
		EBPF_STX_MEM_W(EBPF_REG_R0, EBPF_REG_R1, 0),
		EBPF_MOV64_IMM(EBPF_REG_R0, 0),
		EBPF_EXIT_INSN(),
	};
	struct ebpf_prog prog = {
		.name = "maps_bounds_prog",
		.type = EBPF_PROG_TYPE_TRACEPOINT,
		.insns = prog_insns,
		.insn_cnt = ARRAY_SIZE(prog_insns),
	};
	uint32_t key = 0;
	uint32_t *value;
	int64_t ret;

	zassert_not_equal(test_array_map.id, 0U, "map id should be assigned");
	prog_insns[3].imm = (int32_t)test_array_map.id;

	ret = ebpf_vm_exec(&prog, NULL, 0);
	zassert_equal(ret, 0, "expected VM success, got %lld", ret);

	value = ebpf_map_lookup_elem(&test_array_map, &key);
	zassert_not_null(value, "array map lookup failed");
	zassert_equal(*value, 1U, "expected counter increment, got %u", *value);
}

#ifdef CONFIG_EBPF_MAP_RINGBUF
ZTEST(ebpf_maps, test_ringbuf_output_and_read)
{
	struct ring_record in = {
		.a = 0x12345678,
		.b = 0x55aa,
	};
	struct ring_record out = { 0 };
	uint32_t out_size = 0;
	int ret;

	ret = ebpf_ringbuf_output(&test_ringbuf_map, &in, sizeof(in), 0);
	zassert_ok(ret, "ringbuf output failed: %d", ret);

	ret = ebpf_ringbuf_read(&test_ringbuf_map, &out, sizeof(out), &out_size);
	zassert_ok(ret, "ringbuf read failed: %d", ret);
	zassert_equal(out_size, sizeof(in), "unexpected ringbuf payload size");
	zassert_mem_equal(&out, &in, sizeof(in), "ringbuf payload mismatch");
}
#endif

#ifdef CONFIG_EBPF_MAP_PER_CPU_ARRAY
ZTEST(ebpf_maps, test_percpu_array_current_cpu_slot)
{
	uint32_t key = 0;
	uint32_t new_value = 7;
	uint32_t *value;
	int ret;

	ret = ebpf_map_update_elem(&test_percpu_map, &key, &new_value, 0);
	zassert_ok(ret, "percpu update failed: %d", ret);

	value = ebpf_map_lookup_elem(&test_percpu_map, &key);
	zassert_not_null(value, "percpu lookup returned NULL");
	zassert_equal(*value, new_value, "unexpected percpu value: %u", *value);
}
#endif

ZTEST_SUITE(ebpf_maps, NULL, ebpf_maps_setup, NULL, NULL, NULL);
