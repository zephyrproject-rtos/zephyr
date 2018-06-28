/***************************************************************************//**
* \file cy_smif.h
* \version 1.20
*
* Provides an API declaration of the Cypress SMIF driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_smif Serial Memory Interface (SMIF)
* \{
* The SPI-based communication interface for external memory devices.
*
* SMIF: Serial Memory Interface: This IP block implements an SPI-based
* communication interface for interfacing external memory devices to PSoC. The SMIF
* supports Octal-SPI, Dual Quad-SPI, Quad-SPI, DSPI, and SPI.
*
* Features
*   - Standard SPI Master interface
*   - Supports Single/Dual/Quad/Octal SPI Memories
*   - Supports Dual-Quad SPI mode
*   - Design-time configurable support for multiple (up to 4) external serial
*   memory devices
*   - eXecute-In-Place (XIP) operation mode for both read and write accesses
*   with 4KB XIP read cache and on-the-fly encryption and decryption
*   - Supports external serial memory initialization via Serial Flash
*   Discoverable Parameters (SFDP) standard
*   - Support for SPI clock frequencies up to 80 MHz
*
*
* The primary usage model for the SMIF is that of an external memory interface.
* The SMIF is capable of interfacing with different types of memory, up to four
* types.
*
* \b SMIF driver is divided in next layers
*   - cy_smif.h API
*   - cy_smif_memslot.h API
*   - SMIF configuration structures
*
* The SMIF API is divided into the low-level functions and memory-slot functions. Use
* the low level API for the SMIF block initialization and for implementing a generic 
* SPI communication interface using the SMIF block.
*
* The memory slot API has functions to implement the basic memory operations such as 
* program, read, erase etc. These functions are implemented using the memory 
* parameters in the memory device configuration data structure. The memory-slot
* initialization API initializes all the memory slots based on the settings in the
* array.
*
* \image html smif_1_0_p01_layers.png
*
* SMIF Configuration Tool is a stand-alone application, which is a part of PDL
* and could be found in \<PDL_DIR\>/tools/\<OS_DIR\>/SMIFConfigurationTool
* (e.g. for PDL 3.0.0 and Windows OS PDL/3.0.0/tools/win/SMIFConfigurationTool). Tool
* generates *.c and *.h file with configuration structures. These configuration
* structures are input parameters for cy_smif_memslot API level
*
* \warning The driver is not responsible for external memory persistence. You cannot edit
* a buffer during the Read/Write operations. If there is a memory error, the SMIF ip block 
* can require a reset. To determine if this has happened, check the SMIF 
* busy status using Cy_SMIF_BusyCheck() and implement a timeout. Reset the SMIF 
* block by toggling CTL.ENABLED. Then reconfigure the SMIF block.
*
* For the Write operation, check that the SMIF driver has completed 
* transferring by calling Cy_SMIF_BusyCheck(). Also, check that the memory is 
* available with Cy_SMIF_Memslot_IsBusy() before proceeding. 
*
* Simple example of external flash memory programming using low level SMIF API.
* All steps mentioned in example below are incorporated in
* \ref Cy_SMIF_Memslot_CmdWriteEnable(), \ref Cy_SMIF_Memslot_CmdProgram(), and
* \ref Cy_SMIF_Memslot_IsBusy() of the
* \ref group_smif_mem_slot_functions "memory slot level API".
* \warning Example is simplified, without checks of error conditions.
* \note Flash memories need erase operation before programming. Refer to
* external memory datasheet for specific memory commands.
*
* \snippet smif/smif_sut_01.cydsn/main_cm4.c SMIF_API: Write example
*
* For the Read operation, before accessing the read buffer, check that it is ready
* by calling Cy_SMIF_GetTxFifoStatus().
*
* Simple example of external flash memory read using low level SMIF API. All
* steps mentioned in example below are incorporated in
* \ref Cy_SMIF_Memslot_CmdRead() of the
* \ref group_smif_mem_slot_functions "memory slot level API".
* \warning Example is simplified, without checks of error conditions.
* \note Refer to external memory datasheet for specific memory commands.
*
* \snippet smif/smif_sut_01.cydsn/main_cm4.c SMIF_API: Read example
*
* The user should invalidate the cache by calling Cy_SMIF_CacheInvalidate() when 
* switching from the MMIO mode to XIP mode.
*
* \section group_smif_configuration Configuration Considerations
*
* PDL API has common parameters: base, context, config described in
* \ref page_getting_started_pdl_design "PDL Design" section.
*
* See the documentation for Cy_SMIF_Init() and Cy_SMIF_Memslot_Init() for details
* on the required configuration structures and other initialization topics. 
*
* The normal (MMIO) mode is used for implementing a generic SPI/DSPI/QSPI/Dual
* Quad-SPI/Octal-SPI communication interface using the SMIF block. This
* interface can be used to implement special commands like Program/Erase of
* flash, memory device configuration, sleep mode entry for memory devices or
* other special commands specific to the memory device. The transfer width
* (SPI/DSP/Quad-SPI/Octal-SPI) of a transmission is a parameter set for each
* transmit/receive operation. So these can be changed at run time.
*
* In a typical memory interface with flash memory, the SMIF is used in the
* memory mode when reading from the memory and it switches to the normal mode when
* writing to flash memory.
* A typical memory device has multiple types of commands.
*
* The SMIF interface can be used to transmit different types of commands. Each
* command has different phases: command, dummy cycles, and transmit and receive
* data which require separate APIs.
*
* \subsection group_smif_init SMIF Initialization
* Create interrupt function and allocate memory for SMIF context
* structure
* \snippet smif/smif_sut_01.cydsn/main_cm4.c SMIF_INIT: context and interrupt
* SMIF driver initialization for low level API usage (cysmif.h)
* \snippet smif/smif_sut_01.cydsn/main_cm4.c SMIF_INIT: low level
* Additional steps to initialize SMIF driver for memory slot level API usage
* (cy_smif_memslot.h).
* \snippet smif/smif_sut_01.cydsn/main_cm4.c SMIF_INIT: memslot level
* \note Example does not include initialization of all needed configuration
* structures (\ref cy_stc_smif_mem_device_cfg_t, \ref cy_stc_smif_mem_cmd_t).
* SMIF Configuration tool generates all configuration structures needed for
* memslot level API usage.
*
* \subsection group_smif_xip_init SMIF XIP Initialization
* The eXecute In Place (XIP) is a mode of operation where read or write commands 
* to the memory device are directed through the SMIF without any use of API 
* function calls. In this mode the SMIF block maps the AHB bus-accesses to 
* external memory device addresses to make it behave similar to internal memory. 
* This allows the CPU to execute code directly from external memory. This mode 
* is not limited to code and is suitable also for data read and write accesses. 
* \snippet smif/smif_sut_01.cydsn/main_cm4.c SMIF_INIT: XIP
* \note Example of input parameters initialization is in \ref group_smif_init 
* section.
* \warning Functions that called from external memory should be declared with 
* long call attribute.     
*
* \section group_smif_more_information More Information
*
* More information regarding the Serial Memory Interface can be found in the component 
* datasheet and the Technical Reference Manual (TRM).
* More information regarding the SMIF Configuration Tool are in SMIF
* Configuration Tool User Guide located in \<PDL_DIR\>/tools/\<OS_DIR\>/SMIFConfigurationTool/
* folder
*
* \section group_smif_MISRA MISRA-C Compliance]
* <table class="doxtable">
*   <tr>
*     <th>MISRA rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>11.4</td>
*     <td>A</td>
*     <td>The cast is be performed between a pointer to the object type and a different pointer to the object type.</td>
*     <td>The cast from the pointer to void to the pointer to an unsigned integer does not have any unintended effect, as
*         it is a consequence of the definition of a structure based on hardware registers.</td>
*   </tr>
*   <tr>
*     <td>11.5</td>
*     <td>R</td>
*     <td>Not performed, the cast that removes any const or volatile qualification from the type addressed by a pointer.</td>
*     <td>The removal of the volatile qualification inside the function has no side effects.</td>
*   </tr>
*   <tr>
*     <td>14.2</td>
*     <td>R</td>
*     <td>All non-null statements will either:
*       a) have at least one-side effect however executed, or
*       b) cause control flow to change</td>
*     <td>The readback of the register is required by the hardware.</td>
*   </tr>
* </table>
*
* \section group_smif_changelog Changelog
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
*     <td>Fix write to external memory from CM0+ core. Add checks of API input parameters. 
*         Minor documentation updates</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_smif_macros Macros
* \{
* \defgroup group_smif_macros_status Status Macros
* \defgroup group_smif_macros_cmd Command Macros
* \defgroup group_smif_macros_flags External Memory Flags
* \defgroup group_smif_macros_sfdp SFDP Macros
* \defgroup group_smif_macros_isr Interrupt Macros
* \}
* \defgroup group_smif_functions Functions
* \{
* \defgroup group_smif_low_level_functions Low Level Functions
* \{
* Basic flow for read/write commands using \ref Cy_SMIF_TransmitCommand
* \ref Cy_SMIF_TransmitData \ref Cy_SMIF_ReceiveData
* \ref Cy_SMIF_SendDummyCycles
*
*  \image html smif_1_0_p03_rw_cmd.png
*
* \}
* \defgroup group_smif_mem_slot_functions Memory Slot Functions
* \defgroup group_smif_functions_syspm_callback  Low Power Callback
* \}
* \defgroup group_smif_data_structures Data Structures
* \{
* \defgroup group_smif_data_structures_memslot SMIF Memory Description Structures
* General hierarchy of memory structures are:
* \image html smif_1_0_p02_memslot_stc.png
* Top structure is \ref cy_stc_smif_block_config_t, which could have links up to
* 4 \ref cy_stc_smif_mem_config_t which describes each connected to the SMIF
* external memory.
* \}
* \defgroup group_smif_enums Enumerated Types
*/

#if !defined(CY_SMIF_H)
#define CY_SMIF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cyip_smif.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_syspm.h"


#ifdef CY_IP_MXSMIF

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************
*            Constants
****************************************/

/**
* \addtogroup group_smif_macros
* \{
*/

/** The driver major version */
#define CY_SMIF_DRV_VERSION_MAJOR       1

/** The driver minor version */
#define CY_SMIF_DRV_VERSION_MINOR       20

/** One microsecond timeout for Cy_SMIF_TimeoutRun() */
#define CY_SMIF_WAIT_1_UNIT        (1U)

/** The SMIF driver ID, reported as part of an unsuccessful API return status
 * \ref cy_en_smif_status_t */
#define CY_SMIF_ID CY_PDL_DRV_ID(0x2CU)


/**
* \addtogroup group_smif_macros_isr
* \{
*/

/** Enable XIP_ALIGNMENT_ERROR interrupt see TRM for details */
#define CY_SMIF_ALIGNMENT_ERROR                 (SMIF_INTR_XIP_ALIGNMENT_ERROR_Msk)
/** Enable RX_DATA_FIFO_UNDERFLOW interrupt see TRM for details */
#define CY_SMIF_RX_DATA_FIFO_UNDERFLOW          (SMIF_INTR_RX_DATA_FIFO_UNDERFLOW_Msk)
/** Enable TX_DATA_FIFO_OVERFLOW interrupt see TRM for details */
#define CY_SMIF_TX_DATA_FIFO_OVERFLOW           (SMIF_INTR_TX_DATA_FIFO_OVERFLOW_Msk)
/** Enable TX_CMD_FIFO_OVERFLOW interrupt see TRM for details */
#define CY_SMIF_TX_COMMAND_FIFO_OVERFLOW         (SMIF_INTR_TX_CMD_FIFO_OVERFLOW_Msk)
/** Enable TR_TX_REQ interrupt see TRM for details */
#define CY_SMIF_TX_DATA_FIFO_LEVEL_TRIGGER      (SMIF_INTR_TR_TX_REQ_Msk)
/** Enable TR_RX_REQ interrupt see TRM for details */
#define CY_SMIF_RX_DATA_FIFO_LEVEL_TRIGGER      (SMIF_INTR_TR_RX_REQ_Msk)

/** \} group_smif_macros_isr */

/** \cond INTERNAL */

#define CY_SMIF_CMD_FIFO_TX_MODE           (0UL)
#define CY_SMIF_CMD_FIFO_TX_COUNT_MODE     (1UL)
#define CY_SMIF_CMD_FIFO_RX_COUNT_MODE     (2UL)
#define CY_SMIF_CMD_FIFO_DUMMY_COUNT_MODE  (3UL)

#define CY_SMIF_TX_CMD_FIFO_STATUS_RANGE   (4U)
#define CY_SMIF_TX_DATA_FIFO_STATUS_RANGE  (8U)
#define CY_SMIF_RX_DATA_FIFO_STATUS_RANGE  (8U)

#define CY_SMIF_ONE_BYTE                   (1U)
#define CY_SMIF_TWO_BYTES                  (2U)
#define CY_SMIF_THREE_BYTES                (3U)
#define CY_SMIF_FOUR_BYTES                 (4U)
#define CY_SMIF_FIVE_BYTES                 (5U)
#define CY_SMIF_SIX_BYTES                  (6U)
#define CY_SMIF_SEVEN_BYTES                (7U)
#define CY_SMIF_EIGHT_BYTES                (8U)

#define CY_SMIF_CRYPTO_FIRST_WORD          (0U)
#define CY_SMIF_CRYPTO_SECOND_WORD         (4U)
#define CY_SMIF_CRYPTO_THIRD_WORD          (8U)
#define CY_SMIF_CRYPTO_FOURTH_WORD         (12U)

#define CY_SMIF_CRYPTO_START               (1UL)
#define CY_SMIF_CRYPTO_COMPLETED           (0UL)
#define CY_SMIF_CRYPTO_ADDR_MASK           (0xFFFFFFF0UL)
#define CY_SMIF_AES128_BYTES               (16U)

#define CY_SMIF_CTL_REG_DEFAULT  (0x00000300U) /* 3 - [13:12] CLOCK_IF_RX_SEL  */

#define CY_SMIF_SFDP_FAIL           (0x08U)
#define CY_SMIF_SFDP_FAIL_SS0_POS   (0x00U)
#define CY_SMIF_SFDP_FAIL_SS1_POS   (0x01U)
#define CY_SMIF_SFDP_FAIL_SS2_POS   (0x02U)
#define CY_SMIF_SFDP_FAIL_SS3_POS   (0x03U)

#define CY_SMIF_MAX_DESELECT_DELAY      (7U)
#define CY_SMIF_MAX_TX_TR_LEVEL         (8U)
#define CY_SMIF_MAX_RX_TR_LEVEL         (8U)

#define CY_SMIF_MODE_VALID(mode)    ((CY_SMIF_NORMAL == (cy_en_smif_mode_t)(mode)) || \
                                     (CY_SMIF_MEMORY == (cy_en_smif_mode_t)(mode)))
#define CY_SMIF_BLOCK_EVENT_VALID(event)    ((CY_SMIF_BUS_ERROR == (cy_en_smif_error_event_t)(event)) || \
                                             (CY_SMIF_WAIT_STATES == (cy_en_smif_error_event_t)(event)))
#define CY_SMIF_CLOCK_SEL_VALID(clkSel)     ((CY_SMIF_SEL_INTERNAL_CLK == (cy_en_smif_clk_select_t)(clkSel)) || \
                                             (CY_SMIF_SEL_INV_INTERNAL_CLK == (cy_en_smif_clk_select_t)(clkSel)) || \
                                             (CY_SMIF_SEL_FEEDBACK_CLK == (cy_en_smif_clk_select_t)(clkSel)) || \
                                             (CY_SMIF_SEL_INV_FEEDBACK_CLK == (cy_en_smif_clk_select_t)(clkSel)))
                                             
#define CY_SMIF_DESELECT_DELAY_VALID(delay)     ((delay) <= CY_SMIF_MAX_DESELECT_DELAY)
#define CY_SMIF_SLAVE_SEL_VALID(ss)     ((CY_SMIF_SLAVE_SELECT_0 == (ss)) || \
                                         (CY_SMIF_SLAVE_SELECT_1 == (ss)) || \
                                         (CY_SMIF_SLAVE_SELECT_2 == (ss)) || \
                                         (CY_SMIF_SLAVE_SELECT_3 == (ss)))
#define CY_SMIF_DATA_SEL_VALID(ss)     ((CY_SMIF_DATA_SEL0 == (ss)) || \
                                         (CY_SMIF_DATA_SEL1 == (ss)) || \
                                         (CY_SMIF_DATA_SEL2 == (ss)) || \
                                         (CY_SMIF_DATA_SEL3 == (ss)))
#define CY_SMIF_TXFR_WIDTH_VALID(width)     ((CY_SMIF_WIDTH_SINGLE == (width)) || \
                                            (CY_SMIF_WIDTH_DUAL == (width)) || \
                                            (CY_SMIF_WIDTH_QUAD == (width)) || \
                                            (CY_SMIF_WIDTH_OCTAL == (width)))
#define CY_SMIF_CMD_PARAM_VALID(param, paramSize)     (((paramSize) > 0U)? (NULL != (param)) : (true))

/***************************************
*        Command FIFO Register
***************************************/

/* SMIF->TX_CMD_FIFO_WR  */
#define CY_SMIF_TX_CMD_FIFO_WR_MODE_POS         (18U)   /* [19:18]  Command data mode */
#define CY_SMIF_TX_CMD_FIFO_WR_WIDTH_POS        (16U)   /* [17:16]  Transfer width    */
#define CY_SMIF_TX_CMD_FIFO_WR_LAST_BYTE_POS    (15U)   /* [15]     Last byte      */
#define CY_SMIF_TX_CMD_FIFO_WR_SS_POS           (8U)    /* [11:8]   Slave select      */
#define CY_SMIF_TX_CMD_FIFO_WR_TXDATA_POS       (0U)    /* [0]      Transmitted byte  */
#define CY_SMIF_TX_CMD_FIFO_WR_DUMMY_POS        (0U)    /* [0]      Dummy count       */
#define CY_SMIF_TX_CMD_FIFO_WR_TX_COUNT_POS     (0U)    /* [0]      TX count          */
#define CY_SMIF_TX_CMD_FIFO_WR_RX_COUNT_POS     (0U)    /* [0]      RX count          */

/* SMIF->TX_CMD_FIFO_WR Commands Fields */
#define CY_SMIF_CMD_FIFO_WR_MODE_Pos            (18UL)           /* [19:18]         Command data mode */
#define CY_SMIF_CMD_FIFO_WR_MODE_Msk            (0x000C0000UL)   /* DATA[19:18]       Command data mode    */

#define CY_SMIF_CMD_FIFO_WR_WIDTH_Pos           (16UL)           /* [17:16]         Transfer width       */
#define CY_SMIF_CMD_FIFO_WR_WIDTH_Msk           (0x00030000UL)   /* DATA[17:16]     Transfer width       */

#define CY_SMIF_CMD_FIFO_WR_LAST_BYTE_Pos       (15UL)           /* [15]            Last byte            */
#define CY_SMIF_CMD_FIFO_WR_LAST_BYTE_Msk       (0x00008000UL)   /* DATA[15]        Last byte            */

#define CY_SMIF_CMD_FIFO_WR_SS_Pos              (8UL)            /* [11:8]          Slave select         */
#define CY_SMIF_CMD_FIFO_WR_SS_Msk              (0x00000F00UL)   /* DATA[11:8]      Slave select         */
   
#define CY_SMIF_CMD_FIFO_WR_TXDATA_Pos          (0UL)            /* [0]             Transmitted byte     */
#define CY_SMIF_CMD_FIFO_WR_TXDATA_Msk          (0x000000FFUL)   /* DATA[7:0]         Transmitted byte     */
#define CY_SMIF_CMD_FIFO_WR_DUMMY_Pos           (0UL)            /* [0]             Dummy count          */
#define CY_SMIF_CMD_FIFO_WR_DUMMY_Msk           (0x0000FFFFUL)   /* DATA[15:0]      Dummy count          */
#define CY_SMIF_CMD_FIFO_WR_TX_COUNT_Msk        (0x0000FFFFUL)   /* DATA[15:0]      TX count             */
#define CY_SMIF_CMD_FIFO_WR_TX_COUNT_Pos        (0UL)            /* [0]             TX count             */
#define CY_SMIF_CMD_FIFO_WR_RX_COUNT_Msk        (0x0003FFFFUL)   /* DATA[17:0]      RX count             */
#define CY_SMIF_CMD_FIFO_WR_RX_COUNT_Pos        (0UL)            /* [0]             RX count             */

/** \endcond*/
/** \} group_smif_macros */


/**
* \addtogroup group_smif_enums
* \{
*/

/** The Transfer width options for the command, data, the address and the mode. */
typedef enum
{
    CY_SMIF_WIDTH_SINGLE   = 0U,    /**< Normal SPI mode. */
    CY_SMIF_WIDTH_DUAL     = 1U,    /**< Dual SPI mode. */
    CY_SMIF_WIDTH_QUAD     = 2U,    /**< Quad SPI mode. */
    CY_SMIF_WIDTH_OCTAL    = 3U     /**< Octal SPI mode. */
} cy_en_smif_txfr_width_t;

/** The SMIF error-event selection. */
typedef enum
{
    /**< Generates a bus error. */
    CY_SMIF_BUS_ERROR           = 0UL,
    /** Stalls the bus with the wait states. This option will increase the 
     * interrupt latency. 
     */
    CY_SMIF_WAIT_STATES         = 1UL
} cy_en_smif_error_event_t;

/** The data line-selection options for a slave device. */
typedef enum
{
    /**
    * smif.spi_data[0] = DATA0, smif.spi_data[1] = DATA1, ..., smif.spi_data[7] = DATA7.
    * This value is allowed for the SPI, DSPI, quad-SPI, dual quad-SPI, and octal-SPI modes.
    */
    CY_SMIF_DATA_SEL0      = 0,
    /**
    * smif.spi_data[2] = DATA0, smif.spi_data[3] = DATA1.
    * This value is only allowed for the SPI and DSPI modes.
    */
    CY_SMIF_DATA_SEL1      = 1,
    /**
    * smif.spi_data[4] = DATA0, smif.spi_data[5] = DATA1, ..., smif.spi_data[7] = DATA3.
    * This value is only allowed for the SPI, DSPI, quad-SPI and dual quad-SPI modes.
    */
    CY_SMIF_DATA_SEL2      = 2,
    /**
    * smif.spi_data[6] = DATA0, smif.spi_data[7] = DATA1.
    * This value is only allowed for the SPI and DSPI modes.
    */
    CY_SMIF_DATA_SEL3      = 3
} cy_en_smif_data_select_t;

/** The SMIF modes to work with an external memory. */
typedef enum
{
    CY_SMIF_NORMAL,         /**< Command mode (MMIO mode). */
    CY_SMIF_MEMORY          /**< XIP (eXecute In Place) mode. */
} cy_en_smif_mode_t;

/** The SMIF transfer status return values. */
typedef enum
{
    CY_SMIF_STARTED,       /**< The SMIF started. */
    CY_SMIF_SEND_CMPLT,    /**< The data transmission is complete. */
    CY_SMIF_SEND_BUSY,     /**< The data transmission is in progress. */
    CY_SMIF_REC_CMPLT,     /**< The data reception is completed. */
    CY_SMIF_REC_BUSY,      /**< The data reception is in progress. */
    CY_SMIF_XIP_ERROR,     /**< An XIP alignment error. */
    CY_SMIF_CMD_ERROR,     /**< A TX CMD FIFO overflow. */
    CY_SMIF_TX_ERROR,      /**< A TX DATA FIFO overflow. */
    CY_SMIF_RX_ERROR       /**< An RX DATA FIFO underflow. */

} cy_en_smif_txfr_status_t;

/** The SMIF API return values. */
typedef enum
{
    CY_SMIF_SUCCESS = 0x00U,           /**< Successful SMIF operation. */
    CY_SMIF_CMD_FIFO_FULL   = CY_SMIF_ID |CY_PDL_STATUS_ERROR | 0x01U,     /**< The command is cancelled. The command FIFO is full. */
    CY_SMIF_EXCEED_TIMEOUT  = CY_SMIF_ID |CY_PDL_STATUS_ERROR | 0x02U,    /**< The SMIF operation timeout exceeded. */
    /**
    * The device does not have a QE bit. The device detects
    * 1-1-4 and 1-4-4 Reads based on the instruction.
    */
    CY_SMIF_NO_QE_BIT       = CY_SMIF_ID |CY_PDL_STATUS_ERROR | 0x03U,
    CY_SMIF_BAD_PARAM       = CY_SMIF_ID |CY_PDL_STATUS_ERROR | 0x04U,   /**< The SMIF API received the wrong parameter */
    CY_SMIF_NO_SFDP_SUPPORT = CY_SMIF_ID |CY_PDL_STATUS_ERROR | 0x05U,   /**< The external memory does not support SFDP (JESD216B). */
    /** Failed to initialize the slave select 0 external memory by auto detection (SFDP). */
    CY_SMIF_SFDP_SS0_FAILED = CY_SMIF_ID |CY_PDL_STATUS_ERROR |
                            ((uint32_t)CY_SMIF_SFDP_FAIL << CY_SMIF_SFDP_FAIL_SS0_POS),
    /** Failed to initialize the slave select 1 external memory by auto detection (SFDP). */
    CY_SMIF_SFDP_SS1_FAILED = CY_SMIF_ID | CY_PDL_STATUS_ERROR |
                            ((uint32_t)CY_SMIF_SFDP_FAIL << CY_SMIF_SFDP_FAIL_SS1_POS),
    /** Failed to initialize the slave select 2 external memory by auto detection (SFDP). */
    CY_SMIF_SFDP_SS2_FAILED = CY_SMIF_ID |CY_PDL_STATUS_ERROR |
                            ((uint32_t)CY_SMIF_SFDP_FAIL << CY_SMIF_SFDP_FAIL_SS2_POS),
    /** Failed to initialize the slave select 3 external memory by auto detection (SFDP). */
    CY_SMIF_SFDP_SS3_FAILED = CY_SMIF_ID |CY_PDL_STATUS_ERROR |
                            ((uint32_t)CY_SMIF_SFDP_FAIL << CY_SMIF_SFDP_FAIL_SS3_POS)

} cy_en_smif_status_t;

/** The SMIF slave select definitions for the driver API. Each slave select is
 * represented by an enumeration that has the bit corresponding to the slave 
 * select number set. */
typedef enum
{
   CY_SMIF_SLAVE_SELECT_0 = 1U,  /**< The SMIF slave select 0  */
   CY_SMIF_SLAVE_SELECT_1 = 2U,  /**< The SMIF slave select 1  */
   CY_SMIF_SLAVE_SELECT_2 = 4U,  /**< The SMIF slave select 2  */
   CY_SMIF_SLAVE_SELECT_3 = 8U   /**< The SMIF slave select 3  */
} cy_en_smif_slave_select_t;

/** Specifies the clock source for the receiver clock. */
typedef enum
{
   CY_SMIF_SEL_INTERNAL_CLK     = 0U,  /**< The SMIF internal clock */
   CY_SMIF_SEL_INV_INTERNAL_CLK = 1U,  /**< The SMIF internal inverted clock */
   CY_SMIF_SEL_FEEDBACK_CLK     = 2U,  /**< The SMIF feedback clock */
   CY_SMIF_SEL_INV_FEEDBACK_CLK = 3U   /**< The SMIF feedback inverted clock */
} cy_en_smif_clk_select_t;

/** Specifies enabled type of SMIF cache. */
typedef enum
{
    CY_SMIF_CACHE_SLOW      = 1U,   /**< The SMIF slow cache (in the clk_slow domain) see TRM for details */
    CY_SMIF_CACHE_FAST      = 2U,   /**< The SMIF fast cache  (in the clk_fast domain) see TRM for details */
    CY_SMIF_CACHE_BOTH      = 3U    /**< The SMIF both caches */
} cy_en_smif_cache_en_t;

/** \} group_smif_enums */


/**
* \addtogroup group_smif_data_structures
* \{
*/

/***************************************************************************//**
*
* The SMIF user callback function type called at the end of a transfer.
*
* \param event
* The event which caused a callback call.
*
*******************************************************************************/
typedef void (*cy_smif_event_cb_t)(uint32_t event);


/** The SMIF configuration structure. */
typedef struct
{
    uint32_t mode;              /**<  Specifies the mode of operation \ref cy_en_smif_mode_t. */
    uint32_t deselectDelay;     /**<  Specifies the minimum duration of SPI de-selection between SPI transfers:
                                *   - "0": 1 clock cycle.
                                *   - "1": 2 clock cycles.
                                *   - "2": 3 clock cycles.
                                *   - "3": 4 clock cycles.
                                *   - "4": 5 clock cycles.
                                *   - "5": 6 clock cycles.
                                *   - "6": 7 clock cycles.
                                *   - "7": 8 clock cycles. */
    uint32_t rxClockSel;        /**< Specifies the clock source for the receiver 
                                *  clock \ref cy_en_smif_clk_select_t. */
    uint32_t blockEvent;        /**< Specifies what happens when there is a Read  
                                * from an empty RX FIFO or a Write to a full 
                                * TX FIFO. \ref cy_en_smif_error_event_t. */
} cy_stc_smif_config_t;

/** The SMIF internal context data. The user must not modify it. */
typedef struct
{
    uint8_t volatile * volatile txBufferAddress;    /**<  The pointer to the data to transfer */
    uint32_t txBufferSize;                          /**<  The size of the data to transmit in bytes */
    /**
    * The transfer counter. The number of the transmitted bytes = txBufferSize - txBufferCounter
    */
    uint32_t volatile txBufferCounter;
    uint8_t volatile * volatile rxBufferAddress;    /**<  The pointer to the variable where the received data is stored */
    uint32_t rxBufferSize;                          /**<  The size of the data to be received in bytes */
    /**
    * The transfer counter. The number of the received bytes = rxBufferSize - rxBufferCounter
    */
    uint32_t volatile rxBufferCounter;
    /**
    * The status of the transfer. The transmitting / receiving is completed / in progress
    */
    uint32_t volatile transferStatus;
    cy_smif_event_cb_t volatile txCmpltCb;          /**< The user-defined callback executed at the completion of a transmission */
    cy_smif_event_cb_t volatile rxCmpltCb;          /**< The user-defined callback executed at the completion of a reception */
    /**
    * The timeout in microseconds for the blocking functions. This timeout value applies to all blocking APIs.
    */
    uint32_t timeout;
} cy_stc_smif_context_t;

/** \} group_smif_data_structures */


/**
* \addtogroup group_smif_low_level_functions
* \{
*/

cy_en_smif_status_t Cy_SMIF_Init(SMIF_Type *base, cy_stc_smif_config_t const *config,
                                uint32_t timeout,
                                cy_stc_smif_context_t *context);
void Cy_SMIF_DeInit(SMIF_Type *base);
void Cy_SMIF_SetDataSelect(SMIF_Type *base, cy_en_smif_slave_select_t slaveSelect,
                                cy_en_smif_data_select_t dataSelect);
void Cy_SMIF_SetMode(SMIF_Type *base, cy_en_smif_mode_t mode);
cy_en_smif_mode_t Cy_SMIF_GetMode(SMIF_Type const *base);
cy_en_smif_status_t Cy_SMIF_TransmitCommand(SMIF_Type *base,
                                uint8_t cmd,
                                cy_en_smif_txfr_width_t cmdTxfrWidth,
                                uint8_t const cmdParam[], uint32_t paramSize,
                                cy_en_smif_txfr_width_t paramTxfrWidth,
                                cy_en_smif_slave_select_t slaveSelect, uint32_t cmpltTxfr,
                                cy_stc_smif_context_t const *context);
cy_en_smif_status_t Cy_SMIF_TransmitData(SMIF_Type *base,
                                uint8_t *txBuffer, uint32_t size,
                                cy_en_smif_txfr_width_t transferWidth,
                                cy_smif_event_cb_t TxCmpltCb,
                                cy_stc_smif_context_t *context);
cy_en_smif_status_t  Cy_SMIF_TransmitDataBlocking(SMIF_Type *base,
                                uint8_t *txBuffer,
                                uint32_t size,
                                cy_en_smif_txfr_width_t transferWidth,
                                cy_stc_smif_context_t const *context);
cy_en_smif_status_t Cy_SMIF_ReceiveData(SMIF_Type *base,
                                uint8_t *rxBuffer, uint32_t size,
                                cy_en_smif_txfr_width_t transferWidth,
                                cy_smif_event_cb_t RxCmpltCb,
                                cy_stc_smif_context_t *context);
cy_en_smif_status_t  Cy_SMIF_ReceiveDataBlocking(SMIF_Type *base,
                                uint8_t *rxBuffer,
                                uint32_t size,
                                cy_en_smif_txfr_width_t transferWidth,
                                cy_stc_smif_context_t const *context);
cy_en_smif_status_t Cy_SMIF_SendDummyCycles(SMIF_Type *base, uint32_t cycles);
uint32_t Cy_SMIF_GetTxfrStatus(SMIF_Type *base, cy_stc_smif_context_t const *context);
void Cy_SMIF_Enable(SMIF_Type *base, cy_stc_smif_context_t *context);
__STATIC_INLINE void Cy_SMIF_Disable(SMIF_Type *base);
__STATIC_INLINE void  Cy_SMIF_SetInterruptMask(SMIF_Type *base, uint32_t interrupt);
__STATIC_INLINE uint32_t  Cy_SMIF_GetInterruptMask(SMIF_Type const *base);
__STATIC_INLINE uint32_t  Cy_SMIF_GetInterruptStatusMasked(SMIF_Type const *base);
__STATIC_INLINE uint32_t  Cy_SMIF_GetInterruptStatus(SMIF_Type const *base);
__STATIC_INLINE void  Cy_SMIF_SetInterrupt(SMIF_Type *base, uint32_t interrupt);
__STATIC_INLINE void  Cy_SMIF_ClearInterrupt(SMIF_Type *base, uint32_t interrupt);
__STATIC_INLINE void  Cy_SMIF_SetTxFifoTriggerLevel(SMIF_Type *base, uint32_t level);
__STATIC_INLINE void  Cy_SMIF_SetRxFifoTriggerLevel(SMIF_Type *base, uint32_t level);
__STATIC_INLINE uint32_t  Cy_SMIF_GetCmdFifoStatus(SMIF_Type const *base);
__STATIC_INLINE uint32_t  Cy_SMIF_GetTxFifoStatus(SMIF_Type const *base);
__STATIC_INLINE uint32_t  Cy_SMIF_GetRxFifoStatus(SMIF_Type const *base);
cy_en_smif_status_t  Cy_SMIF_Encrypt(SMIF_Type *base,
                                uint32_t address,
                                uint8_t data[],
                                uint32_t size,
                                cy_stc_smif_context_t const *context);
__STATIC_INLINE bool Cy_SMIF_BusyCheck(SMIF_Type const *base);
__STATIC_INLINE void Cy_SMIF_Interrupt(SMIF_Type *base, cy_stc_smif_context_t *context);
cy_en_smif_status_t Cy_SMIF_CacheEnable(SMIF_Type *base, cy_en_smif_cache_en_t cacheType);
cy_en_smif_status_t Cy_SMIF_CacheDisable(SMIF_Type *base, cy_en_smif_cache_en_t cacheType);
cy_en_smif_status_t Cy_SMIF_CachePrefetchingEnable(SMIF_Type *base, cy_en_smif_cache_en_t cacheType);
cy_en_smif_status_t Cy_SMIF_CachePrefetchingDisable(SMIF_Type *base, cy_en_smif_cache_en_t cacheType);
cy_en_smif_status_t Cy_SMIF_CacheInvalidate(SMIF_Type *base, cy_en_smif_cache_en_t cacheType);

/** \addtogroup group_smif_functions_syspm_callback
* The driver supports SysPm callback for Deep Sleep and Hibernate transition.
* \{
*/
cy_en_syspm_status_t Cy_SMIF_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams);
cy_en_syspm_status_t Cy_SMIF_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams);
/** \} */


/***************************************
*  Internal SMIF function declarations
****************************************/
/** \cond INTERNAL */
__STATIC_INLINE void Cy_SMIF_PushTxFifo(SMIF_Type *baseaddr, cy_stc_smif_context_t *context);      /**< Writes transmitted data into the FIFO. */
__STATIC_INLINE void Cy_SMIF_PopRxFifo(SMIF_Type *baseaddr, cy_stc_smif_context_t *context);       /**< Reads received data from the FIFO. */
__STATIC_INLINE uint32_t Cy_SMIF_PackBytesArray(uint8_t const buff[], bool fourBytes);
__STATIC_INLINE void Cy_SMIF_UnPackByteArray(uint32_t inValue, uint8_t outBuff[], bool fourBytes);
__STATIC_INLINE cy_en_smif_status_t Cy_SMIF_TimeoutRun(uint32_t *timeoutUnits);
__STATIC_INLINE SMIF_DEVICE_Type volatile * Cy_SMIF_GetDeviceBySlot(SMIF_Type *base,
                                cy_en_smif_slave_select_t slaveSelect);
/** \endcond*/

/** \} group_smif_low_level_functions */


/**
* \addtogroup group_smif_low_level_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_SMIF_Disable
****************************************************************************//**
*
* Disables the operation of the SMIF block. The SMIF block can be disabled only
* when it is not in the active state. Use the Cy_SMIF_BusyCheck() API to check
* it before calling this API.
*
* \param base
* Holds the base address of the SMIF block registers.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SMIF_Disable(SMIF_Type *base)
{
    SMIF_CTL(base) &= ~SMIF_CTL_ENABLED_Msk;

}


/*******************************************************************************
* Function Name: Cy_SMIF_SetInterruptMask
****************************************************************************//**
*
* This function is used to set an interrupt mask for the SMIF Interrupt.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param interrupt
* This is the mask for different source options that can be masked. See
* \ref group_smif_macros_isr "Interrupt Macros" for possible values.
*
*******************************************************************************/
__STATIC_INLINE void  Cy_SMIF_SetInterruptMask(SMIF_Type *base, uint32_t interrupt)
{
    SMIF_INTR_MASK(base) = interrupt;
}


/*******************************************************************************
* Function Name: Cy_SMIF_GetInterruptMask
****************************************************************************//**
*
* This function is used to read an interrupt mask for the SMIF Interrupt.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \return Returns the mask set for the SMIF interrupt.
*
*******************************************************************************/
__STATIC_INLINE uint32_t  Cy_SMIF_GetInterruptMask(SMIF_Type const *base)
{
  return (SMIF_INTR_MASK(base));
}


/*******************************************************************************
* Function Name: Cy_SMIF_GetInterruptStatusMasked
****************************************************************************//**
*
* This function is used to read an active masked interrupt. This function can
* be used in the interrupt service-routine to find which source triggered the
* interrupt.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \return Returns a word with bits set at positions corresponding to the
* interrupts triggered in the system.
*
*******************************************************************************/
__STATIC_INLINE uint32_t  Cy_SMIF_GetInterruptStatusMasked(SMIF_Type const *base)
{
  return (SMIF_INTR_MASKED(base));
}


/*******************************************************************************
* Function Name: Cy_SMIF_GetInterruptStatus
****************************************************************************//**
*
* This function is used to read an active interrupt. This status is the unmasked
* result, so will also show interrupts that will not generate active interrupts.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \return Returns a word with bits set at positions corresponding to the
* interrupts triggered in the system.
*
*******************************************************************************/
__STATIC_INLINE uint32_t  Cy_SMIF_GetInterruptStatus(SMIF_Type const *base)
{
  return (SMIF_INTR(base));
}


/*******************************************************************************
* Function Name: Cy_SMIF_SetInterrupt
****************************************************************************//**
*
* This function is used to set an interrupt source. This function can be used
* to activate interrupts through the software.
*
* \note Interrupt sources set using this interrupt will generate interrupts only
* if they are not masked.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param interrupt
* An encoded integer with a bit set corresponding to the interrupt to be
* triggered. See \ref group_smif_macros_isr "Interrupt Macros" for possible
* values.
*
*******************************************************************************/
__STATIC_INLINE void  Cy_SMIF_SetInterrupt(SMIF_Type *base, uint32_t interrupt)
{
    SMIF_INTR_SET(base) = interrupt;
}


/*******************************************************************************
* Function Name: Cy_SMIF_ClearInterrupt
****************************************************************************//**
*
* This function is used to clear an interrupt source. This function can be used
* in the user code to clear all pending interrupts.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param interrupt
* An encoded integer with a bit set corresponding to the interrupt that must
* be cleared. See \ref group_smif_macros_isr "Interrupt Macros" for possible
* values.
*
*******************************************************************************/
__STATIC_INLINE void  Cy_SMIF_ClearInterrupt(SMIF_Type *base, uint32_t interrupt)
{
    SMIF_INTR(base) = interrupt;

    /* Ensure that the initial Write has been flushed out to the hardware. */
    interrupt = SMIF_INTR(base);
}


/*******************************************************************************
* Function Name: Cy_SMIF_SetTxFifoTriggerLevel()
****************************************************************************//**
*
* This function is used to set a trigger level for the TX FIFO. This value must
* be an integer between 0 and 7. For the normal mode only.
* The triggering is active when TX_DATA_FIFO_STATUS <= level.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param level
* The trigger level to set (0-8).
*
*******************************************************************************/
__STATIC_INLINE void  Cy_SMIF_SetTxFifoTriggerLevel(SMIF_Type *base, uint32_t level)
{
    CY_ASSERT_L2(level <= CY_SMIF_MAX_TX_TR_LEVEL);
    SMIF_TX_DATA_FIFO_CTL(base) = level;
}


/*******************************************************************************
* Function Name: Cy_SMIF_SetRxFifoTriggerLevel()
****************************************************************************//**
*
* This function is used to set a trigger level for the RX FIFO. This value must
* be an integer between 0 and 7. For the normal mode only.
* The triggering is active when RX_DATA_FIFOSTATUS > level.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param level
* The trigger level to set(0-8).
*
*******************************************************************************/
__STATIC_INLINE void  Cy_SMIF_SetRxFifoTriggerLevel(SMIF_Type *base, uint32_t level)
{
    CY_ASSERT_L2(level <= CY_SMIF_MAX_RX_TR_LEVEL);
    SMIF_RX_DATA_FIFO_CTL(base) = level;
}


/*******************************************************************************
* Function Name: Cy_SMIF_GetCmdFifoStatus()
****************************************************************************//**
*
* This function is used to read the status of the CMD FIFO.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \return Returns the number of the entries in the CMD FIFO.
*
*******************************************************************************/
__STATIC_INLINE uint32_t  Cy_SMIF_GetCmdFifoStatus(SMIF_Type const *base)
{
  return (_FLD2VAL(SMIF_TX_CMD_FIFO_STATUS_USED3, SMIF_TX_CMD_FIFO_STATUS(base)));
}


/*******************************************************************************
* Function Name: Cy_SMIF_GetTxFifoStatus()
****************************************************************************//**
*
* This function is used to read the status of the TX FIFO.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \return Returns the number of the entries in the TX FIFO.
*
*******************************************************************************/
__STATIC_INLINE uint32_t  Cy_SMIF_GetTxFifoStatus(SMIF_Type const *base)
{
  return (_FLD2VAL(SMIF_TX_DATA_FIFO_STATUS_USED4, SMIF_TX_DATA_FIFO_STATUS(base)));
}


/*******************************************************************************
* Function Name: Cy_SMIF_GetRxFifoStatus()
****************************************************************************//**
*
* This function is used to read the status of the RX FIFO.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \return Returns the number of the entries in the RX FIFO.
*
*******************************************************************************/
__STATIC_INLINE uint32_t  Cy_SMIF_GetRxFifoStatus(SMIF_Type const *base)
{
  return (_FLD2VAL(SMIF_RX_DATA_FIFO_STATUS_USED4, SMIF_RX_DATA_FIFO_STATUS(base)));
}


/*******************************************************************************
* Function Name: Cy_SMIF_BusyCheck
****************************************************************************//**
*
* This function provides the status of the IP block (False - not busy,
* True - busy).
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \return Returns an IP block status.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SMIF_BusyCheck(SMIF_Type const *base)
{
  return (1UL == _FLD2VAL(SMIF_STATUS_BUSY, base->STATUS));
}


/*******************************************************************************
* Function Name: Cy_SMIF_Interrupt
****************************************************************************//**
*
* The Interrupt Service Routine for the SMIF. The interrupt code will be
* responsible for the FIFO operations on FIFO interrupts during ongoing transfers.
* The user must place a call to this interrupt function in the interrupt
* routine corresponding to the interrupt attached to the SMIF. If the
* user does not do this, will break: the functionality of all the API functions in
* the SMIF driver that use SMIF interrupts to affect transfers.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param context
* Passes a configuration structure that contains the transfer parameters of the
* SMIF block.
*
* \globalvars
*  - context->txBufferAddress - The pointer to the data to be transferred.
*
*  - context->txBufferSize - The size of txBuffer.
*
*  - context->txBufferCounter - The number of data entries left to be transferred.
*
* All the Global variables described above are used when the Software Buffer is
* used.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SMIF_Interrupt(SMIF_Type *base, cy_stc_smif_context_t *context)
{
    uint32_t interruptStatus = Cy_SMIF_GetInterruptStatusMasked(base);

    /* Check which interrupt occurred */
    if (0U != (interruptStatus & SMIF_INTR_TR_TX_REQ_Msk))
    {
        /* Send data */
        Cy_SMIF_PushTxFifo(base, context);

        Cy_SMIF_ClearInterrupt(base, SMIF_INTR_TR_TX_REQ_Msk);
    }
    else if (0U != (interruptStatus & SMIF_INTR_TR_RX_REQ_Msk))
    {
        /* Receive data */
        Cy_SMIF_PopRxFifo(base, context);

        Cy_SMIF_ClearInterrupt(base, SMIF_INTR_TR_RX_REQ_Msk);
    }
    else if (0U != (interruptStatus & SMIF_INTR_XIP_ALIGNMENT_ERROR_Msk))
    {
        /* An XIP alignment error */
        context->transferStatus = (uint32_t) CY_SMIF_XIP_ERROR;

        Cy_SMIF_ClearInterrupt(base, SMIF_INTR_XIP_ALIGNMENT_ERROR_Msk);
    }

    else if (0U != (interruptStatus & SMIF_INTR_TX_CMD_FIFO_OVERFLOW_Msk))
    {
        /* TX CMD FIFO overflow */
        context->transferStatus = (uint32_t) CY_SMIF_CMD_ERROR;

        Cy_SMIF_ClearInterrupt(base, SMIF_INTR_TX_CMD_FIFO_OVERFLOW_Msk);
    }

    else if (0U != (interruptStatus & SMIF_INTR_TX_DATA_FIFO_OVERFLOW_Msk))
    {
        /* A TX DATA FIFO overflow */
        context->transferStatus = (uint32_t) CY_SMIF_TX_ERROR;

        Cy_SMIF_ClearInterrupt(base, SMIF_INTR_TX_DATA_FIFO_OVERFLOW_Msk);
    }

    else if (0U != (interruptStatus & SMIF_INTR_RX_DATA_FIFO_UNDERFLOW_Msk))
    {
        /* RX DATA FIFO underflow */
        context->transferStatus = (uint32_t) CY_SMIF_RX_ERROR;

        Cy_SMIF_ClearInterrupt(base, SMIF_INTR_RX_DATA_FIFO_UNDERFLOW_Msk);
    }
    else
    {
        /* Processing of errors */
    }
}


/*******************************************************************************
*  Internal SMIF in-line functions
*******************************************************************************/

/** \cond INTERNAL */

/*******************************************************************************
* Function Name: Cy_SMIF_PushTxFifo
***************************************************************************//***
*
* \internal
*
* \param baseaddr
* Holds the base address of the SMIF block registers.
*
* \param context
* Passes a configuration structure that contains the transfer parameters of the
* SMIF block.
*
* This function writes data in the TX FIFO SMIF buffer by 4, 2, or 1 bytes based
* on the residual number of bytes and the available space in the TX FIFO.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SMIF_PushTxFifo(SMIF_Type *baseaddr, cy_stc_smif_context_t *context)
{
    /* The variable that shows which is smaller: the free FIFO size or amount of bytes to be sent */
    uint32_t writeBytes;
    uint32_t freeFifoBytes;
    uint32_t buffCounter = context->txBufferCounter;
    uint8_t *buff = (uint8_t*) context->txBufferAddress;

    freeFifoBytes = CY_SMIF_TX_DATA_FIFO_STATUS_RANGE - Cy_SMIF_GetTxFifoStatus(baseaddr);
    writeBytes = (freeFifoBytes > buffCounter)? buffCounter: freeFifoBytes;

    /* Check that after a FIFO Write, no data/FIFO space remains */
    while (0U != writeBytes)
    {
        /* The first main use case for long transfers */
        if (writeBytes == CY_SMIF_EIGHT_BYTES)
        {
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], true);
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[4U], true);
        }
        /* The second main use case for short transfers */
        else if(writeBytes == CY_SMIF_ONE_BYTE)
        {
            SMIF_TX_DATA_FIFO_WR1(baseaddr) = buff[0U];
        }
        else if(writeBytes == CY_SMIF_TWO_BYTES)
        {
            SMIF_TX_DATA_FIFO_WR2(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], false);
        }
        else if(writeBytes == CY_SMIF_THREE_BYTES)
        {
            SMIF_TX_DATA_FIFO_WR2(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], false);
            SMIF_TX_DATA_FIFO_WR1(baseaddr) = buff[2U];
        }
        else if(writeBytes == CY_SMIF_FOUR_BYTES)
        {
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], true);
        }
        else if(writeBytes == CY_SMIF_FIVE_BYTES)
        {
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], true);
            SMIF_TX_DATA_FIFO_WR1(baseaddr) = buff[4U];
        }
        else if(writeBytes == CY_SMIF_SIX_BYTES)
        {
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], true);
            SMIF_TX_DATA_FIFO_WR2(baseaddr) = Cy_SMIF_PackBytesArray(&buff[4U], false);
        }
        else if(writeBytes == CY_SMIF_SEVEN_BYTES)
        {
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], true);
            SMIF_TX_DATA_FIFO_WR2(baseaddr) = Cy_SMIF_PackBytesArray(&buff[4U], false);
            SMIF_TX_DATA_FIFO_WR1(baseaddr) = buff[6U];
        }
        else /* The  future IP block with FIFO > 8*/
        {
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[0U], true);
            SMIF_TX_DATA_FIFO_WR4(baseaddr) = Cy_SMIF_PackBytesArray(&buff[4U], true);
            writeBytes = CY_SMIF_EIGHT_BYTES;
        }
        buff = &buff[writeBytes];
        buffCounter -= writeBytes;
        /* Check if we already got new data in TX_FIFO*/
        freeFifoBytes = CY_SMIF_TX_DATA_FIFO_STATUS_RANGE - Cy_SMIF_GetTxFifoStatus(baseaddr);
        writeBytes = (freeFifoBytes > buffCounter)? buffCounter: freeFifoBytes;
    }

    /* Save changes in the context */
    context->txBufferAddress = buff;
    context->txBufferCounter = buffCounter;

    /* Check if all bytes are sent */
    if (0u == buffCounter)
    {
        /* Disable the TR_TX_REQ interrupt */
        Cy_SMIF_SetInterruptMask(baseaddr, Cy_SMIF_GetInterruptMask(baseaddr) & ~SMIF_INTR_TR_TX_REQ_Msk);

        context->transferStatus = (uint32_t) CY_SMIF_SEND_CMPLT;
        if (NULL != context->txCmpltCb)
        {
            context->txCmpltCb((uint32_t) CY_SMIF_SEND_CMPLT);
        }
    }
}


/*******************************************************************************
* Function Name: Cy_SMIF_PopRxFifo
***************************************************************************//***
*
* \internal
*
* \param baseaddr
* Holds the base address of the SMIF block registers.
*
* \param context
* Passes a configuration structure that contains the transfer parameters of the
* SMIF block.
*
* This function reads data from the RX FIFO SMIF buffer by 4, 2, or 1 bytes
* based on the data availability in the RX FIFO and amount of bytes to be
* received.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SMIF_PopRxFifo(SMIF_Type *baseaddr, cy_stc_smif_context_t *context)
{
    /* The variable that shows which is smaller: the free FIFO size or amount of bytes to be received */
    uint32_t readBytes;
    uint32_t loadedFifoBytes;
    uint32_t buffCounter = context->rxBufferCounter;
    uint8_t *buff = (uint8_t*) context->rxBufferAddress;

    loadedFifoBytes = Cy_SMIF_GetRxFifoStatus(baseaddr);
    readBytes = (loadedFifoBytes > buffCounter)? buffCounter: loadedFifoBytes;

    /* Check that after a FIFO Read, no new data is available */
    while (0U != readBytes)
    {
        if (readBytes == CY_SMIF_EIGHT_BYTES)
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[0U], true); 
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[4U], true);
        }
        else if(readBytes == CY_SMIF_ONE_BYTE)
        {
            buff[0U] = (uint8_t)SMIF_RX_DATA_FIFO_RD1(baseaddr);
        }
        else if(readBytes == CY_SMIF_TWO_BYTES)
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD2(baseaddr), &buff[0U], false); 
        }
        else if(readBytes == CY_SMIF_THREE_BYTES)
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD2(baseaddr), &buff[0U], false);
            buff[2U] = (uint8_t)SMIF_RX_DATA_FIFO_RD1(baseaddr);
        }
        else if(readBytes == CY_SMIF_FOUR_BYTES)
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[0U], true);
        }
        else if(readBytes == CY_SMIF_FIVE_BYTES)
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[0U], true);
            buff[4U] = (uint8_t)SMIF_RX_DATA_FIFO_RD1(baseaddr);
        }
        else if(readBytes == CY_SMIF_SIX_BYTES)
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[0U], true);
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD2(baseaddr), &buff[4U], false);
        }
        else if(readBytes == CY_SMIF_SEVEN_BYTES)
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[0U], true);
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD2(baseaddr), &buff[4U], false);
            buff[6U] = (uint8_t)SMIF_RX_DATA_FIFO_RD1(baseaddr);
        }
        else /* The IP block FIFO > 8*/
        {
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[0U], true);
            Cy_SMIF_UnPackByteArray(SMIF_RX_DATA_FIFO_RD4(baseaddr), &buff[4U], true);
            readBytes = CY_SMIF_EIGHT_BYTES;
        }

        buff = &buff[readBytes];
        buffCounter -= readBytes;
        /* Check if we already got new data in RX_FIFO*/
        loadedFifoBytes = Cy_SMIF_GetRxFifoStatus(baseaddr);
        readBytes = (loadedFifoBytes > buffCounter)? buffCounter: loadedFifoBytes;
    }

    /* Save changes in the context */
    context->rxBufferAddress = buff;
    context->rxBufferCounter = buffCounter;

    /* Check if all bytes are received */
    if (0UL == buffCounter)
    {
        /* Disable the TR_RX_REQ interrupt */
        Cy_SMIF_SetInterruptMask(baseaddr, Cy_SMIF_GetInterruptMask(baseaddr) & ~SMIF_INTR_TR_RX_REQ_Msk);
        context->transferStatus = (uint32_t) CY_SMIF_REC_CMPLT;
        if (NULL != context->rxCmpltCb)
        {
            context->rxCmpltCb((uint32_t) CY_SMIF_REC_CMPLT);
        }
    }

    context->rxBufferCounter = buffCounter;
}


/*******************************************************************************
* Function Name: Cy_SMIF_PackBytesArray
***************************************************************************//***
*
* \internal
*
* This function packs 0-numBytes of the buff byte array into a 4-byte value.
*
* \param buff
* The byte array to pack.
*
* \param fourBytes
*  - The True pack is for a 32-bit value.
*  - The False pack is for a 16-bit value.
*
* \return
*  The 4-byte value packed from the byte array.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SMIF_PackBytesArray(uint8_t const buff[], bool fourBytes)
{
    uint32_t result = 0UL;

    result = ((uint32_t)buff[1UL] << 8UL) | (uint32_t)buff[0UL];

    if(fourBytes)
    {
        result |= ((uint32_t)buff[3UL] << 24UL) | ((uint32_t)buff[2UL] << 16UL);
    }

    return result;
}


/*******************************************************************************
* Function Name: Cy_SMIF_UnPackByteArray
***************************************************************************//***
*
* \internal
*
* This function unpacks 0-numBytes from a 4-byte value into the byte array outBuff.
*
* \param smifReg
*  The 4-byte value to unpack.
*
* \param outBuff
* The byte array to fill.
*
* \param fourBytes
*  - The True unpack is for a 32-bit value.
*  - The False unpack is for a 16-bit value.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SMIF_UnPackByteArray(uint32_t inValue, uint8_t outBuff[], bool fourBytes)
{
    outBuff[0UL] = (uint8_t)(inValue & 0xFFUL);
    outBuff[1UL] = (uint8_t)((inValue >> 8UL ) & 0xFFUL);

    if(fourBytes)
    {
        outBuff[2UL] = (uint8_t)((inValue >> 16UL) & 0xFFUL);
        outBuff[3UL] = (uint8_t)((inValue >> 24UL) & 0xFFUL);
    }
}


/*******************************************************************************
* Function Name: Cy_SMIF_TimeoutRun
****************************************************************************//**
*
* \internal
*
* This function checks if the timeout is expired. Use the Cy_SysLib_DelayUs() function for
* implementation.
*
* \param timeoutUnits
*  The pointer to the timeout. The timeout measured in microseconds is multiplied by
*  CY_SMIF_WAIT_1_UNIT.
*
* \return
* A timeout status:
*     - \ref CY_SMIF_SUCCESS - The timeout has not expired or input timeoutUnits is 0.
*     - \ref CY_SMIF_EXCEED_TIMEOUT - The timeout has expired.
*
*******************************************************************************/
__STATIC_INLINE cy_en_smif_status_t Cy_SMIF_TimeoutRun(uint32_t *timeoutUnits)
{
    cy_en_smif_status_t status = CY_SMIF_SUCCESS;
    if (*timeoutUnits > 0u)
    {
        Cy_SysLib_DelayUs(CY_SMIF_WAIT_1_UNIT);
        --(*timeoutUnits);
        status = (0u == (*timeoutUnits))? CY_SMIF_EXCEED_TIMEOUT: CY_SMIF_SUCCESS;
    }
    return status;
}


/*******************************************************************************
* Function Name: Cy_SMIF_GetDeviceBySlot
****************************************************************************//**
*
* \internal
* This function returns the address of the SMIF device registers structure by the slave
* slot number.
*
* \param base
* Holds the base address of the SMIF block registers.
*
* \param slaveSelect
* The slave device ID. This number is either CY_SMIF_SLAVE_SELECT_0 or
* CY_SMIF_SLAVE_SELECT_1 or CY_SMIF_SLAVE_SELECT_2 or CY_SMIF_SLAVE_SELECT_3
* (\ref cy_en_smif_slave_select_t). It defines the slave-select line to use
* during the transmission.
*
*******************************************************************************/
__STATIC_INLINE SMIF_DEVICE_Type volatile * Cy_SMIF_GetDeviceBySlot(SMIF_Type *base,
                                            cy_en_smif_slave_select_t slaveSelect)
{
    SMIF_DEVICE_Type volatile *device;
    /* Connect the slave to its data lines */
    switch (slaveSelect)
    {
        case CY_SMIF_SLAVE_SELECT_0:
            device = &(SMIF_DEVICE_IDX(base, 0)); 
            break;
        case CY_SMIF_SLAVE_SELECT_1:
            device = &(SMIF_DEVICE_IDX(base, 1));
            break;
        case CY_SMIF_SLAVE_SELECT_2:
            device = &(SMIF_DEVICE_IDX(base, 2));
            break;
        case CY_SMIF_SLAVE_SELECT_3:
            device = &(SMIF_DEVICE_IDX(base, 3));
            break;
        default:
            /* A user error*/
            device = NULL;
            break;
    }

    return device;
}

/** \endcond */
/** \} group_smif_low_level_functions */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSMIF */

#endif /* (CY_SMIF_H) */

/** \} group_smif */


/* [] END OF FILE */
