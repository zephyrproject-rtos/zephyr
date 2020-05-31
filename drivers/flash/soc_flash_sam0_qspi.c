/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 * Copyright (c) 2020 Google LLC.
 *
 * This driver is heavily inspired by the spi_nor.c driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_qspi_nor

#include <errno.h>
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <init.h>
#include <string.h>
#include <logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(flash_sam0_qspi, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_SAM0_QSPI_MAX_ID_LEN 3

/* Status register bits */
#define FLASH_SAM0_QSPI_WIP_BIT BIT(0) /* Write in progress */
#define FLASH_SAM0_QSPI_WEL_BIT BIT(1) /* Write enable latch */

/* Flash opcodes */
#define FLASH_SAM0_QSPI_CMD_WRSR 0x01 /* Write status register */
#define FLASH_SAM0_QSPI_CMD_RDSR 0x05 /* Read status register */
#define FLASH_SAM0_QSPI_CMD_READ 0x03 /* Read data */
#define FLASH_SAM0_QSPI_CMD_FASTREAD 0x0B /* Fast read */
#define FLASH_SAM0_QSPI_CMD_QREAD 0x6B /* Quad read */
#define FLASH_SAM0_QSPI_CMD_WREN 0x06 /* Write enable */
#define FLASH_SAM0_QSPI_CMD_WRDI 0x04 /* Write disable */
#define FLASH_SAM0_QSPI_CMD_PP 0x02 /* Page program */
#define FLASH_SAM0_QSPI_CMD_4PP 0x32 /* Quad program */
#define FLASH_SAM0_QSPI_CMD_SE 0x20 /* Sector erase */
#define FLASH_SAM0_QSPI_CMD_BE_32K 0x52 /* Block erase 32KB */
#define FLASH_SAM0_QSPI_CMD_BE 0xD8 /* Block erase */
#define FLASH_SAM0_QSPI_CMD_CE 0xC7 /* Chip erase */
#define FLASH_SAM0_QSPI_CMD_RDID 0x9F /* Read JEDEC ID */
#define FLASH_SAM0_QSPI_CMD_ULBPR 0x98 /* Global Block Protection Unlock */
#define FLASH_SAM0_QSPI_CMD_DPD 0xB9 /* Deep Power Down */
#define FLASH_SAM0_QSPI_CMD_RDPD 0xAB /* Release from Deep Power Down */

/* Page, sector, and block size are standard, not configurable. */
#define FLASH_SAM0_QSPI_PAGE_SIZE 0x0100U
#define FLASH_SAM0_QSPI_SECTOR_SIZE 0x1000U
#define FLASH_SAM0_QSPI_BLOCK_SIZE 0x10000U

/* Some devices support erase operations on 32 KiB blocks.
 * Support is indicated by the has-be32k property.
 */
#define FLASH_SAM0_QSPI_BLOCK32_SIZE 0x8000

/* Test whether offset is aligned. */
#define FLASH_SAM0_QSPI_IS_PAGE_ALIGNED(_ofs)                                  \
	(((_ofs) & (FLASH_SAM0_QSPI_PAGE_SIZE - 1U)) == 0)
#define FLASH_SAM0_QSPI_IS_SECTOR_ALIGNED(_ofs)                                \
	(((_ofs) & (FLASH_SAM0_QSPI_SECTOR_SIZE - 1U)) == 0)
#define FLASH_SAM0_QSPI_IS_BLOCK_ALIGNED(_ofs)                                 \
	(((_ofs) & (FLASH_SAM0_QSPI_BLOCK_SIZE - 1U)) == 0)
#define FLASH_SAM0_QSPI_IS_BLOCK32_ALIGNED(_ofs)                               \
	(((_ofs) & (FLASH_SAM0_QSPI_BLOCK32_SIZE - 1U)) == 0)

struct flash_sam0_qspi_config {
	Qspi *regs;

	u8_t id[FLASH_SAM0_QSPI_MAX_ID_LEN];
	bool has_be32k;
	/* Size from devicetree, in bytes */
	u32_t size;
};

struct flash_sam0_qspi_data {
	struct k_sem sem;
};

/* Everything necessary to acquire owning access to the device. */
static void flash_sam0_qspi_acquire_device(struct device *dev)
{
	struct flash_sam0_qspi_data *const driver_data = dev->driver_data;
	k_sem_take(&driver_data->sem, K_FOREVER);
}

/* Everything necessary to release access to the device. */
static void flash_sam0_qspi_release_device(struct device *dev)
{
	struct flash_sam0_qspi_data *const driver_data = dev->driver_data;
	k_sem_give(&driver_data->sem);
}

static void flash_sam0_qspi_clear_cache(void)
{
	CMCC->CTRL.bit.CEN = 0;
	while (CMCC->SR.bit.CSTS) {
	}
	CMCC->CFG.bit.DCDIS = 1;
	CMCC->CTRL.bit.CEN = 1;
}

static void flash_sam0_qspi_enable_cache(void)
{
	CMCC->CTRL.bit.CEN = 0;
	while (CMCC->SR.bit.CSTS) {
	}
	CMCC->CFG.bit.DCDIS = 0;
	CMCC->MAINT0.bit.INVALL = 1;
	CMCC->CTRL.bit.CEN = 1;
}

static int flash_sam0_qspi_access(struct device *dev, uint8_t command,
				  uint32_t iframe, uint32_t addr,
				  uint8_t *buffer, size_t size)
{
	const struct flash_sam0_qspi_config *cfg = dev->config_info;
	Qspi *regs = cfg->regs;

	if (command == FLASH_SAM0_QSPI_CMD_SE ||
	    command == FLASH_SAM0_QSPI_CMD_BE ||
	    command == FLASH_SAM0_QSPI_CMD_BE_32K) {
		regs->INSTRADDR.reg = addr;
	}

	regs->INSTRCTRL.bit.INSTR = command;
	regs->INSTRFRAME.reg = iframe;

	// Dummy read of INSTRFRAME needed to synchronize.
	// See Instruction Transmission Flow Diagram, figure 37.9, page 995
	// and Example 4, page 998, section 37.6.8.5.
	(volatile uint32_t) regs->INSTRFRAME.reg;
	regs->INTFLAG.reg = regs->INTFLAG.reg;

	if (buffer && size) {
		uint8_t *qspi_mem = (uint8_t *)(QSPI_AHB + addr);
		uint32_t const tfr_type = iframe & QSPI_INSTRFRAME_TFRTYPE_Msk;

		if ((tfr_type == QSPI_INSTRFRAME_TFRTYPE_READ) ||
		    (tfr_type == QSPI_INSTRFRAME_TFRTYPE_READMEMORY)) {
			memcpy(buffer, qspi_mem, size);
		} else {
                        memcpy(qspi_mem, buffer, size);
		}
	}

	regs->CTRLA.reg = QSPI_CTRLA_ENABLE | QSPI_CTRLA_LASTXFER;

	while (!regs->INTFLAG.bit.INSTREND) {
	}
	// TODO	regs->INTFLAG.bit.INSTREND = 1;
	regs->INTFLAG.reg = regs->INTFLAG.reg;

	return 0;
}

static int flash_sam0_qspi_cmd_read(struct device *dev, u8_t opcode, u8_t *dest,
				    size_t length)
{
	uint32_t iframe = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
			  QSPI_INSTRFRAME_ADDRLEN_24BITS |
			  QSPI_INSTRFRAME_TFRTYPE_READ |
			  QSPI_INSTRFRAME_INSTREN | QSPI_INSTRFRAME_DATAEN;
	int ret;

	flash_sam0_qspi_clear_cache();
	ret = flash_sam0_qspi_access(dev, opcode, iframe, 0, dest, length);
	flash_sam0_qspi_enable_cache();

	return ret;
}

static int flash_sam0_qspi_cmd_write(struct device *dev, u8_t opcode)
{
	uint32_t iframe = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
			  QSPI_INSTRFRAME_ADDRLEN_24BITS |
			  QSPI_INSTRFRAME_TFRTYPE_WRITE |
			  QSPI_INSTRFRAME_INSTREN;
	int ret;

	flash_sam0_qspi_clear_cache();
	ret = flash_sam0_qspi_access(dev, opcode, iframe, 0, NULL, 0);
	flash_sam0_qspi_enable_cache();

	return ret;
}

static int flash_sam0_qspi_cmd_addr_read(struct device *dev, u8_t opcode,
					 off_t addr, u8_t *dest, size_t length)
{
	uint32_t iframe = QSPI_INSTRFRAME_WIDTH_QUAD_OUTPUT |
			  QSPI_INSTRFRAME_ADDRLEN_24BITS |
			  QSPI_INSTRFRAME_TFRTYPE_READMEMORY |
			  QSPI_INSTRFRAME_INSTREN | QSPI_INSTRFRAME_ADDREN |
			  QSPI_INSTRFRAME_DATAEN | QSPI_INSTRFRAME_DUMMYLEN(8);
	int ret;

	flash_sam0_qspi_clear_cache();
	ret = flash_sam0_qspi_access(dev, opcode, iframe, addr, dest, length);
	flash_sam0_qspi_enable_cache();

	return ret;
}

static int flash_sam0_qspi_cmd_addr_write(struct device *dev, u8_t opcode,
					  off_t addr, const void *src,
					  size_t length)
{
	uint32_t iframe = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
			  QSPI_INSTRFRAME_ADDRLEN_24BITS |
			  QSPI_INSTRFRAME_TFRTYPE_WRITEMEMORY |
			  QSPI_INSTRFRAME_INSTREN | QSPI_INSTRFRAME_ADDREN |
			  QSPI_INSTRFRAME_DATAEN;

	int ret;

	flash_sam0_qspi_clear_cache();
	ret = flash_sam0_qspi_access(dev, opcode, iframe, addr, src, length);
	flash_sam0_qspi_enable_cache();

	return ret;
}

static int flash_sam0_qspi_cmd_addr_erase(struct device *dev, u8_t opcode,
					  off_t addr)
{
	uint32_t iframe = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
			  QSPI_INSTRFRAME_ADDRLEN_24BITS |
			  QSPI_INSTRFRAME_TFRTYPE_WRITE |
			  QSPI_INSTRFRAME_INSTREN | QSPI_INSTRFRAME_ADDREN;

	return flash_sam0_qspi_access(dev, opcode, iframe, addr, NULL, 0);
}

/**
 * @brief Retrieve the Flash JEDEC ID and compare it with the one expected
 *
 * @param dev The device structure
 * @param flash_id The flash info structure which contains the expected JEDEC ID
 * @return 0 on success, negative errno code otherwise
 */
static inline int
flash_sam0_qspi_read_id(struct device *dev,
			const struct flash_sam0_qspi_config *const flash_id)
{
	u8_t buf[FLASH_SAM0_QSPI_MAX_ID_LEN];

	if (flash_sam0_qspi_cmd_read(dev, FLASH_SAM0_QSPI_CMD_RDID, buf,
				     FLASH_SAM0_QSPI_MAX_ID_LEN) != 0) {
		return -EIO;
	}

	if (memcmp(flash_id->id, buf, FLASH_SAM0_QSPI_MAX_ID_LEN) != 0) {
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Wait until the flash is ready
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static int flash_sam0_qspi_wait_until_ready(struct device *dev)
{
	int ret;
	u8_t reg = 0;

	do {
		ret = flash_sam0_qspi_cmd_read(dev, FLASH_SAM0_QSPI_CMD_RDSR,
					       &reg, 1);
	} while (!ret && (reg & FLASH_SAM0_QSPI_WIP_BIT));

	return ret;
}

static int flash_sam0_qspi_read(struct device *dev, off_t addr, void *dest,
				size_t size)
{
	const struct flash_sam0_qspi_config *params = dev->config_info;
	int ret;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((addr + size) > params->size)) {
		return -EINVAL;
	}

	flash_sam0_qspi_acquire_device(dev);

	flash_sam0_qspi_wait_until_ready(dev);

	ret = flash_sam0_qspi_cmd_addr_read(dev, FLASH_SAM0_QSPI_CMD_QREAD,
					    addr, dest, size);

	flash_sam0_qspi_release_device(dev);
	return ret;
}

static int flash_sam0_qspi_write(struct device *dev, off_t addr,
				 const void *src, size_t size)
{
	const struct flash_sam0_qspi_config *params = dev->config_info;
	int ret = 0;

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > params->size)) {
		return -EINVAL;
	}

	flash_sam0_qspi_acquire_device(dev);

	while (size > 0) {
		size_t to_write = size;

		/* Don't write more than a page. */
		if (to_write >= FLASH_SAM0_QSPI_PAGE_SIZE) {
			to_write = FLASH_SAM0_QSPI_PAGE_SIZE;
		}

		/* Don't write across a page boundary */
		if (((addr + to_write - 1U) / FLASH_SAM0_QSPI_PAGE_SIZE) !=
		    (addr / FLASH_SAM0_QSPI_PAGE_SIZE)) {
			to_write = FLASH_SAM0_QSPI_PAGE_SIZE -
				   (addr % FLASH_SAM0_QSPI_PAGE_SIZE);
		}

		flash_sam0_qspi_cmd_write(dev, FLASH_SAM0_QSPI_CMD_WREN);
		ret = flash_sam0_qspi_cmd_addr_write(
			dev, FLASH_SAM0_QSPI_CMD_PP, addr, src, to_write);
		if (ret != 0) {
			goto out;
		}

		size -= to_write;
		src = (const u8_t *)src + to_write;
		addr += to_write;

		flash_sam0_qspi_wait_until_ready(dev);
	}

out:
	flash_sam0_qspi_release_device(dev);
	return ret;
}

static int flash_sam0_qspi_erase(struct device *dev, off_t addr, size_t size)
{
	const struct flash_sam0_qspi_config *params = dev->config_info;
	int ret = 0;

	/* Should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > params->size)) {
		return -ENODEV;
	}

	flash_sam0_qspi_acquire_device(dev);

	while (size) {
		/* Write enable */
		flash_sam0_qspi_cmd_write(dev, FLASH_SAM0_QSPI_CMD_WREN);

		if (size == params->size) {
			/* chip erase */
			flash_sam0_qspi_cmd_write(dev, FLASH_SAM0_QSPI_CMD_CE);
			size -= params->size;
		} else if ((size >= FLASH_SAM0_QSPI_BLOCK_SIZE) &&
			   FLASH_SAM0_QSPI_IS_BLOCK_ALIGNED(addr)) {
			/* 64 KiB block erase */
			flash_sam0_qspi_cmd_addr_erase(
				dev, FLASH_SAM0_QSPI_CMD_BE, addr);
			addr += FLASH_SAM0_QSPI_BLOCK_SIZE;
			size -= FLASH_SAM0_QSPI_BLOCK_SIZE;
		} else if ((size >= FLASH_SAM0_QSPI_BLOCK32_SIZE) &&
			   FLASH_SAM0_QSPI_IS_BLOCK32_ALIGNED(addr) &&
			   params->has_be32k) {
			/* 32 KiB block erase */
			flash_sam0_qspi_cmd_addr_erase(
				dev, FLASH_SAM0_QSPI_CMD_BE_32K, addr);
			addr += FLASH_SAM0_QSPI_BLOCK32_SIZE;
			size -= FLASH_SAM0_QSPI_BLOCK32_SIZE;
		} else if ((size >= FLASH_SAM0_QSPI_SECTOR_SIZE) &&
			   FLASH_SAM0_QSPI_IS_SECTOR_ALIGNED(addr)) {
			/* sector erase */
			flash_sam0_qspi_cmd_addr_erase(
				dev, FLASH_SAM0_QSPI_CMD_SE, addr);
			addr += FLASH_SAM0_QSPI_SECTOR_SIZE;
			size -= FLASH_SAM0_QSPI_SECTOR_SIZE;
		} else {
			/* minimal erase size is at least a sector size */
			LOG_DBG("unsupported at 0x%lx size %zu", (long)addr,
				size);
			ret = -EINVAL;
			goto out;
		}

		ret = flash_sam0_qspi_wait_until_ready(dev);
		if (ret != 0) {
			goto out;
		}
	}

out:
	flash_sam0_qspi_release_device(dev);

	return ret;
}

static int flash_sam0_qspi_write_protection_set(struct device *dev,
						bool write_protect)
{
	int ret;

	flash_sam0_qspi_acquire_device(dev);
	flash_sam0_qspi_wait_until_ready(dev);

	ret = flash_sam0_qspi_cmd_write(dev, (write_protect) ?
						     FLASH_SAM0_QSPI_CMD_WRDI :
						     FLASH_SAM0_QSPI_CMD_WREN);

	if (IS_ENABLED(DT_INST_PROP(0, requires_ulbpr)) && (ret == 0) &&
	    !write_protect) {
		ret = flash_sam0_qspi_cmd_write(dev, FLASH_SAM0_QSPI_CMD_ULBPR);
	}

	flash_sam0_qspi_release_device(dev);

	return ret;
}

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int flash_sam0_qspi_configure(struct device *dev)
{
	struct flash_sam0_qspi_data *data = dev->driver_data;
	const struct flash_sam0_qspi_config *cfg = dev->config_info;
	Qspi *regs = cfg->regs;
	int frequency = DT_INST_PROP(0, spi_max_frequency);
	int div;

	/* Initialise the QSPI peripheral */
	MCLK->APBCMASK.bit.QSPI_ = 1;
	MCLK->AHBMASK.bit.QSPI_ = 1;
	MCLK->AHBMASK.bit.QSPI_2X_ = 0;

	regs->CTRLA.bit.SWRST = 1;

	regs->CTRLB.reg = QSPI_CTRLB_MODE_MEMORY | QSPI_CTRLB_CSMODE_NORELOAD |
			  QSPI_CTRLB_DATALEN_8BITS | QSPI_CTRLB_CSMODE_LASTXFER;

	/* Read the ID at a slower speed */
	regs->BAUD.bit.BAUD = SOC_ATMEL_SAM0_MCK_FREQ_HZ / 4000000;

	regs->CTRLA.bit.ENABLE = 1;

	if (flash_sam0_qspi_read_id(dev, cfg) != 0) {
		return -ENODEV;
	}

	/* Now that we know the right chip is there, switch to high speed */
	regs->CTRLA.bit.ENABLE = 0;

	/* Pick the frequency or the next fastest. */
	div = ((SOC_ATMEL_SAM0_MCK_FREQ_HZ + frequency - 1) / frequency) - 1;
	div = MAX(0, MIN(UINT8_MAX, div));
	regs->BAUD.bit.BAUD = div;
	regs->CTRLA.bit.ENABLE = 1;

	return 0;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int flash_sam0_qspi_init(struct device *dev)
{
	struct flash_sam0_qspi_data *const driver_data = dev->driver_data;
	k_sem_init(&driver_data->sem, 1, UINT_MAX);

	return flash_sam0_qspi_configure(dev);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)

/* instance 0 size in bytes */
#define INST_0_BYTES (DT_INST_PROP(0, size) / 8)

BUILD_ASSERT(FLASH_SAM0_QSPI_IS_SECTOR_ALIGNED(
		     CONFIG_FLASH_SAM0_QSPI_FLASH_LAYOUT_PAGE_SIZE),
	     "FLASH_SAM0_QSPI_FLASH_LAYOUT_PAGE_SIZE must be multiple of 4096");

/* instance 0 page count */
#define LAYOUT_PAGES_COUNT                                                     \
	(INST_0_BYTES / CONFIG_FLASH_SAM0_QSPI_FLASH_LAYOUT_PAGE_SIZE)

BUILD_ASSERT(
	(CONFIG_FLASH_SAM0_QSPI_FLASH_LAYOUT_PAGE_SIZE * LAYOUT_PAGES_COUNT) ==
		INST_0_BYTES,
	"FLASH_SAM0_QSPI_FLASH_LAYOUT_PAGE_SIZE incompatible with flash size");

static const struct flash_pages_layout dev_layout = {
	.pages_count = LAYOUT_PAGES_COUNT,
	.pages_size = CONFIG_FLASH_SAM0_QSPI_FLASH_LAYOUT_PAGE_SIZE,
};
#undef LAYOUT_PAGES_COUNT

static void
flash_sam0_qspi_pages_layout(struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_sam0_qspi_api = {
	.read = flash_sam0_qspi_read,
	.write = flash_sam0_qspi_write,
	.erase = flash_sam0_qspi_erase,
	.write_protection = flash_sam0_qspi_write_protection_set,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_sam0_qspi_pages_layout,
#endif
	.write_block_size = 1,
};

static const struct flash_sam0_qspi_config flash_sam0_qspi_config_0 = {
	.regs = (Qspi *)DT_REG_ADDR_BY_IDX(DT_BUS(DT_DRV_INST(0)), 0),
	.id = DT_INST_PROP(0, jedec_id),
#if DT_INST_NODE_HAS_PROP(0, has_be32k)
	.has_be32k = true,
#endif /* DT_INST_NODE_HAS_PROP(0, has_be32k) */
	.size = DT_INST_PROP(0, size) / 8,
};

static struct flash_sam0_qspi_data flash_sam0_qspi_data_0;

DEVICE_AND_API_INIT(flaso_sam0_qspi, DT_INST_LABEL(0), &flash_sam0_qspi_init,
		    &flash_sam0_qspi_data_0, &flash_sam0_qspi_config_0,
		    POST_KERNEL, CONFIG_FLASH_SAM0_QSPI_INIT_PRIORITY,
		    &flash_sam0_qspi_api);
