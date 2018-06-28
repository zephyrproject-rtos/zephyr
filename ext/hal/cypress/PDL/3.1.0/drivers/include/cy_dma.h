/***************************************************************************//**
* \file cy_dma.h
* \version 2.10
*
* \brief
* The header file of the DMA driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_dma Direct Memory Access (DMA)
* \{
* Configures a DMA channel and its descriptor(s).
*
* The DMA channel can be used in any project to transfer data
* without CPU intervention basing on a hardware trigger signal from another component.
*
* A device may support more than one DMA hardware block. Each block has a set of 
* registers, a base hardware address, and supports multiple channels. 
* Many API functions for the DMA driver require a base hardware address and  
* channel number. Ensure that you use the correct hardware address for the DMA block in use.
* 
* Features:
* * Devices support up to two DMA hardware blocks
* * Each DMA block supports up to 16 DMA channels
* * Supports channel descriptors in SRAM
* * Four priority levels for each channel
* * Byte, half-word (2-byte), and word (4-byte) transfers
* * Configurable source and destination addresses
*
* \section group_dma_configuration Configuration Considerations
*
* To set up a DMA driver, initialize a descriptor,
* intialize and enable a channel, and enable the DMA block.
* 
* To set up a descriptor, provide the configuration parameters for the 
* descriptor in the \ref cy_stc_dma_descriptor_config_t structure. Then call the 
* \ref Cy_DMA_Descriptor_Init function to initialize the descriptor in SRAM. You can 
* modify the source and destination addresses dynamically by calling 
* \ref Cy_DMA_Descriptor_SetSrcAddress and \ref Cy_DMA_Descriptor_SetDstAddress.
* 
* To set up a DMA channel, provide a filled \ref cy_stc_dma_channel_config_t 
* structure. Call the \ref Cy_DMA_Channel_Init function, specifying the channel
* number. Use \ref Cy_DMA_Channel_Enable to enable the configured DMA channel.
*
* Call \ref Cy_DMA_Channel_Enable for each DMA channel in use.
*
* When configured, another peripheral typically triggers the DMA. The trigger is
* connected to the DMA using the trigger multiplexer. The trigger multiplexer
* driver has a software trigger you can use in firmware to trigger the DMA. See the
* <a href="group__group__trigmux.html">Trigger Multiplexer</a> documentation.
*
* The following is a simplified structure of the DMA driver API interdependencies
* in a typical user application:
* \image html dma.png
*
* <B>NOTE:</B> Even if a DMA channel is enabled, it is not operational until 
* the DMA block is enabled using function \ref Cy_DMA_Enable.\n
* <B>NOTE:</B> if the DMA descriptor is configured to generate an interrupt, 
* the interrupt must be enabled using the \ref Cy_DMA_Channel_SetInterruptMask 
* function for each DMA channel.
*
* For example:
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Enable
* 
* \section group_dma_more_information More Information.
* See: the DMA chapter of the device technical reference manual (TRM);
*      the DMA Component datasheet;
*      CE219940 - PSoC 6 MCU Multiple DMA Concatenation.
*
* \section group_dma_MISRA MISRA-C Compliance
* The DMA driver has the following specific deviations:
*
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>10.3</td>
*     <td>R</td>
*     <td>A composite expression of the "essentially unsigned" type is being
*         cast to a different type category.</td>
*     <td>The value got from the bitfield physically cannot exceed the enumeration
*         that describes this bitfield. So, the code is safe by design.</td>
*   </tr>
*   <tr>
*     <td>14.2</td>
*     <td>R</td>
*     <td>All non-null statements will either:
*       a) have at least one-side effect however executed;
*       b) cause the control flow to change</td>
*     <td>A Read operation result is ignored when this read is just intended
*         to ensure the previous Write operation is flushed out to the hardware.</td>
*   </tr>
* </table>
*
* \section group_dma_changelog Changelog
*
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>2.10</td>
*     <td>   * New features are added
*            * Flattened the driver source code organization into the single
*              source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure. <br>
*         New silicon support.
*     </td>
*   </tr>
*   <tr>
*     <td>2.0.1</td>
*     <td>Changed CY_DMA_BWC macro values from Boolean to numeric</td>
*     <td>Improvements made based on usability feedback</td>
*   </tr>
*   <tr>
*     <td>2.0</td>
*     <td> * All the API is refactored to be consistent within itself and with the
*            rest of the PDL content.
*          * The descriptor API is updated as follows:
*            The \ref Cy_DMA_Descriptor_Init function sets a full bunch of descriptor
*            settings, and the rest of the descriptor API is a get/set interface
*            to each of the descriptor settings.
*          * There is a group of macros to support the backward compatibility with most 
*            of the driver version 1.0 API. But, it is strongly recommended to use
*            the new v2.0 interface in new designs (do not just copy-paste from old
*            projects). To enable the backward compatibility support, the CY_DMA_BWC 
*            definition should be changed to "1".</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*

* \defgroup group_dma_macros Macros
* \defgroup group_dma_functions Functions
* \{
* \defgroup group_dma_block_functions Block Functions
* \defgroup group_dma_channel_functions Channel Functions
* \defgroup group_dma_descriptor_functions Descriptor Functions
* \}
* \defgroup group_dma_data_structures Data Structures
* \defgroup group_dma_enums Enumerated Types
*/

#if !defined(CY_DMA_H)
#define CY_DMA_H

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef CY_IP_M4CPUSS_DMA

#if defined(__cplusplus)
extern "C" {
#endif

/******************************************************************************
 * Macro definitions                                                          *
 ******************************************************************************/

/**
* \addtogroup group_dma_macros
* \{
*/

/** The driver major version */
#define CY_DMA_DRV_VERSION_MAJOR       2

/** The driver minor version */
#define CY_DMA_DRV_VERSION_MINOR       10

/** The DMA driver identifier */
#define CY_DMA_ID                      (CY_PDL_DRV_ID(0x13U))
 
/** The DMA channel interrupt mask */
#define CY_DMA_INTR_MASK               (0x01UL)

/** The minimum X/Y Count API parameters */
#define CY_DMA_LOOP_COUNT_MIN          (1UL)
/** The maximum X/Y Count API parameters */
#define CY_DMA_LOOP_COUNT_MAX          (256UL)

/** The minimum X/Y Increment API parameters */
#define CY_DMA_LOOP_INCREMENT_MIN      (-2048L)
/** The maximum X/Y Increment API parameters */
#define CY_DMA_LOOP_INCREMENT_MAX      (2047L)

/** The backward compatibility flag. Enables a group of macros which provide 
* the backward compatibility with most of the DMA driver version 1.0 interface. */
#ifndef CY_DMA_BWC
    #define CY_DMA_BWC                 (0U) /* Disabled by default */
#endif

/** \} group_dma_macros */


/**
* \addtogroup group_dma_enums
* \{
*/

/** Contains the possible interrupt cause values */
typedef enum
{
    CY_DMA_INTR_CAUSE_NO_INTR            = 0U, /**< No interrupt.                       */
    CY_DMA_INTR_CAUSE_COMPLETION         = 1U, /**< Completion.                         */
    CY_DMA_INTR_CAUSE_SRC_BUS_ERROR      = 2U, /**< Source bus error.                   */
    CY_DMA_INTR_CAUSE_DST_BUS_ERROR      = 3U, /**< Destination bus error.              */
    CY_DMA_INTR_CAUSE_SRC_MISAL          = 4U, /**< Source address is not aligned       */
    CY_DMA_INTR_CAUSE_DST_MISAL          = 5U, /**< Destination address is not aligned. */
    CY_DMA_INTR_CAUSE_CURR_PTR_NULL      = 6U, /**< Current descripror pointer is NULL. */
    CY_DMA_INTR_CAUSE_ACTIVE_CH_DISABLED = 7U, /**< Active channel is disabled.         */
    CY_DMA_INTR_CAUSE_DESCR_BUS_ERROR    = 8U  /**< Descriptor bus error.               */
} cy_en_dma_intr_cause_t;

/** Contains the options for the descriptor type */
typedef enum
{
    CY_DMA_SINGLE_TRANSFER = 0UL,          /**< Single transfer. */
    CY_DMA_1D_TRANSFER     = 1UL,          /**< 1D transfer.     */
    CY_DMA_2D_TRANSFER     = 2UL,          /**< 2D transfer.     */
    CY_DMA_CRC_TRANSFER    = 3UL,          /**< CRC transfer. Supported by the DW_v2 only (TODO: correct the user-facing info) */
} cy_en_dma_descriptor_type_t;

/** Contains the options for the interrupt, trig-in and trig-out type parameters of the descriptor */
typedef enum
{
    CY_DMA_1ELEMENT    = 0UL,              /**< One element transfer.             */
    CY_DMA_X_LOOP      = 1UL,              /**< One X loop transfer.              */
    CY_DMA_DESCR       = 2UL,              /**< One descriptor transfer.          */
    CY_DMA_DESCR_CHAIN = 3UL               /**< Entire descriptor chain transfer. */
} cy_en_dma_trigger_type_t;

/** This enum has the options for the data size */
typedef enum
{
    CY_DMA_BYTE     = 0UL,                 /**< One byte.               */
    CY_DMA_HALFWORD = 1UL,                 /**< Half word (two bytes).  */
    CY_DMA_WORD     = 2UL                  /**< Full word (four bytes). */
} cy_en_dma_data_size_t;

/** This enum has the options for descriptor retriggering */
typedef enum
{
    CY_DMA_RETRIG_IM      = 0UL,           /**< Retrigger immediatelly.             */
    CY_DMA_RETRIG_4CYC    = 1UL,           /**< Retrigger after 4 Clk_Slow cycles.  */
    CY_DMA_RETRIG_16CYC   = 2UL,           /**< Retrigger after 16 Clk_Slow cycles. */
    CY_DMA_WAIT_FOR_REACT = 3UL            /**< Wait for trigger reactivation.      */
} cy_en_dma_retrigger_t;

/** This enum has the options for the transfer size */
typedef enum
{
    CY_DMA_TRANSFER_SIZE_DATA = 0UL,       /**< As specified by dataSize. */
    CY_DMA_TRANSFER_SIZE_WORD = 1UL,       /**< A full word (four bytes). */
} cy_en_dma_transfer_size_t;

/** This enum has the options for the state of the channel when the descriptor is completed   */
typedef enum
{
    CY_DMA_CHANNEL_ENABLED  = 0UL,         /**< Channel stays enabled. */
    CY_DMA_CHANNEL_DISABLED = 1UL          /**< Channel is disabled.   */
} cy_en_dma_channel_state_t;

/** This enum has the return values of the DMA driver */
typedef enum 
{
    CY_DMA_SUCCESS   = 0x00UL, /**< Success. */
    CY_DMA_BAD_PARAM = CY_DMA_ID | CY_PDL_STATUS_ERROR | 0x01UL /**< The input parameters passed to the DMA API are not valid. */
} cy_en_dma_status_t;

/** \} group_dma_enums */


/** \cond Internal */

/* Macros for the conditions used by CY_ASSERT calls */
#define CY_DMA_IS_LOOP_COUNT_VALID(count)      (((count) >= CY_DMA_LOOP_COUNT_MIN) && ((count) <= CY_DMA_LOOP_COUNT_MAX))
#define CY_DMA_IS_LOOP_INCR_VALID(incr)        (((incr) >= CY_DMA_LOOP_INCREMENT_MIN) && ((incr) <= CY_DMA_LOOP_INCREMENT_MAX))
#define CY_DMA_IS_PRIORITY_VALID(prio)         ((prio) <= 3UL)
#define CY_DMA_IS_INTR_MASK_VALID(intr)        (0UL == ((intr) & ((uint32_t) ~CY_DMA_INTR_MASK)))

#define CY_DMA_IS_RETRIG_VALID(retrig)         ((CY_DMA_RETRIG_IM      == (retrig)) || \
                                                (CY_DMA_RETRIG_4CYC    == (retrig)) || \
                                                (CY_DMA_RETRIG_16CYC   == (retrig)) || \
                                                (CY_DMA_WAIT_FOR_REACT == (retrig)))

#define CY_DMA_IS_TRIG_TYPE_VALID(trigType)    ((CY_DMA_1ELEMENT    == (trigType)) || \
                                                (CY_DMA_X_LOOP      == (trigType)) || \
                                                (CY_DMA_DESCR       == (trigType)) || \
                                                (CY_DMA_DESCR_CHAIN == (trigType)))

#define CY_DMA_IS_XFER_SIZE_VALID(xferSize)    ((CY_DMA_TRANSFER_SIZE_DATA == (xferSize)) || \
                                                (CY_DMA_TRANSFER_SIZE_WORD == (xferSize)))

#define CY_DMA_IS_CHANNEL_STATE_VALID(state)   ((CY_DMA_CHANNEL_ENABLED  == (state)) || \
                                                (CY_DMA_CHANNEL_DISABLED == (state)))

#define CY_DMA_IS_DATA_SIZE_VALID(dataSize)    ((CY_DMA_BYTE     == (dataSize)) || \
                                                (CY_DMA_HALFWORD == (dataSize)) || \
                                                (CY_DMA_WORD     == (dataSize)))

#define CY_DMA_IS_TYPE_VALID(descrType)        ((CY_DMA_SINGLE_TRANSFER == (descrType)) || \
                                                (CY_DMA_1D_TRANSFER     == (descrType)) || \
                                                (CY_DMA_2D_TRANSFER     == (descrType)) || \
                                                (CY_DMA_CRC_TRANSFER    == (descrType)))

#define CY_DMA_IS_CH_NR_VALID(chNr)            ((chNr) < CY_DW_CH_NR)


/* The descriptor structure bit field definitions */
#define CY_DMA_CTL_RETRIG_Pos      (0UL)
#define CY_DMA_CTL_RETRIG_Msk      ((uint32_t)0x3UL << CY_DMA_CTL_RETRIG_Pos)
#define CY_DMA_CTL_INTR_TYPE_Pos   (2UL)
#define CY_DMA_CTL_INTR_TYPE_Msk   ((uint32_t)0x3UL << CY_DMA_CTL_INTR_TYPE_Pos)
#define CY_DMA_CTL_TR_OUT_TYPE_Pos (4UL)
#define CY_DMA_CTL_TR_OUT_TYPE_Msk ((uint32_t)0x3UL << CY_DMA_CTL_TR_OUT_TYPE_Pos)
#define CY_DMA_CTL_TR_IN_TYPE_Pos  (6UL)
#define CY_DMA_CTL_TR_IN_TYPE_Msk  ((uint32_t)0x3UL << CY_DMA_CTL_TR_IN_TYPE_Pos)
#define CY_DMA_CTL_CH_DISABLE_Pos  (24UL)
#define CY_DMA_CTL_CH_DISABLE_Msk  ((uint32_t)0x1UL << CY_DMA_CTL_CH_DISABLE_Pos)
#define CY_DMA_CTL_SRC_SIZE_Pos    (26UL)
#define CY_DMA_CTL_SRC_SIZE_Msk    ((uint32_t)0x1UL << CY_DMA_CTL_SRC_SIZE_Pos)
#define CY_DMA_CTL_DST_SIZE_Pos    (27UL)
#define CY_DMA_CTL_DST_SIZE_Msk    ((uint32_t)0x1UL << CY_DMA_CTL_DST_SIZE_Pos)
#define CY_DMA_CTL_DATA_SIZE_Pos   (28UL)
#define CY_DMA_CTL_DATA_SIZE_Msk   ((uint32_t)0x3UL << CY_DMA_CTL_DATA_SIZE_Pos)
#define CY_DMA_CTL_TYPE_Pos        (30UL)
#define CY_DMA_CTL_TYPE_Msk        ((uint32_t)0x3UL << CY_DMA_CTL_TYPE_Pos)

#define CY_DMA_CTL_SRC_INCR_Pos    (0UL)
#define CY_DMA_CTL_SRC_INCR_Msk    ((uint32_t)0xFFFUL << CY_DMA_CTL_SRC_INCR_Pos)
#define CY_DMA_CTL_DST_INCR_Pos    (12UL)
#define CY_DMA_CTL_DST_INCR_Msk    ((uint32_t)0xFFFUL << CY_DMA_CTL_DST_INCR_Pos)
#define CY_DMA_CTL_COUNT_Pos       (24UL)
#define CY_DMA_CTL_COUNT_Msk       ((uint32_t)0xFFUL << CY_DMA_CTL_COUNT_Pos)

/** \endcond */


/**
* \addtogroup group_dma_data_structures
* \{
*/

/** 
* DMA descriptor structure type. It is a user/component-declared structure 
* allocated in RAM. The DMA HW requires a pointer to this structure to work with it.
*
* For advanced users: the descriptor can be allocated even in Flash, then the user
* manually predefines all the structure items with constants, 
* bacause most of the driver's API (especially functions modifying
* descriptors, including \ref Cy_DMA_Descriptor_Init()) can't work with 
* read-only descriptors.
*/
typedef struct
{
    uint32_t ctl;                    /*!< 0x00000000 Descriptor control */
    uint32_t src;                    /*!< 0x00000004 Descriptor source */
    uint32_t dst;                    /*!< 0x00000008 Descriptor destination */
    uint32_t xCtl;                   /*!< 0x0000000C Descriptor X loop control */
    uint32_t yCtl;                   /*!< 0x00000010 Descriptor Y loop control */
    uint32_t nextPtr;                /*!< 0x00000014 Descriptor next pointer */
} cy_stc_dma_descriptor_t;

/**
* This structure is a configuration structure pre-initialized by the user and
* passed as a parameter to the \ref Cy_DMA_Descriptor_Init().
* It can be allocated in RAM/Flash (on user's choice).
* In case of Flash allocation there is a possibility to reinitialize the descriptor in runtime.
* This structure has all the parameters of the descriptor as separate parameters.
* Most of these parameters are represented in the \ref cy_stc_dma_descriptor_t structure as bit fields.
*/
typedef struct
{
    cy_en_dma_retrigger_t       retrigger;       /**< Specifies whether the DW controller should wait for the input trigger to be deactivated. */
    cy_en_dma_trigger_type_t    interruptType;   /**< Sets the event that triggers an interrupt, see \ref cy_en_dma_trigger_type_t. */
    cy_en_dma_trigger_type_t    triggerOutType;  /**< Sets the event that triggers an output,    see \ref cy_en_dma_trigger_type_t. */
    cy_en_dma_channel_state_t   channelState;    /**< Specifies if the channel is enabled or disabled on completion of descriptor see \ref cy_en_dma_channel_state_t. */
    cy_en_dma_trigger_type_t    triggerInType;   /**< Sets what type of transfer is triggered,   see \ref cy_en_dma_trigger_type_t. */
    cy_en_dma_data_size_t       dataSize;        /**< The size of the data bus for transfer,     see \ref cy_en_dma_data_size_t. */
    cy_en_dma_transfer_size_t   srcTransferSize; /**< The source transfer size. */
    cy_en_dma_transfer_size_t   dstTransferSize; /**< The destination transfer size. */
    cy_en_dma_descriptor_type_t descriptorType;  /**< The type of the descriptor                 see \ref cy_en_dma_descriptor_type_t. */
    void *                      srcAddress;      /**< The source address of the transfer. */
    void *                      dstAddress;      /**< The destination address of the transfer. */
    int32_t                     srcXincrement;   /**< The address increment of the source after each X-loop transfer. Valid range is -2048 ... 2047. */
    int32_t                     dstXincrement;   /**< The address increment of the destination after each X-loop transfer. Valid range is -2048 ... 2047. */
    uint32_t                    xCount;          /**< The number of transfers in an X-loop. Valid range is 1 ... 256. */
    int32_t                     srcYincrement;   /**< The address increment of the source after each Y-loop transfer. Valid range is -2048 ... 2047. */
    int32_t                     dstYincrement;   /**< The address increment of the destination after each Y-loop transfer. Valid range is -2048 ... 2047. */
    uint32_t                    yCount;          /**< The number of X-loops in the Y-loop. Valid range is 1 ... 256. */
    cy_stc_dma_descriptor_t *   nextDescriptor;  /**< The next descriptor to chain after completion, a NULL value will signify no chaining. */
} cy_stc_dma_descriptor_config_t;

/** This structure holds the initialization values for the DMA channel */
typedef struct
{
    cy_stc_dma_descriptor_t * descriptor;      /**< The DMA descriptor associated with the channel being initialized           */
    bool     preemptable;                      /**< Specifies if the channel is preemptable by another higher-priority channel */
    uint32_t priority;                         /**< This parameter specifies the channel's priority                            */
    bool     enable;                           /**< This parameter specifies if the channel is enabled after initializing      */
    bool     bufferable;                       /**< This parameter specifies whether a write transaction can complete
                                                    without waiting for the destination to accept the write transaction data.  */
} cy_stc_dma_channel_config_t;


/** This structure holds the initialization values for the CRC feature */
typedef struct
{
    bool     dataReverse;     /**< Specifies the bit order in which a data Byte is processed (reversal is performed after XORing):
                               * 'false': Most significant bit (bit 1) first.
                               * 'true': Least significant bit (bit 0) first.
                               */
    uint32_t dataXor;         /**< Specifies a byte mask with which each data byte is XOR'd. The XOR is performed before data reversal. */
    bool     reminderReverse; /**< Specifies whether the remainder is bit reversed (reversal is performed after XORing) */
    uint32_t reminderXor;     /**< Specifies a mask with which the remainder is XOR'd. The XOR is performed before remainder reversal. */
    uint32_t polynomial;      /**< CRC polynomial. The polynomial is represented WITHOUT the high order bit (this bit is always assumed '1').
                               * The polynomial should be aligned/shifted such that the more significant bits (bit 31 and down) contain the polynomial 
                               * and the less significant bits (bit 0 and up) contain padding '0's. Some frequently used polynomials:
                               * - CRC32: POLYNOMIAL is 0x04c11db7 (x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1).
                               * - CRC16: POLYNOMIAL is 0x80050000 (x^16 + x^15 + x^2 + 1, shifted by 16 bit positions).
                               * - CRC16 CCITT: POLYNOMIAL is 0x10210000 (x^16 + x^12 + x^5 + 1, shifted by 16 bit positions).
                               */
} cy_stc_dma_crc_config_t;

/** \} group_dma_data_structures */


/**
* \addtogroup group_dma_functions
* \{
*/


/**
* \addtogroup group_dma_block_functions
* \{
*/


__STATIC_INLINE void     Cy_DMA_Enable             (DW_Type * base);
__STATIC_INLINE void     Cy_DMA_Disable            (DW_Type * base);
__STATIC_INLINE uint32_t Cy_DMA_GetActiveChannel   (DW_Type const * base);
__STATIC_INLINE void *   Cy_DMA_GetActiveSrcAddress(DW_Type * base);
__STATIC_INLINE void *   Cy_DMA_GetActiveDstAddress(DW_Type * base);

      cy_en_dma_status_t Cy_DMA_Crc_Init             (DW_Type       * base, cy_stc_dma_crc_config_t const * crcConfig);

/** \} group_dma_block_functions */


/**
* \addtogroup group_dma_channel_functions
* \{
*/
      cy_en_dma_status_t Cy_DMA_Channel_Init                    (DW_Type       * base, uint32_t channel, cy_stc_dma_channel_config_t const * channelConfig);
                void     Cy_DMA_Channel_DeInit                  (DW_Type       * base, uint32_t channel);
__STATIC_INLINE void     Cy_DMA_Channel_SetDescriptor           (DW_Type       * base, uint32_t channel, cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE void     Cy_DMA_Channel_Enable                  (DW_Type       * base, uint32_t channel);
__STATIC_INLINE void     Cy_DMA_Channel_Disable                 (DW_Type       * base, uint32_t channel);
__STATIC_INLINE void     Cy_DMA_Channel_SetPriority             (DW_Type       * base, uint32_t channel, uint32_t priority);
__STATIC_INLINE uint32_t Cy_DMA_Channel_GetPriority             (DW_Type const * base, uint32_t channel);
__STATIC_INLINE
  cy_en_dma_intr_cause_t Cy_DMA_Channel_GetStatus               (DW_Type const * base, uint32_t channel);
__STATIC_INLINE
cy_stc_dma_descriptor_t * Cy_DMA_Channel_GetCurrentDescriptor   (DW_Type const * base, uint32_t channel);

__STATIC_INLINE uint32_t Cy_DMA_Channel_GetInterruptStatus      (DW_Type const * base, uint32_t channel);
__STATIC_INLINE void     Cy_DMA_Channel_ClearInterrupt          (DW_Type       * base, uint32_t channel);
__STATIC_INLINE void     Cy_DMA_Channel_SetInterrupt            (DW_Type       * base, uint32_t channel);
__STATIC_INLINE uint32_t Cy_DMA_Channel_GetInterruptMask        (DW_Type const * base, uint32_t channel);
__STATIC_INLINE void     Cy_DMA_Channel_SetInterruptMask        (DW_Type       * base, uint32_t channel, uint32_t interrupt);
__STATIC_INLINE uint32_t Cy_DMA_Channel_GetInterruptStatusMasked(DW_Type const * base, uint32_t channel);

/** \} group_dma_channel_functions */


/**
* \addtogroup group_dma_descriptor_functions
* \{
*/

  cy_en_dma_status_t Cy_DMA_Descriptor_Init  (cy_stc_dma_descriptor_t * descriptor, cy_stc_dma_descriptor_config_t const * config);
                void Cy_DMA_Descriptor_DeInit(cy_stc_dma_descriptor_t * descriptor);
                
                void Cy_DMA_Descriptor_SetNextDescriptor   (cy_stc_dma_descriptor_t * descriptor, cy_stc_dma_descriptor_t const * nextDescriptor);
                void Cy_DMA_Descriptor_SetDescriptorType   (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_descriptor_type_t descriptorType);
__STATIC_INLINE void Cy_DMA_Descriptor_SetSrcAddress       (cy_stc_dma_descriptor_t * descriptor, void const * srcAddress);
__STATIC_INLINE void Cy_DMA_Descriptor_SetDstAddress       (cy_stc_dma_descriptor_t * descriptor, void const * dstAddress);
__STATIC_INLINE void Cy_DMA_Descriptor_SetXloopDataCount   (cy_stc_dma_descriptor_t * descriptor, uint32_t xCount);
__STATIC_INLINE void Cy_DMA_Descriptor_SetYloopDataCount   (cy_stc_dma_descriptor_t * descriptor, uint32_t yCount);
__STATIC_INLINE void Cy_DMA_Descriptor_SetXloopSrcIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t srcXincrement);
__STATIC_INLINE void Cy_DMA_Descriptor_SetXloopDstIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t dstXincrement);
__STATIC_INLINE void Cy_DMA_Descriptor_SetYloopSrcIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t srcYincrement);
__STATIC_INLINE void Cy_DMA_Descriptor_SetYloopDstIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t dstYincrement);
__STATIC_INLINE void Cy_DMA_Descriptor_SetInterruptType    (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_trigger_type_t interruptType);
__STATIC_INLINE void Cy_DMA_Descriptor_SetTriggerInType    (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_trigger_type_t triggerInType);
__STATIC_INLINE void Cy_DMA_Descriptor_SetTriggerOutType   (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_trigger_type_t triggerOutType);
__STATIC_INLINE void Cy_DMA_Descriptor_SetDataSize         (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_data_size_t dataSize);
__STATIC_INLINE void Cy_DMA_Descriptor_SetSrcTransferSize  (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_transfer_size_t srcTransferSize);
__STATIC_INLINE void Cy_DMA_Descriptor_SetDstTransferSize  (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_transfer_size_t dstTransferSize);
__STATIC_INLINE void Cy_DMA_Descriptor_SetRetrigger        (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_retrigger_t retrigger);
__STATIC_INLINE void Cy_DMA_Descriptor_SetChannelState     (cy_stc_dma_descriptor_t * descriptor, cy_en_dma_channel_state_t channelState);

                cy_stc_dma_descriptor_t *   Cy_DMA_Descriptor_GetNextDescriptor   (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_descriptor_type_t Cy_DMA_Descriptor_GetDescriptorType   (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE void *                      Cy_DMA_Descriptor_GetSrcAddress       (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE void *                      Cy_DMA_Descriptor_GetDstAddress       (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE uint32_t                    Cy_DMA_Descriptor_GetXloopDataCount   (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE uint32_t                    Cy_DMA_Descriptor_GetYloopDataCount   (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE int32_t                     Cy_DMA_Descriptor_GetXloopSrcIncrement(cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE int32_t                     Cy_DMA_Descriptor_GetXloopDstIncrement(cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE int32_t                     Cy_DMA_Descriptor_GetYloopSrcIncrement(cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE int32_t                     Cy_DMA_Descriptor_GetYloopDstIncrement(cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_trigger_type_t    Cy_DMA_Descriptor_GetInterruptType    (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_trigger_type_t    Cy_DMA_Descriptor_GetTriggerInType    (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_trigger_type_t    Cy_DMA_Descriptor_GetTriggerOutType   (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_data_size_t       Cy_DMA_Descriptor_GetDataSize         (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_transfer_size_t   Cy_DMA_Descriptor_GetSrcTransferSize  (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_transfer_size_t   Cy_DMA_Descriptor_GetDstTransferSize  (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_retrigger_t       Cy_DMA_Descriptor_GetRetrigger        (cy_stc_dma_descriptor_t const * descriptor);
__STATIC_INLINE cy_en_dma_channel_state_t   Cy_DMA_Descriptor_GetChannelState     (cy_stc_dma_descriptor_t const * descriptor);

/** \} group_dma_descriptor_functions */



/***************************************
*    In-line Function Implementation
***************************************/


/**
* \addtogroup group_dma_block_functions
* \{
*/


/*******************************************************************************
* Function Name: Cy_DMA_Enable
****************************************************************************//**
*
* Enables the DMA block.
*
* \param base
* The pointer to the hardware DMA block.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Enable
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Enable(DW_Type * base)
{
    DW_CTL(base) |= DW_CTL_ENABLED_Msk;
}


/*******************************************************************************
* Function Name: Cy_DMA_Disable
****************************************************************************//**
*
* Disables the DMA block.
*
* \param base
* The pointer to the hardware DMA block.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Disable
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Disable(DW_Type * base)
{
    DW_CTL(base) &= (uint32_t) ~DW_CTL_ENABLED_Msk;
}


/*******************************************************************************
* Function Name: Cy_DMA_GetActiveChannel
****************************************************************************//**
*
* Returns the status of the active/pending channels.
* the DMA block.
*
* \param base
* The pointer to the hardware DMA block.
*
* \return
* Returns a bit-field with all of the currently active/pending channels in the
* DMA block.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Disable
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_DMA_GetActiveChannel(DW_Type const * base)
{
    return(_FLD2VAL(DW_STATUS_CH_IDX, DW_STATUS(base)));
}


/*******************************************************************************
* Function Name: Cy_DMA_GetActiveSrcAddress
****************************************************************************//**
*
* Returns the source address being used for the current transfer.
*
* \param base
* The pointer to the hardware DMA block.
*
* \return
* Returns the pointer to the source of transfer.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_GetActiveSrcAddress
*
*******************************************************************************/
__STATIC_INLINE void * Cy_DMA_GetActiveSrcAddress(DW_Type * base)
{
    return ((void *) DW_DESCR_SRC(base));
}


/*******************************************************************************
* Function Name: Cy_DMA_GetActiveDstAddress
****************************************************************************//**
*
* Returns the destination address being used for the current transfer.
*
* \param base
* The pointer to the hardware DMA block.
*
* \return
* Returns the pointer to the destination of transfer.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_GetActiveSrcAddress
*
*******************************************************************************/
__STATIC_INLINE void * Cy_DMA_GetActiveDstAddress(DW_Type * base)
{
    return ((void *) DW_DESCR_DST(base));
}

/** \} group_dma_block_functions */

/**
* \addtogroup group_dma_descriptor_functions
* \{
*/


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetSrcAddress
****************************************************************************//**
*
* Sets the source address parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param srcAddress
* The source address value for the descriptor.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetSrcAddress(cy_stc_dma_descriptor_t * descriptor, void const * srcAddress)
{
    descriptor->src = (uint32_t) srcAddress;
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetSrcAddress
****************************************************************************//**
*
* Returns the source address parameter of the specified descriptor.
* 
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The source address value of the descriptor.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void * Cy_DMA_Descriptor_GetSrcAddress(cy_stc_dma_descriptor_t const * descriptor)
{
    return ((void *) descriptor->src);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetDstAddress
****************************************************************************//**
*
* Sets the destination address parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param dstAddress
* The destination address value for the descriptor.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetDstAddress(cy_stc_dma_descriptor_t * descriptor, void const * dstAddress)
{
    descriptor->dst = (uint32_t) dstAddress;
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetDstAddress
****************************************************************************//**
*
* Returns the destination address parameter of the specified descriptor.
* 
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The destination address value of the descriptor.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void * Cy_DMA_Descriptor_GetDstAddress(cy_stc_dma_descriptor_t const * descriptor)
{
    return ((void *) descriptor->dst);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetInterruptType
****************************************************************************//**
*
* Sets the interrupt type parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param interruptType
* The interrupt type set for the descriptor. \ref cy_en_dma_trigger_type_t
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetInterruptType(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_trigger_type_t interruptType)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_TRIG_TYPE_VALID(interruptType));
    
    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_INTR_TYPE, interruptType);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetInterruptType
****************************************************************************//**
*
* Returns the Interrupt-Type of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Interrupt-Type \ref cy_en_dma_trigger_type_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_trigger_type_t Cy_DMA_Descriptor_GetInterruptType(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_trigger_type_t) _FLD2VAL(CY_DMA_CTL_INTR_TYPE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetTriggerInType
****************************************************************************//**
*
* Sets the Trigger-In-Type parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param triggerInType
* The Trigger In Type parameter \ref cy_en_dma_trigger_type_t
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetTriggerInType(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_trigger_type_t triggerInType)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_TRIG_TYPE_VALID(triggerInType));
    
    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_TR_IN_TYPE, triggerInType);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetTriggerInType
****************************************************************************//**
*
* Returns the Trigger-In-Type parameter of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Trigger-In-Type \ref cy_en_dma_trigger_type_t
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_trigger_type_t Cy_DMA_Descriptor_GetTriggerInType(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_trigger_type_t) _FLD2VAL(CY_DMA_CTL_TR_IN_TYPE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetTriggerOutType
****************************************************************************//**
*
* Sets the Trigger-Out-Type parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param triggerOutType
* The Trigger-Out-Type set for the descriptor. \ref cy_en_dma_trigger_type_t
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetTriggerOutType(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_trigger_type_t triggerOutType)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_TRIG_TYPE_VALID(triggerOutType));
    
    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_TR_OUT_TYPE, triggerOutType);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetTriggerOutType
****************************************************************************//**
*
* Returns the Trigger-Out-Type parameter of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Trigger-Out-Type parameter \ref cy_en_dma_trigger_type_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_trigger_type_t Cy_DMA_Descriptor_GetTriggerOutType(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_trigger_type_t) _FLD2VAL(CY_DMA_CTL_TR_OUT_TYPE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetDataSize
****************************************************************************//**
*
* Sets the Data Element Size parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param dataSize
* The Data Element Size \ref cy_en_dma_data_size_t
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetDataSize(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_data_size_t dataSize)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_DATA_SIZE_VALID(dataSize));
    
    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_DATA_SIZE, dataSize);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetDataSize
****************************************************************************//**
*
* Returns the Data Element Size parameter of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Data Element Size \ref cy_en_dma_data_size_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_data_size_t Cy_DMA_Descriptor_GetDataSize(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_data_size_t) _FLD2VAL(CY_DMA_CTL_DATA_SIZE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetSrcTransferSize
****************************************************************************//**
*
* Sets the Source Transfer Size parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param srcTransferSize
* The Source Transfer Size \ref cy_en_dma_transfer_size_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetSrcTransferSize(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_transfer_size_t srcTransferSize)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_XFER_SIZE_VALID(srcTransferSize));

    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_SRC_SIZE, srcTransferSize);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetSrcTransferSize
****************************************************************************//**
*
* Returns the Source Transfer Size parameter of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Source Transfer Size \ref cy_en_dma_transfer_size_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_transfer_size_t Cy_DMA_Descriptor_GetSrcTransferSize(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_transfer_size_t) _FLD2VAL(CY_DMA_CTL_SRC_SIZE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetDstTransferSize
****************************************************************************//**
*
* Sets the Destination Transfer Size parameter for the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param dstTransferSize
* The Destination Transfer Size \ref cy_en_dma_transfer_size_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetDstTransferSize(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_transfer_size_t dstTransferSize)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_XFER_SIZE_VALID(dstTransferSize));
    
    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_DST_SIZE, dstTransferSize);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetDstTransferSize
****************************************************************************//**
*
* Returns the Destination Transfer Size parameter of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Destination Transfer Size \ref cy_en_dma_transfer_size_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_transfer_size_t Cy_DMA_Descriptor_GetDstTransferSize(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_transfer_size_t) _FLD2VAL(CY_DMA_CTL_DST_SIZE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetRetrigger
****************************************************************************//**
*
* Sets the retrigger value which specifies whether the controller should 
* wait for the input trigger to be deactivated.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param retrigger 
* The \ref cy_en_dma_retrigger_t parameter specifies whether the controller
* should wait for the input trigger to be deactivated.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetRetrigger(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_retrigger_t retrigger)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_RETRIG_VALID(retrigger));
    
    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_RETRIG, retrigger);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetRetrigger
****************************************************************************//**
*
* Returns a value which specifies whether the controller should 
* wait for the input trigger to be deactivated.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Retrigger setting \ref cy_en_dma_retrigger_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_retrigger_t Cy_DMA_Descriptor_GetRetrigger(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_retrigger_t) _FLD2VAL(CY_DMA_CTL_RETRIG, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetDescriptorType
****************************************************************************//**
*
* Returns the descriptor's type of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The descriptor type \ref cy_en_dma_descriptor_type_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_descriptor_type_t Cy_DMA_Descriptor_GetDescriptorType(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_descriptor_type_t) _FLD2VAL(CY_DMA_CTL_TYPE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetChannelState
****************************************************************************//**
*
* Sets the channel state on completion of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param channelState 
* The channel state \ref cy_en_dma_channel_state_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetChannelState(cy_stc_dma_descriptor_t * descriptor, cy_en_dma_channel_state_t channelState)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L3(CY_DMA_IS_CHANNEL_STATE_VALID(channelState));
    
    CY_REG32_CLR_SET(descriptor->ctl, CY_DMA_CTL_CH_DISABLE, channelState);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetChannelState
****************************************************************************//**
*
* Returns the channel state on completion of the specified descriptor.
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The Channel State setting \ref cy_en_dma_channel_state_t.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE cy_en_dma_channel_state_t Cy_DMA_Descriptor_GetChannelState(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    
    return((cy_en_dma_channel_state_t) _FLD2VAL(CY_DMA_CTL_CH_DISABLE, descriptor->ctl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetXloopDataCount
****************************************************************************//**
*
* Sets the number of data elements to transfer in the X loop
* for the specified descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param xCount
* The number of data elements to transfer in the X loop. Valid range is 1 ... 256.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetXloopDataCount(cy_stc_dma_descriptor_t * descriptor, uint32_t xCount)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_SINGLE_TRANSFER != Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    CY_ASSERT_L2(CY_DMA_IS_LOOP_COUNT_VALID(xCount));
    /* Convert the data count from the user's range (1-256) into the machine range (0-255). */
    CY_REG32_CLR_SET(descriptor->xCtl, CY_DMA_CTL_COUNT, xCount - 1UL);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetXloopDataCount
****************************************************************************//**
*
* Returns the number of data elements for the X loop of the specified
* descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The number of data elements to transfer in the X loop. The range is 1 ... 256.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_DMA_Descriptor_GetXloopDataCount(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_SINGLE_TRANSFER != Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    /* Convert the data count from the machine range (0-255) into the user's range (1-256). */
    return (_FLD2VAL(CY_DMA_CTL_COUNT, descriptor->xCtl) + 1UL);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetXloopSrcIncrement
****************************************************************************//**
*
* Sets the source increment parameter for the X loop of the specified
* descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param srcXincrement
* The value of the source increment. The valid range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetXloopSrcIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t srcXincrement)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_SINGLE_TRANSFER != Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    CY_ASSERT_L2(CY_DMA_IS_LOOP_INCR_VALID(srcXincrement));
    
    CY_REG32_CLR_SET(descriptor->xCtl, CY_DMA_CTL_SRC_INCR, srcXincrement);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetXloopSrcIncrement
****************************************************************************//**
*
* Returns the source increment parameter for the X loop of the specified
* descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The value of the source increment. The range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE int32_t Cy_DMA_Descriptor_GetXloopSrcIncrement(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_SINGLE_TRANSFER != Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    
    return ((int32_t) _FLD2VAL(CY_DMA_CTL_SRC_INCR, descriptor->xCtl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetXloopDstIncrement
****************************************************************************//**
*
* Sets the destination increment parameter for the X loop for the specified
* descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param dstXincrement
* The value of the destination increment. The valid range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetXloopDstIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t dstXincrement)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_SINGLE_TRANSFER != Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    CY_ASSERT_L2(CY_DMA_IS_LOOP_INCR_VALID(dstXincrement));
    
    CY_REG32_CLR_SET(descriptor->xCtl, CY_DMA_CTL_DST_INCR, dstXincrement);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetXloopDstIncrement
****************************************************************************//**
*
* Returns the destination increment parameter for the X loop of the specified
* descriptor (for 1D or 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The value of the destination increment. The range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE int32_t Cy_DMA_Descriptor_GetXloopDstIncrement(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_SINGLE_TRANSFER != Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    
    return ((int32_t) _FLD2VAL(CY_DMA_CTL_DST_INCR, descriptor->xCtl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetYloopDataCount
****************************************************************************//**
*
* Sets the number of data elements for the Y loop of the specified descriptor
* (for 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param yCount
* The number of X loops to execute in the Y loop. The valid range is 1 ... 256.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetYloopDataCount(cy_stc_dma_descriptor_t * descriptor, uint32_t yCount)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_2D_TRANSFER == Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    CY_ASSERT_L2(CY_DMA_IS_LOOP_COUNT_VALID(yCount));
    /* Convert the data count from the user's range (1-256) into the machine range (0-255). */
    CY_REG32_CLR_SET(descriptor->yCtl, CY_DMA_CTL_COUNT, yCount - 1UL);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetYloopDataCount
****************************************************************************//**
*
* Returns the number of X loops to execute in the Y loop of the specified
* descriptor (for 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The number of X loops to execute in the Y loop. The range is 1 ... 256.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_DMA_Descriptor_GetYloopDataCount(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_2D_TRANSFER == Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    /* Convert the data count from the machine range (0-255) into the user's range (1-256). */
    return (_FLD2VAL(CY_DMA_CTL_COUNT, descriptor->yCtl) + 1UL);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetYloopSrcIncrement
****************************************************************************//**
*
* Sets the source increment parameter for the Y loop for the specified
* descriptor (for 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param srcYincrement
* The value of the source increment. The valid range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetYloopSrcIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t srcYincrement)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_2D_TRANSFER == Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    CY_ASSERT_L2(CY_DMA_IS_LOOP_INCR_VALID(srcYincrement));
    
    CY_REG32_CLR_SET(descriptor->yCtl, CY_DMA_CTL_SRC_INCR, srcYincrement);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetYloopSrcIncrement
****************************************************************************//**
*
* Returns the source increment parameter for the outer Y of the specified
* descriptor (for 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The value of the source increment. The range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE int32_t Cy_DMA_Descriptor_GetYloopSrcIncrement(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_2D_TRANSFER == Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    
    return ((int32_t) _FLD2VAL(CY_DMA_CTL_SRC_INCR, descriptor->yCtl));
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_SetYloopDstIncrement
****************************************************************************//**
*
* Sets the destination increment parameter for the Y loop of the specified
* descriptor (for 2D descriptors only).
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \param dstYincrement
* The value of the destination increment. The valid range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_SetNextDescriptor
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Descriptor_SetYloopDstIncrement(cy_stc_dma_descriptor_t * descriptor, int32_t dstYincrement)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_2D_TRANSFER == Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    CY_ASSERT_L2(CY_DMA_IS_LOOP_INCR_VALID(dstYincrement));
    
    CY_REG32_CLR_SET(descriptor->yCtl, CY_DMA_CTL_DST_INCR, dstYincrement);
}


/*******************************************************************************
* Function Name: Cy_DMA_Descriptor_GetYloopDstIncrement
****************************************************************************//**
*
* Returns the destination increment parameter for the Y loop of the specified
* descriptor (for 2D descriptors only). 
*
* \param descriptor
* The descriptor structure instance declared by the user/component.
*
* \return
* The value of the destination increment. The range is -2048 ... 2047.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_GetNextDescriptor
*  
*******************************************************************************/
__STATIC_INLINE int32_t Cy_DMA_Descriptor_GetYloopDstIncrement(cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(NULL != descriptor);
    CY_ASSERT_L1(CY_DMA_2D_TRANSFER == Cy_DMA_Descriptor_GetDescriptorType(descriptor));
    
    return ((int32_t) _FLD2VAL(CY_DMA_CTL_DST_INCR, descriptor->yCtl));
}


/** \} group_dma_descriptor_functions */


/**
* \addtogroup group_dma_channel_functions
* \{
*/


/*******************************************************************************
* Function Name: Cy_DMA_Channel_SetDescriptor
****************************************************************************//**
*
* Sets a descriptor as current for the specified DMA channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \param descriptor
* This is the descriptor to be associated with the channel.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Enable
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Channel_SetDescriptor(DW_Type * base, uint32_t channel, cy_stc_dma_descriptor_t const * descriptor)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    CY_ASSERT_L1(NULL != descriptor);
    
    DW_CH_CURR_PTR(base, channel) = (uint32_t)descriptor;
    DW_CH_IDX(base, channel) &= (uint32_t) ~(DW_CH_STRUCT_CH_IDX_X_IDX_Msk | DW_CH_STRUCT_CH_IDX_Y_IDX_Msk);
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_Enable
****************************************************************************//**
*
* The function is used to enable a DMA channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The  channel number.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Enable
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Channel_Enable(DW_Type * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    DW_CH_CTL(base, channel) |= DW_CH_STRUCT_CH_CTL_ENABLED_Msk;
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_Disable
****************************************************************************//**
*
* The function is used to disable a DMA channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Disable
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Channel_Disable(DW_Type * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    DW_CH_CTL(base, channel) &= (uint32_t) ~DW_CH_STRUCT_CH_CTL_ENABLED_Msk;
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_SetPriority
****************************************************************************//**
*
* The function is used to set a priority for the DMA channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \param priority
* The priority to be set for the DMA channel. The allowed values are 0,1,2,3.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Enable
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Channel_SetPriority(DW_Type * base, uint32_t channel, uint32_t priority)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    CY_ASSERT_L2(CY_DMA_IS_PRIORITY_VALID(priority));
    
    CY_REG32_CLR_SET(DW_CH_CTL(base, channel), DW_CH_STRUCT_CH_CTL_PRIO, priority);
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_GetPriority
****************************************************************************//**
*
* Returns the priority of the DMA channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \return
* The priority of the channel.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Disable
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_DMA_Channel_GetPriority(DW_Type const * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    return ((uint32_t) _FLD2VAL(DW_CH_STRUCT_CH_CTL_PRIO, DW_CH_CTL(base, channel)));
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_GetCurrentDescriptor
****************************************************************************//**
*
* Returns the descriptor that is active in the channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \return
* The pointer to the descriptor assocaited with the channel.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_Descriptor_Deinit
*
*******************************************************************************/
__STATIC_INLINE cy_stc_dma_descriptor_t * Cy_DMA_Channel_GetCurrentDescriptor(DW_Type const * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    return ((cy_stc_dma_descriptor_t*)(DW_CH_CURR_PTR(base, channel)));
}



/*******************************************************************************
* Function Name: Cy_DMA_Channel_GetInterruptStatus
****************************************************************************//**
*
* Returns the interrupt status of the specified channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \return
* The status of an interrupt for the specified channel.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_GetInterruptStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_DMA_Channel_GetInterruptStatus(DW_Type const * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    return (DW_CH_INTR(base, channel));
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_GetStatus
****************************************************************************//**
*
* Returns the interrupt reason of the specified channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \return
* The cause \ref cy_en_dma_intr_cause_t of the interrupt.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_ClearInterrupt
*
*******************************************************************************/
__STATIC_INLINE cy_en_dma_intr_cause_t Cy_DMA_Channel_GetStatus(DW_Type const * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    return ((cy_en_dma_intr_cause_t) _FLD2VAL(DW_CH_STRUCT_CH_STATUS_INTR_CAUSE, DW_CH_STATUS(base, channel)));
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_ClearInterrupt
****************************************************************************//**
*
* Clears the interrupt status of the specified channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_ClearInterrupt
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Channel_ClearInterrupt(DW_Type * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    DW_CH_INTR(base, channel) = CY_DMA_INTR_MASK;
    (void) DW_CH_INTR(base, channel);
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_SetInterrupt
****************************************************************************//**
*
* Sets the interrupt for the specified channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_SetInterruptMask
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Channel_SetInterrupt(DW_Type * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    DW_CH_INTR_SET(base, channel) = CY_DMA_INTR_MASK;
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_GetInterruptMask
****************************************************************************//**
*
* Returns the interrupt mask value of the specified channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \return
* The interrupt mask value.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_SetInterruptMask
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_DMA_Channel_GetInterruptMask(DW_Type const * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    return (DW_CH_INTR_MASK(base, channel));
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_SetInterruptMask
****************************************************************************//**
*
* Sets an interrupt mask value for the specified channel.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \param interrupt
* The interrupt mask:
* CY_DMA_INTR_MASK to enable the interrupt or 0UL to disable the interrupt.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_SetInterruptMask
*
*******************************************************************************/
__STATIC_INLINE void Cy_DMA_Channel_SetInterruptMask(DW_Type * base, uint32_t channel, uint32_t interrupt)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    CY_ASSERT_L2(CY_DMA_IS_INTR_MASK_VALID(interrupt));
    DW_CH_INTR_MASK(base, channel) = interrupt;
}


/*******************************************************************************
* Function Name: Cy_DMA_Channel_GetInterruptStatusMasked
****************************************************************************//**
*
* Returns the logical AND of the corresponding INTR and INTR_MASK fields
* in a single-load operation.
*
* \param base
* The pointer to the hardware DMA block.
*
* \param channel
* The channel number.
*
* \funcusage 
* \snippet dma\2.10\snippet\main.c snippet_Cy_DMA_ClearInterrupt
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_DMA_Channel_GetInterruptStatusMasked(DW_Type const * base, uint32_t channel)
{
    CY_ASSERT_L1(CY_DMA_IS_CH_NR_VALID(channel));
    
    return (DW_CH_INTR_MASKED(base, channel));
}


/** \} group_dma_channel_functions */

/** \} group_dma_functions */


/** \cond The definitions to support the backward compatibility, do not use them in new designs */

#if(0U != CY_DMA_BWC)

    /* Type definitions */
    #define cy_stc_dma_chnl_config_t        cy_stc_dma_channel_config_t
    #define cy_stc_dma_descr_t              cy_stc_dma_descriptor_t
    #define cy_stc_dma_descr_config_t       cy_stc_dma_descriptor_config_t
    #define cy_en_dma_trig_type_t           cy_en_dma_trigger_type_t

    /* Structure items */
    #define DMA_Descriptor                  descriptor
    #define deact                           retrigger
    #define intrType                        interruptType
    #define chStateAtCmplt                  channelState
    #define srcTxfrSize                     srcTransferSize
    #define destTxfrSize                    dstTransferSize
    #define trigoutType                     triggerOutType
    #define triginType                      triggerInType
    #define descrType                       descriptorType
    #define srcAddr                         srcAddress
    #define destAddr                        dstAddress
    #define srcXincr                        srcXincrement
    #define srcYincr                        srcYincrement
    #define destXincr                       dstXincrement
    #define destYincr                       dstYincrement
    #define descrNext                       nextDescriptor

    /* Constants */
    #define CY_DMA_CH_DISABLED              (CY_DMA_CHANNEL_DISABLED)
    #define CY_DMA_CH_ENABLED               (CY_DMA_CHANNEL_ENABLED)

    #define CY_DMA_TXFR_SIZE_DATA_SIZE      (CY_DMA_TRANSFER_SIZE_DATA)
    #define CY_DMA_TXFR_SIZE_WORD           (CY_DMA_TRANSFER_SIZE_WORD)

    #define CY_DMA_INTR_1ELEMENT_CMPLT      (CY_DMA_1ELEMENT)
    #define CY_DMA_INTR_X_LOOP_CMPLT        (CY_DMA_X_LOOP)
    #define CY_DMA_INTR_DESCR_CMPLT         (CY_DMA_DESCR)
    #define CY_DMA_INTR_DESCRCHAIN_CMPLT    (CY_DMA_DESCR_CHAIN)

    #define CY_DMA_TRIGOUT_1ELEMENT_CMPLT   (CY_DMA_1ELEMENT)
    #define CY_DMA_TRIGOUT_X_LOOP_CMPLT     (CY_DMA_X_LOOP)
    #define CY_DMA_TRIGOUT_DESCR_CMPLT      (CY_DMA_DESCR)
    #define CY_DMA_TRIGOUT_DESCRCHAIN_CMPLT (CY_DMA_DESCR_CHAIN)
        
    #define CY_DMA_TRIGIN_1ELEMENT          (CY_DMA_1ELEMENT)
    #define CY_DMA_TRIGIN_XLOOP             (CY_DMA_X_LOOP)
    #define CY_DMA_TRIGIN_DESCR             (CY_DMA_DESCR)
    #define CY_DMA_TRIGIN_DESCRCHAIN        (CY_DMA_DESCR_CHAIN)

    #define CY_DMA_INVALID_INPUT_PARAMETERS (CY_DMA_BAD_PARAM)
    
    #define CY_DMA_RETDIG_IM                (CY_DMA_RETRIG_IM)
    #define CY_DMA_RETDIG_4CYC              (CY_DMA_RETRIG_4CYC)
    #define CY_DMA_RETDIG_16CYC             (CY_DMA_RETRIG_16CYC)

    /* Descriptor structure items */
    #define DESCR_CTL                       ctl
    #define DESCR_SRC                       src
    #define DESCR_DST                       dst
    #define DESCR_X_CTL                     xCtl
    #define DESCR_Y_CTL                     yCtl
    #define DESCR_NEXT_PTR                  nextPtr
    
    /* Descriptor structure bit-fields */
    #define DW_DESCR_STRUCT_DESCR_CTL_WAIT_FOR_DEACT_Pos 0UL
    #define DW_DESCR_STRUCT_DESCR_CTL_WAIT_FOR_DEACT_Msk 0x3UL
    #define DW_DESCR_STRUCT_DESCR_CTL_INTR_TYPE_Pos 2UL
    #define DW_DESCR_STRUCT_DESCR_CTL_INTR_TYPE_Msk 0xCUL
    #define DW_DESCR_STRUCT_DESCR_CTL_TR_OUT_TYPE_Pos 4UL
    #define DW_DESCR_STRUCT_DESCR_CTL_TR_OUT_TYPE_Msk 0x30UL
    #define DW_DESCR_STRUCT_DESCR_CTL_TR_IN_TYPE_Pos 6UL
    #define DW_DESCR_STRUCT_DESCR_CTL_TR_IN_TYPE_Msk 0xC0UL
    #define DW_DESCR_STRUCT_DESCR_CTL_CH_DISABLE_Pos 24UL
    #define DW_DESCR_STRUCT_DESCR_CTL_CH_DISABLE_Msk 0x1000000UL
    #define DW_DESCR_STRUCT_DESCR_CTL_SRC_TRANSFER_SIZE_Pos 26UL
    #define DW_DESCR_STRUCT_DESCR_CTL_SRC_TRANSFER_SIZE_Msk 0x4000000UL
    #define DW_DESCR_STRUCT_DESCR_CTL_DST_TRANSFER_SIZE_Pos 27UL
    #define DW_DESCR_STRUCT_DESCR_CTL_DST_TRANSFER_SIZE_Msk 0x8000000UL
    #define DW_DESCR_STRUCT_DESCR_CTL_DATA_SIZE_Pos 28UL
    #define DW_DESCR_STRUCT_DESCR_CTL_DATA_SIZE_Msk 0x30000000UL
    #define DW_DESCR_STRUCT_DESCR_CTL_DESCR_TYPE_Pos 30UL
    #define DW_DESCR_STRUCT_DESCR_CTL_DESCR_TYPE_Msk 0xC0000000UL
    #define DW_DESCR_STRUCT_DESCR_SRC_SRC_ADDR_Pos  0UL
    #define DW_DESCR_STRUCT_DESCR_SRC_SRC_ADDR_Msk  0xFFFFFFFFUL
    #define DW_DESCR_STRUCT_DESCR_DST_DST_ADDR_Pos  0UL
    #define DW_DESCR_STRUCT_DESCR_DST_DST_ADDR_Msk  0xFFFFFFFFUL
    #define DW_DESCR_STRUCT_DESCR_X_CTL_SRC_X_INCR_Pos 0UL
    #define DW_DESCR_STRUCT_DESCR_X_CTL_SRC_X_INCR_Msk 0xFFFUL
    #define DW_DESCR_STRUCT_DESCR_X_CTL_DST_X_INCR_Pos 12UL
    #define DW_DESCR_STRUCT_DESCR_X_CTL_DST_X_INCR_Msk 0xFFF000UL
    #define DW_DESCR_STRUCT_DESCR_X_CTL_X_COUNT_Pos 24UL
    #define DW_DESCR_STRUCT_DESCR_X_CTL_X_COUNT_Msk 0xFF000000UL
    #define DW_DESCR_STRUCT_DESCR_Y_CTL_SRC_Y_INCR_Pos 0UL
    #define DW_DESCR_STRUCT_DESCR_Y_CTL_SRC_Y_INCR_Msk 0xFFFUL
    #define DW_DESCR_STRUCT_DESCR_Y_CTL_DST_Y_INCR_Pos 12UL
    #define DW_DESCR_STRUCT_DESCR_Y_CTL_DST_Y_INCR_Msk 0xFFF000UL
    #define DW_DESCR_STRUCT_DESCR_Y_CTL_Y_COUNT_Pos 24UL
    #define DW_DESCR_STRUCT_DESCR_Y_CTL_Y_COUNT_Msk 0xFF000000UL
    #define DW_DESCR_STRUCT_DESCR_NEXT_PTR_ADDR_Pos 2UL
    #define DW_DESCR_STRUCT_DESCR_NEXT_PTR_ADDR_Msk 0xFFFFFFFCUL

    /* Functions */
    #define Cy_DMA_GetActiveChnl            Cy_DMA_GetActiveChannel
    #define Cy_DMA_GetActiveSrcAddr         Cy_DMA_GetActiveSrcAddress
    #define Cy_DMA_GetActiveDstAddr         Cy_DMA_GetActiveDstAddress
    #define Cy_DMA_Descr_Init               Cy_DMA_Descriptor_Init
    #define Cy_DMA_Descr_DeInit             Cy_DMA_Descriptor_DeInit
    #define Cy_DMA_Descr_SetSrcAddr         Cy_DMA_Descriptor_SetSrcAddress
    #define Cy_DMA_Descr_SetDestAddr        Cy_DMA_Descriptor_SetDstAddress
    #define Cy_DMA_Descr_SetNxtDescr        Cy_DMA_Descriptor_SetNextDescriptor
    #define Cy_DMA_Descr_SetIntrType        Cy_DMA_Descriptor_SetInterruptType
    #define Cy_DMA_Descr_SetTrigInType      Cy_DMA_Descriptor_SetTriggerInType
    #define Cy_DMA_Descr_SetTrigOutType     Cy_DMA_Descriptor_SetTriggerOutType
    #define Cy_DMA_Chnl_Init                Cy_DMA_Channel_Init
    #define Cy_DMA_Chnl_DeInit              Cy_DMA_Channel_DeInit
    #define Cy_DMA_Chnl_SetDescr            Cy_DMA_Channel_SetDescriptor
    #define Cy_DMA_Chnl_Enable              Cy_DMA_Channel_Enable
    #define Cy_DMA_Chnl_Disable             Cy_DMA_Channel_Disable
    #define Cy_DMA_Chnl_GetCurrentDescr     Cy_DMA_Channel_GetCurrentDescriptor
    #define Cy_DMA_Chnl_SetPriority         Cy_DMA_Channel_SetPriority
    #define Cy_DMA_Chnl_GetPriority         Cy_DMA_Channel_GetPriority
    #define Cy_DMA_Chnl_GetInterruptStatus  Cy_DMA_Channel_GetInterruptStatus
    #define Cy_DMA_Chnl_GetInterruptCause   Cy_DMA_Channel_GetStatus
    #define Cy_DMA_Chnl_ClearInterrupt      Cy_DMA_Channel_ClearInterrupt
    #define Cy_DMA_Chnl_SetInterrupt        Cy_DMA_Channel_SetInterrupt
    #define Cy_DMA_Chnl_GetInterruptMask    Cy_DMA_Channel_GetInterruptMask
    #define Cy_DMA_Chnl_GetInterruptStatusMasked Cy_DMA_Channel_GetInterruptStatusMasked
    #define Cy_DMA_Chnl_SetInterruptMask(base, channel) (Cy_DMA_Channel_SetInterruptMask(base, channel, CY_DMA_INTR_MASK))


/*******************************************************************************
* Function Name: Cy_DMA_Descr_SetTxfrWidth
****************************************************************************//**
* This is a legacy API function, it is left here just for the backward compatibility
* Do not use it in new designs.
*******************************************************************************/
    __STATIC_INLINE void Cy_DMA_Descr_SetTxfrWidth(cy_stc_dma_descr_t * descriptor,
                                                               uint32_t dataElementSize,
                                                               uint32_t srcTxfrWidth,
                                                               uint32_t dstTxfrWidth)
    {
        uint32_t regValue;
        regValue = descriptor->ctl & ((uint32_t)(~(DW_DESCR_STRUCT_DESCR_CTL_DATA_SIZE_Msk |
            DW_DESCR_STRUCT_DESCR_CTL_SRC_TRANSFER_SIZE_Msk |
            DW_DESCR_STRUCT_DESCR_CTL_DST_TRANSFER_SIZE_Msk)));

        descriptor->ctl = regValue |
            _VAL2FLD(DW_DESCR_STRUCT_DESCR_CTL_DATA_SIZE, dataElementSize) |
            _VAL2FLD(DW_DESCR_STRUCT_DESCR_CTL_SRC_TRANSFER_SIZE, srcTxfrWidth) |
            _VAL2FLD(DW_DESCR_STRUCT_DESCR_CTL_DST_TRANSFER_SIZE, dstTxfrWidth);
    }

#endif /* CY_DMA_BWC */

/** \endcond */


#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_M4CPUSS_DMA */

#endif  /* (CY_DMA_H) */

/** \} group_dma */


/* [] END OF FILE */
