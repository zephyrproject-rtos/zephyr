/*
 * Copyright (c) 2011 Petteri Aimonen
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include "src/simple.pb.h"

bool encode_message(uint8_t *buffer, size_t buffer_size, size_t *message_length)
{
	bool status;

	/* Allocate space on the stack to store the message data.
	 *
	 * Nanopb generates simple struct definitions for all the messages.
	 * - check out the contents of simple.pb.h!
	 * It is a good idea to always initialize your structures
	 * so that you do not have garbage data from RAM in there.
	 */
	SimpleMessage message = SimpleMessage_init_zero;

	/* Create a stream that will write to our buffer. */
	pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

	/* Fill in the lucky number */
	message.lucky_number = 13;
	for (int i = 0; i < 8; ++i) {
		message.buffer[i] = (uint8_t)(i * 2);
	}

	/* Now we are ready to encode the message! */
	status = pb_encode(&stream, SimpleMessage_fields, &message);
	*message_length = stream.bytes_written;

	if (!status) {
		printk("Encoding failed: %s\n", PB_GET_ERROR(&stream));
	}

	return status;
}

bool decode_message(uint8_t *buffer, size_t message_length)
{
	bool status;

	/* Allocate space for the decoded message. */
	SimpleMessage message = SimpleMessage_init_zero;

	/* Create a stream that reads from the buffer. */
	pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);

	/* Now we are ready to decode the message. */
	status = pb_decode(&stream, SimpleMessage_fields, &message);

	/* Check for errors... */
	if (status) {
		/* Print the data contained in the message. */
		printk("Your lucky number was %d!\n", (int)message.lucky_number);
		printk("Buffer contains: ");
		for (int i = 0; i < 8; ++i) {
			printk("%s%d", ((i == 0) ? "" : ", "), (int) message.buffer[i]);
		}
		printk("\n");
	} else {
		printk("Decoding failed: %s\n", PB_GET_ERROR(&stream));
	}

	return status;
}

int main(void)
{
	/* This is the buffer where we will store our message. */
	uint8_t buffer[SimpleMessage_size];
	size_t message_length;

	/* Encode our message */
	if (!encode_message(buffer, sizeof(buffer), &message_length)) {
		return 0;
	}

	/* Now we could transmit the message over network, store it in a file or
	 * wrap it to a pigeon's leg.
	 */

	/* But because we are lazy, we will just decode it immediately. */
	decode_message(buffer, message_length);
	return 0;
}
