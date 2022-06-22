/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* when !XIP, .text is in RAM, and vector table must be at its very start */

KEEP(*(.exc_vector_table))
KEEP(*(".exc_vector_table.*"))
