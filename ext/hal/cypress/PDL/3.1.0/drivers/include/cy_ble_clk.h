/***************************************************************************//**
* \file cy_ble_clk.h
* \version 3.0
* 
* The header file of the BLE ECO clock driver.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \addtogroup group_ble_clk BLE ECO Clock (BLE ECO)
* \{
* This driver provides an API to manage the BLE ECO clock block. 
*
* The BLE ECO clock is a high-accuracy high-frequency clock that feeds the
* link-layer controller and the radio Phy. 
*
* This clock is also an input to the system resources subsystem as an
* alternative high-frequency clock source (ALTHF).
*
* \section group_ble_configuration_considerations Configuration Considerations
* To configure the BLE ECO clock, call Cy_BLE_EcoConfigure(). 
*
* The following code shows how to configure the BLE ECO clock:
*\code{.c}
*  >> BLE ECO configuration: ECO Frequency: 32 MHZ, Divider: 2, 
*  >> Start-up time(uS): 1500, Load cap(pF): 9.9
*
*  uint32_t loadCap = ((9.9 - 7.5) / 0.075);
*  uint32_t startTime = (1500 / 31.25);
*
*  >> Initialize the BLE ECO clock
*  Cy_BLE_EcoConfigure(CY_BLE_BLESS_ECO_FREQ_32MHZ, CY_BLE_SYS_ECO_CLK_DIV_2, loadCap, startTime, CY_BLE_ECO_VOLTAGE_REG_AUTO);
*    
*  >> Disable the BLE ECO Clock (by soft reset)
*  Cy_BLE_EcoReset();
*
* \endcode
*
* \section group_ble_clk_more_information More Information
* See the BLE chapter of the device technical reference manual (TRM).
*
* \section group_ble_clk_MISRA MISRA-C Compliance
* The BLE ECO clock driver has the following specific deviations:
* <table class="doxtable">
*     <tr>
*       <th>MISRA rule</th>
*       <th>Rule Class (Required/ Advisory)</th>
*       <th>Rule Description</th>
*       <th>Description of Deviation(s)</th>
*     </tr>
*     <tr>
*       <td>1.2</td>
*       <td>R</td>
*       <td>No reliance shall be placed on undefined or unspecified behaviour.</td>
*       <td>This specific behavior is explicitly covered in rule 5.1.</td>
*     </tr>
*     <tr>
*       <td>5.1</td>
*       <td>R</td>
*       <td>Identifiers (internal and external) shall not rely on the significance of more than 31 characters.</td>
*       <td>This rule applies to ISO:C90 standard. PDL conforms to ISO:C99 that does not require this limitation.</td>
*     </tr>
*     <tr>
*       <td>9.3</td>
*       <td>R</td>
*       <td>In an enumerator list, the '=' construct shall not be used to explicitly initialise members other than the
*           first, unless all items are explicitly initialised.</td>
*       <td>There are enumerator lists which depend on configurations (e.g. cy_en_ble_srvi_t) and require to 
*           use the '=' construct for calculating the instance count for the multi-instances services, 
*           such as HIDS, BAS or CUSTOM</td>
*     </tr>
*     <tr>
*       <td>10.1</td>
*       <td>R</td>
*       <td>The value of an expression of integer type shall not be implicitly converted to a different underlying type
*           under some circumstances.</td>
*       <td>An operand of essentially enum type is being converted to unsigned type as a result of an arithmetic or
*           conditional operation. The conversion does not have any unintended effect.</td>
*     </tr>
*     <tr>
*       <td>11.4</td>
*       <td>A</td>
*       <td>A cast should not be performed between a pointer to object type and a different pointer to object type.</td>
*       <td>A cast involving pointers is conducted with caution that the pointers are correctly aligned for the type of
*           object being pointed to.</td>
*     </tr>
*     <tr>
*       <td>11.5</td>
*       <td>A</td>
*       <td>A cast shall not be performed that removes any const or volatile qualification from the type addressed by a
*           pointer.</td>
*       <td>The const or volatile qualification is lost during pointer cast before passing to the stack functions.</td>
*     </tr>
*     <tr>
*       <td>13.7</td>
*       <td>R</td>
*       <td>Boolean operations whose results are invariant shall not be permitted.</td>
*       <td>A Boolean operator can yield a result that can be proven to be always "true" or always "false" in some specific
*           configurations because of generalized implementation approach.</td>
*     </tr>
*     <tr>
*       <td>16.7</td>
*       <td>A</td>
*       <td>The object addressed by the pointer parameter is not modified and so the pointer could be of type 'pointer to
*             const'.</td>
*       <td>The 'base' and 'content' parameters in Cy_BLE_DeepSleepCallback function are not used by BLE but callback type 
*             is universal for all drivers.</td>
*     </tr>
*     <tr>
*       <td>18.4</td>
*       <td>R</td>
*       <td>Unions shall not be used.</td>
*       <td>The use of deviations is acceptable for packing and unpacking of data, for example when sending and 
*           receiving messages, and implementing variant records provided that the variants are differentiated 
*           by a common field. </td>
*     </tr>
*     <tr>
*       <td>19.16</td>
*       <td>R</td>
*       <td>Preprocessing directives shall be syntactically meaningful even when excluded by the preprocessor.</td>
*       <td>The reason for this deviation is that not standard directive "warning" is used. This directive is works 
*           on all compilers which PDL supports (e.g. GCC, IAR, MDK). </td>
*     </tr>
* </table>
*
* \section group_ble_clk_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason of Change</th></tr>
*   <tr>
*     <td>3.0.0</td>
*     <td>Initial Version. <br> The functionality of the BLE ECO clock is migrated from the BLE Middleware to the separated driver (ble_clk)</td>
*     <td>Independent usage of BLE ECO clock without BLE Middleware</td>
*   </tr>
* </table>
*
* \defgroup group_ble_clk_functions Functions
* \defgroup group_ble_clk_data_type Enumerated Types
* \defgroup group_ble_clk_macros Macros
*/


#if !defined(CY_BLE_CLK_H)
#define CY_BLE_CLK_H

#include <stddef.h>
#include "cyip_ble.h"
#include "cy_device_headers.h"

#if defined(CY_IP_MXBLESS)

#include "cy_syslib.h"
#include "cy_gpio.h"
#include "cy_syspm.h"

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
*       Internal Defines
*******************************************************************************/
/**
 * \addtogroup group_ble_clk_macros
 * @{
 */
/** Driver major version */
#define CY_BLE_CLK_DRV_VERSION_MAJOR    (3)

/** Driver minor version */
#define CY_BLE_CLK_DRV_VERSION_MINOR    (0)

/** Driver ID */
#define CY_BLE_CLK_ID                   (0x05ul << 18u)
/** @} */

/** \cond INTERNAL */
/*******************************************************************************
*       Internal Defines
*******************************************************************************/
#define CY_BLE_PORT2_CFG_VAL                                 (0x66666666u)
#define CY_BLE_PORT3_CFG_VAL                                 (0x66EEE666u)
#define CY_BLE_PORT4_CFG_VAL                                 (0x6666E666u)

#define CY_BLE_PORT_CFG_OUT_VAL                              (0xFFFF0000u)

#define CY_BLE_PORT2_HSIOM_SEL0                              (0x1C1C1C1Cu)
#define CY_BLE_PORT2_HSIOM_SEL1                              (0x1C1C1C1Cu)
#define CY_BLE_PORT3_HSIOM_SEL0                              (0x1A1A1A1Cu)
#define CY_BLE_PORT3_HSIOM_SEL1                              (0x00001A1Au)
#define CY_BLE_PORT4_HSIOM_SEL0                              (0x1C001A1Au)
#define CY_BLE_PORT4_HSIOM_SEL1                              (0x00000000u)

#define CY_BLE_DEFAULT_OSC_STARTUP_DELAY_LF                  (25u)
#define CY_BLE_DEFAULT_CAP_TRIM_VALUE                        (32u)
#define CY_BLE_DEFAULT_ECO_FREQ                              (CY_BLE_BLESS_ECO_FREQ_32MHZ)
#define CY_BLE_DEFAULT_ECO_DIV                               (CY_BLE_SYS_ECO_CLK_DIV_4)

#if !defined (CY_BLE_DEFAULT_HVLDO_STARTUP_DELAY)
    #define CY_BLE_DEFAULT_HVLDO_STARTUP_DELAY               (6UL)
#endif /* CY_BLE_DEFAULT_HVLDO_STARTUP_DELAY */

#if !defined (CY_BLE_DEFAULT_ISOLATE_DEASSERT_DELAY)
    #define CY_BLE_DEFAULT_ISOLATE_DEASSERT_DELAY            (0UL)
#endif /* CY_BLE_DEFAULT_ISOLATE_DEASSERT_DELAY */

#if !defined (CY_BLE_DEFAULT_ACT_TO_SWITCH_DELAY)
    #define CY_BLE_DEFAULT_ACT_TO_SWITCH_DELAY               (0UL)
#endif /* CY_BLE_DEFAULT_ACT_TO_SWITCH_DELAY */

#if !defined (CY_BLE_DEFAULT_HVLDO_DISABLE_DELAY)    
    #define CY_BLE_DEFAULT_HVLDO_DISABLE_DELAY               (1UL)
#endif /* CY_BLE_DEFAULT_HVLDO_DISABLE_DELAY */

#if !defined (CY_BLE_DEFAULT_ACT_STARTUP_DELAY)
    #define CY_BLE_DEFAULT_ACT_STARTUP_DELAY                 (4UL)
#endif /* CY_BLE_DEFAULT_ACT_STARTUP_DELAY */

#if !defined (CY_BLE_DEFAULT_DIG_LDO_STARTUP_DELAY)
    #define CY_BLE_DEFAULT_DIG_LDO_STARTUP_DELAY             (0UL)
#endif /* CY_BLE_DEFAULT_DIG_LDO_STARTUP_DELAY */

#if !defined (CY_BLE_DEFAULT_XTAL_DISABLE_DELAY)
    #define CY_BLE_DEFAULT_XTAL_DISABLE_DELAY                (1UL)
#endif /* CY_BLE_DEFAULT_XTAL_DISABLE_DELAY */

#if !defined (CY_BLE_DEFAULT_DIG_LDO_DISABLE_DELAY)
    #define CY_BLE_DEFAULT_DIG_LDO_DISABLE_DELAY             (0UL)
#endif /* CY_BLE_DEFAULT_DIG_LDO_DISABLE_DELAY */

#if !defined (CY_BLE_DEFAULT_VDDR_STABLE_DELAY)
    #define CY_BLE_DEFAULT_VDDR_STABLE_DELAY                 (1UL)
#endif /* CY_BLE_DEFAULT_VDDR_STABLE_DELAY */

#if !defined (CY_BLE_DEFAULT_RCB_CTRL_LEAD)
    #define CY_BLE_DEFAULT_RCB_CTRL_LEAD                     (0x2UL)
#endif /* CY_BLE_DEFAULT_RCB_CTRL_LEAD */

#if !defined (CY_BLE_DEFAULT_RCB_CTRL_LAG)
    #define CY_BLE_DEFAULT_RCB_CTRL_LAG                      (0x2UL)
#endif /* CY_BLE_DEFAULT_RCB_CTRL_LAG */

#if !defined (CY_BLE_DEFAULT_RCB_CTRL_DIV)
    #define CY_BLE_DEFAULT_RCB_CTRL_DIV                      (0x1UL)     /* LL 8 MHz / 2 */
#endif /* CY_BLE_DEFAULT_RCB_CTRL_DIV */

#if !defined (CY_BLE_DEFAULT_RCB_CTRL_FREQ)
    #define CY_BLE_DEFAULT_RCB_CTRL_FREQ                     (4000000UL) /* Default RCB clock is 4 MHz */
#endif /* CY_BLE_DEFAULT_RCB_CTRL_FREQ */

#define CY_BLE_DEFAULT_ECO_CLK_FREQ_32MHZ                    (32000000UL)
#define CY_BLE_DEFAULT_ECO_CLK_FREQ_16MHZ                    (16000000UL)

#define CY_BLE_MXD_RADIO_CLK_BUF_EN_VAL                      (CY_BLE_PMU_MODE_TRANSITION_REG_CLK_ANA_DIG_EN_BIT | \
                                                              CY_BLE_PMU_MODE_TRANSITION_REG_RST_ACT_N_BIT)
#define CY_BLE_MXD_RADIO_DIG_CLK_OUT_EN_VAL                  (CY_BLE_PMU_PMU_CTRL_REG_CLK_CMOS_SEL_BIT)

/* Radio registers */
#define CY_BLE_PMU_MODE_TRANSITION_REG                       (0x1e02u)
#define CY_BLE_PMU_MODE_TRANSITION_REG_CLK_ANA_DIG_EN_BIT    (uint16_t)(1UL << 12u)
#define CY_BLE_PMU_MODE_TRANSITION_REG_RST_ACT_N_BIT         (uint16_t)(1UL << 11u)

#define CY_BLE_PMU_PMU_CTRL_REG                              (0x1e03u)
#define CY_BLE_PMU_PMU_CTRL_REG_CLK_CMOS_SEL_BIT             (uint16_t)(1UL << 10u)

#define CY_BLE_RF_DCXO_CFG_REG                               (0x1e08u)
#define CY_BLE_RF_DCXO_CFG_REG_DCXO_CAP_SHIFT                (1u)
#define CY_BLE_RF_DCXO_CFG_REG_DCXO_CAP_MASK                 (0xffUL)
#define CY_BLE_RF_DCXO_CFG_REG_VALUE                         (0x1001u)

#define CY_BLE_RF_DCXO_CFG2_REG                              (0x1e0fu)
#define CY_BLE_RF_DCXO_CFG2_REG_VALUE                        (0x6837u)

#define CY_BLE_RF_DCXO_BUF_CFG_REG                           (0x1e09u)
#define CY_BLE_RF_DCXO_BUF_CFG_REG_XTAL_32M_SEL_BIT          (uint16_t)(1UL << 6u)
#define CY_BLE_RF_DCXO_BUF_CFG_REG_BUF_AMP_SEL_SHIFT         (4u)
#define CY_BLE_RF_DCXO_BUF_CFG_REG_BUF_AMP_SEL_MASK          (0x03UL)
#define CY_BLE_RF_DCXO_BUF_CFG_REG_CLK_DIV_SHIFT             (0u)
#define CY_BLE_RF_DCXO_BUF_CFG_REG_CLK_DIV_MASK              (0x0fUL)

#define CY_BLE_RF_LDO_CFG_REG                                (0x1e07u)
#define CY_BLE_RF_LDO_CFG_REG_LDO10_CFG_SHIFT                (11u)
#define CY_BLE_RF_LDO_CFG_REG_LDO10_CFG_MASK                 ((1<<2)-1)
#define CY_BLE_RF_LDO_CFG_REG_LDO_ACT_CFG_SHIFT              (7u)
#define CY_BLE_RF_LDO_CFG_REG_LDO_ACT_CFG_MASK               ((1<<4)-1)
#define CY_BLE_RF_LDO_CFG_REG_LDO_IF_CFG_SHIFT               (5u)
#define CY_BLE_RF_LDO_CFG_REG_LDO_IF_CFG_MASK                ((1<<2)-1)

#define CY_BLE_RF_LDO_EN_REG                                 (0x1e06u)
#define CY_BLE_RF_LDO_EN_REG_LDO_RF_CFG_SHIFT                (6u)
#define CY_BLE_RF_LDO_EN_REG_LDO_RF_CFG_MASK                 ((1<<2)-1)

#if !defined (CY_BLE_DELAY_TIME)
    #define CY_BLE_DELAY_TIME                                (1u) /* in us */
#endif /* CY_BLE_DELAY_TIME */

#if !defined (CY_BLE_RCB_TIMEOUT)    
    #define CY_BLE_RCB_TIMEOUT                               (1000u / CY_BLE_DELAY_TIME)   /* 1ms */
#endif /* CY_BLE_RCB_TIMEOUT */

#if !defined (CY_BLE_VIO_TIMEOUT)    
    #define CY_BLE_VIO_TIMEOUT                               (2000u / CY_BLE_DELAY_TIME)   /* 2ms */
#endif /* CY_BLE_VIO_TIMEOUT */

#if !defined (CY_BLE_ACT_TIMEOUT)    
    #define CY_BLE_ACT_TIMEOUT                               (950000u / CY_BLE_DELAY_TIME) /* 950ms */
#endif /* CY_BLE_ACT_TIMEOUT */

#if !defined (CY_BLE_RCB_RETRIES)    
    #define CY_BLE_RCB_RETRIES                               (10u)
#endif /* CY_BLE_RCB_RETRIES */

#define CY_BLE_ECO_SET_TRIM_DELAY_COEF                       (32u)

/* BWC macros */
#define cy_stc_ble_bless_eco_cfg_params_t   cy_stc_ble_eco_config_t
#define cy_en_ble_bless_sys_eco_clk_div_t   cy_en_ble_eco_sys_clk_div_t 
#define cy_en_ble_bless_eco_freq_t          cy_en_ble_eco_freq_t
/** \endcond */

/**
 * \addtogroup group_ble_clk_data_type
 * @{
 */
/*******************************************************************************
*       Data Types
*******************************************************************************/

/** BLE Radio ECO clock divider */
typedef enum
{
    CY_BLE_MXD_RADIO_CLK_DIV_1 = 0u,
    CY_BLE_MXD_RADIO_CLK_DIV_2 = 1u,
    CY_BLE_MXD_RADIO_CLK_DIV_4 = 2u,
    CY_BLE_MXD_RADIO_CLK_DIV_8 = 4u,
    CY_BLE_MXD_RADIO_CLK_DIV_16 = 8u
} cy_en_ble_mxd_radio_clk_div_t;

/** Sine wave buffer output capability select */
typedef enum
{
    CY_BLE_MXD_RADIO_CLK_BUF_AMP_16M_SMALL = 0u,
    CY_BLE_MXD_RADIO_CLK_BUF_AMP_16M_LARGE = 1u,
    CY_BLE_MXD_RADIO_CLK_BUF_AMP_32M_SMALL = 2u,
    CY_BLE_MXD_RADIO_CLK_BUF_AMP_32M_LARGE = 3u
} cy_en_ble_mxd_radio_clk_buf_amp_t;

/** BLESS clock divider */
typedef enum
{
    CY_BLE_BLESS_XTAL_CLK_DIV_1 = 0u,
    CY_BLE_BLESS_XTAL_CLK_DIV_2 = 1u,
    CY_BLE_BLESS_XTAL_CLK_DIV_4 = 2u,
    CY_BLE_BLESS_XTAL_CLK_DIV_8 = 3u
}cy_en_ble_bless_xtal_clk_div_config_llclk_div_t;

/** BLE ECO Clock Frequency. */
typedef enum
{
    /** ECO Frequency of 16MHz */
    CY_BLE_BLESS_ECO_FREQ_16MHZ,

    /** ECO Frequency of 32MHz */
    CY_BLE_BLESS_ECO_FREQ_32MHZ
} cy_en_ble_eco_freq_t;

/** BLE ECO System clock divider */
typedef enum
{
    /** Link Layer clock divider = 1*/
    CY_BLE_SYS_ECO_CLK_DIV_1 = 0x00u,

    /** Link Layer clock divider = 2*/
    CY_BLE_SYS_ECO_CLK_DIV_2,

    /** Link Layer clock divider = 4*/
    CY_BLE_SYS_ECO_CLK_DIV_4,

    /** Link Layer clock divider = 8*/
    CY_BLE_SYS_ECO_CLK_DIV_8,

    /** Invalid Link Layer clock divider*/
    CY_BLE_SYS_ECO_CLK_DIV_INVALID
    
} cy_en_ble_eco_sys_clk_div_t;

/** BLE ECO Clock return value */
typedef enum
{
    /** ECO started successfully */
    CY_BLE_ECO_SUCCESS = 0x00ul,

    /** Invalid input param values */
    CY_BLE_ECO_BAD_PARAM = CY_PDL_STATUS_ERROR | CY_BLE_CLK_ID | 0x0001ul,

    /** RCB is not available for Firmware control to restart ECO */
    CY_BLE_ECO_RCB_CONTROL_LL = CY_PDL_STATUS_ERROR | CY_BLE_CLK_ID | 0x0002ul,

    /** ECO already started */
    CY_BLE_ECO_ALREADY_STARTED = CY_PDL_STATUS_ERROR | CY_BLE_CLK_ID | 0x0003ul,
    
    /** Hardware error */
    CY_BLE_ECO_HARDWARE_ERROR = CY_PDL_STATUS_ERROR | CY_BLE_CLK_ID | 0x0004ul,
    
} cy_en_ble_eco_status_t;

/** BLE Voltage regulator */
typedef enum
{
    /** Use SIMO Buck or BLE LDO regulator depend on system usage */
    CY_BLE_ECO_VOLTAGE_REG_AUTO,
    
    /** Use BLE LDO */
    CY_BLE_ECO_VOLTAGE_REG_BLESSLDO
    
} cy_en_ble_eco_voltage_reg_t;
/** @} */

/*******************************************************************************
*       Configuration Structures
*******************************************************************************/
/** \cond INTERNAL */
/** BLE ECO configuration structures */
typedef struct
{
    /** 
     *  ECO crystal startup time in multiple of 31.25us (startup_time_from_user min - 31.25us)
     *  ecoXtalStartUpTime = startup_time_from_user/31.25 
     */
    uint8_t                           ecoXtalStartUpTime;

    /** 
     *  ECO crystal load capacitance - In multiple of 0.075pF (pF_from_user min - 7.5pF, pF_from_user max - 26.625pF)
     *  loadcap = ((pF_from_user - 7.5)/0.075) 
     */
    uint8_t                           loadCap;

    /** ECO Frequency. */
    cy_en_ble_bless_eco_freq_t        ecoFreq;

    /** System divider for ECO clock. */
    cy_en_ble_bless_sys_eco_clk_div_t ecoSysDiv;
        
} cy_stc_ble_eco_config_t;
/** \endcond */


/**
 * \addtogroup group_ble_clk_functions
 * @{
 */
/*******************************************************************************
*       Function Prototypes
*******************************************************************************/
cy_en_ble_eco_status_t Cy_BLE_EcoConfigure(cy_en_ble_eco_freq_t freq,
                                           cy_en_ble_eco_sys_clk_div_t sysClkDiv, 
                                           uint32_t cLoad, uint32_t xtalStartUpTime,
                                           cy_en_ble_eco_voltage_reg_t voltageReg);

void Cy_BLE_EcoReset(void);
/** @} */

/** \cond INTERNAL */
cy_en_ble_eco_status_t Cy_BLE_EcoStart(const cy_stc_ble_eco_config_t *config);
void Cy_BLE_EcoStop(void);

/*******************************************************************************
*       Private Function Prototypes
*******************************************************************************/
void Cy_BLE_HAL_Init(void);
/** \endcond */


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* defined(CY_IP_MXBLESS) */
#endif /* CY_BLE_CLK_H */


/* [] END OF FILE */

