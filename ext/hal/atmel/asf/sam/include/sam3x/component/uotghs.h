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

#ifndef _SAM3XA_UOTGHS_COMPONENT_
#define _SAM3XA_UOTGHS_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR USB On-The-Go Interface */
/* ============================================================================= */
/** \addtogroup SAM3XA_UOTGHS USB On-The-Go Interface */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief UotghsDevdma hardware registers */
typedef struct {
  __IO uint32_t UOTGHS_DEVDMANXTDSC;  /**< \brief (UotghsDevdma Offset: 0x0) Device DMA Channel Next Descriptor Address Register */
  __IO uint32_t UOTGHS_DEVDMAADDRESS; /**< \brief (UotghsDevdma Offset: 0x4) Device DMA Channel Address Register */
  __IO uint32_t UOTGHS_DEVDMACONTROL; /**< \brief (UotghsDevdma Offset: 0x8) Device DMA Channel Control Register */
  __IO uint32_t UOTGHS_DEVDMASTATUS;  /**< \brief (UotghsDevdma Offset: 0xC) Device DMA Channel Status Register */
} UotghsDevdma;
/** \brief UotghsHstdma hardware registers */
typedef struct {
  __IO uint32_t UOTGHS_HSTDMANXTDSC;  /**< \brief (UotghsHstdma Offset: 0x0) Host DMA Channel Next Descriptor Address Register */
  __IO uint32_t UOTGHS_HSTDMAADDRESS; /**< \brief (UotghsHstdma Offset: 0x4) Host DMA Channel Address Register */
  __IO uint32_t UOTGHS_HSTDMACONTROL; /**< \brief (UotghsHstdma Offset: 0x8) Host DMA Channel Control Register */
  __IO uint32_t UOTGHS_HSTDMASTATUS;  /**< \brief (UotghsHstdma Offset: 0xC) Host DMA Channel Status Register */
} UotghsHstdma;
/** \brief Uotghs hardware registers */
#define UOTGHSDEVDMA_NUMBER 7
#define UOTGHSHSTDMA_NUMBER 7
typedef struct {
  __IO uint32_t     UOTGHS_DEVCTRL;                     /**< \brief (Uotghs Offset: 0x0000) Device General Control Register */
  __I  uint32_t     UOTGHS_DEVISR;                      /**< \brief (Uotghs Offset: 0x0004) Device Global Interrupt Status Register */
  __O  uint32_t     UOTGHS_DEVICR;                      /**< \brief (Uotghs Offset: 0x0008) Device Global Interrupt Clear Register */
  __O  uint32_t     UOTGHS_DEVIFR;                      /**< \brief (Uotghs Offset: 0x000C) Device Global Interrupt Set Register */
  __I  uint32_t     UOTGHS_DEVIMR;                      /**< \brief (Uotghs Offset: 0x0010) Device Global Interrupt Mask Register */
  __O  uint32_t     UOTGHS_DEVIDR;                      /**< \brief (Uotghs Offset: 0x0014) Device Global Interrupt Disable Register */
  __O  uint32_t     UOTGHS_DEVIER;                      /**< \brief (Uotghs Offset: 0x0018) Device Global Interrupt Enable Register */
  __IO uint32_t     UOTGHS_DEVEPT;                      /**< \brief (Uotghs Offset: 0x001C) Device Endpoint Register */
  __I  uint32_t     UOTGHS_DEVFNUM;                     /**< \brief (Uotghs Offset: 0x0020) Device Frame Number Register */
  __I  uint32_t     Reserved1[55];
  __IO uint32_t     UOTGHS_DEVEPTCFG[10];               /**< \brief (Uotghs Offset: 0x100) Device Endpoint Configuration Register (n = 0) */
  __I  uint32_t     Reserved2[2];
  __I  uint32_t     UOTGHS_DEVEPTISR[10];               /**< \brief (Uotghs Offset: 0x130) Device Endpoint Status Register (n = 0) */
  __I  uint32_t     Reserved3[2];
  __O  uint32_t     UOTGHS_DEVEPTICR[10];               /**< \brief (Uotghs Offset: 0x160) Device Endpoint Clear Register (n = 0) */
  __I  uint32_t     Reserved4[2];
  __O  uint32_t     UOTGHS_DEVEPTIFR[10];               /**< \brief (Uotghs Offset: 0x190) Device Endpoint Set Register (n = 0) */
  __I  uint32_t     Reserved5[2];
  __I  uint32_t     UOTGHS_DEVEPTIMR[10];               /**< \brief (Uotghs Offset: 0x1C0) Device Endpoint Mask Register (n = 0) */
  __I  uint32_t     Reserved6[2];
  __O  uint32_t     UOTGHS_DEVEPTIER[10];               /**< \brief (Uotghs Offset: 0x1F0) Device Endpoint Enable Register (n = 0) */
  __I  uint32_t     Reserved7[2];
  __O  uint32_t     UOTGHS_DEVEPTIDR[10];               /**< \brief (Uotghs Offset: 0x220) Device Endpoint Disable Register (n = 0) */
  __I  uint32_t     Reserved8[50];
       UotghsDevdma UOTGHS_DEVDMA[UOTGHSDEVDMA_NUMBER]; /**< \brief (Uotghs Offset: 0x310) n = 1 .. 7 */
  __I  uint32_t     Reserved9[32];
  __IO uint32_t     UOTGHS_HSTCTRL;                     /**< \brief (Uotghs Offset: 0x0400) Host General Control Register */
  __I  uint32_t     UOTGHS_HSTISR;                      /**< \brief (Uotghs Offset: 0x0404) Host Global Interrupt Status Register */
  __O  uint32_t     UOTGHS_HSTICR;                      /**< \brief (Uotghs Offset: 0x0408) Host Global Interrupt Clear Register */
  __O  uint32_t     UOTGHS_HSTIFR;                      /**< \brief (Uotghs Offset: 0x040C) Host Global Interrupt Set Register */
  __I  uint32_t     UOTGHS_HSTIMR;                      /**< \brief (Uotghs Offset: 0x0410) Host Global Interrupt Mask Register */
  __O  uint32_t     UOTGHS_HSTIDR;                      /**< \brief (Uotghs Offset: 0x0414) Host Global Interrupt Disable Register */
  __O  uint32_t     UOTGHS_HSTIER;                      /**< \brief (Uotghs Offset: 0x0418) Host Global Interrupt Enable Register */
  __IO uint32_t     UOTGHS_HSTPIP;                      /**< \brief (Uotghs Offset: 0x0041C) Host Pipe Register */
  __IO uint32_t     UOTGHS_HSTFNUM;                     /**< \brief (Uotghs Offset: 0x0420) Host Frame Number Register */
  __IO uint32_t     UOTGHS_HSTADDR1;                    /**< \brief (Uotghs Offset: 0x0424) Host Address 1 Register */
  __IO uint32_t     UOTGHS_HSTADDR2;                    /**< \brief (Uotghs Offset: 0x0428) Host Address 2 Register */
  __IO uint32_t     UOTGHS_HSTADDR3;                    /**< \brief (Uotghs Offset: 0x042C) Host Address 3 Register */
  __I  uint32_t     Reserved10[52];
  __IO uint32_t     UOTGHS_HSTPIPCFG[10];               /**< \brief (Uotghs Offset: 0x500) Host Pipe Configuration Register (n = 0) */
  __I  uint32_t     Reserved11[2];
  __I  uint32_t     UOTGHS_HSTPIPISR[10];               /**< \brief (Uotghs Offset: 0x530) Host Pipe Status Register (n = 0) */
  __I  uint32_t     Reserved12[2];
  __O  uint32_t     UOTGHS_HSTPIPICR[10];               /**< \brief (Uotghs Offset: 0x560) Host Pipe Clear Register (n = 0) */
  __I  uint32_t     Reserved13[2];
  __O  uint32_t     UOTGHS_HSTPIPIFR[10];               /**< \brief (Uotghs Offset: 0x590) Host Pipe Set Register (n = 0) */
  __I  uint32_t     Reserved14[2];
  __I  uint32_t     UOTGHS_HSTPIPIMR[10];               /**< \brief (Uotghs Offset: 0x5C0) Host Pipe Mask Register (n = 0) */
  __I  uint32_t     Reserved15[2];
  __O  uint32_t     UOTGHS_HSTPIPIER[10];               /**< \brief (Uotghs Offset: 0x5F0) Host Pipe Enable Register (n = 0) */
  __I  uint32_t     Reserved16[2];
  __O  uint32_t     UOTGHS_HSTPIPIDR[10];               /**< \brief (Uotghs Offset: 0x620) Host Pipe Disable Register (n = 0) */
  __I  uint32_t     Reserved17[2];
  __IO uint32_t     UOTGHS_HSTPIPINRQ[10];              /**< \brief (Uotghs Offset: 0x650) Host Pipe IN Request Register (n = 0) */
  __I  uint32_t     Reserved18[2];
  __IO uint32_t     UOTGHS_HSTPIPERR[10];               /**< \brief (Uotghs Offset: 0x680) Host Pipe Error Register (n = 0) */
  __I  uint32_t     Reserved19[26];
       UotghsHstdma UOTGHS_HSTDMA[UOTGHSHSTDMA_NUMBER]; /**< \brief (Uotghs Offset: 0x710) n = 1 .. 7 */
  __I  uint32_t     Reserved20[32];
  __IO uint32_t     UOTGHS_CTRL;                        /**< \brief (Uotghs Offset: 0x0800) General Control Register */
  __I  uint32_t     UOTGHS_SR;                          /**< \brief (Uotghs Offset: 0x0804) General Status Register */
  __O  uint32_t     UOTGHS_SCR;                         /**< \brief (Uotghs Offset: 0x0808) General Status Clear Register */
  __O  uint32_t     UOTGHS_SFR;                         /**< \brief (Uotghs Offset: 0x080C) General Status Set Register */
  __I  uint32_t     Reserved21[7];
  __I  uint32_t     UOTGHS_FSM;                         /**< \brief (Uotghs Offset: 0x082C) General Finite State Machine Register */
} Uotghs;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- UOTGHS_DEVCTRL : (UOTGHS Offset: 0x0000) Device General Control Register -------- */
#define UOTGHS_DEVCTRL_UADD_Pos 0
#define UOTGHS_DEVCTRL_UADD_Msk (0x7fu << UOTGHS_DEVCTRL_UADD_Pos) /**< \brief (UOTGHS_DEVCTRL) USB Address */
#define UOTGHS_DEVCTRL_UADD(value) ((UOTGHS_DEVCTRL_UADD_Msk & ((value) << UOTGHS_DEVCTRL_UADD_Pos)))
#define UOTGHS_DEVCTRL_ADDEN (0x1u << 7) /**< \brief (UOTGHS_DEVCTRL) Address Enable */
#define UOTGHS_DEVCTRL_DETACH (0x1u << 8) /**< \brief (UOTGHS_DEVCTRL) Detach */
#define UOTGHS_DEVCTRL_RMWKUP (0x1u << 9) /**< \brief (UOTGHS_DEVCTRL) Remote Wake-Up */
#define UOTGHS_DEVCTRL_SPDCONF_Pos 10
#define UOTGHS_DEVCTRL_SPDCONF_Msk (0x3u << UOTGHS_DEVCTRL_SPDCONF_Pos) /**< \brief (UOTGHS_DEVCTRL) Mode Configuration */
#define   UOTGHS_DEVCTRL_SPDCONF_NORMAL (0x0u << 10) /**< \brief (UOTGHS_DEVCTRL) The peripheral starts in full-speed mode and performs a high-speed reset to switch to the high-speed mode if the host is high-speed capable. */
#define   UOTGHS_DEVCTRL_SPDCONF_LOW_POWER (0x1u << 10) /**< \brief (UOTGHS_DEVCTRL) For a better consumption, if high-speed is not needed. */
#define   UOTGHS_DEVCTRL_SPDCONF_HIGH_SPEED (0x2u << 10) /**< \brief (UOTGHS_DEVCTRL) Forced high speed. */
#define   UOTGHS_DEVCTRL_SPDCONF_FORCED_FS (0x3u << 10) /**< \brief (UOTGHS_DEVCTRL) The peripheral remains in full-speed mode whatever the host speed capability. */
#define UOTGHS_DEVCTRL_LS (0x1u << 12) /**< \brief (UOTGHS_DEVCTRL) Low-Speed Mode Force */
#define UOTGHS_DEVCTRL_TSTJ (0x1u << 13) /**< \brief (UOTGHS_DEVCTRL) Test mode J */
#define UOTGHS_DEVCTRL_TSTK (0x1u << 14) /**< \brief (UOTGHS_DEVCTRL) Test mode K */
#define UOTGHS_DEVCTRL_TSTPCKT (0x1u << 15) /**< \brief (UOTGHS_DEVCTRL) Test packet mode */
#define UOTGHS_DEVCTRL_OPMODE2 (0x1u << 16) /**< \brief (UOTGHS_DEVCTRL) Specific Operational mode */
/* -------- UOTGHS_DEVISR : (UOTGHS Offset: 0x0004) Device Global Interrupt Status Register -------- */
#define UOTGHS_DEVISR_SUSP (0x1u << 0) /**< \brief (UOTGHS_DEVISR) Suspend Interrupt */
#define UOTGHS_DEVISR_MSOF (0x1u << 1) /**< \brief (UOTGHS_DEVISR) Micro Start of Frame Interrupt */
#define UOTGHS_DEVISR_SOF (0x1u << 2) /**< \brief (UOTGHS_DEVISR) Start of Frame Interrupt */
#define UOTGHS_DEVISR_EORST (0x1u << 3) /**< \brief (UOTGHS_DEVISR) End of Reset Interrupt */
#define UOTGHS_DEVISR_WAKEUP (0x1u << 4) /**< \brief (UOTGHS_DEVISR) Wake-Up Interrupt */
#define UOTGHS_DEVISR_EORSM (0x1u << 5) /**< \brief (UOTGHS_DEVISR) End of Resume Interrupt */
#define UOTGHS_DEVISR_UPRSM (0x1u << 6) /**< \brief (UOTGHS_DEVISR) Upstream Resume Interrupt */
#define UOTGHS_DEVISR_PEP_0 (0x1u << 12) /**< \brief (UOTGHS_DEVISR) Endpoint 0 Interrupt */
#define UOTGHS_DEVISR_PEP_1 (0x1u << 13) /**< \brief (UOTGHS_DEVISR) Endpoint 1 Interrupt */
#define UOTGHS_DEVISR_PEP_2 (0x1u << 14) /**< \brief (UOTGHS_DEVISR) Endpoint 2 Interrupt */
#define UOTGHS_DEVISR_PEP_3 (0x1u << 15) /**< \brief (UOTGHS_DEVISR) Endpoint 3 Interrupt */
#define UOTGHS_DEVISR_PEP_4 (0x1u << 16) /**< \brief (UOTGHS_DEVISR) Endpoint 4 Interrupt */
#define UOTGHS_DEVISR_PEP_5 (0x1u << 17) /**< \brief (UOTGHS_DEVISR) Endpoint 5 Interrupt */
#define UOTGHS_DEVISR_PEP_6 (0x1u << 18) /**< \brief (UOTGHS_DEVISR) Endpoint 6 Interrupt */
#define UOTGHS_DEVISR_PEP_7 (0x1u << 19) /**< \brief (UOTGHS_DEVISR) Endpoint 7 Interrupt */
#define UOTGHS_DEVISR_PEP_8 (0x1u << 20) /**< \brief (UOTGHS_DEVISR) Endpoint 8 Interrupt */
#define UOTGHS_DEVISR_PEP_9 (0x1u << 21) /**< \brief (UOTGHS_DEVISR) Endpoint 9 Interrupt */
#define UOTGHS_DEVISR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_DEVISR) DMA Channel 1 Interrupt */
#define UOTGHS_DEVISR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_DEVISR) DMA Channel 2 Interrupt */
#define UOTGHS_DEVISR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_DEVISR) DMA Channel 3 Interrupt */
#define UOTGHS_DEVISR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_DEVISR) DMA Channel 4 Interrupt */
#define UOTGHS_DEVISR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_DEVISR) DMA Channel 5 Interrupt */
#define UOTGHS_DEVISR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_DEVISR) DMA Channel 6 Interrupt */
/* -------- UOTGHS_DEVICR : (UOTGHS Offset: 0x0008) Device Global Interrupt Clear Register -------- */
#define UOTGHS_DEVICR_SUSPC (0x1u << 0) /**< \brief (UOTGHS_DEVICR) Suspend Interrupt Clear */
#define UOTGHS_DEVICR_MSOFC (0x1u << 1) /**< \brief (UOTGHS_DEVICR) Micro Start of Frame Interrupt Clear */
#define UOTGHS_DEVICR_SOFC (0x1u << 2) /**< \brief (UOTGHS_DEVICR) Start of Frame Interrupt Clear */
#define UOTGHS_DEVICR_EORSTC (0x1u << 3) /**< \brief (UOTGHS_DEVICR) End of Reset Interrupt Clear */
#define UOTGHS_DEVICR_WAKEUPC (0x1u << 4) /**< \brief (UOTGHS_DEVICR) Wake-Up Interrupt Clear */
#define UOTGHS_DEVICR_EORSMC (0x1u << 5) /**< \brief (UOTGHS_DEVICR) End of Resume Interrupt Clear */
#define UOTGHS_DEVICR_UPRSMC (0x1u << 6) /**< \brief (UOTGHS_DEVICR) Upstream Resume Interrupt Clear */
/* -------- UOTGHS_DEVIFR : (UOTGHS Offset: 0x000C) Device Global Interrupt Set Register -------- */
#define UOTGHS_DEVIFR_SUSPS (0x1u << 0) /**< \brief (UOTGHS_DEVIFR) Suspend Interrupt Set */
#define UOTGHS_DEVIFR_MSOFS (0x1u << 1) /**< \brief (UOTGHS_DEVIFR) Micro Start of Frame Interrupt Set */
#define UOTGHS_DEVIFR_SOFS (0x1u << 2) /**< \brief (UOTGHS_DEVIFR) Start of Frame Interrupt Set */
#define UOTGHS_DEVIFR_EORSTS (0x1u << 3) /**< \brief (UOTGHS_DEVIFR) End of Reset Interrupt Set */
#define UOTGHS_DEVIFR_WAKEUPS (0x1u << 4) /**< \brief (UOTGHS_DEVIFR) Wake-Up Interrupt Set */
#define UOTGHS_DEVIFR_EORSMS (0x1u << 5) /**< \brief (UOTGHS_DEVIFR) End of Resume Interrupt Set */
#define UOTGHS_DEVIFR_UPRSMS (0x1u << 6) /**< \brief (UOTGHS_DEVIFR) Upstream Resume Interrupt Set */
#define UOTGHS_DEVIFR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_DEVIFR) DMA Channel 1 Interrupt Set */
#define UOTGHS_DEVIFR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_DEVIFR) DMA Channel 2 Interrupt Set */
#define UOTGHS_DEVIFR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_DEVIFR) DMA Channel 3 Interrupt Set */
#define UOTGHS_DEVIFR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_DEVIFR) DMA Channel 4 Interrupt Set */
#define UOTGHS_DEVIFR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_DEVIFR) DMA Channel 5 Interrupt Set */
#define UOTGHS_DEVIFR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_DEVIFR) DMA Channel 6 Interrupt Set */
/* -------- UOTGHS_DEVIMR : (UOTGHS Offset: 0x0010) Device Global Interrupt Mask Register -------- */
#define UOTGHS_DEVIMR_SUSPE (0x1u << 0) /**< \brief (UOTGHS_DEVIMR) Suspend Interrupt Mask */
#define UOTGHS_DEVIMR_MSOFE (0x1u << 1) /**< \brief (UOTGHS_DEVIMR) Micro Start of Frame Interrupt Mask */
#define UOTGHS_DEVIMR_SOFE (0x1u << 2) /**< \brief (UOTGHS_DEVIMR) Start of Frame Interrupt Mask */
#define UOTGHS_DEVIMR_EORSTE (0x1u << 3) /**< \brief (UOTGHS_DEVIMR) End of Reset Interrupt Mask */
#define UOTGHS_DEVIMR_WAKEUPE (0x1u << 4) /**< \brief (UOTGHS_DEVIMR) Wake-Up Interrupt Mask */
#define UOTGHS_DEVIMR_EORSME (0x1u << 5) /**< \brief (UOTGHS_DEVIMR) End of Resume Interrupt Mask */
#define UOTGHS_DEVIMR_UPRSME (0x1u << 6) /**< \brief (UOTGHS_DEVIMR) Upstream Resume Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_0 (0x1u << 12) /**< \brief (UOTGHS_DEVIMR) Endpoint 0 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_1 (0x1u << 13) /**< \brief (UOTGHS_DEVIMR) Endpoint 1 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_2 (0x1u << 14) /**< \brief (UOTGHS_DEVIMR) Endpoint 2 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_3 (0x1u << 15) /**< \brief (UOTGHS_DEVIMR) Endpoint 3 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_4 (0x1u << 16) /**< \brief (UOTGHS_DEVIMR) Endpoint 4 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_5 (0x1u << 17) /**< \brief (UOTGHS_DEVIMR) Endpoint 5 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_6 (0x1u << 18) /**< \brief (UOTGHS_DEVIMR) Endpoint 6 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_7 (0x1u << 19) /**< \brief (UOTGHS_DEVIMR) Endpoint 7 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_8 (0x1u << 20) /**< \brief (UOTGHS_DEVIMR) Endpoint 8 Interrupt Mask */
#define UOTGHS_DEVIMR_PEP_9 (0x1u << 21) /**< \brief (UOTGHS_DEVIMR) Endpoint 9 Interrupt Mask */
#define UOTGHS_DEVIMR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_DEVIMR) DMA Channel 1 Interrupt Mask */
#define UOTGHS_DEVIMR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_DEVIMR) DMA Channel 2 Interrupt Mask */
#define UOTGHS_DEVIMR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_DEVIMR) DMA Channel 3 Interrupt Mask */
#define UOTGHS_DEVIMR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_DEVIMR) DMA Channel 4 Interrupt Mask */
#define UOTGHS_DEVIMR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_DEVIMR) DMA Channel 5 Interrupt Mask */
#define UOTGHS_DEVIMR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_DEVIMR) DMA Channel 6 Interrupt Mask */
/* -------- UOTGHS_DEVIDR : (UOTGHS Offset: 0x0014) Device Global Interrupt Disable Register -------- */
#define UOTGHS_DEVIDR_SUSPEC (0x1u << 0) /**< \brief (UOTGHS_DEVIDR) Suspend Interrupt Disable */
#define UOTGHS_DEVIDR_MSOFEC (0x1u << 1) /**< \brief (UOTGHS_DEVIDR) Micro Start of Frame Interrupt Disable */
#define UOTGHS_DEVIDR_SOFEC (0x1u << 2) /**< \brief (UOTGHS_DEVIDR) Start of Frame Interrupt Disable */
#define UOTGHS_DEVIDR_EORSTEC (0x1u << 3) /**< \brief (UOTGHS_DEVIDR) End of Reset Interrupt Disable */
#define UOTGHS_DEVIDR_WAKEUPEC (0x1u << 4) /**< \brief (UOTGHS_DEVIDR) Wake-Up Interrupt Disable */
#define UOTGHS_DEVIDR_EORSMEC (0x1u << 5) /**< \brief (UOTGHS_DEVIDR) End of Resume Interrupt Disable */
#define UOTGHS_DEVIDR_UPRSMEC (0x1u << 6) /**< \brief (UOTGHS_DEVIDR) Upstream Resume Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_0 (0x1u << 12) /**< \brief (UOTGHS_DEVIDR) Endpoint 0 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_1 (0x1u << 13) /**< \brief (UOTGHS_DEVIDR) Endpoint 1 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_2 (0x1u << 14) /**< \brief (UOTGHS_DEVIDR) Endpoint 2 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_3 (0x1u << 15) /**< \brief (UOTGHS_DEVIDR) Endpoint 3 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_4 (0x1u << 16) /**< \brief (UOTGHS_DEVIDR) Endpoint 4 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_5 (0x1u << 17) /**< \brief (UOTGHS_DEVIDR) Endpoint 5 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_6 (0x1u << 18) /**< \brief (UOTGHS_DEVIDR) Endpoint 6 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_7 (0x1u << 19) /**< \brief (UOTGHS_DEVIDR) Endpoint 7 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_8 (0x1u << 20) /**< \brief (UOTGHS_DEVIDR) Endpoint 8 Interrupt Disable */
#define UOTGHS_DEVIDR_PEP_9 (0x1u << 21) /**< \brief (UOTGHS_DEVIDR) Endpoint 9 Interrupt Disable */
#define UOTGHS_DEVIDR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_DEVIDR) DMA Channel 1 Interrupt Disable */
#define UOTGHS_DEVIDR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_DEVIDR) DMA Channel 2 Interrupt Disable */
#define UOTGHS_DEVIDR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_DEVIDR) DMA Channel 3 Interrupt Disable */
#define UOTGHS_DEVIDR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_DEVIDR) DMA Channel 4 Interrupt Disable */
#define UOTGHS_DEVIDR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_DEVIDR) DMA Channel 5 Interrupt Disable */
#define UOTGHS_DEVIDR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_DEVIDR) DMA Channel 6 Interrupt Disable */
/* -------- UOTGHS_DEVIER : (UOTGHS Offset: 0x0018) Device Global Interrupt Enable Register -------- */
#define UOTGHS_DEVIER_SUSPES (0x1u << 0) /**< \brief (UOTGHS_DEVIER) Suspend Interrupt Enable */
#define UOTGHS_DEVIER_MSOFES (0x1u << 1) /**< \brief (UOTGHS_DEVIER) Micro Start of Frame Interrupt Enable */
#define UOTGHS_DEVIER_SOFES (0x1u << 2) /**< \brief (UOTGHS_DEVIER) Start of Frame Interrupt Enable */
#define UOTGHS_DEVIER_EORSTES (0x1u << 3) /**< \brief (UOTGHS_DEVIER) End of Reset Interrupt Enable */
#define UOTGHS_DEVIER_WAKEUPES (0x1u << 4) /**< \brief (UOTGHS_DEVIER) Wake-Up Interrupt Enable */
#define UOTGHS_DEVIER_EORSMES (0x1u << 5) /**< \brief (UOTGHS_DEVIER) End of Resume Interrupt Enable */
#define UOTGHS_DEVIER_UPRSMES (0x1u << 6) /**< \brief (UOTGHS_DEVIER) Upstream Resume Interrupt Enable */
#define UOTGHS_DEVIER_PEP_0 (0x1u << 12) /**< \brief (UOTGHS_DEVIER) Endpoint 0 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_1 (0x1u << 13) /**< \brief (UOTGHS_DEVIER) Endpoint 1 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_2 (0x1u << 14) /**< \brief (UOTGHS_DEVIER) Endpoint 2 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_3 (0x1u << 15) /**< \brief (UOTGHS_DEVIER) Endpoint 3 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_4 (0x1u << 16) /**< \brief (UOTGHS_DEVIER) Endpoint 4 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_5 (0x1u << 17) /**< \brief (UOTGHS_DEVIER) Endpoint 5 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_6 (0x1u << 18) /**< \brief (UOTGHS_DEVIER) Endpoint 6 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_7 (0x1u << 19) /**< \brief (UOTGHS_DEVIER) Endpoint 7 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_8 (0x1u << 20) /**< \brief (UOTGHS_DEVIER) Endpoint 8 Interrupt Enable */
#define UOTGHS_DEVIER_PEP_9 (0x1u << 21) /**< \brief (UOTGHS_DEVIER) Endpoint 9 Interrupt Enable */
#define UOTGHS_DEVIER_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_DEVIER) DMA Channel 1 Interrupt Enable */
#define UOTGHS_DEVIER_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_DEVIER) DMA Channel 2 Interrupt Enable */
#define UOTGHS_DEVIER_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_DEVIER) DMA Channel 3 Interrupt Enable */
#define UOTGHS_DEVIER_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_DEVIER) DMA Channel 4 Interrupt Enable */
#define UOTGHS_DEVIER_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_DEVIER) DMA Channel 5 Interrupt Enable */
#define UOTGHS_DEVIER_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_DEVIER) DMA Channel 6 Interrupt Enable */
/* -------- UOTGHS_DEVEPT : (UOTGHS Offset: 0x001C) Device Endpoint Register -------- */
#define UOTGHS_DEVEPT_EPEN0 (0x1u << 0) /**< \brief (UOTGHS_DEVEPT) Endpoint 0 Enable */
#define UOTGHS_DEVEPT_EPEN1 (0x1u << 1) /**< \brief (UOTGHS_DEVEPT) Endpoint 1 Enable */
#define UOTGHS_DEVEPT_EPEN2 (0x1u << 2) /**< \brief (UOTGHS_DEVEPT) Endpoint 2 Enable */
#define UOTGHS_DEVEPT_EPEN3 (0x1u << 3) /**< \brief (UOTGHS_DEVEPT) Endpoint 3 Enable */
#define UOTGHS_DEVEPT_EPEN4 (0x1u << 4) /**< \brief (UOTGHS_DEVEPT) Endpoint 4 Enable */
#define UOTGHS_DEVEPT_EPEN5 (0x1u << 5) /**< \brief (UOTGHS_DEVEPT) Endpoint 5 Enable */
#define UOTGHS_DEVEPT_EPEN6 (0x1u << 6) /**< \brief (UOTGHS_DEVEPT) Endpoint 6 Enable */
#define UOTGHS_DEVEPT_EPEN7 (0x1u << 7) /**< \brief (UOTGHS_DEVEPT) Endpoint 7 Enable */
#define UOTGHS_DEVEPT_EPEN8 (0x1u << 8) /**< \brief (UOTGHS_DEVEPT) Endpoint 8 Enable */
#define UOTGHS_DEVEPT_EPRST0 (0x1u << 16) /**< \brief (UOTGHS_DEVEPT) Endpoint 0 Reset */
#define UOTGHS_DEVEPT_EPRST1 (0x1u << 17) /**< \brief (UOTGHS_DEVEPT) Endpoint 1 Reset */
#define UOTGHS_DEVEPT_EPRST2 (0x1u << 18) /**< \brief (UOTGHS_DEVEPT) Endpoint 2 Reset */
#define UOTGHS_DEVEPT_EPRST3 (0x1u << 19) /**< \brief (UOTGHS_DEVEPT) Endpoint 3 Reset */
#define UOTGHS_DEVEPT_EPRST4 (0x1u << 20) /**< \brief (UOTGHS_DEVEPT) Endpoint 4 Reset */
#define UOTGHS_DEVEPT_EPRST5 (0x1u << 21) /**< \brief (UOTGHS_DEVEPT) Endpoint 5 Reset */
#define UOTGHS_DEVEPT_EPRST6 (0x1u << 22) /**< \brief (UOTGHS_DEVEPT) Endpoint 6 Reset */
#define UOTGHS_DEVEPT_EPRST7 (0x1u << 23) /**< \brief (UOTGHS_DEVEPT) Endpoint 7 Reset */
#define UOTGHS_DEVEPT_EPRST8 (0x1u << 24) /**< \brief (UOTGHS_DEVEPT) Endpoint 8 Reset */
/* -------- UOTGHS_DEVFNUM : (UOTGHS Offset: 0x0020) Device Frame Number Register -------- */
#define UOTGHS_DEVFNUM_MFNUM_Pos 0
#define UOTGHS_DEVFNUM_MFNUM_Msk (0x7u << UOTGHS_DEVFNUM_MFNUM_Pos) /**< \brief (UOTGHS_DEVFNUM) Micro Frame Number */
#define UOTGHS_DEVFNUM_FNUM_Pos 3
#define UOTGHS_DEVFNUM_FNUM_Msk (0x7ffu << UOTGHS_DEVFNUM_FNUM_Pos) /**< \brief (UOTGHS_DEVFNUM) Frame Number */
#define UOTGHS_DEVFNUM_FNCERR (0x1u << 15) /**< \brief (UOTGHS_DEVFNUM) Frame Number CRC Error */
/* -------- UOTGHS_DEVEPTCFG[10] : (UOTGHS Offset: 0x100) Device Endpoint Configuration Register (n = 0) -------- */
#define UOTGHS_DEVEPTCFG_ALLOC (0x1u << 1) /**< \brief (UOTGHS_DEVEPTCFG[10]) Endpoint Memory Allocate */
#define UOTGHS_DEVEPTCFG_EPBK_Pos 2
#define UOTGHS_DEVEPTCFG_EPBK_Msk (0x3u << UOTGHS_DEVEPTCFG_EPBK_Pos) /**< \brief (UOTGHS_DEVEPTCFG[10]) Endpoint Banks */
#define   UOTGHS_DEVEPTCFG_EPBK_1_BANK (0x0u << 2) /**< \brief (UOTGHS_DEVEPTCFG[10]) Single-bank endpoint */
#define   UOTGHS_DEVEPTCFG_EPBK_2_BANK (0x1u << 2) /**< \brief (UOTGHS_DEVEPTCFG[10]) Double-bank endpoint */
#define   UOTGHS_DEVEPTCFG_EPBK_3_BANK (0x2u << 2) /**< \brief (UOTGHS_DEVEPTCFG[10]) Triple-bank endpoint */
#define UOTGHS_DEVEPTCFG_EPSIZE_Pos 4
#define UOTGHS_DEVEPTCFG_EPSIZE_Msk (0x7u << UOTGHS_DEVEPTCFG_EPSIZE_Pos) /**< \brief (UOTGHS_DEVEPTCFG[10]) Endpoint Size */
#define   UOTGHS_DEVEPTCFG_EPSIZE_8_BYTE (0x0u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 8 bytes */
#define   UOTGHS_DEVEPTCFG_EPSIZE_16_BYTE (0x1u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 16 bytes */
#define   UOTGHS_DEVEPTCFG_EPSIZE_32_BYTE (0x2u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 32 bytes */
#define   UOTGHS_DEVEPTCFG_EPSIZE_64_BYTE (0x3u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 64 bytes */
#define   UOTGHS_DEVEPTCFG_EPSIZE_128_BYTE (0x4u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 128 bytes */
#define   UOTGHS_DEVEPTCFG_EPSIZE_256_BYTE (0x5u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 256 bytes */
#define   UOTGHS_DEVEPTCFG_EPSIZE_512_BYTE (0x6u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 512 bytes */
#define   UOTGHS_DEVEPTCFG_EPSIZE_1024_BYTE (0x7u << 4) /**< \brief (UOTGHS_DEVEPTCFG[10]) 1024 bytes */
#define UOTGHS_DEVEPTCFG_EPDIR (0x1u << 8) /**< \brief (UOTGHS_DEVEPTCFG[10]) Endpoint Direction */
#define   UOTGHS_DEVEPTCFG_EPDIR_OUT (0x0u << 8) /**< \brief (UOTGHS_DEVEPTCFG[10]) The endpoint direction is OUT. */
#define   UOTGHS_DEVEPTCFG_EPDIR_IN (0x1u << 8) /**< \brief (UOTGHS_DEVEPTCFG[10]) The endpoint direction is IN (nor for control endpoints). */
#define UOTGHS_DEVEPTCFG_AUTOSW (0x1u << 9) /**< \brief (UOTGHS_DEVEPTCFG[10]) Automatic Switch */
#define UOTGHS_DEVEPTCFG_EPTYPE_Pos 11
#define UOTGHS_DEVEPTCFG_EPTYPE_Msk (0x3u << UOTGHS_DEVEPTCFG_EPTYPE_Pos) /**< \brief (UOTGHS_DEVEPTCFG[10]) Endpoint Type */
#define   UOTGHS_DEVEPTCFG_EPTYPE_CTRL (0x0u << 11) /**< \brief (UOTGHS_DEVEPTCFG[10]) Control */
#define   UOTGHS_DEVEPTCFG_EPTYPE_ISO (0x1u << 11) /**< \brief (UOTGHS_DEVEPTCFG[10]) Isochronous */
#define   UOTGHS_DEVEPTCFG_EPTYPE_BLK (0x2u << 11) /**< \brief (UOTGHS_DEVEPTCFG[10]) Bulk */
#define   UOTGHS_DEVEPTCFG_EPTYPE_INTRPT (0x3u << 11) /**< \brief (UOTGHS_DEVEPTCFG[10]) Interrupt */
#define UOTGHS_DEVEPTCFG_NBTRANS_Pos 13
#define UOTGHS_DEVEPTCFG_NBTRANS_Msk (0x3u << UOTGHS_DEVEPTCFG_NBTRANS_Pos) /**< \brief (UOTGHS_DEVEPTCFG[10]) Number of transaction per microframe for isochronous endpoint */
#define   UOTGHS_DEVEPTCFG_NBTRANS_0_TRANS (0x0u << 13) /**< \brief (UOTGHS_DEVEPTCFG[10]) reserved to endpoint that does not have the high-bandwidth isochronous capability. */
#define   UOTGHS_DEVEPTCFG_NBTRANS_1_TRANS (0x1u << 13) /**< \brief (UOTGHS_DEVEPTCFG[10]) default value: one transaction per micro-frame. */
#define   UOTGHS_DEVEPTCFG_NBTRANS_2_TRANS (0x2u << 13) /**< \brief (UOTGHS_DEVEPTCFG[10]) 2 transactions per micro-frame. This endpoint should be configured as double-bank. */
#define   UOTGHS_DEVEPTCFG_NBTRANS_3_TRANS (0x3u << 13) /**< \brief (UOTGHS_DEVEPTCFG[10]) 3 transactions per micro-frame. This endpoint should be configured as triple-bank. */
/* -------- UOTGHS_DEVEPTISR[10] : (UOTGHS Offset: 0x130) Device Endpoint Status Register (n = 0) -------- */
#define UOTGHS_DEVEPTISR_TXINI (0x1u << 0) /**< \brief (UOTGHS_DEVEPTISR[10]) Transmitted IN Data Interrupt */
#define UOTGHS_DEVEPTISR_RXOUTI (0x1u << 1) /**< \brief (UOTGHS_DEVEPTISR[10]) Received OUT Data Interrupt */
#define UOTGHS_DEVEPTISR_RXSTPI (0x1u << 2) /**< \brief (UOTGHS_DEVEPTISR[10]) Received SETUP Interrupt */
#define UOTGHS_DEVEPTISR_NAKOUTI (0x1u << 3) /**< \brief (UOTGHS_DEVEPTISR[10]) NAKed OUT Interrupt */
#define UOTGHS_DEVEPTISR_NAKINI (0x1u << 4) /**< \brief (UOTGHS_DEVEPTISR[10]) NAKed IN Interrupt */
#define UOTGHS_DEVEPTISR_OVERFI (0x1u << 5) /**< \brief (UOTGHS_DEVEPTISR[10]) Overflow Interrupt */
#define UOTGHS_DEVEPTISR_STALLEDI (0x1u << 6) /**< \brief (UOTGHS_DEVEPTISR[10]) STALLed Interrupt */
#define UOTGHS_DEVEPTISR_SHORTPACKET (0x1u << 7) /**< \brief (UOTGHS_DEVEPTISR[10]) Short Packet Interrupt */
#define UOTGHS_DEVEPTISR_DTSEQ_Pos 8
#define UOTGHS_DEVEPTISR_DTSEQ_Msk (0x3u << UOTGHS_DEVEPTISR_DTSEQ_Pos) /**< \brief (UOTGHS_DEVEPTISR[10]) Data Toggle Sequence */
#define   UOTGHS_DEVEPTISR_DTSEQ_DATA0 (0x0u << 8) /**< \brief (UOTGHS_DEVEPTISR[10]) Data0 toggle sequence */
#define   UOTGHS_DEVEPTISR_DTSEQ_DATA1 (0x1u << 8) /**< \brief (UOTGHS_DEVEPTISR[10]) Data1 toggle sequence */
#define   UOTGHS_DEVEPTISR_DTSEQ_DATA2 (0x2u << 8) /**< \brief (UOTGHS_DEVEPTISR[10]) Reserved for high-bandwidth isochronous endpoint */
#define   UOTGHS_DEVEPTISR_DTSEQ_MDATA (0x3u << 8) /**< \brief (UOTGHS_DEVEPTISR[10]) Reserved for high-bandwidth isochronous endpoint */
#define UOTGHS_DEVEPTISR_NBUSYBK_Pos 12
#define UOTGHS_DEVEPTISR_NBUSYBK_Msk (0x3u << UOTGHS_DEVEPTISR_NBUSYBK_Pos) /**< \brief (UOTGHS_DEVEPTISR[10]) Number of Busy Banks */
#define   UOTGHS_DEVEPTISR_NBUSYBK_0_BUSY (0x0u << 12) /**< \brief (UOTGHS_DEVEPTISR[10]) 0 busy bank (all banks free) */
#define   UOTGHS_DEVEPTISR_NBUSYBK_1_BUSY (0x1u << 12) /**< \brief (UOTGHS_DEVEPTISR[10]) 1 busy bank */
#define   UOTGHS_DEVEPTISR_NBUSYBK_2_BUSY (0x2u << 12) /**< \brief (UOTGHS_DEVEPTISR[10]) 2 busy banks */
#define   UOTGHS_DEVEPTISR_NBUSYBK_3_BUSY (0x3u << 12) /**< \brief (UOTGHS_DEVEPTISR[10]) 3 busy banks */
#define UOTGHS_DEVEPTISR_CURRBK_Pos 14
#define UOTGHS_DEVEPTISR_CURRBK_Msk (0x3u << UOTGHS_DEVEPTISR_CURRBK_Pos) /**< \brief (UOTGHS_DEVEPTISR[10]) Current Bank */
#define   UOTGHS_DEVEPTISR_CURRBK_BANK0 (0x0u << 14) /**< \brief (UOTGHS_DEVEPTISR[10]) Current bank is bank0 */
#define   UOTGHS_DEVEPTISR_CURRBK_BANK1 (0x1u << 14) /**< \brief (UOTGHS_DEVEPTISR[10]) Current bank is bank1 */
#define   UOTGHS_DEVEPTISR_CURRBK_BANK2 (0x2u << 14) /**< \brief (UOTGHS_DEVEPTISR[10]) Current bank is bank2 */
#define UOTGHS_DEVEPTISR_RWALL (0x1u << 16) /**< \brief (UOTGHS_DEVEPTISR[10]) Read-write Allowed */
#define UOTGHS_DEVEPTISR_CTRLDIR (0x1u << 17) /**< \brief (UOTGHS_DEVEPTISR[10]) Control Direction */
#define UOTGHS_DEVEPTISR_CFGOK (0x1u << 18) /**< \brief (UOTGHS_DEVEPTISR[10]) Configuration OK Status */
#define UOTGHS_DEVEPTISR_BYCT_Pos 20
#define UOTGHS_DEVEPTISR_BYCT_Msk (0x7ffu << UOTGHS_DEVEPTISR_BYCT_Pos) /**< \brief (UOTGHS_DEVEPTISR[10]) Byte Count */
#define UOTGHS_DEVEPTISR_UNDERFI (0x1u << 2) /**< \brief (UOTGHS_DEVEPTISR[10]) Underflow Interrupt */
#define UOTGHS_DEVEPTISR_HBISOINERRI (0x1u << 3) /**< \brief (UOTGHS_DEVEPTISR[10]) High Bandwidth Isochronous IN Underflow Error Interrupt */
#define UOTGHS_DEVEPTISR_HBISOFLUSHI (0x1u << 4) /**< \brief (UOTGHS_DEVEPTISR[10]) High Bandwidth Isochronous IN Flush Interrupt */
#define UOTGHS_DEVEPTISR_CRCERRI (0x1u << 6) /**< \brief (UOTGHS_DEVEPTISR[10]) CRC Error Interrupt */
#define UOTGHS_DEVEPTISR_ERRORTRANS (0x1u << 10) /**< \brief (UOTGHS_DEVEPTISR[10]) High-bandwidth Isochronous OUT Endpoint Transaction Error Interrupt */
/* -------- UOTGHS_DEVEPTICR[10] : (UOTGHS Offset: 0x160) Device Endpoint Clear Register (n = 0) -------- */
#define UOTGHS_DEVEPTICR_TXINIC (0x1u << 0) /**< \brief (UOTGHS_DEVEPTICR[10]) Transmitted IN Data Interrupt Clear */
#define UOTGHS_DEVEPTICR_RXOUTIC (0x1u << 1) /**< \brief (UOTGHS_DEVEPTICR[10]) Received OUT Data Interrupt Clear */
#define UOTGHS_DEVEPTICR_RXSTPIC (0x1u << 2) /**< \brief (UOTGHS_DEVEPTICR[10]) Received SETUP Interrupt Clear */
#define UOTGHS_DEVEPTICR_NAKOUTIC (0x1u << 3) /**< \brief (UOTGHS_DEVEPTICR[10]) NAKed OUT Interrupt Clear */
#define UOTGHS_DEVEPTICR_NAKINIC (0x1u << 4) /**< \brief (UOTGHS_DEVEPTICR[10]) NAKed IN Interrupt Clear */
#define UOTGHS_DEVEPTICR_OVERFIC (0x1u << 5) /**< \brief (UOTGHS_DEVEPTICR[10]) Overflow Interrupt Clear */
#define UOTGHS_DEVEPTICR_STALLEDIC (0x1u << 6) /**< \brief (UOTGHS_DEVEPTICR[10]) STALLed Interrupt Clear */
#define UOTGHS_DEVEPTICR_SHORTPACKETC (0x1u << 7) /**< \brief (UOTGHS_DEVEPTICR[10]) Short Packet Interrupt Clear */
#define UOTGHS_DEVEPTICR_UNDERFIC (0x1u << 2) /**< \brief (UOTGHS_DEVEPTICR[10]) Underflow Interrupt Clear */
#define UOTGHS_DEVEPTICR_HBISOINERRIC (0x1u << 3) /**< \brief (UOTGHS_DEVEPTICR[10]) High bandwidth isochronous IN Underflow Error Interrupt Clear */
#define UOTGHS_DEVEPTICR_HBISOFLUSHIC (0x1u << 4) /**< \brief (UOTGHS_DEVEPTICR[10]) High Bandwidth Isochronous IN Flush Interrupt Clear */
#define UOTGHS_DEVEPTICR_CRCERRIC (0x1u << 6) /**< \brief (UOTGHS_DEVEPTICR[10]) CRC Error Interrupt Clear */
/* -------- UOTGHS_DEVEPTIFR[10] : (UOTGHS Offset: 0x190) Device Endpoint Set Register (n = 0) -------- */
#define UOTGHS_DEVEPTIFR_TXINIS (0x1u << 0) /**< \brief (UOTGHS_DEVEPTIFR[10]) Transmitted IN Data Interrupt Set */
#define UOTGHS_DEVEPTIFR_RXOUTIS (0x1u << 1) /**< \brief (UOTGHS_DEVEPTIFR[10]) Received OUT Data Interrupt Set */
#define UOTGHS_DEVEPTIFR_RXSTPIS (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIFR[10]) Received SETUP Interrupt Set */
#define UOTGHS_DEVEPTIFR_NAKOUTIS (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIFR[10]) NAKed OUT Interrupt Set */
#define UOTGHS_DEVEPTIFR_NAKINIS (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIFR[10]) NAKed IN Interrupt Set */
#define UOTGHS_DEVEPTIFR_OVERFIS (0x1u << 5) /**< \brief (UOTGHS_DEVEPTIFR[10]) Overflow Interrupt Set */
#define UOTGHS_DEVEPTIFR_STALLEDIS (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIFR[10]) STALLed Interrupt Set */
#define UOTGHS_DEVEPTIFR_SHORTPACKETS (0x1u << 7) /**< \brief (UOTGHS_DEVEPTIFR[10]) Short Packet Interrupt Set */
#define UOTGHS_DEVEPTIFR_NBUSYBKS (0x1u << 12) /**< \brief (UOTGHS_DEVEPTIFR[10]) Number of Busy Banks Interrupt Set */
#define UOTGHS_DEVEPTIFR_UNDERFIS (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIFR[10]) Underflow Interrupt Set */
#define UOTGHS_DEVEPTIFR_HBISOINERRIS (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIFR[10]) High bandwidth isochronous IN Underflow Error Interrupt Set */
#define UOTGHS_DEVEPTIFR_HBISOFLUSHIS (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIFR[10]) High Bandwidth Isochronous IN Flush Interrupt Set */
#define UOTGHS_DEVEPTIFR_CRCERRIS (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIFR[10]) CRC Error Interrupt Set */
/* -------- UOTGHS_DEVEPTIMR[10] : (UOTGHS Offset: 0x1C0) Device Endpoint Mask Register (n = 0) -------- */
#define UOTGHS_DEVEPTIMR_TXINE (0x1u << 0) /**< \brief (UOTGHS_DEVEPTIMR[10]) Transmitted IN Data Interrupt */
#define UOTGHS_DEVEPTIMR_RXOUTE (0x1u << 1) /**< \brief (UOTGHS_DEVEPTIMR[10]) Received OUT Data Interrupt */
#define UOTGHS_DEVEPTIMR_RXSTPE (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIMR[10]) Received SETUP Interrupt */
#define UOTGHS_DEVEPTIMR_NAKOUTE (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIMR[10]) NAKed OUT Interrupt */
#define UOTGHS_DEVEPTIMR_NAKINE (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIMR[10]) NAKed IN Interrupt */
#define UOTGHS_DEVEPTIMR_OVERFE (0x1u << 5) /**< \brief (UOTGHS_DEVEPTIMR[10]) Overflow Interrupt */
#define UOTGHS_DEVEPTIMR_STALLEDE (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIMR[10]) STALLed Interrupt */
#define UOTGHS_DEVEPTIMR_SHORTPACKETE (0x1u << 7) /**< \brief (UOTGHS_DEVEPTIMR[10]) Short Packet Interrupt */
#define UOTGHS_DEVEPTIMR_NBUSYBKE (0x1u << 12) /**< \brief (UOTGHS_DEVEPTIMR[10]) Number of Busy Banks Interrupt */
#define UOTGHS_DEVEPTIMR_KILLBK (0x1u << 13) /**< \brief (UOTGHS_DEVEPTIMR[10]) Kill IN Bank */
#define UOTGHS_DEVEPTIMR_FIFOCON (0x1u << 14) /**< \brief (UOTGHS_DEVEPTIMR[10]) FIFO Control */
#define UOTGHS_DEVEPTIMR_EPDISHDMA (0x1u << 16) /**< \brief (UOTGHS_DEVEPTIMR[10]) Endpoint Interrupts Disable HDMA Request */
#define UOTGHS_DEVEPTIMR_NYETDIS (0x1u << 17) /**< \brief (UOTGHS_DEVEPTIMR[10]) NYET Token Disable */
#define UOTGHS_DEVEPTIMR_RSTDT (0x1u << 18) /**< \brief (UOTGHS_DEVEPTIMR[10]) Reset Data Toggle */
#define UOTGHS_DEVEPTIMR_STALLRQ (0x1u << 19) /**< \brief (UOTGHS_DEVEPTIMR[10]) STALL Request */
#define UOTGHS_DEVEPTIMR_UNDERFE (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIMR[10]) Underflow Interrupt */
#define UOTGHS_DEVEPTIMR_HBISOINERRE (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIMR[10]) High Bandwidth Isochronous IN Error Interrupt */
#define UOTGHS_DEVEPTIMR_HBISOFLUSHE (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIMR[10]) High Bandwidth Isochronous IN Flush Interrupt */
#define UOTGHS_DEVEPTIMR_CRCERRE (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIMR[10]) CRC Error Interrupt */
#define UOTGHS_DEVEPTIMR_MDATAE (0x1u << 8) /**< \brief (UOTGHS_DEVEPTIMR[10]) MData Interrupt */
#define UOTGHS_DEVEPTIMR_DATAXE (0x1u << 9) /**< \brief (UOTGHS_DEVEPTIMR[10]) DataX Interrupt */
#define UOTGHS_DEVEPTIMR_ERRORTRANSE (0x1u << 10) /**< \brief (UOTGHS_DEVEPTIMR[10]) Transaction Error Interrupt */
/* -------- UOTGHS_DEVEPTIER[10] : (UOTGHS Offset: 0x1F0) Device Endpoint Enable Register (n = 0) -------- */
#define UOTGHS_DEVEPTIER_TXINES (0x1u << 0) /**< \brief (UOTGHS_DEVEPTIER[10]) Transmitted IN Data Interrupt Enable */
#define UOTGHS_DEVEPTIER_RXOUTES (0x1u << 1) /**< \brief (UOTGHS_DEVEPTIER[10]) Received OUT Data Interrupt Enable */
#define UOTGHS_DEVEPTIER_RXSTPES (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIER[10]) Received SETUP Interrupt Enable */
#define UOTGHS_DEVEPTIER_NAKOUTES (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIER[10]) NAKed OUT Interrupt Enable */
#define UOTGHS_DEVEPTIER_NAKINES (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIER[10]) NAKed IN Interrupt Enable */
#define UOTGHS_DEVEPTIER_OVERFES (0x1u << 5) /**< \brief (UOTGHS_DEVEPTIER[10]) Overflow Interrupt Enable */
#define UOTGHS_DEVEPTIER_STALLEDES (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIER[10]) STALLed Interrupt Enable */
#define UOTGHS_DEVEPTIER_SHORTPACKETES (0x1u << 7) /**< \brief (UOTGHS_DEVEPTIER[10]) Short Packet Interrupt Enable */
#define UOTGHS_DEVEPTIER_NBUSYBKES (0x1u << 12) /**< \brief (UOTGHS_DEVEPTIER[10]) Number of Busy Banks Interrupt Enable */
#define UOTGHS_DEVEPTIER_KILLBKS (0x1u << 13) /**< \brief (UOTGHS_DEVEPTIER[10]) Kill IN Bank */
#define UOTGHS_DEVEPTIER_FIFOCONS (0x1u << 14) /**< \brief (UOTGHS_DEVEPTIER[10]) FIFO Control */
#define UOTGHS_DEVEPTIER_EPDISHDMAS (0x1u << 16) /**< \brief (UOTGHS_DEVEPTIER[10]) Endpoint Interrupts Disable HDMA Request Enable */
#define UOTGHS_DEVEPTIER_NYETDISS (0x1u << 17) /**< \brief (UOTGHS_DEVEPTIER[10]) NYET Token Disable Enable */
#define UOTGHS_DEVEPTIER_RSTDTS (0x1u << 18) /**< \brief (UOTGHS_DEVEPTIER[10]) Reset Data Toggle Enable */
#define UOTGHS_DEVEPTIER_STALLRQS (0x1u << 19) /**< \brief (UOTGHS_DEVEPTIER[10]) STALL Request Enable */
#define UOTGHS_DEVEPTIER_UNDERFES (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIER[10]) Underflow Interrupt Enable */
#define UOTGHS_DEVEPTIER_HBISOINERRES (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIER[10]) High Bandwidth Isochronous IN Error Interrupt Enable */
#define UOTGHS_DEVEPTIER_HBISOFLUSHES (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIER[10]) High Bandwidth Isochronous IN Flush Interrupt Enable */
#define UOTGHS_DEVEPTIER_CRCERRES (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIER[10]) CRC Error Interrupt Enable */
#define UOTGHS_DEVEPTIER_MDATAES (0x1u << 8) /**< \brief (UOTGHS_DEVEPTIER[10]) MData Interrupt Enable */
#define UOTGHS_DEVEPTIER_DATAXES (0x1u << 9) /**< \brief (UOTGHS_DEVEPTIER[10]) DataX Interrupt Enable */
#define UOTGHS_DEVEPTIER_ERRORTRANSES (0x1u << 10) /**< \brief (UOTGHS_DEVEPTIER[10]) Transaction Error Interrupt Enable */
/* -------- UOTGHS_DEVEPTIDR[10] : (UOTGHS Offset: 0x220) Device Endpoint Disable Register (n = 0) -------- */
#define UOTGHS_DEVEPTIDR_TXINEC (0x1u << 0) /**< \brief (UOTGHS_DEVEPTIDR[10]) Transmitted IN Interrupt Clear */
#define UOTGHS_DEVEPTIDR_RXOUTEC (0x1u << 1) /**< \brief (UOTGHS_DEVEPTIDR[10]) Received OUT Data Interrupt Clear */
#define UOTGHS_DEVEPTIDR_RXSTPEC (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIDR[10]) Received SETUP Interrupt Clear */
#define UOTGHS_DEVEPTIDR_NAKOUTEC (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIDR[10]) NAKed OUT Interrupt Clear */
#define UOTGHS_DEVEPTIDR_NAKINEC (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIDR[10]) NAKed IN Interrupt Clear */
#define UOTGHS_DEVEPTIDR_OVERFEC (0x1u << 5) /**< \brief (UOTGHS_DEVEPTIDR[10]) Overflow Interrupt Clear */
#define UOTGHS_DEVEPTIDR_STALLEDEC (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIDR[10]) STALLed Interrupt Clear */
#define UOTGHS_DEVEPTIDR_SHORTPACKETEC (0x1u << 7) /**< \brief (UOTGHS_DEVEPTIDR[10]) Shortpacket Interrupt Clear */
#define UOTGHS_DEVEPTIDR_NBUSYBKEC (0x1u << 12) /**< \brief (UOTGHS_DEVEPTIDR[10]) Number of Busy Banks Interrupt Clear */
#define UOTGHS_DEVEPTIDR_FIFOCONC (0x1u << 14) /**< \brief (UOTGHS_DEVEPTIDR[10]) FIFO Control Clear */
#define UOTGHS_DEVEPTIDR_EPDISHDMAC (0x1u << 16) /**< \brief (UOTGHS_DEVEPTIDR[10]) Endpoint Interrupts Disable HDMA Request Clear */
#define UOTGHS_DEVEPTIDR_NYETDISC (0x1u << 17) /**< \brief (UOTGHS_DEVEPTIDR[10]) NYET Token Disable Clear */
#define UOTGHS_DEVEPTIDR_STALLRQC (0x1u << 19) /**< \brief (UOTGHS_DEVEPTIDR[10]) STALL Request Clear */
#define UOTGHS_DEVEPTIDR_UNDERFEC (0x1u << 2) /**< \brief (UOTGHS_DEVEPTIDR[10]) Underflow Interrupt Clear */
#define UOTGHS_DEVEPTIDR_HBISOINERREC (0x1u << 3) /**< \brief (UOTGHS_DEVEPTIDR[10]) High Bandwidth Isochronous IN Error Interrupt Clear */
#define UOTGHS_DEVEPTIDR_HBISOFLUSHEC (0x1u << 4) /**< \brief (UOTGHS_DEVEPTIDR[10]) High Bandwidth Isochronous IN Flush Interrupt Clear */
#define UOTGHS_DEVEPTIDR_CRCERREC (0x1u << 6) /**< \brief (UOTGHS_DEVEPTIDR[10]) CRC Error Interrupt Clear */
#define UOTGHS_DEVEPTIDR_MDATEC (0x1u << 8) /**< \brief (UOTGHS_DEVEPTIDR[10]) MData Interrupt Clear */
#define UOTGHS_DEVEPTIDR_DATAXEC (0x1u << 9) /**< \brief (UOTGHS_DEVEPTIDR[10]) DataX Interrupt Clear */
#define UOTGHS_DEVEPTIDR_ERRORTRANSEC (0x1u << 10) /**< \brief (UOTGHS_DEVEPTIDR[10]) Transaction Error Interrupt Clear */
/* -------- UOTGHS_DEVDMANXTDSC : (UOTGHS Offset: N/A) Device DMA Channel Next Descriptor Address Register -------- */
#define UOTGHS_DEVDMANXTDSC_NXT_DSC_ADD_Pos 0
#define UOTGHS_DEVDMANXTDSC_NXT_DSC_ADD_Msk (0xffffffffu << UOTGHS_DEVDMANXTDSC_NXT_DSC_ADD_Pos) /**< \brief (UOTGHS_DEVDMANXTDSC) Next Descriptor Address */
#define UOTGHS_DEVDMANXTDSC_NXT_DSC_ADD(value) ((UOTGHS_DEVDMANXTDSC_NXT_DSC_ADD_Msk & ((value) << UOTGHS_DEVDMANXTDSC_NXT_DSC_ADD_Pos)))
/* -------- UOTGHS_DEVDMAADDRESS : (UOTGHS Offset: N/A) Device DMA Channel Address Register -------- */
#define UOTGHS_DEVDMAADDRESS_BUFF_ADD_Pos 0
#define UOTGHS_DEVDMAADDRESS_BUFF_ADD_Msk (0xffffffffu << UOTGHS_DEVDMAADDRESS_BUFF_ADD_Pos) /**< \brief (UOTGHS_DEVDMAADDRESS) Buffer Address */
#define UOTGHS_DEVDMAADDRESS_BUFF_ADD(value) ((UOTGHS_DEVDMAADDRESS_BUFF_ADD_Msk & ((value) << UOTGHS_DEVDMAADDRESS_BUFF_ADD_Pos)))
/* -------- UOTGHS_DEVDMACONTROL : (UOTGHS Offset: N/A) Device DMA Channel Control Register -------- */
#define UOTGHS_DEVDMACONTROL_CHANN_ENB (0x1u << 0) /**< \brief (UOTGHS_DEVDMACONTROL) Channel Enable Command */
#define UOTGHS_DEVDMACONTROL_LDNXT_DSC (0x1u << 1) /**< \brief (UOTGHS_DEVDMACONTROL) Load Next Channel Transfer Descriptor Enable Command */
#define UOTGHS_DEVDMACONTROL_END_TR_EN (0x1u << 2) /**< \brief (UOTGHS_DEVDMACONTROL) End of Transfer Enable Control */
#define UOTGHS_DEVDMACONTROL_END_B_EN (0x1u << 3) /**< \brief (UOTGHS_DEVDMACONTROL) End of Buffer Enable Control */
#define UOTGHS_DEVDMACONTROL_END_TR_IT (0x1u << 4) /**< \brief (UOTGHS_DEVDMACONTROL) End of Transfer Interrupt Enable */
#define UOTGHS_DEVDMACONTROL_END_BUFFIT (0x1u << 5) /**< \brief (UOTGHS_DEVDMACONTROL) End of Buffer Interrupt Enable */
#define UOTGHS_DEVDMACONTROL_DESC_LD_IT (0x1u << 6) /**< \brief (UOTGHS_DEVDMACONTROL) Descriptor Loaded Interrupt Enable */
#define UOTGHS_DEVDMACONTROL_BURST_LCK (0x1u << 7) /**< \brief (UOTGHS_DEVDMACONTROL) Burst Lock Enable */
#define UOTGHS_DEVDMACONTROL_BUFF_LENGTH_Pos 16
#define UOTGHS_DEVDMACONTROL_BUFF_LENGTH_Msk (0xffffu << UOTGHS_DEVDMACONTROL_BUFF_LENGTH_Pos) /**< \brief (UOTGHS_DEVDMACONTROL) Buffer Byte Length (Write-only) */
#define UOTGHS_DEVDMACONTROL_BUFF_LENGTH(value) ((UOTGHS_DEVDMACONTROL_BUFF_LENGTH_Msk & ((value) << UOTGHS_DEVDMACONTROL_BUFF_LENGTH_Pos)))
/* -------- UOTGHS_DEVDMASTATUS : (UOTGHS Offset: N/A) Device DMA Channel Status Register -------- */
#define UOTGHS_DEVDMASTATUS_CHANN_ENB (0x1u << 0) /**< \brief (UOTGHS_DEVDMASTATUS) Channel Enable Status */
#define UOTGHS_DEVDMASTATUS_CHANN_ACT (0x1u << 1) /**< \brief (UOTGHS_DEVDMASTATUS) Channel Active Status */
#define UOTGHS_DEVDMASTATUS_END_TR_ST (0x1u << 4) /**< \brief (UOTGHS_DEVDMASTATUS) End of Channel Transfer Status */
#define UOTGHS_DEVDMASTATUS_END_BF_ST (0x1u << 5) /**< \brief (UOTGHS_DEVDMASTATUS) End of Channel Buffer Status */
#define UOTGHS_DEVDMASTATUS_DESC_LDST (0x1u << 6) /**< \brief (UOTGHS_DEVDMASTATUS) Descriptor Loaded Status */
#define UOTGHS_DEVDMASTATUS_BUFF_COUNT_Pos 16
#define UOTGHS_DEVDMASTATUS_BUFF_COUNT_Msk (0xffffu << UOTGHS_DEVDMASTATUS_BUFF_COUNT_Pos) /**< \brief (UOTGHS_DEVDMASTATUS) Buffer Byte Count */
#define UOTGHS_DEVDMASTATUS_BUFF_COUNT(value) ((UOTGHS_DEVDMASTATUS_BUFF_COUNT_Msk & ((value) << UOTGHS_DEVDMASTATUS_BUFF_COUNT_Pos)))
/* -------- UOTGHS_HSTCTRL : (UOTGHS Offset: 0x0400) Host General Control Register -------- */
#define UOTGHS_HSTCTRL_SOFE (0x1u << 8) /**< \brief (UOTGHS_HSTCTRL) Start of Frame Generation Enable */
#define UOTGHS_HSTCTRL_RESET (0x1u << 9) /**< \brief (UOTGHS_HSTCTRL) Send USB Reset */
#define UOTGHS_HSTCTRL_RESUME (0x1u << 10) /**< \brief (UOTGHS_HSTCTRL) Send USB Resume */
#define UOTGHS_HSTCTRL_SPDCONF_Pos 12
#define UOTGHS_HSTCTRL_SPDCONF_Msk (0x3u << UOTGHS_HSTCTRL_SPDCONF_Pos) /**< \brief (UOTGHS_HSTCTRL) Mode Configuration */
#define   UOTGHS_HSTCTRL_SPDCONF_NORMAL (0x0u << 12) /**< \brief (UOTGHS_HSTCTRL) The host starts in full-speed mode and performs a high-speed reset to switch to the high-speed mode if the downstream peripheral is high-speed capable. */
#define   UOTGHS_HSTCTRL_SPDCONF_LOW_POWER (0x1u << 12) /**< \brief (UOTGHS_HSTCTRL) For a better consumption, if high-speed is not needed. */
#define   UOTGHS_HSTCTRL_SPDCONF_HIGH_SPEED (0x2u << 12) /**< \brief (UOTGHS_HSTCTRL) Forced high speed. */
#define   UOTGHS_HSTCTRL_SPDCONF_FORCED_FS (0x3u << 12) /**< \brief (UOTGHS_HSTCTRL) The host remains to full-speed mode whatever the peripheral speed capability. */
/* -------- UOTGHS_HSTISR : (UOTGHS Offset: 0x0404) Host Global Interrupt Status Register -------- */
#define UOTGHS_HSTISR_DCONNI (0x1u << 0) /**< \brief (UOTGHS_HSTISR) Device Connection Interrupt */
#define UOTGHS_HSTISR_DDISCI (0x1u << 1) /**< \brief (UOTGHS_HSTISR) Device Disconnection Interrupt */
#define UOTGHS_HSTISR_RSTI (0x1u << 2) /**< \brief (UOTGHS_HSTISR) USB Reset Sent Interrupt */
#define UOTGHS_HSTISR_RSMEDI (0x1u << 3) /**< \brief (UOTGHS_HSTISR) Downstream Resume Sent Interrupt */
#define UOTGHS_HSTISR_RXRSMI (0x1u << 4) /**< \brief (UOTGHS_HSTISR) Upstream Resume Received Interrupt */
#define UOTGHS_HSTISR_HSOFI (0x1u << 5) /**< \brief (UOTGHS_HSTISR) Host Start of Frame Interrupt */
#define UOTGHS_HSTISR_HWUPI (0x1u << 6) /**< \brief (UOTGHS_HSTISR) Host Wake-Up Interrupt */
#define UOTGHS_HSTISR_PEP_0 (0x1u << 8) /**< \brief (UOTGHS_HSTISR) Pipe 0 Interrupt */
#define UOTGHS_HSTISR_PEP_1 (0x1u << 9) /**< \brief (UOTGHS_HSTISR) Pipe 1 Interrupt */
#define UOTGHS_HSTISR_PEP_2 (0x1u << 10) /**< \brief (UOTGHS_HSTISR) Pipe 2 Interrupt */
#define UOTGHS_HSTISR_PEP_3 (0x1u << 11) /**< \brief (UOTGHS_HSTISR) Pipe 3 Interrupt */
#define UOTGHS_HSTISR_PEP_4 (0x1u << 12) /**< \brief (UOTGHS_HSTISR) Pipe 4 Interrupt */
#define UOTGHS_HSTISR_PEP_5 (0x1u << 13) /**< \brief (UOTGHS_HSTISR) Pipe 5 Interrupt */
#define UOTGHS_HSTISR_PEP_6 (0x1u << 14) /**< \brief (UOTGHS_HSTISR) Pipe 6 Interrupt */
#define UOTGHS_HSTISR_PEP_7 (0x1u << 15) /**< \brief (UOTGHS_HSTISR) Pipe 7 Interrupt */
#define UOTGHS_HSTISR_PEP_8 (0x1u << 16) /**< \brief (UOTGHS_HSTISR) Pipe 8 Interrupt */
#define UOTGHS_HSTISR_PEP_9 (0x1u << 17) /**< \brief (UOTGHS_HSTISR) Pipe 9 Interrupt */
#define UOTGHS_HSTISR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_HSTISR) DMA Channel 1 Interrupt */
#define UOTGHS_HSTISR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_HSTISR) DMA Channel 2 Interrupt */
#define UOTGHS_HSTISR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_HSTISR) DMA Channel 3 Interrupt */
#define UOTGHS_HSTISR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_HSTISR) DMA Channel 4 Interrupt */
#define UOTGHS_HSTISR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_HSTISR) DMA Channel 5 Interrupt */
#define UOTGHS_HSTISR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_HSTISR) DMA Channel 6 Interrupt */
/* -------- UOTGHS_HSTICR : (UOTGHS Offset: 0x0408) Host Global Interrupt Clear Register -------- */
#define UOTGHS_HSTICR_DCONNIC (0x1u << 0) /**< \brief (UOTGHS_HSTICR) Device Connection Interrupt Clear */
#define UOTGHS_HSTICR_DDISCIC (0x1u << 1) /**< \brief (UOTGHS_HSTICR) Device Disconnection Interrupt Clear */
#define UOTGHS_HSTICR_RSTIC (0x1u << 2) /**< \brief (UOTGHS_HSTICR) USB Reset Sent Interrupt Clear */
#define UOTGHS_HSTICR_RSMEDIC (0x1u << 3) /**< \brief (UOTGHS_HSTICR) Downstream Resume Sent Interrupt Clear */
#define UOTGHS_HSTICR_RXRSMIC (0x1u << 4) /**< \brief (UOTGHS_HSTICR) Upstream Resume Received Interrupt Clear */
#define UOTGHS_HSTICR_HSOFIC (0x1u << 5) /**< \brief (UOTGHS_HSTICR) Host Start of Frame Interrupt Clear */
#define UOTGHS_HSTICR_HWUPIC (0x1u << 6) /**< \brief (UOTGHS_HSTICR) Host Wake-Up Interrupt Clear */
/* -------- UOTGHS_HSTIFR : (UOTGHS Offset: 0x040C) Host Global Interrupt Set Register -------- */
#define UOTGHS_HSTIFR_DCONNIS (0x1u << 0) /**< \brief (UOTGHS_HSTIFR) Device Connection Interrupt Set */
#define UOTGHS_HSTIFR_DDISCIS (0x1u << 1) /**< \brief (UOTGHS_HSTIFR) Device Disconnection Interrupt Set */
#define UOTGHS_HSTIFR_RSTIS (0x1u << 2) /**< \brief (UOTGHS_HSTIFR) USB Reset Sent Interrupt Set */
#define UOTGHS_HSTIFR_RSMEDIS (0x1u << 3) /**< \brief (UOTGHS_HSTIFR) Downstream Resume Sent Interrupt Set */
#define UOTGHS_HSTIFR_RXRSMIS (0x1u << 4) /**< \brief (UOTGHS_HSTIFR) Upstream Resume Received Interrupt Set */
#define UOTGHS_HSTIFR_HSOFIS (0x1u << 5) /**< \brief (UOTGHS_HSTIFR) Host Start of Frame Interrupt Set */
#define UOTGHS_HSTIFR_HWUPIS (0x1u << 6) /**< \brief (UOTGHS_HSTIFR) Host Wake-Up Interrupt Set */
#define UOTGHS_HSTIFR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_HSTIFR) DMA Channel 1 Interrupt Set */
#define UOTGHS_HSTIFR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_HSTIFR) DMA Channel 2 Interrupt Set */
#define UOTGHS_HSTIFR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_HSTIFR) DMA Channel 3 Interrupt Set */
#define UOTGHS_HSTIFR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_HSTIFR) DMA Channel 4 Interrupt Set */
#define UOTGHS_HSTIFR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_HSTIFR) DMA Channel 5 Interrupt Set */
#define UOTGHS_HSTIFR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_HSTIFR) DMA Channel 6 Interrupt Set */
/* -------- UOTGHS_HSTIMR : (UOTGHS Offset: 0x0410) Host Global Interrupt Mask Register -------- */
#define UOTGHS_HSTIMR_DCONNIE (0x1u << 0) /**< \brief (UOTGHS_HSTIMR) Device Connection Interrupt Enable */
#define UOTGHS_HSTIMR_DDISCIE (0x1u << 1) /**< \brief (UOTGHS_HSTIMR) Device Disconnection Interrupt Enable */
#define UOTGHS_HSTIMR_RSTIE (0x1u << 2) /**< \brief (UOTGHS_HSTIMR) USB Reset Sent Interrupt Enable */
#define UOTGHS_HSTIMR_RSMEDIE (0x1u << 3) /**< \brief (UOTGHS_HSTIMR) Downstream Resume Sent Interrupt Enable */
#define UOTGHS_HSTIMR_RXRSMIE (0x1u << 4) /**< \brief (UOTGHS_HSTIMR) Upstream Resume Received Interrupt Enable */
#define UOTGHS_HSTIMR_HSOFIE (0x1u << 5) /**< \brief (UOTGHS_HSTIMR) Host Start of Frame Interrupt Enable */
#define UOTGHS_HSTIMR_HWUPIE (0x1u << 6) /**< \brief (UOTGHS_HSTIMR) Host Wake-Up Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_0 (0x1u << 8) /**< \brief (UOTGHS_HSTIMR) Pipe 0 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_1 (0x1u << 9) /**< \brief (UOTGHS_HSTIMR) Pipe 1 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_2 (0x1u << 10) /**< \brief (UOTGHS_HSTIMR) Pipe 2 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_3 (0x1u << 11) /**< \brief (UOTGHS_HSTIMR) Pipe 3 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_4 (0x1u << 12) /**< \brief (UOTGHS_HSTIMR) Pipe 4 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_5 (0x1u << 13) /**< \brief (UOTGHS_HSTIMR) Pipe 5 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_6 (0x1u << 14) /**< \brief (UOTGHS_HSTIMR) Pipe 6 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_7 (0x1u << 15) /**< \brief (UOTGHS_HSTIMR) Pipe 7 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_8 (0x1u << 16) /**< \brief (UOTGHS_HSTIMR) Pipe 8 Interrupt Enable */
#define UOTGHS_HSTIMR_PEP_9 (0x1u << 17) /**< \brief (UOTGHS_HSTIMR) Pipe 9 Interrupt Enable */
#define UOTGHS_HSTIMR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_HSTIMR) DMA Channel 1 Interrupt Enable */
#define UOTGHS_HSTIMR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_HSTIMR) DMA Channel 2 Interrupt Enable */
#define UOTGHS_HSTIMR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_HSTIMR) DMA Channel 3 Interrupt Enable */
#define UOTGHS_HSTIMR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_HSTIMR) DMA Channel 4 Interrupt Enable */
#define UOTGHS_HSTIMR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_HSTIMR) DMA Channel 5 Interrupt Enable */
#define UOTGHS_HSTIMR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_HSTIMR) DMA Channel 6 Interrupt Enable */
/* -------- UOTGHS_HSTIDR : (UOTGHS Offset: 0x0414) Host Global Interrupt Disable Register -------- */
#define UOTGHS_HSTIDR_DCONNIEC (0x1u << 0) /**< \brief (UOTGHS_HSTIDR) Device Connection Interrupt Disable */
#define UOTGHS_HSTIDR_DDISCIEC (0x1u << 1) /**< \brief (UOTGHS_HSTIDR) Device Disconnection Interrupt Disable */
#define UOTGHS_HSTIDR_RSTIEC (0x1u << 2) /**< \brief (UOTGHS_HSTIDR) USB Reset Sent Interrupt Disable */
#define UOTGHS_HSTIDR_RSMEDIEC (0x1u << 3) /**< \brief (UOTGHS_HSTIDR) Downstream Resume Sent Interrupt Disable */
#define UOTGHS_HSTIDR_RXRSMIEC (0x1u << 4) /**< \brief (UOTGHS_HSTIDR) Upstream Resume Received Interrupt Disable */
#define UOTGHS_HSTIDR_HSOFIEC (0x1u << 5) /**< \brief (UOTGHS_HSTIDR) Host Start of Frame Interrupt Disable */
#define UOTGHS_HSTIDR_HWUPIEC (0x1u << 6) /**< \brief (UOTGHS_HSTIDR) Host Wake-Up Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_0 (0x1u << 8) /**< \brief (UOTGHS_HSTIDR) Pipe 0 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_1 (0x1u << 9) /**< \brief (UOTGHS_HSTIDR) Pipe 1 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_2 (0x1u << 10) /**< \brief (UOTGHS_HSTIDR) Pipe 2 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_3 (0x1u << 11) /**< \brief (UOTGHS_HSTIDR) Pipe 3 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_4 (0x1u << 12) /**< \brief (UOTGHS_HSTIDR) Pipe 4 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_5 (0x1u << 13) /**< \brief (UOTGHS_HSTIDR) Pipe 5 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_6 (0x1u << 14) /**< \brief (UOTGHS_HSTIDR) Pipe 6 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_7 (0x1u << 15) /**< \brief (UOTGHS_HSTIDR) Pipe 7 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_8 (0x1u << 16) /**< \brief (UOTGHS_HSTIDR) Pipe 8 Interrupt Disable */
#define UOTGHS_HSTIDR_PEP_9 (0x1u << 17) /**< \brief (UOTGHS_HSTIDR) Pipe 9 Interrupt Disable */
#define UOTGHS_HSTIDR_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_HSTIDR) DMA Channel 1 Interrupt Disable */
#define UOTGHS_HSTIDR_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_HSTIDR) DMA Channel 2 Interrupt Disable */
#define UOTGHS_HSTIDR_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_HSTIDR) DMA Channel 3 Interrupt Disable */
#define UOTGHS_HSTIDR_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_HSTIDR) DMA Channel 4 Interrupt Disable */
#define UOTGHS_HSTIDR_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_HSTIDR) DMA Channel 5 Interrupt Disable */
#define UOTGHS_HSTIDR_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_HSTIDR) DMA Channel 6 Interrupt Disable */
/* -------- UOTGHS_HSTIER : (UOTGHS Offset: 0x0418) Host Global Interrupt Enable Register -------- */
#define UOTGHS_HSTIER_DCONNIES (0x1u << 0) /**< \brief (UOTGHS_HSTIER) Device Connection Interrupt Enable */
#define UOTGHS_HSTIER_DDISCIES (0x1u << 1) /**< \brief (UOTGHS_HSTIER) Device Disconnection Interrupt Enable */
#define UOTGHS_HSTIER_RSTIES (0x1u << 2) /**< \brief (UOTGHS_HSTIER) USB Reset Sent Interrupt Enable */
#define UOTGHS_HSTIER_RSMEDIES (0x1u << 3) /**< \brief (UOTGHS_HSTIER) Downstream Resume Sent Interrupt Enable */
#define UOTGHS_HSTIER_RXRSMIES (0x1u << 4) /**< \brief (UOTGHS_HSTIER) Upstream Resume Received Interrupt Enable */
#define UOTGHS_HSTIER_HSOFIES (0x1u << 5) /**< \brief (UOTGHS_HSTIER) Host Start of Frame Interrupt Enable */
#define UOTGHS_HSTIER_HWUPIES (0x1u << 6) /**< \brief (UOTGHS_HSTIER) Host Wake-Up Interrupt Enable */
#define UOTGHS_HSTIER_PEP_0 (0x1u << 8) /**< \brief (UOTGHS_HSTIER) Pipe 0 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_1 (0x1u << 9) /**< \brief (UOTGHS_HSTIER) Pipe 1 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_2 (0x1u << 10) /**< \brief (UOTGHS_HSTIER) Pipe 2 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_3 (0x1u << 11) /**< \brief (UOTGHS_HSTIER) Pipe 3 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_4 (0x1u << 12) /**< \brief (UOTGHS_HSTIER) Pipe 4 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_5 (0x1u << 13) /**< \brief (UOTGHS_HSTIER) Pipe 5 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_6 (0x1u << 14) /**< \brief (UOTGHS_HSTIER) Pipe 6 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_7 (0x1u << 15) /**< \brief (UOTGHS_HSTIER) Pipe 7 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_8 (0x1u << 16) /**< \brief (UOTGHS_HSTIER) Pipe 8 Interrupt Enable */
#define UOTGHS_HSTIER_PEP_9 (0x1u << 17) /**< \brief (UOTGHS_HSTIER) Pipe 9 Interrupt Enable */
#define UOTGHS_HSTIER_DMA_1 (0x1u << 25) /**< \brief (UOTGHS_HSTIER) DMA Channel 1 Interrupt Enable */
#define UOTGHS_HSTIER_DMA_2 (0x1u << 26) /**< \brief (UOTGHS_HSTIER) DMA Channel 2 Interrupt Enable */
#define UOTGHS_HSTIER_DMA_3 (0x1u << 27) /**< \brief (UOTGHS_HSTIER) DMA Channel 3 Interrupt Enable */
#define UOTGHS_HSTIER_DMA_4 (0x1u << 28) /**< \brief (UOTGHS_HSTIER) DMA Channel 4 Interrupt Enable */
#define UOTGHS_HSTIER_DMA_5 (0x1u << 29) /**< \brief (UOTGHS_HSTIER) DMA Channel 5 Interrupt Enable */
#define UOTGHS_HSTIER_DMA_6 (0x1u << 30) /**< \brief (UOTGHS_HSTIER) DMA Channel 6 Interrupt Enable */
/* -------- UOTGHS_HSTPIP : (UOTGHS Offset: 0x0041C) Host Pipe Register -------- */
#define UOTGHS_HSTPIP_PEN0 (0x1u << 0) /**< \brief (UOTGHS_HSTPIP) Pipe 0 Enable */
#define UOTGHS_HSTPIP_PEN1 (0x1u << 1) /**< \brief (UOTGHS_HSTPIP) Pipe 1 Enable */
#define UOTGHS_HSTPIP_PEN2 (0x1u << 2) /**< \brief (UOTGHS_HSTPIP) Pipe 2 Enable */
#define UOTGHS_HSTPIP_PEN3 (0x1u << 3) /**< \brief (UOTGHS_HSTPIP) Pipe 3 Enable */
#define UOTGHS_HSTPIP_PEN4 (0x1u << 4) /**< \brief (UOTGHS_HSTPIP) Pipe 4 Enable */
#define UOTGHS_HSTPIP_PEN5 (0x1u << 5) /**< \brief (UOTGHS_HSTPIP) Pipe 5 Enable */
#define UOTGHS_HSTPIP_PEN6 (0x1u << 6) /**< \brief (UOTGHS_HSTPIP) Pipe 6 Enable */
#define UOTGHS_HSTPIP_PEN7 (0x1u << 7) /**< \brief (UOTGHS_HSTPIP) Pipe 7 Enable */
#define UOTGHS_HSTPIP_PEN8 (0x1u << 8) /**< \brief (UOTGHS_HSTPIP) Pipe 8 Enable */
#define UOTGHS_HSTPIP_PRST0 (0x1u << 16) /**< \brief (UOTGHS_HSTPIP) Pipe 0 Reset */
#define UOTGHS_HSTPIP_PRST1 (0x1u << 17) /**< \brief (UOTGHS_HSTPIP) Pipe 1 Reset */
#define UOTGHS_HSTPIP_PRST2 (0x1u << 18) /**< \brief (UOTGHS_HSTPIP) Pipe 2 Reset */
#define UOTGHS_HSTPIP_PRST3 (0x1u << 19) /**< \brief (UOTGHS_HSTPIP) Pipe 3 Reset */
#define UOTGHS_HSTPIP_PRST4 (0x1u << 20) /**< \brief (UOTGHS_HSTPIP) Pipe 4 Reset */
#define UOTGHS_HSTPIP_PRST5 (0x1u << 21) /**< \brief (UOTGHS_HSTPIP) Pipe 5 Reset */
#define UOTGHS_HSTPIP_PRST6 (0x1u << 22) /**< \brief (UOTGHS_HSTPIP) Pipe 6 Reset */
#define UOTGHS_HSTPIP_PRST7 (0x1u << 23) /**< \brief (UOTGHS_HSTPIP) Pipe 7 Reset */
#define UOTGHS_HSTPIP_PRST8 (0x1u << 24) /**< \brief (UOTGHS_HSTPIP) Pipe 8 Reset */
/* -------- UOTGHS_HSTFNUM : (UOTGHS Offset: 0x0420) Host Frame Number Register -------- */
#define UOTGHS_HSTFNUM_MFNUM_Pos 0
#define UOTGHS_HSTFNUM_MFNUM_Msk (0x7u << UOTGHS_HSTFNUM_MFNUM_Pos) /**< \brief (UOTGHS_HSTFNUM) Micro Frame Number */
#define UOTGHS_HSTFNUM_MFNUM(value) ((UOTGHS_HSTFNUM_MFNUM_Msk & ((value) << UOTGHS_HSTFNUM_MFNUM_Pos)))
#define UOTGHS_HSTFNUM_FNUM_Pos 3
#define UOTGHS_HSTFNUM_FNUM_Msk (0x7ffu << UOTGHS_HSTFNUM_FNUM_Pos) /**< \brief (UOTGHS_HSTFNUM) Frame Number */
#define UOTGHS_HSTFNUM_FNUM(value) ((UOTGHS_HSTFNUM_FNUM_Msk & ((value) << UOTGHS_HSTFNUM_FNUM_Pos)))
#define UOTGHS_HSTFNUM_FLENHIGH_Pos 16
#define UOTGHS_HSTFNUM_FLENHIGH_Msk (0xffu << UOTGHS_HSTFNUM_FLENHIGH_Pos) /**< \brief (UOTGHS_HSTFNUM) Frame Length */
#define UOTGHS_HSTFNUM_FLENHIGH(value) ((UOTGHS_HSTFNUM_FLENHIGH_Msk & ((value) << UOTGHS_HSTFNUM_FLENHIGH_Pos)))
/* -------- UOTGHS_HSTADDR1 : (UOTGHS Offset: 0x0424) Host Address 1 Register -------- */
#define UOTGHS_HSTADDR1_HSTADDRP0_Pos 0
#define UOTGHS_HSTADDR1_HSTADDRP0_Msk (0x7fu << UOTGHS_HSTADDR1_HSTADDRP0_Pos) /**< \brief (UOTGHS_HSTADDR1) USB Host Address */
#define UOTGHS_HSTADDR1_HSTADDRP0(value) ((UOTGHS_HSTADDR1_HSTADDRP0_Msk & ((value) << UOTGHS_HSTADDR1_HSTADDRP0_Pos)))
#define UOTGHS_HSTADDR1_HSTADDRP1_Pos 8
#define UOTGHS_HSTADDR1_HSTADDRP1_Msk (0x7fu << UOTGHS_HSTADDR1_HSTADDRP1_Pos) /**< \brief (UOTGHS_HSTADDR1) USB Host Address */
#define UOTGHS_HSTADDR1_HSTADDRP1(value) ((UOTGHS_HSTADDR1_HSTADDRP1_Msk & ((value) << UOTGHS_HSTADDR1_HSTADDRP1_Pos)))
#define UOTGHS_HSTADDR1_HSTADDRP2_Pos 16
#define UOTGHS_HSTADDR1_HSTADDRP2_Msk (0x7fu << UOTGHS_HSTADDR1_HSTADDRP2_Pos) /**< \brief (UOTGHS_HSTADDR1) USB Host Address */
#define UOTGHS_HSTADDR1_HSTADDRP2(value) ((UOTGHS_HSTADDR1_HSTADDRP2_Msk & ((value) << UOTGHS_HSTADDR1_HSTADDRP2_Pos)))
#define UOTGHS_HSTADDR1_HSTADDRP3_Pos 24
#define UOTGHS_HSTADDR1_HSTADDRP3_Msk (0x7fu << UOTGHS_HSTADDR1_HSTADDRP3_Pos) /**< \brief (UOTGHS_HSTADDR1) USB Host Address */
#define UOTGHS_HSTADDR1_HSTADDRP3(value) ((UOTGHS_HSTADDR1_HSTADDRP3_Msk & ((value) << UOTGHS_HSTADDR1_HSTADDRP3_Pos)))
/* -------- UOTGHS_HSTADDR2 : (UOTGHS Offset: 0x0428) Host Address 2 Register -------- */
#define UOTGHS_HSTADDR2_HSTADDRP4_Pos 0
#define UOTGHS_HSTADDR2_HSTADDRP4_Msk (0x7fu << UOTGHS_HSTADDR2_HSTADDRP4_Pos) /**< \brief (UOTGHS_HSTADDR2) USB Host Address */
#define UOTGHS_HSTADDR2_HSTADDRP4(value) ((UOTGHS_HSTADDR2_HSTADDRP4_Msk & ((value) << UOTGHS_HSTADDR2_HSTADDRP4_Pos)))
#define UOTGHS_HSTADDR2_HSTADDRP5_Pos 8
#define UOTGHS_HSTADDR2_HSTADDRP5_Msk (0x7fu << UOTGHS_HSTADDR2_HSTADDRP5_Pos) /**< \brief (UOTGHS_HSTADDR2) USB Host Address */
#define UOTGHS_HSTADDR2_HSTADDRP5(value) ((UOTGHS_HSTADDR2_HSTADDRP5_Msk & ((value) << UOTGHS_HSTADDR2_HSTADDRP5_Pos)))
#define UOTGHS_HSTADDR2_HSTADDRP6_Pos 16
#define UOTGHS_HSTADDR2_HSTADDRP6_Msk (0x7fu << UOTGHS_HSTADDR2_HSTADDRP6_Pos) /**< \brief (UOTGHS_HSTADDR2) USB Host Address */
#define UOTGHS_HSTADDR2_HSTADDRP6(value) ((UOTGHS_HSTADDR2_HSTADDRP6_Msk & ((value) << UOTGHS_HSTADDR2_HSTADDRP6_Pos)))
#define UOTGHS_HSTADDR2_HSTADDRP7_Pos 24
#define UOTGHS_HSTADDR2_HSTADDRP7_Msk (0x7fu << UOTGHS_HSTADDR2_HSTADDRP7_Pos) /**< \brief (UOTGHS_HSTADDR2) USB Host Address */
#define UOTGHS_HSTADDR2_HSTADDRP7(value) ((UOTGHS_HSTADDR2_HSTADDRP7_Msk & ((value) << UOTGHS_HSTADDR2_HSTADDRP7_Pos)))
/* -------- UOTGHS_HSTADDR3 : (UOTGHS Offset: 0x042C) Host Address 3 Register -------- */
#define UOTGHS_HSTADDR3_HSTADDRP8_Pos 0
#define UOTGHS_HSTADDR3_HSTADDRP8_Msk (0x7fu << UOTGHS_HSTADDR3_HSTADDRP8_Pos) /**< \brief (UOTGHS_HSTADDR3) USB Host Address */
#define UOTGHS_HSTADDR3_HSTADDRP8(value) ((UOTGHS_HSTADDR3_HSTADDRP8_Msk & ((value) << UOTGHS_HSTADDR3_HSTADDRP8_Pos)))
#define UOTGHS_HSTADDR3_HSTADDRP9_Pos 8
#define UOTGHS_HSTADDR3_HSTADDRP9_Msk (0x7fu << UOTGHS_HSTADDR3_HSTADDRP9_Pos) /**< \brief (UOTGHS_HSTADDR3) USB Host Address */
#define UOTGHS_HSTADDR3_HSTADDRP9(value) ((UOTGHS_HSTADDR3_HSTADDRP9_Msk & ((value) << UOTGHS_HSTADDR3_HSTADDRP9_Pos)))
/* -------- UOTGHS_HSTPIPCFG[10] : (UOTGHS Offset: 0x500) Host Pipe Configuration Register (n = 0) -------- */
#define UOTGHS_HSTPIPCFG_ALLOC (0x1u << 1) /**< \brief (UOTGHS_HSTPIPCFG[10]) Pipe Memory Allocate */
#define UOTGHS_HSTPIPCFG_PBK_Pos 2
#define UOTGHS_HSTPIPCFG_PBK_Msk (0x3u << UOTGHS_HSTPIPCFG_PBK_Pos) /**< \brief (UOTGHS_HSTPIPCFG[10]) Pipe Banks */
#define   UOTGHS_HSTPIPCFG_PBK_1_BANK (0x0u << 2) /**< \brief (UOTGHS_HSTPIPCFG[10]) Single-bank pipe */
#define   UOTGHS_HSTPIPCFG_PBK_2_BANK (0x1u << 2) /**< \brief (UOTGHS_HSTPIPCFG[10]) Double-bank pipe */
#define   UOTGHS_HSTPIPCFG_PBK_3_BANK (0x2u << 2) /**< \brief (UOTGHS_HSTPIPCFG[10]) Triple-bank pipe */
#define UOTGHS_HSTPIPCFG_PSIZE_Pos 4
#define UOTGHS_HSTPIPCFG_PSIZE_Msk (0x7u << UOTGHS_HSTPIPCFG_PSIZE_Pos) /**< \brief (UOTGHS_HSTPIPCFG[10]) Pipe Size */
#define   UOTGHS_HSTPIPCFG_PSIZE_8_BYTE (0x0u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 8 bytes */
#define   UOTGHS_HSTPIPCFG_PSIZE_16_BYTE (0x1u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 16 bytes */
#define   UOTGHS_HSTPIPCFG_PSIZE_32_BYTE (0x2u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 32 bytes */
#define   UOTGHS_HSTPIPCFG_PSIZE_64_BYTE (0x3u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 64 bytes */
#define   UOTGHS_HSTPIPCFG_PSIZE_128_BYTE (0x4u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 128 bytes */
#define   UOTGHS_HSTPIPCFG_PSIZE_256_BYTE (0x5u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 256 bytes */
#define   UOTGHS_HSTPIPCFG_PSIZE_512_BYTE (0x6u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 512 bytes */
#define   UOTGHS_HSTPIPCFG_PSIZE_1024_BYTE (0x7u << 4) /**< \brief (UOTGHS_HSTPIPCFG[10]) 1024 bytes */
#define UOTGHS_HSTPIPCFG_PTOKEN_Pos 8
#define UOTGHS_HSTPIPCFG_PTOKEN_Msk (0x3u << UOTGHS_HSTPIPCFG_PTOKEN_Pos) /**< \brief (UOTGHS_HSTPIPCFG[10]) Pipe Token */
#define   UOTGHS_HSTPIPCFG_PTOKEN_SETUP (0x0u << 8) /**< \brief (UOTGHS_HSTPIPCFG[10]) SETUP */
#define   UOTGHS_HSTPIPCFG_PTOKEN_IN (0x1u << 8) /**< \brief (UOTGHS_HSTPIPCFG[10]) IN */
#define   UOTGHS_HSTPIPCFG_PTOKEN_OUT (0x2u << 8) /**< \brief (UOTGHS_HSTPIPCFG[10]) OUT */
#define UOTGHS_HSTPIPCFG_AUTOSW (0x1u << 10) /**< \brief (UOTGHS_HSTPIPCFG[10]) Automatic Switch */
#define UOTGHS_HSTPIPCFG_PTYPE_Pos 12
#define UOTGHS_HSTPIPCFG_PTYPE_Msk (0x3u << UOTGHS_HSTPIPCFG_PTYPE_Pos) /**< \brief (UOTGHS_HSTPIPCFG[10]) Pipe Type */
#define   UOTGHS_HSTPIPCFG_PTYPE_CTRL (0x0u << 12) /**< \brief (UOTGHS_HSTPIPCFG[10]) Control */
#define   UOTGHS_HSTPIPCFG_PTYPE_ISO (0x1u << 12) /**< \brief (UOTGHS_HSTPIPCFG[10]) Isochronous */
#define   UOTGHS_HSTPIPCFG_PTYPE_BLK (0x2u << 12) /**< \brief (UOTGHS_HSTPIPCFG[10]) Bulk */
#define   UOTGHS_HSTPIPCFG_PTYPE_INTRPT (0x3u << 12) /**< \brief (UOTGHS_HSTPIPCFG[10]) Interrupt */
#define UOTGHS_HSTPIPCFG_PEPNUM_Pos 16
#define UOTGHS_HSTPIPCFG_PEPNUM_Msk (0xfu << UOTGHS_HSTPIPCFG_PEPNUM_Pos) /**< \brief (UOTGHS_HSTPIPCFG[10]) Pipe Endpoint Number */
#define UOTGHS_HSTPIPCFG_PEPNUM(value) ((UOTGHS_HSTPIPCFG_PEPNUM_Msk & ((value) << UOTGHS_HSTPIPCFG_PEPNUM_Pos)))
#define UOTGHS_HSTPIPCFG_INTFRQ_Pos 24
#define UOTGHS_HSTPIPCFG_INTFRQ_Msk (0xffu << UOTGHS_HSTPIPCFG_INTFRQ_Pos) /**< \brief (UOTGHS_HSTPIPCFG[10]) Pipe Interrupt Request Frequency */
#define UOTGHS_HSTPIPCFG_INTFRQ(value) ((UOTGHS_HSTPIPCFG_INTFRQ_Msk & ((value) << UOTGHS_HSTPIPCFG_INTFRQ_Pos)))
#define UOTGHS_HSTPIPCFG_PINGEN (0x1u << 20) /**< \brief (UOTGHS_HSTPIPCFG[10]) Ping Enable */
#define UOTGHS_HSTPIPCFG_BINTERVAL_Pos 24
#define UOTGHS_HSTPIPCFG_BINTERVAL_Msk (0xffu << UOTGHS_HSTPIPCFG_BINTERVAL_Pos) /**< \brief (UOTGHS_HSTPIPCFG[10]) Binterval Parameter for the Bulk-Out/Ping Transaction */
#define UOTGHS_HSTPIPCFG_BINTERVAL(value) ((UOTGHS_HSTPIPCFG_BINTERVAL_Msk & ((value) << UOTGHS_HSTPIPCFG_BINTERVAL_Pos)))
/* -------- UOTGHS_HSTPIPISR[10] : (UOTGHS Offset: 0x530) Host Pipe Status Register (n = 0) -------- */
#define UOTGHS_HSTPIPISR_RXINI (0x1u << 0) /**< \brief (UOTGHS_HSTPIPISR[10]) Received IN Data Interrupt */
#define UOTGHS_HSTPIPISR_TXOUTI (0x1u << 1) /**< \brief (UOTGHS_HSTPIPISR[10]) Transmitted OUT Data Interrupt */
#define UOTGHS_HSTPIPISR_TXSTPI (0x1u << 2) /**< \brief (UOTGHS_HSTPIPISR[10]) Transmitted SETUP Interrupt */
#define UOTGHS_HSTPIPISR_PERRI (0x1u << 3) /**< \brief (UOTGHS_HSTPIPISR[10]) Pipe Error Interrupt */
#define UOTGHS_HSTPIPISR_NAKEDI (0x1u << 4) /**< \brief (UOTGHS_HSTPIPISR[10]) NAKed Interrupt */
#define UOTGHS_HSTPIPISR_OVERFI (0x1u << 5) /**< \brief (UOTGHS_HSTPIPISR[10]) Overflow Interrupt */
#define UOTGHS_HSTPIPISR_RXSTALLDI (0x1u << 6) /**< \brief (UOTGHS_HSTPIPISR[10]) Received STALLed Interrupt */
#define UOTGHS_HSTPIPISR_SHORTPACKETI (0x1u << 7) /**< \brief (UOTGHS_HSTPIPISR[10]) Short Packet Interrupt */
#define UOTGHS_HSTPIPISR_DTSEQ_Pos 8
#define UOTGHS_HSTPIPISR_DTSEQ_Msk (0x3u << UOTGHS_HSTPIPISR_DTSEQ_Pos) /**< \brief (UOTGHS_HSTPIPISR[10]) Data Toggle Sequence */
#define   UOTGHS_HSTPIPISR_DTSEQ_DATA0 (0x0u << 8) /**< \brief (UOTGHS_HSTPIPISR[10]) Data0 toggle sequence */
#define   UOTGHS_HSTPIPISR_DTSEQ_DATA1 (0x1u << 8) /**< \brief (UOTGHS_HSTPIPISR[10]) Data1 toggle sequence */
#define UOTGHS_HSTPIPISR_NBUSYBK_Pos 12
#define UOTGHS_HSTPIPISR_NBUSYBK_Msk (0x3u << UOTGHS_HSTPIPISR_NBUSYBK_Pos) /**< \brief (UOTGHS_HSTPIPISR[10]) Number of Busy Banks */
#define   UOTGHS_HSTPIPISR_NBUSYBK_0_BUSY (0x0u << 12) /**< \brief (UOTGHS_HSTPIPISR[10]) 0 busy bank (all banks free) */
#define   UOTGHS_HSTPIPISR_NBUSYBK_1_BUSY (0x1u << 12) /**< \brief (UOTGHS_HSTPIPISR[10]) 1 busy bank */
#define   UOTGHS_HSTPIPISR_NBUSYBK_2_BUSY (0x2u << 12) /**< \brief (UOTGHS_HSTPIPISR[10]) 2 busy banks */
#define   UOTGHS_HSTPIPISR_NBUSYBK_3_BUSY (0x3u << 12) /**< \brief (UOTGHS_HSTPIPISR[10]) 3 busy banks */
#define UOTGHS_HSTPIPISR_CURRBK_Pos 14
#define UOTGHS_HSTPIPISR_CURRBK_Msk (0x3u << UOTGHS_HSTPIPISR_CURRBK_Pos) /**< \brief (UOTGHS_HSTPIPISR[10]) Current Bank */
#define   UOTGHS_HSTPIPISR_CURRBK_BANK0 (0x0u << 14) /**< \brief (UOTGHS_HSTPIPISR[10]) Current bank is bank0 */
#define   UOTGHS_HSTPIPISR_CURRBK_BANK1 (0x1u << 14) /**< \brief (UOTGHS_HSTPIPISR[10]) Current bank is bank1 */
#define   UOTGHS_HSTPIPISR_CURRBK_BANK2 (0x2u << 14) /**< \brief (UOTGHS_HSTPIPISR[10]) Current bank is bank2 */
#define UOTGHS_HSTPIPISR_RWALL (0x1u << 16) /**< \brief (UOTGHS_HSTPIPISR[10]) Read-write Allowed */
#define UOTGHS_HSTPIPISR_CFGOK (0x1u << 18) /**< \brief (UOTGHS_HSTPIPISR[10]) Configuration OK Status */
#define UOTGHS_HSTPIPISR_PBYCT_Pos 20
#define UOTGHS_HSTPIPISR_PBYCT_Msk (0x7ffu << UOTGHS_HSTPIPISR_PBYCT_Pos) /**< \brief (UOTGHS_HSTPIPISR[10]) Pipe Byte Count */
#define UOTGHS_HSTPIPISR_UNDERFI (0x1u << 2) /**< \brief (UOTGHS_HSTPIPISR[10]) Underflow Interrupt */
#define UOTGHS_HSTPIPISR_CRCERRI (0x1u << 6) /**< \brief (UOTGHS_HSTPIPISR[10]) CRC Error Interrupt */
/* -------- UOTGHS_HSTPIPICR[10] : (UOTGHS Offset: 0x560) Host Pipe Clear Register (n = 0) -------- */
#define UOTGHS_HSTPIPICR_RXINIC (0x1u << 0) /**< \brief (UOTGHS_HSTPIPICR[10]) Received IN Data Interrupt Clear */
#define UOTGHS_HSTPIPICR_TXOUTIC (0x1u << 1) /**< \brief (UOTGHS_HSTPIPICR[10]) Transmitted OUT Data Interrupt Clear */
#define UOTGHS_HSTPIPICR_TXSTPIC (0x1u << 2) /**< \brief (UOTGHS_HSTPIPICR[10]) Transmitted SETUP Interrupt Clear */
#define UOTGHS_HSTPIPICR_NAKEDIC (0x1u << 4) /**< \brief (UOTGHS_HSTPIPICR[10]) NAKed Interrupt Clear */
#define UOTGHS_HSTPIPICR_OVERFIC (0x1u << 5) /**< \brief (UOTGHS_HSTPIPICR[10]) Overflow Interrupt Clear */
#define UOTGHS_HSTPIPICR_RXSTALLDIC (0x1u << 6) /**< \brief (UOTGHS_HSTPIPICR[10]) Received STALLed Interrupt Clear */
#define UOTGHS_HSTPIPICR_SHORTPACKETIC (0x1u << 7) /**< \brief (UOTGHS_HSTPIPICR[10]) Short Packet Interrupt Clear */
#define UOTGHS_HSTPIPICR_UNDERFIC (0x1u << 2) /**< \brief (UOTGHS_HSTPIPICR[10]) Underflow Interrupt Clear */
#define UOTGHS_HSTPIPICR_CRCERRIC (0x1u << 6) /**< \brief (UOTGHS_HSTPIPICR[10]) CRC Error Interrupt Clear */
/* -------- UOTGHS_HSTPIPIFR[10] : (UOTGHS Offset: 0x590) Host Pipe Set Register (n = 0) -------- */
#define UOTGHS_HSTPIPIFR_RXINIS (0x1u << 0) /**< \brief (UOTGHS_HSTPIPIFR[10]) Received IN Data Interrupt Set */
#define UOTGHS_HSTPIPIFR_TXOUTIS (0x1u << 1) /**< \brief (UOTGHS_HSTPIPIFR[10]) Transmitted OUT Data Interrupt Set */
#define UOTGHS_HSTPIPIFR_TXSTPIS (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIFR[10]) Transmitted SETUP Interrupt Set */
#define UOTGHS_HSTPIPIFR_PERRIS (0x1u << 3) /**< \brief (UOTGHS_HSTPIPIFR[10]) Pipe Error Interrupt Set */
#define UOTGHS_HSTPIPIFR_NAKEDIS (0x1u << 4) /**< \brief (UOTGHS_HSTPIPIFR[10]) NAKed Interrupt Set */
#define UOTGHS_HSTPIPIFR_OVERFIS (0x1u << 5) /**< \brief (UOTGHS_HSTPIPIFR[10]) Overflow Interrupt Set */
#define UOTGHS_HSTPIPIFR_RXSTALLDIS (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIFR[10]) Received STALLed Interrupt Set */
#define UOTGHS_HSTPIPIFR_SHORTPACKETIS (0x1u << 7) /**< \brief (UOTGHS_HSTPIPIFR[10]) Short Packet Interrupt Set */
#define UOTGHS_HSTPIPIFR_NBUSYBKS (0x1u << 12) /**< \brief (UOTGHS_HSTPIPIFR[10]) Number of Busy Banks Set */
#define UOTGHS_HSTPIPIFR_UNDERFIS (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIFR[10]) Underflow Interrupt Set */
#define UOTGHS_HSTPIPIFR_CRCERRIS (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIFR[10]) CRC Error Interrupt Set */
/* -------- UOTGHS_HSTPIPIMR[10] : (UOTGHS Offset: 0x5C0) Host Pipe Mask Register (n = 0) -------- */
#define UOTGHS_HSTPIPIMR_RXINE (0x1u << 0) /**< \brief (UOTGHS_HSTPIPIMR[10]) Received IN Data Interrupt Enable */
#define UOTGHS_HSTPIPIMR_TXOUTE (0x1u << 1) /**< \brief (UOTGHS_HSTPIPIMR[10]) Transmitted OUT Data Interrupt Enable */
#define UOTGHS_HSTPIPIMR_TXSTPE (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIMR[10]) Transmitted SETUP Interrupt Enable */
#define UOTGHS_HSTPIPIMR_PERRE (0x1u << 3) /**< \brief (UOTGHS_HSTPIPIMR[10]) Pipe Error Interrupt Enable */
#define UOTGHS_HSTPIPIMR_NAKEDE (0x1u << 4) /**< \brief (UOTGHS_HSTPIPIMR[10]) NAKed Interrupt Enable */
#define UOTGHS_HSTPIPIMR_OVERFIE (0x1u << 5) /**< \brief (UOTGHS_HSTPIPIMR[10]) Overflow Interrupt Enable */
#define UOTGHS_HSTPIPIMR_RXSTALLDE (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIMR[10]) Received STALLed Interrupt Enable */
#define UOTGHS_HSTPIPIMR_SHORTPACKETIE (0x1u << 7) /**< \brief (UOTGHS_HSTPIPIMR[10]) Short Packet Interrupt Enable */
#define UOTGHS_HSTPIPIMR_NBUSYBKE (0x1u << 12) /**< \brief (UOTGHS_HSTPIPIMR[10]) Number of Busy Banks Interrupt Enable */
#define UOTGHS_HSTPIPIMR_FIFOCON (0x1u << 14) /**< \brief (UOTGHS_HSTPIPIMR[10]) FIFO Control */
#define UOTGHS_HSTPIPIMR_PDISHDMA (0x1u << 16) /**< \brief (UOTGHS_HSTPIPIMR[10]) Pipe Interrupts Disable HDMA Request Enable */
#define UOTGHS_HSTPIPIMR_PFREEZE (0x1u << 17) /**< \brief (UOTGHS_HSTPIPIMR[10]) Pipe Freeze */
#define UOTGHS_HSTPIPIMR_RSTDT (0x1u << 18) /**< \brief (UOTGHS_HSTPIPIMR[10]) Reset Data Toggle */
#define UOTGHS_HSTPIPIMR_UNDERFIE (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIMR[10]) Underflow Interrupt Enable */
#define UOTGHS_HSTPIPIMR_CRCERRE (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIMR[10]) CRC Error Interrupt Enable */
/* -------- UOTGHS_HSTPIPIER[10] : (UOTGHS Offset: 0x5F0) Host Pipe Enable Register (n = 0) -------- */
#define UOTGHS_HSTPIPIER_RXINES (0x1u << 0) /**< \brief (UOTGHS_HSTPIPIER[10]) Received IN Data Interrupt Enable */
#define UOTGHS_HSTPIPIER_TXOUTES (0x1u << 1) /**< \brief (UOTGHS_HSTPIPIER[10]) Transmitted OUT Data Interrupt Enable */
#define UOTGHS_HSTPIPIER_TXSTPES (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIER[10]) Transmitted SETUP Interrupt Enable */
#define UOTGHS_HSTPIPIER_PERRES (0x1u << 3) /**< \brief (UOTGHS_HSTPIPIER[10]) Pipe Error Interrupt Enable */
#define UOTGHS_HSTPIPIER_NAKEDES (0x1u << 4) /**< \brief (UOTGHS_HSTPIPIER[10]) NAKed Interrupt Enable */
#define UOTGHS_HSTPIPIER_OVERFIES (0x1u << 5) /**< \brief (UOTGHS_HSTPIPIER[10]) Overflow Interrupt Enable */
#define UOTGHS_HSTPIPIER_RXSTALLDES (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIER[10]) Received STALLed Interrupt Enable */
#define UOTGHS_HSTPIPIER_SHORTPACKETIES (0x1u << 7) /**< \brief (UOTGHS_HSTPIPIER[10]) Short Packet Interrupt Enable */
#define UOTGHS_HSTPIPIER_NBUSYBKES (0x1u << 12) /**< \brief (UOTGHS_HSTPIPIER[10]) Number of Busy Banks Enable */
#define UOTGHS_HSTPIPIER_PDISHDMAS (0x1u << 16) /**< \brief (UOTGHS_HSTPIPIER[10]) Pipe Interrupts Disable HDMA Request Enable */
#define UOTGHS_HSTPIPIER_PFREEZES (0x1u << 17) /**< \brief (UOTGHS_HSTPIPIER[10]) Pipe Freeze Enable */
#define UOTGHS_HSTPIPIER_RSTDTS (0x1u << 18) /**< \brief (UOTGHS_HSTPIPIER[10]) Reset Data Toggle Enable */
#define UOTGHS_HSTPIPIER_UNDERFIES (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIER[10]) Underflow Interrupt Enable */
#define UOTGHS_HSTPIPIER_CRCERRES (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIER[10]) CRC Error Interrupt Enable */
/* -------- UOTGHS_HSTPIPIDR[10] : (UOTGHS Offset: 0x620) Host Pipe Disable Register (n = 0) -------- */
#define UOTGHS_HSTPIPIDR_RXINEC (0x1u << 0) /**< \brief (UOTGHS_HSTPIPIDR[10]) Received IN Data Interrupt Disable */
#define UOTGHS_HSTPIPIDR_TXOUTEC (0x1u << 1) /**< \brief (UOTGHS_HSTPIPIDR[10]) Transmitted OUT Data Interrupt Disable */
#define UOTGHS_HSTPIPIDR_TXSTPEC (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIDR[10]) Transmitted SETUP Interrupt Disable */
#define UOTGHS_HSTPIPIDR_PERREC (0x1u << 3) /**< \brief (UOTGHS_HSTPIPIDR[10]) Pipe Error Interrupt Disable */
#define UOTGHS_HSTPIPIDR_NAKEDEC (0x1u << 4) /**< \brief (UOTGHS_HSTPIPIDR[10]) NAKed Interrupt Disable */
#define UOTGHS_HSTPIPIDR_OVERFIEC (0x1u << 5) /**< \brief (UOTGHS_HSTPIPIDR[10]) Overflow Interrupt Disable */
#define UOTGHS_HSTPIPIDR_RXSTALLDEC (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIDR[10]) Received STALLed Interrupt Disable */
#define UOTGHS_HSTPIPIDR_SHORTPACKETIEC (0x1u << 7) /**< \brief (UOTGHS_HSTPIPIDR[10]) Short Packet Interrupt Disable */
#define UOTGHS_HSTPIPIDR_NBUSYBKEC (0x1u << 12) /**< \brief (UOTGHS_HSTPIPIDR[10]) Number of Busy Banks Disable */
#define UOTGHS_HSTPIPIDR_FIFOCONC (0x1u << 14) /**< \brief (UOTGHS_HSTPIPIDR[10]) FIFO Control Disable */
#define UOTGHS_HSTPIPIDR_PDISHDMAC (0x1u << 16) /**< \brief (UOTGHS_HSTPIPIDR[10]) Pipe Interrupts Disable HDMA Request Disable */
#define UOTGHS_HSTPIPIDR_PFREEZEC (0x1u << 17) /**< \brief (UOTGHS_HSTPIPIDR[10]) Pipe Freeze Disable */
#define UOTGHS_HSTPIPIDR_UNDERFIEC (0x1u << 2) /**< \brief (UOTGHS_HSTPIPIDR[10]) Underflow Interrupt Disable */
#define UOTGHS_HSTPIPIDR_CRCERREC (0x1u << 6) /**< \brief (UOTGHS_HSTPIPIDR[10]) CRC Error Interrupt Disable */
/* -------- UOTGHS_HSTPIPINRQ[10] : (UOTGHS Offset: 0x650) Host Pipe IN Request Register (n = 0) -------- */
#define UOTGHS_HSTPIPINRQ_INRQ_Pos 0
#define UOTGHS_HSTPIPINRQ_INRQ_Msk (0xffu << UOTGHS_HSTPIPINRQ_INRQ_Pos) /**< \brief (UOTGHS_HSTPIPINRQ[10]) IN Request Number before Freeze */
#define UOTGHS_HSTPIPINRQ_INRQ(value) ((UOTGHS_HSTPIPINRQ_INRQ_Msk & ((value) << UOTGHS_HSTPIPINRQ_INRQ_Pos)))
#define UOTGHS_HSTPIPINRQ_INMODE (0x1u << 8) /**< \brief (UOTGHS_HSTPIPINRQ[10]) IN Request Mode */
/* -------- UOTGHS_HSTPIPERR[10] : (UOTGHS Offset: 0x680) Host Pipe Error Register (n = 0) -------- */
#define UOTGHS_HSTPIPERR_DATATGL (0x1u << 0) /**< \brief (UOTGHS_HSTPIPERR[10]) Data Toggle Error */
#define UOTGHS_HSTPIPERR_DATAPID (0x1u << 1) /**< \brief (UOTGHS_HSTPIPERR[10]) Data PID Error */
#define UOTGHS_HSTPIPERR_PID (0x1u << 2) /**< \brief (UOTGHS_HSTPIPERR[10]) PID Error */
#define UOTGHS_HSTPIPERR_TIMEOUT (0x1u << 3) /**< \brief (UOTGHS_HSTPIPERR[10]) Time-Out Error */
#define UOTGHS_HSTPIPERR_CRC16 (0x1u << 4) /**< \brief (UOTGHS_HSTPIPERR[10]) CRC16 Error */
#define UOTGHS_HSTPIPERR_COUNTER_Pos 5
#define UOTGHS_HSTPIPERR_COUNTER_Msk (0x3u << UOTGHS_HSTPIPERR_COUNTER_Pos) /**< \brief (UOTGHS_HSTPIPERR[10]) Error Counter */
#define UOTGHS_HSTPIPERR_COUNTER(value) ((UOTGHS_HSTPIPERR_COUNTER_Msk & ((value) << UOTGHS_HSTPIPERR_COUNTER_Pos)))
/* -------- UOTGHS_HSTDMANXTDSC : (UOTGHS Offset: N/A) Host DMA Channel Next Descriptor Address Register -------- */
#define UOTGHS_HSTDMANXTDSC_NXT_DSC_ADD_Pos 0
#define UOTGHS_HSTDMANXTDSC_NXT_DSC_ADD_Msk (0xffffffffu << UOTGHS_HSTDMANXTDSC_NXT_DSC_ADD_Pos) /**< \brief (UOTGHS_HSTDMANXTDSC) Next Descriptor Address */
#define UOTGHS_HSTDMANXTDSC_NXT_DSC_ADD(value) ((UOTGHS_HSTDMANXTDSC_NXT_DSC_ADD_Msk & ((value) << UOTGHS_HSTDMANXTDSC_NXT_DSC_ADD_Pos)))
/* -------- UOTGHS_HSTDMAADDRESS : (UOTGHS Offset: N/A) Host DMA Channel Address Register -------- */
#define UOTGHS_HSTDMAADDRESS_BUFF_ADD_Pos 0
#define UOTGHS_HSTDMAADDRESS_BUFF_ADD_Msk (0xffffffffu << UOTGHS_HSTDMAADDRESS_BUFF_ADD_Pos) /**< \brief (UOTGHS_HSTDMAADDRESS) Buffer Address */
#define UOTGHS_HSTDMAADDRESS_BUFF_ADD(value) ((UOTGHS_HSTDMAADDRESS_BUFF_ADD_Msk & ((value) << UOTGHS_HSTDMAADDRESS_BUFF_ADD_Pos)))
/* -------- UOTGHS_HSTDMACONTROL : (UOTGHS Offset: N/A) Host DMA Channel Control Register -------- */
#define UOTGHS_HSTDMACONTROL_CHANN_ENB (0x1u << 0) /**< \brief (UOTGHS_HSTDMACONTROL) Channel Enable Command */
#define UOTGHS_HSTDMACONTROL_LDNXT_DSC (0x1u << 1) /**< \brief (UOTGHS_HSTDMACONTROL) Load Next Channel Transfer Descriptor Enable Command */
#define UOTGHS_HSTDMACONTROL_END_TR_EN (0x1u << 2) /**< \brief (UOTGHS_HSTDMACONTROL) End of Transfer Enable (Control) */
#define UOTGHS_HSTDMACONTROL_END_B_EN (0x1u << 3) /**< \brief (UOTGHS_HSTDMACONTROL) End of Buffer Enable Control */
#define UOTGHS_HSTDMACONTROL_END_TR_IT (0x1u << 4) /**< \brief (UOTGHS_HSTDMACONTROL) End of Transfer Interrupt Enable */
#define UOTGHS_HSTDMACONTROL_END_BUFFIT (0x1u << 5) /**< \brief (UOTGHS_HSTDMACONTROL) End of Buffer Interrupt Enable */
#define UOTGHS_HSTDMACONTROL_DESC_LD_IT (0x1u << 6) /**< \brief (UOTGHS_HSTDMACONTROL) Descriptor Loaded Interrupt Enable */
#define UOTGHS_HSTDMACONTROL_BURST_LCK (0x1u << 7) /**< \brief (UOTGHS_HSTDMACONTROL) Burst Lock Enable */
#define UOTGHS_HSTDMACONTROL_BUFF_LENGTH_Pos 16
#define UOTGHS_HSTDMACONTROL_BUFF_LENGTH_Msk (0xffffu << UOTGHS_HSTDMACONTROL_BUFF_LENGTH_Pos) /**< \brief (UOTGHS_HSTDMACONTROL) Buffer Byte Length (Write-only) */
#define UOTGHS_HSTDMACONTROL_BUFF_LENGTH(value) ((UOTGHS_HSTDMACONTROL_BUFF_LENGTH_Msk & ((value) << UOTGHS_HSTDMACONTROL_BUFF_LENGTH_Pos)))
/* -------- UOTGHS_HSTDMASTATUS : (UOTGHS Offset: N/A) Host DMA Channel Status Register -------- */
#define UOTGHS_HSTDMASTATUS_CHANN_ENB (0x1u << 0) /**< \brief (UOTGHS_HSTDMASTATUS) Channel Enable Status */
#define UOTGHS_HSTDMASTATUS_CHANN_ACT (0x1u << 1) /**< \brief (UOTGHS_HSTDMASTATUS) Channel Active Status */
#define UOTGHS_HSTDMASTATUS_END_TR_ST (0x1u << 4) /**< \brief (UOTGHS_HSTDMASTATUS) End of Channel Transfer Status */
#define UOTGHS_HSTDMASTATUS_END_BF_ST (0x1u << 5) /**< \brief (UOTGHS_HSTDMASTATUS) End of Channel Buffer Status */
#define UOTGHS_HSTDMASTATUS_DESC_LDST (0x1u << 6) /**< \brief (UOTGHS_HSTDMASTATUS) Descriptor Loaded Status */
#define UOTGHS_HSTDMASTATUS_BUFF_COUNT_Pos 16
#define UOTGHS_HSTDMASTATUS_BUFF_COUNT_Msk (0xffffu << UOTGHS_HSTDMASTATUS_BUFF_COUNT_Pos) /**< \brief (UOTGHS_HSTDMASTATUS) Buffer Byte Count */
#define UOTGHS_HSTDMASTATUS_BUFF_COUNT(value) ((UOTGHS_HSTDMASTATUS_BUFF_COUNT_Msk & ((value) << UOTGHS_HSTDMASTATUS_BUFF_COUNT_Pos)))
/* -------- UOTGHS_CTRL : (UOTGHS Offset: 0x0800) General Control Register -------- */
#define UOTGHS_CTRL_IDTE (0x1u << 0) /**< \brief (UOTGHS_CTRL) ID Transition Interrupt Enable */
#define UOTGHS_CTRL_VBUSTE (0x1u << 1) /**< \brief (UOTGHS_CTRL) VBus Transition Interrupt Enable */
#define UOTGHS_CTRL_SRPE (0x1u << 2) /**< \brief (UOTGHS_CTRL) SRP Interrupt Enable */
#define UOTGHS_CTRL_VBERRE (0x1u << 3) /**< \brief (UOTGHS_CTRL) VBus Error Interrupt Enable */
#define UOTGHS_CTRL_BCERRE (0x1u << 4) /**< \brief (UOTGHS_CTRL) B-Connection Error Interrupt Enable */
#define UOTGHS_CTRL_ROLEEXE (0x1u << 5) /**< \brief (UOTGHS_CTRL) Role Exchange Interrupt Enable */
#define UOTGHS_CTRL_HNPERRE (0x1u << 6) /**< \brief (UOTGHS_CTRL) HNP Error Interrupt Enable */
#define UOTGHS_CTRL_STOE (0x1u << 7) /**< \brief (UOTGHS_CTRL) Suspend Time-Out Interrupt Enable */
#define UOTGHS_CTRL_VBUSHWC (0x1u << 8) /**< \brief (UOTGHS_CTRL) VBus Hardware Control */
#define UOTGHS_CTRL_SRPSEL (0x1u << 9) /**< \brief (UOTGHS_CTRL) SRP Selection */
#define UOTGHS_CTRL_SRPREQ (0x1u << 10) /**< \brief (UOTGHS_CTRL) SRP Request */
#define UOTGHS_CTRL_HNPREQ (0x1u << 11) /**< \brief (UOTGHS_CTRL) HNP Request */
#define UOTGHS_CTRL_OTGPADE (0x1u << 12) /**< \brief (UOTGHS_CTRL) OTG Pad Enable */
#define UOTGHS_CTRL_VBUSPO (0x1u << 13) /**< \brief (UOTGHS_CTRL) VBus Polarity Off */
#define UOTGHS_CTRL_FRZCLK (0x1u << 14) /**< \brief (UOTGHS_CTRL) Freeze USB Clock */
#define UOTGHS_CTRL_USBE (0x1u << 15) /**< \brief (UOTGHS_CTRL) UOTGHS Enable */
#define UOTGHS_CTRL_TIMVALUE_Pos 16
#define UOTGHS_CTRL_TIMVALUE_Msk (0x3u << UOTGHS_CTRL_TIMVALUE_Pos) /**< \brief (UOTGHS_CTRL) Timer Value */
#define UOTGHS_CTRL_TIMVALUE(value) ((UOTGHS_CTRL_TIMVALUE_Msk & ((value) << UOTGHS_CTRL_TIMVALUE_Pos)))
#define UOTGHS_CTRL_TIMPAGE_Pos 20
#define UOTGHS_CTRL_TIMPAGE_Msk (0x3u << UOTGHS_CTRL_TIMPAGE_Pos) /**< \brief (UOTGHS_CTRL) Timer Page */
#define UOTGHS_CTRL_TIMPAGE(value) ((UOTGHS_CTRL_TIMPAGE_Msk & ((value) << UOTGHS_CTRL_TIMPAGE_Pos)))
#define UOTGHS_CTRL_UNLOCK (0x1u << 22) /**< \brief (UOTGHS_CTRL) Timer Access Unlock */
#define UOTGHS_CTRL_UIDE (0x1u << 24) /**< \brief (UOTGHS_CTRL) UOTGID Pin Enable */
#define   UOTGHS_CTRL_UIDE_UIMOD (0x0u << 24) /**< \brief (UOTGHS_CTRL) The USB mode (device/host) is selected from the UIMOD bit. */
#define   UOTGHS_CTRL_UIDE_UOTGID (0x1u << 24) /**< \brief (UOTGHS_CTRL) The USB mode (device/host) is selected from the UOTGID input pin. */
#define UOTGHS_CTRL_UIMOD (0x1u << 25) /**< \brief (UOTGHS_CTRL) UOTGHS Mode */
#define   UOTGHS_CTRL_UIMOD_HOST (0x0u << 25) /**< \brief (UOTGHS_CTRL) The module is in USB host mode. */
#define   UOTGHS_CTRL_UIMOD_DEVICE (0x1u << 25) /**< \brief (UOTGHS_CTRL) The module is in USB device mode. */
/* -------- UOTGHS_SR : (UOTGHS Offset: 0x0804) General Status Register -------- */
#define UOTGHS_SR_IDTI (0x1u << 0) /**< \brief (UOTGHS_SR) ID Transition Interrupt */
#define UOTGHS_SR_VBUSTI (0x1u << 1) /**< \brief (UOTGHS_SR) VBus Transition Interrupt */
#define UOTGHS_SR_SRPI (0x1u << 2) /**< \brief (UOTGHS_SR) SRP Interrupt */
#define UOTGHS_SR_VBERRI (0x1u << 3) /**< \brief (UOTGHS_SR) VBus Error Interrupt */
#define UOTGHS_SR_BCERRI (0x1u << 4) /**< \brief (UOTGHS_SR) B-Connection Error Interrupt */
#define UOTGHS_SR_ROLEEXI (0x1u << 5) /**< \brief (UOTGHS_SR) Role Exchange Interrupt */
#define UOTGHS_SR_HNPERRI (0x1u << 6) /**< \brief (UOTGHS_SR) HNP Error Interrupt */
#define UOTGHS_SR_STOI (0x1u << 7) /**< \brief (UOTGHS_SR) Suspend Time-Out Interrupt */
#define UOTGHS_SR_VBUSRQ (0x1u << 9) /**< \brief (UOTGHS_SR) VBus Request */
#define UOTGHS_SR_ID (0x1u << 10) /**< \brief (UOTGHS_SR) UOTGID Pin State */
#define UOTGHS_SR_VBUS (0x1u << 11) /**< \brief (UOTGHS_SR) VBus Level */
#define UOTGHS_SR_SPEED_Pos 12
#define UOTGHS_SR_SPEED_Msk (0x3u << UOTGHS_SR_SPEED_Pos) /**< \brief (UOTGHS_SR) Speed Status */
#define   UOTGHS_SR_SPEED_FULL_SPEED (0x0u << 12) /**< \brief (UOTGHS_SR) Full-Speed mode */
#define   UOTGHS_SR_SPEED_HIGH_SPEED (0x1u << 12) /**< \brief (UOTGHS_SR) High-Speed mode */
#define   UOTGHS_SR_SPEED_LOW_SPEED (0x2u << 12) /**< \brief (UOTGHS_SR) Low-Speed mode */
#define UOTGHS_SR_CLKUSABLE (0x1u << 14) /**< \brief (UOTGHS_SR) UTMI Clock Usable */
/* -------- UOTGHS_SCR : (UOTGHS Offset: 0x0808) General Status Clear Register -------- */
#define UOTGHS_SCR_IDTIC (0x1u << 0) /**< \brief (UOTGHS_SCR) ID Transition Interrupt Clear */
#define UOTGHS_SCR_VBUSTIC (0x1u << 1) /**< \brief (UOTGHS_SCR) VBus Transition Interrupt Clear */
#define UOTGHS_SCR_SRPIC (0x1u << 2) /**< \brief (UOTGHS_SCR) SRP Interrupt Clear */
#define UOTGHS_SCR_VBERRIC (0x1u << 3) /**< \brief (UOTGHS_SCR) VBus Error Interrupt Clear */
#define UOTGHS_SCR_BCERRIC (0x1u << 4) /**< \brief (UOTGHS_SCR) B-Connection Error Interrupt Clear */
#define UOTGHS_SCR_ROLEEXIC (0x1u << 5) /**< \brief (UOTGHS_SCR) Role Exchange Interrupt Clear */
#define UOTGHS_SCR_HNPERRIC (0x1u << 6) /**< \brief (UOTGHS_SCR) HNP Error Interrupt Clear */
#define UOTGHS_SCR_STOIC (0x1u << 7) /**< \brief (UOTGHS_SCR) Suspend Time-Out Interrupt Clear */
#define UOTGHS_SCR_VBUSRQC (0x1u << 9) /**< \brief (UOTGHS_SCR) VBus Request Clear */
/* -------- UOTGHS_SFR : (UOTGHS Offset: 0x080C) General Status Set Register -------- */
#define UOTGHS_SFR_IDTIS (0x1u << 0) /**< \brief (UOTGHS_SFR) ID Transition Interrupt Set */
#define UOTGHS_SFR_VBUSTIS (0x1u << 1) /**< \brief (UOTGHS_SFR) VBus Transition Interrupt Set */
#define UOTGHS_SFR_SRPIS (0x1u << 2) /**< \brief (UOTGHS_SFR) SRP Interrupt Set */
#define UOTGHS_SFR_VBERRIS (0x1u << 3) /**< \brief (UOTGHS_SFR) VBus Error Interrupt Set */
#define UOTGHS_SFR_BCERRIS (0x1u << 4) /**< \brief (UOTGHS_SFR) B-Connection Error Interrupt Set */
#define UOTGHS_SFR_ROLEEXIS (0x1u << 5) /**< \brief (UOTGHS_SFR) Role Exchange Interrupt Set */
#define UOTGHS_SFR_HNPERRIS (0x1u << 6) /**< \brief (UOTGHS_SFR) HNP Error Interrupt Set */
#define UOTGHS_SFR_STOIS (0x1u << 7) /**< \brief (UOTGHS_SFR) Suspend Time-Out Interrupt Set */
#define UOTGHS_SFR_VBUSRQS (0x1u << 9) /**< \brief (UOTGHS_SFR) VBus Request Set */
/* -------- UOTGHS_FSM : (UOTGHS Offset: 0x082C) General Finite State Machine Register -------- */
#define UOTGHS_FSM_DRDSTATE_Pos 0
#define UOTGHS_FSM_DRDSTATE_Msk (0xfu << UOTGHS_FSM_DRDSTATE_Pos) /**< \brief (UOTGHS_FSM) Dual Role Device State */
#define   UOTGHS_FSM_DRDSTATE_A_IDLESTATE (0x0u << 0) /**< \brief (UOTGHS_FSM) This is the start state for A-devices (when the ID pin is 0) */
#define   UOTGHS_FSM_DRDSTATE_A_WAIT_VRISE (0x1u << 0) /**< \brief (UOTGHS_FSM) In this state, the A-device waits for the voltage on VBus to rise above the A-device VBus Valid threshold (4.4 V). */
#define   UOTGHS_FSM_DRDSTATE_A_WAIT_BCON (0x2u << 0) /**< \brief (UOTGHS_FSM) In this state, the A-device waits for the B-device to signal a connection. */
#define   UOTGHS_FSM_DRDSTATE_A_HOST (0x3u << 0) /**< \brief (UOTGHS_FSM) In this state, the A-device that operates in Host mode is operational. */
#define   UOTGHS_FSM_DRDSTATE_A_SUSPEND (0x4u << 0) /**< \brief (UOTGHS_FSM) The A-device operating as a host is in the suspend mode. */
#define   UOTGHS_FSM_DRDSTATE_A_PERIPHERAL (0x5u << 0) /**< \brief (UOTGHS_FSM) The A-device operates as a peripheral. */
#define   UOTGHS_FSM_DRDSTATE_A_WAIT_VFALL (0x6u << 0) /**< \brief (UOTGHS_FSM) In this state, the A-device waits for the voltage on VBus to drop below the A-device Session Valid threshold (1.4 V). */
#define   UOTGHS_FSM_DRDSTATE_A_VBUS_ERR (0x7u << 0) /**< \brief (UOTGHS_FSM) In this state, the A-device waits for recovery of the over-current condition that caused it to enter this state. */
#define   UOTGHS_FSM_DRDSTATE_A_WAIT_DISCHARGE (0x8u << 0) /**< \brief (UOTGHS_FSM) In this state, the A-device waits for the data USB line to discharge (100 us). */
#define   UOTGHS_FSM_DRDSTATE_B_IDLE (0x9u << 0) /**< \brief (UOTGHS_FSM) This is the start state for B-device (when the ID pin is 1). */
#define   UOTGHS_FSM_DRDSTATE_B_PERIPHERAL (0xAu << 0) /**< \brief (UOTGHS_FSM) In this state, the B-device acts as the peripheral. */
#define   UOTGHS_FSM_DRDSTATE_B_WAIT_BEGIN_HNP (0xBu << 0) /**< \brief (UOTGHS_FSM) In this state, the B-device is in suspend mode and waits until 3 ms before initiating the HNP protocol if requested. */
#define   UOTGHS_FSM_DRDSTATE_B_WAIT_DISCHARGE (0xCu << 0) /**< \brief (UOTGHS_FSM) In this state, the B-device waits for the data USB line to discharge (100 us) before becoming Host. */
#define   UOTGHS_FSM_DRDSTATE_B_WAIT_ACON (0xDu << 0) /**< \brief (UOTGHS_FSM) In this state, the B-device waits for the A-device to signal a connect before becoming B-Host. */
#define   UOTGHS_FSM_DRDSTATE_B_HOST (0xEu << 0) /**< \brief (UOTGHS_FSM) In this state, the B-device acts as the Host. */
#define   UOTGHS_FSM_DRDSTATE_B_SRP_INIT (0xFu << 0) /**< \brief (UOTGHS_FSM) In this state, the B-device attempts to start a session using the SRP protocol. */

/*@}*/


#endif /* _SAM3XA_UOTGHS_COMPONENT_ */
