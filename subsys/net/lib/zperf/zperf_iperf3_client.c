/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* iperf3 client. Uploads are not supported yet. */

#include <errno.h>

#include <zephyr/net/zperf.h>

#include "zperf_internal.h"

int zperf_iperf3_udp_upload(const struct zperf_upload_params *param, struct zperf_results *result)
{
	ARG_UNUSED(param);
	ARG_UNUSED(result);

	return -ENOTSUP;
}

int zperf_iperf3_tcp_upload(const struct zperf_upload_params *param, struct zperf_results *result)
{
	ARG_UNUSED(param);
	ARG_UNUSED(result);

	return -ENOTSUP;
}

int zperf_iperf3_udp_upload_async(const struct zperf_upload_params *param, zperf_callback callback,
			   void *user_data)
{
	ARG_UNUSED(param);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}

int zperf_iperf3_tcp_upload_async(const struct zperf_upload_params *param, zperf_callback callback,
			   void *user_data)
{
	ARG_UNUSED(param);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}

void zperf_udp_uploader_init(void)
{
}

void zperf_tcp_uploader_init(void)
{
}
