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

#ifndef _SAM4S_EFC_COMPONENT_
#define _SAM4S_EFC_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Embedded Flash Controller */
/* ============================================================================= */
/** \addtogroup SAM4S_EFC Embedded Flash Controller */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Efc hardware registers */
typedef struct {
  __IO uint32_t EEFC_FMR; /**< \brief (Efc Offset: 0x00) EEFC Flash Mode Register */
  __O  uint32_t EEFC_FCR; /**< \brief (Efc Offset: 0x04) EEFC Flash Command Register */
  __I  uint32_t EEFC_FSR; /**< \brief (Efc Offset: 0x08) EEFC Flash Status Register */
  __I  uint32_t EEFC_FRR; /**< \brief (Efc Offset: 0x0C) EEFC Flash Result Register */
} Efc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- EEFC_FMR : (EFC Offset: 0x00) EEFC Flash Mode Register -------- */
#define EEFC_FMR_FRDY (0x1u << 0) /**< \brief (EEFC_FMR) Flash Ready Interrupt Enable */
#define EEFC_FMR_FWS_Pos 8
#define EEFC_FMR_FWS_Msk (0xfu << EEFC_FMR_FWS_Pos) /**< \brief (EEFC_FMR) Flash Wait State */
#define EEFC_FMR_FWS(value) ((EEFC_FMR_FWS_Msk & ((value) << EEFC_FMR_FWS_Pos)))
#define EEFC_FMR_SCOD (0x1u << 16) /**< \brief (EEFC_FMR) Sequential Code Optimization Disable */
#define EEFC_FMR_FAM (0x1u << 24) /**< \brief (EEFC_FMR) Flash Access Mode */
#define EEFC_FMR_CLOE (0x1u << 26) /**< \brief (EEFC_FMR) Code Loop Optimization Enable */
/* -------- EEFC_FCR : (EFC Offset: 0x04) EEFC Flash Command Register -------- */
#define EEFC_FCR_FCMD_Pos 0
#define EEFC_FCR_FCMD_Msk (0xffu << EEFC_FCR_FCMD_Pos) /**< \brief (EEFC_FCR) Flash Command */
#define EEFC_FCR_FCMD(value) ((EEFC_FCR_FCMD_Msk & ((value) << EEFC_FCR_FCMD_Pos)))
#define   EEFC_FCR_FCMD_GETD (0x0u << 0) /**< \brief (EEFC_FCR) Get Flash descriptor */
#define   EEFC_FCR_FCMD_WP (0x1u << 0) /**< \brief (EEFC_FCR) Write page */
#define   EEFC_FCR_FCMD_WPL (0x2u << 0) /**< \brief (EEFC_FCR) Write page and lock */
#define   EEFC_FCR_FCMD_EWP (0x3u << 0) /**< \brief (EEFC_FCR) Erase page and write page */
#define   EEFC_FCR_FCMD_EWPL (0x4u << 0) /**< \brief (EEFC_FCR) Erase page and write page then lock */
#define   EEFC_FCR_FCMD_EA (0x5u << 0) /**< \brief (EEFC_FCR) Erase all */
#define   EEFC_FCR_FCMD_EPA (0x7u << 0) /**< \brief (EEFC_FCR) Erase pages */
#define   EEFC_FCR_FCMD_SLB (0x8u << 0) /**< \brief (EEFC_FCR) Set lock bit */
#define   EEFC_FCR_FCMD_CLB (0x9u << 0) /**< \brief (EEFC_FCR) Clear lock bit */
#define   EEFC_FCR_FCMD_GLB (0xAu << 0) /**< \brief (EEFC_FCR) Get lock bit */
#define   EEFC_FCR_FCMD_SGPB (0xBu << 0) /**< \brief (EEFC_FCR) Set GPNVM bit */
#define   EEFC_FCR_FCMD_CGPB (0xCu << 0) /**< \brief (EEFC_FCR) Clear GPNVM bit */
#define   EEFC_FCR_FCMD_GGPB (0xDu << 0) /**< \brief (EEFC_FCR) Get GPNVM bit */
#define   EEFC_FCR_FCMD_STUI (0xEu << 0) /**< \brief (EEFC_FCR) Start read unique identifier */
#define   EEFC_FCR_FCMD_SPUI (0xFu << 0) /**< \brief (EEFC_FCR) Stop read unique identifier */
#define   EEFC_FCR_FCMD_GCALB (0x10u << 0) /**< \brief (EEFC_FCR) Get CALIB bit */
#define   EEFC_FCR_FCMD_ES (0x11u << 0) /**< \brief (EEFC_FCR) Erase sector */
#define   EEFC_FCR_FCMD_WUS (0x12u << 0) /**< \brief (EEFC_FCR) Write user signature */
#define   EEFC_FCR_FCMD_EUS (0x13u << 0) /**< \brief (EEFC_FCR) Erase user signature */
#define   EEFC_FCR_FCMD_STUS (0x14u << 0) /**< \brief (EEFC_FCR) Start read user signature */
#define   EEFC_FCR_FCMD_SPUS (0x15u << 0) /**< \brief (EEFC_FCR) Stop read user signature */
#define EEFC_FCR_FARG_Pos 8
#define EEFC_FCR_FARG_Msk (0xffffu << EEFC_FCR_FARG_Pos) /**< \brief (EEFC_FCR) Flash Command Argument */
#define EEFC_FCR_FARG(value) ((EEFC_FCR_FARG_Msk & ((value) << EEFC_FCR_FARG_Pos)))
#define EEFC_FCR_FKEY_Pos 24
#define EEFC_FCR_FKEY_Msk (0xffu << EEFC_FCR_FKEY_Pos) /**< \brief (EEFC_FCR) Flash Writing Protection Key */
#define EEFC_FCR_FKEY(value) ((EEFC_FCR_FKEY_Msk & ((value) << EEFC_FCR_FKEY_Pos)))
#define   EEFC_FCR_FKEY_PASSWD (0x5Au << 24) /**< \brief (EEFC_FCR) The 0x5A value enables the command defined by the bits of the register. If the field is written with a different value, the write is not performed and no action is started. */
/* -------- EEFC_FSR : (EFC Offset: 0x08) EEFC Flash Status Register -------- */
#define EEFC_FSR_FRDY (0x1u << 0) /**< \brief (EEFC_FSR) Flash Ready Status (cleared when Flash is busy) */
#define EEFC_FSR_FCMDE (0x1u << 1) /**< \brief (EEFC_FSR) Flash Command Error Status (cleared on read or by writing EEFC_FCR) */
#define EEFC_FSR_FLOCKE (0x1u << 2) /**< \brief (EEFC_FSR) Flash Lock Error Status (cleared on read) */
#define EEFC_FSR_FLERR (0x1u << 3) /**< \brief (EEFC_FSR) Flash Error Status (cleared when a programming operation starts) */
/* -------- EEFC_FRR : (EFC Offset: 0x0C) EEFC Flash Result Register -------- */
#define EEFC_FRR_FVALUE_Pos 0
#define EEFC_FRR_FVALUE_Msk (0xffffffffu << EEFC_FRR_FVALUE_Pos) /**< \brief (EEFC_FRR) Flash Result Value */

/*@}*/


#endif /* _SAM4S_EFC_COMPONENT_ */
