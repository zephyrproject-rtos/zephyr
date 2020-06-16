/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_STEPPER_CONTEXT_H_
#define ZEPHYR_DRIVERS_STEPPER_STEPPER_CONTEXT_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stepper_context {
	struct k_sem lock;
	struct k_sem sync;
	int status;

#ifdef CONFIG_STEPPER_ASYNC
	struct k_poll_signal *signal;
	bool asynchronous;
#endif /* CONFIG_STEPPER_ASYNC */
};

#define STEPPER_CONTEXT_INIT_LOCK(_data, _ctx_name)                            \
	._ctx_name.lock = Z_SEM_INITIALIZER(_data._ctx_name.lock, 0, 1)

#define STEPPER_CONTEXT_INIT_SYNC(_data, _ctx_name)                            \
	._ctx_name.sync = Z_SEM_INITIALIZER(_data._ctx_name.sync, 0, 1)

static inline void stepper_context_lock(struct stepper_context *ctx,
					bool asynchronous,
					struct k_poll_signal *signal)
{
	k_sem_take(&ctx->lock, K_FOREVER);

#ifdef CONFIG_STEPPER_ASYNC
	ctx->asynchronous = asynchronous;
	ctx->signal = signal;
#endif /* CONFIG_STEPPER_ASYNC */
}

static inline void stepper_context_release(struct stepper_context *ctx,
					   int status)
{
#ifdef CONFIG_STEPPER_ASYNC
	if (ctx->asynchronous && (status == 0)) {
		return;
	}
#endif /* CONFIG_STEPPER_ASYNC */

	k_sem_give(&ctx->lock);
}

static inline void
stepper_context_unlock_unconditionally(struct stepper_context *ctx)
{
	if (!k_sem_count_get(&ctx->lock)) {
		k_sem_give(&ctx->lock);
	}
}

static inline int
stepper_context_wait_for_completion(struct stepper_context *ctx)
{
#ifdef CONFIG_STEPPER_ASYNC
	if (ctx->asynchronous) {
		return 0;
	}
#endif /* CONFIG_STEPPER_ASYNC */

	k_sem_take(&ctx->sync, K_FOREVER);
	return ctx->status;
}

static inline void stepper_context_complete(struct stepper_context *ctx,
					    int status)
{
#ifdef CONFIG_STEPPER_ASYNC
	if (ctx->asynchronous) {
		if (ctx->signal) {
			k_poll_signal_raise(ctx->signal, status);
		}

		k_sem_give(&ctx->lock);
		return;
	}
#endif /* CONFIG_STEPPER_ASYNC */

	if (status != 0) {
		ctx->status = status;
	}
	k_sem_give(&ctx->sync);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_STEPPER_CONTEXT_H_ */
