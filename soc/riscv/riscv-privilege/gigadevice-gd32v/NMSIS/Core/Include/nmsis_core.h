/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NMSIS_CORE_H__
#define __NMSIS_CORE_H__

#include <stdint.h>
#include <arch/riscv/csr.h>
#include "riscv_encoding.h"

/* NMSIS compiler specific defines */
/** \brief Pass information from the compiler to the assembler. */
#ifndef   __ASM
  #define __ASM                                  __asm
#endif

/** \brief Define a static function that should be always inlined by the compiler. */
#ifndef   __STATIC_FORCEINLINE
  #define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static inline
#endif

/** \brief Defines 'read only' permissions */
#ifdef __cplusplus
  #define   __I     volatile
#else
  #define   __I     volatile const
#endif
/** \brief Defines 'write only' permissions */
#define     __O     volatile
/** \brief Defines 'read / write' permissions */
#define     __IO    volatile

/* following defines should be used for structure members */
/** \brief Defines 'read only' structure member permissions */
#define     __IM     volatile const
/** \brief Defines 'write only' structure member permissions */
#define     __OM     volatile
/** \brief Defines 'read/write' structure member permissions */
#define     __IOM    volatile

#ifndef __RISCV_XLEN
/** \brief Refer to the width of an integer register in bits(either 32 or 64) */
  #ifndef __riscv_xlen
    #define __RISCV_XLEN    32
  #else
    #define __RISCV_XLEN    __riscv_xlen
  #endif
#endif /* __RISCV_XLEN */

/** \brief Type of Control and Status Register(CSR), depends on the XLEN defined in RISC-V */
#if __RISCV_XLEN == 32
typedef uint32_t rv_csr_t;
#elif __RISCV_XLEN == 64
typedef uint64_t rv_csr_t;
#else
typedef uint32_t rv_csr_t;
#endif

#define __RV_CSR_READ(x) csr_read(x)
#define __RV_CSR_WRITE(x, y) csr_write(x, y)
#define __RV_CSR_CLEAR(x, y) csr_clear(x, y)
#define __RV_CSR_SET(x, y) csr_set(x, y)

/**
 * \brief   Enable IRQ Interrupts
 * \details Enables IRQ interrupts by setting the MIE-bit in the MSTATUS Register.
 * \remarks
 *          Can only be executed in Privileged modes.
 */
__STATIC_FORCEINLINE void __enable_irq(void)
{
    csr_set(mstatus, MSTATUS_MIE);
}

/**
 * \brief   Disable IRQ Interrupts
 * \details Disables IRQ interrupts by clearing the MIE-bit in the MSTATUS Register.
 * \remarks
 *          Can only be executed in Privileged modes.
 */
__STATIC_FORCEINLINE void __disable_irq(void)
{
    csr_clear(mstatus, MSTATUS_MIE);
}

/**
 * \brief   Wait For Interrupt
 * \details
 * Wait For Interrupt is is executed using CSR_WFE.WFE=0 and WFI instruction.
 * It will suspends execution until interrupt, NMI or Debug happened.
 * When Core is waked up by interrupt, if
 * 1. mstatus.MIE == 1(interrupt enabled), Core will enter ISR code
 * 2. mstatus.MIE == 0(interrupt disabled), Core will resume previous execution
 */
__STATIC_FORCEINLINE void __WFI(void)
{
    __RV_CSR_CLEAR(CSR_WFE, WFE_WFE);
    __ASM volatile("wfi");
}

/**
 * \brief   Wait For Event
 * \details
 * Wait For Event is executed using CSR_WFE.WFE=1 and WFI instruction.
 * It will suspends execution until event, NMI or Debug happened.
 * When Core is waked up, Core will resume previous execution
 */
__STATIC_FORCEINLINE void __WFE(void)
{
    __RV_CSR_SET(CSR_WFE, WFE_WFE);
    __ASM volatile("wfi");
    __RV_CSR_CLEAR(CSR_WFE, WFE_WFE);
}

#include <core_feature_eclic.h>

#endif
