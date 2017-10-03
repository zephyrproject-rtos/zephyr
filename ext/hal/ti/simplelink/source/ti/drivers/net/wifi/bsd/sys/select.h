/*
 * select.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef __SELECT_H__
#define __SELECT_H__

#ifdef    __cplusplus
extern "C" {
#endif

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/driver.h>

/*!

    \addtogroup BSD_Socket
    @{

*/

/* Structures */
#define timeval                             SlTimeval_t
#undef fd_set
#define fd_set                              SlFdSet_t

/* FD_ functions */
#undef  FD_SETSIZE
#define FD_SETSIZE                          SL_FD_SETSIZE
#undef FD_SET
#define FD_SET                              SL_SOCKET_FD_SET
#undef FD_CLR
#define FD_CLR                              SL_SOCKET_FD_CLR
#undef FD_ISSET
#define FD_ISSET                            SL_SOCKET_FD_ISSET
#undef FD_ZERO
#define FD_ZERO                             SL_SOCKET_FD_ZERO

/*!
    \brief Monitor socket activity

    Select allow a program to monitor multiple file descriptors,
    waiting until one or more of the file descriptors become
    "ready" for some class of I/O operation.

    \param[in]  nfds        The highest-numbered file descriptor in any of the
                            three sets, plus 1.
    \param[out] readsds     Socket descriptors list for read monitoring and accept monitoring
    \param[out] writesds    Socket descriptors list for connect monitoring only, write monitoring is not supported
    \param[out] exceptsds   Socket descriptors list for exception monitoring, not supported.
    \param[in]  timeout     Is an upper bound on the amount of time elapsed
                            before select() returns. Null or above 0xffff seconds means
                            infinity timeout. The minimum timeout is 10 milliseconds,
                            less than 10 milliseconds will be set automatically to 10 milliseconds.
                            Max microseconds supported is 0xfffc00.
                            In trigger mode the timout fields must be set to zero.

    \return                 On success, select()  returns the number of
                            file descriptors contained in the three returned
                            descriptor sets (that is, the total number of bits that
                            are set in readfds, writefds, exceptfds) which may be
                            zero if the timeout expires before anything interesting
                            happens.\n On error, a negative value is returned.
                            readsds - return the sockets on which read request will
                            return without delay with valid data.\n
                            writesds - return the sockets on which write request
                            will return without delay.\n
                            exceptsds - return the sockets closed recently. (not supported)\n
                            ENOMEM may be return in case there are no resources in the system
                            In this case try again later or increase MAX_CONCURRENT_ACTIONS
    \sa     sl_Socket
    \note   If the timeout value set to less than 10ms it will automatically set 
            to 10ms to prevent overload of the system\n
            Belongs to \ref basic_api
            
            Several threads can call select at the same time.\b
            Calling this API while the same command is called from another thread, may result
                in one of the following scenarios:
            1. The command will be executed alongside other select callers (success).
            2. The command will wait (internal) until the previous select finish, and then be executed.
            3. There are not enough resources and SL_POOL_IS_EMPTY error will return. 
            In this case, ENOMEM can be increased (result in memory increase) or try
            again later to issue the command.

            In case all the user sockets are open, select will exhibit the behaviour mentioned in (2)
            This is due to the fact select supports multiple callers by utilizing one user socket internally.
            User who wish to ensure multiple select calls at any given time, must reserve one socket out of the 16 given.
   
    \warning

    \par  Example

    - Monitoring two sockets:

    \code
    int socketFD1;
    int socketFD2;
    fd_set readfds;
    struct timeval tv;

    // go and connect to both  servers
    socketFD1 = socket(...);
    socketFD2 = socket(...);
    connect(socketFD1, ...)...
    connect(socketFD2, ...)...

    // clear the set ahead of time
    FD_ZERO(&readfds);

    // add our descriptors to the set
    FD_SET(socketFD1, &readfds);
    FD_SET(socketFD2, &readfds);

    // since we opened socketFD2 second, it's the "greater", so we use that for
    // the nfds param in select();
    n = socketFD2 + 1;

    // wait until either socket has data ready to be recv() (we set timeout to 10.5 Sec)
    tv.tv_sec = 10;
    tv.tv_usec = 500000;
    retVal = select(n, &readfds, NULL, NULL, &tv);

    if (retVal == -1)
    {
        printf("error: %d in select().\n", errno);
    }
    else if (retVal == 0)
    {
        printf("Timeout occurred!  No data after 10.5 seconds.\n");
    }
    else
    {
        // one or both of the descriptors have data: go ahead and read it.
        if (FD_ISSET(socketFD1, &readfds))
        {
            recv(socketFD1, buf1, sizeof buf1, 0);
        }
        if (FD_ISSET(socketFD2, &readfds))
        {
            recv(socketFD2, buf2, sizeof buf2, 0);
        }
    }

    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_Select)
static inline int select(int nfds, fd_set *readsds, fd_set *writesds, fd_set *exceptsds, struct timeval *timeout)
{
    int RetVal = (int)sl_Select(nfds, readsds, writesds, exceptsds, timeout);
    return _SlDrvSetErrno(RetVal);
}
#endif

/*!

 Close the Doxygen group.
 @}

 */

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __SELECT_H__ */
