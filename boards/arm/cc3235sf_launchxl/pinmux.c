/*
 * Copyright (c) 2019, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * pinmux.c
 *
 * configure the device pins for different peripheral signals
 */

/*
 * This file was automatically generated on 7/21/2014 at 3:06:20 PM
 * by TI PinMux version 3.0.334
 * (Then modified to meet Zephyr coding style)
 */

/*
 * TI Recommends use of the PinMux utility to ensure consistent configuration
 * of pins: http://processors.wiki.ti.com/index.php/TI_PinMux_Tool
 *
 * Zephyr GPIO API however allows runtime configuration by applications.
 *
 * For the TI CC32XX port we leverage this output file
 * from the PinMux tool, and guard sections based on Kconfig variables.
 *
 * The individual (uart/gpio) driver init/configuration functions
 * therefore assume pinmux initialization is done here rather in the drivers
 * at runtime.
 */

#include <zephyr/init.h>

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_gpio.h>
#include <driverlib/pin.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/gpio.h>
#include <driverlib/prcm.h>
#include <driverlib/i2c.h>

/* Defines taken from SimpleLink SDK's I2CCC32XX.h: */
/*
 *  Macros defining possible I2C signal pin mux options
 *
 *  The bits in the pin mode macros are as follows:
 *  The lower 8 bits of the macro refer to the pin, offset by 1, to match
 *  driverlib pin defines.  For example, I2C_CC32XX_PIN_01_I2C_SCL & 0xff = 0,
 *  which equals PIN_01 in driverlib pin.h.  By matching the PIN_xx defines in
 *  driverlib pin.h, we can pass the pin directly to the driverlib functions.
 *  The upper 8 bits of the macro correspond to the pin mux config mode
 *  value for the pin to operate in the I2C mode.  For example, pin 1 is
 *  configured with mode 1 to operate as I2C_SCL.
 */
#define I2C_CC32XX_PIN_01_I2C_SCL  0x100  /*!< PIN 1 is used for I2C_SCL */
#define I2C_CC32XX_PIN_02_I2C_SDA  0x101  /*!< PIN 2 is used for I2C_SDA */
#define I2C_CC32XX_PIN_03_I2C_SCL  0x502  /*!< PIN 3 is used for I2C_SCL */
#define I2C_CC32XX_PIN_04_I2C_SDA  0x503  /*!< PIN 4 is used for I2C_SDA */
#define I2C_CC32XX_PIN_05_I2C_SCL  0x504  /*!< PIN 5 is used for I2C_SCL */
#define I2C_CC32XX_PIN_06_I2C_SDA  0x505  /*!< PIN 6 is used for I2C_SDA */
#define I2C_CC32XX_PIN_16_I2C_SCL  0x90F  /*!< PIN 16 is used for I2C_SCL */
#define I2C_CC32XX_PIN_17_I2C_SDA  0x910  /*!< PIN 17 is used for I2C_SDA */

int pinmux_initialize(void)
{

#ifdef CONFIG_UART_CC32XX
	/* Configure PIN_55 for UART0 UART0_TX */
	MAP_PinTypeUART(PIN_55, PIN_MODE_3);

	/* Configure PIN_57 for UART0 UART0_RX */
	MAP_PinTypeUART(PIN_57, PIN_MODE_3);
#endif

	/* Enable Peripheral Clocks */
	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);

	/* The following enables the 3 LEDs for the blinking samples */

	/* Configure PIN_64 for GPIOOutput */
	MAP_PinTypeGPIO(PIN_64, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA1_BASE, 0x2, GPIO_DIR_MODE_OUT);

	/* Configure PIN_01 for GPIOOutput */
	MAP_PinTypeGPIO(PIN_01, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA1_BASE, 0x4, GPIO_DIR_MODE_OUT);

	/* Configure PIN_02 for GPIOOutput */
	MAP_PinTypeGPIO(PIN_02, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA1_BASE, 0x8, GPIO_DIR_MODE_OUT);

	/* SW3: Configure PIN_04 (GPIO13) for GPIOInput */
	MAP_PinTypeGPIO(PIN_04, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA1_BASE, 0x20, GPIO_DIR_MODE_IN);

	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);

	/* SW2: Configure PIN_15 (GPIO22) for GPIOInput */
	MAP_PinTypeGPIO(PIN_15, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA2_BASE, 0x40, GPIO_DIR_MODE_IN);

	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK);


#ifdef CONFIG_I2C_CC32XX
	{
		unsigned long pin;
		unsigned long mode;

		/* Enable the I2C module clocks and wait for completion:*/
		MAP_PRCMPeripheralClkEnable(PRCM_I2CA0,
					    PRCM_RUN_MODE_CLK |
					    PRCM_SLP_MODE_CLK);
		while (!MAP_PRCMPeripheralStatusGet(PRCM_I2CA0)) {
		}

		pin = I2C_CC32XX_PIN_01_I2C_SCL & 0xff;
		mode = (I2C_CC32XX_PIN_01_I2C_SCL >> 8) & 0xff;
		MAP_PinTypeI2C(pin, mode);

		pin = I2C_CC32XX_PIN_02_I2C_SDA & 0xff;
		mode = (I2C_CC32XX_PIN_02_I2C_SDA >> 8) & 0xff;
		MAP_PinTypeI2C(pin, mode);
	}
#endif

	return 0;
}

SYS_INIT(pinmux_initialize, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
