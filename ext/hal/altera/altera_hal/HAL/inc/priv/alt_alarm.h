#ifndef __ALT_PRIV_ALARM_H__
#define __ALT_PRIV_ALARM_H__

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

#include "alt_types.h"

/*
 * This header provides the internal defenitions required by the public 
 * interface alt_alarm.h. These variables and structures are not guaranteed to 
 * exist in future implementations of the HAL.
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * "alt_alarm_s" is a structure type used to maintain lists of alarm callback
 * functions.
 */

struct alt_alarm_s
{
  alt_llist llist;       /* linked list */
  alt_u32 time;          /* time in system ticks of the callback */
  alt_u32 (*callback) (void* context); /* callback function. The return 
                          * value is the period for the next callback; where 
                          * zero indicates that the alarm should be removed 
                          * from the list. 
                          */
  alt_u8 rollover;       /* set when desired alarm time + current time causes
                            overflow, to prevent premature alarm */
  void* context;         /* Argument for the callback */
};

/*
 * "_alt_tick_rate" is a global variable used to store the system clock rate 
 * in ticks per second. This is initialised to zero, which coresponds to there
 * being no system clock available. 
 *
 * It is then set to it's final value by the system clock driver through a call
 * to alt_sysclk_init(). 
 */

extern alt_u32 _alt_tick_rate;

/*
 * "_alt_nticks" is a global variable which records the elapsed number of 
 * system clock ticks since the last call to settimeofday() or since reset if
 * settimeofday() has not been called.
 */

extern volatile alt_u32 _alt_nticks;

/* The list of registered alarms. */

extern alt_llist alt_alarm_list;

#ifdef __cplusplus
}
#endif

#endif /* __ALT_PRIV_ALARM_H__ */
