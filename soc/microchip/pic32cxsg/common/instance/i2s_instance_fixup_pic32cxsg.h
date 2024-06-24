/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_I2S_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_I2S_INSTANCE_FIXUP_H_

/* ========== Register definition for I2S peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_I2S_CTRLA              (0x43002800) /**< \brief (I2S) Control A */
#define REG_I2S_CLKCTRL0           (0x43002804) /**< \brief (I2S) Clock Unit 0 Control */
#define REG_I2S_CLKCTRL1           (0x43002808) /**< \brief (I2S) Clock Unit 1 Control */
#define REG_I2S_INTENCLR           (0x4300280C) /**< \brief (I2S) Interrupt Enable Clear */
#define REG_I2S_INTENSET           (0x43002810) /**< \brief (I2S) Interrupt Enable Set */
#define REG_I2S_INTFLAG            (0x43002814) /**< \brief (I2S) Interrupt Flag Status and Clear */
#define REG_I2S_SYNCBUSY           (0x43002818) /**< \brief (I2S) Synchronization Status */
#define REG_I2S_TXCTRL             (0x43002820) /**< \brief (I2S) Tx Serializer Control */
#define REG_I2S_RXCTRL             (0x43002824) /**< \brief (I2S) Rx Serializer Control */
#define REG_I2S_TXDATA             (0x43002830) /**< \brief (I2S) Tx Data */
#define REG_I2S_RXDATA             (0x43002834) /**< \brief (I2S) Rx Data */
#else
#define REG_I2S_CTRLA              (*(RwReg8 *)0x43002800UL) /**< \brief (I2S) Control A */
#define REG_I2S_CLKCTRL0           (*(RwReg  *)0x43002804UL) /**< \brief (I2S) Clock Unit 0 Control */
#define REG_I2S_CLKCTRL1           (*(RwReg  *)0x43002808UL) /**< \brief (I2S) Clock Unit 1 Control */
#define REG_I2S_INTENCLR           (*(RwReg16*)0x4300280CUL) /**< \brief (I2S) Interrupt Enable Clear */
#define REG_I2S_INTENSET           (*(RwReg16*)0x43002810UL) /**< \brief (I2S) Interrupt Enable Set */
#define REG_I2S_INTFLAG            (*(RwReg16*)0x43002814UL) /**< \brief (I2S) Interrupt Flag Status and Clear */
#define REG_I2S_SYNCBUSY           (*(RoReg16*)0x43002818UL) /**< \brief (I2S) Synchronization Status */
#define REG_I2S_TXCTRL             (*(RwReg  *)0x43002820UL) /**< \brief (I2S) Tx Serializer Control */
#define REG_I2S_RXCTRL             (*(RwReg  *)0x43002824UL) /**< \brief (I2S) Rx Serializer Control */
#define REG_I2S_TXDATA             (*(WoReg  *)0x43002830UL) /**< \brief (I2S) Tx Data */
#define REG_I2S_RXDATA             (*(RoReg  *)0x43002834UL) /**< \brief (I2S) Rx Data */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_I2S_INSTANCE_FIXUP_H_ */
