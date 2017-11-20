/**
  ******************************************************************************
  * @file    es-wifi.c
  * @author  MCD Application Team
  * @brief   This file provides a set of functions to manage the es-wifi module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright © 2017 STMicroelectronics International N.V. 
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
#include "es_wifi.h"

#define AT_OK_STRING "\r\nOK\r\n> "
#define AT_OK_STRING_LEN 8

#define AT_ERROR_STRING "\r\nERROR"



#define CHARISHEXNUM(x)                 (((x) >= '0' && (x) <= '9') || \
                                         ((x) >= 'a' && (x) <= 'f') || \
                                         ((x) >= 'A' && (x) <= 'F'))

#define CHARISNUM(x)                    ((x) >= '0' && (x) <= '9')
#define CHAR2NUM(x)                     ((x) - '0')
/* Private function prototypes -----------------------------------------------*/
static  uint8_t Hex2Num(char a);
static uint32_t ParseHexNumber(char* ptr, uint8_t* cnt);
static uint32_t ParseHexNumber(char* ptr, uint8_t* cnt);
static void ParseMAC(char* ptr, uint8_t* arr);
static void ParseIP(char* ptr, uint8_t* arr);
static ES_WIFI_SecurityType_t ParseSecurity(char* ptr);
#if (ES_WIFI_USE_UART == 1)
static void AT_ParseUARTConfig(char *pdata, ES_WIFI_UARTConfig_t *pConfig);
#endif
static void AT_ParseSystemConfig(char *pdata, ES_WIFI_SystemConfig_t *pConfig);
static void AT_ParseConnSettings(char *pdata, ES_WIFI_Network_t *NetSettings);
static ES_WIFI_Status_t AT_ExecuteCommand(ES_WIFIObject_t *Obj, uint8_t* cmd, uint8_t *pdata);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Convert char in Hex format to integer.
  * @param  a: character to convert
  * @retval integer value.
  */
static  uint8_t Hex2Num(char a) 
{
    if (a >= '0' && a <= '9') {                             /* Char is num */
        return a - '0';
    } else if (a >= 'a' && a <= 'f') {                      /* Char is lowercase character A - Z (hex) */
        return (a - 'a') + 10;
    } else if (a >= 'A' && a <= 'F') {                      /* Char is uppercase character A - Z (hex) */
        return (a - 'A') + 10;
    }
    
    return 0;
}

/**
  * @brief  Extract a hex number from a string.
  * @param  ptr: pointer to string
  * @param  cnt: pointer to the number of parsed digit
  * @retval Hex value.
  */
static uint32_t ParseHexNumber(char* ptr, uint8_t* cnt) 
{
    uint32_t sum = 0;
    uint8_t i = 0;
    
    while (CHARISHEXNUM(*ptr)) {                    		/* Parse number */
        sum <<= 4;
        sum += Hex2Num(*ptr);
        ptr++;
        i++;
    }
    
    if (cnt != NULL) {                               		/* Save number of characters used for number */
        *cnt = i;
    }
    return sum;                                        		/* Return number */
}

/**
  * @brief  Parses and returns number from string.
  * @param  ptr: pointer to string
  * @param  cnt: pointer to the number of parsed digit
  * @retval integer value.
  */
static int32_t ParseNumber(char* ptr, uint8_t* cnt) 
{
    uint8_t minus = 0, i = 0;
    int32_t sum = 0;
    
    if (*ptr == '-') {                                		/* Check for minus character */
        minus = 1;
        ptr++;
        i++;
    }
    while (CHARISNUM(*ptr)) {                        		/* Parse number */
        sum = 10 * sum + CHAR2NUM(*ptr);
        ptr++;
        i++;
    }
    if (cnt != NULL) {                                		/* Save number of characters used for number */
        *cnt = i;
    }
    if (minus) {                                    		/* Minus detected */
        return 0 - sum;
    }
    return sum;                                       		/* Return number */
}

/**
  * @brief  Parses and returns MAC address.
  * @param  ptr: pointer to string
  * @param  arr: pointer to MAC array
  * @retval None.
  */
static void ParseMAC(char* ptr, uint8_t* arr) 
{
  uint8_t hexnum = 0, hexcnt;
  
  while(* ptr) { 
    hexcnt = 1;
    if(*ptr != ':')
    {
      arr[hexnum++] = ParseHexNumber(ptr, &hexcnt);
    }
    ptr = ptr + hexcnt;
  }
}

/**
  * @brief  Parses and returns IP address.
  * @param  ptr: pointer to string
  * @param  arr: pointer to IP array
  * @retval None.
  */
static  void ParseIP(char* ptr, uint8_t* arr) 
{
  uint8_t hexnum = 0, hexcnt;
  
  while(* ptr) { 
    hexcnt = 1;
    if(*ptr != '.')
    {
      arr[hexnum++] = ParseNumber(ptr, &hexcnt);
    }
    ptr = ptr + hexcnt;
  }
}

/**
  * @brief  Parses Security type.
  * @param  ptr: pointer to string
  * @retval Encryption type.
  */
static ES_WIFI_SecurityType_t ParseSecurity(char* ptr) 
{
  if(strstr(ptr,"Open")) return ES_WIFI_SEC_OPEN;
  else if(strstr(ptr,"WEP")) return ES_WIFI_SEC_WEP;
  else if(strstr(ptr,"WPA")) return ES_WIFI_SEC_WPA;   
  else if(strstr(ptr,"WPA2")) return ES_WIFI_SEC_WPA2; 
  else if(strstr(ptr,"WPA WPA2")) return ES_WIFI_SEC_WPA_WPA2; 
  else if(strstr(ptr,"WPA2 TKIP")) return ES_WIFI_SEC_WPA2_TKIP;   
  else return ES_WIFI_SEC_UNKNOWN;           
}

/**
  * @brief  Parses ES module informations and save them in the handle.
  * @param  Obj: pointer to module handle
  * @param  ptr: pointer to string
  * @retval None.
  */
static void AT_ParseInfo(ES_WIFIObject_t *Obj,uint8_t *pdata)
{
  char *ptr;
  uint8_t num = 0;
  
  ptr = strtok((char *)pdata + 2, ",");  
  
  while (ptr != NULL){
    switch (num++) { 
    case 0:
      strncpy((char *)Obj->Product_ID,  ptr, ES_WIFI_PRODUCT_ID_SIZE);
      break;
      
    case 1:
      strncpy((char *)Obj->FW_Rev,  ptr, ES_WIFI_FW_REV_SIZE );
      break;
      
    case 2:
      strncpy((char *)Obj->API_Rev,  ptr, ES_WIFI_API_REV_SIZE);      
      break;
      
    case 3:
      strncpy((char *)Obj->Stack_Rev,  ptr, ES_WIFI_STACK_REV_SIZE);   
      break;
      
    case 4:
      strncpy((char *)Obj->RTOS_Rev,  ptr, ES_WIFI_RTOS_REV_SIZE);        
      break;
      
    case 5:
      Obj->CPU_Clock = ParseNumber(ptr, NULL);
      break;      
      
    case 6:
      ptr = strtok(ptr, "\r"); 
      strncpy((char *)Obj->Product_Name,  ptr, ES_WIFI_PRODUCT_NAME_SIZE);         
      break;
      
    default: break;
    }
    ptr = strtok(NULL, ","); 
  }
}

/**
  * @brief  Parses Access point configuration.
  * @param  APs: Access points structure
  * @param  ptr: pointer to string
  * @retval None.
  */
static void AT_ParseAP(char *pdata, ES_WIFI_APs_t *APs)
{
  uint8_t num = 0;
  char *ptr;
  APs->nbr = 0;
 
  ptr = strtok(pdata + 2, ",");   
  
  while ((ptr != NULL) && (APs->nbr < ES_WIFI_MAX_DETECTED_AP)) {
    switch (num++) { 
    case 0: /* Ignore index */
    case 4: /* Ignore Max Rate */
    case 5: /* Ignore Network Type */
    case 7: /* Ignore Radio Band */      
      break;
      
    case 1:
      ptr[strlen(ptr) - 1] = 0;
      strncpy((char *)APs->AP[APs->nbr].SSID,  ptr+ 1, ES_WIFI_MAX_SSID_NAME_SIZE + 1); 
      break;
      
    case 2: 
      ParseMAC(ptr, APs->AP[APs->nbr].MAC);
      break;

    case 3: 
      APs->AP[APs->nbr].RSSI = ParseNumber(ptr, NULL);
      break;
      
    case 6: 
      APs->AP[APs->nbr].Security = ParseSecurity(ptr);
      break;      

    case 8:            
      APs->AP[APs->nbr].Channel = ParseNumber(ptr, NULL);
      APs->nbr++; 
      num = 1;
      break;

    default: 
      break;
    }
    ptr = strtok(NULL, ","); 
  }
}

#if (ES_WIFI_USE_UART == 1)
/**
  * @brief  Parses UART configuration.
  * @param  APs: UART Config structure
  * @param  ptr: pointer to string
  * @retval None.
  */
static void AT_ParseUARTConfig(char *pdata, ES_WIFI_UARTConfig_t *pConfig)
{
  uint8_t num = 0;
  char *ptr;
 
  ptr = strtok(pdata + 2, ",");   
  
  while (ptr != NULL) {
    switch (num++) {  
    case 0: 
      pConfig->Port = ParseNumber(ptr, NULL);
      break;

    case 1: 
      pConfig->BaudRate = ParseNumber(ptr, NULL);
      break;
      
    case 2: 
      pConfig->DataWidth = ParseNumber(ptr, NULL);
      break;

    case 3: 
      pConfig->Parity = ParseNumber(ptr, NULL);
      break;
    
    case 4: 
      pConfig->StopBits = ParseNumber(ptr, NULL);
      break;   
      
    case 5: 
      pConfig->Mode = ParseNumber(ptr, NULL);
      break;        
      
    default: 
      break;
    }
    ptr = strtok(NULL, ","); 
  }
}
#endif

/**
  * @brief  Parses System configuration.
  * @param  APs: System Config structure
  * @param  ptr: pointer to string
  * @retval None.
  */
static void AT_ParseSystemConfig(char *pdata, ES_WIFI_SystemConfig_t *pConfig)
{
  uint8_t num = 0;
  char *ptr;
 
  ptr = strtok(pdata + 2, ",");   
  
  while (ptr != NULL) {
    switch (num++) {  
    case 0: 
      pConfig->Configuration = ParseNumber(ptr, NULL);
      break;

    case 1: 
      pConfig->WPSPin = ParseNumber(ptr, NULL);
      break;
      
    case 2: 
      pConfig->VID = ParseNumber(ptr, NULL);
      break;
      
    case 3: 
      pConfig->PID = ParseNumber(ptr, NULL);
      break;      

    case 4: 
      ParseMAC(ptr, pConfig->MAC);
      break;
    
    case 5: 
      ParseIP(ptr, pConfig->AP_IPAddress);
      break;   
      
    case 6: 
      pConfig->PS_Mode = ParseNumber(ptr, NULL);
      break;        
     
    case 7: 
      pConfig->RadioMode = ParseNumber(ptr, NULL);
      break;
    
    case 8: 
      pConfig->CurrentBeacon = ParseNumber(ptr, NULL);
      break;   
      
    case 9: 
      pConfig->PrevBeacon = ParseNumber(ptr, NULL);
      break;
      
    case 10: 
      pConfig->ProductName = ParseNumber(ptr, NULL);
      break;      
      
    default: 
      break;
    }
    ptr = strtok(NULL, ","); 
  }
}


/**
  * @brief  Parses WIFI connection settings.
  * @param  NetSettings: settings
  * @param  pdata: pointer to data
  * @retval None.
  */
static void AT_ParseConnSettings(char *pdata, ES_WIFI_Network_t *NetSettings)
{
  uint8_t num = 0;
  char *ptr;
 
  ptr = strtok(pdata + 2, ",");   
  
  while (ptr != NULL) {
    switch (num++) {      
    case 0:
      strncpy((char *)NetSettings->SSID,  ptr, ES_WIFI_MAX_SSID_NAME_SIZE + 1); 
      break;
      
    case 1: 
      strncpy((char *)NetSettings->pswd,  ptr, ES_WIFI_MAX_PSWD_NAME_SIZE + 1); 
      break;
      
    case 2: 
        NetSettings->Security = (ES_WIFI_SecurityType_t)ParseNumber(ptr, NULL);
        break;   

    case 3: 
      NetSettings->DHCP_IsEnabled = ParseNumber(ptr, NULL);
      break;
      
    case 4: 
      NetSettings->IP_Ver = (ES_WIFI_IPVer_t)ParseNumber(ptr, NULL);
      break;      
      
    case 5: 
      ParseIP(ptr, NetSettings->IP_Addr);
      break; 
      
    case 6: 
      ParseIP(ptr, NetSettings->IP_Mask);
      break;      
      
    case 7: 
      ParseIP(ptr, NetSettings->Gateway_Addr);
      break; 
      
    case 8: 
      ParseIP(ptr, NetSettings->DNS1);
      break;      
      
    case 9: 
      ParseIP(ptr, NetSettings->DNS2);
      break; 
      
    case 10: 
      NetSettings->JoinRetries = ParseNumber(ptr, NULL);
      break;  
      
    case 11: 
      NetSettings->AutoConnect = ParseNumber(ptr, NULL);
      break;  

    default: 
      break;
    }
    ptr = strtok(NULL, ",");
    if ((ptr != NULL) && (ptr[-1] == ','))
    { /* Ignore empty fields */
      num++;
    }
  }
}

/**
  * @brief  Execute AT command.
  * @param  Obj: pointer to module handle
  * @param  cmd: pointer to command string
  * @param  pdata: pointer to returned data
  * @retval Operation Status.
  */
static ES_WIFI_Status_t AT_ExecuteCommand(ES_WIFIObject_t *Obj, uint8_t* cmd, uint8_t *pdata)
{
  if(Obj->fops.IO_Send(cmd, strlen((char*)cmd), Obj->Timeout) > 0)
  {
    int16_t n=Obj->fops.IO_Receive(pdata, 0, Obj->Timeout);
    if(n > 0)
    {
      *(pdata+n)=0;
      if(strstr((char *)pdata, AT_OK_STRING))
      {
        return ES_WIFI_STATUS_OK;
      }
      else if(strstr((char *)pdata, AT_ERROR_STRING))
      {
        return ES_WIFI_STATUS_ERROR;
      }      
    }
  }
  return ES_WIFI_STATUS_IO_ERROR;
}

/**
  * @brief  Execute AT command with data.
  * @param  Obj: pointer to module handle
  * @param  cmd: pointer to command string
  * @param  pcmd_data: pointer to binary data
  * @param  len: binary data length
  * @param  pdata: pointer to returned data
  * @retval Operation Status.
  */
static ES_WIFI_Status_t AT_RequestSendData(ES_WIFIObject_t *Obj, uint8_t* cmd, uint8_t *pcmd_data, uint16_t len, uint8_t *pdata)
{      
  /* can send only even number of byte on first send */
  uint16_t n=strlen((char*)cmd);
  if (n &1 ) return ES_WIFI_STATUS_ERROR;
  if(Obj->fops.IO_Send(cmd, n, Obj->Timeout) == n)
  {
    int16_t n=Obj->fops.IO_Send(pcmd_data, len, Obj->Timeout);
    if(n == len)
    {
      n = Obj->fops.IO_Receive(pdata, 0, Obj->Timeout);
      if(n > 0)
      {
        *(pdata+n)=0;
        if(strstr((char *)pdata, AT_OK_STRING))
        {
          return ES_WIFI_STATUS_OK;
        }
        else if(strstr((char *)pdata, AT_ERROR_STRING))
        {
          return ES_WIFI_STATUS_ERROR;
        }      
      }
    }
    else
    {
      return ES_WIFI_STATUS_ERROR;
    }
  }
  return ES_WIFI_STATUS_IO_ERROR;
}


/**
  * @brief  Parses Received short data length.
  * @param  Obj: pointer to module handle
  * @param  cmd:command formatted string
  * @param  pdata: payload
  * @param  Reqlen : requested Data length.
  * @param  ReadData : pointer to received data length.
  * @retval Operation Status.
  */
static ES_WIFI_Status_t ReceiveShortDataLen(ES_WIFIObject_t *Obj,  char *pdata, uint16_t Reqlen, uint16_t *ReadData)
{
   uint16_t len;
   
   len = Obj->fops.IO_Receive(Obj->CmdData, Reqlen + AT_OK_STRING_LEN , Obj->Timeout);
   
   if (len >= AT_OK_STRING_LEN) 
   {
     if(strstr((char *)Obj->CmdData + len - AT_OK_STRING_LEN, AT_OK_STRING))
     {
       *ReadData = len - AT_OK_STRING_LEN;
       memcpy(pdata, Obj->CmdData, *ReadData);
       return ES_WIFI_STATUS_OK; 
     }
   }
   return ES_WIFI_STATUS_IO_ERROR;
   
}

/**
  * @brief  Parses Received long data length.
  * @param  Obj: pointer to module handle
  * @param  cmd:command formatted string
  * @param  pdata: payload
  * @param  Reqlen : requested Data length.
  * @param  ReadData : pointer to received data length.
  * @retval Operation Status.
  */
static ES_WIFI_Status_t ReceiveLongDataLen(ES_WIFIObject_t *Obj,  char *pdata, uint16_t Reqlen, uint16_t *ReadData)
{
  uint16_t len, rlen;
  
  len = Obj->fops.IO_Receive((uint8_t *)pdata, Reqlen, Obj->Timeout);
  
  if (len >= AT_OK_STRING_LEN)  
  {
    if(strstr((char *)pdata + len - AT_OK_STRING_LEN, AT_OK_STRING))
    {
      *ReadData = len - AT_OK_STRING_LEN;
      return ES_WIFI_STATUS_OK; 
    }
    else
    {     
      memcpy(Obj->CmdData, pdata + len - AT_OK_STRING_LEN, AT_OK_STRING_LEN);
      rlen = Obj->fops.IO_Receive(Obj->CmdData + AT_OK_STRING_LEN, AT_OK_STRING_LEN, Obj->Timeout);
      
      if(strstr((char *) Obj->CmdData + rlen, AT_OK_STRING))
      {
        *ReadData = len + rlen - AT_OK_STRING_LEN;
        return ES_WIFI_STATUS_OK; 
      }
    }
  }
  return ES_WIFI_STATUS_IO_ERROR;
}

/**
  * @brief  Parses Received data.
  * @param  Obj: pointer to module handle
  * @param  cmd:command formatted string
  * @param  pdata: payload
  * @param  Reqlen : requested Data length.
  * @param  ReadData : pointer to received data length.
  * @retval Operation Status.
  */
static ES_WIFI_Status_t AT_RequestReceiveData(ES_WIFIObject_t *Obj, uint8_t* cmd, char *pdata, uint16_t Reqlen, uint16_t *ReadData)
{
  if(Obj->fops.IO_Send(cmd, strlen((char*)cmd), Obj->Timeout) > 0)
  {
    if(Obj->fops.IO_Receive(Obj->CmdData, 2, Obj->Timeout) == 2) /* Read Prompt */
    {
      if (Reqlen <= AT_OK_STRING_LEN) return ReceiveShortDataLen(Obj,pdata, Reqlen ,ReadData);
      if (Reqlen >  AT_OK_STRING_LEN) return ReceiveLongDataLen(Obj,pdata, Reqlen ,ReadData);
    }
  }  
  return ES_WIFI_STATUS_IO_ERROR;
}


/**
  * @brief  Initialize WIFI module.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_Init(ES_WIFIObject_t *Obj)
{
  ES_WIFI_Status_t ret = ES_WIFI_STATUS_ERROR;
  
  Obj->Timeout = ES_WIFI_TIMEOUT;
  
  if (Obj->fops.IO_Init() == 0)
  {
    ret = AT_ExecuteCommand(Obj,(uint8_t*)"I?\r\n", Obj->CmdData);
    
    if(ret == ES_WIFI_STATUS_OK)
    {
      AT_ParseInfo (Obj, Obj->CmdData);
    }
  }
  return ret;
}

/**
  * @brief  Return ProductID.
  * @param  Obj: pointer to module handle
  * @param  productID: pointer productID
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_GetProductID(ES_WIFIObject_t *Obj, uint8_t *productID)
{
  strncpy((char *)productID, (char *)Obj->Product_ID, ES_WIFI_PRODUCT_ID_SIZE);
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  Return Firmware Revision.
  * @param  Obj: pointer to module handle
  * @param  productID: pointer Revision
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_GetFWRevID(ES_WIFIObject_t *Obj, uint8_t *FWRev)
{
  strncpy((char *)FWRev, (char *)Obj->FW_Rev, ES_WIFI_FW_REV_SIZE);
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  Return product Name.
  * @param  Obj: pointer to module handle
  * @param  productID: pointer product Name
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_GetProductName(ES_WIFIObject_t *Obj, uint8_t *productName)
{
  strncpy((char *)productName, (char *)Obj->Product_Name, ES_WIFI_PRODUCT_NAME_SIZE);
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  Return API revision.
  * @param  Obj: pointer to module handle
  * @param  productID: pointer API revision.
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_GetAPIRev(ES_WIFIObject_t *Obj, uint8_t *APIRev)
{
  strncpy((char *)APIRev, (char *)Obj->API_Rev, ES_WIFI_API_REV_SIZE);
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  Return Stack Revision.
  * @param  Obj: pointer to module handle
  * @param  productID: pointer Stack Revision
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_GetStackRev(ES_WIFIObject_t *Obj, uint8_t *StackRev)
{
  strncpy((char *)StackRev, (char *)Obj->Stack_Rev, ES_WIFI_STACK_REV_SIZE);
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  Return RTOS Revision
  * @param  Obj: pointer to module handle
  * @param  productID: pointer RTOS Revision
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_GetRTOSRev(ES_WIFIObject_t *Obj, uint8_t *RTOSRev)
{
  strncpy((char *)RTOSRev, (char *)Obj->RTOS_Rev, ES_WIFI_RTOS_REV_SIZE);
  return ES_WIFI_STATUS_OK;
}


/**
  * @brief  Initialize WIFI module.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_RegisterBusIO(ES_WIFIObject_t *Obj, IO_Init_Func IO_Init,
                                                              IO_DeInit_Func  IO_DeInit,
                                                              IO_Delay_Func   IO_Delay,  
                                                              IO_Send_Func    IO_Send,
                                                              IO_Receive_Func  IO_Receive)
{
  if(!Obj || !IO_Init || !IO_DeInit || !IO_Send || !IO_Receive)
  {
    return ES_WIFI_STATUS_ERROR;
  }

  Obj->fops.IO_Init = IO_Init;
  Obj->fops.IO_Send = IO_Send;
  Obj->fops.IO_Receive = IO_Receive;
  Obj->fops.IO_Delay = IO_Delay;  
  
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  Change default Timeout.
  * @param  Obj: pointer to module handle
  * @param  Timeout: Timeout in mS
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_SetTimeout(ES_WIFIObject_t *Obj, uint32_t Timeout)
{
  Obj->Timeout = Timeout;
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  List all detected APs.
  * @param  Obj: pointer to module handle
  * @param  APs: pointer Access points structure
  * @retval Operation Status.
  */
ES_WIFI_Status_t  ES_WIFI_ListAccessPoints(ES_WIFIObject_t *Obj, ES_WIFI_APs_t *APs)
{  

  ES_WIFI_Status_t ret;

  ret = AT_ExecuteCommand(Obj,(uint8_t*)"F0\r", Obj->CmdData);
  if(ret == ES_WIFI_STATUS_OK)
  {
     AT_ParseAP((char *)Obj->CmdData, APs);
  }
  return ret;
}

/**
  * @brief  Join an Access point.
  * @param  Obj: pointer to module handle
  * @param  Ssid: the access point id.
  * @param  Password: the Access point password.
  * @param  SecType: Security type.
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_Connect(ES_WIFIObject_t *Obj, const char* SSID, 
                                         const char* Password,
                                         ES_WIFI_SecurityType_t SecType)
{
  ES_WIFI_Status_t ret;
  
  sprintf((char*)Obj->CmdData,"C1=%s\r", SSID);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"C2=%s\r", Password);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    
    if(ret == ES_WIFI_STATUS_OK)
    {
      Obj->Security = SecType;
      sprintf((char*)Obj->CmdData,"C3=%d\r", (uint8_t)SecType);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      
      if(ret == ES_WIFI_STATUS_OK)
      {
        sprintf((char*)Obj->CmdData,"C0\r");
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);  
        if(ret == ES_WIFI_STATUS_OK)
        {
           Obj->NetSettings.IsConnected = 1;
        }
      }    
    }
  }
  return ret;
}

/**
  * @brief  Check whether the module is connected to an access point.
  * @retval Operation Status.
  */
uint8_t ES_WIFI_IsConnected(ES_WIFIObject_t *Obj)
{
  return Obj->NetSettings.IsConnected;
}
/**
  * @brief  Disconnect from a network.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_Disconnect(ES_WIFIObject_t *Obj)
{
  sprintf((char*)Obj->CmdData,"CD\r"); 
  return  AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
}
/**
  * @brief  Return network settings.
  * @param  Obj: pointer to module handle
  * @param  Pointer to network setting structure.
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_GetNetworkSettings(ES_WIFIObject_t *Obj)
{
  ES_WIFI_Status_t ret;
  
  sprintf((char*)Obj->CmdData,"C?\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  
  if(ret == ES_WIFI_STATUS_OK)
  {
     AT_ParseConnSettings((char *)Obj->CmdData, &Obj->NetSettings);
  }  
  return ret;
}

/**
  * @brief  Configure and activate SoftAP.
  * @param  Obj: pointer to module handle
  * @param  ApConfig : Pointer to AP config structure.
  * @param  ip : AP IP address
  * @param  mac : AP MAC address
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_ActivateAP(ES_WIFIObject_t *Obj, ES_WIFI_APConfig_t *ApConfig)
{
  ES_WIFI_Status_t ret;

  sprintf((char*)Obj->CmdData,"AS=0,%s\r", ApConfig->SSID);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  if(ret == ES_WIFI_STATUS_OK)
  {
  
    sprintf((char*)Obj->CmdData,"A1=%c\r", (int)ApConfig->Security + '0');
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    if(ret == ES_WIFI_STATUS_OK)
    {
      
      sprintf((char*)Obj->CmdData,"A2=%s\r", ApConfig->Pass);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      if(ret == ES_WIFI_STATUS_OK)
      { 
        
        sprintf((char*)Obj->CmdData,"AC=%d\r", ApConfig->Channel);
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
        if(ret == ES_WIFI_STATUS_OK)
        { 
          
          sprintf((char*)Obj->CmdData,"AT=%d\r", ApConfig->MaxConnections);
          ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
          if(ret == ES_WIFI_STATUS_OK)
          { 
            sprintf((char*)Obj->CmdData,"A0\r");
            ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);             
            if(ret == ES_WIFI_STATUS_OK)
            { 
              if(strstr((char *)Obj->CmdData, "[AP     ]"))
              {           
                ret = ES_WIFI_STATUS_OK;
              }       
            }            
          }
        }
      }
    }
  }
  return ret;
}

/**
  * @brief  Get AP notification.
  * @param  Obj: pointer to module handle
  * @param  ip : Assigned ip address.
  * @param  ip : joind device mac address.
  * @retval AP State.
  */
ES_WIFI_APState_t ES_WIFI_WaitAPStateChange(ES_WIFIObject_t *Obj)
{
  ES_WIFI_APState_t ret = ES_WIFI_AP_NONE;
  char *ptr;
   
 #if (ES_WIFI_USE_UART == 1)   
  if(Obj->fops.IO_Receive(Obj->CmdData, 0, Obj->Timeout) > 0)
  {
    if(strstr((char *)Obj->CmdData, AT_ERROR_STRING))
    {
      return ES_WIFI_AP_ERROR;
    }
#else
    do
    {
      sprintf((char*)Obj->CmdData,"MR\r");
      if(AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData) != ES_WIFI_STATUS_OK)
      { 
        return ES_WIFI_AP_ERROR;
      }        
#endif
    else  if(strstr((char *)Obj->CmdData, "[AP DHCP]"))
    {
      ptr = strtok((char *)Obj->CmdData + 2, " ");
      ptr = strtok(NULL, " ");
      ptr = strtok(NULL, " "); 
      ptr = strtok(NULL, " "); 
      ParseMAC((char *)ptr, Obj->APSettings.MAC_Addr); 
      ptr = strtok(NULL, " "); 
      ptr = strtok(NULL, "\r");        
      ParseIP((char *)ptr, Obj->APSettings.IP_Addr); 
      ret = ES_WIFI_AP_ASSIGNED;
#if (ES_WIFI_USE_SPI == 1)          
      break;
#endif      
    }  
    else  if(strstr((char *)Obj->CmdData, "[JOIN   ]"))
    {
      ptr = strtok((char *)Obj->CmdData + 12, ",");
      strncpy((char *)Obj->APSettings.SSID, ptr, ES_WIFI_MAX_SSID_NAME_SIZE  );
      ptr = strtok(NULL, ",");     
      ParseIP((char *)ptr, Obj->APSettings.IP_Addr); 
      ret =  ES_WIFI_AP_JOINED;
#if (ES_WIFI_USE_SPI == 1)          
      break;
#endif        
    } 
#if (ES_WIFI_USE_UART == 1)      
    }
#else
    Obj->fops.IO_Delay(1000);
  } while (1);
#endif  
  return ret;
}

/**
  * @brief  retrn the MAC address of the es module.
  * @param  Obj: pointer to module handle
  * @param  mac: pointer to the MAC address array.
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_GetMACAddress(ES_WIFIObject_t *Obj, uint8_t *mac)
{
  ES_WIFI_Status_t ret ;
  char *ptr;
  
  sprintf((char*)Obj->CmdData,"Z5\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  if(ret == ES_WIFI_STATUS_OK)
  {
    ptr = strtok((char *)(Obj->CmdData + 2), "\r\n");    
    ParseMAC(ptr, mac) ;
  }           
  return ret;
}

/**
  * @brief  retrn the IP address of the es module.
  * @param  Obj: pointer to module handle
  * @param  mac: pointer to the IP address array.
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_GetIPAddress(ES_WIFIObject_t *Obj, uint8_t *ipaddr)
{
  memcpy(ipaddr, Obj->NetSettings.IP_Addr, 4);         
  return ES_WIFI_STATUS_OK;
}

/**
  * @brief  Set the MAC address of the es module.
  * @param  Obj: pointer to module handle
  * @param  mac: pointer to the MAC address array.
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_SetMACAddress(ES_WIFIObject_t *Obj, uint8_t *mac)
{
  ES_WIFI_Status_t ret ;
 
  sprintf((char*)Obj->CmdData,"Z4=%X:%X:%X:%X:%X:%X\r",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5] );
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"Z1\r");
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  }  
  return ret;
}

/**
  * @brief  Reset To factory defaults.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_ResetToFactoryDefault(ES_WIFIObject_t *Obj)
{
  ES_WIFI_Status_t ret ;
 
  sprintf((char*)Obj->CmdData,"Z0\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);       
  return ret;
}

/**
  * @brief  Reset the module.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_ResetModule(ES_WIFIObject_t *Obj)
{
  ES_WIFI_Status_t ret ;
  
  sprintf((char*)Obj->CmdData,"ZR\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);       
  return ret;
}

/**
  * @brief  Set Product Name.
  * @param  Obj: pointer to module handle
  * @param  ProductName : pointer to product name string
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_SetProductName(ES_WIFIObject_t *Obj, uint8_t *ProductName)
{
  ES_WIFI_Status_t ret ;
  
  sprintf((char*)Obj->CmdData,"ZN=%s\r",ProductName);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"Z1\r");
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  }
  return ret;
}

#if (ES_WIFI_USE_FIRMWAREUPDATE == 1)
/**
  * @brief  OTA Firmware Upgrade.
  * @param  Obj: pointer to module handle
  * @param  Upgrade link path
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_OTA_Upgrade(ES_WIFIObject_t *Obj, uint8_t *link)
{
  ES_WIFI_Status_t ret ;
  
  sprintf((char*)Obj->CmdData,"Z0=%d\r%s",strlen((char *)link), (char *)link);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  return ret;
}
#endif
#if (ES_WIFI_USE_UART == 1)
/**
  * @brief  Set UART Baud Rate.
  * @param  Obj: pointer to module handle
  * @param  UART BAud rate
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_SetUARTBaudRate(ES_WIFIObject_t *Obj, uint16_t BaudRate)
{
  ES_WIFI_Status_t ret ;
 
  sprintf((char*)Obj->CmdData,"U2=%d\r", BaudRate);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"U0\r");
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  }
  return ret;
}

/**
  * @brief  Get UART Configuration.
  * @param  Obj: pointer to module handle
  * @param  pointer to UART config structure
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_GetUARTConfig(ES_WIFIObject_t *Obj, ES_WIFI_UARTConfig_t *pconf)
{
  ES_WIFI_Status_t ret ;
 
  sprintf((char*)Obj->CmdData,"U?\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  if(ret == ES_WIFI_STATUS_OK)
  {
    AT_ParseUARTConfig((char *)Obj->CmdData, pconf);
  }
  return ret;
}
#endif

/**
  * @brief  Get System Configuration.
  * @param  Obj: pointer to module handle
  * @param  pointer to System config structure
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_GetSystemConfig(ES_WIFIObject_t *Obj, ES_WIFI_SystemConfig_t *pconf)
{
  ES_WIFI_Status_t ret ;
 
  sprintf((char*)Obj->CmdData,"Z?\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
  if(ret == ES_WIFI_STATUS_OK)
  {
    AT_ParseSystemConfig((char *)Obj->CmdData, pconf);
  }
  return ret;
}

#if (ES_WIFI_USE_PING == 1)
/**
  * @brief  Ping an IP address.
  * @param  Obj: pointer to module handle
  * @param  Ping: ping structure.
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_Ping(ES_WIFIObject_t *Obj, uint8_t *address, uint16_t count, uint16_t interval_ms)
{
  ES_WIFI_Status_t ret;
 
  sprintf((char*)Obj->CmdData,"T1=%d.%d.%d.%d\r", address[0],address[1],
          address[2],address[3]);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  
  if(ret == ES_WIFI_STATUS_OK)
  {
    
    sprintf((char*)Obj->CmdData,"T2=%d\r", count);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    
    if(ret == ES_WIFI_STATUS_OK)
    {
      sprintf((char*)Obj->CmdData,"T3=%d\r", interval_ms);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      
      if(ret == ES_WIFI_STATUS_OK)
      {
        sprintf((char*)Obj->CmdData,"T0=\r");
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);         
      }
    }
  }
  
  return ret;
}
#endif
/**
  * @brief  DNS Lookup to get IP address .
  * @param  Obj: pointer to module handle
  * @param  url: Domain Name.
  * @param  ipaddress: IP address.
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_DNS_LookUp(ES_WIFIObject_t *Obj, const char *url, uint8_t *ipaddress)
{
  ES_WIFI_Status_t ret;
  char *ptr;
  
  sprintf((char*)Obj->CmdData,"D0=%s\r", url);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  
  if(ret == ES_WIFI_STATUS_OK)
  {
    ptr = strtok((char *)Obj->CmdData + 2, "\r");
    ParseIP(ptr, ipaddress);       
  } 
  return ret;
}


/**
  * @brief  Configure and Start a Client connection.
  * @param  Obj: pointer to module handle
  * @param  conn: pointer to the connection structure
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_StartClientConnection(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn)
{
  ES_WIFI_Status_t ret;

  sprintf((char*)Obj->CmdData,"P0=%d\r", conn->Number);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"P1=%d\r", conn->Type);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    if(ret == ES_WIFI_STATUS_OK)
    {
      if ((conn->Type == ES_WIFI_UDP_CONNECTION) && (conn->LocalPort > 0)) 
      {
        sprintf((char*)Obj->CmdData,"P2=%d\r", conn->RemotePort);
        if(AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData) == ES_WIFI_STATUS_ERROR)
        {
          return ES_WIFI_STATUS_ERROR;
        }
      }
      sprintf((char*)Obj->CmdData,"P4=%d\r", conn->RemotePort);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      
      if(ret == ES_WIFI_STATUS_OK)
      {
        sprintf((char*)Obj->CmdData,"P3=%d.%d.%d.%d\r", conn->RemoteIP[0],conn->RemoteIP[1],
                conn->RemoteIP[2],conn->RemoteIP[3]);
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
        if(ret == ES_WIFI_STATUS_OK)
        {
          sprintf((char*)Obj->CmdData,"P6=1\r");
          ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);         
        }          
      }           
    }
  }
  return ret;
}

/**
  * @brief  Stop Client connection.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_StopClientConnection(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn)
{
  ES_WIFI_Status_t ret;
  
  sprintf((char*)Obj->CmdData,"P0=%d\r", conn->Number);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  
  if(ret == ES_WIFI_STATUS_OK)
  {  
    sprintf((char*)Obj->CmdData,"P6=0\r");
    ret =  AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);     
  }
  return ret;
}

#if (ES_WIFI_USE_AWS == 1)
/**
  * @brief  Configure and Start a AWS Client connection.
  * @param  Obj: pointer to module handle
  * @param  conn: pointer to the connection structure
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_StartAWSClientConnection(ES_WIFIObject_t *Obj, ES_WIFI_AWS_Conn_t *conn)
{
  
  ES_WIFI_Status_t ret;

  sprintf((char*)Obj->CmdData,"P0=%d\r", conn->Number);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"P1=%d\r", conn->Type);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    if(ret == ES_WIFI_STATUS_OK)
    {
      sprintf((char*)Obj->CmdData,"P4=%d\r", conn->RemotePort);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      
      if(ret == ES_WIFI_STATUS_OK)
      {
        sprintf((char*)Obj->CmdData,"PM=0,%s\r", conn->PublishTopic);
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
        if(ret == ES_WIFI_STATUS_OK)
        {
          if(ret == ES_WIFI_STATUS_OK)
          {
            sprintf((char*)Obj->CmdData,"PM=1,%s\r", conn->SubscribeTopic);
            ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
            if(ret == ES_WIFI_STATUS_OK)
            {
              
              sprintf((char*)Obj->CmdData,"PM=2,%d\r", conn->MQTTMode);
              ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
              if(ret == ES_WIFI_STATUS_OK)
              {
                sprintf((char*)Obj->CmdData,"PM=5,%s\r", conn->ClientID);
                ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
                if(ret == ES_WIFI_STATUS_OK)
                {
                  sprintf((char*)Obj->CmdData,"PM\r");
                  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
                  if(ret == ES_WIFI_STATUS_OK)
                  {
                    sprintf((char*)Obj->CmdData,"P6=1\r");
                    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);         
                  } 
                }
              }
            }
          }           
        }
      }
    }
  }
  return ret;
}
#endif
/**
  * @brief  Configure and Start a Server.
  * @param  Obj: pointer to module handle
  * @param  conn: pointer to the connection structure
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_StartServerSingleConn(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn)
{
  ES_WIFI_Status_t ret = ES_WIFI_STATUS_ERROR;
  char *ptr;
  
  sprintf((char*)Obj->CmdData,"PK=1,3000\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"P0=%d\r", conn->Number);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    if(ret == ES_WIFI_STATUS_OK)
    {
      sprintf((char*)Obj->CmdData,"P1=%d\r", conn->Type);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      if(ret == ES_WIFI_STATUS_OK)
      {
        sprintf((char*)Obj->CmdData,"P2=%d\r", conn->LocalPort);
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
        if(ret == ES_WIFI_STATUS_OK)
        {       
          sprintf((char*)Obj->CmdData,"P5=1\r");
          ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
          
          if(ret == ES_WIFI_STATUS_OK)
          {  
#if (ES_WIFI_USE_UART == 1)               
            if(Obj->fops.IO_Receive(Obj->CmdData, 0, Obj->Timeout) > 0)
            {
              if(strstr((char *)Obj->CmdData, "Accepted"))
              {
                ptr = strtok((char *)Obj->CmdData + 2, " ");
                ptr = strtok(NULL, " ");
                ptr = strtok(NULL, " "); 
                ptr = strtok(NULL, ":");            
                ParseIP((char *)ptr, conn->RemoteIP);          
                ret = ES_WIFI_STATUS_OK;
              }  
            }
#else
            do
            {
              sprintf((char*)Obj->CmdData,"MR\r");
              ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
              if(ret == ES_WIFI_STATUS_OK)
              {  
                if((strstr((char *)Obj->CmdData, "[SOMA]")) && (strstr((char *)Obj->CmdData, "[EOMA]")))
                {
                  if(strstr((char *)Obj->CmdData, "Accepted"))
                  {
                    ptr = strtok((char *)Obj->CmdData + 2, " ");
                    ptr = strtok(NULL, " ");
                    ptr = strtok(NULL, " "); 
                    ptr = strtok(NULL, ":");            
                    ParseIP((char *)ptr, conn->RemoteIP);          
                    ret = ES_WIFI_STATUS_OK;
                    break;
                  }  
                }
              }
              else
              {
                ret = ES_WIFI_STATUS_ERROR;
                break;
              }
              Obj->fops.IO_Delay(1000);
            } while (1);
#endif  
          }
        }
      }  
    }
  }
  return ret;
}

/**
  * @brief  Stop a Server.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_StopServerSingleConn(ES_WIFIObject_t *Obj)
{
  sprintf((char*)Obj->CmdData,"P5=0\r");
  return AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
}


/**
  * @brief  Configure and Start a Server.
  * @param  Obj: pointer to module handle
  * @param  conn: pointer to the connection structure
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_StartServerMultiConn(ES_WIFIObject_t *Obj, ES_WIFI_Conn_t *conn)
{
  ES_WIFI_Status_t ret = ES_WIFI_STATUS_ERROR;
  char *ptr;
  
  sprintf((char*)Obj->CmdData,"PK=1,3000\r");
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"P0=%d\r", conn->Number);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    if(ret == ES_WIFI_STATUS_OK)
    {
      sprintf((char*)Obj->CmdData,"P1=%d\r", conn->Type);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      if(ret == ES_WIFI_STATUS_OK)
      {
        sprintf((char*)Obj->CmdData,"P2=%d\r", conn->LocalPort);
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
        if(ret == ES_WIFI_STATUS_OK)
        {       
          sprintf((char*)Obj->CmdData,"P8=6\r");
          ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
          
          if(ret == ES_WIFI_STATUS_OK)
          {          
            sprintf((char*)Obj->CmdData,"P5=1\r");
            ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
            
            if(ret == ES_WIFI_STATUS_OK)
            { 
#if (ES_WIFI_USE_UART == 1)               
            if(Obj->fops.IO_Receive(Obj->CmdData, 0, Obj->Timeout) > 0)
            {
              if(strstr((char *)Obj->CmdData, "Accepted"))
              {
                ptr = strtok((char *)Obj->CmdData + 2, " ");
                ptr = strtok(NULL, " ");
                ptr = strtok(NULL, " "); 
                ptr = strtok(NULL, ":");            
                ParseIP((char *)ptr, conn->RemoteIP);          
                ret = ES_WIFI_STATUS_OK;
              }  
            }
#else
            do
            {
              sprintf((char*)Obj->CmdData,"MR\r");
              ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
              if(ret == ES_WIFI_STATUS_OK)
              {  
                if((strstr((char *)Obj->CmdData, "[SOMA]")) && (strstr((char *)Obj->CmdData, "[EOMA]")))
                {
                  if(strstr((char *)Obj->CmdData, "Accepted"))
                  {
                    ptr = strtok((char *)Obj->CmdData + 2, " ");
                    ptr = strtok(NULL, " ");
                    ptr = strtok(NULL, " "); 
                    ptr = strtok(NULL, ":");            
                    ParseIP((char *)ptr, conn->RemoteIP);          
                    ret = ES_WIFI_STATUS_OK;
                    break;
                  }  
                }
              }
              else
              {
                ret = ES_WIFI_STATUS_ERROR;
                break;
              }
              Obj->fops.IO_Delay(1000);
            } while (1);
#endif 
            }
            if(ret == ES_WIFI_STATUS_OK)
            {          
              sprintf((char*)Obj->CmdData,"P7=1\r");
              ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
              
            }
          }
        }
      }
    }
  }
  return ret;
}

/**
  * @brief  Stop a Server.
  * @param  Obj: pointer to module handle
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_StopServerMultiConn(ES_WIFIObject_t *Obj)
{
  ES_WIFI_Status_t ret = ES_WIFI_STATUS_ERROR;
  
  /* close the socket handle for the current request. */
  sprintf((char*)Obj->CmdData,"P7=2\r");
  ret =  AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  
  if(ret == ES_WIFI_STATUS_OK)
  {      
    /*Get the next request out of the queue */    
    sprintf((char*)Obj->CmdData,"P7=3\r");
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData); 
    if(ret == ES_WIFI_STATUS_OK)
    {          
      if(ret == ES_WIFI_STATUS_OK)
      { 
        if(Obj->fops.IO_Receive(Obj->CmdData, 0, Obj->Timeout) > 0)
        {
          if(strstr((char *)Obj->CmdData, "Accepted"))
          {
            ret = ES_WIFI_STATUS_OK;
          }  
        }
      }      
    }
  }
  return ret;       
}
/**
  * @brief  Send an amount data over WIFI.
  * @param  Obj: pointer to module handle
  * @param  Socket: number of the socket
  * @param  pdata: pointer to data
  * @param  len : length of the data to be sent
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_SendData(ES_WIFIObject_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Reqlen , uint16_t *SentLen , uint32_t Timeout)
{
  ES_WIFI_Status_t ret = ES_WIFI_STATUS_ERROR;  
  
  if(Reqlen >= ES_WIFI_PAYLOAD_SIZE ) Reqlen= ES_WIFI_PAYLOAD_SIZE;
  
  *SentLen = Reqlen;
  sprintf((char*)Obj->CmdData,"P0=%d\r", Socket);
  ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
  if(ret == ES_WIFI_STATUS_OK)
  {
    sprintf((char*)Obj->CmdData,"S2=%lu\r",Timeout);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    
    if(ret == ES_WIFI_STATUS_OK)
    {
      sprintf((char *)Obj->CmdData,"S3=%04d\r",Reqlen);
      ret = AT_RequestSendData(Obj, Obj->CmdData, pdata, Reqlen, Obj->CmdData);
      
      if(ret == ES_WIFI_STATUS_OK)
      {
        if(strstr((char *)Obj->CmdData,"-1\r\n"))
        {
          ret = ES_WIFI_STATUS_ERROR;
        }
      }
    }
  }
  
  if (ret == ES_WIFI_STATUS_ERROR) *SentLen = 0;
  return ret;  
}

/**
  * @brief  Receive an amount data over WIFI.
  * @param  Obj: pointer to module handle
  * @param  Socket: number of the socket
  * @param  pdata: pointer to data
  * @param  len : pointer to the length of the data to be received
  * @retval Operation Status.
  */
ES_WIFI_Status_t ES_WIFI_ReceiveData(ES_WIFIObject_t *Obj, uint8_t Socket, uint8_t *pdata, uint16_t Reqlen, uint16_t *Receivedlen, uint32_t Timeout)
{
  ES_WIFI_Status_t ret = ES_WIFI_STATUS_ERROR;  
  
  if(Reqlen <= ES_WIFI_PAYLOAD_SIZE )
  {
    sprintf((char*)Obj->CmdData,"P0=%d\r", Socket);
    ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
    
    if(ret == ES_WIFI_STATUS_OK)
    {
      sprintf((char*)Obj->CmdData,"R1=%d\r", Reqlen);
      ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
      if(ret == ES_WIFI_STATUS_OK)
      { 
        sprintf((char*)Obj->CmdData,"R2=%lu\r", Timeout);
        ret = AT_ExecuteCommand(Obj, Obj->CmdData, Obj->CmdData);
        if(ret == ES_WIFI_STATUS_OK)
        {  
         sprintf((char*)Obj->CmdData,"R0=\r");
          ret = AT_RequestReceiveData(Obj, Obj->CmdData, (char *)pdata, Reqlen, Receivedlen);
        }
      }
      else
      {
        *Receivedlen = 0;
      }
    }
  }
  return ret;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
