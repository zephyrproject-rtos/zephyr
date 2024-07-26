/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_PDEC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_PDEC_INSTANCE_FIXUP_H_

/* ========== Register definition for PDEC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_PDEC_CTRLA             (0x42001C00) /**< \brief (PDEC) Control A */
#define REG_PDEC_CTRLBCLR          (0x42001C04) /**< \brief (PDEC) Control B Clear */
#define REG_PDEC_CTRLBSET          (0x42001C05) /**< \brief (PDEC) Control B Set */
#define REG_PDEC_EVCTRL            (0x42001C06) /**< \brief (PDEC) Event Control */
#define REG_PDEC_INTENCLR          (0x42001C08) /**< \brief (PDEC) Interrupt Enable Clear */
#define REG_PDEC_INTENSET          (0x42001C09) /**< \brief (PDEC) Interrupt Enable Set */
#define REG_PDEC_INTFLAG           (0x42001C0A) /**< \brief (PDEC) Interrupt Flag Status and Clear */
#define REG_PDEC_STATUS            (0x42001C0C) /**< \brief (PDEC) Status */
#define REG_PDEC_DBGCTRL           (0x42001C0F) /**< \brief (PDEC) Debug Control */
#define REG_PDEC_SYNCBUSY          (0x42001C10) /**< \brief (PDEC) Synchronization Status */
#define REG_PDEC_PRESC             (0x42001C14) /**< \brief (PDEC) Prescaler Value */
#define REG_PDEC_FILTER            (0x42001C15) /**< \brief (PDEC) Filter Value */
#define REG_PDEC_PRESCBUF          (0x42001C18) /**< \brief (PDEC) Prescaler Buffer Value */
#define REG_PDEC_FILTERBUF         (0x42001C19) /**< \brief (PDEC) Filter Buffer Value */
#define REG_PDEC_COUNT             (0x42001C1C) /**< \brief (PDEC) Counter Value */
#define REG_PDEC_CC0               (0x42001C20) /**< \brief (PDEC) Channel 0 Compare Value */
#define REG_PDEC_CC1               (0x42001C24) /**< \brief (PDEC) Channel 1 Compare Value */
#define REG_PDEC_CCBUF0            (0x42001C30) /**< \brief (PDEC) Channel Compare Buffer Value 0 */
#define REG_PDEC_CCBUF1            (0x42001C34) /**< \brief (PDEC) Channel Compare Buffer Value 1 */
#else
#define REG_PDEC_CTRLA             (*(RwReg  *)0x42001C00UL) /**< \brief (PDEC) Control A */
#define REG_PDEC_CTRLBCLR          (*(RwReg8 *)0x42001C04UL) /**< \brief (PDEC) Control B Clear */
#define REG_PDEC_CTRLBSET          (*(RwReg8 *)0x42001C05UL) /**< \brief (PDEC) Control B Set */
#define REG_PDEC_EVCTRL            (*(RwReg16*)0x42001C06UL) /**< \brief (PDEC) Event Control */
#define REG_PDEC_INTENCLR          (*(RwReg8 *)0x42001C08UL) /**< \brief (PDEC) Interrupt Enable Clear */
#define REG_PDEC_INTENSET          (*(RwReg8 *)0x42001C09UL) /**< \brief (PDEC) Interrupt Enable Set */
#define REG_PDEC_INTFLAG           (*(RwReg8 *)0x42001C0AUL) /**< \brief (PDEC) Interrupt Flag Status and Clear */
#define REG_PDEC_STATUS            (*(RwReg16*)0x42001C0CUL) /**< \brief (PDEC) Status */
#define REG_PDEC_DBGCTRL           (*(RwReg8 *)0x42001C0FUL) /**< \brief (PDEC) Debug Control */
#define REG_PDEC_SYNCBUSY          (*(RoReg  *)0x42001C10UL) /**< \brief (PDEC) Synchronization Status */
#define REG_PDEC_PRESC             (*(RwReg8 *)0x42001C14UL) /**< \brief (PDEC) Prescaler Value */
#define REG_PDEC_FILTER            (*(RwReg8 *)0x42001C15UL) /**< \brief (PDEC) Filter Value */
#define REG_PDEC_PRESCBUF          (*(RwReg8 *)0x42001C18UL) /**< \brief (PDEC) Prescaler Buffer Value */
#define REG_PDEC_FILTERBUF         (*(RwReg8 *)0x42001C19UL) /**< \brief (PDEC) Filter Buffer Value */
#define REG_PDEC_COUNT             (*(RwReg  *)0x42001C1CUL) /**< \brief (PDEC) Counter Value */
#define REG_PDEC_CC0               (*(RwReg  *)0x42001C20UL) /**< \brief (PDEC) Channel 0 Compare Value */
#define REG_PDEC_CC1               (*(RwReg  *)0x42001C24UL) /**< \brief (PDEC) Channel 1 Compare Value */
#define REG_PDEC_CCBUF0            (*(RwReg  *)0x42001C30UL) /**< \brief (PDEC) Channel Compare Buffer Value 0 */
#define REG_PDEC_CCBUF1            (*(RwReg  *)0x42001C34UL) /**< \brief (PDEC) Channel Compare Buffer Value 1 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_PDEC_INSTANCE_FIXUP_H_ */
