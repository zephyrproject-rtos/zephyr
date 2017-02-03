/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HTTP_UTILS_H__
#define __HTTP_UTILS_H__

#include <net/net_ip.h>

#define RC_STR(rc)	(rc == 0 ? "OK" : "ERROR")

void print_client_banner(const struct sockaddr *addr);

void print_server_banner(const struct sockaddr *addr);

#endif
