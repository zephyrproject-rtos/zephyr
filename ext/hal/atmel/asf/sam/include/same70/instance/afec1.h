/**
 * \file
 *
 * \brief Instance description for AFEC1
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

#ifndef _SAME70_AFEC1_INSTANCE_H_
#define _SAME70_AFEC1_INSTANCE_H_

/* ========== Register definition for AFEC1 peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_AFEC1_CR            (0x40064000) /**< (AFEC1) AFEC Control Register */
#define REG_AFEC1_MR            (0x40064004) /**< (AFEC1) AFEC Mode Register */
#define REG_AFEC1_EMR           (0x40064008) /**< (AFEC1) AFEC Extended Mode Register */
#define REG_AFEC1_SEQ1R         (0x4006400C) /**< (AFEC1) AFEC Channel Sequence 1 Register */
#define REG_AFEC1_SEQ2R         (0x40064010) /**< (AFEC1) AFEC Channel Sequence 2 Register */
#define REG_AFEC1_CHER          (0x40064014) /**< (AFEC1) AFEC Channel Enable Register */
#define REG_AFEC1_CHDR          (0x40064018) /**< (AFEC1) AFEC Channel Disable Register */
#define REG_AFEC1_CHSR          (0x4006401C) /**< (AFEC1) AFEC Channel Status Register */
#define REG_AFEC1_LCDR          (0x40064020) /**< (AFEC1) AFEC Last Converted Data Register */
#define REG_AFEC1_IER           (0x40064024) /**< (AFEC1) AFEC Interrupt Enable Register */
#define REG_AFEC1_IDR           (0x40064028) /**< (AFEC1) AFEC Interrupt Disable Register */
#define REG_AFEC1_IMR           (0x4006402C) /**< (AFEC1) AFEC Interrupt Mask Register */
#define REG_AFEC1_ISR           (0x40064030) /**< (AFEC1) AFEC Interrupt Status Register */
#define REG_AFEC1_OVER          (0x4006404C) /**< (AFEC1) AFEC Overrun Status Register */
#define REG_AFEC1_CWR           (0x40064050) /**< (AFEC1) AFEC Compare Window Register */
#define REG_AFEC1_CGR           (0x40064054) /**< (AFEC1) AFEC Channel Gain Register */
#define REG_AFEC1_DIFFR         (0x40064060) /**< (AFEC1) AFEC Channel Differential Register */
#define REG_AFEC1_CSELR         (0x40064064) /**< (AFEC1) AFEC Channel Selection Register */
#define REG_AFEC1_CDR           (0x40064068) /**< (AFEC1) AFEC Channel Data Register */
#define REG_AFEC1_COCR          (0x4006406C) /**< (AFEC1) AFEC Channel Offset Compensation Register */
#define REG_AFEC1_TEMPMR        (0x40064070) /**< (AFEC1) AFEC Temperature Sensor Mode Register */
#define REG_AFEC1_TEMPCWR       (0x40064074) /**< (AFEC1) AFEC Temperature Compare Window Register */
#define REG_AFEC1_ACR           (0x40064094) /**< (AFEC1) AFEC Analog Control Register */
#define REG_AFEC1_SHMR          (0x400640A0) /**< (AFEC1) AFEC Sample & Hold Mode Register */
#define REG_AFEC1_COSR          (0x400640D0) /**< (AFEC1) AFEC Correction Select Register */
#define REG_AFEC1_CVR           (0x400640D4) /**< (AFEC1) AFEC Correction Values Register */
#define REG_AFEC1_CECR          (0x400640D8) /**< (AFEC1) AFEC Channel Error Correction Register */
#define REG_AFEC1_WPMR          (0x400640E4) /**< (AFEC1) AFEC Write Protection Mode Register */
#define REG_AFEC1_WPSR          (0x400640E8) /**< (AFEC1) AFEC Write Protection Status Register */

#else

#define REG_AFEC1_CR            (*(__O  uint32_t*)0x40064000U) /**< (AFEC1) AFEC Control Register */
#define REG_AFEC1_MR            (*(__IO uint32_t*)0x40064004U) /**< (AFEC1) AFEC Mode Register */
#define REG_AFEC1_EMR           (*(__IO uint32_t*)0x40064008U) /**< (AFEC1) AFEC Extended Mode Register */
#define REG_AFEC1_SEQ1R         (*(__IO uint32_t*)0x4006400CU) /**< (AFEC1) AFEC Channel Sequence 1 Register */
#define REG_AFEC1_SEQ2R         (*(__IO uint32_t*)0x40064010U) /**< (AFEC1) AFEC Channel Sequence 2 Register */
#define REG_AFEC1_CHER          (*(__O  uint32_t*)0x40064014U) /**< (AFEC1) AFEC Channel Enable Register */
#define REG_AFEC1_CHDR          (*(__O  uint32_t*)0x40064018U) /**< (AFEC1) AFEC Channel Disable Register */
#define REG_AFEC1_CHSR          (*(__I  uint32_t*)0x4006401CU) /**< (AFEC1) AFEC Channel Status Register */
#define REG_AFEC1_LCDR          (*(__I  uint32_t*)0x40064020U) /**< (AFEC1) AFEC Last Converted Data Register */
#define REG_AFEC1_IER           (*(__O  uint32_t*)0x40064024U) /**< (AFEC1) AFEC Interrupt Enable Register */
#define REG_AFEC1_IDR           (*(__O  uint32_t*)0x40064028U) /**< (AFEC1) AFEC Interrupt Disable Register */
#define REG_AFEC1_IMR           (*(__I  uint32_t*)0x4006402CU) /**< (AFEC1) AFEC Interrupt Mask Register */
#define REG_AFEC1_ISR           (*(__I  uint32_t*)0x40064030U) /**< (AFEC1) AFEC Interrupt Status Register */
#define REG_AFEC1_OVER          (*(__I  uint32_t*)0x4006404CU) /**< (AFEC1) AFEC Overrun Status Register */
#define REG_AFEC1_CWR           (*(__IO uint32_t*)0x40064050U) /**< (AFEC1) AFEC Compare Window Register */
#define REG_AFEC1_CGR           (*(__IO uint32_t*)0x40064054U) /**< (AFEC1) AFEC Channel Gain Register */
#define REG_AFEC1_DIFFR         (*(__IO uint32_t*)0x40064060U) /**< (AFEC1) AFEC Channel Differential Register */
#define REG_AFEC1_CSELR         (*(__IO uint32_t*)0x40064064U) /**< (AFEC1) AFEC Channel Selection Register */
#define REG_AFEC1_CDR           (*(__I  uint32_t*)0x40064068U) /**< (AFEC1) AFEC Channel Data Register */
#define REG_AFEC1_COCR          (*(__IO uint32_t*)0x4006406CU) /**< (AFEC1) AFEC Channel Offset Compensation Register */
#define REG_AFEC1_TEMPMR        (*(__IO uint32_t*)0x40064070U) /**< (AFEC1) AFEC Temperature Sensor Mode Register */
#define REG_AFEC1_TEMPCWR       (*(__IO uint32_t*)0x40064074U) /**< (AFEC1) AFEC Temperature Compare Window Register */
#define REG_AFEC1_ACR           (*(__IO uint32_t*)0x40064094U) /**< (AFEC1) AFEC Analog Control Register */
#define REG_AFEC1_SHMR          (*(__IO uint32_t*)0x400640A0U) /**< (AFEC1) AFEC Sample & Hold Mode Register */
#define REG_AFEC1_COSR          (*(__IO uint32_t*)0x400640D0U) /**< (AFEC1) AFEC Correction Select Register */
#define REG_AFEC1_CVR           (*(__IO uint32_t*)0x400640D4U) /**< (AFEC1) AFEC Correction Values Register */
#define REG_AFEC1_CECR          (*(__IO uint32_t*)0x400640D8U) /**< (AFEC1) AFEC Channel Error Correction Register */
#define REG_AFEC1_WPMR          (*(__IO uint32_t*)0x400640E4U) /**< (AFEC1) AFEC Write Protection Mode Register */
#define REG_AFEC1_WPSR          (*(__I  uint32_t*)0x400640E8U) /**< (AFEC1) AFEC Write Protection Status Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for AFEC1 peripheral ========== */
#define AFEC1_INSTANCE_ID                        40        
#define AFEC1_DMAC_ID_RX                         36        

#endif /* _SAME70_AFEC1_INSTANCE_ */
