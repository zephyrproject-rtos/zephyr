/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP_SERVER_SERVICE_H_
#define HTTP_SERVER_SERVICE_H_

#include <zephyr/data/json.h>

#define POST_REQUEST_STORAGE_LIMIT CONFIG_NET_HTTP_SERVER_POST_REQUEST_STORAGE_LIMIT

struct arithmetic_result {
	int x;
	int y;
	int result;
};

#endif
