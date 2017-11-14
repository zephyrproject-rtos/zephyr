#ifndef __ALT_AVALON_TIMER_H__
#define __ALT_AVALON_TIMER_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
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

#include <string.h>

#include "alt_types.h"
#include "sys/alt_dev.h"
#include "sys/alt_warning.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#define __ALT_COUNTER_SIZE(name) name##_COUNTER_SIZE
#define _ALT_COUNTER_SIZE(name) __ALT_COUNTER_SIZE(name)

#define ALT_SYS_CLK_COUNTER_SIZE _ALT_COUNTER_SIZE(ALT_SYS_CLK)
#define ALT_TIMESTAMP_COUNTER_SIZE _ALT_COUNTER_SIZE(ALT_TIMESTAMP_CLK)

#if (ALT_SYS_CLK_COUNTER_SIZE == 64)
#define alt_sysclk_type alt_u64
#else
#define alt_sysclk_type alt_u32
#endif

#if (ALT_TIMESTAMP_COUNTER_SIZE == 64)
#define alt_timestamp_type alt_u64
#else
#define alt_timestamp_type alt_u32
#endif

/*
 * The function alt_avalon_timer_sc_init() is the initialisation function for 
 * the system clock. It registers the timers interrupt handler, and then calls 
 * the system clock regestration function, alt_sysclk_init().
 */

extern void alt_avalon_timer_sc_init (void* base, alt_u32 irq_controller_id, 
                                      alt_u32 irq, alt_u32 freq);

/*
 * Variables used to store the timestamp parameters, when the device is to be
 * accessed using the high resolution timestamp driver.
 */

extern void*   altera_avalon_timer_ts_base;
extern alt_u32 altera_avalon_timer_ts_freq;

/*
 * ALTERA_AVALON_TIMER_INSTANCE is the macro used by alt_sys_init() to 
 * allocate any per device memory that may be required. In this case no 
 * allocation is necessary.
 */ 

#define ALTERA_AVALON_TIMER_INSTANCE(name, dev) extern int alt_no_storage

/*
 * Macro used to calculate the timer interrupt frequency. Although this is 
 * somewhat fearsome, when compiled with -O2 it will be resolved at compile 
 * time to a constant value.
 */

#define ALTERA_AVALON_TIMER_FREQ(freq, period, units) \
  strcmp (units, "us") ?                              \
    (strcmp (units, "ms") ?                           \
      (strcmp (units, "s") ?                          \
        ((freq + (period - 1))/period)                \
      : 1)                                            \
    : (1000 + (period - 1))/period)                   \
  : ((1000000 + (period - 1))/period)         

/*
 * Construct macros which contain the base address of the system clock and the
 * timestamp device. These are used below to determine which driver to use for
 * a given timer.
 */

#define __ALT_CLK_BASE(name) name##_BASE
#define _ALT_CLK_BASE(name) __ALT_CLK_BASE(name)

#define ALT_SYS_CLK_BASE _ALT_CLK_BASE(ALT_SYS_CLK)
#define ALT_TIMESTAMP_CLK_BASE _ALT_CLK_BASE(ALT_TIMESTAMP_CLK)

/*
 * If there is no system clock, then the above macro will result in 
 * ALT_SYS_CLK_BASE being set to none_BASE. We therefore need to provide an
 * invalid value for this, so that no timer is wrongly identified as the system
 * clock.
 */

#define none_BASE 0xffffffff

/*
 * ALTERA_AVALON_TIMER_INIT is the macro used by alt_sys_init() to provide
 * the run time initialisation of the device. In this case this translates to
 * a call to alt_avalon_timer_sc_init() if the device is the system clock, i.e.
 * if it has the name "sysclk".
 *
 * If the device is not the system clock, then it is used to provide the
 * timestamp facility. 
 *
 * To ensure as much as possible is evaluated at compile time, rather than 
 * compare the name of the device to "/dev/sysclk" using strcmp(), the base
 * address of the device is compared to SYSCLK_BASE to determine whether it's
 * the system clock. Since the base address of a device must be unique, these
 * two aproaches are equivalent.
 *
 * This macro performs a sanity check to ensure that the interrupt has been
 * connected for this device. If not, then an apropriate error message is 
 * generated at build time.
 */


#define ALTERA_AVALON_TIMER_INIT(name, dev)                                   \
  if (name##_BASE == ALT_SYS_CLK_BASE)                                        \
  {                                                                           \
    if (name##_IRQ == ALT_IRQ_NOT_CONNECTED)                                  \
    {                                                                         \
      ALT_LINK_ERROR ("Error: Interrupt not connected for " #dev ". "         \
                      "The system clock driver requires an interrupt to be "  \
                      "connected. Please select an IRQ for this device in "   \
                      "SOPC builder.");                                       \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      alt_avalon_timer_sc_init((void*) name##_BASE,                           \
                               name##_IRQ_INTERRUPT_CONTROLLER_ID,            \
                               name##_IRQ,                                    \
                               ALTERA_AVALON_TIMER_FREQ(name##_FREQ,          \
                                                        name##_PERIOD,        \
                                                        name##_PERIOD_UNITS));\
    }                                                                         \
  }                                                                           \
  else if (name##_BASE == ALT_TIMESTAMP_CLK_BASE)                             \
  {                                                                           \
    if (name##_SNAPSHOT)                                                      \
    {                                                                         \
      altera_avalon_timer_ts_base = (void*) name##_BASE;                      \
      altera_avalon_timer_ts_freq = name##_FREQ;                              \
    }                                                                         \
    else                                                                      \
    {                                                                         \
       ALT_LINK_ERROR ("Error: Snapshot register not available for "          \
                       #dev ". "                                              \
                      "The timestamp driver requires the snapshot register "  \
                      "to be readable. Please enable this register for this " \
                      "device in SOPC builder.");                             \
    }                                                                         \
  }

/*
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_AVALON_TIMER_H__ */
