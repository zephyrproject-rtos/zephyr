/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

/**
 * @brief Initialize the VCP Volume Renderer role
 *
 * @return 0 if success, errno on failure.
 */
int vcp_vol_renderer_init(void);

/**
 * @brief Initialize BAP Broadcast Sink
 *
 * @return 0 if success, errno on failure.
 */
int bap_broadcast_sink_init(void);

/**
 * @brief Run BAP Broadcast Sink
 *
 * @return 0 if success, errno on failure.
 */
int bap_broadcast_sink_run(void);
