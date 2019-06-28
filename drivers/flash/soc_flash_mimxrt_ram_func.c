/*
 * Copyright (c) 2019 Riedo Networks Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * This is the driver for the S26KL family of HyperFlash devices connected to i.MX-RT
 * hybrid micro-controller family. Tested on mimxrt1050_evk. This code is based on
 * the example "flexspi_hyper_flash_polling_transfer" from NXP's EVKB-IMXRT1050-SDK package.
 *
 * This file contains the RAM function needed for the driver.
 */


#if !CONFIG_CODE_DATA_RELOCATION
#error CONFIG_CODE_DATA_RELOCATION must be enable to use SOC_FLASH_IMXRT.
#endif


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
extern flexspi_device_config_t s26kl_deviceconfig;
extern const uint32_t s26kl_lut[CUSTOM_LUT_LENGTH];

#endif /* CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL */

static inline void flexspi_clock_update(void)
{
	/* Program finished, speed the clock to 166M. */
	FLEXSPI_Enable(FLEXSPI, false);
	CLOCK_DisableClock(kCLOCK_FlexSpi);
	CLOCK_SetDiv(kCLOCK_FlexspiDiv, 0); /* flexspi clock 332M, DDR mode, internal clock 166M. */
	CLOCK_EnableClock(kCLOCK_FlexSpi);
	FLEXSPI_Enable(FLEXSPI, true);
}

#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
int _sk25kl_unlock()
{
	flexspi_transfer_t flashXfer;
	int status;

	/* Write enable */
	flashXfer.deviceAddress = 0;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Command;
	flashXfer.SeqNumber = 2;
	flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE;

	status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

	return (status == kStatus_Success) ? 0 : -EIO;
}


static inline void _sk26kl_felxspi_clock_init(void)
{
	/* Set flexspi root clock to 166MHZ. */
	const clock_usb_pll_config_t g_ccmConfigUsbPll = { .loopDivider = 0U };

	CLOCK_InitUsb1Pll(&g_ccmConfigUsbPll);
	CLOCK_InitUsb1Pfd(kCLOCK_Pfd0, 26);     /* Set PLL3 PFD0 clock 332MHZ. */
	CLOCK_SetMux(kCLOCK_FlexspiMux, 0x3);   /* Choose PLL3 PFD0 clock as flexspi source clock. */
	CLOCK_SetDiv(kCLOCK_FlexspiDiv, 3);     /* flexspi clock 83M, DDR mode, internal clock 42M. */
}
#endif // CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL

int flash_mimxrt_init(struct device *dev)
{
	static flexspi_config_t config;
	struct flash_priv *priv = dev->driver_data;


#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
	_sk26kl_felxspi_clock_init();
#endif  /* CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL */

	/*Get FLEXSPI default settings and configure the flexspi. */
	FLEXSPI_GetDefaultConfig(&config);

#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL

	/*Set AHB buffer size for reading data through AHB bus. */
	config.ahbConfig.enableAHBPrefetch = true;
	/*Allow AHB read start address do not follow the alignment requirement. */
	config.ahbConfig.enableReadAddressOpt = true;
	config.ahbConfig.enableAHBBufferable = true;
	config.ahbConfig.enableAHBCachable = true;
	/* enable diff clock and DQS */
	config.enableSckBDiffOpt = true;
	config.rxSampleClock = kFLEXSPI_ReadSampleClkExternalInputFromDqsPad;
	config.enableCombination = true;
	FLEXSPI_Init(FLEXSPI, &config);

	/* Configure flash settings according to serial flash feature. */
	FLEXSPI_SetFlashConfig(FLEXSPI, &s26kl_deviceconfig, kFLEXSPI_PortA1);

	/* Update LUT table. */
	FLEXSPI_UpdateLUT(FLEXSPI, 0, s26kl_lut, CUSTOM_LUT_LENGTH);
#endif  // CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL

	/* Do software reset. */
	FLEXSPI_SoftwareReset(FLEXSPI);


	k_sem_init(&priv->write_lock, 0, 1);

	return 0;
}

#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
static status_t _flexspi_sk26kl_wait_bus_busy()
{
	/* Wait status ready. */
	bool isBusy;
	uint32_t readValue;
	status_t status;
	flexspi_transfer_t flashXfer;

	flashXfer.deviceAddress = 0;
	flashXfer.port = kFLEXSPI_PortA1;
	flashXfer.cmdType = kFLEXSPI_Read;
	flashXfer.SeqNumber = 2;
	flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_READSTATUS;
	flashXfer.data = &readValue;
	flashXfer.dataSize = 2;

	do {
		status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

		if (status != kStatus_Success) {
			return status;
		}
		if (readValue & 0x8000) {
			isBusy = false;
		} else   {
			isBusy = true;
		}

		if (readValue & 0x3200) {
			status = kStatus_Fail;
			break;
		}

	} while (isBusy);

	return status;
}
#endif /* CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL */


int flash_mimxrt_write(struct device *dev, off_t offset,
		       const void *data, size_t len)
{

	flexspi_transfer_t flashXfer;
	status_t status;
	int return_val = 0;
	int key;

	key = irq_lock();
	{
#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
		_sk25kl_unlock();

		flashXfer.deviceAddress = offset;
		flashXfer.port = kFLEXSPI_PortA1;
		flashXfer.cmdType = kFLEXSPI_Write;
		flashXfer.SeqNumber = 2;
		flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_PAGEPROGRAM;
		flashXfer.data = (uint32_t *)data;
		flashXfer.dataSize = len;
		status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

		if (status != kStatus_Success) {
			return_val =  -EIO;
		}

		if (_flexspi_sk26kl_wait_bus_busy() != kStatus_Success) {
			return_val = -EIO;
		}

#endif   /* CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL */
	}
	irq_unlock(key);

	flexspi_clock_update();

	return return_val;
}


#define SECTOR_MASK (DT_FLASH_ERASE_BLOCK_SIZE - 1)

int flash_mimxrt_erase(struct device *dev, off_t offset, size_t len)
{

	status_t status;
	flexspi_transfer_t flashXfer;
	int return_val = 0;
	int key;

#if 1
	/* Erase can only be done per sector */
	if (((offset & SECTOR_MASK) != 0) || ((len & SECTOR_MASK) != 0)) {
		return -EINVAL;
	}
#endif

	key = irq_lock();
	{
#if CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL
		/* "unlock" */
		_sk25kl_unlock();

		flashXfer.deviceAddress = offset;
		flashXfer.port = kFLEXSPI_PortA1;
		flashXfer.cmdType = kFLEXSPI_Command;
		flashXfer.SeqNumber = 4;
		flashXfer.seqIndex = HYPERFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR;
		status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

		if (status != kStatus_Success) {
			return_val =  -EIO;
		}

		if (_flexspi_sk26kl_wait_bus_busy() != kStatus_Success) {
			return_val =  -EIO;
		}
#endif          /* CONFIG_FLASH_IMXRT_HYPERFLASH_S26KL */
	}
	irq_unlock(key);
	return return_val;
}
