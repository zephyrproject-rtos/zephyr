/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

/**
 * @brief fill in AHB/APB buses configuration structure
 */
static inline int stm32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB3:
		LL_APB3_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB4:
		LL_APB4_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB5:
		LL_APB5_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB3:
		LL_AHB3_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB4:
		LL_AHB4_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB5:
		LL_AHB5_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB6:
		LL_AHB6_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AXI:
		LL_AXI_GRP1_EnableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_MLAHB:
		LL_MLAHB_GRP1_EnableClock(pclken->enr);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline int stm32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB3:
		LL_APB3_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB4:
		LL_APB4_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_APB5:
		LL_APB5_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB3:
		LL_AHB3_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB4:
		LL_AHB4_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB5:
		LL_AHB5_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AHB6:
		LL_AHB6_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_AXI:
		LL_AXI_GRP1_DisableClock(pclken->enr);
		break;
	case STM32_CLOCK_BUS_MLAHB:
		LL_MLAHB_GRP1_DisableClock(pclken->enr);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *clock,
					       clock_control_subsys_t sub_system,
					       uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(clock);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_APB1:
		switch (pclken->enr) {
		case LL_APB1_GRP1_PERIPH_TIM2:
		case LL_APB1_GRP1_PERIPH_TIM3:
		case LL_APB1_GRP1_PERIPH_TIM4:
		case LL_APB1_GRP1_PERIPH_TIM5:
		case LL_APB1_GRP1_PERIPH_TIM6:
		case LL_APB1_GRP1_PERIPH_TIM7:
		case LL_APB1_GRP1_PERIPH_TIM12:
		case LL_APB1_GRP1_PERIPH_TIM13:
		case LL_APB1_GRP1_PERIPH_TIM14:
			*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG1PRES);
			break;
		case LL_APB1_GRP1_PERIPH_LPTIM1:
			*rate = LL_RCC_GetLPTIMClockFreq(
					LL_RCC_LPTIM1_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_SPI2:
		case LL_APB1_GRP1_PERIPH_SPI3:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI23_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_USART2:
		case LL_APB1_GRP1_PERIPH_UART4:
			*rate = LL_RCC_GetUARTClockFreq(
					LL_RCC_UART24_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_USART3:
		case LL_APB1_GRP1_PERIPH_UART5:
			*rate = LL_RCC_GetUARTClockFreq(
					LL_RCC_UART35_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_UART7:
		case LL_APB1_GRP1_PERIPH_UART8:
			*rate = LL_RCC_GetUARTClockFreq(
					LL_RCC_UART78_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_I2C1:
		case LL_APB1_GRP1_PERIPH_I2C2:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C12_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_I2C3:
		case LL_APB1_GRP1_PERIPH_I2C5:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C35_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_SPDIF:
			*rate = LL_RCC_GetSPDIFRXClockFreq(
					LL_RCC_SPDIFRX_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_CEC:
			*rate = LL_RCC_GetCECClockFreq(LL_RCC_CEC_CLKSOURCE);
			break;
		case LL_APB1_GRP1_PERIPH_WWDG1:
		case LL_APB1_GRP1_PERIPH_DAC12:
		case LL_APB1_GRP1_PERIPH_MDIOS:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB2:
		switch (pclken->enr) {
		case LL_APB2_GRP1_PERIPH_TIM1:
		case LL_APB2_GRP1_PERIPH_TIM8:
		case LL_APB2_GRP1_PERIPH_TIM15:
		case LL_APB2_GRP1_PERIPH_TIM16:
		case LL_APB2_GRP1_PERIPH_TIM17:
			*rate = LL_RCC_GetTIMGClockFreq(LL_RCC_TIMG2PRES);
			break;
		case LL_APB2_GRP1_PERIPH_SPI1:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI1_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_SPI4:
		case LL_APB2_GRP1_PERIPH_SPI5:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI45_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_USART6:
			*rate = LL_RCC_GetUARTClockFreq(
					LL_RCC_USART6_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_SAI1:
			*rate = LL_RCC_GetSAIClockFreq(LL_RCC_SAI1_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_SAI2:
			*rate = LL_RCC_GetSAIClockFreq(LL_RCC_SAI2_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_SAI3:
			*rate = LL_RCC_GetSAIClockFreq(LL_RCC_SAI3_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_DFSDM1:
			*rate = LL_RCC_GetDFSDMClockFreq(
					LL_RCC_DFSDM_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_FDCAN:
			*rate = LL_RCC_GetFDCANClockFreq(
					LL_RCC_FDCAN_CLKSOURCE);
			break;
		case LL_APB2_GRP1_PERIPH_ADFSDM1:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB3:
		switch (pclken->enr) {
		case LL_APB3_GRP1_PERIPH_LPTIM2:
		case LL_APB3_GRP1_PERIPH_LPTIM3:
			*rate = LL_RCC_GetLPTIMClockFreq(
					LL_RCC_LPTIM23_CLKSOURCE);
			break;
		case LL_APB3_GRP1_PERIPH_LPTIM4:
		case LL_APB3_GRP1_PERIPH_LPTIM5:
			*rate = LL_RCC_GetLPTIMClockFreq(
					LL_RCC_LPTIM45_CLKSOURCE);
			break;
		case LL_APB3_GRP1_PERIPH_SAI4:
			*rate = LL_RCC_GetSAIClockFreq(LL_RCC_SAI4_CLKSOURCE);
			break;
		case LL_APB3_GRP1_PERIPH_SYSCFG:
		case LL_APB3_GRP1_PERIPH_VREF:
		case LL_APB3_GRP1_PERIPH_TMPSENS:
		case LL_APB3_GRP1_PERIPH_HDP:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB4:
		switch (pclken->enr) {
		case LL_APB4_GRP1_PERIPH_LTDC:
			*rate = LL_RCC_GetLTDCClockFreq();
			break;
		case LL_APB4_GRP1_PERIPH_DSI:
			*rate = LL_RCC_GetDSIClockFreq(LL_RCC_DSI_CLKSOURCE);
			break;
		case LL_APB4_GRP1_PERIPH_USBPHY:
			*rate = LL_RCC_GetUSBPHYClockFreq(
					LL_RCC_USBPHY_CLKSOURCE);
			break;
		case LL_APB4_GRP1_PERIPH_DDRPERFM:
		case LL_APB4_GRP1_PERIPH_STGENRO:
		case LL_APB4_GRP1_PERIPH_STGENROSTP:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_APB5:
		switch (pclken->enr) {
		case LL_APB5_GRP1_PERIPH_SPI6:
			*rate = LL_RCC_GetSPIClockFreq(LL_RCC_SPI6_CLKSOURCE);
			break;
		case LL_APB5_GRP1_PERIPH_I2C4:
		case LL_APB5_GRP1_PERIPH_I2C6:
			*rate = LL_RCC_GetI2CClockFreq(LL_RCC_I2C46_CLKSOURCE);
			break;
		case LL_APB5_GRP1_PERIPH_USART1:
			*rate = LL_RCC_GetUARTClockFreq(
					LL_RCC_USART1_CLKSOURCE);
			break;
		case LL_APB5_GRP1_PERIPH_STGEN:
		case LL_APB5_GRP1_PERIPH_STGENSTP:
			*rate = LL_RCC_GetSTGENClockFreq(
					LL_RCC_STGEN_CLKSOURCE);
			break;
		case LL_APB5_GRP1_PERIPH_RTCAPB:
			*rate = LL_RCC_GetRTCClockFreq();
			break;
		case LL_APB5_GRP1_PERIPH_TZC1:
		case LL_APB5_GRP1_PERIPH_TZC2:
		case LL_APB5_GRP1_PERIPH_TZPC:
		case LL_APB5_GRP1_PERIPH_BSEC:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_AHB2:
		switch (pclken->enr) {
		case LL_AHB2_GRP1_PERIPH_ADC12:
			*rate = LL_RCC_GetADCClockFreq(LL_RCC_ADC_CLKSOURCE);
			break;
		case LL_AHB2_GRP1_PERIPH_USBO:
			*rate = LL_RCC_GetUSBOClockFreq(LL_RCC_USBO_CLKSOURCE);
			break;
		case LL_AHB2_GRP1_PERIPH_SDMMC3:
			*rate = LL_RCC_GetSDMMCClockFreq(
					LL_RCC_SDMMC3_CLKSOURCE);
			break;
		case LL_AHB2_GRP1_PERIPH_DMA1:
		case LL_AHB2_GRP1_PERIPH_DMA2:
		case LL_AHB2_GRP1_PERIPH_DMAMUX:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_AHB3:
		switch (pclken->enr) {
		case LL_AHB3_GRP1_PERIPH_RNG2:
			*rate = LL_RCC_GetRNGClockFreq(LL_RCC_RNG2_CLKSOURCE);
			break;
		case LL_AHB3_GRP1_PERIPH_DCMI:
		case LL_AHB3_GRP1_PERIPH_CRYP2:
		case LL_AHB3_GRP1_PERIPH_HASH2:
		case LL_AHB3_GRP1_PERIPH_CRC2:
		case LL_AHB3_GRP1_PERIPH_HSEM:
		case LL_AHB3_GRP1_PERIPH_IPCC:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_AHB4:
		return -ENOTSUP;
	case STM32_CLOCK_BUS_AHB5:
		switch (pclken->enr) {
		case LL_AHB5_GRP1_PERIPH_RNG1:
			*rate = LL_RCC_GetRNGClockFreq(LL_RCC_RNG1_CLKSOURCE);
			break;
		case LL_AHB5_GRP1_PERIPH_GPIOZ:
		case LL_AHB5_GRP1_PERIPH_CRYP1:
		case LL_AHB5_GRP1_PERIPH_HASH1:
		case LL_AHB5_GRP1_PERIPH_BKPSRAM:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_AHB6:
		switch (pclken->enr) {
		case LL_AHB6_GRP1_PERIPH_ETH1CK:
		case LL_AHB6_GRP1_PERIPH_ETH1TX:
		case LL_AHB6_GRP1_PERIPH_ETH1RX:
		case LL_AHB6_GRP1_PERIPH_ETH1MAC:
		case LL_AHB6_GRP1_PERIPH_ETH1STP:
			*rate = LL_RCC_GetETHClockFreq(LL_RCC_ETH_CLKSOURCE);
			break;
		case LL_AHB6_GRP1_PERIPH_FMC:
			*rate = LL_RCC_GetFMCClockFreq(LL_RCC_FMC_CLKSOURCE);
			break;
		case LL_AHB6_GRP1_PERIPH_QSPI:
			*rate = LL_RCC_GetQSPIClockFreq(LL_RCC_QSPI_CLKSOURCE);
			break;
		case LL_AHB6_GRP1_PERIPH_SDMMC1:
		case LL_AHB6_GRP1_PERIPH_SDMMC2:
			*rate = LL_RCC_GetSDMMCClockFreq(
					LL_RCC_SDMMC12_CLKSOURCE);
			break;
		case LL_AHB6_GRP1_PERIPH_MDMA:
		case LL_AHB6_GRP1_PERIPH_GPU:
		case LL_AHB6_GRP1_PERIPH_CRC1:
		case LL_AHB6_GRP1_PERIPH_USBH:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_AXI:
		switch (pclken->enr) {
		case LL_AXI_GRP1_PERIPH_SYSRAMEN:
		default:
			return -ENOTSUP;
		}
		break;
	case STM32_CLOCK_BUS_MLAHB:
		switch (pclken->enr) {
		case LL_MLAHB_GRP1_PERIPH_RETRAMEN:
		default:
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static DEVICE_API(clock_control, stm32_clock_control_api) = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
};

static int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		    stm32_clock_control_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &stm32_clock_control_api);
