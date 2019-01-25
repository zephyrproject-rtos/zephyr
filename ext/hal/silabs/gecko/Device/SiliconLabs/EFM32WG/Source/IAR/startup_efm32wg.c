/**************************************************************************//**
 * @file startup_efm32wg.c
 * @brief CMSIS Compatible EFM32WG startup file in C for IAR EWARM
 * @version 5.6.0
 ******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories, Inc. www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/

#include "em_device.h"        /* The correct device header file. */
#include <stdbool.h>

#pragma language=extended
#pragma segment="CSTACK"

/* IAR start function */
extern void __iar_program_start(void);
/* CMSIS init function */
extern void SystemInit(void);

/* Auto defined by linker */
extern unsigned char CSTACK$$Limit;

__weak void Reset_Handler(void)
{
  SystemInit();
  __iar_program_start();
}

__weak void NMI_Handler(void)
{
  while (true) {
  }
}

__weak void HardFault_Handler(void)
{
  while (true) {
  }
}

__weak void MemManage_Handler(void)
{
  while (true) {
  }
}

__weak void BusFault_Handler(void)
{
  while (true) {
  }
}

__weak void UsageFault_Handler(void)
{
  while (true) {
  }
}

__weak void SVC_Handler(void)
{
  while (true) {
  }
}

__weak void DebugMon_Handler(void)
{
  while (true) {
  }
}

__weak void PendSV_Handler(void)
{
  while (true) {
  }
}

__weak void SysTick_Handler(void)
{
  while (true) {
  }
}

__weak void DMA_IRQHandler(void)
{
  while (true) {
  }
}

__weak void GPIO_EVEN_IRQHandler(void)
{
  while (true) {
  }
}

__weak void TIMER0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART0_RX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART0_TX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USB_IRQHandler(void)
{
  while (true) {
  }
}

__weak void ACMP0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void ADC0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void DAC0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void I2C0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void I2C1_IRQHandler(void)
{
  while (true) {
  }
}

__weak void GPIO_ODD_IRQHandler(void)
{
  while (true) {
  }
}

__weak void TIMER1_IRQHandler(void)
{
  while (true) {
  }
}

__weak void TIMER2_IRQHandler(void)
{
  while (true) {
  }
}

__weak void TIMER3_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART1_RX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART1_TX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LESENSE_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART2_RX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART2_TX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void UART0_RX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void UART0_TX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void UART1_RX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void UART1_TX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LEUART0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LEUART1_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LETIMER0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void PCNT0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void PCNT1_IRQHandler(void)
{
  while (true) {
  }
}

__weak void PCNT2_IRQHandler(void)
{
  while (true) {
  }
}

__weak void RTC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void BURTC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void CMU_IRQHandler(void)
{
  while (true) {
  }
}

__weak void VCMP_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LCD_IRQHandler(void)
{
  while (true) {
  }
}

__weak void MSC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void AES_IRQHandler(void)
{
  while (true) {
  }
}

__weak void EBI_IRQHandler(void)
{
  while (true) {
  }
}

__weak void EMU_IRQHandler(void)
{
  while (true) {
  }
}

__weak void FPUEH_IRQHandler(void)
{
  while (true) {
  }
}

typedef union {
  void (*pFunc)(void);
  void *topOfStack;
} tVectorEntry;

extern const tVectorEntry __vector_table[];

#pragma data_alignment=256
#pragma location = ".intvec"
const tVectorEntry __vector_table[] = {
  { .topOfStack = &CSTACK$$Limit },  /* With IAR, the CSTACK is defined via */
                                     /* project options settings */

  { Reset_Handler             },
  { NMI_Handler               },
  { HardFault_Handler         },
  { MemManage_Handler         },
  { BusFault_Handler          },
  { UsageFault_Handler        },
  { 0                         },
  { 0                         },
  { 0                         },
  { 0                         },
  { SVC_Handler               },
  { DebugMon_Handler          },
  { 0                         },
  { PendSV_Handler            },
  { SysTick_Handler           },

  { DMA_IRQHandler            },              /* 0 */
  { GPIO_EVEN_IRQHandler      },              /* 1 */
  { TIMER0_IRQHandler         },              /* 2 */
  { USART0_RX_IRQHandler      },              /* 3 */
  { USART0_TX_IRQHandler      },              /* 4 */
  { USB_IRQHandler            },              /* 5 */
  { ACMP0_IRQHandler          },              /* 6 */
  { ADC0_IRQHandler           },              /* 7 */
  { DAC0_IRQHandler           },              /* 8 */
  { I2C0_IRQHandler           },              /* 9 */
  { I2C1_IRQHandler           },              /* 10 */
  { GPIO_ODD_IRQHandler       },              /* 11 */
  { TIMER1_IRQHandler         },              /* 12 */
  { TIMER2_IRQHandler         },              /* 13 */
  { TIMER3_IRQHandler         },              /* 14 */
  { USART1_RX_IRQHandler      },              /* 15 */
  { USART1_TX_IRQHandler      },              /* 16 */
  { LESENSE_IRQHandler        },              /* 17 */
  { USART2_RX_IRQHandler      },              /* 18 */
  { USART2_TX_IRQHandler      },              /* 19 */
  { UART0_RX_IRQHandler       },              /* 20 */
  { UART0_TX_IRQHandler       },              /* 21 */
  { UART1_RX_IRQHandler       },              /* 22 */
  { UART1_TX_IRQHandler       },              /* 23 */
  { LEUART0_IRQHandler        },              /* 24 */
  { LEUART1_IRQHandler        },              /* 25 */
  { LETIMER0_IRQHandler       },              /* 26 */
  { PCNT0_IRQHandler          },              /* 27 */
  { PCNT1_IRQHandler          },              /* 28 */
  { PCNT2_IRQHandler          },              /* 29 */
  { RTC_IRQHandler            },              /* 30 */
  { BURTC_IRQHandler          },              /* 31 */
  { CMU_IRQHandler            },              /* 32 */
  { VCMP_IRQHandler           },              /* 33 */
  { LCD_IRQHandler            },              /* 34 */
  { MSC_IRQHandler            },              /* 35 */
  { AES_IRQHandler            },              /* 36 */
  { EBI_IRQHandler            },              /* 37 */
  { EMU_IRQHandler            },              /* 38 */
  { FPUEH_IRQHandler          },              /* 39 */
};
