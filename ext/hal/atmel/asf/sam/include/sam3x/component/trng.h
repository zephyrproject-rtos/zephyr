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

#ifndef _SAM3XA_TRNG_COMPONENT_
#define _SAM3XA_TRNG_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR True Random Number Generator */
/* ============================================================================= */
/** \addtogroup SAM3XA_TRNG True Random Number Generator */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Trng hardware registers */
typedef struct {
  __O uint32_t TRNG_CR;       /**< \brief (Trng Offset: 0x00) Control Register */
  __I uint32_t Reserved1[3];
  __O uint32_t TRNG_IER;      /**< \brief (Trng Offset: 0x10) Interrupt Enable Register */
  __O uint32_t TRNG_IDR;      /**< \brief (Trng Offset: 0x14) Interrupt Disable Register */
  __I uint32_t TRNG_IMR;      /**< \brief (Trng Offset: 0x18) Interrupt Mask Register */
  __I uint32_t TRNG_ISR;      /**< \brief (Trng Offset: 0x1C) Interrupt Status Register */
  __I uint32_t Reserved2[12];
  __I uint32_t TRNG_ODATA;    /**< \brief (Trng Offset: 0x50) Output Data Register */
} Trng;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- TRNG_CR : (TRNG Offset: 0x00) Control Register -------- */
#define TRNG_CR_ENABLE (0x1u << 0) /**< \brief (TRNG_CR) Enables the TRNG to provide random values */
#define TRNG_CR_KEY_Pos 8
#define TRNG_CR_KEY_Msk (0xffffffu << TRNG_CR_KEY_Pos) /**< \brief (TRNG_CR) Security Key */
#define TRNG_CR_KEY(value) ((TRNG_CR_KEY_Msk & ((value) << TRNG_CR_KEY_Pos)))
/* -------- TRNG_IER : (TRNG Offset: 0x10) Interrupt Enable Register -------- */
#define TRNG_IER_DATRDY (0x1u << 0) /**< \brief (TRNG_IER) Data Ready Interrupt Enable */
/* -------- TRNG_IDR : (TRNG Offset: 0x14) Interrupt Disable Register -------- */
#define TRNG_IDR_DATRDY (0x1u << 0) /**< \brief (TRNG_IDR) Data Ready Interrupt Disable */
/* -------- TRNG_IMR : (TRNG Offset: 0x18) Interrupt Mask Register -------- */
#define TRNG_IMR_DATRDY (0x1u << 0) /**< \brief (TRNG_IMR) Data Ready Interrupt Mask */
/* -------- TRNG_ISR : (TRNG Offset: 0x1C) Interrupt Status Register -------- */
#define TRNG_ISR_DATRDY (0x1u << 0) /**< \brief (TRNG_ISR) Data Ready */
/* -------- TRNG_ODATA : (TRNG Offset: 0x50) Output Data Register -------- */
#define TRNG_ODATA_ODATA_Pos 0
#define TRNG_ODATA_ODATA_Msk (0xffffffffu << TRNG_ODATA_ODATA_Pos) /**< \brief (TRNG_ODATA) Output Data */

/*@}*/


#endif /* _SAM3XA_TRNG_COMPONENT_ */
