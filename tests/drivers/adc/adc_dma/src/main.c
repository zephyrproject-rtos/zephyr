/*
 * Copyright (c) 2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/zephyr.h>
#include <ztest.h>

extern const struct device *get_adc_device(void);
extern const struct device *get_count_device(void);
extern struct k_poll_signal async_sig;

void *adc_dma_setup(void)
{
	k_object_access_grant(get_adc_device(), k_current_get());
	k_object_access_grant(get_count_device(), k_current_get());
#ifdef CONFIG_ADC_ASYNC
	k_object_access_grant(&async_sig, k_current_get());
	k_poll_signal_init(&async_sig);
	k_thread_system_pool_assign(k_current_get());
#endif

	return NULL;
}

ZTEST_SUITE(adc_dma, NULL, adc_dma_setup, NULL, NULL, NULL);
