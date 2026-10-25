/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_UNICAST_CLIENT_H_
#define MOCKS_BAP_UNICAST_CLIENT_H_

#include <zephyr/bluetooth/audio/bap.h>

/* The next bt_bap_unicast_client_disable() of stream fails with err */
void mock_bap_unicast_client_set_disable_err(const struct bt_bap_stream *stream, int err);

#endif /* MOCKS_BAP_UNICAST_CLIENT_H_ */
