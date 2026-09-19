/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>

#include <zephyr/arch/pmu.h>
#if defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/pmuv3.h>
#endif
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_PROFILING_PMU_SAMPLING)
#include <zephyr/profiling/pmu_sampling.h>
#endif

struct perf_shell_ctx {
	bool session_ready;
	bool counting;
	pmu_evt_t counter0_event;
	/*
	 * Counter values saved at perf stop time on the CPU where counting ran.
	 * With LOG_MODE_IMMEDIATE + SHELL_LOG_BACKEND, pmu_init()'s LOG_INF
	 * causes a mutex acquire/release which can trigger a reschedule and
	 * migrate the shell thread to a different CPU between shell commands.
	 * Reading PMU registers on a different CPU gives 0.  Save at stop time
	 * so perf report always returns valid values regardless of migration.
	 */
	uint64_t saved_pmccntr;
	uint64_t saved_counter0;
};

static struct perf_shell_ctx perf_ctx = {
	.counter0_event = PMU_EVT_CPU_CYCLES,
};

#if defined(CONFIG_ARM64)
#define perf_event_name(evt) arch_pmu_event_name((uint32_t)(evt))
#else
static const char *perf_event_name(pmu_evt_t evt)
{
	ARG_UNUSED(evt);

	return "event";
}
#endif

static bool perf_name_match(const char *a, const char *b)
{
	size_t la = strlen(a);
	size_t lb = strlen(b);

	return (la == lb) && (strncasecmp(a, b, la) == 0);
}

static const struct {
	const char *name;
	pmu_evt_t code;
} perf_evt_aliases[] = {
	{ "cycles", PMU_EVT_CPU_CYCLES },
	{ "cpu_cycles", PMU_EVT_CPU_CYCLES },
	{ "instructions", PMU_EVT_INST_RETIRED },
	{ "inst_retired", PMU_EVT_INST_RETIRED },
	{ "branch-misses", PMU_EVT_BR_MIS_PRED },
	{ "br_mis_pred", PMU_EVT_BR_MIS_PRED },
	{ "branches", PMU_EVT_BR_PRED },
	{ "br_pred", PMU_EVT_BR_PRED },
	{ "mem_access", PMU_EVT_MEM_ACCESS },
	{ "mem-access", PMU_EVT_MEM_ACCESS },
	{ "bus_access", PMU_EVT_BUS_ACCESS },
	{ "bus-access", PMU_EVT_BUS_ACCESS },
	{ "bus_cycles", PMU_EVT_BUS_CYCLES },
	{ "l1d_cache_refill", PMU_EVT_L1D_CACHE_REFILL },
	{ "l1i_cache_refill", PMU_EVT_L1I_CACHE_REFILL },
	{ "l1d_cache", PMU_EVT_L1D_CACHE },
	{ "l1i_cache", PMU_EVT_L1I_CACHE },
	{ "l2d_cache", PMU_EVT_L2D_CACHE },
	{ "l2d_cache_refill", PMU_EVT_L2D_CACHE_REFILL },
	{ "sw_incr", PMU_EVT_SW_INCR },
	{ "exc_taken", PMU_EVT_EXC_TAKEN },
	{ "exc_return", PMU_EVT_EXC_RETURN },
	{ "inst_spec", PMU_EVT_INST_SPEC },
	{ "ttbr_write", PMU_EVT_TTBR_WRITE_RETIRED },
	{ "memory_error", PMU_EVT_MEMORY_ERROR },
	{ "l1d_tlb_refill", PMU_EVT_L1D_TLB_REFILL },
	{ "l1i_tlb_refill", PMU_EVT_L1I_TLB_REFILL },
	{ "l1d_cache_wb", PMU_EVT_L1D_CACHE_WB },
	{ "l2d_cache_wb", PMU_EVT_L2D_CACHE_WB },
};

static bool perf_event_is_help(const char *name)
{
	return strcasecmp(name, "help") == 0 || strcmp(name, "?") == 0;
}

static void perf_print_event_list(const struct shell *sh)
{
	shell_print(sh, "perf start -e <name>   (or raw code 0x00-0x1f, hardware may filter)");
	shell_print(sh, "code  event             -e names (synonyms on one line)");

	for (size_t i = 0; i < ARRAY_SIZE(perf_evt_aliases); i++) {
		uint32_t code = (uint32_t)perf_evt_aliases[i].code;
		char names[192];
		size_t pos = 0;
		bool skip;

		skip = false;
		for (size_t k = 0; k < i; k++) {
			if (perf_evt_aliases[k].code == perf_evt_aliases[i].code) {
				skip = true;
				break;
			}
		}
		if (skip) {
			continue;
		}

		names[0] = '\0';
		for (size_t j = 0; j < ARRAY_SIZE(perf_evt_aliases); j++) {
			int n;

			if (perf_evt_aliases[j].code != perf_evt_aliases[i].code) {
				continue;
			}
			n = snprintf(names + pos, sizeof(names) - pos, "%s%s",
				     pos > 0 ? ", " : "", perf_evt_aliases[j].name);
			if (n <= 0 || (size_t)n >= sizeof(names) - pos) {
				break;
			}
			pos += (size_t)n;
		}

		shell_print(sh, "0x%02x  %-18s  %s", code,
			    perf_event_name(perf_evt_aliases[i].code), names);
	}
}

static int cmd_perf_list(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	perf_print_event_list(sh);
	return 0;
}

static int cmd_perf_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if IS_ENABLED(CONFIG_PROFILING_PMU_SAMPLING)
	struct pmu_sampling_stats st;

	pmu_sampling_get_stats(&st);
	shell_print(sh, "PMU sampling: %s", pmu_sampling_is_active() ? "active" : "idle");
	shell_print(sh, " stored samples: %zu", st.stored_samples);
	shell_print(sh, " lost samples:   %zu", st.lost_samples);
	shell_print(sh, " ring fill (max cpu): %u%%", st.buffer_high_water_pct);
#else
	shell_print(sh, "PMU sampling: not built (PROFILING_PMU_SAMPLING=n)");
#endif
	shell_print(sh, "PMU counting session: %s", perf_ctx.counting ? "active" : "idle");
	return 0;
}

static int perf_parse_event(const char *name, pmu_evt_t *evt)
{
	int err = 0;
	unsigned long v;

	if (name[0] == '0' && name[1] == 'x') {
		v = shell_strtoul(name, 16, &err);
		if (err != 0) {
			return -EINVAL;
		}
		if (v > 0x1FU) {
			return -EINVAL;
		}
		*evt = (pmu_evt_t)v;
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(perf_evt_aliases); i++) {
		if (perf_name_match(name, perf_evt_aliases[i].name)) {
			*evt = perf_evt_aliases[i].code;
			return 0;
		}
	}

	return -EINVAL;
}

static int cmd_perf_start(const struct shell *sh, size_t argc, char **argv)
{
	pmu_evt_t evt = PMU_EVT_CPU_CYCLES;
	int ret;

#if IS_ENABLED(CONFIG_PROFILING_PMU_SAMPLING)
	pmu_sampling_stop();
#endif

	ret = pmu_init();
	if (ret != 0) {
		shell_error(sh, "PMU not available (%d)", ret);
		return ret;
	}

	if (pmu_num_counters() == 0U) {
		shell_error(sh, "No PMU event counters");
		return -ENOTSUP;
	}

	if (argc == 3) {
		if (strcmp(argv[1], "-e") != 0) {
			shell_error(sh, "Usage: perf start [-e <event>|help]");
			shell_print(sh, "Try: perf list");
			return -EINVAL;
		}
		if (perf_event_is_help(argv[2])) {
			perf_print_event_list(sh);
			return 0;
		}
		ret = perf_parse_event(argv[2], &evt);
		if (ret != 0) {
			shell_error(sh, "Unknown event: %s (try: perf list)", argv[2]);
			return -EINVAL;
		}
	} else if (argc != 1) {
		shell_error(sh, "Usage: perf start [-e <event>|help]");
		shell_print(sh, "Try: perf list");
		return -EINVAL;
	}

	pmu_stop();
	pmu_counter_disable_all();
	pmu_counter_reset_all();
	pmu_cycle_reset();

	ret = pmu_counter_config(0U, evt);
	if (ret != 0) {
		shell_error(sh, "pmu_counter_config failed (%d)", ret);
		return ret;
	}

	pmu_counter_enable(0U);
	pmu_start();

	perf_ctx.counter0_event = evt;
	perf_ctx.session_ready = true;
	perf_ctx.counting = true;

	shell_print(sh, "perf: counter0=%s (PMCCNTR + EVTEN0 running)",
		    perf_event_name(evt));
	return 0;
}

static int cmd_perf_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if IS_ENABLED(CONFIG_PROFILING_PMU_SAMPLING)
	pmu_sampling_stop();
#endif

	pmu_stop();
	/* Save counter values on this CPU before migration can occur. */
	perf_ctx.saved_pmccntr  = pmu_cycle_count();
	perf_ctx.saved_counter0 = pmu_counter_read(0U);
	perf_ctx.counting = false;
	shell_print(sh, "perf: stopped (counters frozen)");

	return 0;
}

static int cmd_perf_report(const struct shell *sh, size_t argc, char **argv)
{
	uint64_t cy;
	uint64_t c0;
	const char *evname;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!perf_ctx.session_ready) {
		shell_warn(sh, "perf: no session; run perf start first");
	}

	evname = perf_event_name(perf_ctx.counter0_event);
	/*
	 * Use values saved at perf stop — PMU registers are per-CPU so reading
	 * them here would give 0 if the shell thread migrated to another CPU.
	 */
	cy = perf_ctx.saved_pmccntr;
	c0 = perf_ctx.saved_counter0;

	shell_print(sh, "PMCCNTR_EL0:   %llu", (unsigned long long)cy);
	shell_print(sh, "counter0 (%s): %llu", evname, (unsigned long long)c0);

	if (perf_ctx.counting) {
		shell_print(sh, "(counting active; perf stop to freeze)");
	}

	return 0;
}

#if IS_ENABLED(CONFIG_PROFILING_PMU_SAMPLING)

static int cmd_perf_record_start(const struct shell *sh, size_t argc, char **argv)
{
	pmu_evt_t evt = PMU_EVT_CPU_CYCLES;
	uint32_t period_events = 0U;
	unsigned int freq_hz = 0U;
	int duration_s = -1;
	int err = 0;
	int ret;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-e") == 0) {
			if (i + 1 >= argc) {
				shell_error(sh, "-e needs an event name");
				return -EINVAL;
			}
			ret = perf_parse_event(argv[i + 1], &evt);
			if (ret != 0) {
				shell_error(sh, "Unknown event: %s", argv[i + 1]);
				return -EINVAL;
			}
			i += 1;
			continue;
		}
		if (strcmp(argv[i], "-f") == 0) {
			if (i + 1 >= argc) {
				shell_error(sh, "-f needs a sample rate in Hz");
				return -EINVAL;
			}
			freq_hz = (unsigned int)shell_strtoul(argv[i + 1], 10, &err);
			if (err != 0 || freq_hz == 0U) {
				shell_error(sh, "Invalid -f value");
				return -EINVAL;
			}
			i += 1;
			continue;
		}
		if (strcmp(argv[i], "-t") == 0) {
			if (i + 1 >= argc) {
				shell_error(sh, "-t needs duration in seconds");
				return -EINVAL;
			}
			duration_s = (int)shell_strtoul(argv[i + 1], 10, &err);
			if (err != 0 || duration_s <= 0) {
				shell_error(sh, "Invalid -t value (positive seconds)");
				return -EINVAL;
			}
			i += 1;
			continue;
		}
		if (strcmp(argv[i], "-o") == 0) {
			if (i + 1 >= argc) {
				shell_error(sh, "-o needs a path");
				return -EINVAL;
			}
			shell_warn(sh, "-o not supported; capture via 'perf record export'");
			i += 1;
			continue;
		}
		if (argv[i][0] == '-') {
			shell_error(sh, "Unknown option: %s", argv[i]);
			return -EINVAL;
		}

		period_events = (uint32_t)shell_strtoul(argv[i], 10, &err);
		if (err != 0 || period_events == 0U) {
			shell_error(sh, "Invalid period (PMU counts between samples)");
			return -EINVAL;
		}
	}

	if (freq_hz != 0U && period_events != 0U) {
		shell_error(sh, "Use either -f Hz or explicit period counts, not both");
		return -EINVAL;
	}

	if (freq_hz != 0U || period_events == 0U) {
		/*
		 * pmu_sampling_period_from_hz() needs arch_pmu_cpu_freq_mhz()
		 * which is per-CPU.  With LOG_MODE_IMMEDIATE the shell thread
		 * can migrate between perf start and perf record start, leaving
		 * cpu_freq_mhz=0 on the new CPU.  Initialize the current CPU
		 * first (no-op if already done) so the conversion always works.
		 */
		ret = pmu_init();
		if (ret != 0) {
			shell_error(sh, "pmu_init failed (%d)", ret);
			return ret;
		}
	}

	if (freq_hz == 0U && period_events == 0U) {
		ret = pmu_sampling_period_from_hz(1000U, &period_events);
		if (ret != 0) {
			shell_error(sh, "Default ~1 kHz sampling needs cpu_freq_mhz() (%d)",
				    ret);
			return ret;
		}
		shell_print(sh, "(default sample rate 1000 Hz -> period=%u counts)", period_events);
	} else if (freq_hz != 0U) {
		ret = pmu_sampling_period_from_hz(freq_hz, &period_events);
		if (ret != 0) {
			shell_error(sh, "pmu_sampling_period_from_hz failed (%d)", ret);
			return ret;
		}
	}

	perf_ctx.counting = false;
	perf_ctx.session_ready = false;

	{
		struct pmu_sampling_start_cfg cfg = {
			.event = (uint32_t)evt,
			.period_events = period_events,
			.auto_stop_s = duration_s,
		};

		ret = pmu_sampling_start_cfg(&cfg);
	}
	if (ret != 0) {
		shell_error(sh, "pmu_sampling_start_cfg failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "perf record: event=%s period=%u counts%s", perf_event_name(evt),
		    period_events, duration_s > 0 ? " (auto-stop scheduled)" : "");
	return 0;
}

static int cmd_perf_record_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	pmu_sampling_stop();
	shell_print(sh, "perf record: stopped");
	return 0;
}

static int cmd_perf_record_dump(const struct shell *sh, size_t argc, char **argv)
{
	struct pmu_sample_record tmp[32];

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	size_t n = pmu_sampling_copy(tmp, ARRAY_SIZE(tmp));
	size_t total = pmu_sampling_count();

	shell_print(sh, "samples: showing %zu of %zu stored", n, total);
	for (size_t i = 0; i < n; i++) {
		shell_print(sh, "  [%zu] cpu=%u tid=0x%08x ev=0x%04x pc=0x%016llx t=%llu", i,
			    tmp[i].cpu, tmp[i].tid, tmp[i].event,
			    (unsigned long long)tmp[i].pc, (unsigned long long)tmp[i].tstamp);
	}
	return 0;
}

static int cmd_perf_record_export(const struct shell *sh, size_t argc, char **argv)
{
	static uint8_t zperf_buf[CONFIG_PROFILING_PMU_ZPERF_MAX];
	int n;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	n = pmu_sampling_export_zperf(zperf_buf, sizeof(zperf_buf));
	if (n < 0) {
		shell_error(sh, "export failed (%d)", n);
		if (n == -ENOSPC) {
			shell_print(sh, "Increase CONFIG_PROFILING_PMU_ZPERF_MAX or reduce"
					" samples");
		}
		return n;
	}

	shell_print(sh, "zperf v1: %d bytes", n);
	shell_hexdump(sh, zperf_buf, (size_t)n);
	return 0;
}

static int cmd_perf_record_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	pmu_sampling_clear();
	shell_print(sh, "perf record: sample buffer cleared");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_perf_record,
	SHELL_CMD_ARG(start, NULL,
		      "start [-e ev] [-f Hz] | [period] [-t sec] (~1kHz default if no period/-f)",
		      cmd_perf_record_start, 1, 12),
	SHELL_CMD(stop, NULL, "Stop PMU sampling session", cmd_perf_record_stop),
	SHELL_CMD(dump, NULL, "Print recent PC samples", cmd_perf_record_dump),
	SHELL_CMD(export, NULL, "Hex-dump zperf v1 blob (host decode)", cmd_perf_record_export),
	SHELL_CMD(clear, NULL, "Clear sample ring buffer", cmd_perf_record_clear),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_PROFILING_PMU_SAMPLING */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_perf,
	SHELL_CMD_ARG(start, NULL,
		      "Start PMU: perf start [-e <ev>|help]; default -e cycles; see perf list",
		      cmd_perf_start, 1, 2),
	SHELL_CMD(list, NULL, "List names for perf start -e", cmd_perf_list),
	SHELL_CMD(status, NULL, "PMU / sampling status", cmd_perf_status),
	SHELL_CMD(stop, NULL, "Stop PMU (freeze counters)", cmd_perf_stop),
	SHELL_CMD(report, NULL, "Print PMCCNTR and programmable counter 0", cmd_perf_report),
#if IS_ENABLED(CONFIG_PROFILING_PMU_SAMPLING)
	SHELL_CMD(record, &sub_perf_record, "PMU PC sampling (overflow IRQ)", NULL),
#endif
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(perf, &sub_perf,
		   "PMU perf: start/stop/report/list/record (requires ARM64_PMUV3)", NULL);
