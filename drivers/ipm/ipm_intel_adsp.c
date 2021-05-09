/*
 * Copyright (c) 2020, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_adsp_mailbox

#include <device.h>
#include <soc.h>
#include <drivers/ipm.h>

#include <platform/mailbox.h>
#include <platform/shim.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ipm_adsp, CONFIG_IPM_LOG_LEVEL);

/*
 * With IPM data might be transferred by using ID field for simple
 * messages or via shared memory. Following parameters specify maximum
 * values for ID and DATA.
 */
#define IPM_INTEL_ADSP_MAX_DATA_SIZE		256
#define IPM_INTEL_ADSP_MAX_ID_VAL		IPC_DIPCI_MSG_MASK

/* Mailbox ADSP -> Host */
#define IPM_INTEL_ADSP_MAILBOX_OUT		MAILBOX_DSPBOX_BASE
#define IPM_INTEL_ADSP_MAILBOX_OUT_SIZE	MAILBOX_DSPBOX_SIZE

BUILD_ASSERT(IPM_INTEL_ADSP_MAILBOX_OUT_SIZE >= IPM_INTEL_ADSP_MAX_DATA_SIZE);

/* Mailbox Host -> ADSP */
#define IPM_INTEL_ADSP_MAILBOX_IN		MAILBOX_HOSTBOX_BASE
#define IPM_INTEL_ADSP_MAILBOX_IN_SIZE	MAILBOX_HOSTBOX_SIZE

BUILD_ASSERT(IPM_INTEL_ADSP_MAILBOX_IN_SIZE >= IPM_INTEL_ADSP_MAX_DATA_SIZE);

struct ipm_adsp_config {
	void (*irq_config_func)(const struct device *dev);
};

struct ipm_adsp_data {
	ipm_callback_t callback;
	void *user_data;
};

static void ipm_adsp_isr(const struct device *dev)
{
	const struct ipm_adsp_data *data = dev->data;
	uint32_t dipcctl, dipcie, dipct;

	dipct = ipc_read(IPC_DIPCT);
	dipcie = ipc_read(IPC_DIPCIE);
	dipcctl = ipc_read(IPC_DIPCCTL);

	LOG_DBG("dipct 0x%x dipcie 0x%x dipcctl 0x%x", dipct, dipcie, dipcctl);

	/*
	 * DSP core has received a message from IPC initiator (HOST).
	 * Initiator set Doorbel mechanism (HIPCI_BUSY bit).
	 */
	if (dipct & IPC_DIPCT_BUSY && dipcctl & IPC_DIPCCTL_IPCTBIE) {
		/* Mask BUSY interrupt */
		ipc_write(IPC_DIPCCTL, dipcctl & ~IPC_DIPCCTL_IPCTBIE);

		if (data->callback) {
			SOC_DCACHE_INVALIDATE((void *)IPM_INTEL_ADSP_MAILBOX_IN,
					      IPM_INTEL_ADSP_MAILBOX_IN_SIZE);
			/* Use zero copy */
			data->callback(dev, data->user_data,
				       dipct & IPC_DIPCI_MSG_MASK,
				       (void *)IPM_INTEL_ADSP_MAILBOX_IN);
		}

		/*
		 * Clear BUSY indicating to the Host that the message is
		 * received, and DSP is ready to accept another message
		 */
		ipc_write(IPC_DIPCT, ipc_read(IPC_DIPCT) | IPC_DIPCT_BUSY);

		/* Unmask BUSY interrupts */
		ipc_write(IPC_DIPCCTL,
			  ipc_read(IPC_DIPCCTL) | IPC_DIPCCTL_IPCTBIE);
	}

	/*
	 * DSP Initiator DONE indicates that we got reply from HOST that message
	 * is received and we can send another message.
	 */
	if (dipcie & IPC_DIPCIE_DONE && dipcctl & IPC_DIPCCTL_IPCIDIE) {
		/* Mask DONE interrupt */
		ipc_write(IPC_DIPCCTL,
			  ipc_read(IPC_DIPCCTL) & ~IPC_DIPCCTL_IPCIDIE);

		/* Clear DONE bit, Notify HOST that operation is completed */
		ipc_write(IPC_DIPCIE,
			  ipc_read(IPC_DIPCIE) | IPC_DIPCIE_DONE);

		/* Unmask DONE interrupt */
		ipc_write(IPC_DIPCCTL,
			  ipc_read(IPC_DIPCCTL) | IPC_DIPCCTL_IPCIDIE);

		LOG_DBG("Not handled: IPC_DIPCCTL_IPCIDIE");

		/* TODO: implement queued message sending if needed */
	}
}

static int ipm_adsp_send(const struct device *dev, int wait, uint32_t id,
			 const void *data, int size)
{
	LOG_DBG("Send: id %d data %p size %d", id, data, size);
	LOG_HEXDUMP_DBG(data, size, "send");

	if (id > IPM_INTEL_ADSP_MAX_ID_VAL) {
		return -EINVAL;
	}

	if (size > IPM_INTEL_ADSP_MAX_DATA_SIZE) {
		return -EMSGSIZE;
	}

	if (wait) {
		while (ipc_read(IPC_DIPCI) & IPC_DIPCI_BUSY) {
		}
	} else {
		if (ipc_read(IPC_DIPCI) & IPC_DIPCI_BUSY) {
			LOG_DBG("Busy: previous message is not handled");
			return -EBUSY;
		}
	}

	memcpy((void *)IPM_INTEL_ADSP_MAILBOX_OUT, data, size);
	SOC_DCACHE_FLUSH((void *)IPM_INTEL_ADSP_MAILBOX_OUT, size);

	ipc_write(IPC_DIPCIE, 0);
	ipc_write(IPC_DIPCI, IPC_DIPCI_BUSY | id);

	return 0;
}

static void ipm_adsp_register_callback(const struct device *dev,
				       ipm_callback_t cb,
				       void *user_data)
{
	struct ipm_adsp_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;
}

static int ipm_adsp_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("dev %p", dev);

	return IPM_INTEL_ADSP_MAX_DATA_SIZE;
}

static uint32_t ipm_adsp_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("dev %p", dev);

	return IPM_INTEL_ADSP_MAX_ID_VAL;
}

static int ipm_adsp_set_enabled(const struct device *dev, int enable)
{
	LOG_DBG("dev %p", dev);

	/* enable IPC interrupts from host */
	ipc_write(IPC_DIPCCTL, IPC_DIPCCTL_IPCIDIE | IPC_DIPCCTL_IPCTBIE);

	return 0;
}

static int ipm_adsp_init(const struct device *dev)
{
	const struct ipm_adsp_config *config = dev->config;

	LOG_DBG("dev %p", dev);

	config->irq_config_func(dev);

	return 0;
}

static const struct ipm_driver_api ipm_adsp_driver_api = {
	.send = ipm_adsp_send,
	.register_callback = ipm_adsp_register_callback,
	.max_data_size_get = ipm_adsp_max_data_size_get,
	.max_id_val_get = ipm_adsp_max_id_val_get,
	.set_enabled = ipm_adsp_set_enabled,
};

static void ipm_adsp_config_func(const struct device *dev);

static const struct ipm_adsp_config ipm_adsp_config = {
	.irq_config_func = ipm_adsp_config_func,
};

static struct ipm_adsp_data ipm_adsp_data;

DEVICE_DT_INST_DEFINE(0,
		    &ipm_adsp_init,
		    device_pm_control_nop,
		    &ipm_adsp_data, &ipm_adsp_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &ipm_adsp_driver_api);

static void ipm_adsp_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    ipm_adsp_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
