/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP_SERVER_SERVICE_H_
#define HTTP_SERVER_SERVICE_H_

#include <zephyr/data/json.h>

#define POST_REQUEST_STORAGE_LIMIT CONFIG_NET_HTTP_SERVER_POST_REQUEST_STORAGE_LIMIT

enum tls_tag {
	/** The Certificate Authority public key */
	HTTP_SERVER_CA_CERTIFICATE_TAG,
	/** Used for both the public and private server keys */
	HTTP_SERVER_SERVER_CERTIFICATE_TAG,
	/** Used for both the public and private client keys */
	HTTP_SERVER_CLIENT_CERTIFICATE_TAG,
};

struct arithmetic_result {
	int x;
	int y;
	int result;
};

#endif
