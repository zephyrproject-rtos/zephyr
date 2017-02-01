/**
 * \file
 *
 * \brief Instance description for ICM
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

#ifndef _SAME70_ICM_INSTANCE_H_
#define _SAME70_ICM_INSTANCE_H_

/* ========== Register definition for ICM peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_ICM_CFG             (0x40048000) /**< (ICM) Configuration Register */
#define REG_ICM_CTRL            (0x40048004) /**< (ICM) Control Register */
#define REG_ICM_SR              (0x40048008) /**< (ICM) Status Register */
#define REG_ICM_IER             (0x40048010) /**< (ICM) Interrupt Enable Register */
#define REG_ICM_IDR             (0x40048014) /**< (ICM) Interrupt Disable Register */
#define REG_ICM_IMR             (0x40048018) /**< (ICM) Interrupt Mask Register */
#define REG_ICM_ISR             (0x4004801C) /**< (ICM) Interrupt Status Register */
#define REG_ICM_UASR            (0x40048020) /**< (ICM) Undefined Access Status Register */
#define REG_ICM_DSCR            (0x40048030) /**< (ICM) Region Descriptor Area Start Address Register */
#define REG_ICM_HASH            (0x40048034) /**< (ICM) Region Hash Area Start Address Register */
#define REG_ICM_UIHVAL          (0x40048038) /**< (ICM) User Initial Hash Value 0 Register 0 */
#define REG_ICM_UIHVAL0         (0x40048038) /**< (ICM) User Initial Hash Value 0 Register 0 */
#define REG_ICM_UIHVAL1         (0x4004803C) /**< (ICM) User Initial Hash Value 0 Register 1 */
#define REG_ICM_UIHVAL2         (0x40048040) /**< (ICM) User Initial Hash Value 0 Register 2 */
#define REG_ICM_UIHVAL3         (0x40048044) /**< (ICM) User Initial Hash Value 0 Register 3 */
#define REG_ICM_UIHVAL4         (0x40048048) /**< (ICM) User Initial Hash Value 0 Register 4 */
#define REG_ICM_UIHVAL5         (0x4004804C) /**< (ICM) User Initial Hash Value 0 Register 5 */
#define REG_ICM_UIHVAL6         (0x40048050) /**< (ICM) User Initial Hash Value 0 Register 6 */
#define REG_ICM_UIHVAL7         (0x40048054) /**< (ICM) User Initial Hash Value 0 Register 7 */

#else

#define REG_ICM_CFG             (*(__IO uint32_t*)0x40048000U) /**< (ICM) Configuration Register */
#define REG_ICM_CTRL            (*(__O  uint32_t*)0x40048004U) /**< (ICM) Control Register */
#define REG_ICM_SR              (*(__O  uint32_t*)0x40048008U) /**< (ICM) Status Register */
#define REG_ICM_IER             (*(__O  uint32_t*)0x40048010U) /**< (ICM) Interrupt Enable Register */
#define REG_ICM_IDR             (*(__O  uint32_t*)0x40048014U) /**< (ICM) Interrupt Disable Register */
#define REG_ICM_IMR             (*(__I  uint32_t*)0x40048018U) /**< (ICM) Interrupt Mask Register */
#define REG_ICM_ISR             (*(__I  uint32_t*)0x4004801CU) /**< (ICM) Interrupt Status Register */
#define REG_ICM_UASR            (*(__I  uint32_t*)0x40048020U) /**< (ICM) Undefined Access Status Register */
#define REG_ICM_DSCR            (*(__IO uint32_t*)0x40048030U) /**< (ICM) Region Descriptor Area Start Address Register */
#define REG_ICM_HASH            (*(__IO uint32_t*)0x40048034U) /**< (ICM) Region Hash Area Start Address Register */
#define REG_ICM_UIHVAL          (*(__O  uint32_t*)0x40048038U) /**< (ICM) User Initial Hash Value 0 Register 0 */
#define REG_ICM_UIHVAL0         (*(__O  uint32_t*)0x40048038U) /**< (ICM) User Initial Hash Value 0 Register 0 */
#define REG_ICM_UIHVAL1         (*(__O  uint32_t*)0x4004803CU) /**< (ICM) User Initial Hash Value 0 Register 1 */
#define REG_ICM_UIHVAL2         (*(__O  uint32_t*)0x40048040U) /**< (ICM) User Initial Hash Value 0 Register 2 */
#define REG_ICM_UIHVAL3         (*(__O  uint32_t*)0x40048044U) /**< (ICM) User Initial Hash Value 0 Register 3 */
#define REG_ICM_UIHVAL4         (*(__O  uint32_t*)0x40048048U) /**< (ICM) User Initial Hash Value 0 Register 4 */
#define REG_ICM_UIHVAL5         (*(__O  uint32_t*)0x4004804CU) /**< (ICM) User Initial Hash Value 0 Register 5 */
#define REG_ICM_UIHVAL6         (*(__O  uint32_t*)0x40048050U) /**< (ICM) User Initial Hash Value 0 Register 6 */
#define REG_ICM_UIHVAL7         (*(__O  uint32_t*)0x40048054U) /**< (ICM) User Initial Hash Value 0 Register 7 */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for ICM peripheral ========== */
#define ICM_INSTANCE_ID                          32        

#endif /* _SAME70_ICM_INSTANCE_ */
