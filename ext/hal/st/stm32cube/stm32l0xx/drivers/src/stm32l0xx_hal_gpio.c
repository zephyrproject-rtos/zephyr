/**
  ******************************************************************************
  * @file    stm32l0xx_hal_gpio.c
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
  * @brief   GPIO HAL module driver.  
  *          This file provides firmware functions to manage the following 
  *          functionalities of the General Purpose Input/Output (GPIO) peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *         
  @verbatim
  ==============================================================================
                    ##### GPIO Peripheral features #####
  ==============================================================================
  [..] 
    (+) Each port bit of the general-purpose I/O (GPIO) ports can be individually 
  configured by software in several modes:
        (++) Input mode 
        (++) Analog mode
        (++) Output mode
        (++) Alternate function mode
        (++) External interrupt/event lines
 
    (+) During and just after reset, the alternate functions and external interrupt  
  lines are not active and the I/O ports are configured in input floating mode.
  
    (+) All GPIO pins have weak internal pull-up and pull-down resistors, which can be 
  activated or not.

    (+) In Output or Alternate mode, each IO can be configured on open-drain or push-pull
  type and the IO speed can be selected depending on the VDD value.
  
    (+) The microcontroller IO pins are connected to onboard peripherals/modules through a 
        multiplexer that allows only one peripheral alternate function (AF) connected 
  to an IO pin at a time. In this way, there can be no conflict between peripherals 
  sharing the same IO pin. 

    (+) All ports have external interrupt/event capability. To use external interrupt 
  lines, the port must be configured in input mode. All available GPIO pins are 
  connected to the 16 external interrupt/event lines from EXTI0 to EXTI15.
  
    (+) The external interrupt/event controller consists of up to 28 edge detectors
  (16 lines are connected to GPIO) for generating event/interrupt requests (each 
  input line can be independently configured to select the type (interrupt or event) 
  and the corresponding trigger event (rising or falling or both). Each line can 
  also be masked independently. 
  
                     ##### How to use this driver #####
  ==============================================================================  
  [..]             
    (#) Enable the GPIO IOPORT clock using the following function: __HAL_RCC_GPIOx_CLK_ENABLE(). 

    (#) Configure the GPIO pin(s) using HAL_GPIO_Init().
        (++) Configure the IO mode using "Mode" member from GPIO_InitTypeDef structure
        (++) Activate Pull-up, Pull-down resistor using "Pull" member from GPIO_InitTypeDef 
             structure.
        (++) In case of Output or alternate function mode selection: the speed is 
             configured through "Speed" member from GPIO_InitTypeDef structure.
        (++) In alternate mode is selection, the alternate function connected to the IO
             is configured through "Alternate" member from GPIO_InitTypeDef structure.
        (++) Analog mode is required when a pin is to be used as ADC channel 
             or DAC output.
        (++) In case of external interrupt/event selection the "Mode" member from 
             GPIO_InitTypeDef structure select the type (interrupt or event) and 
             the corresponding trigger event (rising or falling or both).
   
    (#) In case of external interrupt/event mode selection, configure NVIC IRQ priority 
        mapped to the EXTI line using HAL_NVIC_SetPriority() and enable it using
        HAL_NVIC_EnableIRQ().
         
   (#) HAL_GPIO_DeInit allows to set register values to their reset value. This function
       is also to be used when unconfiguring pin which was used as an external interrupt
       or in event mode. That is the only way to reset the corresponding bit in
       EXTI & SYSCFG registers.

    (#) To get the level of a pin configured in input mode use HAL_GPIO_ReadPin().
            
    (#) To set/reset the level of a pin configured in output mode use 
        HAL_GPIO_WritePin()/HAL_GPIO_TogglePin().

    (#) To lock pin configuration until next reset use HAL_GPIO_LockPin().

    (#) During and just after reset, the alternate functions are not 
        active and the GPIO pins are configured in input floating mode (except JTAG
        pins).
  
    (#) The LSE oscillator pins OSC32_IN and OSC32_OUT can be used as general purpose 
        (PC14 and PC15, respectively) when the LSE oscillator is off. The LSE has 
        priority over the GPIO function.
  
    (#) The HSE oscillator pins OSC_IN/OSC_OUT can be used as 
        general purpose PH0 and PH1, respectively, when the HSE oscillator is off. 
        The HSE has priority over the GPIO function.
  
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************  
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/** @addtogroup STM32L0xx_HAL_Driver
  * @{
  */

#ifdef HAL_GPIO_MODULE_ENABLED

/** @addtogroup GPIO
  * @brief GPIO HAL module driver
  * @{
  */

/** @addtogroup GPIO_Private
  * @{
  */
/* Private define ------------------------------------------------------------*/

 
#define GPIO_MODE             ((uint32_t)0x00000003U)
#define EXTI_MODE             ((uint32_t)0x10000000U)
#define GPIO_MODE_IT          ((uint32_t)0x00010000U)
#define GPIO_MODE_EVT         ((uint32_t)0x00020000U)
#define RISING_EDGE           ((uint32_t)0x00100000U) 
#define FALLING_EDGE          ((uint32_t)0x00200000U) 
#define GPIO_OUTPUT_TYPE      ((uint32_t)0x00000010U)

#define GPIO_NUMBER           ((uint32_t)16U)

/**
  * @}
  */
/** @addtogroup GPIO_Exported_Functions
  * @{
  */

/** @addtogroup GPIO_Exported_Functions_Group1
 *  @brief    Initialization and de-initialization functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
 
@endverbatim
  * @{
  */

/**
  * @brief  Initializes the GPIOx peripheral according to the specified parameters in the GPIO_Init.
  * @param  GPIOx: where x can be (A..E and H) to select the GPIO peripheral for STM32L0XX family devices.
  *                Note that GPIOE is not available on all devices.
  * @param  GPIO_Init: pointer to a GPIO_InitTypeDef structure that contains
  *                    the configuration information for the specified GPIO peripheral.
  * @retval None
  */
void HAL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
  uint32_t position = 0x00U;
  uint32_t iocurrent = 0x00U;
  uint32_t temp = 0x00U;
 
  /* Check the parameters */
  assert_param(IS_GPIO_MODE(GPIO_Init->Mode));
  assert_param(IS_GPIO_PULL(GPIO_Init->Pull));
  assert_param(IS_GPIO_PIN_AVAILABLE(GPIOx,(GPIO_Init->Pin)));

  /* Configure the port pins */
  while ((GPIO_Init->Pin) >> position)
  {
    /* Get the IO position */
    iocurrent = (GPIO_Init->Pin) & (1U << position);
    
    if(iocurrent)
    {
      /*--------------------- GPIO Mode Configuration ------------------------*/
      /* In case of Alternate function mode selection */
      if((GPIO_Init->Mode == GPIO_MODE_AF_PP) || (GPIO_Init->Mode == GPIO_MODE_AF_OD)) 
      {
        /* Check if the Alternate function is compliant with the GPIO in use */
        assert_param(IS_GPIO_AF_AVAILABLE(GPIOx,(GPIO_Init->Alternate)));
        /* Configure Alternate function mapped with the current IO */ 
        temp = GPIOx->AFR[position >> 3U];
        temp &= ~((uint32_t)0xFU << ((uint32_t)(position & (uint32_t)0x07U) * 4U)) ;
        temp |= ((uint32_t)(GPIO_Init->Alternate) << (((uint32_t)position & (uint32_t)0x07U) * 4U)) ;
        GPIOx->AFR[position >> 3U] = temp;
      } 

      /* In case of Output or Alternate function mode selection */
      if((GPIO_Init->Mode == GPIO_MODE_OUTPUT_PP) || (GPIO_Init->Mode == GPIO_MODE_AF_PP) ||
         (GPIO_Init->Mode == GPIO_MODE_OUTPUT_OD) || (GPIO_Init->Mode == GPIO_MODE_AF_OD))
      {
        /* Check the Speed parameter */
        assert_param(IS_GPIO_SPEED(GPIO_Init->Speed));
        /* Configure the IO Speed */
        temp = GPIOx->OSPEEDR;
        temp &= ~(GPIO_OSPEEDER_OSPEED0 << (position * 2U));
        temp |= (GPIO_Init->Speed << (position * 2U));
        GPIOx->OSPEEDR = temp;

        /* Configure the IO Output Type */
        temp= GPIOx->OTYPER;
        temp &= ~(GPIO_OTYPER_OT_0 << position) ;
        temp |= (((GPIO_Init->Mode & GPIO_OUTPUT_TYPE) >> 4U) << position);
        GPIOx->OTYPER = temp;
      }

      /* Configure IO Direction mode (Input, Output, Alternate or Analog) */
      temp = GPIOx->MODER;
      temp &= ~(GPIO_MODER_MODE0 << (position * 2U));
      temp |= ((GPIO_Init->Mode & GPIO_MODE) << (position * 2U));
      GPIOx->MODER = temp;

      /* Activate the Pull-up or Pull down resistor for the current IO */
      temp = GPIOx->PUPDR;
      temp &= ~(GPIO_PUPDR_PUPD0 << (position * 2U));
      temp |= ((GPIO_Init->Pull) << (position * 2U)); 
      GPIOx->PUPDR = temp;

      /*--------------------- EXTI Mode Configuration ------------------------*/
      /* Configure the External Interrupt or event for the current IO */
      if((GPIO_Init->Mode & EXTI_MODE) == EXTI_MODE) 
      {
        /* Enable SYSCFG Clock */
        __HAL_RCC_SYSCFG_CLK_ENABLE();

        temp = SYSCFG->EXTICR[position >> 2U];
        temp &= ~(((uint32_t)0x0FU) << (4U * (position & 0x03U)));
        temp |= ((uint32_t)(GPIO_GET_INDEX(GPIOx)) << (4U * (position & 0x03U)));
        SYSCFG->EXTICR[position >> 2U] = temp;

        /* Clear EXTI line configuration */
        temp = EXTI->IMR;
        temp &= ~((uint32_t)iocurrent);
        if((GPIO_Init->Mode & GPIO_MODE_IT) == GPIO_MODE_IT)
        {
          temp |= iocurrent;
        }
        EXTI->IMR = temp;

        temp = EXTI->EMR;
        temp &= ~((uint32_t)iocurrent);
        if((GPIO_Init->Mode & GPIO_MODE_EVT) == GPIO_MODE_EVT)
        {
          temp |= iocurrent;
        }
        EXTI->EMR = temp;

        /* Clear Rising Falling edge configuration */
        temp = EXTI->RTSR;
        temp &= ~((uint32_t)iocurrent);
        if((GPIO_Init->Mode & RISING_EDGE) == RISING_EDGE)
        {
          temp |= iocurrent;
        }
        EXTI->RTSR = temp;

        temp = EXTI->FTSR;
        temp &= ~((uint32_t)iocurrent);
        if((GPIO_Init->Mode & FALLING_EDGE) == FALLING_EDGE)
        {
          temp |= iocurrent;
        }
        EXTI->FTSR = temp;
      }
    }
    position++;
  }
}

/**
  * @brief  De-initializes the GPIOx peripheral registers to their default reset values.
  * @param  GPIOx: where x can be (A..E and H) to select the GPIO peripheral for STM32L0XX family devices.
  *                Note that GPIOE is not available on all devices.
  * @param  GPIO_Pin: specifies the port bit to be written.
  *                   This parameter can be one of GPIO_PIN_x where x can be (0..15).
  *                   All port bits are not necessarily available on all GPIOs.
  * @retval None
  */
void HAL_GPIO_DeInit(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin)
{ 
  uint32_t position = 0x00U;
  uint32_t iocurrent = 0x00U;
  uint32_t tmp = 0x00U;

  /* Check the parameters */
  assert_param(IS_GPIO_PIN_AVAILABLE(GPIOx,GPIO_Pin));

  /* Configure the port pins */
  while (GPIO_Pin >> position)
  {
    /* Get the IO position */
    iocurrent = (GPIO_Pin) & (1U << position);

    if(iocurrent)
    {
      /*------------------------- GPIO Mode Configuration --------------------*/
      /* Configure IO Direction in Input Floting Mode */
      GPIOx->MODER &= ~(GPIO_MODER_MODE0 << (position * 2U));
      
      /* Configure the default Alternate Function in current IO */ 
      GPIOx->AFR[position >> 3U] &= ~((uint32_t)0xFU << ((uint32_t)(position & (uint32_t)0x07U) * 4U)) ;
      
      /* Configure the default value for IO Speed */
      GPIOx->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEED0 << (position * 2U));
      
      /* Configure the default value IO Output Type */
      GPIOx->OTYPER  &= ~(GPIO_OTYPER_OT_0 << position) ;
      
      /* Deactivate the Pull-up oand Pull-down resistor for the current IO */
      GPIOx->PUPDR &= ~(GPIO_PUPDR_PUPD0 << (position * 2U));
      
      /*------------------------- EXTI Mode Configuration --------------------*/
      /* Clear the External Interrupt or Event for the current IO */
      
      tmp = SYSCFG->EXTICR[position >> 2U];
      tmp &= (((uint32_t)0x0FU) << (4U * (position & 0x03U)));
      if(tmp == (GPIO_GET_INDEX(GPIOx) << (4U * (position & 0x03U))))
      {
        tmp = ((uint32_t)0x0FU) << (4U * (position & 0x03U));
        SYSCFG->EXTICR[position >> 2U] &= ~tmp;

        /* Clear EXTI line configuration */
        EXTI->IMR &= ~((uint32_t)iocurrent);
        EXTI->EMR &= ~((uint32_t)iocurrent);

        /* Clear Rising Falling edge configuration */
        EXTI->RTSR &= ~((uint32_t)iocurrent);
        EXTI->FTSR &= ~((uint32_t)iocurrent);
      }
    }
     position++;
  }
}

/**
  * @}
  */

/** @addtogroup GPIO_Exported_Functions_Group2
 *  @brief   GPIO Read and Write
 *
@verbatim
 ===============================================================================
                       ##### IO operation functions #####
 ===============================================================================

@endverbatim
  * @{
  */

/**
  * @brief  Reads the specified input port pin.
  * @param  GPIOx: where x can be (A..E and H) to select the GPIO peripheral for STM32L0xx family devices.
  *                Note that GPIOE is not available on all devices.
  * @param  GPIO_Pin: specifies the port bit to read.
  *                   This parameter can be GPIO_PIN_x where x can be (0..15).
  *                   All port bits are not necessarily available on all GPIOs.
  * @retval The input port pin value.
  */
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
  GPIO_PinState bitstatus;
  
  /* Check the parameters */
  assert_param(IS_GPIO_PIN_AVAILABLE(GPIOx,GPIO_Pin));
  
  if((GPIOx->IDR & GPIO_Pin) != (uint32_t)GPIO_PIN_RESET)
  {
    bitstatus = GPIO_PIN_SET;
  }
  else
  {
    bitstatus = GPIO_PIN_RESET;
  }
  return bitstatus;
}

/**
  * @brief  Sets or clears the selected data port bit.
  * 
  * @note   This function uses GPIOx_BSRR register to allow atomic read/modify 
  *         accesses. In this way, there is no risk of an IRQ occurring between
  *         the read and the modify access.
  *               
  * @param  GPIOx: where x can be (A..E and H) to select the GPIO peripheral for STM32L0xx family devices.
  *                Note that GPIOE is not available on all devices.
  * @param  GPIO_Pin: specifies the port bit to be written.
  *                   This parameter can be one of GPIO_PIN_x where x can be (0..15).
  *                   All port bits are not necessarily available on all GPIOs.
  * @param  PinState: specifies the value to be written to the selected bit.
  *                   This parameter can be one of the GPIO_PinState enum values:
  *                        GPIO_PIN_RESET: to clear the port pin
  *                        GPIO_PIN_SET: to set the port pin
  * @retval None
  */
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
  /* Check the parameters */
  assert_param(IS_GPIO_PIN_AVAILABLE(GPIOx,GPIO_Pin));
  assert_param(IS_GPIO_PIN_ACTION(PinState));
  
  if(PinState != GPIO_PIN_RESET)
  {
    GPIOx->BSRR = GPIO_Pin;
  }
  else
  {
    GPIOx->BRR = GPIO_Pin ;
  }
}

/**
  * @brief  Toggles the specified GPIO pins.
  * @param  GPIOx: Where x can be (A..E and H) to select the GPIO peripheral for STM32L0xx family devices.
  *                Note that GPIOE is not available on all devices.
  *                All port bits are not necessarily available on all GPIOs.
  * @param  GPIO_Pin: Specifies the pins to be toggled.
  * @retval None
  */
void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
  /* Check the parameters */
  assert_param(IS_GPIO_PIN_AVAILABLE(GPIOx,GPIO_Pin));

  GPIOx->ODR ^= GPIO_Pin;
}

/**
* @brief  Locks GPIO Pins configuration registers.
* @note   The locked registers are GPIOx_MODER, GPIOx_OTYPER, GPIOx_OSPEEDR,
*         GPIOx_PUPDR, GPIOx_AFRL and GPIOx_AFRH.
* @note   The configuration of the locked GPIO pins can no longer be modified
*         until the next reset.
* @param  GPIOx: where x can be (A..E and H) to select the GPIO peripheral for STM32L0xx family.
*                Note that GPIOE is not available on all devices.
* @param  GPIO_Pin: specifies the port bit to be locked.
*         This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
*         All port bits are not necessarily available on all GPIOs.
* @retval None
*/
HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
  __IO uint32_t tmp = GPIO_LCKR_LCKK;

  /* Check the parameters */
  assert_param(IS_GPIO_PIN_AVAILABLE(GPIOx,GPIO_Pin));

  /* Apply lock key write sequence */
  tmp |= GPIO_Pin;
  /* Set LCKx bit(s): LCKK='1' + LCK[15-0] */
  GPIOx->LCKR = tmp;
  /* Reset LCKx bit(s): LCKK='0' + LCK[15-0] */
  GPIOx->LCKR = GPIO_Pin;
  /* Set LCKx bit(s): LCKK='1' + LCK[15-0] */
  GPIOx->LCKR = tmp;
  /* Read LCKK bit*/
  tmp = GPIOx->LCKR;

  if((GPIOx->LCKR & GPIO_LCKR_LCKK) != RESET)
  {
    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;
  }
}
/**
  * @brief  This function handles EXTI interrupt request.
  * @param  GPIO_Pin: Specifies the pins connected to the EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)
{
  /* EXTI line interrupt detected */
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != RESET) 
  { 
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
    HAL_GPIO_EXTI_Callback(GPIO_Pin);
  }
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected to the EXTI line.
  * @retval None
  */
__weak void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(GPIO_Pin);

  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */ 
}

/**
  * @}
  */


/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_GPIO_MODULE_ENABLED */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

