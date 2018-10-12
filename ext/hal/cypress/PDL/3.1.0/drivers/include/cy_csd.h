/***************************************************************************//**
* \file cy_csd.h
* \version 0.7
*
* \brief
* The header file of the CSD driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_csd CapSense Sigma Delta (CSD)
*/

/**
********************************************************************************
* \addtogroup group_csd
********************************************************************************
* \{
*
* The CSD driver is a low-level peripheral driver that provides an interface to
* a complex mixed signal CSD hardware block, which enables multiple sensing
* capabilities on PSoC including self-cap and mutual-cap capacitive touch
* sensing solution, and a 10-bit ADC.
*
* Implementation of a capacitive touch interface and ADC requires complex
* firmware algorithms in addition to the CSD hardware block. Therefore, it is
* recommended to use CapSense or CSDADC middleware for those functions.
* The CSD driver alone does not provide system level functions, instead it is
* used by upper level middleware to configure the CSD block required by
* an application.
*
* The CSD block can support only one function at a time. However, both CapSense
* and CSDADC functionality can be time-multiplexed in a design. To allow
* seamless time-multiplex implementation of functionality and to avoid
* conflicting access to hardware from upper level, the CSD driver also
* implements lock semaphore mechanism.
*
* The CSD driver supports re-entrance. If a device contains several
* CSD hardware blocks the same CSD driver is used to configure any HW block. For
* that, each function of the CSD driver contains base address to define which
* CSD hardware block the CSD driver communicates to.
*
* For dual-core devices the CSD driver functions could be called either by CM0+
* or CM4 cores. In case of both cores need an access to the CSD Driver you
* should properly manage the memory access.
*
* There is no restriction of the CSD Driver usage in RTOS.
*
********************************************************************************
* \section group_csd_config_usage Usage
********************************************************************************
*
* The CSD driver usage by upper level.
*
* \image html csd_use_case.png "CSD Driver Use Cases" width=800px
* \image latex csd_use_case.png
*
* All functions implemented on top of CSD block could co-exists using 
* time-multiplexed scheme.
* 
* Setting up and using the CSD driver can be summed up in the four stages:
* * Defining configuration in the config structure.
* * Allocating context structure and all sub-structures.
* * Capturing the block and its configuration.
* * Executing needed action to perform any kind of conversion.
*
* The following code snippet demonstrates how to capture the CSD block and 
* perform a conversion:
* 
* \snippet csd/0.7/snippet/main.c snippet_Cy_CSD_Conversion
*
* The following code snippet demonstrates how to check the ID of an upper level
* captured the CSD block:
* 
* \snippet csd/0.7/snippet/main.c snippet_Cy_CSD_CheckKey
* 
* The following code snippet demonstrates how to check if a conversion is 
* complete:
* 
* \snippet csd/0.7/snippet/main.c snippet_Cy_CSD_CheckStatus
* 
* The following code snippet demonstrates how to change the CSD block purpose 
* using time-multiplexed scheme:
* 
* \snippet csd/0.7/snippet/main.c snippet_Cy_CSD_ReConfiguration
* 
* The entire solution either CapSense or CSDADC in addition to the CSD block
* incorporates the following instances:
*
* * \ref group_csd_config_clocks
* * \ref group_csd_config_refgen
* * \ref group_csd_config_interrupts
* * \ref group_csd_config_pin
*
* The CSD driver does not configure those blocks and they should be managed by
* an upper level. When using CapSense or CSDADC, those blocks are managed by
* middleware.
*
********************************************************************************
* \subsection group_csd_config_clocks Clocks
********************************************************************************
*
* The CSD block uses a peripheral clock (clk_peri) for the block operation.
* Set the corresponding dividers to achieve the desired clock frequency.
* For more information, refer to the \ref group_sysclk driver.
*
********************************************************************************
* \subsection group_csd_config_pin GPIO Pins
********************************************************************************
*
* A complete system, based on the CSD block, includes chip terminals (GPIO pin).
* They used as CSDADC channels, IDAC outputs, or to build widgets such as
* buttons, slider elements, touchpad elements, or proximity sensors.
*
* Any analog-capable GPIO pin can be connected to an analog multiplexed bus
* (AMUXBUS) through an analog switch and routed to the CSD block to implement
* one of a desired function.
*
* \note
* The CSD driver does not manage the pin configurations. It is an upper level
* responsibility to properly configure pins.
* It is supposed the pins are properly configured, connected/disconnected
* to the CSD block through AMUXBUS or directly depending on a pin purpose.
* When using CapSense or CSDADC, all the specified in the design pins are
* managed by middleware.
*
* Each available AMUXBUS can be split into multiple segments. Make sure
* the CSD block and a GPIO belong to the same bus segment, otherwise join
* needed segments to provide a block-pin connection.
*
* For more information about pin configuration, refer to the \ref group_gpio
* driver.
*
********************************************************************************
* \subsection group_csd_config_refgen Reference Voltage Input
********************************************************************************
*
* The CSD block requires a reference voltage to generate programmable reference
* voltage within the CSD block for its operation. There are two on-chip
* reference sources VREF and AREF for selection.
*
* For more information about specification and startup of reference voltage
* sources, refer to the \ref group_sysanalog driver prior to making the
* selection.
*
********************************************************************************
* \subsection group_csd_config_interrupts Interrupts
********************************************************************************
*
* The CSD Block has one interrupt that could be assigned to either
* Cortex-M4 core or Cortex-M0+ core. The CSD Block can generate interrupts
* on the following events:
*
* * End of sample: when scanning of a single sensor is complete.
* * End of initialization: when initialization of an analog circuit is complete.
* * End of measurement: when conversion of an CSDADC channel is complete.
*
* Additionally, the CSD interrupt can wake device up from the sleep power mode.
* The CSD block is powered down in the deep sleep or hibernate power modes, it
* cannot be used as an wake up source in these power modes.
*
* \note
* The CSD driver does not manage the CSD block interrupt. It is an upper-level
* responsibility to configure, enable and further manage the block interrupt.
* When using CapSense or CSDADC, the CSD interrupt is managed by middleware.
*
* Implement an interrupt routine and assign it to the CSD interrupt.
* Use the pre-defined enumeration as the interrupt source of the CSD block.
* The CSD interrupt to the NVIC is raised any time the intersection (logic and)
* of the interrupt flags and the corresponding interrupt masks are non-zero.
* The peripheral interrupt status register should be read in the ISR to detect
* which condition generated the interrupt. The appropriate interrupt registers
* should be cleared so that subsequent interrupts can be handled.
* 
* The following code snippet demonstrates how to implement a routine to handle
* the interrupt. The routine gets called when a CSD interrupt is triggered.
* 
* \snippet csd/0.7/snippet/main.c snippet_Cy_CSD_IntHandler
*
* The following code snippet demonstrates how to configure and enable
* the CSD interrupt:
* 
* \snippet csd/0.7/snippet/main.c snippet_Cy_CSD_IntEnabling
*
* For more information, refer to the \ref group_sysint driver.
*
* Alternately, instead of handling the interrupts, the 
* \ref Cy_CSD_GetConversionStatus() function allows for firmware 
* polling of the block status.
*
********************************************************************************
* \section group_csd_config_power_modes Power Modes
********************************************************************************
*
* The CSD block can operate in Active, LPActive, Sleep and LPSleep power modes.
* In Deep Sleep in Hibernate power modes the CSD block is powered off.
* When device wakes up from Deep Sleep the CSD block resumes operation without
* need for re-initialization. In case of wake up from Hibernate power mode,
* the CSD block does not retain configuration and block requires
* re-initialization.
*
* \note
* The CSD driver does not provide a callback function to facilitate the
* low-power mode transitions. It is responsibility of an upper level that
* uses the CSD block to ensure the CSD block is no busy prior a power mode
* transition.
*
* \warning
* Do not enter Deep Sleep power mode if the CSD block conversion is in progress.
* Unexpected behavior may occur.
*
* \note
* A power mode transition is not recommended while the CSD block is busy
* and the CSD block status must be checked using Cy_CSD_GetStatus() function
* prior power mode transition. It is recommended to use the same power mode
* for action operation of the block. These restriction is not applicable to
* Sleep mode and device can seamlessly enter and exit sleep mode while the
* CSD block is busy.
*
* \warning
* Analog start up time for the CSD block is 25us. It is recommended to initiate
* any kind of conversion only after 25us from Deep Sleep / Hibernate exit.
*
* Refer to the \ref group_syspm driver for more information about
* low-power mode transitions.
*
********************************************************************************
* \section group_csd_more_information More Information
********************************************************************************
*
* For more information, refer to the following documents:
*
* * <a href="http://www.cypress.com/trm218176"><b>Technical Reference Manual
* (TRM)</b></a>
*
* * <a href="..\..\pdl_user_guide.pdf"><b>PDL User Guide</b></a>
*
* * \ref page_getting_started "Getting Started with the PDL"
*
* * <a href="http://www.cypress.com/ds218787"><b>PSoC 63 with BLE Datasheet
* Programmable System-on-Chip datasheet</b></a>
*
* * <a href="http://www.cypress.com/an85951"><b>AN85951 PSoC 4 and PSoC 6
* MCU CapSense Design Guide for more details</b></a>
*
* * <a href="http://www.cypress.com/an210781"><b>AN210781 Getting Started
* with PSoC 6 MCU with Bluetooth Low Energy (BLE) Connectivity</b></a>
*
********************************************************************************
* \section group_csd_MISRA MISRA-C Compliance
********************************************************************************
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>14.2</td>
*     <td>R</td>
*     <td>All non-null statements shall either: a) have at least one side-effect
*         however executed, or b) cause control flow to change.</td>
*     <td>The unused function parameters are cast to void. This statement
*         has no side-effect and is used to suppress a compiler warning.</td>
*   </tr>
* </table>
*
********************************************************************************
* \section group_csd_changelog Changelog
********************************************************************************
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>0.7</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*/

/** \} group_csd */

/**
********************************************************************************
* \addtogroup group_csd
********************************************************************************
* \{
* \defgroup group_csd_macros                Macros
* \defgroup group_csd_functions             Functions
* \defgroup group_csd_data_structures       Data Structures
* \defgroup group_csd_enums                 Enumerated Types
*/


#if !defined(CY_CSD_H)
#define CY_CSD_H

#include <stdint.h>
#include <stddef.h>
#include "cy_device_headers.h"
#include "cy_syslib.h"

#ifndef CY_IP_MXCSDV2
    #error "The CSD driver is not supported on this device"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_csd_macros
* \{
*/

/** Driver major version */
#define CY_CSD_DRV_VERSION_MAJOR            (0)

/** Driver minor version */
#define CY_CSD_DRV_VERSION_MINOR            (7)


/******************************************************************************
* API Constants
******************************************************************************/

/** CSD driver identifier */
#define CY_CSD_ID                           (CY_PDL_DRV_ID(0x41U))

/** Initialization macro for the driver context variable */
#define CY_CSD_CONTEXT_INIT_VALUE           {.lockKey = CY_CSD_NONE_KEY}


/*******************************************************************************
* HW CSD Block Registers Constants
*******************************************************************************/

/**
* \defgroup group_csd_reg_const Registers Constants
* \{
*/

/** \} group_csd_reg_const */

/** \} group_csd_macros */


/*******************************************************************************
 * Enumerations
 ******************************************************************************/

/**
* \addtogroup group_csd_enums
* \{
*/

/** CSD status definitions */
typedef enum
{
    /** Successful */
    CY_CSD_SUCCESS = 0x00U,

    /** One or more invalid parameters */
    CY_CSD_BAD_PARAM = CY_CSD_ID | CY_PDL_STATUS_ERROR | 0x01U,

    /** CSD HW block performs conversion */
    CY_CSD_BUSY =  CY_CSD_ID | CY_PDL_STATUS_ERROR | 0x02U,

    /** CSD HW block is captured by another middleware */
    CY_CSD_LOCKED =  CY_CSD_ID | CY_PDL_STATUS_ERROR | 0x03U

} cy_en_csd_status_t;


/** Key definition of upper level that uses the driver */
typedef enum
{
    /** HW block is unused and not captured by any middleware */
    CY_CSD_NONE_KEY = 0U,

    /** HW block is captured by a user */
    CY_CSD_USER_DEFINED_KEY = 1U,

    /** HW block is captured by a CapSense middleware */
    CY_CSD_CAPSENSE_KEY = 2U,

    /** HW block is captured by a ADC composite driver */
    CY_CSD_ADC_KEY = 3U,

    /** HW block is captured by a IDAC composite driver */
    CY_CSD_IDAC_KEY = 4U,

    /** HW block is captured by a CMP composite driver */
    CY_CSD_CMP_KEY = 5U

}cy_en_csd_key_t;

/** \} group_csd_enums */


/*******************************************************************************
*       Type Definitions
*******************************************************************************/

/**
* \addtogroup group_csd_data_structures
* \{
*/

/** CSD configuration structure */
typedef struct
{
    uint32_t config;       /**< Stores the CSD.CONFIG register value */
    uint32_t spare;        /**< Stores the CSD.SPARE register value */
    uint32_t status;       /**< Stores the CSD.STATUS register value */
    uint32_t statSeq;      /**< Stores the CSD.STAT_SEQ register value */
    uint32_t statCnts;     /**< Stores the CSD.STAT_CNTS register value */
    uint32_t statHcnt;     /**< Stores the CSD.STAT_HCNT register value */
    uint32_t resultVal1;   /**< Stores the CSD.RESULT_VAL1 register value */
    uint32_t resultVal2;   /**< Stores the CSD.RESULT_VAL2 register value */
    uint32_t adcRes;       /**< Stores the CSD.ADC_RES register value */
    uint32_t intr;         /**< Stores the CSD.INTR register value */
    uint32_t intrSet;      /**< Stores the CSD.INTR_SET register value */
    uint32_t intrMask;     /**< Stores the CSD.INTR_MASK register value */
    uint32_t intrMasked;   /**< Stores the CSD.INTR_MASKED register value */
    uint32_t hscmp;        /**< Stores the CSD.HSCMP register value */
    uint32_t ambuf;        /**< Stores the CSD.AMBUF register value */
    uint32_t refgen;       /**< Stores the CSD.REFGEN register value */
    uint32_t csdCmp;       /**< Stores the CSD.CSDCMP register value */
    uint32_t swRes;        /**< Stores the CSD.SW_RES register value */
    uint32_t sensePeriod;  /**< Stores the CSD.SENSE_PERIOD register value */
    uint32_t senseDuty;    /**< Stores the CSD.SENSE_DUTY register value */
    uint32_t swHsPosSel;   /**< Stores the CSD.SW_HS_P_SEL register value */
    uint32_t swHsNegSel;   /**< Stores the CSD.SW_HS_N_SEL register value */
    uint32_t swShieldSel;  /**< Stores the CSD.SW_SHIELD_SEL register value */
    uint32_t swAmuxbufSel; /**< Stores the CSD.SW_AMUXBUF_SEL register value */
    uint32_t swBypSel;     /**< Stores the CSD.SW_BYP_SEL register value */
    uint32_t swCmpPosSel;  /**< Stores the CSD.SW_CMP_P_SEL register value */
    uint32_t swCmpNegSel;  /**< Stores the CSD.SW_CMP_N_SEL register value */
    uint32_t swRefgenSel;  /**< Stores the CSD.SW_REFGEN_SEL register value */
    uint32_t swFwModSel;   /**< Stores the CSD.SW_FW_MOD_SEL register value */
    uint32_t swFwTankSel;  /**< Stores the CSD.SW_FW_TANK_SEL register value */
    uint32_t swDsiSel;     /**< Stores the CSD.SW_DSI_SEL register value */
    uint32_t ioSel;        /**< Stores the CSD.IO_SEL register value */
    uint32_t seqTime;      /**< Stores the CSD.SEQ_TIME register value */
    uint32_t seqInitCnt;   /**< Stores the CSD.SEQ_INIT_CNT register value */
    uint32_t seqNormCnt;   /**< Stores the CSD.SEQ_NORM_CNT register value */
    uint32_t adcCtl;       /**< Stores the CSD.ADC_CTL register value */
    uint32_t seqStart;     /**< Stores the CSD.SEQ_START register value */
    uint32_t idacA;        /**< Stores the CSD.IDACA register value */
    uint32_t idacB;        /**< Stores the CSD.IDACB register value */
} cy_stc_csd_config_t;


/** CSD context structure */
typedef struct
{
    /** Middleware ID that currently captured CSD */
    cy_en_csd_key_t lockKey;
} cy_stc_csd_context_t;

/** \} group_csd_data_structures */

/**
* \addtogroup group_csd_reg_const
* \{
*/


/** The register offset */
#define CY_CSD_REG_OFFSET_CONFIG            (offsetof(CSD_Type, CONFIG))
/** The register offset */
#define CY_CSD_REG_OFFSET_SPARE             (offsetof(CSD_Type, SPARE))
/** The register offset */
#define CY_CSD_REG_OFFSET_STATUS            (offsetof(CSD_Type, STATUS))
/** The register offset */
#define CY_CSD_REG_OFFSET_STAT_SEQ          (offsetof(CSD_Type, STAT_SEQ))
/** The register offset */
#define CY_CSD_REG_OFFSET_STAT_CNTS         (offsetof(CSD_Type, STAT_CNTS))
/** The register offset */
#define CY_CSD_REG_OFFSET_STAT_HCNT         (offsetof(CSD_Type, STAT_HCNT))
/** The register offset */
#define CY_CSD_REG_OFFSET_RESULT_VAL1       (offsetof(CSD_Type, RESULT_VAL1))
/** The register offset */
#define CY_CSD_REG_OFFSET_RESULT_VAL2       (offsetof(CSD_Type, RESULT_VAL2))
/** The register offset */
#define CY_CSD_REG_OFFSET_ADC_RES           (offsetof(CSD_Type, ADC_RES))
/** The register offset */
#define CY_CSD_REG_OFFSET_INTR              (offsetof(CSD_Type, INTR))
/** The register offset */
#define CY_CSD_REG_OFFSET_INTR_SET          (offsetof(CSD_Type, INTR_SET))
/** The register offset */
#define CY_CSD_REG_OFFSET_INTR_MASK         (offsetof(CSD_Type, INTR_MASK))
/** The register offset */
#define CY_CSD_REG_OFFSET_INTR_MASKED       (offsetof(CSD_Type, INTR_MASKED))
/** The register offset */
#define CY_CSD_REG_OFFSET_HSCMP             (offsetof(CSD_Type, HSCMP))
/** The register offset */
#define CY_CSD_REG_OFFSET_AMBUF             (offsetof(CSD_Type, AMBUF))
/** The register offset */
#define CY_CSD_REG_OFFSET_REFGEN            (offsetof(CSD_Type, REFGEN))
/** The register offset */
#define CY_CSD_REG_OFFSET_CSDCMP            (offsetof(CSD_Type, CSDCMP))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_RES            (offsetof(CSD_Type, SW_RES))
/** The register offset */
#define CY_CSD_REG_OFFSET_SENSE_PERIOD      (offsetof(CSD_Type, SENSE_PERIOD))
/** The register offset */
#define CY_CSD_REG_OFFSET_SENSE_DUTY        (offsetof(CSD_Type, SENSE_DUTY))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_HS_P_SEL       (offsetof(CSD_Type, SW_HS_P_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_HS_N_SEL       (offsetof(CSD_Type, SW_HS_N_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_SHIELD_SEL     (offsetof(CSD_Type, SW_SHIELD_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_AMUXBUF_SEL    (offsetof(CSD_Type, SW_AMUXBUF_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_BYP_SEL        (offsetof(CSD_Type, SW_BYP_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_CMP_P_SEL      (offsetof(CSD_Type, SW_CMP_P_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_CMP_N_SEL      (offsetof(CSD_Type, SW_CMP_N_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_REFGEN_SEL     (offsetof(CSD_Type, SW_REFGEN_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_FW_MOD_SEL     (offsetof(CSD_Type, SW_FW_MOD_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_FW_TANK_SEL    (offsetof(CSD_Type, SW_FW_TANK_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SW_DSI_SEL        (offsetof(CSD_Type, SW_DSI_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_IO_SEL            (offsetof(CSD_Type, IO_SEL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SEQ_TIME          (offsetof(CSD_Type, SEQ_TIME))
/** The register offset */
#define CY_CSD_REG_OFFSET_SEQ_INIT_CNT      (offsetof(CSD_Type, SEQ_INIT_CNT))
/** The register offset */
#define CY_CSD_REG_OFFSET_SEQ_NORM_CNT      (offsetof(CSD_Type, SEQ_NORM_CNT))
/** The register offset */
#define CY_CSD_REG_OFFSET_ADC_CTL           (offsetof(CSD_Type, ADC_CTL))
/** The register offset */
#define CY_CSD_REG_OFFSET_SEQ_START         (offsetof(CSD_Type, SEQ_START))
/** The register offset */
#define CY_CSD_REG_OFFSET_IDACA             (offsetof(CSD_Type, IDACA))
/** The register offset */
#define CY_CSD_REG_OFFSET_IDACB             (offsetof(CSD_Type, IDACB))
/** \} group_csd_reg_const */

/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/**
* \addtogroup group_csd_functions
* \{
*/

__STATIC_INLINE uint32_t Cy_CSD_ReadReg(CSD_Type * base, uint32_t offset);
__STATIC_INLINE void Cy_CSD_WriteReg(CSD_Type * base, uint32_t offset, uint32_t value);
__STATIC_INLINE void Cy_CSD_SetBits(CSD_Type * base, uint32_t offset, uint32_t mask);
__STATIC_INLINE void Cy_CSD_ClrBits(CSD_Type * base, uint32_t offset, uint32_t mask);
__STATIC_INLINE void Cy_CSD_WriteBits(CSD_Type* base, uint32_t offset, uint32_t mask, uint32_t value);
__STATIC_INLINE cy_en_csd_key_t Cy_CSD_GetLockStatus(CSD_Type * base, cy_stc_csd_context_t * context);
__STATIC_INLINE cy_en_csd_status_t Cy_CSD_GetConversionStatus(CSD_Type * base, cy_stc_csd_context_t * context);

cy_en_csd_status_t Cy_CSD_Init(CSD_Type * base, cy_stc_csd_config_t const * config, cy_en_csd_key_t key, cy_stc_csd_context_t * context);
cy_en_csd_status_t Cy_CSD_DeInit(CSD_Type * base, cy_en_csd_key_t key, cy_stc_csd_context_t * context);
cy_en_csd_status_t Cy_CSD_Configure(CSD_Type * base, cy_stc_csd_config_t const * config, cy_en_csd_key_t key, cy_stc_csd_context_t * context);


/*******************************************************************************
* Function Name: Cy_CSD_ReadReg
****************************************************************************//**
*
* Reads value from the specified CSD HW block register.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param offset
* The offset of the required register's address relatively to the base address.
*
* \return 
* Returns a value of register, specified by the offset parameter.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_CSD_ReadReg(CSD_Type * base, uint32_t offset)
{
    return(* (volatile uint32_t *)((uint32_t)(base) + (offset)));
}


/*******************************************************************************
* Function Name: Cy_CSD_WriteReg
****************************************************************************//**
*
* Writes value to the specified CSD HW block register.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param offset
* The offset of the required register's address relatively to the base address.
*
* \param value
* Specifies a value to be written to the register.
*
*******************************************************************************/
__STATIC_INLINE void Cy_CSD_WriteReg(CSD_Type * base, uint32_t offset, uint32_t value)
{
    (* (volatile uint32_t *)((uint32_t)(base) + offset)) = value;
}


/*******************************************************************************
* Function Name: Cy_CSD_SetBits
****************************************************************************//**
*
* Sets bits, specified by the Mask parameter in the CSD HW block register,
* specified by the Offset parameter.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param offset
* The offset of the required register's address relatively to the base address.
*
* \param mask
* Specifies bits to be set.
*
*******************************************************************************/
__STATIC_INLINE void Cy_CSD_SetBits(CSD_Type * base, uint32_t offset, uint32_t mask)
{
    volatile uint32_t * regPtr = (volatile uint32_t *)((uint32_t)(base) + (offset));
    (* regPtr) |= mask;
}


/*******************************************************************************
* Function Name: Cy_CSD_ClrBits
****************************************************************************//**
*
* Clears bits, specified by the Mask parameter in the CSD HW block register,
* specified by the Offset parameter.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param offset
* The offset of the required register's address relatively to the base address.
*
* \param mask
* Specifies bits to be clear.
*
*******************************************************************************/
__STATIC_INLINE void Cy_CSD_ClrBits(CSD_Type * base, uint32_t offset, uint32_t mask)
{
    volatile uint32_t * regPtr = (volatile uint32_t *)((uint32_t)(base) + (offset));
    (* regPtr) &= (~(mask));
}


/*******************************************************************************
* Function Name: Cy_CSD_WriteBits
****************************************************************************//**
*
* Writes field, specified by the Mask parameter with the value, specified by
* the Value parameter.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param offset
* The offset of the required register's address relatively to the base address.
*
* \param mask
* Specifies bits to be affected.
*
* \param value
* Specifies a value to be written to the register.
*
*******************************************************************************/
__STATIC_INLINE void Cy_CSD_WriteBits(CSD_Type * base, uint32_t offset, uint32_t mask, uint32_t value)
{
    volatile uint32_t * regPtr = (volatile uint32_t *)((uint32_t)(base) + (offset));
    (* regPtr) &= (~(mask));
    (* regPtr) |= ((value) & (mask));
}


/*******************************************************************************
* Function Name: Cy_CSD_GetLockStatus
****************************************************************************//**
*
* Verifies whether specified CSD HW block is not locked by another routine.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param context
* The pointer to the context structure, allocated by user or middleware.
*
* \return 
* Returns a key code. See \ref cy_en_csd_key_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_csd_key_t Cy_CSD_GetLockStatus(CSD_Type * base, cy_stc_csd_context_t * context)
{
    (void)base;
    return(context->lockKey);
}


/*******************************************************************************
* Function Name: Cy_CSD_GetConversionStatus
****************************************************************************//**
*
* Verifies whether the specified CSD HW block is not scanning.
*
* \param base
* The pointer to a CSD HW instance.
*
* \param context
* The pointer to the context structure, allocated by user or middleware.
*
* \return 
* Returns status code. See \ref cy_en_csd_status_t.
*
*******************************************************************************/
__STATIC_INLINE cy_en_csd_status_t Cy_CSD_GetConversionStatus(CSD_Type * base, cy_stc_csd_context_t * context)
{
    cy_en_csd_status_t status = CY_CSD_BUSY;

    (void)context;
    if ((base->SEQ_START & CSD_SEQ_START_START_Msk) == 0)
    {
        status = CY_CSD_SUCCESS;
    }

    return(status);
}
/** \} group_csd_functions */

/** \} group_csd */

#if defined(__cplusplus)
}
#endif

#endif /* CY_CSD_H */


/* [] END OF FILE */
