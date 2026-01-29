/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT microchip_sama7g5_qspi

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include "spi_nor.h"
#include "jesd216.h"
#include "flash_sam_qspi.h"

LOG_MODULE_REGISTER(FLASH_SAM_QSPI, CONFIG_FLASH_LOG_LEVEL);

struct flash_sam_qspi_config {
	qspi_registers_t *qspi_base;
	uint32_t qspi_mem;
	const struct pinctrl_dev_config *pincfg;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct device *dma_dev;
	uint32_t dma_channel;
	uint32_t skip_sfdp;
	uint32_t flash_size;
};

/**
 * struct flash_sam_nor_data - Structure to describe some SPI flash memory
 * @addr_len: Number of address bytes.
 * @dtr: double rate enable.
 * @id: JEDEC ID.
 * @size: The total SPI flash size (in bytes).
 * @page_size:	The page size (in bytes).
 * @erase_types: Description of a supported erase operation.
 * @qer_type: DW15 Quad Enable Requirements specifies status register QE bits
 * @num_mode_cycles: The number of mode clock cycles.
 * @num_wait_states: The number of wait state clock cycles.
 * @read_inst:	The (Fast) Read instruction opcode.
 * @write_inst: The Page Program instruction opcode.
 * @k_mutex mutex;
 */
struct flash_sam_nor_data {
	uint8_t	addr_len;
	uint8_t dtr;
	size_t size;
	size_t page_size;
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	enum jesd216_dw15_qer_type qer_type;
	uint8_t num_mode_cycles;
	uint8_t num_wait_states;
	uint16_t read_inst;
	uint32_t read_proto;
	uint16_t write_inst;
	struct dma_config dma_cfg;
	struct k_mutex mutex;
};

static uint8_t spi_nor_convert_inst(uint8_t inst,
		const uint8_t table[][2], size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (table[i][0] == inst) {
			return table[i][1];
		}
	}

	/* No conversion found, keep input op code. */
	return inst;
}

static inline uint8_t spi_nor_convert_3to4_read(uint8_t inst)
{
	static const uint8_t spi_nor_3to4_read[][2] = {
		{ SPI_NOR_CMD_READ, SPI_NOR_CMD_READ_4B },
		{ SPI_NOR_CMD_QREAD, SPI_NOR_CMD_QREAD_4B },
		{ SPI_NOR_CMD_4READ, SPI_NOR_CMD_4READ_4B },
	};

	return spi_nor_convert_inst(inst, spi_nor_3to4_read,
				    ARRAY_SIZE(spi_nor_3to4_read));
}

static inline uint8_t spi_nor_convert_3to4_erase(uint8_t inst)
{
	static const uint8_t spi_nor_3to4_erase[][2] = {
		{ SPI_NOR_CMD_SE, SPI_NOR_CMD_SE_4B },
		{ SPI_NOR_CMD_BE_32K, SPI_NOR_CMD_BE_32K_4B },
		{ SPI_NOR_CMD_BE, SPI_NOR_CMD_BE_4B },
	};

	return spi_nor_convert_inst(inst, spi_nor_3to4_erase,
				    ARRAY_SIZE(spi_nor_3to4_erase));
}

static const struct flash_parameters FLASH_SAM_QSPI_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff
};

static const struct flash_parameters *
flash_sam_qspi_get_parameters(const struct device *dev)
{
	return &FLASH_SAM_QSPI_parameters;
}

static int flash_sam_qspi_get_size(const struct device *dev, uint64_t *size)
{
	struct flash_sam_nor_data *data = dev->data;

	*size = (uint64_t)data->size;

	return 0;
}

static int qspi_read_status_register(const struct device *dev, uint8_t reg_num, uint8_t *reg)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op op = {0};

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	op.proto = SFLASH_PROTO_1_1_1;
	op.data.dir = QSPI_MEM_DATA_IN;
	op.data.buf.in = reg;
	op.data.nbytes = 1;

	switch (reg_num) {
	case 1U:
		op.cmd.opcode = SPI_NOR_CMD_RDSR;
		break;
	case 2U:
		op.cmd.opcode = SPI_NOR_CMD_RDSR2;
		break;
	case 3U:
		op.cmd.opcode = SPI_NOR_CMD_RDSR3;
		break;
	default:
		return -EINVAL;
	}

	return qspi_exec_op(&hqspi, &op);
}


static int qspi_write_enable(const struct device *dev)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op op = {0};
	uint8_t reg;
	int ret;

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	memset(&op, 0, sizeof(struct qspi_mem_op));
	op.proto = SFLASH_PROTO_1_1_1;
	op.cmd.opcode = SPI_NOR_CMD_WREN;
	ret = qspi_exec_op(&hqspi, &op);
	if (ret) {
		return ret;
	}

	do {
		ret = qspi_read_status_register(dev, 1U, &reg);
	} while (!ret && !(reg & SPI_NOR_WEL_BIT));

	return ret;
}

static int qspi_write_status_register(const struct device *dev, uint8_t reg_num, uint8_t reg)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct flash_sam_nor_data *data = dev->data;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op op = {0};
	uint8_t regs[4] = {0};
	size_t size;
	uint8_t *regs_p;
	int ret;

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	if (reg_num == 1U) {
		size = 1;
		regs[0] = reg;
		regs_p = &regs[0];

		/* 1 byte write clears SR2, write SR2 as well */
		if (data->qer_type == JESD216_DW15_QER_S2B1v1) {
			ret = qspi_read_status_register(dev, 2, &regs[1]);
			if (ret < 0) {
				return ret;
			}
			size++;
		}
	} else if (reg_num == 2U) {
		op.cmd.opcode = SPI_NOR_CMD_WRSR2;
		size = 1;
		regs[1] = reg;
		regs_p = &regs[1];

		/* if SR2 write needs SR1 */
		if ((data->qer_type == JESD216_DW15_QER_VAL_S2B1v1) ||
		    (data->qer_type == JESD216_DW15_QER_VAL_S2B1v4) ||
		    (data->qer_type == JESD216_DW15_QER_VAL_S2B1v5)) {
			ret = qspi_read_status_register(dev, 1, &regs[0]);
			if (ret < 0) {
				return ret;
			}
			op.cmd.opcode = SPI_NOR_CMD_WRSR;
			size++;
			regs_p = &regs[0];
		}
	} else if (reg_num == 3U) {
		op.cmd.opcode = SPI_NOR_CMD_WRSR3;
		size = 1;
		regs[2] = reg;
		regs_p = &regs[2];
	} else {
		return -EINVAL;
	}

	op.proto = SFLASH_PROTO_1_1_1;
	op.data.dir = QSPI_MEM_DATA_OUT;
	op.data.buf.out = regs_p;
	op.data.nbytes = size;

	return qspi_exec_op(&hqspi, &op);
}

static int qspi_wait_until_ready(const struct device *dev)
{
	uint8_t reg;
	int ret;

	do {
		ret = qspi_read_status_register(dev, 1U, &reg);
	} while (!ret && (reg & SPI_NOR_WIP_BIT));

	return ret;
}

static int qspi_quad_enable(const struct device *dev)
{
	struct flash_sam_nor_data *data = dev->data;
	uint8_t qe_reg_num;
	uint8_t qe_bit;
	uint8_t reg;
	int ret;

	switch (data->qer_type) {
	case JESD216_DW15_QER_NONE:
		/* no QE bit, device detects reads based on opcode */
		return 0;
	case JESD216_DW15_QER_S1B6:
		qe_reg_num = 1U;
		qe_bit = BIT(6U);
		break;
	case JESD216_DW15_QER_S2B7:
		qe_reg_num = 2U;
		qe_bit = BIT(7U);
		break;
	case JESD216_DW15_QER_S2B1v1:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v4:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v5:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v6:
		qe_reg_num = 2U;
		qe_bit = BIT(1U);
		break;
	default:
		return -ENOTSUP;
	}

	ret = qspi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	/* exit early if QE bit is already set */
	if ((reg & qe_bit) != 0U) {
		return 0;
	}

	reg |= qe_bit;

	ret = qspi_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	ret = qspi_write_status_register(dev, qe_reg_num, reg);
	if (ret < 0) {
		LOG_ERR("Failed to set QE bit: %d", ret);
		return ret;
	}

	ret = qspi_wait_until_ready(dev);
	if (ret < 0) {
		LOG_ERR("Flash failed to become ready after writing QE bit: %d", ret);
		return ret;
	}

	/* validate that QE bit is set */
	ret = qspi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		LOG_ERR("Failed to fetch QE register after setting it: %d", ret);
		return ret;
	}

	if ((reg & qe_bit) == 0U) {
		LOG_ERR("Status Register %u [0x%02x] not set", qe_reg_num, reg);
		return -EIO;
	}

	return ret;
}

static int flash_sam_qspi_read(const struct device *dev, off_t addr,
				 void *buf, size_t size)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct flash_sam_nor_data *data = dev->data;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op read_op = {0};
	int ret;

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	read_op.proto = data->read_proto;
	read_op.cmd.modebits = data->num_mode_cycles;
	read_op.cmd.waitstates = data->num_wait_states;
	read_op.cmd.dtr = data->dtr;
	read_op.cmd.opcode = data->read_inst;
	read_op.addr.nbytes = data->addr_len;
	read_op.addr.val = (uint32_t)addr;
	read_op.data.dir = QSPI_MEM_DATA_IN;
	read_op.data.nbytes = size;
	read_op.data.buf.in = buf;

	k_mutex_lock(&data->mutex, K_FOREVER);
	ret = qspi_exec_op(&hqspi, &read_op);
	if (ret != 0) {
		LOG_ERR("READ: failed to read qspi flash");
	}
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int flash_sam_qspi_erase(const struct device *dev, off_t addr,
				  size_t size)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct flash_sam_nor_data *data = dev->data;
	const size_t flash_size = data->size;
	struct qspi_priv hqspi = {0};
	int ret = 0;
	struct qspi_mem_op erase_op = {0};

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	erase_op.addr.nbytes = data->addr_len;
	erase_op.proto = SFLASH_PROTO_1_1_1;

	/* erase area must be subregion of device */
	if ((addr < 0) || ((size + addr) > flash_size)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	while ((size > 0) && (ret == 0)) {
		qspi_write_enable(dev);

		/* chip erase */
		if (size == flash_size) {
			erase_op.cmd.opcode = SPI_NOR_CMD_CE;
			ret = qspi_exec_op(&hqspi, &erase_op);
			size -= flash_size;
		} else {
			const struct jesd216_erase_type *erase_types =
							data->erase_types;
			const struct jesd216_erase_type *bet = NULL;

			for (uint8_t ei = 0;
				ei < JESD216_NUM_ERASE_TYPES; ++ei) {
				const struct jesd216_erase_type *etp =
							&erase_types[ei];

				if ((etp->exp != 0)
				    && SPI_NOR_IS_ALIGNED(addr, etp->exp)
				    && SPI_NOR_IS_ALIGNED(size, etp->exp)
				    && ((bet == NULL)
					|| (etp->exp > bet->exp))) {
					bet = etp;
					erase_op.cmd.opcode = bet->cmd;
				}
			}
			if (bet != NULL) {
				erase_op.addr.val = addr;
				ret = qspi_exec_op(&hqspi, &erase_op);
				addr += BIT(bet->exp);
				size -= BIT(bet->exp);
			} else {
				LOG_ERR("Can't erase %zu at 0x%lx",	size, (long)addr);
				ret = -EINVAL;
			}
		}
		qspi_wait_until_ready(dev);
	}
	goto end;

end:
	k_mutex_unlock(&data->mutex);

	return ret;
}


static int qspi_write_unprotect(const struct device *dev)
{
	int ret = 0;
	const struct flash_sam_qspi_config *config = dev->config;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op op = {0};

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	op.proto = SFLASH_PROTO_1_1_1;
	op.cmd.opcode = SPI_NOR_CMD_ULBPR;

	ret = qspi_write_enable(dev);
	if (ret < 0) {
		return -EINVAL;
	}

	ret = qspi_exec_op(&hqspi, &op);
	if (ret < 0) {
		return -EINVAL;
	}

	ret = qspi_wait_until_ready(dev);
	if (ret < 0) {
		return -EINVAL;
	}

	return ret;
}

static int flash_sam_qspi_write(const struct device *dev, off_t addr, const void *buf,
				       size_t size)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct flash_sam_nor_data *data = dev->data;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op write_op = {0};
	int ret = 0;
	size_t page_size = data->page_size;

	/* write non-zero size */
	if (size == 0) {
		return 0;
	}

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	write_op.addr.nbytes = data->addr_len;
	write_op.proto = SFLASH_PROTO_1_1_1;
	write_op.cmd.opcode = data->write_inst;

	k_mutex_lock(&data->mutex, K_FOREVER);
	while (size) {
		size_t page_offset, to_write;

		page_offset = addr & (page_size - 1);
		to_write = min(page_size - page_offset, size);
		write_op.addr.val = addr;
		write_op.data.dir = QSPI_MEM_DATA_OUT;
		write_op.data.nbytes = to_write;
		write_op.data.buf.out = (const uint8_t *)buf;
		ret = qspi_write_enable(dev);
		if (ret < 0) {
			break;
		}

		ret = qspi_exec_op(&hqspi, &write_op);
		if (ret < 0) {
			break;
		}

		ret = qspi_wait_until_ready(dev);
		if (ret < 0) {
			LOG_DBG("Flash failed to become ready after writing ready bit: %d", ret);
			return ret;
		}
		buf = (const uint8_t *)buf + to_write;
		addr += to_write;
		size -= to_write;
	}
	k_mutex_unlock(&data->mutex);

	return ret;
}

#if defined(CONFIG_FLASH_JESD216_API)
/*
 * Read Serial Flash ID :
 * perform a read access over SPI bus for read Identification
 */
static int sam_qspi_read_jedec_id(const struct device *dev, uint8_t *id)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op op = {0};

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	op.proto = SFLASH_PROTO_1_1_1;
	op.cmd.waitstates = 8;
	op.cmd.opcode = JESD216_CMD_READ_ID;
	op.data.dir = QSPI_MEM_DATA_IN;
	op.data.nbytes = JESD216_READ_ID_LEN;
	op.data.buf.in = id;

	return qspi_exec_op(&hqspi, &op);
}
#endif

/*
 * Read Serial Flash Discovery Parameter
 */
static int sam_qspi_read_sfdp(const struct device *dev, off_t addr, void *data,
			  size_t size)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct qspi_priv hqspi = {0};
	struct qspi_mem_op op = {0};

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	op.proto = SFLASH_PROTO_1_1_1;
	op.cmd.waitstates = 8;
	op.cmd.opcode = JESD216_CMD_READ_SFDP;
	op.addr.nbytes = 3;
	op.addr.val = addr;
	op.data.dir = QSPI_MEM_DATA_IN;
	op.data.nbytes = size;
	op.data.buf.in = data;

	return qspi_exec_op(&hqspi, &op);
}

static int spi_nor_process_bfp(const struct device *dev, const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	struct flash_sam_nor_data *data = dev->data;
	struct jesd216_erase_type *etp = data->erase_types;
	const size_t flash_size = (jesd216_bfp_density(bfp) / 8U);
	int rc;

	data->size = (uint32_t)(flash_size);
	data->addr_len = 3;
	if (data->size > 0x01000000) {
		data->addr_len = 4;
	}

	data->page_size = jesd216_bfp_page_size(php, bfp);

	/* Copy over the erase types, preserving their order.  (The
	 * Sector Map Parameter table references them by index.)
	 */
	memset(data->erase_types, 0, sizeof(data->erase_types));
	for (uint8_t ti = 1; ti <= ARRAY_SIZE(data->erase_types); ++ti) {
		if ((jesd216_bfp_erase(bfp, ti, etp) == 0) &&
			(data->addr_len == 4)) {
			etp->cmd = spi_nor_convert_3to4_erase(etp->cmd);
		}
		++etp;
	}

	/*
	 * Only check if the 1-4-4 (i.e. 4READ) or 1-1-4 (QREAD)
	 * is supported - other modes are not.
	 */
	const enum jesd216_mode_type supported_modes[] = {JESD216_MODE_114,
							  JESD216_MODE_144};
	struct jesd216_bfp_dw15 dw15;
	struct jesd216_instr res;

	/* reset active mode */
	data->read_inst = SPI_NOR_CMD_READ;
	data->read_proto = SFLASH_PROTO_1_1_1;
	data->num_wait_states = 0;
	data->num_mode_cycles = 0;

	/* query supported read modes, begin from the slowest */
	for (size_t i = 0; i < ARRAY_SIZE(supported_modes); ++i) {
		rc = jesd216_bfp_read_support(php, bfp, supported_modes[i], &res);
		if (rc >= 0) {
			data->read_proto =
				((supported_modes[i] == JESD216_MODE_114) ?
				SFLASH_PROTO_1_1_4 : SFLASH_PROTO_1_4_4);
			data->read_inst = res.instr;
			data->num_wait_states = res.wait_states;
			data->num_mode_cycles = res.mode_clocks;
		}

		if (data->addr_len == 4) {
			data->read_inst = spi_nor_convert_3to4_read(data->read_inst);
		}

		/* try to decode QE requirement type */
		rc = jesd216_bfp_decode_dw15(php, bfp, &dw15);
		if (rc < 0) {
			/* will use QER from DTS or default (refer to device data) */
			LOG_INF("Unable to decode QE requirement [DW15]: %d\n", rc);
		} else {
			/* bypass DTS QER value */
			data->qer_type = dw15.qer;
		}

		/* enable QE */
		rc = qspi_quad_enable(dev);
		if (rc < 0) {
			LOG_ERR("Failed to enable Quad mode: %d\n", rc);
			return rc;
		}

		data->write_inst = SPI_NOR_CMD_PP;

		if (data->addr_len == 4) {
			data->write_inst = SPI_NOR_CMD_PP_4B;
		}

		if (IS_ENABLED(DT_INST_PROP(0, requires_ulbpr))) {
			qspi_write_unprotect(dev);
		}
	}

	return 0;
}

static int nor_sam_init_params(const struct device *dev)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct flash_sam_nor_data *data = dev->data;
	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;
	int ret;

	if (config->skip_sfdp) {
		goto no_sfdp;
	}

	ret = sam_qspi_read_sfdp(dev, 0, u.raw, sizeof(u.raw));
	if (ret != 0) {
		LOG_ERR("SFDP read failed: %d", ret);
		return ret;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	LOG_INF("jesd216_sfdp_magic: %x\n", magic);
	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return -EINVAL;
	}

	LOG_INF("%s: SFDP v %u.%u AP %x with %u PH", dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php +
							MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);

		LOG_INF("PH%u: %04x rev %u.%u: %u DW @ %x",
			(php - hp->phdr), id, php->rev_major, php->rev_minor,
			php->len_dw, jesd216_param_addr(php));

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[20];
				struct jesd216_bfp bfp;
			} u2;
			const struct jesd216_bfp *bfp = &u2.bfp;

			ret = sam_qspi_read_sfdp(dev, jesd216_param_addr(php),
					     (uint8_t *)u2.dw,
					     MIN(sizeof(uint32_t) * php->len_dw, sizeof(u2.dw)));
			if (ret == 0) {
				ret = spi_nor_process_bfp(dev, php, bfp);
			} else {
				LOG_ERR("SFDP BFP failed: %d", ret);
				break;
			}
		}
		++php;
	}

	return ret;

no_sfdp:
	if ((uint32_t)(config->flash_size) == 0) {
		LOG_ERR("requires 'size' node");
		return -ENODEV;
	}

	data->size = (uint32_t)(config->flash_size);
	data->addr_len = 3;
	if (data->size > 0x01000000) {
		data->addr_len = 4;
	}

	data->read_proto = SFLASH_PROTO_1_1_1;
	data->num_mode_cycles = 0;
	data->num_wait_states = 0;
	memset(data->erase_types, 0, sizeof(data->erase_types));

	/* Default to Fast Read, PP and sector erase for non-SFDP */
	if (data->addr_len == 3) {
		data->read_inst = SPI_NOR_CMD_READ;
		data->write_inst = SPI_NOR_CMD_PP;
		data->erase_types[0].cmd = SPI_NOR_CMD_SE;
		data->erase_types[0].exp = 0xC;
	} else {
		data->read_inst = SPI_NOR_CMD_READ_4B;
		data->write_inst = SPI_NOR_CMD_PP_4B;
		data->erase_types[0].cmd = SPI_NOR_CMD_SE_4B;
		data->erase_types[0].exp = 0xC;
	}

	return 0;
}

static int flash_sam_nor_init(const struct device *dev)
{
	const struct flash_sam_qspi_config *config = dev->config;
	struct flash_sam_nor_data *data = dev->data;
	struct qspi_priv hqspi = {0};
	int ret;

	hqspi.base = config->qspi_base;
	hqspi.mem = config->qspi_mem;
	hqspi.dma = config->dma_dev;
	hqspi.dma_channel = config->dma_channel;

	/* Connect pins to the peripheral */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s: pinctrl_apply_state() => %d", __func__, ret);
		return ret;
	}

	/* Enable module's clock */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&config->clock_cfg);

	k_mutex_init(&data->mutex);
	if (!device_is_ready(hqspi.dma)) {
		LOG_ERR("dma controller device is not ready\n");
		return -ENODEV;
	}

	ret = qspi_sama7g5_init(&hqspi);
	if (ret < 0) {
		LOG_ERR("%s: qspi_sama7g5_init() => %d", __func__, ret);
		return ret;
	}

	/* Parse the Serial Flash Discoverable Parameter tables. */
	ret = nor_sam_init_params(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(flash, flash_sam_nor_api) = {
	.read = flash_sam_qspi_read,
	.write = flash_sam_qspi_write,
	.erase = flash_sam_qspi_erase,
	.get_parameters = flash_sam_qspi_get_parameters,
	.get_size = flash_sam_qspi_get_size,
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = sam_qspi_read_sfdp,
	.read_jedec_id = sam_qspi_read_jedec_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#define FLASH_SAM_QSPI(N)									\
	PINCTRL_DT_INST_DEFINE(N);								\
	static const struct flash_sam_qspi_config						\
		flash_sam_qspi_config_##N = {							\
			.qspi_base = (qspi_registers_t *)DT_INST_REG_ADDR_BY_IDX(N, 0),		\
			.qspi_mem = DT_INST_REG_ADDR_BY_IDX(N, 1),				\
			.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(N),				\
			.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(N),				\
			.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(N, qspi_dma)),	\
			.dma_channel = DT_INST_DMAS_CELL_BY_NAME(N, qspi_dma, channel),		\
			.skip_sfdp = DT_INST_PROP(N, skip_sfdp),				\
			.flash_size = DT_INST_PROP(N, size),					\
	};											\
	static struct flash_sam_nor_data flash_sam_nor_data_##N = {};				\
	DEVICE_DT_INST_DEFINE(N,								\
			      flash_sam_nor_init,						\
			      NULL,								\
			      &flash_sam_nor_data_##N,						\
			      &flash_sam_qspi_config_##N,					\
			      POST_KERNEL,							\
			      CONFIG_FLASH_INIT_PRIORITY,					\
			      &flash_sam_nor_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_SAM_QSPI)
