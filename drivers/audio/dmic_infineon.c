/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_pdm

#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/dma.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dmic_infineon, CONFIG_AUDIO_DMIC_LOG_LEVEL);

#include <cy_pdm_pcm_v2.h>
#include <cy_dma.h>
#include <cy_trigmux.h>

#define PDM_PCM_ERR_INTERRUPT_MASK (CY_PDM_PCM_INTR_MASK & ~CY_PDM_PCM_INTR_RX_TRIGGER)
/* There are 6 PDM channels on the device */
#define PDM_PCM_MAX_CHANNELS       (6U)
/* The hardware FIFO for each channel has 64 entries. Set the trigger threshold to
 * half the maximum FIFO size in order to reduce the chance of an overflow
 */
#define FIFO_TRIGGER_LEVEL         (32U)

/* Device constant configuration data */
struct ifx_dmic_config {
	PDM_Type *reg_addr;
	const struct pinctrl_dev_config *pcfg;
	uint32_t clk_dst;
	/* irq_nums[] indexed by PDM channel number */
	uint32_t irq_nums[PDM_PCM_MAX_CHANNELS];
	void (*irq_config_func)(const struct device *dev);
};

struct dma_channel {
	const struct device *dev_dma;
	uint32_t channel_num;
	struct dma_config dma_cfg;
	/* blk_cfg[] indexed by logical channel number */
	struct dma_block_config blk_cfg[PDM_PCM_MAX_CHANNELS];
	uint32_t remaining_block_size;
};

/* Device run time data */
struct ifx_dmic_data {
	cy_stc_pdm_pcm_config_v2_t pdm_pcm_cfg;
	/* pdm_pcm_ch_cfg[] indexed by PDM channel number */
	cy_stc_pdm_pcm_channel_config_t pdm_pcm_ch_cfg[PDM_PCM_MAX_CHANNELS];
	struct ifx_cat1_clock clock;
	struct dma_channel dma_rx;
	enum dmic_state state;
	struct k_mem_slab *mem_slab;
	uint32_t block_size;
	struct k_msgq *rx_queue;
	void *active_buf;
	uint8_t num_pdm_channels;
	uint8_t configured_pdm_channels; /* bitmask */
};

static int dmic_stop_capture(const struct device *dev);

static int dmic_start_dma(const struct device *dev)
{
	int ret = 0;
	struct ifx_dmic_data *const data = dev->data;
	struct dma_channel *dma_ch = &data->dma_rx;
	uint32_t transfer_size;

	/* compute the size of the next DMA transfer */
	if (data->dma_rx.remaining_block_size <
	    (FIFO_TRIGGER_LEVEL * dma_ch->dma_cfg.source_data_size)) {
		transfer_size = data->dma_rx.remaining_block_size;
	} else {
		transfer_size = FIFO_TRIGGER_LEVEL * dma_ch->dma_cfg.source_data_size;
	}
	data->dma_rx.remaining_block_size -= transfer_size;

	/* Set the destination addresses in each block configuration */
	for (uint8_t ch = 0; ch < data->num_pdm_channels; ch++) {
		dma_ch->blk_cfg[ch].dest_address =
			(uint32_t)data->active_buf + (ch * dma_ch->dma_cfg.source_data_size);
		dma_ch->blk_cfg[ch].block_size = transfer_size;
	}

	ret = dma_config(dma_ch->dev_dma, dma_ch->channel_num, &dma_ch->dma_cfg);
	if (ret < 0) {
		k_mem_slab_free(data->mem_slab, data->active_buf);
		data->active_buf = NULL;
		return ret;
	}

	ret = dma_start(dma_ch->dev_dma, dma_ch->channel_num);
	if (ret < 0) {
		k_mem_slab_free(data->mem_slab, data->active_buf);
		data->active_buf = NULL;
		return ret;
	}

	return 0;
}

static void dma_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	int ret;
	struct device *dev = (struct device *)arg;
	struct ifx_dmic_data *data = dev->data;

	if (data->state != DMIC_STATE_ACTIVE) {
		/* DMIC is not active, do not process the active buffer
		 * or start the next transfer
		 */
		return;
	}

	if (status != 0) {
		LOG_ERR("DMIC DMA error: %d", status);
		dmic_stop_capture(dev);
		data->state = DMIC_STATE_ERROR;
		return;
	}

	if (data->dma_rx.remaining_block_size == 0) {
		data->dma_rx.remaining_block_size = data->block_size;
		/* All PCM data now transferred from HW FIFO(s) to memory buffer */
		if (0 != k_msgq_put(data->rx_queue, &data->active_buf, K_NO_WAIT)) {
			LOG_ERR("DMIC overflow, no space in queue");
			dmic_stop_capture(dev);
			data->state = DMIC_STATE_ERROR;
			return;
		}

		/* Allocate the next memory buffer */
		if (0 != k_mem_slab_alloc(data->mem_slab, &data->active_buf, K_NO_WAIT)) {
			LOG_ERR("No free memory block available for DMIC reception");
			dmic_stop_capture(dev);
			data->state = DMIC_STATE_ERROR;
			return;
		}
	}

	/* configure the next DMA transfer */
	ret = dmic_start_dma(dev);
	if (ret != 0) {
		LOG_ERR("Failed to restart DMIC DMA transfer: %d", ret);
	}
}

static int dmic_start_capture(const struct device *dev)
{
	int ret;
	const struct ifx_dmic_config *const config = dev->config;
	struct ifx_dmic_data *const data = dev->data;

	data->dma_rx.remaining_block_size = data->block_size;

	ret = k_mem_slab_alloc(data->mem_slab, &data->active_buf, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("No free memory block available for DMIC reception");
		data->state = DMIC_STATE_ERROR;
		return -ENOBUFS;
	}

	ret = dmic_start_dma(dev);
	if (ret != 0) {
		LOG_ERR("Failed to start DMIC DMA transfer: %d", ret);
		return ret;
	}

	/* enable PDM channels and associated interrupts */
	for (uint8_t ch = 0; ch < PDM_PCM_MAX_CHANNELS; ch++) {
		if (data->configured_pdm_channels & (1 << ch)) {
			Cy_PDM_PCM_Channel_ClearInterrupt(config->reg_addr, ch,
							  PDM_PCM_ERR_INTERRUPT_MASK);
			Cy_PDM_PCM_Channel_SetInterruptMask(config->reg_addr, ch,
							    PDM_PCM_ERR_INTERRUPT_MASK);
			irq_enable(config->irq_nums[ch]);
			Cy_PDM_PCM_Channel_Enable(config->reg_addr, ch);
		}
	}

	/* activate all configured PDM channels at the same time */
	for (uint8_t ch = 0; ch < PDM_PCM_MAX_CHANNELS; ch++) {
		if (data->configured_pdm_channels & (1 << ch)) {
			Cy_PDM_PCM_Activate_Channel(config->reg_addr, ch);
		}
	}

	data->state = DMIC_STATE_ACTIVE;
	return 0;
}

static int dmic_stop_capture(const struct device *dev)
{
	const struct ifx_dmic_config *const config = dev->config;
	struct ifx_dmic_data *const data = dev->data;
	struct dma_channel *dma_ch = &data->dma_rx;
	void *buf;

	dma_stop(dma_ch->dev_dma, dma_ch->channel_num);

	for (uint8_t ch = 0; ch < PDM_PCM_MAX_CHANNELS; ch++) {
		if (data->configured_pdm_channels & (1 << ch)) {
			Cy_PDM_PCM_Channel_SetInterruptMask(config->reg_addr, ch, 0);
			Cy_PDM_PCM_DeActivate_Channel(config->reg_addr, ch);
			Cy_PDM_PCM_Channel_Disable(config->reg_addr, ch);
			Cy_PDM_PCM_Channel_ClearInterrupt(config->reg_addr, ch,
							  PDM_PCM_ERR_INTERRUPT_MASK);
			irq_disable(config->irq_nums[ch]);
		}
	}

	if (data->active_buf != NULL) {
		k_mem_slab_free(data->mem_slab, data->active_buf);
		data->active_buf = NULL;
	}

	/* Clear the receive queue and free the entire memory slab */
	while (k_msgq_get(data->rx_queue, &buf, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, buf);
	}

	/* Flush the PDM hardware FIFOs */
	for (uint8_t ch = 0; ch < PDM_PCM_MAX_CHANNELS; ch++) {
		if (data->configured_pdm_channels & (1 << ch)) {
			while (Cy_PDM_PCM_Channel_GetNumInFifo(config->reg_addr, ch) > 0) {
				(void)Cy_PDM_PCM_Channel_ReadFifo(config->reg_addr, ch);
			}
		}
	}

	return 0;
}

static int ifx_dmic_configure(const struct device *dev, struct dmic_cfg *cfg)
{
	struct ifx_dmic_data *const data = dev->data;
	const struct ifx_dmic_config *config = dev->config;
	struct pdm_chan_cfg *channel = &cfg->channel;
	struct pcm_stream_cfg *stream = &cfg->streams[0];
	cy_stc_pdm_pcm_config_v2_t *pdm_cfg = &data->pdm_pcm_cfg;
	cy_en_pdm_pcm_word_size_t pcm_word_size;
	uint32_t dma_data_size_bytes;
	cy_en_pdm_pcm_status_t status;
	uint8_t pdm_ctl_idx;
	enum pdm_lr lr;
	uint8_t pdm_ctl_idx_1;
	enum pdm_lr lr_1;

	if (data->state == DMIC_STATE_ACTIVE) {
		LOG_ERR("Cannot configure DMIC in active state");
		return -EBUSY;
	}

	/* validate channel parameters */
	if (channel->req_num_streams != 1) {
		/* only a single stream is supported */
		return -EINVAL;
	}

	if (channel->req_num_chan > PDM_PCM_MAX_CHANNELS || channel->req_num_chan < 1U) {
		return -EINVAL;
	}

	/* validate stream parameters */
	if (stream->pcm_rate == 0U || stream->mem_slab == NULL || stream->block_size == 0U) {
		return -EINVAL;
	}

	switch (stream->pcm_width) {
	case 8:
		pcm_word_size = CY_PDM_PCM_WSIZE_8_BIT;
		dma_data_size_bytes = 1;
		break;
	case 10:
		pcm_word_size = CY_PDM_PCM_WSIZE_10_BIT;
		dma_data_size_bytes = 2;
		break;
	case 12:
		pcm_word_size = CY_PDM_PCM_WSIZE_12_BIT;
		dma_data_size_bytes = 2;
		break;
	case 14:
		pcm_word_size = CY_PDM_PCM_WSIZE_14_BIT;
		dma_data_size_bytes = 2;
		break;
	case 16:
		pcm_word_size = CY_PDM_PCM_WSIZE_16_BIT;
		dma_data_size_bytes = 2;
		break;
	case 18:
		pcm_word_size = CY_PDM_PCM_WSIZE_18_BIT;
		dma_data_size_bytes = 4;
		break;
	case 20:
		pcm_word_size = CY_PDM_PCM_WSIZE_20_BIT;
		dma_data_size_bytes = 4;
		break;
	case 24:
		pcm_word_size = CY_PDM_PCM_WSIZE_24_BIT;
		dma_data_size_bytes = 4;
		break;
	case 32:
		pcm_word_size = CY_PDM_PCM_WSIZE_32_BIT;
		dma_data_size_bytes = 4;
		break;
	default:
		return -EINVAL;
	}

	/* calculate the clock divider needed to produce the desired sample rate */
	uint32_t peri_freq =
		ifx_cat1_utils_peri_pclk_get_frequency(PCLK_PDM0_CLK_IF_SRSS, &data->clock);

	/* clkDiv is the register field and represents a divider value of clkDiv + 1
	 * The TRM recommends an oversampling rate of 64, meaning that the PDM clock
	 * should be 64 times the PCM sample rate.
	 */
	uint16_t clock_div =
		(uint16_t)(((float)peri_freq / ((float)stream->pcm_rate * 64.0f)) - 0.5f);

	/* validate clkDiv range (min = 3, max = 255) - should be set to an odd value */
	if ((clock_div < 3) || (clock_div > 255)) {
		LOG_DBG("Clock divider out of range");
		return -EINVAL;
	}

	if (clock_div % 2 == 0) {
		LOG_WRN("clkDiv is even. 50 percent duty cycle not guaranteed");
	}

	pdm_cfg->clkDiv = clock_div;
	LOG_DBG("DMIC clock divider set to: %u. Fs = %u", pdm_cfg->clkDiv + 1,
		peri_freq / ((pdm_cfg->clkDiv + 1) * 64U));

	/* In stereo configurations, the left channel will sample at 1/4 the PDM clock period
	 * and right channel at 3/4 the PDM clock period
	 */
	uint8_t sample_delay_l = (pdm_cfg->clkDiv + 1U) / 4U - 1U;
	uint8_t sample_delay_r = (3 * (pdm_cfg->clkDiv + 1U)) / 4U - 1U;

	if ((stream->block_size % (channel->req_num_chan * dma_data_size_bytes)) != 0) {
		LOG_ERR("block_size is not aligned to channel count and data size");
		return -EINVAL;
	}

	/* apply PDM configuration */
	status = Cy_PDM_PCM_Init(config->reg_addr, pdm_cfg);
	if (status != CY_PDM_PCM_SUCCESS) {
		return -EIO;
	}

	/* decode channel map and initialize the requested channels */
	data->configured_pdm_channels = 0;
	channel->act_num_chan = 0;
	for (uint8_t ch = 0; ch < channel->req_num_chan; ch += 2) {
		dmic_parse_channel_map(channel->req_chan_map_lo, channel->req_chan_map_hi, ch,
				       &pdm_ctl_idx, &lr);
		uint8_t pdm_ch_n = (2 * pdm_ctl_idx) + ((lr == PDM_CHAN_LEFT) ? 0U : 1U);
		uint8_t pdm_ch_n_1 = 0;

		if ((ch + 1) < channel->req_num_chan) {
			/* paired channel is enabled */
			dmic_parse_channel_map(channel->req_chan_map_lo, channel->req_chan_map_hi,
					       ch + 1, &pdm_ctl_idx_1, &lr_1);
			/* verify that paired channels use the same hardware index */
			if ((lr == lr_1) || (pdm_ctl_idx != pdm_ctl_idx_1)) {
				return -EINVAL;
			}
			pdm_ch_n_1 = (2 * pdm_ctl_idx_1) + ((lr_1 == PDM_CHAN_LEFT) ? 0U : 1U);
		}

		data->pdm_pcm_ch_cfg[pdm_ch_n].wordSize = pcm_word_size;
		data->pdm_pcm_ch_cfg[pdm_ch_n].sampledelay =
			(pdm_ch_n % 2U == 0U) ? sample_delay_l : sample_delay_r;

		status = Cy_PDM_PCM_Channel_Init(config->reg_addr, &data->pdm_pcm_ch_cfg[pdm_ch_n],
						 pdm_ch_n);
		if (status != CY_PDM_PCM_SUCCESS) {
			return -EIO;
		}
		data->configured_pdm_channels |= (1 << (pdm_ch_n));
		channel->act_num_chan++;

		if ((ch + 1) < channel->req_num_chan) {
			/* paired channel is enabled */
			data->pdm_pcm_ch_cfg[pdm_ch_n_1].wordSize = pcm_word_size;
			data->pdm_pcm_ch_cfg[pdm_ch_n_1].sampledelay =
				(pdm_ch_n_1 % 2U == 0U) ? sample_delay_l : sample_delay_r;
			status = Cy_PDM_PCM_Channel_Init(
				config->reg_addr, &data->pdm_pcm_ch_cfg[pdm_ch_n_1], pdm_ch_n_1);
			if (status != CY_PDM_PCM_SUCCESS) {
				return -EIO;
			}
			data->configured_pdm_channels |= (1 << (pdm_ch_n_1));
			channel->act_num_chan++;
		}
	}
	if (channel->act_num_chan != channel->req_num_chan) {
		LOG_ERR("Unable to configure all requested channels");
		return -EIO;
	}

	/* Set up a DMA block configuration for each actual channel */
	data->dma_rx.dma_cfg.source_data_size = dma_data_size_bytes;
	data->dma_rx.dma_cfg.dest_data_size = dma_data_size_bytes;
	data->dma_rx.dma_cfg.block_count = channel->act_num_chan;
	for (uint8_t ch = 0; ch < channel->act_num_chan; ch++) {
		dmic_parse_channel_map(channel->req_chan_map_lo, channel->req_chan_map_hi, ch,
				       &pdm_ctl_idx, &lr);
		uint8_t pdm_ch_n = (2 * pdm_ctl_idx) + ((lr == PDM_CHAN_LEFT) ? 0U : 1U);

		data->dma_rx.blk_cfg[ch].source_address =
			(uint32_t)&PDM_PCM_RX_FIFO_RD(config->reg_addr, pdm_ch_n);
		data->dma_rx.blk_cfg[ch].source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_rx.blk_cfg[ch].dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_rx.blk_cfg[ch].dest_scatter_interval = channel->act_num_chan;
		data->dma_rx.blk_cfg[ch].block_size = FIFO_TRIGGER_LEVEL * dma_data_size_bytes;
		data->dma_rx.blk_cfg[ch].next_block =
			(ch < (channel->act_num_chan - 1)) ? &data->dma_rx.blk_cfg[ch + 1] : NULL;
	}

	channel->act_chan_map_lo = channel->req_chan_map_lo;
	channel->act_chan_map_hi = channel->req_chan_map_hi;

	data->mem_slab = stream->mem_slab;
	data->block_size = stream->block_size;
	data->active_buf = NULL;
	data->state = DMIC_STATE_CONFIGURED;
	data->num_pdm_channels = channel->act_num_chan;

	return 0;
}

static int ifx_dmic_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	int ret = 0;
	const struct ifx_dmic_config *config = dev->config;
	struct ifx_dmic_data *data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_START:
		if (data->state != DMIC_STATE_CONFIGURED) {
			return -EIO;
		}

		ret = dmic_start_capture(dev);
		if (ret != 0) {
			LOG_ERR("Failed to start capture: %d", ret);
			return ret;
		}

		break;

	case DMIC_TRIGGER_RELEASE:
		if (data->state != DMIC_STATE_PAUSED) {
			return -EIO;
		}

		ret = dmic_start_capture(dev);
		if (ret != 0) {
			LOG_ERR("Failed to start capture: %d", ret);
			return ret;
		}

		break;

	case DMIC_TRIGGER_PAUSE:
	case DMIC_TRIGGER_STOP:
	case DMIC_TRIGGER_RESET:
		for (uint8_t ch = 0; ch < PDM_PCM_MAX_CHANNELS; ch++) {
			/* prevent interrupts from occurring while stopping the stream */
			Cy_PDM_PCM_Channel_SetInterruptMask(config->reg_addr, ch, 0);
		}
		data->state = (cmd == DMIC_TRIGGER_RESET)   ? DMIC_STATE_UNINIT
			      : (cmd == DMIC_TRIGGER_PAUSE) ? DMIC_STATE_PAUSED
							    : DMIC_STATE_CONFIGURED;
		dmic_stop_capture(dev);

		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static int ifx_dmic_read(const struct device *dev, uint8_t stream, void **buffer, size_t *size,
			 int32_t timeout)
{
	struct ifx_dmic_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	if ((data->state != DMIC_STATE_ACTIVE) && (data->state != DMIC_STATE_PAUSED)) {
		return -EIO;
	}

	ret = k_msgq_get(data->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret < 0) {
		return ret;
	}

	*size = data->block_size;

	return 0;
}

static int dmic_init(const struct device *dev)
{
	struct ifx_dmic_data *const data = dev->data;
	const struct ifx_dmic_config *const config = dev->config;
	struct dma_channel *dma_ch = &data->dma_rx;
	int ret = 0;
	cy_rslt_t status;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Connect PDM to the configured peripheral clock */
	status = ifx_cat1_utils_peri_pclk_assign_divider(config->clk_dst, &data->clock);
	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Apply initial DMA controller configuration */
	if (!device_is_ready(dma_ch->dev_dma)) {
		return -ENODEV;
	}
	dma_ch->dma_cfg.head_block = &dma_ch->blk_cfg[0];
	dma_ch->dma_cfg.user_data = (void *)dev;
	dma_ch->dma_cfg.dma_callback = dma_callback;

	/* deinitialize PDM/PCM and clear the hardware FIFOs */
	for (uint8_t ch = 0; ch < PDM_PCM_MAX_CHANNELS; ch++) {
		Cy_PDM_PCM_Channel_DeInit(config->reg_addr, ch);
		while (Cy_PDM_PCM_Channel_GetNumInFifo(config->reg_addr, ch) > 0) {
			(void)Cy_PDM_PCM_Channel_ReadFifo(config->reg_addr, ch);
		}
	}
	Cy_PDM_PCM_DeInit(config->reg_addr);

	/* connect unified PDM trigger to the DMA channel. This trigger will activate
	 * when all configured PDM channels have met their FIFO trigger threshold
	 */
	Cy_TrigMux_Connect(PERI_0_TRIG_IN_MUX_0_PDM_RX_REQ_ALL,
			   PERI_0_TRIG_OUT_MUX_0_PDMA0_TR_IN0 + data->dma_rx.channel_num, false,
			   TRIGGER_TYPE_EDGE);

	/* connect channel interrupts */
	config->irq_config_func(dev);

	data->state = DMIC_STATE_INITIALIZED;

	LOG_DBG("Device %s inited", dev->name);

	return 0;
}

static void dmic_isr(const struct device *dev, uint8_t channel)
{
	struct ifx_dmic_data *const data = dev->data;
	const struct ifx_dmic_config *const config = dev->config;

	uint32_t int_status =
		Cy_PDM_PCM_Channel_GetInterruptStatusMasked(config->reg_addr, channel);

	if (((int_status & CY_PDM_PCM_INTR_RX_FIR_OVERFLOW)) ||
	    ((int_status & CY_PDM_PCM_INTR_RX_IF_OVERFLOW))) {
		LOG_ERR("CH:%d Interface overflow - indicates clocking issues", channel);
		data->state = DMIC_STATE_ERROR;
	}

	if (int_status & CY_PDM_PCM_INTR_RX_OVERFLOW) {
		LOG_ERR("CH:%d FIFO overflow", channel);
		data->state = DMIC_STATE_ERROR;
	}

	if (int_status & CY_PDM_PCM_INTR_RX_UNDERFLOW) {
		LOG_ERR("CH:%d FIFO underflow", channel);
		data->state = DMIC_STATE_ERROR;
	}

	if (data->state == DMIC_STATE_ERROR) {
		dmic_stop_capture(dev);
	}

	Cy_PDM_PCM_Channel_ClearInterrupt(config->reg_addr, channel, PDM_PCM_ERR_INTERRUPT_MASK);
}

static void dmic_ch0_isr(const struct device *dev)
{
	dmic_isr(dev, 0);
}

static void dmic_ch1_isr(const struct device *dev)
{
	dmic_isr(dev, 1);
}

static void dmic_ch2_isr(const struct device *dev)
{
	dmic_isr(dev, 2);
}

static void dmic_ch3_isr(const struct device *dev)
{
	dmic_isr(dev, 3);
}

static void dmic_ch4_isr(const struct device *dev)
{
	dmic_isr(dev, 4);
}

static void dmic_ch5_isr(const struct device *dev)
{
	dmic_isr(dev, 5);
}

static const struct _dmic_ops dmic_ops = {
	.configure = ifx_dmic_configure,
	.trigger = ifx_dmic_trigger,
	.read = ifx_dmic_read,
};

#define DMIC_PERI_CLOCK_INFO(n)                                                                    \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),                 \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	},

#define DMIC_DMA_CHANNEL_INIT(index)                                                               \
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, rx)),                            \
	.channel_num = DT_INST_DMAS_CELL_BY_NAME(index, rx, channel),                              \
	.dma_cfg = {.channel_direction = PERIPHERAL_TO_MEMORY,                                     \
		    .source_burst_length = 0,                                                      \
		    .dest_burst_length = 0,                                                        \
		    .block_count = 1,                                                              \
		    .complete_callback_en = 0,                                                     \
		    .source_handshake = 0}

#define ROUTE_INIT(child)                                                                          \
	COND_CODE_1(DT_PROP(child, use_alt_io), (|(1 << DT_PROP(child, channel))), ())

#define PDM_PCM_ROUTE_INIT(index) DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(index), ROUTE_INIT)

#define PDM_PCM_CONFIG(index)                                                                      \
	.pdm_pcm_cfg = {.clksel = CY_PDM_PCM_SEL_SRSS_CLOCK,                                       \
			.halverate = CY_PDM_PCM_RATE_FULL,                                         \
			.route = 0 PDM_PCM_ROUTE_INIT(index),                                      \
			.fir0_coeff_user_value = 0,                                                \
			.fir1_coeff_user_value = 0},

#define PDM_PCM_CH_INIT(child)                                                                     \
	[DT_PROP(child, channel)] = {                                                              \
		.sampledelay = 3,                                                                  \
		.signExtension = false,                                                            \
		.rxFifoTriggerLevel = FIFO_TRIGGER_LEVEL,                                          \
		.fir0_enable = DT_PROP_OR(child, fir0_enable, false),                              \
		.cic_decim_code =                                                                  \
			DT_PROP_OR(child, cic_decimation_code, CY_PDM_PCM_CHAN_CIC_DECIM_32),      \
		.fir0_decim_code =                                                                 \
			DT_PROP_OR(child, fir0_decimation, CY_PDM_PCM_CHAN_FIR0_DECIM_1),          \
		.fir0_scale = DT_PROP_OR(child, fir0_scale, 0),                                    \
		.fir1_decim_code =                                                                 \
			DT_PROP_OR(child, fir1_decimation, CY_PDM_PCM_CHAN_FIR1_DECIM_2),          \
		.fir1_scale = DT_PROP_OR(child, fir1_scale, 15),                                   \
		.dc_block_disable = DT_PROP_OR(child, dc_block_disable, false),                    \
		.dc_block_code =                                                                   \
			DT_PROP_OR(child, dc_block_code, CY_PDM_PCM_CHAN_DCBLOCK_CODE_2)},

#define PDM_PCM_CHANNEL_CONFIG(index)                                                              \
	.pdm_pcm_ch_cfg = {DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(index), PDM_PCM_CH_INIT)},

#define IFX_DMIC_INIT(index)                                                                       \
                                                                                                   \
	static void dmic_irq_config_func_##index(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 0, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 0, priority), dmic_ch0_isr,                  \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 1, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 1, priority), dmic_ch1_isr,                  \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 2, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 2, priority), dmic_ch2_isr,                  \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 3, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 3, priority), dmic_ch3_isr,                  \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 4, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 4, priority), dmic_ch4_isr,                  \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 5, irq),                                     \
			    DT_INST_IRQ_BY_IDX(index, 5, priority), dmic_ch5_isr,                  \
			    DEVICE_DT_INST_GET(index), 0);                                         \
	}                                                                                          \
                                                                                                   \
	K_MSGQ_DEFINE(dmic_msgq##index, sizeof(void *), CONFIG_DMIC_INFINEON_QUEUE_SIZE, 1);       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static struct ifx_dmic_config dmic_config_##index = {                                      \
		.reg_addr = (PDM_Type *)DT_INST_REG_ADDR(index),                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clk_dst = DT_INST_PROP(index, clk_dst),                                           \
		.irq_nums = {DT_INST_IRQ_BY_IDX(index, 0, irq), DT_INST_IRQ_BY_IDX(index, 1, irq), \
			     DT_INST_IRQ_BY_IDX(index, 2, irq), DT_INST_IRQ_BY_IDX(index, 3, irq), \
			     DT_INST_IRQ_BY_IDX(index, 4, irq),                                    \
			     DT_INST_IRQ_BY_IDX(index, 5, irq)},                                   \
		.irq_config_func = dmic_irq_config_func_##index,                                   \
	};                                                                                         \
                                                                                                   \
	static struct ifx_dmic_data dmic_data_##index = {                                          \
		PDM_PCM_CONFIG(index) PDM_PCM_CHANNEL_CONFIG(index) DMIC_PERI_CLOCK_INFO(index)    \
			.dma_rx = {DMIC_DMA_CHANNEL_INIT(index)},                                  \
		.rx_queue = &dmic_msgq##index,                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &dmic_init, NULL, &dmic_data_##index, &dmic_config_##index,   \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dmic_ops);

DT_INST_FOREACH_STATUS_OKAY(IFX_DMIC_INIT)
