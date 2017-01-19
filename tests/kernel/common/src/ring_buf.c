/* test_ring_buf.c: Simple ring buffer test application */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <misc/ring_buffer.h>
#include <logging/sys_log.h>

SYS_RING_BUF_DECLARE_POW2(ring_buf, 8);

char data[] = "ABCDEFGHIJKLMNOPQRSTUVWX";
#define TYPE	1
#define VALUE	2

#define INITIAL_SIZE	2

void ring_buffer_test(void)
{
	int ret, put_count, i;
	uint32_t getdata[6];
	uint8_t getsize, getval;
	uint16_t gettype;
	int dsize = INITIAL_SIZE;

	put_count = 0;
	while (1) {
		ret = sys_ring_buf_put(&ring_buf, TYPE, VALUE,
				       (uint32_t *)data, dsize);
		if (ret == -EMSGSIZE) {
			SYS_LOG_DBG("ring buffer is full");
			break;
		}
		SYS_LOG_DBG("inserted %d chunks, %d remaining", dsize,
		       sys_ring_buf_space_get(&ring_buf));
		dsize = (dsize + 1) % SIZE32_OF(data);
		put_count++;
	}

	getsize = INITIAL_SIZE - 1;
	ret = sys_ring_buf_get(&ring_buf, &gettype, &getval, getdata, &getsize);
	if (ret != -EMSGSIZE) {
		SYS_LOG_DBG("Allowed retreival with insufficient destination buffer space");
		assert_true((getsize == INITIAL_SIZE), "Correct size wasn't reported back to the caller");
	}

	for (i = 0; i < put_count; i++) {
		getsize = SIZE32_OF(getdata);
		ret = sys_ring_buf_get(&ring_buf, &gettype, &getval, getdata,
				       &getsize);
		assert_true((ret == 0), "Couldn't retrieve a stored value");
		SYS_LOG_DBG("got %u chunks of type %u and val %u, %u remaining",
		       getsize, gettype, getval,
		       sys_ring_buf_space_get(&ring_buf));

		assert_true((memcmp((char *)getdata, data, getsize * sizeof(uint32_t)) == 0),
			     "data corrupted");
		assert_true((gettype == TYPE), "type information corrupted");
		assert_true((getval == VALUE), "value information corrupted");
	}

	getsize = SIZE32_OF(getdata);
	ret = sys_ring_buf_get(&ring_buf, &gettype, &getval, getdata,
			       &getsize);
	assert_true((ret == -EAGAIN), "Got data out of an empty buffer");
}
