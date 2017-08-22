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
#else
	ARG_UNUSED(otype);
	return NULL;
#endif
}

/* Stub functions, to be filled in forthcoming patch sets */

static void set_thread_perms(struct _k_object *ko, struct k_thread *thread)
{
	ARG_UNUSED(ko);
	ARG_UNUSED(thread);

	/* STUB */
}


static int test_thread_perms(struct _k_object *ko)
{
	ARG_UNUSED(ko);

	/* STUB */

	return 1;
}

static int _is_thread_user(void)
{
	/* STUB */

	return 0;
}

void k_object_grant_access(void *object, struct k_thread *thread)
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
			return;
		}
	}

	/* userspace can't grant access to objects unless it already has
	 * access to that object
	 */
	if (_is_thread_user() && !test_thread_perms(ko)) {
		printk("insufficient permissions in current thread %p\n",
		       _current);
		printk("Cannot grant access to %s %p for thread %p\n",
		       otype_to_str(ko->type), object, thread);
		k_oops();
	}
	set_thread_perms(ko, thread);
}


int _k_object_validate(void *obj, enum k_objects otype, int init)
{
	struct _k_object *ko;

	ko = _k_object_find(obj);

	if (!ko || ko->type != otype) {
		printk("%p is not a %s\n", obj, otype_to_str(otype));
		return -EBADF;
	}

	/* Uninitialized objects are not owned by anyone. However if an
	 * object is initialized, and the caller is from userspace, then
	 * we need to assert that the user thread has sufficient permissions
	 * to re-initialize.
	 */
	if (ko->flags & K_OBJ_FLAG_INITIALIZED && _is_thread_user() &&
	    !test_thread_perms(ko)) {
		printk("thread %p does not have permission on %s %p\n",
		       _current, otype_to_str(otype), obj);
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

