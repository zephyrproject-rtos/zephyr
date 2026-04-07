/**
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_KERNEL_TIMESLICING_H
#define ZEPHYR_INCLUDE_KERNEL_TIMESLICING_H

void z_time_slice(void);
void z_time_slice_reset(struct k_thread *curr);

#endif /* ZEPHYR_INCLUDE_KERNEL_TIMESLICING_H */
