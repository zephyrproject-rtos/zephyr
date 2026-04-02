/*
 * Copyright (c) 2025 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT microchip_xec_qmspi_ldma

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/dt-bindings/spi/spi.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_xec_ldma, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define XEC_FREQ_HZ_MAX MHZ(96)
#define XEC_FREQ_HZ_MIN (MHZ(96) / 0x10000U)

#define XEC_FORCE_STOP_LOOPS 100

/* If defined the driver will check the contents of
 * struct spi_config passed by the application against
 * current configuration. This allows applications that
 * re-use a non-const struct spi_config by casting it
 * as a pointer to a const structure. Apps should not do
 * this.
 * #define XEC_CHK_SPI_CONFIG_CONTENT
 */

/* Device constant configuration parameters */
struct mec5_qspi_config {
	mm_reg_t regbase;
	int clock_freq;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
	uint32_t ovrc;
	uint16_t enc_pcr;
	uint8_t clk_taps[2];
	uint8_t ctrl_taps[2];
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t dcsda;
};

#define MEC5_QSPI_XFR_FLAG_START BIT(0)
#define MEC5_QSPI_XFR_FLAG_BUSY  BIT(1)
#define MEC5_QSPI_XFR_FLAG_LDMA  BIT(2)

/* Device run time data */
struct mec5_qspi_data {
	struct spi_context ctx; /* has pointer to struct spi_config */
	struct spi_config spi_cfg;
	const struct spi_buf *rxb;
	const struct spi_buf *txb;
	size_t rxcnt;
	size_t txcnt;
	volatile uint32_t qstatus;
	volatile uint32_t xfr_flags;
	size_t total_tx_size;
	size_t total_rx_size;
	size_t chunk_size;
	uint32_t rxdb;
	uint32_t byte_time_ns;
	uint32_t freq;
	uint32_t operation;
	uint8_t cs;
};

static int spi_feature_support(const struct spi_config *config)
{
	/* NOTE: bit(11) is Half-duplex(3-wire) */
	if ((config->operation &
	     (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE | SPI_MODE_LOOP | SPI_HALF_DUPLEX)) != 0) {
		LOG_ERR("Driver does not support LSB first, slave, loop back, or half-duplex");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_CS_ACTIVE_HIGH) != 0) {
		LOG_ERR("CS active high not supported. Use invert property of CS pin");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size != 8 not supported");
		return -ENOTSUP;
	}

	return 0;
}

/* Save register Boot-ROM may have touched based on OTP.
 * Do soft reset
 * Restore saved registers
 */
static void qspi_soft_reset(const struct device *dev)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;
	uint32_t reg_save_restore[4] = {0};

	reg_save_restore[0] = sys_read32(qb + XEC_QSPI_CSTM_OFS);
	reg_save_restore[1] = sys_read32(qb + XEC_QSPI_TAPS_OFS);
	reg_save_restore[2] = sys_read32(qb + XEC_QSPI_TAPS_ADJ_OFS);
	reg_save_restore[3] = sys_read32(qb + XEC_QSPI_TAPS_CR2_OFS);

	sys_set_bit(qb + XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_SRST_POS);
	while (sys_test_bit(qb + XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_SRST_POS) != 0) {
	}

	sys_write32(reg_save_restore[0], qb + XEC_QSPI_CSTM_OFS);
	sys_write32(reg_save_restore[1], qb + XEC_QSPI_TAPS_OFS);
	sys_write32(reg_save_restore[2], qb + XEC_QSPI_TAPS_ADJ_OFS);
	sys_write32(reg_save_restore[3], qb + XEC_QSPI_TAPS_CR2_OFS);
}

/* Compute full-duplex byte time in nanoseconds
 * Multiply frequency divider by one full-duplex byte at max frequency 96 MHz
 * We over estimate to not produce a timeout too short and use 128 for power of 2 multiply
 */
static uint32_t compute_byte_time_ns(uint32_t fdiv)
{
	return (fdiv * 128u);
}

static void set_freq(const struct device *dev, uint32_t freq_hz)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec5_qspi_data *data = dev->data;
	mm_reg_t qb = devcfg->regbase;
	uint32_t fdiv = 8u; /* safe divider */
	uint32_t r = 0;

	if (freq_hz >= XEC_FREQ_HZ_MAX) {
		fdiv = 1u;
	} else if (freq_hz > XEC_FREQ_HZ_MIN) {
		fdiv = XEC_FREQ_HZ_MAX / freq_hz;
		if ((XEC_FREQ_HZ_MAX % freq_hz) >= 32768U) {
			fdiv++;
		}
		if (fdiv > UINT16_MAX) {
			fdiv = UINT16_MAX;
		}
	} else {
		fdiv = 0; /* divide by 65536 (max) */
	}

	data->byte_time_ns = compute_byte_time_ns(fdiv);

	r = sys_read32(qb + XEC_QSPI_MODE_OFS);
	r &= ~(XEC_QSPI_MODE_CK_DIV_MSK);
	r |= XEC_QSPI_MODE_CK_DIV_SET(fdiv);
	sys_write32(r, qb + XEC_QSPI_MODE_OFS);
}

static void set_taps(const struct device *dev, uint8_t cs)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;
	uint32_t r = 0, msk = 0, val = 0;

	if (devcfg->clk_taps[cs] != 0) {
		msk |= XEC_QSPI_TAPS_CK_SEL_MSK;
		val |= XEC_QSPI_TAPS_CK_SEL_SET((uint32_t)devcfg->clk_taps[cs]);
	}

	if (devcfg->ctrl_taps[cs] != 0) {
		msk |= XEC_QSPI_TAPS_CR_SEL_MSK;
		val |= XEC_QSPI_TAPS_CR_SEL_SET((uint32_t)devcfg->ctrl_taps[cs]);
	}

	if (msk != 0) {
		r = sys_read32(qb + XEC_QSPI_TAPS_OFS);
		r = (r & ~msk) | val;
		sys_write32(r, qb + XEC_QSPI_TAPS_OFS);
	}
}

static void set_cs(const struct device *dev, uint8_t cs)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;
	uint32_t r = sys_read32(qb + XEC_QSPI_MODE_OFS);

	r &= ~(XEC_QSPI_MODE_CS_SEL_MSK);
	r |= XEC_QSPI_MODE_CS_SEL_SET((uint32_t)cs);
	sys_write32(r, qb + XEC_QSPI_MODE_OFS);
}

static void set_cs_timing(const struct device *dev, const struct spi_config *config)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	const struct spi_cs_control *pcs = &config->cs;
	mm_reg_t qb = devcfg->regbase;
	uint32_t r = 0, d = 0, msk = 0, val = 0;

	if (devcfg->dcsda != 0) {
		msk |= XEC_QSPI_CSTM_CSD_CSA_MSK;
		val |= XEC_QSPI_CSTM_CSD_CSA_SET((uint32_t)devcfg->dcsda);
	}

	if (pcs->cs_is_gpio == true) {
		return;
	}

	d = pcs->setup_ns / 10u;
	if (d != 0) {
		msk |= XEC_QSPI_CSTM_CSA_FCK_MSK;
		val |= XEC_QSPI_CSTM_CSA_FCK_SET(d);
	}

	d = pcs->hold_ns / 10u;
	if (d != 0) {
		msk |= XEC_QSPI_CSTM_LCK_CSD_MSK;
		val |= XEC_QSPI_CSTM_LCK_CSD_SET(d);
	}

	if (msk != 0) {
		r = sys_read32(qb + XEC_QSPI_CSTM_OFS);
		r = (r & ~msk) | val;
		sys_write32(r, qb + XEC_QSPI_CSTM_OFS);
	}
}

/* Mode SPI_MODE_CPOL : SPI_MODE_CPHA
 *  0      0               0           clock idle lo, data sampled on RE, shifted on FE
 *  1      0               1           clock idle lo, data sampled on FE, shifted on RE
 *  2      1               0           clock idle hi, data sampled on FE, shifted on RE
 *  3      1               1           clock idle hi, data sampled on RE, shifted on FE
 *       CPOL CPHA_SDI CPHA_SDO
 *  0     0     0         0
 *  1     0     1         1
 *  2     1     0         0
 *  3     1     1         1
 *
 * QSPI SPI frequency >= 48 MHz requires:
 * Mode 0: CPOL=0, CPHA_SDO=1, CPHA_SDI=0
 * Mode 3: CPOL=1, CPHA_SDO=0, CPHA_SDI=1
 */
static const uint8_t qspi_signal_mode[4] = {0, 0x6, 0x1, 0x3};

static void set_data_phase(const struct device *dev)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec5_qspi_data *data = dev->data;
	mm_reg_t qb = devcfg->regbase;
	uint32_t sgm = 0, r = 0, v = 0;

	/* request SPI clock inactive state is high */
	if ((data->operation & SPI_MODE_CPOL) != 0) {
		sgm |= BIT(0);
	}

	if ((data->operation & SPI_MODE_CPHA) != 0) {
		sgm |= BIT(1);
	}

	v = qspi_signal_mode[sgm];
	v <<= XEC_QSPI_MODE_CPOL_HI_POS;
	if (data->freq >= MHZ(48)) {
		v ^= BIT(XEC_QSPI_MODE_CPHA_SDO_SE_POS);
	}

	r = sys_read32(qb + XEC_QSPI_MODE_OFS);
	r &= ~(XEC_QSPI_MODE_CP_MSK);
	r |= v;
	sys_write32(r, qb + XEC_QSPI_MODE_OFS);
}

static void set_duplex(const struct device *dev, uint8_t duplex)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;
	uint32_t r = sys_read32(qb + XEC_QSPI_CR_OFS);

	r &= ~(XEC_QSPI_CR_IFM_MSK);
	r |= XEC_QSPI_CR_IFM_SET((uint32_t)duplex);
	sys_write32(r, qb + XEC_QSPI_CR_OFS);
}

static bool req_reconfig(const struct device *dev, const struct spi_config *config)
{
	const struct mec5_qspi_data *data = dev->data;
	const struct spi_context *ctx = &data->ctx;
	const struct spi_config *ctx_cfg = ctx->config;

	if (ctx_cfg != config) {
		return true;
	}
#ifdef XEC_CHK_SPI_CONFIG_CONTENT
	if (ctx_cfg->frequency != config->frequency) {
		return true;
	}

	if (ctx_cfg->operation != config->operation) {
		return true;
	}

	if (ctx_cfg->slave != config->slave) {
		return true;
	}

	if (memcmp((const void *)&ctx_cfg->cs, (const void *)&config->cs,
		   sizeof(struct spi_cs_control)) != 0) {
		return true;
	}
#endif
	return false;
}

static int mec5_qspi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec5_qspi_data *data = dev->data;
	mm_reg_t qb = devcfg->regbase;
	int ret = 0;

	if (config == NULL) {
		return -EINVAL;
	}

	if (req_reconfig(dev, config) == false) {
		return 0;
	}

	ret = spi_feature_support(config);
	if (ret != 0) {
		return ret;
	}

	/* chip select */
#ifdef DT_SPI_CTX_HAS_NO_CS_GPIOS
	if (config->slave >= XEC_QSPI_MAX_CS) {
		LOG_ERR("Invalid HW chip select [0,1]");
		return -EINVAL;
	}
#else
	if ((config->cs.cs_is_gpio == true) && (config->slave >= data->ctx.num_cs_gpios)) {
		LOG_ERR("Invalid GPIO chip select");
		return -EINVAL;
	}
#endif

	data->operation = config->operation;
	data->cs = (uint8_t)(config->slave & 0xffu);
	data->freq = config->frequency;

	if ((data->operation & SPI_HOLD_ON_CS) == 0) {
		qspi_soft_reset(dev);
	}

	set_freq(dev, data->freq);
	set_taps(dev, data->cs);
	set_cs(dev, data->cs);
	set_cs_timing(dev, config);
	set_data_phase(dev);
	set_duplex(dev, XEC_QSPI_CR_IFM_FD);

	data->ctx.config = config;

	/* clear status */
	sys_write32(UINT32_MAX, qb + XEC_QSPI_SR_OFS);
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	/* enable */
	sys_set_bit(qb + XEC_QSPI_MODE_OFS, XEC_QSPI_MODE_ACTV_POS);

	return 0;
}

static int qspi_force_stop(const struct device *dev)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;
	uint32_t loops = XEC_FORCE_STOP_LOOPS;

	/* Is QSPI is asserting CS ? */
	if (sys_test_bit(qb + XEC_QSPI_SR_OFS, XEC_QSPI_SR_CS_ASSERTED_POS) == 0) {
		return 0; /* No */
	}

	/* Force it to stop */
	sys_write32(BIT(XEC_QSPI_EXE_STOP_POS), qb + XEC_QSPI_EXE_OFS);

	/* wait one unit time where a unit is 1, 2, or 4 bytes */
	while (loops != 0) {
		loops--;
		if (sys_test_bit(qb + XEC_QSPI_SR_OFS, XEC_QSPI_SR_CS_ASSERTED_POS) == 0) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int mec5_qspi_do_xfr(const struct device *dev, const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			    bool async, spi_callback_t cb, void *userdata)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec5_qspi_data *data = dev->data;
	mm_reg_t qb = devcfg->regbase;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	if ((data->xfr_flags & MEC5_QSPI_XFR_FLAG_BUSY) != 0) {
		return -EBUSY;
	}

	if ((tx_bufs == NULL) && (rx_bufs == NULL)) {
		return -EINVAL;
	}

	spi_context_lock(ctx, async, cb, userdata, config);

	ret = mec5_qspi_configure(dev, config);
	if (ret != 0) {
		goto do_xfr_exit;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1u);

	data->chunk_size = 0;
	data->total_tx_size = spi_context_total_tx_len(ctx);
	data->total_rx_size = spi_context_total_rx_len(ctx);
	data->xfr_flags = MEC5_QSPI_XFR_FLAG_START;

	/* trigger an empty TX FIFO interrupt to enter the ISR */
	sys_set_bit(qb + XEC_QSPI_IER_OFS, XEC_QSPI_IER_TXB_EMPTY_POS);

	ret = spi_context_wait_for_completion(ctx);

	if (async && (ret == 0)) {
		return 0;
	}

	if (ret != 0) {
		qspi_force_stop(dev);
	}

do_xfr_exit:
	spi_context_release(ctx, 0);

	return ret;
}

static int mec5_qspi_xfr_check1(const struct spi_config *config)
{

	if (soc_taf_enabled() != 0) {
		return -EPERM;
	}

	if (config == NULL) {
		return -EINVAL;
	}

	return 0;
}

static int mec5_qspi_xfr_sync(const struct device *dev, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	int ret = mec5_qspi_xfr_check1(config);

	if (ret != 0) {
		return ret;
	}

	return mec5_qspi_do_xfr(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int mec5_qspi_xfr_async(const struct device *dev, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			       spi_callback_t cb, void *userdata)
{
	int ret = mec5_qspi_xfr_check1(config);

	if (ret != 0) {
		return ret;
	}

	return mec5_qspi_do_xfr(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int mec5_qspi_release(const struct device *dev, const struct spi_config *config)
{
	struct mec5_qspi_data *qdata = dev->data;
	int ret = 0;

	if (soc_taf_enabled() != 0) {
		return -EPERM;
	}

	ret = qspi_force_stop(dev);

	/* increments lock semphare in ctx up to initial limit */
	spi_context_unlock_unconditionally(&qdata->ctx);

	return ret;
}

static void qspi_ldma_init(const struct device *dev)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;

	sys_clear_bits(qb + XEC_QSPI_MODE_OFS,
		       (BIT(XEC_QSPI_MODE_LD_RX_EN_POS) | BIT(XEC_QSPI_MODE_LD_TX_EN_POS)));
	sys_write32(0, qb + XEC_QSPI_LDMA_RX_EN_OFS);
	sys_write32(0, qb + XEC_QSPI_LDMA_TX_EN_OFS);
	sys_write32(BIT(XEC_QSPI_EXE_CLR_FIFOS_POS), qb + XEC_QSPI_EXE_OFS);
	for (uint32_t i = 0; i < XEC_QSPI_LDMA_CHX_MAX; i++) {
		sys_write32(0, qb + XEC_QSPI_LDMA_CHX_CR_OFS(i));
		sys_write32(0, qb + XEC_QSPI_LDMA_CHX_SA_OFS(i));
		sys_write32(0, qb + XEC_QSPI_LDMA_CHX_LR_OFS(i));
	}
}

#define XEC_QSPI_ULDMA_FLAG_START_POS   0
#define XEC_QSPI_ULDMA_FLAG_IEN_POS     1
#define XEC_QSPI_ULDMA_FLAG_CLOSE_POS   2
#define XEC_QSPI_ULDMA_FLAG_INCR_TX_POS 4
#define XEC_QSPI_ULDMA_FLAG_INCR_RX_POS 5

#define ULDMA_DESCR0                                                                               \
	(XEC_QSPI_CR_IFM_SET(XEC_QSPI_CR_IFM_FD) | XEC_QSPI_CR_TXM_SET(XEC_QSPI_CR_TXM_DATA) |     \
	 XEC_QSPI_CR_TXDMA_SET(XEC_QSPI_CR_TXDMA_TLDCH0) | BIT(XEC_QSPI_CR_RX_EN_POS) |            \
	 XEC_QSPI_CR_RXDMA_SET(XEC_QSPI_CR_RXDMA_RLDCH0) | BIT(XEC_QSPI_DR_LD_POS))

#define ULDMA_IEN                                                                                  \
	(BIT(XEC_QSPI_IER_XFR_DONE_POS) | BIT(XEC_QSPI_IER_TXB_ERR_POS) |                          \
	 BIT(XEC_QSPI_IER_RXB_ERR_POS) | BIT(XEC_QSPI_IER_PROG_ERR_POS) |                          \
	 BIT(XEC_QSPI_IER_LDMA_RX_ERR_POS) | BIT(XEC_QSPI_IER_LDMA_TX_ERR_POS))

static int qspi_uldma_fd2(const struct device *dev, const uint8_t *txb, uint8_t *rxb, size_t xfrlen,
			  uint32_t flags)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;
	uint32_t d = 0, rxcr = 0, txcr = 0;

	if (xfrlen == 0) {
		return -EINVAL;
	}

	qspi_ldma_init(dev);

	rxcr = (BIT(XEC_QSPI_LDMA_CHX_CR_EN_POS) | BIT(XEC_QSPI_LDMA_CHX_CR_OVRL_POS) |
		XEC_QSPI_LDMA_CHX_CR_SZ_SET(XEC_QSPI_LDMA_CHX_CR_SZ_1B));
	txcr = rxcr;
	if ((flags & BIT(XEC_QSPI_ULDMA_FLAG_INCR_RX_POS)) != 0) {
		rxcr |= BIT(XEC_QSPI_LDMA_CHX_CR_INCRA_POS);
	}
	if ((flags & BIT(XEC_QSPI_ULDMA_FLAG_INCR_TX_POS)) != 0) {
		txcr |= BIT(XEC_QSPI_LDMA_CHX_CR_INCRA_POS);
	}

	d = ULDMA_DESCR0;
	if ((flags & BIT(XEC_QSPI_ULDMA_FLAG_CLOSE_POS)) != 0) {
		d |= BIT(XEC_QSPI_CR_CLOSE_EN_POS);
	}

	sys_write32(xfrlen, qb + XEC_QSPI_LDMA_CHX_LR_OFS(XEC_QSPI_LDMA_RX_CH0));
	sys_write32((uint32_t)rxb, qb + XEC_QSPI_LDMA_CHX_SA_OFS(XEC_QSPI_LDMA_RX_CH0));
	sys_write32(rxcr, qb + XEC_QSPI_LDMA_CHX_CR_OFS(XEC_QSPI_LDMA_RX_CH0));

	sys_write32(xfrlen, qb + XEC_QSPI_LDMA_CHX_LR_OFS(XEC_QSPI_LDMA_TX_CH0));
	sys_write32((uint32_t)txb, qb + XEC_QSPI_LDMA_CHX_SA_OFS(XEC_QSPI_LDMA_TX_CH0));
	sys_write32(txcr, qb + XEC_QSPI_LDMA_CHX_CR_OFS(XEC_QSPI_LDMA_TX_CH0));

	sys_write32(d, qb + XEC_QSPI_DESCR_OFS(0));
	sys_write32(0, qb + XEC_QSPI_DESCR_OFS(1));

	/* Mark descriptor 0 as using both RX and TX local-DMA */
	sys_write32(BIT(0), qb + XEC_QSPI_LDMA_RX_EN_OFS);
	sys_write32(BIT(0), qb + XEC_QSPI_LDMA_TX_EN_OFS);

	/* Enable RX and TX local-DMA in mode register */
	sys_set_bits(qb + XEC_QSPI_MODE_OFS,
		     (BIT(XEC_QSPI_MODE_LD_RX_EN_POS) | BIT(XEC_QSPI_MODE_LD_TX_EN_POS)));

	/* Enable descriptor mode in control register */
	sys_set_bit(qb + XEC_QSPI_CR_OFS, XEC_QSPI_CR_DESCR_EN_POS);

	d = 0;
	if ((flags & BIT(XEC_QSPI_ULDMA_FLAG_IEN_POS)) != 0) {
		d |= ULDMA_IEN;
	}
	sys_write32(d, qb + XEC_QSPI_IER_OFS);

	if ((flags & BIT(XEC_QSPI_ULDMA_FLAG_START_POS)) != 0) {
		sys_write32(BIT(XEC_QSPI_EXE_START_POS), qb + XEC_QSPI_EXE_OFS);
	}

	return 0;
}

/* ISR helper */
static void mec5_qspi_ctx_next(const struct device *dev)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec5_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	mm_reg_t qb = devcfg->regbase;
	uint32_t qflags = BIT(XEC_QSPI_ULDMA_FLAG_START_POS) | BIT(XEC_QSPI_ULDMA_FLAG_IEN_POS);
	size_t xlen = 0;
	int rc = 0;

	spi_context_update_tx(ctx, 1u, data->chunk_size);
	spi_context_update_rx(ctx, 1u, data->chunk_size);

	if (data->total_tx_size != 0) {
		data->total_tx_size -= data->chunk_size;
	}

	if (data->total_rx_size != 0) {
		data->total_rx_size -= data->chunk_size;
	}

	if ((spi_context_rx_on(ctx) == true) || (spi_context_tx_on(ctx) == true)) {
		xlen = spi_context_max_continuous_chunk(ctx);
		data->chunk_size = xlen;

		uint8_t const *txb = ctx->tx_buf;
		uint8_t *rxb = ctx->rx_buf;

		if (txb != NULL) {
			qflags |= BIT(XEC_QSPI_ULDMA_FLAG_INCR_TX_POS);
		} else {
			txb = (const uint8_t *)&devcfg->ovrc;
		}

		if (rxb != NULL) {
			qflags |= BIT(XEC_QSPI_ULDMA_FLAG_INCR_RX_POS);
		} else {
			rxb = (uint8_t *)&data->rxdb;
		}

		if ((data->total_tx_size <= xlen) && (data->total_rx_size <= xlen)) {
			qflags |= BIT(XEC_QSPI_ULDMA_FLAG_CLOSE_POS);
		}

		data->xfr_flags = MEC5_QSPI_XFR_FLAG_LDMA;

		if (sys_test_bit(qb + XEC_QSPI_SR_OFS, XEC_QSPI_SR_CS_ASSERTED_POS) == 0) {
			spi_context_cs_control(ctx, true);
		}

		rc = qspi_uldma_fd2(dev, (const uint8_t *)txb, rxb, xlen, qflags);
		if (rc != 0) {
			spi_context_cs_control(ctx, false);
			spi_context_complete(&data->ctx, dev, -EIO);
		}
	} else {
		if ((ctx->config->operation & SPI_HOLD_ON_CS) == 0) {
			spi_context_cs_control(ctx, false);
		}

		spi_context_complete(&data->ctx, dev, 0);
	}
}

#define XEC_QSPI_ALL_ERRORS                                                                        \
	(BIT(XEC_QSPI_SR_TXB_ERR_POS) | BIT(XEC_QSPI_SR_RXB_ERR_POS) |                             \
	 BIT(XEC_QSPI_SR_PROG_ERR_POS) | BIT(XEC_QSPI_SR_LDMA_RX_ERR_POS) |                        \
	 BIT(XEC_QSPI_SR_LDMA_TX_ERR_POS))

static void mec5_qspi_isr(const struct device *dev)
{
	struct mec5_qspi_data *data = dev->data;
	const struct mec5_qspi_config *devcfg = dev->config;
	mm_reg_t qb = devcfg->regbase;
	uint32_t hwsts = 0u;
	int status = 0;

	hwsts = sys_read32(qb + XEC_QSPI_SR_OFS);
	data->qstatus = hwsts;

	if ((hwsts & XEC_QSPI_ALL_ERRORS) != 0) {
		status = -EIO;
	} else if ((hwsts & BIT(XEC_QSPI_SR_XFR_DONE_POS)) == 0) {
		status = -EBUSY;
	} else {
		status = 0;
	}

	sys_write32(0, qb + XEC_QSPI_IER_OFS);
	sys_write32(UINT32_MAX, qb + XEC_QSPI_SR_OFS);
	soc_ecia_girq_status_clear(devcfg->girq, devcfg->girq_pos);

	if (status == -EIO) {
		spi_context_complete(&data->ctx, dev, -EIO);
		return;
	}

	if ((data->xfr_flags & MEC5_QSPI_XFR_FLAG_START) != 0) {
		data->xfr_flags &= (uint32_t)~MEC5_QSPI_XFR_FLAG_START;
	}

	mec5_qspi_ctx_next(dev);
}

/*
 * Called for each QSPI controller by the kernel during driver load phase
 * specified in the device initialization structure below.
 * Initialize QSPI controller.
 * Initialize SPI context.
 * QSPI will be fully configured and enabled when the transceive API
 * is called.
 */
static int mec5_qspi_init(const struct device *dev)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec5_qspi_data *data = dev->data;
	int ret = 0;

	data->cs = 0;
	data->freq = devcfg->clock_freq;

	/* clear PCR sleep enable bit for this controller */
	soc_xec_pcr_sleep_en_clear(devcfg->enc_pcr);

	const struct spi_config spi_cfg = {
		.frequency = devcfg->clock_freq,
		.operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE,
		.slave = 0,
		.word_delay = 0,
	};

	ret = mec5_qspi_configure(dev, &spi_cfg);
	if (ret != 0) {
		LOG_ERR("QSPI init error (%d)", ret);
		return -EINVAL;
	}

	ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("QSPI pinctrl setup failed (%d)", ret);
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret != 0) {
		LOG_ERR("QSPI cs config failed (%d)", ret);
		return ret;
	}

	if ((devcfg->irq_config_func) != NULL) {
		devcfg->irq_config_func();
		soc_ecia_girq_ctrl(devcfg->girq, devcfg->girq_pos, 1u);
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

static DEVICE_API(spi, mec5_qspi_driver_api) = {
	.transceive = mec5_qspi_xfr_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = mec5_qspi_xfr_async,
#endif
	.release = mec5_qspi_release,
};

#define XEC_QSPI_DT_GIRQ(inst)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0))
#define XEC_QSPI_DT_GIRQ_POS(inst) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, 0))

#define XEC_QSPI_DT_TAPS(inst, prop, idx)                                                          \
	COND_CODE_1(DT_PROP_HAS_IDX(inst, prop, idx), (DT_INST_PROP_BY_IDX(inst, prop, idx)), (0))

/* The instance number, i is not related to block ID's rather the
 * order the DT tools process all DT files in a build.
 */
#define MEC5_QSPI_DEVICE(i)                                                                        \
	PINCTRL_DT_INST_DEFINE(i);                                                                 \
	static void mec5_qspi_irq_config_##i(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), mec5_qspi_isr,              \
			    DEVICE_DT_INST_GET(i), 0);                                             \
		irq_enable(DT_INST_IRQN(i));                                                       \
	}                                                                                          \
	static struct mec5_qspi_data mec5_qspi_data_##i = {                                        \
		SPI_CONTEXT_INIT_LOCK(mec5_qspi_data_##i, ctx),                                    \
		SPI_CONTEXT_INIT_SYNC(mec5_qspi_data_##i, ctx),                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(i), ctx)};                             \
	static const struct mec5_qspi_config mec5_qspi_config_##i = {                              \
		.regbase = (mm_reg_t)DT_INST_REG_ADDR(i),                                          \
		.clock_freq = DT_INST_PROP_OR(i, clock_frequency, MHZ(12)),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),                                         \
		.irq_config_func = mec5_qspi_irq_config_##i,                                       \
		.enc_pcr = DT_INST_PROP(i, pcr),                                                   \
		.clk_taps[0] = (uint8_t)XEC_QSPI_DT_TAPS(i, clock_taps, 0),                        \
		.clk_taps[1] = (uint8_t)XEC_QSPI_DT_TAPS(i, clock_taps, 1),                        \
		.ctrl_taps[0] = (uint8_t)XEC_QSPI_DT_TAPS(i, ctrl_taps, 0),                        \
		.ctrl_taps[1] = (uint8_t)XEC_QSPI_DT_TAPS(i, ctrl_taps, 1),                        \
		.girq = (uint8_t)XEC_QSPI_DT_GIRQ(i),                                              \
		.girq_pos = (uint8_t)XEC_QSPI_DT_GIRQ_POS(i),                                      \
		.dcsda = DT_INST_PROP_OR(i, dcsda, 6u),                                            \
		.ovrc = (uint32_t)DT_INST_PROP_OR(i, overrun_character, 0),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(i, &mec5_qspi_init, NULL, &mec5_qspi_data_##i,                       \
			      &mec5_qspi_config_##i, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,        \
			      &mec5_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MEC5_QSPI_DEVICE)
