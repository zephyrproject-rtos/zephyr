/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2006 Altera Corporation, San Jose, California, USA.           *
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
******************************************************************************/

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <sys/stat.h>

#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "sys/ioctl.h"
#include "alt_types.h"

#include "altera_avalon_jtag_uart_regs.h"
#include "altera_avalon_jtag_uart.h"

#include "sys/alt_log_printf.h"

#ifdef __ucosii__
#include "includes.h"
#endif /* __ucosii__ */

#ifdef ALTERA_AVALON_JTAG_UART_SMALL

/* ----------------------------------------------------------- */
/* ------------------------ SMALL DRIVER --------------------- */
/* ----------------------------------------------------------- */

/* Write routine.  The small version blocks when there is no space to write
 * into, so it's performance will be very bad if you are writing more than
 * one FIFOs worth of data.  But you said you didn't want to use interrupts :-)
 */

int altera_avalon_jtag_uart_write(altera_avalon_jtag_uart_state* sp, 
  const char * ptr, int count, int flags)
{
  unsigned int base = sp->base;

  const char * end = ptr + count;

  while (ptr < end)
    if ((IORD_ALTERA_AVALON_JTAG_UART_CONTROL(base) & ALTERA_AVALON_JTAG_UART_CONTROL_WSPACE_MSK) != 0)
      IOWR_ALTERA_AVALON_JTAG_UART_DATA(base, *ptr++);

  return count;
}

#else /* !ALTERA_AVALON_JTAG_UART_SMALL */

/* ----------------------------------------------------------- */
/* ------------------------- FAST DRIVER --------------------- */
/* ----------------------------------------------------------- */

int 
altera_avalon_jtag_uart_write(altera_avalon_jtag_uart_state* sp, 
  const char * ptr, int count, int flags)
{
  /* Remove warning at optimisation level 03 by seting out to 0 */
  unsigned int in, out=0;
  unsigned int n;
  alt_irq_context context;

  const char * start = ptr;

  /*
   * When running in a multi threaded environment, obtain the "write_lock"
   * semaphore. This ensures that writing to the device is thread-safe.
   */
  ALT_SEM_PEND (sp->write_lock, 0);

  do
  {
    /* Copy as much as we can into the transmit buffer */
    while (count > 0)
    {
      /* We need a stable value of the out pointer to calculate the space available */
      in  = sp->tx_in;
      out = sp->tx_out;

      if (in < out)
        n = out - 1 - in;
      else if (out > 0)
        n = ALTERA_AVALON_JTAG_UART_BUF_LEN - in;
      else
        n = ALTERA_AVALON_JTAG_UART_BUF_LEN - 1 - in;

      if (n == 0)
        break;

      if (n > count)
        n = count;

      memcpy(sp->tx_buf + in, ptr, n);
      ptr   += n;
      count -= n;

      sp->tx_in = (in + n) % ALTERA_AVALON_JTAG_UART_BUF_LEN;
    }

    /*
     * If interrupts are disabled then we could transmit here, we only need 
     * to enable interrupts if there is no space left in the FIFO
     *
     * For now kick the interrupt routine every time to make it transmit 
     * the data 
     */
    context = alt_irq_disable_all();
    sp->irq_enable |= ALTERA_AVALON_JTAG_UART_CONTROL_WE_MSK;
    IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(sp->base, sp->irq_enable);
    alt_irq_enable_all(context);

    /* 
     * If there is any data left then either return now or block until 
     * some has been sent 
     */
    /* consider: test whether there is anything there while doing this and delay for at most 2s. */
    if (count > 0)
    {
      if (flags & O_NONBLOCK)
        break;

#ifdef __ucosii__
      /* OS Present: Pend on a flag if the OS is running, otherwise spin */
      if(OSRunning == OS_TRUE) {
        /*
         * When running in a multi-threaded mode, we pend on the write event
         * flag set or the timeout flag in the isr. This avoids wasting CPU
         * cycles waiting in this thread, when we could be doing something
         * more profitable elsewhere.
         */
#ifdef ALTERA_AVALON_JTAG_UART_IGNORE_FIFO_FULL_ERROR
        if(!sp->host_inactive)
#endif
        ALT_FLAG_PEND (sp->events,
                       ALT_JTAG_UART_WRITE_RDY | ALT_JTAG_UART_TIMEOUT,
                       OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME,
                       0);
      }
      else {
        /*
         * OS not running: Wait for data to be removed from buffer.
         * Once the interrupt routine has removed some data then we
         * will be able to insert some more.
         */
        while (out == sp->tx_out && sp->host_inactive < sp->timeout)
          ;
      }
#else
      /*
       * No OS present: Always wait for data to be removed from buffer.  Once
       * the interrupt routine has removed some data then we will be able to
       * insert some more.
       */
      while (out == sp->tx_out && sp->host_inactive < sp->timeout)
        ;
#endif /* __ucosii__ */

      if  (sp->host_inactive)
         break;
    }
  }
  while (count > 0);

  /*
   * Now that access to the circular buffer is complete, release the write
   * semaphore so that other threads can access the buffer.
   */
  ALT_SEM_POST (sp->write_lock);

  if (ptr != start)
    return ptr - start;
  else if (flags & O_NONBLOCK)
    return -EWOULDBLOCK;
#ifdef ALTERA_AVALON_JTAG_UART_IGNORE_FIFO_FULL_ERROR
  else if (sp->host_inactive >= sp->timeout) {
    /* 
     * Reset the software FIFO, hardware FIFO could not be reset.
     * Just throw away characters without reporting error. 
     */
    sp->tx_out = sp->tx_in = 0;
    return ptr - start + count;
  }
#endif
  else
    return -EIO; /* Host not connected */
}

#endif /* ALTERA_AVALON_JTAG_UART_SMALL */
