/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_qmspi_ldma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_xec, CONFIG_SPI_LOG_LEVEL);

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pinmux.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "spi_context.h"

/* #define XEC_QMSPI_DEBUG */
#ifdef XEC_QMSPI_DEBUG
#include <zephyr/sys/printk.h>
#endif


/* spin loops waiting for HW to clear soft reset bit */
#define XEC_QMSPI_SRST_LOOPS		16

/* microseconds for busy wait and total wait interval */
#define XEC_QMSPI_WAIT_INTERVAL		8
#define XEC_QMSPI_WAIT_COUNT		64
#define XEC_QMSPI_WAIT_FULL_FIFO	1024

/* QSPI hardware error status
 * Misprogrammed control or descriptors (software error)
 * Overflow TX FIFO
 * Underflow RX FIFO
 */
#define XEC_QSPI_HW_ERRORS		(MCHP_QMSPI_STS_PROG_ERR |	\
					 MCHP_QMSPI_STS_TXB_ERR |	\
					 MCHP_QMSPI_STS_RXB_ERR)

#define XEC_QSPI_HW_ERRORS_LDMA		(MCHP_QMSPI_STS_LDMA_RX_ERR |	\
					 MCHP_QMSPI_STS_LDMA_TX_ERR)

#define XEC_QSPI_HW_ERRORS_ALL		(XEC_QSPI_HW_ERRORS |		\
					 XEC_QSPI_HW_ERRORS_LDMA)

/*
 * Maximum number of units to generate clocks with data lines
 * tri-stated depends upon bus width. Maximum bus width is 4.
 */
#define XEC_QSPI_MAX_TSCLK_UNITS	(MCHP_QMSPI_C_MAX_UNITS / 4)

#define XEC_QMSPI_HALF_DUPLEX		0
#define XEC_QMSPI_FULL_DUPLEX		1
#define XEC_QMSPI_DUAL			2
#define XEC_QMSPI_QUAD			4

/* Device constant configuration parameters */
struct spi_qmspi_config {
	struct qmspi_regs *regs;
	uint32_t cs1_freq;
	uint32_t cs_timing;
	uint16_t taps_adj;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t girq_nvic_aggr;
	uint8_t girq_nvic_direct;
	uint8_t irq_pri;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	uint8_t port_sel;
	uint8_t chip_sel;
	uint8_t width;	/* 0(half) 1(single), 2(dual), 4(quad) */
	uint8_t unused[2];
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

#define XEC_QMSPI_XFR_FLAG_TX		BIT(0)
#define XEC_QMSPI_XFR_FLAG_STARTED	BIT(1)

/* Device run time data */
struct spi_qmspi_data {
	struct spi_context ctx;
	uint32_t qstatus;
	uint8_t np; /* number of data pins: 1, 2, or 4 */
	uint8_t *pd;
	uint32_t dlen;
	uint32_t consumed;
#ifdef CONFIG_SPI_ASYNC
	uint16_t xfr_flags;
	uint8_t ldma_chan;
	uint8_t in_isr;
	size_t xfr_len;
#endif
};

struct xec_qmspi_pin {
	const struct device *dev;
	uint8_t pin;
	uint32_t attrib;
};


static int xec_qmspi_spin_yield(int *counter, int max_count)
{
	*counter = *counter + 1;

	if (*counter > max_count) {
		return -ETIMEDOUT;
	}

	k_busy_wait(XEC_QMSPI_WAIT_INTERVAL);

	return 0;
}

/*
 * reset QMSPI controller with save/restore of timing registers.
 * Some QMSPI timing register may be modified by the Boot-ROM OTP
 * values.
 */
static void qmspi_reset(struct qmspi_regs *regs)
{
	uint32_t taps[3];
	uint32_t malt1;
	uint32_t cstm;
	uint32_t mode;
	uint32_t cnt = XEC_QMSPI_SRST_LOOPS;

	taps[0] = regs->TM_TAPS;
	taps[1] = regs->TM_TAPS_ADJ;
	taps[2] = regs->TM_TAPS_CTRL;
	malt1 = regs->MODE_ALT1;
	cstm = regs->CSTM;
	mode = regs->MODE;
	regs->MODE = MCHP_QMSPI_M_SRST;
	while (regs->MODE & MCHP_QMSPI_M_SRST) {
		if (cnt == 0) {
			break;
		}
		cnt--;
	}
	regs->MODE = 0;
	regs->MODE = mode & ~MCHP_QMSPI_M_ACTIVATE;
	regs->CSTM = cstm;
	regs->MODE_ALT1 = malt1;
	regs->TM_TAPS = taps[0];
	regs->TM_TAPS_ADJ = taps[1];
	regs->TM_TAPS_CTRL = taps[2];
}

/*
 * Calculate 8-bit QMSPI frequency divider register field value.
 * QMSPI input clock is MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ.
 * The 8-bit divider is encoded as:
 * 0 is divide by 256
 * 1 through 255 is divide by this value.
 */
static uint32_t qmspi_encoded_fdiv(uint32_t freq_hz)
{
	uint32_t fdiv = 1;

	if (freq_hz < (MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ / 256u)) {
		fdiv = 0; /* HW fdiv of 0 -> divide by 256 */
	} else if (freq_hz < MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ) {
		fdiv = MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ / freq_hz;
	}

	return fdiv;
}

/* Program QMSPI frequency divider field in mode register */
static void qmspi_set_frequency(struct qmspi_regs *regs, uint32_t freq_hz)
{
	uint32_t fdiv, qmode;

	fdiv = qmspi_encoded_fdiv(freq_hz);

	qmode = regs->MODE & ~(MCHP_QMSPI_M_FDIV_MASK);
	qmode |= (fdiv << MCHP_QMSPI_M_FDIV_POS) & MCHP_QMSPI_M_FDIV_MASK;
	regs->MODE = qmode;
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
 * QMSPI has three controls, CPOL, CPHA for output and CPHA for input.
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

static void qmspi_set_signalling_mode(struct qmspi_regs *regs, uint32_t smode)
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
static uint32_t encode_lines(const struct spi_config *config)
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

static uint8_t npins_from_spi_config(const struct spi_config *config)
{
	switch (config->operation & SPI_LINES_MASK) {
	case SPI_LINES_DUAL:
		return 2u;
	case SPI_LINES_QUAD:
		return 4u;
	default:
		return 1u;
	}
}

/*
 * Configure QMSPI.
 * NOTE: QMSPI Port 0 has two chip selects available. Ports 1 & 2
 * support only CS0#.
 */
static int qmspi_configure(const struct device *dev,
			   const struct spi_config *config)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct qmspi_regs *regs = cfg->regs;
	uint32_t smode;

	if (spi_context_configured(&qdata->ctx, config)) {
		return 0;
	}

	if (config->operation & (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE
				 | SPI_MODE_LOOP)) {
		return -ENOTSUP;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size != 8 not supported");
		return -ENOTSUP;
	}

	smode = encode_lines(config);
	if (smode == 0xff) {
		LOG_ERR("Requested lines mode not supported");
		return -ENOTSUP;
	}

	qdata->np = npins_from_spi_config(config);
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
	if (cfg->chip_sel == 0) {
		smode |= MCHP_QMSPI_M_CS0;
	} else {
		smode |= MCHP_QMSPI_M_CS1;
	}
	regs->MODE = smode;

	/* chip select timing and TAPS adjust */
	regs->CSTM = cfg->cs_timing;
	regs->TM_TAPS_ADJ = cfg->taps_adj;

	/* CS1 alternate mode (frequency) */
	regs->MODE_ALT1 = 0;
	if (cfg->cs1_freq) {
		uint32_t fdiv = qmspi_encoded_fdiv(cfg->cs1_freq);

		regs->MODE_ALT1 = (fdiv << MCHP_QMSPI_MA1_CS1_CDIV_POS) &
				  MCHP_QMSPI_MA1_CS1_CDIV_MSK;
		regs->MODE_ALT1 |= MCHP_QMSPI_MA1_CS1_CDIV_EN;
	}

	qdata->ctx.config = config;

	regs->MODE |= MCHP_QMSPI_M_ACTIVATE;

	return 0;
}

static uint32_t encode_npins(uint8_t npins)
{
	if (npins == 4) {
		return MCHP_QMSPI_C_IFM_4X;
	} else if (npins == 2) {
		return MCHP_QMSPI_C_IFM_2X;
	} else {
		return MCHP_QMSPI_C_IFM_1X;
	}
}

static int qmspi_tx_tsd_clks(struct qmspi_regs *regs, uint8_t npins,
			     uint32_t nclks, bool tx_close)
{
	uint32_t descr = 0;
	uint32_t nu = 0;
	uint32_t qsts = 0;
	int counter = 0;
	int ret = 0;

	LOG_DBG("Sync TSD CLKS: nclks = %u close = %d", nclks, tx_close);

	if (nclks == 0) {
		return 0;
	}

	regs->CTRL = MCHP_QMSPI_C_DESCR_EN | MCHP_QMSPI_C_DESCR(0);
	regs->EXE = MCHP_QMSPI_EXE_CLR_FIFOS;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;

	descr |= encode_npins(npins);
	descr |= MCHP_QMSPI_C_TX_DIS | MCHP_QMSPI_C_DESCR_LAST;

	/* number of clocks to generate */
	while (nclks) {
		nu = nclks;
		if (nu > XEC_QSPI_MAX_TSCLK_UNITS) {
			nu = XEC_QSPI_MAX_TSCLK_UNITS;
		}
		nclks -= nu;

		/* XEC_QSPI_MAX_TSCLK_UNITS guarantees no overflow
		 * for valid npins [1, 2, 4]
		 */
		nu *= npins;
		if (nu % 8) {
			descr |= MCHP_QMSPI_C_XFR_UNITS_BITS;
		} else { /* byte units */
			descr |= MCHP_QMSPI_C_XFR_UNITS_1;
			nu /= 8;
		}

		descr |= ((nu << MCHP_QMSPI_C_XFR_NUNITS_POS) &
			  MCHP_QMSPI_C_XFR_NUNITS_MASK);

		LOG_DBG("Sync TSD CLKS: descr = 0x%08x", descr);

		regs->DESCR[0] = descr;
		regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
		regs->EXE = MCHP_QMSPI_EXE_START;

		counter = 0;
		qsts = regs->STS;
		while ((qsts & (MCHP_QMSPI_STS_DONE |
				MCHP_QMSPI_STS_TXBE_RO)) !=
		       (MCHP_QMSPI_STS_DONE | MCHP_QMSPI_STS_TXBE_RO)) {
			if (qsts & (MCHP_QMSPI_STS_PROG_ERR |
					MCHP_QMSPI_STS_TXB_ERR)) {
				regs->EXE = MCHP_QMSPI_EXE_STOP;
				return -EIO;
			}
			ret =  xec_qmspi_spin_yield(&counter, XEC_QMSPI_WAIT_FULL_FIFO);
			if (ret != 0) {
				regs->EXE = MCHP_QMSPI_EXE_STOP;
				return ret;
			}
			qsts = regs->STS;
		}
	}

	return 0;
}

static int qmspi_tx(struct qmspi_regs *regs, uint8_t npins,
		    const struct spi_buf *tx_buf, bool close)
{
	const uint8_t *p = tx_buf->buf;
	uint32_t tlen = tx_buf->len;
	uint32_t nu = 0;
	uint32_t descr = 0;
	uint32_t qsts = 0;
	uint8_t data_byte = 0;
	int i = 0;
	int ret = 0;
	int counter = 0;

	LOG_DBG("Sync TX: p=%p len = %u close = %d", p, tlen, close);

	if (tlen == 0) {
		return 0;
	}

	regs->CTRL = MCHP_QMSPI_C_DESCR_EN | MCHP_QMSPI_C_DESCR(0);
	regs->EXE = MCHP_QMSPI_EXE_CLR_FIFOS;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;

	descr |= encode_npins(npins);
	descr |= MCHP_QMSPI_C_DESCR_LAST;

	if (p) {
		descr |= MCHP_QMSPI_C_TX_DATA | MCHP_QMSPI_C_XFR_UNITS_1;
	} else { /* length with no data is number of tri-state clocks */
		descr |= MCHP_QMSPI_C_XFR_UNITS_BITS;
		tlen *= npins;
		if ((tlen == 0) || (tlen > MCHP_QMSPI_C_MAX_UNITS)) {
			return -EDOM;
		}
	}

	while (tlen) {
		descr &= ~MCHP_QMSPI_C_XFR_NUNITS_MASK;

		nu = tlen;
		if (p && (nu > MCHP_QMSPI_TX_FIFO_LEN)) {
			nu = MCHP_QMSPI_TX_FIFO_LEN;
		}

		descr |= (nu << MCHP_QMSPI_C_XFR_NUNITS_POS) &
			 MCHP_QMSPI_C_XFR_NUNITS_MASK;

		tlen -= nu;
		if ((tlen == 0) && close) {
			descr |= MCHP_QMSPI_C_CLOSE;
		}

		LOG_DBG("Sync TX: descr=0x%08x", descr);

		regs->DESCR[0] = descr;

		if (p) {
			for (i = 0; i < nu; i++) {
				data_byte = *p++;
				LOG_DBG("Sync TX: TX FIFO 0x%02x", data_byte);
				sys_write8(data_byte,
					   (mm_reg_t)&regs->TX_FIFO);
			}
		}

		regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
		regs->EXE = MCHP_QMSPI_EXE_START;

		counter = 0;
		qsts = regs->STS;
		while ((qsts & (MCHP_QMSPI_STS_DONE |
				MCHP_QMSPI_STS_TXBE_RO)) !=
		       (MCHP_QMSPI_STS_DONE | MCHP_QMSPI_STS_TXBE_RO)) {
			if (qsts & (MCHP_QMSPI_STS_PROG_ERR |
					MCHP_QMSPI_STS_TXB_ERR)) {
				regs->EXE = MCHP_QMSPI_EXE_STOP;
				return -EIO;
			}
			ret =  xec_qmspi_spin_yield(&counter, XEC_QMSPI_WAIT_FULL_FIFO);
			if (ret != 0) {
				regs->EXE = MCHP_QMSPI_EXE_STOP;
				return ret;
			}
			qsts = regs->STS;
		}
	}

	return 0;
}

static int qmspi_rx(struct qmspi_regs *regs, uint8_t npins,
		    const struct spi_buf *rx_buf, bool close)
{
	uint8_t *p = rx_buf->buf;
	size_t rlen = rx_buf->len;
	uint32_t descr = 0;
	uint32_t nu = 0;
	uint32_t nrxb = 0;
	uint32_t qsts = 0;
	int ret = 0;
	int counter = 0;
	uint8_t data_byte = 0;
	uint8_t np = npins;

	LOG_DBG("Sync RX: len = %u close = %d", rlen, close);

	if (rlen == 0) {
		return 0;
	}

	descr |= encode_npins(np);
	descr |= MCHP_QMSPI_C_RX_EN | MCHP_QMSPI_C_XFR_UNITS_1 |
		 MCHP_QMSPI_C_DESCR_LAST;

	regs->CTRL = MCHP_QMSPI_C_DESCR_EN | MCHP_QMSPI_C_DESCR(0);
	regs->EXE = MCHP_QMSPI_EXE_CLR_FIFOS;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;

	while (rlen) {
		descr &= ~MCHP_QMSPI_C_XFR_NUNITS_MASK;

		nu = MCHP_QMSPI_RX_FIFO_LEN;
		if (rlen < MCHP_QMSPI_RX_FIFO_LEN) {
			nu = rlen;
		}

		descr |= (nu << MCHP_QMSPI_C_XFR_NUNITS_POS) &
			 MCHP_QMSPI_C_XFR_NUNITS_MASK;

		rlen -= nu;
		if ((rlen == 0) && close) {
			descr |= MCHP_QMSPI_C_CLOSE;
		}

		LOG_DBG("Sync RX descr = 0x%08x", descr);
		regs->DESCR[0] = descr;
		regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
		regs->EXE = MCHP_QMSPI_EXE_START;

		nrxb = (regs->BCNT_STS & MCHP_QMSPI_RX_BUF_CNT_STS_MASK) >>
			MCHP_QMSPI_RX_BUF_CNT_STS_POS;

		LOG_DBG("Sync RX start buf count = 0x%08x", nrxb);

		while (nrxb < nu) {
			qsts = regs->STS;
			if (qsts & (MCHP_QMSPI_STS_RXB_ERR |
				    MCHP_QMSPI_STS_PROG_ERR)) {
				regs->EXE = MCHP_QMSPI_EXE_STOP;
				return -EIO;
			}
			ret =  xec_qmspi_spin_yield(&counter,
						    XEC_QMSPI_WAIT_FULL_FIFO);
			if (ret != 0) {
				regs->EXE = MCHP_QMSPI_EXE_STOP;
				return ret;
			}

			nrxb = (regs->BCNT_STS &
				MCHP_QMSPI_RX_BUF_CNT_STS_MASK) >>
				MCHP_QMSPI_RX_BUF_CNT_STS_POS;

			LOG_DBG("Sync RX loop buf count = 0x%08x", nrxb);
		}

		LOG_DBG("Sync RX rem buf count = 0x%08x", nrxb);

		while (nrxb) {
			data_byte = sys_read8((mm_reg_t)&regs->RX_FIFO);
			if (p) {
				*p++ = data_byte;
			}
			nrxb--;
		}
	}

	return 0;
}

/* does the buffer set contain data? */
static bool is_buf_set(const struct spi_buf_set *bufs)
{
	if (!bufs) {
		return false;
	}

	if (bufs->count) {
		return true;
	}

	return false;
}

/*
 * can we use struct spi_qmspi_data to hold information
 * about number of pins to use for transmit/receive?
 */
static int qmspi_transceive(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs,
			    const struct spi_buf_set *rx_bufs)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct qmspi_regs *regs = cfg->regs;
	const struct spi_buf *pb;
	bool tx_close = false;
	bool rx_close = false;
	size_t nb = 0;
	int err = 0;

	spi_context_lock(&qdata->ctx, false, NULL, config);

	err = qmspi_configure(dev, config);
	if (err != 0) {
		spi_context_release(&qdata->ctx, err);
		return err;
	}

	spi_context_cs_control(&qdata->ctx, true);

	if (tx_bufs != NULL) {
		pb = tx_bufs->buffers;
		nb = tx_bufs->count;
		while (nb--) {
			if (!(config->operation & SPI_HOLD_ON_CS) && !nb &&
			    !is_buf_set(rx_bufs)) {
				tx_close = true;
			}

			if (pb->buf) {
				err = qmspi_tx(regs, qdata->np, pb, tx_close);
			} else {
				err = qmspi_tx_tsd_clks(regs, qdata->np,
							pb->len, tx_close);
			}
			if (err != 0) {
				spi_context_cs_control(&qdata->ctx, false);
				spi_context_release(&qdata->ctx, err);
				return err;
			}
			pb++;
		}
	}

	if (rx_bufs != NULL) {
		pb = rx_bufs->buffers;
		nb = rx_bufs->count;
		while (nb--) {
			if (!(config->operation & SPI_HOLD_ON_CS) && !nb) {
				rx_close = true;
			}

			err = qmspi_rx(regs, qdata->np, pb, rx_close);
			if (err != 0) {
				break;
			}
			pb++;
		}
	}

	spi_context_cs_control(&qdata->ctx, false);
	spi_context_release(&qdata->ctx, err);
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

static uint32_t ldma_units(const uint8_t *buf, size_t len)
{
	uint32_t mask = ((uint32_t)buf | len) & 0x03u;

	if (!mask) {
		return MCHP_QMSPI_LDC_ASZ_4;
	}
	return MCHP_QMSPI_LDC_ASZ_1;
}

static size_t descr_data_size(uint32_t *descr, const uint8_t *buf, size_t len)
{
	uint32_t qlen = len;
	uint32_t mask = (uint32_t)buf | qlen;
	uint32_t dlen = 0;

	if ((mask & 0x0f) == 0) { /* 16-byte units */
		dlen = (qlen / 16) & MCHP_QMSPI_C_XFR_NUNITS_MASK0;
		*descr = dlen << MCHP_QMSPI_C_XFR_NUNITS_POS;
		*descr |= MCHP_QMSPI_C_XFR_UNITS_16;
		dlen *= 16;
	} else if ((mask & 0x03) == 0) { /* 4 byte units */
		dlen = (qlen / 4) & MCHP_QMSPI_C_XFR_NUNITS_MASK0;
		*descr = dlen << MCHP_QMSPI_C_XFR_NUNITS_POS;
		*descr |= MCHP_QMSPI_C_XFR_UNITS_4;
		dlen *= 4;
	} else { /* QMSPI xfr length units = 1 bytes */
		dlen = qlen & MCHP_QMSPI_C_XFR_NUNITS_MASK0;
		*descr = dlen << MCHP_QMSPI_C_XFR_NUNITS_POS;
		*descr |= MCHP_QMSPI_C_XFR_UNITS_1;
	}

	return dlen;
}

static size_t tx_fifo_fill(struct qmspi_regs *regs, const uint8_t *pdata,
			   size_t ndata)
{
	size_t n = 0;

	while (n < ndata) {
		if (n >= MCHP_QMSPI_TX_FIFO_LEN) {
			break;
		}
		sys_write8(*pdata, (mm_reg_t)&regs->TX_FIFO);
		pdata++;
		n++;
	}

	return n;
}

static size_t rx_fifo_get(struct qmspi_regs *regs, uint8_t *pdata, size_t ndata)
{
	size_t n = 0;
	size_t nrxb = ((regs->BCNT_STS & MCHP_QMSPI_RX_BUF_CNT_STS_MASK) >>
			MCHP_QMSPI_RX_BUF_CNT_STS_POS);

	while ((n < ndata) && (n < nrxb)) {
		*pdata++ = sys_read8((mm_reg_t)&regs->RX_FIFO);
		n++;
	}

	return n;
}

/*
 * Do we close the transaction (de-assert CS#) for transmit?
 * NOTE: driver always performs all TX before RX.
 * For trasmit we close if caller did not request holding CS# active,
 * and we are on last TX buffer and no RX buffers.
 */
static bool is_tx_close(struct spi_context *ctx)
{
	if (!(ctx->owner->operation & SPI_HOLD_ON_CS) &&
	    (ctx->tx_count == 1) && !ctx->rx_count) {
		return true;
	}
	return false;
}

/*
 * Do we close the transaction (de-assert CS#) for receive.
 * For receive we close if caller did not request holding CS# active
 * and we are on last RX buffer.
 */
static bool is_rx_close(struct spi_context *ctx)
{
	if (!(ctx->owner->operation & SPI_HOLD_ON_CS) &&
	    (ctx->rx_count == 1)) {
		return true;
	}
	return false;
}

static bool spi_qmspi_async_start(const struct device *dev)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct spi_context *ctx = &qdata->ctx;
	struct qmspi_regs *regs = cfg->regs;
	uint32_t descr = 0;
	size_t dlen = 0;
	uint8_t npins = qdata->np; /* was cfg->width; */

	qdata->xfr_flags = 0;
	qdata->ldma_chan = 0;
	regs->CTRL = (regs->CTRL & MCHP_QMSPI_C_IFM_MASK) |
		     MCHP_QMSPI_C_DESCR_EN;
	regs->EXE = MCHP_QMSPI_EXE_CLR_FIFOS;
	regs->MODE &= ~(MCHP_QMSPI_M_LDMA_RX_EN | MCHP_QMSPI_M_LDMA_TX_EN);
	regs->LDMA_RX_DESCR_BM = 0;
	regs->LDMA_TX_DESCR_BM = 0;

	/* buffer is not NULL and length is not 0 */
	if (spi_context_tx_buf_on(ctx)) {
		dlen = descr_data_size(&descr, ctx->tx_buf, ctx->tx_len);
		qdata->xfr_len = dlen;
		qdata->xfr_flags |= XEC_QMSPI_XFR_FLAG_TX;
		descr |= MCHP_QMSPI_C_DESCR_LAST;
		if (is_tx_close(ctx)) {
			descr |= MCHP_QMSPI_C_CLOSE;
		}

		if (dlen) {
			descr |= MCHP_QMSPI_C_TX_DATA;
			if (dlen <= MCHP_QMSPI_TX_FIFO_LEN) {
				/* Load data into TX FIFO */
				tx_fifo_fill(regs, ctx->tx_buf, dlen);
			} else {
				/* LDMA TX channel 0 */
				descr |= (1u << MCHP_QMSPI_C_TX_DMA_POS);
				regs->LDTX[0].CTRL =
					ldma_units(ctx->tx_buf, dlen) |
					MCHP_QMSPI_LDC_INCR_EN;
				regs->LDTX[0].MSTART = (uint32_t)ctx->tx_buf;
				regs->LDTX[0].LEN = dlen;
				regs->LDTX[0].CTRL |= MCHP_QMSPI_LDC_EN;
				regs->LDMA_TX_DESCR_BM |= BIT(0);
				regs->MODE |= MCHP_QMSPI_M_LDMA_TX_EN;
				qdata->ldma_chan = 1;
			}
		}
	} else if (spi_context_tx_on(ctx)) {
		/* buffer is NULL and length is not 0. Tri-state clocks */
		qdata->xfr_len = ctx->tx_len;
		qdata->xfr_flags |= XEC_QMSPI_XFR_FLAG_TX;
		dlen = ctx->tx_len * npins;
		descr = dlen << MCHP_QMSPI_C_XFR_NUNITS_POS;
		descr |= MCHP_QMSPI_C_DESCR_LAST;
		if (is_tx_close(ctx)) {
			descr |= MCHP_QMSPI_C_CLOSE;
		}
	} else if (spi_context_rx_buf_on(ctx)) {
		dlen = descr_data_size(&descr, ctx->rx_buf, ctx->rx_len);
		qdata->xfr_len = dlen;
		qdata->xfr_flags &= ~XEC_QMSPI_XFR_FLAG_TX;
		descr |= MCHP_QMSPI_C_RX_EN;
		descr |= MCHP_QMSPI_C_DESCR_LAST;
		if (is_rx_close(ctx)) {
			descr |= MCHP_QMSPI_C_CLOSE;
		}
		if (dlen > MCHP_QMSPI_RX_FIFO_LEN) {
			/* LDMA RX channel 0 */
			descr |= (1u << MCHP_QMSPI_C_RX_DMA_POS);
			regs->LDRX[0].CTRL =
				ldma_units(ctx->rx_buf, dlen) |
				MCHP_QMSPI_LDC_INCR_EN;
			regs->LDRX[0].MSTART = (uint32_t)ctx->rx_buf;
			regs->LDRX[0].LEN = dlen;
			regs->LDRX[0].CTRL |= MCHP_QMSPI_LDC_EN;
			regs->LDMA_RX_DESCR_BM |= BIT(0);
			regs->MODE |= MCHP_QMSPI_M_LDMA_RX_EN;
			qdata->ldma_chan = 1;
		}
	}

	if (descr == 0) {
		return false;
	}

	descr |= encode_npins(npins);

	LOG_DBG("Async descr = 0x%08x", descr);

	regs->DESCR[0] = descr;

	/* clear status */
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;

	/* enable interrupt */
	regs->IEN = MCHP_QMSPI_IEN_XFR_DONE | MCHP_QMSPI_IEN_PROG_ERR |
		    MCHP_QMSPI_IEN_LDMA_RX_ERR | MCHP_QMSPI_IEN_LDMA_TX_ERR;

	/* start */
	qdata->xfr_flags |= XEC_QMSPI_XFR_FLAG_STARTED;
	regs->EXE = MCHP_QMSPI_EXE_START;

	return true;
}

static int qmspi_transceive_async(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs,
				  struct k_poll_signal *async)
{
	struct spi_qmspi_data *data = dev->data;

	spi_context_lock(&data->ctx, true, async, config);

	int ret = qmspi_configure(dev, config);

	if (ret != 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	spi_context_cs_control(&data->ctx, true);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	if (spi_qmspi_async_start(dev)) {
		return 0; /* QMSPI started */
	}

	/* error path */
	spi_context_cs_control(&data->ctx, false);
	spi_context_release(&data->ctx, ret);

	return -EIO;
}
#endif

static int qmspi_release(const struct device *dev,
			 const struct spi_config *config)
{
	struct spi_qmspi_data *data = dev->data;
	const struct spi_qmspi_config *cfg = dev->config;
	struct qmspi_regs *regs = cfg->regs;
	int ret = 0;
	int counter = 0;

	if (regs->STS & MCHP_QMSPI_STS_ACTIVE_RO) {
		/* Force CS# to de-assert on next unit boundary */
		regs->EXE = MCHP_QMSPI_EXE_STOP;
		while (regs->STS & MCHP_QMSPI_STS_ACTIVE_RO) {
			ret = xec_qmspi_spin_yield(&counter,
						   XEC_QMSPI_WAIT_COUNT);
			if (ret != 0) {
				break;
			}
		}
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

void qmspi_xec_isr(const struct device *dev)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *data = dev->data;
	struct qmspi_regs *regs = cfg->regs;
	uint32_t qstatus = regs->STS;
#ifdef CONFIG_SPI_ASYNC
	struct spi_context *ctx = &data->ctx;
	int xstatus = 0;
	size_t nrx = 0;
#endif

	regs->IEN = 0;
	data->qstatus = qstatus;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);

#ifdef CONFIG_SPI_ASYNC
	data->in_isr = 1;

	if (qstatus & XEC_QSPI_HW_ERRORS_ALL) {
		data->qstatus |= BIT(7);
		xstatus = (int)qstatus;
		spi_context_cs_control(&data->ctx, false);
		spi_context_complete(&data->ctx, xstatus);
	}

	if (data->xfr_flags & BIT(0)) { /* is TX ? */
		data->xfr_flags &= ~BIT(0);
		/* if last buffer ctx->tx_len should be 0 and this
		 * routine sets ctx->tx_buf to NULL.
		 * If we have another buffer ctx->tx_buf points to it
		 * and ctx->tx_len is set to new size.
		 * ISSUE: If we use buffer pointer = NULL and len != 0
		 * for tri-state clocks then spi_context_tx_buf_on
		 * will return false because it checks both!
		 */
		spi_context_update_tx(&data->ctx, 1, data->xfr_len);
		if (spi_context_tx_buf_on(&data->ctx)) {
			spi_qmspi_async_start(dev);
			return;
		}

		if (spi_context_rx_buf_on(&data->ctx)) {
			spi_qmspi_async_start(dev);
			return;
		}
	} else {
		/* Handle RX */
		if ((data->ldma_chan == 0) &&
		    (regs->BCNT_STS & MCHP_QMSPI_RX_BUF_CNT_STS_MASK)) {
			/* RX FIFO mode */
			nrx = rx_fifo_get(regs, data->ctx.rx_buf,
					  data->xfr_len);
		} else {
			nrx = data->xfr_len;
		}

		spi_context_update_rx(&data->ctx, 1, nrx);
		if (spi_context_rx_buf_on(&data->ctx)) {
			spi_qmspi_async_start(dev);
			return;
		}
	}

	if (!(ctx->owner->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(&data->ctx, false);
	}

	spi_context_complete(&data->ctx, xstatus);

	data->in_isr = 0;
#endif
}

/*
 * Called for each QMSPI controller instance
 * Initialize QMSPI controller.
 * Disable sleep control.
 * Disable and clear interrupt status.
 * Initialize SPI context.
 * QMSPI will be fully configured and enabled when the transceive API
 * is called.
 */
static int qmspi_xec_init(const struct device *dev)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct qmspi_regs *regs = cfg->regs;
	int ret;
	qdata->qstatus = 0;
	qdata->np = cfg->width;
#ifdef CONFIG_SPI_ASYNC
	qdata->xfr_flags = 0;
	qdata->ldma_chan = 0;
	qdata->in_isr = 0;
	qdata->xfr_len = 0;
#endif

	z_mchp_xec_pcr_periph_sleep(cfg->pcr_idx, cfg->pcr_pos, 0);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("QSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	qmspi_reset(regs);
	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);

#ifdef CONFIG_SPI_ASYNC
	cfg->irq_config_func();
	mchp_xec_ecia_enable(cfg->girq, cfg->girq_pos);
#endif

	spi_context_unlock_unconditionally(&qdata->ctx);

	return 0;
}

static const struct spi_driver_api spi_qmspi_xec_driver_api = {
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

#define XEC_QMSPI_TAPS_ADJ_VAL(a, b) (((a) & 0xffu) | (((b) & 0xffu) << 8))

#define XEC_QMSPI_CS_TIMING(i) XEC_QMSPI_CS_TIMING_VAL(			\
				DT_INST_PROP_OR(i, dcsckon, 6),		\
				DT_INST_PROP_OR(i, dckcsoff, 4),	\
				DT_INST_PROP_OR(i, dldh, 6),		\
				DT_INST_PROP_OR(i, dcsda, 6))

#define XEC_QMSPI_TAPS_ADJ(i) XEC_QMSPI_TAPS_ADJ_VAL(			\
				DT_INST_PROP_OR(i, tctradj, 0),		\
				DT_INST_PROP_OR(i, tsckadj, 0))

#define XEC_QMSPI_GIRQ(i)						\
	MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(i, girqs, 0))

#define XEC_QMSPI_GIRQ_POS(i)						\
	MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(i, girqs, 0))

#define XEC_QMSPI_NVIC_AGGR(i)						\
	MCHP_XEC_ECIA_NVIC_AGGR(DT_INST_PROP_BY_IDX(i, girqs, 0))

#define XEC_QMSPI_NVIC_DIRECT(i)					\
	MCHP_XEC_ECIA_NVIC_DIRECT(DT_INST_PROP_BY_IDX(i, girqs, 0))

/*
 * The instance number, i is not related to block ID's rather the
 * order the DT tools process all DT files in a build.
 */
#define QMSPI_XEC_DEVICE(i)						\
									\
	PINCTRL_DT_INST_DEFINE(i);					\
									\
	static void qmspi_xec_irq_config_func_##i(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(i),				\
			    DT_INST_IRQ(i, priority),			\
			    qmspi_xec_isr,				\
			    DEVICE_DT_INST_GET(i), 0);			\
		irq_enable(DT_INST_IRQN(i));				\
	}								\
									\
	static struct spi_qmspi_data qmspi_xec_data_##i = {		\
		SPI_CONTEXT_INIT_LOCK(qmspi_xec_data_##i, ctx),		\
		SPI_CONTEXT_INIT_SYNC(qmspi_xec_data_##i, ctx),		\
	};								\
	static const struct spi_qmspi_config qmspi_xec_config_##i = {	\
		.regs = (struct qmspi_regs *) DT_INST_REG_ADDR(i),	\
		.cs1_freq = DT_INST_PROP_OR(i, cs1-freq, 0),		\
		.cs_timing = XEC_QMSPI_CS_TIMING(i),			\
		.taps_adj = XEC_QMSPI_TAPS_ADJ(i),			\
		.girq = XEC_QMSPI_GIRQ(i),				\
		.girq_pos = XEC_QMSPI_GIRQ_POS(i),			\
		.girq_nvic_aggr = XEC_QMSPI_NVIC_AGGR(i),		\
		.girq_nvic_direct = XEC_QMSPI_NVIC_DIRECT(i),		\
		.irq_pri = DT_INST_IRQ(i, priority),			\
		.pcr_idx = DT_INST_PROP_BY_IDX(i, pcrs, 0),		\
		.pcr_pos = DT_INST_PROP_BY_IDX(i, pcrs, 1),		\
		.port_sel = DT_INST_PROP_OR(i, port-sel, 0),		\
		.chip_sel = DT_INST_PROP_OR(i, chip-select, 0),		\
		.width = DT_INST_PROP_OR(0, lines, 1),			\
		.irq_config_func = qmspi_xec_irq_config_func_##i,	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),		\
	};								\
	DEVICE_DT_INST_DEFINE(i, &qmspi_xec_init, NULL,			\
		&qmspi_xec_data_##i, &qmspi_xec_config_##i,		\
		POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,			\
		&spi_qmspi_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QMSPI_XEC_DEVICE)
