/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
 #include <zephyr/spinlock.h>

#include <intel_adsp_ipc.h>
#include <adsp_ipc_regs.h>


void intel_adsp_ipc_set_message_handler(const struct device *dev,
	intel_adsp_ipc_handler_t fn, void *arg)
{
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->handle_message = fn;
	devdata->handler_arg = arg;
	k_spin_unlock(&devdata->lock, key);
}

void intel_adsp_ipc_set_done_handler(const struct device *dev,
	intel_adsp_ipc_done_t fn, void *arg)
{
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->done_notify = fn;
	devdata->done_arg = arg;
	k_spin_unlock(&devdata->lock, key);
}

void z_intel_adsp_ipc_isr(const void *devarg)
{
	const struct device *dev = devarg;
	const struct intel_adsp_ipc_config *config = dev->config;
	struct intel_adsp_ipc_data *devdata = dev->data;

	volatile struct intel_adsp_ipc *regs = config->regs;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if (regs->tdr & INTEL_ADSP_IPC_BUSY) {
		bool done = true;

		if (devdata->handle_message != NULL) {
			uint32_t msg = regs->tdr & ~INTEL_ADSP_IPC_BUSY;
			uint32_t ext = regs->tdd;

			done = devdata->handle_message(dev, devdata->handler_arg, msg, ext);
		}

		regs->tdr = INTEL_ADSP_IPC_BUSY;
		if (done && !IS_ENABLED(CONFIG_SOC_INTEL_CAVS_V15)) {
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
			regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
			regs->tda = INTEL_ADSP_IPC_DONE;
#endif
		}
	}

	/* Same signal, but on different bits in 1.5 */
	bool done = IS_ENABLED(CONFIG_SOC_INTEL_CAVS_V15) ?
		(regs->idd & INTEL_ADSP_IPC_DONE) : (regs->ida & INTEL_ADSP_IPC_DONE);

	if (done) {
		if (devdata->done_notify != NULL) {
			devdata->done_notify(dev, devdata->done_arg);
		}
		k_sem_give(&devdata->sem);
		if (IS_ENABLED(CONFIG_SOC_INTEL_CAVS_V15)) {
			regs->idd = INTEL_ADSP_IPC_DONE;
		} else {
			regs->ida = INTEL_ADSP_IPC_DONE;
		}
	}

	k_spin_unlock(&devdata->lock, key);
}

int intel_adsp_ipc_init(const struct device *dev)
{
	struct intel_adsp_ipc_data *devdata = dev->data;
	const struct intel_adsp_ipc_config *config = dev->config;

	memset(devdata, 0, sizeof(*devdata));

	/* ACK any latched interrupts (including TDA to clear IDA on
	 * the other side!), then enable.
	 */
	config->regs->tdr = INTEL_ADSP_IPC_BUSY;
	if (IS_ENABLED(CONFIG_SOC_INTEL_CAVS_V15)) {
		config->regs->idd = INTEL_ADSP_IPC_DONE;
	} else {
		config->regs->ida = INTEL_ADSP_IPC_DONE;
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
		config->regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
		config->regs->tda = INTEL_ADSP_IPC_DONE;
#endif
	}
	config->regs->ctl |= (INTEL_ADSP_IPC_CTL_IDIE | INTEL_ADSP_IPC_CTL_TBIE);
	return 0;
}

void intel_adsp_ipc_complete(const struct device *dev)
{
	const struct intel_adsp_ipc_config *config = dev->config;

#ifdef CONFIG_SOC_SERIES_INTEL_ACE
	config->regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
	config->regs->tda = INTEL_ADSP_IPC_DONE;
#endif
}

bool intel_adsp_ipc_is_complete(const struct device *dev)
{
	const struct intel_adsp_ipc_config *config = dev->config;

	return (config->regs->idr & INTEL_ADSP_IPC_BUSY) == 0;
}

bool intel_adsp_ipc_send_message(const struct device *dev,
			   uint32_t data, uint32_t ext_data)
{
	const struct intel_adsp_ipc_config *config = dev->config;
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if ((config->regs->idr & INTEL_ADSP_IPC_BUSY) != 0) {
		k_spin_unlock(&devdata->lock, key);
		return false;
	}

	k_sem_init(&devdata->sem, 0, 1);
	config->regs->idd = ext_data;
	config->regs->idr = data | INTEL_ADSP_IPC_BUSY;
	k_spin_unlock(&devdata->lock, key);
	return true;
}

bool intel_adsp_ipc_send_message_sync(const struct device *dev,
				uint32_t data, uint32_t ext_data,
				k_timeout_t timeout)
{
	struct intel_adsp_ipc_data *devdata = dev->data;

	bool ret = intel_adsp_ipc_send_message(dev, data, ext_data);

	if (ret) {
		k_sem_take(&devdata->sem, timeout);
	}
	return ret;
}

#if DT_NODE_EXISTS(INTEL_ADSP_IPC_HOST_DTNODE)

#if defined(CONFIG_SOC_SERIES_INTEL_ACE)
#include <ace_v1x-regs.h>

static inline void ace_ipc_intc_unmask(void)
{
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		MTL_DINT[i].ie[MTL_INTL_HIPC] = BIT(0);
	}
}
#else
static inline void ace_ipc_intc_unmask(void) {}
#endif

static int dt_init(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE), 0, z_intel_adsp_ipc_isr,
		INTEL_ADSP_IPC_HOST_DEV, 0);
	irq_enable(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE));

	ace_ipc_intc_unmask();

	return intel_adsp_ipc_init(dev);
}

static const struct intel_adsp_ipc_config ipc_host_config = {
	.regs = (void *)INTEL_ADSP_IPC_REG_ADDRESS,
};

static struct intel_adsp_ipc_data ipc_host_data;

DEVICE_DT_DEFINE(INTEL_ADSP_IPC_HOST_DTNODE, dt_init, NULL, &ipc_host_data, &ipc_host_config,
		 PRE_KERNEL_2, 0, NULL);
#endif
