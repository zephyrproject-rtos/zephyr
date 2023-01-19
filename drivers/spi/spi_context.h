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

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

enum spi_ctx_runtime_op_mode {
	SPI_CTX_RUNTIME_OP_MODE_MASTER = BIT(0),
	SPI_CTX_RUNTIME_OP_MODE_SLAVE  = BIT(1),
};

struct spi_context {
	const struct spi_config *config;
	const struct spi_config *owner;
	const struct gpio_dt_spec *cs_gpios;
	size_t num_cs_gpios;

	struct k_sem lock;
	struct k_sem sync;
	int sync_status;

#ifdef CONFIG_SPI_ASYNC
	spi_callback_t callback;
	void *callback_data;
	bool asynchronous;
#endif /* CONFIG_SPI_ASYNC */
	const struct spi_buf *current_tx;
	size_t tx_count;
	const struct spi_buf *current_rx;
	size_t rx_count;

	const uint8_t *tx_buf;
	size_t tx_len;
	uint8_t *rx_buf;
	size_t rx_len;

#ifdef CONFIG_SPI_SLAVE
	int recv_frames;
#endif /* CONFIG_SPI_SLAVE */
};

#define SPI_CONTEXT_INIT_LOCK(_data, _ctx_name)				\
	._ctx_name.lock = Z_SEM_INITIALIZER(_data._ctx_name.lock, 0, 1)

#define SPI_CONTEXT_INIT_SYNC(_data, _ctx_name)				\
	._ctx_name.sync = Z_SEM_INITIALIZER(_data._ctx_name.sync, 0, 1)

#define SPI_CONTEXT_CS_GPIO_SPEC_ELEM(_node_id, _prop, _idx)		\
	GPIO_DT_SPEC_GET_BY_IDX(_node_id, _prop, _idx),

#define SPI_CONTEXT_CS_GPIOS_FOREACH_ELEM(_node_id)				\
	DT_FOREACH_PROP_ELEM(_node_id, cs_gpios,				\
				SPI_CONTEXT_CS_GPIO_SPEC_ELEM)

#define SPI_CONTEXT_CS_GPIOS_INITIALIZE(_node_id, _ctx_name)				\
	._ctx_name.cs_gpios = (const struct gpio_dt_spec []) {				\
		COND_CODE_1(DT_SPI_HAS_CS_GPIOS(_node_id),				\
			    (SPI_CONTEXT_CS_GPIOS_FOREACH_ELEM(_node_id)), ({0}))	\
	},										\
	._ctx_name.num_cs_gpios = DT_PROP_LEN_OR(_node_id, cs_gpios, 0),

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
				    spi_callback_t callback,
				    void *callback_data,
				    const struct spi_config *spi_cfg)
{
	if ((spi_cfg->operation & SPI_LOCK_ON) &&
		(k_sem_count_get(&ctx->lock) == 0) &&
		(ctx->owner == spi_cfg)) {
			return;
	}

	k_sem_take(&ctx->lock, K_FOREVER);
	ctx->owner = spi_cfg;

#ifdef CONFIG_SPI_ASYNC
	ctx->asynchronous = asynchronous;
	ctx->callback = callback;
	ctx->callback_data = callback_data;
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
		ctx->owner = NULL;
		k_sem_give(&ctx->lock);
	}
#else
	if (!(ctx->config->operation & SPI_LOCK_ON)) {
		ctx->owner = NULL;
		k_sem_give(&ctx->lock);
	}
#endif /* CONFIG_SPI_ASYNC */
}

static inline size_t spi_context_total_tx_len(struct spi_context *ctx);
static inline size_t spi_context_total_rx_len(struct spi_context *ctx);

static inline int spi_context_wait_for_completion(struct spi_context *ctx)
{
	int status = 0;
	bool wait;

#ifdef CONFIG_SPI_ASYNC
	wait = !ctx->asynchronous;
#else
	wait = true;
#endif

	if (wait) {
		k_timeout_t timeout;

		/* Do not use any timeout in the slave mode, as in this case
		 * it is not known when the transfer will actually start and
		 * what the frequency will be.
		 */
		if (IS_ENABLED(CONFIG_SPI_SLAVE) && spi_context_is_slave(ctx)) {
			timeout = K_FOREVER;
		} else {
			uint32_t tx_len = spi_context_total_tx_len(ctx);
			uint32_t rx_len = spi_context_total_rx_len(ctx);
			uint32_t timeout_ms;

			timeout_ms = MAX(tx_len, rx_len) * 8 * 1000 /
				     ctx->config->frequency;
			timeout_ms += CONFIG_SPI_COMPLETION_TIMEOUT_TOLERANCE;

			timeout = K_MSEC(timeout_ms);
		}

		if (k_sem_take(&ctx->sync, timeout)) {
			LOG_ERR("Timeout waiting for transfer complete");
			return -ETIMEDOUT;
		}
		status = ctx->sync_status;
	}

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(ctx) && !status) {
		return ctx->recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

	return status;
}

static inline void spi_context_complete(struct spi_context *ctx,
					const struct device *dev,
					int status)
{
#ifdef CONFIG_SPI_ASYNC
	if (!ctx->asynchronous) {
		ctx->sync_status = status;
		k_sem_give(&ctx->sync);
	} else {
		if (ctx->callback) {
#ifdef CONFIG_SPI_SLAVE
			if (spi_context_is_slave(ctx) && !status) {
				/* Let's update the status so it tells
				 * about number of received frames.
				 */
				status = ctx->recv_frames;
			}
#endif /* CONFIG_SPI_SLAVE */
			ctx->callback(dev, status, ctx->callback_data);
		}

		if (!(ctx->config->operation & SPI_LOCK_ON)) {
			ctx->owner = NULL;
			k_sem_give(&ctx->lock);
		}
	}
#else
	ctx->sync_status = status;
	k_sem_give(&ctx->sync);
#endif /* CONFIG_SPI_ASYNC */
}

static inline int spi_context_cs_configure_all(struct spi_context *ctx)
{
	int ret;
	const struct gpio_dt_spec *cs_gpio;

	for (cs_gpio = ctx->cs_gpios; cs_gpio < &ctx->cs_gpios[ctx->num_cs_gpios]; cs_gpio++) {
		if (!device_is_ready(cs_gpio->port)) {
			LOG_ERR("CS GPIO port %s pin %d is not ready",
				cs_gpio->port->name, cs_gpio->pin);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(cs_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static inline void _spi_context_cs_control(struct spi_context *ctx,
					   bool on, bool force_off)
{
	if (ctx->config && ctx->config->cs && ctx->config->cs->gpio.port) {
		if (on) {
			gpio_pin_set_dt(&ctx->config->cs->gpio, 1);
			k_busy_wait(ctx->config->cs->delay);
		} else {
			if (!force_off &&
			    ctx->config->operation & SPI_HOLD_ON_CS) {
				return;
			}

			k_busy_wait(ctx->config->cs->delay);
			gpio_pin_set_dt(&ctx->config->cs->gpio, 0);
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
		ctx->owner = NULL;
		k_sem_give(&ctx->lock);
	}
}

static inline void *spi_context_get_next_buf(const struct spi_buf **current,
					     size_t *count,
					     size_t *buf_len,
					     uint8_t dfs)
{
	/* This loop skips zero-length buffers in the set, if any. */
	while (*count) {
		if (((*current)->len / dfs) != 0) {
			*buf_len = (*current)->len / dfs;
			return (*current)->buf;
		}
		++(*current);
		--(*count);
	}

	*buf_len = 0;
	return NULL;
}

static inline
void spi_context_buffers_setup(struct spi_context *ctx,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs,
			       uint8_t dfs)
{
	LOG_DBG("tx_bufs %p - rx_bufs %p - %u", tx_bufs, rx_bufs, dfs);

	ctx->current_tx = tx_bufs ? tx_bufs->buffers : NULL;
	ctx->tx_count = ctx->current_tx ? tx_bufs->count : 0;
	ctx->tx_buf = (const uint8_t *)
		spi_context_get_next_buf(&ctx->current_tx, &ctx->tx_count,
					 &ctx->tx_len, dfs);

	ctx->current_rx = rx_bufs ? rx_bufs->buffers : NULL;
	ctx->rx_count = ctx->current_rx ? rx_bufs->count : 0;
	ctx->rx_buf = (uint8_t *)
		spi_context_get_next_buf(&ctx->current_rx, &ctx->rx_count,
					 &ctx->rx_len, dfs);

	ctx->sync_status = 0;

#ifdef CONFIG_SPI_SLAVE
	ctx->recv_frames = 0;
#endif /* CONFIG_SPI_SLAVE */

	LOG_DBG("current_tx %p (%zu), current_rx %p (%zu),"
		" tx buf/len %p/%zu, rx buf/len %p/%zu",
		ctx->current_tx, ctx->tx_count,
		ctx->current_rx, ctx->rx_count,
		(void *)ctx->tx_buf, ctx->tx_len,
		(void *)ctx->rx_buf, ctx->rx_len);
}

static ALWAYS_INLINE
void spi_context_update_tx(struct spi_context *ctx, uint8_t dfs, uint32_t len)
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
		/* Current buffer is done. Get the next one to be processed. */
		++ctx->current_tx;
		--ctx->tx_count;
		ctx->tx_buf = (const uint8_t *)
			spi_context_get_next_buf(&ctx->current_tx,
						 &ctx->tx_count,
						 &ctx->tx_len, dfs);
	} else if (ctx->tx_buf) {
		ctx->tx_buf += dfs * len;
	}

	LOG_DBG("tx buf/len %p/%zu", (void *)ctx->tx_buf, ctx->tx_len);
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
void spi_context_update_rx(struct spi_context *ctx, uint8_t dfs, uint32_t len)
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
		/* Current buffer is done. Get the next one to be processed. */
		++ctx->current_rx;
		--ctx->rx_count;
		ctx->rx_buf = (uint8_t *)
			spi_context_get_next_buf(&ctx->current_rx,
						 &ctx->rx_count,
						 &ctx->rx_len, dfs);
	} else if (ctx->rx_buf) {
		ctx->rx_buf += dfs * len;
	}

	LOG_DBG("rx buf/len %p/%zu", (void *)ctx->rx_buf, ctx->rx_len);
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

/*
 * Returns the maximum length of a transfer for which all currently active
 * directions have a continuous buffer, i.e. the maximum SPI transfer that
 * can be done with DMA that handles only non-scattered buffers.
 */
static inline size_t spi_context_max_continuous_chunk(struct spi_context *ctx)
{
	if (!ctx->tx_len) {
		return ctx->rx_len;
	} else if (!ctx->rx_len) {
		return ctx->tx_len;
	}

	return MIN(ctx->tx_len, ctx->rx_len);
}

static inline size_t spi_context_longest_current_buf(struct spi_context *ctx)
{
	return ctx->tx_len > ctx->rx_len ? ctx->tx_len : ctx->rx_len;
}

static inline size_t spi_context_total_tx_len(struct spi_context *ctx)
{
	size_t n;
	size_t total_len = 0;

	for (n = 0; n < ctx->tx_count; ++n) {
		total_len += ctx->current_tx[n].len;
	}

	return total_len;
}

static inline size_t spi_context_total_rx_len(struct spi_context *ctx)
{
	size_t n;
	size_t total_len = 0;

	for (n = 0; n < ctx->rx_count; ++n) {
		total_len += ctx->current_rx[n].len;
	}

	return total_len;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SPI_SPI_CONTEXT_H_ */
