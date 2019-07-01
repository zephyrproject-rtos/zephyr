/*
 * Copyright (c) 2019 Riedo Networks Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * This is the driver for the S26KL family of HyperFlash devices connected to
 * i.MX-RT hybrid micro-controller family. Tested on mimxrt1050_evk.
 * This code is based on the example "flexspi_hyper_flash_polling_transfer"
 *  from NXP's EVKB-IMXRT1050-SDK package.
 */



#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include "flash_priv.h"

#include "soc_flash_mimxrt.h"

#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
flexspi_device_config_t s26kl_deviceconfig = {
	.flexspiRootClk = 42000000,     /* 42MHZ SPI serial clock */
	.isSck2Enabled = false,
	.flashSize = DT_FLASH_SIZE,     /* Flash size must be in kBytes! */
	.CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
	.CSInterval = 2,
	.CSHoldTime = 0,
	.CSSetupTime = 3,
	.dataValidTime = 1,
	.columnspace = 3,
	.enableWordAddress = true,
	.AWRSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA,
	.AWRSeqNumber = 1,
	.ARDSeqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA,
	.ARDSeqNumber = 1,
	.AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
	.AHBWriteWaitInterval = 20,
};


const u32_t s26kl_lut[CUSTOM_LUT_LENGTH] = {
	/* Read Data */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0,
			kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA + 1] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
			kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x04
		),

	/* Write Data */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x20,
			kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEDATA + 1] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
			kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x02
		),
	/* Read Status */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 1] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		), /* ADDR 0x555 */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 2] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 3] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x70
		),  /* DATA 0x70 */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 4] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0,
			kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 5] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
			kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 0x0B
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 6] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x04,
			kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0
		),

	/* Write Enable */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 1] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 2] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 3] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 4] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 5] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 6] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 7] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55
		),

	/* Erase Sector  */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 1] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 2] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 3] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x80
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 4] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 5] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 6] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 7] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 8] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 9] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 10] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 11] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 12] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 13] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 14] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x30,
			kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00
		),

	/* program page */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 1] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 2] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 3] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 4] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM + 5] =
		FLEXSPI_LUT_SEQ(k
		FLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
		kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x80
	),

	/* Erase chip */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 1] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 2] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 3] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x80
		),
	/* 1 */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 4] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 5] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 6] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 7] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	/* 2 */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 8] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 9] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 10] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 11] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55
		),
	/* 3 */
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 12] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 13] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 14] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05
		),
	[4 * HYPERFLASH_CMD_LUT_SEQ_IDX_ERASECHIP + 15] =
		FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
			kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x10
		),
};
#elif CONFIG_FLASH_IMXRT_QSPI_IS25WP
#error IS25WPxxx QSPI NOR Flash not supported for the moment
#else
#error No flash device configured!
#endif

static int flash_mimxrt_read(struct device *dev, off_t offset,
			     void *data, size_t len)
{
#if CONFIG_FLASH_IMXRT_MEMCPY_READ

	u32_t addr = offset + CONFIG_FLASH_BASE_ADDRESS;

	memcpy(data, (void *) addr, len);

#else

#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
	flexspi_transfer_t flashXfer;
	status_t status;


	flashXfer.deviceAddress = offset;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Read;
	flashXfer.SeqNumber = 1;
	flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READDATA;
	flashXfer.data = data;
	flashXfer.dataSize = len;
	status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

	if (status != kStatus_Success) {
		return -EIO;
	}
#endif

#endif

	return 0;
}

/* Implemented in soc_flash_mimxrt_ram_func.c*/
int flash_mimxrt_init(struct device *dev);
int flash_mimxrt_write(struct device *dev, off_t offset,
		       const void *data, size_t len);
int flash_mimxrt_erase(struct device *dev, off_t offset, size_t len);


static int flash_mimxrt_write_protection(struct device *dev, bool enable)
{

	struct flash_priv *priv = dev->driver_data;

	if (enable) {
		return k_sem_take(&priv->write_lock, K_FOREVER);
	}
	k_sem_give(&priv->write_lock);

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count = (DT_FLASH_SIZE << 10) / DT_FLASH_ERASE_BLOCK_SIZE,
	.pages_size = DT_FLASH_ERASE_BLOCK_SIZE,
};


static void flash_mimxrt_pages_layout(struct device *dev,
				      const struct flash_pages_layout **layout,
				      size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static struct flash_priv flash_data;

static const struct flash_driver_api flash_mimxrt_api = {
	.write_protection = flash_mimxrt_write_protection,
	.erase = flash_mimxrt_erase,
	.write = flash_mimxrt_write,
	.read = flash_mimxrt_read,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mimxrt_pages_layout,
#endif
	.write_block_size = DT_FLASH_WRITE_BLOCK_SIZE,
};



DEVICE_AND_API_INIT(flash_mimxrt, DT_FLASH_DEV_NAME,
		    flash_mimxrt_init, &flash_data, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_mimxrt_api);

