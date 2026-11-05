/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Common context save/restore macros for RH850 G4X */

#include <zephyr/arch/rh850/g4x/g4x.h>
#include <zephyr/arch/rh850/asm/asm_directive.h>

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

RH850_ASM_DIRECTIVE_MACRO(SAVE_FULL_CONTEXT_EI)
    pushsp r1-r2
    pushsp r5-r30

#ifdef CONFIG_FPU_SHARING
    stsr CPU_REG_FPSR, r18, CPU_SEL_FPSR
    stsr CPU_REG_FPEPC, r19, CPU_SEL_FPEPC
    pushsp r18-r19
#endif

#ifdef CONFIG_FXU
    stsr CPU_REG_FXSR, r18, CPU_SEL_FXSR
    stsr CPU_REG_FXXC, r19, CPU_SEL_FXXC
    pushsp r18-r19
#endif

    mov lp, r14
    stsr CPU_REG_EIPSW, r15, CPU_SEL_EIPSW
    stsr CPU_REG_EIPC, r16, CPU_SEL_EIPC
    stsr CPU_REG_EIWR, r17, CPU_SEL_EIWR
    stsr CPU_REG_CTPSW, r18, CPU_SEL_CTPSW
    stsr CPU_REG_CTPC, r19, CPU_SEL_CTPC
    pushsp r14-r19

    stsr CPU_REG_EIIC, r18, CPU_SEL_EIIC
    ldsr r18, CPU_REG_EIWR, CPU_SEL_EIWR
 RH850_ASM_DIRECTIVE_MACRO_END

RH850_ASM_DIRECTIVE_MACRO(RESTORE_FULL_CONTEXT_EIRET)
    popsp r14-r19
    ldsr r19, CPU_REG_CTPC, CPU_SEL_CTPC
    ldsr r18, CPU_REG_CTPSW, CPU_SEL_CTPSW
    ldsr r17, CPU_REG_EIWR, CPU_SEL_EIWR
    ldsr r16, CPU_REG_EIPC, CPU_SEL_EIPC
    ldsr r15, CPU_REG_EIPSW, CPU_SEL_EIPSW
    mov r14, lp

    stsr CPU_REG_EIWR, r18, CPU_SEL_EIWR
    ldsr r18, CPU_REG_EIIC, CPU_SEL_EIIC

#ifdef CONFIG_FXU
    popsp r18-r19
    ldsr r19, CPU_REG_FXXC, CPU_SEL_FXXC
    ldsr r18, CPU_REG_FXSR, CPU_SEL_FXSR
#endif

#ifdef CONFIG_FPU_SHARING
    popsp r18-r19
    ldsr r19, CPU_REG_FPEPC, CPU_SEL_FPEPC
    ldsr r18, CPU_REG_FPSR, CPU_SEL_FPSR
#endif

    popsp r5-r30
    popsp r1-r2
    eiret
 RH850_ASM_DIRECTIVE_MACRO_END

RH850_ASM_DIRECTIVE_MACRO(SAVE_FULL_CONTEXT_FE)
    pushsp r1-r2
    pushsp r5-r30

#ifdef CONFIG_FPU_SHARING
    stsr CPU_REG_FPSR, r18, CPU_SEL_FPSR
    stsr CPU_REG_FPEPC, r19, CPU_SEL_FPEPC
    pushsp r18-r19
#endif

#ifdef CONFIG_FXU
    stsr CPU_REG_FXSR, r18, CPU_SEL_FXSR
    stsr CPU_REG_FXXC, r19, CPU_SEL_FXXC
    pushsp r18-r19
#endif

    mov lp, r14
    stsr CPU_REG_FEPSW, r15, CPU_SEL_FEPSW
    stsr CPU_REG_FEPC, r16, CPU_SEL_FEPC
    stsr CPU_REG_FEWR, r17, CPU_SEL_FEWR
    stsr CPU_REG_CTPSW, r18, CPU_SEL_CTPSW
    stsr CPU_REG_CTPC, r19, CPU_SEL_CTPC
    pushsp r14-r19

    stsr CPU_REG_FEIC, r18, CPU_SEL_FEIC
    ldsr r18, CPU_REG_FEWR, CPU_SEL_FEWR
 RH850_ASM_DIRECTIVE_MACRO_END

RH850_ASM_DIRECTIVE_MACRO(RESTORE_FULL_CONTEXT_FERET)
    popsp r14-r19
    ldsr r19, CPU_REG_CTPC, CPU_SEL_CTPC
    ldsr r18, CPU_REG_CTPSW, CPU_SEL_CTPSW
    ldsr r17, CPU_REG_FEWR, CPU_SEL_FEWR
    ldsr r16, CPU_REG_FEPC, CPU_SEL_FEPC
    ldsr r15, CPU_REG_FEPSW, CPU_SEL_FEPSW
    mov r14, lp

    stsr CPU_REG_FEWR, r18, CPU_SEL_FEWR
    ldsr r18, CPU_REG_FEIC, CPU_SEL_FEIC

#ifdef CONFIG_FXU
    popsp r18-r19
    ldsr r19, CPU_REG_FXXC, CPU_SEL_FXXC
    ldsr r18, CPU_REG_FXSR, CPU_SEL_FXSR
#endif

#ifdef CONFIG_FPU_SHARING
    popsp r18-r19
    ldsr r19, CPU_REG_FPEPC, CPU_SEL_FPEPC
    ldsr r18, CPU_REG_FPSR, CPU_SEL_FPSR
#endif

    popsp r5-r30
    popsp r1-r2
    feret
 RH850_ASM_DIRECTIVE_MACRO_END
