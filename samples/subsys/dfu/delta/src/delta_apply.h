/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DELTA_APPLY_H_
#define DELTA_APPLY_H_

/**
 * @brief Read the patch from patch_partition, apply it to the upload slot,
 *        mark the new image for upgrade, and reboot.
 *
 * @retval 0 on success (does not return, reboots on success).
 * @retval Negative errno on failure.
 */
int delta_apply_and_reboot(void);

#endif /* DELTA_APPLY_H_ */
