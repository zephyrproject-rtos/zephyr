/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void mesh_publish(void);
void mesh_set_name(const char *name, size_t len);

bool mesh_is_initialized(void);
void mesh_start(void);
int mesh_init(void);
