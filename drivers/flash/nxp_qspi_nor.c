/*
 * Copyright (c) 2020 Softube AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_qspi_nor

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include <devicetree.h>

#include "spi_nor.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(flash_qspi, LOG_LEVEL_DBG);

#include "fsl_flexspi.h"

struct flash_priv {
	struct k_sem write_lock;
	FLEXSPI_Type  *pflash_block_base;
};

static const struct flash_parameters
	flash_mcux_flexspi_qspi_flash_parameters = {
	.write_block_size = DT_INST_PROP(0, write_block_size),
	.erase_value = 0xff,
};

/* Get the ram buffer size from the flexspi node */
#define FLASH_WRITE_SIZE DT_PROP(DT_INST(0, nxp_imx_flexspi), \
				 zephyr_ram_buffer_size)

#define CUSTOM_LUT_LENGTH 60
#define FLASH_QUAD_ENABLE 0x40
#define FLASH_BUSY_STATUS_POL 1
#define FLASH_BUSY_STATUS_OFFSET 0
#define FLASH_ERROR_STATUS_MASK 0x0e

#define NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD 0
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS 1
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE 2
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR 3
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD 4

flexspi_device_config_t deviceconfig = {
	.flexspiRootClk = DT_INST_PROP(0, sck_frequency),
	.flashSize = DT_INST_PROP(0, size) / 1024,
	.CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
	.CSInterval = 2,
	.CSHoldTime = 3,
	.CSSetupTime = 3,
	.dataValidTime = 0,
	.columnspace = 0,
	.enableWordAddress = 0,
	.AWRSeqIndex = 0,
	.AWRSeqNumber = 0,
	.ARDSeqIndex = NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD,
	.ARDSeqNumber = 1,
	.AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
	.AHBWriteWaitInterval = 0,
};

static status_t flash_mcux_flexspi_qspi_wait_bus_busy(struct device *dev,
						      off_t offset)
{
	bool is_busy = true;
	uint32_t read_value;
	status_t status;
	flexspi_transfer_t flash_transfer;

	struct flash_priv *priv = dev->data;
	FLEXSPI_Type *base_address = priv->pflash_block_base;

	flash_transfer.deviceAddress = offset;
	flash_transfer.port = kFLEXSPI_PortA1;
	flash_transfer.cmdType = kFLEXSPI_Read;
	flash_transfer.SeqNumber = 1;
	flash_transfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_READSTATUS;
	flash_transfer.data = &read_value;
	flash_transfer.dataSize = 1;

	do {
		status = FLEXSPI_TransferBlocking(base_address, &flash_transfer);
		if (status != kStatus_Success) {
			return status;
		}
		/*
		   Status bit information for IS25LP064.
		   The Write In Progress (WIP) bit is read-only, and can be used to
		   detect the progress or completion of a program or erase operation.
		   When the WIP bit is “0”, the device is ready for write status register,
		   program or erase operation. When the WIP bit is “1”,
		   the device is busy.
		 */
		is_busy = ((read_value & 0x01UL) == 0x01UL) ? true : false;
	} while (is_busy);

	return status;
}

status_t flash_mcux_flexspi_qspi_write_enable(struct device *dev, off_t offset)
{
	status_t status;
	flexspi_transfer_t flash_transfer;

	struct flash_priv *priv = dev->data;
	FLEXSPI_Type *base_address = priv->pflash_block_base;

	flash_transfer.deviceAddress = offset;
	flash_transfer.port = kFLEXSPI_PortA1;
	flash_transfer.cmdType = kFLEXSPI_Command;
	flash_transfer.SeqNumber = 1;
	flash_transfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_WRITEENABLE;

	status = FLEXSPI_TransferBlocking(base_address, &flash_transfer);

	return status;
}

static int flash_mcux_flexspi_qspi_read(struct device *dev, off_t offset,
					void *data, size_t len)
{
	status_t status;
	flexspi_transfer_t flash_transfer;

#ifdef CONFIG_XIP
	unsigned int key;

	key = irq_lock();
#endif

	struct flash_priv *priv = dev->data;
	FLEXSPI_Type *base_address = priv->pflash_block_base;

	flash_transfer.deviceAddress = offset;
	flash_transfer.port = kFLEXSPI_PortA1;
	flash_transfer.cmdType = kFLEXSPI_Read;
	flash_transfer.SeqNumber = 1;
	flash_transfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD;
	flash_transfer.data = (uint32_t *)data;
	flash_transfer.dataSize = len;

	status = FLEXSPI_TransferBlocking(base_address, &flash_transfer);
	if (status == kStatus_Success) {
		status = flash_mcux_flexspi_qspi_wait_bus_busy(dev, offset);
	}

#ifdef CONFIG_XIP
	irq_unlock(key);
#endif

	return status;
}

static int flash_mcux_flexspi_qspi_write(struct device *dev, off_t offset,
					 const void *data, size_t len)
{
	status_t status = kStatus_Fail;
	flexspi_transfer_t flash_transfer;

#ifdef CONFIG_XIP
	unsigned int key;
#endif

/* Only declare if we need to copy writes to ram */
#if(NXP_QSPI_NOR_RAM_WRITE_BUFFER_SIZE > 0)
	static uint8_t ram_buffer[FLASH_WRITE_SIZE];
#endif

	int sectors = len / FLASH_WRITE_SIZE;
	struct flash_priv *priv = dev->data;
	FLEXSPI_Type *base_address = priv->pflash_block_base;

	if (k_sem_take(&priv->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

#ifdef CONFIG_XIP
	key = irq_lock();
#endif

	if (len % FLASH_WRITE_SIZE) {
		sectors++;
	}

	for (int i = 0; i < sectors; i++) {
		off_t offset_to_sector = offset + i * FLASH_WRITE_SIZE;
		uint32_t *offset_to_data =
			(uint32_t *)((uint8_t *)data + i * FLASH_WRITE_SIZE);
		uint32_t bytes_to_write =
			len - i * FLASH_WRITE_SIZE >= FLASH_WRITE_SIZE ?
			FLASH_WRITE_SIZE : len - i * FLASH_WRITE_SIZE;

#if(NXP_QSPI_NOR_RAM_WRITE_BUFFER_SIZE > 0)
		memcpy(ram_buffer, offset_to_data, bytes_to_write);
#endif

		status = flash_mcux_flexspi_qspi_write_enable(dev, offset);
		if (status == kStatus_Success) {
			flash_transfer.deviceAddress = offset_to_sector;
			flash_transfer.port = kFLEXSPI_PortA1;
			flash_transfer.cmdType = kFLEXSPI_Write;
			flash_transfer.SeqNumber = 1;
			flash_transfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD;
#if(NXP_QSPI_NOR_RAM_WRITE_BUFFER_SIZE > 0)
			flash_transfer.data = (uint32_t *)&ram_buffer[0];
#else
			flash_transfer.data = offset_to_data,
#endif
			flash_transfer.dataSize = bytes_to_write;

			status = FLEXSPI_TransferBlocking(base_address, &flash_transfer);
			if (status == kStatus_Success) {
				status = flash_mcux_flexspi_qspi_wait_bus_busy(dev, offset);
			}
		}
	}

#ifdef CONFIG_XIP
	irq_unlock(key);
#endif
	k_sem_give(&priv->write_lock);

	return status;
}

static int flash_mcux_flexspi_qspi_erase(struct device *dev,
					 off_t offset, size_t len)
{
	status_t status = kStatus_Fail;
	flexspi_transfer_t flash_transfer;

#ifdef CONFIG_XIP
	unsigned int key;
#endif

	struct flash_priv *priv = dev->data;
	FLEXSPI_Type *base_address = priv->pflash_block_base;

	if (k_sem_take(&priv->write_lock, K_NO_WAIT)) {
		return -EACCES;
	}

#ifdef CONFIG_XIP
	key = irq_lock();
#endif

	int sectors = len / DT_INST_PROP(0, erase_block_size);
	if (len % DT_INST_PROP(0, erase_block_size)) {
		sectors++;
	}

	for (int i = 0; i < sectors; i++) {
		off_t offset_to_sector =
			offset + i * DT_INST_PROP(0, erase_block_size);

		status = flash_mcux_flexspi_qspi_write_enable(dev, offset);
		if (status == kStatus_Success) {
			flash_transfer.deviceAddress = offset_to_sector;
			flash_transfer.port = kFLEXSPI_PortA1;
			flash_transfer.cmdType = kFLEXSPI_Command;
			flash_transfer.SeqNumber = 1;
			flash_transfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_ERASESECTOR;

			status = FLEXSPI_TransferBlocking(base_address, &flash_transfer);
			if (status == kStatus_Success) {
				status = flash_mcux_flexspi_qspi_wait_bus_busy(dev, offset);
			}
		}
	}

#ifdef CONFIG_XIP
	irq_unlock(key);
#endif
	k_sem_give(&priv->write_lock);

	return status;
}

/* Write/erase operations in this driver is protected by a semaphore. This
 * prevents access from multiple threads, but using this function the semaphore
 * can be locked, preventing all write/erase operations. */
static int flash_mcux_flexspi_qspi_write_protection(struct device *dev,
						    bool enable)
{
	struct flash_priv *priv = dev->data;
	int rc = 0;

	if (enable) {
		rc = k_sem_take(&priv->write_lock, K_FOREVER);
	} else {
		k_sem_give(&priv->write_lock);
	}

	return rc;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = KB(CONFIG_FLASH_SIZE) /
		       DT_INST_PROP(0, erase_block_size),
	.pages_size = DT_INST_PROP(0, erase_block_size),
};

static void flash_mcux_flexspi_qspi_pages_layout(struct device *dev,
						 const struct flash_pages_layout **layout,
						 size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_mcux_flexspi_qspi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_mcux_flexspi_qspi_flash_parameters;
}

static struct flash_priv flash_data;

static const struct flash_driver_api flash_mcux_flexspi_qspi_api = {
	.write_protection = flash_mcux_flexspi_qspi_write_protection,
	.erase = flash_mcux_flexspi_qspi_erase,
	.write = flash_mcux_flexspi_qspi_write,
	.read = flash_mcux_flexspi_qspi_read,
	.get_parameters = flash_mcux_flexspi_qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mcux_flexspi_qspi_pages_layout,
#endif
};

static int flash_mcux_flexspi_qspi_init(const struct device *dev)
{
	/*
	   Get the parent node of the chosen flash chip to get
	   the reg address of the FlexSPI device
	 */
	FLEXSPI_Type *base =
		(FLEXSPI_Type *)DT_REG_ADDR_BY_IDX(DT_INST(0, nxp_imx_flexspi), 0);

	struct flash_priv *priv = dev->data;

	priv->pflash_block_base = base;

#ifdef CONFIG_XIP
	const unsigned int key = irq_lock();
#endif

	flexspi_config_t config;

	/*Get FLEXSPI default settings and configure the flexspi. */
	FLEXSPI_GetDefaultConfig(&config);

	/* Set AHB buffer size for reading data through AHB bus. */
	config.ahbConfig.enableAHBPrefetch = true;
	config.ahbConfig.enableAHBBufferable = true;
	config.ahbConfig.enableReadAddressOpt = true;
	config.ahbConfig.enableAHBCachable = true;
	config.rxSampleClock = kFLEXSPI_ReadSampleClkLoopbackInternally;
	FLEXSPI_Init(base, &config);

	/* Configure flash settings according to serial flash feature. */
	FLEXSPI_SetFlashConfig(base, &deviceconfig, kFLEXSPI_PortA1);

	/* Do software reset. */
	FLEXSPI_SoftwareReset(base);

#ifdef CONFIG_XIP
	irq_unlock(key);
#endif

	k_sem_init(&priv->write_lock, 0, 1);

	return 0;
}

DEVICE_AND_API_INIT(flash_mcux, DT_INST_LABEL(0),
		    flash_mcux_flexspi_qspi_init, &flash_data, NULL, POST_KERNEL,
		    CONFIG_NXP_QSPI_NOR_INIT_PRIORITY, &flash_mcux_flexspi_qspi_api);
