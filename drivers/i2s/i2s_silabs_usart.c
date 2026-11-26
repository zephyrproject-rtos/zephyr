/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * EFR32 USART I2S driver (stereo and mono TX).
 *
 * RX is not supported yet (i2s_configure(I2S_DIR_RX) returns -ENOTSUP).
 *
 * BCLK and LRCLK (WS) toggle only while TX data is shifting out. configure()
 * requires I2S_OPT_BIT_CLK_GATED; I2S_OPT_BIT_CLK_CONT is rejected with -EINVAL.
 *
 * The hardware always emits two channel slots per LRCLK frame on the wire.
 *
 * USART DMA request naming (RM / SoC headers):
 *   TXBL      = Transmit Buffer Level (NOT "left"). Fires when the TX FIFO has
 *               space for another sample. With I2SCTRL.DMASPLIT=0 it requests
 *               one LDMA move for the next word(s) on the bus (stereo: L+R).
 *   TXBLRIGHT = Separate DMA request when DMASPLIT=1; targets the right I2S
 *               slot only.
 *
 * Stereo TX (configure channels == 2, I2SCTRL.DMASPLIT = 0):
 *   One LDMA channel on USARTnTXBL writes interleaved L/R samples to
 *   USARTn_TXDOUBLE. Buffer: [L0, R0, L1, R1, ...].
 *
 * Mono TX (configure channels == 1, I2SCTRL.DMASPLIT = 1):
 *   Two LDMA channels (devicetree "txbl" + "txblright"):
 *     - USARTnTXBL:      coincidentally the LRCLK-low / left wire slot
 *     - USARTnTXBLRIGHT: LRCLK-high / right wire slot
 *   Slot assignment comes from silabs,mono-tx-slot (default right).
 *   Application buffer: [M0, M1, ...] (one sample per LRCLK frame).
 *   I2SCTRL.MONO stays 0; LRCLK still toggles L/R on the bus.
 *
 * With DATABITS=16, USARTn_TXDATA loads only 8 FIFO bits per write; a full
 * I2S slot needs USARTn_TXDOUBLE (see RM 22.3.2.6 / 22.3.2.19). Stereo
 * and mono TX both target TXDOUBLE; mono moves one slot per DMA trigger.
 *
 * i2s_config.word_size must be 16: FORMAT = W16D16, BCLK = 32 x frame clock.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

#include <sl_hal_usart.h>
#include <sl_hal_ldma.h>

#define DT_DRV_COMPAT silabs_usart_i2s

#define I2S_SILABS_SUPPORTED_OPTIONS \
	(I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_GATED)

#define I2S_SILABS_BLOCK_Q_MAX_DEPTH CONFIG_I2S_SILABS_USART_TX_BLOCK_COUNT

/* Stereo: every LRCLK frame carries L + R = 2 channel slots. */
#define I2S_STEREO_SLOTS_PER_FRAME 2U

/* The only supported i2s_config.word_size (W16D16). */
#define I2S_SILABS_WORD_SIZE 16U

/* Bytes of one slot in memory, also the LDMA beat size. */
#define I2S_SILABS_SLOT_BYTES 2U

/*
 * One LDMA descriptor holds at most SL_HAL_LDMA_DESCRIPTOR_MAX_XFER_SIZE
 * transfers of one LDMA beat (one slot). dma_silabs_ldma splits larger
 * dma_block_config.block_size values into linked descriptors and fires
 * complete_callback per descriptor; this driver frees the block on each
 * callback. silabs_ldma_append_block() also rejects oversized blocks.
 */
#define I2S_SILABS_LDMA_MAX_BLOCK_BYTES \
	(SL_HAL_LDMA_DESCRIPTOR_MAX_XFER_SIZE * I2S_SILABS_SLOT_BYTES)

/*
 * Minimum sample rate accepted by configure(). Below 8 kHz the producer-side
 * timing tolerance shrinks and the LDMA TXBL trigger latency dominates audio
 * quality. Most I2S codecs (e.g. TAS2505) also spec >= 8 kHz.
 */
#define I2S_MIN_FRAME_CLK_HZ 8000U

/*
 * LDMA channel priority (0 = highest, 3 = lowest in the Silabs LDMA driver).
 * I2S TX must keep up with the codec but other DMA users (UART console,
 * crypto, etc.) typically have shorter deadlines, so I2S sits at the lowest
 * priority slot.
 */
#define I2S_DMA_CHANNEL_PRIORITY 3U

/*
 * Boolean for dma_config.complete_callback_en (1 = invoke dma_callback on
 * each block completion -- required for our gapless append/chain logic).
 */
#define I2S_DMA_COMPLETE_CB_ENABLED 1U

/* Values are the enum indexes of the silabs,mono-tx-slot devicetree property. */
enum i2s_silabs_usart_mono_tx_slot {
	I2S_SILABS_USART_MONO_TX_SLOT_LEFT = 0,
	I2S_SILABS_USART_MONO_TX_SLOT_RIGHT = 1,
};

/*
 * One queued I2S memory block (pointer + byte length). Stored by value in a
 * k_msgq for ISR/thread-safe block handoff.
 */
struct i2s_silabs_usart_block {
	void *blk;
	size_t len;
};

/*
 * Per-stream DMA runtime state only. Descriptor templates (dma_cfg / blk_cfg)
 * live in instance-static storage pointed from i2s_silabs_usart_cfg.
 */
struct i2s_silabs_usart_dma {
	int channel;
	bool busy;
};

struct i2s_silabs_usart_dma_desc {
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
};

struct i2s_silabs_usart_stream {
	int32_t state;
	bool cfg_valid;
	/*
	 * When state is STOPPING: true = I2S_TRIGGER_DRAIN (empty the queue),
	 * false = I2S_TRIGGER_STOP (finish the current/pending block only).
	 */
	bool drain;
	struct i2s_config cfg;
	struct k_msgq q;
	char q_buf[I2S_SILABS_BLOCK_Q_MAX_DEPTH * sizeof(struct i2s_silabs_usart_block)];
	/*
	 * One-slot undo for a failed silabs_ldma_append_block(): the block was
	 * already accepted by i2s_write() and must stay at the queue head.
	 * .blk == NULL means empty. Checked before k_msgq in TX get paths.
	 */
	struct i2s_silabs_usart_block q_unpop;
	void *active;
	void *pending;
	struct i2s_silabs_usart_dma dma;
	struct k_sem sem;
};

/*
 * data->lock covers the stream state, the active/pending blocks, the DMA
 * busy flags and the shared descriptor templates.
 */
struct i2s_silabs_usart_data {
	struct i2s_silabs_usart_stream tx;
	struct i2s_silabs_usart_dma tx_silence;
	struct k_mutex cfg_lock;
	struct k_spinlock lock;
};

struct i2s_silabs_usart_cfg {
	USART_TypeDef *base;
	const struct device *clock_dev;
	struct silabs_clock_control_cmu_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	const struct device *dma_dev;
	uint32_t dma_txbl_slot;      /* LDMA slot: USARTnTXBL */
	uint32_t dma_txblright_slot; /* LDMA slot: USARTnTXBLRIGHT (mono DMASPLIT only) */
	struct i2s_silabs_usart_dma_desc *tx_dma;
	/* NULL when mono TX (txblright) is not wired in DT. */
	struct i2s_silabs_usart_dma_desc *tx_silence_dma;
	const struct device *mclk_dev;      /* optional CMU CLKOUT providing codec MCLK */
	clock_control_subsys_t mclk_subsys; /* output index for series-clock-output */
	bool has_tx_split;
	uint8_t mono_tx_default;
};

/*
 * Shared mono pad word (BSS / RAM so LDMA can always read it). Always zero;
 * never written. Not per-device -- one word for all instances.
 */
static uint16_t i2s_silabs_usart_silence_word;

/*
 * Gate codec MCLK with the standard clock_control API: enabled before I2S
 * BCLK/LRCLK are started, disabled after they are stopped on de-configure.
 * Wired via silabs,mclk-out. The phandle is a clock-output@N child under
 * silabs,series-clock-output; the device is the parent &clkout and subsys is
 * the child's reg (CLKOUT index).
 */
static void i2s_silabs_usart_mclk_set(const struct device *mclk, clock_control_subsys_t subsys,
				      bool enable)
{
	if (mclk == NULL) {
		return;
	}
	if (enable) {
		(void)clock_control_on(mclk, subsys);
	} else {
		(void)clock_control_off(mclk, subsys);
	}
}

/*
 * TX blocks must come from cfg.mem_slab (i2s_write contract). The driver takes
 * ownership and always returns the block to that slab when done.
 */
static void i2s_silabs_usart_tx_release_block(struct i2s_silabs_usart_stream *tx, void *blk)
{
	if (blk == NULL) {
		return;
	}

	k_mem_slab_free(tx->cfg.mem_slab, blk);
	k_sem_give(&tx->sem);
}

static inline bool i2s_silabs_usart_tx_cfg_is_mono(const struct i2s_config *cfg)
{
	return cfg->channels == 1U;
}

static inline bool i2s_silabs_usart_tx_path_is_mono(const struct device *dev)
{
	const struct i2s_silabs_usart_data *data = dev->data;

	return data->tx.cfg_valid && i2s_silabs_usart_tx_cfg_is_mono(&data->tx.cfg);
}

static inline int i2s_silabs_usart_block_q_put(struct k_msgq *q, void *blk, size_t len)
{
	struct i2s_silabs_usart_block item = {.blk = blk, .len = len};

	return k_msgq_put(q, &item, K_NO_WAIT);
}

static inline int i2s_silabs_usart_block_q_get(struct k_msgq *q, void **blk, size_t *len)
{
	struct i2s_silabs_usart_block item = {0};
	int ret = k_msgq_get(q, &item, K_NO_WAIT);

	if (ret == 0) {
		*blk = item.blk;
		*len = item.len;
	}
	return ret;
}

static inline bool i2s_silabs_usart_tx_q_empty(const struct i2s_silabs_usart_stream *tx)
{
	return tx->q_unpop.blk == NULL &&
		k_msgq_num_used_get((struct k_msgq *)&tx->q) == 0U;
}

static inline int i2s_silabs_usart_tx_q_get(struct i2s_silabs_usart_stream *tx, void **blk,
					    size_t *len)
{
	if (tx->q_unpop.blk != NULL) {
		*blk = tx->q_unpop.blk;
		*len = tx->q_unpop.len;
		tx->q_unpop.blk = NULL;
		tx->q_unpop.len = 0U;
		return 0;
	}

	return i2s_silabs_usart_block_q_get(&tx->q, blk, len);
}

/* Return a just-popped TX block to the head of the queue (next one out). */
static inline int i2s_silabs_usart_tx_q_unpop(struct i2s_silabs_usart_stream *tx, void *blk,
					      size_t len)
{
	if (tx->q_unpop.blk != NULL) {
		return -ENOMEM;
	}

	tx->q_unpop.blk = blk;
	tx->q_unpop.len = len;
	return 0;
}

static void i2s_silabs_usart_dma_stop_if_busy(const struct device *dev,
					      struct i2s_silabs_usart_dma *dma)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;

	if (dma->busy) {
		dma_stop(cfg->dma_dev, (uint32_t)dma->channel);
		dma->busy = false;
	}
}

/* Stop the mono pad-slot channel started by i2s_silabs_usart_mono_tx_silence_run(). */
static void i2s_silabs_usart_mono_tx_silence_stop(const struct device *dev)
{
	struct i2s_silabs_usart_data *data = dev->data;

	if (i2s_silabs_usart_tx_path_is_mono(dev)) {
		i2s_silabs_usart_dma_stop_if_busy(dev, &data->tx_silence);
	}
}

/*
 * Select the TX LDMA request(s). The fixed descriptor fields are set once by
 * i2s_silabs_usart_init().
 */
static void i2s_silabs_usart_tx_dma_apply(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	/* DMASPLIT=0: USARTnTXBL requests every slot (interleaved L+R). */
	bool audio_on_txbl = true;

	if (i2s_silabs_usart_tx_path_is_mono(dev)) {
		/* DMASPLIT=1: audio on one slot request, the zero pad word on the other. */
		audio_on_txbl = pcfg->mono_tx_default == I2S_SILABS_USART_MONO_TX_SLOT_LEFT;
		pcfg->tx_silence_dma->dma_cfg.dma_slot = audio_on_txbl ? pcfg->dma_txblright_slot
									: pcfg->dma_txbl_slot;
	}
	pcfg->tx_dma->dma_cfg.dma_slot = audio_on_txbl ? pcfg->dma_txbl_slot
							: pcfg->dma_txblright_slot;
}

static void i2s_silabs_usart_hw_disable_data_irqs(USART_TypeDef *base)
{
	sl_hal_usart_disable_interrupts(base, USART_IF_TXBL | USART_IF_RXDATAV);
	sl_hal_usart_clear_interrupts(base, USART_IF_TXBL | USART_IF_RXDATAV);
}

static void i2s_silabs_usart_hw_disable_error_irqs(USART_TypeDef *base)
{
	sl_hal_usart_disable_interrupts(base, USART_IF_TXUF);
	sl_hal_usart_clear_interrupts(base, USART_IF_TXUF);
}

/*
 * Compute USART CLKDIV for sync (I2S) mode with fractional precision.
 *
 * sl_hal_usart_sync_calculate_clock_div() only yields the integer divider
 * (clkdiv = (ref/(2*br)) << 8), which leaves the fractional sub-field at 0.
 *
 * RM: br = (128 * ref) / (256 + CLKDIV)  =>  CLKDIV = round(128*ref/br) - 256
 *
 * Returns 0 and stores the divider in *clkdiv_out on success, or -EINVAL when
 * ref_hz or baud_hz is zero, or when baud_hz exceeds what the USART divider
 * can reach (CLKDIV would clamp to the minimum).
 */
static int i2s_silabs_usart_sync_clkdiv(uint32_t ref_hz, uint32_t baud_hz, uint32_t *clkdiv_out)
{
	uint64_t clkdiv;

	if (ref_hz == 0U || baud_hz == 0U) {
		return -EINVAL;
	}

	clkdiv = ((uint64_t)128U * ref_hz + (uint64_t)baud_hz / 2U) / (uint64_t)baud_hz;

	if (clkdiv < 256U) {
		return -EINVAL;
	}
	clkdiv -= 256U;

#if defined(_USART_CLKDIV_DIVEXT_MASK)
	*clkdiv_out = (uint32_t)(clkdiv & (_USART_CLKDIV_DIV_MASK | _USART_CLKDIV_DIVEXT_MASK));
#else
	*clkdiv_out = (uint32_t)(clkdiv & _USART_CLKDIV_DIV_MASK);
#endif
	return 0;
}

/* Checks that do not depend on the USART reference clock. */
static int i2s_silabs_usart_validate_i2s_cfg(const struct i2s_config *ic)
{
	/* Controller only: the mask also rejects I2S_OPT_BIT_CLK_TARGET / FRAME_CLK_TARGET. */
	if ((ic->options & I2S_SILABS_SUPPORTED_OPTIONS) != ic->options) {
		return -EINVAL;
	}
	/*
	 * BCLK/LRCLK run only while TX data is on the wire; there is no free-running
	 * clock before START or with an empty queue. Require I2S_OPT_BIT_CLK_GATED.
	 */
	if ((ic->options & I2S_OPT_BIT_CLK_GATED) == 0U) {
		return -EINVAL;
	}
	if (ic->word_size != I2S_SILABS_WORD_SIZE) {
		return -EINVAL;
	}
	if (ic->frame_clk_freq < I2S_MIN_FRAME_CLK_HZ) {
		return -EINVAL;
	}
	if (ic->channels != 1U && ic->channels != 2U) {
		return -EINVAL;
	}

	switch (ic->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		break;
	default:
		return -EINVAL;
	}

	/* USART I2S is MSB-first; WS polarity is not inverted. */
	if ((ic->format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		return -EINVAL;
	}
	if ((ic->format & I2S_FMT_FRAME_CLK_INV) != 0U) {
		return -EINVAL;
	}

	return 0;
}

/* Check the TX bit clock against the USART reference clock. */
static int i2s_silabs_usart_validate_config(const struct device *dev,
					    const struct i2s_config **out_ic, uint32_t *ref_hz)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	const struct i2s_silabs_usart_data *data = dev->data;
	const struct i2s_config *ic;
	uint64_t bit_hz;
	int err;

	if (!data->tx.cfg_valid) {
		return -EINVAL;
	}

	err = clock_control_get_rate(pcfg->clock_dev,
				     (clock_control_subsys_t)(uintptr_t)&pcfg->clock_cfg, ref_hz);
	if (err < 0) {
		return err;
	}
	if (*ref_hz == 0U) {
		return -EINVAL;
	}

	/* The config passed i2s_silabs_usart_validate_i2s_cfg() in configure(). */
	ic = &data->tx.cfg;

	/* RM: minimum CLKDIV is 0, so max SCLK is ref_hz / 2. */
	bit_hz = (uint64_t)ic->frame_clk_freq * I2S_STEREO_SLOTS_PER_FRAME * I2S_SILABS_WORD_SIZE;
	if (bit_hz > (uint64_t)*ref_hz / 2U) {
		return -EINVAL;
	}

	*out_ic = ic;
	return 0;
}

static int i2s_silabs_usart_hw_apply(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	const struct i2s_config *ic;
	sl_hal_usart_i2s_init_t init = SL_HAL_USART_INIT_I2S_DEFAULT;
	uint32_t ref_hz = 0;
	uint32_t bit_hz;
	uint32_t clkdiv;
	i2s_fmt_t clk_fmt;
	int err;

	err = i2s_silabs_usart_validate_config(dev, &ic, &ref_hz);
	if (err < 0) {
		return err;
	}

	clk_fmt = ic->format & I2S_FMT_CLK_FORMAT_MASK;

	/*
	 * Physical SCLK (bit clock); the wire always carries two slots per frame.
	 * Example: frame_clk=16 kHz -> bit_hz = 512 kHz.
	 */
	bit_hz = ic->frame_clk_freq * I2S_STEREO_SLOTS_PER_FRAME * I2S_SILABS_WORD_SIZE;

	i2s_silabs_usart_tx_dma_apply(dev);
	i2s_silabs_usart_hw_disable_data_irqs(pcfg->base);

	init.sync.clock_div = sl_hal_usart_sync_calculate_clock_div(ref_hz, bit_hz);
	init.sync.data_bits = SL_HAL_USART_DATA_BITS_16;
	init.sync.master = true;
	init.sync.msb_first = true;
	init.sync.clock_mode = (clk_fmt == I2S_FMT_CLK_NF_IB || clk_fmt == I2S_FMT_CLK_IF_IB)
					? SL_HAL_USART_CLOCK_MODE_1
					: SL_HAL_USART_CLOCK_MODE_0;
	init.format = SL_HAL_USART_I2S_FORMAT_W16D16;
	init.justify = SL_HAL_USART_JUSTIFY_LEFT;
	/*
	 * Wire format stays stereo (MONO=0): LRCLK toggles L/R. Mono TX uses
	 * DMASPLIT + dual LDMA (samples on one slot, silence on the other).
	 */
	init.mono = false;
	init.dma_split = i2s_silabs_usart_tx_path_is_mono(dev);
	init.delay = (ic->format & I2S_FMT_DATA_FORMAT_MASK) == I2S_FMT_DATA_FORMAT_I2S;

	sl_hal_usart_reset(pcfg->base);
	sl_hal_usart_init_i2s(pcfg->base, &init);

	/*
	 * Override the integer-only CLKDIV that sl_hal_usart_init_i2s() programs so
	 * BCLK matches bit_hz as closely as the USART fractional divider allows.
	 * Only apply the override when i2s_silabs_usart_sync_clkdiv() returns a valid
	 * value; otherwise keep the divider the init function programmed.
	 */
	if (i2s_silabs_usart_sync_clkdiv(ref_hz, bit_hz, &clkdiv) == 0) {
		pcfg->base->CLKDIV = clkdiv;
	}

	sl_hal_usart_enable(pcfg->base);
	sl_hal_usart_enable_tx(pcfg->base);

	return 0;
}

/* Point the shared TX descriptor template at blk. Call with data->lock held. */
static void i2s_silabs_usart_tx_desc_set(const struct device *dev, void *blk, size_t len)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct dma_block_config *blk_cfg = &cfg->tx_dma->blk_cfg;

	blk_cfg->source_address = (uintptr_t)blk;
	blk_cfg->block_size = len;
}

/*
 * Pick the next application block for the TX channel. idle means the channel
 * has stopped, which is a TX underrun when it happens with an empty queue
 * while RUNNING. Call with data->lock held.
 */
static bool i2s_silabs_usart_tx_next_locked(struct i2s_silabs_usart_data *data, bool idle,
					    void **blk, size_t *len)
{
	struct i2s_silabs_usart_stream *tx = &data->tx;

	/* STOPPING without drain (I2S_TRIGGER_STOP) pulls no more application blocks. */
	if (tx->cfg_valid &&
	    (tx->state == I2S_STATE_RUNNING || (tx->state == I2S_STATE_STOPPING && tx->drain))) {
		if (i2s_silabs_usart_tx_q_get(tx, blk, len) == 0) {
			return true;
		}
		/* A block written before the active one completes is still gapless. */
		if (tx->state == I2S_STATE_RUNNING && idle) {
			tx->state = I2S_STATE_ERROR;
		}
	}

	return false;
}

/*
 * Start the mono pad-slot (silence) channel as a self-linked, endlessly looping
 * descriptor. It only moves a word when the USART requests the pad slot, so it
 * stays in lockstep with the audio channel without needing per-block appends.
 * (Per-block appends on this channel raced the audio channel's completion IRQ:
 * when its previous link had not yet been consumed the append failed with
 * -EINVAL and the already-accepted audio block was dropped -- audible as
 * distortion on the first tone(s).)
 */
static int i2s_silabs_usart_mono_tx_silence_run(const struct device *dev, size_t len)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_dma *silence = &data->tx_silence;
	struct i2s_silabs_usart_dma_desc *desc = cfg->tx_silence_dma;
	int ret;

	if (silence->busy) {
		return 0;
	}

	desc->blk_cfg.block_size = len;
	desc->blk_cfg.next_block = &desc->blk_cfg;

	ret = dma_config(cfg->dma_dev, (uint32_t)silence->channel, &desc->dma_cfg);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(cfg->dma_dev, (uint32_t)silence->channel);
	if (ret < 0) {
		dma_stop(cfg->dma_dev, (uint32_t)silence->channel);
		return ret;
	}

	silence->busy = true;
	return 0;
}

/* Cold start of the TX channel(s): the mono pad channel first, then the audio channel. */
static int i2s_silabs_usart_tx_dma_config_start(const struct device *dev, size_t len)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	int ret = 0;

	if (i2s_silabs_usart_tx_path_is_mono(dev)) {
		ret = i2s_silabs_usart_mono_tx_silence_run(dev, len);
	}
	if (ret >= 0) {
		ret = dma_config(cfg->dma_dev, (uint32_t)tx->dma.channel, &cfg->tx_dma->dma_cfg);
	}
	if (ret >= 0) {
		ret = dma_start(cfg->dma_dev, (uint32_t)tx->dma.channel);
		if (ret < 0) {
			dma_stop(cfg->dma_dev, (uint32_t)tx->dma.channel);
		}
	}
	if (ret < 0) {
		i2s_silabs_usart_mono_tx_silence_stop(dev);
	}

	return ret;
}

/*
 * Keep the TX channel fed. An idle channel gets a cold start plus a chained
 * second descriptor; a running one gets the next descriptor chained with
 * silabs_ldma_append_block(), which LDMA loads via LINKLOAD without a gap.
 * Call with data->lock held. *release is set to an application block the
 * caller must free after unlocking. A negative return means the channel
 * could not be started.
 */
static int i2s_silabs_usart_tx_feed_locked(const struct device *dev, void **release)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	void *blk;
	size_t len;
	bool idle;
	int ret;

	*release = NULL;
	while (tx->pending == NULL) {
		idle = tx->active == NULL;
		if (!i2s_silabs_usart_tx_next_locked(data, idle, &blk, &len)) {
			return 0;
		}
		i2s_silabs_usart_tx_desc_set(dev, blk, len);

		if (!idle) {
			ret = silabs_ldma_append_block(cfg->dma_dev, (uint32_t)tx->dma.channel,
						       &cfg->tx_dma->dma_cfg);
			if (ret == 0) {
				tx->pending = blk;
			} else if (i2s_silabs_usart_tx_q_unpop(tx, blk, len) < 0) {
				/*
				 * The block was accepted by i2s_write() and is retried
				 * on the next completion. unpop only fails if the undo
				 * slot is taken, which a successful pop rules out.
				 */
				*release = blk;
			}
			return 0;
		}

		ret = i2s_silabs_usart_tx_dma_config_start(dev, len);
		if (ret < 0) {
			tx->state = I2S_STATE_ERROR;
			*release = blk;
			return ret;
		}
		tx->active = blk;
		tx->dma.busy = true;
	}

	return 0;
}

/* A STOPPING TX stream is done once no block is left on the channel. */
static void i2s_silabs_usart_tx_finish_stopping_locked(struct i2s_silabs_usart_stream *tx)
{
	if (tx->state != I2S_STATE_STOPPING) {
		return;
	}
	if (tx->active != NULL || tx->pending != NULL) {
		return;
	}
	/* DRAIN must empty the queue; STOP leaves remaining blocks for later START. */
	if (tx->drain && !i2s_silabs_usart_tx_q_empty(tx)) {
		return;
	}
	tx->drain = false;
	tx->state = I2S_STATE_READY;
}

static void i2s_silabs_usart_tx_kick(const struct device *dev)
{
	struct i2s_silabs_usart_data *data = dev->data;
	void *release;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	(void)i2s_silabs_usart_tx_feed_locked(dev, &release);
	i2s_silabs_usart_tx_finish_stopping_locked(&data->tx);
	k_spin_unlock(&data->lock, key);

	i2s_silabs_usart_tx_release_block(&data->tx, release);
}

/*
 * Stop the TX channel(s) and detach what they carry. Call with data->lock
 * held; the caller releases *active and *pending after unlocking.
 */
static void i2s_silabs_usart_tx_halt_locked(const struct device *dev, void **active,
					    void **pending)
{
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;

	i2s_silabs_usart_dma_stop_if_busy(dev, &tx->dma);
	i2s_silabs_usart_mono_tx_silence_stop(dev);
	*active = tx->active;
	tx->active = NULL;
	*pending = tx->pending;
	tx->pending = NULL;
}

/* Stop the TX channel and free every block the TX stream owns. */
static void i2s_silabs_usart_tx_drop(const struct device *dev)
{
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	void *active;
	void *pending;
	void *blk;
	size_t len;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	i2s_silabs_usart_tx_halt_locked(dev, &active, &pending);
	tx->drain = false;
	tx->state = I2S_STATE_READY;
	k_spin_unlock(&data->lock, key);

	i2s_silabs_usart_tx_release_block(tx, pending);
	i2s_silabs_usart_tx_release_block(tx, active);
	while (i2s_silabs_usart_tx_q_get(tx, &blk, &len) == 0) {
		i2s_silabs_usart_tx_release_block(tx, blk);
	}
}

/* TX DMA error: the TX stream goes to ERROR. */
static void i2s_silabs_usart_tx_abort_stream(const struct device *dev)
{
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	void *active;
	void *pending;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (tx->state == I2S_STATE_RUNNING || tx->state == I2S_STATE_STOPPING) {
		tx->state = I2S_STATE_ERROR;
	}
	i2s_silabs_usart_tx_halt_locked(dev, &active, &pending);
	k_spin_unlock(&data->lock, key);

	i2s_silabs_usart_tx_release_block(tx, pending);
	i2s_silabs_usart_tx_release_block(tx, active);
}

static void i2s_silabs_usart_dma_tx_cb(const struct device *dma_dev, void *user_data,
				       uint32_t channel, int status)
{
	const struct device *dev = user_data;
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	void *done;
	void *release;
	k_spinlock_key_t key;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status < 0) {
		i2s_silabs_usart_tx_abort_stream(dev);
		return;
	}

	key = k_spin_lock(&data->lock);
	done = tx->active;

	if (tx->pending != NULL) {
		/*
		 * silabs_ldma_append_block() chained the pending block: the
		 * LDMA has already auto-started it via LINKLOAD before this
		 * callback fires. Do not call dma_stop(): the channel is
		 * still running.
		 */
		tx->active = tx->pending;
		tx->pending = NULL;
	} else {
		/*
		 * No block was chained: DMA is now idle. Call dma_stop()
		 * to clear the LDMA driver's internal "busy" atomic so the
		 * next dma_config() does not fail with -EBUSY.
		 */
		tx->active = NULL;
		dma_stop(cfg->dma_dev, (uint32_t)tx->dma.channel);
		tx->dma.busy = false;
		i2s_silabs_usart_mono_tx_silence_stop(dev);
	}

	(void)i2s_silabs_usart_tx_feed_locked(dev, &release);
	i2s_silabs_usart_tx_finish_stopping_locked(tx);
	k_spin_unlock(&data->lock, key);

	i2s_silabs_usart_tx_release_block(tx, done);
	i2s_silabs_usart_tx_release_block(tx, release);
}

/* (Re)configuration is allowed only while the stream is idle. */
static bool i2s_silabs_usart_cfg_state_allowed(int32_t state)
{
	return (state == I2S_STATE_NOT_READY) || (state == I2S_STATE_READY);
}

static int i2s_silabs_usart_power_down(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;

	if (!i2s_silabs_usart_cfg_state_allowed(data->tx.state)) {
		return -EINVAL;
	}
	if (data->tx.cfg_valid) {
		i2s_silabs_usart_tx_drop(dev);
	}
	data->tx.cfg_valid = false;
	data->tx.state = I2S_STATE_NOT_READY;

	i2s_silabs_usart_hw_disable_data_irqs(pcfg->base);
	i2s_silabs_usart_hw_disable_error_irqs(pcfg->base);
	sl_hal_usart_disable(pcfg->base);
	i2s_silabs_usart_mclk_set(pcfg->mclk_dev, pcfg->mclk_subsys, false);

	return 0;
}

/* Requires channels already checked by i2s_silabs_usart_validate_i2s_cfg(). */
static int i2s_silabs_usart_validate_configure_block_size(const struct i2s_config *cfg)
{
	const uint32_t frame_bytes = I2S_SILABS_SLOT_BYTES * (uint32_t)cfg->channels;

	/* A whole frame is also a whole number of LDMA beats (slots). */
	if (cfg->block_size == 0U || (cfg->block_size % frame_bytes) != 0U) {
		return -EINVAL;
	}
	if (cfg->block_size > I2S_SILABS_LDMA_MAX_BLOCK_BYTES) {
		return -EINVAL;
	}
	return 0;
}

static int i2s_silabs_usart_configure_apply_hw(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	int err;

	err = pinctrl_apply_state(pcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	err = clock_control_on(pcfg->clock_dev, (clock_control_subsys_t)&pcfg->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	i2s_silabs_usart_mclk_set(pcfg->mclk_dev, pcfg->mclk_subsys, true);

	err = i2s_silabs_usart_hw_apply(dev);
	if (err < 0) {
		i2s_silabs_usart_mclk_set(pcfg->mclk_dev, pcfg->mclk_subsys, false);
		return err;
	}

	return 0;
}

static int i2s_silabs_usart_configure(const struct device *dev, enum i2s_dir dir,
				      const struct i2s_config *cfg)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	int err = 0;

	k_mutex_lock(&data->cfg_lock, K_FOREVER);

	if (dir == I2S_DIR_BOTH) {
		err = -ENOSYS;
		goto out;
	}
	if (dir == I2S_DIR_RX) {
		err = -ENOTSUP;
		goto out;
	}
	if (dir != I2S_DIR_TX) {
		err = -EINVAL;
		goto out;
	}

	if (cfg == NULL || cfg->frame_clk_freq == 0U) {
		err = i2s_silabs_usart_power_down(dev);
		goto out;
	}

	err = i2s_silabs_usart_validate_i2s_cfg(cfg);
	if (err < 0) {
		goto out;
	}

	err = i2s_silabs_usart_validate_configure_block_size(cfg);
	if (err < 0) {
		goto out;
	}

	/* hw_apply() resets the USART. */
	if (!i2s_silabs_usart_cfg_state_allowed(data->tx.state)) {
		err = -EINVAL;
		goto out;
	}
	if (cfg->channels == 1U && !pcfg->has_tx_split) {
		err = -EINVAL;
		goto out;
	}

	memcpy(&data->tx.cfg, cfg, sizeof(*cfg));
	data->tx.cfg_valid = true;
	err = i2s_silabs_usart_configure_apply_hw(dev);
	data->tx.cfg_valid = (err == 0);
	data->tx.state = (err == 0) ? I2S_STATE_READY : I2S_STATE_NOT_READY;

out:
	k_mutex_unlock(&data->cfg_lock);
	return err;
}

static const struct i2s_config *i2s_silabs_usart_config_get(const struct device *dev,
							    enum i2s_dir dir)
{
	struct i2s_silabs_usart_data *data = dev->data;

	if (dir == I2S_DIR_TX && data->tx.cfg_valid) {
		return &data->tx.cfg;
	}
	return NULL;
}

static int i2s_silabs_usart_read(const struct device *dev, void **mem_block, size_t *size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mem_block);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

static int i2s_silabs_usart_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	uint32_t frame_bytes;
	int ret;
	k_spinlock_key_t key;

	if (!tx->cfg_valid) {
		return -EIO;
	}
	if (tx->state != I2S_STATE_READY && tx->state != I2S_STATE_RUNNING) {
		return -EIO;
	}

	/* Whole frames only; tx->cfg was validated by configure(). */
	frame_bytes = I2S_SILABS_SLOT_BYTES * (uint32_t)tx->cfg.channels;

	if (size == 0U || size > tx->cfg.block_size || (size % frame_bytes) != 0U) {
		return -EINVAL;
	}

	ret = k_sem_take(&tx->sem, SYS_TIMEOUT_MS(tx->cfg.timeout));
	if (ret < 0) {
		return ret;
	}

	key = k_spin_lock(&data->lock);
	ret = i2s_silabs_usart_block_q_put(&tx->q, mem_block, size);
	k_spin_unlock(&data->lock, key);

	if (ret < 0) {
		k_sem_give(&tx->sem);
		return -ENOMEM;
	}

	if (tx->state == I2S_STATE_RUNNING) {
		i2s_silabs_usart_tx_kick(dev);
	}

	return 0;
}

static int i2s_silabs_usart_trigger_start(const struct device *dev)
{
	struct i2s_silabs_usart_data *data = dev->data;
	k_spinlock_key_t key;

	if (!data->tx.cfg_valid || data->tx.state != I2S_STATE_READY) {
		return -EIO;
	}
	if (i2s_silabs_usart_tx_q_empty(&data->tx)) {
		return -EIO;
	}

	key = k_spin_lock(&data->lock);
	data->tx.drain = false;
	data->tx.state = I2S_STATE_RUNNING;
	k_spin_unlock(&data->lock, key);

	i2s_silabs_usart_tx_kick(dev);
	return (data->tx.state == I2S_STATE_RUNNING) ? 0 : -EIO;
}

static int i2s_silabs_usart_trigger_stop(const struct device *dev, bool drain)
{
	struct i2s_silabs_usart_data *data = dev->data;
	k_spinlock_key_t key;

	if (!data->tx.cfg_valid || data->tx.state != I2S_STATE_RUNNING) {
		return -EIO;
	}

	key = k_spin_lock(&data->lock);
	data->tx.drain = drain;
	data->tx.state = I2S_STATE_STOPPING;
	k_spin_unlock(&data->lock, key);

	/* DRAIN feeds the remaining queue; both settle to READY when done. */
	i2s_silabs_usart_tx_kick(dev);
	return 0;
}

/* DROP works in any configured state; PREPARE only recovers from ERROR. */
static int i2s_silabs_usart_trigger_drop(const struct device *dev, bool prepare)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;

	if (!data->tx.cfg_valid || (prepare && data->tx.state != I2S_STATE_ERROR)) {
		return -EIO;
	}
	i2s_silabs_usart_hw_disable_data_irqs(pcfg->base);
	i2s_silabs_usart_tx_drop(dev);
	return 0;
}

static int i2s_silabs_usart_trigger(const struct device *dev, enum i2s_dir dir,
				    enum i2s_trigger_cmd cmd)
{
	struct i2s_silabs_usart_data *data = dev->data;
	int err;

	if (dir == I2S_DIR_BOTH) {
		return -ENOSYS;
	}
	if (dir == I2S_DIR_RX) {
		return -ENOTSUP;
	}
	if (dir != I2S_DIR_TX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->cfg_lock, K_FOREVER);

	switch (cmd) {
	case I2S_TRIGGER_START:
		err = i2s_silabs_usart_trigger_start(dev);
		break;
	case I2S_TRIGGER_STOP:
		err = i2s_silabs_usart_trigger_stop(dev, false);
		break;
	case I2S_TRIGGER_DRAIN:
		err = i2s_silabs_usart_trigger_stop(dev, true);
		break;
	case I2S_TRIGGER_DROP:
		err = i2s_silabs_usart_trigger_drop(dev, false);
		break;
	case I2S_TRIGGER_PREPARE:
		err = i2s_silabs_usart_trigger_drop(dev, true);
		break;
	default:
		err = -EINVAL;
		break;
	}

	k_mutex_unlock(&data->cfg_lock);
	return err;
}

/*
 * Fixed part of an LDMA descriptor template, including the one-slot transfer
 * size. A NULL callback leaves the completion callback disabled.
 */
static void i2s_silabs_usart_dma_desc_init(const struct device *dev,
					   struct i2s_silabs_usart_dma_desc *desc,
					   uint32_t dma_slot, enum dma_channel_direction direction,
					   dma_callback_t callback)
{
	memset(desc, 0, sizeof(*desc));
	desc->dma_cfg.dma_slot = dma_slot;
	desc->dma_cfg.channel_direction = direction;
	desc->dma_cfg.channel_priority = I2S_DMA_CHANNEL_PRIORITY;
	desc->dma_cfg.head_block = &desc->blk_cfg;
	desc->dma_cfg.dest_data_size = I2S_SILABS_SLOT_BYTES;
	desc->dma_cfg.source_data_size = I2S_SILABS_SLOT_BYTES;
	desc->dma_cfg.source_burst_length = I2S_SILABS_SLOT_BYTES;
	desc->dma_cfg.dest_burst_length = I2S_SILABS_SLOT_BYTES;
	if (callback != NULL) {
		desc->dma_cfg.user_data = (void *)dev;
		desc->dma_cfg.dma_callback = callback;
		desc->dma_cfg.complete_callback_en = I2S_DMA_COMPLETE_CB_ENABLED;
	}
}

static int i2s_silabs_usart_init(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	USART_TypeDef *base = cfg->base;
	int err;

	k_mutex_init(&data->cfg_lock);
	k_msgq_init(&data->tx.q, data->tx.q_buf, sizeof(struct i2s_silabs_usart_block),
		    CONFIG_I2S_SILABS_USART_TX_BLOCK_COUNT);
	k_sem_init(&data->tx.sem, CONFIG_I2S_SILABS_USART_TX_BLOCK_COUNT,
		   CONFIG_I2S_SILABS_USART_TX_BLOCK_COUNT);

	if (!device_is_ready(cfg->dma_dev)) {
		return -ENODEV;
	}

	/*
	 * USART register access faults if the peripheral clock is
	 * still gated (see uart_silabs_init).
	 */
	err = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	if (cfg->has_tx_split) {
		data->tx_silence.channel = dma_request_channel(cfg->dma_dev, NULL);
		if (data->tx_silence.channel < 0) {
			return -ENODEV;
		}

		data->tx.dma.channel = dma_request_channel(cfg->dma_dev, NULL);
		if (data->tx.dma.channel < 0) {
			dma_release_channel(cfg->dma_dev, (uint32_t)data->tx_silence.channel);
			return -ENODEV;
		}
	} else {
		data->tx_silence.channel = -1;
		data->tx.dma.channel = dma_request_channel(cfg->dma_dev, NULL);
		if (data->tx.dma.channel < 0) {
			return -ENODEV;
		}
	}

	/*
	 * Descriptor templates are instance-static (cfg->tx_dma, tx_silence_dma),
	 * not driver data. Everything set here is fixed; the TX requests follow
	 * the stream configuration and the TX source follows each block.
	 */
	i2s_silabs_usart_dma_desc_init(dev, cfg->tx_dma, cfg->dma_txbl_slot, MEMORY_TO_PERIPHERAL,
				       i2s_silabs_usart_dma_tx_cb);
	/* TX DMA writes to TXDOUBLE: one access enqueues one I2S slot (16-bit frame). */
	cfg->tx_dma->blk_cfg.dest_address = (uintptr_t)&base->TXDOUBLE;
	cfg->tx_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	cfg->tx_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	if (cfg->has_tx_split) {
		/* Pad channel: repeats the zero word, no completion callback. */
		i2s_silabs_usart_dma_desc_init(dev, cfg->tx_silence_dma, cfg->dma_txbl_slot,
					       MEMORY_TO_PERIPHERAL, NULL);
		cfg->tx_silence_dma->blk_cfg.dest_address = (uintptr_t)&base->TXDOUBLE;
		cfg->tx_silence_dma->blk_cfg.source_address =
			(uintptr_t)&i2s_silabs_usart_silence_word;
		cfg->tx_silence_dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		cfg->tx_silence_dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	i2s_silabs_usart_hw_disable_data_irqs(base);
	i2s_silabs_usart_hw_disable_error_irqs(base);

	return 0;
}

static DEVICE_API(i2s, i2s_silabs_usart_driver_api) = {
	.configure = i2s_silabs_usart_configure,
	.config_get = i2s_silabs_usart_config_get,
	.read = i2s_silabs_usart_read,
	.write = i2s_silabs_usart_write,
	.trigger = i2s_silabs_usart_trigger,
};

/*
 * Optional codec MCLK from silabs,mclk-out: a clock-output@N child of
 * silabs,series-clock-output (parent device + child reg as subsystem).
 */
#define I2S_SILABS_USART_MCLK_INIT(idx)                                                            \
	COND_CODE_1(                                                                               \
		DT_INST_NODE_HAS_PROP(idx, silabs_mclk_out),                                       \
		(.mclk_dev = DEVICE_DT_GET(                                                        \
			 DT_PARENT(DT_INST_PHANDLE(idx, silabs_mclk_out))),                        \
		 .mclk_subsys = (clock_control_subsys_t)(uintptr_t)DT_REG_ADDR(                    \
			 DT_INST_PHANDLE(idx, silabs_mclk_out)),),                                 \
		(.mclk_dev = NULL, .mclk_subsys = NULL,))

#define I2S_SILABS_USART_DMA_STORAGE(idx)                                                          \
	static struct i2s_silabs_usart_dma_desc i2s_silabs_usart_tx_dma_##idx;                     \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, txblright),                                         \
		    (static struct i2s_silabs_usart_dma_desc                                       \
			     i2s_silabs_usart_tx_silence_dma_##idx;),                              \
		    ())

#define I2S_SILABS_USART_DMA_INIT(idx)                                                             \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(idx, txbl)),                            \
	.dma_txbl_slot = SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(idx, txbl, slot)),   \
	.dma_txblright_slot = COND_CODE_1(                                                         \
		DT_INST_DMAS_HAS_NAME(idx, txblright),                                             \
		(SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(idx, txblright, slot))),     \
		(0U)),                                                                             \
	.tx_dma = &i2s_silabs_usart_tx_dma_##idx,                                                  \
	.tx_silence_dma = COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, txblright),                       \
				      (&i2s_silabs_usart_tx_silence_dma_##idx),                    \
				      (NULL)),                                                     \
	.has_tx_split = DT_INST_DMAS_HAS_NAME(idx, txblright),

#define I2S_SILABS_USART_DEFINE(idx)                                                               \
	I2S_SILABS_USART_DMA_STORAGE(idx);                                                         \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static const struct i2s_silabs_usart_cfg i2s_silabs_usart_cfg_##idx = {                    \
		.base = (USART_TypeDef *)DT_INST_REG_ADDR(idx),                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.mono_tx_default = DT_INST_ENUM_IDX(idx, silabs_mono_tx_slot),                     \
		I2S_SILABS_USART_DMA_INIT(idx)                                                     \
		I2S_SILABS_USART_MCLK_INIT(idx)                                                    \
	};                                                                                         \
	static struct i2s_silabs_usart_data i2s_silabs_usart_data_##idx;                           \
	DEVICE_DT_INST_DEFINE(idx, i2s_silabs_usart_init, NULL, &i2s_silabs_usart_data_##idx,      \
			      &i2s_silabs_usart_cfg_##idx, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,  \
			      &i2s_silabs_usart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_SILABS_USART_DEFINE)
