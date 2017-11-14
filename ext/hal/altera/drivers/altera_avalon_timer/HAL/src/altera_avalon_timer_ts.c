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

#include "system.h"
#include "sys/alt_timestamp.h"

#include "altera_avalon_timer.h"
#include "altera_avalon_timer_regs.h"

#include "alt_types.h"

/*
 * These functions are only available if a timestamp device has been selected
 * for this system.
 */

#if (ALT_TIMESTAMP_CLK_BASE != none_BASE) 

/*
 * The function alt_timestamp_start() can be called at application level to
 * initialise the timestamp facility. In this case the period register is
 * set to full scale, i.e. 0xffffffff, and then started running. Note that
 * the period register may not be writable, depending on the hardware 
 * configuration, in which case this function does not reset the period.
 *
 * The timer is not run in continuous mode, so that the user can detect timer
 * roll-over, i.e. alt_timestamp() returns 0.
 *
 * The return value of this function is 0 upon sucess and -1 if in timestamp
 * device has not been registered. 
 */

int alt_timestamp_start(void)
{
  void* base = altera_avalon_timer_ts_base;

  if (!altera_avalon_timer_ts_freq)
  {
    return -1;
  }
  else
  {
    if(ALT_TIMESTAMP_COUNTER_SIZE == 64) {
        IOWR_ALTERA_AVALON_TIMER_CONTROL (base,ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);
        IOWR_ALTERA_AVALON_TIMER_PERIOD_0 (base, 0xFFFF);
        IOWR_ALTERA_AVALON_TIMER_PERIOD_1 (base, 0xFFFF);;
        IOWR_ALTERA_AVALON_TIMER_PERIOD_2 (base, 0xFFFF);
        IOWR_ALTERA_AVALON_TIMER_PERIOD_3 (base, 0xFFFF);
        IOWR_ALTERA_AVALON_TIMER_CONTROL (base, ALTERA_AVALON_TIMER_CONTROL_START_MSK);
    } else {
        IOWR_ALTERA_AVALON_TIMER_CONTROL (base,ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);
        IOWR_ALTERA_AVALON_TIMER_PERIODL (base, 0xFFFF);
        IOWR_ALTERA_AVALON_TIMER_PERIODH (base, 0xFFFF);
        IOWR_ALTERA_AVALON_TIMER_CONTROL (base, ALTERA_AVALON_TIMER_CONTROL_START_MSK); 
    } 
  }
  return 0;
}

/*
 * alt_timestamp() returns the current timestamp count. In the event that
 * the timer has run full period, or there is no timestamp available, this
 * function return -1.
 *
 * The returned timestamp counts up from the last time the period register
 * was reset. 
 */

alt_timestamp_type alt_timestamp(void)
{

  void* base = altera_avalon_timer_ts_base;

  if (!altera_avalon_timer_ts_freq)
  {
#if (ALT_TIMESTAMP_COUNTER_SIZE == 64)
        return 0xFFFFFFFFFFFFFFFFULL;
#else
        return 0xFFFFFFFF;
#endif
  }
  else
  {
#if (ALT_TIMESTAMP_COUNTER_SIZE == 64)
        IOWR_ALTERA_AVALON_TIMER_SNAP_0 (base, 0);
        alt_timestamp_type snap_0 = IORD_ALTERA_AVALON_TIMER_SNAP_0(base) & ALTERA_AVALON_TIMER_SNAP_0_MSK;
        alt_timestamp_type snap_1 = IORD_ALTERA_AVALON_TIMER_SNAP_1(base) & ALTERA_AVALON_TIMER_SNAP_1_MSK;
        alt_timestamp_type snap_2 = IORD_ALTERA_AVALON_TIMER_SNAP_2(base) & ALTERA_AVALON_TIMER_SNAP_2_MSK;
        alt_timestamp_type snap_3 = IORD_ALTERA_AVALON_TIMER_SNAP_3(base) & ALTERA_AVALON_TIMER_SNAP_3_MSK;
        
        return (0xFFFFFFFFFFFFFFFFULL - ( (snap_3 << 48) | (snap_2 << 32) | (snap_1 << 16) | (snap_0) ));
#else
        IOWR_ALTERA_AVALON_TIMER_SNAPL (base, 0);
        alt_timestamp_type lower = IORD_ALTERA_AVALON_TIMER_SNAPL(base) & ALTERA_AVALON_TIMER_SNAPL_MSK;
        alt_timestamp_type upper = IORD_ALTERA_AVALON_TIMER_SNAPH(base) & ALTERA_AVALON_TIMER_SNAPH_MSK;
        
        return (0xFFFFFFFF - ((upper << 16) | lower)); 
#endif
  }
}

/*
 * Return the number of timestamp ticks per second. This will be 0 if no
 * timestamp device has been registered.
 */

alt_u32 alt_timestamp_freq(void)
{
  return altera_avalon_timer_ts_freq;
}

#endif /* timestamp available */
