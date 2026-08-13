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

void i2s_rtio_init(struct i2s_rtio *ctx)
{
	mpsc_init(&ctx->io_q);
	ctx->curr = NULL;
	mpsc_init(&ctx->prev_q);
	atomic_set(&ctx->io_q_size, 0);
}

static const struct i2s_iodev_config *i2s_rtio_iodev_get_config(const struct rtio_iodev_sqe *curr)
{
	const struct rtio_sqe *curr_sqe = &curr->sqe;
	const struct rtio_iodev *curr_iodev = curr_sqe->iodev;
	struct i2s_iodev_data *curr_data = curr_iodev->data;

	return &curr_data->config;
}

static bool i2s_rtio_iodev_is_equal(const struct rtio_iodev_sqe *prev,
				    const struct rtio_iodev_sqe *curr)
{
	const struct i2s_iodev_config *prev_config = i2s_rtio_iodev_get_config(prev);
	const struct i2s_iodev_config *curr_config = i2s_rtio_iodev_get_config(curr);

	return memcmp(prev_config, curr_config, sizeof(struct i2s_iodev_config)) == 0;
}

static uint8_t i2s_rtio_iodev_get_preload_count(const struct rtio_iodev_sqe *curr)
{
	const struct i2s_iodev_config *curr_config = i2s_rtio_iodev_get_config(curr);

	return curr_config->preload_count;
}

bool i2s_rtio_continue(struct i2s_rtio *ctx)
{
	k_spinlock_key_t key;
	struct rtio_iodev_sqe *prev;
	uint8_t io_q_size;
	struct mpsc_node *node;
	bool stream;

	key = k_spin_lock(&ctx->lock);

	__ASSERT(ctx->curr != NULL, "invalid call to i2s_rtio_continue");

	prev = ctx->curr;

	/* We will complete the submission later */
	mpsc_push(&ctx->prev_q, &prev->q);

	io_q_size = atomic_get(&ctx->io_q_size);

	/*
	 * The stream is running without error given i2s_rtio_continue has been called. Continuing
	 * the stream requires a single available submission with compatible configuration.
	 */
	if (io_q_size) {
		node = mpsc_pop(&ctx->io_q);
		atomic_dec(&ctx->io_q_size);
		ctx->curr = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
	} else {
		ctx->curr = NULL;
	}

	if (ctx->curr == NULL) {
		/* Stream shall be stopped as no new submission is available */
		stream = false;
	} else {
		/* Stream shall continue if new submission configuration is compatible */
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
	uint8_t io_q_size;
	uint8_t preload_size;

	key = k_spin_lock(&ctx->lock);

	node = mpsc_pop(&ctx->prev_q);

	if (node != NULL) {
		prev = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

		/* This is not the last submission of the stream since prev_q was not empty */
		pending = false;
	} else {
		__ASSERT(ctx->curr != NULL, "unbalanced call to i2s_rtio_complete");

		/* This must be the last submission of the stream since prev_q was empty */
		prev = ctx->curr;

		io_q_size = atomic_get(&ctx->io_q_size);
		preload_size = i2s_rtio_iodev_get_preload_count(ctx->curr);

		/* We only start the next stream if preload_size is reached */
		if (io_q_size >= preload_size) {
			node = mpsc_pop(&ctx->io_q);
			atomic_dec(&ctx->io_q_size);
		}

		if (node != NULL) {
			ctx->curr = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
		} else {
			ctx->curr = NULL;
		}

		pending = ctx->curr != NULL;
	}

	k_spin_unlock(&ctx->lock, key);

	/* Complete previous submission without holding the spinlock */
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
	uint8_t io_q_size;
	uint8_t preload_size;
	bool pending;

	mpsc_push(&ctx->io_q, &iodev_sqe->q);
	atomic_inc(&ctx->io_q_size);

	key = k_spin_lock(&ctx->lock);

	running = ctx->curr != NULL;
	io_q_size = atomic_get(&ctx->io_q_size);
	preload_size = i2s_rtio_iodev_get_preload_count(iodev_sqe);

	if (!running && io_q_size >= preload_size) {
		next = mpsc_pop(&ctx->io_q);
		atomic_dec(&ctx->io_q_size);
		ctx->curr = CONTAINER_OF(next, struct rtio_iodev_sqe, q);
		pending = true;
	} else {
		pending = false;
	}

	k_spin_unlock(&ctx->lock, key);

	return pending;
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

int i2s_rtio_configure(struct i2s_rtio *ctx, enum i2s_dir dir, const struct i2s_config *cfg)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(dir);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

const struct i2s_config *i2s_rtio_config_get(struct i2s_rtio *ctx, enum i2s_dir dir)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(dir);
	return NULL;
}

int i2s_rtio_read(struct i2s_rtio *ctx, void **mem_block, size_t *size)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);
	return -ENOTSUP;
}

int i2s_rtio_write(struct i2s_rtio *ctx, void *mem_block, size_t size)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);
	return -ENOTSUP;
}

int i2s_rtio_trigger(struct i2s_rtio *ctx, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(dir);
	ARG_UNUSED(cmd);
	return -ENOTSUP;
}
