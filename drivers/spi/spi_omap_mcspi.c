/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_omap_mcspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(omap_mcspi, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include "spi_context.h"
#include "spi_rtio.h"

#define OMAP_MCSPI_INTERRUPTS DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)

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
	volatile uint32_t IRQSTATUS;
	volatile uint32_t IRQENABLE;
	uint8_t RESERVED_3[0x08];    /**< Reserved, offset: 0x120 - 0x128 */
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
#define OMAP_MCSPI_CHSTAT_TXFFE BIT(3) /* Transmit buffer empty register */
#define OMAP_MCSPI_CHSTAT_RXFFE BIT(5) /* Receive buffer empty register */
#define OMAP_MCSPI_CHSTAT_EOT   BIT(2) /* End of transfer status */
#define OMAP_MCSPI_CHSTAT_TXS   BIT(1) /* Transmit register empty status */
#define OMAP_MCSPI_CHSTAT_RXS   BIT(0) /* Receiver register full status */

/*  FIFO transfer level register */
#define OMAP_MCSPI_XFERLEVEL_WCNT GENMASK(31, 16) /* Word counter for transfer */
#define OMAP_MCSPI_XFERLEVEL_AFL      GENMASK(15, 8)  /* Almost full level, in bytes minus 1 */
#define OMAP_MCSPI_XFERLEVEL_AEL      GENMASK(7, 0)   /* Almost empty level, in bytes minus 1 */
#define OMAP_MCSPI_XFERLEVEL_WCNT_MAX FIELD_GET(OMAP_MCSPI_XFERLEVEL_WCNT, UINT32_MAX)

#define OMAP_MCSPI_IRQ_TX_EMPTY(i)     BIT(4U * (i)) /* TX FIFO has room for AEL + 1 bytes */
#define OMAP_MCSPI_IRQ_TX_UNDERFLOW(i) BIT(4U * (i) + 1U)
#define OMAP_MCSPI_IRQ_RX_FULL(i)      BIT(4U * (i) + 2U) /* RX FIFO holds AFL + 1 bytes */
#define OMAP_MCSPI_IRQ_RX0_OVERFLOW    BIT(3)             /* RX overflow, peripheral mode */
#define OMAP_MCSPI_IRQ_EOW             BIT(17)            /* WCNT words transferred */

#define OMAP_MCSPI_WAIT_WORD_TIMES 2U
#define OMAP_MCSPI_WAIT_MARGIN_US  10U

#define DEV_CFG(dev)  ((const struct omap_mcspi_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct omap_mcspi_data *)(dev)->data)
#define DEV_REGS(dev) ((struct omap_mcspi_regs *)DEVICE_MMIO_GET(dev))

struct omap_mcspi_cfg {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pinctrl;
	uint32_t clock_frequency;
	bool d1_miso_d0_mosi;
	uint8_t num_cs;
#if OMAP_MCSPI_INTERRUPTS
	void (*irq_config)(const struct device *dev);
#endif
};

struct omap_mcspi_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	uint32_t fifo_depth;
	uint32_t chconf;
	uint32_t chctrl;
	uint8_t dfs; /* data frame size - word length in bytes */
	atomic_t pm_held;
#if OMAP_MCSPI_INTERRUPTS
	const uint8_t *irq_tx_buf;
	uint8_t *irq_rx_buf;
	size_t irq_count;
	size_t irq_tx_left;
	size_t irq_rx_left;
	size_t irq_level;
	/*
	 * Serialises the ISR, which may run on another core, against the
	 * thread starting, aborting or releasing a transfer.
	 */
	struct k_spinlock isr_lock;
	bool irq_active; /* the ISR owns the transfer */
	int irq_status;
#endif
};

static int omap_mcspi_pm_get(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	int ret;

	if (atomic_get(&data->pm_held) != 0) {
		return 0;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	atomic_set(&data->pm_held, 1);

	return 0;
}

static void omap_mcspi_pm_put(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);

	if (atomic_cas(&data->pm_held, 1, 0)) {
		(void)pm_device_runtime_put(dev);
	}
}

static bool omap_mcspi_pm_keep(const struct spi_config *config)
{
	return (config->operation & (SPI_LOCK_ON | SPI_HOLD_ON_CS)) != 0U;
}

static void omap_mcspi_channel_enable(const struct device *dev, bool enable)
{
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	uint8_t chan = DEV_DATA(dev)->ctx.config->peripheral;

	if (enable) {
		regs->CHAN[chan].CHCTRL |= OMAP_MCSPI_CHCTRL_EN;
	} else {
		regs->CHAN[chan].CHCTRL &= ~OMAP_MCSPI_CHCTRL_EN;
	}
}

static void omap_mcspi_hw_cs_control(const struct device *dev, bool enabled)
{
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	uint8_t chan = DEV_DATA(dev)->ctx.config->peripheral;
	struct omap_mcspi_data *data = DEV_DATA(dev);

	/*
	 * This works due to omap_mcspi_transceive_one writing data->chconf to the
	 * CHCONF register, meaning we also can't just write into CHCONF directly.
	 * After the transfer we can clear the FORCE bit directly inside the
	 * register.
	 */
	if (enabled) {
		data->chconf |= OMAP_MCSPI_CHCONF_FORCE;
	} else {
		regs->CHAN[chan].CHCONF &= ~OMAP_MCSPI_CHCONF_FORCE;
	}
}

static void omap_mcspi_cs_control(const struct device *dev, bool enabled)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct spi_context *ctx = &data->ctx;

	if (spi_cs_is_gpio(ctx->config)) {
		spi_context_cs_control(ctx, enabled);
	} else if (enabled || (ctx->config->operation & SPI_HOLD_ON_CS) == 0U) {
		omap_mcspi_hw_cs_control(dev, enabled);
	}
}

static void omap_mcspi_set_mode(const struct device *dev, bool is_peripheral)
{
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	uint32_t modulctrl = regs->MODULCTRL;

	/* disable system test mode */
	modulctrl &= ~(OMAP_MCSPI_MODULCTRL_SYSTEST);

	/* set controller or peripheral */
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
	uint8_t chan = config->peripheral;
	uint8_t word_size = SPI_WORD_SIZE_GET(config->operation);
	bool is_peripheral = config->operation & SPI_OP_MODE_PERIPHERAL;
	int rv;

	if (spi_context_configured(ctx, config)) {
		/* This configuration is already in use */
		return 0;
	}

	if (is_peripheral && !IS_ENABLED(CONFIG_SPI_PERIPHERAL)) {
		LOG_ERR("Kconfig for SPI peripheral mode is not enabled");
		return -ENOTSUP;
	}

	if (chan >= cfg->num_cs) {
		LOG_ERR("invalid peripheral selected");
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
	const uint8_t chan = ctx->config->peripheral;
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

static size_t omap_mcspi_chunk_len(struct spi_context *ctx)
{
	return MIN(spi_context_max_continuous_chunk(ctx), OMAP_MCSPI_XFERLEVEL_WCNT_MAX);
}

static void omap_mcspi_chunk_setup(const struct device *dev, size_t count, uint32_t xferlevel,
				   bool irq)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	const uint8_t chan = ctx->config->peripheral;
	const uint8_t *tx_buf = ctx->tx_buf;
	uint8_t *rx_buf = ctx->rx_buf;

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

	/*
	 * The TRM recommends turbo off for a single word, and turbo receive-only
	 * needs at least two. On the interrupt path, with it off, a one-word
	 * transmit-only chunk after a full-duplex one stalled on the AM62L EVM in
	 * spi_loopback_rtio, CHSTAT 0x06, so it is set there unless receive-only.
	 */
	if (count > 1 || (irq && tx_buf != NULL)) {
		data->chconf |= OMAP_MCSPI_CHCONF_TURBO;
	} else {
		data->chconf &= ~OMAP_MCSPI_CHCONF_TURBO;
	}

	/* write chconf and chctrl */
	regs->CHAN[chan].CHCONF = data->chconf;
	regs->CHAN[chan].CHCTRL = data->chctrl;

	/* write WCNT and the FIFO levels */
	regs->XFERLEVEL = FIELD_PREP(OMAP_MCSPI_XFERLEVEL_WCNT, count) | xferlevel;
}

static int omap_mcspi_transceive_one(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct spi_context *ctx = &data->ctx;
	size_t count = omap_mcspi_chunk_len(ctx);
	int rv;

	if (ctx->tx_buf == NULL && ctx->rx_buf == NULL) {
		goto exit;
	}

	omap_mcspi_chunk_setup(dev, count, 0U, false);

	/* enable channel */
	omap_mcspi_channel_enable(dev, true);

	/* we only support PIO for now
	 * TODO: add DMA
	 */
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

#if OMAP_MCSPI_INTERRUPTS
static void omap_mcspi_irq_stop(const struct device *dev)
{
	struct omap_mcspi_regs *regs = DEV_REGS(dev);

	regs->IRQENABLE = 0U;
	omap_mcspi_channel_enable(dev, false);
	regs->IRQSTATUS = UINT32_MAX;
}

static void omap_mcspi_irq_finish(const struct device *dev, int status)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);

	omap_mcspi_irq_stop(dev);
	data->irq_active = false;
	data->irq_status = status;
	omap_mcspi_cs_control(dev, false);

#ifdef CONFIG_SPI_ASYNC
	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME_ASYNC) && k_is_in_isr() &&
	    data->ctx.asynchronous && !omap_mcspi_pm_keep(data->ctx.config) &&
	    atomic_cas(&data->pm_held, 1, 0)) {
		if (pm_device_runtime_put_async(dev, K_NO_WAIT) < 0) {
			atomic_set(&data->pm_held, 1);
		}
	}
#endif

	spi_context_complete(&data->ctx, dev, status);
}

static uint32_t omap_mcspi_word_us(const struct spi_config *config)
{
	if (config->frequency == 0U) {
		return 0U;
	}

	return DIV_ROUND_UP(SPI_WORD_SIZE_GET(config->operation) * USEC_PER_SEC, config->frequency);
}

static void omap_mcspi_irq_write(const struct device *dev, size_t words)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	volatile uint32_t *tx_reg = &regs->CHAN[data->ctx.config->peripheral].TX;

	words = MIN(words, data->irq_tx_left);
	for (size_t i = 0U; i < words; i++) {
		write_tx(data->irq_tx_buf, tx_reg, data->dfs);
		data->irq_tx_buf += data->dfs;
	}
	data->irq_tx_left -= words;
}

static void omap_mcspi_irq_read(const struct device *dev, size_t words)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	const uint32_t word_mask = (1ULL << SPI_WORD_SIZE_GET(ctx->config->operation)) - 1U;
	volatile uint32_t *rx_reg = &regs->CHAN[ctx->config->peripheral].RX;

	words = MIN(words, data->irq_rx_left);
	for (size_t i = 0U; i < words; i++) {
		read_rx(data->irq_rx_buf, rx_reg, data->dfs, word_mask);
		data->irq_rx_buf += data->dfs;
	}
	data->irq_rx_left -= words;
}

static void omap_mcspi_irq_start(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	const uint8_t chan = ctx->config->peripheral;
	uint32_t irqs = OMAP_MCSPI_IRQ_EOW;
	size_t level_bytes;
	size_t count;

	while ((spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) && ctx->tx_buf == NULL &&
	       ctx->rx_buf == NULL) {
		count = omap_mcspi_chunk_len(ctx);
		spi_context_update_rx(ctx, data->dfs, count);
		spi_context_update_tx(ctx, data->dfs, count);
	}

	if (!spi_context_tx_on(ctx) && !spi_context_rx_on(ctx)) {
		omap_mcspi_irq_finish(dev, 0);
		return;
	}

	count = omap_mcspi_chunk_len(ctx);

	level_bytes = data->fifo_depth;
	if (ctx->tx_buf != NULL && ctx->rx_buf != NULL) {
		level_bytes /= 2U;
	}
	if (IS_ENABLED(CONFIG_SPI_PERIPHERAL) && spi_context_is_peripheral(ctx)) {
		level_bytes /= 2U;
	}
	level_bytes = ROUND_DOWN(level_bytes, data->dfs);
	/*
	 * With WCNT set, TX_EMPTY comes only on the edge to a free level while
	 * a whole level of words is still to be written. A tail shorter than
	 * the level never gets one, and the transfer stalls with that tail
	 * unsent. Seen on the AM62L EVM: 36 words at 32 per level left 4.
	 * Send whole levels as one chunk and the tail as the next, which the
	 * first fill below then writes in full.
	 */
	if (count * data->dfs > level_bytes) {
		count = ROUND_DOWN(count, level_bytes / data->dfs);
	} else {
		level_bytes = count * data->dfs;
	}

	data->irq_tx_buf = ctx->tx_buf;
	data->irq_rx_buf = ctx->rx_buf;
	data->irq_count = count;
	data->irq_tx_left = (ctx->tx_buf != NULL) ? count : 0U;
	data->irq_rx_left = (ctx->rx_buf != NULL) ? count : 0U;
	data->irq_level = level_bytes / data->dfs;

	omap_mcspi_chunk_setup(dev, count,
			       FIELD_PREP(OMAP_MCSPI_XFERLEVEL_AFL, level_bytes - 1U) |
				       FIELD_PREP(OMAP_MCSPI_XFERLEVEL_AEL, level_bytes - 1U),
			       true);

	omap_mcspi_channel_enable(dev, true);

	/* Status is reset after the channel is enabled and before the events. */
	regs->IRQSTATUS = UINT32_MAX;

	if (ctx->tx_buf != NULL) {
		/*
		 * Enabling the channel raises TX_EMPTY, and the reset above may
		 * have cleared it: fill the first level here. The next TX_EMPTY
		 * then comes once that level has room again.
		 */
		omap_mcspi_irq_write(dev, data->irq_level);
		/*
		 * Drop a TX_EMPTY from the enable that came after the reset:
		 * the FIFO now holds a whole level, so a genuine request needs
		 * that level to drain first.
		 */
		regs->IRQSTATUS = OMAP_MCSPI_IRQ_TX_EMPTY(chan);
		if (data->irq_tx_left > 0U) {
			irqs |= OMAP_MCSPI_IRQ_TX_EMPTY(chan);
		}
	} else {
		regs->CHAN[chan].TX = 0U;
	}

	if (ctx->rx_buf != NULL) {
		irqs |= OMAP_MCSPI_IRQ_RX_FULL(chan);
	}

	if (IS_ENABLED(CONFIG_SPI_PERIPHERAL) && spi_context_is_peripheral(ctx)) {
		irqs |= OMAP_MCSPI_IRQ_RX0_OVERFLOW;
		if (ctx->tx_buf != NULL) {
			irqs |= OMAP_MCSPI_IRQ_TX_UNDERFLOW(chan);
		}
	}

	regs->IRQENABLE = irqs;
}

static void omap_mcspi_irq_chunk_done(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	struct spi_context *ctx = &data->ctx;
	volatile uint32_t *chstat = &regs->CHAN[ctx->config->peripheral].CHSTAT;
	const size_t count = data->irq_count;
	const uint32_t word_us = omap_mcspi_word_us(ctx->config);
	const uint32_t wait_us = OMAP_MCSPI_WAIT_WORD_TIMES * word_us + OMAP_MCSPI_WAIT_MARGIN_US;

	regs->IRQENABLE = 0U;

	/*
	 * Words of the last level that did not fill an RX_FULL event. EOW may
	 * come as the last word is still being shifted in, so give each word
	 * up to two word times to land.
	 */
	while (data->irq_rx_left > 0U &&
	       WAIT_FOR((*chstat & OMAP_MCSPI_CHSTAT_RXFFE) == 0U, wait_us, NULL)) {
		omap_mcspi_irq_read(dev, 1U);
	}

	if (data->irq_rx_left > 0U || data->irq_tx_left > 0U) {
		omap_mcspi_irq_finish(dev, -EIO);
		return;
	}

	if (ctx->rx_buf == NULL) {
		/*
		 * Transmit only: the last word may still be in the shift
		 * register. It leaves within one word time; this spins in the
		 * ISR for up to two, which is about 42 ms for a 32-bit word at
		 * 1.5 kHz, the slowest clock a 50 MHz input divides down to.
		 */
		if (!WAIT_FOR((*chstat & OMAP_MCSPI_CHSTAT_EOT) != 0U, wait_us, NULL)) {
			omap_mcspi_irq_finish(dev, -EIO);
			return;
		}
	}

	omap_mcspi_irq_stop(dev);

	spi_context_update_rx(ctx, data->dfs, count);
	spi_context_update_tx(ctx, data->dfs, count);

	omap_mcspi_irq_start(dev);
}

static void omap_mcspi_isr_locked(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	const uint8_t chan = data->ctx.config->peripheral;
	const uint32_t status = regs->IRQSTATUS & regs->IRQENABLE;

	if ((status & (OMAP_MCSPI_IRQ_TX_UNDERFLOW(chan) | OMAP_MCSPI_IRQ_RX0_OVERFLOW)) != 0U) {
		omap_mcspi_irq_finish(dev, -EIO);
		return;
	}

	if ((status & OMAP_MCSPI_IRQ_RX_FULL(chan)) != 0U) {
		regs->IRQSTATUS = OMAP_MCSPI_IRQ_RX_FULL(chan);
		omap_mcspi_irq_read(dev, data->irq_level);
	}

	if ((status & OMAP_MCSPI_IRQ_TX_EMPTY(chan)) != 0U) {
		regs->IRQSTATUS = OMAP_MCSPI_IRQ_TX_EMPTY(chan);
		omap_mcspi_irq_write(dev, data->irq_level);
		if (data->irq_tx_left == 0U) {
			regs->IRQENABLE &= ~OMAP_MCSPI_IRQ_TX_EMPTY(chan);
		}
	}

	if ((status & OMAP_MCSPI_IRQ_EOW) != 0U) {
		regs->IRQSTATUS = OMAP_MCSPI_IRQ_EOW;
		omap_mcspi_irq_chunk_done(dev);
	}
}

static void omap_mcspi_isr(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	k_spinlock_key_t key = k_spin_lock(&data->isr_lock);

	if (data->irq_active) {
		omap_mcspi_isr_locked(dev);
	} else {
		regs->IRQENABLE = 0U;
		regs->IRQSTATUS = UINT32_MAX;
	}

	k_spin_unlock(&data->isr_lock, key);
}
#endif

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

	ret = omap_mcspi_pm_get(dev);
	if (ret < 0) {
		LOG_ERR("Failed to resume the controller (%d)", ret);
		goto cleanup_no_cs;
	}

	ret = omap_mcspi_configure(dev, config, tx_bufs, rx_bufs);
	if (ret) {
		LOG_ERR("An error occurred in the SPI configuration");
		goto cleanup_no_cs;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);

	omap_mcspi_cs_control(dev, true);

#if OMAP_MCSPI_INTERRUPTS
	if (DEV_CFG(dev)->irq_config != NULL) {
		k_spinlock_key_t key = k_spin_lock(&data->isr_lock);

		data->irq_active = true;
		omap_mcspi_irq_start(dev);
		k_spin_unlock(&data->isr_lock, key);

		/*
		 * spi_context_complete() releases an asynchronous transfer, which
		 * may already have happened: do not touch ctx after this.
		 */
		if (asynchronous) {
			return 0;
		}

		ret = spi_context_wait_for_completion(ctx);
		if (ret == -EIO) {
			LOG_ERR("Transfer failed, TX/RX left %zu/%zu of %zu", data->irq_tx_left,
				data->irq_rx_left, data->irq_count);
		}
		if (ret == -ETIMEDOUT) {
			key = k_spin_lock(&data->isr_lock);
			if (data->irq_active) {
				struct omap_mcspi_regs *regs = DEV_REGS(dev);
				const uint8_t chan = config->peripheral;

				LOG_ERR("Timed out, IRQSTATUS 0x%08x IRQENABLE 0x%08x "
					"CHSTAT 0x%08x, TX/RX left %zu/%zu of %zu",
					regs->IRQSTATUS, regs->IRQENABLE, regs->CHAN[chan].CHSTAT,
					data->irq_tx_left, data->irq_rx_left, data->irq_count);
				omap_mcspi_irq_stop(dev);
				data->irq_active = false;
				omap_mcspi_cs_control(dev, false);
			} else {
				/* the ISR completed between the timeout and the lock */
				ret = data->irq_status;
			}
			spi_context_clear_completion(ctx);
			k_spin_unlock(&data->isr_lock, key);
		}

		if (!asynchronous && (config->operation & SPI_LOCK_ON) == 0U) {
			if (!omap_mcspi_pm_keep(config)) {
				omap_mcspi_pm_put(dev);
			}
			spi_context_release(ctx, ret);
		}

		return ret;
	}
#endif

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		ret = omap_mcspi_transceive_one(dev);
		if (ret < 0) {
			LOG_ERR("Transaction failed, TX/RX left: %zu/%zu",
				spi_context_tx_len_left(ctx, data->dfs),
				spi_context_rx_len_left(ctx, data->dfs));
			goto cleanup;
		}
	}

cleanup:
	omap_mcspi_cs_control(dev, false);

cleanup_no_cs:
	if (!(config->operation & SPI_LOCK_ON)) {
		if (!omap_mcspi_pm_keep(config)) {
			omap_mcspi_pm_put(dev);
		}
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
#if OMAP_MCSPI_INTERRUPTS
	if (DEV_CFG(dev)->irq_config != NULL) {
		return omap_mcspi_transceive_all(dev, config, tx_bufs, rx_bufs, true, callback,
						 userdata);
	}
#endif

	/* Without an interrupt, transfers can only complete in the caller. */
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int omap_mcspi_hw_init(const struct device *dev)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);
	int ret;

	/* Software Reset */
	regs->SYSCONFIG |= OMAP_MCSPI_SYSCONFIG_SOFTRESET;

	/* Wait till reset is done */
	ret = omap_mcspi_wait_for_reg_bit(&regs->SYSSTATUS, OMAP_MCSPI_SYSSTATUS_RESETDONE);
	if (ret < 0) {
		LOG_ERR("RESETDONE timed out");
		return ret;
	}

	data->fifo_depth = FIELD_GET(OMAP_MCSPI_HWINFO_FFNBYTE, regs->HWINFO) << 4;

	/*
	 * The reset above clears EPOL on every channel, which makes SPIEN
	 * active high and so leaves the line asserted while the channel sits
	 * unused. A peripheral whose chip select is active low is then
	 * selected from here until its own first transfer configures the
	 * channel, and it takes every transfer meant for another peripheral
	 * as its own. Deassert them all up front; a channel that really is
	 * active high has EPOL cleared again when it is configured.
	 */
	ARRAY_FOR_EACH(regs->CHAN, ch) {
		regs->CHAN[ch].CHCONF &= ~OMAP_MCSPI_CHCONF_FORCE;
		regs->CHAN[ch].CHCONF |= OMAP_MCSPI_CHCONF_EPOL;
	}

	data->ctx.config = NULL;

#if OMAP_MCSPI_INTERRUPTS
	if (DEV_CFG(dev)->irq_config != NULL) {
		regs->IRQENABLE = 0U;
		regs->IRQSTATUS = UINT32_MAX;
	}
#endif

	return 0;
}

static int omap_mcspi_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct omap_mcspi_cfg *cfg = DEV_CFG(dev);
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		return omap_mcspi_hw_init(dev);
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("failed to apply pinctrl");
		}
		return ret;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_SLEEP);
		return (ret == -ENOENT) ? 0 : ret;
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int omap_mcspi_init(const struct device *dev)
{
	const struct omap_mcspi_cfg *cfg = DEV_CFG(dev);
	struct omap_mcspi_data *data = DEV_DATA(dev);
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (cfg->num_cs > OMAP_MCSPI_NUM_CHANNELS) {
		LOG_ERR("chipselect count cannot be greater than max channel count");
		return -EINVAL;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}
	spi_context_unlock_unconditionally(&data->ctx);

	ret = pm_device_driver_init(dev, omap_mcspi_pm_action);
	if (ret < 0) {
		return ret;
	}

#if OMAP_MCSPI_INTERRUPTS
	if (cfg->irq_config != NULL) {
		cfg->irq_config(dev);
	}
#endif

	return 0;
}

static int omap_mcspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct omap_mcspi_data *data = DEV_DATA(dev);
	struct omap_mcspi_regs *regs = DEV_REGS(dev);

#if OMAP_MCSPI_INTERRUPTS
	if (DEV_CFG(dev)->irq_config != NULL) {
		k_spinlock_key_t key = k_spin_lock(&data->isr_lock);

		if (data->irq_active) {
			omap_mcspi_irq_finish(dev, -ECANCELED);
		}
		k_spin_unlock(&data->isr_lock, key);
	}
#endif

	if (data->ctx.config != NULL && !spi_cs_is_gpio(data->ctx.config) &&
	    atomic_get(&data->pm_held) != 0) {
		regs->CHAN[data->ctx.config->peripheral].CHCONF &= ~OMAP_MCSPI_CHCONF_FORCE;
	}

	omap_mcspi_pm_put(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, omap_mcspi_api) = {
	.transceive = omap_mcspi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = omap_mcspi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = omap_mcspi_release,
};

#if OMAP_MCSPI_INTERRUPTS
#define OMAP_MCSPI_IRQ_FUNC(n)                                                                     \
	static void omap_mcspi_irq_config_##n(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), omap_mcspi_isr,             \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define OMAP_MCSPI_IRQ_DEFINE(n)                                                                   \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (OMAP_MCSPI_IRQ_FUNC(n)), ())
#define OMAP_MCSPI_IRQ_INIT(n)                                                                     \
	.irq_config = COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (omap_mcspi_irq_config_##n), (NULL)),
#else
#define OMAP_MCSPI_IRQ_DEFINE(n)
#define OMAP_MCSPI_IRQ_INIT(n)
#endif

#define OMAP_MCSPI_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	OMAP_MCSPI_IRQ_DEFINE(n)                                                                   \
	static struct omap_mcspi_cfg omap_mcspi_config_##n = {                                     \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),                               \
		.d1_miso_d0_mosi = DT_INST_PROP(n, ti_d1_miso_d0_mosi),                            \
		.num_cs = DT_INST_PROP(n, ti_spi_num_cs),                                          \
		OMAP_MCSPI_IRQ_INIT(n)                                                             \
	};                                                                                         \
                                                                                                   \
	static struct omap_mcspi_data omap_mcspi_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(omap_mcspi_data_##n, ctx),                                   \
		SPI_CONTEXT_INIT_SYNC(omap_mcspi_data_##n, ctx),                                   \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, omap_mcspi_pm_action);                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(n, omap_mcspi_init, PM_DEVICE_DT_INST_GET(n),                    \
				  &omap_mcspi_data_##n,                                            \
				  &omap_mcspi_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,   \
				  &omap_mcspi_api);

DT_INST_FOREACH_STATUS_OKAY(OMAP_MCSPI_INIT)
