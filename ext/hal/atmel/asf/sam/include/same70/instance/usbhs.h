/**
 * \file
 *
 * \brief Instance description for USBHS
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_USBHS_INSTANCE_H_
#define _SAME70_USBHS_INSTANCE_H_

/* ========== Register definition for USBHS peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_USBHS_DEVDMANXTDSC0 (0x40038310) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 0 */
#define REG_USBHS_DEVDMAADDRESS0 (0x40038314) /**< (USBHS) Device DMA Channel Address Register (n = 1) 0 */
#define REG_USBHS_DEVDMACONTROL0 (0x40038318) /**< (USBHS) Device DMA Channel Control Register (n = 1) 0 */
#define REG_USBHS_DEVDMASTATUS0 (0x4003831C) /**< (USBHS) Device DMA Channel Status Register (n = 1) 0 */
#define REG_USBHS_DEVDMANXTDSC1 (0x40038320) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 1 */
#define REG_USBHS_DEVDMAADDRESS1 (0x40038324) /**< (USBHS) Device DMA Channel Address Register (n = 1) 1 */
#define REG_USBHS_DEVDMACONTROL1 (0x40038328) /**< (USBHS) Device DMA Channel Control Register (n = 1) 1 */
#define REG_USBHS_DEVDMASTATUS1 (0x4003832C) /**< (USBHS) Device DMA Channel Status Register (n = 1) 1 */
#define REG_USBHS_DEVDMANXTDSC2 (0x40038330) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 2 */
#define REG_USBHS_DEVDMAADDRESS2 (0x40038334) /**< (USBHS) Device DMA Channel Address Register (n = 1) 2 */
#define REG_USBHS_DEVDMACONTROL2 (0x40038338) /**< (USBHS) Device DMA Channel Control Register (n = 1) 2 */
#define REG_USBHS_DEVDMASTATUS2 (0x4003833C) /**< (USBHS) Device DMA Channel Status Register (n = 1) 2 */
#define REG_USBHS_DEVDMANXTDSC3 (0x40038340) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 3 */
#define REG_USBHS_DEVDMAADDRESS3 (0x40038344) /**< (USBHS) Device DMA Channel Address Register (n = 1) 3 */
#define REG_USBHS_DEVDMACONTROL3 (0x40038348) /**< (USBHS) Device DMA Channel Control Register (n = 1) 3 */
#define REG_USBHS_DEVDMASTATUS3 (0x4003834C) /**< (USBHS) Device DMA Channel Status Register (n = 1) 3 */
#define REG_USBHS_DEVDMANXTDSC4 (0x40038350) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 4 */
#define REG_USBHS_DEVDMAADDRESS4 (0x40038354) /**< (USBHS) Device DMA Channel Address Register (n = 1) 4 */
#define REG_USBHS_DEVDMACONTROL4 (0x40038358) /**< (USBHS) Device DMA Channel Control Register (n = 1) 4 */
#define REG_USBHS_DEVDMASTATUS4 (0x4003835C) /**< (USBHS) Device DMA Channel Status Register (n = 1) 4 */
#define REG_USBHS_DEVDMANXTDSC5 (0x40038360) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 5 */
#define REG_USBHS_DEVDMAADDRESS5 (0x40038364) /**< (USBHS) Device DMA Channel Address Register (n = 1) 5 */
#define REG_USBHS_DEVDMACONTROL5 (0x40038368) /**< (USBHS) Device DMA Channel Control Register (n = 1) 5 */
#define REG_USBHS_DEVDMASTATUS5 (0x4003836C) /**< (USBHS) Device DMA Channel Status Register (n = 1) 5 */
#define REG_USBHS_DEVDMANXTDSC6 (0x40038370) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 6 */
#define REG_USBHS_DEVDMAADDRESS6 (0x40038374) /**< (USBHS) Device DMA Channel Address Register (n = 1) 6 */
#define REG_USBHS_DEVDMACONTROL6 (0x40038378) /**< (USBHS) Device DMA Channel Control Register (n = 1) 6 */
#define REG_USBHS_DEVDMASTATUS6 (0x4003837C) /**< (USBHS) Device DMA Channel Status Register (n = 1) 6 */
#define REG_USBHS_HSTDMANXTDSC0 (0x40038710) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 0 */
#define REG_USBHS_HSTDMAADDRESS0 (0x40038714) /**< (USBHS) Host DMA Channel Address Register (n = 1) 0 */
#define REG_USBHS_HSTDMACONTROL0 (0x40038718) /**< (USBHS) Host DMA Channel Control Register (n = 1) 0 */
#define REG_USBHS_HSTDMASTATUS0 (0x4003871C) /**< (USBHS) Host DMA Channel Status Register (n = 1) 0 */
#define REG_USBHS_HSTDMANXTDSC1 (0x40038720) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 1 */
#define REG_USBHS_HSTDMAADDRESS1 (0x40038724) /**< (USBHS) Host DMA Channel Address Register (n = 1) 1 */
#define REG_USBHS_HSTDMACONTROL1 (0x40038728) /**< (USBHS) Host DMA Channel Control Register (n = 1) 1 */
#define REG_USBHS_HSTDMASTATUS1 (0x4003872C) /**< (USBHS) Host DMA Channel Status Register (n = 1) 1 */
#define REG_USBHS_HSTDMANXTDSC2 (0x40038730) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 2 */
#define REG_USBHS_HSTDMAADDRESS2 (0x40038734) /**< (USBHS) Host DMA Channel Address Register (n = 1) 2 */
#define REG_USBHS_HSTDMACONTROL2 (0x40038738) /**< (USBHS) Host DMA Channel Control Register (n = 1) 2 */
#define REG_USBHS_HSTDMASTATUS2 (0x4003873C) /**< (USBHS) Host DMA Channel Status Register (n = 1) 2 */
#define REG_USBHS_HSTDMANXTDSC3 (0x40038740) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 3 */
#define REG_USBHS_HSTDMAADDRESS3 (0x40038744) /**< (USBHS) Host DMA Channel Address Register (n = 1) 3 */
#define REG_USBHS_HSTDMACONTROL3 (0x40038748) /**< (USBHS) Host DMA Channel Control Register (n = 1) 3 */
#define REG_USBHS_HSTDMASTATUS3 (0x4003874C) /**< (USBHS) Host DMA Channel Status Register (n = 1) 3 */
#define REG_USBHS_HSTDMANXTDSC4 (0x40038750) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 4 */
#define REG_USBHS_HSTDMAADDRESS4 (0x40038754) /**< (USBHS) Host DMA Channel Address Register (n = 1) 4 */
#define REG_USBHS_HSTDMACONTROL4 (0x40038758) /**< (USBHS) Host DMA Channel Control Register (n = 1) 4 */
#define REG_USBHS_HSTDMASTATUS4 (0x4003875C) /**< (USBHS) Host DMA Channel Status Register (n = 1) 4 */
#define REG_USBHS_HSTDMANXTDSC5 (0x40038760) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 5 */
#define REG_USBHS_HSTDMAADDRESS5 (0x40038764) /**< (USBHS) Host DMA Channel Address Register (n = 1) 5 */
#define REG_USBHS_HSTDMACONTROL5 (0x40038768) /**< (USBHS) Host DMA Channel Control Register (n = 1) 5 */
#define REG_USBHS_HSTDMASTATUS5 (0x4003876C) /**< (USBHS) Host DMA Channel Status Register (n = 1) 5 */
#define REG_USBHS_HSTDMANXTDSC6 (0x40038770) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 6 */
#define REG_USBHS_HSTDMAADDRESS6 (0x40038774) /**< (USBHS) Host DMA Channel Address Register (n = 1) 6 */
#define REG_USBHS_HSTDMACONTROL6 (0x40038778) /**< (USBHS) Host DMA Channel Control Register (n = 1) 6 */
#define REG_USBHS_HSTDMASTATUS6 (0x4003877C) /**< (USBHS) Host DMA Channel Status Register (n = 1) 6 */
#define REG_USBHS_DEVCTRL       (0x40038000) /**< (USBHS) Device General Control Register */
#define REG_USBHS_DEVISR        (0x40038004) /**< (USBHS) Device Global Interrupt Status Register */
#define REG_USBHS_DEVICR        (0x40038008) /**< (USBHS) Device Global Interrupt Clear Register */
#define REG_USBHS_DEVIFR        (0x4003800C) /**< (USBHS) Device Global Interrupt Set Register */
#define REG_USBHS_DEVIMR        (0x40038010) /**< (USBHS) Device Global Interrupt Mask Register */
#define REG_USBHS_DEVIDR        (0x40038014) /**< (USBHS) Device Global Interrupt Disable Register */
#define REG_USBHS_DEVIER        (0x40038018) /**< (USBHS) Device Global Interrupt Enable Register */
#define REG_USBHS_DEVEPT        (0x4003801C) /**< (USBHS) Device Endpoint Register */
#define REG_USBHS_DEVFNUM       (0x40038020) /**< (USBHS) Device Frame Number Register */
#define REG_USBHS_DEVEPTCFG     (0x40038100) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 0 */
#define REG_USBHS_DEVEPTCFG0    (0x40038100) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 0 */
#define REG_USBHS_DEVEPTCFG1    (0x40038104) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 1 */
#define REG_USBHS_DEVEPTCFG2    (0x40038108) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 2 */
#define REG_USBHS_DEVEPTCFG3    (0x4003810C) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 3 */
#define REG_USBHS_DEVEPTCFG4    (0x40038110) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 4 */
#define REG_USBHS_DEVEPTCFG5    (0x40038114) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 5 */
#define REG_USBHS_DEVEPTCFG6    (0x40038118) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 6 */
#define REG_USBHS_DEVEPTCFG7    (0x4003811C) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 7 */
#define REG_USBHS_DEVEPTCFG8    (0x40038120) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 8 */
#define REG_USBHS_DEVEPTCFG9    (0x40038124) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 9 */
#define REG_USBHS_DEVEPTISR     (0x40038130) /**< (USBHS) Device Endpoint Status Register (n = 0) 0 */
#define REG_USBHS_DEVEPTISR0    (0x40038130) /**< (USBHS) Device Endpoint Status Register (n = 0) 0 */
#define REG_USBHS_DEVEPTISR1    (0x40038134) /**< (USBHS) Device Endpoint Status Register (n = 0) 1 */
#define REG_USBHS_DEVEPTISR2    (0x40038138) /**< (USBHS) Device Endpoint Status Register (n = 0) 2 */
#define REG_USBHS_DEVEPTISR3    (0x4003813C) /**< (USBHS) Device Endpoint Status Register (n = 0) 3 */
#define REG_USBHS_DEVEPTISR4    (0x40038140) /**< (USBHS) Device Endpoint Status Register (n = 0) 4 */
#define REG_USBHS_DEVEPTISR5    (0x40038144) /**< (USBHS) Device Endpoint Status Register (n = 0) 5 */
#define REG_USBHS_DEVEPTISR6    (0x40038148) /**< (USBHS) Device Endpoint Status Register (n = 0) 6 */
#define REG_USBHS_DEVEPTISR7    (0x4003814C) /**< (USBHS) Device Endpoint Status Register (n = 0) 7 */
#define REG_USBHS_DEVEPTISR8    (0x40038150) /**< (USBHS) Device Endpoint Status Register (n = 0) 8 */
#define REG_USBHS_DEVEPTISR9    (0x40038154) /**< (USBHS) Device Endpoint Status Register (n = 0) 9 */
#define REG_USBHS_DEVEPTICR     (0x40038160) /**< (USBHS) Device Endpoint Clear Register (n = 0) 0 */
#define REG_USBHS_DEVEPTICR0    (0x40038160) /**< (USBHS) Device Endpoint Clear Register (n = 0) 0 */
#define REG_USBHS_DEVEPTICR1    (0x40038164) /**< (USBHS) Device Endpoint Clear Register (n = 0) 1 */
#define REG_USBHS_DEVEPTICR2    (0x40038168) /**< (USBHS) Device Endpoint Clear Register (n = 0) 2 */
#define REG_USBHS_DEVEPTICR3    (0x4003816C) /**< (USBHS) Device Endpoint Clear Register (n = 0) 3 */
#define REG_USBHS_DEVEPTICR4    (0x40038170) /**< (USBHS) Device Endpoint Clear Register (n = 0) 4 */
#define REG_USBHS_DEVEPTICR5    (0x40038174) /**< (USBHS) Device Endpoint Clear Register (n = 0) 5 */
#define REG_USBHS_DEVEPTICR6    (0x40038178) /**< (USBHS) Device Endpoint Clear Register (n = 0) 6 */
#define REG_USBHS_DEVEPTICR7    (0x4003817C) /**< (USBHS) Device Endpoint Clear Register (n = 0) 7 */
#define REG_USBHS_DEVEPTICR8    (0x40038180) /**< (USBHS) Device Endpoint Clear Register (n = 0) 8 */
#define REG_USBHS_DEVEPTICR9    (0x40038184) /**< (USBHS) Device Endpoint Clear Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIFR     (0x40038190) /**< (USBHS) Device Endpoint Set Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIFR0    (0x40038190) /**< (USBHS) Device Endpoint Set Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIFR1    (0x40038194) /**< (USBHS) Device Endpoint Set Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIFR2    (0x40038198) /**< (USBHS) Device Endpoint Set Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIFR3    (0x4003819C) /**< (USBHS) Device Endpoint Set Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIFR4    (0x400381A0) /**< (USBHS) Device Endpoint Set Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIFR5    (0x400381A4) /**< (USBHS) Device Endpoint Set Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIFR6    (0x400381A8) /**< (USBHS) Device Endpoint Set Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIFR7    (0x400381AC) /**< (USBHS) Device Endpoint Set Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIFR8    (0x400381B0) /**< (USBHS) Device Endpoint Set Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIFR9    (0x400381B4) /**< (USBHS) Device Endpoint Set Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIMR     (0x400381C0) /**< (USBHS) Device Endpoint Mask Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIMR0    (0x400381C0) /**< (USBHS) Device Endpoint Mask Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIMR1    (0x400381C4) /**< (USBHS) Device Endpoint Mask Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIMR2    (0x400381C8) /**< (USBHS) Device Endpoint Mask Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIMR3    (0x400381CC) /**< (USBHS) Device Endpoint Mask Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIMR4    (0x400381D0) /**< (USBHS) Device Endpoint Mask Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIMR5    (0x400381D4) /**< (USBHS) Device Endpoint Mask Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIMR6    (0x400381D8) /**< (USBHS) Device Endpoint Mask Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIMR7    (0x400381DC) /**< (USBHS) Device Endpoint Mask Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIMR8    (0x400381E0) /**< (USBHS) Device Endpoint Mask Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIMR9    (0x400381E4) /**< (USBHS) Device Endpoint Mask Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIER     (0x400381F0) /**< (USBHS) Device Endpoint Enable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIER0    (0x400381F0) /**< (USBHS) Device Endpoint Enable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIER1    (0x400381F4) /**< (USBHS) Device Endpoint Enable Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIER2    (0x400381F8) /**< (USBHS) Device Endpoint Enable Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIER3    (0x400381FC) /**< (USBHS) Device Endpoint Enable Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIER4    (0x40038200) /**< (USBHS) Device Endpoint Enable Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIER5    (0x40038204) /**< (USBHS) Device Endpoint Enable Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIER6    (0x40038208) /**< (USBHS) Device Endpoint Enable Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIER7    (0x4003820C) /**< (USBHS) Device Endpoint Enable Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIER8    (0x40038210) /**< (USBHS) Device Endpoint Enable Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIER9    (0x40038214) /**< (USBHS) Device Endpoint Enable Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIDR     (0x40038220) /**< (USBHS) Device Endpoint Disable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIDR0    (0x40038220) /**< (USBHS) Device Endpoint Disable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIDR1    (0x40038224) /**< (USBHS) Device Endpoint Disable Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIDR2    (0x40038228) /**< (USBHS) Device Endpoint Disable Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIDR3    (0x4003822C) /**< (USBHS) Device Endpoint Disable Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIDR4    (0x40038230) /**< (USBHS) Device Endpoint Disable Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIDR5    (0x40038234) /**< (USBHS) Device Endpoint Disable Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIDR6    (0x40038238) /**< (USBHS) Device Endpoint Disable Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIDR7    (0x4003823C) /**< (USBHS) Device Endpoint Disable Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIDR8    (0x40038240) /**< (USBHS) Device Endpoint Disable Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIDR9    (0x40038244) /**< (USBHS) Device Endpoint Disable Register (n = 0) 9 */
#define REG_USBHS_HSTCTRL       (0x40038400) /**< (USBHS) Host General Control Register */
#define REG_USBHS_HSTISR        (0x40038404) /**< (USBHS) Host Global Interrupt Status Register */
#define REG_USBHS_HSTICR        (0x40038408) /**< (USBHS) Host Global Interrupt Clear Register */
#define REG_USBHS_HSTIFR        (0x4003840C) /**< (USBHS) Host Global Interrupt Set Register */
#define REG_USBHS_HSTIMR        (0x40038410) /**< (USBHS) Host Global Interrupt Mask Register */
#define REG_USBHS_HSTIDR        (0x40038414) /**< (USBHS) Host Global Interrupt Disable Register */
#define REG_USBHS_HSTIER        (0x40038418) /**< (USBHS) Host Global Interrupt Enable Register */
#define REG_USBHS_HSTPIP        (0x4003841C) /**< (USBHS) Host Pipe Register */
#define REG_USBHS_HSTFNUM       (0x40038420) /**< (USBHS) Host Frame Number Register */
#define REG_USBHS_HSTADDR1      (0x40038424) /**< (USBHS) Host Address 1 Register */
#define REG_USBHS_HSTADDR2      (0x40038428) /**< (USBHS) Host Address 2 Register */
#define REG_USBHS_HSTADDR3      (0x4003842C) /**< (USBHS) Host Address 3 Register */
#define REG_USBHS_HSTPIPCFG     (0x40038500) /**< (USBHS) Host Pipe Configuration Register (n = 0) 0 */
#define REG_USBHS_HSTPIPCFG0    (0x40038500) /**< (USBHS) Host Pipe Configuration Register (n = 0) 0 */
#define REG_USBHS_HSTPIPCFG1    (0x40038504) /**< (USBHS) Host Pipe Configuration Register (n = 0) 1 */
#define REG_USBHS_HSTPIPCFG2    (0x40038508) /**< (USBHS) Host Pipe Configuration Register (n = 0) 2 */
#define REG_USBHS_HSTPIPCFG3    (0x4003850C) /**< (USBHS) Host Pipe Configuration Register (n = 0) 3 */
#define REG_USBHS_HSTPIPCFG4    (0x40038510) /**< (USBHS) Host Pipe Configuration Register (n = 0) 4 */
#define REG_USBHS_HSTPIPCFG5    (0x40038514) /**< (USBHS) Host Pipe Configuration Register (n = 0) 5 */
#define REG_USBHS_HSTPIPCFG6    (0x40038518) /**< (USBHS) Host Pipe Configuration Register (n = 0) 6 */
#define REG_USBHS_HSTPIPCFG7    (0x4003851C) /**< (USBHS) Host Pipe Configuration Register (n = 0) 7 */
#define REG_USBHS_HSTPIPCFG8    (0x40038520) /**< (USBHS) Host Pipe Configuration Register (n = 0) 8 */
#define REG_USBHS_HSTPIPCFG9    (0x40038524) /**< (USBHS) Host Pipe Configuration Register (n = 0) 9 */
#define REG_USBHS_HSTPIPISR     (0x40038530) /**< (USBHS) Host Pipe Status Register (n = 0) 0 */
#define REG_USBHS_HSTPIPISR0    (0x40038530) /**< (USBHS) Host Pipe Status Register (n = 0) 0 */
#define REG_USBHS_HSTPIPISR1    (0x40038534) /**< (USBHS) Host Pipe Status Register (n = 0) 1 */
#define REG_USBHS_HSTPIPISR2    (0x40038538) /**< (USBHS) Host Pipe Status Register (n = 0) 2 */
#define REG_USBHS_HSTPIPISR3    (0x4003853C) /**< (USBHS) Host Pipe Status Register (n = 0) 3 */
#define REG_USBHS_HSTPIPISR4    (0x40038540) /**< (USBHS) Host Pipe Status Register (n = 0) 4 */
#define REG_USBHS_HSTPIPISR5    (0x40038544) /**< (USBHS) Host Pipe Status Register (n = 0) 5 */
#define REG_USBHS_HSTPIPISR6    (0x40038548) /**< (USBHS) Host Pipe Status Register (n = 0) 6 */
#define REG_USBHS_HSTPIPISR7    (0x4003854C) /**< (USBHS) Host Pipe Status Register (n = 0) 7 */
#define REG_USBHS_HSTPIPISR8    (0x40038550) /**< (USBHS) Host Pipe Status Register (n = 0) 8 */
#define REG_USBHS_HSTPIPISR9    (0x40038554) /**< (USBHS) Host Pipe Status Register (n = 0) 9 */
#define REG_USBHS_HSTPIPICR     (0x40038560) /**< (USBHS) Host Pipe Clear Register (n = 0) 0 */
#define REG_USBHS_HSTPIPICR0    (0x40038560) /**< (USBHS) Host Pipe Clear Register (n = 0) 0 */
#define REG_USBHS_HSTPIPICR1    (0x40038564) /**< (USBHS) Host Pipe Clear Register (n = 0) 1 */
#define REG_USBHS_HSTPIPICR2    (0x40038568) /**< (USBHS) Host Pipe Clear Register (n = 0) 2 */
#define REG_USBHS_HSTPIPICR3    (0x4003856C) /**< (USBHS) Host Pipe Clear Register (n = 0) 3 */
#define REG_USBHS_HSTPIPICR4    (0x40038570) /**< (USBHS) Host Pipe Clear Register (n = 0) 4 */
#define REG_USBHS_HSTPIPICR5    (0x40038574) /**< (USBHS) Host Pipe Clear Register (n = 0) 5 */
#define REG_USBHS_HSTPIPICR6    (0x40038578) /**< (USBHS) Host Pipe Clear Register (n = 0) 6 */
#define REG_USBHS_HSTPIPICR7    (0x4003857C) /**< (USBHS) Host Pipe Clear Register (n = 0) 7 */
#define REG_USBHS_HSTPIPICR8    (0x40038580) /**< (USBHS) Host Pipe Clear Register (n = 0) 8 */
#define REG_USBHS_HSTPIPICR9    (0x40038584) /**< (USBHS) Host Pipe Clear Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIFR     (0x40038590) /**< (USBHS) Host Pipe Set Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIFR0    (0x40038590) /**< (USBHS) Host Pipe Set Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIFR1    (0x40038594) /**< (USBHS) Host Pipe Set Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIFR2    (0x40038598) /**< (USBHS) Host Pipe Set Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIFR3    (0x4003859C) /**< (USBHS) Host Pipe Set Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIFR4    (0x400385A0) /**< (USBHS) Host Pipe Set Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIFR5    (0x400385A4) /**< (USBHS) Host Pipe Set Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIFR6    (0x400385A8) /**< (USBHS) Host Pipe Set Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIFR7    (0x400385AC) /**< (USBHS) Host Pipe Set Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIFR8    (0x400385B0) /**< (USBHS) Host Pipe Set Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIFR9    (0x400385B4) /**< (USBHS) Host Pipe Set Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIMR     (0x400385C0) /**< (USBHS) Host Pipe Mask Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIMR0    (0x400385C0) /**< (USBHS) Host Pipe Mask Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIMR1    (0x400385C4) /**< (USBHS) Host Pipe Mask Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIMR2    (0x400385C8) /**< (USBHS) Host Pipe Mask Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIMR3    (0x400385CC) /**< (USBHS) Host Pipe Mask Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIMR4    (0x400385D0) /**< (USBHS) Host Pipe Mask Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIMR5    (0x400385D4) /**< (USBHS) Host Pipe Mask Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIMR6    (0x400385D8) /**< (USBHS) Host Pipe Mask Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIMR7    (0x400385DC) /**< (USBHS) Host Pipe Mask Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIMR8    (0x400385E0) /**< (USBHS) Host Pipe Mask Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIMR9    (0x400385E4) /**< (USBHS) Host Pipe Mask Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIER     (0x400385F0) /**< (USBHS) Host Pipe Enable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIER0    (0x400385F0) /**< (USBHS) Host Pipe Enable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIER1    (0x400385F4) /**< (USBHS) Host Pipe Enable Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIER2    (0x400385F8) /**< (USBHS) Host Pipe Enable Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIER3    (0x400385FC) /**< (USBHS) Host Pipe Enable Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIER4    (0x40038600) /**< (USBHS) Host Pipe Enable Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIER5    (0x40038604) /**< (USBHS) Host Pipe Enable Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIER6    (0x40038608) /**< (USBHS) Host Pipe Enable Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIER7    (0x4003860C) /**< (USBHS) Host Pipe Enable Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIER8    (0x40038610) /**< (USBHS) Host Pipe Enable Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIER9    (0x40038614) /**< (USBHS) Host Pipe Enable Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIDR     (0x40038620) /**< (USBHS) Host Pipe Disable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIDR0    (0x40038620) /**< (USBHS) Host Pipe Disable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIDR1    (0x40038624) /**< (USBHS) Host Pipe Disable Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIDR2    (0x40038628) /**< (USBHS) Host Pipe Disable Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIDR3    (0x4003862C) /**< (USBHS) Host Pipe Disable Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIDR4    (0x40038630) /**< (USBHS) Host Pipe Disable Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIDR5    (0x40038634) /**< (USBHS) Host Pipe Disable Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIDR6    (0x40038638) /**< (USBHS) Host Pipe Disable Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIDR7    (0x4003863C) /**< (USBHS) Host Pipe Disable Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIDR8    (0x40038640) /**< (USBHS) Host Pipe Disable Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIDR9    (0x40038644) /**< (USBHS) Host Pipe Disable Register (n = 0) 9 */
#define REG_USBHS_HSTPIPINRQ    (0x40038650) /**< (USBHS) Host Pipe IN Request Register (n = 0) 0 */
#define REG_USBHS_HSTPIPINRQ0   (0x40038650) /**< (USBHS) Host Pipe IN Request Register (n = 0) 0 */
#define REG_USBHS_HSTPIPINRQ1   (0x40038654) /**< (USBHS) Host Pipe IN Request Register (n = 0) 1 */
#define REG_USBHS_HSTPIPINRQ2   (0x40038658) /**< (USBHS) Host Pipe IN Request Register (n = 0) 2 */
#define REG_USBHS_HSTPIPINRQ3   (0x4003865C) /**< (USBHS) Host Pipe IN Request Register (n = 0) 3 */
#define REG_USBHS_HSTPIPINRQ4   (0x40038660) /**< (USBHS) Host Pipe IN Request Register (n = 0) 4 */
#define REG_USBHS_HSTPIPINRQ5   (0x40038664) /**< (USBHS) Host Pipe IN Request Register (n = 0) 5 */
#define REG_USBHS_HSTPIPINRQ6   (0x40038668) /**< (USBHS) Host Pipe IN Request Register (n = 0) 6 */
#define REG_USBHS_HSTPIPINRQ7   (0x4003866C) /**< (USBHS) Host Pipe IN Request Register (n = 0) 7 */
#define REG_USBHS_HSTPIPINRQ8   (0x40038670) /**< (USBHS) Host Pipe IN Request Register (n = 0) 8 */
#define REG_USBHS_HSTPIPINRQ9   (0x40038674) /**< (USBHS) Host Pipe IN Request Register (n = 0) 9 */
#define REG_USBHS_HSTPIPERR     (0x40038680) /**< (USBHS) Host Pipe Error Register (n = 0) 0 */
#define REG_USBHS_HSTPIPERR0    (0x40038680) /**< (USBHS) Host Pipe Error Register (n = 0) 0 */
#define REG_USBHS_HSTPIPERR1    (0x40038684) /**< (USBHS) Host Pipe Error Register (n = 0) 1 */
#define REG_USBHS_HSTPIPERR2    (0x40038688) /**< (USBHS) Host Pipe Error Register (n = 0) 2 */
#define REG_USBHS_HSTPIPERR3    (0x4003868C) /**< (USBHS) Host Pipe Error Register (n = 0) 3 */
#define REG_USBHS_HSTPIPERR4    (0x40038690) /**< (USBHS) Host Pipe Error Register (n = 0) 4 */
#define REG_USBHS_HSTPIPERR5    (0x40038694) /**< (USBHS) Host Pipe Error Register (n = 0) 5 */
#define REG_USBHS_HSTPIPERR6    (0x40038698) /**< (USBHS) Host Pipe Error Register (n = 0) 6 */
#define REG_USBHS_HSTPIPERR7    (0x4003869C) /**< (USBHS) Host Pipe Error Register (n = 0) 7 */
#define REG_USBHS_HSTPIPERR8    (0x400386A0) /**< (USBHS) Host Pipe Error Register (n = 0) 8 */
#define REG_USBHS_HSTPIPERR9    (0x400386A4) /**< (USBHS) Host Pipe Error Register (n = 0) 9 */
#define REG_USBHS_CTRL          (0x40038800) /**< (USBHS) General Control Register */
#define REG_USBHS_SR            (0x40038804) /**< (USBHS) General Status Register */
#define REG_USBHS_SCR           (0x40038808) /**< (USBHS) General Status Clear Register */
#define REG_USBHS_SFR           (0x4003880C) /**< (USBHS) General Status Set Register */

#else

#define REG_USBHS_DEVDMANXTDSC0 (*(__IO uint32_t*)0x40038310U) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 0 */
#define REG_USBHS_DEVDMAADDRESS0 (*(__IO uint32_t*)0x40038314U) /**< (USBHS) Device DMA Channel Address Register (n = 1) 0 */
#define REG_USBHS_DEVDMACONTROL0 (*(__IO uint32_t*)0x40038318U) /**< (USBHS) Device DMA Channel Control Register (n = 1) 0 */
#define REG_USBHS_DEVDMASTATUS0 (*(__IO uint32_t*)0x4003831CU) /**< (USBHS) Device DMA Channel Status Register (n = 1) 0 */
#define REG_USBHS_DEVDMANXTDSC1 (*(__IO uint32_t*)0x40038320U) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 1 */
#define REG_USBHS_DEVDMAADDRESS1 (*(__IO uint32_t*)0x40038324U) /**< (USBHS) Device DMA Channel Address Register (n = 1) 1 */
#define REG_USBHS_DEVDMACONTROL1 (*(__IO uint32_t*)0x40038328U) /**< (USBHS) Device DMA Channel Control Register (n = 1) 1 */
#define REG_USBHS_DEVDMASTATUS1 (*(__IO uint32_t*)0x4003832CU) /**< (USBHS) Device DMA Channel Status Register (n = 1) 1 */
#define REG_USBHS_DEVDMANXTDSC2 (*(__IO uint32_t*)0x40038330U) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 2 */
#define REG_USBHS_DEVDMAADDRESS2 (*(__IO uint32_t*)0x40038334U) /**< (USBHS) Device DMA Channel Address Register (n = 1) 2 */
#define REG_USBHS_DEVDMACONTROL2 (*(__IO uint32_t*)0x40038338U) /**< (USBHS) Device DMA Channel Control Register (n = 1) 2 */
#define REG_USBHS_DEVDMASTATUS2 (*(__IO uint32_t*)0x4003833CU) /**< (USBHS) Device DMA Channel Status Register (n = 1) 2 */
#define REG_USBHS_DEVDMANXTDSC3 (*(__IO uint32_t*)0x40038340U) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 3 */
#define REG_USBHS_DEVDMAADDRESS3 (*(__IO uint32_t*)0x40038344U) /**< (USBHS) Device DMA Channel Address Register (n = 1) 3 */
#define REG_USBHS_DEVDMACONTROL3 (*(__IO uint32_t*)0x40038348U) /**< (USBHS) Device DMA Channel Control Register (n = 1) 3 */
#define REG_USBHS_DEVDMASTATUS3 (*(__IO uint32_t*)0x4003834CU) /**< (USBHS) Device DMA Channel Status Register (n = 1) 3 */
#define REG_USBHS_DEVDMANXTDSC4 (*(__IO uint32_t*)0x40038350U) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 4 */
#define REG_USBHS_DEVDMAADDRESS4 (*(__IO uint32_t*)0x40038354U) /**< (USBHS) Device DMA Channel Address Register (n = 1) 4 */
#define REG_USBHS_DEVDMACONTROL4 (*(__IO uint32_t*)0x40038358U) /**< (USBHS) Device DMA Channel Control Register (n = 1) 4 */
#define REG_USBHS_DEVDMASTATUS4 (*(__IO uint32_t*)0x4003835CU) /**< (USBHS) Device DMA Channel Status Register (n = 1) 4 */
#define REG_USBHS_DEVDMANXTDSC5 (*(__IO uint32_t*)0x40038360U) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 5 */
#define REG_USBHS_DEVDMAADDRESS5 (*(__IO uint32_t*)0x40038364U) /**< (USBHS) Device DMA Channel Address Register (n = 1) 5 */
#define REG_USBHS_DEVDMACONTROL5 (*(__IO uint32_t*)0x40038368U) /**< (USBHS) Device DMA Channel Control Register (n = 1) 5 */
#define REG_USBHS_DEVDMASTATUS5 (*(__IO uint32_t*)0x4003836CU) /**< (USBHS) Device DMA Channel Status Register (n = 1) 5 */
#define REG_USBHS_DEVDMANXTDSC6 (*(__IO uint32_t*)0x40038370U) /**< (USBHS) Device DMA Channel Next Descriptor Address Register (n = 1) 6 */
#define REG_USBHS_DEVDMAADDRESS6 (*(__IO uint32_t*)0x40038374U) /**< (USBHS) Device DMA Channel Address Register (n = 1) 6 */
#define REG_USBHS_DEVDMACONTROL6 (*(__IO uint32_t*)0x40038378U) /**< (USBHS) Device DMA Channel Control Register (n = 1) 6 */
#define REG_USBHS_DEVDMASTATUS6 (*(__IO uint32_t*)0x4003837CU) /**< (USBHS) Device DMA Channel Status Register (n = 1) 6 */
#define REG_USBHS_HSTDMANXTDSC0 (*(__IO uint32_t*)0x40038710U) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 0 */
#define REG_USBHS_HSTDMAADDRESS0 (*(__IO uint32_t*)0x40038714U) /**< (USBHS) Host DMA Channel Address Register (n = 1) 0 */
#define REG_USBHS_HSTDMACONTROL0 (*(__IO uint32_t*)0x40038718U) /**< (USBHS) Host DMA Channel Control Register (n = 1) 0 */
#define REG_USBHS_HSTDMASTATUS0 (*(__IO uint32_t*)0x4003871CU) /**< (USBHS) Host DMA Channel Status Register (n = 1) 0 */
#define REG_USBHS_HSTDMANXTDSC1 (*(__IO uint32_t*)0x40038720U) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 1 */
#define REG_USBHS_HSTDMAADDRESS1 (*(__IO uint32_t*)0x40038724U) /**< (USBHS) Host DMA Channel Address Register (n = 1) 1 */
#define REG_USBHS_HSTDMACONTROL1 (*(__IO uint32_t*)0x40038728U) /**< (USBHS) Host DMA Channel Control Register (n = 1) 1 */
#define REG_USBHS_HSTDMASTATUS1 (*(__IO uint32_t*)0x4003872CU) /**< (USBHS) Host DMA Channel Status Register (n = 1) 1 */
#define REG_USBHS_HSTDMANXTDSC2 (*(__IO uint32_t*)0x40038730U) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 2 */
#define REG_USBHS_HSTDMAADDRESS2 (*(__IO uint32_t*)0x40038734U) /**< (USBHS) Host DMA Channel Address Register (n = 1) 2 */
#define REG_USBHS_HSTDMACONTROL2 (*(__IO uint32_t*)0x40038738U) /**< (USBHS) Host DMA Channel Control Register (n = 1) 2 */
#define REG_USBHS_HSTDMASTATUS2 (*(__IO uint32_t*)0x4003873CU) /**< (USBHS) Host DMA Channel Status Register (n = 1) 2 */
#define REG_USBHS_HSTDMANXTDSC3 (*(__IO uint32_t*)0x40038740U) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 3 */
#define REG_USBHS_HSTDMAADDRESS3 (*(__IO uint32_t*)0x40038744U) /**< (USBHS) Host DMA Channel Address Register (n = 1) 3 */
#define REG_USBHS_HSTDMACONTROL3 (*(__IO uint32_t*)0x40038748U) /**< (USBHS) Host DMA Channel Control Register (n = 1) 3 */
#define REG_USBHS_HSTDMASTATUS3 (*(__IO uint32_t*)0x4003874CU) /**< (USBHS) Host DMA Channel Status Register (n = 1) 3 */
#define REG_USBHS_HSTDMANXTDSC4 (*(__IO uint32_t*)0x40038750U) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 4 */
#define REG_USBHS_HSTDMAADDRESS4 (*(__IO uint32_t*)0x40038754U) /**< (USBHS) Host DMA Channel Address Register (n = 1) 4 */
#define REG_USBHS_HSTDMACONTROL4 (*(__IO uint32_t*)0x40038758U) /**< (USBHS) Host DMA Channel Control Register (n = 1) 4 */
#define REG_USBHS_HSTDMASTATUS4 (*(__IO uint32_t*)0x4003875CU) /**< (USBHS) Host DMA Channel Status Register (n = 1) 4 */
#define REG_USBHS_HSTDMANXTDSC5 (*(__IO uint32_t*)0x40038760U) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 5 */
#define REG_USBHS_HSTDMAADDRESS5 (*(__IO uint32_t*)0x40038764U) /**< (USBHS) Host DMA Channel Address Register (n = 1) 5 */
#define REG_USBHS_HSTDMACONTROL5 (*(__IO uint32_t*)0x40038768U) /**< (USBHS) Host DMA Channel Control Register (n = 1) 5 */
#define REG_USBHS_HSTDMASTATUS5 (*(__IO uint32_t*)0x4003876CU) /**< (USBHS) Host DMA Channel Status Register (n = 1) 5 */
#define REG_USBHS_HSTDMANXTDSC6 (*(__IO uint32_t*)0x40038770U) /**< (USBHS) Host DMA Channel Next Descriptor Address Register (n = 1) 6 */
#define REG_USBHS_HSTDMAADDRESS6 (*(__IO uint32_t*)0x40038774U) /**< (USBHS) Host DMA Channel Address Register (n = 1) 6 */
#define REG_USBHS_HSTDMACONTROL6 (*(__IO uint32_t*)0x40038778U) /**< (USBHS) Host DMA Channel Control Register (n = 1) 6 */
#define REG_USBHS_HSTDMASTATUS6 (*(__IO uint32_t*)0x4003877CU) /**< (USBHS) Host DMA Channel Status Register (n = 1) 6 */
#define REG_USBHS_DEVCTRL       (*(__IO uint32_t*)0x40038000U) /**< (USBHS) Device General Control Register */
#define REG_USBHS_DEVISR        (*(__I  uint32_t*)0x40038004U) /**< (USBHS) Device Global Interrupt Status Register */
#define REG_USBHS_DEVICR        (*(__O  uint32_t*)0x40038008U) /**< (USBHS) Device Global Interrupt Clear Register */
#define REG_USBHS_DEVIFR        (*(__O  uint32_t*)0x4003800CU) /**< (USBHS) Device Global Interrupt Set Register */
#define REG_USBHS_DEVIMR        (*(__I  uint32_t*)0x40038010U) /**< (USBHS) Device Global Interrupt Mask Register */
#define REG_USBHS_DEVIDR        (*(__O  uint32_t*)0x40038014U) /**< (USBHS) Device Global Interrupt Disable Register */
#define REG_USBHS_DEVIER        (*(__O  uint32_t*)0x40038018U) /**< (USBHS) Device Global Interrupt Enable Register */
#define REG_USBHS_DEVEPT        (*(__IO uint32_t*)0x4003801CU) /**< (USBHS) Device Endpoint Register */
#define REG_USBHS_DEVFNUM       (*(__I  uint32_t*)0x40038020U) /**< (USBHS) Device Frame Number Register */
#define REG_USBHS_DEVEPTCFG     (*(__IO uint32_t*)0x40038100U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 0 */
#define REG_USBHS_DEVEPTCFG0    (*(__IO uint32_t*)0x40038100U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 0 */
#define REG_USBHS_DEVEPTCFG1    (*(__IO uint32_t*)0x40038104U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 1 */
#define REG_USBHS_DEVEPTCFG2    (*(__IO uint32_t*)0x40038108U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 2 */
#define REG_USBHS_DEVEPTCFG3    (*(__IO uint32_t*)0x4003810CU) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 3 */
#define REG_USBHS_DEVEPTCFG4    (*(__IO uint32_t*)0x40038110U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 4 */
#define REG_USBHS_DEVEPTCFG5    (*(__IO uint32_t*)0x40038114U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 5 */
#define REG_USBHS_DEVEPTCFG6    (*(__IO uint32_t*)0x40038118U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 6 */
#define REG_USBHS_DEVEPTCFG7    (*(__IO uint32_t*)0x4003811CU) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 7 */
#define REG_USBHS_DEVEPTCFG8    (*(__IO uint32_t*)0x40038120U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 8 */
#define REG_USBHS_DEVEPTCFG9    (*(__IO uint32_t*)0x40038124U) /**< (USBHS) Device Endpoint Configuration Register (n = 0) 9 */
#define REG_USBHS_DEVEPTISR     (*(__I  uint32_t*)0x40038130U) /**< (USBHS) Device Endpoint Status Register (n = 0) 0 */
#define REG_USBHS_DEVEPTISR0    (*(__I  uint32_t*)0x40038130U) /**< (USBHS) Device Endpoint Status Register (n = 0) 0 */
#define REG_USBHS_DEVEPTISR1    (*(__I  uint32_t*)0x40038134U) /**< (USBHS) Device Endpoint Status Register (n = 0) 1 */
#define REG_USBHS_DEVEPTISR2    (*(__I  uint32_t*)0x40038138U) /**< (USBHS) Device Endpoint Status Register (n = 0) 2 */
#define REG_USBHS_DEVEPTISR3    (*(__I  uint32_t*)0x4003813CU) /**< (USBHS) Device Endpoint Status Register (n = 0) 3 */
#define REG_USBHS_DEVEPTISR4    (*(__I  uint32_t*)0x40038140U) /**< (USBHS) Device Endpoint Status Register (n = 0) 4 */
#define REG_USBHS_DEVEPTISR5    (*(__I  uint32_t*)0x40038144U) /**< (USBHS) Device Endpoint Status Register (n = 0) 5 */
#define REG_USBHS_DEVEPTISR6    (*(__I  uint32_t*)0x40038148U) /**< (USBHS) Device Endpoint Status Register (n = 0) 6 */
#define REG_USBHS_DEVEPTISR7    (*(__I  uint32_t*)0x4003814CU) /**< (USBHS) Device Endpoint Status Register (n = 0) 7 */
#define REG_USBHS_DEVEPTISR8    (*(__I  uint32_t*)0x40038150U) /**< (USBHS) Device Endpoint Status Register (n = 0) 8 */
#define REG_USBHS_DEVEPTISR9    (*(__I  uint32_t*)0x40038154U) /**< (USBHS) Device Endpoint Status Register (n = 0) 9 */
#define REG_USBHS_DEVEPTICR     (*(__O  uint32_t*)0x40038160U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 0 */
#define REG_USBHS_DEVEPTICR0    (*(__O  uint32_t*)0x40038160U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 0 */
#define REG_USBHS_DEVEPTICR1    (*(__O  uint32_t*)0x40038164U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 1 */
#define REG_USBHS_DEVEPTICR2    (*(__O  uint32_t*)0x40038168U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 2 */
#define REG_USBHS_DEVEPTICR3    (*(__O  uint32_t*)0x4003816CU) /**< (USBHS) Device Endpoint Clear Register (n = 0) 3 */
#define REG_USBHS_DEVEPTICR4    (*(__O  uint32_t*)0x40038170U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 4 */
#define REG_USBHS_DEVEPTICR5    (*(__O  uint32_t*)0x40038174U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 5 */
#define REG_USBHS_DEVEPTICR6    (*(__O  uint32_t*)0x40038178U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 6 */
#define REG_USBHS_DEVEPTICR7    (*(__O  uint32_t*)0x4003817CU) /**< (USBHS) Device Endpoint Clear Register (n = 0) 7 */
#define REG_USBHS_DEVEPTICR8    (*(__O  uint32_t*)0x40038180U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 8 */
#define REG_USBHS_DEVEPTICR9    (*(__O  uint32_t*)0x40038184U) /**< (USBHS) Device Endpoint Clear Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIFR     (*(__O  uint32_t*)0x40038190U) /**< (USBHS) Device Endpoint Set Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIFR0    (*(__O  uint32_t*)0x40038190U) /**< (USBHS) Device Endpoint Set Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIFR1    (*(__O  uint32_t*)0x40038194U) /**< (USBHS) Device Endpoint Set Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIFR2    (*(__O  uint32_t*)0x40038198U) /**< (USBHS) Device Endpoint Set Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIFR3    (*(__O  uint32_t*)0x4003819CU) /**< (USBHS) Device Endpoint Set Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIFR4    (*(__O  uint32_t*)0x400381A0U) /**< (USBHS) Device Endpoint Set Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIFR5    (*(__O  uint32_t*)0x400381A4U) /**< (USBHS) Device Endpoint Set Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIFR6    (*(__O  uint32_t*)0x400381A8U) /**< (USBHS) Device Endpoint Set Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIFR7    (*(__O  uint32_t*)0x400381ACU) /**< (USBHS) Device Endpoint Set Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIFR8    (*(__O  uint32_t*)0x400381B0U) /**< (USBHS) Device Endpoint Set Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIFR9    (*(__O  uint32_t*)0x400381B4U) /**< (USBHS) Device Endpoint Set Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIMR     (*(__I  uint32_t*)0x400381C0U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIMR0    (*(__I  uint32_t*)0x400381C0U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIMR1    (*(__I  uint32_t*)0x400381C4U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIMR2    (*(__I  uint32_t*)0x400381C8U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIMR3    (*(__I  uint32_t*)0x400381CCU) /**< (USBHS) Device Endpoint Mask Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIMR4    (*(__I  uint32_t*)0x400381D0U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIMR5    (*(__I  uint32_t*)0x400381D4U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIMR6    (*(__I  uint32_t*)0x400381D8U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIMR7    (*(__I  uint32_t*)0x400381DCU) /**< (USBHS) Device Endpoint Mask Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIMR8    (*(__I  uint32_t*)0x400381E0U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIMR9    (*(__I  uint32_t*)0x400381E4U) /**< (USBHS) Device Endpoint Mask Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIER     (*(__O  uint32_t*)0x400381F0U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIER0    (*(__O  uint32_t*)0x400381F0U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIER1    (*(__O  uint32_t*)0x400381F4U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIER2    (*(__O  uint32_t*)0x400381F8U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIER3    (*(__O  uint32_t*)0x400381FCU) /**< (USBHS) Device Endpoint Enable Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIER4    (*(__O  uint32_t*)0x40038200U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIER5    (*(__O  uint32_t*)0x40038204U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIER6    (*(__O  uint32_t*)0x40038208U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIER7    (*(__O  uint32_t*)0x4003820CU) /**< (USBHS) Device Endpoint Enable Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIER8    (*(__O  uint32_t*)0x40038210U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIER9    (*(__O  uint32_t*)0x40038214U) /**< (USBHS) Device Endpoint Enable Register (n = 0) 9 */
#define REG_USBHS_DEVEPTIDR     (*(__O  uint32_t*)0x40038220U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIDR0    (*(__O  uint32_t*)0x40038220U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 0 */
#define REG_USBHS_DEVEPTIDR1    (*(__O  uint32_t*)0x40038224U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 1 */
#define REG_USBHS_DEVEPTIDR2    (*(__O  uint32_t*)0x40038228U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 2 */
#define REG_USBHS_DEVEPTIDR3    (*(__O  uint32_t*)0x4003822CU) /**< (USBHS) Device Endpoint Disable Register (n = 0) 3 */
#define REG_USBHS_DEVEPTIDR4    (*(__O  uint32_t*)0x40038230U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 4 */
#define REG_USBHS_DEVEPTIDR5    (*(__O  uint32_t*)0x40038234U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 5 */
#define REG_USBHS_DEVEPTIDR6    (*(__O  uint32_t*)0x40038238U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 6 */
#define REG_USBHS_DEVEPTIDR7    (*(__O  uint32_t*)0x4003823CU) /**< (USBHS) Device Endpoint Disable Register (n = 0) 7 */
#define REG_USBHS_DEVEPTIDR8    (*(__O  uint32_t*)0x40038240U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 8 */
#define REG_USBHS_DEVEPTIDR9    (*(__O  uint32_t*)0x40038244U) /**< (USBHS) Device Endpoint Disable Register (n = 0) 9 */
#define REG_USBHS_HSTCTRL       (*(__IO uint32_t*)0x40038400U) /**< (USBHS) Host General Control Register */
#define REG_USBHS_HSTISR        (*(__I  uint32_t*)0x40038404U) /**< (USBHS) Host Global Interrupt Status Register */
#define REG_USBHS_HSTICR        (*(__O  uint32_t*)0x40038408U) /**< (USBHS) Host Global Interrupt Clear Register */
#define REG_USBHS_HSTIFR        (*(__O  uint32_t*)0x4003840CU) /**< (USBHS) Host Global Interrupt Set Register */
#define REG_USBHS_HSTIMR        (*(__I  uint32_t*)0x40038410U) /**< (USBHS) Host Global Interrupt Mask Register */
#define REG_USBHS_HSTIDR        (*(__O  uint32_t*)0x40038414U) /**< (USBHS) Host Global Interrupt Disable Register */
#define REG_USBHS_HSTIER        (*(__O  uint32_t*)0x40038418U) /**< (USBHS) Host Global Interrupt Enable Register */
#define REG_USBHS_HSTPIP        (*(__IO uint32_t*)0x4003841CU) /**< (USBHS) Host Pipe Register */
#define REG_USBHS_HSTFNUM       (*(__IO uint32_t*)0x40038420U) /**< (USBHS) Host Frame Number Register */
#define REG_USBHS_HSTADDR1      (*(__IO uint32_t*)0x40038424U) /**< (USBHS) Host Address 1 Register */
#define REG_USBHS_HSTADDR2      (*(__IO uint32_t*)0x40038428U) /**< (USBHS) Host Address 2 Register */
#define REG_USBHS_HSTADDR3      (*(__IO uint32_t*)0x4003842CU) /**< (USBHS) Host Address 3 Register */
#define REG_USBHS_HSTPIPCFG     (*(__IO uint32_t*)0x40038500U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 0 */
#define REG_USBHS_HSTPIPCFG0    (*(__IO uint32_t*)0x40038500U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 0 */
#define REG_USBHS_HSTPIPCFG1    (*(__IO uint32_t*)0x40038504U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 1 */
#define REG_USBHS_HSTPIPCFG2    (*(__IO uint32_t*)0x40038508U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 2 */
#define REG_USBHS_HSTPIPCFG3    (*(__IO uint32_t*)0x4003850CU) /**< (USBHS) Host Pipe Configuration Register (n = 0) 3 */
#define REG_USBHS_HSTPIPCFG4    (*(__IO uint32_t*)0x40038510U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 4 */
#define REG_USBHS_HSTPIPCFG5    (*(__IO uint32_t*)0x40038514U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 5 */
#define REG_USBHS_HSTPIPCFG6    (*(__IO uint32_t*)0x40038518U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 6 */
#define REG_USBHS_HSTPIPCFG7    (*(__IO uint32_t*)0x4003851CU) /**< (USBHS) Host Pipe Configuration Register (n = 0) 7 */
#define REG_USBHS_HSTPIPCFG8    (*(__IO uint32_t*)0x40038520U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 8 */
#define REG_USBHS_HSTPIPCFG9    (*(__IO uint32_t*)0x40038524U) /**< (USBHS) Host Pipe Configuration Register (n = 0) 9 */
#define REG_USBHS_HSTPIPISR     (*(__I  uint32_t*)0x40038530U) /**< (USBHS) Host Pipe Status Register (n = 0) 0 */
#define REG_USBHS_HSTPIPISR0    (*(__I  uint32_t*)0x40038530U) /**< (USBHS) Host Pipe Status Register (n = 0) 0 */
#define REG_USBHS_HSTPIPISR1    (*(__I  uint32_t*)0x40038534U) /**< (USBHS) Host Pipe Status Register (n = 0) 1 */
#define REG_USBHS_HSTPIPISR2    (*(__I  uint32_t*)0x40038538U) /**< (USBHS) Host Pipe Status Register (n = 0) 2 */
#define REG_USBHS_HSTPIPISR3    (*(__I  uint32_t*)0x4003853CU) /**< (USBHS) Host Pipe Status Register (n = 0) 3 */
#define REG_USBHS_HSTPIPISR4    (*(__I  uint32_t*)0x40038540U) /**< (USBHS) Host Pipe Status Register (n = 0) 4 */
#define REG_USBHS_HSTPIPISR5    (*(__I  uint32_t*)0x40038544U) /**< (USBHS) Host Pipe Status Register (n = 0) 5 */
#define REG_USBHS_HSTPIPISR6    (*(__I  uint32_t*)0x40038548U) /**< (USBHS) Host Pipe Status Register (n = 0) 6 */
#define REG_USBHS_HSTPIPISR7    (*(__I  uint32_t*)0x4003854CU) /**< (USBHS) Host Pipe Status Register (n = 0) 7 */
#define REG_USBHS_HSTPIPISR8    (*(__I  uint32_t*)0x40038550U) /**< (USBHS) Host Pipe Status Register (n = 0) 8 */
#define REG_USBHS_HSTPIPISR9    (*(__I  uint32_t*)0x40038554U) /**< (USBHS) Host Pipe Status Register (n = 0) 9 */
#define REG_USBHS_HSTPIPICR     (*(__O  uint32_t*)0x40038560U) /**< (USBHS) Host Pipe Clear Register (n = 0) 0 */
#define REG_USBHS_HSTPIPICR0    (*(__O  uint32_t*)0x40038560U) /**< (USBHS) Host Pipe Clear Register (n = 0) 0 */
#define REG_USBHS_HSTPIPICR1    (*(__O  uint32_t*)0x40038564U) /**< (USBHS) Host Pipe Clear Register (n = 0) 1 */
#define REG_USBHS_HSTPIPICR2    (*(__O  uint32_t*)0x40038568U) /**< (USBHS) Host Pipe Clear Register (n = 0) 2 */
#define REG_USBHS_HSTPIPICR3    (*(__O  uint32_t*)0x4003856CU) /**< (USBHS) Host Pipe Clear Register (n = 0) 3 */
#define REG_USBHS_HSTPIPICR4    (*(__O  uint32_t*)0x40038570U) /**< (USBHS) Host Pipe Clear Register (n = 0) 4 */
#define REG_USBHS_HSTPIPICR5    (*(__O  uint32_t*)0x40038574U) /**< (USBHS) Host Pipe Clear Register (n = 0) 5 */
#define REG_USBHS_HSTPIPICR6    (*(__O  uint32_t*)0x40038578U) /**< (USBHS) Host Pipe Clear Register (n = 0) 6 */
#define REG_USBHS_HSTPIPICR7    (*(__O  uint32_t*)0x4003857CU) /**< (USBHS) Host Pipe Clear Register (n = 0) 7 */
#define REG_USBHS_HSTPIPICR8    (*(__O  uint32_t*)0x40038580U) /**< (USBHS) Host Pipe Clear Register (n = 0) 8 */
#define REG_USBHS_HSTPIPICR9    (*(__O  uint32_t*)0x40038584U) /**< (USBHS) Host Pipe Clear Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIFR     (*(__O  uint32_t*)0x40038590U) /**< (USBHS) Host Pipe Set Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIFR0    (*(__O  uint32_t*)0x40038590U) /**< (USBHS) Host Pipe Set Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIFR1    (*(__O  uint32_t*)0x40038594U) /**< (USBHS) Host Pipe Set Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIFR2    (*(__O  uint32_t*)0x40038598U) /**< (USBHS) Host Pipe Set Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIFR3    (*(__O  uint32_t*)0x4003859CU) /**< (USBHS) Host Pipe Set Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIFR4    (*(__O  uint32_t*)0x400385A0U) /**< (USBHS) Host Pipe Set Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIFR5    (*(__O  uint32_t*)0x400385A4U) /**< (USBHS) Host Pipe Set Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIFR6    (*(__O  uint32_t*)0x400385A8U) /**< (USBHS) Host Pipe Set Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIFR7    (*(__O  uint32_t*)0x400385ACU) /**< (USBHS) Host Pipe Set Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIFR8    (*(__O  uint32_t*)0x400385B0U) /**< (USBHS) Host Pipe Set Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIFR9    (*(__O  uint32_t*)0x400385B4U) /**< (USBHS) Host Pipe Set Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIMR     (*(__I  uint32_t*)0x400385C0U) /**< (USBHS) Host Pipe Mask Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIMR0    (*(__I  uint32_t*)0x400385C0U) /**< (USBHS) Host Pipe Mask Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIMR1    (*(__I  uint32_t*)0x400385C4U) /**< (USBHS) Host Pipe Mask Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIMR2    (*(__I  uint32_t*)0x400385C8U) /**< (USBHS) Host Pipe Mask Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIMR3    (*(__I  uint32_t*)0x400385CCU) /**< (USBHS) Host Pipe Mask Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIMR4    (*(__I  uint32_t*)0x400385D0U) /**< (USBHS) Host Pipe Mask Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIMR5    (*(__I  uint32_t*)0x400385D4U) /**< (USBHS) Host Pipe Mask Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIMR6    (*(__I  uint32_t*)0x400385D8U) /**< (USBHS) Host Pipe Mask Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIMR7    (*(__I  uint32_t*)0x400385DCU) /**< (USBHS) Host Pipe Mask Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIMR8    (*(__I  uint32_t*)0x400385E0U) /**< (USBHS) Host Pipe Mask Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIMR9    (*(__I  uint32_t*)0x400385E4U) /**< (USBHS) Host Pipe Mask Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIER     (*(__O  uint32_t*)0x400385F0U) /**< (USBHS) Host Pipe Enable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIER0    (*(__O  uint32_t*)0x400385F0U) /**< (USBHS) Host Pipe Enable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIER1    (*(__O  uint32_t*)0x400385F4U) /**< (USBHS) Host Pipe Enable Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIER2    (*(__O  uint32_t*)0x400385F8U) /**< (USBHS) Host Pipe Enable Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIER3    (*(__O  uint32_t*)0x400385FCU) /**< (USBHS) Host Pipe Enable Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIER4    (*(__O  uint32_t*)0x40038600U) /**< (USBHS) Host Pipe Enable Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIER5    (*(__O  uint32_t*)0x40038604U) /**< (USBHS) Host Pipe Enable Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIER6    (*(__O  uint32_t*)0x40038608U) /**< (USBHS) Host Pipe Enable Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIER7    (*(__O  uint32_t*)0x4003860CU) /**< (USBHS) Host Pipe Enable Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIER8    (*(__O  uint32_t*)0x40038610U) /**< (USBHS) Host Pipe Enable Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIER9    (*(__O  uint32_t*)0x40038614U) /**< (USBHS) Host Pipe Enable Register (n = 0) 9 */
#define REG_USBHS_HSTPIPIDR     (*(__O  uint32_t*)0x40038620U) /**< (USBHS) Host Pipe Disable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIDR0    (*(__O  uint32_t*)0x40038620U) /**< (USBHS) Host Pipe Disable Register (n = 0) 0 */
#define REG_USBHS_HSTPIPIDR1    (*(__O  uint32_t*)0x40038624U) /**< (USBHS) Host Pipe Disable Register (n = 0) 1 */
#define REG_USBHS_HSTPIPIDR2    (*(__O  uint32_t*)0x40038628U) /**< (USBHS) Host Pipe Disable Register (n = 0) 2 */
#define REG_USBHS_HSTPIPIDR3    (*(__O  uint32_t*)0x4003862CU) /**< (USBHS) Host Pipe Disable Register (n = 0) 3 */
#define REG_USBHS_HSTPIPIDR4    (*(__O  uint32_t*)0x40038630U) /**< (USBHS) Host Pipe Disable Register (n = 0) 4 */
#define REG_USBHS_HSTPIPIDR5    (*(__O  uint32_t*)0x40038634U) /**< (USBHS) Host Pipe Disable Register (n = 0) 5 */
#define REG_USBHS_HSTPIPIDR6    (*(__O  uint32_t*)0x40038638U) /**< (USBHS) Host Pipe Disable Register (n = 0) 6 */
#define REG_USBHS_HSTPIPIDR7    (*(__O  uint32_t*)0x4003863CU) /**< (USBHS) Host Pipe Disable Register (n = 0) 7 */
#define REG_USBHS_HSTPIPIDR8    (*(__O  uint32_t*)0x40038640U) /**< (USBHS) Host Pipe Disable Register (n = 0) 8 */
#define REG_USBHS_HSTPIPIDR9    (*(__O  uint32_t*)0x40038644U) /**< (USBHS) Host Pipe Disable Register (n = 0) 9 */
#define REG_USBHS_HSTPIPINRQ    (*(__IO uint32_t*)0x40038650U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 0 */
#define REG_USBHS_HSTPIPINRQ0   (*(__IO uint32_t*)0x40038650U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 0 */
#define REG_USBHS_HSTPIPINRQ1   (*(__IO uint32_t*)0x40038654U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 1 */
#define REG_USBHS_HSTPIPINRQ2   (*(__IO uint32_t*)0x40038658U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 2 */
#define REG_USBHS_HSTPIPINRQ3   (*(__IO uint32_t*)0x4003865CU) /**< (USBHS) Host Pipe IN Request Register (n = 0) 3 */
#define REG_USBHS_HSTPIPINRQ4   (*(__IO uint32_t*)0x40038660U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 4 */
#define REG_USBHS_HSTPIPINRQ5   (*(__IO uint32_t*)0x40038664U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 5 */
#define REG_USBHS_HSTPIPINRQ6   (*(__IO uint32_t*)0x40038668U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 6 */
#define REG_USBHS_HSTPIPINRQ7   (*(__IO uint32_t*)0x4003866CU) /**< (USBHS) Host Pipe IN Request Register (n = 0) 7 */
#define REG_USBHS_HSTPIPINRQ8   (*(__IO uint32_t*)0x40038670U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 8 */
#define REG_USBHS_HSTPIPINRQ9   (*(__IO uint32_t*)0x40038674U) /**< (USBHS) Host Pipe IN Request Register (n = 0) 9 */
#define REG_USBHS_HSTPIPERR     (*(__IO uint32_t*)0x40038680U) /**< (USBHS) Host Pipe Error Register (n = 0) 0 */
#define REG_USBHS_HSTPIPERR0    (*(__IO uint32_t*)0x40038680U) /**< (USBHS) Host Pipe Error Register (n = 0) 0 */
#define REG_USBHS_HSTPIPERR1    (*(__IO uint32_t*)0x40038684U) /**< (USBHS) Host Pipe Error Register (n = 0) 1 */
#define REG_USBHS_HSTPIPERR2    (*(__IO uint32_t*)0x40038688U) /**< (USBHS) Host Pipe Error Register (n = 0) 2 */
#define REG_USBHS_HSTPIPERR3    (*(__IO uint32_t*)0x4003868CU) /**< (USBHS) Host Pipe Error Register (n = 0) 3 */
#define REG_USBHS_HSTPIPERR4    (*(__IO uint32_t*)0x40038690U) /**< (USBHS) Host Pipe Error Register (n = 0) 4 */
#define REG_USBHS_HSTPIPERR5    (*(__IO uint32_t*)0x40038694U) /**< (USBHS) Host Pipe Error Register (n = 0) 5 */
#define REG_USBHS_HSTPIPERR6    (*(__IO uint32_t*)0x40038698U) /**< (USBHS) Host Pipe Error Register (n = 0) 6 */
#define REG_USBHS_HSTPIPERR7    (*(__IO uint32_t*)0x4003869CU) /**< (USBHS) Host Pipe Error Register (n = 0) 7 */
#define REG_USBHS_HSTPIPERR8    (*(__IO uint32_t*)0x400386A0U) /**< (USBHS) Host Pipe Error Register (n = 0) 8 */
#define REG_USBHS_HSTPIPERR9    (*(__IO uint32_t*)0x400386A4U) /**< (USBHS) Host Pipe Error Register (n = 0) 9 */
#define REG_USBHS_CTRL          (*(__IO uint32_t*)0x40038800U) /**< (USBHS) General Control Register */
#define REG_USBHS_SR            (*(__I  uint32_t*)0x40038804U) /**< (USBHS) General Status Register */
#define REG_USBHS_SCR           (*(__O  uint32_t*)0x40038808U) /**< (USBHS) General Status Clear Register */
#define REG_USBHS_SFR           (*(__O  uint32_t*)0x4003880CU) /**< (USBHS) General Status Set Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for USBHS peripheral ========== */
#define USBHS_INSTANCE_ID                        34        

#endif /* _SAME70_USBHS_INSTANCE_ */
