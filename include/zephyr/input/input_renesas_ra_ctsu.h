/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_INPUT_INPUT_RENESAS_RA_CTSU_H_
#define ZEPHYR_INCLUDE_ZEPHYR_INPUT_INPUT_RENESAS_RA_CTSU_H_

#include <rm_touch.h>

#ifdef __cplusplus
extern "C" {
#endif

struct renesas_ra_ctsu_touch_cfg {
	struct st_touch_instance touch_instance;
};

/**
 * @brief Configure CTSU group device with a Renesas QE for Capacitive Touch Workflow generated
 * configuration
 *
 * @param dev Pointer to the input device instance
 * @param cfg Pointer to the configuration data for the device
 *
 * @retval 0 on success
 * @retval -ENOSYS in case INPUT_RENESAS_RA_QE_TOUCH_CFG was not enabled
 * @retval -errno on failure
 */
__syscall int renesas_ra_ctsu_group_configure(const struct device *dev,
					      const struct renesas_ra_ctsu_touch_cfg *cfg);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/input_renesas_ra_ctsu.h>

#endif /* ZEPHYR_INCLUDE_ZEPHYR_INPUT_INPUT_RENESAS_RA_CTSU_H_ */
