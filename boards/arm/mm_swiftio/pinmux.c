/*
 * Copyright (c) 2019, MADMACHINE LIMITED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>
#include <fsl_gpio.h>
#include <soc.h>

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay)
static gpio_pin_config_t enet_gpio_config = {
	.direction = kGPIO_DigitalOutput,
	.outputLogic = 0,
	.interruptMode = kGPIO_NoIntmode
};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_ACCESS_USDHC1

/*Drive Strength Field: R0(260 Ohm @ 3.3V, 150 Ohm@1.8V, 240 Ohm for DDR)
 * Speed Field: medium(100MHz)
 * Open Drain Enable Field: Open Drain Disabled
 * Pull / Keep Enable Field: Pull/Keeper Enabled
 * Pull / Keep Select Field: Pull
 * Pull Up / Down Config. Field: 47K Ohm Pull Up
 * Hyst. Enable Field: Hysteresis Enabled.
 */

static void mm_swiftio_usdhc_pinmux(
	uint16_t nusdhc, bool init,
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

	if (nusdhc == 0) {
		if (init) {
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_AD_B0_05_GPIO1_IO05,
				0U);
			IOMUXC_SetPinMux(/*SD_CD*/
				IOMUXC_GPIO_B1_12_GPIO2_IO28,
				0U);
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_B1_14_USDHC1_VSELECT,
				0U);
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_SD_B0_00_USDHC1_CMD,
				0U);
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_SD_B0_01_USDHC1_CLK,
				0U);
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_SD_B0_02_USDHC1_DATA0,
				0U);
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_SD_B0_03_USDHC1_DATA1,
				0U);
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_SD_B0_04_USDHC1_DATA2,
				0U);
			IOMUXC_SetPinMux(
				IOMUXC_GPIO_SD_B0_05_USDHC1_DATA3,
				0U);

			IOMUXC_SetPinConfig(
				IOMUXC_GPIO_AD_B0_05_GPIO1_IO05,
				0x10B0u);
			IOMUXC_SetPinConfig(/*SD0_CD_SW*/
				IOMUXC_GPIO_B1_12_GPIO2_IO28,
				0x017089u);
			IOMUXC_SetPinConfig(/*SD0_VSELECT*/
				IOMUXC_GPIO_B1_14_USDHC1_VSELECT,
				0x0170A1u);
		}

		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_00_USDHC1_CMD,
				    cmd_data);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_01_USDHC1_CLK,
				    clk);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_02_USDHC1_DATA0,
				    cmd_data);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_03_USDHC1_DATA1,
				    cmd_data);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA2,
				    cmd_data);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA3,
				    cmd_data);
	}
}
#endif

static int mm_swiftio_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CLOCK_EnableClock(kCLOCK_Iomuxc);
	CLOCK_EnableClock(kCLOCK_IomuxcSnvs);


	/* LED */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_10_GPIO1_IO10, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_10_GPIO1_IO10,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_11_GPIO1_IO11, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_11_GPIO1_IO11,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));


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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c1), okay) && CONFIG_I2C
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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c3), okay) && CONFIG_I2C
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_07_LPI2C3_SCL, 1);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_06_LPI2C3_SDA, 1);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_07_LPI2C3_SCL,
			    IOMUXC_SW_PAD_CTL_PAD_PUS(3) |
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_ODE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_06_LPI2C3_SDA,
			    IOMUXC_SW_PAD_CTL_PAD_PUS(3) |
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_ODE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_DISK_ACCESS_USDHC1
	mm_swiftio_usdhc_pinmux(0, true, 2, 1);
	imxrt_usdhc_pinmux_cb_register(mm_swiftio_usdhc_pinmux);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(csi), okay) && CONFIG_VIDEO
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_04_CSI_PIXCLK,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_05_CSI_MCLK,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B0_14_CSI_VSYNC,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B0_15_CSI_HSYNC,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_08_CSI_DATA09,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_09_CSI_DATA08,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_10_CSI_DATA07,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_11_CSI_DATA06,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_12_CSI_DATA05,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_13_CSI_DATA04,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_14_CSI_DATA03,
		0U);
	IOMUXC_SetPinMux(
		IOMUXC_GPIO_AD_B1_15_CSI_DATA02,
		0U);
#endif
	return 0;
}

SYS_INIT(mm_swiftio_init, PRE_KERNEL_1, 0);
