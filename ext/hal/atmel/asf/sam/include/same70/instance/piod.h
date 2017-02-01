/**
 * \file
 *
 * \brief Instance description for PIOD
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

#ifndef _SAME70_PIOD_INSTANCE_H_
#define _SAME70_PIOD_INSTANCE_H_

/* ========== Register definition for PIOD peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PIOD_PER            (0x400E1400) /**< (PIOD) PIO Enable Register */
#define REG_PIOD_PDR            (0x400E1404) /**< (PIOD) PIO Disable Register */
#define REG_PIOD_PSR            (0x400E1408) /**< (PIOD) PIO Status Register */
#define REG_PIOD_OER            (0x400E1410) /**< (PIOD) Output Enable Register */
#define REG_PIOD_ODR            (0x400E1414) /**< (PIOD) Output Disable Register */
#define REG_PIOD_OSR            (0x400E1418) /**< (PIOD) Output Status Register */
#define REG_PIOD_IFER           (0x400E1420) /**< (PIOD) Glitch Input Filter Enable Register */
#define REG_PIOD_IFDR           (0x400E1424) /**< (PIOD) Glitch Input Filter Disable Register */
#define REG_PIOD_IFSR           (0x400E1428) /**< (PIOD) Glitch Input Filter Status Register */
#define REG_PIOD_SODR           (0x400E1430) /**< (PIOD) Set Output Data Register */
#define REG_PIOD_CODR           (0x400E1434) /**< (PIOD) Clear Output Data Register */
#define REG_PIOD_ODSR           (0x400E1438) /**< (PIOD) Output Data Status Register */
#define REG_PIOD_PDSR           (0x400E143C) /**< (PIOD) Pin Data Status Register */
#define REG_PIOD_IER            (0x400E1440) /**< (PIOD) Interrupt Enable Register */
#define REG_PIOD_IDR            (0x400E1444) /**< (PIOD) Interrupt Disable Register */
#define REG_PIOD_IMR            (0x400E1448) /**< (PIOD) Interrupt Mask Register */
#define REG_PIOD_ISR            (0x400E144C) /**< (PIOD) Interrupt Status Register */
#define REG_PIOD_MDER           (0x400E1450) /**< (PIOD) Multi-driver Enable Register */
#define REG_PIOD_MDDR           (0x400E1454) /**< (PIOD) Multi-driver Disable Register */
#define REG_PIOD_MDSR           (0x400E1458) /**< (PIOD) Multi-driver Status Register */
#define REG_PIOD_PUDR           (0x400E1460) /**< (PIOD) Pull-up Disable Register */
#define REG_PIOD_PUER           (0x400E1464) /**< (PIOD) Pull-up Enable Register */
#define REG_PIOD_PUSR           (0x400E1468) /**< (PIOD) Pad Pull-up Status Register */
#define REG_PIOD_ABCDSR         (0x400E1470) /**< (PIOD) Peripheral Select Register 0 */
#define REG_PIOD_ABCDSR0        (0x400E1470) /**< (PIOD) Peripheral Select Register 0 */
#define REG_PIOD_ABCDSR1        (0x400E1474) /**< (PIOD) Peripheral Select Register 1 */
#define REG_PIOD_IFSCDR         (0x400E1480) /**< (PIOD) Input Filter Slow Clock Disable Register */
#define REG_PIOD_IFSCER         (0x400E1484) /**< (PIOD) Input Filter Slow Clock Enable Register */
#define REG_PIOD_IFSCSR         (0x400E1488) /**< (PIOD) Input Filter Slow Clock Status Register */
#define REG_PIOD_SCDR           (0x400E148C) /**< (PIOD) Slow Clock Divider Debouncing Register */
#define REG_PIOD_PPDDR          (0x400E1490) /**< (PIOD) Pad Pull-down Disable Register */
#define REG_PIOD_PPDER          (0x400E1494) /**< (PIOD) Pad Pull-down Enable Register */
#define REG_PIOD_PPDSR          (0x400E1498) /**< (PIOD) Pad Pull-down Status Register */
#define REG_PIOD_OWER           (0x400E14A0) /**< (PIOD) Output Write Enable */
#define REG_PIOD_OWDR           (0x400E14A4) /**< (PIOD) Output Write Disable */
#define REG_PIOD_OWSR           (0x400E14A8) /**< (PIOD) Output Write Status Register */
#define REG_PIOD_AIMER          (0x400E14B0) /**< (PIOD) Additional Interrupt Modes Enable Register */
#define REG_PIOD_AIMDR          (0x400E14B4) /**< (PIOD) Additional Interrupt Modes Disable Register */
#define REG_PIOD_AIMMR          (0x400E14B8) /**< (PIOD) Additional Interrupt Modes Mask Register */
#define REG_PIOD_ESR            (0x400E14C0) /**< (PIOD) Edge Select Register */
#define REG_PIOD_LSR            (0x400E14C4) /**< (PIOD) Level Select Register */
#define REG_PIOD_ELSR           (0x400E14C8) /**< (PIOD) Edge/Level Status Register */
#define REG_PIOD_FELLSR         (0x400E14D0) /**< (PIOD) Falling Edge/Low-Level Select Register */
#define REG_PIOD_REHLSR         (0x400E14D4) /**< (PIOD) Rising Edge/High-Level Select Register */
#define REG_PIOD_FRLHSR         (0x400E14D8) /**< (PIOD) Fall/Rise - Low/High Status Register */
#define REG_PIOD_LOCKSR         (0x400E14E0) /**< (PIOD) Lock Status */
#define REG_PIOD_WPMR           (0x400E14E4) /**< (PIOD) Write Protection Mode Register */
#define REG_PIOD_WPSR           (0x400E14E8) /**< (PIOD) Write Protection Status Register */
#define REG_PIOD_SCHMITT        (0x400E1500) /**< (PIOD) Schmitt Trigger Register */
#define REG_PIOD_DRIVER         (0x400E1518) /**< (PIOD) I/O Drive Register */
#define REG_PIOD_PCMR           (0x400E1550) /**< (PIOD) Parallel Capture Mode Register */
#define REG_PIOD_PCIER          (0x400E1554) /**< (PIOD) Parallel Capture Interrupt Enable Register */
#define REG_PIOD_PCIDR          (0x400E1558) /**< (PIOD) Parallel Capture Interrupt Disable Register */
#define REG_PIOD_PCIMR          (0x400E155C) /**< (PIOD) Parallel Capture Interrupt Mask Register */
#define REG_PIOD_PCISR          (0x400E1560) /**< (PIOD) Parallel Capture Interrupt Status Register */
#define REG_PIOD_PCRHR          (0x400E1564) /**< (PIOD) Parallel Capture Reception Holding Register */

#else

#define REG_PIOD_PER            (*(__O  uint32_t*)0x400E1400U) /**< (PIOD) PIO Enable Register */
#define REG_PIOD_PDR            (*(__O  uint32_t*)0x400E1404U) /**< (PIOD) PIO Disable Register */
#define REG_PIOD_PSR            (*(__I  uint32_t*)0x400E1408U) /**< (PIOD) PIO Status Register */
#define REG_PIOD_OER            (*(__O  uint32_t*)0x400E1410U) /**< (PIOD) Output Enable Register */
#define REG_PIOD_ODR            (*(__O  uint32_t*)0x400E1414U) /**< (PIOD) Output Disable Register */
#define REG_PIOD_OSR            (*(__I  uint32_t*)0x400E1418U) /**< (PIOD) Output Status Register */
#define REG_PIOD_IFER           (*(__O  uint32_t*)0x400E1420U) /**< (PIOD) Glitch Input Filter Enable Register */
#define REG_PIOD_IFDR           (*(__O  uint32_t*)0x400E1424U) /**< (PIOD) Glitch Input Filter Disable Register */
#define REG_PIOD_IFSR           (*(__I  uint32_t*)0x400E1428U) /**< (PIOD) Glitch Input Filter Status Register */
#define REG_PIOD_SODR           (*(__O  uint32_t*)0x400E1430U) /**< (PIOD) Set Output Data Register */
#define REG_PIOD_CODR           (*(__O  uint32_t*)0x400E1434U) /**< (PIOD) Clear Output Data Register */
#define REG_PIOD_ODSR           (*(__IO uint32_t*)0x400E1438U) /**< (PIOD) Output Data Status Register */
#define REG_PIOD_PDSR           (*(__I  uint32_t*)0x400E143CU) /**< (PIOD) Pin Data Status Register */
#define REG_PIOD_IER            (*(__O  uint32_t*)0x400E1440U) /**< (PIOD) Interrupt Enable Register */
#define REG_PIOD_IDR            (*(__O  uint32_t*)0x400E1444U) /**< (PIOD) Interrupt Disable Register */
#define REG_PIOD_IMR            (*(__I  uint32_t*)0x400E1448U) /**< (PIOD) Interrupt Mask Register */
#define REG_PIOD_ISR            (*(__I  uint32_t*)0x400E144CU) /**< (PIOD) Interrupt Status Register */
#define REG_PIOD_MDER           (*(__O  uint32_t*)0x400E1450U) /**< (PIOD) Multi-driver Enable Register */
#define REG_PIOD_MDDR           (*(__O  uint32_t*)0x400E1454U) /**< (PIOD) Multi-driver Disable Register */
#define REG_PIOD_MDSR           (*(__I  uint32_t*)0x400E1458U) /**< (PIOD) Multi-driver Status Register */
#define REG_PIOD_PUDR           (*(__O  uint32_t*)0x400E1460U) /**< (PIOD) Pull-up Disable Register */
#define REG_PIOD_PUER           (*(__O  uint32_t*)0x400E1464U) /**< (PIOD) Pull-up Enable Register */
#define REG_PIOD_PUSR           (*(__I  uint32_t*)0x400E1468U) /**< (PIOD) Pad Pull-up Status Register */
#define REG_PIOD_ABCDSR         (*(__IO uint32_t*)0x400E1470U) /**< (PIOD) Peripheral Select Register 0 */
#define REG_PIOD_ABCDSR0        (*(__IO uint32_t*)0x400E1470U) /**< (PIOD) Peripheral Select Register 0 */
#define REG_PIOD_ABCDSR1        (*(__IO uint32_t*)0x400E1474U) /**< (PIOD) Peripheral Select Register 1 */
#define REG_PIOD_IFSCDR         (*(__O  uint32_t*)0x400E1480U) /**< (PIOD) Input Filter Slow Clock Disable Register */
#define REG_PIOD_IFSCER         (*(__O  uint32_t*)0x400E1484U) /**< (PIOD) Input Filter Slow Clock Enable Register */
#define REG_PIOD_IFSCSR         (*(__I  uint32_t*)0x400E1488U) /**< (PIOD) Input Filter Slow Clock Status Register */
#define REG_PIOD_SCDR           (*(__IO uint32_t*)0x400E148CU) /**< (PIOD) Slow Clock Divider Debouncing Register */
#define REG_PIOD_PPDDR          (*(__O  uint32_t*)0x400E1490U) /**< (PIOD) Pad Pull-down Disable Register */
#define REG_PIOD_PPDER          (*(__O  uint32_t*)0x400E1494U) /**< (PIOD) Pad Pull-down Enable Register */
#define REG_PIOD_PPDSR          (*(__I  uint32_t*)0x400E1498U) /**< (PIOD) Pad Pull-down Status Register */
#define REG_PIOD_OWER           (*(__O  uint32_t*)0x400E14A0U) /**< (PIOD) Output Write Enable */
#define REG_PIOD_OWDR           (*(__O  uint32_t*)0x400E14A4U) /**< (PIOD) Output Write Disable */
#define REG_PIOD_OWSR           (*(__I  uint32_t*)0x400E14A8U) /**< (PIOD) Output Write Status Register */
#define REG_PIOD_AIMER          (*(__O  uint32_t*)0x400E14B0U) /**< (PIOD) Additional Interrupt Modes Enable Register */
#define REG_PIOD_AIMDR          (*(__O  uint32_t*)0x400E14B4U) /**< (PIOD) Additional Interrupt Modes Disable Register */
#define REG_PIOD_AIMMR          (*(__I  uint32_t*)0x400E14B8U) /**< (PIOD) Additional Interrupt Modes Mask Register */
#define REG_PIOD_ESR            (*(__O  uint32_t*)0x400E14C0U) /**< (PIOD) Edge Select Register */
#define REG_PIOD_LSR            (*(__O  uint32_t*)0x400E14C4U) /**< (PIOD) Level Select Register */
#define REG_PIOD_ELSR           (*(__I  uint32_t*)0x400E14C8U) /**< (PIOD) Edge/Level Status Register */
#define REG_PIOD_FELLSR         (*(__O  uint32_t*)0x400E14D0U) /**< (PIOD) Falling Edge/Low-Level Select Register */
#define REG_PIOD_REHLSR         (*(__O  uint32_t*)0x400E14D4U) /**< (PIOD) Rising Edge/High-Level Select Register */
#define REG_PIOD_FRLHSR         (*(__I  uint32_t*)0x400E14D8U) /**< (PIOD) Fall/Rise - Low/High Status Register */
#define REG_PIOD_LOCKSR         (*(__I  uint32_t*)0x400E14E0U) /**< (PIOD) Lock Status */
#define REG_PIOD_WPMR           (*(__IO uint32_t*)0x400E14E4U) /**< (PIOD) Write Protection Mode Register */
#define REG_PIOD_WPSR           (*(__I  uint32_t*)0x400E14E8U) /**< (PIOD) Write Protection Status Register */
#define REG_PIOD_SCHMITT        (*(__IO uint32_t*)0x400E1500U) /**< (PIOD) Schmitt Trigger Register */
#define REG_PIOD_DRIVER         (*(__IO uint32_t*)0x400E1518U) /**< (PIOD) I/O Drive Register */
#define REG_PIOD_PCMR           (*(__IO uint32_t*)0x400E1550U) /**< (PIOD) Parallel Capture Mode Register */
#define REG_PIOD_PCIER          (*(__O  uint32_t*)0x400E1554U) /**< (PIOD) Parallel Capture Interrupt Enable Register */
#define REG_PIOD_PCIDR          (*(__O  uint32_t*)0x400E1558U) /**< (PIOD) Parallel Capture Interrupt Disable Register */
#define REG_PIOD_PCIMR          (*(__I  uint32_t*)0x400E155CU) /**< (PIOD) Parallel Capture Interrupt Mask Register */
#define REG_PIOD_PCISR          (*(__I  uint32_t*)0x400E1560U) /**< (PIOD) Parallel Capture Interrupt Status Register */
#define REG_PIOD_PCRHR          (*(__I  uint32_t*)0x400E1564U) /**< (PIOD) Parallel Capture Reception Holding Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PIOD peripheral ========== */
#define PIOD_INSTANCE_ID                         16        

#endif /* _SAME70_PIOD_INSTANCE_ */
