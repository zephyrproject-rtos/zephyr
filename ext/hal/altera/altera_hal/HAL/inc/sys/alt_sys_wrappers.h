#ifndef __ALT_SYS_WRAPPERS_H__
#define __ALT_SYS_WRAPPERS_H__

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
 * This file provides the prototypes for the HAL 'UNIX style functions. The
 * names of these functions are defined in alt_syscall.h. THese are defined to
 * be the standard names when running the standalone HAL, e.g. open(), close()
 * etc., but the names may be redefined as a part of an operating system port
 * in order to avoid name clashes.
 */

#include "os/alt_syscall.h"

#include <unistd.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>

extern int     ALT_CLOSE (int __fd);
extern int     ALT_EXECVE (const char *__path, 
                           char * const __argv[], 
                           char * const __envp[]);
extern void    ALT_EXIT (int __status);
extern int     ALT_FSTAT (int file, struct stat *st);
extern int     ALT_FCNTL (int file, int cmd, ...);
extern pid_t   ALT_FORK (void);
extern pid_t   ALT_GETPID (void);

#if defined (__GNUC__) && __GNUC__ >= 4
extern int     ALT_GETTIMEOFDAY (struct timeval  *ptimeval, 
                                 void *ptimezone);
#else
extern int     ALT_GETTIMEOFDAY (struct timeval  *ptimeval, 
                                 struct timezone *ptimezone);
#endif

extern int     ALT_IOCTL (int file, int req, void* arg);
extern int     ALT_ISATTY (int file);
extern int     ALT_KILL (int pid, int sig);
extern int     ALT_LINK (const char *existing, const char *new);
extern off_t   ALT_LSEEK (int file, off_t ptr, int dir);
extern int     ALT_OPEN (const char* file, int flags, ...);
extern int     ALT_READ (int file, void *ptr, size_t len);
extern int     ALT_RENAME (char *existing, char *new);
extern void*   ALT_SBRK (ptrdiff_t incr);
extern int     ALT_SETTIMEOFDAY (const struct timeval  *t,
                                 const struct timezone *tz);
extern int     ALT_STAT (const char *file, struct stat *st);
extern clock_t ALT_TIMES (struct tms *buf);
extern int     ALT_UNLINK (const char *name);

#if defined (__GNUC__) && __GNUC__ >= 4
int ALT_USLEEP (useconds_t us);
#else
unsigned int   ALT_USLEEP (unsigned int us);
#endif

extern int     ALT_WAIT (int *status);
extern int     ALT_WRITE (int file, const void *ptr, size_t len);


extern char** ALT_ENVIRON;

/*
 *
 */

#endif /* __ALT_SYS_WRAPPERS_H__ */
