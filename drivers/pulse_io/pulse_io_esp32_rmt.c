/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rmt

#include "pulse_io_esp32_rmt.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl/pinctrl_esp32_common.h>
#include <zephyr/logging/log.h>

#include <esp_private/esp_clk_tree_common.h>

#ifdef CONFIG_SOC_SERIES_ESP32
#include <zephyr/dt-bindings/pinctrl/esp32-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#include <zephyr/dt-bindings/pinctrl/esp32s2-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#include <zephyr/dt-bindings/pinctrl/esp32s3-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
#include <zephyr/dt-bindings/pinctrl/esp32c3-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C5)
#include <zephyr/dt-bindings/pinctrl/esp32c5-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
#include <zephyr/dt-bindings/pinctrl/esp32c6-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32H2)
#include <zephyr/dt-bindings/pinctrl/esp32h2-gpio-sigmap.h>
#elif defined(CONFIG_SOC_SERIES_ESP32P4)
#include <zephyr/dt-bindings/pinctrl/esp32p4-gpio-sigmap.h>
#endif

LOG_MODULE_REGISTER(pulse_io_esp32_rmt, CONFIG_PULSE_IO_LOG_LEVEL);

static bool rmt_channel_is_tx_capable(uint8_t index)
{
	return index < RMT_NUM_TX_CHANNELS;
}

static bool rmt_channel_is_rx_capable(uint8_t index)
{
	return index >= RMT_RX_CHANNEL_OFFSET &&
	       index < RMT_RX_CHANNEL_OFFSET + RMT_NUM_RX_CHANNELS;
}

static bool rmt_channel_pin_present(const struct pinctrl_dev_config *pcfg, enum pulse_io_dir dir,
				    int rel_id)
{
	for (uint8_t s = 0; s < pcfg->state_cnt; s++) {
		const struct pinctrl_state *state = &pcfg->states[s];

		if (state->id != PINCTRL_STATE_DEFAULT) {
			continue;
		}
		for (uint8_t p = 0; p < state->pin_cnt; p++) {
			uint32_t mux = state->pins[p].pinmux;

			if (dir == PULSE_IO_DIR_TX &&
			    ESP32_PIN_SIGO(mux) == ESP_RMT_SIG_OUT0 + rel_id) {
				return true;
			}
			if (dir == PULSE_IO_DIR_RX &&
			    ESP32_PIN_SIGI(mux) == ESP_RMT_SIG_IN0 + rel_id) {
				return true;
			}
		}
	}
	return false;
}

static struct rmt_channel *rmt_to_channel(const struct device *dev, struct pulse_io_channel *handle)
{
	struct rmt_data *data = dev->data;
	struct rmt_channel *ch = (struct rmt_channel *)handle;

	if (ch < &data->channels[0] || ch >= &data->channels[RMT_NUM_CHANNELS]) {
		return NULL;
	}
	if (((uintptr_t)ch - (uintptr_t)&data->channels[0]) % sizeof(*ch) != 0) {
		return NULL;
	}
	return ch;
}

int rmt_select_channel_clock(const struct device *dev, struct rmt_channel *ch,
			     uint32_t resolution_hz)
{
	struct rmt_data *data = dev->data;
	rmt_clock_source_t clk_src = RMT_CLK_SRC_DEFAULT;
	uint32_t real_div;
	k_spinlock_key_t key;

	if (resolution_hz == 0U) {
		return -EINVAL;
	}

	if (esp_clk_tree_enable_src((soc_module_clk_t)clk_src, true) != 0) {
		LOG_ERR("clock source enable failed");
		return -EIO;
	}

#if RMT_CHANNEL_CLK_INDEPENDENT
	key = k_spin_lock(&data->glock);
	if (data->group_resolution_hz == 0U) {
		data->group_resolution_hz = data->src_clk_hz;
	}
	rmt_ll_set_group_clock_src(data->hal.regs, ch->index, clk_src, 1, 1, 0);
	rmt_ll_enable_group_clock(data->hal.regs, true);
	k_spin_unlock(&data->glock, key);
	real_div = (data->group_resolution_hz + resolution_hz / 2U) / resolution_hz;
	if (real_div == 0U || real_div > RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE) {
		LOG_ERR("channel prescale out of range");
		return -EINVAL;
	}
#else
	uint32_t group_prescale = 0;
	uint32_t group_resolution_hz = 0;
	bool conflict = false;

	real_div = 0;
	key = k_spin_lock(&data->glock);
	if (data->group_resolution_hz == 0U) {
		while (++group_prescale <= RMT_LL_GROUP_CLOCK_MAX_INTEGER_PRESCALE) {
			group_resolution_hz = data->src_clk_hz / group_prescale;
			real_div = (group_resolution_hz + resolution_hz / 2U) / resolution_hz;
			if (real_div > 0U && real_div <= RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE) {
				break;
			}
		}
		if (group_prescale > RMT_LL_GROUP_CLOCK_MAX_INTEGER_PRESCALE) {
			conflict = true;
		} else {
			data->group_resolution_hz = group_resolution_hz;
			rmt_ll_set_group_clock_src(data->hal.regs, ch->index, clk_src,
						   group_prescale, 1, 0);
			rmt_ll_enable_group_clock(data->hal.regs, true);
		}
	} else {
		real_div = (data->group_resolution_hz + resolution_hz / 2U) / resolution_hz;
		if (real_div == 0U || real_div > RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE) {
			conflict = true;
		}
	}
	k_spin_unlock(&data->glock, key);
	if (conflict) {
		LOG_ERR("no prescaler setting reaches %u Hz", resolution_hz);
		return -EINVAL;
	}
#endif

	if (ch->cfg.dir == PULSE_IO_DIR_TX) {
		rmt_ll_tx_set_channel_clock_div(data->hal.regs, ch->index, real_div);
	} else {
		rmt_ll_rx_set_channel_clock_div(data->hal.regs, ch->index - RMT_RX_CHANNEL_OFFSET,
						real_div);
	}
	ch->resolution_hz = data->group_resolution_hz / real_div;
	if (ch->resolution_hz != resolution_hz) {
		LOG_WRN("channel %u resolution set to %u Hz", ch->index, ch->resolution_hz);
	}

	return 0;
}

static int rmt_get_capabilities(const struct device *dev, struct pulse_io_caps *caps)
{
	struct rmt_data *data = dev->data;
	uint32_t max_div;

#if RMT_CHANNEL_CLK_INDEPENDENT
	max_div = RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE;
#else
	max_div = RMT_LL_GROUP_CLOCK_MAX_INTEGER_PRESCALE * RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE;
#endif

	*caps = (struct pulse_io_caps){
		.modes = PULSE_IO_MODE_SYMBOL | PULSE_IO_MODE_CELL,
		.supports_tx = true,
		.supports_rx = true,
		.min_tick_ns = DIV_ROUND_UP(NSEC_PER_SEC, data->src_clk_hz),
		.max_tick_ns = (uint32_t)(((uint64_t)NSEC_PER_SEC * max_div) / data->src_clk_hz),
		.max_duration_ticks = RMT_DURATION_MAX,
		.num_channels = RMT_NUM_CHANNELS,
		.tx_channel_mask = BIT_MASK(RMT_NUM_TX_CHANNELS) << RMT_TX_CHANNEL_OFFSET,
		.rx_channel_mask = BIT_MASK(RMT_NUM_RX_CHANNELS) << RMT_RX_CHANNEL_OFFSET,
		.tx_min_chunk_symbols = RMT_PING_PONG_WORDS * 2,
		.tx_max_streaming = SIZE_MAX,
#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
		.tx_loop_max = UINT32_MAX - 1,
#else
		.tx_loop_max = 1,
#endif
#if SOC_RMT_SUPPORT_TX_LOOP_AUTO_STOP
		.tx_loop_auto_stop = true,
#endif
		.tx_carrier = true,
		.rx_idle_threshold_max = RMT_LL_MAX_IDLE_VALUE,
		.rx_filter_max_ticks = RMT_LL_MAX_FILTER_VALUE,
#if RMT_LL_SUPPORT(RX_DEMODULATION)
		.rx_carrier_demod = true,
#endif
#if SOC_RMT_SUPPORT_RX_PINGPONG
		.rx_streaming = true,
#endif
		.cell_duty_bits = 15,
		.cell_period_per_chunk = true,
	};

	return 0;
}

static int rmt_channel_get(const struct device *dev, uint8_t channel_idx,
			   struct pulse_io_channel **chan)
{
	struct rmt_data *data = dev->data;
	struct rmt_channel *ch;
	k_spinlock_key_t key;
	int ret = 0;

	if (channel_idx >= RMT_NUM_CHANNELS) {
		return -ENODEV;
	}
	ch = &data->channels[channel_idx];

	key = k_spin_lock(&data->glock);
	if (ch->state != RMT_CH_FREE) {
		ret = -EBUSY;
	} else {
		ch->state = RMT_CH_OPEN;
	}
	k_spin_unlock(&data->glock, key);
	if (ret) {
		return ret;
	}

	*chan = (struct pulse_io_channel *)ch;
	return 0;
}

static int rmt_channel_release(const struct device *dev, struct pulse_io_channel *chan)
{
	struct rmt_data *data = dev->data;
	struct rmt_channel *ch = rmt_to_channel(dev, chan);
	k_spinlock_key_t key;

	if (ch == NULL || ch->state == RMT_CH_FREE) {
		return -EINVAL;
	}

	if (ch->state == RMT_CH_ACTIVE) {
		if (ch->cfg.dir == PULSE_IO_DIR_TX) {
			rmt_tx_halt(dev, ch);
		} else {
			rmt_rx_halt(dev, ch);
		}
		k_sem_reset(&ch->done);
	}

	if (ch->intr != NULL) {
		esp_intr_free(ch->intr);
		ch->intr = NULL;
	}

	key = k_spin_lock(&data->glock);
	ch->state = RMT_CH_FREE;
	k_spin_unlock(&data->glock, key);

	return 0;
}

static int rmt_channel_configure(const struct device *dev, struct pulse_io_channel *chan,
				 const struct pulse_io_config *cfg)
{
	const struct rmt_config *config = dev->config;
	struct rmt_channel *ch = rmt_to_channel(dev, chan);
	int rel_id;
	int ret;

	if (ch == NULL || cfg == NULL) {
		return -EINVAL;
	}
	if (ch->state != RMT_CH_OPEN && ch->state != RMT_CH_READY) {
		return -EINVAL;
	}
	if (cfg->mode != PULSE_IO_MODE_SYMBOL && cfg->mode != PULSE_IO_MODE_CELL) {
		return -ENOTSUP;
	}
	if (cfg->dir == PULSE_IO_DIR_TX) {
		if (!rmt_channel_is_tx_capable(ch->index)) {
			return -ENOTSUP;
		}
		rel_id = ch->index;
	} else {
		if (!rmt_channel_is_rx_capable(ch->index)) {
			return -ENOTSUP;
		}
		if (cfg->mode == PULSE_IO_MODE_CELL) {
			return -ENOTSUP;
		}
		rel_id = ch->index - RMT_RX_CHANNEL_OFFSET;
	}
	if (cfg->mode == PULSE_IO_MODE_CELL &&
	    (cfg->cell_period_ticks == 0U || cfg->cell_period_ticks > RMT_DURATION_MAX)) {
		return -EINVAL;
	}
	if (!rmt_channel_pin_present(config->pcfg, cfg->dir, rel_id)) {
		LOG_DBG("channel %u has no %s pin in the default pinctrl state", ch->index,
			cfg->dir == PULSE_IO_DIR_TX ? "RMT_OUT" : "RMT_IN");
		return -EINVAL;
	}

	ch->cfg = *cfg;

	if (cfg->dir == PULSE_IO_DIR_TX) {
		ret = rmt_tx_configure(dev, ch);
	} else {
		ret = rmt_rx_configure(dev, ch);
	}
	if (ret) {
		return ret;
	}

	ch->state = RMT_CH_READY;
	return 0;
}

static int rmt_transmit_sync(const struct device *dev, struct pulse_io_channel *chan,
			     const struct pulse_io_tx_req *req, k_timeout_t timeout)
{
	struct rmt_channel *ch = rmt_to_channel(dev, chan);
	int ret;

	if (ch == NULL || req == NULL || req->symbols == NULL || req->count == 0U) {
		return -EINVAL;
	}
	if (ch->state != RMT_CH_READY || ch->cfg.dir != PULSE_IO_DIR_TX) {
		return -EINVAL;
	}
	if (req->loop_count == UINT32_MAX) {
		return -ENOTSUP;
	}

	ret = rmt_tx_start(dev, ch, req);
	if (ret) {
		return ret;
	}

	ret = k_sem_take(&ch->done, timeout);
	if (ret != 0) {
		rmt_tx_halt(dev, ch);
		k_sem_reset(&ch->done);
		return -ETIMEDOUT;
	}

	return ch->result;
}

static int rmt_receive_sync(const struct device *dev, struct pulse_io_channel *chan,
			    const struct pulse_io_rx_req *req, size_t *count, k_timeout_t timeout)
{
	struct rmt_channel *ch = rmt_to_channel(dev, chan);
	int ret;

	if (ch == NULL || req == NULL || req->symbols == NULL || req->capacity == 0U ||
	    count == NULL) {
		return -EINVAL;
	}
	if (ch->state != RMT_CH_READY || ch->cfg.dir != PULSE_IO_DIR_RX) {
		return -EINVAL;
	}

	ret = rmt_rx_start(dev, ch, req);
	if (ret) {
		return ret;
	}

	ret = k_sem_take(&ch->done, timeout);
	if (ret != 0) {
		rmt_rx_halt(dev, ch);
		k_sem_reset(&ch->done);
		return -ETIMEDOUT;
	}

	*count = ch->rx_count;
	return ch->result;
}

static int rmt_stop(const struct device *dev, struct pulse_io_channel *chan)
{
	struct rmt_channel *ch = rmt_to_channel(dev, chan);

	if (ch == NULL || ch->state == RMT_CH_FREE) {
		return -EINVAL;
	}
	if (ch->state != RMT_CH_ACTIVE) {
		return 0;
	}

	if (ch->cfg.dir == PULSE_IO_DIR_TX) {
		rmt_tx_halt(dev, ch);
	} else {
		rmt_rx_halt(dev, ch);
	}
	ch->result = -ECANCELED;
	k_sem_give(&ch->done);

	return 0;
}

static int rmt_init(const struct device *dev)
{
	const struct rmt_config *config = dev->config;
	struct rmt_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock device not ready");
		return -ENODEV;
	}
#if RMT_DMA_SUPPORTED
	if (config->dma_dev != NULL && !device_is_ready(config->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}
#endif

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("failed to configure RMT pins");
		return ret;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		LOG_ERR("failed to enable RMT clock");
		return ret;
	}
	rmt_hal_init(&data->hal);

	if (esp_clk_tree_src_get_freq_hz((soc_module_clk_t)RMT_CLK_SRC_DEFAULT,
					 ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
					 &data->src_clk_hz) != 0) {
		LOG_ERR("failed to query RMT clock source frequency");
		return -EIO;
	}

	data->filter_clk_hz = data->src_clk_hz;
#if defined(CONFIG_SOC_ESP32) || defined(CONFIG_SOC_ESP32S2)
	esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_APB, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
				     &data->filter_clk_hz);
#endif

	for (uint8_t i = 0; i < RMT_NUM_CHANNELS; i++) {
		struct rmt_channel *ch = &data->channels[i];

		ch->dev = dev;
		ch->index = i;
		ch->state = RMT_CH_FREE;
		ch->hw_mem = &RMTMEM.channels[i].symbols[0];
		k_sem_init(&ch->done, 0, 1);
	}

	return 0;
}

static DEVICE_API(pulse_io, rmt_pulse_io_api) = {
	.get_capabilities = rmt_get_capabilities,
	.channel_get = rmt_channel_get,
	.channel_release = rmt_channel_release,
	.channel_configure = rmt_channel_configure,
	.transmit_sync = rmt_transmit_sync,
	.receive_sync = rmt_receive_sync,
	.stop = rmt_stop,
};

#if RMT_DMA_SUPPORTED
#define RMT_DMA_DEV(idx)                                                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, dmas), (DEVICE_DT_GET(DT_INST_DMAS_CTLR(idx))),     \
		    (NULL))

#define RMT_DMA_CHANNEL(idx, name)                                                                 \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, name), (DT_INST_DMAS_CELL_BY_NAME(idx, name,        \
										channel)),         \
		    (RMT_DMA_CHANNEL_UNDEFINED))

#define RMT_DMA_INIT(idx)                                                                          \
	.dma_dev = RMT_DMA_DEV(idx), .tx_dma_channel = RMT_DMA_CHANNEL(idx, tx),                   \
	.rx_dma_channel = RMT_DMA_CHANNEL(idx, rx),
#else
#define RMT_DMA_INIT(idx)
#endif

#define RMT_PULSE_IO_INIT(idx)                                                                     \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static const struct rmt_config rmt_config_##idx = {                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),          \
		RMT_DMA_INIT(idx)};                                                                \
	static struct rmt_data rmt_data_##idx = {                                                  \
		.hal =                                                                             \
			{                                                                          \
				.regs = (rmt_soc_handle_t)DT_INST_REG_ADDR(idx),                   \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, rmt_init, NULL, &rmt_data_##idx, &rmt_config_##idx,             \
			      POST_KERNEL, CONFIG_PULSE_IO_INIT_PRIORITY, &rmt_pulse_io_api);

DT_INST_FOREACH_STATUS_OKAY(RMT_PULSE_IO_INIT)
