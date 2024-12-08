/*
 * Copyright (c) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_sedi_ipm

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/ipm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipm_sedi, CONFIG_IPM_LOG_LEVEL);

#include "ipm_sedi.h"

extern void sedi_ipc_isr(IN sedi_ipc_t ipc_device);

static void set_ipm_dev_busy(const struct device *dev, bool is_write)
{
	struct ipm_sedi_context *ipm = dev->data;
	unsigned int key = irq_lock();

	atomic_set_bit(&ipm->status, is_write ? IPM_WRITE_BUSY_BIT : IPM_READ_BUSY_BIT);
	pm_device_busy_set(dev);
	irq_unlock(key);
}

static void clear_ipm_dev_busy(const struct device *dev, bool is_write)
{
	struct ipm_sedi_context *ipm = dev->data;
	unsigned int key = irq_lock();

	atomic_clear_bit(&ipm->status, is_write ? IPM_WRITE_BUSY_BIT : IPM_READ_BUSY_BIT);
	if ((!atomic_test_bit(&ipm->status, IPM_WRITE_BUSY_BIT))
		&& (!atomic_test_bit(&ipm->status, IPM_READ_BUSY_BIT))) {
		pm_device_busy_clear(dev);
	}
	irq_unlock(key);
}

static void ipm_event_dispose(IN sedi_ipc_t device, IN uint32_t event, INOUT void *params)
{
	const struct device *dev = (const struct device *)params;
	struct ipm_sedi_context *ipm = dev->data;
	uint32_t drbl_in = 0, len;

	LOG_DBG("dev: %u, event: %u", device, event);
	switch (event) {
	case SEDI_IPC_EVENT_MSG_IN:
		if (ipm->rx_msg_notify_cb != NULL) {
			set_ipm_dev_busy(dev, false);
			sedi_ipc_read_dbl(device, &drbl_in);
			len = IPC_HEADER_GET_LENGTH(drbl_in);
			sedi_ipc_read_msg(device, ipm->incoming_data_buf, len);
			ipm->rx_msg_notify_cb(dev,
					      ipm->rx_msg_notify_cb_data,
					      drbl_in, ipm->incoming_data_buf);
		} else {
			LOG_WRN("no handler for ipm new msg");
		}
		break;
	case SEDI_IPC_EVENT_MSG_PEER_ACKED:
		if (atomic_test_bit(&ipm->status, IPM_WRITE_IN_PROC_BIT)) {
			k_sem_give(&ipm->device_write_msg_sem);
		} else {
			LOG_WRN("no sending in progress, got an ack");
		}
		break;
	default:
		return;
	}
}

static int ipm_init(const struct device *dev)
{
	/* allocate resource and context*/
	const struct ipm_sedi_config_t *info = dev->config;
	sedi_ipc_t device = info->ipc_device;
	struct ipm_sedi_context *ipm = dev->data;

	info->irq_config();
	k_sem_init(&ipm->device_write_msg_sem, 0, 1);
	k_mutex_init(&ipm->device_write_lock);
	ipm->status = 0;

	sedi_ipc_init(device, ipm_event_dispose, (void *)dev);
	atomic_set_bit(&ipm->status, IPM_PEER_READY_BIT);
	LOG_DBG("ipm driver initialized on device: %p", dev);
	return 0;
}

static int ipm_send_isr(const struct device *dev,
			 uint32_t drbl,
			 const void *msg,
			 int msg_size)
{
	const struct ipm_sedi_config_t *info = dev->config;
	sedi_ipc_t device = info->ipc_device;
	uint32_t drbl_acked = 0;

	sedi_ipc_write_msg(device, (uint8_t *)msg,
			   (uint32_t)msg_size);
	sedi_ipc_write_dbl(device, drbl);
	do {
		sedi_ipc_read_ack_drbl(device, &drbl_acked);
	} while ((drbl_acked & BIT(IPC_BUSY_BIT)) == 0);

	return 0;
}

static int ipm_sedi_send(const struct device *dev,
		     int wait,
		     uint32_t drbl,
		     const void *msg,
		     int msg_size)
{
	__ASSERT((dev != NULL), "bad params\n");
	const struct ipm_sedi_config_t *info = dev->config;
	struct ipm_sedi_context *ipm = dev->data;
	sedi_ipc_t device = info->ipc_device;
	int ret, sedi_ret;

	/* check params, check status */
	if ((msg_size > IPC_DATA_LEN_MAX) || ((msg_size > 0) && (msg == NULL)) ||
	    ((drbl & BIT(IPC_BUSY_BIT)) == 0)) {
		LOG_ERR("bad params when sending ipm msg on device: %p", dev);
		return -EINVAL;
	}

	if (wait == 0) {
		LOG_ERR("not support no wait mode when sending ipm msg");
		return -ENOTSUP;
	}

	if (k_is_in_isr()) {
		return ipm_send_isr(dev, drbl, msg, msg_size);
	}

	k_mutex_lock(&ipm->device_write_lock, K_FOREVER);
	set_ipm_dev_busy(dev, true);

	if (!atomic_test_bit(&ipm->status, IPM_PEER_READY_BIT)) {
		LOG_WRN("peer is not ready");
		ret = -EBUSY;
		goto write_err;
	}

	/* write data regs */
	if (msg_size > 0) {
		sedi_ret = sedi_ipc_write_msg(device, (uint8_t *)msg,
					 (uint32_t)msg_size);
		if (sedi_ret != SEDI_DRIVER_OK) {
			LOG_ERR("ipm write data fail on device: %p", dev);
			ret = -EBUSY;
			goto write_err;
		}
	}

	atomic_set_bit(&ipm->status, IPM_WRITE_IN_PROC_BIT);
	/* write drbl regs to interrupt peer*/
	sedi_ret = sedi_ipc_write_dbl(device, drbl);

	if (sedi_ret != SEDI_DRIVER_OK) {
		LOG_ERR("ipm write doorbell fail on device: %p", dev);
		ret = -EBUSY;
		goto func_out;
	}

	/* wait for busy-bit-consumed interrupt */
	ret = k_sem_take(&ipm->device_write_msg_sem, K_MSEC(IPM_TIMEOUT_MS));
	if (ret) {
		LOG_WRN("ipm write timeout on device: %p", dev);
		sedi_ipc_write_dbl(device, 0);
	}

func_out:
	atomic_clear_bit(&ipm->status, IPM_WRITE_IN_PROC_BIT);

write_err:
	clear_ipm_dev_busy(dev, true);
	k_mutex_unlock(&ipm->device_write_lock);
	if (ret == 0) {
		LOG_DBG("ipm wrote a new message on device: %p, drbl=%08x",
			dev, drbl);
	}
	return ret;
}

static void ipm_sedi_register_callback(const struct device *dev, ipm_callback_t cb,
			  void *user_data)
{
	__ASSERT((dev != NULL), "bad params\n");

	struct ipm_sedi_context *ipm = dev->data;

	if (cb == NULL) {
		LOG_ERR("bad params when add ipm callback on device: %p", dev);
		return;
	}

	if (ipm->rx_msg_notify_cb == NULL) {
		ipm->rx_msg_notify_cb = cb;
		ipm->rx_msg_notify_cb_data = user_data;
	} else {
		LOG_ERR("ipm rx callback already exists on device: %p", dev);
	}
}

static void ipm_sedi_complete(const struct device *dev)
{
	int ret;

	__ASSERT((dev != NULL), "bad params\n");

	const struct ipm_sedi_config_t *info = dev->config;
	sedi_ipc_t device = info->ipc_device;

	ret = sedi_ipc_send_ack_drbl(device, 0);
	if (ret != SEDI_DRIVER_OK) {
		LOG_ERR("ipm send ack drl fail on device: %p", dev);
	}

	clear_ipm_dev_busy(dev, false);
}

static int ipm_sedi_get_max_data_size(const struct device *ipmdev)
{
	ARG_UNUSED(ipmdev);
	return IPC_DATA_LEN_MAX;
}

static uint32_t ipm_sedi_get_max_id(const struct device *ipmdev)
{
	ARG_UNUSED(ipmdev);
	return UINT32_MAX;
}

static int ipm_sedi_set_enable(const struct device *dev, int enable)
{
	__ASSERT((dev != NULL), "bad params\n");

	const struct ipm_sedi_config_t *info = dev->config;

	if (enable) {
		irq_enable(info->irq_num);
	} else {
		irq_disable(info->irq_num);
	}
	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int ipm_power_ctrl(const struct device *dev,
			  enum pm_device_action action)
{
	return 0;
}
#endif

static DEVICE_API(ipm, ipm_funcs) = {
	.send = ipm_sedi_send,
	.register_callback = ipm_sedi_register_callback,
	.max_data_size_get = ipm_sedi_get_max_data_size,
	.max_id_val_get = ipm_sedi_get_max_id,
	.complete = ipm_sedi_complete,
	.set_enabled = ipm_sedi_set_enable
};

#define IPM_SEDI_DEV_DEFINE(n)						\
	static struct ipm_sedi_context ipm_data_##n;			\
	static void ipm_##n##_irq_config(void);				\
	static const struct ipm_sedi_config_t ipm_config_##n = {	\
		.ipc_device = DT_INST_PROP(n, peripheral_id),		\
		.irq_num = DT_INST_IRQN(n),				\
		.irq_config = ipm_##n##_irq_config,			\
	};								\
	static void ipm_##n##_irq_config(void)				\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority), sedi_ipc_isr,	\
			    DT_INST_PROP(n, peripheral_id),		\
			    DT_INST_IRQ(n, sense));			\
	}								\
	PM_DEVICE_DT_DEFINE(DT_NODELABEL(ipm##n), ipm_power_ctrl);	\
	DEVICE_DT_INST_DEFINE(n,					\
			      &ipm_init,				\
			      PM_DEVICE_DT_GET(DT_NODELABEL(ipm##n)),   \
			      &ipm_data_##n,				\
			      &ipm_config_##n,				\
			      POST_KERNEL,				\
			      0,					\
			      &ipm_funcs);

DT_INST_FOREACH_STATUS_OKAY(IPM_SEDI_DEV_DEFINE)
