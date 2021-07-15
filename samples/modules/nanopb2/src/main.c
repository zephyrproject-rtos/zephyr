/*
 * Copyright (c) 2011 Petteri Aimonen
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include "src/person.pb.h"

bool encode_data(uint8_t *buffer, size_t buffer_size, size_t *data_length)
{
	bool status;
	char *name = "John Smith";
	char *email = "john.smith@mydomain.com";

	Person person = {};

	/* Create a stream that will write to our buffer. */
	pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

	if (strlen(name) + 1 > sizeof(person.name)) {
		printk("Too long name.\n");
		return false;
	}
	if (strlen(email) + 1 > sizeof(person.email)) {
		printk("Too long email adderss.\n");
		return false;
	}
	person.id = 12345;
	strcpy(person.name, name);
	strcpy(person.email, email);

	status = pb_encode_delimited(&stream, Person_fields, &person);
	*data_length = stream.bytes_written;
	printk("size of data : %zu\n", *data_length);

	/* Check for errors... */
	if (!status) {
		printk("Encoding failed: %s\n", PB_GET_ERROR(&stream));
	}

	return status;
}

bool decode_data(uint8_t *buffer, size_t data_length)
{
	bool status;

	Person person = {};

	pb_istream_t stream = pb_istream_from_buffer(buffer, data_length);

	status = pb_decode_delimited(&stream, Person_fields, &person);

	/* Check for errors... */
	if (status) {
		printk("Person information [ID:%d]\n", person.id);
		printk("\tname : %s\n", person.name);
		printk("\temail: %s\n", person.email);
	} else {
		printk("Decoding failed: %s\n", PB_GET_ERROR(&stream));
	}

	return status;
}

void main(void)
{
	uint8_t buffer[sizeof(Person)];
	size_t data_length;

	/* Encode person data */
	if (!encode_data(buffer, sizeof(buffer), &data_length)) {
		return;
	}

	/* Decode person data */
	decode_data(buffer, data_length);

	return;
}

