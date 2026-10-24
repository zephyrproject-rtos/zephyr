/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * EFR32 USART I2S driver (stereo and mono TX).
 *
 * The hardware always emits two channel slots per LRCLK frame on the wire.
 *
 * USART DMA request naming (RM / SoC headers):
 *   TXBL      = Transmit Buffer Level (NOT "left"). Fires when the TX FIFO has
 *               space for another sample. With I2SCTRL.DMASPLIT=0 it requests
 *               one LDMA move for the next word(s) on the bus (stereo: L+R).
 *   TXBLRIGHT = Separate DMA request when DMASPLIT=1; targets the right I2S
 *               slot only. The "RIGHT" suffix is explicit; TXBL itself is not
 *               an abbreviation for "TX buffer left".
 *
 * Stereo (configure channels == 2, I2SCTRL.DMASPLIT = 0):
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
 * I2S slot needs USARTn_TXDOUBLE (see RM §22.3.2.6 / §22.3.2.19). Stereo
 * and mono TX both target TXDOUBLE; mono moves one slot per DMA trigger.
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
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

#include <sl_hal_usart.h>

#define DT_DRV_COMPAT silabs_usart_i2s

#define I2S_SILABS_SUPPORTED_OPTIONS                                                     \
	(I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONT \
	 | I2S_OPT_BIT_CLK_GATED)

#define I2S_SILABS_TX_BLOCK_Q_DEPTH CONFIG_I2S_SILABS_USART_TX_BLOCK_COUNT
#define I2S_SILABS_RX_BLOCK_Q_DEPTH CONFIG_I2S_SILABS_USART_RX_BLOCK_COUNT
#define I2S_SILABS_BLOCK_Q_MAX_DEPTH \
	MAX(I2S_SILABS_TX_BLOCK_Q_DEPTH, I2S_SILABS_RX_BLOCK_Q_DEPTH)

/* Stereo: every LRCLK frame carries L + R = 2 channel slots. */
#define I2S_STEREO_SLOTS_PER_FRAME 2U

/* USART I2S data word is 16 bits (SL_HAL_USART_I2S_FORMAT_W16D16). */
#define I2S_SILABS_WORD_SIZE_BITS  16U
#define I2S_SILABS_WORD_SIZE_BYTES 2U

/* Conversion from word_size (bits) to bytes per slot. */
#define I2S_BITS_PER_BYTE 8U

/*
 * Minimum sample rate accepted by configure().  Below 8 kHz the producer-side
 * timing tolerance shrinks and the LDMA TXBL trigger latency dominates audio
 * quality.  Most I2S codecs (e.g. TAS2505) also spec >= 8 kHz.
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

/* Bytes per LDMA trigger for a 16-bit I2S data word. */
#define I2S_DMA_DATA_SIZE I2S_SILABS_WORD_SIZE_BYTES

enum i2s_silabs_usart_mono_tx_slot {
	I2S_SILABS_USART_MONO_TX_SLOT_LEFT = 0,
	I2S_SILABS_USART_MONO_TX_SLOT_RIGHT = 1,
};

/* One queued I2S memory block (pointer + byte length). Stored by value in a
 * Zephyr k_msgq for ISR/thread-safe block handoff.
 */
struct i2s_silabs_usart_block {
	void *blk;
	size_t len;
};

struct i2s_silabs_usart_dma {
	const struct device *dma_dev;
	int channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	bool busy;
};

struct i2s_silabs_usart_stream {
	int32_t state;
	bool cfg_valid;
	/*
	 * When state is STOPPING: true = I2S_TRIGGER_DRAIN (empty the TX queue),
	 * false = I2S_TRIGGER_STOP (finish the current/pending block only).
	 */
	bool drain;
	struct i2s_config cfg;
	struct k_msgq q;
	char q_buf[I2S_SILABS_BLOCK_Q_MAX_DEPTH * sizeof(struct i2s_silabs_usart_block)];
	void *active;
	void *pending;
	struct i2s_silabs_usart_dma dma;
	struct k_sem sem;
};

struct i2s_silabs_usart_data {
	struct i2s_silabs_usart_stream tx;
	struct i2s_silabs_usart_stream rx;
	struct i2s_silabs_usart_dma tx_silence;
	int16_t silence_16;
	uint8_t mono_tx_slot;
	struct k_mutex cfg_lock;
};

struct i2s_silabs_usart_cfg {
	USART_TypeDef *base;
	const struct device *clock_dev;
	struct silabs_clock_control_cmu_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_connect)(const struct device *dev);
	const struct device *dma_dev;
	uint32_t dma_txbl_slot;      /* LDMA slot: USARTnTXBL (buffer level, not "left") */
	uint32_t dma_txblright_slot; /* LDMA slot: USARTnTXBLRIGHT (mono DMASPLIT only) */
	uint32_t dma_rx_slot;
	const struct device *mclk_dev; /* optional CMU CLKOUT providing codec MCLK */
	clock_control_subsys_t mclk_subsys; /* output index for series-clock-output */
	bool has_tx_split;
	uint8_t mono_tx_default;
};

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

static void i2s_silabs_usart_isr(const void *arg);
static void i2s_silabs_usart_tx_try_start(const struct device *dev);
static void i2s_silabs_usart_rx_try_start(const struct device *dev);
static inline bool tx_path_is_mono(const struct device *dev);

/*
 * TX buffers normally come from cfg.mem_slab (Zephyr i2s_write contract). Some
 * applications also pass flash / static pointers for zero-copy playback.
 * k_mem_slab_free() writes a free-list link into the block, so only free when
 * the pointer is actually inside the slab.
 */
static bool tx_blk_in_slab(struct k_mem_slab *slab, const void *blk)
{
	const char *p;
	ptrdiff_t offset;

	if ((slab == NULL) || (blk == NULL) || (slab->buffer == NULL)
		|| (slab->info.block_size == 0U)) {
		return false;
	}

	p = blk;
	offset = p - (const char *)slab->buffer;

	return (offset >= 0)
		&& (offset < (ptrdiff_t)(slab->info.block_size * slab->info.num_blocks))
			&& ((offset % (ptrdiff_t)slab->info.block_size) == 0);
}

static void tx_release_block(struct i2s_silabs_usart_stream *tx, void *blk)
{
	if (blk == NULL) {
		return;
	}

	if (tx_blk_in_slab(tx->cfg.mem_slab, blk)) {
		k_mem_slab_free(tx->cfg.mem_slab, blk);
	}
	k_sem_give(&tx->sem);
}

static void tx_finish_stopping_if_quiescent(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;

	if (tx->state != I2S_STATE_STOPPING) {
		return;
	}
	if (tx->active != NULL || tx->pending != NULL) {
		return;
	}
	/* DRAIN must empty the queue; STOP leaves remaining blocks for later START. */
	if (tx->drain && k_msgq_num_used_get(&tx->q) != 0U) {
		return;
	}
	/*
	 * With gapless LDMA chaining, tx->active may already be NULL while the
	 * channel is still marked busy until the final block completes.  Force
	 * the channel idle so STOPPING cannot wedge.
	 */
	if (tx->dma.busy) {
		dma_stop(tx->dma.dma_dev, (uint32_t)tx->dma.channel);
		tx->dma.busy = false;
	}
	if (tx_path_is_mono(dev) && data->tx_silence.busy) {
		dma_stop(cfg->dma_dev, (uint32_t)data->tx_silence.channel);
		data->tx_silence.busy = false;
	}
	tx->drain = false;
	tx->state = I2S_STATE_READY;
}

static inline int block_q_put(struct k_msgq *q, void *blk, size_t len)
{
	struct i2s_silabs_usart_block item = { .blk = blk, .len = len };

	return k_msgq_put(q, &item, K_NO_WAIT);
}

static inline int block_q_get(struct k_msgq *q, void **blk, size_t *len)
{
	struct i2s_silabs_usart_block item = { 0 };
	int ret = k_msgq_get(q, &item, K_NO_WAIT);

	if (ret == 0) {
		*blk = item.blk;
		*len = item.len;
	}
	return ret;
}

static inline void apply_dma_xfer_size(struct dma_config *dma_cfg)
{
	dma_cfg->dest_data_size = I2S_DMA_DATA_SIZE;
	dma_cfg->source_data_size = I2S_DMA_DATA_SIZE;
	dma_cfg->source_burst_length = I2S_DMA_DATA_SIZE;
	dma_cfg->dest_burst_length = I2S_DMA_DATA_SIZE;
}

static inline bool tx_cfg_is_mono(const struct i2s_config *cfg)
{
	return cfg->channels == 1U;
}

static inline bool tx_path_is_mono(const struct device *dev)
{
	const struct i2s_silabs_usart_data *data = dev->data;

	return data->tx.cfg_valid && tx_cfg_is_mono(&data->tx.cfg);
}

static uintptr_t tx_silence_src(const struct device *dev)
{
	const struct i2s_silabs_usart_data *data = dev->data;

	return (uintptr_t)&data->silence_16;
}

static void tx_dma_apply_stereo(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;

	apply_dma_xfer_size(&data->tx.dma.dma_cfg);
	/*
	 * DMASPLIT=0: USARTnTXBL (Transmit Buffer Level) triggers one LDMA move for
	 * interleaved L+R — not the left channel alone.
	 */
	data->tx.dma.dma_cfg.dma_slot = pcfg->dma_txbl_slot;
	data->tx.dma.blk_cfg.dest_address = (uintptr_t)&pcfg->base->TXDOUBLE;
	data->tx.dma.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->tx.dma.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
}

static void tx_dma_apply_mono(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	const bool audio_on_txbl = (data->mono_tx_slot == I2S_SILABS_USART_MONO_TX_SLOT_LEFT);
	const uint32_t audio_slot = audio_on_txbl ? pcfg->dma_txbl_slot : pcfg->dma_txblright_slot;
	const uint32_t pad_slot = audio_on_txbl ? pcfg->dma_txblright_slot : pcfg->dma_txbl_slot;

	apply_dma_xfer_size(&data->tx.dma.dma_cfg);
	apply_dma_xfer_size(&data->tx_silence.dma_cfg);

	data->tx.dma.dma_cfg.dma_slot = audio_slot;
	data->tx.dma.blk_cfg.dest_address = (uintptr_t)&pcfg->base->TXDOUBLE;
	data->tx.dma.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->tx.dma.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	data->tx_silence.dma_cfg.dma_slot = pad_slot;
	data->tx_silence.blk_cfg.dest_address = (uintptr_t)&pcfg->base->TXDOUBLE;
	data->tx_silence.blk_cfg.source_address = tx_silence_src(dev);
	data->tx_silence.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->tx_silence.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->tx_silence.blk_cfg.block_size = I2S_DMA_DATA_SIZE;
}

static void hw_disable_data_irqs(USART_TypeDef *base)
{
	sl_hal_usart_disable_interrupts(base, USART_IF_TXBL | USART_IF_RXDATAV);
	sl_hal_usart_clear_interrupts(base, USART_IF_TXBL | USART_IF_RXDATAV);
}

static void hw_disable_error_irqs(USART_TypeDef *base)
{
	sl_hal_usart_disable_interrupts(base, USART_IF_RXOF | USART_IF_TXUF);
	sl_hal_usart_clear_interrupts(base, USART_IF_RXOF | USART_IF_TXUF);
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
 * ref_hz or baud_hz is zero (no valid divider).
 */
static int i2s_silabs_usart_sync_clkdiv(uint32_t ref_hz, uint32_t baud_hz, uint32_t *clkdiv_out)
{
	uint64_t clkdiv;

	if (ref_hz == 0U || baud_hz == 0U) {
		return -EINVAL;
	}

	clkdiv = ((uint64_t)128U * ref_hz
		+ (uint64_t)baud_hz / 2U) / (uint64_t)baud_hz;

	if (clkdiv < 256U) {
		clkdiv = 256U;
	}
	clkdiv -= 256U;

#if defined(_USART_CLKDIV_DIVEXT_MASK)
	*clkdiv_out = (uint32_t)(clkdiv
		& (_USART_CLKDIV_DIV_MASK | _USART_CLKDIV_DIVEXT_MASK));
#else
	*clkdiv_out = (uint32_t)(clkdiv & _USART_CLKDIV_DIV_MASK);
#endif
	return 0;
}

static int validate_i2s_config(const struct device *dev,
	const struct i2s_config **out_ic,
	uint32_t *ref_hz)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	const struct i2s_silabs_usart_data *data = dev->data;
	const struct i2s_config *ic;
	int err;

	if (!data->tx.cfg_valid && !data->rx.cfg_valid) {
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

	ic = data->tx.cfg_valid ? &data->tx.cfg : &data->rx.cfg;

	if ((ic->options & I2S_SILABS_SUPPORTED_OPTIONS) != ic->options) {
		return -EINVAL;
	}
	if ((ic->options & I2S_OPT_BIT_CLK_TARGET) || (ic->options & I2S_OPT_FRAME_CLK_TARGET)) {
		return -EINVAL;
	}
	/* USART DATABITS max is 16; word_size is the data word, not the slot. */
	if (ic->word_size != I2S_SILABS_WORD_SIZE_BITS) {
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

	*out_ic = ic;
	return 0;
}

static int hw_i2s_apply(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	const struct i2s_config *ic;
	sl_hal_usart_i2s_init_t init = SL_HAL_USART_INIT_I2S_DEFAULT;
	uint32_t ref_hz = 0;
	uint32_t bit_hz;
	uint32_t clkdiv;
	uint32_t interrupt_enable;
	int err;

	err = validate_i2s_config(dev, &ic, &ref_hz);
	if (err < 0) {
		return err;
	}

	/*
	 * Required physical SCLK (bit clock):
	 *
	 *   bit_hz = frame_clk_freq * SLOTS_PER_FRAME * word_size
	 *
	 * SLOTS_PER_FRAME is hard-wired to 2 (stereo) -- every LRCLK period
	 * carries L + R, each `word_size` bits wide.
	 *
	 * Example: frame_clk=16 kHz, word_size=16  -> bit_hz = 16k*2*16 = 512 kHz.
	 *
	 * Converted to a CLKDIV via sl_hal_usart_sync_calculate_clock_div() so the
	 * USART produces SCLK == bit_hz on the wire.
	 */
	bit_hz = ic->frame_clk_freq * I2S_STEREO_SLOTS_PER_FRAME
		* (uint32_t)ic->word_size;

	apply_dma_xfer_size(&data->rx.dma.dma_cfg);

	if (data->tx.cfg_valid) {
		if (tx_cfg_is_mono(&data->tx.cfg)) {
			tx_dma_apply_mono(dev);
		} else {
			tx_dma_apply_stereo(dev);
		}
	}

	hw_disable_data_irqs(pcfg->base);

	init.sync.clock_div = sl_hal_usart_sync_calculate_clock_div(ref_hz, bit_hz);
	init.sync.data_bits = SL_HAL_USART_DATA_BITS_16;
	init.sync.master = true;
	init.sync.msb_first = true;
	init.sync.clock_mode =
		((ic->format & I2S_FMT_CLK_FORMAT_MASK) == I2S_FMT_CLK_NF_IB)
		|| ((ic->format & I2S_FMT_CLK_FORMAT_MASK) == I2S_FMT_CLK_IF_IB)
		? SL_HAL_USART_CLOCK_MODE_1
		: SL_HAL_USART_CLOCK_MODE_0;
	init.format = SL_HAL_USART_I2S_FORMAT_W16D16;
	init.justify = SL_HAL_USART_JUSTIFY_LEFT;
	/*
	 * Wire format stays stereo (MONO=0): LRCLK toggles L/R. Mono TX uses
	 * DMASPLIT + dual LDMA (samples on one slot, silence on the other).
	 */
	init.mono = false;
	init.dma_split = data->tx.cfg_valid && tx_cfg_is_mono(&data->tx.cfg);

	if ((ic->format & I2S_FMT_DATA_FORMAT_MASK) == I2S_FMT_DATA_FORMAT_I2S) {
		init.delay = true;
	} else {
		init.delay = false;
	}

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
	if (data->tx.cfg_valid) {
		sl_hal_usart_enable_tx(pcfg->base);
	}
	if (data->rx.cfg_valid) {
		sl_hal_usart_enable_rx(pcfg->base);
	}

	interrupt_enable = 0U;
	if (data->tx.cfg_valid) {
		interrupt_enable |= USART_IF_TXUF;
	}
	if (data->rx.cfg_valid) {
		interrupt_enable |= USART_IF_RXOF;
	}
	sl_hal_usart_enable_interrupts(pcfg->base, interrupt_enable);

	return 0;
}

static k_timeout_t cfg_timeout(int32_t t)
{
	if (t == SYS_FOREVER_MS) {
		return K_FOREVER;
	}
	if (t == 0) {
		return K_NO_WAIT;
	}
	return SYS_TIMEOUT_MS(t);
}

/*
 * Forward declarations: configure() in its "off" path (frame_clk_freq == 0)
 * tears down DMA / queues and physically gates the BCLK/LRCLK pins, both of
 * which are implemented further down in this file.
 */
static void tx_drop(const struct device *dev);
static void rx_drop(const struct device *dev);

/*
 * Stop / power-down: drop DMA, gate USART clocks, and disable codec MCLK.
 * Caller must hold data->cfg_lock.
 */
static int configure_power_down(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;

	hw_disable_data_irqs(pcfg->base);
	hw_disable_error_irqs(pcfg->base);

	if (data->tx.cfg_valid) {
		tx_drop(dev);
	}
	if (data->rx.cfg_valid) {
		rx_drop(dev);
	}

	sl_hal_usart_disable(pcfg->base);

	data->tx.cfg_valid = false;
	data->rx.cfg_valid = false;
	data->tx.state = I2S_STATE_NOT_READY;
	data->rx.state = I2S_STATE_NOT_READY;

	i2s_silabs_usart_mclk_set(pcfg->mclk_dev, pcfg->mclk_subsys, false);

	return 0;
}

static int validate_configure_block_size(const struct i2s_config *cfg)
{
	const uint32_t frame_bytes = ((uint32_t)cfg->word_size / I2S_BITS_PER_BYTE)
		* (uint32_t)cfg->channels;

	if (cfg->block_size == 0U || frame_bytes == 0U
		|| (cfg->block_size % frame_bytes) != 0U
			|| (cfg->block_size % I2S_DMA_DATA_SIZE) != 0U) {
		return -EINVAL;
	}
	return 0;
}

static bool i2s_cfg_params_mismatch(const struct i2s_config *a, const struct i2s_config *b)
{
	return (a->frame_clk_freq != b->frame_clk_freq)
		|| (a->word_size != b->word_size)
		|| (a->format != b->format)
		|| (a->channels != b->channels);
}

static bool tx_rx_cfg_mismatch(const struct device *dev, enum i2s_dir dir,
	const struct i2s_config *cfg)
{
	const struct i2s_silabs_usart_data *data = dev->data;

	if (dir == I2S_DIR_TX && data->rx.cfg_valid) {
		return i2s_cfg_params_mismatch(cfg, &data->rx.cfg);
	}
	if (dir == I2S_DIR_RX && data->tx.cfg_valid) {
		return i2s_cfg_params_mismatch(cfg, &data->tx.cfg);
	}

	return false;
}

static bool cfg_state_allowed(int32_t state)
{
	return (state == I2S_STATE_NOT_READY) || (state == I2S_STATE_READY);
}

static int i2s_silabs_usart_configure(const struct device *dev, enum i2s_dir dir,
	const struct i2s_config *cfg)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	int err = 0;

	k_mutex_lock(&data->cfg_lock, K_FOREVER);

	if ((dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) && !cfg_state_allowed(data->tx.state)) {
		err = -EINVAL;
		goto out;
	}
	if ((dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) && !cfg_state_allowed(data->rx.state)) {
		err = -EINVAL;
		goto out;
	}

	if (cfg == NULL || cfg->frame_clk_freq == 0U) {
		err = configure_power_down(dev);
		goto out;
	}

	if (cfg->word_size == 0U) {
		err = -EINVAL;
		goto out;
	}

	if (dir == I2S_DIR_RX && cfg->channels != 2U) {
		err = -EINVAL;
		goto out;
	}

	if (dir == I2S_DIR_TX && cfg->channels != 1U && cfg->channels != 2U) {
		err = -EINVAL;
		goto out;
	}

	if (dir == I2S_DIR_TX && cfg->channels == 1U && !pcfg->has_tx_split) {
		err = -EINVAL;
		goto out;
	}

	err = validate_configure_block_size(cfg);
	if (err < 0) {
		goto out;
	}

	if (dir == I2S_DIR_BOTH) {
		err = -ENOSYS;
		goto out;
	}

	if (tx_rx_cfg_mismatch(dev, dir, cfg)) {
		err = -EINVAL;
		goto out;
	}

	if (dir == I2S_DIR_TX) {
		memcpy(&data->tx.cfg, cfg, sizeof(*cfg));
		data->tx.cfg_valid = true;
		if (tx_cfg_is_mono(cfg)) {
			data->mono_tx_slot = pcfg->mono_tx_default;
		}
	} else {
		memcpy(&data->rx.cfg, cfg, sizeof(*cfg));
		data->rx.cfg_valid = true;
	}

	err = pinctrl_apply_state(pcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		goto out;
	}

	err = clock_control_on(pcfg->clock_dev, (clock_control_subsys_t)&pcfg->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		goto out;
	}
	err = 0;

	/* Enable codec MCLK before BCLK/LRCLK so the codec clock domain is ready. */
	i2s_silabs_usart_mclk_set(pcfg->mclk_dev, pcfg->mclk_subsys, true);

	err = hw_i2s_apply(dev);
	if (err < 0) {
		i2s_silabs_usart_mclk_set(pcfg->mclk_dev, pcfg->mclk_subsys, false);
		goto out;
	}

	data->tx.state = data->tx.cfg_valid ? I2S_STATE_READY : I2S_STATE_NOT_READY;
	data->rx.state = data->rx.cfg_valid ? I2S_STATE_READY : I2S_STATE_NOT_READY;

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
	if (dir == I2S_DIR_RX && data->rx.cfg_valid) {
		return &data->rx.cfg;
	}
	return NULL;
}

static void i2s_silabs_usart_dma_tx_cb(const struct device *dma_dev, void *user_data,
	uint32_t channel,
	int status)
{
	const struct device *dev = user_data;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	void *done;
	unsigned int key;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status < 0) {
		tx->state = I2S_STATE_ERROR;
		dma_stop(tx->dma.dma_dev, (uint32_t)tx->dma.channel);
		tx->dma.busy = false;
		if (tx_path_is_mono(dev) && data->tx_silence.busy) {
			dma_stop(data->tx_silence.dma_dev, (uint32_t)data->tx_silence.channel);
			data->tx_silence.busy = false;
		}
		if (tx->pending != NULL) {
			tx_release_block(tx, tx->pending);
			tx->pending = NULL;
		}
		if (tx->active != NULL) {
			tx_release_block(tx, tx->active);
			tx->active = NULL;
		}
		return;
	}

	key = irq_lock();
	done = tx->active;

	if (tx->pending != NULL) {
		/*
		 * silabs_ldma_append_block() chained the pending block: the
		 * LDMA has already auto-started it via LINKLOAD before this
		 * callback fires.  Do NOT call dma_stop() -- the channel is
		 * still running.
		 */
		tx->active = tx->pending;
		tx->pending = NULL;
	} else {
		/*
		 * No block was chained -- DMA is now idle.  Call dma_stop()
		 * to clear the LDMA driver's internal "busy" atomic so the
		 * next dma_config() does not fail with -EBUSY.
		 */
		dma_stop(tx->dma.dma_dev, (uint32_t)tx->dma.channel);
		tx->active = NULL;
		tx->dma.busy = false;
		if (tx_path_is_mono(dev) && data->tx_silence.busy) {
			dma_stop(data->tx_silence.dma_dev, (uint32_t)data->tx_silence.channel);
			data->tx_silence.busy = false;
		}
	}
	irq_unlock(key);

	if (done != NULL) {
		tx_release_block(tx, done);
	}

	/*
	 * STOPPING without drain (I2S_TRIGGER_STOP): do not pull more blocks.
	 * DRAIN / RUNNING continue via tx_try_start.
	 */
	if (tx->state == I2S_STATE_RUNNING || (tx->state == I2S_STATE_STOPPING && tx->drain)) {
		i2s_silabs_usart_tx_try_start(dev);
	}
	tx_finish_stopping_if_quiescent(dev);
}

static void i2s_silabs_usart_dma_rx_cb(const struct device *dma_dev, void *user_data,
	uint32_t channel,
	int status)
{
	const struct device *dev = user_data;
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	void *done;
	size_t done_len;
	unsigned int key;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status < 0) {
		data->rx.state = I2S_STATE_ERROR;
		dma_stop(cfg->dma_dev, (uint32_t)data->rx.dma.channel);
		done = data->rx.active;
		data->rx.active = NULL;
		data->rx.dma.busy = false;
		if (done != NULL) {
			k_mem_slab_free(data->rx.cfg.mem_slab, done);
		}
		return;
	}

	dma_stop(cfg->dma_dev, (uint32_t)data->rx.dma.channel);

	key = irq_lock();
	done = data->rx.active;
	done_len = data->rx.cfg.block_size;
	data->rx.active = NULL;
	data->rx.dma.busy = false;
	irq_unlock(key);

	if (done != NULL) {
		if (block_q_put(&data->rx.q, done, done_len) < 0) {
			k_mem_slab_free(data->rx.cfg.mem_slab, done);
			data->rx.state = I2S_STATE_ERROR;
		} else {
			k_sem_give(&data->rx.sem);
		}
	}

	if (data->rx.state == I2S_STATE_STOPPING) {
		data->rx.state = I2S_STATE_READY;
		return;
	}

	i2s_silabs_usart_rx_try_start(dev);
}

static int mono_tx_silence_run(const struct device *dev, size_t len, bool append)
{
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_dma *silence = &data->tx_silence;
	int ret;

	silence->blk_cfg.block_size = len;
	silence->blk_cfg.source_address = tx_silence_src(dev);

	if (append) {
		return silabs_ldma_append_block(silence->dma_dev, (uint32_t)silence->channel,
			&silence->dma_cfg);
	}

	ret = dma_config(silence->dma_dev, (uint32_t)silence->channel, &silence->dma_cfg);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(silence->dma_dev, (uint32_t)silence->channel);
	if (ret < 0) {
		dma_stop(silence->dma_dev, (uint32_t)silence->channel);
		return ret;
	}

	silence->busy = true;
	return 0;
}

static int dma_config_start(const struct device *dev, void *blk)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	int ret;

	ret = dma_config(tx->dma.dma_dev, (uint32_t)tx->dma.channel,
		&tx->dma.dma_cfg);
	if (ret < 0) {
		if (tx_path_is_mono(dev)) {
			dma_stop(data->tx_silence.dma_dev, (uint32_t)data->tx_silence.channel);
			data->tx_silence.busy = false;
		}
		tx->dma.busy = false;
		tx->active = NULL;
		tx_release_block(tx, blk);
		tx->state = I2S_STATE_ERROR;
		return -1;
	}

	ret = dma_start(tx->dma.dma_dev, (uint32_t)tx->dma.channel);
	if (ret < 0) {
		dma_stop(cfg->dma_dev, (uint32_t)tx->dma.channel);
		if (tx_path_is_mono(dev)) {
			dma_stop(data->tx_silence.dma_dev, (uint32_t)data->tx_silence.channel);
			data->tx_silence.busy = false;
		}
		tx->dma.busy = false;
		tx->active = NULL;
		tx_release_block(tx, blk);
		tx->state = I2S_STATE_ERROR;
		return -1;
	}

	return 0;
}

static void i2s_silabs_usart_tx_try_start(const struct device *dev)
{
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *tx = &data->tx;
	void *blk = NULL;
	size_t len = 0U;
	int ret;
	unsigned int key;

	if (tx->state == I2S_STATE_RUNNING) {
		/* continue */
	} else if (tx->state == I2S_STATE_STOPPING && tx->drain) {
		/* I2S_TRIGGER_DRAIN: keep feeding until the queue is empty */
	} else {
		return;
	}

	key = irq_lock();

	if (tx->active == NULL) {
		/*
		 * DMA is idle -- dequeue a block and perform a full dma_config() +
		 * dma_start() cycle.  Cold-start path used by I2S_TRIGGER_START and as
		 * fallback when the queue was empty.
		 */
		ret = block_q_get(&tx->q, &blk, &len);
		irq_unlock(key);

		if (ret < 0) {
			tx_finish_stopping_if_quiescent(dev);
			return;
		}

		tx->active = blk;
		tx->dma.busy = true;

		tx->dma.blk_cfg.source_address = (uintptr_t)blk;
		tx->dma.blk_cfg.block_size = len;

		if (tx_path_is_mono(dev)) {
			ret = mono_tx_silence_run(dev, len, false);
			if (ret < 0) {
				tx->dma.busy = false;
				tx->active = NULL;
				tx_release_block(tx, blk);
				tx->state = I2S_STATE_ERROR;
				return;
			}
		}

		ret = dma_config_start(dev, blk);
		if (ret < 0) {
			return;
		}

		/* Immediately try to double-buffer for gapless first transition */
		i2s_silabs_usart_tx_try_start(dev);
		return;
	}

	if (tx->pending != NULL) {
		irq_unlock(key);
		return;
	}

	/*
	 * DMA is running and no next block queued yet.  Pop the next block and
	 * use silabs_ldma_append_block() to chain it after the current transfer.
	 * The LDMA hardware will seamlessly transition via LINKLOAD, eliminating
	 * the gap that dma_stop/config/start would introduce.
	 */
	ret = block_q_get(&tx->q, &blk, &len);
	if (ret < 0) {
		irq_unlock(key);
		return;
	}

	tx->dma.blk_cfg.source_address = (uintptr_t)blk;
	tx->dma.blk_cfg.block_size = len;

	if (tx_path_is_mono(dev)) {
		ret = mono_tx_silence_run(dev, len, true);
		if (ret < 0) {
			irq_unlock(key);
			tx_release_block(tx, blk);
			return;
		}
	}

	ret = silabs_ldma_append_block(tx->dma.dma_dev, (uint32_t)tx->dma.channel,
		&tx->dma.dma_cfg);
	if (ret == 0) {
		tx->pending = blk;
	}
	irq_unlock(key);

	if (ret < 0) {
		tx_release_block(tx, blk);
	}
}

static void i2s_silabs_usart_rx_try_start(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	struct i2s_silabs_usart_stream *rx = &data->rx;
	int slab_ret;
	int ret;
	unsigned int key;

	if (rx->state != I2S_STATE_RUNNING) {
		return;
	}

	key = irq_lock();
	if (rx->dma.busy || rx->active != NULL) {
		irq_unlock(key);
		return;
	}
	irq_unlock(key);

	slab_ret = k_mem_slab_alloc(rx->cfg.mem_slab, &rx->active, K_NO_WAIT);
	if (slab_ret < 0) {
		rx->state = I2S_STATE_ERROR;
		return;
	}

	rx->dma.busy = true;
	rx->dma.blk_cfg.dest_address = (uintptr_t)rx->active;
	rx->dma.blk_cfg.block_size = rx->cfg.block_size;

	ret = dma_config(rx->dma.dma_dev, (uint32_t)rx->dma.channel, &rx->dma.dma_cfg);
	if (ret < 0) {
		rx->dma.busy = false;
		k_mem_slab_free(rx->cfg.mem_slab, rx->active);
		rx->active = NULL;
		rx->state = I2S_STATE_ERROR;
		return;
	}

	ret = dma_start(rx->dma.dma_dev, (uint32_t)rx->dma.channel);
	if (ret < 0) {
		dma_stop(cfg->dma_dev, (uint32_t)rx->dma.channel);
		rx->dma.busy = false;
		k_mem_slab_free(rx->cfg.mem_slab, rx->active);
		rx->active = NULL;
		rx->state = I2S_STATE_ERROR;
	}
}

static int i2s_silabs_usart_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct i2s_silabs_usart_data *data = dev->data;
	void *blk = NULL;
	size_t len = 0;
	int ret;
	unsigned int key;

	if (!data->rx.cfg_valid) {
		return -EIO;
	}

	ret = k_sem_take(&data->rx.sem, cfg_timeout(data->rx.cfg.timeout));
	if (ret < 0) {
		return ret;
	}

	key = irq_lock();

	ret = block_q_get(&data->rx.q, &blk, &len);
	irq_unlock(key);

	if (ret < 0) {
		return -EIO;
	}

	*mem_block = blk;
	*size = len;
	return 0;
}

static int i2s_silabs_usart_write(const struct device *dev, void *mem_block, size_t size)
{
	struct i2s_silabs_usart_data *data = dev->data;
	uint32_t tx_frame_bytes;
	int ret;
	unsigned int key;

	if (!data->tx.cfg_valid) {
		return -EIO;
	}
	if (data->tx.state != I2S_STATE_READY && data->tx.state != I2S_STATE_RUNNING) {
		return -EIO;
	}

	tx_frame_bytes = ((uint32_t)data->tx.cfg.word_size / I2S_BITS_PER_BYTE)
		* (uint32_t)data->tx.cfg.channels;

	if (size == 0U || size > data->tx.cfg.block_size || tx_frame_bytes == 0U
		|| (size % tx_frame_bytes) != 0U
			|| (size % I2S_DMA_DATA_SIZE) != 0U) {
		return -EINVAL;
	}

	ret = k_sem_take(&data->tx.sem, cfg_timeout(data->tx.cfg.timeout));
	if (ret < 0) {
		return ret;
	}

	key = irq_lock();

	ret = block_q_put(&data->tx.q, mem_block, size);
	irq_unlock(key);

	if (ret < 0) {
		k_sem_give(&data->tx.sem);
		return -ENOMEM;
	}

	if (data->tx.state == I2S_STATE_RUNNING) {
		i2s_silabs_usart_tx_try_start(dev);
	}

	return 0;
}

static void tx_drop(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	void *blk;
	size_t len;

	if (data->tx.dma.busy) {
		dma_stop(cfg->dma_dev, (uint32_t)data->tx.dma.channel);
		data->tx.dma.busy = false;
	}
	if (tx_path_is_mono(dev) && data->tx_silence.busy) {
		dma_stop(cfg->dma_dev, (uint32_t)data->tx_silence.channel);
		data->tx_silence.busy = false;
	}
	if (data->tx.pending != NULL) {
		tx_release_block(&data->tx, data->tx.pending);
		data->tx.pending = NULL;
	}
	if (data->tx.active != NULL) {
		tx_release_block(&data->tx, data->tx.active);
		data->tx.active = NULL;
	}

	while (block_q_get(&data->tx.q, &blk, &len) == 0) {
		tx_release_block(&data->tx, blk);
	}
	data->tx.drain = false;
}

static void rx_drop(const struct device *dev)
{
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	void *blk;
	size_t len;

	if (data->rx.dma.busy) {
		dma_stop(cfg->dma_dev, (uint32_t)data->rx.dma.channel);
		data->rx.dma.busy = false;
	}
	if (data->rx.active != NULL) {
		k_mem_slab_free(data->rx.cfg.mem_slab, data->rx.active);
		data->rx.active = NULL;
	}

	while (block_q_get(&data->rx.q, &blk, &len) == 0) {
		k_mem_slab_free(data->rx.cfg.mem_slab, blk);
	}
	while (k_sem_take(&data->rx.sem, K_NO_WAIT) == 0) {
	}
}

static int trigger_start(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_silabs_usart_data *data = dev->data;

	if (dir == I2S_DIR_TX && data->tx.cfg_valid) {
		if (data->tx.state != I2S_STATE_READY) {
			return -EINVAL;
		}
		data->tx.drain = false;
		data->tx.state = I2S_STATE_RUNNING;
		i2s_silabs_usart_tx_try_start(dev);
	}
	if (dir == I2S_DIR_RX && data->rx.cfg_valid) {
		if (data->rx.state != I2S_STATE_READY) {
			return -EINVAL;
		}
		data->rx.state = I2S_STATE_RUNNING;
		i2s_silabs_usart_rx_try_start(dev);
	}
	return 0;
}

static int trigger_stop(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_silabs_usart_data *data = dev->data;

	if (dir == I2S_DIR_TX && data->tx.cfg_valid) {
		if (data->tx.state != I2S_STATE_RUNNING) {
			return -EINVAL;
		}
		/*
		 * Finish the in-flight block (and any already-chained pending),
		 * then READY. Remaining queued blocks are kept for a later START.
		 */
		data->tx.drain = false;
		if (data->tx.active == NULL && data->tx.pending == NULL) {
			data->tx.state = I2S_STATE_READY;
		} else {
			data->tx.state = I2S_STATE_STOPPING;
			tx_finish_stopping_if_quiescent(dev);
		}
	}
	if (dir == I2S_DIR_RX && data->rx.cfg_valid) {
		if (data->rx.state != I2S_STATE_RUNNING) {
			return -EINVAL;
		}
		/*
		 * Finish the current RX DMA block, queue it for i2s_read(), then
		 * READY. Do not discard completed/queued blocks (that is DROP).
		 */
		if (data->rx.active == NULL && !data->rx.dma.busy) {
			data->rx.state = I2S_STATE_READY;
		} else {
			data->rx.state = I2S_STATE_STOPPING;
		}
	}
	return 0;
}

static int trigger_drain(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_silabs_usart_data *data = dev->data;

	if (dir == I2S_DIR_TX && data->tx.cfg_valid) {
		if (data->tx.state != I2S_STATE_RUNNING) {
			return -EINVAL;
		}
		data->tx.drain = true;
		data->tx.state = I2S_STATE_STOPPING;
		i2s_silabs_usart_tx_try_start(dev);
		tx_finish_stopping_if_quiescent(dev);
		return 0;
	}
	/* Per I2S API, DRAIN on RX has the same effect as STOP. */
	if (dir == I2S_DIR_RX && data->rx.cfg_valid) {
		return trigger_stop(dev, I2S_DIR_RX);
	}
	return -EINVAL;
}

static int trigger_drop(const struct device *dev, enum i2s_dir dir)
{
	const struct i2s_silabs_usart_cfg *pcfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;

	hw_disable_data_irqs(pcfg->base);
	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		tx_drop(dev);
		data->tx.state = data->tx.cfg_valid ? I2S_STATE_READY : I2S_STATE_NOT_READY;
	}
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		rx_drop(dev);
		data->rx.state = data->rx.cfg_valid ? I2S_STATE_READY : I2S_STATE_NOT_READY;
	}
	return 0;
}

static int trigger_prepare(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_silabs_usart_data *data = dev->data;

	if ((dir == I2S_DIR_TX || dir == I2S_DIR_BOTH)
		&& data->tx.state != I2S_STATE_ERROR) {
		return -EINVAL;
	}
	if ((dir == I2S_DIR_RX || dir == I2S_DIR_BOTH)
		&& data->rx.state != I2S_STATE_ERROR) {
		return -EINVAL;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		tx_drop(dev);
		data->tx.state = data->tx.cfg_valid ? I2S_STATE_READY : I2S_STATE_NOT_READY;
	}
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		rx_drop(dev);
		data->rx.state = data->rx.cfg_valid ? I2S_STATE_READY : I2S_STATE_NOT_READY;
	}

	return 0;
}

static int i2s_silabs_usart_trigger(const struct device *dev, enum i2s_dir dir,
	enum i2s_trigger_cmd cmd)
{
	struct i2s_silabs_usart_data *data = dev->data;
	int err = 0;

	k_mutex_lock(&data->cfg_lock, K_FOREVER);

	switch (cmd) {
	case I2S_TRIGGER_START:
		err = trigger_start(dev, dir);
		break;
	case I2S_TRIGGER_STOP:
		err = trigger_stop(dev, dir);
		break;
	case I2S_TRIGGER_DRAIN:
		err = trigger_drain(dev, dir);
		break;
	case I2S_TRIGGER_DROP:
		err = trigger_drop(dev, dir);
		break;
	case I2S_TRIGGER_PREPARE:
		err = trigger_prepare(dev, dir);
		break;
	default:
		err = -EINVAL;
		break;
	}

	k_mutex_unlock(&data->cfg_lock);
	return err;
}

static void i2s_silabs_usart_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct i2s_silabs_usart_cfg *cfg = dev->config;
	struct i2s_silabs_usart_data *data = dev->data;
	uint32_t flags = sl_hal_usart_get_pending_interrupts(cfg->base);

	if (flags & USART_IF_TXUF) {
		/*
		 * TX underflow at a block boundary is expected when the CPU
		 * cannot chain the next DMA descriptor in time.  With gapless
		 * DMA chaining this should rarely occur, but if it does, just
		 * clear the flag -- the DMA callback will still fire and can
		 * continue streaming.  Making this non-fatal prevents the
		 * stream from being killed at every block boundary at high
		 * sample rates (>= 16 kHz).
		 */
		sl_hal_usart_clear_interrupts(cfg->base, USART_IF_TXUF);
	}

	if (flags & USART_IF_RXOF) {
		data->rx.state = I2S_STATE_ERROR;
		dma_stop(cfg->dma_dev, (uint32_t)data->rx.dma.channel);
		data->rx.dma.busy = false;
		sl_hal_usart_clear_interrupts(cfg->base, USART_IF_RXOF);
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
		I2S_SILABS_TX_BLOCK_Q_DEPTH);
	k_msgq_init(&data->rx.q, data->rx.q_buf, sizeof(struct i2s_silabs_usart_block),
		I2S_SILABS_RX_BLOCK_Q_DEPTH);
	k_sem_init(&data->tx.sem, I2S_SILABS_TX_BLOCK_Q_DEPTH, I2S_SILABS_TX_BLOCK_Q_DEPTH);
	k_sem_init(&data->rx.sem, 0, I2S_SILABS_RX_BLOCK_Q_DEPTH);

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

	data->tx.dma.dma_dev = cfg->dma_dev;
	data->tx_silence.dma_dev = cfg->dma_dev;

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

	data->rx.dma.dma_dev = cfg->dma_dev;
	data->rx.dma.channel = dma_request_channel(cfg->dma_dev, NULL);
	if (data->rx.dma.channel < 0) {
		dma_release_channel(cfg->dma_dev, (uint32_t)data->tx.dma.channel);
		if (cfg->has_tx_split) {
			dma_release_channel(cfg->dma_dev, (uint32_t)data->tx_silence.channel);
		}
		return -ENODEV;
	}

	/*
	 * Provisional DMA defaults for 16-bit words. apply_dma_xfer_size()
	 * keeps these consistent when configure() runs.
	 */
	memset(&data->tx.dma.dma_cfg, 0, sizeof(data->tx.dma.dma_cfg));
	data->tx.dma.dma_cfg.dma_slot = cfg->dma_txbl_slot;
	data->tx.dma.dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	data->tx.dma.dma_cfg.source_data_size = I2S_DMA_DATA_SIZE;
	data->tx.dma.dma_cfg.dest_data_size = I2S_DMA_DATA_SIZE;
	data->tx.dma.dma_cfg.source_burst_length = I2S_DMA_DATA_SIZE;
	data->tx.dma.dma_cfg.dest_burst_length = I2S_DMA_DATA_SIZE;
	data->tx.dma.dma_cfg.head_block = &data->tx.dma.blk_cfg;
	data->tx.dma.dma_cfg.user_data = (void *)dev;
	data->tx.dma.dma_cfg.dma_callback = i2s_silabs_usart_dma_tx_cb;
	data->tx.dma.dma_cfg.complete_callback_en = I2S_DMA_COMPLETE_CB_ENABLED;
	data->tx.dma.dma_cfg.channel_priority = I2S_DMA_CHANNEL_PRIORITY;
	memset(&data->tx.dma.blk_cfg, 0, sizeof(data->tx.dma.blk_cfg));
	/* TX DMA writes to TXDOUBLE: a 16-bit access enqueues one I2S slot. */
	data->tx.dma.blk_cfg.dest_address = (uintptr_t)&base->TXDOUBLE;
	data->tx.dma.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->tx.dma.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	if (cfg->has_tx_split) {
		memset(&data->tx_silence.dma_cfg, 0, sizeof(data->tx_silence.dma_cfg));
		data->tx_silence.dma_cfg.dma_slot = cfg->dma_txbl_slot;
		data->tx_silence.dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		data->tx_silence.dma_cfg.source_data_size = I2S_DMA_DATA_SIZE;
		data->tx_silence.dma_cfg.dest_data_size = I2S_DMA_DATA_SIZE;
		data->tx_silence.dma_cfg.source_burst_length = I2S_DMA_DATA_SIZE;
		data->tx_silence.dma_cfg.dest_burst_length = I2S_DMA_DATA_SIZE;
		data->tx_silence.dma_cfg.head_block = &data->tx_silence.blk_cfg;
		data->tx_silence.dma_cfg.channel_priority = I2S_DMA_CHANNEL_PRIORITY;
		data->tx_silence.dma_cfg.complete_callback_en = 0U;
		memset(&data->tx_silence.blk_cfg, 0, sizeof(data->tx_silence.blk_cfg));
		data->tx_silence.blk_cfg.dest_address = (uintptr_t)&base->TXDOUBLE;
		data->tx_silence.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		data->tx_silence.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* See note above; mirror placeholder for RX. */
	memset(&data->rx.dma.dma_cfg, 0, sizeof(data->rx.dma.dma_cfg));
	data->rx.dma.dma_cfg.dma_slot = cfg->dma_rx_slot;
	data->rx.dma.dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	data->rx.dma.dma_cfg.source_data_size = I2S_DMA_DATA_SIZE;
	data->rx.dma.dma_cfg.dest_data_size = I2S_DMA_DATA_SIZE;
	data->rx.dma.dma_cfg.source_burst_length = I2S_DMA_DATA_SIZE;
	data->rx.dma.dma_cfg.dest_burst_length = I2S_DMA_DATA_SIZE;
	data->rx.dma.dma_cfg.head_block = &data->rx.dma.blk_cfg;
	data->rx.dma.dma_cfg.user_data = (void *)dev;
	data->rx.dma.dma_cfg.dma_callback = i2s_silabs_usart_dma_rx_cb;
	data->rx.dma.dma_cfg.complete_callback_en = I2S_DMA_COMPLETE_CB_ENABLED;
	data->rx.dma.dma_cfg.channel_priority = I2S_DMA_CHANNEL_PRIORITY;
	memset(&data->rx.dma.blk_cfg, 0, sizeof(data->rx.dma.blk_cfg));
	data->rx.dma.blk_cfg.source_address = (uintptr_t)&base->RXDOUBLE;
	data->rx.dma.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->rx.dma.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	hw_disable_data_irqs(base);
	hw_disable_error_irqs(base);
	sl_hal_usart_enable_interrupts(base, USART_IF_RXOF | USART_IF_TXUF);

	cfg->irq_connect(dev);
	return 0;
}

static DEVICE_API(i2s, i2s_silabs_usart_driver_api) = {
	.configure = i2s_silabs_usart_configure,
	.config_get = i2s_silabs_usart_config_get,
	.read = i2s_silabs_usart_read,
	.write = i2s_silabs_usart_write,
	.trigger = i2s_silabs_usart_trigger,
};

#define I2S_SILABS_USART_IRQ_CONNECT(idx)                                                  \
	static void i2s_silabs_usart_irq_connect_##idx(const struct device *dev)               \
	{                                                                                      \
	ARG_UNUSED(dev);                                                                       \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq), DT_INST_IRQ_BY_NAME(idx, rx, priority), \
	i2s_silabs_usart_isr, DEVICE_DT_INST_GET(idx), 0);                                \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq), DT_INST_IRQ_BY_NAME(idx, tx, priority), \
	i2s_silabs_usart_isr, DEVICE_DT_INST_GET(idx), 0);                                \
	irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));                                         \
	irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));                                         \
	}

/*
 * silabs,mclk-out = <&clkoutN> under silabs,series-clock-output → parent
 * device + child reg as subsystem.
 */
#define I2S_SILABS_USART_MCLK_NODE(idx) DT_INST_PHANDLE(idx, silabs_mclk_out)

#define I2S_SILABS_USART_MCLK_DEV(idx)                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, silabs_mclk_out),                               \
	(DEVICE_DT_GET(DT_PARENT(I2S_SILABS_USART_MCLK_NODE(idx)))),                   \
	(NULL))

#define I2S_SILABS_USART_MCLK_SUBSYS(idx)                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, silabs_mclk_out),                               \
		((clock_control_subsys_t)(uintptr_t)DT_REG_ADDR(                               \
		I2S_SILABS_USART_MCLK_NODE(idx))),                                     \
		(NULL))

#define I2S_SILABS_USART_HAS_TXBL_DMA(idx) \
	(DT_INST_DMAS_HAS_NAME(idx, txbl) || DT_INST_DMAS_HAS_NAME(idx, tx))

#define I2S_SILABS_USART_HAS_TXBLRIGHT_DMA(idx) \
	(DT_INST_DMAS_HAS_NAME(idx, txblright) || DT_INST_DMAS_HAS_NAME(idx, tx_right))

#define I2S_SILABS_USART_HAS_TX_SPLIT(idx) \
	(I2S_SILABS_USART_HAS_TXBL_DMA(idx) && I2S_SILABS_USART_HAS_TXBLRIGHT_DMA(idx))

#define I2S_SILABS_USART_TXBL_DMA_NAME(idx) \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, txbl), (txbl), (tx))

#define I2S_SILABS_USART_TXBLRIGHT_DMA_NAME(idx) \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, txblright), (txblright), (tx_right))

#define I2S_SILABS_USART_DMA_TXBL_SLOT(idx)                   \
	SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(idx, \
		I2S_SILABS_USART_TXBL_DMA_NAME(idx), \
		slot))

#define I2S_SILABS_USART_DMA_TXBLRIGHT_SLOT(idx)                                           \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, txblright),                                     \
		(SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(idx, txblright, slot))), \
		(COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, tx_right),                             \
		(SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(            \
		idx, tx_right, slot))),             \
		(0U))))

#define I2S_SILABS_USART_MONO_TX_DEFAULT(idx)                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, silabs_mono_tx_slot), \
			(DT_INST_ENUM_IDX(idx, silabs_mono_tx_slot)), \
			(I2S_SILABS_USART_MONO_TX_SLOT_RIGHT))

#define I2S_SILABS_USART_DEFINE(idx)                                             \
	I2S_SILABS_USART_IRQ_CONNECT(idx);                                           \
	PINCTRL_DT_INST_DEFINE(idx);                                                 \
	static const struct i2s_silabs_usart_cfg i2s_silabs_usart_cfg_##idx = {      \
		.base = (USART_TypeDef *)DT_INST_REG_ADDR(idx),                          \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                    \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                             \
		.irq_connect = i2s_silabs_usart_irq_connect_##idx,                       \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(idx, \
			I2S_SILABS_USART_TXBL_DMA_NAME(idx))), \
		.dma_txbl_slot = I2S_SILABS_USART_DMA_TXBL_SLOT(idx), \
		.dma_txblright_slot = I2S_SILABS_USART_DMA_TXBLRIGHT_SLOT(idx), \
		.dma_rx_slot = SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(idx, \
			rx, slot)), \
		.mclk_dev = I2S_SILABS_USART_MCLK_DEV(idx),  \
		.mclk_subsys = I2S_SILABS_USART_MCLK_SUBSYS(idx), \
		.has_tx_split = I2S_SILABS_USART_HAS_TX_SPLIT(idx), \
		.mono_tx_default = I2S_SILABS_USART_MONO_TX_DEFAULT(idx), \
	};                                                            \
	static struct i2s_silabs_usart_data i2s_silabs_usart_data_##idx; \
	DEVICE_DT_INST_DEFINE(idx, i2s_silabs_usart_init, NULL, &i2s_silabs_usart_data_##idx, \
		&i2s_silabs_usart_cfg_##idx, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,        \
		&i2s_silabs_usart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2S_SILABS_USART_DEFINE)
