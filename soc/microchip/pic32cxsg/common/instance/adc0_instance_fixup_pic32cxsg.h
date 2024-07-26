/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_ADC0_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_ADC0_INSTANCE_FIXUP_H_

/* ========== Register definition for ADC0 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_ADC0_CTRLA             (0x43001C00) /**< \brief (ADC0) Control A */
#define REG_ADC0_EVCTRL            (0x43001C02) /**< \brief (ADC0) Event Control */
#define REG_ADC0_DBGCTRL           (0x43001C03) /**< \brief (ADC0) Debug Control */
#define REG_ADC0_INPUTCTRL         (0x43001C04) /**< \brief (ADC0) Input Control */
#define REG_ADC0_CTRLB             (0x43001C06) /**< \brief (ADC0) Control B */
#define REG_ADC0_REFCTRL           (0x43001C08) /**< \brief (ADC0) Reference Control */
#define REG_ADC0_AVGCTRL           (0x43001C0A) /**< \brief (ADC0) Average Control */
#define REG_ADC0_SAMPCTRL          (0x43001C0B) /**< \brief (ADC0) Sample Time Control */
#define REG_ADC0_WINLT             (0x43001C0C) /**< \brief (ADC0) Window Monitor Lower Threshold */
#define REG_ADC0_WINUT             (0x43001C0E) /**< \brief (ADC0) Window Monitor Upper Threshold */
#define REG_ADC0_GAINCORR          (0x43001C10) /**< \brief (ADC0) Gain Correction */
#define REG_ADC0_OFFSETCORR        (0x43001C12) /**< \brief (ADC0) Offset Correction */
#define REG_ADC0_SWTRIG            (0x43001C14) /**< \brief (ADC0) Software Trigger */
#define REG_ADC0_INTENCLR          (0x43001C2C) /**< \brief (ADC0) Interrupt Enable Clear */
#define REG_ADC0_INTENSET          (0x43001C2D) /**< \brief (ADC0) Interrupt Enable Set */
#define REG_ADC0_INTFLAG           (0x43001C2E) /**< \brief (ADC0) Interrupt Flag Status and Clear */
#define REG_ADC0_STATUS            (0x43001C2F) /**< \brief (ADC0) Status */
#define REG_ADC0_SYNCBUSY          (0x43001C30) /**< \brief (ADC0) Synchronization Busy */
#define REG_ADC0_DSEQDATA          (0x43001C34) /**< \brief (ADC0) DMA Sequencial Data */
#define REG_ADC0_DSEQCTRL          (0x43001C38) /**< \brief (ADC0) DMA Sequential Control */
#define REG_ADC0_DSEQSTAT          (0x43001C3C) /**< \brief (ADC0) DMA Sequencial Status */
#define REG_ADC0_RESULT            (0x43001C40) /**< \brief (ADC0) Result Conversion Value */
#define REG_ADC0_RESS              (0x43001C44) /**< \brief (ADC0) Last Sample Result */
#define REG_ADC0_CALIB             (0x43001C48) /**< \brief (ADC0) Calibration */
#else
#define REG_ADC0_CTRLA             (*(RwReg16*)0x43001C00UL) /**< \brief (ADC0) Control A */
#define REG_ADC0_EVCTRL            (*(RwReg8 *)0x43001C02UL) /**< \brief (ADC0) Event Control */
#define REG_ADC0_DBGCTRL           (*(RwReg8 *)0x43001C03UL) /**< \brief (ADC0) Debug Control */
#define REG_ADC0_INPUTCTRL         (*(RwReg16*)0x43001C04UL) /**< \brief (ADC0) Input Control */
#define REG_ADC0_CTRLB             (*(RwReg16*)0x43001C06UL) /**< \brief (ADC0) Control B */
#define REG_ADC0_REFCTRL           (*(RwReg8 *)0x43001C08UL) /**< \brief (ADC0) Reference Control */
#define REG_ADC0_AVGCTRL           (*(RwReg8 *)0x43001C0AUL) /**< \brief (ADC0) Average Control */
#define REG_ADC0_SAMPCTRL          (*(RwReg8 *)0x43001C0BUL) /**< \brief (ADC0) Sample Time Control */
#define REG_ADC0_WINLT             (*(RwReg16*)0x43001C0CUL) /**< \brief (ADC0) Window Monitor Lower Threshold */
#define REG_ADC0_WINUT             (*(RwReg16*)0x43001C0EUL) /**< \brief (ADC0) Window Monitor Upper Threshold */
#define REG_ADC0_GAINCORR          (*(RwReg16*)0x43001C10UL) /**< \brief (ADC0) Gain Correction */
#define REG_ADC0_OFFSETCORR        (*(RwReg16*)0x43001C12UL) /**< \brief (ADC0) Offset Correction */
#define REG_ADC0_SWTRIG            (*(RwReg8 *)0x43001C14UL) /**< \brief (ADC0) Software Trigger */
#define REG_ADC0_INTENCLR          (*(RwReg8 *)0x43001C2CUL) /**< \brief (ADC0) Interrupt Enable Clear */
#define REG_ADC0_INTENSET          (*(RwReg8 *)0x43001C2DUL) /**< \brief (ADC0) Interrupt Enable Set */
#define REG_ADC0_INTFLAG           (*(RwReg8 *)0x43001C2EUL) /**< \brief (ADC0) Interrupt Flag Status and Clear */
#define REG_ADC0_STATUS            (*(RoReg8 *)0x43001C2FUL) /**< \brief (ADC0) Status */
#define REG_ADC0_SYNCBUSY          (*(RoReg  *)0x43001C30UL) /**< \brief (ADC0) Synchronization Busy */
#define REG_ADC0_DSEQDATA          (*(WoReg  *)0x43001C34UL) /**< \brief (ADC0) DMA Sequencial Data */
#define REG_ADC0_DSEQCTRL          (*(RwReg  *)0x43001C38UL) /**< \brief (ADC0) DMA Sequential Control */
#define REG_ADC0_DSEQSTAT          (*(RoReg  *)0x43001C3CUL) /**< \brief (ADC0) DMA Sequencial Status */
#define REG_ADC0_RESULT            (*(RoReg16*)0x43001C40UL) /**< \brief (ADC0) Result Conversion Value */
#define REG_ADC0_RESS              (*(RoReg16*)0x43001C44UL) /**< \brief (ADC0) Last Sample Result */
#define REG_ADC0_CALIB             (*(RwReg16*)0x43001C48UL) /**< \brief (ADC0) Calibration */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_ADC0_INSTANCE_FIXUP_H_ */
