/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tftp_client, CONFIG_TFTP_LOG_LEVEL);

#include <stddef.h>
#include <zephyr/net/tftp.h>
#include "tftp_client.h"

#define ADDRLEN(sock) \
	(((struct sockaddr *)sock)->sa_family == AF_INET ? \
		sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))

/* TFTP Global Buffer. */
static uint8_t   tftpc_buffer[TFTPC_MAX_BUF_SIZE];

/* Global mutex to protect critical resources. */
K_MUTEX_DEFINE(tftpc_lock);

/*
 * Prepare the read request as required by RFC1350. This packet can be sent
 * out directly to the TFTP server.
 */
static size_t make_request(uint8_t *buf, int request,
			   const char *remote_file, const char *mode)
{
	char *ptr = (char *)buf;
	const char def_mode[] = "octet";

	/* Fill in the Request Type. */
	sys_put_be16(request, ptr);
	ptr += 2;

	/* Copy the name of the remote file. */
	strncpy(ptr, remote_file, TFTP_MAX_FILENAME_SIZE);
	ptr += strlen(remote_file);
	*ptr++ = '\0';

	/* Default to "Octet" if mode not specified. */
	if (mode == NULL) {
		mode = def_mode;
	}

	/* Copy the mode of operation. */
	strncpy(ptr, mode, TFTP_MAX_MODE_SIZE);
	ptr += strlen(mode);
	*ptr++ = '\0';

	return ptr - (char *)buf;
}

/*
 * Send an Error Message to the TFTP Server.
 */
static inline int send_err(int sock, int err_code, char *err_msg)
{
	uint32_t req_size;

	LOG_DBG("Client sending error code: %d", err_code);

	/* Fill in the "Err" Opcode and the actual error code. */
	sys_put_be16(ERROR_OPCODE, tftpc_buffer);
	sys_put_be16(err_code, tftpc_buffer + 2);
	req_size = 4;

	/* Copy the Error String. */
	if (err_msg != NULL) {
		strcpy(tftpc_buffer + req_size, err_msg);
		req_size += strlen(err_msg);
	}

	/* Send Error to server. */
	return send(sock, tftpc_buffer, req_size, 0);
}

/*
 * Send an Ack Message to the TFTP Server.
 */
static inline int send_ack(int sock, struct tftphdr_ack *ackhdr)
{
	LOG_DBG("Client acking block number: %d", ntohs(ackhdr->block));

	send(sock, ackhdr, sizeof(struct tftphdr_ack), 0);

	return 0;
}

static int tftp_send_request(int sock, struct sockaddr *server_addr,
		int request, const char *remote_file, const char *mode)
{
	int tx_count = 0;
	size_t req_size;
	int ret;

	/* Create TFTP Request. */
	req_size = make_request(tftpc_buffer, request, remote_file, mode);

	do {
		tx_count++;

		LOG_DBG("Sending TFTP request %d file %s", request,
			log_strdup(remote_file));

		/* Send the request to the server */
		ret = sendto(sock, tftpc_buffer, req_size, 0, server_addr,
			     ADDRLEN(server_addr));
		if (ret < 0) {
			break;
		}

		/* Poll for the response */
		struct pollfd fds = {
			.fd     = sock,
			.events = ZSOCK_POLLIN,
		};

		ret = poll(&fds, 1, CONFIG_TFTPC_REQUEST_TIMEOUT);
		if (ret <= 0) {
			LOG_DBG("Failed to get data from the TFTP Server"
				", req. no. %d", tx_count);
			continue;
		}

		/* Receive data from the TFTP Server. */
		struct sockaddr from_addr;
		socklen_t from_addr_len = sizeof(from_addr);

		ret = recvfrom(sock, tftpc_buffer, TFTPC_MAX_BUF_SIZE, 0,
				&from_addr, &from_addr_len);
		if (ret < TFTP_HEADER_SIZE) {
			req_size = make_request(tftpc_buffer, request,
						remote_file, mode);
			continue;
		}

		/* Limit communication to the specific address:port */
		connect(sock, &from_addr, from_addr_len);

		break;

	} while (tx_count <= TFTP_REQ_RETX);

	return ret;
}

int tftp_get(struct sockaddr *server_addr, struct tftpc *client,
	     const char *remote_file, const char *mode)
{
	int sock;
	uint32_t tftpc_block_no = 1;
	uint32_t tftpc_index = 0;
	int tx_count = 0;
	struct tftphdr_ack ackhdr = {
		.opcode = htons(ACK_OPCODE),
		.block = htons(1)
	};
	int rcv_size;
	int ret;

	sock = socket(server_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket (IPv4): %d", errno);
		return -errno;
	}

	/* Obtain Global Lock before accessing critical resources. */
	k_mutex_lock(&tftpc_lock, K_FOREVER);

	/* Send out the request to the TFTP Server. */
	ret = tftp_send_request(sock, server_addr, READ_REQUEST, remote_file, mode);
	rcv_size = ret;

	while (rcv_size >= TFTP_HEADER_SIZE && rcv_size <= TFTPC_MAX_BUF_SIZE) {
		/* Process server response. */

		uint16_t opcode = sys_get_be16(tftpc_buffer);
		uint16_t block_no = sys_get_be16(tftpc_buffer + 2);

		LOG_DBG("Received data: opcode %u, block no %u, size %d",
			opcode, block_no, rcv_size);

		if (opcode != DATA_OPCODE) {
			LOG_ERR("Server responded with invalid opcode.");
			ret = TFTPC_REMOTE_ERROR;
			break;
		}

		if (block_no == tftpc_block_no) {
			uint32_t data_size = rcv_size - TFTP_HEADER_SIZE;

			tftpc_block_no++;
			ackhdr.block = htons(block_no);
			tx_count = 0;

			/* Only copy block if user buffer has enough space */
			if (data_size > (client->user_buf_size - tftpc_index)) {
				LOG_ERR("User buffer is full.");
				send_err(sock, TFTP_ERROR_DISK_FULL, NULL);
				ret = TFTPC_BUFFER_OVERFLOW;
				break;
			}

			/* Perform the actual copy and update the index. */
			memcpy(client->user_buf + tftpc_index,
				tftpc_buffer + TFTP_HEADER_SIZE, data_size);
			tftpc_index += data_size;

			/* Per RFC1350, the end of a transfer is marked
			 * by datagram size < TFTPC_MAX_BUF_SIZE.
			 */
			if (rcv_size < TFTPC_MAX_BUF_SIZE) {
				ret = send_ack(sock, &ackhdr);
				client->user_buf_size = tftpc_index;
				break;
			}
		}

		/* Poll for the response */
		struct pollfd fds = {
			.fd     = sock,
			.events = ZSOCK_POLLIN,
		};

		do {
			if (tx_count > TFTP_REQ_RETX) {
				LOG_ERR("No more retransmits. Exiting");
				ret = TFTPC_RETRIES_EXHAUSTED;
				goto req_abort;
			}

			/* Send ACK to the TFTP Server */
			send_ack(sock, &ackhdr);
			tx_count++;

		} while (poll(&fds, 1, CONFIG_TFTPC_REQUEST_TIMEOUT) <= 0);

		/* Receive data from the TFTP Server. */
		ret = recv(sock, tftpc_buffer, TFTPC_MAX_BUF_SIZE, 0);
		rcv_size = ret;
	}

	if (!(rcv_size >= TFTP_HEADER_SIZE && rcv_size <= TFTPC_MAX_BUF_SIZE)) {
		ret = TFTPC_REMOTE_ERROR;
	}

req_abort:
	k_mutex_unlock(&tftpc_lock);
	close(sock);
	return ret;
}
