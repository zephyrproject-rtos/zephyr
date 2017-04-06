/***************************************************************************//**
 * @file em_core.c
 * @brief Core interrupt handling API
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
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
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/
#include "em_core.h"
#include "em_assert.h"

#if defined(EMLIB_USER_CONFIG)
#include "emlib_config.h"
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
  @addtogroup CORE
  @brief Core interrupt handling API

  @li @ref core_intro
  @li @ref core_conf
  @li @ref core_macro_api
  @li @ref core_reimplementation
  @li @ref core_vector_tables
  @li @ref core_examples
  @li @ref core_porting

@n @section core_intro Introduction

  The purpose of the CORE interrupt API is to provide simple and safe means
  to disable and enable interrupts to protect sections of code.

  This is often referred to as "critical sections" and this module provide
  support for three types of critical sections, each with different interrupt
  blocking capabilities.

  @li <b>CRITICAL</b> section: Inside a critical sections all interrupts are
      disabled (except for fault handlers). The PRIMASK register is always used for
      interrupt disable/enable.
  @li <b>ATOMIC</b> section: This type of section is configurable and the default 
      method is to use PRIMASK. With BASEPRI configuration, interrupts with priority 
      equal to or lower than a given configurable level are disabled. The interrupt 
      disable priority level is defined at compile time. The BASEPRI register is not 
      available for all architectures.
  @li <b>NVIC mask</b> section: Disable NVIC (external interrupts) on an
      individual manner.

  em_core also has an API for manipulating RAM based interrupt vector tables.


@n @section core_conf Compile time configuration

  The following @htmlonly #defines @endhtmlonly are used to configure em_core:
  @verbatim
  // The interrupt priority level used inside ATOMIC sections.
  #define CORE_ATOMIC_BASE_PRIORITY_LEVEL    3

  // Method used for interrupt disable/enable within ATOMIC sections.
  #define CORE_ATOMIC_METHOD                 CORE_ATOMIC_METHOD_PRIMASK
  @endverbatim

  If the default values does not support your needs, they can be overridden
  by supplying -D compiler flags on the compiler command line or by collecting
  all macro redefinitions in a file named @em emlib_config.h and then supplying
  -DEMLIB_USER_CONFIG on compiler command line.

  @note The default emlib configuration for ATOMIC section interrupt disable
        method is using PRIMASK, i.e. ATOMIC sections are implemented as
        CRITICAL sections.

  @note Due to architectural limitations Cortex-M0+ devices does not support
        ATOMIC type critical sections using the BASEPRI register. On M0+
        devices ATOMIC section helper macros are available but they are
        implemented as CRITICAL sections using PRIMASK register.


@n @section core_macro_api The macro API

  The primary em_core API is the macro API. The macro API will map to correct
  CORE functions according to the selected @ref CORE_ATOMIC_METHOD and similar
  configurations (the full CORE API is of course also available).
  The most useful macros are:

  @ref CORE_DECLARE_IRQ_STATE @n @ref CORE_ENTER_ATOMIC() @n
  @ref CORE_EXIT_ATOMIC()@n
  Used together to implement an ATOMIC section.
  @verbatim
  {
    CORE_DECLARE_IRQ_STATE;           // Storage for saving IRQ state prior
                                      // atomic section entry.

    CORE_ENTER_ATOMIC();              // Enter atomic section

    ...
    ... your code goes here ...
    ...

    CORE_EXIT_ATOMIC();               // Exit atomic section, IRQ state is restored
  }
  @endverbatim

  @n @ref CORE_ATOMIC_SECTION(yourcode)@n
  A concatenation of all three macros above.
  @verbatim
  {
    CORE_ATOMIC_SECTION(
      ...
      ... your code goes here ...
      ...
    )
  }
  @endverbatim

  @n @ref CORE_DECLARE_IRQ_STATE @n @ref CORE_ENTER_CRITICAL() @n
  @ref CORE_EXIT_CRITICAL() @n @ref CORE_CRITICAL_SECTION(yourcode)@n
  These macros implement CRITICAL sections in a similar fashion as described
  above for ATOMIC sections.

  @n @ref CORE_DECLARE_NVIC_STATE @n @ref CORE_ENTER_NVIC() @n
  @ref CORE_EXIT_NVIC() @n @ref CORE_NVIC_SECTION(yourcode)@n
  These macros implement NVIC mask sections in a similar fashion as described
  above for ATOMIC sections. See @ref core_examples for an example.

  Refer to @em Macros or <em>Macro Definition Documentation</em> below for a
  full list of macros.


@n @section core_reimplementation API reimplementation

  Most of the functions in the API are implemented as weak functions. This means
  that it is easy to reimplement them when special needs arise. Shown below is a
  reimplementation of CRITICAL sections suitable if FreeRTOS is used:
  @verbatim
  CORE_irqState_t CORE_EnterCritical(void)
  {
    vPortEnterCritical();
    return 0;
  }

  void CORE_ExitCritical(CORE_irqState_t irqState)
  {
    (void)irqState;
    vPortExitCritical();
  }
  @endverbatim
  Also note that CORE_Enter/ExitCritical() are not implemented as inline
  functions. As a result, reimplementations will be possible even when original
  implementations reside inside a linked library.

  Some RTOS'es must be notified on interrupt handler entry and exit. Macros
  @ref CORE_INTERRUPT_ENTRY() and @ref CORE_INTERRUPT_EXIT() are suitable
  placeholders for inserting such code. Insert these macros in all your
  interrupt handlers and then override the default macro implementations.
  Here is an example suitable if uC/OS is used:
  @verbatim
  // In emlib_config.h:

  #define CORE_INTERRUPT_ENTRY()   OSIntEnter()
  #define CORE_INTERRUPT_EXIT()    OSIntExit()
  @endverbatim


@n @section core_vector_tables Interrupt vector tables

  When using RAM based interrupt vector tables it is the users responsibility
  to allocate the table space correctly. The tables must be aligned as specified
  in the cpu reference manual.

  @ref CORE_InitNvicVectorTable()@n
  Initialize a RAM based vector table by copying table entries from a source
  vector table to a target table. VTOR is set to the address of the target
  vector table.

  @n @ref CORE_GetNvicRamTableHandler() @n @ref CORE_SetNvicRamTableHandler()@n
  Use these functions to get or set the interrupt handler for a specific IRQn.
  They both use the interrupt vector table defined by current
  VTOR register value.

@n @section core_examples Examples

  Implement a NVIC critical section:
  @verbatim
  {
    CORE_DECLARE_NVIC_ZEROMASK(mask); // A zero initialized NVIC disable mask

    // Set mask bits for IRQ's we wish to block in the NVIC critical section
    // In many cases you can create the disable mask once upon application
    // startup and use the mask globally throughout application lifetime.
    CORE_NvicMaskSetIRQ(LEUART0_IRQn, &mask);
    CORE_NvicMaskSetIRQ(VCMP_IRQn,    &mask);

    // Enter NVIC critical section with the disable mask
    CORE_NVIC_SECTION(&mask,
      ...
      ... your code goes here ...
      ...
    )
  }
  @endverbatim

@n @section core_porting Porting from em_int
  
  Existing code using INT_Enable() and INT_Disable() must be ported to the
  em_core API. While em_int used a global counter to store the interrupt state,
  em_core uses a local variable. Any usage of INT_Disable() therefore needs to
  be replaced with a declaration of the interrupt state variable before entering
  the critical section.

  Since the state variable is in the local scope, the critical section exit
  needs to occur within the scope of the variable. If multiple nested critical
  sections are used, each needs to have its own state variable in its own scope.

  In many cases, completely disabling all interrupts using CRITICAL sections
  might be more heavy-handed than needed. When porting, consider whether other
  types of sections, like ATOMIC or NVIC mask, can be used to only disable
  a subset of the interrupts.

  Replacing em_int calls with em_core function calls:
  @verbatim
  void func(void)
  {
    // INT_Disable();
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
      .
      .
      .
    // INT_Enable();
    CORE_EXIT_ATOMIC();
  }
  @endverbatim
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#if !defined(CORE_ATOMIC_BASE_PRIORITY_LEVEL)
/** The interrupt priority level disabled within ATOMIC regions. Interrupts
 *  with priority level equal to or lower than this definition will be disabled
 *  within ATOMIC regions. */
#define CORE_ATOMIC_BASE_PRIORITY_LEVEL   3
#endif

#if !defined(CORE_ATOMIC_METHOD)
/** Specify which method to use when implementing ATOMIC sections. You can
 *  select between BASEPRI or PRIMASK method.
 *  @note On Cortex-M0+ devices only PRIMASK can be used. */
#define CORE_ATOMIC_METHOD    CORE_ATOMIC_METHOD_PRIMASK
#endif

#if !defined(CORE_INTERRUPT_ENTRY)
// Some RTOS's must be notified on interrupt entry (and exit).
// Use this macro at the start of all your interrupt handlers.
// Reimplement the macro in emlib_config.h to suit the needs of your RTOS.
/** Placeholder for optional interrupt handler entry code. This might be needed
 *  when working with an RTOS. */
#define CORE_INTERRUPT_ENTRY()
#endif

#if !defined(CORE_INTERRUPT_EXIT)
/** Placeholder for optional interrupt handler exit code. This might be needed
 *  when working with an RTOS. */
#define CORE_INTERRUPT_EXIT()
#endif

// Compile time sanity check.
#if (CORE_ATOMIC_METHOD != CORE_ATOMIC_METHOD_PRIMASK) \
    && (CORE_ATOMIC_METHOD != CORE_ATOMIC_METHOD_BASEPRI)
#error "em_core: Undefined ATOMIC IRQ handling strategy."
#endif

/*******************************************************************************
 ******************************   FUNCTIONS   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Disable interrupts.
 *
 *   Disable all interrupts by setting PRIMASK.
 *   (Fault exception handlers will still be enabled).
 ******************************************************************************/
SL_WEAK void CORE_CriticalDisableIrq(void)
{
  __disable_irq();
}

/***************************************************************************//**
 * @brief
 *   Enable interrupts.
 *
 *   Enable interrupts by clearing PRIMASK.
 ******************************************************************************/
SL_WEAK void CORE_CriticalEnableIrq(void)
{
  __enable_irq();
}

/***************************************************************************//**
 * @brief
 *   Enter a CRITICAL section.
 *
 *   When a CRITICAL section is entered, all interrupts (except fault handlers)
 *   are disabled.
 *
 * @return
 *   The value of PRIMASK register prior to CRITICAL section entry.
 ******************************************************************************/
SL_WEAK CORE_irqState_t CORE_EnterCritical(void)
{
  CORE_irqState_t irqState = __get_PRIMASK();
  __disable_irq();
  return irqState;
}

/***************************************************************************//**
 * @brief
 *   Exit a CRITICAL section.
 *
 * @param[in] irqState
 *   The interrupt priority blocking level to restore to PRIMASK when exiting
 *   the CRITICAL section. This value is usually the one returned by a prior
 *   call to @ref CORE_EnterCritical().
 ******************************************************************************/
SL_WEAK void CORE_ExitCritical(CORE_irqState_t irqState)
{
  if (irqState == 0) {
    __enable_irq();
  }
}

/***************************************************************************//**
 * @brief
 *   Brief interrupt enable/disable sequence to allow handling of
 *   pending interrupts.
 *
 * @note
 *   Usully used within a CRITICAL section.
 ******************************************************************************/
SL_WEAK void CORE_YieldCritical(void)
{
  if (__get_PRIMASK() & 1) {
    __enable_irq();
    __disable_irq();
  }
}

/***************************************************************************//**
 * @brief
 *   Disable interrupts.
 *
 *   Disable interrupts with priority lower or equal to
 *   @ref CORE_ATOMIC_BASE_PRIORITY_LEVEL. Sets core BASEPRI register
 *   to CORE_ATOMIC_BASE_PRIORITY_LEVEL.
 *
 * @note
 *   If @ref CORE_ATOMIC_METHOD is @ref CORE_ATOMIC_METHOD_PRIMASK, this
 *   function is identical to @ref CORE_CriticalDisableIrq().
 ******************************************************************************/
SL_WEAK void CORE_AtomicDisableIrq(void)
{
#if (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
  __set_BASEPRI(CORE_ATOMIC_BASE_PRIORITY_LEVEL << (8 - __NVIC_PRIO_BITS));
#else
  __disable_irq();
#endif // (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
}

/***************************************************************************//**
 * @brief
 *   Enable interrupts.
 *
 *   Enable interrupts by setting core BASEPRI register to 0.
 *
 * @note
 *   If @ref CORE_ATOMIC_METHOD is @ref CORE_ATOMIC_METHOD_BASEPRI and PRIMASK
 *   is set (cpu is inside a CRITICAL section), interrupts will still be
 *   disabled after calling this function.
 *
 * @note
 *   If @ref CORE_ATOMIC_METHOD is @ref CORE_ATOMIC_METHOD_PRIMASK, this
 *   function is identical to @ref CORE_CriticalEnableIrq().
 ******************************************************************************/
SL_WEAK void CORE_AtomicEnableIrq(void)
{
#if (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
  __set_BASEPRI(0);
#else
  __enable_irq();
#endif // (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
}

/***************************************************************************//**
 * @brief
 *   Enter an ATOMIC section.
 *
 *   When an ATOMIC section is entered, interrupts with priority lower or equal
 *   to @ref CORE_ATOMIC_BASE_PRIORITY_LEVEL are disabled.
 *
 * @note
 *   If @ref CORE_ATOMIC_METHOD is @ref CORE_ATOMIC_METHOD_PRIMASK, this
 *   function is identical to @ref CORE_EnterCritical().
 *
 * @return
 *   The value of BASEPRI register prior to ATOMIC section entry.
 ******************************************************************************/
SL_WEAK CORE_irqState_t CORE_EnterAtomic(void)
{
#if (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
  CORE_irqState_t irqState = __get_BASEPRI();
  __set_BASEPRI(CORE_ATOMIC_BASE_PRIORITY_LEVEL << (8 - __NVIC_PRIO_BITS));
  return irqState;
#else
  CORE_irqState_t irqState = __get_PRIMASK();
  __disable_irq();
  return irqState;
#endif // (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
}

/***************************************************************************//**
 * @brief
 *   Exit an ATOMIC section.
 *
 * @param[in] irqState
 *   The interrupt priority blocking level to restore to BASEPRI when exiting
 *   the ATOMIC section. This value is usually the one returned by a prior
 *   call to @ref CORE_EnterAtomic().
 *
 * @note
 *   If @ref CORE_ATOMIC_METHOD is set to @ref CORE_ATOMIC_METHOD_PRIMASK, this
 *   function is identical to @ref CORE_ExitCritical().
 ******************************************************************************/
SL_WEAK void CORE_ExitAtomic(CORE_irqState_t irqState)
{
#if (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
  __set_BASEPRI(irqState);
#else
  if (irqState == 0) {
    __enable_irq();
  }
#endif // (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
}

/***************************************************************************//**
 * @brief
 *   Brief interrupt enable/disable sequence to allow handling of
 *   pending interrupts.
 *
 * @note
 *   Usully used within an ATOMIC section.
 *
 * @note
 *   If @ref CORE_ATOMIC_METHOD is @ref CORE_ATOMIC_METHOD_PRIMASK, this
 *   function is identical to @ref CORE_YieldCritical().
 ******************************************************************************/
SL_WEAK void CORE_YieldAtomic(void)
{
#if (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
  CORE_irqState_t basepri = __get_BASEPRI();
  if (basepri >= (CORE_ATOMIC_BASE_PRIORITY_LEVEL << (8 - __NVIC_PRIO_BITS))) {
    __set_BASEPRI(0);
    __set_BASEPRI(basepri);
  }
#else
  if (__get_PRIMASK() & 1) {
    __enable_irq();
    __disable_irq();
  }
#endif // (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
}

/***************************************************************************//**
 * @brief
 *   Enter a NVIC mask section.
 *
 *   When a NVIC mask section is entered, specified NVIC interrupts
 *   are disabled.
 *
 * @param[out] nvicState
 *   Return NVIC interrupts enable mask prior to section entry.
 *
 * @param[in] disable
 *   Mask specifying which NVIC interrupts to disable within the section.
 ******************************************************************************/
void CORE_EnterNvicMask(CORE_nvicMask_t *nvicState,
                        const CORE_nvicMask_t *disable)
{
  CORE_CRITICAL_SECTION(
    *nvicState = *(CORE_nvicMask_t*)&NVIC->ICER[0];
    *(CORE_nvicMask_t*)&NVIC->ICER[0] = *disable;
  )
}

/***************************************************************************//**
 * @brief
 *   Disable NVIC interrupts.
 *
 * @param[in] disable
 *   Mask specifying which NVIC interrupts to disable.
 ******************************************************************************/
void CORE_NvicDisableMask(const CORE_nvicMask_t *disable)
{
  CORE_CRITICAL_SECTION(
    *(CORE_nvicMask_t*)&NVIC->ICER[0] = *disable;
  )
}

/***************************************************************************//**
 * @brief
 *   Set current NVIC interrupt enable mask.
 *
 * @param[out] enable
 *   Mask specifying which NVIC interrupts are currently enabled.
 ******************************************************************************/
void CORE_NvicEnableMask(const CORE_nvicMask_t *enable)
{
  CORE_CRITICAL_SECTION(
    *(CORE_nvicMask_t*)&NVIC->ISER[0] = *enable;
  )
}

/***************************************************************************//**
 * @brief
 *   Brief NVIC interrupt enable/disable sequence to allow handling of
 *   pending interrupts.
 *
 * @param[in] enable
 *   Mask specifying which NVIC interrupts to briefly enable.
 *
 * @note
 *   Usually used within a NVIC mask section.
 ******************************************************************************/
void CORE_YieldNvicMask(const CORE_nvicMask_t *enable)
{
  CORE_nvicMask_t nvicMask;

  // Get current NVIC enable mask.
  CORE_CRITICAL_SECTION(
    nvicMask = *(CORE_nvicMask_t*)&NVIC->ISER[0];
  )

  // Make a mask with bits set for those interrupts that are currently
  // disabled but are set in the enable mask.
#if (CORE_NVIC_REG_WORDS == 1)
  nvicMask.a[0] &= enable->a[0];
  nvicMask.a[0] = ~nvicMask.a[0] & enable->a[0];

  if (nvicMask.a[0] != 0) {

#elif (CORE_NVIC_REG_WORDS == 2)
  nvicMask.a[0] &= enable->a[0];
  nvicMask.a[1] &= enable->a[1];
  nvicMask.a[0] = ~nvicMask.a[0] & enable->a[0];
  nvicMask.a[1] = ~nvicMask.a[1] & enable->a[1];

  if ((nvicMask.a[0] != 0) || (nvicMask.a[1] != 0)) {

#elif (CORE_NVIC_REG_WORDS == 3)
  nvicMask.a[0] &= enable->a[0];
  nvicMask.a[1] &= enable->a[1];
  nvicMask.a[2] &= enable->a[2];
  nvicMask.a[0] = ~nvicMask.a[0] & enable->a[0];
  nvicMask.a[1] = ~nvicMask.a[1] & enable->a[1];
  nvicMask.a[2] = ~nvicMask.a[2] & enable->a[2];

  if ((nvicMask.a[0] != 0) || (nvicMask.a[1] != 0) || (nvicMask.a[2] != 0)) {
#endif

    // Enable previously disabled interrupts.
    *(CORE_nvicMask_t*)&NVIC->ISER[0] = nvicMask;

    // Disable those interrupts again.
    *(CORE_nvicMask_t*)&NVIC->ICER[0] = nvicMask;
  }
}

/***************************************************************************//**
 * @brief
 *   Utility function to set an IRQn bit in a NVIC enable/disable mask.
 *
 * @param[in] irqN
 *   The @ref IRQn_Type enumerator for the interrupt.
 *
 * @param[in,out] mask
 *   The mask to set interrupt bit in.
 ******************************************************************************/
void CORE_NvicMaskSetIRQ(IRQn_Type irqN, CORE_nvicMask_t *mask)
{
  EFM_ASSERT((irqN >= 0) && (irqN < EXT_IRQ_COUNT));
  mask->a[irqN >> 5] |= 1 << (irqN & 0x1F);
}

/***************************************************************************//**
 * @brief
 *   Utility function to clear an IRQn bit in a NVIC enable/disable mask.
 *
 * @param[in] irqN
 *   The @ref IRQn_Type enumerator for the interrupt.
 *
 * @param[in,out] mask
 *   The mask to clear interrupt bit in.
 ******************************************************************************/
void CORE_NvicMaskClearIRQ(IRQn_Type irqN, CORE_nvicMask_t *mask)
{
  EFM_ASSERT((irqN >= 0) && (irqN < EXT_IRQ_COUNT));
  mask->a[irqN >> 5] &= ~(1 << (irqN & 0x1F));
}

/***************************************************************************//**
 * @brief
 *   Check if current cpu operation mode is handler mode.
 *
 * @return
 *   True if cpu in handler mode (currently executing an interrupt handler).
 *   @n False if cpu in thread mode.
 ******************************************************************************/
SL_WEAK bool CORE_InIrqContext(void)
{
  return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

/***************************************************************************//**
 * @brief
 *   Check if a specific interrupt is disabled or blocked.
 *
 * @param[in] irqN
 *   The @ref IRQn_Type enumerator for the interrupt to check.
 *
 * @return
 *   True if interrupt disabled or blocked.
 ******************************************************************************/
SL_WEAK bool CORE_IrqIsBlocked(IRQn_Type irqN)
{
  uint32_t irqPri, activeIrq;

#if (__CORTEX_M >= 3)
  uint32_t basepri;

  EFM_ASSERT((irqN >= MemoryManagement_IRQn) && (irqN < EXT_IRQ_COUNT));
#else
  EFM_ASSERT((irqN >= SVCall_IRQn) && (irqN < EXT_IRQ_COUNT));
#endif

  if (__get_PRIMASK() & 1) {
    return true;                            // All IRQ's are disabled
  }

  if (CORE_NvicIRQDisabled(irqN)) {
    return true;                            // The IRQ in question is disabled
  }

  irqPri  = NVIC_GetPriority(irqN);
#if (__CORTEX_M >= 3)
  basepri = __get_BASEPRI();
  if ((basepri != 0)
      && (irqPri >= (basepri >> (8 - __NVIC_PRIO_BITS)))) {
    return true;                            // The IRQ in question has too low
  }                                         // priority vs. BASEPRI
#endif

  // Check if already in an interrupt handler, if so an interrupt with
  // higher priority (lower priority value) can preempt.
  activeIrq = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
  if ((activeIrq != 0)
      && (irqPri >= NVIC_GetPriority((IRQn_Type)(activeIrq - 16)))) {
    return true;                            // The IRQ in question has too low
  }                                         // priority vs. current active IRQ

  return false;
}

/***************************************************************************//**
 * @brief
 *   Check if interrupts are disabled.
 *
 * @return
 *   True if interrupts are disabled.
 ******************************************************************************/
SL_WEAK bool CORE_IrqIsDisabled(void)
{
#if (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_PRIMASK)
  return (__get_PRIMASK() & 1) == 1;

#elif (CORE_ATOMIC_METHOD == CORE_ATOMIC_METHOD_BASEPRI)
  return ((__get_PRIMASK() & 1) == 1)
         || (__get_BASEPRI() >= (CORE_ATOMIC_BASE_PRIORITY_LEVEL
                                 << (8 - __NVIC_PRIO_BITS)));
#endif
}

/***************************************************************************//**
 * @brief
 *   Get current NVIC enable mask state.
 *
 * @param[out] mask
 *   Current NVIC enable mask.
 ******************************************************************************/
void CORE_GetNvicEnabledMask(CORE_nvicMask_t *mask)
{
  CORE_CRITICAL_SECTION(
    *mask = *(CORE_nvicMask_t*)&NVIC->ISER[0];
  )
}

/***************************************************************************//**
 * @brief
 *   Get NVIC disable state for a given mask.
 *
 * @param[in] mask
 *   NVIC mask to check.
 *
 * @return
 *   True if all NVIC interrupt mask bits are clear.
 ******************************************************************************/
bool CORE_GetNvicMaskDisableState(const CORE_nvicMask_t *mask)
{
  CORE_nvicMask_t nvicMask;

  CORE_CRITICAL_SECTION(
    nvicMask = *(CORE_nvicMask_t*)&NVIC->ISER[0];
  )


#if (CORE_NVIC_REG_WORDS == 1)
  return (mask->a[0] & nvicMask.a[0]) == 0;

#elif (CORE_NVIC_REG_WORDS == 2)
  return ((mask->a[0] & nvicMask.a[0]) == 0)
         && ((mask->a[1] & nvicMask.a[1]) == 0);

#elif (CORE_NVIC_REG_WORDS == 3)
  return ((mask->a[0] & nvicMask.a[0]) == 0)
         && ((mask->a[1] & nvicMask.a[1]) == 0)
         && ((mask->a[2] & nvicMask.a[2]) == 0);
#endif
}

/***************************************************************************//**
 * @brief
 *   Check if a NVIC interrupt is disabled.
 *
 * @param[in] irqN
 *   The @ref IRQn_Type enumerator for the interrupt to check.
 *
 * @return
 *   True if interrupt disabled.
 ******************************************************************************/
bool CORE_NvicIRQDisabled(IRQn_Type irqN)
{
  CORE_nvicMask_t *mask;

  EFM_ASSERT((irqN >= 0) && (irqN < EXT_IRQ_COUNT));
  mask = (CORE_nvicMask_t*)&NVIC->ISER[0];
  return (mask->a[irqN >> 5] & (1 << (irqN & 0x1F))) == 0;
}

/***************************************************************************//**
 * @brief
 *   Utility function to get the handler for a specific interrupt.
 *
 * @param[in] irqN
 *   The @ref IRQn_Type enumerator for the interrupt.
 *
 * @return
 *   The handler address.
 *
 * @note
 *   Uses the interrupt vector table defined by current VTOR register value.
 ******************************************************************************/
void *CORE_GetNvicRamTableHandler(IRQn_Type irqN)
{
  EFM_ASSERT((irqN >= -16) && (irqN < EXT_IRQ_COUNT));
  return (void*)(((uint32_t*)SCB->VTOR)[irqN+16]);
}

/***************************************************************************//**
 * @brief
 *   Utility function to set the handler for a specific interrupt.
 *
 * @param[in] irqN
 *   The @ref IRQn_Type enumerator for the interrupt.
 *
 * @param[in] handler
 *   The handler address.
 *
 * @note
 *   Uses the interrupt vector table defined by current VTOR register value.
 ******************************************************************************/
void CORE_SetNvicRamTableHandler(IRQn_Type irqN, void *handler)
{
  EFM_ASSERT((irqN >= -16) && (irqN < EXT_IRQ_COUNT));
  ((uint32_t*)SCB->VTOR)[irqN+16] = (uint32_t)handler;
}

/***************************************************************************//**
 * @brief
 *   Initialize an interrupt vector table by copying table entries from a
 *   source to a target table.
 *
 * @note This function will set a new VTOR register value.
 *
 * @param[in] sourceTable
 *   Address of source vector table.
 *
 * @param[in] sourceSize
 *   Number of entries is source vector table.
 *
 * @param[in] targetTable
 *   Address of target (new) vector table.
 *
 * @param[in] targetSize
 *   Number of entries is target vector table.
 *
 * @param[in] defaultHandler
 *   Address of interrupt handler used for target entries for which where there
 *   is no corresponding source entry (i.e. target table is larger than source
 *   table).
 *
 * @param[in] overwriteActive
 *   When true, a target table entry is always overwritten with the
 *   corresponding source entry. If false, a target table entry is only
 *   overwritten if it is zero. This makes it possible for an application
 *   to partly initialize a target table before passing it to this function.
 *
 ******************************************************************************/
void CORE_InitNvicVectorTable(uint32_t *sourceTable,
                              uint32_t sourceSize,
                              uint32_t *targetTable,
                              uint32_t targetSize,
                              void *defaultHandler,
                              bool overwriteActive)
{
  uint32_t i;

  // ASSERT on non SRAM based target table.
  EFM_ASSERT(((uint32_t)targetTable >= RAM_MEM_BASE)
             && ((uint32_t)targetTable < (RAM_MEM_BASE + RAM_MEM_SIZE)));

  // ASSERT if misaligned with respect to VTOR register implementation.
#if defined(SCB_VTOR_TBLBASE_Msk)
  EFM_ASSERT(((uint32_t)targetTable & ~(SCB_VTOR_TBLOFF_Msk
                                        | SCB_VTOR_TBLBASE_Msk)) == 0);
#else
  EFM_ASSERT(((uint32_t)targetTable & ~SCB_VTOR_TBLOFF_Msk) == 0);
#endif

  // ASSERT if misaligned with respect to vector table size.
  // Vector table address must be aligned at its size rounded up to nearest 2^n.
  EFM_ASSERT(((uint32_t)targetTable
              & ((1 << (32 - __CLZ((targetSize * 4) - 1))) - 1)) == 0);

  for (i=0; i<targetSize; i++) {
    if (overwriteActive) {                      // Overwrite target entries ?
      if (i<sourceSize) {                       //   targetSize <= sourceSize
        targetTable[i] = sourceTable[i];
      } else {                                  //   targetSize > sourceSize
        targetTable[i] = (uint32_t)defaultHandler;
      }
    } else {                            // Overwrite target entries which are 0
      if (i<sourceSize) {                       // targetSize <= sourceSize
        if (targetTable[i] == 0) {
          targetTable[i] = sourceTable[i];
        }
      } else {                                  // targetSize > sourceSize
        if (targetTable[i] == 0) {
          targetTable[i] = (uint32_t)defaultHandler;
        }
      }
    }
  }
  SCB->VTOR = (uint32_t)targetTable;
}

/** @} (end addtogroup CORE) */
/** @} (end addtogroup emlib) */
