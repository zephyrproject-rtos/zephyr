/*
 * Copyright (c) 2020 Alexander Kozhinov Mail: <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HTTP_SERVER_HANDLERS__
#define __HTTP_SERVER_HANDLERS__

#include "civetweb.h"

void init_http_server_handlers(struct mg_context *ctx);

#endif  /* __HTTP_SERVER_HANDLERS__ */
