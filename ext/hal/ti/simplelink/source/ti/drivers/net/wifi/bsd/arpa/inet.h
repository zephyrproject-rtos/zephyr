/*
 *   Copyright (C) 2016 Texas Instruments Incorporated
 *
 *   All rights reserved. Property of Texas Instruments Incorporated.
 *   Restricted rights to use, duplicate or disclose this code are
 *   granted through contract.
 *
 *   The program may not be used without the written permission of
 *   Texas Instruments Incorporated or against the terms and conditions
 *   stipulated in the agreement under which this program has been supplied,
 *   and under no circumstances can it be used with non-TI connectivity device.
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
