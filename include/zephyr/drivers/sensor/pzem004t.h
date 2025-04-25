/*
 * Copyright (c) 2025 Srishtik Bhandarkar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_PZEM004T_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_PZEM004T_H_

#include <zephyr/drivers/sensor.h>

/* PZEM004T specific channels */
enum sensor_channel_pzem004t {
	/* Energy corresponds to active power accumulated over time.
	 * Units: 1Wh (Watt-hours)
	 */
	SENSOR_CHAN_PZEM004T_ENERGY = SENSOR_CHAN_PRIV_START,
	/* Power factor is defined as ratio of real power to apparent power 0.	01 resolution.
	 * Unit: No unit
	 */
	SENSOR_CHAN_PZEM004T_POWER_FACTOR,
	/* Alarm status is 0xFF when current active power is greater than power alarm threshold.
	 * 0x00 when current power is less than power alarm threshold.
	 * Unit: No unit
	 */
	SENSOR_CHAN_PZEM004T_ALARM_STATUS,
	/* Active Power above which the power alarm threshold is set.
	 * Unit: 1W (Watts)
	 */
	SENSOR_CHAN_PZEM004T_POWER_ALARM_THRESHOLD,
	/* Unique Modbus address of each pzem004t device on the modbus. Only use this
	 * to set individual modbus address by connecteing each device individually.
	 */
	SENSOR_CHAN_PZEM004T_MODBUS_RTU_ADDRESS,
	/* Channel used to set the Modbus address of pzem004t device for the device instance.
	 * This does not set the modbus address of the device. It is used to set the
	 * modbus address of the device instance only for the driver.
	 */
	SENSOR_CHAN_PZEM004T_ADDRESS_INST_SET,
	/* Channel used to reset the Energy counter of the pzem004t sensor. Please enable
	 * the CONFIG_PZEM004T_ENABLE_RESET_ENERGY option in prj.conf in application to
	 * use the channel.
	 */
	SENSOR_CHAN_PZEM004T_RESET_ENERGY,
};

enum sensor_attribute_pzem004t {
	/* Active Power above which the power alarm threshold is set.
	 * Unit: 1W (Watts)
	 */
	SENSOR_ATTR_PZEM004T_POWER_ALARM_THRESHOLD = SENSOR_ATTR_PRIV_START,
	/* Unique Modbus address of each pzem004t device on the modbus. Only use this
	 * to set individual modbus address by connecteing each device individually.
	 */
	SENSOR_ATTR_PZEM004T_MODBUS_RTU_ADDRESS,
	/* Attribute used to set the Modbus address of pzem004t device for the device instance.
	 * This does not set the modbus address of the device. It is used to set the
	 * modbus address of the device instance only for the driver.
	 */
	SENSOR_ATTR_PZEM004T_ADDRESS_INST_SET,
	/* Attribute used to reset the Energy counter of the pzem004t sensor. Please enable
	 * the CONFIG_PZEM004T_ENABLE_RESET_ENERGY option in prj.conf in appplicationtn
	 * use the channel.
	 */
	SENSOR_ATTR_PZEM004T_RESET_ENERGY,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_PZEM004T_H_ */
