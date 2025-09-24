/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The TCPWM function and macros in the PDL use a complex set of arrays and marcos to
 * reference a specific TCPWM instance.  With the information in the .dtsi file, we
 * have the absolute address for each TCPWM instance.  We will make use of that
 * when we provide the ADDRESS to the macros below.  This means that the cntNum parameter
 * is always 0.
 */

#if !defined(ZEPHYR_INCLUDE_DRIVERS_TIMER_IFX_TCPWM_TIMER_H)
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_IFX_TCPWM_TIMER_H

#include <stdint.h>
#include "cy_tcpwm.h"

/** @cond INTERNAL_HIDDEN */
#define IFX_TCPWM_Block_EnableCompare0Swap(ADDRESS, ENABLE)                                        \
	Cy_TCPWM_Block_EnableCompare0Swap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_Block_EnableCompare1Swap(ADDRESS, ENABLE)                                        \
	Cy_TCPWM_Block_EnableCompare1Swap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_Block_GetCC0BufVal(ADDRESS) Cy_TCPWM_Block_GetCC0BufVal((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Block_GetCC0Val(ADDRESS)    Cy_TCPWM_Block_GetCC0Val((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Block_GetCC1BufVal(ADDRESS) Cy_TCPWM_Block_GetCC1BufVal((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Block_GetCC1Val(ADDRESS)    Cy_TCPWM_Block_GetCC1Val((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Block_GetCounter(ADDRESS)   Cy_TCPWM_Block_GetCounter((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Block_GetPeriod(ADDRESS)    Cy_TCPWM_Block_GetPeriod((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Block_SetCC0BufVal(ADDRESS, COMPARE1)                                            \
	Cy_TCPWM_Block_SetCC0BufVal((TCPWM_Type *)ADDRESS, 0, COMPARE1)
#define IFX_TCPWM_Block_SetCC0Val(ADDRESS, COMPARE0)                                               \
	Cy_TCPWM_Block_SetCC0Val((TCPWM_Type *)ADDRESS, 0, COMPARE0)
#define IFX_TCPWM_Block_SetCC1BufVal(ADDRESS, COMPAREBUF1)                                         \
	Cy_TCPWM_Block_SetCC1BufVal((TCPWM_Type *)ADDRESS, 0, COMPAREBUF1)
#define IFX_TCPWM_Block_SetCC1Val(ADDRESS, COMPARE1)                                               \
	Cy_TCPWM_Block_SetCC1Val((TCPWM_Type *)ADDRESS, 0, COMPARE1)
#define IFX_TCPWM_Block_SetCounter(ADDRESS, COUNT)                                                 \
	Cy_TCPWM_Block_SetCounter((TCPWM_Type *)ADDRESS, 0, COUNT)
#define IFX_TCPWM_Block_SetPeriod(ADDRESS, PERIOD)                                                 \
	Cy_TCPWM_Block_SetPeriod((TCPWM_Type *)ADDRESS, 0, PERIOD)
#define IFX_TCPWM_ClearInterrupt(ADDRESS, SOURCE)                                                  \
	Cy_TCPWM_ClearInterrupt((TCPWM_Type *)ADDRESS, 0, SOURCE)
#define IFX_TCPWM_Disable_Single(ADDRESS)     Cy_TCPWM_Disable_Single((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Enable_Single(ADDRESS)      Cy_TCPWM_Enable_Single((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_GetInterruptMask(ADDRESS)   Cy_TCPWM_GetInterruptMask((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_GetInterruptStatus(ADDRESS) Cy_TCPWM_GetInterruptStatus((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_GetInterruptStatusMasked(ADDRESS)                                                \
	Cy_TCPWM_GetInterruptStatusMasked((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_GetTrigPinLevel(ADDRESS, TRIGGERSELECT)                                          \
	Cy_TCPWM_GetTrigPinLevel((TCPWM_Type *)ADDRESS, 0, TRIGGERSELECT)
#define IFX_TCPWM_SetDebugFreeze(ADDRESS, ENABLE)                                                  \
	Cy_TCPWM_SetDebugFreeze((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_SetInterrupt(ADDRESS, SOURCE)                                                    \
	Cy_TCPWM_SetInterrupt((TCPWM_Type *)ADDRESS, 0, SOURCE)
#define IFX_TCPWM_SetInterruptMask(ADDRESS, MASK)                                                  \
	Cy_TCPWM_SetInterruptMask((TCPWM_Type *)ADDRESS, 0, MASK)
#define IFX_TCPWM_TriggerCapture0(ADDRESS) Cy_TCPWM_TriggerCapture0((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_TriggerCapture1(ADDRESS) Cy_TCPWM_TriggerCapture1((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_TriggerStart_Single(ADDRESS)                                                     \
	Cy_TCPWM_TriggerStart_Single((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_TriggerStopOrKill_Single(ADDRESS)                                                \
	Cy_TCPWM_TriggerStopOrKill_Single((TCPWM_Type *)ADDRESS, 0)

#define IFX_TCPWM_Counter_DeInit(ADDRESS, CONFIG)                                                  \
	Cy_TCPWM_Counter_DeInit((TCPWM_Type *)ADDRESS, 0, CONFIG)
#define IFX_TCPWM_Counter_Disable(ADDRESS) Cy_TCPWM_Counter_Disable((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_Enable(ADDRESS)  Cy_TCPWM_Counter_Enable((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_EnableCompare0Swap(ADDRESS, ENABLE)                                      \
	Cy_TCPWM_Counter_EnableCompare0Swap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_Counter_EnableCompare1Swap(ADDRESS, ENABLE)                                      \
	Cy_TCPWM_Counter_EnableCompare1Swap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_Counter_EnableSwap(ADDRESS, ENABLE)                                              \
	Cy_TCPWM_Counter_EnableSwap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_Counter_GetCapture0BufVal(ADDRESS)                                               \
	Cy_TCPWM_Counter_GetCapture0BufVal((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCapture0Val(ADDRESS)                                                  \
	Cy_TCPWM_Counter_GetCapture0Val((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCapture1BufVal(ADDRESS)                                               \
	Cy_TCPWM_Counter_GetCapture1BufVal((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCapture1Val(ADDRESS)                                                  \
	Cy_TCPWM_Counter_GetCapture1Val((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCompare0BufVal(ADDRESS)                                               \
	Cy_TCPWM_Counter_GetCompare0BufVal((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCompare0Val(ADDRESS)                                                  \
	Cy_TCPWM_Counter_GetCompare0Val((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCompare1BufVal(ADDRESS)                                               \
	Cy_TCPWM_Counter_GetCompare1BufVal((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCompare1Val(ADDRESS)                                                  \
	Cy_TCPWM_Counter_GetCompare1Val((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetCounter(ADDRESS) Cy_TCPWM_Counter_GetCounter((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetPeriod(ADDRESS)  Cy_TCPWM_Counter_GetPeriod((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_GetStatus(ADDRESS)  Cy_TCPWM_Counter_GetStatus((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_Counter_Init(ADDRESS, CONFIG)                                                    \
	Cy_TCPWM_Counter_Init((TCPWM_Type *)ADDRESS, 0, CONFIG)
#define IFX_TCPWM_Counter_SetCompare0BufVal(ADDRESS, COMPARE1)                                     \
	Cy_TCPWM_Counter_SetCompare0BufVal((TCPWM_Type *)ADDRESS, 0, COMPARE1)
#define IFX_TCPWM_Counter_SetCompare0Val(ADDRESS, COMPARE0)                                        \
	Cy_TCPWM_Counter_SetCompare0Val((TCPWM_Type *)ADDRESS, 0, COMPARE0)
#define IFX_TCPWM_Counter_SetCompare1BufVal(ADDRESS, COMPAREBUF1)                                  \
	Cy_TCPWM_Counter_SetCompare1BufVal((TCPWM_Type *)ADDRESS, 0, COMPAREBUF1)
#define IFX_TCPWM_Counter_SetCompare1Val(ADDRESS, COMPARE1)                                        \
	Cy_TCPWM_Counter_SetCompare1Val((TCPWM_Type *)ADDRESS, 0, COMPARE1)
#define IFX_TCPWM_Counter_SetCounter(ADDRESS, COUNT)                                               \
	Cy_TCPWM_Counter_SetCounter((TCPWM_Type *)ADDRESS, 0, COUNT)
#define IFX_TCPWM_Counter_SetDirection_Change_Mode(ADDRESS, DIRECTION_MODE)                        \
	Cy_TCPWM_Counter_SetDirection_Change_Mode((TCPWM_Type *)ADDRESS, 0, DIRECTION_MODE)
#define IFX_TCPWM_Counter_SetPeriod(ADDRESS, PERIOD)                                               \
	Cy_TCPWM_Counter_SetPeriod((TCPWM_Type *)ADDRESS, 0, PERIOD)

#define IFX_TCPWM_PWM_Disable(ADDRESS) Cy_TCPWM_PWM_Disable((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_Enable(ADDRESS)  Cy_TCPWM_PWM_Enable((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_EnableCompare0Swap(ADDRESS, ENABLE)                                          \
	Cy_TCPWM_PWM_EnableCompare0Swap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_PWM_EnableLineSelectSwap(ADDRESS, ENABLE)                                        \
	Cy_TCPWM_PWM_EnableLineSelectSwap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_PWM_EnablePeriodSwap(ADDRESS, ENABLE)                                            \
	Cy_TCPWM_PWM_EnablePeriodSwap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_PWM_EnableSwap(ADDRESS, ENABLE)                                                  \
	Cy_TCPWM_PWM_EnableSwap((TCPWM_Type *)ADDRESS, 0, ENABLE)
#define IFX_TCPWM_PWM_GetCompare0BufVal(ADDRESS)                                                   \
	Cy_TCPWM_PWM_GetCompare0BufVal((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_GetCompare0Val(ADDRESS) Cy_TCPWM_PWM_GetCompare0Val((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_GetCounter(ADDRESS)     Cy_TCPWM_PWM_GetCounter((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_GetDtCounter(ADDRESS)   Cy_TCPWM_PWM_GetDtCounter((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_GetPeriod0(ADDRESS)     Cy_TCPWM_PWM_GetPeriod0((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_GetPeriod1(ADDRESS)     Cy_TCPWM_PWM_GetPeriod1((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_GetStatus(ADDRESS)      Cy_TCPWM_PWM_GetStatus((TCPWM_Type *)ADDRESS, 0)
#define IFX_TCPWM_PWM_Init(ADDRESS, CONFIG)   Cy_TCPWM_PWM_Init((TCPWM_Type *)ADDRESS, 0, CONFIG)
#define IFX_TCPWM_PWM_PWMDeadTime(ADDRESS, DEADTIME)                                               \
	Cy_TCPWM_PWM_PWMDeadTime((TCPWM_Type *)ADDRESS, 0, DEADTIME)
#define IFX_TCPWM_PWM_PWMDeadTimeBuff(ADDRESS, DEADTIME)                                           \
	Cy_TCPWM_PWM_PWMDeadTimeBuff((TCPWM_Type *)ADDRESS, 0, DEADTIME)
#define IFX_TCPWM_PWM_PWMDeadTimeBuffN(ADDRESS, DEADTIME)                                          \
	Cy_TCPWM_PWM_PWMDeadTimeBuffN((TCPWM_Type *)ADDRESS, 0, DEADTIME)
#define IFX_TCPWM_PWM_PWMDeadTimeN(ADDRESS, DEADTIME)                                              \
	Cy_TCPWM_PWM_PWMDeadTimeN((TCPWM_Type *)ADDRESS, 0, DEADTIME)
#define IFX_TCPWM_PWM_SetCompare0BufVal(ADDRESS, COMPAREBUF0)                                      \
	Cy_TCPWM_PWM_SetCompare0BufVal((TCPWM_Type *)ADDRESS, 0, COMPAREBUF0)
#define IFX_TCPWM_PWM_SetCompare0Val(ADDRESS, COMPARE0)                                            \
	Cy_TCPWM_PWM_SetCompare0Val((TCPWM_Type *)ADDRESS, 0, COMPARE0)
#define IFX_TCPWM_PWM_SetCounter(ADDRESS, COUNT)                                                   \
	Cy_TCPWM_PWM_SetCounter((TCPWM_Type *)ADDRESS, 0, COUNT)
#define IFX_TCPWM_PWM_SetDT(ADDRESS, DEADTIME)                                                     \
	Cy_TCPWM_PWM_SetDT((TCPWM_Type *)ADDRESS, 0, DEADTIME)
#define IFX_TCPWM_PWM_SetDTBuff(ADDRESS, DEADTIME)                                                 \
	Cy_TCPWM_PWM_SetDTBuff((TCPWM_Type *)ADDRESS, 0, DEADTIME)
#define IFX_TCPWM_PWM_SetPeriod0(ADDRESS, PERIOD0)                                                 \
	Cy_TCPWM_PWM_SetPeriod0((TCPWM_Type *)ADDRESS, 0, PERIOD0)
#define IFX_TCPWM_PWM_SetPeriod1(ADDRESS, PERIOD1)                                                 \
	Cy_TCPWM_PWM_SetPeriod1((TCPWM_Type *)ADDRESS, 0, PERIOD1)
#define IFX_TCPWM_TriggerCaptureOrSwap_Single(ADDRESS)                                             \
	Cy_TCPWM_TriggerCaptureOrSwap_Single((TCPWM_Type *)ADDRESS, 0)
#define IFX_en_tcpwm_status_t(ADDRESS, MODE, PERIOD, DUTY, LIMITER)                                \
	cy_en_tcpwm_status_t Cy_TCPWM_PWM_Configure_Dithering((TCPWM_Type *)ADDRESS, 0, MODE,      \
							      PERIOD, DUTY, LIMITER)
/** @endcond */
#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_IFX_TCPWM_TIMER_H */
