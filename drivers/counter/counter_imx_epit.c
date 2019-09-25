/*
 * Copyright (c) 2018-2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/counter.h>
#include <device.h>
#include "clock_freq.h"
#include "epit.h"

#define COUNTER_MAX_RELOAD	0xFFFFFFFF

struct imx_epit_config {
	struct counter_config_info info;
	EPIT_Type *base;
	u16_t prescaler;
};

struct imx_epit_data {
	volatile counter_top_callback_t callback;
	volatile void *user_data;
};

static inline const struct imx_epit_config *get_epit_config(struct device *dev)
{
	return CONTAINER_OF(dev->config->config_info, struct imx_epit_config,
			    info);
}

static void imx_epit_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	EPIT_Type *base = get_epit_config(dev)->base;
	struct imx_epit_data *driver_data = dev->driver_data;

	EPIT_ClearStatusFlag(base);

	if (driver_data->callback != NULL) {
		driver_data->callback(dev, (void *)driver_data->user_data);
	}
}

static void imx_epit_init(struct device *dev)
{
	struct imx_epit_config *config = (struct imx_epit_config *)
							   get_epit_config(dev);
	EPIT_Type *base = config->base;
	epit_init_config_t epit_config = {
		.freeRun     = true,
		.waitEnable  = true,
		.stopEnable  = true,
		.dbgEnable   = true,
		.enableMode  = true
	};

	/* Adjust frequency in the counter configuration info */
	config->info.freq = get_epit_clock_freq(base)/(config->prescaler + 1U);

	EPIT_Init(base, &epit_config);
}

static int imx_epit_start(struct device *dev)
{
	EPIT_Type *base = get_epit_config(dev)->base;

	/* Set EPIT clock source */
	EPIT_SetClockSource(base, epitClockSourcePeriph);

	/* Set prescaler */
	EPIT_SetPrescaler(base, get_epit_config(dev)->prescaler);

	/* Start the counter */
	EPIT_Enable(base);

	return 0;
}

static int imx_epit_stop(struct device *dev)
{
	EPIT_Type *base = get_epit_config(dev)->base;

	/* Disable EPIT */
	EPIT_Disable(base);

	return 0;
}

static u32_t imx_epit_read(struct device *dev)
{
	EPIT_Type *base = get_epit_config(dev)->base;
	u32_t value;

	value = EPIT_GetCounterLoadValue(base) - EPIT_ReadCounter(base);

	return value;
}

static int imx_epit_set_top_value(struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	EPIT_Type *base = get_epit_config(dev)->base;
	struct imx_epit_data *driver_data = dev->driver_data;

	/* Disable EPIT Output Compare interrupt for consistency */
	EPIT_SetIntCmd(base, false);

	driver_data->callback = cfg->callback;
	driver_data->user_data = cfg->user_data;

	/* Set reload value and optionally counter to "ticks" */
	EPIT_SetOverwriteCounter(base,
				!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET));
	EPIT_SetCounterLoadValue(base, cfg->ticks);

	if (cfg->callback != NULL) {
		/* (Re)enable EPIT Output Compare interrupt */
		EPIT_SetIntCmd(base, true);
	}

	return 0;
}

static u32_t imx_epit_get_pending_int(struct device *dev)
{
	EPIT_Type *base = get_epit_config(dev)->base;

	return EPIT_GetStatusFlag(base) ? 1U : 0U;
}

static u32_t imx_epit_get_top_value(struct device *dev)
{
	EPIT_Type *base = get_epit_config(dev)->base;

	return EPIT_GetCounterLoadValue(base);
}

static u32_t imx_epit_get_max_relative_alarm(struct device *dev)
{
	return COUNTER_MAX_RELOAD;
}

static const struct counter_driver_api imx_epit_driver_api = {
	.start = imx_epit_start,
	.stop = imx_epit_stop,
	.read = imx_epit_read,
	.set_top_value = imx_epit_set_top_value,
	.get_pending_int = imx_epit_get_pending_int,
	.get_top_value = imx_epit_get_top_value,
	.get_max_relative_alarm = imx_epit_get_max_relative_alarm,
};

#define COUNTER_IMX_EPIT_DEVICE(idx)					       \
static int imx_epit_config_func_##idx(struct device *dev);		       \
static const struct imx_epit_config imx_epit_##idx##z_config = {		       \
	.info = {							       \
			.max_top_value = COUNTER_MAX_RELOAD,		       \
			.freq = 1U,					       \
			.flags = 0,					       \
			.channels = 0U,					       \
		},							       \
	.base = (EPIT_Type *)DT_COUNTER_IMX_EPIT_##idx##_BASE_ADDRESS,	       \
	.prescaler = DT_COUNTER_IMX_EPIT_##idx##_PRESCALER,		       \
};									       \
static struct imx_epit_data imx_epit_##idx##_data;			       \
DEVICE_AND_API_INIT(epit_##idx, DT_COUNTER_IMX_EPIT_##idx##_LABEL,	       \
		    &imx_epit_config_func_##idx,			       \
		    &imx_epit_##idx##_data, &imx_epit_##idx##z_config.info,     \
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	       \
		    &imx_epit_driver_api);				       \
static int imx_epit_config_func_##idx(struct device *dev)		       \
{									       \
	imx_epit_init(dev);						       \
	IRQ_CONNECT(DT_COUNTER_IMX_EPIT_##idx##_IRQ,			       \
		    DT_COUNTER_IMX_EPIT_##idx##_IRQ_PRI,		       \
		    imx_epit_isr, DEVICE_GET(epit_##idx), 0);		       \
	irq_enable(DT_COUNTER_IMX_EPIT_##idx##_IRQ);			       \
	return 0;							       \
}

#ifdef CONFIG_COUNTER_IMX_EPIT_1
COUNTER_IMX_EPIT_DEVICE(1);
#endif /* CONFIG_COUNTER_IMX_EPIT_1 */

#ifdef CONFIG_COUNTER_IMX_EPIT_2
COUNTER_IMX_EPIT_DEVICE(2);
#endif /* CONFIG_COUNTER_IMX_EPIT_2 */
