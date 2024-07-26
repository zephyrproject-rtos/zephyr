/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_SERCOM4_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_SERCOM4_INSTANCE_FIXUP_H_

/* ========== Register definition for SERCOM4 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SERCOM4_I2CM_CTRLA     (0x43000000) /**< \brief (SERCOM4) I2CM Control A */
#define REG_SERCOM4_I2CM_CTRLB     (0x43000004) /**< \brief (SERCOM4) I2CM Control B */
#define REG_SERCOM4_I2CM_CTRLC     (0x43000008) /**< \brief (SERCOM4) I2CM Control C */
#define REG_SERCOM4_I2CM_BAUD      (0x4300000C) /**< \brief (SERCOM4) I2CM Baud Rate */
#define REG_SERCOM4_I2CM_INTENCLR  (0x43000014) /**< \brief (SERCOM4) I2CM Interrupt Enable Clear */
#define REG_SERCOM4_I2CM_INTENSET  (0x43000016) /**< \brief (SERCOM4) I2CM Interrupt Enable Set */
#define REG_SERCOM4_I2CM_INTFLAG   (0x43000018) /**< \brief (SERCOM4) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM4_I2CM_STATUS    (0x4300001A) /**< \brief (SERCOM4) I2CM Status */
#define REG_SERCOM4_I2CM_SYNCBUSY  (0x4300001C) /**< \brief (SERCOM4) I2CM Synchronization Busy */
#define REG_SERCOM4_I2CM_ADDR      (0x43000024) /**< \brief (SERCOM4) I2CM Address */
#define REG_SERCOM4_I2CM_DATA      (0x43000028) /**< \brief (SERCOM4) I2CM Data */
#define REG_SERCOM4_I2CM_DBGCTRL   (0x43000030) /**< \brief (SERCOM4) I2CM Debug Control */
#define REG_SERCOM4_I2CS_CTRLA     (0x43000000) /**< \brief (SERCOM4) I2CS Control A */
#define REG_SERCOM4_I2CS_CTRLB     (0x43000004) /**< \brief (SERCOM4) I2CS Control B */
#define REG_SERCOM4_I2CS_CTRLC     (0x43000008) /**< \brief (SERCOM4) I2CS Control C */
#define REG_SERCOM4_I2CS_INTENCLR  (0x43000014) /**< \brief (SERCOM4) I2CS Interrupt Enable Clear */
#define REG_SERCOM4_I2CS_INTENSET  (0x43000016) /**< \brief (SERCOM4) I2CS Interrupt Enable Set */
#define REG_SERCOM4_I2CS_INTFLAG   (0x43000018) /**< \brief (SERCOM4) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM4_I2CS_STATUS    (0x4300001A) /**< \brief (SERCOM4) I2CS Status */
#define REG_SERCOM4_I2CS_SYNCBUSY  (0x4300001C) /**< \brief (SERCOM4) I2CS Synchronization Busy */
#define REG_SERCOM4_I2CS_LENGTH    (0x43000022) /**< \brief (SERCOM4) I2CS Length */
#define REG_SERCOM4_I2CS_ADDR      (0x43000024) /**< \brief (SERCOM4) I2CS Address */
#define REG_SERCOM4_I2CS_DATA      (0x43000028) /**< \brief (SERCOM4) I2CS Data */
#define REG_SERCOM4_SPI_CTRLA      (0x43000000) /**< \brief (SERCOM4) SPI Control A */
#define REG_SERCOM4_SPI_CTRLB      (0x43000004) /**< \brief (SERCOM4) SPI Control B */
#define REG_SERCOM4_SPI_CTRLC      (0x43000008) /**< \brief (SERCOM4) SPI Control C */
#define REG_SERCOM4_SPI_BAUD       (0x4300000C) /**< \brief (SERCOM4) SPI Baud Rate */
#define REG_SERCOM4_SPI_INTENCLR   (0x43000014) /**< \brief (SERCOM4) SPI Interrupt Enable Clear */
#define REG_SERCOM4_SPI_INTENSET   (0x43000016) /**< \brief (SERCOM4) SPI Interrupt Enable Set */
#define REG_SERCOM4_SPI_INTFLAG    (0x43000018) /**< \brief (SERCOM4) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM4_SPI_STATUS     (0x4300001A) /**< \brief (SERCOM4) SPI Status */
#define REG_SERCOM4_SPI_SYNCBUSY   (0x4300001C) /**< \brief (SERCOM4) SPI Synchronization Busy */
#define REG_SERCOM4_SPI_LENGTH     (0x43000022) /**< \brief (SERCOM4) SPI Length */
#define REG_SERCOM4_SPI_ADDR       (0x43000024) /**< \brief (SERCOM4) SPI Address */
#define REG_SERCOM4_SPI_DATA       (0x43000028) /**< \brief (SERCOM4) SPI Data */
#define REG_SERCOM4_SPI_DBGCTRL    (0x43000030) /**< \brief (SERCOM4) SPI Debug Control */
#define REG_SERCOM4_USART_CTRLA    (0x43000000) /**< \brief (SERCOM4) USART Control A */
#define REG_SERCOM4_USART_CTRLB    (0x43000004) /**< \brief (SERCOM4) USART Control B */
#define REG_SERCOM4_USART_CTRLC    (0x43000008) /**< \brief (SERCOM4) USART Control C */
#define REG_SERCOM4_USART_BAUD     (0x4300000C) /**< \brief (SERCOM4) USART Baud Rate */
#define REG_SERCOM4_USART_RXPL     (0x4300000E) /**< \brief (SERCOM4) USART Receive Pulse Length */
#define REG_SERCOM4_USART_INTENCLR (0x43000014) /**< \brief (SERCOM4) USART Interrupt Enable Clear */
#define REG_SERCOM4_USART_INTENSET (0x43000016) /**< \brief (SERCOM4) USART Interrupt Enable Set */
#define REG_SERCOM4_USART_INTFLAG  (0x43000018) /**< \brief (SERCOM4) USART Interrupt Flag Status and Clear */
#define REG_SERCOM4_USART_STATUS   (0x4300001A) /**< \brief (SERCOM4) USART Status */
#define REG_SERCOM4_USART_SYNCBUSY (0x4300001C) /**< \brief (SERCOM4) USART Synchronization Busy */
#define REG_SERCOM4_USART_RXERRCNT (0x43000020) /**< \brief (SERCOM4) USART Receive Error Count */
#define REG_SERCOM4_USART_LENGTH   (0x43000022) /**< \brief (SERCOM4) USART Length */
#define REG_SERCOM4_USART_DATA     (0x43000028) /**< \brief (SERCOM4) USART Data */
#define REG_SERCOM4_USART_DBGCTRL  (0x43000030) /**< \brief (SERCOM4) USART Debug Control */
#else
#define REG_SERCOM4_I2CM_CTRLA     (*(RwReg  *)0x43000000UL) /**< \brief (SERCOM4) I2CM Control A */
#define REG_SERCOM4_I2CM_CTRLB     (*(RwReg  *)0x43000004UL) /**< \brief (SERCOM4) I2CM Control B */
#define REG_SERCOM4_I2CM_CTRLC     (*(RwReg  *)0x43000008UL) /**< \brief (SERCOM4) I2CM Control C */
#define REG_SERCOM4_I2CM_BAUD      (*(RwReg  *)0x4300000CUL) /**< \brief (SERCOM4) I2CM Baud Rate */
#define REG_SERCOM4_I2CM_INTENCLR  (*(RwReg8 *)0x43000014UL) /**< \brief (SERCOM4) I2CM Interrupt Enable Clear */
#define REG_SERCOM4_I2CM_INTENSET  (*(RwReg8 *)0x43000016UL) /**< \brief (SERCOM4) I2CM Interrupt Enable Set */
#define REG_SERCOM4_I2CM_INTFLAG   (*(RwReg8 *)0x43000018UL) /**< \brief (SERCOM4) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM4_I2CM_STATUS    (*(RwReg16*)0x4300001AUL) /**< \brief (SERCOM4) I2CM Status */
#define REG_SERCOM4_I2CM_SYNCBUSY  (*(RoReg  *)0x4300001CUL) /**< \brief (SERCOM4) I2CM Synchronization Busy */
#define REG_SERCOM4_I2CM_ADDR      (*(RwReg  *)0x43000024UL) /**< \brief (SERCOM4) I2CM Address */
#define REG_SERCOM4_I2CM_DATA      (*(RwReg  *)0x43000028UL) /**< \brief (SERCOM4) I2CM Data */
#define REG_SERCOM4_I2CM_DBGCTRL   (*(RwReg8 *)0x43000030UL) /**< \brief (SERCOM4) I2CM Debug Control */
#define REG_SERCOM4_I2CS_CTRLA     (*(RwReg  *)0x43000000UL) /**< \brief (SERCOM4) I2CS Control A */
#define REG_SERCOM4_I2CS_CTRLB     (*(RwReg  *)0x43000004UL) /**< \brief (SERCOM4) I2CS Control B */
#define REG_SERCOM4_I2CS_CTRLC     (*(RwReg  *)0x43000008UL) /**< \brief (SERCOM4) I2CS Control C */
#define REG_SERCOM4_I2CS_INTENCLR  (*(RwReg8 *)0x43000014UL) /**< \brief (SERCOM4) I2CS Interrupt Enable Clear */
#define REG_SERCOM4_I2CS_INTENSET  (*(RwReg8 *)0x43000016UL) /**< \brief (SERCOM4) I2CS Interrupt Enable Set */
#define REG_SERCOM4_I2CS_INTFLAG   (*(RwReg8 *)0x43000018UL) /**< \brief (SERCOM4) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM4_I2CS_STATUS    (*(RwReg16*)0x4300001AUL) /**< \brief (SERCOM4) I2CS Status */
#define REG_SERCOM4_I2CS_SYNCBUSY  (*(RoReg  *)0x4300001CUL) /**< \brief (SERCOM4) I2CS Synchronization Busy */
#define REG_SERCOM4_I2CS_LENGTH    (*(RwReg16*)0x43000022UL) /**< \brief (SERCOM4) I2CS Length */
#define REG_SERCOM4_I2CS_ADDR      (*(RwReg  *)0x43000024UL) /**< \brief (SERCOM4) I2CS Address */
#define REG_SERCOM4_I2CS_DATA      (*(RwReg  *)0x43000028UL) /**< \brief (SERCOM4) I2CS Data */
#define REG_SERCOM4_SPI_CTRLA      (*(RwReg  *)0x43000000UL) /**< \brief (SERCOM4) SPI Control A */
#define REG_SERCOM4_SPI_CTRLB      (*(RwReg  *)0x43000004UL) /**< \brief (SERCOM4) SPI Control B */
#define REG_SERCOM4_SPI_CTRLC      (*(RwReg  *)0x43000008UL) /**< \brief (SERCOM4) SPI Control C */
#define REG_SERCOM4_SPI_BAUD       (*(RwReg8 *)0x4300000CUL) /**< \brief (SERCOM4) SPI Baud Rate */
#define REG_SERCOM4_SPI_INTENCLR   (*(RwReg8 *)0x43000014UL) /**< \brief (SERCOM4) SPI Interrupt Enable Clear */
#define REG_SERCOM4_SPI_INTENSET   (*(RwReg8 *)0x43000016UL) /**< \brief (SERCOM4) SPI Interrupt Enable Set */
#define REG_SERCOM4_SPI_INTFLAG    (*(RwReg8 *)0x43000018UL) /**< \brief (SERCOM4) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM4_SPI_STATUS     (*(RwReg16*)0x4300001AUL) /**< \brief (SERCOM4) SPI Status */
#define REG_SERCOM4_SPI_SYNCBUSY   (*(RoReg  *)0x4300001CUL) /**< \brief (SERCOM4) SPI Synchronization Busy */
#define REG_SERCOM4_SPI_LENGTH     (*(RwReg16*)0x43000022UL) /**< \brief (SERCOM4) SPI Length */
#define REG_SERCOM4_SPI_ADDR       (*(RwReg  *)0x43000024UL) /**< \brief (SERCOM4) SPI Address */
#define REG_SERCOM4_SPI_DATA       (*(RwReg  *)0x43000028UL) /**< \brief (SERCOM4) SPI Data */
#define REG_SERCOM4_SPI_DBGCTRL    (*(RwReg8 *)0x43000030UL) /**< \brief (SERCOM4) SPI Debug Control */
#define REG_SERCOM4_USART_CTRLA    (*(RwReg  *)0x43000000UL) /**< \brief (SERCOM4) USART Control A */
#define REG_SERCOM4_USART_CTRLB    (*(RwReg  *)0x43000004UL) /**< \brief (SERCOM4) USART Control B */
#define REG_SERCOM4_USART_CTRLC    (*(RwReg  *)0x43000008UL) /**< \brief (SERCOM4) USART Control C */
#define REG_SERCOM4_USART_BAUD     (*(RwReg16*)0x4300000CUL) /**< \brief (SERCOM4) USART Baud Rate */
#define REG_SERCOM4_USART_RXPL     (*(RwReg8 *)0x4300000EUL) /**< \brief (SERCOM4) USART Receive Pulse Length */
#define REG_SERCOM4_USART_INTENCLR (*(RwReg8 *)0x43000014UL) /**< \brief (SERCOM4) USART Interrupt Enable Clear */
#define REG_SERCOM4_USART_INTENSET (*(RwReg8 *)0x43000016UL) /**< \brief (SERCOM4) USART Interrupt Enable Set */
#define REG_SERCOM4_USART_INTFLAG  (*(RwReg8 *)0x43000018UL) /**< \brief (SERCOM4) USART Interrupt Flag Status and Clear */
#define REG_SERCOM4_USART_STATUS   (*(RwReg16*)0x4300001AUL) /**< \brief (SERCOM4) USART Status */
#define REG_SERCOM4_USART_SYNCBUSY (*(RoReg  *)0x4300001CUL) /**< \brief (SERCOM4) USART Synchronization Busy */
#define REG_SERCOM4_USART_RXERRCNT (*(RoReg8 *)0x43000020UL) /**< \brief (SERCOM4) USART Receive Error Count */
#define REG_SERCOM4_USART_LENGTH   (*(RwReg16*)0x43000022UL) /**< \brief (SERCOM4) USART Length */
#define REG_SERCOM4_USART_DATA     (*(RwReg  *)0x43000028UL) /**< \brief (SERCOM4) USART Data */
#define REG_SERCOM4_USART_DBGCTRL  (*(RwReg8 *)0x43000030UL) /**< \brief (SERCOM4) USART Debug Control */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_SERCOM4_INSTANCE_FIXUP_H_ */
