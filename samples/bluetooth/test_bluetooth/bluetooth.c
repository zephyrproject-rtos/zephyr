/* bluetooth.c - Bluetooth smoke test */

/*
 * Copyright (c) 2015 Intel Corporation.
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
 * 3) Neither the name of Intel Corporation nor the names of its contributors
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

#include <errno.h>
#include <tc_util.h>

#include <nanokernel.h>
#include <arch/cpu.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/driver.h>

#define EXPECTED_ERROR -ENOSYS

static int driver_open(void)
{
	TC_PRINT("driver: %s\n", __func__);

	/* Indicate that there is no real Bluetooth device */
	return EXPECTED_ERROR;
}

static int driver_send(struct bt_buf *buf)
{
	return 0;
}

static struct bt_driver drv = {
	.head_reserve = 0,
	.open         = driver_open,
	.send         = driver_send,
};

static void driver_init(void)
{
	bt_driver_register(&drv);
}

#if defined(CONFIG_MICROKERNEL)
void mainloop(void)
#else
void main(void)
#endif
{
	int ret, ret_code;

	driver_init();

	ret = bt_enable(NULL);
	if (ret == EXPECTED_ERROR) {
		ret_code = TC_PASS;
	} else {
		ret_code = TC_FAIL;
	}

	TC_END(ret_code, "%s - %s.\n", ret_code == TC_PASS ? PASS : FAIL,
		   __func__);

	TC_END_REPORT(ret_code);
}
