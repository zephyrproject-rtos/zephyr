/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_qmspi_ldma

#include <errno.h>
#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_xec, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* #define MCHP_XEC_QMSPI_DEBUG 1 */

/* MEC172x QMSPI controller SPI Mode 3 signalling has an anomaly where
 * received data is shifted off the input line(s) improperly. Received
 * data bytes will be left shifted by 1. Work-around for SPI Mode 3 is
 * to sample input line(s) on same edge as output data is ready.
 */
#define XEC_QMSPI_SPI_MODE_3_ANOMALY 1

/* common clock control device node for all Microchip XEC chips */
#define MCHP_XEC_CLOCK_CONTROL_NODE	DT_NODELABEL(pcr)

/* spin loops waiting for HW to clear soft reset bit */
#define XEC_QMSPI_SRST_LOOPS		16

/* microseconds for busy wait and total wait interval */
#define XEC_QMSPI_WAIT_INTERVAL		8
#define XEC_QMSPI_WAIT_COUNT		64

/* QSPI transfer and DMA done */
#define XEC_QSPI_HW_XFR_DMA_DONE	(MCHP_QMSPI_STS_DONE | MCHP_QMSPI_STS_DMA_DONE)

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

#define XEC_QSPI_TIMEOUT_US		(100 * 1000) /* 100 ms */

/* Device constant configuration parameters */
struct spi_qmspi_config {
	struct qmspi_regs *regs;
	const struct device *clk_dev;
	struct mchp_xec_pcr_clk_ctrl clksrc;
	uint32_t clock_freq;
	uint32_t cs1_freq;
	uint32_t cs_timing;
	uint16_t taps_adj;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t girq_nvic_aggr;
	uint8_t girq_nvic_direct;
	uint8_t irq_pri;
	uint8_t chip_sel;
	uint8_t width;	/* 0(half) 1(single), 2(dual), 4(quad) */
	uint8_t unused[1];
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

#define XEC_QMSPI_XFR_FLAG_TX		BIT(0)
#define XEC_QMSPI_XFR_FLAG_RX		BIT(1)

/* Device run time data */
struct spi_qmspi_data {
	struct spi_context ctx;
	uint32_t base_freq_hz;
	uint32_t spi_freq_hz;
	uint32_t qstatus;
	uint8_t np; /* number of data pins: 1, 2, or 4 */
#ifdef CONFIG_SPI_ASYNC
	spi_callback_t cb;
	void *userdata;
	size_t xfr_len;
#endif
	uint32_t tempbuf[2];
#ifdef MCHP_XEC_QMSPI_DEBUG
	uint32_t bufcnt_status;
	uint32_t rx_ldma_ctrl0;
	uint32_t tx_ldma_ctrl0;
	uint32_t qunits;
	uint32_t qxfru;
	uint32_t xfrlen;

#endif
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

static uint32_t qmspi_encoded_fdiv(const struct device *dev, uint32_t freq_hz)
{
	struct spi_qmspi_data *qdata = dev->data;

	if (freq_hz == 0u) {
		return 0u; /* maximum frequency divider */
	}

	return (qdata->base_freq_hz / freq_hz);
}

/* Program QMSPI frequency divider field in the mode register.
 * MEC172x QMSPI input clock source is the Fast Peripheral domain whose
 * clock is controlled by the PCR turbo clock. 96 MHz if turbo mode
 * enabled else 48 MHz. Query the clock control driver to get clock
 * rate of fast peripheral domain. MEC172x QMSPI clock divider has
 * been expanded to a 16-bit field encoded as:
 * 0 = divide by 0x10000
 * 1 to 0xffff = divide by this value.
 */
static int qmspi_set_frequency(struct spi_qmspi_data *qdata, struct qmspi_regs *regs,
			       uint32_t freq_hz)
{
	uint32_t clk = MCHP_QMSPI_INPUT_CLOCK_FREQ_HZ;
	uint32_t fdiv = 0u; /* maximum divider */

	if (qdata->base_freq_hz) {
		clk = qdata->base_freq_hz;
	}

	if (freq_hz) {
		fdiv = 1u;
		if (freq_hz < clk) {
			fdiv = clk / freq_hz;
		}
	}

	regs->MODE = ((regs->MODE & ~(MCHP_QMSPI_M_FDIV_MASK)) |
		((fdiv << MCHP_QMSPI_M_FDIV_POS) & MCHP_QMSPI_M_FDIV_MASK));

	if (!fdiv) {
		fdiv = 0x10000u;
	}

	qdata->spi_freq_hz = clk / fdiv;

	return 0;
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
 * QMSPI has three controls, CPOL, CPHA for output and CPHA for input.
 * SPI frequency < 48MHz
 *	Mode 0: CPOL=0 CHPA=0 (CHPA_MISO=0 and CHPA_MOSI=0)
 *	Mode 3: CPOL=1 CHPA=1 (CHPA_MISO=1 and CHPA_MOSI=1)
 * Data sheet recommends when QMSPI set at max. SPI frequency (48MHz).
 * SPI frequency == 48MHz sample and change data on same edge.
 *  Mode 0: CPOL=0 CHPA=0 (CHPA_MISO=1 and CHPA_MOSI=0)
 *  Mode 3: CPOL=1 CHPA=1 (CHPA_MISO=0 and CHPA_MOSI=1)
 *
 * There is an anomaly in MEC172x for SPI signalling mode 3. We must
 * set CHPA_MISO=0 for SPI Mode 3 at all frequencies.
 */

const uint8_t smode_tbl[4] = {
	0x00u, 0x06u, 0x01u,
#ifdef XEC_QMSPI_SPI_MODE_3_ANOMALY
	0x03u, /* CPOL=1, CPHA_MOSI=1, CPHA_MISO=0 */
#else
	0x07u, /* CPOL=1, CPHA_MOSI=1, CPHA_MISO=1 */
#endif
};

const uint8_t smode48_tbl[4] = {
	0x04u, 0x02u, 0x05u, 0x03u
};

static void qmspi_set_signalling_mode(struct spi_qmspi_data *qdata,
				      struct qmspi_regs *regs, uint32_t smode)
{
	const uint8_t *ptbl;
	uint32_t m;

	ptbl = smode_tbl;
	if (qdata->spi_freq_hz >= MHZ(48)) {
		ptbl = smode48_tbl;
	}

	m = (uint32_t)ptbl[smode & 0x03];
	regs->MODE = (regs->MODE & ~(MCHP_QMSPI_M_SIG_MASK))
		     | (m << MCHP_QMSPI_M_SIG_POS);
}

#ifdef CONFIG_SPI_EXTENDED_MODES
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
#endif /* CONFIG_SPI_EXTENDED_MODES */

static int spi_feature_support(const struct spi_config *config)
{
	if (config->operation & (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE | SPI_MODE_LOOP)) {
		LOG_ERR("Driver does not support LSB first, slave, or loop back");
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

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size != 8 not supported");
		return -ENOTSUP;
	}

	return 0;
}

/* Configure QMSPI.
 * NOTE: QMSPI Shared SPI port has two chip selects.
 * Private SPI and internal SPI ports support one chip select.
 * Hardware supports dual and quad I/O. Dual and quad are allowed
 * if SPI extended mode is enabled at build time. User must
 * provide pin configuration via DTS.
 */
static int qmspi_configure(const struct device *dev,
			   const struct spi_config *config)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct qmspi_regs *regs = cfg->regs;
	uint32_t smode;
	int ret;

	if (!config) {
		return -EINVAL;
	}

	if (spi_context_configured(&qdata->ctx, config)) {
		return 0;
	}

	qmspi_set_frequency(qdata, regs, config->frequency);

	/* check new configuration */
	ret = spi_feature_support(config);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_SPI_EXTENDED_MODES
	smode = encode_lines(config);
	if (smode == 0xff) {
		LOG_ERR("Requested lines mode not supported");
		return -ENOTSUP;
	}
	qdata->np = npins_from_spi_config(config);
#else
	smode = MCHP_QMSPI_C_IFM_1X;
	qdata->np = 1u;
#endif
	regs->CTRL = smode;

	smode = 0;
	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		smode |= BIT(0);
	}

	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		smode |= BIT(1);
	}

	qmspi_set_signalling_mode(qdata, regs, smode);

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
		uint32_t fdiv = qmspi_encoded_fdiv(dev, cfg->cs1_freq);

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

/* Common controller transfer initialziation using Local-DMA.
 * Full-duplex: controller configured to transmit and receive simultaneouly.
 * Half-duplex(dual/quad): User may only specify TX or RX buffer sets.
 * Passing both buffers sets is reported as an error.
 */
static inline int qmspi_xfr_cm_init(const struct device *dev,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_qmspi_config *devcfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct qmspi_regs *regs = devcfg->regs;

	regs->IEN = 0;
	regs->EXE = MCHP_QMSPI_EXE_CLR_FIFOS;
	regs->LDMA_RX_DESCR_BM = 0;
	regs->LDMA_TX_DESCR_BM = 0;
	regs->MODE &= ~(MCHP_QMSPI_M_LDMA_TX_EN | MCHP_QMSPI_M_LDMA_RX_EN);
	regs->STS = 0xffffffffu;
	regs->CTRL = encode_npins(qdata->np);

	qdata->qstatus = 0;

#ifdef CONFIG_SPI_EXTENDED_MODES
	if (qdata->np != 1) {
		if (tx_bufs && rx_bufs) {
			LOG_ERR("Cannot specify both TX and RX buffers in half-duplex(dual/quad)");
			return -EPROTONOSUPPORT;
		}
	}
#endif

	return 0;
}

/* QMSPI Local-DMA transfer configuration:
 * Support full and half(dual/quad) duplex transfers.
 * Requires caller to have checked that only one direction was setup
 * in the SPI context: TX or RX not both. (refer to qmspi_xfr_cm_init)
 * Supports spi_buf's where data pointer is NULL and length non-zero.
 * These buffers are used as TX tri-state I/O clock only generation or
 * RX data discard for certain SPI command protocols using dual/quad I/O.
 * 1. Get largest contiguous data size from SPI context.
 * 2. If the SPI TX context has a non-zero length configure Local-DMA TX
 *    channel 1 for contiguous data size. If TX context has valid buffer
 *    configure channel to use context buffer with address increment.
 *    If the TX buffer pointer is NULL interpret byte length as the number
 *    of clocks to generate with output line(s) tri-stated. NOTE: The controller
 *    must be configured with TX disabled to not drive output line(s) during
 *    clock generation. Also, no data should be written to TX FIFO. The unit
 *    size can be set to bits. The number of units to transfer must be computed
 *    based upon the number of output pins in the IOM field: full-duplex is one
 *    bit per clock, dual is 2 bits per clock, and quad is 4 bits per clock.
 *    For example, if I/O lines is 4 (quad) meaning 4 bits per clock and the
 *    user wants 7 clocks then the number of bit units is 4 * 7 = 28.
 * 3. If instead, the SPI RX context has a non-zero length configure Local-DMA
 *    RX channel 1 for the contiguous data size. If RX context has a valid
 *    buffer configure channel to use buffer with address increment else
 *    configure channel for driver data temporary buffer without address
 *    increment.
 * 4. Update QMSPI Control register.
 */
static uint32_t qmspi_ldma_encode_unit_size(uint32_t maddr, size_t len)
{
	uint8_t temp = (maddr | (uint32_t)len) & 0x3u;

	if (temp == 0) {
		return MCHP_QMSPI_LDC_ASZ_4;
	} else if (temp == 2) {
		return MCHP_QMSPI_LDC_ASZ_2;
	} else {
		return MCHP_QMSPI_LDC_ASZ_1;
	}
}

static uint32_t qmspi_unit_size(size_t xfrlen)
{
	if ((xfrlen & 0xfu) == 0u) {
		return 16u;
	} else if ((xfrlen & 0x3u) == 0u) {
		return 4u;
	} else {
		return 1u;
	}
}

static uint32_t qmspi_encode_unit_size(uint32_t units_in_bytes)
{
	if (units_in_bytes == 16u) {
		return MCHP_QMSPI_C_XFR_UNITS_16;
	} else if (units_in_bytes == 4u) {
		return MCHP_QMSPI_C_XFR_UNITS_4;
	} else {
		return MCHP_QMSPI_C_XFR_UNITS_1;
	}
}

static size_t q_ldma_cfg(const struct device *dev)
{
	const struct spi_qmspi_config *devcfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct spi_context *ctx = &qdata->ctx;
	struct qmspi_regs *regs = devcfg->regs;

	size_t ctx_xfr_len = spi_context_max_continuous_chunk(ctx);
	uint32_t ctrl, ldctrl, mstart, qunits, qxfru, xfrlen;

	regs->EXE = MCHP_QMSPI_EXE_CLR_FIFOS;
	regs->MODE &= ~(MCHP_QMSPI_M_LDMA_RX_EN | MCHP_QMSPI_M_LDMA_TX_EN);
	regs->LDRX[0].CTRL = 0;
	regs->LDRX[0].MSTART = 0;
	regs->LDRX[0].LEN = 0;
	regs->LDTX[0].CTRL = 0;
	regs->LDTX[0].MSTART = 0;
	regs->LDTX[0].LEN = 0;

	if (ctx_xfr_len == 0) {
		return 0;
	}

	qunits = qmspi_unit_size(ctx_xfr_len);
	ctrl = qmspi_encode_unit_size(qunits);
	qxfru = ctx_xfr_len / qunits;
	if (qxfru > 0x7fffu) {
		qxfru = 0x7fffu;
	}
	ctrl |= (qxfru << MCHP_QMSPI_C_XFR_NUNITS_POS);
	xfrlen = qxfru * qunits;

#ifdef MCHP_XEC_QMSPI_DEBUG
	qdata->qunits = qunits;
	qdata->qxfru = qxfru;
	qdata->xfrlen = xfrlen;
#endif
	if (spi_context_tx_buf_on(ctx)) {
		mstart = (uint32_t)ctx->tx_buf;
		ctrl |= MCHP_QMSPI_C_TX_DATA | MCHP_QMSPI_C_TX_LDMA_CH0;
		ldctrl = qmspi_ldma_encode_unit_size(mstart, xfrlen);
		ldctrl |= MCHP_QMSPI_LDC_INCR_EN | MCHP_QMSPI_LDC_EN;
		regs->MODE |= MCHP_QMSPI_M_LDMA_TX_EN;
		regs->LDTX[0].LEN = xfrlen;
		regs->LDTX[0].MSTART = mstart;
		regs->LDTX[0].CTRL = ldctrl;
	}

	if (spi_context_rx_buf_on(ctx)) {
		mstart = (uint32_t)ctx->rx_buf;
		ctrl |= MCHP_QMSPI_C_RX_LDMA_CH0 | MCHP_QMSPI_C_RX_EN;
		ldctrl = MCHP_QMSPI_LDC_EN | MCHP_QMSPI_LDC_INCR_EN;
		ldctrl |= qmspi_ldma_encode_unit_size(mstart, xfrlen);
		regs->MODE |= MCHP_QMSPI_M_LDMA_RX_EN;
		regs->LDRX[0].LEN = xfrlen;
		regs->LDRX[0].MSTART = mstart;
		regs->LDRX[0].CTRL = ldctrl;
	}

	regs->CTRL = (regs->CTRL & 0x3u) | ctrl;

	return xfrlen;
}

/* Start and wait for QMSPI synchronous transfer(s) to complete.
 * Initialize QMSPI controller for Local-DMA operation.
 * Iterate over SPI context with non-zero TX or RX data lengths.
 *   1. Configure QMSPI Control register and Local-DMA channel(s)
 *   2. Clear QMSPI status
 *   3. Start QMSPI transfer
 *   4. Poll QMSPI status for transfer done and DMA done with timeout.
 *   5. Hardware anomaly work-around: Poll with timeout QMSPI Local-DMA
 *      TX and RX channels until hardware clears both channel enables.
 *      This indicates hardware is really done with transfer to/from memory.
 *   6. Update SPI context with amount of data transmitted and received.
 * If SPI configuration hold chip select on flag is not set then instruct
 * QMSPI to de-assert chip select.
 * Set SPI context as complete
 */
static int qmspi_xfr_sync(const struct device *dev,
			  const struct spi_config *spi_cfg,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
{
	const struct spi_qmspi_config *devcfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct spi_context *ctx = &qdata->ctx;
	struct qmspi_regs *regs = devcfg->regs;
	size_t xfr_len;

	int ret = qmspi_xfr_cm_init(dev, tx_bufs, rx_bufs);

	if (ret) {
		return ret;
	}

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		xfr_len = q_ldma_cfg(dev);
		regs->STS = 0xffffffffu;
		regs->EXE = MCHP_QMSPI_EXE_START;

#ifdef MCHP_XEC_QMSPI_DEBUG
		uint32_t temp = regs->STS;

		while (!(temp & MCHP_QMSPI_STS_DONE)) {
			temp = regs->STS;
		}
		qdata->qstatus = temp;
		qdata->bufcnt_status = regs->BCNT_STS;
		qdata->rx_ldma_ctrl0 = regs->LDRX[0].CTRL;
		qdata->tx_ldma_ctrl0 = regs->LDTX[0].CTRL;
#else
		uint32_t wcnt = 0;

		qdata->qstatus = regs->STS;
		while (!(qdata->qstatus & MCHP_QMSPI_STS_DONE)) {
			k_busy_wait(1u);
			if (++wcnt > XEC_QSPI_TIMEOUT_US) {
				regs->EXE = MCHP_QMSPI_EXE_STOP;
				return -ETIMEDOUT;
			}
			qdata->qstatus = regs->STS;
		}
#endif
		spi_context_update_tx(ctx, 1, xfr_len);
		spi_context_update_rx(ctx, 1, xfr_len);
	}

	if (!(spi_cfg->operation & SPI_HOLD_ON_CS)) {
		regs->EXE = MCHP_QMSPI_EXE_STOP;
	}

	spi_context_complete(ctx, dev, 0);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
/* Configure QMSPI such that QMSPI transfer FSM and LDMA FSM are synchronized.
 * Transfer length must be programmed into control/descriptor register(s) and
 * LDMA register(s). LDMA override length bit must NOT be set.
 */
static int qmspi_xfr_start_async(const struct device *dev, const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct spi_qmspi_config *devcfg = dev->config;
	struct spi_qmspi_data *qdata = dev->data;
	struct qmspi_regs *regs = devcfg->regs;
	int ret;

	ret = qmspi_xfr_cm_init(dev, tx_bufs, rx_bufs);
	if (ret) {
		return ret;
	}

	qdata->xfr_len = q_ldma_cfg(dev);
	if (!qdata->xfr_len) {
		return 0; /* nothing to do */
	}

	regs->STS = 0xffffffffu;
	regs->EXE = MCHP_QMSPI_EXE_START;
	regs->IEN = MCHP_QMSPI_IEN_XFR_DONE | MCHP_QMSPI_IEN_PROG_ERR
		    | MCHP_QMSPI_IEN_LDMA_RX_ERR | MCHP_QMSPI_IEN_LDMA_TX_ERR;

	return 0;
}

/* Wrapper to start asynchronous (interrupts enabled) SPI transaction */
static int qmspi_xfr_async(const struct device *dev,
			   const struct spi_config *config,
			   const struct spi_buf_set *tx_bufs,
			   const struct spi_buf_set *rx_bufs)
{
	struct spi_qmspi_data *qdata = dev->data;
	int err = 0;

	qdata->qstatus = 0;
	qdata->xfr_len = 0;

	err = qmspi_xfr_start_async(dev, tx_bufs, rx_bufs);

	return err;
}
#endif /* CONFIG_SPI_ASYNC */

/* Start (a)synchronous transaction using QMSPI Local-DMA */
static int qmspi_transceive(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs,
			    const struct spi_buf_set *rx_bufs,
			    bool asynchronous,
			    spi_callback_t cb,
			    void *user_data)
{
	struct spi_qmspi_data *qdata = dev->data;
	struct spi_context *ctx = &qdata->ctx;
	int err = 0;

	if (!config) {
		return -EINVAL;
	}

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	spi_context_lock(&qdata->ctx, asynchronous, cb, user_data, config);

	err = qmspi_configure(dev, config);
	if (err != 0) {
		spi_context_release(ctx, err);
		return err;
	}

	spi_context_cs_control(ctx, true);
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

#ifdef CONFIG_SPI_ASYNC
	if (asynchronous) {
		qdata->cb = cb;
		qdata->userdata = user_data;
		err = qmspi_xfr_async(dev, config, tx_bufs, rx_bufs);
	} else {
		err = qmspi_xfr_sync(dev, config, tx_bufs, rx_bufs);
	}
#else
	err = qmspi_xfr_sync(dev, config, tx_bufs, rx_bufs);
#endif
	if (err) { /* de-assert CS# and give semaphore */
		spi_context_unlock_unconditionally(ctx);
		return err;
	}

	if (asynchronous) {
		return err;
	}

	err = spi_context_wait_for_completion(ctx);
	if (!(config->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(ctx, false);
	}
	spi_context_release(ctx, err);

	return err;
}

static int qmspi_transceive_sync(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	return qmspi_transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC

static int qmspi_transceive_async(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs,
				  spi_callback_t cb,
				  void *userdata)
{
	return qmspi_transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

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
			ret = xec_qmspi_spin_yield(&counter, XEC_QMSPI_WAIT_COUNT);
			if (ret != 0) {
				break;
			}
		}
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

/* QMSPI interrupt handler called by Zephyr ISR
 * All transfers use QMSPI Local-DMA specified by the Control register.
 * QMSPI descriptor mode not used.
 * Full-duplex always uses LDMA TX channel 0 and RX channel 0
 * Half-duplex(dual/quad) use one of TX channel 0 or RX channel 0
 */
void qmspi_xec_isr(const struct device *dev)
{
	const struct spi_qmspi_config *cfg = dev->config;
	struct spi_qmspi_data *data = dev->data;
	struct qmspi_regs *regs = cfg->regs;
	uint32_t qstatus = regs->STS;
#ifdef CONFIG_SPI_ASYNC
	struct spi_context *ctx = &data->ctx;
	int xstatus = 0;
#endif

	regs->IEN = 0;
	data->qstatus = qstatus;
	regs->STS = MCHP_QMSPI_STS_RW1C_MASK;
	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);

#ifdef CONFIG_SPI_ASYNC
	if (qstatus & XEC_QSPI_HW_ERRORS_ALL) {
		xstatus = -EIO;
		data->qstatus |= BIT(7);
		regs->EXE = MCHP_QMSPI_EXE_STOP;
		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, xstatus);
		if (data->cb) {
			data->cb(dev, xstatus, data->userdata);
		}
		return;
	}

	/* Clear Local-DMA enables in Mode and Control registers */
	regs->MODE &= ~(MCHP_QMSPI_M_LDMA_RX_EN | MCHP_QMSPI_M_LDMA_TX_EN);
	regs->CTRL &= MCHP_QMSPI_C_IFM_MASK;

	spi_context_update_tx(ctx, 1, data->xfr_len);
	spi_context_update_rx(ctx, 1, data->xfr_len);

	data->xfr_len = q_ldma_cfg(dev);
	if (data->xfr_len) {
		regs->STS = 0xffffffffu;
		regs->EXE = MCHP_QMSPI_EXE_START;
		regs->IEN = MCHP_QMSPI_IEN_XFR_DONE | MCHP_QMSPI_IEN_PROG_ERR
			    | MCHP_QMSPI_IEN_LDMA_RX_ERR | MCHP_QMSPI_IEN_LDMA_TX_ERR;
		return;
	}

	if (!(ctx->owner->operation & SPI_HOLD_ON_CS)) {
		regs->EXE = MCHP_QMSPI_EXE_STOP;
		spi_context_cs_control(&data->ctx, false);
	}

	spi_context_complete(&data->ctx, dev, xstatus);

	if (data->cb) {
		data->cb(dev, xstatus, data->userdata);
	}
#endif /* CONFIG_SPI_ASYNC */
}

#ifdef CONFIG_PM_DEVICE
/* If the application wants the QMSPI pins to be disabled in suspend it must
 * define pinctr-1 values for each pin in the app/project DT overlay.
 */
static int qmspi_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct spi_qmspi_config *devcfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_SLEEP);
		if (ret == -ENOENT) { /* pinctrl-1 does not exist */
			ret = 0;
		}
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

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
	clock_control_subsys_t clkss = (clock_control_subsys_t)MCHP_XEC_PCR_CLK_PERIPH_FAST;
	int ret = 0;

	qdata->base_freq_hz = 0u;
	qdata->qstatus = 0;
	qdata->np = cfg->width;
#ifdef CONFIG_SPI_ASYNC
	qdata->xfr_len = 0;
#endif

	if (!cfg->clk_dev) {
		LOG_ERR("XEC QMSPI-LDMA clock device not configured");
		return -EINVAL;
	}

	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&cfg->clksrc);
	if (ret < 0) {
		LOG_ERR("XEC QMSPI-LDMA enable clock source error %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(cfg->clk_dev, clkss, &qdata->base_freq_hz);
	if (ret) {
		LOG_ERR("XEC QMSPI-LDMA clock get rate error %d", ret);
		return ret;
	}

	/* controller in known state before enabling pins */
	qmspi_reset(regs);
	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC QMSPI-LDMA pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* default SPI Mode 0 signalling */
	const struct spi_config spi_cfg = {
		.frequency = cfg->clock_freq,
		.operation = SPI_LINES_SINGLE | SPI_WORD_SET(8),
	};

	ret = qmspi_configure(dev, &spi_cfg);
	if (ret) {
		LOG_ERR("XEC QMSPI-LDMA init configure failed (%d)", ret);
		return ret;
	}

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
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
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

#define XEC_QMSPI_PCR_INFO(i)						\
	MCHP_XEC_PCR_SCR_ENCODE(DT_INST_CLOCKS_CELL(i, regidx),		\
				DT_INST_CLOCKS_CELL(i, bitpos),		\
				DT_INST_CLOCKS_CELL(i, domain))

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
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(i)),	\
		.clksrc = { .pcr_info = XEC_QMSPI_PCR_INFO(i), },	\
		.clock_freq = DT_INST_PROP_OR(i, clock_frequency, MHZ(12)), \
		.cs1_freq = DT_INST_PROP_OR(i, cs1_freq, 0),		\
		.cs_timing = XEC_QMSPI_CS_TIMING(i),			\
		.taps_adj = XEC_QMSPI_TAPS_ADJ(i),			\
		.girq = XEC_QMSPI_GIRQ(i),				\
		.girq_pos = XEC_QMSPI_GIRQ_POS(i),			\
		.girq_nvic_aggr = XEC_QMSPI_NVIC_AGGR(i),		\
		.girq_nvic_direct = XEC_QMSPI_NVIC_DIRECT(i),		\
		.irq_pri = DT_INST_IRQ(i, priority),			\
		.chip_sel = DT_INST_PROP_OR(i, chip_select, 0),		\
		.width = DT_INST_PROP_OR(0, lines, 1),			\
		.irq_config_func = qmspi_xec_irq_config_func_##i,	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),		\
	};								\
	PM_DEVICE_DT_INST_DEFINE(i, qmspi_xec_pm_action);		\
	DEVICE_DT_INST_DEFINE(i, qmspi_xec_init,			\
		PM_DEVICE_DT_INST_GET(i),				\
		&qmspi_xec_data_##i, &qmspi_xec_config_##i,		\
		POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,			\
		&spi_qmspi_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QMSPI_XEC_DEVICE)
