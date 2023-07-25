/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_multidomain_helper.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>

static void process(const struct log_backend *const backend,
		    union log_msg_generic *msg)
{
	int err;
	struct log_multidomain_backend *backend_remote = backend->cb->ctx;
	uint32_t dlen = msg->log.hdr.desc.data_len;
	int fsc_plen;

	if (backend_remote->panic) {
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

	/* Need to ensure that package is aligned to a pointer size. */
	uint32_t msg_len = Z_LOG_MSG_LEN(fsc_plen, dlen);
	uint8_t buf[msg_len + sizeof(void *)] __aligned(sizeof(void *));
	size_t msg_offset = offsetof(struct log_multidomain_msg, data);
	struct log_multidomain_msg *out_msg =
		(struct log_multidomain_msg *)&buf[sizeof(void *) - msg_offset];
	struct log_msg *out_log_msg = (struct log_msg *)out_msg->data.log_msg.data;

	/* Set ipc message id. */
	out_msg->id = Z_LOG_MULTIDOMAIN_ID_MSG;
	out_msg->status = Z_LOG_MULTIDOMAIN_STATUS_OK;
	/* Copy log message header. */
	memcpy(&out_log_msg->hdr, &msg->log.hdr, sizeof(struct log_msg_hdr));
	/* Update package len field in the message descriptor. */
	out_log_msg->hdr.desc.package_len = fsc_plen;

	out_log_msg->hdr.source = out_log_msg->hdr.source ?
			(const void *)(IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
				log_dynamic_source_id((void *)out_log_msg->hdr.source) :
				log_const_source_id((void *)out_log_msg->hdr.source)) :
			(const void *)-1;

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

	err = backend_remote->transport_api->send(backend_remote, out_msg, msg_len + msg_offset);
	if (err < 0) {
		__ASSERT(false, "Unexpected error: %d\n", err);
		return;
	}
}

void log_multidomain_backend_on_started(struct log_multidomain_backend *backend_remote, int err)
{
	backend_remote->status = err;

	k_sem_give(&backend_remote->rdy_sem);
}

void log_multidomain_backend_on_error(struct log_multidomain_backend *backend_remote, int err)
{
	backend_remote->status = err;
}

static void get_name_response(struct log_multidomain_backend *backend_remote,
			      uint8_t domain_id, uint16_t source_id,
			      bool domain_name)
{
	const char *name = domain_name ?
			log_domain_name_get(domain_id) :
			log_source_name_get(domain_id, source_id);
	size_t slen = strlen(name);
	size_t msg_offset = offsetof(struct log_multidomain_msg, data);
	size_t msg_size = slen + 1 + msg_offset +
		(domain_name ? sizeof(struct log_multidomain_domain_name) :
				sizeof(struct log_multidomain_source_name));
	uint8_t msg_buf[msg_size];
	struct log_multidomain_msg *outmsg = (struct log_multidomain_msg *)msg_buf;
	char *dst = domain_name ?
			outmsg->data.domain_name.name :
			outmsg->data.source_name.name;
	int err;

	outmsg->id = domain_name ? Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_NAME :
				Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_NAME;
	outmsg->status = Z_LOG_MULTIDOMAIN_STATUS_OK;
	memcpy(dst, name, slen);
	dst[slen] = '\0';

	if (domain_name) {
		outmsg->data.domain_name.domain_id = domain_id;
	} else {
		outmsg->data.source_name.domain_id = domain_id;
		outmsg->data.source_name.source_id = source_id;
	}

	err = backend_remote->transport_api->send(backend_remote, outmsg, msg_size);
	__ASSERT_NO_MSG(err >= 0);
}

void log_multidomain_backend_on_recv_cb(struct log_multidomain_backend *backend_remote,
				   const void *data, size_t len)
{
	struct log_multidomain_msg *msg = (struct log_multidomain_msg *)data;
	struct log_multidomain_msg outmsg = {
		.id = msg->id,
		.status = Z_LOG_MULTIDOMAIN_STATUS_OK
	};
	int err;

	memcpy(&outmsg, msg, sizeof(struct log_multidomain_msg));

	switch (msg->id) {
	case Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_CNT:
		outmsg.data.domain_cnt.count = log_domains_count();
		break;

	case Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_CNT:
		outmsg.data.source_cnt.count =
			log_src_cnt_get(msg->data.source_cnt.domain_id);
		break;

	case Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_NAME:
		get_name_response(backend_remote,
				  msg->data.domain_name.domain_id,
				  0, true);
		return;

	case Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_NAME:
		get_name_response(backend_remote,
				  msg->data.source_name.domain_id,
				  msg->data.source_name.source_id,
				  false);
		return;

	case Z_LOG_MULTIDOMAIN_ID_GET_LEVELS:
		outmsg.data.levels.level =
			log_filter_get(backend_remote->log_backend,
				       outmsg.data.levels.domain_id,
				       outmsg.data.levels.source_id,
				       false);
		outmsg.data.levels.runtime_level =
			log_filter_get(backend_remote->log_backend,
				       outmsg.data.levels.domain_id,
				       outmsg.data.levels.source_id,
				       true);
		break;
	case Z_LOG_MULTIDOMAIN_ID_SET_RUNTIME_LEVEL:
		outmsg.data.set_rt_level.runtime_level =
			log_filter_set(backend_remote->log_backend,
				       outmsg.data.set_rt_level.domain_id,
				       outmsg.data.set_rt_level.source_id,
				       outmsg.data.set_rt_level.runtime_level);
		break;
	case Z_LOG_MULTIDOMAIN_ID_READY:
		backend_remote->ready = true;
		break;
	default:
		__ASSERT(0, "Unexpected message");
		break;
	}

	err = backend_remote->transport_api->send(backend_remote, &outmsg, sizeof(outmsg));
	__ASSERT_NO_MSG(err >= 0);
}

static void init(struct log_backend const *const backend)
{
	struct log_multidomain_backend *backend_remote = backend->cb->ctx;
	int err;

	backend_remote->log_backend = backend;
	k_sem_init(&backend_remote->rdy_sem, 0, 1);

	err = backend_remote->transport_api->init(backend_remote);
	__ASSERT_NO_MSG(err >= 0);

	err = k_sem_take(&backend_remote->rdy_sem, K_MSEC(4000));
	__ASSERT_NO_MSG(err >= 0);
}

static int is_ready(struct log_backend const *const backend)
{
	struct log_multidomain_backend *backend_remote = backend->cb->ctx;

	return backend_remote->ready ? 0 : -EINPROGRESS;
}

static void panic(struct log_backend const *const backend)
{
	struct log_multidomain_backend *backend_remote = backend->cb->ctx;

	backend_remote->panic = true;
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	struct log_multidomain_backend *backend_remote = backend->cb->ctx;
	int err;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_DROPPED,
		.status = Z_LOG_MULTIDOMAIN_STATUS_OK,
		.data = {
			.dropped = {
				.dropped = cnt
			}
		}
	};

	err = backend_remote->transport_api->send(backend_remote, &msg, sizeof(msg));
	__ASSERT_NO_MSG(err >= 0);
}

const struct log_backend_api log_multidomain_backend_api = {
	.process = process,
	.panic = panic,
	.dropped = dropped,
	.init = init,
	.is_ready = is_ready
};
