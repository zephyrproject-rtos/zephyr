/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_fiu_qspi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash/npcx_flash_api_ex.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/flash_controller/npcx_fiu_qspi.h>
#include <soc.h>

#include "flash_npcx_fiu_qspi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npcx_fiu_qspi, CONFIG_FLASH_LOG_LEVEL);

/* Driver convenience defines */
#define HAL_INSTANCE(dev) \
	((struct fiu_reg *)((const struct npcx_qspi_fiu_config *)(dev)->config)->base)

/* Device config */
struct npcx_qspi_fiu_config {
	/* Flash interface unit base address */
	uintptr_t base;
	/* Clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* Enable 2 external SPI devices for direct read on QSPI bus */
	bool en_direct_access_2dev;
	bool base_flash_inv;
};

/* Device data */
struct npcx_qspi_fiu_data {
	/* mutex of qspi bus controller */
	struct k_sem lock_sem;
	/* Current device configuration on QSPI bus */
	const struct npcx_qspi_cfg *cur_cfg;
	/* Current Software controlled Chip-Select number */
	int sw_cs;
	/* Current QSPI bus operation */
	uint32_t operation;
};

/* NPCX SPI User Mode Access (UMA) functions */
static inline void qspi_npcx_uma_cs_level(const struct device *dev, uint8_t sw_cs, bool level)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level) {
		inst->UMA_ECTS |= BIT(sw_cs);
	} else {
		inst->UMA_ECTS &= ~BIT(sw_cs);
	}
}

static inline void qspi_npcx_uma_write_byte(const struct device *dev, uint8_t data)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Set data to UMA_CODE and trigger UMA */
	inst->UMA_CODE = data;
	inst->UMA_CTS = UMA_CODE_CMD_WR_ONLY;
	/* EXEC_DONE will be zero automatically if a UMA transaction is completed. */
	while (IS_BIT_SET(inst->UMA_CTS, NPCX_UMA_CTS_EXEC_DONE)) {
		continue;
	}
}

static inline void qspi_npcx_uma_read_byte(const struct device *dev, uint8_t *data)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Trigger UMA and Get data from DB0 later */
	inst->UMA_CTS = UMA_CODE_RD_BYTE(1);
	while (IS_BIT_SET(inst->UMA_CTS, NPCX_UMA_CTS_EXEC_DONE)) {
		continue;
	}

	*data = inst->UMA_DB0;
}

/* NPCX SPI Direct Read Access (DRA)/User Mode Access (UMA) configuration functions */
static inline void qspi_npcx_config_uma_mode(const struct device *dev,
					     const struct npcx_qspi_cfg *qspi_cfg)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	if ((qspi_cfg->flags & NPCX_QSPI_SEC_FLASH_SL) != 0) {
		inst->UMA_ECTS |= BIT(NPCX_UMA_ECTS_SEC_CS);
	} else {
		inst->UMA_ECTS &= ~BIT(NPCX_UMA_ECTS_SEC_CS);
	}
}

static inline void qspi_npcx_config_dra_4byte_mode(const struct device *dev,
						   const struct npcx_qspi_cfg *qspi_cfg)
{
#if defined(CONFIG_FLASH_NPCX_FIU_SUPP_DRA_4B_ADDR)
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

#if defined(CONFIG_FLASH_NPCX_FIU_DRA_V1)
	if (qspi_cfg->enter_4ba != 0) {
		if ((qspi_cfg->flags & NPCX_QSPI_SEC_FLASH_SL) != 0) {
			inst->SPI1_DEV |= BIT(NPCX_SPI1_DEV_FOUR_BADDR_CS11);
		} else {
			inst->SPI1_DEV |= BIT(NPCX_SPI1_DEV_FOUR_BADDR_CS10);
		}
	} else {
		inst->SPI1_DEV &= ~(BIT(NPCX_SPI1_DEV_FOUR_BADDR_CS11) |
				    BIT(NPCX_SPI1_DEV_FOUR_BADDR_CS10));
	}
#elif defined(CONFIG_FLASH_NPCX_FIU_DRA_V2)
	if (qspi_cfg->enter_4ba != 0) {
		SET_FIELD(inst->SPI_DEV, NPCX_SPI_DEV_NADDRB, NPCX_DEV_NUM_ADDR_4BYTE);
	}
#endif
#endif /* CONFIG_FLASH_NPCX_FIU_SUPP_DRA_4B_ADDR */
}

static inline void qspi_npcx_config_dra_mode(const struct device *dev,
					     const struct npcx_qspi_cfg *qspi_cfg)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Select SPI device number for DRA mode in npcx4 series */
	if (IS_ENABLED(CONFIG_FLASH_NPCX_FIU_DRA_V2)) {
		int spi_dev_num = (qspi_cfg->flags & NPCX_QSPI_SEC_FLASH_SL) != 0 ? 1 : 0;

		SET_FIELD(inst->BURST_CFG, NPCX_BURST_CFG_SPI_DEV_SEL, spi_dev_num);
	}

	/* Enable quad mode of Direct Read Mode if needed */
	if (qspi_cfg->qer_type != JESD216_DW15_QER_NONE) {
		inst->RESP_CFG |= BIT(NPCX_RESP_CFG_QUAD_EN);
	} else {
		inst->RESP_CFG &= ~BIT(NPCX_RESP_CFG_QUAD_EN);
	}

	/* Selects the SPI read access type of Direct Read Access mode */
	SET_FIELD(inst->SPI_FL_CFG, NPCX_SPI_FL_CFG_RD_MODE, qspi_cfg->rd_mode);

	/* Enable/Disable 4 byte address mode for Direct Read Access (DRA) */
	qspi_npcx_config_dra_4byte_mode(dev, qspi_cfg);
}

static inline void qspi_npcx_fiu_set_operation(const struct device *dev, uint32_t operation)
{
	if ((operation & NPCX_EX_OP_INT_FLASH_WP) != 0) {
		npcx_pinctrl_flash_write_protect_set();
	}
}

/* NPCX specific QSPI-FIU controller functions */
int qspi_npcx_fiu_uma_transceive(const struct device *dev, struct npcx_uma_cfg *cfg,
				     uint32_t flags)
{
	struct npcx_qspi_fiu_data *const data = dev->data;

	/* UMA transaction is permitted? */
	if ((data->operation & NPCX_EX_OP_LOCK_UMA) != 0) {
		return -EPERM;
	}

	/* Assert chip select */
	qspi_npcx_uma_cs_level(dev, data->sw_cs, false);

	/* Transmit op-code first */
	qspi_npcx_uma_write_byte(dev, cfg->opcode);

	if ((flags & NPCX_UMA_ACCESS_ADDR) != 0) {
		/* 3-byte or 4-byte address? */
		const int addr_start = (data->cur_cfg->enter_4ba != 0) ? 0 : 1;

		for (size_t i = addr_start; i < 4; i++) {
			LOG_DBG("addr %d, %02x", i, cfg->addr.u8[i]);
			qspi_npcx_uma_write_byte(dev, cfg->addr.u8[i]);
		}
	}

	if ((flags & NPCX_UMA_ACCESS_WRITE) != 0) {
		if (cfg->tx_buf == NULL) {
			return -EINVAL;
		}
		for (size_t i = 0; i < cfg->tx_count; i++) {
			qspi_npcx_uma_write_byte(dev, cfg->tx_buf[i]);
		}
	}

	if ((flags & NPCX_UMA_ACCESS_READ) != 0) {
		if (cfg->rx_buf == NULL) {
			return -EINVAL;
		}
		for (size_t i = 0; i < cfg->rx_count; i++) {
			qspi_npcx_uma_read_byte(dev, cfg->rx_buf + i);
		}
	}

	/* De-assert chip select */
	qspi_npcx_uma_cs_level(dev, data->sw_cs, true);

	return 0;
}

void qspi_npcx_fiu_mutex_lock_configure(const struct device *dev,
					const struct npcx_qspi_cfg *cfg,
					const uint32_t operation)
{
	struct npcx_qspi_fiu_data *const data = dev->data;

	k_sem_take(&data->lock_sem, K_FOREVER);

	/* If the current device is different from previous one, configure it */
	if (data->cur_cfg != cfg) {
		data->cur_cfg = cfg;

		/* Apply pin-muxing and tri-state */
		pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

		/* Configure User Mode Access (UMA) settings */
		qspi_npcx_config_uma_mode(dev, cfg);

		/* Configure for Direct Read Access (DRA) settings */
		qspi_npcx_config_dra_mode(dev, cfg);

		/* Save SW CS bit used in UMA mode */
		data->sw_cs = find_lsb_set(cfg->flags & NPCX_QSPI_SW_CS_MASK) - 1;
	}

	/* Set QSPI bus operation */
	if (data->operation != operation) {
		qspi_npcx_fiu_set_operation(dev, operation);
		data->operation = operation;
	}
}

void qspi_npcx_fiu_mutex_unlock(const struct device *dev)
{
	struct npcx_qspi_fiu_data *const data = dev->data;

	k_sem_give(&data->lock_sem);
}

#if defined(CONFIG_FLASH_NPCX_FIU_DRA_V2)
void qspi_npcx_fiu_set_spi_size(const struct device *dev, const struct npcx_qspi_cfg *cfg)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	uint8_t flags = cfg->flags;

	if (cfg->spi_dev_sz <= NPCX_SPI_DEV_SIZE_128M) {
		if ((flags & NPCX_QSPI_SEC_FLASH_SL) == 0) {
			SET_FIELD(inst->BURST_CFG, NPCX_BURST_CFG_SPI_DEV_SEL, NPCX_SPI_F_CS0);
		} else {
			SET_FIELD(inst->BURST_CFG, NPCX_BURST_CFG_SPI_DEV_SEL, NPCX_SPI_F_CS1);
		}
		inst->SPI_DEV_SIZE = BIT(cfg->spi_dev_sz);
	} else {
		LOG_ERR("Invalid setting of low device size");
	}
}
#endif

static int qspi_npcx_fiu_init(const struct device *dev)
{
	const struct npcx_qspi_fiu_config *const config = dev->config;
	struct npcx_qspi_fiu_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on FIU clock fail %d", ret);
		return ret;
	}

	/* initialize mutex for qspi controller */
	k_sem_init(&data->lock_sem, 1, 1);

	/* Enable direct access for 2 external SPI devices */
	if (config->en_direct_access_2dev) {
#if defined(CONFIG_FLASH_NPCX_FIU_SUPP_DRA_2_DEV)
		struct fiu_reg *const inst = HAL_INSTANCE(dev);

		inst->FIU_EXT_CFG |= BIT(NPCX_FIU_EXT_CFG_SPI1_2DEV);
#if defined(CONFIG_FLASH_NPCX_FIU_SUPP_LOW_DEV_SWAP)
		if (config->base_flash_inv) {
			inst->FIU_EXT_CFG |= BIT(NPCX_FIU_EXT_CFG_LOW_DEV_NUM);
		}
#endif
#endif
	}

	return 0;
}

#define NPCX_SPI_FIU_INIT(n)							\
static const struct npcx_qspi_fiu_config npcx_qspi_fiu_config_##n = {		\
	.base = DT_INST_REG_ADDR(n),						\
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(n),					\
	.en_direct_access_2dev = DT_INST_PROP(n, en_direct_access_2dev),	\
	.base_flash_inv = DT_INST_PROP(n, flash_dev_inv),			\
};										\
static struct npcx_qspi_fiu_data npcx_qspi_fiu_data_##n;			\
DEVICE_DT_INST_DEFINE(n, qspi_npcx_fiu_init, NULL,				\
		      &npcx_qspi_fiu_data_##n, &npcx_qspi_fiu_config_##n,	\
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NPCX_SPI_FIU_INIT)
