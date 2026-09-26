/*
 * Copyright (c) 2022, Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/spinlock.h>

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>

#include <hardware/pio.h>
#include <hardware/clocks.h>

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#include <zephyr/drivers/dma.h>
#include <hardware/dma.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_rpi_pico_pio, CONFIG_UART_LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_uart_pio

#define CYCLES_PER_BIT 8
#define SIDESET_BIT_COUNT 2

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

BUILD_ASSERT(CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE > 1 &&
	    (CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE &
	     (CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE - 1)) == 0,
	    "CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE must be a power of two, and non-zero");

#define PIO_UART_RX_RING_MASK (CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE - 1)

/* Any large count works: the control channel reloads it on completion. */
#define PIO_UART_RX_DMA_RELOAD_COUNT 1000000UL

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

struct pio_uart_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pcfg;
	const uint32_t tx_pin;
	const uint32_t rx_pin;
	uint32_t baudrate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uint8_t *rx_dma_buf;
#endif
};

struct pio_uart_data {
	size_t tx_sm;
	size_t rx_sm;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct k_spinlock cb_lock;
	uart_irq_callback_user_data_t cb;
	void *cb_data;
	atomic_t rx_irq_enabled;
	atomic_t tx_irq_enabled;
	int rx_dma_chan;
	int rx_dma_ctrl_chan;
	uint8_t *rx_dma_buf;
	uint32_t rx_read_pos;
	struct k_thread poll_thread;

	K_KERNEL_STACK_MEMBER(poll_stack, CONFIG_UART_RPI_PICO_PIO_POLL_STACK_SIZE);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
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

	/* Use 64-bit mask to support RP235xB SoCs (i.e. chips with >32 GPIO). */
	pio_sm_set_pins_with_mask64(pio, sm, BIT64(tx_pin), BIT64(tx_pin));
	pio_sm_set_pindirs_with_mask64(pio, sm, BIT64(tx_pin), BIT64(tx_pin));
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
 * The RX program shifts right, so the byte lands in the FIFO word's MSB.
 * Reading it pops the word.
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

/* Source of the control channel's write to the data channel's transfer count. */
static const uint32_t pio_uart_rx_dma_reload_count = PIO_UART_RX_DMA_RELOAD_COUNT;

static int pio_uart_rx_dma_init(PIO pio, uint32_t rx_sm, struct pio_uart_data *data,
				uint8_t *rx_dma_buf)
{
	const struct device *dma_dev = DEVICE_DT_GET_ONE(raspberrypi_pico_dma);
	io_rw_8 *rx_fifo_msb = (io_rw_8 *)&pio->rxf[rx_sm] + 3;
	dma_channel_config dma_cfg;
	dma_channel_config ctrl_cfg;
	int data_chan;
	int ctrl_chan;

	data_chan = dma_request_channel(dma_dev, NULL);
	if (data_chan < 0) {
		return -EBUSY;
	}

	ctrl_chan = dma_request_channel(dma_dev, NULL);
	if (ctrl_chan < 0) {
		dma_release_channel(dma_dev, data_chan);
		return -EBUSY;
	}

	data->rx_dma_chan = data_chan;
	data->rx_dma_ctrl_chan = ctrl_chan;
	data->rx_dma_buf = rx_dma_buf;
	data->rx_read_pos = 0;

	dma_cfg = dma_channel_get_default_config(data_chan);
	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_8);
	channel_config_set_read_increment(&dma_cfg, false);
	channel_config_set_write_increment(&dma_cfg, true);
	channel_config_set_dreq(&dma_cfg, pio_get_dreq(pio, rx_sm, false));
	channel_config_set_ring(&dma_cfg, true,
				 __builtin_ctz(CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE));
	channel_config_set_chain_to(&dma_cfg, ctrl_chan);

	dma_channel_configure(data_chan, &dma_cfg, data->rx_dma_buf, rx_fifo_msb,
			      PIO_UART_RX_DMA_RELOAD_COUNT, true);

	/*
	 * Chained from the data channel on completion, the control channel writes
	 * its TRANS_COUNT trigger alias. This restarts it in hardware and keeps the
	 * write address, so the ring position carries over.
	 */
	ctrl_cfg = dma_channel_get_default_config(ctrl_chan);
	channel_config_set_transfer_data_size(&ctrl_cfg, DMA_SIZE_32);
	channel_config_set_read_increment(&ctrl_cfg, false);
	channel_config_set_write_increment(&ctrl_cfg, false);
	channel_config_set_dreq(&ctrl_cfg, DREQ_FORCE);
	channel_config_set_chain_to(&ctrl_cfg, ctrl_chan);

	dma_channel_configure(ctrl_chan, &ctrl_cfg,
			      (volatile void *)
			      &dma_channel_hw_addr(data_chan)->al1_transfer_count_trig,
			      &pio_uart_rx_dma_reload_count, 1, false);

	return 0;
}

static int pio_uart_rx_ring_read(struct pio_uart_data *data, uint8_t *out, int max)
{
	uint32_t write_pos = ((uint32_t)dma_channel_hw_addr(data->rx_dma_chan)->write_addr -
			      (uint32_t)data->rx_dma_buf) & PIO_UART_RX_RING_MASK;
	int n = 0;

	while (n < max && data->rx_read_pos != write_pos) {
		out[n++] = data->rx_dma_buf[data->rx_read_pos];
		data->rx_read_pos = (data->rx_read_pos + 1) & PIO_UART_RX_RING_MASK;
	}

	return n;
}

static int pio_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);
	int i;

	for (i = 0; i < len && !pio_sm_is_tx_fifo_full(pio, data->tx_sm); i++) {
		pio->txf[data->tx_sm] = tx_data[i];
	}

	/* TXSTALL is sticky. Clearing it ties irq_tx_complete() to this fill. */
	pio->fdebug = BIT(PIO_FDEBUG_TXSTALL_LSB + data->tx_sm);

	return i;
}

static int pio_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct pio_uart_data *data = dev->data;

	return pio_uart_rx_ring_read(data, rx_data, size);
}

static void pio_uart_irq_tx_enable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;

	atomic_set(&data->tx_irq_enabled, 1);
}

static void pio_uart_irq_tx_disable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;

	atomic_set(&data->tx_irq_enabled, 0);
}

static int pio_uart_irq_tx_ready(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	return atomic_get(&data->tx_irq_enabled) && !pio_sm_is_tx_fifo_full(pio, data->tx_sm);
}

static int pio_uart_irq_tx_complete(const struct device *dev)
{
	const struct pio_uart_config *config = dev->config;
	struct pio_uart_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	/*
	 * The last frame keeps shifting out of the OSR after the FIFO empties.
	 * TXSTALL latches once the state machine has finished it.
	 */
	return pio_sm_is_tx_fifo_empty(pio, data->tx_sm) &&
	      (pio->fdebug & BIT(PIO_FDEBUG_TXSTALL_LSB + data->tx_sm));
}

static void pio_uart_irq_rx_enable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;

	atomic_set(&data->rx_irq_enabled, 1);
}

static void pio_uart_irq_rx_disable(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;

	atomic_set(&data->rx_irq_enabled, 0);
}

static int pio_uart_irq_rx_ready(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;
	uint32_t write_pos = ((uint32_t)dma_channel_hw_addr(data->rx_dma_chan)->write_addr -
			      (uint32_t)data->rx_dma_buf) & PIO_UART_RX_RING_MASK;

	return atomic_get(&data->rx_irq_enabled) && (data->rx_read_pos != write_pos);
}

static int pio_uart_irq_is_pending(const struct device *dev)
{
	return pio_uart_irq_rx_ready(dev) || pio_uart_irq_tx_ready(dev);
}

static void pio_uart_irq_update(const struct device *dev)
{
	/* Readiness is read live from the DMA ring and the PIO FIFO. */
	ARG_UNUSED(dev);
}

static void pio_uart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct pio_uart_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->cb_lock);

	data->cb = cb;
	data->cb_data = cb_data;

	k_spin_unlock(&data->cb_lock, key);
}

static void pio_uart_poll_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct pio_uart_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		uart_irq_callback_user_data_t cb;
		void *cb_data;
		k_spinlock_key_t key = k_spin_lock(&data->cb_lock);

		cb = data->cb;
		cb_data = data->cb_data;

		k_spin_unlock(&data->cb_lock, key);

		if ((cb != NULL) &&
		    (atomic_get(&data->rx_irq_enabled) || atomic_get(&data->tx_irq_enabled))) {
			cb(dev, cb_data);
		}

		k_usleep(CONFIG_UART_RPI_PICO_PIO_POLL_PERIOD_US);
	}
}

static void pio_uart_start_poll_thread(const struct device *dev)
{
	struct pio_uart_data *data = dev->data;

	k_thread_create(&data->poll_thread, data->poll_stack,
			K_KERNEL_STACK_SIZEOF(data->poll_stack), pio_uart_poll_thread,
			(void *)dev, NULL, NULL, CONFIG_UART_RPI_PICO_PIO_POLL_THREAD_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&data->poll_thread, "pio_uart_poll");
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
	retval = pio_uart_rx_dma_init(pio, rx_sm, data, config->rx_dma_buf);
	if (retval < 0) {
		LOG_ERR("failed to set up the RX DMA channel: %d", retval);
		return retval;
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	/* Applied last, once the state machine and RX DMA can drain the FIFO. */
	retval = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	pio_uart_start_poll_thread(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
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
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define PIO_UART_INIT(idx)									\
	PINCTRL_DT_INST_DEFINE(idx);								\
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,						\
		(static uint8_t									\
		 pio_uart##idx##_rx_dma_buf[CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE]		\
			__aligned(CONFIG_UART_RPI_PICO_PIO_RX_RING_BUF_SIZE);))		\
	static const struct pio_uart_config pio_uart##idx##_config = {				\
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(idx)),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),					\
		.tx_pin = DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, tx_pins, 0),	\
		.rx_pin = DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, rx_pins, 0),	\
		.baudrate = DT_INST_PROP(idx, current_speed),					\
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,					\
			(.rx_dma_buf = pio_uart##idx##_rx_dma_buf,))				\
	};											\
	static struct pio_uart_data pio_uart##idx##_data;					\
												\
	DEVICE_DT_INST_DEFINE(idx, pio_uart_init, NULL, &pio_uart##idx##_data,			\
			      &pio_uart##idx##_config, POST_KERNEL,				\
			      CONFIG_SERIAL_INIT_PRIORITY,					\
			      &pio_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PIO_UART_INIT)
