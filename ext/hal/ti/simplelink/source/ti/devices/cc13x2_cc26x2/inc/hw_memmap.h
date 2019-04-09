/******************************************************************************
*  Filename:       hw_memmap_h
*  Revised:        2018-05-14 12:24:52 +0200 (Mon, 14 May 2018)
*  Revision:       51990
*
* Copyright (c) 2015 - 2017, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1) Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2) Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3) Neither the name of the ORGANIZATION nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef __HW_MEMMAP_H__
#define __HW_MEMMAP_H__

//*****************************************************************************
//
// The following are defines for the base address of the memories and
// peripherals on the CPU_MMAP interface
//
//*****************************************************************************
#define FLASHMEM_BASE           0x00000000 // FLASHMEM
#define BROM_BASE               0x10000000 // BROM
#define GPRAM_BASE              0x11000000 // GPRAM
#define SRAM_BASE               0x20000000 // SRAM
#define RFC_RAM_BASE            0x21000000 // RFC_RAM
#define RFC_ULLRAM_BASE         0x21004000 // RFC_ULLRAM
#define SSI0_BASE               0x40000000 // SSI
#define UART0_BASE              0x40001000 // UART
#define I2C0_BASE               0x40002000 // I2C
#define SSI1_BASE               0x40008000 // SSI
#define UART1_BASE              0x4000B000 // UART
#define GPT0_BASE               0x40010000 // GPT
#define GPT1_BASE               0x40011000 // GPT
#define GPT2_BASE               0x40012000 // GPT
#define GPT3_BASE               0x40013000 // GPT
#define UDMA0_BASE              0x40020000 // UDMA
#define I2S0_BASE               0x40021000 // I2S
#define GPIO_BASE               0x40022000 // GPIO
#define CRYPTO_BASE             0x40024000 // CRYPTO
#define PKA_BASE                0x40025000 // PKA
#define PKA_RAM_BASE            0x40026000 // PKA_RAM
#define PKA_INT_BASE            0x40027000 // PKA_INT
#define TRNG_BASE               0x40028000 // TRNG
#define FLASH_BASE              0x40030000 // FLASH
#define VIMS_BASE               0x40034000 // VIMS
#define SRAM_MMR_BASE           0x40035000 // SRAM_MMR
#define RFC_PWR_BASE            0x40040000 // RFC_PWR
#define RFC_DBELL_BASE          0x40041000 // RFC_DBELL
#define RFC_RAT_BASE            0x40043000 // RFC_RAT
#define RFC_FSCA_BASE           0x40044000 // RFC_FSCA
#define WDT_BASE                0x40080000 // WDT
#define IOC_BASE                0x40081000 // IOC
#define PRCM_BASE               0x40082000 // PRCM
#define EVENT_BASE              0x40083000 // EVENT
#define SMPH_BASE               0x40084000 // SMPH
#define ADI2_BASE               0x40086000 // ADI
#define ADI3_BASE               0x40086200 // ADI
#define AON_PMCTL_BASE          0x40090000 // AON_PMCTL
#define AON_RTC_BASE            0x40092000 // AON_RTC
#define AON_EVENT_BASE          0x40093000 // AON_EVENT
#define AON_IOC_BASE            0x40094000 // AON_IOC
#define AON_BATMON_BASE         0x40095000 // AON_BATMON
#define AUX_SPIM_BASE           0x400C1000 // AUX_SPIM
#define AUX_MAC_BASE            0x400C2000 // AUX_MAC
#define AUX_TIMER2_BASE         0x400C3000 // AUX_TIMER2
#define AUX_TDC_BASE            0x400C4000 // AUX_TDC
#define AUX_EVCTL_BASE          0x400C5000 // AUX_EVCTL
#define AUX_SYSIF_BASE          0x400C6000 // AUX_SYSIF
#define AUX_TIMER01_BASE        0x400C7000 // AUX_TIMER01
#define AUX_SMPH_BASE           0x400C8000 // AUX_SMPH
#define AUX_ANAIF_BASE          0x400C9000 // AUX_ANAIF
#define AUX_DDI0_OSC_BASE       0x400CA000 // DDI
#define AUX_ADI4_BASE           0x400CB000 // ADI
#define AUX_AIODIO0_BASE        0x400CC000 // AUX_AIODIO
#define AUX_AIODIO1_BASE        0x400CD000 // AUX_AIODIO
#define AUX_AIODIO2_BASE        0x400CE000 // AUX_AIODIO
#define AUX_AIODIO3_BASE        0x400CF000 // AUX_AIODIO
#define AUX_RAM_BASE            0x400E0000 // AUX_RAM
#define AUX_SCE_BASE            0x400E1000 // AUX_SCE
#define FLASH_CFG_BASE          0x50000000 // CC26_DUMMY_COMP
#define FCFG1_BASE              0x50001000 // FCFG1
#define FCFG2_BASE              0x50002000 // FCFG2
#ifndef CCFG_BASE
#define CCFG_BASE               0x50003000 // CCFG
#endif
#define CCFG_BASE_DEFAULT       0x50003000 // CCFG
#define SSI0_NONBUF_BASE        0x60000000 // SSI CPU nonbuf base
#define UART0_NONBUF_BASE       0x60001000 // UART CPU nonbuf base
#define I2C0_NONBUF_BASE        0x60002000 // I2C CPU nonbuf base
#define SSI1_NONBUF_BASE        0x60008000 // SSI CPU nonbuf base
#define UART1_NONBUF_BASE       0x6000B000 // UART CPU nonbuf base
#define GPT0_NONBUF_BASE        0x60010000 // GPT CPU nonbuf base
#define GPT1_NONBUF_BASE        0x60011000 // GPT CPU nonbuf base
#define GPT2_NONBUF_BASE        0x60012000 // GPT CPU nonbuf base
#define GPT3_NONBUF_BASE        0x60013000 // GPT CPU nonbuf base
#define UDMA0_NONBUF_BASE       0x60020000 // UDMA CPU nonbuf base
#define I2S0_NONBUF_BASE        0x60021000 // I2S CPU nonbuf base
#define GPIO_NONBUF_BASE        0x60022000 // GPIO CPU nonbuf base
#define CRYPTO_NONBUF_BASE      0x60024000 // CRYPTO CPU nonbuf base
#define PKA_NONBUF_BASE         0x60025000 // PKA CPU nonbuf base
#define PKA_RAM_NONBUF_BASE     0x60026000 // PKA_RAM CPU nonbuf base
#define PKA_INT_NONBUF_BASE     0x60027000 // PKA_INT CPU nonbuf base
#define TRNG_NONBUF_BASE        0x60028000 // TRNG CPU nonbuf base
#define FLASH_NONBUF_BASE       0x60030000 // FLASH CPU nonbuf base
#define VIMS_NONBUF_BASE        0x60034000 // VIMS CPU nonbuf base
#define SRAM_MMR_NONBUF_BASE    0x60035000 // SRAM_MMR CPU nonbuf base
#define RFC_PWR_NONBUF_BASE     0x60040000 // RFC_PWR CPU nonbuf base
#define RFC_DBELL_NONBUF_BASE   0x60041000 // RFC_DBELL CPU nonbuf base
#define RFC_RAT_NONBUF_BASE     0x60043000 // RFC_RAT CPU nonbuf base
#define RFC_FSCA_NONBUF_BASE    0x60044000 // RFC_FSCA CPU nonbuf base
#define WDT_NONBUF_BASE         0x60080000 // WDT CPU nonbuf base
#define IOC_NONBUF_BASE         0x60081000 // IOC CPU nonbuf base
#define PRCM_NONBUF_BASE        0x60082000 // PRCM CPU nonbuf base
#define EVENT_NONBUF_BASE       0x60083000 // EVENT CPU nonbuf base
#define SMPH_NONBUF_BASE        0x60084000 // SMPH CPU nonbuf base
#define ADI2_NONBUF_BASE        0x60086000 // ADI CPU nonbuf base
#define ADI3_NONBUF_BASE        0x60086200 // ADI CPU nonbuf base
#define AON_PMCTL_NONBUF_BASE   0x60090000 // AON_PMCTL CPU nonbuf base
#define AON_RTC_NONBUF_BASE     0x60092000 // AON_RTC CPU nonbuf base
#define AON_EVENT_NONBUF_BASE   0x60093000 // AON_EVENT CPU nonbuf base
#define AON_IOC_NONBUF_BASE     0x60094000 // AON_IOC CPU nonbuf base
#define AON_BATMON_NONBUF_BASE  0x60095000 // AON_BATMON CPU nonbuf base
#define AUX_SPIM_NONBUF_BASE    0x600C1000 // AUX_SPIM CPU nonbuf base
#define AUX_MAC_NONBUF_BASE     0x600C2000 // AUX_MAC CPU nonbuf base
#define AUX_TIMER2_NONBUF_BASE  0x600C3000 // AUX_TIMER2 CPU nonbuf base
#define AUX_TDC_NONBUF_BASE     0x600C4000 // AUX_TDC CPU nonbuf base
#define AUX_EVCTL_NONBUF_BASE   0x600C5000 // AUX_EVCTL CPU nonbuf base
#define AUX_SYSIF_NONBUF_BASE   0x600C6000 // AUX_SYSIF CPU nonbuf base
#define AUX_TIMER01_NONBUF_BASE \
                                0x600C7000 // AUX_TIMER01 CPU nonbuf base
#define AUX_SMPH_NONBUF_BASE    0x600C8000 // AUX_SMPH CPU nonbuf base
#define AUX_ANAIF_NONBUF_BASE   0x600C9000 // AUX_ANAIF CPU nonbuf base
#define AUX_DDI0_OSC_NONBUF_BASE \
                                0x600CA000 // DDI CPU nonbuf base
#define AUX_ADI4_NONBUF_BASE    0x600CB000 // ADI CPU nonbuf base
#define AUX_AIODIO0_NONBUF_BASE \
                                0x600CC000 // AUX_AIODIO CPU nonbuf base
#define AUX_AIODIO1_NONBUF_BASE \
                                0x600CD000 // AUX_AIODIO CPU nonbuf base
#define AUX_AIODIO2_NONBUF_BASE \
                                0x600CE000 // AUX_AIODIO CPU nonbuf base
#define AUX_AIODIO3_NONBUF_BASE \
                                0x600CF000 // AUX_AIODIO CPU nonbuf base
#define AUX_RAM_NONBUF_BASE     0x600E0000 // AUX_RAM CPU nonbuf base
#define AUX_SCE_NONBUF_BASE     0x600E1000 // AUX_SCE CPU nonbuf base
#define FLASHMEM_ALIAS_BASE     0xA0000000 // FLASHMEM Alias base
#define CPU_ITM_BASE            0xE0000000 // CPU_ITM
#define CPU_DWT_BASE            0xE0001000 // CPU_DWT
#define CPU_FPB_BASE            0xE0002000 // CPU_FPB
#define CPU_SCS_BASE            0xE000E000 // CPU_SCS
#define CPU_TPIU_BASE           0xE0040000 // CPU_TPIU
#define CPU_TIPROP_BASE         0xE00FE000 // CPU_TIPROP
#define CPU_ROM_TABLE_BASE      0xE00FF000 // CPU_ROM_TABLE

#endif // __HW_MEMMAP__
