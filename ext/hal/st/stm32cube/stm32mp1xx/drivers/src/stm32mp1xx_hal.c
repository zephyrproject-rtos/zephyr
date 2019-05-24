/**
  ******************************************************************************
  * @file    stm32mp1xx_hal.c
  * @author  MCD Application Team
  * @brief   HAL module driver.
  *          This is the common part of the HAL initialization
  *
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The common HAL driver contains a set of generic and common APIs that can be
    used by the PPP peripheral drivers and the user to start using the HAL.
    [..]
    The HAL contains two APIs' categories:
         (+) Common HAL APIs
         (+) Services HAL APIs

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @defgroup HAL  HAL
  * @brief HAL module driver.
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup HAL_Private_Defines HAL Private Defines
  * @{
  */
  
/**
 * @brief STM32MP1xx HAL Driver version number
   */
#define __STM32MP1xx_HAL_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define __STM32MP1xx_HAL_VERSION_SUB1   (0x00U) /*!< [23:16] sub1 version */
#define __STM32MP1xx_HAL_VERSION_SUB2   (0x00U) /*!< [15:8]  sub2 version */
#define __STM32MP1xx_HAL_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define __STM32MP1xx_HAL_VERSION         ((__STM32MP1xx_HAL_VERSION_MAIN << 24)\
                                        |(__STM32MP1xx_HAL_VERSION_SUB1 << 16)\
                                        |(__STM32MP1xx_HAL_VERSION_SUB2 << 8 )\
                                        |(__STM32MP1xx_HAL_VERSION_RC))

#define IDCODE_DEVID_MASK    ((uint32_t)0x00000FFF)
#define VREFBUF_TIMEOUT_VALUE     (uint32_t)10   /* 10 ms  */
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup HAL_Private_Variables HAL Private Variables
  * @{
  */
__IO uint32_t uwTick;
#if defined(CORE_CM4)
static uint32_t uwTickPrio   = (1UL << __NVIC_PRIO_BITS); /* Invalid PRIO */
#endif
static HAL_TickFreqTypeDef uwTickFreq = HAL_TICK_FREQ_DEFAULT;  /* 1KHz */
/**
  * @}
  */
  
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup HAL_Private_Functions  HAL Private Functions
  * @{
  */

/** @defgroup HAL_Group1 Initialization and de-initialization Functions
 *  @brief    Initialization and de-initialization functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Initializes the Flash interface the NVIC allocation and initial clock
          configuration. It initializes the systick also when timeout is needed
          and the backup domain when enabled.
      (+) De-Initializes common part of the HAL.
      (+) Configure The time base source to have 1ms time base with a dedicated
          Tick interrupt priority.
        (++) SysTick timer is used by default as source of time base, but user
             can eventually implement his proper time base source (a general purpose
             timer for example or other time source), keeping in mind that Time base
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
             handled in milliseconds basis.
        (++) Time base configuration function (HAL_InitTick ()) is called automatically
             at the beginning of the program after reset by HAL_Init() or at any time
             when clock is configured, by HAL_RCC_ClockConfig().
        (++) Source of time base is configured  to generate interrupts at regular
             time intervals. Care must be taken if HAL_Delay() is called from a
             peripheral ISR process, the Tick interrupt line must have higher priority
            (numerically lower) than the peripheral interrupt. Otherwise the caller
            ISR process will be blocked.
       (++) functions affecting time base configurations are declared as __weak
             to make  override possible  in case of other  implementations in user file.
@endverbatim
  * @{
  */

/**
  * @brief  This function is used to initialize the HAL Library; it must be the first
  *         instruction to be executed in the main program (before to call any other
  *         HAL function), it performs the following:
  *           Configures the SysTick to generate an interrupt each 1 millisecond,
  *           which is clocked by the HSI (at this stage, the clock is not yet
  *           configured and thus the system is running from the internal HSI at 16 MHz).
  *           Set NVIC Group Priority to 4.
  *           Calls the HAL_MspInit() callback function defined in user file
  *           "stm32mp1xx_hal_msp.c" to do the global low level hardware initialization
  *
  * @note   SysTick is used as time base for the HAL_Delay() function, the application
  *         need to ensure that the SysTick time base is always set to 1 millisecond
  *         to have correct HAL operation.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_Init(void)
{
  /* Set Interrupt Group Priority */
#if defined (CORE_CM4)
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
#endif

  /* Use systick as time base source and configure 1ms tick (default clock after Reset is HSI) */
  if(HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Init the low level hardware */
  HAL_MspInit();

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  This function de-Initializes common part of the HAL and stops the systick.
  *         This function is optional.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DeInit(void)
{
  /* Reset of all peripherals */


  /* De-Init the low level hardware */
  HAL_MspDeInit();

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initializes the MSP.
  * @retval None
  */
__weak void HAL_MspInit(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_MspInit could be implemented in the user file
   */
}

/**
  * @brief  DeInitializes the MSP.
  * @retval None
  */
__weak void HAL_MspDeInit(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_MspDeInit could be implemented in the user file
   */
}

/**
  * @brief This function configures the source of the time base.
  *        The time source is configured  to have 1ms time base with a dedicated
  *        Tick interrupt priority.
  * @note This function is called  automatically at the beginning of program after
  *       reset by HAL_Init() or at any time when clock is reconfigured  by HAL_RCC_ClockConfig().
  * @note In the default implementation, SysTick timer is the source of time base.
  *       It is used to generate interrupts at regular time intervals.
  *       Care must be taken if HAL_Delay() is called from a peripheral ISR process,
  *       The the SysTick interrupt must have higher priority (numerically lower)
  *       than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
  *       The function is declared as __weak  to be overwritten  in case of other
  *       implementation  in user file.
  * @param TickPriority: Tick interrupt priority.
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  /*Configure the Tick to have interrupt in 1ms time basis*/
#if defined (CORE_CA7)

#if defined(USE_ST_CASIS)
  HAL_SYSTICK_Config(SystemCoreClock/1000);
#elif defined (USE_PL1_SecurePhysicalTimer_IRQ)
  /* Stop Timer */
  PL1_SetControl(0x0);

  PL1_SetCounterFrequency(HSI_VALUE);

  /* Initialize Counter */
  PL1_SetLoadValue(HSI_VALUE/1000);

  /* Disable corresponding IRQ */
  IRQ_Disable(SecurePhysicalTimer_IRQn);
  IRQ_ClearPending(SecurePhysicalTimer_IRQn);

  /* Set timer priority to lowest (Only bit 7:3 are implemented in MP1 CA7 GIC) */
  /* TickPriority is based on 16 level priority (from MCUs) so set it in 7:4 and leave bit 3=0 */
  IRQ_SetPriority(SecurePhysicalTimer_IRQn, TickPriority << 4);

  /* Set edge-triggered IRQ */
  IRQ_SetMode(SecurePhysicalTimer_IRQn, IRQ_MODE_TRIG_EDGE);

  /* Enable corresponding interrupt */
  IRQ_Enable(SecurePhysicalTimer_IRQn);

  /* Kick start Timer */
  PL1_SetControl(0x1);
#else
  /*Set Counter Frequency */
  PL1_SetCounterFrequency(HSI_VALUE);
 // __set_CNTFRQ(HSI_VALUE);
  /* Initialize Counter */
  PL1_SetLoadValue(0x1);
 // __set_CNTP_TVAL(0x1);
#endif

#endif /* CORE_CA7 */


#if defined (CORE_CM4)
  /* Configure the SysTick to have interrupt in 1ms time basis*/
  if (HAL_SYSTICK_Config(SystemCoreClock /(1000U / uwTickFreq)) > 0U)
  {
    return HAL_ERROR;
  }
  /* Configure the SysTick IRQ priority */
  if (TickPriority < (1UL << __NVIC_PRIO_BITS))
  {
    HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0U);
    uwTickPrio = TickPriority;
  }
  else
  {
    return HAL_ERROR;
  }
#endif /* CORE_CM4 */



  /* Return function status */
  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup HAL_Group2 HAL Control functions
 *  @brief    HAL Control functions
 *
@verbatim
 ===============================================================================
                      ##### HAL Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Provide a tick value in millisecond
      (+) Provide a blocking delay in millisecond
      (+) Suspend the time base source interrupt
      (+) Resume the time base source interrupt
      (+) Get the HAL API driver version
      (+) Get the device identifier
      (+) Get the device revision identifier
      (+) Enable/Disable Debug module during SLEEP mode
      (+) Enable/Disable Debug module during STOP mode
      (+) Enable/Disable Debug module during STANDBY mode

@endverbatim
  * @{
  */

/**
  * @brief This function is called to increment  a global variable "uwTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in Systick ISR.
 * @note This function is declared as __weak to be overwritten in case of other
  *      implementations in user file.
  * @retval None
  */
__weak void HAL_IncTick(void)
{
  uwTick += (uint32_t)uwTickFreq;
}

/**
  * @brief Provides a tick value in millisecond.
  * @note This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @retval tick value
  */
__weak uint32_t HAL_GetTick(void)
{
#if defined (CORE_CA7)

#if defined (USE_ST_CASIS)
  return ( Gen_Timer_Get_PhysicalCount() / (HSI_VALUE/1000));
#elif defined (USE_PL1_SecurePhysicalTimer_IRQ)
  /* tick is incremented in SecurePhysicalTimer_IRQ handler */
  return uwTick;
#else
  /* tick value directly got from 64bits CA7 register*/
  return ( PL1_GetCurrentPhysicalValue() / (HSI_VALUE/1000));
#endif

#endif /* CORE_CA7 */


#if defined (CORE_CM4)
  /* tick is incremented in systick handler */
  return uwTick;
#endif /* CORE_CM4 */

}

#if defined (CORE_CM4)
/**
  * @brief This function returns a tick priority.
  * @retval tick priority
  */
uint32_t HAL_GetTickPrio(void)
{
  return uwTickPrio;
}

/**
  * @brief Set new tick Freq.
  * @retval Status
  */
HAL_StatusTypeDef HAL_SetTickFreq(HAL_TickFreqTypeDef Freq)
{
  HAL_StatusTypeDef status  = HAL_OK;
  assert_param(IS_TICKFREQ(Freq));

  if (uwTickFreq != Freq)
  {
    uwTickFreq = Freq;

    /* Apply the new tick Freq  */
    status = HAL_InitTick(uwTickPrio);
  }

  return status;
}

/**
  * @brief Return tick frequency.
  * @retval tick period in Hz
  */
HAL_TickFreqTypeDef HAL_GetTickFreq(void)
{
  return uwTickFreq;
}
#endif

/**
  * @brief This function provides accurate delay (in milliseconds) based
  *        on variable incremented.
  * @note In the default implementation , SysTick timer is the source of time base.
  *       It is used to generate interrupts at regular time intervals where uwTick
  *       is incremented.
  * @note ThiS function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @param Delay: specifies the delay time length, in milliseconds.
  * @retval None
  */
__weak void HAL_Delay(uint32_t Delay)
{
  uint32_t tickstart = HAL_GetTick();
  uint32_t wait = Delay;

  /* Add a freq to guarantee minimum wait */
  if (wait < HAL_MAX_DELAY)
  {
    wait += (uint32_t)(uwTickFreq);
  }

  while ((HAL_GetTick() - tickstart) < wait)
  {
  }
}

/**
  * @brief Suspend Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once HAL_SuspendTick()
  *       is called, the the SysTick interrupt will be disabled and so Tick increment
  *       is suspended.
  * @note This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
__weak void HAL_SuspendTick(void)
{
#if defined (CORE_CA7)
#elif defined (CORE_CM4)
  /* Disable SysTick Interrupt */
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
#endif
}

/**
  * @brief Resume Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once HAL_ResumeTick()
  *       is called, the the SysTick interrupt will be enabled and so Tick increment
  *       is resumed.
  * @note This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
__weak void HAL_ResumeTick(void)
{
#if defined (CORE_CA7)
#elif defined (CORE_CM4)
  /* Enable SysTick Interrupt */
  SysTick->CTRL  |= SysTick_CTRL_TICKINT_Msk;
#endif
}

/**
  * @brief  Returns the HAL revision
  * @retval version : 0xXYZR (8bits for each decimal, R for RC)
  */
uint32_t HAL_GetHalVersion(void)
{
 return __STM32MP1xx_HAL_VERSION;
}

/**
  * @brief  Returns the device revision identifier.
  * @retval Device revision identifier
  */
uint32_t HAL_GetREVID(void)
{
   return((DBGMCU->IDCODE) >> 16);
}

/**
  * @brief  Returns the device identifier.
  * @retval Device identifier
  */
uint32_t HAL_GetDEVID(void)
{
   return((DBGMCU->IDCODE) & IDCODE_DEVID_MASK);
}

/**
  * @brief  Enable DBG wake up on AIEC
  * @retval None
  */
void HAL_EnableDBGWakeUp(void)
{
#if defined (CORE_CA7)
  SET_BIT(EXTI_C1->IMR3, EXTI_IMR3_IM75);
#elif defined (CORE_CM4)
  SET_BIT(EXTI_C2->IMR3, EXTI_IMR3_IM75);
#endif
}

/**
  * @brief  Disable DBG wake up on AIEC
  * @retval None
  */
void HAL_DisableDBGWakeUp(void)
{
#if defined (CORE_CA7)
  CLEAR_BIT(EXTI_C1->IMR3, EXTI_IMR3_IM75);
#elif defined (CORE_CM4)
  CLEAR_BIT(EXTI_C2->IMR3, EXTI_IMR3_IM75);
#endif
}

/**
  * @brief  Enable the Debug Module during Domain1 SLEEP mode
  * @retval None
  */
void HAL_EnableDBGSleepMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Disable the Debug Module during Domain1 SLEEP mode
  * @retval None
  */
void HAL_DisableDBGSleepMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Enable the Debug Module during Domain1 STOP mode
  * @retval None
  */
void HAL_EnableDBGStopMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Disable the Debug Module during Domain1 STOP mode
  * @retval None
  */
void HAL_DisableDBGStopMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Enable the Debug Module during Domain1 STANDBY mode
  * @retval None
  */
void HAL_EnableDBGStandbyMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

/**
  * @brief  Disable the Debug Module during Domain1 STANDBY mode
  * @retval None
  */
void HAL_DisableDBGStandbyMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}


/**
  * @brief Configure the internal voltage reference buffer voltage scale.
  * @param VoltageScaling  specifies the output voltage to achieve
  *          This parameter can be one of the following values:
  *            @arg SYSCFG_VREFBUF_VOLTAGE_SCALE0: VREF_OUT1 around 2.048 V.
  *                                                This requires VDDA equal to or higher than 2.4 V.
  *            @arg SYSCFG_VREFBUF_VOLTAGE_SCALE1: VREF_OUT2 around 2.5 V.
  *                                                This requires VDDA equal to or higher than 2.8 V.
  *            @arg SYSCFG_VREFBUF_VOLTAGE_SCALE2: VREF_OUT3 around 1.5 V.
  *                                                This requires VDDA equal to or higher than 1.8 V.
  *            @arg SYSCFG_VREFBUF_VOLTAGE_SCALE3: VREF_OUT4 around 1.8 V.
  *                                                This requires VDDA equal to or higher than 2.1 V.
  * @retval None
  */
void HAL_SYSCFG_VREFBUF_VoltageScalingConfig(uint32_t VoltageScaling)
{
  /* Check the parameters */
  assert_param(IS_SYSCFG_VREFBUF_VOLTAGE_SCALE(VoltageScaling));

  MODIFY_REG(VREFBUF->CSR, VREFBUF_CSR_VRS, VoltageScaling);
}

/**
  * @brief Configure the internal voltage reference buffer high impedance mode.
  * @param Mode  specifies the high impedance mode
  *          This parameter can be one of the following values:
  *            @arg SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE: VREF+ pin is internally connect to VREFINT output.
  *            @arg SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE: VREF+ pin is high impedance.
  * @retval None
  */
void HAL_SYSCFG_VREFBUF_HighImpedanceConfig(uint32_t Mode)
{
  /* Check the parameters */
  assert_param(IS_SYSCFG_VREFBUF_HIGH_IMPEDANCE(Mode));

  MODIFY_REG(VREFBUF->CSR, VREFBUF_CSR_HIZ, Mode);
}

/**
  * @brief  Tune the Internal Voltage Reference buffer (VREFBUF).
  * @retval None
  */
void HAL_SYSCFG_VREFBUF_TrimmingConfig(uint32_t TrimmingValue)
{
  /* Check the parameters */
  assert_param(IS_SYSCFG_VREFBUF_TRIMMING(TrimmingValue));

  MODIFY_REG(VREFBUF->CCR, VREFBUF_CCR_TRIM, TrimmingValue);
}

/**
  * @brief  Enable the Internal Voltage Reference buffer (VREFBUF).
  * @retval HAL_OK/HAL_TIMEOUT
  */
HAL_StatusTypeDef HAL_SYSCFG_EnableVREFBUF(void)
{
  uint32_t  tickstart = 0;

  SET_BIT(VREFBUF->CSR, VREFBUF_CSR_ENVR);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait for VRR bit  */
  while(READ_BIT(VREFBUF->CSR, VREFBUF_CSR_VRR) == RESET)
  {
    if((HAL_GetTick() - tickstart) > VREFBUF_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

/**
  * @brief  Disable the Internal Voltage Reference buffer (VREFBUF).
  *
  * @retval None
  */
void HAL_SYSCFG_DisableVREFBUF(void)
{
  CLEAR_BIT(VREFBUF->CSR, VREFBUF_CSR_ENVR);
}

/**
  * @brief  Ethernet PHY Interface Selection either MII or RMII
  * @param  SYSCFG_ETHInterface: Selects the Ethernet PHY interface
  *   This parameter can be one of the following values:
  *   @arg SYSCFG_ETH_MII  : Select the Media Independent Interface
  *   @arg SYSCFG_ETH_GMII : Select the Gigabit Media Independent Interface
  *   @arg SYSCFG_ETH_RGMII: Select the Gigabit Reduced Media Independent Interface
  *   @arg SYSCFG_ETH_RMII : Select the Reduced Media Independent Interface
  * @retval None
  */
void HAL_SYSCFG_ETHInterfaceSelect(uint32_t SYSCFG_ETHInterface)
{
  /* Check the parameter */
  assert_param(IS_SYSCFG_ETHERNET_CONFIG(SYSCFG_ETHInterface));
  SET_BIT(SYSCFG->PMCCLRR, SYSCFG_PMCSETR_ETH_SEL|SYSCFG_PMCSETR_ETH_SELMII_SEL);
  SET_BIT(SYSCFG->PMCSETR, (uint32_t)(SYSCFG_ETHInterface));
}

/**
  * @brief  Analog Switch control for dual analog pads.
  * @param  SYSCFG_AnalogSwitch: Selects the analog pad
  *   This parameter can be one or a combination of the following values:
  *   @arg SYSCFG_SWITCH_PA0 : Select PA0 analog switch
  *   @arg SYSCFG_SWITCH_PA1:  Select PA1 analog switch
  * @param  SYSCFG_SwitchState: Open or Close the analog switch between dual pads (
  *   This parameter can be one or a combination of the following values:
  *   @arg SYSCFG_SWITCH_PA0_OPEN
  *   @arg SYSCFG_SWITCH_PA0_CLOSE
  *   @arg SYSCFG_SWITCH_PA1_OPEN
  *   @arg SYSCFG_SWITCH_PA1_CLOSE
  * @retval None
  */

void HAL_SYSCFG_AnalogSwitchConfig(uint32_t SYSCFG_AnalogSwitch , uint32_t SYSCFG_SwitchState )
{
  /* Check the parameter */
  assert_param(IS_SYSCFG_ANALOG_SWITCH(SYSCFG_AnalogSwitch));
  assert_param(IS_SYSCFG_SWITCH_STATE(SYSCFG_SwitchState));
  SET_BIT(SYSCFG->PMCCLRR, SYSCFG_AnalogSwitch);
  SET_BIT(SYSCFG->PMCSETR, (uint32_t)(SYSCFG_SwitchState));

}


/**
  * @brief  Enables the booster to reduce the total harmonic distortion of the analog
  *         switch when the supply voltage is lower than 2.7 V.
  * @note   Activating the booster allows to guaranty the analog switch AC performance
  *         when the supply voltage is below 2.7 V: in this case, the analog switch
  *         performance is the same on the full voltage range
  * @retval None
  */
void HAL_SYSCFG_EnableBOOST(void)
{
  SET_BIT(SYSCFG->PMCSETR, SYSCFG_PMCSETR_EN_BOOSTER) ;
}

/**
  * @brief  Disables the booster
  * @note   Activating the booster allows to guaranty the analog switch AC performance
  *         when the supply voltage is below 2.7 V: in this case, the analog switch
  *         performance is the same on the full voltage range
  * @retval None
  */
void HAL_SYSCFG_DisableBOOST(void)
{
  SET_BIT(SYSCFG->PMCCLRR, SYSCFG_PMCCLRR_EN_BOOSTER) ;
}


/**
  * @brief  Enables the I/O Compensation Cell.
  * @note   The I/O compensation cell can be used only when the device supply
  *         voltage ranges from 2.4 to 3.6 V.
  * @retval None
  */
void HAL_EnableCompensationCell(void)
{
#if defined(CORE_CM4)
  SET_BIT(SYSCFG->CMPENSETR, SYSCFG_CMPENSETR_MCU_EN) ;
#elif defined(CORE_CA7)
  SET_BIT(SYSCFG->CMPENSETR, SYSCFG_CMPENSETR_MPU_EN) ;
#endif
}

/**
  * @brief  Power-down the I/O Compensation Cell.
  * @note   The I/O compensation cell can be used only when the device supply
  *         voltage ranges from 2.4 to 3.6 V.
  * @retval None
  */
void HAL_DisableCompensationCell(void)
{
#if defined(CORE_CM4)
  SET_BIT(SYSCFG->CMPENCLRR, SYSCFG_CMPENCLRR_MCU_EN) ;
#elif defined(CORE_CA7)
  SET_BIT(SYSCFG->CMPENCLRR, SYSCFG_CMPENCLRR_MPU_EN) ;
#endif
}


/**
  * @brief  To Enable optimize the I/O speed when the product voltage is low.
  * @note   This bit is active only if PRODUCT_BELOW_25V user option bit is set. It must be
  *         used only if the product supply voltage is below 2.5 V. Setting this bit when VDD is
  *         higher than 2.5 V might be destructive.
  * @param  SYSCFG_HighSpeedSignal: Signal selection (TRACE, QUADSPI...)
  *   This parameter can be one or a combination of the following values:
  *   @arg SYSCFG_HIGHSPEED_TRACE_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_QUADSPI_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_ETH_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_SDMMC_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_SPI_SIGNAL
  * @retval None
  */
void HAL_SYSCFG_EnableIOSpeedOptimize(uint32_t SYSCFG_HighSpeedSignal )
{
  SET_BIT(SYSCFG->IOCTRLSETR, SYSCFG_HighSpeedSignal) ;
}

/**
  * @brief  To Disable optimize the I/O speed when the product voltage is low.
  * @note   This bit is active only if PRODUCT_BELOW_25V user option bit is set. It must be
  *         used only if the product supply voltage is below 2.5 V. Setting this bit when VDD is
  *         higher than 2.5 V might be destructive.
  * @param  SYSCFG_HighSpeedSignal: Signal selection (TRACE, QUADSPI...)
  *   This parameter can be one or a combination of the following values:
  *   @arg SYSCFG_HIGHSPEED_TRACE_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_QUADSPI_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_ETH_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_SDMMC_SIGNAL
  *   @arg SYSCFG_HIGHSPEED_SPI_SIGNAL
  * @retval None
  */
void HAL_SYSCFG_DisableIOSpeedOptimize(uint32_t SYSCFG_HighSpeedSignal )
{
  SET_BIT(SYSCFG->IOCTRLCLRR, SYSCFG_HighSpeedSignal) ;
}

/**
  * @brief  Code selection for the I/O Compensation cell
  * @param  SYSCFG_CompCode: Selects the code to be applied for the I/O compensation cell
  *   This parameter can be one of the following values:
  *   @arg SYSCFG_CELL_CODE : Select Code from the cell (available in the SYSCFG_CCVR)
  *   @arg SYSCFG_REGISTER_CODE: Select Code from the SYSCFG compensation cell code register (SYSCFG_CCCR)
  * @retval None
  */
void HAL_SYSCFG_CompensationCodeSelect(uint32_t SYSCFG_CompCode)
{
  /* Check the parameter */
  assert_param(IS_SYSCFG_CODE_SELECT(SYSCFG_CompCode));
  MODIFY_REG(SYSCFG->CMPCR, SYSCFG_CMPCR_SW_CTRL, (uint32_t)(SYSCFG_CompCode));
}

/**
  * @brief  Code selection for the I/O Compensation cell
  * @param  SYSCFG_PMOSCode: PMOS compensation code
  *         This code is applied to the I/O compensation cell when the CS bit of the
  *          SYSCFG_CMPCR is set
  * @param  SYSCFG_NMOSCode: NMOS compensation code
  *         This code is applied to the I/O compensation cell when the CS bit of the
  *          SYSCFG_CMPCR is set
  * @retval None
  */
void HAL_SYSCFG_CompensationCodeConfig(uint32_t SYSCFG_PMOSCode, uint32_t SYSCFG_NMOSCode )
{
  /* Check the parameter */
  assert_param(IS_SYSCFG_CODE_CONFIG(SYSCFG_PMOSCode));
  assert_param(IS_SYSCFG_CODE_CONFIG(SYSCFG_NMOSCode));
  MODIFY_REG(SYSCFG->CMPCR, SYSCFG_CMPCR_RANSRC|SYSCFG_CMPCR_RAPSRC, (((uint32_t)(SYSCFG_PMOSCode)<< 4)|(uint32_t)(SYSCFG_NMOSCode)) );
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

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
