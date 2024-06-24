/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_TC3_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_TC3_INSTANCE_FIXUP_H_

/* ========== Register definition for TC3 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_TC3_CTRLA              (0x4101C000) /**< \brief (TC3) Control A */
#define REG_TC3_CTRLBCLR           (0x4101C004) /**< \brief (TC3) Control B Clear */
#define REG_TC3_CTRLBSET           (0x4101C005) /**< \brief (TC3) Control B Set */
#define REG_TC3_EVCTRL             (0x4101C006) /**< \brief (TC3) Event Control */
#define REG_TC3_INTENCLR           (0x4101C008) /**< \brief (TC3) Interrupt Enable Clear */
#define REG_TC3_INTENSET           (0x4101C009) /**< \brief (TC3) Interrupt Enable Set */
#define REG_TC3_INTFLAG            (0x4101C00A) /**< \brief (TC3) Interrupt Flag Status and Clear */
#define REG_TC3_STATUS             (0x4101C00B) /**< \brief (TC3) Status */
#define REG_TC3_WAVE               (0x4101C00C) /**< \brief (TC3) Waveform Generation Control */
#define REG_TC3_DRVCTRL            (0x4101C00D) /**< \brief (TC3) Control C */
#define REG_TC3_DBGCTRL            (0x4101C00F) /**< \brief (TC3) Debug Control */
#define REG_TC3_SYNCBUSY           (0x4101C010) /**< \brief (TC3) Synchronization Status */
#define REG_TC3_COUNT16_COUNT      (0x4101C014) /**< \brief (TC3) COUNT16 Count */
#define REG_TC3_COUNT16_CC0        (0x4101C01C) /**< \brief (TC3) COUNT16 Compare and Capture 0 */
#define REG_TC3_COUNT16_CC1        (0x4101C01E) /**< \brief (TC3) COUNT16 Compare and Capture 1 */
#define REG_TC3_COUNT16_CCBUF0     (0x4101C030) /**< \brief (TC3) COUNT16 Compare and Capture Buffer 0 */
#define REG_TC3_COUNT16_CCBUF1     (0x4101C032) /**< \brief (TC3) COUNT16 Compare and Capture Buffer 1 */
#define REG_TC3_COUNT32_COUNT      (0x4101C014) /**< \brief (TC3) COUNT32 Count */
#define REG_TC3_COUNT32_CC0        (0x4101C01C) /**< \brief (TC3) COUNT32 Compare and Capture 0 */
#define REG_TC3_COUNT32_CC1        (0x4101C020) /**< \brief (TC3) COUNT32 Compare and Capture 1 */
#define REG_TC3_COUNT32_CCBUF0     (0x4101C030) /**< \brief (TC3) COUNT32 Compare and Capture Buffer 0 */
#define REG_TC3_COUNT32_CCBUF1     (0x4101C034) /**< \brief (TC3) COUNT32 Compare and Capture Buffer 1 */
#define REG_TC3_COUNT8_COUNT       (0x4101C014) /**< \brief (TC3) COUNT8 Count */
#define REG_TC3_COUNT8_PER         (0x4101C01B) /**< \brief (TC3) COUNT8 Period */
#define REG_TC3_COUNT8_CC0         (0x4101C01C) /**< \brief (TC3) COUNT8 Compare and Capture 0 */
#define REG_TC3_COUNT8_CC1         (0x4101C01D) /**< \brief (TC3) COUNT8 Compare and Capture 1 */
#define REG_TC3_COUNT8_PERBUF      (0x4101C02F) /**< \brief (TC3) COUNT8 Period Buffer */
#define REG_TC3_COUNT8_CCBUF0      (0x4101C030) /**< \brief (TC3) COUNT8 Compare and Capture Buffer 0 */
#define REG_TC3_COUNT8_CCBUF1      (0x4101C031) /**< \brief (TC3) COUNT8 Compare and Capture Buffer 1 */
#else
#define REG_TC3_CTRLA              (*(RwReg  *)0x4101C000UL) /**< \brief (TC3) Control A */
#define REG_TC3_CTRLBCLR           (*(RwReg8 *)0x4101C004UL) /**< \brief (TC3) Control B Clear */
#define REG_TC3_CTRLBSET           (*(RwReg8 *)0x4101C005UL) /**< \brief (TC3) Control B Set */
#define REG_TC3_EVCTRL             (*(RwReg16*)0x4101C006UL) /**< \brief (TC3) Event Control */
#define REG_TC3_INTENCLR           (*(RwReg8 *)0x4101C008UL) /**< \brief (TC3) Interrupt Enable Clear */
#define REG_TC3_INTENSET           (*(RwReg8 *)0x4101C009UL) /**< \brief (TC3) Interrupt Enable Set */
#define REG_TC3_INTFLAG            (*(RwReg8 *)0x4101C00AUL) /**< \brief (TC3) Interrupt Flag Status and Clear */
#define REG_TC3_STATUS             (*(RwReg8 *)0x4101C00BUL) /**< \brief (TC3) Status */
#define REG_TC3_WAVE               (*(RwReg8 *)0x4101C00CUL) /**< \brief (TC3) Waveform Generation Control */
#define REG_TC3_DRVCTRL            (*(RwReg8 *)0x4101C00DUL) /**< \brief (TC3) Control C */
#define REG_TC3_DBGCTRL            (*(RwReg8 *)0x4101C00FUL) /**< \brief (TC3) Debug Control */
#define REG_TC3_SYNCBUSY           (*(RoReg  *)0x4101C010UL) /**< \brief (TC3) Synchronization Status */
#define REG_TC3_COUNT16_COUNT      (*(RwReg16*)0x4101C014UL) /**< \brief (TC3) COUNT16 Count */
#define REG_TC3_COUNT16_CC0        (*(RwReg16*)0x4101C01CUL) /**< \brief (TC3) COUNT16 Compare and Capture 0 */
#define REG_TC3_COUNT16_CC1        (*(RwReg16*)0x4101C01EUL) /**< \brief (TC3) COUNT16 Compare and Capture 1 */
#define REG_TC3_COUNT16_CCBUF0     (*(RwReg16*)0x4101C030UL) /**< \brief (TC3) COUNT16 Compare and Capture Buffer 0 */
#define REG_TC3_COUNT16_CCBUF1     (*(RwReg16*)0x4101C032UL) /**< \brief (TC3) COUNT16 Compare and Capture Buffer 1 */
#define REG_TC3_COUNT32_COUNT      (*(RwReg  *)0x4101C014UL) /**< \brief (TC3) COUNT32 Count */
#define REG_TC3_COUNT32_CC0        (*(RwReg  *)0x4101C01CUL) /**< \brief (TC3) COUNT32 Compare and Capture 0 */
#define REG_TC3_COUNT32_CC1        (*(RwReg  *)0x4101C020UL) /**< \brief (TC3) COUNT32 Compare and Capture 1 */
#define REG_TC3_COUNT32_CCBUF0     (*(RwReg  *)0x4101C030UL) /**< \brief (TC3) COUNT32 Compare and Capture Buffer 0 */
#define REG_TC3_COUNT32_CCBUF1     (*(RwReg  *)0x4101C034UL) /**< \brief (TC3) COUNT32 Compare and Capture Buffer 1 */
#define REG_TC3_COUNT8_COUNT       (*(RwReg8 *)0x4101C014UL) /**< \brief (TC3) COUNT8 Count */
#define REG_TC3_COUNT8_PER         (*(RwReg8 *)0x4101C01BUL) /**< \brief (TC3) COUNT8 Period */
#define REG_TC3_COUNT8_CC0         (*(RwReg8 *)0x4101C01CUL) /**< \brief (TC3) COUNT8 Compare and Capture 0 */
#define REG_TC3_COUNT8_CC1         (*(RwReg8 *)0x4101C01DUL) /**< \brief (TC3) COUNT8 Compare and Capture 1 */
#define REG_TC3_COUNT8_PERBUF      (*(RwReg8 *)0x4101C02FUL) /**< \brief (TC3) COUNT8 Period Buffer */
#define REG_TC3_COUNT8_CCBUF0      (*(RwReg8 *)0x4101C030UL) /**< \brief (TC3) COUNT8 Compare and Capture Buffer 0 */
#define REG_TC3_COUNT8_CCBUF1      (*(RwReg8 *)0x4101C031UL) /**< \brief (TC3) COUNT8 Compare and Capture Buffer 1 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_TC3_INSTANCE_FIXUP_H_ */
