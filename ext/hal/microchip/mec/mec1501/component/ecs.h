/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
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

/** @file ecs.h
 *MEC1501 EC Subsystem (ECS) registers
 */
/** @defgroup MEC1501 Peripherals ECS
 */

#include <stdint.h>
#include <stddef.h>

#ifndef _ECS_H
#define _ECS_H

/* =========================================================================*/
/* ================            ECS                         ================ */
/* =========================================================================*/

#define ECS_AHB_ERR_CTRL_DIS_POS    (1ul << 0)
#define ECS_AHB_ERR_CTRL_DIS        (1ul << (ECS_AHB_ERR_CTRL_DIS_POS))

#define ECS_INTR_CNTL_DIRECT_POS    0
#define ECS_INTR_CNTL_DIRECT_EN     (1ul << (ECS_INTR_CNTL_DIRECT_POS))

#define ECS_ETM_CTRL_EN_POS         0
#define ECS_ETM_CTRL_EN             (1ul << (ECS_ETM_CTRL_EN_POS))

#define ECS_DBG_CTRL_MASK           0x1Ful
#define ECS_DBG_CTRL_DBG_EN_POS     0
#define ECS_DBG_CTRL_DBG_EN         (1ul << (ECS_DBG_CTRL_DBG_EN_POS))
#define ECS_DBG_CTRL_MODE_POS       1
#define ECS_DBG_CTRL_MODE_MASK0     0x03
#define ECS_DBG_CTRL_MODE_MASK      ((ECS_DBG_CTRL_DBG_MODE_MASK0) << (ECS_DBG_CTRL_DBG_MODE_POS))
#define ECS_DBG_CTRL_MODE_JTAG      (0x00 << (ECS_DBG_CTRL_DBG_MODE_POS))
#define ECS_DBG_CTRL_MODE_SWD       (0x02 << (ECS_DBG_CTRL_DBG_MODE_POS))
#define ECS_DBG_CTRL_MODE_SWD_SWV   (0x01 << (ECS_DBG_CTRL_DBG_MODE_POS))
#define ECS_DBG_CTRL_PUEN_POS       3
#define ECS_DBG_CTRL_PUEN           (1ul << (ECS_DBG_CTRL_PUEN_POS))
#define ECS_DBG_CTRL_BSCAN_POS      4
#define ECS_DBG_CTRL_BSCAN_EN       (1ul << (ECS_DBG_CTRL_BSCAN_POS))

#define ECS_AESH_BSWAP_MASK         0xFFul
#define ECS_DW_SWAP_IN_POS          0
#define ECS_DW_SWAP_IN_EN           (1ul << (ECS_DW_SWAP_IN_POS))
#define ECS_DW_SWAP_OUT_POS         1
#define ECS_DW_SWAP_OUT_EN          (1ul << (ECS_DW_SWAP_OUT_POS))
#define ECS_BLK_SWAP_IN_POS         2
#define ECS_BLK_SWAP_IN_MASK        (0x07 << (ECS_BLK_SWAP_IN_POS))
#define ECS_BLK_SWAP_IN_DIS         (0x00 << (ECS_BLK_SWAP_IN_POS))
#define ECS_BLK_SWAP_IN_8B          (0x01 << (ECS_BLK_SWAP_IN_POS))
#define ECS_BLK_SWAP_IN_16B         (0x02 << (ECS_BLK_SWAP_IN_POS))
#define ECS_BLK_SWAP_IN_64B         (0x03 << (ECS_BLK_SWAP_IN_POS))
#define ECS_BLK_SWAP_IN_128B        (0x04 << (ECS_BLK_SWAP_IN_POS))
#define ECS_BLK_SWAP_OUT_POS        5
#define ECS_BLK_SWAP_OUT_MASK       (0x07 << (ECS_BLK_SWAP_OUT_POS))
#define ECS_BLK_SWAP_OUT_DIS        (0x00 << (ECS_BLK_SWAP_OUT_POS))
#define ECS_BLK_SWAP_OUT_8B         (0x01 << (ECS_BLK_SWAP_OUT_POS))
#define ECS_BLK_SWAP_OUT_16B        (0x02 << (ECS_BLK_SWAP_OUT_POS))
#define ECS_BLK_SWAP_OUT_64B        (0x03 << (ECS_BLK_SWAP_OUT_POS))
#define ECS_BLK_SWAP_OUT_128B       (0x04 << (ECS_BLK_SWAP_OUT_POS))

/* EC Subystem GPIO Bank Power */
#define ECS_GBPWR_LOCK_POS          7
#define ECS_GBPWR_LOCK              (1ul << (ECS_GBPWR_LOCK_POS))
#define ECS_VTR3_LVL_POS            2
#define ECS_VTR3_LVL_18             (1ul << (ECS_VTR3_LVL_POS))
#define ECS_VTR2_LVL_POS            1
#define ECS_VTR2_LVL_18             (1ul << (ECS_VTR2_LVL_POS))

/* EC Subsystem Comparator Control */
#define ECS_CC_CMP0_EN_POS          0
#define ECS_CC_CMP0_EN              (1ul  << (ECS_CC_CMP0_EN_POS))
#define ECS_CC_CMP0_CFG_LOCK_POS    2
#define ECS_CC_CMP0_CFG_LOCK        (1ul << (ECS_CC_CMP0_CFG_LOCK_POS))
#define ECS_CC_CMP1_EN_POS          4
#define ECS_CC_CMP1_EN              (1ul << (ECS_CC_CMP1_EN_POS))

/* EC Subsystem Comparator Sleep Control */
#define ECS_CSC_CMP0_SLP_EN_POS     0
#define ECS_CSC_CMP0_SLP_EN         (1ul << (ECS_CSC_CMP0_SLP_EN_POS))
#define ECS_CSC_CMP1_SLP_EN_POS     1
#define ECS_CSC_CMP1_SLP_EN         (1ul << (ECS_CSC_CMP1_SLP_EN_POS))

/**
  * @brief EC Subsystem (ECS)
  */
typedef struct
{                               /*!< (@ 0x4000FC00) ECS Structure   */
    __IOM uint8_t   RSVD1[4];
    __IOM uint32_t  AHB_ERR_ADDR;       /*!< (@ 0x0004) ECS AHB Error Address */
    __IOM uint32_t  TEST08;
    __IOM uint32_t  TEST0C;
    __IOM uint32_t  TEST10;
    __IOM uint32_t  AHB_ERR_CTRL;       /*!< (@ 0x0014) ECS AHB Error Control */
    __IOM uint32_t  INTR_CTRL;          /*!< (@ 0x0018) ECS Interupt Control */
    __IOM uint32_t  ETM_CTRL;           /*!< (@ 0x001C) ECS ETM Trace Control */
    __IOM uint32_t  DEBUG_CTRL;         /*!< (@ 0x0020) ECS Debug Control */
    __IOM uint32_t  OTP_LOCK;           /*!< (@ 0x0024) ECS OTP Lock Enable */
    __IOM uint32_t  WDT_CNT;            /*!< (@ 0x0028) ECS WDT Event Count */
    __IOM uint32_t  AESH_BSWAP_CTRL;    /*!< (@ 0x002C) ECS AES-Hash Byte Swap Control */
    __IOM uint32_t  TEST30;
    __IOM uint32_t  TEST34;
    __IOM uint32_t  ADC_VREF_PWRDN;     /*!< (@ 0x0038) ECS ADC Vref Power Down */
    __IOM uint32_t  TEST3C;
    __IOM uint32_t  PECI_DIS;           /*!< (@ 0x0040) ECS PECI Disable */
    __IOM uint32_t  GPIO_PAD_TST;       /*!< (@ 0x0044) ECS GPIO Pad Test */
    __IOM uint32_t  SMBUS_SW_EN0;       /*!< (@ 0x0048) ECS SMBus SW Enable 0 */
    __IOM uint32_t  STAP_TMIR;          /*!< (@ 0x004C) ECS STAP Test Mirror */
    __IOM uint32_t  VCI_FW_OVR;         /*!< (@ 0x0050) ECS VCI FW Override */
    __IOM uint32_t  BROM_STS;           /*!< (@ 0x0054) ECS Boot-ROM Status */
          uint8_t   RSVD2[4];
    __IOM uint32_t  CRYPTO_SRST;        /*!< (@ 0x005C) ECS Crypto HW Soft Reset */
    __IOM uint32_t  TEST60;
    __IOM uint32_t  GPIO_BANK_PWR;      /*!< (@ 0x0064) ECS GPIO Bank Power Select */
    __IOM uint32_t  TEST68;
    __IOM uint32_t  TEST6C;
    __IOM uint32_t  JTAG_MCFG;          /*!< (@ 0x0070) ECS JTAG Master Config */
    __IOM uint32_t  JTAG_MSTS;          /*!< (@ 0x0074) ECS JTAG Master Status */
    __IOM uint32_t  JTAG_MTDO;          /*!< (@ 0x0078) ECS JTAG Master TDO */
    __IOM uint32_t  JTAG_MTDI;          /*!< (@ 0x007C) ECS JTAG Master TDI */
    __IOM uint32_t  JTAG_MTMS;          /*!< (@ 0x0080) ECS JTAG Master TMS */
    __IOM uint32_t  JTAG_MCMD;          /*!< (@ 0x0084) ECS JTAG Master Command */
          uint8_t   RSVD3[8];
    __IOM uint32_t  VW_FW_OVR;          /*!< (@ 0x0090) ECS VWire FW Override */
    __IOM uint32_t  CMP_CTRL;           /*!< (@ 0x0094) ECS Analog Comparator Control */
    __IOM uint32_t  CMP_SLP_CTRL;       /*!< (@ 0x0098) ECS Analog Comparator Sleep Control */
          uint8_t   RSVD4[(0xF0 - 0x9C)];
    __IOM uint32_t  IP_TRIM;            /*!< (@ 0x00F0) ECS IP Trim */
          uint8_t   RSVD5[12];
    __IOM uint32_t  TEST100;
          uint8_t   RSVD6[(0x180 - 0x94)];
    __IOM uint32_t  FW_SCR0;            /*!< (@ 0x0180) ECS FW Scratch 0 */
    __IOM uint32_t  FW_SCR1;            /*!< (@ 0x0180) ECS FW Scratch 1 */
    __IOM uint32_t  FW_SCR2;            /*!< (@ 0x0180) ECS FW Scratch 2 */
    __IOM uint32_t  FW_SCR3;            /*!< (@ 0x0180) ECS FW Scratch 3 */
} ECS_Type;

#endif // #ifndef _ECS_H
/* end ecs.h */
/**   @}
 */

