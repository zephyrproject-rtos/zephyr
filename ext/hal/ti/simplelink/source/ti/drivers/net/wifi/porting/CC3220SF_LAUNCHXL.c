/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/* =========================== SPI Driver Fxns ========================= */
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_types.h>

#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/spi.h>
#include <ti/devices/cc32xx/driverlib/udma.h>

typedef enum CC3220SF_LAUNCHXL_SPIName {
    CC3220SF_LAUNCHXL_SPI0 = 0,
    CC3220SF_LAUNCHXL_SPICOUNT
} CC3220SF_LAUNCHXL_SPIName;

#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC32XXDMA.h>

SPICC32XXDMA_Object spiCC3220SDMAObjects[CC3220SF_LAUNCHXL_SPICOUNT];

__attribute__ ((aligned (32)))
uint32_t spiCC3220SDMAscratchBuf[CC3220SF_LAUNCHXL_SPICOUNT];

const SPICC32XXDMA_HWAttrsV1 spiCC3220SDMAHWAttrs[CC3220SF_LAUNCHXL_SPICOUNT] = {
    /* index 0 is reserved for LSPI that links to the NWP */
    {
	.baseAddr = LSPI_BASE,
	.intNum = INT_LSPI,
	.intPriority = (~0),
	.spiPRCM = PRCM_LSPI,
	.csControl = SPI_SW_CTRL_CS,
	.csPolarity = SPI_CS_ACTIVEHIGH,
	.pinMode = SPI_4PIN_MODE,
	.turboMode = SPI_TURBO_OFF,
	.scratchBufPtr = &spiCC3220SDMAscratchBuf[CC3220SF_LAUNCHXL_SPI0],
	.defaultTxBufValue = 0,
	.rxChannelIndex = UDMA_CH12_LSPI_RX,
	.txChannelIndex = UDMA_CH13_LSPI_TX,
	.minDmaTransferSize = 100,
	.mosiPin = SPICC32XXDMA_PIN_NO_CONFIG,
	.misoPin = SPICC32XXDMA_PIN_NO_CONFIG,
	.clkPin = SPICC32XXDMA_PIN_NO_CONFIG,
	.csPin = SPICC32XXDMA_PIN_NO_CONFIG
    }
};

const SPI_Config SPI_config[CC3220SF_LAUNCHXL_SPICOUNT] = {
    {
	.fxnTablePtr = &SPICC32XXDMA_fxnTable,
	.object = &spiCC3220SDMAObjects[CC3220SF_LAUNCHXL_SPI0],
	.hwAttrs = &spiCC3220SDMAHWAttrs[CC3220SF_LAUNCHXL_SPI0]
    }
};

const uint_least8_t SPI_count = CC3220SF_LAUNCHXL_SPICOUNT;

/*
 *  =============================== DMA ===============================
 */
#include <ti/drivers/dma/UDMACC32XX.h>

__attribute__ ((aligned (1024)))
static tDMAControlTable dmaControlTable[64];

/*
 *  ======== dmaErrorFxn ========
 *  This is the handler for the uDMA error interrupt.
 */
static void dmaErrorFxn(uintptr_t arg)
{
    int status = MAP_uDMAErrorStatusGet();
    MAP_uDMAErrorStatusClear();

    /* Suppress unused variable warning */
    (void)status;

    while (1);
}

UDMACC32XX_Object udmaCC3220SObject;

const UDMACC32XX_HWAttrs udmaCC3220SHWAttrs = {
    .controlBaseAddr = (void *)dmaControlTable,
    .dmaErrorFxn = (UDMACC32XX_ErrorFxn)dmaErrorFxn,
    .intNum = INT_UDMAERR,
    .intPriority = (~0)
};

const UDMACC32XX_Config UDMACC32XX_config = {
    .object = &udmaCC3220SObject,
    .hwAttrs = &udmaCC3220SHWAttrs
};


void CC3220SF_LAUNCHXL_init()
{
	MAP_PRCMLPDSWakeupSourceEnable(PRCM_LPDS_HOST_IRQ);
	SPI_init();
}

#if defined(__SF_DEBUG__)
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(ulDebugHeader, ".dbghdr")
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_location=".dbghdr"
#elif defined(__GNUC__)
__attribute__ ((section (".dbghdr")))
#endif
const unsigned long ulDebugHeader[]=
{
                0x5AA5A55A,
                0x000FF800,
                0xEFA3247D
};
#endif
