/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MESH_DEMO_H
#define MESH_DEMO_H

#include <stdbool.h>

/* Set once the mesh has started; the tx/rx demo tasks gate on it. */
extern bool mesh_running;

/* Toggled by the "mesh autotx" shell command; drives the periodic sender. */
extern bool mesh_autotx;

#endif /* MESH_DEMO_H */
