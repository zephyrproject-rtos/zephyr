/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_omap_mcspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(omap_mcspi);

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include "spi_context.h"
#include <zephyr/sys/util_macro.h>

#include <zephyr/drivers/dma.h>

/* Max clock divisor for granularity of 1 (12-bit) */
#define OMAP_MCSPI_CLK_1_MAX_DIV   (4096)
/* Max clock divisor for granularity of 2^n (15-bit) */
#define OMAP_MCSPI_CLK_2_N_MAX_DIV (32768)
#define OMAP_MCSPI_NUM_CHANNELS    (4)

/* Number of retries when reading some register status */
#define OMAP_MCSPI_REG_RETRIES                 (100)
/* Time to wait between successive retries in microseconds */
#define OMAP_MCSPI_REG_TIME_BETWEEN_RETRIES_US (10)

struct omap_mcspi_regs {
	uint8_t RESERVED_1[0x04];    /**< Reserved, offset: 0x00 - 0x04 */
	volatile uint32_t HWINFO;    /**< MCSPI Hardware configuration register, offset: 0x04 */
	uint8_t RESERVED_2[0x108];   /**< Reserved, offset: 0x08 - 0x110 */
	volatile uint32_t SYSCONFIG; /**< Configuration register, offset: 0x110 */
	volatile uint32_t SYSSTATUS; /**< Status information register, offset: 0x114 */
	uint8_t RESERVED_3[0x10];    /**< Reserved, offset: 0x118 - 0x128 */
	volatile uint32_t MODULCTRL; /**< MCSPI configuration register, offset: 0x128 */
	volatile struct {
		volatile uint32_t CHCONF; /**< Configuration register, offset: 0x12C + (0x14 * i) */
		volatile uint32_t CHSTAT; /**< Status register, offset: 0x130 + (0x14 * i) */
		volatile uint32_t CHCTRL; /**< Control register, offset: 0x134 + (0x14 * i) */
		volatile uint32_t TX;     /**< TX register, offset: 0x138 + (0x14 * i) */
		volatile uint32_t RX;     /**< RX register, offset: 0x13C + (0x14 * i) */
	} CHAN[OMAP_MCSPI_NUM_CHANNELS];
	volatile uint32_t XFERLEVEL; /**< FIFO Transfer Level register, offset: 0x17C */
};

/* Hardware Information */
#define OMAP_MCSPI_HWINFO_FFNBYTE GENMASK(5, 1) /* FIFO depth */

/* Configuration Register */
#define OMAP_MCSPI_SYSCONFIG_SOFTRESET BIT(1) /* Software Reset */

/* Status Register */
#define OMAP_MCSPI_SYSSTATUS_RESETDONE BIT(0) /* Internal Reset Monitoring */

/* MCSPI Configuration Register */
#define OMAP_MCSPI_MODULCTRL_SYSTEST BIT(3) /* System test mode */
#define OMAP_MCSPI_MODULCTRL_MS      BIT(2) /* Peripheral mode */
#define OMAP_MCSPI_MODULCTRL_SINGLE  BIT(0) /* Single channel mode (controller only) */

/* Channel Configuration */
#define OMAP_MCSPI_CHCONF_CLKG        BIT(29)         /* Clock divider granularity */
#define OMAP_MCSPI_CHCONF_FFER        BIT(28)         /* Enable FIFO for receiving */
#define OMAP_MCSPI_CHCONF_FFEW        BIT(27)         /* Enable FIFO for tramitting */
#define OMAP_MCSPI_CHCONF_FORCE       BIT(20)         /* Manual SPIEN assertion */
#define OMAP_MCSPI_CHCONF_TURBO       BIT(19)         /* Turbo mode */
#define OMAP_MCSPI_CHCONF_IS          BIT(18)         /* Input select */
#define OMAP_MCSPI_CHCONF_DPE1        BIT(17)         /* Transmission enabled for data line 1 */
#define OMAP_MCSPI_CHCONF_DPE0        BIT(16)         /* Transmission enabled for data line 0 */
#define OMAP_MCSPI_CHCONF_DMAR        BIT(15)         /* Enable DMA Read Request */
#define OMAP_MCSPI_CHCONF_DMAW        BIT(14)         /* Enable DMA Write Request */
#define OMAP_MCSPI_CHCONF_TRM         GENMASK(13, 12) /* TX/RX mode */
#define OMAP_MCSPI_CHCONF_TRM_TX_ONLY BIT(13)         /* TX/RX mode - Transmit only */
#define OMAP_MCSPI_CHCONF_TRM_RX_ONLY BIT(12)         /* TX/RX mode - Receive only */
#define OMAP_MCSPI_CHCONF_WL          GENMASK(11, 7)  /* SPI word length */
#define OMAP_MCSPI_CHCONF_EPOL        BIT(6)          /* SPIEN polarity */
#define OMAP_MCSPI_CHCONF_CLKD        GENMASK(5, 2)   /* Frequency divider for SPICLK */
#define OMAP_MCSPI_CHCONF_POL         BIT(1)          /* SPICLK polarity */
#define OMAP_MCSPI_CHCONF_PHA         BIT(0)          /* SPICLK phase */

/* Channel Control Register */
#define OMAP_MCSPI_CHCTRL_EXTCLK GENMASK(15, 8) /* Clock ratio extension (concat with CLKD) */
#define OMAP_MCSPI_CHCTRL_EN     BIT(0)         /* Channel enable */

/* Channel Status Register */
#define OMAP_MCSPI_CHSTAT_TXFFE BIT(3) /* Transmit buffer empty registe */
#define OMAP_MCSPI_CHSTAT_RXFFE BIT(5) /* Receive buffer empty registe */
#define OMAP_MCSPI_CHSTAT_EOT   BIT(2) /* End of transfer status */
#define OMAP_MCSPI_CHSTAT_TXS   BIT(1) /* Transmit register empty status */
#define OMAP_MCSPI_CHSTAT_RXS   BIT(0) /* Receiver register full status */

/*  FIFO transfer level register */
#define OMAP_MCSPI_XFERLEVEL_WCNT GENMASK(31, 16) /* Word counter for transfer */
#define OMAP_MCSPI_XFERLEVEL_AFL  GENMASK(15, 8)  /* Buffer Almost Full Trig level */
#define OMAP_MCSPI_XFERLEVEL_AEL  GENMASK(7, 0)   /* Buffer Almost Empty Trig level */

#define DEV_CFG(dev)  ((const struct omap_mcspi_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct omap_mcspi_data *)(dev)->data)
#define DEV_REGS(dev) ((struct omap_mcspi_regs *)DEVICE_MMIO_GET(dev))

#ifdef CONFIG_SPI_OMAP_DMA
struct omap_mcspi_work_struct {
	struct k_work work;
	const struct device *spi_dev;
};

struct omap_dma_data {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
};

struct omap_mcspi_channel_data {
	uint32_t ch_num; /* McSPI Channel Number */
	struct omap_dma_data tx_dma_data;
	struct omap_dma_data rx_dma_data;
	uint8_t dma_completion_flags; /* Stores the necessary completion status of DMA */
	uint8_t dma_status_flags;     /* Stores the status of DMA transfer */
	struct k_sem dma_completion_sem;
};
#endif /* CONFIG_SPI_OMAP_DMA */

#define DMA_CHANNEL_RX_DONE_FLAG BIT(0)
#define DMA_CHANNEL_TX_DONE_FLAG BIT(1)
#define DMA_CHANNEL_ERROR_FLAG   BIT(2)

struct omap_mcspi_cfg {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pinctrl;
	uint32_t clock_frequency;
	bool d1_miso_d0_mosi;
	uint8_t num_cs;
};

struct omap_mcspi_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	uint32_t fifo_depth;
	uint32_t chconf;
	uint32_t chctrl;
	uint8_t dfs; /* data frame size - word length in bytes */
#ifdef CONFIG_SPI_OMAP_DMA
	struct omap_mcspi_channel_data chan_data[OMAP_MCSPI_NUM_CHANNELS];
	struct omap_mcspi_work_struct work_struct;
#endif /* CONFIG_SPI_OMAP_DMA */
};

#ifdef CONFIG_SPI_OMAP_DMA
static void spi_dma_callback(const struct device *dma_dev, void *arg, uint32_t dma_channel,
			     int status)
{
	struct omap_mcspi_channel_data *chan_data = (struct omap_mcspi_channel_data *)arg;

	if (dma_channel == chan_data->tx_dma_data.dma_channel) {
		if (status == DMA_STATUS_COMPLETE) {
			chan_data->dma_status_flags |= DMA_CHANNEL_TX_DONE_FLAG;
		}
	} else if (dma_channel == chan_data->rx_dma_data.dma_channel) {
		if (status == DMA_STATUS_COMPLETE) {
			chan_data->dma_status_flags |= DMA_CHANNEL_RX_DONE_FLAG;
		}
	} else {
		LOG_ERR("Unexpected error in dma callback function for DMA channel %d.",
			dma_channel);
		chan_data->dma_status_flags |= DMA_CHANNEL_ERROR_FLAG;
		k_sem_give(&(chan_data->dma_completion_sem));
	}

	if (chan_data->dma_status_flags == chan_data->dma_completion_flags) {
		k_sem_give(&(chan_data->dma_completion_sem));
	}
}
#endif /* CONFIG_SPI_OMAP_DMA */

static void omap_mcspi_channel_enable(const struct device *dev, bool enable)
{
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	uint8_t chan = DEV_DATA(dev)->ctx.config->slave;

	if (enable) {
		regs->CHAN[chan].CHCTRL |= OMAP_MCSPI_CHCTRL_EN;
	} else {
		regs->CHAN[chan].CHCTRL &= ~OMAP_MCSPI_CHCTRL_EN;
	}
}

static void omap_mcspi_set_mode(const struct device *dev, bool is_peripheral)
{
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	uint32_t modulctrl = regs->MODULCTRL;

	/* disable system test mode */
	modulctrl &= ~(OMAP_MCSPI_MODULCTRL_SYSTEST);

	/* set controller or peripheral (master/slave) */
	if (is_peripheral) {
		modulctrl |= OMAP_MCSPI_MODULCTRL_MS;
	} else {
		modulctrl &= ~OMAP_MCSPI_MODULCTRL_MS;

		/* We only support single-mode for now
		 * TODO: add multi-mode
		 */
		modulctrl |= OMAP_MCSPI_MODULCTRL_SINGLE;
	}

	regs->MODULCTRL = modulctrl;
}

static int omap_mcspi_configure_clk_freq(const struct device *dev, uint32_t speed_hz,
					 uint32_t ref_hz)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	uint32_t extclk = 0;
	uint32_t clkd = 0;
	uint32_t clkg = 0;
	uint32_t f_ratio = DIV_ROUND_UP(ref_hz, speed_hz);

	if (f_ratio <= OMAP_MCSPI_CLK_1_MAX_DIV) {
		/* If under 4096, use the granularity of  1 */
		clkg = 1;
		extclk = (f_ratio - 1) >> 4;
		clkd = (f_ratio - 1) & 0xf;
		/* Otherwise if power of two, use granularity of 2^n (n <= 15) */
	} else if ((f_ratio & (f_ratio - 1)) == 0 && f_ratio <= OMAP_MCSPI_CLK_2_N_MAX_DIV) {
		clkg = 0;
		while (f_ratio != 1) {
			f_ratio >>= 1;
			clkd++;
		}
	} else {
		LOG_ERR("Invalid SPI device frequency: %uHz\n", speed_hz);
		return -EINVAL;
	}

	data->chconf &= ~OMAP_MCSPI_CHCONF_CLKD;
	data->chconf |= FIELD_PREP(OMAP_MCSPI_CHCONF_CLKD, clkd);

	if (clkg) {
		data->chconf |= OMAP_MCSPI_CHCONF_CLKG;
		data->chctrl &= ~OMAP_MCSPI_CHCTRL_EXTCLK;
		data->chctrl |= FIELD_PREP(OMAP_MCSPI_CHCTRL_EXTCLK, extclk);
	} else {
		data->chconf &= ~OMAP_MCSPI_CHCONF_CLKG;
	}

	return 0;
}

static int omap_mcspi_configure(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	const struct omap_mcspi_cfg *cfg = DEV_CFG(dev);
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	uint8_t chan = config->slave;
	uint8_t word_size = SPI_WORD_SIZE_GET(config->operation);
	bool is_peripheral = config->operation & SPI_OP_MODE_SLAVE;
	int rv;

	if (spi_context_configured(ctx, config)) {
		/* This configuration is already in use */
		return 0;
	}

	if (config->operation & SPI_HOLD_ON_CS) {
		return -ENOTSUP;
	}

	if (is_peripheral && !IS_ENABLED(CONFIG_SPI_SLAVE)) {
		LOG_ERR("Kconfig for SPI slave mode is not enabled");
		return -ENOTSUP;
	}

	if (chan >= cfg->num_cs) {
		LOG_ERR("invalid slave selected");
		return -EINVAL;
	}

	if ((config->operation & SPI_HALF_DUPLEX) && tx_bufs && rx_bufs) {
		LOG_ERR("cannot transmit and receive simultaneously with half duplex");
		return -EINVAL;
	}

	if (word_size < 4 || word_size > 32) {
		LOG_ERR("invalid word size");
		return -EINVAL;
	}

	/* update data frame size (word size in bytes) */
	if (word_size <= 8) {
		data->dfs = 1;
	} else if (word_size <= 16) {
		data->dfs = 2;
	} else { /* word_size <= 32 */
		data->dfs = 4;
	}

	ARRAY_FOR_EACH(regs->CHAN, ch) {
		if (ch != chan) {
			/* only when MODULCTRL_SINGLE is set */
			regs->CHAN[ch].CHCTRL &= ~OMAP_MCSPI_CHCTRL_EN;
			regs->CHAN[ch].CHCONF &= ~OMAP_MCSPI_CHCONF_FORCE;
		}

		/* disable FIFO for all channels */
		regs->CHAN[ch].CHCONF &= ~(OMAP_MCSPI_CHCONF_FFER | OMAP_MCSPI_CHCONF_FFEW);
	}

	/* set mode */
	omap_mcspi_set_mode(dev, is_peripheral);

	/* update cached registers */
	data->chconf = regs->CHAN[chan].CHCONF;
	data->chctrl = regs->CHAN[chan].CHCTRL;

	/* configure word length */
	data->chconf &= ~OMAP_MCSPI_CHCONF_WL;
	data->chconf |= FIELD_PREP(OMAP_MCSPI_CHCONF_WL, word_size - 1);

	if (config->operation & SPI_MODE_LOOP) {
		/* d0-in d0-out, loopback */
		data->chconf &= ~(OMAP_MCSPI_CHCONF_IS);
		data->chconf |= (OMAP_MCSPI_CHCONF_DPE1);
		data->chconf &= ~(OMAP_MCSPI_CHCONF_DPE0);
	} else if (cfg->d1_miso_d0_mosi) {
		/* d1-in-d0-out */
		data->chconf |= (OMAP_MCSPI_CHCONF_IS);
		data->chconf |= (OMAP_MCSPI_CHCONF_DPE1);
		data->chconf &= ~(OMAP_MCSPI_CHCONF_DPE0);
	} else {
		/* d0-in d1-out, default */
		data->chconf &= ~(OMAP_MCSPI_CHCONF_IS);
		data->chconf &= ~(OMAP_MCSPI_CHCONF_DPE1);
		data->chconf |= (OMAP_MCSPI_CHCONF_DPE0);
	}

	/* configure spien polarity */
	if (!(config->operation & SPI_CS_ACTIVE_HIGH)) {
		data->chconf |= OMAP_MCSPI_CHCONF_EPOL;
	} else {
		data->chconf &= ~OMAP_MCSPI_CHCONF_EPOL;
	}

	/* set clk polarity */
	if (config->operation & SPI_MODE_CPOL) {
		data->chconf |= OMAP_MCSPI_CHCONF_POL;
	} else {
		data->chconf &= ~OMAP_MCSPI_CHCONF_POL;
	}

	/* set clk phase */
	if (config->operation & SPI_MODE_CPHA) {
		data->chconf |= OMAP_MCSPI_CHCONF_PHA;
	} else {
		data->chconf &= ~OMAP_MCSPI_CHCONF_PHA;
	}

	/* set force */
	if (!spi_cs_is_gpio(config)) {
		data->chconf |= OMAP_MCSPI_CHCONF_FORCE;
	} else {
		data->chconf &= ~OMAP_MCSPI_CHCONF_FORCE;
	}

	rv = omap_mcspi_configure_clk_freq(dev, config->frequency, cfg->clock_frequency);
	if (rv != 0) {
		return rv;
	}

	/* save config in the context */
	ctx->config = config;
	return 0;
}

static int omap_mcspi_wait_for_reg_bit(volatile uint32_t *reg, uint8_t bit)
{
	uint32_t retries = 0;

	while ((*reg & bit) == 0) {
		/* timeout = 1ms */
		if (retries++ > OMAP_MCSPI_REG_RETRIES) {
			return -ETIMEDOUT;
		}
		k_usleep(OMAP_MCSPI_REG_TIME_BETWEEN_RETRIES_US);
	}

	return 0;
}

static ALWAYS_INLINE void write_tx(const uint8_t *tx_buf, volatile uint32_t *tx_reg, uint8_t dfs)
{
	switch (dfs) {
	case 1: {
		*tx_reg = UNALIGNED_GET((uint8_t *)tx_buf);
		break;
	}
	case 2: {
		*tx_reg = UNALIGNED_GET((uint16_t *)tx_buf);
		break;
	}
	case 4: {
		*tx_reg = UNALIGNED_GET((uint32_t *)tx_buf);
		break;
	}
	default: {
		/* unreachable */
	}
	}
}

static ALWAYS_INLINE void read_rx(uint8_t *rx_buf, volatile uint32_t *rx_reg, uint8_t dfs,
				  uint32_t word_mask)
{

	switch (dfs) {
	case 1: {
		UNALIGNED_PUT(*rx_reg & word_mask, (uint8_t *)rx_buf);
		break;
	}
	case 2: {
		UNALIGNED_PUT(*rx_reg & word_mask, (uint16_t *)rx_buf);
		break;
	}
	case 4: {
		UNALIGNED_PUT(*rx_reg & word_mask, (uint32_t *)rx_buf);
		break;
	}
	default: {
		/* unreachable */
	}
	}
}

static int omap_mcspi_transceive_pio(const struct device *dev, size_t count)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	const uint32_t word_mask = (1ULL << SPI_WORD_SIZE_GET(ctx->config->operation)) - 1;
	const uint8_t chan = ctx->config->slave;
	const uint8_t dfs = data->dfs;
	const uint8_t *tx_buf = ctx->tx_buf;
	uint8_t *rx_buf = ctx->rx_buf;

	volatile uint32_t *chstat = &regs->CHAN[chan].CHSTAT;
	volatile uint32_t *tx_reg = &regs->CHAN[chan].TX;
	volatile uint32_t *rx_reg = &regs->CHAN[chan].RX;

	/* RX only */
	if (!tx_buf) {
		/* write dummy value of 0 to TX FIFO */
		*tx_reg = 0;

		while (count != 0) {
			if (omap_mcspi_wait_for_reg_bit(chstat, OMAP_MCSPI_CHSTAT_RXS)) {
				LOG_ERR("RXS timed out");
				return count;
			}

			read_rx(rx_buf, rx_reg, dfs, word_mask);
			rx_buf += dfs;

			count--;
		}

		/* Make sure RX FIFO is empty */
		if (omap_mcspi_wait_for_reg_bit(chstat, OMAP_MCSPI_CHSTAT_RXFFE) != 0) {
			LOG_ERR("RXFFE timed out");
			return count;
		}

		/* TX only */
	} else if (!rx_buf) {
		while (count > 0) {
			size_t num_words = MIN(count, (data->fifo_depth / 2) / dfs);

			/* Make sure TX FIFO is empty */
			if (omap_mcspi_wait_for_reg_bit(chstat, OMAP_MCSPI_CHSTAT_TXFFE)) {
				LOG_ERR("TXFFE timed out");
				return count;
			}

			/* Write and fill the entire TX FIFO */
			for (int i = 0; i < num_words; i++) {
				write_tx(tx_buf, tx_reg, dfs);
				tx_buf += dfs;
			}

			count -= num_words;
		}

		/* Make sure TX FIFO is empty */
		if (omap_mcspi_wait_for_reg_bit(chstat, OMAP_MCSPI_CHSTAT_TXFFE)) {
			LOG_ERR("TXFFE timed out");
			return count;
		}

		/* Both RX and TX */
	} else {
		while (count > 0) {
			size_t num_words = MIN(count, (data->fifo_depth / 2) / dfs);

			/* Make sure TX FIFO is empty */
			if (omap_mcspi_wait_for_reg_bit(chstat, OMAP_MCSPI_CHSTAT_TXFFE)) {
				LOG_ERR("TXFFE timed out");
				return count;
			}

			/* Write and fill the entire TX FIFO */
			for (int i = 0; i < num_words; i++) {
				write_tx(tx_buf, tx_reg, dfs);
				tx_buf += dfs;
			}

			/* Read and empty the entire RX FIFO */
			for (int i = 0; i < num_words; i++) {
				if (omap_mcspi_wait_for_reg_bit(chstat, OMAP_MCSPI_CHSTAT_RXS)) {
					LOG_ERR("RXS timed out");
					return count;
				}

				read_rx(rx_buf, rx_reg, dfs, word_mask);
				rx_buf += dfs;
			}

			count -= num_words;
		}
	}

	omap_mcspi_channel_enable(dev, false);

	return count;
}

#ifdef CONFIG_SPI_OMAP_DMA
static int spi_dma_rx_tx_done(struct omap_mcspi_channel_data *chan_data)
{
	int ret;

	ret = k_sem_take(&chan_data->dma_completion_sem, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Sem take error %d", ret);
		return ret;
	}
	if (chan_data->dma_status_flags & DMA_CHANNEL_ERROR_FLAG) {
		LOG_ERR("Unexpected IO error");
		return -EIO;
	}
	if (chan_data->dma_status_flags == chan_data->dma_completion_flags) {
		return 0;
	}
	return ret;
}
static int omap_mcspi_transceive_dma(const struct device *dev, size_t *count)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	const uint8_t chan = ctx->config->slave;
	const uint8_t dfs = data->dfs;
	const uint8_t *tx_buf = ctx->tx_buf;
	uint8_t *rx_buf = ctx->rx_buf;
	struct omap_dma_data *rx_dma = NULL, *tx_dma = NULL;
	struct omap_mcspi_channel_data *chan_data = &data->chan_data[chan];

	k_sem_reset(&chan_data->dma_completion_sem); /* Reset DMA Semaphore */

	volatile uint32_t *tx_reg = &regs->CHAN[chan].TX;

	uint32_t ret;
	uint32_t rx_burst_len = 1, tx_burst_len = 1;
	uint32_t num_words_transferred = 0;

	if (FIELD_GET(OMAP_MCSPI_CHCONF_FFER, regs->CHAN[chan].CHCONF) &&
	    FIELD_GET(OMAP_MCSPI_CHCONF_FFEW, regs->CHAN[chan].CHCONF)) {
		rx_burst_len = tx_burst_len = data->fifo_depth / (2 * dfs);
	} else if (FIELD_GET(OMAP_MCSPI_CHCONF_FFER, regs->CHAN[chan].CHCONF)) {
		rx_burst_len = data->fifo_depth / (dfs);
	} else if (FIELD_GET(OMAP_MCSPI_CHCONF_FFEW, regs->CHAN[chan].CHCONF)) {
		tx_burst_len = data->fifo_depth / (dfs);
	}

	/* DMA Rx channel config */
	if (rx_buf) {
		while (ctx->current_rx->len % rx_burst_len) {
			rx_burst_len--;
		}
		rx_dma = &(chan_data->rx_dma_data);
		rx_dma->dma_cfg.source_data_size = dfs;
		rx_dma->dma_cfg.dest_data_size = dfs;
		rx_dma->dma_cfg.source_burst_length = rx_burst_len;
		rx_dma->dma_cfg.dest_burst_length = rx_burst_len;
		rx_dma->dma_cfg.user_data = (void *)chan_data;

		rx_dma->blk_cfg.source_address = (uint32_t)&(regs->CHAN[chan].RX);
		rx_dma->blk_cfg.dest_address = (uint32_t)ctx->current_rx->buf;
		rx_dma->blk_cfg.block_size = ctx->current_rx->len;
	}
	/* DMA Tx channel config */
	if (tx_buf) {
		while (ctx->current_tx->len % tx_burst_len) {
			tx_burst_len--;
		}

		tx_dma = &(chan_data->tx_dma_data);
		tx_dma->dma_cfg.source_data_size = dfs;
		tx_dma->dma_cfg.dest_data_size = dfs;
		tx_dma->dma_cfg.source_burst_length = tx_burst_len;
		tx_dma->dma_cfg.dest_burst_length = tx_burst_len;
		tx_dma->dma_cfg.user_data = (void *)chan_data;

		tx_dma->blk_cfg.source_address = (uint32_t)ctx->current_tx->buf;
		tx_dma->blk_cfg.dest_address = ((uint32_t)&(regs->CHAN[chan].TX));
		tx_dma->blk_cfg.block_size = ctx->current_tx->len;
	}

	/* Add DMA Trigger level for Rx and Tx */
	regs->XFERLEVEL = (OMAP_MCSPI_XFERLEVEL_WCNT & regs->XFERLEVEL) |
			  FIELD_PREP(OMAP_MCSPI_XFERLEVEL_AFL, (rx_burst_len * dfs - 1)) |
			  FIELD_PREP(OMAP_MCSPI_XFERLEVEL_AEL, (tx_burst_len * dfs - 1));

	/* RX only */
	if (!tx_buf) {
		/* write dummy value of 0 to TX FIFO */
		*tx_reg = 0;

		/* Set expected completion flag, reset the status flag */
		chan_data->dma_completion_flags = DMA_CHANNEL_RX_DONE_FLAG;
		chan_data->dma_status_flags = 0;

		/* Start Rx DMA Transfer */
		ret = dma_config(rx_dma->dma_dev, rx_dma->dma_channel, &rx_dma->dma_cfg);
		if (ret) {
			LOG_ERR("Rx DMA configuration failed.");
			return ret;
		}
		ret = dma_start(rx_dma->dma_dev, rx_dma->dma_channel);
		if (ret) {
			LOG_ERR("Rx DMA start failed.");
			return ret;
		}

		num_words_transferred = rx_dma->blk_cfg.block_size / dfs;

		/* TX only */
	} else if (!rx_buf) {

		/* Set expected completion flag, reset the status flag */
		chan_data->dma_completion_flags = DMA_CHANNEL_TX_DONE_FLAG;
		chan_data->dma_status_flags = 0;

		/* Start Tx DMA transfer */
		ret = dma_config(tx_dma->dma_dev, tx_dma->dma_channel, &tx_dma->dma_cfg);
		if (ret) {
			LOG_ERR("Tx DMA configuration failed.");
			return ret;
		}
		ret = dma_start(tx_dma->dma_dev, tx_dma->dma_channel);
		if (ret) {
			LOG_ERR("Tx DMA start failed.");
			return ret;
		}
		num_words_transferred = tx_dma->blk_cfg.block_size / dfs;

		/* Both RX and TX */
	} else {
		/* Set expected completion flag, reset the status flag */
		chan_data->dma_completion_flags =
			DMA_CHANNEL_RX_DONE_FLAG | DMA_CHANNEL_TX_DONE_FLAG;
		chan_data->dma_status_flags = 0;

		ret = dma_config(rx_dma->dma_dev, rx_dma->dma_channel, &rx_dma->dma_cfg);
		if (ret) {
			LOG_ERR("Rx DMA configuration failed.");
			return ret;
		}
		ret = dma_config(tx_dma->dma_dev, tx_dma->dma_channel, &tx_dma->dma_cfg);
		if (ret) {
			LOG_ERR("Tx DMA configuration failed.");
			return ret;
		}

		/* Start Rx DMA first */
		ret = dma_start(rx_dma->dma_dev, rx_dma->dma_channel);
		if (ret) {
			LOG_ERR("Rx DMA start failed.");
			return ret;
		}

		/* Start Tx DMA */
		ret = dma_start(tx_dma->dma_dev, tx_dma->dma_channel);
		if (ret) {
			LOG_ERR("Tx DMA start failed.");
			return ret;
		}
		num_words_transferred = tx_dma->blk_cfg.block_size / dfs;
	}
	/* Enable channel */
	omap_mcspi_channel_enable(dev, true);

	ret = spi_dma_rx_tx_done(chan_data);
	if (ret) {
		return ret;
	}
	*count = *count - num_words_transferred;

	/* Disable channel */
	omap_mcspi_channel_enable(dev, false);

	/* update rx buffer */
	spi_context_update_rx(ctx, data->dfs, num_words_transferred);

	/* update tx buffer */
	spi_context_update_tx(ctx, data->dfs, num_words_transferred);

	/* Stop DMA */
	if (rx_buf) {
		dma_stop(rx_dma->dma_dev, rx_dma->dma_channel);
	}
	if (tx_buf) {
		dma_stop(tx_dma->dma_dev, tx_dma->dma_channel);
	}

	return ret;
}
#endif /* CONFIG_SPI_OMAP_DMA */

static int omap_mcspi_transceive_one(const struct device *dev, bool asynchronous)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	const uint8_t chan = ctx->config->slave;
	const uint8_t *tx_buf = ctx->tx_buf;
	uint8_t *rx_buf = ctx->rx_buf;
	size_t count = spi_context_max_continuous_chunk(ctx);
	int rv;

	if (!tx_buf && !rx_buf) {
		goto exit;
	}

	/* disable channel */
	omap_mcspi_channel_enable(dev, false);

	data->chconf &= ~OMAP_MCSPI_CHCONF_TRM;

	if (rx_buf) {
		/* enable read FIFO */
		data->chconf |= OMAP_MCSPI_CHCONF_FFER;
	} else {
		/* tx only */
		data->chconf |= OMAP_MCSPI_CHCONF_TRM_TX_ONLY;

		/* disable read FIFO */
		data->chconf &= ~OMAP_MCSPI_CHCONF_FFER;
	}

	if (tx_buf) {
		/* enable write FIFO */
		data->chconf |= OMAP_MCSPI_CHCONF_FFEW;
	} else {
		/* rx only */
		data->chconf |= OMAP_MCSPI_CHCONF_TRM_RX_ONLY;

		/* disable write FIFO */
		data->chconf &= ~OMAP_MCSPI_CHCONF_FFEW;
	}

	/* Don't let Turbo Mode interfere with SPI+DMA */
#ifndef CONFIG_SPI_OMAP_DMA
	if (count > 1) {
		data->chconf |= OMAP_MCSPI_CHCONF_TURBO;
	} else {
		data->chconf &= ~OMAP_MCSPI_CHCONF_TURBO;
	}
#endif /* CONFIG_SPI_OMAP_DMA */

#ifdef CONFIG_SPI_OMAP_DMA
	if (rx_buf) {
		data->chconf |= OMAP_MCSPI_CHCONF_DMAR;
	}
	if (tx_buf) {
		data->chconf |= OMAP_MCSPI_CHCONF_DMAW;
	}
#endif

	/* write chconf and chctrl */
	regs->CHAN[chan].CHCONF = data->chconf;
	regs->CHAN[chan].CHCTRL = data->chctrl;

	/* write WCNT */
	regs->XFERLEVEL = FIELD_PREP(OMAP_MCSPI_XFERLEVEL_WCNT, count);

#ifdef CONFIG_SPI_OMAP_DMA
	if (asynchronous) {
		/*
		 * NOTE: Only one channel of McSPI can use FIFO
		 * DFS length may vary among channels if using multiple channels at a time
		 * Consider moving dfs size to specific channel data struct
		 */
		while (count > 0) {
			rv = omap_mcspi_transceive_dma(dev, &count);
			if (rv) {
				LOG_ERR("DMA transceive failed");
				return rv;
			}
		}
		return 0;
	}
#endif /* CONFIG_SPI_OMAP_DMA*/

	/* enable channel */
	omap_mcspi_channel_enable(dev, true);

	rv = omap_mcspi_transceive_pio(dev, count);
	if (rv) {
		return -EIO;
	}

exit:
	/* update rx buffer */
	spi_context_update_rx(ctx, data->dfs, count);

	/* update tx buffer */
	spi_context_update_tx(ctx, data->dfs, count);

	return 0;
}

#ifdef CONFIG_SPI_OMAP_DMA
static void omap_mcspi_transceive_work_handler(struct k_work *work)
{
	struct omap_mcspi_work_struct *work_struct =
		CONTAINER_OF(work, struct omap_mcspi_work_struct, work);
	const struct device *dev = work_struct->spi_dev;
	int ret = 0;

	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct spi_context *ctx = &data->ctx;

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		ret = omap_mcspi_transceive_one(dev, true);
		if (ret < 0) {
			LOG_ERR("Transaction failed, TX/RX left: %zu/%zu",
				spi_context_tx_len_left(ctx, data->dfs),
				spi_context_rx_len_left(ctx, data->dfs));
			break;
		}
	}

cleanup:
	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, ret);
}
#endif /* CONFIG_SPI_OMAP_DMA */

static int omap_mcspi_transceive_all(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, bool asynchronous,
				     spi_callback_t callback, void *userdata)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	spi_context_lock(ctx, asynchronous, callback, userdata, config);

	ret = omap_mcspi_configure(dev, config, tx_bufs, rx_bufs);
	if (ret) {
		LOG_ERR("An error occurred in the SPI configuration");
		goto cleanup;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);

	spi_context_cs_control(ctx, true);

#ifdef CONFIG_SPI_OMAP_DMA
	if (asynchronous) {
		k_work_init(&(data->work_struct.work), omap_mcspi_transceive_work_handler);
		ret = k_work_submit(&(data->work_struct.work));

		if (ret < 0) {
			LOG_ERR("Failed to submit workqueue");
			goto cleanup;
		}
		return 0;
	}
#endif

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		ret = omap_mcspi_transceive_one(dev, false);
		if (ret < 0) {
			LOG_ERR("Transaction failed, TX/RX left: %zu/%zu",
				spi_context_tx_len_left(ctx, data->dfs),
				spi_context_rx_len_left(ctx, data->dfs));
			goto cleanup;
		}
	}

cleanup:
	spi_context_cs_control(ctx, false);

	if (!(config->operation & SPI_LOCK_ON)) {
		spi_context_release(ctx, ret);
	}

	return ret;
}

static int omap_mcspi_transceive(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	return omap_mcspi_transceive_all(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int omap_mcspi_transceive_async(const struct device *dev, const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs, spi_callback_t callback,
				       void *userdata)
{
	/* wait for DMA to be implemented to use IRQ and ASYNC */
	return omap_mcspi_transceive_all(dev, config, tx_bufs, rx_bufs, true, callback, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int omap_mcspi_init(const struct device *dev)
{
	const struct omap_mcspi_cfg *cfg = DEV_CFG(dev);
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	regs = DEV_REGS(dev);

	if (cfg->num_cs > OMAP_MCSPI_NUM_CHANNELS) {
		LOG_ERR("chipselect count cannot be greater than max channel count");
		return -EINVAL;
	}

	ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to apply pinctrl");
		return ret;
	}

	/* Software Reset */
	regs->SYSCONFIG |= OMAP_MCSPI_SYSCONFIG_SOFTRESET;

	/* Wait till reset is done */
	ret = omap_mcspi_wait_for_reg_bit(&regs->SYSSTATUS, OMAP_MCSPI_SYSSTATUS_RESETDONE);
	if (ret < 0) {
		LOG_ERR("RESETDONE timed out");
		return ret;
	}

	data->fifo_depth = FIELD_GET(OMAP_MCSPI_HWINFO_FFNBYTE, regs->HWINFO) << 4;

#ifdef CONFIG_SPI_OMAP_DMA
	/* Ensure work_struct is pointing back to spi_dev */
	data->work_struct.spi_dev = dev;

	struct omap_mcspi_channel_data *chan_data;

	for (int i = 0; i < OMAP_MCSPI_NUM_CHANNELS; i++) {
		chan_data = &data->chan_data[i];
		k_sem_init(&chan_data->dma_completion_sem, 0, 1);

		chan_data->tx_dma_data.dma_cfg.head_block = &(chan_data->tx_dma_data.blk_cfg);
		chan_data->rx_dma_data.dma_cfg.head_block = &(chan_data->rx_dma_data.blk_cfg);
	}

#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int omap_mcspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#if defined(CONFIG_SPI_OMAP_DMA)

#define SPI_DMA_CHANNEL_INIT(n, dir, ch_dir, burst_len)                                            \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, dir)),                               \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(n, dir, channel),                                 \
	.dma_cfg = {                                                                               \
		.channel_direction = ch_dir,                                                       \
		.source_data_size = 1,                                                             \
		.dest_data_size = 1,                                                               \
		.source_burst_length = burst_len,                                                  \
		.dest_burst_length = burst_len,                                                    \
		.block_count = 1,                                                                  \
		.dma_callback = spi_dma_callback,                                                  \
		.complete_callback_en = true,                                                      \
	},

#define SPI_DMA_CHANNEL(n, dir, ch_dir, burst_len)                                                 \
	.dir##_dma_data = {COND_CODE_1(                                                           \
		DT_INST_DMAS_HAS_NAME(n, dir),                                                 \
		(SPI_DMA_CHANNEL_INIT(n, dir, ch_dir, burst_len)), (NULL))},

#define SPI_CHAN_DATA_STRUCT_INIT(index, n)                                                        \
	{.ch_num = index,                                                                          \
	 SPI_DMA_CHANNEL(n, rx, PERIPHERAL_TO_MEMORY, 1)                                           \
		 SPI_DMA_CHANNEL(n, tx, MEMORY_TO_PERIPHERAL, 1)}

#define SPI_CHAN_NUM_DUMMY_MACRO(i, _) i
#define SPI_CHAN_DATA_INIT(n)                                                                      \
	.chan_data = {FOR_EACH_FIXED_ARG(SPI_CHAN_DATA_STRUCT_INIT, (,), n,                        \
					 LISTIFY(4, SPI_CHAN_NUM_DUMMY_MACRO, (,)))}

#else
#define SPI_CHAN_DATA_INIT(n)
#endif /* CONFIG_SPI_OMAP_DMA */

/*
 * TODO: Every channel in McSPI has a unique pair of DMA channels
 * In DTS, need to define as rx0, tx0, rx1, tx1...
 * How to pull SPI Channel number from DTS?
 */

static DEVICE_API(spi, omap_mcspi_api) = {
	.transceive = omap_mcspi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = omap_mcspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = omap_mcspi_release,
};

#define OMAP_MCSPI_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct omap_mcspi_cfg omap_mcspi_config_##n = {                                     \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),                               \
		.d1_miso_d0_mosi = DT_INST_PROP(n, ti_d1_miso_d0_mosi),                            \
		.num_cs = DT_INST_PROP(n, ti_spi_num_cs),                                          \
	};                                                                                         \
                                                                                                   \
	static struct omap_mcspi_data omap_mcspi_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(omap_mcspi_data_##n, ctx),                                   \
		SPI_CONTEXT_INIT_SYNC(omap_mcspi_data_##n, ctx),                                   \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx) SPI_CHAN_DATA_INIT(n)};       \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, omap_mcspi_init, NULL, &omap_mcspi_data_##n,                  \
				  &omap_mcspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,   \
				  &omap_mcspi_api);

DT_INST_FOREACH_STATUS_OKAY(OMAP_MCSPI_INIT)
