/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * CFI parallel flash driver for Intel command set (cfi01 qemu device)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(flash_intel_pflash_cfi01);

#define DT_DRV_COMPAT intel_pflash_cfi01

#define SOC_NV_FLASH_COMPAT(node_id)                                                               \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (node_id), ())
#define SOC_NV_FLASH_NODE(inst) DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SOC_NV_FLASH_COMPAT)

/* CFI01 command codes */
#define PFLASH_CMD_READ_ARRAY     0x00FF
#define PFLASH_CMD_SINGLE_PROGRAM 0x40
#define PFLASH_CMD_BLOCK_ERASE    0x20
#define PFLASH_CMD_ERASE_CONFIRM  0xD0
#define PFLASH_CMD_CLEAR_STATUS   0x50
#define PFLASH_CMD_READ_STATUS    0x70
#define PFLASH_CMD_READ_DEVID     0x90
#define PFLASH_CMD_CFI_QUERY      0x98

/* Status register bits */
#define PFLASH_STATUS_READY     0x80 /* Bit 7: Device ready */
#define PFLASH_STATUS_ERASE_ERR 0x20 /* Bit 5: Erase error */
#define PFLASH_STATUS_PROG_ERR  0x10 /* Bit 4: Programming error */

struct pflash_config {
	DEVICE_MMIO_ROM; /* Must be first */
	uint32_t size;
	uint32_t erase_block_size;
	uint8_t bank_width;
	uint8_t device_width;
	struct flash_parameters params;
};

struct pflash_data {
	DEVICE_MMIO_RAM; /* Must be first */
};

/* Helper functions for memory-mapped access */
static uint32_t pflash_read32(uintptr_t addr, uint8_t bank_width)
{
	if (bank_width == 1) {
		return *(volatile uint8_t *)addr;
	} else if (bank_width == 2) {
		return *(volatile uint16_t *)addr;
	} else {
		return *(volatile uint32_t *)addr;
	}
}

static void pflash_write32(uintptr_t addr, uint32_t value, uint8_t bank_width)
{
	if (bank_width == 1) {
		*(volatile uint8_t *)addr = value & 0xFF;
	} else if (bank_width == 2) {
		*(volatile uint16_t *)addr = value & 0xFFFF;
	} else {
		*(volatile uint32_t *)addr = value;
	}
}

/* Poll status register until ready or error */
static int pflash_wait_ready(uintptr_t addr, uint8_t bank_width, uint8_t device_width)
{
	uint32_t status;
	int timeout = 10000; /* ~10ms polling */

	while (timeout-- > 0) {
		/* Read status register */
		pflash_write32(addr, PFLASH_CMD_READ_STATUS, bank_width);
		status = pflash_read32(addr, bank_width);

		if (status & PFLASH_STATUS_READY) {
			/* Check for errors */
			if (status & (PFLASH_STATUS_PROG_ERR | PFLASH_STATUS_ERASE_ERR)) {
				/* Clear status */
				pflash_write32(addr, PFLASH_CMD_CLEAR_STATUS, bank_width);
				return -EIO;
			}
			return 0;
		}
		k_busy_wait(1); /* 1 microsecond busy wait */
	}

	return -ETIMEDOUT;
}

/* Return to Read Array mode */
static void pflash_read_array(uintptr_t addr, uint8_t bank_width)
{
	pflash_write32(addr, PFLASH_CMD_READ_ARRAY, bank_width);
}

/* Flash driver API implementations */

static int pflash_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct pflash_config *cfg = dev->config;
	uint32_t base_addr = DEVICE_MMIO_GET(dev);
	uint8_t *src;

	if (len == 0) {
		return 0;
	}

	if (offset < 0 || offset + len > cfg->size || cfg->size - len < offset) {
		return -EINVAL;
	}

	/* Device is in ROM mode, just memcpy */
	src = (uint8_t *)(base_addr + offset);
	memcpy(data, src, len);

	return 0;
}

static int pflash_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct pflash_config *cfg = dev->config;
	const uint8_t *src = (const uint8_t *)data;
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	uintptr_t addr = base_addr + offset;
	size_t i;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (offset < 0 || offset + len > cfg->size || cfg->size - len < offset) {
		return -EINVAL;
	}

	/* Check alignment */
	if (offset % cfg->params.write_block_size != 0) {
		return -EINVAL;
	}

	/* Write word by word (bank_width aligned) */
	for (i = 0; i < len; i += cfg->bank_width) {
		uint32_t word = 0;
		size_t chunk_size = MIN(cfg->bank_width, len - i);

		/* Build word from input data */
		for (size_t j = 0; j < chunk_size; j++) {
			word |= (src[i + j] << (j * 8));
		}

		/* Single byte program: write cmd then data to same address */
		pflash_write32(addr + i, PFLASH_CMD_SINGLE_PROGRAM, cfg->bank_width);
		pflash_write32(addr + i, word, cfg->bank_width);

		/* Wait for operation to complete */
		ret = pflash_wait_ready(addr + i, cfg->bank_width, cfg->device_width);
		if (ret != 0) {
			LOG_ERR("Write failed at offset 0x%lx", offset + i);
			pflash_read_array(base_addr, cfg->bank_width);
			return ret;
		}
	}

	/* Return to Read Array mode */
	pflash_read_array(base_addr, cfg->bank_width);

	return 0;
}

static int pflash_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct pflash_config *cfg = dev->config;
	uintptr_t base_addr = DEVICE_MMIO_GET(dev);
	uintptr_t addr = base_addr + offset;
	size_t erased = 0;
	int ret;

	if (size == 0) {
		return 0;
	}

	if (offset < 0 || offset + size > cfg->size || cfg->size - size < offset) {
		return -EINVAL;
	}

	/* Check sector alignment */
	if (offset % cfg->erase_block_size != 0 || size % cfg->erase_block_size != 0) {
		return -EINVAL;
	}

	/* Erase sector by sector */
	while (erased < size) {
		/* Block erase: write 0x20 then 0xD0 to sector address */
		pflash_write32(addr, PFLASH_CMD_BLOCK_ERASE, cfg->bank_width);
		pflash_write32(addr, PFLASH_CMD_ERASE_CONFIRM, cfg->bank_width);

		/* Wait for erase to complete */
		ret = pflash_wait_ready(addr, cfg->bank_width, cfg->device_width);
		if (ret != 0) {
			LOG_ERR("Erase failed at offset 0x%lx", offset + erased);
			pflash_read_array(base_addr, cfg->bank_width);
			return ret;
		}

		addr += cfg->erase_block_size;
		erased += cfg->erase_block_size;
	}

	/* Return to Read Array mode */
	pflash_read_array(base_addr, cfg->bank_width);

	return 0;
}

static const struct flash_parameters *pflash_get_parameters(const struct device *dev)
{
	const struct pflash_config *cfg = dev->config;

	return &cfg->params;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void pflash_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			       size_t *layout_size)
{
	const struct pflash_config *cfg = dev->config;
	static struct flash_pages_layout runtime_layout;

	runtime_layout.pages_count = cfg->size / cfg->erase_block_size;
	runtime_layout.pages_size = cfg->erase_block_size;

	*layout = &runtime_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_pflash_api) = {
	.read = pflash_read,
	.write = pflash_write,
	.erase = pflash_erase,
	.get_parameters = pflash_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = pflash_page_layout,
#endif
};

static int pflash_init(const struct device *dev)
{
	const struct pflash_config *cfg = dev->config;

	LOG_DBG("Intel pflash CFI01 driver initialized");
	LOG_DBG("  Sector length: 0x%x bytes", cfg->erase_block_size);
	LOG_DBG("  Bank width: %u bytes", cfg->bank_width);

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Optional: Verify CFI identity */
	uintptr_t cfi_addr = DEVICE_MMIO_GET(dev);

	/* Enter CFI query mode */
	pflash_write32(cfi_addr, PFLASH_CMD_CFI_QUERY, cfg->bank_width);

	/* Read CFI signature bytes at offsets 0x10, 0x11, 0x12 */
	uint32_t q = pflash_read32(cfi_addr + (0x10 << (cfg->bank_width - cfg->device_width)),
				   cfg->bank_width) &
		     0xFF;
	uint32_t r = pflash_read32(cfi_addr + (0x11 << (cfg->bank_width - cfg->device_width)),
				   cfg->bank_width) &
		     0xFF;
	uint32_t y = pflash_read32(cfi_addr + (0x12 << (cfg->bank_width - cfg->device_width)),
				   cfg->bank_width) &
		     0xFF;

	/* Exit CFI query mode */
	pflash_read_array(cfi_addr, cfg->bank_width);

	if (q == 'Q' && r == 'R' && y == 'Y') {
		LOG_DBG("CFI query verified (QRY signature found)");
	} else {
		LOG_WRN("CFI query signature not found: got 0x%02x 0x%02x 0x%02x", q, r, y);
	}

	return 0;
}

/* Configuration macros for each instance */
#define PFLASH_INIT(inst)                                                                          \
	static const struct pflash_config pflash_config_##inst = {                                 \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.size = DT_REG_SIZE(DT_DRV_INST(inst)),                                            \
		.erase_block_size = DT_PROP(SOC_NV_FLASH_NODE(inst), erase_block_size),       \
		.bank_width = DT_INST_PROP_OR(inst, bank_width, 1),                                \
		.device_width = DT_INST_PROP_OR(inst, device_width, 1),                            \
		.params =                                                                          \
			{                                                                          \
				.write_block_size = DT_INST_PROP_OR(inst, bank_width, 1),          \
				.erase_value = 0xff,                                               \
				.caps =                                                            \
					{                                                          \
						.no_explicit_erase = 0,                            \
					},                                                         \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct pflash_data pflash_data_##inst;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, pflash_init, NULL, &pflash_data_##inst, &pflash_config_##inst, \
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_pflash_api);

DT_INST_FOREACH_STATUS_OKAY(PFLASH_INIT)
