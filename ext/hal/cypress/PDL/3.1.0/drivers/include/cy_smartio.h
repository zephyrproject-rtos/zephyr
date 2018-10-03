/***************************************************************************//**
* \file cy_smartio.h
* \version 1.0
*
* \brief
* Provides an API declaration of the Smart I/O driver
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_smartio Smart I/O (SmartIO)
* \{
* The Smart I/O driver provides an API to configure and access the Smart I/O hardware
* present between the GPIOs (pins) and HSIOMs (pin muxes) on select device ports. It
* can be used to perform simple logic operations on peripheral and GPIO signals at
* the GPIO port. Features include,
*
* 1. Purely combinatorial logic using 8 Look-Up Tables (LUT).
* 2. Mixed sequential and combinatorial logic using 8 LUTs and 1 Data Unit (DU).
* 3. Counters and shift registers using 8-bit multi-function DU (simplified ALU).
* 4. Operational in device active, sleep and deep-sleep power modes.
*
* \section group_smartio_configuration Configuration Considerations
*
* \subsection group_smartio_routing_fabric Routing Fabric
* 
* The Smart I/O routing fabric is divided into two portions, where each portion is capable
* of accepting half of the data or GPIO signals. The LUTs have the following structure.
*
* - LUT 7...4 are capable of accepting signals from io/chip 7...4 as inputs.
* - LUT 3...0 are capable of accepting signals from io/chip 3...0 as inputs.
* - The LUTs can accept any LUT output as an input.
* - Each LUT output is dedicated to the corresponding output io/chip terminal. For
*   example, LUT 0 can go to either io0 terminal output or chip0 terminal output or both.
*
* \subsection group_smartio_single_lut Single Source LUT Input
* 
* If a LUT is used, all three inputs to the LUT must be designated. For example, even If a LUT is
* used to accept a single source as its input, all three inputs must accept that same signal. The
* lookup table should then be designed such that it only changes the output value when all
* three inputs satisfy the same condition. For example, consider the case where the signal on 
* data0 must be inverted before being passed to io0. LUT0 accepts chip0 as input 0, 1 and 2. 
* The truth table is defined such that it outputs a logic 1 only when the inputs are all 0.
*
* \subsection group_smartio_clk_rst Clock and Reset Behavior
*
* The Smart I/O hardware drives its synchronous elements using a single clock source. 
* Depending on the clock source, the Smart I/O will have different reset behaviors, which
* will reset all the flip-flops in the LUTs and synchronizers to logic 0. The configuration
* registers will retain their values unless coming out of Power on Reset (POR). Notes:
* 
* - If the block is only disabled, the values in the LUT flip-flips and I/O synchronizers are
*   held as long as the chip remains in a valid power mode.
* - The selected clock for the fabric's synchronous logic is not phase aligned with other
*   synchronous logic on the chip operating on the same clock. Therefore, communication between
*   the Smart I/O and other synchronous logic should be treated as asynchronous (just as the
*   communication between I/O input signals and other synchronous logic should be treated as
*   asynchronous)
*
* <table class="doxtable">
*   <tr>
*     <th>Clock Source</th>
*     <th>Reset Behavior</th>
*     <th>Enable Delay</th>
*     <th>Description</th>
*   </tr>
*   <tr>
*     <td>io/chip 7...0</td>
*     <td>Reset on POR</td>
*     <td>2 clock edges</td>
*     <td>If chosen as the clock source, that particular signal cannot also be used as an input
*         to a LUT as it may cause a race condition. The fabric will be enabled after 2 clock 
*         edges of the signal on the gpio terminal.</td>
*   </tr>
*   <tr>
*     <td>Divided Clock (Active)</td>
*     <td>Reset when going to Deep Sleep, Hibernate or POR</td>
*     <td>2 clock edges</td>
*     <td>The fabric will be enabled after 2 clock edges of the divided clock. Any synchronous
*         logic in the LUTs will be reset to 0 when in chip deep-sleep or hibernate modes.</td>
*   </tr>
*   <tr>
*     <td>Divided Clock (Deep-Sleep)</td>
*     <td>Reset when going to Hibernate or POR</td>
*     <td>2 clock edges</td>
*     <td>The fabric will be enabled after 2 clock edges of the divided clock. Any synchronous
*         logic in the LUTs will be reset to 0 when in hibernate mode.</td>
*   </tr>
*   <tr>
*     <td>Divided Clock (Hibernate)</td>
*     <td>Reset on POR</td>
*     <td>2 clock edges</td>
*     <td>The fabric will be enabled after 2 clock edges of the divided clock.</td>
*   </tr>
*   <tr>
*     <td>LFCLK</td>
*     <td>Reset when going to Hibernate and POR</td>
*     <td>2 clock edges</td>
*     <td>The fabric will be enabled after 2 clock edges of the low frequency clock (LFCLK). 
*         Any synchronous logic in the LUTs will be reset to 0 when in hibernate mode.</td>
*   </tr>
*   <tr>
*     <td>Asynchronous</td>
*     <td>Reset on POR</td>
*     <td>3 clock edges of SYSCLK</td>
*     <td>The fabric will be enabled after 3 clock edges of the clk_peri source.</td>
*   </tr>
* </table>
*
* \subsection group_smartio_sync_req Signal Synchronization Requirement
*
* If any of the signals coming in through the Smart I/O terminals are meant to be used
* in sequential elements in the LUTs, the terminal synchronizer must first be used to synchronize
* that signal to the Component clock. For example, if the signal on io0 must be used in LUT0 in
* Sequential output mode, the synchronization for io0 terminal should be enabled for reliable
* operation.
*
* \subsection group_smartio_comb_feedback LUT Combinatorial Feedback
* 
* Since the LUTs can be configured as purely (or partially) combinatorial elements and since they
* can chain to each other in any fashion, combinatorial timing loops can occur. This causes
* oscillations that burn power and create unpredictable behavior. If a feedback is required, the
* signals should always go through a flip-flop before feeding back. For example, the following is a
* potentially problematic design; LUT1 and LUT2 are configured in combinatorial mode, where their
* respective outputs feed into the inputs of the other. This will result in oscillations. 
* To prevent it, one of the LUTs should be configured to Gated Output mode.
*
* \subsection group_smartio_lpm Low Power Mode
* 
* The Smart I/O hardware is capable of operating during chip Deep-Sleep mode. The block has
* the following requirements when operating in this mode:
*
* - All sequential elements must be clocked by a valid clock in these power domains. Refer to
*   \ref group_smartio_clk_rst for more details.
* - All signals in the block (including the clock) must be less than 1 MHz when in Deep-Sleep
*   mode.
* - The hold override functionality should be enabled when entering Deep-Sleep mode. 
*   This functionality should then be disabled when the chip is not in this mode.
*
* \section group_smartio_more_information More Information
*
* Refer to the technical reference manual (TRM) and the device datasheet.
*
* \section group_smartio_MISRA MISRA-C Compliance]
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
*     <td>The value of a complex expression of integer type shall only be cast to a type of the
*         same signedness that is no wider than the underlying type of the expression.</td>
*     <td>Use of a Cypress defined macro to access memory-mapped objects.</td>
*   </tr>
*   <tr>
*     <td>16.7</td>
*     <td>A</td>
*     <td>A pointer parameter in a function prototype should be declared as pointer to const
*         if the pointer is not used to modify the addressed object.</td>
*     <td>Base address pointers are always constant in drivers. No need to pick and choose 
*         const declaration based on function construction.</td>
*   </tr>
* </table>
*
* \section group_smartio_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_smartio_macros Macros
* \defgroup group_smartio_functions Functions
* \{
*   \defgroup group_smartio_functions_init       Initialization Functions
*   \defgroup group_smartio_functions_general    General Functions
*   \defgroup group_smartio_functions_lut        LUT Functions
*   \defgroup group_smartio_functions_du         Data Unit Functions
* \}
* \defgroup group_smartio_data_structures Data Structures
* \defgroup group_smartio_enums Enumerated Types
*/

#if !defined(CY_SMARTIO_H)
#define CY_SMARTIO_H

#include <stdbool.h>
#include <stddef.h>
#include "cy_device_headers.h"
#include "cy_device.h"
#include "cy_syslib.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_smartio_macros
* \{
*/

/** Driver major version */
#define CY_SMARTIO_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define CY_SMARTIO_DRV_VERSION_MINOR       0

/** SmartIO driver ID */
#define CY_SMARTIO_ID CY_PDL_DRV_ID(0x42u)

/**
* \defgroup group_smartio_channels Smart I/O channel selection
* \{
* Constants for selecting the Smart I/O channels
*/
#define CY_SMARTIO_CHANNEL_NONE 0x00u   /**< No channels */
#define CY_SMARTIO_CHANNEL0     0x01u   /**< Channel 0 (chip0 <-> io0) */
#define CY_SMARTIO_CHANNEL1     0x02u   /**< Channel 1 (chip1 <-> io1) */
#define CY_SMARTIO_CHANNEL2     0x04u   /**< Channel 2 (chip2 <-> io2) */
#define CY_SMARTIO_CHANNEL3     0x08u   /**< Channel 3 (chip3 <-> io3) */
#define CY_SMARTIO_CHANNEL4     0x10u   /**< Channel 4 (chip4 <-> io4) */
#define CY_SMARTIO_CHANNEL5     0x20u   /**< Channel 5 (chip5 <-> io5) */
#define CY_SMARTIO_CHANNEL6     0x40u   /**< Channel 6 (chip6 <-> io6) */
#define CY_SMARTIO_CHANNEL7     0x80u   /**< Channel 7 (chip7 <-> io7) */
#define CY_SMARTIO_CHANNEL_ALL  0xffu   /**< All channels */
/** \} */

/** \} group_smartio_macros */


/***************************************
*       Constants
***************************************/

/** \cond INTERNAL */
#define CY_SMARTIO_ENABLE       1UL
#define CY_SMARTIO_DISABLE      0UL
#define CY_SMARTIO_DEINIT       0UL
#define CY_SMARTIO_LUTMAX       8u
/** \endcond */

/***************************************
*       Enumerations
***************************************/
/**
* \addtogroup group_smartio_enums
* \{
*/

/**
* Smart I/O driver error codes
*/
typedef enum 
{
    CY_SMARTIO_SUCCESS   = 0x00u,                                    /**< Returned successful */
    CY_SMARTIO_BAD_PARAM = CY_SMARTIO_ID | CY_PDL_STATUS_ERROR | 0x01u, /**< Bad parameter was passed */
    CY_SMARTIO_LOCKED   = CY_SMARTIO_ID | CY_PDL_STATUS_ERROR | 0x02u,  /**< Smart I/O is not disabled */
} cy_en_smartio_status_t;

/**
* Smart I/O clock selection
*/
typedef enum {
    CY_SMARTIO_CLK_IO0  = 0,    /**< Clock sourced from signal on io0 */
    CY_SMARTIO_CLK_IO1  = 1,    /**< Clock sourced from signal on io1 */
    CY_SMARTIO_CLK_IO2  = 2,    /**< Clock sourced from signal on io2 */
    CY_SMARTIO_CLK_IO3  = 3,    /**< Clock sourced from signal on io3 */
    CY_SMARTIO_CLK_IO4  = 4,    /**< Clock sourced from signal on io4 */
    CY_SMARTIO_CLK_IO5  = 5,    /**< Clock sourced from signal on io5 */
    CY_SMARTIO_CLK_IO6  = 6,    /**< Clock sourced from signal on io6 */
    CY_SMARTIO_CLK_IO7  = 7,    /**< Clock sourced from signal on io7 */
    CY_SMARTIO_CLK_CHIP0  = 8,  /**< Clock sourced from signal on chip0 */
    CY_SMARTIO_CLK_CHIP1  = 9,  /**< Clock sourced from signal on chip1 */
    CY_SMARTIO_CLK_CHIP2  = 10, /**< Clock sourced from signal on chip2 */
    CY_SMARTIO_CLK_CHIP3  = 11, /**< Clock sourced from signal on chip3 */
    CY_SMARTIO_CLK_CHIP4  = 12, /**< Clock sourced from signal on chip4 */
    CY_SMARTIO_CLK_CHIP5  = 13, /**< Clock sourced from signal on chip5 */
    CY_SMARTIO_CLK_CHIP6  = 14, /**< Clock sourced from signal on chip6 */
    CY_SMARTIO_CLK_CHIP7  = 15, /**< Clock sourced from signal on chip7 */
    CY_SMARTIO_CLK_DIVACT = 16, /**< Clock sourced from a peripheral clock divider (Active) */
    CY_SMARTIO_CLK_DIVDS  = 17, /**< Clock sourced from a peripheral clock divider (Deep-Sleep) */
    CY_SMARTIO_CLK_DIVHIB = 18, /**< Clock sourced from a peripheral clock divider (Hibernate) */
    CY_SMARTIO_CLK_LFCLK  = 19, /**< Clock sourced from LFCLK */
    CY_SMARTIO_CLK_GATED  = 20, /**< Disables the clock. Used when turning off the block */
    CY_SMARTIO_CLK_ASYNC  = 31, /**< Asynchronous operation (only allow combinatorial logic) */
}cy_en_smartio_clksrc_t;

/**
* Smart I/O LUT number
*/
typedef enum {
    CY_SMARTIO_LUT0 = 0,        /**< Look-Up-Table #0 */
    CY_SMARTIO_LUT1 = 1,        /**< Look-Up-Table #1 */
    CY_SMARTIO_LUT2 = 2,        /**< Look-Up-Table #2 */
    CY_SMARTIO_LUT3 = 3,        /**< Look-Up-Table #3 */
    CY_SMARTIO_LUT4 = 4,        /**< Look-Up-Table #4 */
    CY_SMARTIO_LUT5 = 5,        /**< Look-Up-Table #5 */
    CY_SMARTIO_LUT6 = 6,        /**< Look-Up-Table #6 */
    CY_SMARTIO_LUT7 = 7,        /**< Look-Up-Table #7 */
}cy_en_smartio_lutnum_t;

/**
* Smart I/O input trigger number
*/
typedef enum {
    CY_SMARTIO_TR0 = 0,         /**< Input trigger #0 */
    CY_SMARTIO_TR1 = 1,         /**< Input trigger #1 */
    CY_SMARTIO_TR2 = 2,         /**< Input trigger #2 */
}cy_en_smartio_trnum_t;

/**
* Smart I/O Data Unit's input "DATA" number
*/
typedef enum {
    CY_SMARTIO_DATA0 = 0,       /**< Input DATA #0 */
    CY_SMARTIO_DATA1 = 1,       /**< Input DATA #1 */
}cy_en_smartio_datanum_t;

/**
* Smart I/O LUT input trigger source
*/
typedef enum {
    CY_SMARTIO_LUTTR_DU_OUT   = 0,  /**< Data Unit output */
    CY_SMARTIO_LUTTR_LUT0_OUT = 0,  /**< LUT0 output */
    CY_SMARTIO_LUTTR_LUT1_OUT = 1,  /**< LUT1 output */
    CY_SMARTIO_LUTTR_LUT2_OUT = 2,  /**< LUT2 output */
    CY_SMARTIO_LUTTR_LUT3_OUT = 3,  /**< LUT3 output */
    CY_SMARTIO_LUTTR_LUT4_OUT = 4,  /**< LUT4 output */
    CY_SMARTIO_LUTTR_LUT5_OUT = 5,  /**< LUT5 output */
    CY_SMARTIO_LUTTR_LUT6_OUT = 6,  /**< LUT6 output */
    CY_SMARTIO_LUTTR_LUT7_OUT = 7,  /**< LUT7 output */
    CY_SMARTIO_LUTTR_CHIP0    = 8,  /**< Chip signal 0 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_CHIP4    = 8,  /**< Chip signal 4 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_CHIP1    = 9,  /**< Chip signal 1 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_CHIP5    = 9,  /**< Chip signal 5 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_CHIP2    = 10, /**< Chip signal 2 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_CHIP6    = 10, /**< Chip signal 6 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_CHIP3    = 11, /**< Chip signal 3 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_CHIP7    = 11, /**< Chip signal 7 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_IO0      = 12, /**< I/O signal 0 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_IO4      = 12, /**< I/O signal 4 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_IO1      = 13, /**< I/O signal 1 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_IO5      = 13, /**< I/O signal 5 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_IO2      = 14, /**< I/O signal 2 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_IO6      = 14, /**< I/O signal 6 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_IO3      = 15, /**< I/O signal 3 (for LUT 0,1,2,3) */
    CY_SMARTIO_LUTTR_IO7      = 15, /**< I/O signal 7 (for LUT 4,5,6,7) */
    CY_SMARTIO_LUTTR_INVALID  = 255, /**< Invalid input trigger selecttion */
}cy_en_smartio_luttr_t;

/**
* Smart I/O LUT opcode
*/
typedef enum {
    CY_SMARTIO_LUTOPC_COMB      = 0,    /**< Combinatorial output */
    CY_SMARTIO_LUTOPC_GATED_TR2 = 1,    /**< TR2 gated, Combinatorial output */
    CY_SMARTIO_LUTOPC_GATED_OUT = 2,    /**< Sequential (gated) output */
    CY_SMARTIO_LUTOPC_ASYNC_SR  = 3,    /**< Asynchronous Set/Reset mode */
}cy_en_smartio_lutopc_t;

/**
* Smart I/O Data Unit input trigger source
*/
typedef enum {
    CY_SMARTIO_DUTR_ZERO     = 0,   /**< Constant 0 */
    CY_SMARTIO_DUTR_ONE      = 1,   /**< Constant 1 */
    CY_SMARTIO_DUTR_DU_OUT   = 2,   /**< Data Unit output */
    CY_SMARTIO_DUTR_LUT0_OUT = 3,   /**< LUT0 output */
    CY_SMARTIO_DUTR_LUT1_OUT = 4,   /**< LUT1 output */
    CY_SMARTIO_DUTR_LUT2_OUT = 5,   /**< LUT2 output */
    CY_SMARTIO_DUTR_LUT3_OUT = 6,   /**< LUT3 output */
    CY_SMARTIO_DUTR_LUT4_OUT = 7,   /**< LUT4 output */
    CY_SMARTIO_DUTR_LUT5_OUT = 8,   /**< LUT5 output */
    CY_SMARTIO_DUTR_LUT6_OUT = 9,   /**< LUT6 output */
    CY_SMARTIO_DUTR_LUT7_OUT = 10,  /**< LUT7 output */
    CY_SMARTIO_DUTR_INVALID  = 255, /**< Invalid input trigger selected */
}cy_en_smartio_dutr_t;

/**
* Smart I/O Data Unit input "DATA" source
*/
typedef enum {
    CY_SMARTIO_DUDATA_ZERO    = 0,  /**< Constant 0 */
    CY_SMARTIO_DUDATA_CHIP    = 1,  /**< Chip signal [7:0] */
    CY_SMARTIO_DUDATA_IO      = 2,  /**< I/O signal [7:0] */
    CY_SMARTIO_DUDATA_DATAREG = 3,  /**< SMARTIO.DATA register */
}cy_en_smartio_dudata_t;

/**
* Smart I/O Data Unit opcode
*/
typedef enum {
    CY_SMARTIO_DUOPC_INCR = 1,              /**< Increment (Count up) */
    CY_SMARTIO_DUOPC_DECR = 2,              /**< Decrement (Count down) */
    CY_SMARTIO_DUOPC_INCR_WRAP = 3,         /**< Increment and wrap-around (Count up and wrap) */
    CY_SMARTIO_DUOPC_DECR_WRAP = 4,         /**< Decrement and wrap-around (Count down and wrap) */
    CY_SMARTIO_DUOPC_INCR_DECR = 5,         /**< Increment or decrement (Count up/down) */
    CY_SMARTIO_DUOPC_INCR_DECR_WRAP = 6,    /**< Increment or decrement with wrap-around (Count up/down and wrap) */
    CY_SMARTIO_DUOPC_ROR = 7,               /**< Rotate right (shift right and wrap) */
    CY_SMARTIO_DUOPC_SHR = 8,               /**< Shift right and shift data in through MSB */
    CY_SMARTIO_DUOPC_AND_OR = 9,            /**< out = ((DATA0 & DATA1) == 0) ? 0 : 1 */
    CY_SMARTIO_DUOPC_SHR_MAJ3 = 10,         /**< Majority 3 (Check if 2 out of 3 LSB bits are logic 1) */
    CY_SMARTIO_DUOPC_SHR_EQL = 11,          /**< Check for equality against DATA1. Also perform Shift right */
}cy_en_smartio_duopc_t;

/**
* Smart I/O Data Unit operation bit size
*/
typedef enum {
    CY_SMARTIO_DUSIZE_1 = 0,    /**< 1-bit size/width operand */
    CY_SMARTIO_DUSIZE_2 = 1,    /**< 2-bits size/width operand */
    CY_SMARTIO_DUSIZE_3 = 2,    /**< 3-bits size/width operand */
    CY_SMARTIO_DUSIZE_4 = 3,    /**< 4-bits size/width operand */
    CY_SMARTIO_DUSIZE_5 = 4,    /**< 5-bits size/width operand */
    CY_SMARTIO_DUSIZE_6 = 5,    /**< 6-bits size/width operand */
    CY_SMARTIO_DUSIZE_7 = 6,    /**< 7-bits size/width operand */
    CY_SMARTIO_DUSIZE_8 = 7,    /**< 8-bits size/width operand */
}cy_en_smartio_dusize_t;

/** \} group_smartio_enums */


/***************************************
*       Configuration Structures
***************************************/

/**
* \addtogroup group_smartio_data_structures
* \{
*/

/** LUT configuration structure */
typedef struct {
    cy_en_smartio_luttr_t tr0;      /**< LUT input trigger 0 source selection */
    cy_en_smartio_luttr_t tr1;      /**< LUT input trigger 1 source selection */
    cy_en_smartio_luttr_t tr2;      /**< LUT input trigger 2 source selection */
    cy_en_smartio_lutopc_t opcode;  /**< LUT Opcode */
    uint8_t lutMap;                 /**< LUT output mapping */
}cy_stc_smartio_lutcfg_t;

/** Data Unit (DU) configuration structure */
typedef struct {
    cy_en_smartio_dutr_t tr0;       /**< DU input trigger 0 source selection */
    cy_en_smartio_dutr_t tr1;       /**< DU input trigger 1 source selection */
    cy_en_smartio_dutr_t tr2;       /**< DU input trigger 2 source selection */
    cy_en_smartio_dudata_t data0;   /**< DU input DATA0 source selection */
    cy_en_smartio_dudata_t data1;   /**< DU input DATA1 source selection */
    cy_en_smartio_duopc_t opcode;   /**< DU op-code */
    cy_en_smartio_dusize_t size;    /**< DU operation bit size */
    uint8_t dataReg;                /**< DU DATA register value */
}cy_stc_smartio_ducfg_t;

/** Smart I/O configuration structure */
typedef struct {
    cy_en_smartio_clksrc_t clkSrc;          /**< Smart I/O instance clock source */
    uint8_t bypassMask;                     /**< Smart I/O io<->chip channel bypass mask */
    uint8_t ioSyncEn;                       /**< Syncronization enable/disable for I/O terminals */
    uint8_t chipSyncEn;                     /**< Syncronization enable/disable for Chip terminals */
    const cy_stc_smartio_lutcfg_t* lutCfg0; /**< Pointer to LUT0 configuration (NULL if not used) */
    const cy_stc_smartio_lutcfg_t* lutCfg1; /**< Pointer to LUT1 configuration (NULL if not used) */
    const cy_stc_smartio_lutcfg_t* lutCfg2; /**< Pointer to LUT2 configuration (NULL if not used) */
    const cy_stc_smartio_lutcfg_t* lutCfg3; /**< Pointer to LUT3 configuration (NULL if not used) */
    const cy_stc_smartio_lutcfg_t* lutCfg4; /**< Pointer to LUT4 configuration (NULL if not used) */
    const cy_stc_smartio_lutcfg_t* lutCfg5; /**< Pointer to LUT5 configuration (NULL if not used) */
    const cy_stc_smartio_lutcfg_t* lutCfg6; /**< Pointer to LUT6 configuration (NULL if not used) */
    const cy_stc_smartio_lutcfg_t* lutCfg7; /**< Pointer to LUT7 configuration (NULL if not used) */
    const cy_stc_smartio_ducfg_t* duCfg;    /**< Pointer to Data Unit configuration (NULL if not used) */
    bool hldOvr;                            /**< Hold override enable (true) / disable (false) */
}cy_stc_smartio_config_t;

/** \} group_smartio_data_structures */


/***************************************
*        Function Prototypes
***************************************/

/**
* \addtogroup group_smartio_functions
* \{
*/

/**
* \addtogroup group_smartio_functions_init
* \{
*/
cy_en_smartio_status_t Cy_SmartIO_Init(SMARTIO_PRT_Type* base, const cy_stc_smartio_config_t* config);
void Cy_SmartIO_Deinit(SMARTIO_PRT_Type* base);
void Cy_SmartIO_Enable(SMARTIO_PRT_Type* base);
void Cy_SmartIO_Disable(SMARTIO_PRT_Type* base);
/** \} group_smartio_functions_init */

/**
* \addtogroup group_smartio_functions_general
* \{
*/
__STATIC_INLINE uint8_t Cy_SmartIO_GetChBypass(SMARTIO_PRT_Type* base);
cy_en_smartio_status_t Cy_SmartIO_SetChBypass(SMARTIO_PRT_Type* base, uint8_t bypassMask);
__STATIC_INLINE cy_en_smartio_clksrc_t Cy_SmartIO_GetClock(SMARTIO_PRT_Type* base);
cy_en_smartio_status_t Cy_SmartIO_SetClock(SMARTIO_PRT_Type* base, cy_en_smartio_clksrc_t clkSrc);
__STATIC_INLINE uint8_t Cy_SmartIO_GetIoSync(SMARTIO_PRT_Type* base);
cy_en_smartio_status_t Cy_SmartIO_SetIoSync(SMARTIO_PRT_Type* base, uint8_t ioSyncEn);
__STATIC_INLINE uint8_t Cy_SmartIO_GetChipSync(SMARTIO_PRT_Type* base);
cy_en_smartio_status_t Cy_SmartIO_SetChipSync(SMARTIO_PRT_Type* base, uint8_t chipSyncEn);
cy_en_smartio_status_t Cy_SmartIO_HoldOverride(SMARTIO_PRT_Type* base, bool hldOvr);
/** \} group_smartio_functions_general */

/**
* \addtogroup group_smartio_functions_lut
* \{
*/
cy_en_smartio_luttr_t Cy_SmartIO_GetLutTr(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_trnum_t trNum);
cy_en_smartio_status_t Cy_SmartIO_SetLutTr(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_trnum_t trNum, cy_en_smartio_luttr_t trSrc);
cy_en_smartio_status_t Cy_SmartIO_SetLutTrAll(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_luttr_t trSrc);
__STATIC_INLINE cy_en_smartio_lutopc_t Cy_SmartIO_GetLutOpcode(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum);
cy_en_smartio_status_t Cy_SmartIO_SetLutOpcode(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, cy_en_smartio_lutopc_t opcode);
__STATIC_INLINE uint8_t Cy_SmartIO_GetLutMap(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum);
cy_en_smartio_status_t Cy_SmartIO_SetLutMap(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum, uint8_t lutMap);
/** \} group_smartio_functions_lut */

/**
* \addtogroup group_smartio_functions_du
* \{
*/
cy_en_smartio_dutr_t Cy_SmartIO_GetDuTr(SMARTIO_PRT_Type* base, cy_en_smartio_trnum_t trNum);
cy_en_smartio_status_t Cy_SmartIO_SetDuTr(SMARTIO_PRT_Type* base, cy_en_smartio_trnum_t trNum, cy_en_smartio_dutr_t trSrc);
cy_en_smartio_status_t Cy_SmartIO_SetDuTrAll(SMARTIO_PRT_Type* base, cy_en_smartio_dutr_t trSrc);
__STATIC_INLINE cy_en_smartio_dudata_t Cy_SmartIO_GetDuData(SMARTIO_PRT_Type* base, cy_en_smartio_datanum_t dataNum);
cy_en_smartio_status_t Cy_SmartIO_SetDuData(SMARTIO_PRT_Type* base, cy_en_smartio_datanum_t dataNum, cy_en_smartio_dudata_t dataSrc);
__STATIC_INLINE cy_en_smartio_duopc_t Cy_SmartIO_GetDuOpc(SMARTIO_PRT_Type* base);
__STATIC_INLINE cy_en_smartio_dusize_t Cy_SmartIO_GetDuSize(SMARTIO_PRT_Type* base);
cy_en_smartio_status_t Cy_SmartIO_SetDuOperation(SMARTIO_PRT_Type* base, cy_en_smartio_duopc_t opcode, cy_en_smartio_dusize_t size);
__STATIC_INLINE uint8_t Cy_SmartIO_GetDataReg(SMARTIO_PRT_Type* base);
cy_en_smartio_status_t Cy_SmartIO_SetDataReg(SMARTIO_PRT_Type* base, uint8_t dataReg);
/** \} group_smartio_functions_du */


/***************************************
*           Functions
***************************************/

/*******************************************************************************
* Function Name: Cy_SmartIO_GetChBypass
****************************************************************************//**
*
* \brief Gets the bypass/route state of all 8 channels in the Smart I/O
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
* \return
* Bypass/Route state of 8 io<->chip channels (bits [7:0]): 1=bypass, 0=routed.
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SmartIO_GetChBypass(SMARTIO_PRT_Type* base)
{
    return((uint8_t)_FLD2VAL(SMARTIO_PRT_CTL_BYPASS, SMARTIO_PRT_CTL(base)));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetClock
****************************************************************************//**
*
* \brief Gets the clock source of the Smart I/O.
*
* \param base
* Pointer to the Smart I/O base address
*
* \return
* Clock source
*
*******************************************************************************/
__STATIC_INLINE cy_en_smartio_clksrc_t Cy_SmartIO_GetClock(SMARTIO_PRT_Type* base)
{
    return((cy_en_smartio_clksrc_t)_FLD2VAL(SMARTIO_PRT_CTL_CLOCK_SRC, SMARTIO_PRT_CTL(base)));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetIoSync
****************************************************************************//**
*
* \brief Gets the syncronization mode of the 8 I/O terminals.
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
* \return
* Sync mode of 8 I/O terminals (bits [7:0]): 1=sync, 0=no sync.
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SmartIO_GetIoSync(SMARTIO_PRT_Type* base)
{
    return((uint8_t)_FLD2VAL(SMARTIO_PRT_SYNC_CTL_IO_SYNC_EN, SMARTIO_PRT_SYNC_CTL(base)));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetChipSync
****************************************************************************//**
*
* \brief Gets the syncronization mode of the 8 chip-side terminals.
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
* \return
* Sync mode of 8 chip-side terminals (bits [7:0]): 1=sync, 0=no sync.
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SmartIO_GetChipSync(SMARTIO_PRT_Type* base)
{
    return((uint8_t)_FLD2VAL(SMARTIO_PRT_SYNC_CTL_CHIP_SYNC_EN, SMARTIO_PRT_SYNC_CTL(base)));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetLutOpcode
****************************************************************************//**
*
* \brief Gets the opcode of the specified LUT.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param lutNum
* LUT index number
*
* \return
* LUT opcode
*
*******************************************************************************/
__STATIC_INLINE cy_en_smartio_lutopc_t Cy_SmartIO_GetLutOpcode(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum)
{
    return((cy_en_smartio_lutopc_t)(_FLD2VAL(SMARTIO_PRT_LUT_CTL_LUT_OPC, SMARTIO_PRT_LUT_CTL(base, lutNum))));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetLutMap
****************************************************************************//**
*
* \brief Gets the 3:1 truth table of the specified LUT.
*
* <table class="doxtable">
*   <tr><th>tr2</th><th>tr1</th><th>tr0</th><th>lutNum</th></tr>
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
* \return
* Bitfield [7:0] mapping of the 3:1 LUT
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SmartIO_GetLutMap(SMARTIO_PRT_Type* base, cy_en_smartio_lutnum_t lutNum)
{
   return((uint8_t)(_FLD2VAL(SMARTIO_PRT_LUT_CTL_LUT, SMARTIO_PRT_LUT_CTL(base, lutNum))));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetDuData
****************************************************************************//**
*
* \brief Gets the data unit's input "data" source.
*
* \param base
* Pointer to the Smart I/O base address
*
* \param dataNum
* Input data number
*
* \return
* Data unit input trigger source
*
*******************************************************************************/
__STATIC_INLINE cy_en_smartio_dudata_t Cy_SmartIO_GetDuData(SMARTIO_PRT_Type* base, cy_en_smartio_datanum_t dataNum)
{
    return ((dataNum == CY_SMARTIO_DATA0) ? 
                (cy_en_smartio_dudata_t)_FLD2VAL(SMARTIO_PRT_DU_SEL_DU_DATA0_SEL, SMARTIO_PRT_DU_SEL(base)) : 
                (cy_en_smartio_dudata_t)_FLD2VAL(SMARTIO_PRT_DU_SEL_DU_DATA1_SEL, SMARTIO_PRT_DU_SEL(base)));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetDuOpc
****************************************************************************//**
*
* \brief Gets the data unit's opcode.
*
* \param base
* Pointer to the Smart I/O base address
*
* \return
* Data unit opcode
*
*******************************************************************************/
__STATIC_INLINE cy_en_smartio_duopc_t Cy_SmartIO_GetDuOpc(SMARTIO_PRT_Type* base)
{
    return ((cy_en_smartio_duopc_t)_FLD2VAL(SMARTIO_PRT_DU_CTL_DU_OPC, SMARTIO_PRT_DU_CTL(base)));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetDuSize
****************************************************************************//**
*
* \brief Gets the data unit's opcode operand bit-width.
*
* \param base
* Pointer to the Smart I/O base address
*
* \return
* Data unit operand bit-width
*
*******************************************************************************/
__STATIC_INLINE cy_en_smartio_dusize_t Cy_SmartIO_GetDuSize(SMARTIO_PRT_Type* base)
{
    return ((cy_en_smartio_dusize_t)_FLD2VAL(SMARTIO_PRT_DU_CTL_DU_SIZE, SMARTIO_PRT_DU_CTL(base)));
}


/*******************************************************************************
* Function Name: Cy_SmartIO_GetDataReg
****************************************************************************//**
*
* \brief Gets the data unit's DATA register value.
*
* \param base
* Pointer to the Smart I/O base address
*
* \return
* DATA register value
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_SmartIO_GetDataReg(SMARTIO_PRT_Type* base)
{
    return ((uint8_t)(SMARTIO_PRT_DATA(base)));
}


/** \} group_smartio_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_SMARTIO_H */

/** \} group_smartio */

/* [] END OF FILE */
