/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/spinlock.h>

#include <intel_adsp_ipc.h>
#include <zephyr/ipc/backends/intel_adsp_host_ipc.h>

static struct ipc_ept intel_adsp_ipc_ept;
static struct intel_adsp_ipc_ept_priv_data intel_adsp_ipc_priv_data;
static struct ipc_ept_cfg intel_adsp_ipc_ept_cfg;

void intel_adsp_ipc_set_message_handler(const struct device *dev, intel_adsp_ipc_handler_t fn,
					void *arg)
{
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->handle_message = fn;
	devdata->handler_arg = arg;
	k_spin_unlock(&devdata->lock, key);
}

void intel_adsp_ipc_set_done_handler(const struct device *dev, intel_adsp_ipc_done_t fn, void *arg)
{
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->done_notify = fn;
	devdata->done_arg = arg;
	k_spin_unlock(&devdata->lock, key);
}

static void intel_adsp_ipc_receive_cb(const void *data, size_t len, void *priv)
{
	const struct device *dev = INTEL_ADSP_IPC_HOST_DEV;
	struct intel_adsp_ipc_data *devdata = dev->data;
	struct intel_adsp_ipc_ept_priv_data *priv_data =
		(struct intel_adsp_ipc_ept_priv_data *)priv;

	__ASSERT(len == sizeof(uint32_t) * 2, "Unexpected IPC payload length: %zu", len);
	__ASSERT(data != NULL, "IPC payload pointer is NULL");

	const uint32_t *msg = (const uint32_t *)data;

	if (devdata->handle_message != NULL) {
		priv_data->msg_done = devdata->handle_message(dev, devdata->handler_arg, msg[0],
							      msg[1]);
	}
}

void intel_adsp_ipc_complete(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret = ipc_service_release_rx_buffer(&intel_adsp_ipc_ept, NULL);

	ARG_UNUSED(ret);
	__ASSERT(ret == 0, "ipc_service_release_rx_buffer() failed: %d", ret);
}

bool intel_adsp_ipc_is_complete(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ipc_service_get_tx_buffer_size(&intel_adsp_ipc_ept) > 0;
}

int intel_adsp_ipc_send_message(const struct device *dev, uint32_t data, uint32_t ext_data)
{
	ARG_UNUSED(dev);
	uint32_t msg[2] = {data, ext_data};

	return ipc_service_send(&intel_adsp_ipc_ept, &msg, sizeof(msg));
}

/*
 * This helper sends an IPC message and then waits synchronously for completion using the backend
 * semaphore. It is currently only used by tests, SOF firmware does not rely on it.
 *
 * The long‑term plan is to either:
 * - remove this helper entirely,
 * - move the synchronous wait logic to the application layer (SOF), or
 * - extend the generic IPC service API with an explicit synchronous send primitive.
 *
 * Until that decision is made, the function is kept here only to support existing test code and
 * should not be used by new callers.
 */
int intel_adsp_ipc_send_message_sync(const struct device *dev, uint32_t data, uint32_t ext_data,
				     k_timeout_t timeout)
{
	uint32_t msg[2] = {data, ext_data};
	struct intel_adsp_ipc_data *devdata = dev->data;

	int ret = ipc_service_send(&intel_adsp_ipc_ept, &msg, sizeof(msg));

	if (ret < 0) {
		k_sem_take(&devdata->sem, timeout);
	}

	return ret;
}

void intel_adsp_ipc_send_message_emergency(const struct device *dev, uint32_t data,
					   uint32_t ext_data)
{
	int ret;
	uint32_t msg[2] = {data, ext_data};

	ret = ipc_service_send_critical(&intel_adsp_ipc_ept, &msg, sizeof(msg));

	ARG_UNUSED(ret);
}

static struct ipc_ept_cfg intel_adsp_ipc_ept_cfg = {
	.name = "intel_adsp_ipc_ept",
	.cb = {
		.received = intel_adsp_ipc_receive_cb,
	},
	.priv = (void *)&intel_adsp_ipc_priv_data,
};

static int intel_adsp_ipc_old_init(void)
{
	int ret;
	const struct device *ipc_dev = INTEL_ADSP_IPC_HOST_DEV;

	ret = ipc_service_register_endpoint(ipc_dev, &intel_adsp_ipc_ept, &intel_adsp_ipc_ept_cfg);

	return ret;
}

/* Backend is at PRE_KERNEL_2:0, so we need to init after that. */
SYS_INIT(intel_adsp_ipc_old_init, PRE_KERNEL_2, 1);
