/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Dummy header file to avoid compiler errors
 * intentionally left blank
 */

#ifndef KOBJECT_H
#define KOBJECT_H

#include <zephyr/ztest.h>

void k_object_access_grant(const void *object, struct k_thread *thread);

#endif /* KOBJECT_H */
