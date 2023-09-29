/*
 * Copyright (c) 2017-2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_mailbox

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <fsl_mailbox.h>
#include <fsl_clock.h>
#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>

#define MCUX_IPM_DATA_REGS 1
#define MCUX_IPM_MAX_ID_VAL 0

#if (defined(LPC55S69_cm33_core0_SERIES) || defined(LPC55S69_cm33_core1_SERIES))
#ifdef LPC55S69_cm33_core0_SERIES
#define MAILBOX_ID_THIS_CPU kMAILBOX_CM33_Core0
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM33_Core1
#else
#define MAILBOX_ID_THIS_CPU kMAILBOX_CM33_Core1
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM33_Core0
#endif
#else
#if defined(__CM4_CMSIS_VERSION)
#define MAILBOX_ID_THIS_CPU kMAILBOX_CM4
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM0Plus
#else
#define MAILBOX_ID_THIS_CPU kMAILBOX_CM0Plus
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM4
#endif
#endif

struct mcux_mailbox_config {
	MAILBOX_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_mailbox_data {
	ipm_callback_t callback;
	void *callback_ctx;
};

static void mcux_mailbox_isr(const struct device *dev)
{
	struct mcux_mailbox_data *data = dev->data;
	const struct mcux_mailbox_config *config = dev->config;
	mailbox_cpu_id_t cpu_id;

	cpu_id = MAILBOX_ID_THIS_CPU;

	volatile uint32_t value = MAILBOX_GetValue(config->base, cpu_id);

	__ASSERT(value, "spurious MAILBOX interrupt");

	/* Clear or the interrupt gets called intermittently */
	MAILBOX_ClearValueBits(config->base, cpu_id, value);

	if (data->callback) {
		/* Only one MAILBOX, id is unused and set to 0 */
		data->callback(dev, data->callback_ctx, 0, &value);
	}
	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
	 * Store immediate overlapping exception return operation
	 * might vector to incorrect interrupt
	 */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
	barrier_dsync_fence_full();
#endif
}


static int mcux_mailbox_ipm_send(const struct device *d, int wait,
				 uint32_t id,
				 const void *data, int size)
{
	const struct mcux_mailbox_config *config = d->config;
	MAILBOX_Type *base = config->base;
	/* Until we change API to uint32_t array */
	uint32_t data32[MCUX_IPM_DATA_REGS] = {0};
	unsigned int flags;
	int i;

	ARG_UNUSED(wait);

	if (id > MCUX_IPM_MAX_ID_VAL) {
		return -EINVAL;
	}

	if ((size < 0) || (size > MCUX_IPM_DATA_REGS * sizeof(uint32_t))) {
		return -EMSGSIZE;
	}

	flags = irq_lock();

	/* Actual message is passing using 32 bits registers */
	memcpy(data32, data, size);

	for (i = 0; i < ARRAY_SIZE(data32); ++i) {
		MAILBOX_SetValueBits(base, MAILBOX_ID_OTHER_CPU, data32[i]);
	}

	irq_unlock(flags);

	return 0;
}


static int mcux_mailbox_ipm_max_data_size_get(const struct device *d)
{
	ARG_UNUSED(d);
	/* Only a single 32-bit register available */
	return MCUX_IPM_DATA_REGS*sizeof(uint32_t);
}


static uint32_t mcux_mailbox_ipm_max_id_val_get(const struct device *d)
{
	ARG_UNUSED(d);
	/* Only a single instance of MAILBOX available for this platform */
	return MCUX_IPM_MAX_ID_VAL;
}

static void mcux_mailbox_ipm_register_callback(const struct device *d,
					       ipm_callback_t cb,
					       void *context)
{
	struct mcux_mailbox_data *driver_data = d->data;

	driver_data->callback = cb;
	driver_data->callback_ctx = context;
}


static int mcux_mailbox_ipm_set_enabled(const struct device *d, int enable)
{
	/* For now: nothing to be done */
	return 0;
}


static int mcux_mailbox_init(const struct device *dev)
{
	const struct mcux_mailbox_config *config = dev->config;

	MAILBOX_Init(config->base);
	config->irq_config_func(dev);
	return 0;
}

static const struct ipm_driver_api mcux_mailbox_driver_api = {
	.send = mcux_mailbox_ipm_send,
	.register_callback = mcux_mailbox_ipm_register_callback,
	.max_data_size_get = mcux_mailbox_ipm_max_data_size_get,
	.max_id_val_get = mcux_mailbox_ipm_max_id_val_get,
	.set_enabled = mcux_mailbox_ipm_set_enabled
};


/* Config MAILBOX 0 */

static void mcux_mailbox_config_func_0(const struct device *dev);

static const struct mcux_mailbox_config mcux_mailbox_0_config = {
	.base = (MAILBOX_Type *)DT_INST_REG_ADDR(0),
	.irq_config_func = mcux_mailbox_config_func_0,
};

static struct mcux_mailbox_data mcux_mailbox_0_data;

DEVICE_DT_INST_DEFINE(0,
		    &mcux_mailbox_init,
		    NULL,
		    &mcux_mailbox_0_data, &mcux_mailbox_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_mailbox_driver_api);


static void mcux_mailbox_config_func_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    mcux_mailbox_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
