/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_DAC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_DAC_INSTANCE_FIXUP_H_

/* ========== Register definition for DAC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_DAC_CTRLA              (0x43002400) /**< \brief (DAC) Control A */
#define REG_DAC_CTRLB              (0x43002401) /**< \brief (DAC) Control B */
#define REG_DAC_EVCTRL             (0x43002402) /**< \brief (DAC) Event Control */
#define REG_DAC_INTENCLR           (0x43002404) /**< \brief (DAC) Interrupt Enable Clear */
#define REG_DAC_INTENSET           (0x43002405) /**< \brief (DAC) Interrupt Enable Set */
#define REG_DAC_INTFLAG            (0x43002406) /**< \brief (DAC) Interrupt Flag Status and Clear */
#define REG_DAC_STATUS             (0x43002407) /**< \brief (DAC) Status */
#define REG_DAC_SYNCBUSY           (0x43002408) /**< \brief (DAC) Synchronization Busy */
#define REG_DAC_DACCTRL0           (0x4300240C) /**< \brief (DAC) DAC 0 Control */
#define REG_DAC_DACCTRL1           (0x4300240E) /**< \brief (DAC) DAC 1 Control */
#define REG_DAC_DATA0              (0x43002410) /**< \brief (DAC) DAC 0 Data */
#define REG_DAC_DATA1              (0x43002412) /**< \brief (DAC) DAC 1 Data */
#define REG_DAC_DATABUF0           (0x43002414) /**< \brief (DAC) DAC 0 Data Buffer */
#define REG_DAC_DATABUF1           (0x43002416) /**< \brief (DAC) DAC 1 Data Buffer */
#define REG_DAC_DBGCTRL            (0x43002418) /**< \brief (DAC) Debug Control */
#define REG_DAC_RESULT0            (0x4300241C) /**< \brief (DAC) Filter Result 0 */
#define REG_DAC_RESULT1            (0x4300241E) /**< \brief (DAC) Filter Result 1 */
#else
#define REG_DAC_CTRLA              (*(RwReg8 *)0x43002400UL) /**< \brief (DAC) Control A */
#define REG_DAC_CTRLB              (*(RwReg8 *)0x43002401UL) /**< \brief (DAC) Control B */
#define REG_DAC_EVCTRL             (*(RwReg8 *)0x43002402UL) /**< \brief (DAC) Event Control */
#define REG_DAC_INTENCLR           (*(RwReg8 *)0x43002404UL) /**< \brief (DAC) Interrupt Enable Clear */
#define REG_DAC_INTENSET           (*(RwReg8 *)0x43002405UL) /**< \brief (DAC) Interrupt Enable Set */
#define REG_DAC_INTFLAG            (*(RwReg8 *)0x43002406UL) /**< \brief (DAC) Interrupt Flag Status and Clear */
#define REG_DAC_STATUS             (*(RoReg8 *)0x43002407UL) /**< \brief (DAC) Status */
#define REG_DAC_SYNCBUSY           (*(RoReg  *)0x43002408UL) /**< \brief (DAC) Synchronization Busy */
#define REG_DAC_DACCTRL0           (*(RwReg16*)0x4300240CUL) /**< \brief (DAC) DAC 0 Control */
#define REG_DAC_DACCTRL1           (*(RwReg16*)0x4300240EUL) /**< \brief (DAC) DAC 1 Control */
#define REG_DAC_DATA0              (*(WoReg16*)0x43002410UL) /**< \brief (DAC) DAC 0 Data */
#define REG_DAC_DATA1              (*(WoReg16*)0x43002412UL) /**< \brief (DAC) DAC 1 Data */
#define REG_DAC_DATABUF0           (*(WoReg16*)0x43002414UL) /**< \brief (DAC) DAC 0 Data Buffer */
#define REG_DAC_DATABUF1           (*(WoReg16*)0x43002416UL) /**< \brief (DAC) DAC 1 Data Buffer */
#define REG_DAC_DBGCTRL            (*(RwReg8 *)0x43002418UL) /**< \brief (DAC) Debug Control */
#define REG_DAC_RESULT0            (*(RoReg16*)0x4300241CUL) /**< \brief (DAC) Filter Result 0 */
#define REG_DAC_RESULT1            (*(RoReg16*)0x4300241EUL) /**< \brief (DAC) Filter Result 1 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_DAC_INSTANCE_FIXUP_H_ */
