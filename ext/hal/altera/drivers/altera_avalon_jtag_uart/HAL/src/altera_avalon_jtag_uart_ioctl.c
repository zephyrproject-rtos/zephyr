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

#include "sys/ioctl.h"
#include "alt_types.h"

#include "altera_avalon_jtag_uart_regs.h"
#include "altera_avalon_jtag_uart.h"

#include "sys/alt_log_printf.h"

#ifndef ALTERA_AVALON_JTAG_UART_SMALL

/* ----------------------------------------------------------- */
/* ------------------------- FAST DRIVER --------------------- */
/* ----------------------------------------------------------- */

int 
altera_avalon_jtag_uart_ioctl(altera_avalon_jtag_uart_state* sp, int req,
  void* arg)
{
  int rc = -ENOTTY;

  switch (req)
  {
  case TIOCSTIMEOUT:
    /* Set the time to wait until assuming host is not connected */
    if (sp->timeout != INT_MAX)
    {
      int timeout = *((int *)arg);
      sp->timeout = (timeout >= 2 && timeout < INT_MAX) ? timeout : INT_MAX - 1;
      rc = 0;
    }
    break;

  case TIOCGCONNECTED:
    /* Find out whether host is connected */
    if (sp->timeout != INT_MAX)
    {
      *((int *)arg) = (sp->host_inactive < sp->timeout) ? 1 : 0;
      rc = 0;
    }
    break;

  default:
    break;
  }

  return rc;
}

#endif /* !ALTERA_AVALON_JTAG_UART_SMALL */
