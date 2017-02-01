/**
 * \file
 *
 * \brief Instance description for XDMAC
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

#ifndef _SAME70_XDMAC_INSTANCE_H_
#define _SAME70_XDMAC_INSTANCE_H_

/* ========== Register definition for XDMAC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_XDMAC_CIE0          (0x40078050) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 0 */
#define REG_XDMAC_CID0          (0x40078054) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 0 */
#define REG_XDMAC_CIM0          (0x40078058) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 0 */
#define REG_XDMAC_CIS0          (0x4007805C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 0 */
#define REG_XDMAC_CSA0          (0x40078060) /**< (XDMAC) Channel Source Address Register (chid = 0) 0 */
#define REG_XDMAC_CDA0          (0x40078064) /**< (XDMAC) Channel Destination Address Register (chid = 0) 0 */
#define REG_XDMAC_CNDA0         (0x40078068) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 0 */
#define REG_XDMAC_CNDC0         (0x4007806C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 0 */
#define REG_XDMAC_CUBC0         (0x40078070) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 0 */
#define REG_XDMAC_CBC0          (0x40078074) /**< (XDMAC) Channel Block Control Register (chid = 0) 0 */
#define REG_XDMAC_CC0           (0x40078078) /**< (XDMAC) Channel Configuration Register (chid = 0) 0 */
#define REG_XDMAC_CDS_MSP0      (0x4007807C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 0 */
#define REG_XDMAC_CSUS0         (0x40078080) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 0 */
#define REG_XDMAC_CDUS0         (0x40078084) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 0 */
#define REG_XDMAC_CIE1          (0x40078090) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 1 */
#define REG_XDMAC_CID1          (0x40078094) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 1 */
#define REG_XDMAC_CIM1          (0x40078098) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 1 */
#define REG_XDMAC_CIS1          (0x4007809C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 1 */
#define REG_XDMAC_CSA1          (0x400780A0) /**< (XDMAC) Channel Source Address Register (chid = 0) 1 */
#define REG_XDMAC_CDA1          (0x400780A4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 1 */
#define REG_XDMAC_CNDA1         (0x400780A8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 1 */
#define REG_XDMAC_CNDC1         (0x400780AC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 1 */
#define REG_XDMAC_CUBC1         (0x400780B0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 1 */
#define REG_XDMAC_CBC1          (0x400780B4) /**< (XDMAC) Channel Block Control Register (chid = 0) 1 */
#define REG_XDMAC_CC1           (0x400780B8) /**< (XDMAC) Channel Configuration Register (chid = 0) 1 */
#define REG_XDMAC_CDS_MSP1      (0x400780BC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 1 */
#define REG_XDMAC_CSUS1         (0x400780C0) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 1 */
#define REG_XDMAC_CDUS1         (0x400780C4) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 1 */
#define REG_XDMAC_CIE2          (0x400780D0) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 2 */
#define REG_XDMAC_CID2          (0x400780D4) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 2 */
#define REG_XDMAC_CIM2          (0x400780D8) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 2 */
#define REG_XDMAC_CIS2          (0x400780DC) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 2 */
#define REG_XDMAC_CSA2          (0x400780E0) /**< (XDMAC) Channel Source Address Register (chid = 0) 2 */
#define REG_XDMAC_CDA2          (0x400780E4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 2 */
#define REG_XDMAC_CNDA2         (0x400780E8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 2 */
#define REG_XDMAC_CNDC2         (0x400780EC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 2 */
#define REG_XDMAC_CUBC2         (0x400780F0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 2 */
#define REG_XDMAC_CBC2          (0x400780F4) /**< (XDMAC) Channel Block Control Register (chid = 0) 2 */
#define REG_XDMAC_CC2           (0x400780F8) /**< (XDMAC) Channel Configuration Register (chid = 0) 2 */
#define REG_XDMAC_CDS_MSP2      (0x400780FC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 2 */
#define REG_XDMAC_CSUS2         (0x40078100) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 2 */
#define REG_XDMAC_CDUS2         (0x40078104) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 2 */
#define REG_XDMAC_CIE3          (0x40078110) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 3 */
#define REG_XDMAC_CID3          (0x40078114) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 3 */
#define REG_XDMAC_CIM3          (0x40078118) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 3 */
#define REG_XDMAC_CIS3          (0x4007811C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 3 */
#define REG_XDMAC_CSA3          (0x40078120) /**< (XDMAC) Channel Source Address Register (chid = 0) 3 */
#define REG_XDMAC_CDA3          (0x40078124) /**< (XDMAC) Channel Destination Address Register (chid = 0) 3 */
#define REG_XDMAC_CNDA3         (0x40078128) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 3 */
#define REG_XDMAC_CNDC3         (0x4007812C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 3 */
#define REG_XDMAC_CUBC3         (0x40078130) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 3 */
#define REG_XDMAC_CBC3          (0x40078134) /**< (XDMAC) Channel Block Control Register (chid = 0) 3 */
#define REG_XDMAC_CC3           (0x40078138) /**< (XDMAC) Channel Configuration Register (chid = 0) 3 */
#define REG_XDMAC_CDS_MSP3      (0x4007813C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 3 */
#define REG_XDMAC_CSUS3         (0x40078140) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 3 */
#define REG_XDMAC_CDUS3         (0x40078144) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 3 */
#define REG_XDMAC_CIE4          (0x40078150) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 4 */
#define REG_XDMAC_CID4          (0x40078154) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 4 */
#define REG_XDMAC_CIM4          (0x40078158) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 4 */
#define REG_XDMAC_CIS4          (0x4007815C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 4 */
#define REG_XDMAC_CSA4          (0x40078160) /**< (XDMAC) Channel Source Address Register (chid = 0) 4 */
#define REG_XDMAC_CDA4          (0x40078164) /**< (XDMAC) Channel Destination Address Register (chid = 0) 4 */
#define REG_XDMAC_CNDA4         (0x40078168) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 4 */
#define REG_XDMAC_CNDC4         (0x4007816C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 4 */
#define REG_XDMAC_CUBC4         (0x40078170) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 4 */
#define REG_XDMAC_CBC4          (0x40078174) /**< (XDMAC) Channel Block Control Register (chid = 0) 4 */
#define REG_XDMAC_CC4           (0x40078178) /**< (XDMAC) Channel Configuration Register (chid = 0) 4 */
#define REG_XDMAC_CDS_MSP4      (0x4007817C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 4 */
#define REG_XDMAC_CSUS4         (0x40078180) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 4 */
#define REG_XDMAC_CDUS4         (0x40078184) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 4 */
#define REG_XDMAC_CIE5          (0x40078190) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 5 */
#define REG_XDMAC_CID5          (0x40078194) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 5 */
#define REG_XDMAC_CIM5          (0x40078198) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 5 */
#define REG_XDMAC_CIS5          (0x4007819C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 5 */
#define REG_XDMAC_CSA5          (0x400781A0) /**< (XDMAC) Channel Source Address Register (chid = 0) 5 */
#define REG_XDMAC_CDA5          (0x400781A4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 5 */
#define REG_XDMAC_CNDA5         (0x400781A8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 5 */
#define REG_XDMAC_CNDC5         (0x400781AC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 5 */
#define REG_XDMAC_CUBC5         (0x400781B0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 5 */
#define REG_XDMAC_CBC5          (0x400781B4) /**< (XDMAC) Channel Block Control Register (chid = 0) 5 */
#define REG_XDMAC_CC5           (0x400781B8) /**< (XDMAC) Channel Configuration Register (chid = 0) 5 */
#define REG_XDMAC_CDS_MSP5      (0x400781BC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 5 */
#define REG_XDMAC_CSUS5         (0x400781C0) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 5 */
#define REG_XDMAC_CDUS5         (0x400781C4) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 5 */
#define REG_XDMAC_CIE6          (0x400781D0) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 6 */
#define REG_XDMAC_CID6          (0x400781D4) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 6 */
#define REG_XDMAC_CIM6          (0x400781D8) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 6 */
#define REG_XDMAC_CIS6          (0x400781DC) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 6 */
#define REG_XDMAC_CSA6          (0x400781E0) /**< (XDMAC) Channel Source Address Register (chid = 0) 6 */
#define REG_XDMAC_CDA6          (0x400781E4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 6 */
#define REG_XDMAC_CNDA6         (0x400781E8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 6 */
#define REG_XDMAC_CNDC6         (0x400781EC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 6 */
#define REG_XDMAC_CUBC6         (0x400781F0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 6 */
#define REG_XDMAC_CBC6          (0x400781F4) /**< (XDMAC) Channel Block Control Register (chid = 0) 6 */
#define REG_XDMAC_CC6           (0x400781F8) /**< (XDMAC) Channel Configuration Register (chid = 0) 6 */
#define REG_XDMAC_CDS_MSP6      (0x400781FC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 6 */
#define REG_XDMAC_CSUS6         (0x40078200) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 6 */
#define REG_XDMAC_CDUS6         (0x40078204) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 6 */
#define REG_XDMAC_CIE7          (0x40078210) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 7 */
#define REG_XDMAC_CID7          (0x40078214) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 7 */
#define REG_XDMAC_CIM7          (0x40078218) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 7 */
#define REG_XDMAC_CIS7          (0x4007821C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 7 */
#define REG_XDMAC_CSA7          (0x40078220) /**< (XDMAC) Channel Source Address Register (chid = 0) 7 */
#define REG_XDMAC_CDA7          (0x40078224) /**< (XDMAC) Channel Destination Address Register (chid = 0) 7 */
#define REG_XDMAC_CNDA7         (0x40078228) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 7 */
#define REG_XDMAC_CNDC7         (0x4007822C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 7 */
#define REG_XDMAC_CUBC7         (0x40078230) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 7 */
#define REG_XDMAC_CBC7          (0x40078234) /**< (XDMAC) Channel Block Control Register (chid = 0) 7 */
#define REG_XDMAC_CC7           (0x40078238) /**< (XDMAC) Channel Configuration Register (chid = 0) 7 */
#define REG_XDMAC_CDS_MSP7      (0x4007823C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 7 */
#define REG_XDMAC_CSUS7         (0x40078240) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 7 */
#define REG_XDMAC_CDUS7         (0x40078244) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 7 */
#define REG_XDMAC_CIE8          (0x40078250) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 8 */
#define REG_XDMAC_CID8          (0x40078254) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 8 */
#define REG_XDMAC_CIM8          (0x40078258) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 8 */
#define REG_XDMAC_CIS8          (0x4007825C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 8 */
#define REG_XDMAC_CSA8          (0x40078260) /**< (XDMAC) Channel Source Address Register (chid = 0) 8 */
#define REG_XDMAC_CDA8          (0x40078264) /**< (XDMAC) Channel Destination Address Register (chid = 0) 8 */
#define REG_XDMAC_CNDA8         (0x40078268) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 8 */
#define REG_XDMAC_CNDC8         (0x4007826C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 8 */
#define REG_XDMAC_CUBC8         (0x40078270) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 8 */
#define REG_XDMAC_CBC8          (0x40078274) /**< (XDMAC) Channel Block Control Register (chid = 0) 8 */
#define REG_XDMAC_CC8           (0x40078278) /**< (XDMAC) Channel Configuration Register (chid = 0) 8 */
#define REG_XDMAC_CDS_MSP8      (0x4007827C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 8 */
#define REG_XDMAC_CSUS8         (0x40078280) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 8 */
#define REG_XDMAC_CDUS8         (0x40078284) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 8 */
#define REG_XDMAC_CIE9          (0x40078290) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 9 */
#define REG_XDMAC_CID9          (0x40078294) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 9 */
#define REG_XDMAC_CIM9          (0x40078298) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 9 */
#define REG_XDMAC_CIS9          (0x4007829C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 9 */
#define REG_XDMAC_CSA9          (0x400782A0) /**< (XDMAC) Channel Source Address Register (chid = 0) 9 */
#define REG_XDMAC_CDA9          (0x400782A4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 9 */
#define REG_XDMAC_CNDA9         (0x400782A8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 9 */
#define REG_XDMAC_CNDC9         (0x400782AC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 9 */
#define REG_XDMAC_CUBC9         (0x400782B0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 9 */
#define REG_XDMAC_CBC9          (0x400782B4) /**< (XDMAC) Channel Block Control Register (chid = 0) 9 */
#define REG_XDMAC_CC9           (0x400782B8) /**< (XDMAC) Channel Configuration Register (chid = 0) 9 */
#define REG_XDMAC_CDS_MSP9      (0x400782BC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 9 */
#define REG_XDMAC_CSUS9         (0x400782C0) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 9 */
#define REG_XDMAC_CDUS9         (0x400782C4) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 9 */
#define REG_XDMAC_CIE10         (0x400782D0) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 10 */
#define REG_XDMAC_CID10         (0x400782D4) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 10 */
#define REG_XDMAC_CIM10         (0x400782D8) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 10 */
#define REG_XDMAC_CIS10         (0x400782DC) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 10 */
#define REG_XDMAC_CSA10         (0x400782E0) /**< (XDMAC) Channel Source Address Register (chid = 0) 10 */
#define REG_XDMAC_CDA10         (0x400782E4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 10 */
#define REG_XDMAC_CNDA10        (0x400782E8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 10 */
#define REG_XDMAC_CNDC10        (0x400782EC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 10 */
#define REG_XDMAC_CUBC10        (0x400782F0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 10 */
#define REG_XDMAC_CBC10         (0x400782F4) /**< (XDMAC) Channel Block Control Register (chid = 0) 10 */
#define REG_XDMAC_CC10          (0x400782F8) /**< (XDMAC) Channel Configuration Register (chid = 0) 10 */
#define REG_XDMAC_CDS_MSP10     (0x400782FC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 10 */
#define REG_XDMAC_CSUS10        (0x40078300) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 10 */
#define REG_XDMAC_CDUS10        (0x40078304) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 10 */
#define REG_XDMAC_CIE11         (0x40078310) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 11 */
#define REG_XDMAC_CID11         (0x40078314) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 11 */
#define REG_XDMAC_CIM11         (0x40078318) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 11 */
#define REG_XDMAC_CIS11         (0x4007831C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 11 */
#define REG_XDMAC_CSA11         (0x40078320) /**< (XDMAC) Channel Source Address Register (chid = 0) 11 */
#define REG_XDMAC_CDA11         (0x40078324) /**< (XDMAC) Channel Destination Address Register (chid = 0) 11 */
#define REG_XDMAC_CNDA11        (0x40078328) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 11 */
#define REG_XDMAC_CNDC11        (0x4007832C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 11 */
#define REG_XDMAC_CUBC11        (0x40078330) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 11 */
#define REG_XDMAC_CBC11         (0x40078334) /**< (XDMAC) Channel Block Control Register (chid = 0) 11 */
#define REG_XDMAC_CC11          (0x40078338) /**< (XDMAC) Channel Configuration Register (chid = 0) 11 */
#define REG_XDMAC_CDS_MSP11     (0x4007833C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 11 */
#define REG_XDMAC_CSUS11        (0x40078340) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 11 */
#define REG_XDMAC_CDUS11        (0x40078344) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 11 */
#define REG_XDMAC_CIE12         (0x40078350) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 12 */
#define REG_XDMAC_CID12         (0x40078354) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 12 */
#define REG_XDMAC_CIM12         (0x40078358) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 12 */
#define REG_XDMAC_CIS12         (0x4007835C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 12 */
#define REG_XDMAC_CSA12         (0x40078360) /**< (XDMAC) Channel Source Address Register (chid = 0) 12 */
#define REG_XDMAC_CDA12         (0x40078364) /**< (XDMAC) Channel Destination Address Register (chid = 0) 12 */
#define REG_XDMAC_CNDA12        (0x40078368) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 12 */
#define REG_XDMAC_CNDC12        (0x4007836C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 12 */
#define REG_XDMAC_CUBC12        (0x40078370) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 12 */
#define REG_XDMAC_CBC12         (0x40078374) /**< (XDMAC) Channel Block Control Register (chid = 0) 12 */
#define REG_XDMAC_CC12          (0x40078378) /**< (XDMAC) Channel Configuration Register (chid = 0) 12 */
#define REG_XDMAC_CDS_MSP12     (0x4007837C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 12 */
#define REG_XDMAC_CSUS12        (0x40078380) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 12 */
#define REG_XDMAC_CDUS12        (0x40078384) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 12 */
#define REG_XDMAC_CIE13         (0x40078390) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 13 */
#define REG_XDMAC_CID13         (0x40078394) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 13 */
#define REG_XDMAC_CIM13         (0x40078398) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 13 */
#define REG_XDMAC_CIS13         (0x4007839C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 13 */
#define REG_XDMAC_CSA13         (0x400783A0) /**< (XDMAC) Channel Source Address Register (chid = 0) 13 */
#define REG_XDMAC_CDA13         (0x400783A4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 13 */
#define REG_XDMAC_CNDA13        (0x400783A8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 13 */
#define REG_XDMAC_CNDC13        (0x400783AC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 13 */
#define REG_XDMAC_CUBC13        (0x400783B0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 13 */
#define REG_XDMAC_CBC13         (0x400783B4) /**< (XDMAC) Channel Block Control Register (chid = 0) 13 */
#define REG_XDMAC_CC13          (0x400783B8) /**< (XDMAC) Channel Configuration Register (chid = 0) 13 */
#define REG_XDMAC_CDS_MSP13     (0x400783BC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 13 */
#define REG_XDMAC_CSUS13        (0x400783C0) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 13 */
#define REG_XDMAC_CDUS13        (0x400783C4) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 13 */
#define REG_XDMAC_CIE14         (0x400783D0) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 14 */
#define REG_XDMAC_CID14         (0x400783D4) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 14 */
#define REG_XDMAC_CIM14         (0x400783D8) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 14 */
#define REG_XDMAC_CIS14         (0x400783DC) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 14 */
#define REG_XDMAC_CSA14         (0x400783E0) /**< (XDMAC) Channel Source Address Register (chid = 0) 14 */
#define REG_XDMAC_CDA14         (0x400783E4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 14 */
#define REG_XDMAC_CNDA14        (0x400783E8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 14 */
#define REG_XDMAC_CNDC14        (0x400783EC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 14 */
#define REG_XDMAC_CUBC14        (0x400783F0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 14 */
#define REG_XDMAC_CBC14         (0x400783F4) /**< (XDMAC) Channel Block Control Register (chid = 0) 14 */
#define REG_XDMAC_CC14          (0x400783F8) /**< (XDMAC) Channel Configuration Register (chid = 0) 14 */
#define REG_XDMAC_CDS_MSP14     (0x400783FC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 14 */
#define REG_XDMAC_CSUS14        (0x40078400) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 14 */
#define REG_XDMAC_CDUS14        (0x40078404) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 14 */
#define REG_XDMAC_CIE15         (0x40078410) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 15 */
#define REG_XDMAC_CID15         (0x40078414) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 15 */
#define REG_XDMAC_CIM15         (0x40078418) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 15 */
#define REG_XDMAC_CIS15         (0x4007841C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 15 */
#define REG_XDMAC_CSA15         (0x40078420) /**< (XDMAC) Channel Source Address Register (chid = 0) 15 */
#define REG_XDMAC_CDA15         (0x40078424) /**< (XDMAC) Channel Destination Address Register (chid = 0) 15 */
#define REG_XDMAC_CNDA15        (0x40078428) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 15 */
#define REG_XDMAC_CNDC15        (0x4007842C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 15 */
#define REG_XDMAC_CUBC15        (0x40078430) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 15 */
#define REG_XDMAC_CBC15         (0x40078434) /**< (XDMAC) Channel Block Control Register (chid = 0) 15 */
#define REG_XDMAC_CC15          (0x40078438) /**< (XDMAC) Channel Configuration Register (chid = 0) 15 */
#define REG_XDMAC_CDS_MSP15     (0x4007843C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 15 */
#define REG_XDMAC_CSUS15        (0x40078440) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 15 */
#define REG_XDMAC_CDUS15        (0x40078444) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 15 */
#define REG_XDMAC_CIE16         (0x40078450) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 16 */
#define REG_XDMAC_CID16         (0x40078454) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 16 */
#define REG_XDMAC_CIM16         (0x40078458) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 16 */
#define REG_XDMAC_CIS16         (0x4007845C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 16 */
#define REG_XDMAC_CSA16         (0x40078460) /**< (XDMAC) Channel Source Address Register (chid = 0) 16 */
#define REG_XDMAC_CDA16         (0x40078464) /**< (XDMAC) Channel Destination Address Register (chid = 0) 16 */
#define REG_XDMAC_CNDA16        (0x40078468) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 16 */
#define REG_XDMAC_CNDC16        (0x4007846C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 16 */
#define REG_XDMAC_CUBC16        (0x40078470) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 16 */
#define REG_XDMAC_CBC16         (0x40078474) /**< (XDMAC) Channel Block Control Register (chid = 0) 16 */
#define REG_XDMAC_CC16          (0x40078478) /**< (XDMAC) Channel Configuration Register (chid = 0) 16 */
#define REG_XDMAC_CDS_MSP16     (0x4007847C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 16 */
#define REG_XDMAC_CSUS16        (0x40078480) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 16 */
#define REG_XDMAC_CDUS16        (0x40078484) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 16 */
#define REG_XDMAC_CIE17         (0x40078490) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 17 */
#define REG_XDMAC_CID17         (0x40078494) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 17 */
#define REG_XDMAC_CIM17         (0x40078498) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 17 */
#define REG_XDMAC_CIS17         (0x4007849C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 17 */
#define REG_XDMAC_CSA17         (0x400784A0) /**< (XDMAC) Channel Source Address Register (chid = 0) 17 */
#define REG_XDMAC_CDA17         (0x400784A4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 17 */
#define REG_XDMAC_CNDA17        (0x400784A8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 17 */
#define REG_XDMAC_CNDC17        (0x400784AC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 17 */
#define REG_XDMAC_CUBC17        (0x400784B0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 17 */
#define REG_XDMAC_CBC17         (0x400784B4) /**< (XDMAC) Channel Block Control Register (chid = 0) 17 */
#define REG_XDMAC_CC17          (0x400784B8) /**< (XDMAC) Channel Configuration Register (chid = 0) 17 */
#define REG_XDMAC_CDS_MSP17     (0x400784BC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 17 */
#define REG_XDMAC_CSUS17        (0x400784C0) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 17 */
#define REG_XDMAC_CDUS17        (0x400784C4) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 17 */
#define REG_XDMAC_CIE18         (0x400784D0) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 18 */
#define REG_XDMAC_CID18         (0x400784D4) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 18 */
#define REG_XDMAC_CIM18         (0x400784D8) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 18 */
#define REG_XDMAC_CIS18         (0x400784DC) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 18 */
#define REG_XDMAC_CSA18         (0x400784E0) /**< (XDMAC) Channel Source Address Register (chid = 0) 18 */
#define REG_XDMAC_CDA18         (0x400784E4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 18 */
#define REG_XDMAC_CNDA18        (0x400784E8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 18 */
#define REG_XDMAC_CNDC18        (0x400784EC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 18 */
#define REG_XDMAC_CUBC18        (0x400784F0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 18 */
#define REG_XDMAC_CBC18         (0x400784F4) /**< (XDMAC) Channel Block Control Register (chid = 0) 18 */
#define REG_XDMAC_CC18          (0x400784F8) /**< (XDMAC) Channel Configuration Register (chid = 0) 18 */
#define REG_XDMAC_CDS_MSP18     (0x400784FC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 18 */
#define REG_XDMAC_CSUS18        (0x40078500) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 18 */
#define REG_XDMAC_CDUS18        (0x40078504) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 18 */
#define REG_XDMAC_CIE19         (0x40078510) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 19 */
#define REG_XDMAC_CID19         (0x40078514) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 19 */
#define REG_XDMAC_CIM19         (0x40078518) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 19 */
#define REG_XDMAC_CIS19         (0x4007851C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 19 */
#define REG_XDMAC_CSA19         (0x40078520) /**< (XDMAC) Channel Source Address Register (chid = 0) 19 */
#define REG_XDMAC_CDA19         (0x40078524) /**< (XDMAC) Channel Destination Address Register (chid = 0) 19 */
#define REG_XDMAC_CNDA19        (0x40078528) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 19 */
#define REG_XDMAC_CNDC19        (0x4007852C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 19 */
#define REG_XDMAC_CUBC19        (0x40078530) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 19 */
#define REG_XDMAC_CBC19         (0x40078534) /**< (XDMAC) Channel Block Control Register (chid = 0) 19 */
#define REG_XDMAC_CC19          (0x40078538) /**< (XDMAC) Channel Configuration Register (chid = 0) 19 */
#define REG_XDMAC_CDS_MSP19     (0x4007853C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 19 */
#define REG_XDMAC_CSUS19        (0x40078540) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 19 */
#define REG_XDMAC_CDUS19        (0x40078544) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 19 */
#define REG_XDMAC_CIE20         (0x40078550) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 20 */
#define REG_XDMAC_CID20         (0x40078554) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 20 */
#define REG_XDMAC_CIM20         (0x40078558) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 20 */
#define REG_XDMAC_CIS20         (0x4007855C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 20 */
#define REG_XDMAC_CSA20         (0x40078560) /**< (XDMAC) Channel Source Address Register (chid = 0) 20 */
#define REG_XDMAC_CDA20         (0x40078564) /**< (XDMAC) Channel Destination Address Register (chid = 0) 20 */
#define REG_XDMAC_CNDA20        (0x40078568) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 20 */
#define REG_XDMAC_CNDC20        (0x4007856C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 20 */
#define REG_XDMAC_CUBC20        (0x40078570) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 20 */
#define REG_XDMAC_CBC20         (0x40078574) /**< (XDMAC) Channel Block Control Register (chid = 0) 20 */
#define REG_XDMAC_CC20          (0x40078578) /**< (XDMAC) Channel Configuration Register (chid = 0) 20 */
#define REG_XDMAC_CDS_MSP20     (0x4007857C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 20 */
#define REG_XDMAC_CSUS20        (0x40078580) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 20 */
#define REG_XDMAC_CDUS20        (0x40078584) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 20 */
#define REG_XDMAC_CIE21         (0x40078590) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 21 */
#define REG_XDMAC_CID21         (0x40078594) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 21 */
#define REG_XDMAC_CIM21         (0x40078598) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 21 */
#define REG_XDMAC_CIS21         (0x4007859C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 21 */
#define REG_XDMAC_CSA21         (0x400785A0) /**< (XDMAC) Channel Source Address Register (chid = 0) 21 */
#define REG_XDMAC_CDA21         (0x400785A4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 21 */
#define REG_XDMAC_CNDA21        (0x400785A8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 21 */
#define REG_XDMAC_CNDC21        (0x400785AC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 21 */
#define REG_XDMAC_CUBC21        (0x400785B0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 21 */
#define REG_XDMAC_CBC21         (0x400785B4) /**< (XDMAC) Channel Block Control Register (chid = 0) 21 */
#define REG_XDMAC_CC21          (0x400785B8) /**< (XDMAC) Channel Configuration Register (chid = 0) 21 */
#define REG_XDMAC_CDS_MSP21     (0x400785BC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 21 */
#define REG_XDMAC_CSUS21        (0x400785C0) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 21 */
#define REG_XDMAC_CDUS21        (0x400785C4) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 21 */
#define REG_XDMAC_CIE22         (0x400785D0) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 22 */
#define REG_XDMAC_CID22         (0x400785D4) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 22 */
#define REG_XDMAC_CIM22         (0x400785D8) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 22 */
#define REG_XDMAC_CIS22         (0x400785DC) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 22 */
#define REG_XDMAC_CSA22         (0x400785E0) /**< (XDMAC) Channel Source Address Register (chid = 0) 22 */
#define REG_XDMAC_CDA22         (0x400785E4) /**< (XDMAC) Channel Destination Address Register (chid = 0) 22 */
#define REG_XDMAC_CNDA22        (0x400785E8) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 22 */
#define REG_XDMAC_CNDC22        (0x400785EC) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 22 */
#define REG_XDMAC_CUBC22        (0x400785F0) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 22 */
#define REG_XDMAC_CBC22         (0x400785F4) /**< (XDMAC) Channel Block Control Register (chid = 0) 22 */
#define REG_XDMAC_CC22          (0x400785F8) /**< (XDMAC) Channel Configuration Register (chid = 0) 22 */
#define REG_XDMAC_CDS_MSP22     (0x400785FC) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 22 */
#define REG_XDMAC_CSUS22        (0x40078600) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 22 */
#define REG_XDMAC_CDUS22        (0x40078604) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 22 */
#define REG_XDMAC_CIE23         (0x40078610) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 23 */
#define REG_XDMAC_CID23         (0x40078614) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 23 */
#define REG_XDMAC_CIM23         (0x40078618) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 23 */
#define REG_XDMAC_CIS23         (0x4007861C) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 23 */
#define REG_XDMAC_CSA23         (0x40078620) /**< (XDMAC) Channel Source Address Register (chid = 0) 23 */
#define REG_XDMAC_CDA23         (0x40078624) /**< (XDMAC) Channel Destination Address Register (chid = 0) 23 */
#define REG_XDMAC_CNDA23        (0x40078628) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 23 */
#define REG_XDMAC_CNDC23        (0x4007862C) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 23 */
#define REG_XDMAC_CUBC23        (0x40078630) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 23 */
#define REG_XDMAC_CBC23         (0x40078634) /**< (XDMAC) Channel Block Control Register (chid = 0) 23 */
#define REG_XDMAC_CC23          (0x40078638) /**< (XDMAC) Channel Configuration Register (chid = 0) 23 */
#define REG_XDMAC_CDS_MSP23     (0x4007863C) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 23 */
#define REG_XDMAC_CSUS23        (0x40078640) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 23 */
#define REG_XDMAC_CDUS23        (0x40078644) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 23 */
#define REG_XDMAC_GTYPE         (0x40078000) /**< (XDMAC) Global Type Register */
#define REG_XDMAC_GCFG          (0x40078004) /**< (XDMAC) Global Configuration Register */
#define REG_XDMAC_GWAC          (0x40078008) /**< (XDMAC) Global Weighted Arbiter Configuration Register */
#define REG_XDMAC_GIE           (0x4007800C) /**< (XDMAC) Global Interrupt Enable Register */
#define REG_XDMAC_GID           (0x40078010) /**< (XDMAC) Global Interrupt Disable Register */
#define REG_XDMAC_GIM           (0x40078014) /**< (XDMAC) Global Interrupt Mask Register */
#define REG_XDMAC_GIS           (0x40078018) /**< (XDMAC) Global Interrupt Status Register */
#define REG_XDMAC_GE            (0x4007801C) /**< (XDMAC) Global Channel Enable Register */
#define REG_XDMAC_GD            (0x40078020) /**< (XDMAC) Global Channel Disable Register */
#define REG_XDMAC_GS            (0x40078024) /**< (XDMAC) Global Channel Status Register */
#define REG_XDMAC_GRS           (0x40078028) /**< (XDMAC) Global Channel Read Suspend Register */
#define REG_XDMAC_GWS           (0x4007802C) /**< (XDMAC) Global Channel Write Suspend Register */
#define REG_XDMAC_GRWS          (0x40078030) /**< (XDMAC) Global Channel Read Write Suspend Register */
#define REG_XDMAC_GRWR          (0x40078034) /**< (XDMAC) Global Channel Read Write Resume Register */
#define REG_XDMAC_GSWR          (0x40078038) /**< (XDMAC) Global Channel Software Request Register */
#define REG_XDMAC_GSWS          (0x4007803C) /**< (XDMAC) Global Channel Software Request Status Register */
#define REG_XDMAC_GSWF          (0x40078040) /**< (XDMAC) Global Channel Software Flush Request Register */

#else

#define REG_XDMAC_CIE0          (*(__O  uint32_t*)0x40078050U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 0 */
#define REG_XDMAC_CID0          (*(__O  uint32_t*)0x40078054U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 0 */
#define REG_XDMAC_CIM0          (*(__O  uint32_t*)0x40078058U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 0 */
#define REG_XDMAC_CIS0          (*(__I  uint32_t*)0x4007805CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 0 */
#define REG_XDMAC_CSA0          (*(__IO uint32_t*)0x40078060U) /**< (XDMAC) Channel Source Address Register (chid = 0) 0 */
#define REG_XDMAC_CDA0          (*(__IO uint32_t*)0x40078064U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 0 */
#define REG_XDMAC_CNDA0         (*(__IO uint32_t*)0x40078068U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 0 */
#define REG_XDMAC_CNDC0         (*(__IO uint32_t*)0x4007806CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 0 */
#define REG_XDMAC_CUBC0         (*(__IO uint32_t*)0x40078070U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 0 */
#define REG_XDMAC_CBC0          (*(__IO uint32_t*)0x40078074U) /**< (XDMAC) Channel Block Control Register (chid = 0) 0 */
#define REG_XDMAC_CC0           (*(__IO uint32_t*)0x40078078U) /**< (XDMAC) Channel Configuration Register (chid = 0) 0 */
#define REG_XDMAC_CDS_MSP0      (*(__IO uint32_t*)0x4007807CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 0 */
#define REG_XDMAC_CSUS0         (*(__IO uint32_t*)0x40078080U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 0 */
#define REG_XDMAC_CDUS0         (*(__IO uint32_t*)0x40078084U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 0 */
#define REG_XDMAC_CIE1          (*(__O  uint32_t*)0x40078090U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 1 */
#define REG_XDMAC_CID1          (*(__O  uint32_t*)0x40078094U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 1 */
#define REG_XDMAC_CIM1          (*(__O  uint32_t*)0x40078098U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 1 */
#define REG_XDMAC_CIS1          (*(__I  uint32_t*)0x4007809CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 1 */
#define REG_XDMAC_CSA1          (*(__IO uint32_t*)0x400780A0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 1 */
#define REG_XDMAC_CDA1          (*(__IO uint32_t*)0x400780A4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 1 */
#define REG_XDMAC_CNDA1         (*(__IO uint32_t*)0x400780A8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 1 */
#define REG_XDMAC_CNDC1         (*(__IO uint32_t*)0x400780ACU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 1 */
#define REG_XDMAC_CUBC1         (*(__IO uint32_t*)0x400780B0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 1 */
#define REG_XDMAC_CBC1          (*(__IO uint32_t*)0x400780B4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 1 */
#define REG_XDMAC_CC1           (*(__IO uint32_t*)0x400780B8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 1 */
#define REG_XDMAC_CDS_MSP1      (*(__IO uint32_t*)0x400780BCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 1 */
#define REG_XDMAC_CSUS1         (*(__IO uint32_t*)0x400780C0U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 1 */
#define REG_XDMAC_CDUS1         (*(__IO uint32_t*)0x400780C4U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 1 */
#define REG_XDMAC_CIE2          (*(__O  uint32_t*)0x400780D0U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 2 */
#define REG_XDMAC_CID2          (*(__O  uint32_t*)0x400780D4U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 2 */
#define REG_XDMAC_CIM2          (*(__O  uint32_t*)0x400780D8U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 2 */
#define REG_XDMAC_CIS2          (*(__I  uint32_t*)0x400780DCU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 2 */
#define REG_XDMAC_CSA2          (*(__IO uint32_t*)0x400780E0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 2 */
#define REG_XDMAC_CDA2          (*(__IO uint32_t*)0x400780E4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 2 */
#define REG_XDMAC_CNDA2         (*(__IO uint32_t*)0x400780E8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 2 */
#define REG_XDMAC_CNDC2         (*(__IO uint32_t*)0x400780ECU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 2 */
#define REG_XDMAC_CUBC2         (*(__IO uint32_t*)0x400780F0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 2 */
#define REG_XDMAC_CBC2          (*(__IO uint32_t*)0x400780F4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 2 */
#define REG_XDMAC_CC2           (*(__IO uint32_t*)0x400780F8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 2 */
#define REG_XDMAC_CDS_MSP2      (*(__IO uint32_t*)0x400780FCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 2 */
#define REG_XDMAC_CSUS2         (*(__IO uint32_t*)0x40078100U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 2 */
#define REG_XDMAC_CDUS2         (*(__IO uint32_t*)0x40078104U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 2 */
#define REG_XDMAC_CIE3          (*(__O  uint32_t*)0x40078110U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 3 */
#define REG_XDMAC_CID3          (*(__O  uint32_t*)0x40078114U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 3 */
#define REG_XDMAC_CIM3          (*(__O  uint32_t*)0x40078118U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 3 */
#define REG_XDMAC_CIS3          (*(__I  uint32_t*)0x4007811CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 3 */
#define REG_XDMAC_CSA3          (*(__IO uint32_t*)0x40078120U) /**< (XDMAC) Channel Source Address Register (chid = 0) 3 */
#define REG_XDMAC_CDA3          (*(__IO uint32_t*)0x40078124U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 3 */
#define REG_XDMAC_CNDA3         (*(__IO uint32_t*)0x40078128U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 3 */
#define REG_XDMAC_CNDC3         (*(__IO uint32_t*)0x4007812CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 3 */
#define REG_XDMAC_CUBC3         (*(__IO uint32_t*)0x40078130U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 3 */
#define REG_XDMAC_CBC3          (*(__IO uint32_t*)0x40078134U) /**< (XDMAC) Channel Block Control Register (chid = 0) 3 */
#define REG_XDMAC_CC3           (*(__IO uint32_t*)0x40078138U) /**< (XDMAC) Channel Configuration Register (chid = 0) 3 */
#define REG_XDMAC_CDS_MSP3      (*(__IO uint32_t*)0x4007813CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 3 */
#define REG_XDMAC_CSUS3         (*(__IO uint32_t*)0x40078140U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 3 */
#define REG_XDMAC_CDUS3         (*(__IO uint32_t*)0x40078144U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 3 */
#define REG_XDMAC_CIE4          (*(__O  uint32_t*)0x40078150U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 4 */
#define REG_XDMAC_CID4          (*(__O  uint32_t*)0x40078154U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 4 */
#define REG_XDMAC_CIM4          (*(__O  uint32_t*)0x40078158U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 4 */
#define REG_XDMAC_CIS4          (*(__I  uint32_t*)0x4007815CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 4 */
#define REG_XDMAC_CSA4          (*(__IO uint32_t*)0x40078160U) /**< (XDMAC) Channel Source Address Register (chid = 0) 4 */
#define REG_XDMAC_CDA4          (*(__IO uint32_t*)0x40078164U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 4 */
#define REG_XDMAC_CNDA4         (*(__IO uint32_t*)0x40078168U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 4 */
#define REG_XDMAC_CNDC4         (*(__IO uint32_t*)0x4007816CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 4 */
#define REG_XDMAC_CUBC4         (*(__IO uint32_t*)0x40078170U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 4 */
#define REG_XDMAC_CBC4          (*(__IO uint32_t*)0x40078174U) /**< (XDMAC) Channel Block Control Register (chid = 0) 4 */
#define REG_XDMAC_CC4           (*(__IO uint32_t*)0x40078178U) /**< (XDMAC) Channel Configuration Register (chid = 0) 4 */
#define REG_XDMAC_CDS_MSP4      (*(__IO uint32_t*)0x4007817CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 4 */
#define REG_XDMAC_CSUS4         (*(__IO uint32_t*)0x40078180U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 4 */
#define REG_XDMAC_CDUS4         (*(__IO uint32_t*)0x40078184U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 4 */
#define REG_XDMAC_CIE5          (*(__O  uint32_t*)0x40078190U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 5 */
#define REG_XDMAC_CID5          (*(__O  uint32_t*)0x40078194U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 5 */
#define REG_XDMAC_CIM5          (*(__O  uint32_t*)0x40078198U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 5 */
#define REG_XDMAC_CIS5          (*(__I  uint32_t*)0x4007819CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 5 */
#define REG_XDMAC_CSA5          (*(__IO uint32_t*)0x400781A0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 5 */
#define REG_XDMAC_CDA5          (*(__IO uint32_t*)0x400781A4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 5 */
#define REG_XDMAC_CNDA5         (*(__IO uint32_t*)0x400781A8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 5 */
#define REG_XDMAC_CNDC5         (*(__IO uint32_t*)0x400781ACU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 5 */
#define REG_XDMAC_CUBC5         (*(__IO uint32_t*)0x400781B0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 5 */
#define REG_XDMAC_CBC5          (*(__IO uint32_t*)0x400781B4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 5 */
#define REG_XDMAC_CC5           (*(__IO uint32_t*)0x400781B8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 5 */
#define REG_XDMAC_CDS_MSP5      (*(__IO uint32_t*)0x400781BCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 5 */
#define REG_XDMAC_CSUS5         (*(__IO uint32_t*)0x400781C0U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 5 */
#define REG_XDMAC_CDUS5         (*(__IO uint32_t*)0x400781C4U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 5 */
#define REG_XDMAC_CIE6          (*(__O  uint32_t*)0x400781D0U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 6 */
#define REG_XDMAC_CID6          (*(__O  uint32_t*)0x400781D4U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 6 */
#define REG_XDMAC_CIM6          (*(__O  uint32_t*)0x400781D8U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 6 */
#define REG_XDMAC_CIS6          (*(__I  uint32_t*)0x400781DCU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 6 */
#define REG_XDMAC_CSA6          (*(__IO uint32_t*)0x400781E0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 6 */
#define REG_XDMAC_CDA6          (*(__IO uint32_t*)0x400781E4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 6 */
#define REG_XDMAC_CNDA6         (*(__IO uint32_t*)0x400781E8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 6 */
#define REG_XDMAC_CNDC6         (*(__IO uint32_t*)0x400781ECU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 6 */
#define REG_XDMAC_CUBC6         (*(__IO uint32_t*)0x400781F0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 6 */
#define REG_XDMAC_CBC6          (*(__IO uint32_t*)0x400781F4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 6 */
#define REG_XDMAC_CC6           (*(__IO uint32_t*)0x400781F8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 6 */
#define REG_XDMAC_CDS_MSP6      (*(__IO uint32_t*)0x400781FCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 6 */
#define REG_XDMAC_CSUS6         (*(__IO uint32_t*)0x40078200U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 6 */
#define REG_XDMAC_CDUS6         (*(__IO uint32_t*)0x40078204U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 6 */
#define REG_XDMAC_CIE7          (*(__O  uint32_t*)0x40078210U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 7 */
#define REG_XDMAC_CID7          (*(__O  uint32_t*)0x40078214U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 7 */
#define REG_XDMAC_CIM7          (*(__O  uint32_t*)0x40078218U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 7 */
#define REG_XDMAC_CIS7          (*(__I  uint32_t*)0x4007821CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 7 */
#define REG_XDMAC_CSA7          (*(__IO uint32_t*)0x40078220U) /**< (XDMAC) Channel Source Address Register (chid = 0) 7 */
#define REG_XDMAC_CDA7          (*(__IO uint32_t*)0x40078224U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 7 */
#define REG_XDMAC_CNDA7         (*(__IO uint32_t*)0x40078228U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 7 */
#define REG_XDMAC_CNDC7         (*(__IO uint32_t*)0x4007822CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 7 */
#define REG_XDMAC_CUBC7         (*(__IO uint32_t*)0x40078230U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 7 */
#define REG_XDMAC_CBC7          (*(__IO uint32_t*)0x40078234U) /**< (XDMAC) Channel Block Control Register (chid = 0) 7 */
#define REG_XDMAC_CC7           (*(__IO uint32_t*)0x40078238U) /**< (XDMAC) Channel Configuration Register (chid = 0) 7 */
#define REG_XDMAC_CDS_MSP7      (*(__IO uint32_t*)0x4007823CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 7 */
#define REG_XDMAC_CSUS7         (*(__IO uint32_t*)0x40078240U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 7 */
#define REG_XDMAC_CDUS7         (*(__IO uint32_t*)0x40078244U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 7 */
#define REG_XDMAC_CIE8          (*(__O  uint32_t*)0x40078250U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 8 */
#define REG_XDMAC_CID8          (*(__O  uint32_t*)0x40078254U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 8 */
#define REG_XDMAC_CIM8          (*(__O  uint32_t*)0x40078258U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 8 */
#define REG_XDMAC_CIS8          (*(__I  uint32_t*)0x4007825CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 8 */
#define REG_XDMAC_CSA8          (*(__IO uint32_t*)0x40078260U) /**< (XDMAC) Channel Source Address Register (chid = 0) 8 */
#define REG_XDMAC_CDA8          (*(__IO uint32_t*)0x40078264U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 8 */
#define REG_XDMAC_CNDA8         (*(__IO uint32_t*)0x40078268U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 8 */
#define REG_XDMAC_CNDC8         (*(__IO uint32_t*)0x4007826CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 8 */
#define REG_XDMAC_CUBC8         (*(__IO uint32_t*)0x40078270U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 8 */
#define REG_XDMAC_CBC8          (*(__IO uint32_t*)0x40078274U) /**< (XDMAC) Channel Block Control Register (chid = 0) 8 */
#define REG_XDMAC_CC8           (*(__IO uint32_t*)0x40078278U) /**< (XDMAC) Channel Configuration Register (chid = 0) 8 */
#define REG_XDMAC_CDS_MSP8      (*(__IO uint32_t*)0x4007827CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 8 */
#define REG_XDMAC_CSUS8         (*(__IO uint32_t*)0x40078280U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 8 */
#define REG_XDMAC_CDUS8         (*(__IO uint32_t*)0x40078284U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 8 */
#define REG_XDMAC_CIE9          (*(__O  uint32_t*)0x40078290U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 9 */
#define REG_XDMAC_CID9          (*(__O  uint32_t*)0x40078294U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 9 */
#define REG_XDMAC_CIM9          (*(__O  uint32_t*)0x40078298U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 9 */
#define REG_XDMAC_CIS9          (*(__I  uint32_t*)0x4007829CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 9 */
#define REG_XDMAC_CSA9          (*(__IO uint32_t*)0x400782A0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 9 */
#define REG_XDMAC_CDA9          (*(__IO uint32_t*)0x400782A4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 9 */
#define REG_XDMAC_CNDA9         (*(__IO uint32_t*)0x400782A8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 9 */
#define REG_XDMAC_CNDC9         (*(__IO uint32_t*)0x400782ACU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 9 */
#define REG_XDMAC_CUBC9         (*(__IO uint32_t*)0x400782B0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 9 */
#define REG_XDMAC_CBC9          (*(__IO uint32_t*)0x400782B4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 9 */
#define REG_XDMAC_CC9           (*(__IO uint32_t*)0x400782B8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 9 */
#define REG_XDMAC_CDS_MSP9      (*(__IO uint32_t*)0x400782BCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 9 */
#define REG_XDMAC_CSUS9         (*(__IO uint32_t*)0x400782C0U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 9 */
#define REG_XDMAC_CDUS9         (*(__IO uint32_t*)0x400782C4U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 9 */
#define REG_XDMAC_CIE10         (*(__O  uint32_t*)0x400782D0U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 10 */
#define REG_XDMAC_CID10         (*(__O  uint32_t*)0x400782D4U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 10 */
#define REG_XDMAC_CIM10         (*(__O  uint32_t*)0x400782D8U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 10 */
#define REG_XDMAC_CIS10         (*(__I  uint32_t*)0x400782DCU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 10 */
#define REG_XDMAC_CSA10         (*(__IO uint32_t*)0x400782E0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 10 */
#define REG_XDMAC_CDA10         (*(__IO uint32_t*)0x400782E4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 10 */
#define REG_XDMAC_CNDA10        (*(__IO uint32_t*)0x400782E8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 10 */
#define REG_XDMAC_CNDC10        (*(__IO uint32_t*)0x400782ECU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 10 */
#define REG_XDMAC_CUBC10        (*(__IO uint32_t*)0x400782F0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 10 */
#define REG_XDMAC_CBC10         (*(__IO uint32_t*)0x400782F4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 10 */
#define REG_XDMAC_CC10          (*(__IO uint32_t*)0x400782F8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 10 */
#define REG_XDMAC_CDS_MSP10     (*(__IO uint32_t*)0x400782FCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 10 */
#define REG_XDMAC_CSUS10        (*(__IO uint32_t*)0x40078300U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 10 */
#define REG_XDMAC_CDUS10        (*(__IO uint32_t*)0x40078304U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 10 */
#define REG_XDMAC_CIE11         (*(__O  uint32_t*)0x40078310U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 11 */
#define REG_XDMAC_CID11         (*(__O  uint32_t*)0x40078314U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 11 */
#define REG_XDMAC_CIM11         (*(__O  uint32_t*)0x40078318U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 11 */
#define REG_XDMAC_CIS11         (*(__I  uint32_t*)0x4007831CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 11 */
#define REG_XDMAC_CSA11         (*(__IO uint32_t*)0x40078320U) /**< (XDMAC) Channel Source Address Register (chid = 0) 11 */
#define REG_XDMAC_CDA11         (*(__IO uint32_t*)0x40078324U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 11 */
#define REG_XDMAC_CNDA11        (*(__IO uint32_t*)0x40078328U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 11 */
#define REG_XDMAC_CNDC11        (*(__IO uint32_t*)0x4007832CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 11 */
#define REG_XDMAC_CUBC11        (*(__IO uint32_t*)0x40078330U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 11 */
#define REG_XDMAC_CBC11         (*(__IO uint32_t*)0x40078334U) /**< (XDMAC) Channel Block Control Register (chid = 0) 11 */
#define REG_XDMAC_CC11          (*(__IO uint32_t*)0x40078338U) /**< (XDMAC) Channel Configuration Register (chid = 0) 11 */
#define REG_XDMAC_CDS_MSP11     (*(__IO uint32_t*)0x4007833CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 11 */
#define REG_XDMAC_CSUS11        (*(__IO uint32_t*)0x40078340U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 11 */
#define REG_XDMAC_CDUS11        (*(__IO uint32_t*)0x40078344U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 11 */
#define REG_XDMAC_CIE12         (*(__O  uint32_t*)0x40078350U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 12 */
#define REG_XDMAC_CID12         (*(__O  uint32_t*)0x40078354U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 12 */
#define REG_XDMAC_CIM12         (*(__O  uint32_t*)0x40078358U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 12 */
#define REG_XDMAC_CIS12         (*(__I  uint32_t*)0x4007835CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 12 */
#define REG_XDMAC_CSA12         (*(__IO uint32_t*)0x40078360U) /**< (XDMAC) Channel Source Address Register (chid = 0) 12 */
#define REG_XDMAC_CDA12         (*(__IO uint32_t*)0x40078364U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 12 */
#define REG_XDMAC_CNDA12        (*(__IO uint32_t*)0x40078368U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 12 */
#define REG_XDMAC_CNDC12        (*(__IO uint32_t*)0x4007836CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 12 */
#define REG_XDMAC_CUBC12        (*(__IO uint32_t*)0x40078370U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 12 */
#define REG_XDMAC_CBC12         (*(__IO uint32_t*)0x40078374U) /**< (XDMAC) Channel Block Control Register (chid = 0) 12 */
#define REG_XDMAC_CC12          (*(__IO uint32_t*)0x40078378U) /**< (XDMAC) Channel Configuration Register (chid = 0) 12 */
#define REG_XDMAC_CDS_MSP12     (*(__IO uint32_t*)0x4007837CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 12 */
#define REG_XDMAC_CSUS12        (*(__IO uint32_t*)0x40078380U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 12 */
#define REG_XDMAC_CDUS12        (*(__IO uint32_t*)0x40078384U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 12 */
#define REG_XDMAC_CIE13         (*(__O  uint32_t*)0x40078390U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 13 */
#define REG_XDMAC_CID13         (*(__O  uint32_t*)0x40078394U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 13 */
#define REG_XDMAC_CIM13         (*(__O  uint32_t*)0x40078398U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 13 */
#define REG_XDMAC_CIS13         (*(__I  uint32_t*)0x4007839CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 13 */
#define REG_XDMAC_CSA13         (*(__IO uint32_t*)0x400783A0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 13 */
#define REG_XDMAC_CDA13         (*(__IO uint32_t*)0x400783A4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 13 */
#define REG_XDMAC_CNDA13        (*(__IO uint32_t*)0x400783A8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 13 */
#define REG_XDMAC_CNDC13        (*(__IO uint32_t*)0x400783ACU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 13 */
#define REG_XDMAC_CUBC13        (*(__IO uint32_t*)0x400783B0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 13 */
#define REG_XDMAC_CBC13         (*(__IO uint32_t*)0x400783B4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 13 */
#define REG_XDMAC_CC13          (*(__IO uint32_t*)0x400783B8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 13 */
#define REG_XDMAC_CDS_MSP13     (*(__IO uint32_t*)0x400783BCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 13 */
#define REG_XDMAC_CSUS13        (*(__IO uint32_t*)0x400783C0U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 13 */
#define REG_XDMAC_CDUS13        (*(__IO uint32_t*)0x400783C4U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 13 */
#define REG_XDMAC_CIE14         (*(__O  uint32_t*)0x400783D0U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 14 */
#define REG_XDMAC_CID14         (*(__O  uint32_t*)0x400783D4U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 14 */
#define REG_XDMAC_CIM14         (*(__O  uint32_t*)0x400783D8U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 14 */
#define REG_XDMAC_CIS14         (*(__I  uint32_t*)0x400783DCU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 14 */
#define REG_XDMAC_CSA14         (*(__IO uint32_t*)0x400783E0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 14 */
#define REG_XDMAC_CDA14         (*(__IO uint32_t*)0x400783E4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 14 */
#define REG_XDMAC_CNDA14        (*(__IO uint32_t*)0x400783E8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 14 */
#define REG_XDMAC_CNDC14        (*(__IO uint32_t*)0x400783ECU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 14 */
#define REG_XDMAC_CUBC14        (*(__IO uint32_t*)0x400783F0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 14 */
#define REG_XDMAC_CBC14         (*(__IO uint32_t*)0x400783F4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 14 */
#define REG_XDMAC_CC14          (*(__IO uint32_t*)0x400783F8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 14 */
#define REG_XDMAC_CDS_MSP14     (*(__IO uint32_t*)0x400783FCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 14 */
#define REG_XDMAC_CSUS14        (*(__IO uint32_t*)0x40078400U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 14 */
#define REG_XDMAC_CDUS14        (*(__IO uint32_t*)0x40078404U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 14 */
#define REG_XDMAC_CIE15         (*(__O  uint32_t*)0x40078410U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 15 */
#define REG_XDMAC_CID15         (*(__O  uint32_t*)0x40078414U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 15 */
#define REG_XDMAC_CIM15         (*(__O  uint32_t*)0x40078418U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 15 */
#define REG_XDMAC_CIS15         (*(__I  uint32_t*)0x4007841CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 15 */
#define REG_XDMAC_CSA15         (*(__IO uint32_t*)0x40078420U) /**< (XDMAC) Channel Source Address Register (chid = 0) 15 */
#define REG_XDMAC_CDA15         (*(__IO uint32_t*)0x40078424U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 15 */
#define REG_XDMAC_CNDA15        (*(__IO uint32_t*)0x40078428U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 15 */
#define REG_XDMAC_CNDC15        (*(__IO uint32_t*)0x4007842CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 15 */
#define REG_XDMAC_CUBC15        (*(__IO uint32_t*)0x40078430U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 15 */
#define REG_XDMAC_CBC15         (*(__IO uint32_t*)0x40078434U) /**< (XDMAC) Channel Block Control Register (chid = 0) 15 */
#define REG_XDMAC_CC15          (*(__IO uint32_t*)0x40078438U) /**< (XDMAC) Channel Configuration Register (chid = 0) 15 */
#define REG_XDMAC_CDS_MSP15     (*(__IO uint32_t*)0x4007843CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 15 */
#define REG_XDMAC_CSUS15        (*(__IO uint32_t*)0x40078440U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 15 */
#define REG_XDMAC_CDUS15        (*(__IO uint32_t*)0x40078444U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 15 */
#define REG_XDMAC_CIE16         (*(__O  uint32_t*)0x40078450U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 16 */
#define REG_XDMAC_CID16         (*(__O  uint32_t*)0x40078454U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 16 */
#define REG_XDMAC_CIM16         (*(__O  uint32_t*)0x40078458U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 16 */
#define REG_XDMAC_CIS16         (*(__I  uint32_t*)0x4007845CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 16 */
#define REG_XDMAC_CSA16         (*(__IO uint32_t*)0x40078460U) /**< (XDMAC) Channel Source Address Register (chid = 0) 16 */
#define REG_XDMAC_CDA16         (*(__IO uint32_t*)0x40078464U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 16 */
#define REG_XDMAC_CNDA16        (*(__IO uint32_t*)0x40078468U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 16 */
#define REG_XDMAC_CNDC16        (*(__IO uint32_t*)0x4007846CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 16 */
#define REG_XDMAC_CUBC16        (*(__IO uint32_t*)0x40078470U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 16 */
#define REG_XDMAC_CBC16         (*(__IO uint32_t*)0x40078474U) /**< (XDMAC) Channel Block Control Register (chid = 0) 16 */
#define REG_XDMAC_CC16          (*(__IO uint32_t*)0x40078478U) /**< (XDMAC) Channel Configuration Register (chid = 0) 16 */
#define REG_XDMAC_CDS_MSP16     (*(__IO uint32_t*)0x4007847CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 16 */
#define REG_XDMAC_CSUS16        (*(__IO uint32_t*)0x40078480U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 16 */
#define REG_XDMAC_CDUS16        (*(__IO uint32_t*)0x40078484U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 16 */
#define REG_XDMAC_CIE17         (*(__O  uint32_t*)0x40078490U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 17 */
#define REG_XDMAC_CID17         (*(__O  uint32_t*)0x40078494U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 17 */
#define REG_XDMAC_CIM17         (*(__O  uint32_t*)0x40078498U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 17 */
#define REG_XDMAC_CIS17         (*(__I  uint32_t*)0x4007849CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 17 */
#define REG_XDMAC_CSA17         (*(__IO uint32_t*)0x400784A0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 17 */
#define REG_XDMAC_CDA17         (*(__IO uint32_t*)0x400784A4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 17 */
#define REG_XDMAC_CNDA17        (*(__IO uint32_t*)0x400784A8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 17 */
#define REG_XDMAC_CNDC17        (*(__IO uint32_t*)0x400784ACU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 17 */
#define REG_XDMAC_CUBC17        (*(__IO uint32_t*)0x400784B0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 17 */
#define REG_XDMAC_CBC17         (*(__IO uint32_t*)0x400784B4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 17 */
#define REG_XDMAC_CC17          (*(__IO uint32_t*)0x400784B8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 17 */
#define REG_XDMAC_CDS_MSP17     (*(__IO uint32_t*)0x400784BCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 17 */
#define REG_XDMAC_CSUS17        (*(__IO uint32_t*)0x400784C0U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 17 */
#define REG_XDMAC_CDUS17        (*(__IO uint32_t*)0x400784C4U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 17 */
#define REG_XDMAC_CIE18         (*(__O  uint32_t*)0x400784D0U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 18 */
#define REG_XDMAC_CID18         (*(__O  uint32_t*)0x400784D4U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 18 */
#define REG_XDMAC_CIM18         (*(__O  uint32_t*)0x400784D8U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 18 */
#define REG_XDMAC_CIS18         (*(__I  uint32_t*)0x400784DCU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 18 */
#define REG_XDMAC_CSA18         (*(__IO uint32_t*)0x400784E0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 18 */
#define REG_XDMAC_CDA18         (*(__IO uint32_t*)0x400784E4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 18 */
#define REG_XDMAC_CNDA18        (*(__IO uint32_t*)0x400784E8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 18 */
#define REG_XDMAC_CNDC18        (*(__IO uint32_t*)0x400784ECU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 18 */
#define REG_XDMAC_CUBC18        (*(__IO uint32_t*)0x400784F0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 18 */
#define REG_XDMAC_CBC18         (*(__IO uint32_t*)0x400784F4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 18 */
#define REG_XDMAC_CC18          (*(__IO uint32_t*)0x400784F8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 18 */
#define REG_XDMAC_CDS_MSP18     (*(__IO uint32_t*)0x400784FCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 18 */
#define REG_XDMAC_CSUS18        (*(__IO uint32_t*)0x40078500U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 18 */
#define REG_XDMAC_CDUS18        (*(__IO uint32_t*)0x40078504U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 18 */
#define REG_XDMAC_CIE19         (*(__O  uint32_t*)0x40078510U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 19 */
#define REG_XDMAC_CID19         (*(__O  uint32_t*)0x40078514U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 19 */
#define REG_XDMAC_CIM19         (*(__O  uint32_t*)0x40078518U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 19 */
#define REG_XDMAC_CIS19         (*(__I  uint32_t*)0x4007851CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 19 */
#define REG_XDMAC_CSA19         (*(__IO uint32_t*)0x40078520U) /**< (XDMAC) Channel Source Address Register (chid = 0) 19 */
#define REG_XDMAC_CDA19         (*(__IO uint32_t*)0x40078524U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 19 */
#define REG_XDMAC_CNDA19        (*(__IO uint32_t*)0x40078528U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 19 */
#define REG_XDMAC_CNDC19        (*(__IO uint32_t*)0x4007852CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 19 */
#define REG_XDMAC_CUBC19        (*(__IO uint32_t*)0x40078530U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 19 */
#define REG_XDMAC_CBC19         (*(__IO uint32_t*)0x40078534U) /**< (XDMAC) Channel Block Control Register (chid = 0) 19 */
#define REG_XDMAC_CC19          (*(__IO uint32_t*)0x40078538U) /**< (XDMAC) Channel Configuration Register (chid = 0) 19 */
#define REG_XDMAC_CDS_MSP19     (*(__IO uint32_t*)0x4007853CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 19 */
#define REG_XDMAC_CSUS19        (*(__IO uint32_t*)0x40078540U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 19 */
#define REG_XDMAC_CDUS19        (*(__IO uint32_t*)0x40078544U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 19 */
#define REG_XDMAC_CIE20         (*(__O  uint32_t*)0x40078550U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 20 */
#define REG_XDMAC_CID20         (*(__O  uint32_t*)0x40078554U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 20 */
#define REG_XDMAC_CIM20         (*(__O  uint32_t*)0x40078558U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 20 */
#define REG_XDMAC_CIS20         (*(__I  uint32_t*)0x4007855CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 20 */
#define REG_XDMAC_CSA20         (*(__IO uint32_t*)0x40078560U) /**< (XDMAC) Channel Source Address Register (chid = 0) 20 */
#define REG_XDMAC_CDA20         (*(__IO uint32_t*)0x40078564U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 20 */
#define REG_XDMAC_CNDA20        (*(__IO uint32_t*)0x40078568U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 20 */
#define REG_XDMAC_CNDC20        (*(__IO uint32_t*)0x4007856CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 20 */
#define REG_XDMAC_CUBC20        (*(__IO uint32_t*)0x40078570U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 20 */
#define REG_XDMAC_CBC20         (*(__IO uint32_t*)0x40078574U) /**< (XDMAC) Channel Block Control Register (chid = 0) 20 */
#define REG_XDMAC_CC20          (*(__IO uint32_t*)0x40078578U) /**< (XDMAC) Channel Configuration Register (chid = 0) 20 */
#define REG_XDMAC_CDS_MSP20     (*(__IO uint32_t*)0x4007857CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 20 */
#define REG_XDMAC_CSUS20        (*(__IO uint32_t*)0x40078580U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 20 */
#define REG_XDMAC_CDUS20        (*(__IO uint32_t*)0x40078584U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 20 */
#define REG_XDMAC_CIE21         (*(__O  uint32_t*)0x40078590U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 21 */
#define REG_XDMAC_CID21         (*(__O  uint32_t*)0x40078594U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 21 */
#define REG_XDMAC_CIM21         (*(__O  uint32_t*)0x40078598U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 21 */
#define REG_XDMAC_CIS21         (*(__I  uint32_t*)0x4007859CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 21 */
#define REG_XDMAC_CSA21         (*(__IO uint32_t*)0x400785A0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 21 */
#define REG_XDMAC_CDA21         (*(__IO uint32_t*)0x400785A4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 21 */
#define REG_XDMAC_CNDA21        (*(__IO uint32_t*)0x400785A8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 21 */
#define REG_XDMAC_CNDC21        (*(__IO uint32_t*)0x400785ACU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 21 */
#define REG_XDMAC_CUBC21        (*(__IO uint32_t*)0x400785B0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 21 */
#define REG_XDMAC_CBC21         (*(__IO uint32_t*)0x400785B4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 21 */
#define REG_XDMAC_CC21          (*(__IO uint32_t*)0x400785B8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 21 */
#define REG_XDMAC_CDS_MSP21     (*(__IO uint32_t*)0x400785BCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 21 */
#define REG_XDMAC_CSUS21        (*(__IO uint32_t*)0x400785C0U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 21 */
#define REG_XDMAC_CDUS21        (*(__IO uint32_t*)0x400785C4U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 21 */
#define REG_XDMAC_CIE22         (*(__O  uint32_t*)0x400785D0U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 22 */
#define REG_XDMAC_CID22         (*(__O  uint32_t*)0x400785D4U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 22 */
#define REG_XDMAC_CIM22         (*(__O  uint32_t*)0x400785D8U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 22 */
#define REG_XDMAC_CIS22         (*(__I  uint32_t*)0x400785DCU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 22 */
#define REG_XDMAC_CSA22         (*(__IO uint32_t*)0x400785E0U) /**< (XDMAC) Channel Source Address Register (chid = 0) 22 */
#define REG_XDMAC_CDA22         (*(__IO uint32_t*)0x400785E4U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 22 */
#define REG_XDMAC_CNDA22        (*(__IO uint32_t*)0x400785E8U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 22 */
#define REG_XDMAC_CNDC22        (*(__IO uint32_t*)0x400785ECU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 22 */
#define REG_XDMAC_CUBC22        (*(__IO uint32_t*)0x400785F0U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 22 */
#define REG_XDMAC_CBC22         (*(__IO uint32_t*)0x400785F4U) /**< (XDMAC) Channel Block Control Register (chid = 0) 22 */
#define REG_XDMAC_CC22          (*(__IO uint32_t*)0x400785F8U) /**< (XDMAC) Channel Configuration Register (chid = 0) 22 */
#define REG_XDMAC_CDS_MSP22     (*(__IO uint32_t*)0x400785FCU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 22 */
#define REG_XDMAC_CSUS22        (*(__IO uint32_t*)0x40078600U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 22 */
#define REG_XDMAC_CDUS22        (*(__IO uint32_t*)0x40078604U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 22 */
#define REG_XDMAC_CIE23         (*(__O  uint32_t*)0x40078610U) /**< (XDMAC) Channel Interrupt Enable Register (chid = 0) 23 */
#define REG_XDMAC_CID23         (*(__O  uint32_t*)0x40078614U) /**< (XDMAC) Channel Interrupt Disable Register (chid = 0) 23 */
#define REG_XDMAC_CIM23         (*(__O  uint32_t*)0x40078618U) /**< (XDMAC) Channel Interrupt Mask Register (chid = 0) 23 */
#define REG_XDMAC_CIS23         (*(__I  uint32_t*)0x4007861CU) /**< (XDMAC) Channel Interrupt Status Register (chid = 0) 23 */
#define REG_XDMAC_CSA23         (*(__IO uint32_t*)0x40078620U) /**< (XDMAC) Channel Source Address Register (chid = 0) 23 */
#define REG_XDMAC_CDA23         (*(__IO uint32_t*)0x40078624U) /**< (XDMAC) Channel Destination Address Register (chid = 0) 23 */
#define REG_XDMAC_CNDA23        (*(__IO uint32_t*)0x40078628U) /**< (XDMAC) Channel Next Descriptor Address Register (chid = 0) 23 */
#define REG_XDMAC_CNDC23        (*(__IO uint32_t*)0x4007862CU) /**< (XDMAC) Channel Next Descriptor Control Register (chid = 0) 23 */
#define REG_XDMAC_CUBC23        (*(__IO uint32_t*)0x40078630U) /**< (XDMAC) Channel Microblock Control Register (chid = 0) 23 */
#define REG_XDMAC_CBC23         (*(__IO uint32_t*)0x40078634U) /**< (XDMAC) Channel Block Control Register (chid = 0) 23 */
#define REG_XDMAC_CC23          (*(__IO uint32_t*)0x40078638U) /**< (XDMAC) Channel Configuration Register (chid = 0) 23 */
#define REG_XDMAC_CDS_MSP23     (*(__IO uint32_t*)0x4007863CU) /**< (XDMAC) Channel Data Stride Memory Set Pattern (chid = 0) 23 */
#define REG_XDMAC_CSUS23        (*(__IO uint32_t*)0x40078640U) /**< (XDMAC) Channel Source Microblock Stride (chid = 0) 23 */
#define REG_XDMAC_CDUS23        (*(__IO uint32_t*)0x40078644U) /**< (XDMAC) Channel Destination Microblock Stride (chid = 0) 23 */
#define REG_XDMAC_GTYPE         (*(__IO uint32_t*)0x40078000U) /**< (XDMAC) Global Type Register */
#define REG_XDMAC_GCFG          (*(__I  uint32_t*)0x40078004U) /**< (XDMAC) Global Configuration Register */
#define REG_XDMAC_GWAC          (*(__IO uint32_t*)0x40078008U) /**< (XDMAC) Global Weighted Arbiter Configuration Register */
#define REG_XDMAC_GIE           (*(__O  uint32_t*)0x4007800CU) /**< (XDMAC) Global Interrupt Enable Register */
#define REG_XDMAC_GID           (*(__O  uint32_t*)0x40078010U) /**< (XDMAC) Global Interrupt Disable Register */
#define REG_XDMAC_GIM           (*(__I  uint32_t*)0x40078014U) /**< (XDMAC) Global Interrupt Mask Register */
#define REG_XDMAC_GIS           (*(__I  uint32_t*)0x40078018U) /**< (XDMAC) Global Interrupt Status Register */
#define REG_XDMAC_GE            (*(__O  uint32_t*)0x4007801CU) /**< (XDMAC) Global Channel Enable Register */
#define REG_XDMAC_GD            (*(__O  uint32_t*)0x40078020U) /**< (XDMAC) Global Channel Disable Register */
#define REG_XDMAC_GS            (*(__I  uint32_t*)0x40078024U) /**< (XDMAC) Global Channel Status Register */
#define REG_XDMAC_GRS           (*(__IO uint32_t*)0x40078028U) /**< (XDMAC) Global Channel Read Suspend Register */
#define REG_XDMAC_GWS           (*(__IO uint32_t*)0x4007802CU) /**< (XDMAC) Global Channel Write Suspend Register */
#define REG_XDMAC_GRWS          (*(__O  uint32_t*)0x40078030U) /**< (XDMAC) Global Channel Read Write Suspend Register */
#define REG_XDMAC_GRWR          (*(__O  uint32_t*)0x40078034U) /**< (XDMAC) Global Channel Read Write Resume Register */
#define REG_XDMAC_GSWR          (*(__O  uint32_t*)0x40078038U) /**< (XDMAC) Global Channel Software Request Register */
#define REG_XDMAC_GSWS          (*(__I  uint32_t*)0x4007803CU) /**< (XDMAC) Global Channel Software Request Status Register */
#define REG_XDMAC_GSWF          (*(__O  uint32_t*)0x40078040U) /**< (XDMAC) Global Channel Software Flush Request Register */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for XDMAC peripheral ========== */
#define XDMAC_INSTANCE_ID                        58        

#endif /* _SAME70_XDMAC_INSTANCE_ */
