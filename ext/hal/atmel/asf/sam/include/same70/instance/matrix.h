/**
 * \file
 *
 * \brief Instance description for MATRIX
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

#ifndef _SAME70_MATRIX_INSTANCE_H_
#define _SAME70_MATRIX_INSTANCE_H_

/* ========== Register definition for MATRIX peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_MATRIX_PRAS0        (0x40088080) /**< (MATRIX) Priority Register A for Slave 0 */
#define REG_MATRIX_PRBS0        (0x40088084) /**< (MATRIX) Priority Register B for Slave 0 */
#define REG_MATRIX_PRAS1        (0x40088088) /**< (MATRIX) Priority Register A for Slave 1 */
#define REG_MATRIX_PRBS1        (0x4008808C) /**< (MATRIX) Priority Register B for Slave 1 */
#define REG_MATRIX_PRAS2        (0x40088090) /**< (MATRIX) Priority Register A for Slave 2 */
#define REG_MATRIX_PRBS2        (0x40088094) /**< (MATRIX) Priority Register B for Slave 2 */
#define REG_MATRIX_PRAS3        (0x40088098) /**< (MATRIX) Priority Register A for Slave 3 */
#define REG_MATRIX_PRBS3        (0x4008809C) /**< (MATRIX) Priority Register B for Slave 3 */
#define REG_MATRIX_PRAS4        (0x400880A0) /**< (MATRIX) Priority Register A for Slave 4 */
#define REG_MATRIX_PRBS4        (0x400880A4) /**< (MATRIX) Priority Register B for Slave 4 */
#define REG_MATRIX_PRAS5        (0x400880A8) /**< (MATRIX) Priority Register A for Slave 5 */
#define REG_MATRIX_PRBS5        (0x400880AC) /**< (MATRIX) Priority Register B for Slave 5 */
#define REG_MATRIX_PRAS6        (0x400880B0) /**< (MATRIX) Priority Register A for Slave 6 */
#define REG_MATRIX_PRBS6        (0x400880B4) /**< (MATRIX) Priority Register B for Slave 6 */
#define REG_MATRIX_PRAS7        (0x400880B8) /**< (MATRIX) Priority Register A for Slave 7 */
#define REG_MATRIX_PRBS7        (0x400880BC) /**< (MATRIX) Priority Register B for Slave 7 */
#define REG_MATRIX_PRAS8        (0x400880C0) /**< (MATRIX) Priority Register A for Slave 8 */
#define REG_MATRIX_PRBS8        (0x400880C4) /**< (MATRIX) Priority Register B for Slave 8 */
#define REG_MATRIX_MCFG         (0x40088000) /**< (MATRIX) Master Configuration Register 0 */
#define REG_MATRIX_MCFG0        (0x40088000) /**< (MATRIX) Master Configuration Register 0 */
#define REG_MATRIX_MCFG1        (0x40088004) /**< (MATRIX) Master Configuration Register 1 */
#define REG_MATRIX_MCFG2        (0x40088008) /**< (MATRIX) Master Configuration Register 2 */
#define REG_MATRIX_MCFG3        (0x4008800C) /**< (MATRIX) Master Configuration Register 3 */
#define REG_MATRIX_MCFG4        (0x40088010) /**< (MATRIX) Master Configuration Register 4 */
#define REG_MATRIX_MCFG5        (0x40088014) /**< (MATRIX) Master Configuration Register 5 */
#define REG_MATRIX_MCFG6        (0x40088018) /**< (MATRIX) Master Configuration Register 6 */
#define REG_MATRIX_MCFG7        (0x4008801C) /**< (MATRIX) Master Configuration Register 7 */
#define REG_MATRIX_MCFG8        (0x40088020) /**< (MATRIX) Master Configuration Register 8 */
#define REG_MATRIX_MCFG9        (0x40088024) /**< (MATRIX) Master Configuration Register 9 */
#define REG_MATRIX_MCFG10       (0x40088028) /**< (MATRIX) Master Configuration Register 10 */
#define REG_MATRIX_MCFG11       (0x4008802C) /**< (MATRIX) Master Configuration Register 11 */
#define REG_MATRIX_SCFG         (0x40088040) /**< (MATRIX) Slave Configuration Register 0 */
#define REG_MATRIX_SCFG0        (0x40088040) /**< (MATRIX) Slave Configuration Register 0 */
#define REG_MATRIX_SCFG1        (0x40088044) /**< (MATRIX) Slave Configuration Register 1 */
#define REG_MATRIX_SCFG2        (0x40088048) /**< (MATRIX) Slave Configuration Register 2 */
#define REG_MATRIX_SCFG3        (0x4008804C) /**< (MATRIX) Slave Configuration Register 3 */
#define REG_MATRIX_SCFG4        (0x40088050) /**< (MATRIX) Slave Configuration Register 4 */
#define REG_MATRIX_SCFG5        (0x40088054) /**< (MATRIX) Slave Configuration Register 5 */
#define REG_MATRIX_SCFG6        (0x40088058) /**< (MATRIX) Slave Configuration Register 6 */
#define REG_MATRIX_SCFG7        (0x4008805C) /**< (MATRIX) Slave Configuration Register 7 */
#define REG_MATRIX_SCFG8        (0x40088060) /**< (MATRIX) Slave Configuration Register 8 */
#define REG_MATRIX_MRCR         (0x40088100) /**< (MATRIX) Master Remap Control Register */
#define REG_CCFG_CAN0           (0x40088110) /**< (MATRIX) CAN0 Configuration Register */
#define REG_CCFG_SYSIO          (0x40088114) /**< (MATRIX) System I/O and CAN1 Configuration Register */
#define REG_CCFG_SMCNFCS        (0x40088124) /**< (MATRIX) SMC NAND Flash Chip Select Configuration Register */
#define REG_MATRIX_WPMR         (0x400881E4) /**< (MATRIX) Write Protection Mode Register */
#define REG_MATRIX_WPSR         (0x400881E8) /**< (MATRIX) Write Protection Status Register */

#else

#define REG_MATRIX_PRAS0        (*(__IO uint32_t*)0x40088080U) /**< (MATRIX) Priority Register A for Slave 0 */
#define REG_MATRIX_PRBS0        (*(__IO uint32_t*)0x40088084U) /**< (MATRIX) Priority Register B for Slave 0 */
#define REG_MATRIX_PRAS1        (*(__IO uint32_t*)0x40088088U) /**< (MATRIX) Priority Register A for Slave 1 */
#define REG_MATRIX_PRBS1        (*(__IO uint32_t*)0x4008808CU) /**< (MATRIX) Priority Register B for Slave 1 */
#define REG_MATRIX_PRAS2        (*(__IO uint32_t*)0x40088090U) /**< (MATRIX) Priority Register A for Slave 2 */
#define REG_MATRIX_PRBS2        (*(__IO uint32_t*)0x40088094U) /**< (MATRIX) Priority Register B for Slave 2 */
#define REG_MATRIX_PRAS3        (*(__IO uint32_t*)0x40088098U) /**< (MATRIX) Priority Register A for Slave 3 */
#define REG_MATRIX_PRBS3        (*(__IO uint32_t*)0x4008809CU) /**< (MATRIX) Priority Register B for Slave 3 */
#define REG_MATRIX_PRAS4        (*(__IO uint32_t*)0x400880A0U) /**< (MATRIX) Priority Register A for Slave 4 */
#define REG_MATRIX_PRBS4        (*(__IO uint32_t*)0x400880A4U) /**< (MATRIX) Priority Register B for Slave 4 */
#define REG_MATRIX_PRAS5        (*(__IO uint32_t*)0x400880A8U) /**< (MATRIX) Priority Register A for Slave 5 */
#define REG_MATRIX_PRBS5        (*(__IO uint32_t*)0x400880ACU) /**< (MATRIX) Priority Register B for Slave 5 */
#define REG_MATRIX_PRAS6        (*(__IO uint32_t*)0x400880B0U) /**< (MATRIX) Priority Register A for Slave 6 */
#define REG_MATRIX_PRBS6        (*(__IO uint32_t*)0x400880B4U) /**< (MATRIX) Priority Register B for Slave 6 */
#define REG_MATRIX_PRAS7        (*(__IO uint32_t*)0x400880B8U) /**< (MATRIX) Priority Register A for Slave 7 */
#define REG_MATRIX_PRBS7        (*(__IO uint32_t*)0x400880BCU) /**< (MATRIX) Priority Register B for Slave 7 */
#define REG_MATRIX_PRAS8        (*(__IO uint32_t*)0x400880C0U) /**< (MATRIX) Priority Register A for Slave 8 */
#define REG_MATRIX_PRBS8        (*(__IO uint32_t*)0x400880C4U) /**< (MATRIX) Priority Register B for Slave 8 */
#define REG_MATRIX_MCFG         (*(__IO uint32_t*)0x40088000U) /**< (MATRIX) Master Configuration Register 0 */
#define REG_MATRIX_MCFG0        (*(__IO uint32_t*)0x40088000U) /**< (MATRIX) Master Configuration Register 0 */
#define REG_MATRIX_MCFG1        (*(__IO uint32_t*)0x40088004U) /**< (MATRIX) Master Configuration Register 1 */
#define REG_MATRIX_MCFG2        (*(__IO uint32_t*)0x40088008U) /**< (MATRIX) Master Configuration Register 2 */
#define REG_MATRIX_MCFG3        (*(__IO uint32_t*)0x4008800CU) /**< (MATRIX) Master Configuration Register 3 */
#define REG_MATRIX_MCFG4        (*(__IO uint32_t*)0x40088010U) /**< (MATRIX) Master Configuration Register 4 */
#define REG_MATRIX_MCFG5        (*(__IO uint32_t*)0x40088014U) /**< (MATRIX) Master Configuration Register 5 */
#define REG_MATRIX_MCFG6        (*(__IO uint32_t*)0x40088018U) /**< (MATRIX) Master Configuration Register 6 */
#define REG_MATRIX_MCFG7        (*(__IO uint32_t*)0x4008801CU) /**< (MATRIX) Master Configuration Register 7 */
#define REG_MATRIX_MCFG8        (*(__IO uint32_t*)0x40088020U) /**< (MATRIX) Master Configuration Register 8 */
#define REG_MATRIX_MCFG9        (*(__IO uint32_t*)0x40088024U) /**< (MATRIX) Master Configuration Register 9 */
#define REG_MATRIX_MCFG10       (*(__IO uint32_t*)0x40088028U) /**< (MATRIX) Master Configuration Register 10 */
#define REG_MATRIX_MCFG11       (*(__IO uint32_t*)0x4008802CU) /**< (MATRIX) Master Configuration Register 11 */
#define REG_MATRIX_SCFG         (*(__IO uint32_t*)0x40088040U) /**< (MATRIX) Slave Configuration Register 0 */
#define REG_MATRIX_SCFG0        (*(__IO uint32_t*)0x40088040U) /**< (MATRIX) Slave Configuration Register 0 */
#define REG_MATRIX_SCFG1        (*(__IO uint32_t*)0x40088044U) /**< (MATRIX) Slave Configuration Register 1 */
#define REG_MATRIX_SCFG2        (*(__IO uint32_t*)0x40088048U) /**< (MATRIX) Slave Configuration Register 2 */
#define REG_MATRIX_SCFG3        (*(__IO uint32_t*)0x4008804CU) /**< (MATRIX) Slave Configuration Register 3 */
#define REG_MATRIX_SCFG4        (*(__IO uint32_t*)0x40088050U) /**< (MATRIX) Slave Configuration Register 4 */
#define REG_MATRIX_SCFG5        (*(__IO uint32_t*)0x40088054U) /**< (MATRIX) Slave Configuration Register 5 */
#define REG_MATRIX_SCFG6        (*(__IO uint32_t*)0x40088058U) /**< (MATRIX) Slave Configuration Register 6 */
#define REG_MATRIX_SCFG7        (*(__IO uint32_t*)0x4008805CU) /**< (MATRIX) Slave Configuration Register 7 */
#define REG_MATRIX_SCFG8        (*(__IO uint32_t*)0x40088060U) /**< (MATRIX) Slave Configuration Register 8 */
#define REG_MATRIX_MRCR         (*(__IO uint32_t*)0x40088100U) /**< (MATRIX) Master Remap Control Register */
#define REG_CCFG_CAN0           (*(__IO uint32_t*)0x40088110U) /**< (MATRIX) CAN0 Configuration Register */
#define REG_CCFG_SYSIO          (*(__IO uint32_t*)0x40088114U) /**< (MATRIX) System I/O and CAN1 Configuration Register */
#define REG_CCFG_SMCNFCS        (*(__IO uint32_t*)0x40088124U) /**< (MATRIX) SMC NAND Flash Chip Select Configuration Register */
#define REG_MATRIX_WPMR         (*(__IO uint32_t*)0x400881E4U) /**< (MATRIX) Write Protection Mode Register */
#define REG_MATRIX_WPSR         (*(__I  uint32_t*)0x400881E8U) /**< (MATRIX) Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAME70_MATRIX_INSTANCE_ */
