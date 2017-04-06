;/**************************************************************************//**
; * @file startup_efm32wg.s
; * @brief    CMSIS Core Device Startup File
; *           Silicon Labs EFM32WG Device Series
; * @version 5.1.2
; * @date     30. January 2012
; *
; * @note
; * Copyright (C) 2012 ARM Limited. All rights reserved.
; *
; * @par
; * ARM Limited (ARM) is supplying this software for use with Cortex-M
; * processor based microcontrollers.  This file can be freely distributed
; * within development tools that are supporting such ARM based processors.
; *
; * @par
; * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
; * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; *
; ******************************************************************************/

;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbol _program_start or
; a user defined start symbol.
; To override the cstartup defined in the library, simply add your modified
; version to the workbench project.
;
; The vector table is normally located at address 0.
;
; When debugging in RAM, it can be located in RAM with at least a 128 byte
; alignment, 256 byte alignment is requied if all interrupt vectors are in use.
;
; The name "__vector_table" has special meaning for C-SPY:
; it is where the SP start value is found, and the NVIC vector
; table register (VTOR) is initialized to this address if != 0.
;
; Cortex-M version
;
        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(8)

        EXTERN  __iar_program_start
        EXTERN  SystemInit
        PUBLIC  __vector_table
        PUBLIC  __vector_table_0x1c
        PUBLIC  __Vectors
        PUBLIC  __Vectors_End
        PUBLIC  __Vectors_Size

        DATA

__vector_table
        DCD     sfe(CSTACK)
        DCD     Reset_Handler

        DCD     NMI_Handler
        DCD     HardFault_Handler
        DCD     MemManage_Handler
        DCD     BusFault_Handler
        DCD     UsageFault_Handler
__vector_table_0x1c
        DCD     0
        DCD     0
        DCD     0
        DCD     0
        DCD     SVC_Handler
        DCD     DebugMon_Handler
        DCD     0
        DCD     PendSV_Handler
        DCD     SysTick_Handler

        ; External Interrupts

        DCD DMA_IRQHandler  ; 0: DMA Interrupt
        DCD GPIO_EVEN_IRQHandler  ; 1: GPIO_EVEN Interrupt
        DCD TIMER0_IRQHandler  ; 2: TIMER0 Interrupt
        DCD USART0_RX_IRQHandler  ; 3: USART0_RX Interrupt
        DCD USART0_TX_IRQHandler  ; 4: USART0_TX Interrupt
        DCD USB_IRQHandler  ; 5: USB Interrupt
        DCD ACMP0_IRQHandler  ; 6: ACMP0 Interrupt
        DCD ADC0_IRQHandler  ; 7: ADC0 Interrupt
        DCD DAC0_IRQHandler  ; 8: DAC0 Interrupt
        DCD I2C0_IRQHandler  ; 9: I2C0 Interrupt
        DCD I2C1_IRQHandler  ; 10: I2C1 Interrupt
        DCD GPIO_ODD_IRQHandler  ; 11: GPIO_ODD Interrupt
        DCD TIMER1_IRQHandler  ; 12: TIMER1 Interrupt
        DCD TIMER2_IRQHandler  ; 13: TIMER2 Interrupt
        DCD TIMER3_IRQHandler  ; 14: TIMER3 Interrupt
        DCD USART1_RX_IRQHandler  ; 15: USART1_RX Interrupt
        DCD USART1_TX_IRQHandler  ; 16: USART1_TX Interrupt
        DCD LESENSE_IRQHandler  ; 17: LESENSE Interrupt
        DCD USART2_RX_IRQHandler  ; 18: USART2_RX Interrupt
        DCD USART2_TX_IRQHandler  ; 19: USART2_TX Interrupt
        DCD UART0_RX_IRQHandler  ; 20: UART0_RX Interrupt
        DCD UART0_TX_IRQHandler  ; 21: UART0_TX Interrupt
        DCD UART1_RX_IRQHandler  ; 22: UART1_RX Interrupt
        DCD UART1_TX_IRQHandler  ; 23: UART1_TX Interrupt
        DCD LEUART0_IRQHandler  ; 24: LEUART0 Interrupt
        DCD LEUART1_IRQHandler  ; 25: LEUART1 Interrupt
        DCD LETIMER0_IRQHandler  ; 26: LETIMER0 Interrupt
        DCD PCNT0_IRQHandler  ; 27: PCNT0 Interrupt
        DCD PCNT1_IRQHandler  ; 28: PCNT1 Interrupt
        DCD PCNT2_IRQHandler  ; 29: PCNT2 Interrupt
        DCD RTC_IRQHandler  ; 30: RTC Interrupt
        DCD BURTC_IRQHandler  ; 31: BURTC Interrupt
        DCD CMU_IRQHandler  ; 32: CMU Interrupt
        DCD VCMP_IRQHandler  ; 33: VCMP Interrupt
        DCD LCD_IRQHandler  ; 34: LCD Interrupt
        DCD MSC_IRQHandler  ; 35: MSC Interrupt
        DCD AES_IRQHandler  ; 36: AES Interrupt
        DCD EBI_IRQHandler  ; 37: EBI Interrupt
        DCD EMU_IRQHandler  ; 38: EMU Interrupt
        DCD FPUEH_IRQHandler  ; 39: FPUEH Interrupt


__Vectors_End
__Vectors       EQU   __vector_table
__Vectors_Size  EQU   __Vectors_End - __Vectors


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
        THUMB

        PUBWEAK Reset_Handler
        SECTION .text:CODE:REORDER:NOROOT(2)
Reset_Handler
        LDR     R0, =SystemInit
        BLX     R0
        LDR     R0, =__iar_program_start
        BX      R0

        PUBWEAK NMI_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
NMI_Handler
        B NMI_Handler

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
HardFault_Handler
        B HardFault_Handler

        PUBWEAK MemManage_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
MemManage_Handler
        B MemManage_Handler

        PUBWEAK BusFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
BusFault_Handler
        B BusFault_Handler

        PUBWEAK UsageFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
UsageFault_Handler
        B UsageFault_Handler

        PUBWEAK SVC_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SVC_Handler
        B SVC_Handler

        PUBWEAK DebugMon_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DebugMon_Handler
        B DebugMon_Handler

        PUBWEAK PendSV_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
PendSV_Handler
        B PendSV_Handler

        PUBWEAK SysTick_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SysTick_Handler
        B SysTick_Handler

        ; Device specific interrupt handlers

        PUBWEAK DMA_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_IRQHandler
        B DMA_IRQHandler

        PUBWEAK GPIO_EVEN_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
GPIO_EVEN_IRQHandler
        B GPIO_EVEN_IRQHandler

        PUBWEAK TIMER0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TIMER0_IRQHandler
        B TIMER0_IRQHandler

        PUBWEAK USART0_RX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART0_RX_IRQHandler
        B USART0_RX_IRQHandler

        PUBWEAK USART0_TX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART0_TX_IRQHandler
        B USART0_TX_IRQHandler

        PUBWEAK USB_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USB_IRQHandler
        B USB_IRQHandler

        PUBWEAK ACMP0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ACMP0_IRQHandler
        B ACMP0_IRQHandler

        PUBWEAK ADC0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ADC0_IRQHandler
        B ADC0_IRQHandler

        PUBWEAK DAC0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DAC0_IRQHandler
        B DAC0_IRQHandler

        PUBWEAK I2C0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C0_IRQHandler
        B I2C0_IRQHandler

        PUBWEAK I2C1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C1_IRQHandler
        B I2C1_IRQHandler

        PUBWEAK GPIO_ODD_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
GPIO_ODD_IRQHandler
        B GPIO_ODD_IRQHandler

        PUBWEAK TIMER1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TIMER1_IRQHandler
        B TIMER1_IRQHandler

        PUBWEAK TIMER2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TIMER2_IRQHandler
        B TIMER2_IRQHandler

        PUBWEAK TIMER3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TIMER3_IRQHandler
        B TIMER3_IRQHandler

        PUBWEAK USART1_RX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART1_RX_IRQHandler
        B USART1_RX_IRQHandler

        PUBWEAK USART1_TX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART1_TX_IRQHandler
        B USART1_TX_IRQHandler

        PUBWEAK LESENSE_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
LESENSE_IRQHandler
        B LESENSE_IRQHandler

        PUBWEAK USART2_RX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART2_RX_IRQHandler
        B USART2_RX_IRQHandler

        PUBWEAK USART2_TX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART2_TX_IRQHandler
        B USART2_TX_IRQHandler

        PUBWEAK UART0_RX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART0_RX_IRQHandler
        B UART0_RX_IRQHandler

        PUBWEAK UART0_TX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART0_TX_IRQHandler
        B UART0_TX_IRQHandler

        PUBWEAK UART1_RX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART1_RX_IRQHandler
        B UART1_RX_IRQHandler

        PUBWEAK UART1_TX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART1_TX_IRQHandler
        B UART1_TX_IRQHandler

        PUBWEAK LEUART0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
LEUART0_IRQHandler
        B LEUART0_IRQHandler

        PUBWEAK LEUART1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
LEUART1_IRQHandler
        B LEUART1_IRQHandler

        PUBWEAK LETIMER0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
LETIMER0_IRQHandler
        B LETIMER0_IRQHandler

        PUBWEAK PCNT0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
PCNT0_IRQHandler
        B PCNT0_IRQHandler

        PUBWEAK PCNT1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
PCNT1_IRQHandler
        B PCNT1_IRQHandler

        PUBWEAK PCNT2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
PCNT2_IRQHandler
        B PCNT2_IRQHandler

        PUBWEAK RTC_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
RTC_IRQHandler
        B RTC_IRQHandler

        PUBWEAK BURTC_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
BURTC_IRQHandler
        B BURTC_IRQHandler

        PUBWEAK CMU_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CMU_IRQHandler
        B CMU_IRQHandler

        PUBWEAK VCMP_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
VCMP_IRQHandler
        B VCMP_IRQHandler

        PUBWEAK LCD_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
LCD_IRQHandler
        B LCD_IRQHandler

        PUBWEAK MSC_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
MSC_IRQHandler
        B MSC_IRQHandler

        PUBWEAK AES_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
AES_IRQHandler
        B AES_IRQHandler

        PUBWEAK EBI_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EBI_IRQHandler
        B EBI_IRQHandler

        PUBWEAK EMU_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EMU_IRQHandler
        B EMU_IRQHandler

        PUBWEAK FPUEH_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
FPUEH_IRQHandler
        B FPUEH_IRQHandler


        END
