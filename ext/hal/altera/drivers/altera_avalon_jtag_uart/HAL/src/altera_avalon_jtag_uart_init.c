/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2007 Altera Corporation, San Jose, California, USA.           *
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

#ifndef ALTERA_AVALON_JTAG_UART_SMALL

/* ----------------------------------------------------------- */
/* ------------------------- FAST DRIVER --------------------- */
/* ----------------------------------------------------------- */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
static void altera_avalon_jtag_uart_irq(void* context);
#else
static void altera_avalon_jtag_uart_irq(void* context, alt_u32 id);
#endif 
static alt_u32 altera_avalon_jtag_uart_timeout(void* context);

/* 
 * Driver initialization code.  Register interrupts and start a timer
 * which we can use to check whether the host is there.
 * Return 1 on sucessful IRQ register and 0 on failure.
 */

void altera_avalon_jtag_uart_init(altera_avalon_jtag_uart_state* sp, 
                                  int irq_controller_id, int irq)
{
  ALT_FLAG_CREATE(&sp->events, 0);
  ALT_SEM_CREATE(&sp->read_lock, 1);
  ALT_SEM_CREATE(&sp->write_lock, 1);

  /* enable read interrupts at the device */
  sp->irq_enable = ALTERA_AVALON_JTAG_UART_CONTROL_RE_MSK;

  IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(sp->base, sp->irq_enable); 
  
  /* register the interrupt handler */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
  alt_ic_isr_register(irq_controller_id, irq, altera_avalon_jtag_uart_irq, 
                      sp, NULL);
#else
  alt_irq_register(irq, sp, altera_avalon_jtag_uart_irq);
#endif  

  /* Register an alarm to go off every second to check for presence of host */
  sp->host_inactive = 0;

  if (alt_alarm_start(&sp->alarm, alt_ticks_per_second(), 
    &altera_avalon_jtag_uart_timeout, sp) < 0)
  {
    /* If we can't set the alarm then record "don't know if host present" 
     * and behave as though the host is present.
     */
    sp->timeout = INT_MAX;
  }

  /* ALT_LOG - see altera_hal/HAL/inc/sys/alt_log_printf.h */ 
  ALT_LOG_JTAG_UART_ALARM_REGISTER(sp, sp->base);
}

/*
 * Interrupt routine
 */ 
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
static void altera_avalon_jtag_uart_irq(void* context)
#else
static void altera_avalon_jtag_uart_irq(void* context, alt_u32 id)
#endif
{
  altera_avalon_jtag_uart_state* sp = (altera_avalon_jtag_uart_state*) context;
  unsigned int base = sp->base;

  /* ALT_LOG - see altera_hal/HAL/inc/sys/alt_log_printf.h */ 
  ALT_LOG_JTAG_UART_ISR_FUNCTION(base, sp);

  for ( ; ; )
  {
    unsigned int control = IORD_ALTERA_AVALON_JTAG_UART_CONTROL(base);

    /* Return once nothing more to do */
    if ((control & (ALTERA_AVALON_JTAG_UART_CONTROL_RI_MSK | ALTERA_AVALON_JTAG_UART_CONTROL_WI_MSK)) == 0)
      break;

    if (control & ALTERA_AVALON_JTAG_UART_CONTROL_RI_MSK)
    {
      /* process a read irq.  Start by assuming that there is data in the
       * receive FIFO (otherwise why would we have been interrupted?)
       */
      unsigned int data = 1 << ALTERA_AVALON_JTAG_UART_DATA_RAVAIL_OFST;

      for ( ; ; )
      {
        /* Check whether there is space in the buffer.  If not then we must not
         * read any characters from the buffer as they will be lost.
         */
        unsigned int next = (sp->rx_in + 1) % ALTERA_AVALON_JTAG_UART_BUF_LEN;
        if (next == sp->rx_out)
          break;

        /* Try to remove a character from the FIFO and find out whether there
         * are any more characters remaining.
         */
        data = IORD_ALTERA_AVALON_JTAG_UART_DATA(base);
        
        if ((data & ALTERA_AVALON_JTAG_UART_DATA_RVALID_MSK) == 0)
          break;

        sp->rx_buf[sp->rx_in] = (data & ALTERA_AVALON_JTAG_UART_DATA_DATA_MSK) >> ALTERA_AVALON_JTAG_UART_DATA_DATA_OFST;
        sp->rx_in = (sp->rx_in + 1) % ALTERA_AVALON_JTAG_UART_BUF_LEN;

        /* Post an event to notify jtag_uart_read that a character has been read */
        ALT_FLAG_POST (sp->events, ALT_JTAG_UART_READ_RDY, OS_FLAG_SET);
      }

      if (data & ALTERA_AVALON_JTAG_UART_DATA_RAVAIL_MSK)
      {
        /* If there is still data available here then the buffer is full 
         * so turn off receive interrupts until some space becomes available.
         */
        sp->irq_enable &= ~ALTERA_AVALON_JTAG_UART_CONTROL_RE_MSK;
        IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(base, sp->irq_enable);
        
        /* Dummy read to ensure IRQ is cleared prior to ISR completion */
        IORD_ALTERA_AVALON_JTAG_UART_CONTROL(base);
      }
    }

    if (control & ALTERA_AVALON_JTAG_UART_CONTROL_WI_MSK)
    {
      /* process a write irq */
      unsigned int space = (control & ALTERA_AVALON_JTAG_UART_CONTROL_WSPACE_MSK) >> ALTERA_AVALON_JTAG_UART_CONTROL_WSPACE_OFST;

      while (space > 0 && sp->tx_out != sp->tx_in)
      {
        IOWR_ALTERA_AVALON_JTAG_UART_DATA(base, sp->tx_buf[sp->tx_out]);

        sp->tx_out = (sp->tx_out + 1) % ALTERA_AVALON_JTAG_UART_BUF_LEN;

        /* Post an event to notify jtag_uart_write that a character has been written */
        ALT_FLAG_POST (sp->events, ALT_JTAG_UART_WRITE_RDY, OS_FLAG_SET);

        space--;
      }

      if (space > 0)
      {
        /* If we don't have any more data available then turn off the TX interrupt */
        sp->irq_enable &= ~ALTERA_AVALON_JTAG_UART_CONTROL_WE_MSK;
        IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(sp->base, sp->irq_enable);
        
        /* Dummy read to ensure IRQ is cleared prior to ISR completion */
        IORD_ALTERA_AVALON_JTAG_UART_CONTROL(base);
      }
    }
  }
}

/*
 * Timeout routine is called every second
 */

static alt_u32 
altera_avalon_jtag_uart_timeout(void* context) 
{
  altera_avalon_jtag_uart_state* sp = (altera_avalon_jtag_uart_state *) context;

  unsigned int control = IORD_ALTERA_AVALON_JTAG_UART_CONTROL(sp->base);

  if (control & ALTERA_AVALON_JTAG_UART_CONTROL_AC_MSK)
  {
    IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(sp->base, sp->irq_enable | ALTERA_AVALON_JTAG_UART_CONTROL_AC_MSK);
    sp->host_inactive = 0;
  }
  else if (sp->host_inactive < INT_MAX - 2) {
    sp->host_inactive++;
    
    if (sp->host_inactive >= sp->timeout) {
      /* Post an event to indicate host is inactive (for jtag_uart_read */
      ALT_FLAG_POST (sp->events, ALT_JTAG_UART_TIMEOUT, OS_FLAG_SET);
    }
  }

  return alt_ticks_per_second();
}

/*
 * The close() routine is implemented to drain the JTAG UART transmit buffer
 * when not in "small" mode. This routine will wait for transimt data to be
 * emptied unless a timeout from host-activity occurs. If the driver flags
 * have been set to non-blocking mode, this routine will exit immediately if
 * any data remains. This routine should be called indirectly (i.e. though
 * the C library close() routine) so that the file descriptor associated 
 * with the relevant stream (i.e. stdout) can be closed as well. This routine
 * does not manage file descriptors.
 * 
 * The close routine is not implemented for the small driver; instead it will
 * map to null. This is because the small driver simply waits while characters
 * are transmitted; there is no interrupt-serviced buffer to empty 
 */
int altera_avalon_jtag_uart_close(altera_avalon_jtag_uart_state* sp, int flags)
{
  /* 
   * Wait for all transmit data to be emptied by the JTAG UART ISR, or
   * for a host-inactivity timeout, in which case transmit data will be lost
   */
  while ( (sp->tx_out != sp->tx_in) && (sp->host_inactive < sp->timeout) ) {
    if (flags & O_NONBLOCK) {
      return -EWOULDBLOCK; 
    }
  }

  return 0;
}

#endif /* !ALTERA_AVALON_JTAG_UART_SMALL */
