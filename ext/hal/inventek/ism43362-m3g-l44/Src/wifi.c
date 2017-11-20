/**
  ******************************************************************************
  * @file    wifi.c
  * @author  MCD Application Team
  * @brief   WIFI interface file.
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
/* Includes ------------------------------------------------------------------*/
#include "wifi.h"

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
ES_WIFIObject_t    EsWifiObj;

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initialiaze the WIFI core
  * @param  None
  * @retval Operation status
  */
WIFI_Status_t WIFI_Init(void)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;
  
  if(ES_WIFI_RegisterBusIO(&EsWifiObj, 
                           SPI_WIFI_Init, 
                           SPI_WIFI_DeInit,
                           SPI_WIFI_Delay,
                           SPI_WIFI_SendData,
                           SPI_WIFI_ReceiveData) == ES_WIFI_STATUS_OK)
  {
    
    if(ES_WIFI_Init(&EsWifiObj) == ES_WIFI_STATUS_OK)
    {
      ret = WIFI_STATUS_OK;
    }
  }
  return ret;
}

/**
  * @brief  List a defined number of vailable access points
  * @param  APs : pointer to APs structure
  * @param  AP_MaxNbr : Max APs number to be listed
  * @retval Operation status
  */
WIFI_Status_t WIFI_ListAccessPoints(WIFI_APs_t *APs, uint8_t AP_MaxNbr)
{
  uint8_t APCount;
  WIFI_Status_t ret = WIFI_STATUS_ERROR;  
  ES_WIFI_APs_t esWifiAPs;
  
  if(ES_WIFI_ListAccessPoints(&EsWifiObj, &esWifiAPs) == ES_WIFI_STATUS_OK)
  {
    if(esWifiAPs.nbr > 0)
    {
      APs->count = MIN(esWifiAPs.nbr, AP_MaxNbr);  
      for(APCount = 0; APCount < APs->count; APCount++)
      {
        APs->ap[APCount].Ecn = (WIFI_Ecn_t)esWifiAPs.AP[APCount].Security;
        strncpy( (char *)APs->ap[APCount].SSID, (char *)esWifiAPs.AP[APCount].SSID, MIN (WIFI_MAX_SSID_NAME, WIFI_MAX_SSID_NAME));    
        APs->ap[APCount].RSSI = esWifiAPs.AP[APCount].RSSI;
        memcpy(APs->ap[APCount].MAC, esWifiAPs.AP[APCount].MAC, 6);
      }
    }
    ret = WIFI_STATUS_OK;  
  }
  return ret;
}

/**
  * @brief  Join an Access Point
  * @param  SSID : SSID string
  * @param  Password : Password string
  * @param  ecn : Encryption type
  * @param  IP_Addr : Got IP Address
  * @param  IP_Mask : Network IP mask
  * @param  Gateway_Addr : Gateway IP address
  * @param  MAC : pointer to MAC Address
  * @retval Operation status
  */
WIFI_Status_t WIFI_Connect(
                             const char* SSID, 
                             const char* Password,
                             WIFI_Ecn_t ecn)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;  
 
  if(ES_WIFI_Connect(&EsWifiObj, SSID, Password, (ES_WIFI_SecurityType_t) ecn) == ES_WIFI_STATUS_OK)
  {
    if(ES_WIFI_GetNetworkSettings(&EsWifiObj) == ES_WIFI_STATUS_OK)
    {
       ret = WIFI_STATUS_OK;
    }
    
  }
  return ret;
}

/**
  * @brief  This function retrieves the WiFi interface's MAC address.
  * @retval Operation Status.
  */
WIFI_Status_t WIFI_GetMAC_Address(uint8_t  *mac)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(ES_WIFI_GetMACAddress(&EsWifiObj, mac) == ES_WIFI_STATUS_OK)
  {
    ret = WIFI_STATUS_OK;
  }
  return ret;
}

/**
  * @brief  This function retrieves the WiFi interface's IP address.
  * @retval Operation Status.
  */
WIFI_Status_t WIFI_GetIP_Address (uint8_t  *ipaddr)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(EsWifiObj.NetSettings.IsConnected)
  {
    memcpy(ipaddr, EsWifiObj.NetSettings.IP_Addr, 4);
    ret = WIFI_STATUS_OK;
  }
  return ret;
}

/**
  * @brief  Disconnect from a network
  * @param  None
  * @retval Operation status
  */
WIFI_Status_t WIFI_Disconnect(void)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;    
  if( ES_WIFI_Disconnect(&EsWifiObj)== ES_WIFI_STATUS_OK)
  {
      ret = WIFI_STATUS_OK; 
  }
  
  return ret;
}

/**
  * @brief  Configure an Access Point

  * @param  ssid : SSID string
  * @param  pass : Password string
  * @param  ecn : Encryption type
  * @param  channel : channel number
  * @param  max_conn : Max allowed connections
  * @retval Operation status
  */
WIFI_Status_t WIFI_ConfigureAP(uint8_t *ssid, uint8_t *pass, WIFI_Ecn_t ecn, uint8_t channel, uint8_t max_conn)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;  
  ES_WIFI_APConfig_t ApConfig;
  
  strncpy((char*)ApConfig.SSID, (char*)ssid, ES_WIFI_MAX_SSID_NAME_SIZE);
  strncpy((char*)ApConfig.Pass, (char*)pass, ES_WIFI_MAX_PSWD_NAME_SIZE);
  ApConfig.Channel = channel;
  ApConfig.MaxConnections = WIFI_MAX_CONNECTED_STATIONS;
  ApConfig.Security = (ES_WIFI_SecurityType_t)ecn;
  
  if(ES_WIFI_ActivateAP(&EsWifiObj, &ApConfig) == ES_WIFI_STATUS_OK)
  {
      ret = WIFI_STATUS_OK; 
  }
  return ret;
}

/**
  * @brief  Handle the background events of the wifi module

  * @retval None
*/
WIFI_Status_t WIFI_HandleAPEvents(WIFI_APSettings_t *setting)
{
  WIFI_Status_t ret = WIFI_STATUS_OK;   
  ES_WIFI_APState_t State;
  
  State= ES_WIFI_WaitAPStateChange(&EsWifiObj);
  
  switch (State)
  {
  case ES_WIFI_AP_ASSIGNED:
    memcpy(setting->IP_Addr, EsWifiObj.APSettings.IP_Addr, 4);  
    memcpy(setting->MAC_Addr, EsWifiObj.APSettings.MAC_Addr, 6);
    ret = WIFI_STATUS_ASSIGNED;    
    break;
    
  case ES_WIFI_AP_JOINED:
    strncpy((char *)setting->SSID, (char *)EsWifiObj.APSettings.SSID, WIFI_MAX_SSID_NAME);
    memcpy(setting->IP_Addr, EsWifiObj.APSettings.IP_Addr, 4); 
    ret = WIFI_STATUS_JOINED;
    break;
    
  case ES_WIFI_AP_ERROR:
    ret = WIFI_STATUS_ERROR;  
    break;
    
  default:
    break;
  }
  
  return ret;
}

/**
  * @brief  Ping an IP address in the network
  * @param  ipaddr : array of the IP address
  * @retval Operation status
  */
WIFI_Status_t WIFI_Ping(uint8_t* ipaddr, uint16_t count, uint16_t interval_ms)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;  

  if(ES_WIFI_Ping(&EsWifiObj, ipaddr, count, interval_ms) == ES_WIFI_STATUS_OK)
  {
    ret = WIFI_STATUS_OK;
  }
  return ret;
}

/**
  * @brief  Get IP address from URL using DNS
  * @param  location : Host URL
  * @param  ipaddr : array of the IP address
  * @retval Operation status
  */
WIFI_Status_t WIFI_GetHostAddress( char* location, uint8_t* ipaddr)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;  
  
  if (ES_WIFI_DNS_LookUp(&EsWifiObj, location, ipaddr) == ES_WIFI_STATUS_OK)
  {
    return WIFI_STATUS_OK;
  }
  
  return ret;
}
/**
  * @brief  Configure and start a client connection
  * @param  type : Connection type TCP/UDP
  * @param  name : name of the connection
  * @param  ipaddr : Client IP address
  * @param  port : Remote port
  * @param  local_port : Local port
  * @retval Operation status
  */
WIFI_Status_t WIFI_OpenClientConnection(uint32_t socket, WIFI_Protocol_t type, const char* name, uint8_t* ipaddr, uint16_t port, uint16_t local_port)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;
  ES_WIFI_Conn_t conn;
  
  conn.Number = socket;
  conn.RemotePort = port;
  conn.LocalPort = local_port;
  conn.Type = (type == WIFI_TCP_PROTOCOL)? ES_WIFI_TCP_CONNECTION : ES_WIFI_UDP_CONNECTION;
  conn.RemoteIP[0] = ipaddr[0];
  conn.RemoteIP[1] = ipaddr[1];
  conn.RemoteIP[2] = ipaddr[2];
  conn.RemoteIP[3] = ipaddr[3];
  if(ES_WIFI_StartClientConnection(&EsWifiObj, &conn)== ES_WIFI_STATUS_OK)
  {
    ret = WIFI_STATUS_OK;
  }
  return ret;
}

/**
  * @brief  Close client connection
  * @param  type : Connection type TCP/UDP
  * @param  name : name of the connection
  * @param  location : Client address
  * @param  port : Remote port
  * @param  local_port : Local port
  * @retval Operation status
  */
WIFI_Status_t WIFI_CloseClientConnection(uint32_t socket)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;  
  ES_WIFI_Conn_t conn;
  conn.Number = socket;
  
  if(ES_WIFI_StopClientConnection(&EsWifiObj, &conn)== ES_WIFI_STATUS_OK)
  {
    ret = WIFI_STATUS_OK;
  }
  return ret; 
}

/**
  * @brief  Configure and start a Server
  * @param  type : Connection type TCP/UDP
  * @param  name : name of the connection
  * @param  port : Remote port
  * @retval Operation status
  */
WIFI_Status_t WIFI_StartServer(uint32_t socket, WIFI_Protocol_t protocol, const char* name, uint16_t port)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;
  ES_WIFI_Conn_t conn;
  conn.Number = socket;
  conn.LocalPort = port;
  conn.Type = (protocol == WIFI_TCP_PROTOCOL)? ES_WIFI_TCP_CONNECTION : ES_WIFI_UDP_CONNECTION;
  if(ES_WIFI_StartServerSingleConn(&EsWifiObj, &conn)== ES_WIFI_STATUS_OK)
  {
    ret = WIFI_STATUS_OK;
  }
  return ret;
}

/**
  * @brief  Stop a server
  * @retval Operation status
  */
WIFI_Status_t WIFI_StopServer(uint32_t socket)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;
  
  if(ES_WIFI_StopServerSingleConn(&EsWifiObj)== ES_WIFI_STATUS_OK)
  {
    ret = WIFI_STATUS_OK;
  }
  return ret;
}
/**
  * @brief  Send Data on a socket
  * @param  pdata : pointer to data to be sent
  * @param  len : length of data to be sent
  * @retval Operation status
  */
WIFI_Status_t WIFI_SendData(uint8_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *SentDatalen, uint32_t Timeout)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR;

    if(ES_WIFI_SendData(&EsWifiObj, socket, pdata, Reqlen, SentDatalen, Timeout) == ES_WIFI_STATUS_OK)
    {
      ret = WIFI_STATUS_OK;
    }

  return ret;
}

/**
  * @brief  Receive Data from a socket
  * @param  pdata : pointer to Rx buffer
  * @param  *len :  pointer to length of data
  * @retval Operation status
  */
WIFI_Status_t WIFI_ReceiveData(uint8_t socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *RcvDatalen, uint32_t Timeout)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 

  if(ES_WIFI_ReceiveData(&EsWifiObj, socket, pdata, Reqlen, RcvDatalen, Timeout) == ES_WIFI_STATUS_OK)
  {
    ret = WIFI_STATUS_OK; 
  }
  return ret;
}

/**
  * @brief  Customize module data
  * @param  name : MFC name
  * @param  Mac :  Mac Address
  * @retval Operation status
  */
WIFI_Status_t WIFI_SetOEMProperties(const char *name, uint8_t *Mac)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(ES_WIFI_SetProductName(&EsWifiObj, (uint8_t *)name) == ES_WIFI_STATUS_OK)
  {
    if(ES_WIFI_SetMACAddress(&EsWifiObj, Mac) == ES_WIFI_STATUS_OK)
    {
      ret = WIFI_STATUS_OK;
    }
  }
  return ret;
}

/**
  * @brief  Reset the WIFI module
  * @retval Operation status
  */
WIFI_Status_t WIFI_ResetModule(void)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(ES_WIFI_ResetModule(&EsWifiObj) == ES_WIFI_STATUS_OK)
  {
      ret = WIFI_STATUS_OK;
  }
  return ret;
}

/**
  * @brief  Restore module default configuration
  * @retval Operation status
  */
WIFI_Status_t WIFI_SetModuleDefault(void)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(ES_WIFI_ResetToFactoryDefault(&EsWifiObj) == ES_WIFI_STATUS_OK)
  {
      ret = WIFI_STATUS_OK;
  }
  return ret;
}


/**
  * @brief  Update module firmware
  * @param  location : Binary Location IP address
  * @retval Operation status
  */
WIFI_Status_t WIFI_ModuleFirmwareUpdate(const char *location)
{
  return WIFI_STATUS_NOT_SUPPORTED;
}

/**
  * @brief  Return Module firmware revision
  * @param  rev : revision string
  * @retval Operation status
  */
WIFI_Status_t WIFI_GetModuleFwRevision(char *rev)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(EsWifiObj.FW_Rev != NULL)
  {
    strncpy(rev, (char *)EsWifiObj.FW_Rev, ES_WIFI_FW_REV_SIZE);
    ret = WIFI_STATUS_OK; 
  }
  return ret;
}

/**
  * @brief  Return Module ID
  * @param  Info : Module ID string
  * @retval Operation status
  */
WIFI_Status_t WIFI_GetModuleID(char *Id)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(EsWifiObj.Product_ID != NULL)
  {
    strncpy(Id, (char *)EsWifiObj.Product_ID, ES_WIFI_PRODUCT_ID_SIZE);
    ret = WIFI_STATUS_OK; 
  }
  return ret;
}

/**
  * @brief  Return Module Name
  * @param  Info : Module Name string
  * @retval Operation status
  */
WIFI_Status_t WIFI_GetModuleName(char *ModuleName)
{
  WIFI_Status_t ret = WIFI_STATUS_ERROR; 
  
  if(EsWifiObj.Product_Name != NULL)
  {
    strncpy(ModuleName, (char *)EsWifiObj.Product_Name, ES_WIFI_PRODUCT_NAME_SIZE);
    ret = WIFI_STATUS_OK; 
  }
  return ret;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

