/*
 *  Copyright (c) 2026 Picoheart Inc.
 *  Copyright (c) 2026 LiuQian.andy <liuqian.andy@picoheart.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/heaptrace.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/heap_listener.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

LOG_MODULE_REGISTER(heaptrace, CONFIG_LOG_DEFAULT_LEVEL);

#define HEAPTRACE_MAX_RECORDS        CONFIG_HEAPTRACE_MAX_RECORDS
#define HEAPTRACE_MAX_RESIZE_RECORDS CONFIG_HEAPTRACE_MAX_RESIZE_RECORDS
#define HEAPTRACE_MAX_HEAPS          2

#if defined(CONFIG_HEAPTRACE_BACKTRACE)
#define HEAPTRACE_BACKTRACE_DEPTH CONFIG_HEAPTRACE_BACKTRACE_DEPTH
#define HEAPTRACE_BACKTRACE_SKIP  CONFIG_HEAPTRACE_BACKTRACE_SKIP
#include <zephyr/arch/arch_interface.h>
#endif

struct heaptrace_record {
	void *ptr;
	size_t size;
	uintptr_t heap_id;

	k_tid_t alloc_tid;
	/*
	 * save thread name immediately.
	 * if shell cmd get name by tid after thread destroyed.
	 * it will get nothing used info.
	 */
	char alloc_thread_name[HEAPTRACE_THREAD_NAME_LEN];
	uint32_t alloc_time_ms;
	unsigned int alloc_cpu;

#if defined(CONFIG_HEAPTRACE_BACKTRACE)
	int bt_depth;
	unsigned long bt_buf[HEAPTRACE_BACKTRACE_DEPTH];
#else
	int bt_depth;
#endif

	bool used;
};

struct heaptrace_filter_state {
	enum heaptrace_filter_type type;
	union {
		k_tid_t tid;
		char name[HEAPTRACE_THREAD_NAME_LEN];
		unsigned int cpu_id;
	} val;
};

struct heaptrace_resize_record {
	uintptr_t heap_id;
	void *old_end;
	void *new_end;
	k_tid_t tid;
	char thread_name[HEAPTRACE_THREAD_NAME_LEN];
	uint32_t time_ms;
	unsigned int cpu;
};

struct heaptrace_heap_boundary {
	uintptr_t heap_id;
	void *heap_start;
	void *heap_end;
	bool valid;
};

static struct heaptrace_record heaptrace_records[HEAPTRACE_MAX_RECORDS];
static struct k_spinlock heaptrace_lock;
static struct heaptrace_filter_state heaptrace_filter;
static struct heaptrace_resize_record heaptrace_resizes[HEAPTRACE_MAX_RESIZE_RECORDS];
static int heaptrace_resize_head;
static int heaptrace_resize_count;
static struct heaptrace_heap_boundary heaptrace_heaps[HEAPTRACE_MAX_HEAPS];

#if defined(CONFIG_HEAPTRACE_BACKTRACE)

struct bt_collect_ctx {
	int cnt;
	int max;
	int skip;
	unsigned long *buf;
};

static bool bt_collect_cb(void *cookie, unsigned long addr)
{
	struct bt_collect_ctx *ctx = cookie;

	if (ctx->skip > 0) {
		ctx->skip--;
		return true;
	}

	if (ctx->cnt >= ctx->max) {
		return false;
	}

	ctx->buf[ctx->cnt++] = addr;
	return true;
}

static int heaptrace_capture_backtrace(unsigned long *buf, int max_depth)
{
	struct bt_collect_ctx ctx = {
		.cnt = 0,
		.max = max_depth,
		.skip = HEAPTRACE_BACKTRACE_SKIP,
		.buf = buf,
	};

	arch_stack_walk(bt_collect_cb, &ctx, k_current_get(), NULL);
	return ctx.cnt;
}

#endif

static const char *heaptrace_heap_id_to_name(uintptr_t heap_id)
{
	if (heap_id == HEAP_ID_LIBC) {
		return "libc";
	}
	return "sys_heap";
}

static void heaptrace_get_thread_name_by_tid(k_tid_t tid, char *buf, size_t len)
{
	const char *name;

	if (buf == NULL || len == 0U) {
		return;
	}

	name = k_thread_name_get(tid);
	if (name != NULL && name[0] != '\0') {
		strncpy(buf, name, len - 1);
		buf[len - 1] = '\0';
	} else {
		snprintk(buf, len, "<unnamed>:%p", tid);
	}
}

static int heaptrace_find_record_by_ptr_nolock(void *ptr)
{
	for (int i = 0; i < HEAPTRACE_MAX_RECORDS; i++) {
		if (heaptrace_records[i].used && heaptrace_records[i].ptr == ptr) {
			return i;
		}
	}
	return -1;
}

static int heaptrace_find_free_slot_nolock(void)
{
	for (int i = 0; i < HEAPTRACE_MAX_RECORDS; i++) {
		if (!heaptrace_records[i].used) {
			return i;
		}
	}
	return -1;
}

static unsigned int heaptrace_current_cpu_id(void)
{
#ifdef CONFIG_SMP
	return arch_curr_cpu()->id;
#else
	return 0;
#endif
}

static bool heaptrace_event_matches_filter_nolock(k_tid_t tid, unsigned int cpu)
{
	const char *name;

	switch (heaptrace_filter.type) {
	case HEAPTRACE_FILTER_TID:
		return tid == heaptrace_filter.val.tid;
	case HEAPTRACE_FILTER_CPU:
		return cpu == heaptrace_filter.val.cpu_id;
	case HEAPTRACE_FILTER_NAME:
		name = k_thread_name_get(tid);
		if (name != NULL && strstr(name, heaptrace_filter.val.name) != NULL) {
			return true;
		}
		return false;
	default:
		return true;
	}
}

static struct heaptrace_heap_boundary *heaptrace_find_boundary_nolock(uintptr_t heap_id)
{
	for (int i = 0; i < HEAPTRACE_MAX_HEAPS; i++) {
		if (heaptrace_heaps[i].valid && heaptrace_heaps[i].heap_id == heap_id) {
			return &heaptrace_heaps[i];
		}
	}
	return NULL;
}

static struct heaptrace_heap_boundary *heaptrace_find_free_boundary_nolock(void)
{
	for (int i = 0; i < HEAPTRACE_MAX_HEAPS; i++) {
		if (!heaptrace_heaps[i].valid) {
			return &heaptrace_heaps[i];
		}
	}
	return NULL;
}

void heaptrace_alloc(void *ptr, size_t size, uintptr_t heap_id)
{
	k_tid_t tid;
	unsigned int cpu;
	int idx;
	k_spinlock_key_t key;
#if defined(CONFIG_HEAPTRACE_BACKTRACE)
	unsigned long bt_buf[HEAPTRACE_BACKTRACE_DEPTH];
	int bt_depth;
#endif

	if (ptr == NULL || size == 0U) {
		return;
	}

	tid = k_current_get();
	cpu = heaptrace_current_cpu_id();

#if defined(CONFIG_HEAPTRACE_BACKTRACE)
	/* Walk the current stack before taking the spinlock: arch_stack_walk
	 * does not need the heaptrace lock, and this callback already fires
	 * from inside the allocator's own lock. Capturing under both locks
	 * with interrupts disabled made every allocation pay for a full
	 * unwind in a critical section.
	 */
	bt_depth = heaptrace_capture_backtrace(bt_buf, HEAPTRACE_BACKTRACE_DEPTH);
#endif

	key = k_spin_lock(&heaptrace_lock);

	if (!heaptrace_event_matches_filter_nolock(tid, cpu)) {
		k_spin_unlock(&heaptrace_lock, key);
		return;
	}

	idx = heaptrace_find_record_by_ptr_nolock(ptr);
	if (idx >= 0) {
		LOG_WRN("alloc duplicate ptr=%p old_size=%zu new_size=%zu "
			"old_alloc_tid=%p old_alloc_ms=%u "
			"(possible missed free or allocator corruption)",
			ptr, heaptrace_records[idx].size, size,
			heaptrace_records[idx].alloc_tid, heaptrace_records[idx].alloc_time_ms);
	} else {
		idx = heaptrace_find_free_slot_nolock();
	}

	if (idx < 0) {
		k_spin_unlock(&heaptrace_lock, key);
		LOG_ERR("record table full, ptr=%p size=%zu", ptr, size);
		return;
	}

	memset(&heaptrace_records[idx], 0, sizeof(heaptrace_records[idx]));
	heaptrace_records[idx].used = true;
	heaptrace_records[idx].ptr = ptr;
	heaptrace_records[idx].size = size;
	heaptrace_records[idx].heap_id = heap_id;
	heaptrace_records[idx].alloc_tid = tid;
	heaptrace_records[idx].alloc_time_ms = k_uptime_get_32();
	heaptrace_records[idx].alloc_cpu = cpu;
	heaptrace_get_thread_name_by_tid(tid, heaptrace_records[idx].alloc_thread_name,
					 sizeof(heaptrace_records[idx].alloc_thread_name));

#if defined(CONFIG_HEAPTRACE_BACKTRACE)
	heaptrace_records[idx].bt_depth = bt_depth;
	memcpy(heaptrace_records[idx].bt_buf, bt_buf,
	       (size_t)bt_depth * sizeof(unsigned long));
#endif

	k_spin_unlock(&heaptrace_lock, key);
}

void heaptrace_free(void *ptr, size_t size, uintptr_t heap_id)
{
	int idx;
	k_tid_t tid;
	k_spinlock_key_t key;

	if (ptr == NULL) {
		return;
	}

	tid = k_current_get();

	key = k_spin_lock(&heaptrace_lock);

	/*
	 * The acquisition filter only gates which alloc events are recorded.
	 * Free must always attempt removal: a block allocated by a filtered
	 * thread may be freed by another thread, and dropping the free here
	 * would leave the record in the table forever, reporting a phantom
	 * leak.
	 */
	idx = heaptrace_find_record_by_ptr_nolock(ptr);
	if (idx < 0) {
		k_spin_unlock(&heaptrace_lock, key);
		LOG_WRN("free unknown ptr=%p size=%zu heap=%s free_tid=%p", ptr, size,
			heaptrace_heap_id_to_name(heap_id), tid);
		return;
	}

	memset(&heaptrace_records[idx], 0, sizeof(heaptrace_records[idx]));

	k_spin_unlock(&heaptrace_lock, key);
}

void heaptrace_reset(void)
{
	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	memset(heaptrace_records, 0, sizeof(heaptrace_records));
	memset(heaptrace_resizes, 0, sizeof(heaptrace_resizes));
	memset(heaptrace_heaps, 0, sizeof(heaptrace_heaps));
	heaptrace_resize_head = 0;
	heaptrace_resize_count = 0;
	k_spin_unlock(&heaptrace_lock, key);
}

static void heaptrace_fill_info(struct heaptrace_record_info *info,
				const struct heaptrace_record *rec, unsigned long *bt_copy,
				int bt_copy_len)
{
	info->ptr = rec->ptr;
	info->size = rec->size;
	info->heap_id = rec->heap_id;
	info->alloc_tid = rec->alloc_tid;
	memcpy(info->alloc_thread_name, rec->alloc_thread_name, sizeof(info->alloc_thread_name));
	info->alloc_time_ms = rec->alloc_time_ms;
	info->alloc_cpu = rec->alloc_cpu;
	info->bt_depth = rec->bt_depth;
#if defined(CONFIG_HEAPTRACE_BACKTRACE)
	{
		int n = rec->bt_depth;

		if (n > bt_copy_len) {
			n = bt_copy_len;
		}
		if (n > 0) {
			memcpy(bt_copy, rec->bt_buf, (size_t)n * sizeof(unsigned long));
		}
		info->bt_buf = bt_copy;
	}
#else
	info->bt_buf = NULL;
#endif
}

void heaptrace_foreach_record(heaptrace_record_cb_t cb, void *user_data)
{
	struct heaptrace_record_info info;
#if defined(CONFIG_HEAPTRACE_BACKTRACE)
	unsigned long bt_copy[HEAPTRACE_BACKTRACE_DEPTH];
	const int bt_copy_len = HEAPTRACE_BACKTRACE_DEPTH;
#else
	unsigned long bt_copy[1];
	const int bt_copy_len = 0;
#endif
	int i = 0;

	while (1) {
		k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

		while (i < HEAPTRACE_MAX_RECORDS && !heaptrace_records[i].used) {
			i++;
		}

		if (i >= HEAPTRACE_MAX_RECORDS) {
			k_spin_unlock(&heaptrace_lock, key);
			return;
		}

		heaptrace_fill_info(&info, &heaptrace_records[i], bt_copy, bt_copy_len);
		i++;

		k_spin_unlock(&heaptrace_lock, key);

		if (!cb(&info, user_data)) {
			return;
		}
	}
}

size_t heaptrace_outstanding_bytes(void)
{
	size_t total = 0;
	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	for (int i = 0; i < HEAPTRACE_MAX_RECORDS; i++) {
		if (heaptrace_records[i].used) {
			total += heaptrace_records[i].size;
		}
	}

	k_spin_unlock(&heaptrace_lock, key);
	return total;
}

size_t heaptrace_outstanding_blocks(void)
{
	size_t count = 0;
	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	for (int i = 0; i < HEAPTRACE_MAX_RECORDS; i++) {
		if (heaptrace_records[i].used) {
			count++;
		}
	}

	k_spin_unlock(&heaptrace_lock, key);
	return count;
}

struct find_tid_ctx {
	const char *name;
	k_tid_t result;
};

static void find_tid_cb(const struct k_thread *thread, void *user_data)
{
	struct find_tid_ctx *ctx = user_data;
	const char *name;

	if (ctx->result != NULL) {
		return;
	}

	name = k_thread_name_get((k_tid_t)thread);
	if (name != NULL && strstr(name, ctx->name) != NULL) {
		ctx->result = (k_tid_t)thread;
	}
}

k_tid_t heaptrace_find_tid_by_name(const char *name)
{
	struct find_tid_ctx ctx = {
		.name = name,
		.result = NULL,
	};

	k_thread_foreach(find_tid_cb, &ctx);
	return ctx.result;
}

void heaptrace_set_filter_tid(k_tid_t tid)
{
	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	heaptrace_filter.type = HEAPTRACE_FILTER_TID;
	heaptrace_filter.val.tid = tid;

	k_spin_unlock(&heaptrace_lock, key);
}

void heaptrace_set_filter_name(const char *name)
{
	k_spinlock_key_t key;

	if (name == NULL || name[0] == '\0') {
		return;
	}

	key = k_spin_lock(&heaptrace_lock);

	heaptrace_filter.type = HEAPTRACE_FILTER_NAME;
	strncpy(heaptrace_filter.val.name, name, sizeof(heaptrace_filter.val.name) - 1);
	heaptrace_filter.val.name[sizeof(heaptrace_filter.val.name) - 1] = '\0';

	k_spin_unlock(&heaptrace_lock, key);
}

void heaptrace_set_filter_cpu(unsigned int cpu_id)
{
	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	heaptrace_filter.type = HEAPTRACE_FILTER_CPU;
	heaptrace_filter.val.cpu_id = cpu_id;

	k_spin_unlock(&heaptrace_lock, key);
}

void heaptrace_clear_filter(void)
{
	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	heaptrace_filter.type = HEAPTRACE_FILTER_NONE;
	memset(&heaptrace_filter.val, 0, sizeof(heaptrace_filter.val));

	k_spin_unlock(&heaptrace_lock, key);
}

enum heaptrace_filter_type heaptrace_get_filter_type(void)
{
	return heaptrace_filter.type;
}

k_tid_t heaptrace_get_filter_tid(void)
{
	return heaptrace_filter.val.tid;
}

const char *heaptrace_get_filter_name(void)
{
	return heaptrace_filter.val.name;
}

unsigned int heaptrace_get_filter_cpu(void)
{
	return heaptrace_filter.val.cpu_id;
}

const char *heaptrace_filter_str(char *buf, size_t len)
{
	switch (heaptrace_filter.type) {
	case HEAPTRACE_FILTER_TID:
		snprintk(buf, len, "tid=%p", heaptrace_filter.val.tid);
		break;
	case HEAPTRACE_FILTER_NAME:
		snprintk(buf, len, "name=\"%s\"", heaptrace_filter.val.name);
		break;
	case HEAPTRACE_FILTER_CPU:
		snprintk(buf, len, "cpu=%u", heaptrace_filter.val.cpu_id);
		break;
	default:
		snprintk(buf, len, "none");
		break;
	}
	return buf;
}

void heaptrace_resize(uintptr_t heap_id, void *old_end, void *new_end)
{
	k_tid_t tid = k_current_get();
	unsigned int cpu = heaptrace_current_cpu_id();
	struct heaptrace_heap_boundary *b;
	int idx;
	k_spinlock_key_t key;

	key = k_spin_lock(&heaptrace_lock);

	b = heaptrace_find_boundary_nolock(heap_id);
	if (b == NULL) {
		b = heaptrace_find_free_boundary_nolock();
		if (b != NULL) {
			b->heap_id = heap_id;
			b->heap_start = old_end;
			b->valid = true;
		} else {
			LOG_WRN("no free boundary found heap_id=0x%lx old_end=%p new_end=%p",
				(unsigned long)heap_id, old_end, new_end);
		}
	}
	if (b != NULL) {
		b->heap_end = new_end;
	}

	idx = heaptrace_resize_head;
	heaptrace_resizes[idx].heap_id = heap_id;
	heaptrace_resizes[idx].old_end = old_end;
	heaptrace_resizes[idx].new_end = new_end;
	heaptrace_resizes[idx].tid = tid;
	heaptrace_resizes[idx].time_ms = k_uptime_get_32();
	heaptrace_resizes[idx].cpu = cpu;
	heaptrace_get_thread_name_by_tid(tid, heaptrace_resizes[idx].thread_name,
					 sizeof(heaptrace_resizes[idx].thread_name));

	heaptrace_resize_head = (heaptrace_resize_head + 1) % HEAPTRACE_MAX_RESIZE_RECORDS;
	if (heaptrace_resize_count < HEAPTRACE_MAX_RESIZE_RECORDS) {
		heaptrace_resize_count++;
	}

	k_spin_unlock(&heaptrace_lock, key);

	LOG_INF("heap %s resized: old_end=%p new_end=%p delta=%+ld tid=%p",
		heaptrace_heap_id_to_name(heap_id), old_end, new_end,
		(long)((char *)new_end - (char *)old_end), tid);
}

void heaptrace_foreach_resize(heaptrace_resize_cb_t cb, void *user_data)
{
	struct heaptrace_resize_info info;
	int start;
	int count;

	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	start = (heaptrace_resize_head - heaptrace_resize_count +
		 HEAPTRACE_MAX_RESIZE_RECORDS) %
		HEAPTRACE_MAX_RESIZE_RECORDS;
	count = heaptrace_resize_count;
	k_spin_unlock(&heaptrace_lock, key);

	for (int i = 0; i < count; i++) {
		int idx;

		key = k_spin_lock(&heaptrace_lock);
		idx = (start + i) % HEAPTRACE_MAX_RESIZE_RECORDS;
		info.heap_id = heaptrace_resizes[idx].heap_id;
		info.old_end = heaptrace_resizes[idx].old_end;
		info.new_end = heaptrace_resizes[idx].new_end;
		info.tid = heaptrace_resizes[idx].tid;
		memcpy(info.thread_name, heaptrace_resizes[idx].thread_name,
		       sizeof(info.thread_name));
		info.time_ms = heaptrace_resizes[idx].time_ms;
		info.cpu = heaptrace_resizes[idx].cpu;
		k_spin_unlock(&heaptrace_lock, key);

		if (!cb(&info, user_data)) {
			return;
		}
	}
}

void heaptrace_foreach_heap(heaptrace_heap_cb_t cb, void *user_data)
{
	struct heaptrace_heap_info info;

	for (int i = 0; i < HEAPTRACE_MAX_HEAPS; i++) {
		k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

		if (!heaptrace_heaps[i].valid) {
			k_spin_unlock(&heaptrace_lock, key);
			continue;
		}

		info.heap_id = heaptrace_heaps[i].heap_id;
		info.heap_start = heaptrace_heaps[i].heap_start;
		info.heap_end = heaptrace_heaps[i].heap_end;
		info.valid = true;

		k_spin_unlock(&heaptrace_lock, key);

		if (!cb(&info, user_data)) {
			return;
		}
	}
}

#ifdef CONFIG_NEWLIB_LIBC_HEAP_LISTENER
static void heaptrace_heap_resize_cb(uintptr_t heap_id, void *old_heap_end, void *new_heap_end)
{
	heaptrace_resize(heap_id, old_heap_end, new_heap_end);
}

HEAP_LISTENER_RESIZE_DEFINE(heaptrace_libc_resize_listener, HEAP_ID_LIBC, heaptrace_heap_resize_cb);
#endif

#if defined(CONFIG_SYS_HEAP_LISTENER) && (CONFIG_HEAP_MEM_POOL_SIZE > 0)
extern struct sys_heap _system_heap;

static void heaptrace_heap_alloc_cb(uintptr_t heap_id, void *mem, size_t bytes)
{
	heaptrace_alloc(mem, bytes, heap_id);
}

static void heaptrace_heap_free_cb(uintptr_t heap_id, void *mem, size_t bytes)
{
	heaptrace_free(mem, bytes, heap_id);
}

HEAP_LISTENER_ALLOC_DEFINE(heaptrace_sysheap_alloc_listener, HEAP_ID_FROM_POINTER(&_system_heap),
			   heaptrace_heap_alloc_cb);
HEAP_LISTENER_FREE_DEFINE(heaptrace_sysheap_free_listener, HEAP_ID_FROM_POINTER(&_system_heap),
			  heaptrace_heap_free_cb);
#endif

int heaptrace_init(void)
{
	k_spinlock_key_t key = k_spin_lock(&heaptrace_lock);

	memset(heaptrace_records, 0, sizeof(heaptrace_records));
	memset(heaptrace_resizes, 0, sizeof(heaptrace_resizes));
	memset(heaptrace_heaps, 0, sizeof(heaptrace_heaps));
	heaptrace_resize_head = 0;
	heaptrace_resize_count = 0;
	memset(&heaptrace_filter, 0, sizeof(heaptrace_filter));
	k_spin_unlock(&heaptrace_lock, key);

#if defined(CONFIG_NEWLIB_LIBC_HEAP_LISTENER)
	heap_listener_register(&heaptrace_libc_resize_listener);
#endif
#if defined(CONFIG_SYS_HEAP_LISTENER) && (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	heap_listener_register(&heaptrace_sysheap_alloc_listener);
	heap_listener_register(&heaptrace_sysheap_free_listener);
#endif
	return 0;
}

SYS_INIT(heaptrace_init, PRE_KERNEL_1, 0);
