/**************************************************************************//**
 * @file     pwmled.h
 * @brief    Define PWMLED's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef PWMLED_H
#define PWMLED_H

/**
  \brief  Structure type to access PWM LED Breathing (PWMLED).
 */
typedef volatile struct
{
  uint8_t  PL0CFG;                                    /*Configuration Register */
  uint8_t  Reserved0[3];                              /*Reserved */    
  uint8_t  PL0TMROFF;                                 /*Timer Period - Full Off Register */
  uint8_t  PL0TMRFBB;                                 /*Timer Period - Full Off Base and Bright Register */
  uint8_t  PL0TMRON;                                  /*Timer Period - Full On Register */
  uint8_t  PL0TMROBD;                                 /*Timer Period - Full On Base and Dark Register */    
  uint32_t Reserved1[2];                              /*Reserved */                      
  uint8_t  PL1CFG;                                    /*Configuration Register */
  uint8_t  Reserved2[3];                              /*Reserved */    
  uint8_t  PL1TMROFF;                                 /*Timer Period - Full Off Register */
  uint8_t  PL1TMRFBB;                                 /*Timer Period - Full Off Base and Bright Register */
  uint8_t  PL1TMRON;                                  /*Timer Period - Full On Register */
  uint8_t  PL1TMROBD;                                 /*Timer Period - Full On Base and Dark Register */    
  uint32_t Reserved3[2];                              /*Reserved */              
}  PWMLED_T;

#define pwmled                ((PWMLED_T *) PWMLED_BASE)             /* PWMLED Struct       */

#define PWMLED                ((volatile unsigned long  *) PWMLED_BASE)     /* PWMLED  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_PWMLED                      USR_SUPPORT_PWMLED0||USR_SUPPORT_PWMLED1

#define PWMLED0_Open_Drain_Enable           USR_PWMLED0_Open_Drain_Enable
#define PWMLED0_Full_On_percent             USR_PWMLED0_Full_On_percent

#define PWMLED1_Open_Drain_Enable           USR_PWMLED1_Open_Drain_Enable
#define PWMLED1_Full_On_percent             USR_PWMLED1_Full_On_percent

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define PWMLED_CHANNEL_NUM                  2

#define PWMLED0_GPIO_Num                    0x00//0x19
#define PWMLED1_GPIO_Num                    0x22//0x54

#define PWMLED0                             0
#define PWMLED1                             1

#define PWMLED_BASE_CLOCK_8ms               0x08

#define Full_On_100_percent                 0x0F
#define Full_On_98_percent                  0x0E
#define Full_On_97_percent                  0x0D
#define Full_On_95_percent                  0x0C
#define Full_On_94_percent                  0x0B
#define Full_On_92_percent                  0x0A
#define Full_On_91_percent                  0x09
#define Full_On_89_percent                  0x08
#define Full_On_88_percent                  0x07
#define Full_On_86_percent                  0x06
#define Full_On_84_percent                  0x05
#define Full_On_83_percent                  0x04
#define Full_On_81_percent                  0x03
#define Full_On_80_percent                  0x02
#define Full_On_78_percent                  0x01
#define Full_On_77_percent                  0x00

#define PWMLED_FULL_ON_OFF_CLOCK_8ms        0x00
#define PWMLED_FULL_ON_OFF_CLOCK_16ms       0x01
#define PWMLED_FULL_ON_OFF_CLOCK_32ms       0x02
#define PWMLED_FULL_ON_OFF_CLOCK_64ms       0x03

#define PWMLED_BRIGHT_SET_VALUE_MAX         0x0F
#define PWMLED_DARK_SET_VALUE_MAX           0x0F
#define PWMLED_FULL_ON_OFF_SET_VALUE_MAX    0x00FF

//-- Macro Define -----------------------------------------------
#define mPWMLED_Change_Length(percent)      (49+percent)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
#if SUPPORT_PWMLED
void PWMLED_Init(unsigned char PWMLEDNum);
void PWMLED_Enable(unsigned char PWMLEDNum);
void PWMLED_Disable(unsigned char PWMLEDNum);
void PWMLED_GPIO_Enable(unsigned char PWMLEDNum);
void PWMLED_GPIO_Disable(unsigned char PWMLEDNum);
unsigned char PWMLED_Program(unsigned short Full_Off_T,unsigned short To_On_T,unsigned short Full_On_T,unsigned short To_Off_T,unsigned char PWMLEDNum);
#endif  //SUPPORT_PWMLED

#endif //PWMLED_H
