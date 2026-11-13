/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/net/ocpp.h>
#include <zephyr/ztest.h>

#include "ocpp_i.h"

#define PASSWORD_MIN_LEN OCPP_BASIC_AUTH_PASSWORD_MIN_LEN
#define PASSWORD_MAX_LEN CONFIG_OCPP_BASIC_AUTH_PASSWORD_MAX_LEN
#define USERNAME_MAX_LEN CONFIG_OCPP_BASIC_AUTH_USERNAME_MAX_LEN

/* Example of OCPP-J 1.6, section 6.2.2: identity "AL1000" */
static const uint8_t ocpp_j_key[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

#define OCPP_J_HDR "Authorization: Basic QUwxMDAwOgABAgMEBQYH////////////////\r\n"

/* A key of printable characters */
static const char text_key[] = "zephyr-ocpp-sample-1";

#define TEXT_HDR "Authorization: Basic emVwaHlyOnplcGh5ci1vY3BwLXNhbXBsZS0x\r\n"

BUILD_ASSERT(sizeof(ocpp_j_key) <= PASSWORD_MAX_LEN, "the test keys have 20 bytes");

static char hdr[OCPP_AUTH_HDR_SIZE];

static int build(const char *username, const uint8_t *password, size_t password_len)
{
	const struct ocpp_basic_auth auth = {
		.username = username,
		.password = password,
		.password_len = password_len,
	};

	strcpy(hdr, "stale");

	return ocpp_basic_auth_hdr(&auth, hdr, sizeof(hdr));
}

static void expect_rejected(const char *username, const uint8_t *password, size_t password_len,
			    const char *what)
{
	zassert_equal(build(username, password, password_len), -EINVAL, "%s", what);
	zassert_str_equal(hdr, "", "header left after %s", what);
}

ZTEST(net_ocpp_basic_auth, test_no_credentials)
{
	zassert_ok(build(NULL, NULL, 0));
	zassert_str_equal(hdr, "", "header without a username");
}

ZTEST(net_ocpp_basic_auth, test_ocpp_j_example)
{
	/* the key is sent as it is, zero and control bytes included */
	zassert_ok(build("AL1000", ocpp_j_key, sizeof(ocpp_j_key)));
	zassert_str_equal(hdr, OCPP_J_HDR, "header %s", hdr);
}

ZTEST(net_ocpp_basic_auth, test_text_password)
{
	zassert_ok(build("zephyr", (const uint8_t *)text_key, strlen(text_key)));
	zassert_str_equal(hdr, TEXT_HDR, "header %s", hdr);
}

ZTEST(net_ocpp_basic_auth, test_buffer_size)
{
	const struct ocpp_basic_auth auth = {
		.username = "AL1000",
		.password = ocpp_j_key,
		.password_len = sizeof(ocpp_j_key),
	};
	char exact[sizeof(OCPP_J_HDR)];

	zassert_ok(ocpp_basic_auth_hdr(&auth, exact, sizeof(exact)));
	zassert_str_equal(exact, OCPP_J_HDR, "header %s", exact);

	/* one byte short of the header and its terminator */
	zassert_equal(ocpp_basic_auth_hdr(&auth, exact, sizeof(exact) - 1U), -ENOMEM);
	zassert_str_equal(exact, "", "header left in a short buffer");

	zassert_equal(ocpp_basic_auth_hdr(&auth, exact, 0), -ENOMEM);
}

ZTEST(net_ocpp_basic_auth, test_password_length)
{
	static const uint8_t key[PASSWORD_MAX_LEN + 1];

	/* at least 16 bytes: OCPP 1.6 security whitepaper, A00.FR.205 */
	expect_rejected("cp", key, 0, "empty password");
	expect_rejected("cp", key, PASSWORD_MIN_LEN - 1, "short password");
	expect_rejected("cp", key, PASSWORD_MAX_LEN + 1, "long password");
	expect_rejected("cp", NULL, PASSWORD_MIN_LEN, "password without a buffer");

	zassert_ok(build("cp", key, PASSWORD_MIN_LEN));
	zassert_ok(build("cp", key, PASSWORD_MAX_LEN));
}

ZTEST(net_ocpp_basic_auth, test_username)
{
	static const uint8_t key[PASSWORD_MAX_LEN];
	char user[USERNAME_MAX_LEN + 2];

	/* the longest credentials fill the header buffer of the library */
	(void)memset(user, 'u', sizeof(user));
	user[USERNAME_MAX_LEN] = '\0';
	zassert_ok(build(user, key, sizeof(key)));
	zassert_equal(strlen(hdr), sizeof(hdr) - 1U, "header of %zu characters", strlen(hdr));

	user[USERNAME_MAX_LEN] = 'u';
	user[USERNAME_MAX_LEN + 1] = '\0';
	expect_rejected(user, ocpp_j_key, sizeof(ocpp_j_key), "long username");

	expect_rejected("", ocpp_j_key, sizeof(ocpp_j_key), "empty username");

	/* a colon ends the username on the server side */
	expect_rejected("c:p", ocpp_j_key, sizeof(ocpp_j_key), "colon in the username");

	/* RFC 7617 section 2: no control characters in the user-id */
	expect_rejected("c\tp", ocpp_j_key, sizeof(ocpp_j_key), "tab in the username");
	expect_rejected("cp\n", ocpp_j_key, sizeof(ocpp_j_key), "line feed in the username");
	expect_rejected("cp\x7f", ocpp_j_key, sizeof(ocpp_j_key), "DEL in the username");

	/* an identity that is not ASCII is accepted */
	zassert_ok(build("s\xc3\xa4ule", ocpp_j_key, sizeof(ocpp_j_key)));
}

ZTEST_SUITE(net_ocpp_basic_auth, NULL, NULL, NULL, NULL, NULL);
