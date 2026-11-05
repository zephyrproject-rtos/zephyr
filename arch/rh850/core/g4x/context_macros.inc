/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Common context save/restore macros for RH850 G4X (IAR) */

#include <zephyr/arch/rh850/g4x/g4x.h>

/* Exception stack frame layout (top of stack = ctpc)
 *
 * Pushed in this order (first -> last), so ctpc is at the lowest address
 * when the frame is complete:
 *
 *   r1, r2, r5, r6, r7, r8, r9, r10, r11, r12,
 *   r13, r14, r15, r16, r17, r18, r19, r20,
 *   r21, r22, r23, r24, r25, r26, r27, r28,
 *   r29, r30,
 * #ifdef CONFIG_FXU
 *   fxxc, fxsr,
 * #endif
 * #ifdef CONFIG_FPU_SHARING
 *   fpepc, fpsr,
 * #endif
 *   lp, eipsw/fepsw, eipc/fepc, eiic/feic, ctpsw, ctpc
 *
 *  This must MATCH struct arch_esf in exception.h
 */

SAVE_FULL_CONTEXT_EI MACRO
    pushsp r1-r2
    pushsp r5-r30

#ifdef CONFIG_FPU_SHARING
    stsr FPSR, r18
    stsr FPEPC, r19
    pushsp r18-r19
#endif

#ifdef CONFIG_FXU
    stsr CPU_REG_FXSR, r18, 10
    stsr CPU_REG_FXXC, r19, 10
    pushsp r18-r19
#endif

    mov lp, r14
    stsr EIPSW, r15
    stsr EIPC, r16
    stsr EIWR, r17
    stsr CTPSW, r18
    stsr CTPC, r19
    pushsp r14-r19

    stsr EIIC, r18
    ldsr r18, EIWR
 ENDM

RESTORE_FULL_CONTEXT_EIRET MACRO
    popsp r14-r19
    ldsr r19, CTPC
    ldsr r18, CTPSW
    ldsr r17, EIWR
    ldsr r16, EIPC
    ldsr r15, EIPSW
    mov r14, lp

    stsr EIWR, r18
    ldsr r18, EIIC

#ifdef CONFIG_FXU
    popsp r18-r19
    ldsr r19, CPU_REG_FXXC, 10
    ldsr r18, CPU_REG_FXSR, 10
#endif

#ifdef CONFIG_FPU_SHARING
    popsp r18-r19
    ldsr r19, FPEPC
    ldsr r18, FPSR
#endif

    popsp r5-r30
    popsp r1-r2
    eiret
 ENDM

SAVE_FULL_CONTEXT_FE MACRO
    pushsp r1-r2
    pushsp r5-r30

#ifdef CONFIG_FPU_SHARING
    stsr FPSR, r18
    stsr FPEPC, r19
    pushsp r18-r19
#endif

#ifdef CONFIG_FXU
    stsr CPU_REG_FXSR, r18, 10
    stsr CPU_REG_FXXC, r19, 10
    pushsp r18-r19
#endif

    mov lp, r14
    stsr FEPSW, r15
    stsr FEPC, r16
    stsr FEWR, r17
    stsr CTPSW, r18
    stsr CTPC, r19
    pushsp r14-r19

    stsr FEIC, r18
    ldsr r18, FEWR
 ENDM

RESTORE_FULL_CONTEXT_FERET MACRO
    popsp r14-r19
    ldsr r19, CTPC
    ldsr r18, CTPSW
    ldsr r17, FEWR
    ldsr r16, FEPC
    ldsr r15, FEPSW
    mov r14, lp

    stsr FEWR, r18
    ldsr r18, FEIC

#ifdef CONFIG_FXU
    popsp r18-r19
    ldsr r19, CPU_REG_FXXC, 10
    ldsr r18, CPU_REG_FXSR, 10
#endif

#ifdef CONFIG_FPU_SHARING
    popsp r18-r19
    ldsr r19, FPEPC
    ldsr r18, FPSR
#endif

    popsp r5-r30
    popsp r1-r2
    feret
 ENDM
