/*
 * Copyright (c) 2024 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_VPX_ARC_VPX_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_VPX_ARC_VPX_H_

#include <zephyr/sys_clock.h>

/**
 * @brief Obtain a cooperative lock on the VPX vector registers
 *
 * This function is used to obtain a cooperative lock on the current CPU's
 * VPX vector registers before the calling thread uses them. Callers
 * attempting to obtain the cooperative lock must be already restricted to
 * executing on a single CPU, and continue to execute on that same CPU while
 * both waiting and holding the lock.
 *
 * This routine is not callable from an ISR.
 *
 * @param timeout  Waiting period to obtain the lock, or one of the special
 *                 values K_NO_WAIT and K_FOREVER.
 *
 * @return Zero on success, otherwise error code
 */
int arc_vpx_lock(k_timeout_t timeout);

/**
 * @brief Release cooperative lock on the VPX vector registers
 *
 * This function is used to release the cooperative lock on the current CPU's
 * VPX vector registers. It is called after the current thread no longer needs
 * to use the VPX vector registers, thereby allowing another thread to use them.
 *
 * This routine is not callable from an ISR.
 */
void arc_vpx_unlock(void);

/**
 * @brief Release cooperative lock on a CPU's VPX vector registers
 *
 * This function is used to release the cooperative lock on the specified CPU's
 * VPX vector registers. This routine should not be used except by a system
 * monitor to release the cooperative lock in case the locking thread where it
 * is known that the locking thread is unable to release it (e.g. it was
 * aborted while holding the lock).
 *
 * @param cpu_id CPU ID of the VPX vector register set to be unlocked
 */
void arc_vpx_unlock_force(unsigned int cpu_id);

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_VPX_ARC_VPX_H_ */
