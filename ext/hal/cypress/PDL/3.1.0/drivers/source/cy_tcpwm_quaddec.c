/***************************************************************************//**
* \file cy_tcpwm_quaddec.c
* \version 1.10
*
* \brief
*  The source file of the tcpwm driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_tcpwm_quaddec.h"

#ifdef CY_IP_MXTCPWM

#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_Init
****************************************************************************//**
*
* Initializes the counter in the TCPWM block for the QuadDec operation.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum
* The Counter instance number in the selected TCPWM.
*
* \param config
* The pointer to a configuration structure. See \ref cy_stc_tcpwm_quaddec_config_t.
*
* \return error / status code. See \ref cy_en_tcpwm_status_t.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_Config
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_Init
*
*******************************************************************************/
cy_en_tcpwm_status_t Cy_TCPWM_QuadDec_Init(TCPWM_Type *base, uint32_t cntNum, 
                                           cy_stc_tcpwm_quaddec_config_t const *config)
{
    cy_en_tcpwm_status_t status = CY_TCPWM_BAD_PARAM;

    if ((NULL != base) && (NULL != config))
    {
        TCPWM_CNT_CTRL(base, cntNum) = ( _VAL2FLD(TCPWM_CNT_CTRL_QUADRATURE_MODE, config->resolution) |
                            _VAL2FLD(TCPWM_CNT_CTRL_MODE, CY_TCPWM_QUADDEC_CTRL_QUADDEC_MODE));

        if (CY_TCPWM_INPUT_CREATOR != config->phiAInput)
        {        
            TCPWM_CNT_TR_CTRL0(base, cntNum) = (_VAL2FLD(TCPWM_CNT_TR_CTRL0_COUNT_SEL, config->phiAInput) |
                                              _VAL2FLD(TCPWM_CNT_TR_CTRL0_START_SEL, config->phiBInput) |
                                              _VAL2FLD(TCPWM_CNT_TR_CTRL0_RELOAD_SEL, config->indexInput) |
                                              _VAL2FLD(TCPWM_CNT_TR_CTRL0_STOP_SEL, config->stopInput));
        }
        
        TCPWM_CNT_TR_CTRL1(base, cntNum) = (_VAL2FLD(TCPWM_CNT_TR_CTRL1_CAPTURE_EDGE, CY_TCPWM_INPUT_LEVEL) |
                                          _VAL2FLD(TCPWM_CNT_TR_CTRL1_COUNT_EDGE, CY_TCPWM_INPUT_LEVEL) |
                                          _VAL2FLD(TCPWM_CNT_TR_CTRL1_START_EDGE, CY_TCPWM_INPUT_LEVEL) |
                                          _VAL2FLD(TCPWM_CNT_TR_CTRL1_RELOAD_EDGE, config->indexInputMode) |
                                          _VAL2FLD(TCPWM_CNT_TR_CTRL1_STOP_EDGE, config->stopInputMode));

        TCPWM_CNT_INTR_MASK(base, cntNum) = config->interruptSources;

        status = CY_TCPWM_SUCCESS;
    }

    return(status);
}


/*******************************************************************************
* Function Name: Cy_TCPWM_QuadDec_DeInit
****************************************************************************//**
*
* De-initializes the counter in the TCPWM block, returns register values to 
* default.
*
* \param base
* The pointer to a TCPWM instance.
*
* \param cntNum
* The Counter instance number in the selected TCPWM.
*
* \param config
* The pointer to a configuration structure. See \ref cy_stc_tcpwm_quaddec_config_t.
*
* \funcusage
* \snippet tcpwm/tcpwm_v1_0_quaddec_sut_01.cydsn/main_cm4.c snippet_Cy_TCPWM_QuadDec_DeInit
*
*******************************************************************************/
void Cy_TCPWM_QuadDec_DeInit(TCPWM_Type *base, uint32_t cntNum, cy_stc_tcpwm_quaddec_config_t const *config)
{
    TCPWM_CNT_CTRL(base, cntNum) = CY_TCPWM_CNT_CTRL_DEFAULT;
    TCPWM_CNT_COUNTER(base, cntNum) = CY_TCPWM_CNT_COUNTER_DEFAULT;
    TCPWM_CNT_CC(base, cntNum) = CY_TCPWM_CNT_CC_DEFAULT;
    TCPWM_CNT_CC_BUFF(base, cntNum) = CY_TCPWM_CNT_CC_BUFF_DEFAULT;
    TCPWM_CNT_PERIOD(base, cntNum) = CY_TCPWM_CNT_PERIOD_DEFAULT;
    TCPWM_CNT_PERIOD_BUFF(base, cntNum) = CY_TCPWM_CNT_PERIOD_BUFF_DEFAULT;
    TCPWM_CNT_TR_CTRL1(base, cntNum) = CY_TCPWM_CNT_TR_CTRL1_DEFAULT;
    TCPWM_CNT_TR_CTRL2(base, cntNum) = CY_TCPWM_CNT_TR_CTRL2_DEFAULT;
    TCPWM_CNT_INTR(base, cntNum) = CY_TCPWM_CNT_INTR_DEFAULT;
    TCPWM_CNT_INTR_SET(base, cntNum) = CY_TCPWM_CNT_INTR_SET_DEFAULT;
    TCPWM_CNT_INTR_MASK(base, cntNum) = CY_TCPWM_CNT_INTR_MASK_DEFAULT;

    if (CY_TCPWM_INPUT_CREATOR != config->phiAInput)
    {        
        TCPWM_CNT_TR_CTRL0(base, cntNum) = CY_TCPWM_CNT_TR_CTRL0_DEFAULT;
    }
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXTCPWM */

/* [] END OF FILE */
