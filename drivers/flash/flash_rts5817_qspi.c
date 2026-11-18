/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pm/device.h>
#include <string.h>
#ifdef CONFIG_PM_DEVICE
#include <zephyr/drivers/syscon.h>
#endif
#include "flash_rts5817_qspi_reg.h"
#include "flash_rts5817_qspi.h"
#include "dlink_sys_reg.h"

#define DT_DRV_COMPAT realtek_rts5817_flash_controller

LOG_MODULE_REGISTER(flash_rts5817_qspi, CONFIG_FLASH_LOG_LEVEL);

/* JEDEC ID returned by the SPI_NOR_CMD_RDID command: manufacturer, memory type, capacity. */
#define JEDEC_ID_LEN 3

#define FLASH_PROTECT_ALL_BITS GENMASK(4, 2)

#define TRANSFER_TIME_OUT_US      (2500 * 1000)
#define TRANSFER_POLL_INTERVAL_US 10
#define QSPI_DEFAULT_CLK_FREQ     MHZ(120)
#define HOLD_PUPD_SWITCH_DELAY_US 10
#define HOLD_PUPD_SETTLE_DELAY_US 100

/* Controller register encoding for 8 fast-read dummy cycles (cycles - 1). */
#define RTS5817_QSPI_DUMMY_CYCLES_8_REG (8 - 1)
/* Quad Enable (QE) is bit 1 of status register 2, which is bit 9 of the two registers. */
#define RTS5817_QSPI_SR2_QE_BIT         (BITS_PER_BYTE + 1)

struct flash_rts5817_qspi_config {
	mem_addr_t qspi_reg;
	mem_addr_t auto_reg;
	int bus_width;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#ifdef CONFIG_PM_DEVICE
	const struct device *syscon_sys;
#endif
};

struct flash_rts5817_qspi_data {
	const struct rts5817_spi_flash_params *flash;
	uint8_t read_mode;
	uint8_t read_cmd;
	/*
	 * Held with interrupts masked across a transfer: the driver runs XIP from this flash and
	 * the controller cannot serve fetches while it transfers, so an interrupt taken in the
	 * meantime could stall on one.
	 */
	struct k_spinlock lock;
	/* Serializes write and erase, which share the bounce buffer and are not atomic. */
	struct k_sem sem;
	/* The page program DMAs from here: the source of flash_write() may be in this flash. */
	uint8_t write_buf[SPI_NOR_PAGE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif
};

static const struct rts5817_spi_flash_params rts5817_flash_params_table[] = {
	{
		.jedec_id = RTS5817_FLASH_ID_XT25F16F,
		.status_reg_access = RTS5817_STATUS_REG_DUAL_COMBINED_WRITE,
		.dummy_cycles_reg = RTS5817_QSPI_DUMMY_CYCLES_8_REG,
		.quad_enable_bit = RTS5817_QSPI_SR2_QE_BIT,
		.volatile_status_reg_wren_cmd = RTS5817_FLASH_CMD_VOLATILE_SR_WREN,
		.flash_size = MB(2),
	},
	{
		.jedec_id = RTS5817_FLASH_ID_FM25W16,
		.status_reg_access = RTS5817_STATUS_REG_DUAL_SPLIT_WRITE,
		.dummy_cycles_reg = RTS5817_QSPI_DUMMY_CYCLES_8_REG,
		.quad_enable_bit = RTS5817_QSPI_SR2_QE_BIT,
		.volatile_status_reg_wren_cmd = RTS5817_FLASH_CMD_VOLATILE_SR_WREN,
		.flash_size = MB(2),
	},
	{
		.jedec_id = RTS5817_FLASH_ID_GD25Q16,
		.status_reg_access = RTS5817_STATUS_REG_DUAL_COMBINED_WRITE,
		.dummy_cycles_reg = RTS5817_QSPI_DUMMY_CYCLES_8_REG,
		.quad_enable_bit = RTS5817_QSPI_SR2_QE_BIT,
		.volatile_status_reg_wren_cmd = RTS5817_FLASH_CMD_VOLATILE_SR_WREN,
		.flash_size = MB(2),
	},
	{
		.jedec_id = RTS5817_FLASH_ID_P25Q16SH,
		.status_reg_access = RTS5817_STATUS_REG_DUAL_SPLIT_WRITE,
		.dummy_cycles_reg = RTS5817_QSPI_DUMMY_CYCLES_8_REG,
		.quad_enable_bit = RTS5817_QSPI_SR2_QE_BIT,
		.volatile_status_reg_wren_cmd = RTS5817_FLASH_CMD_VOLATILE_SR_WREN,
		.flash_size = MB(2),
	},
	{
		.jedec_id = RTS5817_FLASH_ID_W25Q16JVSIQ,
		.status_reg_access = RTS5817_STATUS_REG_DUAL_SPLIT_WRITE,
		.dummy_cycles_reg = RTS5817_QSPI_DUMMY_CYCLES_8_REG,
		.quad_enable_bit = RTS5817_QSPI_SR2_QE_BIT,
		.volatile_status_reg_wren_cmd = RTS5817_FLASH_CMD_VOLATILE_SR_WREN,
		.flash_size = MB(2),
	},
	{
		.jedec_id = RTS5817_FLASH_ID_ZG25VQ16D,
		.status_reg_access = RTS5817_STATUS_REG_DUAL_SPLIT_WRITE,
		.dummy_cycles_reg = RTS5817_QSPI_DUMMY_CYCLES_8_REG,
		.quad_enable_bit = RTS5817_QSPI_SR2_QE_BIT,
		.volatile_status_reg_wren_cmd = RTS5817_FLASH_CMD_VOLATILE_SR_WREN,
		.flash_size = MB(2),
	},
};

static inline uint32_t rts_fp_qspi_read_reg(const struct device *dev, uint32_t offset)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;

	return sys_read32(dev_cfg->qspi_reg + offset);
}

static inline void rts_fp_qspi_write_reg(const struct device *dev, uint32_t data, uint32_t offset)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;

	sys_write32(data, dev_cfg->qspi_reg + offset);
}

static void rts_fp_qspi_init_read_mode(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;
	struct flash_rts5817_qspi_data *dev_data = dev->data;

	switch (dev_cfg->bus_width) {
	case 1:
		dev_data->read_mode = RTS5817_QSPI_XFER_FAST_READ;
		dev_data->read_cmd = SPI_NOR_CMD_READ_FAST;
		break;
	case 2:
		dev_data->read_mode = RTS5817_QSPI_XFER_FAST_READ_DUAL_OUT;
		dev_data->read_cmd = SPI_NOR_CMD_DREAD;
		break;
	default:
		dev_data->read_mode = RTS5817_QSPI_XFER_FAST_READ_QUAD_OUT;
		dev_data->read_cmd = SPI_NOR_CMD_QREAD;
		break;
	}
}

static int rts_fp_qspi_transfer(const struct device *dev, uint32_t dma_addr, size_t length,
				uint8_t cmd_nums, uint8_t dma_dir)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;

	rts_fp_qspi_write_reg(dev, dma_dir, R_SPI_WB_DMA_DIR);
	rts_fp_qspi_write_reg(dev, dma_addr, R_SPI_WB_DMA_ADDR);
	rts_fp_qspi_write_reg(dev, length, R_SPI_WB_DMA_LEN);

	rts_fp_qspi_write_reg(dev, RTS5817_QSPI_WB_START_MASK | cmd_nums, R_SPI_COM_TRANSFER);

	if (WAIT_FOR(sys_test_bit(dev_cfg->qspi_reg + R_SPI_COM_TRANSFER,
				  RTS5817_QSPI_WB_IDLE_OFFSET),
		     TRANSFER_TIME_OUT_US, k_busy_wait(TRANSFER_POLL_INTERVAL_US))) {
		return 0;
	}

	sys_set_bit(dev_cfg->qspi_reg + R_SPI_COM_TRANSFER, RTS5817_QSPI_WB_RST_OFFSET);

	return -ETIMEDOUT;
}

static void rts_fp_qspi_write_enable_using_cmd1(const struct device *dev)
{
	rts_fp_qspi_write_reg(dev, SPI_NOR_CMD_WREN, R_SPI_SUB1_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB1_ADDR);
	rts_fp_qspi_write_reg(dev, RTS5817_QSPI_XFER_CMD_ONLY, R_SPI_SUB1_MODE);
}

static void rts_fp_qspi_polling_status_using_cmd3(const struct device *dev)
{
	rts_fp_qspi_write_reg(dev, SPI_NOR_CMD_RDSR, R_SPI_SUB3_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB3_ADDR);
	rts_fp_qspi_write_reg(dev, RTS5817_QSPI_XFER_POLL_STATUS, R_SPI_SUB3_MODE);
}

/*
 * Read len bytes into data with DMA. data must be aligned to and padded to a multiple of
 * CONFIG_DCACHE_LINE_SIZE, so that the cache maintenance only touches this buffer.
 */
static int rts_fp_qspi_read_align(const struct device *dev, uint8_t read_mode, uint8_t read_cmd,
				  off_t addr, size_t len, uint8_t *data)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	size_t cache_len = ROUND_UP(len, CONFIG_DCACHE_LINE_SIZE);
	int ret;
	k_spinlock_key_t key;

	if (data == NULL || !IS_ALIGNED(data, CONFIG_DCACHE_LINE_SIZE)) {
		LOG_ERR("Read buffer address is NULL or not aligned to %d",
			CONFIG_DCACHE_LINE_SIZE);
		return -EINVAL;
	}

	/* R_SPI_WB_DMA_LEN and R_SPI_SUB1_LENGTH are both 16 bits wide. */
	if (len == 0 || len > UINT16_MAX) {
		return -EINVAL;
	}

	key = k_spin_lock(&dev_data->lock);
	/* Write back dirty lines so they cannot be evicted over the data written by the DMA. */
	sys_cache_data_flush_range(data, cache_len);

	rts_fp_qspi_write_reg(dev, read_cmd, R_SPI_SUB1_COMMAND);
	rts_fp_qspi_write_reg(dev, read_mode, R_SPI_SUB1_MODE);
	rts_fp_qspi_write_reg(dev, addr, R_SPI_SUB1_ADDR);
	rts_fp_qspi_write_reg(dev, len, R_SPI_SUB1_LENGTH);
	ret = rts_fp_qspi_transfer(dev, (uint32_t)(uintptr_t)data, len,
				   RTS5817_QSPI_COM_TRANSFER_NUM_1, RTS5817_QSPI_DMA_SPI_TO_DTCM);

	/* Drop stale lines so the CPU reads what the DMA wrote. */
	sys_cache_data_invd_range(data, cache_len);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static int rts_fp_read_jedec_id(const struct device *dev, uint32_t *id)
{
	uint8_t temp[CONFIG_DCACHE_LINE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
	int ret;

	ret = rts_fp_qspi_read_align(dev, RTS5817_QSPI_XFER_CMD_DATA_IN, SPI_NOR_CMD_RDID, 0,
				     JEDEC_ID_LEN, temp);
	if (ret != 0) {
		return ret;
	}

	*id = sys_get_be24(temp);

	return 0;
}

static int rts_fp_qspi_write_status(const struct device *dev, uint8_t cmd, uint8_t wren_cmd,
				    uint8_t *value, uint8_t len)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&dev_data->lock);
	sys_cache_data_flush_range(value, len);
	rts_fp_qspi_write_reg(dev, wren_cmd, R_SPI_SUB1_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB1_ADDR);
	rts_fp_qspi_write_reg(dev, RTS5817_QSPI_XFER_CMD_ONLY, R_SPI_SUB1_MODE);

	rts_fp_qspi_write_reg(dev, cmd, R_SPI_SUB2_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB2_ADDR);
	rts_fp_qspi_write_reg(dev, len, R_SPI_SUB2_LENGTH);
	rts_fp_qspi_write_reg(dev, RTS5817_QSPI_XFER_CMD_DATA_OUT, R_SPI_SUB2_MODE);

	rts_fp_qspi_polling_status_using_cmd3(dev);
	ret = rts_fp_qspi_transfer(dev, (uint32_t)(uintptr_t)value, len,
				   RTS5817_QSPI_COM_TRANSFER_NUM_3, RTS5817_QSPI_DMA_DTCM_TO_SPI);
	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static int rts_fp_qspi_read_status_reg(const struct device *dev, uint8_t cmd, uint8_t *status)
{
	uint8_t temp[CONFIG_DCACHE_LINE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
	int ret;

	ret = rts_fp_qspi_read_align(dev, RTS5817_QSPI_XFER_CMD_DATA_IN, cmd, 0, 1, temp);
	if (ret != 0) {
		return ret;
	}
	*status = temp[0];

	return 0;
}

static int rts_fp_flash_read_status(const struct device *dev, uint8_t *value)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret;

	switch (dev_data->flash->status_reg_access) {
	case RTS5817_STATUS_REG_DUAL_COMBINED_WRITE:
	case RTS5817_STATUS_REG_DUAL_SPLIT_WRITE:
		ret = rts_fp_qspi_read_status_reg(dev, SPI_NOR_CMD_RDSR, &value[0]);
		if (ret != 0) {
			return ret;
		}
		return rts_fp_qspi_read_status_reg(dev, SPI_NOR_CMD_RDSR2, &value[1]);
	default:
		return -ENOTSUP;
	}
}

static int rts_fp_flash_write_status(const struct device *dev, uint8_t *value, bool is_volatile)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t wren_cmd =
		is_volatile ? dev_data->flash->volatile_status_reg_wren_cmd : SPI_NOR_CMD_WREN;
	int ret;

	switch (dev_data->flash->status_reg_access) {
	case RTS5817_STATUS_REG_DUAL_COMBINED_WRITE:
		return rts_fp_qspi_write_status(dev, SPI_NOR_CMD_WRSR, wren_cmd, value, 2);
	case RTS5817_STATUS_REG_DUAL_SPLIT_WRITE:
		ret = rts_fp_qspi_write_status(dev, SPI_NOR_CMD_WRSR, wren_cmd, value, 1);
		if (ret != 0) {
			return ret;
		}
		return rts_fp_qspi_write_status(dev, SPI_NOR_CMD_WRSR2, wren_cmd, &value[1], 1);
	default:
		return -ENOTSUP;
	}
}

/* Automatic protection around write and erase: a no-op when it is not enabled. */
static int rts_fp_flash_soft_write_protect(const struct device *dev, bool enable)
{
	uint8_t status_reg[2] = {0, 0};
	uint8_t target = enable ? FLASH_PROTECT_ALL_BITS : 0;
	int ret;

	if (!IS_ENABLED(CONFIG_FLASH_RTS5817_WRITE_PROTECTION_AUTO)) {
		return 0;
	}

	ret = rts_fp_flash_read_status(dev, status_reg);
	if (ret != 0) {
		return ret;
	}

	if ((status_reg[0] & FLASH_PROTECT_ALL_BITS) == target) {
		return 0;
	}

	status_reg[0] = (status_reg[0] & ~FLASH_PROTECT_ALL_BITS) | target;

	return rts_fp_flash_write_status(dev, status_reg, true);
}

static int rts_fp_flash_quad_cfg(const struct device *dev, bool enable)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t status_reg[2] = {0};
	uint8_t reg_idx = dev_data->flash->quad_enable_bit / BITS_PER_BYTE;
	uint8_t bit_pos = dev_data->flash->quad_enable_bit % BITS_PER_BYTE;
	int ret;

	if (reg_idx >= ARRAY_SIZE(status_reg)) {
		return -ENOTSUP;
	}

	ret = rts_fp_flash_read_status(dev, status_reg);
	if (ret != 0) {
		return ret;
	}

	/* Status register writes are non-volatile, so avoid them when QE already matches. */
	if (IS_BIT_SET(status_reg[reg_idx], bit_pos) == enable) {
		return 0;
	}

	WRITE_BIT(status_reg[reg_idx], bit_pos, enable);

	return rts_fp_flash_write_status(dev, status_reg, false);
}

/* The two writes run from the I-cache line, so the code is there while the read mode changes. */
static void __aligned(CONFIG_ICACHE_LINE_SIZE) __noinline set_qspi_auto_mode(mem_addr_t qspi_reg,
									     mem_addr_t auto_reg,
									     uint32_t tctl,
									     uint32_t auto_mode)
{
	sys_write32(tctl, qspi_reg + R_SPI_TCTL);
	sys_write32(auto_mode, auto_reg);
}

static void rts_fp_qspi_set_auto_mode(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint32_t auto_mode;
	uint32_t tctl;
	k_spinlock_key_t key;

	auto_mode = FIELD_PREP(CACHE_A_SPI_DUM_BIT_NUM_MASK, dev_data->flash->dummy_cycles_reg) |
		    FIELD_PREP(CACHE_SPI_AUTOMODE_TRANSFER_MODE_MASK, dev_data->read_mode) |
		    FIELD_PREP(CACHE_SPI_CMD_MASK, dev_data->read_cmd);

	key = k_spin_lock(&dev_data->lock);
	tctl = rts_fp_qspi_read_reg(dev, R_SPI_TCTL) & ~RTS5817_QSPI_U_DUM_BIT_NUM_MASK;
	tctl |= FIELD_PREP(RTS5817_QSPI_U_DUM_BIT_NUM_MASK, dev_data->flash->dummy_cycles_reg);
	set_qspi_auto_mode(dev_cfg->qspi_reg, dev_cfg->auto_reg, tctl, auto_mode);
	k_spin_unlock(&dev_data->lock, key);

	LOG_DBG("Auto mode reg:%x", sys_read32(dev_cfg->auto_reg));
}

static inline bool rts_fp_qspi_is_range_valid(const struct flash_rts5817_qspi_data *dev_data,
					      off_t offset, size_t len)
{
	return offset >= 0 && (size_t)offset <= dev_data->flash->flash_size &&
	       len <= dev_data->flash->flash_size - (size_t)offset;
}

/*
 * Invalidate the cache lines of the part of the range that is inside the memory-mapped window,
 * which starts at CONFIG_FLASH_LOAD_OFFSET.
 */
static void rts_fp_qspi_cache_invd(size_t offset, size_t len)
{
	if (offset + len <= CONFIG_FLASH_LOAD_OFFSET) {
		return;
	}

	if (offset < CONFIG_FLASH_LOAD_OFFSET) {
		len -= CONFIG_FLASH_LOAD_OFFSET - offset;
		offset = CONFIG_FLASH_LOAD_OFFSET;
	}

	void *addr = (void *)((uintptr_t)CONFIG_FLASH_BASE_ADDRESS + offset);

	sys_cache_data_invd_range(addr, len);
	/* The code in flash may be rewritten and executed again without a reset. */
	(void)sys_cache_instr_invd_range(addr, len);
}

static int flash_rts5817_qspi_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t temp_buf[CONFIG_DCACHE_LINE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
	uint8_t *dst = data;
	size_t unaligned;
	size_t temp_len;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (dst == NULL || !rts_fp_qspi_is_range_valid(dev_data, offset, len)) {
		return -EINVAL;
	}

	unaligned = (uintptr_t)dst % CONFIG_DCACHE_LINE_SIZE;
	if (unaligned != 0) {
		temp_len = MIN(len, CONFIG_DCACHE_LINE_SIZE - unaligned);
		ret = rts_fp_qspi_read_align(dev, dev_data->read_mode, dev_data->read_cmd, offset,
					     temp_len, temp_buf);
		if (ret != 0) {
			return ret;
		}
		memcpy(dst, temp_buf, temp_len);
		len -= temp_len;
		offset += temp_len;
		dst += temp_len;
	}

	/* A single DMA transfer is limited to 16 bits of length, so split the aligned body. */
	while (len >= CONFIG_DCACHE_LINE_SIZE) {
		temp_len = MIN(ROUND_DOWN(len, CONFIG_DCACHE_LINE_SIZE),
			       ROUND_DOWN(UINT16_MAX, CONFIG_DCACHE_LINE_SIZE));
		ret = rts_fp_qspi_read_align(dev, dev_data->read_mode, dev_data->read_cmd, offset,
					     temp_len, dst);
		if (ret != 0) {
			return ret;
		}
		len -= temp_len;
		offset += temp_len;
		dst += temp_len;
	}

	if (len != 0) {
		ret = rts_fp_qspi_read_align(dev, dev_data->read_mode, dev_data->read_cmd, offset,
					     len, temp_buf);
		if (ret != 0) {
			return ret;
		}
		memcpy(dst, temp_buf, len);
	}

	return 0;
}

static int rts_fp_qspi_page_program(const struct device *dev, uint32_t offset, uint16_t len,
				    const void *data)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&dev_data->lock);

	sys_cache_data_flush_range((void *)data, len);

	rts_fp_qspi_write_enable_using_cmd1(dev);

	rts_fp_qspi_write_reg(dev, SPI_NOR_CMD_PP, R_SPI_SUB2_COMMAND);
	rts_fp_qspi_write_reg(dev, offset, R_SPI_SUB2_ADDR);
	rts_fp_qspi_write_reg(dev, len, R_SPI_SUB2_LENGTH);
	rts_fp_qspi_write_reg(dev, RTS5817_QSPI_XFER_CMD_ADDR_DATA_OUT, R_SPI_SUB2_MODE);

	rts_fp_qspi_polling_status_using_cmd3(dev);
	ret = rts_fp_qspi_transfer(dev, (uint32_t)(uintptr_t)data, len,
				   RTS5817_QSPI_COM_TRANSFER_NUM_3, RTS5817_QSPI_DMA_DTCM_TO_SPI);

	rts_fp_qspi_cache_invd(offset, len);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static int flash_rts5817_qspi_write(const struct device *dev, off_t offset, const void *data,
				    size_t len)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int wp_ret;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (data == NULL || !rts_fp_qspi_is_range_valid(dev_data, offset, len)) {
		return -EINVAL;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);

	ret = rts_fp_flash_soft_write_protect(dev, false);
	if (ret != 0) {
		k_sem_give(&dev_data->sem);
		return ret;
	}

	while (len != 0) {
		uint16_t page_len = MIN(len, SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE));

		memcpy(dev_data->write_buf, data, page_len);

		ret = rts_fp_qspi_page_program(dev, offset, page_len, dev_data->write_buf);
		if (ret != 0) {
			LOG_ERR("page program err %d", ret);
			break;
		}
		data = (const uint8_t *)data + page_len;
		offset += page_len;
		len -= page_len;
	}

	wp_ret = rts_fp_flash_soft_write_protect(dev, true);
	if (wp_ret != 0 && ret == 0) {
		ret = wp_ret;
	}

	k_sem_give(&dev_data->sem);

	return ret;
}

static int rts_fp_qspi_sector_erase(const struct device *dev, off_t offset)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&dev_data->lock);

	rts_fp_qspi_write_enable_using_cmd1(dev);

	rts_fp_qspi_write_reg(dev, SPI_NOR_CMD_SE, R_SPI_SUB2_COMMAND);
	rts_fp_qspi_write_reg(dev, offset, R_SPI_SUB2_ADDR);
	rts_fp_qspi_write_reg(dev, RTS5817_QSPI_XFER_CMD_ADDR, R_SPI_SUB2_MODE);

	rts_fp_qspi_polling_status_using_cmd3(dev);

	ret = rts_fp_qspi_transfer(dev, 0, 0, RTS5817_QSPI_COM_TRANSFER_NUM_3,
				   RTS5817_QSPI_DMA_SPI_TO_DTCM);

	rts_fp_qspi_cache_invd(offset, SPI_NOR_SECTOR_SIZE);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/*
 * Erasing holds the spinlock, so interrupts stay masked until the sector is done, up to 2.5s.
 * The caller has to keep the watchdog from firing during that window.
 */
static int flash_rts5817_qspi_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int wp_ret;
	int ret;

	if (!rts_fp_qspi_is_range_valid(dev_data, offset, size)) {
		return -EINVAL;
	}

	if (!SPI_NOR_IS_SECTOR_ALIGNED(offset) || !SPI_NOR_IS_SECTOR_ALIGNED(size)) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);

	ret = rts_fp_flash_soft_write_protect(dev, false);
	if (ret != 0) {
		k_sem_give(&dev_data->sem);
		return ret;
	}

	while (size != 0) {
		ret = rts_fp_qspi_sector_erase(dev, offset);
		if (ret != 0) {
			break;
		}
		offset += SPI_NOR_SECTOR_SIZE;
		size -= SPI_NOR_SECTOR_SIZE;
	}

	wp_ret = rts_fp_flash_soft_write_protect(dev, true);
	if (wp_ret != 0 && ret == 0) {
		ret = wp_ret;
	}

	k_sem_give(&dev_data->sem);

	return ret;
}

static const struct flash_parameters *flash_rts5817_qspi_get_parameters(const struct device *dev)
{
	static const struct flash_parameters flash_rts5817_qspi_parameters = {
		.write_block_size = 1,
		.erase_value = 0xff,
	};

	ARG_UNUSED(dev);

	return &flash_rts5817_qspi_parameters;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void flash_rts5817_qspi_pages_layout(const struct device *dev,
					    const struct flash_pages_layout **layout,
					    size_t *layout_size)
{
	const struct flash_rts5817_qspi_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_rts5817_qspi_driver_api) = {
	.read = flash_rts5817_qspi_read,
	.write = flash_rts5817_qspi_write,
	.erase = flash_rts5817_qspi_erase,
	.get_parameters = flash_rts5817_qspi_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_rts5817_qspi_pages_layout,
#endif
};

static int rts_fp_qspi_configure_clk(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;
	int ret;

	if (!device_is_ready(dev_cfg->clock_dev)) {
		return -ENODEV;
	}

	/* Already enabled by the boot ROM at init, but not necessarily after the clock was off. */
	ret = clock_control_on(dev_cfg->clock_dev, dev_cfg->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	ret = clock_control_set_rate(dev_cfg->clock_dev, dev_cfg->clock_subsys,
				     (clock_control_subsys_rate_t)QSPI_DEFAULT_CLK_FREQ);
	if (ret != 0) {
		return ret;
	}

	/* Divider 0 gives the default 60 MHz. */
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_CLK_DIVIDER);

	return 0;
}

static int flash_rts5817_qspi_init(const struct device *dev)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint32_t id_read;
	int ret;

	k_sem_init(&dev_data->sem, 1, 1);

	ret = rts_fp_qspi_configure_clk(dev);
	if (ret != 0) {
		return ret;
	}

	ret = rts_fp_read_jedec_id(dev, &id_read);
	if (ret != 0) {
		LOG_ERR("Fail to read flash id: %d", ret);
		return ret;
	}

	LOG_DBG("Flash id is 0x%x", id_read);
	if (id_read == RTS5817_FLASH_ID_NOFLASH || id_read == RTS5817_FLASH_ID_UNKNOWN) {
		return -ENXIO;
	}

	dev_data->flash = NULL;
	for (size_t i = 0; i < ARRAY_SIZE(rts5817_flash_params_table); i++) {
		if (rts5817_flash_params_table[i].jedec_id == id_read) {
			dev_data->flash = &rts5817_flash_params_table[i];
			break;
		}
	}
	if (dev_data->flash == NULL) {
		LOG_ERR("Unsupported flash id 0x%x", id_read);
		return -ENOTSUP;
	}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
	dev_data->layout.pages_size = SPI_NOR_SECTOR_SIZE;
	dev_data->layout.pages_count = dev_data->flash->flash_size / SPI_NOR_SECTOR_SIZE;
#endif

	rts_fp_qspi_init_read_mode(dev);

	if (dev_data->read_mode == RTS5817_QSPI_XFER_FAST_READ_QUAD_OUT) {
		ret = rts_fp_flash_quad_cfg(dev, true);
		if (ret != 0) {
			return ret;
		}
		rts_fp_qspi_set_auto_mode(dev);
	} else {
		/* The boot ROM enables quad read mode, so the auto mode must switch first. */
		rts_fp_qspi_set_auto_mode(dev);
		ret = rts_fp_flash_quad_cfg(dev, false);
		if (ret != 0) {
			return ret;
		}
	}

	ret = rts_fp_flash_soft_write_protect(dev, true);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("flash init finished");
	return 0;
}

#ifdef CONFIG_PM_DEVICE
/* Preset HOLD# with a pull-down, which applies once the chip enters the low power state. */
static int rts_fp_qspi_suspend(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;
	int ret;

	if (!device_is_ready(dev_cfg->syscon_sys)) {
		return -ENODEV;
	}

	ret = syscon_update_bits(dev_cfg->syscon_sys, R_AL_DUMMY0, ECO_CACHESPI_HOLD_PU_EN, 0);
	if (ret < 0) {
		return ret;
	}

	ret = syscon_update_bits(dev_cfg->syscon_sys, R_AL_DUMMY1, ECO_CACHESPI_HOLD_PD_EN,
				 ECO_CACHESPI_HOLD_PD_EN);
	if (ret < 0) {
		return ret;
	}

	ret = syscon_update_bits(dev_cfg->syscon_sys, R_AL_DUMMY0,
				 ECO_CACHESPI_HOLD_PUPD_CTRL_SWITCH,
				 ECO_CACHESPI_HOLD_PUPD_CTRL_SWITCH);
	if (ret < 0) {
		return ret;
	}
	k_busy_wait(HOLD_PUPD_SWITCH_DELAY_US);

	ret = syscon_update_bits(dev_cfg->syscon_sys, R_AL_DUMMY0,
				 ECO_CACHESPI_HOLD_PUPD_CTRL_SWITCH, 0);
	if (ret < 0) {
		return ret;
	}
	k_busy_wait(HOLD_PUPD_SETTLE_DELAY_US);

	return 0;
}

static int rts_fp_qspi_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret;

	/* A write or erase that is in progress must not be cut by a power state change. */
	if (k_sem_take(&dev_data->sem, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * Suspend only presets HOLD# for the low power state, and it goes back to its
		 * pull-up when the chip leaves it, so resume writes nothing back.
		 */
		ret = rts_fp_qspi_configure_clk(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = rts_fp_qspi_suspend(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_sem_give(&dev_data->sem);

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct flash_rts5817_qspi_config flash_rts5817_qspi_cfg = {
	.qspi_reg = DT_INST_REG_ADDR_BY_NAME(0, qspi_ctrl),
	.auto_reg = DT_INST_REG_ADDR_BY_NAME(0, auto_ctrl),
	.bus_width = DT_INST_PROP(0, bus_width),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(0, clocks, clkid),
#ifdef CONFIG_PM_DEVICE
	.syscon_sys = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon_sys)),
#endif
};

static struct flash_rts5817_qspi_data flash_rts5817_qspi_dev_data;

PM_DEVICE_DT_INST_DEFINE(0, rts_fp_qspi_pm_action);
DEVICE_DT_INST_DEFINE(0, &flash_rts5817_qspi_init, PM_DEVICE_DT_INST_GET(0),
		      &flash_rts5817_qspi_dev_data, &flash_rts5817_qspi_cfg, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_rts5817_qspi_driver_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only one qspi instance can be supported");
