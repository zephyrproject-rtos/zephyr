/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_OTBR_WEB_SERVER) || defined(CONFIG_HDLC_RCP_IF_SPI_DEFERRED_INIT)
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(border_router_main, CONFIG_OTBR_LOG_LEVEL);
#endif

#ifdef CONFIG_OTBR_WEB_SERVER
#include "web_server/web_server.h"
#endif

int main(void)
{
#ifdef CONFIG_OTBR_WEB_SERVER
	/* Setup web server */
	LOG_INF("Zephyr OTBR Web Server setup ...");
	web_server_setup();
#endif

	/* Nothing to do here. The Border Router is automatically started in the background. */

	return 0;
}
