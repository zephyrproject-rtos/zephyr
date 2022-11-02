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

LOG_MODULE_REGISTER(link_ipc);

struct log_link_ipc_service {
	struct ipc_ept ept;
	struct log_multidomain_link link_remote;
};

static void bound_cb(void *priv)
{
	struct log_multidomain_link *link_remote = priv;

	log_multidomain_link_on_started(link_remote, 0);
}

static void error_cb(const char *message, void *priv)
{
	struct log_multidomain_link *link_remote = priv;

	log_multidomain_link_on_error(link_remote, -EIO);
}

static void recv_cb(const void *data, size_t len, void *priv)
{
	struct log_multidomain_link *link_remote = priv;

	log_multidomain_link_on_recv_cb(link_remote, data, len);
}

static int link_ipc_service_send(struct log_multidomain_link *link_remote,
				 void *data, size_t len)
{
	struct log_link_ipc_service *link_ipc_service =
		CONTAINER_OF(link_remote, struct log_link_ipc_service, link_remote);

	return ipc_service_send(&link_ipc_service->ept, data, len);
}

static int link_ipc_service_init(struct log_multidomain_link *link_remote)
{
	struct log_link_ipc_service *link_ipc_service =
		CONTAINER_OF(link_remote, struct log_link_ipc_service, link_remote);
	static struct ipc_ept_cfg ept_cfg = {
		.name = "logging",
		.prio = 0,
		.cb = {
			.bound    = bound_cb,
			.received = recv_cb,
			.error    = error_cb,
		},
	};
	const struct device *ipc_instance = DEVICE_DT_GET(DT_CHOSEN(zephyr_log_ipc));
	int err;

	ept_cfg.priv = (void *)link_remote;
	err = ipc_service_open_instance(ipc_instance);
	if (err < 0 && err != -EALREADY) {
		__ASSERT(0, "ipc_service_open_instance() failure (err:%d)\n", err);
		return err;
	}

	err = ipc_service_register_endpoint(ipc_instance, &link_ipc_service->ept, &ept_cfg);

	return err;
}

struct log_multidomain_link_transport_api log_link_ipc_service_transport_api = {
	.init = link_ipc_service_init,
	.send = link_ipc_service_send
};

static struct log_link_ipc_service link_ipc_service_data = {
	.link_remote = {
		.transport_api = &log_link_ipc_service_transport_api
	}
};

LOG_LINK_DEF(link_ipc_service, log_multidomain_link_api,
	     CONFIG_LOG_LINK_IPC_SERVICE_BUFFER_SIZE,
	     &link_ipc_service_data.link_remote);

const struct log_link *log_link_ipc_get_link(void)
{
	return &link_ipc_service;
}
