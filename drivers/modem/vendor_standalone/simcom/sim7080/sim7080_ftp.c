/*
 * Copyright (C) 2025 Metratec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_sim7080_ftp, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

/**
 * Parse the +FTPGET response.
 *
 * +FTPGET: <mode>,<len>
 *
 * Mode is hard set to 2.
 *
 * Length is the number of bytes following (the ftp data).
 */
MODEM_CMD_DEFINE(on_cmd_ftpget)
{
	int nbytes = atoi(argv[0]);
	int bytes_to_skip;
	size_t out_len;

	if (nbytes == 0) {
		mdata.ftp.nread = 0;
		return 0;
	}

	/* Skip length parameter and trailing \r\n */
	bytes_to_skip = strlen(argv[0]) + 2;

	/* Wait until data is ready.
	 * >= to ensure buffer is not empty after skip.
	 */
	if (net_buf_frags_len(data->rx_buf) <= nbytes + bytes_to_skip) {
		return -EAGAIN;
	}

	out_len = net_buf_linearize(mdata.ftp.read_buffer, mdata.ftp.nread, data->rx_buf,
					bytes_to_skip, nbytes);
	if (out_len != nbytes) {
		LOG_WRN("FTP read size differs!");
	}
	data->rx_buf = net_buf_skip(data->rx_buf, nbytes + bytes_to_skip);

	mdata.ftp.nread = nbytes;

	return 0;
}

int mdm_sim7080_ftp_get_read(char *dst, size_t *size)
{
	int ret;
	char buffer[sizeof("AT+FTPGET=#,######")];
	struct modem_cmd cmds[] = { MODEM_CMD("+FTPGET: 2,", on_cmd_ftpget, 1U, "") };

	/* Some error occurred. */
	if (mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_ERROR ||
		mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_INITIAL) {
		return SIM7080_FTP_RC_ERROR;
	}

	/* Setup buffer. */
	mdata.ftp.read_buffer = dst;
	mdata.ftp.nread = *size;

	/* Read ftp data. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGET=2,%zu", *size);
	if (ret < 0) {
		*size = 0;
		return SIM7080_FTP_RC_ERROR;
	}

	/* Wait for data from the server. */
	k_sem_take(&mdata.sem_ftp, K_MSEC(200));

	if (mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_FINISHED) {
		*size = 0;
		return SIM7080_FTP_RC_FINISHED;
	} else if (mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_ERROR) {
		*size = 0;
		return SIM7080_FTP_RC_ERROR;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), buffer,
				 &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		*size = 0;
		return SIM7080_FTP_RC_ERROR;
	}

	/* Set read size. */
	*size = mdata.ftp.nread;

	return SIM7080_FTP_RC_OK;
}

int mdm_sim7080_ftp_get_start(const char *server, const char *user, const char *passwd,
				  const char *file, const char *path)
{
	int ret;
	char buffer[256];

	/* Start network. */
	ret = mdm_sim7080_start_network();
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to start network for FTP!");
		return -1;
	}

	/* Set connection id for ftp. */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+FTPCID=0",
				 &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set FTP Cid!");
		return -1;
	}

	/* Set ftp server. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPSERV=\"%s\"", server);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
				 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set FTP Cid!");
		return -1;
	}

	/* Set ftp user. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPUN=\"%s\"", user);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
				 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp user!");
		return -1;
	}

	/* Set ftp password. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPPW=\"%s\"", passwd);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
				 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp password!");
		return -1;
	}

	/* Set ftp filename. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGETNAME=\"%s\"", file);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
				 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp filename!");
		return -1;
	}

	/* Set ftp filename. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGETNAME=\"%s\"", file);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
				 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp filename!");
		return -1;
	}

	/* Set ftp path. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGETPATH=\"%s\"", path);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
				 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp path!");
		return -1;
	}

	/* Initialize ftp variables. */
	mdata.ftp.read_buffer = NULL;
	mdata.ftp.nread = 0;
	mdata.ftp.state = SIM7080_FTP_CONNECTION_STATE_INITIAL;

	/* Start the ftp session. */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+FTPGET=1",
				 &mdata.sem_ftp, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to start session!");
		return -1;
	}

	if (mdata.ftp.state != SIM7080_FTP_CONNECTION_STATE_CONNECTED) {
		LOG_WRN("Session state is not connected!");
		return -1;
	}

	return 0;
}
