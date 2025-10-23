/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hl78xx_chat.h
 *
 * Wrapper accessors for MODEM_CHAT_* objects that live in a dedicated
 * translation unit (hl78xx_chat.c). Other driver TUs should only call
 * these functions instead of taking addresses or using sizeof/ARRAY_SIZE
 * on the macro-generated objects.
 */
#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CHAT_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CHAT_H_

#include <stddef.h>
#include <zephyr/modem/chat.h>

/* Forward declare driver data type to keep this header lightweight and avoid
 * circular includes. The implementation file (hl78xx_chat.c) includes
 * hl78xx.h for full driver visibility.
 */
struct hl78xx_data;

/* Chat callback bridge used by driver TUs to receive script results. */
void hl78xx_chat_callback_handler(struct modem_chat *chat, enum modem_chat_script_result result,
				  void *user_data);

/* Wrapper helpers so other translation units don't need compile-time
 * visibility of the MODEM_CHAT_* macro-generated symbols.
 */
const struct modem_chat_match *hl78xx_get_ok_match(void);
const struct modem_chat_match *hl78xx_get_abort_matches(void);
const struct modem_chat_match *hl78xx_get_unsol_matches(void);
size_t hl78xx_get_unsol_matches_size(void);
size_t hl78xx_get_abort_matches_size(void);
const struct modem_chat_match *hl78xx_get_allow_match(void);
size_t hl78xx_get_allow_match_size(void);

/* Run predefined scripts from other units */
int hl78xx_run_init_script(struct hl78xx_data *data);
int hl78xx_run_periodic_script(struct hl78xx_data *data);
int hl78xx_run_post_restart_script(struct hl78xx_data *data);
int hl78xx_run_init_fail_script_async(struct hl78xx_data *data);
int hl78xx_run_enable_ksup_urc_script_async(struct hl78xx_data *data);
int hl78xx_run_pwroff_script_async(struct hl78xx_data *data);
int hl78xx_run_post_restart_script_async(struct hl78xx_data *data);
/* Async runners for init/periodic scripts */
int hl78xx_run_init_script_async(struct hl78xx_data *data);
int hl78xx_run_periodic_script_async(struct hl78xx_data *data);

/* Getter for ksrat match (moved into chat TU) */
const struct modem_chat_match *hl78xx_get_ksrat_match(void);

/* Socket-related chat matches used by the sockets TU */
const struct modem_chat_match *hl78xx_get_sockets_ok_match(void);
const struct modem_chat_match *hl78xx_get_connect_matches(void);
size_t hl78xx_get_connect_matches_size(void);
const struct modem_chat_match *hl78xx_get_sockets_allow_matches(void);
size_t hl78xx_get_sockets_allow_matches_size(void);
const struct modem_chat_match *hl78xx_get_kudpind_match(void);
const struct modem_chat_match *hl78xx_get_ktcpind_match(void);
const struct modem_chat_match *hl78xx_get_ktcpcfg_match(void);
const struct modem_chat_match *hl78xx_get_cgdcontrdp_match(void);
const struct modem_chat_match *hl78xx_get_ktcp_state_match(void);

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CHAT_H_ */
