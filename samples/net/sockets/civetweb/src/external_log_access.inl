/*
 * Copyright (c) 2019 Antmicro Ltd
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

	printf("%s - \"%s %s%s%s HTTP/%s\" %d\n",
	       src_addr,
	       ri->request_method ? ri->request_method : "-",
	       ri->request_uri ? ri->request_uri : "-",
	       ri->query_string ? "?" : "",
	       ri->query_string ? ri->query_string : "",
	       ri->http_version,
	       conn->status_code);
}
