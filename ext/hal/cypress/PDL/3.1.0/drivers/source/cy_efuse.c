/***************************************************************************//**
* \file cy_efuse.c
* \version 1.10
*
* \brief
* Provides API implementation of the eFuse driver.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_efuse.h"
#include "cy_ipc_drv.h"

/** \cond INTERNAL */
#define CY_EFUSE_OPCODE_SUCCESS             (0xA0000000UL)    /**< The command completed with no errors */
#define CY_EFUSE_OPCODE_STS_Msk             (0xF0000000UL)    /**< The status mask of the SROM API return value */
#define CY_EFUSE_OPCODE_INV_PROT            (0xF0000001UL)    /**< The API is not available in the current protection state */
#define CY_EFUSE_OPCODE_INV_ADDR            (0xF0000002UL)    /**< An attempt to read byte from the out-of-bond or protected eFuse region */
#define CY_EFUSE_OPCODE_READ_FUSE_BYTE      (0x03000000UL)    /**< The SROM API opcode for Read fuse byte operation */
#define CY_EFUSE_OPCODE_OFFSET_Pos          (8UL)             /**< A fuse byte offset position in an opcode */
#define CY_EFUSE_OPCODE_DATA_Msk            (0xFFUL)          /**< The mask for extracting data from the SROM API return value */
#define CY_EFUSE_IPC_STRUCT                 (Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL)) /**< IPC structure to be used */
#define CY_EFUSE_IPC_NOTIFY_STRUCT0         (0x1UL << CY_IPC_INTR_SYSCALL1) /**< IPC notify bit for IPC_STRUCT0 (dedicated to System Call) */
/** \endcond */

static volatile uint32_t opcode;

static cy_en_efuse_status_t ProcessOpcode(void);

/*******************************************************************************
* Function Name: Cy_EFUSE_GetEfuseBit
****************************************************************************//**
*
* Reports the current state of a given eFuse bit-number. Consult the device TRM
* to determine the target fuse bit number.
*
* \note An attempt to read an eFuse data from a protected memory region
* will generate a HardFault.
*
* \param bitNum
* The number of the bit to read. The valid range of the bit number is
* from 0 to EFUSE_EFUSE_NR * 32 * 8 - 1 where:
* - EFUSE_EFUSE_NR is number of efuse macros in the selected device series,
* - 32 is a number of fuse bytes in one efuse macro,
* - 8 is a number of fuse bits in the byte.
*
* The EFUSE_EFUSE_NR macro is defined in the series-specific header file, e.g
* \e \<PDL_DIR\>/devices/psoc6/psoc63/include/psoc63_config.\e h
* 
* \param bitVal 
* The pointer to the location to store the bit value.
*
* \return 
* \ref cy_en_efuse_status_t
*
* \funcusage
* The example below shows how to read device life-cycle register bits in
* PSoC 6:
* \snippet eFuse_v1_0_sut_00.cydsn/main_cm0p.c SNIPPET_EFUSE_READ_BIT
*
*******************************************************************************/
cy_en_efuse_status_t Cy_EFUSE_GetEfuseBit(uint32_t bitNum, bool *bitVal)
{
    cy_en_efuse_status_t result = CY_EFUSE_BAD_PARAM;
    
    if (bitVal != NULL)
    {
        uint32_t offset = bitNum / CY_EFUSE_BITS_PER_BYTE;
        uint8_t byteVal;
        *bitVal = false;
        
        /* Read the eFuse byte */
        result = Cy_EFUSE_GetEfuseByte(offset, &byteVal);
        
        if (result == CY_EFUSE_SUCCESS)
        {
            uint32_t bitPos = bitNum % CY_EFUSE_BITS_PER_BYTE;
            /* Extract the bit from the byte */
            *bitVal = (((byteVal >> bitPos) & 0x01U) != 0U);
        }
    }
    return (result);
}


/*******************************************************************************
* Function Name: Cy_EFUSE_GetEfuseByte
****************************************************************************//**
*
* Reports the current state of the eFuse byte.
* If the offset parameter is beyond the available quantities,
* zeroes will be stored to the byteVal parameter. Consult the device TRM
* to determine the target fuse byte offset.
*
* \note An attempt to read an eFuse data from a protected memory region
* will generate a HardFault.
*
* \param offset
* The offset of the byte to read. The valid range of the byte offset is
* from 0 to EFUSE_EFUSE_NR * 32 - 1 where:
* - EFUSE_EFUSE_NR is a number of efuse macros in the selected device series,
* - 32 is a number of fuse bytes in one efuse macro.
*
* The EFUSE_EFUSE_NR macro is defined in the series-specific header file, e.g
* \e \<PDL_DIR\>/devices/psoc6/psoc63/include/psoc63_config.\e h
*
* \param byteVal
* The pointer to the location to store eFuse data.
*
* \return 
* \ref cy_en_efuse_status_t
*
* \funcusage
* The example below shows how to read a device life-cycle stage register in
* PSoC 6:
* \snippet eFuse_v1_0_sut_00.cydsn/main_cm0p.c SNIPPET_EFUSE_READ_LIFECYCLE
*
*******************************************************************************/
cy_en_efuse_status_t Cy_EFUSE_GetEfuseByte(uint32_t offset, uint8_t *byteVal)
{
    cy_en_efuse_status_t result = CY_EFUSE_BAD_PARAM;
    
    if (byteVal != NULL)
    {
        /* Prepare opcode before calling the SROM API */
        opcode = CY_EFUSE_OPCODE_READ_FUSE_BYTE | (offset << CY_EFUSE_OPCODE_OFFSET_Pos);
        
        /* Send the IPC message */
        if (Cy_IPC_Drv_SendMsgPtr(CY_EFUSE_IPC_STRUCT, CY_EFUSE_IPC_NOTIFY_STRUCT0, (void*)&opcode) == CY_IPC_DRV_SUCCESS)
        {           
            /* Wait until the IPC structure is locked */
            while(Cy_IPC_Drv_IsLockAcquired(CY_EFUSE_IPC_STRUCT) != false)
            {
            }
            
            /* The result of the SROM API call is returned to the opcode variable */
            if ((opcode & CY_EFUSE_OPCODE_STS_Msk) == CY_EFUSE_OPCODE_SUCCESS)
            {
                *byteVal = (uint8_t)(opcode & CY_EFUSE_OPCODE_DATA_Msk);
                result = CY_EFUSE_SUCCESS;
            }
            else
            {
                result = ProcessOpcode();
                *byteVal = 0U;
            }
        }
        else
        {
            result = CY_EFUSE_IPC_BUSY;
        }
    }
    return (result);
}


/*******************************************************************************
* Function Name: Cy_EFUSE_GetExternalStatus
****************************************************************************//**
*
* This function handles the case where a module such as a security image captures
* a system call from this driver and reports its own status or error code,
* for example, protection violation. In that case, a function from this
* driver returns an unknown error (see \ref cy_en_efuse_status_t). After receipt
* of an unknown error, the user may call this function to get the status
* of the capturing module.
*
* The user is responsible for parsing the content of the returned value
* and casting it to the appropriate enumeration.
*
* \return
* The error code of the previous efuse operation.
*
*******************************************************************************/
uint32_t Cy_EFUSE_GetExternalStatus(void)
{
    return (opcode);
}


/*******************************************************************************
* Function Name: ProcessOpcode
****************************************************************************//**
*
* Converts System Call returns to the eFuse driver return defines. If 
* an unknown error was returned, the error code can be accessed via the
* Cy_EFUSE_GetExternalStatus() function.
*
* \param opcode The value returned by a System Call.
*
* \return
* \ref cy_en_efuse_status_t
*
*******************************************************************************/
static cy_en_efuse_status_t ProcessOpcode(void)
{
    cy_en_efuse_status_t result;

    switch(opcode)
    {
        case CY_EFUSE_OPCODE_INV_PROT :
        {
            result = CY_EFUSE_INVALID_PROTECTION;
            break;
        }
        case CY_EFUSE_OPCODE_INV_ADDR :
        {
            result = CY_EFUSE_INVALID_FUSE_ADDR;
            break;
        }
        default :
        {
            result = CY_EFUSE_ERR_UNC;
            break;
        }
    }

    return (result);
}


/* [] END OF FILE */
