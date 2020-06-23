/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <stdio.h>

#define UART0 DT_NODELABEL(uart0)

#define GEN_AUX(...) FOR_EACH_NONEMPTY_TERM(IDENTITY, (,), __VA_ARGS__)

extern const uint8_t __device_handles_start[];
extern const uint8_t __device_handles_end[];

static void dump_handles(const char *tag,
			 const device_handle_t *cp)
{
	const device_handle_t *cps = cp;
	unsigned int setnum = 0;
	char buf[128] = {0};
	char *bp = buf;
	struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);

	printk("handles sets for %s: %p:\n", tag, cp);
	while (true) {
		if ((*cp == DEVICE_HANDLE_SEP)
		    || (*cp == DEVICE_HANDLE_ENDS)) {
			printk("\tS%u: %u elts:%s\n", setnum, (unsigned int)(cp - cps), buf);
			bp = buf;
			*bp = 0;
			cps = cp + 1;
			++setnum;
			if (*cp == DEVICE_HANDLE_ENDS) {
				break;
			}
		} else {
			bp += snprintf(bp, buf + sizeof(buf) -  bp, " %d", *cp);
			if (setnum == 0) {
				bp += snprintf(bp, buf + sizeof(buf) - bp, "[%s]",
					       devlist[*cp].name);
			}
		}
		++cp;
	}
	printk("%s has %u sets\n", tag, setnum);
}

static void showhandles(void)
{
	struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);
	const struct device *dpe = devlist + devcnt;
	struct device *dp = devlist;

	printk("%zu devices at %p:\n", devcnt, devlist);
	printk("%u bytes handles at %p\n",
	       __device_handles_end - __device_handles_start,
	       __device_handles_start);
	while (dp < dpe) {
		const char *name = dp->name ? dp->name : "<?>";

		printk("%u: %p: %s\n", (unsigned int)(dp - devlist), dp, name);
		if (dp->handles) {
			dump_handles(name, dp->handles);
		}
		++dp;
	}
}

void main(void)
{
	showhandles();
}
