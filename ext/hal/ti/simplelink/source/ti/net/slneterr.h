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

#ifndef __SL_NET_ERR_H__
#define __SL_NET_ERR_H__


#ifdef __cplusplus
extern "C" {
#endif

/*!
    \defgroup SlNetErr SlNetErr group

    \short Provide BSD and proprietary errors

*/
/*!

    \addtogroup SlNetErr
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

#define SLNETERR_RET_CODE_OK                                            (0L)     /**< Success                                                                 */

#define SLNETERR_GENERAL_DEVICE                                         (-6L)    /**< General device error                                                    */

/* BSD SOCKET ERRORS CODES */

#define SLNETERR_BSD_SOC_ERROR                                          (-1L)    /**< Failure                                                                 */
#define SLNETERR_BSD_ENXIO                                              (-6L)    /**< No such device or address                                               */
#define SLNETERR_BSD_INEXE                                              (-8L)    /**< socket command in execution                                             */
#define SLNETERR_BSD_EBADF                                              (-9L)    /**< Bad file number                                                         */
#define SLNETERR_BSD_ENSOCK                                             (-10L)   /**< The system limit on the total number of open sockets, has been reached   */
#define SLNETERR_BSD_EAGAIN                                             (-11L)   /**< Try again                                                               */
#define SLNETERR_BSD_EWOULDBLOCK                                        SLNETERR_BSD_EAGAIN
#define SLNETERR_BSD_ENOMEM                                             (-12L)   /**< Out of memory                                                           */
#define SLNETERR_BSD_EACCES                                             (-13L)   /**< Permission denied                                                       */
#define SLNETERR_BSD_EFAULT                                             (-14L)   /**< Bad address                                                             */
#define SLNETERR_BSD_ECLOSE                                             (-15L)   /**< close socket operation failed to transmit all queued packets            */
#define SLNETERR_BSD_EALREADY_ENABLED                                   (-21L)   /**< Transceiver - Transceiver already ON. there could be only one           */
#define SLNETERR_BSD_EINVAL                                             (-22L)   /**< Invalid argument                                                        */
#define SLNETERR_BSD_EAUTO_CONNECT_OR_CONNECTING                        (-69L)   /**< Transceiver - During connection, connected or auto mode started         */
#define SLNETERR_BSD_CONNECTION_PENDING                                 (-72L)   /**< Transceiver - Device is connected, disconnect first to open transceiver */
#define SLNETERR_BSD_EUNSUPPORTED_ROLE                                  (-86L)   /**< Transceiver - Trying to start when WLAN role is AP or P2P GO            */
#define SLNETERR_BSD_ENOTSOCK                                           (-88L)   /**< Socket operation on non-socket                                          */
#define SLNETERR_BSD_EDESTADDRREQ                                       (-89L)   /**< Destination address required                                            */
#define SLNETERR_BSD_EMSGSIZE                                           (-90L)   /**< Message too long                                                        */
#define SLNETERR_BSD_EPROTOTYPE                                         (-91L)   /**< Protocol wrong type for socket                                          */
#define SLNETERR_BSD_ENOPROTOOPT                                        (-92L)   /**< Protocol not available                                                  */
#define SLNETERR_BSD_EPROTONOSUPPORT                                    (-93L)   /**< Protocol not supported                                                  */
#define SLNETERR_BSD_ESOCKTNOSUPPORT                                    (-94L)   /**< Socket type not supported                                               */
#define SLNETERR_BSD_EOPNOTSUPP                                         (-95L)   /**< Operation not supported on transport endpoint                           */
#define SLNETERR_BSD_EAFNOSUPPORT                                       (-97L)   /**< Address family not supported by protocol                                */
#define SLNETERR_BSD_EADDRINUSE                                         (-98L)   /**< Address already in use                                                  */
#define SLNETERR_BSD_EADDRNOTAVAIL                                      (-99L)   /**< Cannot assign requested address                                         */
#define SLNETERR_BSD_ENETDOWN                                           (-100L)  /**< Network is down                                                         */
#define SLNETERR_BSD_ENETUNREACH                                        (-101L)  /**< Network is unreachable                                                  */
#define SLNETERR_BSD_ECONNABORTED                                       (-103L)  /**< Software caused connection abort                                        */
#define SLNETERR_BSD_ECONNRESET                                         (-104L)  /**< Connection reset by peer                                                */
#define SLNETERR_BSD_ENOBUFS                                            (-105L)  /**< No buffer space available                                               */
#define SLNETERR_BSD_EOBUFF                                             SLNETERR_BSD_ENOBUFS
#define SLNETERR_BSD_EISCONN                                            (-106L)  /**< Transport endpoint is already connected                                 */
#define SLNETERR_BSD_ENOTCONN                                           (-107L)  /**< Transport endpoint is not connected                                     */
#define SLNETERR_BSD_ESHUTDOWN                                          (-108L)  /**< Cannot send after transport endpoint shutdown                           */
#define SLNETERR_BSD_ETIMEDOUT                                          (-110L)  /**< Connection timed out                                                    */
#define SLNETERR_BSD_ECONNREFUSED                                       (-111L)  /**< Connection refused                                                      */
#define SLNETERR_BSD_EHOSTDOWN                                          (-112L)  /**< Host is down                                                            */
#define SLNETERR_BSD_EHOSTUNREACH                                       (-113L)  /**< No route to host                                                        */
#define SLNETERR_BSD_EALREADY                                           (-114L)  /**< Non blocking connect in progress, try again                             */

/* ssl tls security start with -300 offset */
#define SLNETERR_ESEC_CLOSE_NOTIFY                                      (-300L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_UNEXPECTED_MESSAGE                                (-310L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_BAD_RECORD_MAC                                    (-320L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_DECRYPTION_FAILED                                 (-321L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_RECORD_OVERFLOW                                   (-322L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_DECOMPRESSION_FAILURE                             (-330L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_HANDSHAKE_FAILURE                                 (-340L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_NO_CERTIFICATE                                    (-341L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_BAD_CERTIFICATE                                   (-342L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_UNSUPPORTED_CERTIFICATE                           (-343L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_ILLEGAL_PARAMETER                                 (-347L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_ACCESS_DENIED                                     (-349L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_DECODE_ERROR                                      (-350L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_DECRYPT_ERROR1                                    (-351L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_EXPORT_RESTRICTION                                (-360L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_PROTOCOL_VERSION                                  (-370L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_INSUFFICIENT_SECURITY                             (-371L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_INTERNAL_ERROR                                    (-380L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_USER_CANCELLED                                    (-390L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_NO_RENEGOTIATION                                  (-400L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_UNSUPPORTED_EXTENSION                             (-410L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_CERTIFICATE_UNOBTAINABLE                          (-411L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_UNRECOGNIZED_NAME                                 (-412L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_BAD_CERTIFICATE_STATUS_RESPONSE                   (-413L)  /**< ssl/tls alerts */
#define SLNETERR_ESEC_BAD_CERTIFICATE_HASH_VALUE                        (-414L)  /**< ssl/tls alerts */


/* proprietary secure */
#define SLNETERR_ESEC_GENERAL                                           (-450L)  /**< error secure level general error                                               */
#define SLNETERR_ESEC_DECRYPT                                           (-451L)  /**< error secure level, decrypt recv packet fail                                   */
#define SLNETERR_ESEC_CLOSED                                            (-452L)  /**< secure layer is closed by other size, tcp is still connected                   */
#define SLNETERR_ESEC_SNO_VERIFY                                        (-453L)  /**< Connected without server verification                                          */
#define SLNETERR_ESEC_NO_CA_FILE                                        (-454L)  /**< error secure level CA file not found                                           */
#define SLNETERR_ESEC_MEMORY                                            (-455L)  /**< error secure level No memory  space available                                  */
#define SLNETERR_ESEC_BAD_CA_FILE                                       (-456L)  /**< error secure level bad CA file                                                 */
#define SLNETERR_ESEC_BAD_CERT_FILE                                     (-457L)  /**< error secure level bad Certificate file                                        */
#define SLNETERR_ESEC_BAD_PRIVATE_FILE                                  (-458L)  /**< error secure level bad private file                                            */
#define SLNETERR_ESEC_BAD_DH_FILE                                       (-459L)  /**< error secure level bad DH file                                                 */
#define SLNETERR_ESEC_T00_MANY_SSL_OPENED                               (-460L)  /**< MAX SSL Sockets are opened                                                     */
#define SLNETERR_ESEC_DATE_ERROR                                        (-461L)  /**< connected with certificate date verification error                             */
#define SLNETERR_ESEC_HAND_SHAKE_TIMED_OUT                              (-462L)  /**< connection timed out due to handshake time                                     */
#define SLNETERR_ESEC_TX_BUFFER_NOT_EMPTY                               (-463L)  /**< cannot start ssl connection while send buffer is full                          */
#define SLNETERR_ESEC_RX_BUFFER_NOT_EMPTY                               (-464L)  /**< cannot start ssl connection while recv buffer is full                          */
#define SLNETERR_ESEC_SSL_DURING_HAND_SHAKE                             (-465L)  /**< cannot use while in handshaking                                                */
#define SLNETERR_ESEC_NOT_ALLOWED_WHEN_LISTENING                        (-466L)  /**< the operation is not allowed when listening, do before listen                  */
#define SLNETERR_ESEC_CERTIFICATE_REVOKED                               (-467L)  /**< connected but on of the certificates in the chain is revoked                   */
#define SLNETERR_ESEC_UNKNOWN_ROOT_CA                                   (-468L)  /**< connected but the root CA used to validate the peer is unknown                 */
#define SLNETERR_ESEC_WRONG_PEER_CERT                                   (-469L)  /**< wrong peer cert (server cert) was received while trying to connect to server   */
#define SLNETERR_ESEC_TCP_DISCONNECTED_UNCOMPLETE_RECORD                (-470L)  /**< the other side disconnected the TCP layer and didn't send the whole ssl record */
#define SLNETERR_ESEC_HELLO_VERIFY_ERROR                                (-471L)  /**< Hello verification failed in DTLS                                              */

#define SLNETERR_ESEC_BUFFER_E                                          (-632L)  /**< output buffer too small or input too large */
#define SLNETERR_ESEC_ALGO_ID_E                                         (-633L)  /**< setting algo id error                      */
#define SLNETERR_ESEC_PUBLIC_KEY_E                                      (-634L)  /**< setting public key error                   */
#define SLNETERR_ESEC_DATE_E                                            (-635L)  /**< setting date validity error                */
#define SLNETERR_ESEC_SUBJECT_E                                         (-636L)  /**< setting subject name error                 */
#define SLNETERR_ESEC_ISSUER_E                                          (-637L)  /**< setting issuer  name error                 */
#define SLNETERR_ESEC_CA_TRUE_E                                         (-638L)  /**< setting CA basic constraint true error     */
#define SLNETERR_ESEC_EXTENSIONS_E                                      (-639L)  /**< setting extensions error                   */
#define SLNETERR_ESEC_ASN_PARSE_E                                       (-640L)  /**< ASN parsing error, invalid input           */
#define SLNETERR_ESEC_ASN_VERSION_E                                     (-641L)  /**< ASN version error, invalid number          */
#define SLNETERR_ESEC_ASN_GETINT_E                                      (-642L)  /**< ASN get big int error, invalid data        */
#define SLNETERR_ESEC_ASN_RSA_KEY_E                                     (-643L)  /**< ASN key init error, invalid input          */
#define SLNETERR_ESEC_ASN_OBJECT_ID_E                                   (-644L)  /**< ASN object id error, invalid id            */
#define SLNETERR_ESEC_ASN_TAG_NULL_E                                    (-645L)  /**< ASN tag error, not null                    */
#define SLNETERR_ESEC_ASN_EXPECT_0_E                                    (-646L)  /**< ASN expect error, not zero                 */
#define SLNETERR_ESEC_ASN_BITSTR_E                                      (-647L)  /**< ASN bit string error, wrong id             */
#define SLNETERR_ESEC_ASN_UNKNOWN_OID_E                                 (-648L)  /**< ASN oid error, unknown sum id              */
#define SLNETERR_ESEC_ASN_DATE_SZ_E                                     (-649L)  /**< ASN date error, bad size                   */
#define SLNETERR_ESEC_ASN_BEFORE_DATE_E                                 (-650L)  /**< ASN date error, current date before        */
#define SLNETERR_ESEC_ASN_AFTER_DATE_E                                  (-651L)  /**< ASN date error, current date after         */
#define SLNETERR_ESEC_ASN_SIG_OID_E                                     (-652L)  /**< ASN signature error, mismatched oid        */
#define SLNETERR_ESEC_ASN_TIME_E                                        (-653L)  /**< ASN time error, unknown time type          */
#define SLNETERR_ESEC_ASN_INPUT_E                                       (-654L)  /**< ASN input error, not enough data           */
#define SLNETERR_ESEC_ASN_SIG_CONFIRM_E                                 (-655L)  /**< ASN sig error, confirm failure             */
#define SLNETERR_ESEC_ASN_SIG_HASH_E                                    (-656L)  /**< ASN sig error, unsupported hash type       */
#define SLNETERR_ESEC_ASN_SIG_KEY_E                                     (-657L)  /**< ASN sig error, unsupported key type        */
#define SLNETERR_ESEC_ASN_DH_KEY_E                                      (-658L)  /**< ASN key init error, invalid input          */
#define SLNETERR_ESEC_ASN_NTRU_KEY_E                                    (-659L)  /**< ASN ntru key decode error, invalid input   */
#define SLNETERR_ESEC_ASN_CRIT_EXT_E                                    (-660L)  /**< ASN unsupported critical extension         */
#define SLNETERR_ESEC_ECC_BAD_ARG_E                                     (-670L)  /**< ECC input argument of wrong type           */
#define SLNETERR_ESEC_ASN_ECC_KEY_E                                     (-671L)  /**< ASN ECC bad input                          */
#define SLNETERR_ESEC_ECC_CURVE_OID_E                                   (-672L)  /**< Unsupported ECC OID curve type             */
#define SLNETERR_ESEC_BAD_FUNC_ARG                                      (-673L)  /**< Bad function argument provided             */
#define SLNETERR_ESEC_NOT_COMPILED_IN                                   (-674L)  /**< Feature not compiled in                    */
#define SLNETERR_ESEC_UNICODE_SIZE_E                                    (-675L)  /**< Unicode password too big                   */
#define SLNETERR_ESEC_NO_PASSWORD                                       (-676L)  /**< no password provided by user               */
#define SLNETERR_ESEC_ALT_NAME_E                                        (-677L)  /**< alt name size problem, too big             */
#define SLNETERR_ESEC_ASN_NO_SIGNER_E                                   (-688L)  /**< ASN no signer to confirm failure           */
#define SLNETERR_ESEC_ASN_CRL_CONFIRM_E                                 (-689L)  /**< ASN CRL signature confirm failure          */
#define SLNETERR_ESEC_ASN_CRL_NO_SIGNER_E                               (-690L)  /**< ASN CRL no signer to confirm failure       */
#define SLNETERR_ESEC_ASN_OCSP_CONFIRM_E                                (-691L)  /**< ASN OCSP signature confirm failure         */
#define SLNETERR_ESEC_VERIFY_FINISHED_ERROR                             (-704L)  /**< verify problem on finished                 */
#define SLNETERR_ESEC_VERIFY_MAC_ERROR                                  (-705L)  /**< verify mac problem                         */
#define SLNETERR_ESEC_PARSE_ERROR                                       (-706L)  /**< parse error on header                      */
#define SLNETERR_ESEC_UNKNOWN_HANDSHAKE_TYPE                            (-707L)  /**< weird handshake type                       */
#define SLNETERR_ESEC_SOCKET_ERROR_E                                    (-708L)  /**< error state on socket                      */
#define SLNETERR_ESEC_SOCKET_NODATA                                     (-709L)  /**< expected data, not there                   */
#define SLNETERR_ESEC_INCOMPLETE_DATA                                   (-710L)  /**< don't have enough data to complete task    */
#define SLNETERR_ESEC_UNKNOWN_RECORD_TYPE                               (-711L)  /**< unknown type in record hdr                 */
#define SLNETERR_ESEC_INNER_DECRYPT_ERROR                               (-712L)  /**< error during decryption                    */
#define SLNETERR_ESEC_FATAL_ERROR                                       (-713L)  /**< recvd alert fatal error                    */
#define SLNETERR_ESEC_ENCRYPT_ERROR                                     (-714L)  /**< error during encryption                    */
#define SLNETERR_ESEC_FREAD_ERROR                                       (-715L)  /**< fread problem                              */
#define SLNETERR_ESEC_NO_PEER_KEY                                       (-716L)  /**< need peer's key                            */
#define SLNETERR_ESEC_NO_PRIVATE_KEY                                    (-717L)  /**< need the private key                       */
#define SLNETERR_ESEC_RSA_PRIVATE_ERROR                                 (-718L)  /**< error during rsa priv op                   */
#define SLNETERR_ESEC_NO_DH_PARAMS                                      (-719L)  /**< server missing DH params                   */
#define SLNETERR_ESEC_BUILD_MSG_ERROR                                   (-720L)  /**< build message failure                      */
#define SLNETERR_ESEC_BAD_HELLO                                         (-721L)  /**< client hello malformed                     */
#define SLNETERR_ESEC_DOMAIN_NAME_MISMATCH                              (-722L)  /**< peer subject name mismatch                 */
#define SLNETERR_ESEC_WANT_READ                                         (-723L)  /**< want read, call again                      */
#define SLNETERR_ESEC_NOT_READY_ERROR                                   (-724L)  /**< handshake layer not ready                  */
#define SLNETERR_ESEC_PMS_VERSION_ERROR                                 (-725L)  /**< pre m secret version error                 */
#define SLNETERR_ESEC_WANT_WRITE                                        (-727L)  /**< want write, call again                     */
#define SLNETERR_ESEC_BUFFER_ERROR                                      (-728L)  /**< malformed buffer input                     */
#define SLNETERR_ESEC_VERIFY_CERT_ERROR                                 (-729L)  /**< verify cert error                          */
#define SLNETERR_ESEC_VERIFY_SIGN_ERROR                                 (-730L)  /**< verify sign error                          */
#define SLNETERR_ESEC_LENGTH_ERROR                                      (-741L)  /**< record layer length error                  */
#define SLNETERR_ESEC_PEER_KEY_ERROR                                    (-742L)  /**< can't decode peer key                      */
#define SLNETERR_ESEC_ZERO_RETURN                                       (-743L)  /**< peer sent close notify                     */
#define SLNETERR_ESEC_SIDE_ERROR                                        (-744L)  /**< wrong client/server type                   */
#define SLNETERR_ESEC_NO_PEER_CERT                                      (-745L)  /**< peer didn't send key                       */
#define SLNETERR_ESEC_ECC_CURVETYPE_ERROR                               (-750L)  /**< Bad ECC Curve Type                         */
#define SLNETERR_ESEC_ECC_CURVE_ERROR                                   (-751L)  /**< Bad ECC Curve                              */
#define SLNETERR_ESEC_ECC_PEERKEY_ERROR                                 (-752L)  /**< Bad Peer ECC Key                           */
#define SLNETERR_ESEC_ECC_MAKEKEY_ERROR                                 (-753L)  /**< Bad Make ECC Key                           */
#define SLNETERR_ESEC_ECC_EXPORT_ERROR                                  (-754L)  /**< Bad ECC Export Key                         */
#define SLNETERR_ESEC_ECC_SHARED_ERROR                                  (-755L)  /**< Bad ECC Shared Secret                      */
#define SLNETERR_ESEC_NOT_CA_ERROR                                      (-757L)  /**< Not a CA cert error                        */
#define SLNETERR_ESEC_BAD_PATH_ERROR                                    (-758L)  /**< Bad path for opendir                       */
#define SLNETERR_ESEC_BAD_CERT_MANAGER_ERROR                            (-759L)  /**< Bad Cert Manager                           */
#define SLNETERR_ESEC_OCSP_CERT_REVOKED                                 (-760L)  /**< OCSP Certificate revoked                   */
#define SLNETERR_ESEC_CRL_CERT_REVOKED                                  (-761L)  /**< CRL Certificate revoked                    */
#define SLNETERR_ESEC_CRL_MISSING                                       (-762L)  /**< CRL Not loaded                             */
#define SLNETERR_ESEC_MONITOR_RUNNING_E                                 (-763L)  /**< CRL Monitor already running                */
#define SLNETERR_ESEC_THREAD_CREATE_E                                   (-764L)  /**< Thread Create Error                        */
#define SLNETERR_ESEC_OCSP_NEED_URL                                     (-765L)  /**< OCSP need an URL for lookup                */
#define SLNETERR_ESEC_OCSP_CERT_UNKNOWN                                 (-766L)  /**< OCSP responder doesn't know                */
#define SLNETERR_ESEC_OCSP_LOOKUP_FAIL                                  (-767L)  /**< OCSP lookup not successful                 */
#define SLNETERR_ESEC_MAX_CHAIN_ERROR                                   (-768L)  /**< max chain depth exceeded                   */
#define SLNETERR_ESEC_NO_PEER_VERIFY                                    (-778L)  /**< Need peer cert verify Error                */
#define SLNETERR_ESEC_UNSUPPORTED_SUITE                                 (-790L)  /**< unsupported cipher suite                   */
#define SLNETERR_ESEC_MATCH_SUITE_ERROR                                 (-791L)  /**< can't match cipher suite                   */



/* WLAN ERRORS CODES*/

#define SLNETERR_WLAN_KEY_ERROR                                         (-2049L)
#define SLNETERR_WLAN_INVALID_ROLE                                      (-2050L)
#define SLNETERR_WLAN_PREFERRED_NETWORKS_FILE_LOAD_FAILED               (-2051L)
#define SLNETERR_WLAN_CANNOT_CONFIG_SCAN_DURING_PROVISIONING            (-2052L)
#define SLNETERR_WLAN_INVALID_SECURITY_TYPE                             (-2054L)
#define SLNETERR_WLAN_PASSPHRASE_TOO_LONG                               (-2055L)
#define SLNETERR_WLAN_EAP_WRONG_METHOD                                  (-2057L)
#define SLNETERR_WLAN_PASSWORD_ERROR                                    (-2058L)
#define SLNETERR_WLAN_EAP_ANONYMOUS_LEN_ERROR                           (-2059L)
#define SLNETERR_WLAN_SSID_LEN_ERROR                                    (-2060L)
#define SLNETERR_WLAN_USER_ID_LEN_ERROR                                 (-2061L)
#define SLNETERR_WLAN_PREFERRED_NETWORK_LIST_FULL                       (-2062L)
#define SLNETERR_WLAN_PREFERRED_NETWORKS_FILE_WRITE_FAILED              (-2063L)
#define SLNETERR_WLAN_ILLEGAL_WEP_KEY_INDEX                             (-2064L)
#define SLNETERR_WLAN_INVALID_DWELL_TIME_VALUES                         (-2065L)
#define SLNETERR_WLAN_INVALID_POLICY_TYPE                               (-2066L)
#define SLNETERR_WLAN_PM_POLICY_INVALID_OPTION                          (-2067L)
#define SLNETERR_WLAN_PM_POLICY_INVALID_PARAMS                          (-2068L)
#define SLNETERR_WLAN_WIFI_NOT_CONNECTED                                (-2069L)
#define SLNETERR_WLAN_ILLEGAL_CHANNEL                                   (-2070L)
#define SLNETERR_WLAN_WIFI_ALREADY_DISCONNECTED                         (-2071L)
#define SLNETERR_WLAN_TRANSCEIVER_ENABLED                               (-2072L)
#define SLNETERR_WLAN_GET_NETWORK_LIST_EAGAIN                           (-2073L)
#define SLNETERR_WLAN_GET_PROFILE_INVALID_INDEX                         (-2074L)
#define SLNETERR_WLAN_FAST_CONN_DATA_INVALID                            (-2075L)
#define SLNETERR_WLAN_NO_FREE_PROFILE                                   (-2076L)
#define SLNETERR_WLAN_AP_SCAN_INTERVAL_TOO_LOW                          (-2077L)
#define SLNETERR_WLAN_SCAN_POLICY_INVALID_PARAMS                        (-2078L)

#define SLNETERR_RXFL_OK                                                (0L)     /**< O.K                                                                                     */
#define SLNETERR_RXFL_RANGE_COMPARE_PARAMS_ARE_INVALID                  (-2079L)
#define SLNETERR_RXFL_RXFL_INVALID_PATTERN_LENGTH                       (-2080L) /**< requested length for L1/L4 payload matching must not exceed 16 bytes                    */
#define SLNETERR_RXFL_ACTION_USER_EVENT_ID_TOO_BIG                      (-2081L) /**< user action id for host event must not exceed SLNETERR_WLAN_RX_FILTER_MAX_USER_EVENT_ID */
#define SLNETERR_RXFL_OFFSET_TOO_BIG                                    (-2082L) /**< requested offset for L1/L4 payload matching must not exceed 1535 bytes                  */
#define SLNETERR_RXFL_STAT_UNSUPPORTED                                  (-2083L) /**< get rx filters statistics not supported                                                 */
#define SLNETERR_RXFL_INVALID_FILTER_ARG_UPDATE                         (-2084L) /**< invalid filter args request                                                             */
#define SLNETERR_RXFL_INVALID_SYSTEM_STATE_TRIGGER_FOR_FILTER_TYPE      (-2085L) /**< system state not supported for this filter type                                         */
#define SLNETERR_RXFL_INVALID_FUNC_ID_FOR_FILTER_TYPE                   (-2086L) /**< function id not supported for this filter type                                          */
#define SLNETERR_RXFL_DEPENDENT_FILTER_DO_NOT_EXIST_3                   (-2087L) /**< filter parent doesn't exist                                                             */
#define SLNETERR_RXFL_OUTPUT_OR_INPUT_BUFFER_LENGTH_TOO_SMALL           (-2088L) /**< ! The output buffer length is smaller than required for that operation                  */
#define SLNETERR_RXFL_DEPENDENT_FILTER_SOFTWARE_FILTER_NOT_FIT          (-2089L) /**< Node filter can't be child of software filter and vice_versa                            */
#define SLNETERR_RXFL_DEPENDENCY_IS_NOT_PERSISTENT                      (-2090L) /**< Dependency filter is not persistent                                                     */
#define SLNETERR_RXFL_RXFL_ALLOCATION_PROBLEM                           (-2091L)
#define SLNETERR_RXFL_SYSTEM_STATE_NOT_SUPPORTED_FOR_THIS_FILTER        (-2092L) /**< System state is not supported                                                           */
#define SLNETERR_RXFL_TRIGGER_USE_REG5_TO_REG8                          (-2093L) /**< Only counters 5 - 8 are allowed, for trigger                                            */
#define SLNETERR_RXFL_TRIGGER_USE_REG1_TO_REG4                          (-2094L) /**< Only counters 1 - 4 are allowed, for trigger                                            */
#define SLNETERR_RXFL_ACTION_USE_REG5_TO_REG8                           (-2095L) /**< Only counters 5 - 8 are allowed, for action                                             */
#define SLNETERR_RXFL_ACTION_USE_REG1_TO_REG4                           (-2096L) /**< Only counters 1 - 4 are allowed, for action                                             */
#define SLNETERR_RXFL_FIELD_SUPPORT_ONLY_EQUAL_AND_NOTEQUAL             (-2097L) /**< Rule compare function Id is out of range                                                */
#define SLNETERR_RXFL_WRONG_MULTICAST_BROADCAST_ADDRESS                 (-2098L) /**< The address should be of type multicast or broadcast                                    */
#define SLNETERR_RXFL_THE_FILTER_IS_NOT_OF_HEADER_TYPE                  (-2099L) /**< The filter should be of header type                                                     */
#define SLNETERR_RXFL_WRONG_COMPARE_FUNC_FOR_BROADCAST_ADDRESS          (-2100L) /**< The compare function is not suitable for broadcast address                              */
#define SLNETERR_RXFL_WRONG_MULTICAST_ADDRESS                           (-2101L) /**< The address should be of multicast type                                                 */
#define SLNETERR_RXFL_DEPENDENT_FILTER_IS_NOT_PERSISTENT                (-2102L) /**< The dependency filter is not persistent                                                 */
#define SLNETERR_RXFL_DEPENDENT_FILTER_IS_NOT_ENABLED                   (-2103L) /**< The dependency filter is not enabled                                                    */
#define SLNETERR_RXFL_FILTER_HAS_CHILDS                                 (-2104L) /**< The filter has childs and can't be removed                                              */
#define SLNETERR_RXFL_CHILD_IS_ENABLED                                  (-2105L) /**< Can't disable filter while the child is enabled                                         */
#define SLNETERR_RXFL_DEPENDENCY_IS_DISABLED                            (-2106L) /**< Can't enable filter in case its dependency filter is disabled                           */
#define SLNETERR_RXFL_MAC_SEND_MATCHDB_FAILED                           (-2107L)
#define SLNETERR_RXFL_MAC_SEND_ARG_DB_FAILED                            (-2108L)
#define SLNETERR_RXFL_MAC_SEND_NODEDB_FAILED                            (-2109L)
#define SLNETERR_RXFL_MAC_OPERTATION_RESUME_FAILED                      (-2110L)
#define SLNETERR_RXFL_MAC_OPERTATION_HALT_FAILED                        (-2111L)
#define SLNETERR_RXFL_NUMBER_OF_CONNECTION_POINTS_EXCEEDED              (-2112L) /**< Number of connection points exceeded                                                    */
#define SLNETERR_RXFL_DEPENDENT_FILTER_DEPENDENCY_ACTION_IS_DROP        (-2113L) /**< The dependent filter has Drop action, thus the filter can't be created                  */
#define SLNETERR_RXFL_FILTER_DO_NOT_EXISTS                              (-2114L) /**< The filter doesn't exists                                                               */
#define SLNETERR_RXFL_DEPEDENCY_NOT_ON_THE_SAME_LAYER                   (-2115L) /**< The filter and its dependency must be on the same layer                                 */
#define SLNETERR_RXFL_NUMBER_OF_ARGS_EXCEEDED                           (-2116L) /**< Number of arguments exceeded                                                            */
#define SLNETERR_RXFL_ACTION_NO_REG_NUMBER                              (-2117L) /**< Action require counter number                                                           */
#define SLNETERR_RXFL_DEPENDENT_FILTER_LAYER_DO_NOT_FIT                 (-2118L) /**< the filter and its dependency should be from the same layer                             */
#define SLNETERR_RXFL_DEPENDENT_FILTER_SYSTEM_STATE_DO_NOT_FIT          (-2119L) /**< The filter and its dependency system state don't fit                                    */
#define SLNETERR_RXFL_DEPENDENT_FILTER_DO_NOT_EXIST_2                   (-2120L) /**< The parent filter don't exist                                                           */
#define SLNETERR_RXFL_DEPENDENT_FILTER_DO_NOT_EXIST_1                   (-2121L) /**< The parent filter is null                                                               */
#define SLNETERR_RXFL_RULE_HEADER_ACTION_TYPE_NOT_SUPPORTED             (-2122L) /**< The action type is not supported                                                        */
#define SLNETERR_RXFL_RULE_HEADER_TRIGGER_COMPARE_FUNC_OUT_OF_RANGE     (-2123L) /**< The Trigger comparison function is out of range                                         */
#define SLNETERR_RXFL_RULE_HEADER_TRIGGER_OUT_OF_RANGE                  (-2124L) /**< The Trigger is out of range                                                             */
#define SLNETERR_RXFL_RULE_HEADER_COMPARE_FUNC_OUT_OF_RANGE             (-2125L) /**< The rule compare function is out of range                                               */
#define SLNETERR_RXFL_FRAME_TYPE_NOT_SUPPORTED                          (-2126L) /**< ASCII frame type string is illegal                                                      */
#define SLNETERR_RXFL_RULE_FIELD_ID_NOT_SUPPORTED                       (-2127L) /**< Rule field ID is out of range                                                           */
#define SLNETERR_RXFL_RULE_HEADER_FIELD_ID_ASCII_NOT_SUPPORTED          (-2128L) /**< This ASCII field ID is not supported                                                    */
#define SLNETERR_RXFL_RULE_HEADER_NOT_SUPPORTED                         (-2129L) /**< The header rule is not supported on current release                                     */
#define SLNETERR_RXFL_RULE_HEADER_OUT_OF_RANGE                          (-2130L) /**< The header rule is out of range                                                         */
#define SLNETERR_RXFL_RULE_HEADER_COMBINATION_OPERATOR_OUT_OF_RANGE     (-2131L) /**< Combination function Id is out of range                                                 */
#define SLNETERR_RXFL_RULE_HEADER_FIELD_ID_OUT_OF_RANGE                 (-2132L) /**< rule field Id is out of range                                                           */
#define SLNETERR_RXFL_UPDATE_NOT_SUPPORTED                              (-2133L) /**< Update not supported                                                                    */
#define SLNETERR_RXFL_NO_FILTER_DATABASE_ALLOCATE                       (-2134L)
#define SLNETERR_RXFL_ALLOCATION_FOR_GLOBALS_STRUCTURE_FAILED           (-2135L)
#define SLNETERR_RXFL_ALLOCATION_FOR_DB_NODE_FAILED                     (-2136L)
#define SLNETERR_RXFL_READ_FILE_FILTER_ID_ILLEGAL                       (-2137L)
#define SLNETERR_RXFL_READ_FILE_NUMBER_OF_FILTER_FAILED                 (-2138L)
#define SLNETERR_RXFL_READ_FILE_FAILED                                  (-2139L)
#define SLNETERR_RXFL_NO_FILTERS_ARE_DEFINED                            (-2140L) /**< No filters are defined in the system                                                    */
#define SLNETERR_RXFL_NUMBER_OF_FILTER_EXCEEDED                         (-2141L) /**< Number of max filters exceeded                                                          */
#define SLNETERR_RXFL_BAD_FILE_MODE                                     (-2142L)
#define SLNETERR_RXFL_FAILED_READ_NVFILE                                (-2143L)
#define SLNETERR_RXFL_FAILED_INIT_STORAGE                               (-2144L)
#define SLNETERR_RXFL_CONTINUE_WRITE_MUST_BE_MOD_4                      (-2145L)
#define SLNETERR_RXFL_FAILED_LOAD_FILE                                  (-2146L)
#define SLNETERR_RXFL_INVALID_HANDLE                                    (-2147L)
#define SLNETERR_RXFL_FAILED_TO_WRITE                                   (-2148L)
#define SLNETERR_RXFL_OFFSET_OUT_OF_RANGE                               (-2149L)
#define SLNETERR_RXFL_ALLOC                                             (-2150L)
#define SLNETERR_RXFL_READ_DATA_LENGTH                                  (-2151L)
#define SLNETERR_RXFL_INVALID_FILE_ID                                   (-2152L)
#define SLNETERR_RXFL_FILE_FILTERS_NOT_EXISTS                           (-2153L)
#define SLNETERR_RXFL_FILE_ALREADY_IN_USE                               (-2154L)
#define SLNETERR_RXFL_INVALID_ARGS                                      (-2155L)
#define SLNETERR_RXFL_FAILED_TO_CREATE_FILE                             (-2156L)
#define SLNETERR_RXFL_FS_ALREADY_LOADED                                 (-2157L)
#define SLNETERR_RXFL_UNKNOWN                                           (-2158L)
#define SLNETERR_RXFL_FAILED_TO_CREATE_LOCK_OBJ                         (-2159L)
#define SLNETERR_RXFL_DEVICE_NOT_LOADED                                 (-2160L)
#define SLNETERR_RXFL_INVALID_MAGIC_NUM                                 (-2161L)
#define SLNETERR_RXFL_FAILED_TO_READ                                    (-2162L)
#define SLNETERR_RXFL_NOT_SUPPORTED                                     (-2163L)
#define SLNETERR_WLAN_INVALID_COUNTRY_CODE                              (-2164L)
#define SLNETERR_WLAN_NVMEM_ACCESS_FAILED                               (-2165L)
#define SLNETERR_WLAN_OLD_FILE_VERSION                                  (-2166L)
#define SLNETERR_WLAN_TX_POWER_OUT_OF_RANGE                             (-2167L)
#define SLNETERR_WLAN_INVALID_AP_PASSWORD_LENGTH                        (-2168L)
#define SLNETERR_WLAN_PROVISIONING_ABORT_PROVISIONING_ALREADY_STARTED   (-2169L)
#define SLNETERR_WLAN_PROVISIONING_ABORT_HTTP_SERVER_DISABLED           (-2170L)
#define SLNETERR_WLAN_PROVISIONING_ABORT_PROFILE_LIST_FULL              (-2171L)
#define SLNETERR_WLAN_PROVISIONING_ABORT_INVALID_PARAM                  (-2172L)
#define SLNETERR_WLAN_PROVISIONING_ABORT_GENERAL_ERROR                  (-2173L)
#define SLNETERR_WLAN_MULTICAST_EXCEED_MAX_ADDR                         (-2174L)
#define SLNETERR_WLAN_MULTICAST_INVAL_ADDR                              (-2175L)
#define SLNETERR_WLAN_AP_SCAN_INTERVAL_TOO_SHORT                        (-2176L)
#define SLNETERR_WLAN_PROVISIONING_CMD_NOT_EXPECTED                     (-2177L)


#define SLNETERR_WLAN_AP_ACCESS_LIST_NO_ADDRESS_TO_DELETE               (-2178L) /**< List is empty, no address to delete                 */
#define SLNETERR_WLAN_AP_ACCESS_LIST_FULL                               (-2179L) /**< access list is full                                 */
#define SLNETERR_WLAN_AP_ACCESS_LIST_DISABLED                           (-2180L) /**< access list is disabled                             */
#define SLNETERR_WLAN_AP_ACCESS_LIST_MODE_NOT_SUPPORTED                 (-2181L) /**< Trying to switch to unsupported mode                */
#define SLNETERR_WLAN_AP_STA_NOT_FOUND                                  (-2182L) /**< trying to disconnect station which is not connected */



/* DEVICE ERRORS CODES*/
#define SLNETERR_SUPPLICANT_ERROR                                       (-4097L)
#define SLNETERR_HOSTAPD_INIT_FAIL                                      (-4098L)
#define SLNETERR_HOSTAPD_INIT_IF_FAIL                                   (-4099L)
#define SLNETERR_WLAN_DRV_INIT_FAIL                                     (-4100L)
#define SLNETERR_MDNS_ENABLE_FAIL                                       (-4103L) /**< mDNS enable failed                                                                                            */
#define SLNETERR_ROLE_STA_ERR                                           (-4107L) /**< Failure to load MAC/PHY in STA role                                                                           */
#define SLNETERR_ROLE_AP_ERR                                            (-4108L) /**< Failure to load MAC/PHY in AP role                                                                            */
#define SLNETERR_ROLE_P2P_ERR                                           (-4109L) /**< Failure to load MAC/PHY in P2P role                                                                           */
#define SLNETERR_CALIB_FAIL                                             (-4110L) /**< Failure of calibration                                                                                        */
#define SLNETERR_RESTORE_IMAGE_COMPLETE                                 (-4113L) /**< Return to factory image completed, perform reset                                                              */
#define SLNETERR_UNKNOWN_ERR                                            (-4114L)
#define SLNETERR_GENERAL_ERR                                            (-4115L) /**< General error during init                                                                                     */
#define SLNETERR_WRONG_ROLE                                             (-4116L)
#define SLNETERR_INCOMPLETE_PROGRAMMING                                 (-4117L) /**< Error during programming, Program new image should be invoked (see sl_FsProgram)                              */


#define SLNETERR_PENDING_TXRX_STOP_TIMEOUT_EXP                          (-4118L) /**< Timeout expired before completing all TX/RX                                                                   */
#define SLNETERR_PENDING_TXRX_NO_TIMEOUT                                (-4119L) /**< No Timeout, still have pending  TX/RX                                                                         */
#define SLNETERR_INVALID_PERSISTENT_CONFIGURATION                       (-4120L) /**< persistent configuration can only be set to 0 (disabled) or 1 (enabled)                                      */



/* NETAPP ERRORS CODES*/
#define SLNETERR_MDNS_CREATE_FAIL                                       (-6145L) /**< mDNS create failed                                   */
#define SLNETERR_DEVICE_NAME_LEN_ERR                                    (-6146L) /**< Set Dev name error codes                             */
#define SLNETERR_DEVICE_NAME_INVALID                                    (-6147L) /**< Set Dev name error codes                             */
#define SLNETERR_DOMAIN_NAME_LEN_ERR                                    (-6148L) /**< Set domain name error codes                          */
#define SLNETERR_DOMAIN_NAME_INVALID                                    (-6149L) /**< Set domain name error codes                          */
#define SLNETERR_NET_APP_DNS_QUERY_NO_RESPONSE                          (-6150L) /**< DNS query failed, no response                        */
#define SLNETERR_NET_APP_DNS_ERROR                                      (-6151L) /**< DNS internal error                                   */
#define SLNETERR_NET_APP_DNS_NO_SERVER                                  (-6152L) /**< No DNS server was specified                          */
#define SLNETERR_NET_APP_DNS_TIMEOUTR                                   (-6153L) /**< mDNS parameters error                                */
#define SLNETERR_NET_APP_DNS_QUERY_FAILED                               (-6154L) /**< DNS query failed; no DNS server sent an 'answer'     */
#define SLNETERR_NET_APP_DNS_BAD_ADDRESS_ERROR                          (-6155L) /**< Improperly formatted IPv4 or IPv6 address            */
#define SLNETERR_NET_APP_DNS_SIZE_ERROR                                 (-6156L) /**< DNS destination size is too small                    */
#define SLNETERR_NET_APP_DNS_MALFORMED_PACKET                           (-6157L) /**< Improperly formed or corrupted DNS packet received   */
#define SLNETERR_NET_APP_DNS_BAD_ID_ERROR                               (-6158L) /**< DNS packet from server does not match query ID       */
#define SLNETERR_NET_APP_DNS_PARAM_ERROR                                (-6159L) /**< Invalid params                                       */
#define SLNETERR_NET_APP_DNS_SERVER_NOT_FOUND                           (-6160L) /**< Server not found in Client list of DNS servers       */
#define SLNETERR_NET_APP_DNS_PACKET_CREATE_ERROR                        (-6161L) /**< Error creating DNS packet                            */
#define SLNETERR_NET_APP_DNS_EMPTY_DNS_SERVER_LIST                      (-6162L) /**< DNS Client's list of DNS servers is empty            */
#define SLNETERR_NET_APP_DNS_SERVER_AUTH_ERROR                          (-6163L) /**< Server not able to authenticate answer/authority data*/
#define SLNETERR_NET_APP_DNS_ZERO_GATEWAY_IP_ADDRESS                    (-6164L) /**< DNS Client IP instance has a zero gateway IP address */
#define SLNETERR_NET_APP_DNS_MISMATCHED_RESPONSE                        (-6165L) /**< Server response type does not match the query request*/
#define SLNETERR_NET_APP_DNS_DUPLICATE_ENTRY                            (-6166L) /**< Duplicate entry exists in DNS server table           */
#define SLNETERR_NET_APP_DNS_RETRY_A_QUERY                              (-6167L) /**< SOA status returned; web site only exists as IPv4    */
#define SLNETERR_NET_APP_DNS_INVALID_ADDRESS_TYPE                       (-6168L) /**< IP address type (e.g. IPv6L) not supported           */
#define SLNETERR_NET_APP_DNS_IPV6_NOT_SUPPORTED                         (-6169L) /**< IPv6 disabled                                        */
#define SLNETERR_NET_APP_DNS_NEED_MORE_RECORD_BUFFER                    (-6170L) /**< The buffer size is not enough.                       */
#define SLNETERR_NET_APP_MDNS_ERROR                                     (-6171L) /**< MDNS internal error.                                 */
#define SLNETERR_NET_APP_MDNS_PARAM_ERROR                               (-6172L) /**< MDNS parameters error.                               */
#define SLNETERR_NET_APP_MDNS_CACHE_ERROR                               (-6173L) /**< The Cache size is not enough.                        */
#define SLNETERR_NET_APP_MDNS_UNSUPPORTED_TYPE                          (-6174L) /**< The unsupported resource record type.                */
#define SLNETERR_NET_APP_MDNS_DATA_SIZE_ERROR                           (-6175L) /**< The data size is too big.                            */
#define SLNETERR_NET_APP_MDNS_AUTH_ERROR                                (-6176L) /**< Attempting to parse too large a data.                */
#define SLNETERR_NET_APP_MDNS_PACKET_ERROR                              (-6177L) /**< The packet can not add the resource record.          */
#define SLNETERR_NET_APP_MDNS_DEST_ADDRESS_ERROR                        (-6178L) /**< The destination address error.                       */
#define SLNETERR_NET_APP_MDNS_UDP_PORT_ERROR                            (-6179L) /**< The udp port error.                                  */
#define SLNETERR_NET_APP_MDNS_NOT_LOCAL_LINK                            (-6180L) /**< The message that not originate from the local link.  */
#define SLNETERR_NET_APP_MDNS_EXCEED_MAX_LABEL                          (-6181L) /**< The data exceed the max label size.                  */
#define SLNETERR_NET_APP_MDNS_EXIST_UNIQUE_RR                           (-6182L) /**< At least one unique record in the cache.             */
#define SLNETERR_NET_APP_MDNS_EXIST_ANSWER                              (-6183L) /**< At least one answer record in the cache.             */
#define SLNETERR_NET_APP_MDNS_EXIST_SAME_QUERY                          (-6184L) /**< Exist the same query.                                */
#define SLNETERR_NET_APP_MDNS_DUPLICATE_SERVICE                         (-6185L) /**< Duplicate service.                                   */
#define SLNETERR_NET_APP_MDNS_NO_ANSWER                                 (-6186L) /**< No response for one-shot query.                      */
#define SLNETERR_NET_APP_MDNS_NO_KNOWN_ANSWER                           (-6187L) /**< No known answer for query.                           */
#define SLNETERR_NET_APP_MDNS_NAME_MISMATCH                             (-6188L) /**< The name mismatch.                                   */
#define SLNETERR_NET_APP_MDNS_NOT_STARTED                               (-6189L) /**< MDNS does not start.                                 */
#define SLNETERR_NET_APP_MDNS_HOST_NAME_ERROR                           (-6190L) /**< MDNS host name error.                                */
#define SLNETERR_NET_APP_MDNS_NO_MORE_ENTRIES                           (-6191L) /**< No more entries be found.                            */
#define SLNETERR_NET_APP_MDNS_SERVICE_TYPE_MISMATCH                     (-6192L) /**< The service type mismatch                            */
#define SLNETERR_NET_APP_MDNS_LOOKUP_INDEX_ERROR                        (-6193L) /**< Index is bigger than number of services.             */
#define SLNETERR_NET_APP_MDNS_MAX_SERVICES_ERROR                        (-6194L)
#define SLNETERR_NET_APP_MDNS_IDENTICAL_SERVICES_ERROR                  (-6195L)
#define SLNETERR_NET_APP_MDNS_EXISTED_SERVICE_ERROR                     (-6196L)
#define SLNETERR_NET_APP_MDNS_ERROR_SERVICE_NAME_ERROR                  (-6197L)
#define SLNETERR_NET_APP_MDNS_RX_PACKET_ALLOCATION_ERROR                (-6198L)
#define SLNETERR_NET_APP_MDNS_BUFFER_SIZE_ERROR                         (-6199L)
#define SLNETERR_NET_APP_MDNS_NET_APP_SET_ERROR                         (-6200L)
#define SLNETERR_NET_APP_MDNS_GET_SERVICE_LIST_FLAG_ERROR               (-6201L)
#define SLNETERR_NET_APP_MDNS_MDNS_NO_CONFIGURATION_ERROR               (-6202L)
#define SLNETERR_NET_APP_MDNS_STATUS_ERROR                              (-6203L)
#define SLNETERR_NET_APP_ENOBUFS                                        (-6204L)
#define SLNETERR_NET_APP_DNS_IPV6_REQ_BUT_IPV6_DISABLED                 (-6205L) /**< trying to issue ipv6 DNS request but ipv6 is disabled                          */
#define SLNETERR_NET_APP_DNS_INVALID_FAMILY_TYPE                        (-6206L) /**< Family type is not ipv4 and not ipv6                                           */
#define SLNETERR_NET_APP_DNS_REQ_TOO_BIG                                (-6207L) /**< DNS request size is too big                                                    */
#define SLNETERR_NET_APP_DNS_ALLOC_ERROR                                (-6208L) /**< Allocation error                                                               */
#define SLNETERR_NET_APP_DNS_EXECUTION_ERROR                            (-6209L) /**< Execution error                                                                */
#define SLNETERR_NET_APP_P2P_ROLE_IS_NOT_CONFIGURED                     (-6210L) /**< role p2p is not configured yet, should be CL or GO in order to execute command */
#define SLNETERR_NET_APP_INCORECT_ROLE_FOR_APP                          (-6211L) /**< incorrect role for specific application                                        */
#define SLNETERR_NET_APP_INCORECT_APP_MASK                              (-6212L) /**< mask does not match any app                                                    */
#define SLNETERR_NET_APP_MDNS_ALREADY_STARTED                           (-6213L) /**< mdns application already started                                               */
#define SLNETERR_NET_APP_HTTP_SERVER_ALREADY_STARTED                    (-6214L) /**< http server application already started                                        */

#define SLNETERR_NET_APP_HTTP_GENERAL_ERROR                             (-6216L) /**< New error - Http handle request failed                                         */
#define SLNETERR_NET_APP_HTTP_INVALID_TIMEOUT                           (-6217L) /**< New error - Http timeout invalid argument                                      */
#define SLNETERR_NET_APP_INVALID_URN_LENGTH                             (-6218L) /**< invalid URN length                                                             */
#define SLNETERR_NET_APP_RX_BUFFER_LENGTH                               (-6219L) /**< size of the requested services is smaller than size of the user buffer */



/*< NETCFG ERRORS CODES*/
#define SLNETERR_STATIC_ADDR_SUBNET_ERROR                               (-8193L)
#define SLNETERR_INCORRECT_IPV6_STATIC_LOCAL_ADDR                       (-8194L) /**< Ipv6 Local address prefix is wrong */
#define SLNETERR_INCORRECT_IPV6_STATIC_GLOBAL_ADDR                      (-8195L) /**< Ipv6 Global address prefix is wrong */
#define SLNETERR_IPV6_LOCAL_ADDR_SHOULD_BE_SET_FIRST                    (-8195L) /**< Attempt to set ipv6 global address before ipv6 local address is set */



/* NETUTIL ERRORS CODES */
#define SLNETERR_NETUTIL_CRYPTO_GENERAL                                 (-12289L)
#define SLNETERR_NETUTIL_CRYPTO_INVALID_INDEX                           (-12290L)
#define SLNETERR_NETUTIL_CRYPTO_INVALID_PARAM                           (-12291L)
#define SLNETERR_NETUTIL_CRYPTO_MEM_ALLOC                               (-12292L)
#define SLNETERR_NETUTIL_CRYPTO_INVALID_DB_VER                          (-12293L)
#define SLNETERR_NETUTIL_CRYPTO_UNSUPPORTED_OPTION                      (-12294L)
#define SLNETERR_NETUTIL_CRYPTO_BUFFER_TOO_SMALL                        (-12295L)
#define SLNETERR_NETUTIL_CRYPTO_EMPTY_DB_ENTRY                          (-12296L)
#define SLNETERR_NETUTIL_CRYPTO_NON_TEMPORARY_KEY                       (-12297L)
#define SLNETERR_NETUTIL_CRYPTO_DB_ENTRY_NOT_FREE                       (-12298L)
#define SLNETERR_NETUTIL_CRYPTO_CORRUPTED_DB_FILE                       (-12299L)



/* GENERAL ERRORS CODES*/
#define SLNETERR_INVALID_OPCODE                                         (-14337L)
#define SLNETERR_INVALID_PARAM                                          (-14338L)
#define SLNETERR_STATUS_ERROR                                           (-14341L)
#define SLNETERR_NVMEM_ACCESS_FAILED                                    (-14342L)
#define SLNETERR_NOT_ALLOWED_NWP_LOCKED                                 (-14343L) /**< Device is locked, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram) */

/* SECURITY ERRORS CODE */
#define SLNETERR_LOADING_CERTIFICATE_STORE                              (-28673L)

/* Device is Locked! Return to Factory Image or Program new
   image should be invoked (see sl_FsCtl, sl_FsProgram) */
#define SLNETERR_DEVICE_LOCKED_SECURITY_ALERT                           (-28674L)



/* INTERNAL HOST ERRORS CODES*/

/* Receive this error in case there are no resources to issue the command
   If possible, increase the number of MAX_CONCURRENT_ACTIONS (result in memory increase)
   If not, try again later */
#define SLNETERR_POOL_IS_EMPTY                                          (-2000L)

/* Receive this error in case a given length for RX buffer was too small.
   Receive payload was bigger than the given buffer size. Therefore, payload is cut according to receive size
   Recommend to increase buffer size */
#define SLNETERR_ESMALLBUF                                              (-2001L)

/* Receive this error in case zero length is supplied to a "get" API
   Recommend to supply length according to requested information (view options defines for help) */
#define SLNETERR_EZEROLEN                                               (-2002L)

/* User supplied invalid parameter */
#define SLNETERR_INVALPARAM                                             (-2003L)

/* Failed to open interface  */
#define SLNETERR_BAD_INTERFACE                                          (-2004L)

/* API has been aborted due to an error detected by host driver */
#define SLNETERR_API_ABORTED                                            (-2005L)

/* Parameters are invalid */
#define SLNETERR_RET_CODE_INVALID_INPUT                                 (-2006L)

/* Driver internal error */
#define SLNETERR_RET_CODE_SELF_ERROR                                    (-2007L)

/* NWP internal error */
#define SLNETERR_RET_CODE_NWP_IF_ERROR                                  (-2008L)

/* malloc error */
#define SLNETERR_RET_CODE_MALLOC_ERROR                                  (-2009L)

/* protocol error */
#define SLNETERR_RET_CODE_PROTOCOL_ERROR                                (-2010L)

/* API has been aborted, command is not allowed in device lock state */
#define SLNETERR_RET_CODE_DEV_LOCKED                                    (-2011L)

/* SlNetSock_Start cannot be invoked twice */
#define SLNETERR_RET_CODE_DEV_ALREADY_STARTED                           (-2012L)

/* SL Net API is in progress */
#define SLNETERR_RET_CODE_API_COMMAND_IN_PROGRESS                       (-2013L)

/* Provisioning is in progress -  */
#define SLNETERR_RET_CODE_PROVISIONING_IN_PROGRESS                      (-2014L)

/* Wrong ping parameters - ping cannot be called with the following parameters:
1. infinite ping packet
2. report only when finished
3. no callback supplied  */
#define SLNETERR_RET_CODE_NET_APP_PING_INVALID_PARAMS                   (-2015L)


/* SlNetSock select already in progress.
   this error will be returned if app will try to call
   SlNetSock_select blocking when there is already select trigger in progress */
#define SLNETERR_RET_CODE_SOCKET_SELECT_IN_PROGRESS_ERROR               (-2016L)

#define SLNETERR_RET_CODE_STOP_IN_PROGRESS                              (-2017L)


/* The device has not been started yet */
#define SLNETERR_RET_CODE_DEV_NOT_STARTED                               (-2018L)

/* The event link was not found in the list */
#define SLNETERR_RET_CODE_EVENT_LINK_NOT_FOUND                          (-2019L)

/* Function couldn't find any free space/location */
#define SLNETERR_RET_CODE_NO_FREE_SPACE                                 (-2020L)

/* Function couldn't execute correctly */
#define SLNETERR_RET_CODE_FUNCTION_FAILED                               (-2021L)

/* Mutex creation failed */
#define SLNETERR_RET_CODE_MUTEX_CREATION_FAILED                         (-2022L)

/* Function couldn't find the requested resource */
#define SLNETERR_RET_CODE_COULDNT_FIND_RESOURCE                         (-2023L)

/* Interface doesn't support the non mandatory function */
#define SLNETERR_RET_CODE_DOESNT_SUPPORT_NON_MANDATORY_FXN              (-2024L)

/* Socket creation in progress */
#define SLNETERR_RET_CODE_SOCKET_CREATION_IN_PROGRESS                   (-2025L)

/* Unsupported scenario, option or feature */
#define SLNETERR_RET_CODE_UNSUPPORTED                                   (-2026L)


/* sock related API's from SlNetIf_Config_t failed */
#define SLNETSOCK_ERR_SOCKCREATE_FAILED                                 (-3000L)
#define SLNETSOCK_ERR_SOCKCLOSE_FAILED                                  (-3001L)
#define SLNETSOCK_ERR_SOCKSELECT_FAILED                                 (-3002L)
#define SLNETSOCK_ERR_SOCKSETOPT_FAILED                                 (-3003L)
#define SLNETSOCK_ERR_SOCKGETOPT_FAILED                                 (-3004L)
#define SLNETSOCK_ERR_SOCKRECVFROM_FAILED                               (-3005L)
#define SLNETSOCK_ERR_SOCKSENDTO_FAILED                                 (-3006L)
#define SLNETSOCK_ERR_SOCKSHUTDOWN_FAILED                               (-3007L)
#define SLNETSOCK_ERR_SOCKACCEPT_FAILED                                 (-3008L)
#define SLNETSOCK_ERR_SOCKBIND_FAILED                                   (-3009L)
#define SLNETSOCK_ERR_SOCKLISTEN_FAILED                                 (-3000L)
#define SLNETSOCK_ERR_SOCKCONNECT_FAILED                                (-3001L)
#define SLNETSOCK_ERR_SOCKGETPEERNAME_FAILED                            (-3002L)
#define SLNETSOCK_ERR_SOCKGETLOCALNAME_FAILED                           (-3003L)
#define SLNETSOCK_ERR_SOCKRECV_FAILED                                   (-3004L)
#define SLNETSOCK_ERR_SOCKSEND_FAILED                                   (-3005L)
#define SLNETSOCK_ERR_SOCKSTARTSEC_FAILED                               (-3006L)

/* util related API's from SlNetIf_Config_t failed */
#define SLNETUTIL_ERR_UTILGETHOSTBYNAME_FAILED                          (-3100L)

/*
 * base for util error codes related to SlNetUtil_getAddrInfo and
 * SlNetUtil_gaiStrErr
 */
#define SLNETUTIL_EAI_BASE                                              (-3120L)

/*
 * util error codes related to SlNetUtil_getAddrInfo and SlNetUtil_gaiStrErr
 * The numbering of these codes MUST match the order of the strErrorMsgs array
 * in <ti/net/slnetutils.c>
 */
#define SLNETUTIL_EAI_AGAIN                                             (-3121L)
#define SLNETUTIL_EAI_BADFLAGS                                          (-3122L)
#define SLNETUTIL_EAI_FAIL                                              (-3123L)
#define SLNETUTIL_EAI_FAMILY                                            (-3124L)
#define SLNETUTIL_EAI_MEMORY                                            (-3125L)
#define SLNETUTIL_EAI_NONAME                                            (-3126L)
#define SLNETUTIL_EAI_SERVICE                                           (-3127L)
#define SLNETUTIL_EAI_SOCKTYPE                                          (-3128L)
#define SLNETUTIL_EAI_SYSTEM                                            (-3129L)
#define SLNETUTIL_EAI_OVERFLOW                                          (-3130L)
#define SLNETUTIL_EAI_ADDRFAMILY                                        (-3131L)

/* if related API's from SlNetIf_Config_t failed */
#define SLNETIF_ERR_IFLOADSECOBJ_FAILED                                 (-3200L)
#define SLNETIF_ERR_IFGETIPADDR_FAILED                                  (-3201L)
#define SLNETIF_ERR_IFGETCONNECTIONSTATUS_FAILED                        (-3202L)
#define SLNETIF_ERR_IFCREATECONTEXT_FAILED                              (-3203L)

/*!

 Close the Doxygen group.
 @}

*/

#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif  /*  __SL_NET_ERR_H__ */
