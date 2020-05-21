/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>
#include <fsl_gpio.h>
#include <soc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(mimxrt1060_evk, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
static gpio_pin_config_t enet_gpio_config = {
	.direction = kGPIO_DigitalOutput,
	.outputLogic = 0,
	.interruptMode = kGPIO_NoIntmode
};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_ACCESS_USDHC1

/*Drive Strength Field: R0(260 Ohm @ 3.3V, 150 Ohm@1.8V, 240 Ohm for DDR)
 *Speed Field: medium(100MHz)
 *Open Drain Enable Field: Open Drain Disabled
 *Pull / Keep Enable Field: Pull/Keeper Enabled
 *Pull / Keep Select Field: Pull
 *Pull Up / Down Config. Field: 47K Ohm Pull Up
 *Hyst. Enable Field: Hysteresis Enabled.
 */

static void mimxrt1060_evk_usdhc_pinmux(u16_t nusdhc, bool init, u32_t speed,
					u32_t strength)
{
	u32_t cmd_data = IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) |
			 IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_HYS_MASK |
			 IOMUXC_SW_PAD_CTL_PAD_PUS(1) |
			 IOMUXC_SW_PAD_CTL_PAD_DSE(strength);

	u32_t clk = IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) |
		    IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
		    IOMUXC_SW_PAD_CTL_PAD_HYS_MASK |
		    IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
		    IOMUXC_SW_PAD_CTL_PAD_DSE(strength);

	if (nusdhc != 0) {
		LOG_ERR("Invalid USDHC index");
		return;
	}

	if (init) {
		IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_05_GPIO1_IO05, 0U);

		/* SD_CD */
		IOMUXC_SetPinMux(IOMUXC_GPIO_B1_12_GPIO2_IO28, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_B1_14_USDHC1_VSELECT, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_00_USDHC1_CMD, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_01_USDHC1_CLK, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_02_USDHC1_DATA0, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_03_USDHC1_DATA1, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA2, 0U);
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA3, 0U);

		IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_05_GPIO1_IO05, 0x10B0u);

		/* SD0_CD_SW */
		IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_12_GPIO2_IO28, 0x017089u);

		/* SD0_VSELECT */
		IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_14_USDHC1_VSELECT,
				    0x0170A1u);
	}

	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_00_USDHC1_CMD, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_01_USDHC1_CLK, clk);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_02_USDHC1_DATA0, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_03_USDHC1_DATA1, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA2, cmd_data);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA3, cmd_data);
}
#endif

static int mimxrt1060_evk_init(struct device *dev)
{
	ARG_UNUSED(dev);

	CLOCK_EnableClock(kCLOCK_Iomuxc);
	CLOCK_EnableClock(kCLOCK_IomuxcSnvs);

#if DT_NODE_HAS_PROP(DT_INST(0, focaltech_ft5336), int_gpios)
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_11_GPIO1_IO11, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_11_GPIO1_IO11,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));
#endif

#if !DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay)
	/* LED */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	/* SW0 */
	IOMUXC_SetPinMux(IOMUXC_SNVS_WAKEUP_GPIO5_IO00, 0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart1), okay) && CONFIG_SERIAL
	/* LPUART1 TX/RX */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_12_LPUART1_TX, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_13_LPUART1_RX, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_12_LPUART1_TX,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_13_LPUART1_RX,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart3), okay) && CONFIG_SERIAL
	/* LPUART3 TX/RX */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_06_LPUART3_TX, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_07_LPUART3_RX, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_06_LPUART3_TX,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_07_LPUART3_RX,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c1), okay) && CONFIG_I2C
	/* LPI2C1 SCL, SDA */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_00_LPI2C1_SCL, 1);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_01_LPI2C1_SDA, 1);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_00_LPI2C1_SCL,
			    IOMUXC_SW_PAD_CTL_PAD_PUS(3) |
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_ODE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_01_LPI2C1_SDA,
			    IOMUXC_SW_PAD_CTL_PAD_PUS(3) |
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_ODE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_10_GPIO1_IO10, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_04_ENET_RX_DATA00, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_05_ENET_RX_DATA01, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_06_ENET_RX_EN, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_07_ENET_TX_DATA00, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_08_ENET_TX_DATA01, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_09_ENET_TX_EN, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_10_ENET_REF_CLK, 1);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_11_ENET_RX_ER, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_40_ENET_MDC, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_41_ENET_MDIO, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, 0xB0A9u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_10_GPIO1_IO10, 0xB0A9u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_04_ENET_RX_DATA00, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_05_ENET_RX_DATA01, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_06_ENET_RX_EN, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_07_ENET_TX_DATA00, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_08_ENET_TX_DATA01, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_09_ENET_TX_EN, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_10_ENET_REF_CLK, 0x31);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_11_ENET_RX_ER, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_40_ENET_MDC, 0xB0E9);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_41_ENET_MDIO, 0xB829);

	IOMUXC_EnableMode(IOMUXC_GPR, kIOMUXC_GPR_ENET1TxClkOutputDir, true);

	/* Initialize ENET_INT GPIO */
	GPIO_PinInit(GPIO1, 9, &enet_gpio_config);
	GPIO_PinInit(GPIO1, 10, &enet_gpio_config);

	/* pull up the ENET_INT before RESET. */
	GPIO_WritePinOutput(GPIO1, 10, 1);
	GPIO_WritePinOutput(GPIO1, 9, 0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lcdif), okay) && CONFIG_DISPLAY
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_00_LCD_CLK, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_01_LCD_ENABLE, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_02_LCD_HSYNC, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_03_LCD_VSYNC, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_04_LCD_DATA00, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_05_LCD_DATA01, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_06_LCD_DATA02, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_07_LCD_DATA03, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_08_LCD_DATA04, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_09_LCD_DATA05, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_10_LCD_DATA06, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_11_LCD_DATA07, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_12_LCD_DATA08, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_13_LCD_DATA09, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_14_LCD_DATA10, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_15_LCD_DATA11, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_00_LCD_DATA12, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_01_LCD_DATA13, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_02_LCD_DATA14, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_03_LCD_DATA15, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_00_LCD_CLK, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_01_LCD_ENABLE, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_02_LCD_HSYNC, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_03_LCD_VSYNC, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_04_LCD_DATA00, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_05_LCD_DATA01, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_06_LCD_DATA02, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_07_LCD_DATA03, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_08_LCD_DATA04, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_09_LCD_DATA05, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_10_LCD_DATA06, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_11_LCD_DATA07, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_12_LCD_DATA08, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_13_LCD_DATA09, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_14_LCD_DATA10, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_15_LCD_DATA11, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_00_LCD_DATA12, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_01_LCD_DATA13, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_02_LCD_DATA14, 0x01B0B0u);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_03_LCD_DATA15, 0x01B0B0u);

	/* LCD Reset */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_02_GPIO1_IO02, 0);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_02_GPIO1_IO02, 0x10B0u);

	/* LCD Backlight */
	IOMUXC_SetPinMux(IOMUXC_GPIO_B1_15_GPIO2_IO31, 0);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B1_15_GPIO2_IO31, 0x10B0u);

	gpio_pin_config_t config = {
		kGPIO_DigitalOutput, 0,
	};

	config.outputLogic = 1;
	GPIO_PinInit(GPIO2, 31, &config);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_ACCESS_USDHC1
	mimxrt1060_evk_usdhc_pinmux(0, true, 2, 1);
	imxrt_usdhc_pinmux_cb_register(mimxrt1060_evk_usdhc_pinmux);
#endif

	return 0;
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
static int mimxrt1060_evk_phy_reset(struct device *dev)
{
	/* RESET PHY chip. */
	k_busy_wait(USEC_PER_MSEC * 10U);
	GPIO_WritePinOutput(GPIO1, 9, 1);

	return 0;
}
#endif

SYS_INIT(mimxrt1060_evk_init, PRE_KERNEL_1, 0);
#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
SYS_INIT(mimxrt1060_evk_phy_reset, PRE_KERNEL_2, 0);
#endif
