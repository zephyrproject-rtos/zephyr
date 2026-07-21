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
#include <hal/gdma_hal.h>
#include <hal/gdma_ll.h>
#include <hal/dma_types.h>
#endif

LOG_MODULE_DECLARE(pulse_io_esp32_rmt);

#if RMT_DMA_SUPPORTED
static rmt_symbol_word_t rmt_rx_dma_buf[RMT_DMA_WORDS] __aligned(sizeof(uint32_t));

/* dma_esp32 splits the bounce buffer into descriptor-sized chunks */
#define RMT_RX_DMA_DESCS                                                                           \
	DIV_ROUND_UP(sizeof(rmt_rx_dma_buf), DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED)
#endif

static void rmt_rx_store(struct rmt_channel *ch, const rmt_symbol_word_t *words, size_t nwords)
{
	for (size_t i = 0; i < nwords; i++) {
		if (words[i].duration0 == 0U) {
			break;
		}
		if (ch->rx_count < ch->rx_cap) {
			ch->rx_buf[ch->rx_count++] = (struct pulse_symbol){
				.duration = words[i].duration0,
				.level = words[i].level0,
			};
		} else {
			ch->rx_overflow = true;
		}
		if (words[i].duration1 == 0U) {
			break;
		}
		if (ch->rx_count < ch->rx_cap) {
			ch->rx_buf[ch->rx_count++] = (struct pulse_symbol){
				.duration = words[i].duration1,
				.level = words[i].level1,
			};
		} else {
			ch->rx_overflow = true;
		}
	}
}

static void rmt_rx_finish(struct rmt_channel *ch)
{
	ch->state = RMT_CH_READY;
	ch->result = ch->rx_overflow ? -ENOMEM : 0;
	k_sem_give(&ch->done);
}

static void rmt_rx_isr(void *arg)
{
	struct rmt_channel *ch = arg;
	struct rmt_data *data = ch->dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index - RMT_RX_CHANNEL_OFFSET;
	k_spinlock_key_t key;
	uint32_t status;

	status = rmt_ll_rx_get_interrupt_status(regs, id);

#if SOC_RMT_SUPPORT_RX_PINGPONG
	if (status & RMT_LL_EVENT_RX_THRES(id)) {
		rmt_ll_clear_interrupt_status(regs, RMT_LL_EVENT_RX_THRES(id));

		key = k_spin_lock(&ch->lock);
		rmt_ll_rx_set_mem_owner(regs, id, RMT_LL_MEM_OWNER_SW);
		rmt_rx_store(ch, ch->hw_mem + ch->rx_hw_off, RMT_PING_PONG_WORDS);
		rmt_ll_rx_set_mem_owner(regs, id, RMT_LL_MEM_OWNER_HW);
		k_spin_unlock(&ch->lock, key);

		ch->rx_hw_off = RMT_PING_PONG_WORDS - ch->rx_hw_off;
	}
#endif

	if (status & RMT_LL_EVENT_RX_DONE(id)) {
		uint32_t offset;
		size_t nwords;

		rmt_ll_clear_interrupt_status(regs, RMT_LL_EVENT_RX_DONE(id));

		key = k_spin_lock(&ch->lock);
		rmt_ll_rx_enable(regs, id, false);
		k_spin_unlock(&ch->lock, key);

#if !RMT_LL_SUPPORT(ASYNC_STOP)
		/*
		 * The RX engine cannot be disabled once enabled; when the
		 * channel was already halted, drop the stale event.
		 */
		if (ch->state != RMT_CH_ACTIVE) {
			return;
		}
#endif

		offset = rmt_ll_rx_get_memory_writer_offset(regs, id);
		nwords = (offset >= ch->rx_hw_off) ? (offset - ch->rx_hw_off)
						   : (ch->rx_hw_off - offset);

		key = k_spin_lock(&ch->lock);
		rmt_ll_rx_set_mem_owner(regs, id, RMT_LL_MEM_OWNER_SW);
		rmt_rx_store(ch, ch->hw_mem + ch->rx_hw_off, nwords);
		rmt_ll_rx_set_mem_owner(regs, id, RMT_LL_MEM_OWNER_HW);
		k_spin_unlock(&ch->lock, key);

#if !SOC_RMT_SUPPORT_RX_PINGPONG
		if (rmt_ll_rx_get_interrupt_status_raw(regs, id) & RMT_LL_EVENT_RX_ERROR(id)) {
			key = k_spin_lock(&ch->lock);
			rmt_ll_rx_reset_pointer(regs, id);
			k_spin_unlock(&ch->lock, key);
			rmt_ll_clear_interrupt_status(regs, RMT_LL_EVENT_RX_ERROR(id));
			ch->rx_overflow = true;
		}
#endif

		rmt_rx_finish(ch);
	}
}

#if RMT_DMA_SUPPORTED
static void rmt_rx_dma_eof_cb(const struct device *dma_dev, void *user_data, uint32_t dma_channel,
			      int status)
{
	struct rmt_channel *ch = user_data;
	struct rmt_data *data = ch->dev->data;
	gdma_hal_context_t *dma_hal = dma_dev->data;
	dma_descriptor_t *desc;
	k_spinlock_key_t key;

	if (ch->state != RMT_CH_ACTIVE) {
		return;
	}

	/*
	 * Every filled descriptor reports a block completion; the frame
	 * is still in progress until the chain runs out or the
	 * end-of-frame arrives.
	 */
	if (status == DMA_STATUS_BLOCK && ++ch->rx_dma_blocks < RMT_RX_DMA_DESCS) {
		return;
	}

	key = k_spin_lock(&ch->lock);
	rmt_ll_rx_enable(data->hal.regs, ch->index - RMT_RX_CHANNEL_OFFSET, false);
	k_spin_unlock(&ch->lock, key);

	/* the chain ran out mid-frame: the capture does not fit the buffer */
	if (status != DMA_STATUS_COMPLETE) {
		ch->rx_overflow = true;
		rmt_rx_finish(ch);
		return;
	}

	desc = (dma_descriptor_t *)gdma_ll_rx_get_success_eof_desc_addr(dma_hal->dev,
									dma_channel / 2);
	if (desc != NULL) {
		size_t bytes = (size_t)((uint8_t *)desc->buffer - (uint8_t *)rmt_rx_dma_buf) +
			       desc->dw0.length;

		rmt_rx_store(ch, rmt_rx_dma_buf, bytes / sizeof(rmt_symbol_word_t));
	}

	rmt_rx_finish(ch);
}
#endif

int rmt_rx_configure(const struct device *dev, struct rmt_channel *ch)
{
	struct rmt_data *data = dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index - RMT_RX_CHANNEL_OFFSET;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&data->glock);
	rmt_hal_rx_channel_reset(&data->hal, id);
	k_spin_unlock(&data->glock, key);

	if (ch->intr != NULL) {
		esp_intr_free(ch->intr);
		ch->intr = NULL;
	}

	ch->with_dma = false;
#if RMT_DMA_SUPPORTED
	const struct rmt_config *config = dev->config;

	if (config->dma_dev != NULL && config->rx_dma_channel != RMT_DMA_CHANNEL_UNDEFINED &&
	    id == RMT_NUM_RX_CHANNELS - 1) {
		struct dma_block_config dma_blk = {
			.block_size = sizeof(rmt_rx_dma_buf),
			.dest_address = (uint32_t)rmt_rx_dma_buf,
			.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		};
		struct dma_config dma_cfg = {
			.dma_slot = ESP_GDMA_TRIG_PERIPH_RMT,
			.channel_direction = PERIPHERAL_TO_MEMORY,
			.block_count = 1,
			.head_block = &dma_blk,
			.user_data = ch,
			.dma_callback = rmt_rx_dma_eof_cb,
		};

		ret = dma_config(config->dma_dev, config->rx_dma_channel, &dma_cfg);
		if (ret) {
			LOG_ERR("failed to configure RX DMA channel (%d)", ret);
			return ret;
		}
		ch->with_dma = true;
		key = k_spin_lock(&ch->lock);
		rmt_ll_rx_enable_dma(regs, id, true);
		k_spin_unlock(&ch->lock, key);
	}
#endif

	if (!ch->with_dma) {
		if (esp_intr_alloc_intrstatus(soc_rmt_signals[0].irq, RMT_INTR_ALLOC_FLAGS,
					      (uint32_t)rmt_ll_get_interrupt_status_reg(regs),
					      RMT_LL_EVENT_RX_MASK(id), rmt_rx_isr, ch,
					      &ch->intr) != 0) {
			LOG_ERR("failed to install RX interrupt");
			return -ENODEV;
		}
	}

	ret = rmt_select_channel_clock(dev, ch, ch->cfg.resolution_hz);
	if (ret) {
		return ret;
	}

	rmt_ll_rx_set_mem_blocks(regs, id, 1);
	rmt_ll_rx_set_mem_owner(regs, id, RMT_LL_MEM_OWNER_HW);
#if SOC_RMT_SUPPORT_RX_PINGPONG
	rmt_ll_rx_set_limit(regs, id, RMT_PING_PONG_WORDS);
	rmt_ll_rx_enable_wrap(regs, id, true);
#endif

	if (ch->cfg.rx_carrier_demod) {
#if RMT_LL_SUPPORT(RX_DEMODULATION)
		uint32_t total_ticks;

		if (ch->cfg.carrier_hz == 0U) {
			return -EINVAL;
		}
		total_ticks = ch->resolution_hz / ch->cfg.carrier_hz;
		if (total_ticks == 0U) {
			return -EINVAL;
		}
		/*
		 * Treat anything shorter than a full carrier period as
		 * carrier: exact pulse widths miss pulses that round up
		 * one tick.
		 */
		key = k_spin_lock(&ch->lock);
		rmt_ll_rx_set_carrier_level(regs, id, !ch->cfg.idle_high);
		rmt_ll_rx_set_carrier_high_low_ticks(regs, id, total_ticks, total_ticks);
		rmt_ll_rx_enable_carrier_demodulation(regs, id, true);
		k_spin_unlock(&ch->lock, key);
#else
		return -ENOTSUP;
#endif
	} else {
#if RMT_LL_SUPPORT(RX_DEMODULATION)
		rmt_ll_rx_enable_carrier_demodulation(regs, id, false);
#endif
	}

	return 0;
}

int rmt_rx_start(const struct device *dev, struct rmt_channel *ch,
		 const struct pulse_io_rx_req *req)
{
	struct rmt_data *data = dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index - RMT_RX_CHANNEL_OFFSET;
	uint64_t filter_reg;
	uint32_t idle_reg;
	k_spinlock_key_t key;

	filter_reg = (uint64_t)ch->cfg.rx_filter_ticks * data->filter_clk_hz / ch->resolution_hz;
	if (filter_reg > RMT_LL_MAX_FILTER_VALUE) {
		return -EINVAL;
	}
	idle_reg = ch->cfg.rx_idle_threshold_ticks;
	if (idle_reg == 0U) {
		idle_reg = RMT_LL_MAX_IDLE_VALUE;
	}
	if (idle_reg > RMT_LL_MAX_IDLE_VALUE) {
		return -EINVAL;
	}

	ch->rx_buf = req->symbols;
	ch->rx_cap = req->capacity;
	ch->rx_count = 0;
	ch->rx_hw_off = 0;
	ch->rx_dma_blocks = 0;
	ch->rx_overflow = false;
	ch->result = 0;
	k_sem_reset(&ch->done);

#if RMT_DMA_SUPPORTED
	if (ch->with_dma) {
		const struct rmt_config *config = dev->config;
		int ret;

		dma_stop(config->dma_dev, config->rx_dma_channel);
		ret = dma_reload(config->dma_dev, config->rx_dma_channel, 0,
				 (uint32_t)rmt_rx_dma_buf, sizeof(rmt_rx_dma_buf));
		if (ret == 0) {
			ret = dma_start(config->dma_dev, config->rx_dma_channel);
		}
		if (ret) {
			LOG_ERR("failed to start RX DMA (%d)", ret);
			return ret;
		}
	}
#endif

	if (!ch->with_dma) {
		key = k_spin_lock(&data->glock);
		rmt_ll_clear_interrupt_status(regs, RMT_LL_EVENT_RX_MASK(id));
		rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_RX_MASK(id), true);
		k_spin_unlock(&data->glock, key);
	}

	ch->state = RMT_CH_ACTIVE;
	key = k_spin_lock(&ch->lock);
	rmt_ll_rx_reset_pointer(regs, id);
	rmt_ll_rx_set_mem_owner(regs, id, RMT_LL_MEM_OWNER_HW);
	rmt_ll_rx_set_filter_thres(regs, id, (uint32_t)filter_reg);
	rmt_ll_rx_enable_filter(regs, id, ch->cfg.rx_filter_ticks != 0U);
	rmt_ll_rx_set_idle_thres(regs, id, idle_reg);
	rmt_ll_rx_enable(regs, id, true);
	k_spin_unlock(&ch->lock, key);

	return 0;
}

void rmt_rx_halt(const struct device *dev, struct rmt_channel *ch)
{
	struct rmt_data *data = dev->data;
	rmt_soc_handle_t regs = data->hal.regs;
	uint32_t id = ch->index - RMT_RX_CHANNEL_OFFSET;
	k_spinlock_key_t key;

	ch->state = RMT_CH_READY;

	key = k_spin_lock(&ch->lock);
	rmt_ll_rx_enable(regs, id, false);
	k_spin_unlock(&ch->lock, key);

	if (!ch->with_dma) {
		key = k_spin_lock(&data->glock);
		rmt_ll_enable_interrupt(regs, RMT_LL_EVENT_RX_MASK(id), false);
		rmt_ll_clear_interrupt_status(regs, RMT_LL_EVENT_RX_MASK(id));
		k_spin_unlock(&data->glock, key);
	}
#if RMT_DMA_SUPPORTED
	else {
		const struct rmt_config *config = dev->config;

		dma_stop(config->dma_dev, config->rx_dma_channel);
	}
#endif
}
