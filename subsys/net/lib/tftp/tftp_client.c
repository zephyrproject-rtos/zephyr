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

#define ADDRLEN(sa) \
	(sa.sa_family == AF_INET ? \
		sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))

/*
 * Prepare a request as required by RFC1350. This packet can be sent
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
 * Send Data message to the TFTP Server and receive ACK message from it.
 */
static int send_data(int sock, struct tftpc *client, uint32_t block_no, const uint8_t *data_buffer,
		     size_t data_size)
{
	int ret;
	int send_count = 0, ack_count = 0;
	struct zsock_pollfd fds = {
		.fd     = sock,
		.events = ZSOCK_POLLIN,
	};

	LOG_DBG("Client send data: block no %u, size %u", block_no, data_size + TFTP_HEADER_SIZE);

	do {
		if (send_count > TFTP_REQ_RETX) {
			LOG_ERR("No more retransmits. Exiting");
			return TFTPC_RETRIES_EXHAUSTED;
		}

		/* Prepare DATA packet, send it out then poll for ACK response */
		sys_put_be16(DATA_OPCODE, client->tftp_buf);
		sys_put_be16(block_no, client->tftp_buf + 2);
		memcpy(client->tftp_buf + TFTP_HEADER_SIZE, data_buffer, data_size);

		ret = zsock_send(sock, client->tftp_buf, data_size + TFTP_HEADER_SIZE, 0);
		if (ret < 0) {
			LOG_ERR("send() error: %d", -errno);
			return -errno;
		}

		do {
			if (ack_count > TFTP_REQ_RETX) {
				LOG_WRN("No more waiting for ACK");
				break;
			}

			ret = zsock_poll(&fds, 1, CONFIG_TFTPC_REQUEST_TIMEOUT);
			if (ret < 0) {
				LOG_ERR("recv() error: %d", -errno);
				return -errno;  /* IO error */
			} else if (ret == 0) {
				break;		/* no response, re-send data */
			}

			ret = zsock_recv(sock, client->tftp_buf, TFTPC_MAX_BUF_SIZE, 0);
			if (ret < 0) {
				LOG_ERR("recv() error: %d", -errno);
				return -errno;
			}

			if (ret != TFTP_HEADER_SIZE) {
				break; /* wrong response, re-send data */
			}

			uint16_t opcode = sys_get_be16(client->tftp_buf);
			uint16_t blockno = sys_get_be16(client->tftp_buf + 2);

			LOG_DBG("Receive: opcode %u, block no %u, size %d",
				opcode, blockno, ret);

			if (opcode == ACK_OPCODE && blockno == block_no) {
				return TFTPC_SUCCESS;
			} else if (opcode == ACK_OPCODE && blockno < block_no) {
				LOG_WRN("Server responded with obsolete block number.");
				ack_count++;
				continue; /* duplicated ACK */
			} else if (opcode == ERROR_OPCODE) {
				if (client->callback) {
					struct tftp_evt evt = {
						.type = TFTP_EVT_ERROR
					};

					evt.param.error.msg = client->tftp_buf + TFTP_HEADER_SIZE;
					evt.param.error.code = block_no;
					client->callback(&evt);
				}
				LOG_WRN("Server responded with obsolete block number.");
				break;
			} else {
				LOG_ERR("Server responded with invalid opcode or block number.");
				break; /* wrong response, re-send data */
			}
		} while (true);

		send_count++;
	} while (true);

	return TFTPC_REMOTE_ERROR;
}

/*
 * Send an Error Message to the TFTP Server.
 */
static inline int send_err(int sock, struct tftpc *client, int err_code, char *err_msg)
{
	uint32_t req_size;

	LOG_DBG("Client sending error code: %d", err_code);

	/* Fill in the "Err" Opcode and the actual error code. */
	sys_put_be16(ERROR_OPCODE, client->tftp_buf);
	sys_put_be16(err_code, client->tftp_buf + 2);
	req_size = 4;

	/* Copy the Error String. */
	if (err_msg != NULL) {
		size_t copy_len = strlen(err_msg);

		if (copy_len > sizeof(client->tftp_buf) - req_size) {
			copy_len = sizeof(client->tftp_buf) - req_size;
		}

		memcpy(client->tftp_buf + req_size, err_msg, copy_len);
		req_size += copy_len;
	}

	/* Send Error to server. */
	return zsock_send(sock, client->tftp_buf, req_size, 0);
}

/*
 * Send an Ack Message to the TFTP Server.
 */
static inline int send_ack(int sock, struct tftphdr_ack *ackhdr)
{
	LOG_DBG("Client acking block number: %d", ntohs(ackhdr->block));

	return zsock_send(sock, ackhdr, sizeof(struct tftphdr_ack), 0);
}

static int send_request(int sock, struct tftpc *client,
			int request, const char *remote_file, const char *mode)
{
	int tx_count = 0;
	size_t req_size;
	int ret;

	/* Create TFTP Request. */
	req_size = make_request(client->tftp_buf, request, remote_file, mode);

	do {
		tx_count++;

		LOG_DBG("Sending TFTP request %d file %s", request,
			remote_file);

		/* Send the request to the server */
		ret = zsock_sendto(sock, client->tftp_buf, req_size, 0, &client->server,
				   ADDRLEN(client->server));
		if (ret < 0) {
			break;
		}

		/* Poll for the response */
		struct zsock_pollfd fds = {
			.fd     = sock,
			.events = ZSOCK_POLLIN,
		};

		ret = zsock_poll(&fds, 1, CONFIG_TFTPC_REQUEST_TIMEOUT);
		if (ret <= 0) {
			LOG_DBG("Failed to get data from the TFTP Server"
				", req. no. %d", tx_count);
			continue;
		}

		/* Receive data from the TFTP Server. */
		struct sockaddr from_addr;
		socklen_t from_addr_len = sizeof(from_addr);

		ret = zsock_recvfrom(sock, client->tftp_buf, TFTPC_MAX_BUF_SIZE, 0,
				     &from_addr, &from_addr_len);
		if (ret < TFTP_HEADER_SIZE) {
			req_size = make_request(client->tftp_buf, request,
						remote_file, mode);
			continue;
		}

		/* Limit communication to the specific address:port */
		if (zsock_connect(sock, &from_addr, from_addr_len) < 0) {
			ret = -errno;
			LOG_ERR("connect failed, err %d", ret);
			break;
		}

		break;

	} while (tx_count <= TFTP_REQ_RETX);

	return ret;
}

int tftp_get(struct tftpc *client, const char *remote_file, const char *mode)
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

	if (client == NULL) {
		return -EINVAL;
	}

	sock = zsock_socket(client->server.sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket: %d", errno);
		return -errno;
	}

	/* Send out the READ request to the TFTP Server. */
	ret = send_request(sock, client, READ_REQUEST, remote_file, mode);
	rcv_size = ret;

	while (rcv_size >= TFTP_HEADER_SIZE && rcv_size <= TFTPC_MAX_BUF_SIZE) {
		/* Process server response. */
		uint16_t opcode = sys_get_be16(client->tftp_buf);
		uint16_t block_no = sys_get_be16(client->tftp_buf + 2);

		LOG_DBG("Received data: opcode %u, block no %u, size %d",
			opcode, block_no, rcv_size);

		if (opcode == ERROR_OPCODE) {
			if (client->callback) {
				struct tftp_evt evt = {
					.type = TFTP_EVT_ERROR
				};

				evt.param.error.msg = client->tftp_buf + TFTP_HEADER_SIZE;
				evt.param.error.code = block_no;
				client->callback(&evt);
			}
			ret = TFTPC_REMOTE_ERROR;
			break;
		} else if (opcode != DATA_OPCODE) {
			LOG_ERR("Server responded with invalid opcode.");
			ret = TFTPC_REMOTE_ERROR;
			break;
		}

		if (block_no == tftpc_block_no) {
			uint32_t data_size = rcv_size - TFTP_HEADER_SIZE;

			tftpc_block_no++;
			ackhdr.block = htons(block_no);
			tx_count = 0;

			if (client->callback == NULL) {
				LOG_ERR("No callback defined.");
				if (send_err(sock, client, TFTP_ERROR_DISK_FULL, NULL) < 0) {
					LOG_ERR("Failed to send error response, err: %d",
						-errno);
				}
				ret = TFTPC_BUFFER_OVERFLOW;
				goto get_end;
			}

			/* Send received data to client */
			struct tftp_evt evt = {
				.type = TFTP_EVT_DATA
			};

			evt.param.data.data_ptr = client->tftp_buf + TFTP_HEADER_SIZE;
			evt.param.data.len      = data_size;
			client->callback(&evt);

			/* Update the index. */
			tftpc_index += data_size;

			/* Per RFC1350, the end of a transfer is marked
			 * by datagram size < TFTPC_MAX_BUF_SIZE.
			 */
			if (rcv_size < TFTPC_MAX_BUF_SIZE) {
				(void)send_ack(sock, &ackhdr);
				ret = tftpc_index;
				LOG_DBG("%d bytes received.", tftpc_index);
				/* RFC1350: The host acknowledging the final DATA packet may
				 * terminate its side of the connection on sending the final ACK.
				 */
				break;
			}
		}

		/* Poll for the response */
		struct zsock_pollfd fds = {
			.fd     = sock,
			.events = ZSOCK_POLLIN,
		};

		do {
			if (tx_count > TFTP_REQ_RETX) {
				LOG_ERR("No more retransmits. Exiting");
				ret = TFTPC_RETRIES_EXHAUSTED;
				goto get_end;
			}

			/* Send ACK to the TFTP Server */
			(void)send_ack(sock, &ackhdr);
			tx_count++;
		} while (zsock_poll(&fds, 1, CONFIG_TFTPC_REQUEST_TIMEOUT) <= 0);

		/* Receive data from the TFTP Server. */
		ret = zsock_recv(sock, client->tftp_buf, TFTPC_MAX_BUF_SIZE, 0);
		rcv_size = ret;
	}

	if (!(rcv_size >= TFTP_HEADER_SIZE && rcv_size <= TFTPC_MAX_BUF_SIZE)) {
		ret = TFTPC_REMOTE_ERROR;
	}

get_end:
	zsock_close(sock);
	return ret;
}

int tftp_put(struct tftpc *client, const char *remote_file, const char *mode,
	     const uint8_t *user_buf, uint32_t user_buf_size)
{
	int sock;
	uint32_t tftpc_block_no = 1;
	uint32_t tftpc_index = 0;
	uint32_t send_size;
	uint8_t *send_buffer;
	int ret;

	if (client == NULL || user_buf == NULL || user_buf_size == 0) {
		return -EINVAL;
	}

	sock = zsock_socket(client->server.sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket: %d", errno);
		return -errno;
	}

	/* Send out the WRITE request to the TFTP Server. */
	ret = send_request(sock, client, WRITE_REQUEST, remote_file, mode);

	/* Check connection initiation result */
	if (ret >= TFTP_HEADER_SIZE) {
		uint16_t opcode = sys_get_be16(client->tftp_buf);
		uint16_t block_no = sys_get_be16(client->tftp_buf + 2);

		LOG_DBG("Receive: opcode %u, block no %u, size %d", opcode, block_no, ret);

		if (opcode == ERROR_OPCODE) {
			if (client->callback) {
				struct tftp_evt evt = {
					.type = TFTP_EVT_ERROR
				};

				evt.param.error.msg = client->tftp_buf + TFTP_HEADER_SIZE;
				evt.param.error.code = block_no;
				client->callback(&evt);
			}
			LOG_ERR("Server responded with service reject.");
			ret = TFTPC_REMOTE_ERROR;
			goto put_end;
		} else if (opcode != ACK_OPCODE || block_no != 0) {
			LOG_ERR("Server responded with invalid opcode or block number.");
			ret = TFTPC_REMOTE_ERROR;
			goto put_end;
		}
	} else {
		ret = TFTPC_REMOTE_ERROR;
		goto put_end;
	}

	/* Send out data by chunks */
	do {
		send_size = user_buf_size - tftpc_index;
		if (send_size > TFTP_BLOCK_SIZE) {
			send_size = TFTP_BLOCK_SIZE;
		}
		send_buffer = (uint8_t *)(user_buf + tftpc_index);

		ret = send_data(sock, client, tftpc_block_no, send_buffer, send_size);
		if (ret != TFTPC_SUCCESS) {
			goto put_end;
		} else {
			tftpc_index += send_size;
			tftpc_block_no++;
		}

		/* Per RFC1350, the end of a transfer is marked
		 * by datagram size < TFTPC_MAX_BUF_SIZE.
		 */
		if (send_size < TFTP_BLOCK_SIZE) {
			LOG_DBG("%d bytes sent.", tftpc_index);
			ret = tftpc_index;
			break;
		}
	} while (true);

put_end:
	zsock_close(sock);
	return ret;
}
