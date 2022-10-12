/*
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Copyright (c) 2022 Nuvoton Technology Corporation.
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

#include "NuMicro.h"
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_numaker, LOG_LEVEL_INF);

#include "spi_context.h"

#define DT_DRV_COMPAT nuvoton_numaker_spi

#define SPI_NUMAKER_TX_NOP 0x00

struct spi_numaker_config {
	SPI_T *spi;
    bool is_qspi;
	uint32_t id_rst;
	/* clock configuration */
	uint32_t clk_modidx;
    uint32_t clk_src;
    uint32_t clk_div;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;
};

struct spi_numaker_data {
	struct spi_context ctx;
};


static uint32_t spi_numaker_bus_freq_get(const struct device *dev)
{
	const struct spi_numaker_config *dev_cfg = dev->config;

    if (dev_cfg->is_qspi) {
        return QSPI_GetBusClock((QSPI_T *)dev_cfg->spi);
    } else {
        return SPI_GetBusClock(dev_cfg->spi);
    }
}

static int spi_numaker_configure(const struct device *dev,
			      const struct spi_config *config)
{
	int mode;
    struct spi_numaker_data *data = dev->data;
	const struct spi_numaker_config *dev_cfg = dev->config;

    LOG_DBG("%s", __FUNCTION__);
	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

    if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		LOG_ERR("Loop back mode not support");
		return -ENOTSUP;
    }
    
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not support");
		return -ENOTSUP;
	}

    /* Clear FIFO */
    SPI_ClearRxFIFO(dev_cfg->spi);
    SPI_ClearTxFIFO(dev_cfg->spi);

    /*
        CPOL/CPHA = 0/0 --> SPI_MODE_0
        CPOL/CPHA = 0/1 --> SPI_MODE_1
        CPOL/CPHA = 1/0 --> SPI_MODE_2
        CPOL/CPHA = 1/1 --> SPI_MODE_3
    */
    if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
        mode = (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ?  3 : 2;
    } else {
        mode = (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) ?  1 : 0;
    }
    
    /* Make SPI module be ready to transfer */
    if (dev_cfg->is_qspi) {
        QSPI_Open((QSPI_T *)dev_cfg->spi,
                (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) ? QSPI_SLAVE : QSPI_MASTER,
                (mode == 0) ? QSPI_MODE_0 : (mode == 1) ? QSPI_MODE_1 : (mode == 2) ? QSPI_MODE_2 : QSPI_MODE_3,
                SPI_WORD_SIZE_GET(config->operation),
                config->frequency);
    } else {
        SPI_Open(dev_cfg->spi,
                (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) ? SPI_SLAVE : SPI_MASTER,
                (mode == 0) ? SPI_MODE_0 : (mode == 1) ? SPI_MODE_1 : (mode == 2) ? SPI_MODE_2 : SPI_MODE_3,
                SPI_WORD_SIZE_GET(config->operation),
                config->frequency);
    }
    
    /* Set Transfer LSB or MSB first */
    if ((config->operation) & SPI_TRANSFER_LSB) {
        SPI_SET_LSB_FIRST(dev_cfg->spi);
    } else {
        SPI_SET_MSB_FIRST(dev_cfg->spi);
    }
    
    /* full/half duplex */
    if (((config->operation) & BIT(11)) ) {
        SPI_DISABLE_3WIRE_MODE(dev_cfg->spi);   // full duplex
    } else {
        SPI_ENABLE_3WIRE_MODE(dev_cfg->spi);    // half duplex, which results in 3-wire usage
    }

    /* Active high CS logic */
    if (((config->operation) & BIT(14)) ) {
        SPI_SET_SS_HIGH(dev_cfg->spi);
    } else {
        SPI_SET_SS_LOW(dev_cfg->spi);
    }
    
    /* Enable the automatic hardware slave select function. Select the SS pin and configure as low-active. */
    if (data->ctx.num_cs_gpios != 0) {
        SPI_EnableAutoSS(dev_cfg->spi, SPI_SS, SPI_SS_ACTIVE_LOW);    
    } else {
        SPI_DisableAutoSS(dev_cfg->spi);
    }
    
    /* Set TX FIFO threshold */
    //SPI_SetFIFO(dev_cfg->spi, 2, 2);

	data->ctx.config = config;

	return 0;
}

static int spi_numaker_txrx(const struct device *dev)
{
	struct spi_numaker_data *data = dev->data;
	const struct spi_numaker_config *dev_cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t tx_frame = 0U, rx_frame = 0U;
    uint8_t word_size = 0, spi_dfs = 0;
    uint32_t u32TimeOutCnt;
    int ret = 0; //-1;

    LOG_DBG("%s", __FUNCTION__);
    word_size = SPI_WORD_SIZE_GET(ctx->config->operation);
    
    switch (word_size) {
        case 8:
            spi_dfs = 1;
            break;
        case 16:
            spi_dfs = 2;
            break;
        case 24:
            spi_dfs = 3;
            break;
        case 32:
            spi_dfs = 4;
            break;
        default:
            spi_dfs = 0;
            ret = -1;
            LOG_ERR("Not support SPI WORD size as [%d] bits", word_size);
    }
    if (ret < 0) goto move_exit;
    LOG_DBG("%s -->word_size [%d]", __FUNCTION__, word_size);

    if (spi_context_tx_on(ctx)) {
        tx_frame = ((ctx->tx_buf == NULL) ? SPI_NUMAKER_TX_NOP : UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf)));
        /* Write to TX register */
        SPI_WRITE_TX(dev_cfg->spi, tx_frame);
        spi_context_update_tx(ctx, spi_dfs, 1);
        
        /* Check SPI busy status */
        u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
        while (SPI_IS_BUSY(dev_cfg->spi)) {
            if(--u32TimeOutCnt == 0) {
                LOG_ERR("Wait for SPI time-out");
                ret = -1;
                break;
            }
        }
        if (ret < 0) goto move_exit;
        ret = 0;
        LOG_DBG("%s --> TX [0x%x] done [%d]", __FUNCTION__, tx_frame, ret);
    }
    
    /* Read received data */
    if (spi_context_rx_on(ctx)) {
        if (SPI_GET_RX_FIFO_COUNT(dev_cfg->spi) > 0) {
            rx_frame = SPI_READ_RX(dev_cfg->spi);
            if (ctx->rx_buf != NULL ) UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
            spi_context_update_rx(ctx, spi_dfs, 1);
            ret = 0;
            LOG_DBG("%s --> RX [0x%x] done [%d]", __FUNCTION__, rx_frame, ret);
        }
    }
   
move_exit:
    LOG_DBG("%s --> exit [%d]", __FUNCTION__, ret);
	return ret;
}

/* Remain TX/RX Data in spi_context TX/RX buffer */
static bool spi_numaker_remain_words(struct spi_numaker_data *data)
{
	return spi_context_tx_on(&data->ctx) ||
	       spi_context_rx_on(&data->ctx);
}

static int spi_numaker_transceive(const struct device *dev,
			       const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs,
			       const struct spi_buf_set *rx_bufs)
{
	struct spi_numaker_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const struct spi_numaker_config *dev_cfg = dev->config;
	int ret;

    LOG_DBG("%s", __FUNCTION__);
	spi_context_lock(ctx, false, NULL, config);
    ctx->config = config;
    
	ret = spi_numaker_configure(dev, config);
	if (ret < 0) {
		goto move_exit;
	}

    SPI_ENABLE(dev_cfg->spi);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);
    
    /* if cs is defined: software cs control, set active true */
    if (config->cs) {
        spi_context_cs_control(&data->ctx, true);
    }
    
    /* transceive tx/rx data */    
	do {
		ret = spi_numaker_txrx(dev);
		if (ret < 0) {
			break;
		}
	} while (spi_numaker_remain_words(data));

    /* if cs is defined: software cs control, set active false */
    if (config->cs) {
        spi_context_cs_control(&data->ctx, false);
    }
    SPI_DISABLE(dev_cfg->spi);

move_exit:
	spi_context_release(ctx, ret);
    LOG_DBG("%s --> [%d]", __FUNCTION__, ret);
	return ret;
}

static int spi_numaker_release(const struct device *dev,
			    const struct spi_config *config)
{
	struct spi_numaker_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
    
	if (!spi_context_configured(ctx, config)) {
		return -EINVAL;
	}
	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static struct spi_driver_api spi_numaker_driver_api = {
	.transceive = spi_numaker_transceive,
	.release = spi_numaker_release
};

int spi_numaker_init(const struct device *dev)
{
	struct spi_numaker_data *data = dev->data;
	const struct spi_numaker_config *dev_cfg = dev->config;
	int err = 0;
    struct numaker_scc_subsys scc_subsys;
    
	SYS_UnlockReg();

    /* CLK controller */
    memset(&scc_subsys, 0x00, sizeof(scc_subsys));
    scc_subsys.subsys_id        = NUMAKER_SCC_SUBSYS_ID_PCC;
    scc_subsys.pcc.clk_modidx   = dev_cfg->clk_modidx;
    scc_subsys.pcc.clk_src      = dev_cfg->clk_src;
    scc_subsys.pcc.clk_div      = dev_cfg->clk_div;

    /* Equivalent to CLK_EnableModuleClock() */
    err = clock_control_on(dev_cfg->clk_dev, (clock_control_subsys_t) &scc_subsys);
    if (err != 0) {
        goto move_exit;
    }
    /* Equivalent to CLK_SetModuleClock() */
    err = clock_control_configure(dev_cfg->clk_dev, (clock_control_subsys_t) &scc_subsys, NULL);
    if (err != 0) {
        goto move_exit;
    }

	err = pinctrl_apply_state(dev_cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to apply pinctrl state");
        goto move_exit;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
        goto move_exit;
	}

	spi_context_unlock_unconditionally(&data->ctx);
    
    // Reset this module
    SYS_ResetModule(dev_cfg->id_rst);
    
move_exit:
	SYS_LockReg();
	return err;

}

#define NUMAKER_SPI_INIT(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);						\
	static struct spi_numaker_data spi_numaker_data_##inst = {			\
		SPI_CONTEXT_INIT_LOCK(spi_numaker_data_##inst, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_numaker_data_##inst, ctx),		\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)		\
	};									\
	static struct spi_numaker_config spi_numaker_config_##inst = {			\
		.spi = (SPI_T *)DT_INST_REG_ADDR(inst),					\
        .is_qspi = DT_INST_NODE_HAS_PROP(inst, qspi),					\
        .id_rst = DT_INST_PROP(inst, reset),					\
        .clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),    \
        .clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),          \
        .clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),    \
	    .clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))), \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			        \
	};									\
	DEVICE_DT_INST_DEFINE(inst, &spi_numaker_init, NULL, &spi_numaker_data_##inst,	\
			      &spi_numaker_config_##inst, POST_KERNEL,		\
			      CONFIG_SPI_INIT_PRIORITY, &spi_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_SPI_INIT)
