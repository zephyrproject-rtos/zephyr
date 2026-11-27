/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2S driver for the TI Multichannel Audio Serial Port (McASP), I2S/TDM
 * mode, with data moved by the CPU: through the audio FIFO and the DATA
 * port from a start-of-frame interrupt (CONFIG_I2S_TI_MCASP_AFIFO), or one
 * word per data-ready interrupt through the configuration port
 * (XBUFn/RBUFn). Written from the AM62L TRM (SPRUJB4A, chapter 12.1.1 and
 * register map 14.7.1.1).
 *
 * Transmit and receive run synchronously (ACLKXCTL.ASYNC = 0) from the
 * transmit clock and frame sync generators, which every board using this
 * driver must wire (ACLKX, AFSX). The hardware runs while either direction
 * is active; an idle transmitter sends zeros and an idle receiver discards
 * its words, so each direction can start and stop on a frame boundary
 * without disturbing the other one. Both interrupt handlers count slots,
 * so a block always begins with slot 0 of a frame.
 */

#define DT_DRV_COMPAT ti_am33xx_mcasp_audio

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>
#ifdef CONFIG_I2S_TI_MCASP_DMA
#include <zephyr/drivers/dma.h>
#endif
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2s_ti_mcasp, CONFIG_I2S_LOG_LEVEL);

/* Registers (TRM Table 14-22813) */
#define MCASP_PID              0x000U
#define MCASP_PWRIDLESYSCONFIG 0x004U
#define MCASP_PFUNC            0x010U
#define MCASP_PDIR             0x014U
#define MCASP_GBLCTL           0x044U
#define MCASP_DLBCTL           0x04cU
#define MCASP_DITCTL           0x050U
#define MCASP_RMASK            0x064U
#define MCASP_RFMT             0x068U
#define MCASP_AFSRCTL          0x06cU
#define MCASP_RTDM             0x078U
#define MCASP_RINTCTL          0x07cU
#define MCASP_RSTAT            0x080U
#define MCASP_REVTCTL          0x08cU /* PIDTCTL in the register map */
#define MCASP_XMASK            0x0a4U
#define MCASP_XFMT             0x0a8U
#define MCASP_AFSXCTL          0x0acU
#define MCASP_ACLKXCTL         0x0b0U
#define MCASP_AHCLKXCTL        0x0b4U
#define MCASP_XTDM             0x0b8U
#define MCASP_XINTCTL          0x0bcU
#define MCASP_XSTAT            0x0c0U
#define MCASP_XEVTCTL          0x0ccU
#define MCASP_SRCTL(n)         (0x180 + 4 * (n))
#define MCASP_XBUF(n)          (0x200 + 4 * (n))
#define MCASP_RBUF(n)          (0x280 + 4 * (n))
#define MCASP_WFIFOCTL         0x1000U
#define MCASP_WFIFOSTS         0x1004U
#define MCASP_RFIFOCTL         0x1008U
#define MCASP_RFIFOSTS         0x100cU

#define PWRIDLE_NO_IDLE 1U

#define PDIR_ACLKX BIT(26)
#define PDIR_AFSX  BIT(28)

#define GBLCTL_RCLKRST  BIT(0)
#define GBLCTL_RHCLKRST BIT(1)
#define GBLCTL_RSRCLR   BIT(2)
#define GBLCTL_RSMRST   BIT(3)
#define GBLCTL_RFRST    BIT(4)
#define GBLCTL_XCLKRST  BIT(8)
#define GBLCTL_XHCLKRST BIT(9)
#define GBLCTL_XSRCLR   BIT(10)
#define GBLCTL_XSMRST   BIT(11)
#define GBLCTL_XFRST    BIT(12)

#define DLBCTL_DLBEN     BIT(0)
#define DLBCTL_ORD       BIT(1)
#define DLBCTL_MODE_XCLK (1U << 2)

#define FMT_ROT(x)    ((x) & 0x7U)
#define FMT_BUSEL_CFG BIT(3) /* 0: the DATA port feeds the serializers */
#define FMT_SSZ(x)    (((x) & 0xfU) << 4)
#define FMT_PBIT(x)   (((x) & 0x1fU) << 8)
#define FMT_PAD_PBIT  (2U << 13)
#define FMT_RVRS      BIT(15)
#define FMT_DATDLY(x) (((x) & 0x3U) << 16)

#define AFSCTL_FSP    BIT(0)
#define AFSCTL_FSM    BIT(1)
#define AFSCTL_FWID   BIT(4)
#define AFSCTL_MOD(x) (((x) & 0x1ffU) << 7)

#define ACLKXCTL_DIV(x) (((x) - 1U) & 0x1fU)
#define ACLKXCTL_CLKXM  BIT(5)
#define ACLKXCTL_CLKXP  BIT(7)
#define CLKXDIV_MAX     32U

#define AHCLKXCTL_DIV(x) (((x) - 1U) & 0xfffU)
#define AHCLKXCTL_HCLKXM BIT(15)
#define HCLKXDIV_MAX     4096U

#define INT_UNDRN_OVRN  BIT(0)
#define INT_SYNCERR     BIT(1)
#define INT_DMAERR      BIT(3)
#define INT_DATA        BIT(5)
#define INT_STAFRM      BIT(7)
#define STAT_UNDRN_OVRN BIT(0)
#define STAT_SYNCERR    BIT(1)
#define STAT_CKFAIL     BIT(2)
#define STAT_DATA       BIT(5)
#define STAT_STAFRM     BIT(6)
#define STAT_DMAERR     BIT(7)
#define STAT_W1C        0xf7U
#define STAT_ERRORS     (STAT_UNDRN_OVRN | STAT_SYNCERR | STAT_DMAERR)

/* XEVTCTL.XDATDMA / REVTCTL.RDATDMA: 1 disables DMA requests (TRM Table 12-29) */
#define EVTCTL_DMA_OFF BIT(0)

/*
 * WFIFOCTL / RFIFOCTL (TRM 12.1.1.4.11, 14.7.1.1.2.108-111): 64 words each.
 * NUMDMA words move per McASP request and must equal the serializer count.
 */
#define FIFOCTL_NUMDMA(x) ((x) & 0xffU)
#define FIFOCTL_NUMEVT(x) (((x) & 0xffU) << 8)
#define FIFOCTL_ENA       BIT(16)
#define FIFOSTS_LVL_MASK  0xffU
#define AFIFO_WORDS       64U
#define AFIFO_FRAME_MAX   (AFIFO_WORDS / 2U)

#define SRCTL_SRMOD_TX  1U
#define SRCTL_SRMOD_RX  2U
#define SRCTL_DISMOD_LO (2U << 2)
#define SRCTL_XRDY      BIT(4)
#define SRCTL_RRDY      BIT(5)

#define MCASP_MAX_SER       16U
#define MCASP_MAX_SLOTS     32U
#define MCASP_POLL_US       1000U
#define MCASP_SERDIR_TX     1U
#define MCASP_SERDIR_RX     2U
/* Zero slots sent after the last word before the hardware stops */
#define MCASP_TX_TAIL_SLOTS 2U
#define MCASP_DMA_DEPTH      2U
#define MCASP_DMA_DEPTH_TX   1U
#define MCASP_DMA_TX_LEAD_US 250U
#define MCASP_DMA_BCNT_MASK  GENMASK(30, 0)
#define MCASP_DMA_ALIGN      64U
#define MCASP_DMA_ASEL_ACP0  14U
#define MCASP_DMA_ASEL_ACP1  15U
#define MCASP_DMA_LEAD_FRAMES_MIN 2U

struct mcasp_block {
	void *buf;
	size_t size;
};

struct mcasp_stream {
	struct i2s_config cfg;
	enum i2s_state state;
	struct k_msgq *msgq;
	/* Block in progress; owned by the interrupt handler while active */
	void *buf;
	size_t size;
	size_t pos;
	/* Slot of the next word the interrupt handler moves */
	uint8_t slot;
	/* STOP: end at the end of the current block */
	bool stop;
	/* TX DRAIN: end when the queue is empty */
	bool drain;
	/* RX of a DIR_BOTH DRAIN: end with the block holding the last TX slot */
	bool follow_tx;
	bool dma;
	bool dma_used;
	uint8_t dma_head;
	uint8_t dma_n;
	uint32_t dma_sub;
	struct mcasp_block dma_q[MCASP_DMA_DEPTH];
};

struct mcasp_config {
	DEVICE_MMIO_ROM;
	/* DATA port: the only CPU access to the AFIFO (TRM 12.1.1.4.10.1.3) */
	uintptr_t dat_phys;
	size_t dat_size;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(void);
	const uint8_t *serial_dir;
	uint8_t serial_dir_len;
#ifdef CONFIG_I2S_TI_MCASP_DMA
	const struct device *dma_dev;
	uint32_t dma_ch[2];
	uint8_t dma_asel[2];
#endif
};

struct mcasp_data {
	DEVICE_MMIO_RAM;
	mm_reg_t dat;
	struct k_spinlock lock;
	struct mcasp_stream tx;
	struct mcasp_stream rx;
	uint8_t tx_ser[MCASP_MAX_SER];
	uint8_t rx_ser[MCASP_MAX_SER];
	uint8_t num_tx;
	uint8_t num_rx;
	bool hw_running;
	uint8_t tail;
	/*
	 * Slots moved since the hardware start, and the TX count after the last
	 * slot taken from a block. TX runs up to a FIFO ahead of RX, so a
	 * DIR_BOTH DRAIN ends RX by count, not when TX ends.
	 */
	uint64_t tx_slots;
	uint64_t tx_data_end;
	uint64_t rx_slots;
	uint32_t gblctl;
	/* Frame layout latched at hardware start */
	uint8_t slots;
	uint8_t wsize;
	uint32_t frame_hz;
	uint32_t fck_rate;
	bool dma_ok;
};

static inline uint32_t mcasp_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void mcasp_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

static inline bool use_fifo(void)
{
	return IS_ENABLED(CONFIG_I2S_TI_MCASP_AFIFO);
}

static inline bool use_dma(const struct device *dev)
{
	const struct mcasp_data *data = dev->data;

	return IS_ENABLED(CONFIG_I2S_TI_MCASP_DMA) && data->dma_ok;
}

/*
 * One word for the i-th serializer of the slot. The DATA port takes the
 * words of all serializers at one address, in serializer order (TRM
 * 12.1.1.4.10.1.3).
 */
static inline void tx_put(const struct device *dev, uint8_t i, uint32_t w)
{
	struct mcasp_data *data = dev->data;

	if (use_fifo()) {
		sys_write32(w, data->dat);
	} else {
		mcasp_write(dev, MCASP_XBUF(data->tx_ser[i]), w);
	}
}

static inline uint32_t rx_get(const struct device *dev, uint8_t i)
{
	struct mcasp_data *data = dev->data;

	if (use_fifo()) {
		return sys_read32(data->dat);
	}

	return mcasp_read(dev, MCASP_RBUF(data->rx_ser[i]));
}

static inline uint32_t fifo_level(const struct device *dev, uint32_t sts)
{
	return mcasp_read(dev, sts) & FIFOSTS_LVL_MASK;
}

static inline bool stream_active(const struct mcasp_stream *s)
{
	return s->state == I2S_STATE_RUNNING || s->state == I2S_STATE_STOPPING;
}

static uint8_t word_bytes(uint8_t word_size)
{
	if (word_size <= 8U) {
		return 1U;
	}
	if (word_size <= 16U) {
		return 2U;
	}
	return 4U;
}

/* The shared clock and frame layout follow the active streams, TX first */
static const struct i2s_config *clock_cfg(const struct mcasp_data *data)
{
	return stream_active(&data->tx) ? &data->tx.cfg : &data->rx.cfg;
}

static bool loopback_enabled(const struct mcasp_data *data)
{
	return (stream_active(&data->tx) && (data->tx.cfg.options & I2S_OPT_LOOPBACK) != 0U) ||
	       (stream_active(&data->rx) && (data->rx.cfg.options & I2S_OPT_LOOPBACK) != 0U);
}

/* One clock and frame sync serve both directions: same rate, word, format, slots */
static bool cfg_compatible(const struct mcasp_data *data, const struct i2s_config *tx,
			   const struct i2s_config *rx)
{
	return tx->word_size == rx->word_size && tx->frame_clk_freq == rx->frame_clk_freq &&
	       tx->format == rx->format &&
	       tx->channels / data->num_tx == rx->channels / data->num_rx;
}

/*
 * Loopback connects serializer pairs (TRM 12.1.1.4.14): ORD = 1 sends even
 * serializers to odd ones, ORD = 0 odd to even. Returns the DLBCTL value
 * for the serial-dir layout, or -EINVAL if it pairs neither way.
 */
static int loopback_dlbctl(const struct device *dev)
{
	const struct mcasp_config *cfg = dev->config;
	struct mcasp_data *data = dev->data;

	for (uint32_t ord = 0U; ord <= 1U; ord++) {
		bool ok = data->num_tx > 0U && data->num_tx == data->num_rx;

		for (uint8_t i = 0U; ok && i < data->num_tx; i++) {
			uint8_t t = data->tx_ser[i];
			uint32_t r = ord ? t + 1U : t - 1U;

			ok = ((t & 1U) == (ord ? 0U : 1U)) && r < cfg->serial_dir_len &&
			     cfg->serial_dir[r] == MCASP_SERDIR_RX;
		}
		if (ok) {
			return DLBCTL_DLBEN | DLBCTL_MODE_XCLK | (ord ? DLBCTL_ORD : 0U);
		}
	}

	return -EINVAL;
}

/* Bit clock dividers from AUXCLK: HCLKXDIV (1..4096) x CLKXDIV (1..32) */
static int clock_dividers(uint32_t fck, uint32_t bclk, uint32_t *hdiv, uint32_t *cdiv)
{
	uint64_t best_err = UINT64_MAX;

	if (fck == 0U || bclk == 0U || bclk > fck) {
		return -EINVAL;
	}

	for (uint32_t c = 1U; c <= CLKXDIV_MAX; c++) {
		uint64_t h = ((uint64_t)fck + (uint64_t)bclk * c / 2U) / ((uint64_t)bclk * c);
		uint64_t rate, err;

		if (h < 1U || h > HCLKXDIV_MAX) {
			continue;
		}
		rate = fck / (h * c);
		err = rate > bclk ? rate - bclk : bclk - rate;
		if (err < best_err) {
			best_err = err;
			*hdiv = (uint32_t)h;
			*cdiv = c;
		}
	}

	return best_err == UINT64_MAX ? -EINVAL : 0;
}

/* Write GBLCTL and wait until the value is latched (TRM Table 12-18) */
static int gblctl_write(const struct device *dev, uint32_t val)
{
	struct mcasp_data *data = dev->data;

	data->gblctl = val;
	mcasp_write(dev, MCASP_GBLCTL, val);
	if (WAIT_FOR(mcasp_read(dev, MCASP_GBLCTL) == val, MCASP_POLL_US, k_busy_wait(1))) {
		return 0;
	}

	LOG_ERR("GBLCTL 0x%x not latched (clock missing?)", val);
	return -EIO;
}

static int gblctl_set(const struct device *dev, uint32_t bits)
{
	struct mcasp_data *data = dev->data;

	return gblctl_write(dev, data->gblctl | bits);
}

#ifdef CONFIG_I2S_TI_MCASP_DMA
static inline bool dma_is_tx(const struct device *dev, const struct mcasp_stream *s)
{
	const struct mcasp_data *data = dev->data;

	return s == &data->tx;
}

static void dma_handover(const struct device *dev, struct mcasp_stream *s)
{
	struct mcasp_data *data = dev->data;
	uint32_t reg = dma_is_tx(dev, s) ? MCASP_XINTCTL : MCASP_RINTCTL;

	s->dma = false;
	s->slot = 0U;
	if (data->hw_running) {
		mcasp_write(dev, reg, mcasp_read(dev, reg) | INT_STAFRM);
	}
}

static void dma_abort(const struct device *dev, struct mcasp_stream *s)
{
	const struct mcasp_config *cfg = dev->config;
	bool tx = dma_is_tx(dev, s);
	bool stopped = true;
	int ret;

	if (s->dma_used) {
		ret = dma_stop(cfg->dma_dev, cfg->dma_ch[tx ? 0 : 1]);
		if (ret < 0) {
			LOG_ERR("%s DMA stop: %d", tx ? "TX" : "RX", ret);
			stopped = false;
		}
		s->dma_used = false;
	}
	if (!stopped && s->dma_n > 0U) {
		LOG_ERR("%s DMA may still run: %u block(s) are not returned to the slab",
			tx ? "TX" : "RX", (unsigned int)s->dma_n);
	}
	while (s->dma_n > 0U) {
		if (stopped) {
			k_mem_slab_free(s->cfg.mem_slab, s->dma_q[s->dma_head].buf);
		}
		s->dma_head = (s->dma_head + 1U) % MCASP_DMA_DEPTH;
		s->dma_n--;
	}
	if (s->dma) {
		dma_handover(dev, s);
	}
}
#else
static inline void dma_abort(const struct device *dev, struct mcasp_stream *s)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(s);
}
#endif

/* Lock held */
static void hw_stop(const struct device *dev)
{
	struct mcasp_data *data = dev->data;

	dma_abort(dev, &data->tx);
	dma_abort(dev, &data->rx);

	mcasp_write(dev, MCASP_XINTCTL, 0U);
	mcasp_write(dev, MCASP_RINTCTL, 0U);
	mcasp_write(dev, MCASP_GBLCTL, 0U);
	data->gblctl = 0U;
	/* Disabling flushes the FIFOs (TRM WFIFOCTL.WENA, RFIFOCTL.RENA) */
	mcasp_write(dev, MCASP_WFIFOCTL, 0U);
	mcasp_write(dev, MCASP_RFIFOCTL, 0U);
	/* Dividers in reset pass AUXCLK through: stop driving the pins */
	mcasp_write(dev, MCASP_PDIR, 0U);
	mcasp_write(dev, MCASP_XSTAT, STAT_W1C);
	mcasp_write(dev, MCASP_RSTAT, STAT_W1C);
	data->hw_running = false;
	data->tail = 0U;
}

/* Error on the serial port: stop everything, report on active streams; lock held */
static void hw_error(const struct device *dev, bool tx_side, uint32_t stat)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *streams[] = {&data->tx, &data->rx};

	LOG_ERR("%s status 0x%x, stopping", tx_side ? "XSTAT" : "RSTAT", stat);
	hw_stop(dev);

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct mcasp_stream *s = streams[i];

		if (stream_active(s)) {
			s->state = I2S_STATE_ERROR;
		}
		if (s->buf != NULL) {
			k_mem_slab_free(s->cfg.mem_slab, s->buf);
			s->buf = NULL;
		}
	}
}

static inline uint32_t load_word(const void *buf, size_t pos, uint8_t wsize)
{
	const uint8_t *p = (const uint8_t *)buf + pos;

	switch (wsize) {
	case 1:
		return *p;
	case 2:
		return *(const uint16_t *)p;
	default:
		return *(const uint32_t *)p;
	}
}

static inline void store_word(void *buf, size_t pos, uint8_t wsize, uint32_t val)
{
	uint8_t *p = (uint8_t *)buf + pos;

	switch (wsize) {
	case 1:
		*p = (uint8_t)val;
		break;
	case 2:
		*(uint16_t *)p = (uint16_t)val;
		break;
	default:
		*(uint32_t *)p = val;
		break;
	}
}

/* Write one slot to every TX serializer; lock held */
static void tx_service(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *tx = &data->tx;
	bool use_buf;

	if (stream_active(tx) && tx->slot == 0U && tx->buf == NULL) {
		struct mcasp_block blk;

		if (tx->stop) {
			tx->state = I2S_STATE_READY;
			data->tail = MCASP_TX_TAIL_SLOTS;
		} else if (k_msgq_get(tx->msgq, &blk, K_NO_WAIT) == 0) {
			tx->buf = blk.buf;
			tx->size = blk.size;
			tx->pos = 0U;
		} else if (tx->drain) {
			tx->state = I2S_STATE_READY;
			data->tail = MCASP_TX_TAIL_SLOTS;
		} else {
			LOG_DBG("TX underrun");
			tx->state = I2S_STATE_ERROR;
			data->tail = MCASP_TX_TAIL_SLOTS;
		}
	}

	/* A block starts on slot 0 only */
	use_buf = tx->buf != NULL && (tx->pos != 0U || tx->slot == 0U);

	for (uint8_t i = 0U; i < data->num_tx; i++) {
		uint32_t w = 0U;

		if (use_buf) {
			w = load_word(tx->buf, tx->pos, data->wsize);
			tx->pos += data->wsize;
		}
		tx_put(dev, i, w);
	}

	tx->slot = (tx->slot + 1U) % data->slots;
	data->tx_slots++;
	if (use_buf) {
		data->tx_data_end = data->tx_slots;
	}

	if (use_buf && tx->pos >= tx->size) {
		k_mem_slab_free(tx->cfg.mem_slab, tx->buf);
		tx->buf = NULL;
		if (tx->stop) {
			tx->state = I2S_STATE_READY;
			data->tail = MCASP_TX_TAIL_SLOTS;
		}
	} else if (!use_buf && !stream_active(tx) && data->tail > 0U) {
		data->tail--;
	}
}

/* DIR_BOTH DRAIN: TX sends no more data and RX has received all of it */
static bool rx_follow_done(struct mcasp_data *data)
{
	struct mcasp_stream *tx = &data->tx;
	bool tx_done = !stream_active(tx) ||
		       (tx->drain && tx->buf == NULL && k_msgq_num_used_get(tx->msgq) == 0U);

	return tx_done && data->rx_slots >= data->tx_data_end;
}

/* RX block end: hand the block over and start the next one at once; lock held */
static void rx_block_done(struct mcasp_data *data)
{
	struct mcasp_stream *rx = &data->rx;
	struct mcasp_block blk = {.buf = rx->buf, .size = rx->size};

	rx->buf = NULL;
	if (k_msgq_put(rx->msgq, &blk, K_NO_WAIT) != 0) {
		LOG_DBG("RX queue full");
		k_mem_slab_free(rx->cfg.mem_slab, blk.buf);
		rx->state = I2S_STATE_ERROR;
	} else if (rx->stop || (rx->follow_tx && rx_follow_done(data))) {
		rx->state = I2S_STATE_READY;
	} else if (k_mem_slab_alloc(rx->cfg.mem_slab, &rx->buf, K_NO_WAIT) == 0) {
		/* A STOP from here on ends with this block */
		rx->size = rx->cfg.block_size;
		rx->pos = 0U;
	} else {
		LOG_DBG("RX overrun");
		rx->state = I2S_STATE_ERROR;
	}
}

/* Read one slot from every RX serializer; lock held */
static void rx_service(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *rx = &data->rx;
	bool use_buf;

	if (stream_active(rx) && rx->slot == 0U && rx->buf == NULL) {
		if (rx->stop || (rx->follow_tx && rx_follow_done(data))) {
			rx->state = I2S_STATE_READY;
		} else if (k_mem_slab_alloc(rx->cfg.mem_slab, &rx->buf, K_NO_WAIT) == 0) {
			rx->size = rx->cfg.block_size;
			rx->pos = 0U;
		} else {
			LOG_DBG("RX overrun");
			rx->state = I2S_STATE_ERROR;
		}
	} else if (stream_active(rx) && rx->slot == 0U && rx->pos == 0U && rx->follow_tx &&
		   rx_follow_done(data)) {
		/* TX ended right after the previous block: this one stays empty */
		k_mem_slab_free(rx->cfg.mem_slab, rx->buf);
		rx->buf = NULL;
		rx->state = I2S_STATE_READY;
	}

	/* A block starts on slot 0 only */
	use_buf = rx->buf != NULL && (rx->pos != 0U || rx->slot == 0U);

	for (uint8_t i = 0U; i < data->num_rx; i++) {
		uint32_t w = rx_get(dev, i);

		if (use_buf) {
			store_word(rx->buf, rx->pos, data->wsize, w);
			rx->pos += data->wsize;
		}
	}

	rx->slot = (rx->slot + 1U) % data->slots;
	data->rx_slots++;

	if (use_buf && rx->pos >= rx->size) {
		rx_block_done(data);
	}
}

/*
 * Top the Write FIFO up with whole slots; lock held. Filling runs up to 64
 * words ahead of the serializer, so an empty TX queue is an underrun only
 * once the FIFO is close to empty: a writer may still be in time.
 */
static void tx_fill(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *tx = &data->tx;
	uint32_t level = fifo_level(dev, MCASP_WFIFOSTS);

	while (level + data->num_tx <= AFIFO_WORDS) {
		if (!stream_active(tx) && data->tail == 0U && !stream_active(&data->rx)) {
			/* Nothing left to send: run empty so the hardware can stop */
			break;
		}
		if (stream_active(tx) && tx->buf == NULL && tx->slot == 0U && !tx->stop &&
		    !tx->drain && k_msgq_num_used_get(tx->msgq) == 0U &&
		    level > 2U * data->num_tx) {
			break;
		}
		tx_service(dev);
		level += data->num_tx;
	}
}

/* Empty the Read FIFO, whole slots only; lock held */
static void rx_drain(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	uint32_t level = fifo_level(dev, MCASP_RFIFOSTS);

	while (level >= data->num_rx) {
		rx_service(dev);
		level -= data->num_rx;
	}
}

static bool tx_drained(const struct device *dev)
{
	struct mcasp_data *data = dev->data;

	if (data->num_tx == 0U) {
		return true;
	}

	return data->tail == 0U && (!use_fifo() || fifo_level(dev, MCASP_WFIFOSTS) == 0U);
}

static void maybe_stop(const struct device *dev)
{
	struct mcasp_data *data = dev->data;

	if (data->hw_running && !stream_active(&data->tx) && !stream_active(&data->rx) &&
	    tx_drained(dev)) {
		hw_stop(dev);
	}
}

static void tx_dma_step(const struct device *dev);

static void mcasp_tx_isr(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t stat = mcasp_read(dev, MCASP_XSTAT);

	if (!data->hw_running) {
		mcasp_write(dev, MCASP_XSTAT, stat & STAT_W1C);
		goto out;
	}

	/* An idle direction only moves fill data: its errors do not matter */
	if ((stat & STAT_ERRORS) != 0U && stream_active(&data->tx)) {
		hw_error(dev, true, stat);
		goto out;
	}

	if (use_fifo()) {
		/* Start of frame: top the FIFO up whatever the delay was */
		if (data->num_tx > 0U && !data->tx.dma) {
			tx_fill(dev);
		} else if (data->tx.dma) {
			tx_dma_step(dev);
		}
	} else if ((stat & STAT_DATA) != 0U && data->num_tx > 0U &&
		   (mcasp_read(dev, MCASP_SRCTL(data->tx_ser[0])) & SRCTL_XRDY) != 0U) {
		tx_service(dev);
	}

	/* Clearing the flags re-arms the interrupt (TRM 12.1.1.4.12.4) */
	mcasp_write(dev, MCASP_XSTAT, stat & STAT_W1C);
	(void)mcasp_read(dev, MCASP_XSTAT);
	maybe_stop(dev);
out:
	k_spin_unlock(&data->lock, key);
}

static void mcasp_rx_isr(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t stat = mcasp_read(dev, MCASP_RSTAT);

	if (!data->hw_running) {
		mcasp_write(dev, MCASP_RSTAT, stat & STAT_W1C);
		goto out;
	}

	/* An idle direction only moves fill data: its errors do not matter */
	if ((stat & STAT_ERRORS) != 0U && stream_active(&data->rx)) {
		hw_error(dev, false, stat);
		goto out;
	}

	if (use_fifo()) {
		if (data->num_rx > 0U && !data->rx.dma) {
			rx_drain(dev);
		}
	} else if ((stat & STAT_DATA) != 0U && data->num_rx > 0U &&
		   (mcasp_read(dev, MCASP_SRCTL(data->rx_ser[0])) & SRCTL_RRDY) != 0U) {
		rx_service(dev);
	}

	mcasp_write(dev, MCASP_RSTAT, stat & STAT_W1C);
	(void)mcasp_read(dev, MCASP_RSTAT);
	maybe_stop(dev);
out:
	k_spin_unlock(&data->lock, key);
}

struct mcasp_fmt {
	uint32_t fmt;
	uint32_t fsctl;
	uint32_t aclkxctl;
};

/* Frame sync, delay and clock edge of each format (TRM 12.1.1.4.4, 14.7.1.1.2.26-28) */
static int format_regs(const struct i2s_config *c, uint8_t slots, struct mcasp_fmt *f)
{
	bool fsp, fwid, clkp;
	uint32_t delay;

	if ((c->format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		return -EINVAL;
	}

	switch (c->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		/* WS low for the left channel, MSB one clock after WS */
		fsp = true;
		fwid = true;
		delay = 1U;
		clkp = true;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		fsp = false;
		fwid = true;
		delay = 0U;
		clkp = true;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		/* One-clock pulse, MSB one clock later, sampled on the falling edge */
		fsp = false;
		fwid = false;
		delay = 1U;
		clkp = false;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		fsp = false;
		fwid = true;
		delay = 0U;
		clkp = false;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	if ((c->format & I2S_FMT_FRAME_CLK_INV) != 0U) {
		fsp = !fsp;
	}
	if ((c->format & I2S_FMT_BIT_CLK_INV) != 0U) {
		clkp = !clkp;
	}

	/*
	 * MSB first, data right-aligned in the word (integer): TX rotates
	 * right by the slot size, RX does not rotate (TRM Tables 12-4, 12-6).
	 * Slot size equals word size.
	 */
	f->fmt = FMT_SSZ(c->word_size / 2U - 1U) | FMT_RVRS | FMT_DATDLY(delay) |
		 (use_fifo() ? 0U : FMT_BUSEL_CFG);
	f->fsctl = AFSCTL_MOD(slots) | (fwid ? AFSCTL_FWID : 0U) | (fsp ? AFSCTL_FSP : 0U);
	f->aclkxctl = clkp ? ACLKXCTL_CLKXP : 0U;

	return 0;
}

static bool word_size_valid(uint8_t ws)
{
	return ws >= 8U && ws <= 32U && (ws % 4U) == 0U;
}

static uint8_t dir_serializers(const struct mcasp_data *data, enum i2s_dir dir)
{
	return dir == I2S_DIR_TX ? data->num_tx : data->num_rx;
}

#ifdef CONFIG_I2S_TI_MCASP_DMA
static uint32_t dma_event_words(const struct mcasp_data *data, bool tx)
{
	return (uint32_t)data->slots * (tx ? data->num_tx : data->num_rx);
}

static inline bool dma_asel_coherent(uint8_t asel)
{
	return asel == MCASP_DMA_ASEL_ACP0 || asel == MCASP_DMA_ASEL_ACP1;
}

static int dma_cache_out(const struct device *dev, bool tx, void *buf, size_t size)
{
	const struct mcasp_config *cfg = dev->config;

	if (dma_asel_coherent(cfg->dma_asel[tx ? 0 : 1])) {
		return 0;
	}

	return tx ? sys_cache_data_flush_range(buf, size)
		  : sys_cache_data_flush_and_invd_range(buf, size);
}

static int dma_cache_in(const struct device *dev, void *buf, size_t size)
{
	const struct mcasp_config *cfg = dev->config;

	if (dma_asel_coherent(cfg->dma_asel[1])) {
		return 0;
	}

	return sys_cache_data_invd_range(buf, size);
}

static int dma_tx_flush(const struct device *dev, void *buf, size_t size)
{
	const struct mcasp_config *cfg = dev->config;
	int ret;

	if (cfg->dma_dev == NULL) {
		return 0;
	}

	ret = dma_cache_out(dev, true, buf, size);
	if (ret < 0) {
		LOG_ERR("TX DMA cache maintenance: %d", ret);
	}

	return ret;
}

static void mcasp_dma_cb(const struct device *dma_dev, void *arg, uint32_t channel, int status);

static int dma_submit(const struct device *dev, struct mcasp_stream *s, void *buf, size_t size)
{
	const struct mcasp_config *cfg = dev->config;
	struct mcasp_data *data = dev->data;
	bool tx = dma_is_tx(dev, s);
	uint32_t ch = cfg->dma_ch[tx ? 0 : 1];
	uintptr_t mem = (uintptr_t)buf;
	int ret;

	if (!tx) {
		ret = dma_cache_out(dev, false, buf, size);
		if (ret < 0) {
			LOG_ERR("RX DMA cache maintenance: %d", ret);
			return ret;
		}
	}

	if (!s->dma_used) {
		uint32_t burst = dma_event_words(data, tx) * data->wsize;
		struct dma_block_config blk = {
			.source_address = tx ? mem : cfg->dat_phys,
			.dest_address = tx ? cfg->dat_phys : mem,
			.source_addr_adj = tx ? DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE,
			.dest_addr_adj = tx ? DMA_ADDR_ADJ_NO_CHANGE : DMA_ADDR_ADJ_INCREMENT,
			.block_size = size,
		};
		struct dma_config dc = {
			.channel_direction = tx ? MEMORY_TO_PERIPHERAL : PERIPHERAL_TO_MEMORY,
			.dma_slot = cfg->dma_asel[tx ? 0 : 1],
			.source_data_size = data->wsize,
			.dest_data_size = data->wsize,
			.source_burst_length = burst,
			.dest_burst_length = burst,
			.block_count = 1U,
			.head_block = &blk,
			.user_data = (void *)dev,
			.dma_callback = mcasp_dma_cb,
		};

		ret = dma_config(cfg->dma_dev, ch, &dc);
		if (ret == 0) {
			s->dma_used = true;
			s->dma_head = 0U;
			s->dma_n = 0U;
			s->dma_sub = 0U;
		}
	} else {
		ret = dma_reload(cfg->dma_dev, ch, tx ? mem : cfg->dat_phys,
				 tx ? cfg->dat_phys : mem, size);
	}
	if (ret == 0) {
		ret = dma_start(cfg->dma_dev, ch);
	}
	if (ret < 0) {
		LOG_ERR("%s DMA chan %u: %d", tx ? "TX" : "RX", ch, ret);
		return ret;
	}

	s->dma_q[(s->dma_head + s->dma_n) % MCASP_DMA_DEPTH] =
		(struct mcasp_block){.buf = buf, .size = size};
	s->dma_n++;
	if (tx) {
		data->tx_slots += size / ((size_t)data->wsize * data->num_tx);
		data->tx_data_end = data->tx_slots;
		s->dma_sub += (uint32_t)size;
	}

	return 0;
}

static void tx_dma_refill(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *tx = &data->tx;
	struct mcasp_block blk;

	while (tx->dma && stream_active(tx) && !tx->stop && tx->dma_n < MCASP_DMA_DEPTH_TX &&
	       k_msgq_get(tx->msgq, &blk, K_NO_WAIT) == 0) {
		if (dma_submit(dev, tx, blk.buf, blk.size) < 0) {
			k_mem_slab_free(tx->cfg.mem_slab, blk.buf);
			hw_error(dev, true, 0U);
			return;
		}
	}
}

static void rx_dma_refill(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *rx = &data->rx;
	void *buf;

	while (rx->dma && stream_active(rx) && rx->dma_n < MCASP_DMA_DEPTH &&
	       k_mem_slab_alloc(rx->cfg.mem_slab, &buf, K_NO_WAIT) == 0) {
		if (dma_submit(dev, rx, buf, rx->cfg.block_size) < 0) {
			k_mem_slab_free(rx->cfg.mem_slab, buf);
			hw_error(dev, false, 0U);
			return;
		}
	}
}

static void tx_stafrm(const struct device *dev, bool on)
{
	struct mcasp_data *data = dev->data;
	uint32_t reg;

	if (!data->hw_running) {
		return;
	}
	reg = mcasp_read(dev, MCASP_XINTCTL);
	mcasp_write(dev, MCASP_XINTCTL, on ? reg | INT_STAFRM : reg & ~INT_STAFRM);
}

static uint32_t tx_dma_left(const struct device *dev)
{
	const struct mcasp_config *cfg = dev->config;
	struct mcasp_data *data = dev->data;
	struct dma_status st;

	if (dma_get_status(cfg->dma_dev, cfg->dma_ch[0], &st) < 0) {
		return 0U;
	}

	return (data->tx.dma_sub - (uint32_t)st.total_copied) & MCASP_DMA_BCNT_MASK;
}

static uint32_t tx_dma_lead(const struct mcasp_data *data)
{
	uint32_t frame = (uint32_t)data->slots * data->num_tx;
	uint32_t frames =
		DIV_ROUND_UP(data->tx.cfg.frame_clk_freq * MCASP_DMA_TX_LEAD_US, USEC_PER_SEC);

	return MIN(MAX(frames, MCASP_DMA_LEAD_FRAMES_MIN) * frame, AFIFO_WORDS);
}

static void tx_dma_step(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *tx = &data->tx;
	bool more = k_msgq_num_used_get(tx->msgq) > 0U;
	bool wait = false;
	uint32_t left;

	if (!tx->dma) {
		return;
	}

	left = tx_dma_left(dev);
	if (stream_active(tx) && !tx->stop && more && tx->dma_n < MCASP_DMA_DEPTH_TX) {
		uint32_t unplayed = left / data->wsize + fifo_level(dev, MCASP_WFIFOSTS);

		if (unplayed < tx_dma_lead(data)) {
			tx_dma_refill(dev);
			if (!tx->dma) {
				return;
			}
		} else {
			wait = true;
		}
	} else if (tx->dma_n == 0U && (!stream_active(tx) || tx->stop || (tx->drain && !more))) {
		if (left == 0U) {
			if (stream_active(tx)) {
				tx->state = I2S_STATE_READY;
			}
			data->tail = MCASP_TX_TAIL_SLOTS;
			dma_handover(dev, tx);
			return;
		}
		wait = true;
	}
	tx_stafrm(dev, wait);
}

static void tx_dma_done(const struct device *dev, struct mcasp_block *blk)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *tx = &data->tx;

	k_mem_slab_free(tx->cfg.mem_slab, blk->buf);
	tx_dma_step(dev);
}

static void rx_dma_done(const struct device *dev, struct mcasp_block *blk)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *rx = &data->rx;
	bool end;
	int ret;

	ret = dma_cache_in(dev, blk->buf, blk->size);
	if (ret < 0) {
		LOG_ERR("RX DMA: cache invalidate failed (%d)", ret);
		k_mem_slab_free(rx->cfg.mem_slab, blk->buf);
		rx->state = I2S_STATE_ERROR;
	} else if (k_msgq_put(rx->msgq, blk, K_NO_WAIT) != 0) {
		LOG_DBG("RX DMA: queue full");
		k_mem_slab_free(rx->cfg.mem_slab, blk->buf);
		rx->state = I2S_STATE_ERROR;
	} else {
		data->rx_slots += blk->size / ((size_t)data->wsize * data->num_rx);
		if (rx->stop || (rx->follow_tx && rx_follow_done(data))) {
			rx->state = I2S_STATE_READY;
		}
	}

	end = !stream_active(rx);
	if (!end) {
		rx_dma_refill(dev);
		if (rx->dma_n == 0U && rx->dma) {
			LOG_DBG("RX DMA: overrun");
			rx->state = I2S_STATE_ERROR;
			end = true;
		}
	}
	if (end && rx->dma) {
		dma_abort(dev, rx);
	}
}

static void mcasp_dma_cb(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	const struct device *dev = arg;
	const struct mcasp_config *cfg = dev->config;
	struct mcasp_data *data = dev->data;
	bool tx = channel == cfg->dma_ch[0];
	struct mcasp_stream *s = tx ? &data->tx : &data->rx;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	struct mcasp_block blk;

	ARG_UNUSED(dma_dev);

	if (!s->dma || s->dma_n == 0U || status > 0) {
		goto out;
	}

	if (status < 0) {
		LOG_ERR("%s DMA error %d", tx ? "TX" : "RX", status);
		hw_error(dev, tx, 0U);
		goto out;
	}

	blk = s->dma_q[s->dma_head];
	s->dma_head = (s->dma_head + 1U) % MCASP_DMA_DEPTH;
	s->dma_n--;
	if (tx) {
		tx_dma_done(dev, &blk);
	} else {
		rx_dma_done(dev, &blk);
	}
out:
	k_spin_unlock(&data->lock, key);
}

static int dma_go(const struct device *dev, struct mcasp_stream *s)
{
	int ret = dma_submit(dev, s, s->buf, s->size);

	if (ret < 0) {
		return ret;
	}
	s->buf = NULL;
	if (dma_is_tx(dev, s)) {
		tx_dma_step(dev);
	} else {
		rx_dma_refill(dev);
	}

	return s->dma ? 0 : -EIO;
}

static bool dma_rx_blocks_ok(const struct device *dev, const void *first)
{
	const struct mcasp_config *cfg = dev->config;
	const struct mcasp_data *data = dev->data;

	if (dma_asel_coherent(cfg->dma_asel[1])) {
		return true;
	}
	if ((((uintptr_t)first | data->rx.cfg.mem_slab->info.block_size) &
	     (MCASP_DMA_ALIGN - 1U)) != 0U) {
		LOG_WRN_ONCE("RX blocks not aligned to %u bytes, RX uses interrupts",
			     MCASP_DMA_ALIGN);
		return false;
	}

	return true;
}
#else
static inline int dma_tx_flush(const struct device *dev, void *buf, size_t size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	return 0;
}

static inline void tx_dma_refill(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline void tx_dma_step(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline int dma_go(const struct device *dev, struct mcasp_stream *s)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(s);
	return -ENOTSUP;
}

static inline uint32_t dma_event_words(const struct mcasp_data *data, bool tx)
{
	return tx ? data->num_tx : data->num_rx;
}

static inline bool dma_rx_blocks_ok(const struct device *dev, const void *first)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(first);
	return false;
}
#endif

/* Program and start the serial port; lock held */
static int hw_start(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	const struct i2s_config *c = clock_cfg(data);
	bool is_tx = c == &data->tx.cfg;
	uint8_t nser = dir_serializers(data, is_tx ? I2S_DIR_TX : I2S_DIR_RX);
	bool bclk_target = (c->options & I2S_OPT_BIT_CLK_TARGET) != 0U;
	bool fs_target = (c->options & I2S_OPT_FRAME_CLK_TARGET) != 0U;
	uint32_t hdiv = 1U, cdiv = 1U, pdir = 0U, mask, fmt, gbl_r = 0U, gbl_x, ints;
	struct mcasp_fmt f;
	int ret;

	if (stream_active(&data->tx) && stream_active(&data->rx) &&
	    !cfg_compatible(data, &data->tx.cfg, &data->rx.cfg)) {
		return -EINVAL;
	}

	data->slots = c->channels / nser;
	data->wsize = word_bytes(c->word_size);
	data->frame_hz = c->frame_clk_freq;

	data->tx.dma = data->tx.dma && stream_active(&data->tx) &&
		       dma_event_words(data, true) <= AFIFO_WORDS;
	data->rx.dma = data->rx.dma && stream_active(&data->rx) &&
		       dma_event_words(data, false) <= AFIFO_WORDS;

	ret = format_regs(c, data->slots, &f);
	if (ret < 0) {
		return ret;
	}

	if (!bclk_target) {
		uint32_t bclk = c->frame_clk_freq * data->slots * c->word_size;

		ret = clock_dividers(data->fck_rate, bclk, &hdiv, &cdiv);
		if (ret < 0) {
			return ret;
		}
	}

	ret = gblctl_write(dev, 0U);
	if (ret < 0) {
		return ret;
	}
	mcasp_write(dev, MCASP_PWRIDLESYSCONFIG, PWRIDLE_NO_IDLE);
	mcasp_write(dev, MCASP_DITCTL, 0U);

	/*
	 * Counts first, then enable, all before the McASP leaves reset (TRM
	 * 12.1.1.4.11, WFIFOCTL). Disabling first flushes old words.
	 */
	mcasp_write(dev, MCASP_WFIFOCTL, 0U);
	mcasp_write(dev, MCASP_RFIFOCTL, 0U);
	data->tx_slots = 0U;
	data->tx_data_end = 0U;
	data->rx_slots = 0U;
	if (use_fifo() && data->num_tx > 0U) {
		uint32_t v =
			FIFOCTL_NUMDMA(data->num_tx) |
			FIFOCTL_NUMEVT(data->tx.dma ? dma_event_words(data, true) : data->num_tx);

		mcasp_write(dev, MCASP_WFIFOCTL, v);
		if (data->tx.dma) {
			ret = dma_go(dev, &data->tx);
			if (ret < 0) {
				goto fail;
			}
		}
		mcasp_write(dev, MCASP_WFIFOCTL, v | FIFOCTL_ENA);
	}
	if (use_fifo() && data->num_rx > 0U) {
		uint32_t v =
			FIFOCTL_NUMDMA(data->num_rx) |
			FIFOCTL_NUMEVT(data->rx.dma ? dma_event_words(data, false) : data->num_rx);

		mcasp_write(dev, MCASP_RFIFOCTL, v);
		mcasp_write(dev, MCASP_RFIFOCTL, v | FIFOCTL_ENA);
	}

	if (loopback_enabled(data)) {
		ret = loopback_dlbctl(dev);
		if (ret < 0) {
			goto fail;
		}
		mcasp_write(dev, MCASP_DLBCTL, (uint32_t)ret);
	} else {
		mcasp_write(dev, MCASP_DLBCTL, 0U);
	}

	mask = c->word_size >= 32U ? UINT32_MAX : BIT_MASK(c->word_size);
	fmt = f.fmt;
	mcasp_write(dev, MCASP_XMASK, mask);
	mcasp_write(dev, MCASP_XFMT, fmt | FMT_ROT((c->word_size % 32U) / 4U));
	mcasp_write(dev, MCASP_RMASK, mask);
	if (c->word_size < 32U) {
		fmt |= FMT_PAD_PBIT | FMT_PBIT(c->word_size - 1U);
	}
	mcasp_write(dev, MCASP_RFMT, fmt);

	mcasp_write(dev, MCASP_AFSXCTL, f.fsctl | (fs_target ? 0U : AFSCTL_FSM));
	mcasp_write(dev, MCASP_AFSRCTL, f.fsctl | (fs_target ? 0U : AFSCTL_FSM));
	/* ASYNC = 0: receive uses the transmit clock and frame sync */
	mcasp_write(dev, MCASP_ACLKXCTL,
		    f.aclkxctl | ACLKXCTL_DIV(cdiv) | (bclk_target ? 0U : ACLKXCTL_CLKXM));
	mcasp_write(dev, MCASP_AHCLKXCTL, AHCLKXCTL_HCLKXM | AHCLKXCTL_DIV(hdiv));

	mcasp_write(dev, MCASP_XTDM, BIT_MASK(data->slots));
	mcasp_write(dev, MCASP_RTDM, BIT_MASK(data->slots));
	/* The AFIFO serves the McASP DMA requests (TRM 12.1.1.4.11.1, Table 12-30) */
	mcasp_write(dev, MCASP_XEVTCTL, use_fifo() ? 0U : EVTCTL_DMA_OFF);
	mcasp_write(dev, MCASP_REVTCTL, use_fifo() ? 0U : EVTCTL_DMA_OFF);
	mcasp_write(dev, MCASP_XINTCTL, 0U);
	mcasp_write(dev, MCASP_RINTCTL, 0U);

	for (uint8_t n = 0U; n < MCASP_MAX_SER; n++) {
		mcasp_write(dev, MCASP_SRCTL(n), 0U);
	}
	for (uint8_t i = 0U; i < data->num_tx; i++) {
		mcasp_write(dev, MCASP_SRCTL(data->tx_ser[i]), SRCTL_SRMOD_TX | SRCTL_DISMOD_LO);
		pdir |= BIT(data->tx_ser[i]);
	}
	for (uint8_t i = 0U; i < data->num_rx; i++) {
		mcasp_write(dev, MCASP_SRCTL(data->rx_ser[i]), SRCTL_SRMOD_RX);
	}
	pdir |= (bclk_target ? 0U : PDIR_ACLKX) | (fs_target ? 0U : PDIR_AFSX);

	mcasp_write(dev, MCASP_PFUNC, 0U);
	mcasp_write(dev, MCASP_XSTAT, STAT_W1C);
	mcasp_write(dev, MCASP_RSTAT, STAT_W1C);

	data->tx.slot = 0U;
	data->rx.slot = 0U;
	data->tail = 0U;

	/* TRM Table 12-18 and Figure 12-28: clocks, serializers, state machines, frame sync */
	if (data->num_rx > 0U) {
		gbl_r = GBLCTL_RHCLKRST | GBLCTL_RCLKRST;
	}
	ret = gblctl_set(dev, gbl_r | GBLCTL_XHCLKRST | GBLCTL_XCLKRST);
	if (ret < 0) {
		goto fail;
	}
	mcasp_write(dev, MCASP_PDIR, pdir);

	gbl_r = data->num_rx > 0U ? GBLCTL_RSRCLR : 0U;
	gbl_x = data->num_tx > 0U ? GBLCTL_XSRCLR : 0U;
	ret = gblctl_set(dev, gbl_r | gbl_x);
	if (ret < 0) {
		goto fail;
	}

	if (data->num_tx > 0U) {
		/*
		 * Preload so the state machine starts without an underrun. With
		 * the AFIFO, fill it; it answers the McASP request on its own.
		 */
		if (!data->tx.dma && use_fifo()) {
			tx_fill(dev);
		} else if (!data->tx.dma) {
			tx_service(dev);
		}
		if (!WAIT_FOR((mcasp_read(dev, MCASP_XSTAT) & STAT_DATA) == 0U, MCASP_POLL_US,
			      k_busy_wait(1))) {
			LOG_ERR("TX buffers not loaded: XSTAT 0x%08x WFIFOSTS %u words",
				mcasp_read(dev, MCASP_XSTAT),
				use_fifo() ? fifo_level(dev, MCASP_WFIFOSTS) : 0U);
			ret = -EIO;
			goto fail;
		}
	}

	if (data->rx.dma) {
		ret = dma_go(dev, &data->rx);
		if (ret < 0) {
			goto fail;
		}
	}

	gbl_r = data->num_rx > 0U ? GBLCTL_RSMRST : 0U;
	gbl_x = data->num_tx > 0U ? GBLCTL_XSMRST : 0U;
	ret = gblctl_set(dev, gbl_r | gbl_x);
	if (ret < 0) {
		goto fail;
	}

	mcasp_write(dev, MCASP_XSTAT, STAT_W1C & ~STAT_DATA);
	mcasp_write(dev, MCASP_RSTAT, STAT_W1C);
	/* AFIFO: one interrupt per frame moves whole FIFO loads (TRM Table 12-30) */
	ints = INT_UNDRN_OVRN | INT_SYNCERR | (use_fifo() ? INT_STAFRM | INT_DMAERR : INT_DATA);
	if (data->num_tx > 0U) {
		mcasp_write(dev, MCASP_XINTCTL, ints);
	}
	if (data->num_rx > 0U) {
		mcasp_write(dev, MCASP_RINTCTL, data->rx.dma ? ints & ~INT_STAFRM : ints);
	}
	data->hw_running = true;

	gbl_r = data->num_rx > 0U ? GBLCTL_RFRST : 0U;
	ret = gblctl_set(dev, gbl_r | GBLCTL_XFRST);
	if (ret < 0) {
		goto fail;
	}

	return 0;

fail:
	hw_stop(dev);
	return ret;
}

static void stream_purge(struct mcasp_stream *s, void *buf)
{
	struct mcasp_block blk;

	if (buf != NULL) {
		k_mem_slab_free(s->cfg.mem_slab, buf);
	}
	while (k_msgq_get(s->msgq, &blk, K_NO_WAIT) == 0) {
		k_mem_slab_free(s->cfg.mem_slab, blk.buf);
	}
}

static int check_config(const struct device *dev, enum i2s_dir dir, const struct i2s_config *c)
{
	struct mcasp_data *data = dev->data;
	const struct mcasp_stream *other = dir == I2S_DIR_TX ? &data->rx : &data->tx;
	uint8_t nser = dir_serializers(data, dir);
	uint8_t slots;
	struct mcasp_fmt f;
	int ret;

	if (nser == 0U) {
		LOG_ERR("no %s serializer in serial-dir", dir == I2S_DIR_TX ? "TX" : "RX");
		return -EINVAL;
	}
	if (c->mem_slab == NULL || c->block_size == 0U || !word_size_valid(c->word_size)) {
		return -EINVAL;
	}
	if ((c->options & (I2S_OPT_BIT_CLK_GATED | I2S_OPT_PINGPONG)) != 0U) {
		return -ENOTSUP;
	}
	if (c->channels == 0U || (c->channels % nser) != 0U) {
		return -EINVAL;
	}

	slots = c->channels / nser;
	switch (c->format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		if (slots != 2U) {
			return -EINVAL;
		}
		break;
	default:
		if (slots < 2U || slots > MCASP_MAX_SLOTS) {
			return -EINVAL;
		}
		break;
	}

	if (use_fifo() && c->channels > AFIFO_FRAME_MAX) {
		LOG_ERR("%u words per frame, the FIFO mode takes %u", c->channels,
			AFIFO_FRAME_MAX);
		return -EINVAL;
	}

	ret = format_regs(c, slots, &f);
	if (ret < 0) {
		return ret;
	}

	if ((c->block_size % ((size_t)c->channels * word_bytes(c->word_size))) != 0U) {
		return -EINVAL;
	}

	if ((c->options & I2S_OPT_LOOPBACK) != 0U && loopback_dlbctl(dev) < 0) {
		LOG_ERR("serial-dir does not pair TX and RX serializers for loopback");
		return -EINVAL;
	}

	/* A running direction fixes the clock; otherwise START checks the pair */
	if (stream_active(other) && !cfg_compatible(data, dir == I2S_DIR_TX ? c : &other->cfg,
						    dir == I2S_DIR_TX ? &other->cfg : c)) {
		LOG_ERR("TX and RX must share rate, word size, format and slots");
		return -EINVAL;
	}

	if ((c->options & I2S_OPT_BIT_CLK_TARGET) == 0U) {
		uint32_t hdiv, cdiv, bclk = c->frame_clk_freq * slots * c->word_size;

		ret = clock_dividers(data->fck_rate, bclk, &hdiv, &cdiv);
		if (ret < 0) {
			LOG_ERR("no divider for %u Hz bit clock from %u Hz", bclk, data->fck_rate);
			return ret;
		}
		LOG_DBG("fck %u Hz, bit clock %u Hz (asked %u), HCLKXDIV %u CLKXDIV %u",
			data->fck_rate, data->fck_rate / (hdiv * cdiv), bclk, hdiv, cdiv);
	}

	return 0;
}

static int configure_one(const struct device *dev, enum i2s_dir dir, const struct i2s_config *c)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *s = dir == I2S_DIR_TX ? &data->tx : &data->rx;
	int ret;

	if (stream_active(s)) {
		return -EINVAL;
	}

	if (c->frame_clk_freq == 0U) {
		if (s->state != I2S_STATE_NOT_READY) {
			stream_purge(s, NULL);
		}
		s->state = I2S_STATE_NOT_READY;
		return 0;
	}

	ret = check_config(dev, dir, c);
	if (ret < 0) {
		return ret;
	}

	/* READY or ERROR: drop what the old setting queued */
	if (s->state != I2S_STATE_NOT_READY) {
		stream_purge(s, NULL);
	}
	s->cfg = *c;
	s->stop = false;
	s->drain = false;
	s->follow_tx = false;
	s->state = I2S_STATE_READY;

	return 0;
}

static int mcasp_configure(const struct device *dev, enum i2s_dir dir,
			   const struct i2s_config *i2s_cfg)
{
	const struct mcasp_config *cfg = dev->config;
	struct mcasp_data *data = dev->data;
	int ret;

	if (i2s_cfg->frame_clk_freq != 0U) {
		ret = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &data->fck_rate);
		if (ret < 0) {
			LOG_WRN("fck rate unknown (%d)", ret);
			data->fck_rate = 0U;
		}
	}

	if (dir == I2S_DIR_BOTH) {
		if (stream_active(&data->tx) || stream_active(&data->rx)) {
			return -EINVAL;
		}
		/* Unconfigure RX first, so it does not constrain the new TX setting */
		if (data->rx.state != I2S_STATE_NOT_READY) {
			stream_purge(&data->rx, NULL);
		}
		data->rx.state = I2S_STATE_NOT_READY;
		ret = configure_one(dev, I2S_DIR_TX, i2s_cfg);
		if (ret < 0) {
			return ret;
		}
		return configure_one(dev, I2S_DIR_RX, i2s_cfg);
	}

	if (dir != I2S_DIR_TX && dir != I2S_DIR_RX) {
		return -EINVAL;
	}

	return configure_one(dev, dir, i2s_cfg);
}

static const struct i2s_config *mcasp_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *s;

	if (dir == I2S_DIR_TX) {
		s = &data->tx;
	} else if (dir == I2S_DIR_RX) {
		s = &data->rx;
	} else {
		return NULL;
	}

	return s->state == I2S_STATE_NOT_READY ? NULL : &s->cfg;
}

static int mcasp_write_block(const struct device *dev, void *mem_block, size_t size)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *tx = &data->tx;
	struct mcasp_block blk = {.buf = mem_block, .size = size};
	size_t frame;
	int ret;

	if (tx->state != I2S_STATE_READY && tx->state != I2S_STATE_RUNNING) {
		return -EIO;
	}

	frame = (size_t)tx->cfg.channels * word_bytes(tx->cfg.word_size);
	if (size == 0U || size > tx->cfg.block_size || (size % frame) != 0U) {
		return -EINVAL;
	}

	ret = dma_tx_flush(dev, mem_block, size);
	if (ret < 0) {
		return ret;
	}

	ret = k_msgq_put(tx->msgq, &blk, SYS_TIMEOUT_MS(tx->cfg.timeout));
	if (ret == 0 && tx->dma) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		tx_dma_step(dev);
		k_spin_unlock(&data->lock, key);
	}

	return ret == -ENOMSG ? -EBUSY : ret;
}

static int mcasp_read_block(const struct device *dev, void **mem_block, size_t *size)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_stream *rx = &data->rx;
	struct mcasp_block blk;
	int ret;

	if (rx->state == I2S_STATE_NOT_READY) {
		return -EIO;
	}

	if (rx->state == I2S_STATE_ERROR) {
		ret = k_msgq_get(rx->msgq, &blk, K_NO_WAIT);
		if (ret != 0) {
			return -EIO;
		}
	} else {
		ret = k_msgq_get(rx->msgq, &blk, SYS_TIMEOUT_MS(rx->cfg.timeout));
		if (ret == -ENOMSG) {
			return -EBUSY;
		}
		if (ret != 0) {
			return ret;
		}
	}

	*mem_block = blk.buf;
	*size = blk.size;

	return 0;
}

static bool tail_pending(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	bool pending = data->hw_running && !stream_active(&data->tx) && !stream_active(&data->rx);

	k_spin_unlock(&data->lock, key);

	return pending;
}

static void tail_wait(const struct device *dev)
{
	struct mcasp_data *data = dev->data;
	uint32_t frames, timeout_us;

	if (!tail_pending(dev)) {
		return;
	}

	frames = DIV_ROUND_UP(AFIFO_WORDS, (uint32_t)data->num_tx * data->slots) + 2U;
	timeout_us = DIV_ROUND_UP(frames * USEC_PER_SEC, data->frame_hz);
	if (!WAIT_FOR(!tail_pending(dev), timeout_us, k_usleep(MCASP_POLL_US))) {
		LOG_WRN("TX FIFO not played out");
	}
}

/* Start TX and/or RX; streams are READY */
static int start_streams(const struct device *dev, bool tx, bool rx)
{
	struct mcasp_data *data = dev->data;
	struct mcasp_block blk = {0};
	void *rx_buf = NULL;
	k_spinlock_key_t key;
	int ret = 0;

	/*
	 * Take the first blocks outside the lock: that may wake a writer. RX
	 * owns a block from START on, so a STOP always ends with that block.
	 */
	if (rx && k_mem_slab_alloc(data->rx.cfg.mem_slab, &rx_buf, K_NO_WAIT) != 0) {
		LOG_ERR("RX START without a free block");
		return -ENOMEM;
	}
	if (tx && k_msgq_get(data->tx.msgq, &blk, K_NO_WAIT) != 0) {
		if (rx_buf != NULL) {
			k_mem_slab_free(data->rx.cfg.mem_slab, rx_buf);
		}
		return -EIO;
	}

	if (use_fifo() && data->num_tx > 0U) {
		tail_wait(dev);
	}

	key = k_spin_lock(&data->lock);

	/* Only the zero tail after a TX end is left: restart from a clean frame */
	if (data->hw_running && !stream_active(&data->tx) && !stream_active(&data->rx)) {
		hw_stop(dev);
	}

	if (tx) {
		data->tx.buf = blk.buf;
		data->tx.size = blk.size;
		data->tx.pos = 0U;
		data->tx.stop = false;
		data->tx.drain = false;
		data->tx.dma = use_dma(dev) && !data->hw_running;
		data->tx.state = I2S_STATE_RUNNING;
	}
	if (rx) {
		data->rx.buf = rx_buf;
		data->rx.size = data->rx.cfg.block_size;
		data->rx.pos = 0U;
		data->rx.stop = false;
		data->rx.follow_tx = false;
		data->rx.dma = use_dma(dev) && !data->hw_running && dma_rx_blocks_ok(dev, rx_buf);
		data->rx.state = I2S_STATE_RUNNING;
	}

	if (data->hw_running) {
		/* The interrupt handlers pick the streams up at the next slot 0 */
		goto out;
	}

	/*
	 * In loopback RX only sees what TX sends: when TX has data waiting for
	 * its own START, start the hardware then, so RX gets the first frame.
	 */
	if (!tx && loopback_enabled(data) && data->tx.state == I2S_STATE_READY &&
	    k_msgq_num_used_get(data->tx.msgq) > 0U) {
		goto out;
	}

	ret = hw_start(dev);
	if (ret < 0) {
		struct mcasp_stream *streams[] = {&data->tx, &data->rx};

		for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
			if (stream_active(streams[i])) {
				streams[i]->state = I2S_STATE_ERROR;
			}
		}
		/* The interrupt handlers no longer own the blocks */
		blk.buf = data->tx.buf;
		rx_buf = data->rx.buf;
		data->tx.buf = NULL;
		data->rx.buf = NULL;
	}

out:
	k_spin_unlock(&data->lock, key);

	if (ret < 0) {
		if (blk.buf != NULL) {
			k_mem_slab_free(data->tx.cfg.mem_slab, blk.buf);
		}
		if (rx_buf != NULL) {
			k_mem_slab_free(data->rx.cfg.mem_slab, rx_buf);
		}
	}

	return ret;
}

static int stream_trigger_check(const struct mcasp_stream *s, enum i2s_trigger_cmd cmd)
{
	switch (cmd) {
	case I2S_TRIGGER_START:
		return s->state == I2S_STATE_READY ? 0 : -EIO;
	case I2S_TRIGGER_STOP:
	case I2S_TRIGGER_DRAIN:
		return s->state == I2S_STATE_RUNNING ? 0 : -EIO;
	case I2S_TRIGGER_DROP:
		return s->state == I2S_STATE_NOT_READY ? -EIO : 0;
	case I2S_TRIGGER_PREPARE:
		return s->state == I2S_STATE_ERROR ? 0 : -EIO;
	default:
		return -EINVAL;
	}
}

/* DROP or PREPARE: back to READY and empty the queue */
static void stream_reset(const struct device *dev, struct mcasp_stream *s)
{
	struct mcasp_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	void *buf = s->buf;

	dma_abort(dev, s);
	s->buf = NULL;
	s->stop = false;
	s->drain = false;
	s->follow_tx = false;
	s->state = I2S_STATE_READY;
	if (s == &data->tx) {
		data->tail = 0U;
	}
	/* Nothing left active: stop now, without playing out the FIFO */
	if (data->hw_running && !stream_active(&data->tx) && !stream_active(&data->rx)) {
		hw_stop(dev);
	}
	k_spin_unlock(&data->lock, key);

	stream_purge(s, buf);
}

static int stream_stop(const struct device *dev, struct mcasp_stream *s, bool drain, bool follow_tx)
{
	struct mcasp_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	void *buf = NULL;
	int ret = 0;

	if (s->state != I2S_STATE_RUNNING) {
		/* Ended or failed since the check */
		ret = -EIO;
	} else if (!data->hw_running) {
		/* RX waiting for TX to start in loopback: nothing received yet */
		buf = s->buf;
		s->buf = NULL;
		s->state = I2S_STATE_READY;
	} else {
		s->state = I2S_STATE_STOPPING;
		if (drain) {
			s->drain = true;
		} else if (follow_tx) {
			s->follow_tx = true;
		} else {
			s->stop = true;
		}
		if (s == &data->tx) {
			tx_dma_step(dev);
		}
	}
	k_spin_unlock(&data->lock, key);

	if (buf != NULL) {
		k_mem_slab_free(s->cfg.mem_slab, buf);
	}

	return ret;
}

static int mcasp_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	struct mcasp_data *data = dev->data;
	bool do_tx = dir == I2S_DIR_TX || dir == I2S_DIR_BOTH;
	bool do_rx = dir == I2S_DIR_RX || dir == I2S_DIR_BOTH;
	int ret;

	if (dir != I2S_DIR_TX && dir != I2S_DIR_RX && dir != I2S_DIR_BOTH) {
		return -EINVAL;
	}
	if (cmd > I2S_TRIGGER_PREPARE) {
		return -EINVAL;
	}

	if (do_tx) {
		ret = stream_trigger_check(&data->tx, cmd);
		if (ret < 0) {
			return ret;
		}
	}
	if (do_rx) {
		ret = stream_trigger_check(&data->rx, cmd);
		if (ret < 0) {
			return ret;
		}
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (do_tx && k_msgq_num_used_get(data->tx.msgq) == 0U) {
			LOG_ERR("TX START with an empty queue");
			return -EIO;
		}
		if ((do_tx || stream_active(&data->tx)) && (do_rx || stream_active(&data->rx)) &&
		    !cfg_compatible(data, &data->tx.cfg, &data->rx.cfg)) {
			LOG_ERR("TX and RX must share rate, word size, format and slots");
			return -EIO;
		}
		return start_streams(dev, do_tx, do_rx);
	case I2S_TRIGGER_STOP:
	case I2S_TRIGGER_DRAIN:
		ret = 0;
		if (do_tx) {
			ret = stream_stop(dev, &data->tx, cmd == I2S_TRIGGER_DRAIN, false);
		}
		if (do_rx) {
			/* DRAIN with TX: RX ends with the last TX block; alone it is STOP */
			int rx_ret = stream_stop(dev, &data->rx, false,
						 cmd == I2S_TRIGGER_DRAIN && do_tx);

			ret = ret < 0 ? ret : rx_ret;
		}
		return ret;
	case I2S_TRIGGER_DROP:
	case I2S_TRIGGER_PREPARE:
		if (do_tx) {
			stream_reset(dev, &data->tx);
		}
		if (do_rx) {
			stream_reset(dev, &data->rx);
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static DEVICE_API(i2s, mcasp_driver_api) = {
	.configure = mcasp_configure,
	.config_get = mcasp_config_get,
	.read = mcasp_read_block,
	.write = mcasp_write_block,
	.trigger = mcasp_trigger,
};

static int mcasp_init(const struct device *dev)
{
	const struct mcasp_config *cfg = dev->config;
	struct mcasp_data *data = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	device_map(&data->dat, cfg->dat_phys, cfg->dat_size, K_MEM_CACHE_NONE);

	for (uint8_t n = 0U; n < cfg->serial_dir_len; n++) {
		if (cfg->serial_dir[n] == MCASP_SERDIR_TX) {
			data->tx_ser[data->num_tx] = n;
			data->num_tx++;
		} else if (cfg->serial_dir[n] == MCASP_SERDIR_RX) {
			data->rx_ser[data->num_rx] = n;
			data->num_rx++;
		}
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}
	ret = clock_control_on(cfg->clk_dev, cfg->clk_id);
	if (ret < 0 && ret != -EALREADY && ret != -ENOSYS) {
		LOG_ERR("AUXCLK on: %d", ret);
		return ret;
	}
	ret = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &data->fck_rate);
	if (ret < 0) {
		data->fck_rate = 0U;
	}

	mcasp_write(dev, MCASP_GBLCTL, 0U);
	mcasp_write(dev, MCASP_PDIR, 0U);
	mcasp_write(dev, MCASP_XINTCTL, 0U);
	mcasp_write(dev, MCASP_RINTCTL, 0U);

	data->tx.state = I2S_STATE_NOT_READY;
	data->rx.state = I2S_STATE_NOT_READY;

	cfg->irq_config();

#ifdef CONFIG_I2S_TI_MCASP_DMA
	if (cfg->dma_dev == NULL) {
		LOG_WRN("no DMA controller, using interrupts");
	} else if (!device_is_ready(cfg->dma_dev)) {
		LOG_WRN("DMA controller not ready, using interrupts");
	} else {
		data->dma_ok = true;
		LOG_DBG("DMA %s, TX chan %u, RX chan %u", cfg->dma_dev->name, cfg->dma_ch[0],
			cfg->dma_ch[1]);
	}
#endif

	LOG_DBG("PID 0x%08x, AUXCLK %u Hz, %u TX / %u RX serializers", mcasp_read(dev, MCASP_PID),
		data->fck_rate, data->num_tx, data->num_rx);

	return 0;
}

#ifdef CONFIG_I2S_TI_MCASP_DMA
#define MCASP_DMA_CELL(n, name, cell) DT_INST_DMAS_CELL_BY_NAME(n, name, cell)
#define MCASP_DMA_INIT(n)                                                                          \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),                                                \
		    (.dma_dev = DEVICE_DT_GET_OR_NULL(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),           \
		     .dma_ch = {MCASP_DMA_CELL(n, tx, channel_id),                                 \
				MCASP_DMA_CELL(n, rx, channel_id)},                                \
		     .dma_asel = {MCASP_DMA_CELL(n, tx, asel), MCASP_DMA_CELL(n, rx, asel)},),     \
		    (.dma_dev = NULL,))
#define MCASP_DMA_CHECK(n)                                                                         \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, dmas),                                                 \
		   (BUILD_ASSERT(DT_SAME_NODE(DT_INST_DMAS_CTLR_BY_NAME(n, tx),                    \
					      DT_INST_DMAS_CTLR_BY_NAME(n, rx)),                   \
				 "McASP tx and rx DMA channels must share a controller");))
#else
#define MCASP_DMA_INIT(n)
#define MCASP_DMA_CHECK(n)
#endif

#define MCASP_INIT(n)                                                                              \
	MCASP_DMA_CHECK(n)                                                                         \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, serial_dir) <= MCASP_MAX_SER,                             \
		     "serial-dir has more than 16 entries");                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	K_MSGQ_DEFINE(mcasp_txq_##n, sizeof(struct mcasp_block),                                   \
		      CONFIG_I2S_TI_MCASP_TX_BLOCK_COUNT, 4);                                      \
	K_MSGQ_DEFINE(mcasp_rxq_##n, sizeof(struct mcasp_block),                                   \
		      CONFIG_I2S_TI_MCASP_RX_BLOCK_COUNT, 4);                                      \
	static const uint8_t mcasp_serial_dir_##n[] = DT_INST_PROP(n, serial_dir);                 \
	static void mcasp_irq_config_##n(void)                                                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, tx, irq), DT_INST_IRQ_BY_NAME(n, tx, priority), \
			    mcasp_tx_isr, DEVICE_DT_INST_GET(n),                                   \
			    DT_INST_IRQ_BY_NAME(n, tx, flags));                                    \
		irq_enable(DT_INST_IRQ_BY_NAME(n, tx, irq));                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, rx, irq), DT_INST_IRQ_BY_NAME(n, rx, priority), \
			    mcasp_rx_isr, DEVICE_DT_INST_GET(n),                                   \
			    DT_INST_IRQ_BY_NAME(n, rx, flags));                                    \
		irq_enable(DT_INST_IRQ_BY_NAME(n, rx, irq));                                       \
	}                                                                                          \
	static const struct mcasp_config mcasp_config_##n = {                                      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.dat_phys = DT_INST_REG_ADDR_BY_NAME(n, dat),                                      \
		.dat_size = DT_INST_REG_SIZE_BY_NAME(n, dat),                                      \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                  \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(n, clocks, 0, name),          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config = mcasp_irq_config_##n,                                                \
		.serial_dir = mcasp_serial_dir_##n,                                                \
		.serial_dir_len = ARRAY_SIZE(mcasp_serial_dir_##n),                                \
		MCASP_DMA_INIT(n)};                                                                \
	static struct mcasp_data mcasp_data_##n = {                                                \
		.tx = {.msgq = &mcasp_txq_##n},                                                    \
		.rx = {.msgq = &mcasp_rxq_##n},                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mcasp_init, NULL, &mcasp_data_##n, &mcasp_config_##n,             \
			      POST_KERNEL, CONFIG_I2S_INIT_PRIORITY, &mcasp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCASP_INIT)
