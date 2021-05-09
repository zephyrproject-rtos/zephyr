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

#include <logging/log.h>
LOG_MODULE_REGISTER(sx126x, CONFIG_LORA_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1261)
#define DT_DRV_COMPAT semtech_sx1261
#define SX126X_DEVICE_ID SX1261
#elif DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1262)
#define DT_DRV_COMPAT semtech_sx1262
#define SX126X_DEVICE_ID SX1262
#else
#error No SX126x instance in device tree.
#endif

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(semtech_sx1261) +
	     DT_NUM_INST_STATUS_OKAY(semtech_sx1262) <= 1,
	     "Multiple SX12xx instances in DT");

#define HAVE_GPIO_CS		DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
#define HAVE_GPIO_ANTENNA_ENABLE			\
	DT_INST_NODE_HAS_PROP(0, antenna_enable_gpios)
#define HAVE_GPIO_TX_ENABLE	DT_INST_NODE_HAS_PROP(0, tx_enable_gpios)
#define HAVE_GPIO_RX_ENABLE	DT_INST_NODE_HAS_PROP(0, rx_enable_gpios)

#define GPIO_CS_LABEL		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0)
#define GPIO_CS_PIN		DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define GPIO_CS_FLAGS		DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0)

#define GPIO_RESET_PIN		DT_INST_GPIO_PIN(0, reset_gpios)
#define GPIO_BUSY_PIN		DT_INST_GPIO_PIN(0, busy_gpios)
#define GPIO_DIO1_PIN		DT_INST_GPIO_PIN(0, dio1_gpios)
#define GPIO_ANTENNA_ENABLE_PIN	DT_INST_GPIO_PIN(0, antenna_enable_gpios)
#define GPIO_TX_ENABLE_PIN	DT_INST_GPIO_PIN(0, tx_enable_gpios)
#define GPIO_RX_ENABLE_PIN	DT_INST_GPIO_PIN(0, rx_enable_gpios)

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

struct sx126x_data {
	const struct device *reset;
	const struct device *busy;
	const struct device *dio1;
	struct gpio_callback dio1_irq_callback;
	struct k_work dio1_irq_work;
	DioIrqHandler *radio_dio_irq;
#if HAVE_GPIO_ANTENNA_ENABLE
	const struct device *antenna_enable;
#endif
#if HAVE_GPIO_TX_ENABLE
	const struct device *tx_enable;
#endif
#if HAVE_GPIO_RX_ENABLE
	const struct device *rx_enable;
#endif
	const struct device *spi;
	struct spi_config spi_cfg;
#if HAVE_GPIO_CS
	struct spi_cs_control spi_cs;
#endif
	RadioOperatingModes_t mode;
} dev_data;


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
		ret = spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);
	} else {
		ret = spi_transceive(dev_data.spi, &dev_data.spi_cfg,
				     &tx, &rx);
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
	gpio_pin_set(dev_data.antenna_enable, GPIO_ANTENNA_ENABLE_PIN, 1);
#else
	LOG_DBG("No antenna switch configured");
#endif
}

void SX126xAntSwOff(void)
{
#if HAVE_GPIO_ANTENNA_ENABLE
	LOG_DBG("Disabling antenna switch");
	gpio_pin_set(dev_data.antenna_enable, GPIO_ANTENNA_ENABLE_PIN, 0);
#else
	LOG_DBG("No antenna switch configured");
#endif
}

static void sx126x_set_tx_enable(int value)
{
#if HAVE_GPIO_TX_ENABLE
	gpio_pin_set(dev_data.tx_enable, GPIO_TX_ENABLE_PIN, value);
#endif
}

static void sx126x_set_rx_enable(int value)
{
#if HAVE_GPIO_RX_ENABLE
	gpio_pin_set(dev_data.rx_enable, GPIO_RX_ENABLE_PIN, value);
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
		gpio_pin_interrupt_configure(dev_data.dio1, GPIO_DIO1_PIN,
					     GPIO_INT_DISABLE);
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
	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 0);
	k_sleep(K_MSEC(10));
	/* Device transitions to standby on reset */
	dev_data.mode = MODE_STDBY_RC;
}

void SX126xSetRfTxPower(int8_t power)
{
	LOG_DBG("power: %" PRIi8, power);
	SX126xSetTxParams(power, RADIO_RAMP_40_US);
}

void SX126xWaitOnBusy(void)
{
	while (gpio_pin_get(dev_data.busy, GPIO_BUSY_PIN)) {
		k_sleep(K_MSEC(1));
	}
}

void SX126xWakeup(void)
{
	int ret;

	/* Reenable DIO1 when waking up */
	gpio_pin_interrupt_configure(dev_data.dio1, GPIO_DIO1_PIN,
				     GPIO_INT_EDGE_TO_ACTIVE);

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
	ret = spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);
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
}

static void sx126x_dio1_irq_callback(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(GPIO_DIO1_PIN)) {
		k_work_submit(&dev_data.dio1_irq_work);
	}
}

static int sx126x_lora_init(const struct device *dev)
{
	int ret;

	LOG_DBG("Initializing %s", DT_INST_LABEL(0));

	if (sx12xx_configure_pin(reset, GPIO_OUTPUT_ACTIVE) ||
	    sx12xx_configure_pin(busy, GPIO_INPUT) ||
	    sx12xx_configure_pin(dio1, GPIO_INPUT | GPIO_INT_DEBOUNCE) ||
	    sx12xx_configure_pin(antenna_enable, GPIO_OUTPUT_INACTIVE) ||
	    sx12xx_configure_pin(rx_enable, GPIO_OUTPUT_INACTIVE) ||
	    sx12xx_configure_pin(tx_enable, GPIO_OUTPUT_INACTIVE)) {
		return -EIO;
	}

	k_work_init(&dev_data.dio1_irq_work, sx126x_dio1_irq_work_handler);

	gpio_init_callback(&dev_data.dio1_irq_callback,
			   sx126x_dio1_irq_callback, BIT(GPIO_DIO1_PIN));
	if (gpio_add_callback(dev_data.dio1, &dev_data.dio1_irq_callback) < 0) {
		LOG_ERR("Could not set GPIO callback for DIO1 interrupt.");
		return -EIO;
	}

	dev_data.spi = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!dev_data.spi) {
		LOG_ERR("Cannot get pointer to %s device",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

#if HAVE_GPIO_CS
	dev_data.spi_cs.gpio_dev = device_get_binding(GPIO_CS_LABEL);
	if (!dev_data.spi_cs.gpio_dev) {
		LOG_ERR("Cannot get pointer to %s device", GPIO_CS_LABEL);
		return -EIO;
	}

	dev_data.spi_cs.gpio_pin = GPIO_CS_PIN;
	dev_data.spi_cs.gpio_dt_flags = GPIO_CS_FLAGS;
	dev_data.spi_cs.delay = 0U;

	dev_data.spi_cfg.cs = &dev_data.spi_cs;
#endif
	dev_data.spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	dev_data.spi_cfg.frequency = DT_INST_PROP(0, spi_max_frequency);
	dev_data.spi_cfg.slave = DT_INST_REG_ADDR(0);


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
	.recv = sx12xx_lora_recv,
	.test_cw = sx12xx_lora_test_cw,
};

DEVICE_DT_INST_DEFINE(0, &sx126x_lora_init, device_pm_control_nop, NULL,
		    NULL, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		    &sx126x_lora_api);
