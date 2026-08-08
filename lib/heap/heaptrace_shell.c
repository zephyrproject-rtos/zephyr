/*
 *  Copyright (c) 2026 Picoheart Inc.
 *  Copyright (c) 2026 LiuQian.andy <liuqian.andy@picoheart.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/shell/shell.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/heaptrace.h>
#include <stdlib.h>
#include <string.h>

#define HEAPTRACE_SUMMARY_MAX_THREADS 32

struct heaptrace_dump_ctx {
	const struct shell *sh;
	enum heaptrace_filter_type filter_type;
	k_tid_t filter_tid;
	const char *filter_name;
	unsigned int filter_cpu;
	size_t total;
	size_t count;
};

struct heaptrace_summary_entry {
	k_tid_t tid;
	char name[HEAPTRACE_THREAD_NAME_LEN];
	size_t bytes;
	size_t blocks;
	bool used;
};

struct heaptrace_summary_ctx {
	struct heaptrace_summary_entry entries[HEAPTRACE_SUMMARY_MAX_THREADS];
};

static const char *heaptrace_shell_heap_name(uintptr_t heap_id)
{
	if (heap_id == HEAP_ID_LIBC) {
		return "libc";
	}
	return "sys_heap";
}

static void heaptrace_shell_print_record(const struct shell *sh,
					 const struct heaptrace_record_info *info)
{
	shell_print(sh,
		    "[heaptrace] ptr=%p size=%zu heap=%s alloc_tid=%p "
		    "alloc_thread=%s alloc_cpu=%u alloc_ms=%u",
		    info->ptr, info->size, heaptrace_shell_heap_name(info->heap_id),
		    info->alloc_tid, info->alloc_thread_name, info->alloc_cpu, info->alloc_time_ms);

#if defined(CONFIG_HEAPTRACE_BACKTRACE)
	if (info->bt_depth > 0 && info->bt_buf != NULL) {
		shell_fprintf(sh, SHELL_NORMAL, "[heaptrace]   backtrace:");
		for (int j = 0; j < info->bt_depth; j++) {
			shell_fprintf(sh, SHELL_NORMAL, " %p", (void *)info->bt_buf[j]);
		}
		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}
#endif
}

static bool heaptrace_dump_cb(const struct heaptrace_record_info *info, void *user_data)
{
	struct heaptrace_dump_ctx *ctx = user_data;

	switch (ctx->filter_type) {
	case HEAPTRACE_FILTER_TID:
		if (info->alloc_tid != ctx->filter_tid) {
			return true;
		}
		break;
	case HEAPTRACE_FILTER_NAME:
		if (strstr(info->alloc_thread_name, ctx->filter_name) == NULL) {
			return true;
		}
		break;
	case HEAPTRACE_FILTER_CPU:
		if (info->alloc_cpu != ctx->filter_cpu) {
			return true;
		}
		break;
	default:
		break;
	}

	heaptrace_shell_print_record(ctx->sh, info);

	ctx->total += info->size;
	ctx->count++;
	return true;
}

static void heaptrace_shell_dump(const struct shell *sh, enum heaptrace_filter_type type,
				 k_tid_t tid, const char *name, unsigned int cpu)
{
	struct heaptrace_dump_ctx ctx = {
		.sh = sh,
		.filter_type = type,
		.filter_tid = tid,
		.filter_name = name,
		.filter_cpu = cpu,
	};

	switch (type) {
	case HEAPTRACE_FILTER_TID:
		shell_print(sh,
			    "=============== heaptrace dump by tid %p begin ===============", tid);
		break;
	case HEAPTRACE_FILTER_NAME:
		shell_print(sh,
			    "=============== heaptrace dump by name \"%s\" begin ===============",
			    name);
		break;
	case HEAPTRACE_FILTER_CPU:
		shell_print(sh,
			    "=============== heaptrace dump by cpu %u begin ===============", cpu);
		break;
	default:
		shell_print(sh, "=============== heaptrace dump begin ===============");
		break;
	}

	heaptrace_foreach_record(heaptrace_dump_cb, &ctx);

	switch (type) {
	case HEAPTRACE_FILTER_TID:
		shell_print(sh, "[heaptrace] tid=%p outstanding_blocks=%zu outstanding_bytes=%zu",
			    tid, ctx.count, ctx.total);
		shell_print(sh,
			    "=============== heaptrace dump by tid %p end =================", tid);
		break;
	case HEAPTRACE_FILTER_NAME:
		shell_print(sh,
			    "[heaptrace] name=\"%s\" outstanding_blocks=%zu outstanding_bytes=%zu",
			    name, ctx.count, ctx.total);
		shell_print(sh,
			    "=============== heaptrace dump by name \"%s\" end =================",
			    name);
		break;
	case HEAPTRACE_FILTER_CPU:
		shell_print(sh, "[heaptrace] cpu=%u outstanding_blocks=%zu outstanding_bytes=%zu",
			    cpu, ctx.count, ctx.total);
		shell_print(sh,
			    "=============== heaptrace dump by cpu %u end =================", cpu);
		break;
	default:
		shell_print(sh, "[heaptrace] outstanding_blocks=%zu outstanding_bytes=%zu",
			    ctx.count, ctx.total);
		shell_print(sh, "=============== heaptrace dump end =================");
		break;
	}
}

static bool heaptrace_summary_cb(const struct heaptrace_record_info *info, void *user_data)
{
	struct heaptrace_summary_ctx *ctx = user_data;
	int pos = -1;

	for (int j = 0; j < HEAPTRACE_SUMMARY_MAX_THREADS; j++) {
		if (ctx->entries[j].used && ctx->entries[j].tid == info->alloc_tid) {
			pos = j;
			break;
		}
	}

	if (pos < 0) {
		for (int j = 0; j < HEAPTRACE_SUMMARY_MAX_THREADS; j++) {
			if (!ctx->entries[j].used) {
				pos = j;
				ctx->entries[j].used = true;
				ctx->entries[j].tid = info->alloc_tid;
				strncpy(ctx->entries[j].name, info->alloc_thread_name,
					sizeof(ctx->entries[j].name) - 1);
				ctx->entries[j].name[sizeof(ctx->entries[j].name) - 1] = '\0';
				break;
			}
		}
	}

	if (pos >= 0) {
		ctx->entries[pos].bytes += info->size;
		ctx->entries[pos].blocks++;
	}

	return true;
}

static int cmd_heaptrace_dump(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	heaptrace_shell_dump(sh, HEAPTRACE_FILTER_NONE, NULL, NULL, 0);
	return 0;
}

static int cmd_heaptrace_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	heaptrace_reset();
	shell_print(sh, "[heaptrace] records reset");
	return 0;
}

static int cmd_heaptrace_summary(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct heaptrace_summary_ctx ctx;

	memset(&ctx, 0, sizeof(ctx));
	heaptrace_foreach_record(heaptrace_summary_cb, &ctx);

	shell_print(sh, "=============== heaptrace summary by thread ===============");
	for (int i = 0; i < HEAPTRACE_SUMMARY_MAX_THREADS; i++) {
		if (!ctx.entries[i].used) {
			continue;
		}
		shell_print(sh, "[heaptrace-summary] thread=%s tid=%p blocks=%zu bytes=%zu",
			    ctx.entries[i].name, ctx.entries[i].tid, ctx.entries[i].blocks,
			    ctx.entries[i].bytes);
	}
	shell_print(sh, "======================================================");

	return 0;
}

static int cmd_heaptrace_dump_tid(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long tid_val;
	k_tid_t tid;

	if (argc < 2) {
		shell_error(sh, "usage: heaptrace dump tid <tid_hex>");
		return -EINVAL;
	}

	tid_val = strtoul(argv[1], NULL, 16);
	tid = (k_tid_t)tid_val;

	heaptrace_shell_dump(sh, HEAPTRACE_FILTER_TID, tid, NULL, 0);
	return 0;
}

static int cmd_heaptrace_dump_name(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "usage: heaptrace dump name <thread_name>");
		return -EINVAL;
	}

	heaptrace_shell_dump(sh, HEAPTRACE_FILTER_NAME, NULL, argv[1], 0);
	return 0;
}

static int cmd_heaptrace_dump_cpu(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long cpu_id;

	if (argc < 2) {
		shell_error(sh, "usage: heaptrace dump cpu <cpu_id>");
		return -EINVAL;
	}

	cpu_id = strtoul(argv[1], NULL, 10);
	heaptrace_shell_dump(sh, HEAPTRACE_FILTER_CPU, NULL, NULL, (unsigned int)cpu_id);
	return 0;
}

static int cmd_heaptrace_lookup(const struct shell *sh, size_t argc, char **argv)
{
	k_tid_t tid;

	if (argc < 2) {
		shell_error(sh, "usage: heaptrace lookup <thread_name>");
		return -EINVAL;
	}

	tid = heaptrace_find_tid_by_name(argv[1]);
	if (tid != NULL) {
		shell_print(sh, "thread \"%s\" -> tid=%p", argv[1], tid);
	} else {
		shell_warn(sh, "thread \"%s\" not found", argv[1]);
	}

	return 0;
}

static int cmd_heaptrace_filter_tid(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long tid_val;
	k_tid_t tid;

	if (argc < 2) {
		shell_error(sh, "usage: heaptrace filter tid <tid_hex>");
		return -EINVAL;
	}

	tid_val = strtoul(argv[1], NULL, 16);
	tid = (k_tid_t)tid_val;

	heaptrace_set_filter_tid(tid);
	shell_print(sh, "[heaptrace] filter set: tid=%p", tid);
	return 0;
}

static int cmd_heaptrace_filter_name(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "usage: heaptrace filter name <thread_name>");
		return -EINVAL;
	}

	heaptrace_set_filter_name(argv[1]);
	shell_print(sh, "[heaptrace] filter set: name=\"%s\"", argv[1]);
	return 0;
}

static int cmd_heaptrace_filter_cpu(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long cpu_id;

	if (argc < 2) {
		shell_error(sh, "usage: heaptrace filter cpu <cpu_id>");
		return -EINVAL;
	}

	cpu_id = strtoul(argv[1], NULL, 10);
	heaptrace_set_filter_cpu((unsigned int)cpu_id);
	shell_print(sh, "[heaptrace] filter set: cpu=%u", (unsigned int)cpu_id);
	return 0;
}

static int cmd_heaptrace_filter_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	heaptrace_clear_filter();
	shell_print(sh, "[heaptrace] filter cleared");
	return 0;
}

static int cmd_heaptrace_filter_show(const struct shell *sh, size_t argc, char **argv)
{
	char buf[64];

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	heaptrace_filter_str(buf, sizeof(buf));
	shell_print(sh, "current filter: %s", buf);
	return 0;
}

struct heaptrace_heapinfo_ctx {
	const struct shell *sh;
	uintptr_t heap_id;
	size_t outstanding_bytes;
	size_t outstanding_blocks;
};

static bool heaptrace_heapinfo_accum_cb(const struct heaptrace_record_info *info, void *user_data)
{
	struct heaptrace_heapinfo_ctx *ctx = user_data;

	if (info->heap_id == ctx->heap_id) {
		ctx->outstanding_bytes += info->size;
		ctx->outstanding_blocks++;
	}
	return true;
}

static bool heaptrace_heapinfo_cb(const struct heaptrace_heap_info *info, void *user_data)
{
	const struct shell *sh = user_data;
	struct heaptrace_heapinfo_ctx accum = {
		.sh = sh,
		.heap_id = info->heap_id,
	};
	intptr_t growth;

	heaptrace_foreach_record(heaptrace_heapinfo_accum_cb, &accum);

	growth = (char *)info->heap_end - (char *)info->heap_start;
	shell_print(sh,
		    "[heaptrace] heap=%s heap_id=0x%lx start=%p end=%p growth=%+ld "
		    "outstanding_blocks=%zu outstanding_bytes=%zu",
		    heaptrace_shell_heap_name(info->heap_id), (unsigned long)info->heap_id,
		    info->heap_start, info->heap_end, (long)growth, accum.outstanding_blocks,
		    accum.outstanding_bytes);
	return true;
}

static int cmd_heaptrace_heapinfo(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "=============== heaptrace heapinfo ===============");
	heaptrace_foreach_heap(heaptrace_heapinfo_cb, (void *)sh);
	shell_print(sh, "==================================================");
	return 0;
}

static bool heaptrace_resize_dump_cb(const struct heaptrace_resize_info *info, void *user_data)
{
	const struct shell *sh = user_data;
	intptr_t delta = (char *)info->new_end - (char *)info->old_end;

	shell_print(sh,
		    "[heaptrace-resize] heap=%s old_end=%p new_end=%p delta=%+ld "
		    "tid=%p thread=%s cpu=%u ms=%u",
		    heaptrace_shell_heap_name(info->heap_id), info->old_end, info->new_end,
		    (long)delta, info->tid, info->thread_name, info->cpu, info->time_ms);
	return true;
}

static int cmd_heaptrace_resize(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "=============== heaptrace resize history ===============");
	heaptrace_foreach_resize(heaptrace_resize_dump_cb, (void *)sh);
	shell_print(sh, "=========================================================");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_heaptrace_dump,
	SHELL_CMD(all, NULL, "Dump all outstanding allocations", cmd_heaptrace_dump),
	SHELL_CMD(tid, NULL, "Dump by thread id (hex)", cmd_heaptrace_dump_tid),
	SHELL_CMD(name, NULL, "Dump by thread name (substring)", cmd_heaptrace_dump_name),
	SHELL_CMD(cpu, NULL, "Dump by cpu id (decimal)", cmd_heaptrace_dump_cpu),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_heaptrace_filter,
	SHELL_CMD(tid, NULL, "Set filter by thread id (hex)", cmd_heaptrace_filter_tid),
	SHELL_CMD(name, NULL, "Set filter by thread name (substring)", cmd_heaptrace_filter_name),
	SHELL_CMD(cpu, NULL, "Set filter by cpu id (decimal)", cmd_heaptrace_filter_cpu),
	SHELL_CMD(clear, NULL, "Clear filter (record all events)", cmd_heaptrace_filter_clear),
	SHELL_CMD(show, NULL, "Show current filter", cmd_heaptrace_filter_show),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_heaptrace, SHELL_CMD(dump, &sub_heaptrace_dump, "Dump outstanding allocations", NULL),
	SHELL_CMD(filter, &sub_heaptrace_filter, "Set/clear event filter", NULL),
	SHELL_CMD(reset, NULL, "Reset heaptrace records", cmd_heaptrace_reset),
	SHELL_CMD(summary, NULL, "Dump summary by thread", cmd_heaptrace_summary),
	SHELL_CMD(lookup, NULL, "Lookup thread tid by name", cmd_heaptrace_lookup),
	SHELL_CMD(heapinfo, NULL, "Show heap boundaries and utilization", cmd_heaptrace_heapinfo),
	SHELL_CMD(resize, NULL, "Show heap resize history", cmd_heaptrace_resize),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(heaptrace, &sub_heaptrace, "Heap trace commands", NULL);
