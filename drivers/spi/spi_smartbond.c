/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_smartbond);

#include "spi_context.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/sys/byteorder.h>

#include <DA1469xAB.h>
#include <da1469x_pd.h>

#define DIVN_CLK	32000000	/* DIVN clock: fixed @32MHz */
#define SCLK_FREQ_2MHZ	(DIVN_CLK / 14) /* 2.285714 MHz*/
#define SCLK_FREQ_4MHZ	(DIVN_CLK / 8)	/* 4 MHz */
#define SCLK_FREQ_8MHZ	(DIVN_CLK / 4)	/* 8 MHz */
#define SCLK_FREQ_16MHZ (DIVN_CLK / 2)	/* 16 MHz */

enum spi_smartbond_transfer {
	SPI_SMARTBOND_TRANSFER_TX_ONLY,
	SPI_SMARTBOND_TRANSFER_RX_ONLY,
	SPI_SMARTBOND_TRANSFER_TX_RX,
	SPI_SMARTBOND_TRANSFER_NONE
};

enum spi_smartbond_dma_channel {
	SPI_SMARTBOND_DMA_TX_CHANNEL,
	SPI_SMARTBOND_DMA_RX_CHANNEL
};

enum spi_smartbond_fifo_mode {
	/* Bi-directional mode */
	SPI_SMARTBOND_FIFO_MODE_TX_RX,
	/* TX FIFO single depth, no flow control */
	SPI_SMARTBOND_FIFO_MODE_RX_ONLY,
	/* RX  FIFO single depth, no flow control */
	SPI_SMARTBOND_FIFO_MODE_TX_ONLY,
	SPI_SMARTBOND_FIFO_NONE
};

struct spi_smartbond_cfg {
	SPI_Type *regs;
	int periph_clock_config;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_SPI_SMARTBOND_DMA
	int tx_dma_chan;
	int rx_dma_chan;
	uint8_t tx_slot_mux;
	uint8_t rx_slot_mux;
	const struct device *tx_dma_ctrl;
	const struct device *rx_dma_ctrl;
#endif
};

struct spi_smartbond_data {
	struct spi_context ctx;
	uint8_t dfs;

#if defined(CONFIG_PM_DEVICE)
	uint32_t spi_ctrl_reg;
#endif

#ifdef CONFIG_SPI_SMARTBOND_DMA
	struct dma_config tx_dma_cfg;
	struct dma_config rx_dma_cfg;
	struct dma_block_config tx_dma_block_cfg;
	struct dma_block_config rx_dma_block_cfg;
	struct k_sem rx_dma_sync;
	struct k_sem tx_dma_sync;

	ATOMIC_DEFINE(dma_channel_atomic_flag, 2);

#endif

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_SMARTBOND_DMA)
	size_t rx_len;
	size_t tx_len;
	size_t transferred;
	enum spi_smartbond_transfer transfer_mode;
#endif
};

#define SPI_CTRL_REG_SET_FIELD(_field, _var, _val) \
	(_var) = \
	(((_var) & ~SPI_SPI_CTRL_REG_ ## _field ## _Msk) | \
	(((_val) << SPI_SPI_CTRL_REG_ ## _field ## _Pos) & SPI_SPI_CTRL_REG_ ## _field ## _Msk))

static inline void spi_smartbond_enable(const struct spi_smartbond_cfg *cfg, bool enable)
{
	if (enable) {
		cfg->regs->SPI_CTRL_REG |= SPI_SPI_CTRL_REG_SPI_ON_Msk;
		cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_RST_Msk;
	} else {
		cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_ON_Msk;
		cfg->regs->SPI_CTRL_REG |= SPI_SPI_CTRL_REG_SPI_RST_Msk;
	}
}

static inline bool spi_smartbond_isenabled(const struct spi_smartbond_cfg *cfg)
{
	return (!!(cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_ON_Msk)) &&
	       (!(cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_RST_Msk));
}

static inline void spi_smartbond_write_word(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	struct spi_smartbond_data *data = dev->data;

	/*
	 * No need to typecast the register address as the controller will automatically
	 * generate the necessary clock cycles based on the data size.
	 */
	switch (data->dfs) {
	case 1:
		cfg->regs->SPI_RX_TX_REG = *(uint8_t *)data->ctx.tx_buf;
		break;
	case 2:
		cfg->regs->SPI_RX_TX_REG = sys_get_le16(data->ctx.tx_buf);
		break;
	case 4:
		cfg->regs->SPI_RX_TX_REG = sys_get_le32(data->ctx.tx_buf);
		break;
	}
}

static inline void spi_smartbond_write_dummy(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	cfg->regs->SPI_RX_TX_REG = 0x0;
}

static inline void spi_smartbond_read_word(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	struct spi_smartbond_data *data = dev->data;

	switch (data->dfs) {
	case 1:
		*(uint8_t *)data->ctx.rx_buf = cfg->regs->SPI_RX_TX_REG;
		break;
	case 2:
		sys_put_le16((uint16_t)cfg->regs->SPI_RX_TX_REG, data->ctx.rx_buf);
		break;
	case 4:
		sys_put_le32(cfg->regs->SPI_RX_TX_REG, data->ctx.rx_buf);
		break;
	}
}

static inline void spi_smartbond_read_discard(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	(void)cfg->regs->SPI_RX_TX_REG;
}

static inline int spi_smartbond_set_speed(const struct spi_smartbond_cfg *cfg,
					  const uint32_t frequency)
{
	if (frequency < SCLK_FREQ_2MHZ) {
		LOG_ERR("Frequency is lower than minimal SCLK %d", SCLK_FREQ_2MHZ);
		return -ENOTSUP;
	} else if (frequency < SCLK_FREQ_4MHZ) {
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk) |
			3UL << SPI_SPI_CTRL_REG_SPI_CLK_Pos;
	} else if (frequency < SCLK_FREQ_8MHZ) {
		cfg->regs->SPI_CTRL_REG = (cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk);
	} else if (frequency < SCLK_FREQ_16MHZ) {
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk) |
			1UL << SPI_SPI_CTRL_REG_SPI_CLK_Pos;
	} else {
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_CLK_Msk) |
			2UL << SPI_SPI_CTRL_REG_SPI_CLK_Pos;
	}
	return 0;
}

static inline int spi_smartbond_set_word_size(const struct spi_smartbond_cfg *cfg,
					      struct spi_smartbond_data *data,
					      const uint32_t operation)
{
	switch (SPI_WORD_SIZE_GET(operation)) {
	case 8:
		data->dfs = 1;
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_WORD_Msk);
		break;
	case 16:
		data->dfs = 2;
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_WORD_Msk) |
			(1UL << SPI_SPI_CTRL_REG_SPI_WORD_Pos);
		break;
	case 32:
		data->dfs = 4;
		cfg->regs->SPI_CTRL_REG =
			(cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_WORD_Msk) |
			(2UL << SPI_SPI_CTRL_REG_SPI_WORD_Pos);
		break;
	default:
		LOG_ERR("Word size not supported");
		return -ENOTSUP;
	}

	return 0;
}

static inline void spi_smartbond_pm_policy_state_lock_get(const struct device *dev)
{
#if defined(CONFIG_PM_DEVICE)
	/*
	 * Prevent the SoC from entering the normal sleep state as PDC does not support
	 * waking up the application core following SPI events.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_device_runtime_get(dev);
#endif
}

static inline void spi_smartbond_pm_policy_state_lock_put(const struct device *dev)
{
#if defined(CONFIG_PM_DEVICE)
	pm_device_runtime_put(dev);
	/*
	 * Allow the SoC to enter the normal sleep state once SPI transactions are done.
	 */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static int spi_smartbond_configure(const struct spi_smartbond_cfg *cfg,
				   struct spi_smartbond_data *data,
				   const struct spi_config *spi_cfg)
{
	int rc;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
#ifdef CONFIG_PM_DEVICE
		spi_smartbond_enable(cfg, true);
#endif
		return 0;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not yet supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	if (spi_smartbond_isenabled(cfg)) {
		spi_smartbond_enable(cfg, false);
	}

	rc = spi_smartbond_set_speed(cfg, spi_cfg->frequency);
	if (rc) {
		return rc;
	}

	cfg->regs->SPI_CTRL_REG =
		(spi_cfg->operation & SPI_MODE_CPOL)
			? (cfg->regs->SPI_CTRL_REG | SPI_SPI_CTRL_REG_SPI_POL_Msk)
			: (cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_POL_Msk);

	cfg->regs->SPI_CTRL_REG =
		(spi_cfg->operation & SPI_MODE_CPHA)
			? (cfg->regs->SPI_CTRL_REG | SPI_SPI_CTRL_REG_SPI_PHA_Msk)
			: (cfg->regs->SPI_CTRL_REG & ~SPI_SPI_CTRL_REG_SPI_PHA_Msk);

	rc = spi_smartbond_set_word_size(cfg, data, spi_cfg->operation);
	if (rc) {
		return rc;
	}

	cfg->regs->SPI_CTRL_REG &= ~(SPI_SPI_CTRL_REG_SPI_FIFO_MODE_Msk);

	spi_smartbond_enable(cfg, true);

	cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_MINT_Msk;

	data->ctx.config = spi_cfg;

	return 0;
}

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_SMARTBOND_DMA)
static inline void spi_smartbond_isr_set_status(const struct device *dev, bool status)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	if (status) {
		cfg->regs->SPI_CTRL_REG |= SPI_SPI_CTRL_REG_SPI_MINT_Msk;
	} else {
		cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_MINT_Msk;
	}
}

static inline bool spi_smartbond_is_busy(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	return (cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_BUSY_Msk);
}

static inline void spi_smartbond_clear_interrupt(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	cfg->regs->SPI_CLEAR_INT_REG = 0x1;
}

/* 0 = No RX data available, 1 = data has been transmitted and received */
static inline bool spi_smartbond_is_rx_data(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	return (cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_INT_BIT_Msk);
}

static inline uint8_t spi_smartbond_get_fifo_mode(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	return ((cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_FIFO_MODE_Msk) >>
			SPI_SPI_CTRL_REG_SPI_FIFO_MODE_Pos);
}

static void spi_smartbond_set_fifo_mode(const struct device *dev, enum spi_smartbond_fifo_mode mode)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	bool is_enabled = spi_smartbond_isenabled(cfg);
	enum spi_smartbond_fifo_mode current_mode = spi_smartbond_get_fifo_mode(dev);
	uint32_t spi_ctrl_reg = cfg->regs->SPI_CTRL_REG;

#ifdef CONFIG_SPI_SMARTBOND_DMA
	struct spi_smartbond_data *data = dev->data;
#endif

	if ((current_mode != mode)
#ifdef CONFIG_SPI_SMARTBOND_DMA
		|| (data->dfs == 4)
#endif
		) {
		if (current_mode != SPI_SMARTBOND_FIFO_MODE_RX_ONLY) {
			while (spi_smartbond_is_busy(dev)) {
				;
			}
		}
		/* Controller should be disabled when FIFO mode is updated */
		cfg->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_ON_Msk;

#ifdef CONFIG_SPI_SMARTBOND_DMA
		/*
		 * Workaround for the controller that cannot generate DMA requests
		 * for 4-byte bus length.
		 */
		if (data->dfs == 4) {
			mode = SPI_SMARTBOND_FIFO_NONE;
		}
#endif
		SPI_CTRL_REG_SET_FIELD(SPI_FIFO_MODE, spi_ctrl_reg, mode);


		if (mode != SPI_SMARTBOND_FIFO_NONE) {
			SPI_CTRL_REG_SET_FIELD(SPI_DMA_TXREQ_MODE, spi_ctrl_reg, 0);
		} else {
			SPI_CTRL_REG_SET_FIELD(SPI_DMA_TXREQ_MODE, spi_ctrl_reg, 1);
		}

		if (is_enabled) {
			SPI_CTRL_REG_SET_FIELD(SPI_ON, spi_ctrl_reg, 1);
		}

		cfg->regs->SPI_CTRL_REG = spi_ctrl_reg;
	}
}

static int spi_smartbond_transfer_mode_get(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (spi_context_rx_buf_on(ctx) || spi_context_tx_buf_on(ctx)) {
		/*
		 * Check only buffers' length as it might happen that current buffer is NULL.
		 * In such a case the context should be updated and a dummy write/read should
		 * take place.
		 */
		if (ctx->rx_len || ctx->tx_len) {
			spi_smartbond_set_fifo_mode(dev, SPI_SMARTBOND_FIFO_MODE_TX_RX);
			return SPI_SMARTBOND_TRANSFER_TX_RX;
		}

		if (!spi_context_rx_buf_on(ctx)) {
			spi_smartbond_set_fifo_mode(dev, SPI_SMARTBOND_FIFO_MODE_TX_ONLY);
			return SPI_SMARTBOND_TRANSFER_TX_ONLY;
		}

		if (!spi_context_tx_buf_on(ctx)) {
			/*
			 * Use the TX/RX mode with TX being dummy. Using the RX only mode
			 * is a bit tricky as the controller should generate clock cycles
			 * automatically and immediately after the ISR is enabled.
			 */
			spi_smartbond_set_fifo_mode(dev, SPI_SMARTBOND_FIFO_MODE_TX_RX);
			return SPI_SMARTBOND_TRANSFER_RX_ONLY;
		}
	}

	/* Return waiting updating the fifo mode */
	return SPI_SMARTBOND_TRANSFER_NONE;
}

static inline void spi_smartbond_transfer_mode_check_and_update(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;

	data->transfer_mode = spi_smartbond_transfer_mode_get(dev);
}
#endif

#ifdef CONFIG_SPI_ASYNC
static inline bool spi_smartbond_is_tx_full(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	return (cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_TXH_Msk);
}

static void spi_smartbond_write(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	while (spi_context_tx_buf_on(ctx)) {
		/* Check if TX FIFO is full as otherwise undefined data should be transmitted. */
		if (spi_smartbond_is_tx_full(dev)) {
			spi_smartbond_clear_interrupt(dev);
			break;
		}
		/* Send to TX FIFO and update buffer pointer. */
		spi_smartbond_write_word(dev);
		spi_context_update_tx(ctx, data->dfs, 1);

		/*
		 * It might happen that a NULL buffer with a non-zero length is provided.
		 * In that case, the bytes should be consumed.
		 */
		if (ctx->rx_len && !ctx->rx_buf) {
			spi_smartbond_read_discard(dev);
			spi_context_update_rx(ctx, data->dfs, 1);
		}
	}
}

static void spi_smartbond_transfer(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	while (data->rx_len) {
		/* Zero means that RX FIFO or register is empty */
		if (!spi_smartbond_is_rx_data(dev)) {
			break;
		}

		if (ctx->rx_buf) {
			spi_smartbond_read_word(dev);
		} else {
			spi_smartbond_read_discard(dev);
		}
		spi_context_update_rx(ctx, data->dfs, 1);

		spi_smartbond_clear_interrupt(dev);

		data->rx_len--;
		data->transferred++;
	}

	while (data->tx_len) {
		/* Check if TX FIFO is full as otherwise undefined data should be transmitted. */
		if (spi_smartbond_is_tx_full(dev)) {
			break;
		}

		if (ctx->tx_buf) {
			spi_smartbond_write_word(dev);
		} else {
			spi_smartbond_write_dummy(dev);
		}
		spi_context_update_tx(ctx, data->dfs, 1);

		data->tx_len--;
	}
}

static void spi_smartbond_read(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	while (spi_context_rx_buf_on(ctx)) {
		/* Zero means that RX FIFO or register is empty */
		if (!spi_smartbond_is_rx_data(dev)) {
			break;
		}

		spi_smartbond_read_word(dev);
		spi_context_update_rx(ctx, data->dfs, 1);
		spi_smartbond_clear_interrupt(dev);
	}

	/* Perform dummy access to generate the required clock cycles */
	while (data->tx_len) {
		if (spi_smartbond_is_tx_full(dev)) {
			break;
		}
		spi_smartbond_write_dummy(dev);

		data->tx_len--;
	}
}

static void spi_smartbond_isr_trigger(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	data->transfer_mode = spi_smartbond_transfer_mode_get(dev);

	switch (data->transfer_mode) {
	case SPI_SMARTBOND_TRANSFER_RX_ONLY:
		data->tx_len = spi_context_total_rx_len(ctx);
		spi_smartbond_read(dev);
		break;
	case SPI_SMARTBOND_TRANSFER_TX_ONLY:
		spi_smartbond_write(dev);
		break;
	case SPI_SMARTBOND_TRANSFER_TX_RX:
		/*
		 * Each sub-transfer in the descriptor list should be exercised
		 * separately as it might happen that a buffer is NULL with
		 * non-zero length.
		 */
		data->rx_len = spi_context_max_continuous_chunk(ctx);
		data->tx_len = data->rx_len;
		spi_smartbond_transfer(dev);
		break;
	case SPI_SMARTBOND_TRANSFER_NONE:
		__fallthrough;
	default:
		__ASSERT_MSG_INFO("Invalid transfer mode");
		break;
	}

	spi_smartbond_isr_set_status(dev, true);
}

static int spi_smartbond_transceive_async(const struct device *dev,
					  const struct spi_config *spi_cfg,
					  const struct spi_buf_set *tx_bufs,
					  const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					  void *userdata)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int rc;

	spi_context_lock(ctx, true, cb, userdata, spi_cfg);
	spi_smartbond_pm_policy_state_lock_get(dev);

	rc = spi_smartbond_configure(cfg, data, spi_cfg);
	if (rc == 0) {
		spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);
		spi_context_cs_control(ctx, true);

		/*
		 * PM constraints will be released within ISR once all transfers
		 * are exercised along with de-asserting the #CS line.
		 */
		spi_smartbond_isr_trigger(dev);
	}
	/*
	 * Context will actually be released when \sa spi_context_complete
	 * is called.
	 */
	spi_context_release(ctx, rc);

	return rc;
}
#endif

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_SMARTBOND_DMA)
static void spi_smartbond_isr(void *args)
{
#ifdef CONFIG_SPI_ASYNC
	struct device *dev = args;
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	switch (data->transfer_mode) {
	case SPI_SMARTBOND_TRANSFER_RX_ONLY:
		spi_smartbond_read(dev);
		break;
	case SPI_SMARTBOND_TRANSFER_TX_ONLY:
		spi_smartbond_write(dev);
		break;
	case SPI_SMARTBOND_TRANSFER_TX_RX:
		/* Exersice the type of the next sub-transfer */
		if (!data->rx_len && !data->tx_len) {
			spi_smartbond_transfer_mode_check_and_update(dev);

			if (data->transfer_mode == SPI_SMARTBOND_TRANSFER_RX_ONLY) {
				data->tx_len = spi_context_total_rx_len(ctx) - data->transferred;
				/* Clear in case another truncated transfer should be executed */
				data->transferred = 0;
				spi_smartbond_read(dev);
			} else if (data->transfer_mode == SPI_SMARTBOND_TRANSFER_TX_ONLY) {
				spi_smartbond_write(dev);
			} else if (data->transfer_mode == SPI_SMARTBOND_TRANSFER_TX_RX) {
				data->rx_len = spi_context_max_continuous_chunk(ctx);
				data->tx_len = data->rx_len;
				spi_smartbond_transfer(dev);
			}
		} else {
			spi_smartbond_transfer(dev);
		}
		break;
	case SPI_SMARTBOND_TRANSFER_NONE:
		__fallthrough;
	default:
		__ASSERT_MSG_INFO("Invalid transfer mode");
		break;
	}

	/* All buffers have been exercised, signal completion */
	if (!spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx)) {
		spi_smartbond_isr_set_status(dev, false);

		/* Mark completion to trigger callback function */
		spi_context_complete(ctx, dev, 0);

		spi_context_cs_control(ctx, false);
		spi_smartbond_pm_policy_state_lock_put(data);
	}
#endif
}
#endif

#ifdef CONFIG_SPI_SMARTBOND_DMA
static uint32_t spi_smartbond_read_dummy_buf;

/*
 * Should be used to flush the RX FIFO in case a transaction is requested
 * with NULL pointer and non-zero length. In such a case, data will be
 * shifted into the RX FIFO (regardless of whether or not the RX mode is
 * disabled) which should then be flushed. Otherwise, a next read operation
 * will result in fetching old bytes.
 */
static void spi_smartbond_flush_rx_fifo(const struct device *dev)
{
	while (spi_smartbond_is_busy(dev)) {
	};
	while (spi_smartbond_is_rx_data(dev)) {
		spi_smartbond_read_discard(dev);
		spi_smartbond_clear_interrupt(dev);
	}
}

static int spi_smartbond_dma_tx_channel_request(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	const struct spi_smartbond_cfg *config = dev->config;

	if (!atomic_test_and_set_bit(data->dma_channel_atomic_flag,
		SPI_SMARTBOND_DMA_TX_CHANNEL)) {
		if (dma_request_channel(config->tx_dma_ctrl, (void *)&config->tx_dma_chan) < 0) {
			atomic_clear_bit(data->dma_channel_atomic_flag,
				SPI_SMARTBOND_DMA_TX_CHANNEL);
			return -EIO;
		}
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void spi_smartbond_dma_tx_channel_release(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	const struct spi_smartbond_cfg *config = dev->config;

	if (atomic_test_and_clear_bit(data->dma_channel_atomic_flag,
		SPI_SMARTBOND_DMA_TX_CHANNEL)) {
		dma_release_channel(config->tx_dma_ctrl, config->tx_dma_chan);
	}
}
#endif

static int spi_smartbond_dma_rx_channel_request(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	const struct spi_smartbond_cfg *config = dev->config;

	if (!atomic_test_and_set_bit(data->dma_channel_atomic_flag,
		SPI_SMARTBOND_DMA_RX_CHANNEL)) {
		if (dma_request_channel(config->rx_dma_ctrl, (void *)&config->rx_dma_chan) < 0) {
			atomic_clear_bit(data->dma_channel_atomic_flag,
				SPI_SMARTBOND_DMA_RX_CHANNEL);
			return -EIO;
		}
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void spi_smartbond_dma_rx_channel_release(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	const struct spi_smartbond_cfg *config = dev->config;

	if (atomic_test_and_clear_bit(data->dma_channel_atomic_flag,
		SPI_SMARTBOND_DMA_RX_CHANNEL)) {
		dma_release_channel(config->rx_dma_ctrl, config->rx_dma_chan);
	}
}
#endif

static void spi_smartbond_tx_dma_cb(const struct device *dma, void *arg,
		uint32_t id, int status)
{
	const struct device *dev = arg;
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (status < 0) {
		LOG_WRN("DMA transfer did not complete");
	}

	spi_context_update_tx(ctx, data->dfs, data->tx_len);
	k_sem_give(&data->tx_dma_sync);
}

static void spi_smartbond_rx_dma_cb(const struct device *dma, void *arg,
		uint32_t id, int status)
{
	const struct device *dev = arg;
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (status < 0) {
		LOG_WRN("DMA transfer did not complete");
	}

	spi_context_update_rx(ctx, data->dfs, data->rx_len);
	k_sem_give(&data->rx_dma_sync);
}

#ifdef CONFIG_PM_DEVICE
static void spi_smartbond_dma_deconfig(const struct device *dev)
{
	const struct spi_smartbond_cfg *config = dev->config;

	if (config->rx_dma_ctrl && config->tx_dma_ctrl) {
		dma_stop(config->rx_dma_ctrl, config->rx_dma_chan);
		dma_stop(config->tx_dma_ctrl, config->tx_dma_chan);

		spi_smartbond_dma_rx_channel_release(dev);
		spi_smartbond_dma_tx_channel_release(dev);
	}
}
#endif

static int spi_smartbond_dma_config(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	const struct spi_smartbond_cfg *config = dev->config;
	struct dma_config *tx = &data->tx_dma_cfg;
	struct dma_config *rx = &data->rx_dma_cfg;
	struct dma_block_config *tx_block = &data->tx_dma_block_cfg;
	struct dma_block_config *rx_block = &data->rx_dma_block_cfg;

	/*
	 * DMA RX should be assigned an even number and
	 * DMA TX should be assigned the right next
	 * channel (odd number).
	 */
	if (!(config->tx_dma_chan & 0x1) ||
			(config->rx_dma_chan & 0x1) ||
			(config->tx_dma_chan != (config->rx_dma_chan + 1))) {
		LOG_ERR("Invalid RX/TX channel selection");
		return -EINVAL;
	}

	if (config->tx_slot_mux != config->rx_slot_mux) {
		LOG_ERR("TX/RX DMA slots mismatch");
		return -EINVAL;
	}

	if (!device_is_ready(config->tx_dma_ctrl) ||
		!device_is_ready(config->rx_dma_ctrl)) {
		LOG_ERR("TX/RX DMA device is not ready");
		return -ENODEV;
	}

	if (spi_smartbond_dma_tx_channel_request(dev) < 0) {
		LOG_ERR("TX DMA channel is already occupied");
		return -EIO;
	}

	if (spi_smartbond_dma_rx_channel_request(dev) < 0) {
		LOG_ERR("RX DMA channel is already occupied");
		return -EIO;
	}

	tx->channel_direction = MEMORY_TO_PERIPHERAL;
	tx->dma_callback = spi_smartbond_tx_dma_cb;
	tx->user_data = (void *)dev;
	tx->block_count = 1;
	tx->head_block = &data->tx_dma_block_cfg;
	tx->error_callback_dis = 1;
	tx->dma_slot = config->tx_slot_mux;
	tx->channel_priority = 2;

	/* Burst mode is not using when DREQ is one */
	tx->source_burst_length = 1;
	tx->dest_burst_length = 1;
	/* Source and destination data size should reflect DFS value */
	tx->source_data_size = 0;
	tx->dest_data_size = 0;

	/* Do not change */
	tx_block->dest_addr_adj = 0x2;
	/* Incremental */
	tx_block->source_addr_adj = 0x0;
	tx_block->dest_address = (uint32_t)&config->regs->SPI_RX_TX_REG;

	/*
	 * To be filled when a transaction is requested and
	 * should reflect the total number of bytes.
	 */
	tx_block->block_size = 0;
	/* Should reflect the TX buffer */
	tx_block->source_address = 0;

	rx->channel_direction = PERIPHERAL_TO_MEMORY;
	rx->dma_callback = spi_smartbond_rx_dma_cb;
	rx->user_data = (void *)dev;
	rx->block_count = 1;
	rx->head_block = &data->rx_dma_block_cfg;
	rx->error_callback_dis = 1;
	rx->dma_slot = config->rx_slot_mux;
	rx->channel_priority = 2;

	/* Burst mode is not using when DREQ is one */
	rx->source_burst_length = 1;
	rx->dest_burst_length = 1;
	/* Source and destination data size should reflect DFS value */
	rx->source_data_size = 0;
	rx->dest_data_size = 0;

	/* Do not change */
	rx_block->source_addr_adj = 0x2;
	/* Incremenetal */
	rx_block->dest_addr_adj = 0x0;
	rx_block->source_address = (uint32_t)&config->regs->SPI_RX_TX_REG;

	/*
	 * To be filled when a transaction is requested and
	 * should reflect the total number of bytes.
	 */
	rx_block->block_size = 0;
	/* Should reflect the RX buffer */
	rx_block->dest_address = 0;

	return 0;
}

static int spi_smartbond_dma_trigger(const struct device *dev)
{
	struct spi_smartbond_data *data = dev->data;
	const struct spi_smartbond_cfg *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	struct dma_config *tx = &data->tx_dma_cfg;
	struct dma_config *rx = &data->rx_dma_cfg;
	struct dma_block_config *tx_block = &data->tx_dma_block_cfg;
	struct dma_block_config *rx_block = &data->rx_dma_block_cfg;

	rx->source_data_size = data->dfs;
	rx->dest_data_size = data->dfs;
	tx->source_data_size = data->dfs;
	tx->dest_data_size = data->dfs;

	data->transfer_mode = spi_smartbond_transfer_mode_get(dev);
	do {
		switch (data->transfer_mode) {
		case SPI_SMARTBOND_TRANSFER_RX_ONLY:
			spi_smartbond_flush_rx_fifo(dev);

			data->rx_len = spi_context_max_continuous_chunk(ctx);
			data->tx_len = data->rx_len;

			rx_block->block_size = data->rx_len * data->dfs;
			tx_block->block_size = rx_block->block_size;

			rx_block->dest_address = (uint32_t)ctx->rx_buf;
			rx_block->dest_addr_adj = 0x0;
			tx_block->source_address = (uint32_t)&spi_smartbond_read_dummy_buf;
			/* Non-incremental */
			tx_block->source_addr_adj = 0x2;

			if (dma_config(config->tx_dma_ctrl, config->tx_dma_chan, tx) < 0) {
				LOG_ERR("TX DMA configuration failed");
				return -EINVAL;
			}
			if (dma_config(config->rx_dma_ctrl, config->rx_dma_chan, rx) < 0) {
				LOG_ERR("RX DMA configuration failed");
				return -EINVAL;
			}
			dma_start(config->rx_dma_ctrl, config->rx_dma_chan);
			dma_start(config->tx_dma_ctrl, config->tx_dma_chan);

			/* Wait for the current DMA transfer to complete */
			k_sem_take(&data->tx_dma_sync, K_FOREVER);
			k_sem_take(&data->rx_dma_sync, K_FOREVER);
			break;
		case SPI_SMARTBOND_TRANSFER_TX_ONLY:
			spi_smartbond_flush_rx_fifo(dev);

			data->tx_len = spi_context_max_continuous_chunk(ctx);
			data->rx_len = data->tx_len;

			tx_block->block_size = data->tx_len * data->dfs;
			tx_block->source_address = (uint32_t)ctx->tx_buf;
			tx_block->source_addr_adj = 0x0;

			if (dma_config(config->tx_dma_ctrl, config->tx_dma_chan, tx) < 0) {
				LOG_ERR("TX DMA configuration failed");
				return -EINVAL;
			}
			dma_start(config->tx_dma_ctrl, config->tx_dma_chan);

			/* Wait for the current DMA transfer to complete */
			k_sem_take(&data->tx_dma_sync, K_FOREVER);
			break;
		case SPI_SMARTBOND_TRANSFER_TX_RX:
			spi_smartbond_flush_rx_fifo(dev);

			data->rx_len = spi_context_max_continuous_chunk(ctx);
			data->tx_len = data->rx_len;
			/*
			 * DMA block size represents total number of bytes whilist,
			 * context length is divided by the data size (dfs).
			 */
			tx_block->block_size = data->tx_len * data->dfs;
			rx_block->block_size = tx_block->block_size;

			if (ctx->tx_buf) {
				tx_block->source_address = (uint32_t)ctx->tx_buf;
				tx_block->source_addr_adj = 0x0;
			} else {
				tx_block->source_address = (uint32_t)&spi_smartbond_read_dummy_buf;
				tx_block->source_addr_adj = 0x2;
			}

			if (ctx->rx_buf) {
				rx_block->dest_address = (uint32_t)ctx->rx_buf;
				rx_block->dest_addr_adj = 0x0;
			} else {
				rx_block->dest_address = (uint32_t)&spi_smartbond_read_dummy_buf;
				rx_block->dest_addr_adj = 0x2;
			}

			if (dma_config(config->tx_dma_ctrl, config->tx_dma_chan, tx) < 0) {
				LOG_ERR("TX DMA configuration failed");
				return -EINVAL;
			}
			if (dma_config(config->rx_dma_ctrl, config->rx_dma_chan, rx) < 0) {
				LOG_ERR("RX DMA configuration failed");
				return -EINVAL;
			}
			dma_start(config->rx_dma_ctrl, config->rx_dma_chan);
			dma_start(config->tx_dma_ctrl, config->tx_dma_chan);

			k_sem_take(&data->tx_dma_sync, K_FOREVER);
			k_sem_take(&data->rx_dma_sync, K_FOREVER);

			/*
			 * Regardless of whether or not the RX FIFO is enabled, received
			 * bytes are pushed into it. As such, the RXI FIFO should be
			 * flushed so that a next read access retrives the correct bytes
			 * and not old ones.
			 */
			if (!ctx->rx_buf) {
				spi_smartbond_flush_rx_fifo(dev);
			}
			break;
		case SPI_SMARTBOND_TRANSFER_NONE:
			__fallthrough;
		default:
			__ASSERT_MSG_INFO("Invalid transfer mode");
			break;
		}

		spi_smartbond_transfer_mode_check_and_update(dev);
	} while (data->transfer_mode != SPI_SMARTBOND_TRANSFER_NONE);

	return 0;
}
#endif

static int spi_smartbond_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int rc;

	spi_context_lock(&data->ctx, false, NULL, NULL, spi_cfg);
	spi_smartbond_pm_policy_state_lock_get(dev);

	rc = spi_smartbond_configure(cfg, data, spi_cfg);
	if (rc == 0) {
		spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);
		spi_context_cs_control(ctx, true);

#ifdef CONFIG_SPI_SMARTBOND_DMA
		rc = spi_smartbond_dma_trigger(dev);
		/* Mark completion to trigger callback function */
		spi_context_complete(ctx, dev, 0);
#else
		while (spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx)) {
			if (spi_context_tx_buf_on(ctx)) {
				spi_smartbond_write_word(dev);
				spi_context_update_tx(ctx, data->dfs, 1);
			} else {
				spi_smartbond_write_dummy(dev);
			}

			while (!(cfg->regs->SPI_CTRL_REG & SPI_SPI_CTRL_REG_SPI_INT_BIT_Msk)) {
			};
			if (spi_context_rx_buf_on(ctx)) {
				spi_smartbond_read_word(dev);
				spi_context_update_rx(ctx, data->dfs, 1);
			} else {
				spi_smartbond_read_discard(dev);
				/*
				 * It might happen that a NULL buffer with a non-zero length
				 * is provided. In that case, the bytes should be consumed.
				 */
				if (ctx->rx_len) {
					spi_context_update_rx(ctx, data->dfs, 1);
				}
			}
			cfg->regs->SPI_CLEAR_INT_REG = 1UL;
		}
#endif

		spi_context_cs_control(ctx, false);
	}
	spi_context_release(&data->ctx, rc);

	spi_smartbond_pm_policy_state_lock_put(dev);

	return rc;
}

static int spi_smartbond_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_smartbond_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, spi_cfg)) {
		LOG_ERR("SPI configuration was not the last one to be used");
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static DEVICE_API(spi, spi_smartbond_driver_api) = {
	.transceive = spi_smartbond_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_smartbond_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_smartbond_release,
};

static int spi_smartbond_resume(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;
	struct spi_smartbond_data *data = dev->data;
	int rc;

	CRG_COM->RESET_CLK_COM_REG = cfg->periph_clock_config << 1;
	CRG_COM->SET_CLK_COM_REG = cfg->periph_clock_config;

	rc = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("Failed to configure SPI pins");
		return rc;
	}

	rc = spi_context_cs_configure_all(&data->ctx);
	if (rc < 0) {
		LOG_ERR("Failed to configure CS pins: %d", rc);
		return rc;
	}

#ifdef CONFIG_SPI_SMARTBOND_DMA
	rc = spi_smartbond_dma_config(dev);
	if (rc < 0) {
		LOG_ERR("Failed to configure DMA");
		return rc;
	}
#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int spi_smartbond_suspend(const struct device *dev)
{
	int ret;
	const struct spi_smartbond_cfg *config = dev->config;
	struct spi_smartbond_data *data = dev->data;

	data->spi_ctrl_reg = config->regs->SPI_CTRL_REG;
	/* Disable the SPI digital block */
	config->regs->SPI_CTRL_REG &= ~SPI_SPI_CTRL_REG_SPI_EN_CTRL_Msk;
	/* Gate SPI clocking */
	CRG_COM->RESET_CLK_COM_REG = config->periph_clock_config;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		LOG_WRN("Failed to configure the SPI pins to inactive state");
	}

#ifdef CONFIG_SPI_SMARTBOND_DMA
	spi_smartbond_dma_deconfig(dev);
#endif

	return ret;
}

static int spi_smartbond_pm_action(const struct device *dev,
				   enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		da1469x_pd_acquire(MCU_PD_DOMAIN_COM);
		ret = spi_smartbond_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = spi_smartbond_suspend(dev);
		da1469x_pd_release(MCU_PD_DOMAIN_COM);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif

#define SPI_SMARTBOND_ISR_CONNECT \
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(spi)), DT_IRQ(DT_NODELABEL(spi), priority), \
			spi_smartbond_isr, DEVICE_DT_GET(DT_NODELABEL(spi)), 0); \
		irq_enable(DT_IRQN(DT_NODELABEL(spi)));

#define SPI2_SMARTBOND_ISR_CONNECT \
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(spi2)), DT_IRQ(DT_NODELABEL(spi2), priority), \
			spi_smartbond_isr, DEVICE_DT_GET(DT_NODELABEL(spi2)), 0); \
		irq_enable(DT_IRQN(DT_NODELABEL(spi2)));

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_SMARTBOND_DMA)
static int spi_smartbond_isr_connect(const struct device *dev)
{
	const struct spi_smartbond_cfg *cfg = dev->config;

	switch ((uint32_t)cfg->regs) {
	case (uint32_t)SPI:
		COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(spi)),
			(SPI_SMARTBOND_ISR_CONNECT), (NULL));
		break;
	case (uint32_t)SPI2:
		COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(spi2)),
			(SPI2_SMARTBOND_ISR_CONNECT), (NULL));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static int spi_smartbond_init(const struct device *dev)
{
	int ret;
	struct spi_smartbond_data *data = dev->data;

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_SMARTBOND_DMA)
	data->transfer_mode = SPI_SMARTBOND_TRANSFER_NONE;
#endif
#ifdef CONFIG_SPI_SMARTBOND_DMA
	k_sem_init(&data->tx_dma_sync, 0, 1);
	k_sem_init(&data->rx_dma_sync, 0, 1);
#endif

#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Make sure device state is marked as suspended */
	pm_device_init_suspended(dev);

	ret = pm_device_runtime_enable(dev);

#else
	da1469x_pd_acquire(MCU_PD_DOMAIN_COM);
	ret = spi_smartbond_resume(dev);
#endif
	spi_context_unlock_unconditionally(&data->ctx);

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_SMARTBOND_DMA)
	ret = spi_smartbond_isr_connect(dev);
#endif

	return ret;
}

#ifdef CONFIG_SPI_SMARTBOND_DMA
#define SPI_SMARTBOND_DMA_TX_INIT(id) \
	.tx_dma_chan = DT_INST_DMAS_CELL_BY_NAME(id, tx, channel), \
	.tx_slot_mux = (uint8_t)DT_INST_DMAS_CELL_BY_NAME(id, tx, config), \
	.tx_dma_ctrl = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, tx)),
#else
#define SPI_SMARTBOND_DMA_TX_INIT(id)
#endif

#ifdef CONFIG_SPI_SMARTBOND_DMA
#define SPI_SMARTBOND_DMA_RX_INIT(id) \
	.rx_dma_chan = DT_INST_DMAS_CELL_BY_NAME(id, rx, channel),	\
	.rx_slot_mux = (uint8_t)DT_INST_DMAS_CELL_BY_NAME(id, rx, config),	\
	.rx_dma_ctrl = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, rx)),
#else
#define SPI_SMARTBOND_DMA_RX_INIT(id)
#endif

#ifdef CONFIG_SPI_SMARTBOND_DMA
#define SPI_SMARTBOND_DMA_TX_INVALIDATE(id) \
	.tx_dma_chan = 255, \
	.tx_slot_mux = 255, \
	.tx_dma_ctrl = NULL,
#else
#define SPI_SMARTBOND_DMA_TX_INVALIDATE(id)
#endif

#ifdef CONFIG_SPI_SMARTBOND_DMA
#define SPI_SMARTBOND_DMA_RX_INVALIDATE(id) \
	.rx_dma_chan = 255, \
	.rx_slot_mux = 255, \
	.rx_dma_ctrl = NULL,
#else
#define SPI_SMARTBOND_DMA_RX_INVALIDATE(id)
#endif

#define SPI_SMARTBOND_DEVICE(id)                                                                   \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static const struct spi_smartbond_cfg spi_smartbond_##id##_cfg = {                         \
		.regs = (SPI_Type *)DT_INST_REG_ADDR(id),                                          \
		.periph_clock_config = DT_INST_PROP(id, periph_clock_config),                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                        \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(id, tx),	\
			(SPI_SMARTBOND_DMA_TX_INIT(id)),	\
			(SPI_SMARTBOND_DMA_TX_INVALIDATE(id))) \
		COND_CODE_1(DT_INST_DMAS_HAS_NAME(id, rx), \
			(SPI_SMARTBOND_DMA_RX_INIT(id)),	\
			(SPI_SMARTBOND_DMA_RX_INVALIDATE(id)))	\
	};                                                                                         \
	static struct spi_smartbond_data spi_smartbond_##id##_data = {                             \
		SPI_CONTEXT_INIT_LOCK(spi_smartbond_##id##_data, ctx),                             \
		SPI_CONTEXT_INIT_SYNC(spi_smartbond_##id##_data, ctx),                             \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(id), ctx)};                            \
	PM_DEVICE_DT_INST_DEFINE(id, spi_smartbond_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(id,                                                                  \
			      spi_smartbond_init,                                                  \
			      PM_DEVICE_DT_INST_GET(id),                                           \
			      &spi_smartbond_##id##_data,                                          \
			      &spi_smartbond_##id##_cfg,                                           \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,                               \
			      &spi_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SMARTBOND_DEVICE)
