/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFE_CONFIG_H__
#define NRFE_CONFIG_H__

#ifdef CONFIG_GPIO_NRFE
#include <drivers/gpio/nrfe_gpio.h>
#else
#error "NRFE config header included, even though no SW-define IO device is enabled."
#endif

#ifndef NRFE_RESERVED_PPI_CHANNELS
#define NRFE_RESERVED_PPI_CHANNELS 0
#endif

#endif /* NRFE_CONFIG_H__ */
