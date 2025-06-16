/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_nor

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/nct_flash_api_ex.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/flash_controller/nct_qspi.h>
#include <soc.h>
#ifdef CONFIG_USERSPACE
#include <zephyr/syscall.h>
#include <zephyr/internal/syscall_handler.h>
#endif

#include "flash_nct_qspi.h"
#include "spi_nor.h"
#include "gdma.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_nct_nor, CONFIG_FLASH_LOG_LEVEL);

#ifdef CONFIG_XIP
	#include <zephyr/linker/linker-defs.h>
	#define RAMFUNC __attribute__ ((section(".ramfunc")))
#else
	#define RAMFUNC
#endif

#define BLOCK_64K_SIZE KB(64)
#define BLOCK_4K_SIZE  KB(4)
#define MAPPED_ADDR_NOT_SUPPORT	0xffffffff

/* Device config */
struct flash_nct_nor_config {
	/* QSPI bus device for mutex control and bus configuration */
	const struct device *qspi_bus;
	/* Mapped address for flash read via direct access */
	uintptr_t mapped_addr;
	/* Size of nor device in bytes, from size property */
	uint32_t flash_size;
	/* Maximum chip erase time-out in ms */
	uint32_t max_timeout;
	/* SPI Nor device configuration on QSPI bus */
	struct nct_qspi_cfg qspi_cfg;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

/* Device data */
struct flash_nct_nor_data {
	/* Specific control operation for Quad-SPI Nor Flash */
	uint32_t operation;
};

static const struct flash_parameters flash_nct_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff,
};

#define DT_INST_QUAD_EN_PROP_OR(inst)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, quad_enable_requirements),	\
		    (_CONCAT(JESD216_DW15_QER_VAL_,				\
		     DT_INST_STRING_TOKEN(inst, quad_enable_requirements))),	\
		    ((JESD216_DW15_QER_VAL_NONE)))

RAMFUNC static inline bool is_within_region(off_t addr, size_t size, off_t region_start,
				    size_t region_size)
{
	return (addr >= region_start &&
		(addr < (region_start + region_size)) &&
		((addr + size) <= (region_start + region_size)));
}

RAMFUNC static int flash_nct_transceive(const struct device *dev, struct nct_transceive_cfg *cfg,
				     uint32_t flags)
{
	const struct flash_nct_nor_config *config = dev->config;
	struct flash_nct_nor_data *data = dev->data;
	struct nct_qspi_data *qspi_data = config->qspi_bus->data;
	int ret;

#ifdef CONFIG_XIP
	unsigned int key = 0;

	/* Disable IRQ */
 	key = irq_lock();
#endif

	/* Lock SPI bus and configure it if needed */
	qspi_data->qspi_ops->lock_configure(config->qspi_bus, &config->qspi_cfg,
				       data->operation);

	/* Execute transaction */
	ret = qspi_data->qspi_ops->transceive(config->qspi_bus, cfg, flags);

	/* Unlock SPI bus */
	qspi_data->qspi_ops->unlock(config->qspi_bus);

#ifdef CONFIG_XIP
 	irq_unlock(key);
#endif

	return ret;
}

/* NCT functions for SPI NOR flash */
RAMFUNC static int flash_nct_transceive_cmd_only(const struct device *dev, uint8_t opcode)
{
	struct nct_transceive_cfg cfg = { .opcode = opcode};

	return flash_nct_transceive(dev, &cfg, 0); /* opcode only */
}

RAMFUNC static int flash_nct_transceive_cmd_by_addr(const struct device *dev, uint8_t opcode,
					     uint32_t addr)
{
	struct nct_transceive_cfg cfg = { .opcode = opcode};

	cfg.addr.u32 = sys_cpu_to_be32(addr);
	return flash_nct_transceive(dev, &cfg, NCT_TRANSCEIVE_ACCESS_ADDR);
}

RAMFUNC static int flash_nct_transceive_read_by_addr(const struct device *dev, uint8_t opcode,
					      uint8_t *dst, const size_t size, uint32_t addr)
{
	struct nct_transceive_cfg cfg = { .opcode = opcode,
					  .rx_buf = dst,
					  .rx_count = size};

	cfg.addr.u32 = sys_cpu_to_be32(addr);
	return flash_nct_transceive(dev, &cfg, NCT_TRANSCEIVE_ACCESS_READ |
					NCT_TRANSCEIVE_ACCESS_ADDR);
}

RAMFUNC static int flash_nct_transceive_read(const struct device *dev, uint8_t opcode,
			       uint8_t *dst, const size_t size)
{
	struct nct_transceive_cfg cfg = { .opcode = opcode,
					  .rx_buf = dst,
					  .rx_count = size};

	return flash_nct_transceive(dev, &cfg, NCT_TRANSCEIVE_ACCESS_READ);
}

RAMFUNC static int flash_nct_transceive_write(const struct device *dev, uint8_t opcode,
				       uint8_t *src, const size_t size)
{
	struct nct_transceive_cfg cfg = { .opcode = opcode,
				    .tx_buf = src,
				    .tx_count = size};

	return flash_nct_transceive(dev, &cfg, NCT_TRANSCEIVE_ACCESS_WRITE);
}

RAMFUNC static int flash_nct_transceive_write_by_addr(const struct device *dev, uint8_t opcode,
					       uint8_t *src, const size_t size, uint32_t addr)
{
	struct nct_transceive_cfg cfg = { .opcode = opcode,
					.tx_buf = src,
					.tx_count = size};

	cfg.addr.u32 = sys_cpu_to_be32(addr);
	return flash_nct_transceive(dev, &cfg, NCT_TRANSCEIVE_ACCESS_WRITE |
					 NCT_TRANSCEIVE_ACCESS_ADDR);
}

/* Local SPI NOR flash functions */
RAMFUNC static int flash_nct_nor_wait_until_ready(const struct device *dev)
{
	int ret;
	uint8_t reg;
	const struct flash_nct_nor_config *config = dev->config;
	int64_t st = k_uptime_get();

	do {
		ret = flash_nct_transceive_read(dev, SPI_NOR_CMD_RDSR, &reg, sizeof(reg));
		if (ret != 0) {
			return ret;
		} else if ((reg & SPI_NOR_WIP_BIT) == 0) {
			return 0;
		}

	} while ((k_uptime_get() - st) < config->max_timeout);

	return -EBUSY;
}

RAMFUNC static int flash_nct_nor_read_status_regs(const struct device *dev, uint8_t *sts_reg)
{
	int ret = flash_nct_transceive_read(dev, SPI_NOR_CMD_RDSR, sts_reg, 1);

	if (ret != 0) {
		return ret;
	}
	
	return flash_nct_transceive_read(dev, SPI_NOR_CMD_RDSR2, sts_reg + 1, 1);
}

RAMFUNC static int flash_nct_nor_write_status_regs(const struct device *dev, uint8_t *sts_reg)
{
	int ret;

#ifdef CONFIG_XIP
	unsigned int key = 0;
	
	key = irq_lock();
#endif

	ret = flash_nct_transceive_cmd_only(dev, SPI_NOR_CMD_WREN);
	if (ret != 0) {
		goto exit;
	}

	ret = flash_nct_transceive_write(dev, SPI_NOR_CMD_WRSR, sts_reg, 2);
	if (ret != 0) {
		goto exit;
	}

	ret = flash_nct_nor_wait_until_ready(dev);

exit:

#ifdef CONFIG_XIP
	irq_unlock(key);
#endif

	return ret;
}

/* Flash API functions */
#if defined(CONFIG_FLASH_JESD216_API)
RAMFUNC static int flash_nct_nor_read_jedec_id(const struct device *dev, uint8_t *id)
{
	if (id == NULL) {
		return -EINVAL;
	}

	return flash_nct_transceive_read(dev, SPI_NOR_CMD_RDID, id, SPI_NOR_MAX_ID_LEN);
}

RAMFUNC static int flash_nct_nor_read_sfdp(const struct device *dev, off_t addr,
				    void *data, size_t size)
{
	uint8_t sfdp_addr[4];
	struct nct_transceive_cfg cfg = { .opcode = JESD216_CMD_READ_SFDP,
					.tx_buf = sfdp_addr,
					.tx_count = 4,
					.rx_buf = data,
					.rx_count = size};

	if (data == NULL) {
		return -EINVAL;
	}

	/* CMD_READ_SFDP needs a 24-bit address followed by a dummy byte */
	sfdp_addr[0] = (addr >> 16) & 0xff;
	sfdp_addr[1] = (addr >> 8) & 0xff;
	sfdp_addr[2] = addr & 0xff;
	return flash_nct_transceive(dev, &cfg, NCT_TRANSCEIVE_ACCESS_WRITE |
					 NCT_TRANSCEIVE_ACCESS_READ);
}
#endif /* CONFIG_FLASH_JESD216_API */

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
RAMFUNC static void flash_nct_nor_pages_layout(const struct device *dev,
					const struct flash_pages_layout **layout,
					size_t *layout_size)
{
	const struct flash_nct_nor_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

RAMFUNC static int flash_nct_nor_read(const struct device *dev, off_t addr,
				 void *data, size_t size)
{
	const struct flash_nct_nor_config *config = dev->config;
	struct flash_nct_nor_data *dev_data = dev->data;
	struct nct_qspi_data *qspi_data = config->qspi_bus->data;

	/* Out of the region of nor flash device? */
	if (!is_within_region(addr, size, 0, config->flash_size)) {
		return -EINVAL;
	}

	/* handle by transceive, use genernal Read I/O */
	if (config->mapped_addr == MAPPED_ADDR_NOT_SUPPORT) {
		switch (config->qspi_cfg.rd_mode) {
			case NCT_RD_MODE_FAST:
				return flash_nct_transceive_read_by_addr(dev,
								SPI_NOR_CMD_DREAD, data, size, addr);
				break;
			case NCT_RD_MODE_FAST_DUAL:
				return flash_nct_transceive_read_by_addr(dev,
								SPI_NOR_CMD_2READ, data, size, addr);
				break;
			case NCT_RD_MODE_QUAD:
				return flash_nct_transceive_read_by_addr(dev,
								SPI_NOR_CMD_4READ, data, size, addr);
				break;
			default:
				return flash_nct_transceive_read_by_addr(dev,
								SPI_NOR_CMD_READ, data, size, addr);
				break;
		}
	}

	/* Lock/Unlock SPI bus also for DRA mode */
	qspi_data->qspi_ops->lock_configure(config->qspi_bus, &config->qspi_cfg,
				       dev_data->operation);

	/* Trigger Direct Read Access (DRA) via reading memory mapped-address */
	gdma_memcpy_burst_u32(data, (void *)(config->mapped_addr + addr), size);

	qspi_data->qspi_ops->unlock(config->qspi_bus);

	return 0;
}

RAMFUNC static int flash_nct_nor_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_nct_nor_config *config = dev->config;
	int ret = 0;

#ifdef CONFIG_XIP
	unsigned int key = 0;
#endif

	/* Out of the region of nor flash device? */
	if (!is_within_region(addr, size, 0, config->flash_size)) {
		LOG_ERR("Addr %ld, size %d are out of range", addr, size);
		return -EINVAL;
	}

	/* address must be sector-aligned */
	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		LOG_ERR("Addr %ld is not sector-aligned", addr);
		return -EINVAL;
	}

	/* size must be a multiple of sectors */
	if ((size % BLOCK_4K_SIZE) != 0) {
		LOG_ERR("Size %d is not a multiple of sectors", size);
		return -EINVAL;
	}

#ifdef CONFIG_XIP
	/* if execute in the place, disable IRQ to avoid interrupt and context
	 * switch, to make sure always execute code in the RAM(SRAM) when use
	 * spim driver. nct6xx only have one core, we don't need use lock to
	 * handle race condition.
	 */
	key = irq_lock();
#endif

	/* Select erase opcode by size */
	if (size == config->flash_size) {
		flash_nct_transceive_cmd_only(dev, SPI_NOR_CMD_WREN);
		/* Send chip erase command */
		flash_nct_transceive_cmd_only(dev, SPI_NOR_CMD_CE);
		return flash_nct_nor_wait_until_ready(dev);
	}

	while (size > 0) {
		flash_nct_transceive_cmd_only(dev, SPI_NOR_CMD_WREN);
		/* Send page/block erase command with addr */
		if ((size >= BLOCK_64K_SIZE) && SPI_NOR_IS_64K_ALIGNED(addr)) {
			flash_nct_transceive_cmd_by_addr(dev, SPI_NOR_CMD_BE, addr);
			addr += BLOCK_64K_SIZE;
			size -= BLOCK_64K_SIZE;
		} else {
			flash_nct_transceive_cmd_by_addr(dev, SPI_NOR_CMD_SE, addr);
			addr += BLOCK_4K_SIZE;
			size -= BLOCK_4K_SIZE;
		}
		ret = flash_nct_nor_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}
	}

#ifdef CONFIG_XIP
	irq_unlock(key);
#endif

	return ret;
}

uintptr_t nct6xx_vector_table_save(void);
void nct6xx_vector_table_restore(uintptr_t vtor);

RAMFUNC static int flash_nct_nor_write(const struct device *dev, off_t addr,
				  const void *data, size_t size)
{
	const struct flash_nct_nor_config *config = dev->config;
	uint8_t *tx_buf = (uint8_t *)data;
	int ret = 0;
	size_t sz_write;

#ifdef CONFIG_XIP
	unsigned int key = 0;
#endif

	/* Out of the region of nor flash device? */
	if (!is_within_region(addr, size, 0, config->flash_size)) {
		return -EINVAL;
	}

	/* Don't write more than a page. */
	if (size > SPI_NOR_PAGE_SIZE) {
		sz_write = SPI_NOR_PAGE_SIZE;
	} else {
		sz_write = size;
	}

	/*
	 * Correct the size of first write to not go through page boundary and
	 * make the address of next write to align to page boundary.
	 */
	if (((addr + sz_write - 1U) / SPI_NOR_PAGE_SIZE) != (addr / SPI_NOR_PAGE_SIZE)) {
		sz_write -= (addr + sz_write) & (SPI_NOR_PAGE_SIZE - 1);
	}

#ifdef CONFIG_XIP
	key = irq_lock();
#endif

	while (size > 0) {
		/* Start to write */
		flash_nct_transceive_cmd_only(dev, SPI_NOR_CMD_WREN);
		ret = flash_nct_transceive_write_by_addr(dev, SPI_NOR_CMD_PP, tx_buf,
						   sz_write, addr);
		if (ret != 0) {
			break;
		}

		/* Wait for writing completed */
		ret = flash_nct_nor_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}

		size -= sz_write;
		tx_buf += sz_write;
		addr += sz_write;

		if (size > SPI_NOR_PAGE_SIZE) {
			sz_write = SPI_NOR_PAGE_SIZE;
		} else {
			sz_write = size;
		}
	}

#ifdef CONFIG_XIP
	irq_unlock(key);
#endif

	return ret;
}

RAMFUNC static const struct flash_parameters *
flash_nct_nor_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_nct_parameters;
};

#ifdef CONFIG_FLASH_EX_OP_ENABLED
RAMFUNC static int flash_nct_nor_ex_exec_transceive(const struct device *dev,
				      const struct nct_ex_ops_transceive_in *op_in,
				      const struct nct_ex_ops_transceive_out *op_out)
{
	int flag = 0;
	struct nct_transceive_cfg cfg;

	if (op_in == NULL) {
		return -EINVAL;
	}

	/* Organize a transaction */
	cfg.opcode = op_in->opcode;
	if (op_in->tx_count != 0) {
		cfg.tx_buf = op_in->tx_buf;
		cfg.tx_count = op_in->tx_count;
		flag |= NCT_TRANSCEIVE_ACCESS_WRITE;
	}

	if (op_in->addr_count != 0) {
		cfg.addr.u32 = sys_cpu_to_be32(op_in->addr);
		flag |= NCT_TRANSCEIVE_ACCESS_ADDR;
	}

	if (op_out != NULL && op_in->rx_count != 0) {
		cfg.rx_buf = op_out->rx_buf;
		cfg.rx_count = op_in->rx_count;
		flag |= NCT_TRANSCEIVE_ACCESS_READ;
	}

	return flash_nct_transceive(dev, &cfg, flag);
}

RAMFUNC static int flash_nct_nor_ex_set_spi_spec(const struct device *dev,
					  const struct nct_ex_ops_qspi_oper_in *op_in)
{
	struct flash_nct_nor_data *data = dev->data;

	if (op_in->enable) {
		data->operation |= op_in->mask;
	} else {
		data->operation &= ~op_in->mask;
	}

	return 0;
}

RAMFUNC static int flash_nct_nor_ex_get_spi_spec(const struct device *dev,
					  struct nct_ex_ops_qspi_oper_out *op_out)
{
	struct flash_nct_nor_data *data = dev->data;

	op_out->oper = data->operation;
	return 0;
}

RAMFUNC static int flash_nct_nor_ex_op(const struct device *dev, uint16_t code,
				const uintptr_t in, void *out)
{
#ifdef CONFIG_USERSPACE
	bool syscall_trap = z_syscall_trap();
#endif
	int ret;

	switch (code) {
	case FLASH_NCT_EX_OP_EXEC_TRANSCEIVE:
	{
		struct nct_ex_ops_transceive_in *op_in =
			(struct nct_ex_ops_transceive_in *)in;
		struct nct_ex_ops_transceive_out *op_out =
			(struct nct_ex_ops_transceive_out *)out;
#ifdef CONFIG_USERSPACE
		struct nct_ex_ops_transceive_in in_copy;
		struct nct_ex_ops_transceive_out out_copy;

		if (syscall_trap) {
			K_OOPS(k_usermode_from_copy(&in_copy, op_in, sizeof(in_copy)));
			op_in = &in_copy;
			op_out = &out_copy;
		}
#endif

		ret = flash_nct_nor_ex_exec_transceive(dev, op_in, op_out);
#ifdef CONFIG_USERSPACE
		if (ret == 0 && syscall_trap) {
			K_OOPS(k_usermode_to_copy(out, op_out, sizeof(out_copy)));
		}
#endif
		break;
	}
	case FLASH_NCT_EX_OP_SET_QSPI_OPER:
	{
		struct nct_ex_ops_qspi_oper_in *op_in = (struct nct_ex_ops_qspi_oper_in *)in;
#ifdef CONFIG_USERSPACE
		struct nct_ex_ops_qspi_oper_in in_copy;

		if (syscall_trap) {
			K_OOPS(k_usermode_from_copy(&in_copy, op_in, sizeof(in_copy)));
			op_in = &in_copy;
		}
#endif
		ret = flash_nct_nor_ex_set_spi_spec(dev, op_in);
		break;
	}
	case FLASH_NCT_EX_OP_GET_QSPI_OPER:
	{
		struct nct_ex_ops_qspi_oper_out *op_out =
		(struct nct_ex_ops_qspi_oper_out *)out;
#ifdef CONFIG_USERSPACE
		struct nct_ex_ops_qspi_oper_out out_copy;

		if (syscall_trap) {
			op_out = &out_copy;
		}
#endif
		ret = flash_nct_nor_ex_get_spi_spec(dev, op_out);
#ifdef CONFIG_USERSPACE
		if (ret == 0 && syscall_trap) {
			K_OOPS(k_usermode_to_copy(out, op_out, sizeof(out_copy)));
		}
#endif
		break;
	}
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

static const struct flash_driver_api flash_nct_nor_driver_api = {
	.read = flash_nct_nor_read,
	.write = flash_nct_nor_write,
	.erase = flash_nct_nor_erase,
	.get_parameters = flash_nct_nor_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_nct_nor_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_nct_nor_read_sfdp,
	.read_jedec_id = flash_nct_nor_read_jedec_id,
#endif
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_nct_nor_ex_op,
#endif
};

RAMFUNC static int flash_nct_nor_init(const struct device *dev)
{
	const struct flash_nct_nor_config *config = dev->config;
	int ret;

	if (!IS_ENABLED(CONFIG_FLASH_NCT_NOR_INIT)) {
		return 0;
	}

	/* Enable quad access of spi NOR flash */
	if (config->qspi_cfg.qer_type != JESD216_DW15_QER_NONE) {
		uint8_t qe_idx, qe_bit, sts_reg[2];
		/* Read status registers first */
		ret = flash_nct_nor_read_status_regs(dev, sts_reg);
		if (ret != 0) {
			LOG_ERR("Enable quad access: read reg failed %d!", ret);
			return ret;
		}
		switch (config->qspi_cfg.qer_type) {
		case JESD216_DW15_QER_S1B6:
			qe_idx = 1;
			qe_bit = 6;
			break;
		case JESD216_DW15_QER_S2B1v1:
			__fallthrough;
		case JESD216_DW15_QER_S2B1v4:
			__fallthrough;
		case JESD216_DW15_QER_S2B1v5:
			qe_idx = 2;
			qe_bit = 1;
			break;
		default:
			return -ENOTSUP;
		}
		/* Set QE bit in status register */
		if(sts_reg[qe_idx - 1] & BIT(qe_bit)) {
			ret = 0;
		}
		else {
			sts_reg[qe_idx - 1] |= BIT(qe_bit);
			ret = flash_nct_nor_write_status_regs(dev, sts_reg);
			if (ret != 0) {
				LOG_ERR("Enable quad access: write reg failed %d!", ret);
				return ret;
			}
		}
	}

	/* Enable 4-byte address of spi NOR flash */
	if (config->qspi_cfg.enter_4ba != 0) {
		bool wr_en = (config->qspi_cfg.enter_4ba & 0x02) != 0;

		if (wr_en) {
			ret = flash_nct_transceive_cmd_only(dev, SPI_NOR_CMD_WREN);
			if (ret != 0) {
				LOG_ERR("Enable 4byte addr: WREN failed %d!", ret);
				return ret;
			}
		}
		ret = flash_nct_transceive_cmd_only(dev, SPI_NOR_CMD_4BA);
		if (ret != 0) {
			LOG_ERR("Enable 4byte addr: 4BA failed %d!", ret);
			return ret;
		}
	}

	return 0;
}

#define NCT_FLASH_NOR_INIT(n)							\
BUILD_ASSERT(!(DT_INST_QUAD_EN_PROP_OR(n) == JESD216_DW15_QER_NONE &&		\
			  DT_INST_STRING_TOKEN(n, rd_mode) == NCT_RD_MODE_QUAD),	\
	     	  "[quad-enable-requirements] must be selected in Quad mode");		\
BUILD_ASSERT(!((DT_REG_ADDR(DT_INST_PARENT(n)) == 0x40020000 || \
			  DT_REG_ADDR(DT_INST_PARENT(n)) == 0x40017000) && \
			  DT_INST_STRING_TOKEN(n, rd_mode) != NCT_RD_MODE_NORMAL && \
			  DT_INST_PROP_OR(n, mapped_addr, MAPPED_ADDR_NOT_SUPPORT) == MAPPED_ADDR_NOT_SUPPORT),	\
			  "FIU and SPIM only supports normal mode in non-DMM mode!"); \
PINCTRL_DT_INST_DEFINE(n);							\
static struct flash_nct_nor_config flash_nct_nor_config_##n = {		\
	.qspi_bus = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),			\
	.mapped_addr = DT_INST_PROP_OR(n, mapped_addr, MAPPED_ADDR_NOT_SUPPORT),\
	.flash_size = DT_INST_PROP(n, size) / 8,				\
	.max_timeout = DT_INST_PROP(n, max_timeout),				\
	.qspi_cfg = {								\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.flags = DT_INST_PROP(n, qspi_flags),				\
		.enter_4ba = DT_INST_PROP_OR(n, enter_4byte_addr, 0),		\
		.qer_type = DT_INST_QUAD_EN_PROP_OR(n),				\
		.rd_mode = DT_INST_STRING_TOKEN(n, rd_mode),			\
	},									\
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (					\
		.layout = {							\
			.pages_count = DT_INST_PROP(n, size) /			\
				      (8 * SPI_NOR_PAGE_SIZE),			\
			.pages_size  = SPI_NOR_PAGE_SIZE,			\
		},))								\
};										\
static struct flash_nct_nor_data flash_nct_nor_data_##n;			\
DEVICE_DT_INST_DEFINE(n, flash_nct_nor_init, NULL,				\
		      &flash_nct_nor_data_##n, &flash_nct_nor_config_##n,	\
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,			\
		      &flash_nct_nor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NCT_FLASH_NOR_INIT)
