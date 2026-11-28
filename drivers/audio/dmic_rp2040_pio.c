/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RP2040 PIO-based DMIC driver.
 *
 *   - A PIO state machine (allocated from the parent PIO device) generates
 *     the PDM clock and samples DIN into its RX FIFO
 *   - A DMA channel (Zephyr DMA API) moves raw PDM bits into a ping-pong
 *     buffer; the completion callback re-arms it on the other buffer
 *   - The callback runs the PDM decimation filter and posts completed PCM
 *     blocks to a k_msgq for dmic_read() callers
 */

#define DT_DRV_COMPAT raspberrypi_rp2040_pdm

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/dt-bindings/dma/rpi-pico-dma-common.h>
#include <zephyr/logging/log.h>

#include <hardware/pio.h>

LOG_MODULE_REGISTER(dmic_rp2040_pio, CONFIG_AUDIO_DMIC_LOG_LEVEL);

/* RAW_BUFFER_SIZE must be a multiple of (decimation / 8).
 * For decimation=128: multiple of 16; for decimation=64: multiple of 8.
 * 512 satisfies both.
 */
#define RAW_BUFFER_SIZE  512
#define QUEUE_DEPTH      4

/* Mic startup noise: PCM muted for this long after trigger START */
#define STARTUP_MUTE_MS  20

/* Filter output scale: with volume == MaxVolume the gain is
 * filterGain * 32768 / sub_const for every decimation. A MaxVolume of 1
 * truncates pdm_filter_div_const to 0 (clamped to 1) at decimation 64,
 * making the gain property ineffective.
 */
#define PDM_FILTER_MAX_VOLUME 64

/* The filter tables below are shared, file-scope state: a single instance only. */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "only one raspberrypi,rp2040-pdm instance is supported");

/* --- PIO program -------------------------------------------------------------
 *
 * Ported from ArduinoCore-mbed pdm.pio. Two instructions per PDM clock period:
 *
 * .program pdm_pio
 * .side_set 1
 * .wrap_target
 *     push iffull noblock  side 1   ; clk high, push 8-bit word when full
 *     in   pins, 1         side 0   ; clk low,  sample one bit from DIN
 * .wrap
 */
RPI_PICO_PIO_DEFINE_PROGRAM(pdm_pio, 0, 1,
	0x9040, /* 0: push iffull noblock  side 1 */
	0x4001  /* 1: in   pins, 1         side 0 */
);

/* --- PDM to PCM decimation filter ---------------------------------------
 *
 * Adapted from STMicroelectronics' OpenPDMFilter library
 * (Copyright (c) 2018 STMicroelectronics; Apache-2.0 licensed, same as this
 * file). Trimmed to the mono, look-up-table-based path this driver actually
 * exercises: the upstream library's stereo and non-LUT code paths are
 * unreachable here and have been removed.
 */

#define PDM_FILTER_SINCN          3
#define PDM_FILTER_DECIMATION_MAX 128

struct pdm_filter_state {
	/* Public: set by the caller before pdm_filter_init() */
	float LP_HZ;
	float HP_HZ;
	uint16_t Fs;
	unsigned int nSamples;
	uint8_t Decimation;
	uint8_t MaxVolume;
	uint16_t filterGain;
	/* Private: filter state carried between decimate() calls */
	uint32_t Coef[PDM_FILTER_SINCN];
	int64_t OldOut, OldIn, OldZ;
	uint16_t LP_ALFA;
	uint16_t HP_ALFA;
};

static uint32_t pdm_filter_div_const;
static int64_t pdm_filter_sub_const;
static uint32_t pdm_filter_sinc[PDM_FILTER_DECIMATION_MAX * PDM_FILTER_SINCN];
static uint32_t pdm_filter_sinc1[PDM_FILTER_DECIMATION_MAX];
static uint32_t pdm_filter_sinc2[PDM_FILTER_DECIMATION_MAX * 2];
static uint32_t pdm_filter_coef[PDM_FILTER_SINCN][PDM_FILTER_DECIMATION_MAX];
static int32_t pdm_filter_lut[256][PDM_FILTER_DECIMATION_MAX / 8][PDM_FILTER_SINCN];

static inline int64_t pdm_round_div(int64_t a, int64_t b)
{
	return (a > 0) ? ((a + b / 2) / b) : ((a - b / 2) / b);
}

static inline int16_t pdm_saturate(int64_t n, int64_t lo, int64_t hi)
{
	if (n < lo) {
		return (int16_t)lo;
	}
	if (n > hi) {
		return (int16_t)hi;
	}
	return (int16_t)n;
}

static int32_t pdm_filter_table_mono_64(const uint8_t *data, uint8_t sincn)
{
	return (int32_t)
		pdm_filter_lut[data[0]][0][sincn] +
		pdm_filter_lut[data[1]][1][sincn] +
		pdm_filter_lut[data[2]][2][sincn] +
		pdm_filter_lut[data[3]][3][sincn] +
		pdm_filter_lut[data[4]][4][sincn] +
		pdm_filter_lut[data[5]][5][sincn] +
		pdm_filter_lut[data[6]][6][sincn] +
		pdm_filter_lut[data[7]][7][sincn];
}

static int32_t pdm_filter_table_mono_128(const uint8_t *data, uint8_t sincn)
{
	return (int32_t)
		pdm_filter_lut[data[0]][0][sincn] +
		pdm_filter_lut[data[1]][1][sincn] +
		pdm_filter_lut[data[2]][2][sincn] +
		pdm_filter_lut[data[3]][3][sincn] +
		pdm_filter_lut[data[4]][4][sincn] +
		pdm_filter_lut[data[5]][5][sincn] +
		pdm_filter_lut[data[6]][6][sincn] +
		pdm_filter_lut[data[7]][7][sincn] +
		pdm_filter_lut[data[8]][8][sincn] +
		pdm_filter_lut[data[9]][9][sincn] +
		pdm_filter_lut[data[10]][10][sincn] +
		pdm_filter_lut[data[11]][11][sincn] +
		pdm_filter_lut[data[12]][12][sincn] +
		pdm_filter_lut[data[13]][13][sincn] +
		pdm_filter_lut[data[14]][14][sincn] +
		pdm_filter_lut[data[15]][15][sincn];
}

static void pdm_filter_convolve(const uint32_t *sig, uint16_t sig_len,
				 const uint32_t *kernel, uint16_t kernel_len,
				 uint32_t *result)
{
	for (uint16_t n = 0; n < sig_len + kernel_len - 1; n++) {
		uint16_t kmin = (n >= kernel_len - 1) ? n - (kernel_len - 1) : 0;
		uint16_t kmax = (n < sig_len - 1) ? n : sig_len - 1;

		result[n] = 0;
		for (uint16_t k = kmin; k <= kmax; k++) {
			result[n] += sig[k] * kernel[n - k];
		}
	}
}

static void pdm_filter_init(struct pdm_filter_state *param)
{
	uint8_t decimation = param->Decimation;
	int64_t sum = 0;

	if (decimation == 0) {
		/* Guard against a mis-configured caller: decimation is only
		 * ever 64 or 128 in this driver, but 0 would underflow the
		 * sinc[] index below.
		 */
		return;
	}

	for (int i = 0; i < PDM_FILTER_SINCN; i++) {
		param->Coef[i] = 0;
	}
	for (int i = 0; i < decimation; i++) {
		pdm_filter_sinc1[i] = 1;
	}

	param->OldOut = 0;
	param->OldIn = 0;
	param->OldZ = 0;
	param->LP_ALFA = (param->LP_HZ != 0) ?
		(uint16_t)(param->LP_HZ * 256 / (param->LP_HZ + param->Fs / (2 * 3.14159f))) : 0;
	param->HP_ALFA = (param->HP_HZ != 0) ?
		(uint16_t)(param->Fs * 256 / (2 * 3.14159f * param->HP_HZ + param->Fs)) : 0;

	pdm_filter_sinc[0] = 0;
	pdm_filter_sinc[decimation * PDM_FILTER_SINCN - 1] = 0;
	pdm_filter_convolve(pdm_filter_sinc1, decimation, pdm_filter_sinc1, decimation,
			     pdm_filter_sinc2);
	pdm_filter_convolve(pdm_filter_sinc2, decimation * 2 - 1, pdm_filter_sinc1, decimation,
			     &pdm_filter_sinc[1]);

	for (int j = 0; j < PDM_FILTER_SINCN; j++) {
		for (int i = 0; i < decimation; i++) {
			pdm_filter_coef[j][i] = pdm_filter_sinc[j * decimation + i];
			sum += pdm_filter_sinc[j * decimation + i];
		}
	}

	pdm_filter_sub_const = sum >> 1;
	pdm_filter_div_const = pdm_filter_sub_const * param->MaxVolume / 32768 / param->filterGain;
	pdm_filter_div_const = (pdm_filter_div_const == 0) ? 1 : pdm_filter_div_const;

	/* Build the LUT: for each possible input byte, precompute its
	 * contribution to each of the SINCN convolution stages.
	 */
	for (int s = 0; s < PDM_FILTER_SINCN; s++) {
		uint32_t *coef_p = &pdm_filter_coef[s][0];

		for (int c = 0; c < 256; c++) {
			for (int d = 0; d < decimation / 8; d++) {
				pdm_filter_lut[c][d][s] =
					((c >> 7) & 0x01) * coef_p[d * 8 + 0] +
					((c >> 6) & 0x01) * coef_p[d * 8 + 1] +
					((c >> 5) & 0x01) * coef_p[d * 8 + 2] +
					((c >> 4) & 0x01) * coef_p[d * 8 + 3] +
					((c >> 3) & 0x01) * coef_p[d * 8 + 4] +
					((c >> 2) & 0x01) * coef_p[d * 8 + 5] +
					((c >> 1) & 0x01) * coef_p[d * 8 + 6] +
					((c >> 0) & 0x01) * coef_p[d * 8 + 7];
			}
		}
	}
}

static void pdm_filter_decimate_64(const uint8_t *data, int16_t *data_out, uint16_t volume,
				    struct pdm_filter_state *param)
{
	int64_t old_out = param->OldOut;
	int64_t old_in = param->OldIn;
	int64_t old_z = param->OldZ;

	for (unsigned int i = 0; i < param->nSamples; i++) {
		int64_t z0 = pdm_filter_table_mono_64(data, 0);
		int64_t z1 = pdm_filter_table_mono_64(data, 1);
		int64_t z2 = pdm_filter_table_mono_64(data, 2);
		int64_t z;

		z = param->Coef[1] + z2 - pdm_filter_sub_const;
		param->Coef[1] = param->Coef[0] + z1;
		param->Coef[0] = z0;

		old_out = (param->HP_ALFA * (old_out + z - old_in)) >> 8;
		old_in = z;
		old_z = ((256 - param->LP_ALFA) * old_z + param->LP_ALFA * old_out) >> 8;

		z = old_z * volume;
		z = pdm_round_div(z, pdm_filter_div_const);
		data_out[i] = pdm_saturate(z, -32700, 32700);

		data += PDM_FILTER_DECIMATION_MAX >> 4;
	}

	param->OldOut = old_out;
	param->OldIn = old_in;
	param->OldZ = old_z;
}

static void pdm_filter_decimate_128(const uint8_t *data, int16_t *data_out, uint16_t volume,
				     struct pdm_filter_state *param)
{
	int64_t old_out = param->OldOut;
	int64_t old_in = param->OldIn;
	int64_t old_z = param->OldZ;

	for (unsigned int i = 0; i < param->nSamples; i++) {
		int64_t z0 = pdm_filter_table_mono_128(data, 0);
		int64_t z1 = pdm_filter_table_mono_128(data, 1);
		int64_t z2 = pdm_filter_table_mono_128(data, 2);
		int64_t z;

		z = param->Coef[1] + z2 - pdm_filter_sub_const;
		param->Coef[1] = param->Coef[0] + z1;
		param->Coef[0] = z0;

		old_out = (param->HP_ALFA * (old_out + z - old_in)) >> 8;
		old_in = z;
		old_z = ((256 - param->LP_ALFA) * old_z + param->LP_ALFA * old_out) >> 8;

		z = old_z * volume;
		z = pdm_round_div(z, pdm_filter_div_const);
		data_out[i] = pdm_saturate(z, -32700, 32700);

		data += PDM_FILTER_DECIMATION_MAX >> 3;
	}

	param->OldOut = old_out;
	param->OldIn = old_in;
	param->OldZ = old_z;
}

struct rp2040_pdm_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct device *dma_dev;
	uint32_t dma_channel;
	uint8_t clk_pin;
	uint8_t din_pin;
	uint16_t gain;
};

struct rp2040_pdm_data {
	const struct device *dev;

	/* PIO */
	PIO pio;
	size_t sm;
	uint pio_offset;

	/* DMA */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk;

	/* Raw PDM ping-pong buffers */
	uint8_t raw_buf[2][RAW_BUFFER_SIZE];
	int raw_buf_idx; /* buffer the DMA is currently filling */

	/* PDM decimation filter state */
	struct pdm_filter_state filter;
	int cut_frames;  /* PCM chunks still to mute */

	/* DMIC API contract: caller provides these in configure() */
	struct k_mem_slab *mem_slab;
	uint32_t block_size;

	/* PCM block being filled, one chunk per raw buffer */
	void *cur_block;
	uint32_t cur_offset;
	uint32_t chunk_size;
	int mute_chunks; /* chunks muted after START (STARTUP_MUTE_MS) */

	/* Queue of completed PCM blocks returned by dmic_read() */
	struct k_msgq rx_queue;
	void *rx_queue_buf[QUEUE_DEPTH];

	bool configured;
	volatile bool running;
};

/* --- Capture path ------------------------------------------------------------ */

/* Decimate one raw buffer into the current PCM block; post the block when full */
static void rp2040_pdm_process(struct rp2040_pdm_data *data, const uint8_t *raw)
{
	if (data->cur_block == NULL) {
		if (k_mem_slab_alloc(data->mem_slab, &data->cur_block, K_NO_WAIT) != 0) {
			/* Drop this chunk if no buffer available */
			data->cur_block = NULL;
			return;
		}
		data->cur_offset = 0;
	}

	int16_t *dst = (int16_t *)((uint8_t *)data->cur_block + data->cur_offset);
	uint16_t volume = data->filter.MaxVolume;

	if (data->filter.Decimation == 128) {
		pdm_filter_decimate_128(raw, dst, volume, &data->filter);
	} else {
		pdm_filter_decimate_64(raw, dst, volume, &data->filter);
	}

	/* Mute the first chunks (mic startup noise) */
	if (data->cut_frames > 0) {
		memset(dst, 0, data->chunk_size);
		data->cut_frames--;
	}

	data->cur_offset += data->chunk_size;
	if (data->cur_offset < data->block_size) {
		return;
	}

	/* Block complete: post to queue, drop if full */
	if (k_msgq_put(&data->rx_queue, &data->cur_block, K_NO_WAIT) != 0) {
		k_mem_slab_free(data->mem_slab, data->cur_block);
	}
	data->cur_block = NULL;
}

static void rp2040_pdm_dma_cb(const struct device *dma_dev, void *arg, uint32_t channel,
			      int status)
{
	struct rp2040_pdm_data *data = arg;

	if (!data->running) {
		return;
	}

	if (status < 0) {
		LOG_ERR("DMA error %d, capture stopped", status);
		data->running = false;
		return;
	}

	/* Re-arm on the other buffer before processing the one just filled */
	int done = data->raw_buf_idx;
	int next = done ^ 1;
	int ret = dma_reload(dma_dev, channel, (uint32_t)&data->pio->rxf[data->sm],
			     (uint32_t)data->raw_buf[next], RAW_BUFFER_SIZE);

	if (ret < 0) {
		LOG_ERR("DMA reload failed %d, capture stopped", ret);
		data->running = false;
		return;
	}
	data->raw_buf_idx = next;

	rp2040_pdm_process(data, data->raw_buf[done]);
}

/* Free the partial and all queued PCM blocks. Capture must be stopped. */
static void rp2040_pdm_flush(struct rp2040_pdm_data *data)
{
	void *block;

	if (data->cur_block != NULL) {
		k_mem_slab_free(data->mem_slab, data->cur_block);
		data->cur_block = NULL;
	}
	while (k_msgq_get(&data->rx_queue, &block, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->mem_slab, block);
	}
}

/* Program the (disabled) state machine for the given clock divider */
static void rp2040_pdm_sm_init(const struct rp2040_pdm_config *cfg,
			       struct rp2040_pdm_data *data, float clk_div)
{
	pio_sm_config c = pio_get_default_sm_config();

	sm_config_set_wrap(&c, data->pio_offset + RPI_PICO_PIO_GET_WRAP_TARGET(pdm_pio),
			   data->pio_offset + RPI_PICO_PIO_GET_WRAP(pdm_pio));
	/* Side-set drives the clock pin, IN reads from the DIN pin */
	sm_config_set_sideset(&c, 1, false, false);
	sm_config_set_sideset_pins(&c, cfg->clk_pin);
	sm_config_set_in_pins(&c, cfg->din_pin);
	/* Shift left, no autopush, push threshold = 8 bits */
	sm_config_set_in_shift(&c, false, false, 8);
	/* RX only: 8-deep FIFO gives the DMA re-arm more slack */
	sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
	sm_config_set_clkdiv(&c, clk_div);

	pio_sm_set_consecutive_pindirs(data->pio, data->sm, cfg->din_pin, 1, false);
	pio_sm_set_consecutive_pindirs(data->pio, data->sm, cfg->clk_pin, 1, true);
	pio_sm_set_pins_with_mask(data->pio, data->sm, 0, BIT(cfg->clk_pin));

	/* Leaves the state machine disabled; trigger START enables it */
	pio_sm_init(data->pio, data->sm, data->pio_offset, &c);
}

/* --- Driver ops ------------------------------------------------------------- */

static int rp2040_pdm_configure(const struct device *dev, struct dmic_cfg *cfg)
{
	const struct rp2040_pdm_config *hw = dev->config;
	struct rp2040_pdm_data *data = dev->data;
	uint32_t sys_clk_hz;
	int ret;

	if (data->running) {
		return -EBUSY;
	}

	if (cfg->channel.req_num_streams < 1 ||
	    cfg->streams[0].pcm_width != 16) {
		return -EINVAL;
	}

	/* A single PIO data line carries one microphone */
	if (cfg->channel.req_num_chan != 1) {
		return -EINVAL;
	}

	uint32_t sample_rate = cfg->streams[0].pcm_rate;
	uint32_t min_clk = cfg->io.min_pdm_clk_freq ? cfg->io.min_pdm_clk_freq : 1200000U;
	uint32_t max_clk = cfg->io.max_pdm_clk_freq ? cfg->io.max_pdm_clk_freq : 3250000U;

	/* PDM clock = sample_rate x decimation: prefer 128, fall back to 64 */
	int decimation = 0;

	for (int d = 128; d >= 64; d /= 2) {
		uint64_t clk = (uint64_t)sample_rate * (uint32_t)d;

		if (clk >= min_clk && clk <= max_clk) {
			decimation = d;
			break;
		}
	}
	if (decimation == 0) {
		return -EINVAL;
	}

	/* One raw buffer decimates to a fixed number of PCM samples (a chunk);
	 * a PCM block is filled from one or more consecutive chunks.
	 */
	int pcm_samples = RAW_BUFFER_SIZE / (decimation / 8);
	uint32_t chunk_size = (uint32_t)pcm_samples * sizeof(int16_t);
	uint32_t block_size = cfg->streams[0].block_size;

	if (block_size == 0U || (block_size % chunk_size) != 0U) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(hw->clk_dev, hw->clk_id, &sys_clk_hz);
	if (ret < 0) {
		return ret;
	}

	/* Return blocks still queued to the slab they came from */
	rp2040_pdm_flush(data);

	/* Store parameters for trigger/read */
	data->mem_slab   = cfg->streams[0].mem_slab;
	data->block_size = block_size;
	data->chunk_size = chunk_size;
	data->mute_chunks = (int)((sample_rate * STARTUP_MUTE_MS / 1000U) / (uint32_t)pcm_samples);
	cfg->channel.act_num_chan = 1;
	cfg->channel.act_num_streams = 1;

	/* Initialise the decimation filter (mbed-compatible defaults). */
	data->filter.Fs         = (uint16_t)sample_rate;
	data->filter.MaxVolume  = PDM_FILTER_MAX_VOLUME;
	data->filter.nSamples   = (unsigned int)pcm_samples;
	data->filter.LP_HZ      = (float)sample_rate / 2.0f;
	data->filter.HP_HZ      = 10.0f;
	data->filter.Decimation = (uint8_t)decimation;
	data->filter.filterGain = hw->gain;
	pdm_filter_init(&data->filter);

	/* The PIO program takes two instructions per PDM clock period, so the
	 * state machine runs at sample_rate x decimation x 2.
	 */
	float clk_div = (float)sys_clk_hz /
			(float)(sample_rate * (uint32_t)decimation * 2U);

	pio_sm_set_enabled(data->pio, data->sm, false);
	rp2040_pdm_sm_init(hw, data, clk_div);

	data->configured = true;
	return 0;
}

static int rp2040_pdm_start(const struct device *dev)
{
	const struct rp2040_pdm_config *hw = dev->config;
	struct rp2040_pdm_data *data = dev->data;
	struct dma_config *dcfg = &data->dma_cfg;
	struct dma_block_config *blk = &data->dma_blk;
	int ret;

	/* Drop blocks left over from a previous capture */
	rp2040_pdm_flush(data);

	data->raw_buf_idx = 0;
	data->cut_frames = data->mute_chunks;

	/* PIO RX FIFO -> raw_buf[0], 8-bit transfers paced by the SM's RX DREQ */
	memset(dcfg, 0, sizeof(*dcfg));
	memset(blk, 0, sizeof(*blk));
	dcfg->channel_direction = PERIPHERAL_TO_MEMORY;
	dcfg->dma_slot = RPI_PICO_DMA_DREQ_TO_SLOT(pio_get_dreq(data->pio, data->sm, false));
	dcfg->source_data_size = 1;
	dcfg->dest_data_size = 1;
	dcfg->source_burst_length = 1;
	dcfg->dest_burst_length = 1;
	dcfg->block_count = 1;
	dcfg->head_block = blk;
	dcfg->dma_callback = rp2040_pdm_dma_cb;
	dcfg->user_data = data;
	blk->source_address = (uint32_t)&data->pio->rxf[data->sm];
	blk->dest_address = (uint32_t)data->raw_buf[0];
	blk->block_size = RAW_BUFFER_SIZE;
	blk->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	ret = dma_config(hw->dma_dev, hw->dma_channel, dcfg);
	if (ret < 0) {
		LOG_ERR("dma_config failed %d", ret);
		return ret;
	}

	/* Start the PDM clock from a clean state machine */
	pio_sm_clear_fifos(data->pio, data->sm);
	pio_sm_restart(data->pio, data->sm);
	pio_sm_exec(data->pio, data->sm, pio_encode_jmp(data->pio_offset));

	data->running = true;
	ret = dma_start(hw->dma_dev, hw->dma_channel);
	if (ret < 0) {
		data->running = false;
		LOG_ERR("dma_start failed %d", ret);
		return ret;
	}
	pio_sm_set_enabled(data->pio, data->sm, true);

	/* Small delay for mic to stabilise */
	k_sleep(K_MSEC(100));

	return 0;
}

static void rp2040_pdm_stop(const struct device *dev)
{
	const struct rp2040_pdm_config *hw = dev->config;
	struct rp2040_pdm_data *data = dev->data;
	struct dma_status stat = {.busy = true};

	data->running = false;
	pio_sm_set_enabled(data->pio, data->sm, false);
	dma_stop(hw->dma_dev, hw->dma_channel);

	/* dma_stop() does not wait for the abort to complete */
	for (int i = 0; i < 1000 && stat.busy; i++) {
		if (dma_get_status(hw->dma_dev, hw->dma_channel, &stat) < 0) {
			break;
		}
	}

	if (data->cur_block != NULL) {
		k_mem_slab_free(data->mem_slab, data->cur_block);
		data->cur_block = NULL;
	}
}

static int rp2040_pdm_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct rp2040_pdm_data *data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_START:
		if (!data->configured) {
			return -EIO;
		}
		if (data->running) {
			return 0;
		}
		return rp2040_pdm_start(dev);

	case DMIC_TRIGGER_STOP:
		if (data->running) {
			rp2040_pdm_stop(dev);
		}
		return 0;

	default:
		return -EINVAL;
	}
}

static int rp2040_pdm_read(const struct device *dev, uint8_t stream,
			    void **buf, size_t *size, int32_t timeout)
{
	struct rp2040_pdm_data *data = dev->data;
	void *block = NULL;

	ARG_UNUSED(stream);

	/* SYS_FOREVER_MS = -1 must map to K_FOREVER, not K_MSEC(-1). */
	k_timeout_t t = (timeout == SYS_FOREVER_MS) ? K_FOREVER : K_MSEC(timeout);
	int ret = k_msgq_get(&data->rx_queue, &block, t);

	if (ret < 0) {
		return ret;
	}

	*buf  = block;
	*size = data->block_size;
	return 0;
}

static DEVICE_API(dmic, rp2040_pdm_ops) = {
	.configure = rp2040_pdm_configure,
	.trigger   = rp2040_pdm_trigger,
	.read      = rp2040_pdm_read,
};

/* --- Driver instance init --------------------------------------------------- */

static int rp2040_pdm_init(const struct device *dev)
{
	const struct rp2040_pdm_config *hw = dev->config;
	struct rp2040_pdm_data *data = dev->data;
	int ret;

	if (!device_is_ready(hw->piodev) || !device_is_ready(hw->dma_dev) ||
	    !device_is_ready(hw->clk_dev)) {
		return -ENODEV;
	}

	ret = pinctrl_apply_state(hw->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;
	data->pio = pio_rpi_pico_get_pio(hw->piodev);

	if (!pio_can_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(pdm_pio))) {
		return -EBUSY;
	}

	ret = pio_rpi_pico_allocate_sm(hw->piodev, &data->sm);
	if (ret < 0) {
		return ret;
	}

	data->pio_offset = pio_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(pdm_pio));

	k_msgq_init(&data->rx_queue, (char *)data->rx_queue_buf,
		    sizeof(void *), QUEUE_DEPTH);

	data->configured = false;
	data->running    = false;
	data->cur_block  = NULL;

	return 0;
}

/* --- DT instance macro ------------------------------------------------------ */

#define RP2040_PDM_INIT(inst)                                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, gain) > 0 &&                                   \
		     DT_INST_PROP(inst, gain) <= UINT16_MAX,                            \
		     "gain must be in 1..65535");                                       \
	PINCTRL_DT_INST_DEFINE(inst);                                                  \
	static struct rp2040_pdm_data rp2040_pdm_data_##inst;                          \
	static const struct rp2040_pdm_config rp2040_pdm_cfg_##inst = {                \
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                          \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                   \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(inst, clocks, 0,  \
								      clk_id),          \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, rx)),         \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(inst, rx, channel),           \
		.clk_pin = DT_INST_RPI_PICO_PIO_PIN_BY_NAME(inst, default, 0,          \
							    clk_gpio, 0),               \
		.din_pin = DT_INST_RPI_PICO_PIO_PIN_BY_NAME(inst, default, 0,          \
							    din_gpio, 0),               \
		.gain = DT_INST_PROP(inst, gain),                                      \
	};                                                                             \
	DEVICE_DT_INST_DEFINE(inst, rp2040_pdm_init, NULL,                             \
			      &rp2040_pdm_data_##inst,                                 \
			      &rp2040_pdm_cfg_##inst,                                  \
			      POST_KERNEL,                                             \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY,                         \
			      &rp2040_pdm_ops);

DT_INST_FOREACH_STATUS_OKAY(RP2040_PDM_INIT)
