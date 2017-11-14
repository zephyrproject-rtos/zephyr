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

#ifndef __ALT_AVALON_JTAG_UART_H__
#define __ALT_AVALON_JTAG_UART_H__

#include <stddef.h>

#include "sys/alt_alarm.h"
#include "sys/alt_warning.h"

#include "os/alt_sem.h"
#include "os/alt_flag.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* 
 * If the user wants all drivers to be small rather than fast then make sure
 * this one is marked as needing to be small.
 */
#if defined ALT_USE_SMALL_DRIVERS && !defined ALTERA_AVALON_JTAG_UART_SMALL
#define ALTERA_AVALON_JTAG_UART_SMALL
#endif

/* 
 * If the user wants to ignore FIFO full error after timeout
  */
#if defined ALT_JTAG_UART_IGNORE_FIFO_FULL_ERROR && !defined ALTERA_AVALON_JTAG_UART_IGNORE_FIFO_FULL_ERROR
#define ALTERA_AVALON_JTAG_UART_IGNORE_FIFO_FULL_ERROR
#endif

/*
 * Constants that can be overriden.
 */
#ifndef ALTERA_AVALON_JTAG_UART_DEFAULT_TIMEOUT
#define ALTERA_AVALON_JTAG_UART_DEFAULT_TIMEOUT 10
#endif

#ifndef ALTERA_AVALON_JTAG_UART_BUF_LEN
#define ALTERA_AVALON_JTAG_UART_BUF_LEN 2048
#endif

/*
 * ALT_JTAG_UART_READ_RDY and ALT_JTAG_UART_WRITE_RDY are the bitmasks 
 * that define uC/OS-II event flags that are releated to this device.
 *
 * ALT_JTAG_UART_READ_RDY indicates that there is read data in the buffer 
 * ready to be processed. ALT_JTAG_UART_WRITE_RDY indicates that the transmitter is
 * ready for more data.
 */
#define ALT_JTAG_UART_READ_RDY  0x1
#define ALT_JTAG_UART_WRITE_RDY 0x2
#define ALT_JTAG_UART_TIMEOUT   0x4

/*
 * State structure definition. Each instance of the driver uses one
 * of these structures to hold its associated state.
 */

typedef struct altera_avalon_jtag_uart_state_s
{
  unsigned int base;

#ifndef ALTERA_AVALON_JTAG_UART_SMALL
 
  unsigned int  timeout; /* Timeout until host is assumed inactive */
  alt_alarm     alarm;
  unsigned int  irq_enable;
  unsigned int  host_inactive;

  ALT_SEM      (read_lock)
  ALT_SEM      (write_lock)
  ALT_FLAG_GRP (events)
  
  /* The variables below are volatile because they are modified by the
   * interrupt routine.  Making them volatile and reading them atomically
   * means that we don't need any large critical sections.
   */
  volatile unsigned int rx_in;
  unsigned int  rx_out;
  unsigned int  tx_in;
  volatile unsigned int tx_out;
  char          rx_buf[ALTERA_AVALON_JTAG_UART_BUF_LEN];
  char          tx_buf[ALTERA_AVALON_JTAG_UART_BUF_LEN];

#endif /* !ALTERA_AVALON_JTAG_UART_SMALL */

} altera_avalon_jtag_uart_state;

/*
 * Macros used by alt_sys_init when the ALT file descriptor facility isn't used.
 */

#ifdef ALTERA_AVALON_JTAG_UART_SMALL

#define ALTERA_AVALON_JTAG_UART_STATE_INSTANCE(name, state)    \
  altera_avalon_jtag_uart_state state =                  \
  {                                                      \
    name##_BASE,                                         \
  }

#define ALTERA_AVALON_JTAG_UART_STATE_INIT(name, state)

#else /* !ALTERA_AVALON_JTAG_UART_SMALL */

#define ALTERA_AVALON_JTAG_UART_STATE_INSTANCE(name, state)   \
  altera_avalon_jtag_uart_state state =                  \
  {                                                      \
    name##_BASE,                                         \
    ALTERA_AVALON_JTAG_UART_DEFAULT_TIMEOUT,             \
  }

/*
 * Externally referenced routines
 */
extern void altera_avalon_jtag_uart_init(altera_avalon_jtag_uart_state* sp, 
                                        int irq_controller_id, int irq);

#define ALTERA_AVALON_JTAG_UART_STATE_INIT(name, state)                      \
  {                                                                          \
    if (name##_IRQ == ALT_IRQ_NOT_CONNECTED)                                 \
    {                                                                        \
      ALT_LINK_ERROR ("Error: Interrupt not connected for " #name ". "       \
                      "You have selected the interrupt driven version of "   \
                      "the ALTERA Avalon JTAG UART driver, but the "         \
                      "interrupt is not connected for this device. You can " \
                      "select a polled mode driver by checking the 'small "  \
                      "driver' option in the HAL configuration window, or "  \
                      "by using the -DALTERA_AVALON_JTAG_UART_SMALL "        \
                      "preprocessor flag.");                                 \
    }                                                                        \
    else                                                                     \
      altera_avalon_jtag_uart_init(&state,                                   \
                                   name##_IRQ_INTERRUPT_CONTROLLER_ID,       \
                                   name##_IRQ);                              \
  }

#endif /* ALTERA_AVALON_JTAG_UART_SMALL */

/*
 * Include in case non-direct version of driver required.
 */
#include "altera_avalon_jtag_uart_fd.h"

/*
 * Map alt_sys_init macros to direct or non-direct versions.
 */
#ifdef ALT_USE_DIRECT_DRIVERS

#define ALTERA_AVALON_JTAG_UART_INSTANCE(name, state) \
   ALTERA_AVALON_JTAG_UART_STATE_INSTANCE(name, state)
#define ALTERA_AVALON_JTAG_UART_INIT(name, state) \
   ALTERA_AVALON_JTAG_UART_STATE_INIT(name, state)

#else /* !ALT_USE_DIRECT_DRIVERS */

#define ALTERA_AVALON_JTAG_UART_INSTANCE(name, dev) \
   ALTERA_AVALON_JTAG_UART_DEV_INSTANCE(name, dev)
#define ALTERA_AVALON_JTAG_UART_INIT(name, dev) \
   ALTERA_AVALON_JTAG_UART_DEV_INIT(name, dev)

#endif /* ALT_USE_DIRECT_DRIVERS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_AVALON_JTAG_UART_H__ */
