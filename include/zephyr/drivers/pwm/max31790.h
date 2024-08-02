/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_MAX31790_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_MAX31790_H_

/**
 * @name custom PWM flags for MAX31790
 * These flags can be used with the PWM API in the upper 8 bits of pwm_flags_t
 * They allow the usage of the RPM mode, which will cause the MAX31790 to
 * measure the actual speed of the fan and automatically control it to the
 * desired speed.
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define PWM_MAX31790_FLAG_RPM_MODE_POS		 8
#define PWM_MAX31790_FLAG_SPEED_RANGE_POS	 9
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS 12
#define PWM_MAX31790_FLAG_SPIN_UP_POS		 15
/** @endcond */

/*!
 * @brief RPM mode
 *
 * Activating the RPM mode will cause the parameter pulse of @ref pwm_set_cycles
 * to be interpreted as TACH target count. This basically is the number of internal
 * pulses which occur during each TACH period. Hence, a bigger value means a slower
 * rotation of the fan. The details about the TACH target count has to be calculated
 * can be taken from the datasheet of the MAX31790.
 */
#define PWM_MAX31790_FLAG_RPM_MODE	       (1 << PWM_MAX31790_FLAG_RPM_MODE_POS)
/*!
 * @brief speed range of fan
 *
 * This represents a multiplicator for the TACH count and should be chosen depending
 * on the nominal RPM of the fan. A detailed table on how to choose a proper value
 * can be found in the datasheet of the MAX31790.
 */
#define PWM_MAX31790_FLAG_SPEED_RANGE_1	       (0 << PWM_MAX31790_FLAG_SPEED_RANGE_POS)
#define PWM_MAX31790_FLAG_SPEED_RANGE_2	       (1 << PWM_MAX31790_FLAG_SPEED_RANGE_POS)
#define PWM_MAX31790_FLAG_SPEED_RANGE_4	       (2 << PWM_MAX31790_FLAG_SPEED_RANGE_POS)
#define PWM_MAX31790_FLAG_SPEED_RANGE_8	       (3 << PWM_MAX31790_FLAG_SPEED_RANGE_POS)
#define PWM_MAX31790_FLAG_SPEED_RANGE_16       (4 << PWM_MAX31790_FLAG_SPEED_RANGE_POS)
#define PWM_MAX31790_FLAG_SPEED_RANGE_32       (5 << PWM_MAX31790_FLAG_SPEED_RANGE_POS)
/*!
 * @brief PWM rate of change
 *
 * Configures the internal control loop of the fan. Details about these values can be found
 * in the datasheet of the MAX31790.
 */
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_0 (0 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_1 (1 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_2 (2 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_3 (3 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_4 (4 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_5 (5 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_6 (6 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
#define PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_7 (7 << PWM_MAX31790_FLAG_PWM_RATE_OF_CHANGE_POS)
/*!
 * @brief activate spin up for fan
 *
 * This activates the spin up of the fan, which means that the controller will force the fan
 * to maximum speed for a startup from a completely stopped state.
 */
#define PWM_MAX31790_FLAG_SPIN_UP	       (1 << PWM_MAX31790_FLAG_SPIN_UP_POS)
/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_MAX31790_H_ */
