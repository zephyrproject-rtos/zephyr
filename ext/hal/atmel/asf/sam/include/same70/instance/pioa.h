/**
 * \file
 *
 * \brief Instance description for PIOA
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

#ifndef _SAME70_PIOA_INSTANCE_H_
#define _SAME70_PIOA_INSTANCE_H_

/* ========== Register definition for PIOA peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PIOA_PER            (0x400E0E00) /**< (PIOA) PIO Enable Register */
#define REG_PIOA_PDR            (0x400E0E04) /**< (PIOA) PIO Disable Register */
#define REG_PIOA_PSR            (0x400E0E08) /**< (PIOA) PIO Status Register */
#define REG_PIOA_OER            (0x400E0E10) /**< (PIOA) Output Enable Register */
#define REG_PIOA_ODR            (0x400E0E14) /**< (PIOA) Output Disable Register */
#define REG_PIOA_OSR            (0x400E0E18) /**< (PIOA) Output Status Register */
#define REG_PIOA_IFER           (0x400E0E20) /**< (PIOA) Glitch Input Filter Enable Register */
#define REG_PIOA_IFDR           (0x400E0E24) /**< (PIOA) Glitch Input Filter Disable Register */
#define REG_PIOA_IFSR           (0x400E0E28) /**< (PIOA) Glitch Input Filter Status Register */
#define REG_PIOA_SODR           (0x400E0E30) /**< (PIOA) Set Output Data Register */
#define REG_PIOA_CODR           (0x400E0E34) /**< (PIOA) Clear Output Data Register */
#define REG_PIOA_ODSR           (0x400E0E38) /**< (PIOA) Output Data Status Register */
#define REG_PIOA_PDSR           (0x400E0E3C) /**< (PIOA) Pin Data Status Register */
#define REG_PIOA_IER            (0x400E0E40) /**< (PIOA) Interrupt Enable Register */
#define REG_PIOA_IDR            (0x400E0E44) /**< (PIOA) Interrupt Disable Register */
#define REG_PIOA_IMR            (0x400E0E48) /**< (PIOA) Interrupt Mask Register */
#define REG_PIOA_ISR            (0x400E0E4C) /**< (PIOA) Interrupt Status Register */
#define REG_PIOA_MDER           (0x400E0E50) /**< (PIOA) Multi-driver Enable Register */
#define REG_PIOA_MDDR           (0x400E0E54) /**< (PIOA) Multi-driver Disable Register */
#define REG_PIOA_MDSR           (0x400E0E58) /**< (PIOA) Multi-driver Status Register */
#define REG_PIOA_PUDR           (0x400E0E60) /**< (PIOA) Pull-up Disable Register */
#define REG_PIOA_PUER           (0x400E0E64) /**< (PIOA) Pull-up Enable Register */
#define REG_PIOA_PUSR           (0x400E0E68) /**< (PIOA) Pad Pull-up Status Register */
#define REG_PIOA_ABCDSR         (0x400E0E70) /**< (PIOA) Peripheral Select Register 0 */
#define REG_PIOA_ABCDSR0        (0x400E0E70) /**< (PIOA) Peripheral Select Register 0 */
#define REG_PIOA_ABCDSR1        (0x400E0E74) /**< (PIOA) Peripheral Select Register 1 */
#define REG_PIOA_IFSCDR         (0x400E0E80) /**< (PIOA) Input Filter Slow Clock Disable Register */
#define REG_PIOA_IFSCER         (0x400E0E84) /**< (PIOA) Input Filter Slow Clock Enable Register */
#define REG_PIOA_IFSCSR         (0x400E0E88) /**< (PIOA) Input Filter Slow Clock Status Register */
#define REG_PIOA_SCDR           (0x400E0E8C) /**< (PIOA) Slow Clock Divider Debouncing Register */
#define REG_PIOA_PPDDR          (0x400E0E90) /**< (PIOA) Pad Pull-down Disable Register */
#define REG_PIOA_PPDER          (0x400E0E94) /**< (PIOA) Pad Pull-down Enable Register */
#define REG_PIOA_PPDSR          (0x400E0E98) /**< (PIOA) Pad Pull-down Status Register */
#define REG_PIOA_OWER           (0x400E0EA0) /**< (PIOA) Output Write Enable */
#define REG_PIOA_OWDR           (0x400E0EA4) /**< (PIOA) Output Write Disable */
#define REG_PIOA_OWSR           (0x400E0EA8) /**< (PIOA) Output Write Status Register */
#define REG_PIOA_AIMER          (0x400E0EB0) /**< (PIOA) Additional Interrupt Modes Enable Register */
#define REG_PIOA_AIMDR          (0x400E0EB4) /**< (PIOA) Additional Interrupt Modes Disable Register */
#define REG_PIOA_AIMMR          (0x400E0EB8) /**< (PIOA) Additional Interrupt Modes Mask Register */
#define REG_PIOA_ESR            (0x400E0EC0) /**< (PIOA) Edge Select Register */
#define REG_PIOA_LSR            (0x400E0EC4) /**< (PIOA) Level Select Register */
#define REG_PIOA_ELSR           (0x400E0EC8) /**< (PIOA) Edge/Level Status Register */
#define REG_PIOA_FELLSR         (0x400E0ED0) /**< (PIOA) Falling Edge/Low-Level Select Register */
#define REG_PIOA_REHLSR         (0x400E0ED4) /**< (PIOA) Rising Edge/High-Level Select Register */
#define REG_PIOA_FRLHSR         (0x400E0ED8) /**< (PIOA) Fall/Rise - Low/High Status Register */
#define REG_PIOA_LOCKSR         (0x400E0EE0) /**< (PIOA) Lock Status */
#define REG_PIOA_WPMR           (0x400E0EE4) /**< (PIOA) Write Protection Mode Register */
#define REG_PIOA_WPSR           (0x400E0EE8) /**< (PIOA) Write Protection Status Register */
#define REG_PIOA_SCHMITT        (0x400E0F00) /**< (PIOA) Schmitt Trigger Register */
#define REG_PIOA_DRIVER         (0x400E0F18) /**< (PIOA) I/O Drive Register */
#define REG_PIOA_PCMR           (0x400E0F50) /**< (PIOA) Parallel Capture Mode Register */
#define REG_PIOA_PCIER          (0x400E0F54) /**< (PIOA) Parallel Capture Interrupt Enable Register */
#define REG_PIOA_PCIDR          (0x400E0F58) /**< (PIOA) Parallel Capture Interrupt Disable Register */
#define REG_PIOA_PCIMR          (0x400E0F5C) /**< (PIOA) Parallel Capture Interrupt Mask Register */
#define REG_PIOA_PCISR          (0x400E0F60) /**< (PIOA) Parallel Capture Interrupt Status Register */
#define REG_PIOA_PCRHR          (0x400E0F64) /**< (PIOA) Parallel Capture Reception Holding Register */

#else

#define REG_PIOA_PER            (*(__O  uint32_t*)0x400E0E00U) /**< (PIOA) PIO Enable Register */
#define REG_PIOA_PDR            (*(__O  uint32_t*)0x400E0E04U) /**< (PIOA) PIO Disable Register */
#define REG_PIOA_PSR            (*(__I  uint32_t*)0x400E0E08U) /**< (PIOA) PIO Status Register */
#define REG_PIOA_OER            (*(__O  uint32_t*)0x400E0E10U) /**< (PIOA) Output Enable Register */
#define REG_PIOA_ODR            (*(__O  uint32_t*)0x400E0E14U) /**< (PIOA) Output Disable Register */
#define REG_PIOA_OSR            (*(__I  uint32_t*)0x400E0E18U) /**< (PIOA) Output Status Register */
#define REG_PIOA_IFER           (*(__O  uint32_t*)0x400E0E20U) /**< (PIOA) Glitch Input Filter Enable Register */
#define REG_PIOA_IFDR           (*(__O  uint32_t*)0x400E0E24U) /**< (PIOA) Glitch Input Filter Disable Register */
#define REG_PIOA_IFSR           (*(__I  uint32_t*)0x400E0E28U) /**< (PIOA) Glitch Input Filter Status Register */
#define REG_PIOA_SODR           (*(__O  uint32_t*)0x400E0E30U) /**< (PIOA) Set Output Data Register */
#define REG_PIOA_CODR           (*(__O  uint32_t*)0x400E0E34U) /**< (PIOA) Clear Output Data Register */
#define REG_PIOA_ODSR           (*(__IO uint32_t*)0x400E0E38U) /**< (PIOA) Output Data Status Register */
#define REG_PIOA_PDSR           (*(__I  uint32_t*)0x400E0E3CU) /**< (PIOA) Pin Data Status Register */
#define REG_PIOA_IER            (*(__O  uint32_t*)0x400E0E40U) /**< (PIOA) Interrupt Enable Register */
#define REG_PIOA_IDR            (*(__O  uint32_t*)0x400E0E44U) /**< (PIOA) Interrupt Disable Register */
#define REG_PIOA_IMR            (*(__I  uint32_t*)0x400E0E48U) /**< (PIOA) Interrupt Mask Register */
#define REG_PIOA_ISR            (*(__I  uint32_t*)0x400E0E4CU) /**< (PIOA) Interrupt Status Register */
#define REG_PIOA_MDER           (*(__O  uint32_t*)0x400E0E50U) /**< (PIOA) Multi-driver Enable Register */
#define REG_PIOA_MDDR           (*(__O  uint32_t*)0x400E0E54U) /**< (PIOA) Multi-driver Disable Register */
#define REG_PIOA_MDSR           (*(__I  uint32_t*)0x400E0E58U) /**< (PIOA) Multi-driver Status Register */
#define REG_PIOA_PUDR           (*(__O  uint32_t*)0x400E0E60U) /**< (PIOA) Pull-up Disable Register */
#define REG_PIOA_PUER           (*(__O  uint32_t*)0x400E0E64U) /**< (PIOA) Pull-up Enable Register */
#define REG_PIOA_PUSR           (*(__I  uint32_t*)0x400E0E68U) /**< (PIOA) Pad Pull-up Status Register */
#define REG_PIOA_ABCDSR         (*(__IO uint32_t*)0x400E0E70U) /**< (PIOA) Peripheral Select Register 0 */
#define REG_PIOA_ABCDSR0        (*(__IO uint32_t*)0x400E0E70U) /**< (PIOA) Peripheral Select Register 0 */
#define REG_PIOA_ABCDSR1        (*(__IO uint32_t*)0x400E0E74U) /**< (PIOA) Peripheral Select Register 1 */
#define REG_PIOA_IFSCDR         (*(__O  uint32_t*)0x400E0E80U) /**< (PIOA) Input Filter Slow Clock Disable Register */
#define REG_PIOA_IFSCER         (*(__O  uint32_t*)0x400E0E84U) /**< (PIOA) Input Filter Slow Clock Enable Register */
#define REG_PIOA_IFSCSR         (*(__I  uint32_t*)0x400E0E88U) /**< (PIOA) Input Filter Slow Clock Status Register */
#define REG_PIOA_SCDR           (*(__IO uint32_t*)0x400E0E8CU) /**< (PIOA) Slow Clock Divider Debouncing Register */
#define REG_PIOA_PPDDR          (*(__O  uint32_t*)0x400E0E90U) /**< (PIOA) Pad Pull-down Disable Register */
#define REG_PIOA_PPDER          (*(__O  uint32_t*)0x400E0E94U) /**< (PIOA) Pad Pull-down Enable Register */
#define REG_PIOA_PPDSR          (*(__I  uint32_t*)0x400E0E98U) /**< (PIOA) Pad Pull-down Status Register */
#define REG_PIOA_OWER           (*(__O  uint32_t*)0x400E0EA0U) /**< (PIOA) Output Write Enable */
#define REG_PIOA_OWDR           (*(__O  uint32_t*)0x400E0EA4U) /**< (PIOA) Output Write Disable */
#define REG_PIOA_OWSR           (*(__I  uint32_t*)0x400E0EA8U) /**< (PIOA) Output Write Status Register */
#define REG_PIOA_AIMER          (*(__O  uint32_t*)0x400E0EB0U) /**< (PIOA) Additional Interrupt Modes Enable Register */
#define REG_PIOA_AIMDR          (*(__O  uint32_t*)0x400E0EB4U) /**< (PIOA) Additional Interrupt Modes Disable Register */
#define REG_PIOA_AIMMR          (*(__I  uint32_t*)0x400E0EB8U) /**< (PIOA) Additional Interrupt Modes Mask Register */
#define REG_PIOA_ESR            (*(__O  uint32_t*)0x400E0EC0U) /**< (PIOA) Edge Select Register */
#define REG_PIOA_LSR            (*(__O  uint32_t*)0x400E0EC4U) /**< (PIOA) Level Select Register */
#define REG_PIOA_ELSR           (*(__I  uint32_t*)0x400E0EC8U) /**< (PIOA) Edge/Level Status Register */
#define REG_PIOA_FELLSR         (*(__O  uint32_t*)0x400E0ED0U) /**< (PIOA) Falling Edge/Low-Level Select Register */
#define REG_PIOA_REHLSR         (*(__O  uint32_t*)0x400E0ED4U) /**< (PIOA) Rising Edge/High-Level Select Register */
#define REG_PIOA_FRLHSR         (*(__I  uint32_t*)0x400E0ED8U) /**< (PIOA) Fall/Rise - Low/High Status Register */
#define REG_PIOA_LOCKSR         (*(__I  uint32_t*)0x400E0EE0U) /**< (PIOA) Lock Status */
#define REG_PIOA_WPMR           (*(__IO uint32_t*)0x400E0EE4U) /**< (PIOA) Write Protection Mode Register */
#define REG_PIOA_WPSR           (*(__I  uint32_t*)0x400E0EE8U) /**< (PIOA) Write Protection Status Register */
#define REG_PIOA_SCHMITT        (*(__IO uint32_t*)0x400E0F00U) /**< (PIOA) Schmitt Trigger Register */
#define REG_PIOA_DRIVER         (*(__IO uint32_t*)0x400E0F18U) /**< (PIOA) I/O Drive Register */
#define REG_PIOA_PCMR           (*(__IO uint32_t*)0x400E0F50U) /**< (PIOA) Parallel Capture Mode Register */
#define REG_PIOA_PCIER          (*(__O  uint32_t*)0x400E0F54U) /**< (PIOA) Parallel Capture Interrupt Enable Register */
#define REG_PIOA_PCIDR          (*(__O  uint32_t*)0x400E0F58U) /**< (PIOA) Parallel Capture Interrupt Disable Register */
#define REG_PIOA_PCIMR          (*(__I  uint32_t*)0x400E0F5CU) /**< (PIOA) Parallel Capture Interrupt Mask Register */
#define REG_PIOA_PCISR          (*(__I  uint32_t*)0x400E0F60U) /**< (PIOA) Parallel Capture Interrupt Status Register */
#define REG_PIOA_PCRHR          (*(__I  uint32_t*)0x400E0F64U) /**< (PIOA) Parallel Capture Reception Holding Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PIOA peripheral ========== */
#define PIOA_INSTANCE_ID                         10        
#define PIOA_DMAC_ID_RX                          34        

#endif /* _SAME70_PIOA_INSTANCE_ */
