/***************************************************************************//**
 * @file em_core.h
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
#ifndef EM_CORE_H
#define EM_CORE_H

#include "em_device.h"
#include "em_common.h"

#include <stdbool.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CORE
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** Use PRIMASK register to disable interrupts in ATOMIC sections. */
#define CORE_ATOMIC_METHOD_PRIMASK  0

/** Use BASEPRI register to disable interrupts in ATOMIC sections. */
#define CORE_ATOMIC_METHOD_BASEPRI  1

/** Number of words in a NVIC mask set. */
#define CORE_NVIC_REG_WORDS   ((EXT_IRQ_COUNT + 31) / 32)

/** Number of entries in a default interrupt vector table. */
#define CORE_DEFAULT_VECTOR_TABLE_ENTRIES   (EXT_IRQ_COUNT + 16)

// Compile time sanity check.
#if (CORE_NVIC_REG_WORDS > 3)
#error "em_core: Unexpected NVIC external interrupt count."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 ************************   MACRO API   ***************************************
 ******************************************************************************/

//
//  CRITICAL section macro API.
//

/** Allocate storage for PRIMASK or BASEPRI value for use by
 *  CORE_ENTER/EXIT_ATOMIC() and CORE_ENTER/EXIT_CRITICAL() macros. */
#define CORE_DECLARE_IRQ_STATE        CORE_irqState_t irqState

/** CRITICAL style interrupt disable. */
#define CORE_CRITICAL_IRQ_DISABLE() CORE_CriticalDisableIrq()

/** CRITICAL style interrupt enable. */
#define CORE_CRITICAL_IRQ_ENABLE()  CORE_CriticalEnableIrq()

/** Convenience macro for implementing a CRITICAL section. */
#define CORE_CRITICAL_SECTION(yourcode) \
{                                       \
  CORE_DECLARE_IRQ_STATE;               \
  CORE_ENTER_CRITICAL();                \
  {                                     \
    yourcode                            \
  }                                     \
  CORE_EXIT_CRITICAL();                 \
}

/** Enter CRITICAL section. Assumes that a @ref CORE_DECLARE_IRQ_STATE exist in
 *  scope. */
#define CORE_ENTER_CRITICAL()   irqState = CORE_EnterCritical()

/** Exit CRITICAL section. Assumes that a @ref CORE_DECLARE_IRQ_STATE exist in
 *  scope. */
#define CORE_EXIT_CRITICAL()    CORE_ExitCritical(irqState)

/** CRITICAL style yield. */
#define CORE_YIELD_CRITICAL()   CORE_YieldCritical(void)

//
//  ATOMIC section macro API.
//

/** ATOMIC style interrupt disable. */
#define CORE_ATOMIC_IRQ_DISABLE()   CORE_AtomicDisableIrq()

/** ATOMIC style interrupt enable. */
#define CORE_ATOMIC_IRQ_ENABLE()    CORE_AtomicEnableIrq()

/** Convenience macro for implementing an ATOMIC section. */
#define CORE_ATOMIC_SECTION(yourcode) \
{                                     \
  CORE_DECLARE_IRQ_STATE;             \
  CORE_ENTER_ATOMIC();                \
  {                                   \
    yourcode                          \
  }                                   \
  CORE_EXIT_ATOMIC();                 \
}

/** Enter ATOMIC section. Assumes that a @ref CORE_DECLARE_IRQ_STATE exist in
 *  scope. */
#define CORE_ENTER_ATOMIC()   irqState = CORE_EnterAtomic()

/** Exit ATOMIC section. Assumes that a @ref CORE_DECLARE_IRQ_STATE exist in
 *  scope. */
#define CORE_EXIT_ATOMIC()    CORE_ExitAtomic(irqState)

/** ATOMIC style yield. */
#define CORE_YIELD_ATOMIC()   CORE_YieldAtomic(void)

//
//  NVIC mask section macro API.
//

/** Allocate storage for NVIC interrupt masks for use by
 *  CORE_ENTER/EXIT_NVIC() macros. */
#define CORE_DECLARE_NVIC_STATE       CORE_nvicMask_t nvicState

/** Allocate storage for NVIC interrupt masks.
 *  @param[in] x
 *    The storage variable name to use.*/
#define CORE_DECLARE_NVIC_MASK(x)     CORE_nvicMask_t x

/** Allocate storage for and zero initialize NVIC interrupt mask.
 *  @param[in] x
 *    The storage variable name to use.*/
#define CORE_DECLARE_NVIC_ZEROMASK(x) CORE_nvicMask_t x = {{0}}

/** NVIC mask style interrupt disable.
 *  @param[in] mask
 *    Mask specifying which NVIC interrupts to disable. */
#define CORE_NVIC_DISABLE(mask)   CORE_NvicDisableMask(mask)

/** NVIC mask style interrupt enable.
 *  @param[in] mask
 *    Mask specifying which NVIC interrupts to enable. */
#define CORE_NVIC_ENABLE(mask)    CORE_NvicEnableMask(mask)

/** Convenience macro for implementing a NVIC mask section.
 *  @param[in] mask
 *    Mask specifying which NVIC interrupts to disable within the section.
 *  @param[in] yourcode
 *    The code for the section. */
#define CORE_NVIC_SECTION(mask, yourcode)   \
{                                           \
  CORE_DECLARE_NVIC_STATE;                  \
  CORE_ENTER_NVIC(mask);                    \
  {                                         \
    yourcode                                \
  }                                         \
  CORE_EXIT_NVIC();                         \
}

/** Enter NVIC mask section. Assumes that a @ref CORE_DECLARE_NVIC_STATE exist
 *  in scope.
 *  @param[in] disable
 *    Mask specifying which NVIC interrupts to disable within the section. */
#define CORE_ENTER_NVIC(disable)  CORE_EnterNvicMask(&nvicState,disable)

/** Exit NVIC mask section. Assumes that a @ref CORE_DECLARE_NVIC_STATE exist
 *  in scope. */
#define CORE_EXIT_NVIC()          CORE_NvicEnableMask(&nvicState)

/** NVIC maks style yield.
 * @param[in] enable
 *   Mask specifying which NVIC interrupts to briefly enable. */
#define CORE_YIELD_NVIC(enable)   CORE_YieldNvicMask(enable)

//
//  Miscellaneous macros.
//

/** Check if IRQ is disabled. */
#define CORE_IRQ_DISABLED()       CORE_IrqIsDisabled()

/** Check if inside an IRQ handler. */
#define CORE_IN_IRQ_CONTEXT()     CORE_InIrqContext()

/*******************************************************************************
 *************************   TYPEDEFS   ****************************************
 ******************************************************************************/

/** Storage for PRIMASK or BASEPRI value. */
typedef uint32_t CORE_irqState_t;

/** Storage for NVIC interrupt masks. */
typedef struct {
  uint32_t a[CORE_NVIC_REG_WORDS];    /*!< Array of NVIC mask words. */
} CORE_nvicMask_t;

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void CORE_CriticalDisableIrq(void);
void CORE_CriticalEnableIrq(void);
void CORE_ExitCritical(CORE_irqState_t irqState);
void CORE_YieldCritical(void);
CORE_irqState_t CORE_EnterCritical(void);

void  CORE_AtomicDisableIrq(void);
void  CORE_AtomicEnableIrq(void);
void  CORE_ExitAtomic(CORE_irqState_t irqState);
void  CORE_YieldAtomic(void);
CORE_irqState_t CORE_EnterAtomic(void);

bool  CORE_InIrqContext(void);
bool  CORE_IrqIsBlocked(IRQn_Type irqN);
bool  CORE_IrqIsDisabled(void);

void  CORE_GetNvicEnabledMask(CORE_nvicMask_t *mask);
bool  CORE_GetNvicMaskDisableState(const CORE_nvicMask_t *mask);

void  CORE_EnterNvicMask(CORE_nvicMask_t *nvicState,
                         const CORE_nvicMask_t *disable);
void  CORE_NvicDisableMask(const CORE_nvicMask_t *disable);
void  CORE_NvicEnableMask(const CORE_nvicMask_t *enable);
void  CORE_YieldNvicMask(const CORE_nvicMask_t *enable);
void  CORE_NvicMaskSetIRQ(IRQn_Type irqN, CORE_nvicMask_t *mask);
void  CORE_NvicMaskClearIRQ(IRQn_Type irqN, CORE_nvicMask_t *mask);
bool  CORE_NvicIRQDisabled(IRQn_Type irqN);

void *CORE_GetNvicRamTableHandler(IRQn_Type irqN);
void  CORE_SetNvicRamTableHandler(IRQn_Type irqN, void *handler);
void  CORE_InitNvicVectorTable(uint32_t *sourceTable,
                               uint32_t sourceSize,
                               uint32_t *targetTable,
                               uint32_t targetSize,
                               void *defaultHandler,
                               bool overwriteActive);

#ifdef __cplusplus
}
#endif

/** @} (end addtogroup CORE) */
/** @} (end addtogroup emlib) */

#endif /* EM_CORE_H */
