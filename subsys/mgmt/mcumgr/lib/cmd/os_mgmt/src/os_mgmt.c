/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <assert.h>
#include <string.h>
#include <zephyr.h>
#include <debug/object_tracing.h>
#include <kernel_structs.h>
#include <util/mcumgr_util.h>

#include "tinycbor/cbor.h"
#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"
#include "os_mgmt/os_mgmt.h"
#include "os_mgmt/os_mgmt_impl.h"

/**
 * Command handler: os echo
 */
#if CONFIG_OS_MGMT_ECHO
static int
os_mgmt_echo(struct mgmt_ctxt *ctxt)
{
	char echo_buf[CONFIG_OS_MGMT_ECHO_LENGTH + 1];
	CborError err;

	const struct cbor_attr_t attrs[2] = {
		[0] = {
			.attribute = "d",
			.type = CborAttrTextStringType,
			.addr.string = echo_buf,
			.nodefault = 1,
			.len = CONFIG_OS_MGMT_ECHO_LENGTH,
		},
		[1] = {
			.attribute = NULL
		}
	};

	echo_buf[0] = '\0';

	err = cbor_read_object(&ctxt->it, attrs);
	if (err != 0) {
		return mgmt_err_from_cbor(err);
	}

	echo_buf[sizeof(echo_buf) - 1] = '\0';

	err = cbor_encode_text_stringz(&ctxt->encoder, "r")				||
	      cbor_encode_text_stringz(&ctxt->encoder, echo_buf);

	return (err == 0) ? 0 : MGMT_ERR_ENOMEM;
}
#endif

#if CONFIG_OS_MGMT_TASKSTAT

#if defined(CONFIG_OS_MGMT_TASKSTAT_USE_THREAD_NAME_FOR_NAME)
static inline CborError
os_mgmt_taskstat_encode_thread_name(struct CborEncoder *encoder, int idx,
				    const struct k_thread *thread)
{
	size_t name_len = strlen(thread->name);

	ARG_UNUSED(idx);

	if (name_len > CONFIG_OS_MGMT_TASKSTAT_THREAD_NAME_LEN) {
		name_len = CONFIG_OS_MGMT_TASKSTAT_THREAD_NAME_LEN;
	}

	return cbor_encode_text_string(encoder, thread->name, name_len);
}

#else
static inline CborError
os_mgmt_taskstat_encode_thread_name(struct CborEncoder *encoder, int idx,
				    const struct k_thread *thread)
{
	CborError err = 0;
	char thread_name[CONFIG_OS_MGMT_TASKSTAT_THREAD_NAME_LEN + 1];

#if defined(CONFIG_OS_MGMT_TASKSTAT_USE_THREAD_PRIO_FOR_NAME)
	idx = (int)thread->base.prio;
#elif defined(CONFIG_OS_MGMT_TASKSTAT_USE_THREAD_IDX_FOR_NAME)
	ARG_UNUSED(thread);
#else
#error Unsupported option for taskstat thread name
#endif

	ll_to_s(idx, sizeof(thread_name) - 1, thread_name);
	thread_name[sizeof(thread_name) - 1] = 0;

	err = cbor_encode_text_stringz(encoder, thread_name);
	return err;
}

#endif

static inline int
os_mgmt_taskstat_encode_stack_info(struct CborEncoder *thread_map,
				   const struct k_thread *thread)
{
#ifdef CONFIG_OS_MGMT_TASKSTAT_STACK_INFO
	ssize_t stack_size = 0;
	ssize_t stack_used = 0;
	int err = 0;

#ifdef CONFIG_THREAD_STACK_INFO
	stack_size = thread->stack_info.size / 4;

#ifdef CONFIG_INIT_STACKS
	unsigned int stack_unused;

	if (k_thread_stack_space_get(thread, &stack_unused) == 0) {
		stack_used = (thread->stack_info.size - stack_unused) / 4;
	}
#endif /* CONFIG_INIT_STACKS */
#endif /* CONFIG_THREAD_STACK_INFO */

	err = cbor_encode_text_stringz(thread_map, "stksiz")	||
	      cbor_encode_uint(thread_map, stack_size)		||
	      cbor_encode_text_stringz(thread_map, "stkuse")	||
	      cbor_encode_uint(thread_map, stack_used);

	return err;
#else
	return 0;
#endif /* CONFIG_OS_MGMT_TASKSTAT_STACK_INFO */
}

static inline int
os_mgmt_taskstat_encode_unsupported(struct CborEncoder *thread_map)
{
	int err = 0;

	if (!IS_ENABLED(CONFIG_OS_MGMT_TASKSTAT_ONLY_SUPPORTED_STATS)) {
		err = cbor_encode_text_stringz(thread_map, "runtime")		||
		      cbor_encode_uint(thread_map, 0)				||
		      cbor_encode_text_stringz(thread_map, "cswcnt")		||
		      cbor_encode_uint(thread_map, 0)				||
		      cbor_encode_text_stringz(thread_map, "last_checkin")	||
		      cbor_encode_uint(thread_map, 0)				||
		      cbor_encode_text_stringz(thread_map, "next_checkin")	||
		      cbor_encode_uint(thread_map, 0);
	} else {
		ARG_UNUSED(thread_map);
	}

	return err;
}

static inline int
os_mgmt_taskstat_encode_priority(struct CborEncoder *thread_map, const struct k_thread *thread)
{
	return IS_ENABLED(CONFIG_OS_MGMT_TASKSTAT_SIGNED_PRIORITY) ?
		cbor_encode_int(thread_map, (int)thread->base.prio) :
		cbor_encode_uint(thread_map, (unsigned int)thread->base.prio & 0xff);
}

/**
 * Encodes a single taskstat entry.
 */
static int
os_mgmt_taskstat_encode_one(struct CborEncoder *encoder, int idx, const struct k_thread *thread)
{
	CborEncoder thread_map;
	CborError err;

	/*
	 * Threads are sent as map where thread name is key and value is map
	 * of thread parameters
	 */
	err = os_mgmt_taskstat_encode_thread_name(encoder, idx, thread)			||
	      cbor_encoder_create_map(encoder, &thread_map, CborIndefiniteLength)	||
	      cbor_encode_text_stringz(&thread_map, "prio")				||
	      os_mgmt_taskstat_encode_priority(&thread_map, thread)			||
	      cbor_encode_text_stringz(&thread_map, "tid")				||
	      cbor_encode_uint(&thread_map, idx)					||
	      cbor_encode_text_stringz(&thread_map, "state")				||
	      cbor_encode_uint(&thread_map, thread->base.thread_state)			||
	      os_mgmt_taskstat_encode_stack_info(&thread_map, thread)			||
	      os_mgmt_taskstat_encode_unsupported(&thread_map)				||
	      cbor_encoder_close_container(encoder, &thread_map);

	return err;
}

/**
 * Command handler: os taskstat
 */
static int os_mgmt_taskstat_read(struct mgmt_ctxt *ctxt)
{
	struct CborEncoder tasks_map;
	const struct k_thread *thread;
	int err;
	int thread_idx;

	err = cbor_encode_text_stringz(&ctxt->encoder, "tasks")				||
	      cbor_encoder_create_map(&ctxt->encoder, &tasks_map, CborIndefiniteLength);

	/* If could not create map just exit here */
	if (err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	/* Iterate the list of tasks, encoding each. */
	thread = SYS_THREAD_MONITOR_HEAD;
	thread_idx = 0;
	while (thread != NULL) {
		err = os_mgmt_taskstat_encode_one(&tasks_map, thread_idx, thread);
		if (err != 0) {
			break;
		}

		thread = SYS_THREAD_MONITOR_NEXT(thread);
		++thread_idx;
	}

	err |= cbor_encoder_close_container(&ctxt->encoder, &tasks_map);

	return (err != 0) ? MGMT_ERR_ENOMEM : 0;
}
#endif /* CONFIG_OS_MGMT_TASKSTAT */

/**
 * Command handler: os reset
 */
static int
os_mgmt_reset(struct mgmt_ctxt *ctxt)
{
	return os_mgmt_impl_reset(CONFIG_OS_MGMT_RESET_MS);
}

static const struct mgmt_handler os_mgmt_group_handlers[] = {
#if CONFIG_OS_MGMT_ECHO
	[OS_MGMT_ID_ECHO] = {
		os_mgmt_echo, os_mgmt_echo
	},
#endif
#if CONFIG_OS_MGMT_TASKSTAT
	[OS_MGMT_ID_TASKSTAT] = {
		os_mgmt_taskstat_read, NULL
	},
#endif
	[OS_MGMT_ID_RESET] = {
		NULL, os_mgmt_reset
	},
};

#define OS_MGMT_GROUP_SZ ARRAY_SIZE(os_mgmt_group_handlers)

static struct mgmt_group os_mgmt_group = {
	.mg_handlers = os_mgmt_group_handlers,
	.mg_handlers_count = OS_MGMT_GROUP_SZ,
	.mg_group_id = MGMT_GROUP_ID_OS,
};


void
os_mgmt_register_group(void)
{
	mgmt_register_group(&os_mgmt_group);
}

void
os_mgmt_module_init(void)
{
	os_mgmt_register_group();
}
