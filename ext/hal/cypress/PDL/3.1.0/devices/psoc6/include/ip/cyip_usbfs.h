/***************************************************************************//**
* \file cyip_usbfs.h
*
* \brief
* USBFS IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_USBFS_H_
#define _CYIP_USBFS_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    USBFS
*******************************************************************************/

#define USBFS_USBDEV_SECTION_SIZE               0x00002000UL
#define USBFS_USBLPM_SECTION_SIZE               0x00001000UL
#define USBFS_USBHOST_SECTION_SIZE              0x00002000UL
#define USBFS_SECTION_SIZE                      0x00010000UL

/**
  * \brief USB Device (USBFS_USBDEV)
  */
typedef struct {
  __IOM uint32_t EP0_DR[8];                     /*!< 0x00000000 Control End point EP0 Data Register */
  __IOM uint32_t CR0;                           /*!< 0x00000020 USB control 0 Register */
  __IOM uint32_t CR1;                           /*!< 0x00000024 USB control 1 Register */
  __IOM uint32_t SIE_EP_INT_EN;                 /*!< 0x00000028 USB SIE Data Endpoints Interrupt Enable Register */
  __IOM uint32_t SIE_EP_INT_SR;                 /*!< 0x0000002C USB SIE Data Endpoint Interrupt Status */
  __IOM uint32_t SIE_EP1_CNT0;                  /*!< 0x00000030 Non-control endpoint count register */
  __IOM uint32_t SIE_EP1_CNT1;                  /*!< 0x00000034 Non-control endpoint count register */
  __IOM uint32_t SIE_EP1_CR0;                   /*!< 0x00000038 Non-control endpoint's control Register */
   __IM uint32_t RESERVED;
  __IOM uint32_t USBIO_CR0;                     /*!< 0x00000040 USBIO Control 0 Register */
  __IOM uint32_t USBIO_CR2;                     /*!< 0x00000044 USBIO control 2 Register */
  __IOM uint32_t USBIO_CR1;                     /*!< 0x00000048 USBIO control 1 Register */
   __IM uint32_t RESERVED1;
  __IOM uint32_t DYN_RECONFIG;                  /*!< 0x00000050 USB Dynamic reconfiguration register */
   __IM uint32_t RESERVED2[3];
   __IM uint32_t SOF0;                          /*!< 0x00000060 Start Of Frame Register */
   __IM uint32_t SOF1;                          /*!< 0x00000064 Start Of Frame Register */
   __IM uint32_t RESERVED3[2];
  __IOM uint32_t SIE_EP2_CNT0;                  /*!< 0x00000070 Non-control endpoint count register */
  __IOM uint32_t SIE_EP2_CNT1;                  /*!< 0x00000074 Non-control endpoint count register */
  __IOM uint32_t SIE_EP2_CR0;                   /*!< 0x00000078 Non-control endpoint's control Register */
   __IM uint32_t RESERVED4;
   __IM uint32_t OSCLK_DR0;                     /*!< 0x00000080 Oscillator lock data register 0 */
   __IM uint32_t OSCLK_DR1;                     /*!< 0x00000084 Oscillator lock data register 1 */
   __IM uint32_t RESERVED5[6];
  __IOM uint32_t EP0_CR;                        /*!< 0x000000A0 Endpoint0 control Register */
  __IOM uint32_t EP0_CNT;                       /*!< 0x000000A4 Endpoint0 count Register */
   __IM uint32_t RESERVED6[2];
  __IOM uint32_t SIE_EP3_CNT0;                  /*!< 0x000000B0 Non-control endpoint count register */
  __IOM uint32_t SIE_EP3_CNT1;                  /*!< 0x000000B4 Non-control endpoint count register */
  __IOM uint32_t SIE_EP3_CR0;                   /*!< 0x000000B8 Non-control endpoint's control Register */
   __IM uint32_t RESERVED7[13];
  __IOM uint32_t SIE_EP4_CNT0;                  /*!< 0x000000F0 Non-control endpoint count register */
  __IOM uint32_t SIE_EP4_CNT1;                  /*!< 0x000000F4 Non-control endpoint count register */
  __IOM uint32_t SIE_EP4_CR0;                   /*!< 0x000000F8 Non-control endpoint's control Register */
   __IM uint32_t RESERVED8[13];
  __IOM uint32_t SIE_EP5_CNT0;                  /*!< 0x00000130 Non-control endpoint count register */
  __IOM uint32_t SIE_EP5_CNT1;                  /*!< 0x00000134 Non-control endpoint count register */
  __IOM uint32_t SIE_EP5_CR0;                   /*!< 0x00000138 Non-control endpoint's control Register */
   __IM uint32_t RESERVED9[13];
  __IOM uint32_t SIE_EP6_CNT0;                  /*!< 0x00000170 Non-control endpoint count register */
  __IOM uint32_t SIE_EP6_CNT1;                  /*!< 0x00000174 Non-control endpoint count register */
  __IOM uint32_t SIE_EP6_CR0;                   /*!< 0x00000178 Non-control endpoint's control Register */
   __IM uint32_t RESERVED10[13];
  __IOM uint32_t SIE_EP7_CNT0;                  /*!< 0x000001B0 Non-control endpoint count register */
  __IOM uint32_t SIE_EP7_CNT1;                  /*!< 0x000001B4 Non-control endpoint count register */
  __IOM uint32_t SIE_EP7_CR0;                   /*!< 0x000001B8 Non-control endpoint's control Register */
   __IM uint32_t RESERVED11[13];
  __IOM uint32_t SIE_EP8_CNT0;                  /*!< 0x000001F0 Non-control endpoint count register */
  __IOM uint32_t SIE_EP8_CNT1;                  /*!< 0x000001F4 Non-control endpoint count register */
  __IOM uint32_t SIE_EP8_CR0;                   /*!< 0x000001F8 Non-control endpoint's control Register */
   __IM uint32_t RESERVED12;
  __IOM uint32_t ARB_EP1_CFG;                   /*!< 0x00000200 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP1_INT_EN;                /*!< 0x00000204 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP1_SR;                    /*!< 0x00000208 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED13;
  __IOM uint32_t ARB_RW1_WA;                    /*!< 0x00000210 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW1_WA_MSB;                /*!< 0x00000214 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW1_RA;                    /*!< 0x00000218 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW1_RA_MSB;                /*!< 0x0000021C Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW1_DR;                    /*!< 0x00000220 Endpoint Data Register */
   __IM uint32_t RESERVED14[3];
  __IOM uint32_t BUF_SIZE;                      /*!< 0x00000230 Dedicated Endpoint Buffer Size Register  *1 */
   __IM uint32_t RESERVED15;
  __IOM uint32_t EP_ACTIVE;                     /*!< 0x00000238 Endpoint Active Indication Register  *1 */
  __IOM uint32_t EP_TYPE;                       /*!< 0x0000023C Endpoint Type (IN/OUT) Indication  *1 */
  __IOM uint32_t ARB_EP2_CFG;                   /*!< 0x00000240 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP2_INT_EN;                /*!< 0x00000244 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP2_SR;                    /*!< 0x00000248 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED16;
  __IOM uint32_t ARB_RW2_WA;                    /*!< 0x00000250 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW2_WA_MSB;                /*!< 0x00000254 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW2_RA;                    /*!< 0x00000258 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW2_RA_MSB;                /*!< 0x0000025C Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW2_DR;                    /*!< 0x00000260 Endpoint Data Register */
   __IM uint32_t RESERVED17[3];
  __IOM uint32_t ARB_CFG;                       /*!< 0x00000270 Arbiter Configuration Register  *1 */
  __IOM uint32_t USB_CLK_EN;                    /*!< 0x00000274 USB Block Clock Enable Register */
  __IOM uint32_t ARB_INT_EN;                    /*!< 0x00000278 Arbiter Interrupt Enable  *1 */
   __IM uint32_t ARB_INT_SR;                    /*!< 0x0000027C Arbiter Interrupt Status  *1 */
  __IOM uint32_t ARB_EP3_CFG;                   /*!< 0x00000280 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP3_INT_EN;                /*!< 0x00000284 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP3_SR;                    /*!< 0x00000288 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED18;
  __IOM uint32_t ARB_RW3_WA;                    /*!< 0x00000290 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW3_WA_MSB;                /*!< 0x00000294 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW3_RA;                    /*!< 0x00000298 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW3_RA_MSB;                /*!< 0x0000029C Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW3_DR;                    /*!< 0x000002A0 Endpoint Data Register */
   __IM uint32_t RESERVED19[3];
  __IOM uint32_t CWA;                           /*!< 0x000002B0 Common Area Write Address  *1 */
  __IOM uint32_t CWA_MSB;                       /*!< 0x000002B4 Endpoint Read Address value  *1 */
   __IM uint32_t RESERVED20[2];
  __IOM uint32_t ARB_EP4_CFG;                   /*!< 0x000002C0 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP4_INT_EN;                /*!< 0x000002C4 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP4_SR;                    /*!< 0x000002C8 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED21;
  __IOM uint32_t ARB_RW4_WA;                    /*!< 0x000002D0 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW4_WA_MSB;                /*!< 0x000002D4 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW4_RA;                    /*!< 0x000002D8 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW4_RA_MSB;                /*!< 0x000002DC Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW4_DR;                    /*!< 0x000002E0 Endpoint Data Register */
   __IM uint32_t RESERVED22[3];
  __IOM uint32_t DMA_THRES;                     /*!< 0x000002F0 DMA Burst / Threshold Configuration */
  __IOM uint32_t DMA_THRES_MSB;                 /*!< 0x000002F4 DMA Burst / Threshold Configuration */
   __IM uint32_t RESERVED23[2];
  __IOM uint32_t ARB_EP5_CFG;                   /*!< 0x00000300 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP5_INT_EN;                /*!< 0x00000304 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP5_SR;                    /*!< 0x00000308 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED24;
  __IOM uint32_t ARB_RW5_WA;                    /*!< 0x00000310 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW5_WA_MSB;                /*!< 0x00000314 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW5_RA;                    /*!< 0x00000318 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW5_RA_MSB;                /*!< 0x0000031C Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW5_DR;                    /*!< 0x00000320 Endpoint Data Register */
   __IM uint32_t RESERVED25[3];
  __IOM uint32_t BUS_RST_CNT;                   /*!< 0x00000330 Bus Reset Count Register */
   __IM uint32_t RESERVED26[3];
  __IOM uint32_t ARB_EP6_CFG;                   /*!< 0x00000340 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP6_INT_EN;                /*!< 0x00000344 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP6_SR;                    /*!< 0x00000348 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED27;
  __IOM uint32_t ARB_RW6_WA;                    /*!< 0x00000350 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW6_WA_MSB;                /*!< 0x00000354 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW6_RA;                    /*!< 0x00000358 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW6_RA_MSB;                /*!< 0x0000035C Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW6_DR;                    /*!< 0x00000360 Endpoint Data Register */
   __IM uint32_t RESERVED28[7];
  __IOM uint32_t ARB_EP7_CFG;                   /*!< 0x00000380 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP7_INT_EN;                /*!< 0x00000384 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP7_SR;                    /*!< 0x00000388 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED29;
  __IOM uint32_t ARB_RW7_WA;                    /*!< 0x00000390 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW7_WA_MSB;                /*!< 0x00000394 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW7_RA;                    /*!< 0x00000398 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW7_RA_MSB;                /*!< 0x0000039C Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW7_DR;                    /*!< 0x000003A0 Endpoint Data Register */
   __IM uint32_t RESERVED30[7];
  __IOM uint32_t ARB_EP8_CFG;                   /*!< 0x000003C0 Endpoint Configuration Register  *1 */
  __IOM uint32_t ARB_EP8_INT_EN;                /*!< 0x000003C4 Endpoint Interrupt Enable Register  *1 */
  __IOM uint32_t ARB_EP8_SR;                    /*!< 0x000003C8 Endpoint Interrupt Enable Register  *1 */
   __IM uint32_t RESERVED31;
  __IOM uint32_t ARB_RW8_WA;                    /*!< 0x000003D0 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW8_WA_MSB;                /*!< 0x000003D4 Endpoint Write Address value  *1 */
  __IOM uint32_t ARB_RW8_RA;                    /*!< 0x000003D8 Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW8_RA_MSB;                /*!< 0x000003DC Endpoint Read Address value  *1 */
  __IOM uint32_t ARB_RW8_DR;                    /*!< 0x000003E0 Endpoint Data Register */
   __IM uint32_t RESERVED32[7];
  __IOM uint32_t MEM_DATA[512];                 /*!< 0x00000400 DATA */
   __IM uint32_t RESERVED33[280];
   __IM uint32_t SOF16;                         /*!< 0x00001060 Start Of Frame Register */
   __IM uint32_t RESERVED34[7];
   __IM uint32_t OSCLK_DR16;                    /*!< 0x00001080 Oscillator lock data register */
   __IM uint32_t RESERVED35[99];
  __IOM uint32_t ARB_RW1_WA16;                  /*!< 0x00001210 Endpoint Write Address value */
   __IM uint32_t RESERVED36;
  __IOM uint32_t ARB_RW1_RA16;                  /*!< 0x00001218 Endpoint Read Address value */
   __IM uint32_t RESERVED37;
  __IOM uint32_t ARB_RW1_DR16;                  /*!< 0x00001220 Endpoint Data Register */
   __IM uint32_t RESERVED38[11];
  __IOM uint32_t ARB_RW2_WA16;                  /*!< 0x00001250 Endpoint Write Address value */
   __IM uint32_t RESERVED39;
  __IOM uint32_t ARB_RW2_RA16;                  /*!< 0x00001258 Endpoint Read Address value */
   __IM uint32_t RESERVED40;
  __IOM uint32_t ARB_RW2_DR16;                  /*!< 0x00001260 Endpoint Data Register */
   __IM uint32_t RESERVED41[11];
  __IOM uint32_t ARB_RW3_WA16;                  /*!< 0x00001290 Endpoint Write Address value */
   __IM uint32_t RESERVED42;
  __IOM uint32_t ARB_RW3_RA16;                  /*!< 0x00001298 Endpoint Read Address value */
   __IM uint32_t RESERVED43;
  __IOM uint32_t ARB_RW3_DR16;                  /*!< 0x000012A0 Endpoint Data Register */
   __IM uint32_t RESERVED44[3];
  __IOM uint32_t CWA16;                         /*!< 0x000012B0 Common Area Write Address */
   __IM uint32_t RESERVED45[7];
  __IOM uint32_t ARB_RW4_WA16;                  /*!< 0x000012D0 Endpoint Write Address value */
   __IM uint32_t RESERVED46;
  __IOM uint32_t ARB_RW4_RA16;                  /*!< 0x000012D8 Endpoint Read Address value */
   __IM uint32_t RESERVED47;
  __IOM uint32_t ARB_RW4_DR16;                  /*!< 0x000012E0 Endpoint Data Register */
   __IM uint32_t RESERVED48[3];
  __IOM uint32_t DMA_THRES16;                   /*!< 0x000012F0 DMA Burst / Threshold Configuration */
   __IM uint32_t RESERVED49[7];
  __IOM uint32_t ARB_RW5_WA16;                  /*!< 0x00001310 Endpoint Write Address value */
   __IM uint32_t RESERVED50;
  __IOM uint32_t ARB_RW5_RA16;                  /*!< 0x00001318 Endpoint Read Address value */
   __IM uint32_t RESERVED51;
  __IOM uint32_t ARB_RW5_DR16;                  /*!< 0x00001320 Endpoint Data Register */
   __IM uint32_t RESERVED52[11];
  __IOM uint32_t ARB_RW6_WA16;                  /*!< 0x00001350 Endpoint Write Address value */
   __IM uint32_t RESERVED53;
  __IOM uint32_t ARB_RW6_RA16;                  /*!< 0x00001358 Endpoint Read Address value */
   __IM uint32_t RESERVED54;
  __IOM uint32_t ARB_RW6_DR16;                  /*!< 0x00001360 Endpoint Data Register */
   __IM uint32_t RESERVED55[11];
  __IOM uint32_t ARB_RW7_WA16;                  /*!< 0x00001390 Endpoint Write Address value */
   __IM uint32_t RESERVED56;
  __IOM uint32_t ARB_RW7_RA16;                  /*!< 0x00001398 Endpoint Read Address value */
   __IM uint32_t RESERVED57;
  __IOM uint32_t ARB_RW7_DR16;                  /*!< 0x000013A0 Endpoint Data Register */
   __IM uint32_t RESERVED58[11];
  __IOM uint32_t ARB_RW8_WA16;                  /*!< 0x000013D0 Endpoint Write Address value */
   __IM uint32_t RESERVED59;
  __IOM uint32_t ARB_RW8_RA16;                  /*!< 0x000013D8 Endpoint Read Address value */
   __IM uint32_t RESERVED60;
  __IOM uint32_t ARB_RW8_DR16;                  /*!< 0x000013E0 Endpoint Data Register */
   __IM uint32_t RESERVED61[775];
} USBFS_USBDEV_V1_Type;                         /*!< Size = 8192 (0x2000) */

/**
  * \brief USB Device LPM and PHY Test (USBFS_USBLPM)
  */
typedef struct {
  __IOM uint32_t POWER_CTL;                     /*!< 0x00000000 Power Control Register */
   __IM uint32_t RESERVED;
  __IOM uint32_t USBIO_CTL;                     /*!< 0x00000008 USB IO Control Register */
  __IOM uint32_t FLOW_CTL;                      /*!< 0x0000000C Flow Control Register */
  __IOM uint32_t LPM_CTL;                       /*!< 0x00000010 LPM Control Register */
   __IM uint32_t LPM_STAT;                      /*!< 0x00000014 LPM Status register */
   __IM uint32_t RESERVED1[2];
  __IOM uint32_t INTR_SIE;                      /*!< 0x00000020 USB SOF, BUS RESET and EP0 Interrupt Status */
  __IOM uint32_t INTR_SIE_SET;                  /*!< 0x00000024 USB SOF, BUS RESET and EP0 Interrupt Set */
  __IOM uint32_t INTR_SIE_MASK;                 /*!< 0x00000028 USB SOF, BUS RESET and EP0 Interrupt Mask */
   __IM uint32_t INTR_SIE_MASKED;               /*!< 0x0000002C USB SOF, BUS RESET and EP0 Interrupt Masked */
  __IOM uint32_t INTR_LVL_SEL;                  /*!< 0x00000030 Select interrupt level for each interrupt source */
   __IM uint32_t INTR_CAUSE_HI;                 /*!< 0x00000034 High priority interrupt Cause register */
   __IM uint32_t INTR_CAUSE_MED;                /*!< 0x00000038 Medium priority interrupt Cause register */
   __IM uint32_t INTR_CAUSE_LO;                 /*!< 0x0000003C Low priority interrupt Cause register */
   __IM uint32_t RESERVED2[12];
  __IOM uint32_t DFT_CTL;                       /*!< 0x00000070 DFT control */
   __IM uint32_t RESERVED3[995];
} USBFS_USBLPM_V1_Type;                         /*!< Size = 4096 (0x1000) */

/**
  * \brief USB Host Controller (USBFS_USBHOST)
  */
typedef struct {
  __IOM uint32_t HOST_CTL0;                     /*!< 0x00000000 Host Control 0 Register. */
   __IM uint32_t RESERVED[3];
  __IOM uint32_t HOST_CTL1;                     /*!< 0x00000010 Host Control 1 Register. */
   __IM uint32_t RESERVED1[59];
  __IOM uint32_t HOST_CTL2;                     /*!< 0x00000100 Host Control 2 Register. */
  __IOM uint32_t HOST_ERR;                      /*!< 0x00000104 Host Error Status Register. */
  __IOM uint32_t HOST_STATUS;                   /*!< 0x00000108 Host Status Register. */
  __IOM uint32_t HOST_FCOMP;                    /*!< 0x0000010C Host SOF Interrupt Frame Compare Register */
  __IOM uint32_t HOST_RTIMER;                   /*!< 0x00000110 Host Retry Timer Setup Register */
  __IOM uint32_t HOST_ADDR;                     /*!< 0x00000114 Host Address Register */
  __IOM uint32_t HOST_EOF;                      /*!< 0x00000118 Host EOF Setup Register */
  __IOM uint32_t HOST_FRAME;                    /*!< 0x0000011C Host Frame Setup Register */
  __IOM uint32_t HOST_TOKEN;                    /*!< 0x00000120 Host Token Endpoint Register */
   __IM uint32_t RESERVED2[183];
  __IOM uint32_t HOST_EP1_CTL;                  /*!< 0x00000400 Host Endpoint 1 Control Register */
   __IM uint32_t HOST_EP1_STATUS;               /*!< 0x00000404 Host Endpoint 1 Status Register */
  __IOM uint32_t HOST_EP1_RW1_DR;               /*!< 0x00000408 Host Endpoint 1 Data 1-Byte Register */
  __IOM uint32_t HOST_EP1_RW2_DR;               /*!< 0x0000040C Host Endpoint 1 Data 2-Byte Register */
   __IM uint32_t RESERVED3[60];
  __IOM uint32_t HOST_EP2_CTL;                  /*!< 0x00000500 Host Endpoint 2 Control Register */
   __IM uint32_t HOST_EP2_STATUS;               /*!< 0x00000504 Host Endpoint 2 Status Register */
  __IOM uint32_t HOST_EP2_RW1_DR;               /*!< 0x00000508 Host Endpoint 2 Data 1-Byte Register */
  __IOM uint32_t HOST_EP2_RW2_DR;               /*!< 0x0000050C Host Endpoint 2 Data 2-Byte Register */
   __IM uint32_t RESERVED4[188];
  __IOM uint32_t HOST_LVL1_SEL;                 /*!< 0x00000800 Host Interrupt Level 1 Selection Register */
  __IOM uint32_t HOST_LVL2_SEL;                 /*!< 0x00000804 Host Interrupt Level 2 Selection Register */
   __IM uint32_t RESERVED5[62];
   __IM uint32_t INTR_USBHOST_CAUSE_HI;         /*!< 0x00000900 Interrupt USB Host Cause High Register */
   __IM uint32_t INTR_USBHOST_CAUSE_MED;        /*!< 0x00000904 Interrupt USB Host Cause Medium Register */
   __IM uint32_t INTR_USBHOST_CAUSE_LO;         /*!< 0x00000908 Interrupt USB Host Cause Low Register */
   __IM uint32_t RESERVED6[5];
   __IM uint32_t INTR_HOST_EP_CAUSE_HI;         /*!< 0x00000920 Interrupt USB Host Endpoint Cause High Register */
   __IM uint32_t INTR_HOST_EP_CAUSE_MED;        /*!< 0x00000924 Interrupt USB Host Endpoint Cause Medium Register */
   __IM uint32_t INTR_HOST_EP_CAUSE_LO;         /*!< 0x00000928 Interrupt USB Host Endpoint Cause Low Register */
   __IM uint32_t RESERVED7[5];
  __IOM uint32_t INTR_USBHOST;                  /*!< 0x00000940 Interrupt USB Host Register */
  __IOM uint32_t INTR_USBHOST_SET;              /*!< 0x00000944 Interrupt USB Host Set Register */
  __IOM uint32_t INTR_USBHOST_MASK;             /*!< 0x00000948 Interrupt USB Host Mask Register */
   __IM uint32_t INTR_USBHOST_MASKED;           /*!< 0x0000094C Interrupt USB Host Masked Register */
   __IM uint32_t RESERVED8[44];
  __IOM uint32_t INTR_HOST_EP;                  /*!< 0x00000A00 Interrupt USB Host Endpoint Register */
  __IOM uint32_t INTR_HOST_EP_SET;              /*!< 0x00000A04 Interrupt USB Host Endpoint Set Register */
  __IOM uint32_t INTR_HOST_EP_MASK;             /*!< 0x00000A08 Interrupt USB Host Endpoint Mask Register */
   __IM uint32_t INTR_HOST_EP_MASKED;           /*!< 0x00000A0C Interrupt USB Host Endpoint Masked Register */
   __IM uint32_t RESERVED9[60];
  __IOM uint32_t HOST_DMA_ENBL;                 /*!< 0x00000B00 Host DMA Enable Register */
   __IM uint32_t RESERVED10[7];
  __IOM uint32_t HOST_EP1_BLK;                  /*!< 0x00000B20 Host Endpoint 1 Block Register */
   __IM uint32_t RESERVED11[3];
  __IOM uint32_t HOST_EP2_BLK;                  /*!< 0x00000B30 Host Endpoint 2 Block Register */
   __IM uint32_t RESERVED12[1331];
} USBFS_USBHOST_V1_Type;                        /*!< Size = 8192 (0x2000) */

/**
  * \brief USB Host and Device Controller (USBFS)
  */
typedef struct {
        USBFS_USBDEV_V1_Type USBDEV;            /*!< 0x00000000 USB Device */
        USBFS_USBLPM_V1_Type USBLPM;            /*!< 0x00002000 USB Device LPM and PHY Test */
   __IM uint32_t RESERVED[1024];
        USBFS_USBHOST_V1_Type USBHOST;          /*!< 0x00004000 USB Host Controller */
} USBFS_V1_Type;                                /*!< Size = 24576 (0x6000) */


/* USBFS_USBDEV.EP0_DR */
#define USBFS_USBDEV_EP0_DR_DATA_BYTE_Pos       0UL
#define USBFS_USBDEV_EP0_DR_DATA_BYTE_Msk       0xFFUL
/* USBFS_USBDEV.CR0 */
#define USBFS_USBDEV_CR0_DEVICE_ADDRESS_Pos     0UL
#define USBFS_USBDEV_CR0_DEVICE_ADDRESS_Msk     0x7FUL
#define USBFS_USBDEV_CR0_USB_ENABLE_Pos         7UL
#define USBFS_USBDEV_CR0_USB_ENABLE_Msk         0x80UL
/* USBFS_USBDEV.CR1 */
#define USBFS_USBDEV_CR1_REG_ENABLE_Pos         0UL
#define USBFS_USBDEV_CR1_REG_ENABLE_Msk         0x1UL
#define USBFS_USBDEV_CR1_ENABLE_LOCK_Pos        1UL
#define USBFS_USBDEV_CR1_ENABLE_LOCK_Msk        0x2UL
#define USBFS_USBDEV_CR1_BUS_ACTIVITY_Pos       2UL
#define USBFS_USBDEV_CR1_BUS_ACTIVITY_Msk       0x4UL
#define USBFS_USBDEV_CR1_RESERVED_3_Pos         3UL
#define USBFS_USBDEV_CR1_RESERVED_3_Msk         0x8UL
/* USBFS_USBDEV.SIE_EP_INT_EN */
#define USBFS_USBDEV_SIE_EP_INT_EN_EP1_INTR_EN_Pos 0UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP1_INTR_EN_Msk 0x1UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP2_INTR_EN_Pos 1UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP2_INTR_EN_Msk 0x2UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP3_INTR_EN_Pos 2UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP3_INTR_EN_Msk 0x4UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP4_INTR_EN_Pos 3UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP4_INTR_EN_Msk 0x8UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP5_INTR_EN_Pos 4UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP5_INTR_EN_Msk 0x10UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP6_INTR_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP6_INTR_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP7_INTR_EN_Pos 6UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP7_INTR_EN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP8_INTR_EN_Pos 7UL
#define USBFS_USBDEV_SIE_EP_INT_EN_EP8_INTR_EN_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP_INT_SR */
#define USBFS_USBDEV_SIE_EP_INT_SR_EP1_INTR_Pos 0UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP1_INTR_Msk 0x1UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP2_INTR_Pos 1UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP2_INTR_Msk 0x2UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP3_INTR_Pos 2UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP3_INTR_Msk 0x4UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP4_INTR_Pos 3UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP4_INTR_Msk 0x8UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP5_INTR_Pos 4UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP5_INTR_Msk 0x10UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP6_INTR_Pos 5UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP6_INTR_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP7_INTR_Pos 6UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP7_INTR_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP8_INTR_Pos 7UL
#define USBFS_USBDEV_SIE_EP_INT_SR_EP8_INTR_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP1_CNT0 */
#define USBFS_USBDEV_SIE_EP1_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP1_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP1_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP1_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP1_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP1_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP1_CNT1 */
#define USBFS_USBDEV_SIE_EP1_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP1_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP1_CR0 */
#define USBFS_USBDEV_SIE_EP1_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP1_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP1_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP1_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP1_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP1_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP1_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP1_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP1_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP1_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.USBIO_CR0 */
#define USBFS_USBDEV_USBIO_CR0_RD_Pos           0UL
#define USBFS_USBDEV_USBIO_CR0_RD_Msk           0x1UL
#define USBFS_USBDEV_USBIO_CR0_TD_Pos           5UL
#define USBFS_USBDEV_USBIO_CR0_TD_Msk           0x20UL
#define USBFS_USBDEV_USBIO_CR0_TSE0_Pos         6UL
#define USBFS_USBDEV_USBIO_CR0_TSE0_Msk         0x40UL
#define USBFS_USBDEV_USBIO_CR0_TEN_Pos          7UL
#define USBFS_USBDEV_USBIO_CR0_TEN_Msk          0x80UL
/* USBFS_USBDEV.USBIO_CR2 */
#define USBFS_USBDEV_USBIO_CR2_RESERVED_5_0_Pos 0UL
#define USBFS_USBDEV_USBIO_CR2_RESERVED_5_0_Msk 0x3FUL
#define USBFS_USBDEV_USBIO_CR2_TEST_PKT_Pos     6UL
#define USBFS_USBDEV_USBIO_CR2_TEST_PKT_Msk     0x40UL
#define USBFS_USBDEV_USBIO_CR2_RESERVED_7_Pos   7UL
#define USBFS_USBDEV_USBIO_CR2_RESERVED_7_Msk   0x80UL
/* USBFS_USBDEV.USBIO_CR1 */
#define USBFS_USBDEV_USBIO_CR1_DMO_Pos          0UL
#define USBFS_USBDEV_USBIO_CR1_DMO_Msk          0x1UL
#define USBFS_USBDEV_USBIO_CR1_DPO_Pos          1UL
#define USBFS_USBDEV_USBIO_CR1_DPO_Msk          0x2UL
#define USBFS_USBDEV_USBIO_CR1_RESERVED_2_Pos   2UL
#define USBFS_USBDEV_USBIO_CR1_RESERVED_2_Msk   0x4UL
#define USBFS_USBDEV_USBIO_CR1_IOMODE_Pos       5UL
#define USBFS_USBDEV_USBIO_CR1_IOMODE_Msk       0x20UL
/* USBFS_USBDEV.DYN_RECONFIG */
#define USBFS_USBDEV_DYN_RECONFIG_DYN_CONFIG_EN_Pos 0UL
#define USBFS_USBDEV_DYN_RECONFIG_DYN_CONFIG_EN_Msk 0x1UL
#define USBFS_USBDEV_DYN_RECONFIG_DYN_RECONFIG_EPNO_Pos 1UL
#define USBFS_USBDEV_DYN_RECONFIG_DYN_RECONFIG_EPNO_Msk 0xEUL
#define USBFS_USBDEV_DYN_RECONFIG_DYN_RECONFIG_RDY_STS_Pos 4UL
#define USBFS_USBDEV_DYN_RECONFIG_DYN_RECONFIG_RDY_STS_Msk 0x10UL
/* USBFS_USBDEV.SOF0 */
#define USBFS_USBDEV_SOF0_FRAME_NUMBER_Pos      0UL
#define USBFS_USBDEV_SOF0_FRAME_NUMBER_Msk      0xFFUL
/* USBFS_USBDEV.SOF1 */
#define USBFS_USBDEV_SOF1_FRAME_NUMBER_MSB_Pos  0UL
#define USBFS_USBDEV_SOF1_FRAME_NUMBER_MSB_Msk  0x7UL
/* USBFS_USBDEV.SIE_EP2_CNT0 */
#define USBFS_USBDEV_SIE_EP2_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP2_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP2_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP2_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP2_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP2_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP2_CNT1 */
#define USBFS_USBDEV_SIE_EP2_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP2_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP2_CR0 */
#define USBFS_USBDEV_SIE_EP2_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP2_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP2_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP2_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP2_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP2_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP2_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP2_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP2_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP2_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.OSCLK_DR0 */
#define USBFS_USBDEV_OSCLK_DR0_ADDER_Pos        0UL
#define USBFS_USBDEV_OSCLK_DR0_ADDER_Msk        0xFFUL
/* USBFS_USBDEV.OSCLK_DR1 */
#define USBFS_USBDEV_OSCLK_DR1_ADDER_MSB_Pos    0UL
#define USBFS_USBDEV_OSCLK_DR1_ADDER_MSB_Msk    0x7FUL
/* USBFS_USBDEV.EP0_CR */
#define USBFS_USBDEV_EP0_CR_MODE_Pos            0UL
#define USBFS_USBDEV_EP0_CR_MODE_Msk            0xFUL
#define USBFS_USBDEV_EP0_CR_ACKED_TXN_Pos       4UL
#define USBFS_USBDEV_EP0_CR_ACKED_TXN_Msk       0x10UL
#define USBFS_USBDEV_EP0_CR_OUT_RCVD_Pos        5UL
#define USBFS_USBDEV_EP0_CR_OUT_RCVD_Msk        0x20UL
#define USBFS_USBDEV_EP0_CR_IN_RCVD_Pos         6UL
#define USBFS_USBDEV_EP0_CR_IN_RCVD_Msk         0x40UL
#define USBFS_USBDEV_EP0_CR_SETUP_RCVD_Pos      7UL
#define USBFS_USBDEV_EP0_CR_SETUP_RCVD_Msk      0x80UL
/* USBFS_USBDEV.EP0_CNT */
#define USBFS_USBDEV_EP0_CNT_BYTE_COUNT_Pos     0UL
#define USBFS_USBDEV_EP0_CNT_BYTE_COUNT_Msk     0xFUL
#define USBFS_USBDEV_EP0_CNT_DATA_VALID_Pos     6UL
#define USBFS_USBDEV_EP0_CNT_DATA_VALID_Msk     0x40UL
#define USBFS_USBDEV_EP0_CNT_DATA_TOGGLE_Pos    7UL
#define USBFS_USBDEV_EP0_CNT_DATA_TOGGLE_Msk    0x80UL
/* USBFS_USBDEV.SIE_EP3_CNT0 */
#define USBFS_USBDEV_SIE_EP3_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP3_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP3_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP3_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP3_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP3_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP3_CNT1 */
#define USBFS_USBDEV_SIE_EP3_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP3_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP3_CR0 */
#define USBFS_USBDEV_SIE_EP3_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP3_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP3_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP3_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP3_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP3_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP3_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP3_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP3_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP3_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.SIE_EP4_CNT0 */
#define USBFS_USBDEV_SIE_EP4_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP4_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP4_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP4_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP4_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP4_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP4_CNT1 */
#define USBFS_USBDEV_SIE_EP4_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP4_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP4_CR0 */
#define USBFS_USBDEV_SIE_EP4_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP4_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP4_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP4_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP4_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP4_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP4_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP4_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP4_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP4_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.SIE_EP5_CNT0 */
#define USBFS_USBDEV_SIE_EP5_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP5_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP5_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP5_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP5_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP5_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP5_CNT1 */
#define USBFS_USBDEV_SIE_EP5_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP5_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP5_CR0 */
#define USBFS_USBDEV_SIE_EP5_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP5_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP5_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP5_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP5_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP5_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP5_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP5_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP5_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP5_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.SIE_EP6_CNT0 */
#define USBFS_USBDEV_SIE_EP6_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP6_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP6_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP6_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP6_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP6_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP6_CNT1 */
#define USBFS_USBDEV_SIE_EP6_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP6_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP6_CR0 */
#define USBFS_USBDEV_SIE_EP6_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP6_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP6_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP6_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP6_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP6_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP6_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP6_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP6_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP6_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.SIE_EP7_CNT0 */
#define USBFS_USBDEV_SIE_EP7_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP7_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP7_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP7_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP7_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP7_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP7_CNT1 */
#define USBFS_USBDEV_SIE_EP7_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP7_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP7_CR0 */
#define USBFS_USBDEV_SIE_EP7_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP7_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP7_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP7_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP7_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP7_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP7_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP7_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP7_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP7_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.SIE_EP8_CNT0 */
#define USBFS_USBDEV_SIE_EP8_CNT0_DATA_COUNT_MSB_Pos 0UL
#define USBFS_USBDEV_SIE_EP8_CNT0_DATA_COUNT_MSB_Msk 0x7UL
#define USBFS_USBDEV_SIE_EP8_CNT0_DATA_VALID_Pos 6UL
#define USBFS_USBDEV_SIE_EP8_CNT0_DATA_VALID_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP8_CNT0_DATA_TOGGLE_Pos 7UL
#define USBFS_USBDEV_SIE_EP8_CNT0_DATA_TOGGLE_Msk 0x80UL
/* USBFS_USBDEV.SIE_EP8_CNT1 */
#define USBFS_USBDEV_SIE_EP8_CNT1_DATA_COUNT_Pos 0UL
#define USBFS_USBDEV_SIE_EP8_CNT1_DATA_COUNT_Msk 0xFFUL
/* USBFS_USBDEV.SIE_EP8_CR0 */
#define USBFS_USBDEV_SIE_EP8_CR0_MODE_Pos       0UL
#define USBFS_USBDEV_SIE_EP8_CR0_MODE_Msk       0xFUL
#define USBFS_USBDEV_SIE_EP8_CR0_ACKED_TXN_Pos  4UL
#define USBFS_USBDEV_SIE_EP8_CR0_ACKED_TXN_Msk  0x10UL
#define USBFS_USBDEV_SIE_EP8_CR0_NAK_INT_EN_Pos 5UL
#define USBFS_USBDEV_SIE_EP8_CR0_NAK_INT_EN_Msk 0x20UL
#define USBFS_USBDEV_SIE_EP8_CR0_ERR_IN_TXN_Pos 6UL
#define USBFS_USBDEV_SIE_EP8_CR0_ERR_IN_TXN_Msk 0x40UL
#define USBFS_USBDEV_SIE_EP8_CR0_STALL_Pos      7UL
#define USBFS_USBDEV_SIE_EP8_CR0_STALL_Msk      0x80UL
/* USBFS_USBDEV.ARB_EP1_CFG */
#define USBFS_USBDEV_ARB_EP1_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP1_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP1_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP1_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP1_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP1_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP1_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP1_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP1_INT_EN */
#define USBFS_USBDEV_ARB_EP1_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP1_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP1_SR */
#define USBFS_USBDEV_ARB_EP1_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP1_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP1_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP1_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP1_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP1_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP1_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP1_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP1_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP1_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW1_WA */
#define USBFS_USBDEV_ARB_RW1_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW1_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW1_WA_MSB */
#define USBFS_USBDEV_ARB_RW1_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW1_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW1_RA */
#define USBFS_USBDEV_ARB_RW1_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW1_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW1_RA_MSB */
#define USBFS_USBDEV_ARB_RW1_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW1_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW1_DR */
#define USBFS_USBDEV_ARB_RW1_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW1_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.BUF_SIZE */
#define USBFS_USBDEV_BUF_SIZE_IN_BUF_Pos        0UL
#define USBFS_USBDEV_BUF_SIZE_IN_BUF_Msk        0xFUL
#define USBFS_USBDEV_BUF_SIZE_OUT_BUF_Pos       4UL
#define USBFS_USBDEV_BUF_SIZE_OUT_BUF_Msk       0xF0UL
/* USBFS_USBDEV.EP_ACTIVE */
#define USBFS_USBDEV_EP_ACTIVE_EP1_ACT_Pos      0UL
#define USBFS_USBDEV_EP_ACTIVE_EP1_ACT_Msk      0x1UL
#define USBFS_USBDEV_EP_ACTIVE_EP2_ACT_Pos      1UL
#define USBFS_USBDEV_EP_ACTIVE_EP2_ACT_Msk      0x2UL
#define USBFS_USBDEV_EP_ACTIVE_EP3_ACT_Pos      2UL
#define USBFS_USBDEV_EP_ACTIVE_EP3_ACT_Msk      0x4UL
#define USBFS_USBDEV_EP_ACTIVE_EP4_ACT_Pos      3UL
#define USBFS_USBDEV_EP_ACTIVE_EP4_ACT_Msk      0x8UL
#define USBFS_USBDEV_EP_ACTIVE_EP5_ACT_Pos      4UL
#define USBFS_USBDEV_EP_ACTIVE_EP5_ACT_Msk      0x10UL
#define USBFS_USBDEV_EP_ACTIVE_EP6_ACT_Pos      5UL
#define USBFS_USBDEV_EP_ACTIVE_EP6_ACT_Msk      0x20UL
#define USBFS_USBDEV_EP_ACTIVE_EP7_ACT_Pos      6UL
#define USBFS_USBDEV_EP_ACTIVE_EP7_ACT_Msk      0x40UL
#define USBFS_USBDEV_EP_ACTIVE_EP8_ACT_Pos      7UL
#define USBFS_USBDEV_EP_ACTIVE_EP8_ACT_Msk      0x80UL
/* USBFS_USBDEV.EP_TYPE */
#define USBFS_USBDEV_EP_TYPE_EP1_TYP_Pos        0UL
#define USBFS_USBDEV_EP_TYPE_EP1_TYP_Msk        0x1UL
#define USBFS_USBDEV_EP_TYPE_EP2_TYP_Pos        1UL
#define USBFS_USBDEV_EP_TYPE_EP2_TYP_Msk        0x2UL
#define USBFS_USBDEV_EP_TYPE_EP3_TYP_Pos        2UL
#define USBFS_USBDEV_EP_TYPE_EP3_TYP_Msk        0x4UL
#define USBFS_USBDEV_EP_TYPE_EP4_TYP_Pos        3UL
#define USBFS_USBDEV_EP_TYPE_EP4_TYP_Msk        0x8UL
#define USBFS_USBDEV_EP_TYPE_EP5_TYP_Pos        4UL
#define USBFS_USBDEV_EP_TYPE_EP5_TYP_Msk        0x10UL
#define USBFS_USBDEV_EP_TYPE_EP6_TYP_Pos        5UL
#define USBFS_USBDEV_EP_TYPE_EP6_TYP_Msk        0x20UL
#define USBFS_USBDEV_EP_TYPE_EP7_TYP_Pos        6UL
#define USBFS_USBDEV_EP_TYPE_EP7_TYP_Msk        0x40UL
#define USBFS_USBDEV_EP_TYPE_EP8_TYP_Pos        7UL
#define USBFS_USBDEV_EP_TYPE_EP8_TYP_Msk        0x80UL
/* USBFS_USBDEV.ARB_EP2_CFG */
#define USBFS_USBDEV_ARB_EP2_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP2_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP2_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP2_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP2_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP2_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP2_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP2_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP2_INT_EN */
#define USBFS_USBDEV_ARB_EP2_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP2_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP2_SR */
#define USBFS_USBDEV_ARB_EP2_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP2_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP2_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP2_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP2_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP2_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP2_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP2_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP2_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP2_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW2_WA */
#define USBFS_USBDEV_ARB_RW2_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW2_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW2_WA_MSB */
#define USBFS_USBDEV_ARB_RW2_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW2_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW2_RA */
#define USBFS_USBDEV_ARB_RW2_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW2_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW2_RA_MSB */
#define USBFS_USBDEV_ARB_RW2_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW2_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW2_DR */
#define USBFS_USBDEV_ARB_RW2_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW2_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.ARB_CFG */
#define USBFS_USBDEV_ARB_CFG_AUTO_MEM_Pos       4UL
#define USBFS_USBDEV_ARB_CFG_AUTO_MEM_Msk       0x10UL
#define USBFS_USBDEV_ARB_CFG_DMA_CFG_Pos        5UL
#define USBFS_USBDEV_ARB_CFG_DMA_CFG_Msk        0x60UL
#define USBFS_USBDEV_ARB_CFG_CFG_CMP_Pos        7UL
#define USBFS_USBDEV_ARB_CFG_CFG_CMP_Msk        0x80UL
/* USBFS_USBDEV.USB_CLK_EN */
#define USBFS_USBDEV_USB_CLK_EN_CSR_CLK_EN_Pos  0UL
#define USBFS_USBDEV_USB_CLK_EN_CSR_CLK_EN_Msk  0x1UL
/* USBFS_USBDEV.ARB_INT_EN */
#define USBFS_USBDEV_ARB_INT_EN_EP1_INTR_EN_Pos 0UL
#define USBFS_USBDEV_ARB_INT_EN_EP1_INTR_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_INT_EN_EP2_INTR_EN_Pos 1UL
#define USBFS_USBDEV_ARB_INT_EN_EP2_INTR_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_INT_EN_EP3_INTR_EN_Pos 2UL
#define USBFS_USBDEV_ARB_INT_EN_EP3_INTR_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_INT_EN_EP4_INTR_EN_Pos 3UL
#define USBFS_USBDEV_ARB_INT_EN_EP4_INTR_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_INT_EN_EP5_INTR_EN_Pos 4UL
#define USBFS_USBDEV_ARB_INT_EN_EP5_INTR_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_INT_EN_EP6_INTR_EN_Pos 5UL
#define USBFS_USBDEV_ARB_INT_EN_EP6_INTR_EN_Msk 0x20UL
#define USBFS_USBDEV_ARB_INT_EN_EP7_INTR_EN_Pos 6UL
#define USBFS_USBDEV_ARB_INT_EN_EP7_INTR_EN_Msk 0x40UL
#define USBFS_USBDEV_ARB_INT_EN_EP8_INTR_EN_Pos 7UL
#define USBFS_USBDEV_ARB_INT_EN_EP8_INTR_EN_Msk 0x80UL
/* USBFS_USBDEV.ARB_INT_SR */
#define USBFS_USBDEV_ARB_INT_SR_EP1_INTR_Pos    0UL
#define USBFS_USBDEV_ARB_INT_SR_EP1_INTR_Msk    0x1UL
#define USBFS_USBDEV_ARB_INT_SR_EP2_INTR_Pos    1UL
#define USBFS_USBDEV_ARB_INT_SR_EP2_INTR_Msk    0x2UL
#define USBFS_USBDEV_ARB_INT_SR_EP3_INTR_Pos    2UL
#define USBFS_USBDEV_ARB_INT_SR_EP3_INTR_Msk    0x4UL
#define USBFS_USBDEV_ARB_INT_SR_EP4_INTR_Pos    3UL
#define USBFS_USBDEV_ARB_INT_SR_EP4_INTR_Msk    0x8UL
#define USBFS_USBDEV_ARB_INT_SR_EP5_INTR_Pos    4UL
#define USBFS_USBDEV_ARB_INT_SR_EP5_INTR_Msk    0x10UL
#define USBFS_USBDEV_ARB_INT_SR_EP6_INTR_Pos    5UL
#define USBFS_USBDEV_ARB_INT_SR_EP6_INTR_Msk    0x20UL
#define USBFS_USBDEV_ARB_INT_SR_EP7_INTR_Pos    6UL
#define USBFS_USBDEV_ARB_INT_SR_EP7_INTR_Msk    0x40UL
#define USBFS_USBDEV_ARB_INT_SR_EP8_INTR_Pos    7UL
#define USBFS_USBDEV_ARB_INT_SR_EP8_INTR_Msk    0x80UL
/* USBFS_USBDEV.ARB_EP3_CFG */
#define USBFS_USBDEV_ARB_EP3_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP3_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP3_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP3_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP3_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP3_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP3_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP3_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP3_INT_EN */
#define USBFS_USBDEV_ARB_EP3_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP3_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP3_SR */
#define USBFS_USBDEV_ARB_EP3_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP3_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP3_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP3_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP3_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP3_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP3_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP3_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP3_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP3_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW3_WA */
#define USBFS_USBDEV_ARB_RW3_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW3_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW3_WA_MSB */
#define USBFS_USBDEV_ARB_RW3_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW3_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW3_RA */
#define USBFS_USBDEV_ARB_RW3_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW3_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW3_RA_MSB */
#define USBFS_USBDEV_ARB_RW3_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW3_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW3_DR */
#define USBFS_USBDEV_ARB_RW3_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW3_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.CWA */
#define USBFS_USBDEV_CWA_CWA_Pos                0UL
#define USBFS_USBDEV_CWA_CWA_Msk                0xFFUL
/* USBFS_USBDEV.CWA_MSB */
#define USBFS_USBDEV_CWA_MSB_CWA_MSB_Pos        0UL
#define USBFS_USBDEV_CWA_MSB_CWA_MSB_Msk        0x1UL
/* USBFS_USBDEV.ARB_EP4_CFG */
#define USBFS_USBDEV_ARB_EP4_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP4_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP4_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP4_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP4_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP4_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP4_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP4_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP4_INT_EN */
#define USBFS_USBDEV_ARB_EP4_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP4_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP4_SR */
#define USBFS_USBDEV_ARB_EP4_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP4_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP4_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP4_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP4_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP4_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP4_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP4_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP4_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP4_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW4_WA */
#define USBFS_USBDEV_ARB_RW4_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW4_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW4_WA_MSB */
#define USBFS_USBDEV_ARB_RW4_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW4_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW4_RA */
#define USBFS_USBDEV_ARB_RW4_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW4_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW4_RA_MSB */
#define USBFS_USBDEV_ARB_RW4_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW4_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW4_DR */
#define USBFS_USBDEV_ARB_RW4_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW4_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.DMA_THRES */
#define USBFS_USBDEV_DMA_THRES_DMA_THS_Pos      0UL
#define USBFS_USBDEV_DMA_THRES_DMA_THS_Msk      0xFFUL
/* USBFS_USBDEV.DMA_THRES_MSB */
#define USBFS_USBDEV_DMA_THRES_MSB_DMA_THS_MSB_Pos 0UL
#define USBFS_USBDEV_DMA_THRES_MSB_DMA_THS_MSB_Msk 0x1UL
/* USBFS_USBDEV.ARB_EP5_CFG */
#define USBFS_USBDEV_ARB_EP5_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP5_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP5_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP5_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP5_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP5_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP5_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP5_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP5_INT_EN */
#define USBFS_USBDEV_ARB_EP5_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP5_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP5_SR */
#define USBFS_USBDEV_ARB_EP5_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP5_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP5_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP5_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP5_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP5_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP5_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP5_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP5_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP5_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW5_WA */
#define USBFS_USBDEV_ARB_RW5_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW5_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW5_WA_MSB */
#define USBFS_USBDEV_ARB_RW5_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW5_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW5_RA */
#define USBFS_USBDEV_ARB_RW5_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW5_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW5_RA_MSB */
#define USBFS_USBDEV_ARB_RW5_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW5_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW5_DR */
#define USBFS_USBDEV_ARB_RW5_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW5_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.BUS_RST_CNT */
#define USBFS_USBDEV_BUS_RST_CNT_BUS_RST_CNT_Pos 0UL
#define USBFS_USBDEV_BUS_RST_CNT_BUS_RST_CNT_Msk 0xFUL
/* USBFS_USBDEV.ARB_EP6_CFG */
#define USBFS_USBDEV_ARB_EP6_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP6_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP6_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP6_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP6_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP6_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP6_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP6_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP6_INT_EN */
#define USBFS_USBDEV_ARB_EP6_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP6_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP6_SR */
#define USBFS_USBDEV_ARB_EP6_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP6_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP6_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP6_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP6_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP6_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP6_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP6_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP6_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP6_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW6_WA */
#define USBFS_USBDEV_ARB_RW6_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW6_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW6_WA_MSB */
#define USBFS_USBDEV_ARB_RW6_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW6_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW6_RA */
#define USBFS_USBDEV_ARB_RW6_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW6_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW6_RA_MSB */
#define USBFS_USBDEV_ARB_RW6_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW6_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW6_DR */
#define USBFS_USBDEV_ARB_RW6_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW6_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.ARB_EP7_CFG */
#define USBFS_USBDEV_ARB_EP7_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP7_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP7_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP7_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP7_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP7_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP7_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP7_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP7_INT_EN */
#define USBFS_USBDEV_ARB_EP7_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP7_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP7_SR */
#define USBFS_USBDEV_ARB_EP7_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP7_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP7_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP7_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP7_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP7_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP7_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP7_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP7_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP7_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW7_WA */
#define USBFS_USBDEV_ARB_RW7_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW7_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW7_WA_MSB */
#define USBFS_USBDEV_ARB_RW7_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW7_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW7_RA */
#define USBFS_USBDEV_ARB_RW7_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW7_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW7_RA_MSB */
#define USBFS_USBDEV_ARB_RW7_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW7_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW7_DR */
#define USBFS_USBDEV_ARB_RW7_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW7_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.ARB_EP8_CFG */
#define USBFS_USBDEV_ARB_EP8_CFG_IN_DATA_RDY_Pos 0UL
#define USBFS_USBDEV_ARB_EP8_CFG_IN_DATA_RDY_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP8_CFG_DMA_REQ_Pos    1UL
#define USBFS_USBDEV_ARB_EP8_CFG_DMA_REQ_Msk    0x2UL
#define USBFS_USBDEV_ARB_EP8_CFG_CRC_BYPASS_Pos 2UL
#define USBFS_USBDEV_ARB_EP8_CFG_CRC_BYPASS_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP8_CFG_RESET_PTR_Pos  3UL
#define USBFS_USBDEV_ARB_EP8_CFG_RESET_PTR_Msk  0x8UL
/* USBFS_USBDEV.ARB_EP8_INT_EN */
#define USBFS_USBDEV_ARB_EP8_INT_EN_IN_BUF_FULL_EN_Pos 0UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_IN_BUF_FULL_EN_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_DMA_GNT_EN_Pos 1UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_DMA_GNT_EN_Msk 0x2UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_BUF_OVER_EN_Pos 2UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_BUF_OVER_EN_Msk 0x4UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_BUF_UNDER_EN_Pos 3UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_BUF_UNDER_EN_Msk 0x8UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_ERR_INT_EN_Pos 4UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_ERR_INT_EN_Msk 0x10UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_DMA_TERMIN_EN_Pos 5UL
#define USBFS_USBDEV_ARB_EP8_INT_EN_DMA_TERMIN_EN_Msk 0x20UL
/* USBFS_USBDEV.ARB_EP8_SR */
#define USBFS_USBDEV_ARB_EP8_SR_IN_BUF_FULL_Pos 0UL
#define USBFS_USBDEV_ARB_EP8_SR_IN_BUF_FULL_Msk 0x1UL
#define USBFS_USBDEV_ARB_EP8_SR_DMA_GNT_Pos     1UL
#define USBFS_USBDEV_ARB_EP8_SR_DMA_GNT_Msk     0x2UL
#define USBFS_USBDEV_ARB_EP8_SR_BUF_OVER_Pos    2UL
#define USBFS_USBDEV_ARB_EP8_SR_BUF_OVER_Msk    0x4UL
#define USBFS_USBDEV_ARB_EP8_SR_BUF_UNDER_Pos   3UL
#define USBFS_USBDEV_ARB_EP8_SR_BUF_UNDER_Msk   0x8UL
#define USBFS_USBDEV_ARB_EP8_SR_DMA_TERMIN_Pos  5UL
#define USBFS_USBDEV_ARB_EP8_SR_DMA_TERMIN_Msk  0x20UL
/* USBFS_USBDEV.ARB_RW8_WA */
#define USBFS_USBDEV_ARB_RW8_WA_WA_Pos          0UL
#define USBFS_USBDEV_ARB_RW8_WA_WA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW8_WA_MSB */
#define USBFS_USBDEV_ARB_RW8_WA_MSB_WA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW8_WA_MSB_WA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW8_RA */
#define USBFS_USBDEV_ARB_RW8_RA_RA_Pos          0UL
#define USBFS_USBDEV_ARB_RW8_RA_RA_Msk          0xFFUL
/* USBFS_USBDEV.ARB_RW8_RA_MSB */
#define USBFS_USBDEV_ARB_RW8_RA_MSB_RA_MSB_Pos  0UL
#define USBFS_USBDEV_ARB_RW8_RA_MSB_RA_MSB_Msk  0x1UL
/* USBFS_USBDEV.ARB_RW8_DR */
#define USBFS_USBDEV_ARB_RW8_DR_DR_Pos          0UL
#define USBFS_USBDEV_ARB_RW8_DR_DR_Msk          0xFFUL
/* USBFS_USBDEV.MEM_DATA */
#define USBFS_USBDEV_MEM_DATA_DR_Pos            0UL
#define USBFS_USBDEV_MEM_DATA_DR_Msk            0xFFUL
/* USBFS_USBDEV.SOF16 */
#define USBFS_USBDEV_SOF16_FRAME_NUMBER16_Pos   0UL
#define USBFS_USBDEV_SOF16_FRAME_NUMBER16_Msk   0x7FFUL
/* USBFS_USBDEV.OSCLK_DR16 */
#define USBFS_USBDEV_OSCLK_DR16_ADDER16_Pos     0UL
#define USBFS_USBDEV_OSCLK_DR16_ADDER16_Msk     0x7FFFUL
/* USBFS_USBDEV.ARB_RW1_WA16 */
#define USBFS_USBDEV_ARB_RW1_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW1_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW1_RA16 */
#define USBFS_USBDEV_ARB_RW1_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW1_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW1_DR16 */
#define USBFS_USBDEV_ARB_RW1_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW1_DR16_DR16_Msk      0xFFFFUL
/* USBFS_USBDEV.ARB_RW2_WA16 */
#define USBFS_USBDEV_ARB_RW2_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW2_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW2_RA16 */
#define USBFS_USBDEV_ARB_RW2_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW2_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW2_DR16 */
#define USBFS_USBDEV_ARB_RW2_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW2_DR16_DR16_Msk      0xFFFFUL
/* USBFS_USBDEV.ARB_RW3_WA16 */
#define USBFS_USBDEV_ARB_RW3_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW3_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW3_RA16 */
#define USBFS_USBDEV_ARB_RW3_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW3_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW3_DR16 */
#define USBFS_USBDEV_ARB_RW3_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW3_DR16_DR16_Msk      0xFFFFUL
/* USBFS_USBDEV.CWA16 */
#define USBFS_USBDEV_CWA16_CWA16_Pos            0UL
#define USBFS_USBDEV_CWA16_CWA16_Msk            0x1FFUL
/* USBFS_USBDEV.ARB_RW4_WA16 */
#define USBFS_USBDEV_ARB_RW4_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW4_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW4_RA16 */
#define USBFS_USBDEV_ARB_RW4_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW4_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW4_DR16 */
#define USBFS_USBDEV_ARB_RW4_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW4_DR16_DR16_Msk      0xFFFFUL
/* USBFS_USBDEV.DMA_THRES16 */
#define USBFS_USBDEV_DMA_THRES16_DMA_THS16_Pos  0UL
#define USBFS_USBDEV_DMA_THRES16_DMA_THS16_Msk  0x1FFUL
/* USBFS_USBDEV.ARB_RW5_WA16 */
#define USBFS_USBDEV_ARB_RW5_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW5_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW5_RA16 */
#define USBFS_USBDEV_ARB_RW5_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW5_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW5_DR16 */
#define USBFS_USBDEV_ARB_RW5_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW5_DR16_DR16_Msk      0xFFFFUL
/* USBFS_USBDEV.ARB_RW6_WA16 */
#define USBFS_USBDEV_ARB_RW6_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW6_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW6_RA16 */
#define USBFS_USBDEV_ARB_RW6_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW6_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW6_DR16 */
#define USBFS_USBDEV_ARB_RW6_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW6_DR16_DR16_Msk      0xFFFFUL
/* USBFS_USBDEV.ARB_RW7_WA16 */
#define USBFS_USBDEV_ARB_RW7_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW7_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW7_RA16 */
#define USBFS_USBDEV_ARB_RW7_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW7_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW7_DR16 */
#define USBFS_USBDEV_ARB_RW7_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW7_DR16_DR16_Msk      0xFFFFUL
/* USBFS_USBDEV.ARB_RW8_WA16 */
#define USBFS_USBDEV_ARB_RW8_WA16_WA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW8_WA16_WA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW8_RA16 */
#define USBFS_USBDEV_ARB_RW8_RA16_RA16_Pos      0UL
#define USBFS_USBDEV_ARB_RW8_RA16_RA16_Msk      0x1FFUL
/* USBFS_USBDEV.ARB_RW8_DR16 */
#define USBFS_USBDEV_ARB_RW8_DR16_DR16_Pos      0UL
#define USBFS_USBDEV_ARB_RW8_DR16_DR16_Msk      0xFFFFUL


/* USBFS_USBLPM.POWER_CTL */
#define USBFS_USBLPM_POWER_CTL_SUSPEND_Pos      2UL
#define USBFS_USBLPM_POWER_CTL_SUSPEND_Msk      0x4UL
#define USBFS_USBLPM_POWER_CTL_DP_UP_EN_Pos     16UL
#define USBFS_USBLPM_POWER_CTL_DP_UP_EN_Msk     0x10000UL
#define USBFS_USBLPM_POWER_CTL_DP_BIG_Pos       17UL
#define USBFS_USBLPM_POWER_CTL_DP_BIG_Msk       0x20000UL
#define USBFS_USBLPM_POWER_CTL_DP_DOWN_EN_Pos   18UL
#define USBFS_USBLPM_POWER_CTL_DP_DOWN_EN_Msk   0x40000UL
#define USBFS_USBLPM_POWER_CTL_DM_UP_EN_Pos     19UL
#define USBFS_USBLPM_POWER_CTL_DM_UP_EN_Msk     0x80000UL
#define USBFS_USBLPM_POWER_CTL_DM_BIG_Pos       20UL
#define USBFS_USBLPM_POWER_CTL_DM_BIG_Msk       0x100000UL
#define USBFS_USBLPM_POWER_CTL_DM_DOWN_EN_Pos   21UL
#define USBFS_USBLPM_POWER_CTL_DM_DOWN_EN_Msk   0x200000UL
#define USBFS_USBLPM_POWER_CTL_ENABLE_DPO_Pos   28UL
#define USBFS_USBLPM_POWER_CTL_ENABLE_DPO_Msk   0x10000000UL
#define USBFS_USBLPM_POWER_CTL_ENABLE_DMO_Pos   29UL
#define USBFS_USBLPM_POWER_CTL_ENABLE_DMO_Msk   0x20000000UL
/* USBFS_USBLPM.USBIO_CTL */
#define USBFS_USBLPM_USBIO_CTL_DM_P_Pos         0UL
#define USBFS_USBLPM_USBIO_CTL_DM_P_Msk         0x7UL
#define USBFS_USBLPM_USBIO_CTL_DM_M_Pos         3UL
#define USBFS_USBLPM_USBIO_CTL_DM_M_Msk         0x38UL
/* USBFS_USBLPM.FLOW_CTL */
#define USBFS_USBLPM_FLOW_CTL_EP1_ERR_RESP_Pos  0UL
#define USBFS_USBLPM_FLOW_CTL_EP1_ERR_RESP_Msk  0x1UL
#define USBFS_USBLPM_FLOW_CTL_EP2_ERR_RESP_Pos  1UL
#define USBFS_USBLPM_FLOW_CTL_EP2_ERR_RESP_Msk  0x2UL
#define USBFS_USBLPM_FLOW_CTL_EP3_ERR_RESP_Pos  2UL
#define USBFS_USBLPM_FLOW_CTL_EP3_ERR_RESP_Msk  0x4UL
#define USBFS_USBLPM_FLOW_CTL_EP4_ERR_RESP_Pos  3UL
#define USBFS_USBLPM_FLOW_CTL_EP4_ERR_RESP_Msk  0x8UL
#define USBFS_USBLPM_FLOW_CTL_EP5_ERR_RESP_Pos  4UL
#define USBFS_USBLPM_FLOW_CTL_EP5_ERR_RESP_Msk  0x10UL
#define USBFS_USBLPM_FLOW_CTL_EP6_ERR_RESP_Pos  5UL
#define USBFS_USBLPM_FLOW_CTL_EP6_ERR_RESP_Msk  0x20UL
#define USBFS_USBLPM_FLOW_CTL_EP7_ERR_RESP_Pos  6UL
#define USBFS_USBLPM_FLOW_CTL_EP7_ERR_RESP_Msk  0x40UL
#define USBFS_USBLPM_FLOW_CTL_EP8_ERR_RESP_Pos  7UL
#define USBFS_USBLPM_FLOW_CTL_EP8_ERR_RESP_Msk  0x80UL
/* USBFS_USBLPM.LPM_CTL */
#define USBFS_USBLPM_LPM_CTL_LPM_EN_Pos         0UL
#define USBFS_USBLPM_LPM_CTL_LPM_EN_Msk         0x1UL
#define USBFS_USBLPM_LPM_CTL_LPM_ACK_RESP_Pos   1UL
#define USBFS_USBLPM_LPM_CTL_LPM_ACK_RESP_Msk   0x2UL
#define USBFS_USBLPM_LPM_CTL_NYET_EN_Pos        2UL
#define USBFS_USBLPM_LPM_CTL_NYET_EN_Msk        0x4UL
#define USBFS_USBLPM_LPM_CTL_SUB_RESP_Pos       4UL
#define USBFS_USBLPM_LPM_CTL_SUB_RESP_Msk       0x10UL
/* USBFS_USBLPM.LPM_STAT */
#define USBFS_USBLPM_LPM_STAT_LPM_BESL_Pos      0UL
#define USBFS_USBLPM_LPM_STAT_LPM_BESL_Msk      0xFUL
#define USBFS_USBLPM_LPM_STAT_LPM_REMOTEWAKE_Pos 4UL
#define USBFS_USBLPM_LPM_STAT_LPM_REMOTEWAKE_Msk 0x10UL
/* USBFS_USBLPM.INTR_SIE */
#define USBFS_USBLPM_INTR_SIE_SOF_INTR_Pos      0UL
#define USBFS_USBLPM_INTR_SIE_SOF_INTR_Msk      0x1UL
#define USBFS_USBLPM_INTR_SIE_BUS_RESET_INTR_Pos 1UL
#define USBFS_USBLPM_INTR_SIE_BUS_RESET_INTR_Msk 0x2UL
#define USBFS_USBLPM_INTR_SIE_EP0_INTR_Pos      2UL
#define USBFS_USBLPM_INTR_SIE_EP0_INTR_Msk      0x4UL
#define USBFS_USBLPM_INTR_SIE_LPM_INTR_Pos      3UL
#define USBFS_USBLPM_INTR_SIE_LPM_INTR_Msk      0x8UL
#define USBFS_USBLPM_INTR_SIE_RESUME_INTR_Pos   4UL
#define USBFS_USBLPM_INTR_SIE_RESUME_INTR_Msk   0x10UL
/* USBFS_USBLPM.INTR_SIE_SET */
#define USBFS_USBLPM_INTR_SIE_SET_SOF_INTR_SET_Pos 0UL
#define USBFS_USBLPM_INTR_SIE_SET_SOF_INTR_SET_Msk 0x1UL
#define USBFS_USBLPM_INTR_SIE_SET_BUS_RESET_INTR_SET_Pos 1UL
#define USBFS_USBLPM_INTR_SIE_SET_BUS_RESET_INTR_SET_Msk 0x2UL
#define USBFS_USBLPM_INTR_SIE_SET_EP0_INTR_SET_Pos 2UL
#define USBFS_USBLPM_INTR_SIE_SET_EP0_INTR_SET_Msk 0x4UL
#define USBFS_USBLPM_INTR_SIE_SET_LPM_INTR_SET_Pos 3UL
#define USBFS_USBLPM_INTR_SIE_SET_LPM_INTR_SET_Msk 0x8UL
#define USBFS_USBLPM_INTR_SIE_SET_RESUME_INTR_SET_Pos 4UL
#define USBFS_USBLPM_INTR_SIE_SET_RESUME_INTR_SET_Msk 0x10UL
/* USBFS_USBLPM.INTR_SIE_MASK */
#define USBFS_USBLPM_INTR_SIE_MASK_SOF_INTR_MASK_Pos 0UL
#define USBFS_USBLPM_INTR_SIE_MASK_SOF_INTR_MASK_Msk 0x1UL
#define USBFS_USBLPM_INTR_SIE_MASK_BUS_RESET_INTR_MASK_Pos 1UL
#define USBFS_USBLPM_INTR_SIE_MASK_BUS_RESET_INTR_MASK_Msk 0x2UL
#define USBFS_USBLPM_INTR_SIE_MASK_EP0_INTR_MASK_Pos 2UL
#define USBFS_USBLPM_INTR_SIE_MASK_EP0_INTR_MASK_Msk 0x4UL
#define USBFS_USBLPM_INTR_SIE_MASK_LPM_INTR_MASK_Pos 3UL
#define USBFS_USBLPM_INTR_SIE_MASK_LPM_INTR_MASK_Msk 0x8UL
#define USBFS_USBLPM_INTR_SIE_MASK_RESUME_INTR_MASK_Pos 4UL
#define USBFS_USBLPM_INTR_SIE_MASK_RESUME_INTR_MASK_Msk 0x10UL
/* USBFS_USBLPM.INTR_SIE_MASKED */
#define USBFS_USBLPM_INTR_SIE_MASKED_SOF_INTR_MASKED_Pos 0UL
#define USBFS_USBLPM_INTR_SIE_MASKED_SOF_INTR_MASKED_Msk 0x1UL
#define USBFS_USBLPM_INTR_SIE_MASKED_BUS_RESET_INTR_MASKED_Pos 1UL
#define USBFS_USBLPM_INTR_SIE_MASKED_BUS_RESET_INTR_MASKED_Msk 0x2UL
#define USBFS_USBLPM_INTR_SIE_MASKED_EP0_INTR_MASKED_Pos 2UL
#define USBFS_USBLPM_INTR_SIE_MASKED_EP0_INTR_MASKED_Msk 0x4UL
#define USBFS_USBLPM_INTR_SIE_MASKED_LPM_INTR_MASKED_Pos 3UL
#define USBFS_USBLPM_INTR_SIE_MASKED_LPM_INTR_MASKED_Msk 0x8UL
#define USBFS_USBLPM_INTR_SIE_MASKED_RESUME_INTR_MASKED_Pos 4UL
#define USBFS_USBLPM_INTR_SIE_MASKED_RESUME_INTR_MASKED_Msk 0x10UL
/* USBFS_USBLPM.INTR_LVL_SEL */
#define USBFS_USBLPM_INTR_LVL_SEL_SOF_LVL_SEL_Pos 0UL
#define USBFS_USBLPM_INTR_LVL_SEL_SOF_LVL_SEL_Msk 0x3UL
#define USBFS_USBLPM_INTR_LVL_SEL_BUS_RESET_LVL_SEL_Pos 2UL
#define USBFS_USBLPM_INTR_LVL_SEL_BUS_RESET_LVL_SEL_Msk 0xCUL
#define USBFS_USBLPM_INTR_LVL_SEL_EP0_LVL_SEL_Pos 4UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP0_LVL_SEL_Msk 0x30UL
#define USBFS_USBLPM_INTR_LVL_SEL_LPM_LVL_SEL_Pos 6UL
#define USBFS_USBLPM_INTR_LVL_SEL_LPM_LVL_SEL_Msk 0xC0UL
#define USBFS_USBLPM_INTR_LVL_SEL_RESUME_LVL_SEL_Pos 8UL
#define USBFS_USBLPM_INTR_LVL_SEL_RESUME_LVL_SEL_Msk 0x300UL
#define USBFS_USBLPM_INTR_LVL_SEL_ARB_EP_LVL_SEL_Pos 14UL
#define USBFS_USBLPM_INTR_LVL_SEL_ARB_EP_LVL_SEL_Msk 0xC000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP1_LVL_SEL_Pos 16UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP1_LVL_SEL_Msk 0x30000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP2_LVL_SEL_Pos 18UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP2_LVL_SEL_Msk 0xC0000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP3_LVL_SEL_Pos 20UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP3_LVL_SEL_Msk 0x300000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP4_LVL_SEL_Pos 22UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP4_LVL_SEL_Msk 0xC00000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP5_LVL_SEL_Pos 24UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP5_LVL_SEL_Msk 0x3000000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP6_LVL_SEL_Pos 26UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP6_LVL_SEL_Msk 0xC000000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP7_LVL_SEL_Pos 28UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP7_LVL_SEL_Msk 0x30000000UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP8_LVL_SEL_Pos 30UL
#define USBFS_USBLPM_INTR_LVL_SEL_EP8_LVL_SEL_Msk 0xC0000000UL
/* USBFS_USBLPM.INTR_CAUSE_HI */
#define USBFS_USBLPM_INTR_CAUSE_HI_SOF_INTR_Pos 0UL
#define USBFS_USBLPM_INTR_CAUSE_HI_SOF_INTR_Msk 0x1UL
#define USBFS_USBLPM_INTR_CAUSE_HI_BUS_RESET_INTR_Pos 1UL
#define USBFS_USBLPM_INTR_CAUSE_HI_BUS_RESET_INTR_Msk 0x2UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP0_INTR_Pos 2UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP0_INTR_Msk 0x4UL
#define USBFS_USBLPM_INTR_CAUSE_HI_LPM_INTR_Pos 3UL
#define USBFS_USBLPM_INTR_CAUSE_HI_LPM_INTR_Msk 0x8UL
#define USBFS_USBLPM_INTR_CAUSE_HI_RESUME_INTR_Pos 4UL
#define USBFS_USBLPM_INTR_CAUSE_HI_RESUME_INTR_Msk 0x10UL
#define USBFS_USBLPM_INTR_CAUSE_HI_ARB_EP_INTR_Pos 7UL
#define USBFS_USBLPM_INTR_CAUSE_HI_ARB_EP_INTR_Msk 0x80UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP1_INTR_Pos 8UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP1_INTR_Msk 0x100UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP2_INTR_Pos 9UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP2_INTR_Msk 0x200UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP3_INTR_Pos 10UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP3_INTR_Msk 0x400UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP4_INTR_Pos 11UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP4_INTR_Msk 0x800UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP5_INTR_Pos 12UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP5_INTR_Msk 0x1000UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP6_INTR_Pos 13UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP6_INTR_Msk 0x2000UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP7_INTR_Pos 14UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP7_INTR_Msk 0x4000UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP8_INTR_Pos 15UL
#define USBFS_USBLPM_INTR_CAUSE_HI_EP8_INTR_Msk 0x8000UL
/* USBFS_USBLPM.INTR_CAUSE_MED */
#define USBFS_USBLPM_INTR_CAUSE_MED_SOF_INTR_Pos 0UL
#define USBFS_USBLPM_INTR_CAUSE_MED_SOF_INTR_Msk 0x1UL
#define USBFS_USBLPM_INTR_CAUSE_MED_BUS_RESET_INTR_Pos 1UL
#define USBFS_USBLPM_INTR_CAUSE_MED_BUS_RESET_INTR_Msk 0x2UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP0_INTR_Pos 2UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP0_INTR_Msk 0x4UL
#define USBFS_USBLPM_INTR_CAUSE_MED_LPM_INTR_Pos 3UL
#define USBFS_USBLPM_INTR_CAUSE_MED_LPM_INTR_Msk 0x8UL
#define USBFS_USBLPM_INTR_CAUSE_MED_RESUME_INTR_Pos 4UL
#define USBFS_USBLPM_INTR_CAUSE_MED_RESUME_INTR_Msk 0x10UL
#define USBFS_USBLPM_INTR_CAUSE_MED_ARB_EP_INTR_Pos 7UL
#define USBFS_USBLPM_INTR_CAUSE_MED_ARB_EP_INTR_Msk 0x80UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP1_INTR_Pos 8UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP1_INTR_Msk 0x100UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP2_INTR_Pos 9UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP2_INTR_Msk 0x200UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP3_INTR_Pos 10UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP3_INTR_Msk 0x400UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP4_INTR_Pos 11UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP4_INTR_Msk 0x800UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP5_INTR_Pos 12UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP5_INTR_Msk 0x1000UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP6_INTR_Pos 13UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP6_INTR_Msk 0x2000UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP7_INTR_Pos 14UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP7_INTR_Msk 0x4000UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP8_INTR_Pos 15UL
#define USBFS_USBLPM_INTR_CAUSE_MED_EP8_INTR_Msk 0x8000UL
/* USBFS_USBLPM.INTR_CAUSE_LO */
#define USBFS_USBLPM_INTR_CAUSE_LO_SOF_INTR_Pos 0UL
#define USBFS_USBLPM_INTR_CAUSE_LO_SOF_INTR_Msk 0x1UL
#define USBFS_USBLPM_INTR_CAUSE_LO_BUS_RESET_INTR_Pos 1UL
#define USBFS_USBLPM_INTR_CAUSE_LO_BUS_RESET_INTR_Msk 0x2UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP0_INTR_Pos 2UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP0_INTR_Msk 0x4UL
#define USBFS_USBLPM_INTR_CAUSE_LO_LPM_INTR_Pos 3UL
#define USBFS_USBLPM_INTR_CAUSE_LO_LPM_INTR_Msk 0x8UL
#define USBFS_USBLPM_INTR_CAUSE_LO_RESUME_INTR_Pos 4UL
#define USBFS_USBLPM_INTR_CAUSE_LO_RESUME_INTR_Msk 0x10UL
#define USBFS_USBLPM_INTR_CAUSE_LO_ARB_EP_INTR_Pos 7UL
#define USBFS_USBLPM_INTR_CAUSE_LO_ARB_EP_INTR_Msk 0x80UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP1_INTR_Pos 8UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP1_INTR_Msk 0x100UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP2_INTR_Pos 9UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP2_INTR_Msk 0x200UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP3_INTR_Pos 10UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP3_INTR_Msk 0x400UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP4_INTR_Pos 11UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP4_INTR_Msk 0x800UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP5_INTR_Pos 12UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP5_INTR_Msk 0x1000UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP6_INTR_Pos 13UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP6_INTR_Msk 0x2000UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP7_INTR_Pos 14UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP7_INTR_Msk 0x4000UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP8_INTR_Pos 15UL
#define USBFS_USBLPM_INTR_CAUSE_LO_EP8_INTR_Msk 0x8000UL
/* USBFS_USBLPM.DFT_CTL */
#define USBFS_USBLPM_DFT_CTL_DDFT_OUT_SEL_Pos   0UL
#define USBFS_USBLPM_DFT_CTL_DDFT_OUT_SEL_Msk   0x7UL
#define USBFS_USBLPM_DFT_CTL_DDFT_IN_SEL_Pos    3UL
#define USBFS_USBLPM_DFT_CTL_DDFT_IN_SEL_Msk    0x18UL


/* USBFS_USBHOST.HOST_CTL0 */
#define USBFS_USBHOST_HOST_CTL0_HOST_Pos        0UL
#define USBFS_USBHOST_HOST_CTL0_HOST_Msk        0x1UL
#define USBFS_USBHOST_HOST_CTL0_ENABLE_Pos      31UL
#define USBFS_USBHOST_HOST_CTL0_ENABLE_Msk      0x80000000UL
/* USBFS_USBHOST.HOST_CTL1 */
#define USBFS_USBHOST_HOST_CTL1_CLKSEL_Pos      0UL
#define USBFS_USBHOST_HOST_CTL1_CLKSEL_Msk      0x1UL
#define USBFS_USBHOST_HOST_CTL1_USTP_Pos        1UL
#define USBFS_USBHOST_HOST_CTL1_USTP_Msk        0x2UL
#define USBFS_USBHOST_HOST_CTL1_RST_Pos         7UL
#define USBFS_USBHOST_HOST_CTL1_RST_Msk         0x80UL
/* USBFS_USBHOST.HOST_CTL2 */
#define USBFS_USBHOST_HOST_CTL2_RETRY_Pos       0UL
#define USBFS_USBHOST_HOST_CTL2_RETRY_Msk       0x1UL
#define USBFS_USBHOST_HOST_CTL2_CANCEL_Pos      1UL
#define USBFS_USBHOST_HOST_CTL2_CANCEL_Msk      0x2UL
#define USBFS_USBHOST_HOST_CTL2_SOFSTEP_Pos     2UL
#define USBFS_USBHOST_HOST_CTL2_SOFSTEP_Msk     0x4UL
#define USBFS_USBHOST_HOST_CTL2_ALIVE_Pos       3UL
#define USBFS_USBHOST_HOST_CTL2_ALIVE_Msk       0x8UL
#define USBFS_USBHOST_HOST_CTL2_RESERVED_4_Pos  4UL
#define USBFS_USBHOST_HOST_CTL2_RESERVED_4_Msk  0x10UL
#define USBFS_USBHOST_HOST_CTL2_RESERVED_5_Pos  5UL
#define USBFS_USBHOST_HOST_CTL2_RESERVED_5_Msk  0x20UL
#define USBFS_USBHOST_HOST_CTL2_TTEST_Pos       6UL
#define USBFS_USBHOST_HOST_CTL2_TTEST_Msk       0xC0UL
/* USBFS_USBHOST.HOST_ERR */
#define USBFS_USBHOST_HOST_ERR_HS_Pos           0UL
#define USBFS_USBHOST_HOST_ERR_HS_Msk           0x3UL
#define USBFS_USBHOST_HOST_ERR_STUFF_Pos        2UL
#define USBFS_USBHOST_HOST_ERR_STUFF_Msk        0x4UL
#define USBFS_USBHOST_HOST_ERR_TGERR_Pos        3UL
#define USBFS_USBHOST_HOST_ERR_TGERR_Msk        0x8UL
#define USBFS_USBHOST_HOST_ERR_CRC_Pos          4UL
#define USBFS_USBHOST_HOST_ERR_CRC_Msk          0x10UL
#define USBFS_USBHOST_HOST_ERR_TOUT_Pos         5UL
#define USBFS_USBHOST_HOST_ERR_TOUT_Msk         0x20UL
#define USBFS_USBHOST_HOST_ERR_RERR_Pos         6UL
#define USBFS_USBHOST_HOST_ERR_RERR_Msk         0x40UL
#define USBFS_USBHOST_HOST_ERR_LSTSOF_Pos       7UL
#define USBFS_USBHOST_HOST_ERR_LSTSOF_Msk       0x80UL
/* USBFS_USBHOST.HOST_STATUS */
#define USBFS_USBHOST_HOST_STATUS_CSTAT_Pos     0UL
#define USBFS_USBHOST_HOST_STATUS_CSTAT_Msk     0x1UL
#define USBFS_USBHOST_HOST_STATUS_TMODE_Pos     1UL
#define USBFS_USBHOST_HOST_STATUS_TMODE_Msk     0x2UL
#define USBFS_USBHOST_HOST_STATUS_SUSP_Pos      2UL
#define USBFS_USBHOST_HOST_STATUS_SUSP_Msk      0x4UL
#define USBFS_USBHOST_HOST_STATUS_SOFBUSY_Pos   3UL
#define USBFS_USBHOST_HOST_STATUS_SOFBUSY_Msk   0x8UL
#define USBFS_USBHOST_HOST_STATUS_URST_Pos      4UL
#define USBFS_USBHOST_HOST_STATUS_URST_Msk      0x10UL
#define USBFS_USBHOST_HOST_STATUS_RESERVED_5_Pos 5UL
#define USBFS_USBHOST_HOST_STATUS_RESERVED_5_Msk 0x20UL
#define USBFS_USBHOST_HOST_STATUS_RSTBUSY_Pos   6UL
#define USBFS_USBHOST_HOST_STATUS_RSTBUSY_Msk   0x40UL
#define USBFS_USBHOST_HOST_STATUS_CLKSEL_ST_Pos 7UL
#define USBFS_USBHOST_HOST_STATUS_CLKSEL_ST_Msk 0x80UL
#define USBFS_USBHOST_HOST_STATUS_HOST_ST_Pos   8UL
#define USBFS_USBHOST_HOST_STATUS_HOST_ST_Msk   0x100UL
/* USBFS_USBHOST.HOST_FCOMP */
#define USBFS_USBHOST_HOST_FCOMP_FRAMECOMP_Pos  0UL
#define USBFS_USBHOST_HOST_FCOMP_FRAMECOMP_Msk  0xFFUL
/* USBFS_USBHOST.HOST_RTIMER */
#define USBFS_USBHOST_HOST_RTIMER_RTIMER_Pos    0UL
#define USBFS_USBHOST_HOST_RTIMER_RTIMER_Msk    0x3FFFFUL
/* USBFS_USBHOST.HOST_ADDR */
#define USBFS_USBHOST_HOST_ADDR_ADDRESS_Pos     0UL
#define USBFS_USBHOST_HOST_ADDR_ADDRESS_Msk     0x7FUL
/* USBFS_USBHOST.HOST_EOF */
#define USBFS_USBHOST_HOST_EOF_EOF_Pos          0UL
#define USBFS_USBHOST_HOST_EOF_EOF_Msk          0x3FFFUL
/* USBFS_USBHOST.HOST_FRAME */
#define USBFS_USBHOST_HOST_FRAME_FRAME_Pos      0UL
#define USBFS_USBHOST_HOST_FRAME_FRAME_Msk      0x7FFUL
/* USBFS_USBHOST.HOST_TOKEN */
#define USBFS_USBHOST_HOST_TOKEN_ENDPT_Pos      0UL
#define USBFS_USBHOST_HOST_TOKEN_ENDPT_Msk      0xFUL
#define USBFS_USBHOST_HOST_TOKEN_TKNEN_Pos      4UL
#define USBFS_USBHOST_HOST_TOKEN_TKNEN_Msk      0x70UL
#define USBFS_USBHOST_HOST_TOKEN_TGGL_Pos       8UL
#define USBFS_USBHOST_HOST_TOKEN_TGGL_Msk       0x100UL
/* USBFS_USBHOST.HOST_EP1_CTL */
#define USBFS_USBHOST_HOST_EP1_CTL_PKS1_Pos     0UL
#define USBFS_USBHOST_HOST_EP1_CTL_PKS1_Msk     0x1FFUL
#define USBFS_USBHOST_HOST_EP1_CTL_NULLE_Pos    10UL
#define USBFS_USBHOST_HOST_EP1_CTL_NULLE_Msk    0x400UL
#define USBFS_USBHOST_HOST_EP1_CTL_DMAE_Pos     11UL
#define USBFS_USBHOST_HOST_EP1_CTL_DMAE_Msk     0x800UL
#define USBFS_USBHOST_HOST_EP1_CTL_DIR_Pos      12UL
#define USBFS_USBHOST_HOST_EP1_CTL_DIR_Msk      0x1000UL
#define USBFS_USBHOST_HOST_EP1_CTL_BFINI_Pos    15UL
#define USBFS_USBHOST_HOST_EP1_CTL_BFINI_Msk    0x8000UL
/* USBFS_USBHOST.HOST_EP1_STATUS */
#define USBFS_USBHOST_HOST_EP1_STATUS_SIZE1_Pos 0UL
#define USBFS_USBHOST_HOST_EP1_STATUS_SIZE1_Msk 0x1FFUL
#define USBFS_USBHOST_HOST_EP1_STATUS_VAL_DATA_Pos 16UL
#define USBFS_USBHOST_HOST_EP1_STATUS_VAL_DATA_Msk 0x10000UL
#define USBFS_USBHOST_HOST_EP1_STATUS_INI_ST_Pos 17UL
#define USBFS_USBHOST_HOST_EP1_STATUS_INI_ST_Msk 0x20000UL
#define USBFS_USBHOST_HOST_EP1_STATUS_RESERVED_18_Pos 18UL
#define USBFS_USBHOST_HOST_EP1_STATUS_RESERVED_18_Msk 0x40000UL
/* USBFS_USBHOST.HOST_EP1_RW1_DR */
#define USBFS_USBHOST_HOST_EP1_RW1_DR_BFDT8_Pos 0UL
#define USBFS_USBHOST_HOST_EP1_RW1_DR_BFDT8_Msk 0xFFUL
/* USBFS_USBHOST.HOST_EP1_RW2_DR */
#define USBFS_USBHOST_HOST_EP1_RW2_DR_BFDT16_Pos 0UL
#define USBFS_USBHOST_HOST_EP1_RW2_DR_BFDT16_Msk 0xFFFFUL
/* USBFS_USBHOST.HOST_EP2_CTL */
#define USBFS_USBHOST_HOST_EP2_CTL_PKS2_Pos     0UL
#define USBFS_USBHOST_HOST_EP2_CTL_PKS2_Msk     0x7FUL
#define USBFS_USBHOST_HOST_EP2_CTL_NULLE_Pos    10UL
#define USBFS_USBHOST_HOST_EP2_CTL_NULLE_Msk    0x400UL
#define USBFS_USBHOST_HOST_EP2_CTL_DMAE_Pos     11UL
#define USBFS_USBHOST_HOST_EP2_CTL_DMAE_Msk     0x800UL
#define USBFS_USBHOST_HOST_EP2_CTL_DIR_Pos      12UL
#define USBFS_USBHOST_HOST_EP2_CTL_DIR_Msk      0x1000UL
#define USBFS_USBHOST_HOST_EP2_CTL_BFINI_Pos    15UL
#define USBFS_USBHOST_HOST_EP2_CTL_BFINI_Msk    0x8000UL
/* USBFS_USBHOST.HOST_EP2_STATUS */
#define USBFS_USBHOST_HOST_EP2_STATUS_SIZE2_Pos 0UL
#define USBFS_USBHOST_HOST_EP2_STATUS_SIZE2_Msk 0x7FUL
#define USBFS_USBHOST_HOST_EP2_STATUS_VAL_DATA_Pos 16UL
#define USBFS_USBHOST_HOST_EP2_STATUS_VAL_DATA_Msk 0x10000UL
#define USBFS_USBHOST_HOST_EP2_STATUS_INI_ST_Pos 17UL
#define USBFS_USBHOST_HOST_EP2_STATUS_INI_ST_Msk 0x20000UL
#define USBFS_USBHOST_HOST_EP2_STATUS_RESERVED_18_Pos 18UL
#define USBFS_USBHOST_HOST_EP2_STATUS_RESERVED_18_Msk 0x40000UL
/* USBFS_USBHOST.HOST_EP2_RW1_DR */
#define USBFS_USBHOST_HOST_EP2_RW1_DR_BFDT8_Pos 0UL
#define USBFS_USBHOST_HOST_EP2_RW1_DR_BFDT8_Msk 0xFFUL
/* USBFS_USBHOST.HOST_EP2_RW2_DR */
#define USBFS_USBHOST_HOST_EP2_RW2_DR_BFDT16_Pos 0UL
#define USBFS_USBHOST_HOST_EP2_RW2_DR_BFDT16_Msk 0xFFFFUL
/* USBFS_USBHOST.HOST_LVL1_SEL */
#define USBFS_USBHOST_HOST_LVL1_SEL_SOFIRQ_SEL_Pos 0UL
#define USBFS_USBHOST_HOST_LVL1_SEL_SOFIRQ_SEL_Msk 0x3UL
#define USBFS_USBHOST_HOST_LVL1_SEL_DIRQ_SEL_Pos 2UL
#define USBFS_USBHOST_HOST_LVL1_SEL_DIRQ_SEL_Msk 0xCUL
#define USBFS_USBHOST_HOST_LVL1_SEL_CNNIRQ_SEL_Pos 4UL
#define USBFS_USBHOST_HOST_LVL1_SEL_CNNIRQ_SEL_Msk 0x30UL
#define USBFS_USBHOST_HOST_LVL1_SEL_CMPIRQ_SEL_Pos 6UL
#define USBFS_USBHOST_HOST_LVL1_SEL_CMPIRQ_SEL_Msk 0xC0UL
#define USBFS_USBHOST_HOST_LVL1_SEL_URIRQ_SEL_Pos 8UL
#define USBFS_USBHOST_HOST_LVL1_SEL_URIRQ_SEL_Msk 0x300UL
#define USBFS_USBHOST_HOST_LVL1_SEL_RWKIRQ_SEL_Pos 10UL
#define USBFS_USBHOST_HOST_LVL1_SEL_RWKIRQ_SEL_Msk 0xC00UL
#define USBFS_USBHOST_HOST_LVL1_SEL_RESERVED_13_12_Pos 12UL
#define USBFS_USBHOST_HOST_LVL1_SEL_RESERVED_13_12_Msk 0x3000UL
#define USBFS_USBHOST_HOST_LVL1_SEL_TCAN_SEL_Pos 14UL
#define USBFS_USBHOST_HOST_LVL1_SEL_TCAN_SEL_Msk 0xC000UL
/* USBFS_USBHOST.HOST_LVL2_SEL */
#define USBFS_USBHOST_HOST_LVL2_SEL_EP1_DRQ_SEL_Pos 4UL
#define USBFS_USBHOST_HOST_LVL2_SEL_EP1_DRQ_SEL_Msk 0x30UL
#define USBFS_USBHOST_HOST_LVL2_SEL_EP1_SPK_SEL_Pos 6UL
#define USBFS_USBHOST_HOST_LVL2_SEL_EP1_SPK_SEL_Msk 0xC0UL
#define USBFS_USBHOST_HOST_LVL2_SEL_EP2_DRQ_SEL_Pos 8UL
#define USBFS_USBHOST_HOST_LVL2_SEL_EP2_DRQ_SEL_Msk 0x300UL
#define USBFS_USBHOST_HOST_LVL2_SEL_EP2_SPK_SEL_Pos 10UL
#define USBFS_USBHOST_HOST_LVL2_SEL_EP2_SPK_SEL_Msk 0xC00UL
/* USBFS_USBHOST.INTR_USBHOST_CAUSE_HI */
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_SOFIRQ_INT_Pos 0UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_SOFIRQ_INT_Msk 0x1UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_DIRQ_INT_Pos 1UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_DIRQ_INT_Msk 0x2UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_CNNIRQ_INT_Pos 2UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_CNNIRQ_INT_Msk 0x4UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_CMPIRQ_INT_Pos 3UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_CMPIRQ_INT_Msk 0x8UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_URIRQ_INT_Pos 4UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_URIRQ_INT_Msk 0x10UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_RWKIRQ_INT_Pos 5UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_RWKIRQ_INT_Msk 0x20UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_RESERVED_6_Pos 6UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_RESERVED_6_Msk 0x40UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_TCAN_INT_Pos 7UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_HI_TCAN_INT_Msk 0x80UL
/* USBFS_USBHOST.INTR_USBHOST_CAUSE_MED */
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_SOFIRQ_INT_Pos 0UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_SOFIRQ_INT_Msk 0x1UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_DIRQ_INT_Pos 1UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_DIRQ_INT_Msk 0x2UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_CNNIRQ_INT_Pos 2UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_CNNIRQ_INT_Msk 0x4UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_CMPIRQ_INT_Pos 3UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_CMPIRQ_INT_Msk 0x8UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_URIRQ_INT_Pos 4UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_URIRQ_INT_Msk 0x10UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_RWKIRQ_INT_Pos 5UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_RWKIRQ_INT_Msk 0x20UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_RESERVED_6_Pos 6UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_RESERVED_6_Msk 0x40UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_TCAN_INT_Pos 7UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_MED_TCAN_INT_Msk 0x80UL
/* USBFS_USBHOST.INTR_USBHOST_CAUSE_LO */
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_SOFIRQ_INT_Pos 0UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_SOFIRQ_INT_Msk 0x1UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_DIRQ_INT_Pos 1UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_DIRQ_INT_Msk 0x2UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_CNNIRQ_INT_Pos 2UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_CNNIRQ_INT_Msk 0x4UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_CMPIRQ_INT_Pos 3UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_CMPIRQ_INT_Msk 0x8UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_URIRQ_INT_Pos 4UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_URIRQ_INT_Msk 0x10UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_RWKIRQ_INT_Pos 5UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_RWKIRQ_INT_Msk 0x20UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_RESERVED_6_Pos 6UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_RESERVED_6_Msk 0x40UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_TCAN_INT_Pos 7UL
#define USBFS_USBHOST_INTR_USBHOST_CAUSE_LO_TCAN_INT_Msk 0x80UL
/* USBFS_USBHOST.INTR_HOST_EP_CAUSE_HI */
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP1DRQ_INT_Pos 2UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP1DRQ_INT_Msk 0x4UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP1SPK_INT_Pos 3UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP1SPK_INT_Msk 0x8UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP2DRQ_INT_Pos 4UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP2DRQ_INT_Msk 0x10UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP2SPK_INT_Pos 5UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_HI_EP2SPK_INT_Msk 0x20UL
/* USBFS_USBHOST.INTR_HOST_EP_CAUSE_MED */
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP1DRQ_INT_Pos 2UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP1DRQ_INT_Msk 0x4UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP1SPK_INT_Pos 3UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP1SPK_INT_Msk 0x8UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP2DRQ_INT_Pos 4UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP2DRQ_INT_Msk 0x10UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP2SPK_INT_Pos 5UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_MED_EP2SPK_INT_Msk 0x20UL
/* USBFS_USBHOST.INTR_HOST_EP_CAUSE_LO */
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP1DRQ_INT_Pos 2UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP1DRQ_INT_Msk 0x4UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP1SPK_INT_Pos 3UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP1SPK_INT_Msk 0x8UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP2DRQ_INT_Pos 4UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP2DRQ_INT_Msk 0x10UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP2SPK_INT_Pos 5UL
#define USBFS_USBHOST_INTR_HOST_EP_CAUSE_LO_EP2SPK_INT_Msk 0x20UL
/* USBFS_USBHOST.INTR_USBHOST */
#define USBFS_USBHOST_INTR_USBHOST_SOFIRQ_Pos   0UL
#define USBFS_USBHOST_INTR_USBHOST_SOFIRQ_Msk   0x1UL
#define USBFS_USBHOST_INTR_USBHOST_DIRQ_Pos     1UL
#define USBFS_USBHOST_INTR_USBHOST_DIRQ_Msk     0x2UL
#define USBFS_USBHOST_INTR_USBHOST_CNNIRQ_Pos   2UL
#define USBFS_USBHOST_INTR_USBHOST_CNNIRQ_Msk   0x4UL
#define USBFS_USBHOST_INTR_USBHOST_CMPIRQ_Pos   3UL
#define USBFS_USBHOST_INTR_USBHOST_CMPIRQ_Msk   0x8UL
#define USBFS_USBHOST_INTR_USBHOST_URIRQ_Pos    4UL
#define USBFS_USBHOST_INTR_USBHOST_URIRQ_Msk    0x10UL
#define USBFS_USBHOST_INTR_USBHOST_RWKIRQ_Pos   5UL
#define USBFS_USBHOST_INTR_USBHOST_RWKIRQ_Msk   0x20UL
#define USBFS_USBHOST_INTR_USBHOST_RESERVED_6_Pos 6UL
#define USBFS_USBHOST_INTR_USBHOST_RESERVED_6_Msk 0x40UL
#define USBFS_USBHOST_INTR_USBHOST_TCAN_Pos     7UL
#define USBFS_USBHOST_INTR_USBHOST_TCAN_Msk     0x80UL
/* USBFS_USBHOST.INTR_USBHOST_SET */
#define USBFS_USBHOST_INTR_USBHOST_SET_SOFIRQS_Pos 0UL
#define USBFS_USBHOST_INTR_USBHOST_SET_SOFIRQS_Msk 0x1UL
#define USBFS_USBHOST_INTR_USBHOST_SET_DIRQS_Pos 1UL
#define USBFS_USBHOST_INTR_USBHOST_SET_DIRQS_Msk 0x2UL
#define USBFS_USBHOST_INTR_USBHOST_SET_CNNIRQS_Pos 2UL
#define USBFS_USBHOST_INTR_USBHOST_SET_CNNIRQS_Msk 0x4UL
#define USBFS_USBHOST_INTR_USBHOST_SET_CMPIRQS_Pos 3UL
#define USBFS_USBHOST_INTR_USBHOST_SET_CMPIRQS_Msk 0x8UL
#define USBFS_USBHOST_INTR_USBHOST_SET_URIRQS_Pos 4UL
#define USBFS_USBHOST_INTR_USBHOST_SET_URIRQS_Msk 0x10UL
#define USBFS_USBHOST_INTR_USBHOST_SET_RWKIRQS_Pos 5UL
#define USBFS_USBHOST_INTR_USBHOST_SET_RWKIRQS_Msk 0x20UL
#define USBFS_USBHOST_INTR_USBHOST_SET_RESERVED_6_Pos 6UL
#define USBFS_USBHOST_INTR_USBHOST_SET_RESERVED_6_Msk 0x40UL
#define USBFS_USBHOST_INTR_USBHOST_SET_TCANS_Pos 7UL
#define USBFS_USBHOST_INTR_USBHOST_SET_TCANS_Msk 0x80UL
/* USBFS_USBHOST.INTR_USBHOST_MASK */
#define USBFS_USBHOST_INTR_USBHOST_MASK_SOFIRQM_Pos 0UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_SOFIRQM_Msk 0x1UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_DIRQM_Pos 1UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_DIRQM_Msk 0x2UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_CNNIRQM_Pos 2UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_CNNIRQM_Msk 0x4UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_CMPIRQM_Pos 3UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_CMPIRQM_Msk 0x8UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_URIRQM_Pos 4UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_URIRQM_Msk 0x10UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_RWKIRQM_Pos 5UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_RWKIRQM_Msk 0x20UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_RESERVED_6_Pos 6UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_RESERVED_6_Msk 0x40UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_TCANM_Pos 7UL
#define USBFS_USBHOST_INTR_USBHOST_MASK_TCANM_Msk 0x80UL
/* USBFS_USBHOST.INTR_USBHOST_MASKED */
#define USBFS_USBHOST_INTR_USBHOST_MASKED_SOFIRQED_Pos 0UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_SOFIRQED_Msk 0x1UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_DIRQED_Pos 1UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_DIRQED_Msk 0x2UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_CNNIRQED_Pos 2UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_CNNIRQED_Msk 0x4UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_CMPIRQED_Pos 3UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_CMPIRQED_Msk 0x8UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_URIRQED_Pos 4UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_URIRQED_Msk 0x10UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_RWKIRQED_Pos 5UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_RWKIRQED_Msk 0x20UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_RESERVED_6_Pos 6UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_RESERVED_6_Msk 0x40UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_TCANED_Pos 7UL
#define USBFS_USBHOST_INTR_USBHOST_MASKED_TCANED_Msk 0x80UL
/* USBFS_USBHOST.INTR_HOST_EP */
#define USBFS_USBHOST_INTR_HOST_EP_EP1DRQ_Pos   2UL
#define USBFS_USBHOST_INTR_HOST_EP_EP1DRQ_Msk   0x4UL
#define USBFS_USBHOST_INTR_HOST_EP_EP1SPK_Pos   3UL
#define USBFS_USBHOST_INTR_HOST_EP_EP1SPK_Msk   0x8UL
#define USBFS_USBHOST_INTR_HOST_EP_EP2DRQ_Pos   4UL
#define USBFS_USBHOST_INTR_HOST_EP_EP2DRQ_Msk   0x10UL
#define USBFS_USBHOST_INTR_HOST_EP_EP2SPK_Pos   5UL
#define USBFS_USBHOST_INTR_HOST_EP_EP2SPK_Msk   0x20UL
/* USBFS_USBHOST.INTR_HOST_EP_SET */
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP1DRQS_Pos 2UL
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP1DRQS_Msk 0x4UL
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP1SPKS_Pos 3UL
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP1SPKS_Msk 0x8UL
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP2DRQS_Pos 4UL
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP2DRQS_Msk 0x10UL
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP2SPKS_Pos 5UL
#define USBFS_USBHOST_INTR_HOST_EP_SET_EP2SPKS_Msk 0x20UL
/* USBFS_USBHOST.INTR_HOST_EP_MASK */
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP1DRQM_Pos 2UL
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP1DRQM_Msk 0x4UL
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP1SPKM_Pos 3UL
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP1SPKM_Msk 0x8UL
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP2DRQM_Pos 4UL
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP2DRQM_Msk 0x10UL
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP2SPKM_Pos 5UL
#define USBFS_USBHOST_INTR_HOST_EP_MASK_EP2SPKM_Msk 0x20UL
/* USBFS_USBHOST.INTR_HOST_EP_MASKED */
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP1DRQED_Pos 2UL
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP1DRQED_Msk 0x4UL
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP1SPKED_Pos 3UL
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP1SPKED_Msk 0x8UL
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP2DRQED_Pos 4UL
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP2DRQED_Msk 0x10UL
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP2SPKED_Pos 5UL
#define USBFS_USBHOST_INTR_HOST_EP_MASKED_EP2SPKED_Msk 0x20UL
/* USBFS_USBHOST.HOST_DMA_ENBL */
#define USBFS_USBHOST_HOST_DMA_ENBL_DM_EP1DRQE_Pos 2UL
#define USBFS_USBHOST_HOST_DMA_ENBL_DM_EP1DRQE_Msk 0x4UL
#define USBFS_USBHOST_HOST_DMA_ENBL_DM_EP2DRQE_Pos 3UL
#define USBFS_USBHOST_HOST_DMA_ENBL_DM_EP2DRQE_Msk 0x8UL
/* USBFS_USBHOST.HOST_EP1_BLK */
#define USBFS_USBHOST_HOST_EP1_BLK_BLK_NUM_Pos  16UL
#define USBFS_USBHOST_HOST_EP1_BLK_BLK_NUM_Msk  0xFFFF0000UL
/* USBFS_USBHOST.HOST_EP2_BLK */
#define USBFS_USBHOST_HOST_EP2_BLK_BLK_NUM_Pos  16UL
#define USBFS_USBHOST_HOST_EP2_BLK_BLK_NUM_Msk  0xFFFF0000UL


#endif /* _CYIP_USBFS_H_ */


/* [] END OF FILE */
