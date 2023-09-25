/**************************************************************************//**
 * @file     gpt.h
 * @brief    Define GPT's registers
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef GPT_H
#define GPT_H

/**
  \brief  Structure type to access General Purpose Timer (GPT).
 */

//typedef volatile struct
//{
//  uint8_t  GPTCFG;                                    /*Configuration Register */
//  uint8_t  Reserved0[3];                              /*Reserved */   
//  uint8_t  GPTPF;                                     /*Pending flag Register */
//  uint8_t  Reserved1[3];                              /*Reserved */
//  uint32_t Reserved2[2];                              /*Reserved */   
//  uint8_t  GPT0;                                      /*GPT0 Expiration Value Register */
//  uint8_t  Reserved3[3];                              /*Reserved */
//  uint8_t  GPT1;                                      /*GPT1 Expiration Value Register */
//  uint8_t  Reserved4[3];                              /*Reserved */
//  uint16_t GPT2;                                      /*GPT2 Expiration Value Register */
//  uint16_t Reserved5;                                 /*Reserved */
//  uint16_t GPT3;                                      /*GPT3 Expiration Value Register */
//  uint16_t Reserved6;                                 /*Reserved */                                     
//}  GPT_T;
//
//#define gpt                ((GPT_T *) GPT_BASE)             /* GPT Struct       */
//#define GPT                ((volatile unsigned long  *) GPT_BASE)     /* GPT  array       */

typedef struct
{
  __IOM uint16_t GPTCFG;                                    /*Configuration Register */
        uint16_t Reserved0;                                 /*Reserved */
  __IOM uint8_t  GPTPF;                                     /*Pending flag Register */
        uint8_t  Reserved1[3];                              /*Reserved */
  __IOM uint32_t GPTEV;                                     /*GPT0 Expiration Value Register */
        uint32_t Reserved2;                                 /*Reserved */
} GPT_T;

#define GPT0_BASE            (0x40070000UL)
#define GPT1_BASE            (0x40070010UL)
#define GPT2_BASE            (0x40070020UL)
#define GPT3_BASE            (0x40070030UL)
#define GPT4_BASE            (0x40070040UL)
#define GPT5_BASE            (0x40070050UL)
#define GPT6_BASE            (0x40070060UL)
#define GPT7_BASE            (0x40070070UL)

#define gpt0               ((GPT_T *) GPT0_BASE)             /* GPT Struct       */
#define gpt1               ((GPT_T *) GPT1_BASE)             /* GPT Struct       */
#define gpt2               ((GPT_T *) GPT2_BASE)             /* GPT Struct       */
#define gpt3               ((GPT_T *) GPT3_BASE)             /* GPT Struct       */
#define gpt4               ((GPT_T *) GPT4_BASE)             /* GPT Struct       */
#define gpt5               ((GPT_T *) GPT5_BASE)             /* GPT Struct       */
#define gpt6               ((GPT_T *) GPT6_BASE)             /* GPT Struct       */
#define gpt7               ((GPT_T *) GPT7_BASE)             /* GPT Struct       */

#define GPT                ((volatile unsigned long  *) GPT0_BASE)     /* GPT  array       */


//***************************************************************
// User Define                                                
//***************************************************************
#define GPT_V_OFFSET                USR_GPT_V_OFFSET 
#define SUPPORT_GPT0                ENABLE     
#define SUPPORT_GPT1                ENABLE     
#define SUPPORT_GPT2                ENABLE 
#define SUPPORT_GPT3                USR_SUPPORT_GPT3 
#define GPT3_uSecTime               USR_GPT3_uSecTime
#define SUPPORT_GPT4                USR_SUPPORT_GPT4     
    #define GPT4_uSecTime           USR_GPT4_uSecTime
#define SUPPORT_GPT5                USR_SUPPORT_GPT5     
    #define GPT5_uSecTime           USR_GPT5_uSecTime
#define SUPPORT_GPT6                USR_SUPPORT_GPT6     
    #define GPT6_uSecTime           USR_GPT6_uSecTime
#define SUPPORT_GPT7                USR_SUPPORT_GPT7     
    #define GPT7_uSecTime           USR_GPT7_uSecTime



//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define GPT0CFG_INIT_TABLE_OFFSET       0
#define GPT0PF_INIT_TABLE_OFFSET        1
#define GPT0_INIT_TABLE_OFFSET          2
#define GPT1CFG_INIT_TABLE_OFFSET       4
#define GPT1PF_INIT_TABLE_OFFSET        5
#define GPT1_INIT_TABLE_OFFSET          6
#define GPT2CFG_INIT_TABLE_OFFSET       8
#define GPT2PF_INIT_TABLE_OFFSET        9
#define GPT2_INIT_TABLE_OFFSET          10
#define GPT3CFG_INIT_TABLE_OFFSET       12
#define GPT3PF_INIT_TABLE_OFFSET        13
#define GPT3_INIT_TABLE_OFFSET          14
#define GPT4CFG_INIT_TABLE_OFFSET       16
#define GPT4PF_INIT_TABLE_OFFSET        17
#define GPT4_INIT_TABLE_OFFSET          18
#define GPT5CFG_INIT_TABLE_OFFSET       20
#define GPT5PF_INIT_TABLE_OFFSET        21
#define GPT5_INIT_TABLE_OFFSET          22
#define GPT6CFG_INIT_TABLE_OFFSET       24
#define GPT6PF_INIT_TABLE_OFFSET        25
#define GPT6_INIT_TABLE_OFFSET          26
#define GPT7CFG_INIT_TABLE_OFFSET       28
#define GPT7PF_INIT_TABLE_OFFSET        29
#define GPT7_INIT_TABLE_OFFSET          30

#define GPT0_uSecTime               2500
#define GPT1_uSecTime               7500
#define GPT2_uSecTime               300000

#define GPT0_Prescaler                  0
#define GPT1_Prescaler                  0
#define GPT2_Prescaler                  0
#define GPT3_Prescaler                  0
#define GPT4_Prescaler                  0
#define GPT5_Prescaler                  0
#define GPT6_Prescaler                  0
#define GPT7_Prescaler                  0

//-- Macro Define -----------------------------------------------
#define mGPT_mSecToGPT0Cnt(mSec)    (((((unsigned long)mSec)*1000)+(GPT0_uSecTime>>1))/GPT0_uSecTime)
#define mGPT_mSecToGPT1Cnt(mSec)    (((((unsigned long)mSec)*1000)+(GPT1_uSecTime>>1))/GPT1_uSecTime)
#define mGPT_mSecToGPT2Cnt(mSec)    (((((unsigned long)mSec)*1000)+(GPT2_uSecTime>>1))/GPT2_uSecTime)
#define mGPT_mSecToGPT3Cnt(mSec)    (((((unsigned long)mSec)*1000)+(GPT3_uSecTime>>1))/GPT3_uSecTime)

//#define mGPT_uSecToCnt(uSec)        ((uSec>>5)+((uSec>>4)&0x01)-1)  //uSec>=32

#define mGPT03_uSecToCnt(prescaler,uSec)        (((uSec*32.768/1000)/(prescaler+1))-1)//(((uSec*2/61)/(prescaler+1))-1)
#define mGPT47_uSecToCnt(prescaler,uSec)        ((uSec/(prescaler+1))-1)

#define mGPT_uSecToCnt(uSec)        ((uSec*2/61)-1)

#define mGPT0_INIT_CNT              mGPT_uSecToCnt(GPT0_uSecTime)
#define mGPT1_INIT_CNT              mGPT_uSecToCnt(GPT1_uSecTime)
#define mGPT2_INIT_CNT              mGPT_uSecToCnt(GPT2_uSecTime)
#define mGPT3_INIT_CNT              mGPT_uSecToCnt(GPT3_uSecTime)
#define mGPT4_INIT_CNT              mGPT_uSecToCnt(GPT0_uSecTime)
#define mGPT5_INIT_CNT              mGPT_uSecToCnt(GPT1_uSecTime)
#define mGPT6_INIT_CNT              mGPT_uSecToCnt(GPT2_uSecTime)
#define mGPT7_INIT_CNT              mGPT_uSecToCnt(GPT3_uSecTime)

//#define mGPTIMER0_INT_EN            mSETBIT0(gpt->GPTCFG)	//enable GPT0 interrupt
//#define mGPTIMER0_INT_DIS           mCLRBIT0(gpt->GPTCFG)	//disable GPT0 interrupt
//#define mGPTIMER1_INT_EN            mSETBIT1(gpt->GPTCFG)	//enable GPT1 interrupt
//#define mGPTIMER1_INT_DIS           mCLRBIT1(gpt->GPTCFG)	//disable GPT1 interrupt
//#define mGPTIMER2_INT_EN            mSETBIT2(gpt->GPTCFG)	//enable GPT2 interrupt
//#define mGPTIMER2_INT_DIS           mCLRBIT2(gpt->GPTCFG)	//disable GPT2 interrupt
//#define mGPTIMER3_INT_EN            mSETBIT3(gpt->GPTCFG)	//enable GPT3 interrupt
//#define mGPTIMER3_INT_DIS           mCLRBIT3(gpt->GPTCFG)	//disable GPT3 interrupt
//
//#define mGPTIMER0_INT_FLAG_CLR      gpt->GPTPF = 0x01		//Clear GPT0 interrupt flag, this flag must be cleared by software.
//#define mGPTIMER1_INT_FLAG_CLR      gpt->GPTPF = 0x02		//Clear GPT1 interrupt flag, this flag must be cleared by software.
//#define mGPTIMER2_INT_FLAG_CLR      gpt->GPTPF = 0x04		//Clear GPT2 interrupt flag, this flag must be cleared by software.
//#define mGPTIMER3_INT_FLAG_CLR      gpt->GPTPF = 0x08		//Clear GPT3 interrupt flag, this flag must be cleared by software.
//                                
//#define mGPTIMER0_INT_FLAG_GET      mGETBIT0(gpt->GPTPF)		
//#define mGPTIMER1_INT_FLAG_GET      mGETBIT1(gpt->GPTPF)		
//#define mGPTIMER2_INT_FLAG_GET      mGETBIT2(gpt->GPTPF)		
//#define mGPTIMER3_INT_FLAG_GET      mGETBIT3(gpt->GPTPF)
//
//#define mGPT_mSecToGPTCnt(mSec)    (((((unsigned long)mSec)*1000)+(GPT0_uSecTime>>1))/GPT0_uSecTime)

#define mGPTIMER_PRESCALER_SET(x,v) ((GPT_T*)x)->GPTCFG = ((GPT_T*)x)->GPTCFG&0x00FF|(v<<8)
#define mGPTIMER_INT_EN(x)          mSETBIT0(((GPT_T*)x)->GPTCFG)
#define mGPTIMER_INT_DIS(x)         mCLRBIT0(((GPT_T*)x)->GPTCFG)
                                
#define mGPTIMER_INT_FLAG_CLR(x)    ((GPT_T*)x)->GPTPF = 0x01
#define mGPTIMER_INT_FLAG_GET(x)    (((GPT_T*)x)->GPTPF)

#define mGPTIMER_ECNT_SET(x,bCNT)   ((GPT_T*)x)->GPTEV = bCNT

#define mGPTIMER_RESTART(x)          mSETBIT1(((GPT_T*)x)->GPTCFG)
//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
extern const unsigned long GPT_Init_Table[];

extern unsigned char GPT0_Counter;
extern unsigned char GPT0_Counter_Temp;
extern unsigned char GPT1_Counter;
extern unsigned char GPT1_Counter_Temp;

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use ------------------------------------------------
void GPT_Reg_Init(void);
void GPT_Init(GPT_T* gpt_n, unsigned char PRESCALER, unsigned long ExpirationValue);
//GPT.C
void EnESrvcGPTimer0(void);
void EnESrvcGPTimer1(void);
void EnESrvcGPTimer2(void);
void EnESrvcGPTimer3(void);
void EnESrvcGPTimer4(void);
void EnESrvcGPTimer5(void);
void EnESrvcGPTimer6(void);
void EnESrvcGPTimer7(void);
//-- For OEM Use -----------------------------------------------
void Initiate_GPT0(unsigned char);                          
void Initiate_GPT1(unsigned char);  
void Disable_Timer(unsigned char,unsigned char);            //first byte is for GPT1/2, last byte is for timer1/2          

#endif //GPT_H
