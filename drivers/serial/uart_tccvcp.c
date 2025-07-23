/*
 * Copyright (c) 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tcc_tccvcp_uart

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>

#include <zephyr/dt-bindings/interrupt-controller/tcc-tic.h>
#include <zephyr/drivers/interrupt_controller/intc_tic.h>
#include <zephyr/drivers/gpio/gpio_tccvcp.h>
#include <zephyr/drivers/clock_control/clock_control_tcc_ccu.h>
#include <zephyr/drivers/clock_control/clock_control_tccvcp.h>

#include <soc.h>

#include "uart_tccvcp.h"
#include <string.h>

static struct uart_status uart[UART_CH_MAX];

static void uart_write_reg(uint8_t chan, uint32_t addr, uint32_t set_value)
{
	uint32_t base_addr, reg_addr;

	if (uart[chan].status_base == 0UL) {
		uart[chan].status_base = UART_GET_BASE(chan);
	}

	base_addr = uart[chan].status_base & 0xAFFFFFFFU;
	addr &= 0xFFFFU;
	reg_addr = base_addr + addr;
	sys_write32(set_value, reg_addr);
}

static int32_t uart_clear_gpio(uint8_t chan)
{
	uint32_t gpio_tx = 0, gpio_rx = 0;
	uint32_t gpio_clr2send = 0, gpio_req2send = 0;
	uint32_t ret, ret1, ret2;

	ret = 0;
	ret1 = 0;
	ret2 = 0;

	if (chan >= UART_CH_MAX) {
		return -EINVAL;
	}

	gpio_tx = uart[chan].status_port.bd_port_tx;
	gpio_rx = uart[chan].status_port.bd_port_rx;

	/* Reset gpio */
	ret1 = vcp_gpio_config(gpio_tx, GPIO_FUNC(0UL));
	ret2 = vcp_gpio_config(gpio_rx, GPIO_FUNC(0UL));

	if ((ret1 != 0) || (ret2 != 0)) {
		return -EIO;
	}

	if (uart[chan].status_cts_rts == TCC_ON) {
		gpio_clr2send = uart[chan].status_port.bd_port_cts;
		gpio_req2send = uart[chan].status_port.bd_port_rts;

		ret1 = vcp_gpio_config(gpio_clr2send, GPIO_FUNC(0UL));
		ret2 = vcp_gpio_config(gpio_req2send, GPIO_FUNC(0UL));

		if ((ret1 != 0) || (ret2 != 0)) {
			return -EIO;
		}
	}

	if ((ret == 0) && (chan >= UART_CH3)) {
		/* Reset MFIO Configuration */
		if (chan == UART_CH3) {
			ret = vcp_gpio_mfio_config(GPIO_MFIO_CFG_PERI_SEL0, GPIO_MFIO_DISABLE,
						   GPIO_MFIO_CFG_CH_SEL0, GPIO_MFIO_CH0);
		} else if (chan == UART_CH4) {
			ret = vcp_gpio_mfio_config(GPIO_MFIO_CFG_PERI_SEL1, GPIO_MFIO_DISABLE,
						   GPIO_MFIO_CFG_CH_SEL1, GPIO_MFIO_CH0);
		} else if (chan == UART_CH5) {
			ret = vcp_gpio_mfio_config(GPIO_MFIO_CFG_PERI_SEL2, GPIO_MFIO_DISABLE,
						   GPIO_MFIO_CFG_CH_SEL2, GPIO_MFIO_CH0);
		} else {
			return ret;
		}
	}

	return ret;
}

static int32_t uart_reset(uint8_t chan)
{
	uint32_t ret;
	int32_t clk_ret, clk_bus_id;

	ret = 0;
	clk_bus_id = (int32_t)CLOCK_IOBUS_UART0 + (int32_t)chan;

	/* SW reset */
	clk_ret = clock_set_sw_reset(clk_bus_id, true);

	if (clk_ret != (int32_t)NULL) {
		return -EIO;
	}

	/* Bit Clear */
	clk_ret = clock_set_sw_reset(clk_bus_id, false);

	if (clk_ret != (int32_t)NULL) {
		return -EIO;
	}

	return ret;
}

static void uart_close(uint8_t chan)
{
	int32_t clk_bus_id;
	uint32_t ret;

	if (chan < UART_CH_MAX) {
		/* Disable the UART controller Bus clock */
		clk_bus_id = (int32_t)CLOCK_IOBUS_UART0 + (int32_t)chan;
		clock_set_iobus_pwdn(clk_bus_id, true);

		ret = uart_clear_gpio(chan);

		/* Disable the UART ch */
		sys_write32((uint32_t)NULL, MCU_BSP_UART_BASE + (0x10000UL * (chan)) + UART_REG_CR);

		/* Initialize UART Structure */
		memset(&uart[chan], 0, sizeof(struct uart_status));

		/* UART SW Reset */
		uart_reset(chan);
	}
}

static void uart_status_init(uint8_t chan)
{
	uart[chan].status_is_probed = TCC_OFF;
	uart[chan].status_base = UART_GET_BASE(chan);
	uart[chan].status_chan = chan;
	uart[chan].status_op_mode = UART_POLLING_MODE;
	uart[chan].status_cts_rts = 0;
	uart[chan].status_word_len = WORD_LEN_5;
	uart[chan].status_2stop_bit = 0;
	uart[chan].status_parity = PARITY_SPACE;

	/* Interrupt mode init */
	uart[chan].status_tx_intr.irq_data_xmit_buf = NULL;
	uart[chan].status_tx_intr.irq_data_head = -1;
	uart[chan].status_tx_intr.irq_data_tail = -1;
	uart[chan].status_tx_intr.irq_data_size = 0;
	uart[chan].status_rx_intr.irq_data_xmit_buf = NULL;
	uart[chan].status_rx_intr.irq_data_head = -1;
	uart[chan].status_rx_intr.irq_data_tail = -1;
	uart[chan].status_rx_intr.irq_data_size = 0;
}

static int32_t uart_set_gpio(uint8_t chan, const struct uart_board_port *port_info)
{
	int32_t ret;
	uint32_t ret1, ret2, ret3, ret4;
	uint32_t ret_cfg;

	ret = 0;
	ret_cfg = 1;

	if (port_info != TCC_NULL_PTR) {
		/* set port controller, channel */
		switch (chan) {
		case UART_CH0:
			ret_cfg = vcp_gpio_peri_chan_sel(GPIO_PERICH_SEL_UARTSEL_0,
							 port_info->bd_port_ch);
			break;
		case UART_CH1:
			ret_cfg = vcp_gpio_peri_chan_sel(GPIO_PERICH_SEL_UARTSEL_1,
							 port_info->bd_port_ch);
			break;
		case UART_CH2:
			ret_cfg = vcp_gpio_peri_chan_sel(GPIO_PERICH_SEL_UARTSEL_2,
							 port_info->bd_port_ch);
			break;
		case UART_CH3:
			ret_cfg =
				vcp_gpio_mfio_config(GPIO_MFIO_CFG_PERI_SEL0, GPIO_MFIO_UART3,
						     GPIO_MFIO_CFG_CH_SEL0, port_info->bd_port_ch);
			break;
		case UART_CH4:
			ret_cfg =
				vcp_gpio_mfio_config(GPIO_MFIO_CFG_PERI_SEL1, GPIO_MFIO_UART4,
						     GPIO_MFIO_CFG_CH_SEL1, port_info->bd_port_ch);
			break;
		case UART_CH5:
			ret_cfg =
				vcp_gpio_mfio_config(GPIO_MFIO_CFG_PERI_SEL2, GPIO_MFIO_UART5,
						     GPIO_MFIO_CFG_CH_SEL2, port_info->bd_port_ch);
			break;
		default:
			ret_cfg = -EINVAL;
			break;
		}

		if (ret_cfg == 0) {
			/* set debug port */
			ret1 = vcp_gpio_config(port_info->bd_port_tx,
					       (port_info->bd_port_fs)); /* TX */
			ret2 = vcp_gpio_config(port_info->bd_port_rx,
					       (port_info->bd_port_fs | VCP_GPIO_INPUT |
						GPIO_INPUTBUF_EN)); /* RX */

			uart[chan].status_port.bd_port_cfg = port_info->bd_port_cfg;
			uart[chan].status_port.bd_port_tx = port_info->bd_port_tx;
			uart[chan].status_port.bd_port_rx = port_info->bd_port_rx;
			uart[chan].status_port.bd_port_fs = port_info->bd_port_fs;

			if (uart[chan].status_cts_rts != 0UL) {
				ret3 = vcp_gpio_config(port_info->bd_port_rts,
						       port_info->bd_port_fs); /* RTS */
				ret4 = vcp_gpio_config(port_info->bd_port_cts,
						       port_info->bd_port_fs); /* CTS */

				if ((ret1 != 0) || (ret2 != 0) || (ret3 != 0) || (ret4 != 0)) {
					ret = -EIO;
				} else {
					uart[chan].status_port.bd_port_cts = port_info->bd_port_cts;
					uart[chan].status_port.bd_port_rts = port_info->bd_port_rts;
				}
			}

			if ((uart[chan].status_cts_rts == 0) && ((ret1 != 0) || (ret2 != 0))) {
				ret = -EIO;
			}
		} else {
			ret = -EIO;
		}
	} else {
		ret = -EINVAL;
	}

	return ret;
}

static int32_t uart_set_port_config(uint8_t chan, uint32_t port)
{
	uint32_t idx;
	int32_t ret = 0;
	static const struct uart_board_port board_serial[UART_PORT_TBL_SIZE] = {
		{0UL, GPIO_GPA(28UL), GPIO_GPA(29UL), TCC_GPNONE, TCC_GPNONE, GPIO_FUNC(1UL),
		 GPIO_PERICH_CH0}, /* CTL_0, CH_0 */
		{1UL, GPIO_GPC(16UL), GPIO_GPC(17UL), GPIO_GPC(18UL), GPIO_GPC(19UL),
		 GPIO_FUNC(2UL), GPIO_PERICH_CH1}, /* CTL_0, CH_1 */

		{2UL, GPIO_GPB(8UL), GPIO_GPB(9UL), GPIO_GPB(10UL), GPIO_GPB(11UL), GPIO_FUNC(1UL),
		 GPIO_PERICH_CH0}, /* CTL_1, CH_0 */
		{3UL, GPIO_GPA(6UL), GPIO_GPA(7UL), GPIO_GPA(8UL), GPIO_GPA(9UL), GPIO_FUNC(2UL),
		 GPIO_PERICH_CH1}, /* CTL_1, CH_1 */

		{4UL, GPIO_GPB(25UL), GPIO_GPB(26UL), GPIO_GPB(27UL), GPIO_GPB(28UL),
		 GPIO_FUNC(1UL), GPIO_PERICH_CH0}, /* CTL_2, CH_0 */
		{5UL, GPIO_GPC(0UL), GPIO_GPC(1UL), GPIO_GPC(2UL), GPIO_GPC(3UL), GPIO_FUNC(2UL),
		 GPIO_PERICH_CH1}, /* CTL_2, CH_1 */

		{6UL, GPIO_GPA(16UL), GPIO_GPA(17UL), GPIO_GPA(18UL), GPIO_GPA(19UL),
		 GPIO_FUNC(3UL), GPIO_MFIO_CH0}, /* CTL_3, CH_0 */
		{7UL, GPIO_GPB(0UL), GPIO_GPB(1UL), GPIO_GPB(2UL), GPIO_GPB(3UL), GPIO_FUNC(3UL),
		 GPIO_MFIO_CH1}, /* CTL_3, CH_1 */
		{8UL, GPIO_GPC(4UL), GPIO_GPC(5UL), GPIO_GPC(6UL), GPIO_GPC(7UL), GPIO_FUNC(3UL),
		 GPIO_MFIO_CH2}, /* CTL_3, CH_2 */
		{9UL, GPIO_GPK(11UL), GPIO_GPK(12UL), GPIO_GPK(13UL), GPIO_GPK(14UL),
		 GPIO_FUNC(3UL), GPIO_MFIO_CH3}, /* CTL_3, CH_3 */

		{10UL, GPIO_GPA(20UL), GPIO_GPA(21UL), GPIO_GPA(22UL), GPIO_GPA(23UL),
		 GPIO_FUNC(3UL), GPIO_MFIO_CH0}, /* CTL_4, CH_0 */
		{11UL, GPIO_GPB(4UL), GPIO_GPB(5UL), GPIO_GPB(6UL), GPIO_GPB(7UL), GPIO_FUNC(3UL),
		 GPIO_MFIO_CH1}, /* CTL_4, CH_1 */
		{12UL, GPIO_GPC(8UL), GPIO_GPC(9UL), GPIO_GPC(10UL), GPIO_GPC(11UL), GPIO_FUNC(3UL),
		 GPIO_MFIO_CH2}, /* CTL_4, CH_2 */

		{13UL, GPIO_GPA(24UL), GPIO_GPA(25UL), GPIO_GPA(26UL), GPIO_GPA(27UL),
		 GPIO_FUNC(3UL), GPIO_MFIO_CH0}, /* CTL_5, CH_0 */
		{14UL, GPIO_GPB(8UL), GPIO_GPB(9UL), GPIO_GPB(10UL), GPIO_GPB(11UL), GPIO_FUNC(3UL),
		 GPIO_MFIO_CH1}, /* CTL_5, CH_1 */
		{15UL, GPIO_GPC(12UL), GPIO_GPC(13UL), GPIO_GPC(14UL), GPIO_GPC(15UL),
		 GPIO_FUNC(3UL), GPIO_MFIO_CH2}, /* CTL_5, CH_2 */
	};

	if ((port < UART_PORT_CFG_MAX) && (chan < UART_CH_MAX)) {
		for (idx = 0UL; idx < UART_PORT_CFG_MAX; idx++) {
			if (board_serial[idx].bd_port_cfg == port) {
				ret = uart_set_gpio(chan, &board_serial[idx]);
				break;
			}
		}
	}

	return ret;
}

static int32_t uart_set_baud_rate(uint8_t chan, uint32_t baud)
{
	uint32_t divider, mod, brd_i, brd_f, pclk;
	int32_t ret;

	if (chan >= UART_CH_MAX) {
		ret = -EINVAL;
	} else {
		/* Read the peri clock */
		pclk = clock_get_peri_rate((int32_t)CLOCK_PERI_UART0 + (int32_t)chan);

		if (pclk == 0UL) {
			ret = -EIO;
		} else {
			/* calculate integer baud rate divisor */
			divider = 16UL * baud;
			brd_i = pclk / divider;
			uart_write_reg(chan, UART_REG_IBRD, brd_i);

			/* calculate faction baud rate divisor */
			/* NOTICE : fraction maybe need sampling */
			baud &= 0xFFFFFFU;
			mod = (pclk % (16UL * baud)) & 0xFFFFFFU;
			divider = ((((1UL << 3UL) * 16UL) * mod) / (16UL * baud));
			brd_f = divider / 2UL;
			uart_write_reg(chan, UART_REG_FBRD, brd_f);
			ret = (int32_t)0;
		}
	}
	return ret;
}

static int32_t uart_set_chan_config(struct uart_param *uart_cfg)
{
	uint8_t chan;
	uint8_t word_len = (uint8_t)uart_cfg->word_length;
	uint32_t cr_data = 0, lcr_data = 0;
	int32_t ret, clk_bus_id, clk_peri_id;

	chan = uart_cfg->channel;
	/* Enable the UART controller peri clock */
	clk_bus_id = (int32_t)CLOCK_IOBUS_UART0 + (int32_t)chan;
	clock_set_iobus_pwdn(clk_bus_id, false);
	clk_peri_id = (int32_t)CLOCK_PERI_UART0 + (int32_t)chan;
	ret = clock_set_peri_rate(clk_peri_id, UART_DEBUG_CLK);
	clock_enable_peri(clk_peri_id);

	if (ret == 0) {
		uart_set_baud_rate(chan, uart_cfg->baud_rate);

		/* line control setting */
		/* Word Length */
		word_len &= 0x3U;
		uart_cfg->word_length = (enum uart_word_len)word_len;
		lcr_data |= UART_LCRH_WLEN((uint32_t)uart_cfg->word_length);

		/* Enables FIFOs */
		if (uart_cfg->fifo == ENABLE_FIFO) {
			lcr_data |= UART_LCRH_FEN;
		}

		/* Two Stop Bits */
		if (uart_cfg->stop_bit == TCC_ON) {
			lcr_data |= UART_LCRH_STP2;
		}

		/* Parity Enable */
		switch (uart_cfg->parity) {
		case PARITY_SPACE:
			lcr_data &= ~(UART_LCRH_PEN);
			break;
		case PARITY_EVEN:
			lcr_data |= ((UART_LCRH_PEN | UART_LCRH_EPS) & ~(UART_LCRH_SPS));
			break;
		case PARITY_ODD:
			lcr_data |= ((UART_LCRH_PEN & ~(UART_LCRH_EPS)) & ~(UART_LCRH_SPS));
			break;
		case PARITY_MARK:
			lcr_data |= ((UART_LCRH_PEN & ~(UART_LCRH_EPS)) | UART_LCRH_SPS);
			break;
		default:
			break;
		}

		uart_write_reg(chan, UART_REG_LCRH, lcr_data);

		/* control register setting */
		cr_data = UART_CR_EN;
		cr_data |= UART_CR_TXE;
		cr_data |= UART_CR_RXE;

		if (uart[chan].status_cts_rts != 0UL) { /* brace */
			cr_data |= (UART_CR_RTSEN | UART_CR_CTSEN);
		}

		uart_write_reg(chan, UART_REG_CR, cr_data);
	}

	return ret;
}

static uint32_t uart_read_reg(uint8_t chan, uint32_t addr)
{
	uint32_t ret, base_addr, reg_addr;

	ret = 0;

	if (uart[chan].status_base == 0UL) {
		uart[chan].status_base = UART_GET_BASE(chan);
	}

	base_addr = uart[chan].status_base & 0xAFFFFFFFU;
	addr &= 0xFFFFU;
	reg_addr = base_addr + addr;
	ret = sys_read32(reg_addr);

	return ret;
}

static int32_t uart_probe(struct uart_param *uart_cfg)
{
	uint8_t chan;
	int32_t ret = -1;

	chan = uart_cfg->channel;

	if ((chan < UART_CH_MAX) && (uart[chan].status_is_probed == TCC_OFF)) {
		uart[chan].status_op_mode = uart_cfg->mode;
		uart[chan].status_cts_rts = uart_cfg->cts_rts;

		/* Set port config */
		ret = uart_set_port_config(chan, uart_cfg->port_cfg);

		if (ret == 0) {
			ret = uart_set_chan_config(uart_cfg);

			if (ret == 0) {
				uart[chan].status_is_probed = TCC_ON;
			}
		}
	}

	return ret;
}

static int32_t uart_open(struct uart_param *uart_cfg)
{
	uint8_t chan = uart_cfg->channel;
	int32_t ret = -EINVAL;

	uart_status_init(chan);

	if (uart_cfg->port_cfg <= UART_PORT_CFG_MAX) {
		ret = uart_probe(uart_cfg);
	}

	return ret;
}

static int uart_tccvcp_init(const struct device *dev)
{
	struct uart_param uart_pars;
	uint8_t uart_port;

	uart_port = ((UART_BASE_ADDR - MCU_BSP_UART_BASE) / 0x10000);

	uart_pars.channel = uart_port;
	uart_pars.priority = TIC_PRIORITY_NO_MEAN;
	uart_pars.baud_rate = 115200;
	uart_pars.mode = UART_POLLING_MODE;
	uart_pars.cts_rts = UART_CTSRTS_OFF;
	uart_pars.port_cfg = (uint8_t)(3U + uart_port);
	uart_pars.fifo = DISABLE_FIFO, uart_pars.stop_bit = TWO_STOP_BIT_OFF;
	uart_pars.word_length = WORD_LEN_8;
	uart_pars.parity = PARITY_SPACE;
	uart_pars.callback_fn = TCC_NULL_PTR;

	uart_close(uart_pars.channel);
	uart_open(&uart_pars);

	return 0;
}

int uart_tccvcp_poll_in(const struct device *dev, unsigned char *c)
{
	uint8_t chan;
	uint32_t data, repeat = 0;

	chan = (uint8_t)((UART_BASE_ADDR - 0xA0200000) / 0x10000);

	if (chan >= UART_CH_MAX) {
		return -EINVAL;
	}

	while ((uart_read_reg(chan, UART_REG_FR) & UART_FR_RXFE) != 0UL) {
		if ((uart_read_reg(chan, UART_REG_FR) & UART_FR_RXFE) == 0UL) {
			break;
		}
		repeat++;
		if (repeat > 100) {
			return -EIO;
		}
	}

	data = uart_read_reg(chan, UART_REG_DR);
	*c = (unsigned char)(data & 0xFFUL);

	return 0;
}

void uart_tccvcp_poll_out(const struct device *dev, unsigned char c)
{
	uint8_t chan;
	uint32_t repeat = 0;

	chan = (uint8_t)((UART_BASE_ADDR - 0xA0200000) / 0x10000);

	if (chan >= UART_CH_MAX) {
		return;
	}

	while ((uart_read_reg(chan, UART_REG_FR) & UART_FR_TXFF) != 0UL) {
		if ((uart_read_reg(chan, UART_REG_FR) & UART_FR_TXFF) == 0UL) {
			break;
		}
		repeat++;
		if (repeat > 100) {
			return;
		}
	}
	uart_write_reg(chan, UART_REG_DR, c);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_tccvcp_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_tccvcp_dev_config *dev_cfg = dev->config;
	uint8_t chan;
	struct uart_param uart_cfg;
	int32_t ret;

	chan = (uint8_t)((UART_BASE_ADDR - 0xA0200000) / 0x10000);

	uart_cfg.channel = chan;
	uart_cfg.priority = TIC_PRIORITY_NO_MEAN;
	uart_cfg.baud_rate = 115200;
	uart_cfg.mode = UART_POLLING_MODE;
	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uart_cfg.cts_rts = UART_CTSRTS_OFF;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uart_cfg.cts_rts = UART_CTSRTS_ON;
		break;
	default:
		uart_cfg.cts_rts = UART_CTSRTS_OFF;
		break;
	}

	uart_cfg.port_cfg = (uint8_t)(4U + dev_cfg->channel);

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_8:
		uart_cfg.word_length = WORD_LEN_8;
		break;
	case UART_CFG_DATA_BITS_7:
		uart_cfg.word_length = WORD_LEN_7;
		break;
	case UART_CFG_DATA_BITS_6:
		uart_cfg.word_length = WORD_LEN_6;
		break;
	case UART_CFG_DATA_BITS_5:
		uart_cfg.word_length = WORD_LEN_5;
		break;
	default:
		uart_cfg.word_length = WORD_LEN_8 + 1;
		break;
	}

	uart_cfg.fifo = DISABLE_FIFO;

	if (cfg->stop_bits == UART_CFG_STOP_BITS_2) {
		uart_cfg.stop_bit = TWO_STOP_BIT_ON;
	} else {
		uart_cfg.stop_bit = TWO_STOP_BIT_OFF;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_EVEN:
		uart_cfg.parity = PARITY_EVEN;
		break;
	case UART_CFG_PARITY_ODD:
		uart_cfg.parity = PARITY_ODD;
		break;
	case UART_CFG_PARITY_SPACE:
		uart_cfg.parity = PARITY_SPACE;
		break;
	case UART_CFG_PARITY_MARK:
		uart_cfg.parity = PARITY_MARK;
		break;
	default:
		uart_cfg.parity = PARITY_MARK + 1;
		break;
	}

	uart_cfg.callback_fn = TCC_NULL_PTR;

	ret = uart_set_chan_config(&uart_cfg);
	if (ret == 0) {
		uart[chan].status_cts_rts = uart_cfg.cts_rts;
		uart[chan].status_2stop_bit = uart_cfg.stop_bit;
		uart[chan].status_parity = uart_cfg.parity;
		uart[chan].status_word_len = uart_cfg.word_length;
		uart[chan].baudrate = uart_cfg.baud_rate;
	}

	return ret;
};

static int uart_tccvcp_config_get(const struct device *dev, struct uart_config *cfg)
{
	uint8_t chan;

	chan = (uint8_t)((UART_BASE_ADDR - MCU_BSP_UART_BASE) / 0x10000);
	cfg->baudrate = uart[chan].baudrate;

	if (uart[chan].status_cts_rts == UART_CTSRTS_ON) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	} else {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	}

	switch (uart[chan].status_word_len) {
	case WORD_LEN_8:
		cfg->data_bits = UART_CFG_DATA_BITS_8;
		break;
	case WORD_LEN_7:
		cfg->data_bits = UART_CFG_DATA_BITS_7;
		break;
	case WORD_LEN_6:
		cfg->data_bits = UART_CFG_DATA_BITS_6;
		break;
	case WORD_LEN_5:
		cfg->data_bits = UART_CFG_DATA_BITS_5;
		break;
	default:
		cfg->data_bits = UART_CFG_DATA_BITS_9;
		break;
	}

	if (uart[chan].status_2stop_bit == TWO_STOP_BIT_ON) {
		cfg->stop_bits = UART_CFG_STOP_BITS_2;
	} else {
		cfg->stop_bits = UART_CFG_STOP_BITS_0_5;
	}

	switch (uart[chan].status_parity) {
	case PARITY_EVEN:
		cfg->parity = UART_CFG_PARITY_EVEN;
		break;
	case PARITY_ODD:
		cfg->parity = UART_CFG_PARITY_ODD;
		break;
	case PARITY_SPACE:
		cfg->parity = UART_CFG_PARITY_SPACE;
		break;
	case PARITY_MARK:
		cfg->parity = UART_CFG_PARITY_MARK;
		break;
	default:
		cfg->parity = UART_CFG_PARITY_NONE;
		break;
	}

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static const struct uart_driver_api uart_tccvcp_driver_api = {
	.poll_in = uart_tccvcp_poll_in,
	.poll_out = uart_tccvcp_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_tccvcp_configure,
	.config_get = uart_tccvcp_config_get,
#endif
};

#define UART_TCC_VCP_IRQ_CONF_FUNC_SET(port)
#define UART_TCC_VCP_IRQ_CONF_FUNC(port)

#define UART_TCC_VCP_DEV_DATA(port) static struct uart_tccvcp_dev_data_t uart_tccvcp_dev_data_##port

#if CONFIG_PINCTRL
#define UART_TCC_VCP_PINCTRL_DEFINE(port) PINCTRL_DT_INST_DEFINE(port);
#define UART_TCC_VCP_PINCTRL_INIT(port)   .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(port),
#else
#define UART_TCC_VCP_PINCTRL_DEFINE(port)
#define UART_TCC_VCP_PINCTRL_INIT(port)
#endif /* CONFIG_PINCTRL */

#define UART_TCC_VCP_DEV_CFG(port)                                                                 \
	static struct uart_tccvcp_dev_config uart_tccvcp_dev_cfg_##port = {                        \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(port)),                                           \
		.sys_clk_freq = DT_INST_PROP(port, clock_frequency),                               \
		.baud_rate = DT_INST_PROP(port, current_speed), .channel = port,                   \
		UART_TCC_VCP_IRQ_CONF_FUNC_SET(port) UART_TCC_VCP_PINCTRL_INIT(port)}

#define UART_TCC_VCP_INIT(port)                                                                    \
	DEVICE_DT_INST_DEFINE(port, uart_tccvcp_init, NULL, &uart_tccvcp_dev_data_##port,          \
			      &uart_tccvcp_dev_cfg_##port, PRE_KERNEL_1,                           \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_tccvcp_driver_api)

#define UART_TCC_INSTANTIATE(inst)                                                                 \
	UART_TCC_VCP_PINCTRL_DEFINE(inst)                                                          \
	UART_TCC_VCP_IRQ_CONF_FUNC(inst);                                                          \
	UART_TCC_VCP_DEV_DATA(inst);                                                               \
	UART_TCC_VCP_DEV_CFG(inst);                                                                \
	UART_TCC_VCP_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(UART_TCC_INSTANTIATE)
