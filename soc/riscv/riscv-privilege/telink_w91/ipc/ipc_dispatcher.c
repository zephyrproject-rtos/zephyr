/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ipc_dispatcher.h"

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME ipc_dispatcher
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

struct ipc_dispatcher_record {
	sys_snode_t              node;
	uint32_t                 id;
	ipc_dispatcher_cb_t      callback;
	void                    *param;
};

static struct ipc_ept  ept;
static struct k_mutex  ipc_mutex;
static struct k_sem    ipc_sem_bound;
static sys_slist_t     ipc_records;

static void endpoint_bound(void *priv)
{
	k_sem_give(&ipc_sem_bound);
}

static void endpoint_received(const void *data, size_t len, void *priv)
{
	if (len >= sizeof(uint32_t)) {
		uint32_t *id = (uint32_t *)data;
		bool processed = false;
		struct ipc_dispatcher_record *record;

		k_mutex_lock(&ipc_mutex, K_FOREVER);
		SYS_SLIST_FOR_EACH_CONTAINER(&ipc_records, record, node) {
			if (record->id == *id && record->callback) {
				processed = true;
				record->callback(data, len, record->param);
				break;
			}
		}
		k_mutex_unlock(&ipc_mutex);

		if (!processed) {
			LOG_WRN("IPC received (id = %x) unprocessed", *id);
		}
	} else {
		LOG_WRN("IPC received malformed message");
	}
}

static struct ipc_ept_cfg ept_cfg = {
	.cb = {
		.bound    = endpoint_bound,
		.received = endpoint_received,
	},
};

static int ipc_dispatcher_start(void)
{
	const struct device *const ipc_instance = DEVICE_DT_GET(DT_CHOSEN(telink_ipc_interface));

	k_mutex_init(&ipc_mutex);
	k_sem_init(&ipc_sem_bound, 0, 1);
	sys_slist_init(&ipc_records);

	int ret = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);

	if (ret < 0) {
		LOG_ERR("IPC failed to register endpoint: %d", ret);
	} else {
		ret = k_sem_take(&ipc_sem_bound,
			K_MSEC(CONFIG_TELINK_W91_IPC_DISPATCHER_BOUND_TIMEOUT_MS));
		if (ret < 0) {
			LOG_ERR("IPC endpoint bind timed out: %d", ret);
		}
	}

	return ret;
}

void ipc_dispatcher_add(uint32_t id, ipc_dispatcher_cb_t cb, void *param)
{
	struct ipc_dispatcher_record *record = malloc(sizeof(struct ipc_dispatcher_record));

	if (record) {
		record->id = id;
		record->callback = cb;
		record->param = param;

		bool exist_elem = false;
		struct ipc_dispatcher_record *list_record;

		k_mutex_lock(&ipc_mutex, K_FOREVER);
		SYS_SLIST_FOR_EACH_CONTAINER(&ipc_records, list_record, node) {
			if (list_record->id == id) {
				exist_elem = true;
				break;
			}
		}
		if (!exist_elem) {
			sys_slist_append(&ipc_records, &record->node);
		} else {
			free(record);
			LOG_WRN("IPC dispatcher add record (id = %u) already exists %p",
				id, list_record->callback);
		}
		k_mutex_unlock(&ipc_mutex);

	} else {
		LOG_WRN("IPC dispatcher add record (id = %u) failed (no memory)", id);
	}
}

void ipc_dispatcher_rm(uint32_t id)
{
	bool rm_elem = false;
	struct ipc_dispatcher_record *record;
	sys_snode_t *prev_node = NULL;

	k_mutex_lock(&ipc_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&ipc_records, record, node) {
		if (record->id == id) {
			rm_elem = true;
			sys_slist_remove(&ipc_records, prev_node, &record->node);
			break;
		}
		prev_node = &record->node;
	}

	k_mutex_unlock(&ipc_mutex);

	if (rm_elem) {
		free(record);
	} else {
		LOG_WRN("IPC dispatcher no record (id = %u)", id);
	}
}

int ipc_dispatcher_send(const void *data, size_t len)
{
	return ipc_service_send(&ept, data, len);
}

SYS_INIT(ipc_dispatcher_start, POST_KERNEL, CONFIG_TELINK_W91_IPC_DISPATCHER_INIT_PRIORITY);
