/*
 * Copyright (c) 2026 Eve Redero
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_flash_qspi

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/flash.h>
#include <soc.h>
#include "spi_nor.h"

#include <zephyr/logging/log.h>

#define SAM_IFR_WIDTH_1_1_1 QSPI_IFR_WIDTH_SINGLE_BIT_SPI_Val
#define SAM_IFR_WIDTH_1_1_2 QSPI_IFR_WIDTH_DUAL_OUTPUT_Val
#define SAM_IFR_WIDTH_1_1_4 QSPI_IFR_WIDTH_QUAD_OUTPUT_Val
#define SAM_IFR_WIDTH_1_2_2 QSPI_IFR_WIDTH_DUAL_IO_Val
#define SAM_IFR_WIDTH_1_4_4 QSPI_IFR_WIDTH_QUAD_IO_Val
#define SAM_IFR_WIDTH_2_2_2 QSPI_IFR_WIDTH_DUAL_CMD_Val
#define SAM_IFR_WIDTH_4_4_4 QSPI_IFR_WIDTH_QUAD_CMD_Val

#define SPI_NOR_CMD_PP_1_1_1 SPI_NOR_CMD_PP

#define SPI_NOR_CMD_READ_1_1_1 SPI_NOR_CMD_READ
#define SPI_NOR_CMD_READ_1_1_2 SPI_NOR_CMD_DREAD
#define SPI_NOR_CMD_READ_1_2_2 SPI_NOR_CMD_2READ
#define SPI_NOR_CMD_READ_1_1_4 SPI_NOR_CMD_QREAD
#define SPI_NOR_CMD_READ_1_4_4 SPI_NOR_CMD_4READ

/* This is JEDEC standard, but Micron flashes are different */
/* Volatile write is required in Cypress flash to access SR3 */
#define SPI_NOR_CMD_VOLATILE_WR_VCFGREG 0x50

#define SPI_NOR_STATUS_QUAD_ENABLE  1
#define SPI_NOR_STATUS_QUAD_DISABLE 0
#define SPI_NOR_STATUS_QUAD_OFFSET  1
#define SPI_NOR_STATUS_QUAD_MASK    0x06
#define SPI_NOR_STATUS_DUMMY_OFFSET 0
#define SPI_NOR_STATUS_DUMMY_MASK   0x0f

LOG_MODULE_REGISTER(flash_qspi_atmel_sam, CONFIG_FLASH_LOG_LEVEL);

#define ANY_INST_IS_CYPRESS DT_ANY_INST_HAS_BOOL_STATUS_OKAY(compat_cypress)

#if defined(ANY_INST_IS_CYPRESS)
#define SPI_NOR_CMD_RDSR3_CYPRESS 0x33
#endif

struct sam_flash_erase_data {
	off_t section_start;
	size_t section_end;
	bool succeeded;
};

struct sam_flash_data {
	const struct device *dev;
	struct sam_flash_erase_data erase_data;
	struct k_sem sem;
};

struct sam_flash_config {
	Qspi *regs;
	const bool compat_cypress;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	const uint32_t frequency;
	const uint32_t size;
	const uint8_t double_dr;
	const uint8_t ifr_width_read;
	const uint8_t ifr_width_write;
	const uint8_t nb_dummy;
	const uint8_t qspi_read_cmd;
	const uint8_t qspi_write_cmd;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	const struct flash_pages_layout pages_layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

struct qspi_cmd {
	uint8_t op_code;
	uint32_t addr;
	uint16_t add_len;
	uint8_t transfer_type;
	bool data_en;
	bool fast_transfer;
};

static void acquire_device(const struct device *dev)
{
	struct sam_flash_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static void release_device(const struct device *dev)
{
	struct sam_flash_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

static inline void qspi_wait_end_transmission(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;

	while (!(regs->QSPI_SR & QSPI_SR_INSTRE_Msk)) {
		k_usleep(100);
	}
}

static inline void qspi_flash_sam_end_transfer(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;

	while (!(regs->QSPI_SR & QSPI_SR_TXEMPTY)) {
		k_usleep(100);
	}
	/* Send LASTXFER to end transmission */
	regs->QSPI_CR = QSPI_CR_LASTXFER_Msk;
	qspi_wait_end_transmission(dev);
}

static inline void qspi_flash_sam_dummy_sync(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;
	uint32_t reg;
	/* Dummy read IFR for synchronisation */
	reg = regs->QSPI_IFR;
}

static int qspi_flash_sam_generic_command(const struct device *dev, struct qspi_cmd cmd)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;
	uint32_t spi_ifr = 0U;

	/* Verify that the peripheral is enabled */
	if (!(regs->QSPI_SR & QSPI_SR_QSPIENS)) {
		return -EIO;
	}
	/* If the instruction has an address, write in register */
	if (cmd.add_len == 24 || cmd.add_len == 32) {
		regs->QSPI_IAR = cmd.addr;
		LOG_DBG("IAR (address): 0x%08x", cmd.addr);
		spi_ifr |= QSPI_IFR_ADDREN_Msk;
	} else if (cmd.add_len > 0) {
		LOG_ERR("Invalid address length %d", cmd.add_len);
		return -EINVAL;
	}
	if (cmd.add_len == 32) {
		spi_ifr |= QSPI_IFR_ADDRL_Msk;
	}
	/* If the instruction has no address, we leave zeros */

	/* Second, set up instruction register with the command code */
	regs->QSPI_ICR = QSPI_ICR_INST(cmd.op_code);
	LOG_DBG("ICR (command): 0x%08x", cmd.op_code);
	/* Then, set the instruction frame register with transfer parameters*/
	spi_ifr |= QSPI_IFR_INSTEN_Msk;
	spi_ifr |= QSPI_IFR_TFRTYP(cmd.transfer_type);
	if (cmd.data_en) {
		spi_ifr |= (1 << QSPI_IFR_DATAEN_Pos);
	}
	if (cmd.fast_transfer) {
		/* Fast transfer sets dual/quad mode and dummy
		 * bits, as selected in device tree
		 */
		spi_ifr |= QSPI_IFR_WIDTH(config->ifr_width_write);
	} else {
		spi_ifr |= QSPI_IFR_WIDTH(QSPI_IFR_WIDTH_SINGLE_BIT_SPI_Val);
	}
	/* Dummy bits are only added to fast reads, reads 1-1-2 and 1-1-4 */
	if (cmd.op_code == SPI_NOR_CMD_QREAD || cmd.op_code == SPI_NOR_CMD_DREAD ||
	    cmd.op_code == SPI_NOR_CMD_READ_FAST) {
		spi_ifr |= QSPI_IFR_NBDUM(config->nb_dummy);
		spi_ifr |= QSPI_IFR_WIDTH(config->ifr_width_read);
	} else if (cmd.op_code == SPI_NOR_CMD_4READ) {
		spi_ifr |= QSPI_IFR_WIDTH(config->ifr_width_read);
		spi_ifr |= QSPI_IFR_NBDUM(10);
	} else if (cmd.op_code == SPI_NOR_CMD_PP_1_4_4) {
		spi_ifr |= QSPI_IFR_WIDTH(config->ifr_width_read);
		spi_ifr |= QSPI_IFR_NBDUM(2);
	}
	LOG_DBG("IFR (frame): 0x%08x", spi_ifr);
	regs->QSPI_IFR = spi_ifr;

	/* If it is a simple transmission, wait for the flag to raise */
	if (cmd.transfer_type == QSPI_IFR_TFRTYP_TRSFR_READ_Val) {
		qspi_wait_end_transmission(dev);
	}
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void qspi_flash_sam_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	const struct sam_flash_config *config = dev->config;

	*layout = &config->pages_layout;
	*layout_size = 1;
}
#endif

static inline void qspi_disable(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;

	regs->QSPI_CR = QSPI_CR_QSPIDIS;
	while (regs->QSPI_SR & QSPI_SR_QSPIENS) {
		k_usleep(100);
	}
}

static inline void qspi_enable(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;

	regs->QSPI_CR = QSPI_CR_QSPIEN;
	while (!(regs->QSPI_SR & QSPI_SR_QSPIENS)) {
		k_usleep(100);
	}
}

static inline void qspi_reset(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;

	regs->QSPI_CR = QSPI_CR_SWRST;
}

static int qspi_flash_sam_info(const struct device *dev, uint8_t op_code, uint8_t *data,
			       uint8_t offset)
{
	int ret = 0;
	/* Send the command to set the memory in read mode */
	struct qspi_cmd cmd = {
		.op_code = op_code,
		.addr = 0,
		.add_len = 0,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_READ_MEMORY_Val,
		.data_en = true,
		.fast_transfer = false,
	};

	acquire_device(dev);

	ret = qspi_flash_sam_generic_command(dev, cmd);

	qspi_flash_sam_dummy_sync(dev);

	/* Read memory map address */
	uintptr_t mmap_addr = QSPIMEM_ADDR + offset;

	memcpy(data, (uint8_t *)mmap_addr, 1);

	qspi_flash_sam_end_transfer(dev);
	release_device(dev);

	return ret;
}

static int qspi_flash_sam_register_write(const struct device *dev, uint8_t op_code, uint8_t *data,
					 uint8_t size)
{
	int ret = 0;

	/* Send the command to set the memory in read mode */
	struct qspi_cmd cmd = {
		.op_code = op_code,
		.addr = 0,
		.add_len = 0,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_WRITE_Val,
		.data_en = true,
		.fast_transfer = false,
	};

	acquire_device(dev);
	ret = qspi_flash_sam_generic_command(dev, cmd);

	qspi_flash_sam_dummy_sync(dev);

	/* Write memory map address */
	uintptr_t mmap_addr = QSPIMEM_ADDR;

	memcpy((uint8_t *)mmap_addr, data, size);
	LOG_HEXDUMP_DBG(data, size, "data written");

	qspi_flash_sam_end_transfer(dev);
	release_device(dev);

	return ret;
}

static inline void qspi_write_protection_disable(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	Qspi *regs = config->regs;
	uint32_t spi_wpmr = 0U;

	spi_wpmr |= QSPI_WPMR_WPKEY(0x515350);
	/* Writing 0 in QSPI_WPMR_WPEN bit to disable protection */
	regs->QSPI_WPMR = spi_wpmr;
}

/*** SPI Flash specific functions ***/
static inline void qspi_wait_end_access(const struct device *dev)
{
	uint8_t ret = 0xFF;

	while (ret & SPI_NOR_MEM_RDY_MASK) {
		qspi_flash_sam_info(dev, SPI_NOR_CMD_RDSR, &ret, 0);
		LOG_DBG("NOR RDSR (status): 0x%02x", ret);
	}
}

/* Enable SPI NOR write protection */
static inline void qspi_enable_write(const struct device *dev)
{
	uint8_t ret = 0x00;
	/* Send the command to set the memory in write mode */
	struct qspi_cmd cmd = {
		.op_code = SPI_NOR_CMD_WREN,
		.addr = 0,
		.add_len = 0,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_READ_Val,
		.data_en = false,
		.fast_transfer = false,
	};

	qspi_flash_sam_generic_command(dev, cmd);

	while (!(ret & SPI_NOR_WEL_MASK)) {
		qspi_flash_sam_info(dev, SPI_NOR_CMD_RDSR, &ret, 0);
		LOG_DBG("NOR RDSR: 0x%02x", ret);
	}
}

/* Disable SPI NOR write protection */
static inline void qspi_disable_write(const struct device *dev)
{
	uint8_t ret = 0xFF;
	/* Send the command to set the memory in read mode */
	struct qspi_cmd cmd = {
		.op_code = SPI_NOR_CMD_WRDI,
		.addr = 0,
		.add_len = 0,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_READ_Val,
		.data_en = false,
		.fast_transfer = false,
	};

	qspi_flash_sam_generic_command(dev, cmd);

	while (ret & SPI_NOR_WEL_MASK) {
		qspi_flash_sam_info(dev, SPI_NOR_CMD_RDSR, &ret, 0);
		LOG_DBG("NOR RDSR: 0x%02x", ret);
	}
}

/* Enable SPI NOR volatile write protection ( ) */
static inline void qspi_enable_volatile_write(const struct device *dev)
{
	/* Send the command to set the memory in write mode */
	struct qspi_cmd cmd = {
		.op_code = SPI_NOR_CMD_VOLATILE_WR_VCFGREG,
		.addr = 0,
		.add_len = 0,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_READ_Val,
		.data_en = false,
		.fast_transfer = false,
	};

	qspi_flash_sam_generic_command(dev, cmd);
}

static inline void qspi_write_status_register(const struct device *dev, uint8_t value,
					      uint8_t stat_register_read,
					      uint8_t stat_register_write, uint8_t offset,
					      uint8_t mask)
{
	uint8_t status;
	uint8_t ret = 0x00;

	qspi_flash_sam_info(dev, stat_register_read, &status, 0);

	ret = (value << offset) & mask;
	LOG_DBG("Value: %d, New val: 0x%02x", value, ret);
	status = (status & ~mask) | ret;
	LOG_DBG("New status: 0x%02x", status);
	LOG_HEXDUMP_DBG(&status, 1, "data to write");
	qspi_enable_write(dev);
	qspi_flash_sam_register_write(dev, stat_register_write, &status, 0);
	qspi_disable_write(dev);
	/* The flash write status register operation has a write time */
	k_msleep(100);
}

#if defined(ANY_INST_IS_CYPRESS)
static inline void qspi_write_status_register_cypress(const struct device *dev, uint8_t value,
						      uint8_t num_register, uint8_t offset,
						      uint8_t mask)
{
	/* Cypress / Infineon SPI flash variants do not have Write Register
	 * commands for Register 2 and 3.
	 */
	/* Value is the value you want to write. It comes with its mask in the
	 * byte (0x10 for 1-bit value offset by 8, 0x3 for 2-bit value offset by 0),
	 * the target register index (0
	 * for SR-1) and the offset in said register.
	 */

	uint8_t status[3];
	uint8_t ret = 0x00;

	qspi_flash_sam_info(dev, SPI_NOR_CMD_RDSR, status, 0);
	qspi_flash_sam_info(dev, SPI_NOR_CMD_RDSR2, (status + 1), 0);
	qspi_flash_sam_info(dev, SPI_NOR_CMD_RDSR3_CYPRESS, (status + 2), 0);

	LOG_HEXDUMP_DBG(status, 3, "init reg data");
	ret = (value << offset) & mask;
	LOG_DBG("Value: %d, New val: 0x%02x", value, ret);
	status[num_register] = (status[num_register] & ~mask) | ret;
	LOG_DBG("New status: 0x%02x", status[num_register]);
	LOG_HEXDUMP_DBG(status, 3, "data to write");
	if (num_register == 2) {
		qspi_enable_volatile_write(dev);
	} else {
		qspi_enable_write(dev);
	}
	qspi_flash_sam_register_write(dev, SPI_NOR_CMD_WRSR, status, 3);
	qspi_disable_write(dev);
	/* The flash write status register operation has a write time */
	k_msleep(100);
}
#endif /* ANY_INST_IS_CYPRESS */

/* Enable SPI NOR quad mode */
static inline void qspi_enable_quad_mode(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;

	if (config->compat_cypress) {
		qspi_write_status_register_cypress(dev, SPI_NOR_STATUS_QUAD_ENABLE, 1,
						   SPI_NOR_STATUS_QUAD_OFFSET,
						   SPI_NOR_STATUS_QUAD_MASK);
	} else {
		qspi_write_status_register(dev, SPI_NOR_STATUS_QUAD_ENABLE, SPI_NOR_CMD_RDSR2,
					   SPI_NOR_CMD_WRSR2, SPI_NOR_STATUS_QUAD_OFFSET,
					   SPI_NOR_STATUS_QUAD_MASK);
	}
}

/* Disable SPI NOR quad mode */
static inline void qspi_disable_quad_mode(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;

	if (config->compat_cypress) {
		qspi_write_status_register_cypress(dev, SPI_NOR_STATUS_QUAD_DISABLE, 1,
						   SPI_NOR_STATUS_QUAD_OFFSET,
						   SPI_NOR_STATUS_QUAD_MASK);
	} else {
		qspi_write_status_register(dev, SPI_NOR_STATUS_QUAD_DISABLE, SPI_NOR_CMD_RDSR2,
					   SPI_NOR_CMD_WRSR2, SPI_NOR_STATUS_QUAD_OFFSET,
					   SPI_NOR_STATUS_QUAD_MASK);
	}
}

/* Set SPI NOR dummy bit */
static inline void qspi_set_dummy_bits(const struct device *dev, uint8_t nb_dummy)
{
	const struct sam_flash_config *config = dev->config;

	if (config->compat_cypress) {
		qspi_write_status_register_cypress(dev, nb_dummy, 2, SPI_NOR_STATUS_DUMMY_OFFSET,
						   SPI_NOR_STATUS_DUMMY_MASK);
	} else {
		qspi_write_status_register(dev, nb_dummy, SPI_NOR_CMD_RDSR3, SPI_NOR_CMD_WRSR3,
					   SPI_NOR_STATUS_DUMMY_OFFSET, SPI_NOR_STATUS_DUMMY_MASK);
	}
}

/*** API functions ***/
#if defined(CONFIG_FLASH_JESD216_API)
static int qspi_flash_sam_sfdp_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	int ret = 0;

	struct qspi_cmd cmd = {
		.op_code = 0x5a,
		.addr = 1,
		.add_len = 0,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_READ_MEMORY_Val,
		.data_en = true,
		.fast_transfer = false,
	};

	LOG_DBG("Entering read info");
	acquire_device(dev);

	/* Send the command to set the memory in read mode */
	ret = qspi_flash_sam_generic_command(dev, cmd);

	qspi_flash_sam_dummy_sync(dev);

	/* Read memory map address */
	uintptr_t mmap_addr = QSPIMEM_ADDR + offset;

	memcpy(data, mmap_addr, len);
	LOG_DBG("data: 0x%08x", *data);

	qspi_flash_sam_end_transfer(dev);
	release_device(dev);

	return ret;
}

static int qspi_flash_sam_read_jedec_id(const struct device *dev, uint8_t *id)
{
	int ret = 0;

	if (id == NULL) {
		return -EINVAL;
	}
	qspi_flash_sam_info(dev, SPI_NOR_CMD_RDID, id, 0);
	qspi_flash_sam_info(dev, SPI_NOR_CMD_RDID, id + 1, 1);
	qspi_flash_sam_info(dev, SPI_NOR_CMD_RDID, id + 2, 2);

	return ret;
}

#endif

static int qspi_flash_sam_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct sam_flash_config *config = dev->config;
	const size_t num_sectors = size / SPI_NOR_SECTOR_SIZE;
	const size_t num_blocks = size / SPI_NOR_BLOCK_SIZE;
	int ret = 0;
	int i;
	struct qspi_cmd cmd = {
		.op_code = SPI_NOR_CMD_CE,
		.addr = 0,
		.add_len = 0,
		/* TRSFR READ allows to read and write to the serial memory */
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_READ_Val,
		.data_en = false,
		.fast_transfer = false,
	};

	if (!size) {
		return 0;
	}

	if (offset % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid offset");
		return -EINVAL;
	}

	if (size % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	if ((offset == 0) && (size == config->size)) {
		LOG_DBG("Full flash erase");
		qspi_enable_write(dev);
		cmd.op_code = SPI_NOR_CMD_CE;
		acquire_device(dev);
		ret = qspi_flash_sam_generic_command(dev, cmd);
		release_device(dev);

	} else if ((0 == (offset % SPI_NOR_BLOCK_SIZE)) && (0 == (size % SPI_NOR_BLOCK_SIZE))) {
		cmd.op_code = SPI_NOR_CMD_BE;
		cmd.add_len = 24;
		for (i = 0; i < num_blocks; i++) {
			LOG_DBG("Erasing block %d", i);
			qspi_enable_write(dev);
			cmd.addr = offset + i * SPI_NOR_BLOCK_SIZE;
			acquire_device(dev);
			ret = qspi_flash_sam_generic_command(dev, cmd);
			release_device(dev);
		}
	} else if ((0 == (offset % SPI_NOR_SECTOR_SIZE)) && (0 == (size % SPI_NOR_SECTOR_SIZE))) {
		cmd.op_code = SPI_NOR_CMD_SE;
		cmd.add_len = 24;
		for (i = 0; i < num_sectors; i++) {
			LOG_DBG("Erasing sector %d", i);
			qspi_enable_write(dev);
			cmd.addr = offset + i * SPI_NOR_SECTOR_SIZE;
			acquire_device(dev);
			ret = qspi_flash_sam_generic_command(dev, cmd);
			release_device(dev);
		}
	} else {
		return -EINVAL;
	}

	qspi_wait_end_access(dev);
	qspi_disable_write(dev);

	return ret;
}

static int qspi_flash_sam_read(const struct device *dev, off_t offset, void *data, size_t size)
{
	int ret = 0;
	const struct sam_flash_config *config = dev->config;

	if (!data) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	if (offset < 0 || (offset + size) > config->size) {
		LOG_ERR("Read address or size exceeds expected values. "
			"Addr: 0x%lx size %zu",
			(long)offset, size);
		return -EINVAL;
	}

	acquire_device(dev);

	/* Send the command to set the memory in read mode */
	struct qspi_cmd cmd = {
		.op_code = config->qspi_read_cmd,
		.addr = offset,
		.add_len = 24,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_READ_MEMORY_Val,
		.data_en = true,
		.fast_transfer = true,
	};

	ret = qspi_flash_sam_generic_command(dev, cmd);

	qspi_flash_sam_dummy_sync(dev);

	/* Read memory map address */
	uintptr_t mmap_addr = QSPIMEM_ADDR + offset;

	LOG_DBG("Memory-mapped read from 0x%08lx, len %zu", mmap_addr, size);
	memcpy(data, (void *)mmap_addr, size);

	qspi_flash_sam_end_transfer(dev);
	release_device(dev);

	return ret;
}

static int qspi_flash_sam_write(const struct device *dev, off_t offset, const void *data,
				size_t size)
{
	int ret = 0;
	size_t to_write = size;
	const struct sam_flash_config *config = dev->config;

	if (!data) {
		return -EINVAL;
	}

	if (!size) {
		return 0;
	}

	if ((size % 4U) != 0) {
		return -EINVAL;
	}
	if ((offset % 4U) != 0) {
		return -EINVAL;
	}

	if (offset < 0 || (offset + size) > config->size) {
		LOG_ERR("Write address or size exceeds expected values. "
			"Addr: 0x%lx size %zu",
			(long)offset, size);
		return -EINVAL;
	}

	/* Send the command to set the memory in write mode */
	struct qspi_cmd cmd = {
		.op_code = config->qspi_write_cmd,
		.addr = offset,
		.add_len = 24,
		.transfer_type = QSPI_IFR_TFRTYP_TRSFR_WRITE_MEMORY_Val,
		.data_en = true,
		.fast_transfer = true,
	};

	while (size > 0) {
		to_write = size;
		/* Don't write more than a page. */
		if (to_write >= SPI_NOR_PAGE_SIZE) {
			to_write = SPI_NOR_PAGE_SIZE;
		}

		/* Don't write across a page boundary */
		if (((offset + to_write - 1U) / SPI_NOR_PAGE_SIZE) !=
		    (offset / SPI_NOR_PAGE_SIZE)) {
			to_write = SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE);
		}

		qspi_enable_write(dev);
		acquire_device(dev);
		cmd.addr = offset;
		ret = qspi_flash_sam_generic_command(dev, cmd);

		qspi_flash_sam_dummy_sync(dev);

		/* Memory map address */
		uintptr_t mmap_addr = QSPIMEM_ADDR + offset;

		LOG_DBG("Memory-mapped write to 0x%08lx, len %zu", mmap_addr, to_write);
		memcpy((void *)mmap_addr, data, to_write);

		qspi_flash_sam_end_transfer(dev);
		release_device(dev);
		qspi_wait_end_access(dev);

		/* Next loop */
		size -= to_write;
		data = (const uint8_t *)data + to_write;
		offset += to_write;
	}

	qspi_disable_write(dev);
	return ret;
}

static const struct flash_parameters qspi_flash_sam_parameters = {.write_block_size = 1,
								  .erase_value = 0xff};

static const struct flash_parameters *qspi_flash_sam_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &qspi_flash_sam_parameters;
}

static DEVICE_API(flash, sam_flash_api) = {
	.erase = qspi_flash_sam_erase,
	.write = qspi_flash_sam_write,
	.read = qspi_flash_sam_read,
	.get_parameters = qspi_flash_sam_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = qspi_flash_sam_page_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = qspi_flash_sam_sfdp_read,
	.read_jedec_id = qspi_flash_sam_read_jedec_id,
#endif
};

static int sam_flash_init(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;
	struct sam_flash_data *qspi_data = dev->data;
	int ret;
	int clock_div;
	Qspi *regs = config->regs;
	uint32_t spi_mr = 0U, spi_csr = 0U;

	/* Enable SPI clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&config->clock_cfg);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to configure pins for QSPI");
		return -EIO;
	}
	qspi_data->dev = dev;
	k_sem_init(&qspi_data->sem, 1, 1);
	acquire_device(dev);
	/* Disable */
	qspi_disable(dev);
	qspi_reset(dev);
	/* Set QSPI memory mode */
	spi_mr |= QSPI_MR_SMM_MEMORY;
	/* Disable local loopback */
	spi_mr &= (~QSPI_MR_LLB);
	/* Disable wait data read before transfer */
	spi_mr &= (~QSPI_MR_WDRBT);
	LOG_DBG("MR: 0x%08x", spi_mr);

	regs->QSPI_MR = spi_mr;

	/* Use the requested or next highest possible frequency */
	/* Assuming peripheral does not use divided clock */
	clock_div = SOC_ATMEL_SAM_MCK_FREQ_HZ / config->frequency;
	clock_div = CLAMP(clock_div, 1, UINT8_MAX);
	/* We let CPOL and CPHA to 0*/
	spi_csr |= QSPI_SCR_SCBR(clock_div);
	LOG_DBG("CSR: 0x%08x", spi_csr);

	regs->QSPI_SCR = spi_csr;

	/* Disable write protection */
	regs->QSPI_WPMR &= (~QSPI_WPMR_WPEN_Msk);
	/* Enable */
	qspi_enable(dev);

	release_device(dev);

	qspi_set_dummy_bits(dev, config->nb_dummy);
	if (config->ifr_width_read == SAM_IFR_WIDTH_1_4_4 ||
	    config->ifr_width_read == SAM_IFR_WIDTH_1_1_4) {
		LOG_DBG("Configuring flash in quad mode");
		qspi_enable_quad_mode(dev);
	} else {
		LOG_DBG("Configuring flash in non-quad mode");
		qspi_disable_quad_mode(dev);
	}

	LOG_DBG("Sam flash QSPI init done");

	return 0;
}

#define DT_WRITEOC_PROP_OR(inst, default_value)                                                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, writeoc),                                    \
		    (_CONCAT(SPI_NOR_CMD_PP_, DT_STRING_TOKEN(DT_DRV_INST(inst), writeoc))),    \
		    ((default_value)))

#define DT_REG_READOC_PROP_OR(inst, default_value)                                                 \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, readoc),                                    \
		    (_CONCAT(SAM_IFR_WIDTH_, DT_STRING_TOKEN(DT_DRV_INST(inst), readoc))),    \
		    ((default_value)))

#define DT_REG_WRITEOC_PROP_OR(inst, default_value)                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, writeoc),                                    \
		    (_CONCAT(SAM_IFR_WIDTH_, DT_STRING_TOKEN(DT_DRV_INST(inst), writeoc))),    \
		    ((default_value)))

#define DT_READOC_PROP_OR(inst, default_value)                                                     \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, readoc),                                    \
		    (_CONCAT(SPI_NOR_CMD_READ_, DT_STRING_TOKEN(DT_DRV_INST(inst), readoc))),    \
		    ((default_value)))

#define QSPI_NODE DT_INST_PARENT(0)
PINCTRL_DT_DEFINE(QSPI_NODE);

#define SAM_QSPI_INIT(inst)                                                                        \
	static struct sam_flash_data flash_data_##inst;                                            \
	static const struct sam_flash_config flash_config_##inst = {                               \
		.regs = (Qspi *)DT_REG_ADDR(QSPI_NODE),                                            \
		.clock_cfg = SAM_DT_CLOCK_PMC_CFG(inst, QSPI_NODE),                                \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(QSPI_NODE),                                      \
		.qspi_write_cmd = DT_WRITEOC_PROP_OR(inst, SPI_NOR_CMD_PP_1_1_1),                  \
		.ifr_width_read = DT_REG_READOC_PROP_OR(inst, SAM_IFR_WIDTH_1_1_1),                \
		.ifr_width_write = DT_REG_WRITEOC_PROP_OR(inst, SAM_IFR_WIDTH_1_1_1),              \
		.qspi_read_cmd = DT_READOC_PROP_OR(inst, SPI_NOR_CMD_READ_1_1_1),                  \
		.nb_dummy = DT_INST_PROP_OR(inst, nb_dummy, 0),                                    \
		.size = DT_INST_PROP(inst, size),                                                  \
		.frequency = DT_INST_PROP(inst, qspi_max_frequency),                               \
		.compat_cypress = DT_INST_PROP(inst, compat_cypress),                              \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (                          \
			.pages_layout = {                                            \
			.pages_count = DT_INST_PROP(inst, size)                 \
					/ SPI_NOR_SECTOR_SIZE,                   \
			.pages_size = SPI_NOR_SECTOR_SIZE,                      \
		},                                                           \
		)) };         \
	DEVICE_DT_INST_DEFINE(inst, sam_flash_init, NULL, &flash_data_##inst,                      \
			      &flash_config_##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,       \
			      &sam_flash_api);

DT_INST_FOREACH_STATUS_OKAY(SAM_QSPI_INIT)
