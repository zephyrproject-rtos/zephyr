/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief IPC service backend for Intel Audio DSP host IPC
 *
 * @note When declaring struct ipc_ept_cfg, the field @p priv must point to a struct
 *       intel_adsp_ipc_ept_priv_data. This is used to pass backend private state between the ISR
 *       and the application callbacks.
 *
 * @note For sending messages and in the receive callback, the @p data and @p len arguments
 *       represent a fixed two-word IPC payload rather than a generic byte buffer. The @p data
 *       pointer must reference an array of two uint32_t values (header and extended payload) and
 *       @p len must be sizeof(uint32_t) * 2.
 */

#define DT_DRV_COMPAT intel_adsp_host_ipc

#include <errno.h>

#include <zephyr/irq.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/pm/state.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/spinlock.h>

#include <intel_adsp_ipc.h>
#include <adsp_ipc_regs.h>
#include <adsp_interrupt.h>

#include <zephyr/ipc/backends/intel_adsp_host_ipc.h>

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
	const struct ipc_ept_cfg *ept_cfg = devdata->ept_cfg;

	struct intel_adsp_ipc_ept_priv_data *priv_data =
		(struct intel_adsp_ipc_ept_priv_data *)ept_cfg->priv;

	volatile struct intel_adsp_ipc *regs = config->regs;
	k_spinlock_key_t key = k_spin_lock(&devdata->lock);

	if (regs->tdr & INTEL_ADSP_IPC_BUSY) {
		if (ept_cfg->cb.received != NULL) {
			uint32_t msg[2] = {(regs->tdr & ~INTEL_ADSP_IPC_BUSY), regs->tdd};

			ept_cfg->cb.received(&msg, sizeof(msg), ept_cfg->priv);
		}

		regs->tdr = INTEL_ADSP_IPC_BUSY;
		if (priv_data->msg_done) {
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
			regs->tda = INTEL_ADSP_IPC_ACE1X_TDA_DONE;
#else
			regs->tda = INTEL_ADSP_IPC_DONE;
#endif
			priv_data->msg_done = false;
		}
	}

	/* Same signal, but on different bits in ACE */
	if (regs->ida & INTEL_ADSP_IPC_DONE) {
		bool external_completion = false;

		if (devdata->done_notify != NULL) {
			external_completion = devdata->done_notify(dev, devdata->done_arg);
		}

		devdata->tx_ack_pending = false;
		/*
		 * Allow the system to enter the runtime idle state after the IPC acknowledgment
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
				       const struct ipc_ept_cfg *cfg)
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

/**
 * @brief Send an IPC message.
 *
 * This implements the backend send() hook used by ipc_service_send() for the Intel Audio DSP host
 * IPC.
 *
 * The @a data argument is expected to point to an array of two 32-bit words, where the first word
 * is the IPC header and the second word is the extended payload. The @a len argument must be
 * exactly sizeof(uint32_t) * 2 or the call is rejected.
 *
 * On success the function programs the IPC registers and starts transmission towards the host,
 * enforcing the normal BUSY and TX acknowledgment checks performed by ipc_send_message().
 *
 * @param[in] dev   Pointer to device struct.
 * @param[in] token Backend-specific token (unused).
 * @param[in] data  Pointer to two-word IPC payload.
 * @param[in] len   Size of the payload in bytes (must be 8).
 *
 * @return 0 on success, negative errno on failure.
 *         -EINVAL if @p dev is NULL.
 *         -EBADMSG if @p data is NULL or @p len is invalid.
 *         Propagates error codes from ipc_send_message().
 */
static int intel_adsp_ipc_send(const struct device *dev, void *token, const void *data, size_t len)
{
	ARG_UNUSED(token);

	if (dev == NULL) {
		return -EINVAL;
	}

	if (len != sizeof(uint32_t) * 2 || data == NULL) {
		return -EBADMSG;
	}

	const uint32_t *msg = (const uint32_t *)data;

	return ipc_send_message(dev, msg[0], msg[1]);
}

/*
 * Report the availability of the host IPC channel.
 *
 * This backend uses the TX buffer size query as a way to check whether the host is ready to
 * receive the next message. When the IPC channel is idle (no BUSY bit set and no pending TX
 * acknowledgment), a single “buffer” of two 32-bit words is considered available and the
 * function returns sizeof(uint32_t) * 2. When the channel is still busy, no buffer is
 * available and the function returns 0.
 *
 * Return values:
 * - -EINVAL if @p instance is NULL.
 * - sizeof(uint32_t) * 2 when the channel is ready for a new message.
 * - 0 when the previous message has not yet been fully processed.
 */
int intel_adsp_ipc_get_tx_buffer_size(const struct device *instance, void *token)
{
	ARG_UNUSED(token);

	if (instance == NULL) {
		return -EINVAL;
	}

	if (ipc_is_complete(instance)) {
		return sizeof(uint32_t) * 2;
	}

	return 0;
}

/*
 * This backend does not need to explicitly hold RX buffers because the IPC channel is effectively
 * held from the moment the message is received in the interrupt handler until the firmware
 * completes the handling. However, we must still provide a hold_rx_buffer() implementation so that
 * ipc_service_release_rx_buffer() can check both hold and release callbacks and allow the use of
 * ipc_service_release_rx_buffer() to notify the host that the channel is available again.
 */
int intel_adsp_ipc_hold_rx_buffer(const struct device *instance, void *token, void *data)
{
	ARG_UNUSED(instance);
	ARG_UNUSED(token);
	ARG_UNUSED(data);
	return -ENOTSUP;
}

int intel_adsp_ipc_release_rx_buffer(const struct device *instance, void *token, void *data)
{
	ARG_UNUSED(token);
	ARG_UNUSED(data);

	if (instance == NULL) {
		return -EINVAL;
	}

	ipc_complete(instance);
	return 0;
}

static int intel_adsp_ipc_send_critical(const struct device *dev, void *token,
					const void *data, size_t len)
{
	ARG_UNUSED(token);

	if (dev == NULL) {
		return -EINVAL;
	}

	if (len != sizeof(uint32_t) * 2 || data == NULL) {
		return -EBADMSG;
	}

	const uint32_t *msg = (const uint32_t *)data;

	return ipc_send_message_emergency(dev, msg[0], msg[1]);
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
		/* Clear tx_ack_pending to ensure driver is operational after resume.
		 * Structure now contains function pointers, so memset() cannot be used.
		 */
		devdata->tx_ack_pending = false;
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

const static struct ipc_service_backend intel_adsp_ipc_backend_api = {
	.send = intel_adsp_ipc_send,
	.register_endpoint = intel_adsp_ipc_register_ept,
	.deregister_endpoint = intel_adsp_ipc_deregister_ept,
	.get_tx_buffer_size = intel_adsp_ipc_get_tx_buffer_size,
	.hold_rx_buffer = intel_adsp_ipc_hold_rx_buffer,
	.release_rx_buffer = intel_adsp_ipc_release_rx_buffer,
	.send_critical = intel_adsp_ipc_send_critical,
};

DEVICE_DT_DEFINE(INTEL_ADSP_IPC_HOST_DTNODE, intel_adsp_ipc_dt_init,
		 PM_DEVICE_DT_GET(INTEL_ADSP_IPC_HOST_DTNODE), &ipc_host_data, &ipc_host_config,
		 PRE_KERNEL_2, 0, &intel_adsp_ipc_backend_api);
