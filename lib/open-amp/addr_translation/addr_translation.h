/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <metal/io.h>

/**
 * @brief Return generic I/O operations
 *
 * @param	dev	Driver instance pointer
 * @return	metal_io_ops struct
 */
const struct metal_io_ops *metal_io_get_ops(void);
