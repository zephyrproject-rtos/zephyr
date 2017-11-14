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

#include "alt_types.h"
#include "sys/alt_dev.h"
#include "altera_avalon_jtag_uart.h"

extern int altera_avalon_jtag_uart_read(altera_avalon_jtag_uart_state* sp,
  char* buffer, int space, int flags);
extern int altera_avalon_jtag_uart_write(altera_avalon_jtag_uart_state* sp,
  const char* ptr, int count, int flags);
extern int altera_avalon_jtag_uart_ioctl(altera_avalon_jtag_uart_state* sp,
  int req, void* arg);
extern int altera_avalon_jtag_uart_close(altera_avalon_jtag_uart_state* sp, 
  int flags);

/* ----------------------------------------------------------------------- */
/* --------------------- WRAPPERS FOR ALT FD SUPPORT --------------------- */
/*
 *
 */

int 
altera_avalon_jtag_uart_read_fd(alt_fd* fd, char* buffer, int space)
{
    altera_avalon_jtag_uart_dev* dev = (altera_avalon_jtag_uart_dev*) fd->dev; 

    return altera_avalon_jtag_uart_read(&dev->state, buffer, space,
      fd->fd_flags);
}

int 
altera_avalon_jtag_uart_write_fd(alt_fd* fd, const char* buffer, int space)
{
    altera_avalon_jtag_uart_dev* dev = (altera_avalon_jtag_uart_dev*) fd->dev; 

    return altera_avalon_jtag_uart_write(&dev->state, buffer, space,
      fd->fd_flags);
}

#ifndef ALTERA_AVALON_JTAG_UART_SMALL

int 
altera_avalon_jtag_uart_close_fd(alt_fd* fd)
{
    altera_avalon_jtag_uart_dev* dev = (altera_avalon_jtag_uart_dev*) fd->dev; 

    return altera_avalon_jtag_uart_close(&dev->state, fd->fd_flags);
}

int 
altera_avalon_jtag_uart_ioctl_fd(alt_fd* fd, int req, void* arg)
{
    altera_avalon_jtag_uart_dev* dev = (altera_avalon_jtag_uart_dev*) fd->dev;

    return altera_avalon_jtag_uart_ioctl(&dev->state, req, arg);
}

#endif /* ALTERA_AVALON_JTAG_UART_SMALL */
