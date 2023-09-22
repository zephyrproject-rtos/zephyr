/**************************************************************************//**
 * @file     ucsi.h
 * @brief    Define UCSI's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/
#ifndef UCSI_H
#define UCSI_H

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_UCSI                                USR_SUPPORT_UCSI
#define UCSI_VERSION_MAJOR                          USR_UCSI_VERSION_MAJOR
#define UCSI_VERSION_MINOR                          USR_UCSI_VERSION_MINOR   
#define UCSI_VERSION_SUBMINOR                       USR_UCSI_VERSION_SUBMINOR
#define UCSI_MEM_ADDR                               USR_UCSI_MEM_ADDR
#define UCSI_NotificationID                         USR_UCSI_NotificationID
#define UCSI_ProcessUCSICommandID                   USR_UCSI_ProcessUCSICommandID

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define UCSI_DEBUG                                  DISABLE
//************************************************************
// UCSI Data Structure Define
//************************************************************
#define UCSIVERL                                    0x00
#define UCSIVERH                                    0x01
#define UCSIRSV0                                    0x02
#define UCSIRSV1                                    0x03
#define UCSICCICONNUM                               0x04
#define UCSICCIDLEN                                 0x05
#define UCSICCIRSV                                  0x06
#define UCSICCIINDICATOR                            0x07
#define UCSICTRLCMD                                 0x08
#define UCSICTRLDLEN                                0x09
#define UCSICTRLCMDSPEC0                            0x0A
#define UCSICTRLCMDSPEC1                            0x0B
#define UCSICTRLCMDSPEC2                            0x0C
#define UCSICTRLCMDSPEC3                            0x0D
#define UCSICTRLCMDSPEC4                            0x0E
#define UCSICTRLCMDSPEC5                            0x0F
#define UCSIMSGI00                                  0x10
#define UCSIMSGI15                                  0x1F
#define UCSIMSIO00                                  0x20
#define UCSIMSIO15                                  0x2F

#define UCSI_SIZE                                   0x30
#define UCSI_CTRL_CMDSPEC_SIZE                      0x06
#define UCSI_MSGI_SIZE                              0x10
#define UCSI_MSGO_SIZE                              0x10
//************************************************************
// UCSI Command List
//************************************************************
#define UCSI_CMD_PPM_RESET                          0x01
#define UCSI_CMD_CANCEL                             0x02
#define UCSI_CMD_CONNECTOR_RESET                    0x03
#define UCSI_CMD_ACK_CC_CI                          0x04
#define UCSI_CMD_SET_NOTIFICATION_ENABLE            0x05
#define UCSI_CMD_GET_CAPABILITY                     0x06
#define UCSI_CMD_GET_CONNECTOR_CAPABILITY           0x07
#define UCSI_CMD_SET_UOM                            0x08
#define UCSI_CMD_SET_UOR                            0x09
#define UCSI_CMD_SET_PDM                            0x0A
#define UCSI_CMD_SET_PDR                            0x0B
#define UCSI_CMD_GET_ALTERNATE_MODES                0x0C
#define UCSI_CMD_GET_CAM_SUPPORTED                  0x0D
#define UCSI_CMD_GET_CURRENT_CAM                    0x0E
#define UCSI_CMD_SET_NEW_CAM                        0x0F
#define UCSI_CMD_GET_PDOS                           0x10
#define UCSI_CMD_GET_CABLE_PROPERTY                 0x11
#define UCSI_CMD_GET_CONNECTOR_STATUS               0x12
#define UCSI_CMD_GET_ERROR_STATUS                   0x13
//************************************************************
// UCSI CCI Indicator Define
//************************************************************
#define UCSI_CCIFLAG_NOTSUPPORT                     0x02
#define UCSI_CCIFLAG_CANCEL                         0x04
#define UCSI_CCIFLAG_RESET                          0x08
#define UCSI_CCIFLAG_BUSY                           0x10
#define UCSI_CCIFLAG_ACK                            0x20
#define UCSI_CCIFLAG_ERROR                          0x40
#define UCSI_CCIFLAG_COMPLETE                       0x80
//************************************************************
// UCSI Notification List
//************************************************************
#define UCSI_NOTIFY_INIT                            0x0000
#define UCSI_NOTIFY_CMD_COMPLETE                    0x0001
#define UCSI_NOTIFY_EXT_SUPPLY_CHANGE               0x0002
#define UCSI_NOTIFY_POM_CHANGE                      0x0004

#define UCSI_NOTIFY_PROVIDER_CHANGE                 0x0020
#define UCSI_NOTIFY_NPL_CHANGE                      0x0040
#define UCSI_NOTIFY_PD_RESET_COMPLETE               0x0080
#define UCSI_NOTIFY_CAM_CHANGE                      0x0100
#define UCSI_NOTIFY_BC_CHANGE                       0x0200

#define UCSI_NOTIFY_CONNECTOR_PARTNER_CHANGE        0x0800
#define UCSI_NOTIFY_POWER_DIR_CHANGE                0x1000

#define UCSI_NOTIFY_CONNECT_CHANGE                  0x4000
#define UCSI_NOTIFY_ERROR                           0x8000
//************************************************************
// UCSI Command System Flag Define
//************************************************************
#define UCSI_SYSFLAG_INIT                           0x00
#define UCSI_SYSFLAG_ACTIVE                         0x01
#define UCSI_SYSFLAG_SCI_ISSUE_WAIT                 0x02

typedef struct 
{
    union
    {
        unsigned char   arrByte[UCSI_SIZE];
        struct{
            unsigned char   VERS[2];
            unsigned char   bRSV[2];
            struct{
                unsigned char   RSVBit0      : 1;
                unsigned char   bConnectorNum: 7;
                unsigned char   bDatLen;
                unsigned char   bRSV;
                unsigned char   bIndicator;
            }CCI;
            struct{
                unsigned char   bCmd;
                unsigned char   bDatLen;
                unsigned char   bCmdSpec[UCSI_CTRL_CMDSPEC_SIZE];
            }CTRL;
            unsigned char   MSGI[UCSI_MSGI_SIZE];
            unsigned char   MSGO[UCSI_MSGO_SIZE];
        }Field;
    }UCSI;
    unsigned short  wNotifyStatus;
    unsigned char   bSysFlag;
}_hwUCSI;

//-- Macro Define -----------------------------------------------
#define mUCSI_IsCmd(bCommand)                           (hwUCSI.UCSI.Field.CTRL.bCmd==(bCommand))
#define mUCSI_CTRLCmd_Get                               (hwUCSI.UCSI.Field.CTRL.bCmd)
#define mUCSI_IsCmdSupport                              ((mUCSI_CTRLCmd_Get>=UCSI_CMD_PPM_RESET)&&(mUCSI_CTRLCmd_Get<=UCSI_CMD_GET_ERROR_STATUS))
//************************************************************
// UCSI VERSION Indicator Define
//************************************************************
#define mUCSI_VERSION_Set(VERS_MAJOR, VERS_MINOR, VERS_SUBMINOR) {hwUCSI.UCSI.Field.VERS[1]=(VERS_MAJOR); hwUCSI.UCSI.Field.VERS[0]=((VERS_MINOR)<<4)|((VERS_SUBMINOR)&0x0F);}

#define mUCSI_IsCCIIndicator(Flag)                      (hwUCSI.UCSI.Field.CCI.bIndicator&  (Flag))
#define mUCSI_CCIIndicator_Set(Flag)                    hwUCSI.UCSI.Field.CCI.bIndicator|=  (Flag)
#define mUCSI_CCIIndicator_Clr(Flag)                    hwUCSI.UCSI.Field.CCI.bIndicator&=(~(Flag))
#define mUCSI_CCIIndicator_Config(Flag)                 hwUCSI.UCSI.Field.CCI.bIndicator =  (Flag)

#define mUCSI_NotifyStatus_Get                          hwUCSI.wNotifyStatus
#define mUCSI_NotifyStatus_Set(wStatus)                 hwUCSI.wNotifyStatus = (wStatus)
#define mUCSI_IsNotifyEnable(wNotifyID)                 (hwUCSI.wNotifyStatus&(wNotifyID))

#define mUCSI_IsSysFlag(bFlag)                          ((hwUCSI.bSysFlag&(bFlag))==(bFlag))
#define mUCSI_SysFlag_Set(bFlag)                        hwUCSI.bSysFlag|=  (bFlag)
#define mUCSI_SysFlag_Clr(bFlag)                        hwUCSI.bSysFlag&=(~(bFlag))
#define mUCSI_SysFlag_Config(bFlag)                     hwUCSI.bSysFlag =  (bFlag)

#define mUCSI_MSGINPtr_Get                              (hwUCSI.UCSI.Field.MSGI)
#define mUCSI_CCILenPtr_Get                             (&hwUCSI.UCSI.Field.CCI.bDatLen)
#define mUCSI_CCILenPtr_Set(bLen)                       hwUCSI.UCSI.Field.CCI.bDatLen = (bLen)
#define mUCSI_CCIConnectorNum_Set(bNum)                 hwUCSI.UCSI.Field.CCI.bConnectorNum = (bNum)

//************************************************************
// UCSI Command Argument Define
//************************************************************
//* UCSI_CMD_PPM_RESET ***************************************
//* UCSI_CMD_CANCEL ******************************************
//* UCSI_CMD_CONNECTOR_RESET *********************************
#define mUCSI_CMD_CONNECT_RESET_CONNECTOR_NUM           (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]& 0x7F)
#define mUCSI_CMD_CONNECT_RESET_HARD_RST                (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]>>7   )

//* UCSI_CMD_ACK_CC_CI ***************************************
#define mUCSI_CMD_ACK_CC_CI_ACK_TYPE                    (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]& 0x03)

//* UCSI_CMD_SET_NOTIFICATION_ENABLE *************************
#define mUCSI_CMD_SET_NOTIFICATION_ENABLE_NOTIFYSTA     ((unsigned short)(hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]<<8)|hwUCSI.UCSI.Field.CTRL.bCmdSpec[0])

//* UCSI_CMD_GET_CAPABILITY **********************************
//* UCSI_CMD_GET_CONNECTOR_CAPABILITY*************************
#define mUCSI_CMD_CONNECTOR_CAPABILITY_CONNECTOR_NUM    (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]& 0x7F)

//* UCSI_CMD_SET_UOM *****************************************
#define mUCSI_CMD_SET_UOM_CONNECTOR_NUM                 (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]& 0x7F)
#define mUCSI_CMD_SET_UOM_UOM                           ((hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]>>7)|((hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]&0x03)<<1))

//* UCSI_CMD_SET_UOR *****************************************
#define mUCSI_CMD_SET_UOR_CONNECTOR_NUM                 (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]& 0x7F)
#define mUCSI_CMD_SET_UOR_UOR                           ((hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]>>7)|((hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]&0x03)<<1))

//* UCSI_CMD_SET_PDM *****************************************
#define mUCSI_CMD_SET_PDM_CONNECTOR_NUM                 (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]& 0x7F)
#define mUCSI_CMD_SET_PDM_PDM                           ((hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]>>7)|((hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]&0x03)<<1))

//* UCSI_CMD_SET_PDR *****************************************
#define mUCSI_CMD_SET_PDR_CONNECTOR_NUM                 (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]& 0x7F)
#define mUCSI_CMD_SET_PDR_PDR                           ((hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]>>7)|((hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]&0x03)<<1))

//* UCSI_CMD_GET_ALTERNATE_MODES *****************************
#define mUCSI_CMD_GET_ALTERNATE_MODES_RECIPIENT         (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]&7)
#define mUCSI_CMD_GET_ALTERNATE_MODES_CONNECTOR_NUM     (hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]&0x7F)
#define mUCSI_CMD_GET_ALTERNATE_MODES_ALTMODE_OFFSET    (hwUCSI.UCSI.Field.CTRL.bCmdSpec[2])
#define mUCSI_CMD_GET_ALTERNATE_MODES_ALTMODE_NUM       (hwUCSI.UCSI.Field.CTRL.bCmdSpec[3]&0x03)

//* UCSI_CMD_GET_CAM_SUPPORTED *******************************
#define mUCSI_CMD_GET_CAM_SUPPORTED_CONNECTOR_NUM       (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]&0x7F)

//* UCSI_CMD_GET_CURRENT_CAM *********************************
#define mUCSI_CMD_GET_CURRENT_CAM_CONNECTOR_NUM         (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]&0x7F)

//* UCSI_CMD_SET_NEW_CAM *************************************
#define mUCSI_CMD_SET_NEW_CAM_CONNECTOR_NUM             (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]&0x7F)
#define mUCSI_CMD_SET_NEW_CAM_ENTER_OR_EXIT             (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]>>7  )
#define mUCSI_CMD_SET_NEW_CAM_NEW_CAM                   (hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]     )
#define mUCSI_CMD_SET_NEW_CAM_AM_SPECIFIC               (((unsigned long)hwUCSI.UCSI.Field.CTRL.bCmdSpec[5]<<24)|((unsigned long)hwUCSI.UCSI.Field.CTRL.bCmdSpec[4]<<16)|((unsigned short)hwUCSI.UCSI.Field.CTRL.bCmdSpec[3]<<8)|hwUCSI.UCSI.Field.CTRL.bCmdSpec[2])

//* UCSI_CMD_GET_PDOS ****************************************
#define mUCSI_CMD_GET_PDOS_CONNECTOR_NUM                (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]&0x7F)
#define mUCSI_CMD_GET_PDOS_PARTNER_PDO                  (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]>>7  )
#define mUCSI_CMD_GET_PDOS_PDO_OFFSET                   (hwUCSI.UCSI.Field.CTRL.bCmdSpec[1]     )
#define mUCSI_CMD_GET_PDOS_NUM_OF_PDO                   (hwUCSI.UCSI.Field.CTRL.bCmdSpec[2]&0x03)
#define mUCSI_CMD_GET_PDOS_SOURCE_OR_SINK               ((hwUCSI.UCSI.Field.CTRL.bCmdSpec[2]&0x04)>>2)

//* UCSI_CMD_GET_CABLE_PROPERTY ******************************
#define mUCSI_CMD_GET_CABLE_PROPERTY_CONNECTOR_NUM      (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]&0x7F)

//* UCSI_CMD_GET_CONNECTOR_STATUS ****************************
#define mUCSI_CMD_GET_CONNECTOR_STATUS_CONNECTOR_NUM    (hwUCSI.UCSI.Field.CTRL.bCmdSpec[0]&0x7F)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if SUPPORT_UCSI
extern _hwUCSI hwUCSI;

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void Display_UCSI_Data(void);
void UCSI_DataClr(unsigned char bStart, unsigned char bEnd);
unsigned char UCSI_NotifyProcess(unsigned short wNotifyID);
void UCSI_Init(void);
void UCSI_Handle_MainLoop(void);
#endif  //SUPPORT_UCSI

//-- For OEM Use -----------------------------------------------

#endif  //UCSI_H
