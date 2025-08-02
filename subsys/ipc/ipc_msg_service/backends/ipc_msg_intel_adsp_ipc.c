/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_adsp_host_ipc

#include <errno.h>

#include <zephyr/irq.h>
#include <zephyr/ipc/ipc_msg_service.h>
#include <zephyr/ipc/ipc_msg_service_backend.h>
#include <zephyr/pm/state.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/spinlock.h>

#include <intel_adsp_ipc.h>
#include <adsp_ipc_regs.h>
#include <adsp_interrupt.h>

#include <zephyr/ipc/backends/ipc_msg_intel_adsp_ipc.h>

static inline void ace_ipc_intc_mask(void)
{
#if defined(CONFIG_SOC_SERIES_INTEL_ADSP_ACE)
	ACE_DINT[0].ie[ACE_INTL_HIPC] = ACE_DINT[0].ie[ACE_INTL_HIPC] & ~BIT(0);
#endif
}

static inline void ace_ipc_intc_unmask(void)
{
#if defined(CONFIG_SOC_SERIES_INTEL_ADSP_ACE)
	ACE_DINT[0].ie[ACE_INTL_HIPC] = BIT(0);
#endif
}

static void intel_adsp_ipc_isr(const void *devarg)
{
	const struct device *dev = devarg;
	const struct intel_adsp_ipc_config *config = dev->config;
	struct intel_adsp_ipc_data *devdata = dev->data;
	const struct ipc_msg_ept_cfg *ept_cfg = devdata->ept_cfg;

	volatile struct intel_adsp_ipc *regs = config->regs;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if (regs->tdr & INTEL_ADSP_IPC_BUSY) {
		bool done = true;

		if (ept_cfg->cb.received != NULL) {
			struct intel_adsp_ipc_msg cb_msg = {
				.data = regs->tdr & ~INTEL_ADSP_IPC_BUSY,
				.extdata = regs->tdd,
			};

			int ret = ept_cfg->cb.received(INTEL_ADSP_IPC_MSG, &cb_msg, ept_cfg->priv);

			done = (ret == 0);
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
	bool done = (regs->ida & INTEL_ADSP_IPC_DONE);

	if (done) {
		bool external_completion = false;

		if (ept_cfg->cb.event != NULL) {
			int ret = ept_cfg->cb.event(INTEL_ADSP_IPC_EVT_DONE, NULL, ept_cfg->priv);

			if (ret == INTEL_ADSP_IPC_EVT_RET_EXT_COMPLETE) {
				external_completion = true;
			}
		}

		devdata->tx_ack_pending = false;
		/* Allow the system to enter the runtime idle state after the IPC acknowledgment
		 * is received.
		 */
		pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
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

	k_sem_init(&devdata->sem, 0, 1);

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

static int intel_adsp_ipc_register_ept(const struct device *instance, void **token,
				       const struct ipc_msg_ept_cfg *cfg)
{
	struct intel_adsp_ipc_data *data = instance->data;

	data->ept_cfg = cfg;

	irq_enable(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE));
	ace_ipc_intc_unmask();

	return 0;
}

static int intel_adsp_ipc_deregister_ept(const struct device *instance, void *token)
{
	struct intel_adsp_ipc_data *data = instance->data;

	data->ept_cfg = NULL;

	ace_ipc_intc_mask();
	irq_disable(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE));

	return 0;
}

static void ipc_complete(const struct device *dev)
{
	const struct intel_adsp_ipc_config *config = dev->config;

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	config->regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
	config->regs->tda = INTEL_ADSP_IPC_DONE;
#endif
}

static bool ipc_is_complete(const struct device *dev)
{
	const struct intel_adsp_ipc_config *config = dev->config;
	const struct intel_adsp_ipc_data *devdata = dev->data;
	bool not_busy = (config->regs->idr & INTEL_ADSP_IPC_BUSY) == 0;

	return not_busy && !devdata->tx_ack_pending;
}

static int ipc_send_message(const struct device *dev, uint32_t data, uint32_t ext_data)
{
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state current_state;

	if (pm_device_state_get(INTEL_ADSP_IPC_HOST_DEV, &current_state) != 0 ||
	    current_state != PM_DEVICE_STATE_ACTIVE) {
		return -ESHUTDOWN;
	}
#endif

	pm_device_busy_set(dev);
	const struct intel_adsp_ipc_config *config = dev->config;
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if ((config->regs->idr & INTEL_ADSP_IPC_BUSY) != 0 || devdata->tx_ack_pending) {
		k_spin_unlock(&devdata->lock, key);
		return -EBUSY;
	}

	k_sem_reset(&devdata->sem);

	/* Prevent entering runtime idle state until IPC acknowledgment is received. */
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);

	devdata->tx_ack_pending = true;

	config->regs->idd = ext_data;
	config->regs->idr = data | INTEL_ADSP_IPC_BUSY;

	k_spin_unlock(&devdata->lock, key);

	pm_device_busy_clear(dev);

	return 0;
}

static int ipc_send_message_sync(const struct device *dev, uint32_t data, uint32_t ext_data,
				 k_timeout_t timeout)
{
	struct intel_adsp_ipc_data *devdata = dev->data;

	int ret = ipc_send_message(dev, data, ext_data);

	if (!ret) {
		k_sem_take(&devdata->sem, timeout);
	}

	return ret;
}

static int ipc_send_message_emergency(const struct device *dev, uint32_t data, uint32_t ext_data)
{
	const struct intel_adsp_ipc_config *const config = dev->config;

	volatile struct intel_adsp_ipc *const regs = config->regs;
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

	return 0;
}

static int intel_adsp_ipc_send(const struct device *dev, void *token, uint16_t msg_type,
			       const void *data)
{
	int ret;

	switch (msg_type) {
	case INTEL_ADSP_IPC_MSG: {
		const struct intel_adsp_ipc_msg *msg = (const struct intel_adsp_ipc_msg *)data;

		ret = ipc_send_message(dev, msg->data, msg->extdata);

		break;
	}
	case INTEL_ADSP_IPC_MSG_SYNC: {
		const struct intel_adsp_ipc_msg_sync *msg =
			(const struct intel_adsp_ipc_msg_sync *)data;

		ret = ipc_send_message_sync(dev, msg->data, msg->extdata, msg->timeout);

		break;
	}
	case INTEL_ADSP_IPC_MSG_EMERGENCY: {
		const struct intel_adsp_ipc_msg_emergency *msg =
			(const struct intel_adsp_ipc_msg_emergency *)data;

		ret = ipc_send_message_emergency(dev, msg->data, msg->extdata);

		break;
	}
	case INTEL_ADSP_IPC_MSG_DONE:
		ipc_complete(dev);

		ret = 0;

		break;
	default:
		ret = -EBADMSG;

		break;
	}

	return ret;
}

static int intel_adsp_ipc_query(const struct device *dev, void *token, uint16_t query_type,
				const void *query_data, void *query_response)
{
	int ret;

	ARG_UNUSED(query_data);
	ARG_UNUSED(query_response);

	switch (query_type) {
	case INTEL_ADSP_IPC_QUERY_IS_COMPLETE: {
		bool completed = ipc_is_complete(dev);

		if (completed) {
			ret = 0;
		} else {
			ret = -EAGAIN;
		}

		break;
	}
	default:
		ret = -ENOTSUP;

		break;
	}

	return ret;
}

static int intel_adsp_ipc_dt_init(const struct device *dev)
{
	struct intel_adsp_ipc_data *devdata = dev->data;

	memset(devdata, 0, sizeof(*devdata));

	IRQ_CONNECT(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE), 0, intel_adsp_ipc_isr,
		    INTEL_ADSP_IPC_HOST_DEV, 0);

	return intel_adsp_ipc_init(dev);
}

#ifdef CONFIG_PM_DEVICE

void intel_adsp_ipc_set_resume_handler(const struct device *dev, intel_adsp_ipc_resume_handler_t fn,
				       void *arg)
{
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->resume_fn = fn;
	devdata->resume_fn_args = arg;

	k_spin_unlock(&devdata->lock, key);
}

void intel_adsp_ipc_set_suspend_handler(const struct device *dev,
					intel_adsp_ipc_suspend_handler_t fn, void *arg)
{
	struct intel_adsp_ipc_data *devdata = dev->data;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	devdata->suspend_fn = fn;
	devdata->suspend_fn_args = arg;

	k_spin_unlock(&devdata->lock, key);
}

/**
 * @brief Manages IPC driver power state change.
 *
 * @param dev IPC device.
 * @param action Power state to be changed to.
 *
 * @return int Returns 0 on success or optionaly error code from the
 *             registered ipc_power_control_api callbacks.
 *
 * @note PM lock is taken at the start of each power transition to prevent concurrent calls
 *       to @ref pm_device_action_run function.
 *       If IPC Device performs hardware operation and device is busy (what should not happen)
 *       function returns failure. It is API user responsibility to make sure we are not entering
 *       device power transition while device is busy.
 */
static int ipc_pm_action(const struct device *dev, enum pm_device_action action)
{
	if (pm_device_is_busy(INTEL_ADSP_IPC_HOST_DEV)) {
		return -EBUSY;
	}

	struct intel_adsp_ipc_data *devdata = dev->data;

	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (devdata->suspend_fn) {
			ret = devdata->suspend_fn(dev, devdata->suspend_fn_args);
			if (!ret) {
				irq_disable(DT_IRQN(INTEL_ADSP_IPC_HOST_DTNODE));
			}
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
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
		if (devdata->resume_fn) {
			ret = devdata->resume_fn(dev, devdata->resume_fn_args);
		}
		break;
	default:
		/* Return as default value when given PM action is not supported */
		return -ENOTSUP;
	}

	return ret;
}

PM_DEVICE_DT_DEFINE(INTEL_ADSP_IPC_HOST_DTNODE, ipc_pm_action);

#endif /* CONFIG_PM_DEVICE */

static const struct intel_adsp_ipc_config ipc_host_config = {
	.regs = (void *)INTEL_ADSP_IPC_REG_ADDRESS,
};

static struct intel_adsp_ipc_data ipc_host_data;

const static struct ipc_msg_service_backend intel_adsp_ipc_backend_api = {
	.query = intel_adsp_ipc_query,
	.send = intel_adsp_ipc_send,
	.register_endpoint = intel_adsp_ipc_register_ept,
	.deregister_endpoint = intel_adsp_ipc_deregister_ept,
};

DEVICE_DT_DEFINE(INTEL_ADSP_IPC_HOST_DTNODE, intel_adsp_ipc_dt_init,
		 PM_DEVICE_DT_GET(INTEL_ADSP_IPC_HOST_DTNODE), &ipc_host_data, &ipc_host_config,
		 PRE_KERNEL_2, 0, &intel_adsp_ipc_backend_api);
