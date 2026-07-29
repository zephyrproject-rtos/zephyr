/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

#ifndef BC66X_HW_H_
#define BC66X_HW_H_

#include "bc66x.h"

int bc66x_hw_init(const struct bc66x_config *dev_cfg);
int bc66_hw_start(const struct bc66x_config *dev_cfg);
int bc660k_hw_start(const struct bc66x_config *dev_cfg);
int bc66x_hw_psm_pulse_ms(const struct bc66x_config *dev_cfg, int wait_ms);

#endif /* BC66X_HW_H_ */
