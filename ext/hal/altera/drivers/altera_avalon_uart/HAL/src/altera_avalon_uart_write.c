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

#include <fcntl.h>

#include "sys/alt_dev.h"
#include "sys/alt_irq.h"
#include "sys/ioctl.h"
#include "sys/alt_errno.h"

#include "altera_avalon_uart_regs.h"
#include "altera_avalon_uart.h"

#if defined(ALT_USE_SMALL_DRIVERS) || defined(ALTERA_AVALON_UART_SMALL)

/* ----------------------------------------------------------- */
/* ------------------------ SMALL DRIVER --------------------- */
/* ----------------------------------------------------------- */

/*
 * altera_avalon_uart_write() is called by the system write() function in 
 * order to write a block of data to the UART. 
 * "len" is the length of the data to write,
 * and "ptr" indicates the source address. "fd" is the file descriptor for the 
 * device to be read from.
 *
 * Permission checks are made before the call to altera_avalon_uart_write(), so
 * we know that the file descriptor has been opened with the correct permissions
 * for this operation.
 *
 * The return value is the number of bytes actually written.
 *
 * This function will block on the devices transmit register, until all 
 * characters have been transmitted. This is unless the device is being 
 * accessed in non-blocking mode. In this case this function will return as 
 * soon as the device reports that it is not ready to transmit.
 *
 * Since this is the small footprint version of the UART driver, the value of 
 * CTS is ignored.
 */

int 
altera_avalon_uart_write(altera_avalon_uart_state* sp, const char* ptr, int len,
  int flags)
{
  int block;
  unsigned int status;
  int count;

  block = !(flags & O_NONBLOCK);
  count = len;

  do
  {
    status = IORD_ALTERA_AVALON_UART_STATUS(sp->base);
   
    if (status & ALTERA_AVALON_UART_STATUS_TRDY_MSK)
    {
      IOWR_ALTERA_AVALON_UART_TXDATA(sp->base, *ptr++);
      count--;
    }
  }
  while (block && count);

  if (count)
  {
    ALT_ERRNO = EWOULDBLOCK;
  }

  return (len - count);
}

#else /* Using the "fast" version of the driver */

/* ----------------------------------------------------------- */
/* ------------------------- FAST DRIVER --------------------- */
/* ----------------------------------------------------------- */

/*
 * altera_avalon_uart_write() is called by the system write() function in order
 * to write a block of data to the UART. "len" is the length of the data to 
 * write, and "ptr" indicates the source address. "sp" is the state pointer
 * for the device to be written to.
 *
 * Permission checks are made before the call to altera_avalon_uart_write(), so
 * we know that the file descriptor has been opened with the correct permissions
 * for this operation.
 *
 * The return value is the number of bytes actually written.
 *
 * This function does not communicate with the device directly. Instead data is
 * transfered to a circular buffer. The interrupt handler is then responsible
 * for copying data from this buffer into the device.
 */

int
altera_avalon_uart_write(altera_avalon_uart_state* sp, const char* ptr, int len,
  int flags)
{
  alt_irq_context context;
  int             no_block;
  alt_u32         next;
  int             count = len;

  /* 
   * Construct a flag to indicate whether the device is being accessed in
   * blocking or non-blocking mode.
   */

  no_block = (flags & O_NONBLOCK);

  /*
   * When running in a multi threaded environment, obtain the "write_lock"
   * semaphore. This ensures that writing to the device is thread-safe.
   */

  ALT_SEM_PEND (sp->write_lock, 0);

  /*
   * Loop transferring data from the input buffer to the transmit circular
   * buffer. The loop is terminated once all the data has been transferred,
   * or, (if in non-blocking mode) the buffer becomes full.
   */

  while (count)
  {
    /* Determine the next slot in the buffer to access */

    next = (sp->tx_end + 1) & ALT_AVALON_UART_BUF_MSK;

    /* block waiting for space if necessary */

    if (next == sp->tx_start)
    {
      if (no_block)
      {
        /* Set errno to indicate why this function returned early */
 
        ALT_ERRNO = EWOULDBLOCK;
        break;
      }
      else
      {
        /* Block waiting for space in the circular buffer */

        /* First, ensure transmit interrupts are enabled to avoid deadlock */

        context = alt_irq_disable_all ();
        sp->ctrl |= (ALTERA_AVALON_UART_CONTROL_TRDY_MSK |
                        ALTERA_AVALON_UART_CONTROL_DCTS_MSK);
        IOWR_ALTERA_AVALON_UART_CONTROL(sp->base, sp->ctrl);
        alt_irq_enable_all (context);

        /* wait for space to come free */

        do
        {
          /*
           * When running in a multi-threaded mode, we pend on the write event 
           * flag set in the interrupt service routine. This avoids wasting CPU
           * cycles waiting in this thread, when we could be doing something
           * more profitable elsewhere.
           */

          ALT_FLAG_PEND (sp->events, 
                         ALT_UART_WRITE_RDY,
                         OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME,
                         0);
        }
        while ((next == sp->tx_start));
      }
    }

    count--;

    /* Add the next character to the transmit buffer */

    sp->tx_buf[sp->tx_end] = *ptr++;
    sp->tx_end = next;
  }

  /*
   * Now that access to the circular buffer is complete, release the write
   * semaphore so that other threads can access the buffer.
   */

  ALT_SEM_POST (sp->write_lock);

  /* 
   * Ensure that interrupts are enabled, so that the circular buffer can 
   * drain.
   */

  context = alt_irq_disable_all ();
  sp->ctrl |= ALTERA_AVALON_UART_CONTROL_TRDY_MSK |
                 ALTERA_AVALON_UART_CONTROL_DCTS_MSK;
  IOWR_ALTERA_AVALON_UART_CONTROL(sp->base, sp->ctrl);
  alt_irq_enable_all (context);

  /* return the number of bytes written */

  return (len - count);
}

#endif /* fast driver */
