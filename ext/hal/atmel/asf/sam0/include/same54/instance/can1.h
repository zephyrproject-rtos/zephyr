/**
 * \file
 *
 * \brief Instance description for CAN1
 *
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAME54_CAN1_INSTANCE_
#define _SAME54_CAN1_INSTANCE_

/* ========== Register definition for CAN1 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_CAN1_CREL              (0x42000400) /**< \brief (CAN1) Core Release */
#define REG_CAN1_ENDN              (0x42000404) /**< \brief (CAN1) Endian */
#define REG_CAN1_MRCFG             (0x42000408) /**< \brief (CAN1) Message RAM Configuration */
#define REG_CAN1_DBTP              (0x4200040C) /**< \brief (CAN1) Fast Bit Timing and Prescaler */
#define REG_CAN1_TEST              (0x42000410) /**< \brief (CAN1) Test */
#define REG_CAN1_RWD               (0x42000414) /**< \brief (CAN1) RAM Watchdog */
#define REG_CAN1_CCCR              (0x42000418) /**< \brief (CAN1) CC Control */
#define REG_CAN1_NBTP              (0x4200041C) /**< \brief (CAN1) Nominal Bit Timing and Prescaler */
#define REG_CAN1_TSCC              (0x42000420) /**< \brief (CAN1) Timestamp Counter Configuration */
#define REG_CAN1_TSCV              (0x42000424) /**< \brief (CAN1) Timestamp Counter Value */
#define REG_CAN1_TOCC              (0x42000428) /**< \brief (CAN1) Timeout Counter Configuration */
#define REG_CAN1_TOCV              (0x4200042C) /**< \brief (CAN1) Timeout Counter Value */
#define REG_CAN1_ECR               (0x42000440) /**< \brief (CAN1) Error Counter */
#define REG_CAN1_PSR               (0x42000444) /**< \brief (CAN1) Protocol Status */
#define REG_CAN1_TDCR              (0x42000448) /**< \brief (CAN1) Extended ID Filter Configuration */
#define REG_CAN1_IR                (0x42000450) /**< \brief (CAN1) Interrupt */
#define REG_CAN1_IE                (0x42000454) /**< \brief (CAN1) Interrupt Enable */
#define REG_CAN1_ILS               (0x42000458) /**< \brief (CAN1) Interrupt Line Select */
#define REG_CAN1_ILE               (0x4200045C) /**< \brief (CAN1) Interrupt Line Enable */
#define REG_CAN1_GFC               (0x42000480) /**< \brief (CAN1) Global Filter Configuration */
#define REG_CAN1_SIDFC             (0x42000484) /**< \brief (CAN1) Standard ID Filter Configuration */
#define REG_CAN1_XIDFC             (0x42000488) /**< \brief (CAN1) Extended ID Filter Configuration */
#define REG_CAN1_XIDAM             (0x42000490) /**< \brief (CAN1) Extended ID AND Mask */
#define REG_CAN1_HPMS              (0x42000494) /**< \brief (CAN1) High Priority Message Status */
#define REG_CAN1_NDAT1             (0x42000498) /**< \brief (CAN1) New Data 1 */
#define REG_CAN1_NDAT2             (0x4200049C) /**< \brief (CAN1) New Data 2 */
#define REG_CAN1_RXF0C             (0x420004A0) /**< \brief (CAN1) Rx FIFO 0 Configuration */
#define REG_CAN1_RXF0S             (0x420004A4) /**< \brief (CAN1) Rx FIFO 0 Status */
#define REG_CAN1_RXF0A             (0x420004A8) /**< \brief (CAN1) Rx FIFO 0 Acknowledge */
#define REG_CAN1_RXBC              (0x420004AC) /**< \brief (CAN1) Rx Buffer Configuration */
#define REG_CAN1_RXF1C             (0x420004B0) /**< \brief (CAN1) Rx FIFO 1 Configuration */
#define REG_CAN1_RXF1S             (0x420004B4) /**< \brief (CAN1) Rx FIFO 1 Status */
#define REG_CAN1_RXF1A             (0x420004B8) /**< \brief (CAN1) Rx FIFO 1 Acknowledge */
#define REG_CAN1_RXESC             (0x420004BC) /**< \brief (CAN1) Rx Buffer / FIFO Element Size Configuration */
#define REG_CAN1_TXBC              (0x420004C0) /**< \brief (CAN1) Tx Buffer Configuration */
#define REG_CAN1_TXFQS             (0x420004C4) /**< \brief (CAN1) Tx FIFO / Queue Status */
#define REG_CAN1_TXESC             (0x420004C8) /**< \brief (CAN1) Tx Buffer Element Size Configuration */
#define REG_CAN1_TXBRP             (0x420004CC) /**< \brief (CAN1) Tx Buffer Request Pending */
#define REG_CAN1_TXBAR             (0x420004D0) /**< \brief (CAN1) Tx Buffer Add Request */
#define REG_CAN1_TXBCR             (0x420004D4) /**< \brief (CAN1) Tx Buffer Cancellation Request */
#define REG_CAN1_TXBTO             (0x420004D8) /**< \brief (CAN1) Tx Buffer Transmission Occurred */
#define REG_CAN1_TXBCF             (0x420004DC) /**< \brief (CAN1) Tx Buffer Cancellation Finished */
#define REG_CAN1_TXBTIE            (0x420004E0) /**< \brief (CAN1) Tx Buffer Transmission Interrupt Enable */
#define REG_CAN1_TXBCIE            (0x420004E4) /**< \brief (CAN1) Tx Buffer Cancellation Finished Interrupt Enable */
#define REG_CAN1_TXEFC             (0x420004F0) /**< \brief (CAN1) Tx Event FIFO Configuration */
#define REG_CAN1_TXEFS             (0x420004F4) /**< \brief (CAN1) Tx Event FIFO Status */
#define REG_CAN1_TXEFA             (0x420004F8) /**< \brief (CAN1) Tx Event FIFO Acknowledge */
#else
#define REG_CAN1_CREL              (*(RoReg  *)0x42000400UL) /**< \brief (CAN1) Core Release */
#define REG_CAN1_ENDN              (*(RoReg  *)0x42000404UL) /**< \brief (CAN1) Endian */
#define REG_CAN1_MRCFG             (*(RwReg  *)0x42000408UL) /**< \brief (CAN1) Message RAM Configuration */
#define REG_CAN1_DBTP              (*(RwReg  *)0x4200040CUL) /**< \brief (CAN1) Fast Bit Timing and Prescaler */
#define REG_CAN1_TEST              (*(RwReg  *)0x42000410UL) /**< \brief (CAN1) Test */
#define REG_CAN1_RWD               (*(RwReg  *)0x42000414UL) /**< \brief (CAN1) RAM Watchdog */
#define REG_CAN1_CCCR              (*(RwReg  *)0x42000418UL) /**< \brief (CAN1) CC Control */
#define REG_CAN1_NBTP              (*(RwReg  *)0x4200041CUL) /**< \brief (CAN1) Nominal Bit Timing and Prescaler */
#define REG_CAN1_TSCC              (*(RwReg  *)0x42000420UL) /**< \brief (CAN1) Timestamp Counter Configuration */
#define REG_CAN1_TSCV              (*(RoReg  *)0x42000424UL) /**< \brief (CAN1) Timestamp Counter Value */
#define REG_CAN1_TOCC              (*(RwReg  *)0x42000428UL) /**< \brief (CAN1) Timeout Counter Configuration */
#define REG_CAN1_TOCV              (*(RwReg  *)0x4200042CUL) /**< \brief (CAN1) Timeout Counter Value */
#define REG_CAN1_ECR               (*(RoReg  *)0x42000440UL) /**< \brief (CAN1) Error Counter */
#define REG_CAN1_PSR               (*(RoReg  *)0x42000444UL) /**< \brief (CAN1) Protocol Status */
#define REG_CAN1_TDCR              (*(RwReg  *)0x42000448UL) /**< \brief (CAN1) Extended ID Filter Configuration */
#define REG_CAN1_IR                (*(RwReg  *)0x42000450UL) /**< \brief (CAN1) Interrupt */
#define REG_CAN1_IE                (*(RwReg  *)0x42000454UL) /**< \brief (CAN1) Interrupt Enable */
#define REG_CAN1_ILS               (*(RwReg  *)0x42000458UL) /**< \brief (CAN1) Interrupt Line Select */
#define REG_CAN1_ILE               (*(RwReg  *)0x4200045CUL) /**< \brief (CAN1) Interrupt Line Enable */
#define REG_CAN1_GFC               (*(RwReg  *)0x42000480UL) /**< \brief (CAN1) Global Filter Configuration */
#define REG_CAN1_SIDFC             (*(RwReg  *)0x42000484UL) /**< \brief (CAN1) Standard ID Filter Configuration */
#define REG_CAN1_XIDFC             (*(RwReg  *)0x42000488UL) /**< \brief (CAN1) Extended ID Filter Configuration */
#define REG_CAN1_XIDAM             (*(RwReg  *)0x42000490UL) /**< \brief (CAN1) Extended ID AND Mask */
#define REG_CAN1_HPMS              (*(RoReg  *)0x42000494UL) /**< \brief (CAN1) High Priority Message Status */
#define REG_CAN1_NDAT1             (*(RwReg  *)0x42000498UL) /**< \brief (CAN1) New Data 1 */
#define REG_CAN1_NDAT2             (*(RwReg  *)0x4200049CUL) /**< \brief (CAN1) New Data 2 */
#define REG_CAN1_RXF0C             (*(RwReg  *)0x420004A0UL) /**< \brief (CAN1) Rx FIFO 0 Configuration */
#define REG_CAN1_RXF0S             (*(RoReg  *)0x420004A4UL) /**< \brief (CAN1) Rx FIFO 0 Status */
#define REG_CAN1_RXF0A             (*(RwReg  *)0x420004A8UL) /**< \brief (CAN1) Rx FIFO 0 Acknowledge */
#define REG_CAN1_RXBC              (*(RwReg  *)0x420004ACUL) /**< \brief (CAN1) Rx Buffer Configuration */
#define REG_CAN1_RXF1C             (*(RwReg  *)0x420004B0UL) /**< \brief (CAN1) Rx FIFO 1 Configuration */
#define REG_CAN1_RXF1S             (*(RoReg  *)0x420004B4UL) /**< \brief (CAN1) Rx FIFO 1 Status */
#define REG_CAN1_RXF1A             (*(RwReg  *)0x420004B8UL) /**< \brief (CAN1) Rx FIFO 1 Acknowledge */
#define REG_CAN1_RXESC             (*(RwReg  *)0x420004BCUL) /**< \brief (CAN1) Rx Buffer / FIFO Element Size Configuration */
#define REG_CAN1_TXBC              (*(RwReg  *)0x420004C0UL) /**< \brief (CAN1) Tx Buffer Configuration */
#define REG_CAN1_TXFQS             (*(RoReg  *)0x420004C4UL) /**< \brief (CAN1) Tx FIFO / Queue Status */
#define REG_CAN1_TXESC             (*(RwReg  *)0x420004C8UL) /**< \brief (CAN1) Tx Buffer Element Size Configuration */
#define REG_CAN1_TXBRP             (*(RoReg  *)0x420004CCUL) /**< \brief (CAN1) Tx Buffer Request Pending */
#define REG_CAN1_TXBAR             (*(RwReg  *)0x420004D0UL) /**< \brief (CAN1) Tx Buffer Add Request */
#define REG_CAN1_TXBCR             (*(RwReg  *)0x420004D4UL) /**< \brief (CAN1) Tx Buffer Cancellation Request */
#define REG_CAN1_TXBTO             (*(RoReg  *)0x420004D8UL) /**< \brief (CAN1) Tx Buffer Transmission Occurred */
#define REG_CAN1_TXBCF             (*(RoReg  *)0x420004DCUL) /**< \brief (CAN1) Tx Buffer Cancellation Finished */
#define REG_CAN1_TXBTIE            (*(RwReg  *)0x420004E0UL) /**< \brief (CAN1) Tx Buffer Transmission Interrupt Enable */
#define REG_CAN1_TXBCIE            (*(RwReg  *)0x420004E4UL) /**< \brief (CAN1) Tx Buffer Cancellation Finished Interrupt Enable */
#define REG_CAN1_TXEFC             (*(RwReg  *)0x420004F0UL) /**< \brief (CAN1) Tx Event FIFO Configuration */
#define REG_CAN1_TXEFS             (*(RoReg  *)0x420004F4UL) /**< \brief (CAN1) Tx Event FIFO Status */
#define REG_CAN1_TXEFA             (*(RwReg  *)0x420004F8UL) /**< \brief (CAN1) Tx Event FIFO Acknowledge */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for CAN1 peripheral ========== */
#define CAN1_CLK_AHB_ID             18       // Index of AHB clock
#define CAN1_DMAC_ID_DEBUG          21       // DMA CAN Debug Req
#define CAN1_GCLK_ID                28       // Index of Generic Clock
#define CAN1_MSG_RAM_ADDR           0x20000000
#define CAN1_QOS_RESET_VAL          1        // QOS reset value

#endif /* _SAME54_CAN1_INSTANCE_ */
