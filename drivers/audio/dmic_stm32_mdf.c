/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_mdf

#include <zephyr/audio/dmic.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include <stdint.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(st_stm32_mdf, CONFIG_AUDIO_DMIC_LOG_LEVEL);

#define MDF_MAX_FILTERS 6
#define MDF_MAX_SITFS   6

#define MDF_DATA_RES 24
#define MDF_MCICD_MIN 2

struct dmic_stm32_mdf_log10 {
	unsigned int raw; /* Original number */
	unsigned int log; /* 1000 x log10(raw), rounded */
};

struct dmic_stm32_mdf_scales {
	unsigned int scale;
	int gain_db;
};

struct dmic_stm32_mdf_data {
	MDF_HandleTypeDef hmdf;
	MDF_FilterConfigTypeDef filter_config;
	volatile enum dmic_state state;
	struct k_mem_slab *mem_slab;
	struct k_msgq *rx_queue;
	uint16_t block_size;
	uint32_t chan_map;
	uint8_t pcm_width;
	uint16_t buf_pos;
	void *buffer;
};

struct dmic_stm32_mdf_flt_cfg {
	const struct pinctrl_dev_config *pcfg;
	void (*irq_cfg_func)(const struct device *dev);
	const struct device *parent;
};

struct dmic_stm32_mdf_cfg {
	const struct stm32_pclken pclken; /* Peripheral clock */
	const struct stm32_pclken aclken; /* Audio clock */
	const struct reset_dt_spec reset;
};

/* Each filter has its own ISR */
static void dmic_stm32_mdf_isr(const struct device *dev)
{
	struct dmic_stm32_mdf_data *data = dev->data;

	HAL_MDF_IRQHandler(&data->hmdf);
}

void HAL_MDF_ErrorCallback(MDF_HandleTypeDef *hmdf)
{
	struct dmic_stm32_mdf_data *data = CONTAINER_OF(hmdf, struct dmic_stm32_mdf_data, hmdf);

	switch (HAL_MDF_GetError(hmdf)) {
	case MDF_ERROR_NONE:
		LOG_WRN("No error");
		break;
	case MDF_ERROR_ACQUISITION_OVERFLOW:
		LOG_WRN("Acquisition overflow error");
		break;
	case MDF_ERROR_RSF_OVERRUN:
		LOG_WRN("Reshape filter overrun error");
		break;
	case MDF_ERROR_CLOCK_ABSENCE:
		LOG_WRN("Clock absence detection error");
		break;
	case MDF_ERROR_SHORT_CIRCUIT:
		LOG_WRN("Short circuit detection error");
		break;
	case MDF_ERROR_SATURATION:
		LOG_WRN("Saturation detection error");
		break;
	case MDF_ERROR_OUT_OFF_LIMIT:
		LOG_WRN("Out-off limit detection error");
		break;
	case MDF_ERROR_DMA:
		LOG_WRN("DMA error");
		break;
	default:
		LOG_ERR("Unknown error: %d", HAL_MDF_GetError(hmdf));
	}

	data->state = DMIC_STATE_ERROR;
}

static void dmic_stm32_mdf_write_sample(uint8_t *dst, uint8_t size, int32_t sample)
{
	switch (size) {
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
		CODE_UNREACHABLE;
	}
}

void HAL_MDF_AcqCpltCallback(MDF_HandleTypeDef *hmdf)
{
	struct dmic_stm32_mdf_data *data = CONTAINER_OF(hmdf, struct dmic_stm32_mdf_data, hmdf);
	HAL_StatusTypeDef hal_ret;
	uint8_t sample_size;
	int32_t sample;
	int ret;

	/* FTHF is cleared by reading MDF_DFLTDR until RXFIFO level
	 * is lower than the threshold.
	 * Checking for error after capture value prevents irq deadlock.
	 */
	hal_ret = HAL_MDF_GetAcqValue(hmdf, &sample);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to capture value");
		return;
	}

	if (data->buffer == NULL || data->state == DMIC_STATE_ERROR) {
		return;
	}

	sample_size = DIV_ROUND_UP(data->pcm_width, 8);
	sample >>= MDF_DFLTDR_DR_Pos;

	/* Write sample into current buffer */
	if (data->buf_pos + sample_size <= data->block_size) {
		dmic_stm32_mdf_write_sample((uint8_t *)data->buffer + data->buf_pos, sample_size,
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

/*
 * The CIC output data resolution cannot exceed 26 bits.
 * Output data resolution: DS = N * ln(D) / ln(2) + 1 (for serial interface data),
 * where N is filter order and D the CIC decimation factor.
 * Following table gives the maximum decimation ratio for filter order [0..5].
 */
static const unsigned int cic_max_decim_sitf[] = {512, 512, 512, 322, 76, 32};

/*
 * Prime numbers and their 1000 x log10 table.
 * This prevents from computing each prime numbers.
 */
static const struct dmic_stm32_mdf_log10 log_table[] = {
	{2, 301},    {3, 477},    {5, 699},   {7, 845},   {11, 1041},  {13, 1114},  {17, 1230},
	{19, 1279},  {23, 1362},  {29, 1462}, {31, 1491}, {37, 1568},  {41, 1613},  {43, 1633},
	{47, 1672},  {53, 1724},  {59, 1771}, {61, 1785}, {67, 1826},  {71, 1851},  {73, 1863},
	{79, 1898},  {83, 1919},  {89, 1949}, {97, 1987}, {101, 2004}, {103, 2013}, {107, 2029},
	{109, 2037}, {113, 2053}, {127, 2104},
};

/* Gain (dB) x 10 according to scale value in hex */
static const struct dmic_stm32_mdf_scales scale_table[] = {
	{0x20, -482}, {0x21, -446}, {0x22, -421}, {0x23, -386}, {0x24, -361}, {0x25, -326},
	{0x26, -301}, {0x27, -266}, {0x28, -241}, {0x29, -206}, {0x2A, -181}, {0x2B, -145},
	{0x2C, -120}, {0x2D, -85},  {0x2E, -60},  {0x2F, -25},  {0x00, 0},    {0x01, 35},
	{0x02, 60},   {0x03, 95},   {0x04, 120},  {0x05, 156},  {0x06, 181},  {0x07, 216},
	{0x08, 241},  {0x09, 276},  {0x0A, 301},  {0x0B, 336},  {0x0C, 361},  {0x0D, 396},
	{0x0E, 421},  {0x0F, 457},  {0x10, 482},  {0x11, 517},  {0x12, 542},  {0x13, 577},
	{0x14, 602},  {0x15, 637},  {0x16, 662},  {0x17, 697},  {0x18, 722},
};

static int dmic_stm32_mdf_compute_scale(unsigned int decim, unsigned int order,
					unsigned int data_size)
{
	unsigned long max = ARRAY_SIZE(log_table);
	unsigned int num, d, logd = 0;
	int i, scale;

	/* Decompose decimation ratio D, as prime number factors, to compute log10(D) */
	num = decim;
	while (num > 1) {
		for (i = 0; i < max; i++) {
			d = log_table[i].raw;
			if (!(num % d)) {
				logd += log_table[i].log;
				num = num / d;
				break;
			}
		}
		if (i == max) {
			LOG_WRN("Failed to set scale. Output signal may saturate.");
			return 0;
		}
	}

	/* scale = 20 * ((DS - 1) * log10(2) - NF * log10(D)) */
	scale = 20 * ((data_size - 1) * log_table[0].log - order * logd);

	return scale;
}

static int dmic_stm32_mdf_compute_params(const struct device *dev, unsigned int decim)
{
	struct dmic_stm32_mdf_data *data = dev->data;
	MDF_FilterConfigTypeDef *flt_cfg = &data->filter_config;
	uint8_t cic_order = flt_cfg->CicMode >> MDF_DFLTCICR_CICMOD_Pos;
	unsigned int decim_cic, decim_rsflt = 1;
	unsigned int data_size = MDF_DATA_RES;
	int i, l, max_scale, scale;

	if (flt_cfg->ReshapeFilter.Activation == ENABLE) {
		decim_rsflt = flt_cfg->ReshapeFilter.DecimationRatio;
		data_size -= 2;
	}

	decim_cic = DIV_ROUND_CLOSEST(decim, decim_rsflt);
	if (!IN_RANGE(decim_cic, MDF_MCICD_MIN, cic_max_decim_sitf[cic_order])) {
		LOG_ERR("Decimation factor [%d] out of range for CIC filter order", decim_cic);
		return -EINVAL;
	}

	/*
	 * Compute scaling:
	 * max scale = 20 * log10( 2 exp DS / D exp NF )
	 * - DS = max data size at scale output (RSFLT on: DS = 22 / RSFLT off: DS = 24)
	 * - NF = Main CIC filter order
	 */
	if (is_power_of_two(decim_cic)) {
		/*
		 * Decimation ratio is a power of 2: D = 2 exp n
		 * max scale = 20 * (DS - n * NF) * log10(2)
		 */
		l = log_table[0].log;

		/* Compute max scale (dB) * 1000 */
		max_scale = 20 * (data_size - 1 - (cic_order * (find_msb_set(decim_cic) - 1))) * l;
	} else {
		/*
		 * Decimation ratio is not a power of 2
		 * max scale = 20 * ((DS - 1) * log10(2) - NF * log10(D))
		 */
		max_scale = dmic_stm32_mdf_compute_scale(decim_cic, cic_order, data_size);
	}

	LOG_DBG("Filter order [%d], decimation [%d], data size [%d], max scale [%d]", cic_order,
		decim_cic, data_size, max_scale / 1000);

	/*
	 * Find scale register setting.
	 * Limit max_scale accuracy to first decimal for comparison with scale table values.
	 */
	max_scale = DIV_ROUND_CLOSEST(max_scale, 100);
	for (i = ARRAY_SIZE(scale_table) - 1; i > 0; i--) {
		if (scale_table[i].gain_db < max_scale) {
			break;
		}
	};
	scale = scale_table[i].scale;

	LOG_DBG("Set scale to [%d]dB: [0x%x]", scale_table[i].gain_db / 10, scale);

	flt_cfg->Gain = scale;
	flt_cfg->DecimationRatio = decim_cic;

	LOG_DBG("Scale [%d], Decimation CIC [%d], Decimation RSFLT [%d]",
		scale, decim_cic, decim_rsflt);

	return 0;
}

/* Reclaim completed buffers still queued for the consumer */
static void dmic_stm32_mdf_purge_rx_queue(struct dmic_stm32_mdf_data *data)
{
	void *buffer;

	while (k_msgq_get(data->rx_queue, &buffer, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, buffer);
	}
}

static int dmic_stm32_mdf_start(const struct device *dev)
{
	struct dmic_stm32_mdf_data *data = dev->data;
	HAL_StatusTypeDef hal_ret;
	int ret;

	/* Reclaim any buffers left queued by a previous (stopped) capture so a
	 * fresh start always begins with the full mem_slab available.
	 */
	dmic_stm32_mdf_purge_rx_queue(data);

	/* Allocate memory slab */
	ret = k_mem_slab_alloc(data->mem_slab, &data->buffer, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Could not allocate RX buffer. Dropping RX data: %d", ret);
		data->state = DMIC_STATE_ERROR;
		return ret;
	}
	data->buf_pos = 0;

	/* Start MDF acquisition */
	hal_ret = HAL_MDF_AcqStart_IT(&data->hmdf, &data->filter_config);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to start MDF %s in IRQ mode", dev->name);
		k_mem_slab_free(data->mem_slab, data->buffer);
		data->buffer = NULL;
		return -EIO;
	}

	data->state = DMIC_STATE_ACTIVE;

	return 0;
}

static int dmic_stm32_mdf_stop(const struct device *dev)
{
	struct dmic_stm32_mdf_data *data = dev->data;
	HAL_StatusTypeDef hal_ret;

	/* Stop MDF acquisition */
	hal_ret = HAL_MDF_AcqStop_IT(&data->hmdf);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to stop MDF in IRQ mode");
		return -EIO;
	}

	/* Conversions and their IRQs are stopped: release the in-flight buffer
	 * the driver still owns so it is not leaked across a stop/start cycle.
	 */
	if (data->buffer != NULL) {
		k_mem_slab_free(data->mem_slab, data->buffer);
		data->buffer = NULL;
	}

	data->state = DMIC_STATE_CONFIGURED;

	return 0;
}

static int dmic_stm32_mdf_deinit(const struct device *dev)
{
	struct dmic_stm32_mdf_data *data = dev->data;
	HAL_StatusTypeDef hal_ret;
	int ret = 0;

	if (data->state == DMIC_STATE_ACTIVE) {
		ret = dmic_stm32_mdf_stop(dev);
	}

	hal_ret = HAL_MDF_DeInit(&data->hmdf);
	if (hal_ret != HAL_OK) {
		return -EIO;
	}

	if (data->buffer != NULL) {
		k_mem_slab_free(data->mem_slab, data->buffer);
		data->buffer = NULL;
	}

	/* Drop any completed buffers the consumer never read */
	dmic_stm32_mdf_purge_rx_queue(data);

	data->state = DMIC_STATE_UNINIT;

	return ret;
}

static int dmic_stm32_mdf_init(const struct device *dev)
{
	const struct dmic_stm32_mdf_flt_cfg *cfg = dev->config;
	struct dmic_stm32_mdf_data *data = dev->data;
	int ret;

	/* Configure DT provided pins */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure pinctrl: %d", ret);
		return ret;
	}

	/* Run IRQ init */
	cfg->irq_cfg_func(dev);

	data->state = DMIC_STATE_INITIALIZED;

	return 0;
}

static int dmic_stm32_mdf_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct dmic_stm32_mdf_data *data = dev->data;
	int ret;

	switch (cmd) {
	case DMIC_TRIGGER_PAUSE:
	case DMIC_TRIGGER_STOP:
		if (data->state == DMIC_STATE_ACTIVE) {
			ret = dmic_stm32_mdf_stop(dev);
			if (ret < 0) {
				return ret;
			}
		}
		data->state = DMIC_STATE_CONFIGURED;
		break;
	case DMIC_TRIGGER_RELEASE:
	case DMIC_TRIGGER_START:
		if (data->state != DMIC_STATE_CONFIGURED) {
			LOG_ERR("Device is not configured");
			return -EIO;
		}

		ret = dmic_stm32_mdf_start(dev);
		if (ret < 0) {
			LOG_ERR("Could not start DMIC");
			return -EIO;
		}
		break;
	case DMIC_TRIGGER_RESET:
		ret = dmic_stm32_mdf_deinit(dev);
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

static int dmic_stm32_mdf_read(const struct device *dev, uint8_t stream, void **buffer,
			       size_t *size, int32_t timeout)
{
	struct dmic_stm32_mdf_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);

	/* Check if we are in a state that can read */
	if ((data->state != DMIC_STATE_CONFIGURED) && (data->state != DMIC_STATE_ACTIVE) &&
	    (data->state != DMIC_STATE_PAUSED)) {
		LOG_ERR("Device state is not valid for read: %d", data->state);
		return -EIO;
	}

	ret = k_msgq_get(data->rx_queue, buffer, SYS_TIMEOUT_MS(timeout));
	if (ret < 0) {
		return ret;
	}
	*size = data->block_size;

	return 0;
}

static int dmic_stm32_mdf_configure(const struct device *dev, struct dmic_cfg *cfg)
{
	const struct dmic_stm32_mdf_flt_cfg *drv_cfg = dev->config;
	const struct dmic_stm32_mdf_cfg *parent_cfg = drv_cfg->parent->config;
	struct dmic_stm32_mdf_data *data = dev->data;

	uint32_t best_div = UINT32_MAX, best_proc = 0, best_cck = 0;
	struct pcm_stream_cfg *stream = &cfg->streams[0];
	struct pdm_chan_cfg *channel = &cfg->channel;
	uint32_t d = UINT32_MAX, proc, cck = 1;
	uint32_t bitclk_rate, oversamp;
	uint8_t decim_rsflt, pdm_idx;
	HAL_StatusTypeDef hal_ret;
	uint32_t max, min = 1;
	enum pdm_lr lr = 0;
	int ret = 0;

	if (data->state == DMIC_STATE_ACTIVE) {
		return -EBUSY;
	}

	/* Only one active stream is supported */
	if (channel->req_num_streams != 1) {
		return -EINVAL;
	}

	/* DMIC supports up to 6 active serial interfaces.
	 * Each sitf is mapped to a dedicated filter.
	 */
	if (channel->req_num_chan > MDF_MAX_SITFS || channel->req_chan_map_hi != 0) {
		LOG_ERR("DMIC only supports 6 channels or less");
		return -ENOTSUP;
	}

	if (stream->pcm_rate == 0 || stream->pcm_width == 0) {
		if (data->state == DMIC_STATE_CONFIGURED) {
			ret = dmic_stm32_mdf_deinit(dev);
		}
		return ret;
	}

	if (stream->pcm_width > MDF_DATA_RES) {
		LOG_ERR("DMIC only supports max 24-bit resolution samples");
		return -ENOTSUP;
	}

	/* If DMIC was deinitialized, reinit here */
	if (data->state == DMIC_STATE_UNINIT) {
		ret = dmic_stm32_mdf_init(dev);
		if (ret < 0) {
			LOG_ERR("Could not reinit DMIC: %d", ret);
			return ret;
		}
	}

	/* Tear down a previous configuration before reprogramming the channel */
	if (data->state == DMIC_STATE_CONFIGURED) {
		if (HAL_MDF_GetState(&data->hmdf) != HAL_MDF_STATE_RESET) {
			hal_ret = HAL_MDF_DeInit(&data->hmdf);
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

	/* Clamp prescaler values to hardware limits
	 * Here min and max represent the product of both prescalers
	 * so PROCDIV * CCKDIV.  Since PROCDIV in [1, 128] and CCKDIV in [1, 16]
	 * then the max values of the product is [1, 2048].
	 */
	min = MAX(1, DIV_ROUND_UP(bitclk_rate, cfg->io.max_pdm_clk_freq));
	max = MIN(2048, bitclk_rate / cfg->io.min_pdm_clk_freq);

	/* No valid prescaler exists */
	if (min > max) {
		return -EINVAL;
	}

	LOG_DBG("min [%d], max [%d]", min, max);

	decim_rsflt = (data->filter_config.ReshapeFilter.Activation == ENABLE)
			      ? data->filter_config.ReshapeFilter.DecimationRatio
			      : 1U;

	for (proc = 1; proc <= 128; proc++) {
		uint32_t cckmin = MAX(1, DIV_ROUND_UP(min, proc));
		uint32_t cckmax = MIN(16, max / proc);

		for (cck = cckmin; cck <= cckmax; cck++) {
			d = proc * cck;

			if (((d % decim_rsflt) == 0U) && (d < best_div)) {
				best_div = d;
				best_proc = proc;
				best_cck = cck;
			}
		}
	}

	if (best_div == UINT32_MAX) {
		LOG_ERR("No valid clock divider found");
		return -EINVAL;
	}

	LOG_DBG("input clock [%d] Hz, procdiv [%d], cckdiv [%d], output clock [%d] Hz",
		bitclk_rate, best_proc, best_cck, bitclk_rate / best_div);

	data->hmdf.Init.CommonParam.ProcClockDivider = best_proc;
	data->hmdf.Init.CommonParam.OutputClock.Divider = best_cck;
	bitclk_rate /= best_div;
	oversamp = DIV_ROUND_CLOSEST(bitclk_rate, stream->pcm_rate);

	ret = dmic_stm32_mdf_compute_params(dev, oversamp);
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

	/* One SIFT per filter */
	channel->act_num_chan = 1;
	channel->act_chan_map_lo = dmic_build_channel_map(0, pdm_idx, lr);
	channel->act_chan_map_hi = 0;
	data->chan_map = channel->act_chan_map_lo;

	/* MDF filter initialization */
	hal_ret = HAL_MDF_Init(&data->hmdf);
	if (hal_ret != HAL_OK) {
		LOG_ERR("MDF initialization failed.");
		return -EIO;
	}

	data->mem_slab = stream->mem_slab;
	data->block_size = stream->block_size;
	data->pcm_width = stream->pcm_width;
	data->state = DMIC_STATE_CONFIGURED;

	return 0;
}

static DEVICE_API(dmic, dmic_stm32_mdf_ops) = {
	.configure = dmic_stm32_mdf_configure,
	.trigger = dmic_stm32_mdf_trigger,
	.read = dmic_stm32_mdf_read,
};

static int dmic_stm32_mdf_clock_init(const struct device *dev)
{
	const struct dmic_stm32_mdf_cfg *cfg = dev->config;
	const struct device *cc_node = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret;

	/* Turn on MDF peripheral clock */
	ret = clock_control_on(cc_node, (clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		LOG_ERR("Failed to enable MDF peripheral clock: %d", ret);
		return ret;
	}

	/* Configure MDF kernel clock */
	ret = clock_control_configure(cc_node, (clock_control_subsys_t)&cfg->aclken, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to configure MDF clock: %d", ret);
		return ret;
	}

	return 0;
}

static int dmic_stm32_mdf_pm_init(const struct device *dev)
{
	const struct dmic_stm32_mdf_cfg *cfg = dev->config;
	int ret = 0;

	LOG_DBG("Initializing %s", dev->name);

	/* Turn on MDF peripheral clock */
	ret = dmic_stm32_mdf_clock_init(dev);
	if (ret < 0) {
		LOG_ERR("Could not enable clocks: %d", ret);
		return ret;
	}

	ret = reset_line_toggle_dt(&cfg->reset);
	if (ret < 0) {
		LOG_ERR("Could not toggle reset: %d", ret);
	}

	return ret;
}

static int dmic_stm32_mdf_pm_deinit(const struct device *dev)
{
	const struct dmic_stm32_mdf_cfg *cfg = dev->config;
	int ret = 0;

	LOG_DBG("Deinitializing %s", dev->name);

	reset_line_toggle_dt(&cfg->reset);

	/* Turn off MDF peripheral clock */
	ret = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		LOG_ERR("Could not disable peripheral clock: %d", ret);
	}

	return ret;
}

static int dmic_stm32_mdf_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return dmic_stm32_mdf_pm_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return dmic_stm32_mdf_pm_deinit(dev);
	default:
		return -ENOTSUP;
	}
}

static int dmic_stm32_mdf_device_init(const struct device *dev)
{
	const struct dmic_stm32_mdf_cfg *cfg = dev->config;

	/* Reset MDF */
	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("Reset controller not ready");
		return -ENODEV;
	}

	return pm_device_driver_init(dev, dmic_stm32_mdf_pm_action);
}

#define RESHAPE_FILTER(flt)                                                                        \
	.ReshapeFilter = {                                                                         \
		.Activation = DT_NODE_HAS_PROP(flt, st_reshape_filter_decimation),                 \
		.DecimationRatio = DT_PROP(flt, st_reshape_filter_decimation),                     \
	},

#define HIGHPASS_FILTER(flt)                                                                       \
	.HighPassFilter = {                                                                        \
		.Activation = DT_NODE_HAS_PROP(flt, st_highpass_filter_cutoff),                    \
		.CutOffFrequency = DT_PROP(flt, st_highpass_filter_cutoff) << MDF_DFLTRSFR_HPFC_Pos,  \
	},

#define MDF_FILTERS_DEFINE(flt)                                                                    \
	PINCTRL_DT_DEFINE(flt);                                                                    \
                                                                                                   \
	K_MSGQ_DEFINE(dmic_stm32_mdf_msgq_##flt, sizeof(void *),                                   \
		      CONFIG_AUDIO_DMIC_STM32_QUEUE_SIZE, 4);                                      \
                                                                                                   \
	static void dmic_stm32_mdf_irq_cfg_func_##flt(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(flt), DT_IRQ(flt, priority), dmic_stm32_mdf_isr,               \
			    DEVICE_DT_GET(flt), 0);                                                \
		irq_enable(DT_IRQN(flt));                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct dmic_stm32_mdf_flt_cfg dmic_stm32_mdf_cfg_##flt = {                    \
		.irq_cfg_func = dmic_stm32_mdf_irq_cfg_func_##flt,                                 \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(flt),                                            \
		.parent = DEVICE_DT_GET(DT_PARENT(flt)),                                           \
	};                                                                                         \
                                                                                                   \
	BUILD_ASSERT(DT_PROP(flt, gain) % 3 == 0);                                                 \
                                                                                                   \
	static const MDF_FilterConfigTypeDef config_##flt = {                                      \
		.DataSource = MDF_DATA_SOURCE_BSMX,                                                \
		.Delay = DT_PROP(flt, delay),                                                      \
		.CicMode = (DT_PROP(flt, filter_order) << MDF_DFLTCICR_CICMOD_Pos),                \
		.Offset = DT_PROP(flt, offset),                                                    \
		.Gain = DT_PROP(flt, gain) / 3,                                                    \
		RESHAPE_FILTER(flt)                                                                \
		HIGHPASS_FILTER(flt)                                                               \
		.Integrator.Activation = DT_PROP(flt, st_integrator_on),                           \
		.SoundActivity.Activation = DT_PROP(flt, st_sound_activity_on),                    \
		.AcquisitionMode = MDF_MODE_ASYNC_CONT,                                            \
		.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY,                                     \
		.DiscardSamples = DT_PROP(flt, discard_samples),                                   \
	};                                                                                         \
                                                                                                   \
	static struct dmic_stm32_mdf_data dmic_stm32_mdf_data_##flt = {                            \
		.hmdf = {                                                                          \
			.Instance = (MDF_Filter_TypeDef *)DT_REG_ADDR(flt),                        \
			.Init = {                                                                  \
				.CommonParam = {                                                   \
					.InterleavedFilters = 0,                                   \
					.OutputClock.Activation = ENABLE,                          \
					.OutputClock.Pins = MDF_OUTPUT_CLOCK_0,                    \
					.OutputClock.Trigger.Activation = DISABLE,                 \
				},                                                                 \
				.SerialInterface = {                                               \
					.Activation = ENABLE,                                      \
					.Mode = MDF_SITF_NORMAL_SPI_MODE,                          \
					.ClockSource = MDF_SITF_CCK0_SOURCE,                       \
					.Threshold = 31,                                           \
				},                                                                 \
				.FilterBistream = MDF_BITSTREAM0_FALLING,                          \
			},                                                                         \
		},                                                                                 \
		.filter_config = config_##flt,                                                     \
		.rx_queue = &dmic_stm32_mdf_msgq_##flt,                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(flt, dmic_stm32_mdf_init, NULL, &dmic_stm32_mdf_data_##flt,               \
			 &dmic_stm32_mdf_cfg_##flt, POST_KERNEL, CONFIG_AUDIO_DMIC_INIT_PRIORITY,  \
			 &dmic_stm32_mdf_ops);

#define MDF_DEFINE(n)                                                                              \
	PM_DEVICE_DT_INST_DEFINE(n, dmic_stm32_mdf_pm_action);                                     \
                                                                                                   \
	static const struct dmic_stm32_mdf_cfg dmic_stm32_mdf_cfg_##n = {                          \
		.pclken = STM32_DT_INST_CLOCK_INFO_BY_NAME(n, pclk),                               \
		.aclken = STM32_DT_INST_CLOCK_INFO_BY_NAME(n, aclk),                               \
		.reset = RESET_DT_SPEC_INST_GET(n),                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, dmic_stm32_mdf_device_init, NULL, NULL, &dmic_stm32_mdf_cfg_##n,  \
			      POST_KERNEL, CONFIG_AUDIO_DMIC_INIT_PRIORITY, NULL);                 \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, MDF_FILTERS_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(MDF_DEFINE)
