/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define DT_DRV_COMPAT renesas_ra_qspi_nor

#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/ra_flash_api_extensions.h>
#include "spi_nor.h"
#include "r_spi_flash_api.h"
#include "r_qspi.h"

/* Flash QPI (4-4-4) opcodes */
#define QSPI_QPI_CMD_QPIID  (0xAF) /* QPI ID Read */
#define QSPI_QPI_CMD_RDSFDP (0x5A) /* Read SFDP */
#define QSPI_QPI_CMD_RSTQIO (0xF5) /* Reset QPI */
#define QSPI_QPI_CMD_EQIO   (0x35) /* Enable QPI */

/* XIP (Execute In Place) mode */
#define QSPI_CMD_XIP_ENTER (0x20) /* XIP Enter command */
#define QSPI_CMD_XIP_EXIT  (0xFF) /* XIP Exit command */

#define WRITE_STATUS_BIT 0

#if defined(CONFIG_SOC_SERIES_RA6E2)
#define STATUS_REG_PAYLOAD {0x01, 0x00}
#define SET_SREG_VALUE     (0x00)
#else
#define STATUS_REG_PAYLOAD {0x01, 0x40, 0x00}
#define SET_SREG_VALUE     (0x40)
#endif
/* one byte data transfer */
#define ONE_BYTE             (1)
#define THREE_BYTE           (3)
#define FOUR_BYTE            (4)
#define RESET_VALUE          (0x00)
/* default memory value */
#define QSPI_DEFAULT_MEM_VAL (0xFF)
#define QSPI0_NODE           DT_INST_PARENT(0)
#define RA_QSPI_NOR_NODE     DT_INST(0, renesas_ra_qspi_nor)
#define QSPI_WRITE_BLK_SZ    DT_PROP(RA_QSPI_NOR_NODE, write_block_size)
#define QSPI_ERASE_BLK_SZ    DT_PROP(RA_QSPI_NOR_NODE, erase_block_size)
/* QSPI flash page Size */
#define PAGE_SIZE_BYTE       SPI_NOR_PAGE_SIZE
/* sector size of QSPI flash device */
#define BLOCK_SIZE_4K        (4096U)
#define BLOCK_SIZE_32K       (32768U)
#define BLOCK_SIZE_64K       (65536U)

/* Flash Size*/
#define QSPI_NOR_FLASH_SIZE DT_REG_SIZE(RA_QSPI_NOR_NODE)

#define ERASE_COMMAND_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#define QSPI_ENABLE_QUAD_MODE DT_PROP(RA_QSPI_NOR_NODE, qpi_enable)

PINCTRL_DT_DEFINE(QSPI0_NODE);
LOG_MODULE_REGISTER(flash_qspi_renesas_ra, CONFIG_FLASH_LOG_LEVEL);

struct qspi_flash_ra_data {
	struct st_qspi_instance_ctrl qspi_ctrl;
	struct st_spi_flash_cfg qspi_cfg;
	struct k_sem sem;
};

struct ra_qspi_nor_flash_config {
	const struct pinctrl_dev_config *pcfg;
};

static const spi_flash_erase_command_t g_qspi_erase_command_list[4] = {
	{.command = 0x20, .size = 4096},
	{.command = 0x52, .size = 32768},
	{.command = 0xD8, .size = 65536},
	{.command = 0xC7, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE},
};

static const struct ra_qspi_nor_flash_config qspi_nor_dev_config = {
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(QSPI0_NODE),
};

static const struct flash_parameters qspi_flash_ra_config_para = {
	.write_block_size = QSPI_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

static const qspi_extended_cfg_t g_qspi_extended_cfg = {
	.min_qssl_deselect_cycles = QSPI_QSSL_MIN_HIGH_LEVEL_8_QSPCLK,
	.qspclk_div = QSPI_QSPCLK_DIV_2,
};

static struct qspi_flash_ra_data qspi_flash_data = {
	.qspi_cfg = {
		.spi_protocol = SPI_FLASH_PROTOCOL_EXTENDED_SPI,
		.read_mode = SPI_FLASH_READ_MODE_FAST_READ_QUAD_IO,
		.address_bytes = SPI_FLASH_ADDRESS_BYTES_3,
		.dummy_clocks = SPI_FLASH_DUMMY_CLOCKS_DEFAULT,
		.page_program_address_lines = SPI_FLASH_DATA_LINES_1,
		.page_size_bytes = PAGE_SIZE_BYTE,
		.page_program_command = (SPI_NOR_CMD_PP),
		.write_enable_command = (SPI_NOR_CMD_WREN),
		.status_command = (SPI_NOR_CMD_RDSR),
		.write_status_bit = WRITE_STATUS_BIT,
		.xip_enter_command = QSPI_CMD_XIP_ENTER,
		.xip_exit_command = QSPI_CMD_XIP_EXIT,
		.p_erase_command_list = &g_qspi_erase_command_list[0],
		.erase_command_list_length = ERASE_COMMAND_LENGTH(g_qspi_erase_command_list),
		.p_extend = &g_qspi_extended_cfg,
	}};

static void acquire_device(const struct device *dev)
{
	struct qspi_flash_ra_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static void release_device(const struct device *dev)
{
	struct qspi_flash_ra_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}
static int get_flash_status(const struct device *dev)
{
	struct qspi_flash_ra_data *qspi_data = dev->data;
	spi_flash_status_t status = {.write_in_progress = true};
	int32_t time_out = (INT32_MAX);
	int err;

	do {
		err = R_QSPI_StatusGet(&qspi_data->qspi_ctrl, &status);
		if (err != FSP_SUCCESS) {
			LOG_ERR("Status get failed");
			return -EIO;
		}
		--time_out;
		if (RESET_VALUE >= time_out) {
			return -EIO;
		}
	} while (false != status.write_in_progress);

	return 0;
}

#if defined(CONFIG_FLASH_EX_OP_ENABLED)
static int qspi_flash_ra_ex_op(const struct device *dev, uint16_t code, const uintptr_t in,
			       void *out)
{
	int err = 0;
	uint8_t cmd;
	struct qspi_flash_ra_data *qspi_data = dev->data;

	ARG_UNUSED(in);
	ARG_UNUSED(out);
	acquire_device(dev);

	switch (code) {
	case QSPI_FLASH_EX_OP_EXIT_QPI:
		if (SPI_FLASH_PROTOCOL_QPI != qspi_data->qspi_cfg.spi_protocol) {
			err = 0;
			break;
		}
		cmd = QSPI_QPI_CMD_RSTQIO;
		err = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &cmd, ONE_BYTE, false);
		if (err != FSP_SUCCESS) {
			LOG_ERR("Direct write for EXIT QPI failed");
			err = -EIO;
		}
		break;

	case FLASH_EX_OP_RESET:
		cmd = SPI_NOR_CMD_RESET_EN;
		err = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &cmd, ONE_BYTE, false);
		if (err == FSP_SUCCESS) {
			cmd = SPI_NOR_CMD_RESET_MEM;
			err = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &cmd, ONE_BYTE, false);
			if (err != FSP_SUCCESS) {
				LOG_ERR("Direct write for RESET MEM failed");
				err = -EIO;
			}
		} else {
			if (err != FSP_SUCCESS) {
				LOG_ERR("Direct write for RESET Flash failed");
				err = -EIO;
			}
		}
		break;
	default:
		break;
	}
	release_device(dev);

	return err;
}
#endif

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout qspi_flash_ra_layout = {
	.pages_count = QSPI_NOR_FLASH_SIZE / QSPI_ERASE_BLK_SZ,
	.pages_size = QSPI_ERASE_BLK_SZ,
};

void qspi_flash_ra_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			       size_t *layout_size)
{
	ARG_UNUSED(dev);
	*layout = &qspi_flash_ra_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API)
static int qspi_flash_ra_read_jedec_id(const struct device *dev, uint8_t *id)
{
	struct qspi_flash_ra_data *qspi_data = dev->data;
	int err = 0;
	uint8_t cmd;

	if (id == NULL) {
		return -EINVAL;
	}
	acquire_device(dev);

	if (SPI_FLASH_PROTOCOL_QPI == qspi_data->qspi_cfg.spi_protocol) {
		cmd = QSPI_QPI_CMD_QPIID;
	} else {
		cmd = SPI_NOR_CMD_RDID;
	}
	err = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &cmd, ONE_BYTE, true);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Direct write for READ ID failed");
		err = -EIO;
		goto out;
	}

	err = R_QSPI_DirectRead(&qspi_data->qspi_ctrl, id, THREE_BYTE);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Direct read failed");
		err = -EIO;
		goto out;
	}

	err = get_flash_status(dev);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to get status for QSPI operation");
		err = -EIO;
	}
out:
	release_device(dev);

	return err;
}

static int qspi_flash_ra_sfdp_read(const struct device *dev, off_t addr, void *data, size_t size)
{
	struct qspi_flash_ra_data *qspi_data = dev->data;
	int err = 0;
	uint8_t offset;

	SPI_FLASH_PROTOCOL_QPI == qspi_data->qspi_cfg.spi_protocol ? (offset = 4) : (offset = 1);
	uint8_t *buffer = k_malloc((size + offset) > 4 ? (size + offset) : 4);

	if (!buffer) {
		LOG_ERR("Failed to allocate buffer for SFDP read");
		return -ENOMEM;
	}

	acquire_device(dev);
	memset(&buffer[0], 0, sizeof(buffer));
	buffer[0] = QSPI_QPI_CMD_RDSFDP;
	buffer[1] = addr;
	buffer[2] = addr >> 8;
	buffer[3] = addr >> 16;

	err = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &buffer[0], FOUR_BYTE, true);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Direct write for READ SFDP failed");
		err = -EIO;
		goto out;
	}

	err = R_QSPI_DirectRead(&qspi_data->qspi_ctrl, &buffer[0], size + offset);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Direct read failed");
		err = -EIO;
		goto out;
	}

	err = get_flash_status(dev);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to get status for QSPI operation");
		err = -EIO;
		goto out;
	}

	if (SPI_FLASH_PROTOCOL_QPI == qspi_data->qspi_cfg.spi_protocol) {
		/* 3 dummy byte */
		memcpy(data, &buffer[4], size);
	} else {
		/* 1 dummy byte */
		memcpy(data, &buffer[1], size);
	}
out:
	release_device(dev);
	return err;
}
#endif

static bool qspi_flash_ra_valid(off_t area_size, off_t offset, size_t len)
{
	if ((offset < 0) || (offset >= area_size) || ((area_size - offset) < len)) {
		return false;
	}

	return true;
}

static int qspi_flash_ra_erase(const struct device *dev, off_t offset, size_t len)
{
	struct qspi_flash_ra_data *qspi_data = dev->data;
	int err = 0;
	struct flash_pages_info page_info_start, page_info_end;
	uint32_t erase_size;
	int rc;

	if (!len) {
		return 0;
	}

	if (!qspi_flash_ra_valid(QSPI_NOR_FLASH_SIZE, offset, len)) {
		LOG_ERR("The offset 0x%lx is invalid", (long)offset);
		return -EINVAL;
	}

	if (len % QSPI_ERASE_BLK_SZ != 0) {
		LOG_ERR("The size %u is not align with block size (%u)", len, QSPI_ERASE_BLK_SZ);
		return -EINVAL;
	}

	rc = flash_get_page_info_by_offs(dev, offset, &page_info_start);
	if ((rc != 0) || (offset != page_info_start.start_offset)) {
		LOG_ERR("The offset 0x%lx is not aligned with the starting sector", (long)offset);
		return -EINVAL;
	}

	rc = flash_get_page_info_by_offs(dev, (offset + len), &page_info_end);
	if ((rc != 0) || ((offset + len) != page_info_end.start_offset)) {
		LOG_ERR("The size %u is not aligned with the ending sector", len);
		return -EINVAL;
	}

	acquire_device(dev);
	while (len > 0) {

		if (len < BLOCK_SIZE_32K) {
			erase_size = BLOCK_SIZE_4K;
		} else if (len < BLOCK_SIZE_64K) {
			erase_size = BLOCK_SIZE_32K;
		} else {
			erase_size = BLOCK_SIZE_64K;
		}
		err = R_QSPI_Erase(&qspi_data->qspi_ctrl,
				   (uint8_t *)(QSPI_DEVICE_START_ADDRESS + offset), erase_size);
		if (err) {
			LOG_ERR("Erase failed");
			err = -EIO;
			break;
		}

		err = get_flash_status(dev);
		if (err) {
			LOG_ERR("failed to get status for QSPI operation");
			err = -EIO;
			break;
		}

		offset += erase_size;
		len -= erase_size;
	}
	release_device(dev);

	return err;
}

static int qspi_flash_ra_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	if (!len) {
		return 0;
	}

	if (!qspi_flash_ra_valid(QSPI_NOR_FLASH_SIZE, offset, len)) {
		return -EINVAL;
	}

	acquire_device(dev);

	memcpy(data, (uint8_t *)(QSPI_DEVICE_START_ADDRESS + offset), len);
	release_device(dev);

	return 0;
}

static int qspi_flash_ra_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct qspi_flash_ra_data *qspi_data = dev->data;
	int err = 0;
	uint32_t remaining_bytes = len;
	const uint8_t *p_data = data;
	uint32_t size = len;

	if (!len) {
		return 0;
	}

	if (!qspi_flash_ra_valid(QSPI_NOR_FLASH_SIZE, offset, len)) {
		return -EINVAL;
	}

	acquire_device(dev);
	while (remaining_bytes > 0) {
		size = remaining_bytes > PAGE_SIZE_BYTE ? PAGE_SIZE_BYTE : remaining_bytes;
		err = R_QSPI_Write(&qspi_data->qspi_ctrl, p_data,
				   (uint8_t *)(QSPI_DEVICE_START_ADDRESS + offset), size);
		if (err) {
			LOG_ERR("Direct write failed");
			err = -EIO;
			break;
		}

		err = get_flash_status(dev);
		if (err) {
			LOG_ERR("Failed to get status for QSPI operation");
			err = -EIO;
			break;
		}

		remaining_bytes -= size;
		offset += size;
		p_data += size;
	}
	release_device(dev);
	return err;
}

static int qspi_flash_ra_get_size(const struct device *dev, uint64_t *size)
{
	*size = (uint64_t)QSPI_NOR_FLASH_SIZE;

	return 0;
}

static const struct flash_parameters *qspi_flash_ra_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &qspi_flash_ra_config_para;
}
static const struct flash_driver_api qspi_flash_ra_api = {
	.erase = qspi_flash_ra_erase,
	.write = qspi_flash_ra_write,
	.read = qspi_flash_ra_read,
	.get_parameters = qspi_flash_ra_get_parameters,
	.get_size = qspi_flash_ra_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = qspi_flash_ra_page_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = qspi_flash_ra_sfdp_read,
	.read_jedec_id = qspi_flash_ra_read_jedec_id,
#endif
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	.ex_op = qspi_flash_ra_ex_op,
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
};

static int set_qspi_flash_status(const struct device *dev)
{
	struct qspi_flash_ra_data *qspi_data = dev->data;
	uint8_t data_sreg[] = STATUS_REG_PAYLOAD;
	uint8_t sreg_data = 0;
	int ret;

	ret = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, data_sreg, sizeof(data_sreg), false);
	if (ret) {
		LOG_ERR("Direct write for STATUS_REG_PAYLOAD fail");
		return -EIO;
	}

	ret = get_flash_status(dev);
	if (ret) {
		LOG_ERR("Failed to get status for QSPI operation");
		return -EIO;
	}
	ret = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &(qspi_data->qspi_cfg.status_command),
				 ONE_BYTE, true);
	if (ret) {
		LOG_ERR("Direct write for status command fail ");
		return -EIO;
	}

	ret = R_QSPI_DirectRead(&qspi_data->qspi_ctrl, &sreg_data, ONE_BYTE);
	if (ret) {
		LOG_ERR("Direct read fail");
		return -EIO;
	}

	if (SET_SREG_VALUE != sreg_data) {
		LOG_ERR("Verify status register data failed");
		return -EIO;
	}
	return ret;
}

static int qspi_flash_ra_init(const struct device *dev)
{
	const struct ra_qspi_nor_flash_config *config = dev->config;
	struct qspi_flash_ra_data *qspi_data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to configure pins for QSPI");
		return -EIO;
	}
	k_sem_init(&qspi_data->sem, 1, 1);

	ret = R_QSPI_Open(&qspi_data->qspi_ctrl, &qspi_data->qspi_cfg);
	if (ret) {
		LOG_ERR("Open failed");
		return -EIO;
	}

	ret = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &(qspi_data->qspi_cfg.write_enable_command),
				 ONE_BYTE, false);
	if (ret) {
		LOG_ERR("Direct write enable command failed");
		return -EIO;
	}

	ret = get_flash_status(dev);
	if (ret) {
		LOG_ERR("Failed to get status for QSPI operation");
		return -EIO;
	}

	ret = set_qspi_flash_status(dev);
	if (ret) {
		LOG_ERR("Set qspi flash status failed");
		return -EIO;
	}
#if QSPI_ENABLE_QUAD_MODE
	uint8_t data_qpi_en = QSPI_QPI_CMD_EQIO;

	qspi_data->qspi_cfg.spi_protocol = SPI_FLASH_PROTOCOL_QPI;
	ret = R_QSPI_DirectWrite(&qspi_data->qspi_ctrl, &data_qpi_en, ONE_BYTE, false);
	if (ret) {
		LOG_ERR("Direct write SPI_FLASH_PROTOCOL_QPI failed");
		return -EIO;
	}

	ret = R_QSPI_SpiProtocolSet(&qspi_data->qspi_ctrl, SPI_FLASH_PROTOCOL_QPI);
	if (ret) {
		LOG_ERR("Set SpiProtocol failed");
		return -EIO;
	}
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, qspi_flash_ra_init, NULL, &qspi_flash_data, &qspi_nor_dev_config,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &qspi_flash_ra_api);
