/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_STAT_MGMT_
#define H_STAT_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for statistics management group.
 */
#define STAT_MGMT_ID_SHOW   0
#define STAT_MGMT_ID_LIST   1

/**
 * @brief Represents a single value in a statistics group.
 */
struct stat_mgmt_entry {
	const char *name;
	uint64_t value;
};

#ifdef __cplusplus
}
#endif

#endif /* H_STAT_MGMT_ */
