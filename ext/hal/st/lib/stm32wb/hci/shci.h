/**
 ******************************************************************************
 * @file    shci.h
 * @author  MCD Application Team
 * @brief   HCI command for the system channel
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SHCI_H
#define __SHCI_H

#ifdef __cplusplus
extern "C" {
#endif

  /* Includes ------------------------------------------------------------------*/
  /* Exported types ------------------------------------------------------------*/

  /* SYSTEM EVENT */
  typedef enum
  {
    WIRELESS_FW_RUNNING = 0x00,
    RSS_FW_RUNNING = 0x01,
  } SHCI_SysEvt_Ready_Rsp_t;

#define SHCI_EVTCODE                    ( 0xFF )
#define SHCI_SUB_EVT_CODE_BASE          ( 0x9200 )

  /**
   * THE ORDER SHALL NOT BE CHANGED TO GUARANTEE COMPATIBILITY WITH THE CPU1 DEFINITION
   */
  typedef enum
  {
    SHCI_SUB_EVT_CODE_READY =  SHCI_SUB_EVT_CODE_BASE,
  } SHCI_SUB_EVT_CODE_t;

  typedef PACKED_STRUCT{
    SHCI_SysEvt_Ready_Rsp_t sysevt_ready_rsp;
  } SHCI_C2_Ready_Evt_t;

  /* SYSTEM COMMAND */
  typedef PACKED_STRUCT
  {
    uint32_t MetaData[3];
  } SHCI_Header_t;

  typedef enum
  {
    SHCI_Success = 0x00,
    SHCI_Unknown_Command = 0x01,
    SHCI_ERR_UNSUPPORTED_FEATURE = 0x11,
    SHCI_ERR_INVALID_HCI_CMD_PARAMS = 0x12,
    SHCI_FUS_Command_Not_Supported = 0xFF,
  } SHCI_CmdStatus_t;

  typedef enum
  {
    SHCI_8bits =  0x01,
    SHCI_16bits = 0x02,
    SHCI_32bits = 0x04,
  } SHCI_Busw_t;

  typedef enum
  {
    erase_activity_OFF =  0x00,
    erase_activity_ON = 0x01,
  } SHCI_Erase_Activity_t;

#define SHCI_OGF                        ( 0x3F )
#define SHCI_OCF_BASE                   ( 0x50 )

  /**
   * THE ORDER SHALL NOT BE CHANGED TO GUARANTEE COMPATIBILITY WITH THE CPU2 DEFINITION
   */
  typedef enum
  {
    SHCI_OCF_C2_RESERVED1 =  SHCI_OCF_BASE,
    SHCI_OCF_C2_RESERVED2,
    SHCI_OCF_C2_FUS_GET_STATE,
    SHCI_OCF_C2_FUS_GET_UUID64,
    SHCI_OCF_C2_FUS_FIRMWARE_UPGRADE,
    SHCI_OCF_C2_FUS_REMOVE_WIRELESS_STACK,
    SHCI_OCF_C2_FUS_UPDATE_AUTHENTICATION_KEY,
    SHCI_OCF_C2_FUS_LOCK_AUTHENTICATION_KEY,
    SHCI_OCF_C2_FUS_WRITE_USER_KEY_IN_MEMORY,
    SHCI_OCF_C2_FUS_WRITE_USER_KEY_IN_AES,
    SHCI_OCF_C2_FUS_START_WIRELESS_STACK,
    SHCI_OCF_C2_FUS_UPGRADE,
    SHCI_OCF_C2_FUS_ABORT,
    SHCI_OCF_C2_FUS_RESERVED1,
    SHCI_OCF_C2_FUS_RESERVED2,
    SHCI_OCF_C2_FUS_RESERVED3,
    SHCI_OCF_C2_FUS_RESERVED4,
    SHCI_OCF_C2_FUS_RESERVED5,
    SHCI_OCF_C2_FUS_RESERVED6,
    SHCI_OCF_C2_FUS_RESERVED7,
    SHCI_OCF_C2_FUS_RESERVED8,
    SHCI_OCF_C2_FUS_RESERVED9,
    SHCI_OCF_C2_BLE_init,
    SHCI_OCF_C2_Thread_init,
    SHCI_OCF_C2_Debug_init,
    SHCI_OCF_C2_FLASH_erase_activity,
    SHCI_OCF_C2_Concurrent_Set_Mode,
    SHCI_OCF_C2_FLASH_store_data,
    SHCI_OCF_C2_FLASH_erase_data,
    SHCI_OCF_C2_RADIO_Allow_Low_Power,
    SHCI_OCF_C2_Mac_802_15_4_init,
  } SHCI_OCF_t;

#define SHCI_OPCODE_C2_FUS_GET_STATE         (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_GET_STATE)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_GET_UUID64         (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_GET_UUID64)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_FIRMWARE_UPGRADE   (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_FIRMWARE_UPGRADE)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_REMOVE_WIRELESS_STACK   (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_REMOVE_WIRELESS_STACK)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_UPDATE_AUTHENTICATION_KEY    (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_UPDATE_AUTHENTICATION_KEY)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_LOCK_AUTHENTICATION_KEY    (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_LOCK_AUTHENTICATION_KEY)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_WRITE_USER_KEY_IN_MEMORY    (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_WRITE_USER_KEY_IN_MEMORY)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_WRITE_USER_KEY_IN_AES     (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_WRITE_USER_KEY_IN_AES)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_START_WIRELESS_STACK     (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_START_WIRELESS_STACK)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_UPGRADE              (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_UPGRADE)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_ABORT                (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_ABORT)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED1            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED1)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED2            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED2)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED3            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED3)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED4            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED4)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED5            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED5)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED6            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED6)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED7            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED7)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED8            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED8)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_FUS_RESERVED9            (( SHCI_OGF << 10) + SHCI_OCF_C2_FUS_RESERVED9)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_BLE_INIT                 (( SHCI_OGF << 10) + SHCI_OCF_C2_BLE_init)
  /** THE ORDER SHALL NOT BE CHANGED    */
  typedef PACKED_STRUCT{
  uint8_t* pBleBufferAddress;   /**< NOT USED CURRENTLY */
  uint32_t BleBufferSize;       /**< Size of the Buffer allocated in pBleBufferAddress  */
  uint16_t NumAttrRecord;
  uint16_t NumAttrServ;
  uint16_t AttrValueArrSize;
  uint8_t NumOfLinks;
  uint8_t ExtendedPacketLengthEnable;
  uint8_t PrWriteListSize;
  uint8_t MblockCount;
  uint16_t AttMtu;
  uint16_t SlaveSca;
  uint8_t MasterSca;
  uint8_t LsSource;
  uint32_t MaxConnEventLength;
  uint16_t HsStartupTime;
  uint8_t ViterbiEnable;
  uint8_t LlOnly;
  uint8_t HwVersion;
  } SHCI_C2_Ble_Init_Cmd_Param_t;

  typedef PACKED_STRUCT{
    SHCI_Header_t Header;       /** Does not need to be initialized by the user */
    SHCI_C2_Ble_Init_Cmd_Param_t Param;
  } SHCI_C2_Ble_Init_Cmd_Packet_t;

  /** No response parameters*/

#define SHCI_OPCODE_C2_THREAD_INIT              (( SHCI_OGF << 10) + SHCI_OCF_C2_Thread_init)
/** No command parameters */
/** No response parameters*/

#define SHCI_OPCODE_C2_DEBUG_INIT              (( SHCI_OGF << 10) + SHCI_OCF_C2_Debug_init)
  /** Command parameters */
    typedef PACKED_STRUCT{
      uint8_t *pGpioConfig;
      uint8_t *pTracesConfig;
      uint8_t *pGeneralConfig;
      uint8_t GpioConfigSize;
      uint8_t TracesConfigSize;
      uint8_t GeneralConfigSize;
    } SHCI_C2_DEBUG_init_Cmd_Param_t;

    typedef PACKED_STRUCT{
      SHCI_Header_t Header;       /** Does not need to be initialized by the user */
      SHCI_C2_DEBUG_init_Cmd_Param_t Param;
    } SHCI_C2_DEBUG_Init_Cmd_Packet_t;
    /** No response parameters*/

#define SHCI_OPCODE_C2_FLASH_ERASE_ACTIVITY     (( SHCI_OGF << 10) + SHCI_OCF_C2_FLASH_erase_activity)
  /** Command parameters */
    typedef PACKED_STRUCT{
      SHCI_Erase_Activity_t   EraseActivity;
    } SHCI_C2_FLASH_Erase_Activity_Cmd_Param_t;

    typedef PACKED_STRUCT{
      SHCI_Header_t Header;       /** Does not need to be initialized by the user */
      SHCI_C2_FLASH_Erase_Activity_Cmd_Param_t Param;
    } SHCI_C2_FLASH_Erase_Activity_Cmd_Packet_t;
    /** No response parameters*/

#define SHCI_OPCODE_C2_CONCURRENT_SET_MODE          (( SHCI_OGF << 10) + SHCI_OCF_C2_Concurrent_Set_Mode)
/** command parameters */
    typedef enum
    {
      BLE_ENABLE,
      THREAD_ENABLE,
    } SHCI_C2_CONCURRENT_Mode_Param_t;

#define SHCI_OPCODE_C2_FLASH_STORE_DATA          (( SHCI_OGF << 10) + SHCI_OCF_C2_FLASH_store_data)
#define SHCI_OPCODE_C2_FLASH_ERASE_DATA          (( SHCI_OGF << 10) + SHCI_OCF_C2_FLASH_erase_data)
/** command parameters */
    typedef enum
    {
      BLE_IP,
      THREAD_IP,
    } SHCI_C2_FLASH_Ip_t;

#define SHCI_OPCODE_C2_RADIO_ALLOW_LOW_POWER      (( SHCI_OGF << 10) + SHCI_OCF_C2_RADIO_Allow_Low_Power)

#define SHCI_OPCODE_C2_MAC_802_15_4_INIT        (( SHCI_OGF << 10) + SHCI_OCF_C2_Mac_802_15_4_init)
/** No command parameters */
/** No response parameters*/

  /* Exported constants --------------------------------------------------------*/
  /* External variables --------------------------------------------------------*/
  /* Exported macros -----------------------------------------------------------*/
  /* Exported functions ------------------------------------------------------- */
  /**
  * SHCI_C2_FUS_Get_State
  * @brief Read the RSS
  *        When the wireless FW is running on the CPU2, the command returns SHCI_FUS_Command_Not_Supported
  *        When any RSS command is sent after the SHCI_FUS_Command_Not_Supported has been returned after sending one RSS command,
  *        the CPU2 switches on the RSS ( This reboots automatically the device )
  *
  * @param  None
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_FUS_Get_State( void );

  /**
  * SHCI_C2_BLE_Init
  * @brief Provides parameters and starts the BLE Stack
  *
  * @param  pCmdPacket : Parameters to be provided to the BLE Stack
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_BLE_Init( SHCI_C2_Ble_Init_Cmd_Packet_t *pCmdPacket );

  /**
  * SHCI_C2_THREAD_Init
  * @brief Starts the THREAD Stack
  *
  * @param  None
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_THREAD_Init( void );

  /**
  * SHCI_C2_DEBUG_Init
  * @brief Starts the Traces
  *
  * @param  None
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_DEBUG_Init( SHCI_C2_DEBUG_Init_Cmd_Packet_t *pCmdPacket );

  /**
  * SHCI_C2_FLASH_Erase_Activity
  * @brief Provides the information of the start and the end of a flash erase window on the CPU1
  *
  * @param  pCmdPacket: Start/End of erase activity
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_FLASH_Erase_Activity( SHCI_C2_FLASH_Erase_Activity_Cmd_Packet_t *pCmdPacket );

  /**
  * SHCI_C2_CONCURRENT_SetMode
  * @brief Enable/Disable Thread on CPU2 (M0+)
  *
  * @param  Mode: BLE or Thread enable flag
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_CONCURRENT_SetMode( SHCI_C2_CONCURRENT_Mode_Param_t Mode );

  /**
  * SHCI_C2_FLASH_StoreData
  * @brief Store Data in Flash
  *
  * @param  Ip: BLE or THREAD
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_FLASH_StoreData( SHCI_C2_FLASH_Ip_t Ip );

  /**
  * SHCI_C2_FLASH_EraseData
  * @brief Erase Data in Flash
  *
  * @param  Ip: BLE or THREAD
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_FLASH_EraseData( SHCI_C2_FLASH_Ip_t Ip );

  /**
  * SHCI_C2_RADIO_AllowLowPower
  * @brief Allow or forbid IP_radio (802_15_4 or BLE) to enter in low power mode.
  *
  * @param  Ip: BLE or 802_15_5
  * @param  FlagRadioLowPowerOn: True or false
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_RADIO_AllowLowPower( SHCI_C2_FLASH_Ip_t Ip,uint8_t  FlagRadioLowPowerOn);


  /**
  * SHCI_C2_MAC_802_15_4_Init
  * @brief Starts the MAC 802.15.4 on M0
  *
  * @param  None
  * @retval None
  */
  SHCI_CmdStatus_t SHCI_C2_MAC_802_15_4_Init( void );

  #ifdef __cplusplus
}
#endif

#endif /*__SHCI_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
