/*
 * Copyright (c) 2026 Caleb Perkinson
 * SPDX-License-Identifier: Apache-2.0
 */
#include <J1939Timer.h>

/**************************************************************************************************/
void J1939Timer_App_Init(void)
{
   // TODO Configure for your project here. This will typically involve something like setting
   // up a free-running timer in hardware or a timer in your operating system
}

/**************************************************************************************************/
J1939_Timer_T J1939Timer_App_GetTime(void)
{
   // TODO Configure for your project here. This will typically involve something like returning
   // the time from an operating system function or a hardware timer.
#ifdef WIN32
   return ((J1939_Timer_T)GetTickCount());
#else
// #error You must implement J1939Timer_App_GetTime
#endif

   return (0);
}

#endif
