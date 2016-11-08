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

#ifndef _SAM3XA_UOTGHS_INSTANCE_
#define _SAM3XA_UOTGHS_INSTANCE_

/* ========== Register definition for UOTGHS peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
  #define REG_UOTGHS_DEVCTRL                         (0x400AC000U) /**< \brief (UOTGHS) Device General Control Register */
  #define REG_UOTGHS_DEVISR                          (0x400AC004U) /**< \brief (UOTGHS) Device Global Interrupt Status Register */
  #define REG_UOTGHS_DEVICR                          (0x400AC008U) /**< \brief (UOTGHS) Device Global Interrupt Clear Register */
  #define REG_UOTGHS_DEVIFR                          (0x400AC00CU) /**< \brief (UOTGHS) Device Global Interrupt Set Register */
  #define REG_UOTGHS_DEVIMR                          (0x400AC010U) /**< \brief (UOTGHS) Device Global Interrupt Mask Register */
  #define REG_UOTGHS_DEVIDR                          (0x400AC014U) /**< \brief (UOTGHS) Device Global Interrupt Disable Register */
  #define REG_UOTGHS_DEVIER                          (0x400AC018U) /**< \brief (UOTGHS) Device Global Interrupt Enable Register */
  #define REG_UOTGHS_DEVEPT                          (0x400AC01CU) /**< \brief (UOTGHS) Device Endpoint Register */
  #define REG_UOTGHS_DEVFNUM                         (0x400AC020U) /**< \brief (UOTGHS) Device Frame Number Register */
  #define REG_UOTGHS_DEVEPTCFG                       (0x400AC100U) /**< \brief (UOTGHS) Device Endpoint Configuration Register (n = 0) */
  #define REG_UOTGHS_DEVEPTISR                       (0x400AC130U) /**< \brief (UOTGHS) Device Endpoint Status Register (n = 0) */
  #define REG_UOTGHS_DEVEPTICR                       (0x400AC160U) /**< \brief (UOTGHS) Device Endpoint Clear Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIFR                       (0x400AC190U) /**< \brief (UOTGHS) Device Endpoint Set Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIMR                       (0x400AC1C0U) /**< \brief (UOTGHS) Device Endpoint Mask Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIER                       (0x400AC1F0U) /**< \brief (UOTGHS) Device Endpoint Enable Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIDR                       (0x400AC220U) /**< \brief (UOTGHS) Device Endpoint Disable Register (n = 0) */
  #define REG_UOTGHS_DEVDMANXTDSC1                   (0x400AC310U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 1) */
  #define REG_UOTGHS_DEVDMAADDRESS1                  (0x400AC314U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 1) */
  #define REG_UOTGHS_DEVDMACONTROL1                  (0x400AC318U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 1) */
  #define REG_UOTGHS_DEVDMASTATUS1                   (0x400AC31CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 1) */
  #define REG_UOTGHS_DEVDMANXTDSC2                   (0x400AC320U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 2) */
  #define REG_UOTGHS_DEVDMAADDRESS2                  (0x400AC324U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 2) */
  #define REG_UOTGHS_DEVDMACONTROL2                  (0x400AC328U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 2) */
  #define REG_UOTGHS_DEVDMASTATUS2                   (0x400AC32CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 2) */
  #define REG_UOTGHS_DEVDMANXTDSC3                   (0x400AC330U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 3) */
  #define REG_UOTGHS_DEVDMAADDRESS3                  (0x400AC334U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 3) */
  #define REG_UOTGHS_DEVDMACONTROL3                  (0x400AC338U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 3) */
  #define REG_UOTGHS_DEVDMASTATUS3                   (0x400AC33CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 3) */
  #define REG_UOTGHS_DEVDMANXTDSC4                   (0x400AC340U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 4) */
  #define REG_UOTGHS_DEVDMAADDRESS4                  (0x400AC344U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 4) */
  #define REG_UOTGHS_DEVDMACONTROL4                  (0x400AC348U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 4) */
  #define REG_UOTGHS_DEVDMASTATUS4                   (0x400AC34CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 4) */
  #define REG_UOTGHS_DEVDMANXTDSC5                   (0x400AC350U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 5) */
  #define REG_UOTGHS_DEVDMAADDRESS5                  (0x400AC354U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 5) */
  #define REG_UOTGHS_DEVDMACONTROL5                  (0x400AC358U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 5) */
  #define REG_UOTGHS_DEVDMASTATUS5                   (0x400AC35CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 5) */
  #define REG_UOTGHS_DEVDMANXTDSC6                   (0x400AC360U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 6) */
  #define REG_UOTGHS_DEVDMAADDRESS6                  (0x400AC364U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 6) */
  #define REG_UOTGHS_DEVDMACONTROL6                  (0x400AC368U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 6) */
  #define REG_UOTGHS_DEVDMASTATUS6                   (0x400AC36CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 6) */
  #define REG_UOTGHS_DEVDMANXTDSC7                   (0x400AC370U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 7) */
  #define REG_UOTGHS_DEVDMAADDRESS7                  (0x400AC374U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 7) */
  #define REG_UOTGHS_DEVDMACONTROL7                  (0x400AC378U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 7) */
  #define REG_UOTGHS_DEVDMASTATUS7                   (0x400AC37CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 7) */
  #define REG_UOTGHS_HSTCTRL                         (0x400AC400U) /**< \brief (UOTGHS) Host General Control Register */
  #define REG_UOTGHS_HSTISR                          (0x400AC404U) /**< \brief (UOTGHS) Host Global Interrupt Status Register */
  #define REG_UOTGHS_HSTICR                          (0x400AC408U) /**< \brief (UOTGHS) Host Global Interrupt Clear Register */
  #define REG_UOTGHS_HSTIFR                          (0x400AC40CU) /**< \brief (UOTGHS) Host Global Interrupt Set Register */
  #define REG_UOTGHS_HSTIMR                          (0x400AC410U) /**< \brief (UOTGHS) Host Global Interrupt Mask Register */
  #define REG_UOTGHS_HSTIDR                          (0x400AC414U) /**< \brief (UOTGHS) Host Global Interrupt Disable Register */
  #define REG_UOTGHS_HSTIER                          (0x400AC418U) /**< \brief (UOTGHS) Host Global Interrupt Enable Register */
  #define REG_UOTGHS_HSTPIP                          (0x400AC41CU) /**< \brief (UOTGHS) Host Pipe Register */
  #define REG_UOTGHS_HSTFNUM                         (0x400AC420U) /**< \brief (UOTGHS) Host Frame Number Register */
  #define REG_UOTGHS_HSTADDR1                        (0x400AC424U) /**< \brief (UOTGHS) Host Address 1 Register */
  #define REG_UOTGHS_HSTADDR2                        (0x400AC428U) /**< \brief (UOTGHS) Host Address 2 Register */
  #define REG_UOTGHS_HSTADDR3                        (0x400AC42CU) /**< \brief (UOTGHS) Host Address 3 Register */
  #define REG_UOTGHS_HSTPIPCFG                       (0x400AC500U) /**< \brief (UOTGHS) Host Pipe Configuration Register (n = 0) */
  #define REG_UOTGHS_HSTPIPISR                       (0x400AC530U) /**< \brief (UOTGHS) Host Pipe Status Register (n = 0) */
  #define REG_UOTGHS_HSTPIPICR                       (0x400AC560U) /**< \brief (UOTGHS) Host Pipe Clear Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIFR                       (0x400AC590U) /**< \brief (UOTGHS) Host Pipe Set Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIMR                       (0x400AC5C0U) /**< \brief (UOTGHS) Host Pipe Mask Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIER                       (0x400AC5F0U) /**< \brief (UOTGHS) Host Pipe Enable Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIDR                       (0x400AC620U) /**< \brief (UOTGHS) Host Pipe Disable Register (n = 0) */
  #define REG_UOTGHS_HSTPIPINRQ                      (0x400AC650U) /**< \brief (UOTGHS) Host Pipe IN Request Register (n = 0) */
  #define REG_UOTGHS_HSTPIPERR                       (0x400AC680U) /**< \brief (UOTGHS) Host Pipe Error Register (n = 0) */
  #define REG_UOTGHS_HSTDMANXTDSC1                   (0x400AC710U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 1) */
  #define REG_UOTGHS_HSTDMAADDRESS1                  (0x400AC714U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 1) */
  #define REG_UOTGHS_HSTDMACONTROL1                  (0x400AC718U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 1) */
  #define REG_UOTGHS_HSTDMASTATUS1                   (0x400AC71CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 1) */
  #define REG_UOTGHS_HSTDMANXTDSC2                   (0x400AC720U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 2) */
  #define REG_UOTGHS_HSTDMAADDRESS2                  (0x400AC724U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 2) */
  #define REG_UOTGHS_HSTDMACONTROL2                  (0x400AC728U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 2) */
  #define REG_UOTGHS_HSTDMASTATUS2                   (0x400AC72CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 2) */
  #define REG_UOTGHS_HSTDMANXTDSC3                   (0x400AC730U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 3) */
  #define REG_UOTGHS_HSTDMAADDRESS3                  (0x400AC734U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 3) */
  #define REG_UOTGHS_HSTDMACONTROL3                  (0x400AC738U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 3) */
  #define REG_UOTGHS_HSTDMASTATUS3                   (0x400AC73CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 3) */
  #define REG_UOTGHS_HSTDMANXTDSC4                   (0x400AC740U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 4) */
  #define REG_UOTGHS_HSTDMAADDRESS4                  (0x400AC744U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 4) */
  #define REG_UOTGHS_HSTDMACONTROL4                  (0x400AC748U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 4) */
  #define REG_UOTGHS_HSTDMASTATUS4                   (0x400AC74CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 4) */
  #define REG_UOTGHS_HSTDMANXTDSC5                   (0x400AC750U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 5) */
  #define REG_UOTGHS_HSTDMAADDRESS5                  (0x400AC754U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 5) */
  #define REG_UOTGHS_HSTDMACONTROL5                  (0x400AC758U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 5) */
  #define REG_UOTGHS_HSTDMASTATUS5                   (0x400AC75CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 5) */
  #define REG_UOTGHS_HSTDMANXTDSC6                   (0x400AC760U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 6) */
  #define REG_UOTGHS_HSTDMAADDRESS6                  (0x400AC764U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 6) */
  #define REG_UOTGHS_HSTDMACONTROL6                  (0x400AC768U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 6) */
  #define REG_UOTGHS_HSTDMASTATUS6                   (0x400AC76CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 6) */
  #define REG_UOTGHS_HSTDMANXTDSC7                   (0x400AC770U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 7) */
  #define REG_UOTGHS_HSTDMAADDRESS7                  (0x400AC774U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 7) */
  #define REG_UOTGHS_HSTDMACONTROL7                  (0x400AC778U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 7) */
  #define REG_UOTGHS_HSTDMASTATUS7                   (0x400AC77CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 7) */
  #define REG_UOTGHS_CTRL                            (0x400AC800U) /**< \brief (UOTGHS) General Control Register */
  #define REG_UOTGHS_SR                              (0x400AC804U) /**< \brief (UOTGHS) General Status Register */
  #define REG_UOTGHS_SCR                             (0x400AC808U) /**< \brief (UOTGHS) General Status Clear Register */
  #define REG_UOTGHS_SFR                             (0x400AC80CU) /**< \brief (UOTGHS) General Status Set Register */
  #define REG_UOTGHS_FSM                             (0x400AC82CU) /**< \brief (UOTGHS) General Finite State Machine Register */
#else
  #define REG_UOTGHS_DEVCTRL        (*(__IO uint32_t*)0x400AC000U) /**< \brief (UOTGHS) Device General Control Register */
  #define REG_UOTGHS_DEVISR         (*(__I  uint32_t*)0x400AC004U) /**< \brief (UOTGHS) Device Global Interrupt Status Register */
  #define REG_UOTGHS_DEVICR         (*(__O  uint32_t*)0x400AC008U) /**< \brief (UOTGHS) Device Global Interrupt Clear Register */
  #define REG_UOTGHS_DEVIFR         (*(__O  uint32_t*)0x400AC00CU) /**< \brief (UOTGHS) Device Global Interrupt Set Register */
  #define REG_UOTGHS_DEVIMR         (*(__I  uint32_t*)0x400AC010U) /**< \brief (UOTGHS) Device Global Interrupt Mask Register */
  #define REG_UOTGHS_DEVIDR         (*(__O  uint32_t*)0x400AC014U) /**< \brief (UOTGHS) Device Global Interrupt Disable Register */
  #define REG_UOTGHS_DEVIER         (*(__O  uint32_t*)0x400AC018U) /**< \brief (UOTGHS) Device Global Interrupt Enable Register */
  #define REG_UOTGHS_DEVEPT         (*(__IO uint32_t*)0x400AC01CU) /**< \brief (UOTGHS) Device Endpoint Register */
  #define REG_UOTGHS_DEVFNUM        (*(__I  uint32_t*)0x400AC020U) /**< \brief (UOTGHS) Device Frame Number Register */
  #define REG_UOTGHS_DEVEPTCFG      (*(__IO uint32_t*)0x400AC100U) /**< \brief (UOTGHS) Device Endpoint Configuration Register (n = 0) */
  #define REG_UOTGHS_DEVEPTISR      (*(__I  uint32_t*)0x400AC130U) /**< \brief (UOTGHS) Device Endpoint Status Register (n = 0) */
  #define REG_UOTGHS_DEVEPTICR      (*(__O  uint32_t*)0x400AC160U) /**< \brief (UOTGHS) Device Endpoint Clear Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIFR      (*(__O  uint32_t*)0x400AC190U) /**< \brief (UOTGHS) Device Endpoint Set Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIMR      (*(__I  uint32_t*)0x400AC1C0U) /**< \brief (UOTGHS) Device Endpoint Mask Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIER      (*(__O  uint32_t*)0x400AC1F0U) /**< \brief (UOTGHS) Device Endpoint Enable Register (n = 0) */
  #define REG_UOTGHS_DEVEPTIDR      (*(__O  uint32_t*)0x400AC220U) /**< \brief (UOTGHS) Device Endpoint Disable Register (n = 0) */
  #define REG_UOTGHS_DEVDMANXTDSC1  (*(__IO uint32_t*)0x400AC310U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 1) */
  #define REG_UOTGHS_DEVDMAADDRESS1 (*(__IO uint32_t*)0x400AC314U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 1) */
  #define REG_UOTGHS_DEVDMACONTROL1 (*(__IO uint32_t*)0x400AC318U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 1) */
  #define REG_UOTGHS_DEVDMASTATUS1  (*(__IO uint32_t*)0x400AC31CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 1) */
  #define REG_UOTGHS_DEVDMANXTDSC2  (*(__IO uint32_t*)0x400AC320U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 2) */
  #define REG_UOTGHS_DEVDMAADDRESS2 (*(__IO uint32_t*)0x400AC324U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 2) */
  #define REG_UOTGHS_DEVDMACONTROL2 (*(__IO uint32_t*)0x400AC328U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 2) */
  #define REG_UOTGHS_DEVDMASTATUS2  (*(__IO uint32_t*)0x400AC32CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 2) */
  #define REG_UOTGHS_DEVDMANXTDSC3  (*(__IO uint32_t*)0x400AC330U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 3) */
  #define REG_UOTGHS_DEVDMAADDRESS3 (*(__IO uint32_t*)0x400AC334U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 3) */
  #define REG_UOTGHS_DEVDMACONTROL3 (*(__IO uint32_t*)0x400AC338U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 3) */
  #define REG_UOTGHS_DEVDMASTATUS3  (*(__IO uint32_t*)0x400AC33CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 3) */
  #define REG_UOTGHS_DEVDMANXTDSC4  (*(__IO uint32_t*)0x400AC340U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 4) */
  #define REG_UOTGHS_DEVDMAADDRESS4 (*(__IO uint32_t*)0x400AC344U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 4) */
  #define REG_UOTGHS_DEVDMACONTROL4 (*(__IO uint32_t*)0x400AC348U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 4) */
  #define REG_UOTGHS_DEVDMASTATUS4  (*(__IO uint32_t*)0x400AC34CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 4) */
  #define REG_UOTGHS_DEVDMANXTDSC5  (*(__IO uint32_t*)0x400AC350U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 5) */
  #define REG_UOTGHS_DEVDMAADDRESS5 (*(__IO uint32_t*)0x400AC354U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 5) */
  #define REG_UOTGHS_DEVDMACONTROL5 (*(__IO uint32_t*)0x400AC358U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 5) */
  #define REG_UOTGHS_DEVDMASTATUS5  (*(__IO uint32_t*)0x400AC35CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 5) */
  #define REG_UOTGHS_DEVDMANXTDSC6  (*(__IO uint32_t*)0x400AC360U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 6) */
  #define REG_UOTGHS_DEVDMAADDRESS6 (*(__IO uint32_t*)0x400AC364U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 6) */
  #define REG_UOTGHS_DEVDMACONTROL6 (*(__IO uint32_t*)0x400AC368U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 6) */
  #define REG_UOTGHS_DEVDMASTATUS6  (*(__IO uint32_t*)0x400AC36CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 6) */
  #define REG_UOTGHS_DEVDMANXTDSC7  (*(__IO uint32_t*)0x400AC370U) /**< \brief (UOTGHS) Device DMA Channel Next Descriptor Address Register (n = 7) */
  #define REG_UOTGHS_DEVDMAADDRESS7 (*(__IO uint32_t*)0x400AC374U) /**< \brief (UOTGHS) Device DMA Channel Address Register (n = 7) */
  #define REG_UOTGHS_DEVDMACONTROL7 (*(__IO uint32_t*)0x400AC378U) /**< \brief (UOTGHS) Device DMA Channel Control Register (n = 7) */
  #define REG_UOTGHS_DEVDMASTATUS7  (*(__IO uint32_t*)0x400AC37CU) /**< \brief (UOTGHS) Device DMA Channel Status Register (n = 7) */
  #define REG_UOTGHS_HSTCTRL        (*(__IO uint32_t*)0x400AC400U) /**< \brief (UOTGHS) Host General Control Register */
  #define REG_UOTGHS_HSTISR         (*(__I  uint32_t*)0x400AC404U) /**< \brief (UOTGHS) Host Global Interrupt Status Register */
  #define REG_UOTGHS_HSTICR         (*(__O  uint32_t*)0x400AC408U) /**< \brief (UOTGHS) Host Global Interrupt Clear Register */
  #define REG_UOTGHS_HSTIFR         (*(__O  uint32_t*)0x400AC40CU) /**< \brief (UOTGHS) Host Global Interrupt Set Register */
  #define REG_UOTGHS_HSTIMR         (*(__I  uint32_t*)0x400AC410U) /**< \brief (UOTGHS) Host Global Interrupt Mask Register */
  #define REG_UOTGHS_HSTIDR         (*(__O  uint32_t*)0x400AC414U) /**< \brief (UOTGHS) Host Global Interrupt Disable Register */
  #define REG_UOTGHS_HSTIER         (*(__O  uint32_t*)0x400AC418U) /**< \brief (UOTGHS) Host Global Interrupt Enable Register */
  #define REG_UOTGHS_HSTPIP         (*(__IO uint32_t*)0x400AC41CU) /**< \brief (UOTGHS) Host Pipe Register */
  #define REG_UOTGHS_HSTFNUM        (*(__IO uint32_t*)0x400AC420U) /**< \brief (UOTGHS) Host Frame Number Register */
  #define REG_UOTGHS_HSTADDR1       (*(__IO uint32_t*)0x400AC424U) /**< \brief (UOTGHS) Host Address 1 Register */
  #define REG_UOTGHS_HSTADDR2       (*(__IO uint32_t*)0x400AC428U) /**< \brief (UOTGHS) Host Address 2 Register */
  #define REG_UOTGHS_HSTADDR3       (*(__IO uint32_t*)0x400AC42CU) /**< \brief (UOTGHS) Host Address 3 Register */
  #define REG_UOTGHS_HSTPIPCFG      (*(__IO uint32_t*)0x400AC500U) /**< \brief (UOTGHS) Host Pipe Configuration Register (n = 0) */
  #define REG_UOTGHS_HSTPIPISR      (*(__I  uint32_t*)0x400AC530U) /**< \brief (UOTGHS) Host Pipe Status Register (n = 0) */
  #define REG_UOTGHS_HSTPIPICR      (*(__O  uint32_t*)0x400AC560U) /**< \brief (UOTGHS) Host Pipe Clear Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIFR      (*(__O  uint32_t*)0x400AC590U) /**< \brief (UOTGHS) Host Pipe Set Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIMR      (*(__I  uint32_t*)0x400AC5C0U) /**< \brief (UOTGHS) Host Pipe Mask Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIER      (*(__O  uint32_t*)0x400AC5F0U) /**< \brief (UOTGHS) Host Pipe Enable Register (n = 0) */
  #define REG_UOTGHS_HSTPIPIDR      (*(__O  uint32_t*)0x400AC620U) /**< \brief (UOTGHS) Host Pipe Disable Register (n = 0) */
  #define REG_UOTGHS_HSTPIPINRQ     (*(__IO uint32_t*)0x400AC650U) /**< \brief (UOTGHS) Host Pipe IN Request Register (n = 0) */
  #define REG_UOTGHS_HSTPIPERR      (*(__IO uint32_t*)0x400AC680U) /**< \brief (UOTGHS) Host Pipe Error Register (n = 0) */
  #define REG_UOTGHS_HSTDMANXTDSC1  (*(__IO uint32_t*)0x400AC710U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 1) */
  #define REG_UOTGHS_HSTDMAADDRESS1 (*(__IO uint32_t*)0x400AC714U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 1) */
  #define REG_UOTGHS_HSTDMACONTROL1 (*(__IO uint32_t*)0x400AC718U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 1) */
  #define REG_UOTGHS_HSTDMASTATUS1  (*(__IO uint32_t*)0x400AC71CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 1) */
  #define REG_UOTGHS_HSTDMANXTDSC2  (*(__IO uint32_t*)0x400AC720U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 2) */
  #define REG_UOTGHS_HSTDMAADDRESS2 (*(__IO uint32_t*)0x400AC724U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 2) */
  #define REG_UOTGHS_HSTDMACONTROL2 (*(__IO uint32_t*)0x400AC728U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 2) */
  #define REG_UOTGHS_HSTDMASTATUS2  (*(__IO uint32_t*)0x400AC72CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 2) */
  #define REG_UOTGHS_HSTDMANXTDSC3  (*(__IO uint32_t*)0x400AC730U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 3) */
  #define REG_UOTGHS_HSTDMAADDRESS3 (*(__IO uint32_t*)0x400AC734U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 3) */
  #define REG_UOTGHS_HSTDMACONTROL3 (*(__IO uint32_t*)0x400AC738U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 3) */
  #define REG_UOTGHS_HSTDMASTATUS3  (*(__IO uint32_t*)0x400AC73CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 3) */
  #define REG_UOTGHS_HSTDMANXTDSC4  (*(__IO uint32_t*)0x400AC740U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 4) */
  #define REG_UOTGHS_HSTDMAADDRESS4 (*(__IO uint32_t*)0x400AC744U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 4) */
  #define REG_UOTGHS_HSTDMACONTROL4 (*(__IO uint32_t*)0x400AC748U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 4) */
  #define REG_UOTGHS_HSTDMASTATUS4  (*(__IO uint32_t*)0x400AC74CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 4) */
  #define REG_UOTGHS_HSTDMANXTDSC5  (*(__IO uint32_t*)0x400AC750U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 5) */
  #define REG_UOTGHS_HSTDMAADDRESS5 (*(__IO uint32_t*)0x400AC754U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 5) */
  #define REG_UOTGHS_HSTDMACONTROL5 (*(__IO uint32_t*)0x400AC758U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 5) */
  #define REG_UOTGHS_HSTDMASTATUS5  (*(__IO uint32_t*)0x400AC75CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 5) */
  #define REG_UOTGHS_HSTDMANXTDSC6  (*(__IO uint32_t*)0x400AC760U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 6) */
  #define REG_UOTGHS_HSTDMAADDRESS6 (*(__IO uint32_t*)0x400AC764U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 6) */
  #define REG_UOTGHS_HSTDMACONTROL6 (*(__IO uint32_t*)0x400AC768U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 6) */
  #define REG_UOTGHS_HSTDMASTATUS6  (*(__IO uint32_t*)0x400AC76CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 6) */
  #define REG_UOTGHS_HSTDMANXTDSC7  (*(__IO uint32_t*)0x400AC770U) /**< \brief (UOTGHS) Host DMA Channel Next Descriptor Address Register (n = 7) */
  #define REG_UOTGHS_HSTDMAADDRESS7 (*(__IO uint32_t*)0x400AC774U) /**< \brief (UOTGHS) Host DMA Channel Address Register (n = 7) */
  #define REG_UOTGHS_HSTDMACONTROL7 (*(__IO uint32_t*)0x400AC778U) /**< \brief (UOTGHS) Host DMA Channel Control Register (n = 7) */
  #define REG_UOTGHS_HSTDMASTATUS7  (*(__IO uint32_t*)0x400AC77CU) /**< \brief (UOTGHS) Host DMA Channel Status Register (n = 7) */
  #define REG_UOTGHS_CTRL           (*(__IO uint32_t*)0x400AC800U) /**< \brief (UOTGHS) General Control Register */
  #define REG_UOTGHS_SR             (*(__I  uint32_t*)0x400AC804U) /**< \brief (UOTGHS) General Status Register */
  #define REG_UOTGHS_SCR            (*(__O  uint32_t*)0x400AC808U) /**< \brief (UOTGHS) General Status Clear Register */
  #define REG_UOTGHS_SFR            (*(__O  uint32_t*)0x400AC80CU) /**< \brief (UOTGHS) General Status Set Register */
  #define REG_UOTGHS_FSM            (*(__I  uint32_t*)0x400AC82CU) /**< \brief (UOTGHS) General Finite State Machine Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAM3XA_UOTGHS_INSTANCE_ */
