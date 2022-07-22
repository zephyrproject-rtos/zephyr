/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/spinlock.h>

#include <cavs_ipc.h>
#include <cavs-ipc-regs.h>


void cavs_ipc_set_message_handler(const struct device *dev,
				  cavs_ipc_handler_t fn, void *arg)
{
	struct cavs_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->handle_message = fn;
	devdata->handler_arg = arg;
	k_spin_unlock(&devdata->lock, key);
}

void cavs_ipc_set_done_handler(const struct device *dev,
			       cavs_ipc_done_t fn, void *arg)
{
	struct cavs_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->done_notify = fn;
	devdata->done_arg = arg;
	k_spin_unlock(&devdata->lock, key);
}

void z_cavs_ipc_isr(const void *devarg)
{
	const struct device *dev = devarg;
	const struct cavs_ipc_config *config = dev->config;
	struct cavs_ipc_data *devdata = dev->data;

	volatile struct cavs_ipc *regs = config->regs;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if (regs->tdr & CAVS_IPC_BUSY) {
		bool done = true;

		if (devdata->handle_message != NULL) {
			uint32_t msg = regs->tdr & ~CAVS_IPC_BUSY;
			uint32_t ext = regs->tdd;

			done = devdata->handle_message(dev, devdata->handler_arg,
						       msg, ext);
		}

		regs->tdr = CAVS_IPC_BUSY;
		if (done && !IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
			regs->tda = CAVS_IPC_DONE;
		}
	}

	/* Same signal, but on different bits in 1.5 */
	bool done = IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15) ?
		(regs->idd & CAVS_IPC_IDD15_DONE) : (regs->ida & CAVS_IPC_DONE);

	if (done) {
		if (devdata->done_notify != NULL) {
			devdata->done_notify(dev, devdata->done_arg);
		}
		k_sem_give(&devdata->sem);
		if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
			regs->idd = CAVS_IPC_IDD15_DONE;
		} else {
			regs->ida = CAVS_IPC_DONE;
		}
	}

	k_spin_unlock(&devdata->lock, key);
}

int cavs_ipc_init(const struct device *dev)
{
	struct cavs_ipc_data *devdata = dev->data;
	const struct cavs_ipc_config *config = dev->config;

	memset(devdata, 0, sizeof(*devdata));

	/* ACK any latched interrupts (including TDA to clear IDA on
	 * the other side!), then enable.
	 */
	config->regs->tdr = CAVS_IPC_BUSY;
	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
		config->regs->idd = CAVS_IPC_IDD15_DONE;
	} else {
		config->regs->ida = CAVS_IPC_DONE;
		config->regs->tda = CAVS_IPC_DONE;
	}
	config->regs->ctl |= (CAVS_IPC_CTL_IDIE | CAVS_IPC_CTL_TBIE);
	return 0;
}

void cavs_ipc_complete(const struct device *dev)
{
	const struct cavs_ipc_config *config = dev->config;

	config->regs->tda = CAVS_IPC_DONE;
}

bool cavs_ipc_is_complete(const struct device *dev)
{
	const struct cavs_ipc_config *config = dev->config;

	return (config->regs->idr & CAVS_IPC_BUSY) == 0;
}

bool cavs_ipc_send_message(const struct device *dev,
			   uint32_t data, uint32_t ext_data)
{
	const struct cavs_ipc_config *config = dev->config;
	struct cavs_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if ((config->regs->idr & CAVS_IPC_BUSY) != 0) {
		k_spin_unlock(&devdata->lock, key);
		return false;
	}

	k_sem_init(&devdata->sem, 0, 1);
	config->regs->idd = ext_data;
	config->regs->idr = data | CAVS_IPC_BUSY;
	k_spin_unlock(&devdata->lock, key);
	return true;
}

bool cavs_ipc_send_message_sync(const struct device *dev,
				uint32_t data, uint32_t ext_data,
				k_timeout_t timeout)
{
	struct cavs_ipc_data *devdata = dev->data;

	bool ret = cavs_ipc_send_message(dev, data, ext_data);

	if (ret) {
		k_sem_take(&devdata->sem, timeout);
	}
	return ret;
}

#if DT_NODE_EXISTS(CAVS_HOST_DTNODE)
static int dt_init(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(CAVS_HOST_DTNODE), 0, z_cavs_ipc_isr, CAVS_HOST_DEV, 0);
	irq_enable(DT_IRQN(CAVS_HOST_DTNODE));
	return cavs_ipc_init(dev);
}

static const struct cavs_ipc_config ipc_host_config = {
	.regs = (void *)DT_REG_ADDR(CAVS_HOST_DTNODE),
};

static struct cavs_ipc_data ipc_host_data;

DEVICE_DT_DEFINE(CAVS_HOST_DTNODE, dt_init, NULL, &ipc_host_data, &ipc_host_config,
		 PRE_KERNEL_2, 0, NULL);
#endif
