/*
 * Copyright (c) 2026 Kent Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONN_META_H_
#define CONN_META_H_

#include <stdbool.h>
#include <stdint.h>

/* True if lll_conn_meta_init() ran on the connection, after its parent was set. */
bool conn_meta_inited(uint16_t handle);

#endif /* CONN_META_H_ */
