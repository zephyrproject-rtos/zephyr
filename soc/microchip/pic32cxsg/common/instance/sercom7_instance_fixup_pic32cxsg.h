/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_SERCOM7_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_SERCOM7_INSTANCE_FIXUP_H_

/* ========== Register definition for SERCOM7 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SERCOM7_I2CM_CTRLA     (0x43000C00) /**< \brief (SERCOM7) I2CM Control A */
#define REG_SERCOM7_I2CM_CTRLB     (0x43000C04) /**< \brief (SERCOM7) I2CM Control B */
#define REG_SERCOM7_I2CM_CTRLC     (0x43000C08) /**< \brief (SERCOM7) I2CM Control C */
#define REG_SERCOM7_I2CM_BAUD      (0x43000C0C) /**< \brief (SERCOM7) I2CM Baud Rate */
#define REG_SERCOM7_I2CM_INTENCLR  (0x43000C14) /**< \brief (SERCOM7) I2CM Interrupt Enable Clear */
#define REG_SERCOM7_I2CM_INTENSET  (0x43000C16) /**< \brief (SERCOM7) I2CM Interrupt Enable Set */
#define REG_SERCOM7_I2CM_INTFLAG   (0x43000C18) /**< \brief (SERCOM7) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM7_I2CM_STATUS    (0x43000C1A) /**< \brief (SERCOM7) I2CM Status */
#define REG_SERCOM7_I2CM_SYNCBUSY  (0x43000C1C) /**< \brief (SERCOM7) I2CM Synchronization Busy */
#define REG_SERCOM7_I2CM_ADDR      (0x43000C24) /**< \brief (SERCOM7) I2CM Address */
#define REG_SERCOM7_I2CM_DATA      (0x43000C28) /**< \brief (SERCOM7) I2CM Data */
#define REG_SERCOM7_I2CM_DBGCTRL   (0x43000C30) /**< \brief (SERCOM7) I2CM Debug Control */
#define REG_SERCOM7_I2CS_CTRLA     (0x43000C00) /**< \brief (SERCOM7) I2CS Control A */
#define REG_SERCOM7_I2CS_CTRLB     (0x43000C04) /**< \brief (SERCOM7) I2CS Control B */
#define REG_SERCOM7_I2CS_CTRLC     (0x43000C08) /**< \brief (SERCOM7) I2CS Control C */
#define REG_SERCOM7_I2CS_INTENCLR  (0x43000C14) /**< \brief (SERCOM7) I2CS Interrupt Enable Clear */
#define REG_SERCOM7_I2CS_INTENSET  (0x43000C16) /**< \brief (SERCOM7) I2CS Interrupt Enable Set */
#define REG_SERCOM7_I2CS_INTFLAG   (0x43000C18) /**< \brief (SERCOM7) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM7_I2CS_STATUS    (0x43000C1A) /**< \brief (SERCOM7) I2CS Status */
#define REG_SERCOM7_I2CS_SYNCBUSY  (0x43000C1C) /**< \brief (SERCOM7) I2CS Synchronization Busy */
#define REG_SERCOM7_I2CS_LENGTH    (0x43000C22) /**< \brief (SERCOM7) I2CS Length */
#define REG_SERCOM7_I2CS_ADDR      (0x43000C24) /**< \brief (SERCOM7) I2CS Address */
#define REG_SERCOM7_I2CS_DATA      (0x43000C28) /**< \brief (SERCOM7) I2CS Data */
#define REG_SERCOM7_SPI_CTRLA      (0x43000C00) /**< \brief (SERCOM7) SPI Control A */
#define REG_SERCOM7_SPI_CTRLB      (0x43000C04) /**< \brief (SERCOM7) SPI Control B */
#define REG_SERCOM7_SPI_CTRLC      (0x43000C08) /**< \brief (SERCOM7) SPI Control C */
#define REG_SERCOM7_SPI_BAUD       (0x43000C0C) /**< \brief (SERCOM7) SPI Baud Rate */
#define REG_SERCOM7_SPI_INTENCLR   (0x43000C14) /**< \brief (SERCOM7) SPI Interrupt Enable Clear */
#define REG_SERCOM7_SPI_INTENSET   (0x43000C16) /**< \brief (SERCOM7) SPI Interrupt Enable Set */
#define REG_SERCOM7_SPI_INTFLAG    (0x43000C18) /**< \brief (SERCOM7) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM7_SPI_STATUS     (0x43000C1A) /**< \brief (SERCOM7) SPI Status */
#define REG_SERCOM7_SPI_SYNCBUSY   (0x43000C1C) /**< \brief (SERCOM7) SPI Synchronization Busy */
#define REG_SERCOM7_SPI_LENGTH     (0x43000C22) /**< \brief (SERCOM7) SPI Length */
#define REG_SERCOM7_SPI_ADDR       (0x43000C24) /**< \brief (SERCOM7) SPI Address */
#define REG_SERCOM7_SPI_DATA       (0x43000C28) /**< \brief (SERCOM7) SPI Data */
#define REG_SERCOM7_SPI_DBGCTRL    (0x43000C30) /**< \brief (SERCOM7) SPI Debug Control */
#define REG_SERCOM7_USART_CTRLA    (0x43000C00) /**< \brief (SERCOM7) USART Control A */
#define REG_SERCOM7_USART_CTRLB    (0x43000C04) /**< \brief (SERCOM7) USART Control B */
#define REG_SERCOM7_USART_CTRLC    (0x43000C08) /**< \brief (SERCOM7) USART Control C */
#define REG_SERCOM7_USART_BAUD     (0x43000C0C) /**< \brief (SERCOM7) USART Baud Rate */
#define REG_SERCOM7_USART_RXPL     (0x43000C0E) /**< \brief (SERCOM7) USART Receive Pulse Length */
#define REG_SERCOM7_USART_INTENCLR (0x43000C14) /**< \brief (SERCOM7) USART Interrupt Enable Clear */
#define REG_SERCOM7_USART_INTENSET (0x43000C16) /**< \brief (SERCOM7) USART Interrupt Enable Set */
#define REG_SERCOM7_USART_INTFLAG  (0x43000C18) /**< \brief (SERCOM7) USART Interrupt Flag Status and Clear */
#define REG_SERCOM7_USART_STATUS   (0x43000C1A) /**< \brief (SERCOM7) USART Status */
#define REG_SERCOM7_USART_SYNCBUSY (0x43000C1C) /**< \brief (SERCOM7) USART Synchronization Busy */
#define REG_SERCOM7_USART_RXERRCNT (0x43000C20) /**< \brief (SERCOM7) USART Receive Error Count */
#define REG_SERCOM7_USART_LENGTH   (0x43000C22) /**< \brief (SERCOM7) USART Length */
#define REG_SERCOM7_USART_DATA     (0x43000C28) /**< \brief (SERCOM7) USART Data */
#define REG_SERCOM7_USART_DBGCTRL  (0x43000C30) /**< \brief (SERCOM7) USART Debug Control */
#else
#define REG_SERCOM7_I2CM_CTRLA     (*(RwReg  *)0x43000C00UL) /**< \brief (SERCOM7) I2CM Control A */
#define REG_SERCOM7_I2CM_CTRLB     (*(RwReg  *)0x43000C04UL) /**< \brief (SERCOM7) I2CM Control B */
#define REG_SERCOM7_I2CM_CTRLC     (*(RwReg  *)0x43000C08UL) /**< \brief (SERCOM7) I2CM Control C */
#define REG_SERCOM7_I2CM_BAUD      (*(RwReg  *)0x43000C0CUL) /**< \brief (SERCOM7) I2CM Baud Rate */
#define REG_SERCOM7_I2CM_INTENCLR  (*(RwReg8 *)0x43000C14UL) /**< \brief (SERCOM7) I2CM Interrupt Enable Clear */
#define REG_SERCOM7_I2CM_INTENSET  (*(RwReg8 *)0x43000C16UL) /**< \brief (SERCOM7) I2CM Interrupt Enable Set */
#define REG_SERCOM7_I2CM_INTFLAG   (*(RwReg8 *)0x43000C18UL) /**< \brief (SERCOM7) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM7_I2CM_STATUS    (*(RwReg16*)0x43000C1AUL) /**< \brief (SERCOM7) I2CM Status */
#define REG_SERCOM7_I2CM_SYNCBUSY  (*(RoReg  *)0x43000C1CUL) /**< \brief (SERCOM7) I2CM Synchronization Busy */
#define REG_SERCOM7_I2CM_ADDR      (*(RwReg  *)0x43000C24UL) /**< \brief (SERCOM7) I2CM Address */
#define REG_SERCOM7_I2CM_DATA      (*(RwReg  *)0x43000C28UL) /**< \brief (SERCOM7) I2CM Data */
#define REG_SERCOM7_I2CM_DBGCTRL   (*(RwReg8 *)0x43000C30UL) /**< \brief (SERCOM7) I2CM Debug Control */
#define REG_SERCOM7_I2CS_CTRLA     (*(RwReg  *)0x43000C00UL) /**< \brief (SERCOM7) I2CS Control A */
#define REG_SERCOM7_I2CS_CTRLB     (*(RwReg  *)0x43000C04UL) /**< \brief (SERCOM7) I2CS Control B */
#define REG_SERCOM7_I2CS_CTRLC     (*(RwReg  *)0x43000C08UL) /**< \brief (SERCOM7) I2CS Control C */
#define REG_SERCOM7_I2CS_INTENCLR  (*(RwReg8 *)0x43000C14UL) /**< \brief (SERCOM7) I2CS Interrupt Enable Clear */
#define REG_SERCOM7_I2CS_INTENSET  (*(RwReg8 *)0x43000C16UL) /**< \brief (SERCOM7) I2CS Interrupt Enable Set */
#define REG_SERCOM7_I2CS_INTFLAG   (*(RwReg8 *)0x43000C18UL) /**< \brief (SERCOM7) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM7_I2CS_STATUS    (*(RwReg16*)0x43000C1AUL) /**< \brief (SERCOM7) I2CS Status */
#define REG_SERCOM7_I2CS_SYNCBUSY  (*(RoReg  *)0x43000C1CUL) /**< \brief (SERCOM7) I2CS Synchronization Busy */
#define REG_SERCOM7_I2CS_LENGTH    (*(RwReg16*)0x43000C22UL) /**< \brief (SERCOM7) I2CS Length */
#define REG_SERCOM7_I2CS_ADDR      (*(RwReg  *)0x43000C24UL) /**< \brief (SERCOM7) I2CS Address */
#define REG_SERCOM7_I2CS_DATA      (*(RwReg  *)0x43000C28UL) /**< \brief (SERCOM7) I2CS Data */
#define REG_SERCOM7_SPI_CTRLA      (*(RwReg  *)0x43000C00UL) /**< \brief (SERCOM7) SPI Control A */
#define REG_SERCOM7_SPI_CTRLB      (*(RwReg  *)0x43000C04UL) /**< \brief (SERCOM7) SPI Control B */
#define REG_SERCOM7_SPI_CTRLC      (*(RwReg  *)0x43000C08UL) /**< \brief (SERCOM7) SPI Control C */
#define REG_SERCOM7_SPI_BAUD       (*(RwReg8 *)0x43000C0CUL) /**< \brief (SERCOM7) SPI Baud Rate */
#define REG_SERCOM7_SPI_INTENCLR   (*(RwReg8 *)0x43000C14UL) /**< \brief (SERCOM7) SPI Interrupt Enable Clear */
#define REG_SERCOM7_SPI_INTENSET   (*(RwReg8 *)0x43000C16UL) /**< \brief (SERCOM7) SPI Interrupt Enable Set */
#define REG_SERCOM7_SPI_INTFLAG    (*(RwReg8 *)0x43000C18UL) /**< \brief (SERCOM7) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM7_SPI_STATUS     (*(RwReg16*)0x43000C1AUL) /**< \brief (SERCOM7) SPI Status */
#define REG_SERCOM7_SPI_SYNCBUSY   (*(RoReg  *)0x43000C1CUL) /**< \brief (SERCOM7) SPI Synchronization Busy */
#define REG_SERCOM7_SPI_LENGTH     (*(RwReg16*)0x43000C22UL) /**< \brief (SERCOM7) SPI Length */
#define REG_SERCOM7_SPI_ADDR       (*(RwReg  *)0x43000C24UL) /**< \brief (SERCOM7) SPI Address */
#define REG_SERCOM7_SPI_DATA       (*(RwReg  *)0x43000C28UL) /**< \brief (SERCOM7) SPI Data */
#define REG_SERCOM7_SPI_DBGCTRL    (*(RwReg8 *)0x43000C30UL) /**< \brief (SERCOM7) SPI Debug Control */
#define REG_SERCOM7_USART_CTRLA    (*(RwReg  *)0x43000C00UL) /**< \brief (SERCOM7) USART Control A */
#define REG_SERCOM7_USART_CTRLB    (*(RwReg  *)0x43000C04UL) /**< \brief (SERCOM7) USART Control B */
#define REG_SERCOM7_USART_CTRLC    (*(RwReg  *)0x43000C08UL) /**< \brief (SERCOM7) USART Control C */
#define REG_SERCOM7_USART_BAUD     (*(RwReg16*)0x43000C0CUL) /**< \brief (SERCOM7) USART Baud Rate */
#define REG_SERCOM7_USART_RXPL     (*(RwReg8 *)0x43000C0EUL) /**< \brief (SERCOM7) USART Receive Pulse Length */
#define REG_SERCOM7_USART_INTENCLR (*(RwReg8 *)0x43000C14UL) /**< \brief (SERCOM7) USART Interrupt Enable Clear */
#define REG_SERCOM7_USART_INTENSET (*(RwReg8 *)0x43000C16UL) /**< \brief (SERCOM7) USART Interrupt Enable Set */
#define REG_SERCOM7_USART_INTFLAG  (*(RwReg8 *)0x43000C18UL) /**< \brief (SERCOM7) USART Interrupt Flag Status and Clear */
#define REG_SERCOM7_USART_STATUS   (*(RwReg16*)0x43000C1AUL) /**< \brief (SERCOM7) USART Status */
#define REG_SERCOM7_USART_SYNCBUSY (*(RoReg  *)0x43000C1CUL) /**< \brief (SERCOM7) USART Synchronization Busy */
#define REG_SERCOM7_USART_RXERRCNT (*(RoReg8 *)0x43000C20UL) /**< \brief (SERCOM7) USART Receive Error Count */
#define REG_SERCOM7_USART_LENGTH   (*(RwReg16*)0x43000C22UL) /**< \brief (SERCOM7) USART Length */
#define REG_SERCOM7_USART_DATA     (*(RwReg  *)0x43000C28UL) /**< \brief (SERCOM7) USART Data */
#define REG_SERCOM7_USART_DBGCTRL  (*(RwReg8 *)0x43000C30UL) /**< \brief (SERCOM7) USART Debug Control */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_SERCOM7_INSTANCE_FIXUP_H_ */
