/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) %copyright_year%, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

#ifndef _SAM3XA_HSMCI_INSTANCE_
#define _SAM3XA_HSMCI_INSTANCE_

/* ========== Register definition for HSMCI peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_HSMCI_CR                     (0x40000000U) /**< \brief (HSMCI) Control Register */
  #define REG_HSMCI_MR                     (0x40000004U) /**< \brief (HSMCI) Mode Register */
  #define REG_HSMCI_DTOR                   (0x40000008U) /**< \brief (HSMCI) Data Timeout Register */
  #define REG_HSMCI_SDCR                   (0x4000000CU) /**< \brief (HSMCI) SD/SDIO Card Register */
  #define REG_HSMCI_ARGR                   (0x40000010U) /**< \brief (HSMCI) Argument Register */
  #define REG_HSMCI_CMDR                   (0x40000014U) /**< \brief (HSMCI) Command Register */
  #define REG_HSMCI_BLKR                   (0x40000018U) /**< \brief (HSMCI) Block Register */
  #define REG_HSMCI_CSTOR                  (0x4000001CU) /**< \brief (HSMCI) Completion Signal Timeout Register */
  #define REG_HSMCI_RSPR                   (0x40000020U) /**< \brief (HSMCI) Response Register */
  #define REG_HSMCI_RDR                    (0x40000030U) /**< \brief (HSMCI) Receive Data Register */
  #define REG_HSMCI_TDR                    (0x40000034U) /**< \brief (HSMCI) Transmit Data Register */
  #define REG_HSMCI_SR                     (0x40000040U) /**< \brief (HSMCI) Status Register */
  #define REG_HSMCI_IER                    (0x40000044U) /**< \brief (HSMCI) Interrupt Enable Register */
  #define REG_HSMCI_IDR                    (0x40000048U) /**< \brief (HSMCI) Interrupt Disable Register */
  #define REG_HSMCI_IMR                    (0x4000004CU) /**< \brief (HSMCI) Interrupt Mask Register */
  #define REG_HSMCI_DMA                    (0x40000050U) /**< \brief (HSMCI) DMA Configuration Register */
  #define REG_HSMCI_CFG                    (0x40000054U) /**< \brief (HSMCI) Configuration Register */
  #define REG_HSMCI_WPMR                   (0x400000E4U) /**< \brief (HSMCI) Write Protection Mode Register */
  #define REG_HSMCI_WPSR                   (0x400000E8U) /**< \brief (HSMCI) Write Protection Status Register */
  #define REG_HSMCI_FIFO                   (0x40000200U) /**< \brief (HSMCI) FIFO Memory Aperture0 */
#else
  #define REG_HSMCI_CR    (*(__O  uint32_t*)0x40000000U) /**< \brief (HSMCI) Control Register */
  #define REG_HSMCI_MR    (*(__IO uint32_t*)0x40000004U) /**< \brief (HSMCI) Mode Register */
  #define REG_HSMCI_DTOR  (*(__IO uint32_t*)0x40000008U) /**< \brief (HSMCI) Data Timeout Register */
  #define REG_HSMCI_SDCR  (*(__IO uint32_t*)0x4000000CU) /**< \brief (HSMCI) SD/SDIO Card Register */
  #define REG_HSMCI_ARGR  (*(__IO uint32_t*)0x40000010U) /**< \brief (HSMCI) Argument Register */
  #define REG_HSMCI_CMDR  (*(__O  uint32_t*)0x40000014U) /**< \brief (HSMCI) Command Register */
  #define REG_HSMCI_BLKR  (*(__IO uint32_t*)0x40000018U) /**< \brief (HSMCI) Block Register */
  #define REG_HSMCI_CSTOR (*(__IO uint32_t*)0x4000001CU) /**< \brief (HSMCI) Completion Signal Timeout Register */
  #define REG_HSMCI_RSPR  (*(__I  uint32_t*)0x40000020U) /**< \brief (HSMCI) Response Register */
  #define REG_HSMCI_RDR   (*(__I  uint32_t*)0x40000030U) /**< \brief (HSMCI) Receive Data Register */
  #define REG_HSMCI_TDR   (*(__O  uint32_t*)0x40000034U) /**< \brief (HSMCI) Transmit Data Register */
  #define REG_HSMCI_SR    (*(__I  uint32_t*)0x40000040U) /**< \brief (HSMCI) Status Register */
  #define REG_HSMCI_IER   (*(__O  uint32_t*)0x40000044U) /**< \brief (HSMCI) Interrupt Enable Register */
  #define REG_HSMCI_IDR   (*(__O  uint32_t*)0x40000048U) /**< \brief (HSMCI) Interrupt Disable Register */
  #define REG_HSMCI_IMR   (*(__I  uint32_t*)0x4000004CU) /**< \brief (HSMCI) Interrupt Mask Register */
  #define REG_HSMCI_DMA   (*(__IO uint32_t*)0x40000050U) /**< \brief (HSMCI) DMA Configuration Register */
  #define REG_HSMCI_CFG   (*(__IO uint32_t*)0x40000054U) /**< \brief (HSMCI) Configuration Register */
  #define REG_HSMCI_WPMR  (*(__IO uint32_t*)0x400000E4U) /**< \brief (HSMCI) Write Protection Mode Register */
  #define REG_HSMCI_WPSR  (*(__I  uint32_t*)0x400000E8U) /**< \brief (HSMCI) Write Protection Status Register */
  #define REG_HSMCI_FIFO  (*(__IO uint32_t*)0x40000200U) /**< \brief (HSMCI) FIFO Memory Aperture0 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_HSMCI_INSTANCE_ */
