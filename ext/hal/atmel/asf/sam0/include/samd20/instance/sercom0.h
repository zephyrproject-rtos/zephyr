/**
 * \file
 *
 * \brief Instance description for SERCOM0
 *
 * Copyright (c) 2017 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAMD20_SERCOM0_INSTANCE_
#define _SAMD20_SERCOM0_INSTANCE_

/* ========== Register definition for SERCOM0 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SERCOM0_I2CM_CTRLA     (0x42000800) /**< \brief (SERCOM0) I2CM Control A */
#define REG_SERCOM0_I2CM_CTRLB     (0x42000804) /**< \brief (SERCOM0) I2CM Control B */
#define REG_SERCOM0_I2CM_DBGCTRL   (0x42000808) /**< \brief (SERCOM0) I2CM Debug Control */
#define REG_SERCOM0_I2CM_BAUD      (0x4200080A) /**< \brief (SERCOM0) I2CM Baud Rate */
#define REG_SERCOM0_I2CM_INTENCLR  (0x4200080C) /**< \brief (SERCOM0) I2CM Interrupt Enable Clear */
#define REG_SERCOM0_I2CM_INTENSET  (0x4200080D) /**< \brief (SERCOM0) I2CM Interrupt Enable Set */
#define REG_SERCOM0_I2CM_INTFLAG   (0x4200080E) /**< \brief (SERCOM0) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM0_I2CM_STATUS    (0x42000810) /**< \brief (SERCOM0) I2CM Status */
#define REG_SERCOM0_I2CM_ADDR      (0x42000814) /**< \brief (SERCOM0) I2CM Address */
#define REG_SERCOM0_I2CM_DATA      (0x42000818) /**< \brief (SERCOM0) I2CM Data */
#define REG_SERCOM0_I2CS_CTRLA     (0x42000800) /**< \brief (SERCOM0) I2CS Control A */
#define REG_SERCOM0_I2CS_CTRLB     (0x42000804) /**< \brief (SERCOM0) I2CS Control B */
#define REG_SERCOM0_I2CS_INTENCLR  (0x4200080C) /**< \brief (SERCOM0) I2CS Interrupt Enable Clear */
#define REG_SERCOM0_I2CS_INTENSET  (0x4200080D) /**< \brief (SERCOM0) I2CS Interrupt Enable Set */
#define REG_SERCOM0_I2CS_INTFLAG   (0x4200080E) /**< \brief (SERCOM0) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM0_I2CS_STATUS    (0x42000810) /**< \brief (SERCOM0) I2CS Status */
#define REG_SERCOM0_I2CS_ADDR      (0x42000814) /**< \brief (SERCOM0) I2CS Address */
#define REG_SERCOM0_I2CS_DATA      (0x42000818) /**< \brief (SERCOM0) I2CS Data */
#define REG_SERCOM0_SPI_CTRLA      (0x42000800) /**< \brief (SERCOM0) SPI Control A */
#define REG_SERCOM0_SPI_CTRLB      (0x42000804) /**< \brief (SERCOM0) SPI Control B */
#define REG_SERCOM0_SPI_DBGCTRL    (0x42000808) /**< \brief (SERCOM0) SPI Debug Control */
#define REG_SERCOM0_SPI_BAUD       (0x4200080A) /**< \brief (SERCOM0) SPI Baud Rate */
#define REG_SERCOM0_SPI_INTENCLR   (0x4200080C) /**< \brief (SERCOM0) SPI Interrupt Enable Clear */
#define REG_SERCOM0_SPI_INTENSET   (0x4200080D) /**< \brief (SERCOM0) SPI Interrupt Enable Set */
#define REG_SERCOM0_SPI_INTFLAG    (0x4200080E) /**< \brief (SERCOM0) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM0_SPI_STATUS     (0x42000810) /**< \brief (SERCOM0) SPI Status */
#define REG_SERCOM0_SPI_ADDR       (0x42000814) /**< \brief (SERCOM0) SPI Address */
#define REG_SERCOM0_SPI_DATA       (0x42000818) /**< \brief (SERCOM0) SPI Data */
#define REG_SERCOM0_USART_CTRLA    (0x42000800) /**< \brief (SERCOM0) USART Control A */
#define REG_SERCOM0_USART_CTRLB    (0x42000804) /**< \brief (SERCOM0) USART Control B */
#define REG_SERCOM0_USART_DBGCTRL  (0x42000808) /**< \brief (SERCOM0) USART Debug Control */
#define REG_SERCOM0_USART_BAUD     (0x4200080A) /**< \brief (SERCOM0) USART Baud */
#define REG_SERCOM0_USART_INTENCLR (0x4200080C) /**< \brief (SERCOM0) USART Interrupt Enable Clear */
#define REG_SERCOM0_USART_INTENSET (0x4200080D) /**< \brief (SERCOM0) USART Interrupt Enable Set */
#define REG_SERCOM0_USART_INTFLAG  (0x4200080E) /**< \brief (SERCOM0) USART Interrupt Flag Status and Clear */
#define REG_SERCOM0_USART_STATUS   (0x42000810) /**< \brief (SERCOM0) USART Status */
#define REG_SERCOM0_USART_DATA     (0x42000818) /**< \brief (SERCOM0) USART Data */
#else
#define REG_SERCOM0_I2CM_CTRLA     (*(RwReg  *)0x42000800UL) /**< \brief (SERCOM0) I2CM Control A */
#define REG_SERCOM0_I2CM_CTRLB     (*(RwReg  *)0x42000804UL) /**< \brief (SERCOM0) I2CM Control B */
#define REG_SERCOM0_I2CM_DBGCTRL   (*(RwReg8 *)0x42000808UL) /**< \brief (SERCOM0) I2CM Debug Control */
#define REG_SERCOM0_I2CM_BAUD      (*(RwReg16*)0x4200080AUL) /**< \brief (SERCOM0) I2CM Baud Rate */
#define REG_SERCOM0_I2CM_INTENCLR  (*(RwReg8 *)0x4200080CUL) /**< \brief (SERCOM0) I2CM Interrupt Enable Clear */
#define REG_SERCOM0_I2CM_INTENSET  (*(RwReg8 *)0x4200080DUL) /**< \brief (SERCOM0) I2CM Interrupt Enable Set */
#define REG_SERCOM0_I2CM_INTFLAG   (*(RwReg8 *)0x4200080EUL) /**< \brief (SERCOM0) I2CM Interrupt Flag Status and Clear */
#define REG_SERCOM0_I2CM_STATUS    (*(RwReg16*)0x42000810UL) /**< \brief (SERCOM0) I2CM Status */
#define REG_SERCOM0_I2CM_ADDR      (*(RwReg8 *)0x42000814UL) /**< \brief (SERCOM0) I2CM Address */
#define REG_SERCOM0_I2CM_DATA      (*(RwReg8 *)0x42000818UL) /**< \brief (SERCOM0) I2CM Data */
#define REG_SERCOM0_I2CS_CTRLA     (*(RwReg  *)0x42000800UL) /**< \brief (SERCOM0) I2CS Control A */
#define REG_SERCOM0_I2CS_CTRLB     (*(RwReg  *)0x42000804UL) /**< \brief (SERCOM0) I2CS Control B */
#define REG_SERCOM0_I2CS_INTENCLR  (*(RwReg8 *)0x4200080CUL) /**< \brief (SERCOM0) I2CS Interrupt Enable Clear */
#define REG_SERCOM0_I2CS_INTENSET  (*(RwReg8 *)0x4200080DUL) /**< \brief (SERCOM0) I2CS Interrupt Enable Set */
#define REG_SERCOM0_I2CS_INTFLAG   (*(RwReg8 *)0x4200080EUL) /**< \brief (SERCOM0) I2CS Interrupt Flag Status and Clear */
#define REG_SERCOM0_I2CS_STATUS    (*(RwReg16*)0x42000810UL) /**< \brief (SERCOM0) I2CS Status */
#define REG_SERCOM0_I2CS_ADDR      (*(RwReg  *)0x42000814UL) /**< \brief (SERCOM0) I2CS Address */
#define REG_SERCOM0_I2CS_DATA      (*(RwReg8 *)0x42000818UL) /**< \brief (SERCOM0) I2CS Data */
#define REG_SERCOM0_SPI_CTRLA      (*(RwReg  *)0x42000800UL) /**< \brief (SERCOM0) SPI Control A */
#define REG_SERCOM0_SPI_CTRLB      (*(RwReg  *)0x42000804UL) /**< \brief (SERCOM0) SPI Control B */
#define REG_SERCOM0_SPI_DBGCTRL    (*(RwReg8 *)0x42000808UL) /**< \brief (SERCOM0) SPI Debug Control */
#define REG_SERCOM0_SPI_BAUD       (*(RwReg8 *)0x4200080AUL) /**< \brief (SERCOM0) SPI Baud Rate */
#define REG_SERCOM0_SPI_INTENCLR   (*(RwReg8 *)0x4200080CUL) /**< \brief (SERCOM0) SPI Interrupt Enable Clear */
#define REG_SERCOM0_SPI_INTENSET   (*(RwReg8 *)0x4200080DUL) /**< \brief (SERCOM0) SPI Interrupt Enable Set */
#define REG_SERCOM0_SPI_INTFLAG    (*(RwReg8 *)0x4200080EUL) /**< \brief (SERCOM0) SPI Interrupt Flag Status and Clear */
#define REG_SERCOM0_SPI_STATUS     (*(RwReg16*)0x42000810UL) /**< \brief (SERCOM0) SPI Status */
#define REG_SERCOM0_SPI_ADDR       (*(RwReg  *)0x42000814UL) /**< \brief (SERCOM0) SPI Address */
#define REG_SERCOM0_SPI_DATA       (*(RwReg16*)0x42000818UL) /**< \brief (SERCOM0) SPI Data */
#define REG_SERCOM0_USART_CTRLA    (*(RwReg  *)0x42000800UL) /**< \brief (SERCOM0) USART Control A */
#define REG_SERCOM0_USART_CTRLB    (*(RwReg  *)0x42000804UL) /**< \brief (SERCOM0) USART Control B */
#define REG_SERCOM0_USART_DBGCTRL  (*(RwReg8 *)0x42000808UL) /**< \brief (SERCOM0) USART Debug Control */
#define REG_SERCOM0_USART_BAUD     (*(RwReg16*)0x4200080AUL) /**< \brief (SERCOM0) USART Baud */
#define REG_SERCOM0_USART_INTENCLR (*(RwReg8 *)0x4200080CUL) /**< \brief (SERCOM0) USART Interrupt Enable Clear */
#define REG_SERCOM0_USART_INTENSET (*(RwReg8 *)0x4200080DUL) /**< \brief (SERCOM0) USART Interrupt Enable Set */
#define REG_SERCOM0_USART_INTFLAG  (*(RwReg8 *)0x4200080EUL) /**< \brief (SERCOM0) USART Interrupt Flag Status and Clear */
#define REG_SERCOM0_USART_STATUS   (*(RwReg16*)0x42000810UL) /**< \brief (SERCOM0) USART Status */
#define REG_SERCOM0_USART_DATA     (*(RwReg16*)0x42000818UL) /**< \brief (SERCOM0) USART Data */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for SERCOM0 peripheral ========== */
#define SERCOM0_GCLK_ID_CORE        13      
#define SERCOM0_GCLK_ID_SLOW        12      
#define SERCOM0_INT_MSB             3       
#define SERCOM0_PMSB                3       

#endif /* _SAMD20_SERCOM0_INSTANCE_ */
