/***************************************************************************//**
* \file cy_scb_i2c.h
* \version 2.20
*
* Provides I2C API declarations of the SCB driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \addtogroup group_scb_i2c
* \{
* Driver API for I2C Bus Peripheral
*
* I2C - The Inter-Integrated Circuit (I2C) bus is an industry-standard.
* The two-wire hardware interface was developed by Philips Semiconductors
* (now NXP Semiconductors).
*
* The I2C peripheral driver provides an API to implement I2C slave, master,
* or master-slave devices based on the SCB hardware block.
* I2C devices based on SCB hardware are compatible with I2C
* Standard-mode, Fast-mode, and Fast-mode Plus specifications as defined in
* the I2C-bus specification.
*
* Features:
* * An industry-standard I2C bus interface
* * Supports slave, master, and master-slave operation
* * Supports standard data rates of 100/400/1000 kbps
* * Hardware Address Match, multiple addresses
* * Wake From Deep Sleep on Address Match
*
* \section group_scb_i2c_configuration Configuration Considerations
* The I2C driver configuration can be divided to number of sequential
* steps listed below:
* * \ref group_scb_i2c_config
* * \ref group_scb_i2c_pins
* * \ref group_scb_i2c_clock
* * \ref group_scb_i2c_data_rate
* * \ref group_scb_i2c_intr
* * \ref group_scb_i2c_enable
*
* \note
* I2C driver is built on top of the SCB hardware block. The SCB3 instance is
* used as an example for all code snippets. Modify the code to match your
* design.
*
* \subsection group_scb_i2c_config Configure I2C
* To set up the I2C slave driver, provide the configuration parameters in the
* \ref cy_stc_scb_i2c_config_t structure. Provide i2cMode to the select
* operation mode slave, master or master-slave. For master modes, provide
* useRxFifo and useTxFifo parameters. For slave mode, also provide the
* slaveAddress and slaveAddressMask. The other parameters are optional for
* operation. To initialize the driver, call \ref Cy_SCB_I2C_Init
* function providing a pointer to the filled \ref cy_stc_scb_i2c_config_t
* structure and allocated \ref cy_stc_scb_i2c_context_t.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_CFG
*
* \subsection group_scb_i2c_pins Assign and Configure Pins
* Only dedicated SCB pins can be used for I2C operation. The HSIOM
* register must be configured to connect block to the pins. Also the I2C pins
* must be configured in Open-Drain, Drives Low mode (this pins configuration
* implies usage of external pull-up resistors):
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_CFG_PINS
*
* \note
* The alternative pins configuration is Resistive Pull-ups which implies usage
* internal pull-up resistors. This configuration is not recommended because
* resistor value is fixed and cannot be used for all supported data rates.
* Refer to the device datasheet parameter RPULLUP for resistor value specifications.
*
* \subsection group_scb_i2c_clock Assign Clock Divider
* The clock source must be connected to the SCB block to oversample input and
* output signals. You must use one of the 8-bit or 16-bit dividers <em><b>(the
* source clock of this divider must be Clk_Peri)</b></em>. Use the
* \ref group_sysclk driver API to do that.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_CFG_ASSIGN_CLOCK
*
* Set up I2C slave read and write buffer before enabling its
* operation using \ref Cy_SCB_I2C_SlaveConfigReadBuf and \ref
* Cy_SCB_I2C_SlaveConfigWriteBuf appropriately. Note that the master reads
* data from the slave read buffer and writes data into the slave write buffer.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_CFG_BUFFER
*
* \subsection group_scb_i2c_data_rate Configure Data Rate
* To get I2C slave operation with the desired data rate, the source clock must be
* fast enough to provide sufficient oversampling. Therefore, the clock divider
* must be configured to provide desired clock frequency. Use the
* \ref group_sysclk driver API to do that.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_CFG_DATA_RATE_SLAVE
*
* To get I2C master operation with the desired data rate, the source clock
* frequency and SCL low and high phase duration must be configured. Use the
* \ref group_sysclk driver API to configure source clock frequency. Then call
* \ref Cy_SCB_I2C_SetDataRate to set the SCL low and high phase duration.
* This function reach for SCL low and high phase settings based on source clock
* frequency.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_CFG_DATA_RATE_MASTER
*
* Alternatively, the low and high phase can be set directly using
* \ref Cy_SCB_I2C_MasterSetLowPhaseDutyCycle and
* \ref Cy_SCB_I2C_MasterSetHighPhaseDutyCycle functions. \n
* Refer to the technical reference manual (TRM) section I2C sub-section
* Oversampling and Bit Rate to get information how to configure I2C to run with
* the desired data rate.
*
* \note
* For I2C slave, the analog filter is used for all supported data rates. \n
* For I2C master, the analog filter is used for Standard and Fast modes and the
* digital filter for Fast Plus mode.
*
* \subsection group_scb_i2c_intr Configure Interrupt
* The interrupt is mandatory for I2C operation. The exception is the I2C master,
* which uses only the \ref group_scb_i2c_master_low_level_functions functions.
* The driver provides three interrupt functions: \ref Cy_SCB_I2C_Interrupt,
* \ref Cy_SCB_I2C_SlaveInterrupt, and \ref Cy_SCB_I2C_MasterInterrupt. One of
* these functions must be called in the interrupt handler for the selected SCB
* instance. Call \ref Cy_SCB_I2C_SlaveInterrupt when I2C is configured to
* operate as a slave, \ref Cy_SCB_I2C_MasterInterrupt when I2C is configured
* to operate as a master and \ref Cy_SCB_I2C_Interrupt when I2C is configured
* to operate as master and slave. Using the slave- or master-specific interrupt
* function allows reducing the flash consumed by the I2C driver. Also this
* interrupt must be enabled in the NVIC otherwise it will not work.
* \note
* The I2C driver documentation refers to the \ref Cy_SCB_I2C_Interrupt function
* when interrupt processing is mandatory for the operation. This is done to
* simplify the readability of the driver's documentation. The application should
*  call the slave- or master-specific interrupt functions \ref Cy_SCB_I2C_SlaveInterrupt
* or \ref Cy_SCB_I2C_MasterInterrupt, when appropriate.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_INTR_A
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_INTR_B
*
* \subsection group_scb_i2c_enable Enable I2C
* Finally, enable the I2C operation calling \ref Cy_SCB_I2C_Enable. Then I2C
* slave starts respond to the assigned address and I2C master ready to execute
* transfers.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_ENABLE
*
* \section group_scb_i2c_use_cases Common Use Cases
*
* \subsection group_scb_i2c_master_mode Master Operation
* The master API is divided into two categories:
* \ref group_scb_i2c_master_high_level_functions and
* \ref group_scb_i2c_master_low_level_functions. Therefore, there are two
* methods for initiating I2C master transactions using either <b>Low-Level or
* High-Level</b> API. These two methods are described below. Only one method
* should be used at a time. They should not be mixed.
*
* \subsubsection  group_scb_i2c_master_hl Use High-Level Functions
*  Call \ref Cy_SCB_I2C_MasterRead or \ref Cy_SCB_I2C_MasterWrite to
* communicate with the slave. These functions do not block and only start a
* transaction. After a transaction starts, the \ref Cy_SCB_I2C_Interrupt
* handles the further data transaction until its completion (successfully or with an error
* occurring). Therefore, \ref Cy_SCB_I2C_Interrupt must be called inside the
* interrupt handler to make the functions above work. To monitor the transaction,
* use \ref Cy_SCB_I2C_MasterGetStatus or register callback function using
* \ref Cy_SCB_I2C_RegisterEventCallback to be notified about
* \ref group_scb_i2c_macros_callback_events.
*
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_MASTER_WRITE_READ_INT
*
* \subsubsection group_scb_i2c_master_ll Use Low-Level Functions
* Call \ref Cy_SCB_I2C_MasterSendStart to generate a start, send an address
* with the Read/Write direction bit, and receive acknowledgment. After the
* address is ACKed by the slave, the transaction can be continued by calling
* \ref Cy_SCB_I2C_MasterReadByte or \ref Cy_SCB_I2C_MasterWriteByte depending
* on its direction. These functions handle one byte per call. Therefore,
* they should be called for each byte in the transaction. Note that for the
* Read transaction, the last byte must be NAKed. To complete the current
* transaction, call \ref Cy_SCB_I2C_MasterSendStop or call
* \ref Cy_SCB_I2C_MasterSendReStart to complete the current transaction and
* start a new one. Typically, do a restart to change the transaction
* direction without releasing the bus from the master control.
* The Low-Level functions are blocking and do not require calling
* \ref Cy_SCB_I2C_Interrupt inside the interrupt handler. Using these
* functions requires extensive knowledge of the I2C protocol to execute
* transactions correctly.
*
* <b>Master Write Operation</b>
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_MASTER_WRITE_MANUAL
*
* <b>Master Read Operation</b>
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_MASTER_READ_MANUAL
*
* \subsection group_scb_i2c_slave Slave Operation
* The slave operation is based on the \ref Cy_SCB_I2C_Interrupt that must be
* called inside the interrupt handler. The Read and Write buffer must
* be provided for the slave to enable communication with the master. Use
* \ref Cy_SCB_I2C_SlaveConfigReadBuf and \ref Cy_SCB_I2C_SlaveConfigWriteBuf
* for this purpose. Note that after transaction completion the buffer must be
* configured again. Otherwise, the same buffer is used starting from the point
* where the master stopped a previous transaction.
* For example: The Read buffer is configured to be 10 bytes and the master Read
* is 8 bytes. If the Read buffer is not configured again, the next master Read
* will start from the 9th byte.
* To monitor the transaction status, use \ref Cy_SCB_I2C_SlaveGetStatus or
* use \ref Cy_SCB_I2C_RegisterEventCallback to register a callback function
* to be notified about \ref group_scb_i2c_macros_callback_events.
*
* <b>Get Slave Events Notification</b>
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_SLAVE_REG_CALLBACK
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_SLAVE_NOTIFICATION
*
* <b>Polling Slave Completion Events</b>
* \snippet SCB_CompDatasheet_sut_01_revA.cydsn\i2c_snippets.c I2C_SLAVE_POLLING
*
* \note
* All slave API (except \ref Cy_SCB_I2C_SlaveAbortRead and
* \ref Cy_SCB_I2C_SlaveAbortWrite) <b>are not interrupt-protected</b> and to
* prevent a race condition, they should be protected from the I2C interruption
* in the place where they are called.
*
* \section group_scb_i2c_lp Low Power Support
* The I2C driver provides the callback functions to handle power mode transition.
* The callback \ref Cy_SCB_I2C_DeepSleepCallback must be called
* during execution of \ref Cy_SysPm_DeepSleep; \ref Cy_SCB_I2C_HibernateCallback
* must be called during execution of \ref Cy_SysPm_Hibernate. To trigger the
* callback execution, the callback must be registered before calling the
* power mode transition function. Refer to \ref group_syspm driver for more
* information about power mode transitions and callback registration.
*
* \note
* Only applicable for <b>rev-08 of the CY8CKIT-062-BLE</b>.
* For proper operation, when the I2C slave is configured to be a wakeup
* source from Deep Sleep mode, the \ref Cy_SCB_I2C_DeepSleepCallback must be
* copied and modified. Refer to the function description to get the details.
*
* \section group_scb_i2c_more_information More Information
* For more information on the SCB peripheral, refer to the technical reference
* manual (TRM).
*
* \section group_scb_i2c_MISRA MISRA-C Compliance
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
*     <td>The functions \ref Cy_SCB_I2C_DeepSleepCallback and
*         \ref Cy_SCB_I2C_HibernateCallback are callback of
*         \ref cy_en_syspm_status_t type. The cast operation safety in these
*         functions becomes the user's responsibility because pointers are
*         initialized when callback is registered in SysPm driver.</td>
*   </tr>
*   <tr>
*     <td>14.1</td>
*     <td>R</td>
*     <td>There shall be no unreachable code.</td>
*     <td>The SCB block parameters can be a constant false or true depends on
*         the selected device and cause code to be unreachable.</td>
*   </tr>
*   <tr>
*     <td>13.7</td>
*     <td>R</td>
*     <td>Boolean operations whose results are invariant shall not be
*         permitted.</td>
*     <td>
*         * The SCB block parameters can be a constant false or true depends on
*         the selected device and cause this violation.
*         * The same condition check is executed before and after callback is
*         called because after the callback returns, the condition might be not
*         true any more.</td>
*   </tr>
*   <tr>
*     <td>14.2</td>
*     <td>R</td>
*     <td>All non-null statements shall either: a) have at least one side-effect
*         however executed, or b) cause control flow to change.</td>
*     <td>The unused function parameters are cast to void. This statement
*         has no side-effect and is used to suppress a compiler warning.</td>
*   </tr>
*   <tr>
*     <td>14.7</td>
*     <td>R</td>
*     <td>A function shall have a single point of exit at the end of the
*         function.</td>
*     <td>The functions can return from several points. This is done to improve
*         code clarity when returning error status code if input parameters
*         validation fails.</td>
*   </tr>
* </table>
*
* \section group_scb_i2c_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td rowspan="4"> 2.10</td>
*     <td>Fixed the ReStart condition generation sequence for a write
*         transaction in the \ref Cy_SCB_I2C_MasterWrite function.</td>
*     <td>The driver can notify about a zero length write transaction completion 
*         before the address byte is sent if the \ref Cy_SCB_I2C_MasterWrite 
*         function execution was interrupted between setting the restart 
*         generation command and writing the address byte into the TX FIFO.</td>
*   </tr>
*   <tr>
*     <td>Added the slave- and master-specific interrupt functions:
*         \ref Cy_SCB_I2C_SlaveInterrupt and \ref Cy_SCB_I2C_MasterInterrupt.
*     </td>
*     <td>Improved the interrupt configuration options for the I2C slave and
*         master mode configurations.</td>
*   </tr>
*   <tr>
*     <td>Updated the Start condition generation sequence in the \ref 
*         Cy_SCB_I2C_MasterWrite and \ref Cy_SCB_I2C_MasterRead.</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>Updated the ReStart condition generation sequence for a write
*         transaction in the \ref Cy_SCB_I2C_MasterSendReStart function.</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td rowspan="5"> 2.0</td>
*     <td>Fixed the \ref Cy_SCB_I2C_MasterSendReStart function to properly 
*         generate the ReStart condition when the previous transaction was 
*         a write.</td>
*     <td>The master interpreted the address byte written into the TX FIFO as a
*         data byte and continued a write transaction. The ReStart condition was
*         generated after the master completed transferring the data byte.
*         The SCL line was stretched by the master waiting for the address byte
*         to be written into the TX FIFO after the ReStart condition generation.
*         The following timeout detection released the bus from the master
*         control.</td>
*   </tr>
*   <tr>
*     <td>Fixed the slave operation after the address byte was NACKed by the
*         firmware.</td>
*     <td>The observed slave operation failure depends on whether Level 2 assert
*         is enabled or not. Enabled: the device stuck in the fault handler due
*         to the assert assignment in the \ref Cy_SCB_I2C_Interrupt. Disabled: 
*         the slave sets the transaction completion status and notifies on the 
*         transaction completion event after the address was NACKed. The failure
*         is observed only when the slave is configured to accept an address in 
*         the RX FIFO.</td>
*   </tr>
*   <tr>
*     <td>Added parameters validation for public API.</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>Replaced variables which have limited range of values with enumerated
*         types.</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>Added missing "cy_cb_" to the callback function type names.</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version.</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_scb_i2c_macros Macros
* \defgroup group_scb_i2c_functions Functions
* \{
* \defgroup group_scb_i2c_general_functions General
* \defgroup group_scb_i2c_slave_functions Slave
* \defgroup group_scb_i2c_master_high_level_functions Master High-Level
* \defgroup group_scb_i2c_master_low_level_functions Master Low-Level
* \defgroup group_scb_i2c_interrupt_functions Interrupt
* \defgroup group_scb_i2c_low_power_functions Low Power Callbacks
* \}
* \defgroup group_scb_i2c_data_structures Data Structures
* \defgroup group_scb_i2c_enums Enumerated Types
*/

#if !defined(CY_SCB_I2C_H)
#define CY_SCB_I2C_H

#include "cy_scb_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************
*          Enumerated Types
***************************************/

/**
* \addtogroup group_scb_i2c_enums
* \{
*/

/** I2C status codes */
typedef enum
{
    /** Operation completed successfully */
    CY_SCB_I2C_SUCCESS = 0U,

    /** One or more of input parameters are invalid */
    CY_SCB_I2C_BAD_PARAM = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 1U),

    /**
    * The master is not ready to start a new transaction.
    * Either the master is still processing a previous transaction or in the
    * master-slave mode, the slave operation is in progress. Call this function
    * again after that operation is completed or aborted.
    */
    CY_SCB_I2C_MASTER_NOT_READY = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 2U),

    /**
    * The master operation timed out before completing. Applicable only for
    * the \ref group_scb_i2c_master_low_level_functions functions.
    */
    CY_SCB_I2C_MASTER_MANUAL_TIMEOUT = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 3U),

    /** The slave NACKed the address. Applicable only for the
    * \ref group_scb_i2c_master_low_level_functions functions.
    */
    CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 4U),

    /** The slave NACKed the data byte.  Applicable only for the
    * \ref group_scb_i2c_master_low_level_functions.
    */
    CY_SCB_I2C_MASTER_MANUAL_NAK = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 5U),

    /**
    * The master lost arbitration, the transaction was aborted. Applicable only
    * for the \ref group_scb_i2c_master_low_level_functions functions.
    */
    CY_SCB_I2C_MASTER_MANUAL_ARB_LOST = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 6U),

    /**
    * The master detected an erroneous start or stop, the transaction was
    * aborted. Applicable only for the
    * \ref group_scb_i2c_master_low_level_functions functions.
    */
    CY_SCB_I2C_MASTER_MANUAL_BUS_ERR = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 7U),

    /**
    * The master transaction was aborted and the slave transaction is on-going
    * because the slave was addressed before the master generated a start.
    * Applicable only for the \ref group_scb_i2c_master_low_level_functions
    * functions.
    */
    CY_SCB_I2C_MASTER_MANUAL_ABORT_START = (CY_SCB_ID | CY_PDL_STATUS_ERROR | CY_SCB_I2C_ID | 8U)
} cy_en_scb_i2c_status_t;

/** I2C Operation Modes */
typedef enum
{
    CY_SCB_I2C_SLAVE        = 1U,    /**< Configures SCB for I2C Slave operation */
    CY_SCB_I2C_MASTER       = 2U,    /**< Configures SCB for I2C Master operation */
    CY_SCB_I2C_MASTER_SLAVE = 3U,    /**< Configures SCB for I2C Master-Slave operation */
} cy_en_scb_i2c_mode_t;

/** I2C Transaction Direction */
typedef enum
{
    CY_SCB_I2C_WRITE_XFER = 0U,  /**< Current transaction is Write */
    CY_SCB_I2C_READ_XFER  = 1U,  /**< Current transaction is Read */
} cy_en_scb_i2c_direction_t;

/** I2C Command ACK / NAK */
typedef enum
{
    CY_SCB_I2C_ACK,     /**< Send ACK to current byte */
    CY_SCB_I2C_NAK,     /**< Send NAK to current byte */
} cy_en_scb_i2c_command_t;
/** \} group_scb_i2c_enums */


/***************************************
*       Type Definitions
***************************************/

/**
* \addtogroup group_scb_i2c_data_structures
* \{
*/

/**
* Provides the typedef for the callback function called in the
* \ref Cy_SCB_I2C_Interrupt to notify the user about occurrences of
* \ref group_scb_i2c_macros_callback_events.
*/
typedef void (* cy_cb_scb_i2c_handle_events_t)(uint32_t event);

/**
* Provides the typedef for the callback function called in the
* \ref Cy_SCB_I2C_Interrupt to notify the user about occurrences of
* \ref group_scb_i2c_macros_addr_callback_events.
* This callback must return a decision to ACK (continue transaction) or
* NAK (end transaction) the received address.
* Note if the slave is configured to accept an address in RX FIFO, it must read
* from it using the \ref Cy_SCB_ReadRxFifo function.
*/
typedef cy_en_scb_i2c_command_t (* cy_cb_scb_i2c_handle_addr_t)(uint32_t event);

/** I2C configuration structure */
typedef struct cy_stc_scb_i2c_config
{
    /** Specifies the mode of operation */
    cy_en_scb_i2c_mode_t i2cMode;

    /**
    * The SCB provides an RX FIFO in hardware (consult the selected device
    * datasheet to get the actual FIFO size). The useRxFifo field defines
    * how the driver firmware reads data from the RX FIFO:
    * * If this option is enabled, the hardware is configured to automatically
    *   ACK incoming data, and interrupt is enabled to take data out of the RX
    *   FIFO when it has some number of bytes (typically, when it is half full).
    * * If this option is disabled, the interrupt is enabled to take data out of
    *   the RX FIFO when a byte is available. Also, hardware does not
    *   automatically ACK the data. Firmware must tell the hardware to ACK
    *   the byte (so each byte requires interrupt processing).
    * \n <b>Typically, this option should be enabled</b> to configure hardware to
    * automatically ACK incoming data. Otherwise hardware might not get the command
    * to ACK or NACK a byte fast enough, and clock stretching is applied
    * (the transaction is delayed) until the command is set. When this option is
    * enabled, the number of interrupts required to process the transaction
    * is significantly reduced because several bytes are handled at once.
    * \n <b>However, there is a side effect:</b>
    * * For master mode, the drawback is that the master may receive more
    *   data than desired due to the interrupt latency. An interrupt fires
    *   when the second-to-last byte has been received. This interrupt tells
    *   the hardware to stop receiving data. If the latency of this interrupt
    *   is longer than one transaction of the byte on the I2C bus, then the
    *   hardware automatically ACKs the following bytes until the interrupt
    *   is serviced or the RX FIFO becomes full.
    * * For slave mode, the drawback is that the slave only NACKs
    *   the master when the RX FIFO becomes full, NOT when the slave write
    *   firmware buffer becomes full.
    * \n In either master or slave mode, all received extra bytes are dropped.
    * \note The useRxFifo option is not available if acceptAddrInFifo is true.
    */
    bool useRxFifo;

    /**
    * The SCB provides a TX FIFO in hardware (consult the selected device
    * datasheet to get the actual FIFO size). The useTxFifo option defines how the
    * driver firmware loads data into the TX FIFO:
    * * If this option is enabled, the TX FIFO is fully loaded with data and the
    *   interrupt is enabled to keep the TX FIFO loaded until the end of the transaction.
    * * If this option is disabled, a single byte is loaded into the TX FIFO and
    *   the interrupt enabled to load the next byte when the TX FIFO becomes empty
    *   (so each byte requires interrupt processing).
    * \n <b>Typically, this option should be enabled</b> to keep the TX FIFO loaded with
    * data and reduce the probability of clock stretching. When there is no data
    * to transaction, clock stretching is applied (the transaction is delayed) until
    * the data is loaded. When this option is enabled, the number of interrupts required
    * to process the transaction is significantly reduced because several
    * bytes are handled at once.
    * \n <b>The drawback of enabling useTxFifo</b> is that the abort operation clears
    * the TX FIFO. The TX FIFO clear operation also clears the shift
    * register. As a result the shifter may be cleared in the middle of a byte
    * transaction, corrupting it. The remaining bits to transaction within the
    * corrupted byte are complemented with 1s. If this is an issue,
    * then do not enable this option.
    */
    bool useTxFifo;

    /**
    * The 7-bit right justified slave address, used only for the slave mode
    */
    uint8_t slaveAddress;

    /**
    * The slave address mask, bit 0, must always be 0. It is used only for the
    * slave mode
    */
    uint8_t slaveAddressMask;

    /**
    * True - the slave address is accepted in the RX FIFO, false - the slave
    * addresses are not accepted in the RX FIFO
    */
    bool acceptAddrInFifo;

    /**
    * True - accept the general call address; false - ignore the general
    * call address.
    */
    bool ackGeneralAddr;

    /**
    * When set, the slave will wake the device from Deep Sleep on an address
    * match (the device datasheet must be consulted to determine which SCBs
    * support this mode)
    */
    bool enableWakeFromSleep;
} cy_stc_scb_i2c_config_t;

/** I2C context structure.
* All fields for the context structure are internal. Firmware never reads or
* writes these values. Firmware allocates the structure and provides the
* address of the structure to the driver in function calls. Firmware must
* ensure that the defined instance of this structure remains in scope
* while the drive is in use.
*/
typedef struct cy_stc_scb_i2c_context
{
    /** \cond INTERNAL */
    bool useRxFifo;             /**< Stores RX FIFO configuration */
    bool useTxFifo;             /**< Stores TX FIFO configuration */

    volatile uint32_t state;    /**< The driver state */

    volatile uint32_t masterStatus; /**< The master status */
    bool     masterPause;           /**< Stores how the master ends the transaction */
    bool     masterRdDir;           /**< The direction of the master transaction */

    uint8_t  *masterBuffer;     /**< The pointer to the master buffer (either for a transmit or a receive operation) */
    uint32_t  masterBufferSize;         /**< The current master buffer size */
    volatile uint32_t masterBufferIdx;  /**< The current location in the master buffer */
    volatile uint32_t masterNumBytes;   /**< The number of bytes to send or receive */

    volatile uint32_t slaveStatus;       /**< The slave status */
    volatile bool     slaveRdBufEmpty;   /**< Tracks slave Read buffer empty event */

    uint8_t  *slaveTxBuffer;             /**< The pointer to the slave transmit buffer (a master reads from it) */
    uint32_t  slaveTxBufferSize;         /**< The current slave transmit buffer size */
    volatile uint32_t slaveTxBufferIdx;  /**< The current location in the slave buffer */
    volatile uint32_t slaveTxBufferCnt;  /**< The number of transferred bytes */

    uint8_t  *slaveRxBuffer;             /**< The pointer to the slave receive buffer (a master writes into it) */
    uint32_t  slaveRxBufferSize;         /**< The current slave receive buffer size */
    volatile uint32_t slaveRxBufferIdx;  /**< The current location in the slave buffer */

    /**
    * The pointer to an event callback that is called when any of
    * \ref group_scb_i2c_macros_callback_events occurs
    */
    cy_cb_scb_i2c_handle_events_t cbEvents;

    /**
    * The pointer to an address callback that is called when any of
    * \ref group_scb_i2c_macros_addr_callback_events occurs (applicable only
    * for the slave)
    */
    cy_cb_scb_i2c_handle_addr_t   cbAddr;

    /** \endcond */
} cy_stc_scb_i2c_context_t;

/** The I2C Master transfer structure */
typedef struct cy_stc_scb_i2c_master_xfer_config
{
    /** The 7-bit right justified slave address to communicate with */
    uint8_t  slaveAddress;

    /**
    * The pointer to the buffer for data to read from the slave or with
    * data to write into the slave
    */
    uint8_t  *buffer;

    /** The size of the buffer */
    uint32_t bufferSize;

    /**
    * The transfer operation is pending - the stop condition will not
    * be generated
    */
    bool     xferPending;
} cy_stc_scb_i2c_master_xfer_config_t;
/** \} group_scb_i2c_data_structures */


/***************************************
*        Function Prototypes
***************************************/

/**
* \addtogroup group_scb_i2c_general_functions
* \{
*/
cy_en_scb_i2c_status_t Cy_SCB_I2C_Init(CySCB_Type *base, cy_stc_scb_i2c_config_t const *config,
                                       cy_stc_scb_i2c_context_t *context);
void Cy_SCB_I2C_DeInit(CySCB_Type *base);
__STATIC_INLINE void Cy_SCB_I2C_Enable(CySCB_Type *base);
void Cy_SCB_I2C_Disable(CySCB_Type *base, cy_stc_scb_i2c_context_t *context);

uint32_t Cy_SCB_I2C_SetDataRate(CySCB_Type *base, uint32_t dataRateHz, uint32_t scbClockHz);
uint32_t Cy_SCB_I2C_GetDataRate(CySCB_Type const *base, uint32_t scbClockHz);

__STATIC_INLINE void     Cy_SCB_I2C_SlaveSetAddress(CySCB_Type *base, uint8_t addr);
__STATIC_INLINE uint32_t Cy_SCB_I2C_SlaveGetAddress(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_I2C_SlaveSetAddressMask(CySCB_Type *base, uint8_t addrMask);
__STATIC_INLINE uint32_t Cy_SCB_I2C_SlaveGetAddressMask(CySCB_Type const *base);

__STATIC_INLINE void Cy_SCB_I2C_MasterSetLowPhaseDutyCycle (CySCB_Type *base, uint32_t clockCycles);
__STATIC_INLINE void Cy_SCB_I2C_MasterSetHighPhaseDutyCycle(CySCB_Type *base, uint32_t clockCycles);

__STATIC_INLINE bool Cy_SCB_I2C_IsBusBusy(CySCB_Type const *base);
/** \} group_scb_i2c_general_functions */

/**
* \addtogroup group_scb_i2c_slave_functions
* \{
*/
void Cy_SCB_I2C_SlaveConfigReadBuf (CySCB_Type const *base, uint8_t *buffer, uint32_t size,
                                    cy_stc_scb_i2c_context_t *context);
void Cy_SCB_I2C_SlaveAbortRead     (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
void Cy_SCB_I2C_SlaveConfigWriteBuf(CySCB_Type const *base, uint8_t *buffer, uint32_t size,
                                    cy_stc_scb_i2c_context_t *context);
void Cy_SCB_I2C_SlaveAbortWrite    (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);

uint32_t Cy_SCB_I2C_SlaveGetStatus       (CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context);
uint32_t Cy_SCB_I2C_SlaveClearReadStatus (CySCB_Type const *base, cy_stc_scb_i2c_context_t *context);
uint32_t Cy_SCB_I2C_SlaveClearWriteStatus(CySCB_Type const *base, cy_stc_scb_i2c_context_t *context);

uint32_t Cy_SCB_I2C_SlaveGetReadTransferCount (CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context);
uint32_t Cy_SCB_I2C_SlaveGetWriteTransferCount(CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context);
/** \} group_scb_i2c_slave_functions */

/**
* \addtogroup group_scb_i2c_master_high_level_functions
* \{
*/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterWrite(CySCB_Type *base, cy_stc_scb_i2c_master_xfer_config_t *xferConfig,
                                              cy_stc_scb_i2c_context_t *context);
void     Cy_SCB_I2C_MasterAbortWrite         (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterRead (CySCB_Type *base, cy_stc_scb_i2c_master_xfer_config_t* xferConfig,
                                              cy_stc_scb_i2c_context_t *context);
void     Cy_SCB_I2C_MasterAbortRead          (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
uint32_t Cy_SCB_I2C_MasterGetStatus          (CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context);
uint32_t Cy_SCB_I2C_MasterGetTransferCount   (CySCB_Type const *base, cy_stc_scb_i2c_context_t const *context);
/** \} group_scb_i2c_master_low_high_functions */

/**
* \addtogroup group_scb_i2c_master_low_level_functions
* \{
*/
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterSendStart  (CySCB_Type *base, uint32_t address, cy_en_scb_i2c_direction_t bitRnW,
                                                    uint32_t timeoutMs, cy_stc_scb_i2c_context_t *context);
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterSendReStart(CySCB_Type *base, uint32_t address, cy_en_scb_i2c_direction_t bitRnW,
                                                    uint32_t timeoutMs, cy_stc_scb_i2c_context_t *context);
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterSendStop   (CySCB_Type *base,uint32_t timeoutMs, cy_stc_scb_i2c_context_t *context);
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterReadByte   (CySCB_Type *base, cy_en_scb_i2c_command_t ackNack, uint8_t *byte,
                                                    uint32_t timeoutMs, cy_stc_scb_i2c_context_t *context);
cy_en_scb_i2c_status_t Cy_SCB_I2C_MasterWriteByte  (CySCB_Type *base, uint8_t byte, uint32_t timeoutMs,
                                                    cy_stc_scb_i2c_context_t *context);
/** \} group_scb_i2c_master_low_level_functions */

/**
* \addtogroup group_scb_i2c_interrupt_functions
* \{
*/
void Cy_SCB_I2C_Interrupt      (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
void Cy_SCB_I2C_SlaveInterrupt (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);
void Cy_SCB_I2C_MasterInterrupt (CySCB_Type *base, cy_stc_scb_i2c_context_t *context);

__STATIC_INLINE void Cy_SCB_I2C_RegisterEventCallback(CySCB_Type const *base, cy_cb_scb_i2c_handle_events_t callback,
                                                      cy_stc_scb_i2c_context_t *context);
__STATIC_INLINE void Cy_SCB_I2C_RegisterAddrCallback (CySCB_Type const *base, cy_cb_scb_i2c_handle_addr_t callback,
                                                      cy_stc_scb_i2c_context_t *context);
/** \} group_scb_i2c_interrupt_functions */

/**
* \addtogroup group_scb_i2c_low_power_functions
* \{
*/
cy_en_syspm_status_t Cy_SCB_I2C_DeepSleepCallback(cy_stc_syspm_callback_params_t *callbackParams);
cy_en_syspm_status_t Cy_SCB_I2C_HibernateCallback(cy_stc_syspm_callback_params_t *callbackParams);
/** \} group_scb_i2c_low_power_functions */


/***************************************
*            API Constants
***************************************/

/**
* \addtogroup group_scb_i2c_macros
* \{
*/

/**
* \defgroup group_scb_i2c_macros_slave_status I2C Slave Status
* Each I2C slave status is encoded in a separate bit, therefore multiple bits
* may be set to indicate the current status.
* \{
*/
/** There is a read transaction in progress */
#define CY_SCB_I2C_SLAVE_RD_BUSY       (0x00000001UL)

/**
* All read data has been loaded into the TX FIFO, applicable only if
* the TX FIFO is used
*/
#define CY_SCB_I2C_SLAVE_RD_IN_FIFO    (0x00000002UL)

/**
* The master has finished reading data from the slave
*/
#define CY_SCB_I2C_SLAVE_RD_CMPLT      (0x00000004UL)

/**
* Set when the master tried to read more bytes than available in the configured
* read buffer. The slave is not able to finish the transaction and sends
* \ref CY_SCB_I2C_DEFAULT_TX.
*/
#define CY_SCB_I2C_SLAVE_RD_UNDRFL     (0x00000008UL)

/** There is a write transaction in progress */
#define CY_SCB_I2C_SLAVE_WR_BUSY       (0x00000010UL)

/**
* The master has finished writing data into the slave
*/
#define CY_SCB_I2C_SLAVE_WR_CMPLT      (0x00000020UL)

/**
* The master attempted to write more bytes than space available in the
* configured Write buffer. Note that all subsequent bytes are dropped.
*/
#define CY_SCB_I2C_SLAVE_WR_OVRFL      (0x00000040UL)

/** The slave lost arbitration, and the transaction was aborted */
#define CY_SCB_I2C_SLAVE_ARB_LOST      (0x00000080UL)

/**
* The slave captured an error on the bus during a master transaction (source
* of error is misplaced Start or Stop).
*/
#define CY_SCB_I2C_SLAVE_BUS_ERR       (0x00000100UL)
/** \} group_scb_i2c_macros_slave_status */

/**
* \defgroup group_scb_i2c_macros_master_status I2C Master Status
* Each I2C master status is encoded in a separate bit, therefore multiple
* bits may be set to indicate the current status.
* \{
*/

/**
* The master is busy executing operation started by \ref Cy_SCB_I2C_MasterRead
* or \ref Cy_SCB_I2C_MasterWrite
*/
#define CY_SCB_I2C_MASTER_BUSY         (0x00010000UL)

/**
* All Write data has been loaded into the TX FIFO
*/
#define CY_SCB_I2C_MASTER_WR_IN_FIFO   (0x00020000UL)

/** The slave NACKed the address. */
#define CY_SCB_I2C_MASTER_ADDR_NAK     (0x00100000UL)

/** Write completed before all bytes were sent (last byte was NAKed)
*/
#define CY_SCB_I2C_MASTER_DATA_NAK     (0x00200000UL)

/** The master lost arbitration, the transaction was aborted */
#define CY_SCB_I2C_MASTER_ARB_LOST     (0x00400000UL)

/**
* The master detected an erroneous start or stop, the transaction was aborted
*/
#define CY_SCB_I2C_MASTER_BUS_ERR      (0x00800000UL)

/**
* The master transaction was aborted and the slave transaction is on-going
* because the slave was addressed before the master generated a start
*/
#define CY_SCB_I2C_MASTER_ABORT_START  (0x01000000UL)
/** \} group_scb_i2c_macros_master_status */

/**
* \defgroup group_scb_i2c_macros_callback_events I2C Callback Events
* \{
* Each event is encoded in a separate bit, and therefore it is possible to
* notify about multiple events.
*/
/**
* Indicates that the slave was addressed and the master wants to read data.
* This event can be used to configure the slave Read buffer.
*/
#define CY_SCB_I2C_SLAVE_READ_EVENT            (0x00000001UL)

/**
* Indicates that the slave was addressed and the master wants to write data.
* This event can be used to configure the slave Write buffer.
*/
#define CY_SCB_I2C_SLAVE_WRITE_EVENT           (0x00000002UL)

/**
* All slave data from the configured Read buffer has been loaded into the
* TX FIFO. The content of the Read buffer can be modified. Applicable only
* if the TX FIFO is used.
*/
#define CY_SCB_I2C_SLAVE_RD_IN_FIFO_EVENT      (0x00000004UL)

/**
* The master has read all data out of the configured Read buffer.
* This event can be used to configure the next Read buffer. If the buffer
* remains empty, the \ref CY_SCB_I2C_DEFAULT_TX bytes are returned to the master.
*/
#define CY_SCB_I2C_SLAVE_RD_BUF_EMPTY_EVENT    (0x00000008UL)

/**
* Indicates the master completed reading from the slave (set by the master NAK
* or Stop)
*/
#define CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT        (0x00000010UL)

/**
* Indicates the master completed writing to the slave (set by the master Stop
* or Restart)
*/
#define CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT        (0x00000020UL)

/**
* Indicates the I2C hardware detected an error.
* Check \ref Cy_SCB_I2C_SlaveGetStatus to determine the source of the error.
*/
#define CY_SCB_I2C_SLAVE_ERR_EVENT             (0x00000040UL)

/**
* All data specified by \ref Cy_SCB_I2C_MasterWrite have been loaded
* into the TX FIFO. The content of the master buffer can be modified.
* Applicable only if the TX FIFO is used.
*/
#define CY_SCB_I2C_MASTER_WR_IN_FIFO_EVENT     (0x00010000UL)

/** The master write started by \ref Cy_SCB_I2C_MasterWrite is complete */
#define CY_SCB_I2C_MASTER_WR_CMPLT_EVENT       (0x00020000UL)

/** The master read started by \ref Cy_SCB_I2C_MasterRead is complete */
#define CY_SCB_I2C_MASTER_RD_CMPLT_EVENT       (0x00040000UL)

/**
* Indicates the I2C hardware has detected an error. It occurs together with
* \ref CY_SCB_I2C_MASTER_RD_CMPLT_EVENT or \ref CY_SCB_I2C_MASTER_WR_CMPLT_EVENT
* depends on the direction of the transfer.
* Check \ref Cy_SCB_I2C_MasterGetStatus to determine the source of the error.
*/
#define CY_SCB_I2C_MASTER_ERR_EVENT            (0x00080000UL)
/** \} group_scb_i2c_macros_callback_events */

/**
* \defgroup group_scb_i2c_macros_addr_callback_events I2C Address Callback Events
* Each event is encoded in a separate bit and therefore it is possible to
* notify about multiple events.
* \{
*/
/**
* Indicates the slave was addressed by the general call address
*/
#define CY_SCB_I2C_GENERAL_CALL_EVENT      (0x01UL)

/**
* The slave address is in the RX FIFO.
* Note that the address must be read from the RX FIFO using the
* \ref Cy_SCB_ReadRxFifo function.
*/
#define CY_SCB_I2C_ADDR_IN_FIFO_EVENT      (0x02UL)
/** \} group_scb_i2c_macros_addr_callback_events */

/**
* This value is returned by the slave when there is no data in the
* Read buffer
*/
#define CY_SCB_I2C_DEFAULT_TX  (0xFFUL)


/***************************************
*         Internal Constants
***************************************/

/** \cond INTERNAL */

/* Slave statuses */
#define CY_SCB_I2C_SLAVE_RD_CLEAR  (CY_SCB_I2C_SLAVE_RD_CMPLT  | CY_SCB_I2C_SLAVE_RD_IN_FIFO | \
                                    CY_SCB_I2C_SLAVE_RD_UNDRFL | CY_SCB_I2C_SLAVE_ARB_LOST   | \
                                    CY_SCB_I2C_SLAVE_BUS_ERR)

#define CY_SCB_I2C_SLAVE_WR_CLEAR  (CY_SCB_I2C_SLAVE_WR_CMPLT | CY_SCB_I2C_SLAVE_WR_OVRFL | \
                                    CY_SCB_I2C_SLAVE_ARB_LOST | CY_SCB_I2C_SLAVE_BUS_ERR)

/* Master error statuses */
#define CY_SCB_I2C_MASTER_ERR (CY_SCB_I2C_MASTER_ABORT_START | CY_SCB_I2C_MASTER_ADDR_NAK | \
                               CY_SCB_I2C_MASTER_DATA_NAK    | CY_SCB_I2C_MASTER_BUS_ERR  | \
                               CY_SCB_I2C_MASTER_ARB_LOST)

/* Master interrupt masks */
#define CY_SCB_I2C_MASTER_INTR     (CY_SCB_MASTER_INTR_I2C_ARB_LOST | CY_SCB_MASTER_INTR_I2C_BUS_ERROR | \
                                    CY_SCB_MASTER_INTR_I2C_NACK     | CY_SCB_MASTER_INTR_I2C_STOP)

#define CY_SCB_I2C_MASTER_INTR_ALL   (CY_SCB_I2C_MASTER_INTR | CY_SCB_MASTER_INTR_I2C_ACK)

#define CY_SCB_I2C_MASTER_INTR_ERR   (CY_SCB_MASTER_INTR_I2C_BUS_ERROR | CY_SCB_MASTER_INTR_I2C_ARB_LOST)

#define CY_SCB_I2C_MASTER_INTR_CMPLT (CY_SCB_I2C_MASTER_INTR_ERR | CY_SCB_MASTER_INTR_I2C_STOP)

/* Master statuses. */
#define CY_SCB_I2C_MASTER_TX_BYTE_DONE (CY_SCB_MASTER_INTR_I2C_ACK       | CY_SCB_MASTER_INTR_I2C_NACK | \
                                        CY_SCB_MASTER_INTR_I2C_BUS_ERROR | CY_SCB_MASTER_INTR_I2C_ARB_LOST)

#define CY_SCB_I2C_MASTER_RX_BYTE_DONE (CY_SCB_MASTER_INTR_I2C_BUS_ERROR | CY_SCB_MASTER_INTR_I2C_ARB_LOST)

#define CY_SCB_I2C_MASTER_STOP_DONE    (CY_SCB_MASTER_INTR_I2C_STOP      | \
                                        CY_SCB_MASTER_INTR_I2C_BUS_ERROR | CY_SCB_MASTER_INTR_I2C_ARB_LOST)

#define CY_SCB_I2C_MASTER_TIMEOUT_DONE (0x80000000UL)

/* The slave interrupt mask */
#define CY_SCB_I2C_SLAVE_INTR      (CY_SCB_SLAVE_INTR_I2C_ADDR_MATCH | CY_SCB_SLAVE_INTR_I2C_GENERAL_ADDR | \
                                    CY_SCB_SLAVE_INTR_I2C_STOP       | CY_SCB_SLAVE_INTR_I2C_BUS_ERROR    | \
                                    CY_SCB_SLAVE_INTR_I2C_ARB_LOST)

#define CY_SCB_I2C_SLAVE_INTR_NO_STOP   (CY_SCB_I2C_SLAVE_INTR & (uint32_t) ~CY_SCB_SLAVE_INTR_I2C_STOP)

#define CY_SCB_I2C_SLAVE_INTR_ADDR      (CY_SCB_SLAVE_INTR_I2C_ADDR_MATCH | CY_SCB_SLAVE_INTR_I2C_GENERAL_ADDR)

#define CY_SCB_I2C_SLAVE_ADDR_DONE      (CY_SCB_I2C_SLAVE_INTR_ADDR)

#define CY_SCB_I2C_SLAVE_INTR_NO_ADDR   (CY_SCB_I2C_SLAVE_INTR & (uint32_t) ~CY_SCB_I2C_SLAVE_INTR_ADDR)

#define CY_SCB_I2C_SLAVE_INTR_TX        (CY_SCB_TX_INTR_LEVEL | CY_SCB_TX_INTR_UNDERFLOW)

#define CY_SCB_I2C_SLAVE_INTR_ERROR     (CY_SCB_SLAVE_INTR_I2C_BUS_ERROR | CY_SCB_SLAVE_INTR_I2C_ARB_LOST)

/* I2C states */
#define CY_SCB_I2C_IDLE              (0x10000000UL)
#define CY_SCB_I2C_IDLE_MASK         (0x10000000UL)

/* Master states */
#define CY_SCB_I2C_MASTER_ACTIVE     (0x00100000UL)
#define CY_SCB_I2C_MASTER_WAIT       (0x10100000UL)
#define CY_SCB_I2C_MASTER_RX0        (0x00110000UL)
#define CY_SCB_I2C_MASTER_RX1        (0x00120000UL)
#define CY_SCB_I2C_MASTER_ADDR       (0x10130000UL)
#define CY_SCB_I2C_MASTER_TX         (0x00140000UL)
#define CY_SCB_I2C_MASTER_TX_DONE    (0x00150000UL)
#define CY_SCB_I2C_MASTER_STOP       (0x00160000UL)
#define CY_SCB_I2C_MASTER_WAIT_STOP  (0x00170000UL)
#define CY_SCB_I2C_MASTER_CMPLT      (0x00180000UL)

/* Slave states */
#define CY_SCB_I2C_SLAVE_ACTIVE      (0x00001000UL)
#define CY_SCB_I2C_SLAVE_RX_MASK     (0x00001001UL)
#define CY_SCB_I2C_SLAVE_RX          (0x00001001UL)
#define CY_SCB_I2C_SLAVE_TX          (0x00001002UL)

/* FIFO size */
#define CY_SCB_I2C_FIFO_SIZE          CY_SCB_FIFO_SIZE
#define CY_SCB_I2C_HALF_FIFO_SIZE     (CY_SCB_FIFO_SIZE / 2UL)

#define CY_SCB_I2C_DEFAULT_RETURN    (0xFFUL)

/* Convert the timeout in milliseconds to microseconds */
#define CY_SCB_I2C_CONVERT_TIMEOUT_TO_US(timeoutMs)     ((timeoutMs) * 1000UL)

/* I2C data rates max (Hz): standard, fast and fast plus modes */
#define CY_SCB_I2C_STD_DATA_RATE    (100000U)
#define CY_SCB_I2C_FST_DATA_RATE    (400000U)
#define CY_SCB_I2C_FSTP_DATA_RATE  (1000000U)

/* Slave clock limits */
#define CY_SCB_I2C_SLAVE_STD_CLK_MIN    (1550000U)
#define CY_SCB_I2C_SLAVE_STD_CLK_MAX   (12800000U)
#define CY_SCB_I2C_SLAVE_FST_CLK_MIN    (7820000U)
#define CY_SCB_I2C_SLAVE_FST_CLK_MAX   (15380000U)
#define CY_SCB_I2C_SLAVE_FSTP_CLK_MIN  (15840000U)
#define CY_SCB_I2C_SLAVE_FSTP_CLK_MAX  (89000000U)

/* Master clock (Hz) and duty cycle limits for standard mode */
#define CY_SCB_I2C_MASTER_STD_CLK_MIN           (1550000U)
#define CY_SCB_I2C_MASTER_STD_CLK_MAX           (3200000U)
#define CY_SCB_I2C_MASTER_STD_LOW_PHASE_MIN     (8U)
#define CY_SCB_I2C_MASTER_STD_HIGH_PHASE_MIN    (8U)

/* Master clock (Hz) and duty cycle limits for fast mode */
#define CY_SCB_I2C_MASTER_FST_CLK_MIN            (7820000U)
#define CY_SCB_I2C_MASTER_FST_CLK_MAX           (10000000U)
#define CY_SCB_I2C_MASTER_FST_LOW_PHASE_MIN     (13U)
#define CY_SCB_I2C_MASTER_FST_HIGH_PHASE_MIN    (8U)

/* Master clock (Hz) and duty cycle limits for fast plus mode */
#define CY_SCB_I2C_MASTER_FSTP_CLK_MIN          (14320000U)
#define CY_SCB_I2C_MASTER_FSTP_CLK_MAX          (25800000U)
#define CY_SCB_I2C_MASTER_FSTP_LOW_PHASE_MIN    (9U)
#define CY_SCB_I2C_MASTER_FSTP_HIGH_PHASE_MIN   (6U)

/* SCL low and high time in ns. Takes into account tF and tR */
#define CY_SCB_I2C_MASTER_STD_SCL_LOW   (5000U) /* tLOW  + tF = 4700 + 300  */
#define CY_SCB_I2C_MASTER_STD_SCL_HIGH  (5000U) /* tHIGH + tR = 4000 + 1000 */
#define CY_SCB_I2C_MASTER_FST_SCL_LOW   (1600U) /* tLOW  + tF = 1300 + 300  */
#define CY_SCB_I2C_MASTER_FST_SCL_HIGH   (900U) /* tHIGH + tR = 600 + 300   */
#define CY_SCB_I2C_MASTER_FSTP_SCL_LOW   (620U) /* tLOW  + tF = 500 + 120   */
#define CY_SCB_I2C_MASTER_FSTP_SCL_HIGH  (380U) /* tHIGH + tR = 260 + 120   */

/* Master duty cycle limits */
#define CY_SCB_I2C_LOW_PHASE_MAX    (16U)
#define CY_SCB_I2C_HIGH_PHASE_MAX   (16U)
#define CY_SCB_I2C_DUTY_CYCLE_MAX   (CY_SCB_I2C_LOW_PHASE_MAX + CY_SCB_I2C_HIGH_PHASE_MAX)

/* Analog filter settings. */
#define CY_SCB_I2C_ENABLE_ANALOG_FITLER    (CY_SCB_I2C_CFG_DEF_VAL)
#define CY_SCB_I2C_DISABLE_ANALOG_FITLER   (CY_SCB_I2C_CFG_DEF_VAL & \
                                            (uint32_t) ~(SCB_I2C_CFG_SDA_IN_FILT_SEL_Msk | \
                                                         SCB_I2C_CFG_SCL_IN_FILT_SEL_Msk))

#define CY_SCB_I2C_IS_MODE_VALID(mode)      ( (CY_SCB_I2C_SLAVE  == (mode)) || \
                                              (CY_SCB_I2C_MASTER == (mode)) || \
                                              (CY_SCB_I2C_MASTER_SLAVE == (mode)) )

#define CY_SCB_I2C_IS_RW_BIT_VALID(dir)     ( (CY_SCB_I2C_WRITE_XFER == (dir)) || \
                                              (CY_SCB_I2C_READ_XFER  == (dir)) )

#define CY_SCB_I2C_IS_RESPONSE_VALID(cmd)   ( (CY_SCB_I2C_ACK == (cmd))  || \
                                              (CY_SCB_I2C_NAK == (cmd)) )

#define CY_SCB_I2C_IS_ADDR_MASK_VALID(mask)         ( (0U == ((mask) & 0x01U)) )

#define CY_SCB_I2C_IS_DATA_RATE_VALID(dataRateHz)   ( ((dataRateHz) > 0UL) && \
                                                      ((dataRateHz) <= CY_SCB_I2C_FSTP_DATA_RATE) )

#define CY_SCB_I2C_IS_TIMEOUT_VALID(timeoutMs)              ( (timeoutMs) <= (0xFFFFFFFFUL / 1000UL) )
#define CY_SCB_I2C_IS_LOW_PHASE_CYCLES_VALID(clockCycles)   ( ((clockCycles) >= 7UL) && ((clockCycles) <= 16UL) )
#define CY_SCB_I2C_IS_HIGH_PHASE_CYCLES_VALID(clockCycles)  ( ((clockCycles) >= 5UL) && ((clockCycles) <= 16UL) )

/** \endcond */

/** \} group_scb_i2c_macros */


/***************************************
*    In-line Function Implementation
***************************************/

/**
* \addtogroup group_scb_i2c_general_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_SCB_I2C_Enable
****************************************************************************//**
*
* Enables the SCB block for the I2C operation
*
* \param base
* The pointer to the I2C SCB instance.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_I2C_Enable(CySCB_Type *base)
{
    base->CTRL |= SCB_CTRL_ENABLED_Msk;
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_IsBusBusy
****************************************************************************//**
*
* Checks whether the I2C bus is busy.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \return
* A bus status: busy or not busy.
*
* \note
* After the SCB block is enabled or reset, the valid bus busy-status returns
* after half of the SCL period.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SCB_I2C_IsBusBusy(CySCB_Type const *base)
{
    return _FLD2BOOL(SCB_I2C_STATUS_BUS_BUSY, base->I2C_STATUS);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveSetAddress
****************************************************************************//**
*
* Sets the slave address for the I2C slave.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param addr
* The 7-bit right justified slave address.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_I2C_SlaveSetAddress(CySCB_Type *base, uint8_t addr)
{
    CY_ASSERT_L2(CY_SCB_IS_I2C_ADDR_VALID(addr));

    base->RX_MATCH = _CLR_SET_FLD32U(base->RX_MATCH, SCB_RX_MATCH_ADDR, ((uint32_t)((uint32_t) addr << 1UL)));
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveGetAddress
****************************************************************************//**
*
* Returns the slave address of the I2C slave.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \return
* The 7-bit right justified slave address.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_I2C_SlaveGetAddress(CySCB_Type const *base)
{
    return (_FLD2VAL(SCB_RX_MATCH_ADDR, base->RX_MATCH) >> 1UL);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveSetAddressMask
****************************************************************************//**
*
* Sets the slave address mask for the I2C slave. The LSBit must always be 0.
* In all other bit positions a 1 indicates that the incoming address must match
* the corresponding bit in the slave address. A 0 in the mask means that the
* incoming address does not need to match.
* Example Slave Address = 0x0C. Slave Address Mask = 0x08. This means that the
* hardware will accept both 0x08 and 0x0C as valid addresses.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param addrMask
* The 8-bit address mask, the upper 7 bits correspond to the slave address.
* LSBit must always be 0.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_I2C_SlaveSetAddressMask(CySCB_Type *base, uint8_t addrMask)
{
    CY_ASSERT_L2(CY_SCB_I2C_IS_ADDR_MASK_VALID(addrMask));

    base->RX_MATCH = _CLR_SET_FLD32U(base->RX_MATCH, SCB_RX_MATCH_MASK, ((uint32_t) addrMask));
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_SlaveGetAddressMask
****************************************************************************//**
*
* Returns the slave address mask.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \return
* The 8-bit address mask, the upper 7 bits correspond to the slave address.
* LSBit must always be 0.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_I2C_SlaveGetAddressMask(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_RX_MATCH_MASK, base->RX_MATCH);
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterSetLowPhaseDutyCycle
****************************************************************************//**
*
* This function sets the number of SCB clock cycles in the low phase of SCL.
* If \ref Cy_SCB_I2C_SetDataRate is called after this function, the values
* specified in this function are overwritten.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param clockCycles
* The number of SCB clock cycles in the low phase of SCL.
* The valid range is 7 to 16.
*
* \note
* This function should be used at your own risk. Changing the number of clock
* cycles in a phase of SCL may violate the I2C specification. Make this
* change only while the block is disabled.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_I2C_MasterSetLowPhaseDutyCycle(CySCB_Type *base, uint32_t clockCycles)
{
    CY_ASSERT_L2(CY_SCB_I2C_IS_LOW_PHASE_CYCLES_VALID(clockCycles));

    base->I2C_CTRL = _CLR_SET_FLD32U(base->I2C_CTRL, SCB_I2C_CTRL_LOW_PHASE_OVS, (clockCycles - 1UL));
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_MasterSetHighPhaseDutyCycle
****************************************************************************//**
*
* This function sets the number of SCB clock cycles in the high phase of SCL.
* If \ref Cy_SCB_I2C_SetDataRate is called after this function, the values
* specified in this function get overwritten.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param clockCycles
* The number of SCB clock cycles in the high phase of SCL.
* The valid range is 5 to 16.
*
* \note
* This function should be used at your own risk. Changing the number of clock
* cycles in a phase of SCL may violate the I2C specification. Make this
* change only while the block is disabled.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_I2C_MasterSetHighPhaseDutyCycle(CySCB_Type *base, uint32_t clockCycles)
{
    CY_ASSERT_L2(CY_SCB_I2C_IS_HIGH_PHASE_CYCLES_VALID(clockCycles));

    base->I2C_CTRL = _CLR_SET_FLD32U(base->I2C_CTRL, SCB_I2C_CTRL_HIGH_PHASE_OVS, (clockCycles - 1UL));
}
/** \} group_scb_i2c_general_functions */

/**
* \addtogroup group_scb_i2c_interrupt_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_SCB_I2C_RegisterEventCallback
****************************************************************************//**
*
* Registers a callback function that notifies that
* \ref group_scb_i2c_macros_callback_events occurred in the
* \ref Cy_SCB_I2C_Interrupt.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param callback
* The pointer to a callback function.
* See \ref cy_cb_scb_i2c_handle_events_t for the function prototype.
*
* \param context
* The pointer to context structure \ref cy_stc_scb_i2c_context_t allocated by
* the user. The structure is used while the I2C operation for internal
* configuration and data retention. The user should not modify anything in
* this structure.
*
* \note
* To remove the callback, pass NULL as the pointer to the callback function.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_I2C_RegisterEventCallback(CySCB_Type const *base,
            cy_cb_scb_i2c_handle_events_t callback, cy_stc_scb_i2c_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    context->cbEvents = callback;
}


/*******************************************************************************
* Function Name: Cy_SCB_I2C_RegisterAddrCallback
****************************************************************************//**
*
* Registers a callback function that notifies that
* \ref group_scb_i2c_macros_addr_callback_events occurred in the
* \ref Cy_SCB_I2C_Interrupt.
*
* \param base
* The pointer to the I2C SCB instance.
*
* \param callback
* The pointer to a callback function.
* See \ref cy_cb_scb_i2c_handle_addr_t for the function prototype.
*
* \param context
* The pointer to context structure \ref cy_stc_scb_i2c_context_t allocated by
* the user. The structure is used during the I2C operation for internal
* configuration and data retention. The user should not modify anything in
* this structure.
*
* \note
* To remove the callback, pass NULL as the pointer to a callback function.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_I2C_RegisterAddrCallback(CySCB_Type const *base,
              cy_cb_scb_i2c_handle_addr_t callback, cy_stc_scb_i2c_context_t *context)
{
    /* Suppress a compiler warning about unused variables */
    (void) base;

    context->cbAddr = callback;
}
/** \} group_scb_i2c_interrupt_functions */

#if defined(__cplusplus)
}
#endif

/** \} group_scb_i2c */

#endif /* (CY_SCB_I2C_H) */


/* [] END OF FILE */

