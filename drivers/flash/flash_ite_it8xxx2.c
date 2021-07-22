/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <device.h>
#include <drivers/flash.h>
#include <init.h>
#include <kernel.h>
#include <linker/linker-defs.h>
#include <soc.h>
#include <string.h>

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(flash_ite_it8xxx2);

/* RAM code start address */
extern char _ram_code_start;
#define FLASH_RAMCODE_START ((uint32_t)&_ram_code_start)
#define	FLASH_RAMCODE_START_BIT19      BIT(19)
/* RAM code section */
#define __ram_code __attribute__((section(".__ram_code")))

#define FLASH_IT8XXX2_REG_BASE \
		((struct flash_it8xxx2_regs *)DT_INST_REG_ADDR(0))

#define DEV_DATA(dev) \
	((struct flash_it8xxx2_dev_data *const)(dev)->data)

struct flash_it8xxx2_dev_data {
	struct k_sem sem;
	int all_protected;
	int flash_static_cache_enabled;
};

/*
 * One page program instruction allows maximum 256 bytes (a page) of data
 * to be programmed.
 */
#define CHIP_FLASH_WRITE_PAGE_MAX_SIZE 256
/* Program is run directly from storage */
#define CHIP_MAPPED_STORAGE_BASE       DT_REG_ADDR(DT_NODELABEL(flash0))
/* flash size */
#define CHIP_FLASH_SIZE_BYTES          DT_REG_SIZE(DT_NODELABEL(flash0))
/* protect bank size */
#define CHIP_FLASH_BANK_SIZE           0x00001000
/* base+0000h~base+0FFF */
#define CHIP_RAMCODE_BASE              0x80100000

/*
 * This is the block size of the ILM on the it8xxx2 chip.
 * The ILM for static code cache, CPU fetch instruction from
 * ILM(ILM -> CPU)instead of flash(flash ->  I-Cache -> CPU) if enabled.
 */
#define IT8XXX2_ILM_BLOCK_SIZE 0x00001000

/* page program command  */
#define FLASH_CMD_PAGE_WRITE   0x2
/* ector erase command (erase size is 4KB) */
#define FLASH_CMD_SECTOR_ERASE 0x20
/* command for flash write */
#define FLASH_CMD_WRITE        FLASH_CMD_PAGE_WRITE
/* Write status register */
#define FLASH_CMD_WRSR         0x01
/* Write disable */
#define FLASH_CMD_WRDI         0x04
/* Write enable */
#define FLASH_CMD_WREN         0x06
/* Read status register */
#define FLASH_CMD_RS           0x05

/* Set FSCE# as high level by writing 0 to address xfff_fe00h */
#define FLASH_FSCE_HIGH_ADDRESS        0x0FFFFE00
/* Set FSCE# as low level by writing data to address xfff_fd00h */
#define FLASH_FSCE_LOW_ADDRESS         0x0FFFFD00

#define FWP_REG(bank) (bank / 8)
#define FWP_MASK(bank) (1 << (bank % 8))

enum flash_wp_interface {
	FLASH_WP_HOST = 0x01,
	FLASH_WP_DBGR = 0x02,
	FLASH_WP_EC = 0x04,
};

enum flash_status_mask {
	FLASH_SR_NO_BUSY = 0,
	/* Internal write operation is in progress */
	FLASH_SR_BUSY = 0x01,
	/* Device is memory Write enabled */
	FLASH_SR_WEL = 0x02,

	FLASH_SR_ALL = (FLASH_SR_BUSY | FLASH_SR_WEL),
};

enum flash_transaction_cmd {
	CMD_CONTINUE,
	CMD_END,
};

static const struct flash_parameters flash_it8xxx2_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

void __ram_code ramcode_reset_i_cache(void)
{
	/* I-Cache tag sram reset */
	IT83XX_GCTRL_MCCR |= IT83XX_GCTRL_ICACHE_RESET;
	/* Make sure the I-Cache is reset */
	__asm__ volatile ("fence.i" ::: "memory");

	IT83XX_GCTRL_MCCR &= ~IT83XX_GCTRL_ICACHE_RESET;
	__asm__ volatile ("fence.i" ::: "memory");
}

void __ram_code ramcode_flash_follow_mode(void)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;
	/*
	 * ECINDAR3-0 are EC-indirect memory address registers.
	 *
	 * Enter follow mode by writing 0xf to low nibble of ECINDAR3 register,
	 * and set high nibble as 0x4 to select internal flash.
	 */
	flash_regs->SMFI_ECINDAR3 = (EC_INDIRECT_READ_INTERNAL_FLASH |
		((FLASH_FSCE_HIGH_ADDRESS >> 24) & GENMASK(3, 0)));

	/* Set FSCE# as high level by writing 0 to address xfff_fe00h */
	flash_regs->SMFI_ECINDAR2 = (FLASH_FSCE_HIGH_ADDRESS >> 16) & GENMASK(7, 0);
	flash_regs->SMFI_ECINDAR1 = (FLASH_FSCE_HIGH_ADDRESS >> 8) & GENMASK(7, 0);
	flash_regs->SMFI_ECINDAR0 = FLASH_FSCE_HIGH_ADDRESS & GENMASK(7, 0);

	/* Writing 0 to EC-indirect memory data register */
	flash_regs->SMFI_ECINDDR = 0x00;
}

void __ram_code ramcode_flash_follow_mode_exit(void)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;

	/* Exit follow mode, and keep the setting of selecting internal flash */
	flash_regs->SMFI_ECINDAR3 = EC_INDIRECT_READ_INTERNAL_FLASH;
	flash_regs->SMFI_ECINDAR2 = 0x00;
}

void __ram_code ramcode_flash_fsce_high(void)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;

	/* FSCE# high level */
	flash_regs->SMFI_ECINDAR1 = (FLASH_FSCE_HIGH_ADDRESS >> 8) & GENMASK(7, 0);

	/* Writing 0 to EC-indirect memory data register */
	flash_regs->SMFI_ECINDDR = 0x00;
}

void __ram_code ramcode_flash_write_dat(uint8_t wdata)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;

	/* Write data to FMOSI */
	flash_regs->SMFI_ECINDDR = wdata;
}

void __ram_code ramcode_flash_transaction(int wlen, uint8_t *wbuf, int rlen,
					  uint8_t *rbuf,
					  enum flash_transaction_cmd cmd_end)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;
	int i;

	/*  FSCE# with low level */
	flash_regs->SMFI_ECINDAR1 = (FLASH_FSCE_LOW_ADDRESS >> 8) & GENMASK(7, 0);
	/* Write data to FMOSI */
	for (i = 0; i < wlen; i++) {
		flash_regs->SMFI_ECINDDR = wbuf[i];
	}
	/* Read data from FMISO */
	for (i = 0; i < rlen; i++) {
		rbuf[i] = flash_regs->SMFI_ECINDDR;
	}
	/* FSCE# high level if transaction done */
	if (cmd_end == CMD_END) {
		ramcode_flash_fsce_high();
	}
}

void __ram_code ramcode_flash_cmd_read_status(enum flash_status_mask mask,
					      enum flash_status_mask target)
{
	uint8_t status[1];
	uint8_t cmd_rs[] = {FLASH_CMD_RS};

	/*
	 * We prefer no timeout here. We can always get the status
	 * we want, or wait for watchdog triggered to check
	 * e-flash's status instead of breaking loop.
	 * This will avoid fetching unknown instruction from e-flash
	 * and causing exception.
	 */
	while (1) {
		/* read status */
		ramcode_flash_transaction(sizeof(cmd_rs), cmd_rs, 1, status, CMD_END);
		/* only bit[1:0] valid */
		if ((status[0] & mask) == target) {
			break;
		}
	}
}

void __ram_code ramcode_flash_cmd_write_enable(void)
{
	uint8_t cmd_we[] = {FLASH_CMD_WREN};

	/* enter EC-indirect follow mode */
	ramcode_flash_follow_mode();
	/* send write enable command */
	ramcode_flash_transaction(sizeof(cmd_we), cmd_we, 0, NULL, CMD_END);
	/* read status and make sure busy bit cleared and write enabled. */
	ramcode_flash_cmd_read_status(FLASH_SR_ALL, FLASH_SR_WEL);
	/* exit EC-indirect follow mode */
	ramcode_flash_follow_mode_exit();
}

void __ram_code ramcode_flash_cmd_write_disable(void)
{
	uint8_t cmd_wd[] = {FLASH_CMD_WRDI};

	/* enter EC-indirect follow mode */
	ramcode_flash_follow_mode();
	/* send write disable command */
	ramcode_flash_transaction(sizeof(cmd_wd), cmd_wd, 0, NULL, CMD_END);
	/* make sure busy bit cleared. */
	ramcode_flash_cmd_read_status(FLASH_SR_ALL, FLASH_SR_NO_BUSY);
	/* exit EC-indirect follow mode */
	ramcode_flash_follow_mode_exit();
}

int __ram_code ramcode_flash_verify(int addr, int size, const char *data)
{
	int i;
	uint8_t *wbuf = (uint8_t *)data;
	uint8_t *flash = (uint8_t *)addr;

	/* verify for erase */
	if (data == NULL) {
		for (i = 0; i < size; i++) {
			if (flash[i] != 0xFF) {
				return -EINVAL;
			}
		}
	/* verify for write */
	} else {
		for (i = 0; i < size; i++) {
			if (flash[i] != wbuf[i]) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

void __ram_code ramcode_flash_cmd_write(int addr, int wlen, uint8_t *wbuf)
{
	int i;
	uint8_t flash_write[] = {FLASH_CMD_WRITE, ((addr >> 16) & 0xFF),
		((addr >> 8) & 0xFF), (addr & 0xFF)};

	/* enter EC-indirect follow mode */
	ramcode_flash_follow_mode();
	/* send flash write command (aai word or page program) */
	ramcode_flash_transaction(sizeof(flash_write), flash_write, 0, NULL, CMD_CONTINUE);

	for (i = 0; i < wlen; i++) {
		/* send data byte */
		ramcode_flash_write_dat(wbuf[i]);

		/*
		 * we want to restart the write sequence every IDEAL_SIZE
		 * chunk worth of data.
		 */
		if (!(++addr % CHIP_FLASH_WRITE_PAGE_MAX_SIZE)) {
			uint8_t w_en[] = {FLASH_CMD_WREN};

			ramcode_flash_fsce_high();
			/* make sure busy bit cleared. */
			ramcode_flash_cmd_read_status(FLASH_SR_BUSY, FLASH_SR_NO_BUSY);
			/* send write enable command */
			ramcode_flash_transaction(sizeof(w_en), w_en, 0, NULL, CMD_END);
			/* make sure busy bit cleared and write enabled. */
			ramcode_flash_cmd_read_status(FLASH_SR_ALL, FLASH_SR_WEL);
			/* re-send write command */
			flash_write[1] = (addr >> 16) & GENMASK(7, 0);
			flash_write[2] = (addr >> 8) & GENMASK(7, 0);
			flash_write[3] = addr & GENMASK(7, 0);
			ramcode_flash_transaction(sizeof(flash_write), flash_write,
				0, NULL, CMD_CONTINUE);
		}
	}
	ramcode_flash_fsce_high();
	/* make sure busy bit cleared. */
	ramcode_flash_cmd_read_status(FLASH_SR_BUSY, FLASH_SR_NO_BUSY);
	/* exit EC-indirect follow mode */
	ramcode_flash_follow_mode_exit();
}

void __ram_code ramcode_flash_write(int addr, int wlen, const char *wbuf)
{
	ramcode_flash_cmd_write_enable();
	ramcode_flash_cmd_write(addr, wlen, (uint8_t *)wbuf);
	ramcode_flash_cmd_write_disable();
}

void __ram_code ramcode_flash_cmd_erase(int addr, int cmd)
{
	uint8_t cmd_erase[] = {cmd, ((addr >> 16) & 0xFF),
		((addr >> 8) & 0xFF), (addr & 0xFF)};

	/* enter EC-indirect follow mode */
	ramcode_flash_follow_mode();
	/* send erase command */
	ramcode_flash_transaction(sizeof(cmd_erase), cmd_erase, 0, NULL, CMD_END);
	/* make sure busy bit cleared. */
	ramcode_flash_cmd_read_status(FLASH_SR_BUSY, FLASH_SR_NO_BUSY);
	/* exit EC-indirect follow mode */
	ramcode_flash_follow_mode_exit();
}

void __ram_code ramcode_flash_erase(int addr, int cmd)
{
	ramcode_flash_cmd_write_enable();
	ramcode_flash_cmd_erase(addr, cmd);
	ramcode_flash_cmd_write_disable();
}

/**
 * Protect flash banks until reboot.
 *
 * @param start_bank    Start bank to protect
 * @param bank_count    Number of banks to protect
 */
static void flash_protect_banks(int start_bank, int bank_count,
				enum flash_wp_interface wp_if)
{
	int bank;

	for (bank = start_bank; bank < start_bank + bank_count; bank++) {
		if (wp_if & FLASH_WP_EC) {
			IT83XX_GCTRL_EWPR0PFEC(FWP_REG(bank)) |= FWP_MASK(bank);
		}
		if (wp_if & FLASH_WP_HOST) {
			IT83XX_GCTRL_EWPR0PFH(FWP_REG(bank)) |= FWP_MASK(bank);
		}
		if (wp_if & FLASH_WP_DBGR) {
			IT83XX_GCTRL_EWPR0PFD(FWP_REG(bank)) |= FWP_MASK(bank);
		}
	}
}

/* Read data from flash */
static int __ram_code flash_it8xxx2_read(const struct device *dev, off_t offset,
					 void *data, size_t len)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;
	uint8_t *data_t = data;
	int i;

	for (i = 0; i < len; i++) {
		flash_regs->SMFI_ECINDAR3 = EC_INDIRECT_READ_INTERNAL_FLASH;
		flash_regs->SMFI_ECINDAR2 = (offset >> 16) & GENMASK(7, 0);
		flash_regs->SMFI_ECINDAR1 = (offset >> 8) & GENMASK(7, 0);
		flash_regs->SMFI_ECINDAR0 = (offset & GENMASK(7, 0));

		/*
		 * Read/Write to this register will access one byte on the
		 * flash with the 32-bit flash address defined in ECINDAR3-0
		 */
		data_t[i] = flash_regs->SMFI_ECINDDR;

		offset++;
	}

	return 0;
}

/* Write data to the flash, page by page */
static int __ram_code flash_it8xxx2_write(const struct device *dev, off_t offset,
					  const void *src_data, size_t len)
{
	struct flash_it8xxx2_dev_data *data = DEV_DATA(dev);
	int ret = -EINVAL;
	unsigned int key;

	/*
	 * Check that the offset and length are multiples of the write
	 * block size.
	 */
	if ((offset % FLASH_WRITE_BLK_SZ) != 0) {
		return -EINVAL;
	}
	if ((len % FLASH_WRITE_BLK_SZ) != 0) {
		return -EINVAL;
	}
	if (data->flash_static_cache_enabled == 0) {
		return -EACCES;
	}
	if (data->all_protected) {
		return -EACCES;
	}

	k_sem_take(&data->sem, K_FOREVER);
	/*
	 * CPU can't fetch instruction from flash while use
	 * EC-indirect follow mode to access flash, interrupts need to be
	 * disabled.
	 */
	key = irq_lock();

	ramcode_flash_write(offset, len, src_data);
	ramcode_reset_i_cache();
	/* Get the ILM address of a flash offset. */
	offset |= CHIP_MAPPED_STORAGE_BASE;
	ret = ramcode_flash_verify(offset, len, src_data);

	irq_unlock(key);

	k_sem_give(&data->sem);

	return ret;
}

/* Erase multiple blocks */
static int __ram_code flash_it8xxx2_erase(const struct device *dev,
					  off_t offset, size_t len)
{
	struct flash_it8xxx2_dev_data *data = DEV_DATA(dev);
	int v_size = len, v_addr = offset, ret = -EINVAL;
	unsigned int key;

	/*
	 * Check that the offset and length are multiples of the write
	 * erase block size.
	 */
	if ((offset % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}
	if ((len % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}
	if (data->flash_static_cache_enabled == 0) {
		return -EACCES;
	}
	if (data->all_protected) {
		return -EACCES;
	}

	k_sem_take(&data->sem, K_FOREVER);
	/*
	 * CPU can't fetch instruction from flash while use
	 * EC-indirect follow mode to access flash, interrupts need to be
	 * disabled.
	 */
	key = irq_lock();

	/* Always use sector erase command */
	for (; len > 0; len -= FLASH_ERASE_BLK_SZ) {
		ramcode_flash_erase(offset, FLASH_CMD_SECTOR_ERASE);
		offset += FLASH_ERASE_BLK_SZ;
	}
	ramcode_reset_i_cache();
	/* get the ILM address of a flash offset. */
	v_addr |= CHIP_MAPPED_STORAGE_BASE;
	ret = ramcode_flash_verify(v_addr, v_size, NULL);

	irq_unlock(key);

	k_sem_give(&data->sem);

	return ret;
}

/* Enable or disable the write protection */
static int flash_it8xxx2_write_protection(const struct device *dev,
					  bool enable)
{
	struct flash_it8xxx2_dev_data *data = DEV_DATA(dev);

	if (enable) {
		/* Protect the entire flash */
		flash_protect_banks(0, CHIP_FLASH_SIZE_BYTES /
			CHIP_FLASH_BANK_SIZE, FLASH_WP_EC);
		data->all_protected = 1;
	} else {
		data->all_protected = 0;
	}

	/*
	 * bit[0], eflash protect lock register which can only be write 1 and
	 * only be cleared by power-on reset.
	 */
	IT83XX_GCTRL_EPLR |= IT83XX_GCTRL_EPLR_ENABLE;

	return 0;
}

static const struct flash_parameters *
flash_it8xxx2_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_it8xxx2_parameters;
}

static void flash_code_static_cache(const struct device *dev)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;
	struct flash_it8xxx2_dev_data *data = DEV_DATA(dev);
	unsigned int key;

	/* Make sure no interrupt while enable static cache */
	key = irq_lock();

	/* invalid static cache first */
	IT83XX_GCTRL_RVILMCR0 &= ~ILMCR_ILM0_ENABLE;

	flash_regs->SMFI_SCAR0H = IT8XXX2_SMFI_SCAR0H_ENABLE;

	memcpy((void *)CHIP_RAMCODE_BASE, (const void *)FLASH_RAMCODE_START,
		IT8XXX2_ILM_BLOCK_SIZE);

	/* RISCV ILM 0 Enable */
	IT83XX_GCTRL_RVILMCR0 |= ILMCR_ILM0_ENABLE;

	/* Enable ILM */
	flash_regs->SMFI_SCAR0L = FLASH_RAMCODE_START & GENMASK(7, 0);
	flash_regs->SMFI_SCAR0M = (FLASH_RAMCODE_START >> 8) & GENMASK(7, 0);
	flash_regs->SMFI_SCAR0H = (FLASH_RAMCODE_START >> 16) & GENMASK(2, 0);

	if (FLASH_RAMCODE_START & FLASH_RAMCODE_START_BIT19) {
		flash_regs->SMFI_SCAR0H |= IT8XXX2_SMFI_SC0A19;
	} else {
		flash_regs->SMFI_SCAR0H &= ~IT8XXX2_SMFI_SC0A19;
	}

	data->flash_static_cache_enabled = 0x01;

	irq_unlock(key);
}

static int flash_it8xxx2_init(const struct device *dev)
{
	struct flash_it8xxx2_regs *const flash_regs = FLASH_IT8XXX2_REG_BASE;
	struct flash_it8xxx2_dev_data *data = DEV_DATA(dev);

	/* By default, select internal flash for indirect fast read. */
	flash_regs->SMFI_ECINDAR3 = EC_INDIRECT_READ_INTERNAL_FLASH;

	/*
	 * If the embedded flash's size of this part number is larger
	 * than 256K-byte, enable the page program cycle constructed
	 * by EC-Indirect Follow Mode.
	 */
	flash_regs->SMFI_FLHCTRL6R |= IT8XXX2_SMFI_MASK_ECINDPP;

	/* Initialize mutex for flash controller */
	k_sem_init(&data->sem, 1, 1);

	flash_code_static_cache(dev);

	return 0;
}

static const struct flash_driver_api flash_it8xxx2_api = {
	.write_protection = flash_it8xxx2_write_protection,
	.erase = flash_it8xxx2_erase,
	.write = flash_it8xxx2_write,
	.read = flash_it8xxx2_read,
	.get_parameters = flash_it8xxx2_get_parameters,
};

static struct flash_it8xxx2_dev_data flash_it8xxx2_data;

DEVICE_DT_INST_DEFINE(0, flash_it8xxx2_init, NULL,
		      &flash_it8xxx2_data, NULL,
		      PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		      &flash_it8xxx2_api);
