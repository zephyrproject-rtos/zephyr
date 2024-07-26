/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_AC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_AC_INSTANCE_FIXUP_H_

/* ========== Register definition for AC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_AC_CTRLA               (0x42002000) /**< \brief (AC) Control A */
#define REG_AC_CTRLB               (0x42002001) /**< \brief (AC) Control B */
#define REG_AC_EVCTRL              (0x42002002) /**< \brief (AC) Event Control */
#define REG_AC_INTENCLR            (0x42002004) /**< \brief (AC) Interrupt Enable Clear */
#define REG_AC_INTENSET            (0x42002005) /**< \brief (AC) Interrupt Enable Set */
#define REG_AC_INTFLAG             (0x42002006) /**< \brief (AC) Interrupt Flag Status and Clear */
#define REG_AC_STATUSA             (0x42002007) /**< \brief (AC) Status A */
#define REG_AC_STATUSB             (0x42002008) /**< \brief (AC) Status B */
#define REG_AC_DBGCTRL             (0x42002009) /**< \brief (AC) Debug Control */
#define REG_AC_WINCTRL             (0x4200200A) /**< \brief (AC) Window Control */
#define REG_AC_SCALER0             (0x4200200C) /**< \brief (AC) Scaler 0 */
#define REG_AC_SCALER1             (0x4200200D) /**< \brief (AC) Scaler 1 */
#define REG_AC_COMPCTRL0           (0x42002010) /**< \brief (AC) Comparator Control 0 */
#define REG_AC_COMPCTRL1           (0x42002014) /**< \brief (AC) Comparator Control 1 */
#define REG_AC_SYNCBUSY            (0x42002020) /**< \brief (AC) Synchronization Busy */
#define REG_AC_CALIB               (0x42002024) /**< \brief (AC) Calibration */
#else
#define REG_AC_CTRLA               (*(RwReg8 *)0x42002000UL) /**< \brief (AC) Control A */
#define REG_AC_CTRLB               (*(WoReg8 *)0x42002001UL) /**< \brief (AC) Control B */
#define REG_AC_EVCTRL              (*(RwReg16*)0x42002002UL) /**< \brief (AC) Event Control */
#define REG_AC_INTENCLR            (*(RwReg8 *)0x42002004UL) /**< \brief (AC) Interrupt Enable Clear */
#define REG_AC_INTENSET            (*(RwReg8 *)0x42002005UL) /**< \brief (AC) Interrupt Enable Set */
#define REG_AC_INTFLAG             (*(RwReg8 *)0x42002006UL) /**< \brief (AC) Interrupt Flag Status and Clear */
#define REG_AC_STATUSA             (*(RoReg8 *)0x42002007UL) /**< \brief (AC) Status A */
#define REG_AC_STATUSB             (*(RoReg8 *)0x42002008UL) /**< \brief (AC) Status B */
#define REG_AC_DBGCTRL             (*(RwReg8 *)0x42002009UL) /**< \brief (AC) Debug Control */
#define REG_AC_WINCTRL             (*(RwReg8 *)0x4200200AUL) /**< \brief (AC) Window Control */
#define REG_AC_SCALER0             (*(RwReg8 *)0x4200200CUL) /**< \brief (AC) Scaler 0 */
#define REG_AC_SCALER1             (*(RwReg8 *)0x4200200DUL) /**< \brief (AC) Scaler 1 */
#define REG_AC_COMPCTRL0           (*(RwReg  *)0x42002010UL) /**< \brief (AC) Comparator Control 0 */
#define REG_AC_COMPCTRL1           (*(RwReg  *)0x42002014UL) /**< \brief (AC) Comparator Control 1 */
#define REG_AC_SYNCBUSY            (*(RoReg  *)0x42002020UL) /**< \brief (AC) Synchronization Busy */
#define REG_AC_CALIB               (*(RwReg16*)0x42002024UL) /**< \brief (AC) Calibration */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_AC_INSTANCE_FIXUP_H_ */
