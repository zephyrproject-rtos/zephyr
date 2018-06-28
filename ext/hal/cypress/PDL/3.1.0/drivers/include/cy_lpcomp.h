/***************************************************************************//**
* \file cy_lpcomp.h
* \version 1.20
*
*  This file provides constants and parameter values for the Low Power Comparator driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_lpcomp Low Power Comparator (LPComp)
* \{
* Provides access to the low-power comparators implemented using the fixed-function
* LP comparator block that is present in PSoC 6.
*
* These comparators can perform fast analog signal comparison of internal 
* and external analog signals in all system power modes. Low-power comparator 
* output can be inspected by the CPU, used as an interrupt/wakeup source to the 
* CPU when in low-power mode (Sleep, Low-Power Sleep, or Deep-Sleep), used as 
* a wakeup source to system resources when in Hibernate mode, or fed to DSI as 
* an asynchronous or synchronous signal (level or pulse).
*
* \section group_lpcomp_section_Configuration_Considerations Configuration Considerations
* To set up an LPComp, the inputs, the output, the mode, the interrupts and 
* other configuration parameters should be configured. Power the LPComp to operate.
*
* The sequence recommended for the LPComp operation:
*
* 1) To initialize the driver, call the  Cy_LPComp_Init() function providing 
* the filled cy_stc_lpcomp_config_t structure, the LPComp channel number, 
* and the LPCOMP registers structure pointer.
*
* 2) Optionally, configure the interrupt requests if the interrupt event 
* triggering is needed. Use the Cy_LPComp_SetInterruptMask() function with 
* the parameter for the mask available in the configuration file. 
* Additionally, enable the Global interrupts and initialize the referenced 
* interrupt by setting the priority and the interrupt vector using 
* the \ref Cy_SysInt_Init() function of the sysint driver.
*
* 3) Configure the inputs and the output using the \ref Cy_GPIO_Pin_Init() 
* functions of the GPIO driver. 
* The High Impedance Analog drive mode is for the inputs and 
* the Strong drive mode is for the output. 
* Use the Cy_LPComp_SetInputs() function to connect the comparator inputs 
* to the dedicated IO pins, AMUXBUSA/AMUXBUSB or Vref:
* \image html lpcomp_inputs.png
*
* 4) Power on the comparator using the Cy_LPComp_Enable() function.
*
* 5) The comparator output can be monitored using 
* the Cy_LPComp_GetCompare() function or using the LPComp interrupt 
* (if the interrupt is enabled).
*
* \note The interrupt is not cleared automatically. 
* It is the user's responsibility to do that. 
* The interrupt is cleared by writing a 1 in the corresponding interrupt 
* register bit position. The preferred way to clear interrupt sources 
* is using the Cy_LPComp_ClearInterrupt() function.
*
* \note Individual comparator interrupt outputs are ORed together 
* as a single asynchronous interrupt source before it is sent out and 
* used to wake up the system in the low-power mode. 
* For PSoC 6 devices, the individual comparator interrupt is masked 
* by the INTR_MASK register. The masked result is captured in
* the INTR_MASKED register.
* Writing a 1 to the INTR register bit will clear the interrupt. 
*
* \section group_lpcomp_lp Low Power Support
* The LPComp provides the callback functions to facilitate 
* the low-power mode transition. The callback 
* \ref Cy_LPComp_DeepSleepCallback must be called during execution 
* of \ref Cy_SysPm_DeepSleep; \ref Cy_LPComp_HibernateCallback must be 
* called during execution of \ref Cy_SysPm_Hibernate. 
* To trigger the callback execution, the callback must be registered 
* before calling the mode transition function. 
* Refer to \ref group_syspm driver for more 
* information about low-power mode transitions.
*
* \section group_lpcomp_more_information More Information
*
* Refer to the appropriate device technical reference manual (TRM) for 
* a detailed description of the registers.
*
* \section group_lpcomp_MISRA MISRA-C Compliance
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>11.4</td>
*     <td>A</td>
*     <td>A cast should not be performed between a pointer to object type and
*         a different pointer to object type.</td>
*     <td>
*         The pointer to the buffer memory is void to allow handling different
*         different data types: uint8_t (4-8 bits) or uint16_t (9-16 bits). 
*         The cast operation is safe because the configuration is verified 
*         before operation is performed.
*         The function \ref Cy_LPComp_DeepSleepCallback is a callback of 
*         the \ref cy_en_syspm_status_t type. The cast operation safety in this
*         function becomes the user's responsibility because the pointers are 
*         initialized when a callback is registered in the SysPm driver.</td>
*   </tr>
* </table>
*
* \section group_lpcomp_Changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.20</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>1.10.1</td>
*     <td>Added Low Power Callback section</td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td>1.10</td>
*     <td>The CY_WEAK keyword is removed from Cy_LPComp_DeepSleepCallback() 
*         and Cy_LPComp_HibernateCallback() functions<br>
*         Added input parameter validation to the API functions.</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_lpcomp_macros Macros
* \defgroup group_lpcomp_functions Functions
*   \{
*       \defgroup group_lpcomp_functions_syspm_callback  Low Power Callback
*   \}
* \defgroup group_lpcomp_data_structures Data Structures
* \defgroup group_lpcomp_enums Enumerated Types
*/

#ifndef CY_LPCOMP_PDL_H
#define CY_LPCOMP_PDL_H

/******************************************************************************/
/* Include files                                                              */
/******************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_syspm.h"

#ifdef CY_IP_MXLPCOMP

#ifdef __cplusplus
extern "C"
{
#endif

/**
* \addtogroup group_lpcomp_macros
* \{
*/

/** Driver major version */
#define CY_LPCOMP_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define CY_LPCOMP_DRV_VERSION_MINOR       20

/******************************************************************************
* API Constants
******************************************************************************/

/**< LPCOMP PDL ID */
#define CY_LPCOMP_ID                                CY_PDL_DRV_ID(0x23u)    

/** The LPCOMP's number of channels. */
#define CY_LPCOMP_MAX_CHANNEL_NUM                   (2u)

/** LPCOMP's comparator 1 interrupt mask. */
#define CY_LPCOMP_COMP0                             (0x01u)
/** LPCOMP's comparator 2 interrupt mask. */
#define CY_LPCOMP_COMP1                             (0x02u)

/** \cond INTERNAL_MACROS */


/******************************************************************************
* Registers Constants
******************************************************************************/

#define CY_LPCOMP_MODE_ULP_Pos                      (0x0uL)
#define CY_LPCOMP_MODE_ULP_Msk                      (0x1uL)

#define CY_LPCOMP_INTR_Pos                          (LPCOMP_INTR_COMP0_Pos)
#define CY_LPCOMP_INTR_Msk                          (LPCOMP_INTR_COMP0_Msk | LPCOMP_INTR_COMP1_Msk)

#define CY_LPCOMP_CMP0_SW_POS_Msk                   (LPCOMP_CMP0_SW_CMP0_IP0_Msk | \
                                                    LPCOMP_CMP0_SW_CMP0_AP0_Msk | \
                                                    LPCOMP_CMP0_SW_CMP0_BP0_Msk)
#define CY_LPCOMP_CMP0_SW_NEG_Msk                   (LPCOMP_CMP0_SW_CMP0_IN0_Msk | \
                                                    LPCOMP_CMP0_SW_CMP0_AN0_Msk | \
                                                    LPCOMP_CMP0_SW_CMP0_BN0_Msk | \
                                                    LPCOMP_CMP0_SW_CMP0_VN0_Msk)
#define CY_LPCOMP_CMP1_SW_POS_Msk                   (LPCOMP_CMP1_SW_CMP1_IP1_Msk | \
                                                    LPCOMP_CMP1_SW_CMP1_AP1_Msk | \
                                                    LPCOMP_CMP1_SW_CMP1_BP1_Msk)
#define CY_LPCOMP_CMP1_SW_NEG_Msk                   (LPCOMP_CMP1_SW_CMP1_IN1_Msk | \
                                                    LPCOMP_CMP1_SW_CMP1_AN1_Msk | \
                                                    LPCOMP_CMP1_SW_CMP1_BN1_Msk | \
                                                    LPCOMP_CMP1_SW_CMP1_VN1_Msk)

#define CY_LPCOMP_CMP0_OUTPUT_CONFIG_Pos            LPCOMP_CMP0_CTRL_DSI_BYPASS0_Pos
#define CY_LPCOMP_CMP1_OUTPUT_CONFIG_Pos            LPCOMP_CMP1_CTRL_DSI_BYPASS1_Pos
                                                    
#define CY_LPCOMP_CMP0_OUTPUT_CONFIG_Msk           (LPCOMP_CMP0_CTRL_DSI_BYPASS0_Msk | \
                                                    LPCOMP_CMP0_CTRL_DSI_LEVEL0_Msk)
                                                    
#define CY_LPCOMP_CMP1_OUTPUT_CONFIG_Msk           (LPCOMP_CMP1_CTRL_DSI_BYPASS1_Msk | \
                                                    LPCOMP_CMP1_CTRL_DSI_LEVEL1_Msk)

#define CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SL_SR_Pos HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SL_Pos
                                                    
#define CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SL_SR_Msk (HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SL_Msk | \
                                                     HSIOM_AMUX_SPLIT_CTL_SWITCH_AA_SR_Msk) 
                                                    
#define CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SL_SR_Pos HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SL_Pos
                                                    
#define CY_HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SL_SR_Msk (HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SL_Msk | \
                                                     HSIOM_AMUX_SPLIT_CTL_SWITCH_BB_SR_Msk)

#define CY_LPCOMP_REF_CONNECTED                     (1u)

#define CY_LPCOMP_WAKEUP_PIN0_Msk                   CY_SYSPM_WAKEUP_LPCOMP0
#define CY_LPCOMP_WAKEUP_PIN1_Msk                   CY_SYSPM_WAKEUP_LPCOMP1

/* Internal constants for Cy_LPComp_Enable() */
#define CY_LPCOMP_NORMAL_POWER_DELAY               (3u)
#define CY_LPCOMP_LP_POWER_DELAY                   (6u)
#define CY_LPCOMP_ULP_POWER_DELAY                  (50u)

/** \endcond */
/** \} group_lpcomp_macros */

/**
* \addtogroup group_lpcomp_enums
* \{
*/

/******************************************************************************
 * Enumerations
 *****************************************************************************/
/** The LPCOMP output modes. */
typedef enum
{
    CY_LPCOMP_OUT_PULSE  = 0u,  /**< The LPCOMP DSI output with the pulse option, no bypass. */
    CY_LPCOMP_OUT_DIRECT = 1u,  /**< The LPCOMP bypass mode, the direct output of a comparator. */
    CY_LPCOMP_OUT_SYNC   = 2u   /**< The LPCOMP DSI output with the level option, it is similar 
                                  to the bypass mode but it is 1 cycle slow than the bypass. */
} cy_en_lpcomp_out_t;

/** The LPCOMP hysteresis modes. */
typedef enum
{
    CY_LPCOMP_HYST_ENABLE  = 1u,  /**< The LPCOMP enable hysteresis. */
    CY_LPCOMP_HYST_DISABLE = 0u   /**< The LPCOMP disable hysteresis. */
} cy_en_lpcomp_hyst_t;

/** The LPCOMP's channel number. */
typedef enum
{
    CY_LPCOMP_CHANNEL_0  = 0x1u,  /**< The LPCOMP Comparator 0. */
    CY_LPCOMP_CHANNEL_1  = 0x2u   /**< The LPCOMP Comparator 1. */
} cy_en_lpcomp_channel_t;

/** The LPCOMP interrupt modes. */
typedef enum
{
    CY_LPCOMP_INTR_DISABLE = 0u,  /**< The LPCOMP interrupt disabled, no interrupt will be detected. */
    CY_LPCOMP_INTR_RISING  = 1u,  /**< The LPCOMP interrupt on the rising edge. */
    CY_LPCOMP_INTR_FALLING = 2u,  /**< The LPCOMP interrupt on the falling edge. */
    CY_LPCOMP_INTR_BOTH    = 3u   /**< The LPCOMP interrupt on both rising and falling edges. */
} cy_en_lpcomp_int_t;

/** The LPCOMP power-mode selection. */
typedef enum
{
    CY_LPCOMP_MODE_OFF     = 0u,  /**< The LPCOMP's channel power-off. */
    CY_LPCOMP_MODE_ULP     = 1u,  /**< The LPCOMP's channel ULP mode. */
    CY_LPCOMP_MODE_LP      = 2u,  /**< The LPCOMP's channel LP mode. */
    CY_LPCOMP_MODE_NORMAL  = 3u   /**< The LPCOMP's channel normal mode. */
} cy_en_lpcomp_pwr_t;

/** The LPCOMP inputs. */
typedef enum
{
    CY_LPCOMP_SW_GPIO       = 0x01u,  /**< The LPCOMP input connects to GPIO pin. */
    CY_LPCOMP_SW_AMUXBUSA   = 0x02u,  /**< The LPCOMP input connects to AMUXBUSA. */
    CY_LPCOMP_SW_AMUXBUSB   = 0x04u,  /**< The LPCOMP input connects to AMUXBUSB. */
    CY_LPCOMP_SW_LOCAL_VREF = 0x08u   /**< The LPCOMP input connects to local VREF. */
} cy_en_lpcomp_inputs_t;

/** The LPCOMP error codes. */
typedef enum 
{
    CY_LPCOMP_SUCCESS = 0x00u,                                            /**< Successful */
    CY_LPCOMP_BAD_PARAM = CY_LPCOMP_ID | CY_PDL_STATUS_ERROR | 0x01u,     /**< One or more invalid parameters */
    CY_LPCOMP_TIMEOUT = CY_LPCOMP_ID | CY_PDL_STATUS_ERROR | 0x02u,       /**< Operation timed out */
    CY_LPCOMP_INVALID_STATE = CY_LPCOMP_ID | CY_PDL_STATUS_ERROR | 0x03u, /**< Operation not setup or is in an improper state */
    CY_LPCOMP_UNKNOWN = CY_LPCOMP_ID | CY_PDL_STATUS_ERROR | 0xFFu,       /**< Unknown failure */
} cy_en_lpcomp_status_t;

/** \} group_lpcomp_enums */

/**
* \addtogroup group_lpcomp_data_structures
* \{
*/

/******************************************************************************
 * Structures
 *****************************************************************************/

/** The LPCOMP configuration structure. */
typedef struct {
    cy_en_lpcomp_out_t  outputMode;  /**< The LPCOMP's outputMode: Direct output, 
                                       Synchronized output or Pulse output */
    cy_en_lpcomp_hyst_t hysteresis;  /**< Enables or disables the LPCOMP's hysteresis */
    cy_en_lpcomp_pwr_t power;        /**< Sets the LPCOMP power mode */
    cy_en_lpcomp_int_t intType;      /**< Sets the LPCOMP interrupt mode */
} cy_stc_lpcomp_config_t;

/** \cond CONTEXT_STRUCTURE */

typedef struct {
    cy_en_lpcomp_int_t intType[CY_LPCOMP_MAX_CHANNEL_NUM];
    cy_en_lpcomp_pwr_t power[CY_LPCOMP_MAX_CHANNEL_NUM];
} cy_stc_lpcomp_context_t;

/** \endcond */

/** \} group_lpcomp_data_structures */

/** \cond INTERNAL_MACROS */

/******************************************************************************
 * Macros
 *****************************************************************************/
#define CY_LPCOMP_IS_CHANNEL_VALID(channel)      (((channel) == CY_LPCOMP_CHANNEL_0) || \
                                                  ((channel) == CY_LPCOMP_CHANNEL_1))
#define CY_LPCOMP_IS_OUT_MODE_VALID(mode)        (((mode) == CY_LPCOMP_OUT_PULSE) || \
                                                  ((mode) == CY_LPCOMP_OUT_DIRECT) || \
                                                  ((mode) == CY_LPCOMP_OUT_SYNC))
#define CY_LPCOMP_IS_HYSTERESIS_VALID(hyst)      (((hyst) == CY_LPCOMP_HYST_ENABLE) || \
                                                  ((hyst) == CY_LPCOMP_HYST_DISABLE))
#define CY_LPCOMP_IS_INTR_MODE_VALID(intr)       (((intr) == CY_LPCOMP_INTR_DISABLE) || \
                                                  ((intr) == CY_LPCOMP_INTR_RISING) || \
                                                  ((intr) == CY_LPCOMP_INTR_FALLING) || \
                                                  ((intr) == CY_LPCOMP_INTR_BOTH))
#define CY_LPCOMP_IS_POWER_VALID(power)          (((power) == CY_LPCOMP_MODE_OFF) || \
                                                  ((power) == CY_LPCOMP_MODE_ULP) || \
                                                  ((power) == CY_LPCOMP_MODE_LP) || \
                                                  ((power) == CY_LPCOMP_MODE_NORMAL))
#define CY_LPCOMP_IS_INTR_VALID(intr)            (((intr) == CY_LPCOMP_COMP0) || \
                                                  ((intr) == CY_LPCOMP_COMP1) || \
                                                  ((intr) == (CY_LPCOMP_COMP0 | CY_LPCOMP_COMP1)))
#define CY_LPCOMP_IS_INPUT_P_VALID(input)        (((input) == CY_LPCOMP_SW_GPIO) || \
                                                  ((input) == CY_LPCOMP_SW_AMUXBUSA) || \
                                                  ((input) == CY_LPCOMP_SW_AMUXBUSB))
#define CY_LPCOMP_IS_INPUT_N_VALID(input)        (((input) == CY_LPCOMP_SW_GPIO) || \
                                                  ((input) == CY_LPCOMP_SW_AMUXBUSA) || \
                                                  ((input) == CY_LPCOMP_SW_AMUXBUSB) || \
                                                  ((input) == CY_LPCOMP_SW_LOCAL_VREF))
                                                  
/** \endcond */

/**
* \addtogroup group_lpcomp_functions
* \{
*/

/******************************************************************************
* Functions
*******************************************************************************/

cy_en_lpcomp_status_t Cy_LPComp_Init(LPCOMP_Type *base, cy_en_lpcomp_channel_t channel, const cy_stc_lpcomp_config_t *config);
void Cy_LPComp_Enable(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel);
void Cy_LPComp_Disable(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel);
__STATIC_INLINE void Cy_LPComp_GlobalEnable(LPCOMP_Type *base);
__STATIC_INLINE void Cy_LPComp_GlobalDisable(LPCOMP_Type *base);
__STATIC_INLINE void Cy_LPComp_UlpReferenceEnable(LPCOMP_Type *base);
__STATIC_INLINE void Cy_LPComp_UlpReferenceDisable(LPCOMP_Type *base);
__STATIC_INLINE uint32_t Cy_LPComp_GetCompare(LPCOMP_Type const * base, cy_en_lpcomp_channel_t channel);
void Cy_LPComp_SetPower(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_pwr_t power);
void Cy_LPComp_SetHysteresis(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_hyst_t hysteresis);
void Cy_LPComp_SetInputs(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_inputs_t inputP, cy_en_lpcomp_inputs_t inputN);
void Cy_LPComp_SetOutputMode(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_out_t outType);
void Cy_LPComp_SetInterruptTriggerMode(LPCOMP_Type* base, cy_en_lpcomp_channel_t channel, cy_en_lpcomp_int_t intType);
__STATIC_INLINE uint32_t Cy_LPComp_GetInterruptStatus(LPCOMP_Type const * base);
__STATIC_INLINE void Cy_LPComp_ClearInterrupt(LPCOMP_Type* base, uint32_t interrupt);
__STATIC_INLINE void Cy_LPComp_SetInterrupt(LPCOMP_Type* base, uint32_t interrupt);
__STATIC_INLINE uint32_t Cy_LPComp_GetInterruptMask(LPCOMP_Type const * base);
__STATIC_INLINE void Cy_LPComp_SetInterruptMask(LPCOMP_Type* base, uint32_t interrupt);
__STATIC_INLINE uint32_t Cy_LPComp_GetInterruptStatusMasked(LPCOMP_Type const * base);
__STATIC_INLINE void Cy_LPComp_ConnectULPReference(LPCOMP_Type *base, cy_en_lpcomp_channel_t channel);
/** \addtogroup group_lpcomp_functions_syspm_callback
* The driver supports SysPm callback for Deep Sleep and Hibernate transition.
* \{
*/
cy_en_syspm_status_t Cy_LPComp_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams);
cy_en_syspm_status_t Cy_LPComp_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams);
/** \} */


/*******************************************************************************
* Function Name: Cy_LPComp_GlobalEnable
****************************************************************************//**
*
* Activates the IP of the LPCOMP hardware block. This API should be enabled 
* before operating any channel of comparators.
* Note: Interrupts can be enabled after the block is enabled and the appropriate 
* start-up time has elapsed:
* 3 us for the normal power mode;
* 6 us for the LP mode;
* 50 us for the ULP mode.
*
* \param *base
*     The structure of the channel pointer. 
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_GlobalEnable(LPCOMP_Type* base)
{
    LPCOMP_CONFIG(base) |= LPCOMP_CONFIG_ENABLED_Msk;
}


/*******************************************************************************
* Function Name: Cy_LPComp_GlobalDisable
****************************************************************************//**
*
* Deactivates the IP of the LPCOMP hardware block. 
* (Analog in power down, open all switches, all clocks off).
*
* \param *base
*     The structure of the channel pointer.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_GlobalDisable(LPCOMP_Type *base)
{
    LPCOMP_CONFIG(base) &= (uint32_t) ~LPCOMP_CONFIG_ENABLED_Msk;
}


/*******************************************************************************
* Function Name: Cy_LPComp_UlpReferenceEnable
****************************************************************************//**
*
* Enables the local reference-generator circuit. 
*
* \param *base
*     The structure of the channel pointer.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_UlpReferenceEnable(LPCOMP_Type *base)
{
    LPCOMP_CONFIG(base) |= LPCOMP_CONFIG_LPREF_EN_Msk;
}


/*******************************************************************************
* Function Name: Cy_LPComp_UlpReferenceDisable
****************************************************************************//**
*
* Disables the local reference-generator circuit. 
*
* \param *base
*     The structure of the channel pointer.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_UlpReferenceDisable(LPCOMP_Type *base)
{
    LPCOMP_CONFIG(base) &= (uint32_t) ~LPCOMP_CONFIG_LPREF_EN_Msk;
}


/*******************************************************************************
* Function Name: Cy_LPComp_GetCompare
****************************************************************************//**
*
* This function returns a nonzero value when the voltage connected to the 
* positive input is greater than the negative input voltage. 
*
* \param *base
*     The LPComp register structure pointer.
*
* \param channel
*     The LPComp channel index.
*
* \return LPComp compare result.
*     The value is a nonzero value when the voltage connected to the positive
*     input is greater than the negative input voltage.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_LPComp_GetCompare(LPCOMP_Type const * base, cy_en_lpcomp_channel_t channel)
{
    uint32_t result;
 
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));

    if (CY_LPCOMP_CHANNEL_0 == channel)
    {
        result = _FLD2VAL(LPCOMP_STATUS_OUT0, LPCOMP_STATUS(base));
    }
    else
    {
        result = _FLD2VAL(LPCOMP_STATUS_OUT1, LPCOMP_STATUS(base));
    }
 
    return (result);
}


/*******************************************************************************
* Function Name: Cy_LPComp_SetInterruptMask
****************************************************************************//**
*
*  Configures which bits of the interrupt request register will trigger an 
*  interrupt event.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param interrupt
*  uint32_t interruptMask: Bit Mask of interrupts to set. 
*  Bit 0: COMP0 Interrupt Mask 
*  Bit 1: COMP1 Interrupt Mask
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_SetInterruptMask(LPCOMP_Type* base, uint32_t interrupt)
{
    CY_ASSERT_L2(CY_LPCOMP_IS_INTR_VALID(interrupt));

    LPCOMP_INTR_MASK(base) |= interrupt;
}


/*******************************************************************************
* Function Name: Cy_LPComp_GetInterruptMask
****************************************************************************//**
*
*  Returns an interrupt mask.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \return bit mapping information
*   Bit 0: COMP0 Interrupt Mask
*   Bit 1: COMP1 Interrupt Mask
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_LPComp_GetInterruptMask(LPCOMP_Type const * base)
{
    return (LPCOMP_INTR_MASK(base));
}


/*******************************************************************************
* Function Name: Cy_LPComp_GetInterruptStatusMasked
****************************************************************************//**
*
* Returns an interrupt request register masked by an interrupt mask. 
* Returns the result of the bitwise AND operation between the corresponding 
* interrupt request and mask bits.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \return bit mapping information
*   Bit 0: COMP0 Interrupt Masked
*   Bit 1: COMP1 Interrupt Masked
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_LPComp_GetInterruptStatusMasked(LPCOMP_Type const * base)
{
    return (LPCOMP_INTR_MASKED(base));
}


/*******************************************************************************
* Function Name: Cy_LPComp_GetInterruptStatus
****************************************************************************//**
*
* Returns the status of 2 different LPCOMP interrupt requests.
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \return bit mapping information 
*   Bit 0: COMP0 Interrupt status 
*   Bit 1: COMP1 Interrupt status
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_LPComp_GetInterruptStatus(LPCOMP_Type const * base)
{
    return (_FLD2VAL(CY_LPCOMP_INTR, LPCOMP_INTR(base)));
}


/*******************************************************************************
* Function Name: Cy_LPComp_ClearInterrupt
****************************************************************************//**
*
*  Clears LPCOMP interrupts by setting each bit. 
*
* \param *base
*     The LPCOMP register structure pointer.
*
* \param interrupt
*   Bit 0: COMP0 Interrupt status 
*   Bit 1: COMP1 Interrupt status
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_ClearInterrupt(LPCOMP_Type* base, uint32_t interrupt)
{
    CY_ASSERT_L2(CY_LPCOMP_IS_INTR_VALID(interrupt));
    LPCOMP_INTR(base) |= interrupt;
    (void) LPCOMP_INTR(base);
}


/*******************************************************************************
* Function Name: Cy_LPComp_SetInterrupt
****************************************************************************//**
*
*  Sets a software interrupt request. 
*  This function is used in the case of combined interrupt signal from the global
*  signal reference. This function from either component instance can be used 
*  to trigger either or both software interrupts. It sets the INTR_SET interrupt mask.
*
* \param *base
*    The LPCOMP register structure pointer.
*
* \param interrupt
*   Bit 0: COMP0 Interrupt status 
*   Bit 1: COMP1 Interrupt status
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_SetInterrupt(LPCOMP_Type* base, uint32_t interrupt)
{
    CY_ASSERT_L2(CY_LPCOMP_IS_INTR_VALID(interrupt));
    LPCOMP_INTR_SET(base) = interrupt;
}


/*******************************************************************************
* Function Name: Cy_LPComp_ConnectULPReference
****************************************************************************//**
*
* Connects the local reference generator output to the comparator negative input.
*
* \param *base
* The LPCOMP register structure pointer.
*
* \param channel
* The LPCOMP channel index.
*
* \return None
*
*******************************************************************************/
__STATIC_INLINE void Cy_LPComp_ConnectULPReference(LPCOMP_Type *base, cy_en_lpcomp_channel_t channel)
{
    CY_ASSERT_L3(CY_LPCOMP_IS_CHANNEL_VALID(channel));
    
    if (CY_LPCOMP_CHANNEL_0 == channel)
    {
        LPCOMP_CMP0_SW_CLEAR(base) = CY_LPCOMP_CMP0_SW_NEG_Msk;
        LPCOMP_CMP0_SW(base) = _CLR_SET_FLD32U(LPCOMP_CMP0_SW(base), LPCOMP_CMP0_SW_CMP0_VN0, CY_LPCOMP_REF_CONNECTED);
    }
    else
    {
        LPCOMP_CMP1_SW_CLEAR(base) = CY_LPCOMP_CMP1_SW_NEG_Msk;
        LPCOMP_CMP1_SW(base) = _CLR_SET_FLD32U(LPCOMP_CMP1_SW(base), LPCOMP_CMP1_SW_CMP1_VN1, CY_LPCOMP_REF_CONNECTED);
    }
}

/** \} group_lpcomp_functions */

#ifdef __cplusplus
}
#endif

#endif /* CY_IP_MXLPCOMP */

#endif /* CY_LPCOMP_PDL_H */

/** \} group_lpcomp */

/* [] END OF FILE */
