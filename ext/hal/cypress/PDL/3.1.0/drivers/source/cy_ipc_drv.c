/***************************************************************************//**
* \file cy_ipc_drv.c
* \version 1.30
*
*  \breif
*   IPC Driver - This source file contains the low-level driver code for
*   the IPC hardware.
*
********************************************************************************
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_ipc_drv.h"


/*******************************************************************************
* Function Name: Cy_IPC_Drv_LockRelease
****************************************************************************//**
*
* The function is used to release an IPC channel from the locked state.
* The function also has a way to specify through a parameter which IPC
* interrupts must be notified during the release event.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param releaseEventIntr
* Bit encoded list of IPC interrupt lines that are triggered by a release event.
*
* \return   Status of the operation
*   \retval CY_IPC_DRV_SUCCESS: The function executed successfully and the IPC channel
*                       was released.
*   \retval CY_IPC_DRV_ERROR: The IPC channel was not acquired before the
*                       function call.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_ReadMsgPtr
*
*******************************************************************************/
cy_en_ipcdrv_status_t Cy_IPC_Drv_LockRelease (IPC_STRUCT_Type* base, uint32_t releaseEventIntr)
{
    cy_en_ipcdrv_status_t retStatus;

    /* Check to make sure the IPC is Acquired */
    if( Cy_IPC_Drv_IsLockAcquired(base) )
    {
        /* The IPC was acquired, release the IPC channel */
        Cy_IPC_Drv_ReleaseNotify(base, releaseEventIntr);

        retStatus = CY_IPC_DRV_SUCCESS;
    }
    else   /* The IPC channel was already released (not acquired) */
    {
        retStatus = CY_IPC_DRV_ERROR;
    }

    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_IPC_Drv_SendMsgWord
****************************************************************************//**
*
* This function is used to send a 32-bit word message through an IPC channel.
* The function also has an associated notification field that will let the
* message notify one or multiple IPC interrupts. The IPC channel is locked and
* remains locked after the function returns.  The receiver of the message should
* release the channel.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param notifyEventIntr
* Bit encoded list of IPC interrupt lines that are triggered by a notification.
*
* \param message
* The message word that is the data placed in the IPC data register.
*
* \return   Status of the operation:
*   \retval CY_IPC_DRV_SUCCESS: The send operation was successful.
*   \retval CY_IPC_DRV_ERROR: The IPC channel is unavailable because it is already locked.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_SendMsgWord
*
*******************************************************************************/
cy_en_ipcdrv_status_t  Cy_IPC_Drv_SendMsgWord (IPC_STRUCT_Type* base, uint32_t notifyEventIntr, uint32_t message)
{
    cy_en_ipcdrv_status_t retStatus;

    if( CY_IPC_DRV_SUCCESS == Cy_IPC_Drv_LockAcquire(base) )
    {
        /* If the channel was acquired, send the message. */
        Cy_IPC_Drv_WriteDataValue(base, message);

        Cy_IPC_Drv_AcquireNotify(base, notifyEventIntr);

        retStatus = CY_IPC_DRV_SUCCESS;
    }
    else
    {
        /* Channel was already acquired, return Error */
        retStatus = CY_IPC_DRV_ERROR;
    }
    return (retStatus);
}


/*******************************************************************************
* Function Name: Cy_IPC_Drv_ReadMsgWord
****************************************************************************//**
*
* This function is used to read a 32-bit word message through an IPC channel.
* This function assumes that the channel is locked (for a valid message).
* If the channel is not locked, the message is invalid.  The user must call
* Cy_IPC_Drv_Release() function after reading the message to release the
* IPC channel.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param message
*  A variable where the read data is copied.
*
* \return  Status of the operation
*   \retval CY_IPC_DRV_SUCCESS: The function executed successfully and the IPC
*                       was acquired.
*   \retval CY_IPC_DRV_ERROR:   The function encountered an error because the IPC
*                       channel was already in a released state, meaning the data
*                       may be invalid.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_ReadMsgWord
*
*******************************************************************************/
cy_en_ipcdrv_status_t  Cy_IPC_Drv_ReadMsgWord (IPC_STRUCT_Type const * base, uint32_t * message)
{
    cy_en_ipcdrv_status_t retStatus;

    CY_ASSERT_L1(NULL != message);

    if ( Cy_IPC_Drv_IsLockAcquired(base) )
    {
        /* The channel is locked; message is valid. */
        *message = Cy_IPC_Drv_ReadDataValue(base);

        retStatus = CY_IPC_DRV_SUCCESS;
    }
    else
    {
        /* The channel is not locked so channel is invalid. */
        retStatus = CY_IPC_DRV_ERROR;
    }
    return(retStatus);
}


/* [] END OF FILE */
