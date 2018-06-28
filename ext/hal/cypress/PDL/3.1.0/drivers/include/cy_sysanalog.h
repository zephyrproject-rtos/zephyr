/***************************************************************************//**
* \file cy_sysanalog.h
* \version 1.10
*
* Header file for the system level analog reference driver.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_sysanalog System Analog Reference Block (SysAnalog)
* \{
*
* This driver provides an interface for configuring the Analog Reference (AREF) block
* and querying the INTR_CAUSE register of the PASS.
*
* The AREF block has the following features:
*
*   - Generates a voltage reference (VREF) from one of three sources:
*       - Local 1.2 V reference (<b>low noise, optimized for analog performance</b>)
*       - Reference from the SRSS (high noise, not recommended for analog performance)
*       - An external pin
*   - Generates a 1 uA "zero dependency to absolute temperature" (IZTAT) current reference
*     that is independent of temperature variations. It can come from one of two sources:
*       - Local reference (<b>low noise, optimized for analog performance</b>)
*       - Reference from the SRSS (high noise, not recommended for analog performance)
*   - Generates a "proportional to absolute temperature" (IPTAT) current reference
*   - Option to enable local references in Deep Sleep mode
*
* The locally generated references are the recommended sources for blocks in the PASS because
* they have tighter accuracy, temperature stability, and lower noise than the SRSS references.
* The SRSS references can be used to save power if the low accuracy and higher noise can be tolerated.
*
* \image html aref_block_diagram.png
* \image latex aref_block_diagram.png
*
* The outputs of the AREF are consumed by multiple blocks in the PASS and by the CapSense (CSDv2) block.
* In some cases, these blocks have the option of using the references from the AREF. This selection would be
* in the respective drivers for these blocks. In some cases, these blocks require the references from the
* AREF to function.
*
* <table class="doxtable">
*   <tr><th>AREF Output</th><th>\ref group_sar "SAR"</th><th>\ref group_ctdac "CTDAC"</th><th>\ref group_ctb "CTB"</th><th>CSDv2</th></tr>
*   <tr>
*     <td>VREF</td>
*     <td>optional</td>
*     <td>optional</td>
*     <td>--</td>
*     <td>optional</td>
*   </tr>
*   <tr>
*     <td>IZTAT</td>
*     <td><b>required</b></td>
*     <td>--</td>
*     <td>optional</td>
*     <td>optional</td>
*   </tr>
*   <tr>
*     <td>IPTAT</td>
*     <td>--</td>
*     <td>--</td>
*     <td><b>required</b></td>
*     <td>--</td>
*   </tr>
* </table>
*
* This driver provides a function to query the INTR_CAUSE register of the PASS.
* There are two interrupts in the PASS:
*
*   -# one global interrupt for all CTBs (up to 4)
*   -# one global interrupt for all CTDACs (up to 4)
*
* Because the interrupts are global, the INTR_CAUSE register is needed to query which hardware instance
* triggered the interrupt.
*
* \section group_sysanalog_usage Usage
*
* \subsection group_sysanalog_usage_init Initialization
*
* To configure the AREF, call \ref Cy_SysAnalog_Init and provide a pointer
* to the configuration structure, \ref cy_stc_sysanalog_config_t. Three predefined structures
* are provided in this driver to cover a majority of use cases:
*
*   - \ref Cy_SysAnalog_Fast_Local <b>(recommended for analog performance)</b>
*   - \ref Cy_SysAnalog_Fast_SRSS
*   - \ref Cy_SysAnalog_Fast_External
*
* After initialization, call \ref Cy_SysAnalog_Enable to enable the hardware.
*
* \subsection group_sysanalog_usage_dsop Deep Sleep Operation
*
* The AREF current and voltage references can be enabled to operate in Deep Sleep mode
* with \ref Cy_SysAnalog_SetDeepSleepMode. There are four options for Deep Sleep operation:
*
*   - \ref CY_SYSANALOG_DEEPSLEEP_DISABLE : Disable AREF IP block
*   - \ref CY_SYSANALOG_DEEPSLEEP_IPTAT_1 : Enable IPTAT generator for fast wakeup from Deep Sleep mode. IPTAT outputs for CTBs are disabled.
*   - \ref CY_SYSANALOG_DEEPSLEEP_IPTAT_2 : Enable IPTAT generator and IPTAT outputs for CTB
*   - \ref CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF : Enable all generators and outputs: IPTAT, IZTAT, and VREF
*
* Recall that the CTB requires the IPTAT reference. For the CTB to operate at the 1 uA current mode in Deep Sleep mode,
* the AREF must be enabled for \ref CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF. For the CTB to operate at the 100 nA
* current mode in Deep Sleep mode, the AREF must be enabled for \ref CY_SYSANALOG_DEEPSLEEP_IPTAT_2 minimum. In this
* lower current mode, the AREF IPTAT must be redirected to the CTB IZTAT. See the high level function \ref
* Cy_CTB_SetCurrentMode in the CTB PDL driver.
*
* If the CTDAC is configured to use the VREF in Deep Sleep mode, the AREF must be enabled for \ref CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF.
*
* \note
* The SRSS references are not available to the AREF in Deep Sleep mode. When operating
* in Deep Sleep mode, the local or external references must be selected.
*
* \section group_sysanalog_more_information More Information
*
* For more information on the AREF, refer to the technical reference manual (TRM).
*
* \section group_sysanalog_MISRA MISRA-C Compliance]
*
* This driver does not have any specific deviations.
*
* \section group_sysanalog_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.10</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_sysanalog_macros Macros
* \defgroup group_sysanalog_functions Functions
* \defgroup group_sysanalog_globals Global Variables
* \defgroup group_sysanalog_data_structures Data Structures
* \defgroup group_sysanalog_enums Enumerated Types
*/

#if !defined(CY_SYSANALOG_H)
#define CY_SYSANALOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cy_device_headers.h"
#include "cy_device.h"
#include "cy_syslib.h"
#include "cy_syspm.h"

#ifdef CY_IP_MXS40PASS

#if defined(__cplusplus)
extern "C" {
#endif


/** \addtogroup group_sysanalog_macros
* \{
*/

/** Driver major version */
#define CY_SYSANALOG_DRV_VERSION_MAJOR          1

/** Driver minor version */
#define CY_SYSANALOG_DRV_VERSION_MINOR          10

/** PASS driver identifier */
#define CY_SYSANALOG_ID                         CY_PDL_DRV_ID(0x17u)

/** \cond INTERNAL */
#define CY_SYSANALOG_DEINIT                     (0uL)   /**< De-init value for PASS register */
#define CY_SYSANALOG_DEFAULT_BIAS_SCALE         (1uL << PASS_AREF_AREF_CTRL_AREF_BIAS_SCALE_Pos)    /**< Default AREF bias current scale of 250 nA */

/**< Macros for conditions used in CY_ASSERT calls */
#define CY_SYSANALOG_STARTUP(startup)           (((startup) == CY_SYSANALOG_STARTUP_NORMAL) || ((startup) == CY_SYSANALOG_STARTUP_FAST))
#define CY_SYSANALOG_DEEPSLEEP(deepSleep)       (((deepSleep) == CY_SYSANALOG_DEEPSLEEP_DISABLE) \
                                                || ((deepSleep) == CY_SYSANALOG_DEEPSLEEP_IPTAT_1) \
                                                || ((deepSleep) == CY_SYSANALOG_DEEPSLEEP_IPTAT_2) \
                                                || ((deepSleep) == CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF))
#define CY_SYSANALOG_VREF(vref)                 (((vref) == CY_SYSANALOG_VREF_SOURCE_SRSS) \
                                                || ((vref) == CY_SYSANALOG_VREF_SOURCE_LOCAL_1_2V) \
                                                || ((vref) == CY_SYSANALOG_VREF_SOURCE_EXTERNAL))
#define CY_SYSANALOG_IZTAT(iztat)               (((iztat) == CY_SYSANALOG_IZTAT_SOURCE_SRSS) || ((iztat) == CY_SYSANALOG_IZTAT_SOURCE_LOCAL))

/** \endcond */

/** \} group_sysanalog_macros */

/** \addtogroup group_sysanalog_enums
* \{
*/

/******************************************************************************
 * Enumerations
 *****************************************************************************/

/** The AREF status/error code definitions */
typedef enum
{
    CY_SYSANALOG_SUCCESS    = 0x00uL,                                           /**< Successful */
    CY_SYSANALOG_BAD_PARAM  = CY_SYSANALOG_ID | CY_PDL_STATUS_ERROR | 0x01uL    /**< Invalid input parameters */
}cy_en_sysanalog_status_t;

/** Aref startup mode from power on reset and from Deep Sleep wakeup
*
* To achieve the fast startup time (10 us) from Deep Sleep wakeup, the IPTAT generators must be
* enabled in Deep Sleep mode (see \ref cy_en_sysanalog_deep_sleep_t).
*
* The fast startup is the recommended mode.
*/
typedef enum
{
    CY_SYSANALOG_STARTUP_NORMAL     = 0uL,                                           /**< Normal startup */
    CY_SYSANALOG_STARTUP_FAST       = 1uL << PASS_AREF_AREF_CTRL_AREF_MODE_Pos       /**< Fast startup (10 us) - recommended */
}cy_en_sysanalog_startup_t;

/** AREF voltage reference sources
*
* The voltage reference can come from three sources:
*   - the locally generated 1.2 V reference
*   - the SRSS which provides a 0.8 V reference (not available in Deep Sleep mode)
*   - an external device pin
*/
typedef enum
{
    CY_SYSANALOG_VREF_SOURCE_SRSS        = 0uL,                                         /**< Use 0.8 V Vref from SRSS. Low accuracy high noise source that is not intended for analog subsystems. */
    CY_SYSANALOG_VREF_SOURCE_LOCAL_1_2V  = 1uL << PASS_AREF_AREF_CTRL_VREF_SEL_Pos,     /**< Use locally generated 1.2 V Vref */
    CY_SYSANALOG_VREF_SOURCE_EXTERNAL    = 2uL << PASS_AREF_AREF_CTRL_VREF_SEL_Pos      /**< Use externally supplied Vref */
}cy_en_sysanalog_vref_source_t;


/** AREF IZTAT sources
*
* The AREF generates a 1 uA "Zero dependency To Absolute Temperature" (IZTAT) current reference
* that is independent of temperature variations. It can come from one of two sources:
*   - Local reference (1 uA)
*   - Reference from the SRSS (250 nA that is gained by 4. Not available in Deep Sleep mode)
*/
typedef enum
{
    CY_SYSANALOG_IZTAT_SOURCE_SRSS       = 0uL,                                         /**< Use 250 nA IZTAT from SRSS and gain by 4 to output 1 uA*/
    CY_SYSANALOG_IZTAT_SOURCE_LOCAL      = 1uL << PASS_AREF_AREF_CTRL_IZTAT_SEL_Pos     /**< Use locally generated 1 uA IZTAT */
}cy_en_sysanalog_iztat_source_t;

/** AREF Deep Sleep mode
*
* Configure what part of the AREF block is enabled in Deep Sleep mode.
*   - Disable AREF IP block
*   - Enable IPTAT generator for fast wakeup from Deep Sleep mode.
*     IPTAT outputs for CTBs are disabled.
*   - Enable IPTAT generator and IPTAT outputs for CTB
*   - Enable all generators and outputs: IPTAT, IZTAT, and VREF
*/
typedef enum
{
    CY_SYSANALOG_DEEPSLEEP_DISABLE             = 0uL,                                               /**< Disable AREF IP block */
    CY_SYSANALOG_DEEPSLEEP_IPTAT_1             = PASS_AREF_AREF_CTRL_DEEPSLEEP_ON_Msk | \
                                                 (1uL << PASS_AREF_AREF_CTRL_DEEPSLEEP_MODE_Pos),  /**< Enable IPTAT generator for fast wakeup from Deep Sleep mode
                                                                                                        IPTAT outputs for CTBs are disabled. */
    CY_SYSANALOG_DEEPSLEEP_IPTAT_2             = PASS_AREF_AREF_CTRL_DEEPSLEEP_ON_Msk | \
                                                 (2uL << PASS_AREF_AREF_CTRL_DEEPSLEEP_MODE_Pos),  /**< Enable IPTAT generator and IPTAT outputs for CTB */
    CY_SYSANALOG_DEEPSLEEP_IPTAT_IZTAT_VREF    = PASS_AREF_AREF_CTRL_DEEPSLEEP_ON_Msk | \
                                                 (3uL << PASS_AREF_AREF_CTRL_DEEPSLEEP_MODE_Pos)   /**< Enable all generators and outputs: IPTAT, IZTAT, and VREF */
}cy_en_sysanalog_deep_sleep_t;

/** Interrupt cause sources
*
* There are two interrupts in the PASS:
*   -# one global interrupt for all CTBs
*   -# one global interrupt for all CTDACs
*
* A device could potentially have more than one instance of each IP block,
* CTB or CTDAC. To find out which instance
* caused the interrupt, call \ref Cy_SysAnalog_GetIntrCause and compare the returned
* result with one of these enum values.
*/
typedef enum
{
    CY_SYSANALOG_INTR_CAUSE_CTB0         = PASS_INTR_CAUSE_CTB0_INT_Msk,         /**< Interrupt cause mask for CTB0 */
    CY_SYSANALOG_INTR_CAUSE_CTB1         = PASS_INTR_CAUSE_CTB1_INT_Msk,         /**< Interrupt cause mask for CTB1 */
    CY_SYSANALOG_INTR_CAUSE_CTB2         = PASS_INTR_CAUSE_CTB2_INT_Msk,         /**< Interrupt cause mask for CTB2 */
    CY_SYSANALOG_INTR_CAUSE_CTB3         = PASS_INTR_CAUSE_CTB3_INT_Msk,         /**< Interrupt cause mask for CTB3 */
    CY_SYSANALOG_INTR_CAUSE_CTDAC0       = PASS_INTR_CAUSE_CTDAC0_INT_Msk,       /**< Interrupt cause mask for CTDAC0 */
    CY_SYSANALOG_INTR_CAUSE_CTDAC1       = PASS_INTR_CAUSE_CTDAC1_INT_Msk,       /**< Interrupt cause mask for CTDAC1 */
    CY_SYSANALOG_INTR_CAUSE_CTDAC2       = PASS_INTR_CAUSE_CTDAC2_INT_Msk,       /**< Interrupt cause mask for CTDAC2 */
    CY_SYSANALOG_INTR_CAUSE_CTDAC3       = PASS_INTR_CAUSE_CTDAC3_INT_Msk        /**< Interrupt cause mask for CTDAC3 */
}cy_en_sysanalog_intr_cause_t;

/** \} group_sysanalog_enums */

/** \addtogroup group_sysanalog_data_structures
* \{
*/

/***************************************
*       Configuration Structures
***************************************/

/** Structure to configure the entire AREF block */
typedef struct
{
    cy_en_sysanalog_startup_t               startup;   /**< AREF normal or fast start */
    cy_en_sysanalog_iztat_source_t          iztat;     /**< AREF 1uA IZTAT source: Local or SRSS */
    cy_en_sysanalog_vref_source_t           vref;      /**< AREF Vref: Local, SRSS, or external pin */
    cy_en_sysanalog_deep_sleep_t            deepSleep; /**< AREF Deep Sleep mode */
}cy_stc_sysanalog_config_t;

/** \} group_sysanalog_data_structures */

/** \addtogroup group_sysanalog_globals
* \{
*/
/***************************************
*        Global Constants
***************************************/

/** Configure the AREF to use the local Vref and local IZTAT. Can be used with \ref Cy_SysAnalog_Init.
* Other configuration options are set to:
*   - .startup          = CY_PASS_AREF_MODE_FAST
*   - .deepSleep        = CY_PASS_AREF_DEEPSLEEP_DISABLE
*/
extern const cy_stc_sysanalog_config_t Cy_SysAnalog_Fast_Local;

/** Configure the AREF to use the SRSS Vref and SRSS IZTAT. Can be used with \ref Cy_SysAnalog_Init.
* Other configuration options are set to:
*   - .startup          = CY_PASS_AREF_MODE_FAST
*   - .deepSleep        = CY_PASS_AREF_DEEPSLEEP_DISABLE
*/
extern const cy_stc_sysanalog_config_t Cy_SysAnalog_Fast_SRSS;

/** Configure the AREF to use the external Vref and local IZTAT. Can be used with \ref Cy_SysAnalog_Init.
* Other configuration options are set to:
*   - .startup          = CY_PASS_AREF_MODE_FAST
*   - .deepSleep        = CY_PASS_AREF_DEEPSLEEP_DISABLE
*/
extern const cy_stc_sysanalog_config_t Cy_SysAnalog_Fast_External;

/** \} group_sysanalog_globals */

/** \addtogroup group_sysanalog_functions
* \{
*/

/***************************************
*        Function Prototypes
***************************************/

cy_en_sysanalog_status_t Cy_SysAnalog_Init(const cy_stc_sysanalog_config_t *config);
__STATIC_INLINE void Cy_SysAnalog_DeInit(void);
__STATIC_INLINE uint32_t Cy_SysAnalog_GetIntrCause(void);
__STATIC_INLINE void Cy_SysAnalog_SetDeepSleepMode(cy_en_sysanalog_deep_sleep_t deepSleep);
__STATIC_INLINE cy_en_sysanalog_deep_sleep_t Cy_SysAnalog_GetDeepSleepMode(void);
__STATIC_INLINE void Cy_SysAnalog_Enable(void);
__STATIC_INLINE void Cy_SysAnalog_Disable(void);
__STATIC_INLINE void Cy_SysAnalog_SetArefMode(cy_en_sysanalog_startup_t startup);
__STATIC_INLINE void Cy_SysAnalog_VrefSelect(cy_en_sysanalog_vref_source_t vref);
__STATIC_INLINE void Cy_SysAnalog_IztatSelect(cy_en_sysanalog_iztat_source_t iztat);

/*******************************************************************************
* Function Name: Cy_SysAnalog_DeInit
****************************************************************************//**
*
* Reset AREF configuration back to power on reset defaults.
*
* \return None
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_DEINIT
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysAnalog_DeInit(void)
{
    PASS_AREF_AREF_CTRL = CY_SYSANALOG_DEINIT;
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_GetIntrCause
****************************************************************************//**
*
* Return the PASS interrupt cause register value.
*
* There are two interrupts in the PASS:
*   -# A global interrupt for all CTBs (up to 4)
*   -# A global interrupt for all CTDACs (up to 4)
*
* Compare this returned value with the enum values in \ref cy_en_sysanalog_intr_cause_t
* to determine which block caused/triggered the interrupt.
*
* \return uint32_t
* Interrupt cause register value.
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_GET_INTR_CAUSE
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SysAnalog_GetIntrCause(void)
{
    return PASS_INTR_CAUSE;
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_SetDeepSleepMode
****************************************************************************//**
*
* Set what parts of the AREF are enabled in Deep Sleep mode.
*   - Disable AREF IP block
*   - Enable IPTAT generator for fast wakeup from Deep Sleep mode.
*     IPTAT outputs for CTBs are disabled.
*   - Enable IPTAT generator and IPTAT outputs for CTB
*   - Enable all generators and outputs: IPTAT, IZTAT, and VREF
*
* \note
* The SRSS references are not available to the AREF in Deep Sleep mode. When operating
* in Deep Sleep mode, the local or external references must be selected.
*
* \param deepSleep
* Select a value from \ref cy_en_sysanalog_deep_sleep_t
*
* \return None
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_SET_DEEPSLEEP_MODE
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysAnalog_SetDeepSleepMode(cy_en_sysanalog_deep_sleep_t deepSleep)
{
    CY_ASSERT_L3(CY_SYSANALOG_DEEPSLEEP(deepSleep));

    PASS_AREF_AREF_CTRL = (PASS_AREF_AREF_CTRL & ~(PASS_AREF_AREF_CTRL_DEEPSLEEP_ON_Msk | PASS_AREF_AREF_CTRL_DEEPSLEEP_MODE_Msk)) | \
                      (uint32_t) deepSleep;
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_GetDeepSleepMode
****************************************************************************//**
*
* Return Deep Sleep mode configuration as set by \ref Cy_SysAnalog_SetDeepSleepMode
*
* \return
* A value from \ref cy_en_sysanalog_deep_sleep_t
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_GET_DEEPSLEEP_MODE
*
*******************************************************************************/
__STATIC_INLINE cy_en_sysanalog_deep_sleep_t Cy_SysAnalog_GetDeepSleepMode(void)
{
    return (cy_en_sysanalog_deep_sleep_t) (uint32_t) (PASS_AREF_AREF_CTRL & (PASS_AREF_AREF_CTRL_DEEPSLEEP_ON_Msk | PASS_AREF_AREF_CTRL_DEEPSLEEP_MODE_Msk));
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_Enable
****************************************************************************//**
*
* Enable the AREF hardware block.
*
* \return None
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_ENABLE
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysAnalog_Enable(void)
{
    PASS_AREF_AREF_CTRL |= PASS_AREF_AREF_CTRL_ENABLED_Msk;
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_Disable
****************************************************************************//**
*
* Disable the AREF hardware block.
*
* \return None
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_DISABLE
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysAnalog_Disable(void)
{
    PASS_AREF_AREF_CTRL &= ~PASS_AREF_AREF_CTRL_ENABLED_Msk;
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_SetArefMode
****************************************************************************//**
*
* Set the AREF startup mode from power on reset or from Deep Sleep wakeup.
* The AREF can startup in a normal or fast mode.
*
* If fast startup is desired from Deep Sleep wakeup, the IPTAT generators must be enabled during
* Deep Sleep. This is a minimum Deep Sleep mode setting of \ref CY_SYSANALOG_DEEPSLEEP_IPTAT_1
* (see also \ref Cy_SysAnalog_SetDeepSleepMode).
*
* \param startup
* Value from enum \ref cy_en_sysanalog_startup_t
*
* \return None
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_SET_AREF_MODE
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysAnalog_SetArefMode(cy_en_sysanalog_startup_t startup)
{
    CY_ASSERT_L3(CY_SYSANALOG_STARTUP(startup));

    PASS_AREF_AREF_CTRL = (PASS_AREF_AREF_CTRL & ~PASS_AREF_AREF_CTRL_AREF_MODE_Msk) | (uint32_t) startup;
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_VrefSelect
****************************************************************************//**
*
* Set the source for the Vref. The Vref can come from:
*   - the locally generated 1.2 V reference
*   - the SRSS, which provides a 0.8 V reference (not available to the AREF in Deep Sleep mode)
*   - an external device pin
*
* The locally generated reference has higher accuracy, more stability over temperature,
* and lower noise than the SRSS reference.
*
* \param vref
* Value from enum \ref cy_en_sysanalog_vref_source_t
*
* \return None
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_VREF_SELECT
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysAnalog_VrefSelect(cy_en_sysanalog_vref_source_t vref)
{
    CY_ASSERT_L3(CY_SYSANALOG_VREF(vref));

    PASS_AREF_AREF_CTRL = (PASS_AREF_AREF_CTRL & ~PASS_AREF_AREF_CTRL_VREF_SEL_Msk) | (uint32_t) vref;
}

/*******************************************************************************
* Function Name: Cy_SysAnalog_IztatSelect
****************************************************************************//**
*
* Set the source for the 1 uA IZTAT. The IZTAT can come from:
*   - the locally generated IZTAT
*   - the SRSS (not available to the AREF in Deep Sleep mode)
*
* The locally generated reference has higher accuracy, more stability over temperature,
* and lower noise than the SRSS reference.
*
* \param iztat
* Value from enum \ref cy_en_sysanalog_iztat_source_t
*
* \return None
*
* \funcusage
*
* \snippet sysanalog_sut_01.cydsn/main_cm0p.c SYSANA_SNIPPET_IZTAT_SELECT
*
*******************************************************************************/
__STATIC_INLINE void Cy_SysAnalog_IztatSelect(cy_en_sysanalog_iztat_source_t iztat)
{
    CY_ASSERT_L3(CY_SYSANALOG_IZTAT(iztat));

    PASS_AREF_AREF_CTRL = (PASS_AREF_AREF_CTRL & ~PASS_AREF_AREF_CTRL_IZTAT_SEL_Msk) | (uint32_t) iztat;
}

/** \} group_sysanalog_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXS40PASS */

#endif /** !defined(CY_SYSANALOG_H) */

/** \} group_sysanalog */

/* [] END OF FILE */

