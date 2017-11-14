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
/* ----------------------- SMALL DRIVER ---------------------- */
/* ----------------------------------------------------------- */

/* Read routine.  The small version blocks until it has at least one byte
 * available, it then returns as much as is immediately available without
 * waiting any more.  It's performance will be very poor without
 * interrupts.
 */

int 
altera_avalon_jtag_uart_read(altera_avalon_jtag_uart_state* sp, 
  char* buffer, int space, int flags)
{
  unsigned int base = sp->base;

  char * ptr = buffer;
  char * end = buffer + space;

  while (ptr < end)
  {
    unsigned int data = IORD_ALTERA_AVALON_JTAG_UART_DATA(base);

    if (data & ALTERA_AVALON_JTAG_UART_DATA_RVALID_MSK)
      *ptr++ = (data & ALTERA_AVALON_JTAG_UART_DATA_DATA_MSK) >> ALTERA_AVALON_JTAG_UART_DATA_DATA_OFST;
    else if (ptr != buffer)
      break;
    else if(flags & O_NONBLOCK)
      break;   
    
  }

  if (ptr != buffer)
    return ptr - buffer;
  else if (flags & O_NONBLOCK)
    return -EWOULDBLOCK;
  else
    return -EIO;
}

#else /* !ALTERA_AVALON_JTAG_UART_SMALL */

/* ----------------------------------------------------------- */
/* ----------------------- FAST DRIVER ----------------------- */
/* ----------------------------------------------------------- */

int 
altera_avalon_jtag_uart_read(altera_avalon_jtag_uart_state* sp, 
  char * buffer, int space, int flags)
{
  char * ptr = buffer;

  alt_irq_context context;
  unsigned int n;

  /*
   * When running in a multi threaded environment, obtain the "read_lock"
   * semaphore. This ensures that reading from the device is thread-safe.
   */
  ALT_SEM_PEND (sp->read_lock, 0);

  while (space > 0)
  {
    unsigned int in, out;

    /* Read as much data as possible */
    do
    {
      in  = sp->rx_in;
      out = sp->rx_out;

      if (in >= out)
        n = in - out;
      else
        n = ALTERA_AVALON_JTAG_UART_BUF_LEN - out;

      if (n == 0)
        break; /* No more data available */

      if (n > space)
        n = space;

      memcpy(ptr, sp->rx_buf + out, n);
      ptr   += n;
      space -= n;

      sp->rx_out = (out + n) % ALTERA_AVALON_JTAG_UART_BUF_LEN;
    }
    while (space > 0);

    /* If we read any data then return it */
    if (ptr != buffer)
      break;

    /* If in non-blocking mode then return error */
    if (flags & O_NONBLOCK)
      break;

#ifdef __ucosii__
    /* OS Present: Pend on a flag if the OS is running, otherwise spin */
    if(OSRunning == OS_TRUE) {
      /*
       * When running in a multi-threaded mode, we pend on the read event
       * flag set and timeout event flag set in the isr. This avoids wasting CPU
       * cycles waiting in this thread, when we could be doing something more
       * profitable elsewhere.
       */
      ALT_FLAG_PEND (sp->events,
                     ALT_JTAG_UART_READ_RDY | ALT_JTAG_UART_TIMEOUT,
                     OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME,
                     0);
    }
    else {
      /* Spin until more data arrives or until host disconnects */
      while (in == sp->rx_in && sp->host_inactive < sp->timeout)
        ;
    }
#else
    /* No OS: Always spin */
    while (in == sp->rx_in && sp->host_inactive < sp->timeout)
      ;
#endif /* __ucosii__ */

    if (in == sp->rx_in)
      break;
  }

  /*
   * Now that access to the circular buffer is complete, release the read
   * semaphore so that other threads can access the buffer.
   */

  ALT_SEM_POST (sp->read_lock);

  if (ptr != buffer)
  {
    /* If we read any data then there is space in the buffer so enable interrupts */
    context = alt_irq_disable_all();
    sp->irq_enable |= ALTERA_AVALON_JTAG_UART_CONTROL_RE_MSK;
    IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(sp->base, sp->irq_enable);
    alt_irq_enable_all(context);
  }

  if (ptr != buffer)
    return ptr - buffer;
  else if (flags & O_NONBLOCK)
    return -EWOULDBLOCK;
  else
    return -EIO;
}

#endif /* ALTERA_AVALON_JTAG_UART_SMALL */
