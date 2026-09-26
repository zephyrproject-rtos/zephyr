/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_H_
#define ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/types.h>

#include <zephyr/drivers/sensor/m90e3x.h>

#define M90E3X_SPI_OPERATION                                                                       \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(16) | SPI_TRANSFER_MSB | SPI_HOLD_ON_CS | SPI_LOCK_ON | \
	 SPI_MODE_CPOL | SPI_MODE_CPHA)
#define M90E3X_SPI_READ_MASK  (1U << 15)
#define M90E3X_SPI_WRITE_MASK (0x7FFFU)

/**
 * @brief M90E3X Power Modes
 *
 * @note MSB is PM1 pin. LSB is PM0 pin.
 */
#define M90E3X_MODE_NORMAL              0x3
#define M90E3X_MODE_PARTIAL_MEASUREMENT 0x2
#define M90E3X_MODE_DETECTION           0x1
#define M90E3X_MODE_IDLE                0x0

#define M90E3X_PM0_NORMAL_BIT              (M90E3X_MODE_NORMAL & 0x1)
#define M90E3X_PM1_NORMAL_BIT              (M90E3X_MODE_NORMAL >> 1)
#define M90E3X_PM0_PARTIAL_MEASUREMENT_BIT (M90E3X_MODE_PARTIAL_MEASUREMENT & 0x1)
#define M90E3X_PM1_PARTIAL_MEASUREMENT_BIT (M90E3X_MODE_PARTIAL_MEASUREMENT >> 1)
#define M90E3X_PM0_DETECTION_BIT           (M90E3X_MODE_DETECTION & 0x1)
#define M90E3X_PM1_DETECTION_BIT           (M90E3X_MODE_DETECTION >> 1)
#define M90E3X_PM0_IDLE_BIT                (M90E3X_MODE_IDLE & 0x1)
#define M90E3X_PM1_IDLE_BIT                (M90E3X_MODE_IDLE >> 1)

typedef int (*m90e3x_bus_check_func)(const struct device *dev);
typedef int (*m90e3x_read_func)(const struct device *dev, const m90e3x_register_t addr,
				m90e3x_data_value_t *value);
typedef int (*m90e3x_write_func)(const struct device *dev, const m90e3x_register_t addr,
				 const m90e3x_data_value_t *value);

/**
 * @brief Structure defining the bus I/O functions for M90E3X communication.
 */
struct m90e3x_bus_io {
	m90e3x_bus_check_func bus_check;
	m90e3x_read_func read;
	m90e3x_write_func write;
};

typedef int (*m90e3x_pm_normal_mode_func)(const struct device *dev);
typedef int (*m90e3x_pm_detection_mode_func)(const struct device *dev);
typedef int (*m90e3x_pm_partial_measurement_mode_func)(const struct device *dev);
typedef int (*m90e3x_pm_idle_mode_func)(const struct device *dev);

/**
 * @brief Structure defining the power management mode operations for M90E3X.
 *
 * @note Each function pointer corresponds to a function that sets the M90E3X
 * device into the respective power mode.
 */
struct m90e3x_pm_mode_ops {
	m90e3x_pm_idle_mode_func enter_idle_mode;
	m90e3x_pm_detection_mode_func enter_detection_mode;
	m90e3x_pm_partial_measurement_mode_func enter_partial_measurement_mode;
	m90e3x_pm_normal_mode_func enter_normal_mode;
};

/**
 * @brief Structure defining the trigger context for M90E3X.
 *
 * @note One instance of this structure is created per trigger type (IRQ0, IRQ1, WRN_OUT).
 */
struct m90e3x_trigger_ctx {
	struct sensor_trigger trigger;
	sensor_trigger_handler_t handler;
	struct gpio_callback gpio_cb;
};

/**
 * @brief m90e3x_data Structure.
 *
 * This structure holds all the measurement data read from the M90E3X energy metering IC
 * in a raw format as per the device's register map.
 *
 * @note Energy registers holds the last read values from the device. So the IC clear these
 * registers after read operation, the driver must fetch the data again from the device
 * to update these values.
 *
 * @note Peak registers are only in M90E32AS devices. Otherwise this registers holds THD+N
 * registers for M90E36A devices.
 */
struct m90e3x_data {
	struct m90e3x_energy_data {
		uint16_t APenergyT;
		uint16_t APenergyA;
		uint16_t APenergyB;
		uint16_t APenergyC;
		uint16_t ANenergyT;
		uint16_t ANenergyA;
		uint16_t ANenergyB;
		uint16_t ANenergyC;
		uint16_t RPenergyT;
		uint16_t RPenergyA;
		uint16_t RPenergyB;
		uint16_t RPenergyC;
		uint16_t RNenergyT;
		uint16_t RNenergyA;
		uint16_t RNenergyB;
		uint16_t RNenergyC;
		uint16_t SAenergyT;
		uint16_t SenergyA;
		uint16_t SenergyB;
		uint16_t SenergyC;
	} energy_values;

	struct m90e3x_fundamental_energy_data {
		uint16_t APenergyTF;
		uint16_t APenergyAF;
		uint16_t APenergyBF;
		uint16_t APenergyCF;
		uint16_t ANenergyTF;
		uint16_t ANenergyAF;
		uint16_t ANenergyBF;
		uint16_t ANenergyCF;
	} fundamental_energy_values;

	struct m90e3x_harmonic_energy_data {
		uint16_t APenergyTH;
		uint16_t APenergyAH;
		uint16_t APenergyBH;
		uint16_t APenergyCH;
		uint16_t ANenergyTH;
		uint16_t ANenergyAH;
		uint16_t ANenergyBH;
		uint16_t ANenergyCH;
	} harmonic_energy_values;

	struct m90e3x_power_data {
		int16_t PmeanT;
		int16_t PmeanTLSB;
		int16_t PmeanA;
		int16_t PmeanALSB;
		int16_t PmeanB;
		int16_t PmeanBLSB;
		int16_t PmeanC;
		int16_t PmeanCLSB;
		int16_t QmeanT;
		int16_t QmeanTLSB;
		int16_t QmeanA;
		int16_t QmeanALSB;
		int16_t QmeanB;
		int16_t QmeanBLSB;
		int16_t QmeanC;
		int16_t QmeanCLSB;
		int16_t SmeanT;
		int16_t SAmeanTLSB;
		int16_t SmeanA;
		int16_t SmeanALSB;
		int16_t SmeanB;
		int16_t SmeanBLSB;
		int16_t SmeanC;
		int16_t SmeanCLSB;
	} power_values;

	struct m90e3x_power_factor_data {
		int16_t PFmeanT;
		int16_t PFmeanA;
		int16_t PFmeanB;
		int16_t PFmeanC;
	} power_factor_values;

	struct m90e3x_fundamental_power_data {
		int16_t PmeanTF;
		int16_t PmeanTFLSB;
		int16_t PmeanAF;
		int16_t PmeanAFLSB;
		int16_t PmeanBF;
		int16_t PmeanBFLSB;
		int16_t PmeanCF;
		int16_t PmeanCFLSB;
	} fundamental_power_values;

	struct m90e3x_harmonic_power_data {
		int16_t PmeanTH;
		int16_t PmeanTHLSB;
		int16_t PmeanAH;
		int16_t PmeanAHLSB;
		int16_t PmeanBH;
		int16_t PmeanBHLSB;
		int16_t PmeanCH;
		int16_t PmeanCHLSB;
	} harmonic_power_values;

	struct m90e3x_voltage_rms_data {
		uint16_t UrmsA;
		uint16_t UrmsALSB;
		uint16_t UrmsB;
		uint16_t UrmsBLSB;
		uint16_t UrmsC;
		uint16_t UrmsCLSB;
	} voltage_rms_values;

	struct m90e3x_current_rms_data {
		uint16_t IrmsN;
		uint16_t IrmsA;
		uint16_t IrmsALSB;
		uint16_t IrmsB;
		uint16_t IrmsBLSB;
		uint16_t IrmsC;
		uint16_t IrmsCLSB;
	} current_rms_values;

	union {
		struct m90e32as_peak_data {
			uint16_t UPeakA;
			uint16_t UPeakB;
			uint16_t UPeakC;
			uint16_t IPeakA;
			uint16_t IPeakB;
			uint16_t IPeakC;
		} peak_values;
		struct m90e36a_thdn_data {
			uint16_t THDNUA;
			uint16_t THDNUB;
			uint16_t THDNUC;
			uint16_t THDNIA;
			uint16_t THDNIB;
			uint16_t THDNIC;
		} thdn_values;
	};

	struct m90e3x_phase_angle_data {
		uint16_t PAngleA;
		uint16_t PAngleB;
		uint16_t PAngleC;
		uint16_t UangleA;
		uint16_t UangleB;
		uint16_t UangleC;
	} phase_angle_values;

	uint16_t Freq;
	int16_t Temp;

	struct k_mutex bus_lock;

	enum m90e3x_power_mode current_power_mode;

	struct m90e3x_trigger_ctx cf1;
	struct m90e3x_trigger_ctx cf2;
	struct m90e3x_trigger_ctx cf3;
	struct m90e3x_trigger_ctx cf4;
	struct m90e3x_trigger_ctx irq0_ctx;
	struct m90e3x_trigger_ctx irq1_ctx;
	struct m90e3x_trigger_ctx wrn_out_ctx;

	union {
		struct m90e32as_config_registers m90e32as_config_registers;
	};
};

/**
 * @brief m90e3x_config Structure.
 *
 * This structure holds the configuration parameters of pins and data buses of the M90E3X
 * energy metering IC driver.
 */
struct m90e3x_config {
	struct spi_dt_spec bus;
	const struct m90e3x_bus_io *bus_io;
	const struct m90e3x_pm_mode_ops *pm_mode_ops;
	struct gpio_dt_spec irq0;
	struct gpio_dt_spec irq1;
	struct gpio_dt_spec wrn_out;
	struct gpio_dt_spec cf1;
	struct gpio_dt_spec cf2;
	struct gpio_dt_spec cf3;
	struct gpio_dt_spec cf4;
	struct gpio_dt_spec pm0;
	struct gpio_dt_spec pm1;
};

extern const struct m90e3x_bus_io m90e3x_bus_io_spi;
extern const struct m90e3x_pm_mode_ops m90e3x_pm_mode;

/**
 * @brief Convert M90E3X energy register value to float. Basead on datasheet value scaling.
 *
 * @note 1 LSB = 0.01CF
 *
 * @param reg The M90E3X energy register value.
 * @return float The converted energy value.
 */
static inline float m90e3x_convert_energy_reg(const m90e3x_data_value_t *reg)
{
	return reg->uint16 * 0.01f;
}

/**
 * @brief Convert M90E3X power register values to float. Based on datasheet value scaling.
 *
 * @note 1 LSB = 0.00032W/VAR/VA
 *
 * @param msb_reg The M90E3X power MSB register value.
 * @param lsb_reg The M90E3X power LSB register value.
 * @return float The converted power value.
 */
static inline float m90e3x_convert_power32_regs(const m90e3x_data_value_t *msb_reg,
						const m90e3x_data_value_t *lsb_reg)
{
	return ((int32_t)(msb_reg->int16 << 16) | lsb_reg->uint16) * 0.00032f;
}

/**
 * @brief Convert M90E3X voltage register values to float. Based on datasheet value scaling.
 *
 * @note 1 LSB = 0.01V
 *
 * @param msb_reg The M90E3X voltage MSB register value.
 * @param lsb_reg The M90E3X voltage LSB register value.
 * @return float The converted voltage value.
 */
static inline float m90e3x_convert_voltage32_regs(const m90e3x_data_value_t *msb_reg,
						  const m90e3x_data_value_t *lsb_reg)
{
	return (msb_reg->uint16 + (lsb_reg->uint16 >> 8)) * 0.01f;
}

/**
 * @brief Convert M90E3X current register values to float. Based on datasheet value scaling.
 *
 * @note 1 LSB = 0.001A
 *
 * @param msb_reg The M90E3X current MSB register value.
 * @param lsb_reg The M90E3X current LSB register value.
 * @return float The converted current value.
 */
static inline float m90e3x_convert_current32_regs(const m90e3x_data_value_t *msb_reg,
						  const m90e3x_data_value_t *lsb_reg)
{
	return (msb_reg->uint16 + (lsb_reg->uint16 >> 8)) * 0.001f;
}

/**
 * @brief Convert M90E3X power factor register value to float. Based on datasheet value
 * scaling.
 *
 * @note 1 LSB = 0.001 (-1.000 ~ 1.000)
 *
 * @param reg The M90E3X power factor register value.
 * @return float The converted power factor value.
 */
static inline float m90e3x_convert_power_factor_reg(const m90e3x_data_value_t *reg)
{
	return reg->int16 * 0.001f;
}

/**
 * @brief Convert M90E32AS phase angle register value to float. Based on datasheet value
 * scaling.
 *
 * @note 1 LSB = 0.1° (0 ~ 360°)
 *
 * @param reg The M90E32AS phase angle register value.
 * @return float The converted phase angle value.
 */
static inline float m90e32as_convert_phase_angle_reg(const m90e3x_data_value_t *reg)
{
	return reg->uint16 * 0.1f;
}

/**
 * @brief Convert M90E36A phase angle register value to float. Based on datasheet value
 * scaling.
 *
 * @note 1 LSB = 0.1° (-180 ~ 180°)
 *
 * @param reg The M90E36A phase angle register value.
 * @return float The converted phase angle value.
 */
static inline float m90e36a_convert_phase_angle_reg(const m90e3x_data_value_t *reg)
{
	return reg->int16 * 0.1f;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_ATMEL_M90E3X_H_ */
