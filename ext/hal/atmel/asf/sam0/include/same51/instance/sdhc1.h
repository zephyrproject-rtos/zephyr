/**
 * \file
 *
 * \brief Instance description for SDHC1
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAME51_SDHC1_INSTANCE_
#define _SAME51_SDHC1_INSTANCE_

/* ========== Register definition for SDHC1 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SDHC1_SSAR             (0x46000000U) /**< \brief (SDHC1) SDMA System Address Register */
#define REG_SDHC1_BCR              (0x46000004U) /**< \brief (SDHC1) Block Register */
#define REG_SDHC1_ARG1             (0x46000008U) /**< \brief (SDHC1) Argument Register 1 */
#define REG_SDHC1_CMD              (0x4600000CU) /**< \brief (SDHC1) Command Register */
#define REG_SDHC1_RESP0            (0x46000010U) /**< \brief (SDHC1) Response Register 0 */
#define REG_SDHC1_RESP1            (0x46000014U) /**< \brief (SDHC1) Response Register 1 */
#define REG_SDHC1_RESP2            (0x46000018U) /**< \brief (SDHC1) Response Register 2 */
#define REG_SDHC1_RESP3            (0x4600001CU) /**< \brief (SDHC1) Response Register 3 */
#define REG_SDHC1_BDPR             (0x46000020U) /**< \brief (SDHC1) Buffer Data Port Register */
#define REG_SDHC1_PSR              (0x46000024U) /**< \brief (SDHC1) Present State Register */
#define REG_SDHC1_CR               (0x46000028U) /**< \brief (SDHC1) Control Register */
#define REG_SDHC1_CCR              (0x4600002CU) /**< \brief (SDHC1) Clock Control Register */
#define REG_SDHC1_ISR              (0x46000030U) /**< \brief (SDHC1) Interrupt Status Register */
#define REG_SDHC1_ISER             (0x46000034U) /**< \brief (SDHC1) Interrupt Status Enable Register */
#define REG_SDHC1_IER              (0x46000038U) /**< \brief (SDHC1) Interrupt Enable Register */
#define REG_SDHC1_CSR              (0x4600003CU) /**< \brief (SDHC1) Control and Status Register */
#define REG_SDHC1_CAP              (0x46000040U) /**< \brief (SDHC1) Capabilities Register */
#define REG_SDHC1_CAPH             (0x46000044U) /**< \brief (SDHC1) Capabilities Register High */
#define REG_SDHC1_MCCAP            (0x46000048U) /**< \brief (SDHC1) Maximum Current Capabilities Register */
#define REG_SDHC1_MCCAPH           (0x4600004CU) /**< \brief (SDHC1) Maximum Current Capabilities Register High */
#define REG_SDHC1_FER              (0x46000050U) /**< \brief (SDHC1) Force Event Status Register */
#define REG_SDHC1_AESR             (0x46000054U) /**< \brief (SDHC1) ADMA Error Status Register */
#define REG_SDHC1_ASAR             (0x46000058U) /**< \brief (SDHC1) ADMA System Address Register */
#define REG_SDHC1_ASARH            (0x4600005CU) /**< \brief (SDHC1) ADMA System Address Register High */
#define REG_SDHC1_PVR0             (0x46000060U) /**< \brief (SDHC1) Preset Value Register 0 */
#define REG_SDHC1_PVR1             (0x46000064U) /**< \brief (SDHC1) Preset Value Register 1 */
#define REG_SDHC1_PVR2             (0x46000068U) /**< \brief (SDHC1) Preset Value Register 2 */
#define REG_SDHC1_PVR3             (0x4600006CU) /**< \brief (SDHC1) Preset Value Register 3 */
#define REG_SDHC1_SBC              (0x460000E0U) /**< \brief (SDHC1) Shared Bus Control Register */
#define REG_SDHC1_SIS              (0x460000FCU) /**< \brief (SDHC1) Slot Interrupt Status Register */
#define REG_SDHC1_PSRH             (0x46000100U) /**< \brief (SDHC1) Additional Present State Register */
#define REG_SDHC1_MMCCR            (0x46000104U) /**< \brief (SDHC1) MMC Control Register */
#define REG_SDHC1_IPVERS           (0x460001E8U) /**< \brief (SDHC1) IP Version Register */
#define REG_SDHC1_IPVERSH          (0x460001ECU) /**< \brief (SDHC1) IP Version Register High */
#define REG_SDHC1_NAME             (0x460001F0U) /**< \brief (SDHC1) Name Register */
#define REG_SDHC1_NAMEH            (0x460001F4U) /**< \brief (SDHC1) Name Register High */
#define REG_SDHC1_PARAMETER        (0x460001F8U) /**< \brief (SDHC1) Parameter Register */
#define REG_SDHC1_VERSION          (0x460001FCU) /**< \brief (SDHC1) Version Register */
#else
#define REG_SDHC1_SSAR             (*(RwReg  *)0x46000000U) /**< \brief (SDHC1) SDMA System Address Register */
#define REG_SDHC1_BCR              (*(RwReg  *)0x46000004U) /**< \brief (SDHC1) Block Register */
#define REG_SDHC1_ARG1             (*(RwReg  *)0x46000008U) /**< \brief (SDHC1) Argument Register 1 */
#define REG_SDHC1_CMD              (*(RwReg  *)0x4600000CU) /**< \brief (SDHC1) Command Register */
#define REG_SDHC1_RESP0            (*(RoReg  *)0x46000010U) /**< \brief (SDHC1) Response Register 0 */
#define REG_SDHC1_RESP1            (*(RoReg  *)0x46000014U) /**< \brief (SDHC1) Response Register 1 */
#define REG_SDHC1_RESP2            (*(RoReg  *)0x46000018U) /**< \brief (SDHC1) Response Register 2 */
#define REG_SDHC1_RESP3            (*(RoReg  *)0x4600001CU) /**< \brief (SDHC1) Response Register 3 */
#define REG_SDHC1_BDPR             (*(RwReg  *)0x46000020U) /**< \brief (SDHC1) Buffer Data Port Register */
#define REG_SDHC1_PSR              (*(RoReg  *)0x46000024U) /**< \brief (SDHC1) Present State Register */
#define REG_SDHC1_CR               (*(RwReg  *)0x46000028U) /**< \brief (SDHC1) Control Register */
#define REG_SDHC1_CCR              (*(RwReg  *)0x4600002CU) /**< \brief (SDHC1) Clock Control Register */
#define REG_SDHC1_ISR              (*(RwReg  *)0x46000030U) /**< \brief (SDHC1) Interrupt Status Register */
#define REG_SDHC1_ISER             (*(RwReg  *)0x46000034U) /**< \brief (SDHC1) Interrupt Status Enable Register */
#define REG_SDHC1_IER              (*(RwReg  *)0x46000038U) /**< \brief (SDHC1) Interrupt Enable Register */
#define REG_SDHC1_CSR              (*(RwReg  *)0x4600003CU) /**< \brief (SDHC1) Control and Status Register */
#define REG_SDHC1_CAP              (*(RwReg  *)0x46000040U) /**< \brief (SDHC1) Capabilities Register */
#define REG_SDHC1_CAPH             (*(RwReg  *)0x46000044U) /**< \brief (SDHC1) Capabilities Register High */
#define REG_SDHC1_MCCAP            (*(RoReg  *)0x46000048U) /**< \brief (SDHC1) Maximum Current Capabilities Register */
#define REG_SDHC1_MCCAPH           (*(RoReg  *)0x4600004CU) /**< \brief (SDHC1) Maximum Current Capabilities Register High */
#define REG_SDHC1_FER              (*(WoReg  *)0x46000050U) /**< \brief (SDHC1) Force Event Status Register */
#define REG_SDHC1_AESR             (*(RoReg  *)0x46000054U) /**< \brief (SDHC1) ADMA Error Status Register */
#define REG_SDHC1_ASAR             (*(RwReg  *)0x46000058U) /**< \brief (SDHC1) ADMA System Address Register */
#define REG_SDHC1_ASARH            (*(RwReg  *)0x4600005CU) /**< \brief (SDHC1) ADMA System Address Register High */
#define REG_SDHC1_PVR0             (*(RwReg  *)0x46000060U) /**< \brief (SDHC1) Preset Value Register 0 */
#define REG_SDHC1_PVR1             (*(RwReg  *)0x46000064U) /**< \brief (SDHC1) Preset Value Register 1 */
#define REG_SDHC1_PVR2             (*(RwReg  *)0x46000068U) /**< \brief (SDHC1) Preset Value Register 2 */
#define REG_SDHC1_PVR3             (*(RwReg  *)0x4600006CU) /**< \brief (SDHC1) Preset Value Register 3 */
#define REG_SDHC1_SBC              (*(RwReg  *)0x460000E0U) /**< \brief (SDHC1) Shared Bus Control Register */
#define REG_SDHC1_SIS              (*(RoReg  *)0x460000FCU) /**< \brief (SDHC1) Slot Interrupt Status Register */
#define REG_SDHC1_PSRH             (*(RwReg  *)0x46000100U) /**< \brief (SDHC1) Additional Present State Register */
#define REG_SDHC1_MMCCR            (*(RwReg  *)0x46000104U) /**< \brief (SDHC1) MMC Control Register */
#define REG_SDHC1_IPVERS           (*(RoReg  *)0x460001E8U) /**< \brief (SDHC1) IP Version Register */
#define REG_SDHC1_IPVERSH          (*(RoReg  *)0x460001ECU) /**< \brief (SDHC1) IP Version Register High */
#define REG_SDHC1_NAME             (*(RoReg  *)0x460001F0U) /**< \brief (SDHC1) Name Register */
#define REG_SDHC1_NAMEH            (*(RoReg  *)0x460001F4U) /**< \brief (SDHC1) Name Register High */
#define REG_SDHC1_PARAMETER        (*(RoReg  *)0x460001F8U) /**< \brief (SDHC1) Parameter Register */
#define REG_SDHC1_VERSION          (*(RoReg  *)0x460001FCU) /**< \brief (SDHC1) Version Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for SDHC1 peripheral ========== */
#define SDHC1_CARD_DATA_SIZE        4       
#define SDHC1_CLK_AHB_ID            17      
#define SDHC1_GCLK_ID_SLOW          17      
#define SDHC1_NB_OF_DEVICES         1       

#endif /* _SAME51_SDHC1_INSTANCE_ */
