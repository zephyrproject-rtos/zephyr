/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * Copyright (c) 2025 Abderrahmane JARMOUNI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_ZEPHYR_H_
#define ZEPHYR_MODULES_LVGL_ZEPHYR_H_

#include <zephyr/kernel.h>

#if DT_ZEPHYR_DISPLAYS_COUNT == 0
#error Could not find "zephyr,display" chosen property, or "zephyr,displays" compatible node in DT
#endif /* DT_ZEPHYR_DISPLAYS_COUNT == 0 */

#define LV_DISPLAY_IDX_MACRO(i, _) _##i

#define LV_DISPLAYS_IDX_LIST LISTIFY(DT_ZEPHYR_DISPLAYS_COUNT, LV_DISPLAY_IDX_MACRO, (,))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LittlevGL library
 *
 * This function initializes the LittlevGL library and setups the display and input devices.
 * If `LV_Z_AUTO_INIT` is disabled it must be called before any other LittlevGL function.
 *
 * @return 0 on success, negative errno code on failure
 */
int lvgl_init(void);

#if defined(CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE) || defined(__DOXYGEN__)
/**
 * @brief Gets the instance of LVGL's workqueue
 *
 * This function returns the workqueue instance used
 * by LVGL core, allowing user to submit their own
 * work items to it.
 *
 * @return pointer to LVGL's workqueue instance
 */
struct k_work_q *lvgl_get_workqueue(void);

#endif /* CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE */


#if (defined(CONFIG_LV_Z_LVGL_MUTEX) && !defined(CONFIG_LV_Z_USE_OSAL)) || defined(__DOXYGEN__)
/**
 * @brief Lock internal mutex before accessing LVGL API.
 *
 * @details LVGL API is not thread-safe. Therefore, before accessing any
 * API function, you need to lock the internal mutex, to prevent the
 * LVGL thread from pre-empting the API call.
 */
void lvgl_lock(void);

/**
 * @brief Try to lock internal mutex before accessing LVGL API.
 *
 * @details This function behaves like lvgl_lock(), provided that
 * the internal mutex is unlocked. Otherwise, it returns false without
 * waiting.
 */
bool lvgl_trylock(void);

/**
 * @brief Unlock internal mutex after accessing LVGL API.
 */
void lvgl_unlock(void);

#elif defined(CONFIG_LV_Z_LVGL_MUTEX) && defined(CONFIG_LV_Z_USE_OSAL)

#include <osal/lv_os.h>

static inline void lvgl_lock(void)
{
	lv_lock();
}

static inline bool lvgl_trylock(void)
{
	return lv_lock_isr() == LV_RESULT_OK;
}

static inline void lvgl_unlock(void)
{
	lv_unlock();
}

#else /* CONFIG_LV_Z_LVGL_MUTEX */

#define lvgl_lock(...) (void)0
#define lvgl_trylock(...) true
#define lvgl_unlock(...) (void)0

#endif /* CONFIG_LV_Z_LVGL_MUTEX */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_ZEPHYR_H_ */
