/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_s32_qspi_nor

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_qspi_nor, CONFIG_FLASH_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/util.h>

#include <Qspi_Ip.h>

#include "spi_nor.h"
#include "jesd216.h"

#include "memc_nxp_s32_qspi.h"
#include "flash_nxp_s32_qspi.h"

#define QSPI_INST_NODE_HAS_PROP_EQ_AND_OR(n, prop, val)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop),				\
		(IS_EQ(DT_INST_ENUM_IDX(n, prop), val)),			\
		(0)) ||

#define QSPI_ANY_INST_HAS_PROP_EQ(prop, val)					\
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(QSPI_INST_NODE_HAS_PROP_EQ_AND_OR, prop, val) 0)

#define QSPI_INST_NODE_NOT_HAS_PROP_AND_OR(n, prop) \
	!DT_INST_NODE_HAS_PROP(n, prop) ||

#define QSPI_ANY_INST_HAS_PROP_STATUS_NOT_OKAY(prop) \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(QSPI_INST_NODE_NOT_HAS_PROP_AND_OR, prop) 0)

#define QSPI_QER_TYPE(n)							\
	_CONCAT(JESD216_DW15_QER_VAL_,						\
		DT_INST_STRING_TOKEN_OR(n, quad_enable_requirements, S1B6))

#define QSPI_HAS_QUAD_MODE(n)							\
	(QSPI_INST_NODE_HAS_PROP_EQ_AND_OR(n, readoc, 3)			\
	QSPI_INST_NODE_HAS_PROP_EQ_AND_OR(n, readoc, 4)				\
	QSPI_INST_NODE_HAS_PROP_EQ_AND_OR(n, writeoc, 2)			\
	QSPI_INST_NODE_HAS_PROP_EQ_AND_OR(n, writeoc, 3)			\
	0)

#define QSPI_WRITE_SEQ(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, writeoc),				\
		(_CONCAT(QSPI_SEQ_PP_, DT_INST_STRING_UPPER_TOKEN(n, writeoc))),\
		(QSPI_SEQ_PP_1_1_1))

#define QSPI_READ_SEQ(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, readoc),				\
		(_CONCAT(QSPI_SEQ_READ_, DT_INST_STRING_UPPER_TOKEN(n, readoc))),\
		(QSPI_SEQ_READ_1_1_1))

#define QSPI_LUT_ENTRY_SIZE (FEATURE_QSPI_LUT_SEQUENCE_SIZE * 2)
#define QSPI_LUT_IDX(n)	(n * QSPI_LUT_ENTRY_SIZE)

enum {
	QSPI_SEQ_RDSR,
	QSPI_SEQ_RDSR2,
	QSPI_SEQ_WRSR,
	QSPI_SEQ_WRSR2,
	QSPI_SEQ_WREN,
	QSPI_SEQ_RESET,
	QSPI_SEQ_SE,
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(has_32k_erase)
	QSPI_SEQ_BE_32K,
#endif
	QSPI_SEQ_BE,
	QSPI_SEQ_CE,
	QSPI_SEQ_READ_SFDP,
	QSPI_SEQ_RDID,
#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 0) || QSPI_ANY_INST_HAS_PROP_STATUS_NOT_OKAY(readoc)
	QSPI_SEQ_READ_1_1_1,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 1)
	QSPI_SEQ_READ_1_1_2,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 2)
	QSPI_SEQ_READ_1_2_2,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 3)
	QSPI_SEQ_READ_1_1_4,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 4)
	QSPI_SEQ_READ_1_4_4,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 0) || QSPI_ANY_INST_HAS_PROP_STATUS_NOT_OKAY(writeoc)
	QSPI_SEQ_PP_1_1_1,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 1)
	QSPI_SEQ_PP_1_1_2,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 2)
	QSPI_SEQ_PP_1_1_4,
#endif
#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 3)
	QSPI_SEQ_PP_1_4_4,
#endif
};

#if !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
static const Qspi_Ip_InstrOpType nxp_s32_qspi_lut[][QSPI_LUT_ENTRY_SIZE] = {
	[QSPI_SEQ_RDSR] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_RDSR),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_1, 1U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_RDSR2] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_RDSR2),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_1, 1U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_WRSR] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_WRSR),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_WRITE, QSPI_IP_LUT_PADS_1, 1U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_WRSR2] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_WRSR2),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_WRITE, QSPI_IP_LUT_PADS_1, 1U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_WREN] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_WREN),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_RESET] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_RESET_EN),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_PADS_1, 0U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_RESET_MEM),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_PADS_1, 0U),
	},

	[QSPI_SEQ_SE] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_SE),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(has_32k_erase)
	[QSPI_SEQ_BE_32K] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_BE_32K),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

	[QSPI_SEQ_BE] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_BE),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_CE] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_CE),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_READ_SFDP] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, JESD216_CMD_READ_SFDP),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_DUMMY, QSPI_IP_LUT_PADS_1, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_1, 16U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

	[QSPI_SEQ_RDID] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, JESD216_CMD_READ_ID),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_1, JESD216_READ_ID_LEN),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},

#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 0) || QSPI_ANY_INST_HAS_PROP_STATUS_NOT_OKAY(readoc)
	[QSPI_SEQ_READ_1_1_1] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_READ_FAST),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_DUMMY, QSPI_IP_LUT_PADS_1, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_1, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 1)
	[QSPI_SEQ_READ_1_1_2] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_DREAD),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_DUMMY, QSPI_IP_LUT_PADS_1, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_2, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 2)
	[QSPI_SEQ_READ_1_2_2] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_2READ),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_2, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_DUMMY, QSPI_IP_LUT_PADS_2, 4U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_2, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 3)
	[QSPI_SEQ_READ_1_1_4] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_QREAD),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_DUMMY, QSPI_IP_LUT_PADS_1, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_4, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(readoc, 4)
	[QSPI_SEQ_READ_1_4_4] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_4READ),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_4, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_MODE, QSPI_IP_LUT_PADS_4, 0U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_DUMMY, QSPI_IP_LUT_PADS_4, 4U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_4, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 0) || QSPI_ANY_INST_HAS_PROP_STATUS_NOT_OKAY(writeoc)
	[QSPI_SEQ_PP_1_1_1] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_PP),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_WRITE, QSPI_IP_LUT_PADS_1, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 1)
	[QSPI_SEQ_PP_1_1_2] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_PP_1_1_2),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_WRITE, QSPI_IP_LUT_PADS_2, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 2)
	[QSPI_SEQ_PP_1_1_4] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_PP_1_1_4),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_WRITE, QSPI_IP_LUT_PADS_4, 8U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif

#if QSPI_ANY_INST_HAS_PROP_EQ(writeoc, 3)
	[QSPI_SEQ_PP_1_4_4] = {
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1, SPI_NOR_CMD_PP_1_4_4),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_4, 24U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_WRITE, QSPI_IP_LUT_PADS_4, 16U),
		QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END, QSPI_IP_LUT_SEQ_END),
	},
#endif
};
#endif /* !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME) */

#if !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
static int nxp_s32_qspi_read_status_register(const struct device *dev,
					     uint8_t reg_num,
					     uint8_t *val)
{
	struct nxp_s32_qspi_data *data = dev->data;
	uint16_t lut_idx;
	Qspi_Ip_StatusType status;
	int ret = 0;

	switch (reg_num) {
	case 1U:
		lut_idx = QSPI_LUT_IDX(QSPI_SEQ_RDSR);
		break;
	case 2U:
		lut_idx = QSPI_LUT_IDX(QSPI_SEQ_RDSR2);
		break;
	default:
		LOG_ERR("Reading SR%u is not supported", reg_num);
		return -EINVAL;
	}

	nxp_s32_qspi_lock(dev);

	status = Qspi_Ip_RunReadCommand(data->instance, lut_idx, 0U, val, NULL, sizeof(*val));
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Failed to read SR%u (%d)", reg_num, status);
		ret = -EIO;
	}

	nxp_s32_qspi_unlock(dev);

	return ret;
}

static int nxp_s32_qspi_write_enable(const struct device *dev)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	Qspi_Ip_StatusType status;
	int ret = 0;

	nxp_s32_qspi_lock(dev);

	status = Qspi_Ip_RunCommand(data->instance, memory_cfg->statusConfig.writeEnableSRLut, 0U);
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Failed to enable SR write (%d)", status);
		ret = -EIO;
	}

	nxp_s32_qspi_unlock(dev);

	return ret;
}

static int nxp_s32_qspi_write_status_register(const struct device *dev,
					      uint8_t reg_num,
					      uint8_t val)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_StatusType status;
	uint8_t buf[2] = { 0 };
	uint16_t lut_idx;
	size_t size;
	int ret;

	if (reg_num == 1) {
		/* buf = [val] or [val, SR2] */
		lut_idx = QSPI_LUT_IDX(QSPI_SEQ_WRSR);
		size = 1U;
		buf[0] = val;

		if (config->qer_type == JESD216_DW15_QER_S2B1v1) {
			/* Writing SR1 clears SR2 */
			size = 2U;
			ret = nxp_s32_qspi_read_status_register(dev, 2, &buf[1]);
			if (ret < 0) {
				return ret;
			}
		}
	} else if (reg_num == 2) {
		/* buf = [val] or [SR1, val] */
		if ((config->qer_type == JESD216_DW15_QER_VAL_S2B1v1) ||
		    (config->qer_type == JESD216_DW15_QER_VAL_S2B1v4) ||
		    (config->qer_type == JESD216_DW15_QER_VAL_S2B1v5)) {
			/* Writing SR2 requires writing SR1 as well */
			lut_idx = QSPI_LUT_IDX(QSPI_SEQ_WRSR);
			size = 2U;
			buf[1] = val;
			ret = nxp_s32_qspi_read_status_register(dev, 1, &buf[0]);
			if (ret < 0) {
				return ret;
			}
		} else {
			lut_idx = QSPI_LUT_IDX(QSPI_SEQ_WRSR2);
			size = 1U;
			buf[0] = val;
		}
	} else {
		return -EINVAL;
	}

	nxp_s32_qspi_lock(dev);

	status = Qspi_Ip_RunWriteCommand(data->instance, lut_idx, 0U, (const uint8_t *)buf,
					 (uint32_t)size);
	if (status == STATUS_QSPI_IP_SUCCESS) {
		/* Wait for the write command to complete */
		ret = nxp_s32_qspi_wait_until_ready(dev);
	} else {
		LOG_ERR("Failed to write to SR%u (%d)", reg_num, status);
		ret = -EIO;
	}

	nxp_s32_qspi_unlock(dev);

	return ret;
}

static int nxp_s32_qspi_set_quad_mode(const struct device *dev, bool enabled)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	uint8_t sr_num;
	uint8_t sr_val;
	uint8_t qe_mask;
	bool qe_state;
	int ret;

	switch (config->qer_type) {
	case JESD216_DW15_QER_NONE:
		/* no QE bit, device detects reads based on opcode */
		return 0;
	case JESD216_DW15_QER_S1B6:
		sr_num = 1U;
		qe_mask = BIT(6U);
		break;
	case JESD216_DW15_QER_S2B7:
		sr_num = 2U;
		qe_mask = BIT(7U);
		break;
	case JESD216_DW15_QER_S2B1v1:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v4:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v5:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v6:
		sr_num = 2U;
		qe_mask = BIT(1U);
		break;
	default:
		return -ENOTSUP;
	}

	ret = nxp_s32_qspi_read_status_register(dev, sr_num, &sr_val);
	if (ret < 0) {
		return ret;
	}

	qe_state = ((sr_val & qe_mask) != 0U);
	if (qe_state == enabled) {
		return 0;
	}
	sr_val ^= qe_mask;

	ret = nxp_s32_qspi_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	ret = nxp_s32_qspi_write_status_register(dev, sr_num, sr_val);
	if (ret < 0) {
		return ret;
	}

	/* Verify write was successful */
	ret = nxp_s32_qspi_read_status_register(dev, sr_num, &sr_val);
	if (ret < 0) {
		return ret;
	}

	qe_state = ((sr_val & qe_mask) != 0U);
	if (qe_state != enabled) {
		LOG_ERR("Failed to %s Quad mode", enabled ? "enable" : "disable");
		return -EIO;
	}

	return ret;
}
#endif /* !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME) */

#if defined(CONFIG_FLASH_JESD216_API)
static int nxp_s32_qspi_sfdp_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_StatusType status;
	int ret = 0;

	nxp_s32_qspi_lock(dev);

	status = Qspi_Ip_RunReadCommand(data->instance, data->read_sfdp_lut_idx,
					(uint32_t)offset, (uint8_t *)buf, NULL, (uint32_t)len);
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Failed to read SFDP at 0x%lx (%d)", offset, status);
		ret = -EIO;
	}

	nxp_s32_qspi_unlock(dev);

	return ret;
}
#endif /* CONFIG_FLASH_JESD216_API */

#if defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
static int nxp_s32_qspi_sfdp_config(const struct device *dev)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	Qspi_Ip_StatusType status;

	/* Populate memory configuration with values obtained from SFDP */
	memory_cfg->memType = QSPI_IP_SERIAL_FLASH;
	memory_cfg->lutSequences.opCount = QSPI_SFDP_LUT_SIZE;
	memory_cfg->lutSequences.lutOps = (Qspi_Ip_InstrOpType *)data->lut_ops;
	memory_cfg->initConfiguration.opCount = QSPI_SFDP_INIT_OP_SIZE;
	memory_cfg->initConfiguration.operations = (Qspi_Ip_InitOperationType *)data->init_ops;

	status = Qspi_Ip_ReadSfdp(memory_cfg, &data->memory_conn_cfg);
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Fail to read SFDP (%d)", status);
		return -EIO;
	}

#if defined(CONFIG_FLASH_JESD216_API)
	/* The HAL does not populate LUTs for read SFDP and read ID */
	uint8_t lut_idx = QSPI_SFDP_LUT_SIZE;

	for (int i = 0; i < QSPI_SFDP_LUT_SIZE - 1; i++) {
		if ((data->lut_ops[i] == QSPI_IP_LUT_SEQ_END)
				&& (data->lut_ops[i+1] == QSPI_IP_LUT_SEQ_END)) {
			lut_idx = i + 1;
			break;
		}
	}

	/* Make sure there's enough space to add the LUT sequences */
	if ((lut_idx + QSPI_JESD216_SEQ_SIZE - 1) >= QSPI_SFDP_LUT_SIZE) {
		return -ENOMEM;
	}

	data->read_sfdp_lut_idx = lut_idx;
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1,
						JESD216_CMD_READ_SFDP);
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_ADDR, QSPI_IP_LUT_PADS_1, 24U);
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_DUMMY, QSPI_IP_LUT_PADS_1, 8U);
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_1, 16U);
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END,
						QSPI_IP_LUT_SEQ_END);

	memory_cfg->readIdSettings.readIdLut = lut_idx;
	memory_cfg->readIdSettings.readIdSize = JESD216_READ_ID_LEN;
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_CMD, QSPI_IP_LUT_PADS_1,
						JESD216_CMD_READ_ID);
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_READ, QSPI_IP_LUT_PADS_1,
						JESD216_READ_ID_LEN);
	data->lut_ops[lut_idx++] = QSPI_LUT_OP(QSPI_IP_LUT_INSTR_STOP, QSPI_IP_LUT_SEQ_END,
						QSPI_IP_LUT_SEQ_END);
#endif /* CONFIG_FLASH_JESD216_API */

	return 0;
}
#endif

static int nxp_s32_qspi_init(const struct device *dev)
{
	struct nxp_s32_qspi_data *data = dev->data;
	const struct nxp_s32_qspi_config *config = dev->config;
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	Qspi_Ip_StatusType status;
	int ret = 0;

	/* Used by the HAL to retrieve the internal driver state */
	data->instance = nxp_s32_qspi_register_device();
	__ASSERT_NO_MSG(data->instance < QSPI_IP_MEM_INSTANCE_COUNT);
	data->memory_conn_cfg.qspiInstance = memc_nxp_s32_qspi_get_instance(config->controller);

#if defined(CONFIG_MULTITHREADING)
	k_sem_init(&data->sem, 1, 1);
#endif

#if defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
	nxp_s32_qspi_sfdp_config(dev);
#endif

	/* Init memory device connected to the bus */
	status = Qspi_Ip_Init(data->instance,
			(const Qspi_Ip_MemoryConfigType *)memory_cfg,
			(const Qspi_Ip_MemoryConnectionType *)&data->memory_conn_cfg);
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Fail to init memory device %d (%d)", data->instance, status);
		return -EIO;
	}


#if !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
	uint8_t jedec_id[JESD216_READ_ID_LEN];

	/* Verify connectivity by reading the device ID */
	ret = nxp_s32_qspi_read_id(dev, jedec_id);
	if (ret != 0) {
		LOG_ERR("JEDEC ID read failed (%d)", ret);
		return -ENODEV;
	}

	/*
	 * Check the memory device ID against the one configured from devicetree
	 * to verify we are talking to the correct device.
	 */
	if (memcmp(jedec_id, memory_cfg->readIdSettings.readIdExpected, sizeof(jedec_id)) != 0) {
		LOG_ERR("Device id %02x %02x %02x does not match config %02x %02x %02x",
			jedec_id[0], jedec_id[1], jedec_id[2],
			memory_cfg->readIdSettings.readIdExpected[0],
			memory_cfg->readIdSettings.readIdExpected[1],
			memory_cfg->readIdSettings.readIdExpected[2]);
		return -EINVAL;
	}

	ret = nxp_s32_qspi_set_quad_mode(dev, config->quad_mode);
	if (ret < 0) {
		return ret;
	}
#endif /* !CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME */

	return ret;
}

static DEVICE_API(flash, nxp_s32_qspi_api) = {
	.erase = nxp_s32_qspi_erase,
	.write = nxp_s32_qspi_write,
	.read = nxp_s32_qspi_read,
	.get_parameters = nxp_s32_qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = nxp_s32_qspi_pages_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = nxp_s32_qspi_sfdp_read,
	.read_jedec_id = nxp_s32_qspi_read_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#define QSPI_PAGE_LAYOUT(n)							\
	.layout = {								\
		.pages_count = (DT_INST_PROP(n, size) / 8)			\
			/ CONFIG_FLASH_NXP_S32_QSPI_LAYOUT_PAGE_SIZE,		\
		.pages_size = CONFIG_FLASH_NXP_S32_QSPI_LAYOUT_PAGE_SIZE,	\
	}

#define QSPI_READ_ID_CFG(n)							\
	{									\
		.readIdLut = QSPI_LUT_IDX(QSPI_SEQ_RDID),			\
		.readIdSize = DT_INST_PROP_LEN(n, jedec_id),			\
		.readIdExpected = DT_INST_PROP(n, jedec_id),			\
	}

#define QSPI_MEMORY_CONN_CFG(n)							\
	{									\
		.connectionType = (Qspi_Ip_ConnectionType)DT_INST_REG_ADDR(n),	\
		.memAlignment = DT_INST_PROP(n, write_block_size)		\
	}

#define QSPI_ERASE_CFG(n)							\
	{									\
		.eraseTypes = {							\
			{							\
				.eraseLut = QSPI_LUT_IDX(QSPI_SEQ_SE),		\
				.size = 12, /* 4 KB */				\
			},							\
			{							\
				.eraseLut = QSPI_LUT_IDX(QSPI_SEQ_BE),		\
				.size = 16, /* 64 KB */				\
			},							\
			COND_CODE_1(DT_INST_PROP(n, has_32k_erase), (		\
			{							\
				.eraseLut = QSPI_LUT_IDX(QSPI_SEQ_BE_32K),	\
				.size = 15, /* 32 KB */				\
			},							\
			), (							\
			{							\
				.eraseLut = QSPI_IP_LUT_INVALID,		\
			},							\
			))							\
			{							\
				.eraseLut = QSPI_IP_LUT_INVALID,		\
			},							\
		},								\
		.chipEraseLut = QSPI_LUT_IDX(QSPI_SEQ_CE),			\
	}

#define QSPI_RESET_CFG(n)							\
	{									\
		.resetCmdLut = QSPI_LUT_IDX(QSPI_SEQ_RESET),			\
		.resetCmdCount = 4U,						\
	}

/*
 * SR information used internally by the HAL to access fields BUSY and WEL
 * during read/write/erase and polling status operations.
 */
#define QSPI_STATUS_REG_CFG(n)							\
	{									\
		.statusRegInitReadLut = QSPI_LUT_IDX(QSPI_SEQ_RDSR),		\
		.statusRegReadLut = QSPI_LUT_IDX(QSPI_SEQ_RDSR),		\
		.statusRegWriteLut = QSPI_LUT_IDX(QSPI_SEQ_WRSR),		\
		.writeEnableSRLut = QSPI_LUT_IDX(QSPI_SEQ_WREN),		\
		.writeEnableLut = QSPI_LUT_IDX(QSPI_SEQ_WREN),			\
		.regSize = 1U,							\
		.busyOffset = 0U,						\
		.busyValue = 1U,						\
		.writeEnableOffset = 1U,					\
	}

#define QSPI_INIT_CFG(n)							\
	{									\
		.opCount = 0U,							\
		.operations = NULL,						\
	}

#define QSPI_LUT_CFG(n)								\
	{									\
		.opCount = ARRAY_SIZE(nxp_s32_qspi_lut),			\
		.lutOps = (Qspi_Ip_InstrOpType *)nxp_s32_qspi_lut,		\
	}

#define QSPI_MEMORY_CFG(n)							\
	{									\
		.memType = QSPI_IP_SERIAL_FLASH,				\
		.hfConfig = NULL,						\
		.memSize = DT_INST_PROP(n, size) / 8,				\
		.pageSize = DT_INST_PROP(n, max_program_buffer_size),		\
		.writeLut = QSPI_LUT_IDX(QSPI_WRITE_SEQ(n)),			\
		.readLut = QSPI_LUT_IDX(QSPI_READ_SEQ(n)),			\
		.read0xxLut = QSPI_IP_LUT_INVALID,				\
		.read0xxLutAHB = QSPI_IP_LUT_INVALID,				\
		.eraseSettings = QSPI_ERASE_CFG(n),				\
		.statusConfig = QSPI_STATUS_REG_CFG(n),				\
		.resetSettings = QSPI_RESET_CFG(n),				\
		.initResetSettings = QSPI_RESET_CFG(n),				\
		.initConfiguration = QSPI_INIT_CFG(n),				\
		.lutSequences = QSPI_LUT_CFG(n),				\
		COND_CODE_1(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME, (), (	\
			.readIdSettings = QSPI_READ_ID_CFG(n),)			\
		)								\
		.suspendSettings = {						\
			.eraseSuspendLut = QSPI_IP_LUT_INVALID,			\
			.eraseResumeLut = QSPI_IP_LUT_INVALID,			\
			.programSuspendLut = QSPI_IP_LUT_INVALID,		\
			.programResumeLut = QSPI_IP_LUT_INVALID,		\
		},								\
		.initCallout = NULL,						\
		.resetCallout = NULL,						\
		.errorCheckCallout = NULL,					\
		.eccCheckCallout = NULL,					\
		.ctrlAutoCfgPtr = NULL,						\
	}

#define FLASH_NXP_S32_QSPI_INIT_DEVICE(n)					\
	COND_CODE_1(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME, (), (		\
		BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, jedec_id),		\
			"jedec-id is required for non-runtime SFDP");		\
		BUILD_ASSERT(DT_INST_PROP_LEN(n, jedec_id) == JESD216_READ_ID_LEN,\
			"jedec-id must be of size JESD216_READ_ID_LEN bytes");	\
	))									\
										\
	static const struct nxp_s32_qspi_config nxp_s32_qspi_config_##n = {	\
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),			\
		.flash_parameters = {						\
			.write_block_size = DT_INST_PROP(n, write_block_size),	\
			.erase_value = QSPI_ERASE_VALUE,			\
		},								\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,				\
			(QSPI_PAGE_LAYOUT(n),))					\
		COND_CODE_1(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME, (), (	\
			.memory_cfg = QSPI_MEMORY_CFG(n),			\
			.qer_type = QSPI_QER_TYPE(n),				\
			.quad_mode = QSPI_HAS_QUAD_MODE(n)			\
		))								\
	};									\
										\
	static struct nxp_s32_qspi_data nxp_s32_qspi_data_##n = {		\
		.memory_conn_cfg = QSPI_MEMORY_CONN_CFG(n),			\
		COND_CODE_1(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME, (), (	\
			.read_sfdp_lut_idx = QSPI_LUT_IDX(QSPI_SEQ_READ_SFDP),	\
		))								\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			nxp_s32_qspi_init,					\
			NULL,							\
			&nxp_s32_qspi_data_##n,					\
			&nxp_s32_qspi_config_##n,				\
			POST_KERNEL,						\
			CONFIG_FLASH_INIT_PRIORITY,				\
			&nxp_s32_qspi_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_NXP_S32_QSPI_INIT_DEVICE)
