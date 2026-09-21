/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The zperf upload and download API. The work is done by the client and the
 * server of the iperf protocol version zperf is built for.
 */

#include <errno.h>

#include <zephyr/net/zperf.h>

#include "zperf_internal.h"

#if defined(CONFIG_NET_UDP)

int zperf_udp_upload(const struct zperf_upload_params *param, struct zperf_results *result)
{
	if (param == NULL || result == NULL) {
		return -EINVAL;
	}

	return zperf_iperf2_udp_upload(param, result);
}

int zperf_udp_upload_async(const struct zperf_upload_params *param, zperf_callback callback,
			   void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	return zperf_iperf2_udp_upload_async(param, callback, user_data);
}

#if defined(CONFIG_NET_ZPERF_SERVER)

int zperf_udp_download(const struct zperf_download_params *param, zperf_callback callback,
		       void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	return zperf_iperf2_udp_download(param, callback, user_data);
}

int zperf_udp_download_stop(void)
{
	return zperf_iperf2_udp_download_stop();
}

#endif /* CONFIG_NET_ZPERF_SERVER */
#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)

int zperf_tcp_upload(const struct zperf_upload_params *param, struct zperf_results *result)
{
	if (param == NULL || result == NULL) {
		return -EINVAL;
	}

	return zperf_iperf2_tcp_upload(param, result);
}

int zperf_tcp_upload_async(const struct zperf_upload_params *param, zperf_callback callback,
			   void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	return zperf_iperf2_tcp_upload_async(param, callback, user_data);
}

#if defined(CONFIG_NET_ZPERF_SERVER)

int zperf_tcp_download(const struct zperf_download_params *param, zperf_callback callback,
		       void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	return zperf_iperf2_tcp_download(param, callback, user_data);
}

int zperf_tcp_download_stop(void)
{
	return zperf_iperf2_tcp_download_stop();
}

#endif /* CONFIG_NET_ZPERF_SERVER */
#endif /* CONFIG_NET_TCP */
