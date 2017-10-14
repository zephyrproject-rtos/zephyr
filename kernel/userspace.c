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
#include <syscall_handler.h>

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

struct perm_ctx {
	int parent_id;
	int child_id;
	struct k_thread *parent;
};

static void wordlist_cb(struct _k_object *ko, void *ctx_ptr)
{
	struct perm_ctx *ctx = (struct perm_ctx *)ctx_ptr;

	if (sys_bitfield_test_bit((mem_addr_t)&ko->perms, ctx->parent_id) &&
				  (struct k_thread *)ko->name != ctx->parent) {
		sys_bitfield_set_bit((mem_addr_t)&ko->perms, ctx->child_id);
	}
}

void _thread_perms_inherit(struct k_thread *parent, struct k_thread *child)
{
	struct perm_ctx ctx = {
		parent->base.perm_index,
		child->base.perm_index,
		parent
	};

	if ((ctx.parent_id < 8 * CONFIG_MAX_THREAD_BYTES) &&
	    (ctx.child_id < 8 * CONFIG_MAX_THREAD_BYTES)) {
		_k_object_wordlist_foreach(wordlist_cb, &ctx);
	}
}

void _thread_perms_set(struct _k_object *ko, struct k_thread *thread)
{
	if (thread->base.perm_index < 8 * CONFIG_MAX_THREAD_BYTES) {
		sys_bitfield_set_bit((mem_addr_t)&ko->perms,
				     thread->base.perm_index);
	}
}

void _thread_perms_clear(struct _k_object *ko, struct k_thread *thread)
{
	if (thread->base.perm_index < 8 * CONFIG_MAX_THREAD_BYTES) {
		sys_bitfield_clear_bit((mem_addr_t)&ko->perms,
				       thread->base.perm_index);
	}
}

static void clear_perms_cb(struct _k_object *ko, void *ctx_ptr)
{
	int id = (int)ctx_ptr;

	sys_bitfield_clear_bit((mem_addr_t)&ko->perms, id);
}

void _thread_perms_all_clear(struct k_thread *thread)
{
	if (thread->base.perm_index < 8 * CONFIG_MAX_THREAD_BYTES) {
		_k_object_wordlist_foreach(clear_perms_cb,
					   (void *)thread->base.perm_index);
	}
}

static int thread_perms_test(struct _k_object *ko)
{
	if (ko->flags & K_OBJ_FLAG_PUBLIC) {
		return 1;
	}

	if (_current->base.perm_index < 8 * CONFIG_MAX_THREAD_BYTES) {
		return sys_bitfield_test_bit((mem_addr_t)&ko->perms,
					     _current->base.perm_index);
	}
	return 0;
}

static void dump_permission_error(struct _k_object *ko)
{
	printk("thread %p (%d) does not have permission on %s %p [",
	       _current, _current->base.perm_index,
	       otype_to_str(ko->type), ko->name);
	for (int i = CONFIG_MAX_THREAD_BYTES - 1; i >= 0; i--) {
		printk("%02x", ko->perms[i]);
	}
	printk("]\n");
}

void _dump_object_error(int retval, void *obj, struct _k_object *ko,
			enum k_objects otype)
{
	switch (retval) {
	case -EBADF:
		printk("%p is not a valid %s\n", obj, otype_to_str(otype));
		break;
	case -EPERM:
		dump_permission_error(ko);
		break;
	case -EINVAL:
		printk("%p used before initialization\n", obj);
		break;
	}
}

void _impl_k_object_access_grant(void *object, struct k_thread *thread)
{
	struct _k_object *ko = _k_object_find(object);

	if (ko) {
		_thread_perms_set(ko, thread);
	}
}

void _impl_k_object_access_revoke(void *object, struct k_thread *thread)
{
	struct _k_object *ko = _k_object_find(object);

	if (ko) {
		_thread_perms_clear(ko, thread);
	}
}

void k_object_access_all_grant(void *object)
{
	struct _k_object *ko = _k_object_find(object);

	if (ko) {
		ko->flags |= K_OBJ_FLAG_PUBLIC;
	}
}

int _k_object_validate(struct _k_object *ko, enum k_objects otype, int init)
{
	if (!ko || (otype != K_OBJ_ANY && ko->type != otype)) {
		return -EBADF;
	}

	/* Manipulation of any kernel objects by a user thread requires that
	 * thread be granted access first, even for uninitialized objects
	 */
	if (!thread_perms_test(ko)) {
		return -EPERM;
	}

	/* If we are not initializing an object, and the object is not
	 * initialized, we should freak out
	 */
	if (!init && !(ko->flags & K_OBJ_FLAG_INITIALIZED)) {
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

	/* Allows non-initialization system calls to be made on this object */
	ko->flags |= K_OBJ_FLAG_INITIALIZED;
}

void _k_object_uninit(void *object)
{
	struct _k_object *ko;

	/* See comments in _k_object_init() */
	ko = _k_object_find(object);
	if (!ko) {
		return;
	}

	ko->flags &= ~K_OBJ_FLAG_INITIALIZED;
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

