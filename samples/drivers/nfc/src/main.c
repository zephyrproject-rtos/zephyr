/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/nfc/nfc_tag.h>

#ifdef CONFIG_NFC_NRFX
#define DEV_PTR device_get_binding(CONFIG_NFC_NRFX_DRV_NAME)
#else
#define NFC_DEV nt3h2x11
#define DEV_PTR DEVICE_DT_GET(DT_NODELABEL(NFC_DEV))
#endif

#define NDEF_MSG_BUF_SIZE  (128u)

#define FLAGS_MSG_BEGIN    (0x80u)
#define FLAGS_MSG_END      (0x40u)
#define FLAGS_CHUNK        (0x20u)
#define FLAGS_SHORT_RECORD (0x10u)
#define FLAGS_ID_LENGTH    (0x40u)

#define TNF_EMPTY          (0x00u)
#define TNF_WELLKNOWN      (0x01u)
#define TNF_MIME           (0x02u)
#define TNF_URI            (0x03u)
#define TNF_EXTERNAL       (0x04u)
#define TNF_UNKNOWN        (0x05u)
#define TNF_UNCHANGED      (0x06u)

#define TYPE_WELLKNOWN_TXT {'T'}
#define TYPE_WELLKNOWN_URI {'U'}
#define TYPE_WELLKNOWN_SP  {'S', 'p'}

#define TYPE_MIME_HTML {'t', 'e', 'x', 't', '/', 'h', 't', 'm', 'l'}
#define TYPE_MIME_JSON {'t', 'e', 'x', 't', '/', 'j', 's', 'o', 'n'}

/*
 * Text data of `Well Known : text` consists of:
 * - byte header
 * - language code
 * - text data
 *
 * The byte header is build up as:
 * bit 7   : 0 = UTF8, 1 = UTF16
 * bit 6   : reserved
 * bit 0-5 : length of language code
 */

static const uint8_t txt_utf8_english[] = {
	0x02,
	'E', 'N',
	'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'
};

static const uint8_t txt_utf8_norwegian[] = {
	0x02,
	'N', 'O',
	'H', 'a', 'l', 'l', 'o', ' ', 'V', 'e', 'r', 'd', 'e', 'n', '!'
};

static const uint8_t txt_utf8_polish[] = {
	0x02,
	'P', 'L',
	'V', 'i', 't', 'a', 'j', ' ', 0xc5u, 0x9au, 'w', 'i', 'e', 'c', 'i', 'e', '!'
};

/* Payload: 3x Hello World in different languages */
static const uint8_t txt_payload_cnt = 3u;

/* Buffer used to hold an NFC NDEF message. */
static uint8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];

/**
 * @brief Callback from NFC driver when an event is triggered.
 */
static void nfc_callback(const struct device *dev, enum nfc_tag_event event, const uint8_t *data,
			 size_t data_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);

	printk("NFC-CALLBACK EVENT: %d\n", event);
}

/**
 * @brief Function for encoding the NDEF text message.
 */
static int ndef_add(uint8_t *buf, uint8_t buf_len, uint8_t buf_idx, uint8_t ndef_idx,
		    uint8_t ndef_idx_max, uint8_t tnf, uint8_t *type, uint8_t type_len,
		    const uint8_t *record, uint32_t record_len)
{
	int rv = 0;

	if ((type_len + record_len + 6u) > (buf_len - buf_idx)) {
		printk("Not enough memory in buffer");
		rv = -ENOMEM;

	} else {
		uint8_t idx = buf_idx;

		/* Setup TNF + FLAGS */
		uint8_t hdr = tnf;

		if (ndef_idx == 0) {
			hdr |= FLAGS_MSG_BEGIN;
		}
		if (ndef_idx == ndef_idx_max) {
			hdr |= FLAGS_MSG_END;
		}

		/* Setup Record Header */
		buf[idx + 0] = hdr;
		buf[idx + 1] = type_len;
		buf[idx + 2] = 0xFFu & (record_len >> 24);
		buf[idx + 3] = 0xFFu & (record_len >> 16);
		buf[idx + 4] = 0xFFu & (record_len >> 8);
		buf[idx + 5] = 0xFFu & (record_len >> 0);
		idx += 6;

		/* Setup type */
		(void)memcpy(&buf[idx], type, type_len);
		idx += type_len;

		/* Setup payload data */
		(void)memcpy(&buf[idx], record, record_len);
		idx += record_len;

		rv = idx;
	}

	return rv;
}

int main(void)
{
	int rv;
	uint32_t len = sizeof(ndef_msg_buf);
	const struct device *dev = DEV_PTR;

	printk("Starting NFC Text Record example\n");

	if (dev == NULL) {
		printk("Could not get %s device\n", STRINGIFY(NFC_DEV));
		return -ENODEV;
	}

	/* Set up NFC driver*/
	rv = nfc_tag_init(dev, nfc_callback);
	if (rv != 0) {
		printk("Cannot setup NFC driver!\n");
	}

	/* Set up Tag mode */
	if (rv == 0) {
		rv = nfc_tag_set_type(dev, NFC_TAG_TYPE_T2T);
		if (rv != 0) {
			printk("Cannot setup NFC Tag mode! (%d)\n", rv);
		}
	}

	/* Setup welcome message */
	if (rv == 0) {
		/* Byte-index in msg-buffer at which record is inserted */
		uint8_t buf_idx = 0u;
		/* Type of record */
		uint8_t type_txt[] = TYPE_WELLKNOWN_TXT;

		/* Setup first record (at index 0)*/
		rv = ndef_add(ndef_msg_buf, sizeof(ndef_msg_buf), buf_idx, 0u, txt_payload_cnt - 1u,
			      TNF_WELLKNOWN, type_txt, sizeof(type_txt), txt_utf8_english,
			      sizeof(txt_utf8_english));

		if (rv < 0) {
			printk("Could not add english record");
			return -EINVAL;
		}

		/* Setup second record (at index 1) */
		buf_idx = rv;
		rv = ndef_add(ndef_msg_buf, sizeof(ndef_msg_buf), buf_idx, 1u, txt_payload_cnt - 1u,
			      TNF_WELLKNOWN, type_txt, sizeof(type_txt), txt_utf8_norwegian,
			      sizeof(txt_utf8_norwegian));

		if (rv < 0) {
			printk("Could not add norwegian record");
			return -EINVAL;
		}

		/* Setup third record (at index 2) */
		buf_idx = rv;
		rv = ndef_add(ndef_msg_buf, sizeof(ndef_msg_buf), buf_idx, 2u, txt_payload_cnt - 1u,
			      TNF_WELLKNOWN, type_txt, sizeof(type_txt), txt_utf8_polish,
			      sizeof(txt_utf8_polish));

		if (rv < 0) {
			printk("Could not add polish record");
			return -EINVAL;
		}

		/* Setting up records was a success */
		len = rv;
		rv = 0;
	}

	/* Set created message as the NFC payload */
	if (rv == 0) {
		rv = nfc_tag_set_ndef(dev, ndef_msg_buf, len);
		if (rv != 0) {
			printk("Cannot set payload! (%d)\n", rv);
		}
	}

	/* Start sensing NFC field */
	if (rv == 0) {
		rv = nfc_tag_start(dev);
		if (rv != 0) {
			printk("Cannot start emulation! (%d)\n", rv);
		}
	}

	printk("NFC configuration done (%d)\n", rv);

	return 0;
}
