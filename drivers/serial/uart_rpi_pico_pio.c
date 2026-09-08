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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_rpi_pico_pio, CONFIG_UART_LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_uart_pio

#define CYCLES_PER_BIT 8
#define SIDESET_BIT_COUNT 2

/*
 * Both PIO UART FIFO interrupt sources are level sensitive and stay asserted
 * until the FIFO is drained/filled, so it does not matter which of the PIO
 * block's two NVIC lines they are routed through. Everything goes through
 * line 0; line 1 is wired by the PIO parent but otherwise unused for now.
 */
#define PIO_UART_IRQ_INDEX 0

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
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
	uint32_t rx_source_mask;
	uint32_t tx_source_mask;
	bool tx_irq_en;
	bool rx_irq_en;
#endif
};

RPI_PICO_PIO_DEFINE_PROGRAM(uart_tx, 0, 3,
		/* .wrap_target */
	0x9fa0, /*  0: pull   block           side 1 [7]  */
	0xf727, /*  1: set    x, 7            side 0 [7]  */
	0x6001, /*  2: out    pins, 1                     */
	0x0642, /*  3: jmp    x--, 2                 [6]  */
		/* .wrap */
);

RPI_PICO_PIO_DEFINE_PROGRAM(uart_rx, 1, 8,
	0x20a0, /*  0: wait   1 pin, 0                    */
		/*  .wrap_target */
	0x2020, /*  1: wait   0 pin, 0                    */
	0xea27, /*  2: set    x, 7                   [10] */
	0x4001, /*  3: in     pins, 1                     */
	0x0643, /*  4: jmp    x--, 3                 [6]  */
	0x00c8, /*  5: jmp    pin, 8                      */
	0xc014, /*  6: irq    nowait 4 rel                */
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

/*
 * The RX program shifts right with no autopush, so the received byte lands
 * in the most significant byte of the 4-byte-wide FIFO word. Accessing it
 * pops the word.
 */
static inline uint8_t pio_uart_rx_fifo_get(PIO pio, size_t sm)
{
	io_rw_8 *uart_rx_fifo_msb = (io_rw_8 *)&pio->rxf[sm] + 3;

	return *uart_rx_fifo_msb;
}

static int pio_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct pio_uart_config *config = dev->config;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);
	struct pio_uart_data *data = dev->data;

	if (pio_sm_is_rx_fifo_empty(pio, data->rx_sm)) {
		return -1;
	}

	*c = pio_uart_rx_fifo_get(pio, data->rx_sm);
	return 0;
}

static void pio_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;

	pio_sm_put_blocking(pio_rpi_pico_get_pio(config->piodev), data->tx_sm, (uint32_t)c);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int pio_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);
	int i;

	for (i = 0; i < len && !pio_sm_is_tx_fifo_full(pio, data->tx_sm); i++) {
		pio->txf[data->tx_sm] = tx_data[i];
	}

	/*
	 * TXSTALL is sticky: clear it here so irq_tx_complete() only reports
	 * completion for frames pushed at or after this fill, not a stale
	 * stall left over from a previous, already-drained fill.
	 */
	pio->fdebug = BIT(PIO_FDEBUG_TXSTALL_LSB + data->tx_sm);

	return i;
}

static int pio_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);
	int i;

	for (i = 0; i < size && !pio_sm_is_rx_fifo_empty(pio, data->rx_sm); i++) {
		rx_data[i] = pio_uart_rx_fifo_get(pio, data->rx_sm);
	}

	return i;
}

static void pio_uart_irq_tx_enable(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;

	data->tx_irq_en = true;
	pio_rpi_pico_irq_sources_set(config->piodev, PIO_UART_IRQ_INDEX, data->tx_source_mask,
				     true);
}

static void pio_uart_irq_tx_disable(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;

	data->tx_irq_en = false;
	pio_rpi_pico_irq_sources_set(config->piodev, PIO_UART_IRQ_INDEX, data->tx_source_mask,
				     false);
}

static int pio_uart_irq_tx_ready(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	return data->tx_irq_en && !pio_sm_is_tx_fifo_full(pio, data->tx_sm);
}

static int pio_uart_irq_tx_complete(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	/*
	 * FIFO empty alone is not enough: the last frame can still be
	 * shifting out of the OSR (up to ~10 bit-times at 8 cycles/bit).
	 * TXSTALL only latches once the state machine has actually stalled
	 * for lack of more data.
	 */
	return pio_sm_is_tx_fifo_empty(pio, data->tx_sm) &&
	      (pio->fdebug & BIT(PIO_FDEBUG_TXSTALL_LSB + data->tx_sm));
}

static void pio_uart_irq_rx_enable(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;

	data->rx_irq_en = true;
	pio_rpi_pico_irq_sources_set(config->piodev, PIO_UART_IRQ_INDEX, data->rx_source_mask,
				     true);
}

static void pio_uart_irq_rx_disable(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;

	data->rx_irq_en = false;
	pio_rpi_pico_irq_sources_set(config->piodev, PIO_UART_IRQ_INDEX, data->rx_source_mask,
				     false);
}

static int pio_uart_irq_rx_ready(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	return data->rx_irq_en && !pio_sm_is_rx_fifo_empty(pio, data->rx_sm);
}

static int pio_uart_irq_is_pending(const struct device *dev)
{
	return pio_uart_irq_tx_ready(dev) || pio_uart_irq_rx_ready(dev);
}

static void pio_uart_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void pio_uart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct pio_uart_data *data = dev->data;

	data->cb = cb;
	data->cb_data = cb_data;
}

/*
 * Both FIFO sources are level sensitive and stay asserted until the
 * callback drains/fills, so no acknowledge is needed here.
 */
static void pio_uart_irq_handler(const struct device *piodev, uint32_t ints, void *user_data)
{
	const struct device *dev = user_data;
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;

	ARG_UNUSED(piodev);

	if (data->cb == NULL) {
		uint32_t pending = ints & (data->tx_source_mask | data->rx_source_mask);

		/*
		 * Nothing else drains or fills the FIFO, so an enabled source
		 * with no callback installed would re-enter this handler for
		 * ever. Mask it; irq_tx_enable()/irq_rx_enable() unmask again.
		 */
		pio_rpi_pico_irq_sources_set(config->piodev, PIO_UART_IRQ_INDEX, pending, false);

		if ((pending & data->tx_source_mask) != 0) {
			data->tx_irq_en = false;
		}

		if ((pending & data->rx_source_mask) != 0) {
			data->rx_irq_en = false;
		}

		return;
	}

	data->cb(dev, data->cb_data);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	data->rx_source_mask = BIT(PIO_INTR_SM0_RXNEMPTY_LSB + rx_sm);
	data->tx_source_mask = BIT(PIO_INTR_SM0_TXNFULL_LSB + tx_sm);

	retval = pio_rpi_pico_register_irq(config->piodev, PIO_UART_IRQ_INDEX,
					   data->rx_source_mask | data->tx_source_mask,
					   pio_uart_irq_handler, (void *)dev);
	if (retval < 0) {
		LOG_ERR("failed to register the PIO interrupt handler: %d", retval);
		return retval;
	}
#endif

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

static DEVICE_API(uart, pio_uart_driver_api) = {
	.poll_in = pio_uart_poll_in,
	.poll_out = pio_uart_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = pio_uart_fifo_fill,
	.fifo_read = pio_uart_fifo_read,
	.irq_tx_enable = pio_uart_irq_tx_enable,
	.irq_tx_disable = pio_uart_irq_tx_disable,
	.irq_tx_ready = pio_uart_irq_tx_ready,
	.irq_tx_complete = pio_uart_irq_tx_complete,
	.irq_rx_enable = pio_uart_irq_rx_enable,
	.irq_rx_disable = pio_uart_irq_rx_disable,
	.irq_rx_ready = pio_uart_irq_rx_ready,
	.irq_is_pending = pio_uart_irq_is_pending,
	.irq_update = pio_uart_irq_update,
	.irq_callback_set = pio_uart_irq_callback_set,
#endif
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
