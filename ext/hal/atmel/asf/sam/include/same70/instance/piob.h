/**
 * \file
 *
 * \brief Instance description for PIOB
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

#ifndef _SAME70_PIOB_INSTANCE_H_
#define _SAME70_PIOB_INSTANCE_H_

/* ========== Register definition for PIOB peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PIOB_PER            (0x400E1000) /**< (PIOB) PIO Enable Register */
#define REG_PIOB_PDR            (0x400E1004) /**< (PIOB) PIO Disable Register */
#define REG_PIOB_PSR            (0x400E1008) /**< (PIOB) PIO Status Register */
#define REG_PIOB_OER            (0x400E1010) /**< (PIOB) Output Enable Register */
#define REG_PIOB_ODR            (0x400E1014) /**< (PIOB) Output Disable Register */
#define REG_PIOB_OSR            (0x400E1018) /**< (PIOB) Output Status Register */
#define REG_PIOB_IFER           (0x400E1020) /**< (PIOB) Glitch Input Filter Enable Register */
#define REG_PIOB_IFDR           (0x400E1024) /**< (PIOB) Glitch Input Filter Disable Register */
#define REG_PIOB_IFSR           (0x400E1028) /**< (PIOB) Glitch Input Filter Status Register */
#define REG_PIOB_SODR           (0x400E1030) /**< (PIOB) Set Output Data Register */
#define REG_PIOB_CODR           (0x400E1034) /**< (PIOB) Clear Output Data Register */
#define REG_PIOB_ODSR           (0x400E1038) /**< (PIOB) Output Data Status Register */
#define REG_PIOB_PDSR           (0x400E103C) /**< (PIOB) Pin Data Status Register */
#define REG_PIOB_IER            (0x400E1040) /**< (PIOB) Interrupt Enable Register */
#define REG_PIOB_IDR            (0x400E1044) /**< (PIOB) Interrupt Disable Register */
#define REG_PIOB_IMR            (0x400E1048) /**< (PIOB) Interrupt Mask Register */
#define REG_PIOB_ISR            (0x400E104C) /**< (PIOB) Interrupt Status Register */
#define REG_PIOB_MDER           (0x400E1050) /**< (PIOB) Multi-driver Enable Register */
#define REG_PIOB_MDDR           (0x400E1054) /**< (PIOB) Multi-driver Disable Register */
#define REG_PIOB_MDSR           (0x400E1058) /**< (PIOB) Multi-driver Status Register */
#define REG_PIOB_PUDR           (0x400E1060) /**< (PIOB) Pull-up Disable Register */
#define REG_PIOB_PUER           (0x400E1064) /**< (PIOB) Pull-up Enable Register */
#define REG_PIOB_PUSR           (0x400E1068) /**< (PIOB) Pad Pull-up Status Register */
#define REG_PIOB_ABCDSR         (0x400E1070) /**< (PIOB) Peripheral Select Register 0 */
#define REG_PIOB_ABCDSR0        (0x400E1070) /**< (PIOB) Peripheral Select Register 0 */
#define REG_PIOB_ABCDSR1        (0x400E1074) /**< (PIOB) Peripheral Select Register 1 */
#define REG_PIOB_IFSCDR         (0x400E1080) /**< (PIOB) Input Filter Slow Clock Disable Register */
#define REG_PIOB_IFSCER         (0x400E1084) /**< (PIOB) Input Filter Slow Clock Enable Register */
#define REG_PIOB_IFSCSR         (0x400E1088) /**< (PIOB) Input Filter Slow Clock Status Register */
#define REG_PIOB_SCDR           (0x400E108C) /**< (PIOB) Slow Clock Divider Debouncing Register */
#define REG_PIOB_PPDDR          (0x400E1090) /**< (PIOB) Pad Pull-down Disable Register */
#define REG_PIOB_PPDER          (0x400E1094) /**< (PIOB) Pad Pull-down Enable Register */
#define REG_PIOB_PPDSR          (0x400E1098) /**< (PIOB) Pad Pull-down Status Register */
#define REG_PIOB_OWER           (0x400E10A0) /**< (PIOB) Output Write Enable */
#define REG_PIOB_OWDR           (0x400E10A4) /**< (PIOB) Output Write Disable */
#define REG_PIOB_OWSR           (0x400E10A8) /**< (PIOB) Output Write Status Register */
#define REG_PIOB_AIMER          (0x400E10B0) /**< (PIOB) Additional Interrupt Modes Enable Register */
#define REG_PIOB_AIMDR          (0x400E10B4) /**< (PIOB) Additional Interrupt Modes Disable Register */
#define REG_PIOB_AIMMR          (0x400E10B8) /**< (PIOB) Additional Interrupt Modes Mask Register */
#define REG_PIOB_ESR            (0x400E10C0) /**< (PIOB) Edge Select Register */
#define REG_PIOB_LSR            (0x400E10C4) /**< (PIOB) Level Select Register */
#define REG_PIOB_ELSR           (0x400E10C8) /**< (PIOB) Edge/Level Status Register */
#define REG_PIOB_FELLSR         (0x400E10D0) /**< (PIOB) Falling Edge/Low-Level Select Register */
#define REG_PIOB_REHLSR         (0x400E10D4) /**< (PIOB) Rising Edge/High-Level Select Register */
#define REG_PIOB_FRLHSR         (0x400E10D8) /**< (PIOB) Fall/Rise - Low/High Status Register */
#define REG_PIOB_LOCKSR         (0x400E10E0) /**< (PIOB) Lock Status */
#define REG_PIOB_WPMR           (0x400E10E4) /**< (PIOB) Write Protection Mode Register */
#define REG_PIOB_WPSR           (0x400E10E8) /**< (PIOB) Write Protection Status Register */
#define REG_PIOB_SCHMITT        (0x400E1100) /**< (PIOB) Schmitt Trigger Register */
#define REG_PIOB_DRIVER         (0x400E1118) /**< (PIOB) I/O Drive Register */
#define REG_PIOB_PCMR           (0x400E1150) /**< (PIOB) Parallel Capture Mode Register */
#define REG_PIOB_PCIER          (0x400E1154) /**< (PIOB) Parallel Capture Interrupt Enable Register */
#define REG_PIOB_PCIDR          (0x400E1158) /**< (PIOB) Parallel Capture Interrupt Disable Register */
#define REG_PIOB_PCIMR          (0x400E115C) /**< (PIOB) Parallel Capture Interrupt Mask Register */
#define REG_PIOB_PCISR          (0x400E1160) /**< (PIOB) Parallel Capture Interrupt Status Register */
#define REG_PIOB_PCRHR          (0x400E1164) /**< (PIOB) Parallel Capture Reception Holding Register */

#else

#define REG_PIOB_PER            (*(__O  uint32_t*)0x400E1000U) /**< (PIOB) PIO Enable Register */
#define REG_PIOB_PDR            (*(__O  uint32_t*)0x400E1004U) /**< (PIOB) PIO Disable Register */
#define REG_PIOB_PSR            (*(__I  uint32_t*)0x400E1008U) /**< (PIOB) PIO Status Register */
#define REG_PIOB_OER            (*(__O  uint32_t*)0x400E1010U) /**< (PIOB) Output Enable Register */
#define REG_PIOB_ODR            (*(__O  uint32_t*)0x400E1014U) /**< (PIOB) Output Disable Register */
#define REG_PIOB_OSR            (*(__I  uint32_t*)0x400E1018U) /**< (PIOB) Output Status Register */
#define REG_PIOB_IFER           (*(__O  uint32_t*)0x400E1020U) /**< (PIOB) Glitch Input Filter Enable Register */
#define REG_PIOB_IFDR           (*(__O  uint32_t*)0x400E1024U) /**< (PIOB) Glitch Input Filter Disable Register */
#define REG_PIOB_IFSR           (*(__I  uint32_t*)0x400E1028U) /**< (PIOB) Glitch Input Filter Status Register */
#define REG_PIOB_SODR           (*(__O  uint32_t*)0x400E1030U) /**< (PIOB) Set Output Data Register */
#define REG_PIOB_CODR           (*(__O  uint32_t*)0x400E1034U) /**< (PIOB) Clear Output Data Register */
#define REG_PIOB_ODSR           (*(__IO uint32_t*)0x400E1038U) /**< (PIOB) Output Data Status Register */
#define REG_PIOB_PDSR           (*(__I  uint32_t*)0x400E103CU) /**< (PIOB) Pin Data Status Register */
#define REG_PIOB_IER            (*(__O  uint32_t*)0x400E1040U) /**< (PIOB) Interrupt Enable Register */
#define REG_PIOB_IDR            (*(__O  uint32_t*)0x400E1044U) /**< (PIOB) Interrupt Disable Register */
#define REG_PIOB_IMR            (*(__I  uint32_t*)0x400E1048U) /**< (PIOB) Interrupt Mask Register */
#define REG_PIOB_ISR            (*(__I  uint32_t*)0x400E104CU) /**< (PIOB) Interrupt Status Register */
#define REG_PIOB_MDER           (*(__O  uint32_t*)0x400E1050U) /**< (PIOB) Multi-driver Enable Register */
#define REG_PIOB_MDDR           (*(__O  uint32_t*)0x400E1054U) /**< (PIOB) Multi-driver Disable Register */
#define REG_PIOB_MDSR           (*(__I  uint32_t*)0x400E1058U) /**< (PIOB) Multi-driver Status Register */
#define REG_PIOB_PUDR           (*(__O  uint32_t*)0x400E1060U) /**< (PIOB) Pull-up Disable Register */
#define REG_PIOB_PUER           (*(__O  uint32_t*)0x400E1064U) /**< (PIOB) Pull-up Enable Register */
#define REG_PIOB_PUSR           (*(__I  uint32_t*)0x400E1068U) /**< (PIOB) Pad Pull-up Status Register */
#define REG_PIOB_ABCDSR         (*(__IO uint32_t*)0x400E1070U) /**< (PIOB) Peripheral Select Register 0 */
#define REG_PIOB_ABCDSR0        (*(__IO uint32_t*)0x400E1070U) /**< (PIOB) Peripheral Select Register 0 */
#define REG_PIOB_ABCDSR1        (*(__IO uint32_t*)0x400E1074U) /**< (PIOB) Peripheral Select Register 1 */
#define REG_PIOB_IFSCDR         (*(__O  uint32_t*)0x400E1080U) /**< (PIOB) Input Filter Slow Clock Disable Register */
#define REG_PIOB_IFSCER         (*(__O  uint32_t*)0x400E1084U) /**< (PIOB) Input Filter Slow Clock Enable Register */
#define REG_PIOB_IFSCSR         (*(__I  uint32_t*)0x400E1088U) /**< (PIOB) Input Filter Slow Clock Status Register */
#define REG_PIOB_SCDR           (*(__IO uint32_t*)0x400E108CU) /**< (PIOB) Slow Clock Divider Debouncing Register */
#define REG_PIOB_PPDDR          (*(__O  uint32_t*)0x400E1090U) /**< (PIOB) Pad Pull-down Disable Register */
#define REG_PIOB_PPDER          (*(__O  uint32_t*)0x400E1094U) /**< (PIOB) Pad Pull-down Enable Register */
#define REG_PIOB_PPDSR          (*(__I  uint32_t*)0x400E1098U) /**< (PIOB) Pad Pull-down Status Register */
#define REG_PIOB_OWER           (*(__O  uint32_t*)0x400E10A0U) /**< (PIOB) Output Write Enable */
#define REG_PIOB_OWDR           (*(__O  uint32_t*)0x400E10A4U) /**< (PIOB) Output Write Disable */
#define REG_PIOB_OWSR           (*(__I  uint32_t*)0x400E10A8U) /**< (PIOB) Output Write Status Register */
#define REG_PIOB_AIMER          (*(__O  uint32_t*)0x400E10B0U) /**< (PIOB) Additional Interrupt Modes Enable Register */
#define REG_PIOB_AIMDR          (*(__O  uint32_t*)0x400E10B4U) /**< (PIOB) Additional Interrupt Modes Disable Register */
#define REG_PIOB_AIMMR          (*(__I  uint32_t*)0x400E10B8U) /**< (PIOB) Additional Interrupt Modes Mask Register */
#define REG_PIOB_ESR            (*(__O  uint32_t*)0x400E10C0U) /**< (PIOB) Edge Select Register */
#define REG_PIOB_LSR            (*(__O  uint32_t*)0x400E10C4U) /**< (PIOB) Level Select Register */
#define REG_PIOB_ELSR           (*(__I  uint32_t*)0x400E10C8U) /**< (PIOB) Edge/Level Status Register */
#define REG_PIOB_FELLSR         (*(__O  uint32_t*)0x400E10D0U) /**< (PIOB) Falling Edge/Low-Level Select Register */
#define REG_PIOB_REHLSR         (*(__O  uint32_t*)0x400E10D4U) /**< (PIOB) Rising Edge/High-Level Select Register */
#define REG_PIOB_FRLHSR         (*(__I  uint32_t*)0x400E10D8U) /**< (PIOB) Fall/Rise - Low/High Status Register */
#define REG_PIOB_LOCKSR         (*(__I  uint32_t*)0x400E10E0U) /**< (PIOB) Lock Status */
#define REG_PIOB_WPMR           (*(__IO uint32_t*)0x400E10E4U) /**< (PIOB) Write Protection Mode Register */
#define REG_PIOB_WPSR           (*(__I  uint32_t*)0x400E10E8U) /**< (PIOB) Write Protection Status Register */
#define REG_PIOB_SCHMITT        (*(__IO uint32_t*)0x400E1100U) /**< (PIOB) Schmitt Trigger Register */
#define REG_PIOB_DRIVER         (*(__IO uint32_t*)0x400E1118U) /**< (PIOB) I/O Drive Register */
#define REG_PIOB_PCMR           (*(__IO uint32_t*)0x400E1150U) /**< (PIOB) Parallel Capture Mode Register */
#define REG_PIOB_PCIER          (*(__O  uint32_t*)0x400E1154U) /**< (PIOB) Parallel Capture Interrupt Enable Register */
#define REG_PIOB_PCIDR          (*(__O  uint32_t*)0x400E1158U) /**< (PIOB) Parallel Capture Interrupt Disable Register */
#define REG_PIOB_PCIMR          (*(__I  uint32_t*)0x400E115CU) /**< (PIOB) Parallel Capture Interrupt Mask Register */
#define REG_PIOB_PCISR          (*(__I  uint32_t*)0x400E1160U) /**< (PIOB) Parallel Capture Interrupt Status Register */
#define REG_PIOB_PCRHR          (*(__I  uint32_t*)0x400E1164U) /**< (PIOB) Parallel Capture Reception Holding Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PIOB peripheral ========== */
#define PIOB_INSTANCE_ID                         11        

#endif /* _SAME70_PIOB_INSTANCE_ */
