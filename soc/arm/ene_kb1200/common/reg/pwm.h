/****************************************************************************
 * @file     pwm.h
 * @brief    Define PWM's registers and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ****************************************************************************/

#ifndef PWM_H
#define PWM_H

/**
  \brief  Structure type to access Pulse Width Modulation (PWM).
 */
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

//-- Constant Define -------------------------------------------
#define PWM_CHANNEL_NUM         10

#define PWM0_GPIO_Num           0x3A
#define PWM1_GPIO_Num           0x38
#define PWM2_GPIO_Num           0x3B
#define PWM3_GPIO_Num           0x26
#define PWM4_GPIO_Num           0x31
#define PWM5_GPIO_Num           0x30
#define PWM6_GPIO_Num           0x37
#define PWM7_GPIO_Num           0x23
#define PWM8_GPIO_Num           0x00
#define PWM9_GPIO_Num           0x22

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

#endif //PWM_H
