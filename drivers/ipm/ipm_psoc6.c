/*
 * Copyright (c) 2018, Cypress Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <string.h>
#include <ipm.h>
#include <soc.h>
#include "cy_sysint.h"
#include "cy_ipc_drv.h"
#include "ipm_psoc6.h"


#if defined(CONFIG_SOC_PSOC6_M0)

#define PSOC6_IPM_SEND_CHANNEL DT_PSOC6_CM0_CM4_IPM_CHANNEL
#define PSOC6_IPM_RECV_CHANNEL DT_PSOC6_CM4_CM0_IPM_CHANNEL

#define PSOC6_IPM_LOCAL_INT_NUMB \
DT_CYPRESS_PSOC6_MAILBOX_0_IRQ_CM0_IPC_INT_NUMB
#define PSOC6_IPM_LOCAL_INT_PRIO \
DT_CYPRESS_PSOC6_MAILBOX_0_IRQ_CM0_MUX_IRQ_PRIORITY

#define PSOC6_IPM_REMOTE_INT_NUMB \
DT_CYPRESS_PSOC6_MAILBOX_0_IRQ_CM4_IPC_INT_NUMB

#define PSOC6_IPM_CM0_MUX_INT \
DT_CYPRESS_PSOC6_MAILBOX_PSOC6_IPM0_IRQ_CM0_MUX_IRQ

#define PSOC6_IPM_LOCAL_INT PSOC6_IPM_CM0_MUX_INT

#else /* defined(CONFIG_SOC_PSOC6_M0) */

#define PSOC6_IPM_SEND_CHANNEL DT_PSOC6_CM4_CM0_IPM_CHANNEL
#define PSOC6_IPM_RECV_CHANNEL DT_PSOC6_CM0_CM4_IPM_CHANNEL

#define PSOC6_IPM_LOCAL_INT_NUMB \
DT_CYPRESS_PSOC6_MAILBOX_0_IRQ_CM4_IPC_INT_NUMB
#define PSOC6_IPM_LOCAL_INT_PRIO \
DT_CYPRESS_PSOC6_MAILBOX_0_IRQ_CM4_IPC_INT_NUMB_PRIORITY

#define PSOC6_IPM_REMOTE_INT_NUMB \
DT_CYPRESS_PSOC6_MAILBOX_0_IRQ_CM0_IPC_INT_NUMB

#define PSOC6_IPM_CM0_MUX_INT \
DT_CYPRESS_PSOC6_MAILBOX_PSOC6_IPM0_IRQ_CM0_MUX_IRQ

#define PSOC6_IPM_LOCAL_INT (cpuss_interrupts_ipc_0_IRQn + \
PSOC6_IPM_LOCAL_INT_NUMB)

#endif /* defined(CONFIG_SOC_PSOC6_M0) */

struct psoc6_ipm_config_t {
	u32_t send_channel;
	u32_t recv_channel;
	u32_t local_int_numb;
	u32_t remote_int_numb;
	u32_t local_ipm_irq;
	s32_t ipm_irq_prio;
	s32_t cm0_ipm_irq_base;
};

struct psoc6_ipm_data {
	ipm_callback_t callback;
	void *callback_ctx;
};

static void psoc6_ipm_isr(void *arg)
{
	struct device *dev = arg;
	struct psoc6_ipm_data *data = dev->driver_data;
	const struct psoc6_ipm_config_t *config = dev->config->config_info;
	u32_t value;
	u32_t interrupt_masked;

	/*
	 * Check that there is really the IPC Notify interrupt,
	 * because the same line can be used for the IPC Release interrupt.
	 */
	interrupt_masked = Cy_IPC_Drv_ExtractAcquireMask(
		Cy_IPC_Drv_GetInterruptStatusMasked(
			Cy_IPC_Drv_GetIntrBaseAddr(config->local_int_numb)));

	if (interrupt_masked == (1uL << config->recv_channel)) {
		Cy_IPC_Drv_ClearInterrupt(
			Cy_IPC_Drv_GetIntrBaseAddr(config->local_int_numb),
				CY_IPC_NO_NOTIFICATION, interrupt_masked);

		if (Cy_IPC_Drv_ReadMsgWord(
			Cy_IPC_Drv_GetIpcBaseAddress(config->recv_channel),
				&value) == CY_IPC_DRV_SUCCESS) {
		/* Release the REE IPC channel with the Release interrupt */
			Cy_IPC_Drv_LockRelease(
				Cy_IPC_Drv_GetIpcBaseAddress(
				config->recv_channel),
				(1uL << config->remote_int_numb));

			if (data->callback) {
				data->callback(data->callback_ctx, 0, &value);
			}
		}
	}

	if ((interrupt_masked & (1uL << config->send_channel)) != 0u) {
		Cy_IPC_Drv_ClearInterrupt(
			Cy_IPC_Drv_GetIntrBaseAddr(config->local_int_numb),
			(1uL << config->send_channel), CY_IPC_NO_NOTIFICATION);
	}
}

static int psoc6_ipm_send(struct device *d, int wait, u32_t id,
			const void *data, int size)
{
	const struct psoc6_ipm_config_t *config = d->config->config_info;
	IPC_STRUCT_Type *ipc_base;
	cy_en_ipcdrv_status_t ipc_status;
	u32_t data32;
	int flags;

	ipc_base = Cy_IPC_Drv_GetIpcBaseAddress(config->send_channel);

	if (id > PSOC6_IPM_MAX_ID_VAL) {
		return -EINVAL;
	}

	if (size > sizeof(u32_t)) {
		return -EMSGSIZE;
	}

	/* Mutex is required to lock other tasks until we confirm that there are
	 * no errors
	 */
	flags = irq_lock();

	/* Attempt to acquire the IPC channel by reading the IPC_ACQUIRE
	 * register. If the channel was acquired, the REE has ownership of the
	 * channel for data transmission.
	 */
	ipc_status = Cy_IPC_Drv_LockAcquire(ipc_base);
	if (ipc_status != CY_IPC_DRV_SUCCESS) {
		irq_unlock(flags);
		return -EBUSY;
	}

	memcpy(&data32, data, size);
	Cy_IPC_Drv_WriteDataValue(ipc_base, data32);
	/* Generates a notify event on the TEE interrupt line.*/
	Cy_IPC_Drv_AcquireNotify(ipc_base, (1uL << config->remote_int_numb));

	irq_unlock(flags);

	if (wait) {
		/* Loop until remote clears the status bit */
		while (Cy_IPC_Drv_IsLockAcquired(ipc_base)) {
		}
	}

	return 0;
}

static int psoc6_ipm_max_data_size_get(struct device *d)
{
	ARG_UNUSED(d);

	return sizeof(u32_t);
}

static u32_t psoc6_ipm_max_id_val_get(struct device *d)
{
	ARG_UNUSED(d);

	return PSOC6_IPM_MAX_ID_VAL;
}

static void psoc6_ipm_register_callback(struct device *d,
						ipm_callback_t cb,
						void *context)
{
	struct psoc6_ipm_data *driver_data = d->driver_data;

	driver_data->callback = cb;
	driver_data->callback_ctx = context;
}

static int psoc6_ipm_set_enabled(struct device *d, int enable)
{
	return 0;
}

static int psoc6_ipm_init(struct device *dev)
{
	const struct psoc6_ipm_config_t *config = dev->config->config_info;

#if defined(CONFIG_SOC_PSOC6_M0)
	if (config->cm0_ipm_irq_base > SysTick_IRQn) {
		Cy_SysInt_SetInterruptSource(config->cm0_ipm_irq_base,
			(cy_en_intr_t)CY_IPC_INTR_NUM_TO_VECT(
				(int32_t)config->local_int_numb));
	}
#endif
	Cy_IPC_Drv_SetInterruptMask(
		Cy_IPC_Drv_GetIntrBaseAddr(config->local_int_numb),
		(1uL << config->send_channel), (1uL << config->recv_channel));

	irq_connect_dynamic(config->local_ipm_irq,
		config->ipm_irq_prio,
		psoc6_ipm_isr, dev, 0);

	irq_enable(config->local_ipm_irq);

	return 0;
}

const struct ipm_driver_api psoc6_ipm_api_funcs = {
	.send = psoc6_ipm_send,
	.register_callback = psoc6_ipm_register_callback,
	.max_data_size_get = psoc6_ipm_max_data_size_get,
	.max_id_val_get = psoc6_ipm_max_id_val_get,
	.set_enabled = psoc6_ipm_set_enabled
};

struct psoc6_ipm_config_t psoc6_ipm_config = {
	.send_channel = PSOC6_IPM_SEND_CHANNEL,
	.recv_channel = PSOC6_IPM_RECV_CHANNEL,
	.local_int_numb = PSOC6_IPM_LOCAL_INT_NUMB,
	.remote_int_numb = PSOC6_IPM_REMOTE_INT_NUMB,
	.local_ipm_irq = PSOC6_IPM_LOCAL_INT,
	.ipm_irq_prio = PSOC6_IPM_LOCAL_INT_PRIO,
	.cm0_ipm_irq_base = PSOC6_IPM_CM0_MUX_INT
};

struct psoc6_ipm_data psoc6_ipm_data;

DEVICE_AND_API_INIT(mailbox_0, DT_CYPRESS_PSOC6_MAILBOX_PSOC6_IPM0_LABEL,
	psoc6_ipm_init,
	&psoc6_ipm_data,
	&psoc6_ipm_config,
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	&psoc6_ipm_api_funcs);
