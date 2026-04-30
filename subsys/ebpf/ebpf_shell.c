/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/iterable_sections.h>
#include <stdlib.h>

#include <zephyr/ebpf/ebpf.h>

static const char *ebpf_prog_type_str(enum ebpf_prog_type type)
{
	switch (type) {
	case EBPF_PROG_TYPE_TRACEPOINT: return "tracepoint";
	case EBPF_PROG_TYPE_SCHED:      return "sched";
	case EBPF_PROG_TYPE_SYSCALL:    return "syscall";
	case EBPF_PROG_TYPE_ISR:        return "isr";
	default:                        return "unknown";
	}
}

static const char *ebpf_prog_state_str(enum ebpf_prog_state state)
{
	switch (state) {
	case EBPF_PROG_STATE_DISABLED: return "disabled";
	case EBPF_PROG_STATE_VERIFIED: return "verified";
	case EBPF_PROG_STATE_ENABLED:  return "enabled";
	default:                       return "unknown";
	}
}

static const char *ebpf_map_type_str(enum ebpf_map_type type)
{
	switch (type) {
	case EBPF_MAP_TYPE_ARRAY:         return "array";
	case EBPF_MAP_TYPE_HASH:          return "hash";
	case EBPF_MAP_TYPE_RINGBUF:       return "ringbuf";
	case EBPF_MAP_TYPE_PER_CPU_ARRAY: return "percpu";
	default:                          return "unknown";
	}
}

/* ebpf list */
static int cmd_ebpf_list(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%-20s %-12s %-10s %-6s %-10s",
		    "Name", "Type", "State", "TP", "Runs");
	shell_print(sh, "%-20s %-12s %-10s %-6s %-10s",
		    "----", "----", "-----", "--", "----");

	STRUCT_SECTION_FOREACH(ebpf_prog, prog) {
		shell_print(sh, "%-20s %-12s %-10s %-6d %-10u",
			    prog->name,
			    ebpf_prog_type_str(prog->type),
			    ebpf_prog_state_str(prog->state),
			    prog->attach_point,
			    prog->run_count);
	}

	return 0;
}

/* ebpf enable <name> [tp_id] */
static int cmd_ebpf_enable(const struct shell *sh, size_t argc, char **argv)
{
	struct ebpf_prog *prog = ebpf_prog_find(argv[1]);

	if (!prog) {
		shell_error(sh, "Program '%s' not found", argv[1]);

		return -ENOENT;
	}

	if (argc > 2) {
		int tp_id = atoi(argv[2]);
		int ret = ebpf_prog_attach(prog, tp_id);

		if (ret) {
			shell_error(sh, "Attach failed: %d", ret);

			return ret;
		}
	}

	int ret = ebpf_prog_enable(prog);

	if (ret) {
		shell_error(sh, "Enable failed: %d", ret);

		return ret;
	}

	shell_print(sh, "Program '%s' enabled", prog->name);

	return 0;
}

/* ebpf disable <name> */
static int cmd_ebpf_disable(const struct shell *sh, size_t argc, char **argv)
{
	struct ebpf_prog *prog = ebpf_prog_find(argv[1]);

	if (!prog) {
		shell_error(sh, "Program '%s' not found", argv[1]);

		return -ENOENT;
	}

	ebpf_prog_disable(prog);
	shell_print(sh, "Program '%s' disabled", prog->name);

	return 0;
}

/* ebpf stats <name> */
static int cmd_ebpf_stats(const struct shell *sh, size_t argc, char **argv)
{
	struct ebpf_prog *prog = ebpf_prog_find(argv[1]);

	if (!prog) {
		shell_error(sh, "Program '%s' not found", argv[1]);

		return -ENOENT;
	}

	shell_print(sh, "Program: %s", prog->name);
	shell_print(sh, "  Type:      %s", ebpf_prog_type_str(prog->type));
	shell_print(sh, "  State:     %s", ebpf_prog_state_str(prog->state));
	shell_print(sh, "  TP:        %d", prog->attach_point);
	shell_print(sh, "  Insns:     %u", prog->insn_cnt);
	shell_print(sh, "  Runs:      %u", prog->run_count);
	shell_print(sh, "  Time(ns):  %llu", prog->run_time_ns);

	if (prog->run_count > 0) {
		shell_print(sh, "  Avg(ns):   %llu", prog->run_time_ns / prog->run_count);
	}

	return 0;
}

/* ebpf maps */
static int cmd_ebpf_maps(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "%-20s %-8s %-6s %-6s %-6s",
		    "Name", "Type", "KeySz", "ValSz", "MaxEnt");
	shell_print(sh, "%-20s %-8s %-6s %-6s %-6s",
		    "----", "----", "-----", "-----", "------");

	STRUCT_SECTION_FOREACH(ebpf_map, map) {
		shell_print(sh, "%-20s %-8s %-6u %-6u %-6u",
			    map->name,
			    ebpf_map_type_str(map->type),
			    map->key_size,
			    map->value_size,
			    map->max_entries);
	}

	return 0;
}

/* ebpf dump <map_name> */
static int cmd_ebpf_dump(const struct shell *sh, size_t argc, char **argv)
{
	struct ebpf_map *map = ebpf_map_find(argv[1]);

	if (!map) {
		shell_error(sh, "Map '%s' not found", argv[1]);

		return -ENOENT;
	}

	if (map->type == EBPF_MAP_TYPE_ARRAY) {
		for (uint32_t i = 0; i < map->max_entries; i++) {
			void *val = ebpf_map_lookup_elem(map, &i);

			if (val) {
				shell_print(sh, "[%u]:", i);
				shell_hexdump(sh, val, map->value_size);
			}
		}
	} else {
		shell_print(sh, "Dump not yet supported for %s maps", ebpf_map_type_str(map->type));
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ebpf,
	SHELL_CMD(list, NULL, "List all eBPF programs", cmd_ebpf_list),
	SHELL_CMD_ARG(enable, NULL, "Enable program: ebpf enable <name> [tp_id]",
		      cmd_ebpf_enable, 2, 1),
	SHELL_CMD_ARG(disable, NULL, "Disable program: ebpf disable <name>",
		      cmd_ebpf_disable, 2, 0),
	SHELL_CMD_ARG(stats, NULL, "Show program stats: ebpf stats <name>",
		      cmd_ebpf_stats, 2, 0),
	SHELL_CMD(maps, NULL, "List all eBPF maps", cmd_ebpf_maps),
	SHELL_CMD_ARG(dump, NULL, "Dump map: ebpf dump <map_name>",
		      cmd_ebpf_dump, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ebpf, &sub_ebpf, "eBPF subsystem commands", NULL);
