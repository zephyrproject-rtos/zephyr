/**************************************************************************//**
* @file startup_efm32hg.c
* @brief CMSIS Compatible EFM32HG startup file in C.
*        Should be used with GCC 'GNU Tools ARM Embedded'
* @version 5.6.0
* @date     10. January 2018
******************************************************************************/
/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------
 * Linker generated Symbols
 *----------------------------------------------------------------------------*/
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __copy_table_start__;
extern uint32_t __copy_table_end__;
extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __StackTop;

/*----------------------------------------------------------------------------
 * Exception / Interrupt Handler Function Prototype
 *----------------------------------------------------------------------------*/
typedef union {
  void (*pFunc)(void);
  void *topOfStack;
} tVectorEntry;

/*----------------------------------------------------------------------------
 * External References
 *----------------------------------------------------------------------------*/
#ifndef __START
extern void  _start(void) __attribute__((noreturn));    /* Pre Main (C library entry point) */
#else
extern int  __START(void) __attribute__((noreturn));    /* main entry point */
#endif

#ifndef __NO_SYSTEM_INIT
extern void SystemInit(void);             /* CMSIS System Initialization      */
#endif

/*----------------------------------------------------------------------------
 * Internal References
 *----------------------------------------------------------------------------*/
void Default_Handler(void);                          /* Default empty handler */
void Reset_Handler(void);                            /* Reset Handler */

/*----------------------------------------------------------------------------
 * User Initial Stack & Heap
 *----------------------------------------------------------------------------*/
#ifndef __STACK_SIZE
#define __STACK_SIZE    0x00000400
#endif
static uint8_t stack[__STACK_SIZE] __attribute__ ((aligned(8), used, section(".stack")));

#ifndef __HEAP_SIZE
#define __HEAP_SIZE    0x00000400
#endif
#if __HEAP_SIZE > 0
static uint8_t heap[__HEAP_SIZE]   __attribute__ ((aligned(8), used, section(".heap")));
#endif

/*----------------------------------------------------------------------------
 * Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Cortex-M Processor Exceptions */
void NMI_Handler(void)               __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler(void)         __attribute__ ((weak, alias("Default_Handler")));
void SVC_Handler(void)               __attribute__ ((weak, alias("Default_Handler")));
void PendSV_Handler(void)            __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler(void)           __attribute__ ((weak, alias("Default_Handler")));

/* Part Specific Interrupts */
void DMA_IRQHandler(void)            __attribute__ ((weak, alias("Default_Handler")));
void GPIO_EVEN_IRQHandler(void)      __attribute__ ((weak, alias("Default_Handler")));
void TIMER0_IRQHandler(void)         __attribute__ ((weak, alias("Default_Handler")));
void ACMP0_IRQHandler(void)          __attribute__ ((weak, alias("Default_Handler")));
void ADC0_IRQHandler(void)           __attribute__ ((weak, alias("Default_Handler")));
void I2C0_IRQHandler(void)           __attribute__ ((weak, alias("Default_Handler")));
void GPIO_ODD_IRQHandler(void)       __attribute__ ((weak, alias("Default_Handler")));
void TIMER1_IRQHandler(void)         __attribute__ ((weak, alias("Default_Handler")));
void USART1_RX_IRQHandler(void)      __attribute__ ((weak, alias("Default_Handler")));
void USART1_TX_IRQHandler(void)      __attribute__ ((weak, alias("Default_Handler")));
void LEUART0_IRQHandler(void)        __attribute__ ((weak, alias("Default_Handler")));
void PCNT0_IRQHandler(void)          __attribute__ ((weak, alias("Default_Handler")));
void RTC_IRQHandler(void)            __attribute__ ((weak, alias("Default_Handler")));
void CMU_IRQHandler(void)            __attribute__ ((weak, alias("Default_Handler")));
void VCMP_IRQHandler(void)           __attribute__ ((weak, alias("Default_Handler")));
void MSC_IRQHandler(void)            __attribute__ ((weak, alias("Default_Handler")));
void AES_IRQHandler(void)            __attribute__ ((weak, alias("Default_Handler")));
void USART0_RX_IRQHandler(void)      __attribute__ ((weak, alias("Default_Handler")));
void USART0_TX_IRQHandler(void)      __attribute__ ((weak, alias("Default_Handler")));
void USB_IRQHandler(void)            __attribute__ ((weak, alias("Default_Handler")));
void TIMER2_IRQHandler(void)         __attribute__ ((weak, alias("Default_Handler")));

/*----------------------------------------------------------------------------
 * Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
extern const tVectorEntry __Vectors[];
const tVectorEntry        __Vectors[] __attribute__ ((section(".vectors"))) = {
  /* Cortex-M Exception Handlers */
  { .topOfStack = &__StackTop },              /* Initial Stack Pointer */
  { Reset_Handler             },              /* Reset Handler */
  { NMI_Handler               },              /* NMI Handler */
  { HardFault_Handler         },              /* Hard Fault Handler */
  { Default_Handler           },              /* Reserved */
  { Default_Handler           },              /* Reserved */
  { Default_Handler           },              /* Reserved */
  { Default_Handler           },              /* Reserved */
  { Default_Handler           },              /* Reserved */
  { Default_Handler           },              /* Reserved */
  { Default_Handler           },              /* Reserved */
  { SVC_Handler               },              /* SVCall Handler */
  { Default_Handler           },              /* Reserved */
  { Default_Handler           },              /* Reserved */
  { PendSV_Handler            },              /* PendSV Handler */
  { SysTick_Handler           },              /* SysTick Handler */

  /* External interrupts */

  { DMA_IRQHandler            },              /* 0 */
  { GPIO_EVEN_IRQHandler      },              /* 1 */
  { TIMER0_IRQHandler         },              /* 2 */
  { ACMP0_IRQHandler          },              /* 3 */
  { ADC0_IRQHandler           },              /* 4 */
  { I2C0_IRQHandler           },              /* 5 */
  { GPIO_ODD_IRQHandler       },              /* 6 */
  { TIMER1_IRQHandler         },              /* 7 */
  { USART1_RX_IRQHandler      },              /* 8 */
  { USART1_TX_IRQHandler      },              /* 9 */
  { LEUART0_IRQHandler        },              /* 10 */
  { PCNT0_IRQHandler          },              /* 11 */
  { RTC_IRQHandler            },              /* 12 */
  { CMU_IRQHandler            },              /* 13 */
  { VCMP_IRQHandler           },              /* 14 */
  { MSC_IRQHandler            },              /* 15 */
  { AES_IRQHandler            },              /* 16 */
  { USART0_RX_IRQHandler      },              /* 17 */
  { USART0_TX_IRQHandler      },              /* 18 */
  { USB_IRQHandler            },              /* 19 */
  { TIMER2_IRQHandler         },              /* 20 */
};

/*----------------------------------------------------------------------------
 * Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
void Reset_Handler(void)
{
  uint32_t *pSrc, *pDest;
  uint32_t start, end;
  uint32_t tableStart __attribute__((unused));
  uint32_t tableEnd   __attribute__((unused));

#ifndef __NO_SYSTEM_INIT
  SystemInit();
#endif

/*  Firstly it copies data from read only memory to RAM. There are two schemes
 *  to copy. One can copy more than one sections. Another can only copy
 *  one section.  The former scheme needs more instructions and read-only
 *  data to implement than the latter.
 *  Macro __STARTUP_COPY_MULTIPLE is used to choose between two schemes.  */

#ifdef __STARTUP_COPY_MULTIPLE
/*  Multiple sections scheme.
 *
 *  Between symbol address __copy_table_start__ and __copy_table_end__,
 *  there are array of triplets, each of which specify:
 *    offset 0: LMA of start of a section to copy from
 *    offset 4: VMA of start of a section to copy to
 *    offset 8: size of the section to copy. Must be multiply of 4
 *
 *  All addresses must be aligned to 4 bytes boundary.
 */
  tableStart = (uint32_t) &__copy_table_start__;
  tableEnd   = (uint32_t) &__copy_table_end__;

  for (; tableStart < tableEnd; tableStart += 12U) {
    pSrc  = (uint32_t *) (*(uint32_t *) tableStart);
    start = *(uint32_t *) (tableStart + 4U);
    end   = *(uint32_t *) (tableStart + 8U) + start;
    pDest = (uint32_t *) start;
    for (; start < end; start += 4U) {
      *pDest++ = *pSrc++;
    }
  }
#else
/*  Single section scheme.
 *
 *  The ranges of copy from/to are specified by following symbols
 *    __etext: LMA of start of the section to copy from. Usually end of text
 *    __data_start__: VMA of start of the section to copy to
 *    __data_end__: VMA of end of the section to copy to
 *
 *  All addresses must be aligned to 4 bytes boundary.
 */
  pSrc  = &__etext;
  pDest = &__data_start__;
  start = (uint32_t) &__data_start__;
  end   = (uint32_t) &__data_end__;

  for (; start < end; start += 4U) {
    *pDest++ = *pSrc++;
  }
#endif /*__STARTUP_COPY_MULTIPLE */

/*  This part of work usually is done in C library startup code. Otherwise,
 *  define this macro to enable it in this startup.
 *
 *  There are two schemes too. One can clear multiple BSS sections. Another
 *  can only clear one section. The former is more size expensive than the
 *  latter.
 *
 *  Define macro __STARTUP_CLEAR_BSS_MULTIPLE to choose the former.
 *  Otherwise efine macro __STARTUP_CLEAR_BSS to choose the later.
 */
#ifdef __STARTUP_CLEAR_BSS_MULTIPLE
/*  Multiple sections scheme.
 *
 *  Between symbol address __zero_table_start__ and __zero_table_end__,
 *  there are array of tuples specifying:
 *    offset 0: Start of a BSS section
 *    offset 4: Size of this BSS section. Must be multiply of 4
 */
  tableStart = (uint32_t) &__zero_table_start__;
  tableEnd   = (uint32_t) &__zero_table_end__;

  for (; tableStart < tableEnd; tableStart += 8U) {
    start = *(uint32_t *) tableStart;
    end   = *(uint32_t *) (tableStart + 4U) + start;
    pDest = (uint32_t *) start;
    for (; start < end; start += 4U) {
      *pDest++ = 0UL;
    }
  }
#elif defined (__STARTUP_CLEAR_BSS)
/*  Single BSS section scheme.
 *
 *  The BSS section is specified by following symbols
 *    __bss_start__: start of the BSS section.
 *    __bss_end__: end of the BSS section.
 *
 *  Both addresses must be aligned to 4 bytes boundary.
 */
  pDest = &__bss_start__;
  start = (uint32_t) &__bss_start__;
  end   = (uint32_t) &__bss_end__;

  for (; start < end; start += 4U) {
    *pDest++ = 0UL;
  }
#endif /* __STARTUP_CLEAR_BSS_MULTIPLE || __STARTUP_CLEAR_BSS */

#ifndef __START
#define __START    _start
#endif
  __START();
}

/*----------------------------------------------------------------------------
 * Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
void Default_Handler(void)
{
  while (true) {
  }
}
