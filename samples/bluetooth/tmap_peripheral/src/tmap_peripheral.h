/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>

#if defined(CONFIG_BT_ASCS_ASE_SNK)
#define AVAILABLE_SINK_CONTEXT                                                                     \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |                \
	 BT_AUDIO_CONTEXT_TYPE_MEDIA | BT_AUDIO_CONTEXT_TYPE_GAME |                                \
	 BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL)
#else
#define AVAILABLE_SINK_CONTEXT 0x0000
#endif /* CONFIG_BT_ASCS_ASE_SNK */

#if defined(CONFIG_BT_ASCS_ASE_SRC)
#define AVAILABLE_SOURCE_CONTEXT                                                                   \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |                \
	 BT_AUDIO_CONTEXT_TYPE_MEDIA | BT_AUDIO_CONTEXT_TYPE_GAME |                                \
	 BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL)
#else
#define AVAILABLE_SOURCE_CONTEXT 0x0000
#endif /* CONFIG_BT_ASCS_ASE_SRC */
/**
 * @brief Initialize the VCP Volume Renderer role
 *
 * @return 0 if success, errno on failure.
 */
int vcp_vol_renderer_init(void);

/**
 * @brief Initialize the CSIP Set Member role
 *
 * @return 0 if success, errno on failure.
 */
int csip_set_member_init(void);

/**
 * @brief Generate the Resolvable Set Identifier (RSI) value.
 *
 * @param rsi Pointer to place the 6-octet newly generated RSI data.
 *
 * @return 0 if on success, errno on error.
 */
int csip_generate_rsi(uint8_t *rsi);

/**
 * @brief Initialize BAP Unicast Server role
 *
 * @return 0 if success, errno on failure.
 */
int bap_unicast_sr_init(void);

/**
 * @brief Initialize Call Control Client
 *
 * @param conn Pointer to connection.
 *
 * @return 0 if success, errno on failure.
 */
int ccp_call_ctrl_init(struct bt_conn *conn);

/**
 * @brief Initiate a originate call command
 *
 * @return 0 if success, errno on failure.
 */
int ccp_originate_call(void);

/**
 * @brief Initiate a terminate call command
 *
 * @return 0 if success, errno on failure.
 */
int ccp_terminate_call(void);

/**
 * @brief Initialize Media Controller
 *
 * @param conn Pointer to connection.
 *
 * @return 0 if success, errno on failure.
 */
int mcp_ctlr_init(struct bt_conn *conn);

/**
 * @brief Send a command to the Media Player
 *
 * @param mcp_opcode Command opcode.
 *
 * @return 0 if on success, errno on error.
 */
int mcp_send_cmd(uint8_t mcp_opcode);
