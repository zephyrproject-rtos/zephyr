/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <linker/linker-defs.h>
#include <misc/__assert.h>
#include <misc/printk.h>
#include <init.h>
#include <drivers/rand32.h>

int initialized;

struct object_metadata {
	u8_t *start;
	u8_t *end;
	u32_t offset;
	u32_t size;
	u32_t xor_key;
};

#define OBJ_INFO(name) \
	{ \
	.start = &_##name##_list_start, \
	.end = &_##name##_list_end, \
	.size = sizeof(struct name), \
	.offset = offsetof(struct name, obj), \
	}

#define OBJ_INFO_NOLIST(name) \
	{ \
	.start = NULL, \
	.end = NULL, \
	.size = sizeof(struct name), \
	.offset = offsetof(struct name, obj), \
	}

/* For regions defined in linker/common-ram.ld */
#define OBJ_BOUNDS(name) \
	extern u8_t _##name##_list_start; \
	extern u8_t _##name##_list_end

OBJ_BOUNDS(k_alert);
OBJ_BOUNDS(k_mem_slab);
OBJ_BOUNDS(k_msgq);
OBJ_BOUNDS(k_mutex);
OBJ_BOUNDS(k_pipe);
OBJ_BOUNDS(k_sem);
OBJ_BOUNDS(k_stack);
OBJ_BOUNDS(k_timer);
OBJ_BOUNDS(k_work);

static struct object_metadata ometa[K_OBJ_LAST] = {
	[K_OBJ_ALERT] = OBJ_INFO(k_alert),
	[K_OBJ_DELAYED_WORK] = OBJ_INFO_NOLIST(k_delayed_work),
	[K_OBJ_MEM_SLAB] = OBJ_INFO(k_mem_slab),
	[K_OBJ_MSGQ] = OBJ_INFO(k_msgq),
	[K_OBJ_MUTEX] = OBJ_INFO(k_mutex),
	[K_OBJ_PIPE] = OBJ_INFO(k_pipe),
	[K_OBJ_SEM] = OBJ_INFO(k_sem),
	[K_OBJ_STACK] = OBJ_INFO(k_stack),
	[K_OBJ_THREAD] = OBJ_INFO_NOLIST(k_thread),
	[K_OBJ_TIMER] = OBJ_INFO(k_timer),
	[K_OBJ_WORK] = OBJ_INFO(k_work),
	[K_OBJ_WORK_Q] = OBJ_INFO_NOLIST(k_work_q),
};

#define TO_OBJECT(ptr, type_enum) \
	((u8_t *)ptr + ometa[type_enum].offset)

#define FROM_OBJECT(ptr, type_enum) \
	((u8_t *)ptr - ometa[type_enum].offset)

static int init_k_obj_validate(struct device *unused)
{
	ARG_UNUSED(unused);
	u8_t *pos;
	enum k_objects i;

	initialized = 1;

	for (i = 0; i < K_OBJ_LAST; i++) {
		struct object_metadata *o = &ometa[i];

#if CONFIG_RANDOM_HAS_DRIVER
		o->xor_key = sys_rand32_get();
#else
		o->xor_key = i;
#endif
		if (o->start == NULL) {
			continue;
		}

		for (pos = o->start; pos < o->end; pos += o->size) {
			_k_object_init(pos, i);

			/* Some objects have other objects embedded within */
			switch (i) {
			case K_OBJ_ALERT: {
				struct k_alert *a;
				a = (struct k_alert *)FROM_OBJECT(pos, i);
				_k_object_init(&a->sem, K_OBJ_SEM);
				_k_object_init(&a->work_item, K_OBJ_WORK);
				break;
			}
			default:
				break;
			}
		}
	}

	return 0;
}

#ifdef CONFIG_PRINTK
const char *otype_to_str(enum k_objects otype)
{
	switch (otype) {
	case K_OBJ_ALERT:
		return "k_alert";
	case K_OBJ_DELAYED_WORK:
		return "k_delayed_work";
	case K_OBJ_MEM_SLAB:
		return "k_mem_slab";
	case K_OBJ_MSGQ:
		return "k_msgq";
	case K_OBJ_MUTEX:
		return "k_mutex";
	case K_OBJ_PIPE:
		return "k_pipe";
	case K_OBJ_SEM:
		return "k_sem";
	case K_OBJ_STACK:
		return "k_stack";
	case K_OBJ_THREAD:
		return "k_thread";
	case K_OBJ_TIMER:
		return "k_timer";
	case K_OBJ_WORK:
		return "k_work";
	case K_OBJ_WORK_Q:
		return "k_work_q";
	default:
		return "?";
	}
}
#endif


static void address_check(u32_t oaddr, enum k_objects otype)
{
	u32_t ram_start, ram_end;

#ifdef CONFIG_APPLICATION_MEMORY
	ram_start = ((u32_t)&__kernel_ram_start);
	ram_end = ((u32_t)&__kernel_ram_end);
#else
	ram_start = ((u32_t)&_image_ram_start);
	ram_end = ((u32_t)&_image_ram_end);
#endif

	/* Make sure the kernel object is in RAM somewhere */
	if (oaddr < ram_start || oaddr > (ram_end - sizeof(struct k_object))) {
#ifdef CONFIG_PRINTK
		printk("Bad memory range for %s kernel object %p\n",
		       otype_to_str(otype), (void *)oaddr);
#ifdef CONFIG_APPLICATION_MEMORY
		{
			u32_t app_start, app_end;

			app_start = ((u32_t)&__app_ram_start);
			app_end = ((u32_t)&__app_ram_end);

			/* Expect this to be a very common mistake, let's at
			 * least be helpful about it if printks are turned on
			 */
			if (oaddr >= app_start && oaddr <
			    (app_end - sizeof(struct k_object))) {
				printk("Object found within application memory\n");
				printk("Please define object with __kernel\n");
			}
		}
#endif /* CONFIG_APPLICATION_MEMORY */
#endif /* CONFIG_PRINTK */
		k_oops();
	}
}


void _k_object_validate(void *obj, enum k_objects otype)
{
	u32_t oaddr;
	struct k_object *kobj;

	oaddr = (u32_t)TO_OBJECT(obj, otype);

	address_check(oaddr, otype);
	kobj = (struct k_object *)oaddr;

	/* In some cases the kernel uses some statically-initialzed objects
	 * internally way before we can init the random number generator.
	 */
	if (!initialized) {
		return;
	}

	if ((kobj->xor_ptr ^ ometa[otype].xor_key) != (u32_t)obj) {
#ifdef CONFIG_PRINTK
		printk("%p is not a %s\n", obj, otype_to_str(otype));
#endif
		k_oops();
	}
}


void _k_object_init(void *obj, enum k_objects otype)
{
	u32_t oaddr;
	struct k_object *kobj;

	oaddr = (u32_t)obj;
	__ASSERT(initialized, "init k_object too early");

	address_check(oaddr, otype);

	kobj = (struct k_object *)TO_OBJECT(oaddr, otype);

	kobj->xor_ptr = oaddr ^ ometa[otype].xor_key;
}


/* Needs to be the last thing that happens in PRE_KERNEL_2 as we need the
 * random number generator up and running
 */
SYS_INIT(init_k_obj_validate, PRE_KERNEL_2, 99);

