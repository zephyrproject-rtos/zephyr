/**
 * Copyright (c) 2024 Syslinbit SCOP SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for the MCP3561R/2R/4R ADCs.
 */

#define DT_DRV_COMPAT microchip_mcp356xr

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/mcp356xr.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_ADC_MCP356XR_USE_READ_CRC
#include <zephyr/sys/crc.h>
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

LOG_MODULE_REGISTER(adc_mcp356xr, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define MCP356XR_CRC_POLY (0x8005)
#define MCP356XR_CRC_INIT (0)

#define MCP356XR_COMMAND_FIELD_DEVICE_ADDRESS_MASK   GENMASK(7, 6)
#define MCP356XR_COMMAND_FIELD_REGISTER_ADDRESS_MASK GENMASK(5, 2)
#define MCP356XR_COMMAND_FIELD_FAST_COMMAND_MASK     GENMASK(5, 2)
#define MCP356XR_COMMAND_FIELD_COMMAND_TYPE_MASK     GENMASK(1, 0)

enum adc_mcp356xr_command_type {
	MCP356XR_COMMAND_TYPE_FAST = 0x0,
	MCP356XR_COMMAND_TYPE_SIMPLE_READ = 0x1,
	MCP356XR_COMMAND_TYPE_INCREMENTAL_WRITE = 0x2,
	MCP356XR_COMMAND_TYPE_INCREMENTAL_READ = 0x3,
};

enum adc_mcp356xr_fast_command {
	MCP356XR_FAST_COMMAND_START_CONVERSION = 0xA,
	MCP356XR_FAST_COMMAND_ENTER_STANDBY = 0xB,
	MCP356XR_FAST_COMMAND_SHUTDOWN = 0xC,
	MCP356XR_FAST_COMMAND_FULL_SHUTDOWN = 0xD,
	MCP356XR_FAST_COMMAND_RESET = 0xE,
};

enum adc_mcp356xr_register_address {
	MCP356XR_REGISTER_ADDRESS_ADCDATA = 0x0,
	MCP356XR_REGISTER_ADDRESS_CONFIG0 = 0x1,
	MCP356XR_REGISTER_ADDRESS_CONFIG1 = 0x2,
	MCP356XR_REGISTER_ADDRESS_CONFIG2 = 0x3,
	MCP356XR_REGISTER_ADDRESS_CONFIG3 = 0x4,
	MCP356XR_REGISTER_ADDRESS_IRQ = 0x5,
	MCP356XR_REGISTER_ADDRESS_MUX = 0x6,
	MCP356XR_REGISTER_ADDRESS_SCAN = 0x7,
	MCP356XR_REGISTER_ADDRESS_TIMER = 0x8,
	MCP356XR_REGISTER_ADDRESS_OFFSETCAL = 0x9,
	MCP356XR_REGISTER_ADDRESS_GAINCAL = 0xA,
	MCP356XR_REGISTER_ADDRESS_LOCK = 0xD,
	MCP356XR_REGISTER_ADDRESS_CRCCFG = 0xF,
};

struct adc_mcp356xr_command {
	enum adc_mcp356xr_command_type type;
	union {
		enum adc_mcp356xr_fast_command fast_command;
		enum adc_mcp356xr_register_address register_address;
	};
};

#define MCP356XR_STATUS_FIELD_ADDRESS_ACK_MASK GENMASK(5, 3)
#define MCP356XR_STATUS_FIELD_FLAGS_MASK       GENMASK(2, 0)

#define MCP356XR_STATUS_FIELD_DR_STATUS_MASK     GENMASK(2, 2)
#define MCP356XR_STATUS_FIELD_CRCCFG_STATUS_MASK GENMASK(1, 1)
#define MCP356XR_STATUS_FIELD_POR_STATUS_MASK    GENMASK(0, 0)

#define MCP356XR_STATUS_ADDRESS_ACK_FIELD_VALUE(address) ((address << 1) | (~address & 0x1))

#define MCP356XR_REGISTER_FIELD_CONFIG0_VREF_SEL_MASK       GENMASK(7, 7)
#define MCP356XR_REGISTER_FIELD_CONFIG0_CONFIG0_MASK        GENMASK(6, 6)
#define MCP356XR_REGISTER_FIELD_CONFIG0_CLK_SEL_MASK        GENMASK(5, 4)
#define MCP356XR_REGISTER_FIELD_CONFIG0_CS_SEL_MASK         GENMASK(3, 2)
#define MCP356XR_REGISTER_FIELD_CONFIG0_ADC_MODE_MASK       GENMASK(1, 0)
#define MCP356XR_REGISTER_FIELD_CONFIG1_PRE_MASK            GENMASK(7, 6)
#define MCP356XR_REGISTER_FIELD_CONFIG1_OSR_MASK            GENMASK(5, 2)
#define MCP356XR_REGISTER_FIELD_CONFIG2_BOOST_MASK          GENMASK(7, 6)
#define MCP356XR_REGISTER_FIELD_CONFIG2_GAIN_MASK           GENMASK(5, 3)
#define MCP356XR_REGISTER_FIELD_CONFIG2_AZ_MUX_MASK         GENMASK(2, 2)
#define MCP356XR_REGISTER_FIELD_CONFIG2_AZ_REF_MASK         GENMASK(1, 1)
#define MCP356XR_REGISTER_FIELD_CONFIG3_CONV_MODE_MASK      GENMASK(7, 6)
#define MCP356XR_REGISTER_FIELD_CONFIG3_DATA_FORMAT_MASK    GENMASK(5, 4)
#define MCP356XR_REGISTER_FIELD_CONFIG3_CRC_FORMAT_MASK     GENMASK(3, 3)
#define MCP356XR_REGISTER_FIELD_CONFIG3_EN_CRCCOM_MASK      GENMASK(2, 2)
#define MCP356XR_REGISTER_FIELD_CONFIG3_EN_OFFCAL_MASK      GENMASK(1, 1)
#define MCP356XR_REGISTER_FIELD_CONFIG3_EN_GAINCAL_MASK     GENMASK(0, 0)
#define MCP356XR_REGISTER_FIELD_IRQ_DR_STATUS_MASK          GENMASK(6, 6)
#define MCP356XR_REGISTER_FIELD_IRQ_CRCCFG_STATUS_MASK      GENMASK(5, 5)
#define MCP356XR_REGISTER_FIELD_IRQ_POR_STATUS_MASK         GENMASK(4, 4)
#define MCP356XR_REGISTER_FIELD_IRQ_IRQ_MODE_MASK           GENMASK(3, 2)
#define MCP356XR_REGISTER_FIELD_IRQ_EN_FASTCMD_MASK         GENMASK(1, 1)
#define MCP356XR_REGISTER_FIELD_IRQ_EN_STP_MASK             GENMASK(0, 0)
#define MCP356XR_REGISTER_FIELD_MUX_MUX_VIN_POSITIVE_MASK   GENMASK(7, 4)
#define MCP356XR_REGISTER_FIELD_MUX_MUX_VIN_NEGATIVE_MASK   GENMASK(3, 0)
#define MCP356XR_REGISTER_FIELD_SCAN_DLY_MASK               GENMASK(23, 21)
#define MCP356XR_REGISTER_FIELD_SCAN_CHANNEL_SELECTION_MASK GENMASK(15, 0)
#define MCP356XR_REGISTER_FIELD_TIMER_TIMER_MASK            GENMASK(23, 0)
#define MCP356XR_REGISTER_FIELD_OFFSETCAL_OFFSETCAL_MASK    GENMASK(23, 0)
#define MCP356XR_REGISTER_FIELD_GAINCAL_GAINCAL_MASK        GENMASK(23, 0)
#define MCP356XR_REGISTER_FIELD_LOCK_LOCK_MASK              GENMASK(7, 0)
#define MCP356XR_REGISTER_FIELD_CRCCFG_CRCCFG_MASK          GENMASK(15, 0)

enum {
	MCP356XR_REGISTER_CONFIG0_VREF_SEL_EXTERNAL_VOLTAGE_REF = 0,
	MCP356XR_REGISTER_CONFIG0_VREF_SEL_INTERNAL_VOLTAGE_REF = 1,
};

enum {
	MCP356XR_REGISTER_CONFIG0_CONFIG0_DO_NOT_ENTER_PARTIAL_SHUTDOWN = 0,
	MCP356XR_REGISTER_CONFIG0_CONFIG0_ENTER_PARTIAL_SHUTDOWN = 1,
};

enum {
	MCP356XR_REGISTER_CONFIG0_CLK_SEL_EXTERNAL_CLOCK = 0,
	MCP356XR_REGISTER_CONFIG0_CLK_SEL_EXTERNAL_CLOCK_ = 1, /* also selects the external clock */
	MCP356XR_REGISTER_CONFIG0_CLK_SEL_INTERNAL_CLOCK_NO_OUTPUT = 2,
	MCP356XR_REGISTER_CONFIG0_CLK_SEL_INTERNAL_CLOCK_OUTPUT_AMCLK = 3,
};

enum {
	MCP356XR_REGISTER_CONFIG0_CS_SEL_NO_CURRENT = 0,
	MCP356XR_REGISTER_CONFIG0_CS_SEL_0uA9 = 1,
	MCP356XR_REGISTER_CONFIG0_CS_SEL_3uA7 = 2,
	MCP356XR_REGISTER_CONFIG0_CS_SEL_15uA = 3,
};

enum {
	MCP356XR_REGISTER_CONFIG0_ADC_MODE_SHUTDOWN = 0,
	MCP356XR_REGISTER_CONFIG0_ADC_MODE_SHUTDOWN_ = 1, /* also causes ADC to shutdown */
	MCP356XR_REGISTER_CONFIG0_ADC_MODE_STANDBY = 2,
	MCP356XR_REGISTER_CONFIG0_ADC_MODE_CONVERSION = 3,
};

enum {
	MCP356XR_REGISTER_CONFIG1_OSR_32 = 0,
	MCP356XR_REGISTER_CONFIG1_OSR_64 = 1,
	MCP356XR_REGISTER_CONFIG1_OSR_128 = 2,
	MCP356XR_REGISTER_CONFIG1_OSR_256 = 3,
	MCP356XR_REGISTER_CONFIG1_OSR_512 = 4,
	MCP356XR_REGISTER_CONFIG1_OSR_1024 = 5,
	MCP356XR_REGISTER_CONFIG1_OSR_2048 = 6,
	MCP356XR_REGISTER_CONFIG1_OSR_4096 = 7,
	MCP356XR_REGISTER_CONFIG1_OSR_8192 = 8,
	MCP356XR_REGISTER_CONFIG1_OSR_16384 = 9,
	MCP356XR_REGISTER_CONFIG1_OSR_20480 = 10,
	MCP356XR_REGISTER_CONFIG1_OSR_24576 = 11,
	MCP356XR_REGISTER_CONFIG1_OSR_40960 = 12,
	MCP356XR_REGISTER_CONFIG1_OSR_49152 = 13,
	MCP356XR_REGISTER_CONFIG1_OSR_81920 = 14,
	MCP356XR_REGISTER_CONFIG1_OSR_98304 = 15,
};

#define MCP356XR_REGISTER_CONFIG1_OSR(oversampling)                                                \
	((oversampling <= 14) ? (oversampling - 5) : MCP356XR_REGISTER_CONFIG1_OSR_16384)

enum {
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_16_DIGITAL_MUL_4 = 7,
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_16_DIGITAL_MUL_2 = 6,
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_16 = 5,
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_8 = 4,
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_4 = 3,
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_2 = 2,
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_1 = 1,
	MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_DIV_3 = 0,
};

enum {
	MCP356XR_REGISTER_CONFIG3_CONV_MODE_ONE_SHOT_GO_SHUTDOWN = 0,
	MCP356XR_REGISTER_CONFIG3_CONV_MODE_ONE_SHOT_GO_SHUTDOWN_BIS = 1,
	MCP356XR_REGISTER_CONFIG3_CONV_MODE_ONE_SHOT_GO_STANDBY = 2,
	MCP356XR_REGISTER_CONFIG3_CONV_MODE_CONTINUOUS = 3,
};

enum {
	MCP356XR_REGISTER_CONFIG3_DATA_FORMAT_1_BIT_SIGN_23_BITS_DATA = 0,
	MCP356XR_REGISTER_CONFIG3_DATA_FORMAT_1_BIT_SIGN_23_BITS_DATA_8_BITS_PADDING = 1,
	MCP356XR_REGISTER_CONFIG3_DATA_FORMAT_8_BITS_SIGN_24_BITS_DATA = 2,
	MCP356XR_REGISTER_CONFIG3_DATA_FORMAT_4_BITS_CHAN_ID_4_BITS_SIGN_24_BITS_DATA = 3,
};

enum {
	MCP356XR_REGISTER_CONFIG3_CRC_FORMAT_16_BITS_CRC = 0,
	MCP356XR_REGISTER_CONFIG3_CRC_FORMAT_16_BITS_CRC_16_BITS_PADDING = 1,
};

enum {
	MCP356XR_REGISTER_IRQ_IRQ_MODE_IRQ_OUTPUT_OPEN_DRAIN = 0,
	MCP356XR_REGISTER_IRQ_IRQ_MODE_IRQ_OUTPUT_PUSH_PULL = 1,
	MCP356XR_REGISTER_IRQ_IRQ_MODE_MDAT_OUTPUT = 2,
	MCP356XR_REGISTER_IRQ_IRQ_MODE_MDAT_OUTPUT_BIS = 3,
};

enum {
	MCP356XR_REGISTER_CONFIG_FLAG_ENABLED = 1,
	MCP356XR_REGISTER_CONFIG_FLAG_DISABLED = 0,
};

enum {
	MCP356XR_REGISTER_STATUS_FLAG_ENABLED = 0,
	MCP356XR_REGISTER_STATUS_FLAG_DISABLED = 1,
};

#define MCP356XR_REGISTER_LOCK_UNLOCK_REGISTERS_CODE 0xA5

#define MCP356XR_FIELD_PREP(name, value) FIELD_PREP(MCP356XR_##name##_MASK, value)
#define MCP356XR_FIELD_GET(name, value)  FIELD_GET(MCP356XR_##name##_MASK, value)

#define MCP356XR_COMMAND_FIELD_PREP(field, value) MCP356XR_FIELD_PREP(COMMAND_FIELD_##field, value)

#define MCP356XR_STATUS_FIELD_GET(field, value) MCP356XR_FIELD_GET(STATUS_FIELD_##field, value)

#define MCP356XR_REGISTER_FIELD_PREP(register, field, value)                                       \
	MCP356XR_FIELD_PREP(REGISTER_FIELD_##register##_##field, value)
#define MCP356XR_REGISTER_FIELD_GET(register, field, value)                                        \
	MCP356XR_FIELD_GET(REGISTER_FIELD_##register##_##field, value)
#define MCP356XR_REGISTER_FIELD_SET(register, field, holder, value)                                \
	(holder) = ((holder) & ~(MCP356XR_REGISTER_FIELD_##register##_##field##_MASK)) |           \
		   MCP356XR_REGISTER_FIELD_PREP(register, field, value)

#define REGISTER_ADDRESS(register_name) MCP356XR_REGISTER_ADDRESS_##register_name

#define REGISTER_INDEX_(register_address)                                                          \
	((register_address >= MCP356XR_REGISTER_ADDRESS_CONFIG0)                                   \
		 ? ((register_address <= MCP356XR_REGISTER_ADDRESS_MUX)                            \
			    ? (register_address - MCP356XR_REGISTER_ADDRESS_CONFIG0)               \
			    : MCP356XR_REGISTER_ADDRESS_MUX)                                       \
		 : MCP356XR_REGISTER_ADDRESS_MUX)
#define REGISTER_INDEX(register_name) REGISTER_INDEX_(REGISTER_ADDRESS(register_name))

#define REGISTER_ARRAY_SIZE (REGISTER_INDEX(MUX) + 1)

struct adc_mcp356xr_config {
	struct spi_dt_spec spi;
#ifndef CONFIG_ADC_MCP356XR_POLL
	struct gpio_dt_spec irq;
#endif
	uint8_t address;
	k_timeout_t vref_settle_time;
	uint8_t init_reg_value[REGISTER_ARRAY_SIZE];
};

struct channel_registers {
	uint8_t config0;
	uint8_t config2;
	uint8_t mux;
};

struct adc_mcp356xr_data {
	const struct device *dev;
	struct adc_context ctx;
	int32_t *buffer;
	int32_t *repeat_buffer;
	uint32_t channels;

	bool wait_for_vref_stabilization;
	bool internal_vref_in_use;

	uint8_t config1_register;
	struct k_mutex channel_registers_mutex;
	struct channel_registers channel_registers[CONFIG_ADC_MCP356XR_ADC_CHANNEL_COUNT];

	struct k_thread thread;
	struct k_sem start_sequence;

#ifndef CONFIG_ADC_MCP356XR_POLL
	struct gpio_callback irq_callback_data;
	struct k_sem irq_occurred;
#endif

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_MCP356XR_THREAD_STACK_SIZE);
};

#ifndef CONFIG_ADC_MCP356XR_POLL
static void adc_mcp356xr_irq(const struct device *gpio_port, struct gpio_callback *callback_data,
			     gpio_port_pins_t gpio_pins)
{
	ARG_UNUSED(gpio_port);
	ARG_UNUSED(gpio_pins);

	struct adc_mcp356xr_data *data =
		CONTAINER_OF(callback_data, struct adc_mcp356xr_data, irq_callback_data);

	k_sem_give(&(data->irq_occurred));
}

static int adc_mcp356xr_irq_init(const struct device *dev)
{
	const struct adc_mcp356xr_config *config = dev->config;
	struct adc_mcp356xr_data *data = dev->data;

	int err = 0;

	k_sem_init(&(data->irq_occurred), 0, 1);

	if (!gpio_is_ready_dt(&(config->irq))) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	gpio_init_callback(&(data->irq_callback_data), adc_mcp356xr_irq, BIT(config->irq.pin));
	err = gpio_add_callback_dt(&(config->irq), &(data->irq_callback_data));
	if (err) {
		LOG_ERR("Failed to add irq callback (err %d)", err);
		return err;
	}

	err = gpio_pin_configure_dt(&(config->irq), GPIO_INPUT);
	if (err) {
		LOG_ERR("Cannot configure IRQ GPIO (err %d)", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&(config->irq), GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("Failed to enable interrupt on IRQ pin (err %d)", err);
		return err;
	}

	return 0;
}
#endif

static int adc_mcp356xr_transceive(const struct device *dev, uint8_t command_byte,
				   uint8_t *status_byte, uint8_t *tx_data, size_t tx_length,
				   uint8_t *rx_data, size_t rx_length, uint16_t *crc)
{
	const struct adc_mcp356xr_config *config = dev->config;

	const struct spi_buf tx_buf[2] = {
		{
			.buf = &command_byte,
			.len = 1,
		},
		{
			.buf = tx_data,
			.len = tx_length,
		},
	};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

#ifdef CONFIG_ADC_MCP356XR_USE_READ_CRC
	uint8_t crc_buffer[2] = {0};

	const struct spi_buf rx_buf[3] = {
		{
			.buf = status_byte,
			.len = 1,
		},
		{
			.buf = rx_data,
			.len = rx_length,
		},
		{
			.buf = crc_buffer,
			.len = 2,
		},
	};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ((crc != NULL) ? ARRAY_SIZE(rx_buf) : (ARRAY_SIZE(rx_buf) - 1)),
	};
#else
	const struct spi_buf rx_buf[2] = {
		{
			.buf = status_byte,
			.len = 1,
		},
		{
			.buf = rx_data,
			.len = rx_length,
		},
	};

	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};
#endif

	int err = 0;

	err = spi_transceive_dt(&(config->spi), &tx, &rx);
	if (err) {
		LOG_ERR("SPI error occurred while communicating with ADC (error = %d)", err);
		return -EIO;
	}

#ifdef CONFIG_ADC_MCP356XR_USE_READ_CRC
	if (crc != NULL) {
		*crc = sys_get_be16(crc_buffer);
	}
#else
	(void)crc;
#endif

	return 0;
}

static int adc_mcp356xr_send_command(const struct device *dev, struct adc_mcp356xr_command command,
				     uint8_t *status_flags, uint8_t *data, size_t length)
{
	const struct adc_mcp356xr_config *config = dev->config;

	int err = 0;
	uint8_t status_byte = 0;
	uint16_t crc = 0;

	uint8_t command_byte = MCP356XR_COMMAND_FIELD_PREP(DEVICE_ADDRESS, config->address) |
			       MCP356XR_COMMAND_FIELD_PREP(COMMAND_TYPE, command.type);

	if (command.type == MCP356XR_COMMAND_TYPE_FAST) {
		command_byte |= MCP356XR_COMMAND_FIELD_PREP(FAST_COMMAND, command.fast_command);
	} else {
		command_byte |=
			MCP356XR_COMMAND_FIELD_PREP(REGISTER_ADDRESS, command.register_address);
	}

	switch (command.type) {
	case MCP356XR_COMMAND_TYPE_FAST:
		err = adc_mcp356xr_transceive(dev, command_byte, &status_byte, NULL, 0, NULL, 0,
					      NULL);
		break;
	case MCP356XR_COMMAND_TYPE_SIMPLE_READ:
		err = adc_mcp356xr_transceive(dev, command_byte, &status_byte, NULL, 0, data,
					      length, &crc);
		break;
	case MCP356XR_COMMAND_TYPE_INCREMENTAL_WRITE:
		err = adc_mcp356xr_transceive(dev, command_byte, &status_byte, data, length, NULL,
					      0, NULL);
		break;
	case MCP356XR_COMMAND_TYPE_INCREMENTAL_READ:
		err = adc_mcp356xr_transceive(dev, command_byte, &status_byte, NULL, 0, data,
					      length, &crc);
		break;
	default:
		return -EINVAL;
	}

	if (err) {
		return err;
	}

	if (status_flags != NULL) {
		*status_flags = MCP356XR_STATUS_FIELD_GET(FLAGS, status_byte);
	}

#ifdef CONFIG_ADC_MCP356XR_USE_READ_CRC
	if ((command.type == MCP356XR_COMMAND_TYPE_SIMPLE_READ) ||
	    (command.type == MCP356XR_COMMAND_TYPE_INCREMENTAL_READ)) {
		uint16_t computed_crc = 0;

		computed_crc = crc16(MCP356XR_CRC_POLY, MCP356XR_CRC_INIT, &status_byte, 1);
		computed_crc = crc16(MCP356XR_CRC_POLY, computed_crc, data, length);

		if (crc != computed_crc) {
			return -EILSEQ;
		}
	}
#endif

	if (MCP356XR_STATUS_FIELD_GET(ADDRESS_ACK, status_byte) !=
	    MCP356XR_STATUS_ADDRESS_ACK_FIELD_VALUE(config->address)) {
		return -ENOMSG;
	}

	return 0;
}

static int adc_mcp356xr_send_fast_command(const struct device *dev,
					  enum adc_mcp356xr_fast_command fast_command)
{
	struct adc_mcp356xr_command command = {
		.type = MCP356XR_COMMAND_TYPE_FAST,
		.fast_command = fast_command,
	};

	return adc_mcp356xr_send_command(dev, command, NULL, NULL, 0);
}

static int adc_mcp356xr_incremental_write(const struct device *dev,
					  enum adc_mcp356xr_register_address start_register_address,
					  uint8_t *data, size_t length)
{
	struct adc_mcp356xr_command command = {
		.type = MCP356XR_COMMAND_TYPE_INCREMENTAL_WRITE,
		.register_address = start_register_address,
	};

	return adc_mcp356xr_send_command(dev, command, NULL, data, length);
}

static int adc_mcp356xr_simple_read_with_status(const struct device *dev,
						enum adc_mcp356xr_register_address register_address,
						uint8_t *data, size_t length, uint8_t *status)
{
	struct adc_mcp356xr_command command = {
		.type = MCP356XR_COMMAND_TYPE_SIMPLE_READ,
		.register_address = register_address,
	};

	return adc_mcp356xr_send_command(dev, command, status, data, length);
}

static inline int adc_mcp356xr_simple_read(const struct device *dev,
					   enum adc_mcp356xr_register_address register_address,
					   uint8_t *data, size_t length)
{
	return adc_mcp356xr_simple_read_with_status(dev, register_address, data, length, NULL);
}

static int adc_mcp356xr_unlock_registers(const struct device *dev)
{
	uint8_t unlock_value = MCP356XR_REGISTER_LOCK_UNLOCK_REGISTERS_CODE;

	return adc_mcp356xr_incremental_write(dev, MCP356XR_REGISTER_ADDRESS_LOCK, &unlock_value,
					      1);
}

static inline int adc_mcp356xr_reset(const struct device *dev)
{
	return adc_mcp356xr_send_fast_command(dev, MCP356XR_FAST_COMMAND_RESET);
}

static inline int adc_mcp356xr_start_conversion(const struct device *dev)
{
	return adc_mcp356xr_send_fast_command(dev, MCP356XR_FAST_COMMAND_START_CONVERSION);
}

static inline int adc_mcp356xr_get_status(const struct device *dev, bool *data_ready,
					  bool *por_occurred, bool *crc_error)
{
	uint8_t irq_register_value = 0xFF;

	int err = adc_mcp356xr_simple_read(dev, MCP356XR_REGISTER_ADDRESS_IRQ, &irq_register_value,
					   1);

	if (data_ready != NULL) {
		*data_ready = (MCP356XR_REGISTER_FIELD_GET(IRQ, DR_STATUS, irq_register_value) ==
			       MCP356XR_REGISTER_STATUS_FLAG_ENABLED);
	}

	if (crc_error != NULL) {
		*crc_error = (MCP356XR_REGISTER_FIELD_GET(IRQ, CRCCFG_STATUS, irq_register_value) ==
			      MCP356XR_REGISTER_STATUS_FLAG_ENABLED);
	}

	if (por_occurred != NULL) {
		*por_occurred = (MCP356XR_REGISTER_FIELD_GET(IRQ, POR_STATUS, irq_register_value) ==
				 MCP356XR_REGISTER_STATUS_FLAG_ENABLED);
	}

	return err;
}

static inline int adc_mcp356xr_get_data(const struct device *dev, int32_t *data)
{
	int err = 0;
	uint8_t status = 0;
	uint8_t buffer[4] = {0};

	uint32_t raw_data = 0;

	__ASSERT_NO_MSG(data != NULL);

	err = adc_mcp356xr_simple_read_with_status(dev, MCP356XR_REGISTER_ADDRESS_ADCDATA, buffer,
						   4, &status);
	if (err) {
		return err;
	}

	if (MCP356XR_STATUS_FIELD_GET(POR_STATUS, status) ==
	    MCP356XR_REGISTER_STATUS_FLAG_ENABLED) {
		/* it seems that a POR occurred so data is probably corrupted */
		return -ENXIO;
	}

	raw_data = sys_get_be32(buffer);

	*data = *((int32_t *)&raw_data);

	return 0;
}

static inline void adc_mcp356xr_channel_registers_init(const struct device *dev,
						       struct channel_registers *registers)
{
	const struct adc_mcp356xr_config *config = dev->config;

	__ASSERT_NO_MSG(registers != NULL);

	registers->config0 = config->init_reg_value[REGISTER_INDEX(CONFIG0)];
	registers->config2 = config->init_reg_value[REGISTER_INDEX(CONFIG2)];
	registers->mux = config->init_reg_value[REGISTER_INDEX(MUX)];
}

static inline void adc_mcp356xr_channel_registers_set(const struct device *dev, uint8_t channel_id,
						      struct channel_registers registers)
{
	struct adc_mcp356xr_data *data = dev->data;

	__ASSERT_NO_MSG(channel_id < CONFIG_ADC_MCP356XR_ADC_CHANNEL_COUNT);

	k_mutex_lock(&(data->channel_registers_mutex), K_FOREVER);

	data->channel_registers[channel_id].config0 = registers.config0;
	data->channel_registers[channel_id].config2 = registers.config2;
	data->channel_registers[channel_id].mux = registers.mux;

	k_mutex_unlock(&(data->channel_registers_mutex));
}

static inline struct channel_registers adc_mcp356xr_channel_registers_get(const struct device *dev,
									  uint8_t channel_id)
{
	struct adc_mcp356xr_data *data = dev->data;
	struct channel_registers registers;

	__ASSERT_NO_MSG(channel_id < CONFIG_ADC_MCP356XR_ADC_CHANNEL_COUNT);

	k_mutex_lock(&(data->channel_registers_mutex), K_FOREVER);

	registers.config0 = data->channel_registers[channel_id].config0;
	registers.config2 = data->channel_registers[channel_id].config2;
	registers.mux = data->channel_registers[channel_id].mux;

	k_mutex_unlock(&(data->channel_registers_mutex));

	return registers;
}

static inline int adc_mcp356xr_set_gain(struct channel_registers *registers, enum adc_gain gain)
{
	__ASSERT_NO_MSG(registers != NULL);

	switch (gain) {
	case ADC_GAIN_1_3:
		MCP356XR_REGISTER_FIELD_SET(CONFIG2, GAIN, registers->config2,
					    MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_DIV_3);
		break;
	case ADC_GAIN_1:
		MCP356XR_REGISTER_FIELD_SET(CONFIG2, GAIN, registers->config2,
					    MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_1);
		break;
	case ADC_GAIN_2:
		MCP356XR_REGISTER_FIELD_SET(CONFIG2, GAIN, registers->config2,
					    MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_2);
		break;
	case ADC_GAIN_4:
		MCP356XR_REGISTER_FIELD_SET(CONFIG2, GAIN, registers->config2,
					    MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_4);
		break;
	case ADC_GAIN_8:
		MCP356XR_REGISTER_FIELD_SET(CONFIG2, GAIN, registers->config2,
					    MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_8);
		break;
	case ADC_GAIN_16:
		MCP356XR_REGISTER_FIELD_SET(CONFIG2, GAIN, registers->config2,
					    MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_16);
		break;
	case ADC_GAIN_32:
		MCP356XR_REGISTER_FIELD_SET(
			CONFIG2, GAIN, registers->config2,
			MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_16_DIGITAL_MUL_2);
		break;
	case ADC_GAIN_64:
		MCP356XR_REGISTER_FIELD_SET(
			CONFIG2, GAIN, registers->config2,
			MCP356XR_REGISTER_CONFIG2_GAIN_ANALOG_MUL_16_DIGITAL_MUL_4);
		break;
	default:
		LOG_ERR("Channel gain '%d' is not supported by device", gain);
		return -ENOTSUP;
	}

	return 0;
}

static inline int adc_mcp356xr_set_inputs(struct channel_registers *registers,
					  uint8_t positive_input, uint8_t negative_input)
{
	__ASSERT_NO_MSG(registers != NULL);

	if (positive_input == MCP356XR_INPUT_RESERVED_DO_NOT_USE) {
		LOG_ERR("Invalid channel positive input %d", positive_input);
		return -ENOTSUP;
	}

	if (negative_input == MCP356XR_INPUT_RESERVED_DO_NOT_USE) {
		LOG_ERR("Invalid channel negative input %d", negative_input);
		return -ENOTSUP;
	}

	MCP356XR_REGISTER_FIELD_SET(MUX, MUX_VIN_POSITIVE, registers->mux, positive_input);
	MCP356XR_REGISTER_FIELD_SET(MUX, MUX_VIN_NEGATIVE, registers->mux, negative_input);

	return 0;
}

static inline int adc_mcp356xr_set_reference(struct channel_registers *registers,
					     enum adc_reference reference)
{
	__ASSERT_NO_MSG(registers != NULL);

	if (reference != ADC_REF_EXTERNAL0 && reference != ADC_REF_INTERNAL) {
		LOG_ERR("Channel voltage reference '%d' is not supported by device", reference);
		return -ENOTSUP;
	}

	if (reference == ADC_REF_INTERNAL) {
		MCP356XR_REGISTER_FIELD_SET(
			CONFIG0, VREF_SEL, registers->config0,
			MCP356XR_REGISTER_CONFIG0_VREF_SEL_INTERNAL_VOLTAGE_REF);
	} else {
		MCP356XR_REGISTER_FIELD_SET(
			CONFIG0, VREF_SEL, registers->config0,
			MCP356XR_REGISTER_CONFIG0_VREF_SEL_EXTERNAL_VOLTAGE_REF);
	}

	return 0;
}

static inline int adc_mcp356xr_set_oversampling(const struct device *dev, uint8_t oversampling)
{
	struct adc_mcp356xr_data *data = dev->data;
	const struct adc_mcp356xr_config *config = dev->config;

	if (oversampling < 5) {
		LOG_ERR("Oversamplings below 2^5 is not supported by device");
		return -ENOTSUP;
	}

	if (oversampling > 14) {
		LOG_ERR("Oversampling of 2^%d is not supported by device", oversampling);
		return -ENOTSUP;
	}

	if (oversampling < 8) {
		LOG_WRN("Oversamplings below 2^8 does not allow full 24bits resolution. See "
			"datasheet DS20006391C, table 5-6, page 46 for more details.");
	}

	data->config1_register = config->init_reg_value[REGISTER_INDEX(CONFIG1)];
	MCP356XR_REGISTER_FIELD_SET(CONFIG1, OSR, data->config1_register,
				    MCP356XR_REGISTER_CONFIG1_OSR(oversampling));

	return 0;
}

static int adc_mcp356xr_reset_after_power_on(const struct device *dev)
{
	struct adc_mcp356xr_data *data = dev->data;

	int err = 0;

	err = adc_mcp356xr_unlock_registers(dev);
	if (err) {
		LOG_ERR("Failed to unlock MCP356x registers (error = %d)", err);
		return err;
	}

	/* enable fast commands so reset command is taken into account */
	uint8_t irq_register_value = MCP356XR_REGISTER_FIELD_PREP(
		IRQ, EN_FASTCMD, MCP356XR_REGISTER_CONFIG_FLAG_ENABLED);

	err = adc_mcp356xr_incremental_write(dev, MCP356XR_REGISTER_ADDRESS_IRQ,
					     &irq_register_value, 1);
	if (err) {
		LOG_ERR("Failed to enable fast commands (error = %d)", err);
		return err;
	}

	err = adc_mcp356xr_reset(dev);
	if (err) {
		LOG_ERR("Failed to reset MCP356x (error = %d)", err);
		return err;
	}

	/* by default after a reset the internal voltage reference is selected */
	data->internal_vref_in_use = true;
	/* force a wait to ensure voltage is stable before performing the next acquisition */
	data->wait_for_vref_stabilization = true;

#ifdef CONFIG_ADC_MCP356XR_USE_READ_CRC
	const struct adc_mcp356xr_config *config = dev->config;

	/* directly write CONFIG3 register so that CRC on subsequent reads will be enabled */
	uint8_t config3_register_value = config->init_reg_value[REGISTER_INDEX(CONFIG3)];

	err = adc_mcp356xr_incremental_write(dev, MCP356XR_REGISTER_ADDRESS_CONFIG3,
					     &config3_register_value, 1);
	if (err) {
		LOG_ERR("Failed to enable CRC on read communications (error = %d)", err);
		return err;
	}
#endif

	return 0;
}

static inline int adc_mcp356xr_get_status_and_sanitize(const struct device *dev, bool *data_ready,
						       bool *por_occurred, bool *crc_error)
{
	int err = 0;
	bool local_por_occurred = false;

	err = adc_mcp356xr_get_status(dev, data_ready, &local_por_occurred, crc_error);
	if (local_por_occurred && (err == 0 || err == -EILSEQ)) {
		/* Try do reset the ADC if a POR occurred. Note that in such case the
		 * EN_CRCCOM flag in the CONFIG3 register may have been reset causing the
		 * CRC not being computed by the ADC thus triggering the EILSEQ error.
		 */
		LOG_WRN("Power cycle reset occurred, attempting to reset the ADC");
		err = adc_mcp356xr_reset_after_power_on(dev);
		if (err) {
			LOG_ERR("Failed to properly reset ADC after a power cycle occurred "
				"(error = %d)",
				err);
			return err;
		}
	} else if (err) {
		return err;
	}

	if (por_occurred != NULL) {
		*por_occurred = local_por_occurred;
	}

	return 0;
}

static int adc_mcp356xr_send_configuration(const struct device *dev, uint8_t channel_id)
{
	const struct adc_mcp356xr_config *config = dev->config;
	struct adc_mcp356xr_data *data = dev->data;

	struct channel_registers channel_registers =
		adc_mcp356xr_channel_registers_get(dev, channel_id);

	uint8_t config_buffer[REGISTER_ARRAY_SIZE] = {
		[REGISTER_INDEX(CONFIG0)] = channel_registers.config0,
		[REGISTER_INDEX(CONFIG2)] = channel_registers.config2,
		[REGISTER_INDEX(MUX)] = channel_registers.mux,
		[REGISTER_INDEX(CONFIG1)] = data->config1_register,
		[REGISTER_INDEX(CONFIG3)] = config->init_reg_value[REGISTER_INDEX(CONFIG3)],
		[REGISTER_INDEX(IRQ)] = config->init_reg_value[REGISTER_INDEX(IRQ)],
	};

	bool internal_vref_selected =
		(MCP356XR_REGISTER_FIELD_GET(CONFIG0, VREF_SEL, channel_registers.config0) ==
		 MCP356XR_REGISTER_CONFIG0_VREF_SEL_INTERNAL_VOLTAGE_REF);

	if (internal_vref_selected != data->internal_vref_in_use) {
		/* we are not using the same Vref anymore so we must wait for things to stabilize */
		data->wait_for_vref_stabilization = true;
	}

	data->internal_vref_in_use = internal_vref_selected;

	int err = adc_mcp356xr_incremental_write(dev, MCP356XR_REGISTER_ADDRESS_CONFIG0,
						 config_buffer, ARRAY_SIZE(config_buffer));
	if (err) {
		LOG_ERR("Failed to write configuration (error = %d)", err);
	}

	return err;
}

static int adc_mcp356xr_validate_buffer_size(const struct adc_sequence *sequence)
{
	uint8_t channels = 0;
	size_t needed;

	channels = POPCOUNT(sequence->channels);

	needed = channels * sizeof(int32_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int adc_mcp356xr_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mcp356xr_data *data = dev->data;

	int err = 0;

	if (sequence->channels == 0) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	if (find_msb_set(sequence->channels) > CONFIG_ADC_MCP356XR_ADC_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel selection (0x%x)", sequence->channels);
		return -EINVAL;
	}

	if (sequence->resolution != 24) {
		LOG_ERR("%d bit resolution is not supported", sequence->resolution);
		return -ENOTSUP;
	}

	err = adc_mcp356xr_validate_buffer_size(sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	err = adc_mcp356xr_set_oversampling(dev, sequence->oversampling);
	if (err) {
		return err;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&(data->ctx), sequence);

	return adc_context_wait_for_completion(&(data->ctx));
}

static int adc_mcp356xr_wait_for_data(const struct device *dev)
{
	while (1) {
		int err = 0;
		bool data_ready = false;
		bool por_occurred = false;

#ifdef CONFIG_ADC_MCP356XR_POLL
		k_sleep(K_MSEC(CONFIG_ADC_MCP356XR_POLLING_PERIOD_MS));
#else
		struct adc_mcp356xr_data *data = dev->data;

		k_sem_take(&(data->irq_occurred), K_FOREVER);
#endif

		err = adc_mcp356xr_get_status_and_sanitize(dev, &data_ready, &por_occurred, NULL);
		if (err) {
			LOG_ERR("An error occurred while attempting to retrieve ADC status "
				"(error = %d)",
				err);
			return err;
		}

		if (por_occurred) {
			LOG_ERR("Power cycle reset occurred: stop waiting for data");
			return -ENXIO;
		}

		if (data_ready) {
			return 0;
		}
	}
}

static int adc_mcp356xr_run_acquisition_sequence(const struct device *dev)
{
	struct adc_mcp356xr_data *data = dev->data;
	const struct adc_mcp356xr_config *config = dev->config;

	while (data->channels != 0) {
		int err = 0;
		int32_t read_value = 0;

		uint8_t channel_id = find_lsb_set(data->channels) - 1;

		err = adc_mcp356xr_send_configuration(dev, channel_id);
		if (err) {
			LOG_ERR("Failed to configure channel %d (error = %d)", channel_id, err);
			return err;
		}

		if (data->wait_for_vref_stabilization) {
			k_sleep(config->vref_settle_time);
			data->wait_for_vref_stabilization = false;
		}

		err = adc_mcp356xr_start_conversion(dev);
		if (err) {
			LOG_ERR("Failed to start conversion (error = %d)", err);
			return err;
		}

		err = adc_mcp356xr_wait_for_data(dev);
		if (err) {
			LOG_ERR("An error occurred while waiting for data (error = %d)", err);
			return err;
		}

		err = adc_mcp356xr_get_data(dev, &read_value);
		if (err) {
			LOG_ERR("Failed to retrieve ADC reading (error = %d)", err);
			return err;
		}

		*data->buffer++ = read_value;
		WRITE_BIT(data->channels, channel_id, 0);
	}

	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_mcp356xr_data *data = CONTAINER_OF(ctx, struct adc_mcp356xr_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mcp356xr_data *data = CONTAINER_OF(ctx, struct adc_mcp356xr_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&(data->start_sequence));
}

static int adc_mcp356xr_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	int err = 0;
	struct channel_registers registers;

	if (channel_cfg->channel_id >= CONFIG_ADC_MCP356XR_ADC_CHANNEL_COUNT) {
		LOG_ERR("Channel id '%d' is not supported", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%d'", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (!channel_cfg->differential) {
		LOG_ERR("Single-ended input is not directly supported. Single-ended configuration "
			"is achieved by setting negative input to MCP356XR_INPUT_AGND or any other "
			"input connected to ground.");
		return -ENOTSUP;
	}

	adc_mcp356xr_channel_registers_init(dev, &registers);

	err = adc_mcp356xr_set_gain(&registers, channel_cfg->gain);
	if (err) {
		return err;
	}

	err = adc_mcp356xr_set_inputs(&registers, channel_cfg->input_positive,
				      channel_cfg->input_negative);
	if (err) {
		return err;
	}

	err = adc_mcp356xr_set_reference(&registers, channel_cfg->reference);
	if (err) {
		return err;
	}

	adc_mcp356xr_channel_registers_set(dev, channel_cfg->channel_id, registers);

	return 0;
}

static int adc_mcp356xr_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mcp356xr_data *data = dev->data;

	int err = 0;

	adc_context_lock(&(data->ctx), false, NULL);
	err = adc_mcp356xr_start_read(dev, sequence);
	adc_context_release(&(data->ctx), err);

	return err;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_mcp356xr_read_async(const struct device *dev, const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct adc_mcp356xr_data *data = dev->data;

	int err = 0;

	adc_context_lock(&(data->ctx), true, async);
	err = adc_mcp356xr_start_read(dev, sequence);
	adc_context_release(&(data->ctx), err);

	return err;
}
#endif

static void adc_mcp356xr_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adc_mcp356xr_data *data = p1;
	const struct device *dev = data->dev;

	while (1) {
		int err = 0;

		k_sem_take(&(data->start_sequence), K_FOREVER);

#ifndef CONFIG_ADC_MCP356XR_POLL
		k_sem_reset(&(data->irq_occurred));
#endif

		/* clear any leftover flags and sanitize state */
		err = adc_mcp356xr_get_status_and_sanitize(dev, NULL, NULL, NULL);
		if (err) {
			LOG_ERR("Failed to sanitize ADC state (error = %d)", err);
			adc_context_complete(&(data->ctx), err);
			continue;
		}

		err = adc_mcp356xr_run_acquisition_sequence(dev);
		if (err) {
			adc_context_complete(&(data->ctx), err);
			continue;
		}

		adc_context_on_sampling_done(&(data->ctx), data->dev);
	}
}

static int adc_mcp356xr_init(const struct device *dev)
{
	const struct adc_mcp356xr_config *config = dev->config;
	struct adc_mcp356xr_data *data = dev->data;

	int err = 0;

	if (!spi_is_ready_dt(&(config->spi))) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	err = adc_mcp356xr_reset_after_power_on(dev);
	if (err) {
		return err;
	}

#ifndef CONFIG_ADC_MCP356XR_POLL
	err = adc_mcp356xr_irq_init(dev);
	if (err) {
		return err;
	}
#endif

	k_sem_init(&(data->start_sequence), 0, 1);

	err = k_mutex_init(&(data->channel_registers_mutex));
	if (err) {
		LOG_ERR("Failed to initialize internal mutex");
	}

	adc_context_init(&(data->ctx));

	k_tid_t tid =
		k_thread_create(&(data->thread), data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
				adc_mcp356xr_acquisition_thread, (void *)data, NULL, NULL,
				CONFIG_ADC_MCP356XR_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, dev->name);

	adc_context_unlock_unconditionally(&(data->ctx));

	return 0;
}

static DEVICE_API(adc, adc_mcp356xr_api) = {
	.channel_setup = adc_mcp356xr_channel_setup,
	.read = adc_mcp356xr_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_mcp356xr_read_async,
#endif
	.ref_internal = 2400,
};

#define MCP356XR_SPI_OPERATION                                                                     \
	(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LINES_SINGLE)

#ifdef CONFIG_ADC_MCP356XR_USE_READ_CRC
#define ADC_MCP356XR_INIT_CONFIG3_EN_CRCCOM MCP356XR_REGISTER_CONFIG_FLAG_ENABLED
#else
#define ADC_MCP356XR_INIT_CONFIG3_EN_CRCCOM MCP356XR_REGISTER_CONFIG_FLAG_DISABLED
#endif

#define DT_INST_CLK_SEL(index)                                                                     \
	DT_INST_PROP(index, use_internal_clock)                                                    \
	? (DT_INST_PROP(index, enable_analog_clock_output)                                         \
		   ? MCP356XR_REGISTER_CONFIG0_CLK_SEL_INTERNAL_CLOCK_OUTPUT_AMCLK                 \
		   : MCP356XR_REGISTER_CONFIG0_CLK_SEL_INTERNAL_CLOCK_NO_OUTPUT)                   \
	: MCP356XR_REGISTER_CONFIG0_CLK_SEL_EXTERNAL_CLOCK

#define DT_INST_PRE(index) DT_INST_PROP(index, analog_clock_prescaler)

#define DT_INST_BOOST(index) DT_INST_PROP(index, boost_current_bias)

#define DT_INST_AZ_MUX(index)                                                                      \
	DT_INST_PROP(index, enable_adc_offset_cancellation)                                        \
	? MCP356XR_REGISTER_CONFIG_FLAG_ENABLED : MCP356XR_REGISTER_CONFIG_FLAG_DISABLED

#define DT_INST_AZ_REF(index)                                                                      \
	DT_INST_PROP(index, enable_internal_vref_offset_cancellation)                              \
	? MCP356XR_REGISTER_CONFIG_FLAG_ENABLED : MCP356XR_REGISTER_CONFIG_FLAG_DISABLED

#define DT_INST_IRQ_MODE(index)                                                                    \
	DT_INST_PROP(index, irq_pin_drive_open_drain)                                              \
	? MCP356XR_REGISTER_IRQ_IRQ_MODE_IRQ_OUTPUT_OPEN_DRAIN                                     \
	: MCP356XR_REGISTER_IRQ_IRQ_MODE_IRQ_OUTPUT_PUSH_PULL

#define ADC_MCP356XR_INIT_CONFIG0(index)                                                           \
	MCP356XR_REGISTER_FIELD_PREP(                                                              \
		CONFIG0, CONFIG0,                                                                  \
		MCP356XR_REGISTER_CONFIG0_CONFIG0_DO_NOT_ENTER_PARTIAL_SHUTDOWN) |                 \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG0, CLK_SEL, DT_INST_CLK_SEL(index)) |           \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG0, CS_SEL,                                      \
					     MCP356XR_REGISTER_CONFIG0_CS_SEL_NO_CURRENT) |        \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG0, ADC_MODE,                                    \
					     MCP356XR_REGISTER_CONFIG0_ADC_MODE_STANDBY)

#define ADC_MCP356XR_INIT_CONFIG1(index)                                                           \
	MCP356XR_REGISTER_FIELD_PREP(CONFIG1, PRE, DT_INST_PRE(index))

#define ADC_MCP356XR_INIT_CONFIG2(index)                                                           \
	MCP356XR_REGISTER_FIELD_PREP(CONFIG2, BOOST, DT_INST_BOOST(index)) |                       \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG2, AZ_MUX, DT_INST_AZ_MUX(index)) |             \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG2, AZ_REF, DT_INST_AZ_REF(index)) |             \
		0x1 /* see datasheet page 93 : bit 1 must be set to "1" */

#define ADC_MCP356XR_INIT_CONFIG3(index)                                                           \
	MCP356XR_REGISTER_FIELD_PREP(CONFIG3, CONV_MODE,                                           \
				     MCP356XR_REGISTER_CONFIG3_CONV_MODE_ONE_SHOT_GO_STANDBY) |    \
		MCP356XR_REGISTER_FIELD_PREP(                                                      \
			CONFIG3, DATA_FORMAT,                                                      \
			MCP356XR_REGISTER_CONFIG3_DATA_FORMAT_8_BITS_SIGN_24_BITS_DATA) |          \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG3, CRC_FORMAT,                                  \
					     MCP356XR_REGISTER_CONFIG3_CRC_FORMAT_16_BITS_CRC) |   \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG3, EN_CRCCOM,                                   \
					     ADC_MCP356XR_INIT_CONFIG3_EN_CRCCOM) |                \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG3, EN_OFFCAL,                                   \
					     MCP356XR_REGISTER_CONFIG_FLAG_DISABLED) |             \
		MCP356XR_REGISTER_FIELD_PREP(CONFIG3, EN_GAINCAL,                                  \
					     MCP356XR_REGISTER_CONFIG_FLAG_DISABLED)

#define ADC_MCP356XR_INIT_IRQ(index)                                                               \
	MCP356XR_REGISTER_FIELD_PREP(IRQ, EN_FASTCMD, MCP356XR_REGISTER_CONFIG_FLAG_ENABLED) |     \
		MCP356XR_REGISTER_FIELD_PREP(IRQ, EN_STP,                                          \
					     MCP356XR_REGISTER_CONFIG_FLAG_DISABLED) |             \
		MCP356XR_REGISTER_FIELD_PREP(IRQ, IRQ_MODE, DT_INST_IRQ_MODE(index))

#define ADC_MCP356XR_INIT(index)                                                                   \
	static const struct adc_mcp356xr_config adc_mcp356xr_config_##index = {                    \
		.spi = SPI_DT_SPEC_INST_GET(index, MCP356XR_SPI_OPERATION),                        \
		.address = DT_INST_PROP(index, address),                                           \
		.vref_settle_time = K_MSEC(DT_INST_PROP_OR(index, vref_settle_time_ms, 0)),        \
		.init_reg_value =                                                                  \
			{                                                                          \
				[REGISTER_INDEX(CONFIG0)] = ADC_MCP356XR_INIT_CONFIG0(index),      \
				[REGISTER_INDEX(CONFIG1)] = ADC_MCP356XR_INIT_CONFIG1(index),      \
				[REGISTER_INDEX(CONFIG2)] = ADC_MCP356XR_INIT_CONFIG2(index),      \
				[REGISTER_INDEX(CONFIG3)] = ADC_MCP356XR_INIT_CONFIG3(index),      \
				[REGISTER_INDEX(IRQ)] = ADC_MCP356XR_INIT_IRQ(index),              \
				[REGISTER_INDEX(MUX)] = 0,                                         \
			},                                                                         \
		IF_DISABLED(CONFIG_ADC_MCP356XR_POLL,                                              \
			    (.irq = GPIO_DT_SPEC_INST_GET(index, irq_gpios),))                     \
	};                                                                                         \
                                                                                                   \
	static struct adc_mcp356xr_data adc_mcp356xr_data_##index = {                              \
		ADC_CONTEXT_INIT_TIMER(adc_mcp356xr_data_##index, ctx),                            \
		ADC_CONTEXT_INIT_LOCK(adc_mcp356xr_data_##index, ctx),                             \
		ADC_CONTEXT_INIT_SYNC(adc_mcp356xr_data_##index, ctx),                             \
		.dev = DEVICE_DT_INST_GET(index),                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, adc_mcp356xr_init, NULL, &adc_mcp356xr_data_##index,          \
			      &adc_mcp356xr_config_##index, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, \
			      &adc_mcp356xr_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_MCP356XR_INIT)
