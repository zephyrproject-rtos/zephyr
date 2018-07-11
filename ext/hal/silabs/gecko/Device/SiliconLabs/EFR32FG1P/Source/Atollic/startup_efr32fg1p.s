/**************************************************************************//**
 *  File        : startup_efm32x.s
 *
 *  Abstract    : This file contains interrupt vector and startup code.
 *
 *  Functions   : Reset_Handler
 *
 *  Target      : Silicon Labs EFR32FG1P devices.
 *
 *  Environment : Atollic TrueSTUDIO(R)
 *
 *  Distribution: The file is distributed "as is," without any warranty
 *                of any kind.
 *
 *  (c)Copyright Atollic AB.
 *  You may use this file as-is or modify it according to the needs of your
 *  project. This file may only be built (assembled or compiled and linked)
 *  using the Atollic TrueSTUDIO(R) product. The use of this file together
 *  with other tools than Atollic TrueSTUDIO(R) is not permitted.
 *
 *******************************************************************************
 * Silicon Labs release version
 * @version 5.1.2
 ******************************************************************************/
  .syntax unified
  .thumb

  .global Reset_Handler
  .global InterruptVector
  .global Default_Handler

  /* Linker script definitions */
  /* start address for the initialization values of the .data section */
  .word _sidata
  /* start address for the .data section */
  .word _sdata
  /* end address for the .data section */
  .word _edata
  /* start address for the .bss section */
  .word _sbss
  /* end address for the .bss section */
  .word _ebss

/**
**===========================================================================
**  Program - Reset_Handler
**  Abstract: This code gets called after reset.
**===========================================================================
*/
  .section  .text.Reset_Handler,"ax", %progbits
  .type Reset_Handler, %function
Reset_Handler:
  /* Set stack pointer */
  ldr   sp,=_estack

  /* Branch to SystemInit function */
  bl    SystemInit

  /* Copy data initialization values */
  ldr   r1,=_sidata
  ldr   r2,=_sdata
  ldr   r3,=_edata
  b     cmpdata
CopyLoop:
  ldr   r0, [r1], #4
  str   r0, [r2], #4
cmpdata:
  cmp   r2, r3
  blt   CopyLoop

  /* Clear BSS section */
  movs  r0, #0
  ldr   r2,=_sbss
  ldr   r3,=_ebss
  b     cmpbss
ClearLoop:
  str   r0, [r2], #4
cmpbss:
  cmp   r2, r3
  blt   ClearLoop

  /* Call static constructors */
  bl    __libc_init_array

  /* Branch to main */
  bl    main

  /* If main returns, branch to Default_Handler. */
  b     Default_Handler

  .size  Reset_Handler, .-Reset_Handler

/**
**===========================================================================
**  Program - Default_Handler
**  Abstract: This code gets called when the processor receives an
**    unexpected interrupt.
**===========================================================================
*/
  .section  .text.Default_Handler,"ax", %progbits
Default_Handler:
  b  Default_Handler

  .size  Default_Handler, .-Default_Handler

/**
**===========================================================================
**  Interrupt vector table
**===========================================================================
*/
  .section .isr_vector,"a", %progbits
InterruptVector:
  .word _estack                   /* 0 - Stack pointer */
  .word Reset_Handler             /* 1 - Reset */
  .word NMI_Handler               /* 2 - NMI  */
  .word HardFault_Handler         /* 3 - Hard fault */
  .word MemManage_Handler         /* 4 - Memory management fault */
  .word BusFault_Handler          /* 5 - Bus fault */
  .word UsageFault_Handler        /* 6 - Usage fault */
  .word 0                         /* 7 - Reserved */
  .word 0                         /* 8 - Reserved */
  .word 0                         /* 9 - Reserved */
  .word 0                         /* 10 - Reserved */
  .word SVC_Handler               /* 11 - SVCall */
  .word DebugMonitor_Handler      /* 12 - Reserved for Debug */
  .word 0                         /* 13 - Reserved */
  .word PendSV_Handler            /* 14 - PendSV */
  .word SysTick_Handler           /* 15 - Systick */

  /* External Interrupts */

  .word   EMU_IRQHandler      /* 0 - EMU */
  .word   FRC_PRI_IRQHandler      /* 1 - FRC_PRI */
  .word   WDOG0_IRQHandler      /* 2 - WDOG0 */
  .word   FRC_IRQHandler      /* 3 - FRC */
  .word   MODEM_IRQHandler      /* 4 - MODEM */
  .word   RAC_SEQ_IRQHandler      /* 5 - RAC_SEQ */
  .word   RAC_RSM_IRQHandler      /* 6 - RAC_RSM */
  .word   BUFC_IRQHandler      /* 7 - BUFC */
  .word   LDMA_IRQHandler      /* 8 - LDMA */
  .word   GPIO_EVEN_IRQHandler      /* 9 - GPIO_EVEN */
  .word   TIMER0_IRQHandler      /* 10 - TIMER0 */
  .word   USART0_RX_IRQHandler      /* 11 - USART0_RX */
  .word   USART0_TX_IRQHandler      /* 12 - USART0_TX */
  .word   ACMP0_IRQHandler      /* 13 - ACMP0 */
  .word   ADC0_IRQHandler      /* 14 - ADC0 */
  .word   IDAC0_IRQHandler      /* 15 - IDAC0 */
  .word   I2C0_IRQHandler      /* 16 - I2C0 */
  .word   GPIO_ODD_IRQHandler      /* 17 - GPIO_ODD */
  .word   TIMER1_IRQHandler      /* 18 - TIMER1 */
  .word   USART1_RX_IRQHandler      /* 19 - USART1_RX */
  .word   USART1_TX_IRQHandler      /* 20 - USART1_TX */
  .word   LEUART0_IRQHandler      /* 21 - LEUART0 */
  .word   PCNT0_IRQHandler      /* 22 - PCNT0 */
  .word   CMU_IRQHandler      /* 23 - CMU */
  .word   MSC_IRQHandler      /* 24 - MSC */
  .word   CRYPTO_IRQHandler      /* 25 - CRYPTO */
  .word   LETIMER0_IRQHandler      /* 26 - LETIMER0 */
  .word   AGC_IRQHandler      /* 27 - AGC */
  .word   PROTIMER_IRQHandler      /* 28 - PROTIMER */
  .word   RTCC_IRQHandler      /* 29 - RTCC */
  .word   SYNTH_IRQHandler      /* 30 - SYNTH */
  .word   CRYOTIMER_IRQHandler      /* 31 - CRYOTIMER */
  .word   RFSENSE_IRQHandler      /* 32 - RFSENSE */
  .word   FPUEH_IRQHandler      /* 33 - FPUEH */


/**
**===========================================================================
**  Weak interrupt handlers redirected to Default_Handler. These can be
**  overridden in user code.
**===========================================================================
*/
  .weak NMI_Handler
  .thumb_set NMI_Handler, Default_Handler

  .weak HardFault_Handler
  .thumb_set HardFault_Handler, Default_Handler

  .weak MemManage_Handler
  .thumb_set MemManage_Handler, Default_Handler

  .weak BusFault_Handler
  .thumb_set BusFault_Handler, Default_Handler

  .weak UsageFault_Handler
  .thumb_set UsageFault_Handler, Default_Handler

  .weak SVC_Handler
  .thumb_set SVC_Handler, Default_Handler

  .weak DebugMonitor_Handler
  .thumb_set DebugMonitor_Handler, Default_Handler

  .weak PendSV_Handler
  .thumb_set PendSV_Handler, Default_Handler

  .weak SysTick_Handler
  .thumb_set SysTick_Handler, Default_Handler


  .weak       EMU_IRQHandler
  .thumb_set  EMU_IRQHandler, Default_Handler

  .weak       FRC_PRI_IRQHandler
  .thumb_set  FRC_PRI_IRQHandler, Default_Handler

  .weak       WDOG0_IRQHandler
  .thumb_set  WDOG0_IRQHandler, Default_Handler

  .weak       FRC_IRQHandler
  .thumb_set  FRC_IRQHandler, Default_Handler

  .weak       MODEM_IRQHandler
  .thumb_set  MODEM_IRQHandler, Default_Handler

  .weak       RAC_SEQ_IRQHandler
  .thumb_set  RAC_SEQ_IRQHandler, Default_Handler

  .weak       RAC_RSM_IRQHandler
  .thumb_set  RAC_RSM_IRQHandler, Default_Handler

  .weak       BUFC_IRQHandler
  .thumb_set  BUFC_IRQHandler, Default_Handler

  .weak       LDMA_IRQHandler
  .thumb_set  LDMA_IRQHandler, Default_Handler

  .weak       GPIO_EVEN_IRQHandler
  .thumb_set  GPIO_EVEN_IRQHandler, Default_Handler

  .weak       TIMER0_IRQHandler
  .thumb_set  TIMER0_IRQHandler, Default_Handler

  .weak       USART0_RX_IRQHandler
  .thumb_set  USART0_RX_IRQHandler, Default_Handler

  .weak       USART0_TX_IRQHandler
  .thumb_set  USART0_TX_IRQHandler, Default_Handler

  .weak       ACMP0_IRQHandler
  .thumb_set  ACMP0_IRQHandler, Default_Handler

  .weak       ADC0_IRQHandler
  .thumb_set  ADC0_IRQHandler, Default_Handler

  .weak       IDAC0_IRQHandler
  .thumb_set  IDAC0_IRQHandler, Default_Handler

  .weak       I2C0_IRQHandler
  .thumb_set  I2C0_IRQHandler, Default_Handler

  .weak       GPIO_ODD_IRQHandler
  .thumb_set  GPIO_ODD_IRQHandler, Default_Handler

  .weak       TIMER1_IRQHandler
  .thumb_set  TIMER1_IRQHandler, Default_Handler

  .weak       USART1_RX_IRQHandler
  .thumb_set  USART1_RX_IRQHandler, Default_Handler

  .weak       USART1_TX_IRQHandler
  .thumb_set  USART1_TX_IRQHandler, Default_Handler

  .weak       LEUART0_IRQHandler
  .thumb_set  LEUART0_IRQHandler, Default_Handler

  .weak       PCNT0_IRQHandler
  .thumb_set  PCNT0_IRQHandler, Default_Handler

  .weak       CMU_IRQHandler
  .thumb_set  CMU_IRQHandler, Default_Handler

  .weak       MSC_IRQHandler
  .thumb_set  MSC_IRQHandler, Default_Handler

  .weak       CRYPTO_IRQHandler
  .thumb_set  CRYPTO_IRQHandler, Default_Handler

  .weak       LETIMER0_IRQHandler
  .thumb_set  LETIMER0_IRQHandler, Default_Handler

  .weak       AGC_IRQHandler
  .thumb_set  AGC_IRQHandler, Default_Handler

  .weak       PROTIMER_IRQHandler
  .thumb_set  PROTIMER_IRQHandler, Default_Handler

  .weak       RTCC_IRQHandler
  .thumb_set  RTCC_IRQHandler, Default_Handler

  .weak       SYNTH_IRQHandler
  .thumb_set  SYNTH_IRQHandler, Default_Handler

  .weak       CRYOTIMER_IRQHandler
  .thumb_set  CRYOTIMER_IRQHandler, Default_Handler

  .weak       RFSENSE_IRQHandler
  .thumb_set  RFSENSE_IRQHandler, Default_Handler

  .weak       FPUEH_IRQHandler
  .thumb_set  FPUEH_IRQHandler, Default_Handler


  .end
