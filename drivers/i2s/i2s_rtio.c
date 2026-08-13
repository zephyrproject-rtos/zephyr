/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2s_rtio.h"

const struct rtio_iodev_api i2s_iodev_api = {
	.submit = i2s_iodev_submit,
};

static struct k_spinlock iodev_lock;

void i2s_rtio_init(struct i2s_rtio *ctx, const struct device *dev)
{
	mpsc_init(&ctx->io_q);
	ctx->curr = NULL;
	mpsc_init(&ctx->prev_q);
#if CONFIG_I2S_RTIO_WRAPPER
	k_sem_init(&ctx->r_lock, 1, 1);
	ctx->data.dev = dev;
	ctx->iodev.data = &ctx->data;
	ctx->iodev.api = &i2s_iodev_api;
#endif
}

static bool i2s_rtio_iodev_is_equal(const struct rtio_iodev_sqe *prev,
				    const struct rtio_iodev_sqe *curr)
{
	/* TODO add proper equality check, or document iodevs must be const */
	return prev->sqe.iodev == curr->sqe.iodev;
}

bool i2s_rtio_continue(struct i2s_rtio *ctx)
{
	k_spinlock_key_t key;
	struct rtio_iodev_sqe *prev;
	struct mpsc_node *node;
	bool stream;

	key = k_spin_lock(&ctx->lock);

	prev = ctx->curr;

	mpsc_push(&ctx->prev_q, &prev->q);

	node = mpsc_pop(&ctx->io_q);

	if (node != NULL) {
		ctx->curr = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
	} else {
		ctx->curr = NULL;
	}

	if (ctx->curr == NULL) {
		stream = false;
	} else {
		stream = i2s_rtio_iodev_is_equal(prev, ctx->curr);
	}

	k_spin_unlock(&ctx->lock, key);

	return stream;
}

bool i2s_rtio_complete(struct i2s_rtio *ctx, int status)
{
	k_spinlock_key_t key;
	struct mpsc_node *node;
	struct rtio_iodev_sqe *prev;
	bool pending;

	key = k_spin_lock(&ctx->lock);

	node = mpsc_pop(&ctx->prev_q);

	if (node != NULL) {
		prev = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
		pending = false;
	} else {
		prev = ctx->curr;

		node = mpsc_pop(&ctx->io_q);

		if (node != NULL) {
			ctx->curr = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
		} else {
			ctx->curr = NULL;
		}

		pending = ctx->curr != NULL;
	}

	k_spin_unlock(&ctx->lock, key);

	if (status < 0) {
		rtio_iodev_sqe_err(prev, status);
	} else {
		rtio_iodev_sqe_ok(prev, status);
	}

	return pending;
}

bool i2s_rtio_submit(struct i2s_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe)
{
	k_spinlock_key_t key;
	struct mpsc_node *next;
	bool running;

	mpsc_push(&ctx->io_q, &iodev_sqe->q);

	key = k_spin_lock(&ctx->lock);

	running = ctx->curr != NULL;

	if (!running) {
		next = mpsc_pop(&ctx->io_q);
		ctx->curr = CONTAINER_OF(next, struct rtio_iodev_sqe, q);
	}

	k_spin_unlock(&ctx->lock, key);

	return !running;
}

int z_impl_i2s_configure_iodev(const struct rtio_iodev *i2s_iodev,
			       const struct i2s_iodev_config *config)

{
	struct i2s_iodev_data *iodev_data;
	k_spinlock_key_t key;

	iodev_data = i2s_iodev->data;
	key = k_spin_lock(&iodev_lock);
	iodev_data->config = *config;
	k_spin_unlock(&iodev_lock, key);

	return 0;
}

void z_impl_i2s_get_config_iodev(const struct rtio_iodev *i2s_iodev,
				 struct i2s_iodev_config *config)

{
	struct i2s_iodev_data *iodev_data;
	k_spinlock_key_t key;

	iodev_data = i2s_iodev->data;
	key = k_spin_lock(&iodev_lock);
	*config = iodev_data->config;
	k_spin_unlock(&iodev_lock, key);
}

#if CONFIG_I2S_RTIO_WRAPPER
int i2s_rtio_configure(struct i2s_rtio *ctx, enum i2s_dir dir, const struct i2s_config *cfg)
{
	return -ENOTSUP;
}

const struct i2s_config *i2s_rtio_config_get(struct i2s_rtio *ctx, enum i2s_dir dir)
{
	return NULL;
}

int i2s_rtio_read(struct i2s_rtio *ctx, void **mem_block, size_t *size)
{
	return -ENOTSUP;
}

int i2s_rtio_write(struct i2s_rtio *ctx, void *mem_block, size_t size)
{
	return -ENOTSUP;
}

int i2s_rtio_trigger(struct i2s_rtio *ctx, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	return -ENOTSUP;
}

#else

int i2s_rtio_configure(struct i2s_rtio *ctx, enum i2s_dir dir, const struct i2s_config *cfg)
{
	return -ENOTSUP;
}

const struct i2s_config *i2s_rtio_config_get(struct i2s_rtio *ctx, enum i2s_dir dir)
{
	return NULL;
}

int i2s_rtio_read(struct i2s_rtio *ctx, void **mem_block, size_t *size)
{
	return -ENOTSUP;
}

int i2s_rtio_write(struct i2s_rtio *ctx, void *mem_block, size_t size)
{
	return -ENOTSUP;
}

int i2s_rtio_trigger(struct i2s_rtio *ctx, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	return -ENOTSUP;
}
#endif
