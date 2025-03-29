/*
 * Original license from ina23x driver:
 *  Copyright 2021 The Chromium OS Authors
 *  Copyright 2021 Grinn
 *
 * Copyright 2024, Remie Lowik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INA700_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INA700_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Operating Mode */
#define INA700_OPER_MODE_SHUTDOWN                                   0x0
#define INA700_OPER_MODE_TRIGGERED_BUS_VOLTAGE                      0x1
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE                      0x4
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE_BUS_VOLTAGE          0x5
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE_CURRENT              0x6
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE_CURRENT_BUS_VOLTAGE  0x7
#define INA700_OPER_MODE_CONTINUOUS_BUS_VOLTAGE                     0x9
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE                     0xc
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE_BUS_VOLTAGE         0xd
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE_CURRENT             0xe
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE_CURRENT_BUS_VOLTAGE 0xf

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INA700_H_ */
