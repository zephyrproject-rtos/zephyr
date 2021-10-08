/*
 * Copyright (c) 2021 Bernhard Kraemer
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MDIO_MCUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MDIO_MCUX_H_

#include <drivers/mdio.h>

/**
 * @brief      Signal Transfer Complete
 *
 * @param[in]  dev   Pointer to the device structure for the controller
 *
 */
__syscall void mdio_mcux_transfer_complete(const struct device *dev);

#include <syscalls/mdio_mcux.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MDIO_MCUX_H_ */
