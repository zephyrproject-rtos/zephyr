/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/rh850/g4x/g4x.h>
#include <zephyr/arch/rh850/asm/asm_directive.h>

/* Definition of PE number */
#define PE0       0

  RH850_ASM_DIRECTIVE_EXTERN(__image_ram_start)
  RH850_ASM_DIRECTIVE_EXTERN(___kernel_ram_end)
  RH850_ASM_DIRECTIVE_EXTERN(__vector_start)
  RH850_ASM_DIRECTIVE_EXTERN(__irq_vector_table)
  RH850_ASM_DIRECTIVE_EXTERN(_z_interrupt_stacks)
  RH850_ASM_DIRECTIVE_EXTERN(Region$$Table$$Base)
  RH850_ASM_DIRECTIVE_EXTERN(Region$$Table$$Limit)
  RH850_ASM_DIRECTIVE_WEAK(__iar_static_base$$TP)

  RH850_ASM_DIRECTIVE_EXTERN(_z_prep_c)
  RH850_ASM_DIRECTIVE_EXTERN(_z_rh850_exit)
  RH850_ASM_DIRECTIVE_EXTERN(___iar_data_init2)

  RH850_ASM_DIRECTIVE_SECTION(CODE, 2, .text)

/* Common Startup Routine */
  RH850_ASM_DIRECTIVE_GLOBAL(_z_rh850_reset)
  RH850_ASM_DIRECTIVE_GLOBAL(__start)
__start:
_z_rh850_reset:
  synci

/* CPU function group program registers initialization */
  mov r0, r2
  mov r0, r3
  mov r0, r4
  mov r0, r5
  mov r0, r6
  mov r0, r7
  mov r0, r8
  mov r0, r9
  mov r0, r10
  mov r0, r11
  mov r0, r12
  mov r0, r13
  mov r0, r14
  mov r0, r15
  mov r0, r16
  mov r0, r17
  mov r0, r18
  mov r0, r19
  mov r0, r20
  mov r0, r21
  mov r0, r22
  mov r0, r23
  mov r0, r24
  mov r0, r25
  mov r0, r26
  mov r0, r27
  mov r0, r28
  mov r0, r29
  mov r0, r30
  mov r0, r31

/* CPU function group system registers initialization */
  ldsr r0, CPU_REG_EIPC, CPU_SEL_EIPC
  ldsr r0, CPU_REG_EIPSW, CPU_SEL_EIPSW
  ldsr r0, CPU_REG_FEPC, CPU_SEL_FEPC
  ldsr r0, CPU_REG_FEPSW, CPU_SEL_FEPSW
  ldsr r0, CPU_REG_EIIC, CPU_SEL_EIIC
  ldsr r0, CPU_REG_FEIC, CPU_SEL_FEIC
  ldsr r0, CPU_REG_CTPC, CPU_SEL_CTPC
  ldsr r0, CPU_REG_CTPSW, CPU_SEL_CTPSW
  ldsr r0, CPU_REG_CTBP, CPU_SEL_CTBP
  ldsr r0, CPU_REG_EIWR, CPU_SEL_EIWR
  ldsr r0, CPU_REG_FEWR, CPU_SEL_FEWR
  ldsr r0, CPU_REG_RBASE, CPU_SEL_RBASE
  ldsr r0, CPU_REG_EBASE, CPU_SEL_EBASE
  ldsr r0, CPU_REG_INTBP, CPU_SEL_INTBP
  ldsr r0, CPU_REG_SCCFG, CPU_SEL_SCCFG
  ldsr r0, CPU_REG_SCBP, CPU_SEL_SCBP
  ldsr r0, CPU_REG_MEA, CPU_SEL_MEA
  ldsr r0, CPU_REG_MEI, CPU_SEL_MEI

/* Disable EI-level interrupt */
  movea 0x20, r0, r6
  ldsr r6, CPU_REG_PSW, CPU_SEL_PSW

/* Jump to PEn Startup Routine */
  stsr CPU_REG_PEID, r10, CPU_SEL_PEID
  andi 0x07, r10, r10

  cmp PE0, r10
  be __start_pe0

/* Other PEs are not supported yet: park them */
  RH850_ASM_DIRECTIVE_GLOBAL(__wait_forever)
__wait_forever:
  halt
  br __wait_forever

__start_pe0:
  jr _RESET_PE0

/* Initialize SRAM areas with a known value (0xaa) */
_Init_SRAM:
  movhi RH850_HI(0xAAAAAAAA), r0, r6
  movea RH850_LO(0xAAAAAAAA), r6, r6
  movhi RH850_HI(CONFIG_SRAM_BASE_ADDRESS), r0, r7
  movea RH850_LO(CONFIG_SRAM_BASE_ADDRESS), r7, r7
  movhi RH850_HI(___kernel_ram_end), r0, r8
  movea RH850_LO(___kernel_ram_end), r8, r8

_cram_loop:
  cmp r7, r8
  bne _cram_loop_exit
  jmp [lp]
_cram_loop_exit:
  st.w r6, 0[r7]
  add 0x04, r7
  br _cram_loop

/* PE0 Specific Startup Routine */
  RH850_ASM_DIRECTIVE_GLOBAL(_RESET_PE0)
_RESET_PE0:
#ifdef CONFIG_RH850_RAM_INIT
#ifdef CONFIG_RH850_RAM_INIT_CHECK_RESET
/* Check system reset cause before process Memory initialization */
  movhi   RH850_HI(CONFIG_RH850_RESET_CAUSE_REG_ADDR), r0, r10
  movea   RH850_LO(CONFIG_RH850_RESET_CAUSE_REG_ADDR), r10, r10
  ld.w    0[r10], r11
  movhi   RH850_HI(CONFIG_RH850_RESET_CAUSE_MASK), r0, r12
  movea   RH850_LO(CONFIG_RH850_RESET_CAUSE_MASK), r12, r12
  and     r12, r11
  bne     skip_init_sram
#endif /* CONFIG_RH850_RAM_INIT_CHECK_RESET */

/* Memory Initialization */
/* Initialize SRAM */
  jarl _Init_SRAM, lp
#endif /* CONFIG_RH850_RAM_INIT */

skip_init_sram:
/* PE0 Initialization */
/* Initialization of the global pointer */
  mov r0, gp

/* Initialization of the text pointer */
  movhi RH850_HI(__iar_static_base$$TP), r0, tp
  movea RH850_LO(__iar_static_base$$TP), tp, tp

/* Initialization of the element pointer */
  mov r0, ep

/* Initialization of the stack pointer */
  movhi RH850_HI(_z_interrupt_stacks), r0, r1
  movea RH850_LO(_z_interrupt_stacks), r1, r1
  movhi RH850_HI(CONFIG_ISR_STACK_SIZE), r0, r6
  movea RH850_LO(CONFIG_ISR_STACK_SIZE), r6, r6
  add r6, r1
  mov r1, sp

/* Initialization of the interrupt base pointer */
  movhi RH850_HI(__irq_vector_table), r0, r1
  movea RH850_LO(__irq_vector_table), r1, r1
  ldsr r1, CPU_REG_INTBP, CPU_SEL_INTBP

/* First set EBASE register address */
  movhi RH850_HI(__vector_start), r0, r10
  movea RH850_LO(__vector_start), r10, r10
  ldsr r10, CPU_REG_EBASE, CPU_SEL_EBASE

/* Set PSW.EBV bit to 1 -> RBASE != EBASE */
  stsr CPU_REG_PSW, r10, CPU_SEL_PSW  /* Read PSW register */
  movhi RH850_HI(PSW_EBV), r0, r11    /* PSW.EBV bit mask */
  movea RH850_LO(PSW_EBV), r11, r11
  or r11, r10                         /* Set EBV bit */
  ldsr r10, CPU_REG_PSW, CPU_SEL_PSW  /* Write back to PSW */

/* Initialization of the FPU */
#if defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_FPU)
  stsr CPU_REG_PSW, r10, CPU_SEL_PSW  /* Read current PSW */
  movhi 0x01, r0, r11                 /* PSW.CU0 bit (FPU enable) */
  or r11, r10                         /* Enable FPU in PSW */
  ldsr r10, CPU_REG_PSW, CPU_SEL_PSW  /* Write back to PSW */

  movhi 0x02, r0, r11
  ldsr r11, CPU_REG_FPSR, CPU_SEL_FPSR      /* initialize FPSR */
  ldsr r0, CPU_REG_FPEPC, CPU_SEL_FPEPC     /* initialize FPEPC */
#endif

/* Initialization of the FXU */
#if defined(CONFIG_CPU_HAS_FXU) && defined(CONFIG_FXU)
  stsr CPU_REG_PSW, r10, CPU_SEL_PSW  /* Read current PSW */
  movhi 0x02, r0, r11                 /* PSW.CU1 bit (FXU enable) */
  or r11, r10                         /* Enable FXU in PSW */
  ldsr r10, CPU_REG_PSW, CPU_SEL_PSW  /* Write back to PSW */

  movhi 0x02, r0, r11
  ldsr r11, CPU_REG_FXSR, CPU_SEL_FXSR /* initialize FXSR */
#endif

#ifdef CONFIG_IAR_DATA_INIT
/* Prepare arguments before initialize global data with __iar_data_init2 */
  movhi RH850_HI(Region$$Table$$Base), r0, r6
  movea RH850_LO(Region$$Table$$Base), r6, r6
  movhi RH850_HI(Region$$Table$$Limit), r0, r7
  movea RH850_LO(Region$$Table$$Limit), r7, r7
#endif

/* Jump to z_prep_c */
  jarl _z_prep_c, lp
  movhi RH850_HI(_z_rh850_exit), r0, lp
  movea RH850_LO(_z_rh850_exit), lp, lp
  jmp [lp]

  RH850_ASM_DIRECTIVE_END
