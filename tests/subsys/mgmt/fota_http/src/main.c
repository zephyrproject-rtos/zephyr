/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/mgmt/fota_http.h>
#include <bootutil/bootutil_public.h>
#include <bootutil/image.h>
#include <psa/crypto.h>

#if defined(CONFIG_FOTA_HTTP_TLS)
#include <zephyr/net/tls_credentials.h>

#define CA_TAG     1
#define SERVER_TAG 2

static const unsigned char ca_cert[] = {
#include "ca.der.inc"
};

static const unsigned char server_cert[] = {
#include "server.der.inc"
};

static const unsigned char server_key[] = {
#include "server_privkey.der.inc"
};
#endif

#if defined(CONFIG_NET_IPV6)
#define SERVER_ADDR     "::1"
#define SERVER_URL_HOST "[::1]"
#define SERVER_FAMILY   NET_AF_INET6
#else
#define SERVER_ADDR     "127.0.0.1"
#define SERVER_URL_HOST SERVER_ADDR
#define SERVER_FAMILY   NET_AF_INET
#endif
#define SERVER_PORT_BASE  8080
#define SERVER_STACK_SIZE (IS_ENABLED(CONFIG_FOTA_HTTP_TLS) ? 16384 : 4096)

#define IMAGE_HEADER_SIZE  32
#define IMAGE_PAYLOAD_SIZE 3000
#define IMAGE_SIZE         (IMAGE_HEADER_SIZE + IMAGE_PAYLOAD_SIZE)
#define CHUNK_SIZE         700
#define TRAILER_PAGE_SIZE  4096

struct server_cfg {
	int status;
	const char *extra_headers;
	const uint8_t *body;
	size_t body_len;
	size_t send_len;
	size_t first_len;
	int delay_ms;
	bool chunked;
	bool honor_range;
	bool range_unsatisfiable;
	bool no_content_length;
	bool redirect_once;
	int connections;
};

static struct server_cfg cfg;
static char last_request[512];
static uint16_t server_port = SERVER_PORT_BASE;
static char url_buf[64];
static int requests_served;
static int server_err;
static bool server_running;

static K_THREAD_STACK_DEFINE(server_stack, SERVER_STACK_SIZE);
static struct k_thread server_thread;
static K_SEM_DEFINE(server_ready, 0, 1);

static uint8_t image[IMAGE_SIZE];
static uint8_t garbage[IMAGE_SIZE];
static uint8_t readback[IMAGE_SIZE];

static void build_image(uint8_t *buf, uint8_t major, uint8_t minor, uint16_t revision)
{
	struct image_header hdr = {
		.ih_magic = IMAGE_MAGIC,
		.ih_hdr_size = IMAGE_HEADER_SIZE,
		.ih_img_size = IMAGE_PAYLOAD_SIZE,
		.ih_ver.iv_major = major,
		.ih_ver.iv_minor = minor,
		.ih_ver.iv_revision = revision,
		.ih_ver.iv_build_num = 4,
	};

	memcpy(buf, &hdr, sizeof(hdr));
	for (size_t i = IMAGE_HEADER_SIZE; i < IMAGE_SIZE; i++) {
		buf[i] = (uint8_t)(i * 7 + 3);
	}
}

static int send_all(int sock, const void *data, size_t len)
{
	const uint8_t *p = data;

	while (len > 0) {
		ssize_t sent = zsock_send(sock, p, len, 0);

		if (sent < 0) {
			return -errno;
		}
		p += sent;
		len -= sent;
	}

	return 0;
}

static size_t parse_range(const char *request)
{
	const char *range = strstr(request, "Range: bytes=");

	if (range == NULL) {
		return 0;
	}

	return (size_t)strtoul(range + strlen("Range: bytes="), NULL, 10);
}

static void serve_connection(int client)
{
	char header[256];
	size_t offset = 0;
	size_t len;
	size_t to_send;
	const char *extra_headers = cfg.extra_headers;
	const uint8_t *body = cfg.body;
	size_t body_len = cfg.body_len;
	size_t send_len = cfg.send_len;
	int status = cfg.status;
	int n;

	if (cfg.redirect_once && requests_served > 0) {
		status = 200;
		extra_headers = NULL;
		body = image;
		body_len = IMAGE_SIZE;
		send_len = IMAGE_SIZE;
	}

	memset(last_request, 0, sizeof(last_request));
	len = 0;
	while (len < sizeof(last_request) - 1) {
		ssize_t r =
			zsock_recv(client, last_request + len, sizeof(last_request) - 1 - len, 0);

		if (r <= 0) {
			break;
		}
		len += r;
		if (strstr(last_request, "\r\n\r\n") != NULL) {
			break;
		}
	}

	if (cfg.range_unsatisfiable && strstr(last_request, "Range: bytes=") != NULL) {
		/* The file shrank below the requested offset. Answer once, so the
		 * restart that follows is served the whole file.
		 */
		cfg.range_unsatisfiable = false;
		status = 416;
		body_len = 0;
		send_len = 0;
	} else if (cfg.honor_range) {
		offset = parse_range(last_request);
		if (offset > 0) {
			status = 206;
		}
	}

	to_send = send_len - offset;

	n = snprintf(header, sizeof(header), "HTTP/1.1 %d %s\r\nConnection: close\r\n", status,
		     status == 200 ? "OK" : "Other");
	if (extra_headers != NULL) {
		n += snprintf(header + n, sizeof(header) - n, "%s", extra_headers);
	}
	if (status == 206) {
		n += snprintf(header + n, sizeof(header) - n,
			      "Content-Range: bytes %zu-%zu/%zu\r\n", offset, body_len - 1,
			      body_len);
	}
	if (cfg.chunked) {
		n += snprintf(header + n, sizeof(header) - n, "Transfer-Encoding: chunked\r\n");
	} else if (!cfg.no_content_length) {
		n += snprintf(header + n, sizeof(header) - n, "Content-Length: %zu\r\n",
			      body_len - offset);
	}
	n += snprintf(header + n, sizeof(header) - n, "\r\n");

	if (send_all(client, header, n) != 0) {
		return;
	}

	if (cfg.chunked) {
		size_t pos = 0;

		while (pos < to_send) {
			size_t chunk = MIN(CHUNK_SIZE, to_send - pos);
			char size_line[16];

			snprintf(size_line, sizeof(size_line), "%zx\r\n", chunk);
			if (send_all(client, size_line, strlen(size_line)) != 0 ||
			    send_all(client, body + offset + pos, chunk) != 0 ||
			    send_all(client, "\r\n", 2) != 0) {
				return;
			}
			pos += chunk;
		}
		(void)send_all(client, "0\r\n\r\n", 5);
		return;
	}

	if (cfg.first_len > 0 && cfg.first_len < to_send) {
		if (send_all(client, body + offset, cfg.first_len) != 0) {
			return;
		}
		k_msleep(cfg.delay_ms);
		(void)send_all(client, body + offset + cfg.first_len, to_send - cfg.first_len);
		return;
	}

	(void)send_all(client, body + offset, to_send);
}

static int open_server_socket(void)
{
#if defined(CONFIG_NET_IPV6)
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(server_port),
	};
#else
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(server_port),
	};
#endif
#if defined(CONFIG_FOTA_HTTP_TLS)
	sec_tag_t tags[] = {SERVER_TAG};
	int verify = ZSOCK_TLS_PEER_VERIFY_NONE;
#endif
	int one = 1;
	int sock;
	int ret = 0;

#if defined(CONFIG_NET_IPV6)
	zsock_inet_pton(NET_AF_INET6, SERVER_ADDR, &addr.sin6_addr);
#else
	zsock_inet_pton(NET_AF_INET, SERVER_ADDR, &addr.sin_addr);
#endif

#if defined(CONFIG_FOTA_HTTP_TLS)
	sock = zsock_socket(SERVER_FAMILY, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	if (sock < 0) {
		return -errno;
	}

	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, tags, sizeof(tags));
	if (ret == 0) {
		ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify,
				       sizeof(verify));
	}
#else
	sock = zsock_socket(SERVER_FAMILY, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (sock < 0) {
		return -errno;
	}
#endif

	if (ret == 0) {
		ret = zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &one,
				       sizeof(one));
	}
	if (ret == 0) {
		ret = zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr));
	}
	if (ret == 0) {
		ret = zsock_listen(sock, 2);
	}
	if (ret != 0) {
		ret = -errno;
		zsock_close(sock);
		return ret;
	}

	return sock;
}

static void server_fn(void *p1, void *p2, void *p3)
{
	struct zsock_timeval timeout = {.tv_sec = 2};
	int sock;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sock = open_server_socket();
	if (sock < 0) {
		server_err = sock;
		k_sem_give(&server_ready);
		return;
	}

	server_err = 0;
	zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout, sizeof(timeout));
	k_sem_give(&server_ready);

	while (requests_served < cfg.connections) {
		struct zsock_pollfd pfd = {
			.fd = sock,
			.events = ZSOCK_POLLIN,
		};
		int client;

		if (zsock_poll(&pfd, 1, 3000) <= 0) {
			break;
		}

		client = zsock_accept(sock, NULL, NULL);
		if (client < 0) {
			break;
		}

		zsock_setsockopt(client, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
				 sizeof(timeout));
		zsock_setsockopt(client, ZSOCK_SOL_SOCKET, ZSOCK_SO_SNDTIMEO, &timeout,
				 sizeof(timeout));
		serve_connection(client);
		zsock_close(client);
		requests_served++;
	}

	zsock_close(sock);
}

/* A cut connection lingers and blocks a new bind on its port, so each test gets its own. */
static void server_start(void)
{
	requests_served = 0;
	k_thread_create(&server_thread, server_stack, K_THREAD_STACK_SIZEOF(server_stack),
			server_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&server_ready, K_SECONDS(5)), "server did not start");
	zassert_ok(server_err, "server setup failed (%d)", server_err);
	server_running = true;
}

static void server_stop(void)
{
	if (server_running) {
		if (k_thread_join(&server_thread, K_SECONDS(5)) != 0) {
			k_thread_abort(&server_thread);
		}
		server_running = false;
	}
}

static void cfg_default(void)
{
	memset(&cfg, 0, sizeof(cfg));
	cfg.status = 200;
	cfg.body = image;
	cfg.body_len = IMAGE_SIZE;
	cfg.send_len = IMAGE_SIZE;
	cfg.connections = 1;
}

static void erase_slot0_header(void)
{
	const struct flash_area *fa;

	zassert_ok(flash_area_open(boot_fetch_active_slot(), &fa));
	zassert_ok(flash_area_flatten(fa, 0, 4096));
	flash_area_close(fa);
}

static void verify_slot_content(const uint8_t *expected, size_t len)
{
	const struct flash_area *fa;
	int slot = fota_http_upload_slot(0);

	zassert_true(slot >= 0, "no upload slot");
	zassert_ok(flash_area_open(slot, &fa));
	zassert_ok(flash_area_read(fa, 0, readback, len));
	flash_area_close(fa);
	zassert_mem_equal(readback, expected, len, "slot content differs");
}

static int erase_slot(uint8_t image_index)
{
	int slot = fota_http_upload_slot(image_index);

	if (slot < 0) {
		return slot;
	}

	return boot_erase_img_bank((uint8_t)slot);
}

static int download(const char *url, bool resume)
{
	struct fota_http_download_params params = {
		.url = url,
		.resume = resume,
#if defined(CONFIG_FOTA_HTTP_TLS)
		.tls_hostname = "localhost",
#endif
	};

	return fota_http_download(&params);
}

static const char *make_url(const char *path)
{
	snprintf(url_buf, sizeof(url_buf), "%s://%s:%u%s",
		 IS_ENABLED(CONFIG_FOTA_HTTP_TLS) ? "https" : "http", SERVER_URL_HOST, server_port,
		 path);

	return url_buf;
}

static const char *base_url(void)
{
	return make_url("/image.bin");
}

/* MCUboot trailer magic for the 8 byte write alignment of the flash simulator. */
static const uint8_t boot_magic[BOOT_MAGIC_SZ] = {
	0x77, 0xc2, 0x95, 0xf3, 0x60, 0xd2, 0xef, 0x7f,
	0x35, 0x52, 0x50, 0x0f, 0x2c, 0xb6, 0x79, 0x80,
};

/* The flash simulator keeps a trailer across runs, so erase it before planting magic. */
static int erase_slot0_trailer(void)
{
	const struct flash_area *fa;
	int ret;

	ret = flash_area_open(boot_fetch_active_slot(), &fa);
	if (ret != 0) {
		return ret;
	}

	ret = flash_area_flatten(fa, fa->fa_size - TRAILER_PAGE_SIZE, TRAILER_PAGE_SIZE);
	flash_area_close(fa);

	return ret;
}

static int plant_test_image_trailer(void)
{
	const struct flash_area *fa;
	int ret;

	ret = erase_slot0_trailer();
	if (ret != 0) {
		return ret;
	}

	ret = flash_area_open(boot_fetch_active_slot(), &fa);
	if (ret != 0) {
		return ret;
	}

	ret = flash_area_write(fa, fa->fa_size - BOOT_MAGIC_SZ, boot_magic, BOOT_MAGIC_SZ);
	flash_area_close(fa);

	return ret;
}

static void *suite_setup(void)
{
	build_image(image, 1, 2, 3);
	memset(garbage, 0x5a, sizeof(garbage));
	erase_slot0_header();

	zassert_ok(erase_slot0_trailer());
	zassert_false(fota_http_confirm_pending());

#if defined(CONFIG_FOTA_HTTP_TLS)
	zassert_ok(tls_credential_add(CA_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, ca_cert,
				      sizeof(ca_cert)));
	zassert_ok(tls_credential_add(SERVER_TAG, TLS_CREDENTIAL_PUBLIC_CERTIFICATE, server_cert,
				      sizeof(server_cert)));
	zassert_ok(tls_credential_add(SERVER_TAG, TLS_CREDENTIAL_PRIVATE_KEY, server_key,
				      sizeof(server_key)));
#endif

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	cfg_default();
	server_port++;
	(void)erase_slot(0);
}

static void test_after(void *fixture)
{
	ARG_UNUSED(fixture);

	server_stop();
}

ZTEST(fota_http, test_invalid_params)
{
	struct fota_http_download_params params = {.url = NULL};

	zassert_equal(fota_http_download(NULL), -EINVAL);
	zassert_equal(fota_http_download(&params), -EINVAL);
	zassert_equal(download("not a url", false), -EINVAL);
	zassert_equal(download("ftp://" SERVER_URL_HOST "/image.bin", false), -ENOTSUP);

	if (!IS_ENABLED(CONFIG_FOTA_HTTP_TLS)) {
		zassert_equal(download("https://" SERVER_URL_HOST "/image.bin", false), -ENOTSUP);
	}

	if (!IS_ENABLED(CONFIG_FOTA_HTTP_RESUME)) {
		zassert_equal(download(base_url(), true), -ENOTSUP);
	}

	zassert_equal(erase_slot(CONFIG_UPDATEABLE_IMAGE_NUMBER), -ENODEV);
	zassert_equal(fota_http_upload_slot(CONFIG_UPDATEABLE_IMAGE_NUMBER), -ENODEV);
}

ZTEST(fota_http, test_download_ok)
{
	struct mcuboot_img_header header;

	server_start();

	zassert_ok(download(base_url(), false));
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	zassert_not_null(strstr(last_request, "GET /image.bin HTTP/1.1"));
	verify_slot_content(image, IMAGE_SIZE);

	zassert_ok(fota_http_image_header(0, &header));
	zassert_equal(header.h.v1.sem_ver.major, 1);
	zassert_equal(header.h.v1.sem_ver.minor, 2);
	zassert_equal(header.h.v1.sem_ver.revision, 3);
	zassert_equal(header.h.v1.image_size, IMAGE_PAYLOAD_SIZE);

	zassert_ok(fota_http_apply(0, false));
	zassert_equal(mcuboot_swap_type(), BOOT_SWAP_TYPE_TEST);

	zassert_ok(erase_slot(0));
	zassert_equal(fota_http_image_header(0, &header), -EIO);
	zassert_equal(mcuboot_swap_type(), BOOT_SWAP_TYPE_NONE);
}

ZTEST(fota_http, test_apply_empty_slot)
{
	zassert_equal(fota_http_apply(0, false), -ENOEXEC);
	zassert_equal(fota_http_apply(0, true), -ENOEXEC);
	zassert_equal(mcuboot_swap_type(), BOOT_SWAP_TYPE_NONE);
}

ZTEST(fota_http, test_apply_permanent)
{
	server_start();

	zassert_ok(download(base_url(), false));
	zassert_ok(fota_http_apply(0, true));
	zassert_equal(mcuboot_swap_type(), BOOT_SWAP_TYPE_PERM);
	zassert_ok(erase_slot(0));
}

ZTEST(fota_http, test_extra_headers)
{
	static const char *const headers[] = {"Authorization: Bearer secret\r\n", NULL};
	struct fota_http_download_params params = {
		.url = base_url(),
		.headers = headers,
#if defined(CONFIG_FOTA_HTTP_TLS)
		.tls_hostname = "localhost",
#endif
	};

	server_start();

	zassert_ok(fota_http_download(&params));
	zassert_not_null(strstr(last_request, "Authorization: Bearer secret\r\n"));
}

ZTEST(fota_http, test_status_error)
{
	cfg.status = 404;
	server_start();

	zassert_equal(download(base_url(), false), -EBADMSG);
	zassert_equal(fota_http_bytes_written(), 0);
}

ZTEST(fota_http, test_empty_body)
{
	cfg.body_len = 0;
	cfg.send_len = 0;
	server_start();

	zassert_equal(download(base_url(), false), -ENODATA);
}

ZTEST(fota_http, test_truncated)
{
	cfg.send_len = 1500;
	server_start();

	zassert_equal(download(base_url(), false), -EMSGSIZE);
	zassert_true(fota_http_bytes_written() > 0);
	zassert_true(fota_http_bytes_written() <= 1500);
}

ZTEST(fota_http, test_not_an_image)
{
	cfg.body = garbage;
	server_start();

	zassert_equal(download(base_url(), false), -ENOEXEC);
}

ZTEST(fota_http, test_chunked)
{
	cfg.chunked = true;
	server_start();

	zassert_ok(download(base_url(), false));
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_no_content_length)
{
	cfg.no_content_length = true;
	server_start();

	zassert_ok(download(base_url(), false));
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_no_content_length_truncated)
{
	/* Without a Content-Length nothing announces the total, so only the
	 * image header tells the library how much should have arrived.
	 */
	cfg.no_content_length = true;
	cfg.send_len = IMAGE_SIZE - 64;
	server_start();

	zassert_equal(download(base_url(), false), -EMSGSIZE);
	zassert_true(fota_http_bytes_written() > 0);
	zassert_true(fota_http_bytes_written() < IMAGE_SIZE);
}

static K_SEM_DEFINE(async_done, 0, 1);
static int async_result;

static void async_done_cb(int result, void *user_data)
{
	ARG_UNUSED(user_data);

	async_result = result;
	k_sem_give(&async_done);
}

ZTEST(fota_http, test_redirect_absolute)
{
	char target[96];

	snprintf(target, sizeof(target), "Location: %s\r\n", make_url("/moved.bin"));
	cfg.status = 302;
	cfg.extra_headers = target;
	cfg.body_len = 0;
	cfg.send_len = 0;
	cfg.redirect_once = true;
	cfg.connections = 2;
	server_start();

	zassert_ok(download(base_url(), false));
	zassert_not_null(strstr(last_request, "GET /moved.bin HTTP/1.1"));
	zassert_equal(requests_served, 2);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_redirect_relative)
{
	cfg.status = 301;
	cfg.extra_headers = "Location: /relative.bin\r\n";
	cfg.body_len = 0;
	cfg.send_len = 0;
	cfg.redirect_once = true;
	cfg.connections = 2;
	server_start();

	zassert_ok(download(base_url(), false));
	zassert_not_null(strstr(last_request, "GET /relative.bin HTTP/1.1"));
}

ZTEST(fota_http, test_redirect_loop)
{
	cfg.status = 302;
	cfg.extra_headers = "Location: /again.bin\r\n";
	cfg.body_len = 0;
	cfg.send_len = 0;
	cfg.connections = CONFIG_FOTA_HTTP_MAX_REDIRECTS + 1;
	server_start();

	zassert_equal(download(base_url(), false), -ELOOP);
	zassert_equal(requests_served, CONFIG_FOTA_HTTP_MAX_REDIRECTS + 1);
}

ZTEST(fota_http, test_redirect_without_location)
{
	cfg.status = 302;
	cfg.body_len = 0;
	cfg.send_len = 0;
	server_start();

	zassert_equal(download(base_url(), false), -EBADMSG);
}

ZTEST(fota_http, test_cancel_and_busy)
{
	struct fota_http_download_params params = {
		.url = base_url(),
#if defined(CONFIG_FOTA_HTTP_TLS)
		.tls_hostname = "localhost",
#endif
	};

	cfg.first_len = 600;
	cfg.delay_ms = 1000;
	server_start();

	zassert_equal(fota_http_cancel(), -EALREADY);

	zassert_ok(fota_http_download_async(&params, async_done_cb));
	k_msleep(200);

	zassert_equal(fota_http_download(&params), -EBUSY);
	zassert_equal(fota_http_download_async(&params, async_done_cb), -EBUSY);
	zassert_ok(fota_http_cancel());

	zassert_ok(k_sem_take(&async_done, K_SECONDS(20)));
	zassert_equal(async_result, -ECANCELED);
}

ZTEST(fota_http, test_sha256)
{
	struct fota_http_download_params params = {
		.url = base_url(),
#if defined(CONFIG_FOTA_HTTP_TLS)
		.tls_hostname = "localhost",
#endif
	};
	uint8_t digest[32];
	size_t digest_len;

	zassert_equal(psa_crypto_init(), PSA_SUCCESS);
	zassert_equal(psa_hash_compute(PSA_ALG_SHA_256, image, IMAGE_SIZE, digest, sizeof(digest),
				       &digest_len),
		      PSA_SUCCESS);

	cfg.connections = 2;
	server_start();

	params.sha256 = digest;
	zassert_ok(fota_http_download(&params));

	digest[0] ^= 0xff;
	zassert_equal(fota_http_download(&params), -EILSEQ);
}

ZTEST(fota_http, test_reject_downgrade)
{
	const struct flash_area *fa;
	uint8_t running[IMAGE_HEADER_SIZE];

	build_image(garbage, 2, 0, 0);
	memcpy(running, garbage, sizeof(running));
	memset(garbage, 0x5a, sizeof(garbage));

	zassert_ok(flash_area_open(boot_fetch_active_slot(), &fa));
	zassert_ok(flash_area_write(fa, 0, running, sizeof(running)));
	flash_area_close(fa);

	cfg.connections = 2;
	server_start();

	zassert_equal(download(base_url(), false), -EPERM);

	erase_slot0_header();
	zassert_ok(download(base_url(), false));
}

ZTEST(fota_http, test_resume)
{
	char expected_range[32];
	size_t resumed_from;

	if (!IS_ENABLED(CONFIG_FOTA_HTTP_RESUME)) {
		ztest_test_skip();
	}

	cfg.send_len = 1700;
	cfg.honor_range = true;
	cfg.connections = 2;
	server_start();

	zassert_equal(download(base_url(), false), -EMSGSIZE);
	resumed_from = fota_http_bytes_written();
	zassert_true(resumed_from > 0);

	cfg.send_len = IMAGE_SIZE;
	zassert_ok(download(base_url(), true));
	snprintf(expected_range, sizeof(expected_range), "Range: bytes=%zu-\r\n", resumed_from);
	zassert_not_null(strstr(last_request, expected_range), "missing %s", expected_range);
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_resume_ignored_by_server)
{
	if (!IS_ENABLED(CONFIG_FOTA_HTTP_RESUME)) {
		ztest_test_skip();
	}

	cfg.send_len = 1700;
	cfg.connections = 2;
	server_start();

	zassert_equal(download(base_url(), false), -EMSGSIZE);

	cfg.send_len = IMAGE_SIZE;
	zassert_ok(download(base_url(), true));
	zassert_not_null(strstr(last_request, "Range: bytes="));
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_unconfirmed_rejected)
{
	struct fota_http_download_params params = {.url = base_url()};

	zassert_ok(plant_test_image_trailer());
	zassert_true(fota_http_confirm_pending());
	zassert_equal(fota_http_download(&params), -EPERM);
	zassert_equal(fota_http_download_async(&params, async_done_cb), -EPERM);
	zassert_ok(fota_http_confirm(0));
	zassert_false(fota_http_confirm_pending());
}

ZTEST(fota_http, test_resume_other_url)
{
	if (!IS_ENABLED(CONFIG_FOTA_HTTP_RESUME)) {
		ztest_test_skip();
	}

	cfg.send_len = 1700;
	cfg.honor_range = true;
	cfg.connections = 2;
	server_start();

	zassert_equal(download(base_url(), false), -EMSGSIZE);

	cfg.send_len = IMAGE_SIZE;
	zassert_ok(download(make_url("/other.bin"), true));
	zassert_is_null(strstr(last_request, "Range: bytes="));
	zassert_not_null(strstr(last_request, "GET /other.bin HTTP/1.1"));
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_resume_range_unsatisfiable)
{
	if (!IS_ENABLED(CONFIG_FOTA_HTTP_RESUME)) {
		ztest_test_skip();
	}

	cfg.send_len = 1700;
	cfg.honor_range = true;
	cfg.connections = 3;
	server_start();

	zassert_equal(download(base_url(), false), -EMSGSIZE);

	/* The file shrank, so the saved offset no longer exists and the
	 * library has to fetch the whole image again.
	 */
	cfg.send_len = IMAGE_SIZE;
	cfg.range_unsatisfiable = true;
	zassert_ok(download(base_url(), true));
	zassert_equal(requests_served, 3);
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_resume_if_range)
{
	if (!IS_ENABLED(CONFIG_FOTA_HTTP_RESUME)) {
		ztest_test_skip();
	}

	cfg.send_len = 1700;
	cfg.honor_range = true;
	cfg.extra_headers = "ETag: \"v1\"\r\n";
	cfg.connections = 2;
	server_start();

	zassert_equal(download(base_url(), false), -EMSGSIZE);

	cfg.send_len = IMAGE_SIZE;
	zassert_ok(download(base_url(), true));
	zassert_not_null(strstr(last_request, "If-Range: \"v1\"\r\n"));
	zassert_not_null(strstr(last_request, "Range: bytes="));
	zassert_equal(fota_http_bytes_written(), IMAGE_SIZE);
	verify_slot_content(image, IMAGE_SIZE);
}

ZTEST(fota_http, test_tls_add_credentials)
{
	if (!IS_ENABLED(CONFIG_FOTA_HTTP_TLS)) {
		ztest_test_skip();
	}

#if defined(CONFIG_FOTA_HTTP_TLS)
	/* The suite already registered the CA under the same tag, so this
	 * also covers a second registration being tolerated.
	 */
	zassert_ok(fota_http_tls_add_ca(ca_cert, sizeof(ca_cert)));
	zassert_ok(fota_http_tls_add_client_cert(server_cert, sizeof(server_cert), server_key,
						 sizeof(server_key)));
#endif
}

ZTEST(fota_http, test_tls_hostname_mismatch)
{
	struct fota_http_download_params params = {
		.url = base_url(),
		.tls_hostname = "wrong.example",
	};

	if (!IS_ENABLED(CONFIG_FOTA_HTTP_TLS)) {
		ztest_test_skip();
	}

	server_start();

	zassert_not_equal(fota_http_download(&params), 0);
	zassert_equal(fota_http_bytes_written(), 0);
}

ZTEST_SUITE(fota_http, NULL, suite_setup, test_before, test_after, NULL);
