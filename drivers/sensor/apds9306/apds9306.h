/* Copyright (c) 2024 Daniel Kampert
 * Author: Daniel Kampert <DanielKampert@kampis-Elektroecke.de>
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_APDS9960_APDS9306_H_
#define ZEPHYR_DRIVERS_SENSOR_APDS9960_APDS9306_H_

/** @brief Attribute to set the gain range.
 */
#define SENSOR_APDS9306_ATTR_GAIN (SENSOR_ATTR_PRIV_START + 1)

/** @brief Attribute to set the resolution.
 */
#define SENSOR_APDS9306_ATTR_RESOLUTION (SENSOR_ATTR_PRIV_START + 2)

#endif /* ZEPHYR_DRIVERS_SENSOR_APDS9960_APDS9306_H_*/
