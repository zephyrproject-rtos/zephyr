/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_cadence, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/*******************************************************************************
 * Macro Definition
 ******************************************************************************/
/* Register offset address */
#define SPI_CONF             0x00
#define SPI_INT_STATUS       0x04
#define SPI_INT_ENABLE       0x08
#define SPI_INT_DISABLE      0x0c
#define SPI_INT_MASK         0x10
#define SPI_SPI_ENABLE       0x14
#define SPI_DELAY            0x18
#define SPI_TX_DATA          0x1c
#define SPI_RX_DATA          0x20
#define SPI_SLAVE_IDLE_COUNT 0x24
#define SPI_TX_THRESHOLD     0x28
#define SPI_RX_THRESHOLD     0x2c

/* Configuration register bit offset */
#define SPI_CONF_PCSL_OFFSET 10
#define SPI_CONF_MRCS_OFFSET 8
#define SPI_CONF_TWS_OFFSET  6
#define SPI_CONF_MBRD_OFFSET 3

/* Configuration register bit mask */
#define SPI_CONF_TXCLR     BIT(20)
#define SPI_CONF_RXCLR     BIT(19)
#define SPI_CONF_SPSE      BIT(18)
#define SPI_CONF_MFGE      BIT(17)
#define SPI_CONF_MSC       BIT(16)
#define SPI_CONF_MSE       BIT(15)
#define SPI_CONF_MCSE      BIT(14)
#define SPI_CONF_PCSL_MASK GENMASK(13, 10)
#define SPI_CONF_PSD       BIT(9)
#define SPI_CONF_MRCS      BIT(8)
#define SPI_CONF_TWS_MASK  GENMASK(7, 6)
#define SPI_CONF_MBRD_MASK GENMASK(5, 3)
#define SPI_CONF_CPHA      BIT(2)
#define SPI_CONF_CPOL      BIT(1)
#define SPI_CONF_MSEL      BIT(0)

#define SPI_CONF_INITIAL_VAL (SPI_CONF_PCSL_MASK | SPI_CONF_MCSE | SPI_CONF_MRCS | SPI_CONF_MSEL)

/* Interrupt register bit mask */
#define SPI_INT_TUF BIT(6)
#define SPI_INT_RF  BIT(5)
#define SPI_INT_RNE BIT(4)
#define SPI_INT_TF  BIT(3)
#define SPI_INT_TNF BIT(2)
#define SPI_INT_MF  BIT(1)
#define SPI_INT_ROF BIT(0)

#define SPI_INT_DEFAULT (SPI_INT_RNE | SPI_INT_TNF | SPI_INT_ROF | SPI_INT_TUF)

/* SPI enable register bit offset */
#define SPI_SPI_ENABLE_SPIE BIT(0)

/* Maximum baud rate divisor */
#define SPI_MBRD_MIN 0
#define SPI_MBRD_MAX 7

#define SPI_FREQ_LIST_MAX ((SPI_MBRD_MAX + 1) * 2 + 1)

#define SPI_CFG(dev)         ((struct spi_cdns_cfg *)(dev->config))
#define SPI_REG(dev, offset) ((mem_addr_t)(SPI_CFG(dev)->base + (offset)))
/*******************************************************************************
 * Types Definition
 ******************************************************************************/
typedef void (*irq_config_func_t)(void);

/**
 * @brief SPI Driver config information.
 *
 * This parameter isn't updated after initialization.
 *
 * @param base                         SPI register base address.
 * @param clock_frequency              Peripheral bus clock
 * @param ext_clock                    External clock frequency.
 * @param cs_setup_us                  Array of durations from CS assert to
 *                                     SCLK in us for slaves.
 * @param cs_hold_us                   Array of durations from CS assert to
 *                                     SCLK in us for slaves.
 * @param irq_config                   Interrupt configuration function.
 * @param leave_enabled_during_config  Conditionally enable or
 *                                     disable the SPI bus during
 *                                     configuration
 * @param freq_list                    Selectable clock
 *                                     frequency list.
 */
struct spi_cdns_cfg {
	uint32_t base;
	uint32_t clock_frequency;
	uint32_t ext_clock;
	irq_config_func_t irq_config;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
	uint8_t fifo_width;
	uint16_t rx_fifo_depth;
	uint16_t tx_fifo_depth;
};

/**
 * @brief SPI Driver private data.
 *
 * @param ctx             Transceive context information.
 * @param config          Current SPI controller configuration structure
 * @param freq            Actual transfer frequency.
 * @param tx_remain_entry Remain entries to Tx-FIFO.
 * @param fifo_diff       Difference between Tx-FIFO entry and Rx-FIFO entry.
 */
struct spi_cdns_data {
	struct spi_context ctx;
	struct spi_config config;
	uint32_t freq;
	uint32_t tx_remain_entry;
	int32_t fifo_diff;
};

/*******************************************************************************
 * Private Functions Code
 ******************************************************************************/

/**
 * @brief Mask-write 32-bit value to register.
 *
 * @param addr register address.
 * @param mask 32-bit Mask value.
 * @param value 32-bit value to be written.
 * @return None.
 */
static inline void sys_set_mask32(mem_addr_t addr, uint32_t mask, uint32_t value)
{
	uint32_t temp = sys_read32(addr);

	temp &= ~(mask);
	temp |= value;

	sys_write32(temp, addr);
}

/**
 * @brief Check whether to update context configuration.
 *
 * @param dev Device structure (In memory) per driver instance
 * @param config SPI controller configuration structure
 * @retval true Configuration is same.
 * @retval false Configuration is differ.
 */
static inline bool spi_cdns_context_configured(const struct device *dev,
					       const struct spi_config *config)
{
	struct spi_cdns_data *data = dev->data;

	if (spi_context_configured(&data->ctx, config) &&
	    (data->config.frequency == config->frequency) &&
	    (data->config.operation == config->operation) &&
	    (data->config.slave == config->slave)) {
		return true;
	}

	return false;
}

/**
 * @brief Enable/Disable SPI controller
 *
 * @param dev Device structure (In memory) per driver instance
 * @param on SPI enable flag (True: Enable, False: Disable)
 * @return None.
 */
static inline void spi_cdns_spi_enable(const struct device *dev, bool on)
{
	if (on) {
		sys_set_bits(SPI_REG(dev, SPI_SPI_ENABLE), SPI_SPI_ENABLE_SPIE);
	} else {
		sys_clear_bits(SPI_REG(dev, SPI_SPI_ENABLE), SPI_SPI_ENABLE_SPIE);
	}
}

/**
 * @brief Assert/Deassert chip select line
 *
 * @param dev Device structure (In memory) per driver instance
 * @param on Assert chip select (True: Assert, False: Deassert)
 * @return None.
 */
static inline void spi_cdns_cs_control(const struct device *dev, bool on)
{
	struct spi_cdns_data *data = dev->data;

	if (IS_ENABLED(CONFIG_SPI_SLAVE) && spi_context_is_slave(&data->ctx)) {
		/* Skip slave select assert/de-assert in slave mode */
		return;
	}

	if (on) {
		uint32_t val = SPI_CONF_PCSL_MASK &
			       ~(1 << (SPI_CONF_PCSL_OFFSET + data->ctx.config->slave));
		sys_set_mask32(SPI_REG(dev, SPI_CONF), SPI_CONF_PCSL_MASK, val);
		k_busy_wait(data->ctx.config->cs.delay);
	} else if (!(data->ctx.config->operation & SPI_HOLD_ON_CS)) {
		k_busy_wait(data->ctx.config->cs.delay);
		sys_set_mask32(SPI_REG(dev, SPI_CONF), SPI_CONF_PCSL_MASK, SPI_CONF_PCSL_MASK);
	}
}

static void spi_cdns_config_clock_freq(const struct device *dev, uint32_t spi_freq)
{
	const struct spi_cdns_cfg *cfg = dev->config;
	uint32_t ctrl_reg, baud_rate_div;
	uint32_t clock_freq;

	clock_freq = cfg->clock_frequency;

	ctrl_reg = sys_read32(SPI_REG(dev, SPI_CONF));

	/*
	 * Set the clock frequency
	 * first valid value is 0 (/2)
	 */
	baud_rate_div = SPI_MBRD_MIN;
	while ((baud_rate_div < SPI_MBRD_MAX) && (clock_freq / (2 << baud_rate_div)) > spi_freq) {
		baud_rate_div++;
	}

	ctrl_reg &= ~SPI_CONF_MBRD_MASK;
	ctrl_reg |= baud_rate_div << SPI_CONF_MBRD_OFFSET;

	LOG_DBG("%s: spi baud rate %uHz", dev->name, clock_freq / (2 << baud_rate_div));

	sys_write32(ctrl_reg, SPI_REG(dev, SPI_CONF));
}

/**
 * @brief Send 1-entry to Tx-FIFO
 *
 * @param dev Device structure (In memory) per driver instance
 * @return None.
 */
static void spi_cdns_send(const struct device *dev)
{
	const struct spi_cdns_cfg *config = dev->config;
	struct spi_cdns_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = SPI_WORD_SIZE_GET(ctx->config->operation) / 8;
	uint32_t val = 0;
	int i, loop;

	loop = (config->fifo_width / 8) / dfs;
	for (i = 0; i < loop; i++) {
		if (spi_context_tx_buf_on(ctx)) {
			switch (dfs) {
			case 1:
				if (config->fifo_width == 8) {
					val |= UNALIGNED_GET((uint8_t *)ctx->tx_buf);
				} else if (config->fifo_width == 16) {
					val |= UNALIGNED_GET((uint8_t *)ctx->tx_buf) << 8 * (1 - i);
				} else if (config->fifo_width == 32) {
					val |= UNALIGNED_GET((uint8_t *)ctx->tx_buf) << 8 * (3 - i);
				}
				break;
			case 2:
				if (config->fifo_width == 16) {
					val |= UNALIGNED_GET((uint16_t *)ctx->tx_buf);
				} else if (config->fifo_width == 32) {
					val |= UNALIGNED_GET((uint16_t *)ctx->tx_buf)
					       << 16 * (1 - i);
				}
				break;
			case 4:
				if (config->fifo_width == 32) {
					val |= UNALIGNED_GET((uint32_t *)ctx->tx_buf);
				}
				break;
			}
		}
		if ((spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx))) {
			if (data->tx_remain_entry > 0) {
				data->tx_remain_entry--;
				data->fifo_diff++;
			}
		}
		spi_context_update_tx(&data->ctx, dfs, 1);
	}

	sys_write32(val, SPI_REG(dev, SPI_TX_DATA));
}

/**
 * @brief Receive 1-entry from Rx-FIFO
 *
 * @param dev Device structure (In memory) per driver instance
 * @return None.
 */
static void spi_cdns_recv(const struct device *dev)
{
	const struct spi_cdns_cfg *config = dev->config;
	struct spi_cdns_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = SPI_WORD_SIZE_GET(ctx->config->operation) / 8;
	uint32_t val;
	int i, loop;

	val = sys_read32(SPI_REG(dev, SPI_RX_DATA));

	loop = (config->fifo_width / 8) / dfs;
	for (i = 0; i < loop; i++) {
		if (spi_context_rx_buf_on(ctx)) {
			switch (dfs) {
			case 1:
				if (config->fifo_width == 8) {
					UNALIGNED_PUT(val & 0xFF, (uint8_t *)ctx->rx_buf);
				} else if (config->fifo_width == 16) {
					UNALIGNED_PUT((val >> 8 * (1 - i)) & 0xFF,
						      (uint8_t *)ctx->rx_buf);
				} else if (config->fifo_width == 32) {
					UNALIGNED_PUT((val >> 8 * (3 - i)) & 0xFF,
						      (uint8_t *)ctx->rx_buf);
				}
				break;
			case 2:
				if (config->fifo_width == 16) {
					UNALIGNED_PUT(val & 0xFFFF, (uint16_t *)ctx->rx_buf);
				} else if (config->fifo_width == 32) {
					UNALIGNED_PUT((val >> 16 * (1 - i)) & 0xFFFF,
						      (uint16_t *)ctx->rx_buf);
				}
				break;
			case 4:
				if (config->fifo_width == 32) {
					UNALIGNED_PUT(val, (uint32_t *)ctx->rx_buf);
				}
				break;
			}
		}
		if (data->fifo_diff > 0) {
			data->fifo_diff--;
		}
		spi_context_update_rx(ctx, dfs, 1);
	}
}

/**
 * @brief Push to Tx-FIFO
 *
 * @param dev Device structure (In memory) per driver instance
 * @return None.
 */
static void spi_cdns_push_data(const struct device *dev)
{
	const struct spi_cdns_cfg *config = dev->config;
	struct spi_cdns_data *data = dev->data;
	uint32_t tx_entry;

	if (spi_context_is_slave(&data->ctx)) {
		/*
		 * while tx fifo is not full and there is data to transmit, as we are a target fill
		 * it up until we are full
		 */
		while ((!(sys_read32(SPI_REG(dev, SPI_INT_STATUS)) & SPI_INT_TF)) &&
		       (data->tx_remain_entry > 0)) {
			spi_cdns_send(dev);
		}
	} else {
		/*
		 * We can't fill until we are full as we could chase our tail with waiting until we
		 * are full, while at the same time data is being sent out faster than we can check
		 * if we are full
		 */
		tx_entry = MIN(config->tx_fifo_depth - data->fifo_diff, data->tx_remain_entry);
		while (tx_entry > 0) {
			spi_cdns_send(dev);
			tx_entry--;
		}
	}
}

/**
 * @brief Pull from Rx-FIFO
 *
 * @param dev Device structure (In memory) per driver instance
 * @return None.
 */
static void spi_cdns_pull_data(const struct device *dev)
{
	const struct spi_cdns_cfg *config = dev->config;
	struct spi_cdns_data *data = dev->data;
	uint32_t rx_threshold_tmp;
	uint32_t rx_remain_entry;

	/*
	 * As there is no rx fifo empty status bit, Write the rx threshold
	 * to so the rne status bit will report when there is less than 1
	 * item in the fifo
	 */
	rx_threshold_tmp = sys_read32(SPI_REG(dev, SPI_RX_THRESHOLD));
	sys_write32(1, SPI_REG(dev, SPI_RX_THRESHOLD));

	while (sys_read32(SPI_REG(dev, SPI_INT_STATUS)) & SPI_INT_RNE) {
		spi_cdns_recv(dev);
	}

	/*
	 * The threshold is designed to trigger by FIFO I/O.
	 * Therefore, it is necessary to set rx threshold before pulling.
	 */
	rx_remain_entry = DIV_ROUND_UP(data->fifo_diff, (config->fifo_width / 8));
	if ((rx_remain_entry != 0) && (rx_remain_entry < rx_threshold_tmp)) {
		sys_write32(rx_remain_entry, SPI_REG(dev, SPI_RX_THRESHOLD));
	} else {
		sys_write32(rx_threshold_tmp, SPI_REG(dev, SPI_RX_THRESHOLD));
	}
}

/**
 * @brief Configure SPI controller
 *
 * @param dev Device structure (In memory) per driver instance
 * @param config SPI controller configuration structure
 * @retval 0 No errors.
 * @retval -EINVAL Invalid argument error.
 * @retval -ENOTSUP Unsupported value error.
 */
static int spi_cdns_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_cdns_cfg *dev_config = dev->config;
	struct spi_cdns_data *data = dev->data;
	uint32_t word_size, conf_val;

	if (spi_cdns_context_configured(dev, config)) {
		/* Nothing to do */
		return 0;
	}

	if (config->operation & (SPI_MODE_LOOP | SPI_TRANSFER_LSB | SPI_LINES_DUAL |
				 SPI_LINES_QUAD | SPI_LINES_OCTAL)) {
		return -ENOTSUP;
	}

	/* Active High CS is not supported with hardware CS */
	if (!spi_cs_is_gpio(config) && (config->operation & SPI_CS_ACTIVE_HIGH)) {
		return -ENOTSUP;
	}

	if ((config->operation & SPI_OP_MODE_SLAVE) && !IS_ENABLED(CONFIG_SPI_SLAVE)) {
		LOG_ERR("Kconfig for enable SPI in slave mode is not enabled");
		return -ENOTSUP;
	}

	/* Word Sizes are only compatible with certain fifo widths */
	word_size = SPI_WORD_SIZE_GET(config->operation);
	if (((word_size != 8) && (word_size != 16) && (word_size != 32)) ||
	    (word_size > dev_config->fifo_width) ||
	    ((dev_config->fifo_width == 24) && (word_size == 16)) ||
	    ((dev_config->fifo_width == 32) && (word_size == 24))) {
		return -ENOTSUP;
	}

	data->ctx.config = config;
	data->config = *config;

	conf_val = SPI_CONF_PCSL_MASK | SPI_CONF_MCSE | SPI_CONF_MRCS;

	/* Configure for Master or Slave */
	if (config->operation & SPI_OP_MODE_SLAVE) {
		conf_val &= ~(SPI_CONF_MSEL);
	} else {
		conf_val |= SPI_CONF_MSEL;
	}

	/* Set the polarity */
	if (config->operation & SPI_MODE_CPHA) {
		conf_val |= SPI_CONF_CPHA;
	} else {
		conf_val &= ~(SPI_CONF_CPHA);
	}

	/* Set the phase */
	if (config->operation & SPI_MODE_CPOL) {
		conf_val |= SPI_CONF_CPOL;
	} else {
		conf_val &= ~(SPI_CONF_CPOL);
	}

	/*
	 * Set clock frequency.
	 * SPI clock is generated based on pclk or ext_clk, and the frequency closest
	 * to the value obtained by dividing the two base clocks is selected.
	 */
	spi_cdns_config_clock_freq(dev, config->frequency);

	/* Set transfer word size */
	conf_val &= ~(SPI_CONF_TWS_MASK);
	conf_val |= ((word_size / 8) - 1) << SPI_CONF_TWS_OFFSET;

	sys_write32(conf_val, SPI_REG(dev, SPI_CONF));

	return 0;
}

/**
 * @brief Interrupt handler
 *
 * @param dev Device structure (In memory) per driver instance
 * @return None.
 */
static void spi_cdns_isr(const struct device *dev)
{
	struct spi_cdns_data *data = dev->data;
	int32_t int_status;
	int error = 0;

	int_status = sys_read32(SPI_REG(dev, SPI_INT_STATUS));
	sys_write32(int_status, SPI_REG(dev, SPI_INT_STATUS));

	if ((int_status & SPI_INT_ROF) && spi_context_rx_buf_on(&data->ctx)) {
		LOG_ERR("%s: rx fifo overflow", dev->name);
		error = -EIO;
		goto complete;
	}

	if ((int_status & SPI_INT_TUF) && spi_context_tx_buf_on(&data->ctx)) {
		LOG_ERR("%s: tx fifo underflow", dev->name);
		error = -EIO;
		goto complete;
	}

	if (int_status & SPI_INT_RNE) {
		spi_cdns_pull_data(dev);
	}

	if (int_status & SPI_INT_TNF) {
		spi_cdns_push_data(dev);
	}

	if (!spi_context_tx_buf_on(&data->ctx)) {
		/* Disable Tx-FIFO interrupt for no transfer data */
		sys_write32(SPI_INT_TNF, SPI_REG(dev, SPI_INT_DISABLE));
	}

	if (spi_context_tx_buf_on(&data->ctx) || spi_context_rx_buf_on(&data->ctx)) {
		return;
	}

	if (data->fifo_diff != 0) {
		return;
	}

complete:
	sys_write32(SPI_INT_DEFAULT, SPI_REG(dev, SPI_INT_DISABLE));
#if CONFIG_SPI_ASYNC
	if (data->ctx.asynchronous) {
		if (spi_cs_is_gpio(data->ctx.config)) {
			spi_context_cs_control(&data->ctx, false);
		} else {
			spi_cdns_cs_control(dev, false);
		}
		pm_device_busy_clear(dev);
		pm_device_runtime_put(dev);
	}
#endif

	spi_context_complete(&data->ctx, dev, error);
}

/**
 * @brief Initialize SPI driver
 *
 * @param dev Device structure (In memory) per driver instance
 * @return None.
 */
static int spi_cdns_init(const struct device *dev)
{
	const struct spi_cdns_cfg *cfg = dev->config;
	struct spi_cdns_data *data = dev->data;

	cfg->irq_config();

	sys_write32(SPI_CONF_INITIAL_VAL, SPI_REG(dev, SPI_CONF));

	/* Disable interrupt */
	sys_write32(SPI_INT_DEFAULT, SPI_REG(dev, SPI_INT_DISABLE));
	/* Clear Pending Interrupts */
	(void)sys_read32(SPI_REG(dev, SPI_INT_STATUS));

	/* TxFIFO and RxFIFO clear */
	sys_set_mask32(SPI_REG(dev, SPI_CONF), SPI_CONF_TXCLR | SPI_CONF_RXCLR,
		       SPI_CONF_TXCLR | SPI_CONF_RXCLR);

	spi_cdns_spi_enable(dev, true);

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/*******************************************************************************
 * Public Functions Code
 ******************************************************************************/
/**
 * @brief Internal read/write the specified amount of data.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from, or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to, or NULL if none.
 * @param asynchronous Asynchronous flag
 * @param signal A pointer to a valid and ready to be signaled struct k_poll_signal.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument error.
 * @retval -ENOTSUP Unsupported value error.
 * @retval -EBUSY Waiting period timed out.
 * @retval -EIO Rx FIFO overflow.
 */
static int spi_cdns_transceive(const struct device *dev, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
			       bool asynchronous, spi_callback_t cb, void *userdata)
{
	const struct spi_cdns_cfg *dev_config = dev->config;
	struct spi_cdns_data *data = dev->data;
	uint32_t dfs;
	int ret;

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	pm_device_runtime_get(dev);
	pm_device_busy_set(dev);

	spi_cdns_spi_enable(dev, false);

	ret = spi_cdns_configure(dev, config);
	if (ret < 0) {
		spi_cdns_spi_enable(dev, true);
		goto out;
	}

	/* Disable interrupt */
	sys_write32(SPI_INT_DEFAULT, SPI_REG(dev, SPI_INT_DISABLE));
	/* Clear Pending Interrupts */
	(void)sys_read32(SPI_REG(dev, SPI_INT_STATUS));

	/* Reset semaphore for waiting for completion */
	k_sem_reset(&data->ctx.sync);

	/* TxFIFO and RxFIFO clear */
	sys_set_mask32(SPI_REG(dev, SPI_CONF), SPI_CONF_TXCLR | SPI_CONF_RXCLR,
		       SPI_CONF_TXCLR | SPI_CONF_RXCLR);
	spi_cdns_spi_enable(dev, true);

	data->fifo_diff = 0;

	dfs = SPI_WORD_SIZE_GET(data->ctx.config->operation) / 8;
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, dfs);

	data->tx_remain_entry =
		MAX(spi_context_total_rx_len(&data->ctx), spi_context_total_tx_len(&data->ctx));

	/* 0 byte transfer */
	if ((spi_context_total_rx_len(&data->ctx) == 0) &&
	    (spi_context_total_tx_len(&data->ctx) == 0)) {
		if (asynchronous) {
			spi_context_complete(&data->ctx, dev, 0);
		}
		goto out;
	}

	/* Set fifo thresholds */
	if (spi_context_is_slave(&data->ctx)) {
		sys_write32(1, SPI_REG(dev, SPI_RX_THRESHOLD));
		sys_write32(dev_config->tx_fifo_depth - 1, SPI_REG(dev, SPI_TX_THRESHOLD));
	} else {
		uint32_t fifo_words = MIN(DIV_ROUND_UP(spi_context_total_rx_len(&data->ctx),
						       (dev_config->fifo_width / 8)),
					  dev_config->rx_fifo_depth * 5 / 8);
		sys_write32(fifo_words, SPI_REG(dev, SPI_RX_THRESHOLD));
		sys_write32(dev_config->tx_fifo_depth / 2, SPI_REG(dev, SPI_TX_THRESHOLD));
	}

	if (spi_cs_is_gpio(data->ctx.config)) {
		spi_context_cs_control(&data->ctx, true);
	} else {
		spi_cdns_cs_control(dev, true);
	}

	sys_write32(SPI_INT_DEFAULT, SPI_REG(dev, SPI_INT_ENABLE));

	ret = spi_context_wait_for_completion(&data->ctx);

	if (!asynchronous) {
		if (spi_cs_is_gpio(data->ctx.config)) {
			spi_context_cs_control(&data->ctx, false);
		} else {
			spi_cdns_cs_control(dev, false);
		}
		pm_device_busy_clear(dev);
		pm_device_runtime_put(dev);
	}

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && !ret) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

out:
	spi_context_release(&data->ctx, ret);

	return ret;
}

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from, or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to, or NULL if none.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument error.
 * @retval -ENOTSUP Unsupported value error.
 * @retval -EBUSY Waiting period timed out.
 * @retval -EIO Rx FIFO overflow.
 */
static int spi_cdns_transceive_sync(const struct device *dev, const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return spi_cdns_transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from, or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to, or NULL if none.
 * @param async A pointer to a valid and ready to be signaled struct k_poll_signal.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument error.
 * @retval -ENOTSUP Unsupported value error.
 */
static int spi_cdns_transceive_async(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				     void *userdata)
{
	return spi_cdns_transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

/**
 * @brief Release the SPI device locked on by the current config
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 */
static int spi_cdns_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_cdns_data *data = dev->data;

	if (spi_cs_is_gpio(data->ctx.config)) {
		spi_context_cs_control(&data->ctx, false);
	} else {
		spi_cdns_cs_control(dev, false);
	}
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int spi_cdns_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct spi_cdns_cfg *cfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* TODO: Enable SPI Clock */
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
#endif
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* TODO: Disable SPI Clock */
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
#endif
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/**
 * SPI driver API registered in Zephyr spi framework
 */
static DEVICE_API(spi, spi_cdns_api) = {
	.transceive = spi_cdns_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_cdns_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_cdns_release,
};

/* Set clock-frequency-ext to pclk / 5 if there is no clock-frequency-ext */
#define SPI_CLOCK_FREQUENCY_EXT(n)                                                                 \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clock_frequency_ext), \
		(DT_INST_PROP(n, clock_frequency_ext)),            \
		(DT_INST_PROP(n, clock_frequency) / 5))

#define SPI_CDNS_INIT(n)                                                                           \
	static void spi_cdns_irq_config_##n(void);                                                 \
	static struct spi_cdns_data spi_cdns_data_##n = {                                          \
		SPI_CONTEXT_INIT_LOCK(spi_cdns_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_cdns_data_##n, ctx),                                     \
	};                                                                                         \
	static struct spi_cdns_cfg spi_cdns_cfg_##n = {                                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config = spi_cdns_irq_config_##n,                                             \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),                               \
		.ext_clock = SPI_CLOCK_FREQUENCY_EXT(n),                                           \
	};                                                                                         \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_cdns_init, spi_cdns_pm_action, &spi_cdns_data_##n,        \
				  &spi_cdns_cfg_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,        \
				  &spi_cdns_api);                                                  \
	static void spi_cdns_irq_config_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_cdns_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define DT_DRV_COMPAT cdns_spi
DT_INST_FOREACH_STATUS_OKAY(SPI_CDNS_INIT)
