/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The real altctrl driver is not compiled (CONFIG_MFD_ITE_IT8801=n), but
 * DEVICE_DT_GET references in the KBD driver's mfdctrl macros require device
 * structs to exist. Create minimal stub devices for any it8801-altctrl nodes
 * in the device tree.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul_stub_device.h>

#define DT_DRV_COMPAT ite_it8801_altctrl

DT_INST_FOREACH_STATUS_OKAY(EMUL_STUB_DEVICE);
