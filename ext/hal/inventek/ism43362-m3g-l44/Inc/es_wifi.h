/**
  ******************************************************************************
  * @file    es-wifi.h
  * @author  MCD Application Team
  * @brief   header file for the es-wifi module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ES_WIFI_H
#define __ES_WIFI_H

#ifdef __cplusplus
 extern "C" {
#endif  

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "es_wifi_conf.h"

/* Exported Constants --------------------------------------------------------*/
#define ES_WIFI_PAYLOAD_SIZE     1200
/* Exported macro-------------------------------------------------------------*/
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
   
typedef int8_t (*IO_Init_Func)( void);
typedef int8_t (*IO_DeInit_Func)( void);
typedef void (*IO_Delay_Func)(uint32_t);
typedef int16_t (*IO_Send_Func)( uint8_t *, uint16_t len, uint32_t);
typedef int16_t (*IO_Receive_Func)(uint8_t *, uint16_t len, uint32_t);

/* Exported typedef ----------------------------------------------------------*/
typedef enum {
  ES_WIFI_STATUS_OK             = 0,
  ES_WIFI_STATUS_REQ_DATA_STAGE = 1,  
  ES_WIFI_STATUS_ERROR          = 2,
  ES_WIFI_STATUS_TIMEOUT        = 3,
  ES_WIFI_STATUS_IO_ERROR       = 4,
}ES_WIFI_Status_t;

typedef enum {
  ES_WIFI_MODE_SINGLE           = 0,
  ES_WIFI_MODE_MULTI            = 1,
}ES_WIFI_ConnMode_t;

typedef enum {
  ES_WIFI_TCP_CONNECTION        = 0,
  ES_WIFI_UDP_CONNECTION        = 1,
  ES_WIFI_UDP_LITE_CONNECTION   = 2,
  ES_WIFI_TCP_SSL_CONNECTION    = 3,    
  ES_WIFI_MQTT_CONNECTION       = 4,    
}ES_WIFI_ConnType_t;

/* Security settings for wifi network */
typedef enum {
  ES_WIFI_SEC_OPEN = 0x00,          /*!< Wifi is open */
  ES_WIFI_SEC_WEP  = 0x01,          /*!< Wired Equivalent Privacy option for wifi security. \note This mode can't be used when setting up ES_WIFI wifi */
  ES_WIFI_SEC_WPA  = 0x02,          /*!< Wi-Fi Protected Access */
  ES_WIFI_SEC_WPA2 = 0x03,          /*!< Wi-Fi Protected Access 2 */
  ES_WIFI_SEC_WPA_WPA2= 0x04,       /*!< Wi-Fi Protected Access with both modes */
  ES_WIFI_SEC_WPA2_TKIP= 0x05,      /*!< Wi-Fi Protected Access with both modes */        
  ES_WIFI_SEC_UNKNOWN = 0xFF,       /*!< Wi-Fi Unknown Security mode */        
} ES_WIFI_SecurityType_t;

typedef enum {
  ES_WIFI_IPV4 = 0x00,   
  ES_WIFI_IPV6 = 0x01,   
} ES_WIFI_IPVer_t;

typedef enum {
  ES_WIFI_AP_NONE     = 0x00,   
  ES_WIFI_AP_ASSIGNED = 0x01, 
  ES_WIFI_AP_JOINED   = 0x02,  
  ES_WIFI_AP_ERROR    = 0xFF,    
} ES_WIFI_APState_t;


typedef struct
{
  uint32_t Port;
  uint32_t BaudRate;          
  uint32_t DataWidth;   
  uint32_t Parity;    
  uint32_t StopBits; 
  uint32_t Mode;   
                        
}ES_WIFI_UARTConfig_t;


typedef struct
{
  uint32_t Configuration;
  uint32_t WPSPin;          
  uint32_t VID;   
  uint32_t PID;     
  uint8_t MAC[6];    
  uint8_t AP_IPAddress[4]; 
  uint32_t PS_Mode; 
  uint32_t RadioMode;   
  uint32_t CurrentBeacon;  
  uint32_t PrevBeacon;  
  uint32_t ProductName;      
                        
}ES_WIFI_SystemConfig_t;

typedef struct {
  uint8_t* Address;                        /*!< Pointer to domain or IP to ping */
  uint32_t Time;                           /*!< Time in milliseconds needed for pinging */          
  uint8_t Success;                         /*!< Status indicates if ping was successful */
}ES_WIFI_Ping_t;

typedef struct {
  uint8_t SSID[ES_WIFI_MAX_SSID_NAME_SIZE + 1]; /*!< Service Set Identifier value.Wi-Fi spot name */
  ES_WIFI_SecurityType_t Security;         /*!< Security of Wi-Fi spot.  */ 
  int16_t RSSI;                            /*!< Signal strength of Wi-Fi spot */
  uint8_t MAC[6];                          /*!< MAC address of spot */
  uint8_t Channel;                         /*!< Wi-Fi channel */
} ES_WIFI_AP_t;

/* Access point configuration */
typedef struct {
  uint8_t SSID[ES_WIFI_MAX_SSID_NAME_SIZE + 1];  /*!< Network public name for ESP AP mode */
  uint8_t Pass[ES_WIFI_MAX_PSWD_NAME_SIZE + 1];  /*!< Network password for ESP AP mode */
  ES_WIFI_SecurityType_t Security;          /*!< Security of Wi-Fi spot. This parameter can be a value of \ref ESP8266_Ecn_t enumeration */
  uint8_t Channel;                          /*!< Channel Wi-Fi is operating at */
  uint8_t MaxConnections;                   /*!< Max number of stations that are allowed to connect to ESP AP, between 1 and 4 */
  uint8_t Hidden;                           /*!< Set to 1 if network is hidden (not broadcast) or zero if noz */
} ES_WIFI_APConfig_t;


typedef struct {
  uint8_t SSID[ES_WIFI_MAX_SSID_NAME_SIZE + 1];  /*!< Network public name for ESP AP mode */  
  uint8_t IP_Addr[4];                       /*!< IP Address */
  uint8_t MAC_Addr[6];                      /*!< MAC address */  
} ES_WIFI_APSettings_t;

typedef struct {
  ES_WIFI_AP_t AP[ES_WIFI_MAX_DETECTED_AP];
  uint8_t nbr;
  
}ES_WIFI_APs_t;

typedef struct {
  uint8_t          SSID[ES_WIFI_MAX_SSID_NAME_SIZE + 1];
  uint8_t          pswd[ES_WIFI_MAX_PSWD_NAME_SIZE + 1];
  ES_WIFI_SecurityType_t Security;   
  uint8_t          DHCP_IsEnabled;
  uint8_t          JoinRetries;
  uint8_t          IsConnected; 
  uint8_t          AutoConnect;    
  ES_WIFI_IPVer_t  IP_Ver;
  uint8_t          IP_Addr[4]; 
  uint8_t          IP_Mask[4];  
  uint8_t          Gateway_Addr[4]; 
  uint8_t          DNS1[4];   
  uint8_t          DNS2[4]; 
} ES_WIFI_Network_t;

#if (ES_WIFI_USE_AWS == 1)
typedef struct {
  ES_WIFI_ConnType_t Type;     
  uint8_t            Number;             
  uint16_t           RemotePort;  
  uint8_t            RemoteIP[4];       
  uint8_t            *PublishTopic;             
  uint8_t            *SubscribeTopic;
  uint8_t            *ClientID;  
  uint8_t            MQTTMode;   
} ES_WIFI_AWS_Conn_t;
#endif
  
typedef struct {
  ES_WIFI_ConnType_t Type;     
  uint8_t            Number;             
  uint16_t           RemotePort;         
  uint16_t           LocalPort;          
  uint8_t            RemoteIP[4];        
  char*              Name;  
} ES_WIFI_Conn_t;

typedef struct {
  IO_Init_Func       IO_Init;  
  IO_DeInit_Func     IO_DeInit;
  IO_Delay_Func      IO_Delay;  
  IO_Send_Func       IO_Send;
  IO_Receive_Func    IO_Receive;  
} ES_WIFI_IO_t;

typedef struct {
  uint8_t           Product_ID[ES_WIFI_PRODUCT_ID_SIZE];     
  uint8_t           FW_Rev[ES_WIFI_FW_REV_SIZE];       
  uint8_t           API_Rev[ES_WIFI_API_REV_SIZE];                
  uint8_t           Stack_Rev[ES_WIFI_STACK_REV_SIZE];       
  uint8_t           RTOS_Rev[ES_WIFI_RTOS_REV_SIZE];
  uint8_t           Product_Name[ES_WIFI_PRODUCT_NAME_SIZE];  
  uint32_t          CPU_Clock;
  ES_WIFI_SecurityType_t Security;   
  ES_WIFI_Network_t NetSettings;
  ES_WIFI_APSettings_t APSettings;
  ES_WIFI_IO_t       fops;
  uint8_t            CmdData[ES_WIFI_DATA_SIZE];
  uint32_t           Timeout;
  uint32_t           BufferSize; 
}ES_WIFIObject_t;


/* Exported functions --------------------------------------------------------*/
ES_WIFI_Status_t  ES_WIFI_Init(ES_WIFIObject_t *Obj);
ES_WIFI_Status_t  ES_WIFI_SetTimeout(ES_WIFIObject_t *Obj, uint32_t Timeout);
ES_WIFI_Status_t  ES_WIFI_ListAccessPoints(ES_WIFIObject_t *Obj, ES_WIFI_APs_t *APs);
ES_WIFI_Status_t  ES_WIFI_Connect(ES_WIFIObject_t *Obj, const char* SSID, const char* Password,
                                          ES_WIFI_SecurityType_t SecType);
ES_WIFI_Status_t  ES_WIFI_Disconnect(ES_WIFIObject_t *Obj);
uint8_t           ES_WIFI_IsConnected(ES_WIFIObject_t *Obj);
ES_WIFI_Status_t  ES_WIFI_GetNetworkSettings(ES_WIFIObject_t *Obj);
ES_WIFI_Status_t  ES_WIFI_GetMACAddress(ES_WIFIObject_t *Obj, uint8_t *mac);
ES_WIFI_Status_t  ES_WIFI_GetIPAddress(ES_WIFIObject_t *Obj, uint8_t *ipaddr);
ES_WIFI_Status_t  ES_WIFI_GetProductID(ES_WIFIObject_t *Obj, uint8_t *productID);
ES_WIFI_Status_t  ES_WIFI_GetFWRevID(ES_WIFIObject_t *Obj, uint8_t *FWRev);
ES_WIFI_Status_t  ES_WIFI_GetRTOSRev(ES_WIFIObject_t *Obj, uint8_t *RTOSRev);
ES_WIFI_Status_t  ES_WIFI_GetProductName(ES_WIFIObject_t *Obj, uint8_t *productName);
ES_WIFI_Status_t  ES_WIFI_GetAPIRev(ES_WIFIObject_t *Obj, uint8_t *APIRev);
ES_WIFI_Status_t  ES_WIFI_GetStackRev(ES_WIFIObject_t *Obj, uint8_t *StackRev);


ES_WIFI_Status_t  ES_WIFI_SetMACAddress(ES_WIFIObject_t *Obj, uint8_t *mac);
ES_WIFI_Status_t  ES_WIFI_ResetToFactoryDefault(ES_WIFIObject_t *Obj);
ES_WIFI_Status_t  ES_WIFI_ResetModule(ES_WIFIObject_t *Obj);
ES_WIFI_Status_t  ES_WIFI_SetProductName(ES_WIFIObject_t *Obj, uint8_t *ProductName);
#if (ES_WIFI_USE_PING == 1)
ES_WIFI_Status_t  ES_WIFI_Ping(ES_WIFIObject_t *Obj, uint8_t *address, uint16_t count, uint16_t interval_ms);
#endif
ES_WIFI_Status_t  ES_WIFI_DNS_LookUp(ES_WIFIObject_t *Obj, const char *url, uint8_t *ipaddress);
ES_WIFI_Status_t  ES_WIFI_StartClientConnection(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn);
ES_WIFI_Status_t  ES_WIFI_StopClientConnection(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn);
#if (ES_WIFI_USE_AWS == 1)
ES_WIFI_Status_t  ES_WIFI_StartAWSClientConnection(ES_WIFIObject_t *Obj, ES_WIFI_AWS_Conn_t *conn);
#endif
ES_WIFI_Status_t  ES_WIFI_StartServerSingleConn(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn);
ES_WIFI_Status_t  ES_WIFI_StopServerSingleConn(ES_WIFIObject_t *Obj);
ES_WIFI_Status_t  ES_WIFI_StartServerMultiConn(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn);
ES_WIFI_Status_t  ES_WIFI_StopServerMultiConn(ES_WIFIObject_t *Obj);
ES_WIFI_Status_t  ES_WIFI_SendData(ES_WIFIObject_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Reqlen , uint16_t *SentLen, uint32_t timeout);
ES_WIFI_Status_t  ES_WIFI_ReceiveData(ES_WIFIObject_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *Receivedlen, uint32_t timeout);
ES_WIFI_Status_t  ES_WIFI_ActivateAP(ES_WIFIObject_t *Obj, ES_WIFI_APConfig_t *ApConfig);
ES_WIFI_APState_t ES_WIFI_WaitAPStateChange(ES_WIFIObject_t *Obj);

#if (ES_WIFI_USE_FIRMWAREUPDATE == 1)
ES_WIFI_Status_t  ES_WIFI_OTA_Upgrade(ES_WIFIObject_t *Obj, uint8_t *link);
#endif

#if (ES_WIFI_USE_UART == 1)
ES_WIFI_Status_t  ES_WIFI_SetUARTBaudRate(ES_WIFIObject_t *Obj, uint16_t BaudRate);
ES_WIFI_Status_t  ES_WIFI_GetUARTConfig(ES_WIFIObject_t *Obj, ES_WIFI_UARTConfig_t *pconf);
#endif

ES_WIFI_Status_t  ES_WIFI_GetSystemConfig(ES_WIFIObject_t *Obj, ES_WIFI_SystemConfig_t *pconf);

ES_WIFI_Status_t  ES_WIFI_RegisterBusIO(ES_WIFIObject_t *Obj, IO_Init_Func IO_Init,
                                                              IO_DeInit_Func  IO_DeInit,
                                                              IO_Delay_Func   IO_Delay,  
                                                              IO_Send_Func    IO_Send,
                                                              IO_Receive_Func  IO_Receive);
#ifdef __cplusplus
}
#endif
#endif /*__ES_WIFI_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/ 
