#if defined(CONFIG_SOC_SERIES_STM32U5X)
#include <stm32u5xx_ll_adc.h>
int assign_hardware_trigger(ADC_TypeDef* adc, TIM_TypeDef* timer, int* trigger){
    switch((uint32_t)adc) {
#if defined(ADC1)
        case (uint32_t)ADC1:
#endif
#if defined(ADC2)
        case (uint32_t)ADC2:
#endif
#if defined(ADC1) || defined(ADC2)
            switch((uint32_t)timer) {
                case (uint32_t)TIM3:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM3_TRGO;
                    break;
                case (uint32_t)TIM8:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM8_TRGO;
                    break;
                case (uint32_t)TIM1:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM1_TRGO;
                    break;
                case (uint32_t)TIM2:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM2_TRGO;
                    break;
                case (uint32_t)TIM4:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM4_TRGO;
                    break;
                case (uint32_t)TIM6:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM6_TRGO;
                    break;
                case (uint32_t)TIM15:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM15_TRGO;
                    break;
                default:
                    return -EINVAL;
            }
            break;
#endif
#if defined(ADC4)
        case (uint32_t)ADC4:
            switch((uint32_t)timer) {
                case (uint32_t)TIM2:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM2_TRGO_ADC4;
                    break;
                case (uint32_t)TIM6:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM6_TRGO_ADC4;
                    break;
                case (uint32_t)TIM15:
                    *trigger = LL_ADC_REG_TRIG_EXT_TIM15_TRGO_ADC4;
                    break;
                default:
                    return -EINVAL;
                
            }
            break;
#endif
        default:
            return -EINVAL;
    }

    return 0;
}
#endif