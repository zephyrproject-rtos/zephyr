/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_WATCHDOG_H_
#define ZEPHYR_SOC_INTEL_ADSP_WATCHDOG_H_

/**
 * @brief Pause watchdog operation
 *
 * Sets the pause signal to stop the watchdog timing
 *
 * @param dev Pointer to the device structure
 * @param channel_id Channel identifier
 */
int intel_adsp_watchdog_pause(const struct device *dev, const int channel_id);

/**
 * @brief Resume watchdog operation
 *
 * Clears the pause signal to resume the watchdog timing
 *
 * @param dev Pointer to the device structure
 * @param channel_id Channel identifier
 */
int intel_adsp_watchdog_resume(const struct device *dev, const int channel_id);

#endif /* ZEPHYR_SOC_INTEL_ADSP_WATCHDOG_H_ */
