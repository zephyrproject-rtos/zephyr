/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <errno.h>

/* The thread will test that the refcounting of fd object will
 * work as expected.
 */
static struct k_thread fd_thread;
static int shared_fd;

static struct fd_op_vtable fd_vtable = { 0 };

#define VTABLE_INIT (&fd_vtable)

K_THREAD_STACK_DEFINE(fd_thread_stack, CONFIG_ZTEST_STACK_SIZE +
		      CONFIG_TEST_EXTRA_STACK_SIZE);

ZTEST(fdtable, test_zvfs_reserve_fd)
{
	int fd = zvfs_reserve_fd(); /* function being tested */

	zassert_true(fd >= 0, "fd < 0");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_get_fd_obj_and_vtable)
{
	const struct fd_op_vtable *vtable;

	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0, "fd < 0");

	int *obj;
	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable,
				      NULL); /* function being tested */

	zassert_is_null(obj, "obj is not NULL");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_get_fd_obj)
{
	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0, "fd < 0");

	int err = -1;
	const struct fd_op_vtable *vtable = 0;
	const struct fd_op_vtable *vtable2 = vtable+1;

	int *obj = zvfs_get_fd_obj(fd, vtable, err); /* function being tested */

	/* take branch -- if (_check_fd(fd) < 0) */
	zassert_is_null(obj, "obj not is NULL");

	obj = (void *)1;
	vtable = NULL;

	/* This will set obj and vtable properly */
	zvfs_finalize_fd(fd, obj, vtable);

	obj = zvfs_get_fd_obj(-1, vtable, err); /* function being tested */

	zassert_equal_ptr(obj, NULL, "obj is not NULL when fd < 0");
	zassert_equal(errno, EBADF, "fd: out of bounds error");

	/* take branch -- if (vtable != NULL && fd_entry->vtable != vtable) */
	obj = zvfs_get_fd_obj(fd, vtable2, err); /* function being tested */

	zassert_equal_ptr(obj, NULL, "obj is not NULL - vtable doesn't match");
	zassert_equal(errno, err, "vtable matches");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_finalize_fd)
{
	const struct fd_op_vtable *vtable;

	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0);

	int *obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	const struct fd_op_vtable *original_vtable = vtable;
	int *original_obj = obj;

	zvfs_finalize_fd(fd, obj, vtable); /* function being tested */

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, original_obj, "obj is different after finalizing");
	zassert_equal_ptr(vtable, original_vtable, "vtable is different after finalizing");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_alloc_fd)
{
	const struct fd_op_vtable *vtable = NULL;
	int *obj = NULL;

	int fd = zvfs_alloc_fd(obj, vtable); /* function being tested */
	zassert_true(fd >= 0);

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, NULL, "obj is different after allocating");
	zassert_equal_ptr(vtable, NULL, "vtable is different after allocating");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_free_fd)
{
	const struct fd_op_vtable *vtable = NULL;

	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0);

	zvfs_free_fd(fd); /* function being tested */

	int *obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, NULL, "obj is not NULL after freeing");
}

static void test_cb(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int fd = POINTER_TO_INT(p1);
	const struct fd_op_vtable *vtable;
	int *obj;

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_not_null(obj, "obj is null");
	zassert_not_null(vtable, "vtable is null");

	zvfs_free_fd(fd);

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);
	zassert_is_null(obj, "obj is still there");
	zassert_equal(errno, EBADF, "fd was found");
}

ZTEST(fdtable, test_z_fd_multiple_access)
{
	const struct fd_op_vtable *vtable = VTABLE_INIT;
	void *obj = (void *)vtable;

	shared_fd = zvfs_reserve_fd();
	zassert_true(shared_fd >= 0, "fd < 0");

	zvfs_finalize_fd(shared_fd, obj, vtable);

	k_thread_create(&fd_thread, fd_thread_stack,
			K_THREAD_STACK_SIZEOF(fd_thread_stack),
			test_cb,
			INT_TO_POINTER(shared_fd), NULL, NULL,
			CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_join(&fd_thread, K_FOREVER);

	/* should be null since freed in the other thread */
	obj = zvfs_get_fd_obj_and_vtable(shared_fd, &vtable, NULL);
	zassert_is_null(obj, "obj is still there");
	zassert_equal(errno, EBADF, "fd was found");
}

static int close_calls;

static int counting_close(void *obj)
{
	ARG_UNUSED(obj);

	close_calls++;

	return 0;
}

static int read_calls;

static ssize_t counting_read(void *obj, void *buf, size_t sz)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(buf);
	ARG_UNUSED(sz);

	read_calls++;

	return 0;
}

static const struct fd_op_vtable closing_vtable = {
	.read = counting_read,
	.close = counting_close,
};

ZTEST(fdtable, test_zvfs_fd_get_locked_live)
{
	const struct fd_op_vtable *vtable = NULL;
	const struct fd_op_vtable *ignored;
	struct k_mutex *lock = NULL;
	void *obj = (void *)&fd_vtable;
	void *got;

	int fd = zvfs_reserve_fd();

	zassert_true(fd >= 0, "fd < 0");

	zvfs_finalize_fd(fd, obj, &fd_vtable);
	zassert_equal_ptr(zvfs_get_fd_obj_and_vtable(fd, &ignored, &lock), obj);

	got = zvfs_fd_get_locked(fd, &vtable); /* function being tested */
	zassert_equal_ptr(got, obj, "wrong obj");
	zassert_equal_ptr(vtable, &fd_vtable, "wrong vtable");
	zassert_equal_ptr(lock->owner, k_current_get(), "lock not taken");

	zvfs_fd_unlock_put(fd);

	zassert_is_null(lock->owner, "lock not released");
	zassert_equal_ptr(zvfs_get_fd_obj(fd, NULL, 0), obj, "fd freed by put");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_fd_get_locked_free_slot)
{
	const struct fd_op_vtable *vtable;

	int fd = zvfs_reserve_fd();

	zassert_true(fd >= 0, "fd < 0");

	zvfs_free_fd(fd);

	errno = 0;
	zassert_is_null(zvfs_fd_get_locked(fd, &vtable), "free slot referenced");
	zassert_equal(errno, EBADF);

	int fd2 = zvfs_reserve_fd();

	zassert_equal(fd2, fd, "free slot was resurrected");

	zvfs_free_fd(fd2);
}

ZTEST(fdtable, test_zvfs_fd_get_locked_out_of_range)
{
	const struct fd_op_vtable *vtable;

	errno = 0;
	zassert_is_null(zvfs_fd_get_locked(-1, &vtable));
	zassert_equal(errno, EBADF);

	errno = 0;
	zassert_is_null(zvfs_fd_get_locked(ZVFS_OPEN_SIZE, &vtable));
	zassert_equal(errno, EBADF);
}

ZTEST(fdtable, test_zvfs_close_while_ref_held)
{
	const struct fd_op_vtable *vtable;
	void *obj = (void *)&closing_vtable;

	close_calls = 0;

	int fd = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd >= 0, "fd < 0");

	zassert_equal_ptr(zvfs_fd_get_locked(fd, &vtable), obj);

	zassert_equal(zvfs_close(fd), 0);
	zassert_equal(close_calls, 1);

	/* Once close() has returned, the fd is gone for every lookup */
	errno = 0;
	zassert_is_null(zvfs_get_fd_obj(fd, NULL, 0), "closed fd found");
	zassert_equal(errno, EBADF);

	errno = 0;
	zassert_is_null(zvfs_fd_get_locked(fd, &vtable), "closing fd referenced");
	zassert_equal(errno, EBADF);

	int other = zvfs_reserve_fd();

	zassert_true(other >= 0, "other < 0");
	zassert_not_equal(other, fd, "slot reused while a reference is held");
	zvfs_free_fd(other);

	zvfs_fd_unlock_put(fd);

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "slot not freed by the last put");
	zvfs_free_fd(again);
}

ZTEST(fdtable, test_zvfs_ref_does_not_leak)
{
	const struct fd_op_vtable *vtable;
	void *obj = (void *)&closing_vtable;

	int fd = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd >= 0, "fd < 0");

	for (int i = 0; i < 100; i++) {
		zassert_not_null(zvfs_fd_get_locked(fd, &vtable));
		zvfs_fd_unlock_put(fd);
	}

	zassert_equal(zvfs_close(fd), 0);

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "reference leaked");
	zvfs_free_fd(again);
}

ZTEST(fdtable, test_zvfs_fd_get_locked_unfinalized)
{
	const struct fd_op_vtable *vtable;
	int fd = zvfs_reserve_fd();

	zassert_true(fd >= 0, "fd < 0");

	errno = 0;
	zassert_is_null(zvfs_fd_get_locked(fd, &vtable));
	zassert_equal(errno, EBADF);

	zvfs_free_fd(fd);

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "failed lookup leaked a reference");
	zvfs_free_fd(again);
}

static volatile bool closer_entered;
static volatile bool closer_done;
static int closer_res;
static int closer_errno;

static void closer_cb(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	closer_entered = true;
	closer_res = zvfs_close(POINTER_TO_INT(p1));
	closer_errno = errno;
	closer_done = true;
}

ZTEST(fdtable, test_zvfs_close_waiter_holds_slot)
{
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock;
	void *obj = (void *)&closing_vtable;

	/* The window below is made by priorities, which order threads on one CPU only */
	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}

	close_calls = 0;
	closer_entered = false;
	closer_done = false;

	int fd = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd >= 0, "fd < 0");

	zassert_equal_ptr(zvfs_get_fd_obj_and_vtable(fd, &vtable, &lock), obj);
	zassert_ok(k_mutex_lock(lock, K_FOREVER));

	/* A higher priority closer runs on the yield and blocks on the entry lock. */
	k_thread_create(&fd_thread, fd_thread_stack, K_THREAD_STACK_SIZEOF(fd_thread_stack),
			closer_cb, INT_TO_POINTER(fd), NULL, NULL,
			k_thread_priority_get(k_current_get()) - 1, 0, K_NO_WAIT);
	k_yield();
	zassert_true(closer_entered, "closer did not run");
	zassert_false(closer_done, "closer did not block on the entry lock");

	/* The first close, from the lock owner, drops the table's reference. */
	zassert_equal(zvfs_close(fd), 0);
	zassert_equal(close_calls, 1);

	int other = zvfs_reserve_fd();

	zassert_true(other >= 0, "other < 0");
	zassert_not_equal(other, fd, "slot reused while a close() waits on its lock");
	zvfs_free_fd(other);

	k_mutex_unlock(lock);
	k_thread_join(&fd_thread, K_FOREVER);

	zassert_equal(closer_res, -1);
	zassert_equal(closer_errno, EBADF);
	zassert_equal(close_calls, 1, "fd closed twice");

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "slot not freed after the second close()");
	zvfs_free_fd(again);

	errno = 0;
	zassert_equal(zvfs_close(fd), -1);
	zassert_equal(errno, EBADF);
}

ZTEST(fdtable, test_zvfs_read_after_close_while_ref_held)
{
	const struct fd_op_vtable *vtable;
	void *obj = (void *)&closing_vtable;
	char buf[1];

	read_calls = 0;

	int fd = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd >= 0, "fd < 0");

	zassert_equal_ptr(zvfs_fd_get_locked(fd, &vtable), obj);
	zassert_equal(zvfs_close(fd), 0);

	/* The held reference keeps the slot, not the fd */
	errno = 0;
	zassert_equal(zvfs_read(fd, buf, sizeof(buf), NULL), -1);
	zassert_equal(errno, EBADF);
	zassert_equal(read_calls, 0, "read() ran on a closed fd");

	zvfs_fd_unlock_put(fd);
}

static volatile bool reader_entered;
static volatile bool reader_done;
static ssize_t reader_res;
static int reader_errno;

static void reader_cb(void *p1, void *p2, void *p3)
{
	char buf[1];

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	reader_entered = true;
	reader_res = zvfs_read(POINTER_TO_INT(p1), buf, sizeof(buf), NULL);
	reader_errno = errno;
	reader_done = true;
}

ZTEST(fdtable, test_zvfs_read_queued_during_close)
{
	const struct fd_op_vtable *vtable;
	void *obj = (void *)&closing_vtable;

	/* The window below is made by priorities, which order threads on one CPU only */
	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}

	read_calls = 0;
	reader_entered = false;
	reader_done = false;

	int fd = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd >= 0, "fd < 0");

	/* Our reference keeps the slot whatever the reader does */
	zassert_equal_ptr(zvfs_fd_get_locked(fd, &vtable), obj);

	/* A higher priority reader runs on the yield and blocks on the entry lock. */
	k_thread_create(&fd_thread, fd_thread_stack, K_THREAD_STACK_SIZEOF(fd_thread_stack),
			reader_cb, INT_TO_POINTER(fd), NULL, NULL,
			k_thread_priority_get(k_current_get()) - 1, 0, K_NO_WAIT);
	k_yield();
	zassert_true(reader_entered, "reader did not run");
	zassert_false(reader_done, "reader did not block on the entry lock");

	zassert_equal(zvfs_close(fd), 0);

	zvfs_fd_unlock_put(fd);
	k_thread_join(&fd_thread, K_FOREVER);

	zassert_equal(reader_res, -1);
	zassert_equal(reader_errno, EBADF);
	zassert_equal(read_calls, 0, "read() queued during close() ran on the closed fd");

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "slot not freed after the last reference");
	zvfs_free_fd(again);
}

ZTEST(fdtable, test_zvfs_obj_lookup_skips_closed_fd)
{
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock_b;
	struct k_mutex *lock = NULL;
	struct k_condvar *cond = NULL;
	void *obj = (void *)&closing_vtable;

	int fd_a = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd_a >= 0, "fd_a < 0");

	/* A call still running in A, such as a reader woken by close() */
	zassert_equal_ptr(zvfs_fd_get_locked(fd_a, &vtable), obj);
	zassert_equal(zvfs_close(fd_a), 0);

	/* The object goes to a new fd, as eventfd() reuses a freed one */
	int fd_b = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd_b > fd_a, "fd_b %d did not follow fd_a %d", fd_b, fd_a);
	zassert_equal_ptr(zvfs_get_fd_obj_and_vtable(fd_b, &vtable, &lock_b), obj);

	zassert_true(zvfs_get_obj_lock_and_cond(obj, &closing_vtable, &lock, &cond),
		     "lookup by object stopped at the closed fd");
	zassert_equal_ptr(lock, lock_b, "lookup by object found the closed fd");

	zvfs_fd_unlock_put(fd_a);
	zassert_equal(zvfs_close(fd_b), 0);
}

static volatile bool getter_entered;
static volatile bool getter_done;
static void *getter_res;
static int getter_errno;

static void getter_cb(void *p1, void *p2, void *p3)
{
	const struct fd_op_vtable *vtable;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	getter_entered = true;
	getter_res = zvfs_fd_get_locked(POINTER_TO_INT(p1), &vtable);
	getter_errno = errno;
	getter_done = true;
}

ZTEST(fdtable, test_zvfs_fd_get_locked_queued_during_close)
{
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock;
	void *obj = (void *)&closing_vtable;

	/* The window below is made by priorities, which order threads on one CPU only */
	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}

	getter_entered = false;
	getter_done = false;

	int fd = zvfs_alloc_fd(obj, &closing_vtable);

	zassert_true(fd >= 0, "fd < 0");

	zassert_equal_ptr(zvfs_get_fd_obj_and_vtable(fd, &vtable, &lock), obj);
	zassert_ok(k_mutex_lock(lock, K_FOREVER));

	/* A higher priority caller runs on the yield and blocks on the entry lock. */
	k_thread_create(&fd_thread, fd_thread_stack, K_THREAD_STACK_SIZEOF(fd_thread_stack),
			getter_cb, INT_TO_POINTER(fd), NULL, NULL,
			k_thread_priority_get(k_current_get()) - 1, 0, K_NO_WAIT);
	k_yield();
	zassert_true(getter_entered, "caller did not run");
	zassert_false(getter_done, "caller did not block on the entry lock");

	zassert_equal(zvfs_close(fd), 0);

	k_mutex_unlock(lock);
	k_thread_join(&fd_thread, K_FOREVER);

	zassert_is_null(getter_res, "fd closed while waiting for its lock was returned");
	zassert_equal(getter_errno, EBADF);
	zassert_is_null(lock->owner, "failed call kept the lock");

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "failed call kept its reference");
	zvfs_free_fd(again);
}

static struct k_poll_signal poll_sig[2];
static int prepare_calls;
static int prepare_result;
static int update_calls;
static int offload_calls;

static int poll_ioctl(void *obj, unsigned int request, va_list args)
{
	struct zvfs_pollfd *pfd;
	struct k_poll_event **pev;
	struct k_poll_event *pev_end;

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		(void)va_arg(args, struct zvfs_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);
		prepare_calls++;
		if (prepare_result != 0) {
			return prepare_result;
		}
		if (*pev == pev_end) {
			return -ENOMEM;
		}
		k_poll_event_init(*pev, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, obj);
		(*pev)++;
		return 0;
	case ZFD_IOCTL_POLL_UPDATE:
		pfd = va_arg(args, struct zvfs_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		update_calls++;
		if ((*pev)->state != K_POLL_STATE_NOT_READY) {
			pfd->revents |= ZVFS_POLLIN;
		}
		(*pev)++;
		return 0;
	case ZFD_IOCTL_POLL_OFFLOAD:
		offload_calls++;
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static const struct fd_op_vtable poll_vtable = {
	.close = counting_close,
	.ioctl = poll_ioctl,
};

static void poll_reset(void)
{
	prepare_calls = 0;
	prepare_result = 0;
	update_calls = 0;
	offload_calls = 0;
	close_calls = 0;
	k_poll_signal_init(&poll_sig[0]);
	k_poll_signal_init(&poll_sig[1]);
}

static struct zvfs_pollfd poller_fds[2];
static volatile bool poller_entered;
static volatile bool poller_done;
static int poller_res;

static void poller_cb(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	poller_entered = true;
	poller_res = zvfs_poll(poller_fds, POINTER_TO_INT(p1), POINTER_TO_INT(p2));
	poller_done = true;
}

static void start_poller(int nfds, int timeout_ms)
{
	poller_entered = false;
	poller_done = false;

	/* A higher priority poller runs on the yield */
	k_thread_create(&fd_thread, fd_thread_stack, K_THREAD_STACK_SIZEOF(fd_thread_stack),
			poller_cb, INT_TO_POINTER(nfds), INT_TO_POINTER(timeout_ms), NULL,
			k_thread_priority_get(k_current_get()) - 1, 0, K_NO_WAIT);
	k_yield();
	zassert_true(poller_entered, "poller did not run");
	zassert_false(poller_done, "poller did not block");
}

ZTEST(fdtable, test_zvfs_poll_queued_during_close)
{
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock;

	/* The window below is made by priorities, which order threads on one CPU only */
	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}

	poll_reset();

	int fd = zvfs_alloc_fd(&poll_sig[0], &poll_vtable);

	zassert_true(fd >= 0, "fd < 0");

	zassert_equal_ptr(zvfs_get_fd_obj_and_vtable(fd, &vtable, &lock), &poll_sig[0]);
	zassert_ok(k_mutex_lock(lock, K_FOREVER));

	poller_fds[0] = (struct zvfs_pollfd){.fd = fd, .events = ZVFS_POLLIN};
	start_poller(1, 0);

	zassert_equal(zvfs_close(fd), 0);
	zassert_equal(close_calls, 1);

	k_mutex_unlock(lock);
	k_thread_join(&fd_thread, K_FOREVER);

	zassert_equal(poller_res, 1);
	zassert_equal(poller_fds[0].revents, ZVFS_POLLNVAL);
	zassert_equal(prepare_calls, 0, "poll() queued during close() prepared the closed fd");
	zassert_equal(update_calls, 0);

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "poll() kept its reference");
	zvfs_free_fd(again);
}

ZTEST(fdtable, test_zvfs_poll_close_while_waiting)
{
	/* The window below is made by priorities, which order threads on one CPU only */
	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}

	poll_reset();

	int fd_a = zvfs_alloc_fd(&poll_sig[0], &poll_vtable);
	int fd_b = zvfs_alloc_fd(&poll_sig[1], &poll_vtable);

	zassert_true(fd_a >= 0 && fd_b >= 0, "fd < 0");

	poller_fds[0] = (struct zvfs_pollfd){.fd = fd_a, .events = ZVFS_POLLIN};
	poller_fds[1] = (struct zvfs_pollfd){.fd = fd_b, .events = ZVFS_POLLIN};
	start_poller(2, -1);
	zassert_equal(prepare_calls, 2);

	zassert_equal(zvfs_close(fd_a), 0);

	/* The table may have no other free slot */
	int other = zvfs_reserve_fd();

	zassert_not_equal(other, fd_a, "slot reused while poll() waits on it");
	if (other >= 0) {
		zvfs_free_fd(other);
	}

	zassert_ok(k_poll_signal_raise(&poll_sig[1], 0));
	k_thread_join(&fd_thread, K_FOREVER);

	zassert_equal(poller_res, 2);
	zassert_equal(poller_fds[0].revents, ZVFS_POLLNVAL);
	zassert_equal(poller_fds[1].revents, ZVFS_POLLIN, "fd_b read the closed fd's event");
	zassert_equal(update_calls, 1, "poll() updated the closed fd");

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd_a, "poll() kept its reference");
	zvfs_free_fd(again);
	zassert_equal(zvfs_close(fd_b), 0);
}

static void poll_and_close(int fd, int nfds, int expected)
{
	poller_fds[0] = (struct zvfs_pollfd){.fd = fd, .events = ZVFS_POLLIN};
	poller_fds[1] = (struct zvfs_pollfd){.fd = fd, .events = ZVFS_POLLIN};
	zassert_equal(zvfs_poll(poller_fds, nfds, 0), expected);
	zassert_equal(zvfs_close(fd), 0);

	int again = zvfs_reserve_fd();

	zassert_equal(again, fd, "poll() kept its reference");
	zvfs_free_fd(again);
}

ZTEST(fdtable, test_zvfs_poll_drops_references)
{
	poll_reset();
	zassert_ok(k_poll_signal_raise(&poll_sig[0], 0));

	/* Returning events, with the fd polled twice */
	poll_and_close(zvfs_alloc_fd(&poll_sig[0], &poll_vtable), 2, 2);
	zassert_equal(update_calls, 2);

	/* Failing in prepare */
	poll_reset();
	prepare_result = -EINVAL;
	poll_and_close(zvfs_alloc_fd(&poll_sig[0], &poll_vtable), 1, -1);
	zassert_equal(prepare_calls, 1);

	/* Handing over to an offloaded poll */
	poll_reset();
	prepare_result = -EXDEV;
	poll_and_close(zvfs_alloc_fd(&poll_sig[0], &poll_vtable), 1, 0);
	zassert_equal(offload_calls, 1);
}

ZTEST_SUITE(fdtable, NULL, NULL, NULL, NULL, NULL);
