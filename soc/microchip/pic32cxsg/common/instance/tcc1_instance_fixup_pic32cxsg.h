/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_TCC1_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_TCC1_INSTANCE_FIXUP_H_

/* ========== Register definition for TCC1 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_TCC1_CTRLA             (0x41018000) /**< \brief (TCC1) Control A */
#define REG_TCC1_CTRLBCLR          (0x41018004) /**< \brief (TCC1) Control B Clear */
#define REG_TCC1_CTRLBSET          (0x41018005) /**< \brief (TCC1) Control B Set */
#define REG_TCC1_SYNCBUSY          (0x41018008) /**< \brief (TCC1) Synchronization Busy */
#define REG_TCC1_FCTRLA            (0x4101800C) /**< \brief (TCC1) Recoverable Fault A Configuration */
#define REG_TCC1_FCTRLB            (0x41018010) /**< \brief (TCC1) Recoverable Fault B Configuration */
#define REG_TCC1_WEXCTRL           (0x41018014) /**< \brief (TCC1) Waveform Extension Configuration */
#define REG_TCC1_DRVCTRL           (0x41018018) /**< \brief (TCC1) Driver Control */
#define REG_TCC1_DBGCTRL           (0x4101801E) /**< \brief (TCC1) Debug Control */
#define REG_TCC1_EVCTRL            (0x41018020) /**< \brief (TCC1) Event Control */
#define REG_TCC1_INTENCLR          (0x41018024) /**< \brief (TCC1) Interrupt Enable Clear */
#define REG_TCC1_INTENSET          (0x41018028) /**< \brief (TCC1) Interrupt Enable Set */
#define REG_TCC1_INTFLAG           (0x4101802C) /**< \brief (TCC1) Interrupt Flag Status and Clear */
#define REG_TCC1_STATUS            (0x41018030) /**< \brief (TCC1) Status */
#define REG_TCC1_COUNT             (0x41018034) /**< \brief (TCC1) Count */
#define REG_TCC1_PATT              (0x41018038) /**< \brief (TCC1) Pattern */
#define REG_TCC1_WAVE              (0x4101803C) /**< \brief (TCC1) Waveform Control */
#define REG_TCC1_PER               (0x41018040) /**< \brief (TCC1) Period */
#define REG_TCC1_CC0               (0x41018044) /**< \brief (TCC1) Compare and Capture 0 */
#define REG_TCC1_CC1               (0x41018048) /**< \brief (TCC1) Compare and Capture 1 */
#define REG_TCC1_CC2               (0x4101804C) /**< \brief (TCC1) Compare and Capture 2 */
#define REG_TCC1_CC3               (0x41018050) /**< \brief (TCC1) Compare and Capture 3 */
#define REG_TCC1_PATTBUF           (0x41018064) /**< \brief (TCC1) Pattern Buffer */
#define REG_TCC1_PERBUF            (0x4101806C) /**< \brief (TCC1) Period Buffer */
#define REG_TCC1_CCBUF0            (0x41018070) /**< \brief (TCC1) Compare and Capture Buffer 0 */
#define REG_TCC1_CCBUF1            (0x41018074) /**< \brief (TCC1) Compare and Capture Buffer 1 */
#define REG_TCC1_CCBUF2            (0x41018078) /**< \brief (TCC1) Compare and Capture Buffer 2 */
#define REG_TCC1_CCBUF3            (0x4101807C) /**< \brief (TCC1) Compare and Capture Buffer 3 */
#else
#define REG_TCC1_CTRLA             (*(RwReg  *)0x41018000UL) /**< \brief (TCC1) Control A */
#define REG_TCC1_CTRLBCLR          (*(RwReg8 *)0x41018004UL) /**< \brief (TCC1) Control B Clear */
#define REG_TCC1_CTRLBSET          (*(RwReg8 *)0x41018005UL) /**< \brief (TCC1) Control B Set */
#define REG_TCC1_SYNCBUSY          (*(RoReg  *)0x41018008UL) /**< \brief (TCC1) Synchronization Busy */
#define REG_TCC1_FCTRLA            (*(RwReg  *)0x4101800CUL) /**< \brief (TCC1) Recoverable Fault A Configuration */
#define REG_TCC1_FCTRLB            (*(RwReg  *)0x41018010UL) /**< \brief (TCC1) Recoverable Fault B Configuration */
#define REG_TCC1_WEXCTRL           (*(RwReg  *)0x41018014UL) /**< \brief (TCC1) Waveform Extension Configuration */
#define REG_TCC1_DRVCTRL           (*(RwReg  *)0x41018018UL) /**< \brief (TCC1) Driver Control */
#define REG_TCC1_DBGCTRL           (*(RwReg8 *)0x4101801EUL) /**< \brief (TCC1) Debug Control */
#define REG_TCC1_EVCTRL            (*(RwReg  *)0x41018020UL) /**< \brief (TCC1) Event Control */
#define REG_TCC1_INTENCLR          (*(RwReg  *)0x41018024UL) /**< \brief (TCC1) Interrupt Enable Clear */
#define REG_TCC1_INTENSET          (*(RwReg  *)0x41018028UL) /**< \brief (TCC1) Interrupt Enable Set */
#define REG_TCC1_INTFLAG           (*(RwReg  *)0x4101802CUL) /**< \brief (TCC1) Interrupt Flag Status and Clear */
#define REG_TCC1_STATUS            (*(RwReg  *)0x41018030UL) /**< \brief (TCC1) Status */
#define REG_TCC1_COUNT             (*(RwReg  *)0x41018034UL) /**< \brief (TCC1) Count */
#define REG_TCC1_PATT              (*(RwReg16*)0x41018038UL) /**< \brief (TCC1) Pattern */
#define REG_TCC1_WAVE              (*(RwReg  *)0x4101803CUL) /**< \brief (TCC1) Waveform Control */
#define REG_TCC1_PER               (*(RwReg  *)0x41018040UL) /**< \brief (TCC1) Period */
#define REG_TCC1_CC0               (*(RwReg  *)0x41018044UL) /**< \brief (TCC1) Compare and Capture 0 */
#define REG_TCC1_CC1               (*(RwReg  *)0x41018048UL) /**< \brief (TCC1) Compare and Capture 1 */
#define REG_TCC1_CC2               (*(RwReg  *)0x4101804CUL) /**< \brief (TCC1) Compare and Capture 2 */
#define REG_TCC1_CC3               (*(RwReg  *)0x41018050UL) /**< \brief (TCC1) Compare and Capture 3 */
#define REG_TCC1_PATTBUF           (*(RwReg16*)0x41018064UL) /**< \brief (TCC1) Pattern Buffer */
#define REG_TCC1_PERBUF            (*(RwReg  *)0x4101806CUL) /**< \brief (TCC1) Period Buffer */
#define REG_TCC1_CCBUF0            (*(RwReg  *)0x41018070UL) /**< \brief (TCC1) Compare and Capture Buffer 0 */
#define REG_TCC1_CCBUF1            (*(RwReg  *)0x41018074UL) /**< \brief (TCC1) Compare and Capture Buffer 1 */
#define REG_TCC1_CCBUF2            (*(RwReg  *)0x41018078UL) /**< \brief (TCC1) Compare and Capture Buffer 2 */
#define REG_TCC1_CCBUF3            (*(RwReg  *)0x4101807CUL) /**< \brief (TCC1) Compare and Capture Buffer 3 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_TCC1_INSTANCE_FIXUP_H_ */
