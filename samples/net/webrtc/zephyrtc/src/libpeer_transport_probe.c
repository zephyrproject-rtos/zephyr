/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "libpeer_transport_probe.h"

#include <zephyr/sys/util.h>

#include "platform.h"

#if defined(CONFIG_ZEPHYRTC_TRANSPORT)
#include "dtls_srtp.h"

int peer_init(void);
void peer_deinit(void);
#endif

int libpeer_transport_probe(void)
{
#if defined(CONFIG_ZEPHYRTC_TRANSPORT)
	DtlsSrtp dtls_srtp = {0};
	int result;

	result = peer_init();
	if (result != 0) {
		platform_log("error", "transport_probe", "peer_init_failed=%d", result);
		return result;
	}
	result = dtls_srtp_init(&dtls_srtp, DTLS_SRTP_ROLE_SERVER, NULL);
	if (result != 0) {
		platform_log("error", "transport_probe", "dtls_init_failed=%d", result);
		peer_deinit();
		return result;
	}
	dtls_srtp_deinit(&dtls_srtp);
	peer_deinit();
	platform_log("info", "transport_probe", "status=pass");
#else
	platform_log("info", "transport_probe", "status=disabled");
#endif
	return 0;
}
