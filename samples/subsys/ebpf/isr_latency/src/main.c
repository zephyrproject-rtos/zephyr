/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF ISR latency profiler sample.
 *
 * Demonstrates using two cooperating eBPF programs to measure interrupt
 * service routine (ISR) execution time. One program records a timestamp
 * on ISR entry, the other computes elapsed time on ISR exit and writes
 * the result to a ring buffer map. The application drains the ring
 * buffer periodically and prints min/avg/max latency statistics.
 *
 * NOTE: This sample uses a single timestamp slot, so nested interrupts
 * may produce inaccurate results for the outer ISR. A production
 * implementation could use a per-CPU stack of timestamps.
 */

#include <zephyr/kernel.h>
#include <zephyr/ebpf/ebpf.h>
#include <zephyr/ebpf/ebpf_tracepoint.h>

/**
 * eBPF program: record the ISR entry timestamp.
 *
 * Equivalent C:
 *
 * @code{.c}
 *   uint64_t ts = ktime_get_ns();
 *   uint32_t key = 0;
 *   uint64_t *slot = map_lookup_elem(&ts_map, &key);
 *   if (slot) { *slot = ts; }
 *   return 0;
 * @endcode
 */
static struct ebpf_insn isr_entry_code[] = {
	/* R0 = ktime_get_ns() */
	EBPF_CALL_HELPER(EBPF_HELPER_KTIME_GET_NS),

	/* R6 = timestamp (callee-saved) */
	EBPF_MOV64_REG(EBPF_REG_R6, EBPF_REG_R0),

	/* *(u32*)(R10 - 4) = 0 (key on stack) */
	EBPF_ST_MEM_W(EBPF_REG_R10, -4, 0),

	/* R2 = &key */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),

	/* R1 = ts_map handle (patched at runtime) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),

	/* R0 = map_lookup_elem(R1, R2) */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),

	/* if (R0 == NULL) goto exit */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 1),

	/* *(u64*)(R0) = R6 -- store entry timestamp */
	EBPF_STX_MEM_DW(EBPF_REG_R0, EBPF_REG_R6, 0),

	/* return 0 */
	EBPF_MOV64_IMM(EBPF_REG_R0, 0),
	EBPF_EXIT_INSN(),
};

/**
 * eBPF program: compute ISR duration and output to ring buffer.
 *
 * Equivalent C:
 *
 * @code{.c}
 *   uint64_t exit_ts = ktime_get_ns();
 *   uint32_t key = 0;
 *   uint64_t *entry_ts = map_lookup_elem(&ts_map, &key);
 *   if (!entry_ts) return 0;
 *   uint64_t duration = exit_ts - *entry_ts;
 *   ringbuf_output(&events_map, &duration, sizeof(duration), 0);
 *   return 0;
 * @endcode
 */
static struct ebpf_insn isr_exit_code[] = {
	/* [0] R0 = ktime_get_ns() */
	EBPF_CALL_HELPER(EBPF_HELPER_KTIME_GET_NS),

	/* [1] R6 = exit timestamp */
	EBPF_MOV64_REG(EBPF_REG_R6, EBPF_REG_R0),

	/* [2] key = 0 on stack */
	EBPF_ST_MEM_W(EBPF_REG_R10, -4, 0),

	/* [3-4] R2 = &key */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),

	/* [5] R1 = ts_map handle (patched) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),

	/* [6] R0 = map_lookup_elem(ts_map, &key) */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),

	/* [7] if (R0 == NULL) goto [17] (return 0) */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 9),

	/* [8] R7 = *entry_ts */
	EBPF_LDX_MEM_DW(EBPF_REG_R7, EBPF_REG_R0, 0),

	/* [9] R6 = exit_ts - entry_ts = duration */
	EBPF_SUB64_REG(EBPF_REG_R6, EBPF_REG_R7),

	/* [10] *(u64*)(R10 - 16) = duration */
	EBPF_STX_MEM_DW(EBPF_REG_R10, EBPF_REG_R6, -16),

	/* [11] R1 = events_map handle (patched) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),

	/* [12-13] R2 = &duration on stack */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	EBPF_ADD64_IMM(EBPF_REG_R2, -16),

	/* [14] R3 = sizeof(uint64_t) */
	EBPF_MOV64_IMM(EBPF_REG_R3, 8),

	/* [15] R4 = flags = 0 */
	EBPF_MOV64_IMM(EBPF_REG_R4, 0),

	/* [16] ringbuf_output(events_map, &duration, 8, 0) */
	EBPF_CALL_HELPER(EBPF_HELPER_RINGBUF_OUTPUT),

	/* [17] return 0 */
	EBPF_MOV64_IMM(EBPF_REG_R0, 0),
	EBPF_EXIT_INSN(),
};

/* Array map with a single uint64_t slot to hold the ISR entry timestamp */
EBPF_MAP_DEFINE(ts_map, EBPF_MAP_TYPE_ARRAY, sizeof(uint32_t), sizeof(uint64_t), 1);

/* Ring buffer map for ISR duration events (each event is a uint64_t) */
EBPF_MAP_DEFINE(events_map, EBPF_MAP_TYPE_RINGBUF, 0, 0, 2048);

/* Define the two programs */
EBPF_PROG_DEFINE(isr_entry, EBPF_PROG_TYPE_TRACEPOINT,
		 isr_entry_code, ARRAY_SIZE(isr_entry_code));
EBPF_PROG_DEFINE(isr_exit, EBPF_PROG_TYPE_TRACEPOINT,
		 isr_exit_code, ARRAY_SIZE(isr_exit_code));

int main(void)
{
	int ret;

	printk("eBPF ISR latency profiler\n");
	printk("========================\n\n");

	/**
	 * Patch map handles into the bytecode.
	 * isr_entry_code[5] and isr_exit_code[5] hold the ts_map handle.
	 * isr_exit_code[11] holds the events_map handle.
	 */
	isr_entry_code[5].imm = (int32_t)ts_map.id;
	isr_exit_code[5].imm = (int32_t)ts_map.id;
	isr_exit_code[11].imm = (int32_t)events_map.id;

	/* Attach and enable the entry program */
	ret = ebpf_prog_attach(&isr_entry, EBPF_TP_ISR_ENTER);
	if (ret) {
		printk("Failed to attach isr_entry: %d\n", ret);
		return ret;
	}
	ret = ebpf_prog_enable(&isr_entry);
	if (ret) {
		printk("Failed to enable isr_entry: %d\n", ret);
		return ret;
	}

	/* Attach and enable the exit program */
	ret = ebpf_prog_attach(&isr_exit, EBPF_TP_ISR_EXIT);
	if (ret) {
		printk("Failed to attach isr_exit: %d\n", ret);
		return ret;
	}
	ret = ebpf_prog_enable(&isr_exit);
	if (ret) {
		printk("Failed to enable isr_exit: %d\n", ret);
		return ret;
	}

	printk("eBPF programs attached to ISR_ENTER and ISR_EXIT.\n");
	printk("Profiling ISR latency...\n\n");

	/* Periodically drain the ring buffer and print statistics */
	for (int i = 0; i < 10; i++) {
		k_sleep(K_SECONDS(1));

		uint64_t total_ns = 0;
		uint64_t min_ns = UINT64_MAX;
		uint64_t max_ns = 0;
		uint32_t count = 0;
		uint64_t duration_ns;
		uint32_t size_out;

		while (ebpf_ringbuf_read(&events_map, &duration_ns,
					 sizeof(duration_ns), &size_out) == 0) {
			total_ns += duration_ns;
			count++;
			if (duration_ns < min_ns) {
				min_ns = duration_ns;
			}
			if (duration_ns > max_ns) {
				max_ns = duration_ns;
			}
		}

		if (count > 0) {
			printk("[%2d] ISRs: %u  avg: %llu ns  "
			       "min: %llu ns  max: %llu ns\n",
			       i + 1, count, total_ns / count,
			       min_ns, max_ns);
		} else {
			printk("[%2d] No ISR events captured\n", i + 1);
		}
	}

	/* Disable and detach both programs */
	ebpf_prog_disable(&isr_entry);
	ebpf_prog_disable(&isr_exit);
	ebpf_prog_detach(&isr_entry);
	ebpf_prog_detach(&isr_exit);

	printk("\neBPF programs detached. Profiling complete.\n");

	return 0;
}
