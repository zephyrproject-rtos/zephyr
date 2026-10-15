/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_multidomain_helper.h>
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

struct log_backend_ipc_service {
	struct ipc_ept ept;
	struct log_multidomain_backend backend_remote;
};

static void bound_cb(void *priv)
{
	struct log_multidomain_backend *backend_remote = priv;

	log_multidomain_backend_on_started(backend_remote, 0);
}

static void error_cb(const char *message, void *priv)
{
	struct log_multidomain_backend *backend_remote = priv;

	log_multidomain_backend_on_error(backend_remote, -EIO);
}

static void recv_cb(const void *data, size_t len, void *priv)
{
	struct log_multidomain_backend *backend_remote = priv;

	log_multidomain_backend_on_recv_cb(backend_remote, data, len);
}

static int backend_ipc_service_send(struct log_multidomain_backend *backend_remote,
				 void *data, size_t len)
{
	struct log_backend_ipc_service *backend_ipc_service =
		CONTAINER_OF(backend_remote, struct log_backend_ipc_service, backend_remote);
	int err = ipc_service_send(&backend_ipc_service->ept, data, len);

	return err;
}

static int backend_ipc_service_init(struct log_multidomain_backend *backend_remote)
{
	struct log_backend_ipc_service *backend_ipc_service =
		CONTAINER_OF(backend_remote, struct log_backend_ipc_service, backend_remote);
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

	ept_cfg.priv = (void *)backend_remote;
	err = ipc_service_open_instance(ipc_instance);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	err = ipc_service_register_endpoint(ipc_instance, &backend_ipc_service->ept, &ept_cfg);

	return err;
}

struct log_multidomain_backend_transport_api log_backend_ipc_service_transport_api = {
	.init = backend_ipc_service_init,
	.send = backend_ipc_service_send
};

static struct log_backend_ipc_service backend_ipc_service_data = {
	.backend_remote = {
		.transport_api = &log_backend_ipc_service_transport_api
	}
};

LOG_BACKEND_DEFINE(backend_ipc_service,
		   log_multidomain_backend_api,
		   true,
		   &backend_ipc_service_data.backend_remote);
