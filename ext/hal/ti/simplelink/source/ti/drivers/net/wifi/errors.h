/*
 * errors.h - CC31xx/CC32xx Host Driver Implementation
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



#ifndef __ERROR_H__
#define __ERROR_H__


#ifdef __cplusplus
extern "C" {
#endif


#define SL_RET_CODE_OK                                  (0L)    /* Success */

#define SL_ERROR_GENERAL_DEVICE                         (-6L)   /* General device error */

/* BSD SOCKET ERRORS CODES */

#define SL_ERROR_BSD_SOC_ERROR                          (-1L)   /* Failure */
#define SL_ERROR_BSD_INEXE                              (-8L)   /* socket command in execution  */
#define SL_ERROR_BSD_EBADF                              (-9L)   /* Bad file number */
#define SL_ERROR_BSD_ENSOCK                             (-10L)  /* The system limit on the total number of open socket, has been reached */
#define SL_ERROR_BSD_EAGAIN                             (-11L)  /* Try again */
#define SL_ERROR_BSD_EWOULDBLOCK                        SL_ERROR_BSD_EAGAIN
#define SL_ERROR_BSD_ENOMEM                             (-12L)  /* Out of memory */
#define SL_ERROR_BSD_EACCES                             (-13L)  /* Permission denied */
#define SL_ERROR_BSD_EFAULT                             (-14L)  /* Bad address */
#define SL_ERROR_BSD_ECLOSE                             (-15L)  /* close socket operation failed to transmit all queued packets */
#define SL_ERROR_BSD_EALREADY_ENABLED                   (-21L)  /* Transceiver - Transceiver already ON. there could be only one */
#define SL_ERROR_BSD_EINVAL                             (-22L)  /* Invalid argument */
#define SL_ERROR_BSD_EAUTO_CONNECT_OR_CONNECTING        (-69L)  /* Transceiver - During connection, connected or auto mode started */
#define SL_ERROR_BSD_CONNECTION_PENDING                 (-72L)  /* Transceiver - Device is connected, disconnect first to open transceiver */
#define SL_ERROR_BSD_EUNSUPPORTED_ROLE                  (-86L)  /* Transceiver - Trying to start when WLAN role is AP or P2P GO */
#define SL_ERROR_BSD_EDESTADDRREQ                       (-89L)  /* Destination address required */
#define SL_ERROR_BSD_EPROTOTYPE                         (-91L)  /* Protocol wrong type for socket */
#define SL_ERROR_BSD_ENOPROTOOPT                        (-92L)  /* Protocol not available */
#define SL_ERROR_BSD_EPROTONOSUPPORT                    (-93L)  /* Protocol not supported */
#define SL_ERROR_BSD_ESOCKTNOSUPPORT                    (-94L)  /* Socket type not supported */
#define SL_ERROR_BSD_EOPNOTSUPP                         (-95L)  /* Operation not supported on transport endpoint */
#define SL_ERROR_BSD_EAFNOSUPPORT                       (-97L)  /* Address family not supported by protocol */
#define SL_ERROR_BSD_EADDRINUSE                         (-98L)  /* Address already in use */
#define SL_ERROR_BSD_EADDRNOTAVAIL                      (-99L)  /* Cannot assign requested address */
#define SL_ERROR_BSD_ENETUNREACH                        (-101L) /* Network is unreachable */
#define SL_ERROR_BSD_ENOBUFS                            (-105L) /* No buffer space available */
#define SL_ERROR_BSD_EOBUFF                             SL_ENOBUFS
#define SL_ERROR_BSD_EISCONN                            (-106L) /* Transport endpoint is already connected */
#define SL_ERROR_BSD_ENOTCONN                           (-107L) /* Transport endpoint is not connected */
#define SL_ERROR_BSD_ETIMEDOUT                          (-110L) /* Connection timed out */
#define SL_ERROR_BSD_ECONNREFUSED                       (-111L) /* Connection refused */
#define SL_ERROR_BSD_EALREADY                           (-114L) /* Non blocking connect in progress, try again */

/* ssl tls security start with -300 offset */
#define SL_ERROR_BSD_ESEC_CLOSE_NOTIFY                  (-300L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_UNEXPECTED_MESSAGE            (-310L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_BAD_RECORD_MAC                (-320L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_DECRYPTION_FAILED             (-321L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_RECORD_OVERFLOW               (-322L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_DECOMPRESSION_FAILURE         (-330L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_HANDSHAKE_FAILURE             (-340L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_NO_CERTIFICATE                (-341L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_BAD_CERTIFICATE               (-342L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_UNSUPPORTED_CERTIFICATE       (-343L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_ILLEGAL_PARAMETER             (-347L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_ACCESS_DENIED                 (-349L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_DECODE_ERROR                  (-350L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_DECRYPT_ERROR1                (-351L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_EXPORT_RESTRICTION            (-360L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_PROTOCOL_VERSION              (-370L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_INSUFFICIENT_SECURITY         (-371L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_INTERNAL_ERROR                (-380L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_USER_CANCELLED                (-390L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_NO_RENEGOTIATION              (-400L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_UNSUPPORTED_EXTENSION         (-410L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_CERTIFICATE_UNOBTAINABLE      (-411L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_UNRECOGNIZED_NAME             (-412L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_BAD_CERTIFICATE_STATUS_RESPONSE (-413L) /* ssl/tls alerts */
#define SL_ERROR_BSD_ESEC_BAD_CERTIFICATE_HASH_VALUE      (-414L) /* ssl/tls alerts */
/* propierty secure */
#define SL_ERROR_BSD_ESECGENERAL                         (-450L)  /* error secure level general error */
#define SL_ERROR_BSD_ESECDECRYPT                         (-451L)  /* error secure level, decrypt recv packet fail */
#define SL_ERROR_BSD_ESECCLOSED                          (-452L)  /* secure layrer is closed by other size , tcp is still connected  */
#define SL_ERROR_BSD_ESECSNOVERIFY                       (-453L)  /* Connected without server verification */
#define SL_ERROR_BSD_ESECNOCAFILE                        (-454L)  /* error secure level CA file not found*/
#define SL_ERROR_BSD_ESECMEMORY                          (-455L)  /* error secure level No memory  space available */
#define SL_ERROR_BSD_ESECBADCAFILE                       (-456L)  /* error secure level bad CA file */
#define SL_ERROR_BSD_ESECBADCERTFILE                     (-457L)  /* error secure level bad Certificate file */
#define SL_ERROR_BSD_ESECBADPRIVATEFILE                  (-458L)  /* error secure level bad private file */
#define SL_ERROR_BSD_ESECBADDHFILE                       (-459L)  /* error secure level bad DH file */
#define SL_ERROR_BSD_ESECT00MANYSSLOPENED                (-460L)  /* MAX SSL Sockets are opened */
#define SL_ERROR_BSD_ESECDATEERROR                       (-461L)  /* connected with certificate date verification error */
#define SL_ERROR_BSD_ESECHANDSHAKETIMEDOUT               (-462L)  /* connection timed out due to handshake time */
#define SL_ERROR_BSD_ESECTXBUFFERNOTEMPTY                (-463L) /* cannot start ssl connection while send buffer is full */
#define SL_ERROR_BSD_ESECRXBUFFERNOTEMPTY                (-464L) /* cannot start ssl connection while recv buffer is full */
#define SL_ERROR_BSD_ESECSSLDURINGHANDSHAKE              (-465L) /* cannot use while in hanshaking */
#define SL_ERROR_BSD_ESECNOTALLOWEDWHENLISTENING         (-466L) /* the operation is not allowed when listening, do before listen*/
#define SL_ERROR_BSD_ESECCERTIFICATEREVOKED              (-467L) /* connected but on of the certificates in the chain is revoked */
#define SL_ERROR_BSD_ESECUNKNOWNROOTCA                   (-468L) /* connected but the root CA used to validate the peer is unknown */
#define SL_ERROR_BSD_ESECWRONGPEERCERT                   (-469L) /* wrong peer cert (server cert) was received while trying to connect to server */
#define SL_ERROR_BSD_ESECTCPDISCONNECTEDUNCOMPLETERECORD (-470L) /* the other side disconnected the TCP layer and didn't send the whole ssl record */

#define SL_ERROR_BSD_ESEC_BUFFER_E                       (-632L)  /* output buffer too small or input too large */
#define SL_ERROR_BSD_ESEC_ALGO_ID_E                      (-633L)  /* setting algo id error */
#define SL_ERROR_BSD_ESEC_PUBLIC_KEY_E                   (-634L)  /* setting public key error */
#define SL_ERROR_BSD_ESEC_DATE_E                         (-635L)  /* setting date validity error */
#define SL_ERROR_BSD_ESEC_SUBJECT_E                      (-636L)  /* setting subject name error */
#define SL_ERROR_BSD_ESEC_ISSUER_E                       (-637L)  /* setting issuer  name error */
#define SL_ERROR_BSD_ESEC_CA_TRUE_E                      (-638L)  /* setting CA basic constraint true error */
#define SL_ERROR_BSD_ESEC_EXTENSIONS_E                   (-639L)  /* setting extensions error */
#define SL_ERROR_BSD_ESEC_ASN_PARSE_E                    (-640L)  /* ASN parsing error, invalid input */
#define SL_ERROR_BSD_ESEC_ASN_VERSION_E                  (-641L)  /* ASN version error, invalid number */
#define SL_ERROR_BSD_ESEC_ASN_GETINT_E                   (-642L)  /* ASN get big int error, invalid data */
#define SL_ERROR_BSD_ESEC_ASN_RSA_KEY_E                  (-643L)  /* ASN key init error, invalid input */
#define SL_ERROR_BSD_ESEC_ASN_OBJECT_ID_E                (-644L)  /* ASN object id error, invalid id */
#define SL_ERROR_BSD_ESEC_ASN_TAG_NULL_E                 (-645L)  /* ASN tag error, not null */
#define SL_ERROR_BSD_ESEC_ASN_EXPECT_0_E                 (-646L)  /* ASN expect error, not zero */
#define SL_ERROR_BSD_ESEC_ASN_BITSTR_E                   (-647L)  /* ASN bit string error, wrong id */
#define SL_ERROR_BSD_ESEC_ASN_UNKNOWN_OID_E              (-648L)  /* ASN oid error, unknown sum id */
#define SL_ERROR_BSD_ESEC_ASN_DATE_SZ_E                  (-649L)  /* ASN date error, bad size */
#define SL_ERROR_BSD_ESEC_ASN_BEFORE_DATE_E              (-650L)  /* ASN date error, current date before */
#define SL_ERROR_BSD_ESEC_ASN_AFTER_DATE_E               (-651L)  /* ASN date error, current date after */
#define SL_ERROR_BSD_ESEC_ASN_SIG_OID_E                  (-652L)  /* ASN signature error, mismatched oid */
#define SL_ERROR_BSD_ESEC_ASN_TIME_E                     (-653L)  /* ASN time error, unknown time type */
#define SL_ERROR_BSD_ESEC_ASN_INPUT_E                    (-654L)  /* ASN input error, not enough data */
#define SL_ERROR_BSD_ESEC_ASN_SIG_CONFIRM_E              (-655L)  /* ASN sig error, confirm failure */
#define SL_ERROR_BSD_ESEC_ASN_SIG_HASH_E                 (-656L)  /* ASN sig error, unsupported hash type */
#define SL_ERROR_BSD_ESEC_ASN_SIG_KEY_E                  (-657L)  /* ASN sig error, unsupported key type */
#define SL_ERROR_BSD_ESEC_ASN_DH_KEY_E                   (-658L)  /* ASN key init error, invalid input */
#define SL_ERROR_BSD_ESEC_ASN_NTRU_KEY_E                 (-659L)  /* ASN ntru key decode error, invalid input */
#define SL_ERROR_BSD_ESEC_ASN_CRIT_EXT_E                 (-660L)  /* ASN unsupported critical extension */
#define SL_ERROR_BSD_ESEC_ECC_BAD_ARG_E                  (-670L)  /* ECC input argument of wrong type */
#define SL_ERROR_BSD_ESEC_ASN_ECC_KEY_E                  (-671L)  /* ASN ECC bad input */
#define SL_ERROR_BSD_ESEC_ECC_CURVE_OID_E                (-672L)  /* Unsupported ECC OID curve type */
#define SL_ERROR_BSD_ESEC_BAD_FUNC_ARG                   (-673L)  /* Bad function argument provided */
#define SL_ERROR_BSD_ESEC_NOT_COMPILED_IN                (-674L)  /* Feature not compiled in */
#define SL_ERROR_BSD_ESEC_UNICODE_SIZE_E                 (-675L)  /* Unicode password too big */
#define SL_ERROR_BSD_ESEC_NO_PASSWORD                    (-676L)  /* no password provided by user */
#define SL_ERROR_BSD_ESEC_ALT_NAME_E                     (-677L)  /* alt name size problem, too big */
#define SL_ERROR_BSD_ESEC_ASN_NO_SIGNER_E                (-688L)  /* ASN no signer to confirm failure */
#define SL_ERROR_BSD_ESEC_ASN_CRL_CONFIRM_E              (-689L)  /* ASN CRL signature confirm failure */
#define SL_ERROR_BSD_ESEC_ASN_CRL_NO_SIGNER_E            (-690L)  /* ASN CRL no signer to confirm failure */
#define SL_ERROR_BSD_ESEC_ASN_OCSP_CONFIRM_E             (-691L)  /* ASN OCSP signature confirm failure */
#define SL_ERROR_BSD_ESEC_VERIFY_FINISHED_ERROR          (-704L)  /* verify problem on finished */
#define SL_ERROR_BSD_ESEC_VERIFY_MAC_ERROR               (-705L)  /* verify mac problem       */
#define SL_ERROR_BSD_ESEC_PARSE_ERROR                    (-706L)  /* parse error on header    */
#define SL_ERROR_BSD_ESEC_UNKNOWN_HANDSHAKE_TYPE         (-707L)  /* weird handshake type     */
#define SL_ERROR_BSD_ESEC_SOCKET_ERROR_E                 (-708L)  /* error state on socket    */
#define SL_ERROR_BSD_ESEC_SOCKET_NODATA                  (-709L)  /* expected data, not there */
#define SL_ERROR_BSD_ESEC_INCOMPLETE_DATA                (-710L)  /* don't have enough data to complete task */
#define SL_ERROR_BSD_ESEC_UNKNOWN_RECORD_TYPE            (-711L)  /* unknown type in record hdr */
#define SL_ERROR_BSD_ESEC_INNER_DECRYPT_ERROR            (-712L)  /* error during decryption  */
#define SL_ERROR_BSD_ESEC_FATAL_ERROR                    (-713L)  /* recvd alert fatal error  */
#define SL_ERROR_BSD_ESEC_ENCRYPT_ERROR                  (-714L)  /* error during encryption  */
#define SL_ERROR_BSD_ESEC_FREAD_ERROR                    (-715L)  /* fread problem            */
#define SL_ERROR_BSD_ESEC_NO_PEER_KEY                    (-716L)  /* need peer's key          */
#define SL_ERROR_BSD_ESEC_NO_PRIVATE_KEY                 (-717L)  /* need the private key     */
#define SL_ERROR_BSD_ESEC_RSA_PRIVATE_ERROR              (-718L)  /* error during rsa priv op */
#define SL_ERROR_BSD_ESEC_NO_DH_PARAMS                   (-719L)  /* server missing DH params */
#define SL_ERROR_BSD_ESEC_BUILD_MSG_ERROR                (-720L)  /* build message failure    */
#define SL_ERROR_BSD_ESEC_BAD_HELLO                      (-721L)  /* client hello malformed   */
#define SL_ERROR_BSD_ESEC_DOMAIN_NAME_MISMATCH           (-722L)  /* peer subject name mismatch */
#define SL_ERROR_BSD_ESEC_WANT_READ                      (-723L)  /* want read, call again    */
#define SL_ERROR_BSD_ESEC_NOT_READY_ERROR                (-724L)  /* handshake layer not ready */
#define SL_ERROR_BSD_ESEC_PMS_VERSION_ERROR              (-725L)  /* pre m secret version error */
#define SL_ERROR_BSD_ESEC_WANT_WRITE                     (-727L)  /* want write, call again   */
#define SL_ERROR_BSD_ESEC_BUFFER_ERROR                   (-728L)  /* malformed buffer input   */
#define SL_ERROR_BSD_ESEC_VERIFY_CERT_ERROR              (-729L)  /* verify cert error        */
#define SL_ERROR_BSD_ESEC_VERIFY_SIGN_ERROR              (-730L)  /* verify sign error        */
#define SL_ERROR_BSD_ESEC_LENGTH_ERROR                   (-741L)  /* record layer length error */
#define SL_ERROR_BSD_ESEC_PEER_KEY_ERROR                 (-742L)  /* can't decode peer key */
#define SL_ERROR_BSD_ESEC_ZERO_RETURN                    (-743L)  /* peer sent close notify */
#define SL_ERROR_BSD_ESEC_SIDE_ERROR                     (-744L)  /* wrong client/server type */
#define SL_ERROR_BSD_ESEC_NO_PEER_CERT                   (-745L)  /* peer didn't send key */
#define SL_ERROR_BSD_ESEC_ECC_CURVETYPE_ERROR            (-750L)  /* Bad ECC Curve Type */
#define SL_ERROR_BSD_ESEC_ECC_CURVE_ERROR                (-751L)  /* Bad ECC Curve */
#define SL_ERROR_BSD_ESEC_ECC_PEERKEY_ERROR              (-752L)  /* Bad Peer ECC Key */
#define SL_ERROR_BSD_ESEC_ECC_MAKEKEY_ERROR              (-753L)  /* Bad Make ECC Key */
#define SL_ERROR_BSD_ESEC_ECC_EXPORT_ERROR               (-754L)  /* Bad ECC Export Key */
#define SL_ERROR_BSD_ESEC_ECC_SHARED_ERROR               (-755L)  /* Bad ECC Shared Secret */
#define SL_ERROR_BSD_ESEC_NOT_CA_ERROR                   (-757L)  /* Not a CA cert error */
#define SL_ERROR_BSD_ESEC_BAD_PATH_ERROR                 (-758L)  /* Bad path for opendir */
#define SL_ERROR_BSD_ESEC_BAD_CERT_MANAGER_ERROR         (-759L)  /* Bad Cert Manager */
#define SL_ERROR_BSD_ESEC_OCSP_CERT_REVOKED              (-760L)  /* OCSP Certificate revoked */
#define SL_ERROR_BSD_ESEC_CRL_CERT_REVOKED               (-761L)  /* CRL Certificate revoked */
#define SL_ERROR_BSD_ESEC_CRL_MISSING                    (-762L)  /* CRL Not loaded */
#define SL_ERROR_BSD_ESEC_MONITOR_RUNNING_E              (-763L)  /* CRL Monitor already running */
#define SL_ERROR_BSD_ESEC_THREAD_CREATE_E                (-764L)  /* Thread Create Error */
#define SL_ERROR_BSD_ESEC_OCSP_NEED_URL                  (-765L)  /* OCSP need an URL for lookup */
#define SL_ERROR_BSD_ESEC_OCSP_CERT_UNKNOWN              (-766L)  /* OCSP responder doesn't know */
#define SL_ERROR_BSD_ESEC_OCSP_LOOKUP_FAIL               (-767L)  /* OCSP lookup not successful */
#define SL_ERROR_BSD_ESEC_MAX_CHAIN_ERROR                (-768L)  /* max chain depth exceeded */
#define SL_ERROR_BSD_ESEC_NO_PEER_VERIFY                 (-778L)  /* Need peer cert verify Error */
#define SL_ERROR_BSD_ESEC_UNSUPPORTED_SUITE              (-790L)  /* unsupported cipher suite */
#define SL_ERROR_BSD_ESEC_MATCH_SUITE_ERROR              (-791L)  /* can't match cipher suite */








/* WLAN ERRORS CODES*/

#define  SL_ERROR_WLAN_KEY_ERROR										(-2049L)
#define  SL_ERROR_WLAN_INVALID_ROLE										(-2050L)
#define  SL_ERROR_WLAN_PREFERRED_NETWORKS_FILE_LOAD_FAILED				(-2051L)
#define  SL_ERROR_WLAN_CANNOT_CONFIG_SCAN_DURING_PROVISIONING       	(-2052L)
#define  SL_ERROR_WLAN_INVALID_SECURITY_TYPE							(-2054L)
#define  SL_ERROR_WLAN_PASSPHRASE_TOO_LONG								(-2055L)
#define  SL_ERROR_WLAN_EAP_WRONG_METHOD									(-2057L)
#define  SL_ERROR_WLAN_PASSWORD_ERROR									(-2058L)
#define  SL_ERROR_WLAN_EAP_ANONYMOUS_LEN_ERROR							(-2059L)
#define  SL_ERROR_WLAN_SSID_LEN_ERROR									(-2060L)
#define  SL_ERROR_WLAN_USER_ID_LEN_ERROR								(-2061L)
#define  SL_ERROR_WLAN_PREFERRED_NETWORK_LIST_FULL						(-2062L)
#define  SL_ERROR_WLAN_PREFERRED_NETWORKS_FILE_WRITE_FAILED				(-2063L)
#define  SL_ERROR_WLAN_ILLEGAL_WEP_KEY_INDEX							(-2064L)
#define  SL_ERROR_WLAN_INVALID_DWELL_TIME_VALUES						(-2065L)
#define  SL_ERROR_WLAN_INVALID_POLICY_TYPE								(-2066L)
#define  SL_ERROR_WLAN_PM_POLICY_INVALID_OPTION							(-2067L)
#define  SL_ERROR_WLAN_PM_POLICY_INVALID_PARAMS							(-2068L)
#define  SL_ERROR_WLAN_WIFI_NOT_CONNECTED								(-2069L)
#define  SL_ERROR_WLAN_ILLEGAL_CHANNEL									(-2070L)
#define  SL_ERROR_WLAN_WIFI_ALREADY_DISCONNECTED						(-2071L)
#define  SL_ERROR_WLAN_TRANSCEIVER_ENABLED								(-2072L)
#define  SL_ERROR_WLAN_GET_NETWORK_LIST_EAGAIN							(-2073L)
#define  SL_ERROR_WLAN_GET_PROFILE_INVALID_INDEX						(-2074L)
#define  SL_ERROR_WLAN_FAST_CONN_DATA_INVALID							(-2075L)
#define  SL_ERROR_WLAN_NO_FREE_PROFILE	         						(-2076L)
#define  SL_ERROR_WLAN_AP_SCAN_INTERVAL_TOO_LOW							(-2077L)
#define  SL_ERROR_WLAN_SCAN_POLICY_INVALID_PARAMS						(-2078L)

#define SL_RXFL_OK                                                      (0L)     /*  O.K */
#define SL_ERROR_RXFL_RANGE_COMPARE_PARAMS_ARE_INVALID					(-2079L)
#define SL_ERROR_RXFL_RXFL_INVALID_PATTERN_LENGTH                       (-2080L) /* requested length for L1/L4 payload matching must not exceed 16 bytes */
#define SL_ERROR_RXFL_ACTION_USER_EVENT_ID_TOO_BIG                      (-2081L) /* user action id for host event must not exceed SL_WLAN_RX_FILTER_MAX_USER_EVENT_ID */
#define SL_ERROR_RXFL_OFFSET_TOO_BIG                                    (-2082L) /* requested offset for L1/L4 payload matching must not exceed 1535 bytes  */
#define SL_ERROR_RXFL_STAT_UNSUPPORTED                                  (-2083L) /* get rx filters statistics not supported */
#define SL_ERROR_RXFL_INVALID_FILTER_ARG_UPDATE                         (-2084L) /* invalid filter args request */
#define SL_ERROR_RXFL_INVALID_SYSTEM_STATE_TRIGGER_FOR_FILTER_TYPE      (-2085L) /* system state not supported for this filter type */
#define SL_ERROR_RXFL_INVALID_FUNC_ID_FOR_FILTER_TYPE                   (-2086L) /* function id not supported for this filter type */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_DO_NOT_EXIST_3                   (-2087L) /* filter parent doesn't exist */
#define SL_ERROR_RXFL_OUTPUT_OR_INPUT_BUFFER_LENGTH_TOO_SMALL           (-2088L) /* ! The output buffer length is smaller than required for that operation */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_SOFTWARE_FILTER_NOT_FIT          (-2089L) /* Node filter can't be child of software filter and vice_versa */
#define SL_ERROR_RXFL_DEPENDENCY_IS_NOT_PERSISTENT                      (-2090L) /*  Dependency filter is not persistent */
#define SL_ERROR_RXFL_RXFL_ALLOCATION_PROBLEM							(-2091L)
#define SL_ERROR_RXFL_SYSTEM_STATE_NOT_SUPPORTED_FOR_THIS_FILTER        (-2092L) /*  System state is not supported */
#define SL_ERROR_RXFL_TRIGGER_USE_REG5_TO_REG8                          (-2093L) /*  Only counters 5 - 8 are allowed, for Tigger */
#define SL_ERROR_RXFL_TRIGGER_USE_REG1_TO_REG4                          (-2094L) /*  Only counters 1 - 4 are allowed, for trigger */
#define SL_ERROR_RXFL_ACTION_USE_REG5_TO_REG8                           (-2095L) /*  Only counters 5 - 8 are allowed, for action */
#define SL_ERROR_RXFL_ACTION_USE_REG1_TO_REG4                           (-2096L) /*  Only counters 1 - 4 are allowed, for action */
#define SL_ERROR_RXFL_FIELD_SUPPORT_ONLY_EQUAL_AND_NOTEQUAL             (-2097L) /*  Rule compare function Id is out of range */
#define SL_ERROR_RXFL_WRONG_MULTICAST_BROADCAST_ADDRESS                 (-2098L) /*  The address should be of type mutlicast or broadcast */
#define SL_ERROR_RXFL_THE_FILTER_IS_NOT_OF_HEADER_TYPE                  (-2099L) /*  The filter should be of header type */
#define SL_ERROR_RXFL_WRONG_COMPARE_FUNC_FOR_BROADCAST_ADDRESS          (-2100L) /*  The compare funcion is not suitable for broadcast address */
#define SL_ERROR_RXFL_WRONG_MULTICAST_ADDRESS                           (-2101L) /*  The address should be of muticast type */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_IS_NOT_PERSISTENT                (-2102L) /*  The dependency filter is not persistent */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_IS_NOT_ENABLED                   (-2103L) /*  The dependency filter is not enabled */
#define SL_ERROR_RXFL_FILTER_HAS_CHILDS                                 (-2104L) /*  The filter has childs and can't be removed */
#define SL_ERROR_RXFL_CHILD_IS_ENABLED                                  (-2105L) /*  Can't disable filter while the child is enabled */
#define SL_ERROR_RXFL_DEPENDENCY_IS_DISABLED                            (-2106L) /*  Can't enable filetr in case its depndency filter is disabled */
#define SL_ERROR_RXFL_MAC_SEND_MATCHDB_FAILED                           (-2107L)
#define SL_ERROR_RXFL_MAC_SEND_ARG_DB_FAILED							(-2108L)
#define SL_ERROR_RXFL_MAC_SEND_NODEDB_FAILED                            (-2109L)
#define SL_ERROR_RXFL_MAC_OPERTATION_RESUME_FAILED						(-2110L)
#define SL_ERROR_RXFL_MAC_OPERTATION_HALT_FAILED						(-2111L)
#define SL_ERROR_RXFL_NUMBER_OF_CONNECTION_POINTS_EXCEEDED              (-2112L) /*  Number of connection points exceeded */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_DEPENDENCY_ACTION_IS_DROP        (-2113L) /*  The dependent filter has Drop action, thus the filter can't be created */
#define SL_ERROR_RXFL_FILTER_DO_NOT_EXISTS                              (-2114L) /*  The filter doesn't exists */
#define SL_ERROR_RXFL_DEPEDENCY_NOT_ON_THE_SAME_LAYER                   (-2115L) /*  The filter and its dependency must be on the same layer */
#define SL_ERROR_RXFL_NUMBER_OF_ARGS_EXCEEDED                           (-2116L) /*  Number of arguments excceded */
#define SL_ERROR_RXFL_ACTION_NO_REG_NUMBER                              (-2117L) /*  Action require counter number */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_LAYER_DO_NOT_FIT                 (-2118L) /*  the filter and its dependency should be from the same layer */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_SYSTEM_STATE_DO_NOT_FIT          (-2119L) /*  The filter and its dependency system state don't fit  */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_DO_NOT_EXIST_2                   (-2120L) /*  The parent filter don't exist  */
#define SL_ERROR_RXFL_DEPENDENT_FILTER_DO_NOT_EXIST_1                   (-2121L) /*  The parent filter is null */
#define SL_ERROR_RXFL_RULE_HEADER_ACTION_TYPE_NOT_SUPPORTED             (-2122L) /*  The action type is not supported */
#define SL_ERROR_RXFL_RULE_HEADER_TRIGGER_COMPARE_FUNC_OUT_OF_RANGE     (-2123L) /*  The Trigger comparision function is out of range */
#define SL_ERROR_RXFL_RULE_HEADER_TRIGGER_OUT_OF_RANGE                  (-2124L) /*  The Trigger is out of range */
#define SL_ERROR_RXFL_RULE_HEADER_COMPARE_FUNC_OUT_OF_RANGE             (-2125L) /*  The rule compare function is out of range */
#define SL_ERROR_RXFL_FRAME_TYPE_NOT_SUPPORTED                          (-2126L) /*  ASCII frame type string is illegal */
#define SL_ERROR_RXFL_RULE_FIELD_ID_NOT_SUPPORTED                       (-2127L) /*  Rule field ID is out of range */
#define SL_ERROR_RXFL_RULE_HEADER_FIELD_ID_ASCII_NOT_SUPPORTED          (-2128L) /*  This ASCII field ID is not supported */
#define SL_ERROR_RXFL_RULE_HEADER_NOT_SUPPORTED                         (-2129L) /*  The header rule is not supported on current release */
#define SL_ERROR_RXFL_RULE_HEADER_OUT_OF_RANGE                          (-2130L) /*  The header rule is out of range */
#define SL_ERROR_RXFL_RULE_HEADER_COMBINATION_OPERATOR_OUT_OF_RANGE     (-2131L) /*  Combination function Id is out of ramge */
#define SL_ERROR_RXFL_RULE_HEADER_FIELD_ID_OUT_OF_RANGE                 (-2132L) /*  rule field Id is out of range */
#define SL_ERROR_RXFL_UPDATE_NOT_SUPPORTED                              (-2133L) /*  Update not supported */
#define SL_ERROR_RXFL_NO_FILTER_DATABASE_ALLOCATE                		(-2134L)
#define SL_ERROR_RXFL_ALLOCATION_FOR_GLOBALS_STRUCTURE_FAILED    		(-2135L)
#define SL_ERROR_RXFL_ALLOCATION_FOR_DB_NODE_FAILED       				(-2136L)
#define SL_ERROR_RXFL_READ_FILE_FILTER_ID_ILLEGAL         				(-2137L)
#define SL_ERROR_RXFL_READ_FILE_NUMBER_OF_FILTER_FAILED   				(-2138L)
#define SL_ERROR_RXFL_READ_FILE_FAILED                    				(-2139L)
#define SL_ERROR_RXFL_NO_FILTERS_ARE_DEFINED              				(-2140L)    /*  No filters are defined in the system */
#define SL_ERROR_RXFL_NUMBER_OF_FILTER_EXCEEDED           				(-2141L)    /*  Number of max filters excceded */
#define SL_ERROR_RXFL_BAD_FILE_MODE                 					(-2142L)
#define SL_ERROR_RXFL_FAILED_READ_NVFILE            					(-2143L)
#define SL_ERROR_RXFL_FAILED_INIT_STORAGE           					(-2144L)
#define SL_ERROR_RXFL_CONTINUE_WRITE_MUST_BE_MOD_4  					(-2145L)
#define SL_ERROR_RXFL_FAILED_LOAD_FILE              					(-2146L)
#define SL_ERROR_RXFL_INVALID_HANDLE                					(-2147L)
#define SL_ERROR_RXFL_FAILED_TO_WRITE               					(-2148L)
#define SL_ERROR_RXFL_OFFSET_OUT_OF_RANGE           					(-2149L)
#define SL_ERROR_RXFL_ALLOC                           					(-2150L)
#define SL_ERROR_RXFL_READ_DATA_LENGTH                					(-2151L)
#define SL_ERROR_RXFL_INVALID_FILE_ID                 					(-2152L)
#define SL_ERROR_RXFL_FILE_FILTERS_NOT_EXISTS                 			(-2153L)
#define SL_ERROR_RXFL_FILE_ALREADY_IN_USE             					(-2154L)
#define SL_ERROR_RXFL_INVALID_ARGS                     		 			(-2155L)
#define SL_ERROR_RXFL_FAILED_TO_CREATE_FILE            					(-2156L)
#define SL_ERROR_RXFL_FS_ALREADY_LOADED                       			(-2157L)
#define SL_ERROR_RXFL_UNKNOWN                                 			(-2158L)
#define SL_ERROR_RXFL_FAILED_TO_CREATE_LOCK_OBJ               			(-2159L)
#define SL_ERROR_RXFL_DEVICE_NOT_LOADED                       			(-2160L)
#define SL_ERROR_RXFL_INVALID_MAGIC_NUM									(-2161L)
#define SL_ERROR_RXFL_FAILED_TO_READ									(-2162L)
#define SL_ERROR_RXFL_NOT_SUPPORTED										(-2163L)
#define SL_ERROR_WLAN_INVALID_COUNTRY_CODE 					    		(-2164L)
#define SL_ERROR_WLAN_NVMEM_ACCESS_FAILED 	    				    	(-2165L)
#define SL_ERROR_WLAN_OLD_FILE_VERSION 	     							(-2166L)
#define SL_ERROR_WLAN_TX_POWER_OUT_OF_RANGE    							(-2167L)
#define SL_ERROR_WLAN_INVALID_AP_PASSWORD_LENGTH					    (-2168L)
#define SL_ERROR_WLAN_PROVISIONING_ABORT_PROVISIONING_ALREADY_STARTED   (-2169L)
#define SL_ERROR_WLAN_PROVISIONING_ABORT_HTTP_SERVER_DISABLED           (-2170L)
#define SL_ERROR_WLAN_PROVISIONING_ABORT_PROFILE_LIST_FULL              (-2171L)
#define SL_ERROR_WLAN_PROVISIONING_ABORT_INVALID_PARAM                  (-2172L)
#define SL_ERROR_WLAN_PROVISIONING_ABORT_GENERAL_ERROR                  (-2173L)
#define SL_ERROR_WLAN_MULTICAST_EXCEED_MAX_ADDR                         (-2174L)
#define SL_ERROR_WLAN_MULTICAST_INVAL_ADDR                              (-2175L)
#define SL_ERROR_WLAN_AP_SCAN_INTERVAL_TOO_SHORT                        (-2176L)
#define SL_ERROR_WLAN_PROVISIONING_CMD_NOT_EXPECTED                     (-2177L)


#define SL_ERROR_WLAN_AP_ACCESS_LIST_NO_ADDRESS_TO_DELETE  (-2178L)     /* List is empty, no address to delete */
#define SL_ERROR_WLAN_AP_ACCESS_LIST_FULL                  (-2179L)     /* access list is full */
#define SL_ERROR_WLAN_AP_ACCESS_LIST_DISABLED              (-2180L)     /* access list is disabled */
#define SL_ERROR_WLAN_AP_ACCESS_LIST_MODE_NOT_SUPPORTED    (-2181L)     /* Trying to switch to unsupported mode */
#define SL_ERROR_WLAN_AP_STA_NOT_FOUND                     (-2182L)     /* trying to disconnect station which is not connected */


/* DEVICE ERRORS CODES*/
#define  SL_ERROR_SUPPLICANT_ERROR					   (-4097L)
#define  SL_ERROR_HOSTAPD_INIT_FAIL					   (-4098L)
#define  SL_ERROR_HOSTAPD_INIT_IF_FAIL				   (-4099L)
#define  SL_ERROR_WLAN_DRV_INIT_FAIL				   (-4100L)
#define  SL_ERROR_FS_FILE_TABLE_LOAD_FAILED            (-4102L)  /* init file system failed   */
#define  SL_ERROR_MDNS_ENABLE_FAIL                     (-4103L)  /* mDNS enable failed        */
#define  SL_ERROR_ROLE_STA_ERR						   (-4107L)   /* Failure to load MAC/PHY in STA role */
#define  SL_ERROR_ROLE_AP_ERR						   (-4108L)   /* Failure to load MAC/PHY in AP role */
#define  SL_ERROR_ROLE_P2P_ERR						   (-4109L)   /* Failure to load MAC/PHY in P2P role */
#define  SL_ERROR_CALIB_FAIL						   (-4110L)   /* Failure of calibration */
#define	 SL_ERROR_FS_CORRUPTED_ERR					   (-4111L)   /* FS is corrupted, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram) */
#define	 SL_ERROR_FS_ALERT_ERR						   (-4112L)   /* Device is locked, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram) */
#define	 SL_ERROR_RESTORE_IMAGE_COMPLETE			   (-4113L)   /* Return to factory image completed, perform reset */
#define	 SL_ERROR_UNKNOWN_ERR						   (-4114L)
#define	 SL_ERROR_GENERAL_ERR						   (-4115L)   /* General error during init */
#define  SL_ERROR_WRONG_ROLE                           (-4116L)
#define  SL_ERROR_INCOMPLETE_PROGRAMMING			   (-4117L)   /* Error during programming, Program new image should be invoked (see sl_FsProgram) */


#define SL_ERROR_PENDING_TXRX_STOP_TIMEOUT_EXP         (-4118L)    /* Timeout expired before completing all TX\RX */
#define SL_ERROR_PENDING_TXRX_NO_TIMEOUT               (-4119L)    /* No Timeout , still have pending  TX\RX */
#define SL_ERROR_INVALID_PERSISTENT_CONFIGURATION      (-4120L)    /* persistency configuration can only be set to 0 (disabled) or 1 (enabled) */

/* NETAPP ERRORS CODES*/
#define SL_ERROR_MDNS_CREATE_FAIL                            (-6145L)   /* mDNS create failed        */
#define SL_ERROR_DEVICE_NAME_LEN_ERR                         (-6146L)   /* Set Dev name error codes  */
#define SL_ERROR_DEVICE_NAME_INVALID                         (-6147L)   /* Set Dev name error codes  */
#define SL_ERROR_DOMAIN_NAME_LEN_ERR                         (-6148L)   /* Set domain name error codes */
#define SL_ERROR_DOMAIN_NAME_INVALID                         (-6149L)   /* Set domain name error codes  */
#define SL_ERROR_NET_APP_DNS_QUERY_NO_RESPONSE               (-6150L)   /* DNS query failed, no response */
#define SL_ERROR_NET_APP_DNS_ERROR                           (-6151L)   /* DNS internal error   */
#define SL_ERROR_NET_APP_DNS_NO_SERVER                       (-6152L)   /* No DNS server was specified  */
#define SL_ERROR_NET_APP_DNS_TIMEOUTR                        (-6153L)   /* mDNS parameters error   */
#define SL_ERROR_NET_APP_DNS_QUERY_FAILED                    (-6154L)   /* DNS query failed; no DNS server sent an 'answer'     */
#define SL_ERROR_NET_APP_DNS_BAD_ADDRESS_ERROR               (-6155L)   /* Improperly formatted IPv4 or IPv6 address  */
#define SL_ERROR_NET_APP_DNS_SIZE_ERROR                      (-6156L)   /* DNS destination size is too small    */
#define SL_ERROR_NET_APP_DNS_MALFORMED_PACKET                (-6157L)   /* Improperly formed or corrupted DNS packet received   */
#define SL_ERROR_NET_APP_DNS_BAD_ID_ERROR                    (-6158L)   /* DNS packet from server does not match query ID */
#define SL_ERROR_NET_APP_DNS_PARAM_ERROR                     (-6159L)   /* Invalid params   */
#define SL_ERROR_NET_APP_DNS_SERVER_NOT_FOUND                (-6160L)   /* Server not found in Client list of DNS servers       */
#define SL_ERROR_NET_APP_DNS_PACKET_CREATE_ERROR			 (-6161L)   /* Error creating DNS packet  */
#define SL_ERROR_NET_APP_DNS_EMPTY_DNS_SERVER_LIST           (-6162L)   /* DNS Client's list of DNS servers is empty            */
#define SL_ERROR_NET_APP_DNS_SERVER_AUTH_ERROR               (-6163L)   /* Server not able to authenticate answer/authority data*/
#define SL_ERROR_NET_APP_DNS_ZERO_GATEWAY_IP_ADDRESS         (-6164L)   /* DNS Client IP instance has a zero gateway IP address */
#define SL_ERROR_NET_APP_DNS_MISMATCHED_RESPONSE             (-6165L)   /* Server response type does not match the query request*/
#define SL_ERROR_NET_APP_DNS_DUPLICATE_ENTRY                 (-6166L)   /* Duplicate entry exists in DNS server table           */
#define SL_ERROR_NET_APP_DNS_RETRY_A_QUERY                   (-6167L)   /* SOA status returned; web site only exists as IPv4    */
#define SL_ERROR_NET_APP_DNS_INVALID_ADDRESS_TYPE            (-6168L)   /* IP address type (e.g. IPv6L) not supported   */
#define SL_ERROR_NET_APP_DNS_IPV6_NOT_SUPPORTED			     (-6169L)   /* IPv6 disabled  */
#define SL_ERROR_NET_APP_DNS_NEED_MORE_RECORD_BUFFER		 (-6170L)   /* The buffer size is not enough.  */
#define SL_ERROR_NET_APP_MDNS_ERROR                          (-6171L)   /* MDNS internal error.                                 */
#define SL_ERROR_NET_APP_MDNS_PARAM_ERROR                    (-6172L)   /* MDNS parameters error.                               */
#define SL_ERROR_NET_APP_MDNS_CACHE_ERROR                    (-6173L)   /* The Cache size is not enough.                        */
#define SL_ERROR_NET_APP_MDNS_UNSUPPORTED_TYPE               (-6174L)	/* The unsupported resource record type.                */
#define SL_ERROR_NET_APP_MDNS_DATA_SIZE_ERROR                (-6175L)   /* The data size is too big.                            */
#define SL_ERROR_NET_APP_MDNS_AUTH_ERROR                     (-6176L)   /* Attempting to parse too large a data.                */
#define SL_ERROR_NET_APP_MDNS_PACKET_ERROR                   (-6177L)   /* The packet can not add the resource record.          */
#define SL_ERROR_NET_APP_MDNS_DEST_ADDRESS_ERROR             (-6178L)   /* The destination address error.                       */
#define SL_ERROR_NET_APP_MDNS_UDP_PORT_ERROR                 (-6179L)   /* The udp port error.                        */
#define SL_ERROR_NET_APP_MDNS_NOT_LOCAL_LINK                 (-6180L)   /* The message that not originate from the local link.  */
#define SL_ERROR_NET_APP_MDNS_EXCEED_MAX_LABEL               (-6181L)   /* The data exceed the max laber size.                  */
#define SL_ERROR_NET_APP_MDNS_EXIST_UNIQUE_RR                (-6182L)   /* At least one Unqiue record in the cache.             */
#define SL_ERROR_NET_APP_MDNS_EXIST_ANSWER                   (-6183L)   /* At least one answer record in the cache.             */
#define SL_ERROR_NET_APP_MDNS_EXIST_SAME_QUERY               (-6184L)   /* Exist the same query.   */
#define SL_ERROR_NET_APP_MDNS_DUPLICATE_SERVICE              (-6185L)   /* Duplicate service.                                   */
#define SL_ERROR_NET_APP_MDNS_NO_ANSWER                      (-6186L)   /* No response for one-shot query.                      */
#define SL_ERROR_NET_APP_MDNS_NO_KNOWN_ANSWER                (-6187L)   /* No known answer for query.                           */
#define SL_ERROR_NET_APP_MDNS_NAME_MISMATCH                  (-6188L)   /* The name mismatch.                                   */
#define SL_ERROR_NET_APP_MDNS_NOT_STARTED                    (-6189L)   /* MDNS does not start.                                 */
#define SL_ERROR_NET_APP_MDNS_HOST_NAME_ERROR                (-6190L)   /* MDNS host name error.                                */
#define SL_ERROR_NET_APP_MDNS_NO_MORE_ENTRIES                (-6191L)   /* No more entries be found.                            */
#define SL_ERROR_NET_APP_MDNS_SERVICE_TYPE_MISMATCH          (-6192L)   /* The service type mismatch                            */
#define SL_ERROR_NET_APP_MDNS_LOOKUP_INDEX_ERROR             (-6193L)   /* Index is bigger than number of services.             */
#define SL_ERROR_NET_APP_MDNS_MAX_SERVICES_ERROR             (-6194L)
#define SL_ERROR_NET_APP_MDNS_IDENTICAL_SERVICES_ERROR       (-6195L)
#define SL_ERROR_NET_APP_MDNS_EXISTED_SERVICE_ERROR          (-6196L)
#define SL_ERROR_NET_APP_MDNS_ERROR_SERVICE_NAME_ERROR       (-6197L)
#define SL_ERROR_NET_APP_MDNS_RX_PACKET_ALLOCATION_ERROR     (-6198L)
#define SL_ERROR_NET_APP_MDNS_BUFFER_SIZE_ERROR              (-6199L)
#define SL_ERROR_NET_APP_MDNS_NET_APP_SET_ERROR              (-6200L)
#define SL_ERROR_NET_APP_MDNS_GET_SERVICE_LIST_FLAG_ERROR    (-6201L)
#define SL_ERROR_NET_APP_MDNS_MDNS_NO_CONFIGURATION_ERROR    (-6202L)
#define SL_ERROR_NET_APP_MDNS_STATUS_ERROR                   (-6203L)
#define SL_ERROR_NET_APP_ENOBUFS                             (-6204L)
#define SL_ERROR_NET_APP_DNS_IPV6_REQ_BUT_IPV6_DISABLED      (-6205L)    /* trying to issue ipv6 DNS request but ipv6 is disabled */
#define SL_ERROR_NET_APP_DNS_INVALID_FAMILY_TYPE             (-6206L)    /* Family type is not ipv4 and not ipv6 */
#define SL_ERROR_NET_APP_DNS_REQ_TOO_BIG                     (-6207L)    /* DNS request size is too big */
#define SL_ERROR_NET_APP_DNS_ALLOC_ERROR                     (-6208L)    /* Allocation error */
#define SL_ERROR_NET_APP_DNS_EXECUTION_ERROR                 (-6209L)    /* Execution error */
#define SL_ERROR_NET_APP_P2P_ROLE_IS_NOT_CONFIGURED          (-6210L)    /* role p2p is not configured yet, should be CL or GO in order to execute command */
#define SL_ERROR_NET_APP_INCORECT_ROLE_FOR_APP               (-6211L)    /* incorrect role for specific application */
#define SL_ERROR_NET_APP_INCORECT_APP_MASK                   (-6212L)    /* mask does not match any app */
#define SL_ERROR_NET_APP_MDNS_ALREADY_STARTED                (-6213L)    /* mdns application already started */
#define SL_ERROR_NET_APP_HTTP_SERVER_ALREADY_STARTED         (-6214L)    /* http server application already started */

#define SL_ERROR_NET_APP_HTTP_GENERAL_ERROR                  (-6216L)    /* New error - Http handle request failed */
#define SL_ERROR_NET_APP_HTTP_INVALID_TIMEOUT                (-6217L)    /* New error - Http timeout invalid argument */
#define SL_ERROR_NET_APP_INVALID_URN_LENGTH                  (-6218L)    /* invalid URN length  */
#define SL_ERROR_NET_APP_RX_BUFFER_LENGTH                    (-6219L)    /* size of the requested services is smaller than size of the user buffer */

/* NETCFG ERRORS CODES*/
#define SL_ERROR_STATIC_ADDR_SUBNET_ERROR					 (-8193L)
#define SL_ERROR_INCORRECT_IPV6_STATIC_LOCAL_ADDR            (-8194L) /* Ipv6 Local address perfix is wrong */
#define SL_ERROR_INCORRECT_IPV6_STATIC_GLOBAL_ADDR  		 (-8195L) /* Ipv6 Global address perfix is wrong */
#define SL_ERROR_IPV6_LOCAL_ADDR_SHOULD_BE_SET_FIRST	 	 (-8195L) /* Attempt to set ipv6 global address before ipv6 local address is set */


/* FS ERRORS CODES*/
#define SL_FS_OK                                                    (0L)
#define SL_ERROR_FS_EXTRACTION_WILL_START_AFTER_RESET			  (-10241L)
#define	SL_ERROR_FS_NO_CERTIFICATE_STORE						  (-10242L)
#define	SL_ERROR_FS_IMAGE_SHOULD_BE_AUTHENTICATE				  (-10243L)
#define	SL_ERROR_FS_IMAGE_SHOULD_BE_ENCRYPTED					  (-10244L)
#define	SL_ERROR_FS_IMAGE_CANT_BE_ENCRYPTED					      (-10245L)
#define	SL_ERROR_FS_DEVELOPMENT_BOARD_WRONG_MAC					  (-10246L)
#define	SL_ERROR_FS_DEVICE_NOT_SECURED							  (-10247L)
#define	SL_ERROR_FS_SYSTEM_FILE_ACCESS_DENIED					  (-10248L)
#define SL_ERROR_FS_IMAGE_EXTRACT_EXPECTING_USER_KEY              (-10249L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_CLOSE_FILE            (-10250L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_WRITE_FILE            (-10251L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_OPEN_FILE             (-10252L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_GET_IMAGE_HEADER      (-10253L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_GET_IMAGE_INFO        (-10254L)
#define SL_ERROR_FS_IMAGE_EXTRACT_SET_ID_NOT_EXIST                (-10255L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_DELETE_FILE           (-10256L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_FORMAT_FS             (-10257L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_LOAD_FS               (-10258L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_GET_DEV_INFO          (-10259L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_DELETE_STORAGE        (-10260L)
#define SL_ERROR_FS_IMAGE_EXTRACT_INCORRECT_IMAGE_LOCATION        (-10261L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_CREATE_IMAGE_FILE     (-10262L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_INIT                  (-10263L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_LOAD_FILE_TABLE       (-10264L)
#define SL_ERROR_FS_IMAGE_EXTRACT_ILLEGAL_COMMAND                 (-10266L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_WRITE_FAT             (-10267L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_RET_FACTORY_DEFAULT   (-10268L)
#define SL_ERROR_FS_IMAGE_EXTRACT_FAILED_TO_READ_NV               (-10269L)
#define SL_ERROR_FS_PROGRAMMING_IMAGE_NOT_EXISTS                  (-10270L)
#define SL_ERROR_FS_PROGRAMMING_IN_PROCESS                        (-10271L)
#define SL_ERROR_FS_PROGRAMMING_ALREADY_STARTED                   (-10272L)
#define SL_ERROR_FS_CERT_IN_THE_CHAIN_REVOKED_SECURITY_ALERT      (-10273L)
#define SL_ERROR_FS_INIT_CERTIFICATE_STORE						  (-10274L)
#define SL_ERROR_FS_PROGRAMMING_ILLEGAL_FILE                      (-10275L)
#define SL_ERROR_FS_PROGRAMMING_NOT_STARTED                       (-10276L)
#define SL_ERROR_FS_IMAGE_EXTRACT_NO_FILE_SYSTEM                  (-10277L)
#define SL_ERROR_FS_WRONG_INPUT_SIZE                              (-10278L)
#define SL_ERROR_FS_BUNDLE_FILE_SHOULD_BE_CREATED_WITH_FAILSAFE   (-10279L)
#define SL_ERROR_FS_BUNDLE_NOT_CONTAIN_FILES                      (-10280L)
#define SL_ERROR_FS_BUNDLE_ALREADY_IN_STATE                       (-10281L)
#define SL_ERROR_FS_BUNDLE_NOT_IN_CORRECT_STATE                   (-10282L)
#define SL_ERROR_FS_BUNDLE_FILES_ARE_OPENED						  (-10283L)
#define SL_ERROR_FS_INCORRECT_FILE_STATE_FOR_OPERATION            (-10284L)
#define SL_ERROR_FS_EMPTY_SFLASH								  (-10285L)
#define SL_ERROR_FS_FILE_IS_NOT_SECURE_AND_SIGN					  (-10286L)
#define SL_ERROR_FS_ROOT_CA_IS_UNKOWN							  (-10287L)
#define SL_ERROR_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY             (-10288L)
#define SL_ERROR_FS_WRONG_SIGNATURE_SECURITY_ALERT                (-10289L)
#define SL_ERROR_FS_WRONG_SIGNATURE_OR_CERTIFIC_NAME_LENGTH       (-10290L)
#define SL_ERROR_FS_NOT_16_ALIGNED                                (-10291L)
#define SL_ERROR_FS_CERT_CHAIN_ERROR_SECURITY_ALERT               (-10292L)
#define SL_ERROR_FS_FILE_NAME_EXIST                               (-10293L)
#define SL_ERROR_FS_EXTENDED_BUF_ALREADY_ALLOC					  (-10294L)
#define SL_ERROR_FS_FILE_SYSTEM_NOT_SECURED                       (-10295L)
#define SL_ERROR_FS_OFFSET_NOT_16_BYTE_ALIGN					  (-10296L)
#define SL_ERROR_FS_FAILED_READ_NVMEM							  (-10297L)
#define SL_ERROR_FS_WRONG_FILE_NAME                               (-10298L)
#define SL_ERROR_FS_FILE_SYSTEM_IS_LOCKED                         (-10299L)
#define SL_ERROR_FS_SECURITY_ALERT                                (-10300L)
#define SL_ERROR_FS_FILE_INVALID_FILE_SIZE                        (-10301L)
#define SL_ERROR_FS_INVALID_TOKEN								  (-10302L)
#define SL_ERROR_FS_NO_DEVICE_IS_LOADED                           (-10303L)
#define SL_ERROR_FS_SECURE_CONTENT_INTEGRITY_FAILURE              (-10304L)
#define SL_ERROR_FS_SECURE_CONTENT_RETRIVE_ASYMETRIC_KEY_ERROR    (-10305L)
#define SL_ERROR_FS_OVERLAP_DETECTION_THRESHHOLD                  (-10306L)
#define SL_ERROR_FS_FILE_HAS_RESERVED_NV_INDEX                    (-10307L)
#define SL_ERROR_FS_FILE_MAX_SIZE_EXCEEDED                        (-10310L)
#define SL_ERROR_FS_INVALID_READ_BUFFER                           (-10311L)
#define SL_ERROR_FS_INVALID_WRITE_BUFFER                          (-10312L)
#define SL_ERROR_FS_FILE_IMAGE_IS_CORRUPTED						  (-10313L)
#define SL_ERROR_FS_SIZE_OF_FILE_EXT_EXCEEDED					  (-10314L)
#define SL_ERROR_FS_WARNING_FILE_NAME_NOT_KEPT                    (-10315L)
#define SL_ERROR_FS_MAX_OPENED_FILE_EXCEEDED                      (-10316L)
#define SL_ERROR_FS_FAILED_WRITE_NVMEM_HEADER                     (-10317L)
#define SL_ERROR_FS_NO_AVAILABLE_NV_INDEX                         (-10318L)
#define SL_ERROR_FS_FAILED_TO_ALLOCATE_MEM					      (-10319L)
#define SL_ERROR_FS_OPERATION_BLOCKED_BY_VENDOR                   (-10320L)
#define SL_ERROR_FS_FAILED_TO_READ_NVMEM_FILE_SYSTEM              (-10321L)
#define SL_ERROR_FS_NOT_ENOUGH_STORAGE_SPACE                      (-10322L)
#define SL_ERROR_FS_INIT_WAS_NOT_CALLED                           (-10323L)
#define SL_ERROR_FS_FILE_SYSTEM_IS_BUSY						      (-10324L)
#define SL_ERROR_FS_INVALID_ACCESS_TYPE					   	      (-10325L)
#define SL_ERROR_FS_FILE_ALREADY_EXISTS							  (-10326L)
#define SL_ERROR_FS_PROGRAM_FAILURE								  (-10327L)
#define SL_ERROR_FS_NO_ENTRIES_AVAILABLE						  (-10328L)
#define SL_ERROR_FS_FILE_ACCESS_IS_DIFFERENT					  (-10329L)
#define SL_ERROR_FS_UNVALID_FILE_MODE						  	  (-10330L)
#define SL_ERROR_FS_FAILED_READ_NVFILE							  (-10331L)
#define SL_ERROR_FS_FAILED_INIT_STORAGE							  (-10332L)
#define SL_ERROR_FS_FILE_HAS_NO_FAILSAFE						  (-10333L)
#define SL_ERROR_FS_NO_VALID_COPY_EXISTS                          (-10334L)
#define SL_ERROR_FS_INVALID_HANDLE								  (-10335L)
#define SL_ERROR_FS_FAILED_TO_WRITE								  (-10336L)
#define SL_ERROR_FS_OFFSET_OUT_OF_RANGE							  (-10337L)
#define SL_ERROR_FS_NO_MEMORY                                     (-10338L)
#define SL_ERROR_FS_INVALID_LENGTH_FOR_READ						  (-10339L)
#define SL_ERROR_FS_WRONG_FILE_OPEN_FLAGS						  (-10340L)
#define SL_ERROR_FS_FILE_NOT_EXISTS								  (-10341L)
#define SL_ERROR_FS_IGNORE_COMMIT_ROLLBAC_FLAG                    (-10342L) /* commit rollback flag is not supported upon creation */
#define SL_ERROR_FS_INVALID_ARGS								  (-10343L)
#define SL_ERROR_FS_FILE_IS_PENDING_COMMIT						  (-10344L)
#define SL_ERROR_FS_SECURE_CONTENT_SESSION_ALREADY_EXIST          (-10345L)
#define SL_ERROR_FS_UNKNOWN										  (-10346L)
#define SL_ERROR_FS_FILE_NAME_RESERVED                            (-10347L)
#define SL_ERROR_FS_NO_FILE_SYSTEM                                (-10348L)
#define SL_ERROR_FS_INVALID_MAGIC_NUM							  (-10349L)
#define SL_ERROR_FS_FAILED_TO_READ_NVMEM						  (-10350L)
#define SL_ERROR_FS_NOT_SUPPORTED								  (-10351L)
#define	SL_ERROR_FS_JTAG_IS_OPENED_NO_FORMAT_TO_PRDUCTION	  	  (-10352L)
#define	SL_ERROR_FS_CONFIG_FILE_RET_READ_FAILED           		  (-10353L)
#define	SL_ERROR_FS_CONFIG_FILE_CHECSUM_ERROR_SECURITY_ALERT      (-10354L)
#define	SL_ERROR_FS_CONFIG_FILE_NO_SUCH_FILE              		  (-10355L)
#define	SL_ERROR_FS_CONFIG_FILE_MEMORY_ALLOCATION_FAILED   		  (-10356L)
#define SL_ERROR_FS_IMAGE_HEADER_READ_FAILED					  (-10357L)
#define SL_ERROR_FS_CERT_STORE_DOWNGRADE                          (-10358L)
#define SL_ERROR_FS_PROGRAMMING_IMAGE_NOT_VALID   			      (-10359L)
#define SL_ERROR_FS_PROGRAMMING_IMAGE_NOT_VERIFIED                (-10360L)
#define SL_ERROR_FS_RESERVE_SIZE_IS_SMALLER						  (-10361L)
#define SL_ERROR_FS_WRONG_ALLOCATION_TABLE						  (-10362L)
#define SL_ERROR_FS_ILLEGAL_SIGNATURE                             (-10363L)
#define SL_ERROR_FS_FILE_ALREADY_OPENED_IN_PENDING_STATE          (-10364L)
#define SL_ERROR_FS_INVALID_TOKEN_SECURITY_ALERT				  (-10365L)
#define SL_ERROR_FS_NOT_SECURE									  (-10366L)
#define SL_ERROR_FS_RESET_DURING_PROGRAMMING                      (-10367L)
#define SL_ERROR_FS_CONFIG_FILE_RET_WRITE_FAILED                  (-10368L)
#define SL_ERROR_FS_FILE_IS_ALREADY_OPENED                        (-10369L)
#define SL_ERROR_FS_FILE_IS_OPEN_FOR_WRITE                        (-10370L)
#define SL_ERROR_FS_ALERT_CANT_BE_SET_ON_NON_SECURE_DEVICE        (-10371L) /* Alerts can be configured on non-secure device. */
#define SL_ERROR_FS_WRONG_CERTIFICATE_FILE_NAME                   (-10372L)



/* NETUTIL ERRORS CODES */
#define SL_ERROR_NETUTIL_CRYPTO_GENERAL                   (-12289L)
#define SL_ERROR_NETUTIL_CRYPTO_INVALID_INDEX             (-12290L)
#define SL_ERROR_NETUTIL_CRYPTO_INVALID_PARAM             (-12291L)
#define SL_ERROR_NETUTIL_CRYPTO_MEM_ALLOC                 (-12292L)
#define SL_ERROR_NETUTIL_CRYPTO_INVALID_DB_VER            (-12293L)
#define SL_ERROR_NETUTIL_CRYPTO_UNSUPPORTED_OPTION        (-12294L)
#define SL_ERROR_NETUTIL_CRYPTO_BUFFER_TOO_SMALL          (-12295L)
#define SL_ERROR_NETUTIL_CRYPTO_EMPTY_DB_ENTRY            (-12296L)
#define SL_ERROR_NETUTIL_CRYPTO_NON_TEMPORARY_KEY         (-12297L)
#define SL_ERROR_NETUTIL_CRYPTO_DB_ENTRY_NOT_FREE         (-12298L)
#define SL_ERROR_NETUTIL_CRYPTO_CORRUPTED_DB_FILE         (-12299L)


/* GENERAL ERRORS CODES*/
#define SL_ERROR_INVALID_OPCODE 			(-14337L)
#define SL_ERROR_INVALID_PARAM				(-14338L)
#define SL_ERROR_STATUS_ERROR				(-14341L)
#define SL_ERROR_NVMEM_ACCESS_FAILED   	 	(-14342L)
#define SL_ERROR_NOT_ALLOWED_NWP_LOCKED     (-14343L) /* Device is locked, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram) */

/* SECURITY ERRORS CODE */
#define SL_ERROR_LOADING_CERTIFICATE_STORE       (-28673L)

/* Device is Locked! Return to Factory Image or Program new
   image should be invoked (see sl_FsCtl, sl_FsProgram) */
#define SL_ERROR_DEVICE_LOCKED_SECURITY_ALERT	 (-28674L)



/* INTERNAL HOST ERRORS CODES*/

/* Receive this error in case there are no resources to issue the command
   If possible, increase the number of MAX_CONCURRENT_ACTIONS (result in memory increaseL)
   If not, try again later */
#define SL_POOL_IS_EMPTY (-2000L)

/* Receive this error in case a given length for RX buffer was too small.
   Receive payload was bigger than the given buffer size. Therefore, payload is cut according to receive size
   Recommend to increase buffer size */
#define SL_ESMALLBUF     (-2001L)

/* Receive this error in case zero length is supplied to a "get" API
   Recommend to supply length according to requested information (view options defines for helpL) */
#define SL_EZEROLEN      (-2002L)

/* User supplied invalid parameter */
#define SL_INVALPARAM    (-2003L)

/* Failed to open interface  */
#define SL_BAD_INTERFACE    (-2004L)

/* API has been aborted due to an error detected by host driver */
#define SL_API_ABORTED      (-2005)

/* Parameters are invalid */
#define SL_RET_CODE_INVALID_INPUT         (-2006L)

/* Driver internal error */
#define SL_RET_CODE_SELF_ERROR            (-2007L)

/* NWP internal error */
#define SL_RET_CODE_NWP_IF_ERROR          (-2008L)

/* malloc error */
#define SL_RET_CODE_MALLOC_ERROR          (-2009L)

/* protocol error */
#define SL_RET_CODE_PROTOCOL_ERROR        (-2010L)

/* API has been aborted, command is not allowed in device lock state */
#define SL_RET_CODE_DEV_LOCKED            (-2011L)

/* sl_Start cannot be invoked twice */
#define SL_RET_CODE_DEV_ALREADY_STARTED   (-2012L)

/* SL API is in progress */
#define SL_RET_CODE_API_COMMAND_IN_PROGRESS   (-2013L)

/* Provisionins is in progress -  */
#define SL_RET_CODE_PROVISIONING_IN_PROGRESS   (-2014L)

/* Wrong ping parameters - ping cannot be called with the following parameters: 
1. infinite ping packet
2. report only when finished 
3. no callback supplied  */
#define SL_RET_CODE_NET_APP_PING_INVALID_PARAMS   (-2015L)

/* SL select already in progress.
   this error will be returned if app will try to call
   sl_select blocking when there is already select trigger in progress */
#define SL_RET_CODE_SOCKET_SELECT_IN_PROGRESS_ERROR  (-2016L)

#define SL_RET_CODE_STOP_IN_PROGRESS  (-2017L)

/* The device has not been started yet  */
#define SL_RET_CODE_DEV_NOT_STARTED   (-2018L)

/* The event link was not found in the list */
#define SL_RET_CODE_EVENT_LINK_NOT_FOUND (-2019L)

#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif  /*  __ERROR_H__ */
