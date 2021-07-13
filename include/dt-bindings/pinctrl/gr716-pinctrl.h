/*
 * Copyright (c) 2021 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GR716_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GR716_PINCTRL_H_

#define GR716_IO_MODE_MAX               0x000d
#define GR716_IO_MODE_MASK              0x000f

#define GR716_IO_PULL_MASK              0x0300
#define GR716_IO_PULL_DONTCHANGE        0x0000
#define GR716_IO_PULL_DISABLE           0x0100
#define GR716_IO_PULL_UP                0x0200
#define GR716_IO_PULL_DOWN              0x0300

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GR716_PINCTRL_H_ */
