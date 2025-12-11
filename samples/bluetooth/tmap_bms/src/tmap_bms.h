/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
/**
 * @brief Initialize the CAP Initiator role
 *
 * @return 0 if success, errno on failure.
 */
int cap_initiator_init(void);

/**
 * @brief Setup streams for CAP Initiator
 *
 * @return 0 if success, errno on failure.
 */
int cap_initiator_setup(void);
