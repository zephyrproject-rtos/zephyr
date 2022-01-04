/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ESPI_V2_H_
#define _SOC_ESPI_V2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

int mchp_xec_espi_host_dev_init(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* _SOC_ESPI_V2_H_ */
