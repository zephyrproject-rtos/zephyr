/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/sensor/m90e26.h>

#define M90E26_CMD_READ_MASK     (BIT(7))
#define M90E26_CMD_WRITE_MASK    (0x7F)
#define M90E26_IS_READ_CMD(cmd)  (((cmd) & M90E26_CMD_READ_MASK) != 0)
#define M90E26_IS_WRITE_CMD(cmd) (((cmd) & M90E26_CMD_READ_MASK) == 0)

#if CONFIG_M90E26_BUS_UART
struct m90e26_uart_frame {
	uint8_t start_byte;
	uint8_t addr;
	uint8_t data_high;
	uint8_t data_low;
	uint8_t rcv_checksum;
} __packed;
#endif

union m90e26_bus {
#if CONFIG_M90E26_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_M90E26_BUS_UART
	struct uart_dt_spec {
		const struct device *bus;
		struct uart_config config;
	} uart;
#endif
};

/**
 * @brief m90e26_trigger_ctx Structure.
 *
 * This structure holds the trigger context for M90E26 sensor triggers.
 */
struct m90e26_trigger_ctx {
	struct sensor_trigger trigger;
	sensor_trigger_handler_t handler;
	struct gpio_callback gpio_cb;
};

/**
 * @brief m90e26_data Structure.
 *
 * This structure holds all the measurement data read from the M90E26 energy metering IC
 * in a raw format as per the device's register map.
 *
 * @note Energy registers holds the last read values from the device. So the IC clear these
 * registers after read operation, the driver must fetch the data again from the device
 * to update these values.
 */
struct m90e26_data {
	struct m90e26_energy_data {
		uint16_t APenergy;
		uint16_t ANenergy;
		uint16_t ATenergy;
		uint16_t RPenergy;
		uint16_t RNenergy;
		uint16_t RTenergy;
	} energy_values;

	struct m90e26_power_data {
		int16_t Pmean;
		int16_t Qmean;
		int16_t Smean;
		int16_t Pmean2;
		int16_t Qmean2;
		int16_t Smean2;
	} power_values;

	uint16_t Urms;

	struct m90e26_current_data {
		uint16_t Irms;
		uint16_t Irms2;
	} current_values;

	uint16_t Freq;

	struct m90e26_phase_angle_data {
		int16_t Pangle;
		int16_t Pangle2;
	} pangle_values;

	struct m90e26_power_factor_data {
		int16_t PowerF;
		int16_t PowerF2;
	} pfactor_values;

	struct k_mutex bus_lock;
	struct k_mutex config_lock;

#if CONFIG_M90E26_BUS_UART
	struct k_mutex rx_lock;
	struct m90e26_uart_frame frame;
#endif

	struct m90e26_trigger_ctx irq_ctx;
	struct m90e26_trigger_ctx wrn_out_ctx;
	struct m90e26_trigger_ctx cf1;
	struct m90e26_trigger_ctx cf2;

	struct m90e26_config_registers config_registers;
};

typedef int (*m90e26_bus_check_func)(const struct device *dev);
typedef int (*m90e26_read_func)(const struct device *dev, const m90e26_register_t addr,
				m90e26_data_value_t *value);
typedef int (*m90e26_write_func)(const struct device *dev, const m90e26_register_t addr,
				 const m90e26_data_value_t *value);

/**
 * @brief Structure defining the bus I/O functions for M90E26 communication.
 */
struct m90e26_bus_io {
	m90e26_bus_check_func bus_check;
	m90e26_read_func read;
	m90e26_write_func write;
};

/**
 * @brief m90e26_config Structure.
 *
 * This structure holds the configuration parameters of pins and data buses of the M90E26
 * energy metering IC driver.
 */
struct m90e26_config {
	union m90e26_bus bus;
	const struct m90e26_bus_io *bus_io;
	struct gpio_dt_spec irq;
	struct gpio_dt_spec wrn_out;
	struct gpio_dt_spec cf1;
	struct gpio_dt_spec cf2;
};

#if CONFIG_M90E26_BUS_SPI
#define M90E26_SPI_OPERATION                                                                       \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_HOLD_ON_CS | SPI_LOCK_ON |  \
	 SPI_MODE_CPOL | SPI_MODE_CPHA)
extern const struct m90e26_bus_io m90e26_bus_io_spi;
#endif

#if CONFIG_M90E26_BUS_UART
#define M90E26_UART_START_BYTE 0xFE
extern const struct m90e26_bus_io m90e26_bus_io_uart;
#endif

/**
 * @brief Convert M90E26 energy register value to sensor_value. Basead on datasheet value scaling.
 *
 * @note 1 LSB = 0.1CF
 *
 * @param reg Pointer to the M90E26 energy register value.
 * @param val Pointer to the sensor_value structure to store the converted energy value.
 * @return int 0 on success, negative error code on failure.
 */
static inline int m90e26_convert_energy(const m90e26_data_value_t *reg, struct sensor_value *val)
{
	return sensor_value_from_float(val, reg->uint16 * 0.1f);
}

/**
 * @brief Convert M90E26 power register value to sensor_value. Based on datasheet value scaling.
 *
 * @note 1 LSB = 1W/VAR/VA
 *
 * @param reg Pointer to the M90E26 power register value.
 * @param val Pointer to the sensor_value structure to store the converted power value.
 * @return int 0 on success, negative error code on failure.
 */
static inline int m90e26_convert_power(const m90e26_data_value_t *reg, struct sensor_value *val)
{
	return sensor_value_from_float(val, reg->int16);
}

/**
 * @brief Convert M90E26 current register value to sensor_value. Based on datasheet value scaling.
 *
 * @note 1 LSB = 0.001A
 *
 * @param reg Pointer to the M90E26 current register value.
 * @param val Pointer to the sensor_value structure to store the converted current value.
 * @return int 0 on success, negative error code on failure.
 */
static inline int m90e26_convert_current(const m90e26_data_value_t *reg, struct sensor_value *val)
{
	return sensor_value_from_float(val, reg->uint16 * 0.001f);
}

/**
 * @brief Convert M90E26 phase angle register value to sensor_value. Based on datasheet value
 * scaling.
 *
 * @note 1 LSB = 0.1 degrees
 *
 * @param reg Pointer to the M90E26 phase angle register value.
 * @param val Pointer to the sensor_value structure to store the converted phase angle value.
 * @return int 0 on success, negative error code on failure.
 */
static inline int m90e26_convert_pangle(const m90e26_data_value_t *reg, struct sensor_value *val)
{
	return sensor_value_from_float(val, reg->int16 * 0.1f);
}

/**
 * @brief Convert M90E26 power factor register value to sensor_value. Based on datasheet value
 * scaling.
 *
 * @note 1 LSB = 0.001
 *
 * @param reg Pointer to the M90E26 power factor register value.
 * @param val Pointer to the sensor_value structure to store the converted power factor value.
 * @return int 0 on success, negative error code on failure.
 */
static inline int m90e26_convert_pfactor(const m90e26_data_value_t *reg, struct sensor_value *val)
{
	return sensor_value_from_float(val, reg->int16 * 0.001f);
}

#endif /* ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E26_H_ */
