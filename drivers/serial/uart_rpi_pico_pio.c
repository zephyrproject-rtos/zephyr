/*
 * Copyright (c) 2022, Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>

#include <hardware/pio.h>
#include <hardware/clocks.h>

#define DT_DRV_COMPAT raspberrypi_pico_uart_pio

#define CYCLES_PER_BIT 8
#define SIDESET_BIT_COUNT 2

struct pio_uart_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pcfg;
	const uint32_t tx_pin;
	const uint32_t rx_pin;
	uint32_t baudrate;
};

struct pio_uart_data {
	size_t tx_sm;
	size_t rx_sm;
};

RPI_PICO_PIO_DEFINE_PROGRAM(uart_tx, 0, 3,
		/* .wrap_target */
	0x9fa0, /*  0: pull   block           side 1 [7]  */
	0xf727, /*  1: set    x, 7            side 0 [7]  */
	0x6001, /*  2: out    pins, 1                     */
	0x0642, /*  3: jmp    x--, 2                 [6]  */
		/* .wrap */
);

RPI_PICO_PIO_DEFINE_PROGRAM(uart_rx, 0, 8,
		/*  .wrap_target */
	0x2020, /*  0: wait   0 pin, 0                    */
	0xea27, /*  1: set    x, 7                   [10] */
	0x4001, /*  2: in     pins, 1                     */
	0x0642, /*  3: jmp    x--, 2                 [6]  */
	0x00c8, /*  4: jmp    pin, 8                      */
	0xc014, /*  5: irq    nowait 4 rel                */
	0x20a0, /*  6: wait   1 pin, 0                    */
	0x0000, /*  7: jmp    0                           */
	0x8020, /*  8: push   block                       */
		/*  .wrap */
);

static int pio_uart_tx_init(PIO pio, uint32_t sm, uint32_t tx_pin, float div)
{
	uint32_t offset;
	pio_sm_config sm_config;

	if (!pio_can_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(uart_tx))) {
		return -EBUSY;
	}

	offset = pio_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(uart_tx));
	sm_config = pio_get_default_sm_config();

	sm_config_set_sideset(&sm_config, SIDESET_BIT_COUNT, true, false);
	sm_config_set_out_shift(&sm_config, true, false, 0);
	sm_config_set_out_pins(&sm_config, tx_pin, 1);
	sm_config_set_sideset_pins(&sm_config, tx_pin);
	sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
	sm_config_set_clkdiv(&sm_config, div);
	sm_config_set_wrap(&sm_config,
			   offset + RPI_PICO_PIO_GET_WRAP_TARGET(uart_tx),
			   offset + RPI_PICO_PIO_GET_WRAP(uart_tx));

	pio_sm_set_pins_with_mask(pio, sm, BIT(tx_pin), BIT(tx_pin));
	pio_sm_set_pindirs_with_mask(pio, sm, BIT(tx_pin), BIT(tx_pin));
	pio_sm_init(pio, sm, offset, &sm_config);
	pio_sm_set_enabled(pio, sm, true);

	return 0;
}

static int pio_uart_rx_init(PIO pio, uint32_t sm, uint32_t rx_pin, float div)
{
	pio_sm_config sm_config;
	uint32_t offset;

	if (!pio_can_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(uart_rx))) {
		return -EBUSY;
	}

	offset = pio_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(uart_rx));
	sm_config = pio_get_default_sm_config();

	pio_sm_set_consecutive_pindirs(pio, sm, rx_pin, 1, false);
	sm_config_set_in_pins(&sm_config, rx_pin);
	sm_config_set_jmp_pin(&sm_config, rx_pin);
	sm_config_set_in_shift(&sm_config, true, false, 0);
	sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_RX);
	sm_config_set_clkdiv(&sm_config, div);
	sm_config_set_wrap(&sm_config,
			   offset + RPI_PICO_PIO_GET_WRAP_TARGET(uart_rx),
			   offset + RPI_PICO_PIO_GET_WRAP(uart_rx));

	pio_sm_init(pio, sm, offset, &sm_config);
	pio_sm_set_enabled(pio, sm, true);

	return 0;
}

static int pio_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct pio_uart_config *config = dev->config;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);
	struct pio_uart_data *data = dev->data;
	io_rw_8 *uart_rx_fifo_msb;

	/*
	 * The rx FIFO is 4 bytes wide, add 3 to get the most significant
	 * byte.
	 */
	uart_rx_fifo_msb = (io_rw_8 *)&pio->rxf[data->rx_sm] + 3;
	if (pio_sm_is_rx_fifo_empty(pio, data->rx_sm)) {
		return -1;
	}

	/* Accessing the FIFO pops the read word from it */
	*c = (char)*uart_rx_fifo_msb;
	return 0;
}

static void pio_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;

	pio_sm_put_blocking(pio_rpi_pico_get_pio(config->piodev), data->tx_sm, (uint32_t)c);
}

static int pio_uart_init(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	float sm_clock_div;
	size_t tx_sm;
	size_t rx_sm;
	int retval;
	PIO pio;

	pio = pio_rpi_pico_get_pio(config->piodev);
	sm_clock_div = (float)clock_get_hz(clk_sys) / (CYCLES_PER_BIT * config->baudrate);

	retval = pio_rpi_pico_allocate_sm(config->piodev, &tx_sm);
	retval |= pio_rpi_pico_allocate_sm(config->piodev, &rx_sm);

	if (retval < 0) {
		return retval;
	}

	data->tx_sm = tx_sm;
	data->rx_sm = rx_sm;

	retval = pio_uart_tx_init(pio, tx_sm, config->tx_pin, sm_clock_div);
	if (retval < 0) {
		return retval;
	}

	retval = pio_uart_rx_init(pio, rx_sm, config->rx_pin, sm_clock_div);
	if (retval < 0) {
		return retval;
	}

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

static const struct uart_driver_api pio_uart_driver_api = {
	.poll_in = pio_uart_poll_in,
	.poll_out = pio_uart_poll_out,
};

#define PIO_UART_INIT(idx)									\
	PINCTRL_DT_INST_DEFINE(idx);								\
	static const struct pio_uart_config pio_uart##idx##_config = {				\
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(idx)),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),					\
		.tx_pin = DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, tx_pins, 0),	\
		.rx_pin = DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, rx_pins, 0),	\
		.baudrate = DT_INST_PROP(idx, current_speed),					\
	};											\
	static struct pio_uart_data pio_uart##idx##_data;					\
												\
	DEVICE_DT_INST_DEFINE(idx, pio_uart_init, NULL, &pio_uart##idx##_data,			\
			      &pio_uart##idx##_config, POST_KERNEL,				\
			      CONFIG_SERIAL_INIT_PRIORITY,					\
			      &pio_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PIO_UART_INIT)
