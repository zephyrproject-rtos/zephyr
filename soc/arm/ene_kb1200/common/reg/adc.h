/**************************************************************************//**
 * @file     adc.h
 * @brief    Define ADC's function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ADC_H
#define ADC_H

/**
  \brief  Structure type to access Analog to Digital Convertor (ADC).
 */
typedef volatile struct
{
  uint32_t ADCCFG;                                    /*Configuration Register */
  uint32_t Reserved[3];                               /*Reserved */
  uint32_t ADCxDATA[14];                              /*Data Register */
}  ADC_T;

//-- Constant Define -------------------------------------------
#define ADC_ADC0_GPIO_Num           0x0A
#define ADC_ADC1_GPIO_Num           0x0B
#define ADC_ADC2_GPIO_Num           0x0C
#define ADC_ADC3_GPIO_Num           0x0D
#define ADC_ADC4_GPIO_Num           0x0E
#define ADC_ADC5_GPIO_Num           0x0F
#define ADC_ADC6_GPIO_Num           0x10
#define ADC_ADC7_GPIO_Num           0x11
#define ADC_ADC8_GPIO_Num           0x12
#define ADC_ADC9_GPIO_Num           0x13
#define ADC_ADC10_GPIO_Num          0x14
#define ADC_ADC11_GPIO_Num          0x15
#define ADC_ADC12_GPIO_Num
#define ADC_ADC13_GPIO_Num

#define ADC_Channel_Bit         (0x3FFFUL)          //bit0 = ch0,....bit13 = ch13
#define ADC_Channel_N           14                  //ADC channle number
#define ADC_Invalid_Value       0x8000

//-- Macro Define -----------------------------------------------
#define mADC_ENABLE()           do { adc->ADCCFG |= 1;               } while (0)
#define mADC_Ch_Enable(ch)      do { adc->ADCCFG |= 1UL<<(ch+16);    } while (0)
#define mADC_DISABLE()          do { adc->ADCCFG &= ~(1UL);          } while (0)
#define mADC_Ch_Disable(ch)     do { adc->ADCCFG &= ~(1UL<<(ch+16)); } while (0)

#endif //ADDA_H
