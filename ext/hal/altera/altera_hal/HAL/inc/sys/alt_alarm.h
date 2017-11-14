#ifndef __ALT_ALARM_H__
#define __ALT_ALARM_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

#include "alt_llist.h"
#include "alt_types.h"

#include "priv/alt_alarm.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * "alt_alarm" is a structure type used by applications to register an alarm
 * callback function. An instance of this type must be passed as an input
 * argument to alt_alarm_start(). The user is not responsible for initialising
 * the contents of the instance. This is done by alt_alarm_start(). 
 */

typedef struct alt_alarm_s alt_alarm;

/* 
 * alt_alarm_start() can be called by an application/driver in order to register
 * a function for periodic callback at the system clock frequency. Be aware that
 * this callback is likely to occur in interrupt context. 
 */

extern int alt_alarm_start (alt_alarm* the_alarm, 
                            alt_u32    nticks, 
                            alt_u32    (*callback) (void* context),
                            void*      context);

/*
 * alt_alarm_stop() is used to unregister a callback. Alternatively the callback 
 * can return zero to unregister.
 */

extern void alt_alarm_stop (alt_alarm* the_alarm);

#ifndef ZEPHYR_RTOS
/*
 * Obtain the system clock rate in ticks/s. 
 */

static ALT_INLINE alt_u32 ALT_ALWAYS_INLINE alt_ticks_per_second (void)
{
  return _alt_tick_rate;
}

/*
 * alt_sysclk_init() is intended to be only used by the system clock driver
 * in order to initialise the value of the clock frequency.
 */

static ALT_INLINE int ALT_ALWAYS_INLINE alt_sysclk_init (alt_u32 nticks)
{
  if (! _alt_tick_rate)
  {
    _alt_tick_rate = nticks;
    return 0;
  }
  else
  {
    return -1;
  }
}

/*
 * alt_nticks() returns the elapsed number of system clock ticks since reset.
 */

static ALT_INLINE alt_u32 ALT_ALWAYS_INLINE alt_nticks (void)
{
  return _alt_nticks;
}

/*
 * alt_tick() should only be called by the system clock driver. This is used
 * to notify the system that the system timer period has expired.
 */

extern void alt_tick (void);
#else
/*
 * Obtain the system clock rate in ticks/s.
 */

static ALT_INLINE alt_u32 ALT_ALWAYS_INLINE alt_ticks_per_second (void)
{
  return 0;
}

/*
 * alt_sysclk_init() is intended to be only used by the system clock driver
 * in order to initialise the value of the clock frequency.
 */

static ALT_INLINE int ALT_ALWAYS_INLINE alt_sysclk_init (alt_u32 nticks)
{
  return 0;
}

/*
 * alt_nticks() returns the elapsed number of system clock ticks since reset.
 */

static ALT_INLINE alt_u32 ALT_ALWAYS_INLINE alt_nticks (void)
{
  return 0;
}

/*
 * alt_tick() should only be called by the system clock driver. This is used
 * to notify the system that the system timer period has expired.
 */

extern void alt_tick (void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ALT_ALARM_H__ */
