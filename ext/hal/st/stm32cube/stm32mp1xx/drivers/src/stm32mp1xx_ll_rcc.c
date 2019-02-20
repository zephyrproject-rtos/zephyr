/**
  ******************************************************************************
  * @file    stm32mp1xx_ll_rcc.c
  * @author  MCD Application Team
  * @version $VERSION$
  * @date    $DATE$
  * @brief   RCC LL module driver.
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
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_ll_rcc.h"
#ifdef  USE_FULL_ASSERT
#include "stm32_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32MP1xx_LL_Driver
  * @{
  */

#if defined(RCC)
/** @addtogroup RCC_LL
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @addtogroup RCC_LL_Private_Macros
  * @{
  */

#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_I2C12_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_I2C35_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_I2C46_CLKSOURCE))

#define IS_LL_RCC_SAI_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_SAI1_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_SAI2_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_SAI3_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_SAI4_CLKSOURCE))

#define IS_LL_RCC_SPI_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_SPI1_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_SPI23_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_SPI45_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_SPI6_CLKSOURCE))

#define IS_LL_RCC_UART_CLKSOURCE(__VALUE__)     (((__VALUE__) == LL_RCC_USART1_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_UART24_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_UART35_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_USART6_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_UART78_CLKSOURCE))

#define IS_LL_RCC_SDMMC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_SDMMC12_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_SDMMC3_CLKSOURCE))

#define IS_LL_RCC_ETH_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_ETH_CLKSOURCE))

#define IS_LL_RCC_QSPI_CLKSOURCE(__VALUE__)     (((__VALUE__) == LL_RCC_QSPI_CLKSOURCE))

#define IS_LL_RCC_FMC_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_FMC_CLKSOURCE))

#define IS_LL_RCC_FDCAN_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_FDCAN_CLKSOURCE))

#define IS_LL_RCC_SPDIFRX_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_SPDIFRX_CLKSOURCE))

#define IS_LL_RCC_CEC_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_CEC_CLKSOURCE))

#define IS_LL_RCC_USBPHY_CLKSOURCE(__VALUE__)   (((__VALUE__) == LL_RCC_USBPHY_CLKSOURCE))

#define IS_LL_RCC_USBO_CLKSOURCE(__VALUE__)     (((__VALUE__) == LL_RCC_USBO_CLKSOURCE))

#define IS_LL_RCC_RNG_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_RNG1_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_RNG2_CLKSOURCE))

#define IS_LL_RCC_CKPER_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_CKPER_CLKSOURCE))

#define IS_LL_RCC_STGEN_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_STGEN_CLKSOURCE))

#define IS_LL_RCC_DSI_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_DSI_CLKSOURCE))

#define IS_LL_RCC_ADC_CLKSOURCE(__VALUE__)      (((__VALUE__) == LL_RCC_ADC_CLKSOURCE))

#define IS_LL_RCC_LPTIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_LPTIM1_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_LPTIM23_CLKSOURCE) \
                                               || ((__VALUE__) == LL_RCC_LPTIM45_CLKSOURCE))

#define IS_LL_RCC_DFSDM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_DFSDM_CLKSOURCE))

#define IS_LL_RCC_TIMGPRES(__VALUE__)           (((__VALUE__) == LL_RCC_TIMG1PRES) \
                                               || ((__VALUE__) == LL_RCC_TIMG2PRES))

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCC_LL_Private_Functions RCC Private functions
  * @{
  */
uint32_t RCC_GetMPUSSClockFreq(void);
uint32_t RCC_GetAXISSClockFreq(void);
uint32_t RCC_GetMCUSSClockFreq(void);
uint32_t RCC_GetACLKClockFreq(uint32_t AXISS_Frequency);
uint32_t RCC_GetMLHCLKClockFreq(uint32_t MCUSS_Frequency);
uint32_t RCC_GetPCLK1ClockFreq(uint32_t MLHCLK_Frequency);
uint32_t RCC_GetPCLK2ClockFreq(uint32_t MLHCLK_Frequency);
uint32_t RCC_GetPCLK3ClockFreq(uint32_t MLHCLK_Frequency);
uint32_t RCC_GetPCLK4ClockFreq(uint32_t ACLK_Frequency);
uint32_t RCC_GetPCLK5ClockFreq(uint32_t ACLK_Frequency);

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCC_LL_Exported_Functions
  * @{
  */

/** @addtogroup RCC_LL_EF_Init
  * @{
  */

/**
  * @brief  Reset the RCC clock configuration to the default reset state.
  * @note   The default reset state of the clock configuration is given below:
  *         - HSI  ON and used as system clock source
  *         - HSE, CSI, PLL1, PLL2, PLL3 and PLL4 OFF
  *         - AHBs and APBs bus pre-scalers set to 1.
  *         - MCO1 and MCO2 OFF
  *         - All interrupts disabled (except Wake-up from CSTOP Interrupt Enable)
  * @note   This function doesn't modify the configuration of the
  *         - Peripheral clocks
  *         - LSI, LSE and RTC clocks
  *         - HSECSS and LSECSS
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RCC registers are de-initialized
  *          - ERROR: not applicable
  */
ErrorStatus LL_RCC_DeInit(void)
{
  /* Set HSION bit */
  LL_RCC_HSI_Enable();

  /* Insure HSIRDY bit is set before writing default MSIRANGE value */
  while (LL_RCC_HSI_IsReady() == 0U)
  {}

  /* Reset MPU Clock Selection */
  LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_HSI);

  /* Reset AXI Sub-System Clock Selection */
  LL_RCC_SetAXISSClkSource(LL_RCC_AXISS_CLKSOURCE_HSI);

  /* Reset MCU Sub-System Clock Selection */
  LL_RCC_SetMCUSSClkSource(LL_RCC_MCUSS_CLKSOURCE_HSI);

  /* Reset RCC MPU Clock Divider */
  LL_RCC_SetMPUPrescaler(LL_RCC_MPU_DIV_2);

  /* Reset RCC ACLK (ACLK, HCLK5 and HCLK6) Clock Divider */
  LL_RCC_SetACLKPrescaler(LL_RCC_AXI_DIV_1);

  /* Reset RCC APB4 Clock Divider */
  LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_1);

  /* Reset RCC APB5 Clock Divider */
  LL_RCC_SetAPB5Prescaler(LL_RCC_APB5_DIV_1);

  /* Reset RCC MLHCLK (MLHCLK, MCU, FCLK, HCLK4, HCLK3, HCLK2 and HCLK1) Clock Divider */
  LL_RCC_SetMLHCLKPrescaler(LL_RCC_MCU_DIV_1);

  /* Reset RCC APB1 Clock Divider */
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

  /* Reset RCC APB2 Clock Divider */
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  /* Reset RCC APB3 Clock Divider */
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);

  /* Reset MCO1 Configuration */
  CLEAR_BIT(RCC->MCO1CFGR, RCC_MCO1CFGR_MCO1ON);
  LL_RCC_ConfigMCO(LL_RCC_MCO1SOURCE_HSI, LL_RCC_MCO1_DIV_1);

  /* Reset MCO2 Configuration */
  CLEAR_BIT(RCC->MCO2CFGR, RCC_MCO2CFGR_MCO2ON);
  LL_RCC_ConfigMCO(LL_RCC_MCO2SOURCE_MPU, LL_RCC_MCO2_DIV_1);

  /* Disable PLL1 outputs */
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN | RCC_PLL1CR_DIVQEN |
            RCC_PLL1CR_DIVREN);

  /* Disable PLL1 */
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_PLLON);

  /* Wait for PLL1 READY bit to be reset */
  while (LL_RCC_PLL1_IsReady() != 0U)
  {}

  /* Clear remaining SSCG_CTRL bit */
  LL_RCC_PLL1CSG_Disable();

  /* Disable PLL2 outputs */
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVPEN | RCC_PLL2CR_DIVQEN |
            RCC_PLL2CR_DIVREN);

  /* Disable PLL2 */
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_PLLON);

  /* Wait for PLL2 READY bit to be reset */
  while (LL_RCC_PLL2_IsReady() != 0U)
  {}

  /* Clear remaining SSCG_CTRL bit */
  LL_RCC_PLL2CSG_Disable();

  /* Disable PLL3 outputs */
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVPEN | RCC_PLL3CR_DIVQEN |
            RCC_PLL3CR_DIVREN);

  /* Disable PLL3 */
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_PLLON);

  /* Wait for PLL3 READY bit to be reset */
  while (LL_RCC_PLL3_IsReady() != 0U)
  {}

  /* Clear remaining SSCG_CTRL bit */
  LL_RCC_PLL3CSG_Disable();

  /* Disable PLL4 outputs */
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVPEN | RCC_PLL4CR_DIVQEN |
            RCC_PLL4CR_DIVREN);

  /* Disable PLL4 */
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_PLLON);

  /* Wait for PLL4 READY bit to be reset */
  while (LL_RCC_PLL4_IsReady() != 0U)
  {}

  /* Clear remaining SSCG_CTRL bit */
  LL_RCC_PLL4CSG_Disable();

  /* Clear remaining SSCG_CTRL bit */
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_SSCG_CTRL);

  /* Reset PLL 1 and 2 Ref. Clock Selection Register */
  MODIFY_REG(RCC->RCK12SELR, (RCC_RCK12SELR_PLL12SRC), RCC_RCK12SELR_PLL12SRC_0);

  /* Reset RCC PLL 3 Ref. Clock Selection Register */
  MODIFY_REG(RCC->RCK3SELR, (RCC_RCK3SELR_PLL3SRC), RCC_RCK3SELR_PLL3SRC_0);

  /* Reset PLL4 Ref. Clock Selection Register */
  MODIFY_REG(RCC->RCK4SELR, (RCC_RCK4SELR_PLL4SRC), RCC_RCK4SELR_PLL4SRC_0);

  /* Reset RCC PLL1 Configuration Register 1 */
  WRITE_REG(RCC->PLL1CFGR1, 0x00010031U);

  /* Reset RCC PLL1 Configuration Register 2 */
  WRITE_REG(RCC->PLL1CFGR2, 0x00010100U);

  /* Reset RCC PLL1 Fractional Register */
  CLEAR_REG(RCC->PLL1FRACR);

  /* Reset RCC PLL1 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL1CSGR);

  /* Reset RCC PLL2 Configuration Register 1 */
  WRITE_REG(RCC->PLL2CFGR1, 0x00010063U);

  /* Reset RCC PLL2 Configuration Register 2 */
  WRITE_REG(RCC->PLL2CFGR2, 0x00010101U);

  /* Reset RCC PLL2 Fractional Register */
  CLEAR_REG(RCC->PLL2FRACR);

  /* Reset RCC PLL2 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL2CSGR);

  /* Reset RCC PLL3 Configuration Register 1 */
  WRITE_REG(RCC->PLL3CFGR1, 0x00010031U);

  /* Reset RCC PLL3 Configuration Register 2 */
  WRITE_REG(RCC->PLL3CFGR2, 0x00010101U);

  /* Reset RCC PLL3 Fractional Register */
  CLEAR_REG(RCC->PLL3FRACR);

  /* Reset RCC PLL3 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL3CSGR);

  /* Reset RCC PLL4 Configuration Register 1 */
  WRITE_REG(RCC->PLL4CFGR1, 0x00010031U);

  /* Reset RCC PLL4 Configuration Register 2 */
  WRITE_REG(RCC->PLL4CFGR2, 0x00000000U);

  /* Reset RCC PLL4 Fractional Register */
  CLEAR_REG(RCC->PLL4FRACR);

  /* Reset RCC PLL4 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL4CSGR);

  /* Reset HSIDIV once PLLs are off */
  LL_RCC_HSI_SetDivider(LL_RCC_HSI_DIV_1);

  /* Wait for HSIDIV READY bit to set */
  while (LL_RCC_HSI_IsDividerReady() != 1U)
  {}

  /* Set HSITRIM bits to the reset value*/
  LL_RCC_HSI_SetCalibTrimming(0x0U);

  /* Reset the Oscillator Enable Control registers */
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSIKERON | RCC_OCENCLRR_CSION |
            RCC_OCENCLRR_CSIKERON | RCC_OCENCLRR_DIGBYP | RCC_OCENCLRR_HSEON |
            RCC_OCENCLRR_HSEKERON | RCC_OCENCLRR_HSEBYP);

  /* Reset CSITRIM value */
  LL_RCC_CSI_SetCalibTrimming(0x10U);

#ifdef CORE_CM4
  /* Reset RCC Clock Source Interrupt Enable Register */
  CLEAR_BIT(RCC->MC_CIER, (RCC_MC_CIER_LSIRDYIE | RCC_MC_CIER_LSERDYIE |
                           RCC_MC_CIER_HSIRDYIE | RCC_MC_CIER_HSERDYIE | RCC_MC_CIER_CSIRDYIE |
                           RCC_MC_CIER_PLL1DYIE | RCC_MC_CIER_PLL2DYIE | RCC_MC_CIER_PLL3DYIE |
                           RCC_MC_CIER_PLL4DYIE | RCC_MC_CIER_LSECSSIE | RCC_MC_CIER_WKUPIE));

  /* Clear all RCC MCU interrupt flags */
  SET_BIT(RCC->MC_CIFR, (RCC_MC_CIFR_LSIRDYF | RCC_MC_CIFR_LSERDYF |
                         RCC_MC_CIFR_HSIRDYF | RCC_MC_CIFR_HSERDYF | RCC_MC_CIFR_CSIRDYF |
                         RCC_MC_CIFR_PLL1DYF | RCC_MC_CIFR_PLL2DYF | RCC_MC_CIFR_PLL3DYF |
                         RCC_MC_CIFR_PLL4DYF | RCC_MC_CIFR_LSECSSF));

  /* Clear all RCC MCU Reset Flags */
  SET_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_WWDG1RSTF |
          RCC_MC_RSTSCLRR_IWDG2RSTF | RCC_MC_RSTSCLRR_IWDG1RSTF |
          RCC_MC_RSTSCLRR_MCSYSRSTF | RCC_MC_RSTSCLRR_MPSYSRSTF |
          RCC_MC_RSTSCLRR_MCURSTF | RCC_MC_RSTSCLRR_VCORERSTF |
          RCC_MC_RSTSCLRR_HCSSRSTF | RCC_MC_RSTSCLRR_PADRSTF |
          RCC_MC_RSTSCLRR_BORRSTF | RCC_MC_RSTSCLRR_PORRSTF);
#endif

  return SUCCESS;
}

/**
  * @}
  */

/** @addtogroup RCC_LL_EF_Get_Freq
  * @brief  Return the frequencies of different on chip clocks; MPUSS, AXISS,
  *         MCUSS, ACLK, MLHCLK, APB[5:1] AHB[6:1] and MCU buses clocks and
  *         different peripheral clocks available on the device.
  * @note   If MCUSS source is CSI, function returns values based on CSI_VALUE(*)
  * @note   If MPUSS/AXISS/MCUSS source is HSI, function returns values based on HSI_VALUE(**)
  * @note   If MPUSS/AXISS/MCUSS source is HSE, function returns values based on HSE_VALUE(***)
  * @note   If MPUSS/AXISS/MCUSS source is PLLx, function returns values based
  *         on HSE_VALUE(***), or HSI_VALUE(**) or CSI_VALUE(*) or EXTERNAL_CLOCK_VALUE(****)
  *         multiplied/divided by the PLLx factors where 'PLLx' can be PLL1, PLL2
  *         or PLL3.
  * @note   (*)   CSI_VALUE is a constant defined in this file (default value
  *               4 MHz) but the real value may vary depending on the variations
  *               in voltage and temperature.
  * @note   (**)  HSI_VALUE is a constant defined in this file (default value
  *               64 MHz) but the real value may vary depending on the variations
  *               in voltage and temperature.
  * @note   (***) HSE_VALUE is a constant defined in this file (default value
  *               24 MHz), user has to ensure that HSE_VALUE is same as the real
  *               frequency of the crystal used. Otherwise, this function may
  *               have wrong result.
  * @note  (****) EXTERNAL_CLOCK_VALUE is a constant defined in this file
  *               (default value 12.288 MHz), user has to ensure that
  *               EXTERNAL_CLOCK_VALUE is same as the real frequency of the
  *               I2S_CKIN external oscillator used. Otherwise, this function may
  *               have wrong result.
  * @note   The result of this function could be incorrect when using fractional
  *         value for HSE crystal.
  * @note   This function can be used by the user application to compute the
  *         baud-rate for the communication peripherals or configure other parameters.
  * @{
  */

/**
  * @brief  Return the frequencies of different on chip clocks;  MPUSS, AXISS,
  *         MCUSS, ACLK, MLHCLK, APB[5:1] AHB[6:1] and MCU buses clocks.
  * @note   Each time MPUSS, AXISS, MCUSS, ACLK, MLHCLK, APB[5:1] AHB[6:1] and/or
  *         MCU clock changes, this function must be called to update structure
  *         fields. Otherwise, any configuration based on this function will
  *         be incorrect.
  * @param  RCC_Clocks pointer to a @ref LL_RCC_ClocksTypeDef structure which
  *         will hold the clocks frequencies.
  * @retval None
  */
void LL_RCC_GetSystemClocksFreq(LL_RCC_ClocksTypeDef *RCC_Clocks)
{
  /*!< MPUSS clock frequency */
  RCC_Clocks->MPUSS_Frequency = RCC_GetMPUSSClockFreq();

  /*!< AXISS clock frequency */
  RCC_Clocks->AXISS_Frequency = RCC_GetAXISSClockFreq();

  /*!< MCUSS clock frequency */
  RCC_Clocks->MCUSS_Frequency = RCC_GetMCUSSClockFreq();

  /*!< ACLK clock frequency */
  RCC_Clocks->ACLK_Frequency = RCC_GetACLKClockFreq(RCC_Clocks->AXISS_Frequency);

  /*!< MLHCLK clock frequency */
  RCC_Clocks->MLHCLK_Frequency = RCC_GetMLHCLKClockFreq(RCC_Clocks->MCUSS_Frequency);

  /*!< PCLK1 clock frequency */
  RCC_Clocks->PCLK1_Frequency = RCC_GetPCLK1ClockFreq(RCC_Clocks->MLHCLK_Frequency);

  /*!< PCLK2 clock frequency */
  RCC_Clocks->PCLK2_Frequency = RCC_GetPCLK2ClockFreq(RCC_Clocks->MLHCLK_Frequency);

  /*!< PCLK3 clock frequency */
  RCC_Clocks->PCLK3_Frequency = RCC_GetPCLK3ClockFreq(RCC_Clocks->MLHCLK_Frequency);

  /*!< PCLK4 clock frequency */
  RCC_Clocks->PCLK4_Frequency = RCC_GetPCLK4ClockFreq(RCC_Clocks->ACLK_Frequency);

  /*!< PCLK5 clock frequency */
  RCC_Clocks->PCLK5_Frequency = RCC_GetPCLK5ClockFreq(RCC_Clocks->ACLK_Frequency);

  /*!< HCLK1 clock frequency */
  RCC_Clocks->HCLK1_Frequency = RCC_Clocks->MLHCLK_Frequency;

  /*!< HCLK2 clock frequency */
  RCC_Clocks->HCLK2_Frequency = RCC_Clocks->MLHCLK_Frequency;

  /*!< HCLK3 clock frequency */
  RCC_Clocks->HCLK3_Frequency = RCC_Clocks->MLHCLK_Frequency;

  /*!< HCLK4 clock frequency */
  RCC_Clocks->HCLK4_Frequency = RCC_Clocks->MLHCLK_Frequency;

  /*!< MCU clock frequency */
  RCC_Clocks->MCU_Frequency = RCC_Clocks->MLHCLK_Frequency;

  /*!< HCLK5 clock frequency */
  RCC_Clocks->HCLK5_Frequency = RCC_Clocks->ACLK_Frequency;

  /*!< HCLK6 clock frequency */
  RCC_Clocks->HCLK6_Frequency = RCC_Clocks->ACLK_Frequency;
}

/**
  * @brief  Return PLL1 clocks frequencies
  * @note   LL_RCC_PERIPH_FREQUENCY_NO returned for non activated output or
  *         oscillator not ready.
  *         For PLL1Q and PLL1R LL_RCC_PERIPH_FREQUENCY_NO is returned as these
  *         outputs are not available.
  * @retval None
  */
void LL_RCC_GetPLL1ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks)
{
  uint32_t pllinputfreq = LL_RCC_PERIPH_FREQUENCY_NO, pllsource;
  uint32_t m, n, fracv = 0U;

  /* Fout1_ck = ((HSE_VALUE or HSI_VALUE/HSIDIV) / PLLM) * (PLLN + (FRACV/(2^13)))
   * Where Fout1_ck is the PLL1 frequency before PQR post-dividers
   * PLL_P_Frequency = Fout1_ck/DIVP (if output enabled)
   */

  pllsource = LL_RCC_PLL12_GetSource();

  switch (pllsource)
  {
    case LL_RCC_PLL12SOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        pllinputfreq = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_PLL12SOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    case LL_RCC_PLL12SOURCE_NONE:
    default:
      /* PLL12 clock disabled */
      break;
  }

  PLL_Clocks->PLL_P_Frequency = 0U;
  PLL_Clocks->PLL_Q_Frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  PLL_Clocks->PLL_R_Frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  m = LL_RCC_PLL1_GetM();
  n = LL_RCC_PLL1_GetN();
  if (LL_RCC_PLL1FRACV_IsEnabled() != 0U)
  {
    fracv = LL_RCC_PLL1_GetFRACV();
  }

  if (m != 0U)
  {
    if (LL_RCC_PLL1P_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_P_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL1_GetP());
    }
  }
}

/**
  * @brief  Return PLL2 clocks frequencies
  * @note   LL_RCC_PERIPH_FREQUENCY_NO returned for non activated output or
  *         oscillator not ready
  * @retval None
  */
void LL_RCC_GetPLL2ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks)
{
  uint32_t pllinputfreq = LL_RCC_PERIPH_FREQUENCY_NO, pllsource;
  uint32_t m, n, fracv = 0U;

  /* Fout2_ck = ((HSE_VALUE or HSI_VALUE/HSIDIV) / PLLM) * (PLLN + (FRACV/(2^13)))
   * Where Fout2_ck is the PLL2 frequency before PQR post-dividers
   * PLL_P_Frequency = Fout2_ck/DIVP (if output enabled)
   * PLL_Q_Frequency = Fout2_ck/DIVQ (if output enabled)
   * PLL_R_Frequency = Fout2_ck/DIVR (if output enabled)
   */
  pllsource = LL_RCC_PLL12_GetSource();

  switch (pllsource)
  {
    case LL_RCC_PLL12SOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        pllinputfreq = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_PLL12SOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    case LL_RCC_PLL12SOURCE_NONE:
    default:
      /* PLL12 clock disabled */
      break;
  }

  PLL_Clocks->PLL_P_Frequency = 0U;
  PLL_Clocks->PLL_Q_Frequency = 0U;
  PLL_Clocks->PLL_R_Frequency = 0U;

  m = LL_RCC_PLL2_GetM();
  n = LL_RCC_PLL2_GetN();
  if (LL_RCC_PLL2FRACV_IsEnabled() != 0U)
  {
    fracv = LL_RCC_PLL2_GetFRACV();
  }

  if (m != 0U)
  {
    if (LL_RCC_PLL2P_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_P_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL2_GetP());
    }

    if (LL_RCC_PLL2Q_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_Q_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL2_GetQ());
    }

    if (LL_RCC_PLL2R_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_R_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL2_GetR());
    }
  }
}

/**
  * @brief  Return PLL3 clocks frequencies
  * @note   LL_RCC_PERIPH_FREQUENCY_NO returned for non activated output or
  *         oscillator not ready
  * @retval None
  */
void LL_RCC_GetPLL3ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks)
{
  uint32_t pllinputfreq = LL_RCC_PERIPH_FREQUENCY_NO, pllsource;
  uint32_t m, n, fracv = 0U;

  /* Fout3_ck = ((HSE_VALUE, CSI_VALUE or HSI_VALUE/HSIDIV) / PLLM) * (PLLN + (FRACV/(2^13)))
   * Where Fout3_ck is the PLL3 frequency before PQR post-dividers
   * PLL_P_Frequency = Fout3_ck/DIVP (if output enabled)
   * PLL_Q_Frequency = Fout3_ck/DIVQ (if output enabled)
   * PLL_R_Frequency = Fout3_ck/DIVR (if output enabled)
   */

  pllsource = LL_RCC_PLL3_GetSource();

  switch (pllsource)
  {
    case LL_RCC_PLL3SOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        pllinputfreq = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_PLL3SOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    case LL_RCC_PLL3SOURCE_CSI:
      if (LL_RCC_CSI_IsReady() != 0U)
      {
        pllinputfreq = CSI_VALUE;
      }
      break;

    case LL_RCC_PLL3SOURCE_NONE:
    default:
      /* PLL3 clock disabled */
      break;
  }

  PLL_Clocks->PLL_P_Frequency = 0U;
  PLL_Clocks->PLL_Q_Frequency = 0U;
  PLL_Clocks->PLL_R_Frequency = 0U;

  m = LL_RCC_PLL3_GetM();
  n = LL_RCC_PLL3_GetN();
  if (LL_RCC_PLL3FRACV_IsEnabled() != 0U)
  {
    fracv = LL_RCC_PLL3_GetFRACV();
  }

  if (m != 0U)
  {
    if (LL_RCC_PLL3P_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_P_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL3_GetP());
    }

    if (LL_RCC_PLL3Q_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_Q_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL3_GetQ());
    }

    if (LL_RCC_PLL3R_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_R_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL3_GetR());
    }
  }
}

/**
  * @brief  Return PLL4 clocks frequencies
  * @note   LL_RCC_PERIPH_FREQUENCY_NO returned for non activated output or
  *         oscillator not ready
  * @retval None
  */
void LL_RCC_GetPLL4ClockFreq(LL_PLL_ClocksTypeDef *PLL_Clocks)
{
  uint32_t pllinputfreq = LL_RCC_PERIPH_FREQUENCY_NO, pllsource;
  uint32_t m, n, fracv = 0U;

  /* Fout4_ck = ((HSE_VALUE, CSI_VALUE, EXTERNAL_CLOCK_VALUE or HSI_VALUE/HSIDIV) / PLLM) * (PLLN + (FRACV/(2^13)))
   * Where Fout4_ck is the PLL4 frequency before PQR post-dividers
   * PLL_P_Frequency = Fout4_ck/DIVP (if output enabled)
   * PLL_Q_Frequency = Fout4_ck/DIVQ (if output enabled)
   * PLL_R_Frequency = Fout4_ck/DIVR (if output enabled)
   */

  pllsource = LL_RCC_PLL4_GetSource();

  switch (pllsource)
  {
    case LL_RCC_PLL4SOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        pllinputfreq = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_PLL4SOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    case LL_RCC_PLL4SOURCE_CSI:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    case LL_RCC_PLL4SOURCE_I2SCKIN:
      pllinputfreq = EXTERNAL_CLOCK_VALUE;
      break;
  }

  PLL_Clocks->PLL_P_Frequency = 0U;
  PLL_Clocks->PLL_Q_Frequency = 0U;
  PLL_Clocks->PLL_R_Frequency = 0U;

  m = LL_RCC_PLL4_GetM();
  n = LL_RCC_PLL4_GetN();
  if (LL_RCC_PLL4FRACV_IsEnabled() != 0U)
  {
    fracv = LL_RCC_PLL4_GetFRACV();
  }

  if (m != 0U)
  {
    if (LL_RCC_PLL4P_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_P_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL4_GetP());
    }

    if (LL_RCC_PLL4Q_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_Q_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL4_GetQ());
    }

    if (LL_RCC_PLL4R_IsEnabled() != 0U)
    {
      PLL_Clocks->PLL_R_Frequency = LL_RCC_CalcPLLClockFreq(pllinputfreq, m, n, fracv, LL_RCC_PLL4_GetR());
    }
  }
}

/**
  * @brief  Helper function to calculate the PLL frequency output
  * @note   ex: @ref LL_RCC_CalcPLLClockFreq (HSE_VALUE, @ref LL_RCC_PLL1_GetM (),
  *             @ref LL_RCC_PLL1_GetN (), @ref LL_RCC_PLL1_GetFRACV (), @ref LL_RCC_PLL1_GetP ());
  * @param  PLLInputFreq PLL Input frequency (based on HSE/(HSI/HSIDIV)/CSI/I2SCKIN)
  * @param  M      Between 1 and 64
  * @param  N      Between 4 and 512 on fractional mode (PLL1, PLL2, PLL3 and PLL4).
  *                Between 25 and 100 on integer mode for PLL1 and PLL2.
  *                Between 25 and 200 on integer mode for PLL3 and PLL4.
  * @param  FRACV  Between 0 and 0x1FFF
  * @param  PQR    VCO output divider (P, Q or R)
  *                Between 1 and 128. For PLL1 and PLL2, only the bypass
  *                (division by 1) and even divisions are allowed. For PLL3,
  *                only the even divisions are allowed. If those conditions are
  *                not respected the behavior of the circuit is not guaranteed.
  * @retval PLL clock frequency (in Hz)
  */
uint32_t LL_RCC_CalcPLLClockFreq(uint32_t PLLInputFreq, uint32_t M, uint32_t N, uint32_t FRACV, uint32_t PQR)
{
  float freq;

  freq = ((float)PLLInputFreq / (float)M) * ((float)N + ((float)FRACV / (float)0x2000));

  freq = freq / (float)PQR;

  return (uint32_t)freq;
}

/**
  * @brief  Return I2Cx clock frequency
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C12_CLKSOURCE
  *         @arg @ref LL_RCC_I2C35_CLKSOURCE
  *         @arg @ref LL_RCC_I2C46_CLKSOURCE
  * @retval I2C clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetI2CClockFreq(uint32_t I2CxSource)
{
  uint32_t i2c_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_I2C_CLKSOURCE(I2CxSource));

  switch (LL_RCC_GetI2CClockSource(I2CxSource))
  {
    case LL_RCC_I2C12_CLKSOURCE_PCLK1:
    case LL_RCC_I2C35_CLKSOURCE_PCLK1:
      i2c_frequency = RCC_GetPCLK1ClockFreq(RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq()));
      break;

    case LL_RCC_I2C12_CLKSOURCE_PLL4R:
    case LL_RCC_I2C35_CLKSOURCE_PLL4R:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        i2c_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_I2C12_CLKSOURCE_HSI:
    case LL_RCC_I2C35_CLKSOURCE_HSI:
    case LL_RCC_I2C46_CLKSOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        i2c_frequency = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_I2C12_CLKSOURCE_CSI:
    case LL_RCC_I2C35_CLKSOURCE_CSI:
    case LL_RCC_I2C46_CLKSOURCE_CSI:
      if (LL_RCC_CSI_IsReady() != 0U)
      {
        i2c_frequency = CSI_VALUE;
      }
      break;

    case LL_RCC_I2C46_CLKSOURCE_PCLK5:
      i2c_frequency = RCC_GetPCLK5ClockFreq(RCC_GetACLKClockFreq(RCC_GetAXISSClockFreq()));
      break;

    case LL_RCC_I2C46_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        i2c_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return i2c_frequency;
}

/**
  * @brief  Return SAIx clock frequency
  * @param  SAIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE
  *         @arg @ref LL_RCC_SAI3_CLKSOURCE
  *         @arg @ref LL_RCC_SAI4_CLKSOURCE
  * @retval SAI clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetSAIClockFreq(uint32_t SAIxSource)
{
  uint32_t sai_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_SAI_CLKSOURCE(SAIxSource));

  switch (LL_RCC_GetSAIClockSource(SAIxSource))
  {
    case LL_RCC_SAI1_CLKSOURCE_PLL4Q:
    case LL_RCC_SAI2_CLKSOURCE_PLL4Q:
    case LL_RCC_SAI3_CLKSOURCE_PLL4Q:
    case LL_RCC_SAI4_CLKSOURCE_PLL4Q:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        sai_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_SAI1_CLKSOURCE_PLL3Q:
    case LL_RCC_SAI2_CLKSOURCE_PLL3Q:
    case LL_RCC_SAI3_CLKSOURCE_PLL3Q:
    case LL_RCC_SAI4_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        sai_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_SAI1_CLKSOURCE_I2SCKIN:
    case LL_RCC_SAI2_CLKSOURCE_I2SCKIN:
    case LL_RCC_SAI3_CLKSOURCE_I2SCKIN:
    case LL_RCC_SAI4_CLKSOURCE_I2SCKIN:
      sai_frequency = EXTERNAL_CLOCK_VALUE;
      break;

    case LL_RCC_SAI1_CLKSOURCE_PER:
    case LL_RCC_SAI2_CLKSOURCE_PER:
    case LL_RCC_SAI3_CLKSOURCE_PER:
    case LL_RCC_SAI4_CLKSOURCE_PER:
      sai_frequency = LL_RCC_GetCKPERClockFreq(LL_RCC_CKPER_CLKSOURCE);
      break;

    case LL_RCC_SAI1_CLKSOURCE_PLL3R:
    case LL_RCC_SAI2_CLKSOURCE_PLL3R:
    case LL_RCC_SAI3_CLKSOURCE_PLL3R:
    case LL_RCC_SAI4_CLKSOURCE_PLL3R:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        sai_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_SAI2_CLKSOURCE_SPDIF: //SAI2 manages this SPDIF_CKSYMB_VALUE
    default:
      /* Nothing to do */
      break;
  }

  return sai_frequency;
}

/**
  * @brief  Return SPIx clock frequency
  * @param  SPIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPI1_CLKSOURCE
  *         @arg @ref LL_RCC_SPI23_CLKSOURCE
  *         @arg @ref LL_RCC_SPI45_CLKSOURCE
  *         @arg @ref LL_RCC_SPI6_CLKSOURCE
  * @retval SPI clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetSPIClockFreq(uint32_t SPIxSource)
{
  uint32_t spi_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_SPI_CLKSOURCE(SPIxSource));

  switch (LL_RCC_GetSPIClockSource(SPIxSource))
  {
    case LL_RCC_SPI1_CLKSOURCE_PLL4P:
    case LL_RCC_SPI23_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        spi_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    case LL_RCC_SPI1_CLKSOURCE_PLL3Q:
    case LL_RCC_SPI23_CLKSOURCE_PLL3Q:
    case LL_RCC_SPI6_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        spi_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_SPI1_CLKSOURCE_I2SCKIN:
    case LL_RCC_SPI23_CLKSOURCE_I2SCKIN:
      spi_frequency = EXTERNAL_CLOCK_VALUE;
      break;

    case LL_RCC_SPI1_CLKSOURCE_PER:
    case LL_RCC_SPI23_CLKSOURCE_PER:
      spi_frequency = LL_RCC_GetCKPERClockFreq(LL_RCC_CKPER_CLKSOURCE);
      break;

    case LL_RCC_SPI1_CLKSOURCE_PLL3R:
    case LL_RCC_SPI23_CLKSOURCE_PLL3R:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        spi_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_SPI45_CLKSOURCE_PCLK2:
      spi_frequency = RCC_GetPCLK2ClockFreq(RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq()));
      break;

    case LL_RCC_SPI45_CLKSOURCE_PLL4Q:
    case LL_RCC_SPI6_CLKSOURCE_PLL4Q:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        spi_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_SPI45_CLKSOURCE_HSI:
    case LL_RCC_SPI6_CLKSOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        spi_frequency = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_SPI45_CLKSOURCE_CSI:
    case LL_RCC_SPI6_CLKSOURCE_CSI:
      if (LL_RCC_CSI_IsReady() != 0U)
      {
        spi_frequency = CSI_VALUE;
      }
      break;

    case LL_RCC_SPI45_CLKSOURCE_HSE:
    case LL_RCC_SPI6_CLKSOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        spi_frequency = HSE_VALUE;
      }
      break;

    case LL_RCC_SPI6_CLKSOURCE_PCLK5:
      spi_frequency = RCC_GetPCLK5ClockFreq(RCC_GetACLKClockFreq(RCC_GetAXISSClockFreq()));
      break;

    default:
      /* Nothing to do */
      break;
  }

  return spi_frequency;
}

/**
  * @brief  Return UARTx clock frequency
  * @param  UARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_UART24_CLKSOURCE
  *         @arg @ref LL_RCC_UART35_CLKSOURCE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE
  *         @arg @ref LL_RCC_UART78_CLKSOURCE
  * @retval UART clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetUARTClockFreq(uint32_t UARTxSource)
{
  uint32_t uart_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_UART_CLKSOURCE(UARTxSource));

  switch (LL_RCC_GetUARTClockSource(UARTxSource))
  {
    case LL_RCC_USART1_CLKSOURCE_PCLK5:
      uart_frequency = RCC_GetPCLK5ClockFreq(RCC_GetACLKClockFreq(RCC_GetAXISSClockFreq()));
      break;

    case LL_RCC_USART1_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        uart_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_USART1_CLKSOURCE_HSI:
    case LL_RCC_UART24_CLKSOURCE_HSI:
    case LL_RCC_UART35_CLKSOURCE_HSI:
    case LL_RCC_USART6_CLKSOURCE_HSI:
    case LL_RCC_UART78_CLKSOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        uart_frequency = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_USART1_CLKSOURCE_CSI:
    case LL_RCC_UART24_CLKSOURCE_CSI:
    case LL_RCC_UART35_CLKSOURCE_CSI:
    case LL_RCC_USART6_CLKSOURCE_CSI:
    case LL_RCC_UART78_CLKSOURCE_CSI:
      if (LL_RCC_CSI_IsReady() != 0U)
      {
        uart_frequency = CSI_VALUE;
      }
      break;

    case LL_RCC_USART1_CLKSOURCE_PLL4Q:
    case LL_RCC_UART24_CLKSOURCE_PLL4Q:
    case LL_RCC_UART35_CLKSOURCE_PLL4Q:
    case LL_RCC_USART6_CLKSOURCE_PLL4Q:
    case LL_RCC_UART78_CLKSOURCE_PLL4Q:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        uart_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_USART1_CLKSOURCE_HSE:
    case LL_RCC_UART24_CLKSOURCE_HSE:
    case LL_RCC_UART35_CLKSOURCE_HSE:
    case LL_RCC_USART6_CLKSOURCE_HSE:
    case LL_RCC_UART78_CLKSOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        uart_frequency = HSE_VALUE;
      }
      break;

    case LL_RCC_UART24_CLKSOURCE_PCLK1:
    case LL_RCC_UART35_CLKSOURCE_PCLK1:
    case LL_RCC_UART78_CLKSOURCE_PCLK1:
      uart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq()));
      break;

    case LL_RCC_USART6_CLKSOURCE_PCLK2:
      uart_frequency = RCC_GetPCLK2ClockFreq(RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq()));
      break;

    default:
      /* Nothing to do */
      break;
  }

  return uart_frequency;
}

/**
  * @brief  Return SDMMCx clock frequency
  * @param  SDMMCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC12_CLKSOURCE
  *         @arg @ref LL_RCC_SDMMC3_CLKSOURCE
  * @retval SDMMC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetSDMMCClockFreq(uint32_t SDMMCxSource)
{
  uint32_t sdmmc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_SDMMC_CLKSOURCE(SDMMCxSource));

  switch (LL_RCC_GetSDMMCClockSource(SDMMCxSource))
  {
    case LL_RCC_SDMMC12_CLKSOURCE_HCLK6:
      sdmmc_frequency = RCC_GetACLKClockFreq(RCC_GetAXISSClockFreq());
      break;

    case LL_RCC_SDMMC12_CLKSOURCE_PLL3R:
    case LL_RCC_SDMMC3_CLKSOURCE_PLL3R:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        sdmmc_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_SDMMC12_CLKSOURCE_PLL4P:
    case LL_RCC_SDMMC3_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        sdmmc_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    case LL_RCC_SDMMC12_CLKSOURCE_HSI:
    case LL_RCC_SDMMC3_CLKSOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        sdmmc_frequency = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_SDMMC3_CLKSOURCE_HCLK2:
      sdmmc_frequency = RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq());
      break;

    default:
      /* Nothing to do */
      break;
  }

  return sdmmc_frequency;
}

/**
  * @brief  Return ETHx clock frequency
  * @param  ETHxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ETH_CLKSOURCE
  * @retval ETH clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetETHClockFreq(uint32_t ETHxSource)
{
  uint32_t eth_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_ETH_CLKSOURCE(ETHxSource));

  switch (LL_RCC_GetETHClockSource(ETHxSource))
  {
    case LL_RCC_ETH_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        eth_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    case LL_RCC_ETH_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        eth_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_ETH_CLKSOURCE_OFF:
    default:
      /* Nothing to do */
      break;
  }

  return eth_frequency;
}

/**
  * @brief  Return QSPIx clock frequency
  * @param  QSPIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_QSPI_CLKSOURCE
  * @retval QSPI clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetQSPIClockFreq(uint32_t QSPIxSource)
{
  uint32_t qspi_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_QSPI_CLKSOURCE(QSPIxSource));

  switch (LL_RCC_GetQSPIClockSource(QSPIxSource))
  {
    case LL_RCC_QSPI_CLKSOURCE_ACLK:
      qspi_frequency = RCC_GetACLKClockFreq(RCC_GetACLKClockFreq(RCC_GetAXISSClockFreq()));
      break;

    case LL_RCC_QSPI_CLKSOURCE_PLL3R:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        qspi_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_QSPI_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        qspi_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    case LL_RCC_QSPI_CLKSOURCE_PER:
      qspi_frequency = LL_RCC_GetCKPERClockFreq(LL_RCC_CKPER_CLKSOURCE);
      break;

    default:
      /* Nothing to do */
      break;
  }

  return qspi_frequency;
}

/**
  * @brief  Return FMCx clock frequency
  * @param  FMCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_FMC_CLKSOURCE
  * @retval FMC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetFMCClockFreq(uint32_t FMCxSource)
{
  uint32_t fmc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_FMC_CLKSOURCE(FMCxSource));

  switch (LL_RCC_GetFMCClockSource(FMCxSource))
  {
    case LL_RCC_FMC_CLKSOURCE_ACLK:
      fmc_frequency = RCC_GetACLKClockFreq(RCC_GetACLKClockFreq(RCC_GetAXISSClockFreq()));
      break;

    case LL_RCC_FMC_CLKSOURCE_PLL3R:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        fmc_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_FMC_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        fmc_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    case LL_RCC_FMC_CLKSOURCE_PER:
      fmc_frequency = LL_RCC_GetCKPERClockFreq(LL_RCC_CKPER_CLKSOURCE);
      break;

    default:
      /* Nothing to do */
      break;
  }

  return fmc_frequency;
}

/**
  * @brief  Return FDCANx clock frequency
  * @param  FDCANxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_FDCAN_CLKSOURCE
  * @retval FDCAN clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetFDCANClockFreq(uint32_t FDCANxSource)
{
  uint32_t fdcan_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_FDCAN_CLKSOURCE(FDCANxSource));

  switch (LL_RCC_GetFDCANClockSource(FDCANxSource))
  {
    case LL_RCC_FDCAN_CLKSOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        fdcan_frequency = HSE_VALUE;
      }
      break;

    case LL_RCC_FDCAN_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        fdcan_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_FDCAN_CLKSOURCE_PLL4Q:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        fdcan_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_FDCAN_CLKSOURCE_PLL4R:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        fdcan_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return fdcan_frequency;
}

/**
  * @brief  Return SPDIFRXx clock frequency
  * @param  SPDIFRXxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPDIFRX_CLKSOURCE
  * @retval SPDIFRX clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetSPDIFRXClockFreq(uint32_t SPDIFRXxSource)
{
  uint32_t spdifrx_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_SPDIFRX_CLKSOURCE(SPDIFRXxSource));

  switch (LL_RCC_GetSPDIFRXClockSource(SPDIFRXxSource))
  {
    case LL_RCC_SPDIFRX_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        spdifrx_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    case LL_RCC_SPDIFRX_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        spdifrx_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_SPDIFRX_CLKSOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        spdifrx_frequency = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return spdifrx_frequency;
}

/**
  * @brief  Return CECx clock frequency
  * @param  CECxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE
  * @retval CEC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetCECClockFreq(uint32_t CECxSource)
{
  uint32_t cec_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_CEC_CLKSOURCE(CECxSource));

  switch (LL_RCC_GetCECClockSource(CECxSource))
  {
    case LL_RCC_CEC_CLKSOURCE_LSE:
      if (LL_RCC_LSE_IsReady() != 0U)
      {
        cec_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_CEC_CLKSOURCE_LSI:
      if (LL_RCC_LSI_IsReady() != 0U)
      {
        cec_frequency = LSI_VALUE;
      }
      break;

    case LL_RCC_CEC_CLKSOURCE_CSI122:
      if (LL_RCC_CSI_IsReady() != 0U)
      {
        cec_frequency = (CSI_VALUE / 122U);
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return cec_frequency;
}

/**
  * @brief  Return USBPHYx clock frequency
  * @param  USBPHYxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USBPHY_CLKSOURCE
  * @retval USBPHY clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetUSBPHYClockFreq(uint32_t USBPHYxSource)
{
  uint32_t usbphy_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_USBPHY_CLKSOURCE(USBPHYxSource));

  switch (LL_RCC_GetUSBPHYClockSource(USBPHYxSource))
  {
    case LL_RCC_USBPHY_CLKSOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        usbphy_frequency = HSE_VALUE;
      }
      break;

    case LL_RCC_USBPHY_CLKSOURCE_PLL4R:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        usbphy_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_USBPHY_CLKSOURCE_HSE2:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        usbphy_frequency = (HSE_VALUE / 2U);
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return usbphy_frequency;
}

/**
  * @brief  Return USBOx clock frequency
  * @param  USBOxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USBO_CLKSOURCE
  * @retval USBO clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetUSBOClockFreq(uint32_t USBOxSource)
{
  uint32_t usbo_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_USBO_CLKSOURCE(USBOxSource));

  switch (LL_RCC_GetUSBOClockSource(USBOxSource))
  {
    case LL_RCC_USBO_CLKSOURCE_PLL4R:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        usbo_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_USBO_CLKSOURCE_PHY:
      usbo_frequency = USBO_48M_VALUE; /* rcc_ck_usbo_48m */
      break;

    default:
      /* Nothing to do */
      break;
  }

  return usbo_frequency;
}

/**
  * @brief  Return RNGx clock frequency
  * @param  RNGxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG1_CLKSOURCE
  *         @arg @ref LL_RCC_RNG2_CLKSOURCE
  * @retval RNG clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetRNGClockFreq(uint32_t RNGxSource)
{
  uint32_t rng_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_RNG_CLKSOURCE(RNGxSource));

  switch (LL_RCC_GetRNGClockSource(RNGxSource))
  {
    case LL_RCC_RNG1_CLKSOURCE_CSI:
    case LL_RCC_RNG2_CLKSOURCE_CSI:
      if (LL_RCC_CSI_IsReady() != 0U)
      {
        rng_frequency = CSI_VALUE;
      }
      break;

    case LL_RCC_RNG1_CLKSOURCE_PLL4R:
    case LL_RCC_RNG2_CLKSOURCE_PLL4R:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        rng_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_RNG1_CLKSOURCE_LSE:
    case LL_RCC_RNG2_CLKSOURCE_LSE:
      if (LL_RCC_LSE_IsReady() != 0U)
      {
        rng_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_RNG1_CLKSOURCE_LSI:
    case LL_RCC_RNG2_CLKSOURCE_LSI:
      if (LL_RCC_LSI_IsReady() != 0U)
      {
        rng_frequency = LSI_VALUE;
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return rng_frequency;
}

/**
  * @brief  Return CKPERx clock frequency
  * @param  CKPERxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CKPER_CLKSOURCE
  * @retval CKPER clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetCKPERClockFreq(uint32_t CKPERxSource)
{
  uint32_t ckper_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_CKPER_CLKSOURCE(CKPERxSource));

  switch (LL_RCC_GetCKPERClockSource(CKPERxSource))
  {
    case LL_RCC_CKPER_CLKSOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        ckper_frequency = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_CKPER_CLKSOURCE_CSI:
      if (LL_RCC_CSI_IsReady() != 0U)
      {
        ckper_frequency = CSI_VALUE;
      }
      break;

    case LL_RCC_CKPER_CLKSOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        ckper_frequency = HSE_VALUE;
      }
      break;

    case LL_RCC_CKPER_CLKSOURCE_OFF:
    default:
      /* Nothing to do */
      break;
  }

  return ckper_frequency;
}

/**
  * @brief  Return STGENx clock frequency
  * @param  STGENxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_STGEN_CLKSOURCE
  * @retval STGEN clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetSTGENClockFreq(uint32_t STGENxSource)
{
  uint32_t stgen_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_STGEN_CLKSOURCE(STGENxSource));

  switch (LL_RCC_GetSTGENClockSource(STGENxSource))
  {
    case LL_RCC_STGEN_CLKSOURCE_HSI:
      if (LL_RCC_HSI_IsReady() != 0U)
      {
        stgen_frequency = HSI_VALUE >> LL_RCC_HSI_GetDivider();
      }
      break;

    case LL_RCC_STGEN_CLKSOURCE_HSE:
      if (LL_RCC_HSE_IsReady() != 0U)
      {
        stgen_frequency = HSE_VALUE;
      }
      break;

    case LL_RCC_STGEN_CLKSOURCE_OFF:
    default:
      /* Nothing to do */
      break;
  }

  return stgen_frequency;
}

/**
  * @brief  Return DSIx clock frequency
  * @param  DSIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE
  * @retval DSI clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetDSIClockFreq(uint32_t DSIxSource)
{
  uint32_t dsi_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_DSI_CLKSOURCE(DSIxSource));

  switch (LL_RCC_GetDSIClockSource(DSIxSource))
  {
    case LL_RCC_DSI_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        dsi_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    /* It has no sense to ask for DSIPHY frequency because it is generated
     * by DSI itself, so send back 0 as kernel frequency
     */
    case LL_RCC_DSI_CLKSOURCE_PHY:
      dsi_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;

    default:
      /* Nothing to do */
      break;
  }

  return dsi_frequency;
}

/**
  * @brief  Return ADCx clock frequency
  * @param  ADCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE
  * @retval ADC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetADCClockFreq(uint32_t ADCxSource)
{
  uint32_t adc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_ADC_CLKSOURCE(ADCxSource));

  switch (LL_RCC_GetADCClockSource(ADCxSource))
  {
    case LL_RCC_ADC_CLKSOURCE_PLL4R:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        adc_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    case LL_RCC_ADC_CLKSOURCE_PER:
      adc_frequency = LL_RCC_GetCKPERClockFreq(LL_RCC_CKPER_CLKSOURCE);
      break;

    case LL_RCC_ADC_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        adc_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return adc_frequency;
}

/**
  * @brief  Return LPTIMx clock frequency
  * @param  LPTIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM23_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM45_CLKSOURCE
  * @retval LPTIM clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetLPTIMClockFreq(uint32_t LPTIMxSource)
{
  uint32_t lptim_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_LPTIM_CLKSOURCE(LPTIMxSource));

  switch (LL_RCC_GetLPTIMClockSource(LPTIMxSource))
  {
    case  LL_RCC_LPTIM1_CLKSOURCE_PCLK1:
      lptim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq()));
      break;

    case LL_RCC_LPTIM1_CLKSOURCE_PLL4P:
    case LL_RCC_LPTIM45_CLKSOURCE_PLL4P:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        lptim_frequency = PLL_Clocks.PLL_P_Frequency;
      }
      break;

    case LL_RCC_LPTIM1_CLKSOURCE_PLL3Q:
    case LL_RCC_LPTIM45_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        lptim_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_LPTIM1_CLKSOURCE_LSE:
    case LL_RCC_LPTIM23_CLKSOURCE_LSE:
    case LL_RCC_LPTIM45_CLKSOURCE_LSE:
      if (LL_RCC_LSE_IsReady() != 0U)
      {
        lptim_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_LPTIM1_CLKSOURCE_LSI:
    case LL_RCC_LPTIM23_CLKSOURCE_LSI:
    case LL_RCC_LPTIM45_CLKSOURCE_LSI:
      if (LL_RCC_LSI_IsReady() != 0U)
      {
        lptim_frequency = LSI_VALUE;
      }
      break;

    case LL_RCC_LPTIM1_CLKSOURCE_PER:
    case LL_RCC_LPTIM23_CLKSOURCE_PER:
    case LL_RCC_LPTIM45_CLKSOURCE_PER:
      lptim_frequency = LL_RCC_GetCKPERClockFreq(LL_RCC_CKPER_CLKSOURCE);
      break;

    case LL_RCC_LPTIM23_CLKSOURCE_PCLK3:
    case LL_RCC_LPTIM45_CLKSOURCE_PCLK3:
      lptim_frequency = RCC_GetPCLK3ClockFreq(RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq()));
      break;

    case LL_RCC_LPTIM23_CLKSOURCE_PLL4Q:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        lptim_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_LPTIM1_CLKSOURCE_OFF:
    case LL_RCC_LPTIM23_CLKSOURCE_OFF:
    case LL_RCC_LPTIM45_CLKSOURCE_OFF:
    default:
      /* Nothing to do */
      break;
  }

  return lptim_frequency;
}

/**
  * @brief  Return DFSDM kernel clock frequency
  * @param  DFSDMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM_CLKSOURCE
  * @note   DFSM shares with SAI1 the same kernel clock source
  * @retval DFSDM clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetDFSDMClockFreq(uint32_t DFSDMxSource)
{
  uint32_t dfsdm_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Check parameter */
  assert_param(IS_LL_RCC_DFSDM_CLKSOURCE(DFSDMxSource));

  switch (LL_RCC_GetSAIClockSource(LL_RCC_SAI1_CLKSOURCE))
  {
    case LL_RCC_SAI1_CLKSOURCE_PLL4Q:
      if (LL_RCC_PLL4_IsReady() != 0U)
      {
        LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
        dfsdm_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_SAI1_CLKSOURCE_PLL3Q:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        dfsdm_frequency = PLL_Clocks.PLL_Q_Frequency;
      }
      break;

    case LL_RCC_SAI1_CLKSOURCE_I2SCKIN:
      dfsdm_frequency = EXTERNAL_CLOCK_VALUE;
      break;

    case LL_RCC_SAI1_CLKSOURCE_PER:
      dfsdm_frequency = LL_RCC_GetCKPERClockFreq(LL_RCC_CKPER_CLKSOURCE);
      break;

    case LL_RCC_SAI1_CLKSOURCE_PLL3R:
      if (LL_RCC_PLL3_IsReady() != 0U)
      {
        LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
        dfsdm_frequency = PLL_Clocks.PLL_R_Frequency;
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return dfsdm_frequency;
}

/**
  * @brief  Return LTDC kernel clock frequency
  * @retval LTDC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetLTDCClockFreq(void)
{
  uint32_t ltdc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  if (LL_RCC_PLL4_IsReady() != 0U)
  {
    LL_RCC_GetPLL4ClockFreq(&PLL_Clocks);
    ltdc_frequency = PLL_Clocks.PLL_Q_Frequency;
  }

  return ltdc_frequency;
}

/**
  * @brief  Return RTC clock frequency
  * @retval RTC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetRTCClockFreq(void)
{
  uint32_t rtc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* RTCCLK clock frequency */
  switch (LL_RCC_GetRTCClockSource())
  {
    case LL_RCC_RTC_CLKSOURCE_LSE:     /* LSE clock used as RTC clock source */
      if (LL_RCC_LSE_IsReady() != 0U)
      {
        rtc_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_RTC_CLKSOURCE_LSI:     /* LSI clock used as RTC clock source */
      if (LL_RCC_LSI_IsReady() != 0U)
      {
        rtc_frequency = LSI_VALUE;
      }
      break;

    case LL_RCC_RTC_CLKSOURCE_HSE_DIV: /* HSE clock used as RTC clock source */
      rtc_frequency = (HSE_VALUE / (LL_RCC_GetRTC_HSEPrescaler() + 1U));
      break;

    case LL_RCC_RTC_CLKSOURCE_NONE:    /* No clock used as RTC clock source */
    default:
      /* Nothing to do */
      break;
  }

  return rtc_frequency;
}

/**
  * @brief  Return TIMGx clock frequency
  * @param  TIMGxPrescaler This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIMG1PRES
  *         @arg @ref LL_RCC_TIMG2PRES
  *@note    LL_RCC_TIMG1PRES returns the frequency of the prescaler of timers
  *         located into APB1 domain. It concerns TIM2, TIM3, TIM4, TIM5, TIM6,
  *         TIM7, TIM12, TIM13 and TIM14 frequencies.
  *         LL_RCC_TIMG2PRES returns the frequency of the prescaler of timers
  *         located into APB2 domain. It concerns TIM1, TIM8, TIM15, TIM16, and
  *         TIM17.
  * @retval TIMGx clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetTIMGClockFreq(uint32_t TIMGxPrescaler)
{
  uint32_t timg_frequency, apb1_div;

  /* Check parameter */
  assert_param(IS_LL_RCC_TIMGPRES(TIMGxPrescaler));

  timg_frequency = RCC_GetMLHCLKClockFreq(RCC_GetMCUSSClockFreq());
  apb1_div = LL_RCC_GetAPB1Prescaler();

  switch (LL_RCC_GetTIMGPrescaler(TIMGxPrescaler))
  {
    case LL_RCC_TIMG1PRES_ACTIVATED:
    case LL_RCC_TIMG2PRES_ACTIVATED:
      switch (apb1_div)
      {
        case LL_RCC_APB1_DIV_1:
        case LL_RCC_APB1_DIV_2:
        case LL_RCC_APB1_DIV_4:
          break;
        case LL_RCC_APB1_DIV_8:
          timg_frequency /= 2U;
          break;
        case LL_RCC_APB1_DIV_16:
          timg_frequency /= 4U;
          break;
      }
      break;

    case LL_RCC_TIMG1PRES_DEACTIVATED:
    case LL_RCC_TIMG2PRES_DEACTIVATED:
      switch (apb1_div)
      {
        case LL_RCC_APB1_DIV_1:
        case LL_RCC_APB1_DIV_2:
          break;
        case LL_RCC_APB1_DIV_4:
          timg_frequency /= 2U;
          break;
        case LL_RCC_APB1_DIV_8:
          timg_frequency /= 4U;
          break;
        case LL_RCC_APB1_DIV_16:
          timg_frequency /= 8U;
          break;
      }
      break;

    default:
      timg_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
      break;
  }

  return timg_frequency;
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup RCC_LL_Private_Functions
  * @{
  */

/**
  * @brief  Return MPUSS clock frequency
  * @retval MPUSS clock frequency (in Hz)
  */
uint32_t RCC_GetMPUSSClockFreq(void)
{
  uint32_t frequency = 0U, mpudiv;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Get MPUSS source -------------------------------------------------------*/
  switch (LL_RCC_GetMPUClkSource())
  {
    case LL_RCC_MPU_CLKSOURCE_HSI:
      frequency = (HSI_VALUE >> LL_RCC_HSI_GetDivider());
      break;

    case LL_RCC_MPU_CLKSOURCE_HSE:
      frequency = HSE_VALUE;
      break;

    case LL_RCC_MPU_CLKSOURCE_PLL1:
      LL_RCC_GetPLL1ClockFreq(&PLL_Clocks);
      frequency = PLL_Clocks.PLL_P_Frequency;
      break;

    case LL_RCC_MPU_CLKSOURCE_MPUDIV:
      mpudiv = LL_RCC_GetMPUPrescaler();
      if (mpudiv != LL_RCC_MPU_DIV_OFF)
      {
        LL_RCC_GetPLL1ClockFreq(&PLL_Clocks);
        frequency = (PLL_Clocks.PLL_P_Frequency >> mpudiv);
      }
      break;

    default:
      /* Nothing to do */
      break;
  }

  return frequency;
}

/**
  * @brief  Return AXISS clock frequency
  * @retval AXISS clock frequency (in Hz)
  */
uint32_t RCC_GetAXISSClockFreq(void)
{
  uint32_t frequency = 0U;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Get AXISS source -------------------------------------------------------*/
  switch (LL_RCC_GetAXISSClkSource())
  {
    case LL_RCC_AXISS_CLKSOURCE_HSI:
      frequency = (HSI_VALUE >> LL_RCC_HSI_GetDivider());
      break;

    case LL_RCC_AXISS_CLKSOURCE_HSE:
      frequency = HSE_VALUE;
      break;

    case LL_RCC_AXISS_CLKSOURCE_PLL2:
      LL_RCC_GetPLL2ClockFreq(&PLL_Clocks);
      frequency = PLL_Clocks.PLL_P_Frequency;
      break;

    case LL_RCC_AXISS_CLKSOURCE_OFF:
    default:
      /* Nothing to do */
      break;
  }

  return frequency;
}

/**
  * @brief  Return MCUSS clock frequency
  * @retval MCUSS clock frequency (in Hz)
  */
uint32_t RCC_GetMCUSSClockFreq()
{
  uint32_t frequency = 0U;
  LL_PLL_ClocksTypeDef PLL_Clocks;

  /* Get MCUSS source -------------------------------------------------------*/
  switch (LL_RCC_GetMCUSSClkSource())
  {
    case LL_RCC_MCUSS_CLKSOURCE_HSI:
      frequency = (HSI_VALUE >> LL_RCC_HSI_GetDivider());
      break;

    case LL_RCC_MCUSS_CLKSOURCE_HSE:
      frequency = HSE_VALUE;
      break;

    case LL_RCC_MCUSS_CLKSOURCE_CSI:
      frequency = CSI_VALUE;
      break;

    case LL_RCC_MCUSS_CLKSOURCE_PLL3:
      LL_RCC_GetPLL3ClockFreq(&PLL_Clocks);
      frequency = PLL_Clocks.PLL_P_Frequency;
      break;

    default:
      /* Nothing to do */
      break;
  }

  return frequency;
}

/**
  * @brief  Return ACLK (ACLK, HCLK5 and HCLK6) clock frequency
  * @retval ACLK clock frequency (in Hz)
  */
uint32_t RCC_GetACLKClockFreq(uint32_t AXISS_Frequency)
{
  return LL_RCC_CALC_ACLK_FREQ(AXISS_Frequency, LL_RCC_GetACLKPrescaler());
}

/**
  * @brief  Return MLHCLK (MLHCLK, HCLK[4:1], MCU_CK and FCLK_CK) clock frequency
  * @retval MLHCLK clock frequency (in Hz)
  */
uint32_t RCC_GetMLHCLKClockFreq(uint32_t MCUSS_Frequency)
{
  return LL_RCC_CALC_MLHCLK_FREQ(MCUSS_Frequency, LL_RCC_GetMLHCLKPrescaler());
}

/**
  * @brief  Return PCLK1 clock frequency
  * @param  MLHCLK_Frequency MLHCLK clock frequency
  * @retval PCLK1 clock frequency (in Hz)
  */
uint32_t RCC_GetPCLK1ClockFreq(uint32_t MLHCLK_Frequency)
{
  /* PCLK1 clock frequency */
  return LL_RCC_CALC_PCLK1_FREQ(MLHCLK_Frequency, LL_RCC_GetAPB1Prescaler());
}

/**
  * @brief  Return PCLK2 clock frequency
  * @param  MLHCLK_Frequency MLHCLK clock frequency
  * @retval PCLK2 clock frequency (in Hz)
  */
uint32_t RCC_GetPCLK2ClockFreq(uint32_t MLHCLK_Frequency)
{
  /* PCLK2 clock frequency */
  return LL_RCC_CALC_PCLK2_FREQ(MLHCLK_Frequency, LL_RCC_GetAPB2Prescaler());
}

/**
  * @brief  Return PCLK3 clock frequency
  * @param  MLHCLK_Frequency MLHCLK clock frequency
  * @retval PCLK3 clock frequency (in Hz)
  */
uint32_t RCC_GetPCLK3ClockFreq(uint32_t MLHCLK_Frequency)
{
  /* PCLK3 clock frequency */
  return LL_RCC_CALC_PCLK3_FREQ(MLHCLK_Frequency, LL_RCC_GetAPB3Prescaler());
}

/**
  * @brief  Return PCLK4 clock frequency
  * @param  ACLK_Frequency ACLK clock frequency
  * @retval PCLK4 clock frequency (in Hz)
  */
uint32_t RCC_GetPCLK4ClockFreq(uint32_t ACLK_Frequency)
{
  /* PCLK4 clock frequency */
  return LL_RCC_CALC_PCLK4_FREQ(ACLK_Frequency, LL_RCC_GetAPB4Prescaler());
}

/**
  * @brief  Return PCLK5 clock frequency
  * @param  ACLK_Frequency ACLK clock frequency
  * @retval PCLK5 clock frequency (in Hz)
  */
uint32_t RCC_GetPCLK5ClockFreq(uint32_t ACLK_Frequency)
{
  /* PCLK5 clock frequency */
  return LL_RCC_CALC_PCLK5_FREQ(ACLK_Frequency, LL_RCC_GetAPB5Prescaler());
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* defined(RCC) */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
