/*
 * Copyright (c) 2025 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Performs a magnetic reset.
 *
 * The BMM350 has measures to recover from excessively strong
 * magnetic fields. A magnetic reset is trigged after reset by
 * the device itself, but if excessive field exposure has occurred
 * in suspend mode, the sensor can not detect that event. If a
 * large offset or sensitvity drift indicates that such an event
 * has occurred, then this function can be called by the
 * application.
 *
 * "Enhanced" Magnetic Reset supported by later BMM350 Revisions
 * is not yet implemented.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int bmm350_magnetic_reset(const struct device *dev);
