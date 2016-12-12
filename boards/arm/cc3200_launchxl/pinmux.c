/*
 * pinmux.c
 *
 * configure the device pins for different peripheral signals
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <init.h>

#include "pinmux.h"

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_gpio.h>
#include <driverlib/pin.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/gpio.h>
#include <driverlib/prcm.h>

int pinmux_initialize(struct device *port)
{
	ARG_UNUSED(port);

#ifdef CONFIG_UART_CC32XX
	/* Configure PIN_55 for UART0 UART0_TX */
	MAP_PinTypeUART(PIN_55, PIN_MODE_3);

	/* Configure PIN_57 for UART0 UART0_RX */
	MAP_PinTypeUART(PIN_57, PIN_MODE_3);
#endif

#ifdef CONFIG_GPIO_CC32XX_A1
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
#endif

#ifdef CONFIG_GPIO_CC32XX_A2
	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);

	/* SW2: Configure PIN_15 (GPIO22) for GPIOInput */
	MAP_PinTypeGPIO(PIN_15, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA2_BASE, 0x40, GPIO_DIR_MODE_IN);
#endif

#ifdef CONFIG_GPIO_CC32XX_A3
	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK);
#endif

	return 0;
}

SYS_INIT(pinmux_initialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
