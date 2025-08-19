/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_H
#define COMMON_H

/* Shared data structures for request and response channels */
struct request_data {
	int request_id;
	int min_value;
	int max_value;
};

struct response_data {
	int response_id;
	int value;
};

#endif /* COMMON_H */
