/***************************************************************************//**
* \file cy_scb_common.h
* \version 2.20
*
* Provides common API declarations of the SCB driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_scb Serial Communication Block (SCB)
* \{
* \defgroup group_scb_common Common
* \defgroup group_scb_ezi2c  EZI2C (SCB)
* \defgroup group_scb_i2c    I2C (SCB)
* \defgroup group_scb_spi    SPI (SCB)
* \defgroup group_scb_uart   UART (SCB)
* \} */

/**
* \addtogroup group_scb_common
* \{
*
* Common API for the Serial Communication Block.
*
* This is the common API that provides an interface to the SCB hardware.
* The I2C, SPI, and UART drivers use this common API.
* Most users will use individual drivers and do not need to use the common
* API for the SCB. However, you can use the common SCB API to implement
* a custom driver based on the SCB hardware.
*
* \section group_scb_common_configuration Configuration Considerations
*
* This is not a driver and it does not require configuration.
*
* \section group_scb_common_more_information More Information
*
* Refer to the SCB chapter of the technical reference manual (TRM).
*
* \section group_scb_common_MISRA MISRA-C Compliance
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
*     <td>A cast should not be performed between a pointer to object type and
*         a different pointer to object type.</td>
*     <td>The pointer to the buffer memory is void to allow handling of
*         different data types: uint8_t (4-8 bits) or uint16_t (9-16 bits).
*         The cast operation is safe because the configuration is verified
*         before operation is performed.
*         </td>
*   </tr>
* </table>
*
* \section group_scb_common_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>2.20</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>2.10</td>
*     <td>None.</td>
*     <td>SCB I2C driver updated.</td>
*   </tr>
*   <tr>
*     <td rowspan="2"> 2.0</td>
*     <td>Added parameters validation for public API.
*     <td></td>
*   </tr>
*   <tr>
*     <td>Fixed functions which return interrupt status to return only defined
*         set of interrupt statuses.</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version.</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_scb_common_macros Macros
* \defgroup group_scb_common_functions Functions
* \defgroup group_scb_common_data_structures Data Structures
*
*/

#if !defined(CY_SCB_COMMON_H)
#define CY_SCB_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cyip_scb.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_syspm.h"

#ifdef CY_IP_MXSCB

#if defined(__cplusplus)
extern "C" {
#endif


/***************************************
*        Function Prototypes
***************************************/

/**
* \addtogroup group_scb_common_functions
* \{
*/
__STATIC_INLINE uint32_t Cy_SCB_ReadRxFifo    (CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_SetRxFifoLevel(CySCB_Type *base, uint32_t level);
__STATIC_INLINE uint32_t Cy_SCB_GetNumInRxFifo(CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetRxSrValid  (CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearRxFifo   (CySCB_Type *base);

__STATIC_INLINE void     Cy_SCB_WriteTxFifo   (CySCB_Type *base, uint32_t data);
__STATIC_INLINE void     Cy_SCB_SetTxFifoLevel(CySCB_Type *base, uint32_t level);
__STATIC_INLINE uint32_t Cy_SCB_GetNumInTxFifo(CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetTxSrValid  (CySCB_Type const *base);
__STATIC_INLINE bool     Cy_SCB_IsTxComplete  (CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearTxFifo   (CySCB_Type *base);

__STATIC_INLINE void     Cy_SCB_SetByteMode(CySCB_Type *base, bool byteMode);

__STATIC_INLINE uint32_t Cy_SCB_GetInterruptCause(CySCB_Type const *base);

__STATIC_INLINE uint32_t Cy_SCB_GetRxInterruptStatus(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_SetRxInterruptMask  (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SCB_GetRxInterruptMask  (CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetRxInterruptStatusMasked(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearRxInterrupt    (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE void     Cy_SCB_SetRxInterrupt      (CySCB_Type *base, uint32_t interruptMask);

__STATIC_INLINE uint32_t Cy_SCB_GetTxInterruptStatus(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_SetTxInterruptMask  (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SCB_GetTxInterruptMask  (CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetTxInterruptStatusMasked(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearTxInterrupt    (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE void     Cy_SCB_SetTxInterrupt      (CySCB_Type *base, uint32_t interruptMask);

__STATIC_INLINE uint32_t Cy_SCB_GetMasterInterruptStatus(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_SetMasterInterruptMask  (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SCB_GetMasterInterruptMask  (CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetMasterInterruptStatusMasked(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearMasterInterrupt    (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE void     Cy_SCB_SetMasterInterrupt      (CySCB_Type *base, uint32_t interruptMask);

__STATIC_INLINE uint32_t Cy_SCB_GetSlaveInterruptStatus(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_SetSlaveInterruptMask  (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SCB_GetSlaveInterruptMask  (CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetSlaveInterruptStatusMasked(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearSlaveInterrupt    (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE void     Cy_SCB_SetSlaveInterrupt      (CySCB_Type *base, uint32_t interruptMask);

__STATIC_INLINE uint32_t Cy_SCB_GetI2CInterruptStatus(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_SetI2CInterruptMask  (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SCB_GetI2CInterruptMask  (CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetI2CInterruptStatusMasked(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearI2CInterrupt    (CySCB_Type *base, uint32_t interruptMask);

__STATIC_INLINE uint32_t Cy_SCB_GetSpiInterruptStatus(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_SetSpiInterruptMask  (CySCB_Type *base, uint32_t interruptMask);
__STATIC_INLINE uint32_t Cy_SCB_GetSpiInterruptMask  (CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetSpiInterruptStatusMasked(CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_ClearSpiInterrupt    (CySCB_Type *base, uint32_t interruptMask);


/***************************************
*     Internal Function Prototypes
***************************************/

/** \cond INTERNAL */
void     Cy_SCB_ReadArrayNoCheck  (CySCB_Type const *base, void *buffer, uint32_t size);
uint32_t Cy_SCB_ReadArray         (CySCB_Type const *base, void *buffer, uint32_t size);
void     Cy_SCB_ReadArrayBlocking (CySCB_Type const *base, void *buffer, uint32_t size);
uint32_t Cy_SCB_Write             (CySCB_Type *base, uint32_t data);
void     Cy_SCB_WriteArrayNoCheck (CySCB_Type *base, void *buffer, uint32_t size);
uint32_t Cy_SCB_WriteArray        (CySCB_Type *base, void *buffer, uint32_t size);
void     Cy_SCB_WriteArrayBlocking(CySCB_Type *base, void *buffer, uint32_t size);
void     Cy_SCB_WriteString       (CySCB_Type *base, char_t const string[]);
void     Cy_SCB_WriteDefaultArrayNoCheck(CySCB_Type *base, uint32_t txData, uint32_t size);
uint32_t Cy_SCB_WriteDefaultArray (CySCB_Type *base, uint32_t txData, uint32_t size);

__STATIC_INLINE uint32_t Cy_SCB_GetFifoSize (CySCB_Type const *base);
__STATIC_INLINE void     Cy_SCB_FwBlockReset(CySCB_Type *base);
__STATIC_INLINE bool     Cy_SCB_IsRxDataWidthByte(CySCB_Type const *base);
__STATIC_INLINE bool     Cy_SCB_IsTxDataWidthByte(CySCB_Type const *base);
__STATIC_INLINE uint32_t Cy_SCB_GetRxFifoLevel   (CySCB_Type const *base);
/** \endcond */

/** \} group_scb_common_functions */

/***************************************
*            API Constants
***************************************/

/**
* \addtogroup group_scb_common_macros
* \{
*/

/** Driver major version */
#define CY_SCB_DRV_VERSION_MAJOR    (2)

/** Driver minor version */
#define CY_SCB_DRV_VERSION_MINOR    (20)

/** SCB driver identifier */
#define CY_SCB_ID           CY_PDL_DRV_ID(0x2AU)

/** Position for SCB driver sub mode */
#define CY_SCB_SUB_MODE_Pos (13UL)

/** EZI2C mode identifier */
#define CY_SCB_EZI2C_ID     (0x0UL << CY_SCB_SUB_MODE_Pos)

/** EZI2C mode identifier */
#define CY_SCB_I2C_ID       (0x1UL << CY_SCB_SUB_MODE_Pos)

/** EZI2C mode identifier */
#define CY_SCB_SPI_ID       (0x2UL << CY_SCB_SUB_MODE_Pos)

/** EZI2C mode identifier */
#define CY_SCB_UART_ID      (0x3UL << CY_SCB_SUB_MODE_Pos)

/**
* \defgroup group_scb_common_macros_intr_cause SCB Interrupt Causes
* \{
*/
/** Interrupt from Master interrupt sources */
#define CY_SCB_MASTER_INTR SCB_INTR_CAUSE_M_Msk

/** Interrupt from Slave interrupt sources  */
#define CY_SCB_SLAVE_INTR  SCB_INTR_CAUSE_S_Msk

/** Interrupt from TX interrupt sources */
#define CY_SCB_TX_INTR     SCB_INTR_CAUSE_TX_Msk

/** Interrupt from RX interrupt sources */
#define CY_SCB_RX_INTR     SCB_INTR_CAUSE_RX_Msk

/** Interrupt from I2C externally clocked interrupt sources */
#define CY_SCB_I2C_INTR    SCB_INTR_CAUSE_I2C_EC_Msk

/** Interrupt from SPI externally clocked interrupt sources */
#define CY_SCB_SPI_INTR    SCB_INTR_CAUSE_SPI_EC_Msk
/** \} group_scb_common_macros_intr_cause */

/**
* \defgroup group_scb_common_macros_tx_intr TX Interrupt Statuses
* \{
*/
/**
* The number of data elements in the TX FIFO is less than the value
* of the TX FIFO level
*/
#define CY_SCB_TX_INTR_LEVEL           SCB_INTR_TX_TRIGGER_Msk

/** The TX FIFO is not full */
#define CY_SCB_TX_INTR_NOT_FULL        SCB_INTR_TX_NOT_FULL_Msk

/** The TX FIFO is empty */
#define CY_SCB_TX_INTR_EMPTY           SCB_INTR_TX_EMPTY_Msk

/** An attempt to write to a full TX FIFO */
#define CY_SCB_TX_INTR_OVERFLOW        SCB_INTR_TX_OVERFLOW_Msk

/** An attempt to read from an empty TX FIFO */
#define CY_SCB_TX_INTR_UNDERFLOW       SCB_INTR_TX_UNDERFLOW_Msk

/** The UART transfer is complete: all data elements from the TX FIFO are sent
*/
#define CY_SCB_TX_INTR_UART_DONE       SCB_INTR_TX_UART_DONE_Msk

/** SmartCard only: UART received a NACK */
#define CY_SCB_TX_INTR_UART_NACK       SCB_INTR_TX_UART_NACK_Msk

/**
* SmartCard only: the value on the TX line of the UART does not match the
* value on the RX line
*/
#define CY_SCB_TX_INTR_UART_ARB_LOST   SCB_INTR_TX_UART_ARB_LOST_Msk
/** \} group_scb_common_macros_tx_intr */

/**
* \defgroup group_scb_common_macros_rx_intr RX Interrupt Statuses
* \{
*/
/**
* The number of data elements in the RX FIFO is greater than the value of the
* RX FIFO level
*/
#define CY_SCB_RX_INTR_LEVEL             SCB_INTR_RX_TRIGGER_Msk

/** The RX FIFO is not empty */
#define CY_SCB_RX_INTR_NOT_EMPTY         SCB_INTR_RX_NOT_EMPTY_Msk

/** The RX FIFO is full */
#define CY_SCB_RX_INTR_FULL              SCB_INTR_RX_FULL_Msk

/** An attempt to write to a full RX FIFO */
#define CY_SCB_RX_INTR_OVERFLOW          SCB_INTR_RX_OVERFLOW_Msk

/** An attempt to read from an empty RX FIFO */
#define CY_SCB_RX_INTR_UNDERFLOW         SCB_INTR_RX_UNDERFLOW_Msk

/** A UART framing error detected */
#define CY_SCB_RX_INTR_UART_FRAME_ERROR  SCB_INTR_RX_FRAME_ERROR_Msk

/** A UART parity error detected  */
#define CY_SCB_RX_INTR_UART_PARITY_ERROR SCB_INTR_RX_PARITY_ERROR_Msk

/** A UART break detected */
#define CY_SCB_RX_INTR_UART_BREAK_DETECT SCB_INTR_RX_BREAK_DETECT_Msk
/** \} group_scb_common_macros_rx_intr */

/**
* \defgroup group_scb_common_macros_slave_intr Slave Interrupt Statuses
* \{
*/
/**
* I2C slave lost arbitration: the value driven on the SDA line is not the same
* as the value observed on the SDA line
*/
#define CY_SCB_SLAVE_INTR_I2C_ARB_LOST      SCB_INTR_S_I2C_ARB_LOST_Msk

/** The I2C slave received a NAK */
#define CY_SCB_SLAVE_INTR_I2C_NACK          SCB_INTR_S_I2C_NACK_Msk

/** The I2C slave received  an ACK */
#define CY_SCB_SLAVE_INTR_I2C_ACK           SCB_INTR_S_I2C_ACK_Msk

/**
* A Stop or Repeated Start event for a write transfer intended for this slave
* was detected.
*/
#define CY_SCB_SLAVE_INTR_I2C_WRITE_STOP    SCB_INTR_S_I2C_WRITE_STOP_Msk

/** A Stop or Repeated Start event intended for this slave was detected */
#define CY_SCB_SLAVE_INTR_I2C_STOP          SCB_INTR_S_I2C_STOP_Msk

/** The I2C slave received a Start condition */
#define CY_SCB_SLAVE_INTR_I2C_START         SCB_INTR_S_I2C_START_Msk

/** The I2C slave received the matching address */
#define CY_SCB_SLAVE_INTR_I2C_ADDR_MATCH    SCB_INTR_S_I2C_ADDR_MATCH_Msk

/** The I2C Slave received the general call address */
#define CY_SCB_SLAVE_INTR_I2C_GENERAL_ADDR  SCB_INTR_S_I2C_GENERAL_Msk

/** The I2C slave bus error (detection of unexpected Start or Stop condition) */
#define CY_SCB_SLAVE_INTR_I2C_BUS_ERROR     SCB_INTR_S_I2C_BUS_ERROR_Msk

/**
* The SPI slave select line is deselected at an expected time during an
* SPI transfer.
*/
#define CY_SCB_SLAVE_INTR_SPI_BUS_ERROR     SCB_INTR_S_SPI_BUS_ERROR_Msk
/** \} group_scb_common_macros_slave_intr */

/**
* \defgroup group_scb_common_macros_master_intr Master Interrupt Statuses
* \{
*/
/** The I2C master lost arbitration */
#define CY_SCB_MASTER_INTR_I2C_ARB_LOST    SCB_INTR_M_I2C_ARB_LOST_Msk

/** The I2C master received a NACK */
#define CY_SCB_MASTER_INTR_I2C_NACK        SCB_INTR_M_I2C_NACK_Msk

/** The I2C master received an ACK */
#define CY_SCB_MASTER_INTR_I2C_ACK         SCB_INTR_M_I2C_ACK_Msk

/** The I2C master generated a Stop */
#define CY_SCB_MASTER_INTR_I2C_STOP        SCB_INTR_M_I2C_STOP_Msk

/** The I2C master bus error (detection of unexpected START or STOP condition)
*/
#define CY_SCB_MASTER_INTR_I2C_BUS_ERROR   SCB_INTR_M_I2C_BUS_ERROR_Msk

/**
* The SPI master transfer is complete: all data elements transferred from the
* TX FIFO and TX shift register.
*/
#define CY_SCB_MASTER_INTR_SPI_DONE        SCB_INTR_M_SPI_DONE_Msk
/** \} group_scb_common_macros_master_intr */

/**
* \defgroup group_scb_common_macros_i2c_intr I2C Interrupt Statuses
* \{
*/
/**
* Wake up request: the I2C slave received the matching address.
* Note that this interrupt source triggers in active mode.
*/
#define CY_SCB_I2C_INTR_WAKEUP     SCB_INTR_I2C_EC_WAKE_UP_Msk
/** \} group_scb_common_macros_i2c_intr */

/**
* \defgroup group_scb_common_macros_SpiIntrStatuses SPI Interrupt Statuses
* \{
*/
/**
* Wake up request: the SPI slave detects an active edge of the slave select
* signal. Note that this interrupt source triggers in active mode.
*/
#define CY_SCB_SPI_INTR_WAKEUP     SCB_INTR_SPI_EC_WAKE_UP_Msk
/** \} group_scb_common_macros_SpiIntrStatuses */


/***************************************
*         Internal Constants
***************************************/

/** \cond INTERNAL */

/* Default registers values */
#define CY_SCB_CTRL_DEF_VAL         (_VAL2FLD(SCB_CTRL_OVS, 15UL) | \
                                     _VAL2FLD(SCB_CTRL_MODE, 3UL))

#define CY_SCB_I2C_CTRL_DEF_VAL     (_VAL2FLD(SCB_I2C_CTRL_HIGH_PHASE_OVS, 8UL)        | \
                                     _VAL2FLD(SCB_I2C_CTRL_HIGH_PHASE_OVS, 8UL)        | \
                                     _VAL2FLD(SCB_I2C_CTRL_M_READY_DATA_ACK, 1UL)      | \
                                     _VAL2FLD(SCB_I2C_CTRL_M_NOT_READY_DATA_NACK, 1UL) | \
                                     _VAL2FLD(SCB_I2C_CTRL_S_GENERAL_IGNORE, 1UL)      | \
                                     _VAL2FLD(SCB_I2C_CTRL_S_READY_ADDR_ACK, 1UL)      | \
                                     _VAL2FLD(SCB_I2C_CTRL_S_READY_DATA_ACK, 1UL)      | \
                                     _VAL2FLD(SCB_I2C_CTRL_S_NOT_READY_ADDR_NACK, 1UL) | \
                                     _VAL2FLD(SCB_I2C_CTRL_S_NOT_READY_DATA_NACK, 1UL))

#define CY_SCB_I2C_CFG_DEF_VAL      (_VAL2FLD(SCB_I2C_CFG_SDA_IN_FILT_TRIM, 3UL)   | \
                                     _VAL2FLD(SCB_I2C_CFG_SDA_IN_FILT_SEL, 1UL)    | \
                                     _VAL2FLD(SCB_I2C_CFG_SCL_IN_FILT_SEL, 1UL)    | \
                                     _VAL2FLD(SCB_I2C_CFG_SDA_OUT_FILT0_TRIM, 2UL) | \
                                     _VAL2FLD(SCB_I2C_CFG_SDA_OUT_FILT1_TRIM, 2UL) | \
                                     _VAL2FLD(SCB_I2C_CFG_SDA_OUT_FILT2_TRIM, 2UL))

#define CY_SCB_SPI_CTRL_DEF_VAL     _VAL2FLD(SCB_SPI_CTRL_MODE, 3UL)
#define CY_SCB_UART_CTRL_DEF_VAL    _VAL2FLD(SCB_UART_CTRL_MODE, 3UL)

#define CY_SCB_UART_RX_CTRL_DEF_VAL (_VAL2FLD(SCB_UART_RX_CTRL_STOP_BITS, 2UL) | \
                                     _VAL2FLD(SCB_UART_RX_CTRL_BREAK_WIDTH, 10UL))

#define CY_SCB_UART_TX_CTRL_DEF_VAL _VAL2FLD(SCB_UART_TX_CTRL_STOP_BITS, 2UL)

#define CY_SCB_RX_CTRL_DEF_VAL      (_VAL2FLD(SCB_RX_CTRL_DATA_WIDTH, 7UL) | \
                                     _VAL2FLD(SCB_RX_CTRL_MSB_FIRST,  1UL))

#define CY_SCB_TX_CTRL_DEF_VAL      (_VAL2FLD(SCB_TX_CTRL_DATA_WIDTH, 7UL) | \
                                     _VAL2FLD(SCB_TX_CTRL_MSB_FIRST,  1UL))

/* SCB CTRL modes */
#define CY_SCB_CTRL_MODE_I2C   (0UL)
#define CY_SCB_CTRL_MODE_SPI   (1UL)
#define CY_SCB_CTRL_MODE_UART  (2UL)

/* The position and mask to set the I2C mode */
#define CY_SCB_I2C_CTRL_MODE_Pos    SCB_I2C_CTRL_SLAVE_MODE_Pos
#define CY_SCB_I2C_CTRL_MODE_Msk    (SCB_I2C_CTRL_SLAVE_MODE_Msk | \
                                     SCB_I2C_CTRL_MASTER_MODE_Msk)

/* Cypress ID #282226:
* SCB_I2C_CFG_SDA_IN_FILT_TRIM[1]: SCB clock enable (1), clock disable (0).
*/
#define CY_SCB_I2C_CFG_CLK_ENABLE_Msk  (_VAL2FLD(SCB_I2C_CFG_SDA_IN_FILT_TRIM, 2UL))

/* I2C has fixed data width */
#define CY_SCB_I2C_DATA_WIDTH   (7UL)

/* RX and TX control register values */
#define CY_SCB_I2C_RX_CTRL      (_VAL2FLD(SCB_RX_CTRL_DATA_WIDTH, CY_SCB_I2C_DATA_WIDTH) | \
                                 SCB_RX_CTRL_MSB_FIRST_Msk)
#define CY_SCB_I2C_TX_CTRL      (_VAL2FLD(SCB_TX_CTRL_DATA_WIDTH, CY_SCB_I2C_DATA_WIDTH) | \
                                 SCB_TX_CTRL_MSB_FIRST_Msk | SCB_TX_CTRL_OPEN_DRAIN_Msk)

/* The position and mask to make an address byte */
#define CY_SCB_I2C_ADDRESS_Pos  (1UL)
#define CY_SCB_I2C_ADDRESS_Msk  (0xFEUL)

/* SPI slave select polarity */
#define CY_SCB_SPI_CTRL_SSEL_POLARITY_Pos   SCB_SPI_CTRL_SSEL_POLARITY0_Pos
#define CY_SCB_SPI_CTRL_SSEL_POLARITY_Msk   (SCB_SPI_CTRL_SSEL_POLARITY0_Msk | \
                                             SCB_SPI_CTRL_SSEL_POLARITY1_Msk | \
                                             SCB_SPI_CTRL_SSEL_POLARITY2_Msk | \
                                             SCB_SPI_CTRL_SSEL_POLARITY3_Msk)

/* SPI clock modes: CPHA and CPOL */
#define CY_SCB_SPI_CTRL_CLK_MODE_Pos    SCB_SPI_CTRL_CPHA_Pos
#define CY_SCB_SPI_CTRL_CLK_MODE_Msk    (SCB_SPI_CTRL_CPHA_Msk | SCB_SPI_CTRL_CPOL_Msk)

/* UART parity and parity enable combination */
#define CY_SCB_UART_RX_CTRL_SET_PARITY_Msk      (SCB_UART_RX_CTRL_PARITY_ENABLED_Msk | \
                                                 SCB_UART_RX_CTRL_PARITY_Msk)
#define CY_SCB_UART_RX_CTRL_SET_PARITY_Pos      SCB_UART_RX_CTRL_PARITY_Pos

#define CY_SCB_UART_TX_CTRL_SET_PARITY_Msk      (SCB_UART_TX_CTRL_PARITY_ENABLED_Msk | \
                                                 SCB_UART_TX_CTRL_PARITY_Msk)
#define CY_SCB_UART_TX_CTRL_SET_PARITY_Pos      SCB_UART_TX_CTRL_PARITY_Pos

/* Max number of bits for byte mode */
#define CY_SCB_BYTE_WIDTH   (8UL)

/* Single unit to wait */
#define CY_SCB_WAIT_1_UNIT  (1U)

/* Clear interrupt sources */
#define CY_SCB_CLEAR_ALL_INTR_SRC   (0UL)

/* Hardware FIFO size: EZ_DATA_NR / 4 = (512 / 4) = 128 */
#define CY_SCB_FIFO_SIZE			(128UL)

/* Provides a list of allowed sources */
#define CY_SCB_TX_INTR_MASK     (CY_SCB_TX_INTR_LEVEL     | CY_SCB_TX_INTR_NOT_FULL  | CY_SCB_TX_INTR_EMPTY     | \
                                 CY_SCB_TX_INTR_OVERFLOW  | CY_SCB_TX_INTR_UNDERFLOW | CY_SCB_TX_INTR_UART_DONE | \
                                 CY_SCB_TX_INTR_UART_NACK | CY_SCB_TX_INTR_UART_ARB_LOST)

#define CY_SCB_RX_INTR_MASK     (CY_SCB_RX_INTR_LEVEL             | CY_SCB_RX_INTR_NOT_EMPTY | CY_SCB_RX_INTR_FULL | \
                                 CY_SCB_RX_INTR_OVERFLOW          | CY_SCB_RX_INTR_UNDERFLOW                       | \
                                 CY_SCB_RX_INTR_UART_FRAME_ERROR  | CY_SCB_RX_INTR_UART_PARITY_ERROR               | \
                                 CY_SCB_RX_INTR_UART_BREAK_DETECT)


#define CY_SCB_SLAVE_INTR_MASK  (CY_SCB_SLAVE_INTR_I2C_ARB_LOST   | CY_SCB_SLAVE_INTR_I2C_NACK | CY_SCB_SLAVE_INTR_I2C_ACK   | \
                                 CY_SCB_SLAVE_INTR_I2C_WRITE_STOP | CY_SCB_SLAVE_INTR_I2C_STOP | CY_SCB_SLAVE_INTR_I2C_START | \
                                 CY_SCB_SLAVE_INTR_I2C_ADDR_MATCH | CY_SCB_SLAVE_INTR_I2C_GENERAL_ADDR                       | \
                                 CY_SCB_SLAVE_INTR_I2C_BUS_ERROR  | CY_SCB_SLAVE_INTR_SPI_BUS_ERROR)

#define CY_SCB_MASTER_INTR_MASK (CY_SCB_MASTER_INTR_I2C_ARB_LOST  | CY_SCB_MASTER_INTR_I2C_NACK | \
                                 CY_SCB_MASTER_INTR_I2C_ACK       | CY_SCB_MASTER_INTR_I2C_STOP | \
                                 CY_SCB_MASTER_INTR_I2C_BUS_ERROR | CY_SCB_MASTER_INTR_SPI_DONE)

#define CY_SCB_I2C_INTR_MASK    CY_SCB_I2C_INTR_WAKEUP

#define CY_SCB_SPI_INTR_MASK    CY_SCB_SPI_INTR_WAKEUP

#define CY_SCB_IS_INTR_VALID(intr, mask)            ( 0UL == ((intr) & ((uint32_t) ~(mask))) )
#define CY_SCB_IS_TRIGGER_LEVEL_VALID(base, level)  ((level) < Cy_SCB_GetFifoSize(base))

#define CY_SCB_IS_I2C_ADDR_VALID(addr)              ( (0U == ((addr) & 0x80U)) )
#define CY_SCB_IS_BUFFER_VALID(buffer, size)        ( (NULL != (buffer)) && ((size) > 0UL) )
#define CY_SCB_IS_I2C_BUFFER_VALID(buffer, size)    ( (0UL == (size)) ? true : (NULL != (buffer)) )
/** \endcond */

/** \} group_scb_common_macros */


/***************************************
* Inline Function Implementation
***************************************/

/**
* \addtogroup group_scb_common_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_SCB_ReadRxFifo
****************************************************************************//**
*
* Reads a data element directly out of the RX FIFO.
* This function does not check whether the RX FIFO has data before reading it.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* Data from RX FIFO.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_ReadRxFifo(CySCB_Type const *base)
{
    return (base->RX_FIFO_RD);
}

/*******************************************************************************
* Function Name: Cy_SCB_SetRxFifoLevel
****************************************************************************//**
*
* Sets the RX FIFO level. When there are more data elements in the RX FIFO than
* this level, the RX FIFO level interrupt is triggered.
*
* \param base
* The pointer to the SCB instance.
*
* \param level
* When there are more data elements in the FIFO than this level, the RX level
* interrupt is triggered.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetRxFifoLevel(CySCB_Type *base, uint32_t level)
{
    CY_ASSERT_L2(CY_SCB_IS_TRIGGER_LEVEL_VALID(base, level));

    base->RX_FIFO_CTRL = _CLR_SET_FLD32U(base->RX_FIFO_CTRL, SCB_RX_FIFO_CTRL_TRIGGER_LEVEL, level);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetNumInRxFifo
****************************************************************************//**
*
* Returns the number of data elements currently in the RX FIFO.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The number or data elements in RX FIFO.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetNumInRxFifo(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_RX_FIFO_STATUS_USED, base->RX_FIFO_STATUS);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetRxSrValid
****************************************************************************//**
*
* Returns the status of the RX FIFO Shift Register valid bit.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* 1 - RX shift register valid; 0 - RX shift register not valid.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetRxSrValid(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_RX_FIFO_STATUS_SR_VALID, base->RX_FIFO_STATUS);
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearRxFifo
****************************************************************************//**
*
* Clears the RX FIFO and shifter.
*
* \param base
* The pointer to the SCB instance.
*
* \note
* If there is partial data in the shifter, it is cleared and lost.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearRxFifo(CySCB_Type* base)
{
    base->RX_FIFO_CTRL |= (uint32_t)  SCB_RX_FIFO_CTRL_CLEAR_Msk;
    base->RX_FIFO_CTRL &= (uint32_t) ~SCB_RX_FIFO_CTRL_CLEAR_Msk;

    (void) base->RX_FIFO_CTRL;
}


/*******************************************************************************
* Function Name: Cy_SCB_WriteTxFifo
****************************************************************************//**
*
* Writes data directly into the TX FIFO.
* This function does not check whether the TX FIFO is not full before writing
* into it.
*
* \param base
* The pointer to the SCB instance.
*
* \param data
* Data to write to the TX FIFO.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_WriteTxFifo(CySCB_Type* base, uint32_t data)
{
    base->TX_FIFO_WR = data;
}


/*******************************************************************************
* Function Name: Cy_SCB_SetTxFifoLevel
****************************************************************************//**
*
* Sets the TX FIFO level. When there are fewer data elements in the TX FIFO than
* this level, the TX FIFO level interrupt is triggered.
*
* \param base
* The pointer to the SCB instance.
*
* \param level
* When there are fewer data elements in the FIFO than this level, the TX level
* interrupt is triggered.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetTxFifoLevel(CySCB_Type *base, uint32_t level)
{
    CY_ASSERT_L2(CY_SCB_IS_TRIGGER_LEVEL_VALID(base, level));

    base->TX_FIFO_CTRL = _CLR_SET_FLD32U(base->TX_FIFO_CTRL, SCB_TX_FIFO_CTRL_TRIGGER_LEVEL, level);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetNumInTxFifo
****************************************************************************//**
*
* Returns the number of data elements currently in the TX FIFO.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The number or data elements in the TX FIFO.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetNumInTxFifo(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_TX_FIFO_STATUS_USED, base->TX_FIFO_STATUS);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetTxSrValid
****************************************************************************//**
*
* Returns the status of the TX FIFO Shift Register valid bit.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* 1 - TX shift register valid; 0 - TX shift register not valid.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetTxSrValid(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_TX_FIFO_STATUS_SR_VALID, base->TX_FIFO_STATUS);
}


/*******************************************************************************
* Function Name: Cy_SCB_IsTxComplete
****************************************************************************//**
*
* Checks whether the TX FIFO and Shifter are empty and there is no more data to send.
*
* \param base
* Pointer to SPI the SCB instance.
*
* \return
* If true, transmission complete. If false, transmission is not complete.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SCB_IsTxComplete(CySCB_Type const *base)
{
     return (0UL == (Cy_SCB_GetNumInTxFifo(base) + Cy_SCB_GetTxSrValid(base)));
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearTxFifo
****************************************************************************//**
*
* Clears the TX FIFO.
*
* \param base
* The pointer to the SCB instance.
*
* \note
* The TX FIFO clear operation also clears the shift register. Thus the shifter
* could be cleared in the middle of a data element transfer. Thia results in
* "ones" being sent on the bus for the remainder of the transfer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearTxFifo(CySCB_Type *base)
{
    base->TX_FIFO_CTRL |= (uint32_t)  SCB_TX_FIFO_CTRL_CLEAR_Msk;
    base->TX_FIFO_CTRL &= (uint32_t) ~SCB_TX_FIFO_CTRL_CLEAR_Msk;

    (void) base->TX_FIFO_CTRL;
}


/*******************************************************************************
* Function Name: Cy_SCB_SetByteMode
****************************************************************************//**
*
* Sets whether the RX and TX FIFOs are in byte mode.
* The FIFOs are either 16-bit wide or 8-bit wide (byte mode).
* When the FIFO is in byte mode it is twice as deep. See the device datasheet
* for FIFO depths.
*
* \param base
* The pointer to the SCB instance.
*
* \param byteMode
* If true, TX and RX FIFOs are 8-bit wide. If false, the FIFOs are 16-bit wide.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetByteMode(CySCB_Type *base, bool byteMode)
{
    if (byteMode)
    {
        base->CTRL |=  SCB_CTRL_BYTE_MODE_Msk;
    }
    else
    {
        base->CTRL &= ~SCB_CTRL_BYTE_MODE_Msk;
    }
}


/*******************************************************************************
* Function Name: Cy_SCB_GetInterruptCause
****************************************************************************//**
*
* Returns the mask of bits showing the source of the current triggered
* interrupt. This is useful for modes of operation where an interrupt can
* be generated by conditions in multiple interrupt source registers.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The mask with the OR of the following conditions that have been triggered.
* See \ref group_scb_common_macros_intr_cause for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetInterruptCause(CySCB_Type const *base)
{
    return (base->INTR_CAUSE);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetRxInterruptStatus
****************************************************************************//**
*
* Returns the RX interrupt request register. This register contains the current
* status of the RX interrupt sources.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of the RX interrupt sources. Each constant is a bit field
* value. The value returned may have multiple bits set to indicate the
* current status.
* See \ref group_scb_common_macros_rx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetRxInterruptStatus(CySCB_Type const *base)
{
    return (base->INTR_RX & CY_SCB_RX_INTR_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_SetRxInterruptMask
****************************************************************************//**
*
* Writes the RX interrupt mask register. This register configures which bits
* from the RX interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* Enabled RX interrupt sources.
* See \ref group_scb_common_macros_rx_intr.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetRxInterruptMask(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_RX_INTR_MASK));

    base->INTR_RX_MASK = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetRxInterruptMask
****************************************************************************//**
*
* Returns the RX interrupt mask register. This register specifies which bits
* from the RX interrupt request register trigger can an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* Enabled RX interrupt sources.
* See \ref group_scb_common_macros_rx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetRxInterruptMask(CySCB_Type const *base)
{
    return (base->INTR_RX_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetRxInterruptStatusMasked
****************************************************************************//**
*
* Returns the RX interrupt masked request register. This register contains
* a logical AND of corresponding bits from the RX interrupt request and
* mask registers.
* This function is intended to be used in the interrupt service routine to
* identify which of the enabled RX interrupt sources caused the interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of enabled RX interrupt sources.
* See \ref group_scb_common_macros_rx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetRxInterruptStatusMasked(CySCB_Type const *base)
{
    return (base->INTR_RX_MASKED);
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearRxInterrupt
****************************************************************************//**
*
* Clears the RX interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The RX interrupt sources to be cleared.
* See \ref group_scb_common_macros_rx_intr for the set of constants.
*
* \note
*  - CY_SCB_INTR_RX_FIFO_LEVEL interrupt source is not cleared when
*    the RX FIFO has more entries than the level.
*  - CY_SCB_INTR_RX_NOT_EMPTY interrupt source is not cleared when the
*    RX FIFO is not empty.
*  - CY_SCB_INTR_RX_FULL interrupt source is not cleared when the
*    RX FIFO is full.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearRxInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_RX_INTR_MASK));

    base->INTR_RX = interruptMask;
    (void) base->INTR_RX;
}


/*******************************************************************************
* Function Name: Cy_SCB_SetRxInterrupt
****************************************************************************//**
*
* Sets the RX interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The RX interrupt sources to set in the RX interrupt request register.
* See \ref group_scb_common_macros_rx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetRxInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_RX_INTR_MASK));

    base->INTR_RX_SET = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetTxInterruptStatus
****************************************************************************//**
*
* Returns the TX interrupt request register. This register contains the current
* status of the TX interrupt sources.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of TX interrupt sources.
* Each constant is a bit field value. The value returned may have multiple
* bits set to indicate the current status.
* See \ref group_scb_common_macros_tx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetTxInterruptStatus(CySCB_Type const *base)
{
    return (base->INTR_TX & CY_SCB_TX_INTR_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_SetTxInterruptMask
****************************************************************************//**
*
* Writes the TX interrupt mask register. This register configures which bits
* from the TX interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* Enabled TX interrupt sources.
* See \ref group_scb_common_macros_tx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetTxInterruptMask(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_TX_INTR_MASK));

    base->INTR_TX_MASK = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetTxInterruptMask
****************************************************************************//**
*
* Returns the TX interrupt mask register. This register specifies which
* bits from the TX interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* Enabled TX interrupt sources.
* See \ref group_scb_common_macros_tx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetTxInterruptMask(CySCB_Type const *base)
{
    return (base->INTR_TX_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetTxInterruptStatusMasked
****************************************************************************//**
*
* Returns the TX interrupt masked request register. This register contains
* a logical AND of corresponding bits from the TX interrupt request and
* mask registers.
* This function is intended to be used in the interrupt service routine to
* identify which of enabled TX interrupt sources caused the interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of enabled TX interrupt sources.
* See \ref group_scb_common_macros_tx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetTxInterruptStatusMasked(CySCB_Type const *base)
{
    return (base->INTR_TX_MASKED);
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearTxInterrupt
****************************************************************************//**
*
* Clears the TX interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The TX interrupt sources to be cleared.
* See \ref group_scb_common_macros_tx_intr for the set of constants.
*
* \note
*  - CY_SCB_INTR_TX_FIFO_LEVEL interrupt source is not cleared when the
*    TX FIFO has fewer entries than the TX level.
*  - CY_SCB_INTR_TX_NOT_FULL interrupt source is not cleared when the
*    TX FIFO has empty entries in the TX FIFO.
*  - CY_SCB_INTR_TX_EMPTY interrupt source is not cleared when the
*    TX FIFO is empty.
*  - CY_SCB_INTR_TX_UNDERFLOW interrupt source is not cleared when the
*    TX FIFO is empty. Put data into the TX FIFO before clearing it.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearTxInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_TX_INTR_MASK));

    base->INTR_TX = interruptMask;
    (void) base->INTR_TX;
}


/*******************************************************************************
* Function Name: Cy_SCB_SetTxInterrupt
****************************************************************************//**
*
* Sets TX interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The TX interrupt sources to set in the TX interrupt request register.
* See \ref group_scb_common_macros_tx_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetTxInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_TX_INTR_MASK));

    base->INTR_TX_SET = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetMasterInterruptStatus
****************************************************************************//**
*
* Returns the master interrupt request register. This register contains the current
* status of the master interrupt sources.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of the master interrupt sources.
* Each constant is a bit field value. The value returned may have multiple
* bits set to indicate the current status.
* See \ref group_scb_common_macros_master_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetMasterInterruptStatus(CySCB_Type const *base)
{
    return (base->INTR_M & CY_SCB_MASTER_INTR_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_SetMasterInterruptMask
****************************************************************************//**
*
* Writes the master interrupt mask register. This register specifies which bits
* from the master interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The master interrupt sources to be enable.
* See \ref group_scb_common_macros_master_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetMasterInterruptMask(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_MASTER_INTR_MASK));

    base->INTR_M_MASK = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetMasterInterruptMask
****************************************************************************//**
*
* Returns the master interrupt mask register. This register specifies which bits
* from the master interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* Enabled master interrupt sources.
* See \ref group_scb_common_macros_master_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetMasterInterruptMask(CySCB_Type const *base)
{
    return (base->INTR_M_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetMasterInterruptStatusMasked
****************************************************************************//**
*
* Returns the master interrupt masked request register. This register contains a
* logical AND of corresponding bits from the master interrupt request and mask
* registers.
* This function is intended to be used in the interrupt service routine to
* identify which of the enabled master interrupt sources caused the interrupt
* event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of enabled master interrupt sources.
* See \ref group_scb_common_macros_master_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetMasterInterruptStatusMasked(CySCB_Type const *base)
{
    return (base->INTR_M_MASKED);
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearMasterInterrupt
****************************************************************************//**
*
* Clears master interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The master interrupt sources to be cleared.
* See \ref group_scb_common_macros_master_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearMasterInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_MASTER_INTR_MASK));

    base->INTR_M = interruptMask;
    (void) base->INTR_M;
}


/*******************************************************************************
* Function Name: Cy_SCB_SetMasterInterrupt
****************************************************************************//**
*
* Sets master interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The master interrupt sources to set in the master interrupt request register.
* See \ref group_scb_common_macros_master_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetMasterInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_MASTER_INTR_MASK));

    base->INTR_M_SET = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetSlaveInterruptStatus
****************************************************************************//**
*
* Returns the slave interrupt request register. This register contains the current
* status of the slave interrupt sources.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of the slave interrupt sources.
* Each constant is a bit field value. The value returned may have multiple
* bits set to indicate the current status.
* See \ref group_scb_common_macros_slave_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetSlaveInterruptStatus(CySCB_Type const *base)
{
    return (base->INTR_S & CY_SCB_SLAVE_INTR_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_SetSlaveInterruptMask
****************************************************************************//**
*
* Writes slave interrupt mask register.
* This register specifies which bits from the slave interrupt request register
* can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* Enabled slave interrupt sources.
* See \ref group_scb_common_macros_slave_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetSlaveInterruptMask(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_SLAVE_INTR_MASK));

    base->INTR_S_MASK = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetSlaveInterruptMask
****************************************************************************//**
*
* Returns the slave interrupt mask register.
* This register specifies which bits from the slave interrupt request register
* can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* Enabled slave interrupt sources.
* See \ref group_scb_common_macros_slave_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetSlaveInterruptMask(CySCB_Type const *base)
{
    return (base->INTR_S_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetSlaveInterruptStatusMasked
****************************************************************************//**
*
* Returns the slave interrupt masked request register. This register contains a
* logical AND of corresponding bits from the slave interrupt request and mask
* registers.
* This function is intended to be used in the interrupt service routine to
* identify which of enabled slave interrupt sources caused the interrupt
* event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of enabled slave interrupt sources.
* See \ref group_scb_common_macros_slave_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetSlaveInterruptStatusMasked(CySCB_Type const *base)
{
    return (base->INTR_S_MASKED);
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearSlaveInterrupt
****************************************************************************//**
*
* Clears the slave interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* Slave interrupt sources to be cleared.
* See \ref group_scb_common_macros_slave_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearSlaveInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_SLAVE_INTR_MASK));

    base->INTR_S = interruptMask;
    (void) base->INTR_S;
}


/*******************************************************************************
* Function Name: Cy_SCB_SetSlaveInterrupt
****************************************************************************//**
*
* Sets slave interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The slave interrupt sources to set in the slave interrupt request register
* See \ref group_scb_common_macros_slave_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetSlaveInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_SLAVE_INTR_MASK));

    base->INTR_S_SET = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetI2CInterruptStatus
****************************************************************************//**
*
* Returns the I2C interrupt request register. This register contains the
* current status of the I2C interrupt sources.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of the I2C interrupt sources. Each constant is a bit
* field value.
* The value returned may have multiple bits set to indicate the current status.
* See \ref group_scb_common_macros_slave_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetI2CInterruptStatus(CySCB_Type const *base)
{
    return (base->INTR_I2C_EC & CY_SCB_I2C_INTR_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_SetI2CInterruptMask
****************************************************************************//**
*
* Writes the I2C interrupt mask register. This register specifies which bits
* from the I2C interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* Enabled I2C interrupt sources.
* See \ref group_scb_common_macros_i2c_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetI2CInterruptMask(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_I2C_INTR_MASK));

    base->INTR_I2C_EC_MASK = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetI2CInterruptMask
****************************************************************************//**
*
* Returns the I2C interrupt mask register. This register specifies which bits
* from the I2C interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* Enabled I2C interrupt sources.
* See \ref group_scb_common_macros_i2c_intr.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetI2CInterruptMask(CySCB_Type const *base)
{
    return (base->INTR_I2C_EC_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetI2CInterruptStatusMasked
****************************************************************************//**
*
* Returns the I2C interrupt masked request register. This register contains
* a logical AND of corresponding bits from I2C interrupt request and mask
* registers.
* This function is intended to be used in the interrupt service routine to
* identify which of enabled I2C interrupt sources caused the interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of enabled I2C interrupt sources.
* See \ref group_scb_common_macros_i2c_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetI2CInterruptStatusMasked(CySCB_Type const *base)
{
    return (base->INTR_I2C_EC_MASKED);
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearI2CInterrupt
****************************************************************************//**
*
* Clears I2C interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The I2C interrupt sources to be cleared.
* See \ref group_scb_common_macros_i2c_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearI2CInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_I2C_INTR_MASK));

    base->INTR_I2C_EC = interruptMask;
    (void) base->INTR_I2C_EC;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetSpiInterruptStatus
****************************************************************************//**
*
* Returns the SPI interrupt request register. This register contains the current
* status of the SPI interrupt sources.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of SPI interrupt sources. Each constant is a bit field value.
* The value returned may have multiple bits set to indicate the current status
* See \ref group_scb_common_macros_SpiIntrStatuses for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetSpiInterruptStatus(CySCB_Type const *base)
{
    return (base->INTR_SPI_EC & CY_SCB_SPI_INTR_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_SetSpiInterruptMask
****************************************************************************//**
*
* Writes the SPI interrupt mask register. This register specifies which
* bits from the SPI interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* Enabled SPI interrupt sources.
* See \ref group_scb_common_macros_SpiIntrStatuses for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_SetSpiInterruptMask(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_SPI_INTR_MASK));

    base->INTR_SPI_EC_MASK = interruptMask;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetSpiInterruptMask
****************************************************************************//**
*
* Returns the SPI interrupt mask register. This register specifies which bits
* from the SPI interrupt request register can trigger an interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* Enabled SPI interrupt sources.
* See \ref group_scb_common_macros_SpiIntrStatuses for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetSpiInterruptMask(CySCB_Type const *base)
{
    return (base->INTR_SPI_EC_MASK);
}


/*******************************************************************************
* Function Name: Cy_SCB_GetSpiInterruptStatusMasked
****************************************************************************//**
*
* Returns the SPI interrupt masked request register. This register contains
* a logical AND of corresponding bits from the SPI interrupt request and
* mask registers.
* This function is intended to be used in the interrupt service routine to
* identify which of enabled SPI interrupt sources caused the interrupt event.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* The current status of enabled SPI interrupt sources.
* See \ref group_scb_common_macros_SpiIntrStatuses for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetSpiInterruptStatusMasked(CySCB_Type const *base)
{
    return (base->INTR_SPI_EC_MASKED);
}


/*******************************************************************************
* Function Name: Cy_SCB_ClearSpiInterrupt
****************************************************************************//**
*
* Clears SPI interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the SCB instance.
*
* \param interruptMask
* The SPI interrupt sources to be cleared.
* See \ref group_scb_common_macros_SpiIntrStatuses for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_ClearSpiInterrupt(CySCB_Type *base, uint32_t interruptMask)
{
    CY_ASSERT_L2(CY_SCB_IS_INTR_VALID(interruptMask, CY_SCB_SPI_INTR_MASK));

    base->INTR_SPI_EC = interruptMask;
    (void) base->INTR_SPI_EC;
}

/** \cond INTERNAL */
/*******************************************************************************
* Function Name: Cy_SCB_GetFifoSize
****************************************************************************//**
*
* Returns the RX and TX FIFO depth.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* FIFO depth.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetFifoSize(CySCB_Type const *base)
{
    return (_FLD2BOOL(SCB_CTRL_BYTE_MODE, base->CTRL) ?
                (CY_SCB_FIFO_SIZE) : (CY_SCB_FIFO_SIZE / 2UL));
}


/*******************************************************************************
* Function Name: Cy_SCB_IsRxDataWidthByte
****************************************************************************//**
*
* Returns true if the RX data width is a byte (8 bits).
*
* \param base
* The pointer to the SCB instance.
*
* \return
* True if the RX data width is a byte (8 bits).
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SCB_IsRxDataWidthByte(CySCB_Type const *base)
{
    return (_FLD2VAL(SCB_RX_CTRL_DATA_WIDTH, base->RX_CTRL) < CY_SCB_BYTE_WIDTH);
}


/*******************************************************************************
* Function Name: Cy_SCB_IsTxDataWidthByte
****************************************************************************//**
*
* Returns true if the TX data width is a byte (8 bits).
*
* \param base
* The pointer to the SCB instance.
*
* \return
* If true, the TX data width is a byte (8 bits). Otherwise, false.
*
*******************************************************************************/
__STATIC_INLINE bool Cy_SCB_IsTxDataWidthByte(CySCB_Type const *base)
{
    return (_FLD2VAL(SCB_TX_CTRL_DATA_WIDTH, base->TX_CTRL) < CY_SCB_BYTE_WIDTH);
}


/*******************************************************************************
* Function Name: Cy_SCB_FwBlockReset
****************************************************************************//**
*
* Disables and enables the block to return it into the known state (default):
* FIFOs and interrupt statuses are cleared.
*
* \param base
* The pointer to the SCB instance.
*
*******************************************************************************/
__STATIC_INLINE void Cy_SCB_FwBlockReset(CySCB_Type *base)
{
    base->CTRL &= (uint32_t) ~SCB_CTRL_ENABLED_Msk;

    /* Clean-up command registers */
    base->I2C_M_CMD = 0UL;
    base->I2C_S_CMD = 0UL;

    base->CTRL |= (uint32_t)  SCB_CTRL_ENABLED_Msk;

    (void) base->CTRL;
}


/*******************************************************************************
* Function Name: Cy_SCB_GetRxFifoLevel
****************************************************************************//**
*
* Returns the RX FIFO level when there are more words in the RX FIFO than the
* level, the RX FIFO level interrupt is triggered.
*
* \param base
* The pointer to the SCB instance.
*
* \return
* RX FIFO level.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_SCB_GetRxFifoLevel(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_RX_FIFO_CTRL_TRIGGER_LEVEL, base->RX_FIFO_CTRL);
}

/** \endcond */
/** \} group_scb_common_functions */

#if defined(__cplusplus)
}
#endif

/** \} group_scb_common */

#endif /* CY_IP_MXSCB */

#endif /* (CY_SCB_COMMON_H) */

/* [] END OF FILE */

