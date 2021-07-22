/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>
#include <fsl_gpio.h>
#include <soc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(mimxrt1170_evk, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
static gpio_pin_config_t enet_gpio_config = {
	.direction = kGPIO_DigitalOutput,
	.outputLogic = 0,
	.interruptMode = kGPIO_NoIntmode
};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(csi), okay) && CONFIG_VIDEO
static  gpio_pin_config_t cam_rst_config = {
	.direction = kGPIO_DigitalOutput,
	.outputLogic = 0U,
	.interruptMode = kGPIO_NoIntmode
};

static gpio_pin_config_t cam_pwdn_config = {
	.direction = kGPIO_DigitalOutput,
	.outputLogic = 0U,
	.interruptMode = kGPIO_NoIntmode
};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_DRIVER_SDMMC

/*Drive Strength Field: R0(260 Ohm @ 3.3V, 150 Ohm@1.8V, 240 Ohm for DDR)
 *Speed Field: medium(100MHz)
 *Open Drain Enable Field: Open Drain Disabled
 *Pull / Keep Enable Field: Pull/Keeper Enabled
 *Pull / Keep Select Field: Pull
 *Pull Up / Down Config. Field: 47K Ohm Pull Up
 *Hyst. Enable Field: Hysteresis Enabled.
 */

static void mimxrt1170_evk_usdhc_pinmux(uint16_t nusdhc, bool init,
		uint32_t speed, uint32_t strength)
{
	uint32_t cmd_data = IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) |
			 IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_HYS_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_PUS(1) |
			 IOMUXC_SW_PAD_CTL_PAD_DSE(strength);

	uint32_t clk = IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) |
			IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
			IOMUXC_SW_PAD_CTL_PAD_HYS_MASK |
			IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
			IOMUXC_SW_PAD_CTL_PAD_DSE(strength);

	if (nusdhc != 0) {
		LOG_ERR("Invalid USDHC index");
		return;
	}

	if (init) {
		IOMUXC_SetPinMux(IOMUXC_GPIO_AD_32_GPIO_MUX3_IO31, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_AD_34_USDHC1_VSELECT, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_AD_35_GPIO10_IO02, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_00_USDHC1_CMD, 1U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_01_USDHC1_CLK, 1U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_02_USDHC1_DATA0, 1U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_03_USDHC1_DATA1, 1U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_04_USDHC1_DATA2, 1U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_05_USDHC1_DATA3, 1U);
		IOMUXC_GPR->GPR43 = ((IOMUXC_GPR->GPR43
			& (~(IOMUXC_GPR_GPR43_GPIO_MUX3_GPIO_SEL_HIGH_MASK)))
			| IOMUXC_GPR_GPR43_GPIO_MUX3_GPIO_SEL_HIGH(0x8000U));
	}
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_00_USDHC1_CMD, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_01_USDHC1_CLK, clk);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_02_USDHC1_DATA0, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_03_USDHC1_DATA1, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_04_USDHC1_DATA2, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_05_USDHC1_DATA3, cmd_data);
}
#endif

static int mimxrt1170_evk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CLOCK_EnableClock(kCLOCK_Iomuxc);

	/* USER_LED_CTRL1 */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_04_GPIO9_IO03, 0U);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart1), okay) && CONFIG_SERIAL
	/* LPUART1 TX/RX */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_24_LPUART1_TXD, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_25_LPUART1_RXD, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_24_LPUART1_TXD, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_25_LPUART1_RXD, 0x02U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lcdif), okay) && CONFIG_DISPLAY
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_02_GPIO9_IO01, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_30_GPIO9_IO29, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_15_GPIO11_IO16, 0U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c1), okay) && CONFIG_I2C
	/* LPI2C1 SCL, SDA */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_08_LPI2C1_SCL, 1U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_09_LPI2C1_SDA, 1U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_08_LPI2C1_SCL, 0x10U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_09_LPI2C1_SDA, 0x10U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi1), okay) && CONFIG_SPI
	/* LPIPI1 SCK, PCS0, SIN, SOUT */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_28_LPSPI1_SCK, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_29_LPSPI1_PCS0, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_30_LPSPI1_SOUT, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_31_LPSPI1_SIN, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_28_LPSPI1_SCK, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_29_LPSPI1_PCS0, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_30_LPSPI1_SOUT, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_31_LPSPI1_SIN, 0x02U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_32_ENET_MDC, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_33_ENET_MDIO, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_02_ENET_TX_DATA00, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_03_ENET_TX_DATA01, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_04_ENET_TX_EN, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_05_ENET_REF_CLK1, 1U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_06_ENET_RX_DATA00, 1U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_07_ENET_RX_DATA01, 1U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_08_ENET_RX_EN, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_09_ENET_RX_ER, 0U);
	IOMUXC_GPR->GPR4 = ((IOMUXC_GPR->GPR4 &
		(~(IOMUXC_GPR_GPR4_ENET_REF_CLK_DIR_MASK))) |
			IOMUXC_GPR_GPR4_ENET_REF_CLK_DIR(0x01U));
	IOMUXC_SetPinMux(IOMUXC_GPIO_LPSR_12_GPIO12_IO12, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_12_GPIO9_IO11, 0x06U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_02_ENET_TX_DATA00, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_03_ENET_TX_DATA01, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_04_ENET_TX_EN, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_05_ENET_REF_CLK1, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_06_ENET_RX_DATA00, 0x06U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_07_ENET_RX_DATA01, 0x06U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_08_ENET_RX_EN, 0x06U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_DISP_B2_09_ENET_RX_ER, 0x06U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_LPSR_12_GPIO12_IO12, 0x0EU);

	/* Initialize ENET_INT GPIO */
	GPIO_PinInit(GPIO9, 11, &enet_gpio_config);
	GPIO_PinInit(GPIO12, 12, &enet_gpio_config);

	/* pull up the ENET_INT before RESET. */
	GPIO_WritePinOutput(GPIO1, 11, 1);
	GPIO_WritePinOutput(GPIO1, 12, 0);

#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexpwm1_pwm2), okay) && CONFIG_PWM
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_04_FLEXPWM1_PWM2_A, 0U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(csi), okay) && CONFIG_VIDEO
	/* Initialize GPIO functionality on GPIO_AD_26 (pin L14) */
	GPIO_PinInit(GPIO9, 25U, &cam_pwdn_config);
	/* Initialize GPIO functionality on GPIO_DISP_B2_14 (pin A7) */
	GPIO_PinInit(GPIO11, 15U, &cam_rst_config);

	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_26_GPIO9_IO25, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_DISP_B2_14_GPIO11_IO15, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_LPSR_06_LPI2C6_SDA, 1U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_LPSR_07_LPI2C6_SCL, 1U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcan3), okay) && CONFIG_CAN
	IOMUXC_SetPinMux(IOMUXC_GPIO_LPSR_00_FLEXCAN3_TX, 1U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_LPSR_01_FLEXCAN3_RX, 1U);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_LPSR_00_FLEXCAN3_TX, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_LPSR_01_FLEXCAN3_RX, 0x02U);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_DRIVER_SDMMC
	mimxrt1170_evk_usdhc_pinmux(0, true, 2, 1);
	imxrt_usdhc_pinmux_cb_register(mimxrt1170_evk_usdhc_pinmux);
#endif

	return 0;
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
static int mimxrt1170_evk_phy_reset(const struct device *dev)
{
	/* RESET PHY chip. */
	k_busy_wait(USEC_PER_MSEC * 10U);
	GPIO_WritePinOutput(GPIO12, 12, 1);

	return 0;
}
#endif

SYS_INIT(mimxrt1170_evk_init, PRE_KERNEL_1, 0);
#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
SYS_INIT(mimxrt1170_evk_phy_reset, PRE_KERNEL_2, 0);
#endif
