/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Saravanan Sekar <saravanan@linumiz.com>
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <NuMicro.h>
#include <string.h>

#define DT_DRV_COMPAT nuvoton_numicro_uart

struct uart_numicro_config {
	UART_T *uart;
	uint32_t id_rst;
	uint32_t id_clk;
	const struct pinctrl_dev_config *pincfg;
};

struct uart_numicro_data {
	const struct device *clock;
	struct uart_config ucfg;
};

static int uart_numicro_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_numicro_config *config = dev->config;

	if ((config->uart->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) != 0) {
		return -1;
	}

	*c = (uint8_t)config->uart->DAT;

	return 0;
}

static void uart_numicro_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_numicro_config *config = dev->config;

	UART_Write(config->uart, &c, 1);
}

static int uart_numicro_err_check(const struct device *dev)
{
	return 0;
}

static inline int32_t uart_numicro_convert_stopbit(enum uart_config_stop_bits sb)
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

static inline int32_t uart_numicro_convert_datalen(enum uart_config_data_bits db)
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

static inline uint32_t uart_numicro_convert_parity(enum uart_config_parity parity)
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

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_numicro_configure(const struct device *dev,
				  const struct uart_config *cfg)
{
	const struct uart_numicro_config *config = dev->config;
	struct uart_numicro_data *ddata = dev->data;
	int32_t databits, stopbits;
	uint32_t parity;

	databits = uart_numicro_convert_datalen(cfg->data_bits);
	if (databits < 0) {
		return databits;
	}

	stopbits = uart_numicro_convert_stopbit(cfg->stop_bits);
	if (stopbits < 0) {
		return stopbits;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE) {
		UART_DisableFlowCtrl(config->uart);
	} else if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		UART_EnableFlowCtrl(config->uart);
	} else {
		return -ENOTSUP;
	}

	parity = uart_numicro_convert_parity(cfg->parity);

	UART_SetLineConfig(config->uart, cfg->baudrate, databits, parity,
			   stopbits);

	memcpy(&ddata->ucfg, cfg, sizeof(*cfg));

	return 0;
}

static int uart_numicro_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	struct uart_numicro_data *ddata = dev->data;

	memcpy(cfg, &ddata->ucfg, sizeof(*cfg));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_numicro_init(const struct device *dev)
{
	const struct uart_numicro_config *config = dev->config;
	struct uart_numicro_data *ddata = dev->data;
	int err;

	SYS_ResetModule(config->id_rst);

	SYS_UnlockReg();

	/* Enable UART module clock */
	CLK_EnableModuleClock(config->id_clk);

	/* Select UART0 clock source is PLL */
	CLK_SetModuleClock(config->id_clk, CLK_CLKSEL1_UART0SEL_PLL,
			   CLK_CLKDIV0_UART0(0));

	SYS_LockReg();

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	UART_Open(config->uart, ddata->ucfg.baudrate);

	return 0;
}

static const struct uart_driver_api uart_numicro_driver_api = {
	.poll_in          = uart_numicro_poll_in,
	.poll_out         = uart_numicro_poll_out,
	.err_check        = uart_numicro_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure        = uart_numicro_configure,
	.config_get       = uart_numicro_config_get,
#endif
};

#define NUMICRO_INIT(index)						\
PINCTRL_DT_INST_DEFINE(index);						\
									\
static const struct uart_numicro_config uart_numicro_cfg_##index = {	\
	.uart = (UART_T *)DT_INST_REG_ADDR(index),			\
	.id_rst = UART##index##_RST,					\
	.id_clk = UART##index##_MODULE,					\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),		\
};									\
									\
static struct uart_numicro_data uart_numicro_data_##index = {		\
	.ucfg = {							\
		.baudrate = DT_INST_PROP(index, current_speed),		\
	},								\
};									\
									\
DEVICE_DT_INST_DEFINE(index,						\
		    uart_numicro_init,					\
		    NULL,						\
		    &uart_numicro_data_##index,				\
		    &uart_numicro_cfg_##index,				\
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		\
		    &uart_numicro_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_INIT)
