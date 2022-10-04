/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include "codec.h"
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>

#if DT_ON_BUS(WM8731_NODE, i2c)

#define WM8731_I2C_NODE DT_BUS(WM8731_NODE)
#define WM8731_I2C_ADDR DT_REG_ADDR(WM8731_NODE)

bool init_wm8731_i2c(void)
{
	const struct device *i2c_dev = DEVICE_DT_GET(WM8731_I2C_NODE);

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
