/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_PRINTK_HOOKS_H_
#define ZEPHYR_INCLUDE_SYS_PRINTK_HOOKS_H_

/**
 * @brief printk function handler
 *
 * @param c Character to output
 *
 * @returns The character passed as input.
 */
typedef int (*printk_hook_fn_t)(int c);

/**
 * @brief Install the character output routine for printk
 *
 * To be called by the platform's console driver at init time. Installs a
 * routine that outputs one ASCII character at a time.
 * @param fn putc routine to install
 */
void __printk_hook_install(printk_hook_fn_t fn);

/**
 * @brief Get the current character output routine for printk
 *
 * To be called by any console driver that would like to save
 * current hook - if any - for later re-installation.
 *
 * @return a function pointer or NULL if no hook is set
 */
void *__printk_get_hook(void);

#endif /* ZEPHYR_INCLUDE_SYS_PRINTK_HOOKS_H_ */
