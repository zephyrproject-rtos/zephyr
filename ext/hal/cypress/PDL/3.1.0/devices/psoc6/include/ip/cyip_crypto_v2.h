/***************************************************************************//**
* \file cyip_crypto_v2.h
*
* \brief
* CRYPTO IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_CRYPTO_V2_H_
#define _CYIP_CRYPTO_V2_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    CRYPTO
*******************************************************************************/

#define CRYPTO_V2_SECTION_SIZE                  0x00010000UL

/**
  * \brief Cryptography component (CRYPTO)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t RESERVED;
  __IOM uint32_t RAM_PWR_CTL;                   /*!< 0x00000008 SRAM power control */
  __IOM uint32_t RAM_PWR_DELAY_CTL;             /*!< 0x0000000C SRAM power delay control */
  __IOM uint32_t ECC_CTL;                       /*!< 0x00000010 ECC control */
   __IM uint32_t RESERVED1[3];
   __IM uint32_t ERROR_STATUS0;                 /*!< 0x00000020 Error status 0 */
  __IOM uint32_t ERROR_STATUS1;                 /*!< 0x00000024 Error status 1 */
   __IM uint32_t RESERVED2[54];
  __IOM uint32_t INTR;                          /*!< 0x00000100 Interrupt register */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000104 Interrupt set register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000108 Interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000010C Interrupt masked register */
   __IM uint32_t RESERVED3[60];
  __IOM uint32_t PR_LFSR_CTL0;                  /*!< 0x00000200 Pseudo random LFSR control 0 */
  __IOM uint32_t PR_LFSR_CTL1;                  /*!< 0x00000204 Pseudo random LFSR control 1 */
  __IOM uint32_t PR_LFSR_CTL2;                  /*!< 0x00000208 Pseudo random LFSR control 2 */
  __IOM uint32_t PR_MAX_CTL;                    /*!< 0x0000020C Pseudo random maximum control */
  __IOM uint32_t PR_CMD;                        /*!< 0x00000210 Pseudo random command */
   __IM uint32_t RESERVED4;
  __IOM uint32_t PR_RESULT;                     /*!< 0x00000218 Pseudo random result */
   __IM uint32_t RESERVED5[25];
  __IOM uint32_t TR_CTL0;                       /*!< 0x00000280 True random control 0 */
  __IOM uint32_t TR_CTL1;                       /*!< 0x00000284 True random control 1 */
  __IOM uint32_t TR_CTL2;                       /*!< 0x00000288 True random control 2 */
   __IM uint32_t TR_STATUS;                     /*!< 0x0000028C True random status */
  __IOM uint32_t TR_CMD;                        /*!< 0x00000290 True random command */
   __IM uint32_t RESERVED6;
  __IOM uint32_t TR_RESULT;                     /*!< 0x00000298 True random result */
   __IM uint32_t RESERVED7;
  __IOM uint32_t TR_GARO_CTL;                   /*!< 0x000002A0 True random GARO control */
  __IOM uint32_t TR_FIRO_CTL;                   /*!< 0x000002A4 True random FIRO control */
   __IM uint32_t RESERVED8[6];
  __IOM uint32_t TR_MON_CTL;                    /*!< 0x000002C0 True random monitor control */
   __IM uint32_t RESERVED9;
  __IOM uint32_t TR_MON_CMD;                    /*!< 0x000002C8 True random monitor command */
   __IM uint32_t RESERVED10;
  __IOM uint32_t TR_MON_RC_CTL;                 /*!< 0x000002D0 True random monitor RC control */
   __IM uint32_t RESERVED11;
   __IM uint32_t TR_MON_RC_STATUS0;             /*!< 0x000002D8 True random monitor RC status 0 */
   __IM uint32_t TR_MON_RC_STATUS1;             /*!< 0x000002DC True random monitor RC status 1 */
  __IOM uint32_t TR_MON_AP_CTL;                 /*!< 0x000002E0 True random monitor AP control */
   __IM uint32_t RESERVED12;
   __IM uint32_t TR_MON_AP_STATUS0;             /*!< 0x000002E8 True random monitor AP status 0 */
   __IM uint32_t TR_MON_AP_STATUS1;             /*!< 0x000002EC True random monitor AP status 1 */
   __IM uint32_t RESERVED13[837];
   __IM uint32_t STATUS;                        /*!< 0x00001004 Status */
   __IM uint32_t RESERVED14[14];
  __IOM uint32_t INSTR_FF_CTL;                  /*!< 0x00001040 Instruction FIFO control */
   __IM uint32_t INSTR_FF_STATUS;               /*!< 0x00001044 Instruction FIFO status */
   __OM uint32_t INSTR_FF_WR;                   /*!< 0x00001048 Instruction FIFO write */
   __IM uint32_t RESERVED15[29];
   __IM uint32_t LOAD0_FF_STATUS;               /*!< 0x000010C0 Load 0 FIFO status */
   __IM uint32_t RESERVED16[3];
   __IM uint32_t LOAD1_FF_STATUS;               /*!< 0x000010D0 Load 1 FIFO status */
   __IM uint32_t RESERVED17[7];
   __IM uint32_t STORE_FF_STATUS;               /*!< 0x000010F0 Store FIFO status */
   __IM uint32_t RESERVED18[3];
  __IOM uint32_t AES_CTL;                       /*!< 0x00001100 AES control */
   __IM uint32_t RESERVED19[31];
  __IOM uint32_t RESULT;                        /*!< 0x00001180 Result */
   __IM uint32_t RESERVED20[159];
  __IOM uint32_t CRC_CTL;                       /*!< 0x00001400 CRC control */
   __IM uint32_t RESERVED21[3];
  __IOM uint32_t CRC_DATA_CTL;                  /*!< 0x00001410 CRC data control */
   __IM uint32_t RESERVED22[3];
  __IOM uint32_t CRC_POL_CTL;                   /*!< 0x00001420 CRC polynomial control */
   __IM uint32_t RESERVED23[7];
  __IOM uint32_t CRC_REM_CTL;                   /*!< 0x00001440 CRC remainder control */
   __IM uint32_t RESERVED24;
   __IM uint32_t CRC_REM_RESULT;                /*!< 0x00001448 CRC remainder result */
   __IM uint32_t RESERVED25[13];
  __IOM uint32_t VU_CTL0;                       /*!< 0x00001480 Vector unit control 0 */
  __IOM uint32_t VU_CTL1;                       /*!< 0x00001484 Vector unit control 1 */
  __IOM uint32_t VU_CTL2;                       /*!< 0x00001488 Vector unit control 2 */
   __IM uint32_t RESERVED26;
   __IM uint32_t VU_STATUS;                     /*!< 0x00001490 Vector unit status */
   __IM uint32_t RESERVED27[11];
   __IM uint32_t VU_RF_DATA[16];                /*!< 0x000014C0 Vector unit register-file */
   __IM uint32_t RESERVED28[704];
  __IOM uint32_t DEV_KEY_ADDR0_CTL;             /*!< 0x00002000 Device key address 0 control */
  __IOM uint32_t DEV_KEY_ADDR0;                 /*!< 0x00002004 Device key address 0 */
   __IM uint32_t RESERVED29[6];
  __IOM uint32_t DEV_KEY_ADDR1_CTL;             /*!< 0x00002020 Device key address 1 control */
  __IOM uint32_t DEV_KEY_ADDR1;                 /*!< 0x00002024 Device key address 1 control */
   __IM uint32_t RESERVED30[22];
   __IM uint32_t DEV_KEY_STATUS;                /*!< 0x00002080 Device key status */
   __IM uint32_t RESERVED31[31];
  __IOM uint32_t DEV_KEY_CTL0;                  /*!< 0x00002100 Device key control 0 */
   __IM uint32_t RESERVED32[7];
  __IOM uint32_t DEV_KEY_CTL1;                  /*!< 0x00002120 Device key control 1 */
   __IM uint32_t RESERVED33[6071];
  __IOM uint32_t MEM_BUFF[8192];                /*!< 0x00008000 Memory buffer */
} CRYPTO_V2_Type;                               /*!< Size = 65536 (0x10000) */


/* CRYPTO.CTL */
#define CRYPTO_V2_CTL_P_Pos                     0UL
#define CRYPTO_V2_CTL_P_Msk                     0x1UL
#define CRYPTO_V2_CTL_NS_Pos                    1UL
#define CRYPTO_V2_CTL_NS_Msk                    0x2UL
#define CRYPTO_V2_CTL_PC_Pos                    4UL
#define CRYPTO_V2_CTL_PC_Msk                    0xF0UL
#define CRYPTO_V2_CTL_ECC_EN_Pos                16UL
#define CRYPTO_V2_CTL_ECC_EN_Msk                0x10000UL
#define CRYPTO_V2_CTL_ECC_INJ_EN_Pos            17UL
#define CRYPTO_V2_CTL_ECC_INJ_EN_Msk            0x20000UL
#define CRYPTO_V2_CTL_ENABLED_Pos               31UL
#define CRYPTO_V2_CTL_ENABLED_Msk               0x80000000UL
/* CRYPTO.RAM_PWR_CTL */
#define CRYPTO_V2_RAM_PWR_CTL_PWR_MODE_Pos      0UL
#define CRYPTO_V2_RAM_PWR_CTL_PWR_MODE_Msk      0x3UL
/* CRYPTO.RAM_PWR_DELAY_CTL */
#define CRYPTO_V2_RAM_PWR_DELAY_CTL_UP_Pos      0UL
#define CRYPTO_V2_RAM_PWR_DELAY_CTL_UP_Msk      0x3FFUL
/* CRYPTO.ECC_CTL */
#define CRYPTO_V2_ECC_CTL_WORD_ADDR_Pos         0UL
#define CRYPTO_V2_ECC_CTL_WORD_ADDR_Msk         0x1FFFUL
#define CRYPTO_V2_ECC_CTL_PARITY_Pos            25UL
#define CRYPTO_V2_ECC_CTL_PARITY_Msk            0xFE000000UL
/* CRYPTO.ERROR_STATUS0 */
#define CRYPTO_V2_ERROR_STATUS0_DATA32_Pos      0UL
#define CRYPTO_V2_ERROR_STATUS0_DATA32_Msk      0xFFFFFFFFUL
/* CRYPTO.ERROR_STATUS1 */
#define CRYPTO_V2_ERROR_STATUS1_DATA24_Pos      0UL
#define CRYPTO_V2_ERROR_STATUS1_DATA24_Msk      0xFFFFFFUL
#define CRYPTO_V2_ERROR_STATUS1_IDX_Pos         24UL
#define CRYPTO_V2_ERROR_STATUS1_IDX_Msk         0x7000000UL
#define CRYPTO_V2_ERROR_STATUS1_VALID_Pos       31UL
#define CRYPTO_V2_ERROR_STATUS1_VALID_Msk       0x80000000UL
/* CRYPTO.INTR */
#define CRYPTO_V2_INTR_INSTR_FF_LEVEL_Pos       0UL
#define CRYPTO_V2_INTR_INSTR_FF_LEVEL_Msk       0x1UL
#define CRYPTO_V2_INTR_INSTR_FF_OVERFLOW_Pos    1UL
#define CRYPTO_V2_INTR_INSTR_FF_OVERFLOW_Msk    0x2UL
#define CRYPTO_V2_INTR_TR_INITIALIZED_Pos       2UL
#define CRYPTO_V2_INTR_TR_INITIALIZED_Msk       0x4UL
#define CRYPTO_V2_INTR_TR_DATA_AVAILABLE_Pos    3UL
#define CRYPTO_V2_INTR_TR_DATA_AVAILABLE_Msk    0x8UL
#define CRYPTO_V2_INTR_PR_DATA_AVAILABLE_Pos    4UL
#define CRYPTO_V2_INTR_PR_DATA_AVAILABLE_Msk    0x10UL
#define CRYPTO_V2_INTR_INSTR_OPC_ERROR_Pos      16UL
#define CRYPTO_V2_INTR_INSTR_OPC_ERROR_Msk      0x10000UL
#define CRYPTO_V2_INTR_INSTR_CC_ERROR_Pos       17UL
#define CRYPTO_V2_INTR_INSTR_CC_ERROR_Msk       0x20000UL
#define CRYPTO_V2_INTR_BUS_ERROR_Pos            18UL
#define CRYPTO_V2_INTR_BUS_ERROR_Msk            0x40000UL
#define CRYPTO_V2_INTR_TR_AP_DETECT_ERROR_Pos   19UL
#define CRYPTO_V2_INTR_TR_AP_DETECT_ERROR_Msk   0x80000UL
#define CRYPTO_V2_INTR_TR_RC_DETECT_ERROR_Pos   20UL
#define CRYPTO_V2_INTR_TR_RC_DETECT_ERROR_Msk   0x100000UL
#define CRYPTO_V2_INTR_INSTR_DEV_KEY_ERROR_Pos  21UL
#define CRYPTO_V2_INTR_INSTR_DEV_KEY_ERROR_Msk  0x200000UL
/* CRYPTO.INTR_SET */
#define CRYPTO_V2_INTR_SET_INSTR_FF_LEVEL_Pos   0UL
#define CRYPTO_V2_INTR_SET_INSTR_FF_LEVEL_Msk   0x1UL
#define CRYPTO_V2_INTR_SET_INSTR_FF_OVERFLOW_Pos 1UL
#define CRYPTO_V2_INTR_SET_INSTR_FF_OVERFLOW_Msk 0x2UL
#define CRYPTO_V2_INTR_SET_TR_INITIALIZED_Pos   2UL
#define CRYPTO_V2_INTR_SET_TR_INITIALIZED_Msk   0x4UL
#define CRYPTO_V2_INTR_SET_TR_DATA_AVAILABLE_Pos 3UL
#define CRYPTO_V2_INTR_SET_TR_DATA_AVAILABLE_Msk 0x8UL
#define CRYPTO_V2_INTR_SET_PR_DATA_AVAILABLE_Pos 4UL
#define CRYPTO_V2_INTR_SET_PR_DATA_AVAILABLE_Msk 0x10UL
#define CRYPTO_V2_INTR_SET_INSTR_OPC_ERROR_Pos  16UL
#define CRYPTO_V2_INTR_SET_INSTR_OPC_ERROR_Msk  0x10000UL
#define CRYPTO_V2_INTR_SET_INSTR_CC_ERROR_Pos   17UL
#define CRYPTO_V2_INTR_SET_INSTR_CC_ERROR_Msk   0x20000UL
#define CRYPTO_V2_INTR_SET_BUS_ERROR_Pos        18UL
#define CRYPTO_V2_INTR_SET_BUS_ERROR_Msk        0x40000UL
#define CRYPTO_V2_INTR_SET_TR_AP_DETECT_ERROR_Pos 19UL
#define CRYPTO_V2_INTR_SET_TR_AP_DETECT_ERROR_Msk 0x80000UL
#define CRYPTO_V2_INTR_SET_TR_RC_DETECT_ERROR_Pos 20UL
#define CRYPTO_V2_INTR_SET_TR_RC_DETECT_ERROR_Msk 0x100000UL
#define CRYPTO_V2_INTR_SET_INSTR_DEV_KEY_ERROR_Pos 21UL
#define CRYPTO_V2_INTR_SET_INSTR_DEV_KEY_ERROR_Msk 0x200000UL
/* CRYPTO.INTR_MASK */
#define CRYPTO_V2_INTR_MASK_INSTR_FF_LEVEL_Pos  0UL
#define CRYPTO_V2_INTR_MASK_INSTR_FF_LEVEL_Msk  0x1UL
#define CRYPTO_V2_INTR_MASK_INSTR_FF_OVERFLOW_Pos 1UL
#define CRYPTO_V2_INTR_MASK_INSTR_FF_OVERFLOW_Msk 0x2UL
#define CRYPTO_V2_INTR_MASK_TR_INITIALIZED_Pos  2UL
#define CRYPTO_V2_INTR_MASK_TR_INITIALIZED_Msk  0x4UL
#define CRYPTO_V2_INTR_MASK_TR_DATA_AVAILABLE_Pos 3UL
#define CRYPTO_V2_INTR_MASK_TR_DATA_AVAILABLE_Msk 0x8UL
#define CRYPTO_V2_INTR_MASK_PR_DATA_AVAILABLE_Pos 4UL
#define CRYPTO_V2_INTR_MASK_PR_DATA_AVAILABLE_Msk 0x10UL
#define CRYPTO_V2_INTR_MASK_INSTR_OPC_ERROR_Pos 16UL
#define CRYPTO_V2_INTR_MASK_INSTR_OPC_ERROR_Msk 0x10000UL
#define CRYPTO_V2_INTR_MASK_INSTR_CC_ERROR_Pos  17UL
#define CRYPTO_V2_INTR_MASK_INSTR_CC_ERROR_Msk  0x20000UL
#define CRYPTO_V2_INTR_MASK_BUS_ERROR_Pos       18UL
#define CRYPTO_V2_INTR_MASK_BUS_ERROR_Msk       0x40000UL
#define CRYPTO_V2_INTR_MASK_TR_AP_DETECT_ERROR_Pos 19UL
#define CRYPTO_V2_INTR_MASK_TR_AP_DETECT_ERROR_Msk 0x80000UL
#define CRYPTO_V2_INTR_MASK_TR_RC_DETECT_ERROR_Pos 20UL
#define CRYPTO_V2_INTR_MASK_TR_RC_DETECT_ERROR_Msk 0x100000UL
#define CRYPTO_V2_INTR_MASK_INSTR_DEV_KEY_ERROR_Pos 21UL
#define CRYPTO_V2_INTR_MASK_INSTR_DEV_KEY_ERROR_Msk 0x200000UL
/* CRYPTO.INTR_MASKED */
#define CRYPTO_V2_INTR_MASKED_INSTR_FF_LEVEL_Pos 0UL
#define CRYPTO_V2_INTR_MASKED_INSTR_FF_LEVEL_Msk 0x1UL
#define CRYPTO_V2_INTR_MASKED_INSTR_FF_OVERFLOW_Pos 1UL
#define CRYPTO_V2_INTR_MASKED_INSTR_FF_OVERFLOW_Msk 0x2UL
#define CRYPTO_V2_INTR_MASKED_TR_INITIALIZED_Pos 2UL
#define CRYPTO_V2_INTR_MASKED_TR_INITIALIZED_Msk 0x4UL
#define CRYPTO_V2_INTR_MASKED_TR_DATA_AVAILABLE_Pos 3UL
#define CRYPTO_V2_INTR_MASKED_TR_DATA_AVAILABLE_Msk 0x8UL
#define CRYPTO_V2_INTR_MASKED_PR_DATA_AVAILABLE_Pos 4UL
#define CRYPTO_V2_INTR_MASKED_PR_DATA_AVAILABLE_Msk 0x10UL
#define CRYPTO_V2_INTR_MASKED_INSTR_OPC_ERROR_Pos 16UL
#define CRYPTO_V2_INTR_MASKED_INSTR_OPC_ERROR_Msk 0x10000UL
#define CRYPTO_V2_INTR_MASKED_INSTR_CC_ERROR_Pos 17UL
#define CRYPTO_V2_INTR_MASKED_INSTR_CC_ERROR_Msk 0x20000UL
#define CRYPTO_V2_INTR_MASKED_BUS_ERROR_Pos     18UL
#define CRYPTO_V2_INTR_MASKED_BUS_ERROR_Msk     0x40000UL
#define CRYPTO_V2_INTR_MASKED_TR_AP_DETECT_ERROR_Pos 19UL
#define CRYPTO_V2_INTR_MASKED_TR_AP_DETECT_ERROR_Msk 0x80000UL
#define CRYPTO_V2_INTR_MASKED_TR_RC_DETECT_ERROR_Pos 20UL
#define CRYPTO_V2_INTR_MASKED_TR_RC_DETECT_ERROR_Msk 0x100000UL
#define CRYPTO_V2_INTR_MASKED_INSTR_DEV_KEY_ERROR_Pos 21UL
#define CRYPTO_V2_INTR_MASKED_INSTR_DEV_KEY_ERROR_Msk 0x200000UL
/* CRYPTO.PR_LFSR_CTL0 */
#define CRYPTO_V2_PR_LFSR_CTL0_LFSR32_Pos       0UL
#define CRYPTO_V2_PR_LFSR_CTL0_LFSR32_Msk       0xFFFFFFFFUL
/* CRYPTO.PR_LFSR_CTL1 */
#define CRYPTO_V2_PR_LFSR_CTL1_LFSR31_Pos       0UL
#define CRYPTO_V2_PR_LFSR_CTL1_LFSR31_Msk       0x7FFFFFFFUL
/* CRYPTO.PR_LFSR_CTL2 */
#define CRYPTO_V2_PR_LFSR_CTL2_LFSR29_Pos       0UL
#define CRYPTO_V2_PR_LFSR_CTL2_LFSR29_Msk       0x1FFFFFFFUL
/* CRYPTO.PR_MAX_CTL */
#define CRYPTO_V2_PR_MAX_CTL_DATA32_Pos         0UL
#define CRYPTO_V2_PR_MAX_CTL_DATA32_Msk         0xFFFFFFFFUL
/* CRYPTO.PR_CMD */
#define CRYPTO_V2_PR_CMD_START_Pos              0UL
#define CRYPTO_V2_PR_CMD_START_Msk              0x1UL
/* CRYPTO.PR_RESULT */
#define CRYPTO_V2_PR_RESULT_DATA32_Pos          0UL
#define CRYPTO_V2_PR_RESULT_DATA32_Msk          0xFFFFFFFFUL
/* CRYPTO.TR_CTL0 */
#define CRYPTO_V2_TR_CTL0_SAMPLE_CLOCK_DIV_Pos  0UL
#define CRYPTO_V2_TR_CTL0_SAMPLE_CLOCK_DIV_Msk  0xFFUL
#define CRYPTO_V2_TR_CTL0_RED_CLOCK_DIV_Pos     8UL
#define CRYPTO_V2_TR_CTL0_RED_CLOCK_DIV_Msk     0xFF00UL
#define CRYPTO_V2_TR_CTL0_INIT_DELAY_Pos        16UL
#define CRYPTO_V2_TR_CTL0_INIT_DELAY_Msk        0xFF0000UL
#define CRYPTO_V2_TR_CTL0_VON_NEUMANN_CORR_Pos  24UL
#define CRYPTO_V2_TR_CTL0_VON_NEUMANN_CORR_Msk  0x1000000UL
#define CRYPTO_V2_TR_CTL0_STOP_ON_AP_DETECT_Pos 28UL
#define CRYPTO_V2_TR_CTL0_STOP_ON_AP_DETECT_Msk 0x10000000UL
#define CRYPTO_V2_TR_CTL0_STOP_ON_RC_DETECT_Pos 29UL
#define CRYPTO_V2_TR_CTL0_STOP_ON_RC_DETECT_Msk 0x20000000UL
/* CRYPTO.TR_CTL1 */
#define CRYPTO_V2_TR_CTL1_RO11_EN_Pos           0UL
#define CRYPTO_V2_TR_CTL1_RO11_EN_Msk           0x1UL
#define CRYPTO_V2_TR_CTL1_RO15_EN_Pos           1UL
#define CRYPTO_V2_TR_CTL1_RO15_EN_Msk           0x2UL
#define CRYPTO_V2_TR_CTL1_GARO15_EN_Pos         2UL
#define CRYPTO_V2_TR_CTL1_GARO15_EN_Msk         0x4UL
#define CRYPTO_V2_TR_CTL1_GARO31_EN_Pos         3UL
#define CRYPTO_V2_TR_CTL1_GARO31_EN_Msk         0x8UL
#define CRYPTO_V2_TR_CTL1_FIRO15_EN_Pos         4UL
#define CRYPTO_V2_TR_CTL1_FIRO15_EN_Msk         0x10UL
#define CRYPTO_V2_TR_CTL1_FIRO31_EN_Pos         5UL
#define CRYPTO_V2_TR_CTL1_FIRO31_EN_Msk         0x20UL
/* CRYPTO.TR_CTL2 */
#define CRYPTO_V2_TR_CTL2_SIZE_Pos              0UL
#define CRYPTO_V2_TR_CTL2_SIZE_Msk              0x3FUL
/* CRYPTO.TR_STATUS */
#define CRYPTO_V2_TR_STATUS_INITIALIZED_Pos     0UL
#define CRYPTO_V2_TR_STATUS_INITIALIZED_Msk     0x1UL
/* CRYPTO.TR_CMD */
#define CRYPTO_V2_TR_CMD_START_Pos              0UL
#define CRYPTO_V2_TR_CMD_START_Msk              0x1UL
/* CRYPTO.TR_RESULT */
#define CRYPTO_V2_TR_RESULT_DATA32_Pos          0UL
#define CRYPTO_V2_TR_RESULT_DATA32_Msk          0xFFFFFFFFUL
/* CRYPTO.TR_GARO_CTL */
#define CRYPTO_V2_TR_GARO_CTL_POLYNOMIAL31_Pos  0UL
#define CRYPTO_V2_TR_GARO_CTL_POLYNOMIAL31_Msk  0x7FFFFFFFUL
/* CRYPTO.TR_FIRO_CTL */
#define CRYPTO_V2_TR_FIRO_CTL_POLYNOMIAL31_Pos  0UL
#define CRYPTO_V2_TR_FIRO_CTL_POLYNOMIAL31_Msk  0x7FFFFFFFUL
/* CRYPTO.TR_MON_CTL */
#define CRYPTO_V2_TR_MON_CTL_BITSTREAM_SEL_Pos  0UL
#define CRYPTO_V2_TR_MON_CTL_BITSTREAM_SEL_Msk  0x3UL
/* CRYPTO.TR_MON_CMD */
#define CRYPTO_V2_TR_MON_CMD_START_AP_Pos       0UL
#define CRYPTO_V2_TR_MON_CMD_START_AP_Msk       0x1UL
#define CRYPTO_V2_TR_MON_CMD_START_RC_Pos       1UL
#define CRYPTO_V2_TR_MON_CMD_START_RC_Msk       0x2UL
/* CRYPTO.TR_MON_RC_CTL */
#define CRYPTO_V2_TR_MON_RC_CTL_CUTOFF_COUNT8_Pos 0UL
#define CRYPTO_V2_TR_MON_RC_CTL_CUTOFF_COUNT8_Msk 0xFFUL
/* CRYPTO.TR_MON_RC_STATUS0 */
#define CRYPTO_V2_TR_MON_RC_STATUS0_BIT_Pos     0UL
#define CRYPTO_V2_TR_MON_RC_STATUS0_BIT_Msk     0x1UL
/* CRYPTO.TR_MON_RC_STATUS1 */
#define CRYPTO_V2_TR_MON_RC_STATUS1_REP_COUNT_Pos 0UL
#define CRYPTO_V2_TR_MON_RC_STATUS1_REP_COUNT_Msk 0xFFUL
/* CRYPTO.TR_MON_AP_CTL */
#define CRYPTO_V2_TR_MON_AP_CTL_CUTOFF_COUNT16_Pos 0UL
#define CRYPTO_V2_TR_MON_AP_CTL_CUTOFF_COUNT16_Msk 0xFFFFUL
#define CRYPTO_V2_TR_MON_AP_CTL_WINDOW_SIZE_Pos 16UL
#define CRYPTO_V2_TR_MON_AP_CTL_WINDOW_SIZE_Msk 0xFFFF0000UL
/* CRYPTO.TR_MON_AP_STATUS0 */
#define CRYPTO_V2_TR_MON_AP_STATUS0_BIT_Pos     0UL
#define CRYPTO_V2_TR_MON_AP_STATUS0_BIT_Msk     0x1UL
/* CRYPTO.TR_MON_AP_STATUS1 */
#define CRYPTO_V2_TR_MON_AP_STATUS1_OCC_COUNT_Pos 0UL
#define CRYPTO_V2_TR_MON_AP_STATUS1_OCC_COUNT_Msk 0xFFFFUL
#define CRYPTO_V2_TR_MON_AP_STATUS1_WINDOW_INDEX_Pos 16UL
#define CRYPTO_V2_TR_MON_AP_STATUS1_WINDOW_INDEX_Msk 0xFFFF0000UL
/* CRYPTO.STATUS */
#define CRYPTO_V2_STATUS_BUSY_Pos               31UL
#define CRYPTO_V2_STATUS_BUSY_Msk               0x80000000UL
/* CRYPTO.INSTR_FF_CTL */
#define CRYPTO_V2_INSTR_FF_CTL_EVENT_LEVEL_Pos  0UL
#define CRYPTO_V2_INSTR_FF_CTL_EVENT_LEVEL_Msk  0x7UL
#define CRYPTO_V2_INSTR_FF_CTL_CLEAR_Pos        16UL
#define CRYPTO_V2_INSTR_FF_CTL_CLEAR_Msk        0x10000UL
#define CRYPTO_V2_INSTR_FF_CTL_BLOCK_Pos        17UL
#define CRYPTO_V2_INSTR_FF_CTL_BLOCK_Msk        0x20000UL
/* CRYPTO.INSTR_FF_STATUS */
#define CRYPTO_V2_INSTR_FF_STATUS_USED_Pos      0UL
#define CRYPTO_V2_INSTR_FF_STATUS_USED_Msk      0xFUL
#define CRYPTO_V2_INSTR_FF_STATUS_EVENT_Pos     16UL
#define CRYPTO_V2_INSTR_FF_STATUS_EVENT_Msk     0x10000UL
/* CRYPTO.INSTR_FF_WR */
#define CRYPTO_V2_INSTR_FF_WR_DATA32_Pos        0UL
#define CRYPTO_V2_INSTR_FF_WR_DATA32_Msk        0xFFFFFFFFUL
/* CRYPTO.LOAD0_FF_STATUS */
#define CRYPTO_V2_LOAD0_FF_STATUS_USED5_Pos     0UL
#define CRYPTO_V2_LOAD0_FF_STATUS_USED5_Msk     0x1FUL
#define CRYPTO_V2_LOAD0_FF_STATUS_BUSY_Pos      31UL
#define CRYPTO_V2_LOAD0_FF_STATUS_BUSY_Msk      0x80000000UL
/* CRYPTO.LOAD1_FF_STATUS */
#define CRYPTO_V2_LOAD1_FF_STATUS_USED5_Pos     0UL
#define CRYPTO_V2_LOAD1_FF_STATUS_USED5_Msk     0x1FUL
#define CRYPTO_V2_LOAD1_FF_STATUS_BUSY_Pos      31UL
#define CRYPTO_V2_LOAD1_FF_STATUS_BUSY_Msk      0x80000000UL
/* CRYPTO.STORE_FF_STATUS */
#define CRYPTO_V2_STORE_FF_STATUS_USED5_Pos     0UL
#define CRYPTO_V2_STORE_FF_STATUS_USED5_Msk     0x1FUL
#define CRYPTO_V2_STORE_FF_STATUS_BUSY_Pos      31UL
#define CRYPTO_V2_STORE_FF_STATUS_BUSY_Msk      0x80000000UL
/* CRYPTO.AES_CTL */
#define CRYPTO_V2_AES_CTL_KEY_SIZE_Pos          0UL
#define CRYPTO_V2_AES_CTL_KEY_SIZE_Msk          0x3UL
/* CRYPTO.RESULT */
#define CRYPTO_V2_RESULT_DATA_Pos               0UL
#define CRYPTO_V2_RESULT_DATA_Msk               0xFFFFFFFFUL
/* CRYPTO.CRC_CTL */
#define CRYPTO_V2_CRC_CTL_DATA_REVERSE_Pos      0UL
#define CRYPTO_V2_CRC_CTL_DATA_REVERSE_Msk      0x1UL
#define CRYPTO_V2_CRC_CTL_REM_REVERSE_Pos       8UL
#define CRYPTO_V2_CRC_CTL_REM_REVERSE_Msk       0x100UL
/* CRYPTO.CRC_DATA_CTL */
#define CRYPTO_V2_CRC_DATA_CTL_DATA_XOR_Pos     0UL
#define CRYPTO_V2_CRC_DATA_CTL_DATA_XOR_Msk     0xFFUL
/* CRYPTO.CRC_POL_CTL */
#define CRYPTO_V2_CRC_POL_CTL_POLYNOMIAL_Pos    0UL
#define CRYPTO_V2_CRC_POL_CTL_POLYNOMIAL_Msk    0xFFFFFFFFUL
/* CRYPTO.CRC_REM_CTL */
#define CRYPTO_V2_CRC_REM_CTL_REM_XOR_Pos       0UL
#define CRYPTO_V2_CRC_REM_CTL_REM_XOR_Msk       0xFFFFFFFFUL
/* CRYPTO.CRC_REM_RESULT */
#define CRYPTO_V2_CRC_REM_RESULT_REM_Pos        0UL
#define CRYPTO_V2_CRC_REM_RESULT_REM_Msk        0xFFFFFFFFUL
/* CRYPTO.VU_CTL0 */
#define CRYPTO_V2_VU_CTL0_ALWAYS_EXECUTE_Pos    0UL
#define CRYPTO_V2_VU_CTL0_ALWAYS_EXECUTE_Msk    0x1UL
/* CRYPTO.VU_CTL1 */
#define CRYPTO_V2_VU_CTL1_ADDR24_Pos            8UL
#define CRYPTO_V2_VU_CTL1_ADDR24_Msk            0xFFFFFF00UL
/* CRYPTO.VU_CTL2 */
#define CRYPTO_V2_VU_CTL2_MASK_Pos              8UL
#define CRYPTO_V2_VU_CTL2_MASK_Msk              0x7F00UL
/* CRYPTO.VU_STATUS */
#define CRYPTO_V2_VU_STATUS_CARRY_Pos           0UL
#define CRYPTO_V2_VU_STATUS_CARRY_Msk           0x1UL
#define CRYPTO_V2_VU_STATUS_EVEN_Pos            1UL
#define CRYPTO_V2_VU_STATUS_EVEN_Msk            0x2UL
#define CRYPTO_V2_VU_STATUS_ZERO_Pos            2UL
#define CRYPTO_V2_VU_STATUS_ZERO_Msk            0x4UL
#define CRYPTO_V2_VU_STATUS_ONE_Pos             3UL
#define CRYPTO_V2_VU_STATUS_ONE_Msk             0x8UL
/* CRYPTO.VU_RF_DATA */
#define CRYPTO_V2_VU_RF_DATA_DATA32_Pos         0UL
#define CRYPTO_V2_VU_RF_DATA_DATA32_Msk         0xFFFFFFFFUL
/* CRYPTO.DEV_KEY_ADDR0_CTL */
#define CRYPTO_V2_DEV_KEY_ADDR0_CTL_VALID_Pos   31UL
#define CRYPTO_V2_DEV_KEY_ADDR0_CTL_VALID_Msk   0x80000000UL
/* CRYPTO.DEV_KEY_ADDR0 */
#define CRYPTO_V2_DEV_KEY_ADDR0_ADDR32_Pos      0UL
#define CRYPTO_V2_DEV_KEY_ADDR0_ADDR32_Msk      0xFFFFFFFFUL
/* CRYPTO.DEV_KEY_ADDR1_CTL */
#define CRYPTO_V2_DEV_KEY_ADDR1_CTL_VALID_Pos   31UL
#define CRYPTO_V2_DEV_KEY_ADDR1_CTL_VALID_Msk   0x80000000UL
/* CRYPTO.DEV_KEY_ADDR1 */
#define CRYPTO_V2_DEV_KEY_ADDR1_ADDR32_Pos      0UL
#define CRYPTO_V2_DEV_KEY_ADDR1_ADDR32_Msk      0xFFFFFFFFUL
/* CRYPTO.DEV_KEY_STATUS */
#define CRYPTO_V2_DEV_KEY_STATUS_LOADED_Pos     0UL
#define CRYPTO_V2_DEV_KEY_STATUS_LOADED_Msk     0x1UL
/* CRYPTO.DEV_KEY_CTL0 */
#define CRYPTO_V2_DEV_KEY_CTL0_ALLOWED_Pos      0UL
#define CRYPTO_V2_DEV_KEY_CTL0_ALLOWED_Msk      0x1UL
/* CRYPTO.DEV_KEY_CTL1 */
#define CRYPTO_V2_DEV_KEY_CTL1_ALLOWED_Pos      0UL
#define CRYPTO_V2_DEV_KEY_CTL1_ALLOWED_Msk      0x1UL
/* CRYPTO.MEM_BUFF */
#define CRYPTO_V2_MEM_BUFF_DATA32_Pos           0UL
#define CRYPTO_V2_MEM_BUFF_DATA32_Msk           0xFFFFFFFFUL


#endif /* _CYIP_CRYPTO_V2_H_ */


/* [] END OF FILE */
