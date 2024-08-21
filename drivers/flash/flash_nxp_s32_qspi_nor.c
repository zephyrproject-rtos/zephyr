/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_qspi_nor

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_qspi_nor, CONFIG_FLASH_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/util.h>

#include "spi_nor.h"
#include "jesd216.h"

#include "memc_nxp_s32_qspi.h"

#define QSPI_ERASE_VALUE      0xff
#define QSPI_WRITE_BLOCK_SIZE 1U

struct nxp_s32_qspi_config {
	const struct device *controller;
	enum memc_nxp_qspi_port port;
	struct flash_parameters flash_parameters;
	uint8_t jedec_id[JESD216_READ_ID_LEN];
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if !defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	uint32_t mem_size;
	uint32_t max_write_size;
	nxp_qspi_lut_seq_t *read_seq;
	nxp_qspi_lut_seq_t *write_seq;
	enum jesd216_dw15_qer_type qer_type;
	bool quad_mode;
#endif /* !CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
	uint8_t mem_alignment;
};

struct nxp_s32_qspi_data {
	struct k_sem sem;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	uint32_t mem_size;
	uint32_t max_write_size;
	nxp_qspi_lut_seq_t read_seq;
	nxp_qspi_lut_seq_t write_seq;
	enum jesd216_dw15_qer_type qer_type;
	bool quad_mode;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
};

static int qspi_rdsr_nolock(const struct device *dev, uint8_t reg_num, uint8_t *val);

static ALWAYS_INLINE uint32_t qspi_get_mem_size(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	return data->mem_size;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	return config->mem_size;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
}

static ALWAYS_INLINE uint32_t qspi_get_max_write_size(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	return data->max_write_size;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	return config->max_write_size;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
}

static ALWAYS_INLINE bool qspi_get_quad_mode(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	return data->quad_mode;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	return config->quad_mode;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
}

static ALWAYS_INLINE enum jesd216_dw15_qer_type qspi_get_qer_type(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	return data->qer_type;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	return config->qer_type;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
}

static ALWAYS_INLINE nxp_qspi_lut_seq_t *qspi_get_read_seq(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	return &data->read_seq;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	return config->read_seq;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
}

static ALWAYS_INLINE nxp_qspi_lut_seq_t *qspi_get_write_seq(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	return &data->write_seq;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	return config->write_seq;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
}

static ALWAYS_INLINE struct jesd216_erase_type *qspi_get_erase_types(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	return data->erase_types;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	return (struct jesd216_erase_type *)config->erase_types;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
}

static ALWAYS_INLINE void qspi_acquire(const struct device *dev)
{
	struct nxp_s32_qspi_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static ALWAYS_INLINE void qspi_release(const struct device *dev)
{
	struct nxp_s32_qspi_data *data = dev->data;

	k_sem_give(&data->sem);
}

static ALWAYS_INLINE bool area_is_subregion(const struct device *dev, off_t offset, size_t size)
{
	return ((offset >= 0) && (offset < qspi_get_mem_size(dev)) &&
		((size + offset) <= qspi_get_mem_size(dev)));
}

static int qspi_wait_until_ready_nolock(const struct device *dev)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	uint32_t retries = 0xFFFFFF;
	uint8_t rdsr_val;
	int busy;
	int ret = 0;

	do {
		/* Check if controller is busy */
		busy = memc_nxp_qspi_get_status(config->controller);
		if (!busy) {
			/* Check if memory device is busy */
			ret = qspi_rdsr_nolock(dev, 1U, &rdsr_val);
			if (!ret) {
				busy = FIELD_GET(SPI_NOR_WIP_BIT, rdsr_val) == 1U;
			}
		}
		retries--;
	} while (busy && (retries > 0));

	if (ret) {
		LOG_ERR("Failed to read memory status (%d)", ret);
		ret = -EIO;
	} else if (retries <= 0) {
		LOG_ERR("Timeout, memory is busy");
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int qspi_rdsr_nolock(const struct device *dev, uint8_t reg_num, uint8_t *val)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	int ret = 0;

	uint16_t vlut[NXP_QSPI_LUT_MAX_CMD] = {
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, 0U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_1, 1U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),
	};

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = &vlut,
		.port = config->port,
		.cmd = NXP_QSPI_READ,
		.addr = 0U,
		.data = val,
		.size = sizeof(*val),
		.alignment = config->mem_alignment,
	};

	switch (reg_num) {
	case 1U:
		vlut[0] |= NXP_QSPI_LUT_OPRND(SPI_NOR_CMD_RDSR);
		break;
	case 2U:
		vlut[0] |= NXP_QSPI_LUT_OPRND(SPI_NOR_CMD_RDSR2);
		break;
	default:
		LOG_ERR("Reading SR%u is not supported", reg_num);
		return -EINVAL;
	}

	ret = memc_nxp_qspi_transfer(config->controller, &transfer);
	if (ret) {
		LOG_ERR("Failed to read SR%u (%d)", reg_num, ret);
	}

	return ret;
}

static int qspi_wrsr_nolock(const struct device *dev, uint8_t reg_num, uint8_t val)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	enum jesd216_dw15_qer_type qer_type = qspi_get_qer_type(dev);
	uint8_t buf[2] = {0};
	int ret = 0;

	uint16_t vlut[NXP_QSPI_LUT_MAX_CMD] = {
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, 0U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_WRITE, NXP_QSPI_LUT_PADS_1, 1U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),
	};

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = &vlut,
		.port = config->port,
		.cmd = NXP_QSPI_WRITE,
		.addr = 0U,
		.data = (uint8_t *)buf,
		.size = 0U,
		.alignment = config->mem_alignment,
	};

	if (reg_num == 1) {
		/* buf = [val] or [val, SR2] */
		vlut[0] |= NXP_QSPI_LUT_OPRND(SPI_NOR_CMD_WRSR);
		transfer.size = 1U;
		buf[0] = val;

		if (qer_type == JESD216_DW15_QER_S2B1v1) {
			/* Writing SR1 clears SR2 */
			transfer.size = 2U;
			ret = qspi_rdsr_nolock(dev, 2, &buf[1]);
			if (ret < 0) {
				return ret;
			}
		}
	} else if (reg_num == 2) {
		/* buf = [val] or [SR1, val] */
		if ((qer_type == JESD216_DW15_QER_VAL_S2B1v1) ||
		    (qer_type == JESD216_DW15_QER_VAL_S2B1v4) ||
		    (qer_type == JESD216_DW15_QER_VAL_S2B1v5)) {
			/* Writing SR2 requires writing SR1 as well */
			vlut[0] |= NXP_QSPI_LUT_OPRND(SPI_NOR_CMD_WRSR);
			transfer.size = 2U;
			buf[1] = val;
			ret = qspi_rdsr_nolock(dev, 1, &buf[0]);
			if (ret < 0) {
				return ret;
			}
		} else {
			vlut[0] |= NXP_QSPI_LUT_OPRND(SPI_NOR_CMD_WRSR2);
			transfer.size = 1U;
			buf[0] = val;
		}
	} else {
		return -EINVAL;
	}

	ret = memc_nxp_qspi_transfer(config->controller, &transfer);
	if (ret) {
		LOG_ERR("Failed to write to SR%u (%d)", reg_num, ret);
		ret = -EIO;
	} else {
		/* Wait for the write command to complete */
		ret = qspi_wait_until_ready_nolock(dev);
	}

	return ret;
}

static int qspi_wren_nolock(const struct device *dev)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	uint8_t retries = 5;
	uint8_t sr_val = 0U;
	int ret = 0;

	uint16_t vlut[NXP_QSPI_LUT_MAX_CMD] = {
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, SPI_NOR_CMD_WREN),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),
	};

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = &vlut,
		.port = config->port,
		.cmd = NXP_QSPI_COMMAND,
		.addr = 0U,
		.data = NULL,
		.size = 0U,
		.alignment = config->mem_alignment,
	};

	while (retries--) {
		ret = qspi_wait_until_ready_nolock(dev);
		if (!ret) {
			ret = memc_nxp_qspi_transfer(config->controller, &transfer);
			if (!ret) {
				/* Verify it's actually enabled */
				ret = qspi_rdsr_nolock(dev, 1U, &sr_val);
				if (!ret) {
					if (FIELD_GET(SPI_NOR_WEL_BIT, sr_val) == 1U) {
						break;
					}
				}
			}
		}
	}

	if (ret) {
		LOG_ERR("Failed to enable write (%d)", ret);
	}

	return ret;
}

static int qspi_set_quad_mode(const struct device *dev, bool enabled)
{
	uint8_t sr_num;
	uint8_t sr_val = 0U;
	uint8_t qe_mask;
	bool qe_state;
	int ret = 0;

	switch (qspi_get_qer_type(dev)) {
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

	qspi_acquire(dev);

	ret = qspi_rdsr_nolock(dev, sr_num, &sr_val);
	if (ret < 0) {
		goto exit;
	}

	qe_state = ((sr_val & qe_mask) != 0U);
	if (qe_state == enabled) {
		ret = 0;
		goto exit;
	}
	sr_val ^= qe_mask;

	ret = qspi_wren_nolock(dev);
	if (ret < 0) {
		goto exit;
	}

	ret = qspi_wrsr_nolock(dev, sr_num, sr_val);
	if (ret < 0) {
		goto exit;
	}

	/* Verify write was successful */
	ret = qspi_rdsr_nolock(dev, sr_num, &sr_val);
	if (ret < 0) {
		goto exit;
	}

	qe_state = ((sr_val & qe_mask) != 0U);
	if (qe_state != enabled) {
		LOG_ERR("Failed to %s Quad mode", enabled ? "enable" : "disable");
		ret = -EIO;
	}

exit:
	qspi_release(dev);

	return ret;
}

static int qspi_read(const struct device *dev, off_t offset, void *dest, size_t size)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	int ret = 0;

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = qspi_get_read_seq(dev),
		.port = config->port,
		.cmd = NXP_QSPI_READ,
		.addr = offset,
		.data = dest,
		.size = size,
		.alignment = config->mem_alignment,
	};

	if (!dest) {
		return -EINVAL;
	}

	if (!area_is_subregion(dev, offset, size)) {
		return -ENODEV;
	}

	if (size) {
		qspi_acquire(dev);
		ret = memc_nxp_qspi_transfer(config->controller, &transfer);
		if (ret) {
			LOG_ERR("Failed to read %zu bytes at 0x%lx (%d)", size, offset, ret);
			ret = -EIO;
		}
		qspi_release(dev);
	}

	return ret;
}

static int qspi_write(const struct device *dev, off_t offset, const void *src, size_t size)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	size_t max_write = (size_t)qspi_get_max_write_size(dev);
	uint32_t len;
	int ret = 0;

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = qspi_get_write_seq(dev),
		.port = config->port,
		.cmd = NXP_QSPI_WRITE,
		.alignment = config->mem_alignment,
	};

	if (!src) {
		return -EINVAL;
	}

	if (!area_is_subregion(dev, offset, size)) {
		return -ENODEV;
	}

	qspi_acquire(dev);

	while (size) {
		len = MIN(max_write - (offset % max_write), size);

		transfer.addr = (uint32_t)offset;
		transfer.data = (uint8_t *)src;
		transfer.size = len;

		ret = qspi_wren_nolock(dev);
		if (ret) {
			break;
		}

		ret = memc_nxp_qspi_transfer(config->controller, &transfer);
		if (ret) {
			LOG_ERR("Failed to write %u bytes at 0x%lx (%d)", size, offset, ret);
		}

		ret = qspi_wait_until_ready_nolock(dev);
		if (ret) {
			break;
		}

		size -= len;
		src = (const uint8_t *)src + len;
		offset += len;
	}

	qspi_release(dev);

	return ret;
}

static int qspi_erase_block(const struct device *dev, off_t offset, size_t size,
			    size_t *erased_size)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	struct jesd216_erase_type *erase_types = qspi_get_erase_types(dev);
	const struct jesd216_erase_type *bet = NULL;
	int ret = 0;

	uint16_t vlut[NXP_QSPI_LUT_MAX_CMD] = {
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, 0U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),
	};

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = &vlut,
		.port = config->port,
		.cmd = NXP_QSPI_COMMAND,
		.addr = offset,
		.data = NULL,
		.size = 0U,
		.alignment = config->mem_alignment,
	};

	*erased_size = 0U;

	if (!SPI_NOR_IS_SECTOR_ALIGNED(offset)) {
		LOG_ERR("addr %ld is not sector-aligned", offset);
		return -EINVAL;
	}

	if ((size % KB(4)) != 0) {
		LOG_ERR("size %d is not a multiple of sectors", size);
		return -EINVAL;
	}

	/*
	 * Find the erase type with bigger size that can erase all or part of the
	 * requested memory size
	 */
	for (uint8_t ei = 0; ei < JESD216_NUM_ERASE_TYPES; ++ei) {
		const struct jesd216_erase_type *etp = &erase_types[ei];

		if ((etp->exp != 0) && SPI_NOR_IS_ALIGNED(offset, etp->exp) &&
		    SPI_NOR_IS_ALIGNED(size, etp->exp) &&
		    ((bet == NULL) || (etp->exp > bet->exp))) {
			bet = etp;
		}
	}
	if (!bet) {
		LOG_ERR("Can't erase %zu at 0x%lx", size, offset);
		return -EINVAL;
	}
	vlut[0] |= NXP_QSPI_LUT_OPRND(bet->cmd);

	ret = qspi_wren_nolock(dev);
	if (ret) {
		return ret;
	}

	ret = memc_nxp_qspi_transfer(config->controller, &transfer);
	if (ret) {
		return ret;
	}

	*erased_size = BIT(bet->exp);

	return qspi_wait_until_ready_nolock(dev);
}

static int qspi_erase_chip(const struct device *dev)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	int ret = 0;

	uint16_t vlut[NXP_QSPI_LUT_MAX_CMD] = {
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, SPI_NOR_CMD_CE),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),
	};

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = &vlut,
		.port = config->port,
		.cmd = NXP_QSPI_COMMAND,
		.addr = 0U,
		.data = NULL,
		.size = 0U,
		.alignment = config->mem_alignment,
	};

	ret = qspi_wren_nolock(dev);
	if (ret) {
		return ret;
	}

	ret = memc_nxp_qspi_transfer(config->controller, &transfer);
	if (ret) {
		return ret;
	}

	return qspi_wait_until_ready_nolock(dev);
}

static int qspi_erase(const struct device *dev, off_t offset, size_t size)
{
	size_t erased_size = 0;
	int ret = 0;

	if (!area_is_subregion(dev, offset, size)) {
		return -ENODEV;
	}

	qspi_acquire(dev);
	if (size == qspi_get_mem_size(dev)) {
		ret = qspi_erase_chip(dev);
	} else {
		while (size > 0) {
			ret = qspi_erase_block(dev, offset, size, &erased_size);
			if (ret) {
				break;
			}
			offset += erased_size;
			size -= erased_size;
			erased_size = 0;
		}
	}
	qspi_release(dev);

	return ret;
}

static int qspi_read_id(const struct device *dev, uint8_t *id)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	int ret = 0;

	uint16_t vlut[NXP_QSPI_LUT_MAX_CMD] = {
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, JESD216_CMD_READ_ID),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_1, JESD216_READ_ID_LEN),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),
	};

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = &vlut,
		.port = config->port,
		.cmd = NXP_QSPI_READ,
		.addr = 0U,
		.data = id,
		.size = JESD216_READ_ID_LEN,
		.alignment = config->mem_alignment,
	};

	qspi_acquire(dev);
	ret = memc_nxp_qspi_transfer(config->controller, &transfer);
	qspi_release(dev);

	return ret;
}

#if defined(CONFIG_FLASH_JESD216_API) || defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
static int qspi_sfdp_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	int ret = 0;

	uint16_t vlut[NXP_QSPI_LUT_MAX_CMD] = {
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1,
				 JESD216_CMD_READ_SFDP),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_DUMMY, NXP_QSPI_LUT_PADS_1, 8U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_1, 16U),
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),
	};

	struct memc_nxp_qspi_config transfer = {
		.lut_seq = &vlut,
		.port = config->port,
		.cmd = NXP_QSPI_READ,
		.addr = offset,
		.data = buf,
		.size = len,
		.alignment = config->mem_alignment,
	};

	qspi_acquire(dev);
	ret = memc_nxp_qspi_transfer(config->controller, &transfer);
	qspi_release(dev);

	return ret;
}
#endif /* CONFIG_FLASH_JESD216_API || CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */

#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
static void qspi_sfdp_process_rw_support(const struct device *dev, struct jesd216_instr *cmd,
					 enum jesd216_mode_type mode)
{
	struct nxp_s32_qspi_data *data = dev->data;
	uint16_t *read_seq = *(qspi_get_read_seq(dev));
	uint16_t *write_seq = *(qspi_get_write_seq(dev));
	uint16_t cmd_pads;
	uint16_t addr_pads;
	uint16_t data_pads;
	uint8_t idx = 0U;

	/* Only 3-byte addressing implemented */
	uint16_t addr_bits = 24;

	switch (mode) {
	case JESD216_MODE_144:
		cmd_pads = NXP_QSPI_LUT_PADS_1;
		addr_pads = NXP_QSPI_LUT_PADS_4;
		data_pads = NXP_QSPI_LUT_PADS_4;
		data->quad_mode = true;
		break;
	case JESD216_MODE_114:
		cmd_pads = NXP_QSPI_LUT_PADS_1;
		addr_pads = NXP_QSPI_LUT_PADS_1;
		data_pads = NXP_QSPI_LUT_PADS_4;
		data->quad_mode = true;
		break;
	case JESD216_MODE_122:
		cmd_pads = NXP_QSPI_LUT_PADS_1;
		addr_pads = NXP_QSPI_LUT_PADS_2;
		data_pads = NXP_QSPI_LUT_PADS_2;
		data->quad_mode = false;
		break;
	case JESD216_MODE_112:
		cmd_pads = NXP_QSPI_LUT_PADS_1;
		addr_pads = NXP_QSPI_LUT_PADS_1;
		data_pads = NXP_QSPI_LUT_PADS_2;
		data->quad_mode = false;
		break;
	case JESD216_MODE_111:
		__fallthrough;
	default:
		cmd_pads = NXP_QSPI_LUT_PADS_1;
		addr_pads = NXP_QSPI_LUT_PADS_1;
		data_pads = NXP_QSPI_LUT_PADS_1;
		data->quad_mode = false;
		break;
	}

	/* Build LUT sequence for read operation */
	idx = 0U;
	read_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, cmd_pads, cmd->instr);
	read_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, addr_pads, addr_bits);

	if (cmd->mode_clocks > 0) {
		uint16_t mode_instr = NXP_QSPI_LUT_INSTR_MODE;
		uint16_t mode_bits = cmd->mode_clocks * BIT(addr_pads);

		switch (mode_bits) {
		case 2U:
			mode_instr = NXP_QSPI_LUT_INSTR_MODE2;
			break;
		case 4U:
			mode_instr = NXP_QSPI_LUT_INSTR_MODE4;
			break;
		case 8U:
			__fallthrough;
		default:
			mode_instr = NXP_QSPI_LUT_INSTR_MODE;
			break;
		}
		read_seq[idx++] = NXP_QSPI_LUT_CMD(mode_instr, addr_pads, 0U);
	}

	if (cmd->wait_states > 0) {
		read_seq[idx++] =
			NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_DUMMY, addr_pads, cmd->wait_states);
	}

	read_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, data_pads, 8U);
	read_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U);
	__ASSERT_NO_MSG(idx < sizeof(nxp_qspi_lut_seq_t));

	/*
	 * Build LUT sequence for write operation. Only basic write supported since JESD216 does not
	 * encode information of write modes.
	 */
	cmd_pads = NXP_QSPI_LUT_PADS_1;
	addr_pads = NXP_QSPI_LUT_PADS_1;
	data_pads = NXP_QSPI_LUT_PADS_1;

	idx = 0U;
	write_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, cmd_pads, SPI_NOR_CMD_PP);
	write_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, addr_pads, addr_bits);
	write_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_WRITE, data_pads, 8U);
	write_seq[idx++] = NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U);
	__ASSERT_NO_MSG(idx < sizeof(nxp_qspi_lut_seq_t));
}

static int qspi_sfdp_process_bfp(const struct device *dev, const struct jesd216_param_header *php,
				 const struct jesd216_bfp *bfp)
{
	struct nxp_s32_qspi_data *data = dev->data;
	int ret = 0;

	struct jesd216_bfp_dw15 dw15;
	bool has_dw15 = jesd216_bfp_decode_dw15(php, bfp, &dw15) == 0U;

	data->qer_type = has_dw15 ? dw15.qer : JESD216_DW15_QER_NONE;
	LOG_DBG("QER: %u %s", data->qer_type, !has_dw15 ? "(DW15 not available)" : "");

	/*
	 * Find the best read mode supported. Quad modes will only be queried if DW15 QER is
	 * available, otherwise there is no information available on how to enable the QE bit.
	 */
	/* clang-format off */
	const enum jesd216_mode_type modes[] = {
		/* Other modes not yet implemented */
		JESD216_MODE_144,
		JESD216_MODE_114,
		JESD216_MODE_122,
		JESD216_MODE_112,
		JESD216_MODE_111,
	};
	/* clang-format on */

	struct jesd216_instr cmd;
	enum jesd216_mode_type mode = JESD216_MODE_111;

	for (uint8_t idx = has_dw15 ? 0U : 2U; idx < ARRAY_SIZE(modes); idx++) {
		ret = jesd216_bfp_read_support(php, bfp, modes[idx], &cmd);
		if (ret > 0) {
			mode = modes[idx];
			break;
		}
	}
	if (ret <= 0) {
		/* Fall back to 1-1-1 basic mode */
		cmd.instr = SPI_NOR_CMD_READ;
		cmd.mode_clocks = 0;
		cmd.wait_states = 0;
	}
	LOG_DBG("Read: instr %02Xh, %u mode clocks, %u waits", cmd.instr, cmd.mode_clocks,
		cmd.wait_states);
	qspi_sfdp_process_rw_support(dev, &cmd, mode);

	/* Find the erase types available */
	struct jesd216_erase_type *erase_types = qspi_get_erase_types(dev);
	struct jesd216_erase_type *etp = erase_types;

	for (uint8_t idx = 0; idx < JESD216_NUM_ERASE_TYPES; ++idx) {
		if (jesd216_bfp_erase(bfp, idx + 1, etp) == 0) {
			LOG_DBG("ET%u: instr %02Xh for %u By", idx + 1, etp->cmd,
				(uint32_t)BIT(etp->exp));
		} else {
			memset(etp, 0, sizeof(struct jesd216_erase_type));
		}
		++etp;
	}

	data->mem_size = (uint32_t)(jesd216_bfp_density(bfp) / 8U);
	LOG_DBG("Memory size: %u bytes", data->mem_size);

	uint32_t page_size = jesd216_bfp_page_size(php, bfp);

	/* Maximum write size was initialized with QSPI controller tx FIFO size */
	data->max_write_size = MIN(data->max_write_size, page_size);
	LOG_DBG("Program page size: %u bytes", page_size);

	return 0;
}

static int qspi_sfdp_config(const struct device *dev)
{
	union {
		/* We read only read BFP table */
		uint8_t raw[JESD216_SFDP_SIZE(1U)];
		struct jesd216_sfdp_header sfdp;
	} uheader;

	union {
		/* JESD216D supports up to 20 DWORDS values */
		uint32_t dw[20U];
		struct jesd216_bfp bfp;
	} uparam;

	const struct jesd216_sfdp_header *hp = &uheader.sfdp;
	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_bfp *bfp = &uparam.bfp;
	int ret = 0;

	ret = qspi_sfdp_read(dev, 0, uheader.raw, sizeof(uheader.raw));
	if (ret != 0) {
		LOG_ERR("SFDP read failed: %d", ret);
		return ret;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return -EINVAL;
	}

	uint16_t id = jesd216_param_id(php);

	if (id != JESD216_SFDP_PARAM_ID_BFP) {
		LOG_ERR("SFDP parameter table ID %x does not match BFP ID", id);
		return -EINVAL;
	}

	ret = qspi_sfdp_read(dev, jesd216_param_addr(php), uparam.dw, sizeof(uparam.dw));
	if (ret) {
		return ret;
	}

	return qspi_sfdp_process_bfp(dev, php, bfp);
}
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */

static const struct flash_parameters *qspi_get_parameters(const struct device *dev)
{
	const struct nxp_s32_qspi_config *config = dev->config;

	return &config->flash_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void qspi_pages_layout(const struct device *dev, const struct flash_pages_layout **layout,
			      size_t *layout_size)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;

	*layout = &data->layout;
#else
	const struct nxp_s32_qspi_config *config = dev->config;

	*layout = &config->layout;
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */

	*layout_size = 1;
}

static int qspi_pages_layout_config(const struct device *dev)
{
	int ret = 0;

#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	struct nxp_s32_qspi_data *data = dev->data;
	struct jesd216_erase_type *etp = qspi_get_erase_types(dev);
	const uint32_t mem_size = qspi_get_mem_size(dev);
	const uint32_t layout_page_size = CONFIG_FLASH_NXP_S32_QSPI_LAYOUT_PAGE_SIZE;
	uint8_t exponent = 0U;
	uint32_t erase_size = 0U;

	/* Find the smallest erase size */
	for (uint8_t idx = 0; idx < JESD216_NUM_ERASE_TYPES; ++idx) {
		if ((etp->cmd != 0) && ((exponent == 0) || (etp->exp < exponent))) {
			exponent = etp->exp;
		}
		etp++;
	}
	if (exponent == 0) {
		return -ENOTSUP;
	}
	erase_size = BIT(exponent);

	/* Layout page size must be a multiple of smallest erase size */
	if ((layout_page_size % erase_size) != 0) {
		LOG_ERR("Layout page %u must be a multiple of erase size %u", layout_page_size,
			erase_size);
		return -EINVAL;
	}

	/* Warn but accept layout page sizes that leave inaccessible space */
	if ((mem_size % layout_page_size) != 0) {
		LOG_WRN("Layout page %u wastes space with device size %u", layout_page_size,
			mem_size);
	}

	data->layout.pages_size = layout_page_size;
	data->layout.pages_count = mem_size / layout_page_size;
	LOG_DBG("Layout %zu x %zu By pages", data->layout.pages_count, data->layout.pages_size);
#else
	ARG_UNUSED(dev);
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */

	return ret;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int qspi_init(const struct device *dev)
{
#if !defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	const struct nxp_s32_qspi_config *config = dev->config;
#endif /* !CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */
	struct nxp_s32_qspi_data *data = dev->data;
	uint8_t jedec_id[JESD216_READ_ID_LEN] = {0U};
	int ret = 0;

	k_sem_init(&data->sem, 1, 1);

	ret = qspi_read_id(dev, jedec_id);
	if (ret) {
		LOG_ERR("JEDEC ID read failed (%d)", ret);
		return -ENODEV;
	}

#if defined(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME)
	ret = qspi_sfdp_config(dev);
	if (ret) {
		return -ENODEV;
	}
#else
	/*
	 * Check the memory device ID against the one configured from devicetree
	 * to verify we are talking to the correct device.
	 */
	if (memcmp(jedec_id, config->jedec_id, sizeof(jedec_id)) != 0) {
		LOG_ERR("Device id %02x %02x %02x does not match config %02x %02x %02x",
			jedec_id[0], jedec_id[1], jedec_id[2], config->jedec_id[0],
			config->jedec_id[1], config->jedec_id[2]);
		return -EINVAL;
	}
#endif /* CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME */

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	ret = qspi_pages_layout_config(dev);
	if (ret) {
		return -ENODEV;
	}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

	ret = qspi_set_quad_mode(dev, qspi_get_quad_mode(dev));
	if (ret) {
		return -ENODEV;
	}

	return 0;
}

static const struct flash_driver_api nxp_s32_qspi_api = {
	.erase = qspi_erase,
	.write = qspi_write,
	.read = qspi_read,
	.get_parameters = qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = qspi_pages_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = qspi_sfdp_read,
	.read_jedec_id = qspi_read_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#define QSPI_SEQ_READ_1_1_1                                                                        \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1,                      \
				 SPI_NOR_CMD_READ_FAST),                                           \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_DUMMY, NXP_QSPI_LUT_PADS_1, 8U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_1, 8U),                \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_READ_1_1_2                                                                        \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, SPI_NOR_CMD_DREAD),  \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_DUMMY, NXP_QSPI_LUT_PADS_1, 8U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_2, 8U),                \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_READ_1_2_2                                                                        \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, SPI_NOR_CMD_2READ),  \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_2, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_DUMMY, NXP_QSPI_LUT_PADS_2, 4U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_2, 8U),                \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_READ_1_1_4                                                                        \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, SPI_NOR_CMD_QREAD),  \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_DUMMY, NXP_QSPI_LUT_PADS_1, 8U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_4, 8U),                \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_READ_1_4_4                                                                        \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, SPI_NOR_CMD_4READ),  \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_4, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_MODE, NXP_QSPI_LUT_PADS_4, 0U),                \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_DUMMY, NXP_QSPI_LUT_PADS_4, 4U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_READ, NXP_QSPI_LUT_PADS_4, 8U),                \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_PP_1_1_1                                                                          \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1, SPI_NOR_CMD_PP),     \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_WRITE, NXP_QSPI_LUT_PADS_1, 8U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_PP_1_1_2                                                                          \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1,                      \
				 SPI_NOR_CMD_PP_1_1_2),                                            \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_WRITE, NXP_QSPI_LUT_PADS_2, 8U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_PP_1_1_4                                                                          \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1,                      \
				 SPI_NOR_CMD_PP_1_1_4),                                            \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_1, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_WRITE, NXP_QSPI_LUT_PADS_4, 8U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_SEQ_PP_1_4_4                                                                          \
	{                                                                                          \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_CMD, NXP_QSPI_LUT_PADS_1,                      \
				 SPI_NOR_CMD_PP_1_4_4),                                            \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_ADDR, NXP_QSPI_LUT_PADS_4, 24U),               \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_WRITE, NXP_QSPI_LUT_PADS_4, 16U),              \
		NXP_QSPI_LUT_CMD(NXP_QSPI_LUT_INSTR_STOP, NXP_QSPI_LUT_PADS_1, 0U),                \
	}

#define QSPI_QER_TYPE(n)                                                                           \
	_CONCAT(JESD216_DW15_QER_VAL_, DT_INST_STRING_TOKEN_OR(n, quad_enable_requirements, S1B6))

#define QSPI_INST_NODE_HAS_PROP_EQ(n, prop, val)                                                   \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop), (IS_EQ(DT_INST_ENUM_IDX(n, prop), val)), (0))

/* Keep in sync with dt bindings */
#define QSPI_HAS_QUAD_MODE(n)                                                                      \
	(QSPI_INST_NODE_HAS_PROP_EQ(n, readoc, 3) || QSPI_INST_NODE_HAS_PROP_EQ(n, readoc, 4) ||   \
	 QSPI_INST_NODE_HAS_PROP_EQ(n, writeoc, 2) || QSPI_INST_NODE_HAS_PROP_EQ(n, writeoc, 3))

#define QSPI_READ_SEQ(n)                                                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, readoc),                                              \
		    (_CONCAT(QSPI_SEQ_READ_, DT_INST_STRING_UPPER_TOKEN(n, readoc))),              \
		    (QSPI_SEQ_READ_1_1_1))

#define QSPI_WRITE_SEQ(n)                                                                          \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, writeoc),                                             \
		    (_CONCAT(QSPI_SEQ_PP_, DT_INST_STRING_UPPER_TOKEN(n, writeoc))),               \
		    (QSPI_SEQ_PP_1_1_1))

/* clang-format off */
#define QSPI_INIT_DEVICE(n)                                                                        \
	COND_CODE_1(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME, (),                                \
		    (BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, jedec_id),                              \
				  "jedec-id is required for non-runtime SFDP");                    \
		     BUILD_ASSERT(DT_INST_PROP_LEN(n, jedec_id) == JESD216_READ_ID_LEN,            \
				  "jedec-id must be of size JESD216_READ_ID_LEN bytes");))         \
                                                                                                   \
	IF_DISABLED(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME, (                                  \
	static const uint16_t nxp_s32_qspi_read_seq_##n[NXP_QSPI_LUT_MAX_CMD] = QSPI_READ_SEQ(n);  \
	static const uint16_t nxp_s32_qspi_write_seq_##n[NXP_QSPI_LUT_MAX_CMD] = QSPI_WRITE_SEQ(n);\
	))                                                                                         \
                                                                                                   \
	static const struct nxp_s32_qspi_config nxp_s32_qspi_config_##n = {                        \
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),                                       \
		.port = DT_INST_REG_ADDR(n),                                                       \
		.jedec_id = DT_INST_PROP(n, jedec_id),                                             \
		.flash_parameters = {                                                              \
			.write_block_size = QSPI_WRITE_BLOCK_SIZE,                                 \
			.erase_value = QSPI_ERASE_VALUE,                                           \
		},                                                                                 \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (                                             \
		.layout = {                                                                        \
			.pages_count = ((DT_INST_PROP(n, size) / 8)                                \
					/ CONFIG_FLASH_NXP_S32_QSPI_LAYOUT_PAGE_SIZE),             \
			.pages_size = CONFIG_FLASH_NXP_S32_QSPI_LAYOUT_PAGE_SIZE,                  \
		},))                                                                               \
		IF_DISABLED(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME, (                          \
		.qer_type = QSPI_QER_TYPE(n),                                                      \
		.quad_mode = QSPI_HAS_QUAD_MODE(n),                                                \
		.read_seq = (nxp_qspi_lut_seq_t *)&nxp_s32_qspi_read_seq_##n,                      \
		.write_seq = (nxp_qspi_lut_seq_t *)&nxp_s32_qspi_write_seq_##n,                    \
		.max_write_size = MIN(DT_PROP(DT_INST_BUS(n), tx_fifo_size), SPI_NOR_PAGE_SIZE),   \
		.mem_size = DT_INST_PROP(n, size) / 8,                                             \
		.erase_types = {                                                                   \
			{ .cmd = SPI_NOR_CMD_SE, .exp = 12 },        /* 4 KB*/                     \
			IF_ENABLED(DT_INST_PROP(n, has_32k_erase), (                               \
			{ .cmd = SPI_NOR_CMD_BE_32K, .exp = 15 },    /* 32 KB*/                    \
			))                                                                         \
			{ .cmd = SPI_NOR_CMD_BE, .exp = 16 },        /* 64 KB*/                    \
		},                                                                                 \
		))                                                                                 \
		.mem_alignment = DT_INST_PROP_OR(n, memory_alignment, 1),                          \
	};                                                                                         \
                                                                                                   \
	static struct nxp_s32_qspi_data nxp_s32_qspi_data_##n = {                                  \
		IF_ENABLED(CONFIG_FLASH_NXP_S32_QSPI_NOR_SFDP_RUNTIME, (                           \
		.max_write_size = DT_PROP(DT_INST_BUS(n), tx_fifo_size),                           \
		))                                                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, qspi_init, NULL, &nxp_s32_qspi_data_##n, &nxp_s32_qspi_config_##n,\
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &nxp_s32_qspi_api);
/* clang-format on*/

DT_INST_FOREACH_STATUS_OKAY(QSPI_INIT_DEVICE)
