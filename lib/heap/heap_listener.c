/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/spinlock.h>
#include <zephyr/sys/heap_listener.h>

static struct k_spinlock heap_listener_lock;
static sys_slist_t heap_listener_list = SYS_SLIST_STATIC_INIT(&heap_listener_list);

void heap_listener_register(struct heap_listener *listener)
{
	k_spinlock_key_t key = k_spin_lock(&heap_listener_lock);

	sys_slist_append(&heap_listener_list, &listener->node);

	k_spin_unlock(&heap_listener_lock, key);
}

void heap_listener_unregister(struct heap_listener *listener)
{
	k_spinlock_key_t key = k_spin_lock(&heap_listener_lock);

	sys_slist_find_and_remove(&heap_listener_list, &listener->node);

	k_spin_unlock(&heap_listener_lock, key);
}

void heap_listener_notify_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	struct heap_listener *listener;
	k_spinlock_key_t key = k_spin_lock(&heap_listener_lock);

	SYS_SLIST_FOR_EACH_CONTAINER(&heap_listener_list, listener, node) {
		if (listener->heap_id == heap_id && listener->cb.alloc != NULL &&
		    listener->event == HEAP_ALLOC) {
			listener->cb.alloc(heap_id, mem, bytes);
		}
	}

	k_spin_unlock(&heap_listener_lock, key);
}

void heap_listener_notify_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	struct heap_listener *listener;
	k_spinlock_key_t key = k_spin_lock(&heap_listener_lock);

	SYS_SLIST_FOR_EACH_CONTAINER(&heap_listener_list, listener, node) {
		if (listener->heap_id == heap_id && listener->cb.free != NULL &&
		    listener->event == HEAP_FREE) {
			listener->cb.free(heap_id, mem, bytes);
		}
	}

	k_spin_unlock(&heap_listener_lock, key);
}

void heap_listener_notify_resize(uintptr_t heap_id, void *old_heap_end, void *new_heap_end)
{
	struct heap_listener *listener;
	k_spinlock_key_t key = k_spin_lock(&heap_listener_lock);

	SYS_SLIST_FOR_EACH_CONTAINER(&heap_listener_list, listener, node) {
		if (listener->heap_id == heap_id && listener->cb.resize != NULL &&
		    listener->event == HEAP_RESIZE) {
			listener->cb.resize(heap_id, old_heap_end, new_heap_end);
		}
	}

	k_spin_unlock(&heap_listener_lock, key);
}
