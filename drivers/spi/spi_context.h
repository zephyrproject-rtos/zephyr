/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for SPI drivers
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_CONTEXT_H_
#define ZEPHYR_DRIVERS_SPI_SPI_CONTEXT_H_

#include <drivers/gpio.h>
#include <drivers/spi.h>

#ifdef __cplusplus
extern "C" {
#endif

enum spi_ctx_runtime_op_mode {
	SPI_CTX_RUNTIME_OP_MODE_MASTER = BIT(0),
	SPI_CTX_RUNTIME_OP_MODE_SLAVE  = BIT(1),
};

struct spi_context {
	const struct spi_config *config;

	struct k_sem lock;
	struct k_sem sync;
	int sync_status;

#ifdef CONFIG_SPI_ASYNC
	struct k_poll_signal *signal;
	bool asynchronous;
#endif /* CONFIG_SPI_ASYNC */
	const struct spi_buf *current_tx;
	size_t tx_count;
	const struct spi_buf *current_rx;
	size_t rx_count;

	const u8_t *tx_buf;
	size_t tx_len;
	u8_t *rx_buf;
	size_t rx_len;

#ifdef CONFIG_SPI_SLAVE
	int recv_frames;
#endif /* CONFIG_SPI_SLAVE */
};

#define SPI_CONTEXT_INIT_LOCK(_data, _ctx_name)				\
	._ctx_name.lock = Z_SEM_INITIALIZER(_data._ctx_name.lock, 0, 1)

#define SPI_CONTEXT_INIT_SYNC(_data, _ctx_name)				\
	._ctx_name.sync = Z_SEM_INITIALIZER(_data._ctx_name.sync, 0, 1)

static inline bool spi_context_configured(struct spi_context *ctx,
					  const struct spi_config *config)
{
	return !!(ctx->config == config);
}

static inline bool spi_context_is_slave(struct spi_context *ctx)
{
	return (ctx->config->operation & SPI_OP_MODE_SLAVE);
}

static inline void spi_context_lock(struct spi_context *ctx,
				    bool asynchronous,
				    struct k_poll_signal *signal)
{
	k_sem_take(&ctx->lock, K_FOREVER);

#ifdef CONFIG_SPI_ASYNC
	ctx->asynchronous = asynchronous;
	ctx->signal = signal;
#endif /* CONFIG_SPI_ASYNC */
}

static inline void spi_context_release(struct spi_context *ctx, int status)
{
#ifdef CONFIG_SPI_SLAVE
	if (status >= 0 && (ctx->config->operation & SPI_LOCK_ON)) {
		return;
	}
#endif /* CONFIG_SPI_SLAVE */

#ifdef CONFIG_SPI_ASYNC
	if (!ctx->asynchronous || (status < 0)) {
		k_sem_give(&ctx->lock);
	}
#else
	k_sem_give(&ctx->lock);
#endif /* CONFIG_SPI_ASYNC */
}

static inline int spi_context_wait_for_completion(struct spi_context *ctx)
{
	int status = 0;
#ifdef CONFIG_SPI_ASYNC
	if (!ctx->asynchronous) {
		k_sem_take(&ctx->sync, K_FOREVER);
		status = ctx->sync_status;
	}
#else
	k_sem_take(&ctx->sync, K_FOREVER);
	status = ctx->sync_status;
#endif /* CONFIG_SPI_ASYNC */

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(ctx) && !status) {
		return ctx->recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

	return status;
}

static inline void spi_context_complete(struct spi_context *ctx, int status)
{
#ifdef CONFIG_SPI_ASYNC
	if (!ctx->asynchronous) {
		ctx->sync_status = status;
		k_sem_give(&ctx->sync);
	} else {
		if (ctx->signal) {
#ifdef CONFIG_SPI_SLAVE
			if (spi_context_is_slave(ctx) && !status) {
				/* Let's update the status so it tells
				 * about number of received frames.
				 */
				status = ctx->recv_frames;
			}
#endif /* CONFIG_SPI_SLAVE */
			k_poll_signal_raise(ctx->signal, status);
		}

		if (!(ctx->config->operation & SPI_LOCK_ON)) {
			k_sem_give(&ctx->lock);
		}
	}
#else
	ctx->sync_status = status;
	k_sem_give(&ctx->sync);
#endif /* CONFIG_SPI_ASYNC */
}

static inline int spi_context_cs_active_value(struct spi_context *ctx)
{
	if (ctx->config->operation & SPI_CS_ACTIVE_HIGH) {
		return 1;
	}

	return 0;
}

static inline int spi_context_cs_inactive_value(struct spi_context *ctx)
{
	if (ctx->config->operation & SPI_CS_ACTIVE_HIGH) {
		return 0;
	}

	return 1;
}

static inline void spi_context_cs_configure(struct spi_context *ctx)
{
	if (ctx->config->cs && ctx->config->cs->gpio_dev) {
		gpio_pin_configure(ctx->config->cs->gpio_dev,
				   ctx->config->cs->gpio_pin, GPIO_DIR_OUT);
		gpio_pin_write(ctx->config->cs->gpio_dev,
			       ctx->config->cs->gpio_pin,
			       spi_context_cs_inactive_value(ctx));
	} else {
		LOG_INF("CS control inhibited (no GPIO device)");
	}
}

static inline void _spi_context_cs_control(struct spi_context *ctx,
					   bool on, bool force_off)
{
	if (ctx->config && ctx->config->cs && ctx->config->cs->gpio_dev) {
		if (on) {
			gpio_pin_write(ctx->config->cs->gpio_dev,
				       ctx->config->cs->gpio_pin,
				       spi_context_cs_active_value(ctx));
			k_busy_wait(ctx->config->cs->delay);
		} else {
			if (!force_off &&
			    ctx->config->operation & SPI_HOLD_ON_CS) {
				return;
			}

			k_busy_wait(ctx->config->cs->delay);
			gpio_pin_write(ctx->config->cs->gpio_dev,
				       ctx->config->cs->gpio_pin,
				       spi_context_cs_inactive_value(ctx));
		}
	}
}

static inline void spi_context_cs_control(struct spi_context *ctx, bool on)
{
	_spi_context_cs_control(ctx, on, false);
}

static inline void spi_context_unlock_unconditionally(struct spi_context *ctx)
{
	/* Forcing CS to go to inactive status */
	_spi_context_cs_control(ctx, false, true);

	if (!k_sem_count_get(&ctx->lock)) {
		k_sem_give(&ctx->lock);
	}
}

static inline
void spi_context_buffers_setup(struct spi_context *ctx,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs,
			       u8_t dfs)
{
	LOG_DBG("tx_bufs %p - rx_bufs %p - %u", tx_bufs, rx_bufs, dfs);

	if (tx_bufs) {
		ctx->current_tx = tx_bufs->buffers;
		ctx->tx_count = tx_bufs->count;
		ctx->tx_buf = (const u8_t *)ctx->current_tx->buf;
		ctx->tx_len = ctx->current_tx->len / dfs;
	} else {
		ctx->current_tx = NULL;
		ctx->tx_count = 0;
		ctx->tx_buf = NULL;
		ctx->tx_len = 0;
	}

	if (rx_bufs) {
		ctx->current_rx = rx_bufs->buffers;
		ctx->rx_count = rx_bufs->count;
		ctx->rx_buf = (u8_t *)ctx->current_rx->buf;
		ctx->rx_len = ctx->current_rx->len / dfs;
	} else {
		ctx->current_rx = NULL;
		ctx->rx_count = 0;
		ctx->rx_buf = NULL;
		ctx->rx_len = 0;
	}

	ctx->sync_status = 0;

#ifdef CONFIG_SPI_SLAVE
	ctx->recv_frames = 0;
#endif /* CONFIG_SPI_SLAVE */

	LOG_DBG("current_tx %p (%zu), current_rx %p (%zu),"
		    " tx buf/len %p/%zu, rx buf/len %p/%zu",
		    ctx->current_tx, ctx->tx_count,
		    ctx->current_rx, ctx->rx_count,
		    ctx->tx_buf, ctx->tx_len, ctx->rx_buf, ctx->rx_len);
}

static ALWAYS_INLINE
void spi_context_update_tx(struct spi_context *ctx, u8_t dfs, u32_t len)
{
	if (!ctx->tx_len) {
		return;
	}

	if (len > ctx->tx_len) {
		LOG_ERR("Update exceeds current buffer");
		return;
	}

	ctx->tx_len -= len;
	if (!ctx->tx_len) {
		ctx->tx_count--;
		if (ctx->tx_count) {
			ctx->current_tx++;
			ctx->tx_buf = (const u8_t *)ctx->current_tx->buf;
			ctx->tx_len = ctx->current_tx->len / dfs;
		} else {
			ctx->tx_buf = NULL;
		}
	} else if (ctx->tx_buf) {
		ctx->tx_buf += dfs * len;
	}

	LOG_DBG("tx buf/len %p/%zu", ctx->tx_buf, ctx->tx_len);
}

static ALWAYS_INLINE
bool spi_context_tx_on(struct spi_context *ctx)
{
	return !!(ctx->tx_len);
}

static ALWAYS_INLINE
bool spi_context_tx_buf_on(struct spi_context *ctx)
{
	return !!(ctx->tx_buf && ctx->tx_len);
}

static ALWAYS_INLINE
void spi_context_update_rx(struct spi_context *ctx, u8_t dfs, u32_t len)
{
#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(ctx)) {
		ctx->recv_frames += len;
	}

#endif /* CONFIG_SPI_SLAVE */

	if (!ctx->rx_len) {
		return;
	}

	if (len > ctx->rx_len) {
		LOG_ERR("Update exceeds current buffer");
		return;
	}

	ctx->rx_len -= len;
	if (!ctx->rx_len) {
		ctx->rx_count--;
		if (ctx->rx_count) {
			ctx->current_rx++;
			ctx->rx_buf = (u8_t *)ctx->current_rx->buf;
			ctx->rx_len = ctx->current_rx->len / dfs;
		} else {
			ctx->rx_buf = NULL;
		}
	} else if (ctx->rx_buf) {
		ctx->rx_buf += dfs * len;
	}

	LOG_DBG("rx buf/len %p/%zu", ctx->rx_buf, ctx->rx_len);
}

static ALWAYS_INLINE
bool spi_context_rx_on(struct spi_context *ctx)
{
	return !!(ctx->rx_len);
}

static ALWAYS_INLINE
bool spi_context_rx_buf_on(struct spi_context *ctx)
{
	return !!(ctx->rx_buf && ctx->rx_len);
}

static inline size_t spi_context_longest_current_buf(struct spi_context *ctx)
{
	if (!ctx->tx_len) {
		return ctx->rx_len;
	} else if (!ctx->rx_len) {
		return ctx->tx_len;
	} else if (ctx->tx_len < ctx->rx_len) {
		return ctx->tx_len;
	}

	return ctx->rx_len;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SPI_SPI_CONTEXT_H_ */
