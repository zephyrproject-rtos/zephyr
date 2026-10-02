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
#include <zephyr/logging/log_ctrl.h>

#if DT_HAS_CHOSEN(zephyr_log_ipc)
#define IPC_NODE DT_CHOSEN(zephyr_log_ipc)
#elif DT_NUM_INST_STATUS_OKAY(zephyr_ipc_icbmsg) == 1
#define IPC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_ipc_icbmsg)
#elif DT_NUM_INST_STATUS_OKAY(zephyr_ipc_icmsg) == 1
#define IPC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_ipc_icmsg)
#elif DT_NUM_INST_STATUS_OKAY(zephyr_ipc_openamp_static_vrings) == 1
#define IPC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_ipc_openamp_static_vrings)
#else
#error "No IPC node found"
#endif

/** @brief Run-time data of the backend. */
struct log_backend_ipc_data {
	/** Registered IPC service endpoint. */
	struct ipc_ept ept;
	/** Associated log backend instance. */
	const struct log_backend *log_backend;
	/** Semaphore signaled when the endpoint is bound. */
	struct k_sem rdy_sem;
	/** Status of the last operation. */
	int status;
	/** Set when the backend is in panic mode. */
	bool panic;
	/** Set once the remote link reported that it is ready. */
	bool ready;
};

static struct log_backend_ipc_data backend_ipc_data;

static void process(const struct log_backend *const backend,
		    union log_msg_generic *msg)
{
	int err;
	struct log_backend_ipc_data *data = backend->cb->ctx;
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
	uint32_t msg_len = Z_LOG_MSG_LEN(fsc_plen, dlen);
	uint8_t buf[msg_len + sizeof(void *)] __aligned(sizeof(void *));
	size_t msg_offset = offsetof(struct log_ipc_service_msg, data);
	struct log_ipc_service_msg *out_msg =
		(struct log_ipc_service_msg *)&buf[sizeof(void *) - msg_offset];
	uintptr_t out_log_msg_ptr = (uintptr_t)out_msg->data.log_msg.data;
	struct log_msg *out_log_msg = (struct log_msg *)out_log_msg_ptr;

	/* Set ipc message id. */
	out_msg->id = Z_LOG_IPC_SERVICE_ID_MSG;
	out_msg->status = Z_LOG_IPC_SERVICE_STATUS_OK;
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

	err = ipc_service_send(&data->ept, out_msg, msg_len + msg_offset);
	if (err < 0) {
		__ASSERT(false, "Unexpected error: %d\n", err);
		return;
	}
}

static void bound_cb(void *priv)
{
	struct log_backend_ipc_data *data = priv;

	data->status = 0;
	k_sem_give(&data->rdy_sem);
}

static void error_cb(const char *message, void *priv)
{
	struct log_backend_ipc_data *data = priv;

	ARG_UNUSED(message);

	data->status = -EIO;
}

static void get_name_response(struct log_backend_ipc_data *data, uint16_t source_id)
{
	const char *name = log_source_name_get(Z_LOG_LOCAL_DOMAIN_ID, source_id);
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

	err = ipc_service_send(&data->ept, outmsg, msg_size);
	__ASSERT_NO_MSG(err >= 0);
}

static void recv_cb(const void *buffer, size_t len, void *priv)
{
	struct log_backend_ipc_data *data = priv;
	struct log_ipc_service_msg *msg = (struct log_ipc_service_msg *)buffer;
	struct log_ipc_service_msg outmsg = {
		.id = msg->id,
		.status = Z_LOG_IPC_SERVICE_STATUS_OK
	};
	int err;

	ARG_UNUSED(len);

	memcpy(&outmsg, msg, sizeof(struct log_ipc_service_msg));

	switch (msg->id) {
	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_CNT:
		outmsg.data.source_cnt.count = log_src_cnt_get(Z_LOG_LOCAL_DOMAIN_ID);
		break;

	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME:
		get_name_response(data, msg->data.source_name.source_id);
		return;

	case Z_LOG_IPC_SERVICE_ID_GET_LEVELS:
		outmsg.data.levels.level =
			log_filter_get(data->log_backend,
				       Z_LOG_LOCAL_DOMAIN_ID,
				       outmsg.data.levels.source_id,
				       false);
		outmsg.data.levels.runtime_level =
			log_filter_get(data->log_backend,
				       Z_LOG_LOCAL_DOMAIN_ID,
				       outmsg.data.levels.source_id,
				       true);
		break;
	case Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL:
		outmsg.data.set_rt_level.runtime_level =
			log_filter_set(data->log_backend,
				       Z_LOG_LOCAL_DOMAIN_ID,
				       outmsg.data.set_rt_level.source_id,
				       outmsg.data.set_rt_level.runtime_level);
		break;
	case Z_LOG_IPC_SERVICE_ID_READY:
		data->ready = true;
		break;
	default:
		__ASSERT(0, "Unexpected message");
		break;
	}

	err = ipc_service_send(&data->ept, &outmsg, sizeof(outmsg));
	__ASSERT_NO_MSG(err >= 0);
}

static void init(struct log_backend const *const backend)
{
	struct log_backend_ipc_data *data = backend->cb->ctx;
	static struct ipc_ept_cfg ept_cfg = {
		.name = "logging",
		.prio = 0,
		.cb = {
			.bound    = bound_cb,
			.received = recv_cb,
			.error    = error_cb,
		},
	};
	const struct device *ipc_instance = DEVICE_DT_GET(IPC_NODE);
	int err;

	data->log_backend = backend;
	k_sem_init(&data->rdy_sem, 0, 1);

	ept_cfg.priv = (void *)data;
	err = ipc_service_open_instance(ipc_instance);
	__ASSERT_NO_MSG(err >= 0 || err == -EALREADY);

	err = ipc_service_register_endpoint(ipc_instance, &data->ept, &ept_cfg);
	__ASSERT_NO_MSG(err >= 0);

	err = k_sem_take(&data->rdy_sem, K_MSEC(4000));
	__ASSERT_NO_MSG(err >= 0);
}

static int is_ready(struct log_backend const *const backend)
{
	struct log_backend_ipc_data *data = backend->cb->ctx;

	return data->ready ? 0 : -EINPROGRESS;
}

static void panic(struct log_backend const *const backend)
{
	struct log_backend_ipc_data *data = backend->cb->ctx;

	data->panic = true;
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	struct log_backend_ipc_data *data = backend->cb->ctx;
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

	err = ipc_service_send(&data->ept, &msg, sizeof(msg));
	__ASSERT_NO_MSG(err >= 0);
}

static const struct log_backend_api log_backend_ipc_api = {
	.process = process,
	.panic = panic,
	.dropped = dropped,
	.init = init,
	.is_ready = is_ready
};

LOG_BACKEND_DEFINE(backend_ipc, log_backend_ipc_api, true, &backend_ipc_data);
