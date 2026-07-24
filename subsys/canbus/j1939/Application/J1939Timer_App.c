/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include <J1939Timer.h>

/**************************************************************************************************/
void j1939_timer_app_init(void)
{
   // TODO Configure for your project here. This will typically involve something like setting
   // up a free-running timer in hardware or a timer in your operating system
}

/**************************************************************************************************/
j1939_timer_t j1939_timer_app_get_time(void)
{
   // TODO Configure for your project here. This will typically involve something like returning
   // the time from an operating system function or a hardware timer.
#ifdef WIN32
   return ((j1939_timer_t)GetTickCount());
#else
// #error You must implement j1939_timer_app_get_time
#endif

   return (0);
}
