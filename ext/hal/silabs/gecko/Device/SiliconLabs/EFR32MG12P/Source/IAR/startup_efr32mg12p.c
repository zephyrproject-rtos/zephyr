/**************************************************************************//**
 * @file startup_efr32mg12p.c
 * @brief CMSIS Compatible EFR32MG12P startup file in C for IAR EWARM
 * @version 5.5.0
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

__weak void EMU_IRQHandler(void)
{
  while (true) {
  }
}

__weak void FRC_PRI_IRQHandler(void)
{
  while (true) {
  }
}

__weak void WDOG0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void WDOG1_IRQHandler(void)
{
  while (true) {
  }
}

__weak void FRC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void MODEM_IRQHandler(void)
{
  while (true) {
  }
}

__weak void RAC_SEQ_IRQHandler(void)
{
  while (true) {
  }
}

__weak void RAC_RSM_IRQHandler(void)
{
  while (true) {
  }
}

__weak void BUFC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LDMA_IRQHandler(void)
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

__weak void IDAC0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void I2C0_IRQHandler(void)
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

__weak void LEUART0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void PCNT0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void CMU_IRQHandler(void)
{
  while (true) {
  }
}

__weak void MSC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void CRYPTO0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LETIMER0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void AGC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void PROTIMER_IRQHandler(void)
{
  while (true) {
  }
}

__weak void RTCC_IRQHandler(void)
{
  while (true) {
  }
}

__weak void SYNTH_IRQHandler(void)
{
  while (true) {
  }
}

__weak void CRYOTIMER_IRQHandler(void)
{
  while (true) {
  }
}

__weak void RFSENSE_IRQHandler(void)
{
  while (true) {
  }
}

__weak void FPUEH_IRQHandler(void)
{
  while (true) {
  }
}

__weak void SMU_IRQHandler(void)
{
  while (true) {
  }
}

__weak void WTIMER0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void WTIMER1_IRQHandler(void)
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

__weak void I2C1_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART3_RX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void USART3_TX_IRQHandler(void)
{
  while (true) {
  }
}

__weak void VDAC0_IRQHandler(void)
{
  while (true) {
  }
}

__weak void CSEN_IRQHandler(void)
{
  while (true) {
  }
}

__weak void LESENSE_IRQHandler(void)
{
  while (true) {
  }
}

__weak void CRYPTO1_IRQHandler(void)
{
  while (true) {
  }
}

__weak void TRNG0_IRQHandler(void)
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
  { EMU_IRQHandler            },              /* 0 */
  { FRC_PRI_IRQHandler        },              /* 1 */
  { WDOG0_IRQHandler          },              /* 2 */
  { WDOG1_IRQHandler          },              /* 3 */
  { FRC_IRQHandler            },              /* 4 */
  { MODEM_IRQHandler          },              /* 5 */
  { RAC_SEQ_IRQHandler        },              /* 6 */
  { RAC_RSM_IRQHandler        },              /* 7 */
  { BUFC_IRQHandler           },              /* 8 */
  { LDMA_IRQHandler           },              /* 9 */
  { GPIO_EVEN_IRQHandler      },              /* 10 */
  { TIMER0_IRQHandler         },              /* 11 */
  { USART0_RX_IRQHandler      },              /* 12 */
  { USART0_TX_IRQHandler      },              /* 13 */
  { ACMP0_IRQHandler          },              /* 14 */
  { ADC0_IRQHandler           },              /* 15 */
  { IDAC0_IRQHandler          },              /* 16 */
  { I2C0_IRQHandler           },              /* 17 */
  { GPIO_ODD_IRQHandler       },              /* 18 */
  { TIMER1_IRQHandler         },              /* 19 */
  { USART1_RX_IRQHandler      },              /* 20 */
  { USART1_TX_IRQHandler      },              /* 21 */
  { LEUART0_IRQHandler        },              /* 22 */
  { PCNT0_IRQHandler          },              /* 23 */
  { CMU_IRQHandler            },              /* 24 */
  { MSC_IRQHandler            },              /* 25 */
  { CRYPTO0_IRQHandler        },              /* 26 */
  { LETIMER0_IRQHandler       },              /* 27 */
  { AGC_IRQHandler            },              /* 28 */
  { PROTIMER_IRQHandler       },              /* 29 */
  { RTCC_IRQHandler           },              /* 30 */
  { SYNTH_IRQHandler          },              /* 31 */
  { CRYOTIMER_IRQHandler      },              /* 32 */
  { RFSENSE_IRQHandler        },              /* 33 */
  { FPUEH_IRQHandler          },              /* 34 */
  { SMU_IRQHandler            },              /* 35 */
  { WTIMER0_IRQHandler        },              /* 36 */
  { WTIMER1_IRQHandler        },              /* 37 */
  { PCNT1_IRQHandler          },              /* 38 */
  { PCNT2_IRQHandler          },              /* 39 */
  { USART2_RX_IRQHandler      },              /* 40 */
  { USART2_TX_IRQHandler      },              /* 41 */
  { I2C1_IRQHandler           },              /* 42 */
  { USART3_RX_IRQHandler      },              /* 43 */
  { USART3_TX_IRQHandler      },              /* 44 */
  { VDAC0_IRQHandler          },              /* 45 */
  { CSEN_IRQHandler           },              /* 46 */
  { LESENSE_IRQHandler        },              /* 47 */
  { CRYPTO1_IRQHandler        },              /* 48 */
  { TRNG0_IRQHandler          },              /* 49 */
  { 0                         },              /* 50 - Reserved */
};
