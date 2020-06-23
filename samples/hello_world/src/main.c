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

static void dump_ords(const char *tag,
		      const device_handle_t *cp)
{
	const device_handle_t *cps = cp;
	unsigned int setnum = 0;
	char buf[128] = {0};
	char *bp = buf;

	printk("ordinal sets for %s:\n", tag);
	while (true) {
		if ((*cp == DEVICE_HANDLE_SEP)
		    || (*cp == DEVICE_HANDLE_ENDS)) {
			printk("\tS%u: %u elts:%s\n", setnum, (unsigned int)(cp - cps), buf);
			bp = buf;
			cps = cp + 1;
			++setnum;
			if (*cp == DEVICE_HANDLE_ENDS) {
				break;
			}
		} else {
			bp += snprintf(bp, buf + sizeof(buf) -  bp, " %d", *cp);
		}
		++cp;
	}
	printk("%s has %u sets\n", tag, setnum);
}

void main(void)
{
	struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);
	const struct device *dpe = devlist + devcnt;
	struct device *dp = devlist;

	if (false) {
		static const device_handle_t sets[] = {
			GEN_AUX()
			DEVICE_HANDLE_SEP,
			GEN_AUX(1)
			DEVICE_HANDLE_SEP,
			GEN_AUX(1, 2)
			DEVICE_HANDLE_ENDS,
		};
		dump_ords("test", sets);
	}

	printk("%zu devices at %p:\n", devcnt, devlist);
	while (dp < dpe) {
		const device_handle_t *cp;
		const char *name = dp->name ? dp->name : "<?>";
		size_t nc;

		if (false) {
			dump_ords(name, dp->ords);
		}

		cp = device_get_requires_ord(dp, &nc);
		printk("%p: %s\n", dp, name);
		if (cp != NULL) {
			printk("\tSelf %d, parent %d, %u req:",
			       device_get_self_ord(dp),
			       device_get_parent_ord(dp),
			       nc);

			const device_handle_t *cpe = cp + nc;
			while (cp < cpe) {
				printk(" %d", *cp++);
			}
			printk("\n");
		}
		++dp;
	}
}
