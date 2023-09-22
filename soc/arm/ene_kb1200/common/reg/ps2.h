/**************************************************************************//**
 * @file     ps2.h
 * @brief    Define PS2's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef PS2_H
#define PS2_H

/**
  \brief  Structure type to access PS/2 Controller (PS2).
 */
typedef volatile struct
{
  uint8_t  PS2CLKCR;                                  /*Port's Clock Control Register */
  uint8_t  Reserved0[3];                              /*Reserved */  
  uint8_t  PS2TXC;                                    /*Tx Control Register */
  uint8_t  Reserved1[3];                              /*Reserved */            
  uint8_t  PS2RXC;                                    /*Rx Control Register */
  uint8_t  Reserved2[3];                              /*Reserved */                    
  uint8_t  PS2IE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  PS2PF;                                     /*Event Pending Flag Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  PS2STA;                                    /*Status Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  PS2TXDATA;                                 /*Tx Data Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  PS2RXDATA;                                 /*Rx Data Register */
  uint8_t  Reserved7[3];                              /*Reserved */                     
  uint32_t PS2IOSTA;                                  /*Port IO Status Register */
  uint8_t  PS2SWRST;                                  /*SW Reset Register */
  uint8_t  Reserved8[3];                              /*Reserved */
  uint8_t  PS2OPT;                                    /*Option Register */
  uint8_t  Reserved9[3];                              /*Reserved */           
}  PS2_T;

#define ps2                ((PS2_T *) PS2_BASE)             /* PS2 Struct       */

#define PS2                ((volatile unsigned long  *) PS2_BASE)     /* PS2  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define PS2_V_OFFSET                    USR_PS2_V_OFFSET
#define PS2_SUPPORT                     USR_PS2_SUPPORT
#define SUPPORT_TP_ID_Identify          USR_SUPPORT_TP_ID_Identify
#define Check_AUX_Enable_Package        USR_Check_AUX_Enable_Package
#define SUPPORT_RSP_FC_WO_AUX           USR_SUPPORT_RSP_FC_WO_AUX
#define RSP_FC_AFTER_N_FE               USR_RSP_FC_AFTER_N_FE       //Default KBC response 0xFC after 3 times of 0xFE
//Port_Information
#define KEYBOARD_EXIST                  USR_KEYBOARD_EXIST          //For external KB
#define KEYBOARD_PORT_NO                USR_KEYBOARD_PORT_NO
#define TOUCHPAD_EXIST                  USR_TOUCHPAD_EXIST
#define TOUCHPAD_PORT_NO                USR_TOUCHPAD_PORT_NO

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define PS2_Port1_CLK_GPIO_Num          0x06//0x4A
#define PS2_Port1_DAT_GPIO_Num          0x07//0x4B
#define PS2_Port2_CLK_GPIO_Num          0x08//0x4C
#define PS2_Port2_DAT_GPIO_Num          0x09//0x4D
#define PS2_Port3_CLK_GPIO_Num          0x0F//0x4E
#define PS2_Port3_DAT_GPIO_Num          0x10//0x4F
//INT
#define RX_Byte                         0x01
#define TX_Byte                         0x02
//PS2_Status
#define PS2_Status_Start                0
#define PS2_Status_POST                 1
#define PS2_POST_Wait                   0x00
#define PS2_POST_Data_AA                0x10
#define PS2_POST_Data_00                0x20
#define PS2_POST_CMD_ED                 0x30
#define PS2_POST_Data_FA                0x40
#define PS2_POST_Next                   0x50
#define PS2_AUX_Intiate                 0x60
#define PS2_Status_Normal               2
#define PS2_Port_Num                    4
//intellimouse flag
#define PS2_AUX_Standard                0
#define PS2_AUX_4_byte                  1
#define PS2_AUX_5_button                2
#define Intellimouse_CMD_Size_4         6
#define Intellimouse_Read_ID_1          0x10
#define Intellimouse_Read_ID_2          0x20
#define Intellimouse_Read_ID_3          0x30
#define Intellimouse_Not                0xFF
//Port_Information
#define PS2_Primary_Device              0x80
#define PS2_AUX_Packet_3                0x30
#define PS2_AUX_Packet_4                0x40
#define PS2_5_button                    0x08
#define PS2_No_Device                   0
#define PS2_Keyboard_Device             1
#define PS2_AUX_Device                  2
#define PS2_AUX_Inhibit                 3
//command response
#define PS2_Device_Resend               0xFE
#define PS2_Device_Error                0xFC
#define PS2_Device_ACK                  0xFA
#define PS2_Device_AA                   0xAA
#define PS2_Device_00                   0x00
#define PS2_RspMax                      0x08    //it must be power of 2 and >= 8
//command
#define PS2_KB_LED_CMD                  0xED
#define PS2_AUX_Set_Scaling             0xE7
#define PS2_AUX_Set_Resolution          0xE8
#define PS2_AUX_Set_Stream              0xEA
#define PS2_AUX_Set_Wrap                0xEE
#define PS2_AUX_Set_Remote              0xF0
#define PS2_AUX_Read_ID                 0xF2
#define PS2_AUX_Set_Sampling            0xF3
#define PS2_AUX_ENABLE                  0xF4
#define PS2_AUX_Set_Standard            0xF6
#define PS2_Device_Reset                0xFF
//AUX data
#define PS2_AUX_Always_1                0x08
//PS2_KB_Status
#define PS2_No_LED                      0x00
#define PS2_LED_CMD                     0x40
#define PS2_LED_Parameter               0x80
#define PS2_LED_OK                      0xC0
//PS2_AUX_Status
#define PS2_AUX_STD                     0x80
#define PS2_AUX_STREAM                  0x40
#define PS2_AUX_WRAP                    0x20
#define PS2_AUX_REMOTE                  0x10
#define PS2_AUX_RES                     0x08
#define PS2_AUX_SAM                     0x04
#define PS2_AUX_SCA                     0x02
#define PS2_AUX_EN                      0x01
//PS2_Time_Out
#define PS2_NoTimeout                   0x00
#define PS2_CMD_OK                      0x01
#define PS2_First_Rps                   0x02
#define PS2_AUX_DATA                    0x03
#define PS2_Timeout                     0xff

//-- Macro Define -----------------------------------------------
#define mPS2_Check_TX_TO_Event          (ps2->PS2PF&0x04)
#define mPS2_Check_TX_Event             (ps2->PS2PF&0x02)
#define mPS2_Check_RX_Event             (ps2->PS2PF&0x01)
#define mPS2_Clr_TX_TO_Event            ps2->PS2PF = 0x04
#define mPS2_Clr_TX_Event               ps2->PS2PF = 0x02
#define mPS2_Clr_RX_Event               ps2->PS2PF = 0x01
#define mPS2_Get_RX_Data                (ps2->PS2RXDATA)
#define mPS2_Write_TX_Data(dat)         ps2->PS2TXDATA = dat
#define mDRIVE_ALL_PS2_CLK_LOW          ps2->PS2CLKCR &= ~0x0F
#define mRELEASE_ALL_PS2_CLK            ps2->PS2CLKCR |= 0x0F
#define mDRIVE_PS2_CLK_LOW(temp)        ps2->PS2CLKCR = ps2->PS2CLKCR & (~(0x01 << temp))
#define mRELEASE_PS2_CLK_ONLY(temp)     ps2->PS2CLKCR = (ps2->PS2CLKCR&0xF0) | (0x01 << temp)
#define mRELEASE_PS2_CLK(temp)          ps2->PS2CLKCR = ps2->PS2CLKCR | (0x01 << temp)
#define mLOAD_PS2_CLK                   ps2->PS2CLKCR = PS2PortEnable
#define mSTORE_PS2_CLK                  PS2PortEnable = ps2->PS2CLKCR & 0x0F
#define mPS2_No_CmdRsp                  ((PS2_Port_CmdRsp_Size[0]==0)&&(PS2_Port_CmdRsp_Size[1]==0)&&(PS2_Port_CmdRsp_Size[2]==0)&&(PS2_Port_CmdRsp_Size[3]==0))
//#define mPS2_Clear_Rxflag               ps2->PS2PF = 0x01
#define mPS2_Reset                      ps2->PS2SWRST = 0x01;
#define mPS2_IssueIntSource             __NVIC_SetPendingIRQ(PS2_IRQn)
#define mPS2_No_CMD_to_DO               ((OEMPS2Command==0)&&(Send_TP_CMD==0)&&mPS2_No_CmdRsp)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if PS2_SUPPORT
extern unsigned char PS2PortEnable;
extern unsigned char PS2_Port_CmdRsp_Size[PS2_Port_Num];
extern unsigned char PS2_Temp_CmdRsp_Type;
                    //bit[7:6] = port No. for AUX in multiplexing;
                    //bit[5:4] = 0(No temp buffer), 1(keyboard), 2(AUX device);
extern unsigned char PS2_Temp_CmdRsp_Buffer;
extern unsigned char PS2_Temp_CmdRsp_Port;
extern unsigned char PS2_Temp_Dat_Index;
extern unsigned char PS2_Temp_KB_Dat_Buffer;
extern unsigned char PS2_Temp_AUX_Dat_Buffer;
extern unsigned char PS2_RX_Data;
extern unsigned char Port_Information[PS2_Port_Num];  //PS2_Port_Num = 4
                    //bit7     = primary;
                    //bit[6:4] = data packet;
                    //bit3     = Intellimouse 4/5 buttons;
                    //bit[2:0] = 0(No device), 1(keyboard), 2(AUX device)
extern unsigned char OEMPS2Command;
extern unsigned char Send_TP_CMD;
extern unsigned char Disable_TP_OEM;
extern unsigned char PS2_Break_Code;
extern unsigned char PS2_CmdRsp_First;
                    //clear IBF when first response recevied by PS2
                    //bit[3~0] = port 3~0 active flag, 1 = active
extern unsigned char OEM_TP_RSP_Size;
#if Check_AUX_Enable_Package
extern unsigned char AUX_Enable_Status;
#endif  //Check_AUX_Enable_Package
extern unsigned char TP_ID_Info;
#endif  //PS2_SUPPORT

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
#if PS2_SUPPORT
void PS2_Init(void);
void PS2_GPIO_Enable(void);
void PS2_RX_TimeOut(void);
void PS2_RxData_to_Temp(unsigned char);
void Send_Command_To_TP_MainLoop(void);
void Handle_PS2_ISR(void);
void PS2_HIFRstInit(void);
void GPT2_PS2_Send_CMD_TP(void);
void GPT1_PS2_RX_TimeOut(void);
void Check_KBC_IBF_Set_Interrupt(void);
void KBC_Data_To_PS2_ISR(unsigned char,unsigned char);
void Check_PS2_Port_is_Normal(void);

//-- For OEM Use -----------------------------------------------
void Send_Command_To_TP(unsigned char);
void OEM_Disable_TP(void);
void OEM_Enable_TP(void);
unsigned char OEM_Send_CMD_ARG_To_TP(unsigned char,unsigned char);
#endif  //PS2_SUPPORT
#endif //PS2_H
