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
	bool done = true;

	if (len == INTEL_ADSP_IPC_CB_MSG) {
		const struct intel_adsp_ipc_msg *msg = (const struct intel_adsp_ipc_msg *)data;

		if (devdata->handle_message != NULL) {
			done = devdata->handle_message(dev, devdata->handler_arg, msg->data,
						       msg->ext_data);
		}

		if (done) {
			priv_data->cb_ret = INTEL_ADSP_IPC_CB_RET_OKAY;
		} else {
			priv_data->cb_ret = -EBADMSG;
		}
	} else if (len == INTEL_ADSP_IPC_CB_DONE) {
		bool external_completion = false;

		if (devdata->done_notify != NULL) {
			external_completion = devdata->done_notify(dev, devdata->done_arg);
		}

		if (external_completion) {
			priv_data->cb_ret = INTEL_ADSP_IPC_CB_RET_EXT_COMPLETE;
		} else {
			priv_data->cb_ret = INTEL_ADSP_IPC_CB_RET_OKAY;
		}
	}
}

void intel_adsp_ipc_complete(const struct device *dev)
{
	int ret;

	ret = ipc_service_send(&intel_adsp_ipc_ept, NULL, INTEL_ADSP_IPC_SEND_DONE);

	ARG_UNUSED(ret);
}

bool intel_adsp_ipc_is_complete(const struct device *dev)
{
	int ret;

	ret = ipc_service_send(&intel_adsp_ipc_ept, NULL, INTEL_ADSP_IPC_SEND_IS_COMPLETE);

	return ret == 0;
}

int intel_adsp_ipc_send_message(const struct device *dev, uint32_t data, uint32_t ext_data)
{
	struct intel_adsp_ipc_msg msg = {.data = data, .ext_data = ext_data};
	int ret;

	ret = ipc_service_send(&intel_adsp_ipc_ept, &msg, INTEL_ADSP_IPC_SEND_MSG);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

int intel_adsp_ipc_send_message_sync(const struct device *dev, uint32_t data, uint32_t ext_data,
				     k_timeout_t timeout)
{
	struct intel_adsp_ipc_msg msg = {
		.data = data,
		.ext_data = ext_data,
		.timeout = timeout,
	};
	int ret;

	ret = ipc_service_send(&intel_adsp_ipc_ept, &msg, INTEL_ADSP_IPC_SEND_MSG_SYNC);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

void intel_adsp_ipc_send_message_emergency(const struct device *dev, uint32_t data,
					   uint32_t ext_data)
{
	struct intel_adsp_ipc_msg msg = {.data = data, .ext_data = ext_data};
	int ret;

	ret = ipc_service_send(&intel_adsp_ipc_ept, &msg, INTEL_ADSP_IPC_SEND_MSG_EMERGENCY);

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
