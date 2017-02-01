/**
 * \file
 *
 * \brief Instance description for PIOC
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

#ifndef _SAME70_PIOC_INSTANCE_H_
#define _SAME70_PIOC_INSTANCE_H_

/* ========== Register definition for PIOC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PIOC_PER            (0x400E1200) /**< (PIOC) PIO Enable Register */
#define REG_PIOC_PDR            (0x400E1204) /**< (PIOC) PIO Disable Register */
#define REG_PIOC_PSR            (0x400E1208) /**< (PIOC) PIO Status Register */
#define REG_PIOC_OER            (0x400E1210) /**< (PIOC) Output Enable Register */
#define REG_PIOC_ODR            (0x400E1214) /**< (PIOC) Output Disable Register */
#define REG_PIOC_OSR            (0x400E1218) /**< (PIOC) Output Status Register */
#define REG_PIOC_IFER           (0x400E1220) /**< (PIOC) Glitch Input Filter Enable Register */
#define REG_PIOC_IFDR           (0x400E1224) /**< (PIOC) Glitch Input Filter Disable Register */
#define REG_PIOC_IFSR           (0x400E1228) /**< (PIOC) Glitch Input Filter Status Register */
#define REG_PIOC_SODR           (0x400E1230) /**< (PIOC) Set Output Data Register */
#define REG_PIOC_CODR           (0x400E1234) /**< (PIOC) Clear Output Data Register */
#define REG_PIOC_ODSR           (0x400E1238) /**< (PIOC) Output Data Status Register */
#define REG_PIOC_PDSR           (0x400E123C) /**< (PIOC) Pin Data Status Register */
#define REG_PIOC_IER            (0x400E1240) /**< (PIOC) Interrupt Enable Register */
#define REG_PIOC_IDR            (0x400E1244) /**< (PIOC) Interrupt Disable Register */
#define REG_PIOC_IMR            (0x400E1248) /**< (PIOC) Interrupt Mask Register */
#define REG_PIOC_ISR            (0x400E124C) /**< (PIOC) Interrupt Status Register */
#define REG_PIOC_MDER           (0x400E1250) /**< (PIOC) Multi-driver Enable Register */
#define REG_PIOC_MDDR           (0x400E1254) /**< (PIOC) Multi-driver Disable Register */
#define REG_PIOC_MDSR           (0x400E1258) /**< (PIOC) Multi-driver Status Register */
#define REG_PIOC_PUDR           (0x400E1260) /**< (PIOC) Pull-up Disable Register */
#define REG_PIOC_PUER           (0x400E1264) /**< (PIOC) Pull-up Enable Register */
#define REG_PIOC_PUSR           (0x400E1268) /**< (PIOC) Pad Pull-up Status Register */
#define REG_PIOC_ABCDSR         (0x400E1270) /**< (PIOC) Peripheral Select Register 0 */
#define REG_PIOC_ABCDSR0        (0x400E1270) /**< (PIOC) Peripheral Select Register 0 */
#define REG_PIOC_ABCDSR1        (0x400E1274) /**< (PIOC) Peripheral Select Register 1 */
#define REG_PIOC_IFSCDR         (0x400E1280) /**< (PIOC) Input Filter Slow Clock Disable Register */
#define REG_PIOC_IFSCER         (0x400E1284) /**< (PIOC) Input Filter Slow Clock Enable Register */
#define REG_PIOC_IFSCSR         (0x400E1288) /**< (PIOC) Input Filter Slow Clock Status Register */
#define REG_PIOC_SCDR           (0x400E128C) /**< (PIOC) Slow Clock Divider Debouncing Register */
#define REG_PIOC_PPDDR          (0x400E1290) /**< (PIOC) Pad Pull-down Disable Register */
#define REG_PIOC_PPDER          (0x400E1294) /**< (PIOC) Pad Pull-down Enable Register */
#define REG_PIOC_PPDSR          (0x400E1298) /**< (PIOC) Pad Pull-down Status Register */
#define REG_PIOC_OWER           (0x400E12A0) /**< (PIOC) Output Write Enable */
#define REG_PIOC_OWDR           (0x400E12A4) /**< (PIOC) Output Write Disable */
#define REG_PIOC_OWSR           (0x400E12A8) /**< (PIOC) Output Write Status Register */
#define REG_PIOC_AIMER          (0x400E12B0) /**< (PIOC) Additional Interrupt Modes Enable Register */
#define REG_PIOC_AIMDR          (0x400E12B4) /**< (PIOC) Additional Interrupt Modes Disable Register */
#define REG_PIOC_AIMMR          (0x400E12B8) /**< (PIOC) Additional Interrupt Modes Mask Register */
#define REG_PIOC_ESR            (0x400E12C0) /**< (PIOC) Edge Select Register */
#define REG_PIOC_LSR            (0x400E12C4) /**< (PIOC) Level Select Register */
#define REG_PIOC_ELSR           (0x400E12C8) /**< (PIOC) Edge/Level Status Register */
#define REG_PIOC_FELLSR         (0x400E12D0) /**< (PIOC) Falling Edge/Low-Level Select Register */
#define REG_PIOC_REHLSR         (0x400E12D4) /**< (PIOC) Rising Edge/High-Level Select Register */
#define REG_PIOC_FRLHSR         (0x400E12D8) /**< (PIOC) Fall/Rise - Low/High Status Register */
#define REG_PIOC_LOCKSR         (0x400E12E0) /**< (PIOC) Lock Status */
#define REG_PIOC_WPMR           (0x400E12E4) /**< (PIOC) Write Protection Mode Register */
#define REG_PIOC_WPSR           (0x400E12E8) /**< (PIOC) Write Protection Status Register */
#define REG_PIOC_SCHMITT        (0x400E1300) /**< (PIOC) Schmitt Trigger Register */
#define REG_PIOC_DRIVER         (0x400E1318) /**< (PIOC) I/O Drive Register */
#define REG_PIOC_PCMR           (0x400E1350) /**< (PIOC) Parallel Capture Mode Register */
#define REG_PIOC_PCIER          (0x400E1354) /**< (PIOC) Parallel Capture Interrupt Enable Register */
#define REG_PIOC_PCIDR          (0x400E1358) /**< (PIOC) Parallel Capture Interrupt Disable Register */
#define REG_PIOC_PCIMR          (0x400E135C) /**< (PIOC) Parallel Capture Interrupt Mask Register */
#define REG_PIOC_PCISR          (0x400E1360) /**< (PIOC) Parallel Capture Interrupt Status Register */
#define REG_PIOC_PCRHR          (0x400E1364) /**< (PIOC) Parallel Capture Reception Holding Register */

#else

#define REG_PIOC_PER            (*(__O  uint32_t*)0x400E1200U) /**< (PIOC) PIO Enable Register */
#define REG_PIOC_PDR            (*(__O  uint32_t*)0x400E1204U) /**< (PIOC) PIO Disable Register */
#define REG_PIOC_PSR            (*(__I  uint32_t*)0x400E1208U) /**< (PIOC) PIO Status Register */
#define REG_PIOC_OER            (*(__O  uint32_t*)0x400E1210U) /**< (PIOC) Output Enable Register */
#define REG_PIOC_ODR            (*(__O  uint32_t*)0x400E1214U) /**< (PIOC) Output Disable Register */
#define REG_PIOC_OSR            (*(__I  uint32_t*)0x400E1218U) /**< (PIOC) Output Status Register */
#define REG_PIOC_IFER           (*(__O  uint32_t*)0x400E1220U) /**< (PIOC) Glitch Input Filter Enable Register */
#define REG_PIOC_IFDR           (*(__O  uint32_t*)0x400E1224U) /**< (PIOC) Glitch Input Filter Disable Register */
#define REG_PIOC_IFSR           (*(__I  uint32_t*)0x400E1228U) /**< (PIOC) Glitch Input Filter Status Register */
#define REG_PIOC_SODR           (*(__O  uint32_t*)0x400E1230U) /**< (PIOC) Set Output Data Register */
#define REG_PIOC_CODR           (*(__O  uint32_t*)0x400E1234U) /**< (PIOC) Clear Output Data Register */
#define REG_PIOC_ODSR           (*(__IO uint32_t*)0x400E1238U) /**< (PIOC) Output Data Status Register */
#define REG_PIOC_PDSR           (*(__I  uint32_t*)0x400E123CU) /**< (PIOC) Pin Data Status Register */
#define REG_PIOC_IER            (*(__O  uint32_t*)0x400E1240U) /**< (PIOC) Interrupt Enable Register */
#define REG_PIOC_IDR            (*(__O  uint32_t*)0x400E1244U) /**< (PIOC) Interrupt Disable Register */
#define REG_PIOC_IMR            (*(__I  uint32_t*)0x400E1248U) /**< (PIOC) Interrupt Mask Register */
#define REG_PIOC_ISR            (*(__I  uint32_t*)0x400E124CU) /**< (PIOC) Interrupt Status Register */
#define REG_PIOC_MDER           (*(__O  uint32_t*)0x400E1250U) /**< (PIOC) Multi-driver Enable Register */
#define REG_PIOC_MDDR           (*(__O  uint32_t*)0x400E1254U) /**< (PIOC) Multi-driver Disable Register */
#define REG_PIOC_MDSR           (*(__I  uint32_t*)0x400E1258U) /**< (PIOC) Multi-driver Status Register */
#define REG_PIOC_PUDR           (*(__O  uint32_t*)0x400E1260U) /**< (PIOC) Pull-up Disable Register */
#define REG_PIOC_PUER           (*(__O  uint32_t*)0x400E1264U) /**< (PIOC) Pull-up Enable Register */
#define REG_PIOC_PUSR           (*(__I  uint32_t*)0x400E1268U) /**< (PIOC) Pad Pull-up Status Register */
#define REG_PIOC_ABCDSR         (*(__IO uint32_t*)0x400E1270U) /**< (PIOC) Peripheral Select Register 0 */
#define REG_PIOC_ABCDSR0        (*(__IO uint32_t*)0x400E1270U) /**< (PIOC) Peripheral Select Register 0 */
#define REG_PIOC_ABCDSR1        (*(__IO uint32_t*)0x400E1274U) /**< (PIOC) Peripheral Select Register 1 */
#define REG_PIOC_IFSCDR         (*(__O  uint32_t*)0x400E1280U) /**< (PIOC) Input Filter Slow Clock Disable Register */
#define REG_PIOC_IFSCER         (*(__O  uint32_t*)0x400E1284U) /**< (PIOC) Input Filter Slow Clock Enable Register */
#define REG_PIOC_IFSCSR         (*(__I  uint32_t*)0x400E1288U) /**< (PIOC) Input Filter Slow Clock Status Register */
#define REG_PIOC_SCDR           (*(__IO uint32_t*)0x400E128CU) /**< (PIOC) Slow Clock Divider Debouncing Register */
#define REG_PIOC_PPDDR          (*(__O  uint32_t*)0x400E1290U) /**< (PIOC) Pad Pull-down Disable Register */
#define REG_PIOC_PPDER          (*(__O  uint32_t*)0x400E1294U) /**< (PIOC) Pad Pull-down Enable Register */
#define REG_PIOC_PPDSR          (*(__I  uint32_t*)0x400E1298U) /**< (PIOC) Pad Pull-down Status Register */
#define REG_PIOC_OWER           (*(__O  uint32_t*)0x400E12A0U) /**< (PIOC) Output Write Enable */
#define REG_PIOC_OWDR           (*(__O  uint32_t*)0x400E12A4U) /**< (PIOC) Output Write Disable */
#define REG_PIOC_OWSR           (*(__I  uint32_t*)0x400E12A8U) /**< (PIOC) Output Write Status Register */
#define REG_PIOC_AIMER          (*(__O  uint32_t*)0x400E12B0U) /**< (PIOC) Additional Interrupt Modes Enable Register */
#define REG_PIOC_AIMDR          (*(__O  uint32_t*)0x400E12B4U) /**< (PIOC) Additional Interrupt Modes Disable Register */
#define REG_PIOC_AIMMR          (*(__I  uint32_t*)0x400E12B8U) /**< (PIOC) Additional Interrupt Modes Mask Register */
#define REG_PIOC_ESR            (*(__O  uint32_t*)0x400E12C0U) /**< (PIOC) Edge Select Register */
#define REG_PIOC_LSR            (*(__O  uint32_t*)0x400E12C4U) /**< (PIOC) Level Select Register */
#define REG_PIOC_ELSR           (*(__I  uint32_t*)0x400E12C8U) /**< (PIOC) Edge/Level Status Register */
#define REG_PIOC_FELLSR         (*(__O  uint32_t*)0x400E12D0U) /**< (PIOC) Falling Edge/Low-Level Select Register */
#define REG_PIOC_REHLSR         (*(__O  uint32_t*)0x400E12D4U) /**< (PIOC) Rising Edge/High-Level Select Register */
#define REG_PIOC_FRLHSR         (*(__I  uint32_t*)0x400E12D8U) /**< (PIOC) Fall/Rise - Low/High Status Register */
#define REG_PIOC_LOCKSR         (*(__I  uint32_t*)0x400E12E0U) /**< (PIOC) Lock Status */
#define REG_PIOC_WPMR           (*(__IO uint32_t*)0x400E12E4U) /**< (PIOC) Write Protection Mode Register */
#define REG_PIOC_WPSR           (*(__I  uint32_t*)0x400E12E8U) /**< (PIOC) Write Protection Status Register */
#define REG_PIOC_SCHMITT        (*(__IO uint32_t*)0x400E1300U) /**< (PIOC) Schmitt Trigger Register */
#define REG_PIOC_DRIVER         (*(__IO uint32_t*)0x400E1318U) /**< (PIOC) I/O Drive Register */
#define REG_PIOC_PCMR           (*(__IO uint32_t*)0x400E1350U) /**< (PIOC) Parallel Capture Mode Register */
#define REG_PIOC_PCIER          (*(__O  uint32_t*)0x400E1354U) /**< (PIOC) Parallel Capture Interrupt Enable Register */
#define REG_PIOC_PCIDR          (*(__O  uint32_t*)0x400E1358U) /**< (PIOC) Parallel Capture Interrupt Disable Register */
#define REG_PIOC_PCIMR          (*(__I  uint32_t*)0x400E135CU) /**< (PIOC) Parallel Capture Interrupt Mask Register */
#define REG_PIOC_PCISR          (*(__I  uint32_t*)0x400E1360U) /**< (PIOC) Parallel Capture Interrupt Status Register */
#define REG_PIOC_PCRHR          (*(__I  uint32_t*)0x400E1364U) /**< (PIOC) Parallel Capture Reception Holding Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PIOC peripheral ========== */
#define PIOC_INSTANCE_ID                         12        

#endif /* _SAME70_PIOC_INSTANCE_ */
