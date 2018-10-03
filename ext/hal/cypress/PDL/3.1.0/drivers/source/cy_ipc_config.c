/***************************************************************************//**
* \file cy_ipc_config.c
* \version 1.30
*
*  Description:
*  This C file is not intended to be part of the IPC driver. It is the code
*  required to configure the device specific IPC channels for semaphores
*  and pipes.
*
********************************************************************************
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_ipc_drv.h"
#include "cy_ipc_pipe.h"
#include "cy_ipc_sema.h"

#include "cy_ipc_config.h"

#if (CY_IPC_CYPIPE_ENABLE)
static void Cy_IPC_SystemPipeIsr(void);

#define CY_CYPIPE_DEFAULT_CONFIG    \
{\
    /* .ep0ConfigData */ {\
        /* .ipcNotifierNumber    */  CY_IPC_INTR_CYPIPE_EP0,\
        /* .ipcNotifierPriority  */  CY_IPC_INTR_CYPIPE_PRIOR_EP0,\
        /* .ipcNotifierMuxNumber */  CY_IPC_INTR_CYPIPE_MUX_EP0,\
        /* .epAddress            */  CY_IPC_EP_CYPIPE_CM0_ADDR,\
        /* .epConfig             */  CY_IPC_CYPIPE_CONFIG_EP0\
    },\
    /* .ep1ConfigData */ {\
        /* .ipcNotifierNumber    */  CY_IPC_INTR_CYPIPE_EP1,\
        /* .ipcNotifierPriority  */  CY_IPC_INTR_CYPIPE_PRIOR_EP1,\
        /* .ipcNotifierMuxNumber */  0u,\
        /* .epAddress            */  CY_IPC_EP_CYPIPE_CM4_ADDR,\
        /* .epConfig             */  CY_IPC_CYPIPE_CONFIG_EP1\
    },\
    /* .endpointClientsCount     */  CY_IPC_CYPIPE_CLIENT_CNT,\
    /* .endpointsCallbacksArray  */  cy_ipc_pipe_sysCbArray,\
    /* .userPipeIsrHandler       */  &Cy_IPC_SystemPipeIsr\
}
#endif /* CY_IPC_CYPIPE_ENABLE */

/*******************************************************************************
* Function Name: Cy_IPC_SystemSemaInit
****************************************************************************//**
*
* Initializes the system semaphores. The system semaphores are used by Flash.
*
* This function is called in the SystemInit() function. If the default startup
* file is not used, or SystemInit() is not called in your project,
* call the following three functions prior to executing any flash or EmEEPROM
* write or erase operation:
*  -# Cy_IPC_SystemSemaInit()
*  -# Cy_IPC_SystemPipeInit()
*  -# Cy_Flash_Init()
*
*******************************************************************************/
void Cy_IPC_SystemSemaInit(void)
{
/* Create array used for semaphores */
#if ((CY_CPU_CORTEX_M0P) || !(__CM0P_PRESENT))
    static uint32_t ipcSemaArray[CY_IPC_SEMA_COUNT / CY_IPC_SEMA_PER_WORD];
    (void) Cy_IPC_Sema_Init(CY_IPC_CHAN_SEMA, CY_IPC_SEMA_COUNT, ipcSemaArray);
#else
    (void) Cy_IPC_Sema_Init(CY_IPC_CHAN_SEMA, 0ul, NULL);
#endif
}

/*******************************************************************************
* Function Name: Cy_IPC_SystemPipeInit
****************************************************************************//**
*
* Initializes the system pipes. The system pipes are used by BLE and Flash.
* \note The function should be called on all CPUs.
*
* This function is called in the SystemInit() function. If the default startup
* file is not used, or SystemInit() is not called in your project,
* call the following three functions prior to executing any flash or EmEEPROM
* write or erase operation:
*  -# Cy_IPC_SystemSemaInit()
*  -# Cy_IPC_SystemPipeInit()
*  -# Cy_Flash_Init()
*
* Also this function is called to support BLE host/controller communication.
*
*******************************************************************************/
void Cy_IPC_SystemPipeInit(void)
{
    /* Create an array of endpoint structures */
    static cy_stc_ipc_pipe_ep_t cy_ipc_pipe_sysEpArray[CY_IPC_MAX_ENDPOINTS];

    Cy_IPC_Pipe_Config(cy_ipc_pipe_sysEpArray);

#if (CY_IPC_CYPIPE_ENABLE)
    uint32_t intr;

    static cy_ipc_pipe_callback_ptr_t cy_ipc_pipe_sysCbArray[CY_IPC_CYPIPE_CLIENT_CNT];

    static const cy_stc_ipc_pipe_config_t systemPipeConfig = CY_CYPIPE_DEFAULT_CONFIG;

    intr = Cy_SysLib_EnterCriticalSection();

    Cy_IPC_Pipe_Init(&systemPipeConfig);

    Cy_SysLib_ExitCriticalSection(intr);
#endif /* CY_IPC_CYPIPE_ENABLE */
}

#if (CY_IPC_CYPIPE_ENABLE)
/*******************************************************************************
* Function Name: Cy_IPC_SystemPipeIsr
****************************************************************************//**
*
* This is the interrupt service routine for the system pipe.
*
*******************************************************************************/
static void Cy_IPC_SystemPipeIsr(void)
{
    Cy_IPC_Pipe_ExecuteCallback(CY_IPC_EP_CYPIPE_ADDR);
}
#endif /* CY_IPC_CYPIPE_ENABLE */


/* [] END OF FILE */

