/*
 * Copyright (c) 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/uuid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uuid_sample, LOG_LEVEL_INF);

int main(void)
{
	int result;
	struct uuid uuid_v4 = {0};
	struct uuid uuid_v5_namespace = {0};
	struct uuid uuid_v5 = {0};
	char uuid_v4_str[UUID_STR_LEN] = {0};
	char uuid_v5_str[UUID_STR_LEN] = {0};
	char uuid_v4_base64[UUID_BASE64_LEN] = {0};
	char uuid_v4_base64url[UUID_BASE64URL_LEN] = {0};

	/* Generate an UUID v4 from pseudo-random data */
	result = uuid_generate_v4(&uuid_v4);
	if (result != 0) {
		LOG_ERR("UUID v4 generation failed, error: %s (%d)", strerror(result), result);
		return -1;
	}
	/* Convert the UUID to string and to its base 64 and base 64 URL safe formats */
	result = uuid_to_string(&uuid_v4, uuid_v4_str);
	if (result != 0) {
		LOG_ERR("UUID v4 to string failed, error: %s (%d)", strerror(result), result);
		return -1;
	}
	LOG_INF("UUID v4: '%s'", uuid_v4_str);
	result = uuid_to_base64(&uuid_v4, uuid_v4_base64);
	if (result != 0) {
		LOG_ERR("UUID v4 to base 64 failed, error: %s (%d)", strerror(result), result);
		return -1;
	}
	LOG_INF("UUID v4 base 64: '%s'", uuid_v4_base64);
	result = uuid_to_base64url(&uuid_v4, uuid_v4_base64url);
	if (result != 0) {
		LOG_ERR("UUID v4 to base 64 URL safe failed, error: %s (%d)", strerror(result),
			result);
		return -1;
	}
	LOG_INF("UUID v4 base 64 URL safe: '%s'", uuid_v4_base64url);

	/* Generate an UUID v5 */
	/* This UUID is the same as in RFC 9562 Appendix A.4: "Example of a UUIDv5 Value" */
	result = uuid_from_string("6ba7b810-9dad-11d1-80b4-00c04fd430c8", &uuid_v5_namespace);
	if (result != 0) {
		LOG_ERR("Namespace string to UUID failed, error: %s (%d)", strerror(result),
			result);
		return -1;
	}
	result = uuid_generate_v5(&uuid_v5_namespace, "www.example.com", strlen("www.example.com"),
				  &uuid_v5);
	if (result != 0) {
		LOG_ERR("UUID v5 generation failed, error: %s (%d)", strerror(result), result);
		return -1;
	}
	result = uuid_to_string(&uuid_v5, uuid_v5_str);
	if (result != 0) {
		LOG_ERR("UUID v4 to string failed, error: %s (%d)", strerror(result), result);
		return -1;
	}
	LOG_INF("UUID v5: '%s'", uuid_v5_str);

	LOG_INF("UUID sample completed successfully");

	return 0;
}
