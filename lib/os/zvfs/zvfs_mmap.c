/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Memory mappings of file descriptors and anonymous memory.
 *
 * A mapping owns page-granular storage: anonymously mapped pages when there
 * is an MMU, a block of the kernel heap otherwise. A file-backed mapping is
 * filled from the file when it is created and, when shared and writable,
 * written back on zvfs_msync(), when it is unmapped, and when its owner
 * exits. The mapping holds a reference on the open file object, counted by
 * zvfs_fd_obj_refcount(), so the object outlives its descriptors until the
 * mapping goes. An object that owns its pages (ZFD_IOCTL_MMAP answers with
 * their address) is mapped in place instead, so every mapping of it aliases
 * the same memory.
 *
 * User threads reach a mapping through a memory-domain partition on its
 * pages; the kernel view carries the protection of the pages it owns.
 *
 * A mapping belongs to the thread that created it, or to that thread's
 * process with CONFIG_PROCESS, and is released when the owner exits: in the
 * owner's own context when a process exits, otherwise by the next mapping
 * operation. A process cloned with its address space (CONFIG_PROCESS_VM)
 * gets a copy of each of its parent's mappings that leans on the original
 * for its pages; the last copy to go releases an original its owner has
 * already unmapped.
 *
 * With CONFIG_ZVFS_MMAP_DIRTY_TRACK the pages of a shared, writable file
 * mapping start write-protected. The first write to a page faults into
 * z_mem_write_fault(), which records the page as modified and makes it
 * writable; zvfs_msync() writes back the modified pages only and protects
 * them again.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#include <kernel_internal.h>
#include <mmu.h>

#include <kernel_arch_interface.h>

#define MMAP_PAGE       CONFIG_ZVFS_MMAP_PAGE_SIZE
#define MMAP_PROT_MASK  (ZVFS_PROT_READ | ZVFS_PROT_WRITE | ZVFS_PROT_EXEC)
#define MMAP_FLAGS_MASK (ZVFS_MAP_SHARED | ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS)
#define MMAP_MS_MASK    (ZVFS_MS_ASYNC | ZVFS_MS_INVALIDATE | ZVFS_MS_SYNC)

struct zvfs_mapping {
	sys_dnode_t node;
	/* page-aligned base and page-rounded length of the mapped range */
	uint8_t *addr;
	size_t len;
	/* file offset of the first page */
	size_t off;
	/* storage the mapping owns; NULL when the file object owns the pages */
	void *mem;
	size_t mem_size;
	/* the open file object, referenced until release; NULL for anonymous mappings */
	void *obj;
	const struct fd_op_vtable *vtable;
	/* whose exit releases the mapping; NULL once it is gone */
#ifdef CONFIG_PROCESS
	struct k_process *owner;
#else
	k_tid_t owner;
#endif
	/* the owner exited: released by the next mapping operation */
	bool orphan;
	/* the mapping whose storage this one leans on: a forked copy's or a split piece's original */
	struct zvfs_mapping *origin;
	/* forked copies and split pieces leaning on this mapping's storage */
	int refs;
	/* the remainder of a partially unmapped mapping: its own range, the original's storage */
	bool piece;
#ifdef CONFIG_USERSPACE
	struct k_mem_domain *domain;
	struct k_mem_partition part;
	bool granted;
#endif
	/* a shared file mapping that has been writable: written back on release */
	bool dirty;
#ifdef CONFIG_ZVFS_MMAP_DIRTY_TRACK
	/* one bit per modified page; NULL when the whole mapping is written back */
	unsigned long *dirty_map;
#endif
	/* the file was opened for writing */
	bool writable;
	uint8_t prot;
	uint8_t flags;
};

static struct zvfs_mapping mapping_storage[CONFIG_ZVFS_MMAP_MAX];
SYS_ELASTIPOOL_DEFINE_ADVANCED(mapping_pool, sizeof(struct zvfs_mapping),
			       __alignof(struct zvfs_mapping), CONFIG_ZVFS_MMAP_MAX,
			       CONFIG_ZVFS_MMAP_MAX, NULL, mapping_storage, static);

/* serializes mapping operations, including the file I/O they perform */
static K_MUTEX_DEFINE(mmap_lock);
/*
 * Guards the list: the exit paths read it under this lock alone, so only a
 * holder of mmap_lock may add or remove records, and does so under both.
 */
static struct k_spinlock list_lock;
static sys_dlist_t mappings = SYS_DLIST_STATIC_INIT(&mappings);

#ifdef CONFIG_PROCESS
static inline struct k_process *owner_self(void)
{
	return k_current_get()->process;
}
#else
static inline k_tid_t owner_self(void)
{
	return k_current_get();
}
#endif /* CONFIG_PROCESS */

/* with processes, an operation reaches the caller's own mappings only */
static inline bool mapping_mine(const struct zvfs_mapping *m)
{
	return !IS_ENABLED(CONFIG_PROCESS) || (m->owner == owner_self());
}

/* the mapping that holds the pages: a forked copy's original, else itself */
static inline struct zvfs_mapping *mapping_root(struct zvfs_mapping *m)
{
	return (m->origin != NULL) ? m->origin : m;
}

static inline bool page_aligned(uintptr_t v)
{
	return (v & (MMAP_PAGE - 1)) == 0;
}

static bool range_end(uintptr_t start, size_t len, uintptr_t *end)
{
	size_t rounded;

	if (size_add_overflow(len, MMAP_PAGE - 1, &rounded)) {
		return false;
	}
	rounded &= ~(size_t)(MMAP_PAGE - 1);

	if (rounded > UINTPTR_MAX - start) {
		return false;
	}
	*end = start + rounded;

	return true;
}

static bool valid_prot(int prot)
{
	return (prot & ~MMAP_PROT_MASK) == 0;
}

static bool supported_prot(int prot)
{
	/* writable and executable pages are never granted */
	return (prot & (ZVFS_PROT_WRITE | ZVFS_PROT_EXEC)) != (ZVFS_PROT_WRITE | ZVFS_PROT_EXEC);
}

#ifdef CONFIG_MMU
static uint32_t prot_to_kernel_flags(int prot)
{
	uint32_t flags = 0;

	if ((prot & ZVFS_PROT_WRITE) != 0) {
		flags |= K_MEM_PERM_RW;
	}
	if ((prot & ZVFS_PROT_EXEC) != 0) {
		flags |= K_MEM_PERM_EXEC;
	}

	return flags;
}
#endif /* CONFIG_MMU */

/* protection of the pages the mapping owns, in the kernel's view */
static void mapping_kernel_prot(struct zvfs_mapping *m, int prot)
{
#ifdef CONFIG_MMU
	if (mapping_root(m)->mem != NULL) {
		(void)k_mem_update_flags(m->addr, m->len, prot_to_kernel_flags(prot));
	}
#else
	ARG_UNUSED(m);
	ARG_UNUSED(prot);
#endif /* CONFIG_MMU */
}

#ifdef CONFIG_USERSPACE
static k_mem_partition_attr_t prot_to_attr(int prot)
{
	if ((prot & ZVFS_PROT_WRITE) != 0) {
		return K_MEM_PARTITION_P_RW_U_RW;
	}
#ifdef K_MEM_PARTITION_P_RX_U_RX
	if ((prot & ZVFS_PROT_EXEC) != 0) {
		return K_MEM_PARTITION_P_RX_U_RX;
	}
#endif
	return K_MEM_PARTITION_P_RO_U_RO;
}

static struct k_mem_domain *caller_domain(void)
{
	if (!k_is_in_user_syscall()) {
		return NULL;
	}

	return k_current_get()->mem_domain_info.mem_domain;
}

/* a domain programmed at context switch would not cover the caller until it is switched */
static void domain_apply_current(struct k_mem_domain *domain)
{
#ifdef CONFIG_ARCH_MEM_DOMAIN_APPLY_CURRENT
	if (k_current_get()->mem_domain_info.mem_domain == domain) {
		arch_mem_domain_apply_current();
	}
#else
	ARG_UNUSED(domain);
#endif
}

/*
 * Mappings of an object mapped in place share its address, and a domain holds
 * one partition per range: the first such mapping in a domain owns the
 * partition, later ones lean on it, and the owner hands it on when it goes.
 */
static struct zvfs_mapping *alias_in_domain(const struct zvfs_mapping *m, bool granted)
{
	struct zvfs_mapping *other;
	struct zvfs_mapping *found = NULL;

	K_SPINLOCK(&list_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER(&mappings, other, node) {
			if ((other != m) && (other->owner != NULL) &&
			    (other->domain == m->domain) && (other->addr == m->addr) &&
			    (other->len == m->len) && (other->granted == granted)) {
				found = other;
				break;
			}
		}
	}

	return found;
}

static int mapping_user_grant(struct zvfs_mapping *m, int prot)
{
	int ret;

	if ((m->domain == NULL) || (prot == ZVFS_PROT_NONE)) {
		return 0;
	}

	m->part = (struct k_mem_partition){
		.start = (uintptr_t)m->addr,
		/* a memory-protection region covers the whole (power-of-two) storage */
		.size = ((m->mem != NULL) && !IS_ENABLED(CONFIG_MMU)) ? m->mem_size : m->len,
		.attr = prot_to_attr(prot),
	};
	if ((m->mem == NULL) && (alias_in_domain(m, true) != NULL)) {
		/* an alias already holds the partition on these pages */
		return 0;
	}
	ret = k_mem_domain_add_partition(m->domain, &m->part);
	if (ret != 0) {
		return ret;
	}
	m->granted = true;
	domain_apply_current(m->domain);

	return 0;
}

static void mapping_user_revoke(struct zvfs_mapping *m)
{
	struct zvfs_mapping *heir;

	if (!m->granted) {
		return;
	}
	m->granted = false;

	heir = (m->mem == NULL) ? alias_in_domain(m, false) : NULL;
	if (heir != NULL) {
		/* the partition stays for the alias, which now owns it */
		heir->part = m->part;
		heir->granted = true;
		return;
	}

	(void)k_mem_domain_remove_partition(m->domain, &m->part);
	domain_apply_current(m->domain);
}
#else
static inline int mapping_user_grant(struct zvfs_mapping *m, int prot)
{
	ARG_UNUSED(m);
	ARG_UNUSED(prot);

	return 0;
}

static inline void mapping_user_revoke(struct zvfs_mapping *m)
{
	ARG_UNUSED(m);
}
#endif /* CONFIG_USERSPACE */

static inline bool mapping_granted(const struct zvfs_mapping *m)
{
#ifdef CONFIG_USERSPACE
	return m->granted;
#else
	ARG_UNUSED(m);
	return false;
#endif
}

#ifdef CONFIG_ZVFS_MMAP_DIRTY_TRACK
static inline bool mapping_tracked(const struct zvfs_mapping *m)
{
	return m->dirty_map != NULL;
}

/* the protection the pages carry: tracked pages start write-protected */
static inline int mapping_page_prot(const struct zvfs_mapping *m, int prot)
{
	return mapping_tracked(m) ? (prot & ~ZVFS_PROT_WRITE) : prot;
}

static void mapping_track_init(struct zvfs_mapping *m)
{
	if (m->dirty && (m->mem != NULL)) {
		/* without a map the whole mapping is written back */
		m->dirty_map = k_calloc(BITS_TO_LONGS(m->len / MMAP_PAGE), sizeof(unsigned long));
	}
}

static void mapping_track_free(struct zvfs_mapping *m)
{
	k_free(m->dirty_map);
	m->dirty_map = NULL;
}

static inline bool page_dirty(const struct zvfs_mapping *m, size_t idx)
{
	return (m->dirty_map[idx / BITS_PER_LONG] & BIT(idx % BITS_PER_LONG)) != 0;
}

static inline void page_dirty_set(struct zvfs_mapping *m, size_t idx, bool dirty)
{
	if (dirty) {
		m->dirty_map[idx / BITS_PER_LONG] |= BIT(idx % BITS_PER_LONG);
	} else {
		m->dirty_map[idx / BITS_PER_LONG] &= ~BIT(idx % BITS_PER_LONG);
	}
}

/* change one page's protection in the kernel view and in the granted domain */
static void page_set_prot(struct zvfs_mapping *m, size_t idx, int prot)
{
	uint8_t *page = m->addr + idx * MMAP_PAGE;
#ifdef CONFIG_PROCESS_VM
	struct zvfs_mapping *c;
#endif

	(void)k_mem_update_flags(page, MMAP_PAGE, prot_to_kernel_flags(prot));
#ifdef CONFIG_USERSPACE
	if (m->granted) {
		(void)arch_mem_domain_page_attr_set(m->domain, page, MMAP_PAGE, prot_to_attr(prot));
	}
#endif
#ifdef CONFIG_PROCESS_VM
	/* forked copies see the page through their own domains */
	SYS_DLIST_FOR_EACH_CONTAINER(&mappings, c, node) {
		if ((c->origin == m) && c->granted) {
			(void)arch_mem_domain_page_attr_set(c->domain, page, MMAP_PAGE,
							    prot_to_attr(prot));
		}
	}
#endif
}

bool z_mem_write_fault(void *addr)
{
	uintptr_t a = (uintptr_t)addr;
	struct zvfs_mapping *m;
	bool handled = false;

	K_SPINLOCK(&list_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
			if (!mapping_tracked(m) || (a < (uintptr_t)m->addr) ||
			    (a >= (uintptr_t)m->addr + m->len)) {
				continue;
			}
			if ((m->prot & ZVFS_PROT_WRITE) == 0) {
				/* a genuine protection violation */
				break;
			}

			size_t idx = (a - (uintptr_t)m->addr) / MMAP_PAGE;

			page_dirty_set(m, idx, true);
			page_set_prot(m, idx, m->prot);
			handled = true;
			break;
		}
	}

	return handled;
}
#else
static inline bool mapping_tracked(const struct zvfs_mapping *m)
{
	ARG_UNUSED(m);

	return false;
}

static inline int mapping_page_prot(const struct zvfs_mapping *m, int prot)
{
	ARG_UNUSED(m);

	return prot;
}

static inline void mapping_track_init(struct zvfs_mapping *m)
{
	ARG_UNUSED(m);
}

static inline void mapping_track_free(struct zvfs_mapping *m)
{
	ARG_UNUSED(m);
}
#endif /* CONFIG_ZVFS_MMAP_DIRTY_TRACK */

static int storage_alloc(struct zvfs_mapping *m)
{
#ifdef CONFIG_MMU
	/* pinned: a mapping's pages are present in every domain that maps them */
	m->mem_size = m->len;
	m->mem = k_mem_map(m->len, K_MEM_PERM_RW | K_MEM_MAP_LOCK);
#else
	size_t align = MMAP_PAGE;

	m->mem_size = m->len;
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		/* a memory-protection region: a power of two, aligned to its size */
		m->mem_size = BIT(32 - u32_count_leading_zeros(m->len - 1));
		align = m->mem_size;
	}
	m->mem = k_aligned_alloc(align, m->mem_size);
	if (m->mem != NULL) {
		memset(m->mem, 0, m->mem_size);
	}
#endif /* CONFIG_MMU */
	if (m->mem == NULL) {
		return -ENOMEM;
	}
	m->addr = m->mem;

	return 0;
}

/* the whole storage writable again, whatever its pieces were protected as */
static void storage_kernel_prot_rw(struct zvfs_mapping *m)
{
#ifdef CONFIG_MMU
	(void)k_mem_update_flags(m->mem, m->mem_size, prot_to_kernel_flags(ZVFS_PROT_WRITE));
#else
	ARG_UNUSED(m);
#endif /* CONFIG_MMU */
}

static void storage_free(struct zvfs_mapping *m)
{
	if (m->mem == NULL) {
		return;
	}
#ifdef CONFIG_MMU
	k_mem_unmap(m->mem, m->mem_size);
#else
	k_free(m->mem);
#endif /* CONFIG_MMU */
	m->mem = NULL;
}

/* read [start, end) of the mapping from the file; bytes past EOF read as zero */
static int mapping_fill(struct zvfs_mapping *m, size_t start, size_t end)
{
	size_t pos = start;

	while (pos < end) {
		ssize_t n = m->vtable->read_offs(m->obj, m->addr + pos, end - pos, m->off + pos);

		if (n < 0) {
			return -1;
		}
		if (n == 0) {
			memset(m->addr + pos, 0, end - pos);
			break;
		}
		pos += n;
	}

	return 0;
}

/* re-read [start, end) of a mapping whose pages may be write-protected */
static int mapping_refill(struct zvfs_mapping *m, size_t start, size_t end)
{
	int ret;

	if ((m->obj == NULL) || (mapping_root(m)->mem == NULL)) {
		return 0;
	}

	mapping_user_revoke(m);
	mapping_kernel_prot(m, ZVFS_PROT_WRITE);
	ret = mapping_fill(m, start, end);
	mapping_kernel_prot(m, mapping_page_prot(m, m->prot));
	if (mapping_user_grant(m, mapping_page_prot(m, m->prot)) != 0) {
		errno = ENOMEM;
		ret = -1;
	}

	return ret;
}

static int mapping_write_range(struct zvfs_mapping *m, size_t pos, size_t end)
{
	while (pos < end) {
		ssize_t n = m->vtable->write_offs(m->obj, m->addr + pos, end - pos, m->off + pos);

		if (n < 0) {
			return -1;
		}
		if (n == 0) {
			errno = EIO;
			return -1;
		}
		pos += n;
	}

	return 0;
}

/* write [start, end) of a dirty shared mapping back, within the file's size */
static int mapping_writeback(struct zvfs_mapping *m, size_t start, size_t end)
{
	struct zvfs_stat st;

	if (!m->dirty || (mapping_root(m)->mem == NULL)) {
		return 0;
	}

	if (zvfs_fdtable_call_ioctl(m->vtable, m->obj, ZFD_IOCTL_STAT, &st) < 0) {
		return -1;
	}
	if ((st.size < 0) || (m->off + start >= (size_t)st.size)) {
		return 0;
	}
	end = MIN(end, (size_t)st.size - m->off);

	if (!mapping_tracked(m)) {
		return mapping_write_range(m, start, end);
	}

#ifdef CONFIG_ZVFS_MMAP_DIRTY_TRACK
	/* modified pages only; each one is protected again so a later write re-marks it */
	for (size_t idx = start / MMAP_PAGE; idx * MMAP_PAGE < end; idx++) {
		size_t ps = idx * MMAP_PAGE;

		if (!page_dirty(m, idx)) {
			continue;
		}
		if (mapping_write_range(m, MAX(ps, start), MIN(ps + MMAP_PAGE, end)) < 0) {
			return -1;
		}
		K_SPINLOCK(&list_lock) {
			page_dirty_set(m, idx, false);
			page_set_prot(m, idx, mapping_page_prot(m, m->prot));
		}
	}
#endif /* CONFIG_ZVFS_MMAP_DIRTY_TRACK */

	return 0;
}

/*
 * A mapping counts as a reference on its file object: zvfs_fd_obj_refcount()
 * adds the mappings of the object to its descriptors, so closing the last
 * descriptor leaves the object open until the last mapping releases it.
 */
int z_zvfs_mmap_obj_refs(const void *obj)
{
	struct zvfs_mapping *m;
	int refs = 0;

	K_SPINLOCK(&list_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
			refs += (m->obj == obj);
		}
	}

	return refs;
}

/* the mapping is off both lists: close the object if no descriptor holds it */
static void mapping_obj_unref(struct zvfs_mapping *m)
{
	if (m->obj == NULL) {
		return;
	}
	if ((zvfs_fd_obj_refcount(m->obj) == 0) && (z_zvfs_mmap_obj_refs(m->obj) == 0) &&
	    (m->vtable->close != NULL)) {
		(void)m->vtable->close(m->obj);
	}
	m->obj = NULL;
}

static void mapping_unlink(struct zvfs_mapping *m)
{
	K_SPINLOCK(&list_lock) {
		sys_dlist_remove(&m->node);
	}
}

static void mapping_link(struct zvfs_mapping *m)
{
	K_SPINLOCK(&list_lock) {
		sys_dlist_append(&mappings, &m->node);
	}
}

/* tear a mapping down: write back, revoke, release storage and the file */
static int mapping_release(struct zvfs_mapping *m)
{
	int ret = 0;

	if (m->origin != NULL) {
		struct zvfs_mapping *origin = m->origin;

		/* a forked copy or a piece: the last one out releases an original already unmapped */
		if (m->piece) {
			ret = mapping_writeback(m, 0, m->len);
		}
		mapping_user_revoke(m);
		mapping_obj_unref(m);
		(void)sys_elastipool_free(&mapping_pool, m);
		origin->refs--;
		if ((origin->refs == 0) && (origin->owner == NULL)) {
			mapping_unlink(origin);
			if (mapping_release(origin) < 0) {
				ret = -1;
			}
		}

		return ret;
	}

	ret = mapping_writeback(m, 0, m->len);
	mapping_user_revoke(m);
	if (m->refs > 0) {
		/* copies or pieces still lean on the storage: parked until the last one goes */
		m->owner = NULL;
		m->orphan = false;
		mapping_link(m);

		return ret;
	}
	if (m->mem != NULL) {
		storage_kernel_prot_rw(m);
		storage_free(m);
	}
	mapping_track_free(m);
	mapping_obj_unref(m);
	(void)sys_elastipool_free(&mapping_pool, m);

	return ret;
}

/* release every mapping @a take selects, off the list first; mmap_lock held */
static void mappings_release(bool (*take)(const struct zvfs_mapping *m, const void *arg),
			     const void *arg)
{
	struct zvfs_mapping *m, *next;
	sys_dlist_t taken;

	sys_dlist_init(&taken);
	K_SPINLOCK(&list_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&mappings, m, next, node) {
			if (take(m, arg)) {
				sys_dlist_remove(&m->node);
				sys_dlist_append(&taken, &m->node);
			}
		}
	}

	while (!sys_dlist_is_empty(&taken)) {
		m = CONTAINER_OF(sys_dlist_get(&taken), struct zvfs_mapping, node);
		(void)mapping_release(m);
	}
}

static bool take_orphan(const struct zvfs_mapping *m, const void *arg)
{
	ARG_UNUSED(arg);

	return m->orphan;
}

/* mappings whose owner exited are released by the next mapping operation */
static void orphans_release(void)
{
	mappings_release(take_orphan, NULL);
}

static bool take_owned(const struct zvfs_mapping *m, const void *arg)
{
	return (m->owner != NULL) && ((const void *)m->owner == arg);
}

/* the exit paths only mark: they run under locks the release cannot take */
static void mappings_orphan(const void *owner, const struct k_mem_domain *dying)
{
	struct zvfs_mapping *m;

#ifndef CONFIG_USERSPACE
	ARG_UNUSED(dying);
#endif
	K_SPINLOCK(&list_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
			if (!take_owned(m, owner)) {
				continue;
			}
#ifdef CONFIG_USERSPACE
			if ((dying != NULL) && (m->domain == dying)) {
				/* the domain goes with its process: nothing left to revoke */
				m->granted = false;
			}
#endif
			m->owner = NULL;
			m->orphan = true;
		}
	}
}

#ifdef CONFIG_PROCESS
void z_process_mmap_exit(void)
{
	(void)k_mutex_lock(&mmap_lock, K_FOREVER);
	mappings_release(take_owned, owner_self());
	k_mutex_unlock(&mmap_lock);
}

void z_process_mmap_drop(struct k_process *proc)
{
	const struct k_mem_domain *dying = NULL;

#ifdef CONFIG_PROCESS_VM
	if (proc->vm != NULL) {
		dying = z_process_vm_domain(proc->vm);
	}
#endif /* CONFIG_PROCESS_VM */
	mappings_orphan(proc, dying);
}
#else
void z_zvfs_mmap_thread_exit(struct k_thread *thread)
{
	mappings_orphan(thread, NULL);
}
#endif /* CONFIG_PROCESS */

#ifdef CONFIG_PROCESS_VM
bool z_zvfs_mmap_is_shared(uintptr_t start)
{
	struct zvfs_mapping *m;
	bool shared = false;

	K_SPINLOCK(&list_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
			if (mapping_mine(m) && ((uintptr_t)m->addr == start) &&
			    ((m->flags & ZVFS_MAP_SHARED) != 0)) {
				shared = true;
				break;
			}
		}
	}

	return shared;
}

static bool take_owned_copy(const struct zvfs_mapping *m, const void *arg)
{
	return take_owned(m, arg) && (m->origin != NULL);
}

int z_zvfs_mmap_clone(struct k_process *child, struct k_mem_domain *domain)
{
	struct k_mem_domain *parent_domain = k_current_get()->mem_domain_info.mem_domain;
	struct zvfs_mapping *m;
	sys_dlist_t copies;
	int ret = 0;

	sys_dlist_init(&copies);
	(void)k_mutex_lock(&mmap_lock, K_FOREVER);
	orphans_release();

	SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
		struct zvfs_mapping *c;
		void *obj;

		if (!mapping_mine(m)) {
			continue;
		}
		if (sys_elastipool_alloc(&mapping_pool, &obj) != 0) {
			ret = -ENOMEM;
			break;
		}
		c = obj;
		/* the same range in the child's domain, whose partitions mirror the parent's */
		*c = *m;
		sys_dnode_init(&c->node);
		c->owner = child;
		c->mem = NULL;
		c->mem_size = 0;
		c->refs = 0;
		c->piece = false;
		c->origin = mapping_root(m);
		c->origin->refs++;
#ifdef CONFIG_ZVFS_MMAP_DIRTY_TRACK
		c->dirty_map = NULL;
#endif
		c->granted = m->granted && (m->domain == parent_domain);
		c->domain = c->granted ? domain : NULL;
		sys_dlist_append(&copies, &c->node);
	}

	K_SPINLOCK(&list_lock) {
		while (!sys_dlist_is_empty(&copies)) {
			sys_dlist_append(&mappings, sys_dlist_get(&copies));
		}
	}
	if (ret != 0) {
		mappings_release(take_owned_copy, child);
	}
	k_mutex_unlock(&mmap_lock);

	return ret;
}
#endif /* CONFIG_PROCESS_VM */

static bool mapping_overlap(const struct zvfs_mapping *m, uintptr_t start, uintptr_t end,
			    size_t *lo, size_t *hi)
{
	uintptr_t mstart = (uintptr_t)m->addr;
	uintptr_t mend = mstart + m->len;

	if ((end <= mstart) || (start >= mend)) {
		return false;
	}
	*lo = MAX(start, mstart) - mstart;
	*hi = MIN(end, mend) - mstart;

	return true;
}

static int file_access_mode(int fd)
{
	int flags = zvfs_fcntl(fd, ZVFS_F_GETFL, 0);

	/* an object that does not report its flags is treated as read/write */
	return (flags < 0) ? ZVFS_O_RDWR : (flags & ZVFS_O_ACCMODE);
}

#ifdef CONFIG_ZVFS_MLOCK
#define MLOCK_FLAGS_MASK (ZVFS_MLOCK_CURRENT | ZVFS_MLOCK_FUTURE)

static inline bool mlock_future(void)
{
	return *z_zvfs_proc_mlock_future();
}

#ifdef CONFIG_DEMAND_PAGING
/* a mapping's pages are locked for its lifetime: an unlock passes over them */
static bool mapped_page(uintptr_t page)
{
	struct zvfs_mapping *m;

	SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
		if (IN_RANGE(page, (uintptr_t)m->addr, (uintptr_t)m->addr + m->len - 1)) {
			return true;
		}
	}

	return false;
}

/* every page of [start, end) is one the pager manages: k_mem_pin() panics on any other */
static bool pages_paged(uintptr_t start, uintptr_t end)
{
	for (uintptr_t page = start; page < end; page += MMAP_PAGE) {
		uintptr_t phys;

		if ((arch_page_info_get((void *)page, &phys, false) & ARCH_DATA_PAGE_NOT_MAPPED) !=
		    0) {
			return false;
		}
	}

	return true;
}

static void pages_lock(uintptr_t start, uintptr_t end, bool lock)
{
	uintptr_t run = start;

	if (lock) {
		k_mem_pin((void *)start, end - start);
		return;
	}
	for (uintptr_t page = start; page < end; page += MMAP_PAGE) {
		if (!mapped_page(page)) {
			continue;
		}
		if (run < page) {
			k_mem_unpin((void *)run, page - run);
		}
		run = page + MMAP_PAGE;
	}
	if (run < end) {
		k_mem_unpin((void *)run, end - run);
	}
}

/* pages of memory: the pager cannot pin what lies outside */
static bool pages_in_memory(uintptr_t start, uintptr_t end)
{
	const uintptr_t lo = (uintptr_t)K_MEM_VIRT_RAM_START;
	const uintptr_t hi = (uintptr_t)K_MEM_VIRT_RAM_END - 1;

	return IN_RANGE(start, lo, hi) && IN_RANGE(end - 1, lo, hi);
}

static int range_lock(const void *addr, size_t len, bool lock)
{
	uintptr_t start = ROUND_DOWN((uintptr_t)addr, MMAP_PAGE);
	uintptr_t end;

	if (len == 0) {
		return 0;
	}
	if (!range_end(start, (uintptr_t)addr + len - start, &end) ||
	    !pages_in_memory(start, end) || !pages_paged(start, end)) {
		errno = ENOMEM;
		return -1;
	}
	pages_lock(start, end, lock);

	return 0;
}

/* the caller's mappings and, from user mode, the partitions of its domain */
static void current_lock(bool lock)
{
	struct zvfs_mapping *m;
#ifdef CONFIG_USERSPACE
	struct k_mem_domain *domain = caller_domain();

	if (domain != NULL) {
		for (int i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) {
			const struct k_mem_partition *part = &domain->partitions[i];
			uintptr_t start = ROUND_DOWN(part->start, MMAP_PAGE);
			uintptr_t end;

			if ((part->size == 0) ||
			    !range_end(start, part->start + part->size - start, &end) ||
			    !pages_in_memory(start, end) || !pages_paged(start, end)) {
				continue;
			}
			pages_lock(start, end, lock);
		}
	}
#endif /* CONFIG_USERSPACE */
	if (!lock) {
		return;
	}
	SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
		if (mapping_mine(m)) {
			pages_lock((uintptr_t)m->addr, (uintptr_t)m->addr + m->len, true);
		}
	}
}
#else
static void pages_lock(uintptr_t start, uintptr_t end, bool lock)
{
	ARG_UNUSED(start);
	ARG_UNUSED(end);
	ARG_UNUSED(lock);
}

static int range_lock(const void *addr, size_t len, bool lock)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(len);
	ARG_UNUSED(lock);

	return 0;
}

static void current_lock(bool lock)
{
	ARG_UNUSED(lock);
}
#endif /* CONFIG_DEMAND_PAGING */

static int mlock_op(const void *addr, size_t len, int flags, bool lock)
{
	int ret = 0;

	if (((flags & ~MLOCK_FLAGS_MASK) != 0) ||
	    ((flags != 0) && ((addr != NULL) || (len != 0))) ||
	    ((flags == 0) && Z_DETECT_POINTER_OVERFLOW(addr, len))) {
		errno = EINVAL;
		return -1;
	}

	(void)k_mutex_lock(&mmap_lock, K_FOREVER);
	orphans_release();

	if (flags == 0) {
		ret = range_lock(addr, len, lock);
	} else {
		if ((flags & ZVFS_MLOCK_CURRENT) != 0) {
			current_lock(lock);
		}
		if ((flags & ZVFS_MLOCK_FUTURE) != 0) {
			*z_zvfs_proc_mlock_future() = lock;
		}
	}

	k_mutex_unlock(&mmap_lock);

	return ret;
}

int z_impl_zvfs_mlock(const void *addr, size_t len, int flags)
{
	return mlock_op(addr, len, flags, true);
}

int z_impl_zvfs_munlock(const void *addr, size_t len, int flags)
{
	return mlock_op(addr, len, flags, false);
}
#else
static inline bool mlock_future(void)
{
	return false;
}

static void pages_lock(uintptr_t start, uintptr_t end, bool lock)
{
	ARG_UNUSED(start);
	ARG_UNUSED(end);
	ARG_UNUSED(lock);
}
#endif /* CONFIG_ZVFS_MLOCK */

int z_impl_zvfs_mmap(size_t len, int prot, int flags, int fd, size_t off, void **addr)
{
	struct zvfs_mapping *m;
	struct fd_entry *entry = NULL;
	void *obj = NULL;
	bool shared = (flags & ZVFS_MAP_SHARED) != 0;
	bool anon = (flags & ZVFS_MAP_ANONYMOUS) != 0;
	int acc = ZVFS_O_RDWR;
	uintptr_t end;
	int ret;

	if ((len == 0) || (addr == NULL) || ((flags & ~MMAP_FLAGS_MASK) != 0) ||
	    (shared == ((flags & ZVFS_MAP_PRIVATE) != 0)) || !valid_prot(prot) ||
	    !page_aligned(off) || !range_end(0, len, &end)) {
		errno = EINVAL;
		return -1;
	}
	if (!supported_prot(prot)) {
		errno = ENOTSUP;
		return -1;
	}
	if (!anon) {
		entry = zvfs_fd_entry_get(fd);
		if (entry == NULL) {
			return -1;
		}
		if (!supports_pread_pwrite(entry->mode)) {
			errno = ENODEV;
			return -1;
		}
		acc = file_access_mode(fd);
		if ((acc == ZVFS_O_WRONLY) ||
		    ((acc == ZVFS_O_RDONLY) && shared && ((prot & ZVFS_PROT_WRITE) != 0))) {
			errno = EACCES;
			return -1;
		}
		if (size_add_overflow(off, (size_t)end, &off)) {
			errno = EOVERFLOW;
			return -1;
		}
		off -= (size_t)end;
	}

	(void)k_mutex_lock(&mmap_lock, K_FOREVER);
	orphans_release();

	if (sys_elastipool_alloc(&mapping_pool, &obj) != 0) {
		errno = ENOMEM;
		ret = -1;
		goto unlock;
	}
	m = obj;
	*m = (struct zvfs_mapping){
		.len = (size_t)end,
		.off = off,
		.owner = owner_self(),
		.prot = prot,
		.flags = flags,
		.writable = acc != ZVFS_O_RDONLY,
	};
#ifdef CONFIG_USERSPACE
	m->domain = caller_domain();
#endif

	if (!anon) {
		void *base = NULL;

		/* the object stays open until the mapping is released */
		m->obj = entry->obj;
		m->vtable = entry->vtable;

		/* an object that owns its pages is mapped in place */
		if (zvfs_fdtable_call_ioctl(m->vtable, m->obj, ZFD_IOCTL_MMAP, off, m->len,
					    &base) == 0) {
			m->addr = base;
		} else if ((errno != EOPNOTSUPP) && (errno != ENOTSUP)) {
			ret = -1;
			goto free;
		}
	}

	m->dirty = !anon && shared && ((prot & ZVFS_PROT_WRITE) != 0);

	if (m->addr == NULL) {
		ret = storage_alloc(m);
		if (ret != 0) {
			errno = ENOMEM;
			goto free;
		}
		if (!anon && (mapping_fill(m, 0, m->len) < 0)) {
			ret = -1;
			goto free;
		}
		mapping_track_init(m);
		mapping_kernel_prot(m, mapping_page_prot(m, prot));
	}

	ret = mapping_user_grant(m, mapping_page_prot(m, prot));
	if (ret != 0) {
		errno = ENOMEM;
		goto free;
	}

	mapping_link(m);
	if (mlock_future()) {
		pages_lock((uintptr_t)m->addr, (uintptr_t)m->addr + m->len, true);
	}
	*addr = m->addr;
	k_mutex_unlock(&mmap_lock);

	return 0;

free:
	if (m->mem != NULL) {
		storage_kernel_prot_rw(m);
		storage_free(m);
	}
	mapping_track_free(m);
	/* the caller's descriptor still holds the object: nothing to unreference */
	(void)sys_elastipool_free(&mapping_pool, m);
	ret = -1;
unlock:
	k_mutex_unlock(&mmap_lock);

	return ret;
}

/* forked copies lean on the shape of their original as much as on its pages */
static bool mapping_has_copies(const struct zvfs_mapping *root)
{
	struct zvfs_mapping *c;
	bool found = false;

	K_SPINLOCK(&list_lock) {
		SYS_DLIST_FOR_EACH_CONTAINER(&mappings, c, node) {
			if ((c->origin == root) && !c->piece) {
				found = true;
				break;
			}
		}
	}

	return found;
}

/*
 * Unmap [lo, hi) of a mapping: trim it, or split it into two pieces around the hole.
 * The storage stays whole and is released with the last piece; the hole loses its
 * kernel-mode protection and its user-mode grant.
 */
/* a granted mapping split in two needs a second partition in its domain */
static bool mapping_piece_room(const struct zvfs_mapping *m)
{
#ifdef CONFIG_USERSPACE
	if (mapping_granted(m)) {
		return m->domain->num_partitions < CONFIG_MAX_DOMAIN_PARTITIONS;
	}
#else
	ARG_UNUSED(m);
#endif /* CONFIG_USERSPACE */
	return true;
}

static int mapping_unmap_part(struct zvfs_mapping *m, size_t lo, size_t hi)
{
	struct zvfs_mapping *root = mapping_root(m);
	struct zvfs_mapping *n = NULL;
	void *obj;
	int ret;

	if (((m->origin != NULL) && !m->piece) || mapping_has_copies(root)) {
		/* a forked copy, or an original with forked copies: the shape is shared */
		return -EINVAL;
	}
	if (!IS_ENABLED(CONFIG_MMU) && mapping_granted(m)) {
		/* a memory-protection region is granted whole */
		return -EINVAL;
	}
	if ((lo > 0) && (hi < m->len)) {
		/* a hole in the middle: the part after it becomes a piece of its own */
		if (!mapping_piece_room(m) || (sys_elastipool_alloc(&mapping_pool, &obj) != 0)) {
			return -ENOMEM;
		}
		n = obj;
		*n = *m;
		sys_dnode_init(&n->node);
		n->addr = m->addr + hi;
		n->off = m->off + hi;
		n->len = m->len - hi;
		n->mem = NULL;
		n->mem_size = 0;
		n->refs = 0;
		n->origin = root;
		n->piece = true;
#ifdef CONFIG_USERSPACE
		n->granted = false;
#endif
#ifdef CONFIG_ZVFS_MMAP_DIRTY_TRACK
		n->dirty_map = NULL;
#endif
	}
	/* the hole's modifications reach the file now; the rest is written back whole later */
	ret = (mapping_writeback(m, lo, hi) < 0) ? -EIO : 0;
	mapping_user_revoke(m);
	mapping_track_free(m);
#ifdef CONFIG_MMU
	if (root->mem != NULL) {
		(void)k_mem_update_flags(m->addr + lo, hi - lo, prot_to_kernel_flags(ZVFS_PROT_NONE));
	}
#endif /* CONFIG_MMU */
	if (lo == 0) {
		m->addr += hi;
		m->off += hi;
		m->len -= hi;
	} else {
		m->len = lo;
	}
	mapping_kernel_prot(m, m->prot);
	if (mapping_user_grant(m, m->prot) != 0) {
		ret = -ENOMEM;
	}
	if (n != NULL) {
		root->refs++;
		mapping_kernel_prot(n, n->prot);
		if (mapping_user_grant(n, n->prot) != 0) {
			ret = -ENOMEM;
		}
		mapping_link(n);
	}

	return ret;
}

int z_impl_zvfs_munmap(void *addr, size_t len)
{
	struct zvfs_mapping *m, *next;
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end;
	int ret = 0;

	if ((len == 0) || !page_aligned(start) || !range_end(start, len, &end)) {
		errno = EINVAL;
		return -1;
	}

	(void)k_mutex_lock(&mmap_lock, K_FOREVER);
	orphans_release();

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&mappings, m, next, node) {
		size_t lo, hi;

		if (!mapping_mine(m) || !mapping_overlap(m, start, end, &lo, &hi)) {
			continue;
		}
		if ((lo != 0) || (hi != m->len)) {
			int rc = mapping_unmap_part(m, lo, hi);

			if (rc < 0) {
				errno = -rc;
				ret = -1;
			}
			continue;
		}
		mapping_unlink(m);
		if (mapping_release(m) < 0) {
			ret = -1;
		}
	}

	k_mutex_unlock(&mmap_lock);

	return ret;
}

/* other mappings of the same open file hold their own copy of [lo, hi): re-read it */
static int mapping_refill_others(struct zvfs_mapping *m, size_t lo, size_t hi)
{
	struct zvfs_mapping *o;
	const size_t file_lo = m->off + lo;
	const size_t file_hi = m->off + hi;
	int ret = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(&mappings, o, node) {
		if ((o == m) || (o->obj != m->obj) || (mapping_root(o)->mem == NULL) || (o->off >= file_hi) ||
		    (o->off + o->len <= file_lo)) {
			continue;
		}
		if (mapping_refill(o, MAX(file_lo, o->off) - o->off,
				   MIN(file_hi, o->off + o->len) - o->off) < 0) {
			ret = -1;
		}
	}

	return ret;
}

int z_impl_zvfs_msync(void *addr, size_t len, int flags)
{
	struct zvfs_mapping *m;
	struct zvfs_mapping *t;
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end;
	bool found = false;
	int ret = 0;

	if (((flags & ~MMAP_MS_MASK) != 0) ||
	    ((flags & (ZVFS_MS_SYNC | ZVFS_MS_ASYNC)) == (ZVFS_MS_SYNC | ZVFS_MS_ASYNC)) ||
	    !page_aligned(start) || !range_end(start, len, &end)) {
		errno = EINVAL;
		return -1;
	}

	(void)k_mutex_lock(&mmap_lock, K_FOREVER);
	orphans_release();

	SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
		size_t lo, hi;

		if (!mapping_mine(m) || !mapping_overlap(m, start, end, &lo, &hi)) {
			continue;
		}
		found = true;
		/* a piece covers its own range; a forked copy goes through its original */
		t = m->piece ? m : mapping_root(m);
		if (((flags & (ZVFS_MS_SYNC | ZVFS_MS_ASYNC)) != 0) &&
		    (mapping_writeback(t, lo, hi) < 0)) {
			ret = -1;
		}
		/* unsynced stores are the file's newest data, as in a shared page cache */
		if (((flags & ZVFS_MS_INVALIDATE) != 0) &&
		    ((mapping_writeback(t, lo, hi) < 0) || (mapping_refill(t, lo, hi) < 0) ||
		     (mapping_refill_others(t, lo, hi) < 0))) {
			ret = -1;
		}
	}

	k_mutex_unlock(&mmap_lock);

	if (!found && (len != 0)) {
		errno = ENOMEM;
		return -1;
	}

	return ret;
}

int z_impl_zvfs_mprotect(void *addr, size_t len, int prot)
{
	struct zvfs_mapping *m;
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end;
	bool found = false;
	int ret = 0;

	if (!valid_prot(prot) || !page_aligned(start) || !range_end(start, len, &end)) {
		errno = EINVAL;
		return -1;
	}
	if (!supported_prot(prot)) {
		errno = ENOTSUP;
		return -1;
	}

	(void)k_mutex_lock(&mmap_lock, K_FOREVER);
	orphans_release();

	SYS_DLIST_FOR_EACH_CONTAINER(&mappings, m, node) {
		size_t lo, hi;
		bool shared = (m->flags & ZVFS_MAP_SHARED) != 0;

		if (!mapping_mine(m) || !mapping_overlap(m, start, end, &lo, &hi)) {
			continue;
		}
		found = true;
		if ((lo != 0) || (hi != m->len)) {
			/* part of a mapping: not supported */
			errno = ENOTSUP;
			ret = -1;
			continue;
		}
		if (((prot & ZVFS_PROT_WRITE) != 0) && shared && (m->obj != NULL) && !m->writable) {
			errno = EACCES;
			ret = -1;
			continue;
		}

		mapping_user_revoke(m);
		m->prot = prot;
		if (shared && (m->obj != NULL) && ((prot & ZVFS_PROT_WRITE) != 0)) {
			mapping_root(m)->dirty = true;
			m->dirty = true;
		}
		mapping_kernel_prot(m, mapping_page_prot(m, prot));
#ifdef CONFIG_USERSPACE
		if (m->domain == NULL) {
			m->domain = caller_domain();
		}
#endif
		if (mapping_user_grant(m, mapping_page_prot(m, prot)) != 0) {
			errno = ENOMEM;
			ret = -1;
		}
	}

	k_mutex_unlock(&mmap_lock);

	if (!found && (len != 0)) {
		errno = ENOMEM;
		return -1;
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_mmap(size_t len, int prot, int flags, int fd, size_t off, void **addr)
{
	void *base = NULL;
	int ret;

	K_OOPS(K_SYSCALL_MEMORY_WRITE(addr, sizeof(*addr)));
	if (((flags & ZVFS_MAP_ANONYMOUS) == 0) && (zvfs_fd_syscall_verify(fd) == NULL)) {
		return -1;
	}

	ret = z_impl_zvfs_mmap(len, prot, flags, fd, off, &base);
	if (ret == 0) {
		K_OOPS(k_usermode_to_copy(addr, &base, sizeof(base)));
	}

	return ret;
}
#include <zephyr/syscalls/zvfs_mmap_mrsh.c>

static int z_vrfy_zvfs_munmap(void *addr, size_t len)
{
	return z_impl_zvfs_munmap(addr, len);
}
#include <zephyr/syscalls/zvfs_munmap_mrsh.c>

static int z_vrfy_zvfs_msync(void *addr, size_t len, int flags)
{
	return z_impl_zvfs_msync(addr, len, flags);
}
#include <zephyr/syscalls/zvfs_msync_mrsh.c>

static int z_vrfy_zvfs_mprotect(void *addr, size_t len, int prot)
{
	return z_impl_zvfs_mprotect(addr, len, prot);
}
#include <zephyr/syscalls/zvfs_mprotect_mrsh.c>

#ifdef CONFIG_ZVFS_MLOCK
static int z_vrfy_zvfs_mlock(const void *addr, size_t len, int flags)
{
	if ((flags == 0) && (len != 0)) {
		if (Z_DETECT_POINTER_OVERFLOW(addr, len)) {
			errno = EINVAL;
			return -1;
		}
		/* a range the caller cannot read is not memory it has */
		if (arch_buffer_validate((void *)addr, len, false) != 0) {
			errno = ENOMEM;
			return -1;
		}
	}

	return z_impl_zvfs_mlock(addr, len, flags);
}
#include <zephyr/syscalls/zvfs_mlock_mrsh.c>

static int z_vrfy_zvfs_munlock(const void *addr, size_t len, int flags)
{
	if ((flags == 0) && (len != 0)) {
		if (Z_DETECT_POINTER_OVERFLOW(addr, len)) {
			errno = EINVAL;
			return -1;
		}
		/* a range the caller cannot read is not memory it has */
		if (arch_buffer_validate((void *)addr, len, false) != 0) {
			errno = ENOMEM;
			return -1;
		}
	}

	return z_impl_zvfs_munlock(addr, len, flags);
}
#include <zephyr/syscalls/zvfs_munlock_mrsh.c>
#endif /* CONFIG_ZVFS_MLOCK */
#endif /* CONFIG_USERSPACE */
