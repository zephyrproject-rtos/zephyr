/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_pl011
#define SBSA_COMPAT arm_sbsa_uart

#include <string.h>
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
#if defined(CONFIG_RESET)
#include <zephyr/drivers/reset.h>
#endif
#if defined(CONFIG_CLOCK_CONTROL)
#include <zephyr/drivers/clock_control.h>
#endif

#ifdef CONFIG_CPU_CORTEX_M
#include <cmsis_compiler.h>
#endif

#include "uart_pl011_registers.h"
#include "uart_pl011_ambiq.h"
#include "uart_pl011_raspberrypi_pico.h"

struct pl011_config {
	DEVICE_MMIO_ROM;
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pincfg;
#endif
#if defined(CONFIG_RESET)
	const struct reset_dt_spec reset;
#endif
#if defined(CONFIG_CLOCK_CONTROL)
	const struct device *clock_dev;
	clock_control_subsys_t clock_id;
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
	struct uart_config uart_cfg;
	bool sbsa;		/* SBSA mode */
	uint32_t clk_freq;
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

static void pl011_set_flow_control(const struct device *dev, bool rts, bool cts)
{
	if (rts) {
		get_uart(dev)->cr |= PL011_CR_RTSEn;
	} else {
		get_uart(dev)->cr &= ~PL011_CR_RTSEn;
	}
	if (cts) {
		get_uart(dev)->cr |= PL011_CR_CTSEn;
	} else {
		get_uart(dev)->cr &= ~PL011_CR_CTSEn;
	}
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

static int pl011_err_check(const struct device *dev)
{
	int errors = 0;

	if (get_uart(dev)->rsr & PL011_RSR_ECR_OE) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (get_uart(dev)->rsr & PL011_RSR_ECR_BE) {
		errors |= UART_BREAK;
	}

	if (get_uart(dev)->rsr & PL011_RSR_ECR_PE) {
		errors |= UART_ERROR_PARITY;
	}

	if (get_uart(dev)->rsr & PL011_RSR_ECR_FE) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int pl011_runtime_configure_internal(const struct device *dev,
					const struct uart_config *cfg,
					bool disable)
{
	struct pl011_data *data = dev->data;
	uint32_t lcrh;
	int ret = -ENOTSUP;

	if (data->sbsa) {
		goto out;
	}

	if (disable) {
		pl011_disable(dev);
		pl011_disable_fifo(dev);
	}

	lcrh = get_uart(dev)->lcr_h & ~(PL011_LCRH_FORMAT_MASK | PL011_LCRH_STP2);

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		lcrh &= ~(BIT(1) | BIT(2));
		break;
	case UART_CFG_PARITY_ODD:
		lcrh |= PL011_LCRH_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		lcrh |= PL011_LCRH_PARTIY_EVEN;
		break;
	default:
		goto enable;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		lcrh &= ~(PL011_LCRH_STP2);
		break;
	case UART_CFG_STOP_BITS_2:
		lcrh |= PL011_LCRH_STP2;
		break;
	default:
		goto enable;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		lcrh |= PL011_LCRH_WLEN_SIZE(5) << PL011_LCRH_WLEN_SHIFT;
		break;
	case UART_CFG_DATA_BITS_6:
		lcrh |= PL011_LCRH_WLEN_SIZE(6) << PL011_LCRH_WLEN_SHIFT;
		break;
	case UART_CFG_DATA_BITS_7:
		lcrh |= PL011_LCRH_WLEN_SIZE(7) << PL011_LCRH_WLEN_SHIFT;
		break;
	case UART_CFG_DATA_BITS_8:
		lcrh |= PL011_LCRH_WLEN_SIZE(8) << PL011_LCRH_WLEN_SHIFT;
		break;
	default:
		goto enable;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		pl011_set_flow_control(dev, false, false);
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		pl011_set_flow_control(dev, true, true);
		break;
	default:
		goto enable;
	}

	/* Set baud rate */
	ret = pl011_set_baudrate(dev, data->clk_freq, cfg->baudrate);
	if (ret != 0) {
		goto enable;
	}

	/* Update settings */
	get_uart(dev)->lcr_h = lcrh;

	memcpy(&data->uart_cfg, cfg, sizeof(data->uart_cfg));

enable:
	if (disable) {
		pl011_enable_fifo(dev);
		pl011_enable(dev);
	}

out:
	return ret;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int pl011_runtime_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	return pl011_runtime_configure_internal(dev, cfg, true);
}

static int pl011_runtime_config_get(const struct device *dev,
				struct uart_config *cfg)
{
	struct pl011_data *data = dev->data;

	*cfg = data->uart_cfg;
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

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
	.err_check = pl011_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = pl011_runtime_configure,
	.config_get = pl011_runtime_config_get,
#endif
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

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

#if defined(CONFIG_RESET)
	if (config->reset.dev) {
		ret = reset_line_toggle_dt(&config->reset);
		if (ret) {
			return ret;
		}
	}
#endif

#if defined(CONFIG_CLOCK_CONTROL)
	if (config->clock_dev) {
		clock_control_on(config->clock_dev, config->clock_id);
		clock_control_get_rate(config->clock_dev, config->clock_id, &data->clk_freq);
	}
#endif

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
			ret = config->clk_enable_func(dev, data->clk_freq);
			if (ret) {
				return ret;
			}
		}

		pl011_runtime_configure_internal(dev, &data->uart_cfg, false);

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
		get_uart(dev)->cr &= ~PL011_CR_SIREN;
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

#define COMPAT_SPECIFIC_FUNC_NAME(prefix, name) _CONCAT(_CONCAT(prefix, name), _)

/*
 * The first element of compatible is used to determine the type.
 * When compatible defines as "ambiq,uart", "arm,pl011",
 * this macro expands to pwr_on_ambiq_uart_[n].
 */
#define COMPAT_SPECIFIC_PWR_ON_FUNC(n)                                                             \
	_CONCAT(COMPAT_SPECIFIC_FUNC_NAME(pwr_on_, DT_INST_STRING_TOKEN_BY_IDX(n, compatible, 0)), \
		n)

/*
 * The first element of compatible is used to determine the type.
 * When compatible defines as "ambiq,uart", "arm,pl011",
 * this macro expands to clk_enable_ambiq_uart_[n].
 */
#define COMPAT_SPECIFIC_CLK_ENABLE_FUNC(n)                                                         \
	_CONCAT(COMPAT_SPECIFIC_FUNC_NAME(clk_enable_,                                             \
					  DT_INST_STRING_TOKEN_BY_IDX(n, compatible, 0)), n)

/*
 * The first element of compatible is used to determine the type.
 * When compatible defines as "ambiq,uart", "arm,pl011",
 * this macro expands to AMBIQ_UART_DEFINE(n).
 */
#define COMPAT_SPECIFIC_DEFINE(n)                                                                  \
	_CONCAT(DT_INST_STRING_UPPER_TOKEN_BY_IDX(n, compatible, 0), _DEFINE)(n)

#define COMPAT_SPECIFIC_CLOCK_CTLR_SUBSYS_CELL(n)                                                  \
	_CONCAT(DT_INST_STRING_UPPER_TOKEN_BY_IDX(n, compatible, 0), _CLOCK_CTLR_SUBSYS_CELL)

#if defined(CONFIG_PINCTRL)
#define PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define PINCTRL_DEFINE(n)
#define PINCTRL_INIT(n)
#endif /* CONFIG_PINCTRL */

#if defined(CONFIG_RESET)
#define RESET_INIT(n)                                                                              \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(0, resets), (.reset = RESET_DT_SPEC_INST_GET(n),))
#else
#define RESET_INIT(n)
#endif

#define CLOCK_INIT(n)                                                                              \
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR(n), fixed_clock), (),                   \
		    (.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                           \
		     .clock_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n,                    \
				  COMPAT_SPECIFIC_CLOCK_CTLR_SUBSYS_CELL(n)),))

#define ARM_PL011_DEFINE(n)                                                                        \
	static inline int pwr_on_arm_pl011_##n(void)                                               \
	{                                                                                          \
		return 0;                                                                          \
	}                                                                                          \
	static inline int clk_enable_arm_pl011_##n(const struct device *dev, uint32_t clk)         \
	{                                                                                          \
		return 0;                                                                          \
	}

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
		CLOCK_INIT(n)                                                           \
		PINCTRL_INIT(n)	                                                        \
		.irq_config_func = pl011_irq_config_func_##n,				\
		.clk_enable_func = COMPAT_SPECIFIC_CLK_ENABLE_FUNC(n),		        \
		.pwr_on_func = COMPAT_SPECIFIC_PWR_ON_FUNC(n),			        \
	};
#else
#define PL011_CONFIG_PORT(n)								\
	static struct pl011_config pl011_cfg_port_##n = {				\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		CLOCK_INIT(n)                                                           \
		PINCTRL_INIT(n)	                                                        \
	};
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define PL011_INIT(n)						\
	PINCTRL_DEFINE(n)					\
	COMPAT_SPECIFIC_DEFINE(n)				\
	PL011_CONFIG_PORT(n)					\
								\
	static struct pl011_data pl011_data_port_##n = {	\
		.uart_cfg = {					\
			.baudrate = DT_INST_PROP(n, current_speed), \
			.parity = UART_CFG_PARITY_NONE,			\
			.stop_bits = UART_CFG_STOP_BITS_1,		\
			.data_bits = UART_CFG_DATA_BITS_8,		\
			.flow_ctrl = DT_INST_PROP(n, hw_flow_control)	\
				? UART_CFG_FLOW_CTRL_RTS_CTS		\
				: UART_CFG_FLOW_CTRL_NONE,		\
		},							\
		.clk_freq = COND_CODE_1(                                                \
			DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR(n), fixed_clock),        \
			(DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency)), (0)),    \
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
