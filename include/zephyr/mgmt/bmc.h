/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_H_

#include <stdbool.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

int bmc_init(void);
bool bmc_is_boot_finished(void);
FUNC_NORETURN void bmc_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_H_ */
