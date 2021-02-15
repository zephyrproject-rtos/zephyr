/*
 * Copyright (c) 2019 Antmicro Ltd
 * Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static void log_access(const struct mg_connection *conn)
{
	const struct mg_request_info *ri;
	char src_addr[IP_ADDR_STR_LEN];

	if (!conn || !conn->dom_ctx) {
		return;
	}

	ri = &conn->request_info;

	sockaddr_to_string(src_addr, sizeof(src_addr), &conn->client.rsa);

	LOG_DBG("%s - \"%s %s%s%s HTTP/%s\" %d\n",
		STR_LOG_ALLOC(src_addr),
		STR_LOG_ALLOC(ri->request_method),
		STR_LOG_ALLOC(ri->request_uri),
		(ri->query_string == NULL) ? log_strdup("?") : log_strdup(""),
		STR_LOG_ALLOC(ri->query_string),
		STR_LOG_ALLOC(ri->http_version),
		conn->status_code);
}
