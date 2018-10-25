{% import 'i2c_ll_stm32.meta.jinja2' as i2c_stm32 %}

#ifndef _I2C_LL_STM32_DEVICE_H_
#define _I2C_LL_STM32_DEVICE_H_

{{ i2c_stm32.device(data, ['st,stm32-i2c-v1', 'st,stm32-i2c-v2']) }}

#endif
