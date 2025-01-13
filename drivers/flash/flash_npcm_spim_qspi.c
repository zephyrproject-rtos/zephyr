/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_spim_qspi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash/npcm_flash_api_ex.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/flash_controller/npcm_qspi.h>
#include <soc.h>

#include "spi_nor.h"
#include "flash_npcm_qspi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npcm_spim_qspi, LOG_LEVEL_ERR);

#define NPCM_SPIM_INT_CS	NPCM_QSPI_SW_CS0

#define NPCM_SPIM_MAX_FREQ	MHZ(50)
#define NPCM_SPIM_CLK_DIVIDER	0x1

/* Driver convenience defines */
#define HAL_INSTANCE(dev) \
	((struct spim_reg *)((const struct npcm_qspi_spim_config *)(dev)->config)->base)

/* Device config */
struct npcm_qspi_spim_config {
	/* Flash controller base address */
	uintptr_t base;
	/* Clock configuration */
	struct npcm_clk_cfg clk_cfg;
};

/* NPCM SPI Normal functions */
static inline void qspi_npcm_normal_cs_level(const struct device *dev, uint8_t sw_cs, bool level)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level) {
		inst->SPIM_CTL1 |= BIT(NPCM_SPIM_CTL1_SS);
	} else {
		inst->SPIM_CTL1 &= ~BIT(NPCM_SPIM_CTL1_SS);
	}
}

static void qspi_npcm_spim_cache_on(const struct device *dev)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t ctrl_value = 0;

	ctrl_value = inst->SPIM_CTL1;
	ctrl_value &= ~BIT(NPCM_SPIM_CTL1_CACHEOFF);
	inst->SPIM_CTL1 = ctrl_value;
}

static void qspi_npcm_spim_cache_invalid(const struct device *dev)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t ctrl_value = 0;

	ctrl_value = inst->SPIM_CTL1;
	ctrl_value |= BIT(NPCM_SPIM_CTL1_CDINVAL);
	inst->SPIM_CTL1 = ctrl_value;

	while (inst->SPIM_CTL1 & BIT(NPCM_SPIM_CTL1_CDINVAL)) {
		continue;
	}
}

static inline void qspi_npcm_normal_write_byte(const struct device *dev, uint8_t data)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t ctrl_value = 0;

	ctrl_value = inst->SPIM_CTL0;

	/* output direction */
	ctrl_value |= BIT(NPCM_SPIM_CTL0_QDIODIR);

	/* update control register */
	inst->SPIM_CTL0 = ctrl_value;

	/* fill data */
	inst->SPIM_TX0 = data;

	/* execute transaction */
	inst->SPIM_CTL1 |= BIT(NPCM_SPIM_CTL1_SPIMEN);

	/* SPIMEM will be zero automatically if a transaction is completed. */
	while (inst->SPIM_CTL1 & BIT(NPCM_SPIM_CTL1_SPIMEN)) {
		continue;
	}

	/* write 1 to clear interupt flag */
	inst->SPIM_CTL0 |= BIT(NPCM_SPIM_CTL0_IF);
}

static inline void qspi_npcm_normal_read_byte(const struct device *dev, uint8_t *data)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t ctrl_value = 0;

	ctrl_value = inst->SPIM_CTL0;

	/* input direction */
	ctrl_value &= ~BIT(NPCM_SPIM_CTL0_QDIODIR);

	/* update control register */
	inst->SPIM_CTL0 = ctrl_value;

	/* execute transaction */
	inst->SPIM_CTL1 |= BIT(NPCM_SPIM_CTL1_SPIMEN);

	/* SPIMEM will be zero automatically if a transaction is completed. */
	while (inst->SPIM_CTL1 & BIT(NPCM_SPIM_CTL1_SPIMEN)) {
		continue;
	}

	/* write 1 to clear interupt flag */
	inst->SPIM_CTL0 |= BIT(NPCM_SPIM_CTL0_IF);

	/* fill read data */
	*data = inst->SPIM_RX0;
}

static inline void qspi_npcm_config_normal_mode(const struct device *dev,
					     const struct npcm_qspi_cfg *qspi_cfg)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t ctrl_value = 0;

	ctrl_value = inst->SPIM_CTL0;

	/* normal io mode */
	SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_OPMODE,
			NPCM_SPIM_CTL0_OPMODE_NORMAL_IO);

	/* bit mode to standard */
	SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_BITMODE,
			NPCM_SPIM_CTL0_BITMODE_STANDARD);

	/* data transmit width 8 bits */
	SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_DWIDTH,
			NPCM_SPIM_CTL0_DWIDTH_8);

	/* transaction one byte in one transfer */
	SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_BURSTNUM,
			NPCM_SPIM_CTL0_BURSTNUM_1);

	/* update control register */
	inst->SPIM_CTL0 = ctrl_value;
}

static inline void qspi_npcm_config_dmm_mode(const struct device *dev,
					     const struct npcm_qspi_cfg *qspi_cfg)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t ctrl_value = 0;

	ctrl_value = inst->SPIM_CTL0;

	/* dmm mode */
	SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_OPMODE,
			NPCM_SPIM_CTL0_OPMODE_DMM);

	/* standard read */
	SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_CMDCODE,
			SPI_NOR_CMD_READ);

	switch (qspi_cfg->rd_mode) {
		case NPCM_RD_MODE_NORMAL:
			SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_CMDCODE,
					SPI_NOR_CMD_READ);
			break;
		case NPCM_RD_MODE_FAST:
			SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_CMDCODE,
					SPI_NOR_CMD_READ_FAST);
			break;
		case NPCM_RD_MODE_FAST_DUAL:
			SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_CMDCODE,
					SPI_NOR_CMD_2READ);
			break;
		case NPCM_RD_MODE_QUAD:
			SET_FIELD(ctrl_value, NPCM_SPIM_CTL0_CMDCODE,
					SPI_NOR_CMD_4READ);
			break;
		default:
			LOG_ERR("un-support rd mode:%d", qspi_cfg->rd_mode);
			break;
	}

#if defined(CONFIG_FLASH_NPCM_SPIM_SUPP_DRA_4B_ADDR)
	if (qspi_cfg->enter_4ba != 0) {
		ctrl_value |= BIT(NPCM_SPIM_CTL0_B4ADDREN);
	} else {
		ctrl_value &= ~BIT(NPCM_SPIM_CTL0_B4ADDREN);
	}
#endif

	/* update control register */
	inst->SPIM_CTL0 = ctrl_value;
}

static inline void qspi_npcm_spim_set_operation(const struct device *dev, uint32_t operation)
{
	if ((operation & NPCM_EX_OP_INT_FLASH_WP) != 0) {
		npcm_pinctrl_flash_write_protect_set(NPCM_SPIM_FLASH_WP);
	}
}

/* NPCM specific QSPI-SPIM controller functions */
static int qspi_npcm_spim_normal_transceive(const struct device *dev, struct npcm_transceive_cfg *cfg,
				     uint32_t flags)
{
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	struct npcm_qspi_data *const data = dev->data;
	uint32_t ctrl_value = 0;

	/* Transaction is permitted? */
	if ((data->operation & NPCM_EX_OP_LOCK_TRANSCEIVE) != 0) {
		return -EPERM;
	}

	/* save ctrl0 setting */
	ctrl_value = inst->SPIM_CTL0;

	/* Config normal mode */
        qspi_npcm_config_normal_mode(dev, data->cur_cfg);

	/* Assert chip select */
	qspi_npcm_normal_cs_level(dev, data->sw_cs, false);

	/* Transmit op-code first */
	qspi_npcm_normal_write_byte(dev, cfg->opcode);

	if ((flags & NPCM_TRANSCEIVE_ACCESS_ADDR) != 0) {
		/* 3-byte or 4-byte address? */
		const int addr_start = (data->cur_cfg->enter_4ba != 0) ? 0 : 1;

		for (size_t i = addr_start; i < 4; i++) {
			LOG_DBG("addr %d, %02x", i, cfg->addr.u8[i]);
			qspi_npcm_normal_write_byte(dev, cfg->addr.u8[i]);
		}
	}

	if ((flags & NPCM_TRANSCEIVE_ACCESS_WRITE) != 0) {
		if (cfg->tx_buf == NULL) {
			return -EINVAL;
		}
		for (size_t i = 0; i < cfg->tx_count; i++) {
			qspi_npcm_normal_write_byte(dev, cfg->tx_buf[i]);
		}
	}

	if ((flags & NPCM_TRANSCEIVE_ACCESS_READ) != 0) {
		if (cfg->rx_buf == NULL) {
			return -EINVAL;
		}
		for (size_t i = 0; i < cfg->rx_count; i++) {
			qspi_npcm_normal_read_byte(dev, cfg->rx_buf + i);
		}
	}

	/* De-assert chip select */
	qspi_npcm_normal_cs_level(dev, data->sw_cs, true);

	/* cache invalid after normal I/O mode */
	qspi_npcm_spim_cache_invalid(dev);

	/* restore ctrl0 setting */
	inst->SPIM_CTL0 = ctrl_value;

	return 0;
}

static void qspi_npcm_spim_mutex_lock_configure(const struct device *dev,
					const struct npcm_qspi_cfg *cfg,
					const uint32_t operation)
{
	struct npcm_qspi_data *const data = dev->data;

	k_sem_take(&data->lock_sem, K_FOREVER);

	/* If the current device is different from previous one, configure it */
	if (data->cur_cfg != cfg) {
		data->cur_cfg = cfg;

		/* Apply pin-muxing and tri-state */
		pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

		/* Save SW CS bit used in normal mode */
		data->sw_cs = find_lsb_set(cfg->flags & NPCM_QSPI_SW_CS_MASK) - 1;

		/* Configure for Direct Memory Mapping (DMM) settings */
		qspi_npcm_config_dmm_mode(dev, cfg);
	}

	/* Set QSPI bus operation */
	if (data->operation != operation) {
		qspi_npcm_spim_set_operation(dev, operation);
		data->operation = operation;
	}
}

static void qspi_npcm_spim_mutex_unlock(const struct device *dev)
{
	struct npcm_qspi_data *const data = dev->data;

	k_sem_give(&data->lock_sem);
}

struct npcm_qspi_ops npcm_qspi_spim_ops = {
	.lock_configure = qspi_npcm_spim_mutex_lock_configure,
        .unlock = qspi_npcm_spim_mutex_unlock,
        .transceive = qspi_npcm_spim_normal_transceive,
};

static int qspi_npcm_spim_init(const struct device *dev)
{
	const struct npcm_qspi_spim_config *const config = dev->config;
	struct npcm_qspi_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCM_CLK_CTRL_NODE);
	struct spim_reg *const inst = HAL_INSTANCE(dev);
	uint32_t clock_rate;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on SPIM clock fail %d", ret);
		return ret;
	}

	if (clock_control_get_rate(clk_dev, (clock_control_subsys_t)&config->clk_cfg,
				&clock_rate) < 0) {
		LOG_ERR("Get SPIM source clock fail");
		return -EIO;
	}

	/* Make SPIM frequency < NPCM_SPIM_MAX_FREQ */
	if (clock_rate > NPCM_SPIM_MAX_FREQ) {
		SET_FIELD(inst->SPIM_CTL1, NPCM_SPIM_CTL1_DIVIDER,
				NPCM_SPIM_CLK_DIVIDER);
	} else {
		SET_FIELD(inst->SPIM_CTL1, NPCM_SPIM_CTL1_DIVIDER, 0x0);
	}

	/* initialize mutex for qspi controller */
	k_sem_init(&data->lock_sem, 1, 1);

	/* enable cache */
	qspi_npcm_spim_cache_on(dev);

	return 0;
}

#define NPCM_SPI_SPIM_INIT(n)							\
static const struct npcm_qspi_spim_config npcm_qspi_spim_config_##n = {		\
	.base = DT_INST_REG_ADDR(n),						\
	.clk_cfg = NPCM_DT_CLK_CFG_ITEM(n),					\
};										\
static struct npcm_qspi_data npcm_qspi_data_##n = {				\
	.qspi_ops = &npcm_qspi_spim_ops						\
};										\
DEVICE_DT_INST_DEFINE(n, qspi_npcm_spim_init, NULL,				\
		      &npcm_qspi_data_##n, &npcm_qspi_spim_config_##n,	\
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NPCM_SPI_SPIM_INIT)
