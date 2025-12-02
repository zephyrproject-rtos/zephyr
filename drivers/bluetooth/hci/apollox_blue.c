/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Ambiq Apollox Blue SoC extended driver for SPI based HCI.
 */

#define DT_DRV_COMPAT ambiq_bt_hci_spi

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_raw.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_apollox_driver);

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ambiq.h>

#include <soc.h>
#include "apollox_blue.h"
#if (CONFIG_SOC_SERIES_APOLLO4X)
#include "am_devices_cooper.h"
#elif (CONFIG_SOC_SERIES_APOLLO3X)
#include "am_apollo3_bt_support.h"
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

#define HCI_SPI_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(ambiq_bt_hci_spi)
#define SPI_DEV_NODE DT_BUS(HCI_SPI_NODE)

#if (CONFIG_SOC_SERIES_APOLLO5X)
#define CLK_32M_NODE DT_NODELABEL(xo32m_xtal)
#define CLK_32K_NODE DT_NODELABEL(xo32k_xtal)
#else
#define CLK_32M_NODE DT_NODELABEL(xo32m)
#define CLK_32K_NODE DT_NODELABEL(xo32k)
#endif /* CONFIG_SOC_SERIES_APOLLO5X */
/* Command/response for SPI operation */
#define SPI_WRITE   0x80
#define SPI_READ    0x04
#define READY_BYTE0 0x68
#define READY_BYTE1 0xA8

/* Maximum attempts of SPI write */
#define SPI_WRITE_TIMEOUT 200

#define SPI_MAX_RX_MSG_LEN 258

#if (CONFIG_SOC_SERIES_APOLLO5X)
#define EM9305_STS_CHK_CNT_MAX         10       /* check EM9305 status cnt */
#define WAIT_EM9305_RDY_TIMEOUT        12000    /* EM9305 timeout value.
						 * Assume worst case cold start counter (1.2 sec)
						 */
#define EM9305_BUFFER_SIZE             259      /* Length of RX buffer */
#define EM9305_SPI_HEADER_TX           0x42     /* SPI TX header byte */
#define EM9305_SPI_HEADER_RX           0x81     /* SPI RX header byte */
#define EM9305_STS1_READY_VALUE        0xC0     /* SPI Ready byte */

uint8_t active_state_entered_evt[] = {0x04, 0xFF, 0x01, 0x01};
static const struct gpio_dt_spec irq_gpio = GPIO_DT_SPEC_GET(HCI_SPI_NODE, irq_gpios);
static const struct gpio_dt_spec rst_gpio = GPIO_DT_SPEC_GET(HCI_SPI_NODE, reset_gpios);
static const struct gpio_dt_spec cs_gpio = GPIO_DT_SPEC_GET(SPI_DEV_NODE, cs_gpios);
static const struct gpio_dt_spec cm_gpio = GPIO_DT_SPEC_GET(HCI_SPI_NODE, cm_gpios);

static struct gpio_callback irq_gpio_cb;
static bool spiTxInProgress;                    /* SPI lock when a transmission is in progress */
static volatile bool Em9305status_ok;
am_devices_em9305_callback_t g_Em9305cb;

void am_devices_em9305_set_reset_state(bool data)
{
	gpio_pin_set_dt(&rst_gpio, data);
}

bool am_devices_em9305_get_reset_state(void)
{
	return gpio_pin_get_dt(&rst_gpio);
}

extern void bt_packet_irq_isr(const struct device *unused1, struct gpio_callback *unused2,
			      uint32_t unused3);

static bool irq_pin_state(void)
{
	int pin_state;

	pin_state = gpio_pin_get_dt(&irq_gpio);

	return pin_state > 0;
}

static void bt_em9305_cs_set(void)
{
	gpio_pin_set_dt(&cs_gpio, 1);
}

static void bt_em9305_cs_release(void)
{
	gpio_pin_set_dt(&cs_gpio, 0);
}

static void bt_em9305_wait_ready(void)
{
	uint16_t i;

	for (i = 0; i < WAIT_EM9305_RDY_TIMEOUT; i++) {
		if (irq_pin_state()) {
			break;
		}
		k_busy_wait(100);
	}

	if (i >= WAIT_EM9305_RDY_TIMEOUT) {
		LOG_WRN("EM9305 ready timeout after %d ms", WAIT_EM9305_RDY_TIMEOUT * 100 / 1000);
	}
}

static uint8_t am_devices_em9305_tx_starts(bt_spi_transceive_fun transceive)
{
	uint8_t sCommand[2] = {EM9305_SPI_HEADER_TX, 0x00};
	uint8_t sStas[2] = {0, 0};
	uint8_t ret = 0;

	/* Indicates that a SPI transfer is in progress */
	spiTxInProgress = true;
	/* Select the EM9305 */
	bt_em9305_cs_set();
	/* wait em9305 ready */
	bt_em9305_wait_ready();
	/* check ready again */

	if (!irq_pin_state()) {
		bt_em9305_cs_release();
		spiTxInProgress = false;
		LOG_ERR("wait em9305 ready timeout");
		return ret;
	}

	for (uint32_t i = 0; i < EM9305_STS_CHK_CNT_MAX; i++) {
		/* Select the EM9305 */
		bt_em9305_cs_set();
		ret = transceive(sCommand, 2, sStas, 2);
		if (ret) {
			LOG_ERR("%s: spi write error %d", __func__, ret);
			return ret;
		}

		if (ret != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("%s: ret =%d ", __func__, ret);
			return 0;
		}
		/* Check if the EM9305 is ready and the rx buffer size is not zero. */
		if ((sStas[0] == EM9305_STS1_READY_VALUE) && (sStas[1] != 0x00)) {
			break;
		}
		bt_em9305_cs_release();
	}

	return sStas[1];
}

static void am_devices_em9305_tx_ends(void)
{
	/* Deselect the EM9305 */
	bt_em9305_cs_release();
	/* Indicates that the SPI transfer is finished */
	spiTxInProgress = false;
}

int bt_apollo_spi_send(uint8_t *pui8Values, uint16_t ui32NumBytes, bt_spi_transceive_fun transceive)
{
	uint32_t ui32ErrorStatus = AM_DEVICES_EM9305_STATUS_SUCCESS;
	int ret = -ENOTSUP;
	uint8_t data[EM9305_BUFFER_SIZE];
	uint8_t em9305BufSize = 0;

	if (ui32NumBytes <= EM9305_BUFFER_SIZE) {
		for (uint32_t i = 0; i < ui32NumBytes;) {
			em9305BufSize = am_devices_em9305_tx_starts(transceive);

			if (em9305BufSize == 0x00) {
				ui32ErrorStatus = AM_DEVICES_EM9305_RX_FULL;
				printf("EM9305_RX_FULL\r\n");
				am_devices_em9305_tx_ends();
				break;
			}
			uint32_t len = (em9305BufSize < (ui32NumBytes - i)) ? em9305BufSize
									    : (ui32NumBytes - i);

			/* check again if there is room to send more data */
			if ((len > 0) && (em9305BufSize)) {
				memcpy(data, pui8Values + i, len);
				i += len;

				/* Write to the IOM. */
				/* Transmit the message */
				ret = transceive(data, len, NULL, 0);

				if (ret != AM_HAL_STATUS_SUCCESS) {
					ui32ErrorStatus = AM_DEVICES_EM9305_DATA_TRANSFER_ERROR;
					LOG_ERR("%s: ret= %d", __func__, ret);
				}
			}
			am_devices_em9305_tx_ends();
		}
	} else {
		ui32ErrorStatus = AM_DEVICES_EM9305_DATA_LENGTH_ERROR;
		LOG_ERR("%s: error (STATUS ERROR) Packet Too Large", __func__);
	}

	return ui32ErrorStatus;
}

static void bt_em9305_controller_reset(void)
{
	/* Reset the controller*/
	gpio_pin_set_dt(&rst_gpio, 0);
	/* Take controller out of reset */
	k_sleep(K_MSEC(2));
	gpio_pin_set_dt(&rst_gpio, 1);
	k_sleep(K_MSEC(2));
	gpio_pin_set_dt(&rst_gpio, 0);
}

uint32_t am_devices_em9305_init(am_devices_em9305_callback_t *cb)
{
	if ((!cb) || (!cb->write) || (!cb->reset)) {
		return AM_DEVICES_EM9305_STATUS_ERROR;
	}
	/* Register the callback functions */
	g_Em9305cb.write = cb->write;
	g_Em9305cb.reset = cb->reset;
	g_Em9305cb.reset();
	/* wait for 9305 activated status ok */
	while (!Em9305status_ok) {
		;
	}

	return AM_DEVICES_EM9305_STATUS_SUCCESS;
}
#endif /* CONFIG_SOC_SERIES_APOLLO5X */

#if (CONFIG_SOC_SERIES_APOLLO4X)
static const struct gpio_dt_spec irq_gpio = GPIO_DT_SPEC_GET(HCI_SPI_NODE, irq_gpios);
static const struct gpio_dt_spec rst_gpio = GPIO_DT_SPEC_GET(HCI_SPI_NODE, reset_gpios);
static const struct gpio_dt_spec cs_gpio = GPIO_DT_SPEC_GET(SPI_DEV_NODE, cs_gpios);
static const struct gpio_dt_spec clkreq_gpio = GPIO_DT_SPEC_GET(HCI_SPI_NODE, clkreq_gpios);

static struct gpio_callback irq_gpio_cb;
static struct gpio_callback clkreq_gpio_cb;

static const struct device *clk32m_dev = DEVICE_DT_GET(CLK_32M_NODE);
static const struct device *clk32k_dev = DEVICE_DT_GET(CLK_32K_NODE);
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

extern void bt_packet_irq_isr(const struct device *unused1, struct gpio_callback *unused2,
			      uint32_t unused3);

void bt_apollo_rcv_isr_preprocess(void)
{
#if (CONFIG_SOC_SERIES_APOLLO3X)
	am_apollo3_bt_isr_pre();
#endif /* CONFIG_SOC_SERIES_APOLLO3X */
}

#if (CONFIG_SOC_SERIES_APOLLO4X)
static bool irq_pin_state(void)
{
	int pin_state;

	pin_state = gpio_pin_get_dt(&irq_gpio);
	LOG_DBG("IRQ Pin: %d", pin_state);
	return pin_state > 0;
}

static bool clkreq_pin_state(void)
{
	int pin_state;

	pin_state = gpio_pin_get_dt(&clkreq_gpio);
	LOG_DBG("CLKREQ Pin: %d", pin_state);
	return pin_state > 0;
}

static void bt_clkreq_isr(const struct device *unused1, struct gpio_callback *unused2,
			  uint32_t unused3)
{
	if (clkreq_pin_state()) {
		/* Enable XO32MHz */
		clock_control_on(clk32m_dev,
				 (clock_control_subsys_t)CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_BLE);
		gpio_pin_interrupt_configure_dt(&clkreq_gpio, GPIO_INT_EDGE_FALLING);
	} else {
		/* Disable XO32MHz */
		clock_control_off(clk32m_dev,
				  (clock_control_subsys_t)CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_BLE);
		gpio_pin_interrupt_configure_dt(&clkreq_gpio, GPIO_INT_EDGE_RISING);
	}
}

static void bt_apollo_controller_ready_wait(void)
{
	/* The CS pin is used to wake up the controller as well. If the controller is not ready
	 * to receive the SPI packet, need to inactivate the CS at first and reconfigure the pin
	 * to CS function again before next sending attempt.
	 */
	gpio_pin_configure_dt(&cs_gpio, GPIO_OUTPUT_INACTIVE);
	k_busy_wait(200);
	PINCTRL_DT_DEFINE(SPI_DEV_NODE);
	pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(SPI_DEV_NODE), PINCTRL_STATE_DEFAULT);
	k_busy_wait(2000);
}

static void bt_apollo_controller_reset(void)
{
	/* Reset the controller*/
	gpio_pin_set_dt(&rst_gpio, 1);

	/* Take controller out of reset */
	k_sleep(K_MSEC(10));
	gpio_pin_set_dt(&rst_gpio, 0);

	/* Give the controller some time to boot */
	k_sleep(K_MSEC(500));
}
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

#if !(CONFIG_SOC_SERIES_APOLLO5X)
int bt_apollo_spi_send(uint8_t *data, uint16_t len, bt_spi_transceive_fun transceive)
{
	int ret = -ENOTSUP;

#if (CONFIG_SOC_SERIES_APOLLO4X)
	uint8_t command[1] = {SPI_WRITE};
	uint8_t response[2] = {0, 0};
	uint16_t fail_count = 0;

	do {
		/* Check if the controller is ready to receive the HCI packets. */
		ret = transceive(command, 1, response, 2);
		if ((response[0] != READY_BYTE0) || (response[1] != READY_BYTE1) || ret) {
			bt_apollo_controller_ready_wait();
		} else {
			/* Transmit the message */
			ret = transceive(data, len, NULL, 0);
			if (ret) {
				LOG_ERR("SPI write error %d", ret);
			}
			break;
		}
	} while (fail_count++ < SPI_WRITE_TIMEOUT);
#elif (CONFIG_SOC_SERIES_APOLLO3X)
	ret = transceive(data, len, NULL, 0);
	if ((ret) && (ret != AM_HAL_BLE_STATUS_SPI_NOT_READY)) {
		LOG_ERR("SPI write error %d", ret);
	}
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

	return ret;
}
#endif /* CONFIG_SOC_SERIES_APOLLO5X */

int bt_apollo_spi_rcv(uint8_t *data, uint16_t *len, bt_spi_transceive_fun transceive)
{
#if (CONFIG_SOC_SERIES_APOLLO5X)
	{
		uint8_t sCommand[2] = {EM9305_SPI_HEADER_RX, 0x0};
		uint8_t sStas[2];
		uint8_t ui8RxBytes = 0;
		uint8_t ret = 0;

		*len = 0;
		/* Check if the SPI is free */
		if (spiTxInProgress) {
			/* TX in progress -> Ignore RDY interrupt */
			LOG_ERR("EM9305 SPI TX in progress");
			return AM_DEVICES_EM9305_TX_BUSY;
		}

		/* Check if they are still data to read */
		if (!irq_pin_state()) {
			/* No data */
			return AM_DEVICES_EM9305_NO_DATA_TX;
		}

		do {
			for (uint32_t i = 0; i < EM9305_STS_CHK_CNT_MAX; i++) {
				/* Select the EM9305 */
				bt_em9305_cs_set();
				ret = transceive(sCommand, 2, sStas, 2);

				if (ret != AM_HAL_STATUS_SUCCESS) {
					return AM_DEVICES_EM9305_CMD_TRANSFER_ERROR;
				}

				/* Check if the EM9305 is ready and the tx data size. */
				if ((sStas[0] == EM9305_STS1_READY_VALUE) && (sStas[1] != 0x00)) {
					break;
				}
				bt_em9305_cs_release();
			}

			/* Check that the EM9305 is ready or the receive FIFO is not full. */
			if ((sStas[0] != EM9305_STS1_READY_VALUE) || (sStas[1] == 0x00)) {
				bt_em9305_cs_release();
				LOG_ERR("EM9305 Not Ready sStas.byte0 = 0x%02x, sStas.byte1 = "
					"0x%02x\n",
					sStas[0], sStas[1]);
				return AM_DEVICES_EM9305_NOT_READY;
			}

			/* Set the number of bytes to receive. */
			ui8RxBytes = sStas[1];

			if (irq_pin_state() && (ui8RxBytes != 0)) {
				if ((*len + ui8RxBytes) > EM9305_BUFFER_SIZE) {
					/* Error. Packet too large. */
					LOG_ERR("HCI RX Error (STATUS ERROR) Packet Too Large %d, "
						"%d\n",
						sStas[0], sStas[1]);
					return AM_DEVICES_EM9305_DATA_LENGTH_ERROR;
				}

				/* Read to the IOM. */
				ret = transceive(NULL, 0, data + *len, ui8RxBytes);

				if (ret != AM_HAL_STATUS_SUCCESS) {
					LOG_ERR(" bt_apollo_spi_rcv ret =%d\n", ret);
					return AM_DEVICES_EM9305_DATA_TRANSFER_ERROR;
				}
				*len += ui8RxBytes;
			}
			/* Deselect the EM9305 */
			bt_em9305_cs_release();

		} while (irq_pin_state());

		return AM_DEVICES_EM9305_STATUS_SUCCESS;
	}
#else
	int ret = -ENOTSUP;
	uint8_t response[2] = {0, 0};
	uint16_t read_size = 0;

	do {
#if (CONFIG_SOC_SERIES_APOLLO4X)
		/* Skip if the IRQ pin is not in high state */
		if (!irq_pin_state()) {
			ret = -1;
			break;
		}

		/* Check the available packet bytes */
		uint8_t command[1] = {SPI_READ};
		ret = transceive(command, 1, response, 2);
		if (ret) {
			break;
		}
#elif (CONFIG_SOC_SERIES_APOLLO3X)
		/* Skip if the IRQ bit is not set */
		if (!BLEIFn(0)->BSTATUS_b.BLEIRQ) {
			ret = -1;
			break;
		}

		/* Check the available packet bytes */
		ret = transceive(NULL, 0, response, 2);
		if (ret) {
			break;
		}
#else
		break;
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

		/* Check if the read size is acceptable */
		read_size = (uint16_t)(response[0] | response[1] << 8);
		if ((read_size == 0) || (read_size > SPI_MAX_RX_MSG_LEN)) {
			ret = -1;
			break;
		}

		*len = read_size;

		/* Read the HCI data from controller */
		ret = transceive(NULL, 0, data, read_size);

		if (ret) {
			LOG_ERR("SPI read error %d", ret);
			break;
		}
	} while (0);

	return ret;
#endif
}

bool bt_apollo_vnd_rcv_ongoing(uint8_t *data, uint16_t len)
{
#if (CONFIG_SOC_SERIES_APOLLO4X)
	/* The vendor specific handshake command/response is incompatible with
	 * standard Bluetooth HCI format, need to handle the received packets
	 * specifically.
	 */
	if (am_devices_cooper_get_initialize_state() != AM_DEVICES_COOPER_STATE_INITIALIZED) {
		am_devices_cooper_handshake_recv(data, len);
		return true;
	} else {
		return false;
	}
#elif (CONFIG_SOC_SERIES_APOLLO5X)
	bool ret = false;

	if (memcmp(data, active_state_entered_evt, sizeof(active_state_entered_evt)) == 0) {
		printf("em9305 enter active state \r\n");
		Em9305status_ok = true;
		ret = true;
	}

	return ret;
#else
	return false;
#endif /* CONFIG_SOC_SERIES_APOLLO4X */
}

int bt_hci_transport_setup(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret = 0;

#if (CONFIG_SOC_SERIES_APOLLO5X)
	/* Configure RST pin and hold BLE in Reset */
	ret = gpio_pin_configure_dt(&rst_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	/* Configure IRQ pin and register the callback */
	ret = gpio_pin_configure_dt(&irq_gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&irq_gpio_cb, bt_packet_irq_isr, BIT(irq_gpio.pin));
	ret = gpio_add_callback(irq_gpio.port, &irq_gpio_cb);
	if (ret) {
		return ret;
	}

	/* Configure the interrupt edge for IRQ pin */
	gpio_pin_interrupt_configure_dt(&irq_gpio, GPIO_INT_EDGE_RISING);
#elif (CONFIG_SOC_SERIES_APOLLO4X)
	/* Configure the XO32MHz and XO32kHz clocks.*/
	clock_control_configure(clk32k_dev, NULL, NULL);
	clock_control_configure(clk32m_dev, NULL, NULL);

	/* Enable XO32kHz for Controller */
	clock_control_on(clk32k_dev, (clock_control_subsys_t)CLOCK_CONTROL_AMBIQ_TYPE_LFXTAL);

	/* Enable XO32MHz for Controller */
	clock_control_on(clk32m_dev, (clock_control_subsys_t)CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_BLE);

	/* Configure RST pin and hold BLE in Reset */
	ret = gpio_pin_configure_dt(&rst_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	/* Configure IRQ pin and register the callback */
	ret = gpio_pin_configure_dt(&irq_gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&irq_gpio_cb, bt_packet_irq_isr, BIT(irq_gpio.pin));
	ret = gpio_add_callback(irq_gpio.port, &irq_gpio_cb);
	if (ret) {
		return ret;
	}

	/* Configure CLKREQ pin and register the callback */
	ret = gpio_pin_configure_dt(&clkreq_gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&clkreq_gpio_cb, bt_clkreq_isr, BIT(clkreq_gpio.pin));
	ret = gpio_add_callback(clkreq_gpio.port, &clkreq_gpio_cb);
	if (ret) {
		return ret;
	}

	/* Configure the interrupt edge for CLKREQ pin */
	gpio_pin_interrupt_configure_dt(&clkreq_gpio, GPIO_INT_EDGE_RISING);

	/* Take controller out of reset */
	k_sleep(K_MSEC(10));
	gpio_pin_set_dt(&rst_gpio, 0);

	/* Give the controller some time to boot */
	k_sleep(K_MSEC(500));

	/* Configure the interrupt edge for IRQ pin */
	gpio_pin_interrupt_configure_dt(&irq_gpio, GPIO_INT_EDGE_RISING);
#elif (CONFIG_SOC_SERIES_APOLLO3X)
	IRQ_CONNECT(DT_IRQN(SPI_DEV_NODE), DT_IRQ(SPI_DEV_NODE, priority), bt_packet_irq_isr, 0, 0);
#endif /* CONFIG_SOC_SERIES_APOLLO5X */

	return ret;
}

int bt_apollo_controller_init(spi_transmit_fun transmit)
{
	int ret = 0;

#if (CONFIG_SOC_SERIES_APOLLO5X)
	am_devices_em9305_callback_t cb = {
		.write = transmit,
		.reset = bt_em9305_controller_reset,
	};

	/* Initialize the BLE controller */
	ret = am_devices_em9305_init(&cb);

	if (ret == AM_DEVICES_EM9305_STATUS_SUCCESS) {
		LOG_DBG("bt controller initialized\r\n");
	} else {
		LOG_DBG("bt controller initialization fail\r\n");
	}
#elif (CONFIG_SOC_SERIES_APOLLO4X)
	am_devices_cooper_callback_t cb = {
		.write = transmit,
		.reset = bt_apollo_controller_reset,
	};

	/* Initialize the BLE controller */
	ret = am_devices_cooper_init(&cb);
	if (ret == AM_DEVICES_COOPER_STATUS_SUCCESS) {
		am_devices_cooper_set_initialize_state(AM_DEVICES_COOPER_STATE_INITIALIZED);
		LOG_INF("BT controller initialized");
	} else {
		am_devices_cooper_set_initialize_state(AM_DEVICES_COOPER_STATE_INITIALIZE_FAIL);
		LOG_ERR("BT controller initialization fail");
	}
#elif (CONFIG_SOC_SERIES_APOLLO3X)
	ret = am_apollo3_bt_controller_init();
	if (ret == AM_HAL_STATUS_SUCCESS) {
		LOG_INF("BT controller initialized");
	} else {
		LOG_ERR("BT controller initialization fail");
	}

	irq_enable(DT_IRQN(SPI_DEV_NODE));
#endif /* CONFIG_SOC_SERIES_APOLLO5X */

	return ret;
}

int bt_apollo_controller_deinit(void)
{
	int ret = 0;

#if (CONFIG_SOC_SERIES_APOLLO4X)
	/* Keep the Controller in resetting state */
	gpio_pin_set_dt(&rst_gpio, 1);

	/* Disable XO32MHz */
	clock_control_off(clk32m_dev, (clock_control_subsys_t)CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_BLE);
	/* Disable XO32kHz  */
	clock_control_off(clk32k_dev, (clock_control_subsys_t)CLOCK_CONTROL_AMBIQ_TYPE_LFXTAL);

	/* Disable GPIOs */
	gpio_pin_configure_dt(&irq_gpio, GPIO_DISCONNECTED);
	gpio_pin_configure_dt(&clkreq_gpio, GPIO_DISCONNECTED);
	gpio_remove_callback(clkreq_gpio.port, &clkreq_gpio_cb);
	gpio_remove_callback(irq_gpio.port, &irq_gpio_cb);
#elif (CONFIG_SOC_SERIES_APOLLO3X)
	irq_disable(DT_IRQN(SPI_DEV_NODE));

	ret = am_apollo3_bt_controller_deinit();
	if (ret == AM_HAL_STATUS_SUCCESS) {
		LOG_INF("BT controller deinitialized");
	} else {
		ret = -EPERM;
		LOG_ERR("BT controller deinitialization fails");
	}
#else
	ret = -ENOTSUP;
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

	return ret;
}

#if (CONFIG_SOC_SERIES_APOLLO4X)
static int bt_apollo_set_nvds(void)
{
	int ret;
	struct net_buf *buf;

#if defined(CONFIG_BT_HCI_RAW)
	struct bt_hci_cmd_hdr hdr;

	hdr.opcode = sys_cpu_to_le16(HCI_VSC_UPDATE_NVDS_CFG_CMD_OPCODE);
	hdr.param_len = HCI_VSC_UPDATE_NVDS_CFG_CMD_LENGTH;
	buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, &hdr, sizeof(hdr));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, &am_devices_cooper_nvds[0], HCI_VSC_UPDATE_NVDS_CFG_CMD_LENGTH);
	ret = bt_send(buf);

	if (!ret) {
		/* Give some time to make NVDS take effect in BLE controller */
		k_sleep(K_MSEC(5));

		/* Need to send reset command to make the NVDS take effect */
		hdr.opcode = sys_cpu_to_le16(BT_HCI_OP_RESET);
		hdr.param_len = 0;
		buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, &hdr, sizeof(hdr));
		if (!buf) {
			return -ENOBUFS;
		}

		ret = bt_send(buf);
	}
#else
	uint8_t *p;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	p = net_buf_add(buf, HCI_VSC_UPDATE_NVDS_CFG_CMD_LENGTH);
	memcpy(p, &am_devices_cooper_nvds[0], HCI_VSC_UPDATE_NVDS_CFG_CMD_LENGTH);
	ret = bt_hci_cmd_send_sync(HCI_VSC_UPDATE_NVDS_CFG_CMD_OPCODE, buf, NULL);

	if (!ret) {
		/* Give some time to make NVDS take effect in BLE controller */
		k_sleep(K_MSEC(5));
	}
#endif /* defined(CONFIG_BT_HCI_RAW) */

	return ret;
}
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

int bt_apollo_vnd_setup(void)
{
	int ret = 0;

#if (CONFIG_SOC_SERIES_APOLLO4X)
	/* Set the NVDS parameters to BLE controller */
	ret = bt_apollo_set_nvds();
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

	return ret;
}

int bt_apollo_dev_init(void)
{
#if (CONFIG_SOC_SERIES_APOLLO4X)
	if (!gpio_is_ready_dt(&irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&rst_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&clkreq_gpio)) {
		LOG_ERR("CLKREQ GPIO device not ready");
		return -ENODEV;
	}
#elif (CONFIG_SOC_SERIES_APOLLO5X)
	if (!gpio_is_ready_dt(&irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&rst_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cm_gpio)) {
		LOG_ERR("CM GPIO device not ready");
		return -ENODEV;
	}
#endif /* CONFIG_SOC_SERIES_APOLLO4X */

	return 0;
}
