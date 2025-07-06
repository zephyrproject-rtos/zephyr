/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>

#include <nrfx_spim.h>
#include <nrfx_uarte.h>
#include <drivers/src/prs/nrfx_prs.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#define TRANSFER_LENGTH 10

/* Devicetree nodes corresponding to the peripherals to be used directly via
 * nrfx drivers (SPIM2 and UARTE2).
 */
#define SPIM_NODE  DT_NODELABEL(spi2)
#define UARTE_NODE DT_NODELABEL(uart2)

/* Devicetree node corresponding to the peripheral to be used via Zephyr SPI
 * driver (SPIM1), in the background transfer.
 */
#define SPI_DEV_NODE DT_NODELABEL(spi1)

static nrfx_spim_t spim = NRFX_SPIM_INSTANCE(2);
static nrfx_uarte_t uarte = NRFX_UARTE_INSTANCE(2);
static bool spim_initialized;
static bool uarte_initialized;
static volatile size_t received;
static K_SEM_DEFINE(transfer_finished, 0, 1);

static enum {
	PERFORM_TRANSFER,
	SWITCH_PERIPHERAL
} user_request;
static K_SEM_DEFINE(button_pressed, 0, 1);

static void sw0_handler(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	user_request = PERFORM_TRANSFER;
	k_sem_give(&button_pressed);
}

static void sw1_handler(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	user_request = SWITCH_PERIPHERAL;
	k_sem_give(&button_pressed);
}

static bool init_buttons(void)
{
	static const struct button_spec {
		struct gpio_dt_spec gpio;
		char const *label;
		char const *action;
		gpio_callback_handler_t handler;
	} btn_spec[] = {
		{
			GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),
			DT_PROP(DT_ALIAS(sw0), label),
			"trigger a transfer",
			sw0_handler
		},
		{
			GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios),
			DT_PROP(DT_ALIAS(sw1), label),
			"switch the type of peripheral",
			sw1_handler
		},
	};
	static struct gpio_callback btn_cb_data[ARRAY_SIZE(btn_spec)];

	for (int i = 0; i < ARRAY_SIZE(btn_spec); ++i) {
		const struct button_spec *btn = &btn_spec[i];
		int ret;

		if (!gpio_is_ready_dt(&btn->gpio)) {
			printk("%s is not ready\n", btn->gpio.port->name);
			return false;
		}

		ret = gpio_pin_configure_dt(&btn->gpio, GPIO_INPUT);
		if (ret < 0) {
			printk("Failed to configure %s pin %d: %d\n",
				btn->gpio.port->name, btn->gpio.pin, ret);
			return false;
		}

		ret = gpio_pin_interrupt_configure_dt(&btn->gpio,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			printk("Failed to configure interrupt on %s pin %d: %d\n",
				btn->gpio.port->name, btn->gpio.pin, ret);
			return false;
		}

		gpio_init_callback(&btn_cb_data[i],
				   btn->handler, BIT(btn->gpio.pin));
		gpio_add_callback(btn->gpio.port, &btn_cb_data[i]);
		printk("-> press \"%s\" to %s\n", btn->label, btn->action);
	}

	return true;
}

static void spim_handler(const nrfx_spim_evt_t *p_event, void *p_context)
{
	if (p_event->type == NRFX_SPIM_EVENT_DONE) {
		k_sem_give(&transfer_finished);
	}
}

static bool switch_to_spim(void)
{
	int ret;
	nrfx_err_t err;
	uint32_t sck_pin;

	PINCTRL_DT_DEFINE(SPIM_NODE);

	if (spim_initialized) {
		return true;
	}

	/* If the UARTE is currently initialized, it must be deinitialized
	 * before the SPIM can be used.
	 */
	if (uarte_initialized) {
		nrfx_uarte_uninit(&uarte);
		/* Workaround: uninit does not clear events, make sure all events are cleared. */
		nrfy_uarte_int_init(uarte.p_reg, 0xFFFFFFFF, 0, false);
		uarte_initialized = false;
	}

	nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG(
		NRF_SPIM_PIN_NOT_CONNECTED,
		NRF_SPIM_PIN_NOT_CONNECTED,
		NRF_SPIM_PIN_NOT_CONNECTED,
		NRF_DT_GPIOS_TO_PSEL(SPIM_NODE, cs_gpios));
	spim_config.frequency = MHZ(1);
	spim_config.skip_gpio_cfg = true;
	spim_config.skip_psel_cfg = true;

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(SPIM_NODE),
				  PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Set initial state of SCK according to the SPI mode. */
	sck_pin = nrfy_spim_sck_pin_get(spim.p_reg);

	if (sck_pin != NRF_SPIM_PIN_NOT_CONNECTED) {
		nrfy_gpio_pin_write(sck_pin, (spim_config.mode <= NRF_SPIM_MODE_1) ? 0 : 1);
	}

	err = nrfx_spim_init(&spim, &spim_config, spim_handler, NULL);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_spim_init() failed: 0x%08x\n", err);
		return false;
	}

	spim_initialized = true;
	printk("Switched to SPIM\n");
	return true;
}

static bool spim_transfer(const uint8_t *tx_data, size_t tx_data_len,
			  uint8_t *rx_buf, size_t rx_buf_size)
{
	nrfx_err_t err;
	nrfx_spim_xfer_desc_t xfer_desc = {
		.p_tx_buffer = tx_data,
		.tx_length = tx_data_len,
		.p_rx_buffer = rx_buf,
		.rx_length = rx_buf_size,
	};

	err = nrfx_spim_xfer(&spim, &xfer_desc, 0);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_spim_xfer() failed: 0x%08x\n", err);
		return false;
	}

	if (k_sem_take(&transfer_finished, K_MSEC(100)) != 0) {
		printk("SPIM transfer timeout\n");
		return false;
	}

	received = rx_buf_size;
	return true;
}

static void uarte_handler(const nrfx_uarte_event_t *p_event, void *p_context)
{
	if (p_event->type == NRFX_UARTE_EVT_RX_DONE) {
		received = p_event->data.rx.length;
		k_sem_give(&transfer_finished);
	} else if (p_event->type == NRFX_UARTE_EVT_ERROR) {
		received = 0;
		k_sem_give(&transfer_finished);
	}
}

static bool switch_to_uarte(void)
{
	int ret;
	nrfx_err_t err;

	PINCTRL_DT_DEFINE(UARTE_NODE);

	if (uarte_initialized) {
		return true;
	}

	/* If the SPIM is currently initialized, it must be deinitialized
	 * before the UARTE can be used.
	 */
	if (spim_initialized) {
		nrfx_spim_uninit(&spim);
		/* Workaround: uninit does not clear events, make sure all events are cleared. */
		nrfy_spim_int_init(spim.p_reg, 0xFFFFFFFF, 0, false);
		spim_initialized = false;
	}

	nrfx_uarte_config_t uarte_config = NRFX_UARTE_DEFAULT_CONFIG(
		NRF_UARTE_PSEL_DISCONNECTED,
		NRF_UARTE_PSEL_DISCONNECTED);
	uarte_config.baudrate = NRF_UARTE_BAUDRATE_1000000;
	uarte_config.skip_gpio_cfg = true;
	uarte_config.skip_psel_cfg = true;

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UARTE_NODE),
				  PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	err = nrfx_uarte_init(&uarte, &uarte_config, uarte_handler);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_uarte_init() failed: 0x%08x\n", err);
		return false;
	}

	uarte_initialized = true;
	printk("Switched to UARTE\n");
	return true;
}

static bool uarte_transfer(const uint8_t *tx_data, size_t tx_data_len,
			   uint8_t *rx_buf, size_t rx_buf_size)
{
	nrfx_err_t err;

	err = nrfx_uarte_rx(&uarte, rx_buf, rx_buf_size);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_uarte_rx() failed: 0x%08x\n", err);
		return false;
	}

	err = nrfx_uarte_tx(&uarte, tx_data, tx_data_len, 0);
	if (err != NRFX_SUCCESS) {
		printk("nrfx_uarte_tx() failed: 0x%08x\n", err);
		return false;
	}

	if (k_sem_take(&transfer_finished, K_MSEC(100)) != 0) {
		/* The UARTE transfer finishes when the RX buffer is completely
		 * filled. In case the UARTE receives less data (or nothing at
		 * all) within the specified time, taking the semaphore will
		 * fail. In such case, stop the reception and end the transfer
		 * this way. Now taking the semaphore should be successful.
		 */
		nrfx_uarte_rx_abort(&uarte, 0, 0);
		if (k_sem_take(&transfer_finished, K_MSEC(10)) != 0) {
			printk("UARTE transfer timeout\n");
			return false;
		}
	}

	return true;
}

static void buffer_dump(const uint8_t *buffer, size_t length)
{
	for (int i = 0; i < length; ++i) {
		printk(" %02X", buffer[i]);
	}
	printk("\n");
}

static bool background_transfer(const struct device *spi_dev)
{
	static const uint8_t tx_buffer[] = "Nordic Semiconductor";
	static uint8_t rx_buffer[sizeof(tx_buffer)];
	static const struct spi_config spi_dev_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |
			     SPI_TRANSFER_MSB,
		.frequency = MHZ(1),
		.cs = {
			.gpio = GPIO_DT_SPEC_GET(SPI_DEV_NODE, cs_gpios),
		},
	};
	static const struct spi_buf tx_buf = {
		.buf = (void *)tx_buffer,
		.len = sizeof(tx_buffer)
	};
	static const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	static const struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = sizeof(rx_buffer),
	};
	static const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};
	int ret;

	printk("-- Background transfer on \"%s\" --\n", spi_dev->name);

	ret = spi_transceive(spi_dev, &spi_dev_cfg, &tx, &rx);
	if (ret < 0) {
		printk("Background transfer failed: %d\n", ret);
		return false;
	}

	printk("Tx:");
	buffer_dump(tx_buf.buf, tx_buf.len);
	printk("Rx:");
	buffer_dump(rx_buf.buf, rx_buf.len);
	return true;
}

int main(void)
{
	printk("nrfx PRS example on %s\n", CONFIG_BOARD);

	static uint8_t tx_buffer[TRANSFER_LENGTH];
	static uint8_t rx_buffer[sizeof(tx_buffer)];
	uint8_t fill_value = 0;
	const struct device *const spi_dev = DEVICE_DT_GET(SPI_DEV_NODE);

	if (!device_is_ready(spi_dev)) {
		printk("%s is not ready\n", spi_dev->name);
		return 0;
	}

	/* Install a shared interrupt handler for peripherals used via
	 * nrfx drivers. It will dispatch the interrupt handling to the
	 * driver for the currently initialized peripheral.
	 */
	BUILD_ASSERT(
		DT_IRQ(SPIM_NODE, priority) == DT_IRQ(UARTE_NODE, priority),
		"Interrupt priorities for SPIM_NODE and UARTE_NODE need to be equal.");
	IRQ_CONNECT(DT_IRQN(SPIM_NODE), DT_IRQ(SPIM_NODE, priority),
		    nrfx_isr, nrfx_prs_box_2_irq_handler, 0);

	if (!init_buttons()) {
		return 0;
	}

	/* Initially use the SPIM. */
	if (!switch_to_spim()) {
		return 0;
	}

	for (;;) {
		/* Wait 5 seconds for the user to press a button. If no button
		 * is pressed within this time, perform the background transfer.
		 * Otherwise, realize the operation requested by the user.
		 */
		if (k_sem_take(&button_pressed, K_MSEC(5000)) != 0) {
			if (!background_transfer(spi_dev)) {
				return 0;
			}
		} else {
			bool res;

			switch (user_request) {
			case PERFORM_TRANSFER:
				printk("-- %s transfer --\n",
					spim_initialized ? "SPIM" : "UARTE");
				received = 0;
				for (int i = 0; i < sizeof(tx_buffer); ++i) {
					tx_buffer[i] = fill_value++;
				}
				res = (spim_initialized
				       ? spim_transfer(tx_buffer,
						       sizeof(tx_buffer),
						       rx_buffer,
						       sizeof(rx_buffer))
				       : uarte_transfer(tx_buffer,
							sizeof(tx_buffer),
							rx_buffer,
							sizeof(rx_buffer)));
				if (!res) {
					return 0;
				}

				printk("Tx:");
				buffer_dump(tx_buffer, sizeof(tx_buffer));
				printk("Rx:");
				buffer_dump(rx_buffer, received);
				break;

			case SWITCH_PERIPHERAL:
				res = (spim_initialized
				       ? switch_to_uarte()
				       : switch_to_spim());
				if (!res) {
					return 0;
				}
				break;
			}
		}
	}
	return 0;
}
