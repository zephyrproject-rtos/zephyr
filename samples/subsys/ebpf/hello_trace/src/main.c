/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF hello_trace sample.
 *
 * Demonstrates attaching an eBPF program to the THREAD_SWITCHED_IN
 * tracepoint to count context switches. The counter is stored in an
 * array map and printed periodically.
 */

#include <zephyr/kernel.h>
#include <zephyr/ebpf/ebpf.h>
#include <zephyr/ebpf/ebpf_tracepoint.h>

/**
 * eBPF program: increment a counter each time it runs.
 *
 * Equivalent C logic:
 *
 * @code{.c}
 *   uint32_t key = 0;
 *   uint32_t *val = map_lookup_elem(&counter_map, &key);
 *   if (val) { *val += 1; }
 *   return 0;
 * @endcode
 */
static struct ebpf_insn count_switches_code[] = {
	/* Store key=0 on stack: *(u32*)(R10-4) = 0 */
	EBPF_ST_MEM_W(EBPF_REG_R10, -4, 0),

	/* R2 = R10 - 4 (pointer to key on stack) */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),

	/* R1 = counter_map handle (patched once at runtime after init) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0), /* placeholder, patched below */

	/* R0 = map_lookup_elem(R1=map, R2=key) */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),

	/* if R0 == NULL, goto exit */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 3),

	/* R1 = *(u32*)(R0 + 0) -- load current count */
	EBPF_LDX_MEM_W(EBPF_REG_R1, EBPF_REG_R0, 0),

	/* R1 += 1 */
	EBPF_ADD64_IMM(EBPF_REG_R1, 1),

	/* *(u32*)(R0 + 0) = R1 -- store incremented count */
	EBPF_STX_MEM_W(EBPF_REG_R0, EBPF_REG_R1, 0),

	/* R0 = 0 (return value) */
	EBPF_MOV64_IMM(EBPF_REG_R0, 0),
	EBPF_EXIT_INSN(),
};

/* Define the counter map: 1 entry of uint32_t */
EBPF_MAP_DEFINE(counter_map, EBPF_MAP_TYPE_ARRAY,
		sizeof(uint32_t), sizeof(uint32_t), 1);

/* Define the program */
EBPF_PROG_DEFINE(count_switches, EBPF_PROG_TYPE_SCHED,
		 count_switches_code, ARRAY_SIZE(count_switches_code));

/* Worker thread to generate context switches */
#define WORKER_STACK_SIZE 1024
#define WORKER_PRIORITY   7

static K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);
static struct k_thread worker_thread;

static void worker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		k_sleep(K_MSEC(100));
	}
}

int main(void)
{
	printk("eBPF hello_trace sample\n");
	printk("=======================\n\n");

	/**
	 * Patch the map handle into the bytecode.
	 * The helper resolves this small integer handle to the actual
	 * map object, avoiding pointer truncation on 64-bit targets.
	 */
	count_switches_code[3].imm = (int32_t)counter_map.id;

	/* Attach to THREAD_SWITCHED_IN tracepoint and enable */
	int ret = ebpf_prog_attach(&count_switches, EBPF_TP_THREAD_SWITCHED_IN);

	if (ret) {
		printk("Failed to attach: %d\n", ret);
		return ret;
	}

	ret = ebpf_prog_enable(&count_switches);
	if (ret) {
		printk("Failed to enable: %d\n", ret);
		return ret;
	}

	printk("eBPF program 'count_switches' attached and enabled.\n");
	printk("Counting context switches...\n\n");

	/* Start a worker thread to generate context switches */
	k_thread_create(&worker_thread, worker_stack, WORKER_STACK_SIZE,
			worker_entry, NULL, NULL, NULL, WORKER_PRIORITY, 0, K_NO_WAIT);

	/* Periodically read and display the counter */
	for (int i = 0; i < 10; i++) {
		k_sleep(K_SECONDS(1));

		uint32_t key = 0;
		uint32_t *val = ebpf_map_lookup_elem(&counter_map, &key);

		if (val) {
			printk("[%2d] Context switches: %u (avg exec: %llu ns)\n", i + 1, *val,
				count_switches.run_count > 0 ?
				count_switches.run_time_ns / count_switches.run_count : 0);
		}
	}

	/* Disable and detach */
	ebpf_prog_disable(&count_switches);
	ebpf_prog_detach(&count_switches);

	printk("\neBPF program detached. Sample complete.\n");

	return 0;
}
