/*
 * Copyright (c) Bruce ROSIER
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/fcntl.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/fs/fs.h>

static ssize_t pipe_read_vmeth(void *obj, void *buffer, size_t count);
static ssize_t pipe_write_vmeth(void *obj, const void *buffer, size_t count);
static int pipe_close_vmeth(void *obj);
static int pipe_ioctl_vmeth(void *obj, unsigned int request, va_list args);

struct pipe_desc {
	unsigned char __aligned(4) ring_buffer[CONFIG_POSIX_PIPE_BUF];
	struct k_pipe pipe;
	int flags_read;
	int flags_write;
	atomic_t read_opened;
	atomic_t write_opened;
	atomic_t is_used;
	struct k_sem sync;
};

static struct pipe_desc pipe_desc_array[CONFIG_POSIX_PIPES_MAX];

static struct fd_op_vtable fs_fd_op_vtable = {
	.read = pipe_read_vmeth,
	.write = pipe_write_vmeth,
	.close = pipe_close_vmeth,
	.ioctl = pipe_ioctl_vmeth,
};

static ssize_t pipe_read_vmeth(void *obj, void *buffer, size_t count)
{
	ssize_t total_read = 0;
	int *ptr = (int *)obj;
	unsigned char *read_buf = (unsigned char *)buffer;
	int rc;

	struct pipe_desc *p_pipe_desc =
		(struct pipe_desc *)((char *)ptr - offsetof(struct pipe_desc, flags_read));

	if (((*ptr & FS_O_READ) == 0) || atomic_get(&p_pipe_desc->read_opened) == 0) {
		errno = EACCES;
		return -1;
	}

	if ((*ptr & O_NONBLOCK) == O_NONBLOCK) {
		rc = k_pipe_get(&(p_pipe_desc->pipe), read_buf, count, &total_read, 1, K_NO_WAIT);
		if (rc == -EIO) {
			errno = EAGAIN;
			return -1;
		}

	} else {
		while (count > 0 && atomic_get(&p_pipe_desc->write_opened) == 1) {
			ssize_t read = 0;

			rc = k_pipe_get(&(p_pipe_desc->pipe), read_buf, count, &read, 1, K_NO_WAIT);

			if (rc != -EIO) {
				k_sem_give(&p_pipe_desc->sync);
			}

			if (read != count) {
				k_sem_take(&p_pipe_desc->sync, K_FOREVER);
			}

			read_buf += read;
			count -= read;
			total_read += read;
		}
	}

	return total_read;
}

static ssize_t pipe_write_vmeth(void *obj, const void *buffer, size_t count)
{
	ssize_t total_written = 0;
	int *ptr = (int *)obj;
	unsigned char *write_buf = (unsigned char *)buffer;
	int rc;

	struct pipe_desc *p_pipe_desc =
		(struct pipe_desc *)((char *)ptr - offsetof(struct pipe_desc, flags_write));

	if (((*ptr & FS_O_WRITE) == 0) || atomic_get(&p_pipe_desc->write_opened) == 0) {
		errno = EACCES;
		return -1;
	}

	if ((*ptr & O_NONBLOCK) == O_NONBLOCK) {
		rc = k_pipe_put(&(p_pipe_desc->pipe), write_buf, count, &total_written, 1,
				K_NO_WAIT);
		if (rc == -EIO) {
			errno = EAGAIN;
			return -1;
		}
	} else {
		while (count > 0 && atomic_get(&p_pipe_desc->read_opened) == 1) {
			ssize_t written = 0;

			rc = k_pipe_put(&(p_pipe_desc->pipe), write_buf, count, &written, 1,
					K_NO_WAIT);

			if (rc != -EIO) {
				k_sem_give(&p_pipe_desc->sync);
			}

			if (written != count) {
				k_sem_take(&p_pipe_desc->sync, K_FOREVER);
			}

			write_buf += written;
			count -= written;
			total_written += written;
		}
	}

	return total_written;
}

static int pipe_close_vmeth(void *obj)
{
	int *ptr = (int *)obj;
	struct pipe_desc *p_pipe_desc;

	if ((*ptr & FS_O_WRITE)) {
		p_pipe_desc =
			(struct pipe_desc *)((char *)ptr - offsetof(struct pipe_desc, flags_write));
		atomic_clear(&p_pipe_desc->write_opened);
		k_sem_give(&p_pipe_desc->sync);
	} else if ((*ptr & FS_O_READ) == FS_O_READ) {
		p_pipe_desc =
			(struct pipe_desc *)((char *)ptr - offsetof(struct pipe_desc, flags_read));
		atomic_clear(&p_pipe_desc->read_opened);
		k_sem_give(&p_pipe_desc->sync);
	} else {
		errno = EINVAL;
		return -1;
	}

	if (atomic_get(&p_pipe_desc->read_opened) == 0 &&
	    atomic_get(&p_pipe_desc->write_opened) == 0) {
		k_pipe_flush(&p_pipe_desc->pipe);
		atomic_clear(&p_pipe_desc->is_used);
	}

	return 0;
}

static int pipe_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	int *ptr = (int *)obj;
	struct pipe_desc *p_pipe_desc;

	if ((*ptr & FS_O_WRITE)) {
		p_pipe_desc =
			(struct pipe_desc *)((char *)ptr - offsetof(struct pipe_desc, flags_write));
	} else if ((*ptr & FS_O_READ)) {
		p_pipe_desc =
			(struct pipe_desc *)((char *)ptr - offsetof(struct pipe_desc, flags_read));
	} else {
		errno = EINVAL;
		return -1;
	}

	int fd;
	int flags;
	int ret = 0;

	switch (request) {
	case F_DUPFD:
		fd = va_arg(args, int);
		ret = zvfs_reserve_fd();
		if (ret == -1) {
			errno = ENFILE;
			return -1;
		}
		zvfs_finalize_fd(ret, obj, &fs_fd_op_vtable);
		break;

	case F_GETFL:
		if ((*ptr & (FS_O_READ | FS_O_WRITE)) == FS_O_READ) {
			ret = O_RDONLY;
		} else if ((*ptr & (FS_O_READ | FS_O_WRITE)) == FS_O_WRITE) {
			ret = O_WRONLY;
		} else {
			errno = EINVAL;
			ret = -1;
		}
		if (*ptr & O_NONBLOCK) {
			ret |= O_NONBLOCK;
		}
		break;

	case F_SETFL:
		flags = va_arg(args, int);
		if (flags & O_NONBLOCK) {
			(*ptr) = (*ptr) | O_NONBLOCK;
		} else {
			*ptr &= ~O_NONBLOCK;
		}
		ret = 0;
		break;

	default:
		errno = EINVAL;
		ret = -1;
		break;
	}

	return ret;
}

int pipe(int fildes[2])
{
	int fd1, fd2, i;

	fd1 = zvfs_reserve_fd();
	if (fd1 == -1) {
		errno = ENFILE;
		return -1;
	}

	fd2 = zvfs_reserve_fd();
	if (fd2 == -1) {
		errno = ENFILE;
		zvfs_free_fd(fd1);
		return -1;
	}

	fildes[0] = fd1;
	fildes[1] = fd2;

	for (i = 0; i < CONFIG_POSIX_PIPES_MAX; i++) {
		if (atomic_cas(&pipe_desc_array[i].is_used, 0, 1)) {
			break;
		}
	}

	if (i == CONFIG_POSIX_PIPES_MAX) {
		errno = EMFILE;
		return -1;
	}

	atomic_set(&pipe_desc_array[i].read_opened, 1);
	atomic_set(&pipe_desc_array[i].write_opened, 1);

	k_pipe_init(&(pipe_desc_array[i].pipe), pipe_desc_array[i].ring_buffer,
		    sizeof(pipe_desc_array[i].ring_buffer));

	pipe_desc_array[i].flags_read = FS_O_READ;
	pipe_desc_array[i].flags_write = FS_O_WRITE;

	k_sem_init(&pipe_desc_array[i].sync, 0, 1);

	zvfs_finalize_fd(fd1, &(pipe_desc_array[i].flags_read), &fs_fd_op_vtable);
	zvfs_finalize_fd(fd2, &(pipe_desc_array[i].flags_write), &fs_fd_op_vtable);

	return 0;
}
