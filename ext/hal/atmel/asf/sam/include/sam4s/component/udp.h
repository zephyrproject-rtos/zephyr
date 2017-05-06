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

#ifndef _SAM4S_UDP_COMPONENT_
#define _SAM4S_UDP_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR USB Device Port */
/* ============================================================================= */
/** \addtogroup SAM4S_UDP USB Device Port */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Udp hardware registers */
typedef struct {
  __I  uint32_t UDP_FRM_NUM;  /**< \brief (Udp Offset: 0x000) Frame Number Register */
  __IO uint32_t UDP_GLB_STAT; /**< \brief (Udp Offset: 0x004) Global State Register */
  __IO uint32_t UDP_FADDR;    /**< \brief (Udp Offset: 0x008) Function Address Register */
  __I  uint32_t Reserved1[1];
  __O  uint32_t UDP_IER;      /**< \brief (Udp Offset: 0x010) Interrupt Enable Register */
  __O  uint32_t UDP_IDR;      /**< \brief (Udp Offset: 0x014) Interrupt Disable Register */
  __I  uint32_t UDP_IMR;      /**< \brief (Udp Offset: 0x018) Interrupt Mask Register */
  __I  uint32_t UDP_ISR;      /**< \brief (Udp Offset: 0x01C) Interrupt Status Register */
  __O  uint32_t UDP_ICR;      /**< \brief (Udp Offset: 0x020) Interrupt Clear Register */
  __I  uint32_t Reserved2[1];
  __IO uint32_t UDP_RST_EP;   /**< \brief (Udp Offset: 0x028) Reset Endpoint Register */
  __I  uint32_t Reserved3[1];
  __IO uint32_t UDP_CSR[8];   /**< \brief (Udp Offset: 0x030) Endpoint Control and Status Register */
  __IO uint32_t UDP_FDR[8];   /**< \brief (Udp Offset: 0x050) Endpoint FIFO Data Register */
  __I  uint32_t Reserved4[1];
  __IO uint32_t UDP_TXVC;     /**< \brief (Udp Offset: 0x074) Transceiver Control Register */
} Udp;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- UDP_FRM_NUM : (UDP Offset: 0x000) Frame Number Register -------- */
#define UDP_FRM_NUM_FRM_NUM_Pos 0
#define UDP_FRM_NUM_FRM_NUM_Msk (0x7ffu << UDP_FRM_NUM_FRM_NUM_Pos) /**< \brief (UDP_FRM_NUM) Frame Number as Defined in the Packet Field Formats */
#define UDP_FRM_NUM_FRM_ERR (0x1u << 16) /**< \brief (UDP_FRM_NUM) Frame Error */
#define UDP_FRM_NUM_FRM_OK (0x1u << 17) /**< \brief (UDP_FRM_NUM) Frame OK */
/* -------- UDP_GLB_STAT : (UDP Offset: 0x004) Global State Register -------- */
#define UDP_GLB_STAT_FADDEN (0x1u << 0) /**< \brief (UDP_GLB_STAT) Function Address Enable */
#define UDP_GLB_STAT_CONFG (0x1u << 1) /**< \brief (UDP_GLB_STAT) Configured */
#define UDP_GLB_STAT_ESR (0x1u << 2) /**< \brief (UDP_GLB_STAT) Enable Send Resume */
#define UDP_GLB_STAT_RSMINPR (0x1u << 3) /**< \brief (UDP_GLB_STAT)  */
#define UDP_GLB_STAT_RMWUPE (0x1u << 4) /**< \brief (UDP_GLB_STAT) Remote Wakeup Enable */
/* -------- UDP_FADDR : (UDP Offset: 0x008) Function Address Register -------- */
#define UDP_FADDR_FADD_Pos 0
#define UDP_FADDR_FADD_Msk (0x7fu << UDP_FADDR_FADD_Pos) /**< \brief (UDP_FADDR) Function Address Value */
#define UDP_FADDR_FADD(value) ((UDP_FADDR_FADD_Msk & ((value) << UDP_FADDR_FADD_Pos)))
#define UDP_FADDR_FEN (0x1u << 8) /**< \brief (UDP_FADDR) Function Enable */
/* -------- UDP_IER : (UDP Offset: 0x010) Interrupt Enable Register -------- */
#define UDP_IER_EP0INT (0x1u << 0) /**< \brief (UDP_IER) Enable Endpoint 0 Interrupt */
#define UDP_IER_EP1INT (0x1u << 1) /**< \brief (UDP_IER) Enable Endpoint 1 Interrupt */
#define UDP_IER_EP2INT (0x1u << 2) /**< \brief (UDP_IER) Enable Endpoint 2Interrupt */
#define UDP_IER_EP3INT (0x1u << 3) /**< \brief (UDP_IER) Enable Endpoint 3 Interrupt */
#define UDP_IER_EP4INT (0x1u << 4) /**< \brief (UDP_IER) Enable Endpoint 4 Interrupt */
#define UDP_IER_EP5INT (0x1u << 5) /**< \brief (UDP_IER) Enable Endpoint 5 Interrupt */
#define UDP_IER_EP6INT (0x1u << 6) /**< \brief (UDP_IER) Enable Endpoint 6 Interrupt */
#define UDP_IER_EP7INT (0x1u << 7) /**< \brief (UDP_IER) Enable Endpoint 7 Interrupt */
#define UDP_IER_RXSUSP (0x1u << 8) /**< \brief (UDP_IER) Enable UDP Suspend Interrupt */
#define UDP_IER_RXRSM (0x1u << 9) /**< \brief (UDP_IER) Enable UDP Resume Interrupt */
#define UDP_IER_EXTRSM (0x1u << 10) /**< \brief (UDP_IER)  */
#define UDP_IER_SOFINT (0x1u << 11) /**< \brief (UDP_IER) Enable Start Of Frame Interrupt */
#define UDP_IER_WAKEUP (0x1u << 13) /**< \brief (UDP_IER) Enable UDP Bus Wakeup Interrupt */
/* -------- UDP_IDR : (UDP Offset: 0x014) Interrupt Disable Register -------- */
#define UDP_IDR_EP0INT (0x1u << 0) /**< \brief (UDP_IDR) Disable Endpoint 0 Interrupt */
#define UDP_IDR_EP1INT (0x1u << 1) /**< \brief (UDP_IDR) Disable Endpoint 1 Interrupt */
#define UDP_IDR_EP2INT (0x1u << 2) /**< \brief (UDP_IDR) Disable Endpoint 2 Interrupt */
#define UDP_IDR_EP3INT (0x1u << 3) /**< \brief (UDP_IDR) Disable Endpoint 3 Interrupt */
#define UDP_IDR_EP4INT (0x1u << 4) /**< \brief (UDP_IDR) Disable Endpoint 4 Interrupt */
#define UDP_IDR_EP5INT (0x1u << 5) /**< \brief (UDP_IDR) Disable Endpoint 5 Interrupt */
#define UDP_IDR_EP6INT (0x1u << 6) /**< \brief (UDP_IDR) Disable Endpoint 6 Interrupt */
#define UDP_IDR_EP7INT (0x1u << 7) /**< \brief (UDP_IDR) Disable Endpoint 7 Interrupt */
#define UDP_IDR_RXSUSP (0x1u << 8) /**< \brief (UDP_IDR) Disable UDP Suspend Interrupt */
#define UDP_IDR_RXRSM (0x1u << 9) /**< \brief (UDP_IDR) Disable UDP Resume Interrupt */
#define UDP_IDR_EXTRSM (0x1u << 10) /**< \brief (UDP_IDR)  */
#define UDP_IDR_SOFINT (0x1u << 11) /**< \brief (UDP_IDR) Disable Start Of Frame Interrupt */
#define UDP_IDR_WAKEUP (0x1u << 13) /**< \brief (UDP_IDR) Disable USB Bus Interrupt */
/* -------- UDP_IMR : (UDP Offset: 0x018) Interrupt Mask Register -------- */
#define UDP_IMR_EP0INT (0x1u << 0) /**< \brief (UDP_IMR) Mask Endpoint 0 Interrupt */
#define UDP_IMR_EP1INT (0x1u << 1) /**< \brief (UDP_IMR) Mask Endpoint 1 Interrupt */
#define UDP_IMR_EP2INT (0x1u << 2) /**< \brief (UDP_IMR) Mask Endpoint 2 Interrupt */
#define UDP_IMR_EP3INT (0x1u << 3) /**< \brief (UDP_IMR) Mask Endpoint 3 Interrupt */
#define UDP_IMR_EP4INT (0x1u << 4) /**< \brief (UDP_IMR) Mask Endpoint 4 Interrupt */
#define UDP_IMR_EP5INT (0x1u << 5) /**< \brief (UDP_IMR) Mask Endpoint 5 Interrupt */
#define UDP_IMR_EP6INT (0x1u << 6) /**< \brief (UDP_IMR) Mask Endpoint 6 Interrupt */
#define UDP_IMR_EP7INT (0x1u << 7) /**< \brief (UDP_IMR) Mask Endpoint 7 Interrupt */
#define UDP_IMR_RXSUSP (0x1u << 8) /**< \brief (UDP_IMR) Mask UDP Suspend Interrupt */
#define UDP_IMR_RXRSM (0x1u << 9) /**< \brief (UDP_IMR) Mask UDP Resume Interrupt. */
#define UDP_IMR_EXTRSM (0x1u << 10) /**< \brief (UDP_IMR)  */
#define UDP_IMR_SOFINT (0x1u << 11) /**< \brief (UDP_IMR) Mask Start Of Frame Interrupt */
#define UDP_IMR_BIT12 (0x1u << 12) /**< \brief (UDP_IMR) UDP_IMR Bit 12 */
#define UDP_IMR_WAKEUP (0x1u << 13) /**< \brief (UDP_IMR) USB Bus Wakeup Interrupt */
/* -------- UDP_ISR : (UDP Offset: 0x01C) Interrupt Status Register -------- */
#define UDP_ISR_EP0INT (0x1u << 0) /**< \brief (UDP_ISR) Endpoint 0 Interrupt Status */
#define UDP_ISR_EP1INT (0x1u << 1) /**< \brief (UDP_ISR) Endpoint 1 Interrupt Status */
#define UDP_ISR_EP2INT (0x1u << 2) /**< \brief (UDP_ISR) Endpoint 2 Interrupt Status */
#define UDP_ISR_EP3INT (0x1u << 3) /**< \brief (UDP_ISR) Endpoint 3 Interrupt Status */
#define UDP_ISR_EP4INT (0x1u << 4) /**< \brief (UDP_ISR) Endpoint 4 Interrupt Status */
#define UDP_ISR_EP5INT (0x1u << 5) /**< \brief (UDP_ISR) Endpoint 5 Interrupt Status */
#define UDP_ISR_EP6INT (0x1u << 6) /**< \brief (UDP_ISR) Endpoint 6 Interrupt Status */
#define UDP_ISR_EP7INT (0x1u << 7) /**< \brief (UDP_ISR) Endpoint 7Interrupt Status */
#define UDP_ISR_RXSUSP (0x1u << 8) /**< \brief (UDP_ISR) UDP Suspend Interrupt Status */
#define UDP_ISR_RXRSM (0x1u << 9) /**< \brief (UDP_ISR) UDP Resume Interrupt Status */
#define UDP_ISR_EXTRSM (0x1u << 10) /**< \brief (UDP_ISR)  */
#define UDP_ISR_SOFINT (0x1u << 11) /**< \brief (UDP_ISR) Start of Frame Interrupt Status */
#define UDP_ISR_ENDBUSRES (0x1u << 12) /**< \brief (UDP_ISR) End of BUS Reset Interrupt Status */
#define UDP_ISR_WAKEUP (0x1u << 13) /**< \brief (UDP_ISR) UDP Resume Interrupt Status */
/* -------- UDP_ICR : (UDP Offset: 0x020) Interrupt Clear Register -------- */
#define UDP_ICR_RXSUSP (0x1u << 8) /**< \brief (UDP_ICR) Clear UDP Suspend Interrupt */
#define UDP_ICR_RXRSM (0x1u << 9) /**< \brief (UDP_ICR) Clear UDP Resume Interrupt */
#define UDP_ICR_EXTRSM (0x1u << 10) /**< \brief (UDP_ICR)  */
#define UDP_ICR_SOFINT (0x1u << 11) /**< \brief (UDP_ICR) Clear Start Of Frame Interrupt */
#define UDP_ICR_ENDBUSRES (0x1u << 12) /**< \brief (UDP_ICR) Clear End of Bus Reset Interrupt */
#define UDP_ICR_WAKEUP (0x1u << 13) /**< \brief (UDP_ICR) Clear Wakeup Interrupt */
/* -------- UDP_RST_EP : (UDP Offset: 0x028) Reset Endpoint Register -------- */
#define UDP_RST_EP_EP0 (0x1u << 0) /**< \brief (UDP_RST_EP) Reset Endpoint 0 */
#define UDP_RST_EP_EP1 (0x1u << 1) /**< \brief (UDP_RST_EP) Reset Endpoint 1 */
#define UDP_RST_EP_EP2 (0x1u << 2) /**< \brief (UDP_RST_EP) Reset Endpoint 2 */
#define UDP_RST_EP_EP3 (0x1u << 3) /**< \brief (UDP_RST_EP) Reset Endpoint 3 */
#define UDP_RST_EP_EP4 (0x1u << 4) /**< \brief (UDP_RST_EP) Reset Endpoint 4 */
#define UDP_RST_EP_EP5 (0x1u << 5) /**< \brief (UDP_RST_EP) Reset Endpoint 5 */
#define UDP_RST_EP_EP6 (0x1u << 6) /**< \brief (UDP_RST_EP) Reset Endpoint 6 */
#define UDP_RST_EP_EP7 (0x1u << 7) /**< \brief (UDP_RST_EP) Reset Endpoint 7 */
/* -------- UDP_CSR[8] : (UDP Offset: 0x030) Endpoint Control and Status Register -------- */
#define UDP_CSR_TXCOMP (0x1u << 0) /**< \brief (UDP_CSR[8]) Generates an IN Packet with Data Previously Written in the DPR */
#define UDP_CSR_RX_DATA_BK0 (0x1u << 1) /**< \brief (UDP_CSR[8]) Receive Data Bank 0 */
#define UDP_CSR_RXSETUP (0x1u << 2) /**< \brief (UDP_CSR[8]) Received Setup */
#define UDP_CSR_STALLSENT (0x1u << 3) /**< \brief (UDP_CSR[8]) Stall Sent */
#define UDP_CSR_TXPKTRDY (0x1u << 4) /**< \brief (UDP_CSR[8]) Transmit Packet Ready */
#define UDP_CSR_FORCESTALL (0x1u << 5) /**< \brief (UDP_CSR[8]) Force Stall (used by Control, Bulk and Isochronous Endpoints) */
#define UDP_CSR_RX_DATA_BK1 (0x1u << 6) /**< \brief (UDP_CSR[8]) Receive Data Bank 1 (only used by endpoints with ping-pong attributes) */
#define UDP_CSR_DIR (0x1u << 7) /**< \brief (UDP_CSR[8]) Transfer Direction (only available for control endpoints) (Read/Write) */
#define UDP_CSR_EPTYPE_Pos 8
#define UDP_CSR_EPTYPE_Msk (0x7u << UDP_CSR_EPTYPE_Pos) /**< \brief (UDP_CSR[8]) Endpoint Type (Read/Write) */
#define UDP_CSR_EPTYPE(value) ((UDP_CSR_EPTYPE_Msk & ((value) << UDP_CSR_EPTYPE_Pos)))
#define   UDP_CSR_EPTYPE_CTRL (0x0u << 8) /**< \brief (UDP_CSR[8]) Control */
#define   UDP_CSR_EPTYPE_ISO_OUT (0x1u << 8) /**< \brief (UDP_CSR[8]) Isochronous OUT */
#define   UDP_CSR_EPTYPE_BULK_OUT (0x2u << 8) /**< \brief (UDP_CSR[8]) Bulk OUT */
#define   UDP_CSR_EPTYPE_INT_OUT (0x3u << 8) /**< \brief (UDP_CSR[8]) Interrupt OUT */
#define   UDP_CSR_EPTYPE_ISO_IN (0x5u << 8) /**< \brief (UDP_CSR[8]) Isochronous IN */
#define   UDP_CSR_EPTYPE_BULK_IN (0x6u << 8) /**< \brief (UDP_CSR[8]) Bulk IN */
#define   UDP_CSR_EPTYPE_INT_IN (0x7u << 8) /**< \brief (UDP_CSR[8]) Interrupt IN */
#define UDP_CSR_DTGLE (0x1u << 11) /**< \brief (UDP_CSR[8]) Data Toggle (Read-only) */
#define UDP_CSR_EPEDS (0x1u << 15) /**< \brief (UDP_CSR[8]) Endpoint Enable Disable */
#define UDP_CSR_RXBYTECNT_Pos 16
#define UDP_CSR_RXBYTECNT_Msk (0x7ffu << UDP_CSR_RXBYTECNT_Pos) /**< \brief (UDP_CSR[8]) Number of Bytes Available in the FIFO (Read-only) */
#define UDP_CSR_RXBYTECNT(value) ((UDP_CSR_RXBYTECNT_Msk & ((value) << UDP_CSR_RXBYTECNT_Pos)))
#define UDP_CSR_ISOERROR (0x1u << 3) /**< \brief (UDP_CSR[8]) A CRC error has been detected in an isochronous transfer */
/* -------- UDP_FDR[8] : (UDP Offset: 0x050) Endpoint FIFO Data Register -------- */
#define UDP_FDR_FIFO_DATA_Pos 0
#define UDP_FDR_FIFO_DATA_Msk (0xffu << UDP_FDR_FIFO_DATA_Pos) /**< \brief (UDP_FDR[8]) FIFO Data Value */
#define UDP_FDR_FIFO_DATA(value) ((UDP_FDR_FIFO_DATA_Msk & ((value) << UDP_FDR_FIFO_DATA_Pos)))
/* -------- UDP_TXVC : (UDP Offset: 0x074) Transceiver Control Register -------- */
#define UDP_TXVC_TXVDIS (0x1u << 8) /**< \brief (UDP_TXVC) Transceiver Disable */
#define UDP_TXVC_PUON (0x1u << 9) /**< \brief (UDP_TXVC) Pull-up On */

/*@}*/


#endif /* _SAM4S_UDP_COMPONENT_ */
