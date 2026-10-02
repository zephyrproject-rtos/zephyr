/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/logging/log_ipc_service.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(link_ipc);

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

/** @brief Response returned by the associated remote backend. */
union log_link_ipc_rsp {
	uint16_t count; /**< Source count. */

	/** @brief Destination buffer for a requested name. */
	struct {
		char *dst;   /**< Output buffer for the name. */
		size_t *len; /**< In/out buffer length, updated with the name length. */
	} name;

	/** @brief Returned level settings of a source. */
	struct {
		uint8_t level;         /**< Compile-time level. */
		uint8_t runtime_level; /**< Run-time level. */
	} levels;

	/** @brief Result of a set-runtime-level request. */
	struct {
		uint8_t level; /**< Run-time level actually set. */
	} set_runtime_level;
};

/** @brief Run-time data of the link. */
struct log_link_ipc_data {
	/** Registered IPC service endpoint. */
	struct ipc_ept ept;
	/** Semaphore signaled when a response is ready. */
	struct k_sem rdy_sem;
	/** Associated log link instance. */
	const struct log_link *link;
	/** Storage for the latest response. */
	union log_link_ipc_rsp rsp;
	/** Status of the last operation. */
	int status;
	/** Set once the remote endpoint is bound. */
	bool ready;
};

static struct log_link_ipc_data link_ipc_data;

static void bound_cb(void *priv)
{
	struct log_link_ipc_data *data = priv;

	data->status = 0;
	data->ready = true;
}

static void error_cb(const char *message, void *priv)
{
	struct log_link_ipc_data *data = priv;

	ARG_UNUSED(message);

	data->status = -EIO;
}

static void recv_cb(const void *buffer, size_t len, void *priv)
{
	struct log_link_ipc_data *data = priv;
	struct log_ipc_service_msg *msg = (struct log_ipc_service_msg *)buffer;

	if (msg->status != Z_LOG_IPC_SERVICE_STATUS_OK) {
		data->status = -EIO;
		goto exit;
	} else {
		data->status = 0;
	}

	switch (msg->id) {
	case Z_LOG_IPC_SERVICE_ID_MSG:
		z_log_msg_enqueue(data->link, msg->data.log_msg.data,
				  len - offsetof(struct log_ipc_service_msg, data));
		return;
	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_CNT:
		data->rsp.count = msg->data.source_cnt.count;
		break;
	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME:
	{
		size_t slen = MIN(len - 1, *data->rsp.name.len - 1);

		*data->rsp.name.len = len - 1;
		memcpy(data->rsp.name.dst, msg->data.source_name.name, slen);
		data->rsp.name.dst[slen] = '\0';
		break;
	}
	case Z_LOG_IPC_SERVICE_ID_GET_LEVELS:
		data->rsp.levels.level = msg->data.levels.level;
		data->rsp.levels.runtime_level = msg->data.levels.runtime_level;
		break;
	case Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL:
		data->rsp.set_runtime_level.level = msg->data.set_rt_level.runtime_level;
		break;
	case Z_LOG_IPC_SERVICE_ID_DROPPED:
		return;
	case Z_LOG_IPC_SERVICE_ID_READY:
		break;
	default:
		__ASSERT(0, "Unexpected message");
		return;
	}

exit:
	k_sem_give(&data->rdy_sem);
}

static int getter_msg_process(struct log_link_ipc_data *data,
			      struct log_ipc_service_msg *msg, size_t msg_size)
{
	int err;

	err = ipc_service_send(&data->ept, msg, msg_size);
	if (err < 0) {
		return err;
	}

	err = k_sem_take(&data->rdy_sem, K_MSEC(1000));
	if (err < 0) {
		return err;
	}

	return (data->status == Z_LOG_IPC_SERVICE_STATUS_OK) ? 0 : -EIO;
}

static int link_remote_get_source_count(struct log_link_ipc_data *data, uint16_t *cnt)
{
	int err;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_GET_SOURCE_CNT,
	};

	err = getter_msg_process(data, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	*cnt = data->rsp.count;

	return 0;
}

static int link_remote_ready(struct log_link_ipc_data *data)
{
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_READY
	};

	return getter_msg_process(data, &msg, sizeof(msg));
}

static int link_remote_initiate(const struct log_link *link,
				struct log_link_config *config)
{
	struct log_link_ipc_data *data = link->ctx;
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

	ARG_UNUSED(config);

	data->link = link;
	k_sem_init(&data->rdy_sem, 0, 1);

	ept_cfg.priv = (void *)data;
	err = ipc_service_open_instance(ipc_instance);
	if (err < 0 && err != -EALREADY) {
		__ASSERT(0, "ipc_service_open_instance() failure (err:%d)\n", err);
		return err;
	}

	return ipc_service_register_endpoint(ipc_instance, &data->ept, &ept_cfg);
}

static int link_remote_activate(const struct log_link *link)
{
	struct log_link_ipc_data *data = link->ctx;
	uint16_t cnt;
	int err;

	if (!data->ready) {
		return -EINPROGRESS;
	}

	if (data->status != 0) {
		return data->status;
	}

	err = link_remote_get_source_count(data, &cnt);
	if (err < 0) {
		return err;
	}

	link->ctrl_blk->source_cnt = cnt;

	return link_remote_ready(data);
}

static int link_remote_get_source_name(const struct log_link *link, uint16_t source_id,
					char *name, size_t *length)
{
	struct log_link_ipc_data *data = link->ctx;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME,
		.data = {
			.source_name = {
				.source_id = source_id
			}
		}
	};

	data->rsp.name.dst = name;
	data->rsp.name.len = length;

	return getter_msg_process(data, &msg, sizeof(msg));
}

static int link_remote_get_levels(const struct log_link *link, uint16_t source_id,
				   uint8_t *level, uint8_t *runtime_level)
{
	struct log_link_ipc_data *data = link->ctx;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_GET_LEVELS,
		.data = {
			.levels = {
				.source_id = source_id
			}
		}
	};
	int err;

	err = getter_msg_process(data, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	if (level) {
		*level = data->rsp.levels.level;
	}
	if (runtime_level) {
		*runtime_level = data->rsp.levels.runtime_level;
	}

	return 0;
}

static int link_remote_set_runtime_level(const struct log_link *link, uint16_t source_id,
					 uint8_t level)
{
	struct log_link_ipc_data *data = link->ctx;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL,
		.data = {
			.set_rt_level = {
				.source_id = source_id,
				.runtime_level = level
			}
		}
	};

	return getter_msg_process(data, &msg, sizeof(msg));
}

static const struct log_link_api log_link_ipc_api = {
	.initiate = link_remote_initiate,
	.activate = link_remote_activate,
	.get_source_name = link_remote_get_source_name,
	.get_levels = link_remote_get_levels,
	.set_runtime_level = link_remote_set_runtime_level
};

LOG_LINK_DEF(link_ipc, log_link_ipc_api, CONFIG_LOG_LINK_IPC_BUFFER_SIZE, &link_ipc_data);

const struct log_link *log_link_ipc_get_link(void)
{
	return &link_ipc;
}
