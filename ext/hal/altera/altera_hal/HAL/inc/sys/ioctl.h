#ifndef __IOCTL_H__
#define __IOCTL_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
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

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * The ioctl() system call be used to initiate a variety of control operations
 * on a file descriptor. For the most part this simply translates to a call to
 * the ioctl() function of the associated device driver (TIOCEXCL and
 * TIOCNXCL are notable exceptions - see ioctl.c for details).
 *
 * The interpretation of the ioctl requests are therefore device specific.
 *
 * This function is equivalent to the standard Posix ioctl() call.
 */

extern int ioctl (int fd, int req, void* arg);

/*
 * list of ioctl calls handled by the system ioctl implementation.
 */

#define TIOCEXCL 0x740d /* exclusive use of the device */
#define TIOCNXCL 0x740e /* allow multiple use of the device */

/*
 * ioctl calls which can be handled by device drivers.
 */

#define TIOCOUTQ 0x7472 /* get output queue size */
#define TIOCMGET 0x741d /* get termios flags */
#define TIOCMSET 0x741a /* set termios flags */

/*
 * ioctl calls specific to JTAG UART.
 */

#define TIOCSTIMEOUT 0x6a01 /* Set Timeout before assuming no host present */
#define TIOCGCONNECTED 0x6a02 /* Get indication of whether host is connected */

/*
 *
 */

#ifdef __cplusplus
}
#endif
  
#endif /* __IOCTL_H__ */
