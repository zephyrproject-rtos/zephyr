/*
 * netcfg.h - CC31xx/CC32xx Host Driver Implementation
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

    
#ifndef __NETCFG_H__
#define __NETCFG_H__


#ifdef    __cplusplus
extern "C" {
#endif

/*!
	\defgroup NetCfg 
	\short Controls the configuration of the device addresses (i.e. IP and MAC addresses)

*/

/*!

    \addtogroup NetCfg
    @{

*/


/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

#define SL_MAC_ADDR_LEN                          (6)
#define SL_IPV6_ADDR_LEN                         (16)
#define SL_IPV4_VAL(add_3,add_2,add_1,add_0)     ((((_u32)add_3 << 24) & 0xFF000000) | (((_u32)add_2 << 16) & 0xFF0000) | (((_u32)add_1 << 8) & 0xFF00) | ((_u32)add_0 & 0xFF) )
#define SL_IPV4_BYTE(val,index)                  ( (val >> (index*8)) & 0xFF )


#define SL_NETCFG_IF_IPV6_STA_LOCAL                        (0x4)    /* disable ipv6 local */
#define SL_NETCFG_IF_IPV6_STA_GLOBAL					   (0x8)    /* disable ipv6 global */
#define SL_NETCFG_IF_DISABLE_IPV4_DHCP                     (0x40)   /* disable ipv4 dhcp */
#define SL_NETCFG_IF_IPV6_LOCAL_STATIC                     (0x80)   /* enable ipv6 local static */
#define SL_NETCFG_IF_IPV6_LOCAL_STATELESS                  (0x100)  /* enable ipv6 local stateless */
#define SL_NETCFG_IF_IPV6_LOCAL_STATEFUL               	   (0x200)  /* enable ipv6 local statefull */
#define SL_NETCFG_IF_IPV6_GLOBAL_STATIC                    (0x400)  /* enable ipv6 global static */
#define SL_NETCFG_IF_IPV6_GLOBAL_STATEFUL                  (0x800)  /* enable ipv6 global statefull */
#define SL_NETCFG_IF_DISABLE_IPV4_LLA                      (0x1000) /* disable LLA feature. Relevant only in IPV4 */
#define SL_NETCFG_IF_ENABLE_DHCP_RELEASE                   (0x2000) /* Enables DHCP release when WLAN disconnect command is issued */
#define SL_NETCFG_IF_IPV6_GLOBAL_STATELESS                 (0x4000)  /* enable ipv6 global stateless */
#define SL_NETCFG_IF_DISABLE_FAST_RENEW                    (0x8000)  /* fast renew disabled */


#define	SL_NETCFG_IF_STATE            (0)
#define SL_NETCFG_ADDR_DHCP           (1)
#define SL_NETCFG_ADDR_DHCP_LLA       (2)
#define SL_NETCFG_ADDR_RELEASE_IP     (3)
#define SL_NETCFG_ADDR_STATIC	      (4)
#define SL_NETCFG_ADDR_STATELESS      (5)
#define SL_NETCFG_ADDR_STATEFUL       (6)
#define SL_NETCFG_ADDR_RELEASE_IP_SET (7)
#define SL_NETCFG_ADDR_RELEASE_IP_OFF (8)
#define SL_NETCFG_ADDR_ENABLE_FAST_RENEW  (9)
#define SL_NETCFG_ADDR_DISABLE_FAST_RENEW (10)
#define SL_NETCFG_ADDR_FAST_RENEW_MODE_NO_WAIT_ACK   (11)
#define SL_NETCFG_ADDR_FAST_RENEW_MODE_WAIT_ACK      (12)


/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/
typedef enum
{
    SL_NETCFG_MAC_ADDRESS_SET                   = 1,
    SL_NETCFG_MAC_ADDRESS_GET                   = 2,
    SL_NETCFG_AP_STATIONS_NUM_CONNECTED         = 3,
    SL_NETCFG_AP_STATIONS_INFO_LIST             = 4,
    SL_NETCFG_AP_STATION_DISCONNECT             = 5,
	SL_NETCFG_IF		                        = 6,
	SL_NETCFG_IPV4_STA_ADDR_MODE			    = 7,
	SL_NETCFG_IPV4_AP_ADDR_MODE			        = 8,
	SL_NETCFG_IPV6_ADDR_LOCAL				    = 9,
	SL_NETCFG_IPV6_ADDR_GLOBAL				    = 10,
	SL_NETCFG_IPV4_DHCP_CLIENT                  = 11,
	SL_NETCFG_IPV4_DNS_CLIENT                   = 12,
    MAX_SETTINGS = 0xFF
}SlNetCfg_e;

typedef struct
{    
  _u32 DnsSecondServerAddr;
}SlNetCfgIpV4DnsClientArgs_t;


typedef struct
{    
    _u32  Ip;
    _u32  Gateway;
    _u32  Mask;
    _u32  Dns[2];
    _u32  DhcpServer;
    _u32  LeaseTime;
    _u32  TimeToRenew;
    _u8   DhcpState;
    _u8   Reserved[3];
} SlNetCfgIpv4DhcpClient_t;

typedef enum
{
  SL_NETCFG_DHCP_CLIENT_UNKNOWN = 0,
  SL_NETCFG_DHCP_CLIENT_DISABLED,
  SL_NETCFG_DHCP_CLIENT_ENABLED,
  SL_NETCFG_DHCP_CLIENT_BOUND,
  SL_NETCFG_DHCP_CLIENT_RENEW,
  SL_NETCFG_DHCP_CLIENT_REBIND
}SlNetCfgIpv4DhcpClientState_e;


typedef enum
{
    SL_NETCFG_DHCP_OPT_DISABLE_LLA = 0x2,                   /* 1=LLA disabled, 0=LLA enabled. */
    SL_NETCFG_DHCP_OPT_RELEASE_IP_BEFORE_DISCONNECT = 0x4,  /* 1=DHCP release enabled, 0=DHCP release disabled */
    MAX_SL_NETCFG_DHCP_OPT = 0xFF
} SlNetCfgDhcpOption_e;


typedef struct
{
    _u32  Ip;
    _u32  IpMask;
    _u32  IpGateway;
    _u32  IpDnsServer;
}SlNetCfgIpV4Args_t;


typedef struct
{
    _u32  Ip[4];
    _u32  IpDnsServer[4];
	_u32  IpV6Flags; /* bit 0: Indicate if the address is valid for use in the network (IPv6 DAD completed) . If not, try again later or set a different address. 1=Valid. Relevant for sl_NetCfgGet only. */
}SlNetCfgIpV6Args_t;

#define _SL_NETCFG_IPV6_ADDR_BIT_STATUS					0x01 
#define SL_IS_IPV6_ADDR_VALID(IpV6Flags)              (IpV6Flags & _SL_NETCFG_IPV6_ADDR_BIT_STATUS) 

#define NET_CFG_STA_INFO_STATUS_DHCP_ADDR   1

typedef struct
{
    _u32 Ip;
    _u8  MacAddr[6];
    _u16 Status;
    _u8  Name[32]; 
} SlNetCfgStaInfo_t;


/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/

/*!
    \brief     Setting network configurations

    \param[in] ConfigId   Configuration id: 
											- SL_NETCFG_IF
										    - SL_NETCFG_IPV4_STA_ADDR_MODE
											- SL_NETCFG_IPV6_ADDR_LOCAL
											- SL_NETCFG_IPV6_ADDR_GLOBAL
											- SL_NETCFG_IPV4_AP_ADDR_MODE
											- SL_NETCFG_MAC_ADDRESS_SET
											- SL_NETCFG_AP_STATION_DISCONNECT
    \param[in] ConfigOpt  Configurations option: 
												  - SL_NETCFG_IF_STATE            
												  - SL_NETCFG_ADDR_DHCP           
												  - SL_NETCFG_ADDR_DHCP_LLA       
												  - SL_NETCFG_ADDR_RELEASE_IP     
												  - SL_NETCFG_ADDR_STATIC	      
												  - SL_NETCFG_ADDR_STATELESS      
												  - SL_NETCFG_ADDR_STATEFUL       
												  - SL_NETCFG_ADDR_RELEASE_IP_SET 
												  - SL_NETCFG_ADDR_RELEASE_IP_OFF 
    \param[in] ConfigLen  Configurations len
    \param[in] pValues    Configurations values
    \par 				  Persistent 			    
	\par 
							<b>Reset</b>:                
													- SL_IPV4_AP_P2P_GO_STATIC_ENABLE
													- SL_NETCFG_MAC_ADDRESS_SET
	\par
							<b>Non- Persistent</b>:                 
													- SL_NETCFG_AP_STATION_DISCONNECT
	\par
							<b>System Persistent</b>: 
													- SL_NETCFG_IPV4_STA_ADDR_MODE
													- SL_NETCFG_IF_STATE
													- SL_NETCFG_IPV6_ADDR_LOCAL
													- SL_NETCFG_IPV6_ADDR_GLOBAL

    \return    Non-negative value on success, or -1 for failure
    \sa			sl_NetCfgGet
    \note 
    \warning  

    \par   Examples
    
	- SL_NETCFG_MAC_ADDRESS_SET: <br>
	  Setting MAC address to the Device.
      The new MAC address will override the default MAC address and it be saved in the FileSystem.
      Requires restarting the device for updating this setting.
	\code
        _u8 MAC_Address[6];
        MAC_Address[0] = 0x8;
        MAC_Address[1] = 0x0;
        MAC_Address[2] = 0x28;
        MAC_Address[3] = 0x22;
        MAC_Address[4] = 0x69;
        MAC_Address[5] = 0x31;
        sl_NetCfgSet(SL_NETCFG_MAC_ADDRESS_SET,1,SL_MAC_ADDR_LEN,(_u8 *)MAC_Address);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

    - <b>SL_NETCFG_IPV4_STA_ADDR_MODE</b><br>:
    
	- SL_NETCFG_ADDR_STATIC: <br>
	Setting a static IP address to the device working in STA mode or P2P client.
    The IP address will be stored in the FileSystem.
	\code 
        SlNetCfgIpV4Args_t ipV4;
        ipV4.Ip          = (_u32)SL_IPV4_VAL(10,1,1,201);            // _u32 IP address 
        ipV4.IpMask      = (_u32)SL_IPV4_VAL(255,255,255,0);         // _u32 Subnet mask for this STA/P2P
        ipV4.IpGateway   = (_u32)SL_IPV4_VAL(10,1,1,1);              // _u32 Default gateway address
        ipV4.IpDnsServer = (_u32)SL_IPV4_VAL(8,16,32,64);            // _u32 DNS server address

        sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE,SL_NETCFG_ADDR_STATIC,sizeof(SlNetCfgIpV4Args_t),(_u8 *)&ipV4); 
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_ADDR_DHCP:<br>
	Setting IP address by DHCP to FileSystem using WLAN sta mode or P2P client.
    This should be done once if using Serial Flash.
    This is the system's default mode for acquiring an IP address after WLAN connection.
    \code 
        sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE,SL_NETCFG_ADDR_DHCP,0,0);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_ADDR_DHCP_LLA: <br>
	Setting DHCP LLA will runs LLA mechanism in case DHCP fails to acquire an address
    SL_NETCFG_DHCP_OPT_RELEASE_IP_BEFORE_DISCONNECT - If set, enables sending a DHCP release frame to the server if user issues a WLAN disconnect command.
	\code 
        sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE,SL_NETCFG_ADDR_DHCP_LLA,0,0);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_ADDR_RELEASE_IP_SET: <br>
    Setting release ip before disconnect enables sending a DHCP release frame to the server if user issues a WLAN disconnect command.
	\code
        sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE,SL_NETCFG_ADDR_RELEASE_IP_SET,0,0); 
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_ADDR_RELEASE_IP_OFF:<br>
	Setting release ip before disconnect disables sending a DHCP release frame to the server if user issues a WLAN disconnect command.
	\code 
        sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE,SL_NETCFG_ADDR_RELEASE_IP_OFF,0,0); 
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_IPV4_AP_ADDR_MODE:<br>
	Setting a static IP address to the device working in AP mode or P2P go.
    The IP address will be stored in the FileSystem. Requires restart.
    \code                                                             
        SlNetCfgIpV4Args_t ipV4;
        ipV4.Ip          = (_u32)SL_IPV4_VAL(10,1,1,201);            // _u32 IP address 
        ipV4.IpMask      = (_u32)SL_IPV4_VAL(255,255,255,0);         // _u32 Subnet mask for this AP/P2P
        ipV4.IpGateway   = (_u32)SL_IPV4_VAL(10,1,1,1);              // _u32 Default gateway address
        ipV4.IpDnsServer = (_u32)SL_IPV4_VAL(8,16,32,64);            // _u32 DNS server address

        sl_NetCfgSet(SL_NETCFG_IPV4_AP_ADDR_MODE,SL_NETCFG_ADDR_STATIC,sizeof(SlNetCfgIpV4Args_t),(_u8 *)&ipV4);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_IF:<br>
	Enable\Disable IPV6 interface - Local or/and Global address (Global could not be enabled without Local)
	\code
        _u32 IfBitmap = 0;

	    IfBitmap = SL_NETCFG_IF_IPV6_STA_LOCAL | SL_NETCFG_IF_IPV6_STA_GLOBAL;
		sl_NetCfgSet(SL_NETCFG_IF,SL_NETCFG_IF_STATE,sizeof(IfBitmap),&IfBitmap);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_IPV6_ADDR_LOCAL: <br>
	Setting a IPv6 Local static address to the device working in STA mode.
    The IP address will be stored in the FileSystem. Requires restart.
	\code       
  		SlNetCfgIpV6Args_t ipV6;
		_u32 IfBitmap = 0;

	    IfBitmap = SL_NETCFG_IF_IPV6_STA_LOCAL;
		sl_NetCfgSet(SL_NETCFG_IF,SL_NETCFG_IF_STATE,sizeof(IfBitmap),&IfBitmap);

        ipV6.Ip[0] = 0xfe800000;
		ipV6.Ip[1] = 0x00000000;
		ipV6.Ip[2] = 0x00004040;
		ipV6.Ip[3] = 0x0000ce65;

        sl_NetCfgSet(SL_NETCFG_IPV6_ADDR_LOCAL,SL_NETCFG_ADDR_STATIC,sizeof(SlNetCfgIpV6Args_t),(_u8 *)&ipV6);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_IPV6_ADDR_LOCAL: <br>
	Setting a IPv6 Local stateless address to the device working in STA mode.
    The IP address will be stored in the FileSystem. Requires restart.
    \code       
		_u32 IfBitmap = 0;
	    IfBitmap = SL_NETCFG_IF_IPV6_STA_LOCAL;
		sl_NetCfgSet(SL_NETCFG_IF,SL_NETCFG_IF_STATE,sizeof(IfBitmap),&IfBitmap);
        sl_NetCfgSet(SL_NETCFG_IPV6_ADDR_LOCAL,SL_NETCFG_ADDR_STATELESS,0,0);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_IPV6_ADDR_LOCAL: <br>
	Setting a IPv6 Local statefull address to the device working in STA mode.
    The IP address will be stored in the FileSystem. Requires restart.
	\code
		_u32 IfBitmap = 0;

	    IfBitmap = SL_NETCFG_IF_IPV6_STA_LOCAL;
		sl_NetCfgSet(SL_NETCFG_IF,SL_NETCFG_IF_STATE,sizeof(IfBitmap),&IfBitmap);
        sl_NetCfgSet(SL_NETCFG_IPV6_ADDR_LOCAL,SL_NETCFG_ADDR_STATEFUL,0,0);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

    - SL_NETCFG_IPV6_ADDR_GLOBAL:<br>
	Setting a IPv6 Global static address to the device working in STA mode.
    The IP address will be stored in the FileSystem. Requires restart.
	\code       
		SlNetCfgIpV6Args_t ipV6;
		_u32 IfBitmap = 0;

        ipV6.Ip[0] = 0xfe80;
		ipV6.Ip[1] = 0x03a;
		ipV6.Ip[2] = 0x4040;
		ipV6.Ip[3] = 0xce65;

		ipV6.IpDnsServer[0] = 0xa780;
		ipV6.IpDnsServer[1] = 0x65e;
		ipV6.IpDnsServer[2] = 0x8;
		ipV6.IpDnsServer[3] = 0xce00;

	    IfBitmap = SL_NETCFG_IF_IPV6_STA_GLOBAL;
		sl_NetCfgSet(SL_NETCFG_IF,SL_NETCFG_IF_STATE,sizeof(IfBitmap),&IfBitmap);
		sl_NetCfgSet(SL_NETCFG_IPV6_ADDR_GLOBAL,SL_NETCFG_ADDR_STATIC,sizeof(SlNetCfgIpV6Args_t),(_u8 *)&ipV6);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_IPV6_ADDR_GLOBAL:<br>
	Setting a IPv6 Global statefull address to the device working in STA mode.
    The IP address will be stored in the FileSystem. Requires restart.
	\code
		_u32 IfBitmap = 0;
		IfBitmap = SL_NETCFG_IF_IPV6_STA_GLOBAL;
		sl_NetCfgSet(SL_NETCFG_IF,SL_NETCFG_IF_STATE,sizeof(IfBitmap),&IfBitmap);
		sl_NetCfgSet(SL_NETCFG_IPV6_ADDR_GLOBAL,SL_NETCFG_ADDR_STATEFUL,0,0);
        sl_Stop(0);
        sl_Start(NULL,NULL,NULL);
    \endcode
	<br>

	- SL_NETCFG_AP_STATION_DISCONNECT:<br>
	Disconnet AP station by mac address.
    The AP connected stations list can be read by sl_NetCfgGet with options: SL_AP_STATIONS_NUM_CONNECTED, SL_AP_STATIONS_INFO_LIST
    \code
        _u8  ap_sta_mac[6] = { 0x00, 0x22, 0x33, 0x44, 0x55, 0x66 };     
        sl_NetCfgSet(SL_NETCFG_AP_STATION_DISCONNECT,1,SL_MAC_ADDR_LEN,(_u8 *)ap_sta_mac);
    \endcode 
	<br>

	- SL_NETCFG_IPV4_DNS_CLIENT:<br>
	Set secondary DNS address (DHCP and static configuration) not persistent
	\code                                                                        
		_i32 Status;
		SlNetCfgIpV4DnsClientArgs_t DnsOpt;
		DnsOpt.DnsSecondServerAddr  =  SL_IPV4_VAL(8,8,8,8); ;
		Status = sl_NetCfgSet(SL_NETCFG_IPV4_DNS_CLIENT,0,sizeof(SlNetCfgIpV4DnsClientArgs_t),(unsigned char *)&DnsOpt);
		if( Status )
		{
				// error 
		}
    \endcode

   
*/
#if _SL_INCLUDE_FUNC(sl_NetCfgSet)
_i16 sl_NetCfgSet(const _u16 ConfigId,const _u16 ConfigOpt,const _u16 ConfigLen,const _u8 *pValues);
#endif


/*!
    \brief     Getting network configurations
   
    \param[in] ConfigId      Configuration id

    \param[out] pConfigOpt   Get configurations option 

    \param[out] pConfigLen   The length of the allocated memory as input, when the
                                        function complete, the value of this parameter would be
                             the len that actually read from the device.\n 
                                        If the device return length that is longer from the input 
                                        value, the function will cut the end of the returned structure
                                        and will return ESMALLBUF

    \param[out] pValues - get configurations values
    \return    Zero on success, or -1 on failure
    \sa      	sl_NetCfgSet   
    \note 
    \warning     
    \par   Examples

	- SL_NETCFG_MAC_ADDRESS_GET: <br>
	 Get the device MAC address.
     The returned MAC address is taken from FileSystem first. If the MAC address was not set by SL_MAC_ADDRESS_SET, the default MAC address
     is retrieved from HW.
    \code
	   _u8 macAddressVal[SL_MAC_ADDR_LEN];
       _u16 macAddressLen = SL_MAC_ADDR_LEN;
	   _u16 ConfigOpt = 0;
       sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET,&ConfigOpt,&macAddressLen,(_u8 *)macAddressVal);
    
    \endcode
	<br>

	- SL_NETCFG_IPV4_STA_ADDR_MODE: <br>
	  Get IP address from WLAN station or P2P client. A DHCP flag is returned to indicate if the IP address is static or from DHCP. 
    \code
        _u16 len = sizeof(SlNetCfgIpV4Args_t);
        _u16 ConfigOpt = 0;   //return value could be one of the following: SL_NETCFG_ADDR_DHCP / SL_NETCFG_ADDR_DHCP_LLA / SL_NETCFG_ADDR_STATIC
        SlNetCfgIpV4Args_t ipV4 = {0};
        sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE,&ConfigOpt,&len,(_u8 *)&ipV4);
                                          
		printf("DHCP is %s IP %d.%d.%d.%d MASK %d.%d.%d.%d GW %d.%d.%d.%d DNS %d.%d.%d.%d\n",
            (ConfigOpt == SL_NETCFG_ADDR_DHCP) ? "ON" : "OFF",                                                           
            SL_IPV4_BYTE(ipV4.Ip,3),SL_IPV4_BYTE(ipV4.Ip,2),SL_IPV4_BYTE(ipV4.Ip,1),SL_IPV4_BYTE(ipV4.Ip,0), 
            SL_IPV4_BYTE(ipV4.IpMask,3),SL_IPV4_BYTE(ipV4.IpMask,2),SL_IPV4_BYTE(ipV4.IpMask,1),SL_IPV4_BYTE(ipV4.IpMask,0),         
            SL_IPV4_BYTE(ipV4.IpGateway,3),SL_IPV4_BYTE(ipV4.IpGateway,2),SL_IPV4_BYTE(ipV4.IpGateway,1),SL_IPV4_BYTE(ipV4.IpGateway,0),                 
            SL_IPV4_BYTE(ipV4.IpDnsServer,3),SL_IPV4_BYTE(ipV4.IpDnsServer,2),SL_IPV4_BYTE(ipV4.IpDnsServer,1),SL_IPV4_BYTE(ipV4.IpDnsServer,0));

    \endcode
	<br>

	- SL_NETCFG_IPV4_AP_ADDR_MODE: <br>
	  Get static IP address for AP or P2P go.
    \code
        _u16 len = sizeof(SlNetCfgIpV4Args_t);
        _u16 ConfigOpt = 0;  //return value could be one of the following: SL_NETCFG_ADDR_DHCP / SL_NETCFG_ADDR_DHCP_LLA / SL_NETCFG_ADDR_STATIC
        SlNetCfgIpV4Args_t ipV4 = {0};
        sl_NetCfgGet(SL_NETCFG_IPV4_AP_ADDR_MODE,&ConfigOpt,&len,(_u8 *)&ipV4);
                                          
		printf("DHCP is %s IP %d.%d.%d.%d MASK %d.%d.%d.%d GW %d.%d.%d.%d DNS %d.%d.%d.%d\n",
            (ConfigOpt == SL_NETCFG_ADDR_DHCP) ? "ON" : "OFF",                                                           
            SL_IPV4_BYTE(ipV4.Ip,3),SL_IPV4_BYTE(ipV4.Ip,2),SL_IPV4_BYTE(ipV4.Ip,1),SL_IPV4_BYTE(ipV4.Ip,0), 
            SL_IPV4_BYTE(ipV4.IpMask,3),SL_IPV4_BYTE(ipV4.IpMask,2),SL_IPV4_BYTE(ipV4.IpMask,1),SL_IPV4_BYTE(ipV4.IpMask,0),         
            SL_IPV4_BYTE(ipV4.IpGateway,3),SL_IPV4_BYTE(ipV4.IpGateway,2),SL_IPV4_BYTE(ipV4.IpGateway,1),SL_IPV4_BYTE(ipV4.IpGateway,0),                 
            SL_IPV4_BYTE(ipV4.IpDnsServer,3),SL_IPV4_BYTE(ipV4.IpDnsServer,2),SL_IPV4_BYTE(ipV4.IpDnsServer,1),SL_IPV4_BYTE(ipV4.IpDnsServer,0));
    \endcode
	<br>

	- SL_NETCFG_IF: <br>
	Get interface bitmap
	\code
		_u16 len;
		_u32 IfBitmap;
		len = sizeof(IfBitmap);
		sl_NetCfgGet(SL_NETCFG_IF,NULL,&len,(_u8 *)&IfBitmap);
    \endcode
	<br>

	- SL_NETCFG_IPV6_ADDR_LOCAL: <br>
	Get IPV6 Local address (ipV6.ipV6IsValid holds the address status. 1=Valid, ipv6 DAD completed and address is valid for use) 
	\code
		SlNetCfgIpV6Args_t ipV6;
        _u16 len = sizeof(SlNetCfgIpV6Args_t);
        _u16 ConfigOpt = 0;  //return value could be one of the following: SL_NETCFG_ADDR_STATIC / SL_NETCFG_ADDR_STATELESS / SL_NETCFG_ADDR_STATEFUL
       
		sl_NetCfgGet(SL_NETCFG_IPV6_ADDR_LOCAL,&ConfigOpt,&len,(_u8 *)&ipV6);
		if (SL_IS_IPV6_ADDR_VALID(ipV6.IpV6Flags))
		{
			printf("Ipv6 Local Address is valid: %8x:%8x:%8x:%8x\n", ipV6.Ip[0],ipV6.Ip[0],ipV6.Ip[0],ipV6.Ip[0]);
		}
		else
		{
			printf("Ipv6 Local Address is not valid, wait for DAD to complete or configure a different address");
		}

    \endcode
	<br>

	- SL_NETCFG_IPV6_ADDR_GLOBAL:<br>
	Get IPV6 Global address (ipV6.ipV6IsValid holds the address status. 1=Valid, ipv6 DAD completed and address is valid for use) 
	\code
		SlNetCfgIpV6Args_t ipV6;
        _u16 len = sizeof(SlNetCfgIpV6Args_t);
        _u16 ConfigOpt = 0;  //return value could be one of the following: SL_NETCFG_ADDR_STATIC  / SL_NETCFG_ADDR_STATEFUL
       
		if (SL_IS_IPV6_ADDR_VALID(ipV6.IpV6Flags))
		{
			printf("Ipv6 Global Address is valid: %8x:%8x:%8x:%8x\n", ipV6.Ip[0],ipV6.Ip[0],ipV6.Ip[0],ipV6.Ip[0]);
		}
		else
		{
			printf("Ipv6 Global Address is not valid, wait for DAD to complete or configure a different address");
		}

    \endcode
	<br>

	- SL_NETCFG_AP_STATIONS_NUM_CONNECTED: <br>
	  Get AP numbber of connected stations.   
    \code
        _u8 num_ap_connected_sta;
        _u16 len = sizeof(num_ap_connected_sta);
        sl_NetCfgGet(SL_NETCFG_AP_STATIONS_NUM_CONNECTED, NULL, &len, &num_ap_connected_sta);
        printf("AP number of connected stations = %d\n", num_ap_connected_sta);

    \endcode
	<br>

	- SL_NETCFG_AP_STATIONS_INFO_LIST: <br>
 	  Get AP full list of connected stationss.
    \code
        SlNetCfgStaInfo_t ApStaList[4]; 
        _u16 sta_info_len;
        _u16 start_sta_index = 0;
        int actual_num_sta;
        int i;

        start_sta_index = 0;
        sta_info_len = sizeof(ApStaList);
        sl_NetCfgGet(SL_NETCFG_AP_STATIONS_INFO_LIST, &start_sta_index, &sta_info_len, (_u8 *)ApStaList);
        
        actual_num_sta = sta_info_len / sizeof(SlNetCfgStaInfo_t);
        printf("-Print SL_NETCFG_AP_STATIONS_INFO_LIST actual num_stations = %d (upon sta_info_len = %d)\n", actual_num_sta, sta_info_len);

        for (i=0; i<actual_num_sta; i++)
        {
            SlNetCfgStaInfo_t *staInfo = &ApStaList[i];
            printf("  Ap Station %d is connected\n", i);
            printf("    NAME: %s\n", staInfo->Name);
            printf("    MAC:  %02x:%02x:%02x:%02x:%02x:%02x\n", staInfo->MacAddr[0], staInfo->MacAddr[1], staInfo->MacAddr[2], staInfo->MacAddr[3], staInfo->MacAddr[4], staInfo->MacAddr[5]);
            printf("    IP:   %d.%d.%d.%d\n", SL_IPV4_BYTE(staInfo->Ip,3), SL_IPV4_BYTE(staInfo->Ip,2), SL_IPV4_BYTE(staInfo->Ip,1), SL_IPV4_BYTE(staInfo->Ip,0));
        }

    \endcode
	<br>

	- SL_NETCFG_IPV4_DNS_CLIENT: <br>
	  Set secondary DNS address (DHCP and static configuration) 
	\code                                                               
		_u16 ConfigOpt = 0;
		_i32 Status;
		_u16 pConfigLen = sizeof(SlNetCfgIpV4DnsClientArgs_t);
		SlNetCfgIpV4DnsClientArgs_t DnsOpt;
		Status = sl_NetCfgGet(SL_NETCFG_IPV4_DNS_CLIENT,&ConfigOpt,&pConfigLen,&DnsOpt);
		if( Status )
		{
				// error 
		}
    \endcode

    - SL_NETCFG_IPV4_DHCP_CLIENT: <br>
	  Get DHCP Client info 
	\code                                                               
		_u16 ConfigOpt = 0;
        _u16 pConfigLen = sizeof(SlNetCfgIpv4DhcpClient_t);
        SlNetCfgIpv4DhcpClient_t dhcpCl;
        SlNetCfgIpV4Args_t ipV4 = {0};

        ret = sl_NetCfgGet(SL_NETCFG_IPV4_DHCP_CLIENT, &ConfigOpt, &pConfigLen, (_u8 *)&dhcpCl);
        if(ret < 0)
        {
            printf("Error = %d\n", ret);
        }

    \endcode
   
*/
#if _SL_INCLUDE_FUNC(sl_NetCfgGet)
_i16 sl_NetCfgGet(const _u16 ConfigId ,_u16 *pConfigOpt, _u16 *pConfigLen, _u8 *pValues);
#endif

/*!

 Close the Doxygen group.
 @}

 */


#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif    /*  __NETCFG_H__ */

