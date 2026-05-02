/*
 * Copyright (c) 2025 Srishtik Bhandarkar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PZEM004T_PZEM004T_H_
#define ZEPHYR_DRIVERS_SENSOR_PZEM004T_PZEM004T_H_

/*
 * Register addresses for the PZEM004T sensor.
 * Addresses correspond to sensor measurement.
 */
#define VOLTAGE_REGISTER_ADDRESS      0x0000
#define CURRENT_REGISTER_ADDRESS      0x0001
#define POWER_REGISTER_ADDRESS        0x0003
#define ENERGY_REGISTER_ADDRESS       0x0005
#define FREQUENCY_REGISTER_ADDRESS    0x0007
#define POWER_FACTOR_REGISTER_ADDRESS 0x0008
#define ALARM_STATUS_REGISTER_ADDRESS 0x0009

/*
 * Register addresses for the PZEM004T sensor.
 * Addresses correspond to sensor configuration.
 */
#define POWER_ALARM_THRESHOLD_ADDRESS 0x0001
#define MODBUS_RTU_ADDRESS_REGISTER   0x0002

/*
 * Register lengths for the PZEM004T sensor.
 * Lengths correspond to sensor measurement.
 */
#define VOLTAGE_REGISTER_LENGTH      0x0001
#define CURRENT_REGISTER_LENGTH      0x0002
#define POWER_REGISTER_LENGTH        0x0002
#define ENERGY_REGISTER_LENGTH       0x0002
#define FREQUENCY_REGISTER_LENGTH    0x0001
#define POWER_FACTOR_REGISTER_LENGTH 0x0001
#define ALARM_STATUS_REGISTER_LENGTH 0x0001

/*
 * Register lengths for the PZEM004T sensor.
 * Lengths correspond to sensor configuration.
 */
#define POWER_ALARM_THRESHOLD_REGISTER_LENGTH 0x0001
#define MODBUS_RTU_ADDRESS_REGISTER_LENGTH    0x0001

/*
 * The total length of the register is 10 Units (16 bits each),
 * which includes 2 bytes for voltage, 4 bytes for current, 4 bytes for power,
 * 4 bytes for energy, 2 bytes for frequency, and 2 bytes for power factor.
 */
#define MEASUREMENT_REGISTER_START_ADDRESS 0x0000
#define MEASUREMENT_REGISTER_TOTAL_LENGTH  0x000A

/* Scaling factors for the PZEM004T sensor */
#define PZEM004T_VOLTAGE_SCALE      10   /* Voltage in 0.1V */
#define PZEM004T_CURRENT_SCALE      1000 /* Current in 0.001A */
#define PZEM004T_POWER_SCALE        10   /* Power in 0.1 W */
#define PZEM004T_ENERGY_SCALE       1    /* Energy in 1 Wh */
#define PZEM004T_FREQUENCY_SCALE    10   /* Frequency in 0.1Hz */
#define PZEM004T_POWER_FACTOR_SCALE 100  /* Power factor in 0.01 */

/* Maximum power alarm threshold in watts */
#define PZEM004T_MAX_POWER_ALARM_THRESHOLD	23000
/* Maximum Modbus RTU address */
#define PZEM004T_MAX_MODBUS_RTU_ADDRESS		0xF7
/* PZEM004T default modbus address*/
#define PZEM004T_DEFAULT_MODBUS_ADDRESS		0xF8
/* PZEM004T reset energy response error code*/
#define PZEM004T_RESET_ENERGY_CUSTOM_FC		0x42

struct pzem004t_config {
	const char *modbus_iface_name;
	const struct modbus_iface_param client_param;
};

/* Data structure to hold runtime data */
struct pzem004t_data {
	int iface;                      /* Modbus interface */
	uint8_t modbus_address;         /* Modbus address */
	uint16_t voltage;               /* Latest voltage reading */
	uint16_t current;               /* Latest current reading */
	uint16_t power;                 /* Latest power reading */
	uint32_t energy;                /* Latest energy reading */
	uint16_t frequency;             /* Latest frequency reading */
	uint16_t power_factor;          /* Latest power factor reading */
	uint16_t alarm_status;          /* Latest alarm status */
	uint16_t power_alarm_threshold; /* Power alarm threshold */
	uint8_t modbus_rtu_address;     /* Modbus RTU address */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_PZEM004T_PZEM004T_H_ */
