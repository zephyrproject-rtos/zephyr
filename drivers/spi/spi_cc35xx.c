/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_spi

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_cc35xx, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/arch/common/sys_io.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <string.h>

#include <driverlib/spi.h>
#include <ti/devices/cc35xx/inc/hw_spi.h>
#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
#include <zephyr/devicetree/dma.h>
#include <zephyr/drivers/dma.h>
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */

#include "spi_context.h"

#define SPI_INT_ALL                                                                                \
	(SPI_MIS_TX_SET | SPI_MIS_RX_SET | SPI_MIS_RXOVF_SET | SPI_MIS_IDLE_SET |                  \
	 SPI_MIS_TXEMPTY_SET | SPI_MIS_PER_SET | SPI_MIS_RTOUT_SET | SPI_MIS_DMARX_SET |           \
	 SPI_MIS_DMATX_SET)
#define IDLE_CHAR              0x00
#define SPI_SLAVE_BUS_MAX_FREQ 40000000L

#define SPI_CC35XX_DATA_WIDTH 8
#define SPI_CC35XX_DFS        (SPI_CC35XX_DATA_WIDTH >> 3)

#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN

#define SPI_CC35XX_DMA_MAX_TRANSFER_SIZE 0x3FFFU
#define SPI_CC35XX_SRAM_START            CONFIG_SRAM_BASE_ADDRESS
#define SPI_CC35XX_SRAM_END              (SPI_CC35XX_SRAM_START + (CONFIG_SRAM_SIZE * 1024UL))
#define SPI_CC35XX_DMA_BUFFER_IN_SRAM(buf, len)                                                    \
	({                                                                                         \
		uintptr_t addr = POINTER_TO_UINT(buf);                                             \
		uintptr_t end = addr + (len);                                                      \
                                                                                                   \
		(end >= addr) && (addr >= SPI_CC35XX_SRAM_START) && (end <= SPI_CC35XX_SRAM_END);  \
	})

static uint32_t dummy_tx = IDLE_CHAR;
static uint32_t dummy_rx;

#define SPI_CC35XX_DMA_RX_TRANSFER_DONE BIT(0)
#define SPI_CC35XX_DMA_TX_TRANSFER_DONE BIT(1)

enum transfer_direction {
	SPI_CC35XX_TRANSFER_DIR_NONE,
	SPI_CC35XX_TRANSFER_DIR_TX,
	SPI_CC35XX_TRANSFER_DIR_RX,
	SPI_CC35XX_TRANSFER_DIR_BOTH
};

struct spi_cc35xx_dma_stream {
	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	size_t transfer_length;
};
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */

struct spi_cc35xx_config {
	uint32_t base;
	const struct pinctrl_dev_config *pcfg;
	uint32_t sys_clk_freq;
	const uint8_t irq_num;
};

struct spi_cc35xx_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
	struct spi_cc35xx_dma_stream dma_rx;
	struct spi_cc35xx_dma_stream dma_tx;
	uint8_t dma_status_flags;
	uint8_t tx_scratch_buf[CONFIG_SPI_CC35XX_DMA_SCRATCH_BUFFER_SIZE];
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */
	size_t rxleft;
};

#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
static int spi_cc35xx_dma_start(const struct device *dev, enum transfer_direction dir);
static void spi_cc35xx_dma_stop(const struct device *dev);
static int spi_cc35xx_dma_load_tx(const struct device *dev, const uint8_t *tx_data,
				  size_t buf_size);
static int spi_cc35xx_dma_load_rx(const struct device *dev, uint8_t *rx_data, size_t buf_size);
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */

static void spi_cc35xx_read_rx_fifo(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t rxd;

	while (SPIGetDataNonBlocking(cfg->base, &rxd)) {
		if (spi_context_rx_buf_on(ctx)) {
			*ctx->rx_buf = rxd;
		}
		if (spi_context_rx_on(ctx)) {
			spi_context_update_rx(ctx, SPI_CC35XX_DFS, 1);
		}
	}
}

static void spi_cc35xx_fill_tx_fifo(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t txd;

	while (sys_read32(cfg->base + SPI_O_STA) & SPI_STA_TNF_NOT_FULL) {
		if (!spi_context_tx_on(ctx) && !data->rxleft) {
			return;
		}

		if (data->rxleft) {
			data->rxleft--;
		}

		/* Fill TX fifo with idle chars if we have data to read,
		 * but nothing to send
		 */
		txd = IDLE_CHAR;
		if (spi_context_tx_buf_on(ctx)) {
			txd = *ctx->tx_buf;
		}
		if (spi_context_tx_on(ctx)) {
			spi_context_update_tx(ctx, SPI_CC35XX_DFS, 1);
		}
		SPIPutData(cfg->base, txd);
	}
}

static void spi_cc35xx_flush_fifo(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;

	sys_write32(sys_read32(cfg->base + SPI_O_CTL0) | SPI_CTL0_FIFORST_RST_TRIG |
			    SPI_CTL0_IDLEPOCI_IDLE_ONE,
		    cfg->base + SPI_O_CTL0);

	while (sys_read32(cfg->base + SPI_O_CTL0) & SPI_CTL0_FIFORST) {
		/* NOP */
	}
}

static int spi_cc35xx_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int is_master = SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER;
	uint32_t prot, freq;
	int ret, mode;

	if (spi_context_configured(ctx, config)) {
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word sizes other than 8 bits are not supported");
		return -ENOTSUP;
	}

	SPIDisable(cfg->base);
	SPIDisableInt(cfg->base, SPI_INT_ALL);
	SPIClearInt(cfg->base, SPI_INT_ALL);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Applying SPI pinctrl state failed");
		return ret;
	}

	mode = is_master ? SPI_MODE_CONTROLLER : SPI_MODE_PERIPHERAL;
	if (config->operation & SPI_MODE_LOOP) {
		mode |= SPI_CTL1_LBM_ENABLE;
	}

	prot = is_master ? SPI_CTL0_FRF_MOTOROLA_3WIRE : SPI_CTL0_FRF_MOTOROLA_4WIRE;
	freq = is_master ? config->frequency : SPI_SLAVE_BUS_MAX_FREQ;
	prot |= SPI_MODE_GET(config->operation) & SPI_MODE_CPOL ? SPI_CTL0_SPO_HIGH
								: SPI_CTL0_SPO_LOW;
	prot |= SPI_MODE_GET(config->operation) & SPI_MODE_CPHA ? SPI_CTL0_SPH_SECOND
								: SPI_CTL0_SPH_FIRST;

	SPIConfigSetExpClk(cfg->base, cfg->sys_clk_freq, prot, mode, freq, 8);
	sys_write32(SPI_IFLS_RXSEL_LEVEL_1 | SPI_IFLS_TXSEL_LVL_1_2, cfg->base + SPI_O_IFLS);

	if (config->operation & SPI_TRANSFER_LSB) {
		sys_write32((sys_read32(cfg->base + SPI_O_CTL1) & ~SPI_CTL1_MSB_M) |
				    SPI_CTL1_MSB_LSB,
			    cfg->base + SPI_O_CTL1);
	}

	sys_write32(BIT(0), cfg->base + SPI_O_CLKCFG);

	SPIEnable(cfg->base);

	spi_cc35xx_flush_fifo(dev);

	ctx->config = config;
	return 0;
}

#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
static inline size_t spi_cc35xx_dma_transfer_size(const struct device *dev)
{
	struct spi_cc35xx_data *data = dev->data;

	if (data->ctx.rx_len == 0) {
		return data->ctx.tx_len;
	} else if (data->ctx.tx_len == 0) {
		return data->ctx.rx_len;
	} else {
		return MIN(data->ctx.tx_len, data->ctx.rx_len);
	}
}

static int spi_cc35xx_dma_transmit_next_packet(const struct device *dev,
					       enum transfer_direction dir)
{
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;
	size_t tx_dma_len;
	size_t rx_dma_len;

	if ((dir == SPI_CC35XX_TRANSFER_DIR_RX) || (dir == SPI_CC35XX_TRANSFER_DIR_BOTH)) {
		if (dir == SPI_CC35XX_TRANSFER_DIR_RX) {
			rx_dma_len = ctx->rx_len ? ctx->rx_len
						 : ctx->tx_len - data->dma_tx.transfer_length;
			data->dma_rx.transfer_length = ctx->rx_len ? rx_dma_len : 0;
		} else {
			rx_dma_len = spi_cc35xx_dma_transfer_size(dev);
			data->dma_rx.transfer_length = rx_dma_len;
		}
		if (data->dma_rx.dev_dma && rx_dma_len) {
			ret = spi_cc35xx_dma_load_rx(dev, ctx->rx_buf, rx_dma_len);
			if (ret) {
				LOG_ERR("%s: Failed load RX DMA", dev->name);
				return ret;
			}
		} else {
			dir = (dir == SPI_CC35XX_TRANSFER_DIR_BOTH) ? SPI_CC35XX_TRANSFER_DIR_TX
								    : SPI_CC35XX_TRANSFER_DIR_NONE;
		}

		data->dma_status_flags &= ~SPI_CC35XX_DMA_RX_TRANSFER_DONE;
	}

	if ((dir == SPI_CC35XX_TRANSFER_DIR_TX) || (dir == SPI_CC35XX_TRANSFER_DIR_BOTH)) {
		if (dir == SPI_CC35XX_TRANSFER_DIR_TX) {
			tx_dma_len = ctx->tx_len ? ctx->tx_len
						 : ctx->rx_len - data->dma_rx.transfer_length;
			data->dma_tx.transfer_length = ctx->tx_len ? tx_dma_len : 0;
		} else {
			tx_dma_len = spi_cc35xx_dma_transfer_size(dev);
			data->dma_tx.transfer_length = tx_dma_len;
		}
		if (data->dma_tx.dev_dma && tx_dma_len) {
			ret = spi_cc35xx_dma_load_tx(dev, ctx->tx_buf, tx_dma_len);
			if (ret) {
				LOG_ERR("%s: Failed load TX DMA", dev->name);
				return ret;
			}
		} else {
			dir = (dir == SPI_CC35XX_TRANSFER_DIR_BOTH) ? SPI_CC35XX_TRANSFER_DIR_RX
								    : SPI_CC35XX_TRANSFER_DIR_NONE;
		}

		data->dma_status_flags &= ~SPI_CC35XX_DMA_TX_TRANSFER_DONE;
	}

	spi_cc35xx_dma_start(dev, dir);

	return 0;
}
#else
static void spi_cc35xx_master_transceive(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_cc35xx_flush_fifo(dev);
	spi_context_cs_control(ctx, true);
	spi_cc35xx_fill_tx_fifo(dev);
	SPIClearInt(cfg->base, SPI_MIS_TX_SET);
	SPIEnableInt(cfg->base, SPI_MIS_TX_SET);
}

#ifdef CONFIG_SPI_SLAVE
static void spi_cc35xx_slave_transceive(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;

	spi_cc35xx_flush_fifo(dev);
	spi_cc35xx_fill_tx_fifo(dev);
	SPIClearInt(cfg->base, SPI_MIS_RX_SET);
	SPIEnableInt(cfg->base, SPI_MIS_RX_SET);
}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */

static int spi_cc35xx_transceive(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				 void *userdata, bool async)
{
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, async, cb, userdata, config);

	ret = spi_cc35xx_configure(dev, config);
	if (ret) {
		goto ctx_release;
	}

	spi_cc35xx_flush_fifo(dev);

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, SPI_CC35XX_DFS);

	if (!spi_context_rx_buf_on(ctx) && !spi_context_tx_buf_on(ctx)) {
		goto ctx_release;
	}

#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
	if (spi_context_total_tx_len(ctx) > SPI_CC35XX_DMA_MAX_TRANSFER_SIZE ||
	    spi_context_total_rx_len(ctx) > SPI_CC35XX_DMA_MAX_TRANSFER_SIZE) {
		ret = -EINVAL;
		goto ctx_release;
	}

#ifdef CONFIG_SPI_SLAVE
	if (!spi_context_is_slave(ctx)) {
		spi_context_cs_control(ctx, true);
	}
#else
	spi_context_cs_control(ctx, true);
#endif /* CONFIG_SPI_SLAVE */

	spi_cc35xx_dma_transmit_next_packet(dev, SPI_CC35XX_TRANSFER_DIR_BOTH);
#else
	data->rxleft = spi_context_total_rx_len(ctx);

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(ctx)) {
		spi_cc35xx_slave_transceive(dev);
	} else {
		spi_cc35xx_master_transceive(dev);
	}
#else
	spi_cc35xx_master_transceive(dev);
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */

	ret = spi_context_wait_for_completion(ctx);

	if (async) {
		spi_context_release(ctx, ret);
		return 0;
	}

ctx_release:
#ifdef CONFIG_SPI_SLAVE
	if (!spi_context_is_slave(ctx)) {
		if (!(ctx->config->operation & SPI_HOLD_ON_CS)) {
			spi_context_cs_control(ctx, false);
		}
	} else if (!ret) {
		ret = data->ctx.recv_frames;
	}
#else
	if (!(ctx->config->operation & SPI_HOLD_ON_CS)) {
		spi_context_cs_control(ctx, false);
	}
#endif /* CONFIG_SPI_SLAVE */
#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
	spi_cc35xx_dma_stop(dev);
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_cc35xx_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, config)) {
		return -EINVAL;
	}

	if (SPIBusy(cfg->base)) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static void spi_cc35xx_isr(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t irq_status = SPIIntStatus(cfg->base, true);
	uint32_t txrx_irq = irq_status & (SPI_MIS_RX | SPI_MIS_TX);

	if (txrx_irq) {
		SPIClearInt(cfg->base, txrx_irq);
		spi_cc35xx_read_rx_fifo(dev);

		if (!spi_context_rx_on(ctx) && !spi_context_tx_on(ctx)) {
			SPIDisableInt(cfg->base, txrx_irq);

			if (txrx_irq & SPI_MIS_RX) {
				spi_context_complete(ctx, dev, 0);
			} else {
				SPIClearInt(cfg->base, SPI_MIS_IDLE_SET);
				SPIEnableInt(cfg->base, SPI_MIS_IDLE_SET);
			}
			return;
		}

		spi_cc35xx_fill_tx_fifo(dev);
	}

	if (irq_status & SPI_MIS_IDLE) {
		SPIDisableInt(cfg->base, SPI_MIS_IDLE);
		SPIClearInt(cfg->base, SPI_MIS_IDLE);
		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, 0);
	}

	if (irq_status & SPI_MIS_RXOVF) {
		SPIClearInt(cfg->base, SPI_MIS_RXOVF);
		LOG_ERR("%s: RX overflow occurred", dev->name);
		spi_cc35xx_read_rx_fifo(dev);
		/* flush RX FIFO to clear overflow condition */
		spi_cc35xx_flush_fifo(dev);
		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, -EIO);
	}

#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
	if (irq_status & SPI_MIS_DMARX) {
		SPIDisableInt(cfg->base, SPI_MIS_DMARX);
		SPIClearInt(cfg->base, SPI_MIS_DMARX);
		data->dma_status_flags |= SPI_CC35XX_DMA_RX_TRANSFER_DONE;
	}

	if (irq_status & SPI_MIS_DMATX) {
		SPIDisableInt(cfg->base, SPI_MIS_DMATX);
		SPIClearInt(cfg->base, SPI_MIS_DMATX);
		data->dma_status_flags |= SPI_CC35XX_DMA_TX_TRANSFER_DONE;
	}

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(ctx)) {

		bool tx_done = data->dma_status_flags & SPI_CC35XX_DMA_TX_TRANSFER_DONE;
		bool rx_done = data->dma_status_flags & SPI_CC35XX_DMA_RX_TRANSFER_DONE;

		if (rx_done) {
			spi_context_update_rx(&data->ctx, SPI_CC35XX_DFS,
					      data->ctx.rx_len ? data->dma_rx.transfer_length : 0);
		}
		if (tx_done) {
			spi_context_update_tx(&data->ctx, SPI_CC35XX_DFS,
					      data->ctx.tx_len ? data->dma_tx.transfer_length : 0);
		}

		if (!spi_context_rx_buf_on(ctx) && !spi_context_tx_buf_on(ctx)) {
			/* nothing left to rx or tx, we're done! */
			spi_cc35xx_dma_stop(dev);
			spi_context_complete(&data->ctx, dev, 0);

			return;
		}

		if (rx_done) {
			spi_cc35xx_dma_transmit_next_packet(dev, SPI_CC35XX_TRANSFER_DIR_RX);
		}

		if (tx_done) {
			spi_cc35xx_dma_transmit_next_packet(dev, SPI_CC35XX_TRANSFER_DIR_TX);
		}
		return;
	}
#endif /* CONFIG_SPI_SLAVE */

	if (data->dma_status_flags ==
	    (SPI_CC35XX_DMA_RX_TRANSFER_DONE | SPI_CC35XX_DMA_TX_TRANSFER_DONE)) {
		spi_context_update_rx(&data->ctx, SPI_CC35XX_DFS,
				      data->ctx.rx_len ? data->dma_rx.transfer_length : 0);
		spi_context_update_tx(&data->ctx, SPI_CC35XX_DFS,
				      data->ctx.tx_len ? data->dma_tx.transfer_length : 0);

		if (!spi_context_rx_buf_on(ctx) && !spi_context_tx_buf_on(ctx)) {
			/* nothing left to rx or tx, we're done! */
			if (!(ctx->config->operation & SPI_HOLD_ON_CS)) {
				spi_context_cs_control(&data->ctx, false);
			}
			spi_cc35xx_dma_stop(dev);
			spi_context_complete(&data->ctx, dev, 0);

			return;
		}

		spi_cc35xx_dma_transmit_next_packet(dev, SPI_CC35XX_TRANSFER_DIR_BOTH);
	}
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */
}

#ifdef CONFIG_SPI_ASYNC
static int spi_cc35xx_transceive_async(const struct device *dev, const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				       void *userdata)
{
	return spi_cc35xx_transceive(dev, config, tx_bufs, rx_bufs, cb, userdata, true);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_cc35xx_transceive_sync(const struct device *dev, const struct spi_config *config,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs)
{
	return spi_cc35xx_transceive(dev, config, tx_bufs, rx_bufs, NULL, NULL, false);
}

static DEVICE_API(spi, spi_cc35xx_driver_api) = {
	.transceive = spi_cc35xx_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_cc35xx_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_cc35xx_release,
};

#ifdef CONFIG_SPI_CC35XX_DMA_DRIVEN
static void spi_cc35xx_dma_stop(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;

	SPIDisableDMA(cfg->base, SPI_DMACR_TXEN | SPI_DMACR_RXEN);
	SPIDisableInt(cfg->base, SPI_MIS_DMATX_SET | SPI_MIS_DMARX_SET | SPI_MIS_RXOVF);
	dma_stop(data->dma_rx.dev_dma, data->dma_rx.dma_channel);
	dma_stop(data->dma_tx.dev_dma, data->dma_tx.dma_channel);
}

static int spi_cc35xx_dma_load_tx(const struct device *dev, const uint8_t *tx_data, size_t buf_size)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	data->dma_tx.blk_cfg.dest_address = cfg->base + SPI_O_TXDATA;
	data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_tx.blk_cfg.block_size = buf_size;
	if (spi_context_tx_buf_on(ctx)) {
		if (!SPI_CC35XX_DMA_BUFFER_IN_SRAM(tx_data, buf_size)) {
			if (buf_size > sizeof(data->tx_scratch_buf)) {
				LOG_ERR("%s: TX DMA buffer must be in SRAM", dev->name);
				return -ENOTSUP;
			}

			memcpy(data->tx_scratch_buf, tx_data, buf_size);
			tx_data = data->tx_scratch_buf;
		}

		data->dma_tx.blk_cfg.source_address = (uint32_t)tx_data;
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_tx.blk_cfg.source_address = (uint32_t)&dummy_tx;
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	ret = dma_config(data->dma_tx.dev_dma, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);

	return ret;
}

static int spi_cc35xx_dma_load_rx(const struct device *dev, uint8_t *rx_data, size_t buf_size)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	data->dma_rx.blk_cfg.source_address = cfg->base + SPI_O_RXDATA;
	data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_rx.blk_cfg.block_size = buf_size;

	if (spi_context_rx_buf_on(ctx)) {
		data->dma_rx.blk_cfg.dest_address = (uint32_t)rx_data;
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_rx.blk_cfg.dest_address = (uint32_t)&dummy_rx;
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	ret = dma_config(data->dma_rx.dev_dma, data->dma_rx.dma_channel, &data->dma_rx.dma_cfg);

	return ret;
}

static int spi_cc35xx_dma_start(const struct device *dev, enum transfer_direction dir)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;
	int ret;

	if ((dir == SPI_CC35XX_TRANSFER_DIR_TX) || (dir == SPI_CC35XX_TRANSFER_DIR_BOTH)) {
		SPIDisableDMA(cfg->base, SPI_DMACR_TXEN);

		SPIClearInt(cfg->base, SPI_MIS_DMATX_SET);
		SPIEnableInt(cfg->base, SPI_MIS_DMATX_SET);

		ret = dma_start(data->dma_tx.dev_dma, data->dma_tx.dma_channel);
		if (ret) {
			return ret;
		}
		SPIEnableDMA(cfg->base, SPI_DMACR_TXEN);
	}

	if ((dir == SPI_CC35XX_TRANSFER_DIR_RX) || (dir == SPI_CC35XX_TRANSFER_DIR_BOTH)) {
		SPIDisableDMA(cfg->base, SPI_DMACR_RXEN);

		SPIClearInt(cfg->base, SPI_MIS_DMARX_SET | SPI_MIS_RXOVF);
		SPIEnableInt(cfg->base, SPI_MIS_DMARX_SET | SPI_MIS_RXOVF);

		ret = dma_start(data->dma_rx.dev_dma, data->dma_rx.dma_channel);
		if (ret) {
			return ret;
		}

		SPIEnableDMA(cfg->base, SPI_DMACR_RXEN);
	}

	return 0;
}
static int spi_cc35xx_dma_init(const struct device *dev)
{
	const struct spi_cc35xx_config *cfg = dev->config;
	struct spi_cc35xx_data *data = dev->data;

	if (data->dma_rx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_rx.dev_dma)) {
			LOG_ERR("%s: RX DMA channel not ready", dev->name);
			return -ENODEV;
		}
		data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
		data->dma_rx.dma_cfg.user_data = (void *)data;
	}

	if (data->dma_tx.dev_dma != NULL) {
		if (!device_is_ready(data->dma_tx.dev_dma)) {
			LOG_ERR("%s: TX DMA channel not ready", dev->name);
			return -ENODEV;
		}
		data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
		data->dma_tx.dma_cfg.user_data = (void *)data;
	}

	return 0;
}

#define SPI_CC35XX_DMA_CHANNEL_INIT(n, dir, ch_dir, src_burst, dst_burst)                          \
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, dir)),                               \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(n, dir, channel),                                 \
	.dma_cfg = {                                                                               \
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, dir, channel_config),                     \
		.channel_direction = ch_dir,                                                       \
		.source_data_size = 1,                                                             \
		.dest_data_size = 1,                                                               \
		.source_burst_length = src_burst,                                                  \
		.dest_burst_length = dst_burst,                                                    \
		.block_count = 1,                                                                  \
	}

#define SPI_CC35XX_DMA_CHANNEL(n, dir, ch_dir, src_burst, dst_burst)                               \
	.dma_##dir = {COND_CODE_1(                                                                 \
		DT_INST_DMAS_HAS_NAME(n, dir),                                                     \
		(SPI_CC35XX_DMA_CHANNEL_INIT(n, dir, ch_dir, src_burst, dst_burst)), (NULL))},

#define SPI_CC35XX_DMA_INIT_FUNC(dev) spi_cc35xx_dma_init(dev)
#else
#define SPI_CC35XX_DMA_CHANNEL(n, dir, ch_dir, src_burst, dst_burst)

#define SPI_CC35XX_DMA_INIT_FUNC(dev) 0
#endif /* CONFIG_SPI_CC35XX_DMA_DRIVEN */

#define SPI_CC35XX_INIT_FUNC(n)                                                                    \
	static int spi_cc35xx_init_##n(const struct device *dev)                                   \
	{                                                                                          \
		struct spi_cc35xx_data *data = dev->data;                                          \
		const struct spi_cc35xx_config *cfg = dev->config;                                 \
		int err;                                                                           \
                                                                                                   \
		data->ctx.config = NULL;                                                           \
		err = spi_context_cs_configure_all(&data->ctx);                                    \
		if (err < 0)                                                                       \
			return err;                                                                \
                                                                                                   \
		spi_context_unlock_unconditionally(&data->ctx);                                    \
		SPIDisable(cfg->base);                                                             \
		SPIDisableInt(cfg->base, SPI_INT_ALL);                                             \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_cc35xx_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(cfg->irq_num);                                                          \
		err = SPI_CC35XX_DMA_INIT_FUNC(dev);                                               \
		if (err < 0)                                                                       \
			return err;                                                                \
                                                                                                   \
		return 0;                                                                          \
	}

#define SPI_CC35XX_DEVICE_INIT(n)                                                                  \
	DEVICE_DT_INST_DEFINE(n, spi_cc35xx_init_##n, PM_DEVICE_DT_INST_GET(n),                    \
			      &spi_cc35xx_data_##n, &spi_cc35xx_config_##n, POST_KERNEL,           \
			      CONFIG_SPI_INIT_PRIORITY, &spi_cc35xx_driver_api)

#define SPI_CC35XX_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SPI_CC35XX_INIT_FUNC(n);                                                                   \
                                                                                                   \
	static const struct spi_cc35xx_config spi_cc35xx_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),               \
		.irq_num = DT_INST_IRQN(n),                                                        \
	};                                                                                         \
                                                                                                   \
	static struct spi_cc35xx_data spi_cc35xx_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_cc35xx_data_##n, ctx),                                   \
		SPI_CONTEXT_INIT_SYNC(spi_cc35xx_data_##n, ctx),                                   \
		SPI_CC35XX_DMA_CHANNEL(n, tx, MEMORY_TO_PERIPHERAL, 8, 1)                          \
			SPI_CC35XX_DMA_CHANNEL(n, rx, PERIPHERAL_TO_MEMORY, 1, 8)                  \
				SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};             \
	SPI_CC35XX_DEVICE_INIT(n);

DT_INST_FOREACH_STATUS_OKAY(SPI_CC35XX_INIT)
