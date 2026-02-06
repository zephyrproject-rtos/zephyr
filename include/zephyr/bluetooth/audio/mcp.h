/**
 * @file
 * @brief Bluetooth Media Control Profile (MCP) APIs.
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCP_H_

/**
 * @brief Media Control Profile (MCP)
 *
 * @defgroup bt_mcp Media Control Profile (MCP)
 *
 * @since 4.3
 * @version 0.1.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Media Control Profile (MCP) provides procedures control media.
 * It provides the Media Control Client and the Media Control Server roles,
 * where the former is usually placed on resource constrained devices like headphones,
 * and the latter placed on more powerful devices like phones and PCs.
 */

#include <zephyr/bluetooth/audio/mcs.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup bt_mcp_media_control_server MCP Media Control Server APIs
 * @ingroup bt_mcp
 * @{
 */

/**
 * @brief Register the Media Control Server
 *
 * This will register the Generic Media Control Service (GMCS).
 *
 * @retval 0 Success
 * @retval -EBUSY Registration already in progress
 * @retval -EALREADY Already registered
 * @retval -ENOMEM Could not allocate any more content control services
 */
int bt_mcp_media_control_server_register(void);

/**
 * @brief Execute Command
 *
 * Execute a command in the Media Control Server.
 * Commands may cause the media player to change its state
 *
 * @param command     The command to execute
 *
 * @return 0 if success, errno on failure.
 */
int bt_mcp_media_control_server_command(const struct bt_mcs_cmd *command);

/** @} */ /* End of group bt_mcp_media_control_server */
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCP_H_ */
