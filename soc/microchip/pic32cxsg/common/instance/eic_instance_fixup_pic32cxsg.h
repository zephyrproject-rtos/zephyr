/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_EIC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_EIC_INSTANCE_FIXUP_H_

/* ========== Register definition for EIC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_EIC_CTRLA              (0x40002800) /**< \brief (EIC) Control A */
#define REG_EIC_NMICTRL            (0x40002801) /**< \brief (EIC) Non-Maskable Interrupt Control */
#define REG_EIC_NMIFLAG            (0x40002802) /**< \brief (EIC) Non-Maskable Interrupt Flag Status and Clear */
#define REG_EIC_SYNCBUSY           (0x40002804) /**< \brief (EIC) Synchronization Busy */
#define REG_EIC_EVCTRL             (0x40002808) /**< \brief (EIC) Event Control */
#define REG_EIC_INTENCLR           (0x4000280C) /**< \brief (EIC) Interrupt Enable Clear */
#define REG_EIC_INTENSET           (0x40002810) /**< \brief (EIC) Interrupt Enable Set */
#define REG_EIC_INTFLAG            (0x40002814) /**< \brief (EIC) Interrupt Flag Status and Clear */
#define REG_EIC_ASYNCH             (0x40002818) /**< \brief (EIC) External Interrupt Asynchronous Mode */
#define REG_EIC_CONFIG0            (0x4000281C) /**< \brief (EIC) External Interrupt Sense Configuration 0 */
#define REG_EIC_CONFIG1            (0x40002820) /**< \brief (EIC) External Interrupt Sense Configuration 1 */
#define REG_EIC_DEBOUNCEN          (0x40002830) /**< \brief (EIC) Debouncer Enable */
#define REG_EIC_DPRESCALER         (0x40002834) /**< \brief (EIC) Debouncer Prescaler */
#define REG_EIC_PINSTATE           (0x40002838) /**< \brief (EIC) Pin State */
#else
#define REG_EIC_CTRLA              (*(RwReg8 *)0x40002800UL) /**< \brief (EIC) Control A */
#define REG_EIC_NMICTRL            (*(RwReg8 *)0x40002801UL) /**< \brief (EIC) Non-Maskable Interrupt Control */
#define REG_EIC_NMIFLAG            (*(RwReg16*)0x40002802UL) /**< \brief (EIC) Non-Maskable Interrupt Flag Status and Clear */
#define REG_EIC_SYNCBUSY           (*(RoReg  *)0x40002804UL) /**< \brief (EIC) Synchronization Busy */
#define REG_EIC_EVCTRL             (*(RwReg  *)0x40002808UL) /**< \brief (EIC) Event Control */
#define REG_EIC_INTENCLR           (*(RwReg  *)0x4000280CUL) /**< \brief (EIC) Interrupt Enable Clear */
#define REG_EIC_INTENSET           (*(RwReg  *)0x40002810UL) /**< \brief (EIC) Interrupt Enable Set */
#define REG_EIC_INTFLAG            (*(RwReg  *)0x40002814UL) /**< \brief (EIC) Interrupt Flag Status and Clear */
#define REG_EIC_ASYNCH             (*(RwReg  *)0x40002818UL) /**< \brief (EIC) External Interrupt Asynchronous Mode */
#define REG_EIC_CONFIG0            (*(RwReg  *)0x4000281CUL) /**< \brief (EIC) External Interrupt Sense Configuration 0 */
#define REG_EIC_CONFIG1            (*(RwReg  *)0x40002820UL) /**< \brief (EIC) External Interrupt Sense Configuration 1 */
#define REG_EIC_DEBOUNCEN          (*(RwReg  *)0x40002830UL) /**< \brief (EIC) Debouncer Enable */
#define REG_EIC_DPRESCALER         (*(RwReg  *)0x40002834UL) /**< \brief (EIC) Debouncer Prescaler */
#define REG_EIC_PINSTATE           (*(RoReg  *)0x40002838UL) /**< \brief (EIC) Pin State */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_EIC_INSTANCE_FIXUP_H_ */
