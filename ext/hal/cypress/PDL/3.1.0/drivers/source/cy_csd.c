/***************************************************************************//**
* \file cy_csd.c
* \version 0.7
*
* \brief
* The source file of the CSD driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include <stdint.h>
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_csd.h"

#if defined(__cplusplus)
extern "C" {
#endif


/**
* \addtogroup group_csd_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_CSD_Init
****************************************************************************//**
*
* \brief
* Configures HW CSD block and locks it to be used by specified middleware.
*
* \details
* This function configures HW CSD block and locks it. If the block is already
* in use by other middleware or by application layer then the function return
* corresponding status and does not configure block.
* 
* It is not possible to trigger any kind of conversion using this function. 
* To do this use Cy_CSD_WriteReg() function.
* 
* \param base
* The pointer to a HW CSD instance.
*
* \param config
* The pointer to a configuration structure which contains initial configuration.
*
* \param key
* An ID of middleware or user level function which is going to work with
* specified the HW CSD block.
*
* \param context
* The pointer to the context structure allocated by a user or a middleware.
*
* \return
* Returns an operation result status (CSD status code).
* See \ref cy_en_csd_status_t.
*
*******************************************************************************/
cy_en_csd_status_t Cy_CSD_Init(CSD_Type * base, cy_stc_csd_config_t const * config, cy_en_csd_key_t key, cy_stc_csd_context_t * context)
{
    cy_en_csd_status_t status = CY_CSD_LOCKED;

    if ((NULL == base) || (CY_CSD_NONE_KEY == key) || (NULL == config) || (NULL == context))
    {
        status = CY_CSD_BAD_PARAM;
    }
    else
    {
        if(CY_CSD_NONE_KEY == context->lockKey)
        {
            context->lockKey = key;
            Cy_CSD_Configure(base, config, key, context);
            status = CY_CSD_SUCCESS;
        }
    }

    return(status);
}


/*******************************************************************************
* Function Name: Cy_CSD_DeInit
****************************************************************************//**
*
* \brief
* Releases HW CSD block previously captured by a middleware.
*
* \details
* Releases HW CSD block previously captured by a middleware. If the block is in 
* use by other instance or if the block perform any kind of conversion, the 
* function return corresponding status and does not perform deinitialization.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param key
* An ID of middleware or user level function which is going to work with
* specified CSD HW block.
*
* \param context
* The pointer to the context structure, allocated by user or middleware.
*
* \return
* Returns an operation result status (CSD status code).
* See \ref cy_en_csd_status_t.
*
*******************************************************************************/
cy_en_csd_status_t Cy_CSD_DeInit(CSD_Type * base, cy_en_csd_key_t key,  cy_stc_csd_context_t * context)
{
    cy_en_csd_status_t status = CY_CSD_LOCKED;

    if(key == context->lockKey)
    {
        if(CY_CSD_SUCCESS == Cy_CSD_GetConversionStatus(base, context))
        {
            context->lockKey = CY_CSD_NONE_KEY;
            status = CY_CSD_SUCCESS;
        }
        else
        {
            status = CY_CSD_BUSY;
        }
    }

    return(status);
}


/*******************************************************************************
* Function Name: Cy_CSD_Configure
****************************************************************************//**
*
* \brief
* Sets configuration to all CSD HW block registers at once.
* 
* \details
* Sets configuration to all CSD HW block registers at once. It includes only
* write-enable registers. It excludes SEQ_START register. Therefore,
* it is not possible to trigger any kind of conversion using this function. 
* To do this use Cy_CSD_WriteReg() function.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param config
* The pointer to configuration structure which contains initial configuration.
*
* \param key
* An ID of middleware or user level function which is going to work with
* specified CSD HW block.
*
* \param context
* The pointer to the context structure, allocated by user or middleware.
*
* \return
* Returns an operation result status (CSD status code).
* See \ref cy_en_csd_status_t.
*
*******************************************************************************/
cy_en_csd_status_t Cy_CSD_Configure(CSD_Type * base, cy_stc_csd_config_t const * config, cy_en_csd_key_t key, cy_stc_csd_context_t * context)
{
    cy_en_csd_status_t status = CY_CSD_LOCKED;

    if (key == CY_CSD_NONE_KEY)
    {
        status = CY_CSD_BAD_PARAM;
    }
    else
    {
        if(key == context->lockKey)
        {
            status = CY_CSD_SUCCESS;

            base->CONFIG         = config->config;
            base->SPARE          = config->spare;
            base->INTR           = config->intr;
            base->INTR_SET       = config->intrSet;
            base->INTR_MASK      = config->intrMask;
            base->HSCMP          = config->hscmp;
            base->AMBUF          = config->ambuf;
            base->REFGEN         = config->refgen;
            base->CSDCMP         = config->csdCmp;
            base->SW_RES         = config->swRes;
            base->SENSE_PERIOD   = config->sensePeriod;
            base->SENSE_DUTY     = config->senseDuty;
            base->SW_HS_P_SEL    = config->swHsPosSel;
            base->SW_HS_N_SEL    = config->swHsNegSel;
            base->SW_SHIELD_SEL  = config->swShieldSel;
            base->SW_AMUXBUF_SEL = config->swAmuxbufSel;
            base->SW_BYP_SEL     = config->swBypSel;
            base->SW_CMP_P_SEL   = config->swCmpPosSel;
            base->SW_CMP_N_SEL   = config->swCmpNegSel;
            base->SW_REFGEN_SEL  = config->swRefgenSel;
            base->SW_FW_MOD_SEL  = config->swFwModSel;
            base->SW_FW_TANK_SEL = config->swFwTankSel;
            base->SW_DSI_SEL     = config->swDsiSel;
            base->IO_SEL         = config->ioSel;
            base->SEQ_TIME       = config->seqTime;
            base->SEQ_INIT_CNT   = config->seqInitCnt;
            base->SEQ_NORM_CNT   = config->seqNormCnt;
            base->ADC_CTL        = config->adcCtl;
            base->IDACA          = config->idacA;
            base->IDACB          = config->idacB;
        }
    }

    return(status);
}

/** \} group_csd_functions */

#if defined(__cplusplus)
}
#endif


/* [] END OF FILE */
