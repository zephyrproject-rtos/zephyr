/*
 * Copyright (c) 2020 Alexander Kozhinov Mail: <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HELPER__
#define __HELPER__

#include <logging/log.h>

#define STR_LOG_ALLOC(str)	((str == NULL) ? log_strdup("null") :\
						  log_strdup(str))

#endif  /* __HELPER__ */
