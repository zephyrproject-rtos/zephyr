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
#include <zephyr/logging/log_backend_ipc.h>
#include <zephyr/logging/log_ctrl.h>

/* Time given to the remote to bind the logging endpoint during initialization. */
#define BOUND_TIMEOUT_MS 4000

/* Size of the message header that precedes the log message payload. */
#define IPC_MSG_HDR_SIZE offsetof(struct log_ipc_service_msg, data.log_msg.data)

static void dropped_notify(struct k_work *work);

/* Accumulate dropped messages and schedule the notification on the first one. Drops
 * that occur before the notification executes only increment the counter.
 */
static void dropped_add(struct log_backend_ipc_data *data, uint32_t cnt)
{
	k_timeout_t delay = K_MSEC(CONFIG_LOG_BACKEND_IPC_DROPPED_NOTIFY_DELAY_MS);

	if (atomic_add(&data->dropped, cnt) != 0) {
		return;
	}

	(void)k_work_schedule(&data->dropped_work, delay);
}

/* Write the log message converted to the self-contained form at @p dst. */
static bool out_msg_write(union log_msg_generic *msg, void *dst, int fsc_plen)
{
	struct log_msg *out_log_msg = dst;
	uint32_t dlen = msg->log.hdr.desc.data_len;

	/* Copy log message header. */
	memcpy(&out_log_msg->hdr, &msg->log.hdr, sizeof(struct log_msg_hdr));
	/* Update package len field in the message descriptor. */
	out_log_msg->hdr.desc.package_len = fsc_plen;

	out_log_msg->hdr.source =
		(const void *)(out_log_msg->hdr.source ? log_source_id(out_log_msg->hdr.source)
						       : -1);

	/* Fill new package. */
	fsc_plen = cbprintf_fsc_package(msg->log.data, msg->log.hdr.desc.package_len,
					out_log_msg->data, fsc_plen);
	if (fsc_plen < 0) {
		__ASSERT_NO_MSG(false);
		return false;
	}

	/* Copy data */
	if (dlen) {
		memcpy(&out_log_msg->data[fsc_plen], &msg->log.data[msg->log.hdr.desc.package_len],
		       dlen);
	}

	return true;
}

#ifdef CONFIG_LOG_BACKEND_IPC_ZERO_COPY

/* Send the packet that is being filled in. */
static void pkt_send(struct log_backend_ipc_data *data)
{
	int err;

	if (data->pkt == NULL) {
		return;
	}

	err = ipc_service_send_nocopy(&data->ept, data->pkt, IPC_MSG_HDR_SIZE + data->pkt_len);
	if (err < 0) {
		dropped_add(data, data->pkt->status);
		(void)ipc_service_drop_tx_buffer(&data->ept, data->pkt);
	}

	data->pkt = NULL;
	data->pkt_len = 0;
	data->pkt_cap = 0;
}

static void pkt_flush(struct log_backend_ipc_data *data)
{
	pkt_send(data);
}

/* Get the space for a log message of @p len bytes in the packet that is being filled
 * in. The pending packet is sent and a new one is allocated if the message does not fit.
 * Space is claimed by pkt_commit().
 */
static void *pkt_reserve(struct log_backend_ipc_data *data, size_t len)
{
	if (data->pkt != NULL &&
	    ((data->pkt_len + len > data->pkt_cap) || (data->pkt->status == UINT8_MAX))) {
		pkt_send(data);
	}

	if (data->pkt == NULL) {
		struct log_ipc_service_msg *pkt;
		uint32_t size = MAX(CONFIG_LOG_BACKEND_IPC_PACKET_SIZE, IPC_MSG_HDR_SIZE + len);
		int err;

		err = ipc_service_get_tx_buffer(&data->ept, (void **)&pkt, &size, K_NO_WAIT);
		if (err < 0) {
			return NULL;
		}

		/* Set ipc message id. */
		pkt->id = Z_LOG_IPC_SERVICE_ID_MSG;
		/* Status holds number of log messages in the IPC buffer. */
		pkt->status = 0;
		data->pkt = pkt;
		data->pkt_len = 0;
		data->pkt_cap = size - IPC_MSG_HDR_SIZE;
	}

	return &data->pkt->data.log_msg.data[data->pkt_len];
}

/* Add the message written at the reserved space to the packet. */
static void pkt_commit(struct log_backend_ipc_data *data, size_t len)
{
	data->pkt_len += len;
	data->pkt->status++;
}

static void out_msg_send(struct log_backend_ipc_data *data, union log_msg_generic *msg,
			 size_t log_msg_len, int fsc_plen)
{
	void *dst = pkt_reserve(data, log_msg_len);

	if (dst == NULL) {
		dropped_add(data, 1);
		return;
	}

	if (out_msg_write(msg, dst, fsc_plen)) {
		pkt_commit(data, log_msg_len);
	}
}

#else /* CONFIG_LOG_BACKEND_IPC_ZERO_COPY */

static inline void pkt_flush(struct log_backend_ipc_data *data)
{
	ARG_UNUSED(data);
}

static void out_msg_send(struct log_backend_ipc_data *data, union log_msg_generic *msg,
			 size_t log_msg_len, int fsc_plen)
{
	size_t msg_len = IPC_MSG_HDR_SIZE + log_msg_len;
	uint8_t buf[msg_len] __aligned(sizeof(void *));
	struct log_ipc_service_msg *out_msg = (struct log_ipc_service_msg *)buf;
	int err;

	/* Set ipc message id. */
	out_msg->id = Z_LOG_IPC_SERVICE_ID_MSG;
	/* Status holds number of log messages in the IPC buffer. */
	out_msg->status = 1;

	if (!out_msg_write(msg, out_msg->data.log_msg.data, fsc_plen)) {
		return;
	}

	err = ipc_service_send(&data->ept, out_msg, msg_len);
	if (err < 0) {
		dropped_add(data, 1);
	}
}

#endif /* CONFIG_LOG_BACKEND_IPC_ZERO_COPY */

static void log_backend_ipc_process(const struct log_backend *const backend,
				    union log_msg_generic *msg)
{
	const struct log_backend_ipc_config *config = backend->cb->ctx;
	struct log_backend_ipc_data *data = config->data;
	uint32_t dlen = msg->log.hdr.desc.data_len;
	size_t log_msg_len;
	int fsc_plen;

	if (data->panic) {
		return;
	}

	fsc_plen = cbprintf_fsc_package(msg->log.data, msg->log.hdr.desc.package_len, NULL, 0);
	if (fsc_plen < 0) {
		__ASSERT_NO_MSG(false);
		return;
	}

	/* Need to ensure that package is aligned to a pointer size even though
	 * it is in the packed structured.
	 */
	log_msg_len = Z_LOG_MSG_ALIGNED_WLEN(fsc_plen, dlen) * sizeof(uint32_t);

	out_msg_send(data, msg, log_msg_len, fsc_plen);
}

static void ipc_init(const struct log_backend_ipc_config *config)
{
	int err;
	struct log_backend_ipc_data *data = config->data;

	if (data->initialized) {
		return;
	}

	err = ipc_service_open_instance(config->ipc_instance);
	if (err < 0 && err != -EALREADY) {
		return;
	}

	err = ipc_service_register_endpoint(config->ipc_instance, &data->ept, &config->ept_cfg);
	if (err < 0) {
		return;
	}

	data->initialized = true;
}

static void log_backend_ipc_init(struct log_backend const *const backend)
{
	const struct log_backend_ipc_config *config = backend->cb->ctx;
	struct log_backend_ipc_data *data = config->data;

	data->log_backend = backend;
	k_sem_init(&data->rdy_sem, 0, 1);
	k_work_init_delayable(&data->dropped_work, dropped_notify);

	ipc_init(config);
}

static int log_backend_ipc_is_ready(struct log_backend const *const backend)
{
	const struct log_backend_ipc_config *config = backend->cb->ctx;

	ipc_init(config);

	return config->data->ready ? 0 : -EINPROGRESS;
}

static void log_backend_ipc_panic(struct log_backend const *const backend)
{
	const struct log_backend_ipc_config *config = backend->cb->ctx;

	config->data->panic = true;
	pkt_flush(config->data);
}

static void log_backend_ipc_notify(struct log_backend const *const backend,
				   enum log_backend_evt event, union log_backend_evt_arg *arg)
{
	const struct log_backend_ipc_config *config = backend->cb->ctx;

	ARG_UNUSED(arg);

	/* All pending messages are processed so send the packet without waiting for
	 * the flush timeout.
	 */
	if (event == LOG_BACKEND_EVT_PROCESS_THREAD_DONE) {
		pkt_flush(config->data);
	}
}

static void log_backend_ipc_dropped(const struct log_backend *const backend, uint32_t cnt)
{
	const struct log_backend_ipc_config *config = backend->cb->ctx;

	dropped_add(config->data, cnt);
}

static void dropped_notify(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct log_backend_ipc_data *data =
		CONTAINER_OF(dwork, struct log_backend_ipc_data, dropped_work);
	uint32_t cnt = (uint32_t)atomic_set(&data->dropped, 0);
	int err;
	struct log_ipc_service_msg msg = {.id = Z_LOG_IPC_SERVICE_ID_DROPPED,
					  .status = Z_LOG_IPC_SERVICE_STATUS_OK,
					  .data = {.dropped = {.dropped = cnt}}};

	if (cnt == 0) {
		return;
	}

	err = ipc_service_send(&data->ept, &msg, sizeof(msg));
	(void)err;
	__ASSERT_NO_MSG(err >= 0);
}

TYPE_SECTION_START_EXTERN(const char *, log_strings);

static void send_ready(const struct log_backend_ipc_config *config)
{
	int err;
	/* Local addresses are of use to the remote only if it can read local data. */
	bool str_access = !IS_ENABLED(CONFIG_LOG_BACKEND_IPC_FSC);
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
		.data = {.ready = {
				 .source_addr = str_access ? source_addr : (uintptr_t)0,
				 .log_str_ptr = str_access ? log_str_ptr : (uintptr_t)0,
			 }}};

	msg.data.ready.source_count = log_src_cnt_get(0);
	err = ipc_service_send(&config->data->ept, &msg, sizeof(msg));
	(void)err;
	__ASSERT_NO_MSG(err >= 0);
}

void log_backend_ipc_bound_cb(void *priv)
{
	struct log_backend_ipc_config *config = priv;

	send_ready(config);
	config->data->ready = true;
}

static void get_name_response(const struct log_backend_ipc_config *config, uint16_t source_id)
{
	const char *name = log_source_name_get(0, source_id);
	size_t slen = strlen(name);
	size_t msg_offset = offsetof(struct log_ipc_service_msg, data);
	size_t msg_size = slen + 1 + msg_offset + sizeof(struct log_ipc_service_source_name);
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

void log_backend_ipc_recv_cb(const void *data, size_t len, void *priv)
{
	const struct log_backend_ipc_config *config = priv;

	struct log_ipc_service_msg *msg = (struct log_ipc_service_msg *)data;
	struct log_ipc_service_msg outmsg = {.id = msg->id, .status = Z_LOG_IPC_SERVICE_STATUS_OK};
	int err;

	memcpy(&outmsg, msg, sizeof(struct log_ipc_service_msg));

	switch (msg->id) {
	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME:
		get_name_response(config, msg->data.source_name.source_id);
		return;

	case Z_LOG_IPC_SERVICE_ID_GET_LEVELS:
		outmsg.data.levels.level =
			log_filter_get(config->data->log_backend, outmsg.data.levels.domain_id,
				       outmsg.data.levels.source_id, false);
		outmsg.data.levels.runtime_level =
			log_filter_get(config->data->log_backend, outmsg.data.levels.domain_id,
				       outmsg.data.levels.source_id, true);
		break;
	case Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL:
		outmsg.data.set_rt_level.runtime_level = log_filter_set(
			config->data->log_backend, outmsg.data.set_rt_level.domain_id,
			outmsg.data.set_rt_level.source_id, outmsg.data.set_rt_level.runtime_level);
		break;
	default:
		__ASSERT(0, "Unexpected message");
		break;
	}

	err = ipc_service_send(&config->data->ept, &outmsg, sizeof(outmsg));
	__ASSERT_NO_MSG(err >= 0);
}

const struct log_backend_api log_backend_ipc_api = {.process = log_backend_ipc_process,
						    .panic = log_backend_ipc_panic,
						    .dropped = log_backend_ipc_dropped,
						    .init = log_backend_ipc_init,
						    .is_ready = log_backend_ipc_is_ready,
						    .notify = log_backend_ipc_notify};

#if DT_HAS_CHOSEN(zephyr_log_ipc)
#define IPC_NODE DT_CHOSEN(zephyr_log_ipc)
#elif DT_CHILD_NUM_STATUS_OKAY(DT_PATH(ipc)) == 1
#define IPC_NODE DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(ipc), UTIL_EVAL)
#endif

#if defined(IPC_NODE)
LOG_BACKEND_IPC_DEFINE(backend_ipc, IPC_NODE);
#endif
