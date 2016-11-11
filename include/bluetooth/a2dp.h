/** @file
 * @brief Advance Audio Distribution Profile header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_A2DP_H
#define __BT_A2DP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/avdtp.h>

/** @brief Stream Structure */
struct bt_a2dp_stream {
	/* TODO */
};

/** @brief Stream End Point */
struct bt_a2dp_endpoint {
	/** Stream End Point Information */
	struct bt_avdtp_seid_lsep info;
};

/** @brief A2DP structure */
struct bt_a2dp;

/** @brief A2DP Connect.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish A2DP
 *  connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return pointer to struct bt_a2dp in case of success or NULL in case
 *  of error.
 */
struct bt_a2dp *bt_a2dp_connect(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

#endif /* __BT_A2DP_H */
