/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(tftp_client, LOG_LEVEL_DBG);

/* Public / Internal TFTP header file. */
#include <net/tftp.h>
#include "tftp_client.h"

/* TFTP Global Buffer. */
static uint8_t   tftpc_buffer[TFTPC_MAX_BUF_SIZE];
static uint32_t  tftpc_buffer_size;

/* TFTP Request block number property. */
static uint32_t  tftpc_block_no;
static uint32_t  tftpc_index;

/* Global mutex to protect critical resources. */
K_MUTEX_DEFINE(tftpc_lock);

/* Name: make_request
 * Description: This function takes in a given list of parameters and
 * returns a read request packet. This packet can be sent
 * out directly to the TFTP server.
 */
static uint32_t make_request(const char *remote_file, const char *mode,
			  uint8_t request_type)
{
	uint32_t      req_size;
	const char def_mode[] = "octet";

	/* Populate the read request with the provided params. Note that this
	 * is created per RFC1350.
	 */

	/* Fill in the Request Type. Also keep tabs on the request size. */
	sys_put_be16(request_type, tftpc_buffer);
	req_size = 2;

	/* Copy in the name of the remote file - the file we need to get
	 * from the server. Add an upper bound to ensure no buffers overflow.
	 */
	strncpy(tftpc_buffer + req_size, remote_file, TFTP_MAX_FILENAME_SIZE);
	req_size += strlen(remote_file);

	/* Fill in 0. */
	tftpc_buffer[req_size] = 0x0;
	req_size++;

	/* Default to "Octet" if mode not specified. */
	if (mode == NULL) {
		mode = def_mode;
	}

	/* Copy the mode of operation. */
	strncpy(tftpc_buffer + req_size, mode, TFTP_MAX_MODE_SIZE);
	req_size += strlen(mode);

	/* Fill in 0. */
	tftpc_buffer[req_size] = 0x0;
	req_size++;

	return req_size;
}

/* Name: send_err
 * Description: This function sends an Error report to the TFTP Server.
 */
static inline int send_err(int sock, int err_code, char *err_string)
{
	uint32_t req_size;

	LOG_ERR("Client Error. Sending code: %d(%s)", err_code, err_string);

	/* Fill in the "Err" Opcode and the actual error code. */
	sys_put_be16(ERROR_OPCODE, tftpc_buffer);
	sys_put_be16(err_code, tftpc_buffer + 2);
	req_size = 4;

	/* Copy the Error String. */
	strcpy(tftpc_buffer + req_size, err_string);
	req_size += strlen(err_string);

	/* Send Error to server. */
	return send(sock, tftpc_buffer, req_size, 0);
}

/* Name: tftpc_recv_data
 * Description: This function tries to get data from the TFTP Server
 * (either response or data). Times out eventually.
 */
static int tftpc_recv_data(int sock)
{
	int           stat;
	struct pollfd fds;

	/* Setup FDS. */
	fds.fd     = sock;
	fds.events = ZSOCK_POLLIN;

	/* Enable poll on this socket. Wait for the specified number
	 * of milliseconds before exiting the call.
	 */
	stat = poll(&fds, 1, CONFIG_TFTPC_REQUEST_TIMEOUT);
	if (stat > 0) {
		/* Receive data from the TFTP Server. */
		stat = recv(sock, tftpc_buffer, TFTPC_MAX_BUF_SIZE, 0);

		/* Store the amount of data received in the global variable. */
		tftpc_buffer_size = stat;
	}

	return stat;
}

/* Name: tftpc_process_resp
 * Description: This function will process the data received from the
 * TFTP Server (a file or part of the file) and place it in the user buffer.
 * Return Value: This function will return one of the following values,
 * -> TFTPC_DUPLICATE_DATA: If the block is already received by the client.
 * -> TFTPC_BUFFER_OVERFLOW: If the data is more than the user provided buffer.
 * -> TFTPC_DATA_RX_SUCCESS: data received but their is more to come.
 * -> TFTPC_SUCCESS: Block received successfully and no more data is coming.
 */
static int tftpc_process_resp(int sock, struct tftpc *client)
{
	uint16_t    block_no;

	/* Get the block number as received in the packet. */
	block_no = sys_get_be16(tftpc_buffer + 2);
	if (tftpc_block_no > block_no) {
		LOG_DBG("Duplicate block received: %d", block_no);
		LOG_DBG("Client waiting for Block Number: %d", tftpc_block_no);

		send_ack(sock, block_no);

		/* Duplicate block received. */
		return TFTPC_DUPLICATE_DATA;
	}

	/* Valid block number received. */
	LOG_DBG("Block Number: %d received", tftpc_block_no);

	/* Only copy block if the user buffer has enough space. */
	if (RECV_DATA_SIZE() > (client->user_buf_size - tftpc_index)) {
		send_err(sock, 0x3, "Buffer Overflow");
		return TFTPC_BUFFER_OVERFLOW;
	}

	/* Perform the actual copy and update the index. */
	memcpy(client->user_buf + tftpc_index,
	       tftpc_buffer + TFTP_HEADER_SIZE, RECV_DATA_SIZE());
	tftpc_index += RECV_DATA_SIZE();

	/* "block" of data received. */
	send_ack(sock, block_no);
	tftpc_block_no++;

	/* For RFC1350, the block size will always be 512.
	 * -> If block_size == 512, the transfer is still in progress.
	 * -> If block_size < 512, we will conclude the transfer.
	 */
	return (RECV_DATA_SIZE() == TFTP_BLOCK_SIZE) ? TFTP_BLOCK_SIZE :
		TFTPC_SUCCESS;
}

/* Name: tftp_send_request
 * Description: This function sends out a request to the TFTP Server
 * (Read / Write) and waits for a response. Once we get some response
 * from the server, it is interpreted and ensured to be correct.
 * If not, we keep on poking the server for data until we eventually
 * give up.
 * Return Value: This function will return the "opcode" received from
 * the remote server, i.e.
 * -> ERROR_OPCODE: If the remote server responded with "Error" or if
 *    the client was unable to send / receive anything from the server.
 * -> DATA_OPCODE:  If the remote server responded with "Data".
 * -> ACK_OPCODE:   If the remote server responded with "Ack".
 */
static int tftp_send_request(int sock, uint8_t request,
			     const char *remote_file, const char *mode)
{
	uint8_t    retx_cnt = 0;
	uint32_t   req_size;

	/* Create TFTP Request. */
	req_size = make_request(remote_file, mode, request);

send_req:

	/* Send this request to the server. */
	if (send(sock, tftpc_buffer, req_size, 0)) {
		if (tftpc_recv_data(sock)) {
			return sys_get_be16(tftpc_buffer);
		}
	}

	/* No of times we had to re-tx this "request". */
	retx_cnt++;

	/* Log this out. */
	LOG_DBG("Unable to get data from the TFTP Server.");
	LOG_DBG("no_of_retransmists = %d", retx_cnt);

	/* Are we retransmitting? */
	if (retx_cnt < TFTP_REQ_RETX) {
		LOG_DBG("Are we re-transmitting: YES");
		goto send_req;
	}

	LOG_DBG("Are we re-transmitting: NO");
	return ERROR_OPCODE;
}

/* Name: tftp_get
 * Description: This function gets "file" from the remote server.
 */
int tftp_get(struct sockaddr *server, struct tftpc *client,
	     const char *remote_file, const char *mode)
{

	int32_t   stat     = TFTPC_UNKNOWN_FAILURE;
	uint8_t    retx_cnt = 0;
	int32_t   sock;

	/* Re-init the global "block number" variable. */
	tftpc_block_no = 1;
	tftpc_index    = 0;

	/* Create Socket for TFTP (IPv4 / UDP) */
	sock = socket(server->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket (IPv4): %d", errno);
		return -errno;
	}

	/* Connect with the address.  */
	stat = connect(sock, server, sizeof(struct sockaddr_in6));
	if (stat < 0) {
		LOG_ERR("Cannot connect to UDP remote (IPv4): %d", errno);
		return -errno;
	}

	/* Obtain Global Lock before accessing critical resources. */
	k_mutex_lock(&tftpc_lock, K_FOREVER);

	/* Send out the request to the TFTP Server. */
	stat = tftp_send_request(sock, READ_REQUEST, remote_file, mode);
	if (stat == ERROR_OPCODE) {
		LOG_ERR("Server responded with error.");
		return TFTPC_REMOTE_ERROR;
	}

process_resp:

	/* Process server response. */
	stat = tftpc_process_resp(sock, client);
	if (stat == TFTPC_BUFFER_OVERFLOW ||
	    stat == TFTPC_SUCCESS) {
		goto req_done;
	}

tftpc_recv:

	/* More data is available - Read (or we read a duplicate). */
	stat = tftpc_recv_data(sock);
	if (stat <= 0) {
		/* No response from server. */
		LOG_DBG("Server response timeout.");

		/* Retries exhausted ? */
		if (++retx_cnt >= TFTP_REQ_RETX) {
			LOG_ERR("No more retransmits available. Exiting");
			return TFTPC_RETRIES_EXHAUSTED;
		}

		/* Start Retransmission. */
		LOG_DBG("Starting Re-transmission.");
		send_ack(sock, tftpc_block_no);
		goto tftpc_recv;
	}

	/* Received response from the server. */
	LOG_DBG("Received data of size: %d", stat);
	goto process_resp;

req_done:
	/* Clean up. */
	k_mutex_unlock(&tftpc_lock);
	close(sock);
	return stat;
}
