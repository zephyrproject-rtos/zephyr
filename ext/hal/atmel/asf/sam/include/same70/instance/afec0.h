/**
 * \file
 *
 * \brief Instance description for AFEC0
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

#ifndef _SAME70_AFEC0_INSTANCE_H_
#define _SAME70_AFEC0_INSTANCE_H_

/* ========== Register definition for AFEC0 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_AFEC0_CR            (0x4003C000) /**< (AFEC0) AFEC Control Register */
#define REG_AFEC0_MR            (0x4003C004) /**< (AFEC0) AFEC Mode Register */
#define REG_AFEC0_EMR           (0x4003C008) /**< (AFEC0) AFEC Extended Mode Register */
#define REG_AFEC0_SEQ1R         (0x4003C00C) /**< (AFEC0) AFEC Channel Sequence 1 Register */
#define REG_AFEC0_SEQ2R         (0x4003C010) /**< (AFEC0) AFEC Channel Sequence 2 Register */
#define REG_AFEC0_CHER          (0x4003C014) /**< (AFEC0) AFEC Channel Enable Register */
#define REG_AFEC0_CHDR          (0x4003C018) /**< (AFEC0) AFEC Channel Disable Register */
#define REG_AFEC0_CHSR          (0x4003C01C) /**< (AFEC0) AFEC Channel Status Register */
#define REG_AFEC0_LCDR          (0x4003C020) /**< (AFEC0) AFEC Last Converted Data Register */
#define REG_AFEC0_IER           (0x4003C024) /**< (AFEC0) AFEC Interrupt Enable Register */
#define REG_AFEC0_IDR           (0x4003C028) /**< (AFEC0) AFEC Interrupt Disable Register */
#define REG_AFEC0_IMR           (0x4003C02C) /**< (AFEC0) AFEC Interrupt Mask Register */
#define REG_AFEC0_ISR           (0x4003C030) /**< (AFEC0) AFEC Interrupt Status Register */
#define REG_AFEC0_OVER          (0x4003C04C) /**< (AFEC0) AFEC Overrun Status Register */
#define REG_AFEC0_CWR           (0x4003C050) /**< (AFEC0) AFEC Compare Window Register */
#define REG_AFEC0_CGR           (0x4003C054) /**< (AFEC0) AFEC Channel Gain Register */
#define REG_AFEC0_DIFFR         (0x4003C060) /**< (AFEC0) AFEC Channel Differential Register */
#define REG_AFEC0_CSELR         (0x4003C064) /**< (AFEC0) AFEC Channel Selection Register */
#define REG_AFEC0_CDR           (0x4003C068) /**< (AFEC0) AFEC Channel Data Register */
#define REG_AFEC0_COCR          (0x4003C06C) /**< (AFEC0) AFEC Channel Offset Compensation Register */
#define REG_AFEC0_TEMPMR        (0x4003C070) /**< (AFEC0) AFEC Temperature Sensor Mode Register */
#define REG_AFEC0_TEMPCWR       (0x4003C074) /**< (AFEC0) AFEC Temperature Compare Window Register */
#define REG_AFEC0_ACR           (0x4003C094) /**< (AFEC0) AFEC Analog Control Register */
#define REG_AFEC0_SHMR          (0x4003C0A0) /**< (AFEC0) AFEC Sample & Hold Mode Register */
#define REG_AFEC0_COSR          (0x4003C0D0) /**< (AFEC0) AFEC Correction Select Register */
#define REG_AFEC0_CVR           (0x4003C0D4) /**< (AFEC0) AFEC Correction Values Register */
#define REG_AFEC0_CECR          (0x4003C0D8) /**< (AFEC0) AFEC Channel Error Correction Register */
#define REG_AFEC0_WPMR          (0x4003C0E4) /**< (AFEC0) AFEC Write Protection Mode Register */
#define REG_AFEC0_WPSR          (0x4003C0E8) /**< (AFEC0) AFEC Write Protection Status Register */

#else

#define REG_AFEC0_CR            (*(__O  uint32_t*)0x4003C000U) /**< (AFEC0) AFEC Control Register */
#define REG_AFEC0_MR            (*(__IO uint32_t*)0x4003C004U) /**< (AFEC0) AFEC Mode Register */
#define REG_AFEC0_EMR           (*(__IO uint32_t*)0x4003C008U) /**< (AFEC0) AFEC Extended Mode Register */
#define REG_AFEC0_SEQ1R         (*(__IO uint32_t*)0x4003C00CU) /**< (AFEC0) AFEC Channel Sequence 1 Register */
#define REG_AFEC0_SEQ2R         (*(__IO uint32_t*)0x4003C010U) /**< (AFEC0) AFEC Channel Sequence 2 Register */
#define REG_AFEC0_CHER          (*(__O  uint32_t*)0x4003C014U) /**< (AFEC0) AFEC Channel Enable Register */
#define REG_AFEC0_CHDR          (*(__O  uint32_t*)0x4003C018U) /**< (AFEC0) AFEC Channel Disable Register */
#define REG_AFEC0_CHSR          (*(__I  uint32_t*)0x4003C01CU) /**< (AFEC0) AFEC Channel Status Register */
#define REG_AFEC0_LCDR          (*(__I  uint32_t*)0x4003C020U) /**< (AFEC0) AFEC Last Converted Data Register */
#define REG_AFEC0_IER           (*(__O  uint32_t*)0x4003C024U) /**< (AFEC0) AFEC Interrupt Enable Register */
#define REG_AFEC0_IDR           (*(__O  uint32_t*)0x4003C028U) /**< (AFEC0) AFEC Interrupt Disable Register */
#define REG_AFEC0_IMR           (*(__I  uint32_t*)0x4003C02CU) /**< (AFEC0) AFEC Interrupt Mask Register */
#define REG_AFEC0_ISR           (*(__I  uint32_t*)0x4003C030U) /**< (AFEC0) AFEC Interrupt Status Register */
#define REG_AFEC0_OVER          (*(__I  uint32_t*)0x4003C04CU) /**< (AFEC0) AFEC Overrun Status Register */
#define REG_AFEC0_CWR           (*(__IO uint32_t*)0x4003C050U) /**< (AFEC0) AFEC Compare Window Register */
#define REG_AFEC0_CGR           (*(__IO uint32_t*)0x4003C054U) /**< (AFEC0) AFEC Channel Gain Register */
#define REG_AFEC0_DIFFR         (*(__IO uint32_t*)0x4003C060U) /**< (AFEC0) AFEC Channel Differential Register */
#define REG_AFEC0_CSELR         (*(__IO uint32_t*)0x4003C064U) /**< (AFEC0) AFEC Channel Selection Register */
#define REG_AFEC0_CDR           (*(__I  uint32_t*)0x4003C068U) /**< (AFEC0) AFEC Channel Data Register */
#define REG_AFEC0_COCR          (*(__IO uint32_t*)0x4003C06CU) /**< (AFEC0) AFEC Channel Offset Compensation Register */
#define REG_AFEC0_TEMPMR        (*(__IO uint32_t*)0x4003C070U) /**< (AFEC0) AFEC Temperature Sensor Mode Register */
#define REG_AFEC0_TEMPCWR       (*(__IO uint32_t*)0x4003C074U) /**< (AFEC0) AFEC Temperature Compare Window Register */
#define REG_AFEC0_ACR           (*(__IO uint32_t*)0x4003C094U) /**< (AFEC0) AFEC Analog Control Register */
#define REG_AFEC0_SHMR          (*(__IO uint32_t*)0x4003C0A0U) /**< (AFEC0) AFEC Sample & Hold Mode Register */
#define REG_AFEC0_COSR          (*(__IO uint32_t*)0x4003C0D0U) /**< (AFEC0) AFEC Correction Select Register */
#define REG_AFEC0_CVR           (*(__IO uint32_t*)0x4003C0D4U) /**< (AFEC0) AFEC Correction Values Register */
#define REG_AFEC0_CECR          (*(__IO uint32_t*)0x4003C0D8U) /**< (AFEC0) AFEC Channel Error Correction Register */
#define REG_AFEC0_WPMR          (*(__IO uint32_t*)0x4003C0E4U) /**< (AFEC0) AFEC Write Protection Mode Register */
#define REG_AFEC0_WPSR          (*(__I  uint32_t*)0x4003C0E8U) /**< (AFEC0) AFEC Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for AFEC0 peripheral ========== */
#define AFEC0_INSTANCE_ID                        29        
#define AFEC0_DMAC_ID_RX                         35        

#endif /* _SAME70_AFEC0_INSTANCE_ */
