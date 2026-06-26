/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_AT_MONITOR_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_AT_MONITOR_H_

#include <zephyr/modem/chat.h>

#if defined(CONFIG_HL78XX_AT_MONITOR)
void hl78xx_at_monitor_dispatch(struct modem_chat *chat, char **argv, uint16_t argc);
#else
static inline void hl78xx_at_monitor_dispatch(struct modem_chat *chat, char **argv, uint16_t argc)
{
	(void)chat;
	(void)argv;
	(void)argc;
}
#endif /* CONFIG_HL78XX_AT_MONITOR */
#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_AT_MONITOR_H_ */
