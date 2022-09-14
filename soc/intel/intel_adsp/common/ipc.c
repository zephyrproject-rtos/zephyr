/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
 #include <zephyr/spinlock.h>

#include <intel_adsp_ipc.h>
#include <adsp_ipc_regs.h>
#include <adsp_interrupt.h>
#include <zephyr/irq.h>
#include <zephyr/pm/state.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <errno.h>

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
		if (done) {
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
			regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
			regs->tda = INTEL_ADSP_IPC_DONE;
#endif
		}
	}

	/* Same signal, but on different bits in 1.5 */
	bool done =  (regs->ida & INTEL_ADSP_IPC_DONE);

	if (done) {
		bool external_completion = false;

		if (devdata->done_notify != NULL) {
			external_completion = devdata->done_notify(dev, devdata->done_arg);
		}
		devdata->tx_ack_pending = false;
		k_sem_give(&devdata->sem);

		/* IPC completion registers will be set externally */
		if (external_completion) {
			k_spin_unlock(&devdata->lock, key);
			return;
		}

		regs->ida = INTEL_ADSP_IPC_DONE;
	}

	k_spin_unlock(&devdata->lock, key);
}

int intel_adsp_ipc_init(const struct device *dev)
{
	pm_device_busy_set(dev);
	struct intel_adsp_ipc_data *devdata = dev->data;
	const struct intel_adsp_ipc_config *config = dev->config;

	memset(devdata, 0, sizeof(*devdata));

	/* ACK any latched interrupts (including TDA to clear IDA on
	 * the other side!), then enable.
	 */
	config->regs->tdr = INTEL_ADSP_IPC_BUSY;
	config->regs->ida = INTEL_ADSP_IPC_DONE;
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	config->regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
	config->regs->tda = INTEL_ADSP_IPC_DONE;
#endif
	config->regs->ctl |= (INTEL_ADSP_IPC_CTL_IDIE | INTEL_ADSP_IPC_CTL_TBIE);
	pm_device_busy_clear(dev);

	return 0;
}

void intel_adsp_ipc_complete(const struct device *dev)
{
	const struct intel_adsp_ipc_config *config = dev->config;

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	config->regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
	config->regs->tda = INTEL_ADSP_IPC_DONE;
#endif
}

bool intel_adsp_ipc_is_complete(const struct device *dev)
{
	const struct intel_adsp_ipc_config *config = dev->config;
	const struct intel_adsp_ipc_data *devdata = dev->data;
	bool not_busy = (config->regs->idr & INTEL_ADSP_IPC_BUSY) == 0;

	return not_busy && !devdata->tx_ack_pending;
}

int intel_adsp_ipc_send_message(const struct device *dev,
			   uint32_t data, uint32_t ext_data)
{
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state current_state;

	if (pm_device_state_get(INTEL_ADSP_IPC_HOST_DEV, &current_state) != 0 ||
		current_state != PM_DEVICE_STATE_ACTIVE) {
		return -ESHUTDOWN;
	}
#endif

	if (pm_device_state_is_locked(INTEL_ADSP_IPC_HOST_DEV)) {
		return -EAGAIN;
	}

	pm_device_busy_set(dev);
	const struct intel_adsp_ipc_config *config = dev->config;
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if ((config->regs->idr & INTEL_ADSP_IPC_BUSY) != 0 || devdata->tx_ack_pending) {
		k_spin_unlock(&devdata->lock, key);
		return -EBUSY;
	}

	k_sem_init(&devdata->sem, 0, 1);
	devdata->tx_ack_pending = true;
	config->regs->idd = ext_data;
	config->regs->idr = data | INTEL_ADSP_IPC_BUSY;
	k_spin_unlock(&devdata->lock, key);
	pm_device_busy_clear(dev);
	return 0;
}

int intel_adsp_ipc_send_message_sync(const struct device *dev,
				uint32_t data, uint32_t ext_data,
				k_timeout_t timeout)
{
	struct intel_adsp_ipc_data *devdata = dev->data;

	int ret = intel_adsp_ipc_send_message(dev, data, ext_data);

	if (!ret) {
		k_sem_take(&devdata->sem, timeout);
	}
	return ret;
}

void intel_adsp_ipc_send_message_emergency(const struct device *dev, uint32_t data,
					   uint32_t ext_data)
{
	const struct intel_adsp_ipc_config * const config = dev->config;

	volatile struct intel_adsp_ipc * const regs = config->regs;
	bool done;

	/* check if host is processing message. */
	while (regs->idr & INTEL_ADSP_IPC_BUSY) {
		k_busy_wait(1);
	}

	/* check if host has pending acknowledge msg
	 * Same signal, but on different bits in 1.5
	 */
	done = regs->ida & INTEL_ADSP_IPC_DONE;
	if (done) {
		/* IPC completion */
		regs->ida = INTEL_ADSP_IPC_DONE;
	}

	regs->idd = ext_data;
	regs->idr = data | INTEL_ADSP_IPC_BUSY;
}

#if DT_NODE_EXISTS(INTEL_ADSP_IPC_HOST_DTNODE)

#if defined(CONFIG_SOC_SERIES_INTEL_ADSP_ACE)
static inline void ace_ipc_intc_unmask(void)
{
	ACE_DINT[0].ie[ACE_INTL_HIPC] = BIT(0);
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

#ifdef CONFIG_PM_DEVICE

void intel_adsp_ipc_set_resume_handler(const struct device *dev,
	intel_adsp_ipc_resume_handler_t fn, void *arg)
{
	struct ipc_control_driver_api *api =
		(struct ipc_control_driver_api *)dev->api;
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	api->resume_fn = fn;
	api->resume_fn_args = arg;

	k_spin_unlock(&devdata->lock, key);
}

void intel_adsp_ipc_set_suspend_handler(const struct device *dev,
	intel_adsp_ipc_suspend_handler_t fn, void *arg)
{
	struct ipc_control_driver_api *api =
		(struct ipc_control_driver_api *)dev->api;
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	api->suspend_fn = fn;
	api->suspend_fn_args = arg;

	k_spin_unlock(&devdata->lock, key);
}

/**
 * @brief Manages IPC driver power state change.
 *
 * @param dev IPC device.
 * @param action Power state to be changed to.
 * @return int Returns 0 on success or optionaly error code from the
 * registered ipc_power_control_api callbacks.
 *
 * @note PM lock is taken at the start of each power transition to prevent concurrent calls
 * to @ref pm_device_action_run function.
 * If IPC Device performs hardware operation and device is busy (what should not happen)
 * function returns failure. It is API user responsibility to make sure we are not entering
 * device power transition while device is busy.
 */
static int ipc_pm_action(const struct device *dev, enum pm_device_action action)
{
	if (pm_device_is_busy(INTEL_ADSP_IPC_HOST_DEV)) {
		return -EBUSY;
	}

	const struct ipc_control_driver_api *api =
		(const struct ipc_control_driver_api *)dev->api;

	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_state_lock(dev);
		if (api->suspend_fn) {
			ret = api->suspend_fn(dev, api->suspend_fn_args);
			if (!ret) {
				irq_disable(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE));
			}
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		pm_device_state_lock(dev);
		irq_enable(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE));
		if (!irq_is_enabled(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE))) {
			ret = -EINTR;
			break;
		}
		ace_ipc_intc_unmask();
		ret = intel_adsp_ipc_init(dev);
		if (ret) {
			break;
		}
		if (api->resume_fn) {
			ret = api->resume_fn(dev, api->resume_fn_args);
		}
		break;
	default:
		/* Return as default value when given PM action is not supported */
		return -ENOTSUP;
	}

	pm_device_state_unlock(dev);
	return ret;
}

/**
 * @brief Callback functions to be executed by Zephyr application
 * during IPC device suspend and resume.
 */
static struct ipc_control_driver_api ipc_power_control_api = {
	.resume_fn = NULL,
	.resume_fn_args = NULL,
	.suspend_fn = NULL,
	.suspend_fn_args = NULL
};

PM_DEVICE_DT_DEFINE(INTEL_ADSP_IPC_HOST_DTNODE, ipc_pm_action);

#endif /* CONFIG_PM_DEVICE */

static const struct intel_adsp_ipc_config ipc_host_config = {
	.regs = (void *)INTEL_ADSP_IPC_REG_ADDRESS,
};

static struct intel_adsp_ipc_data ipc_host_data;

DEVICE_DT_DEFINE(INTEL_ADSP_IPC_HOST_DTNODE, dt_init, PM_DEVICE_DT_GET(INTEL_ADSP_IPC_HOST_DTNODE),
	&ipc_host_data, &ipc_host_config, PRE_KERNEL_2, 0, COND_CODE_1(CONFIG_PM_DEVICE,
	(&ipc_power_control_api), (NULL)));

#endif /* DT_NODE_EXISTS(INTEL_ADSP_IPC_HOST_DTNODE) */
