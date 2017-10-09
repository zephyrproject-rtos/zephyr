/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel.h>
#include <string.h>
#include <misc/printk.h>
#include <kernel_structs.h>
#include <sys_io.h>
#include <ksched.h>
#include <syscall.h>

/**
 * Kernel object validation function
 *
 * Retrieve metadata for a kernel object. This function is implemented in
 * the gperf script footer, see gen_kobject_list.py
 *
 * @param obj Address of kernel object to get metadata
 * @return Kernel object's metadata, or NULL if the parameter wasn't the
 * memory address of a kernel object
 */
extern struct _k_object *_k_object_find(void *obj);

const char *otype_to_str(enum k_objects otype)
{
	/* -fdata-sections doesn't work right except in very very recent
	 * GCC and these literal strings would appear in the binary even if
	 * otype_to_str was omitted by the linker
	 */
#ifdef CONFIG_PRINTK
	switch (otype) {
	/* Core kernel objects */
	case K_OBJ_ALERT:
		return "k_alert";
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

	/* Driver subsystems */
	case K_OBJ_DRIVER_ADC:
		return "adc driver";
	case K_OBJ_DRIVER_AIO_CMP:
		return "aio comparator driver";
	case K_OBJ_DRIVER_CLOCK_CONTROL:
		return "clock control driver";
	case K_OBJ_DRIVER_COUNTER:
		return "counter driver";
	case K_OBJ_DRIVER_CRYPTO:
		return "crypto driver";
	case K_OBJ_DRIVER_DMA:
		return "dma driver";
	case K_OBJ_DRIVER_ETH:
		return "ethernet driver";
	case K_OBJ_DRIVER_FLASH:
		return "flash driver";
	case K_OBJ_DRIVER_GPIO:
		return "gpio driver";
	case K_OBJ_DRIVER_I2C:
		return "i2c driver";
	case K_OBJ_DRIVER_I2S:
		return "i2s driver";
	case K_OBJ_DRIVER_IPM:
		return "ipm driver";
	case K_OBJ_DRIVER_PINMUX:
		return "pinmux driver";
	case K_OBJ_DRIVER_PWM:
		return "pwm driver";
	case K_OBJ_DRIVER_RANDOM:
		return "random driver";
	case K_OBJ_DRIVER_RTC:
		return "realtime clock driver";
	case K_OBJ_DRIVER_SENSOR:
		return "sensor driver";
	case K_OBJ_DRIVER_SHARED_IRQ:
		return "shared irq driver";
	case K_OBJ_DRIVER_SPI:
		return "spi driver";
	case K_OBJ_DRIVER_UART:
		return "uart driver";
	case K_OBJ_DRIVER_WDT:
		return "watchdog timer driver";
	default:
		return "?";
	}
#else
	ARG_UNUSED(otype);
	return NULL;
#endif
}

/* Stub functions, to be filled in forthcoming patch sets */

static void set_thread_perms(struct _k_object *ko, struct k_thread *thread)
{
	if (thread->base.perm_index < 8 * CONFIG_MAX_THREAD_BYTES) {
		sys_bitfield_set_bit((mem_addr_t)&ko->perms,
				     thread->base.perm_index);
	}
}

static int test_thread_perms(struct _k_object *ko)
{
	if (_current->base.perm_index < 8 * CONFIG_MAX_THREAD_BYTES) {
		return sys_bitfield_test_bit((mem_addr_t)&ko->perms,
					     _current->base.perm_index);
	}
	return 0;
}

/**
 * Kernek object permission modification check
 *
 * Check that the caller has sufficient perms to modify access permissions for
 * a particular kernel object. oops() if a user thread is trying to something
 * forbidden.
 *
 * @param object to be modified
 * @return NULL if the caller is a kernel thread and the object was not found
 */
static struct _k_object *access_check(void *object)
{
	struct _k_object *ko = _k_object_find(object);

	if (!ko) {
		if (_is_thread_user()) {
			printk("granting access	to non-existent kernel object %p\n",
			       object);
			k_oops();
		} else {
			/* Supervisor threads may at times instantiate objects
			 * that ignore rules on where they can live. Such
			 * objects won't ever be usable from userspace, but
			 * we shouldn't explode.
			 */
			return NULL;
		}
	}

	/* userspace can't grant access to objects unless it already has
	 * access to that object
	 */
	if (_is_thread_user() && !test_thread_perms(ko)) {
		printk("insufficient permissions in current thread %p\n",
		       _current);
		printk("Cannot grant access to %s %p\n",
		       otype_to_str(ko->type), object);
		k_oops();
	}

	return ko;
}

void _impl_k_object_access_grant(void *object, struct k_thread *thread)
{
	struct _k_object *ko = access_check(object);

	if (ko) {
		set_thread_perms(ko, thread);
	}
}

void _impl_k_object_access_all_grant(void *object)
{
	struct _k_object *ko = access_check(object);

	if (ko) {
		memset(ko->perms, 0xFF, CONFIG_MAX_THREAD_BYTES);
	}
}

int _k_object_validate(void *obj, enum k_objects otype, int init)
{
	struct _k_object *ko;

	ko = _k_object_find(obj);

	if (!ko || ko->type != otype) {
		printk("%p is not a %s\n", obj, otype_to_str(otype));
		return -EBADF;
	}

	/* Manipulation of any kernel objects by a user thread requires that
	 * thread be granted access first, even for uninitialized objects
	 */
	if (_is_thread_user() && !test_thread_perms(ko)) {
		printk("thread %p (%d) does not have permission on %s %p [",
		       _current, _current->base.perm_index, otype_to_str(otype),
		       obj);
		for (int i = CONFIG_MAX_THREAD_BYTES - 1; i >= 0; i--) {
			printk("%02x", ko->perms[i]);
		}
		printk("]\n");
		return -EPERM;
	}

	/* If we are not initializing an object, and the object is not
	 * initialized, we should freak out
	 */
	if (!init && !(ko->flags & K_OBJ_FLAG_INITIALIZED)) {
		printk("%p used before initialization\n", obj);
		return -EINVAL;
	}

	return 0;
}


void _k_object_init(void *object)
{
	struct _k_object *ko;

	/* By the time we get here, if the caller was from userspace, all the
	 * necessary checks have been done in _k_object_validate(), which takes
	 * place before the object is initialized.
	 *
	 * This function runs after the object has been initialized and
	 * finalizes it
	 */

	ko = _k_object_find(object);
	if (!ko) {
		/* Supervisor threads can ignore rules about kernel objects
		 * and may declare them on stacks, etc. Such objects will never
		 * be usable from userspace, but we shouldn't explode.
		 */
		return;
	}

	memset(ko->perms, 0, CONFIG_MAX_THREAD_BYTES);
	set_thread_perms(ko, _current);

	ko->flags |= K_OBJ_FLAG_INITIALIZED;
}

static u32_t _handler_bad_syscall(u32_t bad_id, u32_t arg2, u32_t arg3,
				  u32_t arg4, u32_t arg5, u32_t arg6, void *ssf)
{
	printk("Bad system call id %u invoked\n", bad_id);
	_arch_syscall_oops(ssf);
	CODE_UNREACHABLE;
}

static u32_t _handler_no_syscall(u32_t arg1, u32_t arg2, u32_t arg3,
				 u32_t arg4, u32_t arg5, u32_t arg6, void *ssf)
{
	printk("Unimplemented system call\n");
	_arch_syscall_oops(ssf);
	CODE_UNREACHABLE;
}

#include <syscall_dispatch.c>

