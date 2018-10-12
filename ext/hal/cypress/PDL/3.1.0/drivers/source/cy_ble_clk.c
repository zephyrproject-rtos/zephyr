 /***************************************************************************//**
* \file cy_ble_clk.c
* \version 3.0
*
* \brief
*  This driver provides the source code for API BLE ECO clock.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/
#include "cy_ble_clk.h"

#if defined(CY_IP_MXBLESS)

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Internal functions */
static cy_en_ble_eco_status_t Cy_BLE_HAL_RcbRegRead(uint16_t addr, uint16_t *data);
static cy_en_ble_eco_status_t Cy_BLE_HAL_RcbRegWrite(uint16_t addr, uint16_t data);
static cy_en_ble_eco_status_t Cy_BLE_HAL_EcoSetTrim(uint32_t trim, uint32_t startUpTime);
static cy_en_ble_eco_status_t Cy_BLE_HAL_MxdRadioEnableClocks(cy_en_ble_eco_freq_t ecoFreq, 
                                                              cy_en_ble_eco_sys_clk_div_t sysClkDiv);

/*******************************************************************************
* Function Name: Cy_BLE_HAL_Init
****************************************************************************//**
*
*  Initializes the BLESS port.
*
* \return
*  None
*
*******************************************************************************/
void Cy_BLE_HAL_Init(void)
{
    /* Configures pins for BLESS */
    const cy_stc_gpio_prt_config_t port2_cfg =
    {
        .cfg        = CY_BLE_PORT2_CFG_VAL,
        .cfgOut     = CY_BLE_PORT_CFG_OUT_VAL,
        .sel0Active = CY_BLE_PORT2_HSIOM_SEL0,
        .sel1Active = CY_BLE_PORT2_HSIOM_SEL1
    };
    const cy_stc_gpio_prt_config_t port3_cfg =
    {
        .cfg        = CY_BLE_PORT3_CFG_VAL,
        .cfgOut     = CY_BLE_PORT_CFG_OUT_VAL,
        .sel0Active = CY_BLE_PORT3_HSIOM_SEL0,
        .sel1Active = CY_BLE_PORT3_HSIOM_SEL1
    };
    const cy_stc_gpio_prt_config_t port4_cfg =
    {
        .cfg        = CY_BLE_PORT4_CFG_VAL,
        .cfgOut     = CY_BLE_PORT_CFG_OUT_VAL,
        .sel0Active = CY_BLE_PORT4_HSIOM_SEL0,
        .sel1Active = CY_BLE_PORT4_HSIOM_SEL1
    };

    (void)Cy_GPIO_Port_Init(GPIO_PRT2, &port2_cfg);
    (void)Cy_GPIO_Port_Init(GPIO_PRT3, &port3_cfg);
    (void)Cy_GPIO_Port_Init(GPIO_PRT4, &port4_cfg);
}


/*******************************************************************************
* Function Name: Cy_BLE_HAL_RcbRegRead
****************************************************************************//**
*
*  Reads Radio register.
*
*  \param addr: Register address.
*  \param *data: Register data (output parameter).
*
*  \return
*  \ref cy_en_ble_eco_status_t : Return value indicates if the function succeeded or
*  failed. The following are possible error codes:
*
*  Error Codes                 | Description
*  --------------------        | -----------
*  CY_BLE_ECO_SUCCESS          | On successful operation.
*  CY_BLE_ECO_HARDWARE_ERROR   | If RCB operation failed
*
*******************************************************************************/
static cy_en_ble_eco_status_t Cy_BLE_HAL_RcbRegRead(uint16_t addr, uint16_t *data)
{
    cy_en_ble_eco_status_t status = CY_BLE_ECO_SUCCESS;
    uint32_t timeout = CY_BLE_RCB_TIMEOUT;
    uint32_t temp = (uint32_t)((0x8000UL | addr) << 16u);

    /* Write Address to the RCB */
    BLE->RCB.TX_FIFO_WR = temp;

    /* Wait for Data. */
    while(((BLE->RCB.INTR & BLE_RCB_INTR_RCB_DONE_Msk) == 0u) && (timeout > 0u))
    {
        timeout--;
        Cy_SysLib_DelayUs(CY_BLE_DELAY_TIME);
    }

    if(timeout > 0u)
    {
        BLE->RCB.INTR |= BLE_RCB_INTR_RCB_DONE_Msk;
        *data = (uint16_t)BLE->RCB.RX_FIFO_RD;
    }
    else
    {
        status = CY_BLE_ECO_HARDWARE_ERROR;
    }

    return(status);
}


/*******************************************************************************
* Function Name: Cy_BLE_HAL_RcbRegWrite
****************************************************************************//**
*
*  Writes Radio register.
*
*  \param addr: Register address.
*  \param data: Register data.
*
*  \return
*  \ref cy_en_ble_eco_status_t : Return value indicates if the function succeeded or
*  failed. The following are possible error codes:
*
*  Error Codes                 | Description
*  --------------------        | -----------
*  CY_BLE_ECO_SUCCESS          | On successful operation.
*  CY_BLE_ECO_HARDWARE_ERROR   | If RCB operation failed
*
*******************************************************************************/
static cy_en_ble_eco_status_t Cy_BLE_HAL_RcbRegWrite(uint16_t addr, uint16_t data)
{
    cy_en_ble_eco_status_t status = CY_BLE_ECO_SUCCESS;
    uint32_t timeout = CY_BLE_RCB_TIMEOUT;
    uint32_t temp = ((uint32_t)((0x7FFFUL & addr) << 16u)) | data;

    BLE->RCB.TX_FIFO_WR = temp;

    /* Wait for RCB done. */
    while(((BLE->RCB.INTR & BLE_RCB_INTR_RCB_DONE_Msk) == 0u) && (timeout > 0u))
    {
        timeout--;
        Cy_SysLib_DelayUs(CY_BLE_DELAY_TIME);
    }

    if(timeout > 0u)
    {
        BLE->RCB.INTR |= BLE_RCB_INTR_RCB_DONE_Msk;
    }
    else
    {
        status = CY_BLE_ECO_HARDWARE_ERROR;
    }
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_BLE_HAL_EcoSetTrim
****************************************************************************//**
*
*  Sets ECO trim value.
*
*  \param trim       : Trim value.
*         startUpTime: Start up time delay.
*
*  \return
*  \ref cy_en_ble_eco_status_t : Return value indicates if the function succeeded or
*  failed. The following are possible error codes:
*
*  Error Codes                 | Description
*  --------------------        | -----------
*  CY_BLE_ECO_SUCCESS          | On successful operation.
*  CY_BLE_ECO_HARDWARE_ERROR   | If RCB operation failed
*
*******************************************************************************/
static cy_en_ble_eco_status_t Cy_BLE_HAL_EcoSetTrim(uint32_t trim, uint32_t startUpTime)
{
    uint16_t reg = CY_BLE_RF_DCXO_CFG_REG_VALUE;
    cy_en_ble_eco_status_t status;
    
    /* Load the new CAP TRIM value */
    reg |= (uint16_t)((uint16_t)trim << CY_BLE_RF_DCXO_CFG_REG_DCXO_CAP_SHIFT);

    /* Write the new value to the register */
    status = Cy_BLE_HAL_RcbRegWrite(CY_BLE_RF_DCXO_CFG_REG, reg);
        
    if(status == CY_BLE_ECO_SUCCESS)
    {
        /* Write the new value to the CFG2 register */
        status = Cy_BLE_HAL_RcbRegWrite(CY_BLE_RF_DCXO_CFG2_REG, CY_BLE_RF_DCXO_CFG2_REG_VALUE);
    }
    
    Cy_SysLib_DelayUs((uint16_t)startUpTime * CY_BLE_ECO_SET_TRIM_DELAY_COEF);
    
    return(status);
}


/*******************************************************************************
* Function Name: Cy_BLE_HAL_MxdRadioEnableClocks
****************************************************************************//**
*
*  Enables and configures radio clock.
*  
*  \param config
*  The pointer to the configuration structure \ref cy_stc_ble_eco_config_t.
*
*  \return
*  \ref cy_en_ble_eco_status_t : Return value indicates if the function succeeded or
*  failed. The following are possible error codes:
*
*  Error Codes                 | Description
*  --------------------        | -----------
*  CY_BLE_ECO_SUCCESS          | On successful operation.
*  CY_BLE_ECO_HARDWARE_ERROR   | If RCB operation failed
*
*******************************************************************************/
static cy_en_ble_eco_status_t Cy_BLE_HAL_MxdRadioEnableClocks(cy_en_ble_eco_freq_t ecoFreq, 
                                                              cy_en_ble_eco_sys_clk_div_t sysClkDiv)
{
    uint16_t temp;
    uint16_t blerdDivider;
    uint16_t ecoSysDivider;
    uint16_t blellDivider = 0u;
    uint32_t retries = CY_BLE_RCB_RETRIES;
    cy_en_ble_eco_status_t status;

    /* De-assert active domain reset and enable clock buffer in MXD Radio */
    do
    {
        status = Cy_BLE_HAL_RcbRegWrite(CY_BLE_PMU_MODE_TRANSITION_REG, CY_BLE_MXD_RADIO_CLK_BUF_EN_VAL);
        if(status == CY_BLE_ECO_SUCCESS)
        {
            status = Cy_BLE_HAL_RcbRegRead(CY_BLE_PMU_MODE_TRANSITION_REG, &temp);
        }
        if(retries > 0u)
        {
            retries--;
        }
        else
        {
            status = CY_BLE_ECO_HARDWARE_ERROR;
        }
    }
    while((status == CY_BLE_ECO_SUCCESS) && (temp != CY_BLE_MXD_RADIO_CLK_BUF_EN_VAL));

    /* Check if ECO clock output is enabled
     * In MXD_VER2, ECO clock is stopped on overwriting this bit */
    if(status == CY_BLE_ECO_SUCCESS)
    {
        retries = CY_BLE_RCB_RETRIES;
        status = Cy_BLE_HAL_RcbRegRead(CY_BLE_PMU_PMU_CTRL_REG, &temp);
        if((status == CY_BLE_ECO_SUCCESS) && ((temp & CY_BLE_MXD_RADIO_DIG_CLK_OUT_EN_VAL) == 0u)) 
        {
            /* Enable digital ECO clock output from MXD Radio to BLESS */
            do
            {
                status = Cy_BLE_HAL_RcbRegWrite(CY_BLE_PMU_PMU_CTRL_REG, CY_BLE_MXD_RADIO_DIG_CLK_OUT_EN_VAL);
                if(status == CY_BLE_ECO_SUCCESS)
                {
                    status = Cy_BLE_HAL_RcbRegRead(CY_BLE_PMU_PMU_CTRL_REG, &temp);
                }
                if(retries > 0u)
                {
                    retries--;
                }
                else
                {
                    status = CY_BLE_ECO_HARDWARE_ERROR;
                }
            }
            while((status == CY_BLE_ECO_SUCCESS) && (temp != CY_BLE_MXD_RADIO_DIG_CLK_OUT_EN_VAL));
        }
    }

    /* Configure ECO clock frequency and clock divider in MXD Radio */
    if(status == CY_BLE_ECO_SUCCESS)
    {
        /* Read the MXD Radio register */
        status = Cy_BLE_HAL_RcbRegRead(CY_BLE_RF_DCXO_BUF_CFG_REG, &temp);
    }
    if(status == CY_BLE_ECO_SUCCESS)
    {
        /* Clear clock divider and select amp buffer output bits */
        temp &= CY_LO16(~((CY_BLE_RF_DCXO_BUF_CFG_REG_CLK_DIV_MASK << CY_BLE_RF_DCXO_BUF_CFG_REG_CLK_DIV_SHIFT) |
                          (CY_BLE_RF_DCXO_BUF_CFG_REG_BUF_AMP_SEL_MASK << CY_BLE_RF_DCXO_BUF_CFG_REG_BUF_AMP_SEL_SHIFT)));

        /* Total ECO divider consist of divider located on BLERD and BLESS divider
         * Set BLERD divider to maximum value taking in to account that 8 MHz is required for BLELL
         * BLELL clock frequency is set to 8 MHz irrespective of the crystal value.
         */
        if(ecoFreq == CY_BLE_BLESS_ECO_FREQ_32MHZ)
        {
            if(sysClkDiv >= CY_BLE_SYS_ECO_CLK_DIV_4)
            {
                blerdDivider = (uint16_t)CY_BLE_MXD_RADIO_CLK_DIV_4;
            }
            else
            {
                blerdDivider = (uint16_t)sysClkDiv;
                blellDivider = (uint16_t)CY_BLE_MXD_RADIO_CLK_DIV_4 - blerdDivider;
            }
            temp |= CY_BLE_RF_DCXO_BUF_CFG_REG_XTAL_32M_SEL_BIT;
            temp |= (uint16_t)(CY_BLE_MXD_RADIO_CLK_BUF_AMP_32M_LARGE << CY_BLE_RF_DCXO_BUF_CFG_REG_BUF_AMP_SEL_SHIFT);

            /* Update cy_BleEcoClockFreqHz for the proper Cy_SysLib_Delay functionality */
            cy_BleEcoClockFreqHz = CY_BLE_DEFAULT_ECO_CLK_FREQ_32MHZ / (1UL << (uint16_t)sysClkDiv);
        }
        else
        {
            if(sysClkDiv >= CY_BLE_SYS_ECO_CLK_DIV_2)
            {
                blerdDivider = (uint16_t)CY_BLE_MXD_RADIO_CLK_DIV_2;
            }
            else
            {
                blerdDivider = (uint16_t)sysClkDiv;
                blellDivider = (uint16_t)CY_BLE_MXD_RADIO_CLK_DIV_2 - blerdDivider;
            }
            temp &= (uint16_t) ~CY_BLE_RF_DCXO_BUF_CFG_REG_XTAL_32M_SEL_BIT;
            temp |= (uint16_t)(CY_BLE_MXD_RADIO_CLK_BUF_AMP_16M_LARGE << CY_BLE_RF_DCXO_BUF_CFG_REG_BUF_AMP_SEL_SHIFT);

            /* Update cy_BleEcoClockFreqHz for the proper Cy_SysLib_Delay functionality */
            cy_BleEcoClockFreqHz = CY_BLE_DEFAULT_ECO_CLK_FREQ_16MHZ / (1UL << (uint16_t)sysClkDiv);
        }

        temp |= (uint16_t)(blerdDivider << CY_BLE_RF_DCXO_BUF_CFG_REG_CLK_DIV_SHIFT);
        
        /* Write the MXD Radio register */
        status = Cy_BLE_HAL_RcbRegWrite(CY_BLE_RF_DCXO_BUF_CFG_REG, temp);
        
        /* Reduce BLESS divider by BLERD divider value */
        ecoSysDivider = (uint16_t)sysClkDiv - blerdDivider;
        temp = (uint16_t)(ecoSysDivider & BLE_BLESS_XTAL_CLK_DIV_CONFIG_SYSCLK_DIV_Msk);
        temp |= (uint16_t)(blellDivider << BLE_BLESS_XTAL_CLK_DIV_CONFIG_LLCLK_DIV_Pos);

        /* Set clock divider */
        BLE->BLESS.XTAL_CLK_DIV_CONFIG = temp;
    }
    
    /* Update RADIO LDO trim values */
    if((Cy_SysLib_GetDeviceRevision() != CY_SYSLIB_DEVICE_REV_0A) && (SFLASH->RADIO_LDO_TRIMS != 0u))
    {
        if(status == CY_BLE_ECO_SUCCESS)
        {
            status = Cy_BLE_HAL_RcbRegRead(CY_BLE_RF_LDO_CFG_REG, &temp);
        }
        
        if(status == CY_BLE_ECO_SUCCESS) 
        { 
            /* Update LDO_IF value */
            temp &= (uint16_t)~((uint16_t) ((uint16_t)CY_BLE_RF_LDO_CFG_REG_LDO_IF_CFG_MASK << CY_BLE_RF_LDO_CFG_REG_LDO_IF_CFG_SHIFT)); 
            temp |= (uint16_t)(((SFLASH->RADIO_LDO_TRIMS & SFLASH_RADIO_LDO_TRIMS_LDO_IF_Msk) >>
                                SFLASH_RADIO_LDO_TRIMS_LDO_IF_Pos) << CY_BLE_RF_LDO_CFG_REG_LDO_IF_CFG_SHIFT);
            
            /* Update LDO_ACT value */
            temp &= (uint16_t)~((uint16_t) ((uint16_t)CY_BLE_RF_LDO_CFG_REG_LDO_ACT_CFG_MASK << CY_BLE_RF_LDO_CFG_REG_LDO_ACT_CFG_SHIFT)); 
            temp |= (uint16_t)(((SFLASH->RADIO_LDO_TRIMS & SFLASH_RADIO_LDO_TRIMS_LDO_ACT_Msk) >>
                                SFLASH_RADIO_LDO_TRIMS_LDO_ACT_Pos) << CY_BLE_RF_LDO_CFG_REG_LDO_ACT_CFG_SHIFT);
            
            /* Update LDO_DIG value */
            temp &= (uint16_t)~((uint16_t) ((uint16_t)CY_BLE_RF_LDO_CFG_REG_LDO10_CFG_MASK << CY_BLE_RF_LDO_CFG_REG_LDO10_CFG_SHIFT));
            temp |= (uint16_t)(((SFLASH->RADIO_LDO_TRIMS & SFLASH_RADIO_LDO_TRIMS_LDO_DIG_Msk) >>
                                SFLASH_RADIO_LDO_TRIMS_LDO_DIG_Pos) << CY_BLE_RF_LDO_CFG_REG_LDO10_CFG_SHIFT);
            
            status = Cy_BLE_HAL_RcbRegWrite(CY_BLE_RF_LDO_CFG_REG, temp);
        }
        
        if(status == CY_BLE_ECO_SUCCESS)
        {
           status = Cy_BLE_HAL_RcbRegRead(CY_BLE_RF_LDO_EN_REG, &temp);
        }
        
        if(status == CY_BLE_ECO_SUCCESS)
        {  
            /* Update LDO_LNA value */
            temp &= (uint16_t)~(CY_BLE_RF_LDO_EN_REG_LDO_RF_CFG_MASK << CY_BLE_RF_LDO_EN_REG_LDO_RF_CFG_SHIFT);
            temp |= (uint16_t)(((SFLASH->RADIO_LDO_TRIMS & SFLASH_RADIO_LDO_TRIMS_LDO_LNA_Msk) >>
                       SFLASH_RADIO_LDO_TRIMS_LDO_LNA_Pos) << CY_BLE_RF_LDO_EN_REG_LDO_RF_CFG_SHIFT); 
            
            status = Cy_BLE_HAL_RcbRegWrite(CY_BLE_RF_LDO_EN_REG, temp);
        }
    }
    return(status);
}


/*******************************************************************************
* Function Name: Cy_BLE_EcoConfigure
****************************************************************************//**
*
*  This API configures and enables the BLE ECO clock.
*
*  If the BLE Middleware starts, the BLE Stack enables the BLE ECO clock
*  automatically with the default parameters:
*
*    Parameter                 |  Value
*  --------------------        | -----------
*    ECO Frequency             | CY_BLE_DEFAULT_ECO_FREQ
*    Divider                   | CY_BLE_DEFAULT_ECO_DIV
*    Startup time              | CY_BLE_DEFAULT_OSC_STARTUP_DELAY_LF
*    Load cap                  | CY_BLE_DEFAULT_CAP_TRIM_VALUE
*
*  If there is a need to start the BLE ECO with non-default parameters, call the  
*  Cy_BLE_EcoConfigure() function with the custom configuration each time before calling
*  the Cy_BLE_Enable() function.
*
*  This clock is stopped in Deep Sleep and Hibernate power modes.
*
*  \param freq            -  Operating frequency of the crystal, type of
*                            \ref cy_en_ble_eco_freq_t.
*
*  \param sysClkDiv       -  System divider for ECO clock, type of
*                            \ref cy_en_ble_eco_sys_clk_div_t.
*
*  \param cLoad           -  ECO crystal load capacitance in multiple of 0.075pF 
*                            (pF_from_user min - 7.5pF, pF_from_user max - 26.625pF)
*                            cLoad = ((pF_from_user - 7.5)/0.075)
*
*  \param xtalStartUpTime -  ECO crystal startup time in multiple of 31.25us
*                            (startup_time_from_user min - 31.25us)
*                            xtalStartUpTime = startup_time_from_user/31.25 
*                             
*  \param voltageReg      -  BLE Voltage regulator, type of \ref cy_en_ble_eco_voltage_reg_t
*                             
*  \return
*  \ref cy_en_ble_eco_status_t : The return value indicates if the function 
*  succeeded or failed. The possible error codes:
*
*  Error Codes                 | Description
*  --------------------        | -----------
*  CY_BLE_ECO_SUCCESS          | The function completed successfully.
*  CY_BLE_ECO_BAD_PARAM        | The wrong input parameter.
*  CY_BLE_ECO_RCB_CONTROL_LL   | The RCB Control is with BLE Link Layer.
*  CY_BLE_ECO_ALREADY_STARTED  | The BLE ECO clock is already started.
*  CY_BLE_ECO_HARDWARE_ERROR   | The RCB or BLE ECO operation failed.
*
*  \sideeffect
*   The I/O pins will be automatically unfrozen coming out of Hibernate when 
*   the BLE ECO is in use.
*
*  \note Limitation: Do not call this API if the BLE is executed with LPM.
*   There is a risk that when the Cy_BLE_EcoConfigure() function is called on 
*   one core, the BLE is in Deep Sleep mode on the other core. It will cause
*   a fault hard exception.
*
*******************************************************************************/
cy_en_ble_eco_status_t Cy_BLE_EcoConfigure(cy_en_ble_eco_freq_t freq, cy_en_ble_eco_sys_clk_div_t sysClkDiv, 
                                           uint32_t cLoad, uint32_t xtalStartUpTime, cy_en_ble_eco_voltage_reg_t voltageReg)
{
    cy_en_ble_eco_status_t status = CY_BLE_ECO_SUCCESS;
    uint32_t temp = 0u;
    
    if((freq > CY_BLE_BLESS_ECO_FREQ_32MHZ) || (sysClkDiv > CY_BLE_SYS_ECO_CLK_DIV_8))
    {
        status = CY_BLE_ECO_BAD_PARAM;
    }
    else
    {
        /* Unfreeze IO after Hibernate */
        if(Cy_SysPm_GetIoFreezeStatus())
        {
            Cy_SysPm_IoUnfreeze();
        }
            
        if(((BLE->BLESS.MT_CFG & BLE_BLESS_MT_CFG_ENABLE_BLERD_Msk) != 0u) && 
           ((BLE->BLESS.MT_STATUS & BLE_BLESS_MT_STATUS_BLESS_STATE_Msk) != 0u))
        {
            status = CY_BLE_ECO_ALREADY_STARTED;
        }
        else
        {
            /* HAL Initialization */
            Cy_BLE_HAL_Init();
    
            /* BLESS MT Delays configuration */
            BLE->BLESS.MT_DELAY_CFG =
                (uint32_t)(CY_BLE_DEFAULT_HVLDO_STARTUP_DELAY << BLE_BLESS_MT_DELAY_CFG_HVLDO_STARTUP_DELAY_Pos) |
                (uint32_t)(CY_BLE_DEFAULT_ISOLATE_DEASSERT_DELAY << BLE_BLESS_MT_DELAY_CFG_ISOLATE_DEASSERT_DELAY_Pos) |
                (uint32_t)(CY_BLE_DEFAULT_ACT_TO_SWITCH_DELAY << BLE_BLESS_MT_DELAY_CFG_ACT_TO_SWITCH_DELAY_Pos) |
                (uint32_t)(CY_BLE_DEFAULT_HVLDO_DISABLE_DELAY << BLE_BLESS_MT_DELAY_CFG_HVLDO_DISABLE_DELAY_Pos);

            BLE->BLESS.MT_DELAY_CFG2 =
                (uint32_t)(xtalStartUpTime << BLE_BLESS_MT_DELAY_CFG2_OSC_STARTUP_DELAY_LF_Pos) |
                (uint32_t)(CY_BLE_DEFAULT_ACT_STARTUP_DELAY << BLE_BLESS_MT_DELAY_CFG2_ACT_STARTUP_DELAY_Pos) |
                (uint32_t)(CY_BLE_DEFAULT_DIG_LDO_STARTUP_DELAY << BLE_BLESS_MT_DELAY_CFG2_DIG_LDO_STARTUP_DELAY_Pos);

            BLE->BLESS.MT_DELAY_CFG3 =
                (uint32_t)(CY_BLE_DEFAULT_XTAL_DISABLE_DELAY << BLE_BLESS_MT_DELAY_CFG3_XTAL_DISABLE_DELAY_Pos) |
                (uint32_t)(CY_BLE_DEFAULT_DIG_LDO_DISABLE_DELAY << BLE_BLESS_MT_DELAY_CFG3_DIG_LDO_DISABLE_DELAY_Pos) |
                (uint32_t)(CY_BLE_DEFAULT_VDDR_STABLE_DELAY << BLE_BLESS_MT_DELAY_CFG3_VDDR_STABLE_DELAY_Pos);

            /* RCB Initialization
             * Check if the RCB Control is with LL.
             */
            if((BLE->RCB.RCBLL.CTRL & BLE_RCB_RCBLL_CTRL_RCBLL_CTRL_Msk) != 0u)
            {
                status = CY_BLE_ECO_RCB_CONTROL_LL;
            }
            else
            {
                uint32_t rcbDivider = 0u;

                /* If clock source for RCB is PeriClk */
                if((BLE_BLESS->LL_CLK_EN & BLE_BLESS_LL_CLK_EN_SEL_RCB_CLK_Msk) == 0u)
                {
                    if(cy_PeriClkFreqHz > CY_BLE_DEFAULT_RCB_CTRL_FREQ)
                    {
                        rcbDivider = (cy_PeriClkFreqHz / CY_BLE_DEFAULT_RCB_CTRL_FREQ) - 1u;
                    }
                }
                else
                {
                    rcbDivider = CY_BLE_DEFAULT_RCB_CTRL_DIV;
                }

                /* Configure default RCB Parameters. */
                BLE->RCB.CTRL |= (BLE_RCB_CTRL_ENABLED_Msk |
                                  BLE_RCB_CTRL_RX_CLK_EDGE_Msk |
                                  (CY_BLE_DEFAULT_RCB_CTRL_LEAD << BLE_RCB_CTRL_LEAD_Pos) |
                                  (CY_BLE_DEFAULT_RCB_CTRL_LAG << BLE_RCB_CTRL_LAG_Pos) |
                                  ((rcbDivider > 0u) ? ((rcbDivider << BLE_RCB_CTRL_DIV_Pos) |
                                                        BLE_RCB_CTRL_DIV_ENABLED_Msk) : 0u));

                /* If user uses SIMO Buck, enable Buck2 in hardware mode for BLE */
                if((Cy_SysPm_SimoBuckIsEnabled()) && (voltageReg == CY_BLE_ECO_VOLTAGE_REG_AUTO))
                {
                    Cy_SysPm_BuckSetVoltage2(CY_SYSPM_BUCK_OUT2_VOLTAGE_1_3V, true);
                    Cy_SysPm_BuckSetVoltage2HwControl(true);
                }
                else    /* Configure BLE LDO if SIMO Buck2 is not enabled */
                {
                    BLE->BLESS.MISC_EN_CTRL |= (BLE_BLESS_MISC_EN_CTRL_ACT_REG_EN_CTRL_Msk |
                                                BLE_BLESS_MISC_EN_CTRL_BUCK_EN_CTRL_Msk);
                }

                /* Enable the VIO supply and LDO in standby mode for slow ramp */
                temp = BLE->BLESS.MT_CFG;
                temp |= BLE_BLESS_MT_CFG_HVLDO_BYPASS_Msk |
                        BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_BYPASS_Msk |
                        BLE_BLESS_MT_CFG_HVLDO_EN_Msk |
                        BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_EN_Msk;
                BLE->BLESS.MT_CFG = temp;
                Cy_SysLib_DelayUs(64u);

                if((BLE->BLESS.MT_VIO_CTRL & BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_Msk) == 0u)
                {
                    /* Enable LDO */
                    BLE->BLESS.MT_VIO_CTRL = BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_Msk;

                    /* Wait for 64us after turning HVLDO ON */
                    Cy_SysLib_DelayUs(64u);

                    /* Enable LDO Delayed */
                    BLE->BLESS.MT_VIO_CTRL = BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_Msk |
                                             BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_DLY_Msk;
                    /* Wait for 64us */
                    Cy_SysLib_DelayUs(64u);
                }

                /* Disable override mode and let hardware take control of HVLDO */
                temp &= ~(BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_BYPASS_Msk |
                          BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_EN_Msk);
                BLE->BLESS.MT_CFG = temp;

                /* Wait for the VIO stable key write operation to complete */
                {
                    uint32_t timeout = CY_BLE_VIO_TIMEOUT;
                    while(((GPIO->VDD_ACTIVE & 0x10u) == 0u) && (timeout > 0u))
                    {
                        timeout--;
                        Cy_SysLib_DelayUs(CY_BLE_DELAY_TIME);
                    }
                    if(timeout == 0u)
                    {
                        status = CY_BLE_ECO_HARDWARE_ERROR;
                    }
                }
                
                
                if(status == CY_BLE_ECO_SUCCESS)
                {
                    uint32_t timeout = CY_BLE_ACT_TIMEOUT;
                    
                    /* Clear the BLERD_ACTIVE_INTR */
                    BLE->BLESS.INTR_STAT |= BLE_BLESS_INTR_STAT_BLERD_ACTIVE_INTR_Msk;

                    if(!Cy_SysPm_SimoBuckOutputIsEnabled(CY_SYSPM_BUCK_VRF))
                    {
                        temp |= BLE_BLESS_MT_CFG_ACT_LDO_NOT_BUCK_Msk;
                    }
                    /* Enable Radio */
                    temp |= BLE_BLESS_MT_CFG_ENABLE_BLERD_Msk;

                    /* Disable ECO in DeepSleep mode */
                    temp &= ~BLE_BLESS_MT_CFG_DPSLP_ECO_ON_Msk;
                    BLE->BLESS.MT_CFG = temp;

                    /* Wait for BLESS in ACTIVE state */
                    while(((BLE->BLESS.MT_STATUS & BLE_BLESS_MT_STATUS_BLESS_STATE_Msk) == 0u) && (timeout > 0u))
                    {
                        timeout--;
                        Cy_SysLib_DelayUs(CY_BLE_DELAY_TIME);
                    }
                    if(timeout == 0u)
                    {
                        status = CY_BLE_ECO_HARDWARE_ERROR;
                    }
                }
                
                if(status == CY_BLE_ECO_SUCCESS)
                {
                    /* Enable and configure radio clock */
                    status = Cy_BLE_HAL_MxdRadioEnableClocks(freq, sysClkDiv);
                }
                
                if(status == CY_BLE_ECO_SUCCESS)
                {
                    /* Set Load capacitance */
                    status = Cy_BLE_HAL_EcoSetTrim(cLoad, xtalStartUpTime);
                }
            }
        }
    }

    return(status);
}


/*******************************************************************************
* Function Name: Cy_BLE_EcoReset
****************************************************************************//**
*
*  This API resets and disables the BLE ECO clock.
*
*  \return
*   None
*
*******************************************************************************/
void Cy_BLE_EcoReset(void)
{
    /* Initiate Soft Reset */
    BLE->BLESS.LL_CLK_EN |= BLE_BLESS_LL_CLK_EN_BLESS_RESET_Msk;
}


/*******************************************************************************
* Function Name: Cy_BLE_EcoStart
****************************************************************************//**
*
*  This API configure and enables the BLE ECO clock.
*
*  This API is used as the PSoC Creator wrapper function. This function is not
*  recommended for new designs. Please use Cy_BLE_EcoConfigure()
*  instead of this function. 
*
*  \param config
*  The pointer to the configuration structure \ref cy_stc_ble_eco_config_t.
*
*  \return
*  \ref cy_en_ble_eco_status_t : The return value indicates if the function 
*  succeeded or failed. The possible error codes:
*
*  Error Codes                 | Description
*  --------------------        | -----------
*  CY_BLE_ECO_SUCCESS          | The function completed successfully.
*  CY_BLE_ECO_BAD_PARAM        | The wrong input parameter.
*  CY_BLE_ECO_RCB_CONTROL_LL   | The RCB Control is with BLE Link Layer.
*  CY_BLE_ECO_ALREADY_STARTED  | The BLE ECO clock is already started.
*  CY_BLE_ECO_HARDWARE_ERROR   | The RCB or BLE ECO operation failed.
*
*  \sideeffect
*   The I/O pins will be automatically unfrozen coming out of Hibernate when 
*   the BLE ECO is in use.
*
*  \note Limitation: Do not call this API if the BLE is executed with LPM.
*   There is a risk that when the Cy_BLE_EcoStart() function is called on one 
*   core, the BLE is in Deep Sleep mode on the other core. It will cause 
*   a fault hard exception.
*
*******************************************************************************/
cy_en_ble_eco_status_t Cy_BLE_EcoStart(const cy_stc_ble_eco_config_t *config)
{
    cy_en_ble_eco_status_t retValue = CY_BLE_ECO_BAD_PARAM;
    if(config != NULL)
    {
        /* Configure BLE ECO clock */
        retValue = Cy_BLE_EcoConfigure(config->ecoFreq, config->ecoSysDiv, 
                                      (uint32_t) config->loadCap, 
                                      (uint32_t) config->ecoXtalStartUpTime,
                                      CY_BLE_ECO_VOLTAGE_REG_AUTO);
    }
    return (retValue);    
}


/*******************************************************************************
* Function Name: Cy_BLE_EcoStop
****************************************************************************//**
*
*  This API stops BLE ECO clock. 
*   
*  This API is used as the PSoC Creator wrapper function. This function is not
*  recommended for new designs. Please use Cy_BLE_EcoReset() instead of this 
*  function. 
*
*  \return
*   None
*
*******************************************************************************/
void Cy_BLE_EcoStop(void)
{
    Cy_BLE_EcoReset();
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* defined(CY_IP_MXBLESS) */


/* [] END OF FILE */
