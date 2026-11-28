/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_CONTROL_INTERNAL_H_
#define ZEPHYR_NET_TAILSCALE_CONTROL_INTERNAL_H_

#include <psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONTROL_HOST           CONFIG_TAILSCALE_CONTROL_HOST
#define CONTROL_VERSION        131
#define CONTROL_VERSION_STRING "131"
#define CONTROL_KEY_SIZE       32
#define NOISE_RECORD_SIZE      4096
#define NOISE_PLAINTEXT_SIZE   (NOISE_RECORD_SIZE - 3 - 16)

struct tailscale_control_stream;

struct control_io {
#ifdef CONFIG_TAILSCALE_TRANSPORT
	int (*on_idle)(void);
	bool reading_noise_header;
#endif
	int fd;
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	struct proxy_tls *tls;
#endif
	int64_t deadline;
	const struct tailscale_control_stream *stream;
	uint8_t rx[1024];
	size_t rx_pos;
	size_t rx_len;
};

struct control_noise {
	struct control_io *io;
	mbedtls_svc_key_id_t tx_key;
	mbedtls_svc_key_id_t rx_key;
	uint64_t tx_counter;
	uint64_t rx_counter;
	uint8_t record[NOISE_RECORD_SIZE];
	uint8_t plaintext[NOISE_PLAINTEXT_SIZE];
	size_t rx_pos;
	size_t rx_len;
};

int control_io_socket_timeout(const struct control_io *io, int option);
int control_io_write(struct control_io *io, const void *data, size_t len);
int control_io_read(struct control_io *io, void *data, size_t len);
const char *control_http_field(const char *headers, const char *name);
int control_http_reply(struct control_io *io, char *header, size_t cap, int status);

int control_x25519_import(const uint8_t private_key[CONTROL_KEY_SIZE], mbedtls_svc_key_id_t *key);
int control_x25519_public(mbedtls_svc_key_id_t key, uint8_t public_key[CONTROL_KEY_SIZE]);
int control_x25519_agree(mbedtls_svc_key_id_t key, const uint8_t peer[CONTROL_KEY_SIZE],
			 uint8_t shared[CONTROL_KEY_SIZE]);
int control_noise_connect(struct control_noise *noise, mbedtls_svc_key_id_t machine,
			  const uint8_t server[CONTROL_KEY_SIZE]);
int control_noise_read(struct control_noise *noise, void *data, size_t len);
int control_noise_write(struct control_noise *noise, const void *data, size_t len);

struct control_h2 {
#ifdef CONFIG_TAILSCALE_TRANSPORT
	uint32_t status_stream;
	bool status_headers;
#endif
	struct control_noise *noise;
	uint8_t preface_header[9];
	bool has_preface_header;
	uint8_t frame[1024];
};

struct control_h2_body {
	uint32_t stream;
	uint32_t credit;
	size_t remaining;
	uint8_t padding;
	bool end_stream;
};

struct control_h2_map_request {
	const char *node_hex;
	const char *disco_hex;
	const char *hostname;
	const char *local_ipv4;
	const char *local_ipv6;
	const char *public_endpoint;
};

int control_h2_start(struct control_h2 *h2, char *early, size_t cap, size_t *early_len);
int control_h2_request(struct control_h2 *h2, uint32_t stream, const char *path,
		       const char *node_hex, const char *body, size_t len);
int control_h2_response(struct control_h2 *h2, uint32_t stream, char *body, size_t cap,
			size_t *body_len);
int control_h2_open_body(struct control_h2 *h2, uint32_t stream, struct control_h2_body *body);
int control_h2_read_body(struct control_h2 *h2, struct control_h2_body *body, const uint8_t **data,
			 size_t *len);
int control_h2_publish_endpoint(struct control_h2 *h2, uint32_t stream,
				const struct control_h2_map_request *request, uint16_t home,
				char *scratch, size_t capacity);
int control_h2_watch_open(struct control_h2 *h2, const struct control_h2_map_request *request,
			  struct control_h2_body *body, char *scratch, size_t capacity);

#endif /* ZEPHYR_NET_TAILSCALE_CONTROL_INTERNAL_H_ */
