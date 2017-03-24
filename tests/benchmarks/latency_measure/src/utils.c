/* utils.c - utility functions used by latency measurement */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * this file contains commonly used functions
 */

#include <zephyr.h>

#include "timestamp.h"
#include "utils.h"

/* scratchpad for the string used to print on console */
char tmp_string[TMP_STRING_SIZE];

