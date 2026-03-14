/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32a455_rcc.c
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#include "n32a455_rcc.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup RCC
 * @brief RCC driver modules
 * @{
 */

/** @addtogroup RCC_Private_TypesDefinitions
 * @{
 */

/**
 * @}
 */

/** @addtogroup RCC_Private_Defines
 * @{
 */

/* ------------ RCC registers bit address in the alias region ----------- */
#define RCC_OFFSET (RCC_BASE - PERIPH_BASE)

/* --- CTRL Register ---*/

/* Alias word address of HSIEN bit */
#define CTRL_OFFSET   (RCC_OFFSET + 0x00)
#define HSIEN_BITN    0x00
#define CTRL_HSIEN_BB (PERIPH_BB_BASE + (CTRL_OFFSET * 32) + (HSIEN_BITN * 4))

/* Alias word address of PLLEN bit */
#define PLLEN_BITN    0x18
#define CTRL_PLLEN_BB (PERIPH_BB_BASE + (CTRL_OFFSET * 32) + (PLLEN_BITN * 4))

/* Alias word address of CLKSSEN bit */
#define CLKSSEN_BITN    0x13
#define CTRL_CLKSSEN_BB (PERIPH_BB_BASE + (CTRL_OFFSET * 32) + (CLKSSEN_BITN * 4))

/* --- BDCTRL Register ---*/

/* Alias word address of RTCEN bit */
#define BDCTRL_OFFSET   (RCC_OFFSET + 0x20)
#define RTCEN_BITN      0x0F
#define BDCTRL_RTCEN_BB (PERIPH_BB_BASE + (BDCTRL_OFFSET * 32) + (RTCEN_BITN * 4))

/* Alias word address of BDSFTRST bit */
#define BDSFTRST_BITN      0x10
#define BDCTRL_BDSFTRST_BB (PERIPH_BB_BASE + (BDCTRL_OFFSET * 32) + (BDSFTRST_BITN * 4))

/* --- CTRLSTS Register ---*/

/* Alias word address of LSIEN bit */
#define CTRLSTS_OFFSET   (RCC_OFFSET + 0x24)
#define LSIEN_BITNUMBER  0x00
#define CTRLSTS_LSIEN_BB (PERIPH_BB_BASE + (CTRLSTS_OFFSET * 32) + (LSIEN_BITNUMBER * 4))

/* ---------------------- RCC registers bit mask ------------------------ */

/* CTRL register bit mask */
#define CTRL_HSEBP_RESET  ((uint32_t)0xFFFBFFFF)
#define CTRL_HSEBP_SET    ((uint32_t)0x00040000)
#define CTRL_HSEEN_RESET  ((uint32_t)0xFFFEFFFF)
#define CTRL_HSEEN_SET    ((uint32_t)0x00010000)
#define CTRL_HSITRIM_MASK ((uint32_t)0xFFFFFF07)

/* CFG register bit mask */
#define CFG_PLL_MASK ((uint32_t)0xF7C0FFFF)

#define CFG_PLLMULFCT_MASK      ((uint32_t)0x083C0000)
#define CFG_PLLSRC_MASK         ((uint32_t)0x00010000)
#define CFG_PLLHSEPRES_MASK     ((uint32_t)0x00020000)
#define CFG_SCLKSTS_MASK        ((uint32_t)0x0000000C)
#define CFG_SCLKSW_MASK         ((uint32_t)0xFFFFFFFC)
#define CFG_AHBPRES_RESET_MASK  ((uint32_t)0xFFFFFF0F)
#define CFG_AHBPRES_SET_MASK    ((uint32_t)0x000000F0)
#define CFG_APB1PRES_RESET_MASK ((uint32_t)0xFFFFF8FF)
#define CFG_APB1PRES_SET_MASK   ((uint32_t)0x00000700)
#define CFG_APB2PRES_RESET_MASK ((uint32_t)0xFFFFC7FF)
#define CFG_APB2PRES_SET_MASK   ((uint32_t)0x00003800)

/* CFG2 register bit mask */
#define CFG2_TIM18CLKSEL_SET_MASK   ((uint32_t)0x20000000)
#define CFG2_TIM18CLKSEL_RESET_MASK ((uint32_t)0xDFFFFFFF)
#define CFG2_RNGCPRES_SET_MASK      ((uint32_t)0x1F000000)
#define CFG2_RNGCPRES_RESET_MASK    ((uint32_t)0xE0FFFFFF)
#define CFG2_ADC1MSEL_SET_MASK      ((uint32_t)0x00000400)
#define CFG2_ADC1MSEL_RESET_MASK    ((uint32_t)0xFFFFFBFF)
#define CFG2_ADC1MPRES_SET_MASK     ((uint32_t)0x0000F800)
#define CFG2_ADC1MPRES_RESET_MASK   ((uint32_t)0xFFFF07FF)
#define CFG2_ADCPLLPRES_SET_MASK    ((uint32_t)0x000001F0)
#define CFG2_ADCPLLPRES_RESET_MASK  ((uint32_t)0xFFFFFE0F)
#define CFG2_ADCHPRES_SET_MASK      ((uint32_t)0x0000000F)
#define CFG2_ADCHPRES_RESET_MASK    ((uint32_t)0xFFFFFFF0)

/* CFG3 register bit mask */
#define CFGR3_TRNG1MSEL_SET_MASK    ((uint32_t)0x00020000)
#define CFGR3_TRNG1MSEL_RESET_MASK  ((uint32_t)0xFFFDFFFF)
#define CFGR3_TRNG1MPRES_SET_MASK   ((uint32_t)0x0000F800)
#define CFGR3_TRNG1MPRES_RESET_MASK ((uint32_t)0xFFFF07FF)

/* CTRLSTS register bit mask */
#define CSR_RMRSTF_SET ((uint32_t)0x01000000)
#define CSR_RMVF_Reset ((uint32_t)0xfeffffff)

/* RCC Flag Mask */
#define FLAG_MASK ((uint8_t)0x1F)

/* CLKINT register byte 2 (Bits[15:8]) base address */
#define CLKINT_BYTE2_ADDR ((uint32_t)0x40021009)

/* CLKINT register byte 3 (Bits[23:16]) base address */
#define CLKINT_BYTE3_ADDR ((uint32_t)0x4002100A)

/* CFG register byte 4 (Bits[31:24]) base address */
#define CFG_BYTE4_ADDR ((uint32_t)0x40021007)

/* BDCTRL register base address */
#define BDCTRL_ADDR (PERIPH_BASE + BDCTRL_OFFSET)
#define LSE_TRIM_ADDR  (*( uint32_t *)0x40007010)

/**
 * @}
 */

/** @addtogroup RCC_Private_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup RCC_Private_Variables
 * @{
 */

static const uint8_t s_ApbAhbPresTable[16]     = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};
static const uint8_t s_AdcHclkPresTable[16]    = {1, 2, 4, 6, 8, 10, 12, 16, 32, 32, 32, 32, 32, 32, 32, 32};
static const uint16_t s_AdcPllClkPresTable[16] = {1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256, 256, 256, 256, 256};

/**
 * @}
 */

/** @addtogroup RCC_Private_FunctionPrototypes
 * @{
 */

/**
 * @}
 */

/** @addtogroup RCC_Private_Functions
 * @{
 */

/**
 * @brief  Resets the RCC clock configuration to the default reset state.
 */
void RCC_DeInit(void)
{
    /* Set HSIEN bit */
    RCC->CTRL |= (uint32_t)0x00000001;

    /* Reset SW, HPRE, PPRE1, PPRE2 and MCO bits */
    RCC->CFG &= (uint32_t)0xF8FFC000;

    /* Reset HSEON, CLKSSEN and PLLEN bits */
    RCC->CTRL &= (uint32_t)0xFEF6FFFF;

    /* Reset HSEBYP bit */
    RCC->CTRL &= (uint32_t)0xFFFBFFFF;

    /* Reset PLLSRC, PLLXTPRE, PLLMUL bits */
    RCC->CFG &= (uint32_t)0xF700FFFF;

    /* Reset CFG2 register */
    RCC->CFG2 = 0x00003800;

    /* Reset CFG3 register */
    RCC->CFG3 = 0x00003840;

    /* Disable all interrupts and clear pending bits  */
    RCC->CLKINT = 0x009F0000;
}                       

/**
 * @brief  Configures the External High Speed oscillator (HSE).
 * @note   HSE can not be stopped if it is used directly or through the PLL as system clock.
 * @param RCC_HSE specifies the new state of the HSE.
 *   This parameter can be one of the following values:
 *     @arg RCC_HSE_DISABLE HSE oscillator OFF
 *     @arg RCC_HSE_ENABLE HSE oscillator ON
 *     @arg RCC_HSE_BYPASS HSE oscillator bypassed with external clock
 */
void RCC_ConfigHse(uint32_t RCC_HSE)
{
    /* Check the parameters */
    assert_param(IS_RCC_HSE(RCC_HSE));
    /* Reset HSEON and HSEBYP bits before configuring the HSE ------------------*/
    /* Reset HSEON bit */
    RCC->CTRL &= CTRL_HSEEN_RESET;
    /* Reset HSEBYP bit */
    RCC->CTRL &= CTRL_HSEBP_RESET;
    /* Configure HSE (RC_HSE_DISABLE is already covered by the code section above) */
    switch (RCC_HSE)
    {
    case RCC_HSE_ENABLE:
        /* Set HSEON bit */
        RCC->CTRL |= CTRL_HSEEN_SET;
        break;

    case RCC_HSE_BYPASS:
        /* Set HSEBYP and HSEON bits */
        RCC->CTRL |= CTRL_HSEBP_SET | CTRL_HSEEN_SET;
        break;

    default:
        break;
    }
}

/**
 * @brief  Waits for HSE start-up.
 * @return An ErrorStatus enumuration value:
 * - SUCCESS: HSE oscillator is stable and ready to use
 * - ERROR: HSE oscillator not yet ready
 */
ErrorStatus RCC_WaitHseStable(void)
{
    __IO uint32_t StartUpCounter = 0;
    ErrorStatus status           = ERROR;
    FlagStatus HSEStatus         = RESET;

    /* Wait till HSE is ready and if Time out is reached exit */
    do
    {
        HSEStatus = RCC_GetFlagStatus(RCC_FLAG_HSERD);
        StartUpCounter++;
    } while ((StartUpCounter != HSE_STARTUP_TIMEOUT) && (HSEStatus == RESET));

    if (RCC_GetFlagStatus(RCC_FLAG_HSERD) != RESET)
    {
        status = SUCCESS;
    }
    else
    {
        status = ERROR;
    }
    return (status);
}

/**
 * @brief  Adjusts the Internal High Speed oscillator (HSI) calibration value.
 * @param HSICalibrationValue specifies the calibration trimming value.
 *   This parameter must be a number between 0 and 0x1F.
 */
void RCC_SetHsiCalibValue(uint8_t HSICalibrationValue)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_CALIB_VALUE(HSICalibrationValue));
    tmpregister = RCC->CTRL;
    /* Clear HSITRIM[4:0] bits */
    tmpregister &= CTRL_HSITRIM_MASK;
    /* Set the HSITRIM[4:0] bits according to HSICalibrationValue value */
    tmpregister |= (uint32_t)HSICalibrationValue << 3;
    /* Store the new value */
    RCC->CTRL = tmpregister;
}

/**
 * @brief  Enables or disables the Internal High Speed oscillator (HSI).
 * @note   HSI can not be stopped if it is used directly or through the PLL as system clock.
 * @param Cmd new state of the HSI. This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableHsi(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    *(__IO uint32_t*)CTRL_HSIEN_BB = (uint32_t)Cmd;
}

/**
 * @brief  Configures the PLL clock source and multiplication factor.
 * @note   This function must be used only when the PLL is disabled.
 * @note   PLL frequency must be greater than or equal to 32MHz.
 * @param RCC_PLLSource specifies the PLL entry clock source.
 *   this parameter can be one of the following values:
 *     @arg RCC_PLL_SRC_HSI_DIV2 HSI oscillator clock divided by 2 selected as PLL clock entry
 *     @arg RCC_PLL_SRC_HSE_DIV1 HSE oscillator clock selected as PLL clock entry
 *     @arg RCC_PLL_SRC_HSE_DIV2 HSE oscillator clock divided by 2 selected as PLL clock entry
 * @param RCC_PLLMul specifies the PLL multiplication factor.
 *    this parameter can be RCC_PLLMul_x where x:[2,32]
 */
void RCC_ConfigPll(uint32_t RCC_PLLSource, uint32_t RCC_PLLMul)
{
    uint32_t tmpregister = 0;

    /* Check the parameters */
    assert_param(IS_RCC_PLL_SRC(RCC_PLLSource));
    assert_param(IS_RCC_PLL_MUL(RCC_PLLMul));

    tmpregister = RCC->CFG;
    /* Clear PLLSRC, PLLXTPRE and PLLMUL[4:0] bits */
    tmpregister &= CFG_PLL_MASK;
    /* Set the PLL configuration bits */
    tmpregister |= RCC_PLLSource | RCC_PLLMul;
    /* Store the new value */
    RCC->CFG = tmpregister;
}

/**
 * @brief  Enables or disables the PLL.
 * @note   The PLL can not be disabled if it is used as system clock.
 * @param Cmd new state of the PLL. This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnablePll(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    *(__IO uint32_t*)CTRL_PLLEN_BB = (uint32_t)Cmd;
}

/**
 * @brief  Configures the system clock (SYSCLK).
 * @param RCC_SYSCLKSource specifies the clock source used as system clock.
 *   This parameter can be one of the following values:
 *     @arg RCC_SYSCLK_SRC_HSI HSI selected as system clock
 *     @arg RCC_SYSCLK_SRC_HSE HSE selected as system clock
 *     @arg RCC_SYSCLK_SRC_PLLCLK PLL selected as system clock
 */
void RCC_ConfigSysclk(uint32_t RCC_SYSCLKSource)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_SYSCLK_SRC(RCC_SYSCLKSource));
    tmpregister = RCC->CFG;
    /* Clear SW[1:0] bits */
    tmpregister &= CFG_SCLKSW_MASK;
    /* Set SW[1:0] bits according to RCC_SYSCLKSource value */
    tmpregister |= RCC_SYSCLKSource;
    /* Store the new value */
    RCC->CFG = tmpregister;
}

/**
 * @brief  Returns the clock source used as system clock.
 * @return The clock source used as system clock. The returned value can
 *   be one of the following:
 *     - 0x00: HSI used as system clock
 *     - 0x04: HSE used as system clock
 *     - 0x08: PLL used as system clock
 */
uint8_t RCC_GetSysclkSrc(void)
{
    return ((uint8_t)(RCC->CFG & CFG_SCLKSTS_MASK));
}

/**
 * @brief  Configures the AHB clock (HCLK).
 * @param RCC_SYSCLK defines the AHB clock divider. This clock is derived from
 *   the system clock (SYSCLK).
 *   This parameter can be one of the following values:
 *     @arg RCC_SYSCLK_DIV1 AHB clock = SYSCLK
 *     @arg RCC_SYSCLK_DIV2 AHB clock = SYSCLK/2
 *     @arg RCC_SYSCLK_DIV4 AHB clock = SYSCLK/4
 *     @arg RCC_SYSCLK_DIV8 AHB clock = SYSCLK/8
 *     @arg RCC_SYSCLK_DIV16 AHB clock = SYSCLK/16
 *     @arg RCC_SYSCLK_DIV64 AHB clock = SYSCLK/64
 *     @arg RCC_SYSCLK_DIV128 AHB clock = SYSCLK/128
 *     @arg RCC_SYSCLK_DIV256 AHB clock = SYSCLK/256
 *     @arg RCC_SYSCLK_DIV512 AHB clock = SYSCLK/512
 */
void RCC_ConfigHclk(uint32_t RCC_SYSCLK)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_SYSCLK_DIV(RCC_SYSCLK));
    tmpregister = RCC->CFG;
    /* Clear HPRE[3:0] bits */
    tmpregister &= CFG_AHBPRES_RESET_MASK;
    /* Set HPRE[3:0] bits according to RCC_SYSCLK value */
    tmpregister |= RCC_SYSCLK;
    /* Store the new value */
    RCC->CFG = tmpregister;
}

/**
 * @brief  Configures the Low Speed APB clock (PCLK1).
 * @param RCC_HCLK defines the APB1 clock divider. This clock is derived from
 *   the AHB clock (HCLK).
 *   This parameter can be one of the following values:
 *     @arg RCC_HCLK_DIV1 APB1 clock = HCLK
 *     @arg RCC_HCLK_DIV2 APB1 clock = HCLK/2
 *     @arg RCC_HCLK_DIV4 APB1 clock = HCLK/4
 *     @arg RCC_HCLK_DIV8 APB1 clock = HCLK/8
 *     @arg RCC_HCLK_DIV16 APB1 clock = HCLK/16
 */
void RCC_ConfigPclk1(uint32_t RCC_HCLK)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_HCLK_DIV(RCC_HCLK));
    tmpregister = RCC->CFG;
    /* Clear PPRE1[2:0] bits */
    tmpregister &= CFG_APB1PRES_RESET_MASK;
    /* Set PPRE1[2:0] bits according to RCC_HCLK value */
    tmpregister |= RCC_HCLK;
    /* Store the new value */
    RCC->CFG = tmpregister;
}

/**
 * @brief  Configures the High Speed APB clock (PCLK2).
 * @param RCC_HCLK defines the APB2 clock divider. This clock is derived from
 *   the AHB clock (HCLK).
 *   This parameter can be one of the following values:
 *     @arg RCC_HCLK_DIV1 APB2 clock = HCLK
 *     @arg RCC_HCLK_DIV2 APB2 clock = HCLK/2
 *     @arg RCC_HCLK_DIV4 APB2 clock = HCLK/4
 *     @arg RCC_HCLK_DIV8 APB2 clock = HCLK/8
 *     @arg RCC_HCLK_DIV16 APB2 clock = HCLK/16
 */
void RCC_ConfigPclk2(uint32_t RCC_HCLK)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_HCLK_DIV(RCC_HCLK));
    tmpregister = RCC->CFG;
    /* Clear PPRE2[2:0] bits */
    tmpregister &= CFG_APB2PRES_RESET_MASK;
    /* Set PPRE2[2:0] bits according to RCC_HCLK value */
    tmpregister |= RCC_HCLK << 3;
    /* Store the new value */
    RCC->CFG = tmpregister;
}

/**
 * @brief  Enables or disables the specified RCC interrupts.
 * @param RccInt specifies the RCC interrupt sources to be enabled or disabled.
 *
 *   this parameter can be any combination of the following values
 *     @arg RCC_INT_LSIRDIF LSI ready interrupt
 *     @arg RCC_INT_LSERDIF LSE ready interrupt
 *     @arg RCC_INT_HSIRDIF HSI ready interrupt
 *     @arg RCC_INT_HSERDIF HSE ready interrupt
 *     @arg RCC_INT_PLLRDIF PLL ready interrupt
 *
 * @param Cmd new state of the specified RCC interrupts.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_ConfigInt(uint8_t RccInt, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_RCC_INT(RccInt));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        /* Perform Byte access to RCC_CLKINT bits to enable the selected interrupts */
        *(__IO uint8_t*)CLKINT_BYTE2_ADDR |= RccInt;
    }
    else
    {
        /* Perform Byte access to RCC_CLKINT bits to disable the selected interrupts */
        *(__IO uint8_t*)CLKINT_BYTE2_ADDR &= (uint8_t)~RccInt;
    }
}

/**
 * @brief  Configures the TIM1/8 clock (TIM1/8CLK).
 * @param RCC_TIM18CLKSource specifies the TIM1/8 clock source.
 *   This parameter can be one of the following values:
 *     @arg RCC_TIM18CLK_SRC_TIM18CLK
 *     @arg RCC_TIM18CLKSource_AHBCLK
 */
void RCC_ConfigTim18Clk(uint32_t RCC_TIM18CLKSource)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_TIM18CLKSRC(RCC_TIM18CLKSource));

    tmpregister = RCC->CFG2;
    /* Clear TIMCLK_SEL bits */
    tmpregister &= CFG2_TIM18CLKSEL_RESET_MASK;
    /* Set TIMCLK_SEL bits according to RCC_TIM18CLKSource value */
    tmpregister |= RCC_TIM18CLKSource;

    /* Store the new value */
    RCC->CFG2 = tmpregister;
}

/**
 * @brief  Configures the RNGCCLK prescaler.
 * @param RCC_RNGCCLKPrescaler specifies the RNGCCLK prescaler.
 *   This parameter can be one of the following values:
 *     @arg RCC_RNGCCLK_SYSCLK_DIV1 RNGCPRE[24:28] = 00000, SYSCLK Divided By 1
 *     @arg RCC_RNGCCLK_SYSCLK_DIV2 RNGCPRE[24:28] = 00001, SYSCLK Divided By 2
 *     @arg RCC_RNGCCLK_SYSCLK_DIV3 RNGCPRE[24:28] = 00002, SYSCLK Divided By 3
 *               ...
 *     @arg RCC_RNGCCLK_SYSCLK_DIV31 RNGCPRE[24:28] = 11110, SYSCLK Divided By 31
 *     @arg RCC_RNGCCLK_SYSCLK_DIV32 RNGCPRE[24:28] = 11111, SYSCLK Divided By 32
 */
void RCC_ConfigRngcClk(uint32_t RCC_RNGCCLKPrescaler)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_RNGCCLKPRE(RCC_RNGCCLKPrescaler));

    tmpregister = RCC->CFG2;
    /* Clear RNGCPRE[3:0] bits */
    tmpregister &= CFG2_RNGCPRES_RESET_MASK;
    /* Set RNGCPRE[3:0] bits according to RCC_RNGCCLKPrescaler value */
    tmpregister |= RCC_RNGCCLKPrescaler;

    /* Store the new value */
    RCC->CFG2 = tmpregister;
}

/**
 * @brief  Configures the ADCx 1M clock (ADC1MCLK).
 * @param RCC_ADC1MCLKSource specifies the ADC1M clock source.
 *   This parameter can be on of the following values:
 *     @arg RCC_ADC1MCLK_SRC_HSI
 *     @arg RCC_ADC1MCLK_SRC_HSE
 *
 * @param RCC_ADC1MPrescaler specifies the ADC1M clock prescaler.
 *   This parameter can be on of the following values:
 *     @arg RCC_ADC1MCLK_DIV1 ADC1M clock = RCC_ADC1MCLKSource_xxx/1
 *     @arg RCC_ADC1MCLK_DIV2 ADC1M clock = RCC_ADC1MCLKSource_xxx/2
 *     @arg RCC_ADC1MCLK_DIV3 ADC1M clock = RCC_ADC1MCLKSource_xxx/3
 *               ...
 *     @arg RCC_ADC1MCLK_DIV31 ADC1M clock = RCC_ADC1MCLKSource_xxx/31
 *     @arg RCC_ADC1MCLK_DIV32 ADC1M clock = RCC_ADC1MCLKSource_xxx/32
 */
void RCC_ConfigAdc1mClk(uint32_t RCC_ADC1MCLKSource, uint32_t RCC_ADC1MPrescaler)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_ADC1MCLKSRC(RCC_ADC1MCLKSource));
    assert_param(IS_RCC_ADC1MCLKPRE(RCC_ADC1MPrescaler));

    tmpregister = RCC->CFG2;
    /* Clear ADC1MSEL and ADC1MPRE[4:0] bits */
    tmpregister &= CFG2_ADC1MSEL_RESET_MASK;
    tmpregister &= CFG2_ADC1MPRES_RESET_MASK;
    /* Set ADC1MSEL bits according to RCC_ADC1MCLKSource value */
    tmpregister |= RCC_ADC1MCLKSource;
    /* Set ADC1MPRE[4:0] bits according to RCC_ADC1MPrescaler value */
    tmpregister |= RCC_ADC1MPrescaler;

    /* Store the new value */
    RCC->CFG2 = tmpregister;
}

/**
 * @brief  Configures the ADCPLLCLK prescaler, and enable/disable ADCPLLCLK.
 * @param RCC_ADCPLLCLKPrescaler specifies the ADCPLLCLK prescaler.
 *   This parameter can be on of the following values:
 *     @arg RCC_ADCPLLCLK_DISABLE ADCPLLCLKPRES[4:0] = 0xxxx, ADC Pll Clock Disable
 *     @arg RCC_ADCPLLCLK_DIV1 ADCPLLCLKPRES[4:0] = 10000, Pll Clock Divided By 1
 *     @arg RCC_ADCPLLCLK_DIV2 ADCPLLCLKPRES[4:0] = 10001, Pll Clock Divided By 2
 *     @arg RCC_ADCPLLCLK_DIV4 ADCPLLCLKPRES[4:0] = 10010, Pll Clock Divided By 4
 *     @arg RCC_ADCPLLCLK_DIV6 ADCPLLCLKPRES[4:0] = 10011, Pll Clock Divided By 6
 *     @arg RCC_ADCPLLCLK_DIV8 ADCPLLCLKPRES[4:0] = 10100, Pll Clock Divided By 8
 *     @arg RCC_ADCPLLCLK_DIV10 ADCPLLCLKPRES[4:0] = 10101, Pll Clock Divided By 10
 *     @arg RCC_ADCPLLCLK_DIV12 ADCPLLCLKPRES[4:0] = 10110, Pll Clock Divided By 12
 *     @arg RCC_ADCPLLCLK_DIV16 ADCPLLCLKPRES[4:0] = 10111, Pll Clock Divided By 16
 *     @arg RCC_ADCPLLCLK_DIV32 ADCPLLCLKPRES[4:0] = 11000, Pll Clock Divided By 32
 *     @arg RCC_ADCPLLCLK_DIV64 ADCPLLCLKPRES[4:0] = 11001, Pll Clock Divided By 64
 *     @arg RCC_ADCPLLCLK_DIV128 ADCPLLCLKPRES[4:0] = 11010, Pll Clock Divided By 128
 *     @arg RCC_ADCPLLCLK_DIV256 ADCPLLCLKPRES[4:0] = 11011, Pll Clock Divided By 256
 *     @arg RCC_ADCPLLCLK_DIV256 ADCPLLCLKPRES[4:0] = others, Pll Clock Divided By 256
 *
 * @param Cmd specifies the ADCPLLCLK enable/disable selection.
 *   This parameter can be on of the following values:
 *     @arg ENABLE enable ADCPLLCLK
 *     @arg DISABLE disable ADCPLLCLK
 */
void RCC_ConfigAdcPllClk(uint32_t RCC_ADCPLLCLKPrescaler, FunctionalState Cmd)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_ADCPLLCLKPRE(RCC_ADCPLLCLKPrescaler));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    tmpregister = RCC->CFG2;
    /* Clear ADCPLLPRES[4:0] bits */
    tmpregister &= CFG2_ADCPLLPRES_RESET_MASK;

    if (Cmd != DISABLE)
    {
        tmpregister |= RCC_ADCPLLCLKPrescaler;
    }
    else
    {
        tmpregister |= RCC_ADCPLLCLKPrescaler;
        tmpregister &= RCC_ADCPLLCLK_DISABLE;
    }

    /* Store the new value */
    RCC->CFG2 = tmpregister;
}

/**
 * @brief  Configures the ADCHCLK prescaler.
 * @param RCC_ADCHCLKPrescaler specifies the ADCHCLK prescaler.
 *   This parameter can be on of the following values:
 *     @arg RCC_ADCHCLK_DIV1 ADCHCLKPRE[3:0] = 0000, HCLK Clock Divided By 1
 *     @arg RCC_ADCHCLK_DIV2 ADCHCLKPRE[3:0] = 0001, HCLK Clock Divided By 2
 *     @arg RCC_ADCHCLK_DIV4 ADCHCLKPRE[3:0] = 0010, HCLK Clock Divided By 4
 *     @arg RCC_ADCHCLK_DIV6 ADCHCLKPRE[3:0] = 0011, HCLK Clock Divided By 6
 *     @arg RCC_ADCHCLK_DIV8 ADCHCLKPRE[3:0] = 0100, HCLK Clock Divided By 8
 *     @arg RCC_ADCHCLK_DIV10 ADCHCLKPRE[3:0] = 0101, HCLK Clock Divided By 10
 *     @arg RCC_ADCHCLK_DIV12 ADCHCLKPRE[3:0] = 0110, HCLK Clock Divided By 12
 *     @arg RCC_ADCHCLK_DIV16 ADCHCLKPRE[3:0] = 0111, HCLK Clock Divided By 16
 *     @arg RCC_ADCHCLK_DIV32 ADCHCLKPRE[3:0] = 1000, HCLK Clock Divided By 32
 *     @arg RCC_ADCHCLK_DIV32 ADCHCLKPRE[3:0] = others, HCLK Clock Divided By 32
 */
void RCC_ConfigAdcHclk(uint32_t RCC_ADCHCLKPrescaler)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_ADCHCLKPRE(RCC_ADCHCLKPrescaler));

    tmpregister = RCC->CFG2;
    /* Clear ADCHPRE[3:0] bits */
    tmpregister &= CFG2_ADCHPRES_RESET_MASK;
    /* Set ADCHPRE[3:0] bits according to RCC_ADCHCLKPrescaler value */
    tmpregister |= RCC_ADCHCLKPrescaler;

    /* Store the new value */
    RCC->CFG2 = tmpregister;
}

/**
 * @brief  Configures the TRNG 1M clock (TRNG1MCLK).
 * @param RCC_TRNG1MCLKSource specifies the TRNG1M clock source.
 *   This parameter can be on of the following values:
 *     @arg RCC_TRNG1MCLK_SRC_HSI
 *     @arg RCC_TRNG1MCLK_SRC_HSE
 *
 * @param RCC_TRNG1MPrescaler specifies the TRNG1M prescaler.
 *   This parameter can be on of the following values:
 *     @arg RCC_TRNG1MCLKDiv_2 TRNG1M clock = RCC_TRNG1MCLKSource_xxx/2
 *     @arg RCC_TRNG1MCLKDiv_4 TRNG1M clock = RCC_TRNG1MCLKSource_xxx/4
 *     @arg RCC_TRNG1MCLKDiv_6 TRNG1M clock = RCC_TRNG1MCLKSource_xxx/6
 *               ...
 *     @arg RCC_TRNG1MCLKDiv_30 TRNG1M clock = RCC_TRNG1MCLKSource_xxx/30
 *     @arg RCC_TRNG1MCLKDiv_32 TRNG1M clock = RCC_TRNG1MCLKSource_xxx/32
 */
void RCC_ConfigTrng1mClk(uint32_t RCC_TRNG1MCLKSource, uint32_t RCC_TRNG1MPrescaler)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_TRNG1MCLK_SRC(RCC_TRNG1MCLKSource));
    assert_param(IS_RCC_TRNG1MCLKPRE(RCC_TRNG1MPrescaler));

    tmpregister = RCC->CFG3;
    /* Clear TRNG1MSEL and TRNG1MPRE[4:0] bits */
    tmpregister &= CFGR3_TRNG1MSEL_RESET_MASK;
    tmpregister &= CFGR3_TRNG1MPRES_RESET_MASK;
    /* Set TRNG1MSEL bits according to RCC_TRNG1MCLKSource value */
    tmpregister |= RCC_TRNG1MCLKSource;
    /* Set TRNG1MPRE[4:0] bits according to RCC_TRNG1MPrescaler value */
    tmpregister |= RCC_TRNG1MPrescaler;

    /* Store the new value */
    RCC->CFG3 = tmpregister;
}

/**
 * @brief  Enable/disable TRNG clock (TRNGCLK).
 * @param Cmd specifies the TRNGCLK enable/disable selection.
 *   This parameter can be on of the following values:
 *     @arg ENABLE enable TRNGCLK
 *     @arg DISABLE disable TRNGCLK
 */
void RCC_EnableTrng1mClk(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    if (Cmd != DISABLE)
    {
        RCC->CFG3 |= RCC_TRNG1MCLK_ENABLE;
    }
    else
    {
        RCC->CFG3 &= RCC_TRNG1MCLK_DISABLE;
    }
}
/**
 * @brief  Configures the External Low Speed oscillator (LSE) Xtal drive current.
 * @param LSE_Trim specifies LSE Driver Trim Level.
 *        Trim value rang 0x4~0x1FF 
 * @note LSE_Trim bit2 must be set to 1.
 * @note Better compatibility can be achieved by adjusting the drive current.
 */
void RCC_ConfigLSETrim(uint32_t LSE_Trim)
{
    uint32_t tmpregister = 0;
    
    /*Check and set trim value */
    (LSE_Trim>0x1FF) ? (LSE_Trim = 0x1FF):(LSE_Trim &= 0x1FF);
    
    tmpregister = LSE_TRIM_ADDR;
	tmpregister &= 0xFFFFFE00;
	tmpregister |= LSE_Trim;
	
	LSE_TRIM_ADDR = 0x55AA;
	LSE_TRIM_ADDR = tmpregister;
}

/**
 * @brief  Configures the External Low Speed oscillator (LSE).
 * @param RCC_LSE specifies the new state of the LSE.
 *   This parameter can be one of the following values:
 *     @arg RCC_LSE_DISABLE LSE oscillator OFF
 *     @arg RCC_LSE_ENABLE LSE oscillator ON
 *     @arg RCC_LSE_BYPASS LSE oscillator bypassed with external clock
 */
void RCC_ConfigLse(uint8_t RCC_LSE)
{
    /* Check the parameters */
    assert_param(IS_RCC_LSE(RCC_LSE));
    /* Reset LSEON and LSEBYP bits before configuring the LSE ------------------*/
    /* Reset LSEON bit */
    *(__IO uint8_t*)BDCTRL_ADDR = RCC_LSE_DISABLE;
    /* Reset LSEBYP bit */
    *(__IO uint8_t*)BDCTRL_ADDR = RCC_LSE_DISABLE;
    /* Configure LSE (RCC_LSE_DISABLE is already covered by the code section above) */
    switch (RCC_LSE)
    {
    case RCC_LSE_ENABLE:
        /* Set LSEON bit */
        *(__IO uint8_t*)BDCTRL_ADDR = RCC_LSE_ENABLE;
        /* Better compatibility can be achieved by adjusting the drive current. */
        RCC_ConfigLSETrim(0x145);
        break;

    case RCC_LSE_BYPASS:
        /* Set LSEBYP and LSEON bits */
        *(__IO uint8_t*)BDCTRL_ADDR = RCC_LSE_BYPASS | RCC_LSE_ENABLE;
        break;

    default:
        break;
    }
}

/**
 * @brief  Enables or disables the Internal Low Speed oscillator (LSI).
 * @note   LSI can not be disabled if the IWDG is running.
 * @param Cmd new state of the LSI. This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableLsi(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    *(__IO uint32_t*)CTRLSTS_LSIEN_BB = (uint32_t)Cmd;
}

/**
 * @brief  Configures the RTC clock (RTCCLK).
 * @note   Once the RTC clock is selected it can't be changed unless the Backup domain is reset.
 * @param RCC_RTCCLKSource specifies the RTC clock source.
 *   This parameter can be one of the following values:
 *     @arg RCC_RTCCLK_SRC_LSE LSE selected as RTC clock
 *     @arg RCC_RTCCLK_SRC_LSI LSI selected as RTC clock
 *     @arg RCC_RTCCLK_SRC_HSE_DIV128 HSE clock divided by 128 selected as RTC clock
 */
void RCC_ConfigRtcClk(uint32_t RCC_RTCCLKSource)
{
    uint32_t bdctrl_tmp;
    /* Check the parameters */
    assert_param(IS_RCC_RTCCLK_SRC(RCC_RTCCLKSource));

    /* Clear the RTC clock source */
    bdctrl_tmp = RCC->BDCTRL;
    bdctrl_tmp &= (~0x00000300);

    /* Select the RTC clock source */
    bdctrl_tmp |= RCC_RTCCLKSource;
    RCC->BDCTRL = bdctrl_tmp;
}

/**
 * @brief  Enables or disables the RTC clock.
 * @note   This function must be used only after the RTC clock was selected using the RCC_ConfigRtcClk function.
 * @param Cmd new state of the RTC clock. This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableRtcClk(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    *(__IO uint32_t*)BDCTRL_RTCEN_BB = (uint32_t)Cmd;
}

/**
 * @brief  Returns the frequencies of different on chip clocks.
 * @param RCC_Clocks pointer to a RCC_ClocksType structure which will hold
 *         the clocks frequencies.
 * @note   The result of this function could be not correct when using
 *         fractional value for HSE crystal.
 */
void RCC_GetClocksFreqValue(RCC_ClocksType* RCC_Clocks)
{
    uint32_t tmp = 0, pllclk = 0, pllmull = 0, pllsource = 0, presc = 0;

    /* Get PLL clock source and multiplication factor ----------------------*/
    pllmull   = RCC->CFG & CFG_PLLMULFCT_MASK;
    pllsource = RCC->CFG & CFG_PLLSRC_MASK;

    if ((pllmull & RCC_CFG_PLLMULFCT_4) == 0)
    {
        pllmull = (pllmull >> 18) + 2; // PLLMUL[4]=0
    }
    else
    {
        pllmull = ((pllmull >> 18) - 496) + 1; // PLLMUL[4]=1
    }

    if (pllsource == 0x00)
    { /* HSI oscillator clock divided by 2 selected as PLL clock entry */
        pllclk = (HSI_VALUE >> 1) * pllmull;
    }
    else
    {
        /* HSE selected as PLL clock entry */
        if ((RCC->CFG & CFG_PLLHSEPRES_MASK) != (uint32_t)RESET)
        { /* HSE oscillator clock divided by 2 */
            pllclk = (HSE_VALUE >> 1) * pllmull;
        }
        else
        {
            pllclk = HSE_VALUE * pllmull;
        }
    }

    /* Get SYSCLK source -------------------------------------------------------*/
    tmp = RCC->CFG & CFG_SCLKSTS_MASK;

    switch (tmp)
    {
    case 0x00: /* HSI used as system clock */
        RCC_Clocks->SysclkFreq = HSI_VALUE;
        break;
    case 0x04: /* HSE used as system clock */
        RCC_Clocks->SysclkFreq = HSE_VALUE;
        break;
    case 0x08: /* PLL used as system clock */
        RCC_Clocks->SysclkFreq = pllclk;
        break;

    default:
        RCC_Clocks->SysclkFreq = HSI_VALUE;
        break;
    }

    /* Compute HCLK, PCLK1, PCLK2 and ADCCLK clocks frequencies ----------------*/
    /* Get HCLK prescaler */
    tmp   = RCC->CFG & CFG_AHBPRES_SET_MASK;
    tmp   = tmp >> 4;
    presc = s_ApbAhbPresTable[tmp];
    /* HCLK clock frequency */
    RCC_Clocks->HclkFreq = RCC_Clocks->SysclkFreq >> presc;
    /* Get PCLK1 prescaler */
    tmp   = RCC->CFG & CFG_APB1PRES_SET_MASK;
    tmp   = tmp >> 8;
    presc = s_ApbAhbPresTable[tmp];
    /* PCLK1 clock frequency */
    RCC_Clocks->Pclk1Freq = RCC_Clocks->HclkFreq >> presc;
    /* Get PCLK2 prescaler */
    tmp   = RCC->CFG & CFG_APB2PRES_SET_MASK;
    tmp   = tmp >> 11;
    presc = s_ApbAhbPresTable[tmp];
    /* PCLK2 clock frequency */
    RCC_Clocks->Pclk2Freq = RCC_Clocks->HclkFreq >> presc;

    /* Get ADCHCLK prescaler */
    tmp   = RCC->CFG2 & CFG2_ADCHPRES_SET_MASK;
    presc = s_AdcHclkPresTable[tmp];
    /* ADCHCLK clock frequency */
    RCC_Clocks->AdcHclkFreq = RCC_Clocks->HclkFreq / presc;
    /* Get ADCPLLCLK prescaler */
    tmp   = RCC->CFG2 & CFG2_ADCPLLPRES_SET_MASK;
    tmp   = tmp >> 4;
    presc = s_AdcPllClkPresTable[(tmp & 0xF)]; // ignore BIT5
    /* ADCPLLCLK clock frequency */
    RCC_Clocks->AdcPllClkFreq = pllclk / presc;
}

/**
 * @brief  Enables or disables the AHB peripheral clock.
 * @param RCC_AHBPeriph specifies the AHB peripheral to gates its clock.
 *
 *   this parameter can be any combination of the following values:
 *     @arg RCC_AHB_PERIPH_DMA1
 *     @arg RCC_AHB_PERIPH_DMA2
 *     @arg RCC_AHB_PERIPH_SRAM
 *     @arg RCC_AHB_PERIPH_FLITF
 *     @arg RCC_AHB_PERIPH_CRC
 *     @arg RCC_AHB_PERIPH_RNGC
 *     @arg RCC_AHB_PERIPH_SDIO
 *     @arg RCC_AHB_PERIPH_SAC
 *     @arg RCC_AHB_PERIPH_ADC1
 *     @arg RCC_AHB_PERIPH_ADC2
 *     @arg RCC_AHB_PERIPH_ADC3
 *     @arg RCC_AHB_PERIPH_ADC4
 *     @arg RCC_AHB_PERIPH_QSPI
 *
 * @note SRAM and FLITF clock can be disabled only during sleep mode.
 * @param Cmd new state of the specified peripheral clock.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableAHBPeriphClk(uint32_t RCC_AHBPeriph, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_RCC_AHB_PERIPH(RCC_AHBPeriph));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    if (Cmd != DISABLE)
    {
        RCC->AHBPCLKEN |= RCC_AHBPeriph;
    }
    else
    {
        RCC->AHBPCLKEN &= ~RCC_AHBPeriph;
    }
}

/**
 * @brief  Enables or disables the High Speed APB (APB2) peripheral clock.
 * @param RCC_APB2Periph specifies the APB2 peripheral to gates its clock.
 *   This parameter can be any combination of the following values:
 *     @arg RCC_APB2_PERIPH_AFIO, RCC_APB2_PERIPH_GPIOA, RCC_APB2_PERIPH_GPIOB,
 *          RCC_APB2_PERIPH_GPIOC, RCC_APB2_PERIPH_GPIOD, RCC_APB2_PERIPH_GPIOE,
 *          RCC_APB2_PERIPH_GPIOF, RCC_APB2_PERIPH_GPIOG, RCC_APB2_PERIPH_TIM1,
 *          RCC_APB2_PERIPH_SPI1, RCC_APB2_PERIPH_TIM8, RCC_APB2_PERIPH_USART1,
 *          RCC_APB2_PERIPH_UART6, RCC_APB2_PERIPH_UART7, RCC_APB2_PERIPH_I2C3,
 *          RCC_APB2_PERIPH_I2C4
 * @param Cmd new state of the specified peripheral clock.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableAPB2PeriphClk(uint32_t RCC_APB2Periph, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_RCC_APB2_PERIPH(RCC_APB2Periph));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        RCC->APB2PCLKEN |= RCC_APB2Periph;
    }
    else
    {
        RCC->APB2PCLKEN &= ~RCC_APB2Periph;
    }
}

/**
 * @brief  Enables or disables the Low Speed APB (APB1) peripheral clock.
 * @param RCC_APB1Periph specifies the APB1 peripheral to gates its clock.
 *   This parameter can be any combination of the following values:
 *     @arg RCC_APB1_PERIPH_TIM2, RCC_APB1_PERIPH_TIM3, RCC_APB1_PERIPH_TIM4,
 *          RCC_APB1_PERIPH_TIM5, RCC_APB1_PERIPH_TIM6, RCC_APB1_PERIPH_TIM7,
 *          RCC_APB1_PERIPH_COMP, RCC_APB1_PERIPH_COMP_FILT, RCC_APB1_PERIPH_WWDG,
 *          RCC_APB1_PERIPH_SPI2, RCC_APB1_PERIPH_SPI3, RCC_APB1_PERIPH_USART2,
 *          RCC_APB1_PERIPH_USART3, RCC_APB1_PERIPH_UART4, RCC_APB1_PERIPH_UART5,
 *          RCC_APB1_PERIPH_I2C1, RCC_APB1_PERIPH_I2C2, RCC_APB1_PERIPH_CAN1,
 *          RCC_APB1_PERIPH_CAN2, RCC_APB1_PERIPH_BKP, RCC_APB1_PERIPH_PWR,
 *          RCC_APB1_PERIPH_DAC, RCC_APB1_PERIPH_OPAMP
 *
 * @param Cmd new state of the specified peripheral clock.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableAPB1PeriphClk(uint32_t RCC_APB1Periph, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_RCC_APB1_PERIPH(RCC_APB1Periph));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        RCC->APB1PCLKEN |= RCC_APB1Periph;
    }
    else
    {
        RCC->APB1PCLKEN &= ~RCC_APB1Periph;
    }
}

/**
 * @brief Forces or releases AHB peripheral reset.
 * @param RCC_AHBPeriph specifies the AHB peripheral to reset.
 *   This parameter can be any combination of the following values:
 *     @arg   RCC_AHB_PERIPH_QSPI.
 *            RCC_AHB_PERIPH_ADC4.
 *            RCC_AHB_PERIPH_ADC3.
 *            RCC_AHB_PERIPH_ADC2.
 *            RCC_AHB_PERIPH_ADC1.
 *            RCC_AHB_PERIPH_SAC.
 *            RCC_AHB_PERIPH_RNGC.
 * @param Cmd new state of the specified peripheral reset. This parameter can be ENABLE or DISABLE.
 */
void RCC_EnableAHBPeriphReset(uint32_t RCC_AHBPeriph, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_RCC_AHB_PERIPH(RCC_AHBPeriph));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        RCC->AHBPRST |= RCC_AHBPeriph;
    }
    else
    {
        RCC->AHBPRST &= ~RCC_AHBPeriph;
    }
}

/**
 * @brief  Forces or releases High Speed APB (APB2) peripheral reset.
 * @param RCC_APB2Periph specifies the APB2 peripheral to reset.
 *   This parameter can be any combination of the following values:
 *     @arg RCC_APB2_PERIPH_AFIO, RCC_APB2_PERIPH_GPIOA, RCC_APB2_PERIPH_GPIOB,
 *          RCC_APB2_PERIPH_GPIOC, RCC_APB2_PERIPH_GPIOD, RCC_APB2_PERIPH_GPIOE,
 *          RCC_APB2_PERIPH_GPIOF, RCC_APB2_PERIPH_GPIOG, RCC_APB2_PERIPH_TIM1,
 *          RCC_APB2_PERIPH_SPI1, RCC_APB2_PERIPH_TIM8, RCC_APB2_PERIPH_USART1,
 *          RCC_APB2_PERIPH_UART6, RCC_APB2_PERIPH_UART7, RCC_APB2_PERIPH_I2C3, 
 *          RCC_APB2_PERIPH_I2C4
 * @param Cmd new state of the specified peripheral reset.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableAPB2PeriphReset(uint32_t RCC_APB2Periph, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_RCC_APB2_PERIPH(RCC_APB2Periph));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        RCC->APB2PRST |= RCC_APB2Periph;
    }
    else
    {
        RCC->APB2PRST &= ~RCC_APB2Periph;
    }
}

/**
 * @brief  Forces or releases Low Speed APB (APB1) peripheral reset.
 * @param RCC_APB1Periph specifies the APB1 peripheral to reset.
 *   This parameter can be any combination of the following values:
 *     @arg RCC_APB1_PERIPH_TIM2, RCC_APB1_PERIPH_TIM3, RCC_APB1_PERIPH_TIM4,
 *          RCC_APB1_PERIPH_TIM5, RCC_APB1_PERIPH_TIM6, RCC_APB1_PERIPH_TIM7,
 *          RCC_APB1_PERIPH_WWDG, RCC_APB1_PERIPH_SPI2, RCC_APB1_PERIPH_SPI3,
 *          RCC_APB1_PERIPH_USART2, RCC_APB1_PERIPH_USART3, RCC_APB1_PERIPH_UART4,
 *          RCC_APB1_PERIPH_UART5, RCC_APB1_PERIPH_I2C1, RCC_APB1_PERIPH_I2C2,
 *          RCC_APB1_PERIPH_CAN1, RCC_APB1_PERIPH_CAN2, RCC_APB1_PERIPH_BKP,
 *          RCC_APB1_PERIPH_PWR, RCC_APB1_PERIPH_DAC          
 * @param Cmd new state of the specified peripheral clock.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableAPB1PeriphReset(uint32_t RCC_APB1Periph, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_RCC_APB1_PERIPH(RCC_APB1Periph));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        RCC->APB1PRST |= RCC_APB1Periph;
    }
    else
    {
        RCC->APB1PRST &= ~RCC_APB1Periph;
    }
}

/**
 * @brief  BOR reset enable.
 * @param Cmd new state of the BOR reset.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableBORReset(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        RCC->CFG3 |= RCC_BOR_RST_ENABLE;
    }
    else
    {
        RCC->CFG3 &= ~RCC_BOR_RST_ENABLE;
    }
}

/**
 * @brief  Forces or releases the Backup domain reset.
 * @param Cmd new state of the Backup domain reset.
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableBackupReset(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    *(__IO uint32_t*)BDCTRL_BDSFTRST_BB = (uint32_t)Cmd;
}

/**
 * @brief  Enables or disables the Clock Security System.
 * @param Cmd new state of the Clock Security System..
 *   This parameter can be: ENABLE or DISABLE.
 */
void RCC_EnableClockSecuritySystem(FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    *(__IO uint32_t*)CTRL_CLKSSEN_BB = (uint32_t)Cmd;
}

/**
 * @brief  Configures the MCO PLL clock prescaler.
 * @param RCC_MCOPLLCLKPrescaler specifies the MCO PLL clock prescaler.
 *   This parameter can be on of the following values:
 *     @arg RCC_MCO_PLLCLK_DIV2 MCOPRE[3:0] = 0010, PLL Clock Divided By 2
 *     @arg RCC_MCO_PLLCLK_DIV3 MCOPRE[3:0] = 0011, PLL Clock Divided By 3
 *     @arg RCC_MCO_PLLCLK_DIV4 MCOPRE[3:0] = 0100, PLL Clock Divided By 4
 *     @arg RCC_MCO_PLLCLK_DIV5 MCOPRE[3:0] = 0101, PLL Clock Divided By 5
 *                 ...
 *     @arg RCC_MCO_PLLCLK_DIV13 MCOPRE[3:0] = 1101, PLL Clock Divided By 13
 *     @arg RCC_MCO_PLLCLK_DIV14 MCOPRE[3:0] = 1110, PLL Clock Divided By 14
 *     @arg RCC_MCO_PLLCLK_DIV15 MCOPRE[3:0] = 1111, PLL Clock Divided By 15
 */
void RCC_ConfigMcoPllClk(uint32_t RCC_MCOPLLCLKPrescaler)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_MCOPLLCLKPRE(RCC_MCOPLLCLKPrescaler));

    tmpregister = RCC->CFG;
    /* Clear MCOPRE[3:0] bits */
    tmpregister &= ((uint32_t)0x0FFFFFFF);
    /* Set MCOPRE[3:0] bits according to RCC_ADCHCLKPrescaler value */
    tmpregister |= RCC_MCOPLLCLKPrescaler;

    /* Store the new value */
    RCC->CFG = tmpregister;
}

/**
 * @brief  Selects the clock source to output on MCO pin.
 * @param RCC_MCO specifies the clock source to output.
 *
 *   this parameter can be one of the following values:
 *     @arg RCC_MCO_NOCLK No clock selected
 *     @arg RCC_MCO_SYSCLK System clock selected
 *     @arg RCC_MCO_HSI HSI oscillator clock selected
 *     @arg RCC_MCO_HSE HSE oscillator clock selected
 *     @arg RCC_MCO_PLLCLK PLL clock divided by xx selected
 *
 */
void RCC_ConfigMco(uint8_t RCC_MCO)
{
    uint32_t tmpregister = 0;
    /* Check the parameters */
    assert_param(IS_RCC_MCO(RCC_MCO));

    tmpregister = RCC->CFG;
    /* Clear MCO[2:0] bits */
    tmpregister &= ((uint32_t)0xF8FFFFFF);
    /* Set MCO[2:0] bits according to RCC_MCO value */
    tmpregister |= ((uint32_t)(RCC_MCO << 24));

    /* Store the new value */
    RCC->CFG = tmpregister;
}

/**
 * @brief  Checks whether the specified RCC flag is set or not.
 * @param RCC_FLAG specifies the flag to check.
 *
 *   this parameter can be one of the following values:
 *     @arg RCC_FLAG_HSIRD HSI oscillator clock ready
 *     @arg RCC_FLAG_HSERD HSE oscillator clock ready
 *     @arg RCC_FLAG_PLLRD PLL clock ready
 *     @arg RCC_FLAG_LSERD LSE oscillator clock ready
 *     @arg RCC_FLAG_LSIRD LSI oscillator clock ready
 *     @arg RCC_FLAG_BORRST BOR reset flag
 *     @arg RCC_FLAG_RETEMC Retention EMC reset flag
 *     @arg RCC_FLAG_BKPEMC BackUp EMC reset flag
 *     @arg RCC_FLAG_RAMRST RAM reset flag
 *     @arg RCC_FLAG_MMURST Mmu reset flag
 *     @arg RCC_FLAG_PINRST Pin reset
 *     @arg RCC_FLAG_PORRST POR/PDR reset
 *     @arg RCC_FLAG_SFTRST Software reset
 *     @arg RCC_FLAG_IWDGRST Independent Watchdog reset
 *     @arg RCC_FLAG_WWDGRST Window Watchdog reset
 *     @arg RCC_FLAG_LPWRRST Low Power reset
 *
 * @return The new state of RCC_FLAG (SET or RESET).
 */
FlagStatus RCC_GetFlagStatus(uint8_t RCC_FLAG)
{
    uint32_t tmp         = 0;
    uint32_t statusreg   = 0;
    FlagStatus bitstatus = RESET;
    /* Check the parameters */
    assert_param(IS_RCC_FLAG(RCC_FLAG));

    /* Get the RCC register index */
    tmp = RCC_FLAG >> 5;
    if (tmp == 1) /* The flag to check is in CTRL register */
    {
        statusreg = RCC->CTRL;
    }
    else if (tmp == 2) /* The flag to check is in BDCTRL register */
    {
        statusreg = RCC->BDCTRL;
    }
    else /* The flag to check is in CTRLSTS register */
    {
        statusreg = RCC->CTRLSTS;
    }

    /* Get the flag position */
    tmp = RCC_FLAG & FLAG_MASK;
    if ((statusreg & ((uint32_t)1 << tmp)) != (uint32_t)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    /* Return the flag status */
    return bitstatus;
}

/**
 * @brief  Clears the RCC reset flags.
 * @note   The reset flags are: RCC_FLAG_PINRST, RCC_FLAG_PORRST, RCC_FLAG_SFTRST,
 *   RCC_FLAG_IWDGRST, RCC_FLAG_WWDGRST, RCC_FLAG_LPWRRST
 */
void RCC_ClrFlag(void)
{
    /* Set RMVF bit to clear the reset flags */
    RCC->CTRLSTS |= CSR_RMRSTF_SET;
    /* RMVF bit should be reset */
    RCC->CTRLSTS &= CSR_RMVF_Reset;
}

/**
 * @brief  Checks whether the specified RCC interrupt has occurred or not.
 * @param RccInt specifies the RCC interrupt source to check.
 *
 *   this parameter can be one of the following values:
 *     @arg RCC_INT_LSIRDIF LSI ready interrupt
 *     @arg RCC_INT_LSERDIF LSE ready interrupt
 *     @arg RCC_INT_HSIRDIF HSI ready interrupt
 *     @arg RCC_INT_HSERDIF HSE ready interrupt
 *     @arg RCC_INT_PLLRDIF PLL ready interrupt
 *
 *     @arg RCC_INT_CLKSSIF Clock Security System interrupt
 *
 * @return The new state of RccInt (SET or RESET).
 */
INTStatus RCC_GetIntStatus(uint8_t RccInt)
{
    INTStatus bitstatus = RESET;
    /* Check the parameters */
    assert_param(IS_RCC_GET_INT(RccInt));

    /* Check the status of the specified RCC interrupt */
    if ((RCC->CLKINT & RccInt) != (uint32_t)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    /* Return the RccInt status */
    return bitstatus;
}

/**
 * @brief  Clears the RCC's interrupt pending bits.
 * @param RccInt specifies the interrupt pending bit to clear.
 *
 *   this parameter can be any combination of the
 *   following values:
 *     @arg RCC_INT_LSIRDIF LSI ready interrupt
 *     @arg RCC_INT_LSERDIF LSE ready interrupt
 *     @arg RCC_INT_HSIRDIF HSI ready interrupt
 *     @arg RCC_INT_HSERDIF HSE ready interrupt
 *     @arg RCC_INT_PLLRDIF PLL ready interrupt
 *
 *     @arg RCC_INT_CLKSSIF Clock Security System interrupt
 */
void RCC_ClrIntPendingBit(uint8_t RccInt)
{
    /* Check the parameters */
    assert_param(IS_RCC_CLR_INT(RccInt));

    /* Perform Byte access to RCC_CLKINT[23:16] bits to clear the selected interrupt
       pending bits */
    *(__IO uint8_t*)CLKINT_BYTE3_ADDR = RccInt;
}

/**
 * @brief  Get internal high-speed clock correction value.
 * @return Trim value.
 */
uint8_t RCC_GetCurHSITrim(void)
{
    uint32_t tmpregister = 0;
    tmpregister = RCC->CTRL;
    return (tmpregister >> 3) & 0x0f;
    
}

/**
 * @brief  Set internal high-speed clock correction value.
 * @param trim_val correction value to set.
 * @param Range of parameter: 0x0~0x1F.
 */
static void RCC_SetHSITrim(uint8_t trim_val)
{
    uint32_t tmpregister = 0;
    tmpregister = RCC->CTRL;
    /* Clear TRIM[7:3] bits */
    tmpregister &= ((uint32_t)0xFFFFFF07);
    /* Set trim[7:3] bits according to trim value */
    tmpregister |= ((uint32_t)(trim_val << 3));
    /* Store the new value */
    RCC->CTRL = tmpregister;
    
}

/**
 * @brief  Config internal high-speed clock correction value.
 * @param trim_type increasing or decreasing frequency.
 *   this parameter can be 0 or 1;(0: add; 1:sub)
 * @param value need to offset trim value.
 *   Range of parameter: 0x0~0xF.
 * @return The state of Config .
 *     @arg 0: success
 *     @arg 1: trim_type error
 *     @arg 2: trim_val error
 */
uint8_t RCC_ConfigHSITrim(uint8_t trim_type,uint8_t value)
{
    uint8_t temp_cur_trim = 0,temp_trim = 0;
    uint32_t temp_val = 0;
    if(trim_type > 1)
        return 1;
    temp_cur_trim = RCC_GetCurHSITrim();
    if(trim_type == 0){//0:+
        temp_val = (uint32_t)(temp_cur_trim + value);
      }else{//1:-
      temp_val = (uint32_t)(0x10 + temp_cur_trim + value);
    }
    if(temp_val > 0x1F){
        return 2;
    }
    temp_trim = (uint8_t)(temp_val & 0xFF);
    RCC_SetHSITrim(temp_trim);
    return 0;
    
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
