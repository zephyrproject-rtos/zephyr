/*
 * Copyright (c) 2024 Croxel, Inc.
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc/rtio.h>

#include <string.h>

static void adc_rtio_iodev_api_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct adc_dt_spec *spec = (const struct adc_dt_spec *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = spec->dev;

	DEVICE_API_GET(adc, dev)->submit(dev, iodev_sqe);
}

const struct rtio_iodev_api adc_rtio_iodev_api = {
	.submit = adc_rtio_iodev_api_submit,
};

void adc_rtio_init(struct adc_rtio *ctx, const struct device *dev)
{
	mpsc_init(&ctx->io_q);
	ctx->txn_head = NULL;
	ctx->txn_curr = NULL;
#if CONFIG_ADC_RTIO_WRAPPER
	ctx->iodev.api = &adc_rtio_iodev_api,
	ctx->iodev.data = &ctx->spec,
	k_sem_init(&ctx->semlock, 1, 1);
	ctx->spec.dev = dev;

#endif /* CONFIG_ADC_RTIO_WRAPPER */
}

static bool adc_rtio_next(struct adc_rtio *ctx, bool completion)
{
	k_spinlock_key_t key;
	struct mpsc_node *next;

	key = k_spin_lock(&ctx->spinlock);

	if (!completion && ctx->txn_curr != NULL) {
		k_spin_unlock(&ctx->spinlock, key);
		return false;
	}

	next = mpsc_pop(&ctx->io_q);

	if (next != NULL) {
		struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);

		ctx->txn_head = next_sqe;
		ctx->txn_curr = next_sqe;
	} else {
		ctx->txn_head = NULL;
		ctx->txn_curr = NULL;
	}

	k_spin_unlock(&ctx->spinlock, key);

	return (ctx->txn_curr != NULL);
}

bool adc_rtio_complete(struct adc_rtio *ctx, int status)
{
	struct rtio_iodev_sqe *txn_head = ctx->txn_head;
	bool result;

	result = adc_rtio_next(ctx, true);

	if (status < 0) {
		rtio_iodev_sqe_err(txn_head, status);
	} else {
		rtio_iodev_sqe_ok(txn_head, status);
	}

	return result;
}

bool adc_rtio_submit(struct adc_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe)
{
	mpsc_push(&ctx->io_q, &iodev_sqe->q);
	return adc_rtio_next(ctx, false);
}

#if CONFIG_ADC_RTIO_WRAPPER
int adc_rtio_channel_setup(struct adc_rtio *ctx, const struct adc_channel_cfg *channel_cfg)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	__ASSERT_NO_MSG(ctx != NULL);
	__ASSERT_NO_MSG(channel_cfg != NULL);

	k_sem_take(&ctx->semlock, K_FOREVER);

	sqe = rtio_sqe_acquire(ctx->r);
	__ASSERT_NO_MSG(sqe != NULL);

	adc_rtio_sqe_prep_channel_setup(sqe,
					&ctx->iodev,
					RTIO_PRIO_NORM,
					channel_cfg,
					NULL);

	rtio_submit(ctx->r, 1);

	cqe = rtio_cqe_consume(ctx->r);
	__ASSERT_NO_MSG(cqe != NULL);
	ret = cqe->result;
	rtio_cqe_release(ctx->r, cqe);

	k_sem_give(&ctx->semlock);
	return ret;
}

int adc_rtio_read(struct adc_rtio *ctx, const struct adc_sequence *sequence)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	int ret;

	__ASSERT_NO_MSG(ctx != NULL);
	__ASSERT_NO_MSG(sequence != NULL);

	k_sem_take(&ctx->semlock, K_FOREVER);

	sqe = rtio_sqe_acquire(ctx->r);
	__ASSERT_NO_MSG(sqe != NULL);

	adc_rtio_sqe_prep_read(sqe,
			       &ctx->iodev,
			       RTIO_PRIO_NORM,
			       sequence,
			       NULL);

	rtio_submit(ctx->r, 1);

	cqe = rtio_cqe_consume(ctx->r);
	__ASSERT_NO_MSG(cqe != NULL);
	ret = cqe->result;
	rtio_cqe_release(ctx->r, cqe);

	k_sem_give(&ctx->semlock);
	return ret;
}

#if CONFIG_ADC_ASYNC
static void adc_rtio_read_callback(struct rtio *r,
				   const struct rtio_sqe *sqe,
				   int res,
				   void *arg0)
{
	struct adc_rtio *ctx = sqe->userdata;
	struct k_poll_signal *async_sig = arg0;

	ARG_UNUSED(r);
	ARG_UNUSED(sqe);

	k_sem_give(ctx->semlock);

	if (async_sig != NULL) {
		k_poll_signal_raise(async_sig, res);
	}
}

int adc_rtio_read_async(struct adc_rtio *ctx,
			const struct adc_sequence *sequence,
			struct k_poll_signal *async)
{
	struct rtio_sqe *sqe;
	int ret;

	__ASSERT_NO_MSG(ctx != NULL);
	__ASSERT_NO_MSG(sequence != NULL);

	k_sem_take(&ctx->semlock, K_FOREVER);

	sqe = rtio_sqe_acquire(ctx->r);
	__ASSERT_NO_MSG(sqe != NULL);

	adc_rtio_sqe_prep_read(sqe,
			       &ctx->iodev,
			       RTIO_PRIO_NORM,
			       sequence,
			       NULL);

	sqe->flags |= RTIO_SQE_CHAINED | RTIO_SQE_NO_RESPONSE;

	sqe = rtio_sqe_acquire(ctx->r);
	__ASSERT_NO_MSG(sqe != NULL);

	rtio_sqe_prep_callback_no_cqe(sqe,
				      adc_rtio_read_callback,
				      async;
				      ctx);

	rtio_submit(ctx->r, 0);

	return 0;
}
#endif /* CONFIG_ADC_ASYNC */
#endif /* CONFIG_ADC_RTIO_WRAPPER */
