/**************************************************************************//**
 * @file     vbat.h
 * @brief    Define VBAT module's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef VBAT_H
#define VBAT_H

/**
  \brief  Structure type to access VCC0 Power Domain Module (VCC0).
 */
typedef struct
{
  uint8_t  BKUPSTS;                                   /*VBAT Battery-Backed Status Register */ 
  uint8_t  PASCR[127];                                /*VBAT Scratch Register */
  uint8_t  INTRCTR;                                   /*VBAT INTRUSION# Control register */ 
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t Reserved2[7];                              /*Reserved */
  uint8_t  MTCCFG;                                    /*Monotonic Counter Configure register */ 
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  MTCIE;                                     /*Monotonic Counter Interrupt register */ 
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  MTCPF;                                     /*Monotonic Counter Pending Flag register */ 
  uint8_t  Reserved5[3];                              /*Reserved */
  uint32_t MTCMAT;                                    /*Monotonic Counter Match register */ 
  uint32_t MTCCNT;                                    /*Monotonic Counter Value register */ 
} VBAT_T;

//#define vbat                                ((VBAT_T *) VBAT_BASE)                      /* VBAT Struct       */
//#define VBAT                                ((volatile unsigned long  *) VBAT_BASE)     /* VBAT  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_MTC                         USR_SUPPORT_MTC

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------

//-- Macro Define -----------------------------------------------
#define mGet_VCCFail_Flag                   (vbat->BKUPSTS&0x01)
#define mClear_VCCFail_Flag                 vbat->BKUPSTS = 0x01
#define mGet_VCC0Fail_Flag                  (vbat->BKUPSTS&0x02)
#define mClear_VCC0Fail_Flag                vbat->BKUPSTS = 0x02
#define mGet_IntruderAlarm_Flag             (vbat->BKUPSTS&0x40)
#define mClear_IntruderAlarm_Flag           vbat->BKUPSTS = 0x40
#define mGet_InvalidVBATRAM_Flag            (vbat->BKUPSTS&0x80)
#define mClear_InvalidVBATRAM_Flag          vbat->BKUPSTS = 0x80

#define mWrite_VBAT_Scratch_B(num,dat)      vbat->PASCR[num] = dat
#define mRead_VBAT_Scratch_B(num)           (vbat->PASCR[num])

#define mWrite_VBAT_Scratch(num,dat)        VBAT[num] = dat
#define mRead_VBAT_Scratch(num)             (VBAT[num])

#define mIntrusion_Detect_Enable()          vbat->INTRCTR |= 0x01
#define mIntrusion_Detect_Disable()         vbat->INTRCTR &=~0x01
#define mIntrusion_Function_Lock()          vbat->INTRCTR |= 0x02

#define mMTC_Enable                         vbat->MTCCFG |= 0x01 
#define mMTC_Disable                        vbat->MTCCFG &= ~0x01 
#define mMTC_Interrupt_Enable               vbat->MTCIE |= 0x01   
#define mMTC_Interrupt_Disable              vbat->MTCIE &= ~0x01  
#define mGet_MTC_PF                         (vbat->MTCPF&0x01)     
#define mClear_MTC_PF                       vbat->MTCPF = 0x01    
#define mMTC_MatchValue_Set(mv)             vbat->MTCMAT = mv     
#define mMTC_CountValue_Get                 vbat->MTCCNT          
#define mMTC_CountValue_Set(cv)             vbat->MTCCNT = cv     
#define mMTC_SecToCnt(Sec)                  (Sec)                 

#define mMTC_CountValue_Reset               {mMTC_Disable;mMTC_Enable;}
//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void VBAT_Init(void);
void EnESrvcMTC(void);

//-- For OEM Use -----------------------------------------------
void MTC_Init(unsigned long MTC_InitCnt,unsigned long MTC_TimeCnt, unsigned char IntEnable);
void MTC_Disable(void);

#endif //VBAT_H
