/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_ambiq);

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <stdlib.h>
#include <errno.h>
#include "spi_context.h"
#include <am_mcu_apollo.h>

#define PWRCTRL_MAX_WAIT_US 5

typedef int (*ambiq_spi_pwr_func_t)(void);

struct spi_ambiq_config {
	uint32_t base;
	int size;
	uint32_t clock_freq;
	const struct pinctrl_dev_config *pcfg;
	ambiq_spi_pwr_func_t pwr_func;
	void (*irq_config_func)(void);
};

struct spi_ambiq_data {
	struct spi_context ctx;
	am_hal_iom_config_t iom_cfg;
	void *iom_handler;
	int inst_idx;
	bool cont;
};

typedef void (*spi_context_update_trx)(struct spi_context *ctx, uint8_t dfs, uint32_t len);

#define SPI_WORD_SIZE 8

#define SPI_CS_INDEX 3

#ifdef CONFIG_SPI_AMBIQ_DMA
static __aligned(32) struct {
	__aligned(32) uint32_t buf[CONFIG_SPI_DMA_TCB_BUFFER_SIZE];
} spi_dma_tcb_buf[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)] __attribute__((__section__(".nocache")));

static void spi_ambiq_callback(void *callback_ctxt, uint32_t status)
{
	const struct device *dev = callback_ctxt;
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	/* de-assert cs until transfer finished and no need to hold cs */
	if (!data->cont) {
		spi_context_cs_control(ctx, false);
	}
	spi_context_complete(ctx, dev, (status == AM_HAL_STATUS_SUCCESS) ? 0 : -EIO);
}
#endif

static void spi_ambiq_reset(const struct device *dev)
{
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	/* cancel timed out transaction */
	am_hal_iom_disable(data->iom_handler);
	/* NULL config to trigger reconfigure on next xfer */
	ctx->config = NULL;
	spi_context_cs_control(ctx, false);
	/* signal any thread waiting on sync semaphore */
	spi_context_complete(ctx, dev, -ETIMEDOUT);
	/* clean up for next xfer */
	k_sem_reset(&ctx->sync);
}

static void spi_ambiq_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct spi_ambiq_data *data = dev->data;

	am_hal_iom_interrupt_status_get(data->iom_handler, false, &ui32Status);
	am_hal_iom_interrupt_clear(data->iom_handler, ui32Status);
	am_hal_iom_interrupt_service(data->iom_handler, ui32Status);
}

static int spi_config(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	const struct spi_ambiq_config *cfg = dev->config;
	struct spi_context *ctx = &(data->ctx);

	data->iom_cfg.eInterfaceMode = AM_HAL_IOM_SPI_MODE;

	int ret = 0;

	if (spi_context_configured(ctx, config)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != SPI_WORD_SIZE) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_MODE_CPOL) {
		if (config->operation & SPI_MODE_CPHA) {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_3;
		} else {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_2;
		}
	} else {
		if (config->operation & SPI_MODE_CPHA) {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_1;
		} else {
			data->iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_0;
		}
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}
	if (config->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode not supported");
		return -ENOTSUP;
	}

	if (cfg->clock_freq > AM_HAL_IOM_MAX_FREQ) {
		LOG_ERR("Clock frequency too high");
		return -ENOTSUP;
	}

	/* Select slower of two: SPI bus frequency for SPI device or SPI master clock frequency */
	data->iom_cfg.ui32ClockFreq =
		(config->frequency ? MIN(config->frequency, cfg->clock_freq) : cfg->clock_freq);
	ctx->config = config;

#ifdef CONFIG_SPI_AMBIQ_DMA
	data->iom_cfg.pNBTxnBuf = spi_dma_tcb_buf[data->inst_idx].buf;
	data->iom_cfg.ui32NBTxnBufLength = CONFIG_SPI_DMA_TCB_BUFFER_SIZE;
#endif

	/* Disable IOM instance as it cannot be configured when enabled*/
	ret = am_hal_iom_disable(data->iom_handler);

	ret = am_hal_iom_configure(data->iom_handler, &data->iom_cfg);

	ret = am_hal_iom_enable(data->iom_handler);

	return ret;
}

static int spi_ambiq_xfer_half_duplex(const struct device *dev, am_hal_iom_dir_e dir)
{
	am_hal_iom_transfer_t trans = {0};
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool is_last = false;
	uint32_t rem_num, cur_num = 0;
	int ret = 0;
	spi_context_update_trx ctx_update;

	if (dir == AM_HAL_IOM_FULLDUPLEX) {
		return -EINVAL;
	} else if (dir == AM_HAL_IOM_RX) {
		trans.eDirection = AM_HAL_IOM_RX;
		ctx_update = spi_context_update_rx;
	} else {
		trans.eDirection = AM_HAL_IOM_TX;
		ctx_update = spi_context_update_tx;
	}
	if (dir == AM_HAL_IOM_RX) {
		rem_num = ctx->rx_len;
	} else {
		rem_num = ctx->tx_len;
	}
	while (rem_num) {
		cur_num = (rem_num > AM_HAL_IOM_MAX_TXNSIZE_SPI) ? AM_HAL_IOM_MAX_TXNSIZE_SPI
								 : rem_num;
		trans.ui32NumBytes = cur_num;
		trans.pui32TxBuffer = (uint32_t *)ctx->tx_buf;
		trans.pui32RxBuffer = (uint32_t *)ctx->rx_buf;
		ctx_update(ctx, 1, cur_num);
		if ((!spi_context_tx_buf_on(ctx)) && (!spi_context_rx_buf_on(ctx))) {
			is_last = true;
		}
#ifdef CONFIG_SPI_AMBIQ_DMA
		if (AM_HAL_STATUS_SUCCESS !=
		    am_hal_iom_nonblocking_transfer(data->iom_handler, &trans,
						    ((is_last == true) ? spi_ambiq_callback : NULL),
						    (void *)dev)) {
			return -EIO;
		}
		if (is_last) {
			ret = spi_context_wait_for_completion(ctx);
		}
#else
		ret = am_hal_iom_blocking_transfer(data->iom_handler, &trans);
#endif
		rem_num -= cur_num;
		if (ret != 0) {
			return -EIO;
		}
	}

	return 0;
}

static int spi_ambiq_xfer_full_duplex(const struct device *dev)
{
	am_hal_iom_transfer_t trans = {0};
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool trx_once = (ctx->tx_len == ctx->rx_len);
	int ret = 0;

	/* Tx and Rx length must be the same for am_hal_iom_spi_blocking_fullduplex */
	trans.eDirection = AM_HAL_IOM_FULLDUPLEX;
	trans.ui32NumBytes = MIN(ctx->rx_len, ctx->tx_len);
	trans.pui32RxBuffer = (uint32_t *)ctx->rx_buf;
	trans.pui32TxBuffer = (uint32_t *)ctx->tx_buf;
	spi_context_update_tx(ctx, 1, trans.ui32NumBytes);
	spi_context_update_rx(ctx, 1, trans.ui32NumBytes);

	ret = am_hal_iom_spi_blocking_fullduplex(data->iom_handler, &trans);
	if (ret != 0) {
		return -EIO;
	}

	/* Transfer the remaining bytes */
	if (!trx_once) {
		spi_context_update_trx ctx_update;

		if (ctx->tx_len) {
			trans.eDirection = AM_HAL_IOM_TX;
			trans.ui32NumBytes = ctx->tx_len;
			trans.pui32TxBuffer = (uint32_t *)ctx->tx_buf;
			ctx_update = spi_context_update_tx;
		} else {
			trans.eDirection = AM_HAL_IOM_RX;
			trans.ui32NumBytes = ctx->rx_len;
			trans.pui32RxBuffer = (uint32_t *)ctx->rx_buf;
			ctx_update = spi_context_update_rx;
		}
		ret = am_hal_iom_blocking_transfer(data->iom_handler, &trans);
		ctx_update(ctx, 1, trans.ui32NumBytes);
		if (ret != 0) {
			return -EIO;
		}
	}

	return 0;
}

static int spi_ambiq_xfer(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;
	data->cont = (config->operation & SPI_HOLD_ON_CS) ? true : false;

	spi_context_cs_control(ctx, true);

	while (1) {
		if (spi_context_tx_buf_on(ctx) && spi_context_rx_buf_on(ctx)) {
			if (ctx->rx_buf == ctx->tx_buf) {
				spi_context_update_rx(ctx, 1, ctx->rx_len);
			} else if (!(config->operation & SPI_HALF_DUPLEX)) {
				ret = spi_ambiq_xfer_full_duplex(dev);
				if (ret != 0) {
					spi_ambiq_reset(dev);
					LOG_ERR("SPI full-duplex comm error: %d", ret);
					return ret;
				}
			}
		}
		if (spi_context_tx_on(ctx)) {
			if (ctx->tx_buf == NULL) {
				spi_context_update_tx(ctx, 1, ctx->tx_len);
			} else {
				ret = spi_ambiq_xfer_half_duplex(dev, AM_HAL_IOM_TX);
				if (ret != 0) {
					spi_ambiq_reset(dev);
					LOG_ERR("SPI TX comm error: %d", ret);
					return ret;
				}
			}
		} else if (spi_context_rx_on(ctx)) {
			if (ctx->rx_buf == NULL) {
				spi_context_update_rx(ctx, 1, ctx->rx_len);
			} else {
				ret = spi_ambiq_xfer_half_duplex(dev, AM_HAL_IOM_RX);
				if (ret != 0) {
					spi_ambiq_reset(dev);
					LOG_ERR("SPI Rx comm error: %d", ret);
					return ret;
				}
			}
		} else {
			break;
		}
	}

#ifndef CONFIG_SPI_AMBIQ_DMA
	if (!data->cont) {
		spi_context_cs_control(ctx, false);
		spi_context_complete(ctx, dev, ret);
	}
#endif
	return ret;
}

static int spi_ambiq_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_ambiq_data *data = dev->data;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	/* context setup */
	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_config(dev, config);

	if (ret) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	ret = spi_ambiq_xfer(dev, config);

	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_ambiq_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	am_hal_iom_status_t iom_status;

	am_hal_iom_status_get(data->iom_handler, &iom_status);

	if ((iom_status.bStatIdle != IOM0_STATUS_IDLEST_IDLE) ||
	    (iom_status.bStatCmdAct == IOM0_STATUS_CMDACT_ACTIVE) ||
	    (iom_status.ui32NumPendTransactions)) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_ambiq_driver_api = {
	.transceive = spi_ambiq_transceive,
	.release = spi_ambiq_release,
};

static int spi_ambiq_init(const struct device *dev)
{
	struct spi_ambiq_data *data = dev->data;
	const struct spi_ambiq_config *cfg = dev->config;
	int ret = 0;

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_iom_initialize((cfg->base - REG_IOM_BASEADDR) / cfg->size, &data->iom_handler)) {
		LOG_ERR("Fail to initialize SPI\n");
		return -ENXIO;
	}

	ret = cfg->pwr_func();

	ret |= pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	ret |= spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		LOG_ERR("Fail to config SPI pins\n");
		goto end;
	}

#ifdef CONFIG_SPI_AMBIQ_DMA
	am_hal_iom_interrupt_clear(data->iom_handler, AM_HAL_IOM_INT_CQUPD | AM_HAL_IOM_INT_ERR);
	am_hal_iom_interrupt_enable(data->iom_handler, AM_HAL_IOM_INT_CQUPD | AM_HAL_IOM_INT_ERR);
	cfg->irq_config_func();
#endif
end:
	if (ret < 0) {
		am_hal_iom_uninitialize(data->iom_handler);
	} else {
		spi_context_unlock_unconditionally(&data->ctx);
	}
	return ret;
}

#define AMBIQ_SPI_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static int pwr_on_ambiq_spi_##n(void)                                                      \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(PWRCTRL_MAX_WAIT_US);                                                  \
		return 0;                                                                          \
	}                                                                                          \
	static void spi_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_ambiq_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static struct spi_ambiq_data spi_ambiq_data##n = {                                         \
		SPI_CONTEXT_INIT_LOCK(spi_ambiq_data##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_ambiq_data##n, ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx).inst_idx = n};                \
	static const struct spi_ambiq_config spi_ambiq_config##n = {                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = spi_irq_config_func_##n,                                        \
		.pwr_func = pwr_on_ambiq_spi_##n};                                                 \
	DEVICE_DT_INST_DEFINE(n, spi_ambiq_init, NULL, &spi_ambiq_data##n, &spi_ambiq_config##n,   \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_SPI_INIT)
