/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flashiap.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flashiap"
#endif

#define HZ_TO_KHZ_DIV 1000

/*******************************************************************************
 * Code
 ******************************************************************************/

static status_t translate_iap_status(uint32_t status)
{
    /* Translate IAP return code to sdk status code */
    if (status == kStatus_Success)
    {
        return status;
    }
    else
    {
        return MAKE_STATUS(kStatusGroup_FLASHIAP, status);
    }
}

/*!
 * brief	Prepare sector for write operation

 * This function prepares sector(s) for write/erase operation. This function must be
 * called before calling the FLASHIAP_CopyRamToFlash() or FLASHIAP_EraseSector() or
 * FLASHIAP_ErasePage() function. The end sector must be greater than or equal to
 * start sector number.
 *
 * deprecated Do not use this function. It has benn moved to iap driver.
 *
 * param startSector Start sector number.
 * param endSector End sector number.
 *
 * retval #kStatus_FLASHIAP_Success Api was executed successfully.
 * retval #kStatus_FLASHIAP_NoPower Flash memory block is powered down.
 * retval #kStatus_FLASHIAP_NoClock Flash memory block or controller is not clocked.
 * retval #kStatus_FLASHIAP_InvalidSector Sector number is invalid or end sector number
 *         is greater than start sector number.
 * retval #kStatus_FLASHIAP_Busy Flash programming hardware interface is busy.
 */
status_t FLASHIAP_PrepareSectorForWrite(uint32_t startSector, uint32_t endSector)
{
    uint32_t command[5], result[4];

    command[0] = kIapCmd_FLASHIAP_PrepareSectorforWrite;
    command[1] = startSector;
    command[2] = endSector;
    iap_entry(command, result);

    return translate_iap_status(result[0]);
}

/*!
 * brief	Copy RAM to flash.

 * This function programs the flash memory. Corresponding sectors must be prepared
 * via FLASHIAP_PrepareSectorForWrite before calling calling this function. The addresses
 * should be a 256 byte boundary and the number of bytes should be 256 | 512 | 1024 | 4096.
 *
 * deprecated Do not use this function. It has benn moved to iap driver.
 *
 * param dstAddr Destination flash address where data bytes are to be written.
 * param srcAddr Source ram address from where data bytes are to be read.
 * param numOfBytes Number of bytes to be written.
 * param systemCoreClock SystemCoreClock in Hz. It is converted to KHz before calling the
 *                        rom IAP function.
 *
 * retval #kStatus_FLASHIAP_Success Api was executed successfully.
 * retval #kStatus_FLASHIAP_NoPower Flash memory block is powered down.
 * retval #kStatus_FLASHIAP_NoClock Flash memory block or controller is not clocked.
 * retval #kStatus_FLASHIAP_SrcAddrError Source address is not on word boundary.
 * retval #kStatus_FLASHIAP_DstAddrError Destination address is not on a correct boundary.
 * retval #kStatus_FLASHIAP_SrcAddrNotMapped Source address is not mapped in the memory map.
 * retval #kStatus_FLASHIAP_DstAddrNotMapped Destination address is not mapped in the memory map.
 * retval #kStatus_FLASHIAP_CountError Byte count is not multiple of 4 or is not a permitted value.
 * retval #kStatus_FLASHIAP_NotPrepared Command to prepare sector for write operation was not executed.
 * retval #kStatus_FLASHIAP_Busy Flash programming hardware interface is busy.
 */
status_t FLASHIAP_CopyRamToFlash(uint32_t dstAddr, uint32_t *srcAddr, uint32_t numOfBytes, uint32_t systemCoreClock)
{
    uint32_t command[5], result[4];

    command[0] = kIapCmd_FLASHIAP_CopyRamToFlash;
    command[1] = dstAddr;
    command[2] = (uint32_t)srcAddr;
    command[3] = numOfBytes;
    command[4] = systemCoreClock / HZ_TO_KHZ_DIV;
    iap_entry(command, result);

    return translate_iap_status(result[0]);
}

/*!
 * brief	Erase sector

 * This function erases sector(s). The end sector must be greater than or equal to
 * start sector number. FLASHIAP_PrepareSectorForWrite must be called before
 * calling this function.
 *
 * deprecated Do not use this function. It has benn moved to iap driver.
 *
 * param startSector Start sector number.
 * param endSector End sector number.
 * param systemCoreClock SystemCoreClock in Hz. It is converted to KHz before calling the
 *                        rom IAP function.
 *
 * retval #kStatus_FLASHIAP_Success Api was executed successfully.
 * retval #kStatus_FLASHIAP_NoPower Flash memory block is powered down.
 * retval #kStatus_FLASHIAP_NoClock Flash memory block or controller is not clocked.
 * retval #kStatus_FLASHIAP_InvalidSector Sector number is invalid or end sector number
 *         is greater than start sector number.
 * retval #kStatus_FLASHIAP_NotPrepared Command to prepare sector for write operation was not executed.
 * retval #kStatus_FLASHIAP_Busy Flash programming hardware interface is busy.
 */
status_t FLASHIAP_EraseSector(uint32_t startSector, uint32_t endSector, uint32_t systemCoreClock)
{
    uint32_t command[5], result[4];

    command[0] = kIapCmd_FLASHIAP_EraseSector;
    command[1] = startSector;
    command[2] = endSector;
    command[3] = systemCoreClock / HZ_TO_KHZ_DIV;
    iap_entry(command, result);

    return translate_iap_status(result[0]);
}

/*!

 * This function erases page(s). The end page must be greater than or equal to
 * start page number. Corresponding sectors must be prepared via FLASHIAP_PrepareSectorForWrite
 * before calling calling this function.
 *
 * deprecated Do not use this function. It has benn moved to iap driver.
 *
 * param startPage Start page number
 * param endPage End page number
 * param systemCoreClock SystemCoreClock in Hz. It is converted to KHz before calling the
 *                        rom IAP function.
 *
 * retval #kStatus_FLASHIAP_Success Api was executed successfully.
 * retval #kStatus_FLASHIAP_NoPower Flash memory block is powered down.
 * retval #kStatus_FLASHIAP_NoClock Flash memory block or controller is not clocked.
 * retval #kStatus_FLASHIAP_InvalidSector Page number is invalid or end page number
 *         is greater than start page number
 * retval #kStatus_FLASHIAP_NotPrepared Command to prepare sector for write operation was not executed.
 * retval #kStatus_FLASHIAP_Busy Flash programming hardware interface is busy.
 */
status_t FLASHIAP_ErasePage(uint32_t startPage, uint32_t endPage, uint32_t systemCoreClock)
{
    uint32_t command[5], result[4];

    command[0] = kIapCmd_FLASHIAP_ErasePage;
    command[1] = startPage;
    command[2] = endPage;
    command[3] = systemCoreClock / HZ_TO_KHZ_DIV;
    iap_entry(command, result);

    return translate_iap_status(result[0]);
}

/*!
 * brief Blank check sector(s)
 *
 * Blank check single or multiples sectors of flash memory. The end sector must be greater than or equal to
 * start sector number. It can be used to verify the sector eraseure after FLASHIAP_EraseSector call.
 *
 * deprecated Do not use this function. It has benn moved to iap driver.
 *
 * param	startSector	: Start sector number. Must be greater than or equal to start sector number
 * param	endSector	: End sector number
 * retval #kStatus_FLASHIAP_Success One or more sectors are in erased state.
 * retval #kStatus_FLASHIAP_NoPower Flash memory block is powered down.
 * retval #kStatus_FLASHIAP_NoClock Flash memory block or controller is not clocked.
 * retval #kStatus_FLASHIAP_SectorNotblank One or more sectors are not blank.
 */
status_t FLASHIAP_BlankCheckSector(uint32_t startSector, uint32_t endSector)
{
    uint32_t command[5], result[4];

    command[0] = kIapCmd_FLASHIAP_BlankCheckSector;
    command[1] = startSector;
    command[2] = endSector;
    iap_entry(command, result);

    return translate_iap_status(result[0]);
}

/*!
 * brief Compare memory contents of flash with ram.

 * This function compares the contents of flash and ram. It can be used to verify the flash
 * memory contents after FLASHIAP_CopyRamToFlash call.
 *
 * deprecated Do not use this function. It has benn moved to iap driver.
 *
 * param dstAddr Destination flash address.
 * param srcAddr Source ram address.
 * param numOfBytes Number of bytes to be compared.
 *
 * retval #kStatus_FLASHIAP_Success Contents of flash and ram match.
 * retval #kStatus_FLASHIAP_NoPower Flash memory block is powered down.
 * retval #kStatus_FLASHIAP_NoClock Flash memory block or controller is not clocked.
 * retval #kStatus_FLASHIAP_AddrError Address is not on word boundary.
 * retval #kStatus_FLASHIAP_AddrNotMapped Address is not mapped in the memory map.
 * retval #kStatus_FLASHIAP_CountError Byte count is not multiple of 4 or is not a permitted value.
 * retval #kStatus_FLASHIAP_CompareError Destination and source memory contents do not match.
 */
status_t FLASHIAP_Compare(uint32_t dstAddr, uint32_t *srcAddr, uint32_t numOfBytes)
{
    uint32_t command[5], result[4];

    command[0] = kIapCmd_FLASHIAP_Compare;
    command[1] = dstAddr;
    command[2] = (uint32_t)srcAddr;
    command[3] = numOfBytes;
    iap_entry(command, result);

    return translate_iap_status(result[0]);
}
