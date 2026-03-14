/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32a455_i2c.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_I2C_H__
#define __N32A455_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup I2C
 * @{
 */

/** @addtogroup I2C_Exported_Types
 * @{
 */

/**
 * @brief  I2C Init structure definition
 */

typedef struct
{
    uint32_t ClkSpeed; /*!< Specifies the clock frequency.
                                  This parameter must be set to a value lower than 400kHz */

    uint16_t BusMode; /*!< Specifies the I2C mode.
                            This parameter can be a value of @ref I2C_BusMode */

    uint16_t FmDutyCycle; /*!< Specifies the I2C fast mode duty cycle.
                                 This parameter can be a value of @ref I2C_duty_cycle_in_fast_mode */

    uint16_t OwnAddr1; /*!< Specifies the first device own address.
                                   This parameter can be a 7-bit or 10-bit address. */

    uint16_t AckEnable; /*!< Enables or disables the acknowledgement.
                           This parameter can be a value of @ref I2C_acknowledgement */

    uint16_t AddrMode; /*!< Specifies if 7-bit or 10-bit address is acknowledged.
                                           This parameter can be a value of @ref I2C_acknowledged_address */
} I2C_InitType;

/**
 * @}
 */

/** @addtogroup I2C_Exported_Constants
 * @{
 */

#define IS_I2C_PERIPH(PERIPH) (((PERIPH) == I2C1) || ((PERIPH) == I2C2) || ((PERIPH) == I2C3) || ((PERIPH) == I2C4))
/** @addtogroup I2C_BusMode
 * @{
 */

#define I2C_BUSMODE_I2C       ((uint16_t)0x0000)
#define I2C_BUSMODE_SMBDEVICE ((uint16_t)0x0002)
#define I2C_BUSMODE_SMBHOST   ((uint16_t)0x000A)
#define IS_I2C_BUS_MODE(MODE)                                                                                          \
    (((MODE) == I2C_BUSMODE_I2C) || ((MODE) == I2C_BUSMODE_SMBDEVICE) || ((MODE) == I2C_BUSMODE_SMBHOST))
/**
 * @}
 */

/** @addtogroup I2C_duty_cycle_in_fast_mode
 * @{
 */

#define I2C_FMDUTYCYCLE_16_9        ((uint16_t)0x4000) /*!< I2C fast mode Tlow/Thigh = 16/9 */
#define I2C_FMDUTYCYCLE_2           ((uint16_t)0xBFFF) /*!< I2C fast mode Tlow/Thigh = 2 */
#define IS_I2C_FM_DUTY_CYCLE(CYCLE) (((CYCLE) == I2C_FMDUTYCYCLE_16_9) || ((CYCLE) == I2C_FMDUTYCYCLE_2))
/**
 * @}
 */

/** @addtogroup I2C_acknowledgement
 * @{
 */

#define I2C_ACKEN               ((uint16_t)0x0400)
#define I2C_ACKDIS              ((uint16_t)0x0000)
#define IS_I2C_ACK_STATE(STATE) (((STATE) == I2C_ACKEN) || ((STATE) == I2C_ACKDIS))
/**
 * @}
 */

/** @addtogroup I2C_transfer_direction
 * @{
 */

#define I2C_DIRECTION_SEND          ((uint8_t)0x00)
#define I2C_DIRECTION_RECV          ((uint8_t)0x01)
#define IS_I2C_DIRECTION(DIRECTION) (((DIRECTION) == I2C_DIRECTION_SEND) || ((DIRECTION) == I2C_DIRECTION_RECV))
/**
 * @}
 */

/** @addtogroup I2C_acknowledged_address
 * @{
 */

#define I2C_ADDR_MODE_7BIT        ((uint16_t)0x4000)
#define I2C_ADDR_MODE_10BIT       ((uint16_t)0xC000)
#define IS_I2C_ADDR_MODE(ADDRESS) (((ADDRESS) == I2C_ADDR_MODE_7BIT) || ((ADDRESS) == I2C_ADDR_MODE_10BIT))
/**
 * @}
 */

/** @addtogroup I2C_registers
 * @{
 */

#define I2C_REG_CTRL1   ((uint8_t)0x00)
#define I2C_REG_CTRL2   ((uint8_t)0x04)
#define I2C_REG_OADDR1  ((uint8_t)0x08)
#define I2C_REG_OADDR2  ((uint8_t)0x0C)
#define I2C_REG_DAT     ((uint8_t)0x10)
#define I2C_REG_STS1    ((uint8_t)0x14)
#define I2C_REG_STS2    ((uint8_t)0x18)
#define I2C_REG_CLKCTRL ((uint8_t)0x1C)
#define I2C_REG_TMRISE  ((uint8_t)0x20)
#define IS_I2C_REG(REGISTER)                                                                                           \
    (((REGISTER) == I2C_REG_CTRL1) || ((REGISTER) == I2C_REG_CTRL2) || ((REGISTER) == I2C_REG_OADDR1)                  \
     || ((REGISTER) == I2C_REG_OADDR2) || ((REGISTER) == I2C_REG_DAT) || ((REGISTER) == I2C_REG_STS1)                  \
     || ((REGISTER) == I2C_REG_STS2) || ((REGISTER) == I2C_REG_CLKCTRL) || ((REGISTER) == I2C_REG_TMRISE))
/**
 * @}
 */

/** @addtogroup I2C_SMBus_alert_pin_level
 * @{
 */

#define I2C_SMBALERT_LOW        ((uint16_t)0x2000)
#define I2C_SMBALERT_HIGH       ((uint16_t)0xDFFF)
#define IS_I2C_SMB_ALERT(ALERT) (((ALERT) == I2C_SMBALERT_LOW) || ((ALERT) == I2C_SMBALERT_HIGH))
/**
 * @}
 */

/** @addtogroup I2C_PEC_position
 * @{
 */

#define I2C_PEC_POS_NEXT         ((uint16_t)0x0800)
#define I2C_PEC_POS_CURRENT      ((uint16_t)0xF7FF)
#define IS_I2C_PEC_POS(POSITION) (((POSITION) == I2C_PEC_POS_NEXT) || ((POSITION) == I2C_PEC_POS_CURRENT))
/**
 * @}
 */

/** @addtogroup I2C_NCAK_position
 * @{
 */

#define I2C_NACK_POS_NEXT         ((uint16_t)0x0800)
#define I2C_NACK_POS_CURRENT      ((uint16_t)0xF7FF)
#define IS_I2C_NACK_POS(POSITION) (((POSITION) == I2C_NACK_POS_NEXT) || ((POSITION) == I2C_NACK_POS_CURRENT))
/**
 * @}
 */

/** @addtogroup I2C_interrupts_definition
 * @{
 */

#define I2C_INT_BUF        ((uint16_t)0x0400)
#define I2C_INT_EVENT      ((uint16_t)0x0200)
#define I2C_INT_ERR        ((uint16_t)0x0100)
#define IS_I2C_CFG_INT(IT) ((((IT) & (uint16_t)0xF8FF) == 0x00) && ((IT) != 0x00))
/**
 * @}
 */

/** @addtogroup I2C_interrupts_definition
 * @{
 */

#define I2C_INT_SMBALERT ((uint32_t)0x01008000)
#define I2C_INT_TIMOUT   ((uint32_t)0x01004000)
#define I2C_INT_PECERR   ((uint32_t)0x01001000)
#define I2C_INT_OVERRUN  ((uint32_t)0x01000800)
#define I2C_INT_ACKFAIL  ((uint32_t)0x01000400)
#define I2C_INT_ARLOST   ((uint32_t)0x01000200)
#define I2C_INT_BUSERR   ((uint32_t)0x01000100)
#define I2C_INT_TXDATE   ((uint32_t)0x06000080)
#define I2C_INT_RXDATNE  ((uint32_t)0x06000040)
#define I2C_INT_STOPF    ((uint32_t)0x02000010)
#define I2C_INT_ADDR10F  ((uint32_t)0x02000008)
#define I2C_INT_BYTEF    ((uint32_t)0x02000004)
#define I2C_INT_ADDRF    ((uint32_t)0x02000002)
#define I2C_INT_STARTBF  ((uint32_t)0x02000001)

#define IS_I2C_CLR_INT(IT) ((((IT) & (uint16_t)0x20FF) == 0x00) && ((IT) != (uint16_t)0x00))

#define IS_I2C_GET_INT(IT)                                                                                             \
    (((IT) == I2C_INT_SMBALERT) || ((IT) == I2C_INT_TIMOUT) || ((IT) == I2C_INT_PECERR) || ((IT) == I2C_INT_OVERRUN)   \
     || ((IT) == I2C_INT_ACKFAIL) || ((IT) == I2C_INT_ARLOST) || ((IT) == I2C_INT_BUSERR) || ((IT) == I2C_INT_TXDATE)  \
     || ((IT) == I2C_INT_RXDATNE) || ((IT) == I2C_INT_STOPF) || ((IT) == I2C_INT_ADDR10F) || ((IT) == I2C_INT_BYTEF)   \
     || ((IT) == I2C_INT_ADDRF) || ((IT) == I2C_INT_STARTBF))
/**
 * @}
 */

/** @addtogroup I2C_flags_definition
 * @{
 */

/**
 * @brief  STS2 register flags
 */

#define I2C_FLAG_DUALFLAG  ((uint32_t)0x00800000)
#define I2C_FLAG_SMBHADDR  ((uint32_t)0x00400000)
#define I2C_FLAG_SMBDADDR  ((uint32_t)0x00200000)
#define I2C_FLAG_GCALLADDR ((uint32_t)0x00100000)
#define I2C_FLAG_TRF       ((uint32_t)0x00040000)
#define I2C_FLAG_BUSY      ((uint32_t)0x00020000)
#define I2C_FLAG_MSMODE    ((uint32_t)0x00010000)

/**
 * @brief  STS1 register flags
 */

#define I2C_FLAG_SMBALERT ((uint32_t)0x10008000)
#define I2C_FLAG_TIMOUT   ((uint32_t)0x10004000)
#define I2C_FLAG_PECERR   ((uint32_t)0x10001000)
#define I2C_FLAG_OVERRUN  ((uint32_t)0x10000800)
#define I2C_FLAG_ACKFAIL  ((uint32_t)0x10000400)
#define I2C_FLAG_ARLOST   ((uint32_t)0x10000200)
#define I2C_FLAG_BUSERR   ((uint32_t)0x10000100)
#define I2C_FLAG_TXDATE   ((uint32_t)0x10000080)
#define I2C_FLAG_RXDATNE  ((uint32_t)0x10000040)
#define I2C_FLAG_STOPF    ((uint32_t)0x10000010)
#define I2C_FLAG_ADDR10F  ((uint32_t)0x10000008)
#define I2C_FLAG_BYTEF    ((uint32_t)0x10000004)
#define I2C_FLAG_ADDRF    ((uint32_t)0x10000002)
#define I2C_FLAG_STARTBF  ((uint32_t)0x10000001)

#define IS_I2C_CLR_FLAG(FLAG) ((((FLAG) & (uint16_t)0x20FF) == 0x00) && ((FLAG) != (uint16_t)0x00))

#define IS_I2C_GET_FLAG(FLAG)                                                                                          \
    (((FLAG) == I2C_FLAG_DUALFLAG) || ((FLAG) == I2C_FLAG_SMBHADDR) || ((FLAG) == I2C_FLAG_SMBDADDR)                   \
     || ((FLAG) == I2C_FLAG_GCALLADDR) || ((FLAG) == I2C_FLAG_TRF) || ((FLAG) == I2C_FLAG_BUSY)                        \
     || ((FLAG) == I2C_FLAG_MSMODE) || ((FLAG) == I2C_FLAG_SMBALERT) || ((FLAG) == I2C_FLAG_TIMOUT)                    \
     || ((FLAG) == I2C_FLAG_PECERR) || ((FLAG) == I2C_FLAG_OVERRUN) || ((FLAG) == I2C_FLAG_ACKFAIL)                    \
     || ((FLAG) == I2C_FLAG_ARLOST) || ((FLAG) == I2C_FLAG_BUSERR) || ((FLAG) == I2C_FLAG_TXDATE)                      \
     || ((FLAG) == I2C_FLAG_RXDATNE) || ((FLAG) == I2C_FLAG_STOPF) || ((FLAG) == I2C_FLAG_ADDR10F)                     \
     || ((FLAG) == I2C_FLAG_BYTEF) || ((FLAG) == I2C_FLAG_ADDRF) || ((FLAG) == I2C_FLAG_STARTBF))
/**
 * @}
 */

/** @addtogroup I2C_Events
 * @{
 */

/*========================================

                     I2C Master Events (Events grouped in order of communication)
                                                        ==========================================*/
/**
 * @brief  Communication start
 *
 * After sending the START condition (I2C_GenerateStart() function) the master
 * has to wait for this event. It means that the Start condition has been correctly
 * released on the I2C bus (the bus is free, no other devices is communicating).
 *
 */
/* Master mode */
#define I2C_ROLE_MASTER ((uint32_t)0x00010000) /* MSMODE */
/* --EV5 */
#define I2C_EVT_MASTER_MODE_FLAG ((uint32_t)0x00030001) /* BUSY, MSMODE and STARTBF flag */

/**
 * @brief  Address Acknowledge
 *
 * After checking on EV5 (start condition correctly released on the bus), the
 * master sends the address of the slave(s) with which it will communicate
 * (I2C_SendAddr7bit() function, it also determines the direction of the communication:
 * Master transmitter or Receiver). Then the master has to wait that a slave acknowledges
 * his address. If an acknowledge is sent on the bus, one of the following events will
 * be set:
 *
 *  1) In case of Master Receiver (7-bit addressing): the I2C_EVT_MASTER_RXMODE_FLAG
 *     event is set.
 *
 *  2) In case of Master Transmitter (7-bit addressing): the I2C_EVT_MASTER_TXMODE_FLAG
 *     is set
 *
 *  3) In case of 10-Bit addressing mode, the master (just after generating the START
 *  and checking on EV5) has to send the header of 10-bit addressing mode (I2C_SendData()
 *  function). Then master should wait on EV9. It means that the 10-bit addressing
 *  header has been correctly sent on the bus. Then master should send the second part of
 *  the 10-bit address (LSB) using the function I2C_SendAddr7bit(). Then master
 *  should wait for event EV6.
 *
 */

/* --EV6 */
#define I2C_EVT_MASTER_TXMODE_FLAG ((uint32_t)0x00070082) /* BUSY, MSMODE, ADDRF, TXDATE and TRF flags */
#define I2C_EVT_MASTER_RXMODE_FLAG ((uint32_t)0x00030002) /* BUSY, MSMODE and ADDRF flags */
/* --EV9 */
#define I2C_EVT_MASTER_MODE_ADDRESS10_FLAG ((uint32_t)0x00030008) /* BUSY, MSMODE and ADD10RF flags */

/**
 * @brief Communication events
 *
 * If a communication is established (START condition generated and slave address
 * acknowledged) then the master has to check on one of the following events for
 * communication procedures:
 *
 * 1) Master Receiver mode: The master has to wait on the event EV7 then to read
 *    the data received from the slave (I2C_RecvData() function).
 *
 * 2) Master Transmitter mode: The master has to send data (I2C_SendData()
 *    function) then to wait on event EV8 or EV8_2.
 *    These two events are similar:
 *     - EV8 means that the data has been written in the data register and is
 *       being shifted out.
 *     - EV8_2 means that the data has been physically shifted out and output
 *       on the bus.
 *     In most cases, using EV8 is sufficient for the application.
 *     Using EV8_2 leads to a slower communication but ensure more reliable test.
 *     EV8_2 is also more suitable than EV8 for testing on the last data transmission
 *     (before Stop condition generation).
 *
 *  @note In case the  user software does not guarantee that this event EV7 is
 *  managed before the current byte end of transfer, then user may check on EV7
 *  and BSF flag at the same time (ie. (I2C_EVT_MASTER_DATA_RECVD_FLAG | I2C_FLAG_BYTEF)).
 *  In this case the communication may be slower.
 *
 */

/* Master RECEIVER mode -----------------------------*/
/* --EV7 */
#define I2C_EVT_MASTER_DATA_RECVD_FLAG ((uint32_t)0x00030040) /* BUSY, MSMODE and RXDATNE flags */
/* EV7x shifter register full */
#define I2C_EVT_MASTER_SFT_DATA_RECVD_FLAG ((uint32_t)0x00030044) /* BUSY, MSMODE, BSF and RXDATNE flags */

/* Master TRANSMITTER mode --------------------------*/
/* --EV8 */
#define I2C_EVT_MASTER_DATA_SENDING ((uint32_t)0x00070080) /* TRF, BUSY, MSMODE, TXDATE flags */
/* --EV8_2 */
#define I2C_EVT_MASTER_DATA_SENDED ((uint32_t)0x00070084) /* TRF, BUSY, MSMODE, TXDATE and BSF flags */

/*========================================

                     I2C Slave Events (Events grouped in order of communication)
                                                        ==========================================*/

/**
 * @brief  Communication start events
 *
 * Wait on one of these events at the start of the communication. It means that
 * the I2C peripheral detected a Start condition on the bus (generated by master
 * device) followed by the peripheral address. The peripheral generates an ACK
 * condition on the bus (if the acknowledge feature is enabled through function
 * I2C_ConfigAck()) and the events listed above are set :
 *
 * 1) In normal case (only one address managed by the slave), when the address
 *   sent by the master matches the own address of the peripheral (configured by
 *   OwnAddr1 field) the I2C_EVENT_SLAVE_XXX_ADDRESS_MATCHED event is set
 *   (where XXX could be TRANSMITTER or RECEIVER).
 *
 * 2) In case the address sent by the master matches the second address of the
 *   peripheral (configured by the function I2C_ConfigOwnAddr2() and enabled
 *   by the function I2C_EnableDualAddr()) the events I2C_EVENT_SLAVE_XXX_SECONDADDRESS_MATCHED
 *   (where XXX could be TRANSMITTER or RECEIVER) are set.
 *
 * 3) In case the address sent by the master is General Call (address 0x00) and
 *   if the General Call is enabled for the peripheral (using function I2C_EnableGeneralCall())
 *   the following event is set I2C_EVT_SLAVE_GCALLADDR_MATCHED.
 *
 */

/* --EV1  (all the events below are variants of EV1) */
/* 1) Case of One Single Address managed by the slave */
#define I2C_EVT_SLAVE_RECV_ADDR_MATCHED ((uint32_t)0x00020002) /* BUSY and ADDRF flags */
#define I2C_EVT_SLAVE_SEND_ADDR_MATCHED ((uint32_t)0x00060082) /* TRF, BUSY, TXDATE and ADDRF flags */

/* 2) Case of Dual address managed by the slave */
#define I2C_EVT_SLAVE_RECV_ADDR2_MATCHED ((uint32_t)0x00820000) /* DUALF and BUSY flags */
#define I2C_EVT_SLAVE_SEND_ADDR2_MATCHED ((uint32_t)0x00860080) /* DUALF, TRF, BUSY and TXDATE flags */

/* 3) Case of General Call enabled for the slave */
#define I2C_EVT_SLAVE_GCALLADDR_MATCHED ((uint32_t)0x00120000) /* GENCALL and BUSY flags */

/**
 * @brief  Communication events
 *
 * Wait on one of these events when EV1 has already been checked and:
 *
 * - Slave RECEIVER mode:
 *     - EV2: When the application is expecting a data byte to be received.
 *     - EV4: When the application is expecting the end of the communication: master
 *       sends a stop condition and data transmission is stopped.
 *
 * - Slave Transmitter mode:
 *    - EV3: When a byte has been transmitted by the slave and the application is expecting
 *      the end of the byte transmission. The two events I2C_EVT_SLAVE_DATA_SENDED and
 *      I2C_EVT_SLAVE_DATA_SENDING are similar. The second one can optionally be
 *      used when the user software doesn't guarantee the EV3 is managed before the
 *      current byte end of transfer.
 *    - EV3_2: When the master sends a NACK in order to tell slave that data transmission
 *      shall end (before sending the STOP condition). In this case slave has to stop sending
 *      data bytes and expect a Stop condition on the bus.
 *
 *  @note In case the  user software does not guarantee that the event EV2 is
 *  managed before the current byte end of transfer, then user may check on EV2
 *  and BSF flag at the same time (ie. (I2C_EVT_SLAVE_DATA_RECVD | I2C_FLAG_BYTEF)).
 * In this case the communication may be slower.
 *
 */

/* Slave RECEIVER mode --------------------------*/
/* --EV2 */
#define I2C_EVT_SLAVE_DATA_RECVD ((uint32_t)0x00020040) /* BUSY and RXDATNE flags */
/* --EV2x */
#define I2C_EVT_SLAVE_DATA_RECVD_NOBUSY ((uint32_t)0x00000040) /* no BUSY and RXDATNE flags */
/* --EV4  */
#define I2C_EVT_SLAVE_STOP_RECVD ((uint32_t)0x00000010) /* STOPF flag */

/* Slave TRANSMITTER mode -----------------------*/
/* --EV3 */
#define I2C_EVT_SLAVE_DATA_SENDED  ((uint32_t)0x00060084) /* TRF, BUSY, TXDATE and BSF flags */
#define I2C_EVT_SLAVE_DATA_SENDING ((uint32_t)0x00060080) /* TRF, BUSY and TXDATE flags */
/* --EV3_2 */
#define I2C_EVT_SLAVE_ACK_MISS ((uint32_t)0x00000400) /* AF flag */

/*===========================      End of Events Description           ==========================================*/

#define IS_I2C_EVT(EVENT)                                                                                              \
    (((EVENT) == I2C_EVT_SLAVE_SEND_ADDR_MATCHED) || ((EVENT) == I2C_EVT_SLAVE_RECV_ADDR_MATCHED)                      \
     || ((EVENT) == I2C_EVT_SLAVE_SEND_ADDR2_MATCHED) || ((EVENT) == I2C_EVT_SLAVE_RECV_ADDR2_MATCHED)                 \
     || ((EVENT) == I2C_EVT_SLAVE_GCALLADDR_MATCHED) || ((EVENT) == I2C_EVT_SLAVE_DATA_RECVD)                          \
     || ((EVENT) == (I2C_EVT_SLAVE_DATA_RECVD | I2C_FLAG_DUALFLAG))                                                    \
     || ((EVENT) == (I2C_EVT_SLAVE_DATA_RECVD | I2C_FLAG_GCALLADDR)) || ((EVENT) == I2C_EVT_SLAVE_DATA_SENDED)         \
     || ((EVENT) == (I2C_EVT_SLAVE_DATA_SENDED | I2C_FLAG_DUALFLAG))                                                   \
     || ((EVENT) == (I2C_EVT_SLAVE_DATA_SENDED | I2C_FLAG_GCALLADDR)) || ((EVENT) == I2C_EVT_SLAVE_STOP_RECVD)         \
     || ((EVENT) == I2C_EVT_MASTER_MODE_FLAG) || ((EVENT) == I2C_EVT_MASTER_TXMODE_FLAG)                               \
     || ((EVENT) == I2C_EVT_MASTER_RXMODE_FLAG) || ((EVENT) == I2C_EVT_MASTER_DATA_RECVD_FLAG)                         \
     || ((EVENT) == I2C_EVT_MASTER_DATA_SENDED) || ((EVENT) == I2C_EVT_MASTER_DATA_SENDING)                            \
     || ((EVENT) == I2C_EVT_MASTER_MODE_ADDRESS10_FLAG) || ((EVENT) == I2C_EVT_SLAVE_ACK_MISS)                         \
     || ((EVENT) == I2C_EVT_MASTER_SFT_DATA_RECVD_FLAG) || ((EVENT) == I2C_EVT_SLAVE_DATA_RECVD_NOBUSY))
/**
 * @}
 */

/** @addtogroup I2C_own_address1
 * @{
 */

#define IS_I2C_OWN_ADDR1(ADDRESS1) ((ADDRESS1) <= 0x3FF)
/**
 * @}
 */

/** @addtogroup I2C_clock_speed
 * @{
 */

//#define IS_I2C_CLK_SPEED(SPEED) (((SPEED) >= 0x1) && ((SPEED) <= 400000))
#define IS_I2C_CLK_SPEED(SPEED) (((SPEED) >= 0x1) && ((SPEED) <= 1000000))

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup I2C_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup I2C_Exported_Functions
 * @{
 */

void I2C_DeInit(I2C_Module* I2Cx);
void I2C_Init(I2C_Module* I2Cx, I2C_InitType* I2C_InitStruct);
void I2C_InitStruct(I2C_InitType* I2C_InitStruct);
void I2C_Enable(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_EnableDMA(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_EnableDmaLastSend(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_GenerateStart(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_GenerateStop(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_ConfigAck(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_ConfigOwnAddr2(I2C_Module* I2Cx, uint8_t Address);
void I2C_EnableDualAddr(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_EnableGeneralCall(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_ConfigInt(I2C_Module* I2Cx, uint16_t I2C_IT, FunctionalState Cmd);
void I2C_SendData(I2C_Module* I2Cx, uint8_t Data);
uint8_t I2C_RecvData(I2C_Module* I2Cx);
void I2C_SendAddr7bit(I2C_Module* I2Cx, uint8_t Address, uint8_t I2C_Direction);
uint16_t I2C_GetRegister(I2C_Module* I2Cx, uint8_t I2C_Register);
void I2C_EnableSoftwareReset(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_ConfigNackLocation(I2C_Module* I2Cx, uint16_t I2C_NACKPosition);
void I2C_ConfigSmbusAlert(I2C_Module* I2Cx, uint16_t I2C_SMBusAlert);
void I2C_SendPEC(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_ConfigPecLocation(I2C_Module* I2Cx, uint16_t I2C_PECPosition);
void I2C_ComputePec(I2C_Module* I2Cx, FunctionalState Cmd);
uint8_t I2C_GetPec(I2C_Module* I2Cx);
void I2C_EnableArp(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_EnableExtendClk(I2C_Module* I2Cx, FunctionalState Cmd);
void I2C_ConfigFastModeDutyCycle(I2C_Module* I2Cx, uint16_t FmDutyCycle);

/**
 * @brief
 ****************************************************************************************
 *
 *                         I2C State Monitoring Functions
 *
 ****************************************************************************************
 * This I2C driver provides three different ways for I2C state monitoring
 *  depending on the application requirements and constraints:
 *
 *
 * 1) Basic state monitoring:
 *    Using I2C_CheckEvent() function:
 *    It compares the status registers (STS1 and STS2) content to a given event
 *    (can be the combination of one or more flags).
 *    It returns SUCCESS if the current status includes the given flags
 *    and returns ERROR if one or more flags are missing in the current status.
 *    - When to use:
 *      - This function is suitable for most applications as well as for startup
 *      activity since the events are fully described in the product reference manual
 *      (RM0008).
 *      - It is also suitable for users who need to define their own events.
 *    - Limitations:
 *      - If an error occurs (ie. error flags are set besides to the monitored flags),
 *        the I2C_CheckEvent() function may return SUCCESS despite the communication
 *        hold or corrupted real state.
 *        In this case, it is advised to use error interrupts to monitor the error
 *        events and handle them in the interrupt IRQ handler.
 *
 *        @note
 *        For error management, it is advised to use the following functions:
 *          - I2C_ConfigInt() to configure and enable the error interrupts (I2C_INT_ERR).
 *          - I2Cx_ER_IRQHandler() which is called when the error interrupt occurs.
 *            Where x is the peripheral instance (I2C1, I2C2 ...)
 *          - I2C_GetFlag() or I2C_GetIntStatus() to be called into I2Cx_ER_IRQHandler()
 *            in order to determine which error occurred.
 *          - I2C_ClrFlag() or I2C_ClrIntPendingBit() and/or I2C_EnableSoftwareReset()
 *            and/or I2C_GenerateStop() in order to clear the error flag and source,
 *            and return to correct communication status.
 *
 *
 *  2) Advanced state monitoring:
 *     Using the function I2C_GetLastEvent() which returns the image of both status
 *     registers in a single word (uint32_t) (Status Register 2 value is shifted left
 *     by 16 bits and concatenated to Status Register 1).
 *     - When to use:
 *       - This function is suitable for the same applications above but it allows to
 *         overcome the limitations of I2C_GetFlag() function (see below).
 *         The returned value could be compared to events already defined in the
 *         library (n32a455_i2c.h) or to custom values defined by user.
 *       - This function is suitable when multiple flags are monitored at the same time.
 *       - At the opposite of I2C_CheckEvent() function, this function allows user to
 *         choose when an event is accepted (when all events flags are set and no
 *         other flags are set or just when the needed flags are set like
 *         I2C_CheckEvent() function).
 *     - Limitations:
 *       - User may need to define his own events.
 *       - Same remark concerning the error management is applicable for this
 *         function if user decides to check only regular communication flags (and
 *         ignores error flags).
 *
 *
 *  3) Flag-based state monitoring:
 *     Using the function I2C_GetFlag() which simply returns the status of
 *     one single flag (ie. I2C_FLAG_RXDATNE ...).
 *     - When to use:
 *        - This function could be used for specific applications or in debug phase.
 *        - It is suitable when only one flag checking is needed (most I2C events
 *          are monitored through multiple flags).
 *     - Limitations:
 *        - When calling this function, the Status register is accessed. Some flags are
 *          cleared when the status register is accessed. So checking the status
 *          of one Flag, may clear other ones.
 *        - Function may need to be called twice or more in order to monitor one
 *          single event.
 *
 */

/**
 *
 *  1) Basic state monitoring
 *******************************************************************************
 */
ErrorStatus I2C_CheckEvent(I2C_Module* I2Cx, uint32_t I2C_EVENT);
/**
 *
 *  2) Advanced state monitoring
 *******************************************************************************
 */
uint32_t I2C_GetLastEvent(I2C_Module* I2Cx);
/**
 *
 *  3) Flag-based state monitoring
 *******************************************************************************
 */
FlagStatus I2C_GetFlag(I2C_Module* I2Cx, uint32_t I2C_FLAG);
/**
 *
 *******************************************************************************
 */

void I2C_ClrFlag(I2C_Module* I2Cx, uint32_t I2C_FLAG);
INTStatus I2C_GetIntStatus(I2C_Module* I2Cx, uint32_t I2C_IT);
void I2C_ClrIntPendingBit(I2C_Module* I2Cx, uint32_t I2C_IT);

#ifdef __cplusplus
}
#endif

#endif /*__N32A455_I2C_H */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
