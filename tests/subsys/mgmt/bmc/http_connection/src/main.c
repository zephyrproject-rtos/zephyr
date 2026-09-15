/*
 * Exercises the BMC over its own HTTP service, and at the same time proves
 * that an application can extend the BMC without patching it: everything the
 * "extensibility" tests look for is registered from this file.
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/host.h>
#include <zephyr/mgmt/bmc/redfish.h>
#include <zephyr/mgmt/bmc/sensor.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define BMC_SERVER_ADDR      "127.0.0.1"
#define BMC_SERVER_PORT      80
#define RESPONSE_BUFFER_SIZE 2048
#define BOOT_TIMEOUT_MS      5000

/* Base64 of "admin:admin", the credentials this test configures. */
#define BASIC_AUTH "Basic YWRtaW46YWRtaW4="

/*** Application supplied host control backend. ***/
static bool test_host_powered;
static unsigned int test_host_resets;

static int test_host_power_set(bool on)
{
	test_host_powered = on;

	return 0;
}

static int test_host_power_get(bool *on)
{
	*on = test_host_powered;

	return 0;
}

static int test_host_reset(void)
{
	test_host_resets++;

	return 0;
}

static const struct bmc_host_ops test_host_ops = {
	.power_set = test_host_power_set,
	.power_get = test_host_power_get,
	.reset = test_host_reset,
};

static int test_host_init(void)
{
	return bmc_host_ops_register(&test_host_ops);
}

BMC_COMPONENT_DEFINE(test_host, BMC_INIT_PHASE_APP, test_host_init, false);

/*** Application supplied sensor. ***/
static int test_sensor_read(const struct bmc_sensor *sensor, struct sensor_value *val)
{
	ARG_UNUSED(sensor);

	val->val1 = 42;
	val->val2 = 0;

	return 0;
}

BMC_SENSOR_DEFINE(test_sensor, .id = "TestSensor", .name = "Test Sensor",
		  .reading_type = "Temperature", .units = "Cel",
		  .physical_context = "ManagementController", .read = test_sensor_read);

/*** Application supplied Redfish resource. ***/
struct test_oem_resource {
	const char *odata_id;
	const char *id;
};

static const struct json_obj_descr test_oem_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct test_oem_resource, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct test_oem_resource, "Id", id, JSON_TOK_STRING),
};

static int test_oem_get(struct bmc_redfish_ctx *ctx)
{
	const struct test_oem_resource resource = {
		.odata_id = "/redfish/v1/Oem/Test",
		.id = "TestOem",
	};

	if (bmc_redfish_reply_encode(ctx, test_oem_descr, ARRAY_SIZE(test_oem_descr),
				     &resource) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(test_oem, "/redfish/v1/Oem/Test", false, test_oem_get, NULL, NULL);

/*** Test harness. ***/
static void *bmc_suite_setup(void)
{
	int64_t deadline;

	zassert_ok(bmc_init(), "bmc_init() failed");

	deadline = k_uptime_get() + BOOT_TIMEOUT_MS;
	while (!bmc_is_boot_finished() && k_uptime_get() < deadline) {
		k_msleep(10);
	}

	zassert_true(bmc_is_boot_finished(), "BMC did not finish booting");

	return NULL;
}

static int connect_to_bmc(void)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(BMC_SERVER_PORT),
	};
	int fd;
	int ret;

	ret = zsock_inet_pton(AF_INET, BMC_SERVER_ADDR, &addr.sin_addr);
	zassert_equal(ret, 1, "inet_pton() failed");

	fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_not_equal(fd, -1, "socket() failed (%d)", errno);

	ret = zsock_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	zassert_ok(ret, "connect() failed (%d)", errno);

	return fd;
}

/*
 * Dynamic responses arrive with chunked transfer encoding. Compact the body in
 * place so that the checks below see it as one string.
 */
static void dechunk(char *response)
{
	char *headers_end = strstr(response, "\r\n\r\n");
	char *out;
	char *in;

	if (headers_end == NULL || strstr(response, "Transfer-Encoding: chunked") == NULL) {
		return;
	}

	out = headers_end + 4;
	in = out;

	while (true) {
		unsigned long chunk_len;
		char *chunk;

		chunk_len = strtoul(in, &chunk, 16);
		if (chunk == in || strncmp(chunk, "\r\n", 2) != 0 || chunk_len == 0) {
			break;
		}

		chunk += 2;
		memmove(out, chunk, chunk_len);
		out += chunk_len;
		in = chunk + chunk_len + 2;
	}

	*out = '\0';
}

static void http_request(const char *method, const char *path, const char *auth,
			 const char *body, char *response, size_t response_size)
{
	char auth_header[128] = "";
	char request[256];
	size_t response_len = 0;
	int request_len;
	int fd;
	int ret;

	if (auth != NULL) {
		snprintk(auth_header, sizeof(auth_header), "Authorization: %s\r\n", auth);
	}

	request_len = snprintk(request, sizeof(request),
			       "%s %s HTTP/1.1\r\n"
			       "Host: " BMC_SERVER_ADDR "\r\n"
			       "%sContent-Type: application/json\r\n"
			       "Content-Length: %zu\r\n"
			       "Connection: close\r\n"
			       "\r\n"
			       "%s",
			       method, path, auth_header,
			       (body != NULL) ? strlen(body) : 0, (body != NULL) ? body : "");
	zassert_true(request_len > 0 && request_len < (int)sizeof(request),
		     "request does not fit the buffer");

	fd = connect_to_bmc();

	ret = zsock_send(fd, request, request_len, 0);
	zassert_equal(ret, request_len, "send() failed (%d)", errno);

	while (response_len < response_size - 1) {
		ret = zsock_recv(fd, response + response_len, response_size - response_len - 1,
				 0);
		zassert_true(ret >= 0, "recv() failed (%d)", errno);
		if (ret == 0) {
			break;
		}

		response_len += ret;
	}

	zsock_close(fd);

	zassert_true(response_len > 0, "empty response");
	response[response_len] = '\0';

	dechunk(response);
}

/*
 * The Redfish resources build their JSON by appending to a buffer, so make
 * sure the result is one well-formed object and nothing trails it.
 */
static void expect_single_json_object(const char *path, const char *response)
{
	const char *body = strstr(response, "\r\n\r\n");
	bool in_string = false;
	bool escaped = false;
	int depth = 0;

	zassert_not_null(body, "%s: response has no body", path);
	body += 4;

	for (const char *c = body; *c != '\0'; c++) {
		if (escaped) {
			escaped = false;
			continue;
		}

		if (in_string) {
			if (*c == '\\') {
				escaped = true;
			} else if (*c == '"') {
				in_string = false;
			}

			continue;
		}

		switch (*c) {
		case '"':
			in_string = true;
			break;
		case '{':
		case '[':
			depth++;
			break;
		case '}':
		case ']':
			depth--;
			break;
		default:
			zassert_false(depth == 0 && !isspace((unsigned char)*c),
				      "%s: trailing data after the JSON object: %s", path, c);
			break;
		}

		zassert_true(depth >= 0, "%s: unbalanced JSON body", path);
	}

	zassert_equal(depth, 0, "%s: unterminated JSON body", path);
}

static void expect_get_contains(const char *path, const char *auth, const char *expected_1,
				const char *expected_2)
{
	char response[RESPONSE_BUFFER_SIZE];

	http_request("GET", path, auth, NULL, response, sizeof(response));

	zassert_not_null(strstr(response, "HTTP/1.1 200"), "%s: missing 200 response", path);
	zassert_not_null(strstr(response, expected_1), "%s: missing \"%s\"", path, expected_1);
	zassert_not_null(strstr(response, expected_2), "%s: missing \"%s\"", path, expected_2);

	expect_single_json_object(path, response);
}

ZTEST(bmc_http_connection, test_service_root_needs_no_authentication)
{
	expect_get_contains("/redfish/v1", NULL, "application/json", "RootService");
}

ZTEST(bmc_http_connection, test_protected_resource_requires_authentication)
{
	char response[RESPONSE_BUFFER_SIZE];

	http_request("GET", "/redfish/v1/Systems/system", NULL, NULL, response, sizeof(response));
	zassert_not_null(strstr(response, "HTTP/1.1 401"), "expected an unauthorised response");

	expect_get_contains("/redfish/v1/Systems/system", BASIC_AUTH, "ComputerSystem", "system");
}

ZTEST(bmc_http_connection, test_identity_comes_from_kconfig_by_default)
{
	expect_get_contains("/redfish/v1/Systems/system", BASIC_AUTH,
			    CONFIG_BMC_REDFISH_MANUFACTURER, CONFIG_BMC_REDFISH_MODEL);
}

ZTEST(bmc_http_connection, test_registered_sensor_appears_in_the_collection)
{
	expect_get_contains("/redfish/v1/Chassis/1/Sensors", BASIC_AUTH, "SensorCollection",
			    "/redfish/v1/Chassis/1/Sensors/TestSensor");
}

ZTEST(bmc_http_connection, test_registered_sensor_is_readable)
{
	expect_get_contains("/redfish/v1/Chassis/1/Sensors/TestSensor", BASIC_AUTH, "Test Sensor",
			    "\"Reading\":42");
}

ZTEST(bmc_http_connection, test_unknown_sensor_is_not_found)
{
	char response[RESPONSE_BUFFER_SIZE];

	http_request("GET", "/redfish/v1/Chassis/1/Sensors/NoSuchSensor", BASIC_AUTH, NULL,
		     response, sizeof(response));
	zassert_not_null(strstr(response, "HTTP/1.1 404"), "expected a not-found response");
}

/*
 * A path below an existing resource must not be answered by that resource, no
 * matter in which order the resources ended up in the linker section.
 */
ZTEST(bmc_http_connection, test_unknown_resource_is_not_found)
{
	char response[RESPONSE_BUFFER_SIZE];

	http_request("GET", "/redfish/v1/Chassis/1/NoSuchThing", BASIC_AUTH, NULL, response,
		     sizeof(response));
	zassert_not_null(strstr(response, "HTTP/1.1 404"), "expected a not-found response");
}

ZTEST(bmc_http_connection, test_metadata_document_is_served)
{
	char response[RESPONSE_BUFFER_SIZE];

	http_request("GET", "/redfish/v1/$metadata", NULL, NULL, response, sizeof(response));
	zassert_not_null(strstr(response, "HTTP/1.1 200"), "expected the metadata document");
	zassert_not_null(strstr(response, "application/xml"), "expected an XML content type");
}

ZTEST(bmc_http_connection, test_application_resource_is_published)
{
	expect_get_contains("/redfish/v1/Oem/Test", NULL, "TestOem", "/redfish/v1/Oem/Test");
}

ZTEST(bmc_http_connection, test_reset_action_reaches_the_registered_host_ops)
{
	unsigned int resets_before = test_host_resets;
	char response[RESPONSE_BUFFER_SIZE];

	http_request("POST", "/redfish/v1/Systems/system/Actions/ComputerSystem.Reset",
		     BASIC_AUTH, "{\"ResetType\":\"On\"}", response, sizeof(response));
	zassert_not_null(strstr(response, "HTTP/1.1 204"), "expected a no-content response");
	zassert_true(test_host_powered, "the host was not powered on");

	http_request("POST", "/redfish/v1/Systems/system/Actions/ComputerSystem.Reset",
		     BASIC_AUTH, "{\"ResetType\":\"PowerCycle\"}", response, sizeof(response));
	zassert_not_null(strstr(response, "HTTP/1.1 204"), "expected a no-content response");
	zassert_equal(test_host_resets, resets_before + 1, "the host was not reset");
}

ZTEST(bmc_http_connection, test_power_state_is_reported_from_the_host_ops)
{
	zassert_ok(bmc_host_power_set(false), "could not power the host off");
	expect_get_contains("/redfish/v1/Chassis/1", BASIC_AUTH, "Chassis",
			    "\"PowerState\":\"Off\"");

	zassert_ok(bmc_host_power_set(true), "could not power the host on");
	expect_get_contains("/redfish/v1/Chassis/1", BASIC_AUTH, "Chassis",
			    "\"PowerState\":\"On\"");
}

ZTEST_SUITE(bmc_http_connection, NULL, bmc_suite_setup, NULL, NULL, NULL);
