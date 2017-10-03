/*
 * netapp.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef __NETAPP_H__
#define __NETAPP_H__




#ifdef    __cplusplus
extern "C" {
#endif

/*!
	\defgroup NetApp 
	\short Activates networking applications, such as: HTTP Server, DHCP Server, Ping, DNS and mDNS

*/

/*!

    \addtogroup NetApp
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

/* NetApp user events */   
typedef enum
{
	SL_NETAPP_EVENT_IPV4_ACQUIRED = 1,
	SL_NETAPP_EVENT_IPV6_ACQUIRED,
	SL_NETAPP_EVENT_IP_COLLISION,
	SL_NETAPP_EVENT_DHCPV4_LEASED,
	SL_NETAPP_EVENT_DHCPV4_RELEASED,
	SL_NETAPP_EVENT_HTTP_TOKEN_GET,
	SL_NETAPP_EVENT_HTTP_TOKEN_POST,
	SL_NETAPP_EVENT_IPV4_LOST,
	SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT,
	SL_NETAPP_EVENT_IPV6_LOST,
	SL_NETAPP_EVENT_MAX
} SlNetAppEventId_e;

  
#define SL_NETAPP_MDNS_OPTIONS_IS_UNIQUE_BIT	                0x1
#define SL_NETAPP_MDNS_OPTIONS_ADD_SERVICE_BIT		            ((_u32)0x1 << 31)
#define SL_NETAPP_MDNS_OPTIONS_IS_NOT_PERSISTENT                ((_u32)0x1 << 30)
#define SL_NETAPP_MDNS_OPTION_UPDATE_TEXT                       ((_u32)0x1 << 29)
#define SL_NETAPP_MDNS_IPV4_ONLY_SERVICE                        (_u32)(0) /* default mode:zero bits 27,28*/
#define SL_NETAPP_MDNS_IPV6_ONLY_SERVICE                        ((_u32)0x1 << 28)
#define SL_NETAPP_MDNS_IPV6_IPV4_SERVICE                        ((_u32)0x1 << 27)


/*ERROR code*/
#define SL_NETAPP_RX_BUFFER_LENGTH_ERROR (-230)

/*  Http Server interface */
#define SL_NETAPP_MAX_INPUT_STRING                    (64) /*  because of WPA */

#define SL_NETAPP_MAX_AUTH_NAME_LEN                             (20)
#define SL_NETAPP_MAX_AUTH_PASSWORD_LEN                         (20)
#define SL_NETAPP_MAX_AUTH_REALM_LEN                            (20)

#define SL_NETAPP_MAX_DEVICE_URN_LEN     (32+1)
#define SL_NETAPP_MAX_DOMAIN_NAME_LEN    (24+1)

#define SL_NETAPP_MAX_ACTION_LEN                                (30)
#define SL_NETAPP_MAX_TOKEN_NAME_LEN                            (20)  


#define SL_NETAPP_MAX_TOKEN_VALUE_LEN         SL_NETAPP_MAX_INPUT_STRING

#define SL_NETAPP_MAX_SERVICE_TEXT_SIZE                  (256)
#define SL_NETAPP_MAX_SERVICE_NAME_SIZE                  (60)
#define SL_NETAPP_MAX_SERVICE_HOST_NAME_SIZE             (64)


/* Server Responses */
#define SL_NETAPP_HTTPRESPONSE_NONE                   (0)
#define SL_NETAPP_HTTPSETTOKENVALUE                   (1)

#define SL_NETAPP_FAMILY_MASK                         (0x80)

/* mDNS types */
#define SL_NETAPP_MASK_IPP_TYPE_OF_SERVICE           (0x00000001)
#define SL_NETAPP_MASK_DEVICE_INFO_TYPE_OF_SERVICE   (0x00000002)
#define SL_NETAPP_MASK_HTTP_TYPE_OF_SERVICE          (0x00000004)
#define SL_NETAPP_MASK_HTTPS_TYPE_OF_SERVICE         (0x00000008)
#define SL_NETAPP_MASK_WORKSATION_TYPE_OF_SERVICE    (0x00000010)
#define SL_NETAPP_MASK_GUID_TYPE_OF_SERVICE          (0x00000020)
#define SL_NETAPP_MASK_H323_TYPE_OF_SERVICE          (0x00000040)
#define SL_NETAPP_MASK_NTP_TYPE_OF_SERVICE           (0x00000080)
#define SL_NETAPP_MASK_OBJECITVE_TYPE_OF_SERVICE     (0x00000100)
#define SL_NETAPP_MASK_RDP_TYPE_OF_SERVICE           (0x00000200)
#define SL_NETAPP_MASK_REMOTE_TYPE_OF_SERVICE        (0x00000400)
#define SL_NETAPP_MASK_RTSP_TYPE_OF_SERVICE          (0x00000800)
#define SL_NETAPP_MASK_SIP_TYPE_OF_SERVICE           (0x00001000)
#define SL_NETAPP_MASK_SMB_TYPE_OF_SERVICE           (0x00002000)
#define SL_NETAPP_MASK_SOAP_TYPE_OF_SERVICE          (0x00004000)
#define SL_NETAPP_MASK_SSH_TYPE_OF_SERVICE           (0x00008000)
#define SL_NETAPP_MASK_TELNET_TYPE_OF_SERVICE        (0x00010000)
#define SL_NETAPP_MASK_TFTP_TYPE_OF_SERVICE          (0x00020000)
#define SL_NETAPP_MASK_XMPP_CLIENT_TYPE_OF_SERVICE   (0x00040000)
#define SL_NETAPP_MASK_RAOP_TYPE_OF_SERVICE          (0x00080000)
#define SL_NETAPP_MASK_ALL_TYPE_OF_SERVICE           (0xFFFFFFFF)

/********************************************************************************************************/

/* NetApp application IDs */
#define SL_NETAPP_HTTP_SERVER_ID                     (0x01)
#define SL_NETAPP_DHCP_SERVER_ID                     (0x02)
#define SL_NETAPP_MDNS_ID                            (0x04)
#define SL_NETAPP_DNS_SERVER_ID                      (0x08)

#define SL_NETAPP_DEVICE_ID                          (0x10)
#define SL_NETAPP_DNS_CLIENT_ID                      (0x20)
#define SL_NETAPP_STATUS		                     (0x40)

/* NetApp application set/get options */             
#define SL_NETAPP_DHCP_SRV_BASIC_OPT                 (0)             

/* HTTP server set/get options */                    
#define SL_NETAPP_HTTP_PRIMARY_PORT_NUMBER					(0)
#define SL_NETAPP_HTTP_AUTH_CHECK							(1)
#define SL_NETAPP_HTTP_AUTH_NAME							(2)
#define SL_NETAPP_HTTP_AUTH_PASSWORD						(3)
#define SL_NETAPP_HTTP_AUTH_REALM							(4)
#define SL_NETAPP_HTTP_ROM_PAGES_ACCESS						(5)
#define SL_NETAPP_HTTP_SECONDARY_PORT_NUMBER				(6)
#define SL_NETAPP_HTTP_SECONDARY_PORT_ENABLE				(7) /*Enable / disable of secondary port */
#define SL_NETAPP_HTTP_PRIMARY_PORT_SECURITY_MODE			(8)
#define SL_NETAPP_HTTP_PRIVATE_KEY_FILENAME					(9)
#define SL_NETAPP_HTTP_DEVICE_CERTIFICATE_FILENAME			(10)
#define SL_NETAPP_HTTP_CA_CERTIFICATE_FILE_NAME				(11)
#define SL_NETAPP_HTTP_TEMP_REGISTER_MDNS_SERVICE_NAME		(12)
#define SL_NETAPP_HTTP_TEMP_UNREGISTER_MDNS_SERVICE_NAME    (13)
                                                     
#define SL_NETAPP_MDNS_CONT_QUERY_OPT            (1)
#define SL_NETAPP_MDNS_QEVETN_MASK_OPT           (2)
#define SL_NETAPP_MDNS_TIMING_PARAMS_OPT         (3)

/* DNS server set/get options */
#define SL_NETAPP_DNS_OPT_DOMAIN_NAME            (0)

/* Device Config set/get options */
#define SL_NETAPP_DEVICE_URN        (0)
#define SL_NETAPP_DEVICE_DOMAIN      (1)

/* DNS client set/get options */
#define SL_NETAPP_DNS_CLIENT_TIME                (0)

/* Get active application bimap */
#define SL_NETAPP_STATUS_ACTIVE_APP              (0)

#ifdef SL_TINY
#define SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH         63
#else
#define SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH         255
#endif

/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/


typedef struct
{
    _u32 Ip;
    _u32 Gateway;
    _u32 Dns;
}SlIpV4AcquiredAsync_t;

typedef enum
{
  SL_BSD_IPV6_ACQUIRED_TYPE_LOCAL = 1,
  SL_BSD_IPV6_ACQUIRED_TYPE_GLOBAL = 2
}SlIpV6AcquiredAsyncType_e;


typedef struct  
{
    _u32 Ip[4];
    _u32 Dns[4];
}SlIpV6AcquiredAsync_t;

typedef struct
{
   _u32    IpAddress;
   _u32    LeaseTime;
   _u8     Mac[6];
   _u16    Padding;
}SlIpLeasedAsync_t;

typedef struct
{
  _u32    IpAddress;
  _u8     Mac[6];
  _u16    Reason;
}SlIpReleasedAsync_t;

typedef struct
{
   _u32    IpAddress;
   _u8     DhcpMac[6];
   _u8     ConflictMac[6];
}SlIpCollisionAsync_t;

typedef struct
{
	_i16  Status;
	_u16  Padding;
}SlIpV4Lost_t;

typedef struct
{
	_u32 IpLost[4];
}SlIpV6Lost_t;

typedef struct
{
	_i16  Status;
	_u16  Padding;
}SlDhcpIpAcquireTimeout_t;


typedef union
{
  SlIpV4AcquiredAsync_t		IpAcquiredV4;		   /* SL_NETAPP_EVENT_IPV4_ACQUIRED */
  SlIpV6AcquiredAsync_t		IpAcquiredV6;		   /* SL_NETAPP_EVENT_IPV6_ACQUIRED */
  _u32						Sd;					   /* SL_SOCKET_TX_FAILED_EVENT*/ 
  SlIpLeasedAsync_t			IpLeased;			   /* SL_NETAPP_EVENT_DHCPV4_LEASED   */
  SlIpReleasedAsync_t		IpReleased;			   /* SL_NETAPP_EVENT_DHCPV4_RELEASED */
  SlIpV4Lost_t				IpV4Lost;			   /* SL_NETAPP_EVENT_IPV4_LOST */
  SlDhcpIpAcquireTimeout_t	DhcpIpAcquireTimeout;  /* SL_NETAPP_DHCP_ACQUIRE_IPV4_TIMEOUT_EVENT */
  SlIpCollisionAsync_t      IpCollision;		   /* SL_NETAPP_EVENT_IP_COLLISION   */
  SlIpV6Lost_t				IpV6Lost;			   /* SL_NETAPP_EVENT_IPV6_LOST   */
} SlNetAppEventData_u;

typedef struct
{
   _u32                     Id;
   SlNetAppEventData_u      Data;
}SlNetAppEvent_t;


typedef struct
{
    _u32    PacketsSent;
    _u32    PacketsReceived;
    _u16    MinRoundTime;
    _u16    MaxRoundTime;
    _u16    AvgRoundTime;
    _u32    TestTime;
}SlNetAppPingReport_t;

typedef struct
{
    _u32    PingIntervalTime;       /* delay between pings, in milliseconds */
    _u16    PingSize;               /* ping packet size in bytes           */
    _u16    PingRequestTimeout;     /* timeout time for every ping in milliseconds  */
    _u32    TotalNumberOfAttempts;  /* max number of ping requests. 0 - forever    */
    _u32    Flags;                  /* flag - 0 report only when finished, 1 - return response for every ping, 2 - stop after 1 successful ping.  4 - ipv4 header flag - don`t fragment packet */
    _u32    Ip;                     /* IPv4 address or IPv6 first 4 bytes  */
    _u32    Ip1OrPadding;
    _u32    Ip2OrPadding;
    _u32    Ip3OrPadding;
}SlNetAppPingCommand_t;

typedef struct
{
    _u8     Len;
    _u8     *pData;
} SlNetAppHttpServerString_t;

typedef struct
{
    _u8     ValueLen;
    _u8     NameLen;
    _u8     *pTokenValue;
    _u8     *pTokenName;
} SlNetAppHttpServerData_t;

typedef struct
{
    SlNetAppHttpServerString_t Action;
    SlNetAppHttpServerString_t TokenName;
    SlNetAppHttpServerString_t TokenValue;
}SlNetAppHttpServerPostData_t;

typedef union
{
  SlNetAppHttpServerString_t        HttpTokenName; /* SL_NETAPP_HTTPGETTOKENVALUE */
  SlNetAppHttpServerPostData_t      HttpPostData;  /* SL_NETAPP_HTTPPOSTTOKENVALUE */
} SlNetAppHttpServerEventData_u;

typedef union
{
  SlNetAppHttpServerString_t TokenValue;
} SlNetAppHttpServerResponsedata_u;

typedef struct
{
   _u32                          Event;
   SlNetAppHttpServerEventData_u EventData;
}SlNetAppHttpServerEvent_t;

typedef struct
{
   _u32                             Response;
   SlNetAppHttpServerResponsedata_u ResponseData;
}SlNetAppHttpServerResponse_t;

/*****************************************************************************************
*   NETAPP Request/Response/Send/Receive
******************************************************************************************/
/*  TODO: check what definitions are eventually needed */
/* NETAPP http request types */
#define SL_NETAPP_REQUEST_HTTP_GET	1
#define SL_NETAPP_REQUEST_HTTP_POST	2
#define SL_NETAPP_REQUEST_HTTP_PUT	3
#define SL_NETAPP_REQUEST_HTTP_DELETE	4

#define SL_NETAPP_REQUEST_MAX_METADATA_LEN 1024
#define SL_NETAPP_REQUEST_MAX_DATA_LEN     1364   /* Metadata + Payload */


typedef enum
{
	SL_NETAPP_REQUEST_METADATA_TYPE_STATUS = 0,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_VERSION,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_REQUEST_URI,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_QUERY_STRING,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_LEN,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_TYPE,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_LOCATION,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_SERVER,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_USER_AGENT,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_COOKIE,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_SET_COOKIE,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_UPGRADE,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_REFERER,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_ACCEPT,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_ENCODING,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_DISPOSITION,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONNECTION,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_ETAG,
    SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_DATE,
	SL_NETAPP_REQUEST_METADATA_TYPE_HEADER_HOST,
	SL_NETAPP_REQUEST_METADATA_TYPE_ACCEPT_ENCODING,
	SL_NETAPP_REQUEST_METADATA_TYPE_ACCEPT_LANGUAGE,
	SL_NETAPP_REQUEST_METADATA_TYPE_CONTENT_LANGUAGE,
	SL_NETAPP_REQUEST_METADATA_TYPE_ORIGIN,          
    SL_NETAPP_REQUEST_METADATA_TYPE_ORIGIN_CONTROL_ACCESS,
	SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_NONE
	
} SlNetAppMetadataHTTPTypes_e;

typedef enum
{
	SL_NETAPP_RESPONSE_NONE = 0,				/* No response */
	SL_NETAPP_RESPONSE_PENDING = 1,				/* status will arrive in future NetApp Send call (in metadata) */

    SL_NETAPP_HTTP_RESPONSE_101_SWITCHING_PROTOCOLS = 101,      /* 101 Switching Protocol*/
	SL_NETAPP_HTTP_RESPONSE_200_OK = 200, 			/* 200 OK */
	SL_NETAPP_HTTP_RESPONSE_201_CREATED = 201,		/* "HTTP/1.0 201 Created" */
	SL_NETAPP_HTTP_RESPONSE_202_ACCEPTED = 202, 		/* "HTTP/1.0 202 Accepted" */
	SL_NETAPP_HTTP_RESPONSE_204_OK_NO_CONTENT = 204,	/* 204 No Content */
	SL_NETAPP_HTTP_RESPONSE_301_MOVED_PERMANENTLY = 301,	/* "HTTP/1.0 301 Moved Permanently" */
	SL_NETAPP_HTTP_RESPONSE_302_MOVED_TEMPORARILY = 302, 	/* 302 Moved Temporarily (http 1.0) */
	SL_NETAPP_HTTP_RESPONSE_303_SEE_OTHER = 303,		/* "HTTP/1.1 303 See Other" */
	SL_NETAPP_HTTP_RESPONSE_304_NOT_MODIFIED = 304, 	/* "HTTP/1.0 304 Not Modified" */
	SL_NETAPP_HTTP_RESPONSE_400_BAD_REQUEST = 400,		/* "HTTP/1.0 400 Bad Request" */
	SL_NETAPP_HTTP_RESPONSE_403_FORBIDDEN = 403,		/* "HTTP/1.0 403 Forbidden" */
	SL_NETAPP_HTTP_RESPONSE_404_NOT_FOUND = 404,		/* 404 Not Found */
	SL_NETAPP_HTTP_RESPONSE_405_METHOD_NOT_ALLOWED = 405,	/* "HTTP/1.0 405 Method Not Allowed" */
	SL_NETAPP_HTTP_RESPONSE_500_INTERNAL_SERVER_ERROR = 500,/* 500 Internal Server Error */
	SL_NETAPP_HTTP_RESPONSE_503_SERVICE_UNAVAILABLE = 503,	/* "HTTP/1.0 503 Service Unavailable" */
	SL_NETAPP_HTTP_RESPONSE_504_GATEWAY_TIMEOUT = 504	/* "HTTP/1.0 504 Gateway Timeout" */
} SlNetAppResponseCode_e;


#define SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION	0x00000001
#define SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA		0x00000002  /* 0 - data is payload, 1 - data is metadata */
#define SL_NETAPP_REQUEST_RESPONSE_FLAGS_ACCUMULATION	0x00000004
#define SL_NETAPP_REQUEST_RESPONSE_FLAGS_ERROR          0x80000000  /* in that case the last two bytes represents the error code */

typedef struct
{
    _u16 MetadataLen;
    _u8 *pMetadata;
    _u16 PayloadLen;
    _u8 *pPayload;
    _u32 Flags;
} SlNetAppData_t;

typedef struct
{
    _u8     AppId;
    _u8     Type;
    _u16    Handle;
    SlNetAppData_t  requestData;

} SlNetAppRequest_t;

typedef struct
{
    _u16			Status;
    SlNetAppData_t  ResponseData;

} SlNetAppResponse_t;


typedef struct
{
    _u32   lease_time;
    _u32   ipv4_addr_start;
    _u32   ipv4_addr_last;
}SlNetAppDhcpServerBasicOpt_t; 

/* mDNS parameters */
typedef enum
{
    SL_NETAPP_FULL_SERVICE_WITH_TEXT_IPV4_TYPE = 1,
    SL_NETAPP_FULL_SERVICE_IPV4_TYPE,
    SL_NETAPP_SHORT_SERVICE_IPV4_TYPE,
    SL_NETAPP_FULL_SERVICE_WITH_TEXT_IPV6_TYPE ,
    SL_NETAPP_FULL_SERVICE_IPV6_TYPE,
    SL_NETAPP_SHORT_SERVICE_IPV6_TYPE
} SlNetAppGetServiceListType_e;

typedef struct
{
    _u32   service_ipv4;
    _u16   service_port;
    _u16   Reserved;
}SlNetAppGetShortServiceIpv4List_t;

typedef struct
{
    _u32   service_ipv4;
    _u16   service_port;
    _u16   Reserved;
    _u8    service_name[SL_NETAPP_MAX_SERVICE_NAME_SIZE];
    _u8    service_host[SL_NETAPP_MAX_SERVICE_HOST_NAME_SIZE];
}SlNetAppGetFullServiceIpv4List_t;

typedef struct
{
    _u32    service_ipv4;
    _u16    service_port;
    _u16    Reserved;
    _u8     service_name[SL_NETAPP_MAX_SERVICE_NAME_SIZE];
    _u8     service_host[SL_NETAPP_MAX_SERVICE_HOST_NAME_SIZE];
    _u8     service_text[SL_NETAPP_MAX_SERVICE_TEXT_SIZE];
}SlNetAppGetFullServiceWithTextIpv4List_t;

/* IPv6 entries */
typedef struct
{
    _u32   service_ipv6[4];
    _u16   service_port;
    _u16   Reserved;
}SlNetAppGetShortServiceIpv6List_t;

typedef struct
{
    _u32   service_ipv6[4];
    _u16   service_port;
    _u16   Reserved;
    _u8    service_name[SL_NETAPP_MAX_SERVICE_NAME_SIZE];
    _u8    service_host[SL_NETAPP_MAX_SERVICE_HOST_NAME_SIZE];
}SlNetAppGetFullServiceIpv6List_t;

typedef struct
{
    _u32    service_ipv6[4];
    _u16    service_port;
    _u16    Reserved;
    _u8     service_name[SL_NETAPP_MAX_SERVICE_NAME_SIZE];
    _u8     service_host[SL_NETAPP_MAX_SERVICE_HOST_NAME_SIZE];
    _u8     service_text[SL_NETAPP_MAX_SERVICE_TEXT_SIZE];
}SlNetAppGetFullServiceWithTextIpv6List_t;


typedef struct
{
    /*The below parameters are used to configure the advertise times and interval
    For example:
        If:
        Period is set to T
        Repetitions are set to P
        Telescopic factor is K=2
        The transmission shall be:
        advertise P times
        wait T
        advertise P times
        wait 4 * T
        advertise P time
        wait 16 * T  ... (till max time reached / configuration changed / query issued)
    */
    _u32    t;              /* Number of ticks for the initial period. Default is 100 ticks for 1 second. */
    _u32    p;              /* Number of repetitions. Default value is 1                                  */
    _u32    k;              /* Telescopic factor. Default value is 2.                                     */
    _u32    RetransInterval;/* Announcing retransmission interval                                         */
    _u32    Maxinterval;     /* Announcing max period interval                                            */
    _u32    max_time;       /* Announcing max time                                                        */
}SlNetAppServiceAdvertiseTimingParameters_t;


typedef struct
{
   _u16   MaxResponseTime;
   _u16   NumOfRetries;
}SlNetAppDnsClientTime_t; 

/*****************************************************************************/
/* Types declarations                                               */
/*****************************************************************************/
typedef void (*P_SL_DEV_PING_CALLBACK)(SlNetAppPingReport_t*);

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/


/*!
    \brief Starts a network application

    Gets and starts network application for the current WLAN mode

    \param[in] AppBitMap      Application bitmap, could be one or combination of the following:
                              - SL_NETAPP_HTTP_SERVER_ID   
                              - SL_NETAPP_DHCP_SERVER_ID   
                              - SL_NETAPP_MDNS_ID       
							  - SL_NETAPP_DNS_SERVER_ID

	\par	Persistent 	- <b>System Persistent</b>
    \return                   Zero on success, or negative error code on failure

    \sa                       sl_NetAppStop
    \note                     This command activates the application for the current WLAN mode (AP or STA)
    \warning
    \par                 Example  
    
	- Starting internal HTTP server + DHCP server:
	\code
    sl_NetAppStart(SL_NETAPP_HTTP_SERVER_ID | SL_NETAPP_DHCP_SERVER_ID)
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_NetAppStart)
_i16 sl_NetAppStart(const _u32 AppBitMap);
#endif
/*!
    \brief Stops a network application

    Gets and stops network application for the current WLAN mode

    \param[in] AppBitMap    Application id, could be one of the following: \n
                            - SL_NETAPP_HTTP_SERVER_ID 
                            - SL_NETAPP_DHCP_SERVER_ID 
                            - SL_NETAPP_MDNS_ID 
							- SL_NETAPP_DNS_SERVER_ID 

	\par	Persistent 	- <b>System Persistent</b>

    \return                 Zero on success, or nagative error code on failure

    \sa					sl_NetAppStart
    \note               This command disables the application for the current active WLAN mode (AP or STA)
    \warning
    \par                Example
    
	
	- Stopping internal HTTP server:
	\code                
    sl_NetAppStop(SL_NETAPP_HTTP_SERVER_ID); 
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_NetAppStop)
_i16 sl_NetAppStop(const _u32 AppBitMap);
#endif

/*!
    \brief Get host IP by name\n
    Obtain the IP Address of machine on network, by machine name.

    \param[in]  pHostName       Host name
    \param[in]  NameLen         Name length
    \param[out] OutIpAddr       This parameter is filled in with
                                host IP address. In case that host name is not
                                resolved, out_ip_addr is zero.
    \param[in]  Family          Protocol family

    \return                     Zero on success, or negative on failure.\n
                                SL_POOL_IS_EMPTY may be return in case there are no resources in the system\n
									In this case try again later or increase MAX_CONCURRENT_ACTIONS
                                Possible DNS error codes:
                                - SL_NETAPP_DNS_QUERY_NO_RESPONSE       
                                - SL_NETAPP_DNS_NO_SERVER               
                                - SL_NETAPP_DNS_QUERY_FAILED            
                                - SL_NETAPP_DNS_MALFORMED_PACKET        
                                - SL_NETAPP_DNS_MISMATCHED_RESPONSE     

    \sa
    \note   Only one sl_NetAppDnsGetHostByName can be handled at a time.\n
            Calling this API while the same command is called from another thread, may result
            in one of the two scenarios:
            1. The command will wait (internal) until the previous command finish, and then be executed.
            2. There are not enough resources and POOL_IS_EMPTY error will return.\n
            In this case, MAX_CONCURRENT_ACTIONS can be increased (result in memory increase) or try
            again later to issue the command.
    \warning
           In case an IP address in a string format is set as input, without any prefix (e.g. "1.2.3.4") the device will not 
           try to access the DNS and it will return the input address on the 'out_ip_addr' field 
    \par  Example
    
	- Getting host by name:
	\code
    _u32 DestinationIP;
    _u32 AddrSize;
	_i16 SockId;
	SlSockAddrIn_t Addr;

    sl_NetAppDnsGetHostByName("www.google.com", strlen("www.google.com"), &DestinationIP,SL_AF_INET);

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(80);
    Addr.sin_addr.s_addr = sl_Htonl(DestinationIP);
    AddrSize = sizeof(SlSockAddrIn_t);
    SockId = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_NetAppDnsGetHostByName)
_i16 sl_NetAppDnsGetHostByName(_i8 * pHostName,const  _u16 NameLen, _u32*  OutIpAddr,const _u8 Family );
#endif

/*!
        \brief Return service attributes like IP address, port and text according to service name\n
			The user sets a service name Full/Part (see example below), and should get:
			- IP of service
			- The port of service
			- The text of service
        Hence it can make a connection to the specific service and use it.
        It is similar to sl_NetAppDnsGetHostByName method.\n
        It is done by a single shot ipv4 & ipv6 (if enabled) query with PTR type on the service name.
                  The command that is sent is from constant parameters and variables parameters.

        \param[in]     pServiceName                   Service name can be full or partial. \n
                                                  Example for full service name:
                                                  1. PC1._ipp._tcp.local
                                                  2. PC2_server._ftp._tcp.local \n
                                                  .
                                                  Example for partial service name:
                                                  1. _ipp._tcp.local
                                                  2. _ftp._tcp.local

        \param[in]    ServiceLen                  The length of the service name (in_pService).
        \param[in]    Family                      IPv4 or IPv6 (SL_AF_INET , SL_AF_INET6).
        \param[out]    pAddr                      Contains the IP address of the service.
        \param[out]    pPort                      Contains the port of the service.
        \param[out]    pTextLen                   Has 2 options. One as Input field and the other one as output:
                                                  - Input: \n
                                                  Contains the max length of the text that the user wants to get.\n
                                                  It means that if the text len of service is bigger that its value than
                                                  the text is cut to inout_TextLen value.
                                                  - Output: \n
                                                   Contain the length of the text that is returned. Can be full text or part of the text (see above).

        \param[out]   pText     Contains the text of the service full or partial

        \return       Zero on success,\n
                      SL_POOL_IS_EMPTY may be return in case there are no resources in the system, 
                      In this case try again later or increase MAX_CONCURRENT_ACTIONS\n
                      In case No service is found error SL_NETAPP_DNS_NO_ANSWER will be returned
		\sa			  sl_NetAppDnsGetHostByName
        \note         The returns attributes belongs to the first service found.
                      There may be other services with the same service name that will response to the query.
                      The results of these responses are saved in the peer cache of the Device and should be read by another API.\n
                          
                      Only one sl_NetAppDnsGetHostByService can be handled at a time.\n
                      Calling this API while the same command is called from another thread, may result
                      in one of the two scenarios:
                      1. The command will wait (internal) until the previous command finish, and then be executed.
                      2. There are not enough resources and SL_POOL_IS_EMPTY error will return. 
                      In this case, MAX_CONCURRENT_ACTIONS can be increased (result in memory increase) or try
                      again later to issue the command.

        \warning      Text length can be 120 bytes only
*/
#if _SL_INCLUDE_FUNC(sl_NetAppDnsGetHostByService)
_i16 sl_NetAppDnsGetHostByService(_i8  *pServiceName, /*  string containing all (or only part): name + subtype + service */
                                  const _u8  ServiceLen,
                                  const _u8  Family,        /*  4-IPv4 , 16-IPv6  */
                                  _u32 pAddr[], 
                                  _u32 *pPort,
                                  _u16 *pTextLen,     /*  in: max len , out: actual len */
                                  _i8  *pText
                                 );

#endif

/*!
        \brief Get service list\n
        Insert into out pBuffer a list of peer's services that are in the NWP without issuing any queries (relying on pervious collected data).\n
        The list is in a form of service struct. The user should chose the type
        of the service struct like:
            - Full service parameters with text.
            - Full service parameters.
            - Short service parameters (port and IP only) especially for tiny hosts.

        The different types of struct are made to give the 
        possibility to save memory in the host.\n

        The user can also chose how many max services to get and start point index
        NWP peer cache.\n
        For example:
            1.    Get max of 3 full services from index 0. 
				- Up to 3 full services from index 0 are inserted into pBuffer (services that are in indexes 0,1,2).
            2.    Get max of 4 full services from index 3.
				- Up to 4 full services from index 3 are inserted into pBuffer (services that are in indexes 3,4,5,6).
            3.    Get max of 2 int services from index 6.
				- Up to 2 int services from index 6 are inserted into pBuffer (services that are in indexes 6,7).
        See below - command parameters.
                    
        \param[in] IndexOffset - The start index in the peer cache that from it the first service is returned.
        \param[in] MaxServiceCount - The Max services that can be returned if existed or if not exceed the max index 
                      in the peer cache
        \param[in] Flags - an ENUM number that means which service struct to use (means which types of service to fill)                                            
						- use SL_NETAPP_FULL_SERVICE_WITH_TEXT_IPV4_TYPE  for SlNetAppGetFullServiceWithTextIpv4List_t
						- use SL_NETAPP_FULL_SERVICE_IPV4_TYPE for SlNetAppGetFullServiceIpv4List_t
						- use SL_NETAPP_SHORT_SERVICE_IPV4_TYP SlNetAppGetShortServiceIpv4List_t
						- use SL_NETAPP_FULL_SERVICE_IPV6_TYPE, SlNetAppGetFullServiceIpv6List_t
						- use SL_NETAPP_SHORT_SERVICE_IPV6_TYPE SlNetAppGetShortServiceIpv6List_t
						- use SL_NETAPP_FULL_SERVICE_WITH_TEXT_IPV6_TYPE  SlNetAppGetFullServiceWithTextIpv6List_t

       \param[out]  pBuffer - The Services are inserted into this buffer. In the struct form according to the bit that is set in the Flags 
                      input parameter.

        \param[in] BufferLength - The allocated buffer length (pointed by pBuffer).
 
        \return    ServiceFoundCount - The number of the services that were inserted into the buffer.\n 
					Zero means no service is found negative number means an error
        \sa           sl_NetAppMDNSRegisterService
        \note        
        \warning 
                    If the out pBuffer size is bigger than an RX packet(1480), than
                    an error is returned because there is no place in the RX packet.\n
                    The size is a multiply of MaxServiceCount and size of service struct(that is set 
                    according to flag value).
*/

#if _SL_INCLUDE_FUNC(sl_NetAppGetServiceList)
_i16 sl_NetAppGetServiceList(const _u8   IndexOffset,
                             const _u8   MaxServiceCount,
                             const  _u8  Flags,
                                   _i8   *pBuffer,
                             const _u32  BufferLength
                            );

#endif

/*!
        \brief Unregister mDNS service\n
        This function deletes the mDNS service from the mDNS package and the database.

        The mDNS service that is to be unregistered is a service that the application no longer wishes to provide. \n
        The service name should be the full service name according to RFC
        of the DNS-SD - meaning the value in name field in the SRV answer.
                    
        Examples for service names:
        1. PC1._ipp._tcp.local
        2. PC2_server._ftp._tcp.local

        \param[in]    pServiceName              Full service name. \n
        \param[in]    ServiceNameLen            The length of the service. 
        \param[in]    Options            bitwise parameters: \n
										   - SL_NETAPP_MDNS_OPTIONS_IS_UNIQUE_BIT     bit 0  - service is unique per interface (means that the service needs to be unique)
										   - SL_NETAPP_MDNS_IPV6_IPV4_SERVICE         bit 27  - add this service to IPv6 interface, if exist (default is IPv4 service only)
										   - SL_NETAPP_MDNS_IPV6_ONLY_SERVICE         bit 28  - add this service to IPv6 interface, but remove it from IPv4 (only IPv6 is available)
										   - SL_NETAPP_MDNS_OPTION_UPDATE_TEXT        bit 29  - for update text fields (without reregister the service)
										   - SL_NETAPP_MDNS_OPTIONS_IS_NOT_PERSISTENT bit 30  - for setting a non persistent service
										   - SL_NETAPP_MDNS_OPTIONS_ADD_SERVICE_BIT   bit 31  - for internal use if the service should be added or deleted (set means ADD).


        \return    Zero on success, or negative error code on failure 
        \sa          sl_NetAppMDNSRegisterService
        \note        
        \warning 
        The size of the service length should be smaller than 255.
*/
#if _SL_INCLUDE_FUNC(sl_NetAppMDNSUnRegisterService)
_i16 sl_NetAppMDNSUnRegisterService(const _i8 *pServiceName,const _u8 ServiceNameLen,_u32 Options);
#endif

/*!
        \brief Register a new mDNS service\n
        This function registers a new mDNS service to the mDNS package and the DB. \n
        This registered service is a service offered by the application.
        The service name should be full service name according to RFC
        of the DNS-SD - meaning the value in name field in the SRV answer.\n
        Example for service name:
        1. PC1._ipp._tcp.local
        2. PC2_server._ftp._tcp.local

        If the option is_unique is set, mDNS probes the service name to make sure
        it is unique before starting to announce the service on the network.
        Instance is the instance portion of the service name.

        \param[in]  ServiceNameLen     The length of the service.
        \param[in]  TextLen            The length of the service should be smaller than 64.
        \param[in]  Port               The port on this target host port.
        \param[in]  TTL                The TTL of the service
        \param[in]  Options            bitwise parameters: \n
                                       - SL_NETAPP_MDNS_OPTIONS_IS_UNIQUE_BIT     bit 0  - service is unique per interface (means that the service needs to be unique)
                                       - SL_NETAPP_MDNS_IPV6_IPV4_SERVICE         bit 27  - add this service to IPv6 interface, if exist (default is IPv4 service only)
                                       - SL_NETAPP_MDNS_IPV6_ONLY_SERVICE         bit 28  - add this service to IPv6 interface, but remove it from IPv4 (only IPv6 is available)
									   - SL_NETAPP_MDNS_OPTION_UPDATE_TEXT        bit 29  - for update text fields (without reregister the service)
									   - SL_NETAPP_MDNS_OPTIONS_IS_NOT_PERSISTENT bit 30  - for setting a non persistent service
                                       - SL_NETAPP_MDNS_OPTIONS_ADD_SERVICE_BIT   bit 31  - for internal use if the service should be added or deleted (set means ADD).

        \param[in]    pServiceName		The service name.
        \param[in] pText                     The description of the service.
                                                should be as mentioned in the RFC
                                                (according to type of the service IPP,FTP...)

        \return     Zero on success, or negative error code on failure

        \sa              sl_NetAppMDNSUnRegisterService

        \warning      1) Temporary -  there is an allocation on stack of internal buffer.
                    Its size is SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH. \n
                    It means that the sum of the text length and service name length cannot be bigger than
                    SL_NETAPP_MDNS_MAX_SERVICE_NAME_AND_TEXT_LENGTH.\n
                    If it is - An error is returned. \n
                    2) According to now from certain constraints the variables parameters are set in the
                    attribute part (contain constant parameters)

	    \par   Examples:

	- Register a new service:
    \code
		const signed char AddService[40]			= "PC1._ipp._tcp.local";
		_u32  Options;

		Options = SL_NETAPP_MDNS_OPTIONS_IS_UNIQUE_BIT | SL_NETAPP_MDNS_OPTIONS_IS_NOT_PERSISTENT;
		sl_NetAppMDNSRegisterService(AddService,sizeof(AddService),"Service 1;payper=A3;size=5",strlen("Service 1;payper=A3;size=5"),1000,120,Options);
	\endcode
	<br>

	- Update text for existing service:
	\code
		Please Note! Update is for text only! Importent to apply the same persistent flag options as original service registration.\n

		Options = SL_NETAPP_MDNS_OPTION_UPDATE_TEXT | SL_NETAPP_MDNS_OPTIONS_IS_NOT_PERSISTENT;
		sl_NetAppMDNSRegisterService(AddService,sizeof(AddService),"Service 5;payper=A4;size=10",strlen("Service 5;payper=A4;size=10"),1000,120,Options);

    \endcode 
*/
#if _SL_INCLUDE_FUNC(sl_NetAppMDNSRegisterService)
_i16 sl_NetAppMDNSRegisterService( const _i8*  pServiceName, 
                                   const _u8   ServiceNameLen,
                                   const _i8*  pText,
                                   const _u8   TextLen,
                                   const _u16  Port,
                                   const _u32  TTL,
                                         _u32  Options);
#endif

/*!
    \brief send ICMP ECHO_REQUEST to network hosts

    Ping uses the ICMP protocol's mandatory ECHO_REQUEST

    \param[in]   pPingParams     Pointer to the ping request structure: 
                                 - If flags parameter is set to 0, ping will report back once all requested pings are done (as defined by TotalNumberOfAttempts). 
                                 - If flags parameter is set to 1, ping will report back after every ping, for TotalNumberOfAttempts.
                                 - If flags parameter is set to 2, ping will stop after the first successful ping, and report back for the successful ping, as well as any preceding failed ones. \n
								 - If flags parameter is set to 4, for ipv4 -  don`t fragment the ping packet. This flag can be set with other flags.
                                 For stopping an ongoing ping activity, set parameters IP address to 0
    \param[in]   Family          SL_AF_INET or  SL_AF_INET6
    \param[out]  pReport         Ping pReport
    \param[out]  pPingCallback	Callback function upon completion.\n
                                 If callback is NULL, the API is blocked until data arrives


    \return    Zero on success, or negative error code on failure.\n
               SL_POOL_IS_EMPTY may be return in case there are no resources in the system
               In this case try again later or increase MAX_CONCURRENT_ACTIONS

    \sa       
    \note      Only one sl_NetAppPing can be handled at a time.
              Calling this API while the same command is called from another thread, may result
                  in one of the two scenarios:
              1. The command will wait (internal) until the previous command finish, and then be executed.
              2. There are not enough resources and SL_POOL_IS_EMPTY error will return. 
              In this case, MAX_CONCURRENT_ACTIONS can be increased (result in memory increase) or try
              again later to issue the command.
    \warning  
    \par      Example:
    
	- Sending 20 ping requests and reporting results to a callback routine when 
      all requests are sent:
	\code     
		// callback routine
		void pingRes(SlNetAppPingReport_t* pReport)
		{
		// handle ping results 
		}
              
		// ping activation
		void PingTest()
		{
			SlNetAppPingReport_t report;
			SlNetAppPingCommand_t pingCommand;
                 
			pingCommand.Ip = SL_IPV4_VAL(10,1,1,200);     // destination IP address is 10.1.1.200
			pingCommand.PingSize = 150;                   // size of ping, in bytes 
			pingCommand.PingIntervalTime = 100;           // delay between pings, in milliseconds
			pingCommand.PingRequestTimeout = 1000;        // timeout for every ping in milliseconds
			pingCommand.TotalNumberOfAttempts = 20;       // max number of ping requests. 0 - forever 
			pingCommand.Flags = 0;                        // report only when finished
  
			sl_NetAppPing( &pingCommand, SL_AF_INET, &report, pingRes ) ;
		}
    \endcode
	<br>

	- Stopping Ping command:
	\code
		Status = sl_NetAppPing(0, 0, 0, 0 ) ;
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_NetAppPing)
_i16 sl_NetAppPing(const SlNetAppPingCommand_t* pPingParams,const _u8 Family, SlNetAppPingReport_t *pReport, const P_SL_DEV_PING_CALLBACK pPingCallback);
#endif

/*!
    \brief     Setting network application configurations

    \param[in] AppId          Application id, could be one of the following: 
                              - SL_NETAPP_HTTP_SERVER_ID
                              - SL_NETAPP_DHCP_SERVER_ID (AP Role only)
                              - SL_NETAPP_MDNS_ID
							  - SL_NETAPP_DNS_SERVER_ID
							  - SL_NETAPP_DEVICE_ID
							  - SL_NETAPP_DNS_CLIENT_ID

    \param[in] Option     Set option, could be one of the following:
								- For SL_NETAPP_HTTP_SERVER_ID
									- SL_NETAPP_HTTP_PRIMARY_PORT_NUMBER
									- SL_NETAPP_HTTP_AUTH_CHECK
									- SL_NETAPP_HTTP_AUTH_NAME
									- SL_NETAPP_HTTP_AUTH_PASSWORD
									- SL_NETAPP_HTTP_AUTH_REALM
									- SL_NETAPP_HTTP_ROM_PAGES_ACCESS
									- SL_NETAPP_HTTP_SECONDARY_PORT_NUMBER
									- SL_NETAPP_HTTP_SECONDARY_PORT_ENABLE
									- SL_NETAPP_HTTP_PRIMARY_PORT_SECURITY_MODE
									- SL_NETAPP_HTTP_PRIVATE_KEY_FILENAME
									- SL_NETAPP_HTTP_DEVICE_CERTIFICATE_FILENAME
									- SL_NETAPP_HTTP_CA_CERTIFICATE_FILE_NAME
									- SL_NETAPP_HTTP_TEMP_REGISTER_MDNS_SERVICE_NAME
									- SL_NETAPP_HTTP_TEMP_UNREGISTER_MDNS_SERVICE_NAME
								- For SL_NETAPP_DHCP_SERVER_ID:
									- SL_NETAPP_DHCP_SERVER_BASIC_OPT
								- For SL_NETAPP_MDNS_ID:
									- SL_NETAPP_MDNS_CONT_QUERY_OPT
									- SL_NETAPP_MDNS_QEVETN_MASK_OPT
									- SL_NETAPP_MDNS_TIMING_PARAMS_OPT 
								- For SL_NETAPP_DEVICE_ID:
									- SL_NETAPP_DEVICE_URN
									- SL_NETAPP_DEVICE_DOMAIN
								- For SL_NETAPP_DNS_CLIENT_ID:
									- SL_NETAPP_DNS_CLIENT_TIME
    \param[in] OptionLen       Option structure length

    \param[in] pOptionValue    Pointer to the option structure

	\par Persistent 			    
	\par
								<b>Reset</b>:                
													- SL_NETAPP_DHCP_SERVER_BASIC_OPT \n
	\par
								<b>Non- Persistent</b>:					
													- SL_NETAPP_HTTP_TEMP_REGISTER_MDNS_SERVICE_NAME
													- SL_NETAPP_HTTP_TEMP_UNREGISTER_MDNS_SERVICE_NAME \n
	\par
								<b>System Persistent</b>: 
													- SL_NETAPP_HTTP_PRIMARY_PORT_NUMBER
													- SL_NETAPP_HTTP_AUTH_CHECK
													- SL_NETAPP_HTTP_AUTH_NAME
													- SL_NETAPP_HTTP_AUTH_PASSWORD
													- SL_NETAPP_HTTP_AUTH_REALM
													- SL_NETAPP_HTTP_ROM_PAGES_ACCESS
													- SL_NETAPP_HTTP_SECONDARY_PORT_NUMBER
													- SL_NETAPP_HTTP_SECONDARY_PORT_ENABLE
													- SL_NETAPP_HTTP_PRIMARY_PORT_SECURITY_MODE
													- SL_NETAPP_HTTP_PRIVATE_KEY_FILENAME
													- SL_NETAPP_HTTP_DEVICE_CERTIFICATE_FILE_NAME
													- SL_NETAPP_HTTP_CA_CERTIFICATE_FILENAME
													- SL_NETAPP_MDNS_CONT_QUERY_OPT
													- SL_NETAPP_MDNS_QEVETN_MASK_OPT
													- SL_NETAPP_MDNS_TIMING_PARAMS_OPT 
													- SL_NETAPP_DEV_CONF_OPT_DEVICE_URN
													- SL_NETAPP_DEV_CONF_OPT_DOMAIN_NAME

    \return    Zero on success, or negative value if an error occurred.
    \sa sl_NetAppGet
    \note
    \warning
    \par Example
    
	- Setting DHCP Server (AP mode) parameters example:
	\code             
        SlNetAppDhcpServerBasicOpt_t dhcpParams; 
        _u8 outLen = sizeof(SlNetAppDhcpServerBasicOpt_t); 
        dhcpParams.lease_time      = 4096;                         // lease time (in seconds) of the IP Address
        dhcpParams.ipv4_addr_start =  SL_IPV4_VAL(192,168,1,10);   // first IP Address for allocation. IP Address should be set as Hex number - i.e. 0A0B0C01 for (10.11.12.1)
        dhcpParams.ipv4_addr_last  =  SL_IPV4_VAL(192,168,1,16);   // last IP Address for allocation. IP Address should be set as Hex number - i.e. 0A0B0C01 for (10.11.12.1)
        sl_NetAppStop(SL_NETAPP_DHCP_SERVER_ID);                  // Stop DHCP server before settings
        sl_NetAppSet(SL_NETAPP_DHCP_SERVER_ID, SL_NETAPP_DHCP_SRV_BASIC_OPT, outLen, (_u8* )&dhcpParams);  // set parameters
        sl_NetAppStart(SL_NETAPP_DHCP_SERVER_ID);                 // Start DHCP server with new settings
    \endcode
	<br>

	- Setting Device URN name: <br>
	Device name, maximum length of 32 characters 
    Device name affects URN name, and WPS file "device name" in WPS I.E (STA-WPS / P2P)
    In case no device URN name set, the default name is "mysimplelink" 
	In case of setting the device name with length 0, device will return to default name "mysimplelink"
    Allowed characters in device name are: 'a - z' , 'A - Z' , '0-9' and '-'
    \code
        _u8 *my_device = "MY-SIMPLELINK-DEV";
        sl_NetAppSet (SL_NETAPP_DEVICE_ID, SL_NETAPP_DEVICE_URN, strlen(my_device), (_u8 *) my_device);
    \endcode
	<br>
	
	- Register new temporary HTTP service name for MDNS (not persistent):  
	\code
		_u8 *my_http_temp_name = "New - Bonjour Service Name";
		sl_NetAppSet (SL_NETAPP_HTTP_SERVER_ID, SL_NETAPP_HTTP_TEMP_REGISTER_MDNS_SERVICE_NAME, strlen(my_http_temp_name), (_u8 *) my_http_temp_name); 
	\endcode
	<br>

	- Remove registration of current HTTP internal MDNS service (not persistent) :
	\code
		_u8 *old_http_name  = "0800285A7891@mysimplelink-022";
		sl_NetAppSet (SL_NETAPP_HTTP_SERVER_ID, SL_NETAPP_HTTP_TEMP_UNREGISTER_MDNS_SERVICE_NAME, strlen(old_http_name), (_u8 *) old_http_name); 
	\endcode
	<br>

	-   Set DNS client time example: <br>
	    Set DNS client (sl_NetAppDnsGetHostByName) timeout, two parameters max_response_time and number_retries. 
		number_retries: Max number of DNS request before sl_NetAppDnsGetHostByName failed, (up to 100 retries).
		max_response_time: DNS request timeout changed every retry, it`s start with 100 millisecond and increased every retry up to max_response_time milliseconds, (up to 2 seconds)
    \code
		SlNetAppDnsClientTime_t time;
     	time.MaxResponseTime = 2000;
		time.NumOfRetries = 30;
		sl_NetAppSet (SL_NETAPP_DNS_CLIENT_ID, SL_NETAPP_DNS_CLIENT_TIME, sizeof(time), (_u8 *)&time); 

    \endcode
	<br>

	- Start MDNS continuous querys: <br>
	In a continuous mDNS query mode, the device keeps sending queries to the network according to a specific service name. 
	The query will be sent in IPv4 and IPv6 (if enabled) format. To see the completed list of responding services sl_NetAppGetServiceList() need to be called
	\code
		const signed char AddService[40]			= "Printer._ipp._tcp.local";
		_i16 Status;

		Status = sl_NetAppSet(SL_NETAPP_MDNS_ID, SL_NETAPP_MDNS_CONT_QUERY_OPT,strlen(AddService) , &AddService);
	\endcode
	<br>

	- Stop MDNS:
	\code
		Status = sl_NetAppSet(SL_NETAPP_MDNS_ID, SL_NETAPP_MDNS_CONT_QUERY_OPT,0 , 0);
    \endcode
	<br>

	- Set MDNS timing parameters for service advertisement: <br>
	This option allows to control and reconfigures the timing parameters for service advertisement
	\code
		SlNetAppServiceAdvertiseTimingParameters_t Timing;
		_i16 Status;
		
		Timing.t = 200; // 2 seconds
		Timing.p = 2; // 2 repetitions
		Timing.k = 2; // Telescopic factor 2
		Timing.RetransInterval = 0; 
		Timing.Maxinterval = 0xFFFFFFFF;
		Timing.max_time = 5;

		Status = sl_NetAppSet(SL_NETAPP_MDNS_ID, SL_NETAPP_MDNS_TIMING_PARAMS_OPT,sizeof(Timing),&Timing);

    \endcode
	<br>

	- User-defined service types to monitor: <br>
	  In cases that the user decides not to get responses from certain 
	  types of services it should set the adapt bit in the event mask that is related to:
	\code
        
		// bit 0:  _ipp  
		// bit 1:  _device-info 
		// bit 2:  _http
		// bit 3:  _https
		// bit 4:  _workstation
		// bit 5:  _guid
		// bit 6:  _h323
		// bit 7:  _ntp
		// bit 8:  _objective
		// bit 9:  _rdp
		// bit 10: _remote
		// bit 11: _rtsp
		// bit 12: _sip
		// bit 13: _smb
		// bit 14: _soap
		// bit 15: _ssh
		// bit 16: _telnet
		// bit 17: _tftp
		// bit 18: _xmpp-client
		// bit 19: _raop

		
		_u32 EventMask;
		_i16 Status;
		
		EventMask = BIT0 | BIT1 | BIT18;
		Status = sl_NetAppSet(SL_NETAPP_MDNS_ID, SL_NETAPP_MDNS_QEVETN_MASK_OPT,sizeof(EventMask),&EventMask)

    \endcode
	<br>


*/
#if _SL_INCLUDE_FUNC(sl_NetAppSet)
_i16 sl_NetAppSet(const _u8 AppId ,const _u8 Option,const _u8 OptionLen,const _u8 *pOptionValue);
#endif

/*!
    \brief     Getting network applications configurations

    \param[in] AppId          Application id, could be one of the following: \n
                              - SL_NETAPP_HTTP_SERVER_ID
                              - SL_NETAPP_DHCP_SERVER_ID
                              - SL_NETAPP_DNS_SERVER_ID
							  - SL_NETAPP_DEVICE_ID
							  - SL_NETAPP_DNS_CLIENT_ID

    \param[in] Option         Get option, could be one of the following: \n
                              - SL_NETAPP_DHCP_SERVER_ID:
                                 - SL_NETAPP_DHCP_SRV_BASIC_OPT
                              - SL_NETAPP_HTTP_SERVER_ID:
                                 - SL_NETAPP_HTTP_PRIMARY_PORT_NUMBER
                                 - SL_NETAPP_HTTP_AUTH_CHECK
                                 - SL_NETAPP_HTTP_AUTH_NAME
                                 - SL_NETAPP_HTTP_AUTH_PASSWORD
                                 - SL_NETAPP_HTTP_AUTH_REALM
                                 - SL_NETAPP_HTTP_ROM_PAGES_ACCESS
								 - SL_NETAPP_HTTP_SECONDARY_PORT_NUMBER
								 - SL_NETAPP_HTTP_SECONDARY_PORT_ENABLE
								 - SL_NETAPP_HTTP_PRIMARY_PORT_SECURITY_MODE
                              - SL_NETAPP_MDNS_ID:
                                 - SL_NETAPP_MDNS_CONT_QUERY_OPT
                                 - SL_NETAPP_MDNS_QEVETN_MASK_OPT
                                 - SL_NETAPP_MDNS_TIMING_PARAMS_OPT
                              - SL_NETAPP_DEVICE_ID:
                                 - SL_NETAPP_DEVICE_URN
                                 - SL_NETAPP_DEVICE_DOMAIN
							  - SL_NETAPP_DNS_CLIENT_ID:
							     - SL_NETAPP_DNS_CLIENT_TIME


    \param[in] pOptionLen     The length of the allocated memory as input, when the
                                        function complete, the value of this parameter would be
                                        the len that actually read from the device.\n
                                        If the device return length that is longer from the input
                                        value, the function will cut the end of the returned structure
                                        and will return ESMALLBUF

    \param[out] pOptionValue      pointer to the option structure which will be filled with the response from the device

    \return    Zero on success, or negative value if an error occurred.

    \sa	sl_NetAppSet
    \note
    \warning
    \par Example

	- Getting DHCP Server parameters example:
    \code
         SlNetAppDhcpServerBasicOpt_t dhcpParams;
         _u8 outLen = sizeof(SlNetAppDhcpServerBasicOpt_t);
         sl_NetAppGet(SL_NETAPP_DHCP_SERVER_ID, SL_NETAPP_SET_DHCP_SRV_BASIC_OPT, &outLen, (_u8* )&dhcpParams);
 
         printf("DHCP Start IP %d.%d.%d.%d End IP %d.%d.%d.%d Lease time seconds %d\n",                                                           
            SL_IPV4_BYTE(dhcpParams.ipv4_addr_start,3),SL_IPV4_BYTE(dhcpParams.ipv4_addr_start,2),
            SL_IPV4_BYTE(dhcpParams.ipv4_addr_start,1),SL_IPV4_BYTE(dhcpParams.ipv4_addr_start,0), 
            SL_IPV4_BYTE(dhcpParams.ipv4_addr_last,3),SL_IPV4_BYTE(dhcpParams.ipv4_addr_last,2),
            SL_IPV4_BYTE(dhcpParams.ipv4_addr_last,1),SL_IPV4_BYTE(dhcpParams.ipv4_addr_last,0),         
            dhcpParams.lease_time);    
    \endcode
	<br>

	- Getting device URN name: <br>
    Maximum length of 32 characters of device name. 
    Device name affects URN name, own SSID name in AP mode, and WPS file "device name" in WPS I.E (STA-WPS / P2P)
    in case no device URN name set, the default name is "mysimplelink" 
    \code
         _u8 my_device_name[SL_NETAPP_MAX_DEVICE_URN_LEN];
         sl_NetAppGet (SL_NETAPP_DEVICE_ID, SL_NETAPP_DEVICE_URN, strlen(my_device_name), (_u8 *)my_device_name); 
    \endcode
	<br>
    
	- Getting DNS client time: <br>
	  Get DNS client (sl_NetAppDnsGetHostByName) timeout, two parameters max_response_time and number_retries. 
	  number_retries: Max number of DNS request before sl_NetAppDnsGetHostByName failed.
	  max_response_time: DNS request timeout changed every retry, it`s start with 100 millisecond and increased every retry up to max_response_time milliseconds
	\code
		SlNetAppDnsClientTime_t time;
		_u8 pOptionLen  = sizeof(time);
		sl_NetAppGet (SL_NETAPP_DNS_CLIENT_ID, SL_NETAPP_DNS_CLIENT_TIME, &pOptionLen, (_u8 *)&time); 
    \endcode
	<br>

	- Getting active applications: <br>
	  Get active applications for active role. return value is mask of the active application (similar defines as sl_NetAppStart\sl_NetAppStop):
	\code
		_u32 AppBitMap;
		_u8 pOptionLen  = sizeof(AppBitMap);
		sl_NetAppGet (SL_NETAPP_STATUS, SL_NETAPP_STATUS_ACTIVE_APP, &pOptionLen, (_u8 *)&AppBitMap); 

    \endcode

*/
#if _SL_INCLUDE_FUNC(sl_NetAppGet)
_i16 sl_NetAppGet(const _u8 AppId, const _u8 Option,_u8 *pOptionLen, _u8 *pOptionValue);
#endif

/*!
    \brief     Function for sending Netapp response or data following a Netapp request event (i.e. HTTP GET request)


    \param[in] Handle       Handle to send the data to. Should match the handle received in the Netapp request event
    \param[in] DataLen      Data Length
    \param[in] pData         Data to send. Can be just data payload or metadata (depends on flags)
    \param[out] Flags      Can have the following values:
                                    - SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION - More data will arrive in subsequent calls to NetAppSend
                                    - SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA	  - 0 - data is payload, 1 - data is metadata
                                    - SL_NETAPP_REQUEST_RESPONSE_FLAGS_ACCUMULATION - The network processor should accumulate the data chunks and will process it when it is completelly received

    \return    Zero on success, or negative error code on failure

    \sa		sl_NetAppRecv 
    \note
    \warning
*/
#if _SL_INCLUDE_FUNC(sl_NetAppSend)
_u16 sl_NetAppSend( _u16 Handle, _u16 DataLen, _u8 *pData, _u32 Flags);
#endif

/*!
    \brief     Function for retrieving data from the network processor following a Netapp request event (i.e. HTTP POST request)

    \param[in]      Handle     Handle to receive data from. Should match the handle received in the Netapp request event
    \param[in,out]  *DataLen   Max buffer size (in) / Actual data received (out)
    \param[out]     *pData     Data received
    \param[in,out]  *Flags     Can have the following values:
                                        - SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION (out) 
										- More data is pending in the network procesor. Application should continue reading the data by calling sl_NetAppRecv again
										
    \return    Zero on success, or negative error code on failure

    \sa		sl_NetAppSend 
    \note    
    \warning    handle is received in the sl_NetAppRequestHandler callback. Handle is valid untill all data is receive from the network processor.
*/
#if _SL_INCLUDE_FUNC(sl_NetAppRecv)
_SlReturnVal_t sl_NetAppRecv( _u16 Handle, _u16 *DataLen, _u8 *pData, _u32 *Flags);
#endif


/*!

 Close the Doxygen group.
 @}

 */


#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif    /*  __NETAPP_H__ */

