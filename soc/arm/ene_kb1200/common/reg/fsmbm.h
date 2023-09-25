/**************************************************************************//**
 * @file     fsmbm.h
 * @brief    Define FSMBM's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef FSMBM_H
#define FSMBM_H

/**
  \brief  Structure type to access Flexible SMBus Master (FSMBM).
 */
typedef volatile struct
{
  uint32_t FSMBMCFG;                                  /*Configuration Register */
  uint8_t  FSMBMIE;                                   /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  FSMBMPF;                                   /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  FSMBMFRT;                                  /*Protocol Control Register */
  uint8_t  Reserved2[3];                              /*Reserved */        
  uint16_t FSMBMPEC;                                  /*PEC Value Register */
  uint16_t Reserved3;                                 /*Reserved */                 
  uint8_t  FSMBMSTS;                                  /*Status Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  FSMBMADR;                                  /*Slave Address Register */
  uint8_t  Reserved5[3];                              /*Reserved */   
  uint8_t  FSMBMCMD;                                  /*Command Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  FSMBMDAT[32];                              /*Data Register */
  uint8_t  FSMBMPRTC_P;                               /*Protocol Register */
  uint8_t  FSMBMPRTC_C;                               /*Protocol  Register */
  uint16_t Reserved7;                                 /*Reserved */  
  uint8_t  FSMBMNADR;                                 /*HostNotify Slave Address Register */
  uint8_t  Reserved8[3];                              /*Reserved */
  uint16_t FSMBMNDAT;                                 /*HostNotify Data Register */
  uint16_t Reserved9;                                 /*Reserved */                                           
}  FSMBM_T;

#define fsmbm0                ((FSMBM_T *) FSMBM0_BASE)             /* FSMBM Struct       */
#define fsmbm1                ((FSMBM_T *) FSMBM1_BASE)             /* FSMBM Struct       */
#define fsmbm2                ((FSMBM_T *) FSMBM2_BASE)             /* FSMBM Struct       */
#define fsmbm3                ((FSMBM_T *) FSMBM3_BASE)             /* FSMBM Struct       */
#define fsmbm4                ((FSMBM_T *) FSMBM4_BASE)             /* FSMBM Struct       */
#define fsmbm5                ((FSMBM_T *) FSMBM5_BASE)             /* FSMBM Struct       */
#define fsmbm6                ((FSMBM_T *) FSMBM6_BASE)             /* FSMBM Struct       */
#define fsmbm7                ((FSMBM_T *) FSMBM7_BASE)             /* FSMBM Struct       */
#define fsmbm8                ((FSMBM_T *) FSMBM8_BASE)             /* FSMBM Struct       */
#define fsmbm9                ((FSMBM_T *) FSMBM9_BASE)             /* FSMBM Struct       */


//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
//#define FSMBM_NUM                       6
#define FSMBM_NUM                       10
#define FSMBM0                          0x00
#define FSMBM1                          0x01
#define FSMBM2                          0x02
#define FSMBM3                          0x03
#define FSMBM4                          0x04
#define FSMBM5                          0x05
#define FSMBM6                          0x06
#define FSMBM7                          0x07
#define FSMBM8                          0x08
#define FSMBM9                          0x09
#define FSMSM_NoMapping                 0xFE

#define FSMBM0_CLK_GPIO_Num             0x2C//0x44
#define FSMBM0_DAT_GPIO_Num             0x2D//0x45
#define FSMBM1_CLK_GPIO_Num             0x2E//0x46
#define FSMBM1_DAT_GPIO_Num             0x2F//0x47
#define FSMBM2_CLK_GPIO_Num             0x32//0x4A
#define FSMBM2_DAT_GPIO_Num             0x33//0x4B
#define FSMBM3_CLK_GPIO_Num             0x34//0x4C
#define FSMBM3_DAT_GPIO_Num             0x35//0x4D
#define FSMBM4_CLK_GPIO_Num             0x38//0x08
#define FSMBM4_DAT_GPIO_Num             0x39//0x0D
#define FSMBM5_CLK_GPIO_Num             0x4A//0x0B
#define FSMBM5_DAT_GPIO_Num             0x4B//0x0C
#define FSMBM6_CLK_GPIO_Num             0x4C//0x4A
#define FSMBM6_DAT_GPIO_Num             0x4D//0x4B
#define FSMBM7_CLK_GPIO_Num             0x50//0x4C
#define FSMBM7_DAT_GPIO_Num             0x51//0x4D
#define FSMBM8_CLK_GPIO_Num             0x70//0x08
#define FSMBM8_DAT_GPIO_Num             0x71//0x0D
#define FSMBM9_CLK_GPIO_Num             0x72//0x0B
#define FSMBM9_DAT_GPIO_Num             0x73//0x0C



//PROTOCOL
//H/W SMBUS Protocol 0x00~0x1F
#define SMBUS_QUICK_WRITE               0x02
#define SMBUS_QUICK_READ                0x03
#define SMBUS_SEND_BYTE                 0x04
#define SMBUS_RECEIVE_BYTE              0x05
#define SMBUS_WRITE_BYTE                0x06
#define SMBUS_READ_BYTE                 0x07
#define SMBUS_WRITE_WORD                0x08
#define SMBUS_READ_WORD                 0x09
#define SMBUS_WRITE_BLOCK               0x0A
#define SMBUS_READ_BLOCK                0x0B
#define SMBUS_WORD_PROCESS              0x0C
#define SMBUS_BLOCK_PROCESS             0x0D
#define SMBUS_HW_PROTOCOL_MAX_VALUE     0x1F
//I2C Protocol with 1 byte length 0x20~0x2F
#define I2C_WRITE_BLOCK                 0x20        
#define I2C_WRITE_BLOCK_CMD             0x22
#define I2C_READ_BLOCK                  0x21
#define I2C_READ_BLOCK_CNT              0x23
#define I2C_WRITE_READ                  0x24
#define I2C_WRITE_READ_CNT              0x25
#define I2C_1BL_PROTOCOL_MAX_VALUE      0x2F
//SMBUS3.0 Protocol with 1 byte length 0x30~0x3F
#define SMBUS30_WRITE_32                0x34
#define SMBUS30_READ_32                 0x35
#define SMBUS30_WRITE_64                0x38
#define SMBUS30_READ_64                 0x39
#define SMBUS30_BLOCK_PROCESS           0x3D
#define SMBUS30_PROTOCOL_MAX_VALUE      0x3F
//I2C Protocol with 2 bytes length 0x40~0x4F
#define I2C_READ_BLOCK_CNT2L            0x41                //The length includes itself for HIDoverI2C
#define I2C_WRITE_READ_CNT2L            0x42                //The length includes itself for HIDoverI2C
#define I2C_2BL_PROTOCOL_MAX_VALUE      0x4F
//Flexible Protocol 0x7F with 1 byte length
#define FLEXIBLE_PROTOCOL               0x7F

//Error code
#define FSMBM_NO_ERROR                  0x00
#define FSMBM_DEVICE_ADDR_NO_ACK        0x10
#define FSMBM_CMD_NO_ACK                0x12
#define FSMBM_DEVICE_DATA_NO_ACK        0x13
#define FSMBM_LOST_ARBITRATION          0x17
#define FSMBM_SMBUS_TIMEOUT             0x18
#define FSMBM_UNSUPPORTED_PRTC          0x19
#define FSMBM_SMBUS_BUSY                0x1A
#define FSMBM_STOP_FAIL                 0x1E
#define FSMBM_PEC_ERROR                 0x1F
//Packet Form
#define ___NONE                         0x00
#define ___STOP                         0x01
#define __PEC_                          0x02
#define __PEC_STOP                      0x03
#define _CNT__                          0x04
#define _CNT__STOP                      0x05
#define _CNT_PEC_                       0x06
#define _CNT_PEC_STOP                   0x07
#define CMD___                          0x08
#define CMD___STOP                      0x09
#define CMD__PEC_                       0x0A
#define CMD__PEC_STOP                   0x0B
#define CMD_CNT__                       0x0C
#define CMD_CNT__STOP                   0x0D
#define CMD_CNT_PEC_                    0x0E
#define CMD_CNT_PEC_STOP                0x0F

#define FLEXIBLE_CMD                    0x08
#define FLEXIBLE_CNT                    0x04
#define FLEXIBLE_PEC                    0x02
#define FLEXIBLE_STOP                   0x01
//HW
#define FSMBM_BufferSize                0x20
#define FSMBM_Write                     0x00
#define FSMBM_Read                      0x01
//hwFSMBM.Flag
#define FSMBM_BUSY                      0x80
#define FSMBM_R_PRTO                    0x40
#define FSMBM_WRITE_READ                0x20
#define FSMBM_TIMEOUT                   0x10
#define FSMBM_FIRST                     0x08
#define FSMBM_2BL                       0x04

//Timming for F/W Time Out
#define FSMBM_msTimeOut                 100     // Unit: ms, Range: 7.5~1912.5 , Base Clock: 7.5ms(GPT1), Max. deviation: 3.75ms
//Clock Setting = 1 / (1u + (1u * N))  //50% Duty Cycle(bit7 == 0)
#define FSMBM_CLK_1M                    0
#define FSMBM_CLK_500K                  1
#define FSMBM_CLK_333K                  2
#define FSMBM_CLK_250K                  3
#define FSMBM_CLK_200K                  4
#define FSMBM_CLK_167K                  5
#define FSMBM_CLK_143K                  6
#define FSMBM_CLK_125K                  7
#define FSMBM_CLK_111K                  8
#define FSMBM_CLK_100K                  9
#define FSMBM_CLK_91K                   10
#define FSMBM_CLK_83K                   11
#define FSMBM_CLK_71K                   13
#define FSMBM_CLK_63K                   15
#define FSMBM_CLK_50K                   19
#define FSMBM_CLK_40K                   24
#define FSMBM_CLK_30K                   32
#define FSMBM_CLK_20K                   49
#define FSMBM_CLK_10K                   99
//Other(bit7 == 1; bit6~4 = clock high length; bit3~0 = clock low length)
#define FSMBM_CLK_400K                  (0x12|0x80)

//-- Struction Define --------------------------------------------
typedef struct _FSMBM               //Size = 20
{
    unsigned char *pData;
    unsigned char *rData;
    unsigned short pLen;
    unsigned short rLen;  
    unsigned short Index;        
    unsigned char Protocol;     /*Bit 7  : PEC Support
                                  Bit 6-0: Protocol */
    unsigned char TimeOutCnt;
    unsigned char Flag;         /*Bit 7  : BUSY bit
                                  Bit 6  : 0: = Write,1: Read
                                  bit 5  : I2C_WRITE_READ or I2C_WRITE_READ_CNT protocol
                                  Bit 4  : FSMBM_TIMEOUT
                                  Bit 3  : First read packet for 2 bytes length
                                  Bit 2  : 2 bytes length
                                  Bit 1  : Reserved
                                  Bit 0  : Reserved */
}_hwFSMBM;

//-- Macro Define -----------------------------------------------
//Timming for F/W Time Out
#define mFSMBM_TimeOutCnt                   mGPT_mSecToGPT1Cnt(FSMBM_msTimeOut)
//Issue FSMBM interrupt source dependant on Controller
//#define mFSMBM_IssueIntSource               __NVIC_SetPendingIRQ(FSMBM_IRQn)
//FSMBM GPIO Enable/Disable
#define mFSMBM_Bus0_Enable                  {mGPIOFS_En(FSMBM0_CLK_GPIO_Num); mGPIOFS_En(FSMBM0_DAT_GPIO_Num); mGPIOIE_En(FSMBM0_CLK_GPIO_Num); mGPIOIE_En(FSMBM0_DAT_GPIO_Num);}
#define mFSMBM_Bus1_Enable                  {mGPIOFS_En(FSMBM1_CLK_GPIO_Num); mGPIOFS_En(FSMBM1_DAT_GPIO_Num); mGPIOIE_En(FSMBM1_CLK_GPIO_Num); mGPIOIE_En(FSMBM1_DAT_GPIO_Num);}
#define mFSMBM_Bus2_Enable                  {mGPIOFS_En(FSMBM2_CLK_GPIO_Num); mGPIOFS_En(FSMBM2_DAT_GPIO_Num); mGPIOIE_En(FSMBM2_CLK_GPIO_Num); mGPIOIE_En(FSMBM2_DAT_GPIO_Num);}
#define mFSMBM_Bus3_Enable                  {mGPIOFS_En(FSMBM3_CLK_GPIO_Num); mGPIOFS_En(FSMBM3_DAT_GPIO_Num); mGPIOIE_En(FSMBM3_CLK_GPIO_Num); mGPIOIE_En(FSMBM3_DAT_GPIO_Num);}
#define mFSMBM_Bus4_Enable                  {mGPIO38_ALT_SCL4; mGPIOFS_En(FSMBM4_CLK_GPIO_Num); mGPIOFS_En(FSMBM4_DAT_GPIO_Num); mGPIOIE_En(FSMBM4_CLK_GPIO_Num); mGPIOIE_En(FSMBM4_DAT_GPIO_Num);}
#define mFSMBM_Bus5_Enable                  {mGPIOFS_En(FSMBM5_CLK_GPIO_Num); mGPIOFS_En(FSMBM5_DAT_GPIO_Num); mGPIOIE_En(FSMBM5_CLK_GPIO_Num); mGPIOIE_En(FSMBM5_DAT_GPIO_Num);}
#define mFSMBM_Bus6_Enable                  {mGPIOFS_En(FSMBM6_CLK_GPIO_Num); mGPIOFS_En(FSMBM6_DAT_GPIO_Num); mGPIOIE_En(FSMBM6_CLK_GPIO_Num); mGPIOIE_En(FSMBM6_DAT_GPIO_Num);}
#define mFSMBM_Bus7_Enable                  {mGPIOFS_En(FSMBM7_CLK_GPIO_Num); mGPIOFS_En(FSMBM7_DAT_GPIO_Num); mGPIOIE_En(FSMBM7_CLK_GPIO_Num); mGPIOIE_En(FSMBM7_DAT_GPIO_Num);}
#define mFSMBM_Bus8_Enable                  {mGPIO71_ALT_SDA8; mGPIOFS_En(FSMBM8_CLK_GPIO_Num); mGPIOFS_En(FSMBM8_DAT_GPIO_Num); mGPIOIE_En(FSMBM8_CLK_GPIO_Num); mGPIOIE_En(FSMBM8_DAT_GPIO_Num);}
#define mFSMBM_Bus9_Enable                  {mGPIOFS_En(FSMBM9_CLK_GPIO_Num); mGPIOFS_En(FSMBM9_DAT_GPIO_Num); mGPIOIE_En(FSMBM9_CLK_GPIO_Num); mGPIOIE_En(FSMBM9_DAT_GPIO_Num);}

#define mFSMBM_Bus0_Disable                 {mGPIOFS_Dis(FSMBM0_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM0_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM0_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM0_DAT_GPIO_Num);}
#define mFSMBM_Bus1_Disable                 {mGPIOFS_Dis(FSMBM1_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM1_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM1_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM1_DAT_GPIO_Num);}
#define mFSMBM_Bus2_Disable                 {mGPIOFS_Dis(FSMBM2_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM2_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM2_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM2_DAT_GPIO_Num);}
#define mFSMBM_Bus3_Disable                 {mGPIOFS_Dis(FSMBM3_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM3_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM3_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM3_DAT_GPIO_Num);}
#define mFSMBM_Bus4_Disable                 {mGPIOFS_Dis(FSMBM4_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM4_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM4_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM4_DAT_GPIO_Num);}
#define mFSMBM_Bus5_Disable                 {mGPIOFS_Dis(FSMBM5_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM5_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM5_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM5_DAT_GPIO_Num);}
#define mFSMBM_Bus6_Disable                 {mGPIOFS_Dis(FSMBM6_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM6_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM6_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM6_DAT_GPIO_Num);}
#define mFSMBM_Bus7_Disable                 {mGPIOFS_Dis(FSMBM7_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM7_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM7_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM7_DAT_GPIO_Num);}
#define mFSMBM_Bus8_Disable                 {mGPIOFS_Dis(FSMBM8_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM8_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM8_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM8_DAT_GPIO_Num);}
#define mFSMBM_Bus9_Disable                 {mGPIOFS_Dis(FSMBM9_CLK_GPIO_Num); mGPIOFS_Dis(FSMBM9_DAT_GPIO_Num); mGPIOIE_Dis(FSMBM9_CLK_GPIO_Num); mGPIOIE_Dis(FSMBM9_DAT_GPIO_Num);}

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if SUPPORT_FSMBM0
extern _hwFSMBM hwFSMBM0;
#endif  //SUPPORT_FSMBM0
#if SUPPORT_FSMBM1
extern _hwFSMBM hwFSMBM1;
#endif  //SUPPORT_FSMBM1
#if SUPPORT_FSMBM2
extern _hwFSMBM hwFSMBM2;
#endif  //SUPPORT_FSMBM2
#if SUPPORT_FSMBM3
extern _hwFSMBM hwFSMBM3;
#endif  //SUPPORT_FSMBM3
#if SUPPORT_FSMBM4
extern _hwFSMBM hwFSMBM4;
#endif  //SUPPORT_FSMBM4
#if SUPPORT_FSMBM5
extern _hwFSMBM hwFSMBM5;
#endif  //SUPPORT_FSMBM5

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
#if SUPPORT_FSMBM
//-- Kernel Use ------------------------------------------------
void FSMBM_Timer(FSMBM_T* fsmbm);
void Handle_FSMBM_ISR(FSMBM_T* fsmbm);

//-- For OEM Use -----------------------------------------------
void          FSMBM_Init(FSMBM_T* fsmbm, unsigned char, unsigned char, unsigned char);
void          FSMBM_Disable(FSMBM_T* fsmbm);
void          FSMBM_GPIO_Enable(FSMBM_T* fsmbm);
void          FSMBM_GPIO_Disable(FSMBM_T* fsmbm);
void          FSMBM_ClockSet(FSMBM_T* fsmbm);
unsigned char FSMBM_BusyCheck(FSMBM_T* fsmbm);
unsigned char FSMBM_Transaction(FSMBM_T* fsmbm, unsigned char, unsigned char , unsigned char , unsigned short ,unsigned char*, unsigned short, unsigned char*, unsigned char );
#endif  //SUPPORT_FSMBM

#endif //FSMBM_H
