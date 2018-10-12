/***************************************************************************//**
* \file cy_ipc_sema.h
* \version 1.30
*
* \brief
* Header file for IPC SEM functions
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef CY_IPC_SEMA_H
#define CY_IPC_SEMA_H

/******************************************************************************/
/* Include files                                                              */
/******************************************************************************/
#include "cy_ipc_drv.h"
#include <stdbool.h>

/**
* \addtogroup group_ipc_sema IPC semaphores layer (IPC_SEMA)
* \{
* The semaphores layer functions made use of a single IPC channel to allow
* multiple semaphores that can be used by system or user function calls.
* By default there are 128 semaphores provided, although the user may modify
* the default value to any number, limited only by SRAM.
*
*     \defgroup group_ipc_sema_macros Macros
*       Macro definitions are used in the driver
*
*     \defgroup group_ipc_sema_functions Functions
*       Functions are used in the driver
*
*     \defgroup group_ipc_sema_enums Enumerated Types
*       Enumerations are used in the driver
* \}
*
* \addtogroup group_ipc_sema_macros
* \{
*/

/** Software PDL driver ID for IPC semaphore functions */
#define CY_IPC_SEMA_RTN        (0x0100ul)
/** Return prefix for IPC semaphore function status codes */
#define CY_IPC_SEMA_ID_INFO    (uint32_t)( CY_IPC_ID_INFO    | CY_IPC_SEMA_RTN)
/** Return prefix for IPC semaphore function warning return values */
#define CY_IPC_SEMA_ID_WARNING (uint32_t)( CY_IPC_ID_WARNING | CY_IPC_SEMA_RTN)
/** Return prefix for IPC semaphore function error return values */
#define CY_IPC_SEMA_ID_ERROR   (uint32_t)( CY_IPC_ID_ERROR   | CY_IPC_SEMA_RTN)

#define CY_IPC_SEMA_PER_WORD    (uint32_t)32u   /**< 32 semaphores per word */

/** \} group_ipc_sema_macros */

/**
* \addtogroup group_ipc_sema_enums
* \{
*/

/** Return constants for IPC semaphores functions. */
typedef enum
{
    /** No error has occurred */
    CY_IPC_SEMA_SUCCESS            = (uint32_t)(0ul),
    /** Semaphores IPC channel has already been locked */
    CY_IPC_SEMA_ERROR_LOCKED       = (uint32_t)(CY_IPC_SEMA_ID_ERROR | 1ul),
    /** Semaphores IPC channel is unlocked */
    CY_IPC_SEMA_ERROR_UNLOCKED     = (uint32_t)(CY_IPC_SEMA_ID_ERROR | 2ul),
    /** Semaphore API bad parameter */
    CY_IPC_SEMA_BAD_PARAM          = (uint32_t)(CY_IPC_SEMA_ID_ERROR | 3ul),
    /** Semaphore API return when semaphore number is out of the range */
    CY_IPC_SEMA_OUT_OF_RANGE       = (uint32_t)(CY_IPC_SEMA_ID_ERROR | 4ul),

    /** Semaphore API return when IPC channel was not acquired */
    CY_IPC_SEMA_NOT_ACQUIRED       = (uint32_t)(CY_IPC_SEMA_ID_INFO  | 2ul),
    /** Semaphore API return status when semaphore channel is busy or locked
*                              by another process */
    CY_IPC_SEMA_LOCKED             = (uint32_t)(CY_IPC_SEMA_ID_INFO  | 3ul),
    /** Semaphore status return that the semaphore is set */
    CY_IPC_SEMA_STATUS_LOCKED      = (uint32_t)(CY_IPC_SEMA_ID_INFO  | 1ul),
    /** Semaphore status return that the semaphore is cleared */
    CY_IPC_SEMA_STATUS_UNLOCKED    = (uint32_t)(CY_IPC_SEMA_ID_INFO  | 0ul)
} cy_en_ipcsema_status_t;

/** \} group_ipc_sema_enums */

/**
* \addtogroup group_ipc_sema_functions
* \{
*/

#ifdef __cplusplus
extern "C" {
#endif

cy_en_ipcsema_status_t   Cy_IPC_Sema_Init (uint32_t ipcChannel, uint32_t count, uint32_t memPtr[]);
cy_en_ipcsema_status_t   Cy_IPC_Sema_Set (uint32_t semaNumber, bool preemptable);
cy_en_ipcsema_status_t   Cy_IPC_Sema_Clear (uint32_t semaNumber, bool preemptable);
cy_en_ipcsema_status_t   Cy_IPC_Sema_Status (uint32_t semaNumber);
uint32_t Cy_IPC_Sema_GetMaxSems(void);

#ifdef __cplusplus
}
#endif

/** \} group_ipc_sema_functions */

#endif /* CY_IPC_SEMA_H  */


/* [] END OF FILE */
