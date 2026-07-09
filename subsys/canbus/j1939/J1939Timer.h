/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _J1939_TIMER_H_
#define _J1939_TIMER_H_

#include <zephyr/canbus/j1939.h>

/// Initialize timer functions
void J1939Timer_Init(void);

/// @brief Calculate elapsed time
/// @note The first time this function is called, the return value is undefined.  Subsequent
/// calls return the time in miliseconds from the previous call.If the delta exceeds the maximum
/// possible value of J1939Timer_T, this function will return an inaccurate value. Therefore all
/// time calculations using this function must be less than that number of milliseconds.If the
/// elapsedTime pointer is NULL, then the function will return 0
/// @param elapsedTime Previous time
/// @return See note
J1939_Timer_T J1939Timer_Elapse(J1939_Timer_T *elapsedTime);

/// @brief Determine the current time
/// @return Current time in milliseconds
J1939_Timer_T J1939Timer_GetTime(void);

/// Application layer specific initialization needs
void J1939Timer_App_Init(void);

/// @brief Application specific handling of getting the current system time.
/// @return Current time in milliseconds
J1939_Timer_T J1939Timer_App_GetTime(void);

#endif
