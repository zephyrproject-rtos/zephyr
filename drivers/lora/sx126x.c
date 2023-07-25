/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <sx126x/sx126x.h>

#include "sx12xx_common.h"
#include "sx126x_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx126x, CONFIG_LORA_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(semtech_sx1261) +
	     DT_NUM_INST_STATUS_OKAY(semtech_sx1262) +
	     DT_NUM_INST_STATUS_OKAY(st_stm32wl_subghz_radio) <= 1,
	     "Multiple SX126x instances in DT");

#define DIO2_TX_ENABLE DT_INST_PROP(0, dio2_tx_enable)

#define HAVE_DIO3_TCXO		DT_INST_NODE_HAS_PROP(0, dio3_tcxo_voltage)
#if HAVE_DIO3_TCXO
#define TCXO_DIO3_VOLTAGE	DT_INST_PROP(0, dio3_tcxo_voltage)
#endif

#if DT_INST_NODE_HAS_PROP(0, tcxo_power_startup_delay_ms)
#define TCXO_POWER_STARTUP_DELAY_MS			\
	DT_INST_PROP(0, tcxo_power_startup_delay_ms)
#else
#define TCXO_POWER_STARTUP_DELAY_MS	0
#endif

#define SX126X_CALIBRATION_ALL 0x7f

static const struct sx126x_config dev_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
#if HAVE_GPIO_ANTENNA_ENABLE
	.antenna_enable = GPIO_DT_SPEC_INST_GET(0, antenna_enable_gpios),
#endif
#if HAVE_GPIO_TX_ENABLE
	.tx_enable = GPIO_DT_SPEC_INST_GET(0, tx_enable_gpios),
#endif
#if HAVE_GPIO_RX_ENABLE
	.rx_enable = GPIO_DT_SPEC_INST_GET(0, rx_enable_gpios),
#endif
};

static struct sx126x_data dev_data;

void SX126xWaitOnBusy(void);

#define MODE(m) [MODE_##m] = #m
static const char *const mode_names[] = {
	MODE(SLEEP),
	MODE(STDBY_RC),
	MODE(STDBY_XOSC),
	MODE(FS),
	MODE(TX),
	MODE(RX),
	MODE(RX_DC),
	MODE(CAD),
};
#undef MODE

static const char *sx126x_mode_name(RadioOperatingModes_t m)
{
	static const char *unknown_mode = "unknown";

	if (m < ARRAY_SIZE(mode_names) && mode_names[m]) {
		return mode_names[m];
	} else {
		return unknown_mode;
	}
}

static int sx126x_spi_transceive(uint8_t *req_tx, uint8_t *req_rx,
				 size_t req_len, void *data_tx, void *data_rx,
				 size_t data_len)
{
	int ret;

	const struct spi_buf tx_buf[] = {
		{
			.buf = req_tx,
			.len = req_len,
		},
		{
			.buf = data_tx,
			.len = data_len
		}
	};

	const struct spi_buf rx_buf[] = {
		{
			.buf = req_rx,
			.len = req_len,
		},
		{
			.buf = data_rx,
			.len = data_len
		}
	};

	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf)
	};

	/* Wake the device if necessary */
	SX126xCheckDeviceReady();

	if (!req_rx && !data_rx) {
		ret = spi_write_dt(&dev_config.bus, &tx);
	} else {
		ret = spi_transceive_dt(&dev_config.bus, &tx, &rx);
	}

	if (ret < 0) {
		LOG_ERR("SPI transaction failed: %i", ret);
	}

	if (req_len >= 1 && req_tx[0] != RADIO_SET_SLEEP) {
		SX126xWaitOnBusy();
	}
	return ret;
}

uint8_t SX126xReadRegister(uint16_t address)
{
	uint8_t data;

	SX126xReadRegisters(address, &data, 1);

	return data;
}

void SX126xReadRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
	uint8_t req[] = {
		RADIO_READ_REGISTER,
		(address >> 8) & 0xff,
		address & 0xff,
		0,
	};

	LOG_DBG("Reading %" PRIu16 " registers @ 0x%" PRIx16, size, address);
	sx126x_spi_transceive(req, NULL, sizeof(req), NULL, buffer, size);
	LOG_HEXDUMP_DBG(buffer, size, "register_value");
}

void SX126xWriteRegister(uint16_t address, uint8_t value)
{
	SX126xWriteRegisters(address, &value, 1);
}

void SX126xWriteRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
	uint8_t req[] = {
		RADIO_WRITE_REGISTER,
		(address >> 8) & 0xff,
		address & 0xff,
	};

	LOG_DBG("Writing %" PRIu16 " registers @ 0x%" PRIx16
		": 0x%" PRIx8 " , ...",
		size, address, buffer[0]);
	sx126x_spi_transceive(req, NULL, sizeof(req), buffer, NULL, size);
}

uint8_t SX126xReadCommand(RadioCommands_t opcode,
			  uint8_t *buffer, uint16_t size)
{
	uint8_t tx_req[] = {
		opcode,
		0x00,
	};

	uint8_t rx_req[sizeof(tx_req)];

	LOG_DBG("Issuing opcode 0x%x (data size: %" PRIx16 ")",
		opcode, size);
	sx126x_spi_transceive(tx_req, rx_req, sizeof(rx_req),
			      NULL, buffer, size);
	LOG_DBG("-> status: 0x%" PRIx8, rx_req[1]);
	return rx_req[1];
}

void SX126xWriteCommand(RadioCommands_t opcode, uint8_t *buffer, uint16_t size)
{
	uint8_t req[] = {
		opcode,
	};

	LOG_DBG("Issuing opcode 0x%x w. %" PRIu16 " bytes of data",
		opcode, size);
	sx126x_spi_transceive(req, NULL, sizeof(req), buffer, NULL, size);
}

void SX126xReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
	uint8_t req[] = {
		RADIO_READ_BUFFER,
		offset,
		0x00,
	};

	LOG_DBG("Reading buffers @ 0x%" PRIx8 " (%" PRIu8 " bytes)",
		offset, size);
	sx126x_spi_transceive(req, NULL, sizeof(req), NULL, buffer, size);
}

void SX126xWriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
	uint8_t req[] = {
		RADIO_WRITE_BUFFER,
		offset,
	};

	LOG_DBG("Writing buffers @ 0x%" PRIx8 " (%" PRIu8 " bytes)",
		offset, size);
	sx126x_spi_transceive(req, NULL, sizeof(req), buffer, NULL, size);
}

void SX126xAntSwOn(void)
{
#if HAVE_GPIO_ANTENNA_ENABLE
	LOG_DBG("Enabling antenna switch");
	gpio_pin_set_dt(&dev_config.antenna_enable, 1);
#else
	LOG_DBG("No antenna switch configured");
#endif
}

void SX126xAntSwOff(void)
{
#if HAVE_GPIO_ANTENNA_ENABLE
	LOG_DBG("Disabling antenna switch");
	gpio_pin_set_dt(&dev_config.antenna_enable, 0);
#else
	LOG_DBG("No antenna switch configured");
#endif
}

static void sx126x_set_tx_enable(int value)
{
#if HAVE_GPIO_TX_ENABLE
	gpio_pin_set_dt(&dev_config.tx_enable, value);
#endif
}

static void sx126x_set_rx_enable(int value)
{
#if HAVE_GPIO_RX_ENABLE
	gpio_pin_set_dt(&dev_config.rx_enable, value);
#endif
}

RadioOperatingModes_t SX126xGetOperatingMode(void)
{
	return dev_data.mode;
}

void SX126xSetOperatingMode(RadioOperatingModes_t mode)
{
	LOG_DBG("SetOperatingMode: %s (%i)", sx126x_mode_name(mode), mode);

	dev_data.mode = mode;

	/* To avoid inadvertently putting the RF switch in an
	 * undefined state, first disable the port we don't want to
	 * use and then enable the other one.
	 */
	switch (mode) {
	case MODE_TX:
		sx126x_set_rx_enable(0);
		sx126x_set_tx_enable(1);
		break;

	case MODE_RX:
	case MODE_RX_DC:
	case MODE_CAD:
		sx126x_set_tx_enable(0);
		sx126x_set_rx_enable(1);
		break;

	case MODE_SLEEP:
		/* Additionally disable the DIO1 interrupt to save power */
		sx126x_dio1_irq_disable(&dev_data);
		__fallthrough;
	default:
		sx126x_set_rx_enable(0);
		sx126x_set_tx_enable(0);
		break;
	}
}

uint32_t SX126xGetBoardTcxoWakeupTime(void)
{
	return TCXO_POWER_STARTUP_DELAY_MS;
}

uint8_t SX126xGetDeviceId(void)
{
	return SX126X_DEVICE_ID;
}

void SX126xIoIrqInit(DioIrqHandler dioIrq)
{
	LOG_DBG("Configuring DIO IRQ callback");
	dev_data.radio_dio_irq = dioIrq;
}

void SX126xIoTcxoInit(void)
{
#if HAVE_DIO3_TCXO
	CalibrationParams_t cal = {
		.Value = SX126X_CALIBRATION_ALL,
	};

	LOG_DBG("TCXO on DIO3");

	/* Delay in units of 15.625 us (1/64 ms) */
	SX126xSetDio3AsTcxoCtrl(TCXO_DIO3_VOLTAGE,
				TCXO_POWER_STARTUP_DELAY_MS << 6);
	SX126xCalibrate(cal);
#else
	LOG_DBG("No TCXO configured");
#endif
}

void SX126xIoRfSwitchInit(void)
{
	LOG_DBG("Configuring DIO2");
	SX126xSetDio2AsRfSwitchCtrl(DIO2_TX_ENABLE);
}

void SX126xReset(void)
{
	LOG_DBG("Resetting radio");

	sx126x_reset(&dev_data);

	/* Device transitions to standby on reset */
	dev_data.mode = MODE_STDBY_RC;
}

void SX126xSetRfTxPower(int8_t power)
{
	LOG_DBG("power: %" PRIi8, power);
	sx126x_set_tx_params(power, RADIO_RAMP_40_US);
}

void SX126xWaitOnBusy(void)
{
	while (sx126x_is_busy(&dev_data)) {
		k_sleep(K_MSEC(1));
	}
}

void SX126xWakeup(void)
{
	int ret;

	/* Reenable DIO1 when waking up */
	sx126x_dio1_irq_enable(&dev_data);

	uint8_t req[] = { RADIO_GET_STATUS, 0 };
	const struct spi_buf tx_buf = {
		.buf = req,
		.len = sizeof(req),
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	LOG_DBG("Sending GET_STATUS");
	ret = spi_write_dt(&dev_config.bus, &tx);
	if (ret < 0) {
		LOG_ERR("SPI transaction failed: %i", ret);
		return;
	}

	LOG_DBG("Waiting for device...");
	SX126xWaitOnBusy();
	LOG_DBG("Device ready");
	/* This function is only called from sleep mode
	 * All edges on the SS SPI pin will transition the modem to
	 * standby mode (via startup)
	 */
	dev_data.mode = MODE_STDBY_RC;
}

uint32_t SX126xGetDio1PinState(void)
{
	return sx126x_get_dio1_pin_state(&dev_data);
}

static void sx126x_dio1_irq_work_handler(struct k_work *work)
{
	LOG_DBG("Processing DIO1 interrupt");
	if (!dev_data.radio_dio_irq) {
		LOG_WRN("DIO1 interrupt without valid HAL IRQ callback.");
		return;
	}

	dev_data.radio_dio_irq(NULL);
	if (Radio.IrqProcess) {
		Radio.IrqProcess();
	}

	/* Re-enable the interrupt if we are not in sleep mode */
	if (dev_data.mode != MODE_SLEEP) {
		sx126x_dio1_irq_enable(&dev_data);
	}
}

static int sx126x_lora_init(const struct device *dev)
{
	const struct sx126x_config *config = dev->config;
	int ret;

	LOG_DBG("Initializing sx126x");

	if (sx12xx_configure_pin(antenna_enable, GPIO_OUTPUT_INACTIVE) ||
	    sx12xx_configure_pin(rx_enable, GPIO_OUTPUT_INACTIVE) ||
	    sx12xx_configure_pin(tx_enable, GPIO_OUTPUT_INACTIVE)) {
		return -EIO;
	}

	k_work_init(&dev_data.dio1_irq_work, sx126x_dio1_irq_work_handler);

	ret = sx126x_variant_init(dev);
	if (ret) {
		LOG_ERR("Variant initialization failed");
		return ret;
	}

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	ret = sx12xx_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize SX12xx common");
		return ret;
	}

	return 0;
}

static const struct lora_driver_api sx126x_lora_api = {
	.config = sx12xx_lora_config,
	.send = sx12xx_lora_send,
	.send_async = sx12xx_lora_send_async,
	.recv = sx12xx_lora_recv,
	.recv_async = sx12xx_lora_recv_async,
	.test_cw = sx12xx_lora_test_cw,
};

DEVICE_DT_INST_DEFINE(0, &sx126x_lora_init, NULL, &dev_data,
		      &dev_config, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		      &sx126x_lora_api);
