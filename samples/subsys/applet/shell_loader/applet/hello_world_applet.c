/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Minimal applet extension usable while learning about the applet subsystem.
 *
 * The applet subsystem runs the exported "applet_main" symbol on a dedicated
 * thread, so no LLEXT, memory domain or userspace knowledge is needed here.
 */

extern void printk(const char *fmt, ...);

void applet_main(void *arg)
{
	printk("hello world from applet (arg=%p)\n", arg);
}
