/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_fiu_qspi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash/npcm_flash_api_ex.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/flash_controller/npcm_qspi.h>
#include <soc.h>

#include "flash_npcm_qspi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npcm_fiu_qspi, LOG_LEVEL_ERR);

#define NPCM_FIU_CHK_TIMEOUT_US	10000

#define NPCM_FIU_PVT_CS		NPCM_QSPI_SW_CS0
#define NPCM_FIU_SHD_CS		NPCM_QSPI_SW_CS1
#define NPCM_FIU_BACK_CS	NPCM_QSPI_SW_CS2

/* Driver convenience defines */
#define HAL_INSTANCE(dev) \
	((struct fiu_reg *)((const struct npcm_qspi_fiu_config *)(dev)->config)->core_base)
#define HAL_HOST_INSTANCE(dev) \
	((struct fiu_reg *)((const struct npcm_qspi_fiu_config *)(dev)->config)->host_base)

/* Device config */
struct npcm_qspi_fiu_config {
	/* Flash controller core base address */
	uintptr_t core_base;
	/* Flash controller host base address */
	uintptr_t host_base;
	/* Clock configuration */
	uint32_t clk_cfg;
};

/* NPCM SPI User Mode Access (UMA) functions */
static inline void qspi_npcm_uma_cs_level(const struct device *dev, uint8_t sw_cs, bool level)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level) {
		inst->UMA_ECTS |= BIT(sw_cs);
	} else {
		inst->UMA_ECTS &= ~BIT(sw_cs);
	}
}

static inline void qspi_npcm_uma_write_byte(const struct device *dev, uint8_t data)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	struct npcm_qspi_data *const qspi_data = dev->data;
	const struct npcm_qspi_cfg *qspi_cfg = qspi_data->cur_cfg;
	int cts = 0;

	/* Set data to UMA_CODE and trigger UMA */
	inst->UMA_CODE = data;

	cts = UMA_CODE_ONLY_WRITE;

	/* share flash select, otherwise pvt or back */
	if (qspi_cfg->flags & NPCM_FIU_SHD_CS) {
		cts |= UMA_FLD_SHD_SL;
	} else {
		cts &= ~UMA_FLD_SHD_SL;
	}

	inst->UMA_CTS = cts;

	/* EXEC_DONE will be zero automatically if a UMA transaction is completed. */
	while (IS_BIT_SET(inst->UMA_CTS, NPCM_UMA_CTS_EXEC_DONE)) {
		continue;
	}
}

static inline void qspi_npcm_uma_read_byte(const struct device *dev, uint8_t *data)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	struct npcm_qspi_data *const qspi_data = dev->data;
	const struct npcm_qspi_cfg *qspi_cfg = qspi_data->cur_cfg;
	int cts = 0;

	cts = UMA_CODE_ONLY_READ_BYTE(1);

	/* share flash select, otherwise pvt or back */
	if (qspi_cfg->flags & NPCM_FIU_SHD_CS) {
		cts |= UMA_FLD_SHD_SL;
	} else {
		cts &= ~UMA_FLD_SHD_SL;
	}

	/* Trigger UMA and Get data from DB0 later */
	inst->UMA_CTS = cts;

	while (IS_BIT_SET(inst->UMA_CTS, NPCM_UMA_CTS_EXEC_DONE)) {
		continue;
	}

	*data = inst->UMA_DB0;
}

/* NPCM SPI Direct Read Access (DRA)/User Mode Access (UMA) configuration functions */
static inline void qspi_npcm_config_uma_mode(const struct device *dev,
					     const struct npcm_qspi_cfg *qspi_cfg)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* back flash select, otherwise share or pvt */
	if (qspi_cfg->flags & NPCM_FIU_BACK_CS) {
		inst->UMA_ECTS |= BIT(NPCM_UMA_ECTS_DEV_NUM_BACK);
	} else {
		inst->UMA_ECTS &= ~BIT(NPCM_UMA_ECTS_DEV_NUM_BACK);
	}
}

static inline void qspi_npcm_config_dra_4byte_mode(const struct device *dev,
						   const struct npcm_qspi_cfg *qspi_cfg)
{
#if defined(CONFIG_FLASH_NPCM_FIU_SUPP_DRA_4B_ADDR)
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);
	uint8_t addr_4b_en;

	addr_4b_en = (qspi_cfg->flags & NPCM_QSPI_SW_CS_MASK) << 4;

	if (qspi_cfg->enter_4ba != 0) {
		core_inst->ADDR_4B_EN |= addr_4b_en;
		host_inst->ADDR_4B_EN |= addr_4b_en;
	} else {
		core_inst->ADDR_4B_EN &= ~addr_4b_en;
		host_inst->ADDR_4B_EN &= ~addr_4b_en;
	}
#endif /* CONFIG_FLASH_NPCM_FIU_SUPP_DRA_4B_ADDR */
}

static inline void qspi_npcm_config_dra_mode(const struct device *dev,
					     const struct npcm_qspi_cfg *qspi_cfg)
{
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);
	uint8_t rd_mode, rd_burst = NPCM_BURST_CFG_R_BURST_16B;

	switch (qspi_cfg->rd_mode) {
		case NPCM_RD_MODE_NORMAL:
			rd_mode = NPCM_SPI_FL_CFG_RD_MODE_NORMAL;
			break;
		case NPCM_RD_MODE_FAST:
			rd_mode = NPCM_SPI_FL_CFG_RD_MODE_FAST;
			break;
		case NPCM_RD_MODE_FAST_DUAL:
		case NPCM_RD_MODE_QUAD:
			rd_mode = NPCM_SPI_FL_CFG_RD_MODE_FAST_DUAL;
			break;
                default:
			LOG_ERR("un-support rd mode:%d", qspi_cfg->rd_mode);
			return;
	}

	/* Selects the SPI read access type of Direct Read Access mode,
	 * for quad mode, need enable extra configuration.
	 */
	SET_FIELD(core_inst->SPI_FL_CFG, NPCM_SPI_FL_CFG_RD_MODE, rd_mode);
	SET_FIELD(host_inst->SPI_FL_CFG, NPCM_SPI_FL_CFG_RD_MODE, rd_mode);

	if (qspi_cfg->rd_mode == NPCM_RD_MODE_QUAD) {
		core_inst->RESP_CFG |= BIT(NPCM_RESP_CFG_QUAD_EN);
		host_inst->RESP_CFG |= BIT(NPCM_RESP_CFG_QUAD_EN);
	} else {
		core_inst->RESP_CFG &= ~BIT(NPCM_RESP_CFG_QUAD_EN);
		host_inst->RESP_CFG &= ~BIT(NPCM_RESP_CFG_QUAD_EN);
	}

	/* set read max burst 16 bytes */
	SET_FIELD(core_inst->BURST_CFG, NPCM_BURST_CFG_R_BURST, rd_burst);
	SET_FIELD(host_inst->BURST_CFG, NPCM_BURST_CFG_R_BURST, rd_burst);

	/* Enable/Disable 4 byte address mode for Direct Read Access (DRA) */
	qspi_npcm_config_dra_4byte_mode(dev, qspi_cfg);
}

static inline void qspi_npcm_fiu_set_operation(const struct device *dev, uint32_t operation)
{
	if ((operation & NPCM_EX_OP_EXT_FLASH_WP) != 0) {
		npcm_pinctrl_flash_write_protect_set(NPCM_FIU_FLASH_WP);
	}
}

static inline int qspi_npcm_fiu_uma_lock(const struct device *dev)
{
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);

	if (WAIT_FOR(IS_BIT_SET(host_inst->FIU_MSR_STS, NPCM_FIU_MSR_STS_MSTR_INACT),
			NPCM_FIU_CHK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("wait host fiu inactive timeout");
		return -ETIMEDOUT;
	}

	core_inst->FIU_MSR_IE_CFG |= BIT(NPCM_FIU_MSR_IE_CFG_UMA_BLOCK);

	return 0;
}

static inline void qspi_npcm_fiu_uma_release(const struct device *dev)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	inst->FIU_MSR_IE_CFG &= ~BIT(NPCM_FIU_MSR_IE_CFG_UMA_BLOCK);
}

/* NPCM specific QSPI-FIU controller functions */
static int qspi_npcm_fiu_uma_transceive(const struct device *dev, struct npcm_transceive_cfg *cfg,
				     uint32_t flags)
{
	struct npcm_qspi_data *const data = dev->data;
	int ret;

	/* Transaction is permitted? */
	if ((data->operation & NPCM_EX_OP_LOCK_TRANSCEIVE) != 0) {
		return -EPERM;
	}

	/* UMA block */
	ret = qspi_npcm_fiu_uma_lock(dev);
	if (ret) {
		return ret;
	}

	/* Assert chip select */
	qspi_npcm_uma_cs_level(dev, data->sw_cs, false);

	/* Transmit op-code first */
	qspi_npcm_uma_write_byte(dev, cfg->opcode);

	if ((flags & NPCM_TRANSCEIVE_ACCESS_ADDR) != 0) {
		/* 3-byte or 4-byte address? */
		const int addr_start = (data->cur_cfg->enter_4ba != 0) ? 0 : 1;

		for (size_t i = addr_start; i < 4; i++) {
			LOG_DBG("addr %d, %02x", i, cfg->addr.u8[i]);
			qspi_npcm_uma_write_byte(dev, cfg->addr.u8[i]);
		}
	}

	if ((flags & NPCM_TRANSCEIVE_ACCESS_WRITE) != 0) {
		if (cfg->tx_buf == NULL) {
			return -EINVAL;
		}
		for (size_t i = 0; i < cfg->tx_count; i++) {
			qspi_npcm_uma_write_byte(dev, cfg->tx_buf[i]);
		}
	}

	if ((flags & NPCM_TRANSCEIVE_ACCESS_READ) != 0) {
		if (cfg->rx_buf == NULL) {
			return -EINVAL;
		}
		for (size_t i = 0; i < cfg->rx_count; i++) {
			qspi_npcm_uma_read_byte(dev, cfg->rx_buf + i);
		}
	}

	/* De-assert chip select */
	qspi_npcm_uma_cs_level(dev, data->sw_cs, true);

	/* UMA unblock */
	qspi_npcm_fiu_uma_release(dev);

	return 0;
}

static void qspi_npcm_fiu_mutex_lock_configure(const struct device *dev,
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

		/* Save SW CS bit used in UMA mode */
		data->sw_cs = find_lsb_set(cfg->flags & NPCM_QSPI_SW_CS_MASK) - 1;

		/* Configure User Mode Access (UMA) settings */
		qspi_npcm_config_uma_mode(dev, cfg);

		/* Configure for Direct Read Access (DRA) settings */
		qspi_npcm_config_dra_mode(dev, cfg);
	}

	/* Set QSPI bus operation */
	if (data->operation != operation) {
		qspi_npcm_fiu_set_operation(dev, operation);
		data->operation = operation;
	}
}

static void qspi_npcm_fiu_mutex_unlock(const struct device *dev)
{
	struct npcm_qspi_data *const data = dev->data;

	k_sem_give(&data->lock_sem);
}

struct npcm_qspi_ops npcm_qspi_fiu_ops = {
	.lock_configure = qspi_npcm_fiu_mutex_lock_configure,
        .unlock = qspi_npcm_fiu_mutex_unlock,
        .transceive = qspi_npcm_fiu_uma_transceive,
};

static int qspi_npcm_fiu_init(const struct device *dev)
{
	const struct npcm_qspi_fiu_config *const config = dev->config;
	struct npcm_qspi_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on FIU clock fail %d", ret);
		return ret;
	}

	/* initialize mutex for qspi controller */
	k_sem_init(&data->lock_sem, 1, 1);

	return 0;
}

#define NPCM_SPI_FIU_INIT(n)							\
static const struct npcm_qspi_fiu_config npcm_qspi_fiu_config_##n = {		\
	.core_base = DT_INST_REG_ADDR_BY_IDX(n, 0),				\
	.host_base = DT_INST_REG_ADDR_BY_IDX(n, 1),				\
	.clk_cfg = DT_INST_PHA(n, clocks, clk_cfg),				\
};										\
static struct npcm_qspi_data npcm_qspi_data_##n = {				\
	.qspi_ops = &npcm_qspi_fiu_ops						\
};										\
DEVICE_DT_INST_DEFINE(n, qspi_npcm_fiu_init, NULL,				\
		      &npcm_qspi_data_##n, &npcm_qspi_fiu_config_##n,	\
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NPCM_SPI_FIU_INIT)
