/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gppi_ext_allocator.h"
#include <errno.h>

#include <helpers/nrfx_gppi.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define GPPI_EXT_MSG_LEN(member)                                                                   \
	(offsetof(struct gppi_ext_msg, member) + sizeof(((struct gppi_ext_msg *)0)->member))

#define GPPI_EXT_ALLOCATOR_TIMEOUT K_MSEC(CONFIG_GPPI_EXT_ALLOCATOR_TIMEOUT)

struct gppi_ext_allocator_data {
	struct ipc_ept ept;
#ifdef CONFIG_MULTITHREADING
	struct k_sem bound_sem;
#endif
	volatile bool ipc_bound;
};

struct gppi_ext_allocator_config {
	const struct device *instance;
	const struct ipc_ept_cfg ept_cfg;
};

static void ep_bound(void *priv)
{
	struct gppi_ext_allocator_data *data = priv;

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&data->bound_sem);
#endif
	data->ipc_bound = true;
}

static int wait_bound(struct gppi_ext_allocator_data *data)
{
	if (data->ipc_bound) {
		return 0;
	}
#ifdef CONFIG_MULTITHREADING
	return k_sem_take(&data->bound_sem, GPPI_EXT_ALLOCATOR_TIMEOUT);
#else
	while (!data->ipc_bound) {
	}

	return 0;
#endif
}

static int send_msg(struct gppi_ext_allocator_data *data, const struct gppi_ext_msg *msg,
		    size_t len)
{
	int ret;

	ret = wait_bound(data);
	if (ret < 0) {
		return ret;
	}

	return ipc_service_send(&data->ept, msg, len);
}

static void handle_conn_alloc(struct gppi_ext_allocator_data *data, const struct gppi_ext_msg *msg)
{
	struct gppi_ext_msg rsp;
	nrfx_gppi_handle_t handle;
	nrfx_gppi_resource_t resource;
	nrfx_gppi_resource_t *p_resource = NULL;
	int ret;

	rsp.id = GPPI_CONN_ALLOC_RSP;

	if (msg->conn_alloc.has_resource != 0U) {
		resource.domain_id = msg->conn_alloc.resource.domain_id;
		resource.channel = msg->conn_alloc.resource.channel;
		p_resource = &resource;
	}

	ret = nrfx_gppi_ext_conn_alloc(msg->conn_alloc.producer, msg->conn_alloc.consumer, &handle,
				       p_resource);
	rsp.conn_alloc_rsp.result = ret;
	rsp.conn_alloc_rsp.handle = (ret == 0) ? handle : 0U;

	(void)send_msg(data, &rsp, GPPI_EXT_MSG_LEN(conn_alloc_rsp));
}

static void handle_conn_free(const struct gppi_ext_msg *msg)
{
	nrfx_gppi_domain_conn_free(msg->conn_free.handle);
}

static void handle_group_alloc(struct gppi_ext_allocator_data *data, const struct gppi_ext_msg *msg)
{
	struct gppi_ext_msg rsp;
	nrfx_gppi_group_handle_t handle;
	int ret;

	rsp.id = GPPI_GROUP_ALLOC_RSP;

	ret = nrfx_gppi_group_alloc(msg->group_alloc.domain_id, &handle);
	rsp.group_alloc_rsp.result = ret;
	rsp.group_alloc_rsp.handle = (ret == 0) ? (uint32_t)handle : 0U;

	(void)send_msg(data, &rsp, GPPI_EXT_MSG_LEN(group_alloc_rsp));
}

static void handle_group_free(const struct gppi_ext_msg *msg)
{
	nrfx_gppi_group_free((nrfx_gppi_group_handle_t)msg->group_free.handle);
}

static void handle_channel_alloc(struct gppi_ext_allocator_data *data,
				 const struct gppi_ext_msg *msg)
{
	struct gppi_ext_msg rsp;
	int ret;

	rsp.id = GPPI_CHANNEL_ALLOC_RSP;

	ret = nrfx_gppi_channel_alloc(msg->channel_alloc.node_id);
	rsp.channel_alloc_rsp.result = ret;

	(void)send_msg(data, &rsp, GPPI_EXT_MSG_LEN(channel_alloc_rsp));
}

static void handle_channel_free(const struct gppi_ext_msg *msg)
{
	nrfx_gppi_channel_free(msg->channel_free.node_id, msg->channel_free.channel);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	const struct gppi_ext_msg *msg = data;
	struct gppi_ext_allocator_data *srv_data = priv;

	if (IS_ENABLED(CONFIG_IRONSIDE_SE_CALL) &&
	    ((msg->id == GPPI_CONN_ALLOC) || (msg->id == GPPI_CONN_FREE))) {
		/* If ironside is used then IPC callback must be in the thread context.
		 * Adding assert to detect if any IPC appears that uses interrupt context.
		 */
		__ASSERT_NO_MSG(!k_is_in_irq());
	}

	if (len < offsetof(struct gppi_ext_msg, conn_alloc)) {
		return;
	}

	switch (msg->id) {
	case GPPI_CONN_ALLOC:
		if (len < GPPI_EXT_MSG_LEN(conn_alloc)) {
			return;
		}

		handle_conn_alloc(srv_data, msg);
		break;
	case GPPI_CONN_FREE:
		if (len < GPPI_EXT_MSG_LEN(conn_free)) {
			return;
		}

		handle_conn_free(msg);
		break;
	case GPPI_GROUP_ALLOC:
		if (len < GPPI_EXT_MSG_LEN(group_alloc)) {
			return;
		}

		handle_group_alloc(srv_data, msg);
		break;
	case GPPI_GROUP_FREE:
		if (len < GPPI_EXT_MSG_LEN(group_free)) {
			return;
		}

		handle_group_free(msg);
		break;
	case GPPI_CHANNEL_ALLOC:
		if (len < GPPI_EXT_MSG_LEN(channel_alloc)) {
			return;
		}

		handle_channel_alloc(srv_data, msg);
		break;
	case GPPI_CHANNEL_FREE:
		if (len < GPPI_EXT_MSG_LEN(channel_free)) {
			return;
		}

		handle_channel_free(msg);
		break;
	default:
		break;
	}
}

static int init_allocator(const struct gppi_ext_allocator_config *config,
			  struct gppi_ext_allocator_data *data)
{
	int ret;

#ifdef CONFIG_MULTITHREADING
	k_sem_init(&data->bound_sem, 0, 1);
#endif

	ret = ipc_service_open_instance(config->instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	return ipc_service_register_endpoint(config->instance, &data->ept, &config->ept_cfg);
}

#define GPPI_EXT_ALLOCATOR_IPC(_name)                                                              \
	do {                                                                                       \
		static struct gppi_ext_allocator_data gppi_ext_data_##_name;                       \
		static struct gppi_ext_allocator_config gppi_ext_config_##_name = {                \
			.instance = DEVICE_DT_GET(DT_NODELABEL(_name)),                            \
			.ept_cfg =                                                                 \
				{                                                                  \
					.name = "gppi",                                            \
					.prio = 0,                                                 \
					.cb =                                                      \
						{                                                  \
							.bound = ep_bound,                         \
							.received = ep_recv,                       \
						},                                                 \
					.priv = &gppi_ext_data_##_name,                            \
				},                                                                 \
		};                                                                                 \
		int ret = init_allocator(&gppi_ext_config_##_name, &gppi_ext_data_##_name);        \
		if (ret < 0) {                                                                     \
			return ret;                                                                \
		}                                                                                  \
	} while (0)

static int gppi_ext_allocator_init(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ipc0))
	GPPI_EXT_ALLOCATOR_IPC(ipc0);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpuppr_ipc))
	GPPI_EXT_ALLOCATOR_IPC(cpuapp_cpuppr_ipc);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(cpuapp_cpuflpr_ipc))
	GPPI_EXT_ALLOCATOR_IPC(cpuapp_cpuflpr_ipc);
#endif
	return 0;
}

SYS_INIT(gppi_ext_allocator_init, POST_KERNEL, UTIL_INC(CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY));
