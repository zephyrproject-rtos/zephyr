/**
 * \file
 *
 * \brief Instance description for PIOE
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

#ifndef _SAME70_PIOE_INSTANCE_H_
#define _SAME70_PIOE_INSTANCE_H_

/* ========== Register definition for PIOE peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_PIOE_PER            (0x400E1600) /**< (PIOE) PIO Enable Register */
#define REG_PIOE_PDR            (0x400E1604) /**< (PIOE) PIO Disable Register */
#define REG_PIOE_PSR            (0x400E1608) /**< (PIOE) PIO Status Register */
#define REG_PIOE_OER            (0x400E1610) /**< (PIOE) Output Enable Register */
#define REG_PIOE_ODR            (0x400E1614) /**< (PIOE) Output Disable Register */
#define REG_PIOE_OSR            (0x400E1618) /**< (PIOE) Output Status Register */
#define REG_PIOE_IFER           (0x400E1620) /**< (PIOE) Glitch Input Filter Enable Register */
#define REG_PIOE_IFDR           (0x400E1624) /**< (PIOE) Glitch Input Filter Disable Register */
#define REG_PIOE_IFSR           (0x400E1628) /**< (PIOE) Glitch Input Filter Status Register */
#define REG_PIOE_SODR           (0x400E1630) /**< (PIOE) Set Output Data Register */
#define REG_PIOE_CODR           (0x400E1634) /**< (PIOE) Clear Output Data Register */
#define REG_PIOE_ODSR           (0x400E1638) /**< (PIOE) Output Data Status Register */
#define REG_PIOE_PDSR           (0x400E163C) /**< (PIOE) Pin Data Status Register */
#define REG_PIOE_IER            (0x400E1640) /**< (PIOE) Interrupt Enable Register */
#define REG_PIOE_IDR            (0x400E1644) /**< (PIOE) Interrupt Disable Register */
#define REG_PIOE_IMR            (0x400E1648) /**< (PIOE) Interrupt Mask Register */
#define REG_PIOE_ISR            (0x400E164C) /**< (PIOE) Interrupt Status Register */
#define REG_PIOE_MDER           (0x400E1650) /**< (PIOE) Multi-driver Enable Register */
#define REG_PIOE_MDDR           (0x400E1654) /**< (PIOE) Multi-driver Disable Register */
#define REG_PIOE_MDSR           (0x400E1658) /**< (PIOE) Multi-driver Status Register */
#define REG_PIOE_PUDR           (0x400E1660) /**< (PIOE) Pull-up Disable Register */
#define REG_PIOE_PUER           (0x400E1664) /**< (PIOE) Pull-up Enable Register */
#define REG_PIOE_PUSR           (0x400E1668) /**< (PIOE) Pad Pull-up Status Register */
#define REG_PIOE_ABCDSR         (0x400E1670) /**< (PIOE) Peripheral Select Register 0 */
#define REG_PIOE_ABCDSR0        (0x400E1670) /**< (PIOE) Peripheral Select Register 0 */
#define REG_PIOE_ABCDSR1        (0x400E1674) /**< (PIOE) Peripheral Select Register 1 */
#define REG_PIOE_IFSCDR         (0x400E1680) /**< (PIOE) Input Filter Slow Clock Disable Register */
#define REG_PIOE_IFSCER         (0x400E1684) /**< (PIOE) Input Filter Slow Clock Enable Register */
#define REG_PIOE_IFSCSR         (0x400E1688) /**< (PIOE) Input Filter Slow Clock Status Register */
#define REG_PIOE_SCDR           (0x400E168C) /**< (PIOE) Slow Clock Divider Debouncing Register */
#define REG_PIOE_PPDDR          (0x400E1690) /**< (PIOE) Pad Pull-down Disable Register */
#define REG_PIOE_PPDER          (0x400E1694) /**< (PIOE) Pad Pull-down Enable Register */
#define REG_PIOE_PPDSR          (0x400E1698) /**< (PIOE) Pad Pull-down Status Register */
#define REG_PIOE_OWER           (0x400E16A0) /**< (PIOE) Output Write Enable */
#define REG_PIOE_OWDR           (0x400E16A4) /**< (PIOE) Output Write Disable */
#define REG_PIOE_OWSR           (0x400E16A8) /**< (PIOE) Output Write Status Register */
#define REG_PIOE_AIMER          (0x400E16B0) /**< (PIOE) Additional Interrupt Modes Enable Register */
#define REG_PIOE_AIMDR          (0x400E16B4) /**< (PIOE) Additional Interrupt Modes Disable Register */
#define REG_PIOE_AIMMR          (0x400E16B8) /**< (PIOE) Additional Interrupt Modes Mask Register */
#define REG_PIOE_ESR            (0x400E16C0) /**< (PIOE) Edge Select Register */
#define REG_PIOE_LSR            (0x400E16C4) /**< (PIOE) Level Select Register */
#define REG_PIOE_ELSR           (0x400E16C8) /**< (PIOE) Edge/Level Status Register */
#define REG_PIOE_FELLSR         (0x400E16D0) /**< (PIOE) Falling Edge/Low-Level Select Register */
#define REG_PIOE_REHLSR         (0x400E16D4) /**< (PIOE) Rising Edge/High-Level Select Register */
#define REG_PIOE_FRLHSR         (0x400E16D8) /**< (PIOE) Fall/Rise - Low/High Status Register */
#define REG_PIOE_LOCKSR         (0x400E16E0) /**< (PIOE) Lock Status */
#define REG_PIOE_WPMR           (0x400E16E4) /**< (PIOE) Write Protection Mode Register */
#define REG_PIOE_WPSR           (0x400E16E8) /**< (PIOE) Write Protection Status Register */
#define REG_PIOE_SCHMITT        (0x400E1700) /**< (PIOE) Schmitt Trigger Register */
#define REG_PIOE_DRIVER         (0x400E1718) /**< (PIOE) I/O Drive Register */
#define REG_PIOE_PCMR           (0x400E1750) /**< (PIOE) Parallel Capture Mode Register */
#define REG_PIOE_PCIER          (0x400E1754) /**< (PIOE) Parallel Capture Interrupt Enable Register */
#define REG_PIOE_PCIDR          (0x400E1758) /**< (PIOE) Parallel Capture Interrupt Disable Register */
#define REG_PIOE_PCIMR          (0x400E175C) /**< (PIOE) Parallel Capture Interrupt Mask Register */
#define REG_PIOE_PCISR          (0x400E1760) /**< (PIOE) Parallel Capture Interrupt Status Register */
#define REG_PIOE_PCRHR          (0x400E1764) /**< (PIOE) Parallel Capture Reception Holding Register */

#else

#define REG_PIOE_PER            (*(__O  uint32_t*)0x400E1600U) /**< (PIOE) PIO Enable Register */
#define REG_PIOE_PDR            (*(__O  uint32_t*)0x400E1604U) /**< (PIOE) PIO Disable Register */
#define REG_PIOE_PSR            (*(__I  uint32_t*)0x400E1608U) /**< (PIOE) PIO Status Register */
#define REG_PIOE_OER            (*(__O  uint32_t*)0x400E1610U) /**< (PIOE) Output Enable Register */
#define REG_PIOE_ODR            (*(__O  uint32_t*)0x400E1614U) /**< (PIOE) Output Disable Register */
#define REG_PIOE_OSR            (*(__I  uint32_t*)0x400E1618U) /**< (PIOE) Output Status Register */
#define REG_PIOE_IFER           (*(__O  uint32_t*)0x400E1620U) /**< (PIOE) Glitch Input Filter Enable Register */
#define REG_PIOE_IFDR           (*(__O  uint32_t*)0x400E1624U) /**< (PIOE) Glitch Input Filter Disable Register */
#define REG_PIOE_IFSR           (*(__I  uint32_t*)0x400E1628U) /**< (PIOE) Glitch Input Filter Status Register */
#define REG_PIOE_SODR           (*(__O  uint32_t*)0x400E1630U) /**< (PIOE) Set Output Data Register */
#define REG_PIOE_CODR           (*(__O  uint32_t*)0x400E1634U) /**< (PIOE) Clear Output Data Register */
#define REG_PIOE_ODSR           (*(__IO uint32_t*)0x400E1638U) /**< (PIOE) Output Data Status Register */
#define REG_PIOE_PDSR           (*(__I  uint32_t*)0x400E163CU) /**< (PIOE) Pin Data Status Register */
#define REG_PIOE_IER            (*(__O  uint32_t*)0x400E1640U) /**< (PIOE) Interrupt Enable Register */
#define REG_PIOE_IDR            (*(__O  uint32_t*)0x400E1644U) /**< (PIOE) Interrupt Disable Register */
#define REG_PIOE_IMR            (*(__I  uint32_t*)0x400E1648U) /**< (PIOE) Interrupt Mask Register */
#define REG_PIOE_ISR            (*(__I  uint32_t*)0x400E164CU) /**< (PIOE) Interrupt Status Register */
#define REG_PIOE_MDER           (*(__O  uint32_t*)0x400E1650U) /**< (PIOE) Multi-driver Enable Register */
#define REG_PIOE_MDDR           (*(__O  uint32_t*)0x400E1654U) /**< (PIOE) Multi-driver Disable Register */
#define REG_PIOE_MDSR           (*(__I  uint32_t*)0x400E1658U) /**< (PIOE) Multi-driver Status Register */
#define REG_PIOE_PUDR           (*(__O  uint32_t*)0x400E1660U) /**< (PIOE) Pull-up Disable Register */
#define REG_PIOE_PUER           (*(__O  uint32_t*)0x400E1664U) /**< (PIOE) Pull-up Enable Register */
#define REG_PIOE_PUSR           (*(__I  uint32_t*)0x400E1668U) /**< (PIOE) Pad Pull-up Status Register */
#define REG_PIOE_ABCDSR         (*(__IO uint32_t*)0x400E1670U) /**< (PIOE) Peripheral Select Register 0 */
#define REG_PIOE_ABCDSR0        (*(__IO uint32_t*)0x400E1670U) /**< (PIOE) Peripheral Select Register 0 */
#define REG_PIOE_ABCDSR1        (*(__IO uint32_t*)0x400E1674U) /**< (PIOE) Peripheral Select Register 1 */
#define REG_PIOE_IFSCDR         (*(__O  uint32_t*)0x400E1680U) /**< (PIOE) Input Filter Slow Clock Disable Register */
#define REG_PIOE_IFSCER         (*(__O  uint32_t*)0x400E1684U) /**< (PIOE) Input Filter Slow Clock Enable Register */
#define REG_PIOE_IFSCSR         (*(__I  uint32_t*)0x400E1688U) /**< (PIOE) Input Filter Slow Clock Status Register */
#define REG_PIOE_SCDR           (*(__IO uint32_t*)0x400E168CU) /**< (PIOE) Slow Clock Divider Debouncing Register */
#define REG_PIOE_PPDDR          (*(__O  uint32_t*)0x400E1690U) /**< (PIOE) Pad Pull-down Disable Register */
#define REG_PIOE_PPDER          (*(__O  uint32_t*)0x400E1694U) /**< (PIOE) Pad Pull-down Enable Register */
#define REG_PIOE_PPDSR          (*(__I  uint32_t*)0x400E1698U) /**< (PIOE) Pad Pull-down Status Register */
#define REG_PIOE_OWER           (*(__O  uint32_t*)0x400E16A0U) /**< (PIOE) Output Write Enable */
#define REG_PIOE_OWDR           (*(__O  uint32_t*)0x400E16A4U) /**< (PIOE) Output Write Disable */
#define REG_PIOE_OWSR           (*(__I  uint32_t*)0x400E16A8U) /**< (PIOE) Output Write Status Register */
#define REG_PIOE_AIMER          (*(__O  uint32_t*)0x400E16B0U) /**< (PIOE) Additional Interrupt Modes Enable Register */
#define REG_PIOE_AIMDR          (*(__O  uint32_t*)0x400E16B4U) /**< (PIOE) Additional Interrupt Modes Disable Register */
#define REG_PIOE_AIMMR          (*(__I  uint32_t*)0x400E16B8U) /**< (PIOE) Additional Interrupt Modes Mask Register */
#define REG_PIOE_ESR            (*(__O  uint32_t*)0x400E16C0U) /**< (PIOE) Edge Select Register */
#define REG_PIOE_LSR            (*(__O  uint32_t*)0x400E16C4U) /**< (PIOE) Level Select Register */
#define REG_PIOE_ELSR           (*(__I  uint32_t*)0x400E16C8U) /**< (PIOE) Edge/Level Status Register */
#define REG_PIOE_FELLSR         (*(__O  uint32_t*)0x400E16D0U) /**< (PIOE) Falling Edge/Low-Level Select Register */
#define REG_PIOE_REHLSR         (*(__O  uint32_t*)0x400E16D4U) /**< (PIOE) Rising Edge/High-Level Select Register */
#define REG_PIOE_FRLHSR         (*(__I  uint32_t*)0x400E16D8U) /**< (PIOE) Fall/Rise - Low/High Status Register */
#define REG_PIOE_LOCKSR         (*(__I  uint32_t*)0x400E16E0U) /**< (PIOE) Lock Status */
#define REG_PIOE_WPMR           (*(__IO uint32_t*)0x400E16E4U) /**< (PIOE) Write Protection Mode Register */
#define REG_PIOE_WPSR           (*(__I  uint32_t*)0x400E16E8U) /**< (PIOE) Write Protection Status Register */
#define REG_PIOE_SCHMITT        (*(__IO uint32_t*)0x400E1700U) /**< (PIOE) Schmitt Trigger Register */
#define REG_PIOE_DRIVER         (*(__IO uint32_t*)0x400E1718U) /**< (PIOE) I/O Drive Register */
#define REG_PIOE_PCMR           (*(__IO uint32_t*)0x400E1750U) /**< (PIOE) Parallel Capture Mode Register */
#define REG_PIOE_PCIER          (*(__O  uint32_t*)0x400E1754U) /**< (PIOE) Parallel Capture Interrupt Enable Register */
#define REG_PIOE_PCIDR          (*(__O  uint32_t*)0x400E1758U) /**< (PIOE) Parallel Capture Interrupt Disable Register */
#define REG_PIOE_PCIMR          (*(__I  uint32_t*)0x400E175CU) /**< (PIOE) Parallel Capture Interrupt Mask Register */
#define REG_PIOE_PCISR          (*(__I  uint32_t*)0x400E1760U) /**< (PIOE) Parallel Capture Interrupt Status Register */
#define REG_PIOE_PCRHR          (*(__I  uint32_t*)0x400E1764U) /**< (PIOE) Parallel Capture Reception Holding Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for PIOE peripheral ========== */
#define PIOE_INSTANCE_ID                         17        

#endif /* _SAME70_PIOE_INSTANCE_ */
