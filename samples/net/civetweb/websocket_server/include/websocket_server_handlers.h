/*
 * Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WEBSOCKET_SERVER_HANDLERS__
#define __WEBSOCKET_SERVER_HANDLERS__

#include "civetweb.h"

void init_websocket_server_handlers(struct mg_context *ctx);

#endif  /* __WEBSOCKET_SERVER_HANDLERS__ */
