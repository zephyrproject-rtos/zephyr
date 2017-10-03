/*
 * inet.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef __INET_H__
#define __INET_H__

#ifdef    __cplusplus
extern "C" {
#endif

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/bsd/netinet/in.h>
#include <ti/drivers/net/wifi/bsd/sys/socket.h>

/*!

    \addtogroup BSD_Socket
    @{
*/

/*!
    \brief Reorder the bytes of a 32-bit unsigned value

    This function is used to Reorder the bytes of a 32-bit unsigned value from processor order to network order.

    \param [in] val             Variable to reorder

    \return                     Return the reordered variable.

    \sa     sendto  bind  connect  recvfrom  accept
*/
#define htonl                sl_Htonl

/*!
    \brief Reorder the bytes of a 32-bit unsigned value

    This function is used to Reorder the bytes of a 32-bit unsigned value from network order to processor order.

    \param[in] val              Variable to reorder

    \return                     Return the reordered variable.

    \sa     sendto  bind  connect  recvfrom  accept
*/
#define ntohl                sl_Ntohl

/*!
    \brief Reorder the bytes of a 16-bit unsigned value

    This function is used to Reorder the bytes of a 16-bit unsigned value from processor order to network order.

    \param[in] val              Variable to reorder

    \return                     Return the reordered variable.

    \sa     sendto  bind  connect  recvfrom  accept
*/
#define htons                sl_Htons

/*!
    \brief Reorder the bytes of a 16-bit unsigned value

    This function is used to Reorder the bytes of a 16-bit unsigned value from network order to processor order.

    \param[in] val              Variable to reorder

    \return                     Return the reordered variable.

    \sa     sendto  bind  connect  recvfrom  accept
*/
#define ntohs                sl_Ntohs

/*!

 Close the Doxygen group.
 @}

 */

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __INET_H__ */

