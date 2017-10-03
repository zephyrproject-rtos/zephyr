/*
 * wlan.h - CC31xx/CC32xx Host Driver Implementation
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
#include <ti/drivers/net/wifi/simplelink.h>

#ifndef __WLAN_H__
#define __WLAN_H__


#ifdef    __cplusplus
extern "C" {
#endif


/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/
/*!
	\defgroup Wlan 
	\short Controls the use of the WiFi WLAN module

*/
/*!

    \addtogroup Wlan
    - Connection features, such as: profiles, policies, SmartConfig(tm)
    - Advanced WLAN features, such as: scans, rx filters and rx statistics collection

    @{

*/

#define SL_WLAN_BSSID_LENGTH                    (6)
#define SL_WLAN_SSID_MAX_LENGTH                 (32)

#define SL_WLAN_NUM_OF_RATE_INDEXES             (20)
#define SL_WLAN_SIZE_OF_RSSI_HISTOGRAM          (6)
#define SL_WLAN_SMART_CONFIG_KEY_LENGTH         (16)
#define SL_WLAN_SMART_CONFIG_DEFAULT_CIPHER     (1)
#define SL_WLAN_SMART_CONFIG_DEFAULT_GROUP      (0)

#define SL_WLAN_MAX_PROFILES					(7)
#define SL_WLAN_DEL_ALL_PROFILES	            (255)

typedef enum
{
    SL_WLAN_P2P_WPS_METHOD_DEFAULT,
    SL_WLAN_P2P_WPS_METHOD_PIN_USER,
    SL_WLAN_P2P_WPS_METHOD_PIN_MACHINE,
    SL_WLAN_P2P_WPS_METHOD_REKEY,
    SL_WLAN_P2P_WPS_METHOD_PBC,
    SL_WLAN_P2P_WPS_METHOD_REGISTRAR
} SlWlanP2PWpsMethod_e;

/* WLAN user events */
typedef enum
{
    SL_WLAN_EVENT_CONNECT = 1,
    SL_WLAN_EVENT_DISCONNECT,
    SL_WLAN_EVENT_STA_ADDED,
    SL_WLAN_EVENT_STA_REMOVED,

    SL_WLAN_EVENT_P2P_CONNECT,
    SL_WLAN_EVENT_P2P_DISCONNECT,
    SL_WLAN_EVENT_P2P_CLIENT_ADDED,
    SL_WLAN_EVENT_P2P_CLIENT_REMOVED,
    SL_WLAN_EVENT_P2P_DEVFOUND,
    SL_WLAN_EVENT_P2P_REQUEST,
    SL_WLAN_EVENT_P2P_CONNECTFAIL,

    SL_WLAN_EVENT_RXFILTER,
    SL_WLAN_EVENT_PROVISIONING_STATUS,
    SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED,
    SL_WLAN_EVENT_MAX

} SlWlanEventId_e;


/* WLAN Disconnect Reason Codes */
#define  SL_WLAN_DISCONNECT_UNSPECIFIED                                         (1)
#define  SL_WLAN_DISCONNECT_AUTH_NO_LONGER_VALID                                (2)
#define  SL_WLAN_DISCONNECT_DEAUTH_SENDING_STA_LEAVING                          (3)
#define  SL_WLAN_DISCONNECT_INACTIVITY                                          (4)
#define  SL_WLAN_DISCONNECT_TOO_MANY_STA                                        (5)
#define  SL_WLAN_DISCONNECT_FRAME_FROM_NONAUTH_STA                              (6)
#define  SL_WLAN_DISCONNECT_FRAME_FROM_NONASSOC_STA                             (7)
#define  SL_WLAN_DISCONNECT_DISS_SENDING_STA_LEAVING                            (8)
#define  SL_WLAN_DISCONNECT_STA_NOT_AUTH                                        (9)
#define  SL_WLAN_DISCONNECT_POWER_CAPABILITY_INVALID                            (10)
#define  SL_WLAN_DISCONNECT_SUPPORTED_CHANNELS_INVALID                          (11)
#define  SL_WLAN_DISCONNECT_INVALID_IE                                          (13)
#define  SL_WLAN_DISCONNECT_MIC_FAILURE                                         (14)
#define  SL_WLAN_DISCONNECT_FOURWAY_HANDSHAKE_TIMEOUT                           (15)
#define  SL_WLAN_DISCONNECT_GROUPKEY_HANDSHAKE_TIMEOUT                          (16)
#define  SL_WLAN_DISCONNECT_REASSOC_INVALID_IE                                  (17)
#define  SL_WLAN_DISCONNECT_INVALID_GROUP_CIPHER                                (18)
#define  SL_WLAN_DISCONNECT_INVALID_PAIRWISE_CIPHER                             (19)
#define  SL_WLAN_DISCONNECT_INVALID_AKMP                                        (20)
#define  SL_WLAN_DISCONNECT_UNSUPPORTED_RSN_VERSION                             (21)
#define  SL_WLAN_DISCONNECT_INVALID_RSN_CAPABILITIES                            (22)
#define  SL_WLAN_DISCONNECT_IEEE_802_1X_AUTHENTICATION_FAILED                   (23)
#define  SL_WLAN_DISCONNECT_CIPHER_SUITE_REJECTED                               (24)
#define  SL_WLAN_DISCONNECT_DISASSOC_QOS                                        (32)
#define  SL_WLAN_DISCONNECT_DISASSOC_QOS_BANDWIDTH                              (33)
#define  SL_WLAN_DISCONNECT_DISASSOC_EXCESSIVE_ACK_PENDING                      (34)
#define  SL_WLAN_DISCONNECT_DISASSOC_TXOP_LIMIT                                 (35)
#define  SL_WLAN_DISCONNECT_STA_LEAVING                                         (36)
#define  SL_WLAN_DISCONNECT_STA_DECLINED                                        (37)
#define  SL_WLAN_DISCONNECT_STA_UNKNOWN_BA                                      (38)
#define  SL_WLAN_DISCONNECT_STA_TIMEOUT                                         (39)
#define  SL_WLAN_DISCONNECT_STA_UNSUPPORTED_CIPHER_SUITE                        (40)
#define  SL_WLAN_DISCONNECT_USER_INITIATED                                      (200)
#define  SL_WLAN_DISCONNECT_AUTH_TIMEOUT                                        (202)
#define  SL_WLAN_DISCONNECT_ASSOC_TIMEOUT                                       (203)
#define  SL_WLAN_DISCONNECT_SECURITY_FAILURE                                    (204)
#define  SL_WLAN_DISCONNECT_WHILE_CONNNECTING                                   (208)
#define  SL_WLAN_DISCONNECT_MISSING_CERT                                        (209)
#define  SL_WLAN_DISCONNECT_CERTIFICATE_EXPIRED                                 (210)



#define  SL_WLAN_STATUS_DISCONNECTED    (0)
#define  SL_WLAN_STATUS_SCANING         (1)
#define  SL_WLAN_STATUS_CONNECTING      (2)
#define  SL_WLAN_STATUS_CONNECTED       (3)

#define SL_WLAN_PROVISIONING_GENERAL_ERROR                                                                   (0)
#define SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_NETWORK_NOT_FOUND                                      (1)
#define SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_CONNECTION_FAILED                                      (2)
#define SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_CONNECTION_SUCCESS_IP_NOT_ACQUIRED                          (3)
#define SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS_FEEDBACK_FAILED                                     (4)
#define SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS                                                     (5)
#define SL_WLAN_PROVISIONING_ERROR_ABORT                                                                     (6)
#define SL_WLAN_PROVISIONING_ERROR_ABORT_INVALID_PARAM                                                       (7)
#define SL_WLAN_PROVISIONING_ERROR_ABORT_HTTP_SERVER_DISABLED                                                (8)
#define SL_WLAN_PROVISIONING_ERROR_ABORT_PROFILE_LIST_FULL                                                   (9)
#define SL_WLAN_PROVISIONING_ERROR_ABORT_PROVISIONING_ALREADY_STARTED                                        (10)
#define SL_WLAN_PROVISIONING_AUTO_STARTED                                                                    (11)
#define SL_WLAN_PROVISIONING_STOPPED                                                                         (12)
#define SL_WLAN_PROVISIONING_SMART_CONFIG_SYNCED                                                             (13)
#define SL_WLAN_PROVISIONING_SMART_CONFIG_SYNC_TIMEOUT                                                       (14)
#define SL_WLAN_PROVISIONING_CONFIRMATION_WLAN_CONNECT                                                       (15)
#define SL_WLAN_PROVISIONING_CONFIRMATION_IP_ACQUIRED                                                        (16)
#define SL_WLAN_PROVISIONING_EXTERNAL_CONFIGURATION_READY                                                    (17)



#define SL_WLAN_SEC_TYPE_OPEN                                                                                (0)
#define SL_WLAN_SEC_TYPE_WEP                                                                                 (1)
#define SL_WLAN_SEC_TYPE_WPA                                                                                 (2) /* deprecated */
#define SL_WLAN_SEC_TYPE_WPA_WPA2                                                                            (2)
#define SL_WLAN_SEC_TYPE_WPS_PBC                                                                             (3)
#define SL_WLAN_SEC_TYPE_WPS_PIN                                                                             (4)
#define SL_WLAN_SEC_TYPE_WPA_ENT                                                                             (5)
#define SL_WLAN_SEC_TYPE_P2P_PBC                                                                             (6)
#define SL_WLAN_SEC_TYPE_P2P_PIN_KEYPAD                                                                      (7)
#define SL_WLAN_SEC_TYPE_P2P_PIN_DISPLAY                                                                     (8)
#define SL_WLAN_SEC_TYPE_P2P_PIN_AUTO                                                                        (9) /* NOT Supported yet */
#define SL_WLAN_SEC_TYPE_WEP_SHARED                                                                          (10)



#define SL_TLS                                      (0x1)
#define SL_MSCHAP                                   (0x0)
#define SL_PSK                                      (0x2)
#define SL_TTLS                                     (0x10)
#define SL_PEAP0                                    (0x20)
#define SL_PEAP1                                    (0x40)
#define SL_FAST                                     (0x80)


#define SL_WLAN_FAST_AUTH_PROVISIONING                   (0x02)
#define SL_WLAN_FAST_UNAUTH_PROVISIONING                 (0x01)
#define SL_WLAN_FAST_NO_PROVISIONING                     (0x00)
		 
#define SL_WLAN_PROVISIONING_CMD_START_MODE_AP								(0)
#define SL_WLAN_PROVISIONING_CMD_START_MODE_SC								(1)
#define SL_WLAN_PROVISIONING_CMD_START_MODE_APSC							(2)
#define SL_WLAN_PROVISIONING_CMD_START_MODE_APSC_EXTERNAL_CONFIGURATION		(3)
#define SL_WLAN_PROVISIONING_CMD_STOP                                       (4)
#define SL_WLAN_PROVISIONING_CMD_ABORT_EXTERNAL_CONFIRMATION                (5)

/* Provisining API Flags */
#define SL_WLAN_PROVISIONING_CMD_FLAG_EXTERNAL_CONFIRMATION  (0x00000001)

/* to be used only in provisioning stop command */
#define SL_WLAN_PROVISIONING_REMAIN_IN_CURRENT_ROLE      (0xFF)


#define SL_WLAN_EAPMETHOD_PHASE2_SHIFT                   (8)
#define SL_WLAN_EAPMETHOD_PAIRWISE_CIPHER_SHIFT          (19)
#define SL_WLAN_EAPMETHOD_GROUP_CIPHER_SHIFT             (27)
		
#define SL_WLAN_WPA_CIPHER_CCMP                          (0x1)
#define SL_WLAN_WPA_CIPHER_TKIP                          (0x2)
#define SL_WLAN_CC31XX_DEFAULT_CIPHER                    (SL_WLAN_WPA_CIPHER_CCMP | SL_WLAN_WPA_CIPHER_TKIP)

#define SL_WLAN_EAPMETHOD(phase1,phase2,pairwise_cipher,group_cipher)      \
                                                    ((phase1) | \
                                                    ((phase2) << SL_WLAN_EAPMETHOD_PHASE2_SHIFT ) |\
                                                    ((_u32)(pairwise_cipher) << SL_WLAN_EAPMETHOD_PAIRWISE_CIPHER_SHIFT ) |\
                                                    ((_u32)(group_cipher) << SL_WLAN_EAPMETHOD_GROUP_CIPHER_SHIFT ))

/*                                                                         phase1    phase2                                     pairwise_cipher               group_cipher         */
#define SL_WLAN_ENT_EAP_METHOD_TLS                       SL_WLAN_EAPMETHOD(SL_TLS,   0,                                 SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_TTLS_TLS                  SL_WLAN_EAPMETHOD(SL_TTLS,  SL_TLS,                            SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_TTLS_MSCHAPv2             SL_WLAN_EAPMETHOD(SL_TTLS,  SL_MSCHAP,                         SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_TTLS_PSK                  SL_WLAN_EAPMETHOD(SL_TTLS,  SL_PSK,                            SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_PEAP0_TLS                 SL_WLAN_EAPMETHOD(SL_PEAP0, SL_TLS,                            SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_PEAP0_MSCHAPv2            SL_WLAN_EAPMETHOD(SL_PEAP0, SL_MSCHAP,                         SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_PEAP0_PSK                 SL_WLAN_EAPMETHOD(SL_PEAP0, SL_PSK,                            SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_PEAP1_TLS                 SL_WLAN_EAPMETHOD(SL_PEAP1, SL_TLS,                            SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_PEAP1_PSK                 SL_WLAN_EAPMETHOD(SL_PEAP1, SL_PSK,                            SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_FAST_AUTH_PROVISIONING    SL_WLAN_EAPMETHOD(SL_FAST,  SL_WLAN_FAST_AUTH_PROVISIONING,    SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_FAST_UNAUTH_PROVISIONING  SL_WLAN_EAPMETHOD(SL_FAST,  SL_WLAN_FAST_UNAUTH_PROVISIONING,  SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)
#define SL_WLAN_ENT_EAP_METHOD_FAST_NO_PROVISIONING      SL_WLAN_EAPMETHOD(SL_FAST,  SL_WLAN_FAST_NO_PROVISIONING,      SL_WLAN_CC31XX_DEFAULT_CIPHER , SL_WLAN_CC31XX_DEFAULT_CIPHER)

#define SL_WLAN_LONG_PREAMBLE                            (0)
#define SL_WLAN_SHORT_PREAMBLE                           (1)
		  
#define SL_WLAN_RAW_RF_TX_PARAMS_CHANNEL_SHIFT           (0)
#define SL_WLAN_RAW_RF_TX_PARAMS_RATE_SHIFT              (6)
#define SL_WLAN_RAW_RF_TX_PARAMS_POWER_SHIFT             (11)
#define SL_WLAN_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT          (15)
		
#define SL_WLAN_RAW_RF_TX_PARAMS(chan,rate,power,preamble) \
                                                    ((chan << SL_WLAN_RAW_RF_TX_PARAMS_CHANNEL_SHIFT) | \
                                                    (rate << SL_WLAN_RAW_RF_TX_PARAMS_RATE_SHIFT) | \
                                                    (power << SL_WLAN_RAW_RF_TX_PARAMS_POWER_SHIFT) | \
                                                    (preamble << SL_WLAN_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT))


/* wlan config application IDs */
#define SL_WLAN_CFG_AP_ID                           (0)
#define SL_WLAN_CFG_GENERAL_PARAM_ID                (1)
#define SL_WLAN_CFG_P2P_PARAM_ID                    (2)
#define SL_WLAN_CFG_AP_ACCESS_LIST_ID               (3)
#define SL_WLAN_RX_FILTERS_ID                       (4)
#define SL_WLAN_CONNECTION_INFO                     (5)

/* wlan AP Config set/get options */
#define SL_WLAN_AP_OPT_SSID                         (0)
#define SL_WLAN_AP_OPT_CHANNEL                      (3)
#define SL_WLAN_AP_OPT_HIDDEN_SSID                  (4)
#define SL_WLAN_AP_OPT_SECURITY_TYPE                (6)
#define SL_WLAN_AP_OPT_PASSWORD                     (7)
#define SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE      (9)
#define SL_WLAN_GENERAL_PARAM_OPT_STA_TX_POWER      (10)
#define SL_WLAN_GENERAL_PARAM_OPT_AP_TX_POWER       (11)
#define SL_WLAN_GENERAL_PARAM_DISABLE_ENT_SERVER_AUTH (32) 
#define SL_WLAN_GENERAL_PARAM_OPT_SUSPEND_PROFILES  (33)

#define SL_WLAN_P2P_OPT_DEV_NAME                    (12)
#define SL_WLAN_P2P_OPT_DEV_TYPE                    (13)
#define SL_WLAN_P2P_OPT_CHANNEL_N_REGS              (14)
#define SL_WLAN_GENERAL_PARAM_OPT_INFO_ELEMENT      (16)
#define SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS       (18)      /* change the scan channels and RSSI threshold using this configuration option */
#define SL_WLAN_AP_OPT_MAX_STATIONS                 (19)
#define SL_WLAN_AP_ACCESS_LIST_ADD_MAC              (20)
#define SL_WLAN_AP_ACCESS_LIST_DEL_MAC              (21)
#define SL_WLAN_AP_ACCESS_LIST_DEL_IDX              (22)
#define SL_WLAN_AP_ACCESS_LIST_NUM_ENTRIES          (24)
#define SL_WLAN_AP_ACCESS_LIST_MODE                 (25)
#define SL_WLAN_AP_OPT_MAX_STA_AGING                (26)

#define SL_WLAN_RX_FILTER_STATE                     (27)
#define SL_WLAN_RX_FILTER_REMOVE                    (28)
#define SL_WLAN_RX_FILTER_STORE                     (29)
#define SL_WLAN_RX_FILTER_UPDATE_ARGS               (30)
#define SL_WLAN_RX_FILTER_SYS_STATE                 (31)

/* SmartConfig CIPHER options */
#define SL_WLAN_SMART_CONFIG_CIPHER_SFLASH               (0)      /* password is not delivered by the application. The Simple Manager should
                                                                check if the keys are stored in the Flash.                              */
#define SL_WLAN_SMART_CONFIG_CIPHER_AES                  (1)      /* AES (other types are not supported)                                     */
#define SL_WLAN_SMART_CONFIG_CIPHER_NONE                 (0xFF)   /* do not check in the flash                                               */


#define SL_WLAN_POLICY_CONNECTION                        (0x10)
#define SL_WLAN_POLICY_SCAN                              (0x20)
#define SL_WLAN_POLICY_PM                                (0x30)
#define SL_WLAN_POLICY_P2P                               (0x40)

#define SL_WLAN_VAL_2_MASK(position,value)               ((1 & (value))<<(position))
#define SL_WLAN_MASK_2_VAL(position,mask)                (((1 << position) & (mask)) >> (position))

#define SL_WLAN_CONNECTION_POLICY(Auto,Fast,anyP2P,autoProvisioning)         (SL_WLAN_VAL_2_MASK(0,Auto) | SL_WLAN_VAL_2_MASK(1,Fast) | SL_WLAN_VAL_2_MASK(2,0) | SL_WLAN_VAL_2_MASK(3,anyP2P) | SL_WLAN_VAL_2_MASK(4,0) | SL_WLAN_VAL_2_MASK(5,autoProvisioning))
#define SL_WLAN_SCAN_POLICY_EN(policy)                   (SL_WLAN_MASK_2_VAL(0,policy))
#define SL_WLAN_SCAN_POLICY(Enable,Enable_Hidden)        (SL_WLAN_VAL_2_MASK(0,Enable) | SL_WLAN_VAL_2_MASK(1,Enable_Hidden))


#define SL_WLAN_ENABLE_SCAN                              (1)
#define SL_WLAN_DISABLE_SCAN                             (0)
#define SL_WLAN_ALLOW_HIDDEN_SSID_RESULTS                (1)
#define SL_WLAN_BLOCK_HIDDEN_SSID_RESULTS                (0)
		  
#define SL_WLAN_NORMAL_POLICY                            (0)
#define SL_WLAN_LOW_LATENCY_POLICY                       (1)
#define SL_WLAN_LOW_POWER_POLICY                         (2)
#define SL_WLAN_ALWAYS_ON_POLICY                         (3)
#define SL_WLAN_LONG_SLEEP_INTERVAL_POLICY               (4)
		  
#define SL_WLAN_P2P_ROLE_NEGOTIATE                       (3)
#define SL_WLAN_P2P_ROLE_GROUP_OWNER                     (15)
#define SL_WLAN_P2P_ROLE_CLIENT                          (0)
		 
#define SL_WLAN_P2P_NEG_INITIATOR_ACTIVE                 (0)
#define SL_WLAN_P2P_NEG_INITIATOR_PASSIVE                (1)
#define SL_WLAN_P2P_NEG_INITIATOR_RAND_BACKOFF           (2)
		
#define SL_WLAN_POLICY_VAL_2_OPTIONS(position,mask,policy)    ((mask & policy) << position )

#define SL_WLAN_P2P_POLICY(p2pNegType,p2pNegInitiator)   (SL_WLAN_POLICY_VAL_2_OPTIONS(0,0xF,(p2pNegType > SL_WLAN_P2P_ROLE_GROUP_OWNER ? SL_WLAN_P2P_ROLE_GROUP_OWNER : p2pNegType)) | \
                                                     SL_WLAN_POLICY_VAL_2_OPTIONS(4,0x1,(p2pNegType > SL_WLAN_P2P_ROLE_GROUP_OWNER ? 1:0)) | \
                                                     SL_WLAN_POLICY_VAL_2_OPTIONS(5,0x3, p2pNegInitiator))


/* Info elements */
#define SL_WLAN_INFO_ELEMENT_DEFAULT_ID              (0) /* 221 will be used */

/* info element size is up to 252 bytes (+ 3 bytes of OUI). */
#define SL_WLAN_INFO_ELEMENT_MAX_SIZE                (252)

/* For AP - the total length of all info elements is 300 bytes (for example - 4 info elements of 75 bytes each) */
#define SL_WLAN_INFO_ELEMENT_MAX_TOTAL_LENGTH_AP     (300)

/* For P2P - the total length of all info elements is 160 bytes (for example - 4 info elements of 40 bytes each) */
#define SL_WLAN_INFO_ELEMENT_MAX_TOTAL_LENGTH_P2P_GO (160)

#define SL_WLAN_INFO_ELEMENT_AP_ROLE                 (0)
#define SL_WLAN_INFO_ELEMENT_P2P_GO_ROLE             (1)

/* we support up to 4 info elements per Role. */
#define SL_WLAN_MAX_PRIVATE_INFO_ELEMENTS_SUPPROTED  (4)

#define SL_WLAN_INFO_ELEMENT_DEFAULT_OUI_0           (0x08)
#define SL_WLAN_INFO_ELEMENT_DEFAULT_OUI_1           (0x00)
#define SL_WLAN_INFO_ELEMENT_DEFAULT_OUI_2           (0x28)

#define SL_WLAN_INFO_ELEMENT_DEFAULT_OUI             (0x000000)  /* 08, 00, 28 will be used */

#define SL_WLAN_AP_ACCESS_LIST_MODE_DISABLED         0
#define SL_WLAN_AP_ACCESS_LIST_MODE_DENY_LIST        1
#define SL_WLAN_MAX_ACCESS_LIST_STATIONS             8


/* Scan results security information */
#define SL_WLAN_SCAN_RESULT_GROUP_CIPHER(SecurityInfo)                      (SecurityInfo & 0xF)   /* Possible values: NONE,SL_WLAN_CIPHER_BITMAP_TKIP,SL_WLAN_CIPHER_BITMAP_CCMP */
#define SL_WLAN_SCAN_RESULT_UNICAST_CIPHER_BITMAP(SecurityInfo)             ((SecurityInfo & 0xF0) >> 4 ) /* Possible values: NONE,SL_WLAN_CIPHER_BITMAP_WEP40,SL_WLAN_CIPHER_BITMAP_WEP104,SL_WLAN_CIPHER_BITMAP_TKIP,SL_WLAN_CIPHER_BITMAP_CCMP*/
#define SL_WLAN_SCAN_RESULT_HIDDEN_SSID(SecurityInfo)                       (SecurityInfo & 0x2000 ) >> 13 /* Possible values: TRUE/FALSE */    
#define SL_WLAN_SCAN_RESULT_KEY_MGMT_SUITES_BITMAP(SecurityInfo)            (SecurityInfo & 0x1800 ) >> 11  /* Possible values: SL_WLAN_KEY_MGMT_SUITE_802_1_X, SL_WLAN_KEY_MGMT_SUITE_PSK */
#define SL_WLAN_SCAN_RESULT_SEC_TYPE_BITMAP(SecurityInfo)                   (SecurityInfo & 0x700   ) >> 8  /* Possible values: SL_WLAN_SECURITY_TYPE_BITMAP_OPEN, SL_WLAN_SECURITY_TYPE_BITMAP_WEP, SL_WLAN_SECURITY_TYPE_BITMAP_WPA, SL_WLAN_SECURITY_TYPE_BITMAP_WPA2, 0x6 (mix mode) SL_WLAN_SECURITY_TYPE_BITMAP_WPA | SL_WLAN_SECURITY_TYPE_BITMAP_WPA2 */

#define SL_WLAN_SECURITY_TYPE_BITMAP_OPEN            0x0
#define SL_WLAN_SECURITY_TYPE_BITMAP_WEP             0x1
#define SL_WLAN_SECURITY_TYPE_BITMAP_WPA             0x2
#define SL_WLAN_SECURITY_TYPE_BITMAP_WPA2            0x4

#define SL_WLAN_CIPHER_BITMAP_WEP40                  0x1
#define SL_WLAN_CIPHER_BITMAP_WEP104                 0x2
#define SL_WLAN_CIPHER_BITMAP_TKIP                   0x4
#define SL_WLAN_CIPHER_BITMAP_CCMP                   0x8

#define SL_WLAN_KEY_MGMT_SUITE_802_1_X               1
#define SL_WLAN_KEY_MGMT_SUITE_PSK                   2



#define SL_WLAN_RX_FILTER_MAX_FILTERS                       (64)    /* Max number of filters is 64 filters */
#define SL_WLAN_RX_FILTER_MAX_SYS_FILTERS_SETS              (32)    /* The Max number of system filters */
#define SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS         (2)
#define SL_WLAN_RX_FILTER_NUM_OF_FILTER_PAYLOAD_ARGS        (2)
#define SL_WLAN_RX_FILTER_RANGE_ARGS                        (2)
#define SL_WLAN_RX_FILTER_NUM_USER_EVENT_ID                 (64)
#define SL_WLAN_RX_FILTER_MAX_USER_EVENT_ID                 ( ( SL_WLAN_RX_FILTER_NUM_USER_EVENT_ID ) - 1 )

/*  Bit manipulation for 8 bit */
#define SL_WLAN_ISBITSET8(x,i)      ((x[i>>3] & (0x80>>(i&7)))!=0)  /* Is bit set, 8 bit unsigned numbers = x , location = i */
#define SL_WLAN_SETBIT8(x,i)        x[i>>3]|=(0x80>>(i&7));         /* Set bit,8 bit unsigned numbers = x , location = i */
#define SL_WLAN_CLEARBIT8(x,i)      x[i>>3]&=(0x80>>(i&7))^0xFF;    /* Clear bit,8 bit unsigned numbers = x , location = i */


/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

typedef enum
{
    SL_WLAN_RATE_1M         = 1,
    SL_WLAN_RATE_2M         = 2,
    SL_WLAN_RATE_5_5M       = 3,
    SL_WLAN_RATE_11M        = 4,
    SL_WLAN_RATE_6M         = 6,
    SL_WLAN_RATE_9M         = 7,
    SL_WLAN_RATE_12M        = 8,
    SL_WLAN_RATE_18M        = 9,
    SL_WLAN_RATE_24M        = 10,
    SL_WLAN_RATE_36M        = 11,
    SL_WLAN_RATE_48M        = 12,
    SL_WLAN_RATE_54M        = 13,
    SL_WLAN_RATE_MCS_0      = 14,
    SL_WLAN_RATE_MCS_1      = 15,
    SL_WLAN_RATE_MCS_2      = 16,
    SL_WLAN_RATE_MCS_3      = 17,
    SL_WLAN_RATE_MCS_4      = 18,
    SL_WLAN_RATE_MCS_5      = 19,
    SL_WLAN_RATE_MCS_6      = 20,
    SL_WLAN_RATE_MCS_7      = 21,
    SL_WLAN_MAX_NUM_RATES   = 0xFF
}SlWlanRateIndex_e;

typedef enum
{
    SL_WLAN_DEV_PW_DEFAULT      = 0,
    SL_WLAN_DEV_PW_PIN_KEYPAD   = 1,
    SL_WLAN_DEV_PW_PUSH_BUTTON  = 4,
    SL_WLAN_DEV_PW_PIN_DISPLAY  = 5
} SlWlanP2pDevPwdMethod_e;


typedef struct
{
    _u32    Status;
    _u32    SsidLen;
    _u8     Ssid[32];
    _u32    PrivateTokenLen;
    _u8     PrivateToken[32];
}SlWlanSmartConfigStartAsyncResponse_t;

typedef struct
{
    _u16    Status;
    _u16    Padding;
}SlWlanSmartConfigStopAsyncResponse_t;

typedef struct
{
    _u16    Status;
    _u16    Padding;
}SlWlanConnFailureAsyncResponse_t;

typedef struct
{
    _u16    Status;
    _u16    Padding;
}SlWlanProvisioningStatusAsyncResponse_t;

/* rx filter event struct
  this event will be sent from the SL device
  as a result of a passed rx filter
  example:
  suppose we have a filter with an action and we set the following:
  SlWlanRxFilterAction_t Action;
  Action.UserId = 2;
  When the filter result is pass, an SlWlanEventRxFilterInfo_t event will be passed to the user as follows:
  Type will be set to 0
  bit 2 in UserActionIdBitmap will be set in this event, because 2 is the user input for the action arg above.
  an SlWlanEventRxFilterInfo_t event may have several bits set as a result of several rx filters causing different
  events to pass */

typedef struct
{
    _u8     Type;                                                          /* Currently only event type 0 is supported. */
    _u8     UserActionIdBitmap[SL_WLAN_RX_FILTER_NUM_USER_EVENT_ID / 8];   /* Bit X is set indicates that the filter with event action arg X passed. */
}SlWlanEventRxFilterInfo_t;



typedef enum
{
    ROLE_STA      = 0,
    ROLE_RESERVED = 1,
    ROLE_AP       = 2,
    ROLE_P2P      = 3
}SlWlanMode_e;

typedef struct
{
    _u8     SsidLen;
    _u8     SsidName[32];
    _u8     Bssid[6];
    _u8     Padding;
} SlWlanEventConnect_t;

typedef struct
{
    _u8     SsidLen;
    _u8     SsidName[32];
    _u8     Bssid[6];
    _u8     ReasonCode;
} SlWlanEventDisconnect_t;

typedef struct
{
    _u8   Mac[6];
    _u8   Padding[2];
}SlWlanEventSTAAdded_t, SlWlanEventSTARemoved_t;


typedef struct
{
    _u8     SsidLen;
    _u8     SsidName[32];
    _u8     Bssid[6];
    _u8     Reserved;
    _u8     GoDeviceNameLen;
    _u8     GoDeviceName[32];
    _u8     Padding[3];
} SlWlanEventP2PConnect_t;

typedef struct
{
    _u8     SsidLen;
    _u8     SsidName[32];
    _u8     Bssid[6];
    _u8     ReasonCode;
    _u8     GoDeviceNameLen;
    _u8     GoDeviceName[32];
    _u8     Padding[3];
} SlWlanEventP2PDisconnect_t;




typedef struct
{
    _u8     Mac[6];
    _u8     ClDeviceNameLen;
    _u8     ClDeviceName[32];
    _u8     OwnSsidLen;
    _u8     OwnSsid[32];
}SlWlanEventP2PClientAdded_t, SlWlanEventP2PClientRemoved_t;


typedef struct
{
    _u8     GoDeviceNameLen;
    _u8     GoDeviceName[32];
    _u8     Mac[6];
    _u8     WpsMethod;
}SlWlanEventP2PDevFound_t, SlWlanEventP2PRequest_t;


/**************************************************/
typedef struct
{
    _u16    Status;
    _u16    Padding;
}SlWlanEventP2PConnectFail_t;


typedef struct
{
	_u8  ProvisioningStatus;
	_u8  Role;
	_u8  WlanStatus;
	_u8  Ssidlen;
	_u8  Ssid[32];
	_u32 Reserved;
}SlWlanEventProvisioningStatus_t;

typedef struct
{
    _u32    Status;
    _u32    SsidLen;
    _u8     Ssid[32];
    _u32    ReservedLen;
    _u8     Reserved[32];
} SlWlanEventProvisioningProfileAdded_t;


typedef union
{
    SlWlanEventConnect_t                         Connect;                    /* SL_WLAN_EVENT_CONNECT */
    SlWlanEventDisconnect_t                      Disconnect;                 /* SL_WLAN_EVENT_DISCONNECT */
    SlWlanEventSTAAdded_t                        STAAdded;                   /* SL_WLAN_EVENT_STA_ADDED */
    SlWlanEventSTARemoved_t                      STARemoved;                 /* SL_WLAN_EVENT_STA_REMOVED */
    SlWlanEventP2PConnect_t                      P2PConnect;                 /* SL_WLAN_EVENT_P2P_CONNECT */
    SlWlanEventP2PDisconnect_t                   P2PDisconnect;              /* SL_WLAN_EVENT_P2P_DISCONNECT */
    SlWlanEventP2PClientAdded_t                  P2PClientAdded;             /* SL_WLAN_EVENT_P2P_CLIENT_ADDED */
    SlWlanEventP2PClientRemoved_t                P2PClientRemoved;           /* SL_WLAN_EVENT_P2P_CLIENT_REMOVED */
    SlWlanEventP2PDevFound_t                     P2PDevFound;                /* SL_WLAN_EVENT_P2P_DEVFOUND */
    SlWlanEventP2PRequest_t                      P2PRequest;                 /* SL_WLAN_EVENT_P2P_REQUEST */
    SlWlanEventP2PConnectFail_t                  P2PConnectFail;             /* SL_WLAN_EVENT_P2P_CONNECTFAIL */
    SlWlanEventRxFilterInfo_t                    RxFilterInfo;               /* SL_WLAN_EVENT_RXFILTER */
    SlWlanEventProvisioningStatus_t              ProvisioningStatus;         /* SL_WLAN_EVENT_PROVISIONING_STATUS */
    SlWlanEventProvisioningProfileAdded_t        ProvisioningProfileAdded;   /* SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED */

} SlWlanEventData_u;


typedef struct
{
   _u32                 Id;
   SlWlanEventData_u    Data;
} SlWlanEvent_t;


typedef struct
{
    _u32  ReceivedValidPacketsNumber;							/* sum of the packets that been received OK (include filtered) */
    _u32  ReceivedFcsErrorPacketsNumber;						/* sum of the packets that been dropped due to FCS error */
    _u32  ReceivedAddressMismatchPacketsNumber;					/* sum of the packets that been received but filtered out by one of the HW filters */
    _i16  AvarageDataCtrlRssi;									/* average RSSI for all valid data packets received */
    _i16  AvarageMgMntRssi;										/* average RSSI for all valid management packets received */
    _u16  RateHistogram[SL_WLAN_NUM_OF_RATE_INDEXES];           /* rate histogram for all valid packets received */
    _u16  RssiHistogram[SL_WLAN_SIZE_OF_RSSI_HISTOGRAM];        /* RSSI histogram from -40 until -87 (all below and above\n RSSI will appear in the first and last cells */
    _u32  StartTimeStamp;										/* the time stamp started collecting the statistics in uSec */
    _u32  GetTimeStamp;											/* the time stamp called the get statistics command */
}SlWlanGetRxStatResponse_t;




typedef struct
{
    _u8 Ssid[SL_WLAN_SSID_MAX_LENGTH];
    _u8 Bssid[SL_WLAN_BSSID_LENGTH];
    _u8 SsidLen;
    _i8 Rssi;
    _i16 SecurityInfo;
    _i8 Channel;
    _i8 Reserved[1];
}SlWlanNetworkEntry_t;

typedef struct
{
    _u8   Type;
    _i8*  Key;
    _u8   KeyLen;
}SlWlanSecParams_t;

typedef struct
{
    _i8*  User;
    _u8   UserLen;
    _i8*  AnonUser;
    _u8   AnonUserLen;
    _u8   CertIndex;  /* not supported */
    _u32  EapMethod;
}SlWlanSecParamsExt_t;

typedef struct
{
    _i8   User[64];
    _u8   UserLen;
    _i8   AnonUser[64];
    _u8   AnonUserLen;
    _u8   CertIndex;  /* not supported */
    _u32  EapMethod;
}SlWlanGetSecParamsExt_t;


#define SL_WLAN_CONNECTION_PROTOCOL_STA     1
#define SL_WLAN_CONNECTION_PROTOCOL_P2PCL   2

typedef union
{
    SlWlanEventConnect_t       StaConnect;
    SlWlanEventP2PConnect_t    P2PConnect;
} SlWlanConnectionInfo_u;

typedef enum
{
	SL_WLAN_DISCONNECTED = 0,
    SL_WLAN_CONNECTED_STA,
    SL_WLAN_CONNECTED_P2PCL,
    SL_WLAN_CONNECTED_P2PGO,
    SL_WLAN_AP_CONNECTED_STATIONS
}SlWlanConnStatusFlags_e;

typedef struct
{
    _u8 Mode;       /* ROLE_STA, ROLE_AP, ROLE_P2P */
    _u8 ConnStatus; /* SlWlanConnStatusFlags_e */
    _u8 SecType;    /* Current connection security type - (0 in case of disconnect or AP mode) SL_WLAN_SEC_TYPE_OPEN, SL_WLAN_SEC_TYPE_WEP, SL_WLAN_SEC_TYPE_WPA_WPA2, SL_WLAN_SEC_TYPE_WPA_ENT, SL_WLAN_SEC_TYPE_WPS_PBC, SL_WLAN_SEC_TYPE_WPS_PIN */
    _u8 Reserved;
	SlWlanConnectionInfo_u ConnectionInfo;
}SlWlanConnStatusParam_t;

typedef struct
{
    _u32   ChannelsMask;
    _i32   RssiThershold;
}SlWlanScanParamCommand_t;


typedef struct
{
    _u8   Id;
    _u8   Oui[3];
    _u16  Length;
    _u8   Data[252];
} SlWlanInfoElement_t;

typedef struct
{
    _u8          Index;  /* 0 - SL_WLAN_MAX_PRIVATE_INFO_ELEMENTS_SUPPROTED */
    _u8          Role;   /* bit0: AP = 0, GO = 1                    */
    SlWlanInfoElement_t   IE;
} SlWlanSetInfoElement_t;


typedef struct
{
    _u16        Reserved;
    _u16        Reserved2;
    _u16        MaxSleepTimeMs;   /* max sleep time in mSec For setting Long Sleep Interval policy use */
    _u16        Reserved3;
} SlWlanPmPolicyParams_t;



typedef  _i8    SlWlanRxFilterID_t; /* Unique filter ID which is allocated by the system , negative number means error */

/*  Representation of filters Id as a bit field
    The bit field is used to declare which filters are involved
    in operation. Number of filter can be up to 128 filters.
    i.e. 128 bits are needed. On the current release, up to 64 filters can be defined. */
typedef _u8   SlWlanRxFilterIdMask_t[128/8];


typedef _u8  SlWlanRxFilterSysFilters_t; /* Describes the supported system filter sets*/
/* possible values for SlWlanRxFilterSysFilters_t */
#define SL_WLAN_RX_FILTER_ARP_AUTO_REPLY_SYS_FILTERS       (0)
#define SL_WLAN_RX_FILTER_MULTICASTSIPV4_SYS_FILTERS       (1)
#define SL_WLAN_RX_FILTER_MULTICASTSIPV6_SYS_FILTERS       (2)
#define SL_WLAN_RX_FILTER_MULTICASTSWIFI_SYS_FILTERS       (3)
#define SL_WLAN_RX_FILTER_SELF_MAC_ADDR_DROP_SYS_FILTERS   (4)


/*  Describes the supported system filter sets, each bit represents different system filter set
    The filter sets are defined at SlWlanRxFilterSysFilters_t  */
typedef _u8   SlWlanRxFilterSysFiltersMask_t[SL_WLAN_RX_FILTER_MAX_SYS_FILTERS_SETS/8];

typedef struct 
{
    _u16    Offset;     /*  Offset in payload - Where in the payload to search for the pattern */
    _u8     Length;     /* Pattern Length */
    _u8     Reserved;
    _u8     Value[16];/* Up to 16 bytes long (based on pattern length above) */
}SlWlanRxFilterPatternArg_t;


typedef _u8 SlWlanRxFilterRuleType_t; /* Different filter types */
/* possible values for SlWlanRxFilterRuleType_t */
#define SL_WLAN_RX_FILTER_HEADER                    (0)
#define SL_WLAN_RX_FILTER_COMBINATION               (1)

typedef _u8 SlWlanRxFilterFlags_u;
/* Possible values for SlWlanRxFilterFlags_u */
#define SL_WLAN_RX_FILTER_BINARY                    (0x1)
#define SL_WLAN_RX_FILTER_PERSISTENT                (0x8)
#define SL_WLAN_RX_FILTER_ENABLE                    (0x10)

/* Used as comparison function for the header type arguments */
typedef _u8 SlWlanRxFilterRuleHeaderCompareFunction_t;
/* Possible values for SlWlanRxFilterRuleHeaderCompareFunction_t */
#define SL_WLAN_RX_FILTER_CMP_FUNC_IN_BETWEEN       (0)
#define SL_WLAN_RX_FILTER_CMP_FUNC_EQUAL            (1)
#define SL_WLAN_RX_FILTER_CMP_FUNC_NOT_EQUAL_TO     (2)
#define SL_WLAN_RX_FILTER_CMP_FUNC_NOT_IN_BETWEEN   (3)


typedef _u8 SlWlanRxFilterTriggerCompareFunction_t;
/* Possible values for SlWlanRxFilterTriggerCompareFunction_t */
#define SL_WLAN_RX_FILTER_TRIGGER_CMP_FUNC_EQUAL                        (0)
#define SL_WLAN_RX_FILTER_TRIGGER_CMP_FUNC_NOT_EQUAL_TO                 (1) /*   arg1 == protocolVal ,not supported in current release */
#define SL_WLAN_RX_FILTER_TRIGGER_CMP_FUNC_SMALLER_THAN                 (2) /*   arg1 == protocolVal */
#define SL_WLAN_RX_FILTER_TRIGGER_CMP_FUNC_BIGGER_THAN                  (3) /*   arg1 == protocolVal */


typedef _u8 SlWlanRxFilterRuleHeaderField_t; /* Provides list of possible header types which may be defined as part of the rule */
/* Possible values for SlWlanRxFilterRuleHeaderField_t */
#define SL_WLAN_RX_FILTER_HFIELD_NULL                     (0)
#define SL_WLAN_RX_FILTER_HFIELD_FRAME_TYPE               (1) /* 802.11 control\data\management */
#define SL_WLAN_RX_FILTER_HFIELD_FRAME_SUBTYPE            (2) /*  802.11 beacon\probe\.. */
#define SL_WLAN_RX_FILTER_HFIELD_BSSID                    (3)  /*  802.11 bssid type */
#define SL_WLAN_RX_FILTER_HFIELD_MAC_SRC_ADDR             (4)
#define SL_WLAN_RX_FILTER_HFIELD_MAC_DST_ADDR             (5)
#define SL_WLAN_RX_FILTER_HFIELD_FRAME_LENGTH             (6)
#define SL_WLAN_RX_FILTER_HFIELD_ETHER_TYPE               (7)
#define SL_WLAN_RX_FILTER_HFIELD_IP_VERSION               (8)
#define SL_WLAN_RX_FILTER_HFIELD_IP_PROTOCOL              (9)  /* TCP / UDP / ICMP / ICMPv6 / IGMP */
#define SL_WLAN_RX_FILTER_HFIELD_IPV4_SRC_ADDR            (10)
#define SL_WLAN_RX_FILTER_HFIELD_IPV4_DST_ADDR            (11)
#define SL_WLAN_RX_FILTER_HFIELD_IPV6_SRC_ADRR            (12)
#define SL_WLAN_RX_FILTER_HFIELD_IPV6_DST_ADDR            (13)
#define SL_WLAN_RX_FILTER_HFIELD_PORT_SRC                 (14)
#define SL_WLAN_RX_FILTER_HFIELD_PORT_DST                 (15)
#define SL_WLAN_RX_FILTER_HFIELD_L4_PAYLOAD_PATTERN       (19) /* use to look for patterns on the TCP and UDP payloads (after TCP/UDP header) */
#define SL_WLAN_RX_FILTER_HFIELD_L1_PAYLOAD_PATTERN       (20) /* use to look for patterns on the PHY payload (i.e. beginning of WLAN MAC header) */
#define SL_WLAN_RX_FILTER_HFIELD_MAX_FIELD                (21) /* Definition */


/* Holds the header ARGS which are used in case of HDR rule */
typedef union 
{
    /* buffer for pattern matching in payload up to 16 bytes (Binary Values) */
    SlWlanRxFilterPatternArg_t  Pattern;

    /* Buffer for ipv4 address filter. binary arguments, number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS*/
    _u8 Ipv4[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][4]; /*  Binary Values for comparison */

    /* Buffer for ipv4 address filter. Ascii arguments - IPv4 address: 4 bytes: ddd.ddd.ddd.ddd - 15 chars. Number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS*/
    _u8 Ipv4Ascii[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][16]; /*  Ascii Values for comparison */

    /* Buffer for ipv6 address filter. binary arguments, Ascii format is not supported. number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS*/
    _u8 Ipv6[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][16]; /*  Binary Values for comparison */

    /* Buffer for mac address filter. binary arguments.  number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS*/
    _u8 Mac[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][6]; /*  Binary Values for comparison */

    /* Buffer for mac address filter. Ascii arguments - MAC address:  6 bytes: xx:xx:xx:xx:xx:xx - 17 chars.  number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS */
    _u8 MacAscii[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][18]; /*  Ascii Values for comparison */

    /* Buffer for BSSID address filter. binary arguments.  number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS*/
    _u8 Bssid[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][6]; /*  Binary Values for comparison */

    /* Buffer for frame length filter. number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS */
    _u32 FrameLength[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS]; /*  Binary Values for comparison */

    /* Buffer for port filter. number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS */
    _u16 Port[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS]; /*  Binary Values for comparison */

    /* Buffer for Ether filter. number of argument may be up to SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS (according to host endianity) */
    _u32 EtherType[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS]; /*  Binary Values for comparison */

    /* Buffer for ip version filter. Buffer for binary arguments. IP Version - 4 for IPV4 and 6 for IPV6  */
    _u8 IpVersion;

    /* Buffer for frame type filter. Buffer for binary arguments. Frame Type (0 - management, 1 - Control, 2 - Data)  */
    _u8 Frametype[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS];

    /* Buffer for frame subtype filter. Buffer for binary arguments. Frame Sub Type (checkout the full list in the 802.11 spec). e.g. Beacon=0x80, Data=0x08, Qos-Data=0x04, ACK=0xD4, etc. */
    _u8 FrameSubtype[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS];

    /* Buffer for protocol type filter. Buffer for binary arguments. e.g. 1 – ICMP (IPV4 only), 2 - IGMP (IPV4 only), 6 – TCP. 17 – UDP, 58 – ICMPV6 */
    _u8 IpProtocol[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS];

    /* Buffer for ip version filter. Buffer for ASCII arguments. Use for IP version field comparison settings: "IPV4", "IPV6"  */
    _u8 IpVersionAscii[4];

    /* Buffer for frame type filter. Buffer for ASCII arguments. Use for Frame type field comparison settings: "MGMT", "CTRL", "DATA"  */
    _u8 FrametypeAscii[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][4];

    /* Buffer for protocol type filter. Buffer for ASCII arguments. Use for protocol field comparison settings: "ICMP", "IGMP", "TCP, "UDP", "ICMP6"  */
    /* Note: Use memcpy with these strings instead of strcpy (no \0 should be at the end, as the array is 5 bytes long and ICMP6 is already 5 bytes long without the \0) */
    _u8 IpProtocolAscii[SL_WLAN_RX_FILTER_NUM_OF_FILTER_HEADER_ARGS][5];

}SlWlanRxFilterHeaderArg_u;


/* Defines the Header Args and mask */
typedef struct
{
    SlWlanRxFilterHeaderArg_u                   Value; /* Argument for the comparison function */
    _u8                                         Mask[16];  /* the mask is used in order to enable partial comparison (bit level), Use the 0xFFFFFFFF in case you don't want to use mask */

}SlWlanRxFilterRuleHeaderArgs_t;

/* defines the Header rule. The header rule defines the compare function on the protocol header
   For example destMacAddre is between ( 12:6::78:77,  12:6::78:90 ) */
typedef struct 
{
    SlWlanRxFilterRuleHeaderArgs_t              Args; /* Filter arguemnts */
    SlWlanRxFilterRuleHeaderField_t             Field; /* Packet HDR field which will be compared to the argument */
    SlWlanRxFilterRuleHeaderCompareFunction_t   CompareFunc; /*  type of the comparison function */
    _u8                                         Padding[2];
}SlWlanRxFilterRuleHeader_t;


/* Optional operators for the combination type filterID1 is located in the first arg , filterId2 is the second arg */
typedef _u8 SlWlanRxFilterRuleCombinationOperator_t;
/* Possible values for SlWlanRxFilterRuleCombinationOperator_t */
#define SL_WLAN_RX_FILTER_COMBINED_FUNC_NOT     (0) /* filterID1 */
#define SL_WLAN_RX_FILTER_COMBINED_FUNC_AND     (1) /* filterID1 && filterID2 */
#define SL_WLAN_RX_FILTER_COMBINED_FUNC_OR      (2) /* filterID1 && filterID2 */


/* Defines the structure which define the combination type filter
   The combined filter enable to make operation on one or two filter,
   for example filterId1 or and(filterId2,filterId3). */
typedef struct 
{
    SlWlanRxFilterRuleCombinationOperator_t     Operator; /* combination operator */
    SlWlanRxFilterID_t                          CombinationFilterId[SL_WLAN_RX_FILTER_RANGE_ARGS]; /* filterID, may be one or two depends on the combination operator type */
    _u8                                         Padding;
}SlWlanRxFilterRuleCombination_t;


/* Rule structure composed of behavioral flags and the filter rule definitions */
typedef union 
{
    SlWlanRxFilterRuleHeader_t                  Header; /* Filter is from type Header */
    SlWlanRxFilterRuleCombination_t             Combination; /*  Filter is from type Combination */
}SlWlanRxFilterRule_u;


/* Bit field which represents the roleId possible values
   In the current release only Station (with or without promiscuous modes) and AP roles are supported.
   Activating filters before P2P negotiations (i.e. decision whether role is CL or GO) may result with
   unexpected behaviour. After this stage, filters can be activated whereas STA role is the equivalent of P2P CL role
   AP role is the equivalent of P2P GO role.
 */
typedef _u8 SlWlanRxFilterTriggerRoles_t;
/* Possible values for SlWlanRxFilterTriggerRoles_t */
#define SL_WLAN_RX_FILTER_ROLE_AP                            (1)
#define SL_WLAN_RX_FILTER_ROLE_STA                           (2)
#define SL_WLAN_RX_FILTER_ROLE_TRANCIEVER                    (4)
#define SL_WLAN_RX_FILTER_ROLE_NULL                          (0)


typedef _u8 SlWlanRxFilterTriggerConnectionStates_t;
/* Possible values for SlWlanRxFilterTriggerConnectionStates_t */
#define SL_WLAN_RX_FILTER_STATE_STA_CONNECTED     (0x1)
#define SL_WLAN_RX_FILTER_STATE_STA_NOT_CONNECTED (0x2)
#define SL_WLAN_RX_FILTER_STATE_STA_HAS_IP        (0x4)
#define SL_WLAN_RX_FILTER_STATE_STA_HAS_NO_IP     (0x8)


/* There are 8 possible counter. if no counter is needed set to NO_TRIGGER_COUNTER */
typedef _u8 SlWlanRxFilterCounterId_t;
/* Possible values for SlWlanRxFilterCounterId_t */
#define SL_WLAN_RX_FILTER_NO_TRIGGER_COUNTER                (0)
#define SL_WLAN_RX_FILTER_COUNTER1                          (1)
#define SL_WLAN_RX_FILTER_COUNTER2                          (2)
#define SL_WLAN_RX_FILTER_COUNTER3                          (3)
#define SL_WLAN_RX_FILTER_COUNTER4                          (4)
#define SL_WLAN_RX_FILTER_COUNTER5                          (5)
#define SL_WLAN_RX_FILTER_COUNTER6                          (6)
#define SL_WLAN_RX_FILTER_COUNTER7                          (7)
#define SL_WLAN_RX_FILTER_COUNTER8                          (8)
#define SL_WLAN_RX_FILTER_MAX_COUNTER                       (9)


/* The filter trigger, determine when the filter is triggered,
   The filter is triggered in the following condition :\n
   1. The filter parent is triggered\n
   2. The requested connection type exists, i.e. wlan_connect\n
   3. The filter role is the same as the system role\n */
typedef struct
{
    SlWlanRxFilterID_t                      ParentFilterID; /* The parent filter ID, this is the way to build filter tree.  NULL value means tree root */
    SlWlanRxFilterCounterId_t               Counter; /* Trigger only when reach counter threshold */
    SlWlanRxFilterTriggerConnectionStates_t ConnectionState; /* Trigger only with specific connection state */
    SlWlanRxFilterTriggerRoles_t            Role; /* Trigger only with specific role */
    _u32                                    CounterVal;  /* Value for the counter if set */
    SlWlanRxFilterTriggerCompareFunction_t  CompareFunction; /* The compare function reffers to the counter if set */
    _u8                                     Padding[3];
} SlWlanRxFilterTrigger_t;


/* The actions are executed only if the filter is matched,\n
 *  In case of false match the packet is transfered to the HOST. \n
 *  The action is composed of bit field structure, up to 2 actions can be defined per filter.\n  */
typedef _u8 SlWlanRxFilterActionType_t;
/* Possible values for SlWlanRxFilterActionType_t */
#define SL_WLAN_RX_FILTER_ACTION_NULL               (0x0) /* No action to execute*/
#define SL_WLAN_RX_FILTER_ACTION_DROP               (0x1) /* If not dropped ,The packet is passed to the next filter or in case it is the last filter to the host */
#define SL_WLAN_RX_FILTER_ACTION_ON_REG_INCREASE    (0x4) /* action increase counter registers */
#define SL_WLAN_RX_FILTER_ACTION_ON_REG_DECREASE    (0x8) /* action decrease counter registers */
#define SL_WLAN_RX_FILTER_ACTION_ON_REG_RESET       (0x10)/* action reset counter registers */
#define SL_WLAN_RX_FILTER_ACTION_SEND_TEMPLATE      (0x20)/* unsupported */
#define SL_WLAN_RX_FILTER_ACTION_EVENT_TO_HOST      (0x40)/* action can send events to host */



/* Several actions can be defined The action is executed in case the filter rule is matched. */
typedef struct
{
    SlWlanRxFilterActionType_t Type; /* Determine which actions are supported */
    _u8    Counter;    /* The counter in use. In case the action is of type increase\decrease\reset this arg will contain the counter number, The counter number values are as in  ::SlWlanRxFilterCounterId_t.\n*/
    _u16   Reserved;   /* Must be set to zero */
    _u8    UserId;     /* In case action set to host event, user can set id which will return in the event arguments */
    _u8    Padding[3];

} SlWlanRxFilterAction_t;

/* The supported operation: SL_WLAN_RX_FILTER_STATE, SL_WLAN_RX_FILTER_REMOVE */
typedef struct
{
    SlWlanRxFilterIdMask_t FilterBitmap;
    _u8 Padding[4];

} SlWlanRxFilterOperationCommandBuff_t;

/* The supported operation: SL_WLAN_RX_FILTER_UPDATE_ARGS */
typedef struct
{
    _u8  FilterId;
    _u8  BinaryOrAscii; /* Set 1 for Binary argument representation, 0 - for Ascii representation */
	_u8 Padding[2];
    SlWlanRxFilterRuleHeaderArgs_t Args;
    

} SlWlanRxFilterUpdateArgsCommandBuff_t;


/* Filters bitmap enable\disable status return value */
typedef struct 
{
    SlWlanRxFilterIdMask_t FilterIdMask; /* The filter set bit map */

}SlWlanRxFilterRetrieveStateBuff_t;

/* Disbale/Enable system filters */
typedef struct
{
    SlWlanRxFilterSysFiltersMask_t  FilterBitmap; /* The filter set bit map */

} SlWlanRxFilterSysFiltersSetStateBuff_t;

/* System filters status return value */
typedef struct
{
    SlWlanRxFilterSysFiltersMask_t  FilterBitmap; /* The filter get bit map */

} SlWlanRxFilterSysFiltersRetrieveStateBuff_t;


/*****************************************************************************/
/* Function prototypes                                                                       */
/*****************************************************************************/


/*!
    \brief Connect to wlan network as a station


    \param[in]      pName       Up to 32 bytes in case of STA the name is the SSID of the Access Point
    \param[in]      NameLen     Name length
    \param[in]      pMacAddr    6 bytes for MAC address
    \param[in]      pSecParams  Security parameters (use NULL key for SL_WLAN_SEC_TYPE_OPEN)\n
                                security types options: 
                                - SL_WLAN_SEC_TYPE_OPEN
                                - SL_WLAN_SEC_TYPE_WEP
								- SL_WLAN_SEC_TYPE_WEP_SHARED
                                - SL_WLAN_SEC_TYPE_WPA_WPA2
                                - SL_WLAN_SEC_TYPE_WPA_ENT
                                - SL_WLAN_SEC_TYPE_WPS_PBC
                                - SL_WLAN_SEC_TYPE_WPS_PIN

    \param[in]      pSecExtParams  Enterprise parameters (set NULL in case Enterprise parameters is not in use)

    \return         Zero on success, or negative error code on failure


    \sa             sl_WlanDisconnect
    \note           Belongs to \ref ext_api
    \warning        In this version only single enterprise mode could be used\n
                    SL_WLAN_SEC_TYPE_WPA is a deprecated definition, the new definition is SL_WLAN_SEC_TYPE_WPA_WPA2
	\par Example
	
	- Connect without security:
    \code
			SlWlanSecParams_t secParams;
			secParams.Key = "";
			secParams.KeyLen = 0;
			secParams.Type = SL_WLAN_SEC_TYPE_OPEN;
			sl_WlanConnect("ssid_name", strlen("ssid_name"),0,&secParams,0);
	\endcode
*/
#if _SL_INCLUDE_FUNC(sl_WlanConnect)
_i16 sl_WlanConnect(const _i8*  pName,const  _i16 NameLen,const _u8 *pMacAddr,const SlWlanSecParams_t* pSecParams ,const SlWlanSecParamsExt_t* pSecExtParams);
#endif

/*!
    \brief Wlan disconnect

    Disconnect connection

    \return         Zero disconnected done successfully, other already disconnected

    \sa             sl_WlanConnect
    \note           belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_WlanDisconnect)
_i16 sl_WlanDisconnect(void);
#endif

/*!
    \brief Add profile

    When auto start is enabled, the device connects to a
    station from the profiles table. Up to 7 profiles (SL_WLAN_MAX_PROFILES) are
    supported.\n If several profiles configured the device chose
    the highest priority profile, within each priority group,
    device will chose profile based on security policy, signal
    strength, etc parameters.


    \param[in]      pName          Up to 32 bytes in case of STA the name is the
                                   SSID of the Access Point.\n
                                   In case of P2P the name is the remote device name.
    \param[in]      NameLen     Name length
    \param[in]      pMacAddr    6 bytes for MAC address
    \param[in]      pSecParams  Security parameters (use NULL key for SL_WLAN_SEC_TYPE_OPEN)\n
                                Security types options:
                                - SL_WLAN_SEC_TYPE_OPEN
                                - SL_WLAN_SEC_TYPE_WEP
								- SL_WLAN_SEC_TYPE_WEP_SHARED
                                - SL_WLAN_SEC_TYPE_WPA_WPA2
                                - SL_WLAN_SEC_TYPE_WPA_ENT
                                - SL_WLAN_SEC_TYPE_WPS_PBC
                                - SL_WLAN_SEC_TYPE_WPS_PIN

    \param[in]      pSecExtParams  Enterprise parameters - identity, identity length,
                                   Anonymous, Anonymous length, CertIndex (not supported,
                                   certificates need to be placed in a specific file ID),
                                   EapMethod.\n Use NULL in case Enterprise parameters is not in use

    \param[in]      Priority    Profile priority. Lowest priority: 0, Highest priority: 15.
    \param[in]      Options     Not supported

    \return         Profile stored index on success, or negative error code on failure.
    \par Persistent
					Profiles are <b>Persistent</b>
    \sa             sl_WlanProfileGet , sl_WlanProfileDel
    \note           belongs to \ref ext_api
    \warning        Only one Enterprise profile is supported.\n
                    Please Note that in case of adding an existing profile (compared by pName,pMACAddr and security type)
                    the old profile will be deleted and the same index will be returned.\n
                    SL_WLAN_SEC_TYPE_WPA is a deprecated definition, the new definition is SL_WLAN_SEC_TYPE_WPA_WPA2

*/
#if _SL_INCLUDE_FUNC(sl_WlanProfileAdd)
_i16 sl_WlanProfileAdd(const _i8*  pName,const  _i16 NameLen,const _u8 *pMacAddr,const SlWlanSecParams_t* pSecParams ,const SlWlanSecParamsExt_t* pSecExtParams,const _u32 Priority,const _u32  Options);
#endif

/*!
    \brief Get profile

    Read profile from the device

    \param[in]      Index          Profile stored index, if index does not exists error is return
    \param[out]     pName          Up to 32 bytes, in case of sta mode the name of the Access Point\n
                                   In case of p2p mode the name of the Remote Device
    \param[out]     pNameLen       Name length
    \param[out]     pMacAddr       6 bytes for MAC address
    \param[out]     pSecParams     Security parameters. Security types options: 
									- SL_WLAN_SEC_TYPE_OPEN
									- SL_WLAN_SEC_TYPE_WEP
									- SL_WLAN_SEC_TYPE_WEP_SHARED
									- SL_WLAN_SEC_TYPE_WPA_WPA2
									- SL_WLAN_SEC_TYPE_WPA_ENT
									- SL_WLAN_SEC_TYPE_WPS_PBC
									- SL_WLAN_SEC_TYPE_WPS_PIN
								   Key and key length are not return. In case of p2p security type pin the key refers to pin code
                                   return due to security reasons.
    \param[out]     pSecExtParams  Enterprise parameters - identity, identity
                                   length, Anonymous, Anonymous length
                                   CertIndex (not supported), EapMethod.
    \param[out]     pPriority      Profile priority

    \return         Profile security type is returned (0 or positive number) on success, or negative error code on failure

    \sa             sl_WlanProfileAdd , sl_WlanProfileDel
    \note           belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_WlanProfileGet)
_i16 sl_WlanProfileGet(const _i16 Index,_i8*  pName, _i16 *pNameLen, _u8 *pMacAddr, SlWlanSecParams_t* pSecParams, SlWlanGetSecParamsExt_t* pSecExtParams, _u32 *pPriority);
#endif

/*!
    \brief Delete WLAN profile

    Delete WLAN profile

    \param[in]   Index  number of profile to delete. Possible values are 0 to 6.\n
                 Index value SL_WLAN_DEL_ALL_PROFILES will delete all saved profiles

    \return  Zero on success or a negative error code on failure
    \par Persistent    
			Profile deletion is  <b>Persistent</b>
    \sa   sl_WlanProfileAdd , sl_WlanProfileGet
    \note           belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_WlanProfileDel)
_i16 sl_WlanProfileDel(const _i16 Index);
#endif

/*!
    \brief Set policy values

    \param[in]      Type      Type of policy to be modified. The Options are:
                              - SL_WLAN_POLICY_CONNECTION
                              - SL_WLAN_POLICY_SCAN
                              - SL_WLAN_POLICY_PM
                              - SL_WLAN_POLICY_P2P
    \param[in]      Policy    The option value which depends on action type
    \param[in]      pVal      An optional value pointer
    \param[in]      ValLen    An optional value length, in bytes
    \return         Zero on success or negative error code on failure.
    \par Persistent 
			All parameters are <b>System Persistent</b>\n
			Note that for SL_WLAN_POLICY_SCAN - only the interval will be saved.
		
    \sa             sl_WlanPolicyGet
    \note           belongs to \ref ext_api
    \warning
    \par	Example
    
	  <b>SL_WLAN_POLICY_CONNECTION: </b><br> defines options available to connect the CC31xx device to the AP: 
	  The options below could be combined to a single action, if more than one action is required. 

	- Auto Connect: If is set, the CC31xx device tries to automatically reconnect to one of its stored profiles,
	  each time the connection fails or the device is rebooted. To set this option, use: 
	\code	
		sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION,SL_WLAN_CONNECTION_POLICY(1,0,0,0),NULL,0) 
    \endcode
	<br>


	- Fast Connect: If  is set, the CC31xx device tries to establish a fast connection to AP. 
	  To set this option, use: 
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION,SL_WLAN_CONNECTION_POLICY(0,1,0,0),NULL,0)
    \endcode
	<br>

	- P2P: If is set (relevant for P2P mode only),  CC31xx/CC32xx device tries to automatically 
	  connect to the first P2P device available, supporting push button only. To set this option, use: 
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION,SL_WLAN_CONNECTION_POLICY(0,0,1,0),NULL,0)
    \endcode
	<br>

	- Auto Provisioning - If is set, the CC31xx device will automatically start the provisioning process 
	  after a long period of disconnection when profiles exist to set this option, use: 
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION,SL_WLAN_CONNECTION_POLICY(0,0,0,1),NULL,0)			
	\endcode \n
	

	<b>SL_WLAN_POLICY_SCAN:</b><br> defines system scan time interval. \nDefault interval is 10 minutes.
	After settings scan interval, an immediate scan is activated.\n The next scan will be based on the interval settings. 
	For AP scan, minimun interval is 10 seconds.

	-  With hidden SSID: For example, setting scan interval to 1 minute interval use including hidden ssid: 
	\code
		_u32 intervalInSeconds = 60;    
		sl_WlanPolicySet(SL_WLAN_POLICY_SCAN,SL_WLAN_SCAN_POLICY(1,1), (_u8 *)&intervalInSeconds,sizeof(intervalInSeconds)); 
    \endcode
	<br>

	-  No hidden SSID: setting scan interval to 1 minute interval use, not including hidden ssid: 
    \code
		_u32 intervalInSeconds = 60;    
		sl_WlanPolicySet(SL_WLAN_POLICY_SCAN,SL_WLAN_SCAN_POLICY(1,0), (_u8 *)&intervalInSeconds,sizeof(intervalInSeconds));
    \endcode
	<br>

	-  Disable scan:    
	\code
		#define SL_WLAN_DISABLE_SCAN 0
		_u32 intervalInSeconds = 0;
		sl_WlanPolicySet(SL_WLAN_POLICY_SCAN,SL_WLAN_DISABLE_SCAN,(_u8 *)&intervalInSeconds,sizeof(intervalInSeconds));
	\endcode 
	<br>

	<b>SL_WLAN_POLICY_PM: </b><br> defines a power management policy for Station mode only:
	-  Normal power management (default) policy use: 
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_NORMAL_POLICY, NULL,0)
	\endcode
	<br>

	- Low latency power management policy use: 
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_LOW_LATENCY_POLICY, NULL,0)
	\endcode
	<br>

	- Low power management policy use: 
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_LOW_POWER_POLICY, NULL,0) 
	\endcode
	<br>

	- Always on power management policy use: 
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_ALWAYS_ON_POLICY, NULL,0)
	\endcode
	<br>

	- Long Sleep Interval policy use: 
	\code		
	SlWlanPmPolicyParams_t PmPolicyParams;
	memset(&PmPolicyParams,0,sizeof(SlWlanPmPolicyParams_t));
	PmPolicyParams.MaxSleepTimeMs = 800;  //max sleep time in mSec
	sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_LONG_SLEEP_INTERVAL_POLICY, (_u8*)&PmPolicyParams,sizeof(PmPolicyParams)); 
	\endcode
	<br>

	<b>SL_WLAN_POLICY_P2P: </b><br> defines p2p negotiation policy parameters for P2P role:
	- To set intent negotiation value, set on of the following:\n
		SL_WLAN_P2P_ROLE_NEGOTIATE   - intent 3 \n
		SL_WLAN_P2P_ROLE_GROUP_OWNER - intent 15 \n
		SL_WLAN_P2P_ROLE_CLIENT      - intent 0 \n
    <br>
	- To set negotiation initiator value (initiator policy of first negotiation action frame), set on of the following: \n
		SL_WLAN_P2P_NEG_INITIATOR_ACTIVE \n
		SL_WLAN_P2P_NEG_INITIATOR_PASSIVE \n
		SL_WLAN_P2P_NEG_INITIATOR_RAND_BACKOFF \n
	\code
		sl_WlanPolicySet(SL_WLAN_POLICY_P2P, SL_WLAN_P2P_POLICY(SL_WLAN_P2P_ROLE_NEGOTIATE,SL_WLAN_P2P_NEG_INITIATOR_RAND_BACKOFF),NULL,0);
	\endcode
*/
#if _SL_INCLUDE_FUNC(sl_WlanPolicySet)
_i16 sl_WlanPolicySet(const _u8 Type , const _u8 Policy, _u8 *pVal,const _u8 ValLen);
#endif
/*!
    \brief Get policy values

    \param[in]      Type     
						- SL_WLAN_POLICY_CONNECTION 
						- SL_WLAN_POLICY_SCAN 
						- SL_WLAN_POLICY_PM, SL_WLAN_POLICY_P2P
    \param[out]     pPolicy   	argument may be set to any value 
    \param[out]     pVal 		The returned values, depends on each policy type, will be stored in the allocated buffer pointed by pVal
                    with a maximum buffer length set by the calling function and pointed to by argument *pValLen
    \param[out]     pValLen		actual value lenght
    \return         Zero on success, or negative error code on failure

    \sa             sl_WlanPolicySet

    \note           belongs to \ref ext_api

    \warning        The value pointed by the argument *pValLen should be set to a value different from 0 and
                    greater than the buffer length returned from the SL device. Otherwise, an error will be returned.

	\par Example
    
	- SL_WLAN_POLICY_CONNECTION - Get connection policy:
	\code
    _u8 Policy = 0;
    int length = sizeof(PolicyOption);
    int ret;
    ret = sl_WlanPolicyGet(SL_WLAN_POLICY_CONNECTION ,&Policy,0,(_u8*)&length);
               
    if (Policy & SL_WLAN_CONNECTION_POLICY(  1  ,  1  ,  0   ,   0 )   )
    {
        printf("Connection Policy is set to Auto + Fast");
    }
	\endcode
	<br>

	- SL_WLAN_POLICY_SCAN - Get scan policy:
	\code                
    int ScanInterval = 0;  //default value is 600 seconds
    _u8 Policy = 0;       //default value is 0 (disabled)
    int ret;
    length = sizeof(ScanInterval);
    ret = sl_WlanPolicyGet(SL_WLAN_POLICY_SCAN ,&Policy,(_u8*)&ScanInterval,(_u8*)&length);
      
    if (Policy & SL_WLAN_SCAN_POLICY(   0  ,  1   )   )
    {
        printf("Scan Policy is set to Scan visible ssid ");
    }
    if (Policy & SL_WLAN_SCAN_POLICY(   1  ,  0   )   )
    {
        printf("Scan Policy is set to Scan hidden ssid ");
    }
	\endcode
	<br>

	- SL_WLAN_POLICY_PM - Get power management policy:
	\code
    _u8 Policy = 0;
    int ret;
    SlWlanPmPolicyParams_t PmPolicyParams;
    length = sizeof(PmPolicyParams);
    ret = sl_WlanPolicyGet(SL_POLICY_PM ,&Policy,&PmPolicyParams,(_u8*)&length);
    if (Policy ==  SL_WLAN_LONG_SLEEP_INTERVAL_POLICY )
    {
        printf("Connection Policy is set to LONG SLEEP INTERVAL POLICY with inteval = %d ",PmPolicyParams.MaxSleepTimeMs);
    }
	\endcode
	<br>

	-  SL_WLAN_POLICY_P2P - Get P2P policy:
	\code       
    _u8 Policy = 0;
    int ret;
    length = sizeof(Policy);
    ret = sl_WlanPolicyGet(SL_WLAN_POLICY_P2P ,&Policy,0,(_u8*)&length);
                //SL_WLAN_P2P_POLICY(p2pNegType           , p2pNegInitiator)
    if (Policy &  SL_WLAN_P2P_POLICY(0,SL_WLAN_P2P_NEG_INITIATOR_RAND_BACKOFF)   )
    {
        printf("P2P Policy is set to Rand backoff");
    }
                        if (Policy &  SL_WLAN_P2P_POLICY(SL_WLAN_P2P_ROLE_NEGOTIATE,0)   )
    {
        printf("P2P Policy is set to Role Negotiate");
    }
	\endcode
	<br>

*/
#if _SL_INCLUDE_FUNC(sl_WlanPolicyGet)
_i16 sl_WlanPolicyGet(const _u8 Type ,_u8 *pPolicy,_u8 *pVal,_u8 *pValLen);
#endif
/*!
    \brief Gets the WLAN scan operation results

    Gets scan results , gets entry from scan result table

    \param[in]   Index  	Starting index identifier (range 0-29) for getting scan results
    \param[in]   Count  	How many entries to fetch. Max is (30-"Index").
    \param[out]  pEntries  	Pointer to an allocated SlWlanNetworkEntry_t.
                            The number of array items should match "Count" \n
                            sec_type: 
								- SL_WLAN_SCAN_SEC_TYPE_OPEN 
								- SL_WLAN_SCAN_SEC_TYPE_WEP 
								- SL_WLAN_SCAN_SEC_TYPE_WPA 
								- SL_WLAN_SCAN_SEC_TYPE_WPA2

	\return  Number of valid networks list items
    \sa
    \note       belongs to \ref ext_api
    \warning    This command do not initiate any active scanning action
    \par        Example

	- Fetching max 10 results:
    \code       
    SlWlanNetworkEntry_t netEntries[10];
    _u8 i;
    _i16 resultsCount = sl_WlanGetNetworkList(0,10,&netEntries[0]);
    for(i=0; i< resultsCount; i++)
    {
        printf("%d. ",i+1);
        printf("SSID: %.32s        ",netEntries[i].Ssid);
        printf("BSSID: %x:%x:%x:%x:%x:%x    ",netEntries[i].Bssid[0],netEntries[i].Bssid[1],netEntries[i].Bssid[2],netEntries[i].Bssid[3],netEntries[i].Bssid[4],netEntries[i].Bssid[5]);
        printf("Channel: %d    ",netEntries[i].Channel);
        printf("RSSI: %d    ",netEntries[i].Rssi);
        printf("Security type: %d    ",SL_WLAN_SCAN_RESULT_SEC_TYPE_BITMAP(netEntries[i].SecurityInfo));
        printf("Group Cipher: %d    ",SL_WLAN_SCAN_RESULT_GROUP_CIPHER(netEntries[i].SecurityInfo));
        printf("Unicast Cipher bitmap: %d    ",SL_WLAN_SCAN_RESULT_UNICAST_CIPHER_BITMAP(netEntries[i].SecurityInfo));
        printf("Key Mgmt suites bitmap: %d    ",SL_WLAN_SCAN_RESULT_KEY_MGMT_SUITES_BITMAP(netEntries[i].SecurityInfo));
        printf("Hidden SSID: %d\r\n",SL_WLAN_SCAN_RESULT_HIDDEN_SSID(netEntries[i].SecurityInfo));
    }
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_WlanGetNetworkList)
_i16 sl_WlanGetNetworkList(const _u8 Index,const  _u8 Count, SlWlanNetworkEntry_t *pEntries);
#endif

/*!
    \brief   Start collecting wlan RX statistics, for unlimited time.

	\par Parameters 
		None
    \return  Zero on success, or negative error code on failure

    \sa     sl_WlanRxStatStop      sl_WlanRxStatGet
    \note   Belongs to \ref ext_api
    \warning  This API is deprecated and should be removed for next release
    \par        Example

    - Getting wlan RX statistics:
	\code
	void RxStatCollectTwice()
	{
		SlWlanGetRxStatResponse_t rxStat;
		_i16 rawSocket;
		_i8 DataFrame[200];
		struct SlTimeval_t timeval;
		timeval.tv_sec =  0;             // Seconds
		timeval.tv_usec = 20000;         // Microseconds. 10000 microseconds resolution

		sl_WlanRxStatStart();  // set statistics mode

		rawSocket = sl_Socket(SL_AF_RF, SL_SOCK_RAW, eChannel);
		// set timeout - in case we have no activity for the specified channel
		sl_SetSockOpt(rawSocket,SL_SOL_SOCKET,SL_SO_RCVTIMEO, &timeval, sizeof(timeval));    // Enable receive timeout
		status = sl_Recv(rawSocket, DataFrame, sizeof(DataFrame), 0);

		Sleep(1000); // sleep for 1 sec
		sl_WlanRxStatGet(&rxStat,0); // statistics has been cleared upon read
		Sleep(1000); // sleep for 1 sec
		sl_WlanRxStatGet(&rxStat,0);

	}
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_WlanRxStatStart)
_i16 sl_WlanRxStatStart(void);
#endif

/*!
    \brief    Stop collecting wlan RX statistic, (if previous called sl_WlanRxStatStart)

	\par 	Parameters
			None
    \return   Zero on success, or negative error code on failure

    \sa     sl_WlanRxStatStart      sl_WlanRxStatGet
    \note           Belongs to \ref ext_api
    \warning  This API is deprecated and should be removed for next release
*/
#if _SL_INCLUDE_FUNC(sl_WlanRxStatStop)
_i16 sl_WlanRxStatStop(void);
#endif


/*!
    \brief Get wlan RX statistics. Upon calling this command, the statistics counters will be cleared.

    \param[in]  pRxStat 	Pointer to SlWlanGetRxStatResponse_t filled with Rx statistics results
    \param[in]  Flags 		Should be 0  ( not applicable right now, will be added the future )
    \return     Zero on success, or negative error code on failure

    \sa   sl_WlanRxStatStart  sl_WlanRxStatStop
    \note           Belongs to \ref ext_api
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_WlanRxStatGet)
_i16 sl_WlanRxStatGet(SlWlanGetRxStatResponse_t *pRxStat,const _u32 Flags);
#endif


/*!
    \brief  The simpleLink will switch to the appropriate role according to the provisioning mode requested
            and will start the provisioning process.

    \param[in]  ProvisioningCmd            
											- SL_WLAN_PROVISIONING_CMD_START_MODE_AP							0: Start AP provisioning (AP role)
                                            - SL_WLAN_PROVISIONING_CMD_START_MODE_SC							1: Start Smart Config provisioning (STA role)
                                            - SL_WLAN_PROVISIONING_CMD_START_MODE_APSC						2: Start AP+Smart Config provisioning (AP role)
                                            - SL_WLAN_PROVISIONING_CMD_START_MODE_APSC_EXTERNAL_CONFIGURATION 3: Start AP + Smart Config + WAC provisioning (AP role)
                                            - SL_WLAN_PROVISIONING_CMD_STOP                          4: Stop provisioning
                                            - SL_WLAN_PROVISIONING_CMD_ABORT_EXTERNAL_CONFIGURATIONC 5:
    \param[in]  RequestedRoleAfterSuccess   The role that the SimpleLink will switch to in case of a successful provisioning.
                                            0: STA
                                            2: AP
                                            0xFF: stay in current role (relevant only in provisioning_stop)
    \param[in]  InactivityTimeoutSec -      The period of time (in seconds) the system waits before it automatically
                                            stops the provisioning process when no activity is detected.
                                            set to 0 in order to stop provisioning. Minimum InactivityTimeoutSec is 30 seconds.
    \param[in]  pSmartConfigKey             Smart Config key: public key for smart config process (relevent for smart config only)
    \param[in]  Flags                       Can have the following values:
                                    		       - SL_WLAN_PROVISIONING_CMD_FLAG_EXTERNAL_CONFIRMATION - Confirmation phase will be completed externaly by host (e.g. via cloud assist)


    \return     Zero on success, or negative error code on failure

    \sa
    \warning
    \par       Example

	- Start Provisioning - start as STA after success with inactivity timeout of 10 minutes:
	\code
    sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_START_MODE_APSC, ROLE_STA, 600, "Key0Key0Key0Key0", 0x0);
	\endcode
	<br>

    - Stop Provisioning:
    \code
	sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP,0xFF,0,NULL, 0x0);
	\endcode
	<br>

    - Start AP Provisioning with inactivity timeout of 10 minutes
    \code
	sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_START_MODE_APSC,ROLE_AP,600,NULL, 0x0);
	\endcode
	<br>

    - Start AP Provisioning with inactivity timeout of 10 minutes and complete confirmation via user cloud assist
	\code
    sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_START_MODE_APSC, ROLE_AP, 600, NULL, SL_WLAN_PROVISIONING_CMD_FLAG_EXTERNAL_CONFIRMATION);
    \endcode
	<br>

*/

#if _SL_INCLUDE_FUNC(sl_WlanProvisioning)
_i16 sl_WlanProvisioning(_u8 ProvisioningCmd, _u8 RequestedRoleAfterSuccess, _u16 InactivityTimeoutSec,  char *pSmartConfigKey, _u32 Flags);
#endif



/*!
    \brief Wlan set mode

    Setting WLAN mode

    \param[in] Mode  WLAN mode to start the CC31xx device. Possible options are
                    - ROLE_STA - for WLAN station mode
                    - ROLE_AP  - for WLAN AP mode
                    - ROLE_P2P  -for WLAN P2P mode
    \return   Zero on success, or negative error code on failure
    \par Persistent 
		Mode is <b>Persistent</b>
    \sa        sl_Start sl_Stop
    \note           Belongs to \ref ext_api
    \warning   After setting the mode the system must be restarted for activating the new mode
    \par       Example

	- Switch from any role to STA:
    \code
		sl_WlanSetMode(ROLE_STA);
		sl_Stop(0);
		sl_Start(NULL,NULL,NULL);
    \endcode

*/
#if _SL_INCLUDE_FUNC(sl_WlanSetMode)
_i16 sl_WlanSetMode(const _u8  Mode);
#endif


/*!
    \brief     Setting WLAN configurations

    \param[in] ConfigId -  configuration id
                          - <b>SL_WLAN_CFG_AP_ID</b>
                          - <b>SL_WLAN_CFG_GENERAL_PARAM_ID</b>
                          - <b>SL_WLAN_CFG_P2P_PARAM_ID</b>
                          - <b>SL_WLAN_RX_FILTERS_ID</b>

    \param[in] ConfigOpt - configurations option
                          - <b>SL_WLAN_CFG_AP_ID</b>
                              - <b>SL_WLAN_AP_OPT_SSID</b> \n
                                      Set SSID for AP mode. \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_OPT_CHANNEL</b> \n
                                      Set channel for AP mode. \n
                                      The channel is dependant on the country code which is set. i.e. for "US" the channel should be in the range of [1-11] \n
                                      This option takes <b>_u8</b> as a parameter
                              - <b>SL_WLAN_AP_OPT_HIDDEN_SSID</b> \n
                                      Set Hidden SSID Mode for AP mode.Hidden options: \n
                                         0: disabled \n
                                         1: Send empty (length=0) SSID in beacon and ignore probe request for broadcast SSID \n
                                         2: Clear SSID (ASCII 0), but keep the original length (this may be required with some \n
                                            clients that do not support empty SSID) and ignore probe requests for broadcast SSID \n
                                      This option takes <b>_u8</b> as a parameter
                              - <b>SL_WLAN_AP_OPT_SECURITY_TYPE</b> \n
                                      Set Security type for AP mode. Security options are:
                                      - Open security: SL_WLAN_SEC_TYPE_OPEN
                                      - WEP security:  SL_WLAN_SEC_TYPE_WEP
                                      - WPA security:  SL_WLAN_SEC_TYPE_WPA_WPA2  \n
                                      This option takes <b>_u8</b> pointer as a parameter
                              - <b>SL_WLAN_AP_OPT_PASSWORD</b> \n
                                      Set Password for for AP mode (for WEP or for WPA): \n
                                      Password - for WPA: 8 - 63 characters \n
                                      for WEP: 5 / 13 characters (ascii) \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_OPT_MAX_STATIONS</b> \n
                                      Set Max AP stations - 1..4 - Note: can be less than the number of currently connected stations \n
                                      max_stations - 1 characters \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_OPT_MAX_STA_AGING</b> \n
                                      Set Max station aging time - default is 60 seconds \n
                                      max_stations - 2 characters \n
                                      This options takes <b>_u16</b> buffer as parameter
                              - <b>SL_WLAN_AP_ACCESS_LIST_MODE</b> \n
                                      Set AP access list mode - DISABLE, DENY_LIST \n
                                      mode - 1 characters \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_ACCESS_LIST_ADD_MAC</b> \n
                                      Add MAC address to the AP access list: \n
                                      mac_addr - 6 characters \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_ACCESS_LIST_DEL_MAC</b> \n
                                      Del MAC address from the AP access list: \n
                                      mac_addr - 6 characters \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_ACCESS_LIST_DEL_IDX</b> \n
                                      Delete MAC address from index in the AP access list: \n
                                      index - 1 character \n
                                      This options takes <b>_u8</b> buffer as parameter

                          - <b>SL_WLAN_CFG_GENERAL_PARAM_ID</b>
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE</b> \n
                                      Set Country Code for AP mode \n
                                      This options takes <b>_u8</b> 2 bytes buffer as parameter
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_STA_TX_POWER</b> \n
                                      Set STA mode Tx power level \n
                                      Number between 0-15, as dB offset from max power (0 will set MAX power) \n
                                      This options takes <b>_u8</b> as parameter
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_AP_TX_POWER</b>
                                      Set AP mode Tx power level \n
                                      Number between 0-15, as dB offset from max power (0 will set MAX power) \n
                                      This options takes <b>_u8</b> as parameter
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_INFO_ELEMENT</b>
                                      Set Info Element for AP mode. \n
                                      The Application can set up to SL_WLAN_MAX_PRIVATE_INFO_ELEMENTS_SUPPROTED info elements per Role (AP / P2P GO).  \n
                                      To delete an info element use the relevant index and length = 0. \n
                                      For AP - no more than SL_WLAN_INFO_ELEMENT_MAX_TOTAL_LENGTH_AP bytes can be stored for all info elements. \n
                                      For P2P GO - no more than SL_WLAN_INFO_ELEMENT_MAX_TOTAL_LENGTH_P2P_GO bytes can be stored for all info elements.  \n
                                      This option takes SlWlanSetInfoElement_t as parameter
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS</b>
                                      Set scan parameters: RSSI threshold and channel mask.
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_SUSPEND_PROFILES</b>
                                      Set suspended profiles mask (set bits 2 and 4 to suspend profiles 2 and 4).
							   - <b>SL_WLAN_GENERAL_PARAM_DISABLE_ENT_SERVER_AUTH</b>
                                      This option enables to skip server authentication and is valid for one
									  use, when manually connection to an enterprise network
									  

                          - <b>SL_WLAN_CFG_P2P_PARAM_ID</b>
                              - <b>SL_WLAN_P2P_OPT_DEV_TYPE</b> \n
                                      Set P2P Device type.Maximum length of 17 characters. Device type is published under P2P I.E, \n
                                      allows to make devices easier to recognize. \n
                                      In case no device type is set, the default type is "1-0050F204-1"  \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_P2P_OPT_CHANNEL_N_REGS</b> \n
                                     Set P2P Channels. \n
                                     listen channel (either 1/6/11 for 2.4GHz) \n
                                     listen regulatory class (81 for 2.4GHz)   \n
                                     oper channel (either 1/6/11 for 2.4GHz)   \n
                                     oper regulatory class (81 for 2.4GHz)     \n
                                     listen channel and regulatory class will determine the device listen channel during p2p find listen phase \n
                                     oper channel and regulatory class will determine the operating channel preferred by this device (in case it is group owner this will be the operating channel) \n
                                     channels should be one of the social channels (1/6/11). In case no listen/oper channel selected, a random 1/6/11 will be selected.
                                     This option takes pointer to <b>_u8[4]</b> as parameter

                          - <b>SL_WLAN_RX_FILTERS_ID</b>
                              - <b>SL_WLAN_RX_FILTER_STATE</b> \n
                                      Enable or disable filters. The buffer input is SlWlanRxFilterOperationCommandBuff_t\n
                              - <b>SL_WLAN_RX_FILTER_SYS_STATE</b> \n
                                      Enable or disable system filters. The buffer input is SlWlanRxFilterSysFiltersSetStateBuff_t\n
                              - <b>SL_WLAN_RX_FILTER_REMOVE</b> \n
                                      Remove filters. The buffer input is SlWlanRxFilterOperationCommandBuff_t\n
                              - <b>SL_WLAN_RX_FILTER_STORE</b> \n
                                      Save the filters as persistent. \n
                              - <b>SL_WLAN_RX_FILTER_UPDATE_ARGS</b> \n
                                      Update filter arguments. The buffer input is SlWlanRxFilterUpdateArgsCommandBuff_t\n

    \param[in] ConfigLen - configurations len

    \param[in] pValues -   configurations values
	
    \return    Zero on success, or negative error code on failure

    \par Persistent
                        <b>System Persistent</b>:		
								- SL_WLAN_CFG_GENERAL_PARAM_ID
								- SL_WLAN_CFG_P2P_PARAM_ID
								
					    <b>Reset</b>:
								- SL_WLAN_CFG_AP_ID
								
						<b>Non- Persistent</b>:
						  		- SL_WLAN_GENERAL_PARAM_DISABLE_ENT_SERVER_AUTH
    \sa
    \note
    \warning
    \par   Examples

	- SL_WLAN_AP_OPT_SSID:
    \code
		_u8  str[33];
		memset(str, 0, 33);
		memcpy(str, ssid, len);  // ssid string of 32 characters
		sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SSID, strlen(ssid), str);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_CHANNEL:
    \code 
		_u8  val = channel;
		sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_CHANNEL, 1, (_u8 *)&val);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_HIDDEN_SSID:
    \code
        _u8  val = hidden;
        sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_HIDDEN_SSID, 1, (_u8 *)&val);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_SECURITY_TYPE:
    \code
        _u8  val = SL_WLAN_SEC_TYPE_WPA_WPA2;
        sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SECURITY_TYPE, 1, (_u8 *)&val);
    \endcode
    <br>

	- SL_WLAN_AP_OPT_PASSWORD: 
	\code
        _u8  str[65];
        _u16  len = strlen(password);
        memset(str, 0, 65);
        memcpy(str, password, len);
        sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_PASSWORD, len, (_u8 *)str);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_MAX_STATIONS:
    \code
        _u8 max_ap_stations = 3;
        sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_MAX_STATIONS, sizeof(max_ap_stations), (_u8 *)&max_ap_stations);
    \endcode
	<br>
    
	- SL_WLAN_AP_OPT_MAX_STA_AGING:
	\code
        _u16 max_ap_sta_aging = 60;
        sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_MAX_STA_AGING, sizeof(max_ap_sta_aging), (_u8 *)&max_ap_sta_aging);
    \endcode
	<br>

	- SL_WLAN_AP_ACCESS_LIST_MODE:
    \code
		_u8  access list_mode = SL_WLAN_AP_ACCESS_LIST_MODE_DENY_LIST;
		sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_ACCESS_LIST_MODE, sizeof(access list_mode), (_u8 *)&access list_mode);
    \endcode
	<br>

	- SL_WLAN_AP_ACCESS_LIST_ADD_MAC:
    \code
		_u8  sta_mac[6] = { 0x00, 0x22, 0x33, 0x44, 0x55, 0x66 };
		sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_ACCESS_LIST_ADD_MAC, sizeof(sta_mac), (_u8 *)&sta_mac);
    \endcode
	<br>

	- SL_WLAN_AP_ACCESS_LIST_DEL_MAC:
	\code
		_u8  sta_mac[6] = { 0x00, 0x22, 0x33, 0x44, 0x55, 0x66 };
		sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_ACCESS_LIST_DEL_MAC, sizeof(sta_mac), (_u8 *)&sta_mac);
    \endcode
    <br>

	- SL_WLAN_AP_ACCESS_LIST_DEL_IDX:
	\code
		_u8  sta_index = 0;
		sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_ACCESS_LIST_DEL_IDX, sizeof(sta_index), (_u8 *)&sta_index);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_STA_TX_POWER:
    \code
		_u8  stapower=(_u8)power;
		sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, SL_WLAN_GENERAL_PARAM_OPT_STA_TX_POWER,1,(_u8 *)&stapower);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE:
    \code
		_u8*  str = (_u8 *) country;  // string of 2 characters. i.e. - "US"
		sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE, 2, str);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_AP_TX_POWER:
    \code
		_u8  appower=(_u8)power;
		sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, SL_WLAN_GENERAL_PARAM_OPT_AP_TX_POWER,1,(_u8 *)&appower);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_SUSPEND_PROFILES
    \code
		_u32  suspendedProfilesMask=(_u32)mask;
		sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, SL_WLAN_GENERAL_PARAM_OPT_SUSPEND_PROFILES,sizeof(suspendedProfilesMask),(_u32 *)&suspendedProfilesMask);
    \endcode
    <br>

	- SL_WLAN_P2P_OPT_DEV_TYPE:
	\code
		_u8   str[17];
		_u16  len = strlen(device_type);
		memset(str, 0, 17);
		memcpy(str, device_type, len);
		sl_WlanSet(SL_WLAN_CFG_P2P_PARAM_ID, SL_WLAN_P2P_OPT_DEV_TYPE, len, str);
    \endcode
	<br>

	- SL_WLAN_P2P_OPT_CHANNEL_N_REGS
    \code
		_u8  str[4];
		str[0] = (_u8)11;           // listen channel
		str[1] = (_u8)81;           // listen regulatory class
		str[2] = (_u8)6;            // oper channel
		str[3] = (_u8)81;           // oper regulatory class
		sl_WlanSet(SL_WLAN_CFG_P2P_PARAM_ID, SL_WLAN_P2P_OPT_CHANNEL_N_REGS, 4, str);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_INFO_ELEMENT:
     \code
		SlWlanSetInfoElement_t    infoele;
		infoele.Index     = Index;                  // Index of the info element. range: 0 - SL_WLAN_MAX_PRIVATE_INFO_ELEMENTS_SUPPROTED
		infoele.Role      = Role;                   // SL_WLAN_INFO_ELEMENT_AP_ROLE (0) or SL_WLAN_INFO_ELEMENT_P2P_GO_ROLE (1)
		infoele.IE.Id     =  Id;                    // Info element ID. if SL_WLAN_INFO_ELEMENT_DEFAULT_ID (0) is set, ID will be set to 221.
		// Organization unique ID. If all 3 bytes are zero - it will be replaced with 08,00,28.
		infoele.IE.Oui[0] =  Oui0;                  // Organization unique ID first Byte
		infoele.IE.Oui[1] =  Oui1;                  // Organization unique ID second Byte
		infoele.IE.Oui[2] =  Oui2;                  // Organization unique ID third Byte
		infoele.IE.Length = Len;                    // Length of the info element. must be smaller than 253 bytes
		memset(infoele.IE.Data, 0, SL_WLAN_INFO_ELEMENT_MAX_SIZE);
		if ( Len <= SL_WLAN_INFO_ELEMENT_MAX_SIZE )
		{
			memcpy(infoele.IE.Data, IE, Len);           // Info element. length of the info element is [0-252]
			sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,SL_WLAN_GENERAL_PARAM_OPT_INFO_ELEMENT,sizeof(SlWlanSetInfoElement_t),(_u8* ) &infoele);
		}
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS:
	\code
        SlWlanScanParamCommand_t ScanParamConfig;
        ScanParamConfig.RssiThershold = -70;
        ScanParamConfig.ChannelsMask = 0x1FFF;
        sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS,sizeof(ScanParamConfig),(_u8* ) &ScanParamConfig);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_DISABLE_ENT_SERVER_AUTH:
	\code
		_u8 param = 1; // 1 means disable the server authentication
		sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,SL_WLAN_GENERAL_PARAM_DISABLE_ENT_SERVER_AUTH,1,&param);
	\endcode
	<br>
	- SL_WLAN_RX_FILTER_STORE: 
	\code
     sl_WlanSet(SL_WLAN_RX_FILTERS_ID, SL_WLAN_RX_FILTER_STORE, 0, NULL);
	\endcode

*/
#if _SL_INCLUDE_FUNC(sl_WlanSet)
_i16 sl_WlanSet(const _u16 ConfigId ,const _u16 ConfigOpt,const _u16 ConfigLen,const  _u8 *pValues);
#endif

/*!
    \brief     Getting WLAN configurations

    \param[in] ConfigId -  configuration id
                          - <b>SL_WLAN_CFG_AP_ID</b>
                          - <b>SL_WLAN_CFG_GENERAL_PARAM_ID</b>
                          - <b>SL_WLAN_CFG_P2P_PARAM_ID</b>
                          - <b>SL_WLAN_CFG_AP_ACCESS_LIST_ID</b>
                          - <b>SL_WLAN_RX_FILTERS_ID</b>

    \param[out] pConfigOpt - get configurations option
                          - <b>SL_WLAN_CFG_AP_ID</b>
                              - <b>SL_WLAN_AP_OPT_SSID</b> \n
                                      Get SSID for AP mode. \n
                                      Get up to 32 characters of SSID \n
                                      This options takes <b>_u8</b> as parameter
                              - <b>SL_WLAN_AP_OPT_CHANNEL</b> \n
                                      Get channel for AP mode. \n
                                      This option takes <b>_u8</b> as a parameter
                              - <b>SL_WLAN_AP_OPT_HIDDEN_SSID</b> \n
                                      Get Hidden SSID Mode for AP mode.Hidden options: \n
                                         0: disabled \n
                                         1: Send empty (length=0) SSID in beacon and ignore probe request for broadcast SSID \n
                                         2: Clear SSID (ASCII 0), but keep the original length (this may be required with some \n
                                            clients that do not support empty SSID) and ignore probe requests for broadcast SSID \n
                                      This option takes <b>_u8</b> as a parameter
                              - <b>SL_WLAN_AP_OPT_SECURITY_TYPE</b> \n
                                      Get Security type for AP mode. Security options are:
                                      - Open security: SL_WLAN_SEC_TYPE_OPEN
                                      - WEP security:  SL_WLAN_SEC_TYPE_WEP
                                      - WPA security:  SL_WLAN_SEC_TYPE_WPA_WPA2  \n
                                      This option takes <b>_u8</b> as a parameter
                              - <b>SL_WLAN_AP_OPT_PASSWORD</b> \n
                                      Get Password for for AP mode (for WEP or for WPA): \n
                                      Returns password - string, fills up to 64 characters. \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_OPT_MAX_STATIONS</b> \n
                                      Get Max AP allowed stations: \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_AP_OPT_MAX_STA_AGING</b> \n
                                      Get AP aging time in seconds: \n
                                      This options takes <b>_u16</b> buffer as parameter
                              - <b>SL_WLAN_AP_ACCESS_LIST_NUM_ENTRIES</b> \n
                                      Get AP access list number of entries: \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_CFG_AP_ACCESS_LIST_ID</b>
                                -  The option is the start index in the access list \n
                                      Get the AP access list from start index, the number of entries in the list is extracted from the request length.
                          - <b>SL_WLAN_CFG_GENERAL_PARAM_ID</b>
                              - <b> SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS </b> \n
                                      Get scan parameters.
                                      This option uses SlWlanScanParamCommand_t as parameter
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE</b> \n
                                      Get Country Code for AP mode \n
                                      This options takes <b>_u8</b> buffer as parameter
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_STA_TX_POWER</b> \n
                                      Get STA mode Tx power level \n
                                      Number between 0-15, as dB offset from max power (0 indicates MAX power) \n
                                      This options takes <b>_u8</b> as parameter
                              - <b>SL_WLAN_GENERAL_PARAM_OPT_AP_TX_POWER</b>
                                      Get AP mode Tx power level \n
                                      Number between 0-15, as dB offset from max power (0 indicates MAX power) \n
                                      This options takes <b>_u8</b> as parameter
                          - <b>SL_WLAN_CFG_P2P_PARAM_ID</b>
                              - <b>SL_WLAN_P2P_OPT_CHANNEL_N_REGS</b> \n
                                     Get P2P Channels. \n
                                     listen channel (either 1/6/11 for 2.4GHz) \n
                                     listen regulatory class (81 for 2.4GHz)   \n
                                     oper channel (either 1/6/11 for 2.4GHz)   \n
                                     oper regulatory class (81 for 2.4GHz)     \n
                                     listen channel and regulatory class will determine the device listen channel during p2p find listen phase \n
                                     oper channel and regulatory class will determine the operating channel preferred by this device (in case it is group owner this will be the operating channel) \n
                                     channels should be one of the social channels (1/6/11). In case no listen/oper channel selected, a random 1/6/11 will be selected. \n
                                     This option takes pointer to <b>_u8[4]</b> as parameter
                          - <b>SL_WLAN_RX_FILTERS_ID</b>
                                - <b>SL_WLAN_RX_FILTER_STATE</b> \n
                                    Retrieves the filters enable/disable status. The buffer input is SlWlanRxFilterRetrieveStateBuff_t \n
                                - <b>SL_WLAN_RX_FILTER_SYS_STATE</b> \n
                                    Retrieves the system filters enable/disable status. The buffer input is SlWlanRxFilterSysFiltersRetrieveStateBuff_t:

    \param[out] pConfigLen - The length of the allocated memory as input, when the
                                        function complete, the value of this parameter would be
                                        the len that actually read from the device.
                                        If the device return length that is longer from the input
                                        value, the function will cut the end of the returned structure
                                        and will return SL_ESMALLBUF.


    \param[out] pValues - get configurations values
    \return    Zero on success, or negative error code on failure
    \sa   sl_WlanSet
    \note
			In case the device was started as AP mode, but no SSID was set, the Get SSID will return "mysimplelink" and not "mysimplelink-xxyyzz"
    \warning
    \par    Examples
    
	- SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS:
	\code
		SlWlanScanParamCommand_t ScanParamConfig;
		_u16 Option = SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS;
		_u16 OptionLen = sizeof(SlWlanScanParamCommand_t);
		sl_WlanGet(SL_WLAN_CFG_GENERAL_PARAM_ID ,&Option,&OptionLen,(_u8 *)&ScanParamConfig);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_AP_TX_POWER:
    \code
        _i32 TXPower = 0;
        _u16 Option = SL_WLAN_GENERAL_PARAM_OPT_AP_TX_POWER;
        _u16 OptionLen = sizeof(TXPower);
        sl_WlanGet(SL_WLAN_CFG_GENERAL_PARAM_ID ,&Option,&OptionLen,(_u8 *)&TXPower);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPTSTA_TX_POWER:
    \code
		_i32 TXPower = 0;
		_u16 Option = SL_WLAN_GENERAL_PARAM_OPT_STA_TX_POWER;
		_u16 OptionLen = sizeof(TXPower);
		sl_WlanGet(SL_WLAN_CFG_GENERAL_PARAM_ID ,&Option,&OptionLen,(_u8 *)&TXPower);
    \endcode
	<br>

	- SL_WLAN_P2P_OPT_DEV_TYPE:
    \code
        _i8 device_type[18];
        _u16 len = 18;
        _u16 config_opt = SL_WLAN_P2P_OPT_DEV_TYPE;
        sl_WlanGet(SL_WLAN_CFG_P2P_PARAM_ID, &config_opt , &len, (_u8* )device_type);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_SSID:
    \code
        _i8 ssid[33];
        _u16 len = 33;
		sl_Memset(ssid,0,33);
        _u16  config_opt = SL_WLAN_AP_OPT_SSID;
        sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (_u8* )ssid);
    \endcode
	<br>

	- SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE:
    \code
		_i8 country[3];
		_u16 len = 3;
		_u16  config_opt = SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE;
		sl_WlanGet(SL_WLAN_CFG_GENERAL_PARAM_ID, &config_opt, &len, (_u8* )country);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_CHANNEL:
    \code
		_i8 channel;
		_u16 len = 1;
		_u16  config_opt = SL_WLAN_AP_OPT_CHANNEL;
		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8* )&channel);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_HIDDEN_SSID:
    \code
		_u8 hidden;
		_u16 len = 1;
		_u16  config_opt = SL_WLAN_AP_OPT_HIDDEN_SSID;
		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8* )&hidden);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_SECURITY_TYPE:
    \code
		_u8 sec_type;
		_u16 len = 1;
		_u16  config_opt = SL_WLAN_AP_OPT_SECURITY_TYPE;
		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8* )&sec_type);
    \endcode
	<br>
    
	- SL_WLAN_AP_OPT_PASSWORD:
	\code
		_u8 password[64];
		_u16 len = 64;
		sl_Memset(password,0,64);
		_u16 config_opt = SL_WLAN_AP_OPT_PASSWORD;
		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8* )password);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_MAX_STATIONS:
    \code
		_u8 max_ap_stations
		_u16 len = 1;
		_u16  config_opt = SL_WLAN_AP_OPT_MAX_STATIONS;
		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8 *)&max_ap_stations);
    \endcode
	<br>

	- SL_WLAN_AP_OPT_MAX_STA_AGING:
    \code
		_u16 ap_sta_aging;
		_u16 len = 2;
		_u16  config_opt = SL_WLAN_AP_OPT_MAX_STA_AGING;
		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8 *)&ap_sta_aging);
    \endcode
	<br>
    
	- SL_WLAN_AP_ACCESS_LIST_NUM_ENTRIES:
	\code
		_u8 aclist_num_entries;
		_u16 config_opt = SL_WLAN_AP_ACCESS_LIST_NUM_ENTRIES;
		_u16 len = sizeof(aclist_num_entries);
		sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8 *)&aclist_num_entries);
    \endcode
	<br>

	- SL_WLAN_CFG_AP_ACCESS_LIST_ID:
    \code
		_u8 aclist_mac[SL_WLAN_MAX_ACCESS_LIST_STATIONS][MAC_LEN];
		unsigned char aclist_num_entries;
		unsigned short config_opt;
		unsigned short len;
		int actual_aclist_num_entries;
		unsigned short start_aclist_index;
		unsigned short aclist_info_len;
		int i;

		start_aclist_index = 0;
		aclist_info_len = 2*MAC_LEN;
		sl_WlanGet(SL_WLAN_CFG_AP_ACCESS_LIST_ID, &start_aclist_index, &aclist_info_len, (_u8 *)&aclist_mac[start_aclist_index]);

		actual_aclist_num_entries = aclist_info_len / MAC_LEN;
		printf("-Print AP Deny list, num stations = %d\n", actual_aclist_num_entries);
		for (i=0; i<actual_aclist_num_entries; i++)
		{
				_u8 *pMac = aclist_mac[i+start_aclist_index];
				printf("    MAC %d:  %02x:%02x:%02x:%02x:%02x:%02x\n", i, pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);
		}
    \endcode
	<br>

	- SL_WLAN_P2P_OPT_CHANNEL_N_REGS:
    \code
		_u16 listen_channel,listen_reg,oper_channel,oper_reg;
		_u16 len = 4;
		_u16  config_opt = SL_WLAN_P2P_OPT_CHANNEL_N_REGS;
		_u8 channel_n_regs[4];
		sl_WlanGet(SL_WLAN_CFG_P2P_PARAM_ID, &config_opt, &len, (_u8* )channel_n_regs);
		listen_channel = channel_n_regs[0];
		listen_reg = channel_n_regs[1];
		oper_channel = channel_n_regs[2];
		oper_reg = channel_n_regs[3];
    \endcode
	<br>

	- SL_WLAN_RX_FILTER_STATE:
    \code
		int ret = 0;
		SlWlanRxFilterIdMask_t  FilterIdMask;
		_u16 len = sizeof(SlWlanRxFilterIdMask_t);;
		_u16  config_opt = SL_WLAN_RX_FILTER_STATE;
		memset(FilterIdMask,0,sizeof(FilterIdMask));
		ret = sl_WlanGet(SL_WLAN_RX_FILTERS_ID, &config_opt , &len, (_u8* )FilterIdMask);
    \endcode
	<br>

	- SL_WLAN_RX_FILTER_SYS_STATE:
    \code
		int ret = 0;
		SlWlanRxFilterSysFiltersMask_t  FilterSysIdMask;
		_u16 len = sizeof(SlWlanRxFilterSysFiltersMask_t);;
		_u16  config_opt = SL_WLAN_RX_FILTER_SYS_STATE;
		memset(FilterSysIdMask,0,sizeof(FilterSysIdMask));
		ret = sl_WlanGet(SL_WLAN_RX_FILTERS_ID, &config_opt , &len, (_u8* )FilterSysIdMask);
    \endcode
	<br>

	- SL_WLAN_CONNECTION_INFO:
    \code
		_i16 RetVal = 0 ;
		_u16 Len = sizeof(SlWlanConnStatusParam_t) ;
		SlWlanConnStatusParam_t WlanConnectInfo ;
		RetVal = sl_WlanGet(SL_WLAN_CONNECTION_INFO, NULL , &Len, (_u8*)&WlanConnectInfo);
   \endcode
	<br>

*/

#if _SL_INCLUDE_FUNC(sl_WlanGet)
_i16 sl_WlanGet(const _u16 ConfigId, _u16 *pConfigOpt,_u16 *pConfigLen, _u8 *pValues);
#endif

/*!
  \brief Adds new filter rule to the system

  \param[in]    RuleType     The rule type
								- SL_WLAN_RX_FILTER_HEADER
								- SL_WLAN_RX_FILTER_COMBINATION
  
  \param[in]    Flags        Flags which set the type of header rule Args and sets the persistent flag
								- SL_WLAN_RX_FILTER_BINARY
								- SL_WLAN_RX_FILTER_PERSISTENT
								- SL_WLAN_RX_FILTER_ENABLE
  
  \param[in]    pRule        Determine the filter rule logic
  \param[in]    pTrigger     Determine when the rule is triggered also sets rule parent.
  \param[in]    pAction      Sets the action to be executed in case the match functions pass
  \param[out]   pFilterId    The filterId which was created

  \par Persistent   
			Save the filters for persistent can be done by calling  with SL_WLAN_RX_FILTER_STORE

  \return       Zero on success, or negative error code on failure
  \sa
  \note
  \warning
 */
#if _SL_INCLUDE_FUNC(sl_WlanRxFilterAdd)
_i16 sl_WlanRxFilterAdd(    SlWlanRxFilterRuleType_t                RuleType,
                            SlWlanRxFilterFlags_u                   Flags,
                            const SlWlanRxFilterRule_u* const       pRule,
                            const SlWlanRxFilterTrigger_t* const    pTrigger,
                            const SlWlanRxFilterAction_t* const     pAction,
                            SlWlanRxFilterID_t*                     pFilterId);

#endif




/*!

 Close the Doxygen group.
 @}

 */


#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif    /*  __WLAN_H__ */

