/*
 * simplelink.h - CC31xx/CC32xx Host Driver Implementation
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


/*!
    \mainpage SimpleLink Driver

    \section intro_sec Introduction

 The SimpleLink(tm) CC31xx/CC32xx family allows to add Wi-Fi and networking capabilities
 to low-cost embedded products without having prior Wi-Fi, RF or networking expertise.\n
 The CC31xx/CC32xx is an ideal solution for microcontroller-based sensor and control
 applications such as home appliances, home automation and smart metering.\n
 The CC31xx/CC32xx has integrated a comprehensive TCP/IP network stack, Wi-Fi driver and
 security supplicant leading to easier portability to microcontrollers, to an
 ultra-low memory footprint, all without compromising the capabilities and robustness
 of the final application.

 

 \section modules_sec Module Names
 To make it simple, TI's SimpleLink CC31xx/CC32xx platform capabilities were divided into modules by topic (Silo).\n
 These capabilities range from basic device management through wireless
 network configuration, standard BSD socket and much more.\n
 Listed below are the various modules in the SimpleLink CC31xx/CC32xx driver:
     -# \ref Device 	- Controls the behaviour of the CC31xx/CC32xx device (start/stop, events masking and obtaining specific device status)
     -# \ref FileSystem	- Provides file system capabilities to TI's CC31XX that can be used by both the CC31XX device and the user.
     -# \ref NetApp 	- Activates networking applications, such as: HTTP Server, DHCP Server, Ping, DNS and mDNS.
     -# \ref NetCfg 	- Controls the configuration of the device addresses (i.e. IP and MAC addresses)
     -# \ref NetUtil 	- Networking related commands and configuration
     -# \ref Socket 	- Controls standard client/server sockets programming options and capabilities
     -# \ref Wlan 		- Controls the use of the WiFi WLAN module including:
							- Connection features, such as: profiles, policies, SmartConfig(tm)
							- Advanced WLAN features, such as: scans, rx filters and rx statistics collection
	 -# \ref UserEvents	- Function prototypes for event callback handlers

	\section persistency_sec Persistency
			The SimpleLink(tm) device support few different persistency types for settings and configurations:\n
			- <b>Temporary</b>			-	Effective immediately but returned to default after reset\n
			- <b>System Persistent</b> 	-	Effective immediately and kept after reset according\n
										-	to system persistent mode\n
			- <b>Persistent</b> 		-	Effective immediately and kept after reset regardless the system persistent mode\n
			- <b>Optionally Persistent</b>-	Effective immediately and kept after reset according to a parameter in the API call\n
			- <b>Reset</b> 				-	Persistent but effective only after reset\n
			\n
			For all Set/Get function in this guide, the type of persistency per relevant parameters will be 
			described as part of the function description\n

 \section         proting_sec     Porting Guide

 The porting of the SimpleLink host driver to any new platform is based on few simple steps.\n
 This guide takes you through this process step by step. Please follow the instructions
 carefully to avoid any problems during this process and to enable efficient and proper
 work with the device.\n
 Please notice that all modifications and porting adjustments of the driver should be
 made in the user.h header file only. Keeping this method ensure smoothly
transaction to new versions of the driver in the future!\n

The porting process consists of few simple steps:
-# Create user.h for the target platform
-# Select the capabilities set
-# Bind the device enable/disable line
-# Writing your interface communication driver
-# Choose your memory management model
-# OS adaptation
-# Set your asynchronous event handlers
-# Testing

For host interface details please refer to:
http://processors.wiki.ti.com/index.php/CC31xx_Host_Interface

Please see the rest of the page for more details about the different steps.

 \subsection     porting_step1   Step 1 - Create your own user.h file

 The first step is to create a <b><i>user.h</i></b> file that will include your configurations and
 adjustments. \n
 The file should be located in the porting directory (the porting directory is in the same level as the source directory)\n
 It is recommended to use the empty template provided as part of this driver or
 file of other platform such as MSP432 or CC3220, from one of the wide range
 of example applications provided by Texas Instruments.


 \subsection    porting_step2   Step 2 - Select the capabilities set required for your application

 Texas Instruments built 3 different predefined sets of capabilities that would fit most of 
 the target applications.\n
 It is recommended to try and choose one of this predefined capabilities set before going to
 build your own customized set. If you find compatible set you can skip the rest of this step.

 The available sets are:
     -# SL_TINY     -   Compatible to be used on platforms with very limited resources. Provides
                        the best in class low foot print in terms of Code and Data consumption.
     -# SL_SMALL    -   Compatible to most common networking applications. Provide the most
                        common APIs with decent balance between code size, data size, functionality
                        and performances
     -# SL_FULL     -   Provide access to all SimpleLink functionalities


 \subsection    porting_step3   Step 3 - Bind the device enable/disable output line

 The CC3120 has two external hardware lines that can be used to enable/disable the device.
 - <b>nReset</b>
 - <b>nHib</b> - provides mechanism to enter the device into the least current consumption mode. In 
 this mode the RTC value is kept.
 
 The driver manipulates the enable/disable line automatically during sl_Start / sl_Stop.\n
 Not connecting one these lines means that the driver could start only once (sl_Stop will not 
 work correctly and might lead to failure latter on) and the internal provisioning mechanism 
 could not be used.\n

 To bind these lines the following defines should be defined correctly:
 - <b>sl_DeviceEnable</b>
 - <b>sl_DeviceDisable</b>
 
 If some initializations required before the enable/disable macros are called the user can use also the following <i>optional</i> define
 - <b>sl_DeviceEnablePreamble</b>

 \subsection    porting_step4   Step 4 - Writing your interface communication driver

 The SimpleLink CC3120 has two standard communication interfaces
	- SPI
	- UART

 The device detects automatically the active interface during initialization. After the detection, the second interface could not be used.\n
 
 To wrap the driver for the communication channel the following functions should be implemented:
 -# sl_IfOpen
 -# sl_IfClose
 -# sl_IfRead
 -# sl_IfWrite
 -# sl_IfRegIntHdlr

 The way these functions are implemented has direct effect on the performances of the SimpleLink 
 device on this target platform. DMA and Jitter Buffer should be considered.\n
 
 In some platforms the user need to mask the IRQ line when this interrupt could be masked. \n
 The driver can call the mask/unmask whenever is needed. To allow this functionality the 
 user should implement also the following defines:
	- sl_IfMaskIntHdlr
	- sl_IfUnMaskIntHdlr

 By default the driver is writing the command in few transactions to allow zero-copy mechanism. \n
 To enable a Jitter buffer for improving the communication line utilization, the can implement 
 also the following defines:	
	- sl_IfStartWriteSequence
	- sl_IfEndWriteSequence

 \subsection     porting_step5   Step 5 - Choose your memory management model

 The SimpleLink driver support two memory models:
     - Static (default)
     - Dynamic

 To enable the dynamic memory, the following pre-processor define should be set: \n
 #define SL_MEMORY_MGMT_DYNAMIC

 And the following macros should be defined and supplied:
	- sl_Malloc
	- sl_Free

	Using the dynamic mode will allocate the required resources on sl_Start and release these resource on sl_Stop.

 \subsection     porting_step6   Step 6 - OS adaptation

 The SimpleLink driver could run on two kind of platforms:
     -# Non-Os / Single Threaded (default)
     -# Multi-Threaded

 When building a multi-threaded application. the following pre-processor define must be set: \n
 #define SL_PLATFORM_MULTI_THREADED

 If you choose to work in multi-threaded environment under operating system you will have to
 provide some basic adaptation routines to allow the driver to protect access to resources
 for different threads (locking object) and to allow synchronization between threads (sync objects).
 In additional the driver support running without dedicated thread allocated solely to the
 SimpleLink driver. If you choose to work in this mode, you should also supply a spawn method that
 will enable to run function on a temporary context.


 \subsection     porting_step7   Step 7 - Set your asynchronous event handlers routines

 The SimpleLink device generate asynchronous events in several situations.
 These asynchronous events could be masked.
 In order to catch these events you have to provide handler routines.
 Please notice that if you not provide a handler routine and the event is received,
 the driver will drop this event without any indication of this drop.


 \subsection     porting_step8   Step 8 - Run diagnostic tools to validate the correctness of your porting

 The driver is delivered with some porting diagnostic tools to simplify the porting validation process
 and to reduce issues latter. It is very important to follow carefully this process.

 The diagnostic process include:
     -# Validating interface communication driver
     -# Validating basic work with the device


	\section annex_step Annex Persistency
			The SimpleLink(tm) device support few different persistency types for settings and configurations:\n
			- <b>Temporary</b>			-	Effective immediately but returned to default after reset\n
			- <b>System Persistent</b> 	-	Effective immediately and kept after reset according\n
										-	to system persistent mode\n
			- <b>Persistent</b> 		-	Effective immediately and kept after reset regardless the system persistent mode\n
			- <b>Optionally Persistent</b>-	Effective immediately and kept after reset according to a parameter in the API call\n
			- <b>Reset</b> 				-	Persistent but effective only after reset\n
	 
    \section sw_license License

 *
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
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



#ifndef __SIMPLELINK_H__
#define __SIMPLELINK_H__

/* define the default types
 * If user wants to overwrite it,
 * he need to undef and define again */
#define _u8  unsigned char
#define _i8  signed char
#define _u16 unsigned short
#define _i16 signed short
#define _u32 unsigned long
#define _i32 signed long

#define _volatile volatile
#define _const    const

#include <ti/drivers/net/wifi/porting/user.h>

#ifdef    __cplusplus
extern "C"
{
#endif

/*!
	\defgroup UserEvents 
	\short Function prototypes for event callback handlers

*/

/*! \attention  Async event activation notes\n
    Function prototypes for event callback handlers\n
    Event handler function names should be defined in the user.h file\n
    e.g.\n
    "#define slcb_WlanEvtHdlr   SLWlanEventHandler"\n
    Indicates all WLAN events are handled by User func "SLWlanEventHandler"\n
    Important notes:\n
    1. Event handlers cannot activate another SimpleLink API from the event's context
    2. Event's data is valid during event's context. Any application data
       which is required for the user application should be copied or marked
       into user's variables
    3. It is not recommended to delay the execution of the event callback handler

*/

/*!

    \addtogroup UserEvents
    @{

*/


/*****************************************************************************/
/* Macro declarations for Host Driver version                                */
/*****************************************************************************/
#define SL_DRIVER_VERSION   "2.0.1.22"
#define SL_MAJOR_VERSION_NUM    2L
#define SL_MINOR_VERSION_NUM    0L
#define SL_VERSION_NUM          1L
#define SL_SUB_VERSION_NUM      22L


/*****************************************************************************/
/* Macro declarations for predefined configurations                          */
/*****************************************************************************/

#ifdef SL_TINY

#undef SL_INC_ARG_CHECK
#undef SL_INC_EXT_API
#undef SL_INC_SOCK_SERVER_SIDE_API
#undef SL_INC_WLAN_PKG
#undef SL_INC_NET_CFG_PKG
#undef SL_INC_FS_PKG
#undef SL_INC_SET_UART_MODE
#undef SL_INC_NVMEM_PKG
#define SL_INC_SOCK_CLIENT_SIDE_API
#define SL_INC_SOCK_RECV_API
#define SL_INC_SOCK_SEND_API
#define SL_INC_SOCKET_PKG
#define SL_INC_NET_APP_PKG

#endif

#ifdef SL_SMALL
#undef SL_INC_EXT_API
#undef SL_INC_NET_APP_PKG
#undef SL_INC_NET_CFG_PKG
#undef SL_INC_FS_PKG
#define SL_INC_ARG_CHECK
#define SL_INC_WLAN_PKG
#define SL_INC_SOCKET_PKG
#define SL_INC_SOCK_CLIENT_SIDE_API
#define SL_INC_SOCK_SERVER_SIDE_API
#define SL_INC_SOCK_RECV_API
#define SL_INC_SOCK_SEND_API
#define SL_INC_SET_UART_MODE
#endif

#ifdef SL_FULL
#define SL_INC_EXT_API
#define SL_INC_NET_APP_PKG
#define SL_INC_NET_CFG_PKG
#define SL_INC_FS_PKG
#define SL_INC_ARG_CHECK
#define SL_INC_WLAN_PKG
#define SL_INC_SOCKET_PKG
#define SL_INC_SOCK_CLIENT_SIDE_API
#define SL_INC_SOCK_SERVER_SIDE_API
#define SL_INC_SOCK_RECV_API
#define SL_INC_SOCK_SEND_API
#define SL_INC_SET_UART_MODE
#endif



/* #define sl_Memcpy       memcpy */
#define sl_Memset(addr, val, len)      memset(addr, val, (size_t)len)
#define sl_Memcpy(dest, src, len)      memcpy(dest, src, (size_t)len)
#define sl_Memmove(dest, src, len)     memmove(dest, src, (size_t)len)

  
#ifndef SL_TINY
#define SL_MAX_SOCKETS      (_u8)(16)
#else
#define SL_MAX_SOCKETS      (_u8)(2)
#endif


/*****************************************************************************/
/* Types definitions                                                                                                                */
/*****************************************************************************/

#ifndef NULL
#define NULL        (0)
#endif

#ifndef FALSE
#define FALSE       (0)
#endif

#ifndef TRUE
#define TRUE        (!FALSE)
#endif

typedef _u16  _SlOpcode_t;
typedef _u8   _SlArgSize_t;
typedef _i16   _SlDataSize_t;
typedef _i16   _SlReturnVal_t;

/*
 * This event status used to  block or continue the event propagation
 * through all the registered external libs/user application
 *
 */

 typedef enum {
   EVENT_PROPAGATION_BLOCK = 0,
   EVENT_PROPAGATION_CONTINUE

 } _SlEventPropogationStatus_e;



/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/


/*
   objInclusion.h and user.h must be included before all api header files
   objInclusion.h must be the last arrangement just before including the API header files
   since it based on the other configurations to decide which object should be included
*/
#include "source/objInclusion.h"
#include "trace.h"
#include "fs.h"
#include "sl_socket.h"
#include "netapp.h"
#include "wlan.h"
#include "device.h"
#include "netcfg.h"
#include "netutil.h"
#include "errors.h"
#include "eventreg.h"

/*!
	\cond DOXYGEN_IGNORE
*/
 /* In case of use dynamic event registration
  * redirect the event to the internal mechanism */
#if (defined(SL_RUNTIME_EVENT_REGISTERATION))

#define _SlDrvHandleFatalErrorEvents _SlDeviceFatalErrorEvtHdlr
#define _SlDrvHandleGeneralEvents  _SlDeviceGeneralEvtHdlr
#define _SlDrvHandleWlanEvents  _SlWlanEvtHdlr
#define _SlDrvHandleNetAppEvents  _SlNetAppEvtHdlr
#define _SlDrvHandleSockEvents  _SlSockEvtHdlr
#define _SlDrvHandleHttpServerEvents  _SlNetAppHttpServerHdlr
#define _SlDrvHandleNetAppRequestEvents  _SlNetAppRequestHdlr
#define _SlDrvHandleNetAppRequestMemFreeEvents  _SlNetAppRequestMemFree
#define _SlDrvHandleSocketTriggerEvents  _SlSocketTriggerEventHandler

#else

 /* The fatal error events dispatcher which is
  * initialized to the user handler */
#ifdef slcb_DeviceFatalErrorEvtHdlr
#define _SlDrvHandleFatalErrorEvents slcb_DeviceFatalErrorEvtHdlr
#endif

 /* The general events dispatcher which is
  * initialized to the user handler */
#ifdef slcb_DeviceGeneralEvtHdlr
#define _SlDrvHandleGeneralEvents slcb_DeviceGeneralEvtHdlr
#endif

 /* The wlan events dispatcher which is
  * initialized to the user handler */
#ifdef slcb_WlanEvtHdlr
#define _SlDrvHandleWlanEvents slcb_WlanEvtHdlr
#endif

 /* The NetApp events dispatcher which is
  * initialized to the user handler */
#ifdef slcb_NetAppEvtHdlr
#define _SlDrvHandleNetAppEvents slcb_NetAppEvtHdlr
#endif

 /* The http server events dispatcher which is
  * initialized to the user handler if exists */
#ifdef slcb_NetAppHttpServerHdlr
#define _SlDrvHandleHttpServerEvents slcb_NetAppHttpServerHdlr
#endif

 /* The socket events dispatcher which is
  * initialized to the user handler */
#ifdef slcb_SockEvtHdlr
#define _SlDrvHandleSockEvents slcb_SockEvtHdlr
#endif


/* The netapp requests dispatcher which is
  * initialized to the user handler if exists */
#ifdef slcb_NetAppRequestHdlr
#define _SlDrvHandleNetAppRequestEvents slcb_NetAppRequestHdlr
#endif

/* The netapp request mem free requests dispatcher which is
* initialized to the user handler if exists */
#ifdef slcb_NetAppRequestMemFree
#define _SlDrvHandleNetAppRequestMemFreeEvents slcb_NetAppRequestMemFree
#endif

/* The netapp requests dispatcher which is
* initialized to the user handler if exists */
#ifdef slcb_SocketTriggerEventHandler
#define _SlDrvHandleSocketTriggerEvents slcb_SocketTriggerEventHandler
#endif


#endif

#define SL_CONCAT(x,y)	x ## y
#define SL_CONCAT2(x,y)	SL_CONCAT(x,y)


#if (!defined(SL_RUNTIME_EVENT_REGISTERATION))

/*
 * The section below handles the external lib event registration
 * according to the desired events it specified in its API header file.
 * The external lib should be first installed by the user (see user.h)
 */
#ifdef SL_EXT_LIB_1

    /* General Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_1, _NOTIFY_GENERAL_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_1, _GeneralEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib1GeneralEventHandler   SL_CONCAT2(SL_EXT_LIB_1, _GeneralEventHdl)

  #undef EXT_LIB_REGISTERED_GENERAL_EVENTS
    #define EXT_LIB_REGISTERED_GENERAL_EVENTS
  #endif

  /* Wlan Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_1, _NOTIFY_WLAN_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_1, _WlanEventHdl) (SlWlanEvent_t *);
  #define SlExtLib1WlanEventHandler   SL_CONCAT2(SL_EXT_LIB_1, _WlanEventHdl)

  #undef EXT_LIB_REGISTERED_WLAN_EVENTS
    #define EXT_LIB_REGISTERED_WLAN_EVENTS
  #endif

  /* NetApp Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_1, _NOTIFY_NETAPP_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_1, _NetAppEventHdl) (SlNetAppEvent_t *);
  #define SlExtLib1NetAppEventHandler SL_CONCAT2(SL_EXT_LIB_1, _NetAppEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_EVENTS
  #endif

  /* Http Server Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_1, _NOTIFY_HTTP_SERVER_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_1, _HttpServerEventHdl) (SlNetAppHttpServerEvent_t* , SlNetAppHttpServerResponse_t*);
  #define SlExtLib1HttpServerEventHandler SL_CONCAT2(SL_EXT_LIB_1, _HttpServerEventHdl)

  #undef EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
    #define EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
  #endif

  /* Socket Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_1, _NOTIFY_SOCK_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_1, _SockEventHdl) (SlSockEvent_t *);
  #define SlExtLib1SockEventHandler SL_CONCAT2(SL_EXT_LIB_1, _SockEventHdl)

  #undef EXT_LIB_REGISTERED_SOCK_EVENTS
    #define EXT_LIB_REGISTERED_SOCK_EVENTS
  #endif


  /* Fatal Error Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_1, _NOTIFY_FATAL_ERROR_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_1, _FatalErrorEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib1FatalErrorEventHandler   SL_CONCAT2(SL_EXT_LIB_1, _FatalErrorEventHdl)

  #undef EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
    #define EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
  #endif


  /* NetApp requests events registration */
  #if SL_CONCAT2(SL_EXT_LIB_1, _NOTIFY_NETAPP_REQUEST_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_1, _NetAppRequestEventHdl) (SlNetAppRequest_t*, SlNetAppResponse_t *);
  #define SlExtLib1NetAppRequestEventHandler   SL_CONCAT2(SL_EXT_LIB_1, _NetAppRequestEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
  #endif


#endif


#ifdef SL_EXT_LIB_2

    /* General Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_2, _NOTIFY_GENERAL_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_2, _GeneralEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib2GeneralEventHandler   SL_CONCAT2(SL_EXT_LIB_2, _GeneralEventHdl)

  #undef EXT_LIB_REGISTERED_GENERAL_EVENTS
    #define EXT_LIB_REGISTERED_GENERAL_EVENTS
  #endif

  /* Wlan Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_2, _NOTIFY_WLAN_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_2, _WlanEventHdl) (SlWlanEvent_t *);
  #define SlExtLib2WlanEventHandler   SL_CONCAT2(SL_EXT_LIB_2, _WlanEventHdl)

  #undef EXT_LIB_REGISTERED_WLAN_EVENTS
    #define EXT_LIB_REGISTERED_WLAN_EVENTS
  #endif

  /* NetApp Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_2, _NOTIFY_NETAPP_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_2, _NetAppEventHdl) (SlNetAppEvent_t *);
  #define SlExtLib2NetAppEventHandler SL_CONCAT2(SL_EXT_LIB_2, _NetAppEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_EVENTS
  #endif

  /* Http Server Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_2, _NOTIFY_HTTP_SERVER_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_2, _HttpServerEventHdl) (SlNetAppHttpServerEvent_t* , SlNetAppHttpServerResponse_t*);
  #define SlExtLib2HttpServerEventHandler SL_CONCAT2(SL_EXT_LIB_2, _HttpServerEventHdl)

  #undef EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
    #define EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
  #endif

  /* Socket Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_2, _NOTIFY_SOCK_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_2, _SockEventHdl) (SlSockEvent_t *);
  #define SlExtLib2SockEventHandler SL_CONCAT2(SL_EXT_LIB_2, _SockEventHdl)

  #undef EXT_LIB_REGISTERED_SOCK_EVENTS
    #define EXT_LIB_REGISTERED_SOCK_EVENTS
  #endif

   /* Fatal Error Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_2, _NOTIFY_FATAL_ERROR_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_2, _FatalErrorEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib2FatalErrorEventHandler   SL_CONCAT2(SL_EXT_LIB_2, _FatalErrorEventHdl)

  #undef EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
    #define EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
  #endif


    /* NetApp requests events registration */
  #if SL_CONCAT2(SL_EXT_LIB_2, _NOTIFY_NETAPP_REQUEST_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_2, _NetAppRequestEventHdl) (SlNetAppRequest_t*, SlNetAppResponse_t *);
  #define SlExtLib1NetAppRequestEventHandler   SL_CONCAT2(SL_EXT_LIB_2, _NetAppRequestEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
  #endif


#endif


#ifdef SL_EXT_LIB_3

    /* General Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_3, _NOTIFY_GENERAL_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_3, _GeneralEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib3GeneralEventHandler   SL_CONCAT2(SL_EXT_LIB_3, _GeneralEventHdl)

  #undef EXT_LIB_REGISTERED_GENERAL_EVENTS
    #define EXT_LIB_REGISTERED_GENERAL_EVENTS
  #endif

  /* Wlan Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_3, _NOTIFY_WLAN_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_3, _WlanEventHdl) (SlWlanEvent_t *);
  #define SlExtLib3WlanEventHandler   SL_CONCAT2(SL_EXT_LIB_3, _WlanEventHdl)

  #undef EXT_LIB_REGISTERED_WLAN_EVENTS
    #define EXT_LIB_REGISTERED_WLAN_EVENTS
  #endif

  /* NetApp Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_3, _NOTIFY_NETAPP_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_3, _NetAppEventHdl) (SlNetAppEvent_t *);
  #define SlExtLib3NetAppEventHandler SL_CONCAT2(SL_EXT_LIB_3, _NetAppEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_EVENTS
  #endif

  /* Http Server Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_3, _NOTIFY_HTTP_SERVER_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_3, _HttpServerEventHdl) (SlNetAppHttpServerEvent_t* , SlNetAppHttpServerResponse_t*);
  #define SlExtLib3HttpServerEventHandler SL_CONCAT2(SL_EXT_LIB_3, _HttpServerEventHdl)

  #undef EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
    #define EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
  #endif

  /* Socket Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_3, _NOTIFY_SOCK_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_3, _SockEventHdl) (SlSockEvent_t *);
  #define SlExtLib3SockEventHandler SL_CONCAT2(SL_EXT_LIB_3, _SockEventHdl)

  #undef EXT_LIB_REGISTERED_SOCK_EVENTS
    #define EXT_LIB_REGISTERED_SOCK_EVENTS
  #endif


  /* Fatal Error Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_3, _NOTIFY_FATAL_ERROR_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_3, _FatalErrorEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib3FatalErrorEventHandler   SL_CONCAT2(SL_EXT_LIB_3, _FatalErrorEventHdl)

  #undef EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
    #define EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
  #endif

  /* NetApp requests events registration */
  #if SL_CONCAT2(SL_EXT_LIB_3, _NOTIFY_NETAPP_REQUEST_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_3, _NetAppRequestEventHdl) (SlNetAppRequest_t*, SlNetAppResponse_t *);
  #define SlExtLib1NetAppRequestEventHandler   SL_CONCAT2(SL_EXT_LIB_3, _NetAppRequestEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
  #endif


#endif


#ifdef SL_EXT_LIB_4

    /* General Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_4, _NOTIFY_GENERAL_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_4, _GeneralEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib4GeneralEventHandler   SL_CONCAT2(SL_EXT_LIB_4, _GeneralEventHdl)

  #undef EXT_LIB_REGISTERED_GENERAL_EVENTS
    #define EXT_LIB_REGISTERED_GENERAL_EVENTS
  #endif

  /* Wlan Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_4, _NOTIFY_WLAN_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_4, _WlanEventHdl) (SlWlanEvent_t *);
  #define SlExtLib4WlanEventHandler   SL_CONCAT2(SL_EXT_LIB_4, _WlanEventHdl)

  #undef EXT_LIB_REGISTERED_WLAN_EVENTS
    #define EXT_LIB_REGISTERED_WLAN_EVENTS
  #endif

  /* NetApp Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_4, _NOTIFY_NETAPP_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_4, _NetAppEventHdl) (SlNetAppEvent_t *);
  #define SlExtLib4NetAppEventHandler SL_CONCAT2(SL_EXT_LIB_4, _NetAppEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_EVENTS
  #endif

  /* Http Server Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_4, _NOTIFY_HTTP_SERVER_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_4, _HttpServerEventHdl) (SlNetAppHttpServerEvent_t* , SlNetAppHttpServerResponse_t*);
  #define SlExtLib4HttpServerEventHandler SL_CONCAT2(SL_EXT_LIB_4, _HttpServerEventHdl)

  #undef EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
    #define EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
  #endif

  /* Socket Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_4, _NOTIFY_SOCK_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_4, _SockEventHdl) (SlSockEvent_t *);
  #define SlExtLib4SockEventHandler SL_CONCAT2(SL_EXT_LIB_4, _SockEventHdl)

  #undef EXT_LIB_REGISTERED_SOCK_EVENTS
    #define EXT_LIB_REGISTERED_SOCK_EVENTS
  #endif


  /* Fatal Error Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_4, _NOTIFY_FATAL_ERROR_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_4, _FatalErrorEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib4FatalErrorEventHandler   SL_CONCAT2(SL_EXT_LIB_4, _FatalErrorEventHdl)

  #undef EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
    #define EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
  #endif


  /* NetApp requests events registration */
  #if SL_CONCAT2(SL_EXT_LIB_4, _NOTIFY_NETAPP_REQUEST_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_4, _NetAppRequestEventHdl) (SlNetAppRequest_t*, SlNetAppResponse_t *);
  #define SlExtLib1NetAppRequestEventHandler   SL_CONCAT2(SL_EXT_LIB_4, _NetAppRequestEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
  #endif



#endif


#ifdef SL_EXT_LIB_5

    /* General Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_5, _NOTIFY_GENERAL_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_5, _GeneralEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib5GeneralEventHandler   SL_CONCAT2(SL_EXT_LIB_5, _GeneralEventHdl)

  #undef EXT_LIB_REGISTERED_GENERAL_EVENTS
    #define EXT_LIB_REGISTERED_GENERAL_EVENTS
  #endif

  /* Wlan Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_5, _NOTIFY_WLAN_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_5, _WlanEventHdl) (SlWlanEvent_t *);
  #define SlExtLib5WlanEventHandler   SL_CONCAT2(SL_EXT_LIB_5, _WlanEventHdl)

  #undef EXT_LIB_REGISTERED_WLAN_EVENTS
    #define EXT_LIB_REGISTERED_WLAN_EVENTS
  #endif

  /* NetApp Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_5, _NOTIFY_NETAPP_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_5, _NetAppEventHdl) (SlNetAppEvent_t *);
  #define SlExtLib5NetAppEventHandler SL_CONCAT2(SL_EXT_LIB_5, _NetAppEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_EVENTS
  #endif

  /* Http Server Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_5, _NOTIFY_HTTP_SERVER_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_5, _HttpServerEventHdl) (SlNetAppHttpServerEvent_t* , SlNetAppHttpServerResponse_t*);
  #define SlExtLib5HttpServerEventHandler SL_CONCAT2(SL_EXT_LIB_5, _HttpServerEventHdl)

  #undef EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
    #define EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS
  #endif

  /* Socket Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_5, _NOTIFY_SOCK_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_5, _SockEventHdl) (SlSockEvent_t *);
  #define SlExtLib5SockEventHandler SL_CONCAT2(SL_EXT_LIB_5, _SockEventHdl)

  #undef EXT_LIB_REGISTERED_SOCK_EVENTS
    #define EXT_LIB_REGISTERED_SOCK_EVENTS
  #endif


  /* Fatal Error Event Registration */
  #if SL_CONCAT2(SL_EXT_LIB_5, _NOTIFY_FATAL_ERROR_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_5, _FatalErrorEventHdl) (SlDeviceEvent_t *);
  #define SlExtLib5FatalErrorEventHandler   SL_CONCAT2(SL_EXT_LIB_5, _FatalErrorEventHdl)

  #undef EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
    #define EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS
  #endif

  
  /* NetApp requests events registration */
  #if SL_CONCAT2(SL_EXT_LIB_5, _NOTIFY_NETAPP_REQUEST_EVENT)
  extern _SlEventPropogationStatus_e SL_CONCAT2(SL_EXT_LIB_5, _NetAppRequestEventHdl) (SlNetAppRequest_t*, SlNetAppResponse_t *);
  #define SlExtLib1NetAppRequestEventHandler   SL_CONCAT2(SL_EXT_LIB_5, _NetAppRequestEventHdl)

  #undef EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
    #define EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS
  #endif

#endif

#if defined(EXT_LIB_REGISTERED_FATAL_ERROR_EVENTS)
extern void _SlDrvHandleFatalErrorEvents(SlDeviceEvent_t *slFatalErrorEvent);
#endif


#if defined(EXT_LIB_REGISTERED_GENERAL_EVENTS)
extern void _SlDrvHandleGeneralEvents(SlDeviceEvent_t *slGeneralEvent);
#endif

#if defined(EXT_LIB_REGISTERED_WLAN_EVENTS)
extern void _SlDrvHandleWlanEvents(SlWlanEvent_t *slWlanEvent);
#endif

#if defined (EXT_LIB_REGISTERED_NETAPP_EVENTS)
extern void _SlDrvHandleNetAppEvents(SlNetAppEvent_t *slNetAppEvent);
#endif

#if defined(EXT_LIB_REGISTERED_HTTP_SERVER_EVENTS)
extern void _SlDrvHandleHttpServerEvents(SlNetAppHttpServerEvent_t *slHttpServerEvent, SlNetAppHttpServerResponse_t *slHttpServerResponse);
#endif


#if defined(EXT_LIB_REGISTERED_SOCK_EVENTS)
extern void _SlDrvHandleSockEvents(SlSockEvent_t *slSockEvent);
#endif


#if defined(EXT_LIB_REGISTERED_NETAPP_REQUEST_EVENTS)
extern void _SlDrvHandleNetAppRequestEvents(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse);
#endif

#endif //#if (defined(SL_RUNTIME_EVENT_REGISTERATION))


typedef _SlReturnVal_t (*_SlSpawnEntryFunc_t)(void* pValue);

#define SL_SPAWN_FLAG_FROM_SL_IRQ_HANDLER    (0X1)

#ifdef SL_PLATFORM_MULTI_THREADED
    #include "source/spawn.h"
#else
    #include "source/nonos.h"
#endif

/*!
	\endcond
*/


/* Async functions description*/


/*!
    \brief Fatal Error event for inspecting fatal error

    \param[out]      pSlFatalErrorEvent   pointer to SlDeviceFatal_t
	\return None
	\sa
	\note
	\warning
    \par	Example
	\code
		For pSlDeviceFatal->Id = SL_DEVICE_EVENT_FATAL_DEVICE_ABORT
				Indicates a severe error occured and the device stopped
		Use pSlDeviceFatal->Data.DeviceAssert fields
					- Code: An idication of the abort type
					- Value: The abort data


		For pSlDeviceFatal->Id = SL_DEVICE_EVENT_FATAL_NO_CMD_ACK
				Indicates that the command sent to the device had no ack
		Use pSlDeviceFatal->Data.NoCmdAck fields
					- Code: An idication of the cmd opcode
	 
		For pSlDeviceFatal->Id = SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT
				Indicates that the command got a timeout while waiting for its async response
		Use pSlDeviceFatal->Data.CmdTimeout fields
					- Code: An idication of the asyncevent opcode


		For pSlDeviceFatal->Id = SL_DEVICE_EVENT_FATAL_DRIVER_ABORT
				Indicates a severe error occured in the driver
		Use pSlDeviceFatal->Data.DeviceAssert fields
					- None.

		For pSlDeviceFatal->Id = SL_DEVICE_EVENT_FATAL_SYNC_LOSS
				Indicates a sync loss with the device 
		Use pSlDeviceFatal->Data.DeviceAssert fields
					- None.
	\endcode
    \code
		Example for fatal error
			 printf(Abort type =%d Abort Data=0x%x\n\n",
				   pSlDeviceFatal->Data.deviceReport.AbortType,
				   pSlDeviceFatal->Data.deviceReport.AbortData);
    \endcode
*/
#if (defined(slcb_DeviceFatalErrorEvtHdlr))
extern void slcb_DeviceFatalErrorEvtHdlr(SlDeviceFatal_t *pSlFatalErrorEvent);
#endif


/*!
    \brief General async event for inspecting general events

    \param[out]      pSlDeviceEvent   pointer to SlDeviceEvent_t
	\return None
	\sa
	\note
	\warning
    \par	Example
	\code
          For pSlDeviceEvent->Id = SL_DEVICE_EVENT_RESET_REQUEST
          Use pSlDeviceEvent->Data.ResetRequest fields
                  - Status: An error code indication from the device
                  - Source: The sender originator which is based on SlDeviceSource_e enum
          
		  For pSlDeviceEvent->Id = SL_DEVICE_EVENT_ERROR
          Use pSlDeviceEvent->Data.Error fields
                  - Code: An error code indication from the device
                  - Source: The sender originator which is based on SlErrorSender_e enum
	\endcode
    \code
		Example for error event:
			printf(General Event Handler - ID=%d Sender=%d\n\n",
				   pSlDeviceEvent->Data.Error.Code,  // the error code
				   pSlDeviceEvent->Data.Error.Source); // the error source
    \endcode
   
*/
#if (defined(slcb_DeviceGeneralEvtHdlr))
extern void slcb_DeviceGeneralEvtHdlr(SlDeviceEvent_t *pSlDeviceEvent);
#endif

/*!
    \brief WLAN Async event handler

    \param[out]      pSlWlanEvent   pointer to SlWlanEvent_t data
	\return None
	\sa
	\note
	\warning
    \par	Example
	\code
             For pSlWlanEvent->Id = SL_WLAN_EVENT_CONNECT, STA connection indication event
             Use pSlWlanEvent->Data.Connect main fields
                     - SsidLen
                     - SsidName
                     - Bssid

             For pSlWlanEvent->Id = SL_WLAN_EVENT_P2P_CONNECT, P2P client connection indication event
             Use pSlWlanEvent->Data.P2PConnect main fields
                     - SsidLen
                     - SsidName
                     - Bssid
                     - GoDeviceNameLen
                     - GoDeviceName

             For pSlWlanEvent->Id = SL_WLAN_EVENT_DISCONNECT, STA client disconnection event
             Use pSlWlanEvent->Data.Disconnect main fields:
                     - SsidLen
                     - SsidName
                     - Bssid
                     - ReasonCode

             For pSlWlanEvent->Id = SL_WLAN_EVENT_P2P_DISCONNECT, P2P client disconnection event
             Use pSlWlanEvent->Data.P2PDisconnect main fields:
                     - SsidLen
                     - SsidName
                     - Bssid
                     - ReasonCode
                     - GoDeviceNameLen
                     - GoDeviceName

             For pSlWlanEvent->Id = SL_WLAN_EVENT_STA_ADDED, AP connected STA
             Use pSlWlanEvent->Data.STAAdded fields:
                     - Mac

             For pSlWlanEvent->Id = SL_WLAN_EVENT_STA_REMOVED, AP disconnected STA
             Use pSlWlanEvent->Data.STARemoved fields:
                     - Mac

			 For pSlWlanEvent->Id = SL_WLAN_EVENT_P2P_CLIENT_ADDED, P2P(Go) connected P2P(Client)
             Use pSlWlanEvent->Data.P2PClientAdded fields:
                     - Mac
                     - GoDeviceNameLen
                     - GoDeviceName
                     - OwnSsidLen
                     - OwnSsid

             For pSlWlanEvent->Id = SL_WLAN_EVENT_P2P_CLIENT_REMOVED, P2P(Go) disconnected P2P(Client)
             Use pSlWlanEvent->Data.P2PClientRemoved fields:
                     - Mac
                     - GoDeviceNameLen
                     - GoDeviceName
                     - OwnSsidLen
                     - OwnSsid

             For pSlWlanEvent->Id = SL_WLAN_P2P_DEV_FOUND_EVENT
             Use pSlWlanEvent->Data.P2PDevFound fields:
                    - GoDeviceNameLen
                    - GoDeviceName
                    - Mac
                    - WpsMethod

             For pSlWlanEvent->Id = SL_WLAN_EVENT_P2P_REQUEST
             Use pSlWlanEvent->Data.P2PRequest fields
                    - GoDeviceNameLen
                    - GoDeviceName
                    - Mac
                    - WpsMethod

             For pSlWlanEvent->Id = SL_WLAN_EVENT_P2P_CONNECTFAIL, P2P only
			 Use pSlWlanEvent->Data.P2PConnectFail fields:
						- Status

             For pSlWlanEvent->Id = SL_WLAN_EVENT_PROVISIONING_STATUS
             Use pSlWlanEvent->Data.ProvisioningStatus fields
                      - Status

             For pSlWlanEvent->Id = SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED
             Use pSlWlanEvent->Data.ProvisioningProfileAdded fields:
                     - Status
                     - SsidLen
                     - Ssid
                     - Reserved
	\endcode
*/
#if (defined(slcb_WlanEvtHdlr))
extern void slcb_WlanEvtHdlr(SlWlanEvent_t* pSlWlanEvent);
#endif


/*!
    \brief NETAPP Async event handler

    \param[out]      pSlNetAppEvent   pointer to SlNetAppEvent_t data
	\return None
	\sa
	\note
	\warning
    \par	Example
	\code
              For pSlNetAppEvent->Id = SL_NETAPP_EVENT_IPV4_ACQUIRED/SL_NETAPP_EVENT_IPV6_ACQUIRED
              Use pSlNetAppEvent->Data.ipAcquiredV4 (V6) fields
                       - ip
                       - gateway
                       - dns

              For pSlNetAppEvent->Id = SL_NETAPP_IP_LEASED_EVENT, AP or P2P go dhcp lease event
              Use pSlNetAppEvent->Data.ipLeased  fields
                       - ip_address
                       - lease_time
                       - mac

              For pSlNetApp->Id = SL_NETAPP_IP_RELEASED_EVENT, AP or P2P go dhcp ip release event
              Use pSlNetAppEvent->Data.ipReleased fields
                       - ip_address
                       - mac
                       - reason
	\endcode
	
*/
#if (defined(slcb_NetAppEvtHdlr))
extern void slcb_NetAppEvtHdlr(SlNetAppEvent_t* pSlNetAppEvent);
#endif

/*!
    \brief Socket Async event handler

    \param[out]      pSlSockEvent   pointer to SlSockEvent_t data
	\return None
	\sa
	\note
	\warning
    \par	Example
	\code
             For pSlSockEvent->Event = SL_SOCKET_TX_FAILED_EVENT
             Use pSlSockEvent->SockTxFailData fields
                     - sd
                     - status
             For pSlSockEvent->Event = SL_SOCKET_ASYNC_EVENT
             Use pSlSockEvent->SockAsyncData fields
                     - sd
                     - type 
						- SL_SSL_ACCEPT  
						- SL_WLAN_RX_FRAGMENTATION_TOO_BIG 
						- SL_OTHER_SIDE_CLOSE_SSL_DATA_NOT_ENCRYPTED
                     - val
	\endcode

*/
#if (defined(slcb_SockEvtHdlr))
extern void slcb_SockEvtHdlr(SlSockEvent_t* pSlSockEvent);
#endif

/*!
    \brief HTTP server async event

    \param[out] pSlHttpServerEvent   Pointer to SlNetAppHttpServerEvent_t
    \param[in] pSlHttpServerResponse Pointer to SlNetAppHttpServerResponse_t

	\return None
	\sa		slcb_NetAppRequestHdlr
    \note
	\warning
	\par	Example
	\code
          For pSlHttpServerResponse->Event = SL_NETAPP_HTTPGETTOKENVALUE_EVENT
		  Use pSlHttpServerEvent->EventData fields
                 - httpTokenName
                 - data
                 - len
          And pSlHttpServerResponse->ResponseData fields
                 - data
                 - len

          For pSlHttpServerEvent->Event = SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT
          Use pSlHttpServerEvent->EventData.httpPostData fields
                 - action
                 - token_name
                 - token_value
          And pSlHttpServerResponse->ResponseData fields:
                 - data
                 - len
	\endcode
*/
#if (defined(slcb_NetAppHttpServerHdlr))
extern void slcb_NetAppHttpServerHdlr(SlNetAppHttpServerEvent_t *pSlHttpServerEvent, SlNetAppHttpServerResponse_t *pSlHttpServerResponse);
#endif

/*!
    \brief General netapp async event

    \param[out] pNetAppRequest   Pointer to SlNetAppRequest_t
    \param[in] pNetAppResponse Pointer to SlNetAppResponse_t

	\return None
	\sa		slcb_NetAppHttpServerHdlr
    \note
	\warning
	\par	Example
	\code
		TBD
	\endcode
*/
#if (defined(slcb_NetAppRequestHdlr))
extern void slcb_NetAppRequestHdlr(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse);
#endif

/*!
    \brief A handler for freeing the memory of the NetApp response. 

    \param[in,out] buffer		Pointer to the buffer to free

	\return None
	\sa		
    \note
	\warning
	\par	Example
	\code
		TBD
	\endcode
*/
#if (defined(slcb_NetAppRequestMemFree))
extern void slcb_NetAppRequestMemFree (_u8 *buffer);
#endif

/*!
    \brief 		Get the timer counter value (timestamp).\n
				The timer must count from zero to its MAX value.
				For non-os application, this routine must be implemented.  
	\par parameters	 	
		None
	\return		Returns 32-bit timer counter value (ticks unit) 
    \sa
	\note		 
    \note       belongs to \ref porting_sec
    \warning        
*/
#if defined (slcb_GetTimestamp)
extern _u32 slcb_GetTimestamp(void);
#endif


/*!
    \brief 		Socket trigger routine.
				This routine will notify the application that a netwrok activity has 
				been completed on the required socket/s.
	
    \param[out] pSlSockTriggerEvent   pointer to SlSockTriggerEvent_t data
	\return		None.
    \sa
	\note		 
    \note       belongs to \ref porting_sec
    \warning        
*/
#if (defined(slcb_SocketTriggerEventHandler))
extern void slcb_SocketTriggerEventHandler(SlSockTriggerEvent_t* pSlSockTriggerEvent);
#endif


/*!
 Close the Doxygen group.
 @}

 */

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif    /*  __SIMPLELINK_H__ */

