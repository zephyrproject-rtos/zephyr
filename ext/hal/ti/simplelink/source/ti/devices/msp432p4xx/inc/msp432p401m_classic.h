/******************************************************************************
*
* Copyright (C) 2012 - 2017 Texas Instruments Incorporated - http://www.ti.com/
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*  Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*
*  Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the
*  distribution.
*
*  Neither the name of Texas Instruments Incorporated nor the names of
*  its contributors may be used to endorse or promote products derived
*  from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* MSP432P401M Register Definitions
*
* This file includes MSP430 style component and register definitions
* for legacy components re-used in MSP432
*
* File creation date: 2017-08-03
*
******************************************************************************/

#ifndef __MSP432P401M_CLASSIC_H__
#define __MSP432P401M_CLASSIC_H__

/* Use standard integer types with explicit width */
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/******************************************************************************
* Device memory map                                                           *
******************************************************************************/
#define __MAIN_MEMORY_START__                              (0x00000000)          /*!< Main Flash memory start address */
#define __MAIN_MEMORY_END__                                (0x0001FFFF)          /*!< Main Flash memory end address */
#define __BSL_MEMORY_START__                               (0x00202000)          /*!< BSL memory start address */
#define __BSL_MEMORY_END__                                 (0x00203FFF)          /*!< BSL memory end address */
#define __SRAM_START__                                     (0x20000000)          /*!< SRAM memory start address */
#define __SRAM_END__                                       (0x20007FFF)          /*!< SRAM memory end address */

/******************************************************************************
* MSP-format peripheral registers                                             *
******************************************************************************/

/******************************************************************************
* AES256 Registers
******************************************************************************/
#define AESACTL0                                 (HWREG16(0x40003C00))           /*!< AES Accelerator Control Register 0 */
#define AESACTL1                                 (HWREG16(0x40003C02))           /*!< AES Accelerator Control Register 1 */
#define AESASTAT                                 (HWREG16(0x40003C04))           /*!< AES Accelerator Status Register */
#define AESAKEY                                  (HWREG16(0x40003C06))           /*!< AES Accelerator Key Register */
#define AESADIN                                  (HWREG16(0x40003C08))           /*!< AES Accelerator Data In Register */
#define AESADOUT                                 (HWREG16(0x40003C0A))           /*!< AES Accelerator Data Out Register */
#define AESAXDIN                                 (HWREG16(0x40003C0C))           /*!< AES Accelerator XORed Data In Register */
#define AESAXIN                                  (HWREG16(0x40003C0E))           /*!< AES Accelerator XORed Data In Register */

/* Register offsets from AES256_BASE address */
#define OFS_AESACTL0                                       (0x0000)              /*!< AES Accelerator Control Register 0 */
#define OFS_AESACTL1                                       (0x0002)              /*!< AES Accelerator Control Register 1 */
#define OFS_AESASTAT                                       (0x0004)              /*!< AES Accelerator Status Register */
#define OFS_AESAKEY                                        (0x0006)              /*!< AES Accelerator Key Register */
#define OFS_AESADIN                                        (0x0008)              /*!< AES Accelerator Data In Register */
#define OFS_AESADOUT                                       (0x000A)              /*!< AES Accelerator Data Out Register */
#define OFS_AESAXDIN                                       (0x000C)              /*!< AES Accelerator XORed Data In Register */
#define OFS_AESAXIN                                        (0x000E)              /*!< AES Accelerator XORed Data In Register */


/******************************************************************************
* CAPTIO0 Registers
******************************************************************************/
#define CAPTIO0CTL                               (HWREG16(0x4000540E))           /*!< Capacitive Touch IO x Control Register */

/* Register offsets from CAPTIO0_BASE address */
#define OFS_CAPTIO0CTL                                     (0x000E)              /*!< Capacitive Touch IO x Control Register */

#define CAPTIO0CTL_L                                       (HWREG8_L(CAPTIO0CTL))/* Capacitive Touch IO x Control Register */
#define CAPTIO0CTL_H                                       (HWREG8_H(CAPTIO0CTL))/* Capacitive Touch IO x Control Register */

/******************************************************************************
* CAPTIO1 Registers
******************************************************************************/
#define CAPTIO1CTL                               (HWREG16(0x4000580E))           /*!< Capacitive Touch IO x Control Register */

/* Register offsets from CAPTIO1_BASE address */
#define OFS_CAPTIO1CTL                                     (0x000E)              /*!< Capacitive Touch IO x Control Register */

#define CAPTIO1CTL_L                                       (HWREG8_L(CAPTIO1CTL))/* Capacitive Touch IO x Control Register */
#define CAPTIO1CTL_H                                       (HWREG8_H(CAPTIO1CTL))/* Capacitive Touch IO x Control Register */

/******************************************************************************
* COMP_E0 Registers
******************************************************************************/
#define CE0CTL0                                  (HWREG16(0x40003400))           /*!< Comparator Control Register 0 */
#define CE0CTL1                                  (HWREG16(0x40003402))           /*!< Comparator Control Register 1 */
#define CE0CTL2                                  (HWREG16(0x40003404))           /*!< Comparator Control Register 2 */
#define CE0CTL3                                  (HWREG16(0x40003406))           /*!< Comparator Control Register 3 */
#define CE0INT                                   (HWREG16(0x4000340C))           /*!< Comparator Interrupt Control Register */
#define CE0IV                                    (HWREG16(0x4000340E))           /*!< Comparator Interrupt Vector Word Register */

/* Register offsets from COMP_E0_BASE address */
#define OFS_CE0CTL0                                        (0x0000)              /*!< Comparator Control Register 0 */
#define OFS_CE0CTL1                                        (0x0002)              /*!< Comparator Control Register 1 */
#define OFS_CE0CTL2                                        (0x0004)              /*!< Comparator Control Register 2 */
#define OFS_CE0CTL3                                        (0x0006)              /*!< Comparator Control Register 3 */
#define OFS_CE0INT                                         (0x000C)              /*!< Comparator Interrupt Control Register */
#define OFS_CE0IV                                          (0x000E)              /*!< Comparator Interrupt Vector Word Register */


/******************************************************************************
* COMP_E1 Registers
******************************************************************************/
#define CE1CTL0                                  (HWREG16(0x40003800))           /*!< Comparator Control Register 0 */
#define CE1CTL1                                  (HWREG16(0x40003802))           /*!< Comparator Control Register 1 */
#define CE1CTL2                                  (HWREG16(0x40003804))           /*!< Comparator Control Register 2 */
#define CE1CTL3                                  (HWREG16(0x40003806))           /*!< Comparator Control Register 3 */
#define CE1INT                                   (HWREG16(0x4000380C))           /*!< Comparator Interrupt Control Register */
#define CE1IV                                    (HWREG16(0x4000380E))           /*!< Comparator Interrupt Vector Word Register */

/* Register offsets from COMP_E1_BASE address */
#define OFS_CE1CTL0                                        (0x0000)              /*!< Comparator Control Register 0 */
#define OFS_CE1CTL1                                        (0x0002)              /*!< Comparator Control Register 1 */
#define OFS_CE1CTL2                                        (0x0004)              /*!< Comparator Control Register 2 */
#define OFS_CE1CTL3                                        (0x0006)              /*!< Comparator Control Register 3 */
#define OFS_CE1INT                                         (0x000C)              /*!< Comparator Interrupt Control Register */
#define OFS_CE1IV                                          (0x000E)              /*!< Comparator Interrupt Vector Word Register */


/******************************************************************************
* CRC32 Registers
******************************************************************************/
#define CRC32DI                                  (HWREG16(0x40004000))           /*!< Data Input for CRC32 Signature Computation */
#define CRC32DIRB                                (HWREG16(0x40004004))           /*!< Data In Reverse for CRC32 Computation */
#define CRC32INIRES_LO                           (HWREG16(0x40004008))           /*!< CRC32 Initialization and Result, lower 16 bits */
#define CRC32INIRES_HI                           (HWREG16(0x4000400A))           /*!< CRC32 Initialization and Result, upper 16 bits */
#define CRC32RESR_LO                             (HWREG16(0x4000400C))           /*!< CRC32 Result Reverse, lower 16 bits */
#define CRC32RESR_HI                             (HWREG16(0x4000400E))           /*!< CRC32 Result Reverse, Upper 16 bits */
#define CRC16DI                                  (HWREG16(0x40004010))           /*!< Data Input for CRC16 computation */
#define CRC16DIRB                                (HWREG16(0x40004014))           /*!< CRC16 Data In Reverse */
#define CRC16INIRES                              (HWREG16(0x40004018))           /*!< CRC16 Initialization and Result register */
#define CRC16RESR                                (HWREG16(0x4000401E))           /*!< CRC16 Result Reverse */

/* Register offsets from CRC32_BASE address */
#define OFS_CRC32DI                                        (0x0000)              /*!< Data Input for CRC32 Signature Computation */
#define OFS_CRC32DIRB                                      (0x0004)              /*!< Data In Reverse for CRC32 Computation */
#define OFS_CRC32INIRES_LO                                 (0x0008)              /*!< CRC32 Initialization and Result, lower 16 bits */
#define OFS_CRC32INIRES_HI                                 (0x000A)              /*!< CRC32 Initialization and Result, upper 16 bits */
#define OFS_CRC32RESR_LO                                   (0x000C)              /*!< CRC32 Result Reverse, lower 16 bits */
#define OFS_CRC32RESR_HI                                   (0x000E)              /*!< CRC32 Result Reverse, Upper 16 bits */
#define OFS_CRC16DI                                        (0x0010)              /*!< Data Input for CRC16 computation */
#define OFS_CRC16DIRB                                      (0x0014)              /*!< CRC16 Data In Reverse */
#define OFS_CRC16INIRES                                    (0x0018)              /*!< CRC16 Initialization and Result register */
#define OFS_CRC16RESR                                      (0x001E)              /*!< CRC16 Result Reverse */


/******************************************************************************
* DIO Registers
******************************************************************************/
#define PAIN                                     (HWREG16(0x40004C00))           /*!< Port A Input */
#define PAOUT                                    (HWREG16(0x40004C02))           /*!< Port A Output */
#define PADIR                                    (HWREG16(0x40004C04))           /*!< Port A Direction */
#define PAREN                                    (HWREG16(0x40004C06))           /*!< Port A Resistor Enable */
#define PADS                                     (HWREG16(0x40004C08))           /*!< Port A Drive Strength */
#define PASEL0                                   (HWREG16(0x40004C0A))           /*!< Port A Select 0 */
#define PASEL1                                   (HWREG16(0x40004C0C))           /*!< Port A Select 1 */
#define P1IV                                     (HWREG16(0x40004C0E))           /*!< Port 1 Interrupt Vector Register */
#define PASELC                                   (HWREG16(0x40004C16))           /*!< Port A Complement Select */
#define PAIES                                    (HWREG16(0x40004C18))           /*!< Port A Interrupt Edge Select */
#define PAIE                                     (HWREG16(0x40004C1A))           /*!< Port A Interrupt Enable */
#define PAIFG                                    (HWREG16(0x40004C1C))           /*!< Port A Interrupt Flag */
#define P2IV                                     (HWREG16(0x40004C1E))           /*!< Port 2 Interrupt Vector Register */
#define PBIN                                     (HWREG16(0x40004C20))           /*!< Port B Input */
#define PBOUT                                    (HWREG16(0x40004C22))           /*!< Port B Output */
#define PBDIR                                    (HWREG16(0x40004C24))           /*!< Port B Direction */
#define PBREN                                    (HWREG16(0x40004C26))           /*!< Port B Resistor Enable */
#define PBDS                                     (HWREG16(0x40004C28))           /*!< Port B Drive Strength */
#define PBSEL0                                   (HWREG16(0x40004C2A))           /*!< Port B Select 0 */
#define PBSEL1                                   (HWREG16(0x40004C2C))           /*!< Port B Select 1 */
#define P3IV                                     (HWREG16(0x40004C2E))           /*!< Port 3 Interrupt Vector Register */
#define PBSELC                                   (HWREG16(0x40004C36))           /*!< Port B Complement Select */
#define PBIES                                    (HWREG16(0x40004C38))           /*!< Port B Interrupt Edge Select */
#define PBIE                                     (HWREG16(0x40004C3A))           /*!< Port B Interrupt Enable */
#define PBIFG                                    (HWREG16(0x40004C3C))           /*!< Port B Interrupt Flag */
#define P4IV                                     (HWREG16(0x40004C3E))           /*!< Port 4 Interrupt Vector Register */
#define PCIN                                     (HWREG16(0x40004C40))           /*!< Port C Input */
#define PCOUT                                    (HWREG16(0x40004C42))           /*!< Port C Output */
#define PCDIR                                    (HWREG16(0x40004C44))           /*!< Port C Direction */
#define PCREN                                    (HWREG16(0x40004C46))           /*!< Port C Resistor Enable */
#define PCDS                                     (HWREG16(0x40004C48))           /*!< Port C Drive Strength */
#define PCSEL0                                   (HWREG16(0x40004C4A))           /*!< Port C Select 0 */
#define PCSEL1                                   (HWREG16(0x40004C4C))           /*!< Port C Select 1 */
#define P5IV                                     (HWREG16(0x40004C4E))           /*!< Port 5 Interrupt Vector Register */
#define PCSELC                                   (HWREG16(0x40004C56))           /*!< Port C Complement Select */
#define PCIES                                    (HWREG16(0x40004C58))           /*!< Port C Interrupt Edge Select */
#define PCIE                                     (HWREG16(0x40004C5A))           /*!< Port C Interrupt Enable */
#define PCIFG                                    (HWREG16(0x40004C5C))           /*!< Port C Interrupt Flag */
#define P6IV                                     (HWREG16(0x40004C5E))           /*!< Port 6 Interrupt Vector Register */
#define PDIN                                     (HWREG16(0x40004C60))           /*!< Port D Input */
#define PDOUT                                    (HWREG16(0x40004C62))           /*!< Port D Output */
#define PDDIR                                    (HWREG16(0x40004C64))           /*!< Port D Direction */
#define PDREN                                    (HWREG16(0x40004C66))           /*!< Port D Resistor Enable */
#define PDDS                                     (HWREG16(0x40004C68))           /*!< Port D Drive Strength */
#define PDSEL0                                   (HWREG16(0x40004C6A))           /*!< Port D Select 0 */
#define PDSEL1                                   (HWREG16(0x40004C6C))           /*!< Port D Select 1 */
#define P7IV                                     (HWREG16(0x40004C6E))           /*!< Port 7 Interrupt Vector Register */
#define PDSELC                                   (HWREG16(0x40004C76))           /*!< Port D Complement Select */
#define PDIES                                    (HWREG16(0x40004C78))           /*!< Port D Interrupt Edge Select */
#define PDIE                                     (HWREG16(0x40004C7A))           /*!< Port D Interrupt Enable */
#define PDIFG                                    (HWREG16(0x40004C7C))           /*!< Port D Interrupt Flag */
#define P8IV                                     (HWREG16(0x40004C7E))           /*!< Port 8 Interrupt Vector Register */
#define PEIN                                     (HWREG16(0x40004C80))           /*!< Port E Input */
#define PEOUT                                    (HWREG16(0x40004C82))           /*!< Port E Output */
#define PEDIR                                    (HWREG16(0x40004C84))           /*!< Port E Direction */
#define PEREN                                    (HWREG16(0x40004C86))           /*!< Port E Resistor Enable */
#define PEDS                                     (HWREG16(0x40004C88))           /*!< Port E Drive Strength */
#define PESEL0                                   (HWREG16(0x40004C8A))           /*!< Port E Select 0 */
#define PESEL1                                   (HWREG16(0x40004C8C))           /*!< Port E Select 1 */
#define P9IV                                     (HWREG16(0x40004C8E))           /*!< Port 9 Interrupt Vector Register */
#define PESELC                                   (HWREG16(0x40004C96))           /*!< Port E Complement Select */
#define PEIES                                    (HWREG16(0x40004C98))           /*!< Port E Interrupt Edge Select */
#define PEIE                                     (HWREG16(0x40004C9A))           /*!< Port E Interrupt Enable */
#define PEIFG                                    (HWREG16(0x40004C9C))           /*!< Port E Interrupt Flag */
#define P10IV                                    (HWREG16(0x40004C9E))           /*!< Port 10 Interrupt Vector Register */
#define PJIN                                     (HWREG16(0x40004D20))           /*!< Port J Input */
#define PJOUT                                    (HWREG16(0x40004D22))           /*!< Port J Output */
#define PJDIR                                    (HWREG16(0x40004D24))           /*!< Port J Direction */
#define PJREN                                    (HWREG16(0x40004D26))           /*!< Port J Resistor Enable */
#define PJDS                                     (HWREG16(0x40004D28))           /*!< Port J Drive Strength */
#define PJSEL0                                   (HWREG16(0x40004D2A))           /*!< Port J Select 0 */
#define PJSEL1                                   (HWREG16(0x40004D2C))           /*!< Port J Select 1 */
#define PJSELC                                   (HWREG16(0x40004D36))           /*!< Port J Complement Select */
#define P1IN                                     (HWREG8(0x40004C00))            /*!< Port 1 Input */
#define P2IN                                     (HWREG8(0x40004C01))            /*!< Port 2 Input */
#define P2OUT                                    (HWREG8(0x40004C03))            /*!< Port 2 Output */
#define P1OUT                                    (HWREG8(0x40004C02))            /*!< Port 1 Output */
#define P1DIR                                    (HWREG8(0x40004C04))            /*!< Port 1 Direction */
#define P2DIR                                    (HWREG8(0x40004C05))            /*!< Port 2 Direction */
#define P1REN                                    (HWREG8(0x40004C06))            /*!< Port 1 Resistor Enable */
#define P2REN                                    (HWREG8(0x40004C07))            /*!< Port 2 Resistor Enable */
#define P1DS                                     (HWREG8(0x40004C08))            /*!< Port 1 Drive Strength */
#define P2DS                                     (HWREG8(0x40004C09))            /*!< Port 2 Drive Strength */
#define P1SEL0                                   (HWREG8(0x40004C0A))            /*!< Port 1 Select 0 */
#define P2SEL0                                   (HWREG8(0x40004C0B))            /*!< Port 2 Select 0 */
#define P1SEL1                                   (HWREG8(0x40004C0C))            /*!< Port 1 Select 1 */
#define P2SEL1                                   (HWREG8(0x40004C0D))            /*!< Port 2 Select 1 */
#define P1SELC                                   (HWREG8(0x40004C16))            /*!< Port 1 Complement Select */
#define P2SELC                                   (HWREG8(0x40004C17))            /*!< Port 2 Complement Select */
#define P1IES                                    (HWREG8(0x40004C18))            /*!< Port 1 Interrupt Edge Select */
#define P2IES                                    (HWREG8(0x40004C19))            /*!< Port 2 Interrupt Edge Select */
#define P1IE                                     (HWREG8(0x40004C1A))            /*!< Port 1 Interrupt Enable */
#define P2IE                                     (HWREG8(0x40004C1B))            /*!< Port 2 Interrupt Enable */
#define P1IFG                                    (HWREG8(0x40004C1C))            /*!< Port 1 Interrupt Flag */
#define P2IFG                                    (HWREG8(0x40004C1D))            /*!< Port 2 Interrupt Flag */
#define P3IN                                     (HWREG8(0x40004C20))            /*!< Port 3 Input */
#define P4IN                                     (HWREG8(0x40004C21))            /*!< Port 4 Input */
#define P3OUT                                    (HWREG8(0x40004C22))            /*!< Port 3 Output */
#define P4OUT                                    (HWREG8(0x40004C23))            /*!< Port 4 Output */
#define P3DIR                                    (HWREG8(0x40004C24))            /*!< Port 3 Direction */
#define P4DIR                                    (HWREG8(0x40004C25))            /*!< Port 4 Direction */
#define P3REN                                    (HWREG8(0x40004C26))            /*!< Port 3 Resistor Enable */
#define P4REN                                    (HWREG8(0x40004C27))            /*!< Port 4 Resistor Enable */
#define P3DS                                     (HWREG8(0x40004C28))            /*!< Port 3 Drive Strength */
#define P4DS                                     (HWREG8(0x40004C29))            /*!< Port 4 Drive Strength */
#define P4SEL0                                   (HWREG8(0x40004C2B))            /*!< Port 4 Select 0 */
#define P3SEL0                                   (HWREG8(0x40004C2A))            /*!< Port 3 Select 0 */
#define P3SEL1                                   (HWREG8(0x40004C2C))            /*!< Port 3 Select 1 */
#define P4SEL1                                   (HWREG8(0x40004C2D))            /*!< Port 4 Select 1 */
#define P3SELC                                   (HWREG8(0x40004C36))            /*!< Port 3 Complement Select */
#define P4SELC                                   (HWREG8(0x40004C37))            /*!< Port 4 Complement Select */
#define P3IES                                    (HWREG8(0x40004C38))            /*!< Port 3 Interrupt Edge Select */
#define P4IES                                    (HWREG8(0x40004C39))            /*!< Port 4 Interrupt Edge Select */
#define P3IE                                     (HWREG8(0x40004C3A))            /*!< Port 3 Interrupt Enable */
#define P4IE                                     (HWREG8(0x40004C3B))            /*!< Port 4 Interrupt Enable */
#define P3IFG                                    (HWREG8(0x40004C3C))            /*!< Port 3 Interrupt Flag */
#define P4IFG                                    (HWREG8(0x40004C3D))            /*!< Port 4 Interrupt Flag */
#define P5IN                                     (HWREG8(0x40004C40))            /*!< Port 5 Input */
#define P6IN                                     (HWREG8(0x40004C41))            /*!< Port 6 Input */
#define P5OUT                                    (HWREG8(0x40004C42))            /*!< Port 5 Output */
#define P6OUT                                    (HWREG8(0x40004C43))            /*!< Port 6 Output */
#define P5DIR                                    (HWREG8(0x40004C44))            /*!< Port 5 Direction */
#define P6DIR                                    (HWREG8(0x40004C45))            /*!< Port 6 Direction */
#define P5REN                                    (HWREG8(0x40004C46))            /*!< Port 5 Resistor Enable */
#define P6REN                                    (HWREG8(0x40004C47))            /*!< Port 6 Resistor Enable */
#define P5DS                                     (HWREG8(0x40004C48))            /*!< Port 5 Drive Strength */
#define P6DS                                     (HWREG8(0x40004C49))            /*!< Port 6 Drive Strength */
#define P5SEL0                                   (HWREG8(0x40004C4A))            /*!< Port 5 Select 0 */
#define P6SEL0                                   (HWREG8(0x40004C4B))            /*!< Port 6 Select 0 */
#define P5SEL1                                   (HWREG8(0x40004C4C))            /*!< Port 5 Select 1 */
#define P6SEL1                                   (HWREG8(0x40004C4D))            /*!< Port 6 Select 1 */
#define P5SELC                                   (HWREG8(0x40004C56))            /*!< Port 5 Complement Select */
#define P6SELC                                   (HWREG8(0x40004C57))            /*!< Port 6 Complement Select */
#define P5IES                                    (HWREG8(0x40004C58))            /*!< Port 5 Interrupt Edge Select */
#define P6IES                                    (HWREG8(0x40004C59))            /*!< Port 6 Interrupt Edge Select */
#define P5IE                                     (HWREG8(0x40004C5A))            /*!< Port 5 Interrupt Enable */
#define P6IE                                     (HWREG8(0x40004C5B))            /*!< Port 6 Interrupt Enable */
#define P5IFG                                    (HWREG8(0x40004C5C))            /*!< Port 5 Interrupt Flag */
#define P6IFG                                    (HWREG8(0x40004C5D))            /*!< Port 6 Interrupt Flag */
#define P7IN                                     (HWREG8(0x40004C60))            /*!< Port 7 Input */
#define P8IN                                     (HWREG8(0x40004C61))            /*!< Port 8 Input */
#define P7OUT                                    (HWREG8(0x40004C62))            /*!< Port 7 Output */
#define P8OUT                                    (HWREG8(0x40004C63))            /*!< Port 8 Output */
#define P7DIR                                    (HWREG8(0x40004C64))            /*!< Port 7 Direction */
#define P8DIR                                    (HWREG8(0x40004C65))            /*!< Port 8 Direction */
#define P7REN                                    (HWREG8(0x40004C66))            /*!< Port 7 Resistor Enable */
#define P8REN                                    (HWREG8(0x40004C67))            /*!< Port 8 Resistor Enable */
#define P7DS                                     (HWREG8(0x40004C68))            /*!< Port 7 Drive Strength */
#define P8DS                                     (HWREG8(0x40004C69))            /*!< Port 8 Drive Strength */
#define P7SEL0                                   (HWREG8(0x40004C6A))            /*!< Port 7 Select 0 */
#define P8SEL0                                   (HWREG8(0x40004C6B))            /*!< Port 8 Select 0 */
#define P7SEL1                                   (HWREG8(0x40004C6C))            /*!< Port 7 Select 1 */
#define P8SEL1                                   (HWREG8(0x40004C6D))            /*!< Port 8 Select 1 */
#define P7SELC                                   (HWREG8(0x40004C76))            /*!< Port 7 Complement Select */
#define P8SELC                                   (HWREG8(0x40004C77))            /*!< Port 8 Complement Select */
#define P7IES                                    (HWREG8(0x40004C78))            /*!< Port 7 Interrupt Edge Select */
#define P8IES                                    (HWREG8(0x40004C79))            /*!< Port 8 Interrupt Edge Select */
#define P7IE                                     (HWREG8(0x40004C7A))            /*!< Port 7 Interrupt Enable */
#define P8IE                                     (HWREG8(0x40004C7B))            /*!< Port 8 Interrupt Enable */
#define P7IFG                                    (HWREG8(0x40004C7C))            /*!< Port 7 Interrupt Flag */
#define P8IFG                                    (HWREG8(0x40004C7D))            /*!< Port 8 Interrupt Flag */
#define P9IN                                     (HWREG8(0x40004C80))            /*!< Port 9 Input */
#define P10IN                                    (HWREG8(0x40004C81))            /*!< Port 10 Input */
#define P9OUT                                    (HWREG8(0x40004C82))            /*!< Port 9 Output */
#define P10OUT                                   (HWREG8(0x40004C83))            /*!< Port 10 Output */
#define P9DIR                                    (HWREG8(0x40004C84))            /*!< Port 9 Direction */
#define P10DIR                                   (HWREG8(0x40004C85))            /*!< Port 10 Direction */
#define P9REN                                    (HWREG8(0x40004C86))            /*!< Port 9 Resistor Enable */
#define P10REN                                   (HWREG8(0x40004C87))            /*!< Port 10 Resistor Enable */
#define P9DS                                     (HWREG8(0x40004C88))            /*!< Port 9 Drive Strength */
#define P10DS                                    (HWREG8(0x40004C89))            /*!< Port 10 Drive Strength */
#define P9SEL0                                   (HWREG8(0x40004C8A))            /*!< Port 9 Select 0 */
#define P10SEL0                                  (HWREG8(0x40004C8B))            /*!< Port 10 Select 0 */
#define P9SEL1                                   (HWREG8(0x40004C8C))            /*!< Port 9 Select 1 */
#define P10SEL1                                  (HWREG8(0x40004C8D))            /*!< Port 10 Select 1 */
#define P9SELC                                   (HWREG8(0x40004C96))            /*!< Port 9 Complement Select */
#define P10SELC                                  (HWREG8(0x40004C97))            /*!< Port 10 Complement Select */
#define P9IES                                    (HWREG8(0x40004C98))            /*!< Port 9 Interrupt Edge Select */
#define P10IES                                   (HWREG8(0x40004C99))            /*!< Port 10 Interrupt Edge Select */
#define P9IE                                     (HWREG8(0x40004C9A))            /*!< Port 9 Interrupt Enable */
#define P10IE                                    (HWREG8(0x40004C9B))            /*!< Port 10 Interrupt Enable */
#define P9IFG                                    (HWREG8(0x40004C9C))            /*!< Port 9 Interrupt Flag */
#define P10IFG                                   (HWREG8(0x40004C9D))            /*!< Port 10 Interrupt Flag */

/* Register offsets from DIO_BASE address */
#define OFS_PAIN                                           (0x0000)              /*!< Port A Input */
#define OFS_PAOUT                                          (0x0002)              /*!< Port A Output */
#define OFS_PADIR                                          (0x0004)              /*!< Port A Direction */
#define OFS_PAREN                                          (0x0006)              /*!< Port A Resistor Enable */
#define OFS_PADS                                           (0x0008)              /*!< Port A Drive Strength */
#define OFS_PASEL0                                         (0x000A)              /*!< Port A Select 0 */
#define OFS_PASEL1                                         (0x000C)              /*!< Port A Select 1 */
#define OFS_P1IV                                           (0x000E)              /*!< Port 1 Interrupt Vector Register */
#define OFS_PASELC                                         (0x0016)              /*!< Port A Complement Select */
#define OFS_PAIES                                          (0x0018)              /*!< Port A Interrupt Edge Select */
#define OFS_PAIE                                           (0x001A)              /*!< Port A Interrupt Enable */
#define OFS_PAIFG                                          (0x001C)              /*!< Port A Interrupt Flag */
#define OFS_P2IV                                           (0x001E)              /*!< Port 2 Interrupt Vector Register */
#define OFS_PBIN                                           (0x0020)              /*!< Port B Input */
#define OFS_PBOUT                                          (0x0022)              /*!< Port B Output */
#define OFS_PBDIR                                          (0x0024)              /*!< Port B Direction */
#define OFS_PBREN                                          (0x0026)              /*!< Port B Resistor Enable */
#define OFS_PBDS                                           (0x0028)              /*!< Port B Drive Strength */
#define OFS_PBSEL0                                         (0x002A)              /*!< Port B Select 0 */
#define OFS_PBSEL1                                         (0x002C)              /*!< Port B Select 1 */
#define OFS_P3IV                                           (0x002E)              /*!< Port 3 Interrupt Vector Register */
#define OFS_PBSELC                                         (0x0036)              /*!< Port B Complement Select */
#define OFS_PBIES                                          (0x0038)              /*!< Port B Interrupt Edge Select */
#define OFS_PBIE                                           (0x003A)              /*!< Port B Interrupt Enable */
#define OFS_PBIFG                                          (0x003C)              /*!< Port B Interrupt Flag */
#define OFS_P4IV                                           (0x003E)              /*!< Port 4 Interrupt Vector Register */
#define OFS_PCIN                                           (0x0040)              /*!< Port C Input */
#define OFS_PCOUT                                          (0x0042)              /*!< Port C Output */
#define OFS_PCDIR                                          (0x0044)              /*!< Port C Direction */
#define OFS_PCREN                                          (0x0046)              /*!< Port C Resistor Enable */
#define OFS_PCDS                                           (0x0048)              /*!< Port C Drive Strength */
#define OFS_PCSEL0                                         (0x004A)              /*!< Port C Select 0 */
#define OFS_PCSEL1                                         (0x004C)              /*!< Port C Select 1 */
#define OFS_P5IV                                           (0x004E)              /*!< Port 5 Interrupt Vector Register */
#define OFS_PCSELC                                         (0x0056)              /*!< Port C Complement Select */
#define OFS_PCIES                                          (0x0058)              /*!< Port C Interrupt Edge Select */
#define OFS_PCIE                                           (0x005A)              /*!< Port C Interrupt Enable */
#define OFS_PCIFG                                          (0x005C)              /*!< Port C Interrupt Flag */
#define OFS_P6IV                                           (0x005E)              /*!< Port 6 Interrupt Vector Register */
#define OFS_PDIN                                           (0x0060)              /*!< Port D Input */
#define OFS_PDOUT                                          (0x0062)              /*!< Port D Output */
#define OFS_PDDIR                                          (0x0064)              /*!< Port D Direction */
#define OFS_PDREN                                          (0x0066)              /*!< Port D Resistor Enable */
#define OFS_PDDS                                           (0x0068)              /*!< Port D Drive Strength */
#define OFS_PDSEL0                                         (0x006A)              /*!< Port D Select 0 */
#define OFS_PDSEL1                                         (0x006C)              /*!< Port D Select 1 */
#define OFS_P7IV                                           (0x006E)              /*!< Port 7 Interrupt Vector Register */
#define OFS_PDSELC                                         (0x0076)              /*!< Port D Complement Select */
#define OFS_PDIES                                          (0x0078)              /*!< Port D Interrupt Edge Select */
#define OFS_PDIE                                           (0x007A)              /*!< Port D Interrupt Enable */
#define OFS_PDIFG                                          (0x007C)              /*!< Port D Interrupt Flag */
#define OFS_P8IV                                           (0x007E)              /*!< Port 8 Interrupt Vector Register */
#define OFS_PEIN                                           (0x0080)              /*!< Port E Input */
#define OFS_PEOUT                                          (0x0082)              /*!< Port E Output */
#define OFS_PEDIR                                          (0x0084)              /*!< Port E Direction */
#define OFS_PEREN                                          (0x0086)              /*!< Port E Resistor Enable */
#define OFS_PEDS                                           (0x0088)              /*!< Port E Drive Strength */
#define OFS_PESEL0                                         (0x008A)              /*!< Port E Select 0 */
#define OFS_PESEL1                                         (0x008C)              /*!< Port E Select 1 */
#define OFS_P9IV                                           (0x008E)              /*!< Port 9 Interrupt Vector Register */
#define OFS_PESELC                                         (0x0096)              /*!< Port E Complement Select */
#define OFS_PEIES                                          (0x0098)              /*!< Port E Interrupt Edge Select */
#define OFS_PEIE                                           (0x009A)              /*!< Port E Interrupt Enable */
#define OFS_PEIFG                                          (0x009C)              /*!< Port E Interrupt Flag */
#define OFS_P10IV                                          (0x009E)              /*!< Port 10 Interrupt Vector Register */
#define OFS_PJIN                                           (0x0120)              /*!< Port J Input */
#define OFS_PJOUT                                          (0x0122)              /*!< Port J Output */
#define OFS_PJDIR                                          (0x0124)              /*!< Port J Direction */
#define OFS_PJREN                                          (0x0126)              /*!< Port J Resistor Enable */
#define OFS_PJDS                                           (0x0128)              /*!< Port J Drive Strength */
#define OFS_PJSEL0                                         (0x012A)              /*!< Port J Select 0 */
#define OFS_PJSEL1                                         (0x012C)              /*!< Port J Select 1 */
#define OFS_PJSELC                                         (0x0136)              /*!< Port J Complement Select */
#define OFS_P1IN                                           (0x0000)              /*!< Port 1 Input */
#define OFS_P2IN                                           (0x0001)              /*!< Port 2 Input */
#define OFS_P2OUT                                          (0x0003)              /*!< Port 2 Output */
#define OFS_P1OUT                                          (0x0002)              /*!< Port 1 Output */
#define OFS_P1DIR                                          (0x0004)              /*!< Port 1 Direction */
#define OFS_P2DIR                                          (0x0005)              /*!< Port 2 Direction */
#define OFS_P1REN                                          (0x0006)              /*!< Port 1 Resistor Enable */
#define OFS_P2REN                                          (0x0007)              /*!< Port 2 Resistor Enable */
#define OFS_P1DS                                           (0x0008)              /*!< Port 1 Drive Strength */
#define OFS_P2DS                                           (0x0009)              /*!< Port 2 Drive Strength */
#define OFS_P1SEL0                                         (0x000A)              /*!< Port 1 Select 0 */
#define OFS_P2SEL0                                         (0x000B)              /*!< Port 2 Select 0 */
#define OFS_P1SEL1                                         (0x000C)              /*!< Port 1 Select 1 */
#define OFS_P2SEL1                                         (0x000D)              /*!< Port 2 Select 1 */
#define OFS_P1SELC                                         (0x0016)              /*!< Port 1 Complement Select */
#define OFS_P2SELC                                         (0x0017)              /*!< Port 2 Complement Select */
#define OFS_P1IES                                          (0x0018)              /*!< Port 1 Interrupt Edge Select */
#define OFS_P2IES                                          (0x0019)              /*!< Port 2 Interrupt Edge Select */
#define OFS_P1IE                                           (0x001A)              /*!< Port 1 Interrupt Enable */
#define OFS_P2IE                                           (0x001B)              /*!< Port 2 Interrupt Enable */
#define OFS_P1IFG                                          (0x001C)              /*!< Port 1 Interrupt Flag */
#define OFS_P2IFG                                          (0x001D)              /*!< Port 2 Interrupt Flag */
#define OFS_P3IN                                           (0x0020)              /*!< Port 3 Input */
#define OFS_P4IN                                           (0x0021)              /*!< Port 4 Input */
#define OFS_P3OUT                                          (0x0022)              /*!< Port 3 Output */
#define OFS_P4OUT                                          (0x0023)              /*!< Port 4 Output */
#define OFS_P3DIR                                          (0x0024)              /*!< Port 3 Direction */
#define OFS_P4DIR                                          (0x0025)              /*!< Port 4 Direction */
#define OFS_P3REN                                          (0x0026)              /*!< Port 3 Resistor Enable */
#define OFS_P4REN                                          (0x0027)              /*!< Port 4 Resistor Enable */
#define OFS_P3DS                                           (0x0028)              /*!< Port 3 Drive Strength */
#define OFS_P4DS                                           (0x0029)              /*!< Port 4 Drive Strength */
#define OFS_P4SEL0                                         (0x002B)              /*!< Port 4 Select 0 */
#define OFS_P3SEL0                                         (0x002A)              /*!< Port 3 Select 0 */
#define OFS_P3SEL1                                         (0x002C)              /*!< Port 3 Select 1 */
#define OFS_P4SEL1                                         (0x002D)              /*!< Port 4 Select 1 */
#define OFS_P3SELC                                         (0x0036)              /*!< Port 3 Complement Select */
#define OFS_P4SELC                                         (0x0037)              /*!< Port 4 Complement Select */
#define OFS_P3IES                                          (0x0038)              /*!< Port 3 Interrupt Edge Select */
#define OFS_P4IES                                          (0x0039)              /*!< Port 4 Interrupt Edge Select */
#define OFS_P3IE                                           (0x003A)              /*!< Port 3 Interrupt Enable */
#define OFS_P4IE                                           (0x003B)              /*!< Port 4 Interrupt Enable */
#define OFS_P3IFG                                          (0x003C)              /*!< Port 3 Interrupt Flag */
#define OFS_P4IFG                                          (0x003D)              /*!< Port 4 Interrupt Flag */
#define OFS_P5IN                                           (0x0040)              /*!< Port 5 Input */
#define OFS_P6IN                                           (0x0041)              /*!< Port 6 Input */
#define OFS_P5OUT                                          (0x0042)              /*!< Port 5 Output */
#define OFS_P6OUT                                          (0x0043)              /*!< Port 6 Output */
#define OFS_P5DIR                                          (0x0044)              /*!< Port 5 Direction */
#define OFS_P6DIR                                          (0x0045)              /*!< Port 6 Direction */
#define OFS_P5REN                                          (0x0046)              /*!< Port 5 Resistor Enable */
#define OFS_P6REN                                          (0x0047)              /*!< Port 6 Resistor Enable */
#define OFS_P5DS                                           (0x0048)              /*!< Port 5 Drive Strength */
#define OFS_P6DS                                           (0x0049)              /*!< Port 6 Drive Strength */
#define OFS_P5SEL0                                         (0x004A)              /*!< Port 5 Select 0 */
#define OFS_P6SEL0                                         (0x004B)              /*!< Port 6 Select 0 */
#define OFS_P5SEL1                                         (0x004C)              /*!< Port 5 Select 1 */
#define OFS_P6SEL1                                         (0x004D)              /*!< Port 6 Select 1 */
#define OFS_P5SELC                                         (0x0056)              /*!< Port 5 Complement Select */
#define OFS_P6SELC                                         (0x0057)              /*!< Port 6 Complement Select */
#define OFS_P5IES                                          (0x0058)              /*!< Port 5 Interrupt Edge Select */
#define OFS_P6IES                                          (0x0059)              /*!< Port 6 Interrupt Edge Select */
#define OFS_P5IE                                           (0x005A)              /*!< Port 5 Interrupt Enable */
#define OFS_P6IE                                           (0x005B)              /*!< Port 6 Interrupt Enable */
#define OFS_P5IFG                                          (0x005C)              /*!< Port 5 Interrupt Flag */
#define OFS_P6IFG                                          (0x005D)              /*!< Port 6 Interrupt Flag */
#define OFS_P7IN                                           (0x0060)              /*!< Port 7 Input */
#define OFS_P8IN                                           (0x0061)              /*!< Port 8 Input */
#define OFS_P7OUT                                          (0x0062)              /*!< Port 7 Output */
#define OFS_P8OUT                                          (0x0063)              /*!< Port 8 Output */
#define OFS_P7DIR                                          (0x0064)              /*!< Port 7 Direction */
#define OFS_P8DIR                                          (0x0065)              /*!< Port 8 Direction */
#define OFS_P7REN                                          (0x0066)              /*!< Port 7 Resistor Enable */
#define OFS_P8REN                                          (0x0067)              /*!< Port 8 Resistor Enable */
#define OFS_P7DS                                           (0x0068)              /*!< Port 7 Drive Strength */
#define OFS_P8DS                                           (0x0069)              /*!< Port 8 Drive Strength */
#define OFS_P7SEL0                                         (0x006A)              /*!< Port 7 Select 0 */
#define OFS_P8SEL0                                         (0x006B)              /*!< Port 8 Select 0 */
#define OFS_P7SEL1                                         (0x006C)              /*!< Port 7 Select 1 */
#define OFS_P8SEL1                                         (0x006D)              /*!< Port 8 Select 1 */
#define OFS_P7SELC                                         (0x0076)              /*!< Port 7 Complement Select */
#define OFS_P8SELC                                         (0x0077)              /*!< Port 8 Complement Select */
#define OFS_P7IES                                          (0x0078)              /*!< Port 7 Interrupt Edge Select */
#define OFS_P8IES                                          (0x0079)              /*!< Port 8 Interrupt Edge Select */
#define OFS_P7IE                                           (0x007A)              /*!< Port 7 Interrupt Enable */
#define OFS_P8IE                                           (0x007B)              /*!< Port 8 Interrupt Enable */
#define OFS_P7IFG                                          (0x007C)              /*!< Port 7 Interrupt Flag */
#define OFS_P8IFG                                          (0x007D)              /*!< Port 8 Interrupt Flag */
#define OFS_P9IN                                           (0x0080)              /*!< Port 9 Input */
#define OFS_P10IN                                          (0x0081)              /*!< Port 10 Input */
#define OFS_P9OUT                                          (0x0082)              /*!< Port 9 Output */
#define OFS_P10OUT                                         (0x0083)              /*!< Port 10 Output */
#define OFS_P9DIR                                          (0x0084)              /*!< Port 9 Direction */
#define OFS_P10DIR                                         (0x0085)              /*!< Port 10 Direction */
#define OFS_P9REN                                          (0x0086)              /*!< Port 9 Resistor Enable */
#define OFS_P10REN                                         (0x0087)              /*!< Port 10 Resistor Enable */
#define OFS_P9DS                                           (0x0088)              /*!< Port 9 Drive Strength */
#define OFS_P10DS                                          (0x0089)              /*!< Port 10 Drive Strength */
#define OFS_P9SEL0                                         (0x008A)              /*!< Port 9 Select 0 */
#define OFS_P10SEL0                                        (0x008B)              /*!< Port 10 Select 0 */
#define OFS_P9SEL1                                         (0x008C)              /*!< Port 9 Select 1 */
#define OFS_P10SEL1                                        (0x008D)              /*!< Port 10 Select 1 */
#define OFS_P9SELC                                         (0x0096)              /*!< Port 9 Complement Select */
#define OFS_P10SELC                                        (0x0097)              /*!< Port 10 Complement Select */
#define OFS_P9IES                                          (0x0098)              /*!< Port 9 Interrupt Edge Select */
#define OFS_P10IES                                         (0x0099)              /*!< Port 10 Interrupt Edge Select */
#define OFS_P9IE                                           (0x009A)              /*!< Port 9 Interrupt Enable */
#define OFS_P10IE                                          (0x009B)              /*!< Port 10 Interrupt Enable */
#define OFS_P9IFG                                          (0x009C)              /*!< Port 9 Interrupt Flag */
#define OFS_P10IFG                                         (0x009D)              /*!< Port 10 Interrupt Flag */


/******************************************************************************
* EUSCI_A0 Registers
******************************************************************************/
#define UCA0CTLW0                                (HWREG16(0x40001000))           /*!< eUSCI_Ax Control Word Register 0 */
#define UCA0CTLW0_SPI                            (HWREG16(0x40001000))           
#define UCA0CTLW1                                (HWREG16(0x40001002))           /*!< eUSCI_Ax Control Word Register 1 */
#define UCA0BRW                                  (HWREG16(0x40001006))           /*!< eUSCI_Ax Baud Rate Control Word Register */
#define UCA0BRW_SPI                              (HWREG16(0x40001006))           
#define UCA0MCTLW                                (HWREG16(0x40001008))           /*!< eUSCI_Ax Modulation Control Word Register */
#define UCA0STATW                                (HWREG16(0x4000100A))           /*!< eUSCI_Ax Status Register */
#define UCA0STATW_SPI                            (HWREG16(0x4000100A))           
#define UCA0RXBUF                                (HWREG16(0x4000100C))           /*!< eUSCI_Ax Receive Buffer Register */
#define UCA0RXBUF_SPI                            (HWREG16(0x4000100C))           
#define UCA0TXBUF                                (HWREG16(0x4000100E))           /*!< eUSCI_Ax Transmit Buffer Register */
#define UCA0TXBUF_SPI                            (HWREG16(0x4000100E))           
#define UCA0ABCTL                                (HWREG16(0x40001010))           /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define UCA0IRCTL                                (HWREG16(0x40001012))           /*!< eUSCI_Ax IrDA Control Word Register */
#define UCA0IE                                   (HWREG16(0x4000101A))           /*!< eUSCI_Ax Interrupt Enable Register */
#define UCA0IE_SPI                               (HWREG16(0x4000101A))           
#define UCA0IFG                                  (HWREG16(0x4000101C))           /*!< eUSCI_Ax Interrupt Flag Register */
#define UCA0IFG_SPI                              (HWREG16(0x4000101C))           
#define UCA0IV                                   (HWREG16(0x4000101E))           /*!< eUSCI_Ax Interrupt Vector Register */
#define UCA0IV_SPI                               (HWREG16(0x4000101E))           

/* Register offsets from EUSCI_A0_BASE address */
#define OFS_UCA0CTLW0                                      (0x0000)              /*!< eUSCI_Ax Control Word Register 0 */
#define OFS_UCA0CTLW0_SPI                                  (0x0000)              
#define OFS_UCA0CTLW1                                      (0x0002)              /*!< eUSCI_Ax Control Word Register 1 */
#define OFS_UCA0BRW                                        (0x0006)              /*!< eUSCI_Ax Baud Rate Control Word Register */
#define OFS_UCA0BRW_SPI                                    (0x0006)              
#define OFS_UCA0MCTLW                                      (0x0008)              /*!< eUSCI_Ax Modulation Control Word Register */
#define OFS_UCA0STATW                                      (0x000A)              /*!< eUSCI_Ax Status Register */
#define OFS_UCA0STATW_SPI                                  (0x000A)              
#define OFS_UCA0RXBUF                                      (0x000C)              /*!< eUSCI_Ax Receive Buffer Register */
#define OFS_UCA0RXBUF_SPI                                  (0x000C)              
#define OFS_UCA0TXBUF                                      (0x000E)              /*!< eUSCI_Ax Transmit Buffer Register */
#define OFS_UCA0TXBUF_SPI                                  (0x000E)              
#define OFS_UCA0ABCTL                                      (0x0010)              /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define OFS_UCA0IRCTL                                      (0x0012)              /*!< eUSCI_Ax IrDA Control Word Register */
#define OFS_UCA0IE                                         (0x001A)              /*!< eUSCI_Ax Interrupt Enable Register */
#define OFS_UCA0IE_SPI                                     (0x001A)              
#define OFS_UCA0IFG                                        (0x001C)              /*!< eUSCI_Ax Interrupt Flag Register */
#define OFS_UCA0IFG_SPI                                    (0x001C)              
#define OFS_UCA0IV                                         (0x001E)              /*!< eUSCI_Ax Interrupt Vector Register */
#define OFS_UCA0IV_SPI                                     (0x001E)              

#define UCA0CTL0                                           (HWREG8_L(UCA0CTLW0)) /* eUSCI_Ax Control 0 */
#define UCA0CTL1                                           (HWREG8_H(UCA0CTLW0)) /* eUSCI_Ax Control 1 */
#define UCA0BR0                                            (HWREG8_L(UCA0BRW))   /* eUSCI_Ax Baud Rate Control 0 */
#define UCA0BR1                                            (HWREG8_H(UCA0BRW))   /* eUSCI_Ax Baud Rate Control 1 */
#define UCA0IRTCTL                                         (HWREG8_L(UCA0IRCTL)) /* eUSCI_Ax IrDA Transmit Control */
#define UCA0IRRCTL                                         (HWREG8_H(UCA0IRCTL)) /* eUSCI_Ax IrDA Receive Control */

/******************************************************************************
* EUSCI_A1 Registers
******************************************************************************/
#define UCA1CTLW0                                (HWREG16(0x40001400))           /*!< eUSCI_Ax Control Word Register 0 */
#define UCA1CTLW0_SPI                            (HWREG16(0x40001400))           
#define UCA1CTLW1                                (HWREG16(0x40001402))           /*!< eUSCI_Ax Control Word Register 1 */
#define UCA1BRW                                  (HWREG16(0x40001406))           /*!< eUSCI_Ax Baud Rate Control Word Register */
#define UCA1BRW_SPI                              (HWREG16(0x40001406))           
#define UCA1MCTLW                                (HWREG16(0x40001408))           /*!< eUSCI_Ax Modulation Control Word Register */
#define UCA1STATW                                (HWREG16(0x4000140A))           /*!< eUSCI_Ax Status Register */
#define UCA1STATW_SPI                            (HWREG16(0x4000140A))           
#define UCA1RXBUF                                (HWREG16(0x4000140C))           /*!< eUSCI_Ax Receive Buffer Register */
#define UCA1RXBUF_SPI                            (HWREG16(0x4000140C))           
#define UCA1TXBUF                                (HWREG16(0x4000140E))           /*!< eUSCI_Ax Transmit Buffer Register */
#define UCA1TXBUF_SPI                            (HWREG16(0x4000140E))           
#define UCA1ABCTL                                (HWREG16(0x40001410))           /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define UCA1IRCTL                                (HWREG16(0x40001412))           /*!< eUSCI_Ax IrDA Control Word Register */
#define UCA1IE                                   (HWREG16(0x4000141A))           /*!< eUSCI_Ax Interrupt Enable Register */
#define UCA1IE_SPI                               (HWREG16(0x4000141A))           
#define UCA1IFG                                  (HWREG16(0x4000141C))           /*!< eUSCI_Ax Interrupt Flag Register */
#define UCA1IFG_SPI                              (HWREG16(0x4000141C))           
#define UCA1IV                                   (HWREG16(0x4000141E))           /*!< eUSCI_Ax Interrupt Vector Register */
#define UCA1IV_SPI                               (HWREG16(0x4000141E))           

/* Register offsets from EUSCI_A1_BASE address */
#define OFS_UCA1CTLW0                                      (0x0000)              /*!< eUSCI_Ax Control Word Register 0 */
#define OFS_UCA1CTLW0_SPI                                  (0x0000)              
#define OFS_UCA1CTLW1                                      (0x0002)              /*!< eUSCI_Ax Control Word Register 1 */
#define OFS_UCA1BRW                                        (0x0006)              /*!< eUSCI_Ax Baud Rate Control Word Register */
#define OFS_UCA1BRW_SPI                                    (0x0006)              
#define OFS_UCA1MCTLW                                      (0x0008)              /*!< eUSCI_Ax Modulation Control Word Register */
#define OFS_UCA1STATW                                      (0x000A)              /*!< eUSCI_Ax Status Register */
#define OFS_UCA1STATW_SPI                                  (0x000A)              
#define OFS_UCA1RXBUF                                      (0x000C)              /*!< eUSCI_Ax Receive Buffer Register */
#define OFS_UCA1RXBUF_SPI                                  (0x000C)              
#define OFS_UCA1TXBUF                                      (0x000E)              /*!< eUSCI_Ax Transmit Buffer Register */
#define OFS_UCA1TXBUF_SPI                                  (0x000E)              
#define OFS_UCA1ABCTL                                      (0x0010)              /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define OFS_UCA1IRCTL                                      (0x0012)              /*!< eUSCI_Ax IrDA Control Word Register */
#define OFS_UCA1IE                                         (0x001A)              /*!< eUSCI_Ax Interrupt Enable Register */
#define OFS_UCA1IE_SPI                                     (0x001A)              
#define OFS_UCA1IFG                                        (0x001C)              /*!< eUSCI_Ax Interrupt Flag Register */
#define OFS_UCA1IFG_SPI                                    (0x001C)              
#define OFS_UCA1IV                                         (0x001E)              /*!< eUSCI_Ax Interrupt Vector Register */
#define OFS_UCA1IV_SPI                                     (0x001E)              

#define UCA1CTL0                                           (HWREG8_L(UCA1CTLW0)) /* eUSCI_Ax Control 0 */
#define UCA1CTL1                                           (HWREG8_H(UCA1CTLW0)) /* eUSCI_Ax Control 1 */
#define UCA1BR0                                            (HWREG8_L(UCA1BRW))   /* eUSCI_Ax Baud Rate Control 0 */
#define UCA1BR1                                            (HWREG8_H(UCA1BRW))   /* eUSCI_Ax Baud Rate Control 1 */
#define UCA1IRTCTL                                         (HWREG8_L(UCA1IRCTL)) /* eUSCI_Ax IrDA Transmit Control */
#define UCA1IRRCTL                                         (HWREG8_H(UCA1IRCTL)) /* eUSCI_Ax IrDA Receive Control */

/******************************************************************************
* EUSCI_A2 Registers
******************************************************************************/
#define UCA2CTLW0                                (HWREG16(0x40001800))           /*!< eUSCI_Ax Control Word Register 0 */
#define UCA2CTLW0_SPI                            (HWREG16(0x40001800))           
#define UCA2CTLW1                                (HWREG16(0x40001802))           /*!< eUSCI_Ax Control Word Register 1 */
#define UCA2BRW                                  (HWREG16(0x40001806))           /*!< eUSCI_Ax Baud Rate Control Word Register */
#define UCA2BRW_SPI                              (HWREG16(0x40001806))           
#define UCA2MCTLW                                (HWREG16(0x40001808))           /*!< eUSCI_Ax Modulation Control Word Register */
#define UCA2STATW                                (HWREG16(0x4000180A))           /*!< eUSCI_Ax Status Register */
#define UCA2STATW_SPI                            (HWREG16(0x4000180A))           
#define UCA2RXBUF                                (HWREG16(0x4000180C))           /*!< eUSCI_Ax Receive Buffer Register */
#define UCA2RXBUF_SPI                            (HWREG16(0x4000180C))           
#define UCA2TXBUF                                (HWREG16(0x4000180E))           /*!< eUSCI_Ax Transmit Buffer Register */
#define UCA2TXBUF_SPI                            (HWREG16(0x4000180E))           
#define UCA2ABCTL                                (HWREG16(0x40001810))           /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define UCA2IRCTL                                (HWREG16(0x40001812))           /*!< eUSCI_Ax IrDA Control Word Register */
#define UCA2IE                                   (HWREG16(0x4000181A))           /*!< eUSCI_Ax Interrupt Enable Register */
#define UCA2IE_SPI                               (HWREG16(0x4000181A))           
#define UCA2IFG                                  (HWREG16(0x4000181C))           /*!< eUSCI_Ax Interrupt Flag Register */
#define UCA2IFG_SPI                              (HWREG16(0x4000181C))           
#define UCA2IV                                   (HWREG16(0x4000181E))           /*!< eUSCI_Ax Interrupt Vector Register */
#define UCA2IV_SPI                               (HWREG16(0x4000181E))           

/* Register offsets from EUSCI_A2_BASE address */
#define OFS_UCA2CTLW0                                      (0x0000)              /*!< eUSCI_Ax Control Word Register 0 */
#define OFS_UCA2CTLW0_SPI                                  (0x0000)              
#define OFS_UCA2CTLW1                                      (0x0002)              /*!< eUSCI_Ax Control Word Register 1 */
#define OFS_UCA2BRW                                        (0x0006)              /*!< eUSCI_Ax Baud Rate Control Word Register */
#define OFS_UCA2BRW_SPI                                    (0x0006)              
#define OFS_UCA2MCTLW                                      (0x0008)              /*!< eUSCI_Ax Modulation Control Word Register */
#define OFS_UCA2STATW                                      (0x000A)              /*!< eUSCI_Ax Status Register */
#define OFS_UCA2STATW_SPI                                  (0x000A)              
#define OFS_UCA2RXBUF                                      (0x000C)              /*!< eUSCI_Ax Receive Buffer Register */
#define OFS_UCA2RXBUF_SPI                                  (0x000C)              
#define OFS_UCA2TXBUF                                      (0x000E)              /*!< eUSCI_Ax Transmit Buffer Register */
#define OFS_UCA2TXBUF_SPI                                  (0x000E)              
#define OFS_UCA2ABCTL                                      (0x0010)              /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define OFS_UCA2IRCTL                                      (0x0012)              /*!< eUSCI_Ax IrDA Control Word Register */
#define OFS_UCA2IE                                         (0x001A)              /*!< eUSCI_Ax Interrupt Enable Register */
#define OFS_UCA2IE_SPI                                     (0x001A)              
#define OFS_UCA2IFG                                        (0x001C)              /*!< eUSCI_Ax Interrupt Flag Register */
#define OFS_UCA2IFG_SPI                                    (0x001C)              
#define OFS_UCA2IV                                         (0x001E)              /*!< eUSCI_Ax Interrupt Vector Register */
#define OFS_UCA2IV_SPI                                     (0x001E)              

#define UCA2CTL0                                           (HWREG8_L(UCA2CTLW0)) /* eUSCI_Ax Control 0 */
#define UCA2CTL1                                           (HWREG8_H(UCA2CTLW0)) /* eUSCI_Ax Control 1 */
#define UCA2BR0                                            (HWREG8_L(UCA2BRW))   /* eUSCI_Ax Baud Rate Control 0 */
#define UCA2BR1                                            (HWREG8_H(UCA2BRW))   /* eUSCI_Ax Baud Rate Control 1 */
#define UCA2IRTCTL                                         (HWREG8_L(UCA2IRCTL)) /* eUSCI_Ax IrDA Transmit Control */
#define UCA2IRRCTL                                         (HWREG8_H(UCA2IRCTL)) /* eUSCI_Ax IrDA Receive Control */

/******************************************************************************
* EUSCI_A3 Registers
******************************************************************************/
#define UCA3CTLW0                                (HWREG16(0x40001C00))           /*!< eUSCI_Ax Control Word Register 0 */
#define UCA3CTLW0_SPI                            (HWREG16(0x40001C00))           
#define UCA3CTLW1                                (HWREG16(0x40001C02))           /*!< eUSCI_Ax Control Word Register 1 */
#define UCA3BRW                                  (HWREG16(0x40001C06))           /*!< eUSCI_Ax Baud Rate Control Word Register */
#define UCA3BRW_SPI                              (HWREG16(0x40001C06))           
#define UCA3MCTLW                                (HWREG16(0x40001C08))           /*!< eUSCI_Ax Modulation Control Word Register */
#define UCA3STATW                                (HWREG16(0x40001C0A))           /*!< eUSCI_Ax Status Register */
#define UCA3STATW_SPI                            (HWREG16(0x40001C0A))           
#define UCA3RXBUF                                (HWREG16(0x40001C0C))           /*!< eUSCI_Ax Receive Buffer Register */
#define UCA3RXBUF_SPI                            (HWREG16(0x40001C0C))           
#define UCA3TXBUF                                (HWREG16(0x40001C0E))           /*!< eUSCI_Ax Transmit Buffer Register */
#define UCA3TXBUF_SPI                            (HWREG16(0x40001C0E))           
#define UCA3ABCTL                                (HWREG16(0x40001C10))           /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define UCA3IRCTL                                (HWREG16(0x40001C12))           /*!< eUSCI_Ax IrDA Control Word Register */
#define UCA3IE                                   (HWREG16(0x40001C1A))           /*!< eUSCI_Ax Interrupt Enable Register */
#define UCA3IE_SPI                               (HWREG16(0x40001C1A))           
#define UCA3IFG                                  (HWREG16(0x40001C1C))           /*!< eUSCI_Ax Interrupt Flag Register */
#define UCA3IFG_SPI                              (HWREG16(0x40001C1C))           
#define UCA3IV                                   (HWREG16(0x40001C1E))           /*!< eUSCI_Ax Interrupt Vector Register */
#define UCA3IV_SPI                               (HWREG16(0x40001C1E))           

/* Register offsets from EUSCI_A3_BASE address */
#define OFS_UCA3CTLW0                                      (0x0000)              /*!< eUSCI_Ax Control Word Register 0 */
#define OFS_UCA3CTLW0_SPI                                  (0x0000)              
#define OFS_UCA3CTLW1                                      (0x0002)              /*!< eUSCI_Ax Control Word Register 1 */
#define OFS_UCA3BRW                                        (0x0006)              /*!< eUSCI_Ax Baud Rate Control Word Register */
#define OFS_UCA3BRW_SPI                                    (0x0006)              
#define OFS_UCA3MCTLW                                      (0x0008)              /*!< eUSCI_Ax Modulation Control Word Register */
#define OFS_UCA3STATW                                      (0x000A)              /*!< eUSCI_Ax Status Register */
#define OFS_UCA3STATW_SPI                                  (0x000A)              
#define OFS_UCA3RXBUF                                      (0x000C)              /*!< eUSCI_Ax Receive Buffer Register */
#define OFS_UCA3RXBUF_SPI                                  (0x000C)              
#define OFS_UCA3TXBUF                                      (0x000E)              /*!< eUSCI_Ax Transmit Buffer Register */
#define OFS_UCA3TXBUF_SPI                                  (0x000E)              
#define OFS_UCA3ABCTL                                      (0x0010)              /*!< eUSCI_Ax Auto Baud Rate Control Register */
#define OFS_UCA3IRCTL                                      (0x0012)              /*!< eUSCI_Ax IrDA Control Word Register */
#define OFS_UCA3IE                                         (0x001A)              /*!< eUSCI_Ax Interrupt Enable Register */
#define OFS_UCA3IE_SPI                                     (0x001A)              
#define OFS_UCA3IFG                                        (0x001C)              /*!< eUSCI_Ax Interrupt Flag Register */
#define OFS_UCA3IFG_SPI                                    (0x001C)              
#define OFS_UCA3IV                                         (0x001E)              /*!< eUSCI_Ax Interrupt Vector Register */
#define OFS_UCA3IV_SPI                                     (0x001E)              

#define UCA3CTL0                                           (HWREG8_L(UCA3CTLW0)) /* eUSCI_Ax Control 0 */
#define UCA3CTL1                                           (HWREG8_H(UCA3CTLW0)) /* eUSCI_Ax Control 1 */
#define UCA3BR0                                            (HWREG8_L(UCA3BRW))   /* eUSCI_Ax Baud Rate Control 0 */
#define UCA3BR1                                            (HWREG8_H(UCA3BRW))   /* eUSCI_Ax Baud Rate Control 1 */
#define UCA3IRTCTL                                         (HWREG8_L(UCA3IRCTL)) /* eUSCI_Ax IrDA Transmit Control */
#define UCA3IRRCTL                                         (HWREG8_H(UCA3IRCTL)) /* eUSCI_Ax IrDA Receive Control */

/******************************************************************************
* EUSCI_B0 Registers
******************************************************************************/
#define UCB0CTLW0                                (HWREG16(0x40002000))           /*!< eUSCI_Bx Control Word Register 0 */
#define UCB0CTLW0_SPI                            (HWREG16(0x40002000))           
#define UCB0CTLW1                                (HWREG16(0x40002002))           /*!< eUSCI_Bx Control Word Register 1 */
#define UCB0BRW                                  (HWREG16(0x40002006))           /*!< eUSCI_Bx Baud Rate Control Word Register */
#define UCB0BRW_SPI                              (HWREG16(0x40002006))           
#define UCB0STATW                                (HWREG16(0x40002008))           /*!< eUSCI_Bx Status Register */
#define UCB0STATW_SPI                            (HWREG16(0x40002008))           
#define UCB0TBCNT                                (HWREG16(0x4000200A))           /*!< eUSCI_Bx Byte Counter Threshold Register */
#define UCB0RXBUF                                (HWREG16(0x4000200C))           /*!< eUSCI_Bx Receive Buffer Register */
#define UCB0RXBUF_SPI                            (HWREG16(0x4000200C))           
#define UCB0TXBUF                                (HWREG16(0x4000200E))           /*!< eUSCI_Bx Transmit Buffer Register */
#define UCB0TXBUF_SPI                            (HWREG16(0x4000200E))           
#define UCB0I2COA0                               (HWREG16(0x40002014))           /*!< eUSCI_Bx I2C Own Address 0 Register */
#define UCB0I2COA1                               (HWREG16(0x40002016))           /*!< eUSCI_Bx I2C Own Address 1 Register */
#define UCB0I2COA2                               (HWREG16(0x40002018))           /*!< eUSCI_Bx I2C Own Address 2 Register */
#define UCB0I2COA3                               (HWREG16(0x4000201A))           /*!< eUSCI_Bx I2C Own Address 3 Register */
#define UCB0ADDRX                                (HWREG16(0x4000201C))           /*!< eUSCI_Bx I2C Received Address Register */
#define UCB0ADDMASK                              (HWREG16(0x4000201E))           /*!< eUSCI_Bx I2C Address Mask Register */
#define UCB0I2CSA                                (HWREG16(0x40002020))           /*!< eUSCI_Bx I2C Slave Address Register */
#define UCB0IE                                   (HWREG16(0x4000202A))           /*!< eUSCI_Bx Interrupt Enable Register */
#define UCB0IE_SPI                               (HWREG16(0x4000202A))           
#define UCB0IFG                                  (HWREG16(0x4000202C))           /*!< eUSCI_Bx Interrupt Flag Register */
#define UCB0IFG_SPI                              (HWREG16(0x4000202C))           
#define UCB0IV                                   (HWREG16(0x4000202E))           /*!< eUSCI_Bx Interrupt Vector Register */
#define UCB0IV_SPI                               (HWREG16(0x4000202E))           

/* Register offsets from EUSCI_B0_BASE address */
#define OFS_UCB0CTLW0                                      (0x0000)              /*!< eUSCI_Bx Control Word Register 0 */
#define OFS_UCB0CTLW0_SPI                                  (0x0000)              
#define OFS_UCB0CTLW1                                      (0x0002)              /*!< eUSCI_Bx Control Word Register 1 */
#define OFS_UCB0BRW                                        (0x0006)              /*!< eUSCI_Bx Baud Rate Control Word Register */
#define OFS_UCB0BRW_SPI                                    (0x0006)              
#define OFS_UCB0STATW                                      (0x0008)              /*!< eUSCI_Bx Status Register */
#define OFS_UCB0STATW_SPI                                  (0x0008)              
#define OFS_UCB0TBCNT                                      (0x000A)              /*!< eUSCI_Bx Byte Counter Threshold Register */
#define OFS_UCB0RXBUF                                      (0x000C)              /*!< eUSCI_Bx Receive Buffer Register */
#define OFS_UCB0RXBUF_SPI                                  (0x000C)              
#define OFS_UCB0TXBUF                                      (0x000E)              /*!< eUSCI_Bx Transmit Buffer Register */
#define OFS_UCB0TXBUF_SPI                                  (0x000E)              
#define OFS_UCB0I2COA0                                     (0x0014)              /*!< eUSCI_Bx I2C Own Address 0 Register */
#define OFS_UCB0I2COA1                                     (0x0016)              /*!< eUSCI_Bx I2C Own Address 1 Register */
#define OFS_UCB0I2COA2                                     (0x0018)              /*!< eUSCI_Bx I2C Own Address 2 Register */
#define OFS_UCB0I2COA3                                     (0x001A)              /*!< eUSCI_Bx I2C Own Address 3 Register */
#define OFS_UCB0ADDRX                                      (0x001C)              /*!< eUSCI_Bx I2C Received Address Register */
#define OFS_UCB0ADDMASK                                    (0x001E)              /*!< eUSCI_Bx I2C Address Mask Register */
#define OFS_UCB0I2CSA                                      (0x0020)              /*!< eUSCI_Bx I2C Slave Address Register */
#define OFS_UCB0IE                                         (0x002A)              /*!< eUSCI_Bx Interrupt Enable Register */
#define OFS_UCB0IE_SPI                                     (0x002A)              
#define OFS_UCB0IFG                                        (0x002C)              /*!< eUSCI_Bx Interrupt Flag Register */
#define OFS_UCB0IFG_SPI                                    (0x002C)              
#define OFS_UCB0IV                                         (0x002E)              /*!< eUSCI_Bx Interrupt Vector Register */
#define OFS_UCB0IV_SPI                                     (0x002E)              

#define UCB0CTL0                                           (HWREG8_L(UCB0CTLW0)) /* eUSCI_Bx Control 1 */
#define UCB0CTL1                                           (HWREG8_H(UCB0CTLW0)) /* eUSCI_Bx Control 0 */
#define UCB0BR0                                            (HWREG8_L(UCB0BRW))   /* eUSCI_Bx Bit Rate Control 0 */
#define UCB0BR1                                            (HWREG8_H(UCB0BRW))   /* eUSCI_Bx Bit Rate Control 1 */
#define UCB0STAT                                           (HWREG8_L(UCB0STATW)) /* eUSCI_Bx Status */
#define UCB0BCNT                                           (HWREG8_H(UCB0STATW)) /* eUSCI_Bx Byte Counter Register */

/******************************************************************************
* EUSCI_B1 Registers
******************************************************************************/
#define UCB1CTLW0                                (HWREG16(0x40002400))           /*!< eUSCI_Bx Control Word Register 0 */
#define UCB1CTLW0_SPI                            (HWREG16(0x40002400))           
#define UCB1CTLW1                                (HWREG16(0x40002402))           /*!< eUSCI_Bx Control Word Register 1 */
#define UCB1BRW                                  (HWREG16(0x40002406))           /*!< eUSCI_Bx Baud Rate Control Word Register */
#define UCB1BRW_SPI                              (HWREG16(0x40002406))           
#define UCB1STATW                                (HWREG16(0x40002408))           /*!< eUSCI_Bx Status Register */
#define UCB1STATW_SPI                            (HWREG16(0x40002408))           
#define UCB1TBCNT                                (HWREG16(0x4000240A))           /*!< eUSCI_Bx Byte Counter Threshold Register */
#define UCB1RXBUF                                (HWREG16(0x4000240C))           /*!< eUSCI_Bx Receive Buffer Register */
#define UCB1RXBUF_SPI                            (HWREG16(0x4000240C))           
#define UCB1TXBUF                                (HWREG16(0x4000240E))           /*!< eUSCI_Bx Transmit Buffer Register */
#define UCB1TXBUF_SPI                            (HWREG16(0x4000240E))           
#define UCB1I2COA0                               (HWREG16(0x40002414))           /*!< eUSCI_Bx I2C Own Address 0 Register */
#define UCB1I2COA1                               (HWREG16(0x40002416))           /*!< eUSCI_Bx I2C Own Address 1 Register */
#define UCB1I2COA2                               (HWREG16(0x40002418))           /*!< eUSCI_Bx I2C Own Address 2 Register */
#define UCB1I2COA3                               (HWREG16(0x4000241A))           /*!< eUSCI_Bx I2C Own Address 3 Register */
#define UCB1ADDRX                                (HWREG16(0x4000241C))           /*!< eUSCI_Bx I2C Received Address Register */
#define UCB1ADDMASK                              (HWREG16(0x4000241E))           /*!< eUSCI_Bx I2C Address Mask Register */
#define UCB1I2CSA                                (HWREG16(0x40002420))           /*!< eUSCI_Bx I2C Slave Address Register */
#define UCB1IE                                   (HWREG16(0x4000242A))           /*!< eUSCI_Bx Interrupt Enable Register */
#define UCB1IE_SPI                               (HWREG16(0x4000242A))           
#define UCB1IFG                                  (HWREG16(0x4000242C))           /*!< eUSCI_Bx Interrupt Flag Register */
#define UCB1IFG_SPI                              (HWREG16(0x4000242C))           
#define UCB1IV                                   (HWREG16(0x4000242E))           /*!< eUSCI_Bx Interrupt Vector Register */
#define UCB1IV_SPI                               (HWREG16(0x4000242E))           

/* Register offsets from EUSCI_B1_BASE address */
#define OFS_UCB1CTLW0                                      (0x0000)              /*!< eUSCI_Bx Control Word Register 0 */
#define OFS_UCB1CTLW0_SPI                                  (0x0000)              
#define OFS_UCB1CTLW1                                      (0x0002)              /*!< eUSCI_Bx Control Word Register 1 */
#define OFS_UCB1BRW                                        (0x0006)              /*!< eUSCI_Bx Baud Rate Control Word Register */
#define OFS_UCB1BRW_SPI                                    (0x0006)              
#define OFS_UCB1STATW                                      (0x0008)              /*!< eUSCI_Bx Status Register */
#define OFS_UCB1STATW_SPI                                  (0x0008)              
#define OFS_UCB1TBCNT                                      (0x000A)              /*!< eUSCI_Bx Byte Counter Threshold Register */
#define OFS_UCB1RXBUF                                      (0x000C)              /*!< eUSCI_Bx Receive Buffer Register */
#define OFS_UCB1RXBUF_SPI                                  (0x000C)              
#define OFS_UCB1TXBUF                                      (0x000E)              /*!< eUSCI_Bx Transmit Buffer Register */
#define OFS_UCB1TXBUF_SPI                                  (0x000E)              
#define OFS_UCB1I2COA0                                     (0x0014)              /*!< eUSCI_Bx I2C Own Address 0 Register */
#define OFS_UCB1I2COA1                                     (0x0016)              /*!< eUSCI_Bx I2C Own Address 1 Register */
#define OFS_UCB1I2COA2                                     (0x0018)              /*!< eUSCI_Bx I2C Own Address 2 Register */
#define OFS_UCB1I2COA3                                     (0x001A)              /*!< eUSCI_Bx I2C Own Address 3 Register */
#define OFS_UCB1ADDRX                                      (0x001C)              /*!< eUSCI_Bx I2C Received Address Register */
#define OFS_UCB1ADDMASK                                    (0x001E)              /*!< eUSCI_Bx I2C Address Mask Register */
#define OFS_UCB1I2CSA                                      (0x0020)              /*!< eUSCI_Bx I2C Slave Address Register */
#define OFS_UCB1IE                                         (0x002A)              /*!< eUSCI_Bx Interrupt Enable Register */
#define OFS_UCB1IE_SPI                                     (0x002A)              
#define OFS_UCB1IFG                                        (0x002C)              /*!< eUSCI_Bx Interrupt Flag Register */
#define OFS_UCB1IFG_SPI                                    (0x002C)              
#define OFS_UCB1IV                                         (0x002E)              /*!< eUSCI_Bx Interrupt Vector Register */
#define OFS_UCB1IV_SPI                                     (0x002E)              

#define UCB1CTL0                                           (HWREG8_L(UCB1CTLW0)) /* eUSCI_Bx Control 1 */
#define UCB1CTL1                                           (HWREG8_H(UCB1CTLW0)) /* eUSCI_Bx Control 0 */
#define UCB1BR0                                            (HWREG8_L(UCB1BRW))   /* eUSCI_Bx Bit Rate Control 0 */
#define UCB1BR1                                            (HWREG8_H(UCB1BRW))   /* eUSCI_Bx Bit Rate Control 1 */
#define UCB1STAT                                           (HWREG8_L(UCB1STATW)) /* eUSCI_Bx Status */
#define UCB1BCNT                                           (HWREG8_H(UCB1STATW)) /* eUSCI_Bx Byte Counter Register */

/******************************************************************************
* EUSCI_B2 Registers
******************************************************************************/
#define UCB2CTLW0                                (HWREG16(0x40002800))           /*!< eUSCI_Bx Control Word Register 0 */
#define UCB2CTLW0_SPI                            (HWREG16(0x40002800))           
#define UCB2CTLW1                                (HWREG16(0x40002802))           /*!< eUSCI_Bx Control Word Register 1 */
#define UCB2BRW                                  (HWREG16(0x40002806))           /*!< eUSCI_Bx Baud Rate Control Word Register */
#define UCB2BRW_SPI                              (HWREG16(0x40002806))           
#define UCB2STATW                                (HWREG16(0x40002808))           /*!< eUSCI_Bx Status Register */
#define UCB2STATW_SPI                            (HWREG16(0x40002808))           
#define UCB2TBCNT                                (HWREG16(0x4000280A))           /*!< eUSCI_Bx Byte Counter Threshold Register */
#define UCB2RXBUF                                (HWREG16(0x4000280C))           /*!< eUSCI_Bx Receive Buffer Register */
#define UCB2RXBUF_SPI                            (HWREG16(0x4000280C))           
#define UCB2TXBUF                                (HWREG16(0x4000280E))           /*!< eUSCI_Bx Transmit Buffer Register */
#define UCB2TXBUF_SPI                            (HWREG16(0x4000280E))           
#define UCB2I2COA0                               (HWREG16(0x40002814))           /*!< eUSCI_Bx I2C Own Address 0 Register */
#define UCB2I2COA1                               (HWREG16(0x40002816))           /*!< eUSCI_Bx I2C Own Address 1 Register */
#define UCB2I2COA2                               (HWREG16(0x40002818))           /*!< eUSCI_Bx I2C Own Address 2 Register */
#define UCB2I2COA3                               (HWREG16(0x4000281A))           /*!< eUSCI_Bx I2C Own Address 3 Register */
#define UCB2ADDRX                                (HWREG16(0x4000281C))           /*!< eUSCI_Bx I2C Received Address Register */
#define UCB2ADDMASK                              (HWREG16(0x4000281E))           /*!< eUSCI_Bx I2C Address Mask Register */
#define UCB2I2CSA                                (HWREG16(0x40002820))           /*!< eUSCI_Bx I2C Slave Address Register */
#define UCB2IE                                   (HWREG16(0x4000282A))           /*!< eUSCI_Bx Interrupt Enable Register */
#define UCB2IE_SPI                               (HWREG16(0x4000282A))           
#define UCB2IFG                                  (HWREG16(0x4000282C))           /*!< eUSCI_Bx Interrupt Flag Register */
#define UCB2IFG_SPI                              (HWREG16(0x4000282C))           
#define UCB2IV                                   (HWREG16(0x4000282E))           /*!< eUSCI_Bx Interrupt Vector Register */
#define UCB2IV_SPI                               (HWREG16(0x4000282E))           

/* Register offsets from EUSCI_B2_BASE address */
#define OFS_UCB2CTLW0                                      (0x0000)              /*!< eUSCI_Bx Control Word Register 0 */
#define OFS_UCB2CTLW0_SPI                                  (0x0000)              
#define OFS_UCB2CTLW1                                      (0x0002)              /*!< eUSCI_Bx Control Word Register 1 */
#define OFS_UCB2BRW                                        (0x0006)              /*!< eUSCI_Bx Baud Rate Control Word Register */
#define OFS_UCB2BRW_SPI                                    (0x0006)              
#define OFS_UCB2STATW                                      (0x0008)              /*!< eUSCI_Bx Status Register */
#define OFS_UCB2STATW_SPI                                  (0x0008)              
#define OFS_UCB2TBCNT                                      (0x000A)              /*!< eUSCI_Bx Byte Counter Threshold Register */
#define OFS_UCB2RXBUF                                      (0x000C)              /*!< eUSCI_Bx Receive Buffer Register */
#define OFS_UCB2RXBUF_SPI                                  (0x000C)              
#define OFS_UCB2TXBUF                                      (0x000E)              /*!< eUSCI_Bx Transmit Buffer Register */
#define OFS_UCB2TXBUF_SPI                                  (0x000E)              
#define OFS_UCB2I2COA0                                     (0x0014)              /*!< eUSCI_Bx I2C Own Address 0 Register */
#define OFS_UCB2I2COA1                                     (0x0016)              /*!< eUSCI_Bx I2C Own Address 1 Register */
#define OFS_UCB2I2COA2                                     (0x0018)              /*!< eUSCI_Bx I2C Own Address 2 Register */
#define OFS_UCB2I2COA3                                     (0x001A)              /*!< eUSCI_Bx I2C Own Address 3 Register */
#define OFS_UCB2ADDRX                                      (0x001C)              /*!< eUSCI_Bx I2C Received Address Register */
#define OFS_UCB2ADDMASK                                    (0x001E)              /*!< eUSCI_Bx I2C Address Mask Register */
#define OFS_UCB2I2CSA                                      (0x0020)              /*!< eUSCI_Bx I2C Slave Address Register */
#define OFS_UCB2IE                                         (0x002A)              /*!< eUSCI_Bx Interrupt Enable Register */
#define OFS_UCB2IE_SPI                                     (0x002A)              
#define OFS_UCB2IFG                                        (0x002C)              /*!< eUSCI_Bx Interrupt Flag Register */
#define OFS_UCB2IFG_SPI                                    (0x002C)              
#define OFS_UCB2IV                                         (0x002E)              /*!< eUSCI_Bx Interrupt Vector Register */
#define OFS_UCB2IV_SPI                                     (0x002E)              

#define UCB2CTL0                                           (HWREG8_L(UCB2CTLW0)) /* eUSCI_Bx Control 1 */
#define UCB2CTL1                                           (HWREG8_H(UCB2CTLW0)) /* eUSCI_Bx Control 0 */
#define UCB2BR0                                            (HWREG8_L(UCB2BRW))   /* eUSCI_Bx Bit Rate Control 0 */
#define UCB2BR1                                            (HWREG8_H(UCB2BRW))   /* eUSCI_Bx Bit Rate Control 1 */
#define UCB2STAT                                           (HWREG8_L(UCB2STATW)) /* eUSCI_Bx Status */
#define UCB2BCNT                                           (HWREG8_H(UCB2STATW)) /* eUSCI_Bx Byte Counter Register */

/******************************************************************************
* EUSCI_B3 Registers
******************************************************************************/
#define UCB3CTLW0                                (HWREG16(0x40002C00))           /*!< eUSCI_Bx Control Word Register 0 */
#define UCB3CTLW0_SPI                            (HWREG16(0x40002C00))           
#define UCB3CTLW1                                (HWREG16(0x40002C02))           /*!< eUSCI_Bx Control Word Register 1 */
#define UCB3BRW                                  (HWREG16(0x40002C06))           /*!< eUSCI_Bx Baud Rate Control Word Register */
#define UCB3BRW_SPI                              (HWREG16(0x40002C06))           
#define UCB3STATW                                (HWREG16(0x40002C08))           /*!< eUSCI_Bx Status Register */
#define UCB3STATW_SPI                            (HWREG16(0x40002C08))           
#define UCB3TBCNT                                (HWREG16(0x40002C0A))           /*!< eUSCI_Bx Byte Counter Threshold Register */
#define UCB3RXBUF                                (HWREG16(0x40002C0C))           /*!< eUSCI_Bx Receive Buffer Register */
#define UCB3RXBUF_SPI                            (HWREG16(0x40002C0C))           
#define UCB3TXBUF                                (HWREG16(0x40002C0E))           /*!< eUSCI_Bx Transmit Buffer Register */
#define UCB3TXBUF_SPI                            (HWREG16(0x40002C0E))           
#define UCB3I2COA0                               (HWREG16(0x40002C14))           /*!< eUSCI_Bx I2C Own Address 0 Register */
#define UCB3I2COA1                               (HWREG16(0x40002C16))           /*!< eUSCI_Bx I2C Own Address 1 Register */
#define UCB3I2COA2                               (HWREG16(0x40002C18))           /*!< eUSCI_Bx I2C Own Address 2 Register */
#define UCB3I2COA3                               (HWREG16(0x40002C1A))           /*!< eUSCI_Bx I2C Own Address 3 Register */
#define UCB3ADDRX                                (HWREG16(0x40002C1C))           /*!< eUSCI_Bx I2C Received Address Register */
#define UCB3ADDMASK                              (HWREG16(0x40002C1E))           /*!< eUSCI_Bx I2C Address Mask Register */
#define UCB3I2CSA                                (HWREG16(0x40002C20))           /*!< eUSCI_Bx I2C Slave Address Register */
#define UCB3IE                                   (HWREG16(0x40002C2A))           /*!< eUSCI_Bx Interrupt Enable Register */
#define UCB3IE_SPI                               (HWREG16(0x40002C2A))           
#define UCB3IFG                                  (HWREG16(0x40002C2C))           /*!< eUSCI_Bx Interrupt Flag Register */
#define UCB3IFG_SPI                              (HWREG16(0x40002C2C))           
#define UCB3IV                                   (HWREG16(0x40002C2E))           /*!< eUSCI_Bx Interrupt Vector Register */
#define UCB3IV_SPI                               (HWREG16(0x40002C2E))           

/* Register offsets from EUSCI_B3_BASE address */
#define OFS_UCB3CTLW0                                      (0x0000)              /*!< eUSCI_Bx Control Word Register 0 */
#define OFS_UCB3CTLW0_SPI                                  (0x0000)              
#define OFS_UCB3CTLW1                                      (0x0002)              /*!< eUSCI_Bx Control Word Register 1 */
#define OFS_UCB3BRW                                        (0x0006)              /*!< eUSCI_Bx Baud Rate Control Word Register */
#define OFS_UCB3BRW_SPI                                    (0x0006)              
#define OFS_UCB3STATW                                      (0x0008)              /*!< eUSCI_Bx Status Register */
#define OFS_UCB3STATW_SPI                                  (0x0008)              
#define OFS_UCB3TBCNT                                      (0x000A)              /*!< eUSCI_Bx Byte Counter Threshold Register */
#define OFS_UCB3RXBUF                                      (0x000C)              /*!< eUSCI_Bx Receive Buffer Register */
#define OFS_UCB3RXBUF_SPI                                  (0x000C)              
#define OFS_UCB3TXBUF                                      (0x000E)              /*!< eUSCI_Bx Transmit Buffer Register */
#define OFS_UCB3TXBUF_SPI                                  (0x000E)              
#define OFS_UCB3I2COA0                                     (0x0014)              /*!< eUSCI_Bx I2C Own Address 0 Register */
#define OFS_UCB3I2COA1                                     (0x0016)              /*!< eUSCI_Bx I2C Own Address 1 Register */
#define OFS_UCB3I2COA2                                     (0x0018)              /*!< eUSCI_Bx I2C Own Address 2 Register */
#define OFS_UCB3I2COA3                                     (0x001A)              /*!< eUSCI_Bx I2C Own Address 3 Register */
#define OFS_UCB3ADDRX                                      (0x001C)              /*!< eUSCI_Bx I2C Received Address Register */
#define OFS_UCB3ADDMASK                                    (0x001E)              /*!< eUSCI_Bx I2C Address Mask Register */
#define OFS_UCB3I2CSA                                      (0x0020)              /*!< eUSCI_Bx I2C Slave Address Register */
#define OFS_UCB3IE                                         (0x002A)              /*!< eUSCI_Bx Interrupt Enable Register */
#define OFS_UCB3IE_SPI                                     (0x002A)              
#define OFS_UCB3IFG                                        (0x002C)              /*!< eUSCI_Bx Interrupt Flag Register */
#define OFS_UCB3IFG_SPI                                    (0x002C)              
#define OFS_UCB3IV                                         (0x002E)              /*!< eUSCI_Bx Interrupt Vector Register */
#define OFS_UCB3IV_SPI                                     (0x002E)              

#define UCB3CTL0                                           (HWREG8_L(UCB3CTLW0)) /* eUSCI_Bx Control 1 */
#define UCB3CTL1                                           (HWREG8_H(UCB3CTLW0)) /* eUSCI_Bx Control 0 */
#define UCB3BR0                                            (HWREG8_L(UCB3BRW))   /* eUSCI_Bx Bit Rate Control 0 */
#define UCB3BR1                                            (HWREG8_H(UCB3BRW))   /* eUSCI_Bx Bit Rate Control 1 */
#define UCB3STAT                                           (HWREG8_L(UCB3STATW)) /* eUSCI_Bx Status */
#define UCB3BCNT                                           (HWREG8_H(UCB3STATW)) /* eUSCI_Bx Byte Counter Register */

/******************************************************************************
* PMAP Registers
******************************************************************************/
#define PMAPKEYID                                (HWREG16(0x40005000))           /*!< Port Mapping Key Register */
#define PMAPCTL                                  (HWREG16(0x40005002))           /*!< Port Mapping Control Register */
#define P1MAP01                                  (HWREG16(0x40005008))           /*!< Port mapping register, P1.0 and P1.1 */
#define P1MAP23                                  (HWREG16(0x4000500A))           /*!< Port mapping register, P1.2 and P1.3 */
#define P1MAP45                                  (HWREG16(0x4000500C))           /*!< Port mapping register, P1.4 and P1.5 */
#define P1MAP67                                  (HWREG16(0x4000500E))           /*!< Port mapping register, P1.6 and P1.7 */
#define P2MAP01                                  (HWREG16(0x40005010))           /*!< Port mapping register, P2.0 and P2.1 */
#define P2MAP23                                  (HWREG16(0x40005012))           /*!< Port mapping register, P2.2 and P2.3 */
#define P2MAP45                                  (HWREG16(0x40005014))           /*!< Port mapping register, P2.4 and P2.5 */
#define P2MAP67                                  (HWREG16(0x40005016))           /*!< Port mapping register, P2.6 and P2.7 */
#define P3MAP01                                  (HWREG16(0x40005018))           /*!< Port mapping register, P3.0 and P3.1 */
#define P3MAP23                                  (HWREG16(0x4000501A))           /*!< Port mapping register, P3.2 and P3.3 */
#define P3MAP45                                  (HWREG16(0x4000501C))           /*!< Port mapping register, P3.4 and P3.5 */
#define P3MAP67                                  (HWREG16(0x4000501E))           /*!< Port mapping register, P3.6 and P3.7 */
#define P4MAP01                                  (HWREG16(0x40005020))           /*!< Port mapping register, P4.0 and P4.1 */
#define P4MAP23                                  (HWREG16(0x40005022))           /*!< Port mapping register, P4.2 and P4.3 */
#define P4MAP45                                  (HWREG16(0x40005024))           /*!< Port mapping register, P4.4 and P4.5 */
#define P4MAP67                                  (HWREG16(0x40005026))           /*!< Port mapping register, P4.6 and P4.7 */
#define P5MAP01                                  (HWREG16(0x40005028))           /*!< Port mapping register, P5.0 and P5.1 */
#define P5MAP23                                  (HWREG16(0x4000502A))           /*!< Port mapping register, P5.2 and P5.3 */
#define P5MAP45                                  (HWREG16(0x4000502C))           /*!< Port mapping register, P5.4 and P5.5 */
#define P5MAP67                                  (HWREG16(0x4000502E))           /*!< Port mapping register, P5.6 and P5.7 */
#define P6MAP01                                  (HWREG16(0x40005030))           /*!< Port mapping register, P6.0 and P6.1 */
#define P6MAP23                                  (HWREG16(0x40005032))           /*!< Port mapping register, P6.2 and P6.3 */
#define P6MAP45                                  (HWREG16(0x40005034))           /*!< Port mapping register, P6.4 and P6.5 */
#define P6MAP67                                  (HWREG16(0x40005036))           /*!< Port mapping register, P6.6 and P6.7 */
#define P7MAP01                                  (HWREG16(0x40005038))           /*!< Port mapping register, P7.0 and P7.1 */
#define P7MAP23                                  (HWREG16(0x4000503A))           /*!< Port mapping register, P7.2 and P7.3 */
#define P7MAP45                                  (HWREG16(0x4000503C))           /*!< Port mapping register, P7.4 and P7.5 */
#define P7MAP67                                  (HWREG16(0x4000503E))           /*!< Port mapping register, P7.6 and P7.7 */

/* Register offsets from PMAP_BASE address */
#define OFS_PMAPKEYID                                      (0x0000)              /*!< Port Mapping Key Register */
#define OFS_PMAPCTL                                        (0x0002)              /*!< Port Mapping Control Register */
#define OFS_P1MAP01                                        (0x0008)              /*!< Port mapping register, P1.0 and P1.1 */
#define OFS_P1MAP23                                        (0x000A)              /*!< Port mapping register, P1.2 and P1.3 */
#define OFS_P1MAP45                                        (0x000C)              /*!< Port mapping register, P1.4 and P1.5 */
#define OFS_P1MAP67                                        (0x000E)              /*!< Port mapping register, P1.6 and P1.7 */
#define OFS_P2MAP01                                        (0x0010)              /*!< Port mapping register, P2.0 and P2.1 */
#define OFS_P2MAP23                                        (0x0012)              /*!< Port mapping register, P2.2 and P2.3 */
#define OFS_P2MAP45                                        (0x0014)              /*!< Port mapping register, P2.4 and P2.5 */
#define OFS_P2MAP67                                        (0x0016)              /*!< Port mapping register, P2.6 and P2.7 */
#define OFS_P3MAP01                                        (0x0018)              /*!< Port mapping register, P3.0 and P3.1 */
#define OFS_P3MAP23                                        (0x001A)              /*!< Port mapping register, P3.2 and P3.3 */
#define OFS_P3MAP45                                        (0x001C)              /*!< Port mapping register, P3.4 and P3.5 */
#define OFS_P3MAP67                                        (0x001E)              /*!< Port mapping register, P3.6 and P3.7 */
#define OFS_P4MAP01                                        (0x0020)              /*!< Port mapping register, P4.0 and P4.1 */
#define OFS_P4MAP23                                        (0x0022)              /*!< Port mapping register, P4.2 and P4.3 */
#define OFS_P4MAP45                                        (0x0024)              /*!< Port mapping register, P4.4 and P4.5 */
#define OFS_P4MAP67                                        (0x0026)              /*!< Port mapping register, P4.6 and P4.7 */
#define OFS_P5MAP01                                        (0x0028)              /*!< Port mapping register, P5.0 and P5.1 */
#define OFS_P5MAP23                                        (0x002A)              /*!< Port mapping register, P5.2 and P5.3 */
#define OFS_P5MAP45                                        (0x002C)              /*!< Port mapping register, P5.4 and P5.5 */
#define OFS_P5MAP67                                        (0x002E)              /*!< Port mapping register, P5.6 and P5.7 */
#define OFS_P6MAP01                                        (0x0030)              /*!< Port mapping register, P6.0 and P6.1 */
#define OFS_P6MAP23                                        (0x0032)              /*!< Port mapping register, P6.2 and P6.3 */
#define OFS_P6MAP45                                        (0x0034)              /*!< Port mapping register, P6.4 and P6.5 */
#define OFS_P6MAP67                                        (0x0036)              /*!< Port mapping register, P6.6 and P6.7 */
#define OFS_P7MAP01                                        (0x0038)              /*!< Port mapping register, P7.0 and P7.1 */
#define OFS_P7MAP23                                        (0x003A)              /*!< Port mapping register, P7.2 and P7.3 */
#define OFS_P7MAP45                                        (0x003C)              /*!< Port mapping register, P7.4 and P7.5 */
#define OFS_P7MAP67                                        (0x003E)              /*!< Port mapping register, P7.6 and P7.7 */


/******************************************************************************
* REF_A Registers
******************************************************************************/
#define REFCTL0                                  (HWREG16(0x40003000))           /*!< REF Control Register 0 */

/* Register offsets from REF_A_BASE address */
#define OFS_REFCTL0                                        (0x0000)              /*!< REF Control Register 0 */

#define REFCTL0_L                                          (HWREG8_L(REFCTL0))   /* REF Control Register 0 */
#define REFCTL0_H                                          (HWREG8_H(REFCTL0))   /* REF Control Register 0 */

/******************************************************************************
* RTC_C Registers
******************************************************************************/
#define RTCCTL0                                  (HWREG16(0x40004400))           /*!< RTCCTL0 Register */
#define RTCCTL13                                 (HWREG16(0x40004402))           /*!< RTCCTL13 Register */
#define RTCOCAL                                  (HWREG16(0x40004404))           /*!< RTCOCAL Register */
#define RTCTCMP                                  (HWREG16(0x40004406))           /*!< RTCTCMP Register */
#define RTCPS0CTL                                (HWREG16(0x40004408))           /*!< Real-Time Clock Prescale Timer 0 Control Register */
#define RTCPS1CTL                                (HWREG16(0x4000440A))           /*!< Real-Time Clock Prescale Timer 1 Control Register */
#define RTCPS                                    (HWREG16(0x4000440C))           /*!< Real-Time Clock Prescale Timer Counter Register */
#define RTCIV                                    (HWREG16(0x4000440E))           /*!< Real-Time Clock Interrupt Vector Register */
#define RTCTIM0                                  (HWREG16(0x40004410))           /*!< RTCTIM0 Register  Hexadecimal Format */
#define RTCTIM0_BCD                              (HWREG16(0x40004410))           
#define RTCTIM1                                  (HWREG16(0x40004412))           /*!< Real-Time Clock Hour, Day of Week */
#define RTCTIM1_BCD                              (HWREG16(0x40004412))           
#define RTCDATE                                  (HWREG16(0x40004414))           /*!< RTCDATE - Hexadecimal Format */
#define RTCDATE_BCD                              (HWREG16(0x40004414))           
#define RTCYEAR                                  (HWREG16(0x40004416))           /*!< RTCYEAR Register  Hexadecimal Format */
#define RTCYEAR_BCD                              (HWREG16(0x40004416))           
#define RTCAMINHR                                (HWREG16(0x40004418))           /*!< RTCMINHR - Hexadecimal Format */
#define RTCAMINHR_BCD                            (HWREG16(0x40004418))           
#define RTCADOWDAY                               (HWREG16(0x4000441A))           /*!< RTCADOWDAY - Hexadecimal Format */
#define RTCADOWDAY_BCD                           (HWREG16(0x4000441A))           
#define RTCBIN2BCD                               (HWREG16(0x4000441C))           /*!< Binary-to-BCD Conversion Register */
#define RTCBCD2BIN                               (HWREG16(0x4000441E))           /*!< BCD-to-Binary Conversion Register */

/* Register offsets from RTC_C_BASE address */
#define OFS_RTCCTL0                                        (0x0000)              /*!< RTCCTL0 Register */
#define OFS_RTCCTL13                                       (0x0002)              /*!< RTCCTL13 Register */
#define OFS_RTCOCAL                                        (0x0004)              /*!< RTCOCAL Register */
#define OFS_RTCTCMP                                        (0x0006)              /*!< RTCTCMP Register */
#define OFS_RTCPS0CTL                                      (0x0008)              /*!< Real-Time Clock Prescale Timer 0 Control Register */
#define OFS_RTCPS1CTL                                      (0x000A)              /*!< Real-Time Clock Prescale Timer 1 Control Register */
#define OFS_RTCPS                                          (0x000C)              /*!< Real-Time Clock Prescale Timer Counter Register */
#define OFS_RTCIV                                          (0x000E)              /*!< Real-Time Clock Interrupt Vector Register */
#define OFS_RTCTIM0                                        (0x0010)              /*!< RTCTIM0 Register  Hexadecimal Format */
#define OFS_RTCTIM0_BCD                                    (0x0010)              
#define OFS_RTCTIM1                                        (0x0012)              /*!< Real-Time Clock Hour, Day of Week */
#define OFS_RTCTIM1_BCD                                    (0x0012)              
#define OFS_RTCDATE                                        (0x0014)              /*!< RTCDATE - Hexadecimal Format */
#define OFS_RTCDATE_BCD                                    (0x0014)              
#define OFS_RTCYEAR                                        (0x0016)              /*!< RTCYEAR Register  Hexadecimal Format */
#define OFS_RTCYEAR_BCD                                    (0x0016)              
#define OFS_RTCAMINHR                                      (0x0018)              /*!< RTCMINHR - Hexadecimal Format */
#define OFS_RTCAMINHR_BCD                                  (0x0018)              
#define OFS_RTCADOWDAY                                     (0x001A)              /*!< RTCADOWDAY - Hexadecimal Format */
#define OFS_RTCADOWDAY_BCD                                 (0x001A)              
#define OFS_RTCBIN2BCD                                     (0x001C)              /*!< Binary-to-BCD Conversion Register */
#define OFS_RTCBCD2BIN                                     (0x001E)              /*!< BCD-to-Binary Conversion Register */

#define RTCCTL0_L                                          (HWREG8_L(RTCCTL0))   /* RTCCTL0 Register */
#define RTCCTL0_H                                          (HWREG8_H(RTCCTL0))   /* RTCCTL0 Register */
#define RTCCTL1                                            (HWREG8_L(RTCCTL13))  /* RTCCTL13 Register */
#define RTCCTL13_L                                         (HWREG8_L(RTCCTL13))  /* RTCCTL13 Register */
#define RTCCTL3                                            (HWREG8_H(RTCCTL13))  /* RTCCTL13 Register */
#define RTCCTL13_H                                         (HWREG8_H(RTCCTL13))  /* RTCCTL13 Register */
#define RTCOCAL_L                                          (HWREG8_L(RTCOCAL))   /* RTCOCAL Register */
#define RTCOCAL_H                                          (HWREG8_H(RTCOCAL))   /* RTCOCAL Register */
#define RTCTCMP_L                                          (HWREG8_L(RTCTCMP))   /* RTCTCMP Register */
#define RTCTCMP_H                                          (HWREG8_H(RTCTCMP))   /* RTCTCMP Register */
#define RTCPS0CTL_L                                        (HWREG8_L(RTCPS0CTL)) /* Real-Time Clock Prescale Timer 0 Control Register */
#define RTCPS0CTL_H                                        (HWREG8_H(RTCPS0CTL)) /* Real-Time Clock Prescale Timer 0 Control Register */
#define RTCPS1CTL_L                                        (HWREG8_L(RTCPS1CTL)) /* Real-Time Clock Prescale Timer 1 Control Register */
#define RTCPS1CTL_H                                        (HWREG8_H(RTCPS1CTL)) /* Real-Time Clock Prescale Timer 1 Control Register */
#define RTCPS0                                             (HWREG8_L(RTCPS))     /* Real-Time Clock Prescale Timer Counter Register */
#define RTCPS_L                                            (HWREG8_L(RTCPS))     /* Real-Time Clock Prescale Timer Counter Register */
#define RTCPS1                                             (HWREG8_H(RTCPS))     /* Real-Time Clock Prescale Timer Counter Register */
#define RTCPS_H                                            (HWREG8_H(RTCPS))     /* Real-Time Clock Prescale Timer Counter Register */
#define RTCSEC                                             (HWREG8_L(RTCTIM0))   /* Real-Time Clock Seconds */
#define RTCTIM0_L                                          (HWREG8_L(RTCTIM0))   /* Real-Time Clock Seconds */
#define RTCMIN                                             (HWREG8_H(RTCTIM0))   /* Real-Time Clock Minutes */
#define RTCTIM0_H                                          (HWREG8_H(RTCTIM0))   /* Real-Time Clock Minutes */
#define RTCHOUR                                            (HWREG8_L(RTCTIM1))   /* Real-Time Clock Hour */
#define RTCTIM1_L                                          (HWREG8_L(RTCTIM1))   /* Real-Time Clock Hour */
#define RTCDOW                                             (HWREG8_H(RTCTIM1))   /* Real-Time Clock Day of Week */
#define RTCTIM1_H                                          (HWREG8_H(RTCTIM1))   /* Real-Time Clock Day of Week */
#define RTCDAY                                             (HWREG8_L(RTCDATE))   /* Real-Time Clock Day of Month */
#define RTCDATE_L                                          (HWREG8_L(RTCDATE))   /* Real-Time Clock Day of Month */
#define RTCMON                                             (HWREG8_H(RTCDATE))   /* Real-Time Clock Month */
#define RTCDATE_H                                          (HWREG8_H(RTCDATE))   /* Real-Time Clock Month */
#define RTCAMIN                                            (HWREG8_L(RTCAMINHR)) /* Real-Time Clock Minutes Alarm */
#define RTCAMINHR_L                                        (HWREG8_L(RTCAMINHR)) /* Real-Time Clock Minutes Alarm */
#define RTCAHOUR                                           (HWREG8_H(RTCAMINHR)) /* Real-Time Clock Hours Alarm */
#define RTCAMINHR_H                                        (HWREG8_H(RTCAMINHR)) /* Real-Time Clock Hours Alarm */
#define RTCADOW                                            (HWREG8_L(RTCADOWDAY))/* Real-Time Clock Day of Week Alarm */
#define RTCADOWDAY_L                                       (HWREG8_L(RTCADOWDAY))/* Real-Time Clock Day of Week Alarm */
#define RTCADAY                                            (HWREG8_H(RTCADOWDAY))/* Real-Time Clock Day of Month Alarm */
#define RTCADOWDAY_H                                       (HWREG8_H(RTCADOWDAY))/* Real-Time Clock Day of Month Alarm */

/******************************************************************************
* TIMER_A0 Registers
******************************************************************************/
#define TA0CTL                                   (HWREG16(0x40000000))           /*!< TimerAx Control Register */
#define TA0CCTL0                                 (HWREG16(0x40000002))           /*!< Timer_A Capture/Compare Control Register */
#define TA0CCTL1                                 (HWREG16(0x40000004))           /*!< Timer_A Capture/Compare Control Register */
#define TA0CCTL2                                 (HWREG16(0x40000006))           /*!< Timer_A Capture/Compare Control Register */
#define TA0CCTL3                                 (HWREG16(0x40000008))           /*!< Timer_A Capture/Compare Control Register */
#define TA0CCTL4                                 (HWREG16(0x4000000A))           /*!< Timer_A Capture/Compare Control Register */
#define TA0R                                     (HWREG16(0x40000010))           /*!< TimerA register */
#define TA0CCR0                                  (HWREG16(0x40000012))           /*!< Timer_A Capture/Compare  Register */
#define TA0CCR1                                  (HWREG16(0x40000014))           /*!< Timer_A Capture/Compare  Register */
#define TA0CCR2                                  (HWREG16(0x40000016))           /*!< Timer_A Capture/Compare  Register */
#define TA0CCR3                                  (HWREG16(0x40000018))           /*!< Timer_A Capture/Compare  Register */
#define TA0CCR4                                  (HWREG16(0x4000001A))           /*!< Timer_A Capture/Compare  Register */
#define TA0EX0                                   (HWREG16(0x40000020))           /*!< TimerAx Expansion 0 Register */
#define TA0IV                                    (HWREG16(0x4000002E))           /*!< TimerAx Interrupt Vector Register */

/* Register offsets from TIMER_A0_BASE address */
#define OFS_TA0CTL                                         (0x0000)              /*!< TimerAx Control Register */
#define OFS_TA0CCTL0                                       (0x0002)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA0CCTL1                                       (0x0004)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA0CCTL2                                       (0x0006)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA0CCTL3                                       (0x0008)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA0CCTL4                                       (0x000A)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA0R                                           (0x0010)              /*!< TimerA register */
#define OFS_TA0CCR0                                        (0x0012)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA0CCR1                                        (0x0014)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA0CCR2                                        (0x0016)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA0CCR3                                        (0x0018)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA0CCR4                                        (0x001A)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA0EX0                                         (0x0020)              /*!< TimerAx Expansion 0 Register */
#define OFS_TA0IV                                          (0x002E)              /*!< TimerAx Interrupt Vector Register */


/******************************************************************************
* TIMER_A1 Registers
******************************************************************************/
#define TA1CTL                                   (HWREG16(0x40000400))           /*!< TimerAx Control Register */
#define TA1CCTL0                                 (HWREG16(0x40000402))           /*!< Timer_A Capture/Compare Control Register */
#define TA1CCTL1                                 (HWREG16(0x40000404))           /*!< Timer_A Capture/Compare Control Register */
#define TA1CCTL2                                 (HWREG16(0x40000406))           /*!< Timer_A Capture/Compare Control Register */
#define TA1CCTL3                                 (HWREG16(0x40000408))           /*!< Timer_A Capture/Compare Control Register */
#define TA1CCTL4                                 (HWREG16(0x4000040A))           /*!< Timer_A Capture/Compare Control Register */
#define TA1R                                     (HWREG16(0x40000410))           /*!< TimerA register */
#define TA1CCR0                                  (HWREG16(0x40000412))           /*!< Timer_A Capture/Compare  Register */
#define TA1CCR1                                  (HWREG16(0x40000414))           /*!< Timer_A Capture/Compare  Register */
#define TA1CCR2                                  (HWREG16(0x40000416))           /*!< Timer_A Capture/Compare  Register */
#define TA1CCR3                                  (HWREG16(0x40000418))           /*!< Timer_A Capture/Compare  Register */
#define TA1CCR4                                  (HWREG16(0x4000041A))           /*!< Timer_A Capture/Compare  Register */
#define TA1EX0                                   (HWREG16(0x40000420))           /*!< TimerAx Expansion 0 Register */
#define TA1IV                                    (HWREG16(0x4000042E))           /*!< TimerAx Interrupt Vector Register */

/* Register offsets from TIMER_A1_BASE address */
#define OFS_TA1CTL                                         (0x0000)              /*!< TimerAx Control Register */
#define OFS_TA1CCTL0                                       (0x0002)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA1CCTL1                                       (0x0004)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA1CCTL2                                       (0x0006)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA1CCTL3                                       (0x0008)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA1CCTL4                                       (0x000A)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA1R                                           (0x0010)              /*!< TimerA register */
#define OFS_TA1CCR0                                        (0x0012)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA1CCR1                                        (0x0014)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA1CCR2                                        (0x0016)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA1CCR3                                        (0x0018)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA1CCR4                                        (0x001A)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA1EX0                                         (0x0020)              /*!< TimerAx Expansion 0 Register */
#define OFS_TA1IV                                          (0x002E)              /*!< TimerAx Interrupt Vector Register */


/******************************************************************************
* TIMER_A2 Registers
******************************************************************************/
#define TA2CTL                                   (HWREG16(0x40000800))           /*!< TimerAx Control Register */
#define TA2CCTL0                                 (HWREG16(0x40000802))           /*!< Timer_A Capture/Compare Control Register */
#define TA2CCTL1                                 (HWREG16(0x40000804))           /*!< Timer_A Capture/Compare Control Register */
#define TA2CCTL2                                 (HWREG16(0x40000806))           /*!< Timer_A Capture/Compare Control Register */
#define TA2CCTL3                                 (HWREG16(0x40000808))           /*!< Timer_A Capture/Compare Control Register */
#define TA2CCTL4                                 (HWREG16(0x4000080A))           /*!< Timer_A Capture/Compare Control Register */
#define TA2R                                     (HWREG16(0x40000810))           /*!< TimerA register */
#define TA2CCR0                                  (HWREG16(0x40000812))           /*!< Timer_A Capture/Compare  Register */
#define TA2CCR1                                  (HWREG16(0x40000814))           /*!< Timer_A Capture/Compare  Register */
#define TA2CCR2                                  (HWREG16(0x40000816))           /*!< Timer_A Capture/Compare  Register */
#define TA2CCR3                                  (HWREG16(0x40000818))           /*!< Timer_A Capture/Compare  Register */
#define TA2CCR4                                  (HWREG16(0x4000081A))           /*!< Timer_A Capture/Compare  Register */
#define TA2EX0                                   (HWREG16(0x40000820))           /*!< TimerAx Expansion 0 Register */
#define TA2IV                                    (HWREG16(0x4000082E))           /*!< TimerAx Interrupt Vector Register */

/* Register offsets from TIMER_A2_BASE address */
#define OFS_TA2CTL                                         (0x0000)              /*!< TimerAx Control Register */
#define OFS_TA2CCTL0                                       (0x0002)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA2CCTL1                                       (0x0004)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA2CCTL2                                       (0x0006)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA2CCTL3                                       (0x0008)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA2CCTL4                                       (0x000A)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA2R                                           (0x0010)              /*!< TimerA register */
#define OFS_TA2CCR0                                        (0x0012)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA2CCR1                                        (0x0014)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA2CCR2                                        (0x0016)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA2CCR3                                        (0x0018)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA2CCR4                                        (0x001A)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA2EX0                                         (0x0020)              /*!< TimerAx Expansion 0 Register */
#define OFS_TA2IV                                          (0x002E)              /*!< TimerAx Interrupt Vector Register */


/******************************************************************************
* TIMER_A3 Registers
******************************************************************************/
#define TA3CTL                                   (HWREG16(0x40000C00))           /*!< TimerAx Control Register */
#define TA3CCTL0                                 (HWREG16(0x40000C02))           /*!< Timer_A Capture/Compare Control Register */
#define TA3CCTL1                                 (HWREG16(0x40000C04))           /*!< Timer_A Capture/Compare Control Register */
#define TA3CCTL2                                 (HWREG16(0x40000C06))           /*!< Timer_A Capture/Compare Control Register */
#define TA3CCTL3                                 (HWREG16(0x40000C08))           /*!< Timer_A Capture/Compare Control Register */
#define TA3CCTL4                                 (HWREG16(0x40000C0A))           /*!< Timer_A Capture/Compare Control Register */
#define TA3R                                     (HWREG16(0x40000C10))           /*!< TimerA register */
#define TA3CCR0                                  (HWREG16(0x40000C12))           /*!< Timer_A Capture/Compare  Register */
#define TA3CCR1                                  (HWREG16(0x40000C14))           /*!< Timer_A Capture/Compare  Register */
#define TA3CCR2                                  (HWREG16(0x40000C16))           /*!< Timer_A Capture/Compare  Register */
#define TA3CCR3                                  (HWREG16(0x40000C18))           /*!< Timer_A Capture/Compare  Register */
#define TA3CCR4                                  (HWREG16(0x40000C1A))           /*!< Timer_A Capture/Compare  Register */
#define TA3EX0                                   (HWREG16(0x40000C20))           /*!< TimerAx Expansion 0 Register */
#define TA3IV                                    (HWREG16(0x40000C2E))           /*!< TimerAx Interrupt Vector Register */

/* Register offsets from TIMER_A3_BASE address */
#define OFS_TA3CTL                                         (0x0000)              /*!< TimerAx Control Register */
#define OFS_TA3CCTL0                                       (0x0002)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA3CCTL1                                       (0x0004)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA3CCTL2                                       (0x0006)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA3CCTL3                                       (0x0008)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA3CCTL4                                       (0x000A)              /*!< Timer_A Capture/Compare Control Register */
#define OFS_TA3R                                           (0x0010)              /*!< TimerA register */
#define OFS_TA3CCR0                                        (0x0012)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA3CCR1                                        (0x0014)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA3CCR2                                        (0x0016)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA3CCR3                                        (0x0018)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA3CCR4                                        (0x001A)              /*!< Timer_A Capture/Compare  Register */
#define OFS_TA3EX0                                         (0x0020)              /*!< TimerAx Expansion 0 Register */
#define OFS_TA3IV                                          (0x002E)              /*!< TimerAx Interrupt Vector Register */


/******************************************************************************
* WDT_A Registers
******************************************************************************/
#define WDTCTL                                   (HWREG16(0x4000480C))           /*!< Watchdog Timer Control Register */

/* Register offsets from WDT_A_BASE address */
#define OFS_WDTCTL                                         (0x000C)              /*!< Watchdog Timer Control Register */


/******************************************************************************
* Peripheral register control bits (legacy section)                           *
******************************************************************************/

/******************************************************************************
* AES256 Bits (legacy section)
******************************************************************************/
/* AESACTL0[AESOP] Bits */
#define AESOP_OFS                                AES256_CTL0_OP_OFS              /*!< AESOP Offset */
#define AESOP_M                                  AES256_CTL0_OP_MASK             /*!< AES operation */
#define AESOP0                                   AES256_CTL0_OP0                 /*!< AESOP Bit 0 */
#define AESOP1                                   AES256_CTL0_OP1                 /*!< AESOP Bit 1 */
#define AESOP_0                                  AES256_CTL0_OP_0                /*!< Encryption */
#define AESOP_1                                  AES256_CTL0_OP_1                /*!< Decryption. The provided key is the same key used for encryption */
#define AESOP_2                                  AES256_CTL0_OP_2                /*!< Generate first round key required for decryption */
#define AESOP_3                                  AES256_CTL0_OP_3                /*!< Decryption. The provided key is the first round key required for decryption */
/* AESACTL0[AESKL] Bits */
#define AESKL_OFS                                AES256_CTL0_KL_OFS              /*!< AESKL Offset */
#define AESKL_M                                  AES256_CTL0_KL_MASK             /*!< AES key length */
#define AESKL0                                   AES256_CTL0_KL0                 /*!< AESKL Bit 0 */
#define AESKL1                                   AES256_CTL0_KL1                 /*!< AESKL Bit 1 */
#define AESKL_0                                  AES256_CTL0_KL_0                /*!< AES128. The key size is 128 bit */
#define AESKL_1                                  AES256_CTL0_KL_1                /*!< AES192. The key size is 192 bit. */
#define AESKL_2                                  AES256_CTL0_KL_2                /*!< AES256. The key size is 256 bit */
#define AESKL__128BIT                            AES256_CTL0_KL__128BIT          /*!< AES128. The key size is 128 bit */
#define AESKL__192BIT                            AES256_CTL0_KL__192BIT          /*!< AES192. The key size is 192 bit. */
#define AESKL__256BIT                            AES256_CTL0_KL__256BIT          /*!< AES256. The key size is 256 bit */
/* AESACTL0[AESCM] Bits */
#define AESCM_OFS                                AES256_CTL0_CM_OFS              /*!< AESCM Offset */
#define AESCM_M                                  AES256_CTL0_CM_MASK             /*!< AES cipher mode select */
#define AESCM0                                   AES256_CTL0_CM0                 /*!< AESCM Bit 0 */
#define AESCM1                                   AES256_CTL0_CM1                 /*!< AESCM Bit 1 */
#define AESCM_0                                  AES256_CTL0_CM_0                /*!< ECB */
#define AESCM_1                                  AES256_CTL0_CM_1                /*!< CBC */
#define AESCM_2                                  AES256_CTL0_CM_2                /*!< OFB */
#define AESCM_3                                  AES256_CTL0_CM_3                /*!< CFB */
#define AESCM__ECB                               AES256_CTL0_CM__ECB             /*!< ECB */
#define AESCM__CBC                               AES256_CTL0_CM__CBC             /*!< CBC */
#define AESCM__OFB                               AES256_CTL0_CM__OFB             /*!< OFB */
#define AESCM__CFB                               AES256_CTL0_CM__CFB             /*!< CFB */
/* AESACTL0[AESSWRST] Bits */
#define AESSWRST_OFS                             AES256_CTL0_SWRST_OFS           /*!< AESSWRST Offset */
#define AESSWRST                                 AES256_CTL0_SWRST               /*!< AES software reset */
/* AESACTL0[AESRDYIFG] Bits */
#define AESRDYIFG_OFS                            AES256_CTL0_RDYIFG_OFS          /*!< AESRDYIFG Offset */
#define AESRDYIFG                                AES256_CTL0_RDYIFG              /*!< AES ready interrupt flag */
/* AESACTL0[AESERRFG] Bits */
#define AESERRFG_OFS                             AES256_CTL0_ERRFG_OFS           /*!< AESERRFG Offset */
#define AESERRFG                                 AES256_CTL0_ERRFG               /*!< AES error flag */
/* AESACTL0[AESRDYIE] Bits */
#define AESRDYIE_OFS                             AES256_CTL0_RDYIE_OFS           /*!< AESRDYIE Offset */
#define AESRDYIE                                 AES256_CTL0_RDYIE               /*!< AES ready interrupt enable */
/* AESACTL0[AESCMEN] Bits */
#define AESCMEN_OFS                              AES256_CTL0_CMEN_OFS            /*!< AESCMEN Offset */
#define AESCMEN                                  AES256_CTL0_CMEN                /*!< AES cipher mode enable */
/* AESACTL1[AESBLKCNT] Bits */
#define AESBLKCNT_OFS                            AES256_CTL1_BLKCNT_OFS          /*!< AESBLKCNT Offset */
#define AESBLKCNT_M                              AES256_CTL1_BLKCNT_MASK         /*!< Cipher Block Counter */
#define AESBLKCNT0                               AES256_CTL1_BLKCNT0             /*!< AESBLKCNT Bit 0 */
#define AESBLKCNT1                               AES256_CTL1_BLKCNT1             /*!< AESBLKCNT Bit 1 */
#define AESBLKCNT2                               AES256_CTL1_BLKCNT2             /*!< AESBLKCNT Bit 2 */
#define AESBLKCNT3                               AES256_CTL1_BLKCNT3             /*!< AESBLKCNT Bit 3 */
#define AESBLKCNT4                               AES256_CTL1_BLKCNT4             /*!< AESBLKCNT Bit 4 */
#define AESBLKCNT5                               AES256_CTL1_BLKCNT5             /*!< AESBLKCNT Bit 5 */
#define AESBLKCNT6                               AES256_CTL1_BLKCNT6             /*!< AESBLKCNT Bit 6 */
#define AESBLKCNT7                               AES256_CTL1_BLKCNT7             /*!< AESBLKCNT Bit 7 */
/* AESASTAT[AESBUSY] Bits */
#define AESBUSY_OFS                              AES256_STAT_BUSY_OFS            /*!< AESBUSY Offset */
#define AESBUSY                                  AES256_STAT_BUSY                /*!< AES accelerator module busy */
/* AESASTAT[AESKEYWR] Bits */
#define AESKEYWR_OFS                             AES256_STAT_KEYWR_OFS           /*!< AESKEYWR Offset */
#define AESKEYWR                                 AES256_STAT_KEYWR               /*!< All 16 bytes written to AESAKEY */
/* AESASTAT[AESDINWR] Bits */
#define AESDINWR_OFS                             AES256_STAT_DINWR_OFS           /*!< AESDINWR Offset */
#define AESDINWR                                 AES256_STAT_DINWR               /*!< All 16 bytes written to AESADIN, AESAXDIN or AESAXIN */
/* AESASTAT[AESDOUTRD] Bits */
#define AESDOUTRD_OFS                            AES256_STAT_DOUTRD_OFS          /*!< AESDOUTRD Offset */
#define AESDOUTRD                                AES256_STAT_DOUTRD              /*!< All 16 bytes read from AESADOUT */
/* AESASTAT[AESKEYCNT] Bits */
#define AESKEYCNT_OFS                            AES256_STAT_KEYCNT_OFS          /*!< AESKEYCNT Offset */
#define AESKEYCNT_M                              AES256_STAT_KEYCNT_MASK         /*!< Bytes written via AESAKEY for AESKLx=00, half-words written via AESAKEY */
#define AESKEYCNT0                               AES256_STAT_KEYCNT0             /*!< AESKEYCNT Bit 0 */
#define AESKEYCNT1                               AES256_STAT_KEYCNT1             /*!< AESKEYCNT Bit 1 */
#define AESKEYCNT2                               AES256_STAT_KEYCNT2             /*!< AESKEYCNT Bit 2 */
#define AESKEYCNT3                               AES256_STAT_KEYCNT3             /*!< AESKEYCNT Bit 3 */
/* AESASTAT[AESDINCNT] Bits */
#define AESDINCNT_OFS                            AES256_STAT_DINCNT_OFS          /*!< AESDINCNT Offset */
#define AESDINCNT_M                              AES256_STAT_DINCNT_MASK         /*!< Bytes written via AESADIN, AESAXDIN or AESAXIN */
#define AESDINCNT0                               AES256_STAT_DINCNT0             /*!< AESDINCNT Bit 0 */
#define AESDINCNT1                               AES256_STAT_DINCNT1             /*!< AESDINCNT Bit 1 */
#define AESDINCNT2                               AES256_STAT_DINCNT2             /*!< AESDINCNT Bit 2 */
#define AESDINCNT3                               AES256_STAT_DINCNT3             /*!< AESDINCNT Bit 3 */
/* AESASTAT[AESDOUTCNT] Bits */
#define AESDOUTCNT_OFS                           AES256_STAT_DOUTCNT_OFS         /*!< AESDOUTCNT Offset */
#define AESDOUTCNT_M                             AES256_STAT_DOUTCNT_MASK        /*!< Bytes read via AESADOUT */
#define AESDOUTCNT0                              AES256_STAT_DOUTCNT0            /*!< AESDOUTCNT Bit 0 */
#define AESDOUTCNT1                              AES256_STAT_DOUTCNT1            /*!< AESDOUTCNT Bit 1 */
#define AESDOUTCNT2                              AES256_STAT_DOUTCNT2            /*!< AESDOUTCNT Bit 2 */
#define AESDOUTCNT3                              AES256_STAT_DOUTCNT3            /*!< AESDOUTCNT Bit 3 */
/* AESAKEY[AESKEY0] Bits */
#define AESKEY0_OFS                              AES256_KEY_KEY0_OFS             /*!< AESKEY0 Offset */
#define AESKEY0_M                                AES256_KEY_KEY0_MASK            /*!< AES key byte n when AESAKEY is written as half-word */
#define AESKEY00                                 AES256_KEY_KEY00                /*!< AESKEY0 Bit 0 */
#define AESKEY01                                 AES256_KEY_KEY01                /*!< AESKEY0 Bit 1 */
#define AESKEY02                                 AES256_KEY_KEY02                /*!< AESKEY0 Bit 2 */
#define AESKEY03                                 AES256_KEY_KEY03                /*!< AESKEY0 Bit 3 */
#define AESKEY04                                 AES256_KEY_KEY04                /*!< AESKEY0 Bit 4 */
#define AESKEY05                                 AES256_KEY_KEY05                /*!< AESKEY0 Bit 5 */
#define AESKEY06                                 AES256_KEY_KEY06                /*!< AESKEY0 Bit 6 */
#define AESKEY07                                 AES256_KEY_KEY07                /*!< AESKEY0 Bit 7 */
/* AESAKEY[AESKEY1] Bits */
#define AESKEY1_OFS                              AES256_KEY_KEY1_OFS             /*!< AESKEY1 Offset */
#define AESKEY1_M                                AES256_KEY_KEY1_MASK            /*!< AES key byte n+1 when AESAKEY is written as half-word */
#define AESKEY10                                 AES256_KEY_KEY10                /*!< AESKEY1 Bit 0 */
#define AESKEY11                                 AES256_KEY_KEY11                /*!< AESKEY1 Bit 1 */
#define AESKEY12                                 AES256_KEY_KEY12                /*!< AESKEY1 Bit 2 */
#define AESKEY13                                 AES256_KEY_KEY13                /*!< AESKEY1 Bit 3 */
#define AESKEY14                                 AES256_KEY_KEY14                /*!< AESKEY1 Bit 4 */
#define AESKEY15                                 AES256_KEY_KEY15                /*!< AESKEY1 Bit 5 */
#define AESKEY16                                 AES256_KEY_KEY16                /*!< AESKEY1 Bit 6 */
#define AESKEY17                                 AES256_KEY_KEY17                /*!< AESKEY1 Bit 7 */
/* AESADIN[AESDIN0] Bits */
#define AESDIN0_OFS                              AES256_DIN_DIN0_OFS             /*!< AESDIN0 Offset */
#define AESDIN0_M                                AES256_DIN_DIN0_MASK            /*!< AES data in byte n when AESADIN is written as half-word */
#define AESDIN00                                 AES256_DIN_DIN00                /*!< AESDIN0 Bit 0 */
#define AESDIN01                                 AES256_DIN_DIN01                /*!< AESDIN0 Bit 1 */
#define AESDIN02                                 AES256_DIN_DIN02                /*!< AESDIN0 Bit 2 */
#define AESDIN03                                 AES256_DIN_DIN03                /*!< AESDIN0 Bit 3 */
#define AESDIN04                                 AES256_DIN_DIN04                /*!< AESDIN0 Bit 4 */
#define AESDIN05                                 AES256_DIN_DIN05                /*!< AESDIN0 Bit 5 */
#define AESDIN06                                 AES256_DIN_DIN06                /*!< AESDIN0 Bit 6 */
#define AESDIN07                                 AES256_DIN_DIN07                /*!< AESDIN0 Bit 7 */
/* AESADIN[AESDIN1] Bits */
#define AESDIN1_OFS                              AES256_DIN_DIN1_OFS             /*!< AESDIN1 Offset */
#define AESDIN1_M                                AES256_DIN_DIN1_MASK            /*!< AES data in byte n+1 when AESADIN is written as half-word */
#define AESDIN10                                 AES256_DIN_DIN10                /*!< AESDIN1 Bit 0 */
#define AESDIN11                                 AES256_DIN_DIN11                /*!< AESDIN1 Bit 1 */
#define AESDIN12                                 AES256_DIN_DIN12                /*!< AESDIN1 Bit 2 */
#define AESDIN13                                 AES256_DIN_DIN13                /*!< AESDIN1 Bit 3 */
#define AESDIN14                                 AES256_DIN_DIN14                /*!< AESDIN1 Bit 4 */
#define AESDIN15                                 AES256_DIN_DIN15                /*!< AESDIN1 Bit 5 */
#define AESDIN16                                 AES256_DIN_DIN16                /*!< AESDIN1 Bit 6 */
#define AESDIN17                                 AES256_DIN_DIN17                /*!< AESDIN1 Bit 7 */
/* AESADOUT[AESDOUT0] Bits */
#define AESDOUT0_OFS                             AES256_DOUT_DOUT0_OFS           /*!< AESDOUT0 Offset */
#define AESDOUT0_M                               AES256_DOUT_DOUT0_MASK          /*!< AES data out byte n when AESADOUT is read as half-word */
#define AESDOUT00                                AES256_DOUT_DOUT00              /*!< AESDOUT0 Bit 0 */
#define AESDOUT01                                AES256_DOUT_DOUT01              /*!< AESDOUT0 Bit 1 */
#define AESDOUT02                                AES256_DOUT_DOUT02              /*!< AESDOUT0 Bit 2 */
#define AESDOUT03                                AES256_DOUT_DOUT03              /*!< AESDOUT0 Bit 3 */
#define AESDOUT04                                AES256_DOUT_DOUT04              /*!< AESDOUT0 Bit 4 */
#define AESDOUT05                                AES256_DOUT_DOUT05              /*!< AESDOUT0 Bit 5 */
#define AESDOUT06                                AES256_DOUT_DOUT06              /*!< AESDOUT0 Bit 6 */
#define AESDOUT07                                AES256_DOUT_DOUT07              /*!< AESDOUT0 Bit 7 */
/* AESADOUT[AESDOUT1] Bits */
#define AESDOUT1_OFS                             AES256_DOUT_DOUT1_OFS           /*!< AESDOUT1 Offset */
#define AESDOUT1_M                               AES256_DOUT_DOUT1_MASK          /*!< AES data out byte n+1 when AESADOUT is read as half-word */
#define AESDOUT10                                AES256_DOUT_DOUT10              /*!< AESDOUT1 Bit 0 */
#define AESDOUT11                                AES256_DOUT_DOUT11              /*!< AESDOUT1 Bit 1 */
#define AESDOUT12                                AES256_DOUT_DOUT12              /*!< AESDOUT1 Bit 2 */
#define AESDOUT13                                AES256_DOUT_DOUT13              /*!< AESDOUT1 Bit 3 */
#define AESDOUT14                                AES256_DOUT_DOUT14              /*!< AESDOUT1 Bit 4 */
#define AESDOUT15                                AES256_DOUT_DOUT15              /*!< AESDOUT1 Bit 5 */
#define AESDOUT16                                AES256_DOUT_DOUT16              /*!< AESDOUT1 Bit 6 */
#define AESDOUT17                                AES256_DOUT_DOUT17              /*!< AESDOUT1 Bit 7 */
/* AESAXDIN[AESXDIN0] Bits */
#define AESXDIN0_OFS                             AES256_XDIN_XDIN0_OFS           /*!< AESXDIN0 Offset */
#define AESXDIN0_M                               AES256_XDIN_XDIN0_MASK          /*!< AES data in byte n when AESAXDIN is written as half-word */
#define AESXDIN00                                AES256_XDIN_XDIN00              /*!< AESXDIN0 Bit 0 */
#define AESXDIN01                                AES256_XDIN_XDIN01              /*!< AESXDIN0 Bit 1 */
#define AESXDIN02                                AES256_XDIN_XDIN02              /*!< AESXDIN0 Bit 2 */
#define AESXDIN03                                AES256_XDIN_XDIN03              /*!< AESXDIN0 Bit 3 */
#define AESXDIN04                                AES256_XDIN_XDIN04              /*!< AESXDIN0 Bit 4 */
#define AESXDIN05                                AES256_XDIN_XDIN05              /*!< AESXDIN0 Bit 5 */
#define AESXDIN06                                AES256_XDIN_XDIN06              /*!< AESXDIN0 Bit 6 */
#define AESXDIN07                                AES256_XDIN_XDIN07              /*!< AESXDIN0 Bit 7 */
/* AESAXDIN[AESXDIN1] Bits */
#define AESXDIN1_OFS                             AES256_XDIN_XDIN1_OFS           /*!< AESXDIN1 Offset */
#define AESXDIN1_M                               AES256_XDIN_XDIN1_MASK          /*!< AES data in byte n+1 when AESAXDIN is written as half-word */
#define AESXDIN10                                AES256_XDIN_XDIN10              /*!< AESXDIN1 Bit 0 */
#define AESXDIN11                                AES256_XDIN_XDIN11              /*!< AESXDIN1 Bit 1 */
#define AESXDIN12                                AES256_XDIN_XDIN12              /*!< AESXDIN1 Bit 2 */
#define AESXDIN13                                AES256_XDIN_XDIN13              /*!< AESXDIN1 Bit 3 */
#define AESXDIN14                                AES256_XDIN_XDIN14              /*!< AESXDIN1 Bit 4 */
#define AESXDIN15                                AES256_XDIN_XDIN15              /*!< AESXDIN1 Bit 5 */
#define AESXDIN16                                AES256_XDIN_XDIN16              /*!< AESXDIN1 Bit 6 */
#define AESXDIN17                                AES256_XDIN_XDIN17              /*!< AESXDIN1 Bit 7 */
/* AESAXIN[AESXIN0] Bits */
#define AESXIN0_OFS                              AES256_XIN_XIN0_OFS             /*!< AESXIN0 Offset */
#define AESXIN0_M                                AES256_XIN_XIN0_MASK            /*!< AES data in byte n when AESAXIN is written as half-word */
#define AESXIN00                                 AES256_XIN_XIN00                /*!< AESXIN0 Bit 0 */
#define AESXIN01                                 AES256_XIN_XIN01                /*!< AESXIN0 Bit 1 */
#define AESXIN02                                 AES256_XIN_XIN02                /*!< AESXIN0 Bit 2 */
#define AESXIN03                                 AES256_XIN_XIN03                /*!< AESXIN0 Bit 3 */
#define AESXIN04                                 AES256_XIN_XIN04                /*!< AESXIN0 Bit 4 */
#define AESXIN05                                 AES256_XIN_XIN05                /*!< AESXIN0 Bit 5 */
#define AESXIN06                                 AES256_XIN_XIN06                /*!< AESXIN0 Bit 6 */
#define AESXIN07                                 AES256_XIN_XIN07                /*!< AESXIN0 Bit 7 */
/* AESAXIN[AESXIN1] Bits */
#define AESXIN1_OFS                              AES256_XIN_XIN1_OFS             /*!< AESXIN1 Offset */
#define AESXIN1_M                                AES256_XIN_XIN1_MASK            /*!< AES data in byte n+1 when AESAXIN is written as half-word */
#define AESXIN10                                 AES256_XIN_XIN10                /*!< AESXIN1 Bit 0 */
#define AESXIN11                                 AES256_XIN_XIN11                /*!< AESXIN1 Bit 1 */
#define AESXIN12                                 AES256_XIN_XIN12                /*!< AESXIN1 Bit 2 */
#define AESXIN13                                 AES256_XIN_XIN13                /*!< AESXIN1 Bit 3 */
#define AESXIN14                                 AES256_XIN_XIN14                /*!< AESXIN1 Bit 4 */
#define AESXIN15                                 AES256_XIN_XIN15                /*!< AESXIN1 Bit 5 */
#define AESXIN16                                 AES256_XIN_XIN16                /*!< AESXIN1 Bit 6 */
#define AESXIN17                                 AES256_XIN_XIN17                /*!< AESXIN1 Bit 7 */

/******************************************************************************
* CAPTIO Bits (legacy section)
******************************************************************************/
/* CAPTIO0CTL[CAPTIOPISEL] Bits */
#define CAPTIOPISEL_OFS                          CAPTIO_CTL_PISEL_OFS            /*!< CAPTIOPISEL Offset */
#define CAPTIOPISEL_M                            CAPTIO_CTL_PISEL_MASK           /*!< Capacitive Touch IO pin select */
#define CAPTIOPISEL0                             CAPTIO_CTL_PISEL0               /*!< CAPTIOPISEL Bit 0 */
#define CAPTIOPISEL1                             CAPTIO_CTL_PISEL1               /*!< CAPTIOPISEL Bit 1 */
#define CAPTIOPISEL2                             CAPTIO_CTL_PISEL2               /*!< CAPTIOPISEL Bit 2 */
#define CAPTIOPISEL_0                            CAPTIO_CTL_PISEL_0              /*!< Px.0 */
#define CAPTIOPISEL_1                            CAPTIO_CTL_PISEL_1              /*!< Px.1 */
#define CAPTIOPISEL_2                            CAPTIO_CTL_PISEL_2              /*!< Px.2 */
#define CAPTIOPISEL_3                            CAPTIO_CTL_PISEL_3              /*!< Px.3 */
#define CAPTIOPISEL_4                            CAPTIO_CTL_PISEL_4              /*!< Px.4 */
#define CAPTIOPISEL_5                            CAPTIO_CTL_PISEL_5              /*!< Px.5 */
#define CAPTIOPISEL_6                            CAPTIO_CTL_PISEL_6              /*!< Px.6 */
#define CAPTIOPISEL_7                            CAPTIO_CTL_PISEL_7              /*!< Px.7 */
/* CAPTIO0CTL[CAPTIOPOSEL] Bits */
#define CAPTIOPOSEL_OFS                          CAPTIO_CTL_POSEL_OFS            /*!< CAPTIOPOSEL Offset */
#define CAPTIOPOSEL_M                            CAPTIO_CTL_POSEL_MASK           /*!< Capacitive Touch IO port select */
#define CAPTIOPOSEL0                             CAPTIO_CTL_POSEL0               /*!< CAPTIOPOSEL Bit 0 */
#define CAPTIOPOSEL1                             CAPTIO_CTL_POSEL1               /*!< CAPTIOPOSEL Bit 1 */
#define CAPTIOPOSEL2                             CAPTIO_CTL_POSEL2               /*!< CAPTIOPOSEL Bit 2 */
#define CAPTIOPOSEL3                             CAPTIO_CTL_POSEL3               /*!< CAPTIOPOSEL Bit 3 */
#define CAPTIOPOSEL_0                            CAPTIO_CTL_POSEL_0              /*!< Px = PJ */
#define CAPTIOPOSEL_1                            CAPTIO_CTL_POSEL_1              /*!< Px = P1 */
#define CAPTIOPOSEL_2                            CAPTIO_CTL_POSEL_2              /*!< Px = P2 */
#define CAPTIOPOSEL_3                            CAPTIO_CTL_POSEL_3              /*!< Px = P3 */
#define CAPTIOPOSEL_4                            CAPTIO_CTL_POSEL_4              /*!< Px = P4 */
#define CAPTIOPOSEL_5                            CAPTIO_CTL_POSEL_5              /*!< Px = P5 */
#define CAPTIOPOSEL_6                            CAPTIO_CTL_POSEL_6              /*!< Px = P6 */
#define CAPTIOPOSEL_7                            CAPTIO_CTL_POSEL_7              /*!< Px = P7 */
#define CAPTIOPOSEL_8                            CAPTIO_CTL_POSEL_8              /*!< Px = P8 */
#define CAPTIOPOSEL_9                            CAPTIO_CTL_POSEL_9              /*!< Px = P9 */
#define CAPTIOPOSEL_10                           CAPTIO_CTL_POSEL_10             /*!< Px = P10 */
#define CAPTIOPOSEL_11                           CAPTIO_CTL_POSEL_11             /*!< Px = P11 */
#define CAPTIOPOSEL_12                           CAPTIO_CTL_POSEL_12             /*!< Px = P12 */
#define CAPTIOPOSEL_13                           CAPTIO_CTL_POSEL_13             /*!< Px = P13 */
#define CAPTIOPOSEL_14                           CAPTIO_CTL_POSEL_14             /*!< Px = P14 */
#define CAPTIOPOSEL_15                           CAPTIO_CTL_POSEL_15             /*!< Px = P15 */
#define CAPTIOPOSEL__PJ                          CAPTIO_CTL_POSEL__PJ            /*!< Px = PJ */
#define CAPTIOPOSEL__P1                          CAPTIO_CTL_POSEL__P1            /*!< Px = P1 */
#define CAPTIOPOSEL__P2                          CAPTIO_CTL_POSEL__P2            /*!< Px = P2 */
#define CAPTIOPOSEL__P3                          CAPTIO_CTL_POSEL__P3            /*!< Px = P3 */
#define CAPTIOPOSEL__P4                          CAPTIO_CTL_POSEL__P4            /*!< Px = P4 */
#define CAPTIOPOSEL__P5                          CAPTIO_CTL_POSEL__P5            /*!< Px = P5 */
#define CAPTIOPOSEL__P6                          CAPTIO_CTL_POSEL__P6            /*!< Px = P6 */
#define CAPTIOPOSEL__P7                          CAPTIO_CTL_POSEL__P7            /*!< Px = P7 */
#define CAPTIOPOSEL__P8                          CAPTIO_CTL_POSEL__P8            /*!< Px = P8 */
#define CAPTIOPOSEL__P9                          CAPTIO_CTL_POSEL__P9            /*!< Px = P9 */
#define CAPTIOPOSEL__P10                         CAPTIO_CTL_POSEL__P10           /*!< Px = P10 */
#define CAPTIOPOSEL__P11                         CAPTIO_CTL_POSEL__P11           /*!< Px = P11 */
#define CAPTIOPOSEL__P12                         CAPTIO_CTL_POSEL__P12           /*!< Px = P12 */
#define CAPTIOPOSEL__P13                         CAPTIO_CTL_POSEL__P13           /*!< Px = P13 */
#define CAPTIOPOSEL__P14                         CAPTIO_CTL_POSEL__P14           /*!< Px = P14 */
#define CAPTIOPOSEL__P15                         CAPTIO_CTL_POSEL__P15           /*!< Px = P15 */
/* CAPTIO0CTL[CAPTIOEN] Bits */
#define CAPTIOEN_OFS                             CAPTIO_CTL_EN_OFS               /*!< CAPTIOEN Offset */
#define CAPTIOEN                                 CAPTIO_CTL_EN                   /*!< Capacitive Touch IO enable */
/* CAPTIO0CTL[CAPTIOSTATE] Bits */
#define CAPTIOSTATE_OFS                          CAPTIO_CTL_STATE_OFS            /*!< CAPTIOSTATE Offset */
#define CAPTIOSTATE                              CAPTIO_CTL_STATE                /*!< Capacitive Touch IO state */

/******************************************************************************
* COMP_E Bits (legacy section)
******************************************************************************/
/* CE0CTL0[CEIPSEL] Bits */
#define CEIPSEL_OFS                              COMP_E_CTL0_IPSEL_OFS           /*!< CEIPSEL Offset */
#define CEIPSEL_M                                COMP_E_CTL0_IPSEL_MASK          /*!< Channel input selected for the V+ terminal */
#define CEIPSEL0                                 COMP_E_CTL0_IPSEL0              /*!< CEIPSEL Bit 0 */
#define CEIPSEL1                                 COMP_E_CTL0_IPSEL1              /*!< CEIPSEL Bit 1 */
#define CEIPSEL2                                 COMP_E_CTL0_IPSEL2              /*!< CEIPSEL Bit 2 */
#define CEIPSEL3                                 COMP_E_CTL0_IPSEL3              /*!< CEIPSEL Bit 3 */
#define CEIPSEL_0                                COMP_E_CTL0_IPSEL_0             /*!< Channel 0 selected */
#define CEIPSEL_1                                COMP_E_CTL0_IPSEL_1             /*!< Channel 1 selected */
#define CEIPSEL_2                                COMP_E_CTL0_IPSEL_2             /*!< Channel 2 selected */
#define CEIPSEL_3                                COMP_E_CTL0_IPSEL_3             /*!< Channel 3 selected */
#define CEIPSEL_4                                COMP_E_CTL0_IPSEL_4             /*!< Channel 4 selected */
#define CEIPSEL_5                                COMP_E_CTL0_IPSEL_5             /*!< Channel 5 selected */
#define CEIPSEL_6                                COMP_E_CTL0_IPSEL_6             /*!< Channel 6 selected */
#define CEIPSEL_7                                COMP_E_CTL0_IPSEL_7             /*!< Channel 7 selected */
#define CEIPSEL_8                                COMP_E_CTL0_IPSEL_8             /*!< Channel 8 selected */
#define CEIPSEL_9                                COMP_E_CTL0_IPSEL_9             /*!< Channel 9 selected */
#define CEIPSEL_10                               COMP_E_CTL0_IPSEL_10            /*!< Channel 10 selected */
#define CEIPSEL_11                               COMP_E_CTL0_IPSEL_11            /*!< Channel 11 selected */
#define CEIPSEL_12                               COMP_E_CTL0_IPSEL_12            /*!< Channel 12 selected */
#define CEIPSEL_13                               COMP_E_CTL0_IPSEL_13            /*!< Channel 13 selected */
#define CEIPSEL_14                               COMP_E_CTL0_IPSEL_14            /*!< Channel 14 selected */
#define CEIPSEL_15                               COMP_E_CTL0_IPSEL_15            /*!< Channel 15 selected */
/* CE0CTL0[CEIPEN] Bits */
#define CEIPEN_OFS                               COMP_E_CTL0_IPEN_OFS            /*!< CEIPEN Offset */
#define CEIPEN                                   COMP_E_CTL0_IPEN                /*!< Channel input enable for the V+ terminal */
/* CE0CTL0[CEIMSEL] Bits */
#define CEIMSEL_OFS                              COMP_E_CTL0_IMSEL_OFS           /*!< CEIMSEL Offset */
#define CEIMSEL_M                                COMP_E_CTL0_IMSEL_MASK          /*!< Channel input selected for the - terminal */
#define CEIMSEL0                                 COMP_E_CTL0_IMSEL0              /*!< CEIMSEL Bit 0 */
#define CEIMSEL1                                 COMP_E_CTL0_IMSEL1              /*!< CEIMSEL Bit 1 */
#define CEIMSEL2                                 COMP_E_CTL0_IMSEL2              /*!< CEIMSEL Bit 2 */
#define CEIMSEL3                                 COMP_E_CTL0_IMSEL3              /*!< CEIMSEL Bit 3 */
#define CEIMSEL_0                                COMP_E_CTL0_IMSEL_0             /*!< Channel 0 selected */
#define CEIMSEL_1                                COMP_E_CTL0_IMSEL_1             /*!< Channel 1 selected */
#define CEIMSEL_2                                COMP_E_CTL0_IMSEL_2             /*!< Channel 2 selected */
#define CEIMSEL_3                                COMP_E_CTL0_IMSEL_3             /*!< Channel 3 selected */
#define CEIMSEL_4                                COMP_E_CTL0_IMSEL_4             /*!< Channel 4 selected */
#define CEIMSEL_5                                COMP_E_CTL0_IMSEL_5             /*!< Channel 5 selected */
#define CEIMSEL_6                                COMP_E_CTL0_IMSEL_6             /*!< Channel 6 selected */
#define CEIMSEL_7                                COMP_E_CTL0_IMSEL_7             /*!< Channel 7 selected */
#define CEIMSEL_8                                COMP_E_CTL0_IMSEL_8             /*!< Channel 8 selected */
#define CEIMSEL_9                                COMP_E_CTL0_IMSEL_9             /*!< Channel 9 selected */
#define CEIMSEL_10                               COMP_E_CTL0_IMSEL_10            /*!< Channel 10 selected */
#define CEIMSEL_11                               COMP_E_CTL0_IMSEL_11            /*!< Channel 11 selected */
#define CEIMSEL_12                               COMP_E_CTL0_IMSEL_12            /*!< Channel 12 selected */
#define CEIMSEL_13                               COMP_E_CTL0_IMSEL_13            /*!< Channel 13 selected */
#define CEIMSEL_14                               COMP_E_CTL0_IMSEL_14            /*!< Channel 14 selected */
#define CEIMSEL_15                               COMP_E_CTL0_IMSEL_15            /*!< Channel 15 selected */
/* CE0CTL0[CEIMEN] Bits */
#define CEIMEN_OFS                               COMP_E_CTL0_IMEN_OFS            /*!< CEIMEN Offset */
#define CEIMEN                                   COMP_E_CTL0_IMEN                /*!< Channel input enable for the - terminal */
/* CE0CTL1[CEOUT] Bits */
#define CEOUT_OFS                                COMP_E_CTL1_OUT_OFS             /*!< CEOUT Offset */
#define CEOUT                                    COMP_E_CTL1_OUT                 /*!< Comparator output value */
/* CE0CTL1[CEOUTPOL] Bits */
#define CEOUTPOL_OFS                             COMP_E_CTL1_OUTPOL_OFS          /*!< CEOUTPOL Offset */
#define CEOUTPOL                                 COMP_E_CTL1_OUTPOL              /*!< Comparator output polarity */
/* CE0CTL1[CEF] Bits */
#define CEF_OFS                                  COMP_E_CTL1_F_OFS               /*!< CEF Offset */
#define CEF                                      COMP_E_CTL1_F                   /*!< Comparator output filter */
/* CE0CTL1[CEIES] Bits */
#define CEIES_OFS                                COMP_E_CTL1_IES_OFS             /*!< CEIES Offset */
#define CEIES                                    COMP_E_CTL1_IES                 /*!< Interrupt edge select for CEIIFG and CEIFG */
/* CE0CTL1[CESHORT] Bits */
#define CESHORT_OFS                              COMP_E_CTL1_SHORT_OFS           /*!< CESHORT Offset */
#define CESHORT                                  COMP_E_CTL1_SHORT               /*!< Input short */
/* CE0CTL1[CEEX] Bits */
#define CEEX_OFS                                 COMP_E_CTL1_EX_OFS              /*!< CEEX Offset */
#define CEEX                                     COMP_E_CTL1_EX                  /*!< Exchange */
/* CE0CTL1[CEFDLY] Bits */
#define CEFDLY_OFS                               COMP_E_CTL1_FDLY_OFS            /*!< CEFDLY Offset */
#define CEFDLY_M                                 COMP_E_CTL1_FDLY_MASK           /*!< Filter delay */
#define CEFDLY0                                  COMP_E_CTL1_FDLY0               /*!< CEFDLY Bit 0 */
#define CEFDLY1                                  COMP_E_CTL1_FDLY1               /*!< CEFDLY Bit 1 */
#define CEFDLY_0                                 COMP_E_CTL1_FDLY_0              /*!< Typical filter delay of TBD (450) ns */
#define CEFDLY_1                                 COMP_E_CTL1_FDLY_1              /*!< Typical filter delay of TBD (900) ns */
#define CEFDLY_2                                 COMP_E_CTL1_FDLY_2              /*!< Typical filter delay of TBD (1800) ns */
#define CEFDLY_3                                 COMP_E_CTL1_FDLY_3              /*!< Typical filter delay of TBD (3600) ns */
/* CE0CTL1[CEPWRMD] Bits */
#define CEPWRMD_OFS                              COMP_E_CTL1_PWRMD_OFS           /*!< CEPWRMD Offset */
#define CEPWRMD_M                                COMP_E_CTL1_PWRMD_MASK          /*!< Power Mode */
#define CEPWRMD0                                 COMP_E_CTL1_PWRMD0              /*!< CEPWRMD Bit 0 */
#define CEPWRMD1                                 COMP_E_CTL1_PWRMD1              /*!< CEPWRMD Bit 1 */
#define CEPWRMD_0                                COMP_E_CTL1_PWRMD_0             /*!< High-speed mode */
#define CEPWRMD_1                                COMP_E_CTL1_PWRMD_1             /*!< Normal mode */
#define CEPWRMD_2                                COMP_E_CTL1_PWRMD_2             /*!< Ultra-low power mode */
/* CE0CTL1[CEON] Bits */
#define CEON_OFS                                 COMP_E_CTL1_ON_OFS              /*!< CEON Offset */
#define CEON                                     COMP_E_CTL1_ON                  /*!< Comparator On */
/* CE0CTL1[CEMRVL] Bits */
#define CEMRVL_OFS                               COMP_E_CTL1_MRVL_OFS            /*!< CEMRVL Offset */
#define CEMRVL                                   COMP_E_CTL1_MRVL                /*!< This bit is valid of CEMRVS is set to 1 */
/* CE0CTL1[CEMRVS] Bits */
#define CEMRVS_OFS                               COMP_E_CTL1_MRVS_OFS            /*!< CEMRVS Offset */
#define CEMRVS                                   COMP_E_CTL1_MRVS                
/* CE0CTL2[CEREF0] Bits */
#define CEREF0_OFS                               COMP_E_CTL2_REF0_OFS            /*!< CEREF0 Offset */
#define CEREF0_M                                 COMP_E_CTL2_REF0_MASK           /*!< Reference resistor tap 0 */
#define CEREF00                                  COMP_E_CTL2_REF00               /*!< CEREF0 Bit 0 */
#define CEREF01                                  COMP_E_CTL2_REF01               /*!< CEREF0 Bit 1 */
#define CEREF02                                  COMP_E_CTL2_REF02               /*!< CEREF0 Bit 2 */
#define CEREF03                                  COMP_E_CTL2_REF03               /*!< CEREF0 Bit 3 */
#define CEREF04                                  COMP_E_CTL2_REF04               /*!< CEREF0 Bit 4 */
#define CEREF0_0                                 COMP_E_CTL2_REF0_0              /*!< Reference resistor tap for setting 0. */
#define CEREF0_1                                 COMP_E_CTL2_REF0_1              /*!< Reference resistor tap for setting 1. */
#define CEREF0_2                                 COMP_E_CTL2_REF0_2              /*!< Reference resistor tap for setting 2. */
#define CEREF0_3                                 COMP_E_CTL2_REF0_3              /*!< Reference resistor tap for setting 3. */
#define CEREF0_4                                 COMP_E_CTL2_REF0_4              /*!< Reference resistor tap for setting 4. */
#define CEREF0_5                                 COMP_E_CTL2_REF0_5              /*!< Reference resistor tap for setting 5. */
#define CEREF0_6                                 COMP_E_CTL2_REF0_6              /*!< Reference resistor tap for setting 6. */
#define CEREF0_7                                 COMP_E_CTL2_REF0_7              /*!< Reference resistor tap for setting 7. */
#define CEREF0_8                                 COMP_E_CTL2_REF0_8              /*!< Reference resistor tap for setting 8. */
#define CEREF0_9                                 COMP_E_CTL2_REF0_9              /*!< Reference resistor tap for setting 9. */
#define CEREF0_10                                COMP_E_CTL2_REF0_10             /*!< Reference resistor tap for setting 10. */
#define CEREF0_11                                COMP_E_CTL2_REF0_11             /*!< Reference resistor tap for setting 11. */
#define CEREF0_12                                COMP_E_CTL2_REF0_12             /*!< Reference resistor tap for setting 12. */
#define CEREF0_13                                COMP_E_CTL2_REF0_13             /*!< Reference resistor tap for setting 13. */
#define CEREF0_14                                COMP_E_CTL2_REF0_14             /*!< Reference resistor tap for setting 14. */
#define CEREF0_15                                COMP_E_CTL2_REF0_15             /*!< Reference resistor tap for setting 15. */
#define CEREF0_16                                COMP_E_CTL2_REF0_16             /*!< Reference resistor tap for setting 16. */
#define CEREF0_17                                COMP_E_CTL2_REF0_17             /*!< Reference resistor tap for setting 17. */
#define CEREF0_18                                COMP_E_CTL2_REF0_18             /*!< Reference resistor tap for setting 18. */
#define CEREF0_19                                COMP_E_CTL2_REF0_19             /*!< Reference resistor tap for setting 19. */
#define CEREF0_20                                COMP_E_CTL2_REF0_20             /*!< Reference resistor tap for setting 20. */
#define CEREF0_21                                COMP_E_CTL2_REF0_21             /*!< Reference resistor tap for setting 21. */
#define CEREF0_22                                COMP_E_CTL2_REF0_22             /*!< Reference resistor tap for setting 22. */
#define CEREF0_23                                COMP_E_CTL2_REF0_23             /*!< Reference resistor tap for setting 23. */
#define CEREF0_24                                COMP_E_CTL2_REF0_24             /*!< Reference resistor tap for setting 24. */
#define CEREF0_25                                COMP_E_CTL2_REF0_25             /*!< Reference resistor tap for setting 25. */
#define CEREF0_26                                COMP_E_CTL2_REF0_26             /*!< Reference resistor tap for setting 26. */
#define CEREF0_27                                COMP_E_CTL2_REF0_27             /*!< Reference resistor tap for setting 27. */
#define CEREF0_28                                COMP_E_CTL2_REF0_28             /*!< Reference resistor tap for setting 28. */
#define CEREF0_29                                COMP_E_CTL2_REF0_29             /*!< Reference resistor tap for setting 29. */
#define CEREF0_30                                COMP_E_CTL2_REF0_30             /*!< Reference resistor tap for setting 30. */
#define CEREF0_31                                COMP_E_CTL2_REF0_31             /*!< Reference resistor tap for setting 31. */
/* CE0CTL2[CERSEL] Bits */
#define CERSEL_OFS                               COMP_E_CTL2_RSEL_OFS            /*!< CERSEL Offset */
#define CERSEL                                   COMP_E_CTL2_RSEL                /*!< Reference select */
/* CE0CTL2[CERS] Bits */
#define CERS_OFS                                 COMP_E_CTL2_RS_OFS              /*!< CERS Offset */
#define CERS_M                                   COMP_E_CTL2_RS_MASK             /*!< Reference source */
#define CERS0                                    COMP_E_CTL2_RS0                 /*!< CERS Bit 0 */
#define CERS1                                    COMP_E_CTL2_RS1                 /*!< CERS Bit 1 */
#define CERS_0                                   COMP_E_CTL2_RS_0                /*!< No current is drawn by the reference circuitry */
#define CERS_1                                   COMP_E_CTL2_RS_1                /*!< VCC applied to the resistor ladder */
#define CERS_2                                   COMP_E_CTL2_RS_2                /*!< Shared reference voltage applied to the resistor ladder */
#define CERS_3                                   COMP_E_CTL2_RS_3                /*!< Shared reference voltage supplied to V(CREF). Resistor ladder is off */
/* CE0CTL2[CEREF1] Bits */
#define CEREF1_OFS                               COMP_E_CTL2_REF1_OFS            /*!< CEREF1 Offset */
#define CEREF1_M                                 COMP_E_CTL2_REF1_MASK           /*!< Reference resistor tap 1 */
#define CEREF10                                  COMP_E_CTL2_REF10               /*!< CEREF1 Bit 0 */
#define CEREF11                                  COMP_E_CTL2_REF11               /*!< CEREF1 Bit 1 */
#define CEREF12                                  COMP_E_CTL2_REF12               /*!< CEREF1 Bit 2 */
#define CEREF13                                  COMP_E_CTL2_REF13               /*!< CEREF1 Bit 3 */
#define CEREF14                                  COMP_E_CTL2_REF14               /*!< CEREF1 Bit 4 */
#define CEREF1_0                                 COMP_E_CTL2_REF1_0              /*!< Reference resistor tap for setting 0. */
#define CEREF1_1                                 COMP_E_CTL2_REF1_1              /*!< Reference resistor tap for setting 1. */
#define CEREF1_2                                 COMP_E_CTL2_REF1_2              /*!< Reference resistor tap for setting 2. */
#define CEREF1_3                                 COMP_E_CTL2_REF1_3              /*!< Reference resistor tap for setting 3. */
#define CEREF1_4                                 COMP_E_CTL2_REF1_4              /*!< Reference resistor tap for setting 4. */
#define CEREF1_5                                 COMP_E_CTL2_REF1_5              /*!< Reference resistor tap for setting 5. */
#define CEREF1_6                                 COMP_E_CTL2_REF1_6              /*!< Reference resistor tap for setting 6. */
#define CEREF1_7                                 COMP_E_CTL2_REF1_7              /*!< Reference resistor tap for setting 7. */
#define CEREF1_8                                 COMP_E_CTL2_REF1_8              /*!< Reference resistor tap for setting 8. */
#define CEREF1_9                                 COMP_E_CTL2_REF1_9              /*!< Reference resistor tap for setting 9. */
#define CEREF1_10                                COMP_E_CTL2_REF1_10             /*!< Reference resistor tap for setting 10. */
#define CEREF1_11                                COMP_E_CTL2_REF1_11             /*!< Reference resistor tap for setting 11. */
#define CEREF1_12                                COMP_E_CTL2_REF1_12             /*!< Reference resistor tap for setting 12. */
#define CEREF1_13                                COMP_E_CTL2_REF1_13             /*!< Reference resistor tap for setting 13. */
#define CEREF1_14                                COMP_E_CTL2_REF1_14             /*!< Reference resistor tap for setting 14. */
#define CEREF1_15                                COMP_E_CTL2_REF1_15             /*!< Reference resistor tap for setting 15. */
#define CEREF1_16                                COMP_E_CTL2_REF1_16             /*!< Reference resistor tap for setting 16. */
#define CEREF1_17                                COMP_E_CTL2_REF1_17             /*!< Reference resistor tap for setting 17. */
#define CEREF1_18                                COMP_E_CTL2_REF1_18             /*!< Reference resistor tap for setting 18. */
#define CEREF1_19                                COMP_E_CTL2_REF1_19             /*!< Reference resistor tap for setting 19. */
#define CEREF1_20                                COMP_E_CTL2_REF1_20             /*!< Reference resistor tap for setting 20. */
#define CEREF1_21                                COMP_E_CTL2_REF1_21             /*!< Reference resistor tap for setting 21. */
#define CEREF1_22                                COMP_E_CTL2_REF1_22             /*!< Reference resistor tap for setting 22. */
#define CEREF1_23                                COMP_E_CTL2_REF1_23             /*!< Reference resistor tap for setting 23. */
#define CEREF1_24                                COMP_E_CTL2_REF1_24             /*!< Reference resistor tap for setting 24. */
#define CEREF1_25                                COMP_E_CTL2_REF1_25             /*!< Reference resistor tap for setting 25. */
#define CEREF1_26                                COMP_E_CTL2_REF1_26             /*!< Reference resistor tap for setting 26. */
#define CEREF1_27                                COMP_E_CTL2_REF1_27             /*!< Reference resistor tap for setting 27. */
#define CEREF1_28                                COMP_E_CTL2_REF1_28             /*!< Reference resistor tap for setting 28. */
#define CEREF1_29                                COMP_E_CTL2_REF1_29             /*!< Reference resistor tap for setting 29. */
#define CEREF1_30                                COMP_E_CTL2_REF1_30             /*!< Reference resistor tap for setting 30. */
#define CEREF1_31                                COMP_E_CTL2_REF1_31             /*!< Reference resistor tap for setting 31. */
/* CE0CTL2[CEREFL] Bits */
#define CEREFL_OFS                               COMP_E_CTL2_REFL_OFS            /*!< CEREFL Offset */
#define CEREFL_M                                 COMP_E_CTL2_REFL_MASK           /*!< Reference voltage level */
#define CEREFL0                                  COMP_E_CTL2_REFL0               /*!< CEREFL Bit 0 */
#define CEREFL1                                  COMP_E_CTL2_REFL1               /*!< CEREFL Bit 1 */
#define CEREFL_0                                 COMP_E_CTL2_CEREFL_0            /*!< Reference amplifier is disabled. No reference voltage is requested */
#define CEREFL_1                                 COMP_E_CTL2_CEREFL_1            /*!< 1.2 V is selected as shared reference voltage input */
#define CEREFL_2                                 COMP_E_CTL2_CEREFL_2            /*!< 2.0 V is selected as shared reference voltage input */
#define CEREFL_3                                 COMP_E_CTL2_CEREFL_3            /*!< 2.5 V is selected as shared reference voltage input */
#define CEREFL__OFF                              COMP_E_CTL2_REFL__OFF           /*!< Reference amplifier is disabled. No reference voltage is requested */
#define CEREFL__1P2V                             COMP_E_CTL2_REFL__1P2V          /*!< 1.2 V is selected as shared reference voltage input */
#define CEREFL__2P0V                             COMP_E_CTL2_REFL__2P0V          /*!< 2.0 V is selected as shared reference voltage input */
#define CEREFL__2P5V                             COMP_E_CTL2_REFL__2P5V          /*!< 2.5 V is selected as shared reference voltage input */
/* CE0CTL2[CEREFACC] Bits */
#define CEREFACC_OFS                             COMP_E_CTL2_REFACC_OFS          /*!< CEREFACC Offset */
#define CEREFACC                                 COMP_E_CTL2_REFACC              /*!< Reference accuracy */
/* CE0CTL3[CEPD0] Bits */
#define CEPD0_OFS                                COMP_E_CTL3_PD0_OFS             /*!< CEPD0 Offset */
#define CEPD0                                    COMP_E_CTL3_PD0                 /*!< Port disable */
/* CE0CTL3[CEPD1] Bits */
#define CEPD1_OFS                                COMP_E_CTL3_PD1_OFS             /*!< CEPD1 Offset */
#define CEPD1                                    COMP_E_CTL3_PD1                 /*!< Port disable */
/* CE0CTL3[CEPD2] Bits */
#define CEPD2_OFS                                COMP_E_CTL3_PD2_OFS             /*!< CEPD2 Offset */
#define CEPD2                                    COMP_E_CTL3_PD2                 /*!< Port disable */
/* CE0CTL3[CEPD3] Bits */
#define CEPD3_OFS                                COMP_E_CTL3_PD3_OFS             /*!< CEPD3 Offset */
#define CEPD3                                    COMP_E_CTL3_PD3                 /*!< Port disable */
/* CE0CTL3[CEPD4] Bits */
#define CEPD4_OFS                                COMP_E_CTL3_PD4_OFS             /*!< CEPD4 Offset */
#define CEPD4                                    COMP_E_CTL3_PD4                 /*!< Port disable */
/* CE0CTL3[CEPD5] Bits */
#define CEPD5_OFS                                COMP_E_CTL3_PD5_OFS             /*!< CEPD5 Offset */
#define CEPD5                                    COMP_E_CTL3_PD5                 /*!< Port disable */
/* CE0CTL3[CEPD6] Bits */
#define CEPD6_OFS                                COMP_E_CTL3_PD6_OFS             /*!< CEPD6 Offset */
#define CEPD6                                    COMP_E_CTL3_PD6                 /*!< Port disable */
/* CE0CTL3[CEPD7] Bits */
#define CEPD7_OFS                                COMP_E_CTL3_PD7_OFS             /*!< CEPD7 Offset */
#define CEPD7                                    COMP_E_CTL3_PD7                 /*!< Port disable */
/* CE0CTL3[CEPD8] Bits */
#define CEPD8_OFS                                COMP_E_CTL3_PD8_OFS             /*!< CEPD8 Offset */
#define CEPD8                                    COMP_E_CTL3_PD8                 /*!< Port disable */
/* CE0CTL3[CEPD9] Bits */
#define CEPD9_OFS                                COMP_E_CTL3_PD9_OFS             /*!< CEPD9 Offset */
#define CEPD9                                    COMP_E_CTL3_PD9                 /*!< Port disable */
/* CE0CTL3[CEPD10] Bits */
#define CEPD10_OFS                               COMP_E_CTL3_PD10_OFS            /*!< CEPD10 Offset */
#define CEPD10                                   COMP_E_CTL3_PD10                /*!< Port disable */
/* CE0CTL3[CEPD11] Bits */
#define CEPD11_OFS                               COMP_E_CTL3_PD11_OFS            /*!< CEPD11 Offset */
#define CEPD11                                   COMP_E_CTL3_PD11                /*!< Port disable */
/* CE0CTL3[CEPD12] Bits */
#define CEPD12_OFS                               COMP_E_CTL3_PD12_OFS            /*!< CEPD12 Offset */
#define CEPD12                                   COMP_E_CTL3_PD12                /*!< Port disable */
/* CE0CTL3[CEPD13] Bits */
#define CEPD13_OFS                               COMP_E_CTL3_PD13_OFS            /*!< CEPD13 Offset */
#define CEPD13                                   COMP_E_CTL3_PD13                /*!< Port disable */
/* CE0CTL3[CEPD14] Bits */
#define CEPD14_OFS                               COMP_E_CTL3_PD14_OFS            /*!< CEPD14 Offset */
#define CEPD14                                   COMP_E_CTL3_PD14                /*!< Port disable */
/* CE0CTL3[CEPD15] Bits */
#define CEPD15_OFS                               COMP_E_CTL3_PD15_OFS            /*!< CEPD15 Offset */
#define CEPD15                                   COMP_E_CTL3_PD15                /*!< Port disable */
/* CE0INT[CEIFG] Bits */
#define CEIFG_OFS                                COMP_E_INT_IFG_OFS              /*!< CEIFG Offset */
#define CEIFG                                    COMP_E_INT_IFG                  /*!< Comparator output interrupt flag */
/* CE0INT[CEIIFG] Bits */
#define CEIIFG_OFS                               COMP_E_INT_IIFG_OFS             /*!< CEIIFG Offset */
#define CEIIFG                                   COMP_E_INT_IIFG                 /*!< Comparator output inverted interrupt flag */
/* CE0INT[CERDYIFG] Bits */
#define CERDYIFG_OFS                             COMP_E_INT_RDYIFG_OFS           /*!< CERDYIFG Offset */
#define CERDYIFG                                 COMP_E_INT_RDYIFG               /*!< Comparator ready interrupt flag */
/* CE0INT[CEIE] Bits */
#define CEIE_OFS                                 COMP_E_INT_IE_OFS               /*!< CEIE Offset */
#define CEIE                                     COMP_E_INT_IE                   /*!< Comparator output interrupt enable */
/* CE0INT[CEIIE] Bits */
#define CEIIE_OFS                                COMP_E_INT_IIE_OFS              /*!< CEIIE Offset */
#define CEIIE                                    COMP_E_INT_IIE                  /*!< Comparator output interrupt enable inverted polarity */
/* CE0INT[CERDYIE] Bits */
#define CERDYIE_OFS                              COMP_E_INT_RDYIE_OFS            /*!< CERDYIE Offset */
#define CERDYIE                                  COMP_E_INT_RDYIE                /*!< Comparator ready interrupt enable */

/******************************************************************************
* CRC32 Bits (legacy section)
******************************************************************************/
/* DIO_PAIN[P1IN] Bits */
#define P1IN_OFS                                           ( 0)                  /*!< P1IN Offset */
#define P1IN_M                                             (0x00ff)              /*!< Port 1 Input */
/* DIO_PAIN[P2IN] Bits */
#define P2IN_OFS                                           ( 8)                  /*!< P2IN Offset */
#define P2IN_M                                             (0xff00)              /*!< Port 2 Input */
/* DIO_PAOUT[P2OUT] Bits */
#define P2OUT_OFS                                          ( 8)                  /*!< P2OUT Offset */
#define P2OUT_M                                            (0xff00)              /*!< Port 2 Output */
/* DIO_PAOUT[P1OUT] Bits */
#define P1OUT_OFS                                          ( 0)                  /*!< P1OUT Offset */
#define P1OUT_M                                            (0x00ff)              /*!< Port 1 Output */
/* DIO_PADIR[P1DIR] Bits */
#define P1DIR_OFS                                          ( 0)                  /*!< P1DIR Offset */
#define P1DIR_M                                            (0x00ff)              /*!< Port 1 Direction */
/* DIO_PADIR[P2DIR] Bits */
#define P2DIR_OFS                                          ( 8)                  /*!< P2DIR Offset */
#define P2DIR_M                                            (0xff00)              /*!< Port 2 Direction */
/* DIO_PAREN[P1REN] Bits */
#define P1REN_OFS                                          ( 0)                  /*!< P1REN Offset */
#define P1REN_M                                            (0x00ff)              /*!< Port 1 Resistor Enable */
/* DIO_PAREN[P2REN] Bits */
#define P2REN_OFS                                          ( 8)                  /*!< P2REN Offset */
#define P2REN_M                                            (0xff00)              /*!< Port 2 Resistor Enable */
/* DIO_PADS[P1DS] Bits */
#define P1DS_OFS                                           ( 0)                  /*!< P1DS Offset */
#define P1DS_M                                             (0x00ff)              /*!< Port 1 Drive Strength */
/* DIO_PADS[P2DS] Bits */
#define P2DS_OFS                                           ( 8)                  /*!< P2DS Offset */
#define P2DS_M                                             (0xff00)              /*!< Port 2 Drive Strength */
/* DIO_PASEL0[P1SEL0] Bits */
#define P1SEL0_OFS                                         ( 0)                  /*!< P1SEL0 Offset */
#define P1SEL0_M                                           (0x00ff)              /*!< Port 1 Select 0 */
/* DIO_PASEL0[P2SEL0] Bits */
#define P2SEL0_OFS                                         ( 8)                  /*!< P2SEL0 Offset */
#define P2SEL0_M                                           (0xff00)              /*!< Port 2 Select 0 */
/* DIO_PASEL1[P1SEL1] Bits */
#define P1SEL1_OFS                                         ( 0)                  /*!< P1SEL1 Offset */
#define P1SEL1_M                                           (0x00ff)              /*!< Port 1 Select 1 */
/* DIO_PASEL1[P2SEL1] Bits */
#define P2SEL1_OFS                                         ( 8)                  /*!< P2SEL1 Offset */
#define P2SEL1_M                                           (0xff00)              /*!< Port 2 Select 1 */
/* DIO_P1IV[P1IV] Bits */
#define P1IV_OFS                                           ( 0)                  /*!< P1IV Offset */
#define P1IV_M                                             (0x001f)              /*!< Port 1 interrupt vector value */
#define P1IV0                                              (0x0001)              /*!< Port 1 interrupt vector value */
#define P1IV1                                              (0x0002)              /*!< Port 1 interrupt vector value */
#define P1IV2                                              (0x0004)              /*!< Port 1 interrupt vector value */
#define P1IV3                                              (0x0008)              /*!< Port 1 interrupt vector value */
#define P1IV4                                              (0x0010)              /*!< Port 1 interrupt vector value */
#define P1IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P1IV_2                                             (0x0002)              /*!< Interrupt Source: Port 1.0 interrupt; Interrupt Flag: P1IFG0; Interrupt Priority: Highest */
#define P1IV_4                                             (0x0004)              /*!< Interrupt Source: Port 1.1 interrupt; Interrupt Flag: P1IFG1 */
#define P1IV_6                                             (0x0006)              /*!< Interrupt Source: Port 1.2 interrupt; Interrupt Flag: P1IFG2 */
#define P1IV_8                                             (0x0008)              /*!< Interrupt Source: Port 1.3 interrupt; Interrupt Flag: P1IFG3 */
#define P1IV_10                                            (0x000a)              /*!< Interrupt Source: Port 1.4 interrupt; Interrupt Flag: P1IFG4 */
#define P1IV_12                                            (0x000c)              /*!< Interrupt Source: Port 1.5 interrupt; Interrupt Flag: P1IFG5 */
#define P1IV_14                                            (0x000e)              /*!< Interrupt Source: Port 1.6 interrupt; Interrupt Flag: P1IFG6 */
#define P1IV_16                                            (0x0010)              /*!< Interrupt Source: Port 1.7 interrupt; Interrupt Flag: P1IFG7; Interrupt Priority: Lowest */
#define P1IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P1IV__P1IFG0                                       (0x0002)              /*!< Interrupt Source: Port 1.0 interrupt; Interrupt Flag: P1IFG0; Interrupt Priority: Highest */
#define P1IV__P1IFG1                                       (0x0004)              /*!< Interrupt Source: Port 1.1 interrupt; Interrupt Flag: P1IFG1 */
#define P1IV__P1IFG2                                       (0x0006)              /*!< Interrupt Source: Port 1.2 interrupt; Interrupt Flag: P1IFG2 */
#define P1IV__P1IFG3                                       (0x0008)              /*!< Interrupt Source: Port 1.3 interrupt; Interrupt Flag: P1IFG3 */
#define P1IV__P1IFG4                                       (0x000a)              /*!< Interrupt Source: Port 1.4 interrupt; Interrupt Flag: P1IFG4 */
#define P1IV__P1IFG5                                       (0x000c)              /*!< Interrupt Source: Port 1.5 interrupt; Interrupt Flag: P1IFG5 */
#define P1IV__P1IFG6                                       (0x000e)              /*!< Interrupt Source: Port 1.6 interrupt; Interrupt Flag: P1IFG6 */
#define P1IV__P1IFG7                                       (0x0010)              /*!< Interrupt Source: Port 1.7 interrupt; Interrupt Flag: P1IFG7; Interrupt Priority: Lowest */
/* DIO_PASELC[P1SELC] Bits */
#define P1SELC_OFS                                         ( 0)                  /*!< P1SELC Offset */
#define P1SELC_M                                           (0x00ff)              /*!< Port 1 Complement Select */
/* DIO_PASELC[P2SELC] Bits */
#define P2SELC_OFS                                         ( 8)                  /*!< P2SELC Offset */
#define P2SELC_M                                           (0xff00)              /*!< Port 2 Complement Select */
/* DIO_PAIES[P1IES] Bits */
#define P1IES_OFS                                          ( 0)                  /*!< P1IES Offset */
#define P1IES_M                                            (0x00ff)              /*!< Port 1 Interrupt Edge Select */
/* DIO_PAIES[P2IES] Bits */
#define P2IES_OFS                                          ( 8)                  /*!< P2IES Offset */
#define P2IES_M                                            (0xff00)              /*!< Port 2 Interrupt Edge Select */
/* DIO_PAIE[P1IE] Bits */
#define P1IE_OFS                                           ( 0)                  /*!< P1IE Offset */
#define P1IE_M                                             (0x00ff)              /*!< Port 1 Interrupt Enable */
/* DIO_PAIE[P2IE] Bits */
#define P2IE_OFS                                           ( 8)                  /*!< P2IE Offset */
#define P2IE_M                                             (0xff00)              /*!< Port 2 Interrupt Enable */
/* DIO_PAIFG[P1IFG] Bits */
#define P1IFG_OFS                                          ( 0)                  /*!< P1IFG Offset */
#define P1IFG_M                                            (0x00ff)              /*!< Port 1 Interrupt Flag */
/* DIO_PAIFG[P2IFG] Bits */
#define P2IFG_OFS                                          ( 8)                  /*!< P2IFG Offset */
#define P2IFG_M                                            (0xff00)              /*!< Port 2 Interrupt Flag */
/* DIO_P2IV[P2IV] Bits */
#define P2IV_OFS                                           ( 0)                  /*!< P2IV Offset */
#define P2IV_M                                             (0x001f)              /*!< Port 2 interrupt vector value */
#define P2IV0                                              (0x0001)              /*!< Port 2 interrupt vector value */
#define P2IV1                                              (0x0002)              /*!< Port 2 interrupt vector value */
#define P2IV2                                              (0x0004)              /*!< Port 2 interrupt vector value */
#define P2IV3                                              (0x0008)              /*!< Port 2 interrupt vector value */
#define P2IV4                                              (0x0010)              /*!< Port 2 interrupt vector value */
#define P2IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P2IV_2                                             (0x0002)              /*!< Interrupt Source: Port 2.0 interrupt; Interrupt Flag: P2IFG0; Interrupt Priority: Highest */
#define P2IV_4                                             (0x0004)              /*!< Interrupt Source: Port 2.1 interrupt; Interrupt Flag: P2IFG1 */
#define P2IV_6                                             (0x0006)              /*!< Interrupt Source: Port 2.2 interrupt; Interrupt Flag: P2IFG2 */
#define P2IV_8                                             (0x0008)              /*!< Interrupt Source: Port 2.3 interrupt; Interrupt Flag: P2IFG3 */
#define P2IV_10                                            (0x000a)              /*!< Interrupt Source: Port 2.4 interrupt; Interrupt Flag: P2IFG4 */
#define P2IV_12                                            (0x000c)              /*!< Interrupt Source: Port 2.5 interrupt; Interrupt Flag: P2IFG5 */
#define P2IV_14                                            (0x000e)              /*!< Interrupt Source: Port 2.6 interrupt; Interrupt Flag: P2IFG6 */
#define P2IV_16                                            (0x0010)              /*!< Interrupt Source: Port 2.7 interrupt; Interrupt Flag: P2IFG7; Interrupt Priority: Lowest */
#define P2IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P2IV__P2IFG0                                       (0x0002)              /*!< Interrupt Source: Port 2.0 interrupt; Interrupt Flag: P2IFG0; Interrupt Priority: Highest */
#define P2IV__P2IFG1                                       (0x0004)              /*!< Interrupt Source: Port 2.1 interrupt; Interrupt Flag: P2IFG1 */
#define P2IV__P2IFG2                                       (0x0006)              /*!< Interrupt Source: Port 2.2 interrupt; Interrupt Flag: P2IFG2 */
#define P2IV__P2IFG3                                       (0x0008)              /*!< Interrupt Source: Port 2.3 interrupt; Interrupt Flag: P2IFG3 */
#define P2IV__P2IFG4                                       (0x000a)              /*!< Interrupt Source: Port 2.4 interrupt; Interrupt Flag: P2IFG4 */
#define P2IV__P2IFG5                                       (0x000c)              /*!< Interrupt Source: Port 2.5 interrupt; Interrupt Flag: P2IFG5 */
#define P2IV__P2IFG6                                       (0x000e)              /*!< Interrupt Source: Port 2.6 interrupt; Interrupt Flag: P2IFG6 */
#define P2IV__P2IFG7                                       (0x0010)              /*!< Interrupt Source: Port 2.7 interrupt; Interrupt Flag: P2IFG7; Interrupt Priority: Lowest */
/* DIO_PBIN[P3IN] Bits */
#define P3IN_OFS                                           ( 0)                  /*!< P3IN Offset */
#define P3IN_M                                             (0x00ff)              /*!< Port 3 Input */
/* DIO_PBIN[P4IN] Bits */
#define P4IN_OFS                                           ( 8)                  /*!< P4IN Offset */
#define P4IN_M                                             (0xff00)              /*!< Port 4 Input */
/* DIO_PBOUT[P3OUT] Bits */
#define P3OUT_OFS                                          ( 0)                  /*!< P3OUT Offset */
#define P3OUT_M                                            (0x00ff)              /*!< Port 3 Output */
/* DIO_PBOUT[P4OUT] Bits */
#define P4OUT_OFS                                          ( 8)                  /*!< P4OUT Offset */
#define P4OUT_M                                            (0xff00)              /*!< Port 4 Output */
/* DIO_PBDIR[P3DIR] Bits */
#define P3DIR_OFS                                          ( 0)                  /*!< P3DIR Offset */
#define P3DIR_M                                            (0x00ff)              /*!< Port 3 Direction */
/* DIO_PBDIR[P4DIR] Bits */
#define P4DIR_OFS                                          ( 8)                  /*!< P4DIR Offset */
#define P4DIR_M                                            (0xff00)              /*!< Port 4 Direction */
/* DIO_PBREN[P3REN] Bits */
#define P3REN_OFS                                          ( 0)                  /*!< P3REN Offset */
#define P3REN_M                                            (0x00ff)              /*!< Port 3 Resistor Enable */
/* DIO_PBREN[P4REN] Bits */
#define P4REN_OFS                                          ( 8)                  /*!< P4REN Offset */
#define P4REN_M                                            (0xff00)              /*!< Port 4 Resistor Enable */
/* DIO_PBDS[P3DS] Bits */
#define P3DS_OFS                                           ( 0)                  /*!< P3DS Offset */
#define P3DS_M                                             (0x00ff)              /*!< Port 3 Drive Strength */
/* DIO_PBDS[P4DS] Bits */
#define P4DS_OFS                                           ( 8)                  /*!< P4DS Offset */
#define P4DS_M                                             (0xff00)              /*!< Port 4 Drive Strength */
/* DIO_PBSEL0[P4SEL0] Bits */
#define P4SEL0_OFS                                         ( 8)                  /*!< P4SEL0 Offset */
#define P4SEL0_M                                           (0xff00)              /*!< Port 4 Select 0 */
/* DIO_PBSEL0[P3SEL0] Bits */
#define P3SEL0_OFS                                         ( 0)                  /*!< P3SEL0 Offset */
#define P3SEL0_M                                           (0x00ff)              /*!< Port 3 Select 0 */
/* DIO_PBSEL1[P3SEL1] Bits */
#define P3SEL1_OFS                                         ( 0)                  /*!< P3SEL1 Offset */
#define P3SEL1_M                                           (0x00ff)              /*!< Port 3 Select 1 */
/* DIO_PBSEL1[P4SEL1] Bits */
#define P4SEL1_OFS                                         ( 8)                  /*!< P4SEL1 Offset */
#define P4SEL1_M                                           (0xff00)              /*!< Port 4 Select 1 */
/* DIO_P3IV[P3IV] Bits */
#define P3IV_OFS                                           ( 0)                  /*!< P3IV Offset */
#define P3IV_M                                             (0x001f)              /*!< Port 3 interrupt vector value */
#define P3IV0                                              (0x0001)              /*!< Port 3 interrupt vector value */
#define P3IV1                                              (0x0002)              /*!< Port 3 interrupt vector value */
#define P3IV2                                              (0x0004)              /*!< Port 3 interrupt vector value */
#define P3IV3                                              (0x0008)              /*!< Port 3 interrupt vector value */
#define P3IV4                                              (0x0010)              /*!< Port 3 interrupt vector value */
#define P3IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P3IV_2                                             (0x0002)              /*!< Interrupt Source: Port 3.0 interrupt; Interrupt Flag: P3IFG0; Interrupt Priority: Highest */
#define P3IV_4                                             (0x0004)              /*!< Interrupt Source: Port 3.1 interrupt; Interrupt Flag: P3IFG1 */
#define P3IV_6                                             (0x0006)              /*!< Interrupt Source: Port 3.2 interrupt; Interrupt Flag: P3IFG2 */
#define P3IV_8                                             (0x0008)              /*!< Interrupt Source: Port 3.3 interrupt; Interrupt Flag: P3IFG3 */
#define P3IV_10                                            (0x000a)              /*!< Interrupt Source: Port 3.4 interrupt; Interrupt Flag: P3IFG4 */
#define P3IV_12                                            (0x000c)              /*!< Interrupt Source: Port 3.5 interrupt; Interrupt Flag: P3IFG5 */
#define P3IV_14                                            (0x000e)              /*!< Interrupt Source: Port 3.6 interrupt; Interrupt Flag: P3IFG6 */
#define P3IV_16                                            (0x0010)              /*!< Interrupt Source: Port 3.7 interrupt; Interrupt Flag: P3IFG7; Interrupt Priority: Lowest */
#define P3IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P3IV__P3IFG0                                       (0x0002)              /*!< Interrupt Source: Port 3.0 interrupt; Interrupt Flag: P3IFG0; Interrupt Priority: Highest */
#define P3IV__P3IFG1                                       (0x0004)              /*!< Interrupt Source: Port 3.1 interrupt; Interrupt Flag: P3IFG1 */
#define P3IV__P3IFG2                                       (0x0006)              /*!< Interrupt Source: Port 3.2 interrupt; Interrupt Flag: P3IFG2 */
#define P3IV__P3IFG3                                       (0x0008)              /*!< Interrupt Source: Port 3.3 interrupt; Interrupt Flag: P3IFG3 */
#define P3IV__P3IFG4                                       (0x000a)              /*!< Interrupt Source: Port 3.4 interrupt; Interrupt Flag: P3IFG4 */
#define P3IV__P3IFG5                                       (0x000c)              /*!< Interrupt Source: Port 3.5 interrupt; Interrupt Flag: P3IFG5 */
#define P3IV__P3IFG6                                       (0x000e)              /*!< Interrupt Source: Port 3.6 interrupt; Interrupt Flag: P3IFG6 */
#define P3IV__P3IFG7                                       (0x0010)              /*!< Interrupt Source: Port 3.7 interrupt; Interrupt Flag: P3IFG7; Interrupt Priority: Lowest */
/* DIO_PBSELC[P3SELC] Bits */
#define P3SELC_OFS                                         ( 0)                  /*!< P3SELC Offset */
#define P3SELC_M                                           (0x00ff)              /*!< Port 3 Complement Select */
/* DIO_PBSELC[P4SELC] Bits */
#define P4SELC_OFS                                         ( 8)                  /*!< P4SELC Offset */
#define P4SELC_M                                           (0xff00)              /*!< Port 4 Complement Select */
/* DIO_PBIES[P3IES] Bits */
#define P3IES_OFS                                          ( 0)                  /*!< P3IES Offset */
#define P3IES_M                                            (0x00ff)              /*!< Port 3 Interrupt Edge Select */
/* DIO_PBIES[P4IES] Bits */
#define P4IES_OFS                                          ( 8)                  /*!< P4IES Offset */
#define P4IES_M                                            (0xff00)              /*!< Port 4 Interrupt Edge Select */
/* DIO_PBIE[P3IE] Bits */
#define P3IE_OFS                                           ( 0)                  /*!< P3IE Offset */
#define P3IE_M                                             (0x00ff)              /*!< Port 3 Interrupt Enable */
/* DIO_PBIE[P4IE] Bits */
#define P4IE_OFS                                           ( 8)                  /*!< P4IE Offset */
#define P4IE_M                                             (0xff00)              /*!< Port 4 Interrupt Enable */
/* DIO_PBIFG[P3IFG] Bits */
#define P3IFG_OFS                                          ( 0)                  /*!< P3IFG Offset */
#define P3IFG_M                                            (0x00ff)              /*!< Port 3 Interrupt Flag */
/* DIO_PBIFG[P4IFG] Bits */
#define P4IFG_OFS                                          ( 8)                  /*!< P4IFG Offset */
#define P4IFG_M                                            (0xff00)              /*!< Port 4 Interrupt Flag */
/* DIO_P4IV[P4IV] Bits */
#define P4IV_OFS                                           ( 0)                  /*!< P4IV Offset */
#define P4IV_M                                             (0x001f)              /*!< Port 4 interrupt vector value */
#define P4IV0                                              (0x0001)              /*!< Port 4 interrupt vector value */
#define P4IV1                                              (0x0002)              /*!< Port 4 interrupt vector value */
#define P4IV2                                              (0x0004)              /*!< Port 4 interrupt vector value */
#define P4IV3                                              (0x0008)              /*!< Port 4 interrupt vector value */
#define P4IV4                                              (0x0010)              /*!< Port 4 interrupt vector value */
#define P4IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P4IV_2                                             (0x0002)              /*!< Interrupt Source: Port 4.0 interrupt; Interrupt Flag: P4IFG0; Interrupt Priority: Highest */
#define P4IV_4                                             (0x0004)              /*!< Interrupt Source: Port 4.1 interrupt; Interrupt Flag: P4IFG1 */
#define P4IV_6                                             (0x0006)              /*!< Interrupt Source: Port 4.2 interrupt; Interrupt Flag: P4IFG2 */
#define P4IV_8                                             (0x0008)              /*!< Interrupt Source: Port 4.3 interrupt; Interrupt Flag: P4IFG3 */
#define P4IV_10                                            (0x000a)              /*!< Interrupt Source: Port 4.4 interrupt; Interrupt Flag: P4IFG4 */
#define P4IV_12                                            (0x000c)              /*!< Interrupt Source: Port 4.5 interrupt; Interrupt Flag: P4IFG5 */
#define P4IV_14                                            (0x000e)              /*!< Interrupt Source: Port 4.6 interrupt; Interrupt Flag: P4IFG6 */
#define P4IV_16                                            (0x0010)              /*!< Interrupt Source: Port 4.7 interrupt; Interrupt Flag: P4IFG7; Interrupt Priority: Lowest */
#define P4IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P4IV__P4IFG0                                       (0x0002)              /*!< Interrupt Source: Port 4.0 interrupt; Interrupt Flag: P4IFG0; Interrupt Priority: Highest */
#define P4IV__P4IFG1                                       (0x0004)              /*!< Interrupt Source: Port 4.1 interrupt; Interrupt Flag: P4IFG1 */
#define P4IV__P4IFG2                                       (0x0006)              /*!< Interrupt Source: Port 4.2 interrupt; Interrupt Flag: P4IFG2 */
#define P4IV__P4IFG3                                       (0x0008)              /*!< Interrupt Source: Port 4.3 interrupt; Interrupt Flag: P4IFG3 */
#define P4IV__P4IFG4                                       (0x000a)              /*!< Interrupt Source: Port 4.4 interrupt; Interrupt Flag: P4IFG4 */
#define P4IV__P4IFG5                                       (0x000c)              /*!< Interrupt Source: Port 4.5 interrupt; Interrupt Flag: P4IFG5 */
#define P4IV__P4IFG6                                       (0x000e)              /*!< Interrupt Source: Port 4.6 interrupt; Interrupt Flag: P4IFG6 */
#define P4IV__P4IFG7                                       (0x0010)              /*!< Interrupt Source: Port 4.7 interrupt; Interrupt Flag: P4IFG7; Interrupt Priority: Lowest */
/* DIO_PCIN[P5IN] Bits */
#define P5IN_OFS                                           ( 0)                  /*!< P5IN Offset */
#define P5IN_M                                             (0x00ff)              /*!< Port 5 Input */
/* DIO_PCIN[P6IN] Bits */
#define P6IN_OFS                                           ( 8)                  /*!< P6IN Offset */
#define P6IN_M                                             (0xff00)              /*!< Port 6 Input */
/* DIO_PCOUT[P5OUT] Bits */
#define P5OUT_OFS                                          ( 0)                  /*!< P5OUT Offset */
#define P5OUT_M                                            (0x00ff)              /*!< Port 5 Output */
/* DIO_PCOUT[P6OUT] Bits */
#define P6OUT_OFS                                          ( 8)                  /*!< P6OUT Offset */
#define P6OUT_M                                            (0xff00)              /*!< Port 6 Output */
/* DIO_PCDIR[P5DIR] Bits */
#define P5DIR_OFS                                          ( 0)                  /*!< P5DIR Offset */
#define P5DIR_M                                            (0x00ff)              /*!< Port 5 Direction */
/* DIO_PCDIR[P6DIR] Bits */
#define P6DIR_OFS                                          ( 8)                  /*!< P6DIR Offset */
#define P6DIR_M                                            (0xff00)              /*!< Port 6 Direction */
/* DIO_PCREN[P5REN] Bits */
#define P5REN_OFS                                          ( 0)                  /*!< P5REN Offset */
#define P5REN_M                                            (0x00ff)              /*!< Port 5 Resistor Enable */
/* DIO_PCREN[P6REN] Bits */
#define P6REN_OFS                                          ( 8)                  /*!< P6REN Offset */
#define P6REN_M                                            (0xff00)              /*!< Port 6 Resistor Enable */
/* DIO_PCDS[P5DS] Bits */
#define P5DS_OFS                                           ( 0)                  /*!< P5DS Offset */
#define P5DS_M                                             (0x00ff)              /*!< Port 5 Drive Strength */
/* DIO_PCDS[P6DS] Bits */
#define P6DS_OFS                                           ( 8)                  /*!< P6DS Offset */
#define P6DS_M                                             (0xff00)              /*!< Port 6 Drive Strength */
/* DIO_PCSEL0[P5SEL0] Bits */
#define P5SEL0_OFS                                         ( 0)                  /*!< P5SEL0 Offset */
#define P5SEL0_M                                           (0x00ff)              /*!< Port 5 Select 0 */
/* DIO_PCSEL0[P6SEL0] Bits */
#define P6SEL0_OFS                                         ( 8)                  /*!< P6SEL0 Offset */
#define P6SEL0_M                                           (0xff00)              /*!< Port 6 Select 0 */
/* DIO_PCSEL1[P5SEL1] Bits */
#define P5SEL1_OFS                                         ( 0)                  /*!< P5SEL1 Offset */
#define P5SEL1_M                                           (0x00ff)              /*!< Port 5 Select 1 */
/* DIO_PCSEL1[P6SEL1] Bits */
#define P6SEL1_OFS                                         ( 8)                  /*!< P6SEL1 Offset */
#define P6SEL1_M                                           (0xff00)              /*!< Port 6 Select 1 */
/* DIO_P5IV[P5IV] Bits */
#define P5IV_OFS                                           ( 0)                  /*!< P5IV Offset */
#define P5IV_M                                             (0x001f)              /*!< Port 5 interrupt vector value */
#define P5IV0                                              (0x0001)              /*!< Port 5 interrupt vector value */
#define P5IV1                                              (0x0002)              /*!< Port 5 interrupt vector value */
#define P5IV2                                              (0x0004)              /*!< Port 5 interrupt vector value */
#define P5IV3                                              (0x0008)              /*!< Port 5 interrupt vector value */
#define P5IV4                                              (0x0010)              /*!< Port 5 interrupt vector value */
#define P5IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P5IV_2                                             (0x0002)              /*!< Interrupt Source: Port 5.0 interrupt; Interrupt Flag: P5IFG0; Interrupt Priority: Highest */
#define P5IV_4                                             (0x0004)              /*!< Interrupt Source: Port 5.1 interrupt; Interrupt Flag: P5IFG1 */
#define P5IV_6                                             (0x0006)              /*!< Interrupt Source: Port 5.2 interrupt; Interrupt Flag: P5IFG2 */
#define P5IV_8                                             (0x0008)              /*!< Interrupt Source: Port 5.3 interrupt; Interrupt Flag: P5IFG3 */
#define P5IV_10                                            (0x000a)              /*!< Interrupt Source: Port 5.4 interrupt; Interrupt Flag: P5IFG4 */
#define P5IV_12                                            (0x000c)              /*!< Interrupt Source: Port 5.5 interrupt; Interrupt Flag: P5IFG5 */
#define P5IV_14                                            (0x000e)              /*!< Interrupt Source: Port 5.6 interrupt; Interrupt Flag: P5IFG6 */
#define P5IV_16                                            (0x0010)              /*!< Interrupt Source: Port 5.7 interrupt; Interrupt Flag: P5IFG7; Interrupt Priority: Lowest */
#define P5IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P5IV__P5IFG0                                       (0x0002)              /*!< Interrupt Source: Port 5.0 interrupt; Interrupt Flag: P5IFG0; Interrupt Priority: Highest */
#define P5IV__P5IFG1                                       (0x0004)              /*!< Interrupt Source: Port 5.1 interrupt; Interrupt Flag: P5IFG1 */
#define P5IV__P5IFG2                                       (0x0006)              /*!< Interrupt Source: Port 5.2 interrupt; Interrupt Flag: P5IFG2 */
#define P5IV__P5IFG3                                       (0x0008)              /*!< Interrupt Source: Port 5.3 interrupt; Interrupt Flag: P5IFG3 */
#define P5IV__P5IFG4                                       (0x000a)              /*!< Interrupt Source: Port 5.4 interrupt; Interrupt Flag: P5IFG4 */
#define P5IV__P5IFG5                                       (0x000c)              /*!< Interrupt Source: Port 5.5 interrupt; Interrupt Flag: P5IFG5 */
#define P5IV__P5IFG6                                       (0x000e)              /*!< Interrupt Source: Port 5.6 interrupt; Interrupt Flag: P5IFG6 */
#define P5IV__P5IFG7                                       (0x0010)              /*!< Interrupt Source: Port 5.7 interrupt; Interrupt Flag: P5IFG7; Interrupt Priority: Lowest */
/* DIO_PCSELC[P5SELC] Bits */
#define P5SELC_OFS                                         ( 0)                  /*!< P5SELC Offset */
#define P5SELC_M                                           (0x00ff)              /*!< Port 5 Complement Select */
/* DIO_PCSELC[P6SELC] Bits */
#define P6SELC_OFS                                         ( 8)                  /*!< P6SELC Offset */
#define P6SELC_M                                           (0xff00)              /*!< Port 6 Complement Select */
/* DIO_PCIES[P5IES] Bits */
#define P5IES_OFS                                          ( 0)                  /*!< P5IES Offset */
#define P5IES_M                                            (0x00ff)              /*!< Port 5 Interrupt Edge Select */
/* DIO_PCIES[P6IES] Bits */
#define P6IES_OFS                                          ( 8)                  /*!< P6IES Offset */
#define P6IES_M                                            (0xff00)              /*!< Port 6 Interrupt Edge Select */
/* DIO_PCIE[P5IE] Bits */
#define P5IE_OFS                                           ( 0)                  /*!< P5IE Offset */
#define P5IE_M                                             (0x00ff)              /*!< Port 5 Interrupt Enable */
/* DIO_PCIE[P6IE] Bits */
#define P6IE_OFS                                           ( 8)                  /*!< P6IE Offset */
#define P6IE_M                                             (0xff00)              /*!< Port 6 Interrupt Enable */
/* DIO_PCIFG[P5IFG] Bits */
#define P5IFG_OFS                                          ( 0)                  /*!< P5IFG Offset */
#define P5IFG_M                                            (0x00ff)              /*!< Port 5 Interrupt Flag */
/* DIO_PCIFG[P6IFG] Bits */
#define P6IFG_OFS                                          ( 8)                  /*!< P6IFG Offset */
#define P6IFG_M                                            (0xff00)              /*!< Port 6 Interrupt Flag */
/* DIO_P6IV[P6IV] Bits */
#define P6IV_OFS                                           ( 0)                  /*!< P6IV Offset */
#define P6IV_M                                             (0x001f)              /*!< Port 6 interrupt vector value */
#define P6IV0                                              (0x0001)              /*!< Port 6 interrupt vector value */
#define P6IV1                                              (0x0002)              /*!< Port 6 interrupt vector value */
#define P6IV2                                              (0x0004)              /*!< Port 6 interrupt vector value */
#define P6IV3                                              (0x0008)              /*!< Port 6 interrupt vector value */
#define P6IV4                                              (0x0010)              /*!< Port 6 interrupt vector value */
#define P6IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P6IV_2                                             (0x0002)              /*!< Interrupt Source: Port 6.0 interrupt; Interrupt Flag: P6IFG0; Interrupt Priority: Highest */
#define P6IV_4                                             (0x0004)              /*!< Interrupt Source: Port 6.1 interrupt; Interrupt Flag: P6IFG1 */
#define P6IV_6                                             (0x0006)              /*!< Interrupt Source: Port 6.2 interrupt; Interrupt Flag: P6IFG2 */
#define P6IV_8                                             (0x0008)              /*!< Interrupt Source: Port 6.3 interrupt; Interrupt Flag: P6IFG3 */
#define P6IV_10                                            (0x000a)              /*!< Interrupt Source: Port 6.4 interrupt; Interrupt Flag: P6IFG4 */
#define P6IV_12                                            (0x000c)              /*!< Interrupt Source: Port 6.5 interrupt; Interrupt Flag: P6IFG5 */
#define P6IV_14                                            (0x000e)              /*!< Interrupt Source: Port 6.6 interrupt; Interrupt Flag: P6IFG6 */
#define P6IV_16                                            (0x0010)              /*!< Interrupt Source: Port 6.7 interrupt; Interrupt Flag: P6IFG7; Interrupt Priority: Lowest */
#define P6IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P6IV__P6IFG0                                       (0x0002)              /*!< Interrupt Source: Port 6.0 interrupt; Interrupt Flag: P6IFG0; Interrupt Priority: Highest */
#define P6IV__P6IFG1                                       (0x0004)              /*!< Interrupt Source: Port 6.1 interrupt; Interrupt Flag: P6IFG1 */
#define P6IV__P6IFG2                                       (0x0006)              /*!< Interrupt Source: Port 6.2 interrupt; Interrupt Flag: P6IFG2 */
#define P6IV__P6IFG3                                       (0x0008)              /*!< Interrupt Source: Port 6.3 interrupt; Interrupt Flag: P6IFG3 */
#define P6IV__P6IFG4                                       (0x000a)              /*!< Interrupt Source: Port 6.4 interrupt; Interrupt Flag: P6IFG4 */
#define P6IV__P6IFG5                                       (0x000c)              /*!< Interrupt Source: Port 6.5 interrupt; Interrupt Flag: P6IFG5 */
#define P6IV__P6IFG6                                       (0x000e)              /*!< Interrupt Source: Port 6.6 interrupt; Interrupt Flag: P6IFG6 */
#define P6IV__P6IFG7                                       (0x0010)              /*!< Interrupt Source: Port 6.7 interrupt; Interrupt Flag: P6IFG7; Interrupt Priority: Lowest */
/* DIO_PDIN[P7IN] Bits */
#define P7IN_OFS                                           ( 0)                  /*!< P7IN Offset */
#define P7IN_M                                             (0x00ff)              /*!< Port 7 Input */
/* DIO_PDIN[P8IN] Bits */
#define P8IN_OFS                                           ( 8)                  /*!< P8IN Offset */
#define P8IN_M                                             (0xff00)              /*!< Port 8 Input */
/* DIO_PDOUT[P7OUT] Bits */
#define P7OUT_OFS                                          ( 0)                  /*!< P7OUT Offset */
#define P7OUT_M                                            (0x00ff)              /*!< Port 7 Output */
/* DIO_PDOUT[P8OUT] Bits */
#define P8OUT_OFS                                          ( 8)                  /*!< P8OUT Offset */
#define P8OUT_M                                            (0xff00)              /*!< Port 8 Output */
/* DIO_PDDIR[P7DIR] Bits */
#define P7DIR_OFS                                          ( 0)                  /*!< P7DIR Offset */
#define P7DIR_M                                            (0x00ff)              /*!< Port 7 Direction */
/* DIO_PDDIR[P8DIR] Bits */
#define P8DIR_OFS                                          ( 8)                  /*!< P8DIR Offset */
#define P8DIR_M                                            (0xff00)              /*!< Port 8 Direction */
/* DIO_PDREN[P7REN] Bits */
#define P7REN_OFS                                          ( 0)                  /*!< P7REN Offset */
#define P7REN_M                                            (0x00ff)              /*!< Port 7 Resistor Enable */
/* DIO_PDREN[P8REN] Bits */
#define P8REN_OFS                                          ( 8)                  /*!< P8REN Offset */
#define P8REN_M                                            (0xff00)              /*!< Port 8 Resistor Enable */
/* DIO_PDDS[P7DS] Bits */
#define P7DS_OFS                                           ( 0)                  /*!< P7DS Offset */
#define P7DS_M                                             (0x00ff)              /*!< Port 7 Drive Strength */
/* DIO_PDDS[P8DS] Bits */
#define P8DS_OFS                                           ( 8)                  /*!< P8DS Offset */
#define P8DS_M                                             (0xff00)              /*!< Port 8 Drive Strength */
/* DIO_PDSEL0[P7SEL0] Bits */
#define P7SEL0_OFS                                         ( 0)                  /*!< P7SEL0 Offset */
#define P7SEL0_M                                           (0x00ff)              /*!< Port 7 Select 0 */
/* DIO_PDSEL0[P8SEL0] Bits */
#define P8SEL0_OFS                                         ( 8)                  /*!< P8SEL0 Offset */
#define P8SEL0_M                                           (0xff00)              /*!< Port 8 Select 0 */
/* DIO_PDSEL1[P7SEL1] Bits */
#define P7SEL1_OFS                                         ( 0)                  /*!< P7SEL1 Offset */
#define P7SEL1_M                                           (0x00ff)              /*!< Port 7 Select 1 */
/* DIO_PDSEL1[P8SEL1] Bits */
#define P8SEL1_OFS                                         ( 8)                  /*!< P8SEL1 Offset */
#define P8SEL1_M                                           (0xff00)              /*!< Port 8 Select 1 */
/* DIO_P7IV[P7IV] Bits */
#define P7IV_OFS                                           ( 0)                  /*!< P7IV Offset */
#define P7IV_M                                             (0x001f)              /*!< Port 7 interrupt vector value */
#define P7IV0                                              (0x0001)              /*!< Port 7 interrupt vector value */
#define P7IV1                                              (0x0002)              /*!< Port 7 interrupt vector value */
#define P7IV2                                              (0x0004)              /*!< Port 7 interrupt vector value */
#define P7IV3                                              (0x0008)              /*!< Port 7 interrupt vector value */
#define P7IV4                                              (0x0010)              /*!< Port 7 interrupt vector value */
#define P7IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P7IV_2                                             (0x0002)              /*!< Interrupt Source: Port 7.0 interrupt; Interrupt Flag: P7IFG0; Interrupt Priority: Highest */
#define P7IV_4                                             (0x0004)              /*!< Interrupt Source: Port 7.1 interrupt; Interrupt Flag: P7IFG1 */
#define P7IV_6                                             (0x0006)              /*!< Interrupt Source: Port 7.2 interrupt; Interrupt Flag: P7IFG2 */
#define P7IV_8                                             (0x0008)              /*!< Interrupt Source: Port 7.3 interrupt; Interrupt Flag: P7IFG3 */
#define P7IV_10                                            (0x000a)              /*!< Interrupt Source: Port 7.4 interrupt; Interrupt Flag: P7IFG4 */
#define P7IV_12                                            (0x000c)              /*!< Interrupt Source: Port 7.5 interrupt; Interrupt Flag: P7IFG5 */
#define P7IV_14                                            (0x000e)              /*!< Interrupt Source: Port 7.6 interrupt; Interrupt Flag: P7IFG6 */
#define P7IV_16                                            (0x0010)              /*!< Interrupt Source: Port 7.7 interrupt; Interrupt Flag: P7IFG7; Interrupt Priority: Lowest */
#define P7IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P7IV__P7IFG0                                       (0x0002)              /*!< Interrupt Source: Port 7.0 interrupt; Interrupt Flag: P7IFG0; Interrupt Priority: Highest */
#define P7IV__P7IFG1                                       (0x0004)              /*!< Interrupt Source: Port 7.1 interrupt; Interrupt Flag: P7IFG1 */
#define P7IV__P7IFG2                                       (0x0006)              /*!< Interrupt Source: Port 7.2 interrupt; Interrupt Flag: P7IFG2 */
#define P7IV__P7IFG3                                       (0x0008)              /*!< Interrupt Source: Port 7.3 interrupt; Interrupt Flag: P7IFG3 */
#define P7IV__P7IFG4                                       (0x000a)              /*!< Interrupt Source: Port 7.4 interrupt; Interrupt Flag: P7IFG4 */
#define P7IV__P7IFG5                                       (0x000c)              /*!< Interrupt Source: Port 7.5 interrupt; Interrupt Flag: P7IFG5 */
#define P7IV__P7IFG6                                       (0x000e)              /*!< Interrupt Source: Port 7.6 interrupt; Interrupt Flag: P7IFG6 */
#define P7IV__P7IFG7                                       (0x0010)              /*!< Interrupt Source: Port 7.7 interrupt; Interrupt Flag: P7IFG7; Interrupt Priority: Lowest */
/* DIO_PDSELC[P7SELC] Bits */
#define P7SELC_OFS                                         ( 0)                  /*!< P7SELC Offset */
#define P7SELC_M                                           (0x00ff)              /*!< Port 7 Complement Select */
/* DIO_PDSELC[P8SELC] Bits */
#define P8SELC_OFS                                         ( 8)                  /*!< P8SELC Offset */
#define P8SELC_M                                           (0xff00)              /*!< Port 8 Complement Select */
/* DIO_PDIES[P7IES] Bits */
#define P7IES_OFS                                          ( 0)                  /*!< P7IES Offset */
#define P7IES_M                                            (0x00ff)              /*!< Port 7 Interrupt Edge Select */
/* DIO_PDIES[P8IES] Bits */
#define P8IES_OFS                                          ( 8)                  /*!< P8IES Offset */
#define P8IES_M                                            (0xff00)              /*!< Port 8 Interrupt Edge Select */
/* DIO_PDIE[P7IE] Bits */
#define P7IE_OFS                                           ( 0)                  /*!< P7IE Offset */
#define P7IE_M                                             (0x00ff)              /*!< Port 7 Interrupt Enable */
/* DIO_PDIE[P8IE] Bits */
#define P8IE_OFS                                           ( 8)                  /*!< P8IE Offset */
#define P8IE_M                                             (0xff00)              /*!< Port 8 Interrupt Enable */
/* DIO_PDIFG[P7IFG] Bits */
#define P7IFG_OFS                                          ( 0)                  /*!< P7IFG Offset */
#define P7IFG_M                                            (0x00ff)              /*!< Port 7 Interrupt Flag */
/* DIO_PDIFG[P8IFG] Bits */
#define P8IFG_OFS                                          ( 8)                  /*!< P8IFG Offset */
#define P8IFG_M                                            (0xff00)              /*!< Port 8 Interrupt Flag */
/* DIO_P8IV[P8IV] Bits */
#define P8IV_OFS                                           ( 0)                  /*!< P8IV Offset */
#define P8IV_M                                             (0x001f)              /*!< Port 8 interrupt vector value */
#define P8IV0                                              (0x0001)              /*!< Port 8 interrupt vector value */
#define P8IV1                                              (0x0002)              /*!< Port 8 interrupt vector value */
#define P8IV2                                              (0x0004)              /*!< Port 8 interrupt vector value */
#define P8IV3                                              (0x0008)              /*!< Port 8 interrupt vector value */
#define P8IV4                                              (0x0010)              /*!< Port 8 interrupt vector value */
#define P8IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P8IV_2                                             (0x0002)              /*!< Interrupt Source: Port 8.0 interrupt; Interrupt Flag: P8IFG0; Interrupt Priority: Highest */
#define P8IV_4                                             (0x0004)              /*!< Interrupt Source: Port 8.1 interrupt; Interrupt Flag: P8IFG1 */
#define P8IV_6                                             (0x0006)              /*!< Interrupt Source: Port 8.2 interrupt; Interrupt Flag: P8IFG2 */
#define P8IV_8                                             (0x0008)              /*!< Interrupt Source: Port 8.3 interrupt; Interrupt Flag: P8IFG3 */
#define P8IV_10                                            (0x000a)              /*!< Interrupt Source: Port 8.4 interrupt; Interrupt Flag: P8IFG4 */
#define P8IV_12                                            (0x000c)              /*!< Interrupt Source: Port 8.5 interrupt; Interrupt Flag: P8IFG5 */
#define P8IV_14                                            (0x000e)              /*!< Interrupt Source: Port 8.6 interrupt; Interrupt Flag: P8IFG6 */
#define P8IV_16                                            (0x0010)              /*!< Interrupt Source: Port 8.7 interrupt; Interrupt Flag: P8IFG7; Interrupt Priority: Lowest */
#define P8IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P8IV__P8IFG0                                       (0x0002)              /*!< Interrupt Source: Port 8.0 interrupt; Interrupt Flag: P8IFG0; Interrupt Priority: Highest */
#define P8IV__P8IFG1                                       (0x0004)              /*!< Interrupt Source: Port 8.1 interrupt; Interrupt Flag: P8IFG1 */
#define P8IV__P8IFG2                                       (0x0006)              /*!< Interrupt Source: Port 8.2 interrupt; Interrupt Flag: P8IFG2 */
#define P8IV__P8IFG3                                       (0x0008)              /*!< Interrupt Source: Port 8.3 interrupt; Interrupt Flag: P8IFG3 */
#define P8IV__P8IFG4                                       (0x000a)              /*!< Interrupt Source: Port 8.4 interrupt; Interrupt Flag: P8IFG4 */
#define P8IV__P8IFG5                                       (0x000c)              /*!< Interrupt Source: Port 8.5 interrupt; Interrupt Flag: P8IFG5 */
#define P8IV__P8IFG6                                       (0x000e)              /*!< Interrupt Source: Port 8.6 interrupt; Interrupt Flag: P8IFG6 */
#define P8IV__P8IFG7                                       (0x0010)              /*!< Interrupt Source: Port 8.7 interrupt; Interrupt Flag: P8IFG7; Interrupt Priority: Lowest */
/* DIO_PEIN[P9IN] Bits */
#define P9IN_OFS                                           ( 0)                  /*!< P9IN Offset */
#define P9IN_M                                             (0x00ff)              /*!< Port 9 Input */
/* DIO_PEIN[P10IN] Bits */
#define P10IN_OFS                                          ( 8)                  /*!< P10IN Offset */
#define P10IN_M                                            (0xff00)              /*!< Port 10 Input */
/* DIO_PEOUT[P9OUT] Bits */
#define P9OUT_OFS                                          ( 0)                  /*!< P9OUT Offset */
#define P9OUT_M                                            (0x00ff)              /*!< Port 9 Output */
/* DIO_PEOUT[P10OUT] Bits */
#define P10OUT_OFS                                         ( 8)                  /*!< P10OUT Offset */
#define P10OUT_M                                           (0xff00)              /*!< Port 10 Output */
/* DIO_PEDIR[P9DIR] Bits */
#define P9DIR_OFS                                          ( 0)                  /*!< P9DIR Offset */
#define P9DIR_M                                            (0x00ff)              /*!< Port 9 Direction */
/* DIO_PEDIR[P10DIR] Bits */
#define P10DIR_OFS                                         ( 8)                  /*!< P10DIR Offset */
#define P10DIR_M                                           (0xff00)              /*!< Port 10 Direction */
/* DIO_PEREN[P9REN] Bits */
#define P9REN_OFS                                          ( 0)                  /*!< P9REN Offset */
#define P9REN_M                                            (0x00ff)              /*!< Port 9 Resistor Enable */
/* DIO_PEREN[P10REN] Bits */
#define P10REN_OFS                                         ( 8)                  /*!< P10REN Offset */
#define P10REN_M                                           (0xff00)              /*!< Port 10 Resistor Enable */
/* DIO_PEDS[P9DS] Bits */
#define P9DS_OFS                                           ( 0)                  /*!< P9DS Offset */
#define P9DS_M                                             (0x00ff)              /*!< Port 9 Drive Strength */
/* DIO_PEDS[P10DS] Bits */
#define P10DS_OFS                                          ( 8)                  /*!< P10DS Offset */
#define P10DS_M                                            (0xff00)              /*!< Port 10 Drive Strength */
/* DIO_PESEL0[P9SEL0] Bits */
#define P9SEL0_OFS                                         ( 0)                  /*!< P9SEL0 Offset */
#define P9SEL0_M                                           (0x00ff)              /*!< Port 9 Select 0 */
/* DIO_PESEL0[P10SEL0] Bits */
#define P10SEL0_OFS                                        ( 8)                  /*!< P10SEL0 Offset */
#define P10SEL0_M                                          (0xff00)              /*!< Port 10 Select 0 */
/* DIO_PESEL1[P9SEL1] Bits */
#define P9SEL1_OFS                                         ( 0)                  /*!< P9SEL1 Offset */
#define P9SEL1_M                                           (0x00ff)              /*!< Port 9 Select 1 */
/* DIO_PESEL1[P10SEL1] Bits */
#define P10SEL1_OFS                                        ( 8)                  /*!< P10SEL1 Offset */
#define P10SEL1_M                                          (0xff00)              /*!< Port 10 Select 1 */
/* DIO_P9IV[P9IV] Bits */
#define P9IV_OFS                                           ( 0)                  /*!< P9IV Offset */
#define P9IV_M                                             (0x001f)              /*!< Port 9 interrupt vector value */
#define P9IV0                                              (0x0001)              /*!< Port 9 interrupt vector value */
#define P9IV1                                              (0x0002)              /*!< Port 9 interrupt vector value */
#define P9IV2                                              (0x0004)              /*!< Port 9 interrupt vector value */
#define P9IV3                                              (0x0008)              /*!< Port 9 interrupt vector value */
#define P9IV4                                              (0x0010)              /*!< Port 9 interrupt vector value */
#define P9IV_0                                             (0x0000)              /*!< No interrupt pending */
#define P9IV_2                                             (0x0002)              /*!< Interrupt Source: Port 9.0 interrupt; Interrupt Flag: P9IFG0; Interrupt Priority: Highest */
#define P9IV_4                                             (0x0004)              /*!< Interrupt Source: Port 9.1 interrupt; Interrupt Flag: P9IFG1 */
#define P9IV_6                                             (0x0006)              /*!< Interrupt Source: Port 9.2 interrupt; Interrupt Flag: P9IFG2 */
#define P9IV_8                                             (0x0008)              /*!< Interrupt Source: Port 9.3 interrupt; Interrupt Flag: P9IFG3 */
#define P9IV_10                                            (0x000a)              /*!< Interrupt Source: Port 9.4 interrupt; Interrupt Flag: P9IFG4 */
#define P9IV_12                                            (0x000c)              /*!< Interrupt Source: Port 9.5 interrupt; Interrupt Flag: P9IFG5 */
#define P9IV_14                                            (0x000e)              /*!< Interrupt Source: Port 9.6 interrupt; Interrupt Flag: P9IFG6 */
#define P9IV_16                                            (0x0010)              /*!< Interrupt Source: Port 9.7 interrupt; Interrupt Flag: P9IFG7; Interrupt Priority: Lowest */
#define P9IV__NONE                                         (0x0000)              /*!< No interrupt pending */
#define P9IV__P9IFG0                                       (0x0002)              /*!< Interrupt Source: Port 9.0 interrupt; Interrupt Flag: P9IFG0; Interrupt Priority: Highest */
#define P9IV__P9IFG1                                       (0x0004)              /*!< Interrupt Source: Port 9.1 interrupt; Interrupt Flag: P9IFG1 */
#define P9IV__P9IFG2                                       (0x0006)              /*!< Interrupt Source: Port 9.2 interrupt; Interrupt Flag: P9IFG2 */
#define P9IV__P9IFG3                                       (0x0008)              /*!< Interrupt Source: Port 9.3 interrupt; Interrupt Flag: P9IFG3 */
#define P9IV__P9IFG4                                       (0x000a)              /*!< Interrupt Source: Port 9.4 interrupt; Interrupt Flag: P9IFG4 */
#define P9IV__P9IFG5                                       (0x000c)              /*!< Interrupt Source: Port 9.5 interrupt; Interrupt Flag: P9IFG5 */
#define P9IV__P9IFG6                                       (0x000e)              /*!< Interrupt Source: Port 9.6 interrupt; Interrupt Flag: P9IFG6 */
#define P9IV__P9IFG7                                       (0x0010)              /*!< Interrupt Source: Port 9.7 interrupt; Interrupt Flag: P9IFG7; Interrupt Priority: Lowest */
/* DIO_PESELC[P9SELC] Bits */
#define P9SELC_OFS                                         ( 0)                  /*!< P9SELC Offset */
#define P9SELC_M                                           (0x00ff)              /*!< Port 9 Complement Select */
/* DIO_PESELC[P10SELC] Bits */
#define P10SELC_OFS                                        ( 8)                  /*!< P10SELC Offset */
#define P10SELC_M                                          (0xff00)              /*!< Port 10 Complement Select */
/* DIO_PEIES[P9IES] Bits */
#define P9IES_OFS                                          ( 0)                  /*!< P9IES Offset */
#define P9IES_M                                            (0x00ff)              /*!< Port 9 Interrupt Edge Select */
/* DIO_PEIES[P10IES] Bits */
#define P10IES_OFS                                         ( 8)                  /*!< P10IES Offset */
#define P10IES_M                                           (0xff00)              /*!< Port 10 Interrupt Edge Select */
/* DIO_PEIE[P9IE] Bits */
#define P9IE_OFS                                           ( 0)                  /*!< P9IE Offset */
#define P9IE_M                                             (0x00ff)              /*!< Port 9 Interrupt Enable */
/* DIO_PEIE[P10IE] Bits */
#define P10IE_OFS                                          ( 8)                  /*!< P10IE Offset */
#define P10IE_M                                            (0xff00)              /*!< Port 10 Interrupt Enable */
/* DIO_PEIFG[P9IFG] Bits */
#define P9IFG_OFS                                          ( 0)                  /*!< P9IFG Offset */
#define P9IFG_M                                            (0x00ff)              /*!< Port 9 Interrupt Flag */
/* DIO_PEIFG[P10IFG] Bits */
#define P10IFG_OFS                                         ( 8)                  /*!< P10IFG Offset */
#define P10IFG_M                                           (0xff00)              /*!< Port 10 Interrupt Flag */
/* DIO_P10IV[P10IV] Bits */
#define P10IV_OFS                                          ( 0)                  /*!< P10IV Offset */
#define P10IV_M                                            (0x001f)              /*!< Port 10 interrupt vector value */
#define P10IV0                                             (0x0001)              /*!< Port 10 interrupt vector value */
#define P10IV1                                             (0x0002)              /*!< Port 10 interrupt vector value */
#define P10IV2                                             (0x0004)              /*!< Port 10 interrupt vector value */
#define P10IV3                                             (0x0008)              /*!< Port 10 interrupt vector value */
#define P10IV4                                             (0x0010)              /*!< Port 10 interrupt vector value */
#define P10IV_0                                            (0x0000)              /*!< No interrupt pending */
#define P10IV_2                                            (0x0002)              /*!< Interrupt Source: Port 10.0 interrupt; Interrupt Flag: P10IFG0; Interrupt Priority: Highest */
#define P10IV_4                                            (0x0004)              /*!< Interrupt Source: Port 10.1 interrupt; Interrupt Flag: P10IFG1 */
#define P10IV_6                                            (0x0006)              /*!< Interrupt Source: Port 10.2 interrupt; Interrupt Flag: P10IFG2 */
#define P10IV_8                                            (0x0008)              /*!< Interrupt Source: Port 10.3 interrupt; Interrupt Flag: P10IFG3 */
#define P10IV_10                                           (0x000a)              /*!< Interrupt Source: Port 10.4 interrupt; Interrupt Flag: P10IFG4 */
#define P10IV_12                                           (0x000c)              /*!< Interrupt Source: Port 10.5 interrupt; Interrupt Flag: P10IFG5 */
#define P10IV_14                                           (0x000e)              /*!< Interrupt Source: Port 10.6 interrupt; Interrupt Flag: P10IFG6 */
#define P10IV_16                                           (0x0010)              /*!< Interrupt Source: Port 10.7 interrupt; Interrupt Flag: P10IFG7; Interrupt Priority: Lowest */
#define P10IV__NONE                                        (0x0000)              /*!< No interrupt pending */
#define P10IV__P10IFG0                                     (0x0002)              /*!< Interrupt Source: Port 10.0 interrupt; Interrupt Flag: P10IFG0; Interrupt Priority: Highest */
#define P10IV__P10IFG1                                     (0x0004)              /*!< Interrupt Source: Port 10.1 interrupt; Interrupt Flag: P10IFG1 */
#define P10IV__P10IFG2                                     (0x0006)              /*!< Interrupt Source: Port 10.2 interrupt; Interrupt Flag: P10IFG2 */
#define P10IV__P10IFG3                                     (0x0008)              /*!< Interrupt Source: Port 10.3 interrupt; Interrupt Flag: P10IFG3 */
#define P10IV__P10IFG4                                     (0x000a)              /*!< Interrupt Source: Port 10.4 interrupt; Interrupt Flag: P10IFG4 */
#define P10IV__P10IFG5                                     (0x000c)              /*!< Interrupt Source: Port 10.5 interrupt; Interrupt Flag: P10IFG5 */
#define P10IV__P10IFG6                                     (0x000e)              /*!< Interrupt Source: Port 10.6 interrupt; Interrupt Flag: P10IFG6 */
#define P10IV__P10IFG7                                     (0x0010)              /*!< Interrupt Source: Port 10.7 interrupt; Interrupt Flag: P10IFG7; Interrupt Priority: Lowest */


/******************************************************************************
* EUSCI_A Bits (legacy section)
******************************************************************************/
/* UCA0CTLW0[UCSWRST] Bits */
#define UCSWRST_OFS                              EUSCI_A_CTLW0_SWRST_OFS         /*!< UCSWRST Offset */
#define UCSWRST                                  EUSCI_A_CTLW0_SWRST             /*!< Software reset enable */
/* UCA0CTLW0[UCTXBRK] Bits */
#define UCTXBRK_OFS                              EUSCI_A_CTLW0_TXBRK_OFS         /*!< UCTXBRK Offset */
#define UCTXBRK                                  EUSCI_A_CTLW0_TXBRK             /*!< Transmit break */
/* UCA0CTLW0[UCTXADDR] Bits */
#define UCTXADDR_OFS                             EUSCI_A_CTLW0_TXADDR_OFS        /*!< UCTXADDR Offset */
#define UCTXADDR                                 EUSCI_A_CTLW0_TXADDR            /*!< Transmit address */
/* UCA0CTLW0[UCDORM] Bits */
#define UCDORM_OFS                               EUSCI_A_CTLW0_DORM_OFS          /*!< UCDORM Offset */
#define UCDORM                                   EUSCI_A_CTLW0_DORM              /*!< Dormant */
/* UCA0CTLW0[UCBRKIE] Bits */
#define UCBRKIE_OFS                              EUSCI_A_CTLW0_BRKIE_OFS         /*!< UCBRKIE Offset */
#define UCBRKIE                                  EUSCI_A_CTLW0_BRKIE             /*!< Receive break character interrupt enable */
/* UCA0CTLW0[UCRXEIE] Bits */
#define UCRXEIE_OFS                              EUSCI_A_CTLW0_RXEIE_OFS         /*!< UCRXEIE Offset */
#define UCRXEIE                                  EUSCI_A_CTLW0_RXEIE             /*!< Receive erroneous-character interrupt enable */
/* UCA0CTLW0[UCSSEL] Bits */
#define UCSSEL_OFS                               EUSCI_A_CTLW0_SSEL_OFS          /*!< UCSSEL Offset */
#define UCSSEL_M                                 EUSCI_A_CTLW0_SSEL_MASK         /*!< eUSCI_A clock source select */
#define UCSSEL0                                  EUSCI_A_CTLW0_SSEL0             /*!< UCSSEL Bit 0 */
#define UCSSEL1                                  EUSCI_A_CTLW0_SSEL1             /*!< UCSSEL Bit 1 */
#define UCSSEL_0                                 EUSCI_A_CTLW0_UCSSEL_0          /*!< UCLK */
#define UCSSEL_1                                 EUSCI_A_CTLW0_UCSSEL_1          /*!< ACLK */
#define UCSSEL_2                                 EUSCI_A_CTLW0_UCSSEL_2          /*!< SMCLK */
#define UCSSEL__UCLK                             EUSCI_A_CTLW0_SSEL__UCLK        /*!< UCLK */
#define UCSSEL__ACLK                             EUSCI_A_CTLW0_SSEL__ACLK        /*!< ACLK */
#define UCSSEL__SMCLK                            EUSCI_A_CTLW0_SSEL__SMCLK       /*!< SMCLK */
/* UCA0CTLW0[UCSYNC] Bits */
#define UCSYNC_OFS                               EUSCI_A_CTLW0_SYNC_OFS          /*!< UCSYNC Offset */
#define UCSYNC                                   EUSCI_A_CTLW0_SYNC              /*!< Synchronous mode enable */
/* UCA0CTLW0[UCMODE] Bits */
#define UCMODE_OFS                               EUSCI_A_CTLW0_MODE_OFS          /*!< UCMODE Offset */
#define UCMODE_M                                 EUSCI_A_CTLW0_MODE_MASK         /*!< eUSCI_A mode */
#define UCMODE0                                  EUSCI_A_CTLW0_MODE0             /*!< UCMODE Bit 0 */
#define UCMODE1                                  EUSCI_A_CTLW0_MODE1             /*!< UCMODE Bit 1 */
#define UCMODE_0                                 EUSCI_A_CTLW0_MODE_0            /*!< UART mode */
#define UCMODE_1                                 EUSCI_A_CTLW0_MODE_1            /*!< Idle-line multiprocessor mode */
#define UCMODE_2                                 EUSCI_A_CTLW0_MODE_2            /*!< Address-bit multiprocessor mode */
#define UCMODE_3                                 EUSCI_A_CTLW0_MODE_3            /*!< UART mode with automatic baud-rate detection */
/* UCA0CTLW0[UCSPB] Bits */
#define UCSPB_OFS                                EUSCI_A_CTLW0_SPB_OFS           /*!< UCSPB Offset */
#define UCSPB                                    EUSCI_A_CTLW0_SPB               /*!< Stop bit select */
/* UCA0CTLW0[UC7BIT] Bits */
#define UC7BIT_OFS                               EUSCI_A_CTLW0_SEVENBIT_OFS      /*!< UC7BIT Offset */
#define UC7BIT                                   EUSCI_A_CTLW0_SEVENBIT          /*!< Character length */
/* UCA0CTLW0[UCMSB] Bits */
#define UCMSB_OFS                                EUSCI_A_CTLW0_MSB_OFS           /*!< UCMSB Offset */
#define UCMSB                                    EUSCI_A_CTLW0_MSB               /*!< MSB first select */
/* UCA0CTLW0[UCPAR] Bits */
#define UCPAR_OFS                                EUSCI_A_CTLW0_PAR_OFS           /*!< UCPAR Offset */
#define UCPAR                                    EUSCI_A_CTLW0_PAR               /*!< Parity select */
/* UCA0CTLW0[UCPEN] Bits */
#define UCPEN_OFS                                EUSCI_A_CTLW0_PEN_OFS           /*!< UCPEN Offset */
#define UCPEN                                    EUSCI_A_CTLW0_PEN               /*!< Parity enable */
/* UCA0CTLW0_SPI[UCSWRST] Bits */
//#define UCSWRST_OFS                              EUSCI_A_CTLW0_SWRST_OFS         /*!< UCSWRST Offset */
//#define UCSWRST                                  EUSCI_A_CTLW0_SWRST             /*!< Software reset enable */
/* UCA0CTLW0_SPI[UCSTEM] Bits */
#define UCSTEM_OFS                               EUSCI_A_CTLW0_STEM_OFS          /*!< UCSTEM Offset */
#define UCSTEM                                   EUSCI_A_CTLW0_STEM              /*!< STE mode select in master mode. */
/* UCA0CTLW0_SPI[UCSSEL] Bits */
//#define UCSSEL_OFS                               EUSCI_A_CTLW0_SSEL_OFS          /*!< UCSSEL Offset */
//#define UCSSEL_M                                 EUSCI_A_CTLW0_SSEL_MASK         /*!< eUSCI_A clock source select */
//#define UCSSEL0                                  EUSCI_A_CTLW0_SSEL0             /*!< UCSSEL Bit 0 */
//#define UCSSEL1                                  EUSCI_A_CTLW0_SSEL1             /*!< UCSSEL Bit 1 */
//#define UCSSEL_0                                 EUSCI_A_CTLW0_UCSSEL_0          /*!< Reserved */
//#define UCSSEL_1                                 EUSCI_A_CTLW0_UCSSEL_1          /*!< ACLK */
//#define UCSSEL_2                                 EUSCI_A_CTLW0_UCSSEL_2          /*!< SMCLK */
//#define UCSSEL__ACLK                             EUSCI_A_CTLW0_SSEL__ACLK        /*!< ACLK */
//#define UCSSEL__SMCLK                            EUSCI_A_CTLW0_SSEL__SMCLK       /*!< SMCLK */
/* UCA0CTLW0_SPI[UCSYNC] Bits */
//#define UCSYNC_OFS                               EUSCI_A_CTLW0_SYNC_OFS          /*!< UCSYNC Offset */
//#define UCSYNC                                   EUSCI_A_CTLW0_SYNC              /*!< Synchronous mode enable */
/* UCA0CTLW0_SPI[UCMODE] Bits */
//#define UCMODE_OFS                               EUSCI_A_CTLW0_MODE_OFS          /*!< UCMODE Offset */
//#define UCMODE_M                                 EUSCI_A_CTLW0_MODE_MASK         /*!< eUSCI mode */
//#define UCMODE0                                  EUSCI_A_CTLW0_MODE0             /*!< UCMODE Bit 0 */
//#define UCMODE1                                  EUSCI_A_CTLW0_MODE1             /*!< UCMODE Bit 1 */
//#define UCMODE_0                                 EUSCI_A_CTLW0_MODE_0            /*!< 3-pin SPI */
//#define UCMODE_1                                 EUSCI_A_CTLW0_MODE_1            /*!< 4-pin SPI with UCxSTE active high: Slave enabled when UCxSTE = 1 */
//#define UCMODE_2                                 EUSCI_A_CTLW0_MODE_2            /*!< 4-pin SPI with UCxSTE active low: Slave enabled when UCxSTE = 0 */
/* UCA0CTLW0_SPI[UCMST] Bits */
#define UCMST_OFS                                EUSCI_A_CTLW0_MST_OFS           /*!< UCMST Offset */
#define UCMST                                    EUSCI_A_CTLW0_MST               /*!< Master mode select */
/* UCA0CTLW0_SPI[UC7BIT] Bits */
//#define UC7BIT_OFS                               EUSCI_A_CTLW0_SEVENBIT_OFS      /*!< UC7BIT Offset */
//#define UC7BIT                                   EUSCI_A_CTLW0_SEVENBIT          /*!< Character length */
/* UCA0CTLW0_SPI[UCMSB] Bits */
//#define UCMSB_OFS                                EUSCI_A_CTLW0_MSB_OFS           /*!< UCMSB Offset */
//#define UCMSB                                    EUSCI_A_CTLW0_MSB               /*!< MSB first select */
/* UCA0CTLW0_SPI[UCCKPL] Bits */
#define UCCKPL_OFS                               EUSCI_A_CTLW0_CKPL_OFS          /*!< UCCKPL Offset */
#define UCCKPL                                   EUSCI_A_CTLW0_CKPL              /*!< Clock polarity select */
/* UCA0CTLW0_SPI[UCCKPH] Bits */
#define UCCKPH_OFS                               EUSCI_A_CTLW0_CKPH_OFS          /*!< UCCKPH Offset */
#define UCCKPH                                   EUSCI_A_CTLW0_CKPH              /*!< Clock phase select */
/* UCA0CTLW1[UCGLIT] Bits */
#define UCGLIT_OFS                               EUSCI_A_CTLW1_GLIT_OFS          /*!< UCGLIT Offset */
#define UCGLIT_M                                 EUSCI_A_CTLW1_GLIT_MASK         /*!< Deglitch time */
#define UCGLIT0                                  EUSCI_A_CTLW1_GLIT0             /*!< UCGLIT Bit 0 */
#define UCGLIT1                                  EUSCI_A_CTLW1_GLIT1             /*!< UCGLIT Bit 1 */
#define UCGLIT_0                                 EUSCI_A_CTLW1_GLIT_0            /*!< Approximately 2 ns (equivalent of 1 delay element) */
#define UCGLIT_1                                 EUSCI_A_CTLW1_GLIT_1            /*!< Approximately 50 ns */
#define UCGLIT_2                                 EUSCI_A_CTLW1_GLIT_2            /*!< Approximately 100 ns */
#define UCGLIT_3                                 EUSCI_A_CTLW1_GLIT_3            /*!< Approximately 200 ns */
/* UCA0MCTLW[UCOS16] Bits */
#define UCOS16_OFS                               EUSCI_A_MCTLW_OS16_OFS          /*!< UCOS16 Offset */
#define UCOS16                                   EUSCI_A_MCTLW_OS16              /*!< Oversampling mode enabled */
/* UCA0MCTLW[UCBRF] Bits */
#define UCBRF_OFS                                EUSCI_A_MCTLW_BRF_OFS           /*!< UCBRF Offset */
#define UCBRF_M                                  EUSCI_A_MCTLW_BRF_MASK          /*!< First modulation stage select */
/* UCA0MCTLW[UCBRS] Bits */
#define UCBRS_OFS                                EUSCI_A_MCTLW_BRS_OFS           /*!< UCBRS Offset */
#define UCBRS_M                                  EUSCI_A_MCTLW_BRS_MASK          /*!< Second modulation stage select */
/* UCA0STATW[UCBUSY] Bits */
#define UCBUSY_OFS                               EUSCI_A_STATW_BUSY_OFS          /*!< UCBUSY Offset */
#define UCBUSY                                   EUSCI_A_STATW_BUSY              /*!< eUSCI_A busy */
/* UCA0STATW[UCADDR_UCIDLE] Bits */
#define UCADDR_UCIDLE_OFS                        EUSCI_A_STATW_ADDR_IDLE_OFS     /*!< UCADDR_UCIDLE Offset */
#define UCADDR_UCIDLE                            EUSCI_A_STATW_ADDR_IDLE         /*!< Address received / Idle line detected */
/* UCA0STATW[UCRXERR] Bits */
#define UCRXERR_OFS                              EUSCI_A_STATW_RXERR_OFS         /*!< UCRXERR Offset */
#define UCRXERR                                  EUSCI_A_STATW_RXERR             /*!< Receive error flag */
/* UCA0STATW[UCBRK] Bits */
#define UCBRK_OFS                                EUSCI_A_STATW_BRK_OFS           /*!< UCBRK Offset */
#define UCBRK                                    EUSCI_A_STATW_BRK               /*!< Break detect flag */
/* UCA0STATW[UCPE] Bits */
#define UCPE_OFS                                 EUSCI_A_STATW_PE_OFS            /*!< UCPE Offset */
#define UCPE                                     EUSCI_A_STATW_PE                
/* UCA0STATW[UCOE] Bits */
#define UCOE_OFS                                 EUSCI_A_STATW_OE_OFS            /*!< UCOE Offset */
#define UCOE                                     EUSCI_A_STATW_OE                /*!< Overrun error flag */
/* UCA0STATW[UCFE] Bits */
#define UCFE_OFS                                 EUSCI_A_STATW_FE_OFS            /*!< UCFE Offset */
#define UCFE                                     EUSCI_A_STATW_FE                /*!< Framing error flag */
/* UCA0STATW[UCLISTEN] Bits */
#define UCLISTEN_OFS                             EUSCI_A_STATW_LISTEN_OFS        /*!< UCLISTEN Offset */
#define UCLISTEN                                 EUSCI_A_STATW_LISTEN            /*!< Listen enable */
/* UCA0STATW_SPI[UCBUSY] Bits */
//#define UCBUSY_OFS                               EUSCI_A_STATW_SPI_BUSY_OFS      /*!< UCBUSY Offset */
//#define UCBUSY                                   EUSCI_A_STATW_SPI_BUSY          /*!< eUSCI_A busy */
/* UCA0STATW_SPI[UCOE] Bits */
//#define UCOE_OFS                                 EUSCI_A_STATW_OE_OFS            /*!< UCOE Offset */
//#define UCOE                                     EUSCI_A_STATW_OE                /*!< Overrun error flag */
/* UCA0STATW_SPI[UCFE] Bits */
//#define UCFE_OFS                                 EUSCI_A_STATW_FE_OFS            /*!< UCFE Offset */
//#define UCFE                                     EUSCI_A_STATW_FE                /*!< Framing error flag */
/* UCA0STATW_SPI[UCLISTEN] Bits */
//#define UCLISTEN_OFS                             EUSCI_A_STATW_LISTEN_OFS        /*!< UCLISTEN Offset */
//#define UCLISTEN                                 EUSCI_A_STATW_LISTEN            /*!< Listen enable */
/* UCA0RXBUF[UCRXBUF] Bits */
#define UCRXBUF_OFS                              EUSCI_A_RXBUF_RXBUF_OFS         /*!< UCRXBUF Offset */
#define UCRXBUF_M                                EUSCI_A_RXBUF_RXBUF_MASK        /*!< Receive data buffer */
/* UCA0RXBUF_SPI[UCRXBUF] Bits */
//#define UCRXBUF_OFS                              EUSCI_A_RXBUF_RXBUF_OFS         /*!< UCRXBUF Offset */
//#define UCRXBUF_M                                EUSCI_A_RXBUF_RXBUF_MASK        /*!< Receive data buffer */
/* UCA0TXBUF[UCTXBUF] Bits */
#define UCTXBUF_OFS                              EUSCI_A_TXBUF_TXBUF_OFS         /*!< UCTXBUF Offset */
#define UCTXBUF_M                                EUSCI_A_TXBUF_TXBUF_MASK        /*!< Transmit data buffer */
/* UCA0TXBUF_SPI[UCTXBUF] Bits */
//#define UCTXBUF_OFS                              EUSCI_A_TXBUF_TXBUF_OFS         /*!< UCTXBUF Offset */
//#define UCTXBUF_M                                EUSCI_A_TXBUF_TXBUF_MASK        /*!< Transmit data buffer */
/* UCA0ABCTL[UCABDEN] Bits */
#define UCABDEN_OFS                              EUSCI_A_ABCTL_ABDEN_OFS         /*!< UCABDEN Offset */
#define UCABDEN                                  EUSCI_A_ABCTL_ABDEN             /*!< Automatic baud-rate detect enable */
/* UCA0ABCTL[UCBTOE] Bits */
#define UCBTOE_OFS                               EUSCI_A_ABCTL_BTOE_OFS          /*!< UCBTOE Offset */
#define UCBTOE                                   EUSCI_A_ABCTL_BTOE              /*!< Break time out error */
/* UCA0ABCTL[UCSTOE] Bits */
#define UCSTOE_OFS                               EUSCI_A_ABCTL_STOE_OFS          /*!< UCSTOE Offset */
#define UCSTOE                                   EUSCI_A_ABCTL_STOE              /*!< Synch field time out error */
/* UCA0ABCTL[UCDELIM] Bits */
#define UCDELIM_OFS                              EUSCI_A_ABCTL_DELIM_OFS         /*!< UCDELIM Offset */
#define UCDELIM_M                                EUSCI_A_ABCTL_DELIM_MASK        /*!< Break/synch delimiter length */
#define UCDELIM0                                 EUSCI_A_ABCTL_DELIM0            /*!< UCDELIM Bit 0 */
#define UCDELIM1                                 EUSCI_A_ABCTL_DELIM1            /*!< UCDELIM Bit 1 */
#define UCDELIM_0                                EUSCI_A_ABCTL_DELIM_0           /*!< 1 bit time */
#define UCDELIM_1                                EUSCI_A_ABCTL_DELIM_1           /*!< 2 bit times */
#define UCDELIM_2                                EUSCI_A_ABCTL_DELIM_2           /*!< 3 bit times */
#define UCDELIM_3                                EUSCI_A_ABCTL_DELIM_3           /*!< 4 bit times */
/* UCA0IRCTL[UCIREN] Bits */
#define UCIREN_OFS                               EUSCI_A_IRCTL_IREN_OFS          /*!< UCIREN Offset */
#define UCIREN                                   EUSCI_A_IRCTL_IREN              /*!< IrDA encoder/decoder enable */
/* UCA0IRCTL[UCIRTXCLK] Bits */
#define UCIRTXCLK_OFS                            EUSCI_A_IRCTL_IRTXCLK_OFS       /*!< UCIRTXCLK Offset */
#define UCIRTXCLK                                EUSCI_A_IRCTL_IRTXCLK           /*!< IrDA transmit pulse clock select */
/* UCA0IRCTL[UCIRTXPL] Bits */
#define UCIRTXPL_OFS                             EUSCI_A_IRCTL_IRTXPL_OFS        /*!< UCIRTXPL Offset */
#define UCIRTXPL_M                               EUSCI_A_IRCTL_IRTXPL_MASK       /*!< Transmit pulse length */
/* UCA0IRCTL[UCIRRXFE] Bits */
#define UCIRRXFE_OFS                             EUSCI_A_IRCTL_IRRXFE_OFS        /*!< UCIRRXFE Offset */
#define UCIRRXFE                                 EUSCI_A_IRCTL_IRRXFE            /*!< IrDA receive filter enabled */
/* UCA0IRCTL[UCIRRXPL] Bits */
#define UCIRRXPL_OFS                             EUSCI_A_IRCTL_IRRXPL_OFS        /*!< UCIRRXPL Offset */
#define UCIRRXPL                                 EUSCI_A_IRCTL_IRRXPL            /*!< IrDA receive input UCAxRXD polarity */
/* UCA0IRCTL[UCIRRXFL] Bits */
#define UCIRRXFL_OFS                             EUSCI_A_IRCTL_IRRXFL_OFS        /*!< UCIRRXFL Offset */
#define UCIRRXFL_M                               EUSCI_A_IRCTL_IRRXFL_MASK       /*!< Receive filter length */
/* UCA0IE[UCRXIE] Bits */
#define UCRXIE_OFS                               EUSCI_A_IE_RXIE_OFS             /*!< UCRXIE Offset */
#define UCRXIE                                   EUSCI_A_IE_RXIE                 /*!< Receive interrupt enable */
/* UCA0IE[UCTXIE] Bits */
#define UCTXIE_OFS                               EUSCI_A_IE_TXIE_OFS             /*!< UCTXIE Offset */
#define UCTXIE                                   EUSCI_A_IE_TXIE                 /*!< Transmit interrupt enable */
/* UCA0IE[UCSTTIE] Bits */
#define UCSTTIE_OFS                              EUSCI_A_IE_STTIE_OFS            /*!< UCSTTIE Offset */
#define UCSTTIE                                  EUSCI_A_IE_STTIE                /*!< Start bit interrupt enable */
/* UCA0IE[UCTXCPTIE] Bits */
#define UCTXCPTIE_OFS                            EUSCI_A_IE_TXCPTIE_OFS          /*!< UCTXCPTIE Offset */
#define UCTXCPTIE                                EUSCI_A_IE_TXCPTIE              /*!< Transmit complete interrupt enable */
/* UCA0IE_SPI[UCRXIE] Bits */
//#define UCRXIE_OFS                               EUSCI_A_IE_RXIE_OFS             /*!< UCRXIE Offset */
//#define UCRXIE                                   EUSCI_A_IE_RXIE                 /*!< Receive interrupt enable */
/* UCA0IE_SPI[UCTXIE] Bits */
//#define UCTXIE_OFS                               EUSCI_A_IE_TXIE_OFS             /*!< UCTXIE Offset */
//#define UCTXIE                                   EUSCI_A_IE_TXIE                 /*!< Transmit interrupt enable */
/* UCA0IFG[UCRXIFG] Bits */
#define UCRXIFG_OFS                              EUSCI_A_IFG_RXIFG_OFS           /*!< UCRXIFG Offset */
#define UCRXIFG                                  EUSCI_A_IFG_RXIFG               /*!< Receive interrupt flag */
/* UCA0IFG[UCTXIFG] Bits */
#define UCTXIFG_OFS                              EUSCI_A_IFG_TXIFG_OFS           /*!< UCTXIFG Offset */
#define UCTXIFG                                  EUSCI_A_IFG_TXIFG               /*!< Transmit interrupt flag */
/* UCA0IFG[UCSTTIFG] Bits */
#define UCSTTIFG_OFS                             EUSCI_A_IFG_STTIFG_OFS          /*!< UCSTTIFG Offset */
#define UCSTTIFG                                 EUSCI_A_IFG_STTIFG              /*!< Start bit interrupt flag */
/* UCA0IFG[UCTXCPTIFG] Bits */
#define UCTXCPTIFG_OFS                           EUSCI_A_IFG_TXCPTIFG_OFS        /*!< UCTXCPTIFG Offset */
#define UCTXCPTIFG                               EUSCI_A_IFG_TXCPTIFG            /*!< Transmit ready interrupt enable */
/* UCA0IFG_SPI[UCRXIFG] Bits */
//#define UCRXIFG_OFS                              EUSCI_A_IFG_RXIFG_OFS           /*!< UCRXIFG Offset */
//#define UCRXIFG                                  EUSCI_A_IFG_RXIFG               /*!< Receive interrupt flag */
/* UCA0IFG_SPI[UCTXIFG] Bits */
//#define UCTXIFG_OFS                              EUSCI_A_IFG_TXIFG_OFS           /*!< UCTXIFG Offset */
//#define UCTXIFG                                  EUSCI_A_IFG_TXIFG               /*!< Transmit interrupt flag */

/******************************************************************************
* EUSCI_B Bits (legacy section)
******************************************************************************/
/* UCB0CTLW0[UCSWRST] Bits */
//#define UCSWRST_OFS                              EUSCI_B_CTLW0_SWRST_OFS         /*!< UCSWRST Offset */
//#define UCSWRST                                  EUSCI_B_CTLW0_SWRST             /*!< Software reset enable */
/* UCB0CTLW0[UCTXSTT] Bits */
#define UCTXSTT_OFS                              EUSCI_B_CTLW0_TXSTT_OFS         /*!< UCTXSTT Offset */
#define UCTXSTT                                  EUSCI_B_CTLW0_TXSTT             /*!< Transmit START condition in master mode */
/* UCB0CTLW0[UCTXSTP] Bits */
#define UCTXSTP_OFS                              EUSCI_B_CTLW0_TXSTP_OFS         /*!< UCTXSTP Offset */
#define UCTXSTP                                  EUSCI_B_CTLW0_TXSTP             /*!< Transmit STOP condition in master mode */
/* UCB0CTLW0[UCTXNACK] Bits */
#define UCTXNACK_OFS                             EUSCI_B_CTLW0_TXNACK_OFS        /*!< UCTXNACK Offset */
#define UCTXNACK                                 EUSCI_B_CTLW0_TXNACK            /*!< Transmit a NACK */
/* UCB0CTLW0[UCTR] Bits */
#define UCTR_OFS                                 EUSCI_B_CTLW0_TR_OFS            /*!< UCTR Offset */
#define UCTR                                     EUSCI_B_CTLW0_TR                /*!< Transmitter/receiver */
/* UCB0CTLW0[UCTXACK] Bits */
#define UCTXACK_OFS                              EUSCI_B_CTLW0_TXACK_OFS         /*!< UCTXACK Offset */
#define UCTXACK                                  EUSCI_B_CTLW0_TXACK             /*!< Transmit ACK condition in slave mode */
/* UCB0CTLW0[UCSSEL] Bits */
//#define UCSSEL_OFS                               EUSCI_B_CTLW0_SSEL_OFS          /*!< UCSSEL Offset */
//#define UCSSEL_M                                 EUSCI_B_CTLW0_SSEL_MASK         /*!< eUSCI_B clock source select */
//#define UCSSEL0                                  EUSCI_B_CTLW0_SSEL0             /*!< UCSSEL Bit 0 */
//#define UCSSEL1                                  EUSCI_B_CTLW0_SSEL1             /*!< UCSSEL Bit 1 */
//#define UCSSEL_0                                 EUSCI_B_CTLW0_UCSSEL_0          /*!< UCLKI */
//#define UCSSEL_1                                 EUSCI_B_CTLW0_UCSSEL_1          /*!< ACLK */
//#define UCSSEL_2                                 EUSCI_B_CTLW0_UCSSEL_2          /*!< SMCLK */
#define UCSSEL_3                                 EUSCI_B_CTLW0_UCSSEL_3          /*!< SMCLK */
#define UCSSEL__UCLKI                            EUSCI_B_CTLW0_SSEL__UCLKI       /*!< UCLKI */
//#define UCSSEL__ACLK                             EUSCI_B_CTLW0_SSEL__ACLK        /*!< ACLK */
//#define UCSSEL__SMCLK                            EUSCI_B_CTLW0_SSEL__SMCLK       /*!< SMCLK */
/* UCB0CTLW0[UCSYNC] Bits */
//#define UCSYNC_OFS                               EUSCI_B_CTLW0_SYNC_OFS          /*!< UCSYNC Offset */
//#define UCSYNC                                   EUSCI_B_CTLW0_SYNC              /*!< Synchronous mode enable */
/* UCB0CTLW0[UCMODE] Bits */
//#define UCMODE_OFS                               EUSCI_B_CTLW0_MODE_OFS          /*!< UCMODE Offset */
//#define UCMODE_M                                 EUSCI_B_CTLW0_MODE_MASK         /*!< eUSCI_B mode */
//#define UCMODE0                                  EUSCI_B_CTLW0_MODE0             /*!< UCMODE Bit 0 */
//#define UCMODE1                                  EUSCI_B_CTLW0_MODE1             /*!< UCMODE Bit 1 */
//#define UCMODE_0                                 EUSCI_B_CTLW0_MODE_0            /*!< 3-pin SPI */
//#define UCMODE_1                                 EUSCI_B_CTLW0_MODE_1            /*!< 4-pin SPI (master or slave enabled if STE = 1) */
//#define UCMODE_2                                 EUSCI_B_CTLW0_MODE_2            /*!< 4-pin SPI (master or slave enabled if STE = 0) */
//#define UCMODE_3                                 EUSCI_B_CTLW0_MODE_3            /*!< I2C mode */
/* UCB0CTLW0[UCMST] Bits */
//#define UCMST_OFS                                EUSCI_B_CTLW0_MST_OFS           /*!< UCMST Offset */
//#define UCMST                                    EUSCI_B_CTLW0_MST               /*!< Master mode select */
/* UCB0CTLW0[UCMM] Bits */
#define UCMM_OFS                                 EUSCI_B_CTLW0_MM_OFS            /*!< UCMM Offset */
#define UCMM                                     EUSCI_B_CTLW0_MM                /*!< Multi-master environment select */
/* UCB0CTLW0[UCSLA10] Bits */
#define UCSLA10_OFS                              EUSCI_B_CTLW0_SLA10_OFS         /*!< UCSLA10 Offset */
#define UCSLA10                                  EUSCI_B_CTLW0_SLA10             /*!< Slave addressing mode select */
/* UCB0CTLW0[UCA10] Bits */
#define UCA10_OFS                                EUSCI_B_CTLW0_A10_OFS           /*!< UCA10 Offset */
#define UCA10                                    EUSCI_B_CTLW0_A10               /*!< Own addressing mode select */
/* UCB0CTLW0_SPI[UCSWRST] Bits */
//#define UCSWRST_OFS                              EUSCI_B_CTLW0_SWRST_OFS         /*!< UCSWRST Offset */
//#define UCSWRST                                  EUSCI_B_CTLW0_SWRST             /*!< Software reset enable */
/* UCB0CTLW0_SPI[UCSTEM] Bits */
//#define UCSTEM_OFS                               EUSCI_B_CTLW0_STEM_OFS          /*!< UCSTEM Offset */
//#define UCSTEM                                   EUSCI_B_CTLW0_STEM              /*!< STE mode select in master mode. */
/* UCB0CTLW0_SPI[UCSSEL] Bits */
//#define UCSSEL_OFS                               EUSCI_B_CTLW0_SSEL_OFS          /*!< UCSSEL Offset */
//#define UCSSEL_M                                 EUSCI_B_CTLW0_SSEL_MASK         /*!< eUSCI_B clock source select */
//#define UCSSEL0                                  EUSCI_B_CTLW0_SSEL0             /*!< UCSSEL Bit 0 */
//#define UCSSEL1                                  EUSCI_B_CTLW0_SSEL1             /*!< UCSSEL Bit 1 */
//#define UCSSEL_0                                 EUSCI_B_CTLW0_UCSSEL_0          /*!< Reserved */
//#define UCSSEL_1                                 EUSCI_B_CTLW0_UCSSEL_1          /*!< ACLK */
//#define UCSSEL_2                                 EUSCI_B_CTLW0_UCSSEL_2          /*!< SMCLK */
//#define UCSSEL_3                                 EUSCI_B_CTLW0_UCSSEL_3          /*!< SMCLK */
//#define UCSSEL__ACLK                             EUSCI_B_CTLW0_SSEL__ACLK        /*!< ACLK */
//#define UCSSEL__SMCLK                            EUSCI_B_CTLW0_SSEL__SMCLK       /*!< SMCLK */
/* UCB0CTLW0_SPI[UCSYNC] Bits */
//#define UCSYNC_OFS                               EUSCI_B_CTLW0_SYNC_OFS          /*!< UCSYNC Offset */
//#define UCSYNC                                   EUSCI_B_CTLW0_SYNC              /*!< Synchronous mode enable */
/* UCB0CTLW0_SPI[UCMODE] Bits */
//#define UCMODE_OFS                               EUSCI_B_CTLW0_MODE_OFS          /*!< UCMODE Offset */
//#define UCMODE_M                                 EUSCI_B_CTLW0_MODE_MASK         /*!< eUSCI mode */
//#define UCMODE0                                  EUSCI_B_CTLW0_MODE0             /*!< UCMODE Bit 0 */
//#define UCMODE1                                  EUSCI_B_CTLW0_MODE1             /*!< UCMODE Bit 1 */
//#define UCMODE_0                                 EUSCI_B_CTLW0_MODE_0            /*!< 3-pin SPI */
//#define UCMODE_1                                 EUSCI_B_CTLW0_MODE_1            /*!< 4-pin SPI with UCxSTE active high: Slave enabled when UCxSTE = 1 */
//#define UCMODE_2                                 EUSCI_B_CTLW0_MODE_2            /*!< 4-pin SPI with UCxSTE active low: Slave enabled when UCxSTE = 0 */
//#define UCMODE_3                                 EUSCI_B_CTLW0_MODE_3            /*!< I2C mode */
/* UCB0CTLW0_SPI[UCMST] Bits */
//#define UCMST_OFS                                EUSCI_B_CTLW0_MST_OFS           /*!< UCMST Offset */
//#define UCMST                                    EUSCI_B_CTLW0_MST               /*!< Master mode select */
/* UCB0CTLW0_SPI[UC7BIT] Bits */
//#define UC7BIT_OFS                               EUSCI_B_CTLW0_SEVENBIT_OFS      /*!< UC7BIT Offset */
//#define UC7BIT                                   EUSCI_B_CTLW0_SEVENBIT          /*!< Character length */
/* UCB0CTLW0_SPI[UCMSB] Bits */
//#define UCMSB_OFS                                EUSCI_B_CTLW0_MSB_OFS           /*!< UCMSB Offset */
//#define UCMSB                                    EUSCI_B_CTLW0_MSB               /*!< MSB first select */
/* UCB0CTLW0_SPI[UCCKPL] Bits */
//#define UCCKPL_OFS                               EUSCI_B_CTLW0_CKPL_OFS          /*!< UCCKPL Offset */
//#define UCCKPL                                   EUSCI_B_CTLW0_CKPL              /*!< Clock polarity select */
/* UCB0CTLW0_SPI[UCCKPH] Bits */
//#define UCCKPH_OFS                               EUSCI_B_CTLW0_CKPH_OFS          /*!< UCCKPH Offset */
//#define UCCKPH                                   EUSCI_B_CTLW0_CKPH              /*!< Clock phase select */
/* UCB0CTLW1[UCGLIT] Bits */
//#define UCGLIT_OFS                               EUSCI_B_CTLW1_GLIT_OFS          /*!< UCGLIT Offset */
//#define UCGLIT_M                                 EUSCI_B_CTLW1_GLIT_MASK         /*!< Deglitch time */
//#define UCGLIT0                                  EUSCI_B_CTLW1_GLIT0             /*!< UCGLIT Bit 0 */
//#define UCGLIT1                                  EUSCI_B_CTLW1_GLIT1             /*!< UCGLIT Bit 1 */
//#define UCGLIT_0                                 EUSCI_B_CTLW1_GLIT_0            /*!< 50 ns */
//#define UCGLIT_1                                 EUSCI_B_CTLW1_GLIT_1            /*!< 25 ns */
//#define UCGLIT_2                                 EUSCI_B_CTLW1_GLIT_2            /*!< 12.5 ns */
//#define UCGLIT_3                                 EUSCI_B_CTLW1_GLIT_3            /*!< 6.25 ns */
/* UCB0CTLW1[UCASTP] Bits */
#define UCASTP_OFS                               EUSCI_B_CTLW1_ASTP_OFS          /*!< UCASTP Offset */
#define UCASTP_M                                 EUSCI_B_CTLW1_ASTP_MASK         /*!< Automatic STOP condition generation */
#define UCASTP0                                  EUSCI_B_CTLW1_ASTP0             /*!< UCASTP Bit 0 */
#define UCASTP1                                  EUSCI_B_CTLW1_ASTP1             /*!< UCASTP Bit 1 */
#define UCASTP_0                                 EUSCI_B_CTLW1_ASTP_0            /*!< No automatic STOP generation. The STOP condition is generated after the user  */
                                                                                 /* sets the UCTXSTP bit. The value in UCBxTBCNT is a don't care. */
#define UCASTP_1                                 EUSCI_B_CTLW1_ASTP_1            /*!< UCBCNTIFG is set with the byte counter reaches the threshold defined in  */
                                                                                 /* UCBxTBCNT */
#define UCASTP_2                                 EUSCI_B_CTLW1_ASTP_2            /*!< A STOP condition is generated automatically after the byte counter value  */
                                                                                 /* reached UCBxTBCNT. UCBCNTIFG is set with the byte counter reaching the */
                                                                                 /* threshold */
/* UCB0CTLW1[UCSWACK] Bits */
#define UCSWACK_OFS                              EUSCI_B_CTLW1_SWACK_OFS         /*!< UCSWACK Offset */
#define UCSWACK                                  EUSCI_B_CTLW1_SWACK             /*!< SW or HW ACK control */
/* UCB0CTLW1[UCSTPNACK] Bits */
#define UCSTPNACK_OFS                            EUSCI_B_CTLW1_STPNACK_OFS       /*!< UCSTPNACK Offset */
#define UCSTPNACK                                EUSCI_B_CTLW1_STPNACK           /*!< ACK all master bytes */
/* UCB0CTLW1[UCCLTO] Bits */
#define UCCLTO_OFS                               EUSCI_B_CTLW1_CLTO_OFS          /*!< UCCLTO Offset */
#define UCCLTO_M                                 EUSCI_B_CTLW1_CLTO_MASK         /*!< Clock low timeout select */
#define UCCLTO0                                  EUSCI_B_CTLW1_CLTO0             /*!< UCCLTO Bit 0 */
#define UCCLTO1                                  EUSCI_B_CTLW1_CLTO1             /*!< UCCLTO Bit 1 */
#define UCCLTO_0                                 EUSCI_B_CTLW1_CLTO_0            /*!< Disable clock low timeout counter */
#define UCCLTO_1                                 EUSCI_B_CTLW1_CLTO_1            /*!< 135 000 SYSCLK cycles (approximately 28 ms) */
#define UCCLTO_2                                 EUSCI_B_CTLW1_CLTO_2            /*!< 150 000 SYSCLK cycles (approximately 31 ms) */
#define UCCLTO_3                                 EUSCI_B_CTLW1_CLTO_3            /*!< 165 000 SYSCLK cycles (approximately 34 ms) */
/* UCB0CTLW1[UCETXINT] Bits */
#define UCETXINT_OFS                             EUSCI_B_CTLW1_ETXINT_OFS        /*!< UCETXINT Offset */
#define UCETXINT                                 EUSCI_B_CTLW1_ETXINT            /*!< Early UCTXIFG0 */
/* UCB0STATW[UCBBUSY] Bits */
#define UCBBUSY_OFS                              EUSCI_B_STATW_BBUSY_OFS         /*!< UCBBUSY Offset */
#define UCBBUSY                                  EUSCI_B_STATW_BBUSY             /*!< Bus busy */
/* UCB0STATW[UCGC] Bits */
#define UCGC_OFS                                 EUSCI_B_STATW_GC_OFS            /*!< UCGC Offset */
#define UCGC                                     EUSCI_B_STATW_GC                /*!< General call address received */
/* UCB0STATW[UCSCLLOW] Bits */
#define UCSCLLOW_OFS                             EUSCI_B_STATW_SCLLOW_OFS        /*!< UCSCLLOW Offset */
#define UCSCLLOW                                 EUSCI_B_STATW_SCLLOW            /*!< SCL low */
/* UCB0STATW[UCBCNT] Bits */
#define UCBCNT_OFS                               EUSCI_B_STATW_BCNT_OFS          /*!< UCBCNT Offset */
#define UCBCNT_M                                 EUSCI_B_STATW_BCNT_MASK         /*!< Hardware byte counter value */
/* UCB0STATW_SPI[UCBUSY] Bits */
//#define UCBUSY_OFS                               EUSCI_B_STATW_SPI_BUSY_OFS      /*!< UCBUSY Offset */
//#define UCBUSY                                   EUSCI_B_STATW_SPI_BUSY          /*!< eUSCI_B busy */
/* UCB0STATW_SPI[UCOE] Bits */
//#define UCOE_OFS                                 EUSCI_B_STATW_OE_OFS            /*!< UCOE Offset */
//#define UCOE                                     EUSCI_B_STATW_OE                /*!< Overrun error flag */
/* UCB0STATW_SPI[UCFE] Bits */
//#define UCFE_OFS                                 EUSCI_B_STATW_FE_OFS            /*!< UCFE Offset */
//#define UCFE                                     EUSCI_B_STATW_FE                /*!< Framing error flag */
/* UCB0STATW_SPI[UCLISTEN] Bits */
//#define UCLISTEN_OFS                             EUSCI_B_STATW_LISTEN_OFS        /*!< UCLISTEN Offset */
//#define UCLISTEN                                 EUSCI_B_STATW_LISTEN            /*!< Listen enable */
/* UCB0TBCNT[UCTBCNT] Bits */
#define UCTBCNT_OFS                              EUSCI_B_TBCNT_TBCNT_OFS         /*!< UCTBCNT Offset */
#define UCTBCNT_M                                EUSCI_B_TBCNT_TBCNT_MASK        /*!< Byte counter threshold value */
/* UCB0RXBUF[UCRXBUF] Bits */
//#define UCRXBUF_OFS                              EUSCI_B_RXBUF_RXBUF_OFS         /*!< UCRXBUF Offset */
//#define UCRXBUF_M                                EUSCI_B_RXBUF_RXBUF_MASK        /*!< Receive data buffer */
/* UCB0RXBUF_SPI[UCRXBUF] Bits */
//#define UCRXBUF_OFS                              EUSCI_B_RXBUF_RXBUF_OFS         /*!< UCRXBUF Offset */
//#define UCRXBUF_M                                EUSCI_B_RXBUF_RXBUF_MASK        /*!< Receive data buffer */
/* UCB0TXBUF[UCTXBUF] Bits */
//#define UCTXBUF_OFS                              EUSCI_B_TXBUF_TXBUF_OFS         /*!< UCTXBUF Offset */
//#define UCTXBUF_M                                EUSCI_B_TXBUF_TXBUF_MASK        /*!< Transmit data buffer */
/* UCB0TXBUF_SPI[UCTXBUF] Bits */
//#define UCTXBUF_OFS                              EUSCI_B_TXBUF_TXBUF_OFS         /*!< UCTXBUF Offset */
//#define UCTXBUF_M                                EUSCI_B_TXBUF_TXBUF_MASK        /*!< Transmit data buffer */
/* UCB0I2COA0[I2COA0] Bits */
#define I2COA0_OFS                               EUSCI_B_I2COA0_I2COA0_OFS       /*!< I2COA0 Offset */
#define I2COA0_M                                 EUSCI_B_I2COA0_I2COA0_MASK      /*!< I2C own address */
/* UCB0I2COA0[UCOAEN] Bits */
#define UCOAEN_OFS                               EUSCI_B_I2COA0_OAEN_OFS         /*!< UCOAEN Offset */
#define UCOAEN                                   EUSCI_B_I2COA0_OAEN             /*!< Own Address enable register */
/* UCB0I2COA0[UCGCEN] Bits */
#define UCGCEN_OFS                               EUSCI_B_I2COA0_GCEN_OFS         /*!< UCGCEN Offset */
#define UCGCEN                                   EUSCI_B_I2COA0_GCEN             /*!< General call response enable */
/* UCB0I2COA1[I2COA1] Bits */
#define I2COA1_OFS                               EUSCI_B_I2COA1_I2COA1_OFS       /*!< I2COA1 Offset */
#define I2COA1_M                                 EUSCI_B_I2COA1_I2COA1_MASK      /*!< I2C own address */
/* UCB0I2COA1[UCOAEN] Bits */
//#define UCOAEN_OFS                               EUSCI_B_I2COA1_OAEN_OFS         /*!< UCOAEN Offset */
//#define UCOAEN                                   EUSCI_B_I2COA1_OAEN             /*!< Own Address enable register */
/* UCB0I2COA2[I2COA2] Bits */
#define I2COA2_OFS                               EUSCI_B_I2COA2_I2COA2_OFS       /*!< I2COA2 Offset */
#define I2COA2_M                                 EUSCI_B_I2COA2_I2COA2_MASK      /*!< I2C own address */
/* UCB0I2COA2[UCOAEN] Bits */
//#define UCOAEN_OFS                               EUSCI_B_I2COA2_OAEN_OFS         /*!< UCOAEN Offset */
//#define UCOAEN                                   EUSCI_B_I2COA2_OAEN             /*!< Own Address enable register */
/* UCB0I2COA3[I2COA3] Bits */
#define I2COA3_OFS                               EUSCI_B_I2COA3_I2COA3_OFS       /*!< I2COA3 Offset */
#define I2COA3_M                                 EUSCI_B_I2COA3_I2COA3_MASK      /*!< I2C own address */
/* UCB0I2COA3[UCOAEN] Bits */
//#define UCOAEN_OFS                               EUSCI_B_I2COA3_OAEN_OFS         /*!< UCOAEN Offset */
//#define UCOAEN                                   EUSCI_B_I2COA3_OAEN             /*!< Own Address enable register */
/* UCB0ADDRX[ADDRX] Bits */
#define ADDRX_OFS                                EUSCI_B_ADDRX_ADDRX_OFS         /*!< ADDRX Offset */
#define ADDRX_M                                  EUSCI_B_ADDRX_ADDRX_MASK        /*!< Received Address Register */
#define ADDRX0                                   EUSCI_B_ADDRX_ADDRX0            /*!< ADDRX Bit 0 */
#define ADDRX1                                   EUSCI_B_ADDRX_ADDRX1            /*!< ADDRX Bit 1 */
#define ADDRX2                                   EUSCI_B_ADDRX_ADDRX2            /*!< ADDRX Bit 2 */
#define ADDRX3                                   EUSCI_B_ADDRX_ADDRX3            /*!< ADDRX Bit 3 */
#define ADDRX4                                   EUSCI_B_ADDRX_ADDRX4            /*!< ADDRX Bit 4 */
#define ADDRX5                                   EUSCI_B_ADDRX_ADDRX5            /*!< ADDRX Bit 5 */
#define ADDRX6                                   EUSCI_B_ADDRX_ADDRX6            /*!< ADDRX Bit 6 */
#define ADDRX7                                   EUSCI_B_ADDRX_ADDRX7            /*!< ADDRX Bit 7 */
#define ADDRX8                                   EUSCI_B_ADDRX_ADDRX8            /*!< ADDRX Bit 8 */
#define ADDRX9                                   EUSCI_B_ADDRX_ADDRX9            /*!< ADDRX Bit 9 */
/* UCB0ADDMASK[ADDMASK] Bits */
#define ADDMASK_OFS                              EUSCI_B_ADDMASK_ADDMASK_OFS     /*!< ADDMASK Offset */
#define ADDMASK_M                                EUSCI_B_ADDMASK_ADDMASK_MASK    
/* UCB0I2CSA[I2CSA] Bits */
#define I2CSA_OFS                                EUSCI_B_I2CSA_I2CSA_OFS         /*!< I2CSA Offset */
#define I2CSA_M                                  EUSCI_B_I2CSA_I2CSA_MASK        /*!< I2C slave address */
/* UCB0IE[UCRXIE0] Bits */
#define UCRXIE0_OFS                              EUSCI_B_IE_RXIE0_OFS            /*!< UCRXIE0 Offset */
#define UCRXIE0                                  EUSCI_B_IE_RXIE0                /*!< Receive interrupt enable 0 */
/* UCB0IE[UCTXIE0] Bits */
#define UCTXIE0_OFS                              EUSCI_B_IE_TXIE0_OFS            /*!< UCTXIE0 Offset */
#define UCTXIE0                                  EUSCI_B_IE_TXIE0                /*!< Transmit interrupt enable 0 */
/* UCB0IE[UCSTTIE] Bits */
//#define UCSTTIE_OFS                              EUSCI_B_IE_STTIE_OFS            /*!< UCSTTIE Offset */
//#define UCSTTIE                                  EUSCI_B_IE_STTIE                /*!< START condition interrupt enable */
/* UCB0IE[UCSTPIE] Bits */
#define UCSTPIE_OFS                              EUSCI_B_IE_STPIE_OFS            /*!< UCSTPIE Offset */
#define UCSTPIE                                  EUSCI_B_IE_STPIE                /*!< STOP condition interrupt enable */
/* UCB0IE[UCALIE] Bits */
#define UCALIE_OFS                               EUSCI_B_IE_ALIE_OFS             /*!< UCALIE Offset */
#define UCALIE                                   EUSCI_B_IE_ALIE                 /*!< Arbitration lost interrupt enable */
/* UCB0IE[UCNACKIE] Bits */
#define UCNACKIE_OFS                             EUSCI_B_IE_NACKIE_OFS           /*!< UCNACKIE Offset */
#define UCNACKIE                                 EUSCI_B_IE_NACKIE               /*!< Not-acknowledge interrupt enable */
/* UCB0IE[UCBCNTIE] Bits */
#define UCBCNTIE_OFS                             EUSCI_B_IE_BCNTIE_OFS           /*!< UCBCNTIE Offset */
#define UCBCNTIE                                 EUSCI_B_IE_BCNTIE               /*!< Byte counter interrupt enable */
/* UCB0IE[UCCLTOIE] Bits */
#define UCCLTOIE_OFS                             EUSCI_B_IE_CLTOIE_OFS           /*!< UCCLTOIE Offset */
#define UCCLTOIE                                 EUSCI_B_IE_CLTOIE               /*!< Clock low timeout interrupt enable */
/* UCB0IE[UCRXIE1] Bits */
#define UCRXIE1_OFS                              EUSCI_B_IE_RXIE1_OFS            /*!< UCRXIE1 Offset */
#define UCRXIE1                                  EUSCI_B_IE_RXIE1                /*!< Receive interrupt enable 1 */
/* UCB0IE[UCTXIE1] Bits */
#define UCTXIE1_OFS                              EUSCI_B_IE_TXIE1_OFS            /*!< UCTXIE1 Offset */
#define UCTXIE1                                  EUSCI_B_IE_TXIE1                /*!< Transmit interrupt enable 1 */
/* UCB0IE[UCRXIE2] Bits */
#define UCRXIE2_OFS                              EUSCI_B_IE_RXIE2_OFS            /*!< UCRXIE2 Offset */
#define UCRXIE2                                  EUSCI_B_IE_RXIE2                /*!< Receive interrupt enable 2 */
/* UCB0IE[UCTXIE2] Bits */
#define UCTXIE2_OFS                              EUSCI_B_IE_TXIE2_OFS            /*!< UCTXIE2 Offset */
#define UCTXIE2                                  EUSCI_B_IE_TXIE2                /*!< Transmit interrupt enable 2 */
/* UCB0IE[UCRXIE3] Bits */
#define UCRXIE3_OFS                              EUSCI_B_IE_RXIE3_OFS            /*!< UCRXIE3 Offset */
#define UCRXIE3                                  EUSCI_B_IE_RXIE3                /*!< Receive interrupt enable 3 */
/* UCB0IE[UCTXIE3] Bits */
#define UCTXIE3_OFS                              EUSCI_B_IE_TXIE3_OFS            /*!< UCTXIE3 Offset */
#define UCTXIE3                                  EUSCI_B_IE_TXIE3                /*!< Transmit interrupt enable 3 */
/* UCB0IE[UCBIT9IE] Bits */
#define UCBIT9IE_OFS                             EUSCI_B_IE_BIT9IE_OFS           /*!< UCBIT9IE Offset */
#define UCBIT9IE                                 EUSCI_B_IE_BIT9IE               /*!< Bit position 9 interrupt enable */
/* UCB0IE_SPI[UCRXIE] Bits */
//#define UCRXIE_OFS                               EUSCI_B_IE_RXIE_OFS             /*!< UCRXIE Offset */
//#define UCRXIE                                   EUSCI_B_IE_RXIE                 /*!< Receive interrupt enable */
/* UCB0IE_SPI[UCTXIE] Bits */
//#define UCTXIE_OFS                               EUSCI_B_IE_TXIE_OFS             /*!< UCTXIE Offset */
//#define UCTXIE                                   EUSCI_B_IE_TXIE                 /*!< Transmit interrupt enable */
/* UCB0IFG[UCRXIFG0] Bits */
#define UCRXIFG0_OFS                             EUSCI_B_IFG_RXIFG0_OFS          /*!< UCRXIFG0 Offset */
#define UCRXIFG0                                 EUSCI_B_IFG_RXIFG0              /*!< eUSCI_B receive interrupt flag 0 */
/* UCB0IFG[UCTXIFG0] Bits */
#define UCTXIFG0_OFS                             EUSCI_B_IFG_TXIFG0_OFS          /*!< UCTXIFG0 Offset */
#define UCTXIFG0                                 EUSCI_B_IFG_TXIFG0              /*!< eUSCI_B transmit interrupt flag 0 */
/* UCB0IFG[UCSTTIFG] Bits */
//#define UCSTTIFG_OFS                             EUSCI_B_IFG_STTIFG_OFS          /*!< UCSTTIFG Offset */
//#define UCSTTIFG                                 EUSCI_B_IFG_STTIFG              /*!< START condition interrupt flag */
/* UCB0IFG[UCSTPIFG] Bits */
#define UCSTPIFG_OFS                             EUSCI_B_IFG_STPIFG_OFS          /*!< UCSTPIFG Offset */
#define UCSTPIFG                                 EUSCI_B_IFG_STPIFG              /*!< STOP condition interrupt flag */
/* UCB0IFG[UCALIFG] Bits */
#define UCALIFG_OFS                              EUSCI_B_IFG_ALIFG_OFS           /*!< UCALIFG Offset */
#define UCALIFG                                  EUSCI_B_IFG_ALIFG               /*!< Arbitration lost interrupt flag */
/* UCB0IFG[UCNACKIFG] Bits */
#define UCNACKIFG_OFS                            EUSCI_B_IFG_NACKIFG_OFS         /*!< UCNACKIFG Offset */
#define UCNACKIFG                                EUSCI_B_IFG_NACKIFG             /*!< Not-acknowledge received interrupt flag */
/* UCB0IFG[UCBCNTIFG] Bits */
#define UCBCNTIFG_OFS                            EUSCI_B_IFG_BCNTIFG_OFS         /*!< UCBCNTIFG Offset */
#define UCBCNTIFG                                EUSCI_B_IFG_BCNTIFG             /*!< Byte counter interrupt flag */
/* UCB0IFG[UCCLTOIFG] Bits */
#define UCCLTOIFG_OFS                            EUSCI_B_IFG_CLTOIFG_OFS         /*!< UCCLTOIFG Offset */
#define UCCLTOIFG                                EUSCI_B_IFG_CLTOIFG             /*!< Clock low timeout interrupt flag */
/* UCB0IFG[UCRXIFG1] Bits */
#define UCRXIFG1_OFS                             EUSCI_B_IFG_RXIFG1_OFS          /*!< UCRXIFG1 Offset */
#define UCRXIFG1                                 EUSCI_B_IFG_RXIFG1              /*!< eUSCI_B receive interrupt flag 1 */
/* UCB0IFG[UCTXIFG1] Bits */
#define UCTXIFG1_OFS                             EUSCI_B_IFG_TXIFG1_OFS          /*!< UCTXIFG1 Offset */
#define UCTXIFG1                                 EUSCI_B_IFG_TXIFG1              /*!< eUSCI_B transmit interrupt flag 1 */
/* UCB0IFG[UCRXIFG2] Bits */
#define UCRXIFG2_OFS                             EUSCI_B_IFG_RXIFG2_OFS          /*!< UCRXIFG2 Offset */
#define UCRXIFG2                                 EUSCI_B_IFG_RXIFG2              /*!< eUSCI_B receive interrupt flag 2 */
/* UCB0IFG[UCTXIFG2] Bits */
#define UCTXIFG2_OFS                             EUSCI_B_IFG_TXIFG2_OFS          /*!< UCTXIFG2 Offset */
#define UCTXIFG2                                 EUSCI_B_IFG_TXIFG2              /*!< eUSCI_B transmit interrupt flag 2 */
/* UCB0IFG[UCRXIFG3] Bits */
#define UCRXIFG3_OFS                             EUSCI_B_IFG_RXIFG3_OFS          /*!< UCRXIFG3 Offset */
#define UCRXIFG3                                 EUSCI_B_IFG_RXIFG3              /*!< eUSCI_B receive interrupt flag 3 */
/* UCB0IFG[UCTXIFG3] Bits */
#define UCTXIFG3_OFS                             EUSCI_B_IFG_TXIFG3_OFS          /*!< UCTXIFG3 Offset */
#define UCTXIFG3                                 EUSCI_B_IFG_TXIFG3              /*!< eUSCI_B transmit interrupt flag 3 */
/* UCB0IFG[UCBIT9IFG] Bits */
#define UCBIT9IFG_OFS                            EUSCI_B_IFG_BIT9IFG_OFS         /*!< UCBIT9IFG Offset */
#define UCBIT9IFG                                EUSCI_B_IFG_BIT9IFG             /*!< Bit position 9 interrupt flag */
/* UCB0IFG_SPI[UCRXIFG] Bits */
//#define UCRXIFG_OFS                              EUSCI_B_IFG_RXIFG_OFS           /*!< UCRXIFG Offset */
//#define UCRXIFG                                  EUSCI_B_IFG_RXIFG               /*!< Receive interrupt flag */
/* UCB0IFG_SPI[UCTXIFG] Bits */
//#define UCTXIFG_OFS                              EUSCI_B_IFG_TXIFG_OFS           /*!< UCTXIFG Offset */
//#define UCTXIFG                                  EUSCI_B_IFG_TXIFG               /*!< Transmit interrupt flag */

/******************************************************************************
* PMAP Bits (legacy section)
******************************************************************************/
/* PMAPCTL[PMAPLOCKED] Bits */
#define PMAPLOCKED_OFS                           PMAP_CTL_LOCKED_OFS             /*!< PMAPLOCKED Offset */
#define PMAPLOCKED                               PMAP_CTL_LOCKED                 /*!< Port mapping lock bit */
/* PMAPCTL[PMAPRECFG] Bits */
#define PMAPRECFG_OFS                            PMAP_CTL_PRECFG_OFS             /*!< PMAPRECFG Offset */
#define PMAPRECFG                                PMAP_CTL_PRECFG                 /*!< Port mapping reconfiguration control bit */
/* Pre-defined bitfield values */
/* PMAP_PMAPCTL[PMAPLOCKED] Bits */
#define PMAPLOCKED_OFS                                     PMAP_CTL_LOCKED_OFS   /*!< PMAPLOCKED Offset */
#define PMAPLOCKED                                         PMAP_CTL_LOCKED       /*!< Port mapping lock bit */
/* PMAP_PMAPCTL[PMAPRECFG] Bits */
#define PMAPRECFG_OFS                                      PMAP_CTL_PRECFG_OFS   /*!< PMAPRECFG Offset */
#define PMAPRECFG                                          PMAP_CTL_PRECFG       /*!< Port mapping reconfiguration control bit */

#define PM_NONE                                            PMAP_NONE
#define PM_UCA0CLK                                         PMAP_UCA0CLK
#define PM_UCA0RXD                                         PMAP_UCA0RXD
#define PM_UCA0SOMI                                        PMAP_UCA0SOMI
#define PM_UCA0TXD                                         PMAP_UCA0TXD
#define PM_UCA0SIMO                                        PMAP_UCA0SIMO
#define PM_UCB0CLK                                         PMAP_UCB0CLK
#define PM_UCB0SDA                                         PMAP_UCB0SDA
#define PM_UCB0SIMO                                        PMAP_UCB0SIMO
#define PM_UCB0SCL                                         PMAP_UCB0SCL
#define PM_UCB0SOMI                                        PMAP_UCB0SOMI
#define PM_UCA1STE                                         PMAP_UCA1STE
#define PM_UCA1CLK                                         PMAP_UCA1CLK
#define PM_UCA1RXD                                         PMAP_UCA1RXD
#define PM_UCA1SOMI                                        PMAP_UCA1SOMI
#define PM_UCA1TXD                                         PMAP_UCA1TXD
#define PM_UCA1SIMO                                        PMAP_UCA1SIMO
#define PM_UCA2STE                                         PMAP_UCA2STE
#define PM_UCA2CLK                                         PMAP_UCA2CLK
#define PM_UCA2RXD                                         PMAP_UCA2RXD
#define PM_UCA2SOMI                                        PMAP_UCA2SOMI
#define PM_UCA2TXD                                         PMAP_UCA2TXD
#define PM_UCA2SIMO                                        PMAP_UCA2SIMO
#define PM_UCB2STE                                         PMAP_UCB2STE
#define PM_UCB2CLK                                         PMAP_UCB2CLK
#define PM_UCB2SDA                                         PMAP_UCB2SDA
#define PM_UCB2SIMO                                        PMAP_UCB2SIMO
#define PM_UCB2SCL                                         PMAP_UCB2SCL
#define PM_UCB2SOMI                                        PMAP_UCB2SOMI
#define PM_TA0CCR0A                                        PMAP_TA0CCR0A
#define PM_TA0CCR1A                                        PMAP_TA0CCR1A
#define PM_TA0CCR2A                                        PMAP_TA0CCR2A
#define PM_TA0CCR3A                                        PMAP_TA0CCR3A
#define PM_TA0CCR4A                                        PMAP_TA0CCR4A
#define PM_TA1CCR1A                                        PMAP_TA1CCR1A
#define PM_TA1CCR2A                                        PMAP_TA1CCR2A
#define PM_TA1CCR3A                                        PMAP_TA1CCR3A
#define PM_TA1CCR4A                                        PMAP_TA1CCR4A
#define PM_TA0CLK                                          PMAP_TA0CLK
#define PM_CE0OUT                                          PMAP_CE0OUT
#define PM_TA1CLK                                          PMAP_TA1CLK
#define PM_CE1OUT                                          PMAP_CE1OUT
#define PM_DMAE0                                           PMAP_DMAE0
#define PM_SMCLK                                           PMAP_SMCLK
#define PM_ANALOG                                          PMAP_ANALOG

#define PMAPKEY                                            PMAP_KEYID_VAL        /*!< Port Mapping Key */
#define PMAPPWD                                            PMAP_KEYID_VAL        /*!< Legacy Definition: Mapping Key register */
#define PMAPPW                                             PMAP_KEYID_VAL        /*!< Legacy Definition: Port Mapping Password */


/******************************************************************************
* REF_A Bits (legacy section)
******************************************************************************/
/* REFCTL0[REFON] Bits */
#define REFON_OFS                                REF_A_CTL0_ON_OFS               /*!< REFON Offset */
#define REFON                                    REF_A_CTL0_ON                   /*!< Reference enable */
/* REFCTL0[REFOUT] Bits */
#define REFOUT_OFS                               REF_A_CTL0_OUT_OFS              /*!< REFOUT Offset */
#define REFOUT                                   REF_A_CTL0_OUT                  /*!< Reference output buffer */
/* REFCTL0[REFTCOFF] Bits */
#define REFTCOFF_OFS                             REF_A_CTL0_TCOFF_OFS            /*!< REFTCOFF Offset */
#define REFTCOFF                                 REF_A_CTL0_TCOFF                /*!< Temperature sensor disabled */
/* REFCTL0[REFVSEL] Bits */
#define REFVSEL_OFS                              REF_A_CTL0_VSEL_OFS             /*!< REFVSEL Offset */
#define REFVSEL_M                                REF_A_CTL0_VSEL_MASK            /*!< Reference voltage level select */
#define REFVSEL0                                 REF_A_CTL0_VSEL0                /*!< REFVSEL Bit 0 */
#define REFVSEL1                                 REF_A_CTL0_VSEL1                /*!< REFVSEL Bit 1 */
#define REFVSEL_0                                REF_A_CTL0_VSEL_0               /*!< 1.2 V available when reference requested or REFON = 1 */
#define REFVSEL_1                                REF_A_CTL0_VSEL_1               /*!< 1.45 V available when reference requested or REFON = 1 */
#define REFVSEL_3                                REF_A_CTL0_VSEL_3               /*!< 2.5 V available when reference requested or REFON = 1 */
/* REFCTL0[REFGENOT] Bits */
#define REFGENOT_OFS                             REF_A_CTL0_GENOT_OFS            /*!< REFGENOT Offset */
#define REFGENOT                                 REF_A_CTL0_GENOT                /*!< Reference generator one-time trigger */
/* REFCTL0[REFBGOT] Bits */
#define REFBGOT_OFS                              REF_A_CTL0_BGOT_OFS             /*!< REFBGOT Offset */
#define REFBGOT                                  REF_A_CTL0_BGOT                 /*!< Bandgap and bandgap buffer one-time trigger */
/* REFCTL0[REFGENACT] Bits */
#define REFGENACT_OFS                            REF_A_CTL0_GENACT_OFS           /*!< REFGENACT Offset */
#define REFGENACT                                REF_A_CTL0_GENACT               /*!< Reference generator active */
/* REFCTL0[REFBGACT] Bits */
#define REFBGACT_OFS                             REF_A_CTL0_BGACT_OFS            /*!< REFBGACT Offset */
#define REFBGACT                                 REF_A_CTL0_BGACT                /*!< Reference bandgap active */
/* REFCTL0[REFGENBUSY] Bits */
#define REFGENBUSY_OFS                           REF_A_CTL0_GENBUSY_OFS          /*!< REFGENBUSY Offset */
#define REFGENBUSY                               REF_A_CTL0_GENBUSY              /*!< Reference generator busy */
/* REFCTL0[BGMODE] Bits */
#define BGMODE_OFS                               REF_A_CTL0_BGMODE_OFS           /*!< BGMODE Offset */
#define BGMODE                                   REF_A_CTL0_BGMODE               /*!< Bandgap mode */
/* REFCTL0[REFGENRDY] Bits */
#define REFGENRDY_OFS                            REF_A_CTL0_GENRDY_OFS           /*!< REFGENRDY Offset */
#define REFGENRDY                                REF_A_CTL0_GENRDY               /*!< Variable reference voltage ready status */
/* REFCTL0[REFBGRDY] Bits */
#define REFBGRDY_OFS                             REF_A_CTL0_BGRDY_OFS            /*!< REFBGRDY Offset */
#define REFBGRDY                                 REF_A_CTL0_BGRDY                /*!< Buffered bandgap voltage ready status */

/******************************************************************************
* RTC_C Bits (legacy section)
******************************************************************************/
/* RTCCTL0[RTCRDYIFG] Bits */
#define RTCRDYIFG_OFS                            RTC_C_CTL0_RDYIFG_OFS           /*!< RTCRDYIFG Offset */
#define RTCRDYIFG                                RTC_C_CTL0_RDYIFG               /*!< Real-time clock ready interrupt flag */
/* RTCCTL0[RTCAIFG] Bits */
#define RTCAIFG_OFS                              RTC_C_CTL0_AIFG_OFS             /*!< RTCAIFG Offset */
#define RTCAIFG                                  RTC_C_CTL0_AIFG                 /*!< Real-time clock alarm interrupt flag */
/* RTCCTL0[RTCTEVIFG] Bits */
#define RTCTEVIFG_OFS                            RTC_C_CTL0_TEVIFG_OFS           /*!< RTCTEVIFG Offset */
#define RTCTEVIFG                                RTC_C_CTL0_TEVIFG               /*!< Real-time clock time event interrupt flag */
/* RTCCTL0[RTCOFIFG] Bits */
#define RTCOFIFG_OFS                             RTC_C_CTL0_OFIFG_OFS            /*!< RTCOFIFG Offset */
#define RTCOFIFG                                 RTC_C_CTL0_OFIFG                /*!< 32-kHz crystal oscillator fault interrupt flag */
/* RTCCTL0[RTCRDYIE] Bits */
#define RTCRDYIE_OFS                             RTC_C_CTL0_RDYIE_OFS            /*!< RTCRDYIE Offset */
#define RTCRDYIE                                 RTC_C_CTL0_RDYIE                /*!< Real-time clock ready interrupt enable */
/* RTCCTL0[RTCAIE] Bits */
#define RTCAIE_OFS                               RTC_C_CTL0_AIE_OFS              /*!< RTCAIE Offset */
#define RTCAIE                                   RTC_C_CTL0_AIE                  /*!< Real-time clock alarm interrupt enable */
/* RTCCTL0[RTCTEVIE] Bits */
#define RTCTEVIE_OFS                             RTC_C_CTL0_TEVIE_OFS            /*!< RTCTEVIE Offset */
#define RTCTEVIE                                 RTC_C_CTL0_TEVIE                /*!< Real-time clock time event interrupt enable */
/* RTCCTL0[RTCOFIE] Bits */
#define RTCOFIE_OFS                              RTC_C_CTL0_OFIE_OFS             /*!< RTCOFIE Offset */
#define RTCOFIE                                  RTC_C_CTL0_OFIE                 /*!< 32-kHz crystal oscillator fault interrupt enable */
/* RTCCTL0[RTCKEY] Bits */
#define RTCKEY_OFS                               RTC_C_CTL0_KEY_OFS              /*!< RTCKEY Offset */
#define RTCKEY_M                                 RTC_C_CTL0_KEY_MASK             /*!< Real-time clock key */
/* RTCCTL13[RTCTEV] Bits */
#define RTCTEV_OFS                               RTC_C_CTL13_TEV_OFS             /*!< RTCTEV Offset */
#define RTCTEV_M                                 RTC_C_CTL13_TEV_MASK            /*!< Real-time clock time event */
#define RTCTEV0                                  RTC_C_CTL13_TEV0                /*!< RTCTEV Bit 0 */
#define RTCTEV1                                  RTC_C_CTL13_TEV1                /*!< RTCTEV Bit 1 */
#define RTCTEV_0                                 RTC_C_CTL13_TEV_0               /*!< Minute changed */
#define RTCTEV_1                                 RTC_C_CTL13_TEV_1               /*!< Hour changed */
#define RTCTEV_2                                 RTC_C_CTL13_TEV_2               /*!< Every day at midnight (00:00) */
#define RTCTEV_3                                 RTC_C_CTL13_TEV_3               /*!< Every day at noon (12:00) */
/* RTCCTL13[RTCSSEL] Bits */
#define RTCSSEL_OFS                              RTC_C_CTL13_SSEL_OFS            /*!< RTCSSEL Offset */
#define RTCSSEL_M                                RTC_C_CTL13_SSEL_MASK           /*!< Real-time clock source select */
#define RTCSSEL0                                 RTC_C_CTL13_SSEL0               /*!< RTCSSEL Bit 0 */
#define RTCSSEL1                                 RTC_C_CTL13_SSEL1               /*!< RTCSSEL Bit 1 */
#define RTCSSEL_0                                RTC_C_CTL13_SSEL_0              /*!< BCLK */
#define RTCSSEL__BCLK                            RTC_C_CTL13_SSEL__BCLK          /*!< BCLK */
/* RTCCTL13[RTCRDY] Bits */
#define RTCRDY_OFS                               RTC_C_CTL13_RDY_OFS             /*!< RTCRDY Offset */
#define RTCRDY                                   RTC_C_CTL13_RDY                 /*!< Real-time clock ready */
/* RTCCTL13[RTCMODE] Bits */
#define RTCMODE_OFS                              RTC_C_CTL13_MODE_OFS            /*!< RTCMODE Offset */
#define RTCMODE                                  RTC_C_CTL13_MODE                
/* RTCCTL13[RTCHOLD] Bits */
#define RTCHOLD_OFS                              RTC_C_CTL13_HOLD_OFS            /*!< RTCHOLD Offset */
#define RTCHOLD                                  RTC_C_CTL13_HOLD                /*!< Real-time clock hold */
/* RTCCTL13[RTCBCD] Bits */
#define RTCBCD_OFS                               RTC_C_CTL13_BCD_OFS             /*!< RTCBCD Offset */
#define RTCBCD                                   RTC_C_CTL13_BCD                 /*!< Real-time clock BCD select */
/* RTCCTL13[RTCCALF] Bits */
#define RTCCALF_OFS                              RTC_C_CTL13_CALF_OFS            /*!< RTCCALF Offset */
#define RTCCALF_M                                RTC_C_CTL13_CALF_MASK           /*!< Real-time clock calibration frequency */
#define RTCCALF0                                 RTC_C_CTL13_CALF0               /*!< RTCCALF Bit 0 */
#define RTCCALF1                                 RTC_C_CTL13_CALF1               /*!< RTCCALF Bit 1 */
#define RTCCALF_0                                RTC_C_CTL13_CALF_0              /*!< No frequency output to RTCCLK pin */
#define RTCCALF_1                                RTC_C_CTL13_CALF_1              /*!< 512 Hz */
#define RTCCALF_2                                RTC_C_CTL13_CALF_2              /*!< 256 Hz */
#define RTCCALF_3                                RTC_C_CTL13_CALF_3              /*!< 1 Hz */
#define RTCCALF__NONE                            RTC_C_CTL13_CALF__NONE          /*!< No frequency output to RTCCLK pin */
#define RTCCALF__512                             RTC_C_CTL13_CALF__512           /*!< 512 Hz */
#define RTCCALF__256                             RTC_C_CTL13_CALF__256           /*!< 256 Hz */
#define RTCCALF__1                               RTC_C_CTL13_CALF__1             /*!< 1 Hz */
/* RTCOCAL[RTCOCAL] Bits */
#define RTCOCAL_OFS                              RTC_C_OCAL_OCAL_OFS             /*!< RTCOCAL Offset */
#define RTCOCAL_M                                RTC_C_OCAL_OCAL_MASK            /*!< Real-time clock offset error calibration */
/* RTCOCAL[RTCOCALS] Bits */
#define RTCOCALS_OFS                             RTC_C_OCAL_OCALS_OFS            /*!< RTCOCALS Offset */
#define RTCOCALS                                 RTC_C_OCAL_OCALS                /*!< Real-time clock offset error calibration sign */
/* RTCTCMP[RTCTCMP] Bits */
#define RTCTCMP_OFS                              RTC_C_TCMP_TCMPX_OFS            /*!< RTCTCMP Offset */
#define RTCTCMP_M                                RTC_C_TCMP_TCMPX_MASK           /*!< Real-time clock temperature compensation */
/* RTCTCMP[RTCTCOK] Bits */
#define RTCTCOK_OFS                              RTC_C_TCMP_TCOK_OFS             /*!< RTCTCOK Offset */
#define RTCTCOK                                  RTC_C_TCMP_TCOK                 /*!< Real-time clock temperature compensation write OK */
/* RTCTCMP[RTCTCRDY] Bits */
#define RTCTCRDY_OFS                             RTC_C_TCMP_TCRDY_OFS            /*!< RTCTCRDY Offset */
#define RTCTCRDY                                 RTC_C_TCMP_TCRDY                /*!< Real-time clock temperature compensation ready */
/* RTCTCMP[RTCTCMPS] Bits */
#define RTCTCMPS_OFS                             RTC_C_TCMP_TCMPS_OFS            /*!< RTCTCMPS Offset */
#define RTCTCMPS                                 RTC_C_TCMP_TCMPS                /*!< Real-time clock temperature compensation sign */
/* RTCPS0CTL[RT0PSIFG] Bits */
#define RT0PSIFG_OFS                             RTC_C_PS0CTL_RT0PSIFG_OFS       /*!< RT0PSIFG Offset */
#define RT0PSIFG                                 RTC_C_PS0CTL_RT0PSIFG           /*!< Prescale timer 0 interrupt flag */
/* RTCPS0CTL[RT0PSIE] Bits */
#define RT0PSIE_OFS                              RTC_C_PS0CTL_RT0PSIE_OFS        /*!< RT0PSIE Offset */
#define RT0PSIE                                  RTC_C_PS0CTL_RT0PSIE            /*!< Prescale timer 0 interrupt enable */
/* RTCPS0CTL[RT0IP] Bits */
#define RT0IP_OFS                                RTC_C_PS0CTL_RT0IP_OFS          /*!< RT0IP Offset */
#define RT0IP_M                                  RTC_C_PS0CTL_RT0IP_MASK         /*!< Prescale timer 0 interrupt interval */
#define RT0IP0                                   RTC_C_PS0CTL_RT0IP0             /*!< RT0IP Bit 0 */
#define RT0IP1                                   RTC_C_PS0CTL_RT0IP1             /*!< RT0IP Bit 1 */
#define RT0IP2                                   RTC_C_PS0CTL_RT0IP2             /*!< RT0IP Bit 2 */
#define RT0IP_0                                  RTC_C_PS0CTL_RT0IP_0            /*!< Divide by 2 */
#define RT0IP_1                                  RTC_C_PS0CTL_RT0IP_1            /*!< Divide by 4 */
#define RT0IP_2                                  RTC_C_PS0CTL_RT0IP_2            /*!< Divide by 8 */
#define RT0IP_3                                  RTC_C_PS0CTL_RT0IP_3            /*!< Divide by 16 */
#define RT0IP_4                                  RTC_C_PS0CTL_RT0IP_4            /*!< Divide by 32 */
#define RT0IP_5                                  RTC_C_PS0CTL_RT0IP_5            /*!< Divide by 64 */
#define RT0IP_6                                  RTC_C_PS0CTL_RT0IP_6            /*!< Divide by 128 */
#define RT0IP_7                                  RTC_C_PS0CTL_RT0IP_7            /*!< Divide by 256 */
#define RT0IP__2                                 RTC_C_PS0CTL_RT0IP__2           /*!< Divide by 2 */
#define RT0IP__4                                 RTC_C_PS0CTL_RT0IP__4           /*!< Divide by 4 */
#define RT0IP__8                                 RTC_C_PS0CTL_RT0IP__8           /*!< Divide by 8 */
#define RT0IP__16                                RTC_C_PS0CTL_RT0IP__16          /*!< Divide by 16 */
#define RT0IP__32                                RTC_C_PS0CTL_RT0IP__32          /*!< Divide by 32 */
#define RT0IP__64                                RTC_C_PS0CTL_RT0IP__64          /*!< Divide by 64 */
#define RT0IP__128                               RTC_C_PS0CTL_RT0IP__128         /*!< Divide by 128 */
#define RT0IP__256                               RTC_C_PS0CTL_RT0IP__256         /*!< Divide by 256 */
/* RTCPS1CTL[RT1PSIFG] Bits */
#define RT1PSIFG_OFS                             RTC_C_PS1CTL_RT1PSIFG_OFS       /*!< RT1PSIFG Offset */
#define RT1PSIFG                                 RTC_C_PS1CTL_RT1PSIFG           /*!< Prescale timer 1 interrupt flag */
/* RTCPS1CTL[RT1PSIE] Bits */
#define RT1PSIE_OFS                              RTC_C_PS1CTL_RT1PSIE_OFS        /*!< RT1PSIE Offset */
#define RT1PSIE                                  RTC_C_PS1CTL_RT1PSIE            /*!< Prescale timer 1 interrupt enable */
/* RTCPS1CTL[RT1IP] Bits */
#define RT1IP_OFS                                RTC_C_PS1CTL_RT1IP_OFS          /*!< RT1IP Offset */
#define RT1IP_M                                  RTC_C_PS1CTL_RT1IP_MASK         /*!< Prescale timer 1 interrupt interval */
#define RT1IP0                                   RTC_C_PS1CTL_RT1IP0             /*!< RT1IP Bit 0 */
#define RT1IP1                                   RTC_C_PS1CTL_RT1IP1             /*!< RT1IP Bit 1 */
#define RT1IP2                                   RTC_C_PS1CTL_RT1IP2             /*!< RT1IP Bit 2 */
#define RT1IP_0                                  RTC_C_PS1CTL_RT1IP_0            /*!< Divide by 2 */
#define RT1IP_1                                  RTC_C_PS1CTL_RT1IP_1            /*!< Divide by 4 */
#define RT1IP_2                                  RTC_C_PS1CTL_RT1IP_2            /*!< Divide by 8 */
#define RT1IP_3                                  RTC_C_PS1CTL_RT1IP_3            /*!< Divide by 16 */
#define RT1IP_4                                  RTC_C_PS1CTL_RT1IP_4            /*!< Divide by 32 */
#define RT1IP_5                                  RTC_C_PS1CTL_RT1IP_5            /*!< Divide by 64 */
#define RT1IP_6                                  RTC_C_PS1CTL_RT1IP_6            /*!< Divide by 128 */
#define RT1IP_7                                  RTC_C_PS1CTL_RT1IP_7            /*!< Divide by 256 */
#define RT1IP__2                                 RTC_C_PS1CTL_RT1IP__2           /*!< Divide by 2 */
#define RT1IP__4                                 RTC_C_PS1CTL_RT1IP__4           /*!< Divide by 4 */
#define RT1IP__8                                 RTC_C_PS1CTL_RT1IP__8           /*!< Divide by 8 */
#define RT1IP__16                                RTC_C_PS1CTL_RT1IP__16          /*!< Divide by 16 */
#define RT1IP__32                                RTC_C_PS1CTL_RT1IP__32          /*!< Divide by 32 */
#define RT1IP__64                                RTC_C_PS1CTL_RT1IP__64          /*!< Divide by 64 */
#define RT1IP__128                               RTC_C_PS1CTL_RT1IP__128         /*!< Divide by 128 */
#define RT1IP__256                               RTC_C_PS1CTL_RT1IP__256         /*!< Divide by 256 */
/* RTCPS[RT0PS] Bits */
#define RT0PS_OFS                                RTC_C_PS_RT0PS_OFS              /*!< RT0PS Offset */
#define RT0PS_M                                  RTC_C_PS_RT0PS_MASK             /*!< Prescale timer 0 counter value */
/* RTCPS[RT1PS] Bits */
#define RT1PS_OFS                                RTC_C_PS_RT1PS_OFS              /*!< RT1PS Offset */
#define RT1PS_M                                  RTC_C_PS_RT1PS_MASK             /*!< Prescale timer 1 counter value */
/* RTCTIM0[SECONDS] Bits */
#define SECONDS_OFS                              RTC_C_TIM0_SEC_OFS              /*!< Seconds Offset */
#define SECONDS_M                                RTC_C_TIM0_SEC_MASK             /*!< Seconds (0 to 59) */
/* RTCTIM0[MINUTES] Bits */
#define MINUTES_OFS                              RTC_C_TIM0_MIN_OFS              /*!< Minutes Offset */
#define MINUTES_M                                RTC_C_TIM0_MIN_MASK             /*!< Minutes (0 to 59) */
/* RTCTIM0_BCD[SECONDSLOWDIGIT] Bits */
#define SECONDSLOWDIGIT_OFS                      RTC_C_TIM0_SEC_LD_OFS           /*!< SecondsLowDigit Offset */
#define SECONDSLOWDIGIT_M                        RTC_C_TIM0_SEC_LD_MASK          /*!< Seconds  low digit (0 to 9) */
/* RTCTIM0_BCD[SECONDSHIGHDIGIT] Bits */
#define SECONDSHIGHDIGIT_OFS                     RTC_C_TIM0_SEC_HD_OFS           /*!< SecondsHighDigit Offset */
#define SECONDSHIGHDIGIT_M                       RTC_C_TIM0_SEC_HD_MASK          /*!< Seconds  high digit (0 to 5) */
/* RTCTIM0_BCD[MINUTESLOWDIGIT] Bits */
#define MINUTESLOWDIGIT_OFS                      RTC_C_TIM0_MIN_LD_OFS           /*!< MinutesLowDigit Offset */
#define MINUTESLOWDIGIT_M                        RTC_C_TIM0_MIN_LD_MASK          /*!< Minutes  low digit (0 to 9) */
/* RTCTIM0_BCD[MINUTESHIGHDIGIT] Bits */
#define MINUTESHIGHDIGIT_OFS                     RTC_C_TIM0_MIN_HD_OFS           /*!< MinutesHighDigit Offset */
#define MINUTESHIGHDIGIT_M                       RTC_C_TIM0_MIN_HD_MASK          /*!< Minutes  high digit (0 to 5) */
/* RTCTIM1[HOURS] Bits */
#define HOURS_OFS                                RTC_C_TIM1_HOUR_OFS             /*!< Hours Offset */
#define HOURS_M                                  RTC_C_TIM1_HOUR_MASK            /*!< Hours (0 to 23) */
/* RTCTIM1[DAYOFWEEK] Bits */
#define DAYOFWEEK_OFS                            RTC_C_TIM1_DOW_OFS              /*!< DayofWeek Offset */
#define DAYOFWEEK_M                              RTC_C_TIM1_DOW_MASK             /*!< Day of week (0 to 6) */
/* RTCTIM1_BCD[HOURSLOWDIGIT] Bits */
#define HOURSLOWDIGIT_OFS                        RTC_C_TIM1_HOUR_LD_OFS          /*!< HoursLowDigit Offset */
#define HOURSLOWDIGIT_M                          RTC_C_TIM1_HOUR_LD_MASK         /*!< Hours  low digit (0 to 9) */
/* RTCTIM1_BCD[HOURSHIGHDIGIT] Bits */
#define HOURSHIGHDIGIT_OFS                       RTC_C_TIM1_HOUR_HD_OFS          /*!< HoursHighDigit Offset */
#define HOURSHIGHDIGIT_M                         RTC_C_TIM1_HOUR_HD_MASK         /*!< Hours  high digit (0 to 2) */
/* RTCTIM1_BCD[DAYOFWEEK] Bits */
//#define DAYOFWEEK_OFS                            RTC_C_TIM1_DOW_OFS              /*!< DayofWeek Offset */
//#define DAYOFWEEK_M                              RTC_C_TIM1_DOW_MASK             /*!< Day of week (0 to 6) */
/* RTCDATE[DAY] Bits */
#define DAY_OFS                                  RTC_C_DATE_DAY_OFS              /*!< Day Offset */
#define DAY_M                                    RTC_C_DATE_DAY_MASK             /*!< Day of month (1 to 28, 29, 30, 31) */
/* RTCDATE[MONTH] Bits */
#define MONTH_OFS                                RTC_C_DATE_MON_OFS              /*!< Month Offset */
#define MONTH_M                                  RTC_C_DATE_MON_MASK             /*!< Month (1 to 12) */
/* RTCDATE_BCD[DAYLOWDIGIT] Bits */
#define DAYLOWDIGIT_OFS                          RTC_C_DATE_DAY_LD_OFS           /*!< DayLowDigit Offset */
#define DAYLOWDIGIT_M                            RTC_C_DATE_DAY_LD_MASK          /*!< Day of month  low digit (0 to 9) */
/* RTCDATE_BCD[DAYHIGHDIGIT] Bits */
#define DAYHIGHDIGIT_OFS                         RTC_C_DATE_DAY_HD_OFS           /*!< DayHighDigit Offset */
#define DAYHIGHDIGIT_M                           RTC_C_DATE_DAY_HD_MASK          /*!< Day of month  high digit (0 to 3) */
/* RTCDATE_BCD[MONTHLOWDIGIT] Bits */
#define MONTHLOWDIGIT_OFS                        RTC_C_DATE_MON_LD_OFS           /*!< MonthLowDigit Offset */
#define MONTHLOWDIGIT_M                          RTC_C_DATE_MON_LD_MASK          /*!< Month  low digit (0 to 9) */
/* RTCDATE_BCD[MONTHHIGHDIGIT] Bits */
#define MONTHHIGHDIGIT_OFS                       RTC_C_DATE_MON_HD_OFS           /*!< MonthHighDigit Offset */
#define MONTHHIGHDIGIT                           RTC_C_DATE_MON_HD               /*!< Month  high digit (0 or 1) */
/* RTCYEAR[YEARLOWBYTE] Bits */
#define YEARLOWBYTE_OFS                          RTC_C_YEAR_YEAR_LB_OFS          /*!< YearLowByte Offset */
#define YEARLOWBYTE_M                            RTC_C_YEAR_YEAR_LB_MASK         /*!< Year  low byte. Valid values for Year are 0 to 4095. */
/* RTCYEAR[YEARHIGHBYTE] Bits */
#define YEARHIGHBYTE_OFS                         RTC_C_YEAR_YEAR_HB_OFS          /*!< YearHighByte Offset */
#define YEARHIGHBYTE_M                           RTC_C_YEAR_YEAR_HB_MASK         /*!< Year  high byte. Valid values for Year are 0 to 4095. */
/* RTCYEAR_BCD[YEAR] Bits */
#define YEAR_OFS                                 RTC_C_YEAR_YEAR_OFS             /*!< Year Offset */
#define YEAR_M                                   RTC_C_YEAR_YEAR_MASK            /*!< Year  lowest digit (0 to 9) */
/* RTCYEAR_BCD[DECADE] Bits */
#define DECADE_OFS                               RTC_C_YEAR_DEC_OFS              /*!< Decade Offset */
#define DECADE_M                                 RTC_C_YEAR_DEC_MASK             /*!< Decade (0 to 9) */
/* RTCYEAR_BCD[CENTURYLOWDIGIT] Bits */
#define CENTURYLOWDIGIT_OFS                      RTC_C_YEAR_CENT_LD_OFS          /*!< CenturyLowDigit Offset */
#define CENTURYLOWDIGIT_M                        RTC_C_YEAR_CENT_LD_MASK         /*!< Century  low digit (0 to 9) */
/* RTCYEAR_BCD[CENTURYHIGHDIGIT] Bits */
#define CENTURYHIGHDIGIT_OFS                     RTC_C_YEAR_CENT_HD_OFS          /*!< CenturyHighDigit Offset */
#define CENTURYHIGHDIGIT_M                       RTC_C_YEAR_CENT_HD_MASK         /*!< Century  high digit (0 to 4) */
/* RTCAMINHR[MINUTES] Bits */
//#define MINUTES_OFS                              RTC_C_AMINHR_MIN_OFS            /*!< Minutes Offset */
//#define MINUTES_M                                RTC_C_AMINHR_MIN_MASK           /*!< Minutes (0 to 59) */
/* RTCAMINHR[MINAE] Bits */
#define MINAE_OFS                                RTC_C_AMINHR_MINAE_OFS          /*!< MINAE Offset */
#define MINAE                                    RTC_C_AMINHR_MINAE              /*!< Alarm enable */
/* RTCAMINHR[HOURS] Bits */
//#define HOURS_OFS                                RTC_C_AMINHR_HOUR_OFS           /*!< Hours Offset */
//#define HOURS_M                                  RTC_C_AMINHR_HOUR_MASK          /*!< Hours (0 to 23) */
/* RTCAMINHR[HOURAE] Bits */
#define HOURAE_OFS                               RTC_C_AMINHR_HOURAE_OFS         /*!< HOURAE Offset */
#define HOURAE                                   RTC_C_AMINHR_HOURAE             /*!< Alarm enable */
/* RTCAMINHR_BCD[MINUTESLOWDIGIT] Bits */
//#define MINUTESLOWDIGIT_OFS                      RTC_C_AMINHR_MIN_LD_OFS         /*!< MinutesLowDigit Offset */
//#define MINUTESLOWDIGIT_M                        RTC_C_AMINHR_MIN_LD_MASK        /*!< Minutes  low digit (0 to 9) */
/* RTCAMINHR_BCD[MINUTESHIGHDIGIT] Bits */
//#define MINUTESHIGHDIGIT_OFS                     RTC_C_AMINHR_MIN_HD_OFS         /*!< MinutesHighDigit Offset */
//#define MINUTESHIGHDIGIT_M                       RTC_C_AMINHR_MIN_HD_MASK        /*!< Minutes  high digit (0 to 5) */
/* RTCAMINHR_BCD[MINAE] Bits */
//#define MINAE_OFS                                RTC_C_AMINHR_MINAE_OFS          /*!< MINAE Offset */
//#define MINAE                                    RTC_C_AMINHR_MINAE              /*!< Alarm enable */
/* RTCAMINHR_BCD[HOURSLOWDIGIT] Bits */
//#define HOURSLOWDIGIT_OFS                        RTC_C_AMINHR_HOUR_LD_OFS        /*!< HoursLowDigit Offset */
//#define HOURSLOWDIGIT_M                          RTC_C_AMINHR_HOUR_LD_MASK       /*!< Hours  low digit (0 to 9) */
/* RTCAMINHR_BCD[HOURSHIGHDIGIT] Bits */
//#define HOURSHIGHDIGIT_OFS                       RTC_C_AMINHR_HOUR_HD_OFS        /*!< HoursHighDigit Offset */
//#define HOURSHIGHDIGIT_M                         RTC_C_AMINHR_HOUR_HD_MASK       /*!< Hours  high digit (0 to 2) */
/* RTCAMINHR_BCD[HOURAE] Bits */
//#define HOURAE_OFS                               RTC_C_AMINHR_HOURAE_OFS         /*!< HOURAE Offset */
//#define HOURAE                                   RTC_C_AMINHR_HOURAE             /*!< Alarm enable */
/* RTCADOWDAY[DAYOFWEEK] Bits */
//#define DAYOFWEEK_OFS                            RTC_C_ADOWDAY_DOW_OFS           /*!< DayofWeek Offset */
//#define DAYOFWEEK_M                              RTC_C_ADOWDAY_DOW_MASK          /*!< Day of week (0 to 6) */
/* RTCADOWDAY[DOWAE] Bits */
#define DOWAE_OFS                                RTC_C_ADOWDAY_DOWAE_OFS         /*!< DOWAE Offset */
#define DOWAE                                    RTC_C_ADOWDAY_DOWAE             /*!< Alarm enable */
/* RTCADOWDAY[DAYOFMONTH] Bits */
#define DAYOFMONTH_OFS                           RTC_C_ADOWDAY_DAY_OFS           /*!< DayofMonth Offset */
#define DAYOFMONTH_M                             RTC_C_ADOWDAY_DAY_MASK          /*!< Day of month (1 to 28, 29, 30, 31) */
/* RTCADOWDAY[DAYAE] Bits */
#define DAYAE_OFS                                RTC_C_ADOWDAY_DAYAE_OFS         /*!< DAYAE Offset */
#define DAYAE                                    RTC_C_ADOWDAY_DAYAE             /*!< Alarm enable */
/* RTCADOWDAY_BCD[DAYOFWEEK] Bits */
//#define DAYOFWEEK_OFS                            RTC_C_ADOWDAY_DOW_OFS           /*!< DayofWeek Offset */
//#define DAYOFWEEK_M                              RTC_C_ADOWDAY_DOW_MASK          /*!< Day of week (0 to 6) */
/* RTCADOWDAY_BCD[DOWAE] Bits */
//#define DOWAE_OFS                                RTC_C_ADOWDAY_DOWAE_OFS         /*!< DOWAE Offset */
//#define DOWAE                                    RTC_C_ADOWDAY_DOWAE             /*!< Alarm enable */
/* RTCADOWDAY_BCD[DAYLOWDIGIT] Bits */
//#define DAYLOWDIGIT_OFS                          RTC_C_ADOWDAY_DAY_LD_OFS        /*!< DayLowDigit Offset */
//#define DAYLOWDIGIT_M                            RTC_C_ADOWDAY_DAY_LD_MASK       /*!< Day of month  low digit (0 to 9) */
/* RTCADOWDAY_BCD[DAYHIGHDIGIT] Bits */
//#define DAYHIGHDIGIT_OFS                         RTC_C_ADOWDAY_DAY_HD_OFS        /*!< DayHighDigit Offset */
//#define DAYHIGHDIGIT_M                           RTC_C_ADOWDAY_DAY_HD_MASK       /*!< Day of month  high digit (0 to 3) */
/* RTCADOWDAY_BCD[DAYAE] Bits */
//#define DAYAE_OFS                                RTC_C_ADOWDAY_DAYAE_OFS         /*!< DAYAE Offset */
//#define DAYAE                                    RTC_C_ADOWDAY_DAYAE             /*!< Alarm enable */
/* Pre-defined bitfield values */
#define RTCKEY                                             RTC_C_KEY              /*!< RTC_C Key Value for RTC_C write access */
#define RTCKEY_H                                           RTC_C_KEY_H            /*!< RTC_C Key Value for RTC_C write access */
#define RTCKEY_VAL                                         RTC_C_KEY_VAL          /*!< RTC_C Key Value for RTC_C write access */


/******************************************************************************
* TIMER_A Bits (legacy section)
******************************************************************************/
/* TA0CTL[TAIFG] Bits */
#define TAIFG_OFS                                TIMER_A_CTL_IFG_OFS             /*!< TAIFG Offset */
#define TAIFG                                    TIMER_A_CTL_IFG                 /*!< TimerA interrupt flag */
/* TA0CTL[TAIE] Bits */
#define TAIE_OFS                                 TIMER_A_CTL_IE_OFS              /*!< TAIE Offset */
#define TAIE                                     TIMER_A_CTL_IE                  /*!< TimerA interrupt enable */
/* TA0CTL[TACLR] Bits */
#define TACLR_OFS                                TIMER_A_CTL_CLR_OFS             /*!< TACLR Offset */
#define TACLR                                    TIMER_A_CTL_CLR                 /*!< TimerA clear */
/* TA0CTL[MC] Bits */
#define MC_OFS                                   TIMER_A_CTL_MC_OFS              /*!< MC Offset */
#define MC_M                                     TIMER_A_CTL_MC_MASK             /*!< Mode control */
#define MC0                                      TIMER_A_CTL_MC0                 /*!< MC Bit 0 */
#define MC1                                      TIMER_A_CTL_MC1                 /*!< MC Bit 1 */
#define MC_0                                     TIMER_A_CTL_MC_0                /*!< Stop mode: Timer is halted */
#define MC_1                                     TIMER_A_CTL_MC_1                /*!< Up mode: Timer counts up to TAxCCR0 */
#define MC_2                                     TIMER_A_CTL_MC_2                /*!< Continuous mode: Timer counts up to 0FFFFh */
#define MC_3                                     TIMER_A_CTL_MC_3                /*!< Up/down mode: Timer counts up to TAxCCR0 then down to 0000h */
#define MC__STOP                                 TIMER_A_CTL_MC__STOP            /*!< Stop mode: Timer is halted */
#define MC__UP                                   TIMER_A_CTL_MC__UP              /*!< Up mode: Timer counts up to TAxCCR0 */
#define MC__CONTINUOUS                           TIMER_A_CTL_MC__CONTINUOUS      /*!< Continuous mode: Timer counts up to 0FFFFh */
#define MC__UPDOWN                               TIMER_A_CTL_MC__UPDOWN          /*!< Up/down mode: Timer counts up to TAxCCR0 then down to 0000h */
/* TA0CTL[ID] Bits */
#define ID_OFS                                   TIMER_A_CTL_ID_OFS              /*!< ID Offset */
#define ID_M                                     TIMER_A_CTL_ID_MASK             /*!< Input divider */
#define ID0                                      TIMER_A_CTL_ID0                 /*!< ID Bit 0 */
#define ID1                                      TIMER_A_CTL_ID1                 /*!< ID Bit 1 */
#define ID_0                                     TIMER_A_CTL_ID_0                /*!< /1 */
#define ID_1                                     TIMER_A_CTL_ID_1                /*!< /2 */
#define ID_2                                     TIMER_A_CTL_ID_2                /*!< /4 */
#define ID_3                                     TIMER_A_CTL_ID_3                /*!< /8 */
#define ID__1                                    TIMER_A_CTL_ID__1               /*!< /1 */
#define ID__2                                    TIMER_A_CTL_ID__2               /*!< /2 */
#define ID__4                                    TIMER_A_CTL_ID__4               /*!< /4 */
#define ID__8                                    TIMER_A_CTL_ID__8               /*!< /8 */
/* TA0CTL[TASSEL] Bits */
#define TASSEL_OFS                               TIMER_A_CTL_SSEL_OFS            /*!< TASSEL Offset */
#define TASSEL_M                                 TIMER_A_CTL_SSEL_MASK           /*!< TimerA clock source select */
#define TASSEL0                                  TIMER_A_CTL_SSEL0               /*!< TASSEL Bit 0 */
#define TASSEL1                                  TIMER_A_CTL_SSEL1               /*!< TASSEL Bit 1 */
#define TASSEL_0                                 TIMER_A_CTL_TASSEL_0            /*!< TAxCLK */
#define TASSEL_1                                 TIMER_A_CTL_TASSEL_1            /*!< ACLK */
#define TASSEL_2                                 TIMER_A_CTL_TASSEL_2            /*!< SMCLK */
#define TASSEL_3                                 TIMER_A_CTL_TASSEL_3            /*!< INCLK */
#define TASSEL__TACLK                            TIMER_A_CTL_SSEL__TACLK         /*!< TAxCLK */
#define TASSEL__ACLK                             TIMER_A_CTL_SSEL__ACLK          /*!< ACLK */
#define TASSEL__SMCLK                            TIMER_A_CTL_SSEL__SMCLK         /*!< SMCLK */
#define TASSEL__INCLK                            TIMER_A_CTL_SSEL__INCLK         /*!< INCLK */
/* TA0CCTLn[CCIFG] Bits */
#define CCIFG_OFS                                TIMER_A_CCTLN_CCIFG_OFS         /*!< CCIFG Offset */
#define CCIFG                                    TIMER_A_CCTLN_CCIFG             /*!< Capture/compare interrupt flag */
/* TA0CCTLn[COV] Bits */
#define COV_OFS                                  TIMER_A_CCTLN_COV_OFS           /*!< COV Offset */
#define COV                                      TIMER_A_CCTLN_COV               /*!< Capture overflow */
/* TA0CCTLn[OUT] Bits */
#define OUT_OFS                                  TIMER_A_CCTLN_OUT_OFS           /*!< OUT Offset */
//#define OUT                                      TIMER_A_CCTLN_OUT               /*!< Output */
/* TA0CCTLn[CCI] Bits */
#define CCI_OFS                                  TIMER_A_CCTLN_CCI_OFS           /*!< CCI Offset */
#define CCI                                      TIMER_A_CCTLN_CCI               /*!< Capture/compare input */
/* TA0CCTLn[CCIE] Bits */
#define CCIE_OFS                                 TIMER_A_CCTLN_CCIE_OFS          /*!< CCIE Offset */
#define CCIE                                     TIMER_A_CCTLN_CCIE              /*!< Capture/compare interrupt enable */
/* TA0CCTLn[OUTMOD] Bits */
#define OUTMOD_OFS                               TIMER_A_CCTLN_OUTMOD_OFS        /*!< OUTMOD Offset */
#define OUTMOD_M                                 TIMER_A_CCTLN_OUTMOD_MASK       /*!< Output mode */
#define OUTMOD0                                  TIMER_A_CCTLN_OUTMOD0           /*!< OUTMOD Bit 0 */
#define OUTMOD1                                  TIMER_A_CCTLN_OUTMOD1           /*!< OUTMOD Bit 1 */
#define OUTMOD2                                  TIMER_A_CCTLN_OUTMOD2           /*!< OUTMOD Bit 2 */
#define OUTMOD_0                                 TIMER_A_CCTLN_OUTMOD_0          /*!< OUT bit value */
#define OUTMOD_1                                 TIMER_A_CCTLN_OUTMOD_1          /*!< Set */
#define OUTMOD_2                                 TIMER_A_CCTLN_OUTMOD_2          /*!< Toggle/reset */
#define OUTMOD_3                                 TIMER_A_CCTLN_OUTMOD_3          /*!< Set/reset */
#define OUTMOD_4                                 TIMER_A_CCTLN_OUTMOD_4          /*!< Toggle */
#define OUTMOD_5                                 TIMER_A_CCTLN_OUTMOD_5          /*!< Reset */
#define OUTMOD_6                                 TIMER_A_CCTLN_OUTMOD_6          /*!< Toggle/set */
#define OUTMOD_7                                 TIMER_A_CCTLN_OUTMOD_7          /*!< Reset/set */
/* TA0CCTLn[CAP] Bits */
#define CAP_OFS                                  TIMER_A_CCTLN_CAP_OFS           /*!< CAP Offset */
#define CAP                                      TIMER_A_CCTLN_CAP               /*!< Capture mode */
/* TA0CCTLn[SCCI] Bits */
#define SCCI_OFS                                 TIMER_A_CCTLN_SCCI_OFS          /*!< SCCI Offset */
#define SCCI                                     TIMER_A_CCTLN_SCCI              /*!< Synchronized capture/compare input */
/* TA0CCTLn[SCS] Bits */
#define SCS_OFS                                  TIMER_A_CCTLN_SCS_OFS           /*!< SCS Offset */
#define SCS                                      TIMER_A_CCTLN_SCS               /*!< Synchronize capture source */
/* TA0CCTLn[CCIS] Bits */
#define CCIS_OFS                                 TIMER_A_CCTLN_CCIS_OFS          /*!< CCIS Offset */
#define CCIS_M                                   TIMER_A_CCTLN_CCIS_MASK         /*!< Capture/compare input select */
#define CCIS0                                    TIMER_A_CCTLN_CCIS0             /*!< CCIS Bit 0 */
#define CCIS1                                    TIMER_A_CCTLN_CCIS1             /*!< CCIS Bit 1 */
#define CCIS_0                                   TIMER_A_CCTLN_CCIS_0            /*!< CCIxA */
#define CCIS_1                                   TIMER_A_CCTLN_CCIS_1            /*!< CCIxB */
#define CCIS_2                                   TIMER_A_CCTLN_CCIS_2            /*!< GND */
#define CCIS_3                                   TIMER_A_CCTLN_CCIS_3            /*!< VCC */
#define CCIS__CCIA                               TIMER_A_CCTLN_CCIS__CCIA        /*!< CCIxA */
#define CCIS__CCIB                               TIMER_A_CCTLN_CCIS__CCIB        /*!< CCIxB */
#define CCIS__GND                                TIMER_A_CCTLN_CCIS__GND         /*!< GND */
#define CCIS__VCC                                TIMER_A_CCTLN_CCIS__VCC         /*!< VCC */
/* TA0CCTLn[CM] Bits */
#define CM_OFS                                   TIMER_A_CCTLN_CM_OFS            /*!< CM Offset */
#define CM_M                                     TIMER_A_CCTLN_CM_MASK           /*!< Capture mode */
#define CM0                                      TIMER_A_CCTLN_CM0               /*!< CM Bit 0 */
#define CM1                                      TIMER_A_CCTLN_CM1               /*!< CM Bit 1 */
#define CM_0                                     TIMER_A_CCTLN_CM_0              /*!< No capture */
#define CM_1                                     TIMER_A_CCTLN_CM_1              /*!< Capture on rising edge */
#define CM_2                                     TIMER_A_CCTLN_CM_2              /*!< Capture on falling edge */
#define CM_3                                     TIMER_A_CCTLN_CM_3              /*!< Capture on both rising and falling edges */
#define CM__NONE                                 TIMER_A_CCTLN_CM__NONE          /*!< No capture */
#define CM__RISING                               TIMER_A_CCTLN_CM__RISING        /*!< Capture on rising edge */
#define CM__FALLING                              TIMER_A_CCTLN_CM__FALLING       /*!< Capture on falling edge */
#define CM__BOTH                                 TIMER_A_CCTLN_CM__BOTH          /*!< Capture on both rising and falling edges */
/* TA0EX0[TAIDEX] Bits */
#define TAIDEX_OFS                               TIMER_A_EX0_IDEX_OFS            /*!< TAIDEX Offset */
#define TAIDEX_M                                 TIMER_A_EX0_IDEX_MASK           /*!< Input divider expansion */
#define TAIDEX0                                  TIMER_A_EX0_IDEX0               /*!< TAIDEX Bit 0 */
#define TAIDEX1                                  TIMER_A_EX0_IDEX1               /*!< TAIDEX Bit 1 */
#define TAIDEX2                                  TIMER_A_EX0_IDEX2               /*!< TAIDEX Bit 2 */
#define TAIDEX_0                                 TIMER_A_EX0_TAIDEX_0            /*!< Divide by 1 */
#define TAIDEX_1                                 TIMER_A_EX0_TAIDEX_1            /*!< Divide by 2 */
#define TAIDEX_2                                 TIMER_A_EX0_TAIDEX_2            /*!< Divide by 3 */
#define TAIDEX_3                                 TIMER_A_EX0_TAIDEX_3            /*!< Divide by 4 */
#define TAIDEX_4                                 TIMER_A_EX0_TAIDEX_4            /*!< Divide by 5 */
#define TAIDEX_5                                 TIMER_A_EX0_TAIDEX_5            /*!< Divide by 6 */
#define TAIDEX_6                                 TIMER_A_EX0_TAIDEX_6            /*!< Divide by 7 */
#define TAIDEX_7                                 TIMER_A_EX0_TAIDEX_7            /*!< Divide by 8 */
#define TAIDEX__1                                TIMER_A_EX0_IDEX__1             /*!< Divide by 1 */
#define TAIDEX__2                                TIMER_A_EX0_IDEX__2             /*!< Divide by 2 */
#define TAIDEX__3                                TIMER_A_EX0_IDEX__3             /*!< Divide by 3 */
#define TAIDEX__4                                TIMER_A_EX0_IDEX__4             /*!< Divide by 4 */
#define TAIDEX__5                                TIMER_A_EX0_IDEX__5             /*!< Divide by 5 */
#define TAIDEX__6                                TIMER_A_EX0_IDEX__6             /*!< Divide by 6 */
#define TAIDEX__7                                TIMER_A_EX0_IDEX__7             /*!< Divide by 7 */
#define TAIDEX__8                                TIMER_A_EX0_IDEX__8             /*!< Divide by 8 */

/******************************************************************************
* WDT_A Bits (legacy section)
******************************************************************************/
/* WDTCTL[WDTIS] Bits */
#define WDTIS_OFS                                WDT_A_CTL_IS_OFS                /*!< WDTIS Offset */
#define WDTIS_M                                  WDT_A_CTL_IS_MASK               /*!< Watchdog timer interval select */
#define WDTIS0                                   WDT_A_CTL_IS0                   /*!< WDTIS Bit 0 */
#define WDTIS1                                   WDT_A_CTL_IS1                   /*!< WDTIS Bit 1 */
#define WDTIS2                                   WDT_A_CTL_IS2                   /*!< WDTIS Bit 2 */
#define WDTIS_0                                  WDT_A_CTL_IS_0                  /*!< Watchdog clock source / (2^(31)) (18:12:16 at 32.768 kHz) */
#define WDTIS_1                                  WDT_A_CTL_IS_1                  /*!< Watchdog clock source /(2^(27)) (01:08:16 at 32.768 kHz) */
#define WDTIS_2                                  WDT_A_CTL_IS_2                  /*!< Watchdog clock source /(2^(23)) (00:04:16 at 32.768 kHz) */
#define WDTIS_3                                  WDT_A_CTL_IS_3                  /*!< Watchdog clock source /(2^(19)) (00:00:16 at 32.768 kHz) */
#define WDTIS_4                                  WDT_A_CTL_IS_4                  /*!< Watchdog clock source /(2^(15)) (1 s at 32.768 kHz) */
#define WDTIS_5                                  WDT_A_CTL_IS_5                  /*!< Watchdog clock source / (2^(13)) (250 ms at 32.768 kHz) */
#define WDTIS_6                                  WDT_A_CTL_IS_6                  /*!< Watchdog clock source / (2^(9)) (15.625 ms at 32.768 kHz) */
#define WDTIS_7                                  WDT_A_CTL_IS_7                  /*!< Watchdog clock source / (2^(6)) (1.95 ms at 32.768 kHz) */
/* WDTCTL[WDTCNTCL] Bits */
#define WDTCNTCL_OFS                             WDT_A_CTL_CNTCL_OFS             /*!< WDTCNTCL Offset */
#define WDTCNTCL                                 WDT_A_CTL_CNTCL                 /*!< Watchdog timer counter clear */
/* WDTCTL[WDTTMSEL] Bits */
#define WDTTMSEL_OFS                             WDT_A_CTL_TMSEL_OFS             /*!< WDTTMSEL Offset */
#define WDTTMSEL                                 WDT_A_CTL_TMSEL                 /*!< Watchdog timer mode select */
/* WDTCTL[WDTSSEL] Bits */
#define WDTSSEL_OFS                              WDT_A_CTL_SSEL_OFS              /*!< WDTSSEL Offset */
#define WDTSSEL_M                                WDT_A_CTL_SSEL_MASK             /*!< Watchdog timer clock source select */
#define WDTSSEL0                                 WDT_A_CTL_SSEL0                 /*!< WDTSSEL Bit 0 */
#define WDTSSEL1                                 WDT_A_CTL_SSEL1                 /*!< WDTSSEL Bit 1 */
#define WDTSSEL_0                                WDT_A_CTL_SSEL_0                /*!< SMCLK */
#define WDTSSEL_1                                WDT_A_CTL_SSEL_1                /*!< ACLK */
#define WDTSSEL_2                                WDT_A_CTL_SSEL_2                /*!< VLOCLK */
#define WDTSSEL_3                                WDT_A_CTL_SSEL_3                /*!< BCLK */
#define WDTSSEL__SMCLK                           WDT_A_CTL_SSEL__SMCLK           /*!< SMCLK */
#define WDTSSEL__ACLK                            WDT_A_CTL_SSEL__ACLK            /*!< ACLK */
#define WDTSSEL__VLOCLK                          WDT_A_CTL_SSEL__VLOCLK          /*!< VLOCLK */
#define WDTSSEL__BCLK                            WDT_A_CTL_SSEL__BCLK            /*!< BCLK */
/* WDTCTL[WDTHOLD] Bits */
#define WDTHOLD_OFS                              WDT_A_CTL_HOLD_OFS              /*!< WDTHOLD Offset */
#define WDTHOLD                                  WDT_A_CTL_HOLD                  /*!< Watchdog timer hold */
/* WDTCTL[WDTPW] Bits */
#define WDTPW_OFS                                WDT_A_CTL_PW_OFS                /*!< WDTPW Offset */
#define WDTPW_M                                  WDT_A_CTL_PW_MASK               /*!< Watchdog timer password */
/* Pre-defined bitfield values */
#define WDTPW                                          WDT_A_CTL_PW              /*!< WDT Key Value for WDT write access */


#ifdef __cplusplus
}
#endif

#endif /* __MSP432P401M_CLASSIC_H__ */
