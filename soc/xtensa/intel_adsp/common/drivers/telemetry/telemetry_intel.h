/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TELEMETRY_INTEL_
#define _TELEMETRY_INTEL_

#include <stdint.h>
#include <adsp_types.h>

#define FW_REPORTED_MAX_CORES_COUNT 5
#define FW_MAX_REPORTED_TASKS 32
#define AREGS_COUNT 64
#define LP_TIMER_DELAY_CAUSE_COUNT 1
#define MAX_NUMBER_MEM_BINS 1

extern struct byte_array shared_telemetry_buffer;

struct system_tick_info_s {
	uint32_t count;
	uint32_t last_sys_tick_count;
	uint32_t max_sys_tick_count;
	uint32_t last_ccount;
	uint32_t avg_utilization;
	uint32_t peak_utilization;
	uint32_t peak_utilization_4k;
	uint32_t peak_utilization_8k;
	uint32_t rsvd[2];
};

struct deadlock_info_s {
	uint32_t register_a0;
	uint32_t register_a1;
	uint32_t cached_stack_ptr;
	uint32_t run_condition_data[9];
};

struct assert_statistics_s {
	uint32_t asserts_triggered_count;
	uint32_t last_assert_log_id;
};

struct assert_info_s {
	uint32_t            mode;
	struct assert_statistics_s stats[FW_REPORTED_MAX_CORES_COUNT];
};

struct xxxruns_info_s {
	uint32_t gateway_underruns_count;
	uint32_t last_underrun_gateway_node_id;
	uint32_t gateway_overruns_count;
	uint32_t last_overrun_gateway_node_id;
	uint32_t mixer_underruns_count;
	uint32_t last_mixer_underrun_resource_id;
};

struct performance_info_s {
	uint32_t cpu_frequency;
	uint32_t dropped_lp_timer_ticks_cause[LP_TIMER_DELAY_CAUSE_COUNT];
};

struct sram_mem_info_s {
	uint32_t total_pages_cnt;
	uint32_t pages_in_use_cnt;
};

struct sram_mem_pool_info_s {
	uint32_t registered_pools_cnt;
	uint32_t pools_in_use_cnt;
	uint32_t registered_pages_cnt;
	uint32_t pages_in_use_cnt;
};

struct mem_pools_info_s {
	struct sram_mem_info_s      hpsram_mem_info;
	struct sram_mem_pool_info_s hpsram_mem_pool_info;
	struct sram_mem_info_s      lpsram_mem_info;
	struct sram_mem_pool_info_s lpsram_mem_pool_info;
};

struct timeout_info_s {
	uint32_t expected_value;
	uint32_t current_value;
};

struct ulp_prevention_info_s {
	uint32_t preventedby_high_power;
	uint32_t preventedby_idle_mode_count_low;
	uint32_t idle_mode_count_down_;
	uint32_t preventedby_on_pipeline_state;
	uint32_t preventedby_fastmode_task;
	uint32_t prevent_autonomus_reset_counter;
};

struct transition_info_s {
	uint32_t entry_cnt;
	uint32_t exit_cnt;
	uint32_t last_entry_systick;
};

struct vad_kpd_stats_s {
	uint32_t vad_cnt;
	uint32_t kpd_cnt;
};

struct ulp_telemetry_s {
	struct ulp_prevention_info_s prevention_info;
	struct transition_info_s     transition_info;
	struct vad_kpd_stats_s       vad_kpd_stats;
};

struct thread_entry_point_info_s {
	uintptr_t call_stack;
	uint32_t start_systick;
};

struct task_info_s {
	uint32_t task_id;
	uint8_t task_state;
	uint8_t task_type;
	uint8_t task_subtype;
	uint8_t core_id;
	struct thread_entry_point_info_s blockade_entry_point;
	struct thread_entry_point_info_s cs_entry_point;
};

struct CoreExceptionRecord {
	uint32_t version;
	uint32_t stackdump_completion;
	uint64_t timestamp;
	uint32_t rec_state;
	uint32_t exec_ctx;
	uint32_t epc_1;
	uint32_t eps_2;
	uint32_t epc_2;
	uint32_t depc;
	uint32_t debugcause;
	uint32_t exccause;
	uint32_t excvaddr;
	uint32_t excsave;
	uint32_t interrupt;
	uint32_t ar[AREGS_COUNT];
	uint32_t windowbase;
	uint32_t windowstart;
	uint32_t mem_epc[4];
	uint32_t stack_base_addr;
};

struct TelemetryWndData {
	uint32_t            separator_1;
	struct system_tick_info_s  system_tick_info[FW_REPORTED_MAX_CORES_COUNT];
	uint32_t            separator_2;
	struct deadlock_info_s     deadlock_info[FW_REPORTED_MAX_CORES_COUNT];
	uint32_t            separator_3;
	struct assert_info_s       assert_info;
	uint32_t            separator_4;
	struct xxxruns_info_s      xxxruns_info;
	uint32_t            separator_5;
	struct performance_info_s  performance_info;
	uint32_t            separator_6;
	struct mem_pools_info_s    mem_pools_info;
	uint32_t            separator_7;
	struct timeout_info_s      timeout_info;
	uint32_t            separator_8;
	struct ulp_telemetry_s     ulp_telemetry;
	uint32_t            separator_9;
	struct transition_info_s   evad_transition_info;
	uint32_t            separator_10;
	struct task_info_s         task_info[FW_MAX_REPORTED_TASKS];
	uint32_t            separator_11;
	struct CoreExceptionRecord core_exception_record[FW_REPORTED_MAX_CORES_COUNT];
};

struct dca_telemetry_s {
	uint32_t data_size;
	uint32_t version;
	uint64_t memory_utilization_bin[MAX_NUMBER_MEM_BINS];
	int32_t max_memory_consumption;
	uint64_t active_time[FW_REPORTED_MAX_CORES_COUNT];
	uint64_t ulp_time;
};

void telemetry_init(void);
struct byte_array telemetry_get_buffer(void);
struct TelemetryWndData *telemetry_get_data(void);

#endif /* _TELEMETRY_INTEL_ */
