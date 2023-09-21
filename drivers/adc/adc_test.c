/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real ADC driver. It is used to instantiate struct
 * devices for the "vnd,adc" devicetree compatible used in test code.
 */

#define DT_DRV_COMPAT vnd_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

static int vnd_adc_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	return -ENOTSUP;
}

static int vnd_adc_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	return -ENOTSUP;
}

#ifdef CONFIG_ADC_ASYNC
static int vnd_adc_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif

static const struct adc_driver_api vnd_adc_api = {
	.channel_setup = vnd_adc_channel_setup,
	.read = vnd_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = vnd_adc_read_async,
#endif
};

#define VND_ADC_INIT(n)						  \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL,	  \
			      POST_KERNEL,			  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &vnd_adc_api);

DT_INST_FOREACH_STATUS_OKAY(VND_ADC_INIT)
