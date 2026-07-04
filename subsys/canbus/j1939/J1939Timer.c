/*
 * Copyright (c) 2026 Caleb Perkinson
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939Timer.h"
#include <zephyr/kernel.h>

/**************************************************************************************************/
void J1939Timer_Init(void)
{
   J1939Timer_App_Init();
}

/**************************************************************************************************/
J1939_Timer_T J1939Timer_Elapse(J1939_Timer_T *elapsedTime)
{
   J1939_Timer_T currentTime = J1939Timer_GetTime();
   J1939_Timer_T delta = 0;

   if (elapsedTime)
   {
      delta = (J1939_Timer_T)(currentTime - *elapsedTime);
      *elapsedTime = currentTime;
   }

   return (delta);
}

/**************************************************************************************************/
J1939_Timer_T J1939Timer_GetTime(void)
{
   return (J1939_Timer_T)k_uptime_get_32();
}
