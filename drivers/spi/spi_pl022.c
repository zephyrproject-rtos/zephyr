/*
 * Copyright 2022 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_pl022

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include <zephyr/spinlock.h>
#include <soc.h>
#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif
#if defined(CONFIG_SPI_PL022_DMA)
#include <zephyr/drivers/dma.h>
#endif

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(spi_pl022);

#include "spi_context.h"

#define SSP_MASK(regname, name) GENMASK(SSP_##regname##_##name##_MSB, SSP_##regname##_##name##_LSB)

/* PL022 Register definitions */

/*
 * Macros to access SSP Registers with their offsets
 */
#define SSP_CR0(r)      (r + 0x000)
#define SSP_CR1(r)      (r + 0x004)
#define SSP_DR(r)       (r + 0x008)
#define SSP_SR(r)       (r + 0x00C)
#define SSP_CPSR(r)     (r + 0x010)
#define SSP_IMSC(r)     (r + 0x014)
#define SSP_RIS(r)      (r + 0x018)
#define SSP_MIS(r)      (r + 0x01C)
#define SSP_ICR(r)      (r + 0x020)
#define SSP_DMACR(r)    (r + 0x024)

/*
 * Control Register 0
 */
#define SSP_CR0_SCR_MSB 15
#define SSP_CR0_SCR_LSB 8
#define SSP_CR0_SPH_MSB 7
#define SSP_CR0_SPH_LSB 7
#define SSP_CR0_SPO_MSB 6
#define SSP_CR0_SPO_LSB 6
#define SSP_CR0_FRF_MSB 5
#define SSP_CR0_FRF_LSB 4
#define SSP_CR0_DSS_MSB 3
#define SSP_CR0_DSS_LSB 0

/* Data size select */
#define SSP_CR0_MASK_DSS SSP_MASK(CR0, DSS)
/* Frame format */
#define SSP_CR0_MASK_FRF SSP_MASK(CR0, FRF)
/* Polarity */
#define SSP_CR0_MASK_SPO SSP_MASK(CR0, SPO)
/* Phase */
#define SSP_CR0_MASK_SPH SSP_MASK(CR0, SPH)
/* Serial Clock Rate */
#define SSP_CR0_MASK_SCR SSP_MASK(CR0, SCR)

/*
 * Control Register 1
 */
#define SSP_CR1_SOD_MSB 3
#define SSP_CR1_SOD_LSB 3
#define SSP_CR1_MS_MSB 2
#define SSP_CR1_MS_LSB 2
#define SSP_CR1_SSE_MSB 1
#define SSP_CR1_SSE_LSB 1
#define SSP_CR1_LBM_MSB 0
#define SSP_CR1_LBM_LSB 0

/* Loopback Mode */
#define SSP_CR1_MASK_LBM SSP_MASK(CR1, LBM)
/* Port Enable */
#define SSP_CR1_MASK_SSE SSP_MASK(CR1, SSE)
/* Controller/Peripheral (Master/Slave) select */
#define SSP_CR1_MASK_MS SSP_MASK(CR1, MS)
/* Peripheral (Slave) mode output disabled */
#define SSP_CR1_MASK_SOD SSP_MASK(CR1, SOD)

/*
 * Status Register
 */
#define SSP_SR_BSY_MSB 4
#define SSP_SR_BSY_LSB 4
#define SSP_SR_RFF_MSB 3
#define SSP_SR_RFF_LSB 3
#define SSP_SR_RNE_MSB 2
#define SSP_SR_RNE_LSB 2
#define SSP_SR_TNF_MSB 1
#define SSP_SR_TNF_LSB 1
#define SSP_SR_TFE_MSB 0
#define SSP_SR_TFE_LSB 0

/* TX FIFO empty */
#define SSP_SR_MASK_TFE SSP_MASK(SR, TFE)
/* TX FIFO not full */
#define SSP_SR_MASK_TNF SSP_MASK(SR, TNF)
/* RX FIFO not empty */
#define SSP_SR_MASK_RNE SSP_MASK(SR, RNE)
/* RX FIFO full */
#define SSP_SR_MASK_RFF SSP_MASK(SR, RFF)
/* Busy Flag */
#define SSP_SR_MASK_BSY SSP_MASK(SR, BSY)

/*
 * Clock Prescale Register
 */
#define SSP_CPSR_CPSDVSR_MSB 7
#define SSP_CPSR_CPSDVSR_LSB 0
/* Clock prescale divider */
#define SSP_CPSR_MASK_CPSDVSR SSP_MASK(CPSR, CPSDVSR)

/*
 * Interrupt Mask Set/Clear Register
 */
#define SSP_IMSC_TXIM_MSB 3
#define SSP_IMSC_TXIM_LSB 3
#define SSP_IMSC_RXIM_MSB 2
#define SSP_IMSC_RXIM_LSB 2
#define SSP_IMSC_RTIM_MSB 1
#define SSP_IMSC_RTIM_LSB 1
#define SSP_IMSC_RORIM_MSB 0
#define SSP_IMSC_RORIM_LSB 0

/* Receive Overrun Interrupt mask */
#define SSP_IMSC_MASK_RORIM SSP_MASK(IMSC, RORIM)
/* Receive timeout Interrupt mask */
#define SSP_IMSC_MASK_RTIM SSP_MASK(IMSC, RTIM)
/* Receive FIFO Interrupt mask */
#define SSP_IMSC_MASK_RXIM SSP_MASK(IMSC, RXIM)
/* Transmit FIFO Interrupt mask */
#define SSP_IMSC_MASK_TXIM SSP_MASK(IMSC, TXIM)

/*
 * Raw Interrupt Status Register
 */
#define SSP_RIS_TXRIS_MSB 3
#define SSP_RIS_TXRIS_LSB 3
#define SSP_RIS_RXRIS_MSB 2
#define SSP_RIS_RXRIS_LSB 2
#define SSP_RIS_RTRIS_MSB 1
#define SSP_RIS_RTRIS_LSB 1
#define SSP_RIS_RORRIS_MSB 0
#define SSP_RIS_RORRIS_LSB 0

/* Receive Overrun Raw Interrupt status */
#define SSP_RIS_MASK_RORRIS SSP_MASK(RIS, RORRIS)
/* Receive Timeout Raw Interrupt status */
#define SSP_RIS_MASK_RTRIS SSP_MASK(RIS, RTRIS)
/* Receive FIFO Raw Interrupt status */
#define SSP_RIS_MASK_RXRIS SSP_MASK(RIS, RXRIS)
/* Transmit FIFO Raw Interrupt status */
#define SSP_RIS_MASK_TXRIS SSP_MASK(RIS, TXRIS)

/*
 * Masked Interrupt Status Register
 */
#define SSP_MIS_TXMIS_MSB 3
#define SSP_MIS_TXMIS_LSB 3
#define SSP_MIS_RXMIS_MSB 2
#define SSP_MIS_RXMIS_LSB 2
#define SSP_MIS_RTMIS_MSB 1
#define SSP_MIS_RTMIS_LSB 1
#define SSP_MIS_RORMIS_MSB 0
#define SSP_MIS_RORMIS_LSB 0

/* Receive Overrun Masked Interrupt status */
#define SSP_MIS_MASK_RORMIS SSP_MASK(MIS, RORMIS)
/* Receive Timeout Masked Interrupt status */
#define SSP_MIS_MASK_RTMIS SSP_MASK(MIS, RTMIS)
/* Receive FIFO Masked Interrupt status */
#define SSP_MIS_MASK_RXMIS SSP_MASK(MIS, RXMIS)
/* Transmit FIFO Masked Interrupt status */
#define SSP_MIS_MASK_TXMIS SSP_MASK(MIS, TXMIS)

/*
 * Interrupt Clear Register
 */
#define SSP_ICR_RTIC_MSB 1
#define SSP_ICR_RTIC_LSB 1
#define SSP_ICR_RORIC_MSB 0
#define SSP_ICR_RORIC_LSB 0

/* Receive Overrun Raw Clear Interrupt bit */
#define SSP_ICR_MASK_RORIC SSP_MASK(ICR, RORIC)
/* Receive Timeout Clear Interrupt bit */
#define SSP_ICR_MASK_RTIC SSP_MASK(ICR, RTIC)

/*
 * DMA Control Register
 */
#define SSP_DMACR_TXDMAE_MSB 1
#define SSP_DMACR_TXDMAE_LSB 1
#define SSP_DMACR_RXDMAE_MSB 0
#define SSP_DMACR_RXDMAE_LSB 0

/* Receive DMA Enable bit */
#define SSP_DMACR_MASK_RXDMAE SSP_MASK(DMACR, RXDMAE)
/* Transmit DMA Enable bit */
#define SSP_DMACR_MASK_TXDMAE SSP_MASK(DMACR, TXDMAE)

/* End register definitions */

/*
 * Clock Parameter ranges
 */
#define CPSDVR_MIN 0x02
#define CPSDVR_MAX 0xFE

#define SCR_MIN 0x00
#define SCR_MAX 0xFF

/* Fifo depth */
#define SSP_FIFO_DEPTH 8

/*
 * Register READ/WRITE macros
 */
#define SSP_READ_REG(reg) (*((volatile uint32_t *)reg))
#define SSP_WRITE_REG(reg, val) (*((volatile uint32_t *)reg) = val)
#define SSP_CLEAR_REG(reg, val) (*((volatile uint32_t *)reg) &= ~(val))

/*
 * Status check macros
 */
#define SSP_BUSY(reg) (SSP_READ_REG(SSP_SR(reg)) & SSP_SR_MASK_BSY)
#define SSP_RX_FIFO_NOT_EMPTY(reg) (SSP_READ_REG(SSP_SR(reg)) & SSP_SR_MASK_RNE)
#define SSP_TX_FIFO_EMPTY(reg) (SSP_READ_REG(SSP_SR(reg)) & SSP_SR_MASK_TFE)
#define SSP_TX_FIFO_NOT_FULL(reg) (SSP_READ_REG(SSP_SR(reg)) & SSP_SR_MASK_TNF)

#if defined(CONFIG_SPI_PL022_DMA)
enum spi_pl022_dma_direction {
	TX = 0,
	RX,
	NUM_OF_DIRECTION
};

struct spi_pl022_dma_config {
	const struct device *dev;
	uint32_t channel;
	uint32_t channel_config;
	uint32_t slot;
};

struct spi_pl022_dma_data {
	struct dma_config config;
	struct dma_block_config block;
	uint32_t count;
	bool callbacked;
};
#endif

/*
 * Max frequency
 */
#define MAX_FREQ_CONTROLLER_MODE(cfg) (cfg->pclk / 2)
#define MAX_FREQ_PERIPHERAL_MODE(cfg) (cfg->pclk / 12)

struct spi_pl022_cfg {
	const uint32_t reg;
	const uint32_t pclk;
	const bool dma_enabled;
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pincfg;
#endif
#if defined(CONFIG_SPI_PL022_INTERRUPT)
	void (*irq_config)(const struct device *port);
#endif
#if defined(CONFIG_SPI_PL022_DMA)
	const struct spi_pl022_dma_config dma[NUM_OF_DIRECTION];
#endif
};

struct spi_pl022_data {
	struct spi_context ctx;
	uint32_t tx_count;
	uint32_t rx_count;
	struct k_spinlock lock;
#if defined(CONFIG_SPI_PL022_DMA)
	struct spi_pl022_dma_data dma[NUM_OF_DIRECTION];
#endif
};

#if defined(CONFIG_SPI_PL022_DMA)
static uint32_t dummy_tx;
static uint32_t dummy_rx;
#endif

/* Helper Functions */

static inline uint32_t spi_pl022_calc_prescale(const uint32_t pclk, const uint32_t baud)
{
	uint32_t prescale;

	/* prescale only can take even number */
	for (prescale = CPSDVR_MIN; prescale < CPSDVR_MAX; prescale += 2) {
		if (pclk < (prescale + 2) * CPSDVR_MAX * baud) {
			break;
		}
	}

	return prescale;
}

static inline uint32_t spi_pl022_calc_postdiv(const uint32_t pclk,
					      const uint32_t baud, const uint32_t prescale)
{
	uint32_t postdiv;

	for (postdiv = SCR_MAX + 1; postdiv > SCR_MIN + 1; --postdiv) {
		if (pclk / (prescale * (postdiv - 1)) > baud) {
			break;
		}
	}
	return postdiv - 1;
}

static int spi_pl022_configure(const struct device *dev,
			       const struct spi_config *spicfg)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	const uint16_t op = spicfg->operation;
	uint32_t prescale;
	uint32_t postdiv;
	uint32_t cr0;
	uint32_t cr1;

	if (spi_context_configured(&data->ctx, spicfg)) {
		return 0;
	}

	if (spicfg->frequency > MAX_FREQ_CONTROLLER_MODE(cfg)) {
		LOG_ERR("Frequency is up to %u in controller mode.", MAX_FREQ_CONTROLLER_MODE(cfg));
		return -ENOTSUP;
	}

	if (op & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB-first not supported");
		return -ENOTSUP;
	}

	/* Half-duplex mode has not been implemented */
	if (op & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	/* Peripheral mode has not been implemented */
	if (SPI_OP_MODE_GET(op) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Peripheral mode is not supported");
		return -ENOTSUP;
	}

	/* Word sizes other than 8 bits has not been implemented */
	if (SPI_WORD_SIZE_GET(op) != 8) {
		LOG_ERR("Word sizes other than 8 bits are not supported");
		return -ENOTSUP;
	}

	/* configure registers */

	prescale = spi_pl022_calc_prescale(cfg->pclk, spicfg->frequency);
	postdiv = spi_pl022_calc_postdiv(cfg->pclk, spicfg->frequency, prescale);

	cr0 = 0;
	cr0 |= (postdiv << SSP_CR0_SCR_LSB);
	cr0 |= (SPI_WORD_SIZE_GET(op) - 1);
	cr0 |= (op & SPI_MODE_CPOL) ? SSP_CR0_MASK_SPO : 0;
	cr0 |= (op & SPI_MODE_CPHA) ? SSP_CR0_MASK_SPH : 0;

	cr1 = 0;
	cr1 |= SSP_CR1_MASK_SSE; /* Always enable SPI */
	cr1 |= (op & SPI_MODE_LOOP) ? SSP_CR1_MASK_LBM : 0;

	SSP_WRITE_REG(SSP_CPSR(cfg->reg), prescale);
	SSP_WRITE_REG(SSP_CR0(cfg->reg), cr0);
	SSP_WRITE_REG(SSP_CR1(cfg->reg), cr1);

#if defined(CONFIG_SPI_PL022_INTERRUPT)
	if (!cfg->dma_enabled) {
		SSP_WRITE_REG(SSP_IMSC(cfg->reg),
			      SSP_IMSC_MASK_RORIM | SSP_IMSC_MASK_RTIM | SSP_IMSC_MASK_RXIM);
	}
#endif

	data->ctx.config = spicfg;

	return 0;
}

static inline bool spi_pl022_transfer_ongoing(struct spi_pl022_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

#if defined(CONFIG_SPI_PL022_DMA)
static void spi_pl022_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				   int status);

static size_t spi_pl022_dma_enabled_num(const struct device *dev)
{
	const struct spi_pl022_cfg *cfg = dev->config;

	return cfg->dma_enabled ? 2 : 0;
}

static uint32_t spi_pl022_dma_setup(const struct device *dev, const uint32_t dir)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	struct dma_config *dma_cfg = &data->dma[dir].config;
	struct dma_block_config *block_cfg = &data->dma[dir].block;
	const struct spi_pl022_dma_config *dma = &cfg->dma[dir];
	int ret;

	memset(dma_cfg, 0, sizeof(struct dma_config));
	memset(block_cfg, 0, sizeof(struct dma_block_config));

	dma_cfg->source_burst_length = 1;
	dma_cfg->dest_burst_length = 1;
	dma_cfg->user_data = (void *)dev;
	dma_cfg->block_count = 1U;
	dma_cfg->head_block = block_cfg;
	dma_cfg->dma_slot = cfg->dma[dir].slot;
	dma_cfg->channel_direction = dir == TX ? MEMORY_TO_PERIPHERAL : PERIPHERAL_TO_MEMORY;

	if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
		dma_cfg->source_data_size = 1;
		dma_cfg->dest_data_size = 1;
	} else {
		dma_cfg->source_data_size = 2;
		dma_cfg->dest_data_size = 2;
	}

	block_cfg->block_size = spi_context_max_continuous_chunk(&data->ctx);

	if (dir == TX) {
		dma_cfg->dma_callback = spi_pl022_dma_callback;
		block_cfg->dest_address = SSP_DR(cfg->reg);
		block_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (spi_context_tx_buf_on(&data->ctx)) {
			block_cfg->source_address = (uint32_t)data->ctx.tx_buf;
			block_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			block_cfg->source_address = (uint32_t)&dummy_tx;
			block_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	if (dir == RX) {
		dma_cfg->dma_callback = spi_pl022_dma_callback;
		block_cfg->source_address = SSP_DR(cfg->reg);
		block_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

		if (spi_context_rx_buf_on(&data->ctx)) {
			block_cfg->dest_address = (uint32_t)data->ctx.rx_buf;
			block_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			block_cfg->dest_address = (uint32_t)&dummy_rx;
			block_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}

	ret = dma_config(dma->dev, dma->channel, dma_cfg);
	if (ret < 0) {
		LOG_ERR("dma_config %p failed %d\n", dma->dev, ret);
		return ret;
	}

	data->dma[dir].callbacked = false;

	ret = dma_start(dma->dev, dma->channel);
	if (ret < 0) {
		LOG_ERR("dma_start %p failed %d\n", dma->dev, ret);
		return ret;
	}

	return 0;
}

static int spi_pl022_start_dma_transceive(const struct device *dev)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	int ret = 0;

	SSP_CLEAR_REG(SSP_DMACR(cfg->reg), SSP_DMACR_MASK_RXDMAE | SSP_DMACR_MASK_TXDMAE);

	for (size_t i = 0; i < spi_pl022_dma_enabled_num(dev); i++) {
		ret = spi_pl022_dma_setup(dev, i);
		if (ret < 0) {
			goto on_error;
		}
	}

	SSP_WRITE_REG(SSP_DMACR(cfg->reg), SSP_DMACR_MASK_RXDMAE | SSP_DMACR_MASK_TXDMAE);

on_error:
	if (ret < 0) {
		for (size_t i = 0; i < spi_pl022_dma_enabled_num(dev); i++) {
			dma_stop(cfg->dma[i].dev, cfg->dma[i].channel);
		}
	}
	return ret;
}

static bool spi_pl022_chunk_transfer_finished(const struct device *dev)
{
	struct spi_pl022_data *data = dev->data;
	struct spi_pl022_dma_data *dma = data->dma;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);

	return (MIN(dma[TX].count, dma[RX].count) >= chunk_len);
}

static void spi_pl022_complete(const struct device *dev, int status)
{
	struct spi_pl022_data *data = dev->data;
	const struct spi_pl022_cfg *cfg = dev->config;

	for (size_t i = 0; i < spi_pl022_dma_enabled_num(dev); i++) {
		dma_stop(cfg->dma[i].dev, cfg->dma[i].channel);
	}

	spi_context_complete(&data->ctx, dev, status);
}

static void spi_pl022_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				   int status)
{
	const struct device *dev = (const struct device *)arg;
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	bool complete = false;
	k_spinlock_key_t key;
	size_t chunk_len;
	int err = 0;

	if (status < 0) {
		key = k_spin_lock(&data->lock);

		LOG_ERR("dma:%p ch:%d callback gets error: %d", dma_dev, channel, status);
		spi_pl022_complete(dev, status);

		k_spin_unlock(&data->lock, key);
		return;
	}

	key = k_spin_lock(&data->lock);

	chunk_len = spi_context_max_continuous_chunk(&data->ctx);
	for (size_t i = 0; i < ARRAY_SIZE(cfg->dma); i++) {
		if (dma_dev == cfg->dma[i].dev && channel == cfg->dma[i].channel) {
			data->dma[i].count += chunk_len;
			data->dma[i].callbacked = true;
		}
	}
	/* Check transfer finished.
	 * The transmission of this chunk is complete if both the dma[TX].count
	 * and the dma[RX].count reach greater than or equal to the chunk_len.
	 * chunk_len is zero here means the transfer is already complete.
	 */
	if (spi_pl022_chunk_transfer_finished(dev)) {
		if (SPI_WORD_SIZE_GET(data->ctx.config->operation) == 8) {
			spi_context_update_tx(&data->ctx, 1, chunk_len);
			spi_context_update_rx(&data->ctx, 1, chunk_len);
		} else {
			spi_context_update_tx(&data->ctx, 2, chunk_len);
			spi_context_update_rx(&data->ctx, 2, chunk_len);
		}

		if (spi_pl022_transfer_ongoing(data)) {
			/* Next chunk is available, reset the count and
			 * continue processing
			 */
			data->dma[TX].count = 0;
			data->dma[RX].count = 0;
		} else {
			/* All data is processed, complete the process */
			complete = true;
		}
	}

	if (!complete && data->dma[TX].callbacked && data->dma[RX].callbacked) {
		err = spi_pl022_start_dma_transceive(dev);
		if (err) {
			complete = true;
		}
	}

	if (complete) {
		spi_pl022_complete(dev, err);
	}

	k_spin_unlock(&data->lock, key);
}

#endif /* DMA */

#if defined(CONFIG_SPI_PL022_INTERRUPT)

static void spi_pl022_async_xfer(const struct device *dev)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	/* Process by per chunk */
	size_t chunk_len = spi_context_max_continuous_chunk(ctx);
	uint32_t txrx;

	/* Read RX FIFO */
	while (SSP_RX_FIFO_NOT_EMPTY(cfg->reg) && (data->rx_count < chunk_len)) {
		txrx = SSP_READ_REG(SSP_DR(cfg->reg));

		/* Discard received data if rx buffer not assigned */
		if (ctx->rx_buf) {
			*(((uint8_t *)ctx->rx_buf) + data->rx_count) = (uint8_t)txrx;
		}
		data->rx_count++;
	}

	/* Check transfer finished.
	 * The transmission of this chunk is complete if both the tx_count
	 * and the rx_count reach greater than or equal to the chunk_len.
	 * chunk_len is zero here means the transfer is already complete.
	 */
	if (MIN(data->tx_count, data->rx_count) >= chunk_len && chunk_len > 0) {
		spi_context_update_tx(ctx, 1, chunk_len);
		spi_context_update_rx(ctx, 1, chunk_len);
		if (spi_pl022_transfer_ongoing(data)) {
			/* Next chunk is available, reset the count and continue processing */
			data->tx_count = 0;
			data->rx_count = 0;
			chunk_len = spi_context_max_continuous_chunk(ctx);
		} else {
			/* All data is processed, complete the process */
			spi_context_complete(ctx, dev, 0);
			return;
		}
	}

	/* Fill up TX FIFO */
	for (uint32_t i = 0; i < SSP_FIFO_DEPTH; i++) {
		if ((data->tx_count < chunk_len) && SSP_TX_FIFO_NOT_FULL(cfg->reg)) {
			/* Send 0 in the case of read only operation */
			txrx = 0;

			if (ctx->tx_buf) {
				txrx = *(((uint8_t *)ctx->tx_buf) + data->tx_count);
			}
			SSP_WRITE_REG(SSP_DR(cfg->reg), txrx);
			data->tx_count++;
		} else {
			break;
		}
	}
}

static void spi_pl022_start_async_xfer(const struct device *dev)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;

	/* Ensure writable */
	while (!SSP_TX_FIFO_EMPTY(cfg->reg))
		;
	/* Drain RX FIFO */
	while (SSP_RX_FIFO_NOT_EMPTY(cfg->reg))
		SSP_READ_REG(SSP_DR(cfg->reg));

	data->tx_count = 0;
	data->rx_count = 0;

	SSP_WRITE_REG(SSP_ICR(cfg->reg), SSP_ICR_MASK_RORIC | SSP_ICR_MASK_RTIC);

	spi_pl022_async_xfer(dev);
}

static void spi_pl022_isr(const struct device *dev)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t mis = SSP_READ_REG(SSP_MIS(cfg->reg));

	if (mis & SSP_MIS_MASK_RORMIS) {
		SSP_WRITE_REG(SSP_IMSC(cfg->reg), 0);
		spi_context_complete(ctx, dev, -EIO);
	} else {
		spi_pl022_async_xfer(dev);
	}

	SSP_WRITE_REG(SSP_ICR(cfg->reg), SSP_ICR_MASK_RORIC | SSP_ICR_MASK_RTIC);
}

#else

static void spi_pl022_xfer(const struct device *dev)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	const size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);
	const void *txbuf = data->ctx.tx_buf;
	void *rxbuf = data->ctx.rx_buf;
	uint32_t txrx;
	size_t fifo_cnt = 0;

	data->tx_count = 0;
	data->rx_count = 0;

	/* Ensure writable */
	while (!SSP_TX_FIFO_EMPTY(cfg->reg))
		;
	/* Drain RX FIFO */
	while (SSP_RX_FIFO_NOT_EMPTY(cfg->reg))
		SSP_READ_REG(SSP_DR(cfg->reg));

	while (data->rx_count < chunk_len || data->tx_count < chunk_len) {
		/* Fill up fifo with available TX data */
		while (SSP_TX_FIFO_NOT_FULL(cfg->reg) && data->tx_count < chunk_len &&
		       fifo_cnt < SSP_FIFO_DEPTH) {
			/* Send 0 in the case of read only operation */
			txrx = 0;

			if (txbuf) {
				txrx = ((uint8_t *)txbuf)[data->tx_count];
			}
			SSP_WRITE_REG(SSP_DR(cfg->reg), txrx);
			data->tx_count++;
			fifo_cnt++;
		}
		while (data->rx_count < chunk_len && fifo_cnt > 0) {
			if (!SSP_RX_FIFO_NOT_EMPTY(cfg->reg))
				continue;

			txrx = SSP_READ_REG(SSP_DR(cfg->reg));

			/* Discard received data if rx buffer not assigned */
			if (rxbuf) {
				((uint8_t *)rxbuf)[data->rx_count] = (uint8_t)txrx;
			}
			data->rx_count++;
			fifo_cnt--;
		}
	}
}

#endif

static int spi_pl022_transceive_impl(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     spi_callback_t cb,
				     void *userdata)
{
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(&data->ctx, (cb ? true : false), cb, userdata, config);

	ret = spi_pl022_configure(dev, config);
	if (ret < 0) {
		goto error;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(ctx, true);

	if (cfg->dma_enabled) {
#if defined(CONFIG_SPI_PL022_DMA)
		for (size_t i = 0; i < ARRAY_SIZE(data->dma); i++) {
			const struct spi_pl022_cfg *cfg = dev->config;
			struct dma_status stat = {.busy = true};

			dma_stop(cfg->dma[i].dev, cfg->dma[i].channel);

			while (stat.busy) {
				dma_get_status(cfg->dma[i].dev, cfg->dma[i].channel, &stat);
			}

			data->dma[i].count = 0;
		}

		ret = spi_pl022_start_dma_transceive(dev);
		if (ret < 0) {
			spi_context_cs_control(ctx, false);
			goto error;
		}
		ret = spi_context_wait_for_completion(ctx);
#endif
	} else
#if defined(CONFIG_SPI_PL022_INTERRUPT)
	{
		spi_pl022_start_async_xfer(dev);
		ret = spi_context_wait_for_completion(ctx);
	}
#else
	{
		do {
			spi_pl022_xfer(dev);
			spi_context_update_tx(ctx, 1, data->tx_count);
			spi_context_update_rx(ctx, 1, data->rx_count);
		} while (spi_pl022_transfer_ongoing(data));

#if defined(CONFIG_SPI_ASYNC)
		spi_context_complete(&data->ctx, dev, ret);
#endif
	}
#endif

	spi_context_cs_control(ctx, false);

error:
	spi_context_release(&data->ctx, ret);

	return ret;
}

/* API Functions */

static int spi_pl022_transceive(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	return spi_pl022_transceive_impl(dev, config, tx_bufs, rx_bufs, NULL, NULL);
}

#if defined(CONFIG_SPI_ASYNC)

static int spi_pl022_transceive_async(const struct device *dev,
				      const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      spi_callback_t cb,
				      void *userdata)
{
	return spi_pl022_transceive_impl(dev, config, tx_bufs, rx_bufs, cb, userdata);
}

#endif

static int spi_pl022_release(const struct device *dev,
			     const struct spi_config *config)
{
	struct spi_pl022_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct spi_driver_api spi_pl022_api = {
	.transceive = spi_pl022_transceive,
#if defined(CONFIG_SPI_ASYNC)
	.transceive_async = spi_pl022_transceive_async,
#endif
	.release = spi_pl022_release
};

static int spi_pl022_init(const struct device *dev)
{
	/* Initialize with lowest frequency */
	const struct spi_config spicfg = {
		.frequency = 0,
		.operation = SPI_WORD_SET(8),
		.slave = 0,
	};
	const struct spi_pl022_cfg *cfg = dev->config;
	struct spi_pl022_data *data = dev->data;
	int ret;

#if defined(CONFIG_PINCTRL)
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}
#endif

	if (cfg->dma_enabled) {
#if defined(CONFIG_SPI_PL022_DMA)
		for (size_t i = 0; i < spi_pl022_dma_enabled_num(dev); i++) {
			uint32_t ch_filter = BIT(cfg->dma[i].channel);

			if (!device_is_ready(cfg->dma[i].dev)) {
				LOG_ERR("DMA %s not ready", cfg->dma[i].dev->name);
				return -ENODEV;
			}

			ret = dma_request_channel(cfg->dma[i].dev, &ch_filter);
			if (ret < 0) {
				LOG_ERR("dma_request_channel failed %d", ret);
				return ret;
			}
		}
#endif
	} else {
#if defined(CONFIG_SPI_PL022_INTERRUPT)
		cfg->irq_config(dev);
#endif
	}

	ret = spi_pl022_configure(dev, &spicfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure spi");
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		LOG_ERR("Failed to spi_context configure");
		return ret;
	}

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define DMA_INITIALIZER(idx, dir)                                                                  \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(idx, dir)),                         \
		.channel = DT_INST_DMAS_CELL_BY_NAME(idx, dir, channel),                           \
		.slot = DT_INST_DMAS_CELL_BY_NAME(idx, dir, slot),                                 \
		.channel_config = DT_INST_DMAS_CELL_BY_NAME(idx, dir, channel_config),             \
	}

#define DMAS_DECL(idx)                                                                             \
	{                                                                                          \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, tx), (DMA_INITIALIZER(idx, tx)), ({0})),    \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, rx), (DMA_INITIALIZER(idx, rx)), ({0})),    \
	}

#define DMAS_ENABLED(idx) (DT_INST_DMAS_HAS_NAME(idx, tx) && DT_INST_DMAS_HAS_NAME(idx, rx))

#define SPI_PL022_INIT(idx)                                                                        \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(idx);))                                 \
	IF_ENABLED(CONFIG_SPI_PL022_INTERRUPT,                                                     \
		   (static void spi_pl022_irq_config_##idx(const struct device *dev)               \
		    {                                                                              \
			   IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority),              \
				       spi_pl022_isr, DEVICE_DT_INST_GET(idx), 0);                 \
			   irq_enable(DT_INST_IRQN(idx));                                          \
		    }))                                                                            \
	static struct spi_pl022_data spi_pl022_data_##idx = {                                      \
		SPI_CONTEXT_BASE_INIT(spi_pl022_data_##idx, ctx),                                  \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(idx), ctx)};                           \
	static struct spi_pl022_cfg spi_pl022_cfg_##idx = {                                        \
		.reg = DT_INST_REG_ADDR(idx),                                                      \
		.pclk = DT_INST_PROP_BY_PHANDLE(idx, clocks, clock_frequency),                     \
		IF_ENABLED(CONFIG_PINCTRL, (.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),))       \
		IF_ENABLED(CONFIG_SPI_PL022_DMA, (.dma = DMAS_DECL(idx),)) COND_CODE_1(            \
				CONFIG_SPI_PL022_DMA, (.dma_enabled = DMAS_ENABLED(idx),),         \
				(.dma_enabled = false,))                                           \
		IF_ENABLED(CONFIG_SPI_PL022_INTERRUPT,                                             \
					   (.irq_config = spi_pl022_irq_config_##idx,))};          \
	DEVICE_DT_INST_DEFINE(idx, spi_pl022_init, NULL, &spi_pl022_data_##idx,                    \
			      &spi_pl022_cfg_##idx, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,         \
			      &spi_pl022_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_PL022_INIT)
