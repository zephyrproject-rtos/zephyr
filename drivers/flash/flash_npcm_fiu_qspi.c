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
#include <zephyr/sys/util.h>
#include <soc.h>

#include "flash_npcm_qspi.h"
#include "npcm_fiu_registers.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npcm_fiu_qspi, CONFIG_FLASH_LOG_LEVEL);

#define NPCM_FIU_CHK_TIMEOUT_US	10000

/* Physical CS to FIU register-role mapping */
#define NPCM_FIU_SHD_CS		NPCM_QSPI_SW_CS1
#define NPCM_FIU_BACK_CS	NPCM_QSPI_SW_CS2
#define NPCM_FIU_PVT_CS		NPCM_QSPI_SW_CS0

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
	uint32_t clk_id;
	/* Optional write-protect pinctrl state; NULL if board has none. */
	const struct pinctrl_dev_config *wp_pcfg;
};

static int qspi_npcm_fiu_write_protect_enable(const struct device *dev)
{
	const struct npcm_qspi_fiu_config *const config = dev->config;

	if (config->wp_pcfg == NULL) {
		return -ENOTSUP;
	}

	return pinctrl_apply_state(config->wp_pcfg, PINCTRL_STATE_DEFAULT);
}

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

static inline int qspi_npcm_uma_write_byte(const struct device *dev, uint8_t data)
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
	if (WAIT_FOR(!IS_BIT_SET(inst->UMA_CTS, NPCM_UMA_CTS_EXEC_DONE),
		     NPCM_FIU_CHK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("UMA write byte timeout");
		return -ETIMEDOUT;
	}

	return 0;
}

static inline int qspi_npcm_uma_read_byte(const struct device *dev, uint8_t *data)
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

	if (WAIT_FOR(!IS_BIT_SET(inst->UMA_CTS, NPCM_UMA_CTS_EXEC_DONE),
		     NPCM_FIU_CHK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("UMA read byte timeout");
		return -ETIMEDOUT;
	}

	*data = inst->UMA_DB0;
	return 0;
}

/* NPCM SPI Direct Read Access (DRA)/User Mode Access (UMA) configuration functions */
static inline void qspi_npcm_config_uma_mode(const struct device *dev,
					     const struct npcm_qspi_cfg *qspi_cfg)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	if (qspi_cfg->flags & NPCM_FIU_BACK_CS) {
		inst->UMA_ECTS |= BIT(NPCM_UMA_ECTS_DEV_NUM_BACK);
	} else {
		inst->UMA_ECTS &= ~BIT(NPCM_UMA_ECTS_DEV_NUM_BACK);
	}
}

static inline void qspi_npcm_fiu_uma_release(const struct device *dev)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	inst->FIU_MSR_IE_CFG &= ~BIT(NPCM_FIU_MSR_IE_CFG_UMA_BLOCK);
}

static inline int qspi_npcm_fiu_uma_lock(const struct device *dev)
{
	const struct npcm_qspi_fiu_config *cfg = dev->config;
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);

	if (cfg->host_base != cfg->core_base) {
		struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);

		if (WAIT_FOR(IS_BIT_SET(host_inst->FIU_MSR_STS,
				NPCM_FIU_MSR_STS_MSTR_INACT),
				NPCM_FIU_CHK_TIMEOUT_US, NULL) == false) {
			LOG_ERR("wait host fiu inactive timeout");
			return -ETIMEDOUT;
		}
	}

	core_inst->FIU_MSR_IE_CFG |= BIT(NPCM_FIU_MSR_IE_CFG_UMA_BLOCK);

	return 0;
}

/* Enable/Disable 4-byte address mode for Direct Read Access (DRA) */
static inline void qspi_npcm_config_dra_4byte_mode(const struct device *dev,
						   const struct npcm_qspi_cfg *qspi_cfg)
{
#if defined(CONFIG_FLASH_NPCM_FIU_SUPP_DRA_4B_ADDR)
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);
	uint8_t addr_4b_en = (qspi_cfg->flags & NPCM_QSPI_SW_CS_MASK) << 4;

	if (qspi_cfg->enter_4ba != 0) {
		core_inst->ADDR_4B_EN |= addr_4b_en;
		host_inst->ADDR_4B_EN |= addr_4b_en;
	} else {
		core_inst->ADDR_4B_EN &= ~addr_4b_en;
		host_inst->ADDR_4B_EN &= ~addr_4b_en;
	}
#endif /* CONFIG_FLASH_NPCM_FIU_SUPP_DRA_4B_ADDR */
}

int flash_npcm_fiu_set_4b_mode(const struct device *dev, bool enable)
{
#if defined(CONFIG_FLASH_NPCM_FIU_SUPP_DRA_4B_ADDR)
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);
	struct npcm_qspi_data *qspi_data = dev->data;
	const struct npcm_qspi_cfg *qspi_cfg = qspi_data->cur_cfg;
	uint8_t addr_4b_en_mask;

	if (qspi_cfg == NULL) {
		return -EINVAL;
	}

	addr_4b_en_mask = (qspi_cfg->flags & NPCM_QSPI_SW_CS_MASK) << 4;

	if (enable) {
		core_inst->ADDR_4B_EN |= addr_4b_en_mask;
		host_inst->ADDR_4B_EN |= addr_4b_en_mask;
	} else {
		core_inst->ADDR_4B_EN &= ~addr_4b_en_mask;
		host_inst->ADDR_4B_EN &= ~addr_4b_en_mask;
	}
#endif /* CONFIG_FLASH_NPCM_FIU_SUPP_DRA_4B_ADDR */

	return 0;
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
		LOG_ERR("unsupported rd mode: %d", qspi_cfg->rd_mode);
		return;
	}

	SET_FIELD(core_inst->SPI_FL_CFG, NPCM_SPI_FL_CFG_RD_MODE, rd_mode);
	SET_FIELD(host_inst->SPI_FL_CFG, NPCM_SPI_FL_CFG_RD_MODE, rd_mode);

	if (qspi_cfg->rd_mode == NPCM_RD_MODE_QUAD) {
		core_inst->RESP_CFG |= BIT(NPCM_RESP_CFG_QUAD_EN);
		host_inst->RESP_CFG |= BIT(NPCM_RESP_CFG_QUAD_EN);
	} else {
		core_inst->RESP_CFG &= ~BIT(NPCM_RESP_CFG_QUAD_EN);
		host_inst->RESP_CFG &= ~BIT(NPCM_RESP_CFG_QUAD_EN);
	}

	if (qspi_cfg->flags & NPCM_QSPI_QPI_MODE) {
		core_inst->Q_P_EN |= BIT(NPCM_Q_P_EN_CMD_QUAD_EN) |
				     BIT(NPCM_Q_P_EN_ADDR_QUAD_EN) |
				     BIT(NPCM_Q_P_EN_UMA_QUADREAD) |
				     BIT(NPCM_Q_P_EN_QUAD_P_EN);
		host_inst->Q_P_EN |= BIT(NPCM_Q_P_EN_CMD_QUAD_EN) |
				     BIT(NPCM_Q_P_EN_ADDR_QUAD_EN) |
				     BIT(NPCM_Q_P_EN_UMA_QUADREAD) |
				     BIT(NPCM_Q_P_EN_QUAD_P_EN);
		core_inst->BURST_CFG |= BIT(NPCM_BURST_CFG_SPI_WR_EN);
		host_inst->BURST_CFG |= BIT(NPCM_BURST_CFG_SPI_WR_EN);
	} else {
		core_inst->Q_P_EN &= ~(BIT(NPCM_Q_P_EN_CMD_QUAD_EN) |
				       BIT(NPCM_Q_P_EN_ADDR_QUAD_EN) |
				       BIT(NPCM_Q_P_EN_UMA_QUADREAD) |
				       BIT(NPCM_Q_P_EN_QUAD_P_EN));
		host_inst->Q_P_EN &= ~(BIT(NPCM_Q_P_EN_CMD_QUAD_EN) |
				       BIT(NPCM_Q_P_EN_ADDR_QUAD_EN) |
				       BIT(NPCM_Q_P_EN_UMA_QUADREAD) |
				       BIT(NPCM_Q_P_EN_QUAD_P_EN));
	}

	/* Set read burst to max 16 bytes */
	SET_FIELD(core_inst->BURST_CFG, NPCM_BURST_CFG_R_BURST, rd_burst);
	SET_FIELD(host_inst->BURST_CFG, NPCM_BURST_CFG_R_BURST, rd_burst);

	/* Enable/Disable 4-byte address mode for Direct Read Access (DRA) */
	qspi_npcm_config_dra_4byte_mode(dev, qspi_cfg);
}

/* Assert the board's write-protect pin function, if requested and defined. */
static inline void qspi_npcm_fiu_set_operation(const struct device *dev, uint32_t operation)
{
	if ((operation & (NPCM_EX_OP_INT_FLASH_WP | NPCM_EX_OP_EXT_FLASH_WP)) != 0) {
		(void)qspi_npcm_fiu_write_protect_enable(dev);
	}
}

static int qspi_npcm_fiu_uma_transceive(const struct device *dev, struct npcm_transceive_cfg *cfg,
				     uint32_t flags)
{
	struct npcm_qspi_data *const data = dev->data;
	int ret = 0;

	if ((data->operation & NPCM_EX_OP_LOCK_TRANSCEIVE) != 0) {
		ret = -EPERM;
		goto exit;
	}

	ret = qspi_npcm_fiu_uma_lock(dev);
	if (ret) {
		goto exit;
	}

	/* Assert chip select */
	qspi_npcm_uma_cs_level(dev, data->sw_cs, false);

	/* Transmit op-code first */
	ret = qspi_npcm_uma_write_byte(dev, cfg->opcode);
	if (ret != 0) {
		goto release_and_exit;
	}

	if ((flags & NPCM_TRANSCEIVE_ACCESS_ADDR) != 0) {
		size_t addr_bytes;
		const struct npcm_qspi_cfg *qspi_cfg = data->cur_cfg;
		int addr_start;

		if (cfg->addr_count > 0) {
			addr_bytes = cfg->addr_count;
		} else if (qspi_cfg->enter_4ba != 0) {
			addr_bytes = 4;
		} else {
			addr_bytes = 3;
		}

		addr_start = 4 - addr_bytes;
		for (size_t i = addr_start; i < 4; i++) {
			LOG_DBG("addr %d, %02x", i, cfg->addr.u8[i]);
			ret = qspi_npcm_uma_write_byte(dev, cfg->addr.u8[i]);
			if (ret != 0) {
				goto release_and_exit;
			}
		}
	}

	if ((flags & NPCM_TRANSCEIVE_ACCESS_WRITE) != 0) {
		if (cfg->tx_buf == NULL) {
			ret = -EINVAL;
			goto release_and_exit;
		}
		for (size_t i = 0; i < cfg->tx_count; i++) {
			ret = qspi_npcm_uma_write_byte(dev, cfg->tx_buf[i]);
			if (ret != 0) {
				goto release_and_exit;
			}
		}
	}

	if ((flags & NPCM_TRANSCEIVE_ACCESS_READ) != 0) {
		if (cfg->rx_buf == NULL) {
			ret = -EINVAL;
			goto release_and_exit;
		}
		for (size_t i = 0; i < cfg->rx_count; i++) {
			ret = qspi_npcm_uma_read_byte(dev, cfg->rx_buf + i);
			if (ret != 0) {
				goto release_and_exit;
			}
		}
	}

release_and_exit:
	/* De-assert chip select */
	qspi_npcm_uma_cs_level(dev, data->sw_cs, true);

	/* UMA unblock */
	qspi_npcm_fiu_uma_release(dev);

exit:
	return ret;
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

		pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

		data->sw_cs = find_lsb_set(cfg->flags & NPCM_QSPI_SW_CS_MASK) - 1;

		qspi_npcm_config_uma_mode(dev, cfg);
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

/* Enhanced RD_MODE configuration function */
static int qspi_npcm_fiu_set_read_mode(const struct device *dev, uint8_t rd_mode)
{
	struct npcm_qspi_data *const data = dev->data;
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	struct fiu_reg *const host_inst = HAL_HOST_INSTANCE(dev);
	uint8_t hw_rd_mode;
	int ret = 0;

	switch (rd_mode) {
	case NPCM_RD_MODE_NORMAL:
		hw_rd_mode = NPCM_SPI_FL_CFG_RD_MODE_NORMAL;
		break;
	case NPCM_RD_MODE_FAST:
		hw_rd_mode = NPCM_SPI_FL_CFG_RD_MODE_FAST;
		break;
	case NPCM_RD_MODE_FAST_DUAL:
		hw_rd_mode = NPCM_SPI_FL_CFG_RD_MODE_FAST_DUAL;
		break;
	case NPCM_RD_MODE_QUAD:
		hw_rd_mode = NPCM_SPI_FL_CFG_RD_MODE_FAST_DUAL; /* QUAD uses same as FAST_DUAL */
		break;
	default:
		ret = -EINVAL;
		goto exit;
	}

	k_sem_take(&data->lock_sem, K_FOREVER);

	SET_FIELD(core_inst->SPI_FL_CFG, NPCM_SPI_FL_CFG_RD_MODE, hw_rd_mode);
	SET_FIELD(host_inst->SPI_FL_CFG, NPCM_SPI_FL_CFG_RD_MODE, hw_rd_mode);

	if (rd_mode == NPCM_RD_MODE_QUAD) {
		core_inst->RESP_CFG |= BIT(NPCM_RESP_CFG_QUAD_EN);
		host_inst->RESP_CFG |= BIT(NPCM_RESP_CFG_QUAD_EN);
	} else {
		core_inst->RESP_CFG &= ~BIT(NPCM_RESP_CFG_QUAD_EN);
		host_inst->RESP_CFG &= ~BIT(NPCM_RESP_CFG_QUAD_EN);
	}

	k_sem_give(&data->lock_sem);

exit:
	return ret;
}

static int qspi_npcm_fiu_get_read_mode(const struct device *dev, uint8_t *rd_mode)
{
	struct npcm_qspi_data *const data = dev->data;
	struct fiu_reg *const core_inst = HAL_INSTANCE(dev);
	uint8_t hw_rd_mode;
	bool quad_enabled;
	int ret = 0;

	if (rd_mode == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	k_sem_take(&data->lock_sem, K_FOREVER);

	hw_rd_mode = GET_FIELD(core_inst->SPI_FL_CFG, NPCM_SPI_FL_CFG_RD_MODE);
	quad_enabled = (core_inst->RESP_CFG & BIT(NPCM_RESP_CFG_QUAD_EN)) != 0;

	if (quad_enabled && hw_rd_mode == NPCM_SPI_FL_CFG_RD_MODE_FAST_DUAL) {
		*rd_mode = NPCM_RD_MODE_QUAD;
	} else {
		switch (hw_rd_mode) {
		case NPCM_SPI_FL_CFG_RD_MODE_NORMAL:
			*rd_mode = NPCM_RD_MODE_NORMAL;
			break;
		case NPCM_SPI_FL_CFG_RD_MODE_FAST:
			*rd_mode = NPCM_RD_MODE_FAST;
			break;
		case NPCM_SPI_FL_CFG_RD_MODE_FAST_DUAL:
			*rd_mode = NPCM_RD_MODE_FAST_DUAL;
			break;
		default:
			*rd_mode = NPCM_RD_MODE_NORMAL; /* fallback */
			ret = -EIO;
			break;
		}
	}

	k_sem_give(&data->lock_sem);

exit:
	return ret;
}

/* FIU hardware CRC32/Checksum engine */
static int qspi_npcm_fiu_crc_init(const struct device *dev, uint8_t mode,
				  uint8_t source, uint32_t initial_value)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	struct npcm_qspi_data *const data = dev->data;

	k_sem_take(&data->lock_sem, K_FOREVER);

	inst->CRCCON = 0;
	inst->CRCRSLT = initial_value;

	if (mode == NPCM_CRC_MODE_CHECKSUM) {
		inst->CRCCON |= BIT(1);
	}

	if (source == NPCM_CRC_SRC_FLASH) {
		inst->CRCCON |= BIT(2);
	}

	inst->CRCCON |= BIT(0);

	k_sem_give(&data->lock_sem);
	return 0;
}

static int qspi_npcm_fiu_crc_add_data(const struct device *dev,
				      const uint8_t *data_buf, size_t length)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	struct npcm_qspi_data *const data = dev->data;

	if (data_buf == NULL || length == 0) {
		return -EINVAL;
	}

	k_sem_take(&data->lock_sem, K_FOREVER);

	for (size_t i = 0; i < length; i++) {
		inst->CRCENT = data_buf[i];
	}

	k_sem_give(&data->lock_sem);
	return 0;
}

static int qspi_npcm_fiu_crc_get_result(const struct device *dev, uint32_t *result)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	struct npcm_qspi_data *const data = dev->data;

	if (result == NULL) {
		return -EINVAL;
	}

	k_sem_take(&data->lock_sem, K_FOREVER);

	*result = inst->CRCRSLT;

	k_sem_give(&data->lock_sem);
	return 0;
}

struct npcm_qspi_ops npcm_qspi_fiu_ops = {
	.lock_configure = qspi_npcm_fiu_mutex_lock_configure,
	.unlock = qspi_npcm_fiu_mutex_unlock,
	.transceive = qspi_npcm_fiu_uma_transceive,
	.set_read_mode = qspi_npcm_fiu_set_read_mode,
	.get_read_mode = qspi_npcm_fiu_get_read_mode,
	.crc_init = qspi_npcm_fiu_crc_init,
	.crc_add_data = qspi_npcm_fiu_crc_add_data,
	.crc_get_result = qspi_npcm_fiu_crc_get_result,
};

static int qspi_npcm_fiu_init(const struct device *dev)
{
	const struct npcm_qspi_fiu_config *const config = dev->config;
	struct npcm_qspi_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		ret = -ENODEV;
		goto exit;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t)config->clk_id);
	if (ret < 0) {
		LOG_ERR("Turn on FIU clock fail %d", ret);
		goto exit;
	}

	/* initialize mutex for qspi controller */
	k_sem_init(&data->lock_sem, 1, 1);

exit:
	return ret;
}

/* pinctrl-0 is optional on this node; only define it when present. */
#define NPCM_SPI_FIU_WP_PINCTRL_DEFINE(n)					\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (PINCTRL_DT_INST_DEFINE(n);))

#define NPCM_SPI_FIU_WP_PCFG_GET(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),			\
		    (PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define NPCM_SPI_FIU_INIT(n)							\
NPCM_SPI_FIU_WP_PINCTRL_DEFINE(n)						\
static const struct npcm_qspi_fiu_config npcm_qspi_fiu_config_##n = {		\
	.core_base = DT_INST_REG_ADDR_BY_IDX(n, 0),				\
	.host_base = DT_INST_REG_ADDR_BY_IDX(n, 1),				\
	.clk_id = DT_INST_PHA(n, clocks, clk_id),				\
	.wp_pcfg = NPCM_SPI_FIU_WP_PCFG_GET(n),				\
};										\
static struct npcm_qspi_data npcm_qspi_data_##n = {				\
	.qspi_ops = &npcm_qspi_fiu_ops						\
};										\
DEVICE_DT_INST_DEFINE(n, qspi_npcm_fiu_init, NULL,				\
		      &npcm_qspi_data_##n, &npcm_qspi_fiu_config_##n,	\
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NPCM_SPI_FIU_INIT)
