/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_fiu_qspi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash/nct_flash_api_ex.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/flash_controller/nct_qspi.h>
#include <soc.h>

#include "spi_nor.h"
#include "flash_nct_qspi.h"
#include "gdma.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nct_fiu_qspi, LOG_LEVEL_ERR);

#define NCT_FIU_CHK_TIMEOUT_US	10000

#define NCT_FIU_PVT_CS		NCT_QSPI_SW_CS0
#define NCT_FIU_SHD_CS		NCT_QSPI_SW_CS1
#define NCT_FIU_BACK_CS	NCT_QSPI_SW_CS2

/* System configuration */
#define HAL_SFCG_INST() (struct scfg_reg *)(DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg))

/* Driver convenience defines */
#define HAL_INSTANCE(dev) \
	((struct fiu_reg *)((const struct nct_qspi_fiu_config *)(dev)->config)->core_base)
#define HAL_HOST_INSTANCE(dev) \
	((struct fiu_reg *)((const struct nct_qspi_fiu_config *)(dev)->config)->host_base)

/* Device config */
struct nct_qspi_fiu_config {
	/* Flash controller core base address */
	uintptr_t core_base;
	/* Flash controller host base address */
	uintptr_t host_base;
	/* Clock configuration */
	uint32_t clk_cfg;
};

/* NCT SPI User Mode Access (UMA) functions */
static inline void qspi_nct_uma_cs_level(const struct device *dev, uint8_t sw_cs, bool level)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level) {
		inst->UMA_ECTS |= sw_cs;
	} else {
		inst->UMA_ECTS &= ~sw_cs;
	}
}

static inline void qspi_nct_uma_write_byte(const struct device *dev, uint8_t data)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Set data to UMA_CODE and trigger UMA */
	inst->UMA_CODE = data;

	inst->UMA_CTS = UMA_CODE_ONLY_WRITE;

	/* EXEC_DONE will be zero automatically if a UMA transaction is completed. */
	while (IS_BIT_SET(inst->UMA_CTS, NCT_UMA_CTS_EXEC_DONE)) {
		continue;
	}
}

static inline void qspi_nct_uma_write_bytes(const struct device *dev, uint8_t* data, uint32_t len)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	uint32_t wr_len;

	while(len)
	{
		wr_len = (len > 16)?(16):(len);
		len -= wr_len;

		memcpy((void*)inst->EXT_DB_F_0, data, wr_len);
		data += wr_len;

		inst->EXT_DB_CFG = BIT(NCT_EXT_DB_CFG_EXT_DB_EN) | wr_len;
		inst->UMA_CTS = (UMA_FLD_EXEC | UMA_FLD_WRITE | UMA_FLD_NO_CMD);

		/* EXEC_DONE will be zero automatically if a UMA transaction is completed. */
		while (IS_BIT_SET(inst->UMA_CTS, NCT_UMA_CTS_EXEC_DONE)) {
			continue;
		}
	}

	inst->EXT_DB_CFG &= ~BIT(NCT_EXT_DB_CFG_EXT_DB_EN);
}

static inline void qspi_nct_uma_read_bytes(const struct device *dev, uint8_t *data, uint32_t len)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	uint32_t rd_len;

	while(len)
	{
		rd_len = (len > 4)?(4):(len);
		len -= rd_len;

		inst->UMA_CTS = UMA_FLD_EXEC | UMA_FLD_NO_CMD | rd_len;

		/* EXEC_DONE will be zero automatically if a UMA transaction is completed. */
		while (IS_BIT_SET(inst->UMA_CTS, NCT_UMA_CTS_EXEC_DONE)) {
			continue;
		}

		if(rd_len == 4)
		{
			*((uint32_t*)data) = inst->UMA_DB0_3;
			data += rd_len;
		}
		else
		{
			uint32_t dat = inst->UMA_DB0_3;

			while(rd_len--)
			{
				*data++ = dat & 0xFF;
				dat >>= 8;
			}
		}
	}

	inst->EXT_DB_CFG &= ~BIT(NCT_EXT_DB_CFG_EXT_DB_EN);
}

/* NCT SPI Direct Read Access (DRA)/User Mode Access (UMA) configuration functions */
static inline void qspi_nct_config_uma_mode(const struct device *dev)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* set address length to 0 */
	SET_FIELD(inst->UMA_ECTS, NCT_UMA_ECTS_UMA_ADDR_SIZE, 0);
}

static inline void qspi_nct_config_dra_4byte_mode(const struct device *dev,
						   const struct nct_qspi_cfg *qspi_cfg)
{
#if defined(CONFIG_FLASH_NCT_FIU_SUPP_DRA_4B_ADDR)
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);
	struct nct_qspi_data *const data = dev->data;
	
	if (qspi_cfg->enter_4ba != 0) {
		core_inst->FIU_EXT_CFG |= BIT(NCT_FIU_EXT_CFG_FOUR_BADDR);
		host_inst->FIU_EXT_CFG |= BIT(NCT_FIU_EXT_CFG_FOUR_BADDR);
	}
	else {
		if(data->sw_cs & NCT_QSPI_SW_CS0) {
			core_inst->SET_CMD_EN &= ~BIT(NCT_SET_CMD_EN_PVT_CMD_EN);
			host_inst->SET_CMD_EN &= ~BIT(NCT_SET_CMD_EN_PVT_CMD_EN);
		}
		else if(data->sw_cs & NCT_QSPI_SW_CS1) {
			core_inst->SET_CMD_EN &= ~BIT(NCPM_SET_CMD_EN_SHD_CMD_EN);
			host_inst->SET_CMD_EN &= ~BIT(NCPM_SET_CMD_EN_SHD_CMD_EN);
		}
		else if(data->sw_cs & NCT_QSPI_SW_CS2) {
			core_inst->SET_CMD_EN &= ~BIT(NCPM_SET_CMD_EN_BACK_CMD_EN);
			host_inst->SET_CMD_EN &= ~BIT(NCPM_SET_CMD_EN_BACK_CMD_EN);
		}

		core_inst->FIU_EXT_CFG &= ~BIT(NCT_FIU_EXT_CFG_FOUR_BADDR);
		host_inst->FIU_EXT_CFG &= ~BIT(NCT_FIU_EXT_CFG_FOUR_BADDR);
	}

#endif /* CONFIG_FLASH_NCT_FIU_SUPP_DRA_4B_ADDR */
}

static inline void qspi_nct_config_dra_mode(const struct device *dev,
					     const struct nct_qspi_cfg *qspi_cfg)
{
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);

	/* Selects the SPI read access type of Direct Read Access mode */
	switch (qspi_cfg->rd_mode) {
		case NCT_RD_MODE_NORMAL:
			SET_FIELD(core_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_NORMAL);
			SET_FIELD(host_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_NORMAL);
             break;
		case NCT_RD_MODE_FAST:
			SET_FIELD(core_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_FAST);
			SET_FIELD(host_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_FAST);
			break;
		case NCT_RD_MODE_FAST_DUAL:
			SET_FIELD(core_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_FAST_DUAL);
			SET_FIELD(host_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_FAST_DUAL);
			break;
		case NCT_RD_MODE_QUAD:
			SET_FIELD(core_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_FAST_DUAL);
			SET_FIELD(host_inst->SPI_FL_CFG, NCT_SPI_FL_CFG_RD_MODE,
					NCT_SPI_FL_CFG_RD_MODE_FAST_DUAL);

			core_inst->RESP_CFG |= BIT(NCT_RESP_CFG_QUAD_EN);
			host_inst->RESP_CFG |= BIT(NCT_RESP_CFG_QUAD_EN);

			/* Enable quad mode of Direct Read Mode if needed */
			if (qspi_cfg->qer_type != JESD216_DW15_QER_NONE) {
				/* set quad mode enable*/

			}
			break;
		default:
			LOG_ERR("un-support rd mode:%d", qspi_cfg->rd_mode);
			break;
	}

	/* Enable/Disable 4 byte address mode for Direct Read Access (DRA) */
	qspi_nct_config_dra_4byte_mode(dev, qspi_cfg);

	/* set read max burst 16 bytes */
	SET_FIELD(core_inst->BURST_CFG, NCT_BURST_CFG_R_BURST, NCT_BURST_CFG_R_BURST_16B);
	SET_FIELD(host_inst->BURST_CFG, NCT_BURST_CFG_R_BURST, NCT_BURST_CFG_R_BURST_16B);
}

static inline void qspi_nct_fiu_set_operation(const struct device *dev, uint32_t operation)
{
	if ((operation & NCT_EX_OP_EXT_FLASH_WP) != 0) {
		nct_pinctrl_flash_write_protect_set(NCT_FIU_FLASH_WP);
	}
}

static inline int qspi_nct_fiu_uma_lock(const struct device *dev)
{
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);

	if (WAIT_FOR(IS_BIT_SET(host_inst->FIU_MSR_STS, NCT_FIU_MSR_STS_MSTR_INACT),
			NCT_FIU_CHK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("wait host fiu inactive timeout");
		return -ETIMEDOUT;
	}

	core_inst->FIU_MSR_IE_CFG |= BIT(NCT_FIU_MSR_IE_CFG_UMA_BLOCK);

	return 0;
}

static inline void qspi_nct_fiu_uma_release(const struct device *dev)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	inst->FIU_MSR_IE_CFG &= ~BIT(NCT_FIU_MSR_IE_CFG_UMA_BLOCK);
}

/* NCT specific QSPI-FIU controller functions */
static int qspi_nct_fiu_uma_transceive(const struct device *dev, struct nct_transceive_cfg *cfg,
				     uint32_t flags)
{
	struct nct_qspi_data *const data = dev->data;

	/* Transaction is permitted? */
	if ((data->operation & NCT_EX_OP_LOCK_TRANSCEIVE) != 0) {
		return -EPERM;
	}

	/* uma init */
	qspi_nct_config_uma_mode(dev);

	/* UMA block */
	qspi_nct_fiu_uma_lock(dev);

	/* Assert chip select */
	qspi_nct_uma_cs_level(dev, data->sw_cs, false);

	/* Transmit op-code first */
	qspi_nct_uma_write_byte(dev, cfg->opcode);

	if ((flags & NCT_TRANSCEIVE_ACCESS_ADDR) != 0) {
		qspi_nct_uma_write_bytes(dev, &cfg->addr.u8[(data->cur_cfg->enter_4ba != 0) ? 0 : 1],
									(data->cur_cfg->enter_4ba != 0) ? 4 : 3);
	}

	if ((flags & NCT_TRANSCEIVE_ACCESS_WRITE) != 0) {
		if (cfg->tx_buf == NULL) {
			return -EINVAL;
		}

		qspi_nct_uma_write_bytes(dev, cfg->tx_buf,  cfg->tx_count);
	}

	if ((flags & NCT_TRANSCEIVE_ACCESS_READ) != 0) {
		if (cfg->rx_buf == NULL) {
			return -EINVAL;
		}

		qspi_nct_uma_read_bytes(dev, cfg->rx_buf, cfg->rx_count);
	}

	/* De-assert chip select */
	qspi_nct_uma_cs_level(dev, data->sw_cs, true);

	/* UMA unblock */
	qspi_nct_fiu_uma_release(dev);

	return 0;
}

static void qspi_nct_fiu_mutex_lock_configure(const struct device *dev,
					const struct nct_qspi_cfg *cfg,
					const uint32_t operation)
{
	struct nct_qspi_data *const data = dev->data;
	struct scfg_reg *inst_scfg = HAL_SFCG_INST();

	k_sem_take(&data->lock_sem, K_FOREVER);

	/* If the current device is different from previous one, configure it */
	if (data->cur_cfg != cfg) {
		data->cur_cfg = cfg;

		/* Apply pin-muxing and tri-state */
		pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

		/* Save SW CS bit used in UMA mode */
		data->sw_cs = find_lsb_set(cfg->flags & NCT_QSPI_SW_CS_MASK);

		/* Configure for Direct Read Access (DRA) settings */
		qspi_nct_config_dra_mode(dev, cfg);
	}

	/* pin select */
	inst_scfg->DEVALT0[0xc] |= BIT(2);

	/* Set QSPI bus operation */
	if (data->operation != operation) {
		qspi_nct_fiu_set_operation(dev, operation);
		data->operation = operation;
	}
}

static void qspi_nct_fiu_mutex_unlock(const struct device *dev)
{
	struct nct_qspi_data *const data = dev->data;

	k_sem_give(&data->lock_sem);
}

struct nct_qspi_ops nct_qspi_fiu_ops = {
	.lock_configure = qspi_nct_fiu_mutex_lock_configure,
        .unlock = qspi_nct_fiu_mutex_unlock,
        .transceive = qspi_nct_fiu_uma_transceive,
};

static int qspi_nct_fiu_init(const struct device *dev)
{
	const struct nct_qspi_fiu_config *const config = dev->config;
	struct nct_qspi_data *const data = dev->data;
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

#define NCT_SPI_FIU_INIT(n)							\
static const struct nct_qspi_fiu_config nct_qspi_fiu_config_##n = {		\
	.core_base = DT_INST_REG_ADDR_BY_IDX(n, 0),				\
	.host_base = DT_INST_REG_ADDR_BY_IDX(n, 1),				\
	.clk_cfg = DT_INST_PHA(n, clocks, clk_cfg),				\
};										\
static struct nct_qspi_data nct_qspi_data_##n = {				\
	.qspi_ops = &nct_qspi_fiu_ops						\
};										\
DEVICE_DT_INST_DEFINE(n, qspi_nct_fiu_init, NULL,				\
		      &nct_qspi_data_##n, &nct_qspi_fiu_config_##n,	\
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NCT_SPI_FIU_INIT)
