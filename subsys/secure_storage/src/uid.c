/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/secure_storage/uid.h>

LOG_MODULE_REGISTER(secure_storage_common, CONFIG_SECURE_STORAGE_LOG_LEVEL);

#ifndef CONFIG_SECURE_STORAGE_64_BIT_UID
BUILD_ASSERT(sizeof(secure_storage_uid_t) == 4); /* ITS/PS UIDs are 32-bit */
BUILD_ASSERT(1 << SECURE_STORAGE_CALLER_ID_BIT_SIZE >= SECURE_STORAGE_CALLER_COUNT);
BUILD_ASSERT(SECURE_STORAGE_CALLER_ID_BIT_SIZE + SECURE_STORAGE_UID_BIT_SIZE == 32);
#endif

psa_status_t secure_storage_make_uid(secure_storage_caller_id_t caller_id,
				     psa_storage_uid_t uid,
				     secure_storage_uid_t *out_uid)
{
	if (uid == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

#ifndef CONFIG_SECURE_STORAGE_64_BIT_UID
	/* Check that the UID is not bigger than the maximum defined size. */
	if (uid & GENMASK64(63, SECURE_STORAGE_UID_BIT_SIZE)) {
		LOG_DBG("UID %u/%#llx cannot be used as it has bits set past "
			"the first " STRINGIFY(SECURE_STORAGE_UID_BIT_SIZE) " ones.",
			caller_id, (unsigned long long)uid);
		return PSA_ERROR_INVALID_ARGUMENT;
	}
#endif /* !CONFIG_SECURE_STORAGE_64_BIT_UID */

	*out_uid = (secure_storage_uid_t){.caller_id = caller_id, .uid = uid};
	return PSA_SUCCESS;
}
