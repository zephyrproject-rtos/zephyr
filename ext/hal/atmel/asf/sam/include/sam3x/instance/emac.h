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

#ifndef _SAM3XA_EMAC_INSTANCE_
#define _SAM3XA_EMAC_INSTANCE_

/* ========== Register definition for EMAC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_EMAC_NCR                    (0x400B0000U) /**< \brief (EMAC) Network Control Register */
  #define REG_EMAC_NCFGR                  (0x400B0004U) /**< \brief (EMAC) Network Configuration Register */
  #define REG_EMAC_NSR                    (0x400B0008U) /**< \brief (EMAC) Network Status Register */
  #define REG_EMAC_TSR                    (0x400B0014U) /**< \brief (EMAC) Transmit Status Register */
  #define REG_EMAC_RBQP                   (0x400B0018U) /**< \brief (EMAC) Receive Buffer Queue Pointer Register */
  #define REG_EMAC_TBQP                   (0x400B001CU) /**< \brief (EMAC) Transmit Buffer Queue Pointer Register */
  #define REG_EMAC_RSR                    (0x400B0020U) /**< \brief (EMAC) Receive Status Register */
  #define REG_EMAC_ISR                    (0x400B0024U) /**< \brief (EMAC) Interrupt Status Register */
  #define REG_EMAC_IER                    (0x400B0028U) /**< \brief (EMAC) Interrupt Enable Register */
  #define REG_EMAC_IDR                    (0x400B002CU) /**< \brief (EMAC) Interrupt Disable Register */
  #define REG_EMAC_IMR                    (0x400B0030U) /**< \brief (EMAC) Interrupt Mask Register */
  #define REG_EMAC_MAN                    (0x400B0034U) /**< \brief (EMAC) Phy Maintenance Register */
  #define REG_EMAC_PTR                    (0x400B0038U) /**< \brief (EMAC) Pause Time Register */
  #define REG_EMAC_PFR                    (0x400B003CU) /**< \brief (EMAC) Pause Frames Received Register */
  #define REG_EMAC_FTO                    (0x400B0040U) /**< \brief (EMAC) Frames Transmitted Ok Register */
  #define REG_EMAC_SCF                    (0x400B0044U) /**< \brief (EMAC) Single Collision Frames Register */
  #define REG_EMAC_MCF                    (0x400B0048U) /**< \brief (EMAC) Multiple Collision Frames Register */
  #define REG_EMAC_FRO                    (0x400B004CU) /**< \brief (EMAC) Frames Received Ok Register */
  #define REG_EMAC_FCSE                   (0x400B0050U) /**< \brief (EMAC) Frame Check Sequence Errors Register */
  #define REG_EMAC_ALE                    (0x400B0054U) /**< \brief (EMAC) Alignment Errors Register */
  #define REG_EMAC_DTF                    (0x400B0058U) /**< \brief (EMAC) Deferred Transmission Frames Register */
  #define REG_EMAC_LCOL                   (0x400B005CU) /**< \brief (EMAC) Late Collisions Register */
  #define REG_EMAC_ECOL                   (0x400B0060U) /**< \brief (EMAC) Excessive Collisions Register */
  #define REG_EMAC_TUND                   (0x400B0064U) /**< \brief (EMAC) Transmit Underrun Errors Register */
  #define REG_EMAC_CSE                    (0x400B0068U) /**< \brief (EMAC) Carrier Sense Errors Register */
  #define REG_EMAC_RRE                    (0x400B006CU) /**< \brief (EMAC) Receive Resource Errors Register */
  #define REG_EMAC_ROV                    (0x400B0070U) /**< \brief (EMAC) Receive Overrun Errors Register */
  #define REG_EMAC_RSE                    (0x400B0074U) /**< \brief (EMAC) Receive Symbol Errors Register */
  #define REG_EMAC_ELE                    (0x400B0078U) /**< \brief (EMAC) Excessive Length Errors Register */
  #define REG_EMAC_RJA                    (0x400B007CU) /**< \brief (EMAC) Receive Jabbers Register */
  #define REG_EMAC_USF                    (0x400B0080U) /**< \brief (EMAC) Undersize Frames Register */
  #define REG_EMAC_STE                    (0x400B0084U) /**< \brief (EMAC) SQE Test Errors Register */
  #define REG_EMAC_RLE                    (0x400B0088U) /**< \brief (EMAC) Received Length Field Mismatch Register */
  #define REG_EMAC_HRB                    (0x400B0090U) /**< \brief (EMAC) Hash Register Bottom [31:0] Register */
  #define REG_EMAC_HRT                    (0x400B0094U) /**< \brief (EMAC) Hash Register Top [63:32] Register */
  #define REG_EMAC_SA1B                   (0x400B0098U) /**< \brief (EMAC) Specific Address 1 Bottom Register */
  #define REG_EMAC_SA1T                   (0x400B009CU) /**< \brief (EMAC) Specific Address 1 Top Register */
  #define REG_EMAC_SA2B                   (0x400B00A0U) /**< \brief (EMAC) Specific Address 2 Bottom Register */
  #define REG_EMAC_SA2T                   (0x400B00A4U) /**< \brief (EMAC) Specific Address 2 Top Register */
  #define REG_EMAC_SA3B                   (0x400B00A8U) /**< \brief (EMAC) Specific Address 3 Bottom Register */
  #define REG_EMAC_SA3T                   (0x400B00ACU) /**< \brief (EMAC) Specific Address 3 Top Register */
  #define REG_EMAC_SA4B                   (0x400B00B0U) /**< \brief (EMAC) Specific Address 4 Bottom Register */
  #define REG_EMAC_SA4T                   (0x400B00B4U) /**< \brief (EMAC) Specific Address 4 Top Register */
  #define REG_EMAC_TID                    (0x400B00B8U) /**< \brief (EMAC) Type ID Checking Register */
  #define REG_EMAC_USRIO                  (0x400B00C0U) /**< \brief (EMAC) User Input/Output Register */
#else
  #define REG_EMAC_NCR   (*(__IO uint32_t*)0x400B0000U) /**< \brief (EMAC) Network Control Register */
  #define REG_EMAC_NCFGR (*(__IO uint32_t*)0x400B0004U) /**< \brief (EMAC) Network Configuration Register */
  #define REG_EMAC_NSR   (*(__I  uint32_t*)0x400B0008U) /**< \brief (EMAC) Network Status Register */
  #define REG_EMAC_TSR   (*(__IO uint32_t*)0x400B0014U) /**< \brief (EMAC) Transmit Status Register */
  #define REG_EMAC_RBQP  (*(__IO uint32_t*)0x400B0018U) /**< \brief (EMAC) Receive Buffer Queue Pointer Register */
  #define REG_EMAC_TBQP  (*(__IO uint32_t*)0x400B001CU) /**< \brief (EMAC) Transmit Buffer Queue Pointer Register */
  #define REG_EMAC_RSR   (*(__IO uint32_t*)0x400B0020U) /**< \brief (EMAC) Receive Status Register */
  #define REG_EMAC_ISR   (*(__IO uint32_t*)0x400B0024U) /**< \brief (EMAC) Interrupt Status Register */
  #define REG_EMAC_IER   (*(__O  uint32_t*)0x400B0028U) /**< \brief (EMAC) Interrupt Enable Register */
  #define REG_EMAC_IDR   (*(__O  uint32_t*)0x400B002CU) /**< \brief (EMAC) Interrupt Disable Register */
  #define REG_EMAC_IMR   (*(__I  uint32_t*)0x400B0030U) /**< \brief (EMAC) Interrupt Mask Register */
  #define REG_EMAC_MAN   (*(__IO uint32_t*)0x400B0034U) /**< \brief (EMAC) Phy Maintenance Register */
  #define REG_EMAC_PTR   (*(__IO uint32_t*)0x400B0038U) /**< \brief (EMAC) Pause Time Register */
  #define REG_EMAC_PFR   (*(__IO uint32_t*)0x400B003CU) /**< \brief (EMAC) Pause Frames Received Register */
  #define REG_EMAC_FTO   (*(__IO uint32_t*)0x400B0040U) /**< \brief (EMAC) Frames Transmitted Ok Register */
  #define REG_EMAC_SCF   (*(__IO uint32_t*)0x400B0044U) /**< \brief (EMAC) Single Collision Frames Register */
  #define REG_EMAC_MCF   (*(__IO uint32_t*)0x400B0048U) /**< \brief (EMAC) Multiple Collision Frames Register */
  #define REG_EMAC_FRO   (*(__IO uint32_t*)0x400B004CU) /**< \brief (EMAC) Frames Received Ok Register */
  #define REG_EMAC_FCSE  (*(__IO uint32_t*)0x400B0050U) /**< \brief (EMAC) Frame Check Sequence Errors Register */
  #define REG_EMAC_ALE   (*(__IO uint32_t*)0x400B0054U) /**< \brief (EMAC) Alignment Errors Register */
  #define REG_EMAC_DTF   (*(__IO uint32_t*)0x400B0058U) /**< \brief (EMAC) Deferred Transmission Frames Register */
  #define REG_EMAC_LCOL  (*(__IO uint32_t*)0x400B005CU) /**< \brief (EMAC) Late Collisions Register */
  #define REG_EMAC_ECOL  (*(__IO uint32_t*)0x400B0060U) /**< \brief (EMAC) Excessive Collisions Register */
  #define REG_EMAC_TUND  (*(__IO uint32_t*)0x400B0064U) /**< \brief (EMAC) Transmit Underrun Errors Register */
  #define REG_EMAC_CSE   (*(__IO uint32_t*)0x400B0068U) /**< \brief (EMAC) Carrier Sense Errors Register */
  #define REG_EMAC_RRE   (*(__IO uint32_t*)0x400B006CU) /**< \brief (EMAC) Receive Resource Errors Register */
  #define REG_EMAC_ROV   (*(__IO uint32_t*)0x400B0070U) /**< \brief (EMAC) Receive Overrun Errors Register */
  #define REG_EMAC_RSE   (*(__IO uint32_t*)0x400B0074U) /**< \brief (EMAC) Receive Symbol Errors Register */
  #define REG_EMAC_ELE   (*(__IO uint32_t*)0x400B0078U) /**< \brief (EMAC) Excessive Length Errors Register */
  #define REG_EMAC_RJA   (*(__IO uint32_t*)0x400B007CU) /**< \brief (EMAC) Receive Jabbers Register */
  #define REG_EMAC_USF   (*(__IO uint32_t*)0x400B0080U) /**< \brief (EMAC) Undersize Frames Register */
  #define REG_EMAC_STE   (*(__IO uint32_t*)0x400B0084U) /**< \brief (EMAC) SQE Test Errors Register */
  #define REG_EMAC_RLE   (*(__IO uint32_t*)0x400B0088U) /**< \brief (EMAC) Received Length Field Mismatch Register */
  #define REG_EMAC_HRB   (*(__IO uint32_t*)0x400B0090U) /**< \brief (EMAC) Hash Register Bottom [31:0] Register */
  #define REG_EMAC_HRT   (*(__IO uint32_t*)0x400B0094U) /**< \brief (EMAC) Hash Register Top [63:32] Register */
  #define REG_EMAC_SA1B  (*(__IO uint32_t*)0x400B0098U) /**< \brief (EMAC) Specific Address 1 Bottom Register */
  #define REG_EMAC_SA1T  (*(__IO uint32_t*)0x400B009CU) /**< \brief (EMAC) Specific Address 1 Top Register */
  #define REG_EMAC_SA2B  (*(__IO uint32_t*)0x400B00A0U) /**< \brief (EMAC) Specific Address 2 Bottom Register */
  #define REG_EMAC_SA2T  (*(__IO uint32_t*)0x400B00A4U) /**< \brief (EMAC) Specific Address 2 Top Register */
  #define REG_EMAC_SA3B  (*(__IO uint32_t*)0x400B00A8U) /**< \brief (EMAC) Specific Address 3 Bottom Register */
  #define REG_EMAC_SA3T  (*(__IO uint32_t*)0x400B00ACU) /**< \brief (EMAC) Specific Address 3 Top Register */
  #define REG_EMAC_SA4B  (*(__IO uint32_t*)0x400B00B0U) /**< \brief (EMAC) Specific Address 4 Bottom Register */
  #define REG_EMAC_SA4T  (*(__IO uint32_t*)0x400B00B4U) /**< \brief (EMAC) Specific Address 4 Top Register */
  #define REG_EMAC_TID   (*(__IO uint32_t*)0x400B00B8U) /**< \brief (EMAC) Type ID Checking Register */
  #define REG_EMAC_USRIO (*(__IO uint32_t*)0x400B00C0U) /**< \brief (EMAC) User Input/Output Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_EMAC_INSTANCE_ */
