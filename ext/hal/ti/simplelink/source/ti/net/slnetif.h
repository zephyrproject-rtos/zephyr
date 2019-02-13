/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

#ifndef __SL_NET_IF_H__
#define __SL_NET_IF_H__

#include <stdint.h>
#include <stdbool.h>

#include <ti/net/slnetsock.h>

#ifdef    __cplusplus
extern "C" {
#endif

/*!
    \defgroup SlNetIf SlNetIf group

    \short Controls standard stack/interface options and capabilities

*/
/*!
    \addtogroup SlNetIf
    @{
*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

/* Interface ID bit pool to be used in interface add and in socket creation  */
#define SLNETIF_ID_1                (1 << 0)  /**< Interface ID 1 */
#define SLNETIF_ID_2                (1 << 1)  /**< Interface ID 2 */
#define SLNETIF_ID_3                (1 << 2)  /**< Interface ID 3 */
#define SLNETIF_ID_4                (1 << 3)  /**< Interface ID 4 */
#define SLNETIF_ID_5                (1 << 4)  /**< Interface ID 5 */
#define SLNETIF_ID_6                (1 << 5)  /**< Interface ID 6 */
#define SLNETIF_ID_7                (1 << 6)  /**< Interface ID 7 */
#define SLNETIF_ID_8                (1 << 7)  /**< Interface ID 8 */
#define SLNETIF_ID_9                (1 << 8)  /**< Interface ID 9 */
#define SLNETIF_ID_10               (1 << 9)  /**< Interface ID 10 */
#define SLNETIF_ID_11               (1 << 10) /**< Interface ID 11 */
#define SLNETIF_ID_12               (1 << 11) /**< Interface ID 12 */
#define SLNETIF_ID_13               (1 << 12) /**< Interface ID 13 */
#define SLNETIF_ID_14               (1 << 13) /**< Interface ID 14 */
#define SLNETIF_ID_15               (1 << 14) /**< Interface ID 15 */
#define SLNETIF_ID_16               (1 << 15) /**< Interface ID 16 */

#define SLNETIF_MAX_IF              (16)      /**< Maximum interface */

/* this macro returns 0 when only one bit is set and a number when it isn't */
#define ONLY_ONE_BIT_IS_SET(x)      (((x > 0) && ((x & (x - 1)) == 0))?true:false)


/* Interface connection status bit pool to be used in set interface connection status function */

#define SLNETIF_STATUS_DISCONNECTED (0)
#define SLNETIF_STATUS_CONNECTED    (1)

/*!
    \brief Interface state bit pool to be used in set interface state function
*/
typedef enum
{
    SLNETIF_STATE_DISABLE = 0,
    SLNETIF_STATE_ENABLE  = 1
} SlNetIfState_e;

/*!
    \brief Address type enum to be used in get ip address function
*/
typedef enum
{
    SLNETIF_IPV4_ADDR        = 0,
    SLNETIF_IPV6_ADDR_LOCAL  = 1,
    SLNETIF_IPV6_ADDR_GLOBAL = 2
} SlNetIfAddressType_e;

/* Address config return values that can be retrieved in get ip address function */
#define SLNETIF_ADDR_CFG_UNKNOWN   (0)
#define SLNETIF_ADDR_CFG_DHCP      (1)
#define SLNETIF_ADDR_CFG_DHCP_LLA  (2)
#define SLNETIF_ADDR_CFG_STATIC    (4)
#define SLNETIF_ADDR_CFG_STATELESS (5)
#define SLNETIF_ADDR_CFG_STATEFUL  (6)

/* Security object types for load Sec Obj function */
#define SLNETIF_SEC_OBJ_TYPE_RSA_PRIVATE_KEY (1)
#define SLNETIF_SEC_OBJ_TYPE_CERTIFICATE     (2)
#define SLNETIF_SEC_OBJ_TYPE_DH_KEY          (3)


/*!
    Check if interface state is enabled.

    \sa SlNetIf_queryIf()
 */
#define SLNETIF_QUERY_IF_STATE_BIT               (1 << 0)
/*!
    Check if interface connection status is connected.

    \sa SlNetIf_queryIf()
 */
#define SLNETIF_QUERY_IF_CONNECTION_STATUS_BIT   (1 << 1)
/*!
    Enable last partial match in an interface search, if no existing
    interface matches the query completely.

    \sa SlNetIf_queryIf()
 */
#define SLNETIF_QUERY_IF_ALLOW_PARTIAL_MATCH_BIT (1 << 2)

/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

/*!
    \brief SlNetIf_Config_t structure contains all the function callbacks that are expected to be filled by the relevant network stack interface \n
           Each interface has different capabilities, so not all the API's must be supported therefore an API's can be defined as:
           - <b>Mandatory API's</b>     - must be supported by the interface in order to be part of SlNetSock layer
           - <b>Non-Mandatory API's</b> - can be supported, but not mandatory for basic SlNetSock proper operation

    \note  Interface that is not supporting a non-mandatory API should set it to \b NULL in its function list

    \sa SlNetIf_Config_t
*/
typedef struct SlNetIf_Config_t
{
    /* socket related API's */
    int16_t (*sockCreate)        (void *ifContext, int16_t domain, int16_t type, int16_t protocol, void **sdContext);                /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetSock_create           */
    int32_t (*sockClose)         (int16_t sd, void *sdContext);                                                                      /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetSock_close            */
    int32_t (*sockShutdown)      (int16_t sd, void *sdContext, int16_t how);                                                         /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_shutdown     */
    int16_t (*sockAccept)        (int16_t sd, void *sdContext, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen, uint8_t flags, void **acceptedSdContext);        /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_accept   */
    int32_t (*sockBind)          (int16_t sd, void *sdContext, const SlNetSock_Addr_t *addr, int16_t addrlen);                       /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_bind         */
    int32_t (*sockListen)        (int16_t sd, void *sdContext, int16_t backlog);                                                     /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_listen       */
    int32_t (*sockConnect)       (int16_t sd, void *sdContext, const SlNetSock_Addr_t *addr, SlNetSocklen_t addrlen, uint8_t flags); /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_connect      */
    int32_t (*sockGetPeerName)   (int16_t sd, void *sdContext, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen);                     /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_getPeerName  */
    int32_t (*sockGetLocalName)  (int16_t sd, void *sdContext, SlNetSock_Addr_t *addr, SlNetSocklen_t *addrlen);                     /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_getSockName  */
    int32_t (*sockSelect)        (void *ifContext, int16_t nsds, SlNetSock_SdSet_t *readsds, SlNetSock_SdSet_t *writesds, SlNetSock_SdSet_t *exceptsds, SlNetSock_Timeval_t *timeout); /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetSock_select  */
    int32_t (*sockSetOpt)        (int16_t sd, void *sdContext, int16_t level, int16_t optname, void *optval, SlNetSocklen_t optlen);                             /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetSock_setOpt       */
    int32_t (*sockGetOpt)        (int16_t sd, void *sdContext, int16_t level, int16_t optname, void *optval, SlNetSocklen_t *optlen);                            /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetSock_getOpt       */
    int32_t (*sockRecv)          (int16_t sd, void *sdContext, void *buf, uint32_t len, uint32_t flags);                                                         /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_recv     */
    int32_t (*sockRecvFrom)      (int16_t sd, void *sdContext, void *buf, uint32_t len, uint32_t flags, SlNetSock_Addr_t *from, SlNetSocklen_t *fromlen);        /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetSock_recvFrom     */
    int32_t (*sockSend)          (int16_t sd, void *sdContext, const void *buf, uint32_t len, uint32_t flags);                                                   /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_send     */
    int32_t (*sockSendTo)        (int16_t sd, void *sdContext, const void *buf, uint32_t len, uint32_t flags, const SlNetSock_Addr_t *to, SlNetSocklen_t tolen); /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetSock_sendTo       */
    int32_t (*sockstartSec)      (int16_t sd, void *sdContext, SlNetSockSecAttrib_t *secAttrib, uint8_t flags);                                                  /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetSock_startSec */

    /* util related API's */
    int32_t (*utilGetHostByName) (void *ifContext, char *name, const uint16_t nameLen, uint32_t *ipAddr, uint16_t *ipAddrLen, const uint8_t family);  /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetUtil_getHostByName */

    /* if related API's */
    int32_t (*ifGetIPAddr)           (void *ifContext, SlNetIfAddressType_e addrType, uint16_t *addrConfig, uint32_t *ipAddr);                      /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetIf_getIPAddr           */
    int32_t (*ifGetConnectionStatus) (void *ifContext);                                                                                             /*!< \b Mandatory API \n The actual implementation of the interface for ::SlNetIf_getConnectionStatus */
    int32_t (*ifLoadSecObj)          (void *ifContext, uint16_t objType, char *objName, int16_t objNameLen, uint8_t *objBuff, int16_t objBuffLen);  /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetIf_loadSecObj      */
    int32_t (*ifCreateContext)       (uint16_t ifID, const char *ifName, void **ifContext);                                                         /*!< \b Non-Mandatory API \n The actual implementation of the interface for ::SlNetIf_add             */

} SlNetIf_Config_t;


/*!
    \brief The SlNetIf_t structure holds the configuration of the interface
           Its ID, name, flags and the configuration list - ::SlNetIf_Config_t.
*/
typedef struct SlNetIf_t
{
    uint32_t          ifID;
    char             *ifName;
    int32_t           flags;
    SlNetIf_Config_t *ifConf;
    void             *ifContext;
} SlNetIf_t;

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

/*!

    \brief Initialize the SlNetIf module

    \param[in] flags            For future usage,
                                The value 0 may be used in order to run the
                                default flags

    \return                     Zero on success, or negative error code on failure

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_init snippet
*/
int32_t SlNetIf_init(int32_t flags);

/*!
    \brief Add a new SlNetIf-compatible interface to the system

    The SlNetIf_add function allows the application to add specific interfaces
    with their priorities and function list.\n
    This function gives full control to the application on the interfaces.

    \param[in] ifID            Specifies the interface which needs
                               to be added.\n
                               The values of the interface identifier
                               is defined with the prefix SLNETIF_ID_
                               which defined in slnetif.h
    \param[in] ifName          Specifies the name of the interface,
                               \b Note: Can be set to NULL, but when set to NULL
                                     cannot be used with SlNetIf_getIDByName
    \param[in] ifConf          Specifies the function list for the
                               interface
    \param[in] priority        Specifies the priority needs to be
                               set (In ascending order).
                               Note: maximum priority is 15

    \return                    Zero on success, or negative error code on failure

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_add wifi snippet
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_add NDK snippet
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_add NDKSec snippet
*/
int32_t SlNetIf_add(uint16_t ifID, char *ifName, const SlNetIf_Config_t *ifConf, uint8_t priority);


/*!
    \brief Get interface configuration from interface ID

    The SlNetIf_getIfByID function retrieves the configuration of the
    requested interface.

    \param[in] ifID      Specifies the interface which its configuration
                         needs to be retrieved.\n
                         The values of the interface identifier is
                         defined with the prefix SLNETIF_ID_ which
                         defined in slnetif.h

    \return              A pointer to the configuration of the
                         interface on success, or NULL on failure

    \sa     SlNetIf_add()

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_getIfByID snippet
*/
SlNetIf_t * SlNetIf_getIfByID(uint16_t ifID);


/*!
    \brief Query for the highest priority interface, given a list of
    interfaces and properties.

    \param[in]  ifBitmap     The bit-wise OR of interfaces to be searched.\n
                             Note: Zero is currently not valid.
    \param[in]  queryBitmap  The bit-wise OR of additional query criteria.

    \remarks    @c queryBitmap can be set to :
                    - #SLNETIF_QUERY_IF_STATE_BIT
                    - #SLNETIF_QUERY_IF_CONNECTION_STATUS_BIT
                    - #SLNETIF_QUERY_IF_ALLOW_PARTIAL_MATCH_BIT

    \return     A pointer to the configuration of a found
                interface on success, or NULL on failure

    \sa     SlNetIf_add()
    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_queryIf snippet
*/
SlNetIf_t * SlNetIf_queryIf(uint32_t ifBitmap, uint32_t queryBitmap);


/*!
    \brief Get interface Name from interface ID

    The SlNetIf_getNameByID function retrieves the name of the requested
    interface.

    \param[in] ifID      Specifies the interface which its name needs
                         to be retrieved.\n
                         The values of the interface identifier is
                         defined with the prefix SLNETIF_ID_ which
                         defined in slnetif.h

    \return              A pointer to the name of the interface on
                         success, or NULL on failure

    \sa     SlNetIf_add()
    \sa     SlNetIf_getIDByName()

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_getNameByID snippet
*/
const char * SlNetIf_getNameByID(uint16_t ifID);


/*!
    \brief Get interface ID from interface name

    The SlNetIf_getIDByName function retrieves the interface identifier of the
    requested interface name.

    \param[in] ifName    Specifies the interface which its interface
                         identifier needs to be retrieved.\n

    \return              The interface identifier value of the interface
                         on success, or negative error code on failure
                         The values of the interface identifier is
                         defined with the prefix SLNETIF_ID_ which
                         defined in slnetif.h

    \sa     SlNetIf_add()
    \sa     SlNetIf_getNameByID()
    \sa     SlNetSock_getIfID()

    \note                - Input NULL as ifName will return error code.
                         - When using more than one interface with the same
                           name, the ID of the highest priority interface
                           will be returned
    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_getIDByName snippet
*/
int32_t SlNetIf_getIDByName(char *ifName);


/*!
    \brief Get interface priority

    The SlNetIf_getPriority function retrieves the priority of the
    interface.

    \param[in] ifID      Specifies the interface which its priority
                         needs to be retrieved.\n
                         The values of the interface identifier is
                         defined with the prefix SLNETIF_ID_ which
                         defined in slnetif.h

    \return              The priority value of the interface on success,
                         or negative error code on failure

    \sa     SlNetIf_add()
    \sa     SlNetIf_setPriority()

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_getPriority snippet
*/
int32_t SlNetIf_getPriority(uint16_t ifID);


/*!
    \brief Set interface priority

    The SlNetIf_setPriority function sets new priority to the requested interface.

    \param[in] ifID          Specifies the interface which its priority
                             needs to be changed.\n
                             The values of the interface identifier is
                             defined with the prefix SLNETIF_ID_ which
                             defined in slnetif.h
    \param[in] priority      Specifies the priority needs to be set.
                             (In ascending order)
                             Note: maximum priority is 15

    \return                  Zero on success, or negative error code on
                             failure

    \sa     SlNetIf_add()
    \sa     SlNetIf_getPriority()

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_setPriority snippet
*/
int32_t SlNetIf_setPriority(uint16_t ifID, uint8_t priority);


/*!
    \brief Set interface state

    Enable or disable the interface.

    \param[in] ifID       Specifies the interface which its state
                          needs to be changed.\n
                          The values of the interface identifier is
                          defined with the prefix SLNETIF_ID_ which
                          defined in slnetif.h
    \param[in] ifState    Specifies the interface state.\n
                          The values of the interface state are defined
                          with the prefix SLNETIF_INTERFACE_ which
                          defined in slnetif.h

    \return               Zero on success, or negative error code on
                          failure

    \sa     SlNetIf_add()
    \sa     SlNetIf_getState()

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_setState snippet
*/
int32_t SlNetIf_setState(uint16_t ifID, SlNetIfState_e ifState);


/*!
    \brief Get interface state

    Obtain the current state of the interface.

    \param[in] ifID      Specifies the interface which its state needs
                         to be retrieved.\n
                         The values of the interface identifier is
                         defined with the prefix SLNETIF_ID_ which
                         defined in slnetif.h

    \return              State of the interface on success, or negative
                         error code on failure

    \sa     SlNetIf_add()
    \sa     SlNetIf_setState()

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_getState snippet
*/
int32_t SlNetIf_getState(uint16_t ifID);


/*!
    \brief Get the connection status of an interface

    \param[in] ifID      Interface ID

    \return              @c SLNETIF_STATUS_ value on success,
                         or negative error code on failure

    \remark              @c ifID should be a value with the @c SLNETIF_ID_
                         prefix

    \sa     SlNetIf_add()
    \sa     SLNETIF_STATUS_CONNECTED
    \sa     SLNETIF_STATUS_DISCONNECTED

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_getConnectionStatus snippet
*/
int32_t SlNetIf_getConnectionStatus(uint16_t ifID);


/*!
    \brief Get IP Address of specific interface

    The SlNetIf_getIPAddr function retrieve the IP address of a specific
    interface according to the Address Type, IPv4, IPv6 LOCAL
    or IPv6 GLOBAL.

    \param[in]  ifID          Specifies the interface which its connection
                              state needs to be retrieved.\n
                              The values of the interface identifier is
                              defined with the prefix SLNETIF_ID_ which
                              defined in slnetif.h
    \param[in]  addrType      Address type:
                                          - #SLNETIF_IPV4_ADDR
                                          - #SLNETIF_IPV6_ADDR_LOCAL
                                          - #SLNETIF_IPV6_ADDR_GLOBAL
    \param[out] addrConfig    Address config:
                                          - #SLNETIF_ADDR_CFG_UNKNOWN
                                          - #SLNETIF_ADDR_CFG_DHCP
                                          - #SLNETIF_ADDR_CFG_DHCP_LLA
                                          - #SLNETIF_ADDR_CFG_STATIC
                                          - #SLNETIF_ADDR_CFG_STATELESS
                                          - #SLNETIF_ADDR_CFG_STATEFUL
    \param[out] ipAddr        IP Address according to the Address Type

    \return                   Zero on success, or negative error code on failure

    \sa     SlNetIfAddressType_e

    \slnetif_not_threadsafe

    \par    Examples
    \snippet ti/net/test/snippets/slnetif.c SlNetIf_getIPAddr snippet
*/
int32_t SlNetIf_getIPAddr(uint16_t ifID, SlNetIfAddressType_e addrType, uint16_t *addrConfig, uint32_t *ipAddr);


/*!
    \brief Load secured buffer to the network stack

    The SlNetSock_secLoadObj function loads buffer/files into the inputted
    network stack for future usage of the socket SSL/TLS connection.
    This option is relevant for network stacks with file system and also for
    network stacks that lack file system that can store the secured files.

    \param[in] objType          Specifies the security object type which
                                could be one of the following:\n
                                   - #SLNETIF_SEC_OBJ_TYPE_RSA_PRIVATE_KEY
                                   - #SLNETIF_SEC_OBJ_TYPE_CERTIFICATE
                                   - #SLNETIF_SEC_OBJ_TYPE_DH_KEY
    \param[in] objName          Specifies the name/input identifier of the
                                secured buffer loaded
                                for file systems - this can be the file name
                                for plain text buffer loading this can be the
                                name of the object
    \param[in] objNameLen       Specifies the buffer name length to be loaded.\n
    \param[in] objBuff          Specifies the pointer to the secured buffer to
                                be loaded.\n
    \param[in] objBuffLen       Specifies the buffer length to be loaded.\n
    \param[in] ifBitmap         Specifies the interfaces which the security
                                objects needs to be added to.\n
                                The values of the interface identifiers
                                is defined with the prefix SLNETIF_ID_
                                which defined in slnetif.h

    \return                     On success, buffer type handler index to be
                                used when attaching the secured buffer to a
                                socket.\n
                                A successful return code should be a positive
                                number (int16)\n
                                On error, a negative value will be returned
                                specifying the error code.
                                - #SLNETERR_STATUS_ERROR - load operation failed

    \sa                         SlNetSock_setOpt()

    \slnetif_not_threadsafe

*/
int32_t SlNetIf_loadSecObj(uint16_t objType, char *objName, int16_t objNameLen, uint8_t *objBuff, int16_t objBuffLen, uint32_t ifBitmap);

/*!

 Close the Doxygen group.
 @}

*/

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __SL_NET_IF_H__ */
