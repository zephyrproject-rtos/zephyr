/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_TC5_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_TC5_INSTANCE_FIXUP_H_

/* ========== Register definition for TC5 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_TC5_CTRLA              (0x42001800) /**< \brief (TC5) Control A */
#define REG_TC5_CTRLBCLR           (0x42001804) /**< \brief (TC5) Control B Clear */
#define REG_TC5_CTRLBSET           (0x42001805) /**< \brief (TC5) Control B Set */
#define REG_TC5_EVCTRL             (0x42001806) /**< \brief (TC5) Event Control */
#define REG_TC5_INTENCLR           (0x42001808) /**< \brief (TC5) Interrupt Enable Clear */
#define REG_TC5_INTENSET           (0x42001809) /**< \brief (TC5) Interrupt Enable Set */
#define REG_TC5_INTFLAG            (0x4200180A) /**< \brief (TC5) Interrupt Flag Status and Clear */
#define REG_TC5_STATUS             (0x4200180B) /**< \brief (TC5) Status */
#define REG_TC5_WAVE               (0x4200180C) /**< \brief (TC5) Waveform Generation Control */
#define REG_TC5_DRVCTRL            (0x4200180D) /**< \brief (TC5) Control C */
#define REG_TC5_DBGCTRL            (0x4200180F) /**< \brief (TC5) Debug Control */
#define REG_TC5_SYNCBUSY           (0x42001810) /**< \brief (TC5) Synchronization Status */
#define REG_TC5_COUNT16_COUNT      (0x42001814) /**< \brief (TC5) COUNT16 Count */
#define REG_TC5_COUNT16_CC0        (0x4200181C) /**< \brief (TC5) COUNT16 Compare and Capture 0 */
#define REG_TC5_COUNT16_CC1        (0x4200181E) /**< \brief (TC5) COUNT16 Compare and Capture 1 */
#define REG_TC5_COUNT16_CCBUF0     (0x42001830) /**< \brief (TC5) COUNT16 Compare and Capture Buffer 0 */
#define REG_TC5_COUNT16_CCBUF1     (0x42001832) /**< \brief (TC5) COUNT16 Compare and Capture Buffer 1 */
#define REG_TC5_COUNT32_COUNT      (0x42001814) /**< \brief (TC5) COUNT32 Count */
#define REG_TC5_COUNT32_CC0        (0x4200181C) /**< \brief (TC5) COUNT32 Compare and Capture 0 */
#define REG_TC5_COUNT32_CC1        (0x42001820) /**< \brief (TC5) COUNT32 Compare and Capture 1 */
#define REG_TC5_COUNT32_CCBUF0     (0x42001830) /**< \brief (TC5) COUNT32 Compare and Capture Buffer 0 */
#define REG_TC5_COUNT32_CCBUF1     (0x42001834) /**< \brief (TC5) COUNT32 Compare and Capture Buffer 1 */
#define REG_TC5_COUNT8_COUNT       (0x42001814) /**< \brief (TC5) COUNT8 Count */
#define REG_TC5_COUNT8_PER         (0x4200181B) /**< \brief (TC5) COUNT8 Period */
#define REG_TC5_COUNT8_CC0         (0x4200181C) /**< \brief (TC5) COUNT8 Compare and Capture 0 */
#define REG_TC5_COUNT8_CC1         (0x4200181D) /**< \brief (TC5) COUNT8 Compare and Capture 1 */
#define REG_TC5_COUNT8_PERBUF      (0x4200182F) /**< \brief (TC5) COUNT8 Period Buffer */
#define REG_TC5_COUNT8_CCBUF0      (0x42001830) /**< \brief (TC5) COUNT8 Compare and Capture Buffer 0 */
#define REG_TC5_COUNT8_CCBUF1      (0x42001831) /**< \brief (TC5) COUNT8 Compare and Capture Buffer 1 */
#else
#define REG_TC5_CTRLA              (*(RwReg  *)0x42001800UL) /**< \brief (TC5) Control A */
#define REG_TC5_CTRLBCLR           (*(RwReg8 *)0x42001804UL) /**< \brief (TC5) Control B Clear */
#define REG_TC5_CTRLBSET           (*(RwReg8 *)0x42001805UL) /**< \brief (TC5) Control B Set */
#define REG_TC5_EVCTRL             (*(RwReg16*)0x42001806UL) /**< \brief (TC5) Event Control */
#define REG_TC5_INTENCLR           (*(RwReg8 *)0x42001808UL) /**< \brief (TC5) Interrupt Enable Clear */
#define REG_TC5_INTENSET           (*(RwReg8 *)0x42001809UL) /**< \brief (TC5) Interrupt Enable Set */
#define REG_TC5_INTFLAG            (*(RwReg8 *)0x4200180AUL) /**< \brief (TC5) Interrupt Flag Status and Clear */
#define REG_TC5_STATUS             (*(RwReg8 *)0x4200180BUL) /**< \brief (TC5) Status */
#define REG_TC5_WAVE               (*(RwReg8 *)0x4200180CUL) /**< \brief (TC5) Waveform Generation Control */
#define REG_TC5_DRVCTRL            (*(RwReg8 *)0x4200180DUL) /**< \brief (TC5) Control C */
#define REG_TC5_DBGCTRL            (*(RwReg8 *)0x4200180FUL) /**< \brief (TC5) Debug Control */
#define REG_TC5_SYNCBUSY           (*(RoReg  *)0x42001810UL) /**< \brief (TC5) Synchronization Status */
#define REG_TC5_COUNT16_COUNT      (*(RwReg16*)0x42001814UL) /**< \brief (TC5) COUNT16 Count */
#define REG_TC5_COUNT16_CC0        (*(RwReg16*)0x4200181CUL) /**< \brief (TC5) COUNT16 Compare and Capture 0 */
#define REG_TC5_COUNT16_CC1        (*(RwReg16*)0x4200181EUL) /**< \brief (TC5) COUNT16 Compare and Capture 1 */
#define REG_TC5_COUNT16_CCBUF0     (*(RwReg16*)0x42001830UL) /**< \brief (TC5) COUNT16 Compare and Capture Buffer 0 */
#define REG_TC5_COUNT16_CCBUF1     (*(RwReg16*)0x42001832UL) /**< \brief (TC5) COUNT16 Compare and Capture Buffer 1 */
#define REG_TC5_COUNT32_COUNT      (*(RwReg  *)0x42001814UL) /**< \brief (TC5) COUNT32 Count */
#define REG_TC5_COUNT32_CC0        (*(RwReg  *)0x4200181CUL) /**< \brief (TC5) COUNT32 Compare and Capture 0 */
#define REG_TC5_COUNT32_CC1        (*(RwReg  *)0x42001820UL) /**< \brief (TC5) COUNT32 Compare and Capture 1 */
#define REG_TC5_COUNT32_CCBUF0     (*(RwReg  *)0x42001830UL) /**< \brief (TC5) COUNT32 Compare and Capture Buffer 0 */
#define REG_TC5_COUNT32_CCBUF1     (*(RwReg  *)0x42001834UL) /**< \brief (TC5) COUNT32 Compare and Capture Buffer 1 */
#define REG_TC5_COUNT8_COUNT       (*(RwReg8 *)0x42001814UL) /**< \brief (TC5) COUNT8 Count */
#define REG_TC5_COUNT8_PER         (*(RwReg8 *)0x4200181BUL) /**< \brief (TC5) COUNT8 Period */
#define REG_TC5_COUNT8_CC0         (*(RwReg8 *)0x4200181CUL) /**< \brief (TC5) COUNT8 Compare and Capture 0 */
#define REG_TC5_COUNT8_CC1         (*(RwReg8 *)0x4200181DUL) /**< \brief (TC5) COUNT8 Compare and Capture 1 */
#define REG_TC5_COUNT8_PERBUF      (*(RwReg8 *)0x4200182FUL) /**< \brief (TC5) COUNT8 Period Buffer */
#define REG_TC5_COUNT8_CCBUF0      (*(RwReg8 *)0x42001830UL) /**< \brief (TC5) COUNT8 Compare and Capture Buffer 0 */
#define REG_TC5_COUNT8_CCBUF1      (*(RwReg8 *)0x42001831UL) /**< \brief (TC5) COUNT8 Compare and Capture Buffer 1 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_TC5_INSTANCE_FIXUP_H_ */
