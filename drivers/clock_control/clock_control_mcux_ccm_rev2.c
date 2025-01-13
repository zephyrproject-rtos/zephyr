/*
 * Copyright 2021,2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_ccm_rev2
#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

static int mcux_ccm_on(const struct device *dev,
				  clock_control_subsys_t sub_system)
{
	uint32_t clock_name = (uintptr_t)sub_system;
	uint32_t peripheral, instance;

	peripheral = (clock_name & IMX_CCM_PERIPHERAL_MASK);
	instance = (clock_name & IMX_CCM_INSTANCE_MASK);
	switch (peripheral) {
#ifdef CONFIG_ETH_NXP_ENET

#ifdef CONFIG_SOC_MIMX9352
#define ENET1G_CLOCK	kCLOCK_Enet1
#else
#define ENET_CLOCK	kCLOCK_Enet
#define ENET1G_CLOCK	kCLOCK_Enet_1g
#endif
#ifdef ENET_CLOCK
	case IMX_CCM_ENET_CLK:
		CLOCK_EnableClock(ENET_CLOCK);
		return 0;
#endif
	case IMX_CCM_ENET1G_CLK:
		CLOCK_EnableClock(ENET1G_CLOCK);
		return 0;
#endif
	default:
		(void)instance;
		return 0;
	}
}

static int mcux_ccm_off(const struct device *dev,
				   clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_ccm_get_subsys_rate(const struct device *dev,
					clock_control_subsys_t sub_system,
					uint32_t *rate)
{
	uint32_t clock_name = (size_t) sub_system;
	uint32_t clock_root, peripheral, instance;

	peripheral = (clock_name & IMX_CCM_PERIPHERAL_MASK);
	instance = (clock_name & IMX_CCM_INSTANCE_MASK);
	switch (peripheral) {
#ifdef CONFIG_I2C_MCUX_LPI2C
#if defined(CONFIG_SOC_SERIES_IMXRT118X)
	case IMX_CCM_LPI2C0102_CLK:
		clock_root = kCLOCK_Root_Lpi2c0102 + instance;
		break;
#else
	case IMX_CCM_LPI2C1_CLK:
		clock_root = kCLOCK_Root_Lpi2c1 + instance;
		break;
#endif
#endif

#ifdef CONFIG_I3C_MCUX
	case IMX_CCM_I3C1_CLK:
	case IMX_CCM_I3C2_CLK:
		clock_root = kCLOCK_Root_I3c1 + instance;
		break;
#endif

#ifdef CONFIG_SPI_MCUX_LPSPI
#if defined(CONFIG_SOC_SERIES_IMXRT118X)
	case IMX_CCM_LPSPI0102_CLK:
		clock_root = kCLOCK_Root_Lpspi0102 + instance;
		break;
#else
	case IMX_CCM_LPSPI1_CLK:
		clock_root = kCLOCK_Root_Lpspi1 + instance;
		break;
#endif /* CONFIG_SOC_SERIES_IMXRT118X */
#endif /* CONFIG_SPI_MCUX_LPSPI */

#ifdef CONFIG_UART_MCUX_LPUART
#if defined(CONFIG_SOC_SERIES_IMXRT118X)
	case IMX_CCM_LPUART0102_CLK:
	case IMX_CCM_LPUART0304_CLK:
		clock_root = kCLOCK_Root_Lpuart0102 + instance;
		break;
#else
	case IMX_CCM_LPUART1_CLK:
	case IMX_CCM_LPUART2_CLK:
		clock_root = kCLOCK_Root_Lpuart1 + instance;
		break;
#endif
#endif

#if CONFIG_IMX_USDHC
	case IMX_CCM_USDHC1_CLK:
	case IMX_CCM_USDHC2_CLK:
		clock_root = kCLOCK_Root_Usdhc1 + instance;
		break;
#endif

#ifdef CONFIG_DMA_MCUX_EDMA
	case IMX_CCM_EDMA_CLK:
		clock_root = kCLOCK_Root_Bus;
		break;
	case IMX_CCM_EDMA_LPSR_CLK:
		clock_root = kCLOCK_Root_Bus_Lpsr;
		break;
#endif

#ifdef CONFIG_DMA_MCUX_EDMA_V4
	case IMX_CCM_EDMA3_CLK:
		clock_root = kCLOCK_Root_M33;
		break;
	case IMX_CCM_EDMA4_CLK:
		clock_root = kCLOCK_Root_Wakeup_Axi;
		break;
#endif

#ifdef CONFIG_PWM_MCUX
#if defined(CONFIG_SOC_SERIES_IMXRT118X)
	case IMX_CCM_PWM_CLK:
		clock_root = kCLOCK_Root_Bus_Aon;
		break;
#else
	case IMX_CCM_PWM_CLK:
		clock_root = kCLOCK_Root_Bus;
		break;
#endif /* CONFIG_SOC_SERIES_IMXRT118X */
#endif /* CONFIG_PWM_MCUX */

#ifdef CONFIG_CAN_MCUX_FLEXCAN
	case IMX_CCM_CAN1_CLK:
		clock_root = kCLOCK_Root_Can1 + instance;
		break;
#endif

#ifdef CONFIG_COUNTER_MCUX_GPT
	case IMX_CCM_GPT_CLK:
		clock_root = kCLOCK_Root_Gpt1 + instance;
		break;
#endif

#ifdef CONFIG_I2S_MCUX_SAI
	case IMX_CCM_SAI1_CLK:
		clock_root =  kCLOCK_Root_Sai1;
		break;
	case IMX_CCM_SAI2_CLK:
		clock_root =  kCLOCK_Root_Sai2;
		break;
	case IMX_CCM_SAI3_CLK:
		clock_root =  kCLOCK_Root_Sai3;
		break;
	case IMX_CCM_SAI4_CLK:
		clock_root =  kCLOCK_Root_Sai4;
		break;
#endif

#ifdef CONFIG_ETH_NXP_ENET
	case IMX_CCM_ENET_CLK:
	case IMX_CCM_ENET1G_CLK:
#ifdef CONFIG_SOC_MIMX9352
		clock_root = kCLOCK_Root_WakeupAxi;
#else
		clock_root = kCLOCK_Root_Bus;
#endif
		break;
#endif

#if defined(CONFIG_SOC_MIMX9352) && defined(CONFIG_DAI_NXP_SAI)
	case IMX_CCM_SAI1_CLK:
	case IMX_CCM_SAI2_CLK:
	case IMX_CCM_SAI3_CLK:
		clock_root = kCLOCK_Root_Sai1 + instance;
		uint32_t mux = CLOCK_GetRootClockMux(clock_root);
		uint32_t divider = CLOCK_GetRootClockDiv(clock_root);

		/* assumption: SAI's SRC is AUDIO_PLL */
		if (mux != 1) {
			return -EINVAL;
		}

		/* assumption: AUDIO_PLL's frequency is 393216000 Hz */
		*rate = 393216000 / divider;

		return 0;
#endif
#if defined(CONFIG_COUNTER_MCUX_TPM) || defined(CONFIG_PWM_MCUX_TPM)
#if defined(CONFIG_SOC_SERIES_IMXRT118X)
	case IMX_CCM_TPM_CLK:
		switch (instance) {
		case 0:
			clock_root = kCLOCK_Root_Bus_Aon;
			break;
		case 1:
			clock_root = kCLOCK_Root_Tpm2;
			break;
		case 2:
			clock_root = kCLOCK_Root_Bus_Wakeup;
			break;
		default:
			clock_root = (kCLOCK_Root_Tpm4 + instance - 3);
		}
		break;
#else
	case IMX_CCM_TPM_CLK:
		clock_root = kCLOCK_Root_Tpm1 + instance;
		break;
#endif
#endif

#ifdef CONFIG_MCUX_FLEXIO
	case IMX_CCM_FLEXIO1_CLK:
		clock_root = kCLOCK_Root_Flexio1;
		break;
	case IMX_CCM_FLEXIO2_CLK:
		clock_root = kCLOCK_Root_Flexio2;
		break;
#endif

#if defined(CONFIG_PWM_MCUX_QTMR) || defined(CONFIG_COUNTER_MCUX_QTMR)
#if defined(CONFIG_SOC_SERIES_IMXRT118X)
	case IMX_CCM_QTMR_CLK:
		clock_root = kCLOCK_Root_Bus_Aon;
		break;
#else
	case IMX_CCM_QTMR1_CLK:
	case IMX_CCM_QTMR2_CLK:
	case IMX_CCM_QTMR3_CLK:
	case IMX_CCM_QTMR4_CLK:
		clock_root = kCLOCK_Root_Bus;
		break;
#endif /* CONFIG_SOC_SERIES_IMXRT118X */
#endif /* CONFIG_PWM_MCUX_QTMR || CONFIG_COUNTER_MCUX_QTMR */

#ifdef CONFIG_MEMC_MCUX_FLEXSPI
	case IMX_CCM_FLEXSPI_CLK:
	case IMX_CCM_FLEXSPI2_CLK:
		clock_root = kCLOCK_Root_Flexspi1 + instance;
		break;
#endif
#ifdef CONFIG_COUNTER_NXP_PIT
	case IMX_CCM_PIT_CLK:
		clock_root = kCLOCK_Root_Bus + instance;
		break;
#endif

#ifdef CONFIG_ADC_MCUX_LPADC
	case IMX_CCM_LPADC1_CLK:
		clock_root = kCLOCK_Root_Adc1 + instance;
		break;
#endif

#if defined(CONFIG_ETH_NXP_IMX_NETC)
	case IMX_CCM_NETC_CLK:
		clock_root = kCLOCK_Root_Netc;
		break;
#endif

#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
	case IMX_CCM_MIPI_CSI2RX_ROOT_CLK:
		clock_root = kCLOCK_Root_Csi2;
		break;
	case IMX_CCM_MIPI_CSI2RX_ESC_CLK:
		clock_root = kCLOCK_Root_Csi2_Esc;
		break;
	case IMX_CCM_MIPI_CSI2RX_UI_CLK:
		clock_root = kCLOCK_Root_Csi2_Ui;
		break;
#endif

	default:
		return -EINVAL;
	}
#if defined(CONFIG_SOC_MIMX9352) || defined(CONFIG_SOC_MIMX9131)
	*rate = CLOCK_GetIpFreq(clock_root);
#else
	*rate = CLOCK_GetRootClockFreq(clock_root);
#endif
	return 0;
}

/*
 * Since this function is used to reclock the FlexSPI when running in
 * XIP, it must be located in RAM when MEMC driver is enabled.
 */
#ifdef CONFIG_MEMC_MCUX_FLEXSPI
#define CCM_SET_FUNC_ATTR __ramfunc
#else
#define CCM_SET_FUNC_ATTR
#endif

static int CCM_SET_FUNC_ATTR mcux_ccm_set_subsys_rate(const struct device *dev,
			clock_control_subsys_t subsys,
			clock_control_subsys_rate_t rate)
{
	uint32_t clock_name = (uintptr_t)subsys;
	uint32_t clock_rate = (uintptr_t)rate;

	switch (clock_name) {
	case IMX_CCM_FLEXSPI_CLK:
		__fallthrough;
	case IMX_CCM_FLEXSPI2_CLK:
#if (defined(CONFIG_SOC_SERIES_IMXRT11XX) || defined(CONFIG_SOC_SERIES_IMXRT118X)) \
		&& defined(CONFIG_MEMC_MCUX_FLEXSPI)
		/* The SOC is using the FlexSPI for XIP. Therefore,
		 * the FlexSPI itself must be managed within the function,
		 * which is SOC specific.
		 */
		return flexspi_clock_set_freq(clock_name, clock_rate);
#endif

#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
	case IMX_CCM_MIPI_CSI2RX_ROOT_CLK:
		return mipi_csi2rx_clock_set_freq(kCLOCK_Root_Csi2, clock_rate);
	case IMX_CCM_MIPI_CSI2RX_UI_CLK:
		return mipi_csi2rx_clock_set_freq(kCLOCK_Root_Csi2_Ui, clock_rate);
	case IMX_CCM_MIPI_CSI2RX_ESC_CLK:
		return mipi_csi2rx_clock_set_freq(kCLOCK_Root_Csi2_Esc, clock_rate);
#endif

	default:
		/* Silence unused variable warning */
		ARG_UNUSED(clock_rate);
		return -ENOTSUP;
	}
}

static DEVICE_API(clock_control, mcux_ccm_driver_api) = {
	.on = mcux_ccm_on,
	.off = mcux_ccm_off,
	.get_rate = mcux_ccm_get_subsys_rate,
	.set_rate = mcux_ccm_set_subsys_rate,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &mcux_ccm_driver_api);
