/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RP2040 PIO-based DMIC driver.
 *
 *   - PIO state machine generates PDM clock and samples bits into RX FIFO
 *   - DMA transfers raw PDM bits from PIO RX FIFO to a ping-pong buffer
 *   - DMA completion ISR (DMA_IRQ_0) runs the PDM decimation filter
 *   - Resulting PCM blocks are posted to a k_msgq for dmic_read() callers
 */

#define DT_DRV_COMPAT raspberrypi_rp2040_pdm

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/irq.h>

#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>
#include <hardware/regs/intctrl.h> /* DMA_IRQ_0 = 11 */

#include "dmic_rp2040_pio.pio.h"

/* RAW_BUFFER_SIZE must be a multiple of (decimation / 8).
 * For decimation=128: multiple of 16; for decimation=64: multiple of 8.
 * 512 satisfies both.
 */
#define RAW_BUFFER_SIZE  512
#define DECIMATION       128
#define QUEUE_DEPTH      4

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
	uint8_t clk_pin;
	uint8_t din_pin;
	uint8_t pio_num;
	int     gain;
};

struct rp2040_pdm_data {
	/* PIO */
	PIO  pio;
	uint sm;
	uint pio_offset;

	/* DMA */
	int  dma_ch;

	/* Raw PDM ping-pong buffers */
	uint8_t raw_buf[2][RAW_BUFFER_SIZE];
	volatile int raw_buf_idx;

	/* PDM decimation filter state */
	struct pdm_filter_state filter;
	int cut_frames;  /* PCM blocks to mute at startup */

	/* DMIC API contract: caller provides these in configure() */
	struct k_mem_slab *mem_slab;
	uint32_t block_size;

	/* Queue of completed PCM blocks returned by dmic_read() */
	struct k_msgq rx_queue;
	void *rx_queue_buf[QUEUE_DEPTH];

	bool configured;
	bool running;
};

/* --- ISR ------------------------------------------------------------------ */

static void rp2040_pdm_dma_isr(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct rp2040_pdm_data *data = dev->data;

	/* Acknowledge DMA_IRQ_0 for our channel */
	if (!(dma_hw->ints0 & (1u << data->dma_ch))) {
		return; /* Not our channel - spurious or shared IRQ */
	}
	dma_hw->ints0 = 1u << data->dma_ch;

	/* Restart DMA to the alternate buffer before processing.
	 * Must reload TRANS_COUNT explicitly - after completion it is 0.
	 */
	int shadow = data->raw_buf_idx ^ 1;

	dma_channel_set_trans_count(data->dma_ch, RAW_BUFFER_SIZE, false);
	dma_channel_set_write_addr(data->dma_ch, data->raw_buf[shadow], true);

	/* Allocate a PCM block from the caller-supplied slab */
	void *pcm_buf;

	if (k_mem_slab_alloc(data->mem_slab, &pcm_buf, K_NO_WAIT) != 0) {
		/* Drop frame if no buffer available */
		data->raw_buf_idx = shadow;
		return;
	}

	/* Decimate raw PDM bits into PCM samples */
	int16_t *dst = (int16_t *)pcm_buf;

	if (data->filter.Decimation == 128) {
		pdm_filter_decimate_128(data->raw_buf[data->raw_buf_idx], dst, 1, &data->filter);
	} else {
		pdm_filter_decimate_64(data->raw_buf[data->raw_buf_idx], dst, 1, &data->filter);
	}

	/* Mute the first few frames (mic startup noise) */
	if (data->cut_frames > 0) {
		memset(pcm_buf, 0, data->block_size);
		data->cut_frames--;
	}

	data->raw_buf_idx = shadow;

	/* Post to queue; drop if full */
	if (k_msgq_put(&data->rx_queue, &pcm_buf, K_NO_WAIT) != 0) {
		k_mem_slab_free(data->mem_slab, pcm_buf);
	}
}

/* --- Driver ops ------------------------------------------------------------- */

static int rp2040_pdm_configure(const struct device *dev, struct dmic_cfg *cfg)
{
	const struct rp2040_pdm_config *hw = dev->config;
	struct rp2040_pdm_data *data = dev->data;

	if (cfg->channel.req_num_streams < 1 ||
	    cfg->streams[0].pcm_width != 16) {
		return -EINVAL;
	}

	/* Release resources from a previous configure() */
	if (data->configured) {
		pio_sm_set_enabled(data->pio, data->sm, false);
		pio_sm_unclaim(data->pio, data->sm);
		pio_remove_program(data->pio, &pdm_pio_program, data->pio_offset);
		dma_channel_unclaim(data->dma_ch);
		data->configured = false;
	}

	uint32_t sample_rate = cfg->streams[0].pcm_rate;

	/* Store parameters for trigger/read */
	data->mem_slab  = cfg->streams[0].mem_slab;
	data->block_size = cfg->streams[0].block_size;

	/* Select decimation factor based on sample rate and clock constraints.
	 * PDM clock = sample_rate x decimation x 2.
	 * Mic accepts 1.2 - 3.25 MHz: prefer 128, fall back to 64.
	 */
	int decimation = DECIMATION;

	if ((uint64_t)sample_rate * decimation * 2 > 3250000U) {
		decimation = 64;
	}

	int raw_buf_len = RAW_BUFFER_SIZE / (decimation / 8);
	int pcm_samples = raw_buf_len;
	uint32_t expected_block_size = (uint32_t)pcm_samples * sizeof(int16_t);

	/* The caller's block_size must match what the filter produces */
	if (data->block_size != expected_block_size) {
		data->block_size = expected_block_size;
	}

	/* Initialise the decimation filter (mbed-compatible defaults). */
	data->filter.Fs         = sample_rate;
	data->filter.MaxVolume  = 1;
	data->filter.nSamples   = pcm_samples;
	data->filter.LP_HZ      = sample_rate / 2;
	data->filter.HP_HZ      = 10;
	data->filter.Decimation = decimation;
	data->filter.filterGain = hw->gain;
	pdm_filter_init(&data->filter);

	/* Select PIO block */
	data->pio = (hw->pio_num == 0) ? pio0 : pio1;

	/* Add PIO program */
	if (!pio_can_add_program(data->pio, &pdm_pio_program)) {
		return -ENODEV;
	}
	data->pio_offset = pio_add_program(data->pio, &pdm_pio_program);

	/* Claim a free state machine */
	int sm = pio_claim_unused_sm(data->pio, false);

	if (sm < 0) {
		pio_remove_program(data->pio, &pdm_pio_program, data->pio_offset);
		return -EBUSY;
	}
	data->sm = (uint)sm;

	/* Compute clock divider: sys_clk / (sample_rate x decimation x 2).
	 * Note: pico-sdk's clock_get_hz(clk_sys) returns 0 because Zephyr's
	 * clock_control driver bring up the PLLs but never populates pico-sdk's
	 * internal configured_freq[] table. Read the rate from devicetree.
	 */
	const uint32_t sys_clk_hz =
		DT_PROP(DT_NODELABEL(clk_sys), clock_frequency);
	float clk_div = (float)sys_clk_hz /
			(float)(sample_rate * (uint32_t)decimation * 2u);
	pdm_pio_program_init(data->pio, data->sm, data->pio_offset,
			     hw->clk_pin, hw->din_pin, clk_div);

	/* Claim DMA channel */
	data->dma_ch = dma_claim_unused_channel(true);

	data->configured = true;
	return 0;
}

static int rp2040_pdm_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct rp2040_pdm_data *data = dev->data;

	if (!data->configured && cmd == DMIC_TRIGGER_START) {
		return -EIO;
	}

	switch (cmd) {
	case DMIC_TRIGGER_START: {
		if (data->running) {
			return 0;
		}

		/* Flush the queue */
		k_msgq_purge(&data->rx_queue);

		data->raw_buf_idx = 0;
		data->cut_frames = 5; /* mute first 5 PCM blocks at startup */

		/* Set up DMA: PIO RX FIFO to raw_buf[0], 8-bit transfers */
		dma_channel_config c = dma_channel_get_default_config(data->dma_ch);

		channel_config_set_read_increment(&c, false);
		channel_config_set_write_increment(&c, true);
		channel_config_set_dreq(&c, pio_get_dreq(data->pio, data->sm, false));
		channel_config_set_transfer_data_size(&c, DMA_SIZE_8);

		/* Enable DMA_IRQ_0 for our channel */
		dma_hw->ints0 = 1u << data->dma_ch; /* clear any pending */
		dma_channel_set_irq0_enabled(data->dma_ch, true);

		dma_channel_configure(data->dma_ch, &c,
			data->raw_buf[0],          /* destination */
			&data->pio->rxf[data->sm], /* source */
			RAW_BUFFER_SIZE,
			true);                     /* start immediately */

		irq_enable(DMA_IRQ_0);

		/* Small delay for mic to stabilise */
		k_sleep(K_MSEC(100));

		data->running = true;
		break;
	}

	case DMIC_TRIGGER_STOP:
		if (!data->running) {
			return 0;
		}
		irq_disable(DMA_IRQ_0);
		dma_channel_set_irq0_enabled(data->dma_ch, false);
		dma_channel_abort(data->dma_ch);
		pio_sm_set_enabled(data->pio, data->sm, false);
		data->running = false;
		break;

	default:
		return -EINVAL;
	}

	return 0;
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
	struct rp2040_pdm_data *data = dev->data;

	/* Initialise the PCM block message queue */
	k_msgq_init(&data->rx_queue, (char *)data->rx_queue_buf,
		    sizeof(void *), QUEUE_DEPTH);

	data->configured = false;
	data->running    = false;

	/* Connect DMA_IRQ_0 to our ISR.  The IRQ is left disabled here;
	 * rp2040_pdm_trigger(START) enables it.
	 */
	IRQ_CONNECT(DMA_IRQ_0, 3,
		    rp2040_pdm_dma_isr, DEVICE_DT_INST_GET(0), 0);

	return 0;
}

/* --- DT instance macro ------------------------------------------------------ */

#define RP2040_PDM_INIT(inst)                                           \
	static struct rp2040_pdm_data rp2040_pdm_data_##inst;           \
	static const struct rp2040_pdm_config rp2040_pdm_cfg_##inst = { \
		.clk_pin = DT_INST_PROP(inst, clk_pin),                 \
		.din_pin = DT_INST_PROP(inst, din_pin),                 \
		.pio_num = DT_INST_PROP(inst, pio_num),                 \
		.gain    = DT_INST_PROP(inst, gain),                    \
	};                                                               \
	DEVICE_DT_INST_DEFINE(inst, rp2040_pdm_init, NULL,               \
			      &rp2040_pdm_data_##inst,                   \
			      &rp2040_pdm_cfg_##inst,                    \
			      POST_KERNEL,                               \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY,           \
			      &rp2040_pdm_ops);

DT_INST_FOREACH_STATUS_OKAY(RP2040_PDM_INIT)
