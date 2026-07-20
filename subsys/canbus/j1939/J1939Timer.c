/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939Timer.h"
#include <zephyr/kernel.h>

/**************************************************************************************************/
void j1939_timer_init(void)
{
   j1939_timer_app_init();
}

/**************************************************************************************************/
j1939_timer_t j1939_timer_elapse(j1939_timer_t *elapsedTime)
{
   j1939_timer_t currentTime = j1939_timer_get_time();
   j1939_timer_t delta = 0;

   if (elapsedTime)
   {
      delta = (j1939_timer_t)(currentTime - *elapsedTime);
      *elapsedTime = currentTime;
   }

   return (delta);
}

/**************************************************************************************************/
j1939_timer_t j1939_timer_get_time(void)
{
   return (j1939_timer_t)k_uptime_get_32();
}
