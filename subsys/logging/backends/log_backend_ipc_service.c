/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_ipc_service.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_ipc_service.h>
#include <zephyr/logging/log_ctrl.h>

#if DT_HAS_CHOSEN(zephyr_log_ipc)
#define IPC_NODE DT_CHOSEN(zephyr_log_ipc)
#elif DT_CHILD_NUM_STATUS_OKAY(DT_PATH(ipc))
#define IPC_NODE DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(ipc), UTIL_EVAL)
#else
#error "No IPC node found"
#endif

/* Time given to the remote to bind the logging endpoint during initialization. */
#define BOUND_TIMEOUT_MS 4000

static void dropped_notify(struct k_work *work);

static void log_backend_ipc_service_process(const struct log_backend *const backend,
		    union log_msg_generic *msg);
static void log_backend_ipc_service_panic(struct log_backend const *const backend);
static void log_backend_ipc_service_dropped(const struct log_backend *const backend, uint32_t cnt);
static void log_backend_ipc_service_init(struct log_backend const *const backend);
static int log_backend_ipc_service_is_ready(struct log_backend const *const backend);

const struct log_backend_api log_backend_ipc_service_api = {
	.process = log_backend_ipc_service_process,
	.panic = log_backend_ipc_service_panic,
	.dropped = log_backend_ipc_service_dropped,
	.init = log_backend_ipc_service_init,
	.is_ready = log_backend_ipc_service_is_ready
};

LOG_BACKEND_IPC_SERVICE_DEFINE(backend_ipc_service, IPC_NODE);

/* Accumulate dropped messages and schedule the notification on the first one. Drops
 * that occur before the notification executes only increment the counter.
 */
static void dropped_add(struct log_backend_ipc_service_data *data, uint32_t cnt)
{
	k_timeout_t delay = K_MSEC(CONFIG_LOG_BACKEND_IPC_SERVICE_DROPPED_NOTIFY_DELAY_MS);

	if (atomic_add(&data->dropped, cnt) != 0) {
		return;
	}

	(void)k_work_schedule(&data->dropped_work, delay);
}

static void log_backend_ipc_service_process(const struct log_backend *const backend,
		    union log_msg_generic *msg)
{
	int err;
	const struct log_backend_ipc_service_config *config = backend->cb->ctx;
	struct log_backend_ipc_service_data *data = config->data;
	uint32_t dlen = msg->log.hdr.desc.data_len;
	int fsc_plen;

	if (data->panic) {
		return;
	}

	fsc_plen = cbprintf_fsc_package(msg->log.data,
					msg->log.hdr.desc.package_len,
					NULL,
					0);
	if (fsc_plen < 0) {
		__ASSERT_NO_MSG(false);
		return;
	}

	/* Need to ensure that package is aligned to a pointer size even though
	 * it is in the packed structured.
	 */
	size_t log_msg_len = Z_LOG_MSG_ALIGNED_WLEN(fsc_plen, dlen) * sizeof(uint32_t);
	size_t msg_offset = offsetof(struct log_ipc_service_msg, data);
	size_t msg_len = log_msg_len + msg_offset;
	uint8_t buf[msg_len] __aligned(sizeof(void *));
	struct log_ipc_service_msg *out_msg;
	struct log_msg *out_log_msg;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_IPC_SERVICE_ZERO_COPY)) {
		uint32_t size = msg_len;

		err = ipc_service_get_tx_buffer(&data->ept, (void **)&out_msg, &size, K_NO_WAIT);
		if (err < 0) {
			dropped_add(data, 1);
			return;
		}
	} else {
		out_msg = (struct log_ipc_service_msg *)buf;
	}

	out_log_msg = (struct log_msg *)out_msg->data.log_msg.data;

	/* Set ipc message id. */
	out_msg->id = Z_LOG_IPC_SERVICE_ID_MSG;
	/* Status holds number of log messages in the IPC buffer. */
	out_msg->status = 1;
	/* Copy log message header. */
	memcpy(&out_log_msg->hdr, &msg->log.hdr, sizeof(struct log_msg_hdr));
	/* Update package len field in the message descriptor. */
	out_log_msg->hdr.desc.package_len = fsc_plen;

	out_log_msg->hdr.source = (const void *)(out_log_msg->hdr.source ?
				log_source_id(out_log_msg->hdr.source) : -1);

	/* Fill new package. */
	fsc_plen = cbprintf_fsc_package(msg->log.data, msg->log.hdr.desc.package_len,
					out_log_msg->data, fsc_plen);
	if (fsc_plen < 0) {
		__ASSERT_NO_MSG(false);
		return;
	}

	/* Copy data */
	if (dlen) {
		memcpy(&out_log_msg->data[fsc_plen],
		       &msg->log.data[msg->log.hdr.desc.package_len],
		       dlen);
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_IPC_SERVICE_ZERO_COPY)) {
		err = ipc_service_send_nocopy(&data->ept, out_msg, msg_len);
	} else {
		err = ipc_service_send(&data->ept, out_msg, msg_len);
	}

	if (err < 0) {
		if (IS_ENABLED(CONFIG_LOG_BACKEND_IPC_SERVICE_ZERO_COPY)) {
			(void)ipc_service_drop_tx_buffer(&data->ept, out_msg);
		}
		dropped_add(data, 1);
		return;
	}
}

static void log_backend_ipc_service_init(struct log_backend const *const backend)
{
	const struct log_backend_ipc_service_config *config = backend->cb->ctx;
	struct log_backend_ipc_service_data *data = config->data;
	int err;

	data->log_backend = backend;
	k_sem_init(&data->rdy_sem, 0, 1);
	k_work_init_delayable(&data->dropped_work, dropped_notify);

	err = ipc_service_open_instance(config->ipc_instance);
	__ASSERT_NO_MSG(err >= 0 || err == -EALREADY);

	err = ipc_service_register_endpoint(config->ipc_instance, &data->ept, &config->ept_cfg);
	__ASSERT_NO_MSG(err >= 0);

	err = k_sem_take(&data->rdy_sem, K_MSEC(4000));
	__ASSERT_NO_MSG(err >= 0);
}

static int log_backend_ipc_service_is_ready(struct log_backend const *const backend)
{
	const struct log_backend_ipc_service_config *config = backend->cb->ctx;

	return config->data->ready ? 0 : -EINPROGRESS;
}

static void log_backend_ipc_service_panic(struct log_backend const *const backend)
{
	const struct log_backend_ipc_service_config *config = backend->cb->ctx;

	config->data->panic = true;
}

static void log_backend_ipc_service_dropped(const struct log_backend *const backend, uint32_t cnt)
{
	const struct log_backend_ipc_service_config *config = backend->cb->ctx;

	dropped_add(config->data, cnt);
}

static void dropped_notify(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct log_backend_ipc_service_data *data =
		CONTAINER_OF(dwork, struct log_backend_ipc_service_data, dropped_work);
	uint32_t cnt = (uint32_t)atomic_set(&data->dropped, 0);
	int err;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_DROPPED,
		.status = Z_LOG_IPC_SERVICE_STATUS_OK,
		.data = {
			.dropped = {
				.dropped = cnt
			}
		}
	};

	if (cnt == 0) {
		return;
	}

	err = ipc_service_send(&data->ept, &msg, sizeof(msg));
	(void)err;
	__ASSERT_NO_MSG(err >= 0);
}

TYPE_SECTION_START_EXTERN(const char *, log_strings);

static void send_ready(const struct log_backend_ipc_service_config *config)
{
	int err;
	/* Local addresses are of use to the remote only if it can read local data. */
	bool str_access = IS_ENABLED(CONFIG_LOG_BACKEND_IPC_SERVICE_STRING_ACCESS);
	/* Source IDs put in log messages index the dynamic section when runtime filtering
	 * is enabled but both sections are ordered by the module name so the same ID
	 * addresses the same source in either of them. The constant section is shared as
	 * it is the one holding source names and compile time levels.
	 */
	uintptr_t source_addr = (uintptr_t)TYPE_SECTION_START(log_const);
	uintptr_t log_str_ptr = (uintptr_t)TYPE_SECTION_START(log_strings);
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_READY,
		.status = Z_LOG_IPC_SERVICE_STATUS_OK,
		.data = {
			.ready = {
				.source_addr = str_access ? source_addr : (uintptr_t)0,
				.log_str_ptr = str_access ? log_str_ptr : (uintptr_t)0,
			}
		}
	};

	msg.data.ready.source_count = log_src_cnt_get(0);
	err = ipc_service_send(&config->data->ept, &msg, sizeof(msg));
	(void)err;
	__ASSERT_NO_MSG(err >= 0);
}

void log_backend_ipc_service_bound_cb(void *priv)
{
	struct log_backend_ipc_service_config *config = priv;

	send_ready(config);
	config->data->ready = true;

	k_sem_give(&config->data->rdy_sem);
}

static void get_name_response(const struct log_backend_ipc_service_config *config,
			      uint16_t source_id)
{
	const char *name = log_source_name_get(0, source_id);
	size_t slen = strlen(name);
	size_t msg_offset = offsetof(struct log_ipc_service_msg, data);
	size_t msg_size = slen + 1 + msg_offset +
		sizeof(struct log_ipc_service_source_name);
	uint8_t msg_buf[msg_size];
	struct log_ipc_service_msg *outmsg = (struct log_ipc_service_msg *)msg_buf;
	char *dst = outmsg->data.source_name.name;
	int err;

	outmsg->id = Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME;
	outmsg->status = Z_LOG_IPC_SERVICE_STATUS_OK;
	memcpy(dst, name, slen);
	dst[slen] = '\0';

	outmsg->data.source_name.source_id = source_id;

	err = ipc_service_send(&config->data->ept, outmsg, msg_size);
	__ASSERT_NO_MSG(err >= 0);
}

void log_backend_ipc_service_recv_cb(const void *data, size_t len, void *priv)
{
	const struct log_backend_ipc_service_config *config = priv;

	struct log_ipc_service_msg *msg = (struct log_ipc_service_msg *)data;
	struct log_ipc_service_msg outmsg = {
		.id = msg->id,
		.status = Z_LOG_IPC_SERVICE_STATUS_OK
	};
	int err;

	memcpy(&outmsg, msg, sizeof(struct log_ipc_service_msg));

	switch (msg->id) {
	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME:
		get_name_response(config, msg->data.source_name.source_id);
		return;

	case Z_LOG_IPC_SERVICE_ID_GET_LEVELS:
		outmsg.data.levels.level =
			log_filter_get(config->data->log_backend,
				       outmsg.data.levels.domain_id,
				       outmsg.data.levels.source_id,
				       false);
		outmsg.data.levels.runtime_level =
			log_filter_get(config->data->log_backend,
				       outmsg.data.levels.domain_id,
				       outmsg.data.levels.source_id,
				       true);
		break;
	case Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL:
		outmsg.data.set_rt_level.runtime_level =
			log_filter_set(config->data->log_backend,
				       outmsg.data.set_rt_level.domain_id,
				       outmsg.data.set_rt_level.source_id,
				       outmsg.data.set_rt_level.runtime_level);
		break;
	default:
		__ASSERT(0, "Unexpected message");
		break;
	}

	err = ipc_service_send(&config->data->ept, &outmsg, sizeof(outmsg));
	__ASSERT_NO_MSG(err >= 0);
}
