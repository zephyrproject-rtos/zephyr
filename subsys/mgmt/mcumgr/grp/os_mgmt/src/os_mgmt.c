/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/object_tracing.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/logging/log.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>

#ifdef CONFIG_REBOOT
#include <zephyr/sys/reboot.h>
#endif

#ifdef CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#endif

#if defined(CONFIG_MCUMGR_GRP_OS_INFO) || defined(CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO)
#include <stdio.h>
#include <version.h>
#if defined(CONFIG_MCUMGR_GRP_OS_INFO)
#include <os_mgmt_processor.h>
#endif
#if defined(CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO)
#include <bootutil/boot_status.h>
#endif
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
#include <zephyr/net/hostname.h>
#elif defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#endif
#endif

LOG_MODULE_REGISTER(mcumgr_os_grp, CONFIG_MCUMGR_GRP_OS_LOG_LEVEL);

#ifdef CONFIG_REBOOT
static void os_mgmt_reset_work_handler(struct k_work *work);
static void os_mgmt_reset_cb(struct k_timer *timer);

K_WORK_DEFINE(os_mgmt_reset_work, os_mgmt_reset_work_handler);
static K_TIMER_DEFINE(os_mgmt_reset_timer, os_mgmt_reset_cb, NULL);
#endif

/* This is passed to zcbor_map_start/end_endcode as a number of
 * expected "columns" (tid, priority, and so on)
 * The value here does not affect memory allocation is is used
 * to predict how big the map may be. If you increase number
 * of "columns" the taskstat sends you may need to increase the
 * value otherwise zcbor_map_end_encode may return with error.
 */
#define TASKSTAT_COLUMNS_MAX	20

#ifdef CONFIG_MCUMGR_GRP_OS_TASKSTAT
/* Thread iterator information passing structure */
struct thread_iterator_info {
	zcbor_state_t *zse;
	int thread_idx;
	bool ok;
};
#endif

/* Specifies what the "all" ('a') of info parameter shows */
#define OS_MGMT_INFO_FORMAT_ALL                                                               \
	OS_MGMT_INFO_FORMAT_KERNEL_NAME | OS_MGMT_INFO_FORMAT_NODE_NAME |                     \
		OS_MGMT_INFO_FORMAT_KERNEL_RELEASE | OS_MGMT_INFO_FORMAT_KERNEL_VERSION |     \
		(IS_ENABLED(CONFIG_MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME) ?                      \
			OS_MGMT_INFO_FORMAT_BUILD_DATE_TIME : 0) |                            \
		OS_MGMT_INFO_FORMAT_MACHINE | OS_MGMT_INFO_FORMAT_PROCESSOR |                 \
		OS_MGMT_INFO_FORMAT_HARDWARE_PLATFORM | OS_MGMT_INFO_FORMAT_OPERATING_SYSTEM

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME
extern uint8_t *MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME;
#endif

/**
 * Command handler: os echo
 */
#ifdef CONFIG_MCUMGR_GRP_OS_ECHO
static int os_mgmt_echo(struct smp_streamer *ctxt)
{
	bool ok;
	zcbor_state_t *zsd = ctxt->reader->zs;
	zcbor_state_t *zse = ctxt->writer->zs;
	struct zcbor_string data = { 0 };
	size_t decoded;

	struct zcbor_map_decode_key_val echo_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("d", zcbor_tstr_decode, &data),
	};

	ok = zcbor_map_decode_bulk(zsd, echo_decode, ARRAY_SIZE(echo_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	ok = zcbor_tstr_put_lit(zse, "r")	&&
	     zcbor_tstr_encode(zse, &data);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}
#endif

#ifdef CONFIG_MCUMGR_GRP_OS_TASKSTAT

#ifdef CONFIG_MCUMGR_GRP_OS_TASKSTAT_USE_THREAD_NAME_FOR_NAME
static inline bool
os_mgmt_taskstat_encode_thread_name(zcbor_state_t *zse, int idx,
				    const struct k_thread *thread)
{
	size_t name_len = strlen(thread->name);

	ARG_UNUSED(idx);

	if (name_len > CONFIG_MCUMGR_GRP_OS_TASKSTAT_THREAD_NAME_LEN) {
		name_len = CONFIG_MCUMGR_GRP_OS_TASKSTAT_THREAD_NAME_LEN;
	}

	return zcbor_tstr_encode_ptr(zse, thread->name, name_len);
}

#else
static inline bool
os_mgmt_taskstat_encode_thread_name(zcbor_state_t *zse, int idx,
				    const struct k_thread *thread)
{
	char thread_name[CONFIG_MCUMGR_GRP_OS_TASKSTAT_THREAD_NAME_LEN + 1];

#if defined(CONFIG_MCUMGR_GRP_OS_TASKSTAT_USE_THREAD_PRIO_FOR_NAME)
	idx = (int)thread->base.prio;
#elif defined(CONFIG_MCUMGR_GRP_OS_TASKSTAT_USE_THREAD_IDX_FOR_NAME)
	ARG_UNUSED(thread);
#else
#error Unsupported option for taskstat thread name
#endif

	snprintf(thread_name, sizeof(thread_name) - 1, "%d", idx);
	thread_name[sizeof(thread_name) - 1] = 0;

	return zcbor_tstr_put_term(zse, thread_name);
}

#endif

static inline bool
os_mgmt_taskstat_encode_stack_info(zcbor_state_t *zse,
				   const struct k_thread *thread)
{
#ifdef CONFIG_MCUMGR_GRP_OS_TASKSTAT_STACK_INFO
	size_t stack_size = 0;
	size_t stack_used = 0;
	bool ok = true;

#ifdef CONFIG_THREAD_STACK_INFO
	stack_size = thread->stack_info.size / 4;

#ifdef CONFIG_INIT_STACKS
	unsigned int stack_unused;

	if (k_thread_stack_space_get(thread, &stack_unused) == 0) {
		stack_used = (thread->stack_info.size - stack_unused) / 4;
	}
#endif /* CONFIG_INIT_STACKS */
#endif /* CONFIG_THREAD_STACK_INFO */
	ok = zcbor_tstr_put_lit(zse, "stksiz")		&&
	     zcbor_uint64_put(zse, stack_size)		&&
	     zcbor_tstr_put_lit(zse, "stkuse")		&&
	     zcbor_uint64_put(zse, stack_used);

	return ok;
#else
	return true;
#endif /* CONFIG_MCUMGR_GRP_OS_TASKSTAT_STACK_INFO */
}

static inline bool
os_mgmt_taskstat_encode_runtime_info(zcbor_state_t *zse,
				     const struct k_thread *thread)
{
	bool ok = true;

#if defined(CONFIG_SCHED_THREAD_USAGE)
	k_thread_runtime_stats_t thread_stats;

	k_thread_runtime_stats_get((struct k_thread *)thread, &thread_stats);

	ok = zcbor_tstr_put_lit(zse, "runtime") &&
	zcbor_uint64_put(zse, thread_stats.execution_cycles);
#elif !defined(CONFIG_MCUMGR_GRP_OS_TASKSTAT_ONLY_SUPPORTED_STATS)
	ok = zcbor_tstr_put_lit(zse, "runtime") &&
	zcbor_uint32_put(zse, 0);
#endif

	return ok;
}

static inline bool os_mgmt_taskstat_encode_unsupported(zcbor_state_t *zse)
{
	bool ok = true;

	if (!IS_ENABLED(CONFIG_MCUMGR_GRP_OS_TASKSTAT_ONLY_SUPPORTED_STATS)) {
		ok = zcbor_tstr_put_lit(zse, "cswcnt")		&&
		     zcbor_uint32_put(zse, 0)			&&
		     zcbor_tstr_put_lit(zse, "last_checkin")	&&
		     zcbor_uint32_put(zse, 0)			&&
		     zcbor_tstr_put_lit(zse, "next_checkin")	&&
		     zcbor_uint32_put(zse, 0);
	} else {
		ARG_UNUSED(zse);
	}

	return ok;
}

static inline bool
os_mgmt_taskstat_encode_priority(zcbor_state_t *zse, const struct k_thread *thread)
{
	return (zcbor_tstr_put_lit(zse, "prio")					&&
		IS_ENABLED(CONFIG_MCUMGR_GRP_OS_TASKSTAT_SIGNED_PRIORITY) ?
		zcbor_int32_put(zse, (int)thread->base.prio) :
		zcbor_uint32_put(zse, (unsigned int)thread->base.prio) & 0xff);
}

/**
 * Encodes a single taskstat entry.
 */
static void os_mgmt_taskstat_encode_one(const struct k_thread *thread, void *user_data)
{
	/*
	 * Threads are sent as map where thread name is key and value is map
	 * of thread parameters
	 */
	struct thread_iterator_info *iterator_ctx = (struct thread_iterator_info *)user_data;

	if (iterator_ctx->ok == true) {
		iterator_ctx->ok =
			os_mgmt_taskstat_encode_thread_name(iterator_ctx->zse,
							    iterator_ctx->thread_idx, thread)	&&
			zcbor_map_start_encode(iterator_ctx->zse, TASKSTAT_COLUMNS_MAX)		&&
			os_mgmt_taskstat_encode_priority(iterator_ctx->zse, thread)		&&
			zcbor_tstr_put_lit(iterator_ctx->zse, "tid")				&&
			zcbor_uint32_put(iterator_ctx->zse, iterator_ctx->thread_idx)		&&
			zcbor_tstr_put_lit(iterator_ctx->zse, "state")				&&
			zcbor_uint32_put(iterator_ctx->zse, thread->base.thread_state)		&&
			os_mgmt_taskstat_encode_stack_info(iterator_ctx->zse, thread)		&&
			os_mgmt_taskstat_encode_runtime_info(iterator_ctx->zse, thread)		&&
			os_mgmt_taskstat_encode_unsupported(iterator_ctx->zse)			&&
			zcbor_map_end_encode(iterator_ctx->zse, TASKSTAT_COLUMNS_MAX);

		++iterator_ctx->thread_idx;
	}
}

/**
 * Command handler: os taskstat
 */
static int os_mgmt_taskstat_read(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	struct thread_iterator_info iterator_ctx = {
		.zse = zse,
		.thread_idx = 0,
		.ok = true,
	};

	zcbor_tstr_put_lit(zse, "tasks");
	zcbor_map_start_encode(zse, CONFIG_MCUMGR_GRP_OS_TASKSTAT_MAX_NUM_THREADS);

	/* Iterate the list of tasks, encoding each. */
	k_thread_foreach(os_mgmt_taskstat_encode_one, (void *)&iterator_ctx);

	if (!iterator_ctx.ok) {
		LOG_ERR("Task iterator status is not OK");
	}

	if (!iterator_ctx.ok ||
	    !zcbor_map_end_encode(zse, CONFIG_MCUMGR_GRP_OS_TASKSTAT_MAX_NUM_THREADS)) {
		return MGMT_ERR_EMSGSIZE;
	}

	return 0;
}
#endif /* CONFIG_MCUMGR_GRP_OS_TASKSTAT */

#ifdef CONFIG_REBOOT
/**
 * Command handler: os reset
 */
static void os_mgmt_reset_work_handler(struct k_work *work)
{
	sys_reboot(SYS_REBOOT_WARM);
}

static void os_mgmt_reset_cb(struct k_timer *timer)
{
	/* Reboot the system from the system workqueue thread. */
	k_work_submit(&os_mgmt_reset_work);
}

static int os_mgmt_reset(struct smp_streamer *ctxt)
{
#if defined(CONFIG_MCUMGR_GRP_OS_RESET_HOOK)
	zcbor_state_t *zsd = ctxt->reader->zs;
	zcbor_state_t *zse = ctxt->writer->zs;
	size_t decoded;
	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;

	struct os_mgmt_reset_data reboot_data = {
		.force = false
	};

	struct zcbor_map_decode_key_val reset_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("force", zcbor_bool_decode, &reboot_data.force),
	};

	/* Since this is a core command, if we fail to decode the data, ignore the error and
	 * continue with the default parameter of force being false.
	 */
	(void)zcbor_map_decode_bulk(zsd, reset_decode, ARRAY_SIZE(reset_decode), &decoded);
	status = mgmt_callback_notify(MGMT_EVT_OP_OS_MGMT_RESET, &reboot_data,
				      sizeof(reboot_data), &err_rc, &err_group);

	if (status != MGMT_CB_OK) {
		bool ok;

		if (status == MGMT_CB_ERROR_RC) {
			return err_rc;
		}

		ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
		return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
	}
#endif

	k_timer_start(&os_mgmt_reset_timer, K_MSEC(CONFIG_MCUMGR_GRP_OS_RESET_MS),
		      K_NO_WAIT);
	return 0;
}
#endif

#ifdef CONFIG_MCUMGR_GRP_OS_MCUMGR_PARAMS
static int
os_mgmt_mcumgr_params(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "buf_size")		&&
	     zcbor_uint32_put(zse, CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE)	&&
	     zcbor_tstr_put_lit(zse, "buf_count")		&&
	     zcbor_uint32_put(zse, CONFIG_MCUMGR_TRANSPORT_NETBUF_COUNT);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}
#endif

#if defined(CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO)

#if IS_ENABLED(CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP)
#define BOOTLOADER_MODE MCUBOOT_MODE_SINGLE_SLOT
#elif IS_ENABLED(CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH)
#define BOOTLOADER_MODE MCUBOOT_MODE_SWAP_USING_SCRATCH
#elif IS_ENABLED(CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY)
#define BOOTLOADER_MODE MCUBOOT_MODE_UPGRADE_ONLY
#elif IS_ENABLED(CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_WITHOUT_SCRATCH)
#define BOOTLOADER_MODE MCUBOOT_MODE_SWAP_USING_MOVE
#elif IS_ENABLED(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP)
#define BOOTLOADER_MODE MCUBOOT_MODE_DIRECT_XIP
#elif IS_ENABLED(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
#define BOOTLOADER_MODE MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT
#else
#define BOOTLOADER_MODE -1
#endif

static int
os_mgmt_bootloader_info(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	struct zcbor_string query = { 0 };
	size_t decoded;
	bool ok;

	struct zcbor_map_decode_key_val bootloader_info[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("query", zcbor_tstr_decode, &query),
	};

	if (zcbor_map_decode_bulk(zsd, bootloader_info, ARRAY_SIZE(bootloader_info), &decoded)) {
		return MGMT_ERR_EINVAL;
	}

	/* If no parameter is recognized then just introduce the bootloader. */
	if (decoded == 0) {
		ok = zcbor_tstr_put_lit(zse, "bootloader") &&
		     zcbor_tstr_put_lit(zse, "MCUboot");
	} else if (zcbor_map_decode_bulk_key_found(bootloader_info, ARRAY_SIZE(bootloader_info),
		   "query") &&
		   (sizeof("mode") - 1) == query.len &&
		   memcmp("mode", query.value, query.len) == 0) {

		ok = zcbor_tstr_put_lit(zse, "mode") &&
		     zcbor_int32_put(zse, BOOTLOADER_MODE);
#if IS_ENABLED(MCUBOOT_BOOTLOADER_NO_DOWNGRADE)
		ok = zcbor_tstr_put_lit(zse, "no-downgrade") &&
		     zcbor_bool_encode(zse, true);
#endif
	} else {
		return OS_MGMT_ERR_QUERY_YIELDS_NO_ANSWER;
	}

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}
#endif

#ifdef CONFIG_MCUMGR_GRP_OS_INFO
/**
 * Command handler: os info
 */
static int os_mgmt_info(struct smp_streamer *ctxt)
{
	struct zcbor_string format = { 0 };
	uint8_t output[CONFIG_MCUMGR_GRP_OS_INFO_MAX_RESPONSE_SIZE] = { 0 };
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	uint32_t format_bitmask = 0;
	bool prior_output = false;
	size_t i = 0;
	size_t decoded;
	bool custom_os_name = false;
	int rc;
	uint16_t output_length = 0;
	uint16_t valid_formats = 0;

	struct zcbor_map_decode_key_val fs_info_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("format", zcbor_tstr_decode, &format),
	};

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
	struct os_mgmt_info_check check_data = {
		.format = &format,
		.format_bitmask = &format_bitmask,
		.valid_formats = &valid_formats,
		.custom_os_name = &custom_os_name,
	};

	struct os_mgmt_info_append append_data = {
		.format_bitmask = &format_bitmask,
		.all_format_specified = false,
		.output = output,
		.output_length = &output_length,
		.buffer_size = sizeof(output),
		.prior_output = &prior_output,
	};
#endif

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;
#endif

	if (zcbor_map_decode_bulk(zsd, fs_info_decode, ARRAY_SIZE(fs_info_decode), &decoded)) {
		return MGMT_ERR_EINVAL;
	}

	/* Process all input characters in format value */
	while (i < format.len) {
		switch (format.value[i]) {
		case 'a': {
#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
			append_data.all_format_specified = true;
#endif

			format_bitmask = OS_MGMT_INFO_FORMAT_ALL;
			++valid_formats;
			break;
		}
		case 's': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_KERNEL_NAME;
			++valid_formats;
			break;
		}
		case 'n': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_NODE_NAME;
			++valid_formats;
			break;
		}
		case 'r': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_KERNEL_RELEASE;
			++valid_formats;
			break;
		}
		case 'v': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_KERNEL_VERSION;
			++valid_formats;
			break;
		}
#ifdef CONFIG_MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME
		case 'b': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_BUILD_DATE_TIME;
			++valid_formats;
			break;
		}
#endif
		case 'm': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_MACHINE;
			++valid_formats;
			break;
		}
		case 'p': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_PROCESSOR;
			++valid_formats;
			break;
		}
		case 'i': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_HARDWARE_PLATFORM;
			++valid_formats;
			break;
		}
		case 'o': {
			format_bitmask |= OS_MGMT_INFO_FORMAT_OPERATING_SYSTEM;
			++valid_formats;
			break;
		}
		default: {
			break;
		}
		}

		++i;
	}

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
	/* Run callbacks to see if any additional handlers will add options */
	(void)mgmt_callback_notify(MGMT_EVT_OP_OS_MGMT_INFO_CHECK, &check_data,
				   sizeof(check_data), &err_rc, &err_group);
#endif

	if (valid_formats != format.len) {
		/* A provided format specifier is not valid */
		bool ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_OS, OS_MGMT_ERR_INVALID_FORMAT);

		return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
	} else if (format_bitmask == 0) {
		/* If no value is provided, use default of kernel name */
		format_bitmask = OS_MGMT_INFO_FORMAT_KERNEL_NAME;
	}

	/* Process all options in order and append to output string */
	if (format_bitmask & OS_MGMT_INFO_FORMAT_KERNEL_NAME) {
		rc = snprintf(output, (sizeof(output) - output_length), "Zephyr");

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
	}

	if (format_bitmask & OS_MGMT_INFO_FORMAT_NODE_NAME) {
		/* Get hostname, if enabled */
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
		/* From network */
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s" : "%s"), net_hostname_get());
#elif defined(CONFIG_BT)
		/* From Bluetooth */
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s" : "%s"), bt_get_name());
#else
		/* Not available */
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      "%sunknown", (prior_output == true ? " " : ""));
#endif

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_NODE_NAME;
	}

	if (format_bitmask & OS_MGMT_INFO_FORMAT_KERNEL_RELEASE) {
#ifdef BUILD_VERSION
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s" : "%s"), STRINGIFY(BUILD_VERSION));
#else
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      "%sunknown", (prior_output == true ? " " : ""));
#endif

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_KERNEL_RELEASE;
	}

	if (format_bitmask & OS_MGMT_INFO_FORMAT_KERNEL_VERSION) {
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s" : "%s"), KERNEL_VERSION_STRING);

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_KERNEL_VERSION;
	}

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME
	if (format_bitmask & OS_MGMT_INFO_FORMAT_BUILD_DATE_TIME) {
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s" : "%s"),
			      MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME);

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_BUILD_DATE_TIME;
	}
#endif

	if (format_bitmask & OS_MGMT_INFO_FORMAT_MACHINE) {
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s" : "%s"), CONFIG_ARCH);

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_MACHINE;
	}

	if (format_bitmask & OS_MGMT_INFO_FORMAT_PROCESSOR) {
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s" : "%s"), PROCESSOR_NAME);

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_PROCESSOR;
	}

	if (format_bitmask & OS_MGMT_INFO_FORMAT_HARDWARE_PLATFORM) {
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      (prior_output == true ? " %s%s%s" : "%s%s%s"), CONFIG_BOARD,
			      (sizeof(CONFIG_BOARD_REVISION) > 1 ? "@" : ""),
			      CONFIG_BOARD_REVISION);

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_HARDWARE_PLATFORM;
	}

	/* If custom_os_name is not set (by extension code) then return the default OS name of
	 * Zephyr
	 */
	if (format_bitmask & OS_MGMT_INFO_FORMAT_OPERATING_SYSTEM && custom_os_name == false) {
		rc = snprintf(&output[output_length], (sizeof(output) - output_length),
			      "%sZephyr", (prior_output == true ? " " : ""));

		if (rc < 0 || rc >= (sizeof(output) - output_length)) {
			goto fail;
		} else {
			output_length += (uint16_t)rc;
		}

		prior_output = true;
		format_bitmask &= ~OS_MGMT_INFO_FORMAT_OPERATING_SYSTEM;
	}

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
	/* Call custom handler command for additional output/processing */
	status = mgmt_callback_notify(MGMT_EVT_OP_OS_MGMT_INFO_APPEND, &append_data,
				      sizeof(append_data), &err_rc, &err_group);

	if (status != MGMT_CB_OK) {
		bool ok;

		if (status == MGMT_CB_ERROR_RC) {
			return err_rc;
		}

		ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
		return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
	}
#endif

	if (zcbor_tstr_put_lit(zse, "output") &&
	    zcbor_tstr_encode_ptr(zse, output, output_length)) {
		return MGMT_ERR_EOK;
	}

fail:
	return MGMT_ERR_EMSGSIZE;
}
#endif

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief	Translate OS mgmt group error code into MCUmgr error code
 *
 * @param ret	#os_mgmt_err_code_t error code
 *
 * @return	#mcumgr_err_t error code
 */
static int os_mgmt_translate_error_code(uint16_t err)
{
	int rc;

	switch (err) {
	case OS_MGMT_ERR_INVALID_FORMAT:
		rc = MGMT_ERR_EINVAL;
		break;

	case OS_MGMT_ERR_UNKNOWN:
	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler os_mgmt_group_handlers[] = {
#ifdef CONFIG_MCUMGR_GRP_OS_ECHO
	[OS_MGMT_ID_ECHO] = {
		os_mgmt_echo, os_mgmt_echo
	},
#endif
#ifdef CONFIG_MCUMGR_GRP_OS_TASKSTAT
	[OS_MGMT_ID_TASKSTAT] = {
		os_mgmt_taskstat_read, NULL
	},
#endif
#ifdef CONFIG_REBOOT
	[OS_MGMT_ID_RESET] = {
		NULL, os_mgmt_reset
	},
#endif
#ifdef CONFIG_MCUMGR_GRP_OS_MCUMGR_PARAMS
	[OS_MGMT_ID_MCUMGR_PARAMS] = {
		os_mgmt_mcumgr_params, NULL
	},
#endif
#ifdef CONFIG_MCUMGR_GRP_OS_INFO
	[OS_MGMT_ID_INFO] = {
		os_mgmt_info, NULL
	},
#endif
#ifdef CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO
	[OS_MGMT_ID_BOOTLOADER_INFO] = {
		os_mgmt_bootloader_info, NULL
	},
#endif
};

#define OS_MGMT_GROUP_SZ ARRAY_SIZE(os_mgmt_group_handlers)

static struct mgmt_group os_mgmt_group = {
	.mg_handlers = os_mgmt_group_handlers,
	.mg_handlers_count = OS_MGMT_GROUP_SZ,
	.mg_group_id = MGMT_GROUP_ID_OS,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = os_mgmt_translate_error_code,
#endif
};

static void os_mgmt_register_group(void)
{
	mgmt_register_group(&os_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(os_mgmt, os_mgmt_register_group);
