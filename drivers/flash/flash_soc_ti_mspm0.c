/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_flash_controller

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(flash_ti_mspm0, CONFIG_FLASH_LOG_LEVEL);

#define FLASHCTL_IMASK		0x1028
#define FLASHCTL_ICLR		0x1048
#define FLASHCTL_CMDEXEC	0x1100
#define FLASHCTL_CMDTYPE	0x1104
#define FLASHCTL_CMDCTL		0x1108
#define FLASHCTL_CMDADDR	0x1120
#define FLASHCTL_CMDBYTEN	0x1124
#define FLASHCTL_CMDDATA(n)	(0x1130 + (n) * sizeof(uint32_t))
#define FLASHCTL_CMDWEPROTA	0x11D0
#define FLASHCTL_CMDWEPROTB	0x11D4
#define FLASHCTL_STATCMD	0x13D0

#define FLASHCTL_INT_DONE		BIT(0)
#define FLASHCTL_CMDEXEC_VAL		BIT(0)

#define FLASHCTL_CMD_MASK		GENMASK(2, 0)
#define FLASHCTL_CMD_PROGRAM		0x1
#define FLASHCTL_CMD_ERASE		0x2
#define FLASHCTL_CMD_CLEARSTATUS	0x5
#define FLASHCTL_SIZE_MASK		GENMASK(6, 4)
#define FLASHCTL_SIZE_ONEWORD		0x0
#define FLASHCTL_SIZE_SECTOR		0x4

#define FLASHCTL_CMDTYPE_VAL(size, cmd)				\
	(FIELD_PREP(FLASHCTL_SIZE_MASK, (size)) |		\
	 FIELD_PREP(FLASHCTL_CMD_MASK, (cmd)))

#define FLASHCTL_CMDCTL_ADDRXLATEOVR	BIT(16)
#define FLASHCTL_CMDCTL_ECCGENOVR	BIT(17)

#define FLASHCTL_STAT_CMDDONE		BIT(0)
#define FLASHCTL_STAT_CMDPASS		BIT(1)
#define FLASHCTL_STAT_FAILWEPROT	BIT(4)
#define FLASHCTL_STAT_FAILILLADDR	BIT(6)

#define FLASHCTL_WEPROTA_SECTORS	32
#define FLASHCTL_WEPROTB_GROUP		8
#define FLASHCTL_WEPROTB_BITS		32

#define MSPM0_FLASH_NODE	DT_INST_CHILD(0, flash_0)
#define MSPM0_FLASH_BASE	DT_REG_ADDR(MSPM0_FLASH_NODE)
#define MSPM0_FLASH_SIZE	DT_REG_SIZE(MSPM0_FLASH_NODE)
#define MSPM0_SECTOR_SIZE	DT_PROP(MSPM0_FLASH_NODE, erase_block_size)
#define MSPM0_WORD_SIZE		DT_PROP(MSPM0_FLASH_NODE, write_block_size)
#define MSPM0_NUM_BANKS		DT_INST_PROP(0, ti_num_banks)
#define MSPM0_SECTORS		(MSPM0_FLASH_SIZE / MSPM0_SECTOR_SIZE)
#define MSPM0_BANK_SECTORS	(MSPM0_SECTORS / MSPM0_NUM_BANKS)

#define FLASHCTL_CMDBYTEN_WORD	\
	GENMASK(MSPM0_WORD_SIZE + MSPM0_WORD_SIZE / 8 - 1, 0)

BUILD_ASSERT(MSPM0_WORD_SIZE == 8 || MSPM0_WORD_SIZE == 16,
	     "Flash word must be 64 or 128 bits");
BUILD_ASSERT((MSPM0_SECTOR_SIZE % MSPM0_WORD_SIZE) == 0,
	     "Flash sector size must be a multiple of the word size");
BUILD_ASSERT((MSPM0_SECTORS % MSPM0_NUM_BANKS) == 0,
	     "Flash banks must be of equal size");
BUILD_ASSERT(MSPM0_BANK_SECTORS <=
	     FLASHCTL_WEPROTB_BITS * FLASHCTL_WEPROTB_GROUP,
	     "Flash bank size not supported by CMDWEPROTB");

struct flash_ti_mspm0_config {
	uintptr_t base;
	uint32_t num_banks;
	bool has_weprota;
	const struct flash_parameters parameters;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout flash_layout;
#endif
};

struct flash_ti_mspm0_data {
	struct k_mutex lock;
};

static inline bool flash_ti_mspm0_valid_range(off_t offset, size_t len)
{
	return (offset >= 0) && ((size_t)offset < MSPM0_FLASH_SIZE) &&
	       ((MSPM0_FLASH_SIZE - (size_t)offset) >= len);
}

static __ramfunc uint32_t flash_ti_mspm0_exec(uintptr_t base)
{
	uint32_t exec;

	sys_write32(FLASHCTL_CMDEXEC_VAL, base + FLASHCTL_CMDEXEC);

	do {
		exec = sys_read32(base + FLASHCTL_CMDEXEC);
	} while ((exec & FLASHCTL_CMDEXEC_VAL) != 0U);

	return sys_read32(base + FLASHCTL_STATCMD);
}

static uint32_t flash_ti_mspm0_run(const struct device *dev)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;
	unsigned int key;
	uint32_t status;

	key = irq_lock();
	status = flash_ti_mspm0_exec(cfg->base);
	irq_unlock(key);

	return status;
}

static int flash_ti_mspm0_check(uint32_t status, off_t offset)
{
	const uint32_t done = FLASHCTL_STAT_CMDDONE | FLASHCTL_STAT_CMDPASS;
	const uint32_t prot = FLASHCTL_STAT_FAILWEPROT |
			      FLASHCTL_STAT_FAILILLADDR;

	if ((status & done) == done) {
		return 0;
	}

	LOG_ERR("Flash command at 0x%lx failed, STATCMD: 0x%08x",
		(long)offset, status);

	return ((status & prot) != 0) ? -EACCES : -EIO;
}

static void flash_ti_mspm0_clear_status(const struct device *dev)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	sys_write32(FLASHCTL_CMDTYPE_VAL(FLASHCTL_SIZE_ONEWORD,
					 FLASHCTL_CMD_CLEARSTATUS),
		    cfg->base + FLASHCTL_CMDTYPE);

	(void)flash_ti_mspm0_run(dev);
}

static void flash_ti_mspm0_unprotect(const struct device *dev, off_t offset)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;
	uint32_t sector = (offset / MSPM0_SECTOR_SIZE) % MSPM0_BANK_SECTORS;
	uint32_t weprota = UINT32_MAX;
	uint32_t weprotb = UINT32_MAX;

	if (cfg->has_weprota && (sector < FLASHCTL_WEPROTA_SECTORS)) {
		weprota &= ~BIT(sector);
	}

	if (!cfg->has_weprota || (cfg->num_banks > 1)) {
		weprotb &= ~BIT(sector / FLASHCTL_WEPROTB_GROUP);
	} else if (sector >= FLASHCTL_WEPROTA_SECTORS) {
		weprotb &= ~BIT((sector - FLASHCTL_WEPROTA_SECTORS) /
				FLASHCTL_WEPROTB_GROUP);
	}

	if (cfg->has_weprota) {
		sys_write32(weprota, cfg->base + FLASHCTL_CMDWEPROTA);
	}

	sys_write32(weprotb, cfg->base + FLASHCTL_CMDWEPROTB);
}

static int flash_ti_mspm0_erase_sector(const struct device *dev,
				       off_t offset)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	flash_ti_mspm0_clear_status(dev);
	flash_ti_mspm0_unprotect(dev, offset);

	sys_write32(FLASHCTL_CMDTYPE_VAL(FLASHCTL_SIZE_SECTOR,
					 FLASHCTL_CMD_ERASE),
		    cfg->base + FLASHCTL_CMDTYPE);
	sys_write32(MSPM0_FLASH_BASE + offset, cfg->base + FLASHCTL_CMDADDR);

	return flash_ti_mspm0_check(flash_ti_mspm0_run(dev), offset);
}

static int flash_ti_mspm0_program_word(const struct device *dev,
				       off_t offset, const uint8_t *src)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	flash_ti_mspm0_clear_status(dev);
	flash_ti_mspm0_unprotect(dev, offset);

	sys_write32(FLASHCTL_CMDTYPE_VAL(FLASHCTL_SIZE_ONEWORD,
					 FLASHCTL_CMD_PROGRAM),
		    cfg->base + FLASHCTL_CMDTYPE);
	sys_write32(MSPM0_FLASH_BASE + offset, cfg->base + FLASHCTL_CMDADDR);
	sys_write32(FLASHCTL_CMDBYTEN_WORD, cfg->base + FLASHCTL_CMDBYTEN);

	for (size_t i = 0; i < MSPM0_WORD_SIZE / sizeof(uint32_t); i++) {
		sys_write32(sys_get_le32(&src[i * sizeof(uint32_t)]),
			    cfg->base + FLASHCTL_CMDDATA(i));
	}

	return flash_ti_mspm0_check(flash_ti_mspm0_run(dev), offset);
}

static int flash_ti_mspm0_erase(const struct device *dev, off_t offset,
				size_t len)
{
	struct flash_ti_mspm0_data *data = dev->data;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (!flash_ti_mspm0_valid_range(offset, len)) {
		LOG_ERR("Erase range invalid. Offset %ld, len: %zu",
			(long)offset, len);
		return -EINVAL;
	}

	if (!IS_ALIGNED(offset, MSPM0_SECTOR_SIZE) ||
	    !IS_ALIGNED(len, MSPM0_SECTOR_SIZE)) {
		LOG_ERR("Erase offset and length must be sector aligned");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len != 0) {
		ret = flash_ti_mspm0_erase_sector(dev, offset);
		if (ret < 0) {
			break;
		}

		offset += MSPM0_SECTOR_SIZE;
		len -= MSPM0_SECTOR_SIZE;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int flash_ti_mspm0_write(const struct device *dev, off_t offset,
				const void *buf, size_t len)
{
	struct flash_ti_mspm0_data *data = dev->data;
	const uint8_t *src = buf;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (!flash_ti_mspm0_valid_range(offset, len)) {
		LOG_ERR("Write range invalid. Offset %ld, len: %zu",
			(long)offset, len);
		return -EINVAL;
	}

	if (!IS_ALIGNED(offset, MSPM0_WORD_SIZE) ||
	    !IS_ALIGNED(len, MSPM0_WORD_SIZE)) {
		LOG_ERR("Write offset and length must be write block aligned");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	while (len != 0) {
		ret = flash_ti_mspm0_program_word(dev, offset, src);
		if (ret < 0) {
			break;
		}

		offset += MSPM0_WORD_SIZE;
		src += MSPM0_WORD_SIZE;
		len -= MSPM0_WORD_SIZE;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int flash_ti_mspm0_read(const struct device *dev, off_t offset,
			       void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (len == 0) {
		return 0;
	}

	if (!flash_ti_mspm0_valid_range(offset, len)) {
		LOG_ERR("Read range invalid. Offset %ld, len: %zu",
			(long)offset, len);
		return -EINVAL;
	}

	memcpy(data, (const uint8_t *)(MSPM0_FLASH_BASE + offset), len);

	return 0;
}

static const struct flash_parameters *
flash_ti_mspm0_get_parameters(const struct device *dev)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	return &cfg->parameters;
}

static int flash_ti_mspm0_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = MSPM0_FLASH_SIZE;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void flash_ti_mspm0_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;

	*layout = &cfg->flash_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_ti_mspm0_driver_api) = {
	.erase = flash_ti_mspm0_erase,
	.write = flash_ti_mspm0_write,
	.read = flash_ti_mspm0_read,
	.get_parameters = flash_ti_mspm0_get_parameters,
	.get_size = flash_ti_mspm0_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_ti_mspm0_page_layout,
#endif
};

static int flash_ti_mspm0_init(const struct device *dev)
{
	const struct flash_ti_mspm0_config *cfg = dev->config;
	struct flash_ti_mspm0_data *data = dev->data;
	uint32_t ctl;

	k_mutex_init(&data->lock);

	/* Use hardware ECC generation and system to bank address translation */
	ctl = sys_read32(cfg->base + FLASHCTL_CMDCTL);
	ctl &= ~(FLASHCTL_CMDCTL_ECCGENOVR | FLASHCTL_CMDCTL_ADDRXLATEOVR);
	sys_write32(ctl, cfg->base + FLASHCTL_CMDCTL);

	/* Completion is polled, keep the DONE interrupt masked and cleared */
	sys_write32(0, cfg->base + FLASHCTL_IMASK);
	sys_write32(FLASHCTL_INT_DONE, cfg->base + FLASHCTL_ICLR);

	return 0;
}

#define FLASH_TI_MSPM0_HAS_WEPROTA(inst)				\
	!DT_INST_PROP(inst, ti_no_weprota)

#define FLASH_TI_MSPM0_DEVICE_INIT(inst)				\
	static const struct flash_ti_mspm0_config			\
		flash_ti_mspm0_cfg##inst = {				\
		.base = DT_INST_REG_ADDR(inst),				\
		.num_banks = DT_INST_PROP(inst, ti_num_banks),		\
		.has_weprota = FLASH_TI_MSPM0_HAS_WEPROTA(inst),	\
		.parameters = {						\
			.write_block_size = MSPM0_WORD_SIZE,		\
			.erase_value = 0xff,				\
		},							\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (			\
		.flash_layout = {					\
			.pages_count = MSPM0_SECTORS,			\
			.pages_size = MSPM0_SECTOR_SIZE,		\
		},))							\
	};								\
									\
	static struct flash_ti_mspm0_data flash_ti_mspm0_data##inst;	\
									\
	DEVICE_DT_INST_DEFINE(inst, flash_ti_mspm0_init, NULL,		\
			      &flash_ti_mspm0_data##inst,		\
			      &flash_ti_mspm0_cfg##inst, POST_KERNEL,	\
			      CONFIG_FLASH_INIT_PRIORITY,		\
			      &flash_ti_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_TI_MSPM0_DEVICE_INIT);
