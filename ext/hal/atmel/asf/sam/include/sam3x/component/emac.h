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

#ifndef _SAM3XA_EMAC_COMPONENT_
#define _SAM3XA_EMAC_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Ethernet MAC 10/100 */
/* ============================================================================= */
/** \addtogroup SAM3XA_EMAC Ethernet MAC 10/100 */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief EmacSa hardware registers */
typedef struct {
  __IO uint32_t EMAC_SAxB; /**< \brief (EmacSa Offset: 0x0) Specific Address 1 Bottom Register */
  __IO uint32_t EMAC_SAxT; /**< \brief (EmacSa Offset: 0x4) Specific Address 1 Top Register */
} EmacSa;
/** \brief Emac hardware registers */
#define EMACSA_NUMBER 4
typedef struct {
  __IO uint32_t EMAC_NCR;               /**< \brief (Emac Offset: 0x00) Network Control Register */
  __IO uint32_t EMAC_NCFGR;             /**< \brief (Emac Offset: 0x04) Network Configuration Register */
  __I  uint32_t EMAC_NSR;               /**< \brief (Emac Offset: 0x08) Network Status Register */
  __I  uint32_t Reserved1[2];
  __IO uint32_t EMAC_TSR;               /**< \brief (Emac Offset: 0x14) Transmit Status Register */
  __IO uint32_t EMAC_RBQP;              /**< \brief (Emac Offset: 0x18) Receive Buffer Queue Pointer Register */
  __IO uint32_t EMAC_TBQP;              /**< \brief (Emac Offset: 0x1C) Transmit Buffer Queue Pointer Register */
  __IO uint32_t EMAC_RSR;               /**< \brief (Emac Offset: 0x20) Receive Status Register */
  __IO uint32_t EMAC_ISR;               /**< \brief (Emac Offset: 0x24) Interrupt Status Register */
  __O  uint32_t EMAC_IER;               /**< \brief (Emac Offset: 0x28) Interrupt Enable Register */
  __O  uint32_t EMAC_IDR;               /**< \brief (Emac Offset: 0x2C) Interrupt Disable Register */
  __I  uint32_t EMAC_IMR;               /**< \brief (Emac Offset: 0x30) Interrupt Mask Register */
  __IO uint32_t EMAC_MAN;               /**< \brief (Emac Offset: 0x34) Phy Maintenance Register */
  __IO uint32_t EMAC_PTR;               /**< \brief (Emac Offset: 0x38) Pause Time Register */
  __IO uint32_t EMAC_PFR;               /**< \brief (Emac Offset: 0x3C) Pause Frames Received Register */
  __IO uint32_t EMAC_FTO;               /**< \brief (Emac Offset: 0x40) Frames Transmitted Ok Register */
  __IO uint32_t EMAC_SCF;               /**< \brief (Emac Offset: 0x44) Single Collision Frames Register */
  __IO uint32_t EMAC_MCF;               /**< \brief (Emac Offset: 0x48) Multiple Collision Frames Register */
  __IO uint32_t EMAC_FRO;               /**< \brief (Emac Offset: 0x4C) Frames Received Ok Register */
  __IO uint32_t EMAC_FCSE;              /**< \brief (Emac Offset: 0x50) Frame Check Sequence Errors Register */
  __IO uint32_t EMAC_ALE;               /**< \brief (Emac Offset: 0x54) Alignment Errors Register */
  __IO uint32_t EMAC_DTF;               /**< \brief (Emac Offset: 0x58) Deferred Transmission Frames Register */
  __IO uint32_t EMAC_LCOL;              /**< \brief (Emac Offset: 0x5C) Late Collisions Register */
  __IO uint32_t EMAC_ECOL;              /**< \brief (Emac Offset: 0x60) Excessive Collisions Register */
  __IO uint32_t EMAC_TUND;              /**< \brief (Emac Offset: 0x64) Transmit Underrun Errors Register */
  __IO uint32_t EMAC_CSE;               /**< \brief (Emac Offset: 0x68) Carrier Sense Errors Register */
  __IO uint32_t EMAC_RRE;               /**< \brief (Emac Offset: 0x6C) Receive Resource Errors Register */
  __IO uint32_t EMAC_ROV;               /**< \brief (Emac Offset: 0x70) Receive Overrun Errors Register */
  __IO uint32_t EMAC_RSE;               /**< \brief (Emac Offset: 0x74) Receive Symbol Errors Register */
  __IO uint32_t EMAC_ELE;               /**< \brief (Emac Offset: 0x78) Excessive Length Errors Register */
  __IO uint32_t EMAC_RJA;               /**< \brief (Emac Offset: 0x7C) Receive Jabbers Register */
  __IO uint32_t EMAC_USF;               /**< \brief (Emac Offset: 0x80) Undersize Frames Register */
  __IO uint32_t EMAC_STE;               /**< \brief (Emac Offset: 0x84) SQE Test Errors Register */
  __IO uint32_t EMAC_RLE;               /**< \brief (Emac Offset: 0x88) Received Length Field Mismatch Register */
  __I  uint32_t Reserved2[1];
  __IO uint32_t EMAC_HRB;               /**< \brief (Emac Offset: 0x90) Hash Register Bottom [31:0] Register */
  __IO uint32_t EMAC_HRT;               /**< \brief (Emac Offset: 0x94) Hash Register Top [63:32] Register */
       EmacSa   EMAC_SA[EMACSA_NUMBER]; /**< \brief (Emac Offset: 0x98) sa = 1 .. 4 */
  __IO uint32_t EMAC_TID;               /**< \brief (Emac Offset: 0xB8) Type ID Checking Register */
  __I  uint32_t Reserved3[1];
  __IO uint32_t EMAC_USRIO;             /**< \brief (Emac Offset: 0xC0) User Input/Output Register */
} Emac;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- EMAC_NCR : (EMAC Offset: 0x00) Network Control Register -------- */
#define EMAC_NCR_LB (0x1u << 0) /**< \brief (EMAC_NCR) LoopBack */
#define EMAC_NCR_LLB (0x1u << 1) /**< \brief (EMAC_NCR) Loopback local */
#define EMAC_NCR_RE (0x1u << 2) /**< \brief (EMAC_NCR) Receive enable */
#define EMAC_NCR_TE (0x1u << 3) /**< \brief (EMAC_NCR) Transmit enable */
#define EMAC_NCR_MPE (0x1u << 4) /**< \brief (EMAC_NCR) Management port enable */
#define EMAC_NCR_CLRSTAT (0x1u << 5) /**< \brief (EMAC_NCR) Clear statistics registers */
#define EMAC_NCR_INCSTAT (0x1u << 6) /**< \brief (EMAC_NCR) Increment statistics registers */
#define EMAC_NCR_WESTAT (0x1u << 7) /**< \brief (EMAC_NCR) Write enable for statistics registers */
#define EMAC_NCR_BP (0x1u << 8) /**< \brief (EMAC_NCR) Back pressure */
#define EMAC_NCR_TSTART (0x1u << 9) /**< \brief (EMAC_NCR) Start transmission */
#define EMAC_NCR_THALT (0x1u << 10) /**< \brief (EMAC_NCR) Transmit halt */
/* -------- EMAC_NCFGR : (EMAC Offset: 0x04) Network Configuration Register -------- */
#define EMAC_NCFGR_SPD (0x1u << 0) /**< \brief (EMAC_NCFGR) Speed */
#define EMAC_NCFGR_FD (0x1u << 1) /**< \brief (EMAC_NCFGR) Full Duplex */
#define EMAC_NCFGR_JFRAME (0x1u << 3) /**< \brief (EMAC_NCFGR) Jumbo Frames */
#define EMAC_NCFGR_CAF (0x1u << 4) /**< \brief (EMAC_NCFGR) Copy All Frames */
#define EMAC_NCFGR_NBC (0x1u << 5) /**< \brief (EMAC_NCFGR) No Broadcast */
#define EMAC_NCFGR_MTI (0x1u << 6) /**< \brief (EMAC_NCFGR) Multicast Hash Enable */
#define EMAC_NCFGR_UNI (0x1u << 7) /**< \brief (EMAC_NCFGR) Unicast Hash Enable */
#define EMAC_NCFGR_BIG (0x1u << 8) /**< \brief (EMAC_NCFGR) Receive 1536 bytes frames */
#define EMAC_NCFGR_CLK_Pos 10
#define EMAC_NCFGR_CLK_Msk (0x3u << EMAC_NCFGR_CLK_Pos) /**< \brief (EMAC_NCFGR) MDC clock divider */
#define   EMAC_NCFGR_CLK_MCK_8 (0x0u << 10) /**< \brief (EMAC_NCFGR) MCK divided by 8 (MCK up to 20 MHz). */
#define   EMAC_NCFGR_CLK_MCK_16 (0x1u << 10) /**< \brief (EMAC_NCFGR) MCK divided by 16 (MCK up to 40 MHz). */
#define   EMAC_NCFGR_CLK_MCK_32 (0x2u << 10) /**< \brief (EMAC_NCFGR) MCK divided by 32 (MCK up to 80 MHz). */
#define   EMAC_NCFGR_CLK_MCK_64 (0x3u << 10) /**< \brief (EMAC_NCFGR) MCK divided by 64 (MCK up to 160 MHz). */
#define EMAC_NCFGR_RTY (0x1u << 12) /**< \brief (EMAC_NCFGR) Retry test */
#define EMAC_NCFGR_PAE (0x1u << 13) /**< \brief (EMAC_NCFGR) Pause Enable */
#define EMAC_NCFGR_RBOF_Pos 14
#define EMAC_NCFGR_RBOF_Msk (0x3u << EMAC_NCFGR_RBOF_Pos) /**< \brief (EMAC_NCFGR) Receive Buffer Offset */
#define   EMAC_NCFGR_RBOF_OFFSET_0 (0x0u << 14) /**< \brief (EMAC_NCFGR) No offset from start of receive buffer. */
#define   EMAC_NCFGR_RBOF_OFFSET_1 (0x1u << 14) /**< \brief (EMAC_NCFGR) One-byte offset from start of receive buffer. */
#define   EMAC_NCFGR_RBOF_OFFSET_2 (0x2u << 14) /**< \brief (EMAC_NCFGR) Two-byte offset from start of receive buffer. */
#define   EMAC_NCFGR_RBOF_OFFSET_3 (0x3u << 14) /**< \brief (EMAC_NCFGR) Three-byte offset from start of receive buffer. */
#define EMAC_NCFGR_RLCE (0x1u << 16) /**< \brief (EMAC_NCFGR) Receive Length field Checking Enable */
#define EMAC_NCFGR_DRFCS (0x1u << 17) /**< \brief (EMAC_NCFGR) Discard Receive FCS */
#define EMAC_NCFGR_EFRHD (0x1u << 18) /**< \brief (EMAC_NCFGR)  */
#define EMAC_NCFGR_IRXFCS (0x1u << 19) /**< \brief (EMAC_NCFGR) Ignore RX FCS */
/* -------- EMAC_NSR : (EMAC Offset: 0x08) Network Status Register -------- */
#define EMAC_NSR_MDIO (0x1u << 1) /**< \brief (EMAC_NSR)  */
#define EMAC_NSR_IDLE (0x1u << 2) /**< \brief (EMAC_NSR)  */
/* -------- EMAC_TSR : (EMAC Offset: 0x14) Transmit Status Register -------- */
#define EMAC_TSR_UBR (0x1u << 0) /**< \brief (EMAC_TSR) Used Bit Read */
#define EMAC_TSR_COL (0x1u << 1) /**< \brief (EMAC_TSR) Collision Occurred */
#define EMAC_TSR_RLES (0x1u << 2) /**< \brief (EMAC_TSR) Retry Limit exceeded */
#define EMAC_TSR_TGO (0x1u << 3) /**< \brief (EMAC_TSR) Transmit Go */
#define EMAC_TSR_BEX (0x1u << 4) /**< \brief (EMAC_TSR) Buffers exhausted mid frame */
#define EMAC_TSR_COMP (0x1u << 5) /**< \brief (EMAC_TSR) Transmit Complete */
#define EMAC_TSR_UND (0x1u << 6) /**< \brief (EMAC_TSR) Transmit Underrun */
/* -------- EMAC_RBQP : (EMAC Offset: 0x18) Receive Buffer Queue Pointer Register -------- */
#define EMAC_RBQP_ADDR_Pos 2
#define EMAC_RBQP_ADDR_Msk (0x3fffffffu << EMAC_RBQP_ADDR_Pos) /**< \brief (EMAC_RBQP) Receive buffer queue pointer address */
#define EMAC_RBQP_ADDR(value) ((EMAC_RBQP_ADDR_Msk & ((value) << EMAC_RBQP_ADDR_Pos)))
/* -------- EMAC_TBQP : (EMAC Offset: 0x1C) Transmit Buffer Queue Pointer Register -------- */
#define EMAC_TBQP_ADDR_Pos 2
#define EMAC_TBQP_ADDR_Msk (0x3fffffffu << EMAC_TBQP_ADDR_Pos) /**< \brief (EMAC_TBQP) Transmit buffer queue pointer address */
#define EMAC_TBQP_ADDR(value) ((EMAC_TBQP_ADDR_Msk & ((value) << EMAC_TBQP_ADDR_Pos)))
/* -------- EMAC_RSR : (EMAC Offset: 0x20) Receive Status Register -------- */
#define EMAC_RSR_BNA (0x1u << 0) /**< \brief (EMAC_RSR) Buffer Not Available */
#define EMAC_RSR_REC (0x1u << 1) /**< \brief (EMAC_RSR) Frame Received */
#define EMAC_RSR_OVR (0x1u << 2) /**< \brief (EMAC_RSR) Receive Overrun */
/* -------- EMAC_ISR : (EMAC Offset: 0x24) Interrupt Status Register -------- */
#define EMAC_ISR_MFD (0x1u << 0) /**< \brief (EMAC_ISR) Management Frame Done */
#define EMAC_ISR_RCOMP (0x1u << 1) /**< \brief (EMAC_ISR) Receive Complete */
#define EMAC_ISR_RXUBR (0x1u << 2) /**< \brief (EMAC_ISR) Receive Used Bit Read */
#define EMAC_ISR_TXUBR (0x1u << 3) /**< \brief (EMAC_ISR) Transmit Used Bit Read */
#define EMAC_ISR_TUND (0x1u << 4) /**< \brief (EMAC_ISR) Ethernet Transmit Buffer Underrun */
#define EMAC_ISR_RLEX (0x1u << 5) /**< \brief (EMAC_ISR) Retry Limit Exceeded */
#define EMAC_ISR_TXERR (0x1u << 6) /**< \brief (EMAC_ISR) Transmit Error */
#define EMAC_ISR_TCOMP (0x1u << 7) /**< \brief (EMAC_ISR) Transmit Complete */
#define EMAC_ISR_ROVR (0x1u << 10) /**< \brief (EMAC_ISR) Receive Overrun */
#define EMAC_ISR_HRESP (0x1u << 11) /**< \brief (EMAC_ISR) Hresp not OK */
#define EMAC_ISR_PFRE (0x1u << 12) /**< \brief (EMAC_ISR) Pause Frame Received */
#define EMAC_ISR_PTZ (0x1u << 13) /**< \brief (EMAC_ISR) Pause Time Zero */
/* -------- EMAC_IER : (EMAC Offset: 0x28) Interrupt Enable Register -------- */
#define EMAC_IER_MFD (0x1u << 0) /**< \brief (EMAC_IER) Management Frame sent */
#define EMAC_IER_RCOMP (0x1u << 1) /**< \brief (EMAC_IER) Receive Complete */
#define EMAC_IER_RXUBR (0x1u << 2) /**< \brief (EMAC_IER) Receive Used Bit Read */
#define EMAC_IER_TXUBR (0x1u << 3) /**< \brief (EMAC_IER) Transmit Used Bit Read */
#define EMAC_IER_TUND (0x1u << 4) /**< \brief (EMAC_IER) Ethernet Transmit Buffer Underrun */
#define EMAC_IER_RLE (0x1u << 5) /**< \brief (EMAC_IER) Retry Limit Exceeded */
#define EMAC_IER_TXERR (0x1u << 6) /**< \brief (EMAC_IER)  */
#define EMAC_IER_TCOMP (0x1u << 7) /**< \brief (EMAC_IER) Transmit Complete */
#define EMAC_IER_ROVR (0x1u << 10) /**< \brief (EMAC_IER) Receive Overrun */
#define EMAC_IER_HRESP (0x1u << 11) /**< \brief (EMAC_IER) Hresp not OK */
#define EMAC_IER_PFR (0x1u << 12) /**< \brief (EMAC_IER) Pause Frame Received */
#define EMAC_IER_PTZ (0x1u << 13) /**< \brief (EMAC_IER) Pause Time Zero */
/* -------- EMAC_IDR : (EMAC Offset: 0x2C) Interrupt Disable Register -------- */
#define EMAC_IDR_MFD (0x1u << 0) /**< \brief (EMAC_IDR) Management Frame sent */
#define EMAC_IDR_RCOMP (0x1u << 1) /**< \brief (EMAC_IDR) Receive Complete */
#define EMAC_IDR_RXUBR (0x1u << 2) /**< \brief (EMAC_IDR) Receive Used Bit Read */
#define EMAC_IDR_TXUBR (0x1u << 3) /**< \brief (EMAC_IDR) Transmit Used Bit Read */
#define EMAC_IDR_TUND (0x1u << 4) /**< \brief (EMAC_IDR) Ethernet Transmit Buffer Underrun */
#define EMAC_IDR_RLE (0x1u << 5) /**< \brief (EMAC_IDR) Retry Limit Exceeded */
#define EMAC_IDR_TXERR (0x1u << 6) /**< \brief (EMAC_IDR)  */
#define EMAC_IDR_TCOMP (0x1u << 7) /**< \brief (EMAC_IDR) Transmit Complete */
#define EMAC_IDR_ROVR (0x1u << 10) /**< \brief (EMAC_IDR) Receive Overrun */
#define EMAC_IDR_HRESP (0x1u << 11) /**< \brief (EMAC_IDR) Hresp not OK */
#define EMAC_IDR_PFR (0x1u << 12) /**< \brief (EMAC_IDR) Pause Frame Received */
#define EMAC_IDR_PTZ (0x1u << 13) /**< \brief (EMAC_IDR) Pause Time Zero */
/* -------- EMAC_IMR : (EMAC Offset: 0x30) Interrupt Mask Register -------- */
#define EMAC_IMR_MFD (0x1u << 0) /**< \brief (EMAC_IMR) Management Frame sent */
#define EMAC_IMR_RCOMP (0x1u << 1) /**< \brief (EMAC_IMR) Receive Complete */
#define EMAC_IMR_RXUBR (0x1u << 2) /**< \brief (EMAC_IMR) Receive Used Bit Read */
#define EMAC_IMR_TXUBR (0x1u << 3) /**< \brief (EMAC_IMR) Transmit Used Bit Read */
#define EMAC_IMR_TUND (0x1u << 4) /**< \brief (EMAC_IMR) Ethernet Transmit Buffer Underrun */
#define EMAC_IMR_RLE (0x1u << 5) /**< \brief (EMAC_IMR) Retry Limit Exceeded */
#define EMAC_IMR_TXERR (0x1u << 6) /**< \brief (EMAC_IMR)  */
#define EMAC_IMR_TCOMP (0x1u << 7) /**< \brief (EMAC_IMR) Transmit Complete */
#define EMAC_IMR_ROVR (0x1u << 10) /**< \brief (EMAC_IMR) Receive Overrun */
#define EMAC_IMR_HRESP (0x1u << 11) /**< \brief (EMAC_IMR) Hresp not OK */
#define EMAC_IMR_PFR (0x1u << 12) /**< \brief (EMAC_IMR) Pause Frame Received */
#define EMAC_IMR_PTZ (0x1u << 13) /**< \brief (EMAC_IMR) Pause Time Zero */
/* -------- EMAC_MAN : (EMAC Offset: 0x34) Phy Maintenance Register -------- */
#define EMAC_MAN_DATA_Pos 0
#define EMAC_MAN_DATA_Msk (0xffffu << EMAC_MAN_DATA_Pos) /**< \brief (EMAC_MAN)  */
#define EMAC_MAN_DATA(value) ((EMAC_MAN_DATA_Msk & ((value) << EMAC_MAN_DATA_Pos)))
#define EMAC_MAN_CODE_Pos 16
#define EMAC_MAN_CODE_Msk (0x3u << EMAC_MAN_CODE_Pos) /**< \brief (EMAC_MAN)  */
#define EMAC_MAN_CODE(value) ((EMAC_MAN_CODE_Msk & ((value) << EMAC_MAN_CODE_Pos)))
#define EMAC_MAN_REGA_Pos 18
#define EMAC_MAN_REGA_Msk (0x1fu << EMAC_MAN_REGA_Pos) /**< \brief (EMAC_MAN) Register Address */
#define EMAC_MAN_REGA(value) ((EMAC_MAN_REGA_Msk & ((value) << EMAC_MAN_REGA_Pos)))
#define EMAC_MAN_PHYA_Pos 23
#define EMAC_MAN_PHYA_Msk (0x1fu << EMAC_MAN_PHYA_Pos) /**< \brief (EMAC_MAN) PHY Address */
#define EMAC_MAN_PHYA(value) ((EMAC_MAN_PHYA_Msk & ((value) << EMAC_MAN_PHYA_Pos)))
#define EMAC_MAN_RW_Pos 28
#define EMAC_MAN_RW_Msk (0x3u << EMAC_MAN_RW_Pos) /**< \brief (EMAC_MAN) Read-write */
#define EMAC_MAN_RW(value) ((EMAC_MAN_RW_Msk & ((value) << EMAC_MAN_RW_Pos)))
#define EMAC_MAN_SOF_Pos 30
#define EMAC_MAN_SOF_Msk (0x3u << EMAC_MAN_SOF_Pos) /**< \brief (EMAC_MAN) Start of frame */
#define EMAC_MAN_SOF(value) ((EMAC_MAN_SOF_Msk & ((value) << EMAC_MAN_SOF_Pos)))
/* -------- EMAC_PTR : (EMAC Offset: 0x38) Pause Time Register -------- */
#define EMAC_PTR_PTIME_Pos 0
#define EMAC_PTR_PTIME_Msk (0xffffu << EMAC_PTR_PTIME_Pos) /**< \brief (EMAC_PTR) Pause Time */
#define EMAC_PTR_PTIME(value) ((EMAC_PTR_PTIME_Msk & ((value) << EMAC_PTR_PTIME_Pos)))
/* -------- EMAC_PFR : (EMAC Offset: 0x3C) Pause Frames Received Register -------- */
#define EMAC_PFR_FROK_Pos 0
#define EMAC_PFR_FROK_Msk (0xffffu << EMAC_PFR_FROK_Pos) /**< \brief (EMAC_PFR) Pause Frames received OK */
#define EMAC_PFR_FROK(value) ((EMAC_PFR_FROK_Msk & ((value) << EMAC_PFR_FROK_Pos)))
/* -------- EMAC_FTO : (EMAC Offset: 0x40) Frames Transmitted Ok Register -------- */
#define EMAC_FTO_FTOK_Pos 0
#define EMAC_FTO_FTOK_Msk (0xffffffu << EMAC_FTO_FTOK_Pos) /**< \brief (EMAC_FTO) Frames Transmitted OK */
#define EMAC_FTO_FTOK(value) ((EMAC_FTO_FTOK_Msk & ((value) << EMAC_FTO_FTOK_Pos)))
/* -------- EMAC_SCF : (EMAC Offset: 0x44) Single Collision Frames Register -------- */
#define EMAC_SCF_SCF_Pos 0
#define EMAC_SCF_SCF_Msk (0xffffu << EMAC_SCF_SCF_Pos) /**< \brief (EMAC_SCF) Single Collision Frames */
#define EMAC_SCF_SCF(value) ((EMAC_SCF_SCF_Msk & ((value) << EMAC_SCF_SCF_Pos)))
/* -------- EMAC_MCF : (EMAC Offset: 0x48) Multiple Collision Frames Register -------- */
#define EMAC_MCF_MCF_Pos 0
#define EMAC_MCF_MCF_Msk (0xffffu << EMAC_MCF_MCF_Pos) /**< \brief (EMAC_MCF) Multicollision Frames */
#define EMAC_MCF_MCF(value) ((EMAC_MCF_MCF_Msk & ((value) << EMAC_MCF_MCF_Pos)))
/* -------- EMAC_FRO : (EMAC Offset: 0x4C) Frames Received Ok Register -------- */
#define EMAC_FRO_FROK_Pos 0
#define EMAC_FRO_FROK_Msk (0xffffffu << EMAC_FRO_FROK_Pos) /**< \brief (EMAC_FRO) Frames Received OK */
#define EMAC_FRO_FROK(value) ((EMAC_FRO_FROK_Msk & ((value) << EMAC_FRO_FROK_Pos)))
/* -------- EMAC_FCSE : (EMAC Offset: 0x50) Frame Check Sequence Errors Register -------- */
#define EMAC_FCSE_FCSE_Pos 0
#define EMAC_FCSE_FCSE_Msk (0xffu << EMAC_FCSE_FCSE_Pos) /**< \brief (EMAC_FCSE) Frame Check Sequence Errors */
#define EMAC_FCSE_FCSE(value) ((EMAC_FCSE_FCSE_Msk & ((value) << EMAC_FCSE_FCSE_Pos)))
/* -------- EMAC_ALE : (EMAC Offset: 0x54) Alignment Errors Register -------- */
#define EMAC_ALE_ALE_Pos 0
#define EMAC_ALE_ALE_Msk (0xffu << EMAC_ALE_ALE_Pos) /**< \brief (EMAC_ALE) Alignment Errors */
#define EMAC_ALE_ALE(value) ((EMAC_ALE_ALE_Msk & ((value) << EMAC_ALE_ALE_Pos)))
/* -------- EMAC_DTF : (EMAC Offset: 0x58) Deferred Transmission Frames Register -------- */
#define EMAC_DTF_DTF_Pos 0
#define EMAC_DTF_DTF_Msk (0xffffu << EMAC_DTF_DTF_Pos) /**< \brief (EMAC_DTF) Deferred Transmission Frames */
#define EMAC_DTF_DTF(value) ((EMAC_DTF_DTF_Msk & ((value) << EMAC_DTF_DTF_Pos)))
/* -------- EMAC_LCOL : (EMAC Offset: 0x5C) Late Collisions Register -------- */
#define EMAC_LCOL_LCOL_Pos 0
#define EMAC_LCOL_LCOL_Msk (0xffu << EMAC_LCOL_LCOL_Pos) /**< \brief (EMAC_LCOL) Late Collisions */
#define EMAC_LCOL_LCOL(value) ((EMAC_LCOL_LCOL_Msk & ((value) << EMAC_LCOL_LCOL_Pos)))
/* -------- EMAC_ECOL : (EMAC Offset: 0x60) Excessive Collisions Register -------- */
#define EMAC_ECOL_EXCOL_Pos 0
#define EMAC_ECOL_EXCOL_Msk (0xffu << EMAC_ECOL_EXCOL_Pos) /**< \brief (EMAC_ECOL) Excessive Collisions */
#define EMAC_ECOL_EXCOL(value) ((EMAC_ECOL_EXCOL_Msk & ((value) << EMAC_ECOL_EXCOL_Pos)))
/* -------- EMAC_TUND : (EMAC Offset: 0x64) Transmit Underrun Errors Register -------- */
#define EMAC_TUND_TUND_Pos 0
#define EMAC_TUND_TUND_Msk (0xffu << EMAC_TUND_TUND_Pos) /**< \brief (EMAC_TUND) Transmit Underruns */
#define EMAC_TUND_TUND(value) ((EMAC_TUND_TUND_Msk & ((value) << EMAC_TUND_TUND_Pos)))
/* -------- EMAC_CSE : (EMAC Offset: 0x68) Carrier Sense Errors Register -------- */
#define EMAC_CSE_CSE_Pos 0
#define EMAC_CSE_CSE_Msk (0xffu << EMAC_CSE_CSE_Pos) /**< \brief (EMAC_CSE) Carrier Sense Errors */
#define EMAC_CSE_CSE(value) ((EMAC_CSE_CSE_Msk & ((value) << EMAC_CSE_CSE_Pos)))
/* -------- EMAC_RRE : (EMAC Offset: 0x6C) Receive Resource Errors Register -------- */
#define EMAC_RRE_RRE_Pos 0
#define EMAC_RRE_RRE_Msk (0xffffu << EMAC_RRE_RRE_Pos) /**< \brief (EMAC_RRE) Receive Resource Errors */
#define EMAC_RRE_RRE(value) ((EMAC_RRE_RRE_Msk & ((value) << EMAC_RRE_RRE_Pos)))
/* -------- EMAC_ROV : (EMAC Offset: 0x70) Receive Overrun Errors Register -------- */
#define EMAC_ROV_ROVR_Pos 0
#define EMAC_ROV_ROVR_Msk (0xffu << EMAC_ROV_ROVR_Pos) /**< \brief (EMAC_ROV) Receive Overrun */
#define EMAC_ROV_ROVR(value) ((EMAC_ROV_ROVR_Msk & ((value) << EMAC_ROV_ROVR_Pos)))
/* -------- EMAC_RSE : (EMAC Offset: 0x74) Receive Symbol Errors Register -------- */
#define EMAC_RSE_RSE_Pos 0
#define EMAC_RSE_RSE_Msk (0xffu << EMAC_RSE_RSE_Pos) /**< \brief (EMAC_RSE) Receive Symbol Errors */
#define EMAC_RSE_RSE(value) ((EMAC_RSE_RSE_Msk & ((value) << EMAC_RSE_RSE_Pos)))
/* -------- EMAC_ELE : (EMAC Offset: 0x78) Excessive Length Errors Register -------- */
#define EMAC_ELE_EXL_Pos 0
#define EMAC_ELE_EXL_Msk (0xffu << EMAC_ELE_EXL_Pos) /**< \brief (EMAC_ELE) Excessive Length Errors */
#define EMAC_ELE_EXL(value) ((EMAC_ELE_EXL_Msk & ((value) << EMAC_ELE_EXL_Pos)))
/* -------- EMAC_RJA : (EMAC Offset: 0x7C) Receive Jabbers Register -------- */
#define EMAC_RJA_RJB_Pos 0
#define EMAC_RJA_RJB_Msk (0xffu << EMAC_RJA_RJB_Pos) /**< \brief (EMAC_RJA) Receive Jabbers */
#define EMAC_RJA_RJB(value) ((EMAC_RJA_RJB_Msk & ((value) << EMAC_RJA_RJB_Pos)))
/* -------- EMAC_USF : (EMAC Offset: 0x80) Undersize Frames Register -------- */
#define EMAC_USF_USF_Pos 0
#define EMAC_USF_USF_Msk (0xffu << EMAC_USF_USF_Pos) /**< \brief (EMAC_USF) Undersize frames */
#define EMAC_USF_USF(value) ((EMAC_USF_USF_Msk & ((value) << EMAC_USF_USF_Pos)))
/* -------- EMAC_STE : (EMAC Offset: 0x84) SQE Test Errors Register -------- */
#define EMAC_STE_SQER_Pos 0
#define EMAC_STE_SQER_Msk (0xffu << EMAC_STE_SQER_Pos) /**< \brief (EMAC_STE) SQE test errors */
#define EMAC_STE_SQER(value) ((EMAC_STE_SQER_Msk & ((value) << EMAC_STE_SQER_Pos)))
/* -------- EMAC_RLE : (EMAC Offset: 0x88) Received Length Field Mismatch Register -------- */
#define EMAC_RLE_RLFM_Pos 0
#define EMAC_RLE_RLFM_Msk (0xffu << EMAC_RLE_RLFM_Pos) /**< \brief (EMAC_RLE) Receive Length Field Mismatch */
#define EMAC_RLE_RLFM(value) ((EMAC_RLE_RLFM_Msk & ((value) << EMAC_RLE_RLFM_Pos)))
/* -------- EMAC_HRB : (EMAC Offset: 0x90) Hash Register Bottom [31:0] Register -------- */
#define EMAC_HRB_ADDR_Pos 0
#define EMAC_HRB_ADDR_Msk (0xffffffffu << EMAC_HRB_ADDR_Pos) /**< \brief (EMAC_HRB)  */
#define EMAC_HRB_ADDR(value) ((EMAC_HRB_ADDR_Msk & ((value) << EMAC_HRB_ADDR_Pos)))
/* -------- EMAC_HRT : (EMAC Offset: 0x94) Hash Register Top [63:32] Register -------- */
#define EMAC_HRT_ADDR_Pos 0
#define EMAC_HRT_ADDR_Msk (0xffffffffu << EMAC_HRT_ADDR_Pos) /**< \brief (EMAC_HRT)  */
#define EMAC_HRT_ADDR(value) ((EMAC_HRT_ADDR_Msk & ((value) << EMAC_HRT_ADDR_Pos)))
/* -------- EMAC_SAxB : (EMAC Offset: N/A) Specific Address 1 Bottom Register -------- */
#define EMAC_SAxB_ADDR_Pos 0
#define EMAC_SAxB_ADDR_Msk (0xffffffffu << EMAC_SAxB_ADDR_Pos) /**< \brief (EMAC_SAxB)  */
#define EMAC_SAxB_ADDR(value) ((EMAC_SAxB_ADDR_Msk & ((value) << EMAC_SAxB_ADDR_Pos)))
/* -------- EMAC_SAxT : (EMAC Offset: N/A) Specific Address 1 Top Register -------- */
#define EMAC_SAxT_ADDR_Pos 0
#define EMAC_SAxT_ADDR_Msk (0xffffu << EMAC_SAxT_ADDR_Pos) /**< \brief (EMAC_SAxT)  */
#define EMAC_SAxT_ADDR(value) ((EMAC_SAxT_ADDR_Msk & ((value) << EMAC_SAxT_ADDR_Pos)))
/* -------- EMAC_TID : (EMAC Offset: 0xB8) Type ID Checking Register -------- */
#define EMAC_TID_TID_Pos 0
#define EMAC_TID_TID_Msk (0xffffu << EMAC_TID_TID_Pos) /**< \brief (EMAC_TID) Type ID checking */
#define EMAC_TID_TID(value) ((EMAC_TID_TID_Msk & ((value) << EMAC_TID_TID_Pos)))
/* -------- EMAC_USRIO : (EMAC Offset: 0xC0) User Input/Output Register -------- */
#define EMAC_USRIO_RMII (0x1u << 0) /**< \brief (EMAC_USRIO) Reduce MII */
#define EMAC_USRIO_CLKEN (0x1u << 1) /**< \brief (EMAC_USRIO) Clock Enable */

/*@}*/


#endif /* _SAM3XA_EMAC_COMPONENT_ */
