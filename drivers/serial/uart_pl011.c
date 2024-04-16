/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_pl011
#define SBSA_COMPAT arm_sbsa_uart

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/irq.h>
#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif

#ifdef CONFIG_CPU_CORTEX_M
#include <cmsis_compiler.h>
#endif

#include "uart_pl011_registers.h"
#include "uart_pl011_ambiq.h"

struct pl011_config {
	DEVICE_MMIO_ROM;
	uint32_t sys_clk_freq;
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pincfg;
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
	int (*clk_enable_func)(const struct device *dev, uint32_t clk);
	int (*pwr_on_func)(void);
};

/* Device data structure */
struct pl011_data {
	DEVICE_MMIO_RAM;
	uint32_t baud_rate;	/* Baud rate */
	bool sbsa;		/* SBSA mode */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	volatile bool sw_call_txdrdy;
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
};

static void pl011_enable(const struct device *dev)
{
	get_uart(dev)->cr |=  PL011_CR_UARTEN;
}

static void pl011_disable(const struct device *dev)
{
	get_uart(dev)->cr &= ~PL011_CR_UARTEN;
}

static void pl011_enable_fifo(const struct device *dev)
{
	get_uart(dev)->lcr_h |= PL011_LCRH_FEN;
}

static void pl011_disable_fifo(const struct device *dev)
{
	get_uart(dev)->lcr_h &= ~PL011_LCRH_FEN;
}

static int pl011_set_baudrate(const struct device *dev,
			      uint32_t clk, uint32_t baudrate)
{
	/* Avoiding float calculations, bauddiv is left shifted by 6 */
	uint64_t bauddiv = (((uint64_t)clk) << PL011_FBRD_WIDTH)
				/ (baudrate * 16U);

	/* Valid bauddiv value
	 * uart_clk (min) >= 16 x baud_rate (max)
	 * uart_clk (max) <= 16 x 65535 x baud_rate (min)
	 */
	if ((bauddiv < (1u << PL011_FBRD_WIDTH))
		|| (bauddiv > (65535u << PL011_FBRD_WIDTH))) {
		return -EINVAL;
	}

	get_uart(dev)->ibrd = bauddiv >> PL011_FBRD_WIDTH;
	get_uart(dev)->fbrd = bauddiv & ((1u << PL011_FBRD_WIDTH) - 1u);

	barrier_dmem_fence_full();

	/* In order to internally update the contents of ibrd or fbrd, a
	 * lcr_h write must always be performed at the end
	 * ARM DDI 0183F, Pg 3-13
	 */
	get_uart(dev)->lcr_h = get_uart(dev)->lcr_h;

	return 0;
}

static bool pl011_is_readable(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	if (!data->sbsa &&
	    (!(get_uart(dev)->cr & PL011_CR_UARTEN) || !(get_uart(dev)->cr & PL011_CR_RXE))) {
		return false;
	}

	return (get_uart(dev)->fr & PL011_FR_RXFE) == 0U;
}

static int pl011_poll_in(const struct device *dev, unsigned char *c)
{
	if (!pl011_is_readable(dev)) {
		return -1;
	}

	/* got a character */
	*c = (unsigned char)get_uart(dev)->dr;

	return get_uart(dev)->rsr & PL011_RSR_ERROR_MASK;
}

static void pl011_poll_out(const struct device *dev,
					     unsigned char c)
{
	/* Wait for space in FIFO */
	while (get_uart(dev)->fr & PL011_FR_TXFF) {
		; /* Wait */
	}

	/* Send a character */
	get_uart(dev)->dr = (uint32_t)c;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int pl011_fifo_fill(const struct device *dev,
				    const uint8_t *tx_data, int len)
{
	uint8_t num_tx = 0U;

	while (!(get_uart(dev)->fr & PL011_FR_TXFF) && (len - num_tx > 0)) {
		get_uart(dev)->dr = tx_data[num_tx++];
	}
	return num_tx;
}

static int pl011_fifo_read(const struct device *dev,
				    uint8_t *rx_data, const int len)
{
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) && !(get_uart(dev)->fr & PL011_FR_RXFE)) {
		rx_data[num_rx++] = get_uart(dev)->dr;
	}

	return num_rx;
}

static void pl011_irq_tx_enable(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	get_uart(dev)->imsc |= PL011_IMSC_TXIM;
	if (data->sw_call_txdrdy) {
		/* Verify if the callback has been registered */
		if (data->irq_cb) {
			/*
			 * Due to HW limitation, the first TX interrupt should
			 * be triggered by the software.
			 *
			 * PL011 TX interrupt is based on a transition through
			 * a level, rather than on the level itself[1]. So that,
			 * enable TX interrupt can not trigger TX interrupt if
			 * no data was filled to TX FIFO at the beginning.
			 *
			 * [1]: PrimeCell UART (PL011) Technical Reference Manual
			 *      functional-overview/interrupts
			 */
			data->irq_cb(dev, data->irq_cb_data);
		}
		data->sw_call_txdrdy = false;
	}
}

static void pl011_irq_tx_disable(const struct device *dev)
{
	get_uart(dev)->imsc &= ~PL011_IMSC_TXIM;
}

static int pl011_irq_tx_complete(const struct device *dev)
{
	/* Check for UART is busy transmitting data. */
	return ((get_uart(dev)->fr & PL011_FR_BUSY) == 0);
}

static int pl011_irq_tx_ready(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	if (!data->sbsa && !(get_uart(dev)->cr & PL011_CR_TXE))
		return false;

	return ((get_uart(dev)->imsc & PL011_IMSC_TXIM) &&
		/* Check for TX interrupt status is set or TX FIFO is empty. */
		(get_uart(dev)->ris & PL011_RIS_TXRIS || get_uart(dev)->fr & PL011_FR_TXFE));
}

static void pl011_irq_rx_enable(const struct device *dev)
{
	get_uart(dev)->imsc |= PL011_IMSC_RXIM | PL011_IMSC_RTIM;
}

static void pl011_irq_rx_disable(const struct device *dev)
{
	get_uart(dev)->imsc &= ~(PL011_IMSC_RXIM | PL011_IMSC_RTIM);
}

static int pl011_irq_rx_ready(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	if (!data->sbsa && !(get_uart(dev)->cr & PL011_CR_RXE))
		return false;

	return ((get_uart(dev)->imsc & PL011_IMSC_RXIM) &&
		(!(get_uart(dev)->fr & PL011_FR_RXFE)));
}

static void pl011_irq_err_enable(const struct device *dev)
{
	/* enable framing, parity, break, and overrun */
	get_uart(dev)->imsc |= PL011_IMSC_ERROR_MASK;
}

static void pl011_irq_err_disable(const struct device *dev)
{
	get_uart(dev)->imsc &= ~PL011_IMSC_ERROR_MASK;
}

static int pl011_irq_is_pending(const struct device *dev)
{
	return pl011_irq_rx_ready(dev) || pl011_irq_tx_ready(dev);
}

static int pl011_irq_update(const struct device *dev)
{
	return 1;
}

static void pl011_irq_callback_set(const struct device *dev,
					    uart_irq_callback_user_data_t cb,
					    void *cb_data)
{
	struct pl011_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api pl011_driver_api = {
	.poll_in = pl011_poll_in,
	.poll_out = pl011_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = pl011_fifo_fill,
	.fifo_read = pl011_fifo_read,
	.irq_tx_enable = pl011_irq_tx_enable,
	.irq_tx_disable = pl011_irq_tx_disable,
	.irq_tx_ready = pl011_irq_tx_ready,
	.irq_rx_enable = pl011_irq_rx_enable,
	.irq_rx_disable = pl011_irq_rx_disable,
	.irq_tx_complete = pl011_irq_tx_complete,
	.irq_rx_ready = pl011_irq_rx_ready,
	.irq_err_enable = pl011_irq_err_enable,
	.irq_err_disable = pl011_irq_err_disable,
	.irq_is_pending = pl011_irq_is_pending,
	.irq_update = pl011_irq_update,
	.irq_callback_set = pl011_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int pl011_init(const struct device *dev)
{
	const struct pl011_config *config = dev->config;
	struct pl011_data *data = dev->data;
	int ret;
	uint32_t lcrh;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/*
	 * If working in SBSA mode, we assume that UART is already configured,
	 * or does not require configuration at all (if UART is emulated by
	 * virtualization software).
	 */
	if (!data->sbsa) {
#if defined(CONFIG_PINCTRL)
		ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret) {
			return ret;
		}
#endif
		/* Call vendor-specific function to power on the peripheral */
		if (config->pwr_on_func != NULL) {
			ret = config->pwr_on_func();
		}

		/* disable the uart */
		pl011_disable(dev);
		pl011_disable_fifo(dev);

		/* Call vendor-specific function to enable clock for the peripheral */
		if (config->clk_enable_func != NULL) {
			ret = config->clk_enable_func(dev, config->sys_clk_freq);
			if (ret) {
				return ret;
			}
		}

		/* Set baud rate */
		ret = pl011_set_baudrate(dev, config->sys_clk_freq,
					 data->baud_rate);
		if (ret != 0) {
			return ret;
		}

		/* Setting the default character format */
		lcrh = get_uart(dev)->lcr_h & ~(PL011_LCRH_FORMAT_MASK);
		lcrh &= ~(BIT(0) | BIT(7));
		lcrh |= PL011_LCRH_WLEN_SIZE(8) << PL011_LCRH_WLEN_SHIFT;
		get_uart(dev)->lcr_h = lcrh;

		/* Setting transmit and receive interrupt FIFO level */
		get_uart(dev)->ifls = FIELD_PREP(PL011_IFLS_TXIFLSEL_M, TXIFLSEL_1_8_FULL)
			| FIELD_PREP(PL011_IFLS_RXIFLSEL_M, RXIFLSEL_1_2_FULL);

		/* Enabling the FIFOs */
		pl011_enable_fifo(dev);
	}
	/* initialize all IRQs as masked */
	get_uart(dev)->imsc = 0U;
	get_uart(dev)->icr = PL011_IMSC_MASK_ALL;

	if (!data->sbsa) {
		get_uart(dev)->dmacr = 0U;
		barrier_isync_fence_full();
		get_uart(dev)->cr &= ~(BIT(14) | BIT(15) | BIT(1));
		get_uart(dev)->cr |= PL011_CR_RXE | PL011_CR_TXE;
		barrier_isync_fence_full();
	}
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
	data->sw_call_txdrdy = true;
#endif
	if (!data->sbsa) {
		pl011_enable(dev);
	}

	return 0;
}

#if defined(CONFIG_PINCTRL)
#define PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define PINCTRL_DEFINE(n)
#define PINCTRL_INIT(n)
#endif /* CONFIG_PINCTRL */

#define PL011_GET_COMPAT_QUIRK_NONE(n)	NULL

#define PL011_GET_COMPAT_CLK_QUIRK_0(n)					\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), ambiq_uart),	\
		    (clk_enable_ambiq_uart),				\
		    PL011_GET_COMPAT_QUIRK_NONE(n))

#define PL011_GET_COMPAT_PWR_QUIRK_0(n)					\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), ambiq_uart),	\
		    (pwr_on_ambiq_uart_##n),				\
		    PL011_GET_COMPAT_QUIRK_NONE(n))

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void pl011_isr(const struct device *dev)
{
	struct pl011_data *data = dev->data;

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define PL011_IRQ_CONFIG_FUNC_BODY(n, prop, i)		\
	{						\
		IRQ_CONNECT(DT_IRQ_BY_IDX(n, i, irq),	\
			DT_IRQ_BY_IDX(n, i, priority),	\
			pl011_isr,			\
			DEVICE_DT_GET(n),		\
			0);				\
		irq_enable(DT_IRQ_BY_IDX(n, i, irq));	\
	}

#define PL011_CONFIG_PORT(n)								\
	static void pl011_irq_config_func_##n(const struct device *dev)			\
	{										\
		DT_INST_FOREACH_PROP_ELEM(n, interrupt_names,				\
			PL011_IRQ_CONFIG_FUNC_BODY)					\
	};										\
											\
	static struct pl011_config pl011_cfg_port_##n = {				\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),	\
		PINCTRL_INIT(n)	\
		.irq_config_func = pl011_irq_config_func_##n,				\
		.clk_enable_func = PL011_GET_COMPAT_CLK_QUIRK_0(n),			\
		.pwr_on_func = PL011_GET_COMPAT_PWR_QUIRK_0(n),				\
	};
#else
#define PL011_CONFIG_PORT(n)								\
	static struct pl011_config pl011_cfg_port_##n = {				\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),	\
		PINCTRL_INIT(n)	\
	};
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define PL011_INIT(n)						\
	PINCTRL_DEFINE(n)					\
	PL011_QUIRK_AMBIQ_UART_DEFINE(n)			\
	PL011_CONFIG_PORT(n)					\
								\
	static struct pl011_data pl011_data_port_##n = {	\
		.baud_rate = DT_INST_PROP(n, current_speed),	\
	};							\
								\
	DEVICE_DT_INST_DEFINE(n, &pl011_init,			\
			NULL,					\
			&pl011_data_port_##n,			\
			&pl011_cfg_port_##n,			\
			PRE_KERNEL_1,				\
			CONFIG_SERIAL_INIT_PRIORITY,		\
			&pl011_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PL011_INIT)

#ifdef CONFIG_UART_PL011_SBSA

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT SBSA_COMPAT

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define PL011_SBSA_CONFIG_PORT(n)						\
	static void pl011_irq_config_func_sbsa_##n(const struct device *dev)	\
	{									\
		DT_INST_FOREACH_PROP_ELEM(n, interrupt_names,			\
			PL011_IRQ_CONFIG_FUNC_BODY)				\
	};									\
										\
	static struct pl011_config pl011_cfg_sbsa_##n = {			\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),				\
		.irq_config_func = pl011_irq_config_func_sbsa_##n,		\
	};
#else
#define PL011_SBSA_CONFIG_PORT(n)				\
	static struct pl011_config pl011_cfg_sbsa_##n = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),		\
	};
#endif

#define PL011_SBSA_INIT(n)					\
	PL011_SBSA_CONFIG_PORT(n)				\
								\
	static struct pl011_data pl011_data_sbsa_##n = {	\
		.sbsa = true,					\
	};							\
								\
	DEVICE_DT_INST_DEFINE(n, &pl011_init,			\
			NULL,					\
			&pl011_data_sbsa_##n,			\
			&pl011_cfg_sbsa_##n,			\
			PRE_KERNEL_1,				\
			CONFIG_SERIAL_INIT_PRIORITY,		\
			&pl011_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PL011_SBSA_INIT)

#endif /* CONFIG_UART_PL011_SBSA */
