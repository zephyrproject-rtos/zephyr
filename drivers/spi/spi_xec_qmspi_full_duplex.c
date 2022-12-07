/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_qmspi_full_duplex

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
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "spi_context.h"
#include "spi_xec_qmspi_full_duplex.h"

#if XEC_QSPI_TX_FIFO_SIZE < XEC_QSPI_RX_FIFO_SIZE
#define XEC_QSPI_CHUNK_SIZE XEC_QSPI_TX_FIFO_SIZE
#else
#define XEC_QSPI_CHUNK_SIZE XEC_QSPI_RX_FIFO_SIZE
#endif

/* spin loops waiting for HW to clear soft reset bit */
#define XEC_QSPI_SRST_LOOPS		16

/* microseconds for busy wait and total wait interval */
#define XEC_QSPI_WAIT_INTERVAL		8
#define XEC_QSPI_WAIT_COUNT		64
#define XEC_QSPI_WAIT_FULL_FIFO		1024

/* 3 Tap Regs - Tap, Tap Ctrl, Tap Adjust */
#define TAP_REGS_MAX		3

#define CLOCK_DIV_0_VALUE	0x10000

/*
 * Maximum number of units to generate clocks with data lines
 * tri-stated depends upon bus width. Maximum bus width is 4.
 */
#define XEC_QSPI_MAX_TSCLK_UNITS	(MCHP_QMSPI_C_MAX_UNITS / 4)

#define XEC_QSPI_HALF_DUPLEX		0
#define XEC_QSPI_FULL_DUPLEX		1
#define XEC_QSPI_DUAL			2
#define XEC_QSPI_QUAD			4

#define XEC_QSPI_STS_ERRORS (BIT(XEC_QSPI_STS_TXB_ERR_POS) |			\
			     BIT(XEC_QSPI_STS_RXB_ERR_POS) |			\
			     BIT(XEC_QSPI_STS_PROG_ERR_POS) |			\
			     BIT(XEC_QSPI_STS_LDMA_RX_ERR_POS) |		\
			     BIT(XEC_QSPI_STS_LDMA_TX_ERR_POS))

#define XEC_QSPI_IEN_DONE_ERR (BIT(XEC_QSPI_IEN_XFR_DONE_POS) |			\
			       BIT(XEC_QSPI_IEN_TXB_ERR_POS) |			\
			       BIT(XEC_QSPI_IEN_RXB_ERR_POS) |			\
			       BIT(XEC_QSPI_IEN_PROG_ERR_POS) |			\
			       BIT(XEC_QSPI_IEN_LDMA_RX_ERR_POS) |		\
			       BIT(XEC_QSPI_IEN_LDMA_TX_ERR_POS));

/* Device constant configuration parameters */
struct spi_xec_qspi_config {
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
	uint8_t chip_sel;
	uint8_t width;	/* 0(half) 1(single), 2(dual), 4(quad) */
	uint8_t unused[2];
	const struct pinctrl_dev_config *pcfg;
};

#define XEC_QMSPI_XFR_FLAG_TX		BIT(0)
#define XEC_QMSPI_XFR_FLAG_STARTED	BIT(1)

/* Device run time data */
struct spi_xec_qspi_data {
	struct spi_context ctx;
	uint32_t qstatus;
	uint8_t np; /* number of data pins: 1, 2, or 4 */
};

static int xec_qspi_spin_yield(int *counter, int max_count)
{
	*counter = *counter + 1;

	if (*counter > max_count) {
		return -ETIMEDOUT;
	}

	k_busy_wait(XEC_QSPI_WAIT_INTERVAL);

	return 0;
}

/*
 * reset QMSPI controller with save/restore of timing registers.
 * Some QMSPI timing register may be modified by the Boot-ROM OTP
 * values.
 */
static void xec_qspi_reset(struct qmspi_regs *regs)
{
	uint32_t taps[TAP_REGS_MAX];
	uint32_t malt1;
	uint32_t cstm;
	uint32_t mode;
	uint32_t cnt = XEC_QSPI_SRST_LOOPS;

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

static uint32_t qspi_source_clock_freq(void)
{
	struct pcr_regs const *pcr =
		(struct pcr_regs *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 0));

	if (pcr->TURBO_CLK & MCHP_PCR_TURBO_CLK_96M) {
		return MEC172X_QSPI_TURBO_SRC_CLOCK_HZ;
	}
	return MEC172X_QSPI_SRC_CLOCK_HZ;
}

/*
 * Calculate QMSPI frequency divider register field value based upon
 * the configured QMSPI input frequency: 48 or 96 MHz.
 * The hardware divider is encoded as:
 * 0 is divide by full divider range: 256 or 65536.
 * Non-zero is divide by that value: 1 to 256 or 655356.
 */
static uint32_t qspi_encoded_fdiv(uint32_t freq_hz)
{
	uint32_t fdiv = 1;
	uint32_t src_clk = qspi_source_clock_freq();

	if (freq_hz < (src_clk / 256u)) {
		fdiv = 0; /* HW fdiv = 0 is divide by 256 */
	} else if (freq_hz < src_clk) {
		/* truncated integer division may result in lower freq. */
		fdiv = src_clk / freq_hz;
	}

	return fdiv;
}

/* Program QMSPI frequency divider field in mode register */
static void qspi_set_frequency(struct qmspi_regs *regs, uint32_t freq_hz)
{
	uint32_t fdiv, mode;

	fdiv = qspi_encoded_fdiv(freq_hz);
	mode = regs->MODE & ~(XEC_QSPI_M_CLK_DIV_MASK);
	mode |= ((fdiv << XEC_QSPI_M_CLK_DIV_POS) & XEC_QSPI_M_CLK_DIV_MASK);
	regs->MODE = mode;
}

static uint32_t qspi_get_frequency(struct qmspi_regs *regs)
{
	uint32_t src_clk = qspi_source_clock_freq();
	uint32_t fdiv = (regs->MODE & XEC_QSPI_M_CLK_DIV_MASK)
			>> XEC_QSPI_M_CLK_DIV_POS;

	if (fdiv == 0) {
		fdiv = CLOCK_DIV_0_VALUE;
	}

	return (src_clk / fdiv);
}

/*
 * SPI signalling mode: CPOL and CPHA
 * CPOL = 0 is clock idle state is low, 1 is clock idle state is high
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
 * Data sheet recommends when QMSPI set at >= 48MHz, sample and change data
 * on the same edge.
 *  Mode 0: CPOL=0 CPHA=0 (CHPA_MISO=1 and CHPA_MOSI=0)
 *  Mode 3: CPOL=1 CPHA=1 (CHPA_MISO=0 and CHPA_MOSI=1)
 *
 * smode_tbl and smode48_tbl has the byte values for Mode 0, 1, 2, 3
 *
 * Byte values correspond to bits 8. 9. 10 in QMSPI Mode Register
 * Bit 8 - CPOL
 * Bit 9 - CHPA MOSI
 * Bit 10 - CHPA MISO
 */
const uint8_t smode_tbl[4] = {
	0x00u, 0x06u, 0x01u, 0x07u
};

const uint8_t smode48_tbl[4] = {
	0x04u, 0x02u, 0x05u, 0x03u
};

static void qspi_set_signalling_mode(struct qmspi_regs *regs, uint32_t smode)
{
	const uint8_t *ptbl;
	uint32_t m;

	ptbl = smode_tbl;
	if (qspi_get_frequency(regs) >= MHZ(48)) {
		ptbl = smode48_tbl;
	}

	m = (uint32_t)ptbl[smode & GENMASK(1, 0)];
	regs->MODE = (regs->MODE & ~(XEC_QSPI_M_CP_MSK)) |
		     (m << XEC_QSPI_M_CPOL_POS);
}

static uint8_t npins_from_spi_config(const struct spi_config *config)
{
	uint8_t lines = 1u;

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES)) {
		switch (config->operation & SPI_LINES_MASK) {
		case SPI_LINES_DUAL:
			lines = 2u;
			break;
		case SPI_LINES_QUAD:
			lines = 4u;
			break;
		default:
			lines = 1u;
			break;
		}
	}

	return lines;
}

/*
 * Configure QSPI.
 * NOTE: QSPI Port 0 has two chip selects available. Ports 1 & 2
 * support only CS0#.
 */
static int qspi_configure(const struct device *dev,
			  const struct spi_config *spi_conf)
{
	const struct spi_xec_qspi_config * const cfg = dev->config;
	struct spi_xec_qspi_data * const qdata = dev->data;
	struct qmspi_regs * const regs = cfg->regs;
	struct spi_context *ctx = &qdata->ctx;
	uint32_t smode;

	if (spi_context_configured(ctx, spi_conf)) {
		return 0;
	}

	if (spi_conf->operation & (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE
				 | SPI_MODE_LOOP)) {
		return -ENOTSUP;
	}

	if ((IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	     ((spi_conf->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE))) {
		LOG_ERR("Single(full-duplex) only");
		return -EINVAL;
	}

	if (spi_conf->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(spi_conf->operation) != 8) {
		LOG_ERR("Word size != 8 not supported");
		return -ENOTSUP;
	}

	smode = SPI_LINES_SINGLE;
	qdata->np = npins_from_spi_config(spi_conf);
	regs->CTRL = smode;

	/* Use the requested or next highest possible frequency */
	qspi_set_frequency(regs, spi_conf->frequency);

	smode = 0;
	if ((spi_conf->operation & SPI_MODE_CPHA) != 0U) {
		smode |= (BIT(0));
	}

	if ((spi_conf->operation & SPI_MODE_CPOL) != 0U) {
		smode |= (BIT(1));
	}

	qspi_set_signalling_mode(regs, smode);

	/* chip select */
	smode = regs->MODE & ~(XEC_QSPI_M_CS_SEL_MSK);
	if (cfg->chip_sel == 0) {
		smode |= XEC_QSPI_M_CS0_SEL;
	} else {
		smode |= XEC_QSPI_M_CS1_SEL;
	}
	regs->MODE = smode;

	/* chip select timing */
	regs->CSTM = cfg->cs_timing;

	regs->TM_TAPS_ADJ = cfg->taps_adj;
	/* CS1 alternate mode (frequency) */
	regs->MODE_ALT1 = 0;
	if (cfg->cs1_freq) {
		uint32_t fdiv = qspi_encoded_fdiv(cfg->cs1_freq);

		regs->MODE_ALT1 = (fdiv << XEC_QSPI_MALT1_CLK_DIV_POS) &
				  XEC_QSPI_MALT1_CLK_DIV_MSK;
		regs->MODE_ALT1 |= BIT(XEC_QSPI_MALT1_EN_POS);
	}

	ctx->config = spi_conf;

	regs->MODE |= BIT(XEC_QSPI_M_ACTV_POS);

	return 0;
}

static uint32_t encode_npins(uint8_t npins)
{
	uint32_t encoding = XEC_QSPI_C_IFC_1X;

	if (npins == 4) {
		encoding = XEC_QSPI_C_IFC_4X;
	} else if (npins == 2) {
		encoding = XEC_QSPI_C_IFC_2X;
	} else {
		encoding = XEC_QSPI_C_IFC_1X;
	}

	return encoding;
}

/* Allocate QMSPI HW descriptor registers to process the given number of bytes
 * or until all descriptors are allocated. Returns the number of remaining
 * bytes not in allocation. Updates the word pointed to by ndescr with the
 * number of descriptors allocated. Descriptor allocation always begins with
 * descriptor 0. Descriptor QMSPI unit size, number of units, and next
 * descriptor fields are programmed other fields in descr_base are preserved.
 */
static size_t descr_alloc(struct qmspi_regs * const regs, size_t nbytes,
			  uint32_t descr_base, uint32_t *ndescr)
{
	size_t nb = nbytes;
	uint32_t idx = 0u;
	uint32_t descr = 0u;

	descr_base &= ~(XEC_QSPI_C_Q_XFR_UNITS_MSK | XEC_QSPI_C_Q_NUNITS_MSK |
			XEC_QSPI_C_FN_DESCR_MSK);

	while (nb && (idx < 16)) {
		if (nb <= XEC_QSPI_C_Q_NUNITS_MAX) {
			descr = nb;
			descr <<= XEC_QSPI_C_Q_NUNITS_POS;
			descr |= XEC_QSPI_C_Q_XFR_UNITS_1B;
			nb = 0u;
		} else {
			descr = (nb >> 4);
			nb -= (descr << 4);
			descr <<= XEC_QSPI_C_Q_NUNITS_POS;
			descr |= XEC_QSPI_C_Q_XFR_UNITS_16B;
		}

		descr |= (XEC_QSPI_C_IFC_1X | XEC_QSPI_C_TX_EN_DATA |
			  BIT(XEC_QSPI_C_RX_EN_POS));
		descr |= XEC_QSPI_C_FN_DESCR((idx + 1u));

		regs->DESCR[idx++] = descr_base | descr;
	}

	if (idx) {
		regs->DESCR[idx - 1u] |= BIT(XEC_QSPI_D_DESCR_LAST_POS);
	}

	if (ndescr) {
		*ndescr = idx;
	}

	return nb;
}

/* Polling full-duplex transfer using QMSPI descriptors and FIFO's.
 * Allocate hardware descriptors for maximum total transfer size.
 * Descriptors configured for both transmit and receive.
 * If TX context has no data, transmit 0 bytes.
 * If RX context has no buffer, throw away received bytes.
 * If user set SPI_HOLD_ON_CS flag configure to not de-assert chip select
 * when the last descriptor is completed. When transfer is completed without
 * error mark context complete.
 */
static int xec_qspi_fd_descr(const struct device *dev,
			     const struct spi_config *spi_conf,
			     const struct spi_buf_set *tx_bufs,
			     const struct spi_buf_set *rx_bufs)
{
	const struct spi_xec_qspi_config *cfg = dev->config;
	struct spi_xec_qspi_data * const qdata = dev->data;
	struct qmspi_regs * const regs = cfg->regs;
	struct spi_context *ctx = &qdata->ctx;
	uint32_t descr_base, nd;
	size_t len, ntx, nrx, rem, xfr_len;
	uint8_t txb, rxb;
	bool close = true;

	xfr_len = MAX(spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx));
	if (!xfr_len) {
		return 0;
	}

	regs->CTRL = 0;
	regs->EXE = BIT(XEC_QSPI_EXE_CLR_FIFOS_POS);
	regs->STS |= regs->STS;
	regs->CTRL = BIT(XEC_QSPI_C_DESCR_MODE_EN_POS);

	descr_base = encode_npins(qdata->np);
	descr_base |= (XEC_QSPI_C_TX_EN_DATA | BIT(XEC_QSPI_C_RX_EN_POS) |
		       BIT(XEC_QSPI_C_CLOSE_POS));

	if (spi_conf->operation & SPI_HOLD_ON_CS) {
		close = false;
	}

	len = xfr_len;
	while (len) {
		nd = 0u;
		rem = descr_alloc(regs, len, descr_base, &nd);

		__ASSERT_NO_MSG(nd != 0);
		__ASSERT_NO_MSG(rem < len);

		if ((rem == 0) && close) {
			regs->DESCR[nd - 1u] |= BIT(XEC_QSPI_C_CLOSE_POS);
		}

		/* NOTE: start with TX FIFO empty causes read-only TX stall
		 * status to be set.
		 */
		regs->EXE = BIT(XEC_QSPI_EXE_START_POS);

		ntx = len - rem;
		nrx = ntx;
		while (ntx || nrx) {
			if (regs->STS & XEC_QSPI_STS_ERRORS) {
				LOG_ERR("QMSPI errors(sts): 0x%08x\n", regs->STS);
				return -EIO;
			}
			if (ntx && !(regs->STS & BIT(XEC_QSPI_STS_TXB_FULL_POS))) {
				txb = 0u;
				if (ctx->tx_buf) {
					txb = *(uint8_t *)(ctx->tx_buf);
				}
				sys_write8(txb, (mem_addr_t)&regs->TX_FIFO);
				spi_context_update_tx(ctx, 1, 1);
				ntx--;
			}
			if (nrx && !(regs->STS & BIT(XEC_QSPI_STS_RXB_EMPTY_POS))) {
				rxb = sys_read8((mem_addr_t)&regs->RX_FIFO);
				if (ctx->rx_buf) {
					*(uint8_t *)(ctx->rx_buf) = rxb;
				}
				spi_context_update_rx(ctx, 1, 1);
				nrx--;
			}
		}

		len = rem;
	}

	spi_context_complete(ctx, dev, 0);

	return 0;
}

static int xec_qspi_xfr(const struct device *dev,
			const struct spi_config *spi_conf,
			const struct spi_buf_set *tx_bufs,
			const struct spi_buf_set *rx_bufs,
			bool asynchronous)
{
	const struct spi_xec_qspi_config *cfg = dev->config;
	struct spi_xec_qspi_data * const qdata = dev->data;
	struct qmspi_regs * const regs = cfg->regs;
	struct spi_context *ctx = &qdata->ctx;
	int ret = 0;

	spi_context_lock(ctx, asynchronous, NULL, NULL, spi_conf);

	ret = qspi_configure(dev, spi_conf);
	if (ret != 0) {
		spi_context_release(ctx, ret);
		return ret;
	}

	spi_context_cs_control(&qdata->ctx, true);
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	ret = xec_qspi_fd_descr(dev, spi_conf, tx_bufs, rx_bufs);
	if (ret) {
		regs->EXE = BIT(XEC_QSPI_EXE_STOP_POS);
		spi_context_unlock_unconditionally(&qdata->ctx);
		return ret;
	}

	if (!(spi_conf->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(ctx, false);
	}

	/* Attempts to take semaphore with timeout. Descriptor transfer
	 * routine completes the context giving the semaphore.
	 */
	ret = spi_context_wait_for_completion(ctx);
	/* gives semaphore */
	spi_context_release(ctx, ret);

	return ret;
}

static int xec_qspi_transceive(const struct device *dev,
				     const struct spi_config *spi_conf,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	return xec_qspi_xfr(dev, spi_conf, tx_bufs, rx_bufs, false);
}

#ifdef CONFIG_SPI_ASYNC
static int xec_qspi_transceive_async(const struct device *dev,
				     const struct spi_config *spi_conf,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif

static int xec_qspi_release(const struct device *dev,
			    const struct spi_config *spi_conf)
{
	struct spi_xec_qspi_data * const qdata = dev->data;
	const struct spi_xec_qspi_config *cfg = dev->config;
	struct qmspi_regs * const regs = cfg->regs;
	struct spi_context *ctx = &qdata->ctx;
	int ret = 0;
	int counter = 0;

	if (regs->STS & BIT(XEC_QSPI_STS_XFR_ACTIVE_POS)) {
		/* Force CS# to de-assert on next unit boundary */
		regs->EXE = BIT(XEC_QSPI_EXE_STOP_POS);
		while (regs->STS & BIT(XEC_QSPI_STS_XFR_ACTIVE_POS)) {
			ret = xec_qspi_spin_yield(&counter,
						  XEC_QSPI_WAIT_COUNT);
			if (ret != 0) {
				break;
			}
		}
	}

	spi_context_unlock_unconditionally(ctx);

	return ret;
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
static int xec_qspi_init(const struct device *dev)
{
	const struct spi_xec_qspi_config *cfg = dev->config;
	struct spi_xec_qspi_data * const qdata = dev->data;
	struct qmspi_regs * const regs = cfg->regs;
	int ret = 0;

	qdata->qstatus = 0;
	qdata->np = cfg->width;

	z_mchp_xec_pcr_periph_sleep(cfg->pcr_idx, cfg->pcr_pos, 0);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("QSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	xec_qspi_reset(regs);

	spi_context_unlock_unconditionally(&qdata->ctx);

	return 0;
}

static const struct spi_driver_api spi_xec_qspi_driver_api = {
	.transceive = xec_qspi_transceive,
	.release = xec_qspi_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = xec_qspi_transceive_async,
#endif
};

#define XEC_QSPI_CS_TIMING_VAL(a, b, c, d) (((a) & 0xFu) \
					    | (((b) & 0xFu) << 8) \
					    | (((c) & 0xFu) << 16) \
					    | (((d) & 0xFu) << 24))

#define XEC_QSPI_TAPS_ADJ_VAL(a, b) (((a) & 0xffu) | (((b) & 0xffu) << 8))

#define XEC_QSPI_CS_TIMING(i) XEC_QSPI_CS_TIMING_VAL(			\
				DT_INST_PROP_OR(i, dcsckon, 6),		\
				DT_INST_PROP_OR(i, dckcsoff, 4),	\
				DT_INST_PROP_OR(i, dldh, 6),		\
				DT_INST_PROP_OR(i, dcsda, 6))

#define XEC_QSPI_TAPS_ADJ(i) XEC_QSPI_TAPS_ADJ_VAL(			\
				DT_INST_PROP_OR(i, tctradj, 0),		\
				DT_INST_PROP_OR(i, tsckadj, 0))

#define XEC_QSPI_GIRQ(i)						\
	MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(i, girqs, 0))

#define XEC_QSPI_GIRQ_POS(i)						\
	MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(i, girqs, 0))

#define XEC_QSPI_NVIC_AGGR(i)						\
	MCHP_XEC_ECIA_NVIC_AGGR(DT_INST_PROP_BY_IDX(i, girqs, 0))

#define XEC_QSPI_NVIC_DIRECT(i)					\
	MCHP_XEC_ECIA_NVIC_DIRECT(DT_INST_PROP_BY_IDX(i, girqs, 0))

/*
 * The instance number, i is not related to block ID's rather the
 * order the DT tools process all DT files in a build.
 */
#define XEC_QSPI_DEVICE(i)						\
									\
	PINCTRL_DT_INST_DEFINE(i);					\
									\
	static struct spi_xec_qspi_data xec_qspi_data_##i = {		\
		SPI_CONTEXT_INIT_LOCK(xec_qspi_data_##i, ctx),		\
		SPI_CONTEXT_INIT_SYNC(xec_qspi_data_##i, ctx),		\
	};								\
	static const struct spi_xec_qspi_config xec_qspi_config_##i = {	\
		.regs = (struct qmspi_regs *) DT_INST_REG_ADDR(i),	\
		.cs1_freq = DT_INST_PROP_OR(i, cs1_freq, 0),		\
		.cs_timing = XEC_QSPI_CS_TIMING(i),			\
		.taps_adj = XEC_QSPI_TAPS_ADJ(i),			\
		.girq = XEC_QSPI_GIRQ(i),				\
		.girq_pos = XEC_QSPI_GIRQ_POS(i),			\
		.girq_nvic_aggr = XEC_QSPI_NVIC_AGGR(i),		\
		.girq_nvic_direct = XEC_QSPI_NVIC_DIRECT(i),		\
		.irq_pri = DT_INST_IRQ(i, priority),			\
		.pcr_idx = DT_INST_PROP_BY_IDX(i, pcrs, 0),		\
		.pcr_pos = DT_INST_PROP_BY_IDX(i, pcrs, 1),		\
		.chip_sel = DT_INST_PROP_OR(i, chip_select, 0),		\
		.width = DT_INST_PROP_OR(i, lines, 1),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),		\
	};								\
	DEVICE_DT_INST_DEFINE(i, &xec_qspi_init, NULL,			\
		&xec_qspi_data_##i, &xec_qspi_config_##i,		\
		POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,			\
		&spi_xec_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_QSPI_DEVICE)
