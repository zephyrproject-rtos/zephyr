/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the I/O controller (pinmux) for
 * Texas Instrument's CC2650 SoC.
 *
 * For these SoCs, available pin functions are as follows:
 *
 * 0x00 - GPIO
 * 0x07 - AON 32 Khz clock
 * 0x08 - AUX IO
 * 0x09 - SSI0 RX
 * 0x0A - SSI0 TX
 * 0x0B - SSI0 FSS
 * 0x0C - SSI0 CLK
 * 0x0D - I2C SDA
 * 0x0E - I2C SCL
 * 0x0F - UART0 RX
 * 0x10 - UART0 TX
 * 0x11 - UART0 CTS
 * 0x12 - UART0 RTS
 * 0x17 - Port event 0
 * 0x18 - Port event 1
 * 0x19 - Port event 2
 * 0x1A - Port event 3
 * 0x1B - Port event 4
 * 0x1C - Port event 5
 * 0x1D - Port event 6
 * 0x1E - Port event 7
 * 0x20 - CPU SWV
 * 0x21 - SSI1 RX
 * 0x22 - SSI1 TX
 * 0x23 - SSI1 FSS
 * 0x24 - SSI1 CLK
 * 0x25 - I2S data 0
 * 0x26 - I2S data 1
 * 0x27 - I2S WCLK
 * 0x28 - I2S BCLK
 * 0x29 - I2S MCLK
 * 0x2E - RF Core Trace
 * 0x2F - RF Core data out 0
 * 0x30 - RF Core data out 1
 * 0x31 - RF Core data out 2
 * 0x32 - RF Core data out 3
 * 0x33 - RF Core data in 0
 * 0x34 - RF Core data in 1
 * 0x35 - RF Core SMI data link out
 * 0x36 - RF Core SMI data link in
 * 0x37 - RF Core SMI command link out
 * 0x38 - RF Core SMI command link in
 */

#include <sys/__assert.h>
#include <sys/util.h>
#include <toolchain.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <soc.h>
#include <sys/sys_io.h>


#define IOCFG_REG(Func) \
	REG_ADDR(DT_TI_CC2650_PINMUX_40081000_BASE_ADDRESS, \
		 CC2650_IOC_IOCFG0 + 0x4 * Func)

static int pinmux_cc2650_init(struct device *dev)
{
	/* Do nothing */
	ARG_UNUSED(dev);
	return 0;
}

static int pinmux_cc2650_set(struct device *dev, u32_t pin,
			      u32_t func)
{
	const u32_t iocfg = IOCFG_REG(pin);
	u32_t conf = sys_read32(iocfg);

	conf &= ~CC2650_IOC_IOCFGX_PORT_ID_MASK;
	conf |= func & CC2650_IOC_IOCFGX_PORT_ID_MASK;
	sys_write32(conf, iocfg);

	return 0;
};

static int pinmux_cc2650_get(struct device *dev, u32_t pin,
			      u32_t *func)
{
	const u32_t iocfg = IOCFG_REG(pin);
	u32_t conf = sys_read32(iocfg);

	*func = conf & CC2650_IOC_IOCFGX_PORT_ID_MASK;
	return 0;
}

static int pinmux_cc2650_pullup(struct device *dev, u32_t pin,
				 u8_t func)
{
	__ASSERT((func == PINMUX_PULLUP_ENABLE) |
		 (func == PINMUX_PULLUP_DISABLE),
		 "Pullup mode is invalid");

	const u32_t iocfg = IOCFG_REG(pin);
	u32_t conf = sys_read32(iocfg);

	conf &= ~CC2650_IOC_IOCFGX_PULL_CTL_MASK;
	if (func == PINMUX_PULLUP_ENABLE) {
		conf |= CC2650_IOC_PULL_UP;
	} else {
		conf |= CC2650_IOC_NO_PULL;
	}
	sys_write32(conf, iocfg);

	return 0;
}

static int pinmux_cc2650_input(struct device *dev, u32_t pin,
				u8_t func)
{
	__ASSERT((func == PINMUX_INPUT_ENABLED) |
		 (func == PINMUX_OUTPUT_ENABLED),
		 "I/O mode is invalid");

	const u32_t iocfg = IOCFG_REG(pin);
	const u32_t gpio_doe = DT_TI_CC2650_GPIO_40022000_BASE_ADDRESS +
			       CC2650_GPIO_DOE31_0;
	u32_t iocfg_conf = sys_read32(iocfg);
	u32_t gpio_doe_conf = sys_read32(gpio_doe);

	iocfg_conf    &= ~CC2650_IOC_IOCFGX_IE_MASK;
	if (func == PINMUX_INPUT_ENABLED) {
		iocfg_conf |= CC2650_IOC_INPUT_ENABLED;
		gpio_doe_conf &= ~BIT(pin);
	} else {
		iocfg_conf |= CC2650_IOC_INPUT_DISABLED;
		gpio_doe_conf |= BIT(pin);
	}
	sys_write32(iocfg_conf, iocfg);
	sys_write32(gpio_doe_conf, gpio_doe);

	return 0;

}

const struct pinmux_driver_api pinmux_cc2650_funcs = {
	.set    = pinmux_cc2650_set,
	.get    = pinmux_cc2650_get,
	.pullup = pinmux_cc2650_pullup,
	.input  = pinmux_cc2650_input
};

DEVICE_AND_API_INIT(pinmux_cc2650_0, CONFIG_PINMUX_NAME,
		    pinmux_cc2650_init, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_cc2650_funcs);
