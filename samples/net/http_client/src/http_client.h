/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include "http_client_types.h"
#include "http_client_rcv.h"

int http_init(struct http_client_ctx *http_ctx);

int http_reset_ctx(struct http_client_ctx *http_ctx);

#endif
