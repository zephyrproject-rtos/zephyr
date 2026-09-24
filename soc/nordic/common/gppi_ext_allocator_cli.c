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

union gppi_ext_allocator_handle {
	nrfx_gppi_handle_t handle;
	nrfx_gppi_group_handle_t group_handle;
};

static const struct device *ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
static struct ipc_ept ept;
static union gppi_ext_allocator_handle rsp_handle;
static int rsp_result;
static volatile bool ipc_bound;
static bool initialized;
#ifdef CONFIG_MULTITHREADING
static struct k_sem rsp_sem;
static struct k_sem bound_sem;
static struct k_mutex ipc_mutex;
#else
static volatile bool rsp_received;
#endif

static void ep_bound(void *priv)
{
	ARG_UNUSED(priv);

#ifdef CONFIG_MULTITHREADING
	k_sem_give(&bound_sem);
#endif
	ipc_bound = true;
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	const struct gppi_ext_msg *msg = data;

	ARG_UNUSED(priv);

	if (len < offsetof(struct gppi_ext_msg, conn_alloc)) {
		return;
	}

	switch (msg->id) {
	case GPPI_CONN_ALLOC_RSP:
		rsp_result = msg->conn_alloc_rsp.result;
		rsp_handle.handle = msg->conn_alloc_rsp.handle;
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&rsp_sem);
#else
		rsp_received = true;
#endif
		break;
	case GPPI_GROUP_ALLOC_RSP:
		rsp_result = msg->group_alloc_rsp.result;
		rsp_handle.group_handle = (nrfx_gppi_group_handle_t)msg->group_alloc_rsp.handle;
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&rsp_sem);
#else
		rsp_received = true;
#endif
		break;
	case GPPI_CHANNEL_ALLOC_RSP:
		rsp_result = msg->channel_alloc_rsp.result;
#ifdef CONFIG_MULTITHREADING
		k_sem_give(&rsp_sem);
#else
		rsp_received = true;
#endif
		break;
	default:
		break;
	}
}

static const struct ipc_ept_cfg gppi_ext_allocator_cli_ept_cfg = {
	.name = "gppi",
	.prio = 0,
	.cb = {
		.bound = ep_bound,
		.received = ep_recv,
	},
};

static int gppi_ext_allocator_cli_init(void)
{
	int ret;

#ifdef CONFIG_MULTITHREADING
	k_sem_init(&rsp_sem, 0, 1);
	k_sem_init(&bound_sem, 0, 1);
	k_mutex_init(&ipc_mutex);
#endif

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ept, &gppi_ext_allocator_cli_ept_cfg);
	if (ret < 0) {
		return ret;
	}

	initialized = true;
	return 0;
}

SYS_INIT(gppi_ext_allocator_cli_init, POST_KERNEL,
	 UTIL_INC(CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY));

static int wait_bound(void)
{
	if (ipc_bound) {
		return 0;
	}
#ifdef CONFIG_MULTITHREADING
	return k_sem_take(&bound_sem, GPPI_EXT_ALLOCATOR_TIMEOUT);
#else
	while (!ipc_bound) {
	}

	return 0;
#endif
}

static int wait_rsp(void)
{
#ifdef CONFIG_MULTITHREADING
	return k_sem_take(&rsp_sem, GPPI_EXT_ALLOCATOR_TIMEOUT);
#else
	while (!rsp_received) {
	}
	rsp_received = false;
	return 0;
#endif
}

static int send(const void *msg, size_t len, bool do_wait_rsp)
{
	int ret;

	if (!initialized) {
		return -EAGAIN;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

#ifdef CONFIG_MULTITHREADING
	ret = k_mutex_lock(&ipc_mutex, GPPI_EXT_ALLOCATOR_TIMEOUT);
	if (ret < 0) {
		return ret;
	}
#endif

	ret = wait_bound();
	if (ret < 0) {
		goto unlock;
	}

	ret = ipc_service_send(&ept, msg, len);
	if (ret < 0) {
		goto unlock;
	}

	if (do_wait_rsp) {
		ret = wait_rsp();
	}
unlock:
#ifdef CONFIG_MULTITHREADING
	k_mutex_unlock(&ipc_mutex);
#endif

	return ret;
}

int nrfx_gppi_ext_conn_alloc(uint32_t producer, uint32_t consumer, nrfx_gppi_handle_t *p_handle,
			     nrfx_gppi_resource_t *p_resource)
{
	struct gppi_ext_msg msg;
	int ret;

	if (p_handle == NULL) {
		return -EINVAL;
	}

	msg.id = GPPI_CONN_ALLOC;
	msg.conn_alloc.producer = producer;
	msg.conn_alloc.consumer = consumer;

	if (p_resource != NULL) {
		msg.conn_alloc.has_resource = 1;
		msg.conn_alloc.resource.domain_id = p_resource->domain_id;
		msg.conn_alloc.resource.channel = p_resource->channel;
	} else {
		msg.conn_alloc.has_resource = 0;
	}

	ret = send(&msg, GPPI_EXT_MSG_LEN(conn_alloc), true);
	if (ret < 0) {
		return ret;
	}

	*p_handle = rsp_handle.handle;

	return rsp_result;
}

void nrfx_gppi_domain_conn_free(nrfx_gppi_handle_t handle)
{
	struct gppi_ext_msg msg;
	int ret;

	msg.id = GPPI_CONN_FREE;
	msg.conn_free.handle = handle;

	ret = send(&msg, GPPI_EXT_MSG_LEN(conn_free), false);
	(void)ret;
	__ASSERT_NO_MSG(ret >= 0);
}

int nrfx_gppi_group_alloc(uint32_t domain_id, nrfx_gppi_group_handle_t *p_handle)
{
	struct gppi_ext_msg msg;
	int ret;

	if (p_handle == NULL) {
		return -EINVAL;
	}

	msg.id = GPPI_GROUP_ALLOC;
	msg.group_alloc.domain_id = domain_id;

	ret = send(&msg, GPPI_EXT_MSG_LEN(group_alloc), true);
	if (ret < 0) {
		return ret;
	}

	*p_handle = rsp_handle.group_handle;

	return rsp_result;
}

void nrfx_gppi_group_free(nrfx_gppi_group_handle_t handle)
{
	struct gppi_ext_msg msg;
	int ret;

	msg.id = GPPI_GROUP_FREE;
	msg.group_free.handle = handle;

	ret = send(&msg, GPPI_EXT_MSG_LEN(group_free), false);
	(void)ret;
	__ASSERT_NO_MSG(ret >= 0);
}

int nrfx_gppi_channel_alloc(uint32_t node_id)
{
	struct gppi_ext_msg msg;
	int ret;

	msg.id = GPPI_CHANNEL_ALLOC;
	msg.channel_alloc.node_id = node_id;

	ret = send(&msg, GPPI_EXT_MSG_LEN(channel_alloc), true);
	if (ret < 0) {
		return ret;
	}

	return rsp_result;
}

void nrfx_gppi_channel_free(uint32_t node_id, uint8_t channel)
{
	struct gppi_ext_msg msg;
	int ret;

	msg.id = GPPI_CHANNEL_FREE;
	msg.channel_free.node_id = node_id;
	msg.channel_free.channel = channel;

	ret = send(&msg, GPPI_EXT_MSG_LEN(channel_free), false);
	(void)ret;
	__ASSERT_NO_MSG(ret >= 0);
}
