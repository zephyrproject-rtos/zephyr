/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "codec.h"
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

#if DT_ON_BUS(WM8731_NODE, i2c)

#define WM8731_I2C_NODE DT_BUS(WM8731_NODE)
#define WM8731_I2C_ADDR DT_REG_ADDR(WM8731_NODE)

bool init_wm8731_i2c(void)
{
	const struct device *const i2c_dev = DEVICE_DT_GET(WM8731_I2C_NODE);

	/* Initialization data for WM8731 registers. */
	static const uint8_t init[][2] = {
		/*
		 * Reset Register:
		 * [8:0] RESET = 0 (reset device)
		 */
		{ 0x1E, 0x00 },
		/*
		 * Power Down Control:
		 *   [7] POWEROFF = 0 (Disable POWEROFF)
		 *   [6] CLKOUTPD = 1 (Enable Power Down)
		 *   [5] OSCPDD = 0 (Disable Power Down)
		 *   [4] OUTPD = 1 (Enable Power Down)
		 *   [3] DACPD = 0 (Disable Power Down)
		 *   [2] ADCPD = 0 (Disable Power Down)
		 *   [1] MICPD = 1 (Enable Power Down)
		 *   [0] LINEINPD = 0 (Disable Power Down)
		 */
		{ 0x0C, 0x52 },
		/*
		 * Left Line In:
		 *   [8] LRINBOTH = 1 (Enable Simultaneous Load)
		 *   [7] LINMUTE = 0 (Disable Mute)
		 * [4:0] LINVOL = 0x07 (-24 dB)
		 */
		{ 0x01, 0x07 },
		/*
		 * Left Headphone Out:
		 *   [8] LRHPBOTH = 1 (Enable Simultaneous Load)
		 *   [7] LZCEN = 0 (Disable)
		 * [6:0] LHPVOL = 0x79 (0 dB)
		 */
		{ 0x05, 0x79 },
		/*
		 * Analogue Audio Path Control:
		 * [7:6] SIDEATT = 0 (-6 dB)
		 *   [5] SIDETONE = 0 (Disable Side Tone)
		 *   [4] DACSEL = 1 (Select DAC)
		 *   [3] BYPASS = 0 (Disable Bypass)
		 *   [2] INSEL = 0 (Line Input Select to ADC)
		 *   [1] MUTEMIC = 1 (Enable Mute)
		 *   [0] MICBOOST = 0 (Disable Boost)
		 */
		{ 0x08, 0x12 },
		/*
		 * Digital Audio Path Control:
		 *   [4] HPOR = 0 (clear offset)
		 *   [3] DACMU = 0 (Disable soft mute)
		 * [2:1] DEEMP = 0 (Disable)
		 *   [0] ADCHPD = 1 (Disable High Pass Filter)
		 */
		{ 0x0A, 0x01 },
		/*
		 * Digital Audio Interface Format:
		 *   [7] BCLKINV = 0 (Don't invert BCLK)
		 *   [6] MS = 0 (Enable Slave Mode)
		 *   [5] LRSWAP = 0 (Right Channel DAC Data Right)
		 *   [4] LRP = 1 (Right Channel DAC data when DACLRC high)
		 * [3:2] IWL = 0 (16 bits)
		 * [1:0] FORMAT = 2 (I2S Format)
		 */
		{ 0x0E, 0x12 },
		/*
		 * Sampling Control:
		 *   [7] CLKODIV2 = 0 (CLOCKOUT is Core Clock)
		 *   [6] CLKIDIV2 = 0 (Core Clock is MCLK)
		 * [5:2] SR = 0x8 (44.1 kHz)
		 *   [1] BOSR = 0 (256fs)
		 *   [0] USB/NORMAL = 0 (Normal mode)
		 */
		{ 0x10, 0x20 },
		/*
		 * Active Control:
		 * [0] ACTIVE = 1 (Active)
		 */
		{ 0x12, 0x01 },
		/*
		 * As recommended in WAN_0111, set the OUTPD bit in the Power
		 * Down Control register to 0 at the very end of the power-on
		 * sequence.
		 */
		{ 0x0C, 0x42 }
	};

	if (!device_is_ready(i2c_dev)) {
		printk("%s is not ready\n", i2c_dev->name);
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(init); ++i) {
		const uint8_t *entry = init[i];
		int ret;

		ret = i2c_reg_write_byte(i2c_dev, WM8731_I2C_ADDR,
					 entry[0], entry[1]);
		if (ret < 0) {
			printk("Initialization step %d failed\n", i);
			return false;
		}
	}

	return true;
}

#endif /* DT_ON_BUS(WM8731_NODE, i2c) */

#if DT_ON_BUS(MAX9867_NODE, i2c)

#define MAX9867_I2C_NODE             DT_BUS(MAX9867_NODE)
#define MAX9867_I2C_ADDR             DT_REG_ADDR(MAX9867_NODE)
/* Register addresses */
#define MAX9867_00_STATUS            0x00
#define MAX9867_01_JACKSENSE         0x01
#define MAX9867_02_AUX_HIGH          0x02
#define MAX9867_03_AUX_LOW           0x03
#define MAX9867_04_INT_EN            0x04
#define MAX9867_05_SYS_CLK           0x05
#define MAX9867_06_CLK_HIGH          0x06
#define MAX9867_07_CLK_LOW           0x07
#define MAX9867_08_DAI_FORMAT        0x08
#define MAX9867_09_DAI_CLOCK         0x09
#define MAX9867_0A_DIG_FILTER        0x0A
#define MAX9867_0B_SIDETONE          0x0B
#define MAX9867_0C_LVL_DAC           0x0C
#define MAX9867_0D_LVL_ADC           0x0D
#define MAX9867_0E_LVL_LINE_IN_LEFT  0x0E
#define MAX9867_0F_LVL_LINE_IN_RIGHT 0x0F
#define MAX9867_10_VOL_LEFT          0x10
#define MAX9867_11_VOL_RIGHT         0x11
#define MAX9867_12_MIC_GAIN_LEFT     0x12
#define MAX9867_13_MIC_GAIN_RIGHT    0x13
#define MAX9867_14_ADC_INPUT         0x14
#define MAX9867_15_MIC               0x15
#define MAX9867_16_MODE              0x16
#define MAX9867_17_PWR_SYS           0x17
#define MAX9867_FF_REV_ID            0xFF

/* MAX9867_04_INT_EN */
#define MAX9867_ICLD   (1 << 7)
#define MAX9867_SDODLY (1 << 2)

/* MAX9867_05_SYS_CLK */
#define MAX9867_PSCLK_POS 4

/* MAX9867_06_CLK_HIGH */
#define MAX9867_PLL              (1 << 7)
#define MAX9867_NI_UPPER_8KHZ    0x10
#define MAX9867_NI_UPPER_16KHZ   0x20
#define MAX9867_NI_UPPER_24KHZ   0x30
#define MAX9867_NI_UPPER_32KHZ   0x40
#define MAX9867_NI_UPPER_44p1KHZ 0x58
#define MAX9867_NI_UPPER_48KHZ   0x60

/* MAX9867_07_CLK_LOW */
#define MAX9867_NI_LOWER_OTHER   0x00
#define MAX9867_NI_LOWER_44p1KHZ 0x33

/* MAX9867_08_DAI_FORMAT */
#define MAX9867_MAS    (1 << 7)
#define MAX9867_WCI    (1 << 6)
#define MAX9867_BCI    (1 << 5)
#define MAX9867_DLY    (1 << 4)
#define MAX9867_HIZOFF (1 << 3)
#define MAX9867_TDM    (1 << 2)

/* MAX9867_09_DAI_CLOCK */
#define MAX9867_BSEL_PCLK_DIV8 0x06

/* MAX9867_0D_LVL_ADC */
#define MAX9867_AVL_POS 4
#define MAX9867_AVR_POS 0

/*
 * MAX9867_0E_LVL_LINE_IN_LEFT
 * MAX9867_0F_LVL_LINE_IN_RIGHT
 */
#define MAX9867_LI_MUTE     (1 << 6)
#define MAX9867_LI_GAIN_POS 0

/*
 * MAX9867_10_VOL_LEFT
 * MAX9867_11_VOL_RIGHT
 */
#define MAX9867_VOL_POS 0

/* MAX9867_14_ADC_INPUT */
#define MAX9867_MXINL_POS       6
#define MAX9867_MXINR_POS       4
#define MAX9867_MXIN_DIS        0
#define MAX9867_MXIN_ANALOG_MIC 1
#define MAX9867_MXIN_LINE       2

/* MAX9867_15_MIC */
#define MAX9867_MICCLK_POS  6
#define MAX9867_DIGMICL_POS 5
#define MAX9867_DIGMICR_POS 4

/* MAX9867_16_MODE */
#define MAX9867_HPMODE_POS          0
#define MAX9867_STEREO_SE_CLICKLESS 4
#define MAX9867_MONO_SE_CLICKLESS   5

/* MAX9867_17_PWR_SYS */
#define MAX9867_SHDN  (1 << 7)
#define MAX9867_LNLEN (1 << 6)
#define MAX9867_LNREN (1 << 5)
#define MAX9867_DALEN (1 << 3)
#define MAX9867_DAREN (1 << 2)
#define MAX9867_ADLEN (1 << 1)
#define MAX9867_ADREN (1 << 0)

bool init_max9867_i2c(void)
{
	const struct device *const i2c_dev = DEVICE_DT_GET(MAX9867_I2C_NODE);

	/* Initialization data for MAX9867 registers. */
	static const uint8_t init[][2] = {
		/* Shutdown MAX9867 during configuration */
		{MAX9867_17_PWR_SYS, 0x00},
		/*
		 * Clear all regs to POR state. The MAX9867 does not not have an external
		 * reset signal. So manually writing 0, from (0x04 - 0x17)
		 */
		{MAX9867_04_INT_EN, 0x00},
		{MAX9867_05_SYS_CLK, 0x00},
		{MAX9867_06_CLK_HIGH, 0x00},
		{MAX9867_07_CLK_LOW, 0x00},
		{MAX9867_08_DAI_FORMAT, 0x00},
		{MAX9867_09_DAI_CLOCK, 0x00},
		{MAX9867_0A_DIG_FILTER, 0x00},
		{MAX9867_0B_SIDETONE, 0x00},
		{MAX9867_0C_LVL_DAC, 0x00},
		{MAX9867_0D_LVL_ADC, 0x00},
		{MAX9867_0E_LVL_LINE_IN_LEFT, 0x00},
		{MAX9867_0F_LVL_LINE_IN_RIGHT, 0x00},
		{MAX9867_10_VOL_LEFT, 0x00},
		{MAX9867_11_VOL_RIGHT, 0x00},
		{MAX9867_12_MIC_GAIN_LEFT, 0x00},
		{MAX9867_13_MIC_GAIN_RIGHT, 0x00},
		{MAX9867_14_ADC_INPUT, 0x00},
		{MAX9867_15_MIC, 0x00},
		{MAX9867_16_MODE, 0x00},
		{MAX9867_17_PWR_SYS, 0x00},
		/*
		 * Select MCLK prescaler. PSCLK divides MCLK to generate a PCLK between 10MHz and
		 * 20MHz. Set prescaler, FREQ field is 0 for Normal or PLL mode, < 20MHz.
		 */
		{MAX9867_05_SYS_CLK, 0x01 << MAX9867_PSCLK_POS},
		/* Configure codec to generate 48kHz sampling frequency in controller mode */
		{MAX9867_06_CLK_HIGH, MAX9867_NI_UPPER_44p1KHZ},
		{MAX9867_07_CLK_LOW, MAX9867_NI_LOWER_44p1KHZ},
		{MAX9867_09_DAI_CLOCK, MAX9867_BSEL_PCLK_DIV8},
		/* I2S format */
		{MAX9867_08_DAI_FORMAT, MAX9867_MAS | MAX9867_DLY | MAX9867_HIZOFF},
		/* */
		{MAX9867_0A_DIG_FILTER, 0xA2},
		/* Select Digital microphone input */
		{MAX9867_15_MIC, ((0x1 << MAX9867_DIGMICR_POS))},
		/* ADC level */
		{MAX9867_0D_LVL_ADC, (3 << MAX9867_AVL_POS) | (3 << MAX9867_AVR_POS)},
		/*Set line-in level, disconnect line input from playback amplifiers */
		{MAX9867_0E_LVL_LINE_IN_LEFT, (0x0C << MAX9867_LI_GAIN_POS) | MAX9867_LI_MUTE},
		{MAX9867_0F_LVL_LINE_IN_RIGHT, (0x0C << MAX9867_LI_GAIN_POS) | MAX9867_LI_MUTE},
		/* Headphone mode */
		{MAX9867_16_MODE, MAX9867_STEREO_SE_CLICKLESS << MAX9867_HPMODE_POS},
		/* Set playback volume */
		{MAX9867_10_VOL_LEFT, 0x04 << MAX9867_VOL_POS},
		{MAX9867_11_VOL_RIGHT, 0x04 << MAX9867_VOL_POS},
		/* Enable */
		{MAX9867_17_PWR_SYS, MAX9867_SHDN | MAX9867_DALEN | MAX9867_DAREN | MAX9867_ADLEN},
	};

	if (!device_is_ready(i2c_dev)) {
		printk("%s is not ready\n", i2c_dev->name);
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(init); ++i) {
		const uint8_t *entry = init[i];
		int ret;

		ret = i2c_reg_write_byte(i2c_dev, MAX9867_I2C_ADDR, entry[0], entry[1]);
		if (ret < 0) {
			printk("Initialization step %d failed with %d\n", i, ret);
			return false;
		}
	}

	return true;
}

#endif /* DT_ON_BUS(MAX9867_NODE, i2c) */
