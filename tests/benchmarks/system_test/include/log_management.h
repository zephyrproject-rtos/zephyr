/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LOG_MANAGEMENT_H
#define __LOG_MANAGEMENT_H

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <stdio.h>

extern FILE *output_file;

/* ANSI escape codes for colors */
#define RED(x)   "\x1b[31m" x "\x1b[0m"
#define GREEN(x) "\x1b[32m" x "\x1b[0m"

extern void log_init_output(void);
extern void log_print_data(char *title, uint8_t *data, uint8_t size);
extern void log_printDescription(void);

#endif
