/**************************************************************************//**
 * @file     pwm.h
 * @brief    Define PWM's registers and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef PWM_H
#define PWM_H

/**
  \brief  Structure type to access Pulse Width Modulation (PWM).
 */
//typedef volatile struct
//{
//  uint16_t PWM0CFG;                                   /*Configuration Register */
//  uint16_t Reserved0;                                 /*Reserved */
//  uint16_t PWM0HIGH;                                  /*High Length Register */
//  uint16_t Reserved1;                                 /*Reserved */  
//  uint16_t PWM0CYC;                                   /*Cycle Length Register */
//  uint16_t Reserved2;                                 /*Reserved */                       
//  uint32_t PWM0CHC;                                   /*Current High/Cycle Length Register */
//  uint16_t PWM1CFG;                                   /*Configuration Register */
//  uint16_t Reserved3;                                 /*Reserved */
//  uint16_t PWM1HIGH;                                  /*High Length Register */
//  uint16_t Reserved4;                                 /*Reserved */  
//  uint16_t PWM1CYC;                                   /*Cycle Length Register */
//  uint16_t Reserved5;                                 /*Reserved */                       
//  uint32_t PWM1CHC;                                   /*Current High/Cycle Length Register */
//  uint16_t PWM2CFG;                                   /*Configuration Register */
//  uint16_t Reserved6;                                 /*Reserved */
//  uint16_t PWM2HIGH;                                  /*High Length Register */
//  uint16_t Reserved7;                                 /*Reserved */  
//  uint16_t PWM2CYC;                                   /*Cycle Length Register */
//  uint16_t Reserved8;                                 /*Reserved */                       
//  uint32_t PWM2CHC;                                   /*Current High/Cycle Length Register */
//  uint16_t PWM3CFG;                                   /*Configuration Register */
//  uint16_t Reserved9;                                 /*Reserved */
//  uint16_t PWM3HIGH;                                  /*High Length Register */
//  uint16_t Reserved10;                                /*Reserved */  
//  uint16_t PWM3CYC;                                   /*Cycle Length Register */
//  uint16_t Reserved11;                                /*Reserved */                       
//  uint32_t PWM3CHC;                                   /*Current High/Cycle Length Register */      
//}  PWM_T;

typedef volatile struct
{
  uint16_t PWMCFG;                                    /*Configuration Register */
  uint16_t Reserved0;                                 /*Reserved */
  uint16_t PWMHIGH;                                   /*High Length Register */
  uint16_t Reserved1;                                 /*Reserved */  
  uint16_t PWMCYC;                                    /*Cycle Length Register */
  uint16_t Reserved2;                                 /*Reserved */                       
  uint32_t PWMCHC;                                    /*Current High/Cycle Length Register */
}  PWM_T;

//#define pwm                ((PWM_T *) PWM_BASE)             /* PWM Struct       */
//#define PWM                ((volatile unsigned long  *) PWM_BASE)     /* PWM  array       */

//***************************************************************
// User Define                                                
//***************************************************************

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
//#define PWM_CHANNEL_NUM         16
//#define PWM_EGPIO_START_NUM     4
#define PWM_CHANNEL_NUM         10

#define PWM0_GPIO_Num           0x3A//0x0F
#define PWM1_GPIO_Num           0x38//0x10
#define PWM2_GPIO_Num           0x3B//0x11
#define PWM3_GPIO_Num           0x26//0x19
#define PWM4_GPIO_Num           0x31//0x3A
#define PWM5_GPIO_Num           0x30//0x38
#define PWM6_GPIO_Num           0x37//0x3B
#define PWM7_GPIO_Num           0x23//0x26
#define PWM8_GPIO_Num           0x00//0x3B
#define PWM9_GPIO_Num           0x22//0x26

#define PWM_CFG_OFFSET          0
#define PWM_HIGH_OFFSET         1
#define PWM_CYC_OFFSET          2

#define PWM_SOURCE_CLK_32M      0x0000
#define PWM_SOURCE_CLK_1M       0x4000
#define PWM_SOURCE_CLK_32_768K  0x8000

#define PWM_PRESCALER_BIT_S     8

#define PWM_RULE0               0x0000
#define PWM_RULE1               0x0080

#define PWM_PUSHPULL            0x0000
#define PWM_OPENDRAIN           0x0002

//-- Macro Define -----------------------------------------------
//#define mPWM_Enable(Channel)                         PWM[PWM_CFG_OFFSET+(Channel<<2)] |= 0x01
//#define mPWM_Disable(Channel)                        PWM[PWM_CFG_OFFSET+(Channel<<2)] &= (~0x01)
//
//#define mPWM_GET_BASE_CLK(Clock_Source, Prescaler)   ((Clock_Source&PWM_SOURCE_CLK_32_768K)? 32768:(((Clock_Source&PWM_SOURCE_CLK_1M)?1000000:32000000)/Prescaler))   
//#define mPWM_GET_CYCLE(Base_Clock, Freq)             (Base_Clock/Freq) 
//#define mPWM_GET_HIGH(Cycle, Percent)                (((unsigned long)Cycle)*Percent/100)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
extern const unsigned char PWM_GPIO[];

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void PWM_GPIO_Enable(unsigned char);
void PWM_GPIO_Disable(unsigned char);

//-- For OEM Use -----------------------------------------------
void PWM_Setting(unsigned char, unsigned char, unsigned short, unsigned char);
void PWM_DutySetting(unsigned char Channel, unsigned char Percent);

#endif //PWM_H
