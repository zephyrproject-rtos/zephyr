/*
 * Copyright 2023 NXP
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/types.h>

/**
 * @brief Initialize the MCP Server role
 *
 * @return 0 if success, errno on failure.
 */
int mcp_server_init(void);

/**
 * @brief Initialize the CCP Call Control Server role
 *
 * @return 0 if success, errno on failure.
 */
int ccp_call_control_server_init(void);

/**
 * @brief Initialize the VCP Volume Controller role
 *
 * @return 0 if success, errno on failure.
 */
int vcp_vol_ctlr_init(void);

/**
 * @brief Send the Mute command to the VCP Volume Renderer
 *
 * @return 0 if success, errno on failure.
 */
int vcp_vol_ctlr_mute(void);

/**
 * @brief Send the Unmute command to the VCP Volume Renderer
 *
 * @return 0 if success, errno on failure.
 */
int vcp_vol_ctlr_unmute(void);

/**
 * @brief Set the volume for the VCP Volume Renderer
 *
 * @return 0 if success, errno on failure.
 */
int vcp_vol_ctlr_set_vol(uint8_t volume);

/**
 * @brief Initialize the CAP Initiator role
 *
 * @return 0 if success, errno on failure.
 */
int cap_initiator_init(void);

/**
 * @brief Setup streams for CAP Initiator
 *
 * @return 0 if success, errno on failure.
 */
int cap_initiator_setup(struct bt_conn *conn);
