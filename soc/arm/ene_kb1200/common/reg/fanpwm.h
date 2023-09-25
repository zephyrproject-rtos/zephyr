/**************************************************************************//**
 * @file     fanpwm.h
 * @brief    Define FANPWM's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef FANPWM_H
#define FANPWM_H

/**
  \brief  Structure type to access Fan PWM (FANPWM).
 */
typedef volatile struct
{
  uint16_t FPWM0CFG;                                  /*Configuration Register */
  uint16_t Reserved0;                                 /*Reserved */
  uint16_t FPWM0HIGH;                                 /*High Length Register */
  uint16_t Reserved1;                                 /*Reserved */  
  uint16_t FPWM0CYC;                                  /*Cycle Length Register */
  uint16_t Reserved2;                                 /*Reserved */                       
  uint32_t FPWM0CHC;                                  /*Current High/Cycle Length Register */
  uint16_t FPWM1CFG;                                  /*Configuration Register */
  uint16_t Reserved3;                                 /*Reserved */
  uint16_t FPWM1HIGH;                                 /*High Length Register */
  uint16_t Reserved4;                                 /*Reserved */  
  uint16_t FPWM1CYC;                                  /*Cycle Length Register */
  uint16_t Reserved5;                                 /*Reserved */                       
  uint32_t FPWM1CHC;                                  /*Current High/Cycle Length Register */   
}  FANPWM_T;

#define fanpwm                ((FANPWM_T *) FANPWM_BASE)             /* FANPWM Struct       */

#define FANPWM                ((volatile unsigned long  *) FANPWM_BASE)     /* FANPWM  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define FIXED_FANPWM_SUPPORT                USR_FIXED_FANPWM_SUPPORT

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define FANPWM_CHANNEL_NUM          2
                                 
#define FANPWM0_GPIO_Num            0x49//0x12
#define FANPWM1_GPIO_Num            0x74//0x13
                                 
#define FANPWM_CFG_OFFSET           0
#define FANPWM_HIGH_OFFSET          1
#define FANPWM_CYC_OFFSET           2
#define FANPWM_CHC_OFFSET           3
                                  
#define FANPWM_SOURCE_CLK_32M       0x0000
#define FANPWM_SOURCE_CLK_1M        0x4000
#define FANPWM_SOURCE_CLK_32_768K   0x8000
                                 
#define FANPWM_PRESCALER_BIT_S      8
                                 
#define FANPWM_RULE0                0x0000
#define FANPWM_RULE1                0x0080
                                  
#define FANPWM_PUSHPULL             0x0000
#define FANPWM_OPENDRAIN            0x0002
                                
//-- Macro Define -----------------------------------------------
#define mFANPWM_Enable(Channel)                         FANPWM[FANPWM_CFG_OFFSET+(Channel<<2)] |= 0x01
#define mFANPWM_Disable(Channel)                        FANPWM[FANPWM_CFG_OFFSET+(Channel<<2)] &= (~0x01)

#define mFANPWM_GET_BASE_CLK(Clock_Source, Prescaler)   ((Clock_Source&FANPWM_SOURCE_CLK_32_768K)? 32768:(((Clock_Source&FANPWM_SOURCE_CLK_1M)?1000000:32000000)/Prescaler))   
#define mFANPWM_GET_CYCLE(Base_Clock, Freq)             (Base_Clock/Freq) 
#define mFANPWM_GET_HIGH(Cycle, Percent)                (((unsigned long)Cycle)*Percent/100)
         
#define mGet_FANPWM_CYCLE(Channel)                      FANPWM[FANPWM_CYC_OFFSET+(Channel<<2)]
                            
//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
                                
//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void FANPWM_GPIO_Enable(unsigned char);
void FANPWM_GPIO_Disable(unsigned char);

//-- For OEM Use -----------------------------------------------
void FANPWM_Setting(unsigned char, unsigned char, unsigned short, unsigned char);
void FANPWM_DutySetting(unsigned char Channel, unsigned char Percent);

#endif //FANPWM_H
