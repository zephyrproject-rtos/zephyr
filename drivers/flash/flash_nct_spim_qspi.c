/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_spim_qspi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash/nct_flash_api_ex.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/flash_controller/nct_qspi.h>
#include <soc.h>

#include "spi_nor.h"
#include "flash_nct_qspi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nct_spim_qspi, LOG_LEVEL_ERR);

#ifdef CONFIG_XIP
	#include <zephyr/linker/linker-defs.h>
	#define RAMFUNC __attribute__((section(".ramfunc")))
#else
	#define RAMFUNC
#endif

#define NCT_SPIM_MAX_FREQ  	MHZ(50)
#define NCT_SPIM_CLK_DIVIDER   0x1

/* Driver convenience defines */
#define HAL_INSTANCE(dev) \
	((struct spim_reg *)((struct nct_qspi_spim_config *)(dev)->config)->base)

/* Device config */
struct nct_qspi_spim_config {
	/* Flash controller base address */
	uintptr_t base;
	/* Clock configuration */
	uint32_t clk_cfg;
};

#define SPIM_CTL0_DIRECT(cmd)	(((cmd) << 24) | \
								((NCT_SPIM_CTL0_OPMODE_DMM) << 22) | \
								BIT(NCT_SPIM_CTL0_CIPHOFF));

#define SPIM_CTL0_NORMAL(bit_mode, output, bitwidth, len) \
										((NCT_SPIM_CTL0_OPMODE_NORMAL_IO << 22) | \
										((bit_mode) << 20) | \
										((output) << NCT_SPIM_CTL0_QDIODIR) | \
										(((len) - 1) << 13) | \
										((bitwidth-1) << 8) | \
										BIT(NCT_SPIM_CTL0_CIPHOFF));

/* NCT SPI Normal functions */
RAMFUNC static inline void qspi_nct_normal_cs_level(const struct device *dev, uint8_t sw_cs, bool level)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level) {
		inst->SPIM_CTL1 |= BIT(NCT_SPIM_CTL1_SS);
	} else {
		inst->SPIM_CTL1 &= ~BIT(NCT_SPIM_CTL1_SS);
	}
}

RAMFUNC static inline void qspi_nct_spim_cache_on(const struct device *dev)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);

	inst->SPIM_CTL1 &= ~BIT(NCT_SPIM_CTL1_CACHEOFF);
}

RAMFUNC static inline void qspi_nct_spim_cache_invalid(const struct device *dev)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);

	inst->SPIM_CTL1 |= BIT(NCT_SPIM_CTL1_CDINVAL);
	while (inst->SPIM_CTL1 & BIT(NCT_SPIM_CTL1_CDINVAL)) {
		continue;
	}
}

RAMFUNC static void qspi_nct_normal_write_bytes(const struct device *dev, uint8_t* data, size_t len)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint8_t tx_len;

	while(len)
	{
		tx_len = (len > 4)?(4):(len);
		len -= tx_len;

		inst->SPIM_CTL0 = SPIM_CTL0_NORMAL(NCT_SPIM_CTL0_BITMODE_STANDARD, 1, 8, tx_len);

	    while(tx_len)
	    {
	    	inst->SPIM_TX[--tx_len] = *data++;
	    }

		/* execute transaction */
		inst->SPIM_CTL1 |= BIT(NCT_SPIM_CTL1_SPIMEN);

		/* SPIMEM will be zero automatically if a transaction is completed. */
		while (inst->SPIM_CTL1 & BIT(NCT_SPIM_CTL1_SPIMEN)) {
			continue;
		}
	}
}

RAMFUNC static void qspi_nct_normal_write_byte(const struct device *dev, uint8_t data)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);

	inst->SPIM_CTL0 = SPIM_CTL0_NORMAL(NCT_SPIM_CTL0_BITMODE_STANDARD, 1, 8, 1);

	inst->SPIM_TX[0] = data;

	/* execute transaction */
	inst->SPIM_CTL1 |= BIT(NCT_SPIM_CTL1_SPIMEN);

	/* SPIMEM will be zero automatically if a transaction is completed. */
	while (inst->SPIM_CTL1 & BIT(NCT_SPIM_CTL1_SPIMEN)) {
		continue;
	}
}

RAMFUNC static void qspi_nct_normal_read_bytes(const struct device *dev, uint8_t *data, size_t len)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint8_t rx_len;

	while(len)
	{
		rx_len = (len > 4)?(4):(len);
		len -= rx_len;

		/* update control register */
		inst->SPIM_CTL0 = SPIM_CTL0_NORMAL(NCT_SPIM_CTL0_BITMODE_STANDARD, 0, 8, rx_len);

		/* execute transaction */
		inst->SPIM_CTL1 |= BIT(NCT_SPIM_CTL1_SPIMEN);

		/* SPIMEM will be zero automatically if a transaction is completed. */
		while (inst->SPIM_CTL1 & BIT(NCT_SPIM_CTL1_SPIMEN)) {
			continue;
		}

        while(rx_len) {
            *data++ = inst->SPIM_RX[--rx_len];
        }
	}
}

RAMFUNC static inline void qspi_nct_config_normal_mode(const struct device *dev,
					     const struct nct_qspi_cfg *qspi_cfg)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);

	inst->SPIM_CTL0 = SPIM_CTL0_NORMAL(NCT_SPIM_CTL0_BITMODE_STANDARD, 0, 8, 0);
}

RAMFUNC static inline void qspi_nct_config_dmm_mode(const struct device *dev,
					     const struct nct_qspi_cfg *qspi_cfg)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);

	switch (qspi_cfg->rd_mode) {
		case NCT_RD_MODE_NORMAL:
			inst->SPIM_CTL0 = SPIM_CTL0_DIRECT(SPI_NOR_CMD_READ);
			break;
		case NCT_RD_MODE_FAST:
			inst->SPIM_CTL0 = SPIM_CTL0_DIRECT(SPI_NOR_CMD_READ_FAST);
			break;
		case NCT_RD_MODE_FAST_DUAL:
			inst->SPIM_CTL0 = SPIM_CTL0_DIRECT(SPI_NOR_CMD_2READ);
			break;
		case NCT_RD_MODE_QUAD:
			inst->SPIM_CTL0 = SPIM_CTL0_DIRECT(SPI_NOR_CMD_4READ);
			break;
		default:
			LOG_ERR("un-support rd mode:%d", qspi_cfg->rd_mode);
			break;
	}
}

RAMFUNC static inline void qspi_nct_spim_set_operation(const struct device *dev, uint32_t operation)
{
	if ((operation & NCT_EX_OP_INT_FLASH_WP) != 0) {
		nct_pinctrl_flash_write_protect_set(NCT_SPIM_FLASH_WP);
	}
}

RAMFUNC static void qspi_nct_wait_flash_ready(const struct device *dev)
{
	uint8_t reg1_sts;

	/* check busy bit in status register 1 */
	do {
		qspi_nct_normal_cs_level(dev, 0, false);

		qspi_nct_normal_write_byte(dev, 0x05);
		qspi_nct_normal_read_bytes(dev, &reg1_sts, 1);

		qspi_nct_normal_cs_level(dev, 0, true);
	} while (reg1_sts & 0x01);
}

/* NCT specific QSPI-SPIM controller functions */
RAMFUNC static int qspi_nct_spim_normal_transceive(const struct device *dev, struct nct_transceive_cfg *cfg,
				     uint32_t flags)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	struct nct_qspi_data *const data = dev->data;
	uint32_t ctrl_value = 0;

	/* Transaction is permitted? */
	if ((data->operation & NCT_EX_OP_LOCK_TRANSCEIVE) != 0) {
		return -EPERM;
	}

	/* save ctrl0 setting */
	ctrl_value = inst->SPIM_CTL0;

	/* Config normal mode */
    qspi_nct_config_normal_mode(dev, data->cur_cfg);

	/* Assert chip select */
	qspi_nct_normal_cs_level(dev, 0, false);

	/* Transmit op-code first */
	qspi_nct_normal_write_byte(dev, cfg->opcode);

	if ((flags & NCT_TRANSCEIVE_ACCESS_ADDR) != 0) {
		qspi_nct_normal_write_bytes(dev, &cfg->addr.u8[1],	3);
	}

	if ((flags & NCT_TRANSCEIVE_ACCESS_WRITE) != 0) {
		if (cfg->tx_buf == NULL) {
			return -EINVAL;
		}

		qspi_nct_normal_write_bytes(dev, cfg->tx_buf, cfg->tx_count);
	}

	if ((flags & NCT_TRANSCEIVE_ACCESS_READ) != 0) {
		if (cfg->rx_buf == NULL) {
			return -EINVAL;
		}

		qspi_nct_normal_read_bytes(dev, cfg->rx_buf, cfg->rx_count);
	}

	/* De-assert chip select */
	qspi_nct_normal_cs_level(dev, 0, true);

	/* Wait flash ready during flash write operation. */
	if(((flags & NCT_TRANSCEIVE_ACCESS_READ) == 0) && (cfg->opcode != SPI_NOR_CMD_WREN)) {
		qspi_nct_wait_flash_ready(dev);
	}

	/* cache invalid after normal I/O mode */
	qspi_nct_spim_cache_invalid(dev);

	/* restore ctrl0 setting */
	inst->SPIM_CTL0 = ctrl_value;

	return 0;
}

RAMFUNC static void qspi_nct_spim_mutex_lock_configure(const struct device *dev,
					const struct nct_qspi_cfg *cfg,
					const uint32_t operation)
{
	struct nct_qspi_data *const data = dev->data;

	k_sem_take(&data->lock_sem, K_FOREVER);

	/* If the current device is different from previous one, configure it */
	if (data->cur_cfg != cfg) {
		data->cur_cfg = cfg;

		/* Apply pin-muxing and tri-state */
		pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

		/* Configure for Direct Memory Mapping (DMM) settings */
		qspi_nct_config_dmm_mode(dev, cfg);
	}

	/* Set QSPI bus operation */
	if (data->operation != operation) {
		qspi_nct_spim_set_operation(dev, operation);
		data->operation = operation;
	}
}

RAMFUNC static void qspi_nct_spim_mutex_unlock(const struct device *dev)
{
	struct nct_qspi_data *const data = dev->data;

	k_sem_give(&data->lock_sem);
}

struct nct_qspi_ops nct_qspi_spim_ops = {
	.lock_configure = qspi_nct_spim_mutex_lock_configure,
        .unlock = qspi_nct_spim_mutex_unlock,
        .transceive = qspi_nct_spim_normal_transceive,
};

RAMFUNC static int qspi_nct_spim_init(struct device *dev)
{
	struct nct_qspi_spim_config * config = (void *)dev->config;
	struct nct_qspi_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t clock_rate;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on SPIM clock fail %d", ret);
		return ret;
	}

    if (clock_control_get_rate(clk_dev,
                (clock_control_subsys_t)config->clk_cfg,
                &clock_rate) < 0) {
        LOG_ERR("Get SPIM source clock fail");
        return -EIO;
    }

    /* Make SPIM frequency < NCT_SPIM_MAX_FREQ */
    if (clock_rate > NCT_SPIM_MAX_FREQ) {
        SET_FIELD(inst->SPIM_CTL1, NCT_SPIM_CTL1_DIVIDER,
                NCT_SPIM_CLK_DIVIDER);
    } else {
        SET_FIELD(inst->SPIM_CTL1, NCT_SPIM_CTL1_DIVIDER, 0);
    }

	/* initialize mutex for qspi controller */
	k_sem_init(&data->lock_sem, 1, 1);

	/* enable cache */
	qspi_nct_spim_cache_on(dev);

	return 0;
}

#define NCT_SPI_SPIM_INIT(n)							\
static struct nct_qspi_spim_config nct_qspi_spim_config_##n = {		\
	.base = DT_INST_REG_ADDR(n),						\
	.clk_cfg = DT_INST_PHA(n, clocks, clk_cfg),					\
};										\
static struct nct_qspi_data nct_qspi_data_##n = {				\
	.qspi_ops = &nct_qspi_spim_ops						\
};										\
DEVICE_DT_INST_DEFINE(n, qspi_nct_spim_init, NULL,				\
		      &nct_qspi_data_##n, &nct_qspi_spim_config_##n,	\
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NCT_SPI_SPIM_INIT)
