/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pulse_io_esp32_rmt.h"

#include <zephyr/logging/log.h>

#if RMT_DMA_SUPPORTED
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#endif

LOG_MODULE_DECLARE(pulse_io_esp32_rmt);

#if RMT_DMA_SUPPORTED
static rmt_symbol_word_t rmt_tx_dma_buf[RMT_DMA_WORDS] __aligned(sizeof(uint32_t));
#endif

static bool rmt_tx_next_half(struct rmt_channel *ch, uint16_t *dur, uint8_t *level)
{
	struct rmt_tx_iter *it = &ch->iter;
	uint32_t chunk;

	while (it->rem == 0U) {
		if (it->pos >= it->count) {
			return false;
		}
		if (it->syms != NULL) {
			it->rem = it->syms[it->pos].duration;
			it->level = it->syms[it->pos].level;
			it->pos++;
		} else if (it->cell_phase == 0U) {
			it->rem = it->cells[it->pos].duty;
			it->level = 1;
			it->cell_phase = 1;
		} else {
			it->rem = it->cell_period - it->cells[it->pos].duty;
			it->level = 0;
			it->cell_phase = 0;
			it->pos++;
		}
	}

	chunk = MIN(it->rem, RMT_DURATION_MAX);
	it->rem -= chunk;
	*dur = (uint16_t)chunk;
	*level = it->level;
	return true;
}

static size_t rmt_tx_fill(struct rmt_channel *ch, rmt_symbol_word_t *mem, size_t start,
			  size_t max_words)
{
	size_t n = 0;
	uint16_t d0, d1;
	uint8_t l0, l1;

	while (n < max_words && !ch->eof_written) {
		if (!rmt_tx_next_half(ch, &d0, &l0)) {
			mem[start + n] = (rmt_symbol_word_t){
				.duration0 = 0,
				.level0 = ch->cfg.idle_high,
				.duration1 = 0,
				.level1 = ch->cfg.idle_high,
			};
			n++;
			ch->eof_written = true;
			break;
		}
		if (!rmt_tx_next_half(ch, &d1, &l1)) {
			d1 = 0;
			l1 = ch->cfg.idle_high;
			ch->eof_written = true;
		}
		mem[start + n] = (rmt_symbol_word_t){
			.duration0 = d0,
			.level0 = l0,
			.duration1 = d1,
			.level1 = l1,
		};
		n++;
	}

	return n;
}

static void rmt_tx_isr(void *arg)
{
	struct rmt_channel *ch = arg;
	struct rmt_data *data = ch->dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index;
	k_spinlock_key_t key;
	uint32_t status;

	status = rmt_ll_tx_get_interrupt_status(regs, id);
	rmt_ll_clear_interrupt_status(regs, status);

	if (status & RMT_LL_EVENT_TX_THRES(id)) {
		if (!ch->eof_written) {
			rmt_tx_fill(ch, ch->hw_mem, ch->refill_region * RMT_PING_PONG_WORDS,
				    RMT_PING_PONG_WORDS);
			ch->refill_region ^= 1;
		}
		if (ch->eof_written) {
			key = k_spin_lock(&data->glock);
			rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_TX_THRES(id), false);
			k_spin_unlock(&data->glock, key);
		}
	}

	if (status & RMT_LL_EVENT_TX_DONE(id)) {
		ch->state = RMT_CH_READY;
		ch->result = 0;
		k_sem_give(&ch->done);
	}

#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
	if (status & RMT_LL_EVENT_TX_LOOP_END(id)) {
#if !SOC_RMT_SUPPORT_TX_LOOP_AUTO_STOP
		key = k_spin_lock(&ch->lock);
		rmt_ll_tx_stop(regs, id);
		k_spin_unlock(&ch->lock, key);
#endif
		if (ch->remain_loops > 0U) {
			uint32_t batch = MIN(ch->remain_loops, RMT_LL_MAX_LOOP_COUNT_PER_BATCH);

			ch->remain_loops -= batch;
			key = k_spin_lock(&ch->lock);
			rmt_ll_tx_set_loop_count(regs, id, batch);
			rmt_ll_tx_reset_pointer(regs, id);
			rmt_ll_tx_start(regs, id);
			k_spin_unlock(&ch->lock, key);
		} else {
#if RMT_DMA_SUPPORTED
			if (ch->with_dma) {
				key = k_spin_lock(&ch->lock);
				rmt_ll_tx_enable_dma(regs, id, true);
				k_spin_unlock(&ch->lock, key);
			}
#endif
			ch->state = RMT_CH_READY;
			ch->result = 0;
			k_sem_give(&ch->done);
		}
	}
#endif
}

int rmt_tx_configure(const struct device *dev, struct rmt_channel *ch)
{
	struct rmt_data *data = dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&data->glock);
	rmt_hal_tx_channel_reset(&data->hal, id);
	k_spin_unlock(&data->glock, key);

	if (ch->intr != NULL) {
		esp_intr_free(ch->intr);
		ch->intr = NULL;
	}
	if (esp_intr_alloc_intrstatus(soc_rmt_signals[0].irq, RMT_INTR_ALLOC_FLAGS,
				      (uint32_t)rmt_ll_get_interrupt_status_reg(regs),
				      RMT_LL_EVENT_TX_MASK(id), rmt_tx_isr, ch, &ch->intr) != 0) {
		LOG_ERR("failed to install TX interrupt");
		return -ENODEV;
	}

	ch->with_dma = false;
#if RMT_DMA_SUPPORTED
	const struct rmt_config *config = dev->config;

	if (config->dma_dev != NULL && config->tx_dma_channel != RMT_DMA_CHANNEL_UNDEFINED &&
	    id == RMT_NUM_TX_CHANNELS - 1) {
		struct dma_block_config dma_blk = {
			.block_size = sizeof(rmt_tx_dma_buf),
			.source_address = (uint32_t)rmt_tx_dma_buf,
			.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		};
		struct dma_config dma_cfg = {
			.dma_slot = ESP_GDMA_TRIG_PERIPH_RMT,
			.channel_direction = MEMORY_TO_PERIPHERAL,
			.block_count = 1,
			.head_block = &dma_blk,
		};

		ret = dma_config(config->dma_dev, config->tx_dma_channel, &dma_cfg);
		if (ret) {
			LOG_ERR("failed to configure TX DMA channel (%d)", ret);
			return ret;
		}
		ch->with_dma = true;
		key = k_spin_lock(&ch->lock);
		rmt_ll_tx_enable_dma(regs, id, true);
		k_spin_unlock(&ch->lock, key);
	}
#endif

	ret = rmt_select_channel_clock(dev, ch, ch->cfg.resolution_hz);
	if (ret) {
		return ret;
	}

	rmt_ll_tx_set_mem_blocks(regs, id, 1);
	rmt_ll_tx_set_limit(regs, id, RMT_PING_PONG_WORDS);
	rmt_ll_tx_fix_idle_level(regs, id, ch->cfg.idle_high, true);
	rmt_ll_tx_enable_wrap(regs, id, true);

	if (ch->cfg.carrier_en) {
		uint32_t total_ticks;
		uint32_t high_ticks;

		if (ch->cfg.carrier_hz == 0U || ch->cfg.carrier_duty_pct == 0U ||
		    ch->cfg.carrier_duty_pct >= 100U) {
			return -EINVAL;
		}
		total_ticks = data->group_resolution_hz / ch->cfg.carrier_hz;
		if (total_ticks == 0U) {
			return -EINVAL;
		}
		high_ticks = total_ticks * ch->cfg.carrier_duty_pct / 100U;
		key = k_spin_lock(&ch->lock);
		rmt_ll_tx_set_carrier_level(regs, id, !ch->cfg.idle_high);
		rmt_ll_tx_set_carrier_high_low_ticks(regs, id, high_ticks,
						     total_ticks - high_ticks);
		rmt_ll_tx_enable_carrier_always_on(regs, id, false);
		rmt_ll_tx_enable_carrier_modulation(regs, id, true);
		k_spin_unlock(&ch->lock, key);
	} else {
		rmt_ll_tx_enable_carrier_modulation(regs, id, false);
	}

	return 0;
}

int rmt_tx_start(const struct device *dev, struct rmt_channel *ch,
		 const struct pulse_io_tx_req *req)
{
	struct rmt_data *data = dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index;
	bool loop = req->loop_count > 0U;
	k_spinlock_key_t key;

#if !SOC_RMT_SUPPORT_TX_LOOP_COUNT
	if (loop) {
		return -ENOTSUP;
	}
#endif

	if (ch->cfg.mode == PULSE_IO_MODE_CELL) {
		for (size_t i = 0; i < req->count; i++) {
			if (req->cells[i].duty > ch->cfg.cell_period_ticks) {
				return -EINVAL;
			}
		}
	}

	ch->iter = (struct rmt_tx_iter){
		.syms = ch->cfg.mode == PULSE_IO_MODE_SYMBOL ? req->symbols : NULL,
		.cells = ch->cfg.mode == PULSE_IO_MODE_CELL ? req->cells : NULL,
		.cell_period = ch->cfg.cell_period_ticks,
		.count = req->count,
	};
	ch->eof_written = false;
	ch->refill_region = 0;
	ch->remain_loops = 0;
	ch->result = 0;
	k_sem_reset(&ch->done);

	if (loop) {
#if RMT_DMA_SUPPORTED
		/*
		 * Loop transmissions replay from the channel memory, which
		 * the CPU cannot fill and the engine does not read while
		 * DMA access is enabled.
		 */
		if (ch->with_dma) {
			key = k_spin_lock(&ch->lock);
			rmt_ll_tx_enable_dma(regs, id, false);
			k_spin_unlock(&ch->lock, key);
		}
#endif
		/* the looped pattern must fit entirely in the channel memory */
		rmt_tx_fill(ch, ch->hw_mem, 0, RMT_MEM_WORDS);
		if (!ch->eof_written) {
			return -EINVAL;
		}
		ch->remain_loops = req->loop_count;
	}

	key = k_spin_lock(&ch->lock);
	rmt_ll_tx_reset_pointer(regs, id);
	rmt_ll_tx_enable_loop(regs, id, loop);
#if SOC_RMT_SUPPORT_TX_LOOP_AUTO_STOP
	rmt_ll_tx_enable_loop_autostop(regs, id, true);
#endif
#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
	rmt_ll_tx_reset_loop_count(regs, id);
	rmt_ll_tx_enable_loop_count(regs, id, loop);
	if (loop) {
		uint32_t batch = MIN(ch->remain_loops, RMT_LL_MAX_LOOP_COUNT_PER_BATCH);

		ch->remain_loops -= batch;
		rmt_ll_tx_set_loop_count(regs, id, batch);
	}
#endif
	k_spin_unlock(&ch->lock, key);

	key = k_spin_lock(&data->glock);
#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
	rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_TX_LOOP_END(id), loop);
#endif
	if (!ch->with_dma) {
		rmt_ll_clear_interrupt_status(regs, RMT_LL_EVENT_TX_THRES(id));
		rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_TX_THRES(id), !loop);
	}
	rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_TX_DONE(id), !loop);
	k_spin_unlock(&data->glock, key);

	if (!loop) {
		bool use_dma = false;

#if RMT_DMA_SUPPORTED
		use_dma = ch->with_dma;
		if (use_dma) {
			const struct rmt_config *config = dev->config;
			int ret;

			size_t words;

			words = rmt_tx_fill(ch, rmt_tx_dma_buf, 0, RMT_DMA_WORDS);
			if (!ch->eof_written) {
				return -ENOMEM;
			}
			dma_stop(config->dma_dev, config->tx_dma_channel);
			/*
			 * The descriptor must end exactly at the payload; a
			 * longer one leaves the engine mid-transfer and the
			 * next transmit emits nothing.
			 */
			ret = dma_reload(config->dma_dev, config->tx_dma_channel,
					 (uint32_t)rmt_tx_dma_buf, 0,
					 words * sizeof(rmt_symbol_word_t));
			if (ret == 0) {
				ret = dma_start(config->dma_dev, config->tx_dma_channel);
			}
			if (ret) {
				LOG_ERR("failed to start TX DMA (%d)", ret);
				return ret;
			}
			/* wait for the DMA data to reach the RMT memory block */
			k_busy_wait(1);
		}
#endif
		if (!use_dma) {
			rmt_tx_fill(ch, ch->hw_mem, 0, RMT_MEM_WORDS);
			if (ch->eof_written) {
				key = k_spin_lock(&data->glock);
				rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_TX_THRES(id), false);
				k_spin_unlock(&data->glock, key);
			}
		}
	}

	ch->state = RMT_CH_ACTIVE;
	key = k_spin_lock(&ch->lock);
	rmt_ll_tx_fix_idle_level(regs, id, ch->cfg.idle_high, true);
	rmt_ll_tx_start(regs, id);
	k_spin_unlock(&ch->lock, key);

	return 0;
}

void rmt_tx_halt(const struct device *dev, struct rmt_channel *ch)
{
	struct rmt_data *data = dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index;
	k_spinlock_key_t key;

	key = k_spin_lock(&ch->lock);
	rmt_ll_tx_enable_loop(regs, id, false);
#if RMT_LL_SUPPORT(ASYNC_STOP)
	rmt_ll_tx_stop(regs, id);
#endif
	k_spin_unlock(&ch->lock, key);

	key = k_spin_lock(&data->glock);
	rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_TX_MASK(id), false);
#if !RMT_LL_SUPPORT(ASYNC_STOP)
	/*
	 * The TX engine cannot be stopped asynchronously; overwrite the
	 * channel memory with stop markers and wait for the engine to
	 * consume one.
	 */
	memset(ch->hw_mem, 0, RMT_MEM_WORDS * sizeof(rmt_symbol_word_t));
	while (!(rmt_ll_tx_get_interrupt_status_raw(regs, id) & RMT_LL_EVENT_TX_DONE(id))) {
	}
#endif
	rmt_ll_clear_interrupt_status(regs, RMT_LL_EVENT_TX_MASK(id));
	k_spin_unlock(&data->glock, key);

#if RMT_DMA_SUPPORTED
	if (ch->with_dma) {
		const struct rmt_config *config = dev->config;

		dma_stop(config->dma_dev, config->tx_dma_channel);
		key = k_spin_lock(&ch->lock);
		rmt_ll_tx_enable_dma(regs, id, true);
		k_spin_unlock(&ch->lock, key);
	}
#endif

	ch->state = RMT_CH_READY;
}
