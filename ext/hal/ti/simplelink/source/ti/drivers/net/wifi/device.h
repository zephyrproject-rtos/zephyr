/*
 * device.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef __DEVICE_H__
#define __DEVICE_H__



#ifdef __cplusplus
extern "C" {
#endif

/*!
	\defgroup Device 
	\short Controls the behaviour of the CC31xx/CC32xx device (start/stop, events masking and obtaining specific device status)

*/

/*!

    \addtogroup Device
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/
/* Convert event id to event mask to be used in sl_DeviceEventMaskSet and sl_DeviceEventMaskGet */     
#define SL_DEVICE_EVENT_BIT(EventId)        (SL_WLAN_VAL_2_MASK(EventId,1) )     



typedef enum
{
	SL_DEVICE_EVENT_FATAL_DEVICE_ABORT = 1,
	SL_DEVICE_EVENT_FATAL_DRIVER_ABORT,
	SL_DEVICE_EVENT_FATAL_SYNC_LOSS,
	SL_DEVICE_EVENT_FATAL_NO_CMD_ACK,
	SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT,
	SL_DEVICE_EVENT_RESET_REQUEST,
	SL_DEVICE_EVENT_ERROR,
	SL_DEVICE_EVENT_MAX

} SlDeviceEventId_e;

typedef struct
{
	_i16    Status;
	_u16	Caller;
}SlDeviceEventResetRequest_t;

typedef enum
{
    SL_DEVICE_SOURCE_OTHER,
    SL_DEVICE_SOURCE_WLAN,
    SL_DEVICE_SOURCE_NETCFG,
    SL_DEVICE_SOURCE_NETAPP, 
    SL_DEVICE_SOURCE_SECURITY,

    SL_DEVICE_SOURCE_LAST = 0xFF /* last one */
}SlDeviceSource_e;

typedef struct
{
	SlDeviceSource_e	Source;
	_i16				Code;
}SlDeviceEventError_t;

typedef union
{
	SlDeviceEventResetRequest_t	ResetRequest;
	SlDeviceEventError_t		Error;
}SlDeviceEventData_u;



typedef enum
{
	SL_DEVICE_RESET_REQUEST_CALLER_PROVISIONING,
	SL_DEVICE_RESET_REQUEST_CALLER_PROVISIONING_EXTERNAL_CONFIGURATION,
	SL_DEVICE_RESET_REQUEST_NUM_OF_CALLERS
}SlDeviceResetRequestCaller_e;

typedef struct
{
	_u32				Id;
	SlDeviceEventData_u	Data;
}SlDeviceEvent_t;

/*!
	\cond DOXYGEN_REMOVE
*/
void slcb_DeviceEvtHdlr(SlDeviceEvent_t* pEvent);
/*!
	\endcond
*/

typedef struct
{
	_u32				Code;
	_u32				Value;
} SlDeviceFatalDeviceAssert_t;


typedef struct
{
	_u32 Code;
} SlDeviceFatalNoCmdAck_t, SlDeviceFatalCmdTimeout_t;


typedef union
{	
	SlDeviceFatalDeviceAssert_t		DeviceAssert;
	SlDeviceFatalNoCmdAck_t			NoCmdAck;
	SlDeviceFatalCmdTimeout_t		CmdTimeout;
}SlDeviceFatalData_u;


typedef struct
{
	_u32					Id;
	SlDeviceFatalData_u		Data;
}SlDeviceFatal_t;



/*
    Declare the different IDs for sl_DeviceGet and sl_DeviceSet
 */  
#define SL_DEVICE_GENERAL				          (1)
#define SL_DEVICE_IOT				  			  (4)
#define SL_DEVICE_STATUS                          (2)

/*
    Declare the different Options for SL_DEVICE_GENERAL in sl_DeviceGet and sl_DeviceSet
 */  
#define SL_DEVICE_GENERAL_DATE_TIME (11)
#define SL_DEVICE_GENERAL_PERSISTENT (5)
#define SL_DEVICE_GENERAL_VERSION   (12)

/*
    Declare the different Options for SL_DEVICE_IOT in sl_DeviceGet and sl_DeviceSet
*/  
#define SL_DEVICE_IOT_UDID			  (41)

/* Events list to mask/unmask*/
#define SL_DEVICE_EVENT_CLASS_DEVICE                     (1)
#define SL_DEVICE_EVENT_CLASS_WLAN                       (2)
#define SL_DEVICE_EVENT_CLASS_BSD                        (3)
#define SL_DEVICE_EVENT_CLASS_NETAPP                     (4)
#define SL_DEVICE_EVENT_CLASS_NETCFG                     (5)
#define SL_DEVICE_EVENT_CLASS_FS                         (6)
#define SL_DEVICE_EVENT_CLASS_NETUTIL			         (7)


/******************  DEVICE CLASS status ****************/
#define SL_DEVICE_EVENT_DROPPED_DEVICE_ASYNC_GENERAL_ERROR       (0x00000001L)
#define SL_DEVICE_STATUS_DEVICE_SMART_CONFIG_ACTIVE              (0x80000000L)
  
/******************  WLAN CLASS status ****************/
#define SL_DEVICE_EVENT_DROPPED_WLAN_WLANASYNCONNECTEDRESPONSE        (0x00000001L)
#define SL_DEVICE_EVENT_DROPPED_WLAN_WLANASYNCDISCONNECTEDRESPONSE    (0x00000002L)
#define SL_DEVICE_EVENT_DROPPED_WLAN_STA_CONNECTED                    (0x00000004L)
#define SL_DEVICE_EVENT_DROPPED_WLAN_STA_DISCONNECTED                 (0x00000008L)
#define SL_DEVICE_EVENT_DROPPED_WLAN_P2P_DEV_FOUND                    (0x00000010L)
#define SL_DEVICE_EVENT_DROPPED_WLAN_CONNECTION_FAILED                (0x00000020L)
#define SL_DEVICE_EVENT_DROPPED_WLAN_P2P_NEG_REQ_RECEIVED             (0x00000040L)
#define SL_DEVICE_EVENT_DROPPED_WLAN_RX_FILTERS                       (0x00000080L)
#define SL_DEVICE_STATUS_WLAN_STA_CONNECTED                           (0x80000000L)
                      
/******************  NETAPP CLASS status ****************/
#define SL_DEVICE_EVENT_DROPPED_NETAPP_IPACQUIRED                      (0x00000001L)
#define SL_DEVICE_EVENT_DROPPED_NETAPP_IPACQUIRED_V6                   (0x00000002L)
#define SL_DEVICE_EVENT_DROPPED_NETAPP_IP_LEASED                       (0x00000004L)
#define SL_DEVICE_EVENT_DROPPED_NETAPP_IP_RELEASED                     (0x00000008L)
#define SL_DEVICE_EVENT_DROPPED_NETAPP_IPV4_LOST				       (0x00000010L)
#define SL_DEVICE_EVENT_DROPPED_NETAPP_DHCP_ACQUIRE_TIMEOUT			   (0x00000020L)
#define SL_DEVICE_EVENT_DROPPED_NETAPP_IP_COLLISION                    (0x00000040L)
#define SL_DEVICE_EVENT_DROPPED_NETAPP_IPV6_LOST					   (0x00000080L)
                      
/******************  BSD CLASS status ****************/
#define SL_DEVICE_EVENT_DROPPED_SOCKET_TXFAILEDASYNCRESPONSE        (0x00000001L)
  
/******************  FS CLASS  ****************/
  


/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

#ifdef SL_IF_TYPE_UART
typedef struct  
{
    _u32             BaudRate;
    _u8              FlowControlEnable;
    _u8              CommPort;
} SlDeviceUartIfParams_t;
#endif

#ifdef SL_IF_TYPE_UART

#define SL_DEVICE_BAUD_9600		(9600L)
#define SL_DEVICE_BAUD_14400	(14400L)
#define SL_DEVICE_BAUD_19200	(19200L)
#define SL_DEVICE_BAUD_38400	(38400L)
#define SL_DEVICE_BAUD_57600	(57600L)
#define SL_DEVICE_BAUD_115200	(115200L)
#define SL_DEVICE_BAUD_230400	(230400L)
#define SL_DEVICE_BAUD_460800	(460800L)
#define SL_DEVICE_BAUD_921600	(921600L)

#endif

typedef struct
{
    _u32                ChipId;
    _u8                 FwVersion[4];
    _u8                 PhyVersion[4];
    _u8                 NwpVersion[4];
    _u16                RomVersion;
    _u16                Padding;
}SlDeviceVersion_t;


typedef struct  
{
       /* time */
    _u32                tm_sec;
    _u32                tm_min;
    _u32                tm_hour;
       /* date */
    _u32                tm_day; /* 1-31 */
    _u32                tm_mon; /* 1-12 */
    _u32                tm_year; /*  YYYY 4 digits  */
    _u32                tm_week_day; /* not required */
    _u32                tm_year_day; /* not required */ 
    _u32                reserved[3];  
}SlDateTime_t;


/******************************************************************************/
/* Type declarations                                                          */
/******************************************************************************/
typedef struct  
{
    _u32    ChipId;
    _u32    MoreData;
}SlDeviceInitInfo_t;

typedef void (*P_INIT_CALLBACK)(_u32 Status, SlDeviceInitInfo_t *DeviceInitInfo);

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

/*!
    \brief Start the SimpleLink device
    
    This function initialize the communication interface, set the enable pin 
    of the device, and call to the init complete callback.

    \param[in]      pIfHdl              Opened Interface Object. In case the interface 
                                        must be opened outside the SimpleLink Driver, the
                                        user might give the handler to be used in \n
                                        any access of the communication interface with the 
                                        device (UART/SPI). \n
                                        The SimpleLink driver will open an interface port
                                        only if this parameter is null! \n
    \param[in]      pDevName            The name of the device to open. Could be used when 
                                        the pIfHdl is null, to transfer information to the 
                                        open interface function \n
                                        This pointer could be used to pass additional information to
                                        sl_IfOpen in case it is required (e.g. UART com port name)
    \param[in]      pInitCallBack       Pointer to function that would be called
                                        on completion of the initialization process.\n
                                        If this parameter is NULL the function is 
                                        blocked until the device initialization 
                                        is completed, otherwise the function returns 
                                        immediately.

    \return         Returns the current active role (STA/AP/P2P) or an error code:
                    - ROLE_STA, ROLE_AP, ROLE_P2P in case of success, 
                      otherwise in failure one of the following is return:
                    - SL_ERROR_ROLE_STA_ERR  (Failure to load MAC/PHY in STA role)
                    - SL_ERROR_ROLE_AP_ERR  (Failure to load MAC/PHY in AP role)
                    - SL_ERROR_ROLE_P2P_ERR  (Failure to load MAC/PHY in P2P role)
					- SL_ERROR_CALIB_FAIL (Failure of calibration)
					- SL_ERROR_FS_CORRUPTED_ERR (FS is corrupted, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram))
					- SL_ERROR_FS_ALERT_ERR (Device is locked, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram))
					- SL_ERROR_RESTORE_IMAGE_COMPLETE (Return to factory image completed, perform reset)
					- SL_ERROR_GENERAL_ERR (General error during init)

     \sa             sl_Stop

    \note           Belongs to \ref basic_api

    \warning        This function must be called before any other SimpleLink API is used, or after sl_Stop is called for reinit the device
    \par            Example:

	- Open interface without callback routine. The interface name and handler are
      handled by the sl_IfOpen routine: 
    \code            
		if( sl_Start(NULL, NULL, NULL) < 0 )
		{
			LOG("Error opening interface to device\n"); 
		}
    \endcode
	<br>

	- Open interface with a callback routine:
	\code
		void SimpleLinkInitCallback(_u32 status)
		{
			LOG("Handle SimpleLink Interface acording to ststus %d\n", status);
		}
					
		void main(void) 
		{
			if  (sl_Start(NULL, NULL, SimpleLinkInitCallback) < 0)
			{
				LOG("Error opening interface to device\n");
			}
		}
	\endcode

*/
#if _SL_INCLUDE_FUNC(sl_Start)
_i16 sl_Start(const void* pIfHdl, _i8*  pDevName, const P_INIT_CALLBACK pInitCallBack);
#endif

/*!
    \brief Stop the SimpleLink device

    This function clears the enable pin of the device, closes the communication \n
    interface and invokes the stop complete callback 

    \param[in]      Timeout                       Stop timeout in msec. Should be used to give the device time to finish \n 
                                                  any transmission/reception that is not completed when the function was called. \n
                    Additional options:
                    - 0                             Enter to hibernate immediately \n
                    - 0xFFFF                        Host waits for device's response before \n
                                                    hibernating, without timeout protection \n      
                    - 0 < Timeout[msec] < 0xFFFF    Host waits for device's response before \n
                                                    hibernating, with a defined timeout protection \n
                                                    This timeout defines the max time to wait. The NWP \n
                                                    response can be sent earlier than this timeout.

    \return         Zero on success, or a negative value if an error occurred     
 
    \sa             sl_Start

    \note           This API will shutdown the device and invoke the "i/f close" function regardless \n
                    if it was opened implicitly or explicitly. \n
                    It is up to the platform interface library to properly handle interface close \n
                    routine \n
                    Belongs to \ref basic_api \n
    \warning     
*/
#if _SL_INCLUDE_FUNC(sl_Stop)
_i16 sl_Stop(const _u16 Timeout);
#endif


/*!
    \brief     Setting device configurations

    \param[in] DeviceSetId   configuration id:  
												- SL_DEVICE_GENERAL

    \param[in] Option        configurations option: 
													- SL_DEVICE_GENERAL_DATE_TIME
													- SL_DEVICE_GENERAL_PERSISTENT
    \param[in] ConfigLen     configurations len
    \param[in] pValues       configurations values
	
    \return    Zero on success, or a negative value if an error occurred
    \par Persistent
		SL_DEVICE_GENERAL_DATE_TIME - Non-Persistent (Kept during hibernate)
		SL_DEVICE_GENERAL_PERSISTENT - Persistent
    \sa         
    \note 
    \warning     
    \par   Examples:

	- Setting device time and date example:
    \code
         SlDateTime_t dateTime= {0};
         dateTime.tm_day =   (_u32)23;          // Day of month (DD format) range 1-31
         dateTime.tm_mon =   (_u32)6;           // Month (MM format) in the range of 1-12 
         dateTime.tm_year =  (_u32)2014;        // Year (YYYY format) 
         dateTime.tm_hour =  (_u32)17;          // Hours in the range of 0-23
         dateTime.tm_min =   (_u32)55;          // Minutes in the range of 0-59
         dateTime.tm_sec =   (_u32)22;          // Seconds in the range of  0-59
         sl_DeviceSet(SL_DEVICE_GENERAL,
                   SL_DEVICE_GENERAL_DATE_TIME,
                   sizeof(SlDateTime_t),
                   (_u8 *)(&dateTime));
	\endcode
	<br>

	- Setting system persistent configuration: <br>
	  Sets the default system-wide configuration persistence mode. 
	  In case true, all APIs that follow 'system configured' persistence (see persistence attribute noted per API) shall maintain the configured settings. 
	  In case false, all calls to APIs that follow 'system configured' persistence shall be volatile. Configuration should revert to default after reset or power recycle
	\code
		_u8 persistent = 1;
		sl_DeviceSet(SL_DEVICE_GENERAL,
                   SL_DEVICE_GENERAL_PERSISTENT,
                   sizeof(_u8),
                   (_u8 *)(&persistent));
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_DeviceSet)
_i16 sl_DeviceSet(const _u8 DeviceSetId ,const _u8 Option,const _u16 ConfigLen,const _u8 *pValues);
#endif

/*!
    \brief      Internal function for getting device configurations
    \param[in]  DeviceGetId   configuration id:
												- SL_DEVICE_STATUS
												- SL_DEVICE_GENERAL
												- SL_DEVICE_IOT

    \param[out] pOption   Get configurations option:
												- SL_DEVICE_STATUS:
													- SL_DEVICE_EVENT_CLASS_DEVICE
													- SL_DEVICE_EVENT_CLASS_WLAN  
													- SL_DEVICE_EVENT_CLASS_BSD   
													- SL_DEVICE_EVENT_CLASS_NETAPP
													- SL_DEVICE_EVENT_CLASS_NETCFG
													- SL_DEVICE_EVENT_CLASS_FS 
												- SL_DEVICE_GENERAL:
													- SL_DEVICE_GENERAL_VERSION
													- SL_DEVICE_GENERAL_DATE_TIME  
													- SL_DEVICE_GENERAL_PERSISTENT   
												- SL_DEVICE_IOT:
													- SL_DEVICE_IOT_UDID
										
    \param[out] pConfigLen   The length of the allocated memory as input, when the
                             function complete, the value of this parameter would be
                             the len that actually read from the device.\n 
                             If the device return length that is longer from the input 
                             value, the function will cut the end of the returned structure
                             and will return SL_ESMALLBUF
    \param[out] pValues      Get requested configurations values
    \return     Zero on success, or a negative value if an error occurred
    \sa         
    \note 
    \warning  
    \par        Examples

	- Getting WLAN class status (status is always cleared on read):
    \code
		 _u32 statusWlan;
		 _u8 pConfigOpt;
		 _u16 pConfigLen;
		 pConfigOpt = SL_DEVICE_EVENT_CLASS_WLAN;
		 pConfigLen = sizeof(_u32);
		 sl_DeviceGet(SL_DEVICE_STATUS,&pConfigOpt,&pConfigLen,(_u8 *)(&statusWlan));
		 if (SL_DEVICE_STATUS_WLAN_STA_CONNECTED & statusWlan )
		 {
			printf("Device is connected\n");
		 }
		 if (SL_DEVICE_EVENT_DROPPED_WLAN_RX_FILTERS & statusWlan )
		 {
			printf("RX filer event dropped\n");
		 }

	 \endcode
	 <br>

	-  Getting version:
	\code
		SlDeviceVersion_t ver;
		pConfigLen = sizeof(ver);
		pConfigOpt = SL_DEVICE_GENERAL_VERSION;
		sl_DeviceGet(SL_DEVICE_GENERAL,&pConfigOpt,&pConfigLen,(_u8 *)(&ver));
		printf("CHIP %d\nMAC 31.%d.%d.%d.%d\nPHY %d.%d.%d.%d\nNWP %d.%d.%d.%d\nROM %d\nHOST %d.%d.%d.%d\n",
						 ver.ChipId,
						 ver.FwVersion[0],ver.FwVersion[1],
						 ver.FwVersion[2],ver.FwVersion[3],
						 ver.PhyVersion[0],ver.PhyVersion[1],
						 ver.PhyVersion[2],ver.PhyVersion[3],
						 ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
						 ver.RomVersion,
						 SL_MAJOR_VERSION_NUM,SL_MINOR_VERSION_NUM,SL_VERSION_NUM,SL_SUB_VERSION_NUM);

	\endcode
	<br>

	- Getting Device time and date:
	\code
         SlDateTime_t dateTime =  {0};  
         _i16 configLen = sizeof(SlDateTime_t); 
         _i8 configOpt = SL_DEVICE_GENERAL_DATE_TIME;
         sl_DeviceGet(SL_DEVICE_GENERAL,&configOpt, &configLen,(_u8 *)(&dateTime)); 
         
         printf("Day %d,Mon %d,Year %d,Hour %,Min %d,Sec %d\n",dateTime.tm_day,dateTime.tm_mon,dateTime.tm_year,
                 dateTime.tm_hour,dateTime.tm_min,dateTime.tm_sec);
     \endcode

	- Getting persistency system configuration:
    \code
        _i16 configLen = sizeof(_u8); 
        _i8 configOpt = SL_DEVICE_GENERAL_PERSISTENT;
        sl_DeviceGet(SL_DEVICE_GENERAL,&configOpt, &configLen,&persistent); 
    \endcode

*/
#if _SL_INCLUDE_FUNC(sl_DeviceGet)
_i16 sl_DeviceGet(const _u8 DeviceGetId, _u8 *pOption,_u16 *pConfigLen, _u8 *pValues);
#endif


/*!
    \brief          Set asynchronous event mask
    
    Mask asynchronous events from the device.\n 
	Masked events do not generate asynchronous messages from the device.\n
    By default - all events are active



    \param[in]      EventClass          The classification groups that the 
                                        mask is referred to. Need to be one of
                                        the following:
                                        - SL_DEVICE_EVENT_CLASS_DEVICE
                                        - SL_DEVICE_EVENT_CLASS_WLAN  
                                        - SL_DEVICE_EVENT_CLASS_BSD   
                                        - SL_DEVICE_EVENT_CLASS_NETAPP
                                        - SL_DEVICE_EVENT_CLASS_NETCFG
                                        - SL_DEVICE_EVENT_CLASS_FS 


    \param[in]      Mask               Event Mask bitmap. Valid mask are (per group):
                                        - SL_DEVICE_EVENT_CLASS_WLAN user events
                                          - SL_WLAN_EVENT_CONNECT    
										  - SL_WLAN_EVENT_P2P_CONNECT
                                          - SL_WLAN_EVENT_DISCONNECT
										  - SL_WLAN_EVENT_P2P_DISCONNECT
										  - SL_WLAN_EVENT_STA_ADDED
										  - SL_WLAN_EVENT_STA_REMOVED
										  - SL_WLAN_EVENT_P2P_CLIENT_ADDED
										  - SL_WLAN_EVENT_P2P_CLIENT_REMOVED
										  - SL_WLAN_EVENT_P2P_DEVFOUND
										  - SL_WLAN_EVENT_P2P_REQUEST
										  - SL_WLAN_EVENT_P2P_CONNECTFAIL
										  - SL_WLAN_EVENT_PROVISIONING_STATUS
										  - SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED
                                          - SL_WLAN_EVENT_RXFILTER

                                        - SL_DEVICE_EVENT_CLASS_DEVICE user events
                                          - SL_DEVICE_EVENT_ERROR

                                        - SL_DEVICE_EVENT_CLASS_BSD user events
                                          - SL_SOCKET_TX_FAILED_EVENT     
                                          - SL_SOCKET_ASYNC_EVENT 

                                        - SL_DEVICE_EVENT_CLASS_NETAPP user events
                                          - SL_NETAPP_EVENT_IPV4_ACQUIRED 
                                          - SL_NETAPP_EVENT_IPV6_ACQUIRED
										  - SL_NETAPP_EVENT_DHCPV4_LEASED
										  - SL_NETAPP_EVENT_DHCPV4_RELEASED
										  - SL_NETAPP_EVENT_IP_COLLISION
										  - SL_NETAPP_EVENT_IPV4_LOST
										  - SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT
										  - SL_NETAPP_EVENT_IPV6_LOST

     
    \return          Zero on success, or a negative value if an error occurred        
	\par Persistent  System Persistent
    \sa              sl_DeviceEventMaskGet

    \note            Belongs to \ref ext_api \n
    \warning     
    \par             Example

	- Masking connection/disconnection async events from WLAN class: 
    \code
		sl_DeviceEventMaskSet(SL_DEVICE_EVENT_CLASS_WLAN, (SL_DEVICE_EVENT_BIT(SL_WLAN_EVENT_CONNECT) | SL_DEVICE_EVENT_BIT(SL_WLAN_EVENT_DISCONNECT) ) );
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_DeviceEventMaskSet)
_i16 sl_DeviceEventMaskSet(const _u8 EventClass ,const _u32 Mask);
#endif

/*!
    \brief Get current event mask of the device
    
    Return the events bit mask from the device. In case event is 
    masked, the device will not send that event.  
    
    \param[in]      EventClass          The classification groups that the 
                                        mask is referred to. Need to be one of
                                        the following:
                                        - SL_DEVICE_EVENT_CLASS_GLOBAL
                                        - SL_DEVICE_EVENT_CLASS_DEVICE
                                        - SL_DEVICE_EVENT_CLASS_WLAN  
                                        - SL_DEVICE_EVENT_CLASS_BSD   
                                        - SL_DEVICE_EVENT_CLASS_NETAPP
                                        - SL_DEVICE_EVENT_CLASS_NETCFG
                                        - SL_DEVICE_EVENT_CLASS_FS 

    \param[out]      pMask              Pointer to mask bitmap where the 
                                        value should be stored. Bitmasks are the same as in \ref sl_DeviceEventMaskSet
     
    \return         Zero on success, or a negative value if an error occurred       
     
    \sa             sl_DeviceEventMaskSet         

    \note           Belongs to \ref ext_api

    \warning     
    \par           Example

	- Getting an event mask for WLAN class:
    \code
		_u32 maskWlan;
		sl_DeviceEventMaskGet(SL_DEVICE_EVENT_CLASS_WLAN,&maskWlan);
   \endcode
*/
#if _SL_INCLUDE_FUNC(sl_DeviceEventMaskGet)
_i16 sl_DeviceEventMaskGet(const _u8 EventClass,_u32 *pMask);
#endif


/*!
    \brief The SimpleLink task entry

    This function must be called from the main loop or from dedicated thread in
    the following cases:
        - Non-Os Platform - should be called from the mail loop
        - Multi Threaded Platform when the user does not implement the external spawn functions - 
           should be called from dedicated thread allocated to the SimpleLink driver.
           In this mode the function never return.

	\par parameters			
			None 
	
    \return         None
    \sa
    \note           Belongs to \ref basic_api

    \warning        This function must be called from a thread that is start running before 
                    any call to other SimpleLink API
*/
#if _SL_INCLUDE_FUNC(sl_Task)
void* sl_Task(void* pEntry);
#endif




/*!
    \brief Setting the internal uart mode 

    \param[in]      pUartParams          Pointer to the uart configuration parameter set: 
                                         - baudrate     - up to 711 Kbps
                                         - flow control - enable/disable 
                                         - comm port    - the comm port number
    
    \return         On success zero is returned, otherwise - Failed.   
    \sa
    \note           Belongs to \ref basic_api

    \warning        This function must consider the host uart capability
*/
#ifdef SL_IF_TYPE_UART
#if _SL_INCLUDE_FUNC(sl_DeviceUartSetMode)
_i16 sl_DeviceUartSetMode(const SlDeviceUartIfParams_t* pUartParams);
#endif
#endif

/*!

 Close the Doxygen group.
 @}

 */


#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif  /*  __DEVICE_H__ */


