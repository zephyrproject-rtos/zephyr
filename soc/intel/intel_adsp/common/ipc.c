/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/spinlock.h>

#include <intel_adsp_ipc.h>
#include <zephyr/ipc/backends/ipc_msg_intel_adsp_ipc.h>

static struct ipc_msg_ept intel_adsp_ipc_ept;
static struct ipc_msg_ept_cfg intel_adsp_ipc_ept_cfg;

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

static int intel_adsp_ipc_receive_cb(uint16_t msg_type, const void *msg_data, void *priv)
{
	const struct device *dev = INTEL_ADSP_IPC_HOST_DEV;
	struct intel_adsp_ipc_data *devdata = dev->data;
	const struct intel_adsp_ipc_msg *msg = (const struct intel_adsp_ipc_msg *)msg_data;
	bool done = true;

	if (msg_type != INTEL_ADSP_IPC_MSG) {
		return -EBADMSG;
	}

	if (devdata->handle_message != NULL) {
		done = devdata->handle_message(dev, devdata->handler_arg, msg->data, msg->extdata);
	}

	if (done) {
		return 0;
	} else {
		return -EBADMSG;
	}
}

static int intel_adsp_ipc_event_cb(uint16_t evt_type, const void *evt_data, void *priv)
{
	const struct device *dev = INTEL_ADSP_IPC_HOST_DEV;
	struct intel_adsp_ipc_data *devdata = dev->data;
	bool external_completion = false;

	ARG_UNUSED(evt_data);

	if (evt_type != INTEL_ADSP_IPC_EVT_DONE) {
		return -EINVAL;
	}

	if (devdata->done_notify != NULL) {
		external_completion = devdata->done_notify(dev, devdata->done_arg);
	}

	if (external_completion) {
		return INTEL_ADSP_IPC_EVT_RET_EXT_COMPLETE;
	} else {
		return 0;
	}
}

void intel_adsp_ipc_complete(const struct device *dev)
{
	int ret;

	ret = ipc_msg_service_send(&intel_adsp_ipc_ept, INTEL_ADSP_IPC_MSG_DONE, NULL);

	ARG_UNUSED(ret);
}

bool intel_adsp_ipc_is_complete(const struct device *dev)
{
	int ret;

	ret = ipc_msg_service_query(&intel_adsp_ipc_ept, INTEL_ADSP_IPC_QUERY_IS_COMPLETE, NULL,
				    NULL);

	return ret == 0;
}

int intel_adsp_ipc_send_message(const struct device *dev, uint32_t data, uint32_t ext_data)
{
	struct intel_adsp_ipc_msg msg = {.data = data, .extdata = ext_data};
	int ret;

	ret = ipc_msg_service_send(&intel_adsp_ipc_ept, INTEL_ADSP_IPC_MSG, &msg);

	return ret;
}

int intel_adsp_ipc_send_message_sync(const struct device *dev, uint32_t data, uint32_t ext_data,
				     k_timeout_t timeout)
{
	struct intel_adsp_ipc_msg_sync msg = {
		.data = data, .extdata = ext_data, .timeout = timeout};
	int ret;

	ret = ipc_msg_service_send(&intel_adsp_ipc_ept, INTEL_ADSP_IPC_MSG_SYNC, &msg);

	return ret;
}

void intel_adsp_ipc_send_message_emergency(const struct device *dev, uint32_t data,
					   uint32_t ext_data)
{
	struct intel_adsp_ipc_msg_emergency msg = {.data = data, .extdata = ext_data};
	int ret;

	ret = ipc_msg_service_send(&intel_adsp_ipc_ept, INTEL_ADSP_IPC_MSG_EMERGENCY, &msg);

	ARG_UNUSED(ret);
}

static struct ipc_msg_ept_cfg intel_adsp_ipc_ept_cfg = {
	.name = "intel_adsp_ipc_ept",
	.cb = {
		.received = intel_adsp_ipc_receive_cb,
		.event = intel_adsp_ipc_event_cb,
	},
};

static int intel_adsp_ipc_old_init(void)
{
	int ret;
	const struct device *ipc_dev = INTEL_ADSP_IPC_HOST_DEV;

	ret = ipc_msg_service_register_endpoint(ipc_dev, &intel_adsp_ipc_ept,
						&intel_adsp_ipc_ept_cfg);

	return ret;
}

/* Backend is at PRE_KERNEL_2:0, so we need to init after that. */
SYS_INIT(intel_adsp_ipc_old_init, PRE_KERNEL_2, 1);
