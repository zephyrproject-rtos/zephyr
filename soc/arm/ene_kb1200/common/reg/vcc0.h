/**************************************************************************//**
 * @file     vcc0.h
 * @brief    Define VCC0 module's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef VCC0_H
#define VCC0_H

/**
  \brief  Structure type to access VCC0 Power Domain Module (VCC0).
 */
typedef volatile struct
{
  uint16_t PAREGPLC;                                  /*VCC0 PLC Register */
  uint16_t PAREGFLAG;                                 /*VCC0 Flag Register */  
  uint8_t  PB8SCR;                                    /*Power Button Override Control Register */
  uint8_t  Reserved0[3];                              /*Reserved */    
//  uint32_t PASCR;                                     /*VCC0 Scratch Register */
//  uint32_t Reserved1[5];                              /*Reserved */
  uint32_t Reserved1[6];
  uint8_t  Reserved2;                                 /*Reserved */    
  uint8_t  IOSCCFG_T;                                 /*OSC32K Configuration Register */
  uint16_t Reserved3;                                 /*Reserved */ 
  uint32_t Reserved4[7];                              /*Reserved */
  uint8_t  HIBTMRCFG;                                 /*Hibernation Timer Configuration Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  HIBTMRIE;                                  /*Hibernation Timer Interrupt Enable Register */
  uint8_t  Reserved6[3];                              /*Reserved */   
  uint8_t  HIBTMRPF;                                  /*Hibernation Timer Pending Flag Register */
  uint8_t  Reserved7[3];                              /*Reserved */
  uint32_t HIBTMRMAT;                                 /*Hibernation Timer Match Value Register */
  uint32_t HIBTMRCNT;                                 /*Hibernation Timer count Value Register */                   
}  VCC0_T;

#define vcc0                ((VCC0_T *) VCC0_BASE)             /* VCC0 Struct       */

#define VCC0                ((volatile unsigned long  *) VCC0_BASE)     /* VCC0  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_PowerButton_RST         USR_SUPPORT_PowerButton_RST //When power button keep low over 12s, EC reset
    #define PowerButton_RST_Sec         USR_PowerButton_RST_Sec     //(4~35)

#define SUPPORT_HIBTMR                  USR_SUPPORT_HIBTMR

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
//#define HIBTMR_Assert_GPIO_Num          0x7A
#define HIBTMR_Assert_GPIO_Num          0x20

#define HIBTMR_CFG_Interrupt_Enable     0x01
#define HIBTMR_CFG_Interrupt_Disable    0x00
//#define HIBTMR_CFG_Assert_GPIO7A        0x02
//#define HIBTMR_CFG_Not_Assert_GPIO7A    0x00
#define HIBTMR_CFG_Assert_GPIO20        0x02
#define HIBTMR_CFG_Not_Assert_GPIO20    0x00

//-- Macro Define -----------------------------------------------
#define mGet_PBO_Flag                   (vcc0->PAREGFLAG&0x0001)
#define mClear_PBO_Flag                 vcc0->PAREGFLAG = 0x0001

#define mPBO_Disable                    vcc0->PB8SCR &= ~0x01
#define mPBO_Set_RST_Sec(sec)           vcc0->PB8SCR = (vcc0->PB8SCR&0x07)|((sec-4)<<3)|0x01

//#define mWrite_VBAT_Scratch(1,dat)        mWrite_VBAT_Scratch(1,dat)//vcc0->PASCR = dat
//#define mRead_VBAT_Scratch(1)              mRead_VBAT_Scratch(1)//vcc0->PASCR

#define mHIBTMR_Enable                  vcc0->HIBTMRCFG |= 0x01
#define mHIBTMR_Disable                 vcc0->HIBTMRCFG &= ~0x01
#define mHIBTMR_Timer_Start             vcc0->HIBTMRCFG |= 0x02
#define mHIBTMR_Timer_Stop              vcc0->HIBTMRCFG &= ~0x02
//#define mHIBTMR_Assert_GPIO7A           vcc0->HIBTMRCFG |= 0x04
//#define mHIBTMR_Not_Assert_GPIO7A       vcc0->HIBTMRCFG &= ~0x04
#define mHIBTMR_Assert_GPIO20           vcc0->HIBTMRCFG |= 0x04
#define mHIBTMR_Not_Assert_GPIO20       vcc0->HIBTMRCFG &= ~0x04
#define mHIBTMR_Counter_Reset           vcc0->HIBTMRCFG |= 0x80
#define mHIBTMR_Interrupt_Enable        vcc0->HIBTMRIE |= 0x01
#define mHIBTMR_Interrupt_Disable       vcc0->HIBTMRIE &= ~0x01
#define mGet_HIBTMR_PF                  (vcc0->HIBTMRPF&0x01)
#define mClear_HIBTMR_PF                vcc0->HIBTMRPF = 0x01
#define mHIBTMR_MatchValue_Set(mv)      vcc0->HIBTMRMAT = mv
#define mGet_HIBTMR_Count_Value         vcc0->HIBTMRCNT
#define mHIBTMR_SecToCnt(Sec)           (Sec<<7)
#define mHIBTMR_mSecToCnt(mSec)         (mHIBTMR_SecToCnt(mSec)/1000)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void VCC0_Init(void);
void EnESrvcHIBTMR(void);

//-- For OEM Use -----------------------------------------------
void HIBTMR_Init(unsigned long HIBTMR_TimeCnt, unsigned char Configuration);
void HIBTMR_Disable(void);

#endif //VCC0_H
