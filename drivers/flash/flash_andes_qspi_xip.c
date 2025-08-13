/*
 * Copyright 2025 The ChromiumOS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This driver is inspired from the flash_andes_qspi.c flash driver.
 *
 */

#define DT_DRV_COMPAT andestech_qspi_nor_xip

#include "soc_v5.h"

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/drivers/flash/andes_flash_xip_api_ex.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "flash_andes_qspi.h"
#include "spi_nor.h"

LOG_MODULE_REGISTER(flash_andes_xip, CONFIG_FLASH_LOG_LEVEL);

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define ANDES_ACCESS_ADDRESSED BIT(0)

/* Indicates that an access command is performing a write.
 * If not provided access is a read.
 */
#define ANDES_ACCESS_WRITE BIT(1)

/* Set max write size to page size. */
#define PAGE_SIZE 256

#define MMISC_CTL_BRPE_EN BIT(3)

#define MEMCTRL_CHG BIT(8)

struct atcspi200_regs {
	/* 0x00 */
	volatile uint32_t ID;
	/* 0x04 */
	volatile uint8_t reserver_04_0F[12];
	/* 0x10 */
	volatile uint32_t TFMAT;
	/* 0x14 */
	volatile uint32_t DIOCTRL;
	/* 0x18 */
	volatile uint32_t WRCNT;
	/* 0x1C */
	volatile uint32_t RDCNT;
	/* 0x20 */
	volatile uint32_t TCTRL;
	/* 0x24 */
	volatile uint32_t CMD;
	/* 0x28 */
	volatile uint32_t ADDR;
	/* 0x2C */
	volatile uint32_t DATA;
	/* 0x30 */
	volatile uint32_t CTRL;
	/* 0x34 */
	volatile uint32_t STATUS;
	/* 0x38 */
	volatile uint32_t INTEN;
	/* 0x3C */
	volatile uint32_t INTST;
	/* 0x40 */
	volatile uint32_t IFTIM;
	/* 0x44 */
	volatile uint8_t reserver_44_4F[12];
	/* 0x50 */
	volatile uint32_t MEMCTRL;
	/* 0x54 */
	volatile uint8_t reserver_54_5F[12];
	/* 0x60 */
	volatile uint32_t SLVSR;
	/* 0x64 */
	volatile uint32_t SLVCOUNT;
	/* 0x68 */
	volatile uint8_t reserver_68_7B[20];
	/* 0x7C */
	volatile uint32_t CONF;
};

struct flash_andes_qspi_xip_config {
	struct flash_parameters parameters;
	struct atcspi200_regs *regs;
	uint32_t mapped_base;
	uint32_t flash_size;
	bool is_xip;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

struct flash_andes_qspi_xip_data {
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	/* Lock of the status registers. */
	bool status_lock;
#endif
};

#define flash_andes_qspi_xip_cmd_read(dev, opcode, dest, length)                                   \
	flash_andes_qspi_xip_access(dev, opcode, 0, 0, dest, length)
#define flash_andes_qspi_xip_cmd_write_data(dev, opcode, src, length)                              \
	flash_andes_qspi_xip_access(dev, opcode, ANDES_ACCESS_WRITE, 0, src, length)
#define flash_andes_qspi_xip_cmd_write(dev, opcode)                                                \
	flash_andes_qspi_xip_access(dev, opcode, ANDES_ACCESS_WRITE, 0, NULL, 0)
#define flash_andes_qspi_xip_cmd_addr_read(dev, opcode, addr, dest, length)                        \
	flash_andes_qspi_xip_access(dev, opcode, ANDES_ACCESS_ADDRESSED, addr, dest, length)
#define flash_andes_qspi_xip_cmd_addr_write(dev, opcode, addr, src, length)                        \
	flash_andes_qspi_xip_access(dev, opcode, ANDES_ACCESS_WRITE | ANDES_ACCESS_ADDRESSED,      \
				    addr, (void *)src, length)

#define STAT_TXFULL    BIT(23)
#define STAT_RXEMPTY   BIT(14)
#define STAT_SPIACTIVE BIT(0)

static __ramfunc void handle_data_transfer(bool is_write, uint8_t *data, size_t length,
					   struct atcspi200_regs *regs)
{
	size_t processed = 0;

	/* Data Merge is disabled. */
	if (is_write) {
		while (length > processed) {
			if (!(regs->STATUS & STAT_TXFULL)) {
				regs->DATA = data[processed];
				processed++;
			}
		}
	} else {
		while (length > processed) {
			if (!(regs->STATUS & STAT_RXEMPTY)) {
				data[processed] = regs->DATA;
				processed++;
			}
		}
	}
}

/*
 * @brief Send an SPI command
 *
 * @param dev Device struct
 * @param opcode The command to send
 * @param access flags that determine how the command is constructed.
 * @param addr The address to send
 * @param data The buffer to store or read transferred data
 * @param length The size of the buffer
 * @return 0 on success
 */
static __ramfunc int flash_andes_qspi_xip_access(const struct device *const dev, uint8_t opcode,
						 uint8_t access, off_t addr, void *data,
						 size_t length)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;
	struct atcspi200_regs *regs = config->regs;
	bool is_write = (access & ANDES_ACCESS_WRITE) != 0;
	uint32_t tctrl;
	uint32_t tfmat;

	if ((length > 0) && (data == NULL)) {
		return -EINVAL;
	}

	/* Wait till previous SPI transfer is finished. */
	while (regs->STATUS & STAT_SPIACTIVE) {
	}

	/* Command phase enable */
	tctrl = TCTRL_CMD_EN_MSK;
	if (access & ANDES_ACCESS_ADDRESSED) {
		/* Enable and set ADDR len */
		regs->ADDR = addr;
		/* Address phase enable */
		tctrl |= TCTRL_ADDR_EN_MSK;
	}

	if (length == 0) {
		tctrl |= TRNS_MODE_NONE_DATA;
	} else if (is_write) {
		tctrl |= TRNS_MODE_WRITE_ONLY;
#ifdef CONFIG_FLASH_ANDES_QSPI_XIP_COUNT_REGS
		regs->WRCNT = length - 1;
#else  /* CONFIG_FLASH_ANDES_QSPI_XIP_COUNT_REGS */
		tctrl |= (length - 1) << TCTRL_WR_TCNT_OFFSET;
#endif /* CONFIG_FLASH_ANDES_QSPI_XIP_COUNT_REGS */
	} else {
		tctrl |= TRNS_MODE_READ_ONLY;
#ifdef CONFIG_FLASH_ANDES_QSPI_XIP_COUNT_REGS
		regs->RDCNT = length - 1;
#else  /* CONFIG_FLASH_ANDES_QSPI_XIP_COUNT_REGS */
		tctrl |= (length - 1) << TCTRL_RD_TCNT_OFFSET;
#endif /* CONFIG_FLASH_ANDES_QSPI_XIP_COUNT_REGS */
	}

	if (opcode == SPI_NOR_CMD_PP_1_1_4) {
		tctrl |= DUAL_IO_MODE;
	}

	tfmat = regs->TFMAT;
	/* Clear data and address length fields + disable data merge. */
	tfmat &= ~TFMAT_DATA_LEN_MSK & ~TFMAT_ADDR_LEN_MSK & ~TFMAT_DATA_MERGE_MSK;
	/* Set data len to 7+1 and address to 3 bytes. */
	tfmat |= (7 << TFMAT_DATA_LEN_OFFSET) | (0x2 << TFMAT_ADDR_LEN_OFFSET);
	regs->TFMAT = tfmat;
	regs->TCTRL = tctrl;
	/* write CMD register to send command*/
	regs->CMD = opcode;

	if (length > 0) {
		handle_data_transfer(is_write, (uint8_t *)data, length, regs);
	}

	/* Wait till SPI transfer is finished. */
	while (regs->STATUS & STAT_SPIACTIVE) {
	}

	return 0;
}

/**
 * @brief Wait until the flash is ready
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static __ramfunc int flash_andes_qspi_xip_wait_until_ready(const struct device *dev)
{
	int ret;
	uint8_t reg;

	do {
		ret = flash_andes_qspi_xip_cmd_read(dev, FLASH_ANDES_CMD_RDSR, &reg, 1);
	} while (!ret && (reg & FLASH_ANDES_WIP_BIT));

	return ret;
}

/**
 * @brief Set write protection
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static __ramfunc int write_protection_set(const struct device *dev, bool write_protect)
{
	return flash_andes_qspi_xip_cmd_write(dev, (write_protect) ? FLASH_ANDES_CMD_WRDI
								   : FLASH_ANDES_CMD_WREN);
}

static unsigned int prepare_for_ramfunc(void)
{
	unsigned int key;

	key = irq_lock();
	csr_clear(NDS_MMISC_CTL, MMISC_CTL_BRPE_EN);

	return key;
}

static void cleanup_after_ramfunc(unsigned int key)
{
	csr_set(NDS_MMISC_CTL, MMISC_CTL_BRPE_EN);
	irq_unlock(key);
}

static __ramfunc void prepare_for_flashing(const struct device *dev)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;
	struct atcspi200_regs *regs = config->regs;
	uint32_t memctrl;

	/* Make sure a previous SPI transfer is finished. */
	while (regs->STATUS & STAT_SPIACTIVE) {
	}

	/* Exit memory-mapped interface before SPI transfer. */
	memctrl = regs->MEMCTRL;
	regs->MEMCTRL = memctrl;
	while (regs->MEMCTRL & MEMCTRL_CHG) {
	}
}

static __ramfunc void cleanup_after_flashing(const struct device *dev, off_t addr, size_t size)
{
#ifdef CONFIG_CACHE_MANAGEMENT
	if (size > 0) {
		const struct flash_andes_qspi_xip_config *config = dev->config;

		/* Invalidate modyfied flash memory. */
		cache_data_invd_range((void *)(addr + config->mapped_base), size);
		cache_instr_invd_range((void *)(addr + config->mapped_base), size);
	}
#endif /* CONFIG_CACHE_MANAGEMENT */
}

static int flash_andes_qspi_xip_read(const struct device *dev, off_t addr, void *dest, size_t size)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;

	if (size == 0) {
		return 0;
	}

	/* Check read boundaries */
	if ((addr < 0 || addr >= config->flash_size) || ((config->flash_size - addr) < size)) {
		return -EINVAL;
	}

	/* Use memory-mapped mechanism for reading. */
	memcpy(dest, (void *)(config->mapped_base + addr), size);

	return 0;
}

static __ramfunc int do_write(const struct device *dev, off_t addr, const void *src, size_t size)
{
	int ret = 0;
	size_t to_write;
	off_t addr_curr = addr;
	size_t size_remainig = size;

	prepare_for_flashing(dev);

	while (size_remainig > 0) {
		ret = write_protection_set(dev, false);
		if (ret != 0) {
			break;
		}

		/* Get the adequate size to send*/
		to_write = MIN(PAGE_SIZE - (addr_curr % PAGE_SIZE), size_remainig);
		ret = flash_andes_qspi_xip_cmd_addr_write(dev, SPI_NOR_CMD_PP_1_1_4, addr_curr, src,
							  to_write);
		flash_andes_qspi_xip_wait_until_ready(dev);

		if (ret != 0) {
			break;
		}

		src = (const uint8_t *)src + to_write;
		size_remainig -= to_write;
		addr_curr += to_write;
	}

	/* Make sure write protection is enabled at the end of write. */
	int ret2 = write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

	cleanup_after_flashing(dev, addr, size);

	return ret;
}

static int flash_andes_qspi_xip_write(const struct device *dev, off_t addr, const void *src,
				      size_t size)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;
	int ret = 0;
	unsigned int key;

	if (size == 0) {
		return 0;
	}

	/* Check write boundaries */
	if ((addr < 0 || addr >= config->flash_size) || ((config->flash_size - addr) < size)) {
		return -EINVAL;
	}

	/*
	 * Synchronous mechanisms like semaphores are not needed,
	 * because interrupts are always locked and there are no reschedule points.
	 */
	key = prepare_for_ramfunc();

	ret = do_write(dev, addr, src, size);

	cleanup_after_ramfunc(key);

	return ret;
}

static __ramfunc int do_erase(const struct device *dev, off_t addr, size_t size)
{
	int ret = 0;
	off_t addr_curr = addr;
	size_t size_remainig = size;

	prepare_for_flashing(dev);

	while (size_remainig > 0) {
		ret = write_protection_set(dev, false);
		if (ret != 0) {
			break;
		}

		/* Use the smallest erase unit not to hold CPU for long. */
		ret = flash_andes_qspi_xip_cmd_addr_write(dev, SPI_NOR_CMD_SE, addr_curr, NULL, 0);
		flash_andes_qspi_xip_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}
		addr_curr += SPI_NOR_SECTOR_SIZE;
		size_remainig -= SPI_NOR_SECTOR_SIZE;
	}

	/* Make sure write protection is enabled at the end of erase. */
	int ret2 = write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

	cleanup_after_flashing(dev, addr, size);

	return ret;
}

static int flash_andes_qspi_xip_erase(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;
	int ret = 0;
	unsigned int key;

	if (size == 0) {
		return 0;
	}

	/* Check erase boundaries */
	if ((addr < 0 || addr >= config->flash_size) || ((config->flash_size - addr) < size)) {
		return -EINVAL;
	}

	/* address must be sector-aligned */
	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		return -EINVAL;
	}

	/* size must be a multiple of sectors */
	if ((size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	/*
	 * Synchronous mechanisms like semaphores are not needed,
	 * because interrupts are always locked and there are no reschedule points.
	 */
	key = prepare_for_ramfunc();

	do_erase(dev, addr, size);

	cleanup_after_ramfunc(key);

	return ret;
}

static int flash_andes_qspi_xip_init(const struct device *dev)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;

	if (!config->is_xip) {
		return -EINVAL;
	}

	return 0;
}

static const struct flash_parameters *flash_andes_qspi_xip_get_parameters(const struct device *dev)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;

	return &config->parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_andes_qspi_xip_pages_layout(const struct device *dev,
					      const struct flash_pages_layout **layout,
					      size_t *layout_size)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static __ramfunc int flash_andes_qspi_xip_get_status(const struct device *dev,
						     struct andes_xip_ex_ops_get_out *op_out)
{
	int ret = 0;

	prepare_for_flashing(dev);

	ret = flash_andes_qspi_xip_cmd_read(dev, SPI_NOR_CMD_RDSR, &op_out->regs[0], 1);
	if (ret) {
		goto cleanup;
	}

	ret = flash_andes_qspi_xip_cmd_read(dev, SPI_NOR_CMD_RDSR2, &op_out->regs[1], 1);
	if (ret) {
		goto cleanup;
	}

	ret = flash_andes_qspi_xip_cmd_read(dev, SPI_NOR_CMD_RDSR3, &op_out->regs[2], 1);

cleanup:
	cleanup_after_flashing(dev, 0, 0);

	return ret;
}

static __ramfunc int write_status_register(const struct device *dev, uint8_t sr, uint8_t mask,
					   uint8_t op_read, uint8_t op_write)
{
	uint8_t sr_curr;
	uint8_t sr_new;
	int ret, ret2 = 0;

	if (!mask) {
		return 0;
	}

	ret = flash_andes_qspi_xip_cmd_read(dev, op_read, &sr_curr, 1);
	if (ret) {
		return ret;
	}
	sr_new = (sr_curr & ~mask) | sr;
	if (sr_new != sr_curr) {
		ret = write_protection_set(dev, false);
		if (ret != 0) {
			return ret;
		}
		ret = flash_andes_qspi_xip_cmd_write_data(dev, op_write, &sr_new, 1);
		ret2 = flash_andes_qspi_xip_wait_until_ready(dev);
	}

	if (!ret) {
		ret = ret2;
	}

	return ret;
}

static __ramfunc int flash_andes_qspi_xip_set_status(const struct device *dev,
						     struct andes_xip_ex_ops_set_in *op_out)
{
	int ret;
	struct flash_andes_qspi_xip_data *data = dev->data;

	if (data->status_lock) {
		return -EPERM;
	}

	prepare_for_flashing(dev);

	ret = write_status_register(dev, op_out->regs[0], op_out->masks[0], SPI_NOR_CMD_RDSR,
				    SPI_NOR_CMD_WRSR);
	if (ret) {
		goto cleanup;
	}

	ret = write_status_register(dev, op_out->regs[1], op_out->masks[1], SPI_NOR_CMD_RDSR2,
				    SPI_NOR_CMD_WRSR2);
	if (ret) {
		goto cleanup;
	}

	ret = write_status_register(dev, op_out->regs[2], op_out->masks[2], SPI_NOR_CMD_RDSR3,
				    SPI_NOR_CMD_WRSR3);

cleanup:
	cleanup_after_flashing(dev, 0, 0);

	return ret;
}

static __ramfunc int flash_andes_qspi_xip_lock(const struct device *dev,
					       struct andes_xip_ex_ops_lock_in *op_in)
{
	struct flash_andes_qspi_xip_data *data = dev->data;

	data->status_lock = op_in->enable;

	return 0;
}

static __ramfunc int flash_andes_qspi_xip_lock_state(const struct device *dev,
						     struct andes_xip_ex_ops_lock_state_out *op_out)
{
	struct flash_andes_qspi_xip_data *data = dev->data;

	op_out->state = data->status_lock;

	return 0;
}

static __ramfunc int
flash_andes_qspi_xip_set_memrdcmd(const struct device *dev,
				  struct andes_xip_ex_ops_mem_read_cmd_in *op_in)
{
	const struct flash_andes_qspi_xip_config *config = dev->config;
	struct atcspi200_regs *regs = config->regs;

	prepare_for_flashing(dev);

	regs->MEMCTRL = (uint32_t)op_in->cmd;
	while (regs->MEMCTRL & MEMCTRL_CHG) {
	}

	cleanup_after_flashing(dev, 0, 0);

	return 0;
}

static int flash_andes_qspi_xip_ex_op(const struct device *dev, uint16_t code, const uintptr_t in,
				      void *out)
{
#ifdef CONFIG_USERSPACE
	bool syscall_trap = z_syscall_trap();
#endif
	int key, ret;

	switch (code) {
	case FLASH_ANDES_XIP_EX_OP_GET_STATUS_REGS: {
		struct andes_xip_ex_ops_get_out *op_out = (struct andes_xip_ex_ops_get_out *)out;
#ifdef CONFIG_USERSPACE
		struct andes_xip_ex_ops_get_out copy_out;

		if (syscall_trap) {
			op_out = &copy_out;
		}
#endif
		key = prepare_for_ramfunc();
		ret = flash_andes_qspi_xip_get_status(dev, op_out);
		cleanup_after_ramfunc(key);
#ifdef CONFIG_USERSPACE
		if (ret == 0 && syscall_trap) {
			K_OOPS(k_usermode_to_copy(out, op_out, sizeof(copy_out)));
		}
#endif
		break;
	}
	case FLASH_ANDES_XIP_EX_OP_SET_STATUS_REGS: {
		struct andes_xip_ex_ops_set_in *op_in = (struct andes_xip_ex_ops_set_in *)in;
#ifdef CONFIG_USERSPACE
		struct andes_xip_ex_ops_set_in copy_in;

		if (syscall_trap) {
			K_OOPS(k_usermode_from_copy(&copy_in, op_in, sizeof(copy_in)));
			op_in = &copy_in;
		}
#endif
		key = prepare_for_ramfunc();
		ret = flash_andes_qspi_xip_set_status(dev, op_in);
		cleanup_after_ramfunc(key);
		break;
	}
	case FLASH_ANDES_XIP_EX_OP_LOCK: {
		struct andes_xip_ex_ops_lock_in *op_in = (struct andes_xip_ex_ops_lock_in *)in;
#ifdef CONFIG_USERSPACE
		struct andes_xip_ex_ops_lock_in copy_in;

		if (syscall_trap) {
			K_OOPS(k_usermode_from_copy(&copy_in, op_in, sizeof(copy_in)));
			op_in = &copy_in;
		}
#endif
		ret = flash_andes_qspi_xip_lock(dev, op_in);
		break;
	}
	case FLASH_ANDES_XIP_EX_OP_LOCK_STATE: {
		struct andes_xip_ex_ops_lock_state_out *op_out =
			(struct andes_xip_ex_ops_lock_state_out *)out;
#ifdef CONFIG_USERSPACE
		struct andes_xip_ex_ops_lock_state_out copy_out;

		if (syscall_trap) {
			op_out = &copy_out;
		}
#endif
		key = prepare_for_ramfunc();
		ret = flash_andes_qspi_xip_lock_state(dev, op_out);
		cleanup_after_ramfunc(key);
#ifdef CONFIG_USERSPACE
		if (ret == 0 && syscall_trap) {
			K_OOPS(k_usermode_to_copy(out, op_out, sizeof(copy_out)));
		}
#endif
		break;
	}
	case FLASH_ANDES_XIP_EX_OP_MEM_READ_CMD: {
		struct andes_xip_ex_ops_mem_read_cmd_in *op_in =
			(struct andes_xip_ex_ops_mem_read_cmd_in *)in;
#ifdef CONFIG_USERSPACE
		struct andes_xip_ex_ops_mem_read_cmd_in copy_in;

		if (syscall_trap) {
			K_OOPS(k_usermode_from_copy(&copy_in, op_in, sizeof(copy_in)));
			op_in = &copy_in;
		}
#endif
		key = prepare_for_ramfunc();
		ret = flash_andes_qspi_xip_set_memrdcmd(dev, op_in);
		cleanup_after_ramfunc(key);
		break;
	}
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

static DEVICE_API(flash, flash_andes_qspi_xip_api) = {
	.read = flash_andes_qspi_xip_read,
	.write = flash_andes_qspi_xip_write,
	.erase = flash_andes_qspi_xip_erase,
	.get_parameters = flash_andes_qspi_xip_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_andes_qspi_xip_pages_layout,
#endif
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_andes_qspi_xip_ex_op,
#endif
};

#define IS_XIP(node_id) DT_SAME_NODE(node_id, DT_CHOSEN(zephyr_flash))

#define LAYOUT_PAGES_PROP(n)                                               \
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,                               \
		(.layout = {                                               \
			.pages_count = (DT_INST_PROP(n, size) /            \
				CONFIG_FLASH_ANDES_QSPI_LAYOUT_PAGE_SIZE), \
			.pages_size =                                      \
				CONFIG_FLASH_ANDES_QSPI_LAYOUT_PAGE_SIZE,  \
		},                                                         \
		))

#define FLASH_ANDES_QSPI_XIP_INIT(n)                                                               \
	static const struct flash_andes_qspi_xip_config flash_andes_qspi_xip_config_##n = {        \
		.regs = (struct atcspi200_regs *)DT_REG_ADDR(DT_INST_BUS(n)),                      \
		.mapped_base = DT_REG_ADDR_BY_IDX(DT_INST_BUS(n), 1),                              \
		.parameters = {.write_block_size = 1, .erase_value = 0xff},                        \
		.is_xip = IS_XIP(DT_DRV_INST(n)),                                                  \
		.flash_size = DT_INST_PROP(n, size),                                               \
		LAYOUT_PAGES_PROP(n)};                                                             \
                                                                                                   \
	static struct flash_andes_qspi_xip_data flash_andes_qspi_xip_data_##n;                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &flash_andes_qspi_xip_init, NULL, &flash_andes_qspi_xip_data_##n, \
			      &flash_andes_qspi_xip_config_##n, POST_KERNEL,                       \
			      CONFIG_FLASH_ANDES_QSPI_INIT_PRIORITY, &flash_andes_qspi_xip_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_ANDES_QSPI_XIP_INIT)
