/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Destroy semaphore.
 *
 * see IEEE 1003.1
 */
int sem_destroy(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (k_sem_count_get(semaphore)) {
		errno = EBUSY;
		return -1;
	}

	k_sem_reset(semaphore);
	return 0;
}

/**
 * @brief Get value of semaphore.
 *
 * See IEEE 1003.1
 */
int sem_getvalue(sem_t *semaphore, int *value)
{
	*value = (int) k_sem_count_get(semaphore);
	return 0;
}
/**
 * @brief Initialize semaphore.
 *
 * See IEEE 1003.1
 */
int sem_init(sem_t *semaphore, int pshared, unsigned int value)
{
	if (value > CONFIG_SEM_VALUE_MAX) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * Zephyr has no concept of process, so only thread shared
	 * semaphore makes sense in here.
	 */
	__ASSERT(pshared == 0, "pshared should be 0");

	k_sem_init(semaphore, value, CONFIG_SEM_VALUE_MAX);

	return 0;
}

/**
 * @brief Unlock a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_post(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	k_sem_give(semaphore);
	return 0;
}

/**
 * @brief Try time limited locking a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_timedwait(sem_t *semaphore, struct timespec *abstime)
{
	s32_t timeout;
	s64_t current_ms, abstime_ms;

	__ASSERT(abstime, "abstime pointer NULL");

	if ((abstime->tv_sec < 0) || (abstime->tv_nsec >= NSEC_PER_SEC)) {
		errno = EINVAL;
		return -1;
	}

	current_ms = (s64_t)k_uptime_get();
	abstime_ms = (s64_t)_ts_to_ms(abstime);

	if (abstime_ms <= current_ms) {
		timeout = K_NO_WAIT;
	} else {
		timeout = (s32_t)(abstime_ms - current_ms);
	}

	if (k_sem_take(semaphore, timeout)) {
		errno = ETIMEDOUT;
		return -1;
	}

	return 0;
}

/**
 * @brief Lock a semaphore if not taken.
 *
 * See IEEE 1003.1
 */
int sem_trywait(sem_t *semaphore)
{
	if (k_sem_take(semaphore, K_NO_WAIT) == -EBUSY) {
		errno = EAGAIN;
		return -1;
	} else {
		return 0;
	}
}

/**
 * @brief Lock a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_wait(sem_t *semaphore)
{
	k_sem_take(semaphore, K_FOREVER);
	return 0;
}


/*
 * Named semaphore design:
 * - Opened table -> which semaphore is open by which thread.
 * - Named semaphore pool -> global struct to allocate named sem.
 */

/*
 * Used for error setting and exit
 */
#define SEM_ERRNO_SET(_code)      \
	do {                      \
		errno = (_code);  \
		goto error;       \
	} while (0)

#define NSEM_SYS_N_MAX     (32) /* Limit of allocated nsem in system */
#define NSEM_SYS_OPEN_MAX  (32) /* System limit of simultaneously open nsem */
#define NSEM_PROC_OPEN_MAX  (4) /* Process limit of simultaneously open nsem */

/*
 * Private named semaphore
 */
struct _nsem {
	sem_t sem;
	const char *name;
	bool to_free; /* Flag set by unlink */
	bool in_use;  /* Used to check if this spot is available */
};
static unsigned int sem_named_n;
static struct _nsem sem_named[NSEM_SYS_N_MAX];

/*
 * Given a sem_t value, return the _nsem object containing it.
 * This needs to be updated if we change the order of the elements in
 * struct _nsem.
 */
#define nsem_container_of(sem_t_ptr) ((struct _nsem *)sem_t_ptr)

/*
 * Opened table entries
 */
struct _nsem_otable {
	struct _nsem *nsem_ptr;
	k_tid_t proc;
};
static struct _nsem_otable nsem_otable[NSEM_SYS_OPEN_MAX];

/*
 * Private functions.
 */

/*
 * Only called if we are sure it does not exist
 */
static inline struct _nsem *nsem_alloc(const char *name)
{
	struct _nsem *it, *lmt;

	it = sem_named;
	lmt = sem_named + NSEM_SYS_N_MAX;

	while (it != lmt) {
		if (!it->in_use) {
			it->in_use = true;
			it->to_free = false;
			it->name = name;

			return it;
		}
		++it;
	}

	return NULL;
}

static inline void nsem_free(struct _nsem *nsem)
{
	nsem->name = NULL;
	nsem->in_use = false;
	nsem->to_free = false;
	sem_destroy(&nsem->sem); /* This should never fail */
}

/*
 * NULL if not found, pointer to the object if found.
 * In sem_named there can be several elements with the same name, but all
 * except one must have the field "to_free" == 0, thus, the rest are waiting to
 * be closed.
 */
static inline struct _nsem *nsem_find_by_name(const char *name)
{
	struct _nsem *it, *lmt;

	it = sem_named;
	lmt = sem_named + NSEM_SYS_N_MAX;

	while (it != lmt) {
		if (!strncmp(name, it->name, PATH_MAX) &&
		    it->in_use && !it->to_free)
			return it;
		++it;
	}

	return NULL;
}

static inline struct _nsem_otable *nsem_otable_alloc(k_tid_t proc,
						     struct _nsem *nsem)
{
	struct _nsem_otable *it, *lmt;

	it = nsem_otable;
	lmt = nsem_otable + NSEM_SYS_N_MAX;

	while (it != lmt) {
		if (!it->nsem_ptr) {
			it->nsem_ptr = nsem;
			it->proc = proc;

			return it;
		}
		++it;
	}

	return NULL;
}

static inline void nsem_otable_free(struct _nsem_otable *ote)
{
	ote->nsem_ptr = NULL; /* Flag to mark as non used */
}

/*
 * Find in opened table an entry matching with arguments.
 * Returns NULL if not found, or invalid arguments.
 */
static inline struct _nsem_otable *nsem_otable_find(k_tid_t proc,
						    struct _nsem *nsem)
{
	struct _nsem_otable *it, *lmt;

	if (!nsem)
		return NULL;

	it = nsem_otable;
	lmt = nsem_otable + NSEM_SYS_OPEN_MAX;

	while (it != lmt) {
		if (it->nsem_ptr == nsem && it->proc == proc) {
			return it;
		}
		++it;
	}

	return 0;
}

static inline unsigned int nsem_otable_count_refs(struct _nsem *nsem)
{
	struct _nsem_otable *it, *lmt;
	unsigned int count = 0;

	if (nsem == NULL) {
		return 0;
	}

	it = nsem_otable;
	lmt = nsem_otable + NSEM_SYS_OPEN_MAX;

	while (it != lmt) {
		if (it->nsem_ptr == nsem) {
			++count;
		}
		++it;
	}

	return count;
}

static inline unsigned nsem_otable_count_opened(k_tid_t proc)
{
	struct _nsem_otable *it, *lmt;
	unsigned int count = 0;

	it = nsem_otable;
	lmt = nsem_otable + NSEM_SYS_OPEN_MAX;

	while (it != lmt) {
		if (it->proc == proc) {
			++count;
		}
		++it;
	}

	return count;
}

/*
 * Flags control if the semaphore is created or simply retrieved
 * O_CREAT: Creates a new semaphore with mode_t permissions and count.
 * O_EXCL:  Passed along with O_CREAT, fails if a semaphore with name already
 *          exists.
 *
 * This function is atomic since a few variables are shared.
 */
sem_t *sem_open(const char *name, int oflag, ...)
{
	struct _nsem *nsem;
	unsigned int value;
	unsigned int key;
	va_list al;
	k_tid_t curr;

	key = irq_lock();

	nsem = nsem_find_by_name(name);
	curr = k_current_get();

	/*
	 * Multiple calls to sem_open() by the same process
	 * should be successful.
	 */
	if (nsem_otable_find(curr, nsem)) { /* Deals with NULL */
		goto success;
	}

	if (sem_named_n == NSEM_SYS_N_MAX) {
		SEM_ERRNO_SET(ENFILE);
	}

	if (nsem_otable_count_opened(curr) == NSEM_PROC_OPEN_MAX) {
		SEM_ERRNO_SET(EMFILE);
	}

	/* Check if this named semaphore already exists */
	if (nsem) {
		if (oflag & O_EXCL) {
			SEM_ERRNO_SET(EEXIST);
		}
	} else {
		if (!(oflag & O_CREAT)) {
			SEM_ERRNO_SET(ENOENT);
		}

		va_start(al, oflag);
		va_arg(al, mode_t); /* permissions not used */
		value = va_arg(al, unsigned int);
		va_end(al);

		nsem = nsem_alloc(name);
		sem_init(&nsem->sem, 0, value);
		sem_named_n++;
	}

	/* Should always be successful, we checked the size before */
	nsem_otable_alloc(curr, nsem);

success:
	irq_unlock(key);
	return &nsem->sem;

error:
	irq_unlock(key);
	return SEM_FAILED;
}

/*
 * Close can destroy a semaphore too if sem_unlink has been called on that sem
 * before and it's the last reference to it.
 */
int sem_close(sem_t *sem)
{
	struct _nsem_otable *ote;
	struct _nsem *nsem;
	unsigned int key;
	k_tid_t curr;

	key = irq_lock();


	/* This is safe since we don't read or write nsem */
	nsem = nsem_container_of(sem);
	curr = k_current_get();
	ote = nsem_otable_find(curr, nsem);

	/* If ote is set, nsem is legit so we can use it */
	if (!ote) {
		SEM_ERRNO_SET(EINVAL);
	}

	/* Dereference semaphore */
	nsem_otable_free(ote);
	if (nsem->to_free && !nsem_otable_count_refs(nsem)) {
		nsem_free(nsem);
		sem_named_n--;
	}

	irq_unlock(key);
	return 0;

error:
	irq_unlock(key);
	return -1;
}

/*
 * Free a semaphore if no references left.
 * Otherwise, mark it as pending to free.
 */
int sem_unlink(const char *name)
{
	struct _nsem *nsem;
	unsigned int key, refs;

	key = irq_lock();
	nsem = nsem_find_by_name(name);

	if (!nsem) {
		SEM_ERRNO_SET(ENOENT);
	}

	refs = nsem_otable_count_refs(nsem);

	if (!refs) {
		nsem_free(nsem);
		sem_named_n--;
	} else {
		nsem->to_free = true;
	}

	irq_unlock(key);
	return 0;

error:
	irq_unlock(key);
	return -1;
}
