/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#ifdef CONFIG_FLASH_EX_OP_ENABLED
#include <zephyr/drivers/flash/rts5817_flash_api_extensions.h>
#endif
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include "zephyr/device.h"
#include "flash_rts5817_qspi_reg.h"
#include "flash_rts5817_qspi.h"
#include "zephyr/spinlock.h"
#include <stdint.h>
#include <string.h>

#define DT_DRV_COMPAT realtek_rts5817_qspi

LOG_MODULE_REGISTER(flash_rts5817_qspi, CONFIG_FLASH_LOG_LEVEL);

#define CACHE_SPI_AUTOMODE_TRANSFER_MODE_OFFSET 8
#define CACHE_A_SPI_DUM_BIT_NUM_OFFSET          16
#define ERASE_SECTOR_SIZE                       0x1000

#define READ_BUF_ADDR_ALIGN_SIZE 32

#define WP_PIN_CFG 0x0
#define WP_PIN_O   0x4
#define WP_PIN_OE  0x8
#define WP_PIN_I   0xC
#define WP_PIN_INC 0x10

#define WP_PIN_FUNC_MASK GENMASK(9, 8)
#define WP_PIN_FUNC_WP   (0x1 << 8)
#define WP_PIN_FUNC_GPIO (0x2 << 8)

#define FLASH_PROTECT_ALL_BITS 0x1C

struct flash_rts5817_qspi_config {
	mem_addr_t qspi_reg;
	mem_addr_t auto_reg;
	mem_addr_t wp_reg;
	int bus_width;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct flash_rts5817_qspi_data {
	const spi_flash_params *flash;
	uint8_t read_mode;
	uint8_t read_cmd;
	struct k_spinlock lock;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

static inline uint8_t rts_fp_qspi_get_read_mode(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;
	uint8_t read_mode;

	if (dev_conf->bus_width == 1) {
		read_mode = SPI_FAST_READ_MODE;
	} else if (dev_conf->bus_width == 2) {
		read_mode = SPI_FAST_READ_DUAL_OUT_MODE;
	} else {
		read_mode = SPI_FAST_READ_QUAD_OUT_MODE;
	}

	return read_mode;
}

static inline uint32_t rts_fp_qspi_read_reg(const struct device *dev, uint32_t offset)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;

	return sys_read32(dev_conf->qspi_reg + offset);
}

static inline void rts_fp_qspi_write_reg(const struct device *dev, uint32_t data, uint32_t offset)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;

	sys_write32(data, dev_conf->qspi_reg + offset);
}

static inline uint32_t rts_fp_auto_mode_read_reg(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;

	return sys_read32(dev_conf->auto_reg);
}

static inline void rts_fp_auto_mode_write_reg(const struct device *dev, uint32_t data)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;

	sys_write32(data, dev_conf->auto_reg);
}

#ifdef CONFIG_FLASH_RTS5817_WP_CTRL
static inline uint32_t rts_fp_wp_read_reg(const struct device *dev, uint32_t offset)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;

	return sys_read32(dev_conf->wp_reg + offset);
}

static inline void rts_fp_wp_write_reg(const struct device *dev, uint32_t data, uint32_t offset)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;

	sys_write32(data, dev_conf->wp_reg + offset);
}

static void rts_fp_flash_wp_set(const struct device *dev, enum flash_rts5817_wp_state state)
{
	rts_fp_wp_write_reg(dev, state, WP_PIN_O);
}

static void rts_fp_flash_wp_get(const struct device *dev, enum flash_rts5817_wp_state *state)
{
	*state = rts_fp_wp_read_reg(dev, WP_PIN_O) & 0x1;
}

static void rts_fp_flash_wp_init(const struct device *dev)
{
	uint32_t reg;

	reg = rts_fp_wp_read_reg(dev, WP_PIN_CFG);

	reg &= ~(WP_PIN_FUNC_MASK);
	reg |= WP_PIN_FUNC_GPIO;

	rts_fp_wp_write_reg(dev, reg, WP_PIN_CFG);
	rts_fp_wp_write_reg(dev, 0x1, WP_PIN_INC);
	rts_fp_wp_write_reg(dev, 0x1, WP_PIN_OE);
}
#endif

static int check_transfer_end(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_conf = dev->config;
	uint32_t timeout = 5000;

	while (timeout) {
		if (sys_test_bit(dev_conf->qspi_reg + R_SPI_COM_TRANSFER, SPI_WB_IDLE_OFFSET)) {
			break;
		}
		k_busy_wait(500);
		timeout--;
	}

	if ((timeout > 0) &&
	    !sys_test_bit(dev_conf->qspi_reg + R_SPI_COM_TRANSFER, SPI_WB_TIMEOUT_OFFSET)) {
		return 0;
	}

	sys_set_bit(dev_conf->qspi_reg + R_SPI_COM_TRANSFER, SPI_WB_RST_OFFSET);

	return -ETIMEDOUT;
}

static int rts_fp_qspi_transfer(const struct device *dev, uint32_t dma_addr, size_t length,
				uint8_t cmd_nums, uint8_t dma_dir)
{
	rts_fp_qspi_write_reg(dev, dma_dir, R_SPI_WB_DMA_DIR);
	rts_fp_qspi_write_reg(dev, dma_addr, R_SPI_WB_DMA_ADDR);
	rts_fp_qspi_write_reg(dev, length, R_SPI_WB_DMA_LEN);

	rts_fp_qspi_write_reg(dev, 0xFF, R_SPI_TOP_CTL);
	rts_fp_qspi_write_reg(dev, SPI_WB_START_MASK | cmd_nums, R_SPI_COM_TRANSFER);

	return check_transfer_end(dev);
}

/* Write enable */
static void rts_fp_qspi_write_enable_using_cmd1(const struct device *dev)
{
	rts_fp_qspi_write_reg(dev, SF_WREN, R_SPI_SUB1_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB1_ADDR);
	rts_fp_qspi_write_reg(dev, SPI_C_MODE0, R_SPI_SUB1_MODE);
}

/* Polling WIP bit in status reg */
static void rts_fp_qspi_polling_status_using_cmd3(const struct device *dev)
{
	rts_fp_qspi_write_reg(dev, SF_RDSR, R_SPI_SUB3_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB3_ADDR);
	rts_fp_qspi_write_reg(dev, SPI_POLLING_MODE0, R_SPI_SUB3_MODE);
}

static int rts_fp_qspi_read_align(const struct device *dev, uint8_t read_mode, uint8_t read_cmd,
				  off_t addr, size_t len, uint8_t *data)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret;
	k_spinlock_key_t key;

	/* Check alignment */
	if ((uint32_t)data & (READ_BUF_ADDR_ALIGN_SIZE - 1)) {
		LOG_ERR("Read buffer address is not aligned to %d", READ_BUF_ADDR_ALIGN_SIZE);
		return -EINVAL;
	}

	key = k_spin_lock(&dev_data->lock);
	sys_cache_data_flush_and_invd_range(data, len);

	rts_fp_qspi_write_reg(dev, read_cmd, R_SPI_SUB1_COMMAND);
	rts_fp_qspi_write_reg(dev, read_mode, R_SPI_SUB1_MODE);
	rts_fp_qspi_write_reg(dev, addr, R_SPI_SUB1_ADDR);
	rts_fp_qspi_write_reg(dev, len, R_SPI_SUB1_LENGTH);
	ret = rts_fp_qspi_transfer(dev, (uint32_t)data, len, SPI_COM_TRANSFER_NUM_1, SPI_TO_DTCM);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static int rts_fp_read_jedec_id(const struct device *dev, uint8_t *id)
{
	uint8_t temp[3] __aligned(READ_BUF_ADDR_ALIGN_SIZE);
	int ret;

	if (id == NULL) {
		return -EINVAL;
	}

	ret = rts_fp_qspi_read_align(dev, SPI_CDI_MODE0, SF_READID, 0, 3, temp);
	if (ret) {
		return ret;
	}

	memcpy(id, temp, 3);

	return ret;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int rts_fp_flash_read_uid(const struct device *dev, uint8_t *uid, size_t len)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t temp[16] __aligned(READ_BUF_ADDR_ALIGN_SIZE);
	int ret;

	if (uid == NULL) {
		return -EINVAL;
	}

	if (len > 16) {
		return -EINVAL;
	}

	switch ((device_id_t)(dev_data->flash->device_id)) {
	case ZG25VQ16D:
	case ZG25VQ32D:
	case GD25Q16:
	case GD25Q32:
	case W25Q16JVSIQ:
	case W25Q32JVSIQ:
	case P25Q16SH:
	case P25Q32SH:
	case GD25Q80E:
		ret = rts_fp_qspi_read_align(dev, SPI_FAST_READ_MODE, SF_READ_UID, 0, len, temp);
		if (ret) {
			return ret;
		}
		break;
	case MX25R8035:
		ret = rts_fp_qspi_read_align(dev, SPI_FAST_READ_MODE, SF_READ_UID_MXIC, 0x1E0, len,
					     temp);
		if (ret) {
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	memcpy(uid, temp, len);

	return ret;
}
#endif

static int rts_fp_qspi_write_status(const struct device *dev, uint8_t cmd, uint8_t *value,
				    uint8_t len)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;

	sys_cache_data_flush_range(value, len);
	/* Write enable */
	rts_fp_qspi_write_reg(dev, dev_data->flash->sr_wren_command, R_SPI_SUB1_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB1_ADDR);
	rts_fp_qspi_write_reg(dev, SPI_C_MODE0, R_SPI_SUB1_MODE);

	/* Write status */
	rts_fp_qspi_write_reg(dev, cmd, R_SPI_SUB2_COMMAND);
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_SUB2_ADDR);
	rts_fp_qspi_write_reg(dev, len, R_SPI_SUB2_LENGTH);
	rts_fp_qspi_write_reg(dev, SPI_CDO_MODE0, R_SPI_SUB2_MODE);

	/* Polling WIP bit in status reg */
	rts_fp_qspi_polling_status_using_cmd3(dev);
	return rts_fp_qspi_transfer(dev, (uint32_t)value, len, SPI_COM_TRANSFER_NUM_3, DTCM_TO_SPI);
}

static int rts_fp_flash_read_status(const struct device *dev, uint8_t *value)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t temp[2] __aligned(READ_BUF_ADDR_ALIGN_SIZE);
	int ret;

	switch (dev_data->flash->str_rw_type) {
	case SREG1_R1_W1:
		ret = rts_fp_qspi_read_align(dev, SPI_CDI_MODE0, SF_RDSR, 0, 1, temp);
		value[0] = temp[0];
		break;
	case SREG2_R2_W1:
	case SREG2_R2_W2:
		ret = rts_fp_qspi_read_align(dev, SPI_CDI_MODE0, SF_RDSR, 0, 1, temp);
		if (ret) {
			return ret;
		}
		value[0] = temp[0];
		ret = rts_fp_qspi_read_align(dev, SPI_CDI_MODE0, SF_RDSR_H, 0, 1, temp);
		if (ret) {
			return ret;
		}

		value[1] = temp[0];
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int rts_fp_flash_write_status(const struct device *dev, uint8_t *value)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret;

	switch (dev_data->flash->str_rw_type) {
	case SREG1_R1_W1:
		ret = rts_fp_qspi_write_status(dev, SF_WRSR, value, 1);
		break;
	case SREG2_R2_W1:
		ret = rts_fp_qspi_write_status(dev, SF_WRSR, value, 2);
		break;
	case SREG2_R2_W2:
		ret = rts_fp_qspi_write_status(dev, SF_WRSR, value, 1);
		if (ret) {
			return ret;
		}
		ret = rts_fp_qspi_write_status(dev, SF_WRSR_H, &value[1], 1);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

static int rts_fp_flash_soft_write_protect(const struct device *dev, bool enable)
{
	uint8_t status_reg[2] = {0, 0};
	int ret;

	ret = rts_fp_flash_read_status(dev, status_reg);
	if (ret) {
		return ret;
	}

	if (enable) {
		if ((status_reg[0] & FLASH_PROTECT_ALL_BITS) == FLASH_PROTECT_ALL_BITS) {
			return 0;
		}
		status_reg[0] |= FLASH_PROTECT_ALL_BITS;
	} else {
		if ((status_reg[0] & FLASH_PROTECT_ALL_BITS) == 0) {
			return 0;
		}
		status_reg[0] &= ~FLASH_PROTECT_ALL_BITS;
	}

	return rts_fp_flash_write_status(dev, status_reg);
}

static int rts_fp_flash_quad_cfg(const struct device *dev, bool enable)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t status_reg[2] = {0};
	int ret;

	ret = rts_fp_flash_read_status(dev, status_reg);
	if (ret) {
		return ret;
	}

	if (enable) {
		if (dev_data->flash->location_qe < 8) {
			status_reg[0] |= 1 << dev_data->flash->location_qe;
		} else {
			status_reg[1] |= 1 << (dev_data->flash->location_qe - 8);
		}
	} else {
		if (dev_data->flash->location_qe < 8) {
			status_reg[0] &= ~(1 << dev_data->flash->location_qe);
		} else {
			status_reg[1] &= ~(1 << (dev_data->flash->location_qe - 8));
		}
	}

	return rts_fp_flash_write_status(dev, status_reg);
}

static uint8_t rts_fp_flash_get_read_command(const struct device *dev)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t read_cmd;

	switch (dev_data->read_mode) {
	case SPI_FAST_READ_MODE:
		read_cmd = SF_FAST_READ;
		break;
	case SPI_FAST_READ_DUAL_OUT_MODE:
		read_cmd = SF_FAST_READ_DUAL_OUT;
		break;
	case SPI_FAST_READ_QUAD_OUT_MODE:
		read_cmd = SF_FAST_READ_QUAD_OUT;
		break;
	default:
		read_cmd = SF_FAST_READ_DUAL_OUT;
		break;
	}

	return read_cmd;
}

static void __attribute((aligned(32), noinline)) set_qspi_auto_mode(const struct device *dev,
								    uint32_t auto_mode)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint32_t reg;

	reg = rts_fp_qspi_read_reg(dev, R_SPI_TCTL);
	rts_fp_qspi_write_reg(dev,
			      (reg & ~SPI_U_DUM_BIT_NUM_MASK) |
				      (dev_data->flash->dummy_cycle << SPI_U_DUM_BIT_NUM_OFFSET),
			      R_SPI_TCTL);
	rts_fp_auto_mode_write_reg(dev, auto_mode);
}

static void rts_fp_qspi_set_auto_mode(const struct device *dev)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint32_t auto_mode = 0;

	auto_mode |= dev_data->flash->dummy_cycle << CACHE_A_SPI_DUM_BIT_NUM_OFFSET;
	auto_mode |= dev_data->read_mode << CACHE_SPI_AUTOMODE_TRANSFER_MODE_OFFSET;
	auto_mode |= dev_data->read_cmd;

	set_qspi_auto_mode(dev, auto_mode);

	LOG_DBG("Auto mode reg:%x", rts_fp_auto_mode_read_reg(dev));
}

static int flash_rts5817_qspi_read(const struct device *dev, off_t addr, void *data, size_t size)
{
	const struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint8_t temp_buf[READ_BUF_ADDR_ALIGN_SIZE] __aligned(READ_BUF_ADDR_ALIGN_SIZE);
	uint16_t temp_len;
	int ret;

	temp_len = READ_BUF_ADDR_ALIGN_SIZE - ((uint32_t)data % READ_BUF_ADDR_ALIGN_SIZE);
	if (temp_len != READ_BUF_ADDR_ALIGN_SIZE) {
		if (size < temp_len) {
			temp_len = size;
		}
		ret = rts_fp_qspi_read_align(dev, dev_data->read_mode, dev_data->read_cmd, addr,
					     temp_len, temp_buf);
		if (ret) {
			return ret;
		}
		memcpy(data, temp_buf, temp_len);
		size -= temp_len;
		addr += temp_len;
		data = (uint8_t *)data + temp_len;
	}

	temp_len = (size >> 5) << 5;
	if (temp_len) {
		ret = rts_fp_qspi_read_align(dev, dev_data->read_mode, dev_data->read_cmd, addr,
					     temp_len, data);
		if (ret) {
			return ret;
		}
		size -= temp_len;
		addr += temp_len;
		data = (uint8_t *)data + temp_len;
	}

	if (size) {
		ret = rts_fp_qspi_read_align(dev, dev_data->read_mode, dev_data->read_cmd, addr,
					     size, temp_buf);
		if (ret) {
			return ret;
		}
		memcpy(data, temp_buf, size);
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

	/* Page program */
	rts_fp_qspi_write_reg(dev, SF_PAGE_PROG, R_SPI_SUB2_COMMAND);
	rts_fp_qspi_write_reg(dev, offset, R_SPI_SUB2_ADDR);
	rts_fp_qspi_write_reg(dev, len, R_SPI_SUB2_LENGTH);
	rts_fp_qspi_write_reg(dev, SPI_CADO_MODE0, R_SPI_SUB2_MODE);

	rts_fp_qspi_polling_status_using_cmd3(dev);
	ret = rts_fp_qspi_transfer(dev, (uint32_t)data, len, SPI_COM_TRANSFER_NUM_3, DTCM_TO_SPI);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static int flash_rts5817_qspi_write(const struct device *dev, off_t offset, const void *data,
				    size_t len)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	uint16_t page_len;
	int ret = 0;

	if (offset + len > dev_data->flash->flash_size) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_FLASH_RTS5817_WRITE_PROTECTION_AUTO)) {
		ret = rts_fp_flash_soft_write_protect(dev, false);
		if (ret) {
			return ret;
		}
	}

	while (len) {
		page_len = 256 - (offset & 0xFF);
		if (len < page_len) {
			page_len = len;
		}

		ret = rts_fp_qspi_page_program(dev, offset, page_len, data);
		if (ret) {
			LOG_ERR("page program err %d", ret);
			return ret;
		}
		data = (const uint8_t *)data + page_len;
		offset += page_len;
		len -= page_len;
	}

	if (IS_ENABLED(CONFIG_FLASH_RTS5817_WRITE_PROTECTION_AUTO)) {
		ret = rts_fp_flash_soft_write_protect(dev, true);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

static int rts_fp_qspi_sector_erase(const struct device *dev, off_t offset)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&dev_data->lock);

	/* Write enable */
	rts_fp_qspi_write_enable_using_cmd1(dev);

	rts_fp_qspi_write_reg(dev, SF_SECT_ERASE, R_SPI_SUB2_COMMAND);
	rts_fp_qspi_write_reg(dev, offset, R_SPI_SUB2_ADDR);
	rts_fp_qspi_write_reg(dev, SPI_CA_MODE0, R_SPI_SUB2_MODE);

	rts_fp_qspi_polling_status_using_cmd3(dev);

	ret = rts_fp_qspi_transfer(dev, 0, 0, SPI_COM_TRANSFER_NUM_3, SPI_TO_DTCM);

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

static int flash_rts5817_qspi_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	int ret = 0;

	if (offset < 0 || offset + size > dev_data->flash->flash_size) {
		return -EINVAL;
	}

	if (size % ERASE_SECTOR_SIZE) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_FLASH_RTS5817_WRITE_PROTECTION_AUTO)) {
		ret = rts_fp_flash_soft_write_protect(dev, false);
		if (ret) {
			return ret;
		}
	}

	while (size) {
		ret = rts_fp_qspi_sector_erase(dev, offset);
		if (ret) {
			return ret;
		}
		offset += ERASE_SECTOR_SIZE;
		size -= ERASE_SECTOR_SIZE;
	}

	if (IS_ENABLED(CONFIG_FLASH_RTS5817_WRITE_PROTECTION_AUTO)) {
		ret = rts_fp_flash_soft_write_protect(dev, true);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

static const struct flash_parameters flash_rts5817_qspi_parameters = {.write_block_size = 1,
								      .erase_value = 0xff};

static const struct flash_parameters *flash_rts5817_qspi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_rts5817_qspi_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_rts5817_qspi_pages_layout(const struct device *dev,
					    const struct flash_pages_layout **layout,
					    size_t *layout_size)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_rts5817_ex_op_write_status(const struct device *dev, uint8_t *sr)
{
	struct flash_rts5817_qspi_data *dev_data = dev->data;

	/* Note: QE bit should not be clear if auto read mode is quad */
	if (dev_data->read_mode == SF_FAST_READ_QUAD_OUT) {
		if (dev_data->flash->location_qe < 8) {
			sr[0] |= 1 << dev_data->flash->location_qe;
		} else {
			sr[1] |= 1 << (dev_data->flash->location_qe - 8);
		}
	}

	return rts_fp_flash_write_status(dev, sr);
}

static int flash_rts5817_ex_op_read_status(const struct device *dev, uint8_t *sr)
{
	return rts_fp_flash_read_status(dev, sr);
}

static int flash_rts5817_ex_op_read_uid(const struct device *dev, uint8_t *uid, size_t len)
{
	return rts_fp_flash_read_uid(dev, uid, len);
}

static int flash_rts5817_ex_op_wp_pin_set(const struct device *dev,
					  enum flash_rts5817_wp_state wp_state)
{
	int ret = 0;
#ifdef CONFIG_FLASH_RTS5817_WP_CTRL
	rts_fp_flash_wp_set(dev, wp_state);
#else
	ret = -ENOTSUP;
#endif
	return ret;
}

static int flash_rts5817_ex_op_wp_pin_get(const struct device *dev,
					  enum flash_rts5817_wp_state *wp_state)
{
	int ret = 0;
#ifdef CONFIG_FLASH_RTS5817_WP_CTRL
	rts_fp_flash_wp_get(dev, wp_state);
#else
	ret = -ENOTSUP;
#endif
	return ret;
}

static int flash_rts5817_ex_op(const struct device *dev, uint16_t opcode, const uintptr_t in,
			       void *out)
{
	int ret = -EINVAL;

	switch (opcode) {
	case FLASH_RTS5817_EX_OP_WR_SR:
		ret = flash_rts5817_ex_op_write_status(dev, (uint8_t *)in);
		break;
	case FLASH_RTS5817_EX_OP_RD_SR:
		ret = flash_rts5817_ex_op_read_status(dev, (uint8_t *)out);
		break;
	case FLASH_RTS5817_EX_OP_RD_UID:
		ret = flash_rts5817_ex_op_read_uid(dev, (uint8_t *)out, *(uint8_t *)in);
		break;
	case FLASH_RTS5817_EX_OP_SOFT_PROTECT:
		ret = rts_fp_flash_soft_write_protect(dev, *(bool *)in);
		break;
	case FLASH_RTS5817_EX_OP_WP_PIN_SET:
		ret = flash_rts5817_ex_op_wp_pin_set(dev, *(enum flash_rts5817_wp_state *)in);
		break;
	case FLASH_RTS5817_EX_OP_WP_PIN_GET:
		ret = flash_rts5817_ex_op_wp_pin_get(dev, (enum flash_rts5817_wp_state *)out);
		break;
	default:
		break;
	}

	return ret;
}
#endif

static DEVICE_API(flash, flash_rts5817_qspi_driver_api) = {
	.read = flash_rts5817_qspi_read,
	.write = flash_rts5817_qspi_write,
	.erase = flash_rts5817_qspi_erase,
	.get_parameters = flash_rts5817_qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_rts5817_qspi_pages_layout,
#endif
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_rts5817_ex_op,
#endif
};

static int flash_rts5817_qspi_init(const struct device *dev)
{
	const struct flash_rts5817_qspi_config *dev_cfg = dev->config;
	struct flash_rts5817_qspi_data *dev_data = dev->data;
	const spi_flash_params *sf_table = NULL;
	clock_control_subsys_rate_t rate = (clock_control_subsys_rate_t)MHZ(120);
	uint8_t jedec_id[3];
	uint32_t id_read;
	uint8_t i;
	int ret;

	/* clock */
	if (clock_control_set_rate(dev_cfg->clock_dev, dev_cfg->clock_subsys, rate) != 0) {
		ret = -EINVAL;
		return ret;
	}

	/* Divider is set to 0, so default ck is 60MHz */
	rts_fp_qspi_write_reg(dev, 0x0, R_SPI_CLK_DIVIDER);

	LOG_DBG("flash init clock finished");

	/* Check flash JEDEC ID */
	sf_table = spi_flash_params_table;

	/* read flash id */
	ret = rts_fp_read_jedec_id(dev, jedec_id);
	if (ret) {
		LOG_ERR("%s: %d err %d\n", __func__, __LINE__, ret);
		return ret;
	}

	id_read = (jedec_id[0] << 16 | jedec_id[1] << 8 | jedec_id[2]);
	LOG_DBG("%s: %d flash id  %x\n", __func__, __LINE__, id_read);
	if (id_read == NOFLASH) {
		return -ENXIO;
	}

	for (i = 0; i < SUPPORT_FLASH_NUM; i++) {
		if (sf_table[i].device_id == id_read) {
			break;
		}
	}
	dev_data->flash = &sf_table[i];
	dev_data->layout.pages_size = ERASE_SECTOR_SIZE; /* sector erase 4K */
	dev_data->layout.pages_count = dev_data->flash->flash_size / ERASE_SECTOR_SIZE;

	if (IS_ENABLED(CONFIG_FLASH_RTS5817_WRITE_PROTECTION_AUTO)) {
		ret = rts_fp_flash_soft_write_protect(dev, true);
		if (ret) {
			return ret;
		}
	}

	dev_data->read_mode = rts_fp_qspi_get_read_mode(dev);
	dev_data->read_cmd = rts_fp_flash_get_read_command(dev);

	if (dev_data->read_mode == SPI_FAST_READ_QUAD_OUT_MODE) {
		rts_fp_flash_quad_cfg(dev, true);
		rts_fp_qspi_set_auto_mode(dev);
	} else {
		/* Note: quad read mode is enabled in ROM by default,
		 * so it needs to configuring auto read mode to dual
		 * or single mode before clear QE bit of flash.
		 */
		rts_fp_qspi_set_auto_mode(dev);
		rts_fp_flash_quad_cfg(dev, false);
	}

#ifdef CONFIG_FLASH_RTS5817_WP_CTRL
	rts_fp_flash_wp_init(dev);
#endif
	LOG_DBG("flash init finished");
	return ret;
}

static const struct flash_rts5817_qspi_config flash_rts5817_qspi_cfg = {
	.qspi_reg = DT_INST_REG_ADDR_BY_NAME(0, qspi_ctrl),
	.auto_reg = DT_INST_REG_ADDR_BY_NAME(0, auto_ctrl),
	.wp_reg = DT_INST_REG_ADDR_BY_NAME(0, wp_ctrl),
	.bus_width = DT_INST_PROP(0, bus_width),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(0, clocks, clkid),
};

static struct flash_rts5817_qspi_data flash_rts5817_qspi_dev_data;

DEVICE_DT_INST_DEFINE(0, &flash_rts5817_qspi_init, NULL, &flash_rts5817_qspi_dev_data,
		      &flash_rts5817_qspi_cfg, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &flash_rts5817_qspi_driver_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only one qspi instance can be supported");
