/*
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_qmspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_xec, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/* Device constant configuration parameters */
struct spi_qmspi_config {
	QMSPI_Type *regs;
	uint32_t cs_timing;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t girq_nvic_aggr;
	uint8_t girq_nvic_direct;
	uint8_t irq_pri;
	uint8_t chip_sel;
	uint8_t width;	/* 1(single), 2(dual), 4(quad) */
	uint8_t unused;
	const struct pinctrl_dev_config *pcfg;
};

/* Device run time data */
struct spi_qmspi_data {
	struct spi_context ctx;
};

static inline uint32_t descr_rd(QMSPI_Type *regs, uint32_t did)
{
	uintptr_t raddr = (uintptr_t)regs + MCHP_QMSPI_DESC0_OFS +
			  ((did & MCHP_QMSPI_C_NEXT_DESCR_MASK0) << 2);

	return REG32(raddr);
}

static inline void descr_wr(QMSPI_Type *regs, uint32_t did, uint32_t val)
{
	uintptr_t raddr = (uintptr_t)regs + MCHP_QMSPI_DESC0_OFS +
			  ((did & MCHP_QMSPI_C_NEXT_DESCR_MASK0) << 2);

	REG32(raddr) = val;
}

static inline void txb_wr8(QMSPI_Type *regs, uint8_t data8)
{
	REG8(&regs->TX_FIFO) = data8;
}

static inline uint8_t rxb_rd8(QMSPI_Type *regs)
{
	return REG8(&regs->RX_FIFO);
}

/*
 * Program QMSPI frequency.
 * MEC1501 base frequency is 48MHz. QMSPI frequency divider field in the
 * mode register is defined as: 0=maximum divider of 256. Values 1 through
 * 255 divide 48MHz by that value.
 */
static void qmspi_set_frequency(QMSPI_Type *regs, uint32_t freq_hz)
{
	uint32_t div, qmode;

	if (freq_hz == 0) {
		div = 0; /* max divider = 256 */
	} else {
		div = MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ / freq_hz;
		if (div == 0) {
			div = 1; /* max freq. divider = 1 */
		} else if (div > 0xffu) {
			div = 0u; /* max divider = 256 */
		}
	}

	qmode = regs->MODE & ~(MCHP_QMSPI_M_FDIV_MASK);
	qmode |= (div << MCHP_QMSPI_M_FDIV_POS) & MCHP_QMSPI_M_FDIV_MASK;
	regs->MODE = qmode;
}

/*
 * SPI signalling mode: CPOL and CPHA
 * CPOL = 0 is clock idles low, 1 is clock idle high
 * CPHA = 0 Transmitter changes data on trailing of preceding clock cycle.
 *          Receiver samples data on leading edge of clock cycle.
 *        1 Transmitter changes data on leading edge of current clock cycle.
 *          Receiver samples data on the trailing edge of clock cycle.
 * SPI Mode nomenclature:
 * Mode CPOL CPHA
 *  0     0    0
 *  1     0    1
 *  2     1    0
 *  3     1    1
 * MEC1501 has three controls, CPOL, CPHA for output and CPHA for input.
 * SPI frequency < 48MHz
 *	Mode 0: CPOL=0 CHPA=0 (CHPA_MISO=0 and CHPA_MOSI=0)
 *	Mode 3: CPOL=1 CHPA=1 (CHPA_MISO=1 and CHPA_MOSI=1)
 * Data sheet recommends when QMSPI set at max. SPI frequency (48MHz).
 * SPI frequency == 48MHz sample and change data on same edge.
 *  Mode 0: CPOL=0 CHPA=0 (CHPA_MISO=1 and CHPA_MOSI=0)
 *  Mode 3: CPOL=1 CHPA=1 (CHPA_MISO=0 and CHPA_MOSI=1)
 */

const uint8_t smode_tbl[4] = {
	0x00u, 0x06u, 0x01u, 0x07u
};

const uint8_t smode48_tbl[4] = {
	0x04u, 0x02u, 0x05u, 0x03u
};

static void qmspi_set_signalling_mode(QMSPI_Type *regs, uint32_t smode)
{
	const uint8_t *ptbl;
	uint32_t m;

	ptbl = smode_tbl;
	if (((regs->MODE >> MCHP_QMSPI_M_FDIV_POS) &
	    MCHP_QMSPI_M_FDIV_MASK0) == 1) {
		ptbl = smode48_tbl;
	}

	m = (uint32_t)ptbl[smode & 0x03];
	regs->MODE = (regs->MODE & ~(MCHP_QMSPI_M_SIG_MASK))
		     | (m << MCHP_QMSPI_M_SIG_POS);
}

/*
 * QMSPI HW support single, dual, and quad.
 * Return QMSPI Control/Descriptor register encoded value.
 */
static uint32_t qmspi_config_get_lines(const struct spi_config *config)
{
#ifdef CONFIG_SPI_EXTENDED_MODES
	uint32_t qlines;

	switch (config->operation & SPI_LINES_MASK) {
	case SPI_LINES_SINGLE:
		qlines = MCHP_QMSPI_C_IFM_1X;
		break;
#if DT_INST_PROP(0, lines) > 1
	case SPI_LINES_DUAL:
		qlines = MCHP_QMSPI_C_IFM_2X;
		break;
#endif
#if DT_INST_PROP(0, lines) > 2
	case SPI_LINES_QUAD:
		qlines = MCHP_QMSPI_C_IFM_4X;
		break;
#endif
	default:
		qlines = 0xffu;
	}

	return qlines;
#else
	return MCHP_QMSPI_C_IFM_1X;
#endif
}

/*
 * Configure QMSPI.
 * NOTE: QMSPI can control two chip selects. At this time we use CS0# only.
 */
static int qmspi_configure(const struct device *dev,
			   const struct spi_config *config)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *data = dev->data;
	QMSPI_Type *regs = cfg->regs;
	uint32_t smode;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		return -ENOTSUP;
	}

	if (config->operation & (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE
				 | SPI_MODE_LOOP)) {
		return -ENOTSUP;
	}

	smode = qmspi_config_get_lines(config);
	if (smode == 0xff) {
		return -ENOTSUP;
	}

	regs->CTRL = smode;

	/* Use the requested or next highest possible frequency */
	qmspi_set_frequency(regs, config->frequency);

	smode = 0;
	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		smode |= (1ul << 0);
	}

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		smode |= (1ul << 1);
	}

	qmspi_set_signalling_mode(regs, smode);

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	}

	/* chip select */
	smode = regs->MODE & ~(MCHP_QMSPI_M_CS_MASK);
#if DT_INST_PROP(0, chip_select) == 0
	smode |= MCHP_QMSPI_M_CS0;
#else
	smode |= MCHP_QMSPI_M_CS1;
#endif
	regs->MODE = smode;

	/* chip select timing */
	regs->CSTM = cfg->cs_timing;

	data->ctx.config = config;

	regs->MODE |= MCHP_QMSPI_M_ACTIVATE;

	return 0;
}

/*
 * Transmit dummy clocks - QMSPI will generate requested number of
 * SPI clocks with I/O pins tri-stated.
 * Single mode: 1 bit per clock -> IFM field = 00b. Max 0x7fff clocks
 * Dual mode: 2 bits per clock  -> IFM field = 01b. Max 0x3fff clocks
 * Quad mode: 4 bits per clock  -> IFM field = 1xb. Max 0x1fff clocks
 * QMSPI unit size set to bits.
 */
static int qmspi_tx_dummy_clocks(QMSPI_Type *regs, uint32_t nclocks)
{
	uint32_t descr, ifm, qstatus;

	ifm = regs->CTRL & MCHP_QMSPI_C_IFM_MASK;
	descr = ifm | MCHP_QMSPI_C_TX_DIS | MCHP_QMSPI_C_XFR_UNITS_BITS
		| MCHP_QMSPI_C_DESCR_LAST | MCHP_QMSPI_C_DESCR0;

	if (ifm & 0x01) {
		nclocks <<= 1;
	} else if (ifm & 0x02) {
		nclocks <<= 2;
	}
	descr |= (nclocks << MCHP_QMSPI_C_XFR_NUNITS_POS);

	descr_wr(regs, 0, descr);

	regs->CTRL |= MCHP_QMSPI_C_DESCR_EN;
	regs->IEN = 0;
	regs->STS = 0xfffffffful;

	regs->EXE = MCHP_QMSPI_EXE_START;
	do {
		qstatus = regs->STS;
		if (qstatus & MCHP_QMSPI_STS_PROG_ERR) {
			return -EIO;
		}
	} while ((qstatus & MCHP_QMSPI_STS_DONE) == 0);

	return 0;
}

/*
 * Return unit size power of 2 given number of bytes to transfer.
 */
static uint32_t qlen_shift(uint32_t len)
{
	uint32_t ushift;

	/* is len a multiple of 4 or 16? */
	if ((len & 0x0F) == 0) {
		ushift = 4;
	} else if ((len & 0x03) == 0) {
		ushift = 2;
	} else {
		ushift = 0;
	}

	return ushift;
}

/*
 * Return QMSPI unit size of the number of units field in QMSPI
 * control/descriptor register.
 * Input: power of 2 unit size 4, 2, or 0(default) corresponding
 * to 16, 4, or 1 byte units.
 */
static uint32_t get_qunits(uint32_t qshift)
{
	if (qshift == 4) {
		return MCHP_QMSPI_C_XFR_UNITS_16;
	} else if (qshift == 2) {
		return MCHP_QMSPI_C_XFR_UNITS_4;
	} else {
		return MCHP_QMSPI_C_XFR_UNITS_1;
	}
}

/*
 * Allocate(build) one or more descriptors.
 * QMSPI contains 16 32-bit descriptor registers used as a linked
 * list of operations. Using only 32-bits there are limitations.
 * Each descriptor is limited to 0x7FFF units where unit size can
 * be 1, 4, or 16 bytes. A descriptor can perform transmit or receive
 * but not both simultaneously. Order of descriptor processing is specified
 * by the first descriptor field of the control register, the next descriptor
 * fields in each descriptor, and the descriptors last flag.
 */
static int qmspi_descr_alloc(QMSPI_Type *regs, const struct spi_buf *txb,
			     int didx, bool is_tx)
{
	uint32_t descr, qshift, n, nu;
	int dn;

	if (didx >= MCHP_QMSPI_MAX_DESCR) {
		return -EAGAIN;
	}

	if (txb->len == 0) {
		return didx; /* nothing to do */
	}

	/* b[1:0] IFM and b[3:2] transmit mode */
	descr = (regs->CTRL & MCHP_QMSPI_C_IFM_MASK);
	if (is_tx) {
		descr |= MCHP_QMSPI_C_TX_DATA;
	} else {
		descr |= MCHP_QMSPI_C_RX_EN;
	}

	/* b[11:10] unit size 1, 4, or 16 bytes */
	qshift = qlen_shift(txb->len);
	nu = txb->len >> qshift;
	descr |= get_qunits(qshift);

	do {
		descr &= 0x0FFFul;

		dn = didx + 1;
		/* b[15:12] next descriptor pointer */
		descr |= ((dn & MCHP_QMSPI_C_NEXT_DESCR_MASK0) <<
			  MCHP_QMSPI_C_NEXT_DESCR_POS);

		n = nu;
		if (n > MCHP_QMSPI_C_MAX_UNITS) {
			n = MCHP_QMSPI_C_MAX_UNITS;
		}

		descr |= (n << MCHP_QMSPI_C_XFR_NUNITS_POS);
		descr_wr(regs, didx, descr);

		if (dn < MCHP_QMSPI_MAX_DESCR) {
			didx++;
		} else {
			return -EAGAIN;
		}

		nu -= n;
	} while (nu);

	return dn;
}

static int qmspi_tx(QMSPI_Type *regs, const struct spi_buf *tx_buf,
		    bool close)
{
	const uint8_t *p = tx_buf->buf;
	size_t tlen = tx_buf->len;
	uint32_t descr;
	int didx;

	if (tlen == 0) {
		return 0;
	}

	/* Buffer pointer is NULL and number of bytes != 0 ? */
	if (p == NULL) {
		return qmspi_tx_dummy_clocks(regs, tlen);
	}

	didx = qmspi_descr_alloc(regs, tx_buf, 0, true);
	if (didx < 0) {
		return didx;
	}

	/* didx points to last allocated descriptor + 1 */
	__ASSERT(didx > 0, "QMSPI descriptor index=%d expected > 0\n", didx);
	didx--;

	descr = descr_rd(regs, didx) | MCHP_QMSPI_C_DESCR_LAST;
	if (close) {
		descr |= MCHP_QMSPI_C_CLOSE;
	}
	descr_wr(regs, didx, descr);

	regs->CTRL = (regs->CTRL & MCHP_QMSPI_C_IFM_MASK) |
		     MCHP_QMSPI_C_DESCR_EN | MCHP_QMSPI_C_DESCR0;
	regs->IEN = 0;
	regs->STS = 0xfffffffful;

	/* preload TX_FIFO */
	while (tlen) {
		tlen--;
		txb_wr8(regs, *p);
		p++;

		if (regs->STS & MCHP_QMSPI_STS_TXBF_RO) {
			break;
		}
	}

	regs->EXE = MCHP_QMSPI_EXE_START;

	if (regs->STS & MCHP_QMSPI_STS_PROG_ERR) {
		return -EIO;
	}

	while (tlen) {

		while (regs->STS & MCHP_QMSPI_STS_TXBF_RO) {
		}

		txb_wr8(regs, *p);
		p++;
		tlen--;
	}

	/* Wait for TX FIFO to drain and last byte to be clocked out */
	for (;;) {
		if (regs->STS & MCHP_QMSPI_STS_DONE) {
			break;
		}
	}

	return 0;
}

static int qmspi_rx(QMSPI_Type *regs, const struct spi_buf *rx_buf,
		    bool close)
{
	uint8_t *p = rx_buf->buf;
	size_t rlen = rx_buf->len;
	uint32_t descr;
	int didx;
	uint8_t data_byte;

	if (rlen == 0) {
		return 0;
	}

	didx = qmspi_descr_alloc(regs, rx_buf, 0, false);
	if (didx < 0) {
		return didx;
	}

	/* didx points to last allocated descriptor + 1 */
	__ASSERT_NO_MSG(didx > 0);
	didx--;

	descr = descr_rd(regs, didx) | MCHP_QMSPI_C_DESCR_LAST;
	if (close) {
		descr |= MCHP_QMSPI_C_CLOSE;
	}
	descr_wr(regs, didx, descr);

	regs->CTRL = (regs->CTRL & MCHP_QMSPI_C_IFM_MASK)
		     | MCHP_QMSPI_C_DESCR_EN | MCHP_QMSPI_C_DESCR0;
	regs->IEN = 0;
	regs->STS = 0xfffffffful;

	/*
	 * Trigger read based on the descriptor(s) programmed above.
	 * QMSPI will generate clocks until the RX FIFO is filled.
	 * More clocks will be generated as we pull bytes from the RX FIFO.
	 * QMSPI Programming error will be triggered after start if
	 * descriptors were programmed options that cannot be enabled
	 * simultaneously.
	 */
	regs->EXE = MCHP_QMSPI_EXE_START;
	if (regs->STS & MCHP_QMSPI_STS_PROG_ERR) {
		return -EIO;
	}

	while (rlen) {
		if (!(regs->STS & MCHP_QMSPI_STS_RXBE_RO)) {
			data_byte = rxb_rd8(regs);
			if (p != NULL) {
				*p++ = data_byte;
			}
			rlen--;
		}
	}

	return 0;
}

static int qmspi_transceive(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs,
			    const struct spi_buf_set *rx_bufs)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *data = dev->data;
	QMSPI_Type *regs = cfg->regs;
	const struct spi_buf *ptx;
	const struct spi_buf *prx;
	size_t nb;
	uint32_t descr, last_didx;
	int err;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	err = qmspi_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	spi_context_cs_control(&data->ctx, true);

	if (tx_bufs != NULL) {
		ptx = tx_bufs->buffers;
		nb = tx_bufs->count;
		while (nb--) {
			err = qmspi_tx(regs, ptx, false);
			if (err != 0) {
				goto done;
			}
			ptx++;
		}
	}

	if (rx_bufs != NULL) {
		prx = rx_bufs->buffers;
		nb = rx_bufs->count;
		while (nb--) {
			err = qmspi_rx(regs, prx, false);
			if (err != 0) {
				goto done;
			}
			prx++;
		}
	}

	/*
	 * If caller doesn't need CS# held asserted then find the last
	 * descriptor, set its close flag, and set stop.
	 */
	if (!(config->operation & SPI_HOLD_ON_CS)) {
		/* Get last descriptor from status register */
		last_didx = (regs->STS >> MCHP_QMSPI_C_NEXT_DESCR_POS)
			    & MCHP_QMSPI_C_NEXT_DESCR_MASK0;
		descr = descr_rd(regs, last_didx) | MCHP_QMSPI_C_CLOSE;
		descr_wr(regs, last_didx, descr);
		regs->EXE = MCHP_QMSPI_EXE_STOP;
	}

	spi_context_cs_control(&data->ctx, false);

done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int qmspi_transceive_sync(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	return qmspi_transceive(dev, config, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC
static int qmspi_transceive_async(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs,
				  struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif

static int qmspi_release(const struct device *dev,
			 const struct spi_config *config)
{
	struct spi_qmspi_data *data = dev->data;
	const struct spi_qmspi_config *cfg = dev->config;
	QMSPI_Type *regs = cfg->regs;

	/* Force CS# to de-assert on next unit boundary */
	regs->EXE = MCHP_QMSPI_EXE_STOP;

	while (regs->STS & MCHP_QMSPI_STS_ACTIVE_RO) {
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/*
 * Initialize QMSPI controller.
 * Disable sleep control.
 * Disable and clear interrupt status.
 * Initialize SPI context.
 * QMSPI will be configured and enabled when the transceive API is called.
 */
static int qmspi_init(const struct device *dev)
{
	int err;
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *data = dev->data;
	QMSPI_Type *regs = cfg->regs;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("QSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	mchp_pcr_periph_slp_ctrl(PCR_QMSPI, MCHP_PCR_SLEEP_DIS);

	regs->MODE = MCHP_QMSPI_M_SRST;

	MCHP_GIRQ_CLR_EN(cfg->girq, cfg->girq_pos);
	MCHP_GIRQ_SRC_CLR(cfg->girq, cfg->girq_pos);

	MCHP_GIRQ_BLK_CLREN(cfg->girq);
	NVIC_ClearPendingIRQ(cfg->girq_nvic_direct);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_qmspi_driver_api = {
	.transceive = qmspi_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = qmspi_transceive_async,
#endif
	.release = qmspi_release,
};


#define XEC_QMSPI_CS_TIMING_VAL(a, b, c, d) (((a) & 0xFu) \
					     | (((b) & 0xFu) << 8) \
					     | (((c) & 0xFu) << 16) \
					     | (((d) & 0xFu) << 24))


#define XEC_QMSPI_0_CS_TIMING XEC_QMSPI_CS_TIMING_VAL(			\
				DT_INST_PROP(0, dcsckon),		\
				DT_INST_PROP(0, dckcsoff),		\
				DT_INST_PROP(0, dldh),			\
				DT_INST_PROP(0, dcsda))

#if DT_NODE_HAS_STATUS(DT_INST(0, microchip_xec_qmspi), okay)

PINCTRL_DT_INST_DEFINE(0);

static const struct spi_qmspi_config spi_qmspi_0_config = {
	.regs = (QMSPI_Type *)DT_INST_REG_ADDR(0),
	.cs_timing = XEC_QMSPI_0_CS_TIMING,
	.girq = MCHP_QMSPI_GIRQ_NUM,
	.girq_pos = MCHP_QMSPI_GIRQ_POS,
	.girq_nvic_direct = MCHP_QMSPI_GIRQ_NVIC_DIRECT,
	.irq_pri = DT_INST_IRQ(0, priority),
	.chip_sel = DT_INST_PROP(0, chip_select),
	.width = DT_INST_PROP(0, lines),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct spi_qmspi_data spi_qmspi_0_dev_data = {
	SPI_CONTEXT_BASE_INIT(spi_qmspi_0_dev_data, ctx),
	SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(0), ctx)
};

DEVICE_DT_INST_DEFINE(0,
		    &qmspi_init, NULL, &spi_qmspi_0_dev_data,
		    &spi_qmspi_0_config, POST_KERNEL,
		    CONFIG_SPI_INIT_PRIORITY, &spi_qmspi_driver_api);

#endif /* DT_NODE_HAS_STATUS(DT_INST(0, microchip_xec_qmspi), okay) */
