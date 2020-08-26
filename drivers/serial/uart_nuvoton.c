/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Saravanan Sekar <saravanan@linumiz.com>
 */

#include <drivers/uart.h>
#include <NuMicro.h>
#include <string.h>

#define DT_DRV_COMPAT nuvoton_numicro_uart

/* Device data structure */
#define DEV_CFG(dev)						\
	((const struct uart_numicro_config * const)(dev)->config)

#define DRV_DATA(dev)						\
	((struct uart_numicro_data * const)(dev)->data)

#define UART_STRUCT(dev)					\
	((UART_T *)(DEV_CFG(dev))->devcfg.base)

struct uart_numicro_config {
	struct uart_device_config devcfg;
	uint32_t id_rst;
	uint32_t id_clk;
};

struct uart_numicro_data {
	struct device *clock;
	struct uart_config ucfg;
};

static int uart_numicro_poll_in(struct device *dev, unsigned char *c)
{
	uint32_t count;

	count = UART_Read(UART_STRUCT(dev), c, 1);
	if (!count) {
		return -1;
	}

	return 0;
}

static void uart_numicro_poll_out(struct device *dev, unsigned char c)
{
	UART_Write(UART_STRUCT(dev), &c, 1);
}

static int uart_numicro_err_check(struct device *dev)
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

static int uart_numicro_configure(struct device *dev,
				  const struct uart_config *cfg)
{
	struct uart_numicro_data *ddata = DRV_DATA(dev);
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
		UART_DisableFlowCtrl(UART_STRUCT(dev));
	} else if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		UART_EnableFlowCtrl(UART_STRUCT(dev));
	} else {
		return -ENOTSUP;
	}

	parity = uart_numicro_convert_parity(cfg->parity);

	UART_SetLineConfig(UART_STRUCT(dev), cfg->baudrate, databits,
			   parity, stopbits);

	memcpy(&ddata->ucfg, cfg, sizeof(*cfg));

	return 0;
}

static int uart_numicro_config_get(struct device *dev, struct uart_config *cfg)
{
	struct uart_numicro_data *ddata = DRV_DATA(dev);

	memcpy(cfg, &ddata->ucfg, sizeof(*cfg));

	return 0;
}

static int uart_numicro_init(struct device *dev)
{
	const struct uart_numicro_config *config = DEV_CFG(dev);
	struct uart_numicro_data *ddata = DRV_DATA(dev);

	SYS_ResetModule(config->id_rst);

	SYS_UnlockReg();

	/* Enable UART module clock */
	CLK_EnableModuleClock(config->id_clk);

	/* Select UART0 clock source is PLL */
	CLK_SetModuleClock(config->id_clk, CLK_CLKSEL1_UART0SEL_PLL,
			   CLK_CLKDIV0_UART0(0));

	/* Set pinctrl for UART0 RXD and TXD */
	SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
	SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD |
			  SYS_GPB_MFPH_PB13MFP_UART0_TXD);

	SYS_LockReg();

	UART_Open(UART_STRUCT(dev), ddata->ucfg.baudrate);

	return 0;
}

static const struct uart_driver_api uart_numicro_driver_api = {
	.poll_in          = uart_numicro_poll_in,
	.poll_out         = uart_numicro_poll_out,
	.err_check        = uart_numicro_err_check,
	.configure        = uart_numicro_configure,
	.config_get       = uart_numicro_config_get,
};

#define NUMICRO_INIT(index)						\
									\
static const struct uart_numicro_config uart_numicro_cfg_##index = {	\
	.devcfg = {							\
		.base = (uint8_t *)DT_INST_REG_ADDR(index),		\
	},								\
	.id_rst = UART##index##_RST,					\
	.id_clk = UART##index##_MODULE,					\
};									\
									\
static struct uart_numicro_data uart_numicro_data_##index = {		\
	.ucfg = {							\
		.baudrate = DT_INST_PROP(index, current_speed),		\
	},								\
};									\
									\
DEVICE_AND_API_INIT(uart_numicro_##index, DT_INST_LABEL(index),		\
		    &uart_numicro_init,					\
		    &uart_numicro_data_##index,				\
		    &uart_numicro_cfg_##index,				\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &uart_numicro_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_INIT)
