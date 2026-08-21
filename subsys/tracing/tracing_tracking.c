/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/spinlock.h>
#include <zephyr/syscall.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/heap_release_hook.h>
#include <zephyr/tracing/tracking.h>
#include <zephyr/sys/iterable_sections.h>

struct k_timer *_track_list_k_timer;
struct k_spinlock _track_list_k_timer_lock;

struct k_mem_slab *_track_list_k_mem_slab;
struct k_spinlock _track_list_k_mem_slab_lock;

struct k_sem *_track_list_k_sem;
struct k_spinlock _track_list_k_sem_lock;

struct k_mutex *_track_list_k_mutex;
struct k_spinlock _track_list_k_mutex_lock;

struct k_stack *_track_list_k_stack;
struct k_spinlock _track_list_k_stack_lock;

struct k_msgq *_track_list_k_msgq;
struct k_spinlock _track_list_k_msgq_lock;

struct k_mbox *_track_list_k_mbox;
struct k_spinlock _track_list_k_mbox_lock;

struct k_pipe *_track_list_k_pipe;
struct k_spinlock _track_list_k_pipe_lock;

struct k_queue *_track_list_k_queue;
struct k_spinlock _track_list_k_queue_lock;

#ifdef CONFIG_EVENTS
struct k_event *_track_list_k_event;
struct k_spinlock _track_list_k_event_lock;
#endif

/* Removes every tracked object that overlaps the [start, start + size)
 * memory range from one tracking list.
 *
 * The tracking lists are intrusive and permanent: an object that is tracked and
 * then freed would leave a node pointing into memory that gets reused, and the
 * next traversal would follow a corrupted link. The heap calls
 * heap_release_hook() so those nodes are dropped while the memory is still
 * valid.
 *
 * Overlap rather than containment: an in-place shrink cuts inside one
 * allocation, so an object can start below the released range while its link
 * field lies inside it. Such an object has lost part of its storage and has
 * to leave the list too.
 *
 * Must always be invoked through SYS_PORT_TRACING_TYPE_MASK(): a type that is
 * masked off has no _obj_track_next field, and the mask drops this whole block
 * along with it.
 */
#define SYS_TRACK_LIST_REMOVE_RANGE(list, start, size)                                             \
	do {                                                                                       \
		k_spinlock_key_t key = k_spin_lock(&list##_lock);                                  \
		uintptr_t _start = (uintptr_t)(start);                                             \
		__typeof__(list) *_link = &(list);                                                 \
		while (*_link != NULL) {                                                           \
			uintptr_t _addr = (uintptr_t)*_link;                                       \
			if (_addr + sizeof(**_link) > _start &&                                    \
			    _addr < _start + (size)) {                                             \
				*_link = (*_link)->_obj_track_next;                                \
			} else {                                                                   \
				_link = &(*_link)->_obj_track_next;                                \
			}                                                                          \
		}                                                                                  \
		k_spin_unlock(&list##_lock, key);                                                  \
	} while (false)

/* Iterates in the tracking list and prepends an object only when it doesn't exist */
#define SYS_TRACK_LIST_PREPEND(list, obj)                                                          \
	do {                                                                                       \
		k_spinlock_key_t key = k_spin_lock(&list##_lock);                                  \
		if ((obj) != (list)) {                                                             \
			__typeof__(list) _cur = (list);                                            \
			while (_cur != NULL) {                                                     \
				if (_cur == (obj)) {                                               \
					break;                                                     \
				}                                                                  \
				_cur = _cur->_obj_track_next;                                      \
			}                                                                          \
			if (_cur == NULL) {                                                        \
				(obj)->_obj_track_next = (list);                                   \
				(list) = (obj);                                                    \
			}                                                                          \
		}                                                                                  \
		k_spin_unlock(&list##_lock, key);                                                  \
	} while (false)

#define SYS_TRACK_STATIC_INIT(type, ...) \
	do { \
		STRUCT_SECTION_FOREACH(type, obj) \
			_SYS_PORT_TRACKING_OBJ_INIT(type)(obj, ##__VA_ARGS__); \
	} while (false)


void sys_track_k_timer_init(struct k_timer *timer)
{
	SYS_PORT_TRACING_TYPE_MASK(k_timer,
			SYS_TRACK_LIST_PREPEND(_track_list_k_timer, timer));
}

void sys_track_k_mem_slab_init(struct k_mem_slab *slab)
{
	SYS_PORT_TRACING_TYPE_MASK(k_mem_slab,
			SYS_TRACK_LIST_PREPEND(_track_list_k_mem_slab, slab));
}

void sys_track_k_sem_init(struct k_sem *sem)
{
	if (sem) {
		SYS_PORT_TRACING_TYPE_MASK(k_sem,
				SYS_TRACK_LIST_PREPEND(_track_list_k_sem, sem));
	}
}

void sys_track_k_mutex_init(struct k_mutex *mutex)
{
	SYS_PORT_TRACING_TYPE_MASK(k_mutex,
			SYS_TRACK_LIST_PREPEND(_track_list_k_mutex, mutex));
}

void sys_track_k_stack_init(struct k_stack *stack)
{
	SYS_PORT_TRACING_TYPE_MASK(k_stack,
			SYS_TRACK_LIST_PREPEND(_track_list_k_stack, stack));
}

void sys_track_k_msgq_init(struct k_msgq *msgq)
{
	SYS_PORT_TRACING_TYPE_MASK(k_msgq,
			SYS_TRACK_LIST_PREPEND(_track_list_k_msgq, msgq));
}

void sys_track_k_mbox_init(struct k_mbox *mbox)
{
	SYS_PORT_TRACING_TYPE_MASK(k_mbox,
			SYS_TRACK_LIST_PREPEND(_track_list_k_mbox, mbox));
}

void sys_track_k_pipe_init(struct k_pipe *pipe, void *buffer, size_t size)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(size);

	SYS_PORT_TRACING_TYPE_MASK(k_pipe,
			SYS_TRACK_LIST_PREPEND(_track_list_k_pipe, pipe));
}

void sys_track_k_queue_init(struct k_queue *queue)
{
	SYS_PORT_TRACING_TYPE_MASK(k_queue,
			SYS_TRACK_LIST_PREPEND(_track_list_k_queue, queue));
}

#ifdef CONFIG_EVENTS
void sys_track_k_event_init(struct k_event *event)
{
	SYS_PORT_TRACING_TYPE_MASK(k_event,
			SYS_TRACK_LIST_PREPEND(_track_list_k_event, event));
}
#endif

/* sys_heap release hook: a block of memory is about to go back to the heap,
 * so every tracked object living in it has to leave the lists first.
 *
 * The lists are unordered, so each one is walked in full: the cost is one
 * pass over every tracking list per release, and each pass takes that list's
 * spinlock. This is only ever paid with object tracking enabled, which is
 * already a debug and observability option, and the lists hold one node per
 * live kernel object.
 *
 * Called with the caller's heap lock held, so the tracking locks are always
 * taken after it and never the other way round.
 */
void heap_release_hook(void *mem, size_t bytes)
{
	/* User mode reaches this hook directly: the common libc malloc arena
	 * is user-accessible and free() runs in the caller's context. The
	 * tracking lists are kernel data, and tracked objects are only ever
	 * initialized through syscalls and released from kernel context, so
	 * a user mode release cannot hold one and is skipped.
	 */
	if (k_is_user_context()) {
		return;
	}
	compiler_barrier();

	if (mem == NULL || bytes == 0U) {
		return;
	}

	SYS_PORT_TRACING_TYPE_MASK(k_timer,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_timer, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_mem_slab,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_mem_slab, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_sem,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_sem, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_mutex,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_mutex, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_stack,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_stack, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_msgq,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_msgq, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_mbox,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_mbox, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_pipe,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_pipe, mem, bytes));
	SYS_PORT_TRACING_TYPE_MASK(k_queue,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_queue, mem, bytes));
#ifdef CONFIG_EVENTS
	SYS_PORT_TRACING_TYPE_MASK(k_event,
				   SYS_TRACK_LIST_REMOVE_RANGE(_track_list_k_event, mem, bytes));
#endif
}

#ifdef CONFIG_NETWORKING
void sys_track_socket_init(int sock, int family, int type, int proto)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(family);
	ARG_UNUSED(type);
	ARG_UNUSED(proto);
}
#endif

static int sys_track_static_init(void)
{

	SYS_PORT_TRACING_TYPE_MASK(k_timer,
			SYS_TRACK_STATIC_INIT(k_timer));

	SYS_PORT_TRACING_TYPE_MASK(k_mem_slab,
			SYS_TRACK_STATIC_INIT(k_mem_slab, 0));

	SYS_PORT_TRACING_TYPE_MASK(k_sem,
			SYS_TRACK_STATIC_INIT(k_sem, 0));

	SYS_PORT_TRACING_TYPE_MASK(k_mutex,
			SYS_TRACK_STATIC_INIT(k_mutex, 0));

	SYS_PORT_TRACING_TYPE_MASK(k_stack,
			SYS_TRACK_STATIC_INIT(k_stack));

	SYS_PORT_TRACING_TYPE_MASK(k_msgq,
			SYS_TRACK_STATIC_INIT(k_msgq));

	SYS_PORT_TRACING_TYPE_MASK(k_mbox,
			SYS_TRACK_STATIC_INIT(k_mbox));

	SYS_PORT_TRACING_TYPE_MASK(k_pipe,
			SYS_TRACK_STATIC_INIT(k_pipe, NULL, 0));

	SYS_PORT_TRACING_TYPE_MASK(k_queue,
			SYS_TRACK_STATIC_INIT(k_queue));

#ifdef CONFIG_EVENTS
	SYS_PORT_TRACING_TYPE_MASK(k_event,
			SYS_TRACK_STATIC_INIT(k_event));
#endif

	return 0;
}

SYS_INIT(sys_track_static_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
