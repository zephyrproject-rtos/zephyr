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

#ifndef _SAM3XA_CHIPID_COMPONENT_
#define _SAM3XA_CHIPID_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Chip Identifier */
/* ============================================================================= */
/** \addtogroup SAM3XA_CHIPID Chip Identifier */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Chipid hardware registers */
typedef struct {
  __I uint32_t CHIPID_CIDR; /**< \brief (Chipid Offset: 0x0) Chip ID Register */
  __I uint32_t CHIPID_EXID; /**< \brief (Chipid Offset: 0x4) Chip ID Extension Register */
} Chipid;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CHIPID_CIDR : (CHIPID Offset: 0x0) Chip ID Register -------- */
#define CHIPID_CIDR_VERSION_Pos 0
#define CHIPID_CIDR_VERSION_Msk (0x1fu << CHIPID_CIDR_VERSION_Pos) /**< \brief (CHIPID_CIDR) Version of the Device */
#define CHIPID_CIDR_EPROC_Pos 5
#define CHIPID_CIDR_EPROC_Msk (0x7u << CHIPID_CIDR_EPROC_Pos) /**< \brief (CHIPID_CIDR) Embedded Processor */
#define   CHIPID_CIDR_EPROC_ARM946ES (0x1u << 5) /**< \brief (CHIPID_CIDR) ARM946ES */
#define   CHIPID_CIDR_EPROC_ARM7TDMI (0x2u << 5) /**< \brief (CHIPID_CIDR) ARM7TDMI */
#define   CHIPID_CIDR_EPROC_CM3 (0x3u << 5) /**< \brief (CHIPID_CIDR) Cortex-M3 */
#define   CHIPID_CIDR_EPROC_ARM920T (0x4u << 5) /**< \brief (CHIPID_CIDR) ARM920T */
#define   CHIPID_CIDR_EPROC_ARM926EJS (0x5u << 5) /**< \brief (CHIPID_CIDR) ARM926EJS */
#define   CHIPID_CIDR_EPROC_CA5 (0x6u << 5) /**< \brief (CHIPID_CIDR) Cortex-A5 */
#define   CHIPID_CIDR_EPROC_CM4 (0x7u << 5) /**< \brief (CHIPID_CIDR) Cortex-M4 */
#define CHIPID_CIDR_NVPSIZ_Pos 8
#define CHIPID_CIDR_NVPSIZ_Msk (0xfu << CHIPID_CIDR_NVPSIZ_Pos) /**< \brief (CHIPID_CIDR) Nonvolatile Program Memory Size */
#define   CHIPID_CIDR_NVPSIZ_NONE (0x0u << 8) /**< \brief (CHIPID_CIDR) None */
#define   CHIPID_CIDR_NVPSIZ_8K (0x1u << 8) /**< \brief (CHIPID_CIDR) 8 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_16K (0x2u << 8) /**< \brief (CHIPID_CIDR) 16 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_32K (0x3u << 8) /**< \brief (CHIPID_CIDR) 32 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_64K (0x5u << 8) /**< \brief (CHIPID_CIDR) 64 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_128K (0x7u << 8) /**< \brief (CHIPID_CIDR) 128 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_256K (0x9u << 8) /**< \brief (CHIPID_CIDR) 256 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_512K (0xAu << 8) /**< \brief (CHIPID_CIDR) 512 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_1024K (0xCu << 8) /**< \brief (CHIPID_CIDR) 1024 Kbytes */
#define   CHIPID_CIDR_NVPSIZ_2048K (0xEu << 8) /**< \brief (CHIPID_CIDR) 2048 Kbytes */
#define CHIPID_CIDR_NVPSIZ2_Pos 12
#define CHIPID_CIDR_NVPSIZ2_Msk (0xfu << CHIPID_CIDR_NVPSIZ2_Pos) /**< \brief (CHIPID_CIDR) Second Nonvolatile Program Memory Size */
#define   CHIPID_CIDR_NVPSIZ2_NONE (0x0u << 12) /**< \brief (CHIPID_CIDR) None */
#define   CHIPID_CIDR_NVPSIZ2_8K (0x1u << 12) /**< \brief (CHIPID_CIDR) 8 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_16K (0x2u << 12) /**< \brief (CHIPID_CIDR) 16 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_32K (0x3u << 12) /**< \brief (CHIPID_CIDR) 32 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_64K (0x5u << 12) /**< \brief (CHIPID_CIDR) 64 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_128K (0x7u << 12) /**< \brief (CHIPID_CIDR) 128 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_256K (0x9u << 12) /**< \brief (CHIPID_CIDR) 256 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_512K (0xAu << 12) /**< \brief (CHIPID_CIDR) 512 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_1024K (0xCu << 12) /**< \brief (CHIPID_CIDR) 1024 Kbytes */
#define   CHIPID_CIDR_NVPSIZ2_2048K (0xEu << 12) /**< \brief (CHIPID_CIDR) 2048 Kbytes */
#define CHIPID_CIDR_SRAMSIZ_Pos 16
#define CHIPID_CIDR_SRAMSIZ_Msk (0xfu << CHIPID_CIDR_SRAMSIZ_Pos) /**< \brief (CHIPID_CIDR) Internal SRAM Size */
#define   CHIPID_CIDR_SRAMSIZ_48K (0x0u << 16) /**< \brief (CHIPID_CIDR) 48 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_192K (0x1u << 16) /**< \brief (CHIPID_CIDR) 192 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_2K (0x2u << 16) /**< \brief (CHIPID_CIDR) 2 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_6K (0x3u << 16) /**< \brief (CHIPID_CIDR) 6 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_24K (0x4u << 16) /**< \brief (CHIPID_CIDR) 24 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_4K (0x5u << 16) /**< \brief (CHIPID_CIDR) 4 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_80K (0x6u << 16) /**< \brief (CHIPID_CIDR) 80 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_160K (0x7u << 16) /**< \brief (CHIPID_CIDR) 160 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_8K (0x8u << 16) /**< \brief (CHIPID_CIDR) 8 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_16K (0x9u << 16) /**< \brief (CHIPID_CIDR) 16 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_32K (0xAu << 16) /**< \brief (CHIPID_CIDR) 32 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_64K (0xBu << 16) /**< \brief (CHIPID_CIDR) 64 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_128K (0xCu << 16) /**< \brief (CHIPID_CIDR) 128 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_256K (0xDu << 16) /**< \brief (CHIPID_CIDR) 256 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_96K (0xEu << 16) /**< \brief (CHIPID_CIDR) 96 Kbytes */
#define   CHIPID_CIDR_SRAMSIZ_512K (0xFu << 16) /**< \brief (CHIPID_CIDR) 512 Kbytes */
#define CHIPID_CIDR_ARCH_Pos 20
#define CHIPID_CIDR_ARCH_Msk (0xffu << CHIPID_CIDR_ARCH_Pos) /**< \brief (CHIPID_CIDR) Architecture Identifier */
#define   CHIPID_CIDR_ARCH_SAM3AxC (0x83u << 20) /**< \brief (CHIPID_CIDR) SAM3AxC (100-pin version) */
#define   CHIPID_CIDR_ARCH_SAM3XxC (0x84u << 20) /**< \brief (CHIPID_CIDR) SAM3XxC (100-pin version) */
#define   CHIPID_CIDR_ARCH_SAM3XxE (0x85u << 20) /**< \brief (CHIPID_CIDR) SAM3XxE (144-pin version) */
#define   CHIPID_CIDR_ARCH_SAM3XxG (0x86u << 20) /**< \brief (CHIPID_CIDR) SAM3XxG (208/217-pin version) */
#define CHIPID_CIDR_NVPTYP_Pos 28
#define CHIPID_CIDR_NVPTYP_Msk (0x7u << CHIPID_CIDR_NVPTYP_Pos) /**< \brief (CHIPID_CIDR) Nonvolatile Program Memory Type */
#define   CHIPID_CIDR_NVPTYP_ROM (0x0u << 28) /**< \brief (CHIPID_CIDR) ROM */
#define   CHIPID_CIDR_NVPTYP_ROMLESS (0x1u << 28) /**< \brief (CHIPID_CIDR) ROMless or on-chip Flash */
#define   CHIPID_CIDR_NVPTYP_FLASH (0x2u << 28) /**< \brief (CHIPID_CIDR) Embedded Flash Memory */
#define   CHIPID_CIDR_NVPTYP_ROM_FLASH (0x3u << 28) /**< \brief (CHIPID_CIDR) ROM and Embedded Flash Memory- NVPSIZ is ROM size- NVPSIZ2 is Flash size */
#define   CHIPID_CIDR_NVPTYP_SRAM (0x4u << 28) /**< \brief (CHIPID_CIDR) SRAM emulating ROM */
#define CHIPID_CIDR_EXT (0x1u << 31) /**< \brief (CHIPID_CIDR) Extension Flag */
/* -------- CHIPID_EXID : (CHIPID Offset: 0x4) Chip ID Extension Register -------- */
#define CHIPID_EXID_EXID_Pos 0
#define CHIPID_EXID_EXID_Msk (0xffffffffu << CHIPID_EXID_EXID_Pos) /**< \brief (CHIPID_EXID) Chip ID Extension */

/*@}*/


#endif /* _SAM3XA_CHIPID_COMPONENT_ */
