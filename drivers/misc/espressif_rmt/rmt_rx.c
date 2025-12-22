/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_rom_gpio.h"
#include "soc/rmt_periph.h"
#include "soc/rtc.h"
#if SOC_RMT_SUPPORT_DMA
#include <hal/gdma_ll.h>
#include <hal/gdma_hal.h>
#endif
#include "hal/rmt_ll.h"
#include "rmt_private.h"

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pinctrl/pinctrl_esp32_common.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espressif_rmt_rx, CONFIG_ESPRESSIF_RMT_LOG_LEVEL);

#include <zephyr/drivers/misc/espressif_rmt/rmt.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt_rx.h>

#ifdef CONFIG_SOC_SERIES_ESP32
#include <zephyr/dt-bindings/pinctrl/esp32-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#include <zephyr/dt-bindings/pinctrl/esp32s2-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#include <zephyr/dt-bindings/pinctrl/esp32s3-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
#include <zephyr/dt-bindings/pinctrl/esp32c3-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
#include <zephyr/dt-bindings/pinctrl/esp32c6-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32H2)
#include <zephyr/dt-bindings/pinctrl/esp32h2-gpio-sigmap.h>
#endif

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

/* RMT RX Channel ID from pinmux configuration */
#define RMT_RX_CHANNEL_ID(pinmux) \
	ESP32_PIN_SIGI(pinmux) - ESP_RMT_SIG_IN0

static int rmt_del_rx_channel(rmt_channel_handle_t channel);
static int rmt_rx_demodulate_carrier(rmt_channel_handle_t channel, const rmt_carrier_config_t
	*config);
static int rmt_rx_enable(rmt_channel_handle_t channel);
static int rmt_rx_disable(rmt_channel_handle_t channel);
static void rmt_rx_default_isr(void *args);

#if SOC_RMT_SUPPORT_DMA
static void rmt_dma_rx_eof_cb(const struct device *dma_dev, void *user_data, uint32_t dma_channel,
	int status);

static int rmt_rx_init_dma_link(const struct device *dev, rmt_rx_channel_t *rx_channel,
	const rmt_rx_channel_config_t *config)
{
	const struct espressif_rmt_config *cfg = dev->config;
	rmt_symbol_word_t *dma_mem_base;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int rc;

	/* Check DMA device is available */
	if (!cfg->dma_dev) {
		LOG_ERR("DMA device is not available");
		return -ENODEV;
	}

	/* Allocate memory */
	dma_mem_base = k_aligned_alloc(sizeof(uint32_t), sizeof(rmt_symbol_word_t)
		* config->mem_block_symbols);
	if (!dma_mem_base) {
		LOG_ERR("no mem for rx DMA buffer");
		return -ENOMEM;
	}
	rx_channel->base.dma_mem_base = dma_mem_base;
	rx_channel->base.dma_mem_size = sizeof(rmt_symbol_word_t) * config->mem_block_symbols;

	/* Configure DMA */
	dma_blk.block_size = rx_channel->base.dma_mem_size;
	dma_blk.dest_address = (uint32_t)rx_channel->base.dma_mem_base;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_RMT;
	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.user_data = (void *)rx_channel;
	dma_cfg.dma_callback = rmt_dma_rx_eof_cb;
	rc = dma_config(cfg->dma_dev, cfg->rx_dma_channel, &dma_cfg);
	if (rc) {
		LOG_ERR("Failed to configure DMA channel: %" PRIu32" (%d)", cfg->rx_dma_channel,
			rc);
		return rc;
	}
	rx_channel->base.dma_dev = cfg->dma_dev;
	rx_channel->base.dma_channel = cfg->rx_dma_channel;

	return 0;
}
#endif

static int rmt_rx_register_to_group(rmt_rx_channel_t *rx_channel,
	const rmt_rx_channel_config_t *config)
{
	k_spinlock_key_t key;
	size_t mem_block_num = 0;
	/*
	 * start to search for a free channel
	 * a channel can take up its neighbour's memory block,
	 * so the neighbour channel won't work, we should skip these "invaded" ones
	 */
	int channel_scan_start = RMT_RX_CHANNEL_OFFSET_IN_GROUP;
	int channel_scan_end = RMT_RX_CHANNEL_OFFSET_IN_GROUP + SOC_RMT_RX_CANDIDATES_PER_GROUP;
	uint32_t channel_mask;
	rmt_group_t *group = NULL;
	int wanted_channel_id;
	int channel_id = -1;

#if SOC_RMT_SUPPORT_DMA
	if (rx_channel->base.with_dma) {
		/*
		 * for DMA mode, the memory block number is always 1;
		 * for non-DMA mode, memory block number is configured by user
		 */
		mem_block_num = 1;
		/* Only the last channel has the DMA capability */
		channel_scan_start = RMT_RX_CHANNEL_OFFSET_IN_GROUP
			+ SOC_RMT_RX_CANDIDATES_PER_GROUP - 1;
		rx_channel->ping_pong_symbols = 0; /* with DMA, we don't need to do ping-pong */
	} else {
#endif
		/* one channel can occupy multiple memory blocks */
		mem_block_num = config->mem_block_symbols / SOC_RMT_MEM_WORDS_PER_CHANNEL;
		if (mem_block_num * SOC_RMT_MEM_WORDS_PER_CHANNEL < config->mem_block_symbols) {
			mem_block_num++;
		}
		rx_channel->ping_pong_symbols = mem_block_num * SOC_RMT_MEM_WORDS_PER_CHANNEL / 2;
#if SOC_RMT_SUPPORT_DMA
	}
#endif
	rx_channel->base.mem_block_num = mem_block_num;

	/* search free channel and then register to the group */
	/* memory blocks used by one channel must be continuous */
	channel_mask = (1 << mem_block_num) - 1;
	for (int i = 0; i < SOC_RMT_GROUPS; i++) {
		group = rmt_acquire_group_handle(i);
		if (!group) {
			LOG_ERR("Unable to allocate memory for group");
			return -ENOMEM;
		}
		key = k_spin_lock(&group->spinlock);
		wanted_channel_id = RMT_RX_CHANNEL_ID(config->gpio_pinmux);
		for (int j = channel_scan_start; j < channel_scan_end; j++) {
			if ((!(group->occupy_mask & (channel_mask << j)))
				&& (wanted_channel_id == j - RMT_RX_CHANNEL_OFFSET_IN_GROUP)) {
				group->occupy_mask |= (channel_mask << j);
				/* the channel ID should index from 0 */
				channel_id = j - RMT_RX_CHANNEL_OFFSET_IN_GROUP;
				group->rx_channels[channel_id] = rx_channel;
				break;
			}
		}
		k_spin_unlock(&group->spinlock, key);
		if (channel_id < 0) {
			/*
			 * didn't find a capable channel in the group,
			 * don't forget to release the group handle
			 */
			rmt_release_group_handle(group);
			group = NULL;
		} else {
			rx_channel->base.channel_id = channel_id;
			rx_channel->base.channel_mask = channel_mask;
			rx_channel->base.group = group;
			break;
		}
	}
	if (channel_id < 0) {
		LOG_ERR("No rx channel available");
		return -ENOMEM;
	}

	return 0;
}

static void rmt_rx_unregister_from_group(rmt_channel_t *channel, rmt_group_t *group)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&group->spinlock);
	group->rx_channels[channel->channel_id] = NULL;
	group->occupy_mask &= ~(channel->channel_mask
		<< (channel->channel_id + RMT_RX_CHANNEL_OFFSET_IN_GROUP));
	k_spin_unlock(&group->spinlock, key);
	/* channel has a reference on group, release it now */
	rmt_release_group_handle(group);
}

static int rmt_rx_destroy(rmt_rx_channel_t *rx_channel)
{
	esp_err_t ret;
#if SOC_RMT_SUPPORT_DMA
	int rc;
#endif

	if (rx_channel->base.intr) {
		ret = esp_intr_free(rx_channel->base.intr);
		if (ret) {
			LOG_ERR("delete interrupt service failed");
			return -ENODEV;
		}
	}
#if CONFIG_ESPRESSIF_RMT_PM
	if (rx_channel->base.pm_lock) {
		ret = esp_pm_lock_delete(rx_channel->base.pm_lock);
		if (ret) {
			LOG_ERR"delete pm_lock failed");
			return -ENODEV;
		}
	}
#endif
#if SOC_RMT_SUPPORT_DMA
	if (rx_channel->base.dma_dev) {
		rc = dma_stop(rx_channel->base.dma_dev, rx_channel->base.dma_channel);
		if (rc) {
			LOG_ERR("Stopping DMA channel failed");
			return rc;
		}
	}
	if (rx_channel->base.dma_mem_base) {
		k_free(rx_channel->base.dma_mem_base);
	}
#endif
	if (rx_channel->base.group) {
		/* de-register channel from RMT group */
		rmt_rx_unregister_from_group(&rx_channel->base, rx_channel->base.group);
	}
	free(rx_channel);

	return 0;
}

int rmt_new_rx_channel(const struct device *dev, const rmt_rx_channel_config_t *config,
	rmt_channel_handle_t *ret_chan)
{
#if SOC_RMT_SUPPORT_DMA
	const struct espressif_rmt_config *cfg = dev->config;
#endif
	rmt_rx_channel_t *rx_channel = NULL;
#if SOC_RMT_SUPPORT_DMA
	bool with_dma;
	size_t num_dma_nodes = 0;
#endif
	k_spinlock_key_t key;
	uint32_t mem_caps;
	esp_err_t ret;
	int rc;

	/* Check if priority is valid */
	if (config->intr_priority) {
		if (!(config->intr_priority > 0)
			|| !(1 << (config->intr_priority) & RMT_ALLOW_INTR_PRIORITY_MASK)) {
			LOG_ERR("Invalid interrupt priority: %d", config->intr_priority);
			return -EINVAL;
		}
	}
	if (!(config && ret_chan && config->resolution_hz)) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	if (!((config->mem_block_symbols & 0x01) == 0
		&& config->mem_block_symbols >= SOC_RMT_MEM_WORDS_PER_CHANNEL)) {
		LOG_ERR("Parameter mem_block_symbols must be even and at least %d",
			SOC_RMT_MEM_WORDS_PER_CHANNEL);
		return -EINVAL;
	}
#if SOC_RMT_SUPPORT_DMA
	/* Only the last channel has DMA support */
	with_dma = (cfg->dma_dev)
		&& (cfg->rx_dma_channel != ESPRESSIF_RMT_DMA_CHANNEL_UNDEFINED)
		&& (RMT_RX_CHANNEL_ID(config->gpio_pinmux) == SOC_RMT_RX_CANDIDATES_PER_GROUP - 1);
	/* Compute number of DMA nodes */
	if (with_dma) {
		num_dma_nodes = config->mem_block_symbols
			* sizeof(rmt_symbol_word_t) / RMT_DMA_DESC_BUF_MAX_SIZE + 1;
	}
#endif
	/* malloc channel memory */
	mem_caps = RMT_MEM_ALLOC_CAPS;
#if SOC_RMT_SUPPORT_DMA
	if (with_dma) {
		/* DMA descriptors must be placed in internal SRAM */
		mem_caps |= MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
	}
#endif
#if SOC_RMT_SUPPORT_DMA
	rx_channel = heap_caps_calloc(1, sizeof(rmt_rx_channel_t) + num_dma_nodes
		* sizeof(dma_descriptor_t), mem_caps);
#else
	rx_channel = heap_caps_calloc(1, sizeof(rmt_rx_channel_t), mem_caps);
#endif
	if (!rx_channel) {
		LOG_ERR("Unable to allocate memory for rx channel");
		return -ENOMEM;
	}
#if SOC_RMT_SUPPORT_DMA
	rx_channel->base.with_dma = with_dma;
	rx_channel->num_dma_nodes = num_dma_nodes;
#endif

	/* register the channel to group */
	rc = rmt_rx_register_to_group(rx_channel, config);
	if (rc) {
		LOG_ERR("Unable to register channel");
		goto err;
	}

	rmt_group_t *group = rx_channel->base.group;
	rmt_hal_context_t *hal = &group->hal;
	int channel_id = rx_channel->base.channel_id;

	/* reset channel, make sure the RX engine is not working, and events are cleared */
	key = k_spin_lock(&group->spinlock);
	rmt_hal_rx_channel_reset(&group->hal, channel_id);
	k_spin_unlock(&group->spinlock, key);

	/* When channel receives an end-maker, a DMA in_suc_eof interrupt will be generated */
	/* So we don't rely on RMT interrupt any more, GDMA event callback is sufficient */
#if SOC_RMT_SUPPORT_DMA
	if (with_dma) {
		rc = rmt_rx_init_dma_link(dev, rx_channel, config);
		if (rc) {
			LOG_ERR("install rx DMA failed");
			goto err;
		}
	} else {
#endif
		/*
		 * RMT interrupt is mandatory if the channel doesn't use DMA
		 * --- install interrupt service
		 * interrupt is mandatory to run basic RMT transactions, so it's not lazy
		 * installed in `rmt_tx_register_event_callbacks()`
		 */
		/* 1-- Set user specified priority to `group->intr_priority` */
		if (rmt_set_intr_priority_to_group(group, config->intr_priority)) {
			LOG_ERR("intr_priority conflict");
			rc = -ENODEV;
			goto err;
		}
		/* 2-- Get interrupt allocation flag */
		int isr_flags = rmt_get_isr_flags(group);
		/* 3-- Allocate interrupt using isr_flag */
		ret = esp_intr_alloc_intrstatus(rmt_periph_signals.groups[group->group_id].irq,
			isr_flags, (uint32_t)rmt_ll_get_interrupt_status_reg(hal->regs),
			RMT_LL_EVENT_RX_MASK(channel_id), rmt_rx_default_isr, rx_channel,
			&rx_channel->base.intr);
		if (ret) {
			LOG_ERR("install rx interrupt failed");
			rc = -ENODEV;
			goto err;
		}
#if SOC_RMT_SUPPORT_DMA
	}
#endif

	/* select the clock source */
	rc = rmt_select_periph_clock(&rx_channel->base, config->clk_src);
	if (rc) {
		LOG_ERR("set group clock failed");
		goto err;
	}
	/* set channel clock resolution, find the divider to get the closest resolution */
	uint32_t real_div = (group->resolution_hz
		+ config->resolution_hz / 2) / config->resolution_hz;

	rmt_ll_rx_set_channel_clock_div(hal->regs, channel_id, real_div);
	/* resolution loss due to division, calculate the real resolution */
	rx_channel->base.resolution_hz = group->resolution_hz / real_div;
	if (rx_channel->base.resolution_hz != config->resolution_hz) {
		LOG_WRN("channel resolution loss, real=%"PRIu32, rx_channel->base.resolution_hz);
	}

	rx_channel->filter_clock_resolution_hz = group->resolution_hz;
	/*
	 * On esp32 and esp32s2, the counting clock used by the RX filter always comes from
	 * APB clock no matter what the clock source is used by the RMT channel as the "core"
	 * clock
	 */
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
	esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_APB, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
		&rx_channel->filter_clock_resolution_hz);
#endif

	rmt_ll_rx_set_mem_blocks(hal->regs, channel_id, rx_channel->base.mem_block_num);
	rmt_ll_rx_set_mem_owner(hal->regs, channel_id, RMT_LL_MEM_OWNER_HW);
#if SOC_RMT_SUPPORT_RX_PINGPONG
	rmt_ll_rx_set_limit(hal->regs, channel_id, rx_channel->ping_pong_symbols);
	/* always enable rx wrap, both DMA mode and ping-pong mode rely this feature */
	rmt_ll_rx_enable_wrap(hal->regs, channel_id, true);
#endif
#if SOC_RMT_SUPPORT_RX_DEMODULATION
	/* disable carrier demodulation by default, can reenable by `rmt_apply_carrier()` */
	rmt_ll_rx_enable_carrier_demodulation(hal->regs, channel_id, false);
#endif

	/* initialize other members of rx channel */
	rx_channel->base.fsm = ATOMIC_INIT(RMT_FSM_INIT);
	rx_channel->base.direction = RMT_CHANNEL_DIRECTION_RX;
	rx_channel->base.hw_mem_base = &RMTMEM.channels[channel_id
		+ RMT_RX_CHANNEL_OFFSET_IN_GROUP].symbols[0];
	/* polymorphic methods */
	rx_channel->base.del = rmt_del_rx_channel;
	rx_channel->base.set_carrier_action = rmt_rx_demodulate_carrier;
	rx_channel->base.enable = rmt_rx_enable;
	rx_channel->base.disable = rmt_rx_disable;
	/* return general channel handle */
	*ret_chan = &rx_channel->base;
	LOG_DBG("new rx channel(%d,%d) at %p, gpio=%d, res=%"PRIu32"Hz, hw_mem_base=%p, "
		"ping_pong_size=%d", group->group_id, channel_id, rx_channel,
		ESP32_PIN_NUM(config->gpio_pinmux), rx_channel->base.resolution_hz,
		rx_channel->base.hw_mem_base, rx_channel->ping_pong_symbols);

	return 0;

err:
	if (rx_channel) {
		rmt_rx_destroy(rx_channel);
	}

	return rc;
}

static int rmt_del_rx_channel(rmt_channel_handle_t channel)
{
	rmt_rx_channel_t *rx_chan = __containerof(channel, rmt_rx_channel_t, base);
	rmt_group_t *group = channel->group;
	int rc;

	if (atomic_get(&channel->fsm) != RMT_FSM_INIT) {
		LOG_ERR("channel not in init state");
		return -ENODEV;
	}

	/* recycle memory resource */
	LOG_DBG("del rx channel(%d,%d)", group->group_id, channel->channel_id);
	rc = rmt_rx_destroy(rx_chan);
	if (rc) {
		LOG_ERR("destroy rx channel failed");
		return rc;
	}

	return 0;
}

int rmt_rx_register_event_callbacks(rmt_channel_handle_t channel,
	const rmt_rx_event_callbacks_t *cbs, void *user_data)
{
	rmt_rx_channel_t *rx_chan;

	if (!(channel && cbs)) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	if (channel->direction != RMT_CHANNEL_DIRECTION_RX) {
		LOG_ERR("Invalid channel direction");
		return -EINVAL;
	}
	rx_chan = __containerof(channel, rmt_rx_channel_t, base);

#if CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE
	if (cbs->on_recv_done) {
		if (!esp_ptr_in_iram(cbs->on_recv_done)) {
			LOG_ERR("on_recv_done callback not in IRAM");
			return -EINVAL;
		}
	}
	if (user_data) {
		if (!esp_ptr_internal(user_data)) {
			LOG_ERR("user context not in internal RAM");
			return -EINVAL;
		}
	}
#endif

	rx_chan->on_recv_done = cbs->on_recv_done;
	rx_chan->user_data = user_data;

	return 0;
}

int rmt_receive(rmt_channel_handle_t channel, void *buffer, size_t buffer_size,
	const rmt_receive_config_t *config)
{
	rmt_rx_channel_t *rx_chan;
	k_spinlock_key_t key;
#if SOC_RMT_SUPPORT_DMA
	int rc;
#endif

	if (!(channel && buffer && buffer_size && config)) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	if (channel->direction != RMT_CHANNEL_DIRECTION_RX) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	rx_chan = __containerof(channel, rmt_rx_channel_t, base);

#if SOC_RMT_SUPPORT_DMA
	if (channel->dma_dev) {
		if (!esp_ptr_internal(buffer)) {
			LOG_ERR("Buffer must locate in internal RAM for DMA use");
			return -EINVAL;
		}
		if (!(buffer_size <= rx_chan->num_dma_nodes * RMT_DMA_DESC_BUF_MAX_SIZE)) {
			LOG_ERR("buffer size exceeds DMA capacity");
			return -EINVAL;
		}
	}
#endif
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	uint32_t filter_reg_value = ((uint64_t)rx_chan->filter_clock_resolution_hz
		* config->signal_range_min_ns) / 1000000000UL;
	uint32_t idle_reg_value = ((uint64_t)channel->resolution_hz
		* config->signal_range_max_ns) / 1000000000UL;

	if (filter_reg_value > RMT_LL_MAX_FILTER_VALUE) {
		LOG_ERR("signal_range_min_ns too big");
		return -EINVAL;
	}
	if (idle_reg_value > RMT_LL_MAX_IDLE_VALUE) {
		LOG_ERR("signal_range_max_ns too big");
		return -EINVAL;
	}

	/* check if we're in a proper state to start the receiver */
	if (!atomic_cas(&channel->fsm, RMT_FSM_ENABLE, RMT_FSM_RUN_WAIT)) {
		LOG_ERR("channel not in enable state");
		return -ENODEV;
	}

	/* fill in the transaction descriptor */
	rmt_rx_trans_desc_t *t = &rx_chan->trans_desc;

	t->buffer = buffer;
	t->buffer_size = buffer_size;
	t->received_symbol_num = 0;
	t->copy_dest_off = 0;

#if SOC_RMT_SUPPORT_DMA
	if (channel->dma_dev) {
		rc = dma_reload(channel->dma_dev, channel->dma_channel, 0,
			(uint32_t)channel->dma_mem_base, channel->dma_mem_size);
		if (rc) {
			LOG_ERR("Reloading DMA channel failed");
			return -ENODEV;
		}
		rc = dma_start(channel->dma_dev, channel->dma_channel);
		if (rc) {
			LOG_ERR("Starting DMA channel failed");
			return -ENODEV;
		}
	}
#endif

	rx_chan->mem_off = 0;
	key = k_spin_lock(&channel->spinlock);
	/* reset memory writer offset */
	rmt_ll_rx_reset_pointer(hal->regs, channel->channel_id);
	rmt_ll_rx_set_mem_owner(hal->regs, channel->channel_id, RMT_LL_MEM_OWNER_HW);
	/* set sampling parameters of incoming signals */
	rmt_ll_rx_set_filter_thres(hal->regs, channel->channel_id, filter_reg_value);
	rmt_ll_rx_enable_filter(hal->regs, channel->channel_id, config->signal_range_min_ns != 0);
	rmt_ll_rx_set_idle_thres(hal->regs, channel->channel_id, idle_reg_value);
	/* turn on RMT RX machine */
	rmt_ll_rx_enable(hal->regs, channel->channel_id, true);
	k_spin_unlock(&channel->spinlock, key);

	/* saying we're in running state, this state will last until the receiving is done */
	/* i.e., we will switch back to the enable state in the receive done interrupt handler */
	atomic_set(&channel->fsm, RMT_FSM_RUN);

	return 0;
}

static int rmt_rx_demodulate_carrier(rmt_channel_handle_t channel,
	const rmt_carrier_config_t *config)
{
#if !SOC_RMT_SUPPORT_RX_DEMODULATION
	LOG_ERR("rx demodulation not supported");
	return -ENODEV;
#else
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	uint32_t real_frequency = 0;
	k_spinlock_key_t key;

	if (config && config->frequency_hz) {
		/*
		 * carrier demodulation module works base on channel clock
		 * (this is different from TX carrier modulation mode)
		 * Note this division operation will lose precision
		 */
		uint32_t total_ticks = channel->resolution_hz / config->frequency_hz;
		uint32_t high_ticks = total_ticks * config->duty_cycle;
		uint32_t low_ticks = total_ticks - high_ticks;

		key = k_spin_lock(&channel->spinlock);
		rmt_ll_rx_set_carrier_level(hal->regs, channel->channel_id,
			!config->flags.polarity_active_low);
		rmt_ll_rx_set_carrier_high_low_ticks(hal->regs, channel->channel_id, high_ticks,
			low_ticks);
		k_spin_unlock(&channel->spinlock, key);
		/* save real carrier frequency */
		real_frequency = channel->resolution_hz / (high_ticks + low_ticks);
	}

	/* enable/disable carrier demodulation */
	key = k_spin_lock(&channel->spinlock);
	rmt_ll_rx_enable_carrier_demodulation(hal->regs, channel->channel_id, real_frequency > 0);
	k_spin_unlock(&channel->spinlock, key);

	if (real_frequency > 0) {
		LOG_DBG("enable carrier demodulation for channel(%d,%d), freq=%"PRIu32"Hz",
			group->group_id, channel->channel_id, real_frequency);
	} else {
		LOG_DBG("disable carrier demodulation for channel(%d, %d)", group->group_id,
			channel->channel_id);
	}

	return 0;
#endif
}

static int rmt_rx_enable(rmt_channel_handle_t channel)
{
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	k_spinlock_key_t key;

	/* can only enable the channel when it's in "init" state */
	if (!atomic_cas(&channel->fsm, RMT_FSM_INIT, RMT_FSM_ENABLE_WAIT)) {
		LOG_ERR("channel not in init state");
		return -ENODEV;
	}

#if CONFIG_ESPRESSIF_RMT_PM
	/* acquire power manager lock */
	if (channel->pm_lock) {
		esp_pm_lock_acquire(channel->pm_lock);
	}
#endif
#if SOC_RMT_SUPPORT_DMA
	if (channel->dma_dev) {
		/* enable the DMA access mode */
		key = k_spin_lock(&channel->spinlock);
		rmt_ll_rx_enable_dma(hal->regs, channel->channel_id, true);
		k_spin_unlock(&channel->spinlock, key);
	} else {
#endif
		key = k_spin_lock(&group->spinlock);
		rmt_ll_enable_interrupt(hal->regs, RMT_LL_EVENT_RX_MASK(channel->channel_id), true);
		k_spin_unlock(&group->spinlock, key);
#if SOC_RMT_SUPPORT_DMA
	}
#endif

	atomic_set(&channel->fsm, RMT_FSM_ENABLE);

	return 0;
}

static int rmt_rx_disable(rmt_channel_handle_t channel)
{
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	bool valid_state = false;
	k_spinlock_key_t key;
#if SOC_RMT_SUPPORT_DMA
	int rc;
#endif

	if (atomic_cas(&channel->fsm, RMT_FSM_ENABLE, RMT_FSM_INIT_WAIT)) {
		valid_state = true;
	}
	if (atomic_cas(&channel->fsm, RMT_FSM_RUN, RMT_FSM_INIT_WAIT)) {
		valid_state = true;
	}
	if (!valid_state) {
		LOG_ERR("Channel can't be disabled in current state");
		return -ENODEV;
	}

	key = k_spin_lock(&channel->spinlock);
	rmt_ll_rx_enable(hal->regs, channel->channel_id, false);
	k_spin_unlock(&channel->spinlock, key);

#if SOC_RMT_SUPPORT_DMA
	if (channel->dma_dev) {
		rc = dma_stop(channel->dma_dev, channel->dma_channel);
		if (rc) {
			LOG_ERR("Stopping DMA channel failed");
			return rc;
		}
		key = k_spin_lock(&channel->spinlock);
		rmt_ll_rx_enable_dma(hal->regs, channel->channel_id, false);
		k_spin_unlock(&channel->spinlock, key);
	} else {
#endif
		key = k_spin_lock(&group->spinlock);
		rmt_ll_enable_interrupt(hal->regs, RMT_LL_EVENT_RX_MASK(channel->channel_id),
			false);
		rmt_ll_clear_interrupt_status(hal->regs, RMT_LL_EVENT_RX_MASK(channel->channel_id));
		k_spin_unlock(&group->spinlock, key);
#if SOC_RMT_SUPPORT_DMA
	}
#endif

#if CONFIG_ESPRESSIF_RMT_PM
	/* release power manager lock */
	if (channel->pm_lock) {
		esp_pm_lock_release(channel->pm_lock);
	}
#endif

	/* now we can switch the state to init */
	atomic_set(&channel->fsm, RMT_FSM_INIT);

	return 0;
}

static size_t IRAM_ATTR rmt_copy_symbols(rmt_symbol_word_t *symbol_stream, size_t symbol_num,
	void *buffer, size_t offset, size_t buffer_size)
{
	size_t mem_want = symbol_num * sizeof(rmt_symbol_word_t);
	size_t mem_have = buffer_size - offset;
	size_t copy_size = MIN(mem_want, mem_have);

	/* do memory copy */
	memcpy(((char *)buffer) + offset, symbol_stream, copy_size);

	return copy_size;
}

static bool IRAM_ATTR rmt_isr_handle_rx_done(rmt_rx_channel_t *rx_chan)
{
	rmt_channel_t *channel = &rx_chan->base;
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	uint32_t channel_id = channel->channel_id;
	rmt_rx_trans_desc_t *trans_desc = &rx_chan->trans_desc;
	bool need_yield = false;

	rmt_ll_clear_interrupt_status(hal->regs, RMT_LL_EVENT_RX_DONE(channel_id));

	k_spinlock_key_t key = k_spin_lock(&channel->spinlock);
	/*
	 * disable the RX engine,
	 * it will be enabled again when next time user calls `rmt_receive()`
	 */
	rmt_ll_rx_enable(hal->regs, channel_id, false);
	uint32_t offset = rmt_ll_rx_get_memory_writer_offset(hal->regs, channel_id);
	/* sanity check */
	assert(offset >= rx_chan->mem_off);
	rmt_ll_rx_set_mem_owner(hal->regs, channel_id, RMT_LL_MEM_OWNER_SW);
	/* copy the symbols to user space */
	size_t stream_symbols = offset - rx_chan->mem_off;
	size_t copy_size = rmt_copy_symbols(channel->hw_mem_base + rx_chan->mem_off, stream_symbols,
		trans_desc->buffer, trans_desc->copy_dest_off, trans_desc->buffer_size);
	rmt_ll_rx_set_mem_owner(hal->regs, channel_id, RMT_LL_MEM_OWNER_HW);
	k_spin_unlock(&channel->spinlock, key);

#if !SOC_RMT_SUPPORT_RX_PINGPONG
	/*
	 * for chips doesn't support ping-pong RX, we should check whether the receiver has
	 * encountered with a long frame, whose length is longer than the channel capacity
	 */
	if (rmt_ll_rx_get_interrupt_status_raw(hal->regs, channel_id)
			& RMT_LL_EVENT_RX_ERROR(channel_id)) {
		key = k_spin_lock(&channel->spinlock);
		rmt_ll_rx_reset_pointer(hal->regs, channel_id);
		k_spin_unlock(&channel->spinlock, key);
		/*
		 * this clear operation can only take effect after we copy out the received
		 * data and reset the pointer
		 */
		rmt_ll_clear_interrupt_status(hal->regs, RMT_LL_EVENT_RX_ERROR(channel_id));
		ESP_DRAM_LOGE("rmt", "hw buffer too small, received symbols truncated");
	}
#endif

	/* check whether all symbols are copied */
	if (copy_size != stream_symbols * sizeof(rmt_symbol_word_t)) {
		ESP_DRAM_LOGE("rmt", "user buffer too small, received symbols truncated");
	}
	trans_desc->copy_dest_off += copy_size;
	trans_desc->received_symbol_num += copy_size / sizeof(rmt_symbol_word_t);
	/*
	 * switch back to the enable state, then user can call `rmt_receive` to start
	 * a new receive
	 */
	atomic_set(&channel->fsm, RMT_FSM_ENABLE);

	/* notify the user with receive RMT symbols */
	if (rx_chan->on_recv_done) {
		rmt_rx_done_event_data_t edata = {
			.received_symbols = trans_desc->buffer,
			.num_symbols = trans_desc->received_symbol_num,
		};
		if (rx_chan->on_recv_done(channel, &edata, rx_chan->user_data)) {
			need_yield = true;
		}
	}

	return need_yield;
}

#if SOC_RMT_SUPPORT_RX_PINGPONG
static bool IRAM_ATTR rmt_isr_handle_rx_threshold(rmt_rx_channel_t *rx_chan)
{
	rmt_channel_t *channel = &rx_chan->base;
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	uint32_t channel_id = channel->channel_id;
	rmt_rx_trans_desc_t *trans_desc = &rx_chan->trans_desc;
	k_spinlock_key_t key;

	rmt_ll_clear_interrupt_status(hal->regs, RMT_LL_EVENT_RX_THRES(channel_id));

	key = k_spin_lock(&channel->spinlock);
	rmt_ll_rx_set_mem_owner(hal->regs, channel_id, RMT_LL_MEM_OWNER_SW);
	/* copy the symbols to user space */
	size_t copy_size = rmt_copy_symbols(channel->hw_mem_base + rx_chan->mem_off,
		rx_chan->ping_pong_symbols, trans_desc->buffer, trans_desc->copy_dest_off,
		trans_desc->buffer_size);
	rmt_ll_rx_set_mem_owner(hal->regs, channel_id, RMT_LL_MEM_OWNER_HW);
	k_spin_unlock(&channel->spinlock, key);

	/* check whether all symbols are copied */
	if (copy_size != rx_chan->ping_pong_symbols * sizeof(rmt_symbol_word_t)) {
		ESP_DRAM_LOGE("rmt", "received symbols truncated");
	}
	trans_desc->copy_dest_off += copy_size;
	trans_desc->received_symbol_num += copy_size / sizeof(rmt_symbol_word_t);
	/* update the hw memory offset, where stores the next RMT symbols to copy */
	rx_chan->mem_off = rx_chan->ping_pong_symbols - rx_chan->mem_off;

	return false;
}
#endif

static void IRAM_ATTR rmt_rx_default_isr(void *args)
{
	rmt_rx_channel_t *rx_chan = (rmt_rx_channel_t *)args;
	rmt_channel_t *channel = &rx_chan->base;
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	uint32_t channel_id = channel->channel_id;

	uint32_t status = rmt_ll_rx_get_interrupt_status(hal->regs, channel_id);

#if SOC_RMT_SUPPORT_RX_PINGPONG
	/* RX threshold interrupt */
	if (status & RMT_LL_EVENT_RX_THRES(channel_id)) {
		rmt_isr_handle_rx_threshold(rx_chan);
	}
#endif

	/* RX end interrupt */
	if (status & RMT_LL_EVENT_RX_DONE(channel_id)) {
		rmt_isr_handle_rx_done(rx_chan);
	}
}

#if SOC_RMT_SUPPORT_DMA
static void rmt_dma_rx_eof_cb(const struct device *dma_dev, void *user_data, uint32_t dma_channel,
	int status)
{
	bool need_yield = false;
	rmt_rx_channel_t *rx_chan = (rmt_rx_channel_t *)user_data;
	rmt_channel_t *channel = &rx_chan->base;
	rmt_group_t *group = channel->group;
	rmt_hal_context_t *hal = &group->hal;
	uint32_t channel_id = channel->channel_id;
	gdma_hal_context_t *dma_hal = dma_dev->data;
	dma_descriptor_t *desc;

	k_spinlock_key_t key = k_spin_lock(&channel->spinlock);
	/* disable the RX engine, it will be enabled again in the next `rmt_receive()` */
	rmt_ll_rx_enable(hal->regs, channel_id, false);
	k_spin_unlock(&channel->spinlock, key);

	/*
	 * switch back to the enable state, then user can call `rmt_receive` to start
	 * a new receive
	 */
	atomic_set(&channel->fsm, RMT_FSM_ENABLE);

	if (rx_chan->on_recv_done) {
		/* Get actual transferred bytes from DMA descriptor */
		desc = (dma_descriptor_t *)gdma_ll_rx_get_success_eof_desc_addr(dma_hal->dev,
			dma_channel / 2);
		if (!desc) {
			LOG_ERR("DMA descriptor not found");
			return;
		}
		rmt_rx_done_event_data_t edata = {
			.received_symbols = rx_chan->base.dma_mem_base,
			.num_symbols = desc->dw0.length / sizeof(uint32_t)
		};
		if (rx_chan->on_recv_done(channel, &edata, rx_chan->user_data)) {
			need_yield = true;
		}
	}
}
#endif
