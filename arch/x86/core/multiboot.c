/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/x86/multiboot.h>

#ifdef CONFIG_X86_MULTIBOOT_INFO

struct x86_multiboot_info x86_multiboot_info;

#ifdef CONFIG_X86_MULTIBOOT_FRAMEBUF

#include <display/framebuf.h>

static struct framebuf_dev_data multiboot_framebuf_data = {
	.width = CONFIG_X86_MULTIBOOT_FRAMEBUF_X,
	.height = CONFIG_X86_MULTIBOOT_FRAMEBUF_Y
};

static int multiboot_framebuf_init(struct device *dev)
{
	struct framebuf_dev_data *data = FRAMEBUF_DATA(dev);
	struct x86_multiboot_info *info = &x86_multiboot_info;

	if ((info->flags & X86_MULTIBOOT_INFO_FLAGS_FB) &&
	    (info->fb_width >= CONFIG_X86_MULTIBOOT_FRAMEBUF_X) &&
	    (info->fb_height >= CONFIG_X86_MULTIBOOT_FRAMEBUF_Y) &&
	    (info->fb_bpp == 32) && (info->fb_addr_hi == 0)) {
		/*
		 * We have a usable multiboot framebuffer - it is 32 bpp
		 * and at least as large as the requested dimensions. Compute
		 * the pitch and adjust the start address center our canvas.
		 */

		u16_t adj_x;
		u16_t adj_y;
		u32_t *buffer;

		adj_x = info->fb_width - CONFIG_X86_MULTIBOOT_FRAMEBUF_X;
		adj_y = info->fb_height - CONFIG_X86_MULTIBOOT_FRAMEBUF_Y;
		data->pitch = (info->fb_pitch / 4) + adj_x;
		adj_x /= 2;
		adj_y /= 2;
		buffer = (uint32_t *) (uintptr_t) info->fb_addr_lo;
		buffer += adj_x;
		buffer += adj_y * data->pitch;
		data->buffer = buffer;
		return 0;
	} else {
		return -ENOTSUP;
	}
}

DEVICE_AND_API_INIT(multiboot_framebuf,
		    "FRAMEBUF",
		    multiboot_framebuf_init,
		    &multiboot_framebuf_data,
		    NULL,
		    PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &framebuf_display_api);

#endif /* CONFIG_X86_MULTIBOOT_FRAMEBUF */

#endif /* CONFIG_X86_MULTIBOOT_INFO */
