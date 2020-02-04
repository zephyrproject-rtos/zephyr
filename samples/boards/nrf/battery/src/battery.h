/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APPLICATION_BATTERY_H_
#define APPLICATION_BATTERY_H_

/** Enable or disable measurement of the battery voltage.
 *
 * @param enable true to enable, false to disable
 *
 * @return zero on success, or a negative error code.
 */
int battery_measure_enable(bool enable);

/** Measure the battery voltage.
 *
 * @return the battery voltage in millivolts, or a negative error
 * code.
 */
int battery_sample(void);

#endif /* APPLICATION_BATTERY_H_ */
