/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mediatek_mt8188_uart

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

#include "uart_mtk_common.h"

static DEVICE_API(uart, uart_mtk_driver_api) = {
	.poll_in = uart_mtk_poll_in,
	.poll_out = uart_mtk_poll_out,
	.err_check = uart_mtk_err_check,

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_mtk_configure,
	.config_get = uart_mtk_config_get,
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mtk_fifo_fill,
	.fifo_read = uart_mtk_fifo_read,
	.irq_tx_enable = uart_mtk_irq_tx_enable,
	.irq_tx_disable = uart_mtk_irq_tx_disable,
	.irq_tx_ready = uart_mtk_irq_tx_ready,
	.irq_rx_enable = uart_mtk_irq_rx_enable,
	.irq_rx_disable = uart_mtk_irq_rx_disable,
	.irq_rx_ready = uart_mtk_irq_rx_ready,
	.irq_is_pending = uart_mtk_irq_is_pending,
	.irq_update = uart_mtk_irq_update,
	.irq_callback_set = uart_mtk_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_IRQ_CONFIG_FUNC(n)                                                                    \
	static void uart_mtk_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_mtk_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define UART_IRQ_CONFIG_INIT(n) .irq_config_func = uart_mtk_irq_config_##n,
#else
#define UART_IRQ_CONFIG_FUNC(n)
#define UART_IRQ_CONFIG_INIT(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_CLOCK_CONTROL
#define UART_CLOCK_INIT(n)                                                                         \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                        \
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, id),
#else
#define UART_CLOCK_INIT(n)
#endif

#ifdef CONFIG_PINCTRL
#define UART_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define UART_PINCTRL_INIT(n)   .pinctrl_config = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define UART_PINCTRL_DEFINE(n)
#define UART_PINCTRL_INIT(n)
#endif

#define UART_INIT(n)                                                                               \
	UART_PINCTRL_DEFINE(n)                                                                     \
	UART_IRQ_CONFIG_FUNC(n)                                                                    \
                                                                                                   \
	static uart_mtk_data_t uart_mtk_data_##n = {                                               \
		.uart_cfg =                                                                        \
			{                                                                          \
				.baudrate = DT_INST_PROP(n, current_speed),                        \
				.parity = DT_INST_ENUM_IDX(n, parity),                             \
				.stop_bits = DT_INST_ENUM_IDX(n, stop_bits),                       \
				.data_bits = DT_INST_ENUM_IDX(n, data_bits),                       \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static const uart_mtk_config_t uart_mtk_config_##n = {                                     \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		UART_CLOCK_INIT(n) UART_PINCTRL_INIT(n) UART_IRQ_CONFIG_INIT(n)};                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &uart_mtk_init, NULL, &uart_mtk_data_##n, &uart_mtk_config_##n,   \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_mtk_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_INIT)
