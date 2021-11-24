/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <drivers/spi.h>
#include <zephyr.h>

#include <sx126x/sx126x.h>

#include "sx12xx_common.h"
#include "sx126x_common.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(sx126x, CONFIG_LORA_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(semtech_sx1261) +
	     DT_NUM_INST_STATUS_OKAY(semtech_sx1262) +
	     DT_NUM_INST_STATUS_OKAY(st_stm32wl_subghz_radio) <= 1,
	     "Multiple SX126x instances in DT");


void SX126xWaitOnBusy(void);

static const struct device *const sx162x = DEVICE_DT_GET(DT_DRV_INST(0));

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
	const struct sx126x_config *cfg = sx162x->config;
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
		ret = spi_write_dt(&cfg->bus, &tx);
	} else {
		ret = spi_transceive_dt(&cfg->bus, &tx, &rx);
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
	const struct sx126x_config *cfg = sx162x->config;

	LOG_DBG("Enabling antenna switch");
	gpio_pin_set_dt(&cfg->antenna_enable, 1);
#else
	LOG_DBG("No antenna switch configured");
#endif
}

void SX126xAntSwOff(void)
{
#if HAVE_GPIO_ANTENNA_ENABLE
	const struct sx126x_config *cfg = sx162x->config;

	LOG_DBG("Disabling antenna switch");
	gpio_pin_set_dt(&cfg->antenna_enable, 0);
#else
	LOG_DBG("No antenna switch configured");
#endif
}

static void sx126x_set_tx_enable(int value)
{
#if HAVE_GPIO_TX_ENABLE
	const struct sx126x_config *cfg = sx162x->config;

	gpio_pin_set_dt(&cfg->tx_enable, value);
#endif
}

static void sx126x_set_rx_enable(int value)
{
#if HAVE_GPIO_RX_ENABLE
	const struct sx126x_config *cfg = sx162x->config;

	gpio_pin_set_dt(&cfg->rx_enable, value);
#endif
}

RadioOperatingModes_t SX126xGetOperatingMode(void)
{
	struct sx126x_data *data = sx162x->data;

	return data->mode;
}

void SX126xSetOperatingMode(RadioOperatingModes_t mode)
{
	struct sx126x_data *data = sx162x->data;

	LOG_DBG("SetOperatingMode: %s (%i)", sx126x_mode_name(mode), mode);

	data->mode = mode;

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
		sx126x_dio1_irq_disable(data);
		__fallthrough;
	default:
		sx126x_set_rx_enable(0);
		sx126x_set_tx_enable(0);
		break;
	}
}

uint32_t SX126xGetBoardTcxoWakeupTime(void)
{
	return DT_INST_PROP_OR(0, tcxo_power_startup_delay_ms, 0);
}

uint8_t SX126xGetDeviceId(void)
{
	return SX126X_DEVICE_ID;
}

void SX126xIoIrqInit(DioIrqHandler dioIrq)
{
	struct sx126x_data *data = sx162x->data;

	LOG_DBG("Configuring DIO IRQ callback");
	data->radio_dio_irq = dioIrq;
}

void SX126xIoTcxoInit(void)
{
#if DT_INST_NODE_HAS_PROP(0, dio3_tcxo_voltage)
	CalibrationParams_t cal = {
		.Value = 0x7F, /* Calibrate all */
	};

	LOG_DBG("TCXO on DIO3");

	/* Delay in units of 15.625 us (1/64 ms) */
	SX126xSetDio3AsTcxoCtrl(DT_INST_PROP(0, dio3_tcxo_voltage),
				DT_INST_PROP_OR(0, tcxo_power_startup_delay_ms, 0) << 6);
	SX126xCalibrate(cal);
#else
	LOG_DBG("No TCXO configured");
#endif
}

void SX126xIoRfSwitchInit(void)
{
	LOG_DBG("Configuring DIO2");
	SX126xSetDio2AsRfSwitchCtrl(DT_INST_PROP(0, dio2_tx_enable));
}

void SX126xReset(void)
{
	struct sx126x_data *data = sx162x->data;

	LOG_DBG("Resetting radio");

	sx126x_reset(data);

	/* Device transitions to standby on reset */
	data->mode = MODE_STDBY_RC;
}

void SX126xSetRfTxPower(int8_t power)
{
	LOG_DBG("power: %" PRIi8, power);
	SX126xSetTxParams(power, RADIO_RAMP_40_US);
}

void SX126xWaitOnBusy(void)
{
	struct sx126x_data *data = sx162x->data;

	while (sx126x_is_busy(data)) {
		k_sleep(K_MSEC(1));
	}
}

void SX126xWakeup(void)
{
	const struct sx126x_config *cfg = sx162x->config;
	struct sx126x_data *data = sx162x->data;
	int ret;

	/* Reenable DIO1 when waking up */
	sx126x_dio1_irq_enable(data);

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
	ret = spi_write_dt(&cfg->bus, &tx);
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
	data->mode = MODE_STDBY_RC;
}

uint32_t SX126xGetDio1PinState(void)
{
	struct sx126x_data *data = sx162x->data;

	return sx126x_get_dio1_pin_state(data);
}

static void sx126x_dio1_irq_work_handler(struct k_work *work)
{
	struct sx126x_data *data = sx162x->data;

	LOG_DBG("Processing DIO1 interrupt");
	if (!data->radio_dio_irq) {
		LOG_WRN("DIO1 interrupt without valid HAL IRQ callback.");
		return;
	}

	data->radio_dio_irq(NULL);
	if (Radio.IrqProcess) {
		Radio.IrqProcess();
	}

	/* Re-enable the interrupt if we are not in sleep mode */
	if (data->mode != MODE_SLEEP) {
		sx126x_dio1_irq_enable(data);
	}
}

static int sx126x_lora_init(const struct device *dev)
{
	const struct sx126x_config *cfg = dev->config;
	struct sx126x_data *data = dev->data;
	int ret;

	LOG_DBG("Initializing %s", dev->name);

	if (sx12xx_configure_pin(&cfg->antenna_enable, GPIO_OUTPUT_INACTIVE) ||
	    sx12xx_configure_pin(&cfg->rx_enable, GPIO_OUTPUT_INACTIVE) ||
	    sx12xx_configure_pin(&cfg->tx_enable, GPIO_OUTPUT_INACTIVE)) {
		return -EIO;
	}

	k_work_init(&data->dio1_irq_work, sx126x_dio1_irq_work_handler);

	ret = sx126x_variant_init(dev);
	if (ret) {
		LOG_ERR("Variant initialization failed");
		return ret;
	}

	if (!spi_is_ready(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
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

static const struct sx126x_config sx126x_0_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
	.antenna_enable = GPIO_DT_SPEC_INST_GET_OR(0, antenna_enable_gpios, {0}),
	.tx_enable = GPIO_DT_SPEC_INST_GET_OR(0, tx_enable_gpios, {0}),
	.rx_enable = GPIO_DT_SPEC_INST_GET_OR(0, rx_enable_gpios, {0})
};

static struct sx126x_data sx126x_0_data;

DEVICE_DT_INST_DEFINE(0, &sx126x_lora_init, NULL, &sx126x_0_data,
		      &sx126x_0_config, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		      &sx126x_lora_api);
