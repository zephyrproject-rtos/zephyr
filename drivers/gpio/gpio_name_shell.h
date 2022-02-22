/*
 * Copyright (c) 2022 Google Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header for accessing GPIO name functions for shell.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_NAME_SHELL_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_NAME_SHELL_H_

/**
 * @brief Function to display the details of a single named GPIO.
 *
 * @param shell A pointer to the shell instance.
 * @param name A pointer to the name string identifying the GPIO.
 *
 * @return 0 on success, negative errno otherwise.
 */
int cmd_gpio_name_show(const struct shell *shell, const char *name);

/**
 * @brief Function to display all the named GPIOs.
 *
 * @param shell A pointer to the shell instance.
 */
void cmd_gpio_name_show_all(const struct shell *shell);

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_NAME_SHELL_H_ */
