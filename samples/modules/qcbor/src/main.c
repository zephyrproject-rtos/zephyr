/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/printk.h>

#include <qcbor/qcbor.h>
#include <qcbor/qcbor_spiffy_decode.h>

struct message {
	int64_t v_int;
	int64_t v_12;
	UsefulBufC v_str;
	UsefulBufC v_13;
	double v_double;
};

static int encode_message(const struct message *msg, uint8_t *buffer, size_t buffer_size,
			  size_t *message_length)
{
	QCBOREncodeContext ctx;
	UsefulBuf bufc = { buffer, buffer_size };
	QCBORError qerr;

	QCBOREncode_Init(&ctx, bufc);

	QCBOREncode_OpenMap(&ctx);

	QCBOREncode_AddInt64ToMap(&ctx, "int", msg->v_int);
	QCBOREncode_AddInt64ToMapN(&ctx, 12, msg->v_12);
	QCBOREncode_AddTextToMap(&ctx, "str", msg->v_str);
	QCBOREncode_AddTextToMapN(&ctx, 13, msg->v_13);
	QCBOREncode_AddDoubleToMap(&ctx, "double", msg->v_double);

	QCBOREncode_CloseMap(&ctx);

	qerr = QCBOREncode_FinishGetSize(&ctx, message_length);
	if (qerr != QCBOR_SUCCESS) {
		/* TODO: better error mapping */
		return -EINVAL;
	}

	return 0;
}

static int decode_message(struct message *msg, uint8_t *buffer, size_t message_length)
{
	QCBORDecodeContext ctx;
	UsefulBufC bufc = { buffer, message_length };

	QCBORError qerr;

	QCBORDecode_Init(&ctx, bufc, QCBOR_DECODE_MODE_NORMAL);

	QCBORDecode_EnterMap(&ctx, NULL);

	QCBORDecode_GetInt64InMapSZ(&ctx, "int", &msg->v_int);
	QCBORDecode_GetInt64InMapN(&ctx, 12, &msg->v_12);
	QCBORDecode_GetTextStringInMapSZ(&ctx, "str", &msg->v_str);
	QCBORDecode_GetTextStringInMapN(&ctx, 13, &msg->v_13);
	QCBORDecode_GetDoubleInMapSZ(&ctx, "double", &msg->v_double);

	QCBORDecode_ExitMap(&ctx);

	qerr = QCBORDecode_GetError(&ctx);
	if (qerr != QCBOR_SUCCESS) {
		/* TODO: better error mapping */
		return -EINVAL;
	}

	return 0;
}

void main(void)
{
	const struct message msg_encode = {
		.v_int = 5,
		.v_12 = 6,
		.v_str = UsefulBuf_FROM_SZ_LITERAL("seven"),
		.v_13 = UsefulBuf_FROM_SZ_LITERAL("eight"),
		.v_double = 9.2,
	};
	struct message msg_decoded = {};
	/* This is the buffer where we will store our message. */
	uint8_t buffer[128];
	size_t message_length;
	int err;

	printk("Encoding message\n");

	err = encode_message(&msg_encode, buffer, sizeof(buffer), &message_length);
	if (err) {
		printk("Failed to encode\n");
		return;
	}

	printk("Decoding message\n");

	err = decode_message(&msg_decoded, buffer, message_length);
	if (err) {
		printk("Failed to decode\n");
		return;
	}

	printk("Comparing message\n");

	/* Compare encoded and decoded message */
	if (msg_encode.v_int != msg_decoded.v_int ||
	    msg_encode.v_12 != msg_decoded.v_12 ||
	    UsefulBuf_Compare(msg_encode.v_str, msg_decoded.v_str) != 0 ||
	    UsefulBuf_Compare(msg_encode.v_13, msg_decoded.v_13) != 0 ||
	    msg_encode.v_double != msg_decoded.v_double) {
		printk("Decoded message is not equal to encoded message\n");
		return;
	}

	printk("Decoded message is equal to encoded message\n");
}
