/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Device Data Context header */

#ifndef ZEPHYR_INCLUDE_SYS_DDC_H
#define ZEPHYR_INCLUDE_SYS_DDC_H

#include <zephyr/device.h>

struct ddc_config;

/* Macro to use as very first attribute in a custom device config structure */
#define DDC_CFG struct ddc_config ddc_cfg

/* Including generated ddc_types.h */
#include <ddc_types.h>

#endif /* ZEPHYR_INCLUDE_SYS_DDC_H */
