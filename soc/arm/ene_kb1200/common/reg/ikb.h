/**************************************************************************//**
 * @file     ikb.h
 * @brief    Define IKB's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef IKB_H
#define IKB_H

/**
  \brief  Structure type to access Internal Keyboard Controller (IKB).
 */
typedef volatile struct
{
  uint32_t IKBSCAN;                                   /*Scan Configuration Register */
  uint32_t IKBPIN;                                    /*Pin Configuration Register */  
  uint32_t Reserved0[2];                              /*Reserved */ 
  uint16_t IKBKGEN;                                   /*Key Generate Register */
  uint16_t Reserved1;                                 /*Reserved */      
  uint8_t  IKBPS2CFG;                                 /*PS/2 Configuration Register */
  uint8_t  Reserved2[3];                              /*Reserved */                       
  uint8_t  IKBIE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  IKBPF;                                     /*Event Pending Flag Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  IKBCR;                                     /*Control Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  IKBMK;                                     /*Make Key Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  IKBBK;                                     /*Break Key Register */
  uint8_t  Reserved7[3];                              /*Reserved */
  uint8_t  IKBSADR;                                   /*Scan Address Register */
  uint8_t  Reserved8[3];                              /*Reserved */
  uint8_t  IKBTXDAT;                                  /*Tx Data Register */
  uint8_t  Reserved9[3];                              /*Reserved */                                        
  uint8_t  IKBRXDAT;                                  /*Rx Data Register */
  uint8_t  Reserved10[3];                             /*Reserved */ 
  uint8_t  IKBLED;                                    /*LED Control Register */
  uint8_t  Reserved11[3];                             /*Reserved */         
  uint8_t  IKBTYPEC;                                  /*Typematic Control Register */
  uint8_t  Reserved12[3];                             /*Reserved */
  uint32_t Reserved13[20];                            /*Reserved */
  uint8_t  IKBSCANINF[32];                            /*Scan Information Register */        
  uint8_t  IKBKEYINF[32];                             /*Key Information Register */                   
}  IKB_T;

#define ikb                ((IKB_T *) IKB_BASE)             /* IKB Struct       */

#define IKB                ((volatile unsigned long  *) IKB_BASE)     /* IKB  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define IKB_V_OFFSET                USR_IKB_V_OFFSET
#define IKB_EXIST                   USR_IKB_EXIST
#define IKB_MK_BK_From_OTHER_DEVICE USR_IKB_MK_BK_From_OTHER_DEVICE     // 0: from IKB; 1: from other device and IKB will not scan and generate key
#define IKB_SCROLOCK_SUPPORT        USR_IKB_SCROLOCK_SUPPORT            // 0: No Support; 1: Support
#define IKB_SCROLOCK_GPIO_Num       USR_IKB_SCROLOCK_GPIO_Num           // 0x00~0x7F
#define IKB_NUMLOCK_SUPPORT         USR_IKB_NUMLOCK_SUPPORT             // 0: No Support; 1: Support
#define IKB_NUMLOCK_GPIO_Num        USR_IKB_NUMLOCK_GPIO_Num            // 0x00~0x7F
#define IKB_CAPSLOCK_SUPPORT        USR_IKB_CAPSLOCK_SUPPORT            // 0: No Support; 1: Support
#define IKB_CAPSLOCK_GPIO_Num       USR_IKB_CAPSLOCK_GPIO_Num           // 0x00~0x7F
#define IKB_FNLOCK_EQ_NUMLOCK       USR_IKB_FNLOCK_EQ_NUMLOCK           // 0: Num Lock!=Fn; 1: Num Lock=Fn

#define IKB_CONVERT_MATRIXVALUE     USR_IKB_CONVERT_MATRIXVALUE         // 0: SET2_CODE(for PS/2 or KBC); 1: HID_CODE(for I2C or USB)
#define IKB_KEY_TO_KBC              USR_IKB_KEY_TO_KBC                  // 0: To PS/2; 1: To KBC

#define IKB_Mode                    USR_IKB_Mode                        // 0: Standard mode; 1: NKRO mode
#define Standard_Mode               0
#define NKRO_Mode                   1
//#define IKB_DAC_Voltage             USR_IKB_NKRO_DAC_Voltage
//#define IKB_RELEASE_TIME            ((IKB_Mode==Standard_Mode)?USR_IKB_STD_RELEASE_TIME:USR_IKB_NKRO_RELEASE_TIME)
//#define IKB_DRIVELOW_TIME           ((IKB_Mode==Standard_Mode)?USR_IKB_STD_DRIVELOW_TIME:USR_IKB_NKRO_DRIVELOW_TIME)
//#define IKB_1st_REPEAT_SUP          USR_IKB_STD_1st_REPEAT_SUP
//#define IKB_TYPEMATIC_SUP           USR_IKB_STD_TYPEMATIC_SUP
//#define SUPPORT_IKB_KSI_NKROKSO_PIN ((IKB_Mode==Standard_Mode)?USR_SUPPORT_IKB_STD_KSI_PIN:USR_SUPPORT_IKB_NKRO_KSO_PIN)  
//#define SUPPORT_IKB_KSO_NKROKSI_PIN ((IKB_Mode==Standard_Mode)?USR_SUPPORT_IKB_STD_KSO_PIN:USR_SUPPORT_IKB_NKRO_KSI_PIN)             
//#define IKB_BREAK_DEB_CNT           ((IKB_Mode==Standard_Mode)?USR_IKB_STD_BREAK_DEB_CNT:USR_IKB_NKRO_BREAK_DEB_CNT)
//#define IKB_MAKE_DEB_CNT            ((IKB_Mode==Standard_Mode)?USR_IKB_STD_MAKE_DEB_CNT:USR_IKB_NKRO_MAKE_DEB_CNT)
//#define IKB_KEEPSCAN_GHOSTKEY       USR_IKB_STD_KEEPSCAN_GHOSTKEY
//#define IKB_EXPANSION_MAX_KeyNUM    ((IKB_Mode==Standard_Mode)?USR_IKB_STD_EXPANSION_MAX_KeyNUM:USR_IKB_NKRO_EXPANSION_MAX_KeyNUM)
//#define IKB_SPECIAL_KEY             USR_IKB_STD_SPECIAL_KEY
#define IKB_RELEASE_TIME             USR_IKB_STD_RELEASE_TIME        
#define IKB_DRIVELOW_TIME            USR_IKB_STD_DRIVELOW_TIME       
#define IKB_1st_REPEAT_SUP          USR_IKB_STD_1st_REPEAT_SUP
#define IKB_TYPEMATIC_SUP           USR_IKB_STD_TYPEMATIC_SUP
#define SUPPORT_IKB_KSI_NKROKSO_PIN  USR_SUPPORT_IKB_STD_KSI_PIN     
#define SUPPORT_IKB_KSO_NKROKSI_PIN  USR_SUPPORT_IKB_STD_KSO_PIN            
#define IKB_BREAK_DEB_CNT            USR_IKB_STD_BREAK_DEB_CNT       
#define IKB_MAKE_DEB_CNT             USR_IKB_STD_MAKE_DEB_CNT        
#define IKB_KEEPSCAN_GHOSTKEY       USR_IKB_STD_KEEPSCAN_GHOSTKEY
#define IKB_EXPANSION_MAX_KeyNUM     USR_IKB_STD_EXPANSION_MAX_KeyNUM
#define IKB_SPECIAL_KEY             USR_IKB_STD_SPECIAL_KEY     

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define IKB_KSI00_GPIO_Num              0x68//0x30
#define IKB_KSI01_GPIO_Num              0x69//0x31
#define IKB_KSI02_GPIO_Num              0x6A//0x32
#define IKB_KSI03_GPIO_Num              0x6B//0x33 
#define IKB_KSI04_GPIO_Num              0x6C//0x34 
#define IKB_KSI05_GPIO_Num              0x6D//0x35 
#define IKB_KSI06_GPIO_Num              0x6E//0x36 
#define IKB_KSI07_GPIO_Num              0x6F//0x37

#define IKB_KSO00_GPIO_Num              0x56//0x20
#define IKB_KSO01_GPIO_Num              0x57//0x21
#define IKB_KSO02_GPIO_Num              0x58//0x22
#define IKB_KSO03_GPIO_Num              0x59//0x23
#define IKB_KSO04_GPIO_Num              0x5A//0x24
#define IKB_KSO05_GPIO_Num              0x5B//0x25
#define IKB_KSO06_GPIO_Num              0x5C//0x26
#define IKB_KSO07_GPIO_Num              0x5D//0x27

#define IKB_KSO08_GPIO_Num              0x5E//0x28
#define IKB_KSO09_GPIO_Num              0x5F//0x29
#define IKB_KSO10_GPIO_Num              0x60//0x2A
#define IKB_KSO11_GPIO_Num              0x61//0x2B
#define IKB_KSO12_GPIO_Num              0x62//0x2C
#define IKB_KSO13_GPIO_Num              0x63//0x2D
#define IKB_KSO14_GPIO_Num              0x64//0x2E
#define IKB_KSO15_GPIO_Num              0x65//0x2F

#define IKB_KSO16_NKROKSI16_GPIO_Num    0x66//0x48
#define IKB_KSO17_NKROKSI17_GPIO_Num    0x67//0x49
#define IKB_KSO18_NKROKSI18_GPIO_Num    0x75//0x4E
#define IKB_KSO19_NKROKSI19_GPIO_Num    0x02//0x4F

//#define IKB_KSO20_NKROKSI20_GPIO_Num    0x52
#define IKB_SCROLED                     0x01
#define IKB_NUMLED                      0x02
#define IKB_CAPSLED                     0x04
#define SET2_CODE                       0
#define HID_CODE                        1
#define IKB_PS2_Buffer_Length           16
#define Matrix_Buffer_Length            ((IKB_EXPANSION_MAX_KeyNUM==DISABLE)?8:32)
#define Make_Flag                       1
#define Break_Flag                      0
//Interrupt                             
#define MakeKey_Interrupt               0x01
#define BreakKey_Interrupt              0x02
#define Repeat_Timeout_Interrupt        0x08
#define TX_Finish_Interrupt             0x10
#define RX_Finish_Interrupt             0x20
#define Reset_CMD_Interrupt             0x40
#define LED_CMD_Interrupt               0x80
//BIT_8 ServiceIKBFlag;                 
#define MakeKey_INT                     bit0
#define BreakKey_INT                    bit1
//Matrix Value
#define MV_Empty_Key                    0xB3
#define MV_Hot_Key_Start                0xB4
#define MV_Hot_Key_End                  0xC7
#define MV_Fn_Lock_Key_Start            0xC8
#define MV_Fn_Lock_Key_End              0xDF
#define MV_Fn_Shift_Key_Start           0xE0
#define MV_Fn_Shift_Key_End             0xFF

//-- Macro Define -----------------------------------------------
#define mClear_IKB_PF(flag)			ikb->IKBPF = flag
#define mCheck_IKB_PF(flag)		    (ikb->IKBPF&flag)
#define mDisable_Repeat_Timer       {ikb->IKBIE &= ~0x08; mClear_IKB_PF(Repeat_Timeout_Interrupt);}
#define mEnable_Repeat_Timer        {ikb->IKBIE |= 0x08; mClear_IKB_PF(Repeat_Timeout_Interrupt);}
#define mDisable_IKB_HW_Scan        ikb->IKBSCAN &= ~1UL
#define mDisable_IKB_HW_Kgen        ikb->IKBKGEN &= ~0x0001
#define mIKB_Reset_LED_TYPEMATIC	{ikb->IKBTYPEC = 0x30; IKB_LED_Set(0);}
#define mIKB_STD_CMD_HW_ENABLE      ikb->IKBPS2CFG = 0x03
#define mIKB_IE_Set(ie)             ikb->IKBIE = ie
#define mIKB_Write_TX(dat)          ikb->IKBTXDAT = dat

#if IKB_MK_BK_From_OTHER_DEVICE
#define mMK_Temp                    MK_BK_OD_MA
#define mBK_Temp                    MK_BK_OD_MA
#define mEnable_IKB_HW_Scan
#define mEnable_IKB_HW_Kgen
#define mIKB_Debug_IKB_PF           (ikb->IKBPF|ServiceIKBFlag.Byte)
#define mClear_Make_Flag            ServiceIKBFlag.Bit.MakeKey_INT = 0
#define mClear_Break_Flag           ServiceIKBFlag.Bit.BreakKey_INT = 0
#define mCheck_IKB_Event_Finish     (ServiceIKBFlag.Byte&0x03)
#else
#define mMK_Temp                    ikb->IKBMK
#define mBK_Temp                    ikb->IKBBK
#define mEnable_IKB_HW_Kgen         ikb->IKBKGEN |= 0x0001
#define mEnable_IKB_HW_Scan         ikb->IKBSCAN |= 1UL
#define mIKB_Debug_IKB_PF           ikb->IKBPF
#define mClear_Make_Flag            mClear_IKB_PF(MakeKey_Interrupt)
#define mClear_Break_Flag           mClear_IKB_PF(BreakKey_Interrupt)
#define mCheck_IKB_Event_Finish     (mCheck_IKB_PF(BreakKey_Interrupt)||mCheck_IKB_PF(MakeKey_Interrupt))
#endif  //IKB_MK_BK_From_OTHER_DEVICE

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if IKB_EXIST
#if IKB_MK_BK_From_OTHER_DEVICE
extern volatile BIT_8 ServiceIKBFlag;
extern unsigned char MK_BK_OD_MA;
#endif  //IKB_MK_BK_From_OTHER_DEVICE
extern unsigned char IKB_KEY;
extern unsigned char IKB_Special_Key;
                    //bit7 : Left Ctrl
                    //bit6 : Left Shift
                    //bit5 : Left Alt
                    //bit4 : Left GUI
                    //bit3 : Right Ctrl
                    //bit2 : Right Shift
                    //bit1 : Right Alt
                    //bit0 : Right GUI
extern unsigned char IKBTXDAT_FULL;
extern unsigned char IKB_PS2_Buffer[IKB_PS2_Buffer_Length];      //IKB_PS2_Buffer_Length = 16
extern unsigned char IKB_PS2_Buffer_S;
extern unsigned char IKB_PS2_Buffer_E;
extern unsigned char IKB_PS2_Buffer_Full;
extern unsigned char New_Matrix_Buffer[Matrix_Buffer_Length];    //Matrix_Buffer_Length = 8 or 32
extern unsigned char Last_Matrix_address;
extern unsigned char Fn_Shift_Press;
extern unsigned char Fn_Lock_Press;
extern unsigned char Matrix_Value_Buffer[Matrix_Buffer_Length];  //Matrix_Buffer_Length = 8 or 32
extern unsigned char KB_Reset_CMD;

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void Handle_IKB(void);
unsigned char Transfer_MV_SET2(unsigned char, unsigned char);
#if IKB_KEY_TO_KBC
void IKB_Key_To_KBC_OB(void);
#endif  //IKB_KEY_TO_KBC

//-- For OEM Use -----------------------------------------------
void Write_IKB_PS2_Buffer(unsigned char);
void Ignore_IKB_PS2_Buffer(unsigned char);
void Handle_Reset_CMD(void);
void IKB_Init(void);
void IKB_GPIO_Enable(void);
void IKB_Disable_KSI_IE(void);
void IKB_Enable_KSI_IE(void);
void IKB_LED_Set(unsigned char led);
#if IKB_MK_BK_From_OTHER_DEVICE
unsigned char IKB_Create_Make_Break_Key(unsigned char, unsigned char);
#endif  //IKB_MK_BK_From_OTHER_DEVICE
#endif  //IKB_EXIST

#endif //IKB_H
