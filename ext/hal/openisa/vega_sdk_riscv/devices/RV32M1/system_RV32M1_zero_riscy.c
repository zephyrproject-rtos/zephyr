/*
** ###################################################################
**     Processors:          RV32M1_zero_riscy
**                          RV32M1_zero_riscy
**
**     Compilers:           Keil ARM C/C++ Compiler
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          MCUXpresso Compiler
**
**     Reference manual:    RV32M1 Series Reference Manual, Rev. 1 , 8/10/2018
**     Version:             rev. 1.0, 2018-10-02
**     Build:               b180926
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2018 NXP
**     All rights reserved.
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2018-10-02)
**         Initial version.
**
** ###################################################################
*/

/*!
 * @file RV32M1_zero_riscy
 * @version 1.0
 * @date 2018-10-02
 * @brief Device specific configuration file for RV32M1_zero_riscy
 *        (implementation file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "fsl_device_registers.h"
#include "fsl_common.h"

typedef void (*irq_handler_t)(void);

extern void CTI1_IRQHandler(void);
extern void DMA1_04_DriverIRQHandler(void);
extern void DMA1_15_DriverIRQHandler(void);
extern void DMA1_26_DriverIRQHandler(void);
extern void DMA1_37_DriverIRQHandler(void);
extern void DMA1_Error_DriverIRQHandler(void);
extern void CMC1_IRQHandler(void);
extern void LLWU1_IRQHandler(void);
extern void MUB_IRQHandler(void);
extern void WDOG1_IRQHandler(void);
extern void CAU3_Task_Complete_IRQHandler(void);
extern void CAU3_Security_Violation_IRQHandler(void);
extern void TRNG_IRQHandler(void);
extern void LPIT1_IRQHandler(void);
extern void LPTMR2_IRQHandler(void);
extern void TPM3_IRQHandler(void);
extern void LPI2C3_DriverIRQHandler(void);
extern void RF0_0_IRQHandler(void);
extern void RF0_1_IRQHandler(void);
extern void LPSPI3_DriverIRQHandler(void);
extern void LPUART3_DriverIRQHandler(void);
extern void PORTE_IRQHandler(void);
extern void LPCMP1_IRQHandler(void);
extern void RTC_IRQHandler(void);
extern void INTMUX1_0_DriverIRQHandler(void);
extern void INTMUX1_1_DriverIRQHandler(void);
extern void INTMUX1_2_DriverIRQHandler(void);
extern void INTMUX1_3_DriverIRQHandler(void);
extern void INTMUX1_4_DriverIRQHandler(void);
extern void INTMUX1_5_DriverIRQHandler(void);
extern void INTMUX1_6_DriverIRQHandler(void);
extern void INTMUX1_7_DriverIRQHandler(void);
extern void EWM_IRQHandler(void);
extern void FTFE_Command_Complete_IRQHandler(void);
extern void FTFE_Read_Collision_IRQHandler(void);
extern void SPM_IRQHandler(void);
extern void SCG_IRQHandler(void);
extern void LPIT0_IRQHandler(void);
extern void LPTMR0_IRQHandler(void);
extern void LPTMR1_IRQHandler(void);
extern void TPM0_IRQHandler(void);
extern void TPM1_IRQHandler(void);
extern void TPM2_IRQHandler(void);
extern void EMVSIM0_IRQHandler(void);
extern void FLEXIO0_DriverIRQHandler(void);
extern void LPI2C0_DriverIRQHandler(void);
extern void LPI2C1_DriverIRQHandler(void);
extern void LPI2C2_DriverIRQHandler(void);
extern void I2S0_DriverIRQHandler(void);
extern void USDHC0_DriverIRQHandler(void);
extern void LPSPI0_DriverIRQHandler(void);
extern void LPSPI1_DriverIRQHandler(void);
extern void LPSPI2_DriverIRQHandler(void);
extern void LPUART0_DriverIRQHandler(void);
extern void LPUART1_DriverIRQHandler(void);
extern void LPUART2_DriverIRQHandler(void);
extern void USB0_IRQHandler(void);
extern void PORTA_IRQHandler(void);
extern void PORTB_IRQHandler(void);
extern void PORTC_IRQHandler(void);
extern void PORTD_IRQHandler(void);
extern void ADC0_IRQHandler(void);
extern void LPCMP0_IRQHandler(void);
extern void LPDAC0_IRQHandler(void);
extern void DMA1_15_IRQHandler(void);
extern void DMA1_26_IRQHandler(void);
extern void DMA1_37_IRQHandler(void);
extern void DMA1_Error_IRQHandler(void);
extern void LPI2C3_IRQHandler(void);
extern void LPSPI3_IRQHandler(void);
extern void LPUART3_IRQHandler(void);
extern void INTMUX1_0_IRQHandler(void);
extern void INTMUX1_1_IRQHandler(void);
extern void INTMUX1_2_IRQHandler(void);
extern void INTMUX1_3_IRQHandler(void);
extern void INTMUX1_4_IRQHandler(void);
extern void INTMUX1_5_IRQHandler(void);
extern void INTMUX1_6_IRQHandler(void);
extern void INTMUX1_7_IRQHandler(void);
extern void FLEXIO0_IRQHandler(void);
extern void LPI2C0_IRQHandler(void);
extern void LPI2C1_IRQHandler(void);
extern void LPI2C2_IRQHandler(void);
extern void I2S0_IRQHandler(void);
extern void USDHC0_IRQHandler(void);
extern void LPSPI0_IRQHandler(void);
extern void LPSPI1_IRQHandler(void);
extern void LPSPI2_IRQHandler(void);
extern void LPUART0_IRQHandler(void);
extern void LPUART1_IRQHandler(void);
extern void LPUART2_IRQHandler(void);

/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;

extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

static void copy_section(uint32_t * p_load, uint32_t * p_vma, uint32_t * p_vma_end)
{
    while(p_vma <= p_vma_end)
    {
        *p_vma = *p_load;
        ++p_load;
        ++p_vma;
    }
}

static void zero_section(uint32_t * start, uint32_t * end)
{
    uint32_t * p_zero = start;

    while(p_zero <= end)
    {
        *p_zero = 0;
        ++p_zero;
    }
}

#define DEFINE_IRQ_HANDLER(irq_handler, driver_irq_handler) \
    void __attribute__((weak)) irq_handler(void) { driver_irq_handler();}

#define DEFINE_DEFAULT_IRQ_HANDLER(irq_handler) void irq_handler() __attribute__((weak, alias("DefaultIRQHandler")))

DEFINE_DEFAULT_IRQ_HANDLER(CTI1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(DMA1_04_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(DMA1_15_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(DMA1_26_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(DMA1_37_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(DMA1_Error_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(CMC1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LLWU1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(MUB_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(WDOG1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(CAU3_Task_Complete_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(CAU3_Security_Violation_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(TRNG_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPIT1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPTMR2_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(TPM3_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPI2C3_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(RF0_0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(RF0_1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPSPI3_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPUART3_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(PORTE_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPCMP1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(RTC_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_0_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_1_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_2_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_3_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_4_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_5_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_6_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(INTMUX1_7_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(EWM_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(FTFE_Command_Complete_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(FTFE_Read_Collision_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(SPM_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(SCG_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPIT0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPTMR0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPTMR1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(TPM0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(TPM1_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(TPM2_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(EMVSIM0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(FLEXIO0_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPI2C0_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPI2C1_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPI2C2_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(I2S0_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(USDHC0_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPSPI0_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPSPI1_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPSPI2_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPUART0_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPUART1_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPUART2_DriverIRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(USB0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(PORTA_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(PORTB_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(PORTC_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(PORTD_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(ADC0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPCMP0_IRQHandler);
DEFINE_DEFAULT_IRQ_HANDLER(LPDAC0_IRQHandler);

DEFINE_IRQ_HANDLER(DMA1_04_IRQHandler, DMA1_04_DriverIRQHandler);
DEFINE_IRQ_HANDLER(DMA1_15_IRQHandler, DMA1_15_DriverIRQHandler);
DEFINE_IRQ_HANDLER(DMA1_26_IRQHandler, DMA1_26_DriverIRQHandler);
DEFINE_IRQ_HANDLER(DMA1_37_IRQHandler, DMA1_37_DriverIRQHandler);
DEFINE_IRQ_HANDLER(DMA1_Error_IRQHandler, DMA1_Error_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPI2C3_IRQHandler, LPI2C3_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPSPI3_IRQHandler, LPSPI3_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPUART3_IRQHandler, LPUART3_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_0_IRQHandler, INTMUX1_0_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_1_IRQHandler, INTMUX1_1_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_2_IRQHandler, INTMUX1_2_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_3_IRQHandler, INTMUX1_3_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_4_IRQHandler, INTMUX1_4_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_5_IRQHandler, INTMUX1_5_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_6_IRQHandler, INTMUX1_6_DriverIRQHandler);
DEFINE_IRQ_HANDLER(INTMUX1_7_IRQHandler, INTMUX1_7_DriverIRQHandler);
DEFINE_IRQ_HANDLER(FLEXIO0_IRQHandler, FLEXIO0_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPI2C0_IRQHandler, LPI2C0_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPI2C1_IRQHandler, LPI2C1_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPI2C2_IRQHandler, LPI2C2_DriverIRQHandler);
DEFINE_IRQ_HANDLER(I2S0_IRQHandler, I2S0_DriverIRQHandler);
DEFINE_IRQ_HANDLER(USDHC0_IRQHandler, USDHC0_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPSPI0_IRQHandler, LPSPI0_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPSPI1_IRQHandler, LPSPI1_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPSPI2_IRQHandler, LPSPI2_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPUART0_IRQHandler, LPUART0_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPUART1_IRQHandler, LPUART1_DriverIRQHandler);
DEFINE_IRQ_HANDLER(LPUART2_IRQHandler, LPUART2_DriverIRQHandler);

__attribute__((section("user_vectors"))) const irq_handler_t isrTable[] =
{
	CTI1_IRQHandler,
	DMA1_04_IRQHandler,
	DMA1_15_IRQHandler,
	DMA1_26_IRQHandler,
	DMA1_37_IRQHandler,
	DMA1_Error_IRQHandler,
	CMC1_IRQHandler,
	LLWU1_IRQHandler,
	MUB_IRQHandler,
	WDOG1_IRQHandler,
	CAU3_Task_Complete_IRQHandler,
	CAU3_Security_Violation_IRQHandler,
	TRNG_IRQHandler,
	LPIT1_IRQHandler,
	LPTMR2_IRQHandler,
	TPM3_IRQHandler,
	LPI2C3_IRQHandler,
	RF0_0_IRQHandler,
	RF0_1_IRQHandler,
	LPSPI3_IRQHandler,
	LPUART3_IRQHandler,
	PORTE_IRQHandler,
	LPCMP1_IRQHandler,
	RTC_IRQHandler,
	INTMUX1_0_IRQHandler,
	INTMUX1_1_IRQHandler,
	INTMUX1_2_IRQHandler,
	INTMUX1_3_IRQHandler,
	INTMUX1_4_IRQHandler,
	INTMUX1_5_IRQHandler,
	INTMUX1_6_IRQHandler,
	INTMUX1_7_IRQHandler,
	EWM_IRQHandler,
	FTFE_Command_Complete_IRQHandler,
	FTFE_Read_Collision_IRQHandler,
	SPM_IRQHandler,
	SCG_IRQHandler,
	LPIT0_IRQHandler,
	LPTMR0_IRQHandler,
	LPTMR1_IRQHandler,
	TPM0_IRQHandler,
	TPM1_IRQHandler,
	TPM2_IRQHandler,
	EMVSIM0_IRQHandler,
	FLEXIO0_IRQHandler,
	LPI2C0_IRQHandler,
	LPI2C1_IRQHandler,
	LPI2C2_IRQHandler,
	I2S0_IRQHandler,
	USDHC0_IRQHandler,
	LPSPI0_IRQHandler,
	LPSPI1_IRQHandler,
	LPSPI2_IRQHandler,
	LPUART0_IRQHandler,
	LPUART1_IRQHandler,
	LPUART2_IRQHandler,
	USB0_IRQHandler,
	PORTA_IRQHandler,
	PORTB_IRQHandler,
	PORTC_IRQHandler,
	PORTD_IRQHandler,
	ADC0_IRQHandler,
	LPCMP0_IRQHandler,
	LPDAC0_IRQHandler,
};

extern uint32_t __VECTOR_TABLE[];

static uint32_t irqNesting = 0;

static void DefaultIRQHandler(void)
{
    for (;;)
    {
    }
}

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

void SystemInit (void) {
#if (DISABLE_WDOG)
  WDOG1->CNT = 0xD928C520U;
  WDOG1->TOVAL = 0xFFFF;
  WDOG1->CS = (uint32_t) ((WDOG1->CS) & ~WDOG_CS_EN_MASK) | WDOG_CS_UPDATE_MASK;
#endif /* (DISABLE_WDOG) */

  SystemInitHook();

  copy_section(&__etext, &__data_start__, &__data_end__);
  zero_section(&__bss_start__, &__bss_end__);

  /* Setup the vector table address. */
  irqNesting = 0;

  __ASM volatile("csrw 0x305, %0" :: "r"((uint32_t)__VECTOR_TABLE)); /* MTVEC */
  __ASM volatile("csrw 0x005, %0" :: "r"((uint32_t)__VECTOR_TABLE)); /* UTVEC */

  /* Clear all pending flags. */
  EVENT_UNIT->INTPTPENDCLEAR = 0xFFFFFFFF;
  EVENT_UNIT->EVTPENDCLEAR = 0xFFFFFFFF;
  /* Set all interrupt as secure interrupt. */
  EVENT_UNIT->INTPTSECURE = 0xFFFFFFFF;
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {

  uint32_t SCGOUTClock;                                 /* Variable to store output clock frequency of the SCG module */
  uint16_t Divider;
  Divider = ((SCG->CSR & SCG_CSR_DIVCORE_MASK) >> SCG_CSR_DIVCORE_SHIFT) + 1;

  switch ((SCG->CSR & SCG_CSR_SCS_MASK) >> SCG_CSR_SCS_SHIFT) {
    case 0x1:
      /* System OSC */
      SCGOUTClock = CPU_XTAL_CLK_HZ;
      break;
    case 0x2:
      /* Slow IRC */
      SCGOUTClock = (((SCG->SIRCCFG & SCG_SIRCCFG_RANGE_MASK) >> SCG_SIRCCFG_RANGE_SHIFT) ? 8000000 : 2000000);
      break;
    case 0x3:
      /* Fast IRC */
      SCGOUTClock = 48000000 + ((SCG->FIRCCFG & SCG_FIRCCFG_RANGE_MASK) >> SCG_FIRCCFG_RANGE_SHIFT) * 4000000;
      break;
    case 0x5:
      /* Low Power FLL */
      SCGOUTClock = 48000000 + ((SCG->LPFLLCFG & SCG_LPFLLCFG_FSEL_MASK) >> SCG_LPFLLCFG_FSEL_SHIFT) * 24000000;
      break;
    default:
      return;
  }
  SystemCoreClock = (SCGOUTClock / Divider);
}

/* ----------------------------------------------------------------------------
   -- SystemInitHook()
   ---------------------------------------------------------------------------- */

__attribute__ ((weak)) void SystemInitHook (void) {
  /* Void implementation of the weak function. */
}

#if defined(__IAR_SYSTEMS_ICC__)
#pragma weak SystemIrqHandler
void SystemIrqHandler(uint32_t mcause) {
#elif defined(__GNUC__)
__attribute__((weak)) void SystemIrqHandler(uint32_t mcause) {
#else
  #error Not supported compiler type
#endif
    uint32_t intNum;

    if (mcause & 0x80000000) /* For external interrupt. */
    {
        intNum = mcause & 0x1FUL;

        irqNesting++;

        /* Clear pending flag in EVENT unit .*/
        EVENT_UNIT->INTPTPENDCLEAR = (1U << intNum);

        /* Read back to make sure write finished. */
        (void)(EVENT_UNIT->INTPTPENDCLEAR);

        __enable_irq();      /* Support nesting interrupt */

        /* Now call the real irq handler for intNum */
        isrTable[intNum]();

        __disable_irq();

        irqNesting--;
    }
}

/* Use LIPT1 channel 0 for systick. */
#define SYSTICK_LPIT LPIT1
#define SYSTICK_LPIT_CH 0
#define SYSTICK_LPIT_IRQn LPIT1_IRQn

/* Leverage LPIT0 to provide Systick */
void SystemSetupSystick(uint32_t tickRateHz, uint32_t intPriority)
{
    /* Init pit module */
    CLOCK_EnableClock(kCLOCK_Lpit1);

    /* Reset the timer channels and registers except the MCR register */
    SYSTICK_LPIT->MCR |= LPIT_MCR_SW_RST_MASK;
    SYSTICK_LPIT->MCR &= ~LPIT_MCR_SW_RST_MASK;

    /* Setup timer operation in debug and doze modes and enable the module */
    SYSTICK_LPIT->MCR = LPIT_MCR_DBG_EN_MASK | LPIT_MCR_DOZE_EN_MASK | LPIT_MCR_M_CEN_MASK;

    /* Set timer period for channel 0 */
    SYSTICK_LPIT->CHANNEL[SYSTICK_LPIT_CH].TVAL = (CLOCK_GetIpFreq(kCLOCK_Lpit1) / tickRateHz) - 1;

    /* Enable timer interrupts for channel 0 */
    SYSTICK_LPIT->MIER |= (1U << SYSTICK_LPIT_CH);

    /* Set interrupt priority. */
    EVENT_SetIRQPriority(SYSTICK_LPIT_IRQn, intPriority);

    /* Enable interrupt at the EVENT unit */
    EnableIRQ(SYSTICK_LPIT_IRQn);

    /* Start channel 0 */
    SYSTICK_LPIT->SETTEN |= (LPIT_SETTEN_SET_T_EN_0_MASK << SYSTICK_LPIT_CH);
}

uint32_t SystemGetIRQNestingLevel(void)
{
    return irqNesting;
}

void SystemClearSystickFlag(void)
{
    /* Channel 0. */
    SYSTICK_LPIT->MSR = (1U << SYSTICK_LPIT_CH);
}

void EVENT_SetIRQPriority(IRQn_Type IRQn, uint8_t intPriority)
{
    uint8_t regIdx;
    uint8_t regOffset;

    if ((IRQn < 32) && (intPriority < 8))
    {
        /*
         * 4 priority control registers, each register controls 8 interrupts.
         * Bit 0-2: interrupt 0
         * Bit 4-7: interrupt 1
         * ...
         * Bit 28-30: interrupt 7
         */
        regIdx = IRQn >> 3U;
        regOffset = (IRQn & 0x07U) * 4U;

        EVENT_UNIT->INTPTPRI[regIdx] = (EVENT_UNIT->INTPTPRI[regIdx] & ~(0x0F << regOffset)) | (intPriority << regOffset);
    }
}

bool SystemInISR(void)
{
    return ((EVENT_UNIT->INTPTENACTIVE) != 0);;
}

void EVENT_SystemReset(void)
{
    EVENT_UNIT->SLPCTRL |= EVENT_SLPCTRL_SYSRSTREQST_MASK;
}
