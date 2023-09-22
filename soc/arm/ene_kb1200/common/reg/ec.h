/**************************************************************************//**
 * @file     ec.h
 * @brief    Define EC's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef EC_H
#define EC_H

#define USR_HIF_ECEnable    1    //0: Disable; 1: Enable (Default)
#define USR_SMBUS_Base_Address                      0x80 


/**
  \brief  Structure type to access ECECECx (EC).
 */
typedef volatile struct
{
  uint32_t ECICFG;                                    /*Configuration Register */
  uint8_t  ECIIE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  ECIPF;                                    /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t Reserved2;                                 /*Reserved */
  uint8_t  ECISTS;                                   /*Status Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  ECICMD;                                   /*Command Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  ECIODP;                                   /*Output Data Port Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  ECIIDP;                                   /*Input Dtat Port Register */
  uint8_t  Reserved6[3];                              /*Reserved */ 
  uint8_t  ECISCID;                                  /*SCI ID Write Port Register */
  uint8_t  Reserved7[3];                              /*Reserved */                                       
}  EC_T;

//#define ec                ((EC_T *) ECI_BASE)             /* EC Struct       */
//#define EC                ((volatile unsigned long  *) ECI_BASE)     /* EC  array       */

////***************************************************************
//// User Define                                                
////***************************************************************
//// ACPI(62/66) Port Setting
//#define HIF_ECEnable                USR_HIF_ECEnable        //0: Disable; 1: Enable (Default)
//#define HIF_ECAddress               USR_HIF_ECAddress       //Base Address
//#define EC_V_OFFSET                 USR_EC_V_OFFSET
//#define SMBUS_Base_Address          USR_SMBUS_Base_Address
//#define SCI_MODE                    USR_SCI_MODE
//#define GPIO_SCI_PIN_NUM            USR_GPIO_SCI_PIN_NUM
//#define SCIPolarity                 USR_SCIPolarity
//#define SCITime                     USR_SCITime
//
////***************************************************************
////  Kernel Define                                             
////***************************************************************
////-- Constant Define -------------------------------------------
//#define EC_SUPPORT                          HIF_ECEnable
//
//#define EC_STD_CMD_SCI                      ENABLE          //Standard EC command generates SCI pulse
//#define EC_SCI_Queue_Length                 8
//
//#define EC_OBFStatus                        bit0
//#define EC_IBFStatus                        bit1

#define EC_Read_CMD                         0x80
#define EC_Write_CMD                        0x81
#define EC_Burst_Enable_CMD                 0x82
#define EC_Burst_Disable_CMD                0x83
#define EC_Query_CMD                        0x84

////-- Macro Define -----------------------------------------------
//#define mEC_OBF_Check                       (ec->ECISTS&0x01)   // 0: empty  1:full
//#define mEC_Event_IBF                       (ec->ECIPF&0x02)
//#define mEC_Event_OBF                       (ec->ECIPF&0x01)
//#define mClr_EC_IBF_Event                   ec->ECIPF = 0x02
//#define mClr_EC_OBF_Event                   ec->ECIPF = 0x01
//#define mHost_Access_EC_Cmd_Port            (ec->ECISTS&0x08)
//#define mGet_EC_Command                     ec->ECICMD
//#define mGet_EC_Data                        ec->ECIIDP
//#define mEC_Write_Data(dat)                 ec->ECIODP = dat
//#define mClr_EC_IBF                         ec->ECISTS &= (0x10|0x02)
////#define mEC_Gen_SCI(value)                  {ec->ECISCID = value; VW_Index6.Bit.SCI_Request = 1; EC_SCI_TimerOutCounter = 0;}
//#define mEC_Gen_SCI(value)                  ec->ECISCID = value; 
//
//
//#define mACPI_Gen_Int                       mEC_Gen_SCI(0)
//#define mGet_EC_STD_CMD_SCI                 (ec->ECICFG&0x0100)
//#define mEC_Burst_En                        ec->ECISTS = 0x10;
//#define mEC_Burst_Dis                       ec->ECISTS = 0x00;
//#define mClr_Query_Flag                     ec->ECISTS &= (0x10|0x20)
//
//
////***************************************************************
////  Extern Function Declare                                          
////***************************************************************
////-- Kernel Use -----------------------------------------------
//void EC_Init(void);
//#if EC_SUPPORT
//void EnESrvcEC(void);
//void Handle_EC_OBF(void);
//void Handle_EC_IBF(void);
//void EC_Standard_Cmd(unsigned char);
////void Handle_EC_ISR(void);
//void EC_Burst_TimeOut_ISR(void);
//void EC_ESPIVW_SCI_Check_MainLoop(void);
//void EC_ESPIVW_SMI_Check_MainLoop(void);
//void EC_SCI_Queue_ReCheck(void);
//void EC_IBF_ReCheck(void);
//
////-- For OEM Use -----------------------------------------------
//unsigned char Set_SCI_Queue(unsigned char);
//unsigned char Get_SCI_Queue(void);
//unsigned char Check_SCI_Queue(void);
//void          EC_Data_To_Host(unsigned char);
//unsigned char Read_EC_Space(unsigned char);
//void          Write_EC_Space(unsigned char);
//#endif //EC_SUPPORT

#endif //EC_H
