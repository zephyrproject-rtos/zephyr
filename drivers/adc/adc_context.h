/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ADC_ADC_CONTEXT_H_
#define ZEPHYR_DRIVERS_ADC_ADC_CONTEXT_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

struct adc_context;

/*
 * Each driver should provide implementations of the following two functions:
 * - adc_context_start_sampling() that will be called when a sampling (of one
 *   or more channels, depending on the realized sequence) is to be started
 * - adc_context_update_buffer_pointer() that will be called when the sample
 *   buffer pointer should be prepared for writing of next sampling results,
 *   the "repeat_sampling" parameter indicates if the results should be written
 *   in the same place as before (when true) or as consecutive ones (otherwise).
 */
static void adc_context_start_sampling(struct adc_context *ctx);
static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling);
/*
 * If a given driver uses some dedicated hardware timer to trigger consecutive
 * samplings, it should implement also the following two functions. Otherwise,
 * it should define the ADC_CONTEXT_USES_KERNEL_TIMER macro to enable parts of
 * this module that utilize a standard kernel timer.
 */
static void adc_context_enable_timer(struct adc_context *ctx);
static void adc_context_disable_timer(struct adc_context *ctx);

/*
 * If a driver needs to do something after a context complete then
 * then this optional function can be overwritten. This will be called
 * after a sequence has ended, and *not* when restarted with ADC_ACTION_REPEAT.
 * To enable this function define ADC_CONTEXT_ENABLE_ON_COMPLETE.
 */
#ifdef ADC_CONTEXT_ENABLE_ON_COMPLETE
static void adc_context_on_complete(struct adc_context *ctx, int status);
#endif /* ADC_CONTEXT_ENABLE_ON_COMPLETE */

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

#ifdef ADC_CONTEXT_USES_KERNEL_TIMER
static void adc_context_on_timer_expired(struct k_timer *timer_id);
#endif

static inline void adc_context_init(struct adc_context *ctx)
{
#ifdef ADC_CONTEXT_USES_KERNEL_TIMER
	k_timer_init(&ctx->timer, adc_context_on_timer_expired, NULL);
#endif
	k_sem_init(&ctx->lock, 0, 1);
	k_sem_init(&ctx->sync, 0, 1);
}

static inline void adc_context_request_next_sampling(struct adc_context *ctx)
{
	if (atomic_inc(&ctx->sampling_requested) == 0) {
		adc_context_start_sampling(ctx);
	} else {
		/*
		 * If a sampling was already requested and was not finished yet,
		 * do not start another one from here, this will be done from
		 * adc_context_on_sampling_done() after the current sampling is
		 * complete. Instead, note this fact, and inform the user about
		 * it after the sequence is done.
		 */
		ctx->status = -EBUSY;
	}
}

#ifdef ADC_CONTEXT_USES_KERNEL_TIMER
static inline void adc_context_enable_timer(struct adc_context *ctx)
{
	k_timer_start(&ctx->timer, K_NO_WAIT, K_USEC(ctx->options.interval_us));
}

static inline void adc_context_disable_timer(struct adc_context *ctx)
{
	k_timer_stop(&ctx->timer);
}

static void adc_context_on_timer_expired(struct k_timer *timer_id)
{
	struct adc_context *ctx =
		CONTAINER_OF(timer_id, struct adc_context, timer);

	adc_context_request_next_sampling(ctx);
}
#endif /* ADC_CONTEXT_USES_KERNEL_TIMER */

static inline void adc_context_lock(struct adc_context *ctx,
				    bool asynchronous,
				    struct k_poll_signal *signal)
{
	k_sem_take(&ctx->lock, K_FOREVER);

#ifdef CONFIG_ADC_ASYNC
	ctx->asynchronous = asynchronous;
	ctx->signal = signal;
#endif /* CONFIG_ADC_ASYNC */
}

static inline void adc_context_release(struct adc_context *ctx, int status)
{
#ifdef CONFIG_ADC_ASYNC
	if (ctx->asynchronous && (status == 0)) {
		return;
	}
#endif /* CONFIG_ADC_ASYNC */

	k_sem_give(&ctx->lock);
}

static inline void adc_context_unlock_unconditionally(struct adc_context *ctx)
{
	if (!k_sem_count_get(&ctx->lock)) {
		k_sem_give(&ctx->lock);
	}
}

static inline int adc_context_wait_for_completion(struct adc_context *ctx)
{
#ifdef CONFIG_ADC_ASYNC
	if (ctx->asynchronous) {
		return 0;
	}
#endif /* CONFIG_ADC_ASYNC */

	k_sem_take(&ctx->sync, K_FOREVER);
	return ctx->status;
}

static inline void adc_context_complete(struct adc_context *ctx, int status)
{
#ifdef ADC_CONTEXT_ENABLE_ON_COMPLETE
	adc_context_on_complete(ctx, status);
#endif /* ADC_CONTEXT_ENABLE_ON_COMPLETE */

#ifdef CONFIG_ADC_ASYNC
	if (ctx->asynchronous) {
		if (ctx->signal) {
			k_poll_signal_raise(ctx->signal, status);
		}

		k_sem_give(&ctx->lock);
		return;
	}
#endif /* CONFIG_ADC_ASYNC */

	/*
	 * Override the status only when an error is signaled to this function.
	 * Please note that adc_context_request_next_sampling() might have set
	 * this field.
	 */
	if (status != 0) {
		ctx->status = status;
	}
	k_sem_give(&ctx->sync);
}

static inline void adc_context_start_read(struct adc_context *ctx,
					  const struct adc_sequence *sequence)
{
	ctx->sequence = *sequence;
	ctx->status = 0;

	if (sequence->options) {
		ctx->options = *sequence->options;
		ctx->sequence.options = &ctx->options;
		ctx->sampling_index = 0U;

		if (ctx->options.interval_us != 0U) {
			atomic_set(&ctx->sampling_requested, 0);
			adc_context_enable_timer(ctx);
			return;
		}
	}

	adc_context_start_sampling(ctx);
}

/*
 * This function should be called after a sampling (of one or more channels,
 * depending on the realized sequence) is done. It calls the defined callback
 * function if required and takes further actions accordingly.
 */
static inline void adc_context_on_sampling_done(struct adc_context *ctx,
						const struct device *dev)
{
	if (ctx->sequence.options) {
		adc_sequence_callback callback = ctx->options.callback;
		enum adc_action action;
		bool finish = false;
		bool repeat = false;

		if (callback) {
			action = callback(dev,
					  &ctx->sequence,
					  ctx->sampling_index);
		} else {
			action = ADC_ACTION_CONTINUE;
		}

		switch (action) {
		case ADC_ACTION_REPEAT:
			repeat = true;
			break;
		case ADC_ACTION_FINISH:
			finish = true;
			break;
		default: /* ADC_ACTION_CONTINUE */
			if (ctx->sampling_index <
			    ctx->options.extra_samplings) {
				++ctx->sampling_index;
			} else {
				finish = true;
			}
		}

		if (!finish) {
			adc_context_update_buffer_pointer(ctx, repeat);

			/*
			 * Immediately start the next sampling if working with
			 * a zero interval or if the timer expired again while
			 * the current sampling was in progress.
			 */
			if (ctx->options.interval_us == 0U) {
				adc_context_start_sampling(ctx);
			} else if (atomic_dec(&ctx->sampling_requested) > 1) {
				adc_context_start_sampling(ctx);
			}

			return;
		}

		if (ctx->options.interval_us != 0U) {
			adc_context_disable_timer(ctx);
		}
	}

	adc_context_complete(ctx, 0);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ADC_ADC_CONTEXT_H_ */
