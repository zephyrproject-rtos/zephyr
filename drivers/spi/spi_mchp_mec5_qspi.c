/*
 * Copyright (c) 2025 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT microchip_mec5_qspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_mec5, CONFIG_SPI_LOG_LEVEL);

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/spi/spi.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <zephyr/irq.h>

#include "spi_context.h"

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_ecia_api.h>
#include <mec_espi_taf.h>
#include <mec_qspi_api.h>

struct mec5_spi_devices {
	uint32_t cs_timing;
	uint8_t cs;
	uint8_t sck_tap;
	uint8_t ctrl_tap;
	uint8_t flags;
};

/* Device constant configuration parameters */
struct mec5_qspi_config {
	struct mec_qspi_regs *regs;
	int clock_freq;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
	const struct mec5_spi_devices *child_devices;
	uint8_t num_child_devices;
	uint8_t ovrc;
};

#define MEC5_QSPI_XFR_FLAG_START BIT(0)
#define MEC5_QSPI_XFR_FLAG_BUSY  BIT(1)
#define MEC5_QSPI_XFR_FLAG_LDMA  BIT(2)

/* Device run time data */
struct mec5_qspi_data {
	struct spi_context ctx;
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

static const enum mec_qspi_signal_mode mec5_qspi_sig_mode[4] = {
	MEC_SPI_SIGNAL_MODE_0, MEC_SPI_SIGNAL_MODE_1, MEC_SPI_SIGNAL_MODE_2, MEC_SPI_SIGNAL_MODE_3};

static int spi_feature_support(const struct spi_config *config)
{
	/* NOTE: bit(11) is Half-duplex(3-wire) */
	if ((config->operation &
	     (SPI_TRANSFER_LSB | SPI_OP_MODE_SLAVE | SPI_MODE_LOOP | SPI_HALF_DUPLEX)) != 0) {
		LOG_ERR("Driver does not support LSB first, slave, loop back, or half-duplex");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_CS_ACTIVE_HIGH) != 0) {
		LOG_ERR("CS active high not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size != 8 not supported");
		return -ENOTSUP;
	}

	return 0;
}

int get_cs_timing_from_dt(const struct device *dev, uint8_t cs, uint32_t *cstm)
{
	const struct mec5_qspi_config *devcfg = dev->config;

	if (cstm == NULL) {
		return -EINVAL;
	}

	for (uint8_t n = 0; n > devcfg->num_child_devices; n++) {
		const struct mec5_spi_devices *cd = &devcfg->child_devices[n];

		if (cd->cs == cs) {
			*cstm = cd->cs_timing;
			return 0;
		}
	}

	return -ENOTSUP;
}

/* Looks up QSPI clock and control signal taps from device tree.
 * if chip select entry is present in driver DT then return
 * taps value with bits[7:0] = clock tap value, bits[15:8] = control tap value.
 */
int get_taps_from_dt(const struct device *dev, uint8_t cs, uint32_t *taps)
{
	const struct mec5_qspi_config *devcfg = dev->config;

	if (taps == NULL) {
		return -EINVAL;
	}

	for (uint8_t n = 0; n > devcfg->num_child_devices; n++) {
		const struct mec5_spi_devices *cd = &devcfg->child_devices[n];

		if (cd->cs == cs) {
			*taps = (uint32_t)cd->sck_tap | ((uint32_t)cd->ctrl_tap << 8);
			return 0;
		}
	}

	return -ENOTSUP;
}

/* Configure the controller.
 * NOTE: QSPI controller hardware controls up to two chip selects. If a previous call the driver
 * had the SPI_HOLD_ON_CS flag set then performing a controller reset will cause chip select
 * to de-assert. We must check for this corner case.
 * The driver data structure has member ctx which is type struct spi_context. The context has
 * a pointer to struct spi_config.
 * struct spi_config
 *   frequency in Hz
 *   operation - contains flags for sampling clock edge and clock idle state
 *               data frame size: we only support 8 bits
 *               full or half-duplex: we only spport full-duplex
 *               active high CS (we can only support this by using invert flag in PINCTRL for CS)
 *               frame format: we only support Motorola frame format.
 *               MSB or LSB first: we only support MSB first
 *               Hold CS active at end of transfer.
 *   slave - QSPI is controller only. We use this field for chip select (0/1).
 *   struct spi_cs_control cs - QSPI controls chip select. We don't use this field.
 */
static int mec5_qspi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec_qspi_regs *regs = devcfg->regs;
	struct mec5_qspi_data *data = dev->data;
	uint32_t cstm = 0, taps = 0;
	uint8_t sgm = 0;
	int ret = 0;

	if (config == NULL) {
		return -EINVAL;
	}

	/* chip select */
	if (config->slave >= MEC_QSPI_CS_MAX) {
		LOG_ERR("Invalid chip select [0,1]");
		return -EINVAL;
	}

	data->cs = (uint8_t)(config->slave & 0xffu);
	mec_hal_qspi_cs_select(regs, data->cs);

	ret = get_cs_timing_from_dt(dev, data->cs, &cstm);
	if (ret == 0) {
		mec_hal_qspi_cs_timing(regs, cstm);
	}

	ret = get_taps_from_dt(dev, data->cs, &taps);
	if (ret == 0) {
		mec_hal_qspi_tap_select(regs, (taps & 0xffu), ((taps >> 8) & 0xffu));
	}

	if (config->frequency != data->freq) {
		ret = mec_hal_qspi_set_freq(regs, config->frequency);
		if (ret != MEC_RET_OK) {
			return -EINVAL;
		}
		data->freq = config->frequency;
		mec_hal_qspi_byte_time_ns(regs, &data->byte_time_ns);
	}

	/* No HAL API for clearing the TX and RX FIFOs */
	regs->EXE = MEC_BIT(MEC_QSPI_EXE_CLRF_Pos);
	regs->STATUS = UINT32_MAX;

	if (config->operation == data->operation) {
		return 0;
	}

	data->operation = config->operation;
	ret = spi_feature_support(config);
	if (ret != 0) {
		return ret;
	}

	ret = mec_hal_qspi_io(regs, MEC_QSPI_IO_FULL_DUPLEX);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	if ((data->operation & SPI_MODE_CPHA) != 0) {
		sgm |= BIT(0);
	}
	if ((data->operation & SPI_MODE_CPOL) != 0) {
		sgm |= BIT(1);
	}
	/* requires QSPI frequency to be programmed first */
	ret = mec_hal_qspi_spi_signal_mode(regs, mec5_qspi_sig_mode[sgm]);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	data->ctx.config = config;

	return 0;
}

static int mec5_qspi_do_xfr(const struct device *dev, const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			    bool async, spi_callback_t cb, void *userdata)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec5_qspi_data *data = dev->data;
	struct mec_qspi_regs *regs = devcfg->regs;
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
	mec_hal_qspi_intr_ctrl_msk(regs, 1u, MEC_QSPI_IEN_TXB_EMPTY);

	ret = spi_context_wait_for_completion(ctx);

	if (async && (ret == 0)) {
		return 0;
	}

	if (ret != 0) {
		mec_hal_qspi_force_stop(devcfg->regs);
	}
do_xfr_exit:
	spi_context_release(ctx, 0);

	return ret;
}

static int mec5_qspi_xfr_check1(const struct spi_config *config)
{
	if (mec_hal_espi_taf_is_activated() == true) {
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
	const struct mec5_qspi_config *cfg = dev->config;
	int ret = 0;

	if (mec_hal_espi_taf_is_activated() == true) {
		return -EPERM;
	}

	ret = mec_hal_qspi_force_stop(cfg->regs);

	/* increments lock semphare in ctx up to initial limit */
	spi_context_unlock_unconditionally(&qdata->ctx);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

/* ISR helper */
static void mec5_qspi_ctx_next(const struct device *dev)
{
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec_qspi_regs *regs = devcfg->regs;
	struct mec5_qspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t xlen = 0;
	uint32_t qflags = MEC5_QSPI_ULDMA_FLAG_START | MEC5_QSPI_ULDMA_FLAG_IEN;

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
			qflags |= MEC5_QSPI_ULDMA_FLAG_INCR_TX;
		} else {
			txb = &devcfg->ovrc;
		}

		if (rxb != NULL) {
			qflags |= MEC5_QSPI_ULDMA_FLAG_INCR_RX;
		} else {
			rxb = (uint8_t *)&data->rxdb;
		}

		if ((data->total_tx_size <= xlen) && (data->total_rx_size <= xlen)) {
			qflags |= MEC5_QSPI_ULDMA_FLAG_CLOSE;
		}

		data->xfr_flags = MEC5_QSPI_XFR_FLAG_LDMA;
		mec_hal_qspi_uldma_fd2(regs, (const uint8_t *)txb, rxb, xlen, qflags);
	} else {
		spi_context_complete(&data->ctx, dev, 0);
	}
}

static void mec5_qspi_isr(const struct device *dev)
{
	struct mec5_qspi_data *data = dev->data;
	const struct mec5_qspi_config *devcfg = dev->config;
	struct mec_qspi_regs *regs = devcfg->regs;
	uint32_t hwsts = 0u;
	int status = 0;

	hwsts = mec_hal_qspi_hw_status(regs);
	data->qstatus = hwsts;
	status = mec_hal_qspi_done(regs);

	mec_hal_qspi_intr_ctrl(regs, 0);
	mec_hal_qspi_hw_status_clr(regs, hwsts);
	mec_hal_qspi_girq_clr(regs);

	if (status == MEC_RET_ERR_HW) {
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
	struct mec_qspi_regs *regs = devcfg->regs;
	struct mec5_qspi_data *data = dev->data;
	enum mec_qspi_cs cs = MEC_QSPI_CS_0;
	enum mec_qspi_io iom = MEC_QSPI_IO_FULL_DUPLEX;
	enum mec_qspi_signal_mode spi_mode = MEC_SPI_SIGNAL_MODE_0;
	int ret = 0;

	data->cs = 0;

	ret = mec_hal_qspi_init(regs, (uint32_t)devcfg->clock_freq, spi_mode, iom, cs);
	if (ret != MEC_RET_OK) {
		LOG_ERR("QSPI init error (%d)", ret);
		return -EINVAL;
	}

	data->freq = devcfg->clock_freq;
	data->operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	mec_hal_qspi_byte_time_ns(regs, &data->byte_time_ns);

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

#define MEC5_QSPI_CS_TIMING_VAL(a, b, c, d)                                                        \
	(((a) & 0xFu) | (((b) & 0xFu) << 8) | (((c) & 0xFu) << 16) | (((d) & 0xFu) << 24))

#define MEC5_QSPI_CS_TMV(node_id)                                                                  \
	MEC5_QSPI_CS_TIMING_VAL(DT_PROP_OR(node_id, dcsckon, 6), DT_PROP_OR(node_id, dckcsoff, 4), \
				DT_PROP_OR(node_id, dldh, 6), DT_PROP_OR(node_id, dcsda, 6))

#define MEC5_QSPI_IRQ_HANDLER_FUNC(id) .irq_config_func = mec5_qspi_irq_config_##id,

#define MEC5_QSPI_IRQ_HANDLER_CFG(id)                                                              \
	static void mec5_qspi_irq_config_##id(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mec5_qspi_isr,            \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

#define MEC5_QSPI_CHILD_FLAGS(node_id)                                                             \
	((DT_PROP_OR(node_id, spi_cpol, 0) & 0x1u) |                                               \
	 ((DT_PROP_OR(node_id, spi_cpha, 0) & 0x1u) << 1))

#define MEC5_QSPI_CHILD_INFO(node_id)                                                              \
	{                                                                                          \
		.cs_timing = MEC5_QSPI_CS_TMV(node_id),                                            \
		.cs = (uint8_t)(DT_REG_ADDR(node_id) & 0xffu),                                     \
		.sck_tap = (uint8_t)(DT_PROP_OR(node_id, clock_tap, 0)),                           \
		.ctrl_tap = (uint8_t)(DT_PROP_OR(node_id, ctrl_tap, 0)),                           \
		.flags = MEC5_QSPI_CHILD_FLAGS(node_id),                                           \
	},

#define MEC5_QSPI_CHILD_DEVS(i)                                                                    \
	static const struct mec5_spi_devices mec5_qspi_children_##i[] = {                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(i, MEC5_QSPI_CHILD_INFO)}

/* The instance number, i is not related to block ID's rather the
 * order the DT tools process all DT files in a build.
 */
#define MEC5_QSPI_DEVICE(i)                                                                        \
	PINCTRL_DT_INST_DEFINE(i);                                                                 \
	MEC5_QSPI_CHILD_DEVS(i);                                                                   \
	MEC5_QSPI_IRQ_HANDLER_CFG(i)                                                               \
                                                                                                   \
	static struct mec5_qspi_data mec5_qspi_data_##i = {                                        \
		SPI_CONTEXT_INIT_LOCK(mec5_qspi_data_##i, ctx),                                    \
		SPI_CONTEXT_INIT_SYNC(mec5_qspi_data_##i, ctx),                                    \
	};                                                                                         \
	static const struct mec5_qspi_config mec5_qspi_config_##i = {                              \
		.regs = (struct mec_qspi_regs *)DT_INST_REG_ADDR(i),                               \
		.clock_freq = DT_INST_PROP_OR(i, clock_frequency, MHZ(12)),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),                                         \
		.ovrc = DT_INST_PROP_OR(i, overrun_character, 0),                                  \
		MEC5_QSPI_IRQ_HANDLER_FUNC(i).child_devices = mec5_qspi_children_##i,              \
		.num_child_devices = ARRAY_SIZE(mec5_qspi_children_##i),                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(i, &mec5_qspi_init, NULL, &mec5_qspi_data_##i,                       \
			      &mec5_qspi_config_##i, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,        \
			      &mec5_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MEC5_QSPI_DEVICE)
