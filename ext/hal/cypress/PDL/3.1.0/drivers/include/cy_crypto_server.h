/***************************************************************************//**
* \file cy_crypto_server.h
* \version 2.10
*
* \brief
*  This file provides the prototypes for common API
*  in the Crypto driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/


#if !defined(CY_CRYPTO_SERVER_H)
#define CY_CRYPTO_SERVER_H

#include "cy_crypto_common.h"
#include "cy_syslib.h"

#if (CY_CRYPTO_CORE_ENABLE)

#if (CPUSS_CRYPTO_PRESENT == 1)

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_crypto_srv_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_Crypto_Server_Start_Base
****************************************************************************//**
*
* This function starts the Crypto server on the CM0+ core, sets up an interrupt
* for the IPC Crypto channel on the CM0+ core, sets up an interrupt
* to catch Crypto HW errors. Should be invoked only on CM0.
*
* This function available for CM0+ core only.
*
* \param config
* The Crypto configuration structure.
*
* \param context
* The pointer to the \ref cy_stc_crypto_server_context_t structure that stores
* the Crypto server context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Server_Start_Base(cy_stc_crypto_config_t const *config,
                                             cy_stc_crypto_server_context_t *context);

/*******************************************************************************
* Function Name: Cy_Crypto_Server_Start_Extra
****************************************************************************//**
*
* This function starts the Crypto server on the CM0+ core, sets up an interrupt
* for the IPC Crypto channel on the CM0+ core, sets up an interrupt
* to catch Crypto HW errors. Should be invoked only on CM0.
*
* This function available for CM0+ core only.
*
* \param config
* The Crypto configuration structure.
*
* \param context
* The pointer to the \ref cy_stc_crypto_server_context_t structure that stores
* the Crypto server context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Server_Start_Extra(cy_stc_crypto_config_t const *config,
                                             cy_stc_crypto_server_context_t *context);

/*******************************************************************************
* Function Name: Cy_Crypto_Server_Start_Full
****************************************************************************//**
*
* This function starts the Crypto server on the CM0+ core, sets up an interrupt
* for the IPC Crypto channel on the CM0+ core, sets up an interrupt
* to catch Crypto HW errors. Should be invoked only on CM0.
*
* This function available for CM0+ core only.
*
* \param config
* The Crypto configuration structure.
*
* \param context
* The pointer to the \ref cy_stc_crypto_server_context_t structure that stores
* the Crypto server context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Server_Start_Full(cy_stc_crypto_config_t const *config,
                                             cy_stc_crypto_server_context_t *context);

/*******************************************************************************
* Function Name: Cy_Crypto_Server_Stop
****************************************************************************//**
*
* This function stops the Crypto server by disabling the IPC notify interrupt
* and Crypto error interrupt. Should be invoked only on CM0.
*
* This function available for CM0+ core only.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Server_Stop(void);

/*******************************************************************************
* Function Name: Cy_Crypto_Server_Process
****************************************************************************//**
*
* This function parses input data received from the Crypto Client,
* runs the appropriate Crypto function and releases the Crypto IPC channel.
*
* This function available for CM0+ core only.
*
*******************************************************************************/
void Cy_Crypto_Server_Process(void);

/*******************************************************************************
* Function Name: Cy_Crypto_Server_GetDataHandler
****************************************************************************//**
*
* This function is a IPC Crypto channel notify interrupt-routine.
* It receives information from the Crypto client,
* runs the process if user not setup own handler.
*
* This function available for CM0+ core only.
*
*******************************************************************************/
void Cy_Crypto_Server_GetDataHandler(void);

/*******************************************************************************
* Function Name: Cy_Crypto_Server_ErrorHandler
****************************************************************************//**
*
* This function is a routine to handle an interrupt caused by the Crypto hardware error.
*
* This function available for CM0+ core only.
*
*******************************************************************************/
void Cy_Crypto_Server_ErrorHandler(void);

/** \} group_crypto_srv_functions */

#define Cy_Crypto_Server_Start               Cy_Crypto_Server_Start_Full

#if defined(__cplusplus)
}
#endif

#endif /* #if (CPUSS_CRYPTO_PRESENT == 1) */

#endif /* #if (CY_CRYPTO_CORE_ENABLE) */

#endif /* #if !defined(CY_CRYPTO_SERVER_H) */


/* [] END OF FILE */
