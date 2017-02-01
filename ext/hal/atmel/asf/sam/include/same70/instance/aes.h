/**
 * \file
 *
 * \brief Instance description for AES
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

#ifndef _SAME70_AES_INSTANCE_H_
#define _SAME70_AES_INSTANCE_H_

/* ========== Register definition for AES peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_AES_CR              (0x4006C000) /**< (AES) Control Register */
#define REG_AES_MR              (0x4006C004) /**< (AES) Mode Register */
#define REG_AES_IER             (0x4006C010) /**< (AES) Interrupt Enable Register */
#define REG_AES_IDR             (0x4006C014) /**< (AES) Interrupt Disable Register */
#define REG_AES_IMR             (0x4006C018) /**< (AES) Interrupt Mask Register */
#define REG_AES_ISR             (0x4006C01C) /**< (AES) Interrupt Status Register */
#define REG_AES_KEYWR           (0x4006C020) /**< (AES) Key Word Register 0 */
#define REG_AES_KEYWR0          (0x4006C020) /**< (AES) Key Word Register 0 */
#define REG_AES_KEYWR1          (0x4006C024) /**< (AES) Key Word Register 1 */
#define REG_AES_KEYWR2          (0x4006C028) /**< (AES) Key Word Register 2 */
#define REG_AES_KEYWR3          (0x4006C02C) /**< (AES) Key Word Register 3 */
#define REG_AES_KEYWR4          (0x4006C030) /**< (AES) Key Word Register 4 */
#define REG_AES_KEYWR5          (0x4006C034) /**< (AES) Key Word Register 5 */
#define REG_AES_KEYWR6          (0x4006C038) /**< (AES) Key Word Register 6 */
#define REG_AES_KEYWR7          (0x4006C03C) /**< (AES) Key Word Register 7 */
#define REG_AES_IDATAR          (0x4006C040) /**< (AES) Input Data Register 0 */
#define REG_AES_IDATAR0         (0x4006C040) /**< (AES) Input Data Register 0 */
#define REG_AES_IDATAR1         (0x4006C044) /**< (AES) Input Data Register 1 */
#define REG_AES_IDATAR2         (0x4006C048) /**< (AES) Input Data Register 2 */
#define REG_AES_IDATAR3         (0x4006C04C) /**< (AES) Input Data Register 3 */
#define REG_AES_ODATAR          (0x4006C050) /**< (AES) Output Data Register 0 */
#define REG_AES_ODATAR0         (0x4006C050) /**< (AES) Output Data Register 0 */
#define REG_AES_ODATAR1         (0x4006C054) /**< (AES) Output Data Register 1 */
#define REG_AES_ODATAR2         (0x4006C058) /**< (AES) Output Data Register 2 */
#define REG_AES_ODATAR3         (0x4006C05C) /**< (AES) Output Data Register 3 */
#define REG_AES_IVR             (0x4006C060) /**< (AES) Initialization Vector Register 0 */
#define REG_AES_IVR0            (0x4006C060) /**< (AES) Initialization Vector Register 0 */
#define REG_AES_IVR1            (0x4006C064) /**< (AES) Initialization Vector Register 1 */
#define REG_AES_IVR2            (0x4006C068) /**< (AES) Initialization Vector Register 2 */
#define REG_AES_IVR3            (0x4006C06C) /**< (AES) Initialization Vector Register 3 */
#define REG_AES_AADLENR         (0x4006C070) /**< (AES) Additional Authenticated Data Length Register */
#define REG_AES_CLENR           (0x4006C074) /**< (AES) Plaintext/Ciphertext Length Register */
#define REG_AES_GHASHR          (0x4006C078) /**< (AES) GCM Intermediate Hash Word Register 0 */
#define REG_AES_GHASHR0         (0x4006C078) /**< (AES) GCM Intermediate Hash Word Register 0 */
#define REG_AES_GHASHR1         (0x4006C07C) /**< (AES) GCM Intermediate Hash Word Register 1 */
#define REG_AES_GHASHR2         (0x4006C080) /**< (AES) GCM Intermediate Hash Word Register 2 */
#define REG_AES_GHASHR3         (0x4006C084) /**< (AES) GCM Intermediate Hash Word Register 3 */
#define REG_AES_TAGR            (0x4006C088) /**< (AES) GCM Authentication Tag Word Register 0 */
#define REG_AES_TAGR0           (0x4006C088) /**< (AES) GCM Authentication Tag Word Register 0 */
#define REG_AES_TAGR1           (0x4006C08C) /**< (AES) GCM Authentication Tag Word Register 1 */
#define REG_AES_TAGR2           (0x4006C090) /**< (AES) GCM Authentication Tag Word Register 2 */
#define REG_AES_TAGR3           (0x4006C094) /**< (AES) GCM Authentication Tag Word Register 3 */
#define REG_AES_CTRR            (0x4006C098) /**< (AES) GCM Encryption Counter Value Register */
#define REG_AES_GCMHR           (0x4006C09C) /**< (AES) GCM H Word Register 0 */
#define REG_AES_GCMHR0          (0x4006C09C) /**< (AES) GCM H Word Register 0 */
#define REG_AES_GCMHR1          (0x4006C0A0) /**< (AES) GCM H Word Register 1 */
#define REG_AES_GCMHR2          (0x4006C0A4) /**< (AES) GCM H Word Register 2 */
#define REG_AES_GCMHR3          (0x4006C0A8) /**< (AES) GCM H Word Register 3 */

#else

#define REG_AES_CR              (*(__O  uint32_t*)0x4006C000U) /**< (AES) Control Register */
#define REG_AES_MR              (*(__IO uint32_t*)0x4006C004U) /**< (AES) Mode Register */
#define REG_AES_IER             (*(__O  uint32_t*)0x4006C010U) /**< (AES) Interrupt Enable Register */
#define REG_AES_IDR             (*(__O  uint32_t*)0x4006C014U) /**< (AES) Interrupt Disable Register */
#define REG_AES_IMR             (*(__I  uint32_t*)0x4006C018U) /**< (AES) Interrupt Mask Register */
#define REG_AES_ISR             (*(__I  uint32_t*)0x4006C01CU) /**< (AES) Interrupt Status Register */
#define REG_AES_KEYWR           (*(__O  uint32_t*)0x4006C020U) /**< (AES) Key Word Register 0 */
#define REG_AES_KEYWR0          (*(__O  uint32_t*)0x4006C020U) /**< (AES) Key Word Register 0 */
#define REG_AES_KEYWR1          (*(__O  uint32_t*)0x4006C024U) /**< (AES) Key Word Register 1 */
#define REG_AES_KEYWR2          (*(__O  uint32_t*)0x4006C028U) /**< (AES) Key Word Register 2 */
#define REG_AES_KEYWR3          (*(__O  uint32_t*)0x4006C02CU) /**< (AES) Key Word Register 3 */
#define REG_AES_KEYWR4          (*(__O  uint32_t*)0x4006C030U) /**< (AES) Key Word Register 4 */
#define REG_AES_KEYWR5          (*(__O  uint32_t*)0x4006C034U) /**< (AES) Key Word Register 5 */
#define REG_AES_KEYWR6          (*(__O  uint32_t*)0x4006C038U) /**< (AES) Key Word Register 6 */
#define REG_AES_KEYWR7          (*(__O  uint32_t*)0x4006C03CU) /**< (AES) Key Word Register 7 */
#define REG_AES_IDATAR          (*(__O  uint32_t*)0x4006C040U) /**< (AES) Input Data Register 0 */
#define REG_AES_IDATAR0         (*(__O  uint32_t*)0x4006C040U) /**< (AES) Input Data Register 0 */
#define REG_AES_IDATAR1         (*(__O  uint32_t*)0x4006C044U) /**< (AES) Input Data Register 1 */
#define REG_AES_IDATAR2         (*(__O  uint32_t*)0x4006C048U) /**< (AES) Input Data Register 2 */
#define REG_AES_IDATAR3         (*(__O  uint32_t*)0x4006C04CU) /**< (AES) Input Data Register 3 */
#define REG_AES_ODATAR          (*(__I  uint32_t*)0x4006C050U) /**< (AES) Output Data Register 0 */
#define REG_AES_ODATAR0         (*(__I  uint32_t*)0x4006C050U) /**< (AES) Output Data Register 0 */
#define REG_AES_ODATAR1         (*(__I  uint32_t*)0x4006C054U) /**< (AES) Output Data Register 1 */
#define REG_AES_ODATAR2         (*(__I  uint32_t*)0x4006C058U) /**< (AES) Output Data Register 2 */
#define REG_AES_ODATAR3         (*(__I  uint32_t*)0x4006C05CU) /**< (AES) Output Data Register 3 */
#define REG_AES_IVR             (*(__O  uint32_t*)0x4006C060U) /**< (AES) Initialization Vector Register 0 */
#define REG_AES_IVR0            (*(__O  uint32_t*)0x4006C060U) /**< (AES) Initialization Vector Register 0 */
#define REG_AES_IVR1            (*(__O  uint32_t*)0x4006C064U) /**< (AES) Initialization Vector Register 1 */
#define REG_AES_IVR2            (*(__O  uint32_t*)0x4006C068U) /**< (AES) Initialization Vector Register 2 */
#define REG_AES_IVR3            (*(__O  uint32_t*)0x4006C06CU) /**< (AES) Initialization Vector Register 3 */
#define REG_AES_AADLENR         (*(__IO uint32_t*)0x4006C070U) /**< (AES) Additional Authenticated Data Length Register */
#define REG_AES_CLENR           (*(__IO uint32_t*)0x4006C074U) /**< (AES) Plaintext/Ciphertext Length Register */
#define REG_AES_GHASHR          (*(__IO uint32_t*)0x4006C078U) /**< (AES) GCM Intermediate Hash Word Register 0 */
#define REG_AES_GHASHR0         (*(__IO uint32_t*)0x4006C078U) /**< (AES) GCM Intermediate Hash Word Register 0 */
#define REG_AES_GHASHR1         (*(__IO uint32_t*)0x4006C07CU) /**< (AES) GCM Intermediate Hash Word Register 1 */
#define REG_AES_GHASHR2         (*(__IO uint32_t*)0x4006C080U) /**< (AES) GCM Intermediate Hash Word Register 2 */
#define REG_AES_GHASHR3         (*(__IO uint32_t*)0x4006C084U) /**< (AES) GCM Intermediate Hash Word Register 3 */
#define REG_AES_TAGR            (*(__I  uint32_t*)0x4006C088U) /**< (AES) GCM Authentication Tag Word Register 0 */
#define REG_AES_TAGR0           (*(__I  uint32_t*)0x4006C088U) /**< (AES) GCM Authentication Tag Word Register 0 */
#define REG_AES_TAGR1           (*(__I  uint32_t*)0x4006C08CU) /**< (AES) GCM Authentication Tag Word Register 1 */
#define REG_AES_TAGR2           (*(__I  uint32_t*)0x4006C090U) /**< (AES) GCM Authentication Tag Word Register 2 */
#define REG_AES_TAGR3           (*(__I  uint32_t*)0x4006C094U) /**< (AES) GCM Authentication Tag Word Register 3 */
#define REG_AES_CTRR            (*(__I  uint32_t*)0x4006C098U) /**< (AES) GCM Encryption Counter Value Register */
#define REG_AES_GCMHR           (*(__IO uint32_t*)0x4006C09CU) /**< (AES) GCM H Word Register 0 */
#define REG_AES_GCMHR0          (*(__IO uint32_t*)0x4006C09CU) /**< (AES) GCM H Word Register 0 */
#define REG_AES_GCMHR1          (*(__IO uint32_t*)0x4006C0A0U) /**< (AES) GCM H Word Register 1 */
#define REG_AES_GCMHR2          (*(__IO uint32_t*)0x4006C0A4U) /**< (AES) GCM H Word Register 2 */
#define REG_AES_GCMHR3          (*(__IO uint32_t*)0x4006C0A8U) /**< (AES) GCM H Word Register 3 */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for AES peripheral ========== */
#define AES_INSTANCE_ID                          56        
#define AES_DMAC_ID_TX                           37        
#define AES_DMAC_ID_RX                           38        

#endif /* _SAME70_AES_INSTANCE_ */
