/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util_macro.h"
#include <zephyr/arch/common/ffs.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/cache.h>
#include <soc.h>

#include <stdint.h>
#include <string.h>

#define DT_DRV_COMPAT st_stm32_dfsdm

#define DFSDM_MAX_FILTERS  4
#define DFSDM_MAX_CHANNELS 8

/* Oversampling max values */
#define DFSDM_MAX_INT_OVERSAMPLING 256
#define DFSDM_MAX_FL_OVERSAMPLING  1024

/* Limit filter output resolution to 31 bits. (i.e. sample range is +/-2^30) */
#define DFSDM_DATA_MAX BIT(30)

/* Prescaler clock divider limits */
#define DFSDM_CLKDIV_MIN 2
#define DFSDM_CLKDIV_MAX 256

/*
 * Data are output as two's complement data in a 24 bit field.
 * Data from filters are in the range +/-2^(n-1)
 * 2^(n-1) maximum positive value cannot be coded in 2's complement n bits
 * An extra bit is required to avoid wrap-around of the binary code for 2^(n-1)
 * So, the resolution of samples from filter is actually limited to 23 bits
 */
#define DFSDM_DATA_RES 24

LOG_MODULE_REGISTER(st_stm32_dfsdm, CONFIG_AUDIO_DMIC_LOG_LEVEL);

struct dmic_stm32_dfsdm_filter_osr {
	uint16_t iosr;    /* Integrator oversampling */
	uint16_t fosr;    /* Filter oversampling */
	uint8_t rshift;   /* Output sample right shift (hardware shift) */
	int8_t pcm_shift; /* PCM scaling shift after the hardware shift */
	uint64_t res;     /* Output sample resolution */
};

struct dmic_stm32_dfsdm_filter_data {
	volatile enum dmic_state state;
	DFSDM_Channel_HandleTypeDef *hchannels; /* Active hardware channels */
	DFSDM_Filter_HandleTypeDef hfilter;
	struct dmic_stm32_dfsdm_filter_osr osr[2]; /* 0: regular, 1: fast conversion mode */
	struct k_mem_slab *mem_slab;
	void *buffer;
	uint16_t buf_pos;
	struct k_msgq *rx_queue;
	uint8_t num_channels; /* Number of active channels */
	uint32_t chan_map;    /* Requested channel map by user */
	uint16_t block_size;
	uint8_t sincorder;
	uint8_t pcm_width;
	int8_t pcm_shift; /* PCM scaling shift for the active filter output */
	int32_t *dma_buffer;
	uint32_t dma_half_block_size_in_bytes;
	struct dma_block_config dma_block;
	int32_t *dma_buffer_ptr[2];
	int32_t *dma_current_buffer;
};

struct dmic_stm32_dfsdm_filter_cfg {
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
	const struct device *parent;
	uint8_t hw_channel;
	const struct device *dma_dev;
	uint32_t dma_channel;
	uint32_t dma_slot;
};

struct dmic_stm32_dfsdm_cfg {
	const struct stm32_pclken pclken; /* Peripheral clock */
	const struct stm32_pclken aclken; /* Audio clock */
	const struct reset_dt_spec reset;
};

/* Each filter has its own ISR */
static void dmic_stm32_dfsdm_isr(const struct device *dev)
{
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;

	HAL_DFSDM_IRQHandler(&data->hfilter);
}

void HAL_DFSDM_FilterErrorCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
	struct dmic_stm32_dfsdm_filter_data *data =
		CONTAINER_OF(hdfsdm_filter, struct dmic_stm32_dfsdm_filter_data, hfilter);

	switch (HAL_DFSDM_FilterGetError(hdfsdm_filter)) {
	case DFSDM_FILTER_ERROR_NONE:
		LOG_WRN("No error");
		break;
	case DFSDM_FILTER_ERROR_REGULAR_OVERRUN:
		LOG_WRN("Regular conversion overrun error");
		break;
	case DFSDM_FILTER_ERROR_INJECTED_OVERRUN:
		LOG_WRN("Injected conversion overrun error");
		break;
	case DFSDM_FILTER_ERROR_DMA:
		LOG_WRN("DMA error");
		break;
	default:
		LOG_ERR("Unknown error");
	}

	data->state = DMIC_STATE_ERROR;
}

static void dmic_stm32_dfsdm_write_sample(uint8_t *dst, uint8_t sample_size, int32_t sample)
{
	switch (sample_size) {
	case 1:
		*dst = (uint8_t)CLAMP(sample, INT8_MIN, INT8_MAX);
		break;
	case 2:
		sys_put_le16((uint16_t)CLAMP(sample, INT16_MIN, INT16_MAX), dst);
		break;
	case 3:
		sys_put_le24((uint32_t)CLAMP(sample, -BIT(23), BIT(23) - 1), dst);
		break;
	default:
		break;
	}
}

void HAL_DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
	struct dmic_stm32_dfsdm_filter_data *data =
		CONTAINER_OF(hdfsdm_filter, struct dmic_stm32_dfsdm_filter_data, hfilter);
	uint32_t chan;
	int32_t sample;
	uint8_t sample_size;
	int ret;

	/* REOCF is only cleared on DFSDM_FLTxRDATAR read.
	 * Read sample before checking error to prevent irq deadlock.
	 */
	sample = HAL_DFSDM_FilterGetRegularValue(hdfsdm_filter, &chan);

	if (data->buffer == NULL || data->state == DMIC_STATE_ERROR) {
		return;
	}

	sample_size = DIV_ROUND_UP(data->pcm_width, BITS_PER_BYTE);

	if (data->pcm_shift > 0) {
		sample >>= data->pcm_shift;
	} else if (data->pcm_shift < 0) {
		sample <<= -data->pcm_shift;
	} else {
		/* Already in the correct format, no need to shift */
	}

	/* Write sample into current buffer */
	if (data->buf_pos + sample_size <= data->block_size) {
		dmic_stm32_dfsdm_write_sample((uint8_t *)data->buffer + data->buf_pos, sample_size,
					      sample);
		data->buf_pos += sample_size;
	}

	/* Push full buffer to the queue and allocate a new one */
	if (data->buf_pos >= data->block_size) {
		void *full_buf = data->buffer;

		ret = k_msgq_put(data->rx_queue, &full_buf, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("RX queue overflow, dropping full buffer: %d", ret);
			k_mem_slab_free(data->mem_slab, full_buf);
		}

		ret = k_mem_slab_alloc(data->mem_slab, &data->buffer, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Could not allocate next RX buffer: %d", ret);
			data->buffer = NULL;
			data->state = DMIC_STATE_ERROR;
			return;
		}
		data->buf_pos = 0;
	}
}

/* DFSDM channels in HAL are defined as follows:
 * - in 16-bit LSB the channel mask is set
 * - in 16-bit MSB the channel number is set
 * e.g. for channel 5 definition:
 *   0x00000020 | 0x00050000 = 0x00050020
 */
static uint32_t hw_chan_from_index(uint8_t idx)
{
	return ((uint32_t)idx << 16) | BIT(idx);
}

/* Determine optimal values for FOSR and IOSR to maximize data output resolution.
 *
 * FOSR - Filter OverSampling Ratio (decimation factor in the Sincx/FastSinc filter)
 * IOSR - Integrator OverSampling Ratio (sum of data for a given number of samples from the filter)
 */
static int dmic_stm32_dfsdm_compute_osrs(struct dmic_stm32_dfsdm_filter_osr *osr, uint32_t ford,
					 uint8_t fast_mode, unsigned int oversamp,
					 uint8_t pcm_width)
{
	uint32_t fosr, iosr;
	uint64_t res;
	uint8_t p = ford;
	uint8_t m = 1;
	int shift = 0;
	int bits = 0;

	if (osr == NULL) {
		return -EINVAL;
	}

	memset(osr, 0, sizeof(*osr));

	if (ford == DFSDM_FILTER_FASTSINC_ORDER) {
		m = 2;
		p = 2;
	}

	for (fosr = 1; fosr <= DFSDM_MAX_FL_OVERSAMPLING; fosr++) {
		for (iosr = 1; iosr <= DFSDM_MAX_INT_OVERSAMPLING; iosr++) {
			uint32_t dec;

			/* Fast conversion mode changes the decimation rate algorithm */
			if (fast_mode != 0) {
				dec = fosr * iosr;
			} else if (ford == DFSDM_FILTER_FASTSINC_ORDER) {
				dec = fosr * (iosr + 3) + 2;
			} else {
				dec = fosr * (iosr - 1 + p) + p;
			}

			if (dec > oversamp) {
				break;
			} else if (dec != oversamp) {
				continue;
			}

			/* Compute resolution */
			res = m;
			for (int i = 0; i < p; i++) {
				res *= fosr;
				if (res > DFSDM_DATA_MAX) {
					break;
				}
			}
			if (res > DFSDM_DATA_MAX) {
				continue;
			}

			res *= iosr;
			if (res > DFSDM_DATA_MAX) {
				continue;
			}

			if (res >= osr->res) {
				osr->res = res;
				osr->fosr = fosr;
				osr->iosr = iosr;

				bits = find_msb_set(osr->res);

				if (osr->res > BIT(bits - 1)) {
					bits++;
				}

				shift = DFSDM_DATA_RES - bits;
				/* Compute right shift */
				if (shift > 0) {
					/* Resolution is lower than 24 bits */
					osr->rshift = 0;
					osr->pcm_shift = (int8_t)bits - (int8_t)pcm_width;
				} else {
					osr->rshift = 1 - shift;
					osr->pcm_shift = DFSDM_DATA_RES - pcm_width - 1;
				}
			}
		}
	}

	LOG_DBG("fast_mode %d, fosr %d, iosr %d, res 0x%llx/%d bits, rshift %d, pcm_shift %d",
		fast_mode, osr->fosr, osr->iosr, osr->res, bits, osr->rshift, osr->pcm_shift);
	if (!osr->res) {
		return -EINVAL;
	}

	return 0;
}

static int dmic_stm32_dfsdm_get_all_osrs(const struct device *dev, unsigned int oversamp,
					 uint8_t pcm_width)
{
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	int ret = 0;

	memset(&data->osr[0], 0, sizeof(data->osr[0]));
	memset(&data->osr[1], 0, sizeof(data->osr[1]));

	/* Compute OSR for regular conversion mode */
	ret = dmic_stm32_dfsdm_compute_osrs(&data->osr[0], data->sincorder, 0, oversamp, pcm_width);
	if (ret < 0) {
		LOG_ERR("Filter parameters not found for regular conversion: %d", ret);
		return -EINVAL;
	}

	/* Compute OSR for fast conversion mode */
	ret = dmic_stm32_dfsdm_compute_osrs(&data->osr[1], data->sincorder, 1, oversamp, pcm_width);
	if (ret < 0) {
		LOG_ERR("Filter parameters not found for fast mode conversion: %d", ret);
		return -EINVAL;
	}

	return 0;
}

/* Reclaim completed buffers still queued for the consumer */
static void dmic_stm32_dfsdm_purge_rx_queue(struct dmic_stm32_dfsdm_filter_data *data)
{
	void *buffer;

	while (k_msgq_get(data->rx_queue, &buffer, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, buffer);
	}
}

static int dmic_stm32_dfsdm_stop(const struct device *dev)
{
	const struct dmic_stm32_dfsdm_filter_cfg *drv_cfg = dev->config;
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	HAL_StatusTypeDef hal_ret;

	/* DMA VERSION */
	if (drv_cfg->dma_dev != NULL) {
		hal_ret = HAL_DFSDM_FilterRegularStop(&data->hfilter);
		dma_stop(drv_cfg->dma_dev, drv_cfg->dma_channel);
	} else {
		hal_ret = HAL_DFSDM_FilterRegularStop_IT(&data->hfilter);
	}

	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to stop DFSDM in regular mode");
		return -EIO;
	}

	hal_ret = HAL_DFSDM_FilterDeInit(&(data->hfilter));
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to stop filter %s", dev->name);
		return -EIO;
	}

	/* Stop channel */
	hal_ret = HAL_DFSDM_ChannelDeInit(data->hchannels);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to stop channel");
		return -EIO;
	}

	/* Conversions and their IRQs are stopped: release the in-flight buffer
	 * the driver still owns so it is not leaked across a stop/start cycle.
	 */
	if (data->buffer != NULL) {
		k_mem_slab_free(data->mem_slab, data->buffer);
		data->buffer = NULL;
	}

	return 0;
}

static void dmic_stm32_dfsdm_dma_callback(const struct device *dma_dev, void *user_data,
					  uint32_t channel, int status)
{
	const struct device *dev = user_data;
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	void *slab_buf;
	int ret;

	if (status != DMA_STATUS_BLOCK && status != DMA_STATUS_COMPLETE) {
		LOG_ERR("DMA Error occurred");
		return;
	}

	/* Protect Cache: Invalidate the specific half-buffer so the CPU sees new RAM data */
	sys_cache_data_invd_range(data->dma_current_buffer, data->dma_half_block_size_in_bytes);

	/* Grab a clean block from the user's mem_slab */
	ret = k_mem_slab_alloc(data->mem_slab, &slab_buf, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Audio slab empty, dropping frame!");
		return;
	}

	/* Shift and pack the data */
	uint32_t num_samples = data->dma_half_block_size_in_bytes / sizeof(int32_t);

	if (data->pcm_width == 16) {
		int16_t *dst = (int16_t *)slab_buf;

		for (uint32_t i = 0; i < num_samples; i++) {
			/* Shift right by 16 to extract the MSB 16-bits of the audio */
			dst[i] = (int16_t)(data->dma_current_buffer[i] >> 16);
		}
	} else {
		int32_t *dst = (int32_t *)slab_buf;

		for (uint32_t i = 0; i < num_samples; i++) {
			/* Shift right by 8 to extract the 24-bit audio and sign-extend */
			dst[i] = (data->dma_current_buffer[i] >> 8);
		}
	}

	if (data->dma_current_buffer == data->dma_buffer_ptr[0]) {
		data->dma_current_buffer = data->dma_buffer_ptr[1];
	} else {
		data->dma_current_buffer = data->dma_buffer_ptr[0];
	}

	/* Push completed block to the app queue */
	ret = k_msgq_put(data->rx_queue, &slab_buf, K_NO_WAIT);
	if (ret < 0) {
		k_mem_slab_free(data->mem_slab, slab_buf);
		LOG_ERR("RX queue full, dropped frame");
	}
}

static int dmic_stm32_dfsdm_start(const struct device *dev)
{
	const struct dmic_stm32_dfsdm_filter_cfg *drv_cfg = dev->config;
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	HAL_StatusTypeDef hal_ret;
	int ret;

	/* Reclaim any buffers left queued by a previous (stopped) capture so a
	 * fresh start always begins with the full mem_slab available.
	 */
	dmic_stm32_dfsdm_purge_rx_queue(data);

	/* DMA VERSION */
	if (drv_cfg->dma_dev != NULL) {
		data->dma_block.block_size = data->dma_half_block_size_in_bytes * 2;
		data->dma_block.source_address = (uint32_t)&data->hfilter.Instance->FLTRDATAR;
		data->dma_block.dest_address = (uint32_t)data->dma_buffer;
		data->dma_block.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->dma_block.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		data->dma_block.source_reload_en = 1;
		data->dma_block.dest_reload_en = 1;
		data->dma_block.next_block = NULL;

		struct dma_config dma_cfg = {
			.channel_direction = PERIPHERAL_TO_MEMORY,
			.source_data_size = 4,
			.dest_data_size = 4,
			.source_burst_length = 4,
			.dest_burst_length = 4,
			.block_count = 1,
			.dma_slot = drv_cfg->dma_slot,
			.head_block = &data->dma_block,
			.dma_callback = dmic_stm32_dfsdm_dma_callback,
			.user_data = (void *)dev,
			.cyclic = 1,
			.half_complete_callback_en = 1,
		};

		ret = dma_config(drv_cfg->dma_dev, drv_cfg->dma_channel, &dma_cfg);
		if (ret < 0) {
			LOG_ERR("Failed to configure DMA: %d", ret);
			return ret;
		}

		ret = dma_start(drv_cfg->dma_dev, drv_cfg->dma_channel);
		if (ret < 0) {
			LOG_ERR("Failed to start DMA: %d", ret);
			return ret;
		}

		/* RDMAEN is already set, just trigger the basic software start */
		hal_ret = HAL_DFSDM_FilterRegularStart(&data->hfilter);
		if (hal_ret != HAL_OK) {
			LOG_ERR("Failed to start DFSDM filter");
			return -EIO;
		}
	} else {
		ret = k_mem_slab_alloc(data->mem_slab, &data->buffer, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Could not allocate RX buffer. Dropping RX data: %d", ret);
			data->state = DMIC_STATE_ERROR;
			return ret;
		}
		data->buf_pos = 0;

		/* Start DFSDM conversions */
		hal_ret = HAL_DFSDM_FilterRegularStart_IT(&data->hfilter);
		if (hal_ret != HAL_OK) {
			LOG_ERR("Failed to start DFSDM filter %s in irq mode", dev->name);
			k_mem_slab_free(data->mem_slab, data->buffer);
			data->buffer = NULL;
			return -EIO;
		}
	}
	data->state = DMIC_STATE_ACTIVE;

	return 0;
}

static int dmic_stm32_dfsdm_filter_deinit(const struct device *dev)
{
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	int ret = 0;

	if (data->state == DMIC_STATE_ACTIVE) {
		ret = dmic_stm32_dfsdm_stop(dev);
	}

	if (data->buffer != NULL) {
		k_mem_slab_free(data->mem_slab, data->buffer);
		data->buffer = NULL;
	}

	/* Drop any completed buffers the consumer never read */
	dmic_stm32_dfsdm_purge_rx_queue(data);

	data->state = DMIC_STATE_UNINIT;

	return ret;
}

static int dmic_stm32_dfsdm_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	int ret;

	switch (cmd) {
	case DMIC_TRIGGER_PAUSE:
	case DMIC_TRIGGER_STOP:
		if (data->state == DMIC_STATE_ACTIVE) {
			ret = dmic_stm32_dfsdm_stop(dev);
			if (ret < 0) {
				return ret;
			}
		}
		data->state = DMIC_STATE_CONFIGURED;
		break;
	case DMIC_TRIGGER_START:
		if (data->state != DMIC_STATE_CONFIGURED) {
			return -EIO;
		}

		ret = dmic_stm32_dfsdm_start(dev);
		if (ret < 0) {
			LOG_ERR("Could not start DMIC");
			return ret;
		}
		break;
	case DMIC_TRIGGER_RELEASE:
	case DMIC_TRIGGER_RESET:
		ret = dmic_stm32_dfsdm_filter_deinit(dev);
		if (ret < 0) {
			LOG_ERR("Could not deinit DMIC");
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int dmic_stm32_dfsdm_read(const struct device *dev, uint8_t stream, void **buffer,
				 size_t *size, int32_t timeout)
{
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	/* Check if we are in a state that can read */
	if ((data->state != DMIC_STATE_CONFIGURED) && (data->state != DMIC_STATE_ACTIVE) &&
	    (data->state != DMIC_STATE_PAUSED)) {
		LOG_ERR("Device state is not valid for read");
		return -EIO;
	}

	ret = k_msgq_get(data->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret < 0) {
		return ret;
	}
	*size = data->block_size;

	return 0;
}

static int dmic_stm32_dfsdm_setup_channel(const struct device *dev, uint32_t div, uint8_t chan,
					  enum pdm_lr lr)
{
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	DFSDM_Channel_HandleTypeDef *hchannel = data->hchannels;
	DFSDM_Filter_HandleTypeDef *hfilter = &data->hfilter;
	uint8_t fast_mode = 0;

	if (hfilter->Init.RegularParam.FastMode == ENABLE) {
		fast_mode = 1;
	}

	data->pcm_shift = data->osr[fast_mode].pcm_shift;
	hchannel->Init.RightBitShift = data->osr[fast_mode].rshift;
	hchannel->Init.OutputClock.Divider = div;
	hchannel->Init.SerialInterface.SpiClock = DFSDM_CHANNEL_SPI_CLOCK_INTERNAL;

	if (lr == PDM_CHAN_RIGHT) {
		hchannel->Init.Input.Pins = DFSDM_CHANNEL_FOLLOWING_CHANNEL_PINS;
		hchannel->Init.SerialInterface.Type = DFSDM_CHANNEL_SPI_FALLING;
	} else {
		hchannel->Init.Input.Pins = DFSDM_CHANNEL_SAME_CHANNEL_PINS;
		hchannel->Init.SerialInterface.Type = DFSDM_CHANNEL_SPI_RISING;
	}

	if (HAL_DFSDM_ChannelInit(hchannel) != HAL_OK) {
		LOG_ERR("Failed to init channel %d", chan);
		return -EIO;
	}

	return 0;
}

static int dmic_stm32_dfsdm_setup_filter(const struct device *dev, uint8_t chan)
{
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	DFSDM_Filter_HandleTypeDef *hfilter = &data->hfilter;
	HAL_StatusTypeDef hal_ret;
	uint8_t fast_mode = 0;

	if (hfilter->Init.RegularParam.FastMode == ENABLE) {
		fast_mode = 1;
	}

	hfilter->Init.FilterParam.IntOversampling = data->osr[fast_mode].iosr;
	hfilter->Init.FilterParam.Oversampling = data->osr[fast_mode].fosr;

	/* SincOrder is used as is in HAL for register value */
	hfilter->Init.FilterParam.SincOrder = data->sincorder << DFSDM_FLTFCR_FORD_Pos;

	hal_ret = HAL_DFSDM_FilterInit(hfilter);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to init filter");
		return -EIO;
	}

	/* Configure regular channel, with continuous conversion for now */
	hal_ret = HAL_DFSDM_FilterConfigRegChannel(hfilter, hw_chan_from_index(chan),
						   DFSDM_CONTINUOUS_CONV_ON);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to configure regular channel");
		return -EIO;
	}

	return 0;
}

static int dmic_stm32_dfsdm_filter_init(const struct device *dev)
{
	const struct dmic_stm32_dfsdm_filter_cfg *cfg = dev->config;
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;
	int ret;

	/* Configure DT provided pins */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure pinctrl: %d", ret);
		return ret;
	}

	/* Connect filter interrupts */
	cfg->irq_config_func(dev);

	data->state = DMIC_STATE_INITIALIZED;

	return 0;
}

static int dmic_stm32_dfsdm_configure(const struct device *dev, struct dmic_cfg *cfg)
{
	const struct dmic_stm32_dfsdm_filter_cfg *drv_cfg = dev->config;
	const struct dmic_stm32_dfsdm_cfg *parent_cfg = drv_cfg->parent->config;
	struct dmic_stm32_dfsdm_filter_data *data = dev->data;

	struct pcm_stream_cfg *stream = &cfg->streams[0];
	struct pdm_chan_cfg *channel = &cfg->channel;
	uint32_t bitclk_rate, oversamp;
	uint32_t max, min = 1;
	uint8_t hw_chan = drv_cfg->hw_channel;
	uint8_t pdm_idx;
	enum pdm_lr lr = 0;
	HAL_StatusTypeDef hal_ret;
	uint32_t requested_samples = stream->block_size / (stream->pcm_width / 8);
	int ret = 0;

	if (data->state == DMIC_STATE_ACTIVE) {
		return -EBUSY;
	}

	/* Only one active stream is supported */
	if (channel->req_num_streams != 1) {
		return -EINVAL;
	}

	/* DMA buffer must be bigger enough to fit user request block size */
	if (drv_cfg->dma_dev != NULL &&
	    requested_samples > CONFIG_DMIC_STM32_DFSDM_MAX_SAMPLES) {
		LOG_ERR("Requested %u samples, max allowed is %d. "
			"Increase CONFIG_DMIC_STM32_DFSDM_MAX_SAMPLES in Kconfig.",
			requested_samples, CONFIG_DMIC_STM32_DFSDM_MAX_SAMPLES);
		return -EINVAL;
	}

	/* DMIC supports up to 8 active channels.
	 * But in regular mode (only supported right now), only 1 channel
	 * per filter can exist.  Verify user is not requesting more.
	 */
	if (channel->req_num_chan > DFSDM_MAX_CHANNELS || channel->req_chan_map_hi != 0) {
		LOG_ERR("DMIC only supports 8 channels or less");
		return -ENOTSUP;
	}

	if (stream->pcm_rate == 0 || stream->pcm_width == 0) {
		if (data->state == DMIC_STATE_CONFIGURED) {
			ret = dmic_stm32_dfsdm_filter_deinit(dev);
		}
		return ret;
	}

	if (stream->pcm_width > DFSDM_DATA_RES) {
		LOG_ERR("DMIC only supports max 24-bit resolution samples");
		return -ENOTSUP;
	}

	/* If DMIC was deinitialized, reinit here */
	if (data->state == DMIC_STATE_UNINIT) {
		ret = dmic_stm32_dfsdm_filter_init(dev);
		if (ret < 0) {
			LOG_ERR("Could not reinit DMIC: %d", ret);
			return ret;
		}
	}

	/* Tear down a previous configuration before reprogramming the channel */
	if (data->state == DMIC_STATE_CONFIGURED) {
		if (HAL_DFSDM_ChannelGetState(data->hchannels) != HAL_DFSDM_CHANNEL_STATE_RESET) {
			hal_ret = HAL_DFSDM_ChannelDeInit(data->hchannels);
			if (hal_ret != HAL_OK) {
				LOG_ERR("Failed to deinit channel before reconfiguring");
				return -EIO;
			}
		}

		if (HAL_DFSDM_FilterGetState(&data->hfilter) != HAL_DFSDM_FILTER_STATE_RESET) {
			hal_ret = HAL_DFSDM_FilterDeInit(&data->hfilter);
			if (hal_ret != HAL_OK) {
				LOG_ERR("Failed to deinit filter before reconfiguring");
				return -EIO;
			}
		}
	}

	/* Get Audio clock */
	ret = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				     (clock_control_subsys_t)&parent_cfg->aclken, &bitclk_rate);
	if (ret < 0) {
		return ret;
	}

	/* Clamp prescaler values to hardware limits */
	min = MAX(DFSDM_CLKDIV_MIN, DIV_ROUND_UP(bitclk_rate, cfg->io.max_pdm_clk_freq));
	max = MIN(DFSDM_CLKDIV_MAX, bitclk_rate / cfg->io.min_pdm_clk_freq);

	/* No valid prescaler exists */
	if (min > max) {
		return -EINVAL;
	}

	/* All values are valid within [min, max] range
	 * Take the min to get the fastest clock possible
	 */
	bitclk_rate /= min;
	LOG_INF("bitclk_rate %d Hz, div %d, output clock %d Hz", bitclk_rate * min, min,
		bitclk_rate);

	oversamp = DIV_ROUND_CLOSEST(bitclk_rate, stream->pcm_rate);
	ret = dmic_stm32_dfsdm_get_all_osrs(dev, oversamp, stream->pcm_width);
	if (ret < 0) {
		return ret;
	}

	/* Validate requested L/R pairs before applying hardware limits */
	for (uint8_t chan = 0; chan < channel->req_num_chan; chan += 2) {
		enum pdm_lr lr_0, lr_1;
		uint8_t hw_chan_0, hw_chan_1;

		dmic_parse_channel_map(channel->req_chan_map_lo, channel->req_chan_map_hi, chan,
				       &hw_chan_0, &lr_0);
		if ((chan + 1) < channel->req_num_chan) {
			dmic_parse_channel_map(channel->req_chan_map_lo, channel->req_chan_map_hi,
					       chan + 1, &hw_chan_1, &lr_1);
			if ((lr_0 == lr_1) || (hw_chan_0 != hw_chan_1)) {
				return -EINVAL;
			}
		}
	}

	dmic_parse_channel_map(channel->req_chan_map_lo, 0, 0, &pdm_idx, &lr);

	/* Regular mode supports one DFSDM channel per filter */
	channel->act_num_chan = 1;
	channel->act_chan_map_lo = dmic_build_channel_map(0, pdm_idx, lr);
	channel->act_chan_map_hi = 0;
	data->chan_map = channel->act_chan_map_lo;

	/* DFSDM channel initialization */
	ret = dmic_stm32_dfsdm_setup_channel(dev, min, hw_chan, lr);
	if (ret < 0) {
		return ret;
	}

	if (drv_cfg->dma_dev != NULL) {
		/* Calculate internal DMA half-transfer size in bytes
		 * (DFSDM DMA always outputs 32-bit words, so sizeof(int32_t))
		 */
		data->dma_half_block_size_in_bytes = requested_samples * sizeof(int32_t);
		data->dma_buffer_ptr[0] = data->dma_buffer;
		data->dma_buffer_ptr[1] = data->dma_buffer + requested_samples;
		data->dma_current_buffer = data->dma_buffer_ptr[0];
		/* Update the HAL init structure BEFORE HAL_DFSDM_FilterInit is called */
		data->hfilter.Init.RegularParam.DmaMode = ENABLE;

		LOG_DBG("DMA Configured: %u samples requested. Internal half-buffer = %u bytes.",
			requested_samples, data->dma_half_block_size_in_bytes);
	} else {
		data->hfilter.Init.RegularParam.DmaMode = DISABLE;
	}

	/* DFSDM filter initialization */
	ret = dmic_stm32_dfsdm_setup_filter(dev, hw_chan);
	if (ret < 0) {
		return ret;
	}

	data->mem_slab = stream->mem_slab;
	data->block_size = stream->block_size;
	data->pcm_width = stream->pcm_width;
	data->num_channels = channel->act_num_chan;
	data->state = DMIC_STATE_CONFIGURED;

	return 0;
}

static DEVICE_API(dmic, dmic_stm32_dfsdm_ops) = {
	.configure = dmic_stm32_dfsdm_configure,
	.trigger = dmic_stm32_dfsdm_trigger,
	.read = dmic_stm32_dfsdm_read,
};

static int dmic_stm32_dfsdm_init(const struct device *dev)
{
	const struct dmic_stm32_dfsdm_cfg *cfg = dev->config;
	int ret;

	LOG_DBG("Initializing %s", dev->name);

	/* Turn on DFSDM peripheral clock */
	ret = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		LOG_ERR("Could not enable peripheral clock: %d", ret);
		return ret;
	}

	ret = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				      (clock_control_subsys_t)&cfg->aclken, NULL);
	if (ret < 0) {
		LOG_ERR("Could not configure aclk: %d", ret);
		return ret;
	}

	/* Reset DFSDM */
	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("Reset controller not ready");
		return -ENODEV;
	}
	reset_line_toggle_dt(&cfg->reset);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int dmic_stm32_dfsdm_deinit(const struct device *dev)
{
	const struct dmic_stm32_dfsdm_cfg *cfg = dev->config;
	int ret = 0;

	reset_line_toggle_dt(&cfg->reset);

	/* Turn off DFSDM peripheral clock */
	ret = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		LOG_ERR("Could not disable peripheral clock: %d", ret);
	}

	return ret;
}

static int dmic_stm32_dfsdm_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return dmic_stm32_dfsdm_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return dmic_stm32_dfsdm_deinit(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define DMIC_DFSDM_CHAN_IDX(chan) DT_PROP(chan, reg)
/* Evaluates to NULL/0 when DMA is NOT present in the devicetree */

#define DFSDM_DMA_DEV(id)                                                      \
	COND_CODE_1(DT_NODE_HAS_PROP(id, dmas),                                \
	(DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(id, rx))), (NULL))

#define DFSDM_DMA_CHAN(id)                                                     \
	DT_DMAS_CELL_BY_NAME_OR(id, rx, channel, 0)

#define DFSDM_DMA_SLOT(id)                                                     \
	DT_DMAS_CELL_BY_NAME_OR(id, rx, slot, 0)

#define DMIC_DFSDM_CHAN_REG_ADDR(chan)                                                             \
	(DFSDM_Channel_TypeDef *)(DT_REG_ADDR(DT_GPARENT(chan)) + DMIC_DFSDM_CHAN_IDX(chan) * 0x20)

#define DMIC_DFSDM_CHAN_DEFINE(chan)                                                               \
	{                                                                                          \
		.Instance = DMIC_DFSDM_CHAN_REG_ADDR(chan),                                        \
		.Init =                                                                            \
			{                                                                          \
				.Awd =                                                             \
					{                                                          \
						.FilterOrder = DFSDM_CHANNEL_FASTSINC_ORDER,       \
						.Oversampling = 1,                                 \
					},                                                         \
				.Input =                                                           \
					{                                                          \
						.DataPacking = DFSDM_CHANNEL_STANDARD_MODE,        \
						.Multiplexer = DFSDM_CHANNEL_EXTERNAL_INPUTS,      \
					},                                                         \
				.Offset = 0,                                                       \
				.OutputClock =                                                     \
					{                                                          \
						.Activation = ENABLE,                              \
						.Selection = DFSDM_CHANNEL_OUTPUT_CLOCK_AUDIO,     \
					},                                                         \
				.SerialInterface =                                                 \
					{                                                          \
						.SpiClock = DFSDM_CHANNEL_SPI_CLOCK_INTERNAL,      \
						.Type = DFSDM_CHANNEL_SPI_RISING,                  \
					},                                                         \
			},                                                                         \
	},

#define DMIC_DFSDM_FLT_REG_ADDR(flt)                                                               \
	(DFSDM_Filter_TypeDef *)(DT_REG_ADDR(DT_PARENT(flt)) + DT_REG_ADDR(flt) * 0x80 + 0x100)

/*
 * Filters can only have one enabled channel child. The build assert in
 * DMIC_DFSDM_FILTERS_DEFINE() keeps this FOREACH expansion to a single value.
 */
#define DMIC_DFSDM_HW_CHANNEL(flt) DT_FOREACH_CHILD_STATUS_OKAY_SEP(flt, DMIC_DFSDM_CHAN_IDX, ())

#define DMIC_DFSDM_FILTER_HFILTER(flt)                                                             \
	{                                                                                          \
		.Instance = DMIC_DFSDM_FLT_REG_ADDR(flt),                                          \
		.Init =                                                                            \
			{                                                                          \
				.InjectedParam =                                                   \
					{                                                          \
						.DmaMode = DISABLE,                                \
						.ExtTrigger = DFSDM_FILTER_EXT_TRIG_TIM8_TRGO,     \
						.ExtTriggerEdge =                                  \
							DFSDM_FILTER_EXT_TRIG_BOTH_EDGES,          \
						.ScanMode = DISABLE,                               \
						.Trigger = DT_PROP(flt, filter0_sync),             \
					},                                                         \
				.RegularParam =                                                    \
					{                                                          \
						.DmaMode = DISABLE,                                \
						.FastMode = ENABLE,                                \
						.Trigger = DFSDM_FILTER_SW_TRIGGER,                \
					},                                                         \
			},                                                                         \
	}

#define DMIC_DFSDM_FILTERS_DEFINE(flt)                                                             \
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(flt) == 1,                                           \
		     "One DFSDM channel child per filter (regular mode)");                         \
                                                                                                   \
	PINCTRL_DT_DEFINE(flt);                                                                    \
                                                                                                   \
	K_MSGQ_DEFINE(dmic_stm32_dfsdm_msgq_##flt, sizeof(void *),                                 \
		      CONFIG_DMIC_STM32_DFSDM_QUEUE_SIZE, 4);                                      \
                                                                                                   \
	IF_ENABLED(DT_NODE_HAS_PROP(flt, dmas),                                                    \
		(static int32_t __aligned(32)                                                      \
		dma_buf_##flt[CONFIG_DMIC_STM32_DFSDM_MAX_SAMPLES * 2];))                          \
		                                                                                   \
	static DFSDM_Channel_HandleTypeDef hchannels_##flt[] = {                                   \
		DT_FOREACH_CHILD(flt, DMIC_DFSDM_CHAN_DEFINE)};                                    \
                                                                                                   \
	static void dmic_stm32_dfsdm_irq_cfg_func_##flt(const struct device *dev)                  \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(flt), DT_IRQ(flt, priority), dmic_stm32_dfsdm_isr,             \
			    DEVICE_DT_GET(flt), 0);                                                \
		irq_enable(DT_IRQN(flt));                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct dmic_stm32_dfsdm_filter_cfg dmic_stm32_dfsdm_filter_##flt = {          \
		.irq_config_func = dmic_stm32_dfsdm_irq_cfg_func_##flt,                            \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(flt),                                            \
		.parent = DEVICE_DT_GET(DT_PARENT(flt)),                                           \
		.hw_channel = DMIC_DFSDM_HW_CHANNEL(flt),                                          \
		/* Conditionally compile the DMA spec based on Devicetree */                       \
		.dma_dev = DFSDM_DMA_DEV(flt),                                                     \
		.dma_channel = DFSDM_DMA_CHAN(flt),                                                \
		.dma_slot = DFSDM_DMA_SLOT(flt),                                                   \
	};                                                                                         \
                                                                                                   \
	static struct dmic_stm32_dfsdm_filter_data dmic_stm32_dfsdm_filter_data_##flt = {          \
		.hchannels = hchannels_##flt,                                                      \
		.hfilter = DMIC_DFSDM_FILTER_HFILTER(flt),                                         \
		.sincorder = DT_PROP(flt, filter_order),                                           \
		.rx_queue = &dmic_stm32_dfsdm_msgq_##flt,                                          \
		.dma_buffer = COND_CODE_1(DT_NODE_HAS_PROP(flt, dmas),                             \
					  (dma_buf_##flt), (NULL)),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(flt, dmic_stm32_dfsdm_filter_init, NULL,                                  \
			 &dmic_stm32_dfsdm_filter_data_##flt, &dmic_stm32_dfsdm_filter_##flt,      \
			 POST_KERNEL, CONFIG_AUDIO_DMIC_INIT_PRIORITY, &dmic_stm32_dfsdm_ops);

#define DFSDM_DEFINE(n)                                                                            \
	PM_DEVICE_DT_INST_DEFINE(n, dmic_stm32_dfsdm_pm_action);                                   \
                                                                                                   \
	static const struct dmic_stm32_dfsdm_cfg dmic_stm32_dfsdm_cfg_##n = {                      \
		.pclken = STM32_DT_INST_CLOCK_INFO_BY_NAME(n, pclk),                               \
		.aclken = STM32_DT_INST_CLOCK_INFO_BY_NAME(n, aclk),                               \
		.reset = RESET_DT_SPEC_INST_GET(n),                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, dmic_stm32_dfsdm_init, NULL, NULL, &dmic_stm32_dfsdm_cfg_##n,     \
			      POST_KERNEL, CONFIG_AUDIO_DMIC_INIT_PRIORITY, NULL);                 \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, DMIC_DFSDM_FILTERS_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(DFSDM_DEFINE)
