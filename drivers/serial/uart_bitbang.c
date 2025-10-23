/*
 * Copyright (c) 2025 Joel Guittet
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uart_bitbang

#define LOG_LEVEL CONFIG_UART_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_bitbang);

#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

enum uart_bitbang_state {
	UART_BITBANG_IDLE,
	UART_BITBANG_START_BIT,
	UART_BITBANG_DATA,
	UART_BITBANG_PARITY,
	UART_BITBANG_STOP_BIT_1,
	UART_BITBANG_STOP_BIT_2,
	UART_BITBANG_COMPLETE,
};

struct uart_bitbang_config {
	/* GPIOs */
	struct gpio_dt_spec tx_gpio;
	struct gpio_dt_spec rx_gpio;
	struct gpio_dt_spec de_gpio;
	/* Counters */
	const struct device *tx_counter;
	const struct device *rx_counter;
	/* UART config */
	struct uart_config *uart_cfg;
	/* MSB first */
	bool msb;
};

struct uart_bitbang_data {
	/* Configuration */
	struct uart_bitbang_config *config;
	/* Error flags */
	int err;
	/* Tx state */
	enum uart_bitbang_state tx_state;
	/* Tx bit index */
	int tx_index;
	/* Tx data */
	uint16_t *tx_data;
	/* Tx parity */
	int tx_parity;
	/* Tx counter config */
	struct counter_top_cfg tx_counter_cfg;
	/* Tx ring buffer */
	struct ring_buf *tx_ringbuf;
	/* Rx state */
	enum uart_bitbang_state rx_state;
	/* Rx bit index */
	int rx_index;
	/* Rx data */
	uint16_t rx_data;
	/* Rx parity */
	int rx_parity;
	/* Rx counter config */
	struct counter_top_cfg rx_counter_cfg;
	/* Rx callback data */
	struct gpio_callback rx_gpio_cb_data;
	/* Rx ring buffer */
	struct ring_buf *rx_ringbuf;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Interrupt flags */
#define UART_BITBANG_IRQ_TC   (1 << 0)
#define UART_BITBANG_IRQ_RXNE (1 << 1)
#define UART_BITBANG_IRQ_PE   (1 << 2)
	int irq;
	/* User callback and data */
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
};

static int uart_bitbang_init(const struct device *dev);

static inline uint8_t uart_bitbang_data_bits_to_len(uint8_t data_bits)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		return 5;
	case UART_CFG_DATA_BITS_6:
		return 6;
	case UART_CFG_DATA_BITS_7:
		return 7;
	case UART_CFG_DATA_BITS_8:
		return 8;
	case UART_CFG_DATA_BITS_9:
		return 9;
	}
	return 0;
}

static int uart_bitbang_compute_parity(const struct device *dev, uint16_t data_u16)
{
	const struct uart_bitbang_config *config = dev->config;
	uint8_t len = uart_bitbang_data_bits_to_len(config->uart_cfg->data_bits);
	int parity = 0, index;

	if (config->uart_cfg->parity != UART_CFG_PARITY_NONE) {
		for (index = 0; index < len; index++) {
			const int shift = config->msb ? (len - 1 - index) : index;
			const int d = (data_u16 >> shift) & 0x1;

			parity += d;
		}
		if (config->uart_cfg->parity == UART_CFG_PARITY_ODD) {
			parity = (parity % 2) ? 0 : 1;
		} else if (config->uart_cfg->parity == UART_CFG_PARITY_EVEN) {
			parity = (parity % 2) ? 1 : 0;
		} else if (config->uart_cfg->parity == UART_CFG_PARITY_MARK) {
			parity = 1;
		} else if (config->uart_cfg->parity == UART_CFG_PARITY_SPACE) {
			parity = 0;
		}
	}

	return parity;
}

/*
 * To perform reception of data, a counter interrupt is used for each bit to be read.
 * This interrupt uses a state machine and gets the rx gpio depending of the current state.
 */
static void uart_bitbang_rx_counter_top_interrupt(const struct device *dev, void *user_data)
{
	const struct device *uart_dev = (struct device *)user_data;
	const struct uart_bitbang_config *config = uart_dev->config;
	struct uart_bitbang_data *data = uart_dev->data;
	uint8_t len = uart_bitbang_data_bits_to_len(config->uart_cfg->data_bits);
	int rc;

	/* Rx state machine */
	if (data->rx_state == UART_BITBANG_DATA) {

		/* Compute rx data depending of the bit index */
		const int shift = config->msb ? (len - 1 - data->rx_index) : data->rx_index;

		data->rx_data |= ((gpio_pin_get_dt(&config->rx_gpio) & 0x1) << shift);
		data->rx_index++;
		if (data->rx_index == len) {
			if (config->uart_cfg->parity != UART_CFG_PARITY_NONE) {
				data->rx_state = UART_BITBANG_PARITY;
			} else {
				data->rx_state = UART_BITBANG_COMPLETE;
			}
		}

	} else if (data->rx_state == UART_BITBANG_PARITY) {

		/* Read parity bit value */
		data->rx_parity = gpio_pin_get_dt(&config->rx_gpio);
		data->rx_state = UART_BITBANG_COMPLETE;
	}

	/* Intentional fall-through */
	if (data->rx_state == UART_BITBANG_COMPLETE) {

		/* Stop rx counter */
		counter_stop(config->rx_counter);
		data->rx_state = UART_BITBANG_IDLE;

		/* Enable rx gpio interrupt */
		rc = gpio_pin_interrupt_configure_dt(&config->rx_gpio, GPIO_INT_EDGE_FALLING);
		if (rc < 0) {
			LOG_ERR("Couldn't configure rx pin (%d)", rc);
		}

		/* Check parity */
		if (config->uart_cfg->parity != UART_CFG_PARITY_NONE) {
			if (data->rx_parity !=
			    uart_bitbang_compute_parity(uart_dev, data->rx_data)) {

				/* Indicate parity error */
				data->err |= UART_ERROR_PARITY;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
				if ((data->user_cb) && (data->irq & UART_BITBANG_IRQ_PE)) {
					data->user_cb(dev, data->user_data);
				}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

				return;
			}
		}

		/* Push data received to rx ring buffer */
		ring_buf_put(data->rx_ringbuf, (const uint8_t *)&data->rx_data, sizeof(uint16_t));

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		if ((data->user_cb) && (data->irq & UART_BITBANG_IRQ_RXNE)) {
			data->user_cb(dev, data->user_data);
		}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	}
}

static void uart_bitbang_rx_callback(const struct device *dev, struct gpio_callback *cb,
				     uint32_t pins)
{
	struct uart_bitbang_data *data =
		CONTAINER_OF(cb, struct uart_bitbang_data, rx_gpio_cb_data);
	struct uart_bitbang_config *config = data->config;
	(void)pins;
	int rc;

	/* Start bit detected, start reception of data */
	data->rx_data = 0;
	data->rx_index = 0;
	data->rx_state = UART_BITBANG_DATA;
	counter_reset(config->rx_counter);
	counter_start(config->rx_counter);

	/* Disable rx gpio interrupt */
	rc = gpio_pin_interrupt_configure_dt(&config->rx_gpio, GPIO_INT_DISABLE);
	if (rc < 0) {
		LOG_ERR("Couldn't configure rx pin (%d)", rc);
	}
}

static int uart_bitbang_poll_in_u16(const struct device *dev, uint16_t *in_u16)
{
	struct uart_bitbang_data *data = dev->data;

	uint32_t s = ring_buf_get(data->rx_ringbuf, (uint8_t *)in_u16, sizeof(uint16_t));

	return (s == sizeof(uint16_t)) ? 0 : -1;
}

static int uart_bitbang_poll_in(const struct device *dev, unsigned char *c)
{
	uint16_t in_u16;
	int rc;

	rc = uart_bitbang_poll_in_u16(dev, &in_u16);
	*c = (unsigned char)(in_u16 & 0xFF);

	return rc;
}

/*
 * To perform transmission of data, a counter interrupt is used for each bit to be transmitted.
 * This interrupt uses a state machine and sets the tx gpio depending of the current state.
 */
static void uart_bitbang_tx_counter_top_interrupt(const struct device *dev, void *user_data)
{
	const struct device *uart_dev = (struct device *)user_data;
	const struct uart_bitbang_config *config = uart_dev->config;
	struct uart_bitbang_data *data = uart_dev->data;
	uint8_t len = uart_bitbang_data_bits_to_len(config->uart_cfg->data_bits);
	uint32_t size;

	/* Tx state machine */
	switch (data->tx_state) {
	case UART_BITBANG_IDLE:
		/* Claim the next data */
		size = ring_buf_get_claim(data->tx_ringbuf, (uint8_t **)&data->tx_data,
					  sizeof(uint16_t));
		if (size == sizeof(uint16_t)) {
			/* Start next transmission */
			data->tx_index = 0;
			data->tx_parity = uart_bitbang_compute_parity(uart_dev, *data->tx_data);
			data->tx_state = UART_BITBANG_START_BIT;
			/* Assert RS485 driver enable pin */
			if ((config->uart_cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) &&
			    (config->de_gpio.port != NULL)) {
				gpio_pin_set_dt(&config->de_gpio, 1);
			}
		} else {
			/* End of the transmission, stop tx counter */
			counter_stop(config->tx_counter);
			/* Release RS485 driver enable pin */
			if ((config->uart_cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) &&
			    (config->de_gpio.port != NULL)) {
				gpio_pin_set_dt(&config->de_gpio, 0);
			}
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
			if ((data->user_cb) && (data->irq & UART_BITBANG_IRQ_TC)) {
				data->user_cb(dev, data->user_data);
			}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
		}
		break;
	case UART_BITBANG_START_BIT:
		/* Set start bit value */
		gpio_pin_set_dt(&config->tx_gpio, 0);
		/* Prepare transmission of data */
		data->tx_state = UART_BITBANG_DATA;
		break;
	case UART_BITBANG_DATA:
		/* Set tx gpio depending of the bit index */
		const int shift = config->msb ? (len - 1 - data->tx_index) : data->tx_index;
		const int d = (*data->tx_data >> shift) & 0x1;

		gpio_pin_set_dt(&config->tx_gpio, d);
		data->tx_index++;
		if (data->tx_index == len) {
			if (config->uart_cfg->parity != UART_CFG_PARITY_NONE) {
				data->tx_state = UART_BITBANG_PARITY;
			} else {
				data->tx_state = UART_BITBANG_STOP_BIT_1;
			}
		}
		break;
	case UART_BITBANG_PARITY:
		/* Set parity bit value */
		gpio_pin_set_dt(&config->tx_gpio, data->tx_parity);
		data->tx_state = UART_BITBANG_STOP_BIT_1;
		break;
	case UART_BITBANG_STOP_BIT_1:
		/* Set stop bit value */
		gpio_pin_set_dt(&config->tx_gpio, 1);
		if (config->uart_cfg->stop_bits > UART_CFG_STOP_BITS_1) {
			data->tx_state = UART_BITBANG_STOP_BIT_2;
		} else {
			data->tx_state = UART_BITBANG_COMPLETE;
		}
		break;
	case UART_BITBANG_STOP_BIT_2:
		/* Wait one more bit in case 1.5 or 2 stop bits */
		data->tx_state = UART_BITBANG_COMPLETE;
		break;
	case UART_BITBANG_COMPLETE:
		/* Terminate current transfer */
		ring_buf_get_finish(data->tx_ringbuf, sizeof(uint16_t));
		data->tx_state = UART_BITBANG_IDLE;
		break;
	}
}

static void uart_bitbang_poll_out_u16(const struct device *dev, uint16_t out_u16)
{
	const struct uart_bitbang_config *config = dev->config;
	struct uart_bitbang_data *data = dev->data;

	/* Transmit data */
	if (config->tx_gpio.port != NULL) {

		/* Push data to send to tx ring buffer */
		ring_buf_put(data->tx_ringbuf, (const uint8_t *)&out_u16, sizeof(uint16_t));

		/* Start tx counter if not already started */
		counter_reset(config->tx_counter);
		counter_start(config->tx_counter);
	}
}

static void uart_bitbang_poll_out(const struct device *dev, unsigned char c)
{
	uart_bitbang_poll_out_u16(dev, (uint16_t)c);
}

/*
 * Handler used when tx and rx counters are the same instance (half-duplex communications).
 * The tx or rx handler is called depending on the device state (rx state not idle indicating the
 * reception of data).
 */
static void uart_bitbang_tx_rx_counter_top_interrupt(const struct device *dev, void *user_data)
{
	const struct device *uart_dev = (struct device *)user_data;
	struct uart_bitbang_data *data = uart_dev->data;

	if (data->rx_state != UART_BITBANG_IDLE) {
		uart_bitbang_rx_counter_top_interrupt(dev, user_data);
	} else {
		uart_bitbang_tx_counter_top_interrupt(dev, user_data);
	}
}

static int uart_bitbang_err_check(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;
	int err;

	/* Check for errors, then clear them */
	err = data->err;
	data->err = 0;

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_bitbang_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_bitbang_config *config = dev->config;

	/* Copy configuration */
	memcpy(config->uart_cfg, cfg, sizeof(struct uart_config));

	/* Reinitialize device */
	return uart_bitbang_init(dev);
};

static int uart_bitbang_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_bitbang_config *config = dev->config;

	/* Copy configuration */
	memcpy(cfg, config->uart_cfg, sizeof(struct uart_config));

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_bitbang_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	int index = 0;

	while (size - index > 0) {
		uart_bitbang_poll_out(dev, tx_data[index]);
		index++;
	}

	return index;
}

#ifdef CONFIG_UART_WIDE_DATA

static int uart_bitbang_fifo_fill_u16(const struct device *dev, const uint16_t *tx_data, int size)
{
	int index = 0;

	while (size - index > 0) {
		uart_bitbang_poll_out_u16(dev, tx_data[index]);
		index++;
	}

	return index;
}

#endif /* CONFIG_UART_WIDE_DATA */

static int uart_bitbang_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	int index = 0;

	while (size - index > 0) {
		if (uart_bitbang_poll_in(dev, &rx_data[index]) < 0) {
			break;
		}
		index++;
	}

	return index;
}

#ifdef CONFIG_UART_WIDE_DATA

static int uart_bitbang_fifo_read_u16(const struct device *dev, uint16_t *rx_data, const int size)
{
	int index = 0;

	while (size - index > 0) {
		if (uart_bitbang_poll_in_u16(dev, &rx_data[index]) < 0) {
			break;
		}
		index++;
	}

	return index;
}

#endif /* CONFIG_UART_WIDE_DATA */

static void uart_bitbang_irq_tx_enable(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	/* Enable Transmission Complete interrupt */
	data->irq |= UART_BITBANG_IRQ_TC;
}

static void uart_bitbang_irq_tx_disable(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	/* Disable Transmission Complete interrupt */
	data->irq &= ~UART_BITBANG_IRQ_TC;
}

static int uart_bitbang_irq_tx_ready(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	return (int)(ring_buf_space_get(data->tx_ringbuf) / sizeof(uint16_t));
}

static void uart_bitbang_irq_rx_enable(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	/* Enable Receive data register Not Empty interrupt */
	data->irq |= UART_BITBANG_IRQ_RXNE;
}

static void uart_bitbang_irq_rx_disable(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	/* Disable Receive data register Not Empty interrupt */
	data->irq &= ~UART_BITBANG_IRQ_RXNE;
}

static int uart_bitbang_irq_tx_complete(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	return ring_buf_is_empty(data->tx_ringbuf) ? 1 : 0;
}

static int uart_bitbang_irq_rx_ready(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	return (ring_buf_size_get(data->rx_ringbuf) > 0) ? 1 : 0;
}

static void uart_bitbang_irq_err_enable(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	/* Enable Parity Error interrupt */
	data->irq |= UART_BITBANG_IRQ_PE;
}

static void uart_bitbang_irq_err_disable(const struct device *dev)
{
	struct uart_bitbang_data *data = dev->data;

	/* Disable Parity Error interrupt */
	data->irq &= ~UART_BITBANG_IRQ_PE;
}

static int uart_bitbang_irq_is_pending(const struct device *dev)
{
	return 0;
}

static int uart_bitbang_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_bitbang_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_bitbang_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_bitbang_api) = {
	.poll_in = uart_bitbang_poll_in,
	.poll_out = uart_bitbang_poll_out,
#ifdef CONFIG_UART_WIDE_DATA
	.poll_in_u16 = uart_bitbang_poll_in_u16,
	.poll_out_u16 = uart_bitbang_poll_out_u16,
#endif /* CONFIG_UART_WIDE_DATA */
	.err_check = uart_bitbang_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_bitbang_configure,
	.config_get = uart_bitbang_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_bitbang_fifo_fill,
#ifdef CONFIG_UART_WIDE_DATA
	.fifo_fill_u16 = uart_bitbang_fifo_fill_u16,
#endif /* CONFIG_UART_WIDE_DATA */
	.fifo_read = uart_bitbang_fifo_read,
#ifdef CONFIG_UART_WIDE_DATA
	.fifo_read_u16 = uart_bitbang_fifo_read_u16,
#endif /* CONFIG_UART_WIDE_DATA */
	.irq_tx_enable = uart_bitbang_irq_tx_enable,
	.irq_tx_disable = uart_bitbang_irq_tx_disable,
	.irq_tx_ready = uart_bitbang_irq_tx_ready,
	.irq_rx_enable = uart_bitbang_irq_rx_enable,
	.irq_rx_disable = uart_bitbang_irq_rx_disable,
	.irq_tx_complete = uart_bitbang_irq_tx_complete,
	.irq_rx_ready = uart_bitbang_irq_rx_ready,
	.irq_err_enable = uart_bitbang_irq_err_enable,
	.irq_err_disable = uart_bitbang_irq_err_disable,
	.irq_is_pending = uart_bitbang_irq_is_pending,
	.irq_update = uart_bitbang_irq_update,
	.irq_callback_set = uart_bitbang_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_bitbang_init(const struct device *dev)
{
	const struct uart_bitbang_config *config = dev->config;
	struct uart_bitbang_data *data = dev->data;
	int rc;

	/* Initialize tx state */
	data->tx_state = UART_BITBANG_IDLE;

	/*
	 * Setup tx counter so that the counter interrupts for each bit to be generated
	 * A state machine in the interrupt handler permits to generate the uart signal
	 */
	if (config->tx_gpio.port != NULL) {
		if (config->tx_counter == NULL) {
			LOG_ERR("Couldn't configure tx counter");
			return -ENODEV;
		} else if (config->tx_counter != config->rx_counter) {
			if (!device_is_ready(config->tx_counter)) {
				LOG_ERR("Couldn't configure tx counter");
				return -ENODEV;
			}
			data->tx_counter_cfg.callback = uart_bitbang_tx_counter_top_interrupt;
			data->tx_counter_cfg.ticks = counter_get_frequency(config->tx_counter) /
						     config->uart_cfg->baudrate;
			data->tx_counter_cfg.user_data = (void *)dev;
			data->tx_counter_cfg.flags = 0;
			rc = counter_set_top_value(config->tx_counter, &data->tx_counter_cfg);
			if (rc < 0) {
				LOG_ERR("Couldn't configure tx counter (%d)", rc);
				return rc;
			}
		}
	}

	/* Initialize rx state */
	data->rx_state = UART_BITBANG_IDLE;

	/*
	 * Setup rx counter so that the counter interrupts for each bit to be read
	 * A state machine in the interrupt handler permits to capture the uart signal
	 */
	if (config->rx_gpio.port != NULL) {
		if (config->rx_counter == NULL) {
			LOG_ERR("Couldn't configure rx counter");
			return -ENODEV;
		} else if (config->rx_counter != config->tx_counter) {
			if (!device_is_ready(config->rx_counter)) {
				LOG_ERR("Couldn't configure rx counter");
				return -ENODEV;
			}
			data->rx_counter_cfg.callback = uart_bitbang_rx_counter_top_interrupt;
			data->rx_counter_cfg.ticks = counter_get_frequency(config->rx_counter) /
						     config->uart_cfg->baudrate;
			data->rx_counter_cfg.user_data = (void *)dev;
			data->rx_counter_cfg.flags = 0;
			rc = counter_set_top_value(config->rx_counter, &data->rx_counter_cfg);
			if (rc < 0) {
				LOG_ERR("Couldn't configure rx counter (%d)", rc);
				return rc;
			}
		}
	}

	/*
	 * Setup tx/rx counter in case it is the same instance so that the counter interrupts for
	 * each bit to be generated or read
	 * The interrupt handler calls the tx or rx counter handler depending on the device state
	 */
	if (((config->tx_gpio.port != NULL) || (config->rx_gpio.port != NULL)) &&
	    (config->tx_counter != NULL) && (config->tx_counter == config->rx_counter)) {
		if (!device_is_ready(config->tx_counter)) {
			LOG_ERR("Couldn't configure tx/rx counter");
			return -ENODEV;
		}
		data->tx_counter_cfg.callback = uart_bitbang_tx_rx_counter_top_interrupt;
		data->tx_counter_cfg.ticks =
			counter_get_frequency(config->tx_counter) / config->uart_cfg->baudrate;
		data->tx_counter_cfg.user_data = (void *)dev;
		data->tx_counter_cfg.flags = 0;
		rc = counter_set_top_value(config->tx_counter, &data->tx_counter_cfg);
		if (rc < 0) {
			LOG_ERR("Couldn't configure tx/rx counter (%d)", rc);
			return rc;
		}
	}

	/* Setup tx gpio if it is defined */
	if (config->tx_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->tx_gpio)) {
			LOG_ERR("GPIO port for tx pin is not ready");
			return -ENODEV;
		}
		rc = gpio_pin_configure_dt(&config->tx_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("Couldn't configure tx pin (%d)", rc);
			return rc;
		}
		rc = gpio_pin_set_dt(&config->tx_gpio, 1);
		if (rc < 0) {
			LOG_ERR("Couldn't set tx pin (%d)", rc);
			return rc;
		}
	}

	/* Setup rx gpio if it is defined */
	if (config->rx_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->rx_gpio)) {
			LOG_ERR("GPIO port for rx pin is not ready");
			return -ENODEV;
		}
		rc = gpio_pin_configure_dt(&config->rx_gpio, GPIO_INPUT);
		if (rc < 0) {
			LOG_ERR("Couldn't configure rx pin (%d)", rc);
			return rc;
		}
		rc = gpio_pin_interrupt_configure_dt(&config->rx_gpio, GPIO_INT_EDGE_FALLING);
		if (rc < 0) {
			LOG_ERR("Couldn't configure rx pin (%d)", rc);
			return rc;
		}
		gpio_init_callback(&data->rx_gpio_cb_data, uart_bitbang_rx_callback,
				   BIT(config->rx_gpio.pin));
		rc = gpio_add_callback_dt(&config->rx_gpio, &data->rx_gpio_cb_data);
		if (rc < 0) {
			LOG_ERR("Couldn't configure rx callback (%d)", rc);
			return rc;
		}
	}

	/* Setup RS485 driver enable gpio if it is defined */
	if ((config->uart_cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) &&
	    (config->de_gpio.port != NULL)) {
		if (!gpio_is_ready_dt(&config->de_gpio)) {
			LOG_ERR("GPIO port for driver enable pin is not ready");
			return -ENODEV;
		}
		rc = gpio_pin_configure_dt(&config->de_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			LOG_ERR("Couldn't configure driver enable pin (%d)", rc);
			return rc;
		}
		rc = gpio_pin_set_dt(&config->de_gpio, 0);
		if (rc < 0) {
			LOG_ERR("Couldn't set driver enable pin (%d)", rc);
			return rc;
		}
	}

	return 0;
}

#define UART_BITBANG_INIT(index)                                                                   \
	static struct uart_config uart_cfg_##index = {                                             \
		.baudrate = DT_INST_PROP(index, current_speed),                                    \
		.parity = DT_INST_ENUM_IDX(index, parity),                                         \
		.stop_bits = DT_INST_ENUM_IDX(index, stop_bits),                                   \
		.data_bits = DT_INST_ENUM_IDX(index, data_bits),                                   \
		.flow_ctrl = DT_INST_PROP(index, hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS     \
			     : DT_INST_PROP(index, hw_rs485_flow_control)                          \
				     ? UART_CFG_FLOW_CTRL_RS485                                    \
				     : UART_CFG_FLOW_CTRL_NONE,                                    \
	};                                                                                         \
	static struct uart_bitbang_config uart_bitbang_config_##index = {                          \
		.tx_gpio = GPIO_DT_SPEC_INST_GET_OR(index, tx_gpios, {0}),                         \
		.rx_gpio = GPIO_DT_SPEC_INST_GET_OR(index, rx_gpios, {0}),                         \
		.de_gpio = GPIO_DT_SPEC_INST_GET_OR(index, de_gpios, {0}),                         \
		.tx_counter = DEVICE_DT_GET_OR_NULL(                                               \
			DT_CHILD(DT_INST_PHANDLE(index, tx_timer), counter)),                      \
		.rx_counter = DEVICE_DT_GET_OR_NULL(                                               \
			DT_CHILD(DT_INST_PHANDLE(index, rx_timer), counter)),                      \
		.uart_cfg = &uart_cfg_##index,                                                     \
		.msb = DT_INST_PROP_OR(index, msb, false),                                         \
	};                                                                                         \
	RING_BUF_DECLARE(uart_bitbang_tx_ringbuf##index, DT_INST_PROP(index, tx_fifo_size));       \
	RING_BUF_DECLARE(uart_bitbang_rx_ringbuf##index, DT_INST_PROP(index, rx_fifo_size));       \
	static struct uart_bitbang_data uart_bitbang_data_##index = {                              \
		.config = &uart_bitbang_config_##index,                                            \
		.tx_ringbuf = &uart_bitbang_tx_ringbuf##index,                                     \
		.rx_ringbuf = &uart_bitbang_rx_ringbuf##index,                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, uart_bitbang_init, PM_DEVICE_DT_INST_GET(index),              \
			      &uart_bitbang_data_##index, &uart_bitbang_config_##index,            \
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY, &uart_bitbang_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BITBANG_INIT)
