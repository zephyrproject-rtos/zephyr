/**
 * \file
 *
 * \brief Instance description for CAN0
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

#ifndef _SAME54_CAN0_INSTANCE_
#define _SAME54_CAN0_INSTANCE_

/* ========== Register definition for CAN0 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_CAN0_CREL              (0x42000000) /**< \brief (CAN0) Core Release */
#define REG_CAN0_ENDN              (0x42000004) /**< \brief (CAN0) Endian */
#define REG_CAN0_MRCFG             (0x42000008) /**< \brief (CAN0) Message RAM Configuration */
#define REG_CAN0_DBTP              (0x4200000C) /**< \brief (CAN0) Fast Bit Timing and Prescaler */
#define REG_CAN0_TEST              (0x42000010) /**< \brief (CAN0) Test */
#define REG_CAN0_RWD               (0x42000014) /**< \brief (CAN0) RAM Watchdog */
#define REG_CAN0_CCCR              (0x42000018) /**< \brief (CAN0) CC Control */
#define REG_CAN0_NBTP              (0x4200001C) /**< \brief (CAN0) Nominal Bit Timing and Prescaler */
#define REG_CAN0_TSCC              (0x42000020) /**< \brief (CAN0) Timestamp Counter Configuration */
#define REG_CAN0_TSCV              (0x42000024) /**< \brief (CAN0) Timestamp Counter Value */
#define REG_CAN0_TOCC              (0x42000028) /**< \brief (CAN0) Timeout Counter Configuration */
#define REG_CAN0_TOCV              (0x4200002C) /**< \brief (CAN0) Timeout Counter Value */
#define REG_CAN0_ECR               (0x42000040) /**< \brief (CAN0) Error Counter */
#define REG_CAN0_PSR               (0x42000044) /**< \brief (CAN0) Protocol Status */
#define REG_CAN0_TDCR              (0x42000048) /**< \brief (CAN0) Extended ID Filter Configuration */
#define REG_CAN0_IR                (0x42000050) /**< \brief (CAN0) Interrupt */
#define REG_CAN0_IE                (0x42000054) /**< \brief (CAN0) Interrupt Enable */
#define REG_CAN0_ILS               (0x42000058) /**< \brief (CAN0) Interrupt Line Select */
#define REG_CAN0_ILE               (0x4200005C) /**< \brief (CAN0) Interrupt Line Enable */
#define REG_CAN0_GFC               (0x42000080) /**< \brief (CAN0) Global Filter Configuration */
#define REG_CAN0_SIDFC             (0x42000084) /**< \brief (CAN0) Standard ID Filter Configuration */
#define REG_CAN0_XIDFC             (0x42000088) /**< \brief (CAN0) Extended ID Filter Configuration */
#define REG_CAN0_XIDAM             (0x42000090) /**< \brief (CAN0) Extended ID AND Mask */
#define REG_CAN0_HPMS              (0x42000094) /**< \brief (CAN0) High Priority Message Status */
#define REG_CAN0_NDAT1             (0x42000098) /**< \brief (CAN0) New Data 1 */
#define REG_CAN0_NDAT2             (0x4200009C) /**< \brief (CAN0) New Data 2 */
#define REG_CAN0_RXF0C             (0x420000A0) /**< \brief (CAN0) Rx FIFO 0 Configuration */
#define REG_CAN0_RXF0S             (0x420000A4) /**< \brief (CAN0) Rx FIFO 0 Status */
#define REG_CAN0_RXF0A             (0x420000A8) /**< \brief (CAN0) Rx FIFO 0 Acknowledge */
#define REG_CAN0_RXBC              (0x420000AC) /**< \brief (CAN0) Rx Buffer Configuration */
#define REG_CAN0_RXF1C             (0x420000B0) /**< \brief (CAN0) Rx FIFO 1 Configuration */
#define REG_CAN0_RXF1S             (0x420000B4) /**< \brief (CAN0) Rx FIFO 1 Status */
#define REG_CAN0_RXF1A             (0x420000B8) /**< \brief (CAN0) Rx FIFO 1 Acknowledge */
#define REG_CAN0_RXESC             (0x420000BC) /**< \brief (CAN0) Rx Buffer / FIFO Element Size Configuration */
#define REG_CAN0_TXBC              (0x420000C0) /**< \brief (CAN0) Tx Buffer Configuration */
#define REG_CAN0_TXFQS             (0x420000C4) /**< \brief (CAN0) Tx FIFO / Queue Status */
#define REG_CAN0_TXESC             (0x420000C8) /**< \brief (CAN0) Tx Buffer Element Size Configuration */
#define REG_CAN0_TXBRP             (0x420000CC) /**< \brief (CAN0) Tx Buffer Request Pending */
#define REG_CAN0_TXBAR             (0x420000D0) /**< \brief (CAN0) Tx Buffer Add Request */
#define REG_CAN0_TXBCR             (0x420000D4) /**< \brief (CAN0) Tx Buffer Cancellation Request */
#define REG_CAN0_TXBTO             (0x420000D8) /**< \brief (CAN0) Tx Buffer Transmission Occurred */
#define REG_CAN0_TXBCF             (0x420000DC) /**< \brief (CAN0) Tx Buffer Cancellation Finished */
#define REG_CAN0_TXBTIE            (0x420000E0) /**< \brief (CAN0) Tx Buffer Transmission Interrupt Enable */
#define REG_CAN0_TXBCIE            (0x420000E4) /**< \brief (CAN0) Tx Buffer Cancellation Finished Interrupt Enable */
#define REG_CAN0_TXEFC             (0x420000F0) /**< \brief (CAN0) Tx Event FIFO Configuration */
#define REG_CAN0_TXEFS             (0x420000F4) /**< \brief (CAN0) Tx Event FIFO Status */
#define REG_CAN0_TXEFA             (0x420000F8) /**< \brief (CAN0) Tx Event FIFO Acknowledge */
#else
#define REG_CAN0_CREL              (*(RoReg  *)0x42000000UL) /**< \brief (CAN0) Core Release */
#define REG_CAN0_ENDN              (*(RoReg  *)0x42000004UL) /**< \brief (CAN0) Endian */
#define REG_CAN0_MRCFG             (*(RwReg  *)0x42000008UL) /**< \brief (CAN0) Message RAM Configuration */
#define REG_CAN0_DBTP              (*(RwReg  *)0x4200000CUL) /**< \brief (CAN0) Fast Bit Timing and Prescaler */
#define REG_CAN0_TEST              (*(RwReg  *)0x42000010UL) /**< \brief (CAN0) Test */
#define REG_CAN0_RWD               (*(RwReg  *)0x42000014UL) /**< \brief (CAN0) RAM Watchdog */
#define REG_CAN0_CCCR              (*(RwReg  *)0x42000018UL) /**< \brief (CAN0) CC Control */
#define REG_CAN0_NBTP              (*(RwReg  *)0x4200001CUL) /**< \brief (CAN0) Nominal Bit Timing and Prescaler */
#define REG_CAN0_TSCC              (*(RwReg  *)0x42000020UL) /**< \brief (CAN0) Timestamp Counter Configuration */
#define REG_CAN0_TSCV              (*(RoReg  *)0x42000024UL) /**< \brief (CAN0) Timestamp Counter Value */
#define REG_CAN0_TOCC              (*(RwReg  *)0x42000028UL) /**< \brief (CAN0) Timeout Counter Configuration */
#define REG_CAN0_TOCV              (*(RwReg  *)0x4200002CUL) /**< \brief (CAN0) Timeout Counter Value */
#define REG_CAN0_ECR               (*(RoReg  *)0x42000040UL) /**< \brief (CAN0) Error Counter */
#define REG_CAN0_PSR               (*(RoReg  *)0x42000044UL) /**< \brief (CAN0) Protocol Status */
#define REG_CAN0_TDCR              (*(RwReg  *)0x42000048UL) /**< \brief (CAN0) Extended ID Filter Configuration */
#define REG_CAN0_IR                (*(RwReg  *)0x42000050UL) /**< \brief (CAN0) Interrupt */
#define REG_CAN0_IE                (*(RwReg  *)0x42000054UL) /**< \brief (CAN0) Interrupt Enable */
#define REG_CAN0_ILS               (*(RwReg  *)0x42000058UL) /**< \brief (CAN0) Interrupt Line Select */
#define REG_CAN0_ILE               (*(RwReg  *)0x4200005CUL) /**< \brief (CAN0) Interrupt Line Enable */
#define REG_CAN0_GFC               (*(RwReg  *)0x42000080UL) /**< \brief (CAN0) Global Filter Configuration */
#define REG_CAN0_SIDFC             (*(RwReg  *)0x42000084UL) /**< \brief (CAN0) Standard ID Filter Configuration */
#define REG_CAN0_XIDFC             (*(RwReg  *)0x42000088UL) /**< \brief (CAN0) Extended ID Filter Configuration */
#define REG_CAN0_XIDAM             (*(RwReg  *)0x42000090UL) /**< \brief (CAN0) Extended ID AND Mask */
#define REG_CAN0_HPMS              (*(RoReg  *)0x42000094UL) /**< \brief (CAN0) High Priority Message Status */
#define REG_CAN0_NDAT1             (*(RwReg  *)0x42000098UL) /**< \brief (CAN0) New Data 1 */
#define REG_CAN0_NDAT2             (*(RwReg  *)0x4200009CUL) /**< \brief (CAN0) New Data 2 */
#define REG_CAN0_RXF0C             (*(RwReg  *)0x420000A0UL) /**< \brief (CAN0) Rx FIFO 0 Configuration */
#define REG_CAN0_RXF0S             (*(RoReg  *)0x420000A4UL) /**< \brief (CAN0) Rx FIFO 0 Status */
#define REG_CAN0_RXF0A             (*(RwReg  *)0x420000A8UL) /**< \brief (CAN0) Rx FIFO 0 Acknowledge */
#define REG_CAN0_RXBC              (*(RwReg  *)0x420000ACUL) /**< \brief (CAN0) Rx Buffer Configuration */
#define REG_CAN0_RXF1C             (*(RwReg  *)0x420000B0UL) /**< \brief (CAN0) Rx FIFO 1 Configuration */
#define REG_CAN0_RXF1S             (*(RoReg  *)0x420000B4UL) /**< \brief (CAN0) Rx FIFO 1 Status */
#define REG_CAN0_RXF1A             (*(RwReg  *)0x420000B8UL) /**< \brief (CAN0) Rx FIFO 1 Acknowledge */
#define REG_CAN0_RXESC             (*(RwReg  *)0x420000BCUL) /**< \brief (CAN0) Rx Buffer / FIFO Element Size Configuration */
#define REG_CAN0_TXBC              (*(RwReg  *)0x420000C0UL) /**< \brief (CAN0) Tx Buffer Configuration */
#define REG_CAN0_TXFQS             (*(RoReg  *)0x420000C4UL) /**< \brief (CAN0) Tx FIFO / Queue Status */
#define REG_CAN0_TXESC             (*(RwReg  *)0x420000C8UL) /**< \brief (CAN0) Tx Buffer Element Size Configuration */
#define REG_CAN0_TXBRP             (*(RoReg  *)0x420000CCUL) /**< \brief (CAN0) Tx Buffer Request Pending */
#define REG_CAN0_TXBAR             (*(RwReg  *)0x420000D0UL) /**< \brief (CAN0) Tx Buffer Add Request */
#define REG_CAN0_TXBCR             (*(RwReg  *)0x420000D4UL) /**< \brief (CAN0) Tx Buffer Cancellation Request */
#define REG_CAN0_TXBTO             (*(RoReg  *)0x420000D8UL) /**< \brief (CAN0) Tx Buffer Transmission Occurred */
#define REG_CAN0_TXBCF             (*(RoReg  *)0x420000DCUL) /**< \brief (CAN0) Tx Buffer Cancellation Finished */
#define REG_CAN0_TXBTIE            (*(RwReg  *)0x420000E0UL) /**< \brief (CAN0) Tx Buffer Transmission Interrupt Enable */
#define REG_CAN0_TXBCIE            (*(RwReg  *)0x420000E4UL) /**< \brief (CAN0) Tx Buffer Cancellation Finished Interrupt Enable */
#define REG_CAN0_TXEFC             (*(RwReg  *)0x420000F0UL) /**< \brief (CAN0) Tx Event FIFO Configuration */
#define REG_CAN0_TXEFS             (*(RoReg  *)0x420000F4UL) /**< \brief (CAN0) Tx Event FIFO Status */
#define REG_CAN0_TXEFA             (*(RwReg  *)0x420000F8UL) /**< \brief (CAN0) Tx Event FIFO Acknowledge */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for CAN0 peripheral ========== */
#define CAN0_CLK_AHB_ID             17       // Index of AHB clock
#define CAN0_DMAC_ID_DEBUG          20       // DMA CAN Debug Req
#define CAN0_GCLK_ID                27       // Index of Generic Clock
#define CAN0_MSG_RAM_ADDR           0x20000000
#define CAN0_QOS_RESET_VAL          1        // QOS reset value

#endif /* _SAME54_CAN0_INSTANCE_ */
