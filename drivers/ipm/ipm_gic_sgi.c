/*
 * Copyright (c) 2022 openEuler
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_gic_sgi

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/interrupt_controller/gic.h>


/* Device config structure */
struct ipm_gic_sgi_device_config {
	void (*irq_config_func)(const struct device *d);
	int intno;
};

/* Device data structure */
struct ipm_gic_sgi_data {
	ipm_callback_t callback;
	void *user_data;
};


static int gic_sgi_send(const struct device *d, int wait, uint32_t id,
			  const void *data, int size)
{
	/* use SGI x to send ipi */
	const uint64_t mpidr = GET_MPIDR();
	const struct ipm_gic_sgi_device_config *config = d->config;
	/*
	 * Send SGI to all cores except itself
	 * Note: Assume only one Cluster now.
	 */
	gic_raise_sgi(config->intno, mpidr, SGIR_TGT_MASK & ~(1 << MPIDR_TO_CORE(mpidr)));

	return 0;
}

static uint32_t gic_sgi_max_id_val_get(const struct device *d)
{
	ARG_UNUSED(d);

	return 0;
}

static int gic_sgi_init(const struct device *d)
{
	const struct ipm_gic_sgi_device_config *config = d->config;

	/* just register sgi into, no special init */
	config->irq_config_func(d);

	return 0;
}

static void gic_sgi_isr(const struct device *d)
{
	struct ipm_gic_sgi_data *driver_data = d->data;

	if (driver_data->callback) {
		driver_data->callback(d, driver_data->user_data, 0, 0);
	}
}

static int gic_sgi_set_enabled(const struct device *d, int enable)
{
	ARG_UNUSED(d);
	ARG_UNUSED(enable);

	/* already enabled in gic_sgi_config_func */
	return 0;
}

static int gic_sgi_max_data_size_get(const struct device *d)
{
	ARG_UNUSED(d);

	/* sgi is just IPI, no data can be sent */
	return 0;
}

static void gic_sgi_register_cb(const struct device *d,
				ipm_callback_t cb,
				void *user_data)
{
	struct ipm_gic_sgi_data *driver_data = d->data;

	driver_data->callback = cb;
	driver_data->user_data = user_data;
}

static const struct ipm_driver_api gic_sgi_driver_api = {
	.send = gic_sgi_send,
	.register_callback = gic_sgi_register_cb,
	.max_data_size_get = gic_sgi_max_data_size_get,
	.max_id_val_get = gic_sgi_max_id_val_get,
	.set_enabled = gic_sgi_set_enabled,
};

static void gic_sgi_irq_config_func_0(const struct device *d);

static const struct ipm_gic_sgi_device_config gic_sgi_cfg_0 = {
	.irq_config_func = gic_sgi_irq_config_func_0,
	.intno = CONFIG_IPM_GIC_SGI_INTNO,
};

static struct ipm_gic_sgi_data gic_sgi_data_0 = {
	.callback = NULL,
	.user_data = NULL,
};

DEVICE_DT_INST_DEFINE(0,
			&gic_sgi_init,
			NULL,
			&gic_sgi_data_0,
			&gic_sgi_cfg_0, PRE_KERNEL_1,
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&gic_sgi_driver_api);

static void gic_sgi_irq_config_func_0(const struct device *d)
{
	ARG_UNUSED(d);
	IRQ_CONNECT(CONFIG_IPM_GIC_SGI_INTNO,
			IRQ_DEFAULT_PRIORITY,
			gic_sgi_isr,
			DEVICE_DT_INST_GET(0),
			0);
	irq_enable(CONFIG_IPM_GIC_SGI_INTNO);
}
