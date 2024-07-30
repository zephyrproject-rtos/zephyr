/*
 * Copyright (c) 2023 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_tftp_client_app
#define LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/tftp.h>

#define APP_BANNER		"Run TFTP client"
#define TFTP_SAMPLE_DATA	"Lorem ipsum dolor sit amet, consectetur adipiscing elit"

static struct tftpc client;

static void tftp_event_callback(const struct tftp_evt *evt)
{
	switch (evt->type) {
	case TFTP_EVT_DATA:
		LOG_HEXDUMP_INF(evt->param.data.data_ptr,
				evt->param.data.len,
				"Received data: ");
		break;
	case TFTP_EVT_ERROR:
		LOG_ERR("Error code %d msg: %s",
			evt->param.error.code,
			evt->param.error.msg);
	default:
		break;
	}
}

static int tftp_init(const char *hostname)
{
	struct sockaddr remote_addr;
	struct addrinfo *res, hints = {0};
	int ret;

	/* Setup TFTP server address */
	hints.ai_socktype = SOCK_DGRAM;

	ret = getaddrinfo(hostname, CONFIG_TFTP_APP_PORT, &hints, &res);
	if (ret != 0) {
		LOG_ERR("Unable to resolve address");
		/* DNS error codes don't align with normal errors */
		return -ENOENT;
	}

	memcpy(&remote_addr, res->ai_addr, sizeof(remote_addr));
	freeaddrinfo(res);

	/* Save sockaddr into TFTP client handler */
	memcpy(&client.server, &remote_addr, sizeof(client.server));
	/* Register TFTP client callback */
	client.callback = tftp_event_callback;

	return 0;
}

int main(void)
{
	int ret;

	LOG_INF(APP_BANNER);

	ret = tftp_init(CONFIG_TFTP_APP_SERVER);
	if (ret < 0) {
		LOG_ERR("Unable to initialize TFTP client");
		return ret;
	}

	/* Get file1.bin in octet mode */
	ret = tftp_get(&client, "file1.bin", "octet");
	if (ret < 0) {
		LOG_ERR("Error while getting file (%d)", ret);
		return ret;
	}

	LOG_INF("TFTP client get done");

	/* Put TFTP sample data into newfile.bin to server in octet mode */
	ret = tftp_put(&client, "newfile.bin", "octet",
			TFTP_SAMPLE_DATA, sizeof(TFTP_SAMPLE_DATA));
	if (ret < 0 || ret != sizeof(TFTP_SAMPLE_DATA)) {
		LOG_ERR("Error while putting file (%d)", ret);
		return ret;
	}

	LOG_INF("TFTP client put done");

	return 0;
}
