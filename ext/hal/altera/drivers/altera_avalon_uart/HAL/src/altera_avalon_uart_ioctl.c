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
#include <string.h>

#include "sys/alt_irq.h"
#include "sys/ioctl.h"
#include "sys/alt_errno.h"

#include "altera_avalon_uart_regs.h"
#include "altera_avalon_uart.h"


#if !defined(ALT_USE_SMALL_DRIVERS) && !defined(ALTERA_AVALON_UART_SMALL)

/* ----------------------------------------------------------- */
/* ------------------------- FAST DRIVER --------------------- */
/* ----------------------------------------------------------- */

/*
 * To reduce the code footprint of this driver, the ioctl() function is not
 * included by default. If you wish to use the ioctl features provided 
 * below, you can do so by adding the option : -DALTERA_AVALON_UART_USE_IOCTL
 * to CPPFLAGS in the Makefile (or through the Eclipse IDE).
 */

#ifdef ALTERA_AVALON_UART_USE_IOCTL

/*
 * altera_avalon_uart_ioctl() is called by the system ioctl() function to handle
 * ioctl requests for the UART. The only ioctl requests supported are TIOCMGET
 * and TIOCMSET.
 *
 * TIOCMGET returns a termios structure that describes the current device
 * configuration.
 *
 * TIOCMSET sets the device (if possible) to match the requested configuration.
 * The requested configuration is described using a termios structure passed
 * through the input argument "arg".
 */

static int altera_avalon_uart_tiocmget(altera_avalon_uart_state* sp,
  struct termios* term);
static int altera_avalon_uart_tiocmset(altera_avalon_uart_state* sp,
  struct termios* term);

int 
altera_avalon_uart_ioctl(altera_avalon_uart_state* sp, int req, void* arg)
{
  int rc = -ENOTTY;

  switch (req)
  {
  case TIOCMGET:
    rc = altera_avalon_uart_tiocmget(sp, (struct termios*) arg);
    break;
  case TIOCMSET:
    rc = altera_avalon_uart_tiocmset(sp, (struct termios*) arg);
    break;
  default:
    break;
  }
  return rc;
}

/*
 * altera_avalon_uart_tiocmget() is used by altera_avalon_uart_ioctl() to fill
 * in the input termios structure with the current device configuration. 
 *
 * See termios.h for further details on the contents of the termios structure.
 */

static int 
altera_avalon_uart_tiocmget(altera_avalon_uart_state* sp,
  struct termios* term)
{
  memcpy (term, &sp->termios, sizeof (struct termios));
  return 0;
}

/*
 * altera_avalon_uart_tiocmset() is used by altera_avalon_uart_ioctl() to 
 * configure the device according to the settings in the input termios 
 * structure. In practice the only configuration that can be changed is the
 * baud rate, and then only if the hardware is configured to have a writable
 * baud register.
 */

static int 
altera_avalon_uart_tiocmset(altera_avalon_uart_state* sp,
  struct termios* term)
{
  speed_t speed;

  speed = sp->termios.c_ispeed;

  /* Update the settings if the hardware supports it */

  if (!(sp->flags & ALT_AVALON_UART_FB))
  {
    sp->termios.c_ispeed = sp->termios.c_ospeed = term->c_ispeed;
  }
  /* 
   * If the request was for an unsupported setting, return an error.
   */

  if (memcmp(term, &sp->termios, sizeof (struct termios)))
  {
    sp->termios.c_ispeed = sp->termios.c_ospeed = speed;
    return -EIO;
  }

  /*
   * Otherwise, update the hardware.
   */
  
  IOWR_ALTERA_AVALON_UART_DIVISOR(sp->base, 
    ((sp->freq/sp->termios.c_ispeed) - 1));

  return 0;
}

#endif /* ALTERA_AVALON_UART_USE_IOCTL */

#endif /* fast driver */
