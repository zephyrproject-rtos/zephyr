/***************************************************************************//**
* \file cy_ctb.c
* \version 1.10
*
* \brief
* Provides the public functions for the CTB driver.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_ctb.h"

#ifdef CY_IP_MXS40PASS_CTB

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************
*       Fast Config Selections
***************************************/
const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Unused =
{
    /*.oa0Power        */ CY_CTB_POWER_OFF,
    /*.oa0Mode         */ CY_CTB_MODE_OPAMP1X,
    /*.oa0SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Comp =
{
    /*.oa0Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa0Mode         */ CY_CTB_MODE_COMP,
    /*.oa0SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Opamp1x =
{
    /*.oa0Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa0Mode         */ CY_CTB_MODE_OPAMP1X,
    /*.oa0SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Opamp10x =
{
    /*.oa0Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa0Mode         */ CY_CTB_MODE_OPAMP10X,
    /*.oa0SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Diffamp =
{
    /*.oa0Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa0Mode         */ CY_CTB_MODE_OPAMP10X,
    /*.oa0SwitchCtrl   */ (uint32_t) CY_CTB_SW_OA0_POS_PIN0_MASK | (uint32_t) CY_CTB_SW_OA0_NEG_PIN1_MASK,
    /*.ctdSwitchCtrl   */ (uint32_t) CY_CTB_SW_CTD_CHOLD_OA0_POS_ISOLATE_MASK,
};

const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Vdac_Out =
{
    /*.oa0Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa0Mode         */ CY_CTB_MODE_OPAMP10X,
    /*.oa0SwitchCtrl   */ (uint32_t) CY_CTB_SW_OA0_NEG_OUT_MASK | (uint32_t) CY_CTB_SW_OA0_OUT_SHORT_1X_10X_MASK,
    /*.ctdSwitchCtrl   */ (uint32_t) CY_CTB_SW_CTD_OUT_CHOLD_MASK | (uint32_t) CY_CTB_SW_CTD_CHOLD_OA0_POS_MASK,
};

const cy_stc_ctb_fast_config_oa0_t Cy_CTB_Fast_Opamp0_Vdac_Out_SH =
{
    /*.oa0Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa0Mode         */ CY_CTB_MODE_OPAMP10X,
    /*.oa0SwitchCtrl   */ (uint32_t) CY_CTB_SW_OA0_NEG_OUT_MASK | (uint32_t) CY_CTB_SW_OA0_OUT_SHORT_1X_10X_MASK,
    /*.ctdSwitchCtrl   */ (uint32_t) CY_CTB_SW_CTD_OUT_CHOLD_MASK | (uint32_t) CY_CTB_SW_CTD_CHOLD_OA0_POS_MASK | (uint32_t) CY_CTB_SW_CTD_CHOLD_CONNECT_MASK,
};

const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Unused =
{
    /*.oa1Power        */ CY_CTB_POWER_OFF,
    /*.oa1Mode         */ CY_CTB_MODE_OPAMP1X,
    /*.oa1SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Comp =
{
    /*.oa1Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa1Mode         */ CY_CTB_MODE_COMP,
    /*.oa1SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Opamp1x =
{
    /*.oa1Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa1Mode         */ CY_CTB_MODE_OPAMP1X,
    /*.oa1SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Opamp10x =
{
    /*.oa1Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa1Mode         */ CY_CTB_MODE_OPAMP10X,
    /*.oa1SwitchCtrl   */ CY_CTB_DEINIT,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Diffamp =
{
    /*.oa1Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa1Mode         */ CY_CTB_MODE_OPAMP10X,
    /*.oa1SwitchCtrl   */ (uint32_t) CY_CTB_SW_OA1_POS_PIN7_MASK | (uint32_t) CY_CTB_SW_OA1_NEG_PIN4_MASK,
    /*.ctdSwitchCtrl   */ CY_CTB_DEINIT,
};

const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Vdac_Ref_Aref =
{
    /*.oa1Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa1Mode         */ CY_CTB_MODE_OPAMP1X,
    /*.oa1SwitchCtrl   */ (uint32_t) CY_CTB_SW_OA1_NEG_OUT_MASK | (uint32_t) CY_CTB_SW_OA1_POS_AREF_MASK,
    /*.ctdSwitchCtrl   */ (uint32_t) CY_CTB_SW_CTD_REF_OA1_OUT_MASK,
};

const cy_stc_ctb_fast_config_oa1_t Cy_CTB_Fast_Opamp1_Vdac_Ref_Pin5 =
{
    /*.oa1Power        */ CY_CTB_POWER_MEDIUM,
    /*.oa1Mode         */ CY_CTB_MODE_OPAMP1X,
    /*.oa1SwitchCtrl   */ (uint32_t) CY_CTB_SW_OA1_NEG_OUT_MASK | (uint32_t) CY_CTB_SW_OA1_POS_PIN5_MASK,
    /*.ctdSwitchCtrl   */ (uint32_t) CY_CTB_SW_CTD_REF_OA1_OUT_MASK,
};

/*******************************************************************************
* Function Name: Cy_CTB_Init
****************************************************************************//**
*
* Initialize or restore the CTB and both opamps according to the
* provided settings. Parameters are usually set only once, at initialization.
*
* \param base
* Pointer to structure describing registers
*
* \param config
* Pointer to structure containing configuration data for entire CTB
*
* \return
* Status of initialization, \ref CY_CTB_SUCCESS or \ref CY_CTB_BAD_PARAM
*
* \funcusage
*
* The following code snippet configures Opamp0 as a comparator
* and Opamp1 as an opamp follower with 10x drive. The terminals
* are routed to external pins by closing the switches shown.
*
* \image html ctb_init_funcusage.png
* \image latex ctb_init_funcusage.png
*
* \snippet ctb_sut_01.cydsn/main_cm0p.c SNIPPET_CTBINIT
*
*******************************************************************************/
cy_en_ctb_status_t Cy_CTB_Init(CTBM_Type *base, const cy_stc_ctb_config_t *config)
{
    CY_ASSERT_L1(NULL != base);
    CY_ASSERT_L1(NULL != config);

    cy_en_ctb_status_t result;

    if ((NULL == base) || (NULL == config))
    {
       result = CY_CTB_BAD_PARAM;
    }
    else
    {
        CY_ASSERT_L3(CY_CTB_DEEPSLEEP(config->deepSleep));

        /* Enum checks for Opamp0 config */
        CY_ASSERT_L3(CY_CTB_OAPOWER(config->oa0Power));
        CY_ASSERT_L3(CY_CTB_OAMODE(config->oa0Mode));
        CY_ASSERT_L3(CY_CTB_OAPUMP(config->oa0Pump));
        CY_ASSERT_L3(CY_CTB_COMPEDGE(config->oa0CompEdge));
        CY_ASSERT_L3(CY_CTB_COMPLEVEL(config->oa0CompLevel));
        CY_ASSERT_L3(CY_CTB_COMPBYPASS(config->oa0CompBypass));
        CY_ASSERT_L3(CY_CTB_COMPHYST(config->oa0CompHyst));

        /* Enum checks for Opamp0 config */
        CY_ASSERT_L3(CY_CTB_OAPOWER(config->oa1Power));
        CY_ASSERT_L3(CY_CTB_OAMODE(config->oa1Mode));
        CY_ASSERT_L3(CY_CTB_OAPUMP(config->oa1Pump));
        CY_ASSERT_L3(CY_CTB_COMPEDGE(config->oa1CompEdge));
        CY_ASSERT_L3(CY_CTB_COMPLEVEL(config->oa1CompLevel));
        CY_ASSERT_L3(CY_CTB_COMPBYPASS(config->oa1CompBypass));
        CY_ASSERT_L3(CY_CTB_COMPHYST(config->oa1CompHyst));

        /* Boundary checks for analog routing switch masks */
        CY_ASSERT_L2(CY_CTB_OA0SWITCH(config->oa0SwitchCtrl));
        CY_ASSERT_L2(CY_CTB_OA1SWITCH(config->oa1SwitchCtrl));
        CY_ASSERT_L2(CY_CTB_CTDSWITCH(config->ctdSwitchCtrl));

        CTBM_CTB_CTRL(base) = (uint32_t) config->deepSleep;
        CTBM_OA_RES0_CTRL(base) = (uint32_t) config->oa0Power \
                             | (uint32_t) config->oa0Mode \
                             | (uint32_t) config->oa0Pump \
                             | (uint32_t) config->oa0CompEdge \
                             | (uint32_t) config->oa0CompLevel \
                             | (uint32_t) config->oa0CompBypass \
                             | (uint32_t) config->oa0CompHyst \
                             | ((CY_CTB_MODE_OPAMP1X == config->oa0Mode) ? CY_CTB_OPAMP_BOOST_ENABLE : CY_CTB_OPAMP_BOOST_DISABLE);

        CTBM_OA_RES1_CTRL(base) = (uint32_t) config->oa1Power \
                             | (uint32_t) config->oa1Mode \
                             | (uint32_t) config->oa1Pump \
                             | (uint32_t) config->oa1CompEdge \
                             | (uint32_t) config->oa1CompLevel \
                             | (uint32_t) config->oa1CompBypass \
                             | (uint32_t) config->oa1CompHyst \
                             | ((CY_CTB_MODE_OPAMP1X == config->oa1Mode) ? CY_CTB_OPAMP_BOOST_ENABLE : CY_CTB_OPAMP_BOOST_DISABLE);

        CTBM_INTR_MASK(base) = (config->oa0CompIntrEn ? CTBM_INTR_MASK_COMP0_MASK_Msk : CY_CTB_DEINIT) \
                          | (config->oa1CompIntrEn ? CTBM_INTR_MASK_COMP1_MASK_Msk : CY_CTB_DEINIT);

        CTBM_OA0_COMP_TRIM(base) = (uint32_t) ((config->oa0Mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);
        CTBM_OA1_COMP_TRIM(base) = (uint32_t) ((config->oa1Mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);

        if (config->configRouting)
        {
            CTBM_OA0_SW(base) = config->oa0SwitchCtrl;
            CTBM_OA1_SW(base) = config->oa1SwitchCtrl;
            CTBM_CTD_SW(base) = config->ctdSwitchCtrl;
        }

        result = CY_CTB_SUCCESS;
    }

    return result;
}

/*******************************************************************************
* Function Name: Cy_CTB_OpampInit
****************************************************************************//**
*
* Initialize each opamp separately without impacting analog routing.
* Intended for use by automatic analog routing and configuration tools
* to configure each opamp without having to integrate the settings with
* those of the other opamp first.
*
* Can also be used to configure both opamps to have the same settings.
*
* \param base
* Pointer to structure describing registers
*
* \param opampNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \param config
* Pointer to structure containing configuration data
*
* \return
* Status of initialization, \ref CY_CTB_SUCCESS or \ref CY_CTB_BAD_PARAM
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm0p.c SNIPPET_OPAMPINIT
*
*******************************************************************************/
cy_en_ctb_status_t Cy_CTB_OpampInit(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, const cy_stc_ctb_opamp_config_t *config)
{
    CY_ASSERT_L1(NULL != base);
    CY_ASSERT_L1(NULL != config);

    cy_en_ctb_status_t result;
    uint32_t oaResCtrl;

    if ((NULL == base) || (NULL == config))
    {
       result = CY_CTB_BAD_PARAM;
    }
    else
    {
        CY_ASSERT_L3(CY_CTB_OPAMPNUM(opampNum));
        CY_ASSERT_L3(CY_CTB_DEEPSLEEP(config->deepSleep));
        CY_ASSERT_L3(CY_CTB_OAPOWER(config->oaPower));
        CY_ASSERT_L3(CY_CTB_OAMODE(config->oaMode));
        CY_ASSERT_L3(CY_CTB_OAPUMP(config->oaPump));
        CY_ASSERT_L3(CY_CTB_COMPEDGE(config->oaCompEdge));
        CY_ASSERT_L3(CY_CTB_COMPLEVEL(config->oaCompLevel));
        CY_ASSERT_L3(CY_CTB_COMPBYPASS(config->oaCompBypass));
        CY_ASSERT_L3(CY_CTB_COMPHYST(config->oaCompHyst));

        CTBM_CTB_CTRL(base) = (uint32_t) config->deepSleep;

        /* The two opamp control registers are symmetrical */
        oaResCtrl = (uint32_t) config->oaPower \
                    | (uint32_t) config->oaMode \
                    | (uint32_t) config->oaPump \
                    | (uint32_t) config->oaCompEdge \
                    | (uint32_t) config->oaCompLevel \
                    | (uint32_t) config->oaCompBypass \
                    | (uint32_t) config->oaCompHyst \
                    | ((CY_CTB_MODE_OPAMP1X == config->oaMode) ? CY_CTB_OPAMP_BOOST_ENABLE : CY_CTB_OPAMP_BOOST_DISABLE);

        if ((opampNum == CY_CTB_OPAMP_0) || (opampNum == CY_CTB_OPAMP_BOTH))
        {
            CTBM_OA_RES0_CTRL(base) = oaResCtrl;
            CTBM_OA0_COMP_TRIM(base) = (uint32_t) ((config->oaMode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);

            /* The INTR_MASK register is shared between the two opamps */
            CTBM_INTR_MASK(base) |= (config->oaCompIntrEn ? CTBM_INTR_MASK_COMP0_MASK_Msk : CY_CTB_DEINIT);
        }

        if ((opampNum == CY_CTB_OPAMP_1) || (opampNum == CY_CTB_OPAMP_BOTH))
        {
            CTBM_OA_RES1_CTRL(base) = oaResCtrl;
            CTBM_OA1_COMP_TRIM(base) = (uint32_t) ((config->oaMode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);

            /* The INTR_MASK register is shared between the two opamps */
            CTBM_INTR_MASK(base) |= (config->oaCompIntrEn ? CTBM_INTR_MASK_COMP1_MASK_Msk : CY_CTB_DEINIT);
        }

        result = CY_CTB_SUCCESS;
    }

    return result;
}

/*******************************************************************************
* Function Name: Cy_CTB_DeInit
****************************************************************************//**
*
* Reset CTB registers back to power on reset defaults.
*
* \param base
* Pointer to structure describing registers
*
* \param deInitRouting
* If true, all analog routing switches are reset to their default state.
* If false, analog switch registers are untouched.
*
* \return
* Status of initialization, \ref CY_CTB_SUCCESS or \ref CY_CTB_BAD_PARAM
*
*******************************************************************************/
cy_en_ctb_status_t Cy_CTB_DeInit(CTBM_Type *base, bool deInitRouting)
{
    CY_ASSERT_L1(NULL != base);

    cy_en_ctb_status_t result;

    if (NULL == base)
    {
        result = CY_CTB_BAD_PARAM;
    }
    else
    {
        CTBM_CTB_CTRL(base)          = CY_CTB_DEINIT;
        CTBM_OA_RES0_CTRL(base)      = CY_CTB_DEINIT;
        CTBM_OA_RES1_CTRL(base)      = CY_CTB_DEINIT;
        CTBM_INTR_MASK(base)         = CY_CTB_DEINIT;

        if (deInitRouting)
        {
            CTBM_OA0_SW_CLEAR(base)   = CY_CTB_DEINIT_OA0_SW;
            CTBM_OA1_SW_CLEAR(base)   = CY_CTB_DEINIT_OA1_SW;
            CTBM_CTD_SW_CLEAR(base)   = CY_CTB_DEINIT_CTD_SW;
        }

        result                  = CY_CTB_SUCCESS;
    }

    return result;
}

/*******************************************************************************
* Function Name: Cy_CTB_FastInit
****************************************************************************//**
*
* Initialize each opamp of the CTB to one of the common use modes.
*
* This function provides a quick and easy method of configuring the CTB
* using pre-defined configurations.
* Only routing switches required for the selected mode are configured, leaving final input and output connections
* to the user.
* Additional use modes that relate to the \ref group_ctdac "CTDAC"
* are provided to support easy configuration of the CTDAC output buffer and input
* reference buffer.
*
* The fast configuration structures define the opamp power, mode, and routing.
* This function sets the other configuration options of the CTB to:
*   - .deepSleep    = CY_CTB_DEEPSLEEP_DISABLE
*   - .oaPump       = \ref CY_CTB_PUMP_ENABLE
*   - .oaCompEdge   = \ref CY_CTB_COMP_EDGE_BOTH
*   - .oaCompLevel  = \ref CY_CTB_COMP_DSI_TRIGGER_OUT_LEVEL
*   - .oaCompBypass = \ref CY_CTB_COMP_BYPASS_SYNC
*   - .oaCompHyst   = \ref CY_CTB_COMP_HYST_10MV
*   - .oaCompIntrEn = true

* \param base
* Pointer to structure describing registers
*
* \param config0
* Pointer to structure containing configuration data for quick initialization
* of Opamp0. Defined your own or use one of the provided structures:
* - \ref Cy_CTB_Fast_Opamp0_Unused
* - \ref Cy_CTB_Fast_Opamp0_Comp
* - \ref Cy_CTB_Fast_Opamp0_Opamp1x
* - \ref Cy_CTB_Fast_Opamp0_Opamp10x
* - \ref Cy_CTB_Fast_Opamp0_Diffamp
* - \ref Cy_CTB_Fast_Opamp0_Vdac_Out
* - \ref Cy_CTB_Fast_Opamp0_Vdac_Out_SH
*
* \param config1
* Pointer to structure containing configuration data for quick initialization
* of Opamp1. Defined your own or use one of the provided structures:
* - \ref Cy_CTB_Fast_Opamp1_Unused
* - \ref Cy_CTB_Fast_Opamp1_Comp
* - \ref Cy_CTB_Fast_Opamp1_Opamp1x
* - \ref Cy_CTB_Fast_Opamp1_Opamp10x
* - \ref Cy_CTB_Fast_Opamp1_Diffamp
* - \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Aref
* - \ref Cy_CTB_Fast_Opamp1_Vdac_Ref_Pin5
*
* \return
* Status of initialization, \ref CY_CTB_SUCCESS or \ref CY_CTB_BAD_PARAM
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm0p.c SNIPPET_FASTINIT
*
*******************************************************************************/
cy_en_ctb_status_t Cy_CTB_FastInit(CTBM_Type *base, const cy_stc_ctb_fast_config_oa0_t *config0, const cy_stc_ctb_fast_config_oa1_t *config1)
{
    CY_ASSERT_L1(NULL != base);
    CY_ASSERT_L1(NULL != config0);
    CY_ASSERT_L1(NULL != config1);

    cy_en_ctb_status_t result;

    if ((NULL == base) || (NULL == config0) || (NULL == config1))
    {
        result = CY_CTB_BAD_PARAM;
    }
    else
    {
        /* Enum and boundary checks for config0 */
        CY_ASSERT_L3(CY_CTB_OAPOWER(config0->oa0Power));
        CY_ASSERT_L3(CY_CTB_OAMODE(config0->oa0Mode));
        CY_ASSERT_L2(CY_CTB_OA0SWITCH(config0->oa0SwitchCtrl));
        CY_ASSERT_L2(CY_CTB_CTDSWITCH(config0->ctdSwitchCtrl));

        /* Enum and boundary checks for config1 */
        CY_ASSERT_L3(CY_CTB_OAPOWER(config1->oa1Power));
        CY_ASSERT_L3(CY_CTB_OAMODE(config1->oa1Mode));
        CY_ASSERT_L2(CY_CTB_OA1SWITCH(config1->oa1SwitchCtrl));
        CY_ASSERT_L2(CY_CTB_CTDSWITCH(config1->ctdSwitchCtrl));

        CTBM_CTB_CTRL(base) = (uint32_t) CY_CTB_DEEPSLEEP_DISABLE;

        CTBM_OA_RES0_CTRL(base) = (uint32_t) config0->oa0Power \
                             | (uint32_t) config0->oa0Mode \
                             | (uint32_t) CY_CTB_PUMP_ENABLE \
                             | (uint32_t) CY_CTB_COMP_EDGE_BOTH \
                             | (uint32_t) CY_CTB_COMP_DSI_TRIGGER_OUT_LEVEL \
                             | (uint32_t) CY_CTB_COMP_BYPASS_SYNC \
                             | (uint32_t) CY_CTB_COMP_HYST_10MV \
                             | ((CY_CTB_MODE_OPAMP1X == config0->oa0Mode) ? CY_CTB_OPAMP_BOOST_ENABLE : CY_CTB_OPAMP_BOOST_DISABLE);

        CTBM_OA_RES1_CTRL(base) = (uint32_t) config1->oa1Power \
                             | (uint32_t) config1->oa1Mode \
                             | (uint32_t) CY_CTB_PUMP_ENABLE \
                             | (uint32_t) CY_CTB_COMP_EDGE_BOTH \
                             | (uint32_t) CY_CTB_COMP_DSI_TRIGGER_OUT_LEVEL \
                             | (uint32_t) CY_CTB_COMP_BYPASS_SYNC \
                             | (uint32_t) CY_CTB_COMP_HYST_10MV \
                             | ((CY_CTB_MODE_OPAMP1X == config1->oa1Mode) ? CY_CTB_OPAMP_BOOST_ENABLE : CY_CTB_OPAMP_BOOST_DISABLE);

        CTBM_INTR_MASK(base) = CTBM_INTR_MASK_COMP0_MASK_Msk | CTBM_INTR_MASK_COMP1_MASK_Msk;

        CTBM_OA0_COMP_TRIM(base) = (uint32_t) ((config0->oa0Mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);
        CTBM_OA1_COMP_TRIM(base) = (uint32_t) ((config1->oa1Mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);

        CTBM_OA0_SW(base)   = config0->oa0SwitchCtrl;
        CTBM_OA1_SW(base)   = config1->oa1SwitchCtrl;
        CTBM_CTD_SW(base)   = config0->ctdSwitchCtrl | config1->ctdSwitchCtrl;

        result = CY_CTB_SUCCESS;
    }

    return result;
}

/*******************************************************************************
* Function Name: Cy_CTB_SetCurrentMode
****************************************************************************//**
*
* High level function to configure the current modes of the opamps.
* This function configures all opamps of the CTB to the same current mode.
* These modes are differentiated by the reference current level, the opamp
* input range, and the Deep Sleep mode operation.
*
*   - The reference current level is set using \ref Cy_CTB_SetIptatLevel
*   - When 1 uA current level is used in Deep Sleep,
*       - All generators in the AREF must be enabled in Deep Sleep. That is,
*           \ref Cy_SysAnalog_SetDeepSleepMode is called with CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF.
*   - When 100 nA current level is used,
*       - \ref Cy_CTB_EnableRedirect is called to route the AREF IPTAT reference
*           to the opamp IZTAT and disable the opamps IPTAT.
*       - The IPTAT generator is enabled in Deep Sleep. That is,
*           \ref Cy_SysAnalog_SetDeepSleepMode is called with CY_SYSANALOG_DEEPSLEEP_IPTAT_2
*           unless it is already configured for CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF.
*
* \note
* The IPTAT level is a chip wide configuration so multiple
* opamps cannot operate at different IPTAT levels.
* When calling \ref Cy_CTB_SetCurrentMode for a CTB instance on the device,
* it should be called for all other CTB instances as well.
*
* <table class="doxtable">
*   <tr>
*     <th>Current Mode</th>
*     <th>IPTAT Level</th>
*     <th>Input Range</th>
*     <th>Deep Sleep Operation</th>
*   </tr>
*   <tr>
*     <td>\ref CY_CTB_CURRENT_HIGH_ACTIVE</td>
*     <td>1 uA</td>
*     <td>Rail-to-Rail (charge pump enabled)</td>
*     <td>Disabled in Deep Sleep</td>
*   </tr>
*   <tr>
*     <td>\ref CY_CTB_CURRENT_HIGH_ACTIVE_DEEPSLEEP</td>
*     <td>1 uA</td>
*     <td>0 - VDDA-1.5 V (charge pump disabled)</td>
*     <td>Enabled in Deep Sleep</td>
*   </tr>
*   <tr>
*     <td>\ref CY_CTB_CURRENT_LOW_ACTIVE_DEEPSLEEP</td>
*     <td>100 nA</td>
*     <td>0 - VDDA-1.5 V (charge pump disabled)</td>
*     <td>Enabled in Deep Sleep</td>
*   </tr>
* </table>
*
* \note
* The output range of the opamp is 0.2 V to VDDA - 0.2 V (depending on output load).
*
* \param base
* Pointer to structure describing registers
*
* \param currentMode
* Current mode selection
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_CURRENT_MODE
*
*******************************************************************************/
void Cy_CTB_SetCurrentMode(CTBM_Type *base, cy_en_ctb_current_mode_t currentMode)
{
    CY_ASSERT_L3(CY_CTB_CURRENTMODE(currentMode));

    cy_en_sysanalog_deep_sleep_t arefDeepSleep;

    switch(currentMode)
    {
    case CY_CTB_CURRENT_HIGH_ACTIVE:

        /* Does not disable AREF for Deep Sleep in case the AREF is used by other blocks */

        /* Use a 1 uA IPTAT level and disable redirection */
        Cy_CTB_SetIptatLevel(CY_CTB_IPTAT_NORMAL);
        Cy_CTB_DisableRedirect();

        /* Disable Deep Sleep mode for the CTB - not opamp specific */
        Cy_CTB_SetDeepSleepMode(base, CY_CTB_DEEPSLEEP_DISABLE);

        /* Enable Opamp0 pump */
        CTBM_OA_RES0_CTRL(base) |= CTBM_OA_RES0_CTRL_OA0_PUMP_EN_Msk;

        /* Enable Opamp1 pump */
        CTBM_OA_RES1_CTRL(base) |= CTBM_OA_RES1_CTRL_OA1_PUMP_EN_Msk;

        break;
    case CY_CTB_CURRENT_HIGH_ACTIVE_DEEPSLEEP:

        /* All generators (IPTAT, IZTAT, and VREF) of the AREF block must be enabled for Deep Sleep */
        Cy_SysAnalog_SetDeepSleepMode(CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF);

        /* Use a 1 uA IPTAT level and disable redirection */
        Cy_CTB_SetIptatLevel(CY_CTB_IPTAT_NORMAL);
        Cy_CTB_DisableRedirect();

        /* Enable Deep Sleep mode for the CTB - not opamp specific */
        Cy_CTB_SetDeepSleepMode(base, CY_CTB_DEEPSLEEP_ENABLE);

        /* Disable Opamp0 pump */
        CTBM_OA_RES0_CTRL(base) &= ~CTBM_OA_RES0_CTRL_OA0_PUMP_EN_Msk;

        /* Disable Opamp1 pump */
        CTBM_OA_RES1_CTRL(base) &= ~CTBM_OA_RES1_CTRL_OA1_PUMP_EN_Msk;

        break;
    case CY_CTB_CURRENT_LOW_ACTIVE_DEEPSLEEP:
    default:

        /* The AREF IPTAT output for the opamps must be enabled in Deep Sleep.
         * This means a minimum Deep Sleep mode setting of CY_SYSANALOG_DEEPSLEEP_IPTAT_2. */
        arefDeepSleep = Cy_SysAnalog_GetDeepSleepMode();
        if ((arefDeepSleep == CY_SYSANALOG_DEEPSLEEP_DISABLE) || (arefDeepSleep == CY_SYSANALOG_DEEPSLEEP_IPTAT_1))
        {
            Cy_SysAnalog_SetDeepSleepMode(CY_SYSANALOG_DEEPSLEEP_IPTAT_2);
        }

        /* Use a 100 nA IPTAT level and enable redirection */
        Cy_CTB_SetIptatLevel(CY_CTB_IPTAT_LOW);
        Cy_CTB_EnableRedirect();

        /* Enable Deep Sleep mode for the CTB - not opamp specific */
        Cy_CTB_SetDeepSleepMode(base, CY_CTB_DEEPSLEEP_ENABLE);

        /* Disable Opamp0 pump */
        CTBM_OA_RES0_CTRL(base) &= ~CTBM_OA_RES0_CTRL_OA0_PUMP_EN_Msk;

        /* Disable Opamp1 pump */
        CTBM_OA_RES1_CTRL(base) &= ~CTBM_OA_RES1_CTRL_OA1_PUMP_EN_Msk;
        break;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_SetDeepSleepMode
****************************************************************************//**
*
* Enable or disable the entire CTB (not per opamp) in Deep Sleep mode.
*
* If enabled, the AREF block must also be enabled for Deep Sleep to provide
* the needed reference currents to the opamps (see \ref Cy_SysAnalog_SetDeepSleepMode).
* Additionally, ensure that only internal CTB switches are used for routing.
* Switches on AMUXBUSA and AMUXBUSB are not enabled in Deep Sleep.
* See the \ref group_ctb_dependencies section for more information.
*
* \note
* In Deep Sleep mode, the charge pumps are disabled so the input
* range of the opamps is reduced to 0 V to VDDA - 1.5 V.
*
* \param base
* Pointer to structure describing registers
*
* \param deepSleep
* \ref CY_CTB_DEEPSLEEP_DISABLE or \ref CY_CTB_DEEPSLEEP_ENABLE from
* \ref cy_en_ctb_deep_sleep_t.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_DEEPSLEEP_MODE
*
*******************************************************************************/
void Cy_CTB_SetDeepSleepMode(CTBM_Type *base, cy_en_ctb_deep_sleep_t deepSleep)
{
    CY_ASSERT_L3(CY_CTB_DEEPSLEEP(deepSleep));

    uint32_t ctbCtrl;

    ctbCtrl = CTBM_CTB_CTRL(base) & ~CTBM_CTB_CTRL_DEEPSLEEP_ON_Msk;

    CTBM_CTB_CTRL(base) = ctbCtrl | (uint32_t)deepSleep;
}

/*******************************************************************************
* Function Name: Cy_CTB_SetOutputMode
****************************************************************************//**
*
* Set the opamp output mode to 1x drive, 10x drive, or comparator mode.
*
* \param base
* Pointer to structure describing registers
*
* \param opampNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \param mode
* Opamp mode selection. Select a value from \ref cy_en_ctb_mode_t.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_OUTPUT_MODE
*
*******************************************************************************/
void Cy_CTB_SetOutputMode(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, cy_en_ctb_mode_t mode)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(opampNum));
    CY_ASSERT_L3(CY_CTB_OAMODE(mode));

    uint32_t oaCtrlReg;

    if ((opampNum == CY_CTB_OPAMP_0) || (opampNum == CY_CTB_OPAMP_BOTH))
    {

        /* Clear the three affected bits before setting them */
        oaCtrlReg = CTBM_OA_RES0_CTRL(base) & ~(CTBM_OA_RES0_CTRL_OA0_DRIVE_STR_SEL_Msk | CTBM_OA_RES0_CTRL_OA0_COMP_EN_Msk | CTBM_OA_RES0_CTRL_OA0_BOOST_EN_Msk);
        CTBM_OA_RES0_CTRL(base) = oaCtrlReg | (uint32_t) mode | ((mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_BOOST_DISABLE : CY_CTB_OPAMP_BOOST_ENABLE);
        CTBM_OA0_COMP_TRIM(base) = (uint32_t) ((mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);
    }

    if ((opampNum == CY_CTB_OPAMP_1) || (opampNum == CY_CTB_OPAMP_BOTH))
    {
        oaCtrlReg = CTBM_OA_RES1_CTRL(base) & ~(CTBM_OA_RES1_CTRL_OA1_DRIVE_STR_SEL_Msk | CTBM_OA_RES1_CTRL_OA1_COMP_EN_Msk | CTBM_OA_RES1_CTRL_OA1_BOOST_EN_Msk);
        CTBM_OA_RES1_CTRL(base) = oaCtrlReg | (uint32_t) mode | ((mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_BOOST_DISABLE : CY_CTB_OPAMP_BOOST_ENABLE);
        CTBM_OA1_COMP_TRIM(base) = (uint32_t) ((mode == CY_CTB_MODE_OPAMP10X) ? CY_CTB_OPAMP_COMPENSATION_CAP_MAX: CY_CTB_OPAMP_COMPENSATION_CAP_MIN);
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_SetPower
****************************************************************************//**
*
* Configure the power level and charge pump for a specific opamp.
*
* At higher power levels, the opamp consumes more current but provides more
* gain bandwidth.
* Enabling the charge pump increases current but provides
* rail-to-rail input range. Disabling the charge pump limits the input range to
* VDDA - 1.5 V.
* See the device datasheet for performance specifications.
*
* \param base
* Pointer to structure describing registers
*
* \param opampNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \param power
* Power mode selection. Select a value from \ref cy_en_ctb_power_t.
*
* \param pump
* Enable or disable the charge pump. Select a value from \ref cy_en_ctb_pump_t.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_POWER
*
*******************************************************************************/
void Cy_CTB_SetPower(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, cy_en_ctb_power_t power, cy_en_ctb_pump_t pump)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(opampNum));
    CY_ASSERT_L3(CY_CTB_OAPOWER(power));
    CY_ASSERT_L3(CY_CTB_OAPUMP(pump));

    uint32_t oaCtrlReg;

    if ((opampNum == CY_CTB_OPAMP_0) || (opampNum == CY_CTB_OPAMP_BOTH))
    {

        /* Clear the two affected bits before setting them */
        oaCtrlReg = CTBM_OA_RES0_CTRL(base) & ~(CTBM_OA_RES0_CTRL_OA0_PWR_MODE_Msk | CTBM_OA_RES0_CTRL_OA0_PUMP_EN_Msk);
        CTBM_OA_RES0_CTRL(base) = oaCtrlReg | (uint32_t) power | (uint32_t) pump;
    }

    if ((opampNum == CY_CTB_OPAMP_1) || (opampNum == CY_CTB_OPAMP_BOTH))
    {
        oaCtrlReg = CTBM_OA_RES1_CTRL(base) & ~(CTBM_OA_RES1_CTRL_OA1_PWR_MODE_Msk | CTBM_OA_RES1_CTRL_OA1_PUMP_EN_Msk);
        CTBM_OA_RES1_CTRL(base) = oaCtrlReg | (uint32_t) power | (uint32_t) pump;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_DACSampleAndHold
****************************************************************************//**
*
* Perform sampling and holding of the CTDAC output.
* To perform a sample or a hold, a preparation step must first be executed to
* open the required switches. Because of this, each sample or hold
* requires three function calls:
*
* -# Call this function to prepare for a sample or hold
* -# Enable or disable the CTDAC output
* -# Call this function again to perform a sample or hold
*
* It takes 10 us to perform a sample of the CTDAC output to provide
* time for the capacitor to settle to the new value.
*
* \param base
* Pointer to structure describing registers
*
* \param mode
* Mode to prepare or perform a sample or hold, or disable the ability
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SAMPLE_CODE_SNIPPET
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_HOLD_CODE_SNIPPET
*
*******************************************************************************/
void Cy_CTB_DACSampleAndHold(CTBM_Type *base, cy_en_ctb_sample_hold_mode_t mode)
{
    CY_ASSERT_L3(CY_CTB_SAMPLEHOLD(mode));

    switch(mode)
    {
    case CY_CTB_SH_DISABLE:
        CTBM_CTD_SW_CLEAR(base) = (uint32_t) CY_CTB_SW_CTD_OUT_OA0_1X_OUT_MASK               /* Open COB switch */
                             | (uint32_t) CY_CTB_SW_CTD_CHOLD_OA0_POS_ISOLATE_MASK      /* Open CIS switch */
                             | (uint32_t) CY_CTB_SW_CTD_CHOLD_LEAKAGE_REDUCTION_MASK    /* Open ILR switch */
                             | (uint32_t) CY_CTB_SW_CTD_CHOLD_CONNECT_MASK;             /* Open CHD switch */
        CTBM_CTD_SW(base)       = (uint32_t) CY_CTB_SW_CTD_OUT_CHOLD_MASK;                   /* Close COS switch */
        break;
    case CY_CTB_SH_PREPARE_SAMPLE:
        CTBM_CTD_SW_CLEAR(base) = (uint32_t) CY_CTB_SW_CTD_OUT_OA0_1X_OUT_MASK               /* Open COB switch */
                             | (uint32_t) CY_CTB_SW_CTD_CHOLD_OA0_POS_ISOLATE_MASK      /* Open CIS switch */
                             | (uint32_t) CY_CTB_SW_CTD_CHOLD_LEAKAGE_REDUCTION_MASK;   /* Open ILR switch */
        CTBM_CTD_SW(base)       = (uint32_t) CY_CTB_SW_CTD_CHOLD_CONNECT_MASK;               /* Close CHD switch */
        break;
    case CY_CTB_SH_SAMPLE:
        CTBM_CTD_SW(base)       = (uint32_t) CY_CTB_SW_CTD_OUT_CHOLD_MASK;                   /* Close COS switch */
        break;
    case CY_CTB_SH_PREPARE_HOLD:
        CTBM_CTD_SW_CLEAR(base) = (uint32_t) CY_CTB_SW_CTD_OUT_CHOLD_MASK                    /* Open COS switch */
                             | (uint32_t) CY_CTB_SW_CTD_CHOLD_OA0_POS_ISOLATE_MASK;     /* Open CIS switch */
        break;
    case CY_CTB_SH_HOLD:
    default:
        CTBM_CTD_SW(base)       = (uint32_t) CY_CTB_SW_CTD_OUT_OA0_1X_OUT_MASK               /* Close COB switch to reduce leakage through COS switch */
                             | (uint32_t) CY_CTB_SW_CTD_CHOLD_LEAKAGE_REDUCTION_MASK;   /* Close ILR switch to reduce leakage through CIS switch */
        break;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_OpampSetOffset
****************************************************************************//**
*
* Override the CTB opamp offset factory trim.
* The trim is a six bit value and the MSB is a direction bit.
*
* <table class="doxtable">
*   <tr>
*     <th>Bit 5</th>
*     <th>Bits 4:0</th>
*     <th>Note</th>
*   </tr>
*   <tr>
*     <td>0</td>
*     <td>00000</td>
*     <td>Negative trim direction - minimum setting</td>
*   </tr>
*   <tr>
*     <td>0</td>
*     <td>11111</td>
*     <td>Negative trim direction - maximum setting</td>
*   </tr>
*   <tr>
*     <td>1</td>
*     <td>00000</td>
*     <td>Positive trim direction - minimum setting</td>
*   </tr>
*   <tr>
*     <td>1</td>
*     <td>11111</td>
*     <td>Positive trim direction - maximum setting</td>
*   </tr>
* </table>
*
* \param base
* Pointer to structure describing registers
*
* \param opampNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \param trim
* Trim value from 0 to 63
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_OFFSET_TRIM
*
*******************************************************************************/
void Cy_CTB_OpampSetOffset(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, uint32_t trim)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(opampNum));
    CY_ASSERT_L2(CY_CTB_TRIM(trim));

    if ((opampNum == CY_CTB_OPAMP_0) || (opampNum == CY_CTB_OPAMP_BOTH))
    {
        CTBM_OA0_OFFSET_TRIM(base) = (trim << CTBM_OA0_OFFSET_TRIM_OA0_OFFSET_TRIM_Pos) & CTBM_OA0_OFFSET_TRIM_OA0_OFFSET_TRIM_Msk;
    }

    if ((opampNum == CY_CTB_OPAMP_1) || (opampNum == CY_CTB_OPAMP_BOTH))
    {
        CTBM_OA1_OFFSET_TRIM(base) = (trim << CTBM_OA1_OFFSET_TRIM_OA1_OFFSET_TRIM_Pos) & CTBM_OA1_OFFSET_TRIM_OA1_OFFSET_TRIM_Msk;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_OpampGetOffset
****************************************************************************//**
*
* Return the current CTB opamp offset trim value.
*
* \param base
* Pointer to structure describing registers
*
* \param opampNum
* \ref CY_CTB_OPAMP_0 or \ref CY_CTB_OPAMP_1
*
* \return Offset trim value
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_GET_OFFSET_TRIM
*
*******************************************************************************/
uint32_t Cy_CTB_OpampGetOffset(const CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM_0_1(opampNum));

    uint32_t trimReg;

    if (opampNum == CY_CTB_OPAMP_0)
    {
        trimReg = CTBM_OA0_OFFSET_TRIM(base);
    }
    else
    {
        trimReg = CTBM_OA1_OFFSET_TRIM(base);
    }

    return trimReg;
}

/*******************************************************************************
* Function Name: Cy_CTB_OpampSetSlope
****************************************************************************//**
*
* Override the CTB opamp slope factory trim.
* The offset of the opamp will vary across temperature.
* This trim compensates for the slope of the offset across temperature.
* This compensation uses a bias current from the Analaog Reference block.
* To disable it, set the trim to 0.
*
* The trim is a six bit value and the MSB is a direction bit.
*
* <table class="doxtable">
*   <tr>
*     <th>Bit 5</th>
*     <th>Bits 4:0</th>
*     <th>Note</th>
*   </tr>
*   <tr>
*     <td>0</td>
*     <td>00000</td>
*     <td>Negative trim direction - minimum setting</td>
*   </tr>
*   <tr>
*     <td>0</td>
*     <td>11111</td>
*     <td>Negative trim direction - maximum setting</td>
*   </tr>
*   <tr>
*     <td>1</td>
*     <td>00000</td>
*     <td>Positive trim direction - minimum setting</td>
*   </tr>
*   <tr>
*     <td>1</td>
*     <td>11111</td>
*     <td>Positive trim direction - maximum setting</td>
*   </tr>
* </table>
*
* \param base
* Pointer to structure describing registers
*
* \param opampNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \param trim
* Trim value from 0 to 63
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_SLOPE_TRIM
*
*******************************************************************************/
void Cy_CTB_OpampSetSlope(CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum, uint32_t trim)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(opampNum));
    CY_ASSERT_L2(CY_CTB_TRIM(trim));

    if ((opampNum == CY_CTB_OPAMP_0) || (opampNum == CY_CTB_OPAMP_BOTH))
    {
        CTBM_OA0_SLOPE_OFFSET_TRIM(base) = (trim << CTBM_OA0_SLOPE_OFFSET_TRIM_OA0_SLOPE_OFFSET_TRIM_Pos) & CTBM_OA0_SLOPE_OFFSET_TRIM_OA0_SLOPE_OFFSET_TRIM_Msk;
    }

    if ((opampNum == CY_CTB_OPAMP_1) || (opampNum == CY_CTB_OPAMP_BOTH))
    {
        CTBM_OA1_SLOPE_OFFSET_TRIM(base) = (trim << CTBM_OA1_SLOPE_OFFSET_TRIM_OA1_SLOPE_OFFSET_TRIM_Pos) & CTBM_OA1_SLOPE_OFFSET_TRIM_OA1_SLOPE_OFFSET_TRIM_Msk;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_OpampGetSlope
****************************************************************************//**
*
* Return the CTB opamp slope trim value.
*
* \param base
* Pointer to structure describing registers
*
* \param opampNum
* \ref CY_CTB_OPAMP_0 or \ref CY_CTB_OPAMP_1
*
* \return Slope trim value
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_GET_SLOPE_TRIM
*
*******************************************************************************/
uint32_t Cy_CTB_OpampGetSlope(const CTBM_Type *base, cy_en_ctb_opamp_sel_t opampNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM_0_1(opampNum));

    uint32_t trimReg;

    if (opampNum == CY_CTB_OPAMP_0)
    {
        trimReg = CTBM_OA0_SLOPE_OFFSET_TRIM(base);
    }
    else
    {
        trimReg = CTBM_OA1_SLOPE_OFFSET_TRIM(base);
    }

    return trimReg;
}

/*******************************************************************************
* Function Name: Cy_CTB_SetAnalogSwitch
****************************************************************************//**
*
* Provide firmware control of the CTB switches. Each call to this function
* can open a set of switches or close a set of switches in one register.
*
* \param base
* Pointer to structure describing registers
*
* \param switchSelect
* A value of the enum \ref cy_en_ctb_switch_register_sel_t to select the switch
* register
*
* \param switchMask
* The mask of the switches to either open or close.
* The switch masks can be found in the following enums: \ref cy_en_ctb_oa0_switches_t,
* \ref cy_en_ctb_oa1_switches_t, and \ref cy_en_ctb_ctd_switches_t.
* Use the enum that is consistent with the provided register.
*
* \param state
* \ref CY_CTB_SWITCH_OPEN or \ref CY_CTB_SWITCH_CLOSE
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_SET_ANALOG_SWITCH
*
*******************************************************************************/
void Cy_CTB_SetAnalogSwitch(CTBM_Type *base, cy_en_ctb_switch_register_sel_t switchSelect, uint32_t switchMask, cy_en_ctb_switch_state_t state)
{
    CY_ASSERT_L3(CY_CTB_SWITCHSELECT(switchSelect));
    CY_ASSERT_L2(CY_CTB_SWITCHMASK(switchSelect, switchMask));
    CY_ASSERT_L3(CY_CTB_SWITCHSTATE(state));

    __IOM uint32_t *switchReg;
    __IOM uint32_t *switchClearReg;

    switch(switchSelect)
    {
    case CY_CTB_SWITCH_OA0_SW:
        switchReg = &CTBM_OA0_SW(base);
        switchClearReg = &CTBM_OA0_SW_CLEAR(base);
        break;
    case CY_CTB_SWITCH_OA1_SW:
        switchReg = &CTBM_OA1_SW(base);
        switchClearReg = &CTBM_OA1_SW_CLEAR(base);
        break;
    case CY_CTB_SWITCH_CTD_SW:
    default:
        switchReg = &CTBM_CTD_SW(base);
        switchClearReg = &CTBM_CTD_SW_CLEAR(base);
        break;
    }

    switch(state)
    {
    case CY_CTB_SWITCH_CLOSE:
        *switchReg = switchMask;
        break;
    case CY_CTB_SWITCH_OPEN:
    default:
        *switchClearReg = switchMask;
        break;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_GetAnalogSwitch
****************************************************************************//**
*
* Return the open or closed state of the specified analog switch.
*
* \param base
* Pointer to structure describing registers
*
* \param switchSelect
* A value of the enum \ref cy_en_ctb_switch_register_sel_t to select the switch
* register
*
* \return
* The state of the switches in the provided register.
* Compare this value to the switch masks in the following enums:
* \ref cy_en_ctb_oa0_switches_t, \ref cy_en_ctb_oa1_switches_t, and \ref cy_en_ctb_ctd_switches_t.
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_GET_ANALOG_SWITCH
*
*******************************************************************************/
uint32_t Cy_CTB_GetAnalogSwitch(const CTBM_Type *base, cy_en_ctb_switch_register_sel_t switchSelect)
{
    CY_ASSERT_L3(CY_CTB_SWITCHSELECT(switchSelect));

    uint32_t switchRegValue;

    switch(switchSelect)
    {
    case CY_CTB_SWITCH_OA0_SW:
        switchRegValue = CTBM_OA0_SW(base);
        break;
    case CY_CTB_SWITCH_OA1_SW:
        switchRegValue = CTBM_OA1_SW(base);
        break;
    case CY_CTB_SWITCH_CTD_SW:
    default:
        switchRegValue = CTBM_CTD_SW(base);
        break;
    }

    return switchRegValue;
}

/*******************************************************************************
* Function Name: Cy_CTB_CompSetConfig
****************************************************************************//**
*
* Configure the CTB comparator for pulse or level output, to bypass clock
* synchronization, and to enable hysteresis.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \param level
* Configure output to produce a pulse or level output signal

* \param bypass
* Configure output to be clock synchronized or unsynchronized

* \param hyst
* Enable or disable input hysteresis

* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_COMP_SET_CONFIG
*
*******************************************************************************/
void Cy_CTB_CompSetConfig(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum, cy_en_ctb_comp_level_t level, cy_en_ctb_comp_bypass_t bypass, cy_en_ctb_comp_hyst_t hyst)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(compNum));
    CY_ASSERT_L3(CY_CTB_COMPLEVEL(level));
    CY_ASSERT_L3(CY_CTB_COMPBYPASS(bypass));
    CY_ASSERT_L3(CY_CTB_COMPHYST(hyst));

    uint32_t opampCtrlReg;

    if ((compNum == CY_CTB_OPAMP_0) || (compNum == CY_CTB_OPAMP_BOTH))
    {
        opampCtrlReg = CTBM_OA_RES0_CTRL(base) & ~(CTBM_OA_RES0_CTRL_OA0_HYST_EN_Msk | CTBM_OA_RES0_CTRL_OA0_BYPASS_DSI_SYNC_Msk | CTBM_OA_RES0_CTRL_OA0_DSI_LEVEL_Msk);
        CTBM_OA_RES0_CTRL(base) = opampCtrlReg | (uint32_t) level |(uint32_t) bypass | (uint32_t) hyst;
    }

    if ((compNum == CY_CTB_OPAMP_1) || (compNum == CY_CTB_OPAMP_BOTH))
    {
        opampCtrlReg = CTBM_OA_RES1_CTRL(base) & ~(CTBM_OA_RES1_CTRL_OA1_HYST_EN_Msk | CTBM_OA_RES1_CTRL_OA1_BYPASS_DSI_SYNC_Msk | CTBM_OA_RES1_CTRL_OA1_DSI_LEVEL_Msk);
        CTBM_OA_RES1_CTRL(base) = opampCtrlReg | (uint32_t) level |(uint32_t) bypass | (uint32_t) hyst;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_CompGetConfig
****************************************************************************//**
*
* Return the CTB comparator operating configuration as set by \ref Cy_CTB_CompSetConfig.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0 or \ref CY_CTB_OPAMP_1
*
* \return
* The comparator configuration.
* Compare the register value with the masks in \ref cy_en_ctb_comp_level_t,
* \ref cy_en_ctb_comp_bypass_t, and \ref cy_en_ctb_comp_hyst_t.
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_COMP_GET_CONFIG
*
*******************************************************************************/
uint32_t Cy_CTB_CompGetConfig(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM_0_1(compNum));

    uint32_t config;

    if (compNum == CY_CTB_OPAMP_0)
    {
        config = CTBM_OA_RES0_CTRL(base) & (CTBM_OA_RES0_CTRL_OA0_HYST_EN_Msk | CTBM_OA_RES0_CTRL_OA0_BYPASS_DSI_SYNC_Msk | CTBM_OA_RES0_CTRL_OA0_DSI_LEVEL_Msk);
    }
    else
    {
        config = CTBM_OA_RES1_CTRL(base) & (CTBM_OA_RES1_CTRL_OA1_HYST_EN_Msk | CTBM_OA_RES1_CTRL_OA1_BYPASS_DSI_SYNC_Msk | CTBM_OA_RES1_CTRL_OA1_DSI_LEVEL_Msk);
    }

    return config;
}

/*******************************************************************************
* Function Name: Cy_CTB_CompSetInterruptEdgeType
****************************************************************************//**
*
* Configure the type of edge that will trigger a comparator interrupt.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0, \ref CY_CTB_OPAMP_1, or \ref CY_CTB_OPAMP_BOTH
*
* \param edge
* Edge type that will trigger an interrupt. Select a value from \ref cy_en_ctb_comp_edge_t.
*
* \return None
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_COMP_SET_INTERRUPT_EDGE_TYPE
*
*******************************************************************************/
void Cy_CTB_CompSetInterruptEdgeType(CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum, cy_en_ctb_comp_edge_t edge)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM(compNum));
    CY_ASSERT_L3(CY_CTB_COMPEDGE(edge));

    uint32_t opampCtrlReg;

    if ((compNum == CY_CTB_OPAMP_0) || (compNum == CY_CTB_OPAMP_BOTH))
    {
        opampCtrlReg = CTBM_OA_RES0_CTRL(base) & ~(CTBM_OA_RES0_CTRL_OA0_COMPINT_Msk);
        CTBM_OA_RES0_CTRL(base) = opampCtrlReg | (uint32_t) edge;
    }

    if ((compNum == CY_CTB_OPAMP_1) || (compNum == CY_CTB_OPAMP_BOTH))
    {
        opampCtrlReg = CTBM_OA_RES1_CTRL(base) & ~(CTBM_OA_RES1_CTRL_OA1_COMPINT_Msk);
        CTBM_OA_RES1_CTRL(base) = opampCtrlReg | (uint32_t) edge;
    }
}

/*******************************************************************************
* Function Name: Cy_CTB_CompGetStatus
****************************************************************************//**
*
* Return the comparator output status.
* When the positive input voltage is greater than the negative input voltage,
* the comparator status is high. Otherwise, the status is low.
*
* \param base
* Pointer to structure describing registers
*
* \param compNum
* \ref CY_CTB_OPAMP_0 or \ref CY_CTB_OPAMP_1.
* \ref CY_CTB_OPAMP_NONE and \ref CY_CTB_OPAMP_BOTH are invalid options.
*
* \return
* The comparator status.
* A value of 0 is returned if compNum is invalid.
* - 0: Status is low
* - 1: Status is high
*
* \funcusage
*
* \snippet ctb_sut_01.cydsn/main_cm4.c CTB_SNIPPET_COMP_GET_STATUS
*
*******************************************************************************/
uint32_t Cy_CTB_CompGetStatus(const CTBM_Type *base, cy_en_ctb_opamp_sel_t compNum)
{
    CY_ASSERT_L3(CY_CTB_OPAMPNUM_0_1(compNum));

    uint32_t compStatusResult;

    if (CY_CTB_OPAMP_0 == compNum)
    {
        compStatusResult = (CTBM_COMP_STAT(base) & CTBM_COMP_STAT_OA0_COMP_Msk) >> CTBM_COMP_STAT_OA0_COMP_Pos;
    }
    else if (CY_CTB_OPAMP_1 == compNum)
    {
        compStatusResult = (CTBM_COMP_STAT(base) & CTBM_COMP_STAT_OA1_COMP_Msk) >> CTBM_COMP_STAT_OA1_COMP_Pos;
    }
    else
    {
        compStatusResult = 0uL;
    }

    return compStatusResult;
}

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40PASS_CTB */

/* [] END OF FILE */

