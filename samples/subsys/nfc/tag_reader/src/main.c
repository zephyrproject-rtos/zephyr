/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/nfc/ndef.h>
#include <zephyr/nfc/poller.h>
#include <zephyr/nfc/tag.h>

LOG_MODULE_REGISTER(nfc_tag_reader, LOG_LEVEL_INF);

#define NDEF_BUF_SIZE 256
#define MAX_RECORDS   8

NFC_POLLER_DEFINE(poller, DEVICE_DT_GET(DT_ALIAS(nfc0)));

static const char *tag_type_str(enum nfc_tag_type type)
{
	switch (type) {
	case NFC_TAG_TYPE_T2T:
		return "Type 2";
	case NFC_TAG_TYPE_T4T_A:
		return "Type 4A";
	case NFC_TAG_TYPE_T4T_B:
		return "Type 4B";
	default:
		return "unknown";
	}
}

static void print_records(const uint8_t *msg, uint16_t len)
{
	struct nfc_ndef_record records[MAX_RECORDS];
	size_t count = ARRAY_SIZE(records);
	int ret;

	ret = nfc_ndef_msg_parse(msg, len, records, &count);
	if (ret < 0) {
		LOG_ERR("cannot parse the NDEF message (%d)", ret);
		return;
	}

	for (size_t i = 0U; i < count; i++) {
		LOG_INF("  record %zu: TNF %u, %u byte payload", i, records[i].tnf,
			records[i].payload_len);
		LOG_HEXDUMP_INF(records[i].type, records[i].type_len, "  type");
		LOG_HEXDUMP_INF(records[i].payload, records[i].payload_len, "  payload");
	}
}

static void read_tag(const struct nfc_target *target)
{
	uint8_t msg[NDEF_BUF_SIZE];
	uint16_t len = sizeof(msg);
	struct nfc_tag tag;
	int ret;

	ret = nfc_tag_connect(&poller, target, &tag, K_SECONDS(2));
	if (ret < 0) {
		if (ret == -ENOTSUP) {
			LOG_INF("not an NDEF tag");
		} else {
			LOG_ERR("cannot connect to the tag (%d)", ret);
		}
		(void)nfc_target_release(&poller, target, K_MSEC(100));
		return;
	}

	LOG_INF("%s Tag", tag_type_str(tag.type));

	ret = nfc_tag_read_ndef(&tag, msg, &len, K_SECONDS(1));
	if (ret == -ENOENT) {
		LOG_INF("  the tag holds no NDEF message");
	} else if (ret < 0) {
		LOG_ERR("  cannot read the NDEF message (%d)", ret);
	} else {
		print_records(msg, len);
	}

	ret = nfc_tag_close(&tag, K_MSEC(100));
	if (ret < 0) {
		LOG_ERR("cannot release the tag (%d)", ret);
	}
}

int main(void)
{
	uint8_t seen[NFC_UID_MAXLEN];
	uint8_t seen_len = 0U;
	int ret;

	if (!device_is_ready(poller.dev)) {
		LOG_ERR("%s is not ready", poller.dev->name);
		return 0;
	}

	ret = nfc_poller_start(&poller, NFC_PROTO_ISO14443A);
	if (ret < 0) {
		LOG_ERR("cannot start the poller (%d)", ret);
		return 0;
	}

	LOG_INF("Present a tag to %s", poller.dev->name);

	while (true) {
		struct nfc_target target;
		const uint8_t *uid;
		uint8_t uid_len;

		ret = nfc_discover(&poller, K_MSEC(500), &target);
		if (ret == -EAGAIN) {
			/* Nothing in the field; the call already waited. */
			seen_len = 0U;
			continue;
		}
		if (ret < 0) {
			LOG_ERR("cannot poll for a tag (%d)", ret);
			k_sleep(K_SECONDS(1));
			continue;
		}

		uid = nfc_target_uid(&target, &uid_len);
		if (uid_len == seen_len && memcmp(uid, seen, uid_len) == 0) {
			continue;
		}
		memcpy(seen, uid, uid_len);
		seen_len = uid_len;

		LOG_HEXDUMP_INF(uid, uid_len, "UID");

		read_tag(&target);
	}

	return 0;
}
