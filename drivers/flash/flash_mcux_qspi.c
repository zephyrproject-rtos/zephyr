/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_qspi_nor

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include "memc_mcux_qspi.h"
#include "spi_nor.h"

LOG_MODULE_REGISTER(flash_mcux_qspi, CONFIG_FLASH_LOG_LEVEL);

#define FLASH_MCUX_QSPI_LUT_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0][0]))

/* Flash commands */
#define QSPI_NOR_CMD_READ_STATUS        0x05
#define QSPI_NOR_CMD_WRITE_ENABLE       0x06
#define QSPI_NOR_CMD_READ_MFR_ID        0x90
#define QSPI_NOR_CMD_PAGE_PROGRAM       0x02
#define QSPI_NOR_CMD_QUAD_PAGE_PROGRAM  0x32
#define QSPI_NOR_CMD_SECTOR_ERASE       0x20
#define QSPI_NOR_CMD_BLOCK_ERASE_32K    0x52
#define QSPI_NOR_CMD_BLOCK_ERASE_64K    0xD8
#define QSPI_NOR_CMD_CHIP_ERASE         0xC7
#define QSPI_NOR_CMD_READ_FAST          0x0B
#define QSPI_NOR_CMD_READ_FAST_QUAD     0xEB

/* Status register bits */
#define QSPI_NOR_SR_WIP                 BIT(0)
#define QSPI_NOR_SR_WEL                 BIT(1)

/* wait_ready polling: 100us per poll, 10000 polls = 1s total (covers 400ms sector erase max) */
#define QSPI_NOR_WAIT_READY_POLL_US     100
#define QSPI_NOR_WAIT_READY_MAX_POLLS   10000

/* LUT sequence indices */
#define QSPI_LUT_SEQ_IDX_READ           0   /* Seq0: Quad Read */
#define QSPI_LUT_SEQ_IDX_WRITE_ENABLE   1   /* Seq1: Write Enable */
#define QSPI_LUT_SEQ_IDX_CHIP_ERASE     2   /* Seq2: Erase All */
#define QSPI_LUT_SEQ_IDX_READ_STATUS    3   /* Seq3: Read Status */
#define QSPI_LUT_SEQ_IDX_PAGE_PROGRAM   4   /* Seq4: Page Program */
#define QSPI_LUT_SEQ_IDX_WRITE_REGISTER 5   /* Seq5: Write Register */
#define QSPI_LUT_SEQ_IDX_READ_CONFIG    6   /* Seq6: Read Config Register */
#define QSPI_LUT_SEQ_IDX_ERASE_SECTOR   7   /* Seq7: Erase Sector */
#define QSPI_LUT_SEQ_IDX_READ_ID        8   /* Seq8: Read ID */

struct flash_mcux_qspi_nor_config {
	const struct device *qspi_dev;
	const char *dev_name;
	struct flash_parameters flash_params;
	uint32_t flash_size;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout pages_layout;
#endif
};

struct flash_mcux_qspi_nor_data {
	struct k_sem sem;
	uint32_t amba_address;
};

static const uint32_t qspi_nor_lut_table[][FSL_FEATURE_QSPI_LUT_SEQ_UNIT] = {
	/* Seq0: Quad Read (0x6B) */
	/* CMD:   0x6B - Quad Read, Single pad */
	/* ADDR:  0x18 - 24bit address, Single pad */
	/* DUMMY: 0x08 - 8 clock cycles, Quad pad */
	/* READ:  0x80 - Read 128 bytes, Quad pad */
	[QSPI_LUT_SEQ_IDX_READ] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x6B, QSPI_ADDR, QSPI_PAD_1, 0x18),
		QSPI_LUT_SEQ(QSPI_DUMMY, QSPI_PAD_4, 0x08, QSPI_READ, QSPI_PAD_4, 0x80),
	},

	/* Seq1: Write Enable (0x06) */
	/* CMD: 0x06 - Write Enable, Single pad */
	[QSPI_LUT_SEQ_IDX_WRITE_ENABLE] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x06,  QSPI_STOP, 0, 0),
	},

	/* Seq2: Erase All (0x60) */
	/* CMD: 0x60 - Erase All chip, Single pad */
	[QSPI_LUT_SEQ_IDX_CHIP_ERASE] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x60, QSPI_STOP, 0, 0),
	},

	/* Seq3: Read Status (0x05) */
	/* CMD:  0x05 - Read Status, Single pad */
	/* READ: 0x01 - Read 1 byte */
	[QSPI_LUT_SEQ_IDX_READ_STATUS] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x05, QSPI_READ, QSPI_PAD_1, 0x01),
	},

	/* Seq4: Page Program (0x32) - Quad Input Page Program */
	/* CMD:   0x32 - Quad Page Program, Single pad */
	/* ADDR:  0x18 - 24bit address, Single pad */
	/* WRITE: 0x80 - Write 128 bytes, Quad pad */
	[QSPI_LUT_SEQ_IDX_PAGE_PROGRAM] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x32, QSPI_ADDR, QSPI_PAD_1, 0x18),
		QSPI_LUT_SEQ(QSPI_WRITE, QSPI_PAD_4, 0x80, QSPI_STOP, 0, 0),
	},

	/* Seq5: Write Register (0x01) */
	/* CMD:   0x01 - Write Status Register, Single pad */
	/* WRITE: 0x01 - Write 1 byte, Single pad */
	[QSPI_LUT_SEQ_IDX_WRITE_REGISTER] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x01, QSPI_WRITE, QSPI_PAD_1, 0x01),
	},

	/* Seq6: Read Config Register (0x15) */
	/* CMD:  0x15 - Read Config Register, Single pad */
	/* READ: 0x01 - Read 1 byte */
	[QSPI_LUT_SEQ_IDX_READ_CONFIG] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x15, QSPI_READ, QSPI_PAD_1, 0x01),
	},

	/* Seq7: Sector Erase (0x20) - 4KB Sector Erase */
	/* CMD:  0x20 - Sector Erase, Single pad */
	/* ADDR: 0x18 - 24bit address, Single pad */
	[QSPI_LUT_SEQ_IDX_ERASE_SECTOR] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x20, QSPI_ADDR, QSPI_PAD_1, 0x18),
	},

	/* Seq8: Read ID (0x90) */
	/* CMD:  0x90 - Read Manufacturer/Device ID, Single pad */
	/* ADDR: 0x18 - 24bit address, Single pad */
	/* READ: 0x02 - Read 2 bytes */
	[QSPI_LUT_SEQ_IDX_READ_ID] = {
		QSPI_LUT_SEQ(QSPI_CMD, QSPI_PAD_1, 0x90, QSPI_ADDR, QSPI_PAD_1, 0x18),
		QSPI_LUT_SEQ(QSPI_READ, QSPI_PAD_1, 0x02, QSPI_STOP, 0, 0),
	},
};

/* Conditional fields for qspi_flash_config_t (controller feature flags) */
#if defined(FSL_FEATURE_QSPI_HAS_NO_TDH) && FSL_FEATURE_QSPI_HAS_NO_TDH
#define FLASH_MCUX_QSPI_DATA_HOLD_TIME
#else
#define FLASH_MCUX_QSPI_DATA_HOLD_TIME	.dataHoldTime = 0,
#endif

#if defined(FSL_FEATURE_QSPI_HAS_NO_MCR_END) && FSL_FEATURE_QSPI_HAS_NO_MCR_END
#define FLASH_MCUX_QSPI_MCR_END
#else
#define FLASH_MCUX_QSPI_MCR_END	.endian = kQSPI_64LittleEndian,
#endif

/* Flash device-specific configuration (timing + LUT). Add entries here to support new devices. */
struct qspi_nor_dev_config {
	const char *name_prefix;
	uint8_t cs_hold_time;
	uint8_t cs_setup_time;
	const uint32_t *lut;
	size_t lut_count;
};

static const struct qspi_nor_dev_config device_configs[] = {
	{
		.name_prefix   = "w25q64",
		.cs_hold_time  = 3,
		.cs_setup_time = 3,
		.lut           = &qspi_nor_lut_table[0][0],
		.lut_count     = FLASH_MCUX_QSPI_LUT_ARRAY_SIZE(qspi_nor_lut_table),
	},
};

static int flash_mcux_qspi_nor_read_status(const struct device *dev, uint32_t *status)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;
	struct flash_mcux_qspi_nor_data *data = dev->data;

	struct memc_mcux_qspi_transfer transfer = {
		.device_address = data->amba_address,
		.is_read = true,
		.seq_index = QSPI_LUT_SEQ_IDX_READ_STATUS * FSL_FEATURE_QSPI_LUT_SEQ_UNIT,
		.data = status,
		.data_size = 1,
	};
	return memc_mcux_qspi_transfer(config->qspi_dev, &transfer);
}

static int flash_mcux_qspi_nor_wait_ready(const struct device *dev)
{
	uint32_t status;
	int timeout = QSPI_NOR_WAIT_READY_MAX_POLLS;

	while (timeout-- > 0) {
		if (flash_mcux_qspi_nor_read_status(dev, &status) < 0) {
			return -EIO;
		}

		if ((status & QSPI_NOR_SR_WIP) == 0) {
			return 0;
		}

		k_sleep(K_USEC(QSPI_NOR_WAIT_READY_POLL_US));
	}

	return -ETIMEDOUT;
}

static int flash_mcux_qspi_nor_write_enable(const struct device *dev)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;
	struct flash_mcux_qspi_nor_data *data = dev->data;

	struct memc_mcux_qspi_transfer transfer = {
		.device_address = data->amba_address,
		.is_read = false,
		.seq_index = QSPI_LUT_SEQ_IDX_WRITE_ENABLE * FSL_FEATURE_QSPI_LUT_SEQ_UNIT,
		.data = NULL,
		.data_size = 0,
	};
	return memc_mcux_qspi_transfer(config->qspi_dev, &transfer);
}

static ALWAYS_INLINE bool area_is_subregion(const struct device *dev,
							   off_t offset, size_t len)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;

	return ((offset >= 0) && ((size_t)offset < config->flash_size) &&
		((config->flash_size - (size_t)offset) >= len));
}

static int flash_mcux_qspi_nor_read(const struct device *dev, off_t offset,
					void *data, size_t len)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;
	struct flash_mcux_qspi_nor_data *dev_data = dev->data;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (!area_is_subregion(dev, offset, len)) {
		return -EINVAL;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);

	struct memc_mcux_qspi_transfer transfer = {
		.device_address = dev_data->amba_address + offset,
		.is_read = true,
		.seq_index = QSPI_LUT_SEQ_IDX_READ * FSL_FEATURE_QSPI_LUT_SEQ_UNIT,
		.data = data,
		.data_size = len,
	};

	ret = memc_mcux_qspi_transfer(config->qspi_dev, &transfer);
	if (ret < 0) {
		LOG_ERR("memc_mcux_qspi_transfer read failed: %d", ret);
	}

	k_sem_give(&dev_data->sem);

	return ret;
}

static int flash_mcux_qspi_nor_write(const struct device *dev, off_t offset,
					 const void *data, size_t len)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;
	struct flash_mcux_qspi_nor_data *dev_data = dev->data;
	const uint8_t *src = data;
	size_t remaining = len;
	uint32_t page_offset;
	uint32_t write_len;
	int ret = 0;

	if (len == 0) {
		return 0;
	}

	if (!area_is_subregion(dev, offset, len)) {
		return -EINVAL;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);

	while (remaining > 0) {
		page_offset = offset % SPI_NOR_PAGE_SIZE;
		write_len = MIN(remaining, SPI_NOR_PAGE_SIZE - page_offset);

		ret = flash_mcux_qspi_nor_write_enable(dev);
		if (ret < 0) {
			break;
		}

		struct memc_mcux_qspi_transfer transfer = {
			.device_address = dev_data->amba_address + offset,
			.is_read = false,
			.seq_index = QSPI_LUT_SEQ_IDX_PAGE_PROGRAM * FSL_FEATURE_QSPI_LUT_SEQ_UNIT,
			.data = (void *)src,
			.data_size = write_len,
		};
		ret = memc_mcux_qspi_transfer(config->qspi_dev, &transfer);
		if (ret < 0) {
			break;
		}

		ret = flash_mcux_qspi_nor_wait_ready(dev);
		if (ret < 0) {
			break;
		}

		offset += write_len;
		src += write_len;
		remaining -= write_len;
	}

	k_sem_give(&dev_data->sem);
	return ret;
}

static int flash_mcux_qspi_nor_erase_sector(const struct device *dev, off_t offset)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;
	struct flash_mcux_qspi_nor_data *data = dev->data;

	struct memc_mcux_qspi_transfer transfer = {
		.device_address = data->amba_address + offset,
		.is_read = false,
		.seq_index = QSPI_LUT_SEQ_IDX_ERASE_SECTOR * FSL_FEATURE_QSPI_LUT_SEQ_UNIT,
		.data = NULL,
		.data_size = 0,
	};

	return memc_mcux_qspi_transfer(config->qspi_dev, &transfer);
}

static int flash_mcux_qspi_nor_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_mcux_qspi_nor_data *dev_data = dev->data;
	uint32_t erase_size;
	int ret = 0;

	if (size == 0) {
		return 0;
	}

	if (!area_is_subregion(dev, offset, size)) {
		return -EINVAL;
	}

	/* Check alignment */
	if ((offset % SPI_NOR_SECTOR_SIZE) != 0 || (size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);

	while (size > 0) {
		erase_size = SPI_NOR_SECTOR_SIZE;
		ret = flash_mcux_qspi_nor_write_enable(dev);
		if (ret < 0) {
			goto out;
		}
		ret = flash_mcux_qspi_nor_erase_sector(dev, offset);
		if (ret < 0) {
			goto out;
		}

		ret = flash_mcux_qspi_nor_wait_ready(dev);
		if (ret < 0) {
			goto out;
		}

		offset += erase_size;
		size -= erase_size;
	}

out:
	k_sem_give(&dev_data->sem);
	return ret;
}

static const struct flash_parameters *
flash_mcux_qspi_nor_get_parameters(const struct device *dev)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;

	return &config->flash_params;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_mcux_qspi_nor_pages_layout(const struct device *dev,
					     const struct flash_pages_layout **layout,
					     size_t *layout_size)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;

	*layout = &config->pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api flash_mcux_qspi_nor_api = {
	.read = flash_mcux_qspi_nor_read,
	.write = flash_mcux_qspi_nor_write,
	.erase = flash_mcux_qspi_nor_erase,
	.get_parameters = flash_mcux_qspi_nor_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mcux_qspi_nor_pages_layout,
#endif
};

static int flash_mcux_qspi_nor_probe(const struct device *dev)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;
	const struct qspi_nor_dev_config *dev_cfg = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(device_configs); i++) {
		if (strncmp(device_configs[i].name_prefix, config->dev_name,
			    strlen(device_configs[i].name_prefix)) == 0) {
			dev_cfg = &device_configs[i];
			break;
		}
	}

	if (dev_cfg == NULL) {
		LOG_ERR("Unsupported device: %s", config->dev_name);
		return -ENOTSUP;
	}

	qspi_flash_config_t flash_config = {
		.flashA1Size    = config->flash_size,
		.flashA2Size    = 0,
		.CSHoldTime     = dev_cfg->cs_hold_time,
		.CSSetupTime    = dev_cfg->cs_setup_time,
		/* NOTE: SDK typo for "columnspace". */
		.cloumnspace    = 0,
		.dataLearnValue = 0,
		FLASH_MCUX_QSPI_DATA_HOLD_TIME
		FLASH_MCUX_QSPI_MCR_END
		.enableWordAddress = false,
	};

	return memc_qspi_set_device_config(config->qspi_dev, &flash_config,
					   dev_cfg->lut, dev_cfg->lut_count);
}

static int flash_mcux_qspi_nor_init(const struct device *dev)
{
	const struct flash_mcux_qspi_nor_config *config = dev->config;
	struct flash_mcux_qspi_nor_data *data = dev->data;
	uint32_t val = 0;
	int ret;

	if (!device_is_ready(config->qspi_dev)) {
		LOG_ERR("QSPI device is not ready");
		return -ENODEV;
	}

	k_sem_init(&data->sem, 1, 1);

	data->amba_address = memc_mcux_qspi_get_ahb_address(config->qspi_dev);

	ret = flash_mcux_qspi_nor_probe(dev);
	if (ret < 0) {
		return ret;
	}

	struct memc_mcux_qspi_transfer transfer = {
		.device_address = data->amba_address,
		.is_read = true,
		.seq_index = QSPI_LUT_SEQ_IDX_READ_ID * FSL_FEATURE_QSPI_LUT_SEQ_UNIT,
		.data = &val,
		.data_size = 2,
	};

	ret = memc_mcux_qspi_transfer(config->qspi_dev, &transfer);
	if (ret == 0) {
		LOG_INF("Flash Device id=0x%x", val);
	}

	LOG_INF("QSPI NOR flash initialized");

	return 0;
}

#define FLASH_MCUX_QSPI_NOR_INIT(n)						\
	static const struct flash_mcux_qspi_nor_config				\
		flash_mcux_qspi_nor_config_##n = {				\
		.qspi_dev  = DEVICE_DT_GET(DT_INST_PARENT(n)),			\
		.dev_name  = DT_INST_PROP(n, device_name),			\
		.flash_params = {						\
			.write_block_size = 1,					\
			.erase_value = 0xFF,					\
		},								\
		.flash_size = DT_INST_PROP(n, size),				\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (				\
		.pages_layout = {						\
			.pages_count = DT_INST_PROP(n, size) / SPI_NOR_SECTOR_SIZE,\
			.pages_size  = SPI_NOR_SECTOR_SIZE,			\
		},))								\
	};									\
	static struct flash_mcux_qspi_nor_data flash_mcux_qspi_nor_data_##n;	\
	DEVICE_DT_INST_DEFINE(n, &flash_mcux_qspi_nor_init, NULL,		\
				  &flash_mcux_qspi_nor_data_##n,			\
				  &flash_mcux_qspi_nor_config_##n,			\
				  POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,		\
				  &flash_mcux_qspi_nor_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MCUX_QSPI_NOR_INIT)
