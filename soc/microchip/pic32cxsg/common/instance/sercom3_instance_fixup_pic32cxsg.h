/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_SERCOM3_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_SERCOM3_INSTANCE_FIXUP_H_

/* ========== Register definition for SERCOM3 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SERCOM3_I2CM_CTRLA     (0x41014000) /**< \brief (SERCOM3) I2CM Control A */
#define REG_SERCOM3_I2CM_CTRLB     (0x41014004) /**< \brief (SERCOM3) I2CM Control B */
#define REG_SERCOM3_I2CM_CTRLC     (0x41014008) /**< \brief (SERCOM3) I2CM Control C */
#define REG_SERCOM3_I2CM_BAUD      (0x4101400C) /**< \brief (SERCOM3) I2CM Baud Rate */
#define REG_SERCOM3_I2CM_INTENCLR  (0x41014014) /**< \brief (SERCOM3) I2CM Interrupt Enable Clear */
#define REG_SERCOM3_I2CM_INTENSET  (0x41014016) /**< \brief (SERCOM3) I2CM Interrupt Enable Set */
#define REG_SERCOM3_I2CM_INTFLAG   (0x41014018) /**< \brief (SERCOM3) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM3_I2CM_STATUS    (0x4101401A) /**< \brief (SERCOM3) I2CM Status */
#define REG_SERCOM3_I2CM_SYNCBUSY  (0x4101401C) /**< \brief (SERCOM3) I2CM Synchronization Busy */
#define REG_SERCOM3_I2CM_ADDR      (0x41014024) /**< \brief (SERCOM3) I2CM Address */
#define REG_SERCOM3_I2CM_DATA      (0x41014028) /**< \brief (SERCOM3) I2CM Data */
#define REG_SERCOM3_I2CM_DBGCTRL   (0x41014030) /**< \brief (SERCOM3) I2CM Debug Control */
#define REG_SERCOM3_I2CS_CTRLA     (0x41014000) /**< \brief (SERCOM3) I2CS Control A */
#define REG_SERCOM3_I2CS_CTRLB     (0x41014004) /**< \brief (SERCOM3) I2CS Control B */
#define REG_SERCOM3_I2CS_CTRLC     (0x41014008) /**< \brief (SERCOM3) I2CS Control C */
#define REG_SERCOM3_I2CS_INTENCLR  (0x41014014) /**< \brief (SERCOM3) I2CS Interrupt Enable Clear */
#define REG_SERCOM3_I2CS_INTENSET  (0x41014016) /**< \brief (SERCOM3) I2CS Interrupt Enable Set */
#define REG_SERCOM3_I2CS_INTFLAG   (0x41014018) /**< \brief (SERCOM3) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM3_I2CS_STATUS    (0x4101401A) /**< \brief (SERCOM3) I2CS Status */
#define REG_SERCOM3_I2CS_SYNCBUSY  (0x4101401C) /**< \brief (SERCOM3) I2CS Synchronization Busy */
#define REG_SERCOM3_I2CS_LENGTH    (0x41014022) /**< \brief (SERCOM3) I2CS Length */
#define REG_SERCOM3_I2CS_ADDR      (0x41014024) /**< \brief (SERCOM3) I2CS Address */
#define REG_SERCOM3_I2CS_DATA      (0x41014028) /**< \brief (SERCOM3) I2CS Data */
#define REG_SERCOM3_SPI_CTRLA      (0x41014000) /**< \brief (SERCOM3) SPI Control A */
#define REG_SERCOM3_SPI_CTRLB      (0x41014004) /**< \brief (SERCOM3) SPI Control B */
#define REG_SERCOM3_SPI_CTRLC      (0x41014008) /**< \brief (SERCOM3) SPI Control C */
#define REG_SERCOM3_SPI_BAUD       (0x4101400C) /**< \brief (SERCOM3) SPI Baud Rate */
#define REG_SERCOM3_SPI_INTENCLR   (0x41014014) /**< \brief (SERCOM3) SPI Interrupt Enable Clear */
#define REG_SERCOM3_SPI_INTENSET   (0x41014016) /**< \brief (SERCOM3) SPI Interrupt Enable Set */
#define REG_SERCOM3_SPI_INTFLAG    (0x41014018) /**< \brief (SERCOM3) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM3_SPI_STATUS     (0x4101401A) /**< \brief (SERCOM3) SPI Status */
#define REG_SERCOM3_SPI_SYNCBUSY   (0x4101401C) /**< \brief (SERCOM3) SPI Synchronization Busy */
#define REG_SERCOM3_SPI_LENGTH     (0x41014022) /**< \brief (SERCOM3) SPI Length */
#define REG_SERCOM3_SPI_ADDR       (0x41014024) /**< \brief (SERCOM3) SPI Address */
#define REG_SERCOM3_SPI_DATA       (0x41014028) /**< \brief (SERCOM3) SPI Data */
#define REG_SERCOM3_SPI_DBGCTRL    (0x41014030) /**< \brief (SERCOM3) SPI Debug Control */
#define REG_SERCOM3_USART_CTRLA    (0x41014000) /**< \brief (SERCOM3) USART Control A */
#define REG_SERCOM3_USART_CTRLB    (0x41014004) /**< \brief (SERCOM3) USART Control B */
#define REG_SERCOM3_USART_CTRLC    (0x41014008) /**< \brief (SERCOM3) USART Control C */
#define REG_SERCOM3_USART_BAUD     (0x4101400C) /**< \brief (SERCOM3) USART Baud Rate */
#define REG_SERCOM3_USART_RXPL     (0x4101400E) /**< \brief (SERCOM3) USART Receive Pulse Length */
#define REG_SERCOM3_USART_INTENCLR (0x41014014) /**< \brief (SERCOM3) USART Interrupt Enable Clear */
#define REG_SERCOM3_USART_INTENSET (0x41014016) /**< \brief (SERCOM3) USART Interrupt Enable Set */
#define REG_SERCOM3_USART_INTFLAG  (0x41014018) /**< \brief (SERCOM3) USART Interrupt Flag Status and Clear */
#define REG_SERCOM3_USART_STATUS   (0x4101401A) /**< \brief (SERCOM3) USART Status */
#define REG_SERCOM3_USART_SYNCBUSY (0x4101401C) /**< \brief (SERCOM3) USART Synchronization Busy */
#define REG_SERCOM3_USART_RXERRCNT (0x41014020) /**< \brief (SERCOM3) USART Receive Error Count */
#define REG_SERCOM3_USART_LENGTH   (0x41014022) /**< \brief (SERCOM3) USART Length */
#define REG_SERCOM3_USART_DATA     (0x41014028) /**< \brief (SERCOM3) USART Data */
#define REG_SERCOM3_USART_DBGCTRL  (0x41014030) /**< \brief (SERCOM3) USART Debug Control */
#else
#define REG_SERCOM3_I2CM_CTRLA     (*(RwReg  *)0x41014000UL) /**< \brief (SERCOM3) I2CM Control A */
#define REG_SERCOM3_I2CM_CTRLB     (*(RwReg  *)0x41014004UL) /**< \brief (SERCOM3) I2CM Control B */
#define REG_SERCOM3_I2CM_CTRLC     (*(RwReg  *)0x41014008UL) /**< \brief (SERCOM3) I2CM Control C */
#define REG_SERCOM3_I2CM_BAUD      (*(RwReg  *)0x4101400CUL) /**< \brief (SERCOM3) I2CM Baud Rate */
#define REG_SERCOM3_I2CM_INTENCLR  (*(RwReg8 *)0x41014014UL) /**< \brief (SERCOM3) I2CM Interrupt Enable Clear */
#define REG_SERCOM3_I2CM_INTENSET  (*(RwReg8 *)0x41014016UL) /**< \brief (SERCOM3) I2CM Interrupt Enable Set */
#define REG_SERCOM3_I2CM_INTFLAG   (*(RwReg8 *)0x41014018UL) /**< \brief (SERCOM3) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM3_I2CM_STATUS    (*(RwReg16*)0x4101401AUL) /**< \brief (SERCOM3) I2CM Status */
#define REG_SERCOM3_I2CM_SYNCBUSY  (*(RoReg  *)0x4101401CUL) /**< \brief (SERCOM3) I2CM Synchronization Busy */
#define REG_SERCOM3_I2CM_ADDR      (*(RwReg  *)0x41014024UL) /**< \brief (SERCOM3) I2CM Address */
#define REG_SERCOM3_I2CM_DATA      (*(RwReg  *)0x41014028UL) /**< \brief (SERCOM3) I2CM Data */
#define REG_SERCOM3_I2CM_DBGCTRL   (*(RwReg8 *)0x41014030UL) /**< \brief (SERCOM3) I2CM Debug Control */
#define REG_SERCOM3_I2CS_CTRLA     (*(RwReg  *)0x41014000UL) /**< \brief (SERCOM3) I2CS Control A */
#define REG_SERCOM3_I2CS_CTRLB     (*(RwReg  *)0x41014004UL) /**< \brief (SERCOM3) I2CS Control B */
#define REG_SERCOM3_I2CS_CTRLC     (*(RwReg  *)0x41014008UL) /**< \brief (SERCOM3) I2CS Control C */
#define REG_SERCOM3_I2CS_INTENCLR  (*(RwReg8 *)0x41014014UL) /**< \brief (SERCOM3) I2CS Interrupt Enable Clear */
#define REG_SERCOM3_I2CS_INTENSET  (*(RwReg8 *)0x41014016UL) /**< \brief (SERCOM3) I2CS Interrupt Enable Set */
#define REG_SERCOM3_I2CS_INTFLAG   (*(RwReg8 *)0x41014018UL) /**< \brief (SERCOM3) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM3_I2CS_STATUS    (*(RwReg16*)0x4101401AUL) /**< \brief (SERCOM3) I2CS Status */
#define REG_SERCOM3_I2CS_SYNCBUSY  (*(RoReg  *)0x4101401CUL) /**< \brief (SERCOM3) I2CS Synchronization Busy */
#define REG_SERCOM3_I2CS_LENGTH    (*(RwReg16*)0x41014022UL) /**< \brief (SERCOM3) I2CS Length */
#define REG_SERCOM3_I2CS_ADDR      (*(RwReg  *)0x41014024UL) /**< \brief (SERCOM3) I2CS Address */
#define REG_SERCOM3_I2CS_DATA      (*(RwReg  *)0x41014028UL) /**< \brief (SERCOM3) I2CS Data */
#define REG_SERCOM3_SPI_CTRLA      (*(RwReg  *)0x41014000UL) /**< \brief (SERCOM3) SPI Control A */
#define REG_SERCOM3_SPI_CTRLB      (*(RwReg  *)0x41014004UL) /**< \brief (SERCOM3) SPI Control B */
#define REG_SERCOM3_SPI_CTRLC      (*(RwReg  *)0x41014008UL) /**< \brief (SERCOM3) SPI Control C */
#define REG_SERCOM3_SPI_BAUD       (*(RwReg8 *)0x4101400CUL) /**< \brief (SERCOM3) SPI Baud Rate */
#define REG_SERCOM3_SPI_INTENCLR   (*(RwReg8 *)0x41014014UL) /**< \brief (SERCOM3) SPI Interrupt Enable Clear */
#define REG_SERCOM3_SPI_INTENSET   (*(RwReg8 *)0x41014016UL) /**< \brief (SERCOM3) SPI Interrupt Enable Set */
#define REG_SERCOM3_SPI_INTFLAG    (*(RwReg8 *)0x41014018UL) /**< \brief (SERCOM3) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM3_SPI_STATUS     (*(RwReg16*)0x4101401AUL) /**< \brief (SERCOM3) SPI Status */
#define REG_SERCOM3_SPI_SYNCBUSY   (*(RoReg  *)0x4101401CUL) /**< \brief (SERCOM3) SPI Synchronization Busy */
#define REG_SERCOM3_SPI_LENGTH     (*(RwReg16*)0x41014022UL) /**< \brief (SERCOM3) SPI Length */
#define REG_SERCOM3_SPI_ADDR       (*(RwReg  *)0x41014024UL) /**< \brief (SERCOM3) SPI Address */
#define REG_SERCOM3_SPI_DATA       (*(RwReg  *)0x41014028UL) /**< \brief (SERCOM3) SPI Data */
#define REG_SERCOM3_SPI_DBGCTRL    (*(RwReg8 *)0x41014030UL) /**< \brief (SERCOM3) SPI Debug Control */
#define REG_SERCOM3_USART_CTRLA    (*(RwReg  *)0x41014000UL) /**< \brief (SERCOM3) USART Control A */
#define REG_SERCOM3_USART_CTRLB    (*(RwReg  *)0x41014004UL) /**< \brief (SERCOM3) USART Control B */
#define REG_SERCOM3_USART_CTRLC    (*(RwReg  *)0x41014008UL) /**< \brief (SERCOM3) USART Control C */
#define REG_SERCOM3_USART_BAUD     (*(RwReg16*)0x4101400CUL) /**< \brief (SERCOM3) USART Baud Rate */
#define REG_SERCOM3_USART_RXPL     (*(RwReg8 *)0x4101400EUL) /**< \brief (SERCOM3) USART Receive Pulse Length */
#define REG_SERCOM3_USART_INTENCLR (*(RwReg8 *)0x41014014UL) /**< \brief (SERCOM3) USART Interrupt Enable Clear */
#define REG_SERCOM3_USART_INTENSET (*(RwReg8 *)0x41014016UL) /**< \brief (SERCOM3) USART Interrupt Enable Set */
#define REG_SERCOM3_USART_INTFLAG  (*(RwReg8 *)0x41014018UL) /**< \brief (SERCOM3) USART Interrupt Flag Status and Clear */
#define REG_SERCOM3_USART_STATUS   (*(RwReg16*)0x4101401AUL) /**< \brief (SERCOM3) USART Status */
#define REG_SERCOM3_USART_SYNCBUSY (*(RoReg  *)0x4101401CUL) /**< \brief (SERCOM3) USART Synchronization Busy */
#define REG_SERCOM3_USART_RXERRCNT (*(RoReg8 *)0x41014020UL) /**< \brief (SERCOM3) USART Receive Error Count */
#define REG_SERCOM3_USART_LENGTH   (*(RwReg16*)0x41014022UL) /**< \brief (SERCOM3) USART Length */
#define REG_SERCOM3_USART_DATA     (*(RwReg  *)0x41014028UL) /**< \brief (SERCOM3) USART Data */
#define REG_SERCOM3_USART_DBGCTRL  (*(RwReg8 *)0x41014030UL) /**< \brief (SERCOM3) USART Debug Control */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_SERCOM3_INSTANCE_FIXUP_H_ */
