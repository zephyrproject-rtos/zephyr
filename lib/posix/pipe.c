/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include <kernel.h>
#include <posix/unistd.h>
#include <sys/fdtable.h>
#include <sys/util.h>

#if 0
#define D(fmt, args...) printk("D: %s(): %d: " fmt "\n", __func__, __LINE__, ##args)
#else
#define D(fmt, args...)
#endif

static const struct fd_op_vtable pipe_fd_op_vtable;

struct pipe_fd_object {
	/* counterpart file descriptor for the other end of the pipe */
	int other;
	/* indicates *this* object is the read end of the pipe */
	bool is_read_end :1;
	/* indicates that *this* file descriptor is in blocking mode */
	bool is_blocking :1;
};

/* read side of pipe */
struct pipe_fd_robject {
	struct pipe_fd_object obj;
};

/* write side of pipe */
struct pipe_fd_wobject {
	struct pipe_fd_object obj;
	struct k_pipe pipe;
	unsigned char buf[CONFIG_POSIX_PIPE_SIZE];
};

static ssize_t pipe_read(void *_obj, void *buf, size_t sz)
{
	struct pipe_fd_object *const obj = (struct pipe_fd_object *)_obj;

	int res;
	int key;
	struct pipe_fd_object *other_obj;
	struct pipe_fd_wobject *wobj;
	size_t bytes_read = 0;
	size_t min_xfer;
	int timeout;

	D("obj: %p buf: %p: sz: %u", obj, buf, sz );

	key = irq_lock();

	if (!obj->is_read_end) {
		D("attempt to read from the write end");
		errno = EBADF;
		res = -1;
		goto out;
	}

	other_obj = z_get_fd_obj(obj->other, &pipe_fd_op_vtable, 0);
	if (other_obj == NULL) {
		D("write side of pipe has closed down");
		res = 0;
		goto out;
	}

// Need to learn a bit more about Zephyr kernel vs user space.
// This causes a compile error, even when I include syscall_handler.h
/*
	if (Z_SYSCALL_MEMORY_WRITE(buf,sz)) {
		errno = EFAULT;
		res = -1;
		goto out;
	}
*/

	if (sz == 0) {
		D("request to read 0 bytes");
		res = 0;
		goto out;
	}

	wobj = CONTAINER_OF(other_obj, struct pipe_fd_wobject, obj);
	assert(wobj != NULL);

	D("wobj: %p", wobj);

	if (obj->is_blocking) {
		min_xfer = 1;
		// N.B. K_FOREVER does not work here!!
		timeout = 42;
	} else {
		min_xfer = 0;
		timeout = K_NO_WAIT;
	}

	D("k_pipe_get(data: %p bytes_to_read: %u min_xfer: %u timeout: %d", buf, sz, min_xfer, timeout);
	res = k_pipe_get(&wobj->pipe, buf, sz, &bytes_read, min_xfer, timeout);
	D("k_pipe_get(): %d bytes_read: %u", res, bytes_read);

	if (res < 0) {
		errno = -res;
		res = -1;
		goto out;
	}
	assert(res == 0);

	if (obj->is_blocking) {
		assert(bytes_read > 0);
		res = bytes_read;
		goto out;
	}

	if (bytes_read == 0) {
		errno = EAGAIN;
		res = -1;
	} else {
		res = bytes_read;
	}

out:
	irq_unlock(key);
	D("returning %d", res);
	return res;
}

static ssize_t pipe_write(void *_obj, const void *buf, size_t sz)
{
	struct pipe_fd_object *const obj = (struct pipe_fd_object *)_obj;

	int res;
	int key;
	struct pipe_fd_object *other_obj;
	struct pipe_fd_wobject *wobj;
	size_t min_xfer;
	size_t bytes_written = 0;
	int timeout;

	D("obj: %p buf: '%s': sz: %u", obj, (NULL == buf) ? "(null)" : (char *)buf, sz );

	key = irq_lock();

	if (obj->is_read_end) {
		D("attempt to write to the read end");
		errno = EBADF;
		res = -1;
		goto out;
	}

	other_obj = z_get_fd_obj(obj->other, &pipe_fd_op_vtable, 0);
	if (other_obj == NULL) {
		D("write side of pipe has closed down");
		res = 0;
		goto out;
	}

// Need to learn a bit more about Zephyr kernel vs user space.
// This causes a compile error, even when I include syscall_handler.h
/*
	if (Z_SYSCALL_MEMORY_READ(buf,sz)) {
		errno = EFAULT;
		res = -1;
		goto out;
	}
*/

	if (sz == 0) {
		D("request to write 0 bytes");
		res = 0;
		goto out;
	}

	if (obj->is_blocking) {
		min_xfer = 1;
		// N.B. K_FOREVER does not work here!!
		timeout = 42;
	} else {
		min_xfer = 0;
		timeout = K_NO_WAIT;
	}

	wobj = CONTAINER_OF(obj, struct pipe_fd_wobject, obj);
	assert(wobj != NULL);

	D("k_pipe_put(data: %p bytes_to_write: %u min_xfer: %u timeout: %d", buf, sz, min_xfer, timeout);
	res = k_pipe_put(&wobj->pipe, (void *)buf, sz, &bytes_written, min_xfer, timeout);
	D("k_pipe_put(): %d bytes_written: %u", res, bytes_written);

	if (res < 0) {
		errno = -res;
		res = -1;
		goto out;
	}
	assert(res == 0);

	if (obj->is_blocking) {
		assert(bytes_written > 0);
		res = bytes_written;
		goto out;
	}

	if (bytes_written == 0) {
		errno = EAGAIN;
		res = -1;
	} else {
		res = bytes_written;
	}

out:
	irq_unlock(key);
	D("returning %d", res);
	return res;
}

static int pipe_ioctl_close(void *_obj, va_list args) {

	(void) args;

	struct pipe_fd_object *const obj = (struct pipe_fd_object *)_obj;

	int res;
	int key;
	struct pipe_fd_robject *robj;
	struct pipe_fd_wobject *wobj;

	D("obj: %p", obj);

	key = irq_lock();

	if (obj->is_read_end) {
		robj = CONTAINER_OF(obj, struct pipe_fd_robject, obj);
		assert(robj != NULL);
		D("calling k_free(robj)");
		k_free(robj);
		res = 0;
		goto out;
	}

	wobj = CONTAINER_OF(obj, struct pipe_fd_wobject, obj);
	assert(wobj != NULL);

	D("calling k_pipe_cleanup");
	k_pipe_cleanup(&wobj->pipe);

	D("calling k_free(wobj)");
	k_free(wobj);

	res = 0;

out:
	irq_unlock(key);
	D("returning %d", res);
	return res;
}

static int pipe_ioctl(void *obj, unsigned int request, va_list args)
{

	D("obj: %p request: %u", obj, request);

	switch(request) {

	case ZFD_IOCTL_CLOSE:
		return pipe_ioctl_close(obj, args);

	default:
		errno = ENOSYS;
		return -1;
	}
}

static const struct fd_op_vtable pipe_fd_op_vtable = {
	.read = pipe_read,
	.write = pipe_write,
	.ioctl = pipe_ioctl,
};

static int pipe_fd_alloc(bool is_read_end, struct pipe_fd_object **obj)
{

	const size_t size = is_read_end ? sizeof(struct pipe_fd_robject) : sizeof(struct pipe_fd_wobject);

	int res;
	int fd;
	void *o;

	D("is_read_end: %u &obj: %p", is_read_end, obj);

	assert(obj != NULL);

	D("allocating fd..");
	res = z_alloc_fd(*obj, &pipe_fd_op_vtable);
	if (res == -1) {
		D("z_alloc_fd() failed");
		errno = ENFILE;
		res = -1;
		goto out;
	}
	fd = res;
	D("allocated fd %d", fd);

	D("allocating %s object", is_read_end ? "robj" : "wobj");
	o = k_malloc(size);
	if (o == NULL) {
		errno = ENFILE;
		res = -1;
		goto free_fd;
	}
	memset(o, 0, size);

	if (is_read_end) {
		*obj = & ((struct pipe_fd_robject *)o)->obj;
	} else {
		struct pipe_fd_wobject *wobj = (struct pipe_fd_wobject *)o;
		*obj = & wobj->obj;
		D("initializing Zephyr pipe of size %u at %p", sizeof(wobj->buf), wobj->buf);
		k_pipe_init(&wobj->pipe, wobj->buf, sizeof(wobj->buf));
	}
	D("allocated obj: %p", *obj);
	(*obj)->is_read_end = is_read_end;
	(*obj)->is_blocking = true;

	D("success!");

	z_finalize_fd(fd, *obj, &pipe_fd_op_vtable);

	res = fd;
	goto out;

free_fd:
	z_free_fd(fd);

out:
	return res;
}

int pipe(int fildes[2])
{
	int res;
	int tmp[2];
	struct pipe_fd_object *obj[2] = {};

	D("fildes: %p", fildes);
	if (fildes == NULL) {
		D("argument was NULL");
		errno = EFAULT;
		res = -1;
		goto out;
	}

	D("allocating read fd");
	res = pipe_fd_alloc(true, &obj[0]);
	if (res == -1) {
		goto out;
	}
	tmp[0] = res;
	D("allocated read fd %d", tmp[0]);

	D("allocating write fd");
	res = pipe_fd_alloc(false, &obj[1]);
	if (res == -1) {
		goto free_read_fd;
	}
	tmp[1] = res;
	D("allocated write fd %d", tmp[1]);

	fildes[0] = tmp[0];
	fildes[1] = tmp[1];
	D("fildes: {%d, %d}", fildes[0], fildes[1]);

	/* link the two file descriptors to each other */
	obj[0]->other = fildes[1];
	obj[1]->other = fildes[0];

	D("obj: {%p, %p}", obj[0], obj[1]);

	D("success!");

	res = 0;
	goto out;

free_read_fd:
	z_free_fd(tmp[0]);
	k_free(obj[0]);

out:
	return res;
}
