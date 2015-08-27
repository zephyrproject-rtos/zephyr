/* test_ring_buf.c: Simple ring buffer test application */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <tc_util.h>
#include <nanokernel.h>
#include <misc/ring_buffer.h>

SYS_RING_BUF_DECLARE_POW2(ring_buf, 8);

char data[] = "ABCDEFGHIJKLMNOPQRSTUVWX";
#define TYPE	1
#define VALUE	2

#define INITIAL_SIZE	2

void main(void)
{
	int ret, put_count, i, rv;
	uint32_t getdata[6];
	uint8_t getsize, getval;
	uint16_t gettype;
	int dsize = INITIAL_SIZE;

	TC_START("Test ring buffers");

	rv = TC_FAIL;
	put_count = 0;
	while (1) {
		ret = sys_ring_buf_put(&ring_buf, TYPE, VALUE,
				       (uint32_t *)data, dsize);
		if (ret == -EMSGSIZE) {
			printk("ring buffer is full\n");
			break;
		}
		printk("inserted %d chunks, %d remaining\n", dsize,
		       sys_ring_buf_space_get(&ring_buf));
		dsize = (dsize + 1) % SIZE32_OF(data);
		put_count++;
	}

	getsize = INITIAL_SIZE - 1;
	ret = sys_ring_buf_get(&ring_buf, &gettype, &getval, getdata, &getsize);
	if (ret != -EMSGSIZE) {
		printk("Allowed retreival with insufficient destination buffer space\n");
		if (getsize != INITIAL_SIZE)
			printk("Correct size wasn't reported back to the caller\n");
		goto done;
	}

	for (i = 0; i < put_count; i++) {
		getsize = SIZE32_OF(getdata);
		ret = sys_ring_buf_get(&ring_buf, &gettype, &getval, getdata,
				       &getsize);
		if (ret < 0) {
			printk("Couldn't retrieve a stored value (%d)\n",
			       ret);
			goto done;
		}
		printk("got %u chunks of type %u and val %u, %u remaining\n",
		       getsize, gettype, getval,
		       sys_ring_buf_space_get(&ring_buf));

		if (memcmp((char*)getdata, data, getsize * sizeof(uint32_t))) {
			printk("data corrupted\n");
			goto done;
		}
		if (gettype != TYPE) {
			printk("type information corrupted\n");
			goto done;
		}
		if (getval != VALUE) {
			printk("value information corrupted\n");
			goto done;
		}
	}

	getsize = SIZE32_OF(getdata);
	ret = sys_ring_buf_get(&ring_buf, &gettype, &getval, getdata,
			       &getsize);
	if (ret != -EAGAIN) {
		printk("Got data out of an empty buffer");
		goto done;
	}
	printk("empty buffer detected\n");

	rv = TC_PASS;
done:
	printk("head: %d tail: %d\n", ring_buf.head, ring_buf.tail);

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
