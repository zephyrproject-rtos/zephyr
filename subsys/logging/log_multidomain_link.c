/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/logging/log_multidomain_helper.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(link_ipc);

void log_multidomain_link_on_error(struct log_multidomain_link *link_remote, int err)
{
	link_remote->status = err;
}

void log_multidomain_link_on_started(struct log_multidomain_link *link_remote, int err)
{
	link_remote->status = err;

	if (err == 0) {
		link_remote->ready = true;
	}
}

void log_multidomain_link_on_recv_cb(struct log_multidomain_link *link_remote,
				const void *data, size_t len)
{
	struct log_multidomain_msg *msg = (struct log_multidomain_msg *)data;

	if (msg->status != Z_LOG_MULTIDOMAIN_STATUS_OK) {
		link_remote->status = -EIO;
		goto exit;
	} else {
		link_remote->status = 0;
	}

	switch (msg->id) {
	case Z_LOG_MULTIDOMAIN_ID_MSG:
		z_log_msg_enqueue(link_remote->link,
				  msg->data.log_msg.data,
				  len - offsetof(struct log_multidomain_msg, data));
		return;
	case Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_CNT:
		link_remote->dst.count = msg->data.domain_cnt.count;
		break;
	case Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_CNT:
		link_remote->dst.count = msg->data.source_cnt.count;
		break;
	case Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_NAME:
	{
		size_t slen = MIN(len - 1, *link_remote->dst.name.len - 1);

		*link_remote->dst.name.len = len - 1;
		memcpy(link_remote->dst.name.dst, msg->data.domain_name.name, slen);
		link_remote->dst.name.dst[slen] = '\0';
		break;
	}
	case Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_NAME:
	{
		size_t slen = MIN(len - 1, *link_remote->dst.name.len - 1);

		*link_remote->dst.name.len = len - 1;
		memcpy(link_remote->dst.name.dst, msg->data.source_name.name, slen);
		link_remote->dst.name.dst[slen] = '\0';
		break;
	}
	case Z_LOG_MULTIDOMAIN_ID_GET_LEVELS:
		link_remote->dst.levels.level = msg->data.levels.level;
		link_remote->dst.levels.runtime_level = msg->data.levels.runtime_level;
		break;
	case Z_LOG_MULTIDOMAIN_ID_SET_RUNTIME_LEVEL:
		link_remote->dst.set_runtime_level.level = msg->data.set_rt_level.runtime_level;
		break;
	case Z_LOG_MULTIDOMAIN_ID_READY:
		break;
	default:
		__ASSERT(0, "Unexpected message");
		break;
	}

exit:
	k_sem_give(&link_remote->rdy_sem);
}

static int getter_msg_process(struct log_multidomain_link *link_remote,
			      struct log_multidomain_msg *msg, size_t msg_size)
{
	int err;

	err = link_remote->transport_api->send(link_remote, msg, msg_size);
	if (err < 0) {
		return err;
	}

	err = k_sem_take(&link_remote->rdy_sem, K_MSEC(1000));
	if (err < 0) {
		return err;
	}

	return (link_remote->status == Z_LOG_MULTIDOMAIN_STATUS_OK) ? 0 : -EIO;
}

static int link_remote_get_domain_count(struct log_multidomain_link *link_remote,
					uint16_t *cnt)
{
	int err;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_CNT,
	};

	err = getter_msg_process(link_remote, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	*cnt = link_remote->dst.count;

	return 0;
}

static int link_remote_get_source_count(struct log_multidomain_link *link_remote,
					      uint32_t domain_id,
					      uint16_t *cnt)
{
	int err;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_CNT,
		.data = { .source_cnt = { .domain_id = domain_id } }
	};

	err = getter_msg_process(link_remote, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	*cnt = link_remote->dst.count;

	return 0;
}

static int link_remote_ready(struct log_multidomain_link *link_remote)
{
	int err;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_READY
	};

	err = getter_msg_process(link_remote, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	return 0;
}

static int link_remote_initiate(const struct log_link *link,
				struct log_link_config *config)
{
	struct log_multidomain_link *link_remote = link->ctx;

	link_remote->link = link;
	k_sem_init(&link_remote->rdy_sem, 0, 1);

	return link_remote->transport_api->init(link_remote);
}

static int link_remote_activate(const struct log_link *link)
{
	struct log_multidomain_link *link_remote = link->ctx;
	int err;

	if (!link_remote->ready) {
		return -EINPROGRESS;
	}

	if (link_remote->status != 0) {
		return link_remote->status;
	}

	uint16_t cnt;

	err = link_remote_get_domain_count(link_remote, &cnt);
	if (err < 0) {
		return err;
	}

	if (cnt > ARRAY_SIZE(link->ctrl_blk->source_cnt)) {
		__ASSERT(0, "Number of domains not supported.");
		return -ENOMEM;
	}

	link->ctrl_blk->domain_cnt = cnt;
	for (int i = 0; i < link->ctrl_blk->domain_cnt; i++) {
		err = link_remote_get_source_count(link_remote, i, &cnt);
		if (err < 0) {
			return err;
		}

		link->ctrl_blk->source_cnt[i] = cnt;
	}

	err = link_remote_ready(link_remote);

	return err;
}

static int link_remote_get_domain_name(const struct log_link *link,
					uint32_t domain_id,
					char *name, uint32_t *length)
{
	struct log_multidomain_link *link_remote = link->ctx;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_GET_DOMAIN_NAME,
		.data = { .domain_name = { .domain_id = domain_id } }
	};
	int err;


	link_remote->dst.name.dst = name;
	link_remote->dst.name.len = length;

	err = getter_msg_process(link_remote, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	return 0;
}

static int link_remote_get_source_name(const struct log_link *link,
					uint32_t domain_id, uint16_t source_id,
					char *name, size_t *length)
{
	struct log_multidomain_link *link_remote = link->ctx;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_GET_SOURCE_NAME,
		.data = {
			.source_name = {
				.domain_id = domain_id,
				.source_id = source_id
			}
		}
	};
	int err;

	link_remote->dst.name.dst = name;
	link_remote->dst.name.len = length;

	err = getter_msg_process(link_remote, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	return 0;
}

static int link_remote_get_levels(const struct log_link *link,
				   uint32_t domain_id, uint16_t source_id,
				   uint8_t *level, uint8_t *runtime_level)
{
	struct log_multidomain_link *link_remote = link->ctx;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_GET_LEVELS,
		.data = {
			.levels = {
				.domain_id = domain_id,
				.source_id = source_id
			}
		}
	};
	int err;

	err = getter_msg_process(link_remote, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	if (level) {
		*level = link_remote->dst.levels.level;
	}
	if (runtime_level) {
		*runtime_level = link_remote->dst.levels.runtime_level;
	}

	return 0;
}

static int link_remote_set_runtime_level(const struct log_link *link,
					 uint32_t domain_id, uint16_t source_id,
					 uint8_t level)
{
	struct log_multidomain_link *link_remote = link->ctx;
	struct log_multidomain_msg msg = {
		.id = Z_LOG_MULTIDOMAIN_ID_SET_RUNTIME_LEVEL,
		.data = {
			.set_rt_level = {
				.domain_id = domain_id,
				.source_id = source_id,
				.runtime_level = level
			}
		}
	};
	int err;

	err = getter_msg_process(link_remote, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	return 0;
}

struct log_link_api log_multidomain_link_api = {
	.initiate = link_remote_initiate,
	.activate = link_remote_activate,
	.get_domain_name = link_remote_get_domain_name,
	.get_source_name = link_remote_get_source_name,
	.get_levels = link_remote_get_levels,
	.set_runtime_level = link_remote_set_runtime_level
};
