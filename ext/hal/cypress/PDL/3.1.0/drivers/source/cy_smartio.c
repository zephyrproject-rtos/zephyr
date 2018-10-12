/***************************************************************************//**
* \file cy_smartio.c
* \version 1.0
*
* \brief
* Provides an API implementation of the Smart I/O driver
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_smartio.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
* Function Name: Cy_SmartIO_Init
****************************************************************************//**
*
* \brief Initializes the Smart I/O.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param config
* Pointer to the Smart I/O configuration structure
*
* \return
* Status of the initialization operation
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_Init(SMARTIO_PRT_Type* base, const cy_stc_smartio_config_t* config)
{
    cy_en_smartio_status_t status = CY_SMARTIO_SUCCESS;
    
    if(NULL != config)
    {
        SMARTIO_PRT_CTL(base) = _VAL2FLD(SMARTIO_PRT_CTL_BYPASS, config->bypassMask)
                                | _VAL2FLD(SMARTIO_PRT_CTL_CLOCK_SRC, config->clkSrc)
                                | _VAL2FLD(SMARTIO_PRT_CTL_HLD_OVR, config->hldOvr)
                                | _VAL2FLD(SMARTIO_PRT_CTL_PIPELINE_EN, CY_SMARTIO_ENABLE) 
                                | _VAL2FLD(SMARTIO_PRT_CTL_ENABLED, CY_SMARTIO_DISABLE);
        SMARTIO_PRT_SYNC_CTL(base) = _VAL2FLD(SMARTIO_PRT_SYNC_CTL_IO_SYNC_EN, config->ioSyncEn)
                                | _VAL2FLD(SMARTIO_PRT_SYNC_CTL_CHIP_SYNC_EN, config->chipSyncEn);

        /* LUT configurations - skip if lutCfg is a NULL pointer */
        if(NULL != config->lutCfg0)
        {
            SMARTIO_PRT_LUT_SEL(base, 0) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg0->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg0->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg0->tr2);
            SMARTIO_PRT_LUT_CTL(base, 0) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg0->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg0->opcode);
        }
        if(NULL != config->lutCfg1)
        {
            SMARTIO_PRT_LUT_SEL(base, 1) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg1->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg1->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg1->tr2);
            SMARTIO_PRT_LUT_CTL(base, 1) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg1->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg1->opcode);
        }
        if(NULL != config->lutCfg2)
        {
            SMARTIO_PRT_LUT_SEL(base, 2) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg2->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg2->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg2->tr2);
            SMARTIO_PRT_LUT_CTL(base, 2) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg2->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg2->opcode);
        }
        if(NULL != config->lutCfg3)
        {
            SMARTIO_PRT_LUT_SEL(base, 3) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg3->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg3->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg3->tr2);
            SMARTIO_PRT_LUT_CTL(base, 3) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg3->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg3->opcode);
        }
        if(NULL != config->lutCfg4)
        {
            SMARTIO_PRT_LUT_SEL(base, 4) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg4->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg4->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg4->tr2);
            SMARTIO_PRT_LUT_CTL(base, 4) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg4->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg4->opcode);
        }
        if(NULL != config->lutCfg5)
        {
            SMARTIO_PRT_LUT_SEL(base, 5) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg5->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg5->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg5->tr2);
            SMARTIO_PRT_LUT_CTL(base, 5) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg5->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg5->opcode);
        }
        if(NULL != config->lutCfg6)
        {
            SMARTIO_PRT_LUT_SEL(base, 6) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg6->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg6->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg6->tr2);
            SMARTIO_PRT_LUT_CTL(base, 6) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg6->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg6->opcode);
        }
        if(NULL != config->lutCfg7)
        {
            SMARTIO_PRT_LUT_SEL(base, 7) = _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, config->lutCfg7->tr0)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, config->lutCfg7->tr1)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, config->lutCfg7->tr2);
            SMARTIO_PRT_LUT_CTL(base, 7) = _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, config->lutCfg7->lutMap)
                                            | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, config->lutCfg7->opcode);
        }

        /* DU Configuration - skip if duCfg is a NULL pointer */
        if(NULL != config->duCfg)
        {
            SMARTIO_PRT_DU_SEL(base) = _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR0_SEL, config->duCfg->tr0)
                                        | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR1_SEL, config->duCfg->tr1)
                                        | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR2_SEL, config->duCfg->tr2)
                                        | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_DATA0_SEL, config->duCfg->data0)
                                        | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_DATA1_SEL, config->duCfg->data1);
            SMARTIO_PRT_DU_CTL(base) = _VAL2FLD(SMARTIO_PRT_DU_CTL_DU_SIZE, config->duCfg->size)
                                        | _VAL2FLD(SMARTIO_PRT_DU_CTL_DU_OPC, config->duCfg->opcode);
            SMARTIO_PRT_DATA(base) = _VAL2FLD(SMARTIO_PRT_DATA_DATA, config->duCfg->dataReg);
        }
    }
    else
    {
        status = CY_SMARTIO_BAD_PARAM;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_Deinit
****************************************************************************//**
*
* \brief Resets the Smart I/O to default values.
*
* \param base
* Pointer to the Smart I/O base address
*
*******************************************************************************/
void Cy_SmartIO_Deinit(SMARTIO_PRT_Type* base)
{
    SMARTIO_PRT_CTL(base) = _VAL2FLD(SMARTIO_PRT_CTL_BYPASS, CY_SMARTIO_CHANNEL_ALL)
                            | _VAL2FLD(SMARTIO_PRT_CTL_CLOCK_SRC, CY_SMARTIO_CLK_GATED)
                            | _VAL2FLD(SMARTIO_PRT_CTL_PIPELINE_EN, CY_SMARTIO_ENABLE) 
                            | _VAL2FLD(SMARTIO_PRT_CTL_ENABLED, CY_SMARTIO_DISABLE);
    SMARTIO_PRT_SYNC_CTL(base) = CY_SMARTIO_DEINIT;
    for(uint8_t idx = 0u; idx < CY_SMARTIO_LUTMAX; idx++)
    {
        SMARTIO_PRT_LUT_SEL(base, idx) = CY_SMARTIO_DEINIT;
        SMARTIO_PRT_LUT_CTL(base, idx) = CY_SMARTIO_DEINIT;
    }
    SMARTIO_PRT_DU_SEL(base) = CY_SMARTIO_DEINIT;
    SMARTIO_PRT_DU_CTL(base) = CY_SMARTIO_DEINIT;
    SMARTIO_PRT_DATA(base) = CY_SMARTIO_DEINIT;
}


/*******************************************************************************
* Function Name: Cy_SmartIO_Enable
****************************************************************************//**
*
* \brief Enables the Smart I/O.
*
* \param base
* Pointer to the Smart I/O base address
*
*******************************************************************************/
void Cy_SmartIO_Enable(SMARTIO_PRT_Type* base)
{
    uint32_t tempReg;
    
    tempReg = (SMARTIO_PRT_CTL(base) & (~(SMARTIO_PRT_CTL_PIPELINE_EN_Msk | SMARTIO_PRT_CTL_ENABLED_Msk)));
    SMARTIO_PRT_CTL(base) = tempReg 
                | _VAL2FLD(SMARTIO_PRT_CTL_PIPELINE_EN, CY_SMARTIO_DISABLE) 
                | _VAL2FLD(SMARTIO_PRT_CTL_ENABLED, CY_SMARTIO_ENABLE);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_Disable
****************************************************************************//**
*
* \brief Disables the Smart I/O.
*
* \param base
* Pointer to the Smart I/O base address
*
*******************************************************************************/
void Cy_SmartIO_Disable(SMARTIO_PRT_Type* base)
{
    uint32_t tempReg;
    
    tempReg = (SMARTIO_PRT_CTL(base) & (~(SMARTIO_PRT_CTL_PIPELINE_EN_Msk | SMARTIO_PRT_CTL_ENABLED_Msk)));
    SMARTIO_PRT_CTL(base) = tempReg 
                | _VAL2FLD(SMARTIO_PRT_CTL_PIPELINE_EN, CY_SMARTIO_ENABLE) 
                | _VAL2FLD(SMARTIO_PRT_CTL_ENABLED, CY_SMARTIO_DISABLE);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetChBypass
****************************************************************************//**
*
* \brief Sets the bypass/route state of all 8 channels in the Smart I/O
*
* <table class="doxtable">
*   <tr><th>Bypass bit</th><th>    Channel  </th></tr>
*   <tr><td>     0    </td><td> io0<->chip0 </td></tr>
*   <tr><td>     1    </td><td> io1<->chip1 </td></tr>
*   <tr><td>     2    </td><td> io2<->chip2 </td></tr>
*   <tr><td>     3    </td><td> io3<->chip3 </td></tr>
*   <tr><td>     4    </td><td> io4<->chip4 </td></tr>
*   <tr><td>     5    </td><td> io5<->chip5 </td></tr>
*   <tr><td>     6    </td><td> io6<->chip6 </td></tr>
*   <tr><td>     7    </td><td> io7<->chip7 </td></tr>
* </table>
*
* \param base
* Pointer to the Smart I/O base address
*
* \param bypassMask
* Bypass/Route state of 8 io<->chip channels (bits [7:0]): 1=bypass, 0=routed.
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetChBypass(SMARTIO_PRT_Type* base, uint8_t bypassMask)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_CTL(base) & (~SMARTIO_PRT_CTL_BYPASS_Msk));
        SMARTIO_PRT_CTL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_CTL_BYPASS, bypassMask);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetClock
****************************************************************************//**
*
* \brief Sets the clock source of the Smart I/O.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param clkSrc
* Pointer to the Smart I/O base address
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetClock(SMARTIO_PRT_Type* base, cy_en_smartio_clksrc_t clkSrc)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_CTL(base) & (~SMARTIO_PRT_CTL_CLOCK_SRC_Msk));
        SMARTIO_PRT_CTL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_CTL_CLOCK_SRC, clkSrc);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetIoSync
****************************************************************************//**
*
* \brief Sets the syncronization mode of the 8 I/O terminals.
*
* <table class="doxtable">
*   <tr><th> Sync bit </th><th> I/O terminal </th></tr>
*   <tr><td>     0    </td><td>     io0      </td></tr>
*   <tr><td>     1    </td><td>     io1      </td></tr>
*   <tr><td>     2    </td><td>     io2      </td></tr>
*   <tr><td>     3    </td><td>     io3      </td></tr>
*   <tr><td>     4    </td><td>     io4      </td></tr>
*   <tr><td>     5    </td><td>     io5      </td></tr>
*   <tr><td>     6    </td><td>     io6      </td></tr>
*   <tr><td>     7    </td><td>     io7      </td></tr>
* </table>
*
* \param base
* Pointer to the Smart I/O base address
*
* \param ioSyncEn
* Sync mode of 8 I/O terminals (bits [7:0]): 1=sync, 0=no sync.
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetIoSync(SMARTIO_PRT_Type* base, uint8_t ioSyncEn)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_SYNC_CTL(base) & (~SMARTIO_PRT_SYNC_CTL_IO_SYNC_EN_Msk));
        SMARTIO_PRT_SYNC_CTL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_SYNC_CTL_IO_SYNC_EN, ioSyncEn);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetChipSync
****************************************************************************//**
*
* \brief Sets the syncronization mode of the 8 chip-side terminals.
*
* <table class="doxtable">
*   <tr><th> Sync bit </th><th> chip terminal </th></tr>
*   <tr><td>     0    </td><td>     chip0     </td></tr>
*   <tr><td>     1    </td><td>     chip1     </td></tr>
*   <tr><td>     2    </td><td>     chip2     </td></tr>
*   <tr><td>     3    </td><td>     chip3     </td></tr>
*   <tr><td>     4    </td><td>     chip4     </td></tr>
*   <tr><td>     5    </td><td>     chip5     </td></tr>
*   <tr><td>     6    </td><td>     chip6     </td></tr>
*   <tr><td>     7    </td><td>     chip7     </td></tr>
* </table>
*
* \param base
* Pointer to the Smart I/O base address
*
* \param chipSyncEn
* Sync mode of 8 chip-side terminals (bits [7:0]): 1=sync, 0=no sync.
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetChipSync(SMARTIO_PRT_Type* base, uint8_t chipSyncEn)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_SYNC_CTL(base) & (~SMARTIO_PRT_SYNC_CTL_CHIP_SYNC_EN_Msk));
        SMARTIO_PRT_SYNC_CTL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_SYNC_CTL_CHIP_SYNC_EN, chipSyncEn);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_HoldOverride
****************************************************************************//**
*
* \brief Configures the hold override mode.
*
* In Deep-Sleep power mode, the HSIOM holds the GPIO output and output enable
* signals for all signals that operate in chip active domain. Enabling the hold
* override allows the Smart I/O to deliver Deep-Sleep output functionality
* on these GPIO terminals. If the Smart I/O should not drive any of the GPIO 
* outputs, the hold override should be disabled.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param hldOvr
* true = Enabled: Smart I/O controls the port I/Os
* false = Disabled: HSIOM controls the port I/Os
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_HoldOverride(SMARTIO_PRT_Type* base, bool hldOvr)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_CTL(base) & (~SMARTIO_PRT_CTL_HLD_OVR_Msk));
        SMARTIO_PRT_CTL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_CTL_HLD_OVR, hldOvr);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetLutTr
****************************************************************************//**
*
* \brief Gets the specified LUT input trigger source.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param lutNum
* LUT index number
*
* \param trNum
* Input trigger number
*
* \return
* LUT input trigger source
*
*******************************************************************************/
cy_en_smartio_luttr_t Cy_SmartIO_GetLutTr(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_trnum_t trNum)
{
    cy_en_smartio_luttr_t trSrc;
    
    switch(trNum)
    {
        case(CY_SMARTIO_TR0):
        {
            trSrc = (cy_en_smartio_luttr_t)_FLD2VAL(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, SMARTIO_PRT_LUT_SEL(base, lutNum));
            break;
        }
        case(CY_SMARTIO_TR1):
        {
            trSrc = (cy_en_smartio_luttr_t)_FLD2VAL(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, SMARTIO_PRT_LUT_SEL(base, lutNum));
            break;
        }
        case(CY_SMARTIO_TR2):
        {
            trSrc = (cy_en_smartio_luttr_t)_FLD2VAL(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, SMARTIO_PRT_LUT_SEL(base, lutNum));
            break;
        }
        default:
        {
            trSrc = CY_SMARTIO_LUTTR_INVALID;
            break;
        }
    }
        
    return(trSrc);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetLutTr
****************************************************************************//**
*
* \brief Sets the specified LUT input trigger source.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param lutNum
* LUT index number
*
* \param trNum
* Input trigger number
*
* \param trSrc
* Input trigger source
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetLutTr(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_trnum_t trNum, cy_en_smartio_luttr_t trSrc)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        status = CY_SMARTIO_SUCCESS;
        switch(trNum)
        {
            case(CY_SMARTIO_TR0):
            {
                tempReg = (SMARTIO_PRT_LUT_SEL(base, lutNum) & (~SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL_Msk));
                SMARTIO_PRT_LUT_SEL(base, lutNum) = tempReg | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, trSrc);
                break;
            }
            case(CY_SMARTIO_TR1):
            {
                tempReg = (SMARTIO_PRT_LUT_SEL(base, lutNum) & (~SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL_Msk));
                SMARTIO_PRT_LUT_SEL(base, lutNum) = tempReg | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, trSrc);
                break;
            }
            case(CY_SMARTIO_TR2):
            {
                tempReg = (SMARTIO_PRT_LUT_SEL(base, lutNum) & (~SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL_Msk));
                SMARTIO_PRT_LUT_SEL(base, lutNum) = tempReg | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, trSrc);
                break;
            }
            default:
            {
                status = CY_SMARTIO_BAD_PARAM;
                break;
            }
        }
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetLutTrAll
****************************************************************************//**
*
* \brief Sets all LUT input triggers to the same source.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param lutNum
* LUT index number
*
* \param trSrc
* Input trigger source
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetLutTrAll(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_luttr_t trSrc)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_LUT_SEL(base, lutNum) 
                    & (~(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL_Msk 
                        | SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL_Msk 
                        | SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL_Msk)));
        SMARTIO_PRT_LUT_SEL(base, lutNum) = tempReg 
                                | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL, trSrc)
                                | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL, trSrc)
                                | _VAL2FLD(SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL, trSrc);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetLutOpcode
****************************************************************************//**
*
* \brief Sets the opcode of the specified LUT.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param lutNum
* LUT index number
*
* \param opcode
* LUT opcode
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetLutOpcode(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_lutopc_t opcode)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_LUT_CTL(base, lutNum) & (~SMARTIO_PRT_LUT_CTL_LUT_OPC_Msk));
        SMARTIO_PRT_LUT_CTL(base, lutNum) = tempReg | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT_OPC, opcode);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetLutMap
****************************************************************************//**
*
* \brief Sets the 3 bit input to 1 bit output mapping of the specified LUT.
*
* <table class="doxtable">
*   <tr><th>tr2</th><th>tr1</th><th>tr0</th><th>lutMap</th></tr>
*   <tr><td> 0 </td><td> 0 </td><td> 0 </td><td> bit 0 </td></tr>
*   <tr><td> 0 </td><td> 0 </td><td> 1 </td><td> bit 1 </td></tr>
*   <tr><td> 0 </td><td> 1 </td><td> 0 </td><td> bit 2 </td></tr>
*   <tr><td> 0 </td><td> 1 </td><td> 1 </td><td> bit 3 </td></tr>
*   <tr><td> 1 </td><td> 0 </td><td> 0 </td><td> bit 4 </td></tr>
*   <tr><td> 1 </td><td> 0 </td><td> 1 </td><td> bit 5 </td></tr>
*   <tr><td> 1 </td><td> 1 </td><td> 0 </td><td> bit 6 </td></tr>
*   <tr><td> 1 </td><td> 1 </td><td> 1 </td><td> bit 7 </td></tr>
* </table>
*
* \param base
* Pointer to the Smart I/O base address
*
* \param lutNum
* LUT index number
*
* \param lutMap
* Bitfield [7:0] mapping of the 3:1 LUT
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetLutMap(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, uint8_t lutMap)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_LUT_CTL(base, lutNum) & (~SMARTIO_PRT_LUT_CTL_LUT_Msk));
        SMARTIO_PRT_LUT_CTL(base, lutNum) = tempReg | _VAL2FLD(SMARTIO_PRT_LUT_CTL_LUT, lutMap);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetDuTr
****************************************************************************//**
*
* \brief Gets the data unit input trigger source.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param trNum
* Input trigger number
*
* \return
* Data unit input trigger source
*
*******************************************************************************/
cy_en_smartio_dutr_t Cy_SmartIO_GetDuTr(SMARTIO_PRT_Type* base, cy_en_smartio_trnum_t trNum)
{
    cy_en_smartio_dutr_t trSrc;
    
    switch(trNum)
    {
        case(CY_SMARTIO_TR0):
        {
            trSrc = (cy_en_smartio_dutr_t)_FLD2VAL(SMARTIO_PRT_DU_SEL_DU_TR0_SEL, SMARTIO_PRT_DU_SEL(base));
            break;
        }
        case(CY_SMARTIO_TR1):
        {
            trSrc = (cy_en_smartio_dutr_t)_FLD2VAL(SMARTIO_PRT_DU_SEL_DU_TR1_SEL, SMARTIO_PRT_DU_SEL(base));
            break;
        }
        case(CY_SMARTIO_TR2):
        {
            trSrc = (cy_en_smartio_dutr_t)_FLD2VAL(SMARTIO_PRT_DU_SEL_DU_TR2_SEL, SMARTIO_PRT_DU_SEL(base));
            break;
        }
        default:
        {
            trSrc = CY_SMARTIO_DUTR_INVALID;
            break;
        }
    }
        
    return(trSrc);
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetDuTr
****************************************************************************//**
*
* \brief Sets the data unit input trigger source.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param trNum
* Input trigger number
*
* \param trSrc
* Input trigger source
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetDuTr(SMARTIO_PRT_Type* base, cy_en_smartio_trnum_t trNum, cy_en_smartio_dutr_t trSrc)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        status = CY_SMARTIO_SUCCESS;
        switch(trNum)
        {
            case(CY_SMARTIO_TR0):
            {
                tempReg = (SMARTIO_PRT_DU_SEL(base) & (~SMARTIO_PRT_DU_SEL_DU_TR0_SEL_Msk));
                SMARTIO_PRT_DU_SEL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR0_SEL, trSrc);
                break;
            }
            case(CY_SMARTIO_TR1):
            {
                tempReg = (SMARTIO_PRT_DU_SEL(base) & (~SMARTIO_PRT_DU_SEL_DU_TR1_SEL_Msk));
                SMARTIO_PRT_DU_SEL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR1_SEL, trSrc);
                break;
            }
            case(CY_SMARTIO_TR2):
            {
                tempReg = (SMARTIO_PRT_DU_SEL(base) & (~SMARTIO_PRT_DU_SEL_DU_TR2_SEL_Msk));
                SMARTIO_PRT_DU_SEL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR2_SEL, trSrc);
                break;
            }
            default:
            {
                status = CY_SMARTIO_BAD_PARAM;
                break;
            }
        }
    }
    
    return status;
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetDuTrAll
****************************************************************************//**
*
* \brief Sets all the data unit input trigger sources.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param trSrc
* Input trigger source
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetDuTrAll(SMARTIO_PRT_Type* base, cy_en_smartio_dutr_t trSrc)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        tempReg = (SMARTIO_PRT_DU_SEL(base)
                    & (~(SMARTIO_PRT_DU_SEL_DU_TR0_SEL_Msk 
                        | SMARTIO_PRT_DU_SEL_DU_TR1_SEL_Msk 
                        | SMARTIO_PRT_DU_SEL_DU_TR2_SEL_Msk)));
        SMARTIO_PRT_DU_SEL(base) = tempReg 
                        | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR0_SEL, trSrc)
                        | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR1_SEL, trSrc)
                        | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_TR2_SEL, trSrc);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return status;
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetDuData
****************************************************************************//**
*
* \brief Sets the data unit's input "data" source.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param dataNum
* Input data number
*
* \param dataSrc
* Data unit input trigger source
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetDuData(SMARTIO_PRT_Type* base, cy_en_smartio_datanum_t dataNum, cy_en_smartio_dudata_t dataSrc)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    uint32_t tempReg;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        if(dataNum == CY_SMARTIO_DATA0)
        {
            tempReg = (SMARTIO_PRT_DU_SEL(base) & (~SMARTIO_PRT_DU_SEL_DU_DATA0_SEL_Msk));
            SMARTIO_PRT_DU_SEL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_DATA0_SEL, dataSrc);
        }
        else
        {
            tempReg = (SMARTIO_PRT_DU_SEL(base) & (~SMARTIO_PRT_DU_SEL_DU_DATA1_SEL_Msk));
            SMARTIO_PRT_DU_SEL(base) = tempReg | _VAL2FLD(SMARTIO_PRT_DU_SEL_DU_DATA1_SEL, dataSrc);  
        }
        status = CY_SMARTIO_SUCCESS;
    }
    
    return status;
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetDuOperation
****************************************************************************//**
*
* \brief Sets the data unit's opcode and operand bit-width.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param opcode
* Data Unit opcode
*
* \param size
* Data unit operand bit-width
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetDuOperation(SMARTIO_PRT_Type* base, cy_en_smartio_duopc_t opcode, cy_en_smartio_dusize_t size)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        SMARTIO_PRT_DU_CTL(base) = _VAL2FLD(SMARTIO_PRT_DU_CTL_DU_SIZE, size)
                                    | _VAL2FLD(SMARTIO_PRT_DU_CTL_DU_OPC, opcode);
        status = CY_SMARTIO_SUCCESS;
    }
    
    return status;
}


/*******************************************************************************
* Function Name: Cy_SmartIO_SetDataReg
****************************************************************************//**
*
* \brief Sets the data unit's DATA register value.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param dataReg
* DATA register value
*
* \return
* Status of the operation
*
* \note The Smart I/O block must be disabled before calling this function.
*
*******************************************************************************/
cy_en_smartio_status_t Cy_SmartIO_SetDataReg(SMARTIO_PRT_Type* base, uint8_t dataReg)
{
    cy_en_smartio_status_t status = CY_SMARTIO_LOCKED;
    
    if(CY_SMARTIO_DISABLE == _FLD2VAL(SMARTIO_PRT_CTL_ENABLED, SMARTIO_PRT_CTL(base)))
    {
        SMARTIO_PRT_DATA(base) = dataReg;
        status = CY_SMARTIO_SUCCESS;
    }
    
    return status;
}



#if defined(__cplusplus)
}
#endif

/* [] END OF FILE */
