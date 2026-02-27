/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ADC_ADC_CONTEXT_DECLARE_H_
#define ZEPHYR_DRIVERS_ADC_ADC_CONTEXT_DECLARE_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

struct adc_context {
	atomic_t sampling_requested;
#ifdef ADC_CONTEXT_USES_KERNEL_TIMER
	struct k_timer timer;
#endif /* ADC_CONTEXT_USES_KERNEL_TIMER */

	struct k_sem lock;
	struct k_sem sync;
	int status;

#ifdef CONFIG_ADC_ASYNC
	struct k_poll_signal *signal;
	bool asynchronous;
#endif /* CONFIG_ADC_ASYNC */

	struct adc_sequence sequence;
	struct adc_sequence_options options;
	uint16_t sampling_index;
};

#ifdef ADC_CONTEXT_USES_KERNEL_TIMER
#define ADC_CONTEXT_INIT_TIMER(_data, _ctx_name) \
	._ctx_name.timer = Z_TIMER_INITIALIZER(_data._ctx_name.timer, \
						adc_context_on_timer_expired, \
						NULL)
#endif /* ADC_CONTEXT_USES_KERNEL_TIMER */

#define ADC_CONTEXT_INIT_LOCK(_data, _ctx_name) \
	._ctx_name.lock = Z_SEM_INITIALIZER(_data._ctx_name.lock, 0, 1)

#define ADC_CONTEXT_INIT_SYNC(_data, _ctx_name) \
	._ctx_name.sync = Z_SEM_INITIALIZER(_data._ctx_name.sync, 0, 1)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ADC_ADC_CONTEXT_DECLARE_H_ */
