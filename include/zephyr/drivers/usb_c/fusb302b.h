#ifndef ZEPHYR_INCLUDE_DRIVERS_USBC_FUSB302B_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBC_FUSB302B_H_

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb_c/usbc_tc.h>

/**
 * Measure VBUS exactly. Note: This is slower than fusb302_check_vbus_level,
 * since it manually iterates through comparator configurations.
 *
 * @param dev fusb302b device
 * @param[out] meas Measured VBUS in millivolts
 * @return 0 on success, -EIO on bus error
 */
int fusb302_measure_vbus(const struct device *dev, int *meas);

/**
 * Check if VBUS matches the specified level
 *
 * @param dev fusb302b device
 * @param level Voltage level that VBUS should be tested against
 * @return true if VBUS is < VBUS_SAFE0V or < VBUS_REMOVED or > VBUS_PRESENT
 */
bool fusb302_check_vbus_level(const struct device *dev, enum tc_vbus_level level);

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_FUSB302B_H_ */
