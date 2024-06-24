/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_SERCOM5_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_SERCOM5_INSTANCE_FIXUP_H_

/* ========== Register definition for SERCOM5 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SERCOM5_I2CM_CTRLA     (0x43000400) /**< \brief (SERCOM5) I2CM Control A */
#define REG_SERCOM5_I2CM_CTRLB     (0x43000404) /**< \brief (SERCOM5) I2CM Control B */
#define REG_SERCOM5_I2CM_CTRLC     (0x43000408) /**< \brief (SERCOM5) I2CM Control C */
#define REG_SERCOM5_I2CM_BAUD      (0x4300040C) /**< \brief (SERCOM5) I2CM Baud Rate */
#define REG_SERCOM5_I2CM_INTENCLR  (0x43000414) /**< \brief (SERCOM5) I2CM Interrupt Enable Clear */
#define REG_SERCOM5_I2CM_INTENSET  (0x43000416) /**< \brief (SERCOM5) I2CM Interrupt Enable Set */
#define REG_SERCOM5_I2CM_INTFLAG   (0x43000418) /**< \brief (SERCOM5) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM5_I2CM_STATUS    (0x4300041A) /**< \brief (SERCOM5) I2CM Status */
#define REG_SERCOM5_I2CM_SYNCBUSY  (0x4300041C) /**< \brief (SERCOM5) I2CM Synchronization Busy */
#define REG_SERCOM5_I2CM_ADDR      (0x43000424) /**< \brief (SERCOM5) I2CM Address */
#define REG_SERCOM5_I2CM_DATA      (0x43000428) /**< \brief (SERCOM5) I2CM Data */
#define REG_SERCOM5_I2CM_DBGCTRL   (0x43000430) /**< \brief (SERCOM5) I2CM Debug Control */
#define REG_SERCOM5_I2CS_CTRLA     (0x43000400) /**< \brief (SERCOM5) I2CS Control A */
#define REG_SERCOM5_I2CS_CTRLB     (0x43000404) /**< \brief (SERCOM5) I2CS Control B */
#define REG_SERCOM5_I2CS_CTRLC     (0x43000408) /**< \brief (SERCOM5) I2CS Control C */
#define REG_SERCOM5_I2CS_INTENCLR  (0x43000414) /**< \brief (SERCOM5) I2CS Interrupt Enable Clear */
#define REG_SERCOM5_I2CS_INTENSET  (0x43000416) /**< \brief (SERCOM5) I2CS Interrupt Enable Set */
#define REG_SERCOM5_I2CS_INTFLAG   (0x43000418) /**< \brief (SERCOM5) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM5_I2CS_STATUS    (0x4300041A) /**< \brief (SERCOM5) I2CS Status */
#define REG_SERCOM5_I2CS_SYNCBUSY  (0x4300041C) /**< \brief (SERCOM5) I2CS Synchronization Busy */
#define REG_SERCOM5_I2CS_LENGTH    (0x43000422) /**< \brief (SERCOM5) I2CS Length */
#define REG_SERCOM5_I2CS_ADDR      (0x43000424) /**< \brief (SERCOM5) I2CS Address */
#define REG_SERCOM5_I2CS_DATA      (0x43000428) /**< \brief (SERCOM5) I2CS Data */
#define REG_SERCOM5_SPI_CTRLA      (0x43000400) /**< \brief (SERCOM5) SPI Control A */
#define REG_SERCOM5_SPI_CTRLB      (0x43000404) /**< \brief (SERCOM5) SPI Control B */
#define REG_SERCOM5_SPI_CTRLC      (0x43000408) /**< \brief (SERCOM5) SPI Control C */
#define REG_SERCOM5_SPI_BAUD       (0x4300040C) /**< \brief (SERCOM5) SPI Baud Rate */
#define REG_SERCOM5_SPI_INTENCLR   (0x43000414) /**< \brief (SERCOM5) SPI Interrupt Enable Clear */
#define REG_SERCOM5_SPI_INTENSET   (0x43000416) /**< \brief (SERCOM5) SPI Interrupt Enable Set */
#define REG_SERCOM5_SPI_INTFLAG    (0x43000418) /**< \brief (SERCOM5) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM5_SPI_STATUS     (0x4300041A) /**< \brief (SERCOM5) SPI Status */
#define REG_SERCOM5_SPI_SYNCBUSY   (0x4300041C) /**< \brief (SERCOM5) SPI Synchronization Busy */
#define REG_SERCOM5_SPI_LENGTH     (0x43000422) /**< \brief (SERCOM5) SPI Length */
#define REG_SERCOM5_SPI_ADDR       (0x43000424) /**< \brief (SERCOM5) SPI Address */
#define REG_SERCOM5_SPI_DATA       (0x43000428) /**< \brief (SERCOM5) SPI Data */
#define REG_SERCOM5_SPI_DBGCTRL    (0x43000430) /**< \brief (SERCOM5) SPI Debug Control */
#define REG_SERCOM5_USART_CTRLA    (0x43000400) /**< \brief (SERCOM5) USART Control A */
#define REG_SERCOM5_USART_CTRLB    (0x43000404) /**< \brief (SERCOM5) USART Control B */
#define REG_SERCOM5_USART_CTRLC    (0x43000408) /**< \brief (SERCOM5) USART Control C */
#define REG_SERCOM5_USART_BAUD     (0x4300040C) /**< \brief (SERCOM5) USART Baud Rate */
#define REG_SERCOM5_USART_RXPL     (0x4300040E) /**< \brief (SERCOM5) USART Receive Pulse Length */
#define REG_SERCOM5_USART_INTENCLR (0x43000414) /**< \brief (SERCOM5) USART Interrupt Enable Clear */
#define REG_SERCOM5_USART_INTENSET (0x43000416) /**< \brief (SERCOM5) USART Interrupt Enable Set */
#define REG_SERCOM5_USART_INTFLAG  (0x43000418) /**< \brief (SERCOM5) USART Interrupt Flag Status and Clear */
#define REG_SERCOM5_USART_STATUS   (0x4300041A) /**< \brief (SERCOM5) USART Status */
#define REG_SERCOM5_USART_SYNCBUSY (0x4300041C) /**< \brief (SERCOM5) USART Synchronization Busy */
#define REG_SERCOM5_USART_RXERRCNT (0x43000420) /**< \brief (SERCOM5) USART Receive Error Count */
#define REG_SERCOM5_USART_LENGTH   (0x43000422) /**< \brief (SERCOM5) USART Length */
#define REG_SERCOM5_USART_DATA     (0x43000428) /**< \brief (SERCOM5) USART Data */
#define REG_SERCOM5_USART_DBGCTRL  (0x43000430) /**< \brief (SERCOM5) USART Debug Control */
#else
#define REG_SERCOM5_I2CM_CTRLA     (*(RwReg  *)0x43000400UL) /**< \brief (SERCOM5) I2CM Control A */
#define REG_SERCOM5_I2CM_CTRLB     (*(RwReg  *)0x43000404UL) /**< \brief (SERCOM5) I2CM Control B */
#define REG_SERCOM5_I2CM_CTRLC     (*(RwReg  *)0x43000408UL) /**< \brief (SERCOM5) I2CM Control C */
#define REG_SERCOM5_I2CM_BAUD      (*(RwReg  *)0x4300040CUL) /**< \brief (SERCOM5) I2CM Baud Rate */
#define REG_SERCOM5_I2CM_INTENCLR  (*(RwReg8 *)0x43000414UL) /**< \brief (SERCOM5) I2CM Interrupt Enable Clear */
#define REG_SERCOM5_I2CM_INTENSET  (*(RwReg8 *)0x43000416UL) /**< \brief (SERCOM5) I2CM Interrupt Enable Set */
#define REG_SERCOM5_I2CM_INTFLAG   (*(RwReg8 *)0x43000418UL) /**< \brief (SERCOM5) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM5_I2CM_STATUS    (*(RwReg16*)0x4300041AUL) /**< \brief (SERCOM5) I2CM Status */
#define REG_SERCOM5_I2CM_SYNCBUSY  (*(RoReg  *)0x4300041CUL) /**< \brief (SERCOM5) I2CM Synchronization Busy */
#define REG_SERCOM5_I2CM_ADDR      (*(RwReg  *)0x43000424UL) /**< \brief (SERCOM5) I2CM Address */
#define REG_SERCOM5_I2CM_DATA      (*(RwReg  *)0x43000428UL) /**< \brief (SERCOM5) I2CM Data */
#define REG_SERCOM5_I2CM_DBGCTRL   (*(RwReg8 *)0x43000430UL) /**< \brief (SERCOM5) I2CM Debug Control */
#define REG_SERCOM5_I2CS_CTRLA     (*(RwReg  *)0x43000400UL) /**< \brief (SERCOM5) I2CS Control A */
#define REG_SERCOM5_I2CS_CTRLB     (*(RwReg  *)0x43000404UL) /**< \brief (SERCOM5) I2CS Control B */
#define REG_SERCOM5_I2CS_CTRLC     (*(RwReg  *)0x43000408UL) /**< \brief (SERCOM5) I2CS Control C */
#define REG_SERCOM5_I2CS_INTENCLR  (*(RwReg8 *)0x43000414UL) /**< \brief (SERCOM5) I2CS Interrupt Enable Clear */
#define REG_SERCOM5_I2CS_INTENSET  (*(RwReg8 *)0x43000416UL) /**< \brief (SERCOM5) I2CS Interrupt Enable Set */
#define REG_SERCOM5_I2CS_INTFLAG   (*(RwReg8 *)0x43000418UL) /**< \brief (SERCOM5) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM5_I2CS_STATUS    (*(RwReg16*)0x4300041AUL) /**< \brief (SERCOM5) I2CS Status */
#define REG_SERCOM5_I2CS_SYNCBUSY  (*(RoReg  *)0x4300041CUL) /**< \brief (SERCOM5) I2CS Synchronization Busy */
#define REG_SERCOM5_I2CS_LENGTH    (*(RwReg16*)0x43000422UL) /**< \brief (SERCOM5) I2CS Length */
#define REG_SERCOM5_I2CS_ADDR      (*(RwReg  *)0x43000424UL) /**< \brief (SERCOM5) I2CS Address */
#define REG_SERCOM5_I2CS_DATA      (*(RwReg  *)0x43000428UL) /**< \brief (SERCOM5) I2CS Data */
#define REG_SERCOM5_SPI_CTRLA      (*(RwReg  *)0x43000400UL) /**< \brief (SERCOM5) SPI Control A */
#define REG_SERCOM5_SPI_CTRLB      (*(RwReg  *)0x43000404UL) /**< \brief (SERCOM5) SPI Control B */
#define REG_SERCOM5_SPI_CTRLC      (*(RwReg  *)0x43000408UL) /**< \brief (SERCOM5) SPI Control C */
#define REG_SERCOM5_SPI_BAUD       (*(RwReg8 *)0x4300040CUL) /**< \brief (SERCOM5) SPI Baud Rate */
#define REG_SERCOM5_SPI_INTENCLR   (*(RwReg8 *)0x43000414UL) /**< \brief (SERCOM5) SPI Interrupt Enable Clear */
#define REG_SERCOM5_SPI_INTENSET   (*(RwReg8 *)0x43000416UL) /**< \brief (SERCOM5) SPI Interrupt Enable Set */
#define REG_SERCOM5_SPI_INTFLAG    (*(RwReg8 *)0x43000418UL) /**< \brief (SERCOM5) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM5_SPI_STATUS     (*(RwReg16*)0x4300041AUL) /**< \brief (SERCOM5) SPI Status */
#define REG_SERCOM5_SPI_SYNCBUSY   (*(RoReg  *)0x4300041CUL) /**< \brief (SERCOM5) SPI Synchronization Busy */
#define REG_SERCOM5_SPI_LENGTH     (*(RwReg16*)0x43000422UL) /**< \brief (SERCOM5) SPI Length */
#define REG_SERCOM5_SPI_ADDR       (*(RwReg  *)0x43000424UL) /**< \brief (SERCOM5) SPI Address */
#define REG_SERCOM5_SPI_DATA       (*(RwReg  *)0x43000428UL) /**< \brief (SERCOM5) SPI Data */
#define REG_SERCOM5_SPI_DBGCTRL    (*(RwReg8 *)0x43000430UL) /**< \brief (SERCOM5) SPI Debug Control */
#define REG_SERCOM5_USART_CTRLA    (*(RwReg  *)0x43000400UL) /**< \brief (SERCOM5) USART Control A */
#define REG_SERCOM5_USART_CTRLB    (*(RwReg  *)0x43000404UL) /**< \brief (SERCOM5) USART Control B */
#define REG_SERCOM5_USART_CTRLC    (*(RwReg  *)0x43000408UL) /**< \brief (SERCOM5) USART Control C */
#define REG_SERCOM5_USART_BAUD     (*(RwReg16*)0x4300040CUL) /**< \brief (SERCOM5) USART Baud Rate */
#define REG_SERCOM5_USART_RXPL     (*(RwReg8 *)0x4300040EUL) /**< \brief (SERCOM5) USART Receive Pulse Length */
#define REG_SERCOM5_USART_INTENCLR (*(RwReg8 *)0x43000414UL) /**< \brief (SERCOM5) USART Interrupt Enable Clear */
#define REG_SERCOM5_USART_INTENSET (*(RwReg8 *)0x43000416UL) /**< \brief (SERCOM5) USART Interrupt Enable Set */
#define REG_SERCOM5_USART_INTFLAG  (*(RwReg8 *)0x43000418UL) /**< \brief (SERCOM5) USART Interrupt Flag Status and Clear */
#define REG_SERCOM5_USART_STATUS   (*(RwReg16*)0x4300041AUL) /**< \brief (SERCOM5) USART Status */
#define REG_SERCOM5_USART_SYNCBUSY (*(RoReg  *)0x4300041CUL) /**< \brief (SERCOM5) USART Synchronization Busy */
#define REG_SERCOM5_USART_RXERRCNT (*(RoReg8 *)0x43000420UL) /**< \brief (SERCOM5) USART Receive Error Count */
#define REG_SERCOM5_USART_LENGTH   (*(RwReg16*)0x43000422UL) /**< \brief (SERCOM5) USART Length */
#define REG_SERCOM5_USART_DATA     (*(RwReg  *)0x43000428UL) /**< \brief (SERCOM5) USART Data */
#define REG_SERCOM5_USART_DBGCTRL  (*(RwReg8 *)0x43000430UL) /**< \brief (SERCOM5) USART Debug Control */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_SERCOM5_INSTANCE_FIXUP_H_ */
