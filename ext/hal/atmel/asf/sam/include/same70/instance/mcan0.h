/**
 * \file
 *
 * \brief Instance description for MCAN0
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

#ifndef _SAME70_MCAN0_INSTANCE_H_
#define _SAME70_MCAN0_INSTANCE_H_

/* ========== Register definition for MCAN0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_MCAN0_CUST          (0x40030008) /**< (MCAN0) Customer Register */
#define REG_MCAN0_FBTP          (0x4003000C) /**< (MCAN0) Fast Bit Timing and Prescaler Register */
#define REG_MCAN0_TEST          (0x40030010) /**< (MCAN0) Test Register */
#define REG_MCAN0_RWD           (0x40030014) /**< (MCAN0) RAM Watchdog Register */
#define REG_MCAN0_CCCR          (0x40030018) /**< (MCAN0) CC Control Register */
#define REG_MCAN0_BTP           (0x4003001C) /**< (MCAN0) Bit Timing and Prescaler Register */
#define REG_MCAN0_TSCC          (0x40030020) /**< (MCAN0) Timestamp Counter Configuration Register */
#define REG_MCAN0_TSCV          (0x40030024) /**< (MCAN0) Timestamp Counter Value Register */
#define REG_MCAN0_TOCC          (0x40030028) /**< (MCAN0) Timeout Counter Configuration Register */
#define REG_MCAN0_TOCV          (0x4003002C) /**< (MCAN0) Timeout Counter Value Register */
#define REG_MCAN0_ECR           (0x40030040) /**< (MCAN0) Error Counter Register */
#define REG_MCAN0_PSR           (0x40030044) /**< (MCAN0) Protocol Status Register */
#define REG_MCAN0_IR            (0x40030050) /**< (MCAN0) Interrupt Register */
#define REG_MCAN0_IE            (0x40030054) /**< (MCAN0) Interrupt Enable Register */
#define REG_MCAN0_ILS           (0x40030058) /**< (MCAN0) Interrupt Line Select Register */
#define REG_MCAN0_ILE           (0x4003005C) /**< (MCAN0) Interrupt Line Enable Register */
#define REG_MCAN0_GFC           (0x40030080) /**< (MCAN0) Global Filter Configuration Register */
#define REG_MCAN0_SIDFC         (0x40030084) /**< (MCAN0) Standard ID Filter Configuration Register */
#define REG_MCAN0_XIDFC         (0x40030088) /**< (MCAN0) Extended ID Filter Configuration Register */
#define REG_MCAN0_XIDAM         (0x40030090) /**< (MCAN0) Extended ID AND Mask Register */
#define REG_MCAN0_HPMS          (0x40030094) /**< (MCAN0) High Priority Message Status Register */
#define REG_MCAN0_NDAT1         (0x40030098) /**< (MCAN0) New Data 1 Register */
#define REG_MCAN0_NDAT2         (0x4003009C) /**< (MCAN0) New Data 2 Register */
#define REG_MCAN0_RXF0C         (0x400300A0) /**< (MCAN0) Receive FIFO 0 Configuration Register */
#define REG_MCAN0_RXF0S         (0x400300A4) /**< (MCAN0) Receive FIFO 0 Status Register */
#define REG_MCAN0_RXF0A         (0x400300A8) /**< (MCAN0) Receive FIFO 0 Acknowledge Register */
#define REG_MCAN0_RXBC          (0x400300AC) /**< (MCAN0) Receive Rx Buffer Configuration Register */
#define REG_MCAN0_RXF1C         (0x400300B0) /**< (MCAN0) Receive FIFO 1 Configuration Register */
#define REG_MCAN0_RXF1S         (0x400300B4) /**< (MCAN0) Receive FIFO 1 Status Register */
#define REG_MCAN0_RXF1A         (0x400300B8) /**< (MCAN0) Receive FIFO 1 Acknowledge Register */
#define REG_MCAN0_RXESC         (0x400300BC) /**< (MCAN0) Receive Buffer / FIFO Element Size Configuration Register */
#define REG_MCAN0_TXBC          (0x400300C0) /**< (MCAN0) Transmit Buffer Configuration Register */
#define REG_MCAN0_TXFQS         (0x400300C4) /**< (MCAN0) Transmit FIFO/Queue Status Register */
#define REG_MCAN0_TXESC         (0x400300C8) /**< (MCAN0) Transmit Buffer Element Size Configuration Register */
#define REG_MCAN0_TXBRP         (0x400300CC) /**< (MCAN0) Transmit Buffer Request Pending Register */
#define REG_MCAN0_TXBAR         (0x400300D0) /**< (MCAN0) Transmit Buffer Add Request Register */
#define REG_MCAN0_TXBCR         (0x400300D4) /**< (MCAN0) Transmit Buffer Cancellation Request Register */
#define REG_MCAN0_TXBTO         (0x400300D8) /**< (MCAN0) Transmit Buffer Transmission Occurred Register */
#define REG_MCAN0_TXBCF         (0x400300DC) /**< (MCAN0) Transmit Buffer Cancellation Finished Register */
#define REG_MCAN0_TXBTIE        (0x400300E0) /**< (MCAN0) Transmit Buffer Transmission Interrupt Enable Register */
#define REG_MCAN0_TXBCIE        (0x400300E4) /**< (MCAN0) Transmit Buffer Cancellation Finished Interrupt Enable Register */
#define REG_MCAN0_TXEFC         (0x400300F0) /**< (MCAN0) Transmit Event FIFO Configuration Register */
#define REG_MCAN0_TXEFS         (0x400300F4) /**< (MCAN0) Transmit Event FIFO Status Register */
#define REG_MCAN0_TXEFA         (0x400300F8) /**< (MCAN0) Transmit Event FIFO Acknowledge Register */

#else

#define REG_MCAN0_CUST          (*(__IO uint32_t*)0x40030008U) /**< (MCAN0) Customer Register */
#define REG_MCAN0_FBTP          (*(__IO uint32_t*)0x4003000CU) /**< (MCAN0) Fast Bit Timing and Prescaler Register */
#define REG_MCAN0_TEST          (*(__IO uint32_t*)0x40030010U) /**< (MCAN0) Test Register */
#define REG_MCAN0_RWD           (*(__IO uint32_t*)0x40030014U) /**< (MCAN0) RAM Watchdog Register */
#define REG_MCAN0_CCCR          (*(__IO uint32_t*)0x40030018U) /**< (MCAN0) CC Control Register */
#define REG_MCAN0_BTP           (*(__IO uint32_t*)0x4003001CU) /**< (MCAN0) Bit Timing and Prescaler Register */
#define REG_MCAN0_TSCC          (*(__IO uint32_t*)0x40030020U) /**< (MCAN0) Timestamp Counter Configuration Register */
#define REG_MCAN0_TSCV          (*(__IO uint32_t*)0x40030024U) /**< (MCAN0) Timestamp Counter Value Register */
#define REG_MCAN0_TOCC          (*(__IO uint32_t*)0x40030028U) /**< (MCAN0) Timeout Counter Configuration Register */
#define REG_MCAN0_TOCV          (*(__IO uint32_t*)0x4003002CU) /**< (MCAN0) Timeout Counter Value Register */
#define REG_MCAN0_ECR           (*(__I  uint32_t*)0x40030040U) /**< (MCAN0) Error Counter Register */
#define REG_MCAN0_PSR           (*(__I  uint32_t*)0x40030044U) /**< (MCAN0) Protocol Status Register */
#define REG_MCAN0_IR            (*(__IO uint32_t*)0x40030050U) /**< (MCAN0) Interrupt Register */
#define REG_MCAN0_IE            (*(__IO uint32_t*)0x40030054U) /**< (MCAN0) Interrupt Enable Register */
#define REG_MCAN0_ILS           (*(__IO uint32_t*)0x40030058U) /**< (MCAN0) Interrupt Line Select Register */
#define REG_MCAN0_ILE           (*(__IO uint32_t*)0x4003005CU) /**< (MCAN0) Interrupt Line Enable Register */
#define REG_MCAN0_GFC           (*(__IO uint32_t*)0x40030080U) /**< (MCAN0) Global Filter Configuration Register */
#define REG_MCAN0_SIDFC         (*(__IO uint32_t*)0x40030084U) /**< (MCAN0) Standard ID Filter Configuration Register */
#define REG_MCAN0_XIDFC         (*(__IO uint32_t*)0x40030088U) /**< (MCAN0) Extended ID Filter Configuration Register */
#define REG_MCAN0_XIDAM         (*(__IO uint32_t*)0x40030090U) /**< (MCAN0) Extended ID AND Mask Register */
#define REG_MCAN0_HPMS          (*(__I  uint32_t*)0x40030094U) /**< (MCAN0) High Priority Message Status Register */
#define REG_MCAN0_NDAT1         (*(__IO uint32_t*)0x40030098U) /**< (MCAN0) New Data 1 Register */
#define REG_MCAN0_NDAT2         (*(__IO uint32_t*)0x4003009CU) /**< (MCAN0) New Data 2 Register */
#define REG_MCAN0_RXF0C         (*(__IO uint32_t*)0x400300A0U) /**< (MCAN0) Receive FIFO 0 Configuration Register */
#define REG_MCAN0_RXF0S         (*(__I  uint32_t*)0x400300A4U) /**< (MCAN0) Receive FIFO 0 Status Register */
#define REG_MCAN0_RXF0A         (*(__IO uint32_t*)0x400300A8U) /**< (MCAN0) Receive FIFO 0 Acknowledge Register */
#define REG_MCAN0_RXBC          (*(__IO uint32_t*)0x400300ACU) /**< (MCAN0) Receive Rx Buffer Configuration Register */
#define REG_MCAN0_RXF1C         (*(__IO uint32_t*)0x400300B0U) /**< (MCAN0) Receive FIFO 1 Configuration Register */
#define REG_MCAN0_RXF1S         (*(__I  uint32_t*)0x400300B4U) /**< (MCAN0) Receive FIFO 1 Status Register */
#define REG_MCAN0_RXF1A         (*(__IO uint32_t*)0x400300B8U) /**< (MCAN0) Receive FIFO 1 Acknowledge Register */
#define REG_MCAN0_RXESC         (*(__IO uint32_t*)0x400300BCU) /**< (MCAN0) Receive Buffer / FIFO Element Size Configuration Register */
#define REG_MCAN0_TXBC          (*(__IO uint32_t*)0x400300C0U) /**< (MCAN0) Transmit Buffer Configuration Register */
#define REG_MCAN0_TXFQS         (*(__I  uint32_t*)0x400300C4U) /**< (MCAN0) Transmit FIFO/Queue Status Register */
#define REG_MCAN0_TXESC         (*(__IO uint32_t*)0x400300C8U) /**< (MCAN0) Transmit Buffer Element Size Configuration Register */
#define REG_MCAN0_TXBRP         (*(__I  uint32_t*)0x400300CCU) /**< (MCAN0) Transmit Buffer Request Pending Register */
#define REG_MCAN0_TXBAR         (*(__IO uint32_t*)0x400300D0U) /**< (MCAN0) Transmit Buffer Add Request Register */
#define REG_MCAN0_TXBCR         (*(__IO uint32_t*)0x400300D4U) /**< (MCAN0) Transmit Buffer Cancellation Request Register */
#define REG_MCAN0_TXBTO         (*(__I  uint32_t*)0x400300D8U) /**< (MCAN0) Transmit Buffer Transmission Occurred Register */
#define REG_MCAN0_TXBCF         (*(__I  uint32_t*)0x400300DCU) /**< (MCAN0) Transmit Buffer Cancellation Finished Register */
#define REG_MCAN0_TXBTIE        (*(__IO uint32_t*)0x400300E0U) /**< (MCAN0) Transmit Buffer Transmission Interrupt Enable Register */
#define REG_MCAN0_TXBCIE        (*(__IO uint32_t*)0x400300E4U) /**< (MCAN0) Transmit Buffer Cancellation Finished Interrupt Enable Register */
#define REG_MCAN0_TXEFC         (*(__IO uint32_t*)0x400300F0U) /**< (MCAN0) Transmit Event FIFO Configuration Register */
#define REG_MCAN0_TXEFS         (*(__I  uint32_t*)0x400300F4U) /**< (MCAN0) Transmit Event FIFO Status Register */
#define REG_MCAN0_TXEFA         (*(__IO uint32_t*)0x400300F8U) /**< (MCAN0) Transmit Event FIFO Acknowledge Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for MCAN0 peripheral ========== */
#define MCAN0_INSTANCE_ID                        35        

#endif /* _SAME70_MCAN0_INSTANCE_ */
