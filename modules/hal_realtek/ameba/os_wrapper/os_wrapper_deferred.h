/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private to os_wrapper/.
 */

#ifndef ZEPHYR_MODULES_HAL_REALTEK_AMEBA_OS_WRAPPER_DEFERRED_H_
#define ZEPHYR_MODULES_HAL_REALTEK_AMEBA_OS_WRAPPER_DEFERRED_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*deferred_fn_t)(void *a1, void *a2, void *a3);

/*
 * Run fn(a1, a2, a3) once on sysworkq. Returns 0, -EINVAL (NULL fn), -ENOMEM
 * (pool exhausted) or -EAGAIN (submit rejected).
 */
int deferred_submit(deferred_fn_t fn, void *a1, void *a2, void *a3);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_HAL_REALTEK_AMEBA_OS_WRAPPER_DEFERRED_H_ */
