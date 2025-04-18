/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_PL011_AMBIQ_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_PL011_AMBIQ_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include "uart_pl011_registers.h"
#include <soc.h>

static inline void pl011_ambiq_enable_clk(const struct device *dev)
{
	get_uart(dev)->cr |= PL011_CR_AMBIQ_CLKEN;
}

static inline int pl011_ambiq_clk_set(const struct device *dev, uint32_t clk)
{
	uint8_t clksel;

	switch (clk) {
	case 3000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_3MHZ;
		break;
	case 6000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_6MHZ;
		break;
	case 12000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_12MHZ;
		break;
	case 24000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_24MHZ;
		break;
#if !defined(CONFIG_SOC_SERIES_APOLLO3X)
	case 48000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_48MHZ;
		break;
#if !defined(CONFIG_SOC_SERIES_APOLLO4X)
	case AM_HAL_UART_PLLCLK_FREQ:
		clksel = PL011_CR_AMBIQ_CLKSEL_PLL;
		break;
#endif
#endif
	default:
		return -EINVAL;
	}

	get_uart(dev)->cr |= FIELD_PREP(PL011_CR_AMBIQ_CLKSEL, clksel);
	return 0;
}

static inline int clk_enable_ambiq_uart(const struct device *dev, uint32_t clk)
{
	pl011_ambiq_enable_clk(dev);
	return pl011_ambiq_clk_set(dev, clk);
}

#ifdef CONFIG_PM_DEVICE

/* Register status record.
 * The register status will be preserved to this variable before entering sleep mode,
 * and they will be restored after wake up.
 */
typedef struct {
	bool bValid;
	uint32_t regILPR;
	uint32_t regIBRD;
	uint32_t regFBRD;
	uint32_t regLCRH;
	uint32_t regCR;
	uint32_t regIFLS;
	uint32_t regIER;
} uart_register_state_t;
static uart_register_state_t sRegState[AM_REG_UART_NUM_MODULES];

static int uart_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	int key;

	/*Uart module number*/
	uint32_t ui32Module = ((uint32_t)get_uart(dev) == UART0_BASE) ? 0 : 1;

	/*Uart Power module*/
	am_hal_pwrctrl_periph_e eUARTPowerModule =
		((am_hal_pwrctrl_periph_e)(AM_HAL_PWRCTRL_PERIPH_UART0 + ui32Module));

	/*Uart register status*/
	uart_register_state_t *pRegisterStatus = &sRegState[ui32Module];

	/* Decode the requested power state and update UART operation accordingly.*/
	switch (action) {

	/* Turn on the UART. */
	case PM_DEVICE_ACTION_RESUME:

		/* Make sure we don't try to restore an invalid state.*/
		if (!pRegisterStatus->bValid) {
			return -EPERM;
		}

		/*The resume and suspend actions may be executed back-to-back,
		 * so we add a busy wait here for stabilization.
		 */
		k_busy_wait(100);

		/* Enable power control.*/
		am_hal_pwrctrl_periph_enable(eUARTPowerModule);

		/* Restore UART registers*/
		key = irq_lock();

		UARTn(ui32Module)->ILPR = pRegisterStatus->regILPR;
		UARTn(ui32Module)->IBRD = pRegisterStatus->regIBRD;
		UARTn(ui32Module)->FBRD = pRegisterStatus->regFBRD;
		UARTn(ui32Module)->LCRH = pRegisterStatus->regLCRH;
		UARTn(ui32Module)->CR = pRegisterStatus->regCR;
		UARTn(ui32Module)->IFLS = pRegisterStatus->regIFLS;
		UARTn(ui32Module)->IER = pRegisterStatus->regIER;
		pRegisterStatus->bValid = false;

		irq_unlock(key);

		return 0;
	case PM_DEVICE_ACTION_SUSPEND:

		while ((get_uart(dev)->fr & PL011_FR_BUSY) != 0)
			;

		/* Preserve UART registers*/
		key = irq_lock();

		pRegisterStatus->regILPR = UARTn(ui32Module)->ILPR;
		pRegisterStatus->regIBRD = UARTn(ui32Module)->IBRD;
		pRegisterStatus->regFBRD = UARTn(ui32Module)->FBRD;
		pRegisterStatus->regLCRH = UARTn(ui32Module)->LCRH;
		pRegisterStatus->regCR = UARTn(ui32Module)->CR;
		pRegisterStatus->regIFLS = UARTn(ui32Module)->IFLS;
		pRegisterStatus->regIER = UARTn(ui32Module)->IER;
		pRegisterStatus->bValid = true;

		irq_unlock(key);

		/* Clear all interrupts before sleeping as having a pending UART
		 * interrupt burns power.
		 */
		UARTn(ui32Module)->IEC = 0xFFFFFFFF;

		/* If the user is going to sleep, certain bits of the CR register
		 * need to be 0 to be low power and have the UART shut off.
		 * Since the user either wishes to retain state which takes place
		 * above or the user does not wish to retain state, it is acceptable
		 * to set the entire CR register to 0.
		 */
		UARTn(ui32Module)->CR = 0;

		/* Disable power control.*/
		am_hal_pwrctrl_periph_disable(eUARTPowerModule);
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define AMBIQ_UART_DEFINE(n)                                                                       \
	PM_DEVICE_DT_INST_DEFINE(n, uart_ambiq_pm_action);                                         \
	static int pwr_on_ambiq_uart_##n(void)                                                     \
	{                                                                                          \
		uint32_t module = (DT_INST_REG_ADDR(n) - UART0_BASE) / (UART1_BASE - UART0_BASE);  \
		am_hal_pwrctrl_periph_e eUARTPowerModule =                                         \
			((am_hal_pwrctrl_periph_e)(AM_HAL_PWRCTRL_PERIPH_UART0 + module));         \
		return am_hal_pwrctrl_periph_enable(eUARTPowerModule);                             \
	}                                                                                          \
	static inline int clk_enable_ambiq_uart_##n(const struct device *dev, uint32_t clk)        \
	{                                                                                          \
		return clk_enable_ambiq_uart(dev, clk);                                            \
	}

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_PL011_AMBIQ_H_ */
