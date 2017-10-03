/*
 * errno.h - CC31xx/CC32xx Host Driver Implementation
 *
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/



/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#ifndef ERRNO_H_
#define ERRNO_H_

#ifdef    __cplusplus
extern "C" {
#endif

#include <ti/drivers/net/wifi/simplelink.h>

/* BSD API error codes */
#ifndef ERROR
#define ERROR                               SL_ERROR_BSD_SOC_ERROR
#endif
#ifndef INEXE
#define INEXE                               SL_ERROR_BSD_INEXE
#endif
#ifndef EBADF
#define EBADF                               SL_ERROR_BSD_EBADF
#endif
#ifndef EBADE
#define EBADE                               (52L)   /* Invalid exchange */
#endif
#ifndef ENSOCK
#define ENSOCK                              SL_ERROR_BSD_ENSOCK
#endif
#ifndef EAGAIN
#define EAGAIN                              SL_ERROR_BSD_EAGAIN
#ifdef  EWOULDBLOCK
#undef  EWOULDBLOCK
#endif
#define EWOULDBLOCK                         EAGAIN
#else
#ifdef  EWOULDBLOCK
#undef  EWOULDBLOCK
#endif
#define EWOULDBLOCK                         EAGAIN
#endif
#ifndef ENOMEM
#define ENOMEM                              SL_ERROR_BSD_ENOMEM
#endif
#ifndef EACCES
#define EACCES                              SL_ERROR_BSD_EACCES
#endif
#ifndef EFAULT
#define EFAULT                              SL_ERROR_BSD_EFAULT
#endif
#ifndef EINVAL
#define EINVAL                              SL_ERROR_BSD_EINVAL
#endif
#ifndef EDESTADDRREQ
#define EDESTADDRREQ                        SL_ERROR_BSD_EDESTADDRREQ
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE                          SL_ERROR_BSD_EPROTOTYPE
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT                         SL_ERROR_BSD_ENOPROTOOPT
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT                     SL_ERROR_BSD_EPROTONOSUPPORT
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT                     SL_ERROR_BSD_ESOCKTNOSUPPORT
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP                          SL_ERROR_BSD_EOPNOTSUPP
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT                        SL_ERROR_BSD_EAFNOSUPPORT
#endif
#ifndef EADDRINUSE
#define EADDRINUSE                          SL_ERROR_BSD_EADDRINUSE
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL                       SL_ERROR_BSD_EADDRNOTAVAIL
#endif
#ifndef ENETUNREACH
#define ENETUNREACH                         SL_ERROR_BSD_ENETUNREACH
#endif
#ifndef ENOBUFS
#define ENOBUFS                              SL_ERROR_BSD_ENOBUFS
#ifdef  EOBUFF
#undef  EOBUFF
#endif
#define EOBUFF                               ENOBUFS
#else
#ifdef  EOBUFF
#undef  EOBUFF
#endif
#define EOBUFF                               ENOBUFS
#endif
#ifndef EISCONN
#define EISCONN                             SL_ERROR_BSD_EISCONN
#endif
#ifndef ENOTCONN
#define ENOTCONN                            SL_ERROR_BSD_ENOTCONN
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT                           SL_ERROR_BSD_ETIMEDOUT
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED                        SL_ERROR_BSD_ECONNREFUSED
#endif


#ifdef SL_INC_INTERNAL_ERRNO
int *  __errno(void);
#else
extern int slcb_SetErrno(int Errno);
#endif


#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* ERRNO_H_ */
