/**
 * \file
 *
 * \brief Instance description for MCAN1
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_MCAN1_INSTANCE_H_
#define _SAME70_MCAN1_INSTANCE_H_

/* ========== Register definition for MCAN1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_MCAN1_CUST          (0x40034008) /**< (MCAN1) Customer Register */
#define REG_MCAN1_FBTP          (0x4003400C) /**< (MCAN1) Fast Bit Timing and Prescaler Register */
#define REG_MCAN1_TEST          (0x40034010) /**< (MCAN1) Test Register */
#define REG_MCAN1_RWD           (0x40034014) /**< (MCAN1) RAM Watchdog Register */
#define REG_MCAN1_CCCR          (0x40034018) /**< (MCAN1) CC Control Register */
#define REG_MCAN1_BTP           (0x4003401C) /**< (MCAN1) Bit Timing and Prescaler Register */
#define REG_MCAN1_TSCC          (0x40034020) /**< (MCAN1) Timestamp Counter Configuration Register */
#define REG_MCAN1_TSCV          (0x40034024) /**< (MCAN1) Timestamp Counter Value Register */
#define REG_MCAN1_TOCC          (0x40034028) /**< (MCAN1) Timeout Counter Configuration Register */
#define REG_MCAN1_TOCV          (0x4003402C) /**< (MCAN1) Timeout Counter Value Register */
#define REG_MCAN1_ECR           (0x40034040) /**< (MCAN1) Error Counter Register */
#define REG_MCAN1_PSR           (0x40034044) /**< (MCAN1) Protocol Status Register */
#define REG_MCAN1_IR            (0x40034050) /**< (MCAN1) Interrupt Register */
#define REG_MCAN1_IE            (0x40034054) /**< (MCAN1) Interrupt Enable Register */
#define REG_MCAN1_ILS           (0x40034058) /**< (MCAN1) Interrupt Line Select Register */
#define REG_MCAN1_ILE           (0x4003405C) /**< (MCAN1) Interrupt Line Enable Register */
#define REG_MCAN1_GFC           (0x40034080) /**< (MCAN1) Global Filter Configuration Register */
#define REG_MCAN1_SIDFC         (0x40034084) /**< (MCAN1) Standard ID Filter Configuration Register */
#define REG_MCAN1_XIDFC         (0x40034088) /**< (MCAN1) Extended ID Filter Configuration Register */
#define REG_MCAN1_XIDAM         (0x40034090) /**< (MCAN1) Extended ID AND Mask Register */
#define REG_MCAN1_HPMS          (0x40034094) /**< (MCAN1) High Priority Message Status Register */
#define REG_MCAN1_NDAT1         (0x40034098) /**< (MCAN1) New Data 1 Register */
#define REG_MCAN1_NDAT2         (0x4003409C) /**< (MCAN1) New Data 2 Register */
#define REG_MCAN1_RXF0C         (0x400340A0) /**< (MCAN1) Receive FIFO 0 Configuration Register */
#define REG_MCAN1_RXF0S         (0x400340A4) /**< (MCAN1) Receive FIFO 0 Status Register */
#define REG_MCAN1_RXF0A         (0x400340A8) /**< (MCAN1) Receive FIFO 0 Acknowledge Register */
#define REG_MCAN1_RXBC          (0x400340AC) /**< (MCAN1) Receive Rx Buffer Configuration Register */
#define REG_MCAN1_RXF1C         (0x400340B0) /**< (MCAN1) Receive FIFO 1 Configuration Register */
#define REG_MCAN1_RXF1S         (0x400340B4) /**< (MCAN1) Receive FIFO 1 Status Register */
#define REG_MCAN1_RXF1A         (0x400340B8) /**< (MCAN1) Receive FIFO 1 Acknowledge Register */
#define REG_MCAN1_RXESC         (0x400340BC) /**< (MCAN1) Receive Buffer / FIFO Element Size Configuration Register */
#define REG_MCAN1_TXBC          (0x400340C0) /**< (MCAN1) Transmit Buffer Configuration Register */
#define REG_MCAN1_TXFQS         (0x400340C4) /**< (MCAN1) Transmit FIFO/Queue Status Register */
#define REG_MCAN1_TXESC         (0x400340C8) /**< (MCAN1) Transmit Buffer Element Size Configuration Register */
#define REG_MCAN1_TXBRP         (0x400340CC) /**< (MCAN1) Transmit Buffer Request Pending Register */
#define REG_MCAN1_TXBAR         (0x400340D0) /**< (MCAN1) Transmit Buffer Add Request Register */
#define REG_MCAN1_TXBCR         (0x400340D4) /**< (MCAN1) Transmit Buffer Cancellation Request Register */
#define REG_MCAN1_TXBTO         (0x400340D8) /**< (MCAN1) Transmit Buffer Transmission Occurred Register */
#define REG_MCAN1_TXBCF         (0x400340DC) /**< (MCAN1) Transmit Buffer Cancellation Finished Register */
#define REG_MCAN1_TXBTIE        (0x400340E0) /**< (MCAN1) Transmit Buffer Transmission Interrupt Enable Register */
#define REG_MCAN1_TXBCIE        (0x400340E4) /**< (MCAN1) Transmit Buffer Cancellation Finished Interrupt Enable Register */
#define REG_MCAN1_TXEFC         (0x400340F0) /**< (MCAN1) Transmit Event FIFO Configuration Register */
#define REG_MCAN1_TXEFS         (0x400340F4) /**< (MCAN1) Transmit Event FIFO Status Register */
#define REG_MCAN1_TXEFA         (0x400340F8) /**< (MCAN1) Transmit Event FIFO Acknowledge Register */

#else

#define REG_MCAN1_CUST          (*(__IO uint32_t*)0x40034008U) /**< (MCAN1) Customer Register */
#define REG_MCAN1_FBTP          (*(__IO uint32_t*)0x4003400CU) /**< (MCAN1) Fast Bit Timing and Prescaler Register */
#define REG_MCAN1_TEST          (*(__IO uint32_t*)0x40034010U) /**< (MCAN1) Test Register */
#define REG_MCAN1_RWD           (*(__IO uint32_t*)0x40034014U) /**< (MCAN1) RAM Watchdog Register */
#define REG_MCAN1_CCCR          (*(__IO uint32_t*)0x40034018U) /**< (MCAN1) CC Control Register */
#define REG_MCAN1_BTP           (*(__IO uint32_t*)0x4003401CU) /**< (MCAN1) Bit Timing and Prescaler Register */
#define REG_MCAN1_TSCC          (*(__IO uint32_t*)0x40034020U) /**< (MCAN1) Timestamp Counter Configuration Register */
#define REG_MCAN1_TSCV          (*(__IO uint32_t*)0x40034024U) /**< (MCAN1) Timestamp Counter Value Register */
#define REG_MCAN1_TOCC          (*(__IO uint32_t*)0x40034028U) /**< (MCAN1) Timeout Counter Configuration Register */
#define REG_MCAN1_TOCV          (*(__IO uint32_t*)0x4003402CU) /**< (MCAN1) Timeout Counter Value Register */
#define REG_MCAN1_ECR           (*(__I  uint32_t*)0x40034040U) /**< (MCAN1) Error Counter Register */
#define REG_MCAN1_PSR           (*(__I  uint32_t*)0x40034044U) /**< (MCAN1) Protocol Status Register */
#define REG_MCAN1_IR            (*(__IO uint32_t*)0x40034050U) /**< (MCAN1) Interrupt Register */
#define REG_MCAN1_IE            (*(__IO uint32_t*)0x40034054U) /**< (MCAN1) Interrupt Enable Register */
#define REG_MCAN1_ILS           (*(__IO uint32_t*)0x40034058U) /**< (MCAN1) Interrupt Line Select Register */
#define REG_MCAN1_ILE           (*(__IO uint32_t*)0x4003405CU) /**< (MCAN1) Interrupt Line Enable Register */
#define REG_MCAN1_GFC           (*(__IO uint32_t*)0x40034080U) /**< (MCAN1) Global Filter Configuration Register */
#define REG_MCAN1_SIDFC         (*(__IO uint32_t*)0x40034084U) /**< (MCAN1) Standard ID Filter Configuration Register */
#define REG_MCAN1_XIDFC         (*(__IO uint32_t*)0x40034088U) /**< (MCAN1) Extended ID Filter Configuration Register */
#define REG_MCAN1_XIDAM         (*(__IO uint32_t*)0x40034090U) /**< (MCAN1) Extended ID AND Mask Register */
#define REG_MCAN1_HPMS          (*(__I  uint32_t*)0x40034094U) /**< (MCAN1) High Priority Message Status Register */
#define REG_MCAN1_NDAT1         (*(__IO uint32_t*)0x40034098U) /**< (MCAN1) New Data 1 Register */
#define REG_MCAN1_NDAT2         (*(__IO uint32_t*)0x4003409CU) /**< (MCAN1) New Data 2 Register */
#define REG_MCAN1_RXF0C         (*(__IO uint32_t*)0x400340A0U) /**< (MCAN1) Receive FIFO 0 Configuration Register */
#define REG_MCAN1_RXF0S         (*(__I  uint32_t*)0x400340A4U) /**< (MCAN1) Receive FIFO 0 Status Register */
#define REG_MCAN1_RXF0A         (*(__IO uint32_t*)0x400340A8U) /**< (MCAN1) Receive FIFO 0 Acknowledge Register */
#define REG_MCAN1_RXBC          (*(__IO uint32_t*)0x400340ACU) /**< (MCAN1) Receive Rx Buffer Configuration Register */
#define REG_MCAN1_RXF1C         (*(__IO uint32_t*)0x400340B0U) /**< (MCAN1) Receive FIFO 1 Configuration Register */
#define REG_MCAN1_RXF1S         (*(__I  uint32_t*)0x400340B4U) /**< (MCAN1) Receive FIFO 1 Status Register */
#define REG_MCAN1_RXF1A         (*(__IO uint32_t*)0x400340B8U) /**< (MCAN1) Receive FIFO 1 Acknowledge Register */
#define REG_MCAN1_RXESC         (*(__IO uint32_t*)0x400340BCU) /**< (MCAN1) Receive Buffer / FIFO Element Size Configuration Register */
#define REG_MCAN1_TXBC          (*(__IO uint32_t*)0x400340C0U) /**< (MCAN1) Transmit Buffer Configuration Register */
#define REG_MCAN1_TXFQS         (*(__I  uint32_t*)0x400340C4U) /**< (MCAN1) Transmit FIFO/Queue Status Register */
#define REG_MCAN1_TXESC         (*(__IO uint32_t*)0x400340C8U) /**< (MCAN1) Transmit Buffer Element Size Configuration Register */
#define REG_MCAN1_TXBRP         (*(__I  uint32_t*)0x400340CCU) /**< (MCAN1) Transmit Buffer Request Pending Register */
#define REG_MCAN1_TXBAR         (*(__IO uint32_t*)0x400340D0U) /**< (MCAN1) Transmit Buffer Add Request Register */
#define REG_MCAN1_TXBCR         (*(__IO uint32_t*)0x400340D4U) /**< (MCAN1) Transmit Buffer Cancellation Request Register */
#define REG_MCAN1_TXBTO         (*(__I  uint32_t*)0x400340D8U) /**< (MCAN1) Transmit Buffer Transmission Occurred Register */
#define REG_MCAN1_TXBCF         (*(__I  uint32_t*)0x400340DCU) /**< (MCAN1) Transmit Buffer Cancellation Finished Register */
#define REG_MCAN1_TXBTIE        (*(__IO uint32_t*)0x400340E0U) /**< (MCAN1) Transmit Buffer Transmission Interrupt Enable Register */
#define REG_MCAN1_TXBCIE        (*(__IO uint32_t*)0x400340E4U) /**< (MCAN1) Transmit Buffer Cancellation Finished Interrupt Enable Register */
#define REG_MCAN1_TXEFC         (*(__IO uint32_t*)0x400340F0U) /**< (MCAN1) Transmit Event FIFO Configuration Register */
#define REG_MCAN1_TXEFS         (*(__I  uint32_t*)0x400340F4U) /**< (MCAN1) Transmit Event FIFO Status Register */
#define REG_MCAN1_TXEFA         (*(__IO uint32_t*)0x400340F8U) /**< (MCAN1) Transmit Event FIFO Acknowledge Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for MCAN1 peripheral ========== */
#define MCAN1_INSTANCE_ID                        37        

#endif /* _SAME70_MCAN1_INSTANCE_ */
