/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OBJECT_APPLICATION_PROGRAM__
#define __OBJECT_APPLICATION_PROGRAM__

#include <stdint.h>
#include "object_property_types.h"

void object_application_program_properties_written(PropertyID id);

/**
 * Publish PID_PROGRAM_VERSION when this firmware declares an application.
 *
 * Must be called AFTER memory_read(), because it only fills the property in
 * when the persisted value is still all zeros — a value ETS has written wins.
 */
void object_application_program_init(void);

#endif
