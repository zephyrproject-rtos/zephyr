/*
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_qmspi_v2

#include <logging/log.h>
LOG_MODULE_REGISTER(spi_xec, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include <errno.h>
#include <device.h>
#include <drivers/clock_control/mchp_xec_clock_control.h>
#include <drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <drivers/spi.h>
#include <soc.h>
#include <sys/sys_io.h>
#include <sys/util.h>

#define XEC_QMSPI_ADJ_SIG_FREQ		48000000

/* microseconds for busy wait and total wait interval */
#define XEC_QMSPI_WAIT_INTERVAL		16
#define XEC_QMSPI_WAIT_COUNT		16
#define XEC_QMSPI_WAIT_FULL_FIFO	256

/* Device constant configuration parameters */
struct spi_qmspi_config {
	uintptr_t base;
	uint32_t cs_timing;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t girq_nvic_aggr;
	uint8_t girq_nvic_direct;
	uint8_t irq_pri;
	uint8_t pcr_idx;
	uint8_t pcr_bitpos;
	uint8_t chip_sel;
	uint8_t width;	/* 1(single), 2(dual), 4(quad) */
};

/* Device run time data */
struct spi_qmspi_data {
	struct spi_context ctx;
};

static int xec_qmspi_spin_yield(int *counter)
{
	*counter = *counter + 1;

	if (*counter > XEC_QMSPI_WAIT_COUNT) {
		return -ETIMEDOUT;
	}

	k_busy_wait(XEC_QMSPI_WAIT_INTERVAL);

	return 0;
}

/*
 * MEC172x Boot-ROM programs QMSPI timing taps registers based on OTP settings.
 * Save/restore timing taps on soft-reset of controller.
 */
static void qmspi_reset_dev(const struct device *dev)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct qmspi_regs *regs = (struct qmspi_regs *)cfg->base;
	uint32_t taps[3];

	taps[0] = regs->TM_TAPS;
	taps[1] = regs->TM_TAPS_ADJ;
	taps[2] = regs->TM_TAPS_CTRL;

	/* soft reset, self-clearing */
	regs->MODE = MCHP_QMSPI_M_SRST;
	/* force some delay */
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;

	regs->MODE = 0;
	regs->TM_TAPS = taps[0];
	regs->TM_TAPS_ADJ = taps[1];
	regs->TM_TAPS_CTRL = taps[2];
}

/*
 * Program QMSPI frequency.
 * MEC172x QMSPI SPI base clock is 96MHz. MEC152x is 48MHz.
 * QMSPI frequency divider field in the mode register is defined as:
 * 0=maximum divider of 256. Values 1 through 255 divide base clock
 * by that value.
 */
static void qmspi_set_frequency(struct qmspi_regs *regs, uint32_t freq_hz)
{
	uint32_t qfdiv, qmode;

	if (freq_hz == 0) {
		qfdiv = 0; /* max divider = 256 */
	} else {
		qfdiv = MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ / freq_hz;
		if (qfdiv == 0) {
			qfdiv = 1; /* max freq. divider = 1 */
		} else if (qfdiv > 255) {
			qfdiv = 0u; /* max divider = 256 */
		}
	}

	qmode = regs->MODE & ~(MCHP_QMSPI_M_FDIV_MASK);
	qmode |= (qfdiv << MCHP_QMSPI_M_FDIV_POS) & MCHP_QMSPI_M_FDIV_MASK;
	regs->MODE = qmode;
}

static uint32_t qmspi_get_freq_hz(struct qmspi_regs *regs)
{
	uint32_t qdiv;

	qdiv = (regs->MODE >> MCHP_QMSPI_M_FDIV_POS) & MCHP_QMSPI_M_FDIV_MASK0;
	if (qdiv == 0) {
		qdiv = 256;
	}

	return (MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ / qdiv);
}

/*
 * SPI signalling mode: CPOL and CPHA
 * CPOL = 0 is clock idles low, 1 is clock idle high
 * CPHA = 0 Transmitter changes data on trailing of preceding clock cycle.
 *          Receiver samples data on leading edge of clock cyle.
 *        1 Transmitter changes data on leading edge of current clock cycle.
 *          Receiver samples data on the trailing edge of clock cycle.
 * SPI Mode nomenclature:
 * Mode CPOL CPHA
 *  0     0    0
 *  1     0    1
 *  2     1    0
 *  3     1    1
 * QMSPI has three bits: CPOL, CPHA_MOSI for output and CPHA_MISO for input.
 * SPI frequency < 48MHz
 *	Mode 0: CPOL=0 CHPA=0 (CHPA_MISO=0 and CHPA_MOSI=0)
 *	Mode 3: CPOL=1 CHPA=1 (CHPA_MISO=1 and CHPA_MOSI=1)
 * For frequencies >= 48MHz data sheet recommends:
 * SPI frequency >= 48MHz sample and change data on same edge.
 *  Mode 0: CPOL=0 CHPA=0 (CHPA_MISO=1 and CHPA_MOSI=0)
 *  Mode 3: CPOL=1 CHPA=1 (CHPA_MISO=0 and CHPA_MOSI=1)
 */

const uint8_t smode_tbl[4] = {
	0x00u, 0x06u, 0x01u, 0x07u
};

/* CPOL, CPHA_MOSI, and CPHA_MISO for frequencies >= XEC_QMSPI_ADJ_SIG_FREQ */
const uint8_t smode_adj_tbl[4] = {
	0x04u, 0x02u, 0x05u, 0x03u
};

static void qmspi_set_signalling_mode(struct qmspi_regs *regs, uint32_t smode)
{
	const uint8_t *ptbl = smode_tbl;
	uint32_t m, freqhz;

	freqhz = qmspi_get_freq_hz(regs);
	if (freqhz >= XEC_QMSPI_ADJ_SIG_FREQ) {
		ptbl = smode_adj_tbl;
	}

	m = (uint32_t)ptbl[smode & 0x03];
	regs->MODE = (regs->MODE & ~(MCHP_QMSPI_M_SIG_MASK))
		     | (m << MCHP_QMSPI_M_SIG_POS);
}

/*
 * QMSPI HW supports single, dual, and quad.
 * Return QMSPI Control/Descriptor register encoded value.
 */
static uint32_t qmspi_config_get_lines(const struct spi_config *config)
{
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
}

/* Configure QMSPI */
static int qmspi_configure(const struct device *dev,
			   const struct spi_config *config)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct qmspi_regs *regs = (struct qmspi_regs *)cfg->base;
	struct spi_qmspi_data *data = dev->data;
	uint32_t smode;

	/* reset controller on any error or data left in FIFOs */
	if ((regs->STS & (MCHP_QMSPI_STS_TXB_ERR | MCHP_QMSPI_STS_RXB_ERR |
			 MCHP_QMSPI_STS_PROG_ERR)) || (regs->BCNT_STS)) {
		qmspi_reset_dev(dev);
	} else if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (config->operation & (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE
				 | SPI_MODE_LOOP)) {
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
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

	/* Add driver specific data to SPI context structure */
	spi_context_cs_configure(&data->ctx);

	regs->MODE |= MCHP_QMSPI_M_ACTIVATE;

	return 0;
}

/*
 * Transmit dummy clocks - QMSPI will generate requested number of
 * SPI clocks with I/O pins tri-stated.
 * Single mode: 1 bit per clock -> IFM field = 00b. Max 0x7fff clocks
 * Dual mode: 2 bits per clock  -> IFM field = 01b. Max 0x3fff clocks
 * Quad mode: 4 bits per clock  -> IFM fiels = 1xb. Max 0x1fff clocks
 * QMSPI unit size set to bits.
 */
static int qmspi_tx_dummy_clocks(struct qmspi_regs *regs, uint32_t nclocks)
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

	regs->DESCR[0] = descr;

	regs->CTRL |= MCHP_QMSPI_C_DESCR_EN;
	regs->IEN = 0;
	regs->STS = 0xfffffffful;

	regs->EXE = MCHP_QMSPI_EXE_START;
	/* TODO timeout */
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
static int qmspi_descr_alloc(struct qmspi_regs *regs, const struct spi_buf *txb,
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
		regs->DESCR[didx] = descr;

		if (dn < MCHP_QMSPI_MAX_DESCR) {
			didx++;
		} else {
			return -EAGAIN;
		}

		nu -= n;
	} while (nu);

	return dn;
}

static int qmspi_tx(struct qmspi_regs *regs, const struct spi_buf *tx_buf,
		    bool close)
{
	const uint8_t *p = tx_buf->buf;
	size_t tlen = tx_buf->len;
	uint32_t descr, count;
	int didx, ret;

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
	__ASSERT_NO_MSG(didx > 0);
	didx--;

	descr = regs->DESCR[didx] | MCHP_QMSPI_C_DESCR_LAST;
	if (close) {
		descr |= MCHP_QMSPI_C_CLOSE;
	}
	regs->DESCR[didx] = descr;

	regs->CTRL = (regs->CTRL & MCHP_QMSPI_C_IFM_MASK) |
		     MCHP_QMSPI_C_DESCR_EN | MCHP_QMSPI_C_DESCR0;
	regs->IEN = 0;
	regs->STS = 0xfffffffful;

	/* preload TX_FIFO */
	while (tlen) {
		tlen--;
		sys_write8(*p, (mm_reg_t)&regs->TX_FIFO);
		p++;

		if (regs->STS & MCHP_QMSPI_STS_TXBF_RO) {
			break;
		}
	}

	regs->EXE = MCHP_QMSPI_EXE_START;

	if (regs->STS & MCHP_QMSPI_STS_PROG_ERR) {
		return -EIO;
	}

	count = 0;
	while (tlen) {
		if (!(regs->STS & MCHP_QMSPI_STS_TXBF_RO)) {
			sys_write8(*p, (mm_reg_t)&regs->TX_FIFO);
			p++;
			tlen--;
			count = 0;
		} else {
			ret = xec_qmspi_spin_yield(&count);
			if (ret < 0) {
				return ret;
			}
		}
	}

	/* Wait for TX FIFO to drain and last byte to be clocked out */
	count = XEC_QMSPI_WAIT_FULL_FIFO;
	do {
		if (regs->STS & MCHP_QMSPI_STS_DONE) {
			return 0;
		}

		k_busy_wait(XEC_QMSPI_WAIT_INTERVAL);
	} while (count--);

	return -ETIMEDOUT;
}

static int qmspi_rx(struct qmspi_regs *regs, const struct spi_buf *rx_buf,
		    bool close)
{
	uint8_t *p = rx_buf->buf;
	size_t rlen = rx_buf->len;
	uint32_t descr, count;
	int didx, ret;
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

	descr = regs->DESCR[didx] | MCHP_QMSPI_C_DESCR_LAST;
	if (close) {
		descr |= MCHP_QMSPI_C_CLOSE;
	}
	regs->DESCR[didx] = descr;

	regs->CTRL = (regs->CTRL & MCHP_QMSPI_C_IFM_MASK)
		     | MCHP_QMSPI_C_DESCR_EN | MCHP_QMSPI_C_DESCR0;
	regs->IEN = 0;
	regs->STS = 0xffffffffu;

	/*
	 * Trigger read based on the descriptor(s) programmed above.
	 * QMSPI will generate clocks until the RX FIFO is filled.
	 * More clocks will be generated as we pull bytes from the RX FIFO.
	 * QMSPI Programming error will be triggered after start if
	 * descriptors were programmed with options that cannot be enabled
	 * simultaneously.
	 */
	regs->EXE = MCHP_QMSPI_EXE_START;
	if (regs->STS & MCHP_QMSPI_STS_PROG_ERR) {
		return -EIO;
	}

	count = 0;
	while (rlen) {
		if (!(regs->STS & MCHP_QMSPI_STS_RXBE_RO)) {
			data_byte = sys_read8((mm_reg_t)&regs->RX_FIFO);
			if (p != NULL) {
				*p++ = data_byte;
			}
			rlen--;
			count = 0;
		} else {
			ret = xec_qmspi_spin_yield(&count);
			if (ret < 0) {
				return ret;
			}
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
	struct qmspi_regs *regs = (struct qmspi_regs *)cfg->base;
	struct spi_qmspi_data *data = dev->data;
	const struct spi_buf *ptx;
	const struct spi_buf *prx;
	size_t nb;
	bool close, close_on_tx, close_on_rx;
	int err;

	spi_context_lock(&data->ctx, false, NULL, config);

	err = qmspi_configure(dev, config);
	if (err != 0) {
		goto done;
	}

	close_on_tx = false;
	close_on_rx = false;
	if (!(config->operation & SPI_HOLD_ON_CS)) {
		if (tx_bufs && tx_bufs->count) {
			close_on_tx = true;
		}
		if (rx_bufs && rx_bufs->count) {
			close_on_tx = false;
			close_on_rx = true;
		}
	}

	spi_context_cs_control(&data->ctx, true);

	close = false;
	if (tx_bufs != NULL) {
		for (nb = 1; nb <= tx_bufs->count; nb++) {
			if (close_on_tx && (nb == tx_bufs->count)) {
				close = true;
			}
			ptx = &tx_bufs->buffers[nb - 1];
			err = qmspi_tx(regs, ptx, close);
			if (err != 0) {
				goto done;
			}
		}
	}

	close = false;
	if (rx_bufs != NULL) {
		for (nb = 1; nb <= rx_bufs->count; nb++) {
			if (close_on_rx && (nb == rx_bufs->count)) {
				close = true;
			}
			prx = &rx_bufs->buffers[nb - 1];
			err = qmspi_rx(regs, prx, close);
			if (err != 0) {
				break;
			}
		}
	}

done:
	spi_context_cs_control(&data->ctx, false);

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
	struct qmspi_regs *regs = (struct qmspi_regs *)cfg->base;
	int ret = 0;
	uint32_t count = 0;

	/* Force CS# to de-assert on next unit boundary */
	regs->EXE = MCHP_QMSPI_EXE_STOP;

	while (regs->STS & MCHP_QMSPI_STS_ACTIVE_RO) {
		ret = xec_qmspi_spin_yield(&count);
		if (ret < 0) {
			break;
		}
	}

	/* clear the FIFOs */
	regs->EXE = MCHP_QMSPI_EXE_CLR_FIFOS;

	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
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
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *data = dev->data;

	z_mchp_xec_pcr_periph_sleep(cfg->pcr_idx, cfg->pcr_bitpos, 0);

	qmspi_reset_dev(dev);

	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);
	mchp_xec_ecia_girq_src_dis(cfg->girq, cfg->girq_pos);
	mchp_xec_ecia_nvic_clr_pend(cfg->girq_nvic_direct);

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

#if DT_NODE_HAS_STATUS(DT_INST(0, microchip_xec_qmspi_v2), okay)

static const struct spi_qmspi_config spi_qmspi_0_config = {
	.base = (uintptr_t) DT_INST_REG_ADDR(0),
	.cs_timing = XEC_QMSPI_0_CS_TIMING,
	.girq = DT_INST_PROP_BY_IDX(0, girqs, 0),
	.girq_pos = DT_INST_PROP_BY_IDX(0, girqs, 1),
	.girq_nvic_direct = DT_INST_IRQN(0),
	.irq_pri = DT_INST_IRQ(0, priority),
	.pcr_idx = DT_INST_PROP_BY_IDX(0, pcrs, 0),
	.pcr_bitpos = DT_INST_PROP_BY_IDX(0, pcrs, 1),
	.chip_sel = DT_INST_PROP(0, chip_select),
	.width = DT_INST_PROP(0, lines)
};

static struct spi_qmspi_data spi_qmspi_0_dev_data = {
	SPI_CONTEXT_INIT_LOCK(spi_qmspi_0_dev_data, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_qmspi_0_dev_data, ctx)
};

DEVICE_DT_INST_DEFINE(0,
		    &qmspi_init, NULL, &spi_qmspi_0_dev_data,
		    &spi_qmspi_0_config, POST_KERNEL,
		    CONFIG_SPI_INIT_PRIORITY, &spi_qmspi_driver_api);

#endif /* DT_NODE_HAS_STATUS(DT_INST(0, microchip_xec_qmspi), okay) */
