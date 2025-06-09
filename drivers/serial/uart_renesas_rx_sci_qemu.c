/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_uart_sci_qemu

#include <zephyr/drivers/uart.h>
#include <soc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(renesas_rx_uart_sci_qemu, CONFIG_UART_LOG_LEVEL);

#define REG_MASK(reg) (BIT_MASK(_CONCAT(reg, _LEN)) << _CONCAT(reg, _POS))

/* Registers */
#define SMR 0x00 /*!< Serial Mode Register             */
#define BRR 0x01 /*!< Bit Rate Register                */
#define SCR 0x02 /*!< Serial Control Register          */
#define TDR 0x03 /*!< Transmit Data Register           */
#define SSR 0x04 /*!< Serial Status Register           */
#define RDR 0x05 /*!< Receive Data Register            */

/**
 * SSR (Serial Status Register)
 *
 * - MPBT[0..1]: Multi-Processor Bit Transfer
 * - MPB[1..2]:  Multi-Processor
 * - TEND[2..3]: Transmit End Flag
 * - PER[3..4]:  Parity Error Flag
 * - FER[4..5]:  Framing Error Flag
 * - ORER[5..6]: Overrun Error Flag
 * - RDRF[6..7]: Receive Data Full Flag
 * - TDRE[7..8]: Transmit Data Empty Flag
 */
#define SSR_MPBT_POS (0)
#define SSR_MPBT_LEN (1)
#define SSR_MPB_POS  (1)
#define SSR_MPB_LEN  (1)
#define SSR_TEND_POS (2)
#define SSR_TEND_LEN (1)
#define SSR_PER_POS  (3)
#define SSR_PER_LEN  (1)
#define SSR_FER_POS  (4)
#define SSR_FER_LEN  (1)
#define SSR_ORER_POS (5)
#define SSR_ORER_LEN (1)
#define SSR_RDRF_POS (6)
#define SSR_RDRF_LEN (1)
#define SSR_TDRE_POS (7)
#define SSR_TDRE_LEN (1)

struct uart_renesas_rx_sci_qemu_cfg {
	mem_addr_t regs;
};

struct uart_renesas_rx_sci_qemu_data {
	const struct device *dev;
	struct uart_config uart_config;
};

static uint8_t uart_renesas_rx_qemu_read_8(const struct device *dev, uint32_t offs)
{
	const struct uart_renesas_rx_sci_qemu_cfg *config = dev->config;

	return sys_read8(config->regs + offs);
}

static void uart_renesas_rx_qemu_write_8(const struct device *dev, uint32_t offs, uint8_t value)
{
	const struct uart_renesas_rx_sci_qemu_cfg *config = dev->config;

	sys_write8(value, config->regs + offs);
}

static int uart_renesas_rx_sci_qemu_poll_in(const struct device *dev, unsigned char *c)
{
	if ((uart_renesas_rx_qemu_read_8(dev, SSR) & REG_MASK(SSR_RDRF)) == 0) {
		/* There are no characters available to read. */
		return -1;
	}

	*c = uart_renesas_rx_qemu_read_8(dev, RDR);

	return 0;
}

static void uart_renesas_rx_sci_qemu_poll_out(const struct device *dev, unsigned char c)
{
	while (!(uart_renesas_rx_qemu_read_8(dev, SSR) & REG_MASK(SSR_TEND))) {
	}

	uart_renesas_rx_qemu_write_8(dev, TDR, c);
}

static const struct uart_driver_api uart_rx_driver_api = {
	.poll_in = uart_renesas_rx_sci_qemu_poll_in,
	.poll_out = uart_renesas_rx_sci_qemu_poll_out,
};

/* Device Instantiation */
#define UART_RENESAS_RX_SCI_QEMU_CFG_INIT(n)                                                       \
	static const struct uart_renesas_rx_sci_qemu_cfg uart_rx_sci_cfg_##n = {                   \
		.regs = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
	}

#define UART_RENESAS_RX_SCI_QEMU_INIT(n)                                                           \
	UART_RENESAS_RX_SCI_QEMU_CFG_INIT(n);                                                      \
                                                                                                   \
	static struct uart_renesas_rx_sci_qemu_data uart_rx_sci_data_##n = {                       \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(n, current_speed),                        \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &uart_rx_sci_data_##n, &uart_rx_sci_cfg_##n,          \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RENESAS_RX_SCI_QEMU_INIT)
