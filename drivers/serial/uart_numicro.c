/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Saravanan Sekar <saravanan@linumiz.com>
 * Copyright (c) 2026 Fiona Behrens <me@kloenk.dev>
 */

#define DT_DRV_COMPAT nuvoton_numicro_uart

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numicro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree.h>
#include <NuMicro.h>

struct numicro_uart_config {
	UART_T *regs;
	const struct reset_dt_spec reset;
	const struct numicro_scc_subsys clock_subsys;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;
};

struct numicro_uart_data {
	struct uart_config ucfg;
};

static int numicro_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct numicro_uart_config *config = dev->config;

	if ((config->regs->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) != 0) {
		return -1;
	}

	*c = (uint8_t)config->regs->DAT;

	return 0;
}

static void numicro_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct numicro_uart_config *config = dev->config;

	UART_Write(config->regs, &c, 1);
}

static int numicro_uart_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static inline int32_t numicro_uart_convert_stopbit(enum uart_config_stop_bits sb)
{
	switch (sb) {
	case UART_CFG_STOP_BITS_1:
		return UART_STOP_BIT_1;
	case UART_CFG_STOP_BITS_1_5:
		return UART_STOP_BIT_1_5;
	case UART_CFG_STOP_BITS_2:
		return UART_STOP_BIT_2;
	default:
		return -ENOTSUP;
	}
};

static inline int32_t numicro_uart_convert_datalen(enum uart_config_data_bits db)
{
	switch (db) {
	case UART_CFG_DATA_BITS_5:
		return UART_WORD_LEN_5;
	case UART_CFG_DATA_BITS_6:
		return UART_WORD_LEN_6;
	case UART_CFG_DATA_BITS_7:
		return UART_WORD_LEN_7;
	case UART_CFG_DATA_BITS_8:
		return UART_WORD_LEN_8;
	default:
		return -ENOTSUP;
	}
}

static inline uint32_t numicro_uart_convert_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return UART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return UART_PARITY_EVEN;
	case UART_CFG_PARITY_MARK:
		return UART_PARITY_MARK;
	case UART_CFG_PARITY_SPACE:
		return UART_PARITY_SPACE;
	case UART_CFG_PARITY_NONE:
	default:
		return UART_PARITY_NONE;
	}
}

static inline int numicro_uart_set_buadrate(const struct device *dev, uint32_t baudrate)
{
	int err;
	const struct numicro_uart_config *config = dev->config;

	if (baudrate != 0) {
		uint32_t src_rate = 0;
		err = clock_control_get_rate(
			config->clk_dev, (clock_control_subsys_t)&config->clock_subsys, &src_rate);
		if (err < 0) {
			return err;
		}

		uint32_t baud_div = UART_BAUD_MODE2_DIVIDER(src_rate, baudrate);
		if (baud_div > 0xFFFF) {
			config->regs->BAUD =
				(UART_BAUD_MODE0 | UART_BAUD_MODE0_DIVIDER(src_rate, baudrate));
		} else {
			config->regs->BAUD = (UART_BAUD_MODE2 | baud_div);
		}
	}
	return 0;
}

/* Apply config without chainging anything. use in numicro_uart_configure and numicro_uart_init */
static int numicro_uart_set_config(const struct device *dev, const struct uart_config *cfg)
{
	const struct numicro_uart_config *config = dev->config;

	int32_t databits, stopbits, rc;
	uint32_t parity;

	databits = numicro_uart_convert_datalen(cfg->data_bits);
	if (databits < 0) {
		return databits;
	}

	stopbits = numicro_uart_convert_stopbit(cfg->stop_bits);
	if (stopbits < 0) {
		return stopbits;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE) {
		config->regs->INTEN &= ~(UART_INTEN_ATORTSEN_Msk | UART_INTEN_ATOCTSEN_Msk);
	} else if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		config->regs->MODEM |= UART_MODEM_RTSACTLV_Msk;
		config->regs->MODEMSTS |= UART_MODEMSTS_CTSACTLV_Msk;
		config->regs->INTEN |= UART_INTEN_ATORTSEN_Msk | UART_INTEN_ATOCTSEN_Msk;
	} else {
		return -ENOTSUP;
	}

	parity = numicro_uart_convert_parity(cfg->parity);

	rc = numicro_uart_set_buadrate(dev, cfg->baudrate);
	if (rc < 0) {
		return rc;
	}

	config->regs->LINE = databits | parity | stopbits;

	return rc;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int numicro_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct numicro_uart_data *data = dev->data;
	int rc;

	rc = numicro_uart_set_config(dev, cfg);
	if (rc < 0) {
		return rc;
	}

	memcpy(&data->ucfg, cfg, sizeof(*cfg));

	return rc;
}

static int numicro_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct numicro_uart_data *data = dev->data;

	memcpy(cfg, &data->ucfg, sizeof(*cfg));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int numicro_uart_init(const struct device *dev)
{
	const struct numicro_uart_config *config = dev->config;
	struct numicro_uart_data *data = dev->data;
	int err;

	/* Same as BSP SYS_ResetModule */
	reset_line_toggle_dt(&config->reset);

	/* Equivalent to CLK_EnableModuleClock(clk_modidx) */
	err = clock_control_on(config->clk_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (err != 0) {
		goto out;
	}
	/* Equivalent to CLK_SetModuleClock(clk_modidx, clk_src, clk_div) */
	err = clock_control_configure(config->clk_dev,
				      (clock_control_subsys_t)&config->clock_subsys, NULL);
	if (err != 0) {
		goto out;
	}

	/* Set pinctrl for UART0 RXD and TXD */
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		goto out;
	}

	/* select UART function */
	config->regs->FUNCSEL = UART_FUNCSEL_UART;

	/* Set UART RX and RTS trigger level */
	config->regs->FIFO &= ~(UART_FIFO_RFITL_Msk | UART_FIFO_RTSTRGLV_Msk);

	err = numicro_uart_set_config(dev, &data->ucfg);
	if (err < 0) {
		goto out;
	}
out:
	return err;
}

static DEVICE_API(uart, numicro_uart_driver_api) = {
	.poll_in = numicro_uart_poll_in,
	.poll_out = numicro_uart_poll_out,
	.err_check = numicro_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = numicro_uart_configure,
	.config_get = numicro_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
};

#define NUMICRO_UART_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct numicro_uart_config numicro_uart_config_##inst = {                     \
		.regs = (UART_T *)DT_INST_REG_ADDR(inst),                                          \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clock_subsys =                                                                    \
			{                                                                          \
				.subsys_id = NUMICRO_SCC_SUBSYS_ID_PCC,                            \
				.pcc =                                                             \
					{                                                          \
						.clk_mod = DT_INST_CLOCKS_CELL(                    \
							inst, clock_module_index),                 \
						.clk_src =                                         \
							DT_INST_CLOCKS_CELL(inst, clock_source),   \
						.clk_div =                                         \
							DT_INST_CLOCKS_CELL(inst, clock_divider),  \
					},                                                         \
			},                                                                         \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
	};                                                                                         \
                                                                                                   \
	static struct numicro_uart_data numicro_uart_data_##inst = {                               \
		.ucfg = {.baudrate = DT_INST_PROP(inst, current_speed),                            \
			 .parity = DT_INST_ENUM_IDX(inst, parity),                                 \
			 .stop_bits = DT_INST_ENUM_IDX(inst, stop_bits),                           \
			 .data_bits = DT_INST_ENUM_IDX(inst, data_bits),                           \
			 .flow_ctrl = DT_INST_PROP(inst, hw_flow_control)},                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, numicro_uart_init, NULL, &numicro_uart_data_##inst,            \
			      &numicro_uart_config_##inst, PRE_KERNEL_1,                           \
			      CONFIG_SERIAL_INIT_PRIORITY, &numicro_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_UART_INIT)
