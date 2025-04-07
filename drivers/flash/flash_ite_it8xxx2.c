/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

#include <ilm.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_ite_it8xxx2);

#define FLASH_ITE_EC_REGS_BASE ((struct smfi_ite_ec_regs *)DT_INST_REG_ADDR(0))

struct flash_it8xxx2_dev_data {
	struct k_sem sem;
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

/*
 * This is the block size of the ILM on the it8xxx2 chip.
 * The ILM for static code cache, CPU fetch instruction from
 * ILM(ILM -> CPU)instead of flash(flash ->  I-Cache -> CPU) if enabled.
 */
#define IT8XXX2_ILM_BLOCK_SIZE 0x00001000

/* page program command  */
#define FLASH_CMD_PAGE_WRITE   0x2
/* sector erase command (erase size is 4KB) */
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

void __soc_ram_code ramcode_reset_i_cache(void)
{
#ifdef CONFIG_SOC_SERIES_IT8XXX2
	struct gctrl_ite_ec_regs *const gctrl_regs = GCTRL_ITE_EC_REGS_BASE;

	/* I-Cache tag sram reset */
	gctrl_regs->GCTRL_MCCR |= IT8XXX2_GCTRL_ICACHE_RESET;
	/* Make sure the I-Cache is reset */
	__asm__ volatile ("fence.i" ::: "memory");

	gctrl_regs->GCTRL_MCCR &= ~IT8XXX2_GCTRL_ICACHE_RESET;
	__asm__ volatile ("fence.i" ::: "memory");
#else
	custom_reset_instr_cache();
#endif
}

void __soc_ram_code ramcode_flash_follow_mode(void)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;
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

void __soc_ram_code ramcode_flash_follow_mode_exit(void)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;

	/* Exit follow mode, and keep the setting of selecting internal flash */
	flash_regs->SMFI_ECINDAR3 = EC_INDIRECT_READ_INTERNAL_FLASH;
	flash_regs->SMFI_ECINDAR2 = 0x00;
}

void __soc_ram_code ramcode_flash_fsce_high(void)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;
	struct gctrl_ite_ec_regs *const gctrl_regs = GCTRL_ITE_EC_REGS_BASE;

	/* FSCE# high level */
	flash_regs->SMFI_ECINDAR1 = (FLASH_FSCE_HIGH_ADDRESS >> 8) & GENMASK(7, 0);

	/*
	 * A short delay (15~30 us) before #CS be driven high to ensure
	 * last byte has been latched in.
	 *
	 * For a loop that writing 0 to WNCKR register for N times, the delay
	 * value will be: ((N-1) / 65.536 kHz) to (N / 65.536 kHz).
	 * So we perform 2 consecutive writes to WNCKR here to ensure the
	 * minimum delay is 15us.
	 */
	gctrl_regs->GCTRL_WNCKR = 0;
	gctrl_regs->GCTRL_WNCKR = 0;

	/* Writing 0 to EC-indirect memory data register */
	flash_regs->SMFI_ECINDDR = 0x00;
}

void __soc_ram_code ramcode_flash_write_dat(uint8_t wdata)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;

	/* Write data to FMOSI */
	flash_regs->SMFI_ECINDDR = wdata;
}

void __soc_ram_code ramcode_flash_transaction(int wlen, uint8_t *wbuf, int rlen, uint8_t *rbuf,
					      enum flash_transaction_cmd cmd_end)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;
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

void __soc_ram_code ramcode_flash_cmd_read_status(enum flash_status_mask mask,
						  enum flash_status_mask target)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;
	uint8_t cmd_rs[] = {FLASH_CMD_RS};

	/* Send read status command */
	ramcode_flash_transaction(sizeof(cmd_rs), cmd_rs, 0, NULL, CMD_CONTINUE);

	/*
	 * We prefer no timeout here. We can always get the status
	 * we want, or wait for watchdog triggered to check
	 * e-flash's status instead of breaking loop.
	 * This will avoid fetching unknown instruction from e-flash
	 * and causing exception.
	 */
	while ((flash_regs->SMFI_ECINDDR & mask) != target) {
		/* read status and check if it is we want. */
		;
	}

	/* transaction done, drive #CS high */
	ramcode_flash_fsce_high();
}

void __soc_ram_code ramcode_flash_cmd_write_enable(void)
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

void __soc_ram_code ramcode_flash_cmd_write_disable(void)
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

int __soc_ram_code ramcode_flash_verify(int addr, int size, const char *data)
{
	int i;
	uint8_t *wbuf = (uint8_t *)data;
	uint8_t *flash = (uint8_t *)addr;

	if (data == NULL) {
		/* verify for erase */
		for (i = 0; i < size; i++) {
			if (flash[i] != 0xFF) {
				return -EINVAL;
			}
		}
	} else {
		/* verify for write */
		for (i = 0; i < size; i++) {
			if (flash[i] != wbuf[i]) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

void __soc_ram_code ramcode_flash_cmd_write(int addr, int wlen, uint8_t *wbuf)
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

void __soc_ram_code ramcode_flash_write(int addr, int wlen, const char *wbuf)
{
	ramcode_flash_cmd_write_enable();
	ramcode_flash_cmd_write(addr, wlen, (uint8_t *)wbuf);
	ramcode_flash_cmd_write_disable();
}

void __soc_ram_code ramcode_flash_cmd_erase(int addr, int cmd)
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

void __soc_ram_code ramcode_flash_erase(int addr, int cmd)
{
	ramcode_flash_cmd_write_enable();
	ramcode_flash_cmd_erase(addr, cmd);
	ramcode_flash_cmd_write_disable();
}

/* Read data from flash */
static int __soc_ram_code flash_it8xxx2_read(const struct device *dev, off_t offset, void *data,
					     size_t len)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;
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
static int __soc_ram_code flash_it8xxx2_write(const struct device *dev, off_t offset,
					      const void *src_data, size_t len)
{
	struct flash_it8xxx2_dev_data *data = dev->data;
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
	if (!it8xxx2_is_ilm_configured()) {
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
static int __soc_ram_code flash_it8xxx2_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_it8xxx2_dev_data *data = dev->data;
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
	if (!it8xxx2_is_ilm_configured()) {
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

static const struct flash_parameters *
flash_it8xxx2_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_it8xxx2_parameters;
}

static int flash_it8xxx2_init(const struct device *dev)
{
	struct smfi_ite_ec_regs *const flash_regs = FLASH_ITE_EC_REGS_BASE;
	struct flash_it8xxx2_dev_data *data = dev->data;

	/* By default, select internal flash for indirect fast read. */
	flash_regs->SMFI_ECINDAR3 = EC_INDIRECT_READ_INTERNAL_FLASH;

	/*
	 * If the embedded flash's size of this part number is larger
	 * than 256K-byte, enable the page program cycle constructed
	 * by EC-Indirect Follow Mode.
	 */
	flash_regs->SMFI_FLHCTRL6R |= ITE_EC_SMFI_MASK_ECINDPP;

	/* Initialize mutex for flash controller */
	k_sem_init(&data->sem, 1, 1);

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) /
			DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_it8xxx2_pages_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_it8xxx2_api) = {
	.erase = flash_it8xxx2_erase,
	.write = flash_it8xxx2_write,
	.read = flash_it8xxx2_read,
	.get_parameters = flash_it8xxx2_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_it8xxx2_pages_layout,
#endif
};

static struct flash_it8xxx2_dev_data flash_it8xxx2_data;

DEVICE_DT_INST_DEFINE(0, flash_it8xxx2_init, NULL,
		      &flash_it8xxx2_data, NULL,
		      PRE_KERNEL_1,
		      CONFIG_FLASH_INIT_PRIORITY,
		      &flash_it8xxx2_api);
