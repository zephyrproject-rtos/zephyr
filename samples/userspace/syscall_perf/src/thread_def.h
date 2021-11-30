/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef THREAD_DEF_H
#define THREAD_DEF_H

void supervisor_thread_function(void *p1, void *p2, void *p3);

void user_thread_function(void *p1, void *p2, void *p3);

#endif /* THREAD_DEF_H */
