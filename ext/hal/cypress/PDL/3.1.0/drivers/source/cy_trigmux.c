/***************************************************************************//**
* \file cy_trigmux.c
* \version 1.20
*
* \brief Trigger mux API.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_trigmux.h"
#include "cy_device.h"


/** \cond internal */
#define CY_TRIGMUX_ACTIVATE                     (0x1UL)

#define CY_TRIGMUX_PARAM_1TO1_MASK              (PERI_V2_TR_CMD_OUT_SEL_Msk | CY_PERI_TR_CMD_GROUP_SEL_Msk | PERI_TR_1TO1_GR_V2_TR_CTL_TR_SEL_Msk)

#define CY_TRIGMUX_IS_TRIGTYPE_VALID(trigType)  (((trigType) == TRIGGER_TYPE_EDGE) || \
                                                 ((trigType) == TRIGGER_TYPE_LEVEL))

#define CY_TRIGMUX_V1_IS_CYCLES_VALID(cycles)   (CY_TRIGGER_INFINITE >= cycles)
#define CY_TRIGMUX_V2_IS_CYCLES_VALID(cycles)   ((CY_TRIGGER_DEACTIVATE == cycles) || \
                                                 (CY_TRIGGER_TWO_CYCLES == cycles) || \
                                                 (CY_TRIGGER_INFINITE   == cycles))
#define CY_TRIGMUX_IS_CYCLES_VALID(cycles)      ((CY_PERI_V1 && CY_TRIGMUX_V1_IS_CYCLES_VALID(cycles)) || \
                                                                CY_TRIGMUX_V2_IS_CYCLES_VALID(cycles))

#define CY_TRIGMUX_INTRIG_MASK                  (PERI_TR_CMD_GROUP_SEL_Msk | PERI_TR_GR_TR_OUT_CTL_TR_SEL_Msk)
#define CY_TRIGMUX_IS_INTRIG_VALID(inTrg)       (0UL == (inTrg & (uint32_t)~CY_TRIGMUX_INTRIG_MASK))

#define CY_TRIGMUX_OUTTRIG_MASK                 (PERI_TR_CMD_OUT_SEL_Msk | PERI_TR_CMD_GROUP_SEL_Msk | CY_PERI_TR_CTL_SEL_Msk)
#define CY_TRIGMUX_IS_OUTTRIG_VALID(outTrg)     ((0UL == (outTrg & (uint32_t)~CY_TRIGMUX_OUTTRIG_MASK)) && \
                                                 (0UL != (outTrg & PERI_TR_CMD_OUT_SEL_Msk)))

#define CY_TRIGMUX_TRIGLINE_MASK                (PERI_TR_CMD_OUT_SEL_Msk | CY_PERI_TR_CMD_GROUP_SEL_Msk | CY_PERI_TR_CTL_SEL_Msk)
#define CY_TRIGMUX_IS_TRIGLINE_VALID(trgLn)     (0U == (trgLn & (uint32_t)~CY_TRIGMUX_TRIGLINE_MASK))

#define CY_TRIGMUX_TR_CTL(outTrig)              (PERI_TR_GR_TR_CTL(_FLD2VAL(CY_PERI_TR_CMD_GROUP_SEL, outTrig), \
                                                                   _FLD2VAL(CY_PERI_TR_CTL_SEL,       outTrig)))
/** \endcond */


/*******************************************************************************
* Function Name: Cy_TrigMux_Connect
****************************************************************************//**
*
* Connects an input trigger source and output trigger.
*
* \param inTrig
* An input selection for the trigger mux.
* - Bit 30 should be cleared.
* - Bits 11:8 represent the trigger group selection.
*  For PSoC6-BLE2 devices: (todo: correct the user-facing family name)
* - Bits 6:0 select the output trigger number in the trigger group.
*  For PSoC6-2M devices: (todo: correct the user-facing family name)
* - Bits 7:0 select the output trigger number in the trigger group.
*
* \param outTrig
* The output of the trigger mux. This refers to the consumer of the trigger mux.
* - Bit 30 should be set.
* - Bits 11:8 represent the trigger group selection.
*  For PSoC6-BLE2 devices: (todo: correct the user-facing family name)
* - Bits 6:0 select the output trigger number in the trigger group.
*  For PSoC6-2M devices: (todo: correct the user-facing family name)
* - Bits 7:0 select the output trigger number in the trigger group.
*
* \param invert
* - true: The output trigger is inverted.
* - false: The output trigger is not inverted.
*
* \param trigType The trigger signal type.
* - TRIGGER_TYPE_EDGE: The trigger is synchronized to the consumer blocks clock
*   and a two-cycle pulse is generated on this clock.
* - TRIGGER_TYPE_LEVEL: The trigger is a simple level output.
*
* \return A status
* - CY_TRIGMUX_SUCCESS: The connection is made successfully.
* - CY_TRIGMUX_BAD_PARAM: Some parameter is invalid.
*
* \funcusage
* \snippet trigmux\1.20\snippet\main.c snippet_Cy_TrigMux_Connect
*  
*******************************************************************************/
cy_en_trigmux_status_t Cy_TrigMux_Connect(uint32_t inTrig, uint32_t outTrig, bool invert, en_trig_type_t trigType)
{
    cy_en_trigmux_status_t retVal = CY_TRIGMUX_BAD_PARAM;
    CY_ASSERT_L3(CY_TRIGMUX_IS_TRIGTYPE_VALID(trigType));
    CY_ASSERT_L2(CY_TRIGMUX_IS_INTRIG_VALID(inTrig));
    CY_ASSERT_L2(CY_TRIGMUX_IS_OUTTRIG_VALID(outTrig));

    /* inTrig and outTrig should be in the same group */
    if ((inTrig & PERI_TR_CMD_GROUP_SEL_Msk) == (outTrig & PERI_TR_CMD_GROUP_SEL_Msk))
    {
        uint32_t interruptState = Cy_SysLib_EnterCriticalSection();

        CY_TRIGMUX_TR_CTL(outTrig) = (CY_TRIGMUX_TR_CTL(outTrig) &
                                      (uint32_t)~(PERI_TR_GR_TR_OUT_CTL_TR_SEL_Msk |
                                                  PERI_TR_GR_TR_OUT_CTL_TR_INV_Msk |
                                                  PERI_TR_GR_TR_OUT_CTL_TR_EDGE_Msk)) |
                                        (_VAL2FLD(PERI_TR_GR_TR_OUT_CTL_TR_SEL, inTrig) |
                                        _BOOL2FLD(PERI_TR_GR_TR_OUT_CTL_TR_INV, invert) |
                                         _VAL2FLD(PERI_TR_GR_TR_OUT_CTL_TR_EDGE, trigType));

        Cy_SysLib_ExitCriticalSection(interruptState);

        retVal = CY_TRIGMUX_SUCCESS;
    }

    return retVal;
}


/*******************************************************************************
* Function Name: Cy_TrigMux_Select
****************************************************************************//**
*
* Enables and configures the specified 1-to-1 trigger line.
*
* \param outTrig
* The 1to1 trigger line.
* - Bit 30 should be set.
* - Bit 29 should be set.
* - Bits 11:8 represent the trigger group selection.
* - Bits 7:0 select the trigger line number in the trigger group.
*
* \param invert
* - true: The trigger signal is inverted.
* - false: The trigger signal is not inverted.
*
* \param trigType The trigger signal type.
* - TRIGGER_TYPE_EDGE: The trigger is synchronized to the consumer blocks clock
*   and a two-cycle pulse is generated on this clock.
* - TRIGGER_TYPE_LEVEL: The trigger is a simple level output.
*
* \return A status
* - CY_TRIGMUX_SUCCESS: The selection is made successfully.
* - CY_TRIGMUX_BAD_PARAM: Some parameter is invalid.
*
* \funcusage
* \snippet trigmux\1.20\snippet\main.c snippet_Cy_TrigMux_Select
*  
*******************************************************************************/
cy_en_trigmux_status_t Cy_TrigMux_Select(uint32_t outTrig, bool invert, en_trig_type_t trigType)
{
    cy_en_trigmux_status_t retVal = CY_TRIGMUX_BAD_PARAM;
    
    CY_ASSERT_L3(CY_TRIGMUX_IS_TRIGTYPE_VALID(trigType));

    if (!CY_PERI_V1)
    {
        uint32_t interruptState;

        interruptState = Cy_SysLib_EnterCriticalSection();

        CY_TRIGMUX_TR_CTL(outTrig) = (CY_TRIGMUX_TR_CTL(outTrig) &
                                      (uint32_t)~(PERI_TR_1TO1_GR_V2_TR_CTL_TR_SEL_Msk |
                                                  PERI_TR_1TO1_GR_V2_TR_CTL_TR_INV_Msk |
                                                  PERI_TR_1TO1_GR_V2_TR_CTL_TR_EDGE_Msk)) |
                                        (_VAL2FLD(PERI_TR_1TO1_GR_V2_TR_CTL_TR_SEL, 1UL) |
                                        _BOOL2FLD(PERI_TR_1TO1_GR_V2_TR_CTL_TR_INV, invert) |
                                         _VAL2FLD(PERI_TR_1TO1_GR_V2_TR_CTL_TR_EDGE, trigType));

        Cy_SysLib_ExitCriticalSection(interruptState);

        retVal = CY_TRIGMUX_SUCCESS;
    }

    return retVal;
}


/*******************************************************************************
* Function Name: Cy_TrigMux_Deselect
****************************************************************************//**
*
* Disables the specified 1-to-1 trigger line.
*
* \param outTrig
* The 1to1 trigger line.
* - Bit 30 should be set.
* - Bit 29 should be set.
* - Bits 11:8 represent the trigger group selection.
* - Bits 7:0 select the trigger line number in the trigger group.
*
* \return A status
* - CY_TRIGMUX_SUCCESS: The deselection is made successfully.
* - CY_TRIGMUX_BAD_PARAM: Some parameter is invalid.
*
* \funcusage
* \snippet trigmux\1.20\snippet\main.c snippet_Cy_TrigMux_Deselect
*  
*******************************************************************************/
cy_en_trigmux_status_t Cy_TrigMux_Deselect(uint32_t outTrig)
{
    cy_en_trigmux_status_t retVal = CY_TRIGMUX_BAD_PARAM;

    if (!CY_PERI_V1)
    {
        uint32_t interruptState;

        interruptState = Cy_SysLib_EnterCriticalSection();

        CY_TRIGMUX_TR_CTL(outTrig) &= (uint32_t)~(PERI_TR_1TO1_GR_V2_TR_CTL_TR_SEL_Msk |
                                                  PERI_TR_1TO1_GR_V2_TR_CTL_TR_INV_Msk |
                                                  PERI_TR_1TO1_GR_V2_TR_CTL_TR_EDGE_Msk);

        Cy_SysLib_ExitCriticalSection(interruptState);

        retVal = CY_TRIGMUX_SUCCESS;
    }

    return retVal;
}


/*******************************************************************************
* Function Name: Cy_TrigMux_SetDebugFreeze
****************************************************************************//**
*
* Enables/disables the Debug Freeze feature for the specified trigger 
* multiplexer or 1-to-1 trigger line.
*
* \param outTrig
* The output of the trigger mux or dedicated 1-to-1 trigger line.
* - Bit 30 should be set.
* - Bit 29 represents whether it is a trigger multiplexer or 1-to-1 trigger line.
*    - 1: a 1-to-1 trigger line, 
*    - 0: a trigger multiplexer.
* - Bits 11:8 represent the trigger group selection.
* - Bits 7:0 select the output trigger number in the trigger group.
*
* \param enable
* - true: The Debug Freeze feature is enabled.
* - false: The Debug Freeze feature is Disabled.
*
* \return A status
* - CY_TRIGMUX_SUCCESS: The operation is made successfully.
* - CY_TRIGMUX_BAD_PARAM: The outTrig parameter is invalid.
*
* \funcusage
* \snippet trigmux\1.20\snippet\main.c snippet_Cy_TrigMux_SetDebugFreeze
*  
*******************************************************************************/
cy_en_trigmux_status_t Cy_TrigMux_SetDebugFreeze(uint32_t outTrig, bool enable)
{    
    cy_en_trigmux_status_t retVal = CY_TRIGMUX_BAD_PARAM;

    if (!CY_PERI_V1)
    {
        uint32_t interruptState;

        interruptState = Cy_SysLib_EnterCriticalSection();

        if (enable)
        {
            CY_TRIGMUX_TR_CTL(outTrig) |= PERI_TR_GR_V2_TR_CTL_DBG_FREEZE_EN_Msk;
        }
        else
        {
            CY_TRIGMUX_TR_CTL(outTrig) &= (uint32_t)~PERI_TR_GR_V2_TR_CTL_DBG_FREEZE_EN_Msk;
        }

        Cy_SysLib_ExitCriticalSection(interruptState);

        retVal = CY_TRIGMUX_SUCCESS;
    }

    return retVal;
}


/*******************************************************************************
* Function Name: Cy_TrigMux_SwTrigger
****************************************************************************//**
*
* This function generates a software trigger on an input trigger line. 
* All output triggers connected to this input trigger will be triggered. 
* The function also verifies that there is no activated trigger before 
* generating another activation. 
*
* \param trigLine
* The input of the trigger mux.
* - Bit 30 represents if the signal is an input/output. When this bit is set, 
*   the trigger activation is for an output trigger from the trigger multiplexer.
*   When this bit is reset, the trigger activation is for an input trigger to 
*   the trigger multiplexer.
* - Bits 11:8 represent the trigger group selection.
*  For PSoC6-BLE2 devices: (todo: correct the user-facing family name)
* - Bits 6:0 select the output trigger number in the trigger group.
*  For PSoC6-2M devices: (todo: correct the user-facing family name)
* - Bits 7:0 select the output trigger number in the trigger group.
*
* \param cycles
*  The number of "Clk_Peri" cycles during which the trigger remains activated.
* <table class="doxtable">
*   <tr><th>Device</th><th>Description</th></tr>
*   <tr>
*     <td>PSoC6-BLE2</td>
*     <td>The valid range of cycles is 1 ... 254.
*   Also there are special values:
*   - CY_TRIGGER_INFINITE - trigger remains activated untill user deactivates it by
*   calling this function with CY_TRIGGER_DEACTIVATE parameter.
*   - CY_TRIGGER_DEACTIVATE - it is used to deactivate the trigger activated by
*   calling this function with CY_TRIGGER_INFINITE parameter.</td>
*   </tr>
*   <tr>
*     <td>PSoC6-2M</td>
*     <td>The only valid value of cycles is 2.</td>
*   </tr>
* </table>
* 
* \return A status:
* - CY_TRIGMUX_SUCCESS: The trigger is successfully activated/deactivated.
* - CY_TRIGMUX_INVALID_STATE: The trigger is already activated/not active.
* - CY_TRIGMUX_BAD_PARAM: Some parameter is invalid.
*
* \funcusage
* \snippet trigmux\1.20\snippet\main.c snippet_Cy_TrigMux_SwTrigger
*
*******************************************************************************/
cy_en_trigmux_status_t Cy_TrigMux_SwTrigger(uint32_t trigLine, uint32_t cycles)
{
    cy_en_trigmux_status_t retVal = CY_TRIGMUX_INVALID_STATE;

    CY_ASSERT_L2(CY_TRIGMUX_IS_TRIGLINE_VALID(trigLine));
    CY_ASSERT_L2(CY_TRIGMUX_IS_CYCLES_VALID(cycles));

    if (CY_TRIGGER_DEACTIVATE != cycles)
    {
        /* Activate the trigger if it is not in the active state. */
        if (PERI_TR_CMD_ACTIVATE_Msk != (PERI_TR_CMD & PERI_TR_CMD_ACTIVATE_Msk))
        {

            uint32_t trCmd = (trigLine & (PERI_TR_CMD_TR_SEL_Msk |
                                          PERI_TR_CMD_OUT_SEL_Msk |
                                       CY_PERI_TR_CMD_GROUP_SEL_Msk)) |
                                          PERI_TR_CMD_ACTIVATE_Msk;
            
            retVal = CY_TRIGMUX_SUCCESS;
            
            if (CY_PERI_V1) /* mxperi_v1 */
            {
                PERI_TR_CMD = trCmd | _VAL2FLD(PERI_TR_CMD_COUNT, cycles);
            }
            else if (CY_TRIGGER_TWO_CYCLES == cycles) /* mxperi_v2 or later, 2 cycles pulse */
            {
                PERI_TR_CMD = trCmd | PERI_V2_TR_CMD_TR_EDGE_Msk;
            }
            else if (CY_TRIGGER_INFINITE == cycles) /* mxperi_v2 or later, infinite activating */
            {
                PERI_TR_CMD = trCmd;
            }
            else /* mxperi_v2 or later, invalid cycles value */
            {
                retVal = CY_TRIGMUX_BAD_PARAM;
            }
        }
    }
    else
    {
        /* Forcibly deactivate the trigger if it is in the active state.. */
        if (PERI_TR_CMD_ACTIVATE_Msk == (PERI_TR_CMD & PERI_TR_CMD_ACTIVATE_Msk))
        {
            PERI_TR_CMD = 0UL;

            retVal = CY_TRIGMUX_SUCCESS;
        }
    }

    return retVal;
}


/* [] END OF FILE */
