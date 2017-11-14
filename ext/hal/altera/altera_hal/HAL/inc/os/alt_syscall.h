#ifndef __ALT_SYSCALL_H__
#define __ALT_SYSCALL_H__

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
******************************************************************************/

/*
 * The macros defined in this file are used to provide the function names used
 * for the HAL 'UNIX style' interface, e.g. read(), write() etc. 
 *
 * Operating systems which are ported to the HAL can provide their own 
 * version of this file, which will be used in preference. This allows
 * the operating system to provide it's own implementation of the top level
 * system calls, while retaining the HAL functions under a different name, 
 * for example, alt_read(), alt_write() etc.
 */

#define ALT_CLOSE        close
#define ALT_ENVIRON      environ
#define ALT_EXECVE       execve
#define ALT_EXIT         _exit
#define ALT_FCNTL        fcntl
#define ALT_FORK         fork
#define ALT_FSTAT        fstat
#define ALT_GETPID       getpid
#define ALT_GETTIMEOFDAY gettimeofday
#define ALT_IOCTL        ioctl
#define ALT_ISATTY       isatty
#define ALT_KILL         kill
#define ALT_LINK         link
#define ALT_LSEEK        lseek
#define ALT_OPEN         open
#define ALT_READ         read
#define ALT_RENAME       _rename
#define ALT_SBRK         sbrk
#define ALT_SETTIMEOFDAY settimeofday
#define ALT_STAT         stat
#define ALT_UNLINK       unlink
#define ALT_USLEEP       usleep
#define ALT_WAIT         wait
#define ALT_WRITE        write
#define ALT_TIMES        times

/*
 *
 */

#endif /* __ALT_SYSCALL_H__ */
