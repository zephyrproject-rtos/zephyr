/*
 * Copyright (c) 2026 Sentek Pty Ltd
 *
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>
#include "fcb_log.h"

#define MAX_FCB_LOG_ENTRY_SIZE 120


const struct log_backend *log_backend_fcb_get(void);