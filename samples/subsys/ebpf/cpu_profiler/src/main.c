/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF CPU profiler with idle-duration histogram.
 *
 * Two eBPF programs cooperate on the IDLE_ENTER / IDLE_EXIT tracepoints to
 * build an in-kernel profile of CPU utilisation:
 *
 *  - idle_enter — timestamps entry into the idle thread.
 *  - idle_exit  — 49-instruction program that
 *     - (1) computes idle duration,
 *     - (2) accumulates total idle nanoseconds,
 *     - (3) counts transitions, and
 *     - (4) classifies each idle period into a 6-bucket histogram.
 *
 * The application drains the maps every two seconds and prints an ASCII
 * histogram together with the active/idle ratio.
 *
 * Maps used:
 *   ts_map     — ARRAY[1]  uint64_t  entry timestamp
 *   stats_map  — ARRAY[2]  uint64_t  [0]=total_idle_ns, [1]=transition_count
 *   histogram  — ARRAY[6]  uint64_t  bucket counts
 *
 * Histogram buckets:
 *   0:      < 100 us
 *   1:  100 - 500 us
 *   2:  500 us - 1 ms
 *   3:    1 - 10 ms
 *   4:   10 - 100 ms
 *   5:      > 100 ms
 */

#include <zephyr/kernel.h>
#include <zephyr/ebpf/ebpf.h>
#include <zephyr/ebpf/ebpf_tracepoint.h>
#include <string.h>

#define NUM_BUCKETS      6
#define NUM_REPORTS      5
#define REPORT_INTERVAL  K_SECONDS(2)
#define BAR_WIDTH        24

static const char *bucket_labels[NUM_BUCKETS] = {
	"     < 100 us",
	" 100 - 500 us",
	" 500 us - 1ms",
	"    1 - 10 ms",
	"  10 - 100 ms",
	"     > 100 ms",
};

/**
 * idle_enter — record entry timestamp (11 instructions)
 *
 * @code{.c}
 *   uint64_t ts = ktime_get_ns();
 *   uint32_t key = 0;
 *   uint64_t *slot = map_lookup_elem(&ts_map, &key);
 *   if (slot) { *slot = ts; }
 *   return 0;
 * @endcode
 */
static struct ebpf_insn idle_enter_code[] = {
	/* [0]  R0 = ktime_get_ns() */
	EBPF_CALL_HELPER(EBPF_HELPER_KTIME_GET_NS),
	/* [1]  R6 = timestamp */
	EBPF_MOV64_REG(EBPF_REG_R6, EBPF_REG_R0),
	/* [2]  *(u32*)(R10-4) = 0 */
	EBPF_ST_MEM_W(EBPF_REG_R10, -4, 0),
	/* [3]  R2 = &key */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	/* [4] */
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),
	/* [5]  R1 = ts_map.id (patched) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),
	/* [6]  R0 = map_lookup_elem */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),
	/* [7]  if NULL → [9] */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 1),
	/* [8]  *slot = timestamp */
	EBPF_STX_MEM_DW(EBPF_REG_R0, EBPF_REG_R6, 0),
	/* [9]  return 0 */
	EBPF_MOV64_IMM(EBPF_REG_R0, 0),
	/* [10] */
	EBPF_EXIT_INSN(),
};

/**
 * idle_exit — THE 49-INSTRUCTION PROGRAM
 *
 * Five phases executed on every idle→active transition:
 *
 *   Phase 1 [0-9]   Compute duration = exit_ts − entry_ts
 *   Phase 2 [10-17] Accumulate total idle ns   → stats_map[0]
 *   Phase 3 [18-26] Increment idle => active transition count → stats_map[1]
 *   Phase 4 [27-37] Classify duration into histogram bucket (R9)
 *   Phase 5 [38-46] Increment histogram[bucket]
 *                   R6 < 100000?    => bucket 0
 *                   R6 < 500000?    => bucket 1
 *                   R6 < 1000000?   => bucket 2
 *                   R6 < 10000000?  => bucket 3
 *                   R6 < 100000000? => bucket 4
 *                   else            => bucket 5
 *   Exit    [47-48]
 *
 * Register allocation:
 *   R6  = duration_ns (preserved across all CALLs)
 *   R7  = entry_ts    (phase 1 only)
 *   R8  = scratch for load/add/store
 *   R9  = histogram bucket index (phase 4-5)
 *   R10 = frame pointer (stack key at R10-4)
 *
 */
static struct ebpf_insn idle_exit_code[] = {
	/* === Phase 1: compute idle duration === */

	/* [0]  R0 = ktime_get_ns() */
	EBPF_CALL_HELPER(EBPF_HELPER_KTIME_GET_NS),
	/* [1]  R6 = exit_ts */
	EBPF_MOV64_REG(EBPF_REG_R6, EBPF_REG_R0),
	/* [2]  key = 0 */
	EBPF_ST_MEM_W(EBPF_REG_R10, -4, 0),
	/* [3]  R2 = &key */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	/* [4] */
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),
	/* [5]  R1 = ts_map.id (patched) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),
	/* [6]  R0 = map_lookup_elem(ts_map, &key) */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),
	/* [7]  if NULL → exit [47]  (offset 39) */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 39),
	/* [8]  R7 = *entry_ts */
	EBPF_LDX_MEM_DW(EBPF_REG_R7, EBPF_REG_R0, 0),
	/* [9]  R6 = exit_ts − entry_ts = duration */
	EBPF_SUB64_REG(EBPF_REG_R6, EBPF_REG_R7),

	/* === Phase 2: stats_map[0] += duration === */

	/* [10] R2 = &key (key=0 still on stack) */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	/* [11] */
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),
	/* [12] R1 = stats_map.id (patched) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),
	/* [13] R0 = map_lookup_elem(stats_map, &key=0) */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),
	/* [14] if NULL → [18] (skip to phase 3) */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 3),
	/* [15] R8 = current total */
	EBPF_LDX_MEM_DW(EBPF_REG_R8, EBPF_REG_R0, 0),
	/* [16] R8 += duration */
	EBPF_ADD64_REG(EBPF_REG_R8, EBPF_REG_R6),
	/* [17] store back */
	EBPF_STX_MEM_DW(EBPF_REG_R0, EBPF_REG_R8, 0),

	/* === Phase 3: stats_map[1] += 1 === */

	/* [18] key = 1 */
	EBPF_ST_MEM_W(EBPF_REG_R10, -4, 1),
	/* [19] R2 = &key */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	/* [20] */
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),
	/* [21] R1 = stats_map.id (patched) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),
	/* [22] R0 = map_lookup_elem(stats_map, &key=1) */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),
	/* [23] if NULL → [27] (skip to phase 4) */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 3),
	/* [24] R8 = current count */
	EBPF_LDX_MEM_DW(EBPF_REG_R8, EBPF_REG_R0, 0),
	/* [25] R8 += 1 */
	EBPF_ADD64_IMM(EBPF_REG_R8, 1),
	/* [26] store back */
	EBPF_STX_MEM_DW(EBPF_REG_R0, EBPF_REG_R8, 0),

	/* === Phase 4: classify duration into bucket (R9) ===
	 *
	 *   bucket 0:      < 100 us    (< 100000 ns)
	 *   bucket 1:  100 - 500 us
	 *   bucket 2:  500 us - 1 ms
	 *   bucket 3:    1 - 10 ms
	 *   bucket 4:   10 - 100 ms
	 *   bucket 5:      >= 100 ms
	 */

	/* [27] R9 = 0 (default bucket) */
	EBPF_MOV64_IMM(EBPF_REG_R9, 0),
	/* [28] if R6 < 100000 → bucket 0 → [38] */
	EBPF_JLT_IMM(EBPF_REG_R6, 100000, 9),
	/* [29] */
	EBPF_ADD64_IMM(EBPF_REG_R9, 1),
	/* [30] if R6 < 500000 → bucket 1 → [38] */
	EBPF_JLT_IMM(EBPF_REG_R6, 500000, 7),
	/* [31] */
	EBPF_ADD64_IMM(EBPF_REG_R9, 1),
	/* [32] if R6 < 1000000 → bucket 2 → [38] */
	EBPF_JLT_IMM(EBPF_REG_R6, 1000000, 5),
	/* [33] */
	EBPF_ADD64_IMM(EBPF_REG_R9, 1),
	/* [34] if R6 < 10000000 → bucket 3 → [38] */
	EBPF_JLT_IMM(EBPF_REG_R6, 10000000, 3),
	/* [35] */
	EBPF_ADD64_IMM(EBPF_REG_R9, 1),
	/* [36] if R6 < 100000000 → bucket 4 → [38] */
	EBPF_JLT_IMM(EBPF_REG_R6, 100000000, 1),
	/* [37] >= 100 ms → bucket 5 */
	EBPF_ADD64_IMM(EBPF_REG_R9, 1),

	/* === Phase 5: histogram[bucket] += 1 === */

	/* [38] key = bucket index */
	EBPF_STX_MEM_W(EBPF_REG_R10, EBPF_REG_R9, -4),
	/* [39] R2 = &key */
	EBPF_MOV64_REG(EBPF_REG_R2, EBPF_REG_R10),
	/* [40] */
	EBPF_ADD64_IMM(EBPF_REG_R2, -4),
	/* [41] R1 = histogram.id (patched) */
	EBPF_MOV64_IMM(EBPF_REG_R1, 0),
	/* [42] R0 = map_lookup_elem(histogram, &bucket) */
	EBPF_CALL_HELPER(EBPF_HELPER_MAP_LOOKUP_ELEM),
	/* [43] if NULL → exit [47] */
	EBPF_JEQ_IMM(EBPF_REG_R0, 0, 3),
	/* [44] R8 = current count */
	EBPF_LDX_MEM_DW(EBPF_REG_R8, EBPF_REG_R0, 0),
	/* [45] R8 += 1 */
	EBPF_ADD64_IMM(EBPF_REG_R8, 1),
	/* [46] store back */
	EBPF_STX_MEM_DW(EBPF_REG_R0, EBPF_REG_R8, 0),

	/* === Exit === */

	/* [47] return 0 */
	EBPF_MOV64_IMM(EBPF_REG_R0, 0),
	/* [48] */
	EBPF_EXIT_INSN(),
};

/* ------------------------------------------------------------------ */
/*  Maps                                                              */
/* ------------------------------------------------------------------ */
/* Single uint64_t slot for the idle-entry timestamp */
EBPF_MAP_DEFINE(ts_map, EBPF_MAP_TYPE_ARRAY,
		sizeof(uint32_t), sizeof(uint64_t), 1);

/* stats_map[0] = total idle ns, stats_map[1] = transition count */
EBPF_MAP_DEFINE(stats_map, EBPF_MAP_TYPE_ARRAY,
		sizeof(uint32_t), sizeof(uint64_t), 2);

/* 6-bucket histogram of idle durations */
EBPF_MAP_DEFINE(histogram, EBPF_MAP_TYPE_ARRAY,
		sizeof(uint32_t), sizeof(uint64_t), NUM_BUCKETS);

/* ------------------------------------------------------------------ */
/*  Programs                                                          */
/* ------------------------------------------------------------------ */
EBPF_PROG_DEFINE(idle_enter, EBPF_PROG_TYPE_TRACEPOINT,
		 idle_enter_code, ARRAY_SIZE(idle_enter_code));
EBPF_PROG_DEFINE(idle_exit, EBPF_PROG_TYPE_TRACEPOINT,
		 idle_exit_code, ARRAY_SIZE(idle_exit_code));

/* ------------------------------------------------------------------ */
/*  Worker thread — one sleep duration per report phase               */
/* ------------------------------------------------------------------ */
#define WORKER_STACK_SIZE 1024
#define WORKER_PRIORITY   7

static K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);
static struct k_thread worker_thread;

/*
 * main() advances worker_phase before each report interval so that
 * every report captures a histogram dominated by a single bucket.
 */
static volatile int worker_phase;

static void worker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		switch (worker_phase) {
		case 0: k_sleep(K_USEC(300)); break;  /* → bucket 1: 100-500 us */
		case 1: k_sleep(K_USEC(800)); break;  /* → bucket 2: 500us-1ms  */
		case 2: k_sleep(K_MSEC(5)); break;    /* → bucket 3: 1-10 ms    */
		case 3: k_sleep(K_MSEC(50)); break;   /* → bucket 4: 10-100 ms  */
		case 4: k_sleep(K_MSEC(200)); break;  /* → bucket 5: > 100 ms   */
		}
	}
}

/* ------------------------------------------------------------------ */
/*  Histogram printing                                                */
/* ------------------------------------------------------------------ */
static void print_bar(uint32_t count, uint32_t max_count)
{
	char bar[BAR_WIDTH + 1];
	int len = 0;

	if (max_count > 0) {
		len = (int)((uint64_t)count * BAR_WIDTH / max_count);
	}
	if (count > 0 && len == 0) {
		len = 1;
	}
	if (len > BAR_WIDTH) {
		len = BAR_WIDTH;
	}

	memset(bar, '#', len);
	memset(bar + len, ' ', BAR_WIDTH - len);
	bar[BAR_WIDTH] = '\0';
	printk("%s", bar);
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */
int main(void)
{
	int ret;

	printk("eBPF CPU profiler\n");
	printk("=================\n\n");

	/* ---- Patch map handles into bytecode ---- */
	idle_enter_code[5].imm  = (int32_t)ts_map.id;

	idle_exit_code[5].imm   = (int32_t)ts_map.id;
	idle_exit_code[12].imm  = (int32_t)stats_map.id;
	idle_exit_code[21].imm  = (int32_t)stats_map.id;
	idle_exit_code[41].imm  = (int32_t)histogram.id;

	/* ---- Attach and enable ---- */
	ret = ebpf_prog_attach(&idle_enter, EBPF_TP_IDLE_ENTER);
	if (ret) {
		printk("Failed to attach idle_enter: %d\n", ret);
		return ret;
	}
	ret = ebpf_prog_enable(&idle_enter);
	if (ret) {
		printk("Failed to enable idle_enter: %d\n", ret);
		return ret;
	}

	ret = ebpf_prog_attach(&idle_exit, EBPF_TP_IDLE_EXIT);
	if (ret) {
		printk("Failed to attach idle_exit: %d\n", ret);
		return ret;
	}
	ret = ebpf_prog_enable(&idle_exit);
	if (ret) {
		printk("Failed to enable idle_exit: %d\n", ret);
		return ret;
	}

	printk("eBPF programs attached to IDLE_ENTER / IDLE_EXIT.\n");

	/* Start the worker thread */
	k_thread_create(&worker_thread, worker_stack, WORKER_STACK_SIZE,
			worker_entry, NULL, NULL, NULL,
			WORKER_PRIORITY, 0, K_NO_WAIT);

	printk("Profiling (reports every 2 s)...\n");

	static const char *phase_labels[NUM_REPORTS] = {
		"sleep 300 us", "sleep 800 us", "sleep 5 ms",
		"sleep 50 ms", "sleep 200 ms",
	};

	/* ---- Reporting loop ---- */
	uint64_t prev_idle_ns = 0;
	uint64_t prev_transitions = 0;
	uint64_t prev_buckets[NUM_BUCKETS] = {0};
	int64_t  prev_ms = k_uptime_get();

	for (int i = 0; i < NUM_REPORTS; i++) {
		/* Switch the worker to this phase's sleep duration */
		worker_phase = i;
		k_sleep(REPORT_INTERVAL);

		int64_t now_ms = k_uptime_get();
		int64_t elapsed_ms = now_ms - prev_ms;

		prev_ms = now_ms;

		/* --- Read stats_map --- */
		uint32_t key;
		uint64_t *val;

		key = 0;
		val = ebpf_map_lookup_elem(&stats_map, &key);
		uint64_t cur_idle_ns = val ? *val : 0;
		uint64_t idle_delta = cur_idle_ns - prev_idle_ns;

		prev_idle_ns = cur_idle_ns;

		key = 1;
		val = ebpf_map_lookup_elem(&stats_map, &key);
		uint64_t cur_trans = val ? *val : 0;
		uint64_t trans_delta = cur_trans - prev_transitions;

		prev_transitions = cur_trans;

		/* --- Compute utilisation --- */
		uint32_t idle_pct = 0;

		if (elapsed_ms > 0) {
			idle_pct = (uint32_t)(idle_delta /
				   ((uint64_t)elapsed_ms * 10000));
		}
		if (idle_pct > 100) {
			idle_pct = 100;
		}

		/* --- Read histogram (delta since last report) --- */
		uint32_t delta[NUM_BUCKETS];
		uint32_t max_count = 0;

		for (int b = 0; b < NUM_BUCKETS; b++) {
			key = (uint32_t)b;
			val = ebpf_map_lookup_elem(&histogram, &key);
			uint64_t cur = val ? *val : 0;

			delta[b] = (uint32_t)(cur - prev_buckets[b]);
			prev_buckets[b] = cur;
			if (delta[b] > max_count) {
				max_count = delta[b];
			}
		}

		/* --- Print report --- */
		printk("\n--- CPU Profiler [%d] %s ---\n",
		       i + 1, phase_labels[i]);
		printk("Active: %u%%  Idle: %u%%  "
		       "Transitions: %llu\n",
		       100 - idle_pct, idle_pct, trans_delta);
		printk("\nIdle duration distribution:\n");

		for (int b = 0; b < NUM_BUCKETS; b++) {
			printk("  %s : ", bucket_labels[b]);
			print_bar(delta[b], max_count);
			printk("  %u\n", delta[b]);
		}
	}

	/* ---- Cleanup ---- */
	ebpf_prog_disable(&idle_enter);
	ebpf_prog_disable(&idle_exit);
	ebpf_prog_detach(&idle_enter);
	ebpf_prog_detach(&idle_exit);

	printk("\neBPF programs detached. Profiling complete.\n");

	return 0;
}
