/**************************************************************************//**
 * @file     kbc.h
 * @brief    Define KBC's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef KBC_H
#define KBC_H

/**
  \brief  Structure type to access Keyboard Controller (KBC).
 */
typedef volatile struct
{
  uint16_t KBCCFG;                                    /*Configuration Register */
  uint16_t Reserved0;                                 /*Reserved */    
  uint8_t  KBCIE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  KBCPF;                                     /*Event Pending Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint32_t Reserved3;                                 /*Reserved */
  uint8_t  KBCSTS;                                    /*Status Register */
  uint8_t  Reserved4[3];                              /*Reserved */  
  uint8_t  KBCCMD;                                    /*Command Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  KBCODP;                                    /*Output Data Port Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  KBCIDP;                                    /*Input Data Port Register */
  uint8_t  Reserved7[3];                              /*Reserved */ 
  uint8_t  KBCCB;                                     /*Command Byte Register */
  uint8_t  Reserved8[3];                              /*Reserved */ 
  uint8_t  KBCDATH;                                   /*Write Data from H/W Register */
  uint8_t  Reserved9[3];                              /*Reserved */                                                    
}  KBC_T;

//#define kbc                ((KBC_T *) KBC_BASE)             /* KBC Struct       */
//#define KBC                ((volatile unsigned long  *) KBC_BASE)     /* KBC  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define KBC_V_OFFSET                USR_KBC_V_OFFSET
#define KBC_SUPPORT                 USR_KBC_SUPPORT
#define Multiplexing                USR_Multiplexing        //0 : Disable multiplexing mode; 1 : Enable
#define KBC_OB_ReadClr              USR_KBC_OB_ReadClr
#define VIRTUAL_TP                  USR_VIRTUAL_TP          //It will response all TP's CMDs to host
#define VIRTUAL_KB                  USR_VIRTUAL_KB          //It will response all KB's CMDs to host
#define SUPPORT_KBC_GA20            USR_SUPPORT_KBC_GA20
#define KBC_GPIO_Num_GA20           USR_KBC_GPIO_Num_GA20
#define SUPPORT_KBC_GA20_OD         USR_SUPPORT_KBC_GA20_OD
#define KBC_GA20_DEFAULT            USR_KBC_GA20_DEFAULT

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define PATCH_SIRQ_KBC_OBF_CNT_END  20                //20 * 2.5 = 50ms
#define PS2_Keyboard_Device         1
#define PS2_AUX_Device              2
#define PS2_Device_Resend           0xFE
//KCCB
#define Scan_Code_Conversion	    0x40
#define AUX_Device_Disable		    0x20
#define KB_Device_Disable		    0x10
#define KBC_System_Flag             0x04
#define IRQ12_Enable			    0x02
#define IRQ1_Enable				    0x01
//KBSTS
#define Parity_Error			    0x80
#define Time_Out				    0x40
#define	AUX_Flag				    0x20
#define Address_64				    0x08
#define Multiplexing_Error		    0x04
#define IBF						    0x02
#define OBF						    0x01
//INTC
#define OBF_Event				    0x01
#define IBF_Event				    0x02
//PS2M
#define Multiplexing_Bit		    0x80
#define No_Multiplexing			    0xFF
#define KBC_Password_Num		    0x08
#define KBC_A5_Doing			    0x01
#define KBC_A5_OK				    0x02
//KBCCMD
#define KBCCMD_20h				    0x01
#define KBCCMD_C0h				    0x02
#define KBCCMD_D0h				    0x04
//#define KBCCMD_D1h				    0x08
#define KBCCMD_D2h				    0x08
#define KBCCMD_D3h				    0x10
#define KBCCMD_E0h				    0x20
//#define KBCCMD_FEh				    0x80
//KBC_RAM Define
#define KBC_RAM_SIZE                31
#define KBC_RAM_READ_OFFSET         0x21
#define KBC_RAM_WRITE_OFFSET        0x61 
#define KBC_GPIO_KBRST              0x01
//BIT_8 kbcflag
#define KBC_OBFStatus               bit0
#define KBC_IBFStatus               bit1

//-- Macro Define -----------------------------------------------
#define mHost_Access_KBC_Cmd_Port   (kbc->KBCSTS&0x08)
#define mKBC_IBF_Status             (kbc->KBCSTS&0x02)
#define mKBC_OBF_Status             (kbc->KBCSTS&0x01)
#define mKBC_OBF_Empty              ((kbc->KBCSTS&0x01)==0)
#define mKBC_OBF_Clr                kbc->KBCSTS &= 0xFD
#define mKBC_IBF_Clr                kbc->KBCSTS &= 0xFE
#define mKBC_Gen_KBCRST             VW_Index6.Bit.RCIN_Request = 1
#define mKBC_IssueIntSource         __NVIC_SetPendingIRQ(KBC_IRQn)

#define mKBC_SetGA20Low             mGPIODO_Low(KBC_GPIO_Num_GA20)
#define mKBC_SetGA20High            mGPIODO_High(KBC_GPIO_Num_GA20)
#define mKBC_OUTPORT_GA20_Get       (mGPIODO_Get(KBC_GPIO_Num_GA20)<<1)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if KBC_SUPPORT
//extern volatile BIT_8 ServiceKBCFlag;
//extern BIT_8 kbcflag;
extern unsigned char KBCCmdByte;
extern unsigned char KBC_OEM_Flag;
extern unsigned char KB_wait_arg;
extern unsigned char AUX_wait_arg;
extern unsigned char PS2_AUX_Port_Disable;//bit[3:0] = port3~0 AUX En/Disable
extern unsigned char Multiplexing_Mode;
extern unsigned char Multiplexing_Mode_Enable;
extern unsigned char Multiplexing_Mode_State;
extern unsigned char Command_Device_Type;
extern unsigned char D4_Processing;//0:No, 1:Yes
extern unsigned char KB_F0_Processing;
extern unsigned char FW_Issue_KBD_ISR;
extern unsigned char KBC_Password_Install;
extern unsigned char KBC_Password_Enable;
extern unsigned char KBC_Password_Point;
extern unsigned char KBC_Password_Length;
extern unsigned char KBC_Password[KBC_Password_Num]; //KBC_Password_Num = 8
extern unsigned char KBC_RAM[KBC_RAM_SIZE];
#endif  //KBC_SUPPORT
#if VIRTUAL_TP
extern unsigned char AUXIKB_Response[4];
extern unsigned char AUXIKB_Response_P;
extern unsigned char AUXIKB_CMD;
extern unsigned char AUXIKB_Sample;
extern unsigned char AUXIKB_Resolution;
extern unsigned char AUXIKB_Status;
#endif  //VIRTUAL_TP
#if VIRTUAL_KB
extern unsigned char KBD_RSP_Buffer[4];
extern unsigned char KBD_RSP_Point;
extern unsigned char KBD_RSP_Counter;
extern unsigned char KBD_CMD_ARG;
extern unsigned char IKBLED_Temp;
extern unsigned char IKBTYPM_Temp;
extern unsigned char Enable_Report_Scancode;
#endif  //VIRTUAL_KB

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

void KBC_Init(void);
#if KBC_SUPPORT
void Handle_KBC(void);
void CLR_KBC_IBF(void);
void Set_Port_Clock(unsigned char);
void Handle_KBC_ISR(void);
void KBC_HIFRstInit(void);
void KBC_ESPIVW_RCIN_Check_MainLoop(void);
void KBC_IBF_ReCheck(void);
#if SUPPORT_KBC_GA20
void KBC_GA20_Init(void);
#endif //SUPPORT_KBC_GA20

//-- For OEM Use -----------------------------------------------
unsigned char KBC_Data_To_Host(unsigned char,unsigned char,unsigned char);
#endif  //KBC_SUPPORT
#if VIRTUAL_TP
void VTP_Command(void);
#endif  //VIRTUAL_TP
#if VIRTUAL_KB
void VKB_Command(void);
#endif  //VIRTUAL_KB
#endif //KBC_H
